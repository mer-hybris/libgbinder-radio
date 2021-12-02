/*
 * Copyright (C) 2021 Jolla Ltd.
 * Copyright (C) 2021 Slava Monich <slava.monich@jolla.com>
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

#include "radio_client_p.h"
#include "radio_instance_p.h"
#include "radio_request_p.h"
#include "radio_util.h"
#include "radio_log.h"

#include <gutil_macros.h>
#include <gutil_misc.h>

/* This API exists since 1.4.3 */

/*
 * Requests are considered pending for no longer than pending_timeout
 * because pending requests prevent blocking requests from being
 * submitted and we don't want to get stuck forever.
 */
#define DEFAULT_PENDING_TIMEOUT_MS (30000)

#define KEY(serial) GUINT_TO_POINTER(serial)

enum radio_events {
    RADIO_EVENT_IND,
    RADIO_EVENT_RESP,
    RADIO_EVENT_ACK,
    RADIO_EVENT_DEATH,
    RADIO_EVENT_CONNECTED,
    RADIO_EVENT_COUNT
};

typedef struct radio_client_object {
    RadioClient pub;
    gulong event_ids[RADIO_EVENT_COUNT];
    guint32 last_serial;
    GHashTable* requests;       /* All requests (weak references)  */
    GHashTable* active;         /* Requests in QUEUED and PENDING states  */
    GHashTable* pending;        /* Requests in PENDING state  */
    RadioRequest* queue_first;  /* First QUEUED request */
    RadioRequest* queue_last;   /* Last QUEUED request */
    RadioRequest* block_req;
    RadioRequestGroup* owner;
    GSList* owner_queue;

    gint64 next_deadline;       /* When the first PENDING request expires */
    guint default_timeout_ms;   /* Default PENDING timeout, milliseconds */
    guint timeout_id;
} RadioClientObject;

typedef GObjectClass RadioClientObjectClass;
GType radio_client_object_get_type() RADIO_INTERNAL;
G_DEFINE_TYPE(RadioClientObject, radio_client_object, G_TYPE_OBJECT)

#define PARENT_CLASS radio_client_object_parent_class
#define THIS_TYPE (radio_client_object_get_type())
#define THIS(obj) G_TYPE_CHECK_INSTANCE_CAST(obj, THIS_TYPE, RadioClientObject)

enum radio_client_signal {
    SIGNAL_INDICATION,
    SIGNAL_OWNER,
    SIGNAL_DEATH,
    SIGNAL_CONNECTED,
    SIGNAL_COUNT
};

#define SIGNAL_INDICATION_NAME "radio-client-indication"
#define SIGNAL_OWNER_NAME      "radio-client-owner"
#define SIGNAL_DEATH_NAME      "radio-client-death"
#define SIGNAL_CONNECTED_NAME  "radio-client-connected"

static guint radio_client_signals[SIGNAL_COUNT] = { 0 };

static inline RadioClientObject* radio_client_cast(RadioClient* client)
    { return client ? G_CAST(client, RadioClientObject, pub) : NULL; }
static inline gboolean radio_client_can_retry(RadioRequest* req)
    { return req->max_retries < 0 || req->max_retries > req->retry_count; }

static
void
radio_client_object_reset_timeout(
    RadioClientObject* self);

/*==========================================================================*
 * Implementation
 *==========================================================================*/

static
guint32
radio_client_reserve_serial(
    RadioClientObject* self)
{
    do { self->last_serial++; }
    while (!self->last_serial ||
        g_hash_table_contains(self->requests, KEY(self->last_serial)));
    return self->last_serial;
}

static
guint
radio_client_object_timeout_ms(
    RadioClientObject* self,
    RadioRequest* req)
{
    return req->timeout_ms ? req->timeout_ms : self->default_timeout_ms;
}

static
void
radio_client_cancel_transaction(
    RadioClientObject* self,
    RadioRequest* req)
{
    if (req->tx_id) {
        radio_instance_cancel_request(self->pub.instance, req->tx_id);
        req->tx_id = 0;
    }
}

