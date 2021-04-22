/*
 * Copyright (C) 2018-2021 Jolla Ltd.
 * Copyright (C) 2018-2021 Slava Monich <slava.monich@jolla.com>
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

#include "radio_instance.h"
#include "radio_registry_p.h"
#include "radio_util.h"
#include "radio_log.h"

#include <gbinder.h>

#include <gutil_idlepool.h>
#include <gutil_misc.h>
#include <gutil_strv.h>

#include <glib-object.h>

typedef GObjectClass RadioInstanceClass;
struct radio_instance_priv {
    GUtilIdlePool* idle;
    GBinderClient* client;
    GBinderRemoteObject* remote;
    GBinderLocalObject* response;
    GBinderLocalObject* indication;
    GHashTable* resp_quarks;
    GHashTable* ind_quarks;
    gulong death_id;
    char* dev;
    char* slot;
    char* key;
    char* modem;
};

G_DEFINE_TYPE(RadioInstance, radio_instance, G_TYPE_OBJECT)

enum radio_instance_signal {
    SIGNAL_HANDLE_INDICATION,
    SIGNAL_HANDLE_RESPONSE,
    SIGNAL_OBSERVE_INDICATION,
    SIGNAL_OBSERVE_RESPONSE,
    SIGNAL_ACK,
    SIGNAL_DEATH,
    SIGNAL_ENABLED,
    SIGNAL_COUNT
};

#define SIGNAL_HANDLE_INDICATION_NAME  "radio-instance-handle-indication"
#define SIGNAL_HANDLE_RESPONSE_NAME    "radio-instance-handle-response"
#define SIGNAL_OBSERVE_INDICATION_NAME "radio-instance-observe-indication"
#define SIGNAL_OBSERVE_RESPONSE_NAME   "radio-instance-observe-response"
#define SIGNAL_ACK_NAME                "radio-instance-ack"
#define SIGNAL_DEATH_NAME              "radio-instance-death"
#define SIGNAL_ENABLED_NAME            "radio-instance-enabled"

static guint radio_instance_signals[SIGNAL_COUNT] = { 0 };

static GHashTable* radio_instance_table = NULL;

#define DEFAULT_INTERFACE RADIO_INTERFACE_1_0

static const GBinderClientIfaceInfo radio_iface_info[] = {
    {RADIO_1_2, RADIO_1_2_REQ_LAST },
    {RADIO_1_1, RADIO_1_1_REQ_LAST },
    {RADIO_1_0, RADIO_1_0_REQ_LAST }
};

static const char* const radio_indication_ifaces[] = {
    RADIO_INDICATION_1_2,
    RADIO_INDICATION_1_1,
    RADIO_INDICATION_1_0,
    NULL
};

static const char* const radio_response_ifaces[] = {
    RADIO_RESPONSE_1_2,
    RADIO_RESPONSE_1_1,
    RADIO_RESPONSE_1_0,
    NULL
};

typedef struct radio_interface_desc {
    RADIO_INTERFACE version;
    const char* radio_iface;
    const char* const* ind_ifaces;
    const char* const* resp_ifaces;
} RadioInterfaceDesc;

static const RadioInterfaceDesc radio_interfaces[] = {
    {
        RADIO_INTERFACE_1_2,
        RADIO_1_2,
        radio_indication_ifaces + 0,
        radio_response_ifaces + 0,
    },{
        RADIO_INTERFACE_1_1,
        RADIO_1_1,
        radio_indication_ifaces + 1,
        radio_response_ifaces + 1
    },{
        RADIO_INTERFACE_1_0,
        RADIO_1_0,
        radio_indication_ifaces + 2,
        radio_response_ifaces + 2
    }
};

/*==========================================================================*
 * Implementation
 *==========================================================================*/

