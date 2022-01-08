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

#include "radio_base.h"
#include "radio_client.h"
#include "radio_instance_p.h"
#include "radio_request_p.h"
#include "radio_util.h"
#include "radio_log.h"

#include <gutil_macros.h>
#include <gutil_misc.h>

/* This API exists since 1.4.3 */

enum radio_events {
    RADIO_EVENT_IND,
    RADIO_EVENT_RESP,
    RADIO_EVENT_ACK,
    RADIO_EVENT_DEATH,
    RADIO_EVENT_CONNECTED,
    RADIO_EVENT_COUNT
};

struct radio_client {
    RadioBase base;
    RadioInstance* instance;
    gulong event_ids[RADIO_EVENT_COUNT];
};

typedef RadioBaseClass RadioClientClass;
GType radio_client_get_type() RADIO_INTERNAL;
G_DEFINE_TYPE(RadioClient, radio_client, RADIO_TYPE_BASE)

#define PARENT_CLASS radio_client_parent_class
#define THIS_TYPE radio_client_get_type()
#define THIS(obj) G_TYPE_CHECK_INSTANCE_CAST(obj, THIS_TYPE, RadioClient)

typedef struct radio_client_call {
    RadioRequest* req;
    RadioBaseRequestSentFunc callback;
} RadioClientCall;

enum radio_client_signal {
    SIGNAL_INDICATION,
    SIGNAL_DEATH,
    SIGNAL_CONNECTED,
    SIGNAL_COUNT
};

#define SIGNAL_INDICATION_NAME "radio-client-indication"
#define SIGNAL_DEATH_NAME      "radio-client-death"
#define SIGNAL_CONNECTED_NAME  "radio-client-connected"

static guint radio_client_signals[SIGNAL_COUNT] = { 0 };

/*==========================================================================*
 * Implementation
 *==========================================================================*/

static
void
radio_client_call_free(
    RadioClientCall* call)
{
    radio_request_unref(call->req);
    gutil_slice_free(call);
}

static
void
radio_client_call_destroy(
    void* user_data1,
    void* user_data2)
{
    radio_client_call_free(user_data2);
}

static
void
radio_client_call_complete(
    RadioInstance* instance,
    gulong id,
    int status,
    void* user_data1,
    void* user_data2)
{
    RadioClientCall* call = user_data2;

    call->callback(RADIO_BASE(user_data1), call->req, status);
}

static
void
radio_client_handle_death(
    RadioInstance* instance,
    gpointer user_data)
{
    RadioBase* base = RADIO_BASE(user_data);

    radio_base_ref(base);
    radio_base_handle_death(base);
    g_signal_emit(base, radio_client_signals[SIGNAL_DEATH], 0);
    radio_base_unref(base);
}

static
void
radio_client_handle_connected(
    RadioInstance* instance,
    gpointer user_data)
{
    g_signal_emit(THIS(user_data), radio_client_signals[SIGNAL_CONNECTED], 0);
    radio_base_submit_requests(RADIO_BASE(user_data));
}