static
void
radio_client_dequeue_request(
    RadioClientObject* self,
    RadioRequest* req,
    RadioRequest* prev)
{
    RadioRequest* next = req->queue_next;

    if (prev) {
        prev->queue_next = next;
    } else {
        self->queue_first = next;
    }
    if (next) {
        req->queue_next = NULL;
    } else {
        self->queue_last = prev;
    }
}

static
void
radio_client_deactivate_request(
    RadioClientObject* self,
    RadioRequest* req)
{
    g_hash_table_remove(self->pending, KEY(req->serial2));
    g_hash_table_remove(self->pending, KEY(req->serial));
    g_hash_table_remove(self->active, KEY(req->serial2));
    g_hash_table_remove(self->active, KEY(req->serial));
    if (req->state == RADIO_REQUEST_STATE_QUEUED) {
        RadioRequest* ptr = self->queue_first;
        RadioRequest* prev = NULL;

        /* Remove it from the queue */
        while (ptr) {
            if (ptr == req) {
                radio_client_dequeue_request(self, req, prev);
                break;
            }
            prev = ptr;
            ptr = ptr->queue_next;
        }
    }
    if (self->block_req == req) {
        /* Let the life continue */
        self->block_req = NULL;
        radio_request_unref(req);
    }
}

static
void
radio_client_drop_req(
    RadioClientObject* self,
    RadioRequest* req)
{
    radio_client_cancel_transaction(self, req);
    radio_client_deactivate_request(self, req);
    radio_client_object_reset_timeout(self);
    if (req->state < RADIO_REQUEST_STATE_FAILED) {
        req->state = RADIO_REQUEST_STATE_CANCELLED;
    }
}

static
void
radio_client_detach_req(
    gpointer key,
    gpointer value,
    gpointer user_data)
{
    RadioRequest* req = value;

    radio_client_cancel_transaction((RadioClientObject*)user_data, req);
    req->client = NULL;
}

static
void
radio_client_fail_request(
    RadioClientObject* self,
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
    radio_client_drop_req(self, req);
    radio_request_unref(req);
}

static
void
radio_client_request_failed(
    RadioClientObject* self,
    RadioRequest* req)
{
    radio_client_fail_request(self, req,
        RADIO_REQUEST_STATE_FAILED,
        RADIO_TX_STATUS_FAILED);
}

static
void
radio_client_tx_complete(
    RadioInstance* instance,
    gulong id,
    int status,
    void* user_data1,
    void* user_data2)
{
    RadioRequest* req = user_data2;

    req->tx_id = 0;
    if (status != GBINDER_STATUS_OK) {
        RadioClientObject* self = THIS(user_data1);

        g_object_ref(self);
        radio_client_request_failed(self, req);
        g_object_unref(self);
    }
}

static
gboolean
radio_client_submit_transaction(
    RadioClientObject* self,
    RadioRequest* req)
{
    if (req->serial2) {
        const guint32 used = req->serial2;

        /* Pick another serial */
        req->serial2 = radio_client_reserve_serial(self);
        g_hash_table_insert(self->requests, KEY(req->serial2), req);
        g_hash_table_insert(self->active, KEY(req->serial2),
            radio_request_ref(req));
        if (used != req->serial) {
            g_hash_table_remove(self->requests, KEY(used));
            g_hash_table_remove(self->pending, KEY(used));
            g_hash_table_remove(self->active, KEY(used));
        }
        radio_request_update_serial(req, req->serial2);
    } else {
        /* First submission */
        req->serial2 = req->serial;
    }

    /* Actually submit the transaction */
    req->tx_id = radio_instance_send_request(self->pub.instance,
        req->code, req->args, radio_client_tx_complete, NULL, self, req);
    if (req->tx_id) {
        req->state = RADIO_REQUEST_STATE_PENDING;
        g_hash_table_insert(self->pending, KEY(req->serial2),
            radio_request_ref(req));
        return TRUE;
    } else {
        return FALSE;
    }
}