static
GQuark
radio_instance_ind_quark(
    RadioInstance* self,
    RADIO_IND ind)
{
    GQuark q = 0;

    if (ind != RADIO_IND_ANY) {
        RadioInstancePriv* priv = self->priv;
        gpointer key = GUINT_TO_POINTER(ind);

        q = GPOINTER_TO_UINT(g_hash_table_lookup(priv->ind_quarks, key));
        if (!q) {
            const char* known = radio_ind_name(ind);

            if (known) {
                q = g_quark_from_static_string(known);
            } else {
                q = g_quark_from_string(radio_instance_ind_name(self, ind));
            }
            g_hash_table_insert(priv->ind_quarks, key, GUINT_TO_POINTER(q));
        }
    }
    return q;
}

static
GQuark
radio_instance_resp_quark(
    RadioInstance* self,
    RADIO_RESP resp)
{
    GQuark q = 0;

    if (resp != RADIO_RESP_ANY) {
        RadioInstancePriv* priv = self->priv;
        gpointer key = GUINT_TO_POINTER(resp);

        q = GPOINTER_TO_UINT(g_hash_table_lookup(priv->resp_quarks, key));
        if (!q) {
            const char* known = radio_resp_name(resp);

            if (known) {
                q = g_quark_from_static_string(known);
            } else {
                q = g_quark_from_string(radio_instance_resp_name(self, resp));
            }
            g_hash_table_insert(priv->resp_quarks, key, GUINT_TO_POINTER(q));
        }
    }
    return q;
}

static
GBinderLocalReply*
radio_instance_indication(
    GBinderLocalObject* obj,
    GBinderRemoteRequest* req,
    guint code,
    guint flags,
    int* status,
    void* user_data)
{
    RadioInstance* self = RADIO_INSTANCE(user_data);
    const char* iface = gbinder_remote_request_interface(req);

    if (gutil_strv_contains((const GStrV*)radio_indication_ifaces, iface)) {
        GBinderReader reader;
        guint type;

        /* All these should be one-way */
        GASSERT(flags & GBINDER_TX_FLAG_ONEWAY);
        gbinder_remote_request_init_reader(req, &reader);
        if (gbinder_reader_read_uint32(&reader, &type) &&
            (type == RADIO_IND_UNSOLICITED || type == RADIO_IND_ACK_EXP)) {
            GQuark quark = radio_instance_ind_quark(self, code);
            gboolean handled = FALSE;

            g_signal_emit(self,
                radio_instance_signals[SIGNAL_HANDLE_INDICATION], quark,
                code, type, &reader, &handled);
            g_signal_emit(self,
                radio_instance_signals[SIGNAL_OBSERVE_INDICATION], quark,
                code, type, &reader);
            if (type == RADIO_IND_ACK_EXP && !handled) {
                GDEBUG("ack unhandled indication");
                radio_instance_ack(self);
            }
            *status = GBINDER_STATUS_OK;
        } else {
            GWARN("Failed to decode indication %u", code);
            *status = GBINDER_STATUS_FAILED;
        }
    } else {
        GWARN("Unexpected indication %s %u", iface, code);
        *status = GBINDER_STATUS_FAILED;
    }
    return NULL;
}

static
GBinderLocalReply*
radio_instance_response(
    GBinderLocalObject* obj,
    GBinderRemoteRequest* req,
    guint code,
    guint flags,
    int* status,
    void* user_data)
{
    RadioInstance* self = RADIO_INSTANCE(user_data);
    const char* iface = gbinder_remote_request_interface(req);

    if (gutil_strv_contains((const GStrV*)radio_response_ifaces, iface)) {
        /* All these should be one-way transactions */
        GASSERT(flags & GBINDER_TX_FLAG_ONEWAY);
        if (code == RADIO_RESP_ACKNOWLEDGE_REQUEST) {
            /* oneway acknowledgeRequest(int32_t serial) */
            gint32 serial;

            GDEBUG("%s %u acknowledgeRequest", iface, code);
            if (gbinder_remote_request_read_int32(req, &serial)) {
                g_signal_emit(self, radio_instance_signals[SIGNAL_ACK], 0,
                    serial);
            }
        } else {
            /* All other responses have RadioResponseInfo */
            GBinderReader reader;
            const RadioResponseInfo* info;

            gbinder_remote_request_init_reader(req, &reader);
            info = gbinder_reader_read_hidl_struct(&reader, RadioResponseInfo);
            if (info) {
                GQuark quark = radio_instance_resp_quark(self, code);
                gboolean handled = FALSE;

                g_signal_emit(self,
                    radio_instance_signals[SIGNAL_HANDLE_RESPONSE], quark,
                    code, info, &reader, &handled);
                g_signal_emit(self,
                    radio_instance_signals[SIGNAL_OBSERVE_RESPONSE], quark,
                    code, info, &reader);
                if (info->type == RADIO_RESP_SOLICITED_ACK_EXP && !handled) {
                    GDEBUG("ack unhandled response");
                    radio_instance_ack(self);
                }
            }
        }
        *status = GBINDER_STATUS_OK;
    } else {
        GWARN("Unexpected response %s %u", iface, code);
        *status = GBINDER_STATUS_FAILED;
    }
    return NULL;
}