static
void
radio_client_handle_ack(
    RadioInstance* instance,
    guint32 serial,
    gpointer user_data)
{
    radio_base_handle_ack(RADIO_BASE(user_data), serial);
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
    g_signal_emit(THIS(user_data), radio_client_signals[SIGNAL_INDICATION],
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
    if (!radio_base_handle_resp(RADIO_BASE(user_data), code, info, reader)) {
        const char* name = radio_resp_name(code);

        /* Most likely this is a response to a cancelled request */
        GDEBUG("Ignoring IRadio response [%08x] %u %s", info->serial, code,
            name ? name : "");
    }
}

static
gulong
radio_client_add_handler(
    RadioClient* self,
    enum radio_client_signal sig,
    RadioClientFunc fn,
    gpointer user_data)
{
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
    RadioClient* self = NULL;

    if (G_LIKELY(instance)) {
        self = g_object_new(THIS_TYPE, NULL);
        radio_base_initialize(&self->base);
        self->instance = radio_instance_ref(instance);
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
    return self;
}

RadioClient*
radio_client_ref(
    RadioClient* self)
{
    if (G_LIKELY(self)) {
        g_object_ref(self);
    }
    return self;
}

void
radio_client_unref(
    RadioClient* self)
{
    if (G_LIKELY(self)) {
        g_object_unref(self);
    }
}

const char*
radio_client_slot(
    RadioClient* self)
{
    return G_LIKELY(self) ? self->instance->slot : NULL;
}

gboolean
radio_client_dead(
    RadioClient* self)
{
    return !self || self->instance->dead;
}

gboolean
radio_client_connected(
    RadioClient* self)
{
    return self && self->instance->connected;
}

RADIO_INTERFACE
radio_client_interface(
    RadioClient* self)
{
    return G_LIKELY(self) ? self->instance->version : RADIO_INTERFACE_NONE;
}

void
radio_client_set_default_timeout(
    RadioClient* self,
    int milliseconds)
{
    if (G_LIKELY(self)) {
        radio_base_set_default_timeout(&self->base, milliseconds);
    }
}

gulong
radio_client_add_indication_handler(
    RadioClient* self,
    RADIO_IND code,
    RadioClientIndicationFunc fn,
    gpointer user_data)
{
    return (G_LIKELY(self) && G_LIKELY(fn)) ?
        g_signal_connect_closure_by_id(self,
            radio_client_signals[SIGNAL_INDICATION],
            radio_instance_ind_quark(self->instance, code),
            g_cclosure_new(G_CALLBACK(fn), user_data, NULL), FALSE) : 0;
}

gulong
radio_client_add_owner_changed_handler(
    RadioClient* self,
    RadioClientFunc fn,
    gpointer user_data)
{
    return self ? radio_base_add_owner_changed_handler(&self->base,
        (RadioBaseFunc) fn, user_data) : 0;
}

gulong
radio_client_add_death_handler(
    RadioClient* self,
    RadioClientFunc fn,
    gpointer user_data)
{
    return radio_client_add_handler(self, SIGNAL_DEATH, fn, user_data);
}

gulong
radio_client_add_connected_handler(
    RadioClient* self,
    RadioClientFunc fn,
    gpointer user_data)
{
    return radio_client_add_handler(self, SIGNAL_CONNECTED, fn, user_data);
}

void
radio_client_remove_handler(
    RadioClient* self,
    gulong id)
{
    if (G_LIKELY(id) && G_LIKELY(self)) {
        g_signal_handler_disconnect(self, id);
    }
}

void
radio_client_remove_handlers(
    RadioClient* self,
    gulong* ids,
    int count)
{
    gutil_disconnect_handlers(self, ids, count);
}

/*==========================================================================*
 * Methods
 *==========================================================================*/

static
gboolean
radio_client_is_dead(
    RadioBase* base)
{
    return THIS(base)->instance->dead;
}

static
gboolean
radio_client_can_submit_requests(
    RadioBase* base)
{
    return THIS(base)->instance->connected;
}

static
GBinderLocalRequest*
radio_client_new_request(
    RadioBase* base,
    guint32 code)
{
    return radio_instance_new_request(THIS(base)->instance, code);
}

static
gulong
radio_client_send_request(
    RadioBase* base,
    RadioRequest* req,
    RadioBaseRequestSentFunc callback)
{
    RadioClientCall* call = g_slice_new(RadioClientCall);
    gulong tx_id;

    call->callback = callback;
    call->req = radio_request_ref(req);
    tx_id = radio_instance_send_request(THIS(base)->instance,
        req->code, req->args, radio_client_call_complete,
        radio_client_call_destroy, base, call);
    if (tx_id) {
        return tx_id;
    } else {
        radio_client_call_free(call);
        return 0;
    }
}

static
void
radio_client_cancel_request(
    RadioBase* base,
    gulong id)
{
    radio_instance_cancel_request(THIS(base)->instance, id);
}

/*==========================================================================*
 * Internals
 *==========================================================================*/

static
void
radio_client_init(
    RadioClient* self)
{
}

static
void
radio_client_finalize(
    GObject* object)
{
    RadioClient* self = THIS(object);

    radio_instance_remove_all_handlers(self->instance, self->event_ids);
    radio_instance_unref(self->instance);
    G_OBJECT_CLASS(PARENT_CLASS)->finalize(object);
}

static
void
radio_client_class_init(
    RadioClientClass* klass)
{
    RadioBaseClass* base_class = RADIO_BASE_CLASS(klass);
    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    GType type = G_OBJECT_CLASS_TYPE(klass);

    base_class->is_dead = radio_client_is_dead;
    base_class->can_submit_requests = radio_client_can_submit_requests;
    base_class->new_request = radio_client_new_request;
    base_class->send_request = radio_client_send_request;
    base_class->cancel_request = radio_client_cancel_request;
    object_class->finalize = radio_client_finalize;

    radio_client_signals[SIGNAL_INDICATION] =
        g_signal_new(SIGNAL_INDICATION_NAME, type,
            G_SIGNAL_RUN_FIRST | G_SIGNAL_DETAILED, 0, NULL, NULL, NULL,
            G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_POINTER);
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