static
gboolean
radio_client_can_set_owner(
    RadioClientObject* self,
    RadioRequestGroup* group)
{
    GHashTableIter it;
    gpointer value;

    g_hash_table_iter_init(&it, self->pending);
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
radio_client_queue_request(
    RadioClientObject* self,
    RadioRequest* req)
{
    req->state = RADIO_REQUEST_STATE_QUEUED;
    if (self->queue_last) {
        self->queue_last->queue_next = req;
    } else {
        self->queue_first = req;
    }
    self->queue_last = req;
}

static
guint
radio_client_submit_queued_requests(
    RadioClientObject* self)
{
    guint submitted = 0;

    if (!self->block_req && self->pub.instance->connected) {
        RadioRequest* prev = NULL;
        RadioRequest* req = self->queue_first;

        while (req) {
            if (self->owner == req->group ||
               (!self->owner && !self->owner_queue)) {
                /* Remove it from the queue */
                radio_client_dequeue_request(self, req, prev);

                /* Initiate the transaction and update the request state */
                if (radio_client_submit_transaction(self, req)) {
                    submitted++;
                    if (req->blocking) {
                        self->block_req = radio_request_ref(req);
                        break;
                    }
                } else {
                    radio_client_request_failed(self, req);
                }
            } else {
                prev = req;
            }
            req = req->queue_next;
        }
    }
    return submitted;
}

static
void
radio_client_handle_death(
    RadioInstance* instance,
    gpointer user_data)
{
    RadioClientObject* self = THIS(user_data);
    RadioRequest* dead = NULL;
    RadioRequest* req;
    GHashTableIter it;
    gpointer value;

    g_object_ref(self);

    /*
     * We are going to empty the queue and reuse queue_next link to
     * build the list of dead requests.
     */
    self->queue_first = self->queue_last = NULL;

    /* Steal all active requests from the table */
    g_hash_table_remove_all(self->pending);
    g_hash_table_iter_init(&it, self->active);
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
        radio_client_request_failed(self, req);
        radio_request_unref(req);
    }

    g_signal_emit(self, radio_client_signals[SIGNAL_DEATH], 0);
    g_object_unref(self);
}

static
void
radio_client_handle_connected(
    RadioInstance* instance,
    gpointer user_data)
{
    RadioClientObject* self = THIS(user_data);

    g_signal_emit(self, radio_client_signals[SIGNAL_CONNECTED], 0);
    if (radio_client_submit_queued_requests(self)) {
        radio_client_object_reset_timeout(self);
    }
}

static
void
radio_client_handle_ack(
    RadioInstance* instance,
    guint32 serial,
    gpointer user_data)
{
    RadioClientObject* self = THIS(user_data);
    RadioRequest* req = g_hash_table_lookup(self->active, KEY(serial));

    if (req) {
        GDEBUG("%08x acked", serial);
        req->acked = TRUE;
    } else {
        GWARN("%08x unexpected ack", serial);
    }
}

static
void
radio_client_handle_ind(
    RadioInstance* instance,
    RADIO_IND code,
    RADIO_IND_TYPE type,
    const GBinderReader* reader,
    gpointer user_data)
{
    RadioClientObject* self = THIS(user_data);

    g_signal_emit(self, radio_client_signals[SIGNAL_INDICATION],
        radio_instance_ind_quark(instance, code), code, reader);
}