static
void
radio_instance_drop_binder(
    RadioInstance* self)
{
    RadioInstancePriv* priv = self->priv;

    if (priv->indication) {
        gbinder_local_object_drop(priv->indication);
        priv->indication = NULL;
    }
    if (priv->response) {
        gbinder_local_object_drop(priv->response);
        priv->response = NULL;
    }
    if (priv->remote) {
        gbinder_remote_object_remove_handler(priv->remote, priv->death_id);
        gbinder_remote_object_unref(priv->remote);
        priv->death_id = 0;
        priv->remote = NULL;
    }
}

static
void
radio_instance_remove(
    const char* key)
{
    if (radio_instance_table &&
        g_hash_table_contains(radio_instance_table, key)) {
        g_hash_table_remove(radio_instance_table, key);
        radio_registry_instance_removed(key);
        if (g_hash_table_size(radio_instance_table) == 0) {
            g_hash_table_unref(radio_instance_table);
            radio_instance_table = NULL;
        }
    }
}

static
void
radio_instance_died(
    GBinderRemoteObject* obj,
    void* user_data)
{
    RadioInstance* self = RADIO_INSTANCE(user_data);

    self->dead = TRUE;
    GWARN("%s died", self->key);
    radio_instance_ref(self);
    radio_instance_drop_binder(self);
    g_signal_emit(self, radio_instance_signals[SIGNAL_DEATH], 0);
    radio_instance_remove(self->key);
    radio_instance_unref(self);
}

static
void
radio_instance_gone(
    gpointer key_ptr,
    GObject* dead)
{
    char* key = key_ptr;

    GASSERT(radio_instance_table);
    GVERBOSE_("%s", key);
    radio_instance_remove(key);
    g_free(key);
}

static
RadioInstance*
radio_instance_create_version(
    GBinderServiceManager* sm,
    GBinderRemoteObject* remote,
    const char* dev,
    const char* slot,
    const char* key,
    const char* modem,
    int slot_index,
    const RadioInterfaceDesc* desc)
{
    RadioInstance* self = g_object_new(RADIO_TYPE_INSTANCE, NULL);
    RadioInstancePriv* priv = self->priv;
    GBinderLocalRequest* req;
    GBinderWriter writer;
    int status;

    self->slot = priv->slot = g_strdup(slot);
    self->dev = priv->dev = g_strdup(dev);
    self->key = priv->key = g_strdup(key);
    self->modem = priv->modem = g_strdup(modem);
    self->slot_index = slot_index;
    self->version = desc->version;

    priv->remote = gbinder_remote_object_ref(remote);
    priv->client = gbinder_client_new2(remote,
        radio_iface_info, G_N_ELEMENTS(radio_iface_info));
    priv->indication = gbinder_servicemanager_new_local_object2(sm,
        desc->ind_ifaces, radio_instance_indication, self);
    priv->response = gbinder_servicemanager_new_local_object2(sm,
        desc->resp_ifaces, radio_instance_response, self);
    priv->death_id = gbinder_remote_object_add_death_handler(remote,
        radio_instance_died, self);

    /* IRadio::setResponseFunctions */
    req = gbinder_client_new_request2(priv->client,
        RADIO_REQ_SET_RESPONSE_FUNCTIONS);
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_local_object(&writer, priv->response);
    gbinder_writer_append_local_object(&writer, priv->indication);
    gbinder_remote_reply_unref(gbinder_client_transact_sync_reply(priv->client,
        RADIO_REQ_SET_RESPONSE_FUNCTIONS, req, &status));
    GVERBOSE_("setResponseFunctions %s status %d", slot, status);
    gbinder_local_request_unref(req);

    GDEBUG("Instance '%s'", slot);

    /*
     * Don't destroy GBinderServiceManager right away in case if we
     * have another slot to initialize.
     */
    gutil_idle_pool_add_object(priv->idle, g_object_ref(sm));
    return self;
}

