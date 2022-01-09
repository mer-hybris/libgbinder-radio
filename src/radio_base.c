/*
 * Copyright (C) 2021-2022 Jolla Ltd.
 * Copyright (C) 2021-2022 Slava Monich <slava.monich@jolla.com>
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *   3. Neither the names of the copyright holders nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * any official policies, either expressed or implied.
 */

#define GLIB_DISABLE_DEPRECATION_WARNINGS

#include "radio_base.h"
#include "radio_request_p.h"
#include "radio_util.h"
#include "radio_log.h"

/*
 * Requests are considered pending for no longer than pending_timeout
 * because pending requests prevent blocking requests from being
 * submitted and we don't want to get stuck forever.
 */
#define DEFAULT_PENDING_TIMEOUT_MS (30000)

#define KEY(serial) GUINT_TO_POINTER(serial)

struct radio_base_priv {
    GHashTable* requests;       /* All requests (weak references)  */
    GHashTable* active;         /* Requests in QUEUED and PENDING states  */
    GHashTable* pending;        /* Requests in PENDING state  */
    RadioRequest* queue_first;  /* First QUEUED request */
    RadioRequest* queue_last;   /* Last QUEUED request */
    RadioRequest* block_req;
    RadioRequestGroup* owner;
    GSList* owner_queue;

    gint64 next_wakeup;         /* When the next timer is scheduled */
    guint default_timeout_ms;   /* Default PENDING timeout, milliseconds */
    guint timeout_id;
};

#define PARENT_CLASS radio_base_parent_class
#define THIS_TYPE RADIO_TYPE_BASE
#define THIS(obj) RADIO_BASE(obj)
G_DEFINE_ABSTRACT_TYPE(RadioBase, radio_base, G_TYPE_OBJECT)

enum radio_base_signal {
    SIGNAL_OWNER,
    SIGNAL_COUNT
};

#define SIGNAL_OWNER_NAME      "radio-base-owner"

static guint radio_base_signals[SIGNAL_COUNT] = { 0 };

static inline gboolean
radio_base_can_retry(RadioRequest* req)
    { return req->max_retries < 0 || req->max_retries > req->retry_count; }

static inline gboolean
radio_base_request_blocked(RadioBasePriv* q, RadioRequest* req)
    { return (q->owner != req->group &&
             (q->owner || q->owner_queue)) ||
             (q->block_req && q->block_req != req); }

/*==========================================================================*
 * Implementation
 *==========================================================================*/

static
guint32
radio_base_reserve_serial(
    RadioBase* self)
{
    /*
     * Using a static counter practically guarantees that different
     * serials would never be in use simultanously by different
     * RadioBase-derived objects. And hopefully reduce the chance
     * of confusion on the service side.
     */
    static guint32 last_serial = 0;
    RadioBasePriv* priv = self->priv;

    do { last_serial++; }
    while (!last_serial ||
        g_hash_table_contains(priv->requests, KEY(last_serial)));
    return last_serial;
}

static
void
radio_base_unlink_request(
    RadioBasePriv* q,
    RadioRequest* req,
    RadioRequest* prev)
{
    RadioRequest* next = req->queue_next;

    if (prev) {
        prev->queue_next = next;
    } else {
        q->queue_first = next;
    }
    if (next) {
        req->queue_next = NULL;
    } else {
        q->queue_last = prev;
    }
}

static
gboolean
radio_base_dequeue_request(
    RadioBasePriv* q,
    RadioRequest* req)
{
    RadioRequest* ptr = q->queue_first;
    RadioRequest* prev = NULL;

    while (ptr) {
        if (ptr == req) {
            radio_base_unlink_request(q, req, prev);
            return TRUE;
        }
        prev = ptr;
        ptr = ptr->queue_next;
    }
    return FALSE;
}

static
void
radio_base_deactivate_request(
    RadioBase* self,
    RadioRequest* req)
{
    RadioBasePriv* priv = self->priv;

    g_hash_table_remove(priv->pending, KEY(req->serial2));
    g_hash_table_remove(priv->pending, KEY(req->serial));
    g_hash_table_remove(priv->active, KEY(req->serial2));
    g_hash_table_remove(priv->active, KEY(req->serial));
    if (req->state == RADIO_REQUEST_STATE_QUEUED) {
        radio_base_dequeue_request(priv, req);
    }
    if (priv->block_req == req) {
        /* Let the life continue */
        priv->block_req = NULL;
        radio_request_unref(req);
    }
}