static
void
radio_client_handle_resp(
    RadioInstance* instance,
    RADIO_RESP code,
    const RadioResponseInfo* info,
    const GBinderReader* reader,
    gpointer user_data)
{
    RadioClientObject* self = THIS(user_data);
    RadioRequest* req = g_hash_table_lookup(self->active, KEY(info->serial));

    if (req) {
        RadioRequestRetryFunc retry = req->retry;

        /* Temporary ref */
        g_object_ref(self);

        /* Response may come before completion of the request */
        radio_client_cancel_transaction(self, req);

        /* Do we need to retry? */
        if (radio_client_can_retry(req) && retry(req, RADIO_TX_STATUS_OK,
            code, info->error, reader, req->user_data)) {
            /* Re-queue the request */
            req->retry_count++;
            req->deadline = g_get_monotonic_time() +
                MICROSEC(req->retry_delay_ms);
            radio_client_queue_request(self, req);
        } else if (g_hash_table_steal(self->active, KEY(info->serial))) {
            req->state = RADIO_REQUEST_STATE_DONE;
            radio_client_deactivate_request(self, req);
            if (!self->owner && self->owner_queue) {
                /* There's a request group waiting to become the owner */
                GSList* l = self->owner_queue;
                RadioRequestGroup* group = l->data;

                if (radio_client_can_set_owner(self, group)) {
                    self->owner = group;
                    self->owner_queue = l->next;
                    g_slist_free_1(l);
                    g_signal_emit(self, radio_client_signals[SIGNAL_OWNER], 0);
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

        radio_client_submit_queued_requests(self);
        radio_client_object_reset_timeout(self);
        g_object_unref(self);
    } else {
        const char* name = radio_resp_name(code);

        /* Most likely this is a response to a cancelled request */
        GDEBUG("Ignoring response [%08x] %u %s", info->serial, code,
            name ? name : "");
    }
}

static
gboolean
radio_client_timeout(
    gpointer user_data)
{
    RadioClientObject* self = THIS(user_data);
    const gint64 now = g_get_monotonic_time();
    GHashTableIter it;
    gpointer value;
    GSList* expired = NULL;
    RadioRequest* req;

    g_object_ref(self);
    self->timeout_id = 0;

    /* Scan all active requests (including the queued ones) */
    g_hash_table_iter_init(&it, self->active);
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
            radio_client_deactivate_request(self, l->data);
        }

        /*
         * And this loop may and probably will invoke callbacks and may
         * even drop the last references to the requests.
         */
        for (l = expired; l; l = l->next) {
            req = l->data;
            radio_client_fail_request(self, req,
                RADIO_REQUEST_STATE_FAILED,
                RADIO_TX_STATUS_TIMEOUT);
            /* Drop the stolen reference */
            radio_request_unref(req);
        }

        /* Done with the list */
        g_slist_free(expired);

        /* Kick the request queue */
        radio_client_submit_queued_requests(self);
    }

    /* A retry timeout may have expired, check if we need to submit requests */
    radio_client_submit_queued_requests(self);
    radio_client_object_reset_timeout(self);
    g_object_unref(self);
    return G_SOURCE_REMOVE;
}

static
void
radio_client_object_reset_timeout(
    RadioClientObject* self)
{
    if (g_hash_table_size(self->active)) {
        GHashTableIter it;
        const gint64 now = g_get_monotonic_time();
        gint64 deadline = 0;
        gpointer value;

        /* Calculate the new deadline */
        g_hash_table_iter_init(&it, self->active);
        while (g_hash_table_iter_next(&it, NULL, &value)) {
            RadioRequest* req = value;

            if (!deadline || deadline > req->deadline) {
                deadline = req->deadline;
            }
        }

        if (self->next_deadline != deadline) {
            /* Convert to milliseconds */
            const guint timeout_ms = (deadline > now) ?
                (guint)(((deadline - now) + 999)/1000) : 0;

            /* Start or restart the timer (should it be suspend-aware?) */
            if (self->timeout_id) {
                g_source_remove(self->timeout_id);
            }
            GVERBOSE("Next timeout check in %u ms", timeout_ms);
            self->next_deadline = deadline;
            self->timeout_id = timeout_ms ?
                g_timeout_add(timeout_ms, radio_client_timeout, self) :
                g_idle_add(radio_client_timeout, self);
        }
    } else {
        /* No more pending requests, cancel the timeout */
        if (self->timeout_id) {
            g_source_remove(self->timeout_id);
            self->timeout_id = 0;
        }
    }
}

/*==========================================================================*
 * Internal API
 *==========================================================================*/

void
radio_client_register_request(
    RadioClient* client,
    RadioRequest* req)
{
    /* Caller makes sure that both arguments are not NULL */
    RadioClientObject* self = radio_client_cast(client);

    req->client = client;
    req->serial = radio_client_reserve_serial(self);
    g_hash_table_insert(self->requests, KEY(req->serial), req);
}

void
radio_client_unregister_request(
    RadioClient* client,
    RadioRequest* req)
{
    RadioClientObject* self = radio_client_cast(client);

    /* Caller doesn't check client for NULL */
    if (G_LIKELY(self)) {
        g_hash_table_remove(self->requests, KEY(req->serial));
        g_hash_table_remove(self->requests, KEY(req->serial2));
        req->serial = req->serial2 = 0;
        req->client = NULL;
    }
}

gboolean
radio_client_submit_request(
    RadioClient* client,
    RadioRequest* req)
{
    /*
     * Caller makes sure that both arguments are not NULL. Note that if the
     * client is dead, request stays in the NEW state and can be resubmitted
     * later (not sure if this is a useful feature though).
     */
    if (req->state == RADIO_REQUEST_STATE_NEW && !client->instance->dead) {
        RadioClientObject* self = radio_client_cast(client);
        RadioRequestCompleteFunc complete = req->complete;
        const uint timeout = radio_client_object_timeout_ms(self, req);

        /* Queue the request */
        req->deadline = g_get_monotonic_time() + MICROSEC(timeout);
        radio_client_queue_request(self, req);

        /* Create an internal reference to the request */
        g_hash_table_insert(self->active, KEY(req->serial),
            radio_request_ref(req));

        /* Don't complete the request if it fails right away */
        req->complete = NULL;
        if (radio_client_submit_queued_requests(self)) {
            radio_client_object_reset_timeout(self);
        }
        if (req->state < RADIO_REQUEST_STATE_FAILED) {
            req->complete = complete;
            return TRUE;
        }
    }
    return FALSE;
}

gboolean
radio_client_retry_request(
    RadioClient* client,
    RadioRequest* req)
{
    /* Caller makes sure that both arguments are not NULL */
    if (req->state == RADIO_REQUEST_STATE_PENDING &&
        radio_client_can_retry(req)) {
        RadioClientObject* self = radio_client_cast(client);

        radio_client_cancel_transaction(self, req);
        req->retry_count++;
        req->deadline = g_get_monotonic_time() + MICROSEC(req->retry_delay_ms);
        radio_client_queue_request(self, req);
        if (radio_client_submit_queued_requests(self)) {
            radio_client_object_reset_timeout(self);
        }
        return (req->state == RADIO_REQUEST_STATE_PENDING);
    }
    return FALSE;
}

void
radio_client_request_dropped(
    RadioRequest* req)
{
    if (req->client) {
        radio_client_drop_req(radio_client_cast(req->client), req);
    }
}

guint
radio_client_timeout_ms(
    RadioClient* client,
    RadioRequest* req)
{
    /* Caller makes sure that both arguments are not NULL */
    return radio_client_object_timeout_ms(radio_client_cast(client), req);
}

void
radio_client_reset_timeout(
    RadioClient* client)
{
    /* Caller makes sure that argument is not NULL */
    radio_client_object_reset_timeout(radio_client_cast(client));
}

RADIO_BLOCK
radio_client_block_status(
    RadioClient* client,
    RadioRequestGroup* group)
{
    RadioClientObject* self = radio_client_cast(client);

    /* Group is never NULL but client pointer isn't checked by the caller */
    if (G_LIKELY(self)) {
        if (self->owner == group) {
            return RADIO_BLOCK_ACQUIRED;
        } else if (g_slist_find(self->owner_queue, group)) {
            return RADIO_BLOCK_QUEUED;
        }
    }
    return RADIO_BLOCK_NONE;
}

RADIO_BLOCK
radio_client_block(
    RadioClient* client,
    RadioRequestGroup* group)
{
    RadioClientObject* self = radio_client_cast(client);
    RADIO_BLOCK state = RADIO_BLOCK_NONE;

    /* Group is never NULL but client pointer isn't checked by the caller */
    if (G_LIKELY(self)) {
        if (self->owner == group) {
            /* This group is already the owner */
            state = RADIO_BLOCK_ACQUIRED;
        } else if (!self->owner && !self->owner_queue &&
            radio_client_can_set_owner(self, group)) {
            self->owner = group;
            state = RADIO_BLOCK_ACQUIRED;
            g_signal_emit(self, radio_client_signals[SIGNAL_OWNER], 0);
        } else {
            if (!g_slist_find(self->owner_queue, group)) {
                /* Not found in the queue */
                self->owner_queue = g_slist_append(self->owner_queue, group);
            }
            state = RADIO_BLOCK_QUEUED;
        }
    }
    return state;
}

void
radio_client_unblock(
    RadioClient* client,
    RadioRequestGroup* group)
{
    RadioClientObject* self = radio_client_cast(client);

    /* Group is never NULL but client pointer isn't checked by the caller */
    if (G_LIKELY(self)) {
        if (self->owner == group) {
            if (self->owner_queue) {
                self->owner = self->owner_queue->data;
                self->owner_queue = g_slist_delete_link(self->owner_queue,
                    self->owner_queue);
            } else {
                self->owner = NULL;
            }
            g_signal_emit(self, radio_client_signals[SIGNAL_OWNER], 0);
            if (radio_client_submit_queued_requests(self)) {
                radio_client_object_reset_timeout(self);
            }
        } else {
            self->owner_queue = g_slist_remove(self->owner_queue, group);
        }
    }
}

static
gulong
radio_client_add_handler(
    RadioClient* client,
    enum radio_client_signal sig,
    RadioClientFunc fn,
    gpointer user_data)
{
    RadioClientObject* self = radio_client_cast(client);

    return (G_LIKELY(self) && G_LIKELY(fn)) ?
        g_signal_connect_closure_by_id(self, radio_client_signals[sig], 0,
             g_cclosure_new(G_CALLBACK(fn), user_data, NULL), FALSE) : 0;
}

/*==========================================================================*
 * API
 *==========================================================================*/

RadioClient*
radio_client_new(
    RadioInstance* instance)
{
    RadioClient* client = NULL;

    if (G_LIKELY(instance)) {
        RadioClientObject* self = g_object_new(THIS_TYPE, NULL);

        client = &self->pub;
        client->instance = radio_instance_ref(instance);
        self->event_ids[RADIO_EVENT_IND] =
            radio_instance_add_indication_observer(instance, RADIO_IND_ANY,
                radio_client_handle_ind, self);
        self->event_ids[RADIO_EVENT_RESP] =
            radio_instance_add_response_observer(instance, RADIO_RESP_ANY,
                radio_client_handle_resp, self);
        self->event_ids[RADIO_EVENT_ACK] =
            radio_instance_add_ack_handler(instance,
                radio_client_handle_ack, self);
        self->event_ids[RADIO_EVENT_DEATH] =
            radio_instance_add_death_handler(instance,
                radio_client_handle_death, self);
        self->event_ids[RADIO_EVENT_CONNECTED] =
            radio_instance_add_connected_handler(instance,
                radio_client_handle_connected, self);
    }
    return client;
}

RadioClient*
radio_client_ref(
    RadioClient* client)
{
    RadioClientObject* self = radio_client_cast(client);

    if (G_LIKELY(self)) {
        g_object_ref(client);
    }
    return client;
}

void
radio_client_unref(
    RadioClient* client)
{
    RadioClientObject* self = radio_client_cast(client);

    if (G_LIKELY(self)) {
        g_object_unref(self);
    }
}

const char*
radio_client_slot(
    RadioClient* client)
{
    return G_LIKELY(client) ? client->instance->slot : NULL;
}

gboolean
radio_client_dead(
    RadioClient* client)
{
    return !client || client->instance->dead;
}

gboolean
radio_client_connected(
    RadioClient* client)
{
    return client && client->instance->connected;
}

RADIO_INTERFACE
radio_client_interface(
    RadioClient* client)
{
    return G_LIKELY(client) ? client->instance->version : RADIO_INTERFACE_NONE;
}

void
radio_client_set_default_timeout(
    RadioClient* client,
    int ms)
{
    RadioClientObject* self = radio_client_cast(client);

    if (G_LIKELY(self)) {
        if (ms <= 0) {
            ms = DEFAULT_PENDING_TIMEOUT_MS;
        }
        if (self->default_timeout_ms != ms) {
            const int delta = ms - self->default_timeout_ms;
            gboolean updated = FALSE;
            GHashTableIter it;
            gpointer value;

            /* Calculate new deadlines */
            g_hash_table_iter_init(&it, self->active);
            while (g_hash_table_iter_next(&it, NULL, &value)) {
                RadioRequest* req = value;

                if (!req->timeout_ms) {
                    /* This request is using the default timeout */
                    req->deadline += MICROSEC(delta);
                    updated = TRUE;
                }
            }
            self->default_timeout_ms = ms;
            if (updated) {
                radio_client_object_reset_timeout(self);
            }
        }
    }
}

gulong
radio_client_add_indication_handler(
    RadioClient* client,
    RADIO_IND code,
    RadioClientIndicationFunc fn,
    gpointer user_data)
{
    RadioClientObject* self = radio_client_cast(client);

    return (G_LIKELY(self) && G_LIKELY(fn)) ?
        g_signal_connect_closure_by_id(self,
            radio_client_signals[SIGNAL_INDICATION],
            radio_instance_ind_quark(client->instance, code),
            g_cclosure_new(G_CALLBACK(fn), user_data, NULL), FALSE) : 0;
}

gulong
radio_client_add_owner_changed_handler(
    RadioClient* client,
    RadioClientFunc fn,
    gpointer user_data)
{
    return radio_client_add_handler(client, SIGNAL_OWNER, fn, user_data);
}

gulong
radio_client_add_death_handler(
    RadioClient* client,
    RadioClientFunc fn,
    gpointer user_data)
{
    return radio_client_add_handler(client, SIGNAL_DEATH, fn, user_data);
}

gulong
radio_client_add_connected_handler(
    RadioClient* client,
    RadioClientFunc fn,
    gpointer user_data)
{
    return radio_client_add_handler(client, SIGNAL_CONNECTED, fn, user_data);
}

void
radio_client_remove_handler(
    RadioClient* client,
    gulong id)
{
    if (G_LIKELY(id)) {
        RadioClientObject* self = radio_client_cast(client);

        if (G_LIKELY(self)) {
            g_signal_handler_disconnect(self, id);
        }
    }
}

void
radio_client_remove_handlers(
    RadioClient* client,
    gulong* ids,
    int count)
{
    gutil_disconnect_handlers(radio_client_cast(client), ids, count);
}

/*==========================================================================*
 * Internals
 *==========================================================================*/

static
void
radio_client_object_init(
    RadioClientObject* self)
{
    self->requests = g_hash_table_new(g_direct_hash, g_direct_equal);
    self->active = g_hash_table_new_full(g_direct_hash, g_direct_equal,
        NULL, radio_request_unref_func);
    self->pending = g_hash_table_new_full(g_direct_hash, g_direct_equal,
        NULL, radio_request_unref_func);
    self->default_timeout_ms = DEFAULT_PENDING_TIMEOUT_MS;
}

static
void
radio_client_object_finalize(
    GObject* object)
{
    RadioClientObject* self = THIS(object);
    RadioClient* client = &self->pub;

    if (self->timeout_id) {
        g_source_remove(self->timeout_id);
    }
    radio_instance_remove_all_handlers(client->instance, self->event_ids);
    radio_instance_unref(client->instance);
    g_hash_table_foreach(self->requests, radio_client_detach_req, self);
    g_hash_table_destroy(self->requests);
    g_hash_table_destroy(self->active);
    g_hash_table_destroy(self->pending);
    G_OBJECT_CLASS(PARENT_CLASS)->finalize(object);
}

static
void
radio_client_object_class_init(
    RadioClientObjectClass* klass)
{
    GType type = G_OBJECT_CLASS_TYPE(klass);

    G_OBJECT_CLASS(klass)->finalize = radio_client_object_finalize;
    radio_client_signals[SIGNAL_INDICATION] =
        g_signal_new(SIGNAL_INDICATION_NAME, type,
            G_SIGNAL_RUN_FIRST | G_SIGNAL_DETAILED, 0, NULL, NULL, NULL,
            G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_POINTER);
    radio_client_signals[SIGNAL_OWNER] =
        g_signal_new(SIGNAL_OWNER_NAME, type, G_SIGNAL_RUN_FIRST, 0,
            NULL, NULL, NULL, G_TYPE_NONE, 0);
    radio_client_signals[SIGNAL_DEATH] =
        g_signal_new(SIGNAL_DEATH_NAME, type, G_SIGNAL_RUN_FIRST, 0,
            NULL, NULL, NULL, G_TYPE_NONE, 0);
    radio_client_signals[SIGNAL_CONNECTED] =
        g_signal_new(SIGNAL_CONNECTED_NAME, type, G_SIGNAL_RUN_FIRST, 0,
            NULL, NULL, NULL, G_TYPE_NONE, 0);
}

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