static
RadioInstance*
radio_instance_create(
    const char* dev,
    const char* slot,
    const char* key,
    const char* modem,
    int slot_index,
    RADIO_INTERFACE max_version)
{
    RadioInstance* self = NULL;
    GBinderServiceManager* sm = gbinder_servicemanager_new(dev);

    if (sm) {
        guint i;

        for (i = 0; i < G_N_ELEMENTS(radio_interfaces) && !self; i++) {
            const RadioInterfaceDesc* desc = radio_interfaces + i;

            if (desc->version <= max_version) {
                char* fqname = g_strconcat(desc->radio_iface, "/", slot, NULL);
                GBinderRemoteObject* obj = /* autoreleased */
                    gbinder_servicemanager_get_service_sync(sm, fqname, NULL);

                if (obj) {
                    GINFO("Connected to %s", fqname);
                    self = radio_instance_create_version(sm, obj, dev, slot, key,
                        modem, slot_index, desc);
                }
                g_free(fqname);
            }
        }
        gbinder_servicemanager_unref(sm);
    }
    return self;
}

static
char*
radio_instance_make_key(
    const char* dev,
    const char* name,
    RADIO_INTERFACE version)
{
    return g_strdup_printf("%s:%s:%d", dev, name, version);
}

/*==========================================================================*
 * API
 *==========================================================================*/

RadioInstance*
radio_instance_new(
    const char* dev,
    const char* name)
{
    return radio_instance_new_with_version(dev, name, DEFAULT_INTERFACE);
}

RadioInstance*
radio_instance_new_with_modem_and_slot(
    const char* dev,
    const char* name,
    const char* modem,
    int slot) /* Since 1.0.7 */
{
    return radio_instance_new_with_modem_slot_and_version(dev, name, modem,
        slot, DEFAULT_INTERFACE);
}

RadioInstance*
radio_instance_new_with_version(
    const char* dev,
    const char* name,
    RADIO_INTERFACE version) /* Since 1.2.1 */
{
    if (name && name[0]) {
        const char* modem;
        int slot;

        if (!g_strcmp0(name, "slot1")) {
            modem = "/ril_0";
            slot = 0;
        } else if (!g_strcmp0(name, "slot2")) {
            modem = "/ril_1";
            slot = 1;
        } else {
            GWARN("Unexpected slot '%s'", name);
            modem = NULL;
            slot = 0;
        }
        return radio_instance_new_with_modem_slot_and_version(dev, name, modem,
            slot, version);
    }
    return NULL;
}

RadioInstance*
radio_instance_new_with_modem_slot_and_version(
    const char* dev,
    const char* name,
    const char* modem,
    int slot,
    RADIO_INTERFACE version) /* Since 1.2.1 */
{
    if (dev && dev[0] && name && name[0]) {
        char* key = radio_instance_make_key(dev, name, version);
        RadioInstance* self = NULL;

        if (radio_instance_table) {
            self = g_hash_table_lookup(radio_instance_table, key);
        }
        if (self) {
            g_free(key);
            return radio_instance_ref(self);
        } else {
            self = radio_instance_create(dev, name, key, modem, slot, version);
            if (self) {
                if (!radio_instance_table) {
                    radio_instance_table = g_hash_table_new_full
                        (g_str_hash, g_str_equal, g_free, NULL);
                }
                g_hash_table_replace(radio_instance_table, g_strdup(key), self);
                g_object_weak_ref(G_OBJECT(self), radio_instance_gone, key);
                radio_registry_instance_added(self);
                return self;
            }
        }
        g_free(key);
    }
    return NULL;
}