static
void
radio_base_drop_req(
    RadioBase* self,
    RadioRequest* req)
{
    radio_base_cancel_request(self, req);
    radio_base_deactivate_request(self, req);
    radio_base_reset_timeout(self);
    if (req->state < RADIO_REQUEST_STATE_FAILED) {
        req->state = RADIO_REQUEST_STATE_CANCELLED;
    }
}

static
void
radio_base_detach_req(
    gpointer key,
    gpointer value,
    gpointer user_data)
{
    RadioRequest* req = value;

    radio_base_cancel_request(THIS(user_data), req);
    req->object = NULL;
}

static
void
radio_base_fail_request(
    RadioBase* self,
    RadioRequest* req,
    RADIO_REQUEST_STATE state,
    RADIO_TX_STATUS status)
{
    radio_request_ref(req);
    req->state = state;
    if (req->complete) {
        RadioRequestCompleteFunc complete = req->complete;

        req->complete = NULL;
        complete(req, status, RADIO_RESP_NONE, RADIO_ERROR_NONE, NULL,
            req->user_data);
    }
    radio_base_drop_req(self, req);
    radio_request_unref(req);
}

static
void
radio_base_request_failed(
    RadioBase* self,
    RadioRequest* req)
{
    radio_base_fail_request(self, req,
        RADIO_REQUEST_STATE_FAILED,
        RADIO_TX_STATUS_FAILED);
}

static
void
radio_base_request_sent(
    RadioBase* self,
    RadioRequest* req,
    int status)
{
    req->tx_id = 0;
    if (status != GBINDER_STATUS_OK) {
        g_object_ref(self);
        radio_base_request_failed(self, req);
        g_object_unref(self);
    }
}

static
gboolean
radio_base_submit_transaction(
    RadioBase* self,
    RadioRequest* req)
{
    RadioBasePriv* priv = self->priv;

    if (req->serial2) {
        const guint32 used = req->serial2;

        /* Pick another serial and record it */
        req->serial2 = radio_base_reserve_serial(self);
        g_hash_table_insert(priv->requests, KEY(req->serial2), req);
        g_hash_table_insert(priv->active, KEY(req->serial2),
            radio_request_ref(req));

        /* Drop the old one */
        g_hash_table_remove(priv->pending, KEY(used));
        g_hash_table_remove(priv->active, KEY(used));

        /* Keep the original serial in priv->requests */
        if (used != req->serial) {
            g_hash_table_remove(priv->requests, KEY(used));
        }

        /* Update the RPC header */
        radio_request_update_serial(req, req->serial2);
        GDEBUG("Resubmitting request %u [%08x => %08x]",
            req->code, used, req->serial2);
    } else {
        /* First submission */
        req->serial2 = req->serial;
    }

    /* Actually submit the transaction */
    req->tx_id = RADIO_BASE_GET_CLASS(self)->send_request(self, req,
        radio_base_request_sent);
    if (req->tx_id) {
        req->scheduled = 0; /* Not scheduled anymore */
        req->state = RADIO_REQUEST_STATE_PENDING;
        g_hash_table_insert(priv->pending, KEY(req->serial2),
            radio_request_ref(req));
        return TRUE;
    } else {
        return FALSE;
    }
}

static
gboolean
radio_base_can_set_owner(
    RadioBase* self,
    RadioRequestGroup* group)
{
    RadioBasePriv* priv = self->priv;
    GHashTableIter it;
    gpointer value;

    g_hash_table_iter_init(&it, priv->pending);
    while (g_hash_table_iter_next(&it, NULL, &value)) {
        RadioRequest* req = value;

        if (req->group != group) {
            /*
             * There's a pending request not associated with any group
             * or associated with a different group. The specified group
             * can't become the owner just yet.
             */
            return FALSE;
        }
    }

    /*
     * There are no pending requests, or all pending requests are
     * associated with the specified group. This group can be the owner.
     */
    return TRUE;
}

static
void
radio_base_queue_request(
    RadioBasePriv* priv,
    RadioRequest* req)
{
    req->state = RADIO_REQUEST_STATE_QUEUED;
    if (priv->queue_last) {
        priv->queue_last->queue_next = req;
    } else {
        priv->queue_first = req;
    }
    priv->queue_last = req;
}