RadioInstance*
radio_instance_get(
    const char* dev,
    const char* name)
{
    return radio_instance_get_with_version(dev, name, DEFAULT_INTERFACE);
}

RadioInstance*
radio_instance_get_with_interface(
    const char* dev,
    const char* name,
    RADIO_INTERFACE version) /* 1.2.1, deprecated */
{
    return radio_instance_get_with_version(dev, name, version);
}

RadioInstance*
radio_instance_get_with_version(
    const char* dev,
    const char* name,
    RADIO_INTERFACE version) /* Since 1.2.2 */
{
    RadioInstance* self = NULL;

    if (dev && dev[0] && name && name[0]) {
        char* key = radio_instance_make_key(dev, name, version);

        if (radio_instance_table) {
            self = g_hash_table_lookup(radio_instance_table, key);
        }
        g_free(key);
    }
    return self;
}

RadioInstance* const*
radio_instance_get_all(
    void)
{
    if (radio_instance_table) {
        /* If the table exists, it must be non-empty */
        const guint n = g_hash_table_size(radio_instance_table);
        RadioInstance** all = g_new0(RadioInstance*, n + 1);
        RadioInstance* last = NULL;
        GHashTableIter it;
        gpointer value;
        guint i = 0;

        g_hash_table_iter_init(&it, radio_instance_table);
        while (g_hash_table_iter_next(&it, NULL, &value)) {
            last = all[i++] = value;
        }

        /* Add the array itself to the idle pool of the last instance */
        gutil_idle_pool_add(last->priv->idle, all, g_free);
        return all;
    }
    return NULL;
}

RadioInstance*
radio_instance_ref(
    RadioInstance* self)
{
    if (G_LIKELY(self)) {
        g_object_ref(RADIO_INSTANCE(self));
        return self;
    } else {
        return NULL;
    }
}

void
radio_instance_unref(
    RadioInstance* self)
{
    if (G_LIKELY(self)) {
        g_object_unref(RADIO_INSTANCE(self));
    }
}

const char*
radio_instance_req_name(
    RadioInstance* self,
    RADIO_REQ req)
{
    const char* known = radio_req_name(req);

    if (known) {
        return known;
    } else if (G_LIKELY(self)) {
        char* str = g_strdup_printf("%u", (guint)req);

        gutil_idle_pool_add(self->priv->idle, str, g_free);
        return str;
    } else {
        return NULL;
    }
}

const char*
radio_instance_resp_name(
    RadioInstance* self,
    RADIO_RESP resp)
{
    const char* known = radio_resp_name(resp);

    if (known) {
        return known;
    } else if (G_LIKELY(self)) {
        char* str = g_strdup_printf("%u", (guint)resp);

        gutil_idle_pool_add(self->priv->idle, str, g_free);
        return str;
    } else {
        return NULL;
    }
}

const char*
radio_instance_ind_name(
    RadioInstance* self,
    RADIO_IND ind)
{
    const char* known = radio_ind_name(ind);

    if (known) {
        return known;
    } else if (G_LIKELY(self)) {
        char* str = g_strdup_printf("%u", (guint)ind);

        gutil_idle_pool_add(self->priv->idle, str, g_free);
        return str;
    } else {
        return NULL;
    }
}


gboolean
radio_instance_is_dead(
    RadioInstance* self)
{
    return G_UNLIKELY(!self) || self->dead;
}

gboolean
radio_instance_ack(
    RadioInstance* self)
{
    if (G_LIKELY(self)) {
        return gbinder_client_transact_sync_oneway(self->priv->client,
            RADIO_REQ_RESPONSE_ACKNOWLEDGEMENT, NULL) >= 0;
    }
    return 0;
}

GBinderLocalRequest*
radio_instance_new_request(
    RadioInstance* self,
    RADIO_REQ code)
{
    if (G_LIKELY(self)) {
        return gbinder_client_new_request2(self->priv->client, code);
    }
    return NULL;
}

gboolean
radio_instance_send_request_sync(
    RadioInstance* self,
    RADIO_REQ code,
    GBinderLocalRequest* args)
{
    if (G_LIKELY(self)) {
        return gbinder_client_transact_sync_oneway(self->priv->client,
            code, args) >= 0;
    }
    return FALSE;
}

void
radio_instance_set_enabled(
    RadioInstance* self,
    gboolean enabled) /* Since 1.0.7 */
{
    if (G_LIKELY(self) && self->enabled != enabled) {
        self->enabled = enabled;
        GDEBUG("%s %sabled", self->slot, enabled ? "en" : "dis");
        g_signal_emit(self, radio_instance_signals[SIGNAL_ENABLED], 0);
    }
}

gulong
radio_instance_add_indication_handler(
    RadioInstance* self,
    RADIO_IND ind,
    RadioIndicationHandlerFunc func,
    gpointer user_data)
{
    return (G_LIKELY(self) && G_LIKELY(func)) ?
        g_signal_connect_closure_by_id(self,
            radio_instance_signals[SIGNAL_HANDLE_INDICATION],
            radio_instance_ind_quark(self, ind),
            g_cclosure_new(G_CALLBACK(func), user_data, NULL), FALSE) : 0;
}

gulong
radio_instance_add_indication_observer(
    RadioInstance* self,
    RADIO_IND ind,
    RadioIndicationObserverFunc func,
    gpointer user_data)
{
    return (G_LIKELY(self) && G_LIKELY(func)) ?
        g_signal_connect_closure_by_id(self,
            radio_instance_signals[SIGNAL_OBSERVE_INDICATION],
            radio_instance_ind_quark(self, ind),
            g_cclosure_new(G_CALLBACK(func), user_data, NULL), FALSE) : 0;
}
gulong
radio_instance_add_response_handler(
    RadioInstance* self,
    RADIO_RESP resp,
    RadioResponseHandlerFunc func,
    gpointer user_data)
{
    return (G_LIKELY(self) && G_LIKELY(func)) ?
        g_signal_connect_closure_by_id(self,
            radio_instance_signals[SIGNAL_HANDLE_RESPONSE],
            radio_instance_resp_quark(self, resp),
            g_cclosure_new(G_CALLBACK(func), user_data, NULL), FALSE) : 0;
}

gulong
radio_instance_add_response_observer(
    RadioInstance* self,
    RADIO_RESP resp,
    RadioResponseObserverFunc func,
    gpointer user_data)
{
    return (G_LIKELY(self) && G_LIKELY(func)) ?
        g_signal_connect_closure_by_id(self,
            radio_instance_signals[SIGNAL_OBSERVE_RESPONSE],
            radio_instance_resp_quark(self, resp),
            g_cclosure_new(G_CALLBACK(func), user_data, NULL), FALSE) : 0;
}

gulong
radio_instance_add_ack_handler(
    RadioInstance* self,
    RadioAckFunc func,
    gpointer user_data)
{
    return (G_LIKELY(self) && G_LIKELY(func)) ? g_signal_connect(self,
        SIGNAL_ACK_NAME, G_CALLBACK(func), user_data) : 0;
}

gulong
radio_instance_add_death_handler(
    RadioInstance* self,
    RadioInstanceFunc func,
    gpointer user_data)
{
    return (G_LIKELY(self) && G_LIKELY(func)) ? g_signal_connect(self,
        SIGNAL_DEATH_NAME, G_CALLBACK(func), user_data) : 0;
}

gulong
radio_instance_add_enabled_handler(
    RadioInstance* self,
    RadioInstanceFunc func,
    gpointer user_data) /* Since 1.0.7 */
{
    return (G_LIKELY(self) && G_LIKELY(func)) ? g_signal_connect(self,
        SIGNAL_ENABLED_NAME, G_CALLBACK(func), user_data) : 0;
}

void
radio_instance_remove_handler(
    RadioInstance* self,
    gulong id)
{
    if (G_LIKELY(self) && G_LIKELY(id)) {
        g_signal_handler_disconnect(self, id);
    }
}