static
guint
radio_base_submit_queued_requests(
    RadioBase* self)
{
    RadioBasePriv* priv = self->priv;
    guint submitted = 0;

    if (RADIO_BASE_GET_CLASS(self)->can_submit_requests(self)) {
        RadioRequest* prev = NULL;
        RadioRequest* req = priv->queue_first;
        const gint64 now = g_get_monotonic_time();

        while (req) {
            RadioRequest* next = req->queue_next;

            if (!radio_base_request_blocked(priv, req) &&
                /* If the request is scheduled, don't submit it too early */
                (!req->scheduled || now >= req->scheduled)) {
                /* Remove it from the queue (prev remains untouched) */
                radio_base_unlink_request(priv, req, prev);

                /* Initiate the transaction and update the request state */
                if (radio_base_submit_transaction(self, req)) {
                    submitted++;
                    if (req->blocking) {
                        priv->block_req = radio_request_ref(req);
                        break;
                    }
                } else {
                    radio_base_request_failed(self, req);
                }
            } else {
                /* What hasn't been removed from the queue, becomes prev */
                prev = req;
            }
            req = next;
        }
    }
    return submitted;
}

static
gboolean
radio_base_timeout(
    gpointer user_data)
{
    RadioBase* self = THIS(user_data);
    RadioBasePriv* priv = self->priv;
    const gint64 now = g_get_monotonic_time();
    GHashTableIter it;
    gpointer value;
    GSList* expired = NULL;
    RadioRequest* req;

    g_object_ref(self);
    priv->timeout_id = 0;

    /* Scan all active requests (including the queued ones) */
    g_hash_table_iter_init(&it, priv->active);
    while (g_hash_table_iter_next(&it, NULL, &value)) {
        req = value;
        if (req->deadline <= now) {
            GDEBUG("Request %u (%08x/%08x) expired",
                req->code, req->serial, req->serial2);

            /* Don't unref those just yet */
            g_hash_table_iter_steal(&it);
            expired = g_slist_append(expired, req);
        }
    }

    if (expired) {
        GSList* l;

        /*
         * We are still holding the stolen reference for each expired request,
         * meaning this won't destroy them and won't invoke any callbacks.
         */
        for (l = expired; l; l = l->next) {
            radio_base_deactivate_request(self, l->data);
        }

        /*
         * And this loop may and probably will invoke callbacks and may
         * even drop the last references to the requests.
         */
        for (l = expired; l; l = l->next) {
            req = l->data;
            radio_base_fail_request(self, req,
                RADIO_REQUEST_STATE_FAILED,
                RADIO_TX_STATUS_TIMEOUT);
            /* Drop the stolen reference */
            radio_request_unref(req);
        }

        /* Done with the list */
        g_slist_free(expired);
    }

    /* A retry timeout may have expired, check if we need to submit requests */
    radio_base_submit_queued_requests(self);
    radio_base_reset_timeout(self);
    g_object_unref(self);
    return G_SOURCE_REMOVE;
}

/*==========================================================================*
 * Internal API
 *==========================================================================*/

void
radio_base_initialize(
    RadioBase* self)
{
}

void
radio_base_register_request(
    RadioBase* self,
    RadioRequest* req)
{
    /* Caller makes sure that both arguments are not NULL */
    RadioBasePriv* priv = self->priv;

    req->object = self;
    req->serial = radio_base_reserve_serial(self);
    g_hash_table_insert(priv->requests, KEY(req->serial), req);
}

void
radio_base_unregister_request(
    RadioBase* self,
    RadioRequest* req)
{
    /* Caller doesn't check base for NULL */
    if (G_LIKELY(self)) {
        RadioBasePriv* priv = self->priv;

        g_hash_table_remove(priv->requests, KEY(req->serial));
        g_hash_table_remove(priv->requests, KEY(req->serial2));
        req->serial = req->serial2 = 0;
        req->object = NULL;
    }
}

gboolean
radio_base_submit_request(
    RadioBase* self,
    RadioRequest* req)
{
    /*
     * Caller makes sure that both arguments are not NULL. Note that if the
     * base is dead, request stays in the NEW state and can be resubmitted
     * later (not sure if this is a useful feature though).
     */
    if (req->state == RADIO_REQUEST_STATE_NEW &&
        !RADIO_BASE_GET_CLASS(self)->is_dead(self)) {
        RadioBasePriv* priv = self->priv;
        RadioRequestCompleteFunc complete = req->complete;
        const guint timeout = radio_base_timeout_ms(self, req);

        /* Queue the request */
        req->deadline = g_get_monotonic_time() + MICROSEC(timeout);
        radio_base_queue_request(priv, req);

        /* Create an internal reference to the request */
        g_hash_table_insert(priv->active, KEY(req->serial),
            radio_request_ref(req));

        /* Don't complete the request if it fails right away */
        req->complete = NULL;
        if (radio_base_submit_queued_requests(self)) {
            radio_base_reset_timeout(self);
        }
        if (req->state < RADIO_REQUEST_STATE_FAILED) {
            req->complete = complete;
            return TRUE;
        }
    }
    return FALSE;
}

gboolean
radio_base_retry_request(
    RadioBase* self,
    RadioRequest* req)
{
    /* Caller makes sure that both arguments are not NULL */
    if (req->state == RADIO_REQUEST_STATE_PENDING &&
        radio_base_can_retry(req)) {
        RadioBasePriv* priv = self->priv;

        radio_base_cancel_request(self, req);
        req->retry_count++;
        radio_base_queue_request(priv, req);
        if (radio_base_submit_queued_requests(self)) {
            radio_base_reset_timeout(self);
        }
        return (req->state == RADIO_REQUEST_STATE_PENDING);
    }
    return FALSE;
}

void
radio_base_request_dropped(
    RadioRequest* req)
{
    if (req->object) {
        radio_base_drop_req(req->object, req);
    }
}

guint
radio_base_timeout_ms(
    RadioBase* self,
    RadioRequest* req)
{
    /* Caller checks object pointer for NULL */
    return req->timeout_ms ? req->timeout_ms : self->priv->default_timeout_ms;
}

void
radio_base_reset_timeout(
    RadioBase* self)
{
    RadioBasePriv* priv = self->priv;

    if (g_hash_table_size(priv->active)) {
        GHashTableIter it;
        const gint64 now = g_get_monotonic_time();
        gint64 next_wakeup = 0;
        gpointer value;

        /* Calculate the new deadline */
        g_hash_table_iter_init(&it, priv->active);
        while (g_hash_table_iter_next(&it, NULL, &value)) {
            RadioRequest* req = value;

            if (!next_wakeup || next_wakeup > req->deadline) {
                next_wakeup = req->deadline;
            }
            if (req->scheduled && !radio_base_request_blocked(priv, req)) {
                if (!next_wakeup || next_wakeup > req->scheduled) {
                    next_wakeup = req->scheduled;
                }
            }
        }

        if (next_wakeup) {
            /* Convert to milliseconds */
            const guint timeout_ms = (next_wakeup > now) ?
                (guint)(((next_wakeup - now) + 999)/1000) : 0;

            /* Start or restart the timer (should it be suspend-aware?) */
            if (priv->timeout_id) {
                g_source_remove(priv->timeout_id);
            }
            GVERBOSE("Next timeout check in %u ms", timeout_ms);
            priv->next_wakeup = next_wakeup;
            priv->timeout_id = timeout_ms ?
                g_timeout_add(timeout_ms, radio_base_timeout, self) :
                g_idle_add(radio_base_timeout, self);
        }
    } else {
        /* No more pending requests, cancel the timeout */
        if (priv->timeout_id) {
            g_source_remove(priv->timeout_id);
            priv->timeout_id = 0;
        }
    }
}

RADIO_BLOCK
radio_base_block_status(
    RadioBase* self,
    RadioRequestGroup* group)
{
    RadioBasePriv* priv = self->priv;

    /* Caller checks object pointer for NULL */
    if (priv->owner == group) {
        return RADIO_BLOCK_ACQUIRED;
    } else if (g_slist_find(priv->owner_queue, group)) {
        return RADIO_BLOCK_QUEUED;
    } else {
        return RADIO_BLOCK_NONE;
    }
}

RADIO_BLOCK
radio_base_block(
    RadioBase* self,
    RadioRequestGroup* group)
{
    RadioBasePriv* priv = self->priv;

    /* Caller checks object pointer for NULL */
    if (priv->owner == group) {
        /* This group is already the owner */
        return  RADIO_BLOCK_ACQUIRED;
    } else if (!priv->owner && !priv->owner_queue &&
        radio_base_can_set_owner(self, group)) {
        priv->owner = group;
        g_signal_emit(self, radio_base_signals[SIGNAL_OWNER], 0);
        return RADIO_BLOCK_ACQUIRED;
    } else {
        if (!g_slist_find(priv->owner_queue, group)) {
            /* Not found in the queue */
            priv->owner_queue = g_slist_append(priv->owner_queue, group);
        }
        return RADIO_BLOCK_QUEUED;
    }
}

void
radio_base_unblock(
    RadioBase* self,
    RadioRequestGroup* group)
{
    /* Group is never NULL but base pointer isn't checked by the caller */
    if (G_LIKELY(self)) {
        RadioBasePriv* priv = self->priv;

        if (priv->owner == group) {
            if (priv->owner_queue) {
                priv->owner = priv->owner_queue->data;
                priv->owner_queue = g_slist_delete_link(priv->owner_queue,
                    priv->owner_queue);
            } else {
                priv->owner = NULL;
            }
            g_signal_emit(self, radio_base_signals[SIGNAL_OWNER], 0);
            if (radio_base_submit_queued_requests(self)) {
                radio_base_reset_timeout(self);
            }
        } else {
            priv->owner_queue = g_slist_remove(priv->owner_queue, group);
        }
    }
}

gboolean
radio_base_handle_resp(
    RadioBase* self,
    guint32 code,
    const RadioResponseInfo* info,
    const GBinderReader* reader)
{
    RadioBasePriv* priv = self->priv;
    RadioRequest* req = g_hash_table_lookup(priv->active, KEY(info->serial));

    if (req) {
        RadioRequestRetryFunc retry = req->retry;

        /* Temporary ref */
        g_object_ref(self);

        /* Response may come before completion of the request */
        radio_base_cancel_request(self, req);

        /* Do we need to retry? */
        if (radio_base_can_retry(req) && retry(req, RADIO_TX_STATUS_OK,
            code, info->error, reader, req->user_data)) {
            /* Re-queue the request */
            req->retry_count++;
            req->scheduled = g_get_monotonic_time() +
                MICROSEC(req->retry_delay_ms);
            radio_base_queue_request(priv, req);
        } else if (g_hash_table_steal(priv->active, KEY(info->serial))) {
            req->state = RADIO_REQUEST_STATE_DONE;
            radio_base_deactivate_request(self, req);
            if (!priv->owner && priv->owner_queue) {
                /* There's a request group waiting to become the owner */
                GSList* l = priv->owner_queue;
                RadioRequestGroup* group = l->data;

                if (radio_base_can_set_owner(self, group)) {
                    priv->owner = group;
                    priv->owner_queue = l->next;
                    g_slist_free_1(l);
                    g_signal_emit(self, radio_base_signals[SIGNAL_OWNER], 0);
                }
            }
            if (req->complete) {
                RadioRequestCompleteFunc fn = req->complete;

                req->complete = NULL;
                fn(req, RADIO_TX_STATUS_OK, code, info->error, reader,
                    req->user_data);
            }
            radio_request_unref(req);
        }

        radio_base_submit_queued_requests(self);
        radio_base_reset_timeout(self);
        g_object_unref(self);
        return TRUE;
    }
    /* Most likely, the corresponding request was cancelled */
    return FALSE;
}

void
radio_base_handle_ack(
    RadioBase* self,
    guint32 serial)
{
    RadioBasePriv* priv = self->priv;
    RadioRequest* req = g_hash_table_lookup(priv->active, KEY(serial));

    if (req) {
        GDEBUG("%08x acked", serial);
        req->acked = TRUE;
    } else {
        GWARN("%08x unexpected ack", serial);
    }
}

void
radio_base_handle_death(
    RadioBase* self)
{
    RadioBasePriv* priv = self->priv;
    RadioRequest* dead = NULL;
    RadioRequest* req;
    GHashTableIter it;
    gpointer value;

    /*
     * We are going to empty the queue and reuse queue_next link to
     * build the list of dead requests.
     */
    priv->queue_first = priv->queue_last = NULL;

    /* Steal all active requests from the table */
    g_hash_table_remove_all(priv->pending);
    g_hash_table_iter_init(&it, priv->active);
    while (g_hash_table_iter_next(&it, NULL, &value)) {
        req = value;
        GDEBUG("Dropping request %u (%08x/%08x) due to radio death",
            req->code, req->serial, req->serial2);

        /* Don't unref them just yet */
        g_hash_table_iter_steal(&it);

        /*
         * Make sure that request state isn't RADIO_REQUEST_STATE_QUEUED,
         * so that we can safely reuse queue_next link.
         */
        req->state = RADIO_REQUEST_STATE_QUEUED;
        req->queue_next = dead;
        dead = req;
    }

    while (dead) {
        /* Fail the dead request and drop the stolen ref */
        req = dead;
        dead = req->queue_next;
        req->queue_next = NULL;
        radio_base_request_failed(self, req);
        radio_request_unref(req);
    }
}

void
radio_base_submit_requests(
    RadioBase* self)
{
    if (radio_base_submit_queued_requests(self)) {
        radio_base_reset_timeout(self);
    }
}

void
radio_base_cancel_request(
    RadioBase* self,
    RadioRequest* req)
{
    req->scheduled = 0;
    if (req->tx_id) {
        RADIO_BASE_GET_CLASS(self)->cancel_request(self, req->tx_id);
        req->tx_id = 0;
    }
}

void
radio_base_set_default_timeout(
    RadioBase* self,
    int ms)
{
    /* Caller checks object pointer for NULL */
    RadioBasePriv* priv = self->priv;

    if (ms <= 0) {
        ms = DEFAULT_PENDING_TIMEOUT_MS;
    }
    if (priv->default_timeout_ms != ms) {
        const int delta = ms - priv->default_timeout_ms;
        gboolean updated = FALSE;
        GHashTableIter it;
        gpointer value;

        /* Calculate new deadlines */
        g_hash_table_iter_init(&it, priv->active);
        while (g_hash_table_iter_next(&it, NULL, &value)) {
            RadioRequest* req = value;

            if (!req->timeout_ms) {
                /* This request is using the default timeout */
                req->deadline += MICROSEC(delta);
                updated = TRUE;
            }
        }
        priv->default_timeout_ms = ms;
        if (updated) {
            radio_base_reset_timeout(self);
        }
    }
}

gulong
radio_base_add_owner_changed_handler(
    RadioBase* self,
    RadioBaseFunc fn,
    gpointer user_data)
{
    /* Caller checks object pointer for NULL */
    return G_LIKELY(fn) ?
        g_signal_connect_closure_by_id(self, radio_base_signals[SIGNAL_OWNER],
             0, g_cclosure_new(G_CALLBACK(fn), user_data, NULL), FALSE) : 0;
}

/*==========================================================================*
 * Internals
 *==========================================================================*/

static
void
radio_base_init(
    RadioBase* self)
{
    RadioBasePriv* priv = G_TYPE_INSTANCE_GET_PRIVATE(self, RADIO_TYPE_BASE,
        RadioBasePriv);

    self->priv = priv;
    priv->requests = g_hash_table_new(g_direct_hash, g_direct_equal);
    priv->active = g_hash_table_new_full(g_direct_hash, g_direct_equal,
        NULL, radio_request_unref_func);
    priv->pending = g_hash_table_new_full(g_direct_hash, g_direct_equal,
        NULL, radio_request_unref_func);
    priv->default_timeout_ms = DEFAULT_PENDING_TIMEOUT_MS;
}

static
void
radio_base_object_finalize(
    GObject* object)
{
    RadioBase* self = THIS(object);
    RadioBasePriv* priv = self->priv;

    if (priv->timeout_id) {
        g_source_remove(priv->timeout_id);
    }
    g_hash_table_foreach(priv->requests, radio_base_detach_req, self);
    g_hash_table_destroy(priv->requests);
    g_hash_table_destroy(priv->active);
    g_hash_table_destroy(priv->pending);
    G_OBJECT_CLASS(PARENT_CLASS)->finalize(object);
}

static
void
radio_base_class_init(
    RadioBaseClass* klass)
{
    g_type_class_add_private(klass, sizeof(RadioBasePriv));
    G_OBJECT_CLASS(klass)->finalize = radio_base_object_finalize;
    radio_base_signals[SIGNAL_OWNER] = g_signal_new(SIGNAL_OWNER_NAME,
        G_OBJECT_CLASS_TYPE(klass), G_SIGNAL_RUN_FIRST, 0, NULL, NULL, NULL,
        G_TYPE_NONE, 0);
}

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