void
radio_instance_remove_handlers(
    RadioInstance* self,
    gulong* ids,
    int count)
{
    gutil_disconnect_handlers(self, ids, count);
}

/*==========================================================================*
 * Internals
 *==========================================================================*/

static
void
radio_instance_init(
    RadioInstance* self)
{
    RadioInstancePriv* priv = G_TYPE_INSTANCE_GET_PRIVATE
        (self, RADIO_TYPE_INSTANCE, RadioInstancePriv);

    self->priv = priv;
    priv->idle = gutil_idle_pool_new();
    priv->resp_quarks = g_hash_table_new(g_direct_hash, g_direct_equal);
    priv->ind_quarks = g_hash_table_new(g_direct_hash, g_direct_equal);
}

static
void
radio_instance_finalize(
    GObject* object)
{
    RadioInstance* self = RADIO_INSTANCE(object);
    RadioInstancePriv* priv = self->priv;

    radio_instance_drop_binder(self);
    gbinder_client_unref(priv->client);
    gutil_idle_pool_destroy(priv->idle);
    g_hash_table_destroy(priv->resp_quarks);
    g_hash_table_destroy(priv->ind_quarks);
    g_free(priv->slot);
    g_free(priv->dev);
    g_free(priv->key);
    g_free(priv->modem);
    G_OBJECT_CLASS(radio_instance_parent_class)->finalize(object);
}

static
void
radio_instance_class_init(
    RadioInstanceClass* klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    GType type = G_OBJECT_CLASS_TYPE(klass);

    g_type_class_add_private(klass, sizeof(RadioInstancePriv));
    object_class->finalize = radio_instance_finalize;

    radio_instance_signals[SIGNAL_HANDLE_INDICATION] =
        g_signal_new(SIGNAL_HANDLE_INDICATION_NAME, type,
            G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED, 0,
            g_signal_accumulator_true_handled, NULL, NULL,
            G_TYPE_BOOLEAN, 3, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_POINTER);
    radio_instance_signals[SIGNAL_HANDLE_RESPONSE] =
        g_signal_new(SIGNAL_HANDLE_RESPONSE_NAME, type,
            G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED, 0,
            g_signal_accumulator_true_handled, NULL, NULL,
            G_TYPE_BOOLEAN, 3, G_TYPE_UINT, G_TYPE_POINTER, G_TYPE_POINTER);
    radio_instance_signals[SIGNAL_OBSERVE_INDICATION] =
        g_signal_new(SIGNAL_OBSERVE_INDICATION_NAME, type,
            G_SIGNAL_RUN_FIRST | G_SIGNAL_DETAILED, 0, NULL, NULL, NULL,
            G_TYPE_NONE, 3, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_POINTER);
    radio_instance_signals[SIGNAL_OBSERVE_RESPONSE] =
        g_signal_new(SIGNAL_OBSERVE_RESPONSE_NAME, type,
            G_SIGNAL_RUN_FIRST | G_SIGNAL_DETAILED, 0, NULL, NULL, NULL,
            G_TYPE_NONE, 3, G_TYPE_UINT, G_TYPE_POINTER, G_TYPE_POINTER);
    radio_instance_signals[SIGNAL_ACK] =
        g_signal_new(SIGNAL_ACK_NAME, type,
            G_SIGNAL_RUN_FIRST, 0, NULL, NULL, NULL,
            G_TYPE_NONE, 1, G_TYPE_UINT);
    radio_instance_signals[SIGNAL_DEATH] =
        g_signal_new(SIGNAL_DEATH_NAME, type,
            G_SIGNAL_RUN_FIRST, 0, NULL, NULL, NULL,
            G_TYPE_NONE, 0);
    radio_instance_signals[SIGNAL_ENABLED] =
        g_signal_new(SIGNAL_ENABLED_NAME, type,
            G_SIGNAL_RUN_FIRST, 0, NULL, NULL, NULL,
            G_TYPE_NONE, 0);
}

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */

