/*
 * Copyright (C) 2018-2022 Jolla Ltd.
 * Copyright (C) 2018-2022 Slava Monich <slava.monich@jolla.com>
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

#include "radio_instance_p.h"
#include "radio_registry_p.h"
#include "radio_util_p.h"
#include "radio_log.h"

#include <gbinder.h>

#include <gutil_idlepool.h>
#include <gutil_macros.h>
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
    GHashTable* req_quarks;
    GHashTable* resp_quarks;
    GHashTable* ind_quarks;
    gulong death_id;
    char* dev;
    char* slot;
    char* key;
    char* modem;
};

G_DEFINE_TYPE(RadioInstance, radio_instance, G_TYPE_OBJECT)

typedef enum radio_instance_signal {
    #define SIGNAL_INDEX(x) SIGNAL_OBSERVE_REQUEST_##x,
    FOREACH_OBSERVER_PRIORITY(SIGNAL_INDEX)
    #undef SIGNAL_INDEX
    #define SIGNAL_INDEX(x) SIGNAL_OBSERVE_RESPONSE_##x,
    FOREACH_OBSERVER_PRIORITY(SIGNAL_INDEX)
    #undef SIGNAL_INDEX
    #define SIGNAL_INDEX(x) SIGNAL_OBSERVE_INDICATION_##x,
    FOREACH_OBSERVER_PRIORITY(SIGNAL_INDEX)
    #undef SIGNAL_INDEX
    SIGNAL_HANDLE_RESPONSE,
    SIGNAL_HANDLE_INDICATION,
    SIGNAL_ACK,
    SIGNAL_DEATH,
    SIGNAL_ENABLED,
    SIGNAL_CONNECTED,
    SIGNAL_COUNT
} RADIO_INSTANCE_SIGNAL;

static const char* radio_instance_signal_observe_request_name[] = {
    #define SIGNAL_NAME(x) "radio-instance-observe-request-" #x,
    FOREACH_OBSERVER_PRIORITY(SIGNAL_NAME)
    #undef SIGNAL_NAME
    NULL
};

static const char* radio_instance_signal_observe_response_name[] = {
    #define SIGNAL_NAME(x) "radio-instance-observe-response-" #x,
    FOREACH_OBSERVER_PRIORITY(SIGNAL_NAME)
    #undef SIGNAL_NAME
    NULL
};

static const char* radio_instance_signal_observe_indication_name[] = {
    #define SIGNAL_NAME(x) "radio-instance-observe-indication-" #x,
    FOREACH_OBSERVER_PRIORITY(SIGNAL_NAME)
    #undef SIGNAL_NAME
    NULL
};

#define SIGNAL_HANDLE_INDICATION_NAME  "radio-instance-handle-indication"
#define SIGNAL_HANDLE_RESPONSE_NAME    "radio-instance-handle-response"
#define SIGNAL_ACK_NAME                "radio-instance-ack"
#define SIGNAL_DEATH_NAME              "radio-instance-death"
#define SIGNAL_ENABLED_NAME            "radio-instance-enabled"
#define SIGNAL_CONNECTED_NAME          "radio-instance-connected"

static guint radio_instance_signals[SIGNAL_COUNT] = { 0 };

static GHashTable* radio_instance_table = NULL;

#define DEFAULT_INTERFACE RADIO_INTERFACE_1_0

static const GBinderClientIfaceInfo radio_iface_info[] = {
    {RADIO_1_5, RADIO_1_5_REQ_LAST },
    {RADIO_1_4, RADIO_1_4_REQ_LAST },
    {RADIO_1_3, RADIO_1_3_REQ_LAST },
    {RADIO_1_2, RADIO_1_2_REQ_LAST },
    {RADIO_1_1, RADIO_1_1_REQ_LAST },
    {RADIO_1_0, RADIO_1_0_REQ_LAST }
};
G_STATIC_ASSERT(G_N_ELEMENTS(radio_iface_info) == RADIO_INTERFACE_COUNT);

static const char* const radio_indication_ifaces[] = {
    RADIO_INDICATION_1_5,
    RADIO_INDICATION_1_4,
    RADIO_INDICATION_1_3,
    RADIO_INDICATION_1_2,
    RADIO_INDICATION_1_1,
    RADIO_INDICATION_1_0,
    NULL
};
G_STATIC_ASSERT(G_N_ELEMENTS(radio_indication_ifaces) == RADIO_INTERFACE_COUNT + 1);

static const char* const radio_response_ifaces[] = {
    RADIO_RESPONSE_1_5,
    RADIO_RESPONSE_1_4,
    RADIO_RESPONSE_1_3,
    RADIO_RESPONSE_1_2,
    RADIO_RESPONSE_1_1,
    RADIO_RESPONSE_1_0,
    NULL
};
G_STATIC_ASSERT(G_N_ELEMENTS(radio_response_ifaces) == RADIO_INTERFACE_COUNT + 1);

typedef struct radio_interface_desc {
    RADIO_INTERFACE version;
    RADIO_AIDL_INTERFACE aidl_interface;
    const char* radio_iface;
    const char* const* ind_ifaces;
    const char* const* resp_ifaces;
    gint32 set_response_functions_req;
} RadioInterfaceDesc;

#define RADIO_INTERFACE_INDEX(x) (RADIO_INTERFACE_COUNT - x - 1)

#define RADIO_INTERFACE_DESC(v) \
        RADIO_INTERFACE_##v, \
        RADIO_AIDL_INTERFACE_NONE, \
        RADIO_##v, \
        radio_indication_ifaces + RADIO_INTERFACE_INDEX(RADIO_INTERFACE_##v), \
        radio_response_ifaces + RADIO_INTERFACE_INDEX(RADIO_INTERFACE_##v), \
        RADIO_REQ_SET_RESPONSE_FUNCTIONS

static const RadioInterfaceDesc radio_interfaces[] = {
   { RADIO_INTERFACE_DESC(1_5) },
   { RADIO_INTERFACE_DESC(1_4) },
   { RADIO_INTERFACE_DESC(1_3) },
   { RADIO_INTERFACE_DESC(1_2) },
   { RADIO_INTERFACE_DESC(1_1) },
   { RADIO_INTERFACE_DESC(1_0) }
};
G_STATIC_ASSERT(G_N_ELEMENTS(radio_interfaces) == RADIO_INTERFACE_COUNT);

static const GBinderClientIfaceInfo radio_aidl_iface_info[] = {
    {RADIO_DATA, RADIO_DATA_1_REQ_LAST},
    {RADIO_MESSAGING, RADIO_MESSAGING_1_REQ_LAST},
    {RADIO_MODEM, RADIO_MODEM_1_REQ_LAST},
    {RADIO_NETWORK, RADIO_NETWORK_1_REQ_LAST},
    {RADIO_SIM, RADIO_SIM_1_REQ_LAST},
    {RADIO_VOICE, RADIO_VOICE_1_REQ_LAST},
};

static const char* const radio_data_indication_ifaces[] = {
    RADIO_DATA_INDICATION,
    NULL
};

static const char* const radio_data_response_ifaces[] = {
    RADIO_DATA_RESPONSE,
    NULL
};

static const char* const radio_messaging_indication_ifaces[] = {
    RADIO_MESSAGING_INDICATION,
    NULL
};

static const char* const radio_messaging_response_ifaces[] = {
    RADIO_MESSAGING_RESPONSE,
    NULL
};

static const char* const radio_modem_indication_ifaces[] = {
    RADIO_MODEM_INDICATION,
    NULL
};

static const char* const radio_modem_response_ifaces[] = {
    RADIO_MODEM_RESPONSE,
    NULL
};

static const char* const radio_network_indication_ifaces[] = {
    RADIO_NETWORK_INDICATION,
    NULL
};

static const char* const radio_network_response_ifaces[] = {
    RADIO_NETWORK_RESPONSE,
    NULL
};

static const char* const radio_sim_indication_ifaces[] = {
    RADIO_SIM_INDICATION,
    NULL
};

static const char* const radio_sim_response_ifaces[] = {
    RADIO_SIM_RESPONSE,
    NULL
};

static const char* const radio_voice_indication_ifaces[] = {
    RADIO_VOICE_INDICATION,
    NULL
};

static const char* const radio_voice_response_ifaces[] = {
    RADIO_VOICE_RESPONSE,
    NULL
};

static const RadioInterfaceDesc radio_aidl_interfaces[] = {
    {
        RADIO_INTERFACE_NONE,
        RADIO_DATA_INTERFACE,
        RADIO_DATA,
        radio_data_indication_ifaces,
        radio_data_response_ifaces,
        RADIO_DATA_REQ_SET_RESPONSE_FUNCTIONS,
    },
    {
        RADIO_INTERFACE_NONE,
        RADIO_MESSAGING_INTERFACE,
        RADIO_MESSAGING,
        radio_messaging_indication_ifaces,
        radio_messaging_response_ifaces,
        RADIO_MESSAGING_REQ_SET_RESPONSE_FUNCTIONS,
    },
    {
        RADIO_INTERFACE_NONE,
        RADIO_MODEM_INTERFACE,
        RADIO_MODEM,
        radio_modem_indication_ifaces,
        radio_modem_response_ifaces,
        RADIO_MODEM_REQ_SET_RESPONSE_FUNCTIONS,
    },
    {
        RADIO_INTERFACE_NONE,
        RADIO_NETWORK_INTERFACE,
        RADIO_NETWORK,
        radio_network_indication_ifaces,
        radio_network_response_ifaces,
        RADIO_NETWORK_REQ_SET_RESPONSE_FUNCTIONS,
    },
    {
        RADIO_INTERFACE_NONE,
        RADIO_SIM_INTERFACE,
        RADIO_SIM,
        radio_sim_indication_ifaces,
        radio_sim_response_ifaces,
        RADIO_SIM_REQ_SET_RESPONSE_FUNCTIONS,
    },
    {
        RADIO_INTERFACE_NONE,
        RADIO_VOICE_INTERFACE,
        RADIO_VOICE,
        radio_voice_indication_ifaces,
        radio_voice_response_ifaces,
        RADIO_VOICE_REQ_SET_RESPONSE_FUNCTIONS,
    }
};
G_STATIC_ASSERT(G_N_ELEMENTS(radio_aidl_interfaces) == RADIO_AIDL_INTERFACE_COUNT);

typedef struct radio_instance_tx {
    RadioInstance* instance;
    RadioInstanceTxCompleteFunc complete;
    RadioInstanceTxDestroyFunc destroy;
    gulong id;
    void* user_data1;
    void* user_data2;
} RadioInstanceTx;

/*==========================================================================*
 * Implementation
 *==========================================================================*/

static
GQuark
radio_instance_req_quark(
    RadioInstance* self,
    RADIO_REQ req)
{
    GQuark q = 0;

    if (req != RADIO_REQ_ANY) {
        RadioInstancePriv* priv = self->priv;
        gpointer key = GUINT_TO_POINTER(req);

        q = GPOINTER_TO_UINT(g_hash_table_lookup(priv->req_quarks, key));
        if (!q) {
            const char* known = radio_req_name2(self, req);

            if (known) {
                q = g_quark_from_static_string(known);
            } else {
                q = g_quark_from_string(radio_instance_req_name(self, req));
            }
            g_hash_table_insert(priv->req_quarks, key, GUINT_TO_POINTER(q));
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
            const char* known = radio_resp_name2(self, resp);

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
void
radio_instance_notify_request_observers(
    RadioInstance* self,
    RADIO_REQ code,
    GBinderLocalRequest* args)
{
    GQuark quark = 0;
    int i;

    for (i = RADIO_OBSERVER_PRIORITY_COUNT - 1; i >= 0; i--) {
        guint id = radio_instance_signals[SIGNAL_OBSERVE_REQUEST_0 + i];

        if (id) {
            if (!quark) {
                quark = radio_instance_req_quark(self, code);
            }
            g_signal_emit(self, id, quark, code, args);
        }
    }
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

    if (gutil_strv_contains((const GStrV*)radio_indication_ifaces, iface)
        || gutil_strv_contains((const GStrV*)radio_data_indication_ifaces, iface)
        || gutil_strv_contains((const GStrV*)radio_messaging_indication_ifaces, iface)
        || gutil_strv_contains((const GStrV*)radio_modem_indication_ifaces, iface)
        || gutil_strv_contains((const GStrV*)radio_network_indication_ifaces, iface)
        || gutil_strv_contains((const GStrV*)radio_sim_indication_ifaces, iface)
        || gutil_strv_contains((const GStrV*)radio_voice_indication_ifaces, iface)) {
        GBinderReader reader;
        guint type;

        /* All these should be one-way */
        GASSERT(flags & GBINDER_TX_FLAG_ONEWAY);
        gbinder_remote_request_init_reader(req, &reader);
        if (gbinder_reader_read_uint32(&reader, &type) &&
            (type == RADIO_IND_UNSOLICITED || type == RADIO_IND_ACK_EXP)) {
            const GQuark quark = radio_instance_ind_quark(self, code);
            const guint* signals = radio_instance_signals +
                SIGNAL_OBSERVE_INDICATION_0;
            int p = RADIO_OBSERVER_PRIORITY_HIGHEST;
            gboolean handled = FALSE;
            guint ind_ril_connected = 0;

            /* High-priority observers are notified first */
            for (; p > RADIO_OBSERVER_PRIORITY_DEFAULT; p--) {
                if (signals[RADIO_OBSERVER_PRIORITY_INDEX(p)]) {
                    g_signal_emit(self, signals
                        [RADIO_OBSERVER_PRIORITY_INDEX(p)],
                        quark, code, type, &reader);
                }
            }

            /* rilConnected is a special case */
            if (self->interface_type == RADIO_INTERFACE_TYPE_HIDL) {
                ind_ril_connected = RADIO_IND_RIL_CONNECTED;
            } else if (self->interface_type == RADIO_INTERFACE_TYPE_AIDL
                && gutil_strv_contains((const GStrV*)radio_modem_indication_ifaces, iface)) {
                ind_ril_connected = RADIO_MODEM_IND_RIL_CONNECTED;
            }

            if (ind_ril_connected && code == ind_ril_connected) {
                if (G_UNLIKELY(self->connected)) {
                    /* We are only supposed to receive it once */
                    GWARN("%s received unexpected rilConnected", self->slot);
                } else {
                    GDEBUG("%s connected", self->slot);
                    self->connected = TRUE;
                    g_signal_emit(self, radio_instance_signals
                        [SIGNAL_CONNECTED], 0);
                }
            }

            /* Notify handlers */
            g_signal_emit(self, radio_instance_signals
                [SIGNAL_HANDLE_INDICATION],
                quark, code, type, &reader, &handled);

            /* And then remaining observers in their priority order */
            for (; p >= RADIO_OBSERVER_PRIORITY_LOWEST; p--) {
                if (signals[RADIO_OBSERVER_PRIORITY_INDEX(p)]) {
                    g_signal_emit(self, signals
                        [RADIO_OBSERVER_PRIORITY_INDEX(p)],
                        quark, code, type, &reader);
                }
            }

            /* Ack unhandled indications */
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
    const RadioResponseInfo* info = NULL;
    GBinderReader reader;
    gint32 ack_serial = 0;

    gbinder_remote_request_init_reader(req, &reader);

    if (gutil_strv_contains((const GStrV*)radio_response_ifaces, iface)) {
        if (code == RADIO_RESP_ACKNOWLEDGE_REQUEST) {
            gbinder_reader_read_int32(&reader, &ack_serial);
        } else {
            /* All other responses have RadioResponseInfo */
            info = gbinder_reader_read_hidl_struct(&reader, RadioResponseInfo);
        }
    } else if (gutil_strv_contains((const GStrV*)radio_data_response_ifaces, iface)) {
        if (code == RADIO_DATA_RESP_ACKNOWLEDGE_REQUEST) {
            gbinder_reader_read_int32(&reader, &ack_serial);
        } else {
            gsize out_size;
            info = gbinder_reader_read_parcelable(&reader, &out_size);
            GASSERT(out_size >= sizeof(RadioResponseInfo));
        }
    } else if (gutil_strv_contains((const GStrV*)radio_messaging_response_ifaces, iface)) {
        if (code == RADIO_MESSAGING_RESP_ACKNOWLEDGE_REQUEST) {
            gbinder_reader_read_int32(&reader, &ack_serial);
        } else {
            /* RadioResponseInfo has the same fields/padding between HIDL and AIDL */
            gsize out_size;
            info = gbinder_reader_read_parcelable(&reader, &out_size);
            GASSERT(out_size >= sizeof(RadioResponseInfo));
        }
    } else if (gutil_strv_contains((const GStrV*)radio_modem_response_ifaces, iface)) {
        if (code == RADIO_MODEM_RESP_ACKNOWLEDGE_REQUEST) {
            gbinder_reader_read_int32(&reader, &ack_serial);
        } else {
            /* RadioResponseInfo has the same fields/padding between HIDL and AIDL */
            gsize out_size;
            info = gbinder_reader_read_parcelable(&reader, &out_size);
            GASSERT(out_size >= sizeof(RadioResponseInfo));
        }
    } else if (gutil_strv_contains((const GStrV*)radio_network_response_ifaces, iface)) {
        if (code == RADIO_NETWORK_RESP_ACKNOWLEDGE_REQUEST) {
            gbinder_reader_read_int32(&reader, &ack_serial);
        } else {
            gsize out_size;
            info = gbinder_reader_read_parcelable(&reader, &out_size);
            GASSERT(out_size >= sizeof(RadioResponseInfo));
        }
    } else if (gutil_strv_contains((const GStrV*)radio_sim_response_ifaces, iface)) {
        if (code == RADIO_SIM_RESP_ACKNOWLEDGE_REQUEST) {
            gbinder_reader_read_int32(&reader, &ack_serial);
        } else {
            gsize out_size;
            info = gbinder_reader_read_parcelable(&reader, &out_size);
            GASSERT(out_size >= sizeof(RadioResponseInfo));
        }
    } else if (gutil_strv_contains((const GStrV*)radio_voice_response_ifaces, iface)) {
        if (code == RADIO_VOICE_RESP_ACKNOWLEDGE_REQUEST) {
            gbinder_reader_read_int32(&reader, &ack_serial);
        } else {
            gsize out_size;
            info = gbinder_reader_read_parcelable(&reader, &out_size);
            GASSERT(out_size >= sizeof(RadioResponseInfo));
        }
    } else {
        GWARN("Unexpected response %s %u", iface, code);
        *status = GBINDER_STATUS_FAILED;
        return NULL;
    }

    /* All these should be one-way transactions */
    GASSERT(flags & GBINDER_TX_FLAG_ONEWAY);

    if (ack_serial) {
        GDEBUG("%s %u acknowledgeRequest", iface, code);
        g_signal_emit(self, radio_instance_signals[SIGNAL_ACK], 0,
            ack_serial);
    } else if (info) {
        const GQuark quark = radio_instance_resp_quark(self, code);
        const guint* signals = radio_instance_signals +
            SIGNAL_OBSERVE_RESPONSE_0;
        int p = RADIO_OBSERVER_PRIORITY_HIGHEST;
        gboolean handled = FALSE;

        /* High-priority observers are notified first */
        for (; p > RADIO_OBSERVER_PRIORITY_DEFAULT; p--) {
            if (signals[RADIO_OBSERVER_PRIORITY_INDEX(p)]) {
                g_signal_emit(self, signals
                    [RADIO_OBSERVER_PRIORITY_INDEX(p)],
                    quark, code, info, &reader);
            }
        }

        /* Then handlers */
        g_signal_emit(self, radio_instance_signals
            [SIGNAL_HANDLE_RESPONSE],
            quark, code, info, &reader, &handled);

        /* And then remaining observers in their priority order */
        for (; p >= RADIO_OBSERVER_PRIORITY_LOWEST; p--) {
            if (signals[RADIO_OBSERVER_PRIORITY_INDEX(p)]) {
                g_signal_emit(self, signals
                    [RADIO_OBSERVER_PRIORITY_INDEX(p)],
                    quark, code, info, &reader);
            }
        }

        /* Ack unhandled responses */
        if (info->type == RADIO_RESP_SOLICITED_ACK_EXP && !handled) {
            GDEBUG("ack unhandled response");
            radio_instance_ack(self);
        }
    }
    *status = GBINDER_STATUS_OK;
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
    self->connected = FALSE;
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
    self->interface_aidl = desc->aidl_interface;

    priv->remote = gbinder_remote_object_ref(remote);
    priv->indication = gbinder_servicemanager_new_local_object2(sm,
        desc->ind_ifaces, radio_instance_indication, self);
    priv->response = gbinder_servicemanager_new_local_object2(sm,
        desc->resp_ifaces, radio_instance_response, self);
    priv->death_id = gbinder_remote_object_add_death_handler(remote,
        radio_instance_died, self);

    if (desc->version != RADIO_INTERFACE_NONE) {
        self->interface_type = RADIO_INTERFACE_TYPE_HIDL;
        priv->client = gbinder_client_new2(remote,
            radio_iface_info, G_N_ELEMENTS(radio_iface_info));
    } else if (desc->aidl_interface != RADIO_AIDL_INTERFACE_NONE) {
        self->interface_type = RADIO_INTERFACE_TYPE_AIDL;
        priv->client = gbinder_client_new2(remote,
            radio_aidl_iface_info + desc->aidl_interface, 1);

        gbinder_local_object_set_stability(priv->indication, GBINDER_STABILITY_VINTF);
        gbinder_local_object_set_stability(priv->response, GBINDER_STABILITY_VINTF);
    }

    /* IRadio::setResponseFunctions */
    req = gbinder_client_new_request2(priv->client,
        desc->set_response_functions_req);
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_local_object(&writer, priv->response);
    gbinder_writer_append_local_object(&writer, priv->indication);
    gbinder_remote_reply_unref(gbinder_client_transact_sync_reply(priv->client,
        desc->set_response_functions_req, req, &status));
    GVERBOSE_("setResponseFunctions %s status %d", slot, status);
    gbinder_local_request_unref(req);

    GDEBUG("Instance '%s'", slot);

    /*
     * Don't destroy GBinderServiceManager right away in case if we
     * have another slot to initialize.
     */
    gutil_idle_pool_add(priv->idle, gbinder_servicemanager_ref(sm),
        (GDestroyNotify) gbinder_servicemanager_unref);
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
    RADIO_INTERFACE max_version,
    RADIO_AIDL_INTERFACE aidl_interface)
{
    RadioInstance* self = NULL;
    GBinderServiceManager* sm = gbinder_servicemanager_new(dev);
    const RadioInterfaceDesc* interfaces = NULL;
    gsize num_interfaces = 0;

    if (!sm) {
        GERR_("Failed to get ServiceManager on %s", dev);
        return NULL;
    }

    if (aidl_interface == RADIO_AIDL_INTERFACE_NONE) {
        interfaces = radio_interfaces;
        num_interfaces = G_N_ELEMENTS(radio_interfaces);
    } else if (aidl_interface > RADIO_AIDL_INTERFACE_NONE
                && aidl_interface < RADIO_AIDL_INTERFACE_COUNT) {
        interfaces = radio_aidl_interfaces + aidl_interface;
        num_interfaces = 1;
    }

    for (guint i = 0; i < num_interfaces && !self; i++) {
        const RadioInterfaceDesc* desc = interfaces + i;

        if (desc->version <= max_version) {
            char* fqname = g_strconcat(desc->radio_iface, "/", slot, NULL);
            GBinderRemoteObject* obj = /* autoreleased */
                gbinder_servicemanager_get_service_sync(sm, fqname, NULL);

            if (obj) {
                GINFO("Connected to %s", fqname);
                self = radio_instance_create_version(sm, obj, dev, slot,
                    key, modem, slot_index, desc);
            }
            g_free(fqname);
        }
    }

    gbinder_servicemanager_unref(sm);

    return self;
}

static
char*
radio_instance_make_key(
    const char* dev,
    const char* name,
    RADIO_INTERFACE version,
    RADIO_AIDL_INTERFACE aidl_interface)
{
    return g_strdup_printf("%s:%s:%d:%d", dev, name, version, aidl_interface);
}

static
void
radio_instance_tx_free(
    RadioInstanceTx* tx)
{
    radio_instance_unref(tx->instance);
    gutil_slice_free(tx);
}

static
void
radio_instance_tx_destroy(
    gpointer tx_data)
{
    RadioInstanceTx* tx = tx_data;

    if (tx->destroy) {
        tx->destroy(tx->user_data1, tx->user_data2);
    }
    radio_instance_tx_free(tx);
}

static
void
radio_instance_tx_complete(
    GBinderClient* client,
    GBinderRemoteReply* reply,
    int status,
    void* tx_data)
{
    RadioInstanceTx* tx = tx_data;

    if (tx->complete) {
        tx->complete(tx->instance, tx->id, status, tx->user_data1,
            tx->user_data2);
    }
}

/*==========================================================================*
 * Internal API
 *==========================================================================*/

gulong
radio_instance_send_request(
    RadioInstance* self,
    RADIO_REQ code,
    GBinderLocalRequest* args,
    RadioInstanceTxCompleteFunc complete,
    RadioInstanceTxDestroyFunc destroy,
    void* user_data1,
    void* user_data2)
{
    if (G_LIKELY(self)) {
        RadioInstancePriv* priv = self->priv;

        if (complete || destroy) {
            RadioInstanceTx* tx = g_slice_new(RadioInstanceTx);

            tx->instance = radio_instance_ref(self);
            tx->complete = complete;
            tx->destroy = destroy;
            tx->user_data1 = user_data1;
            tx->user_data2 = user_data2;
            radio_instance_notify_request_observers(self, code, args);
            tx->id = gbinder_client_transact(priv->client, code,
                GBINDER_TX_FLAG_ONEWAY, args, radio_instance_tx_complete,
                radio_instance_tx_destroy, tx);
            if (tx->id) {
                return tx->id;
            } else {
                radio_instance_tx_free(tx);
            }
        } else {
            /* No need to allocate the context */
            radio_instance_notify_request_observers(self, code, args);
            return gbinder_client_transact(priv->client, code,
                GBINDER_TX_FLAG_ONEWAY, args, NULL, NULL, NULL);
        }
    }
    return 0;
}

void
radio_instance_cancel_request(
    RadioInstance* self,
    gulong id)
{
    if (G_LIKELY(self)) {
        gbinder_client_cancel(self->priv->client, id);
    }
}

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
            const char* known = radio_ind_name2(self, ind);

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
    return radio_instance_new_with_modem_slot_version_and_interface(
        dev, name, modem, slot, version, RADIO_AIDL_INTERFACE_NONE);
}

RadioInstance*
radio_instance_new_with_modem_slot_version_and_interface(
    const char* dev,
    const char* name,
    const char* modem,
    int slot,
    RADIO_INTERFACE version,
    RADIO_AIDL_INTERFACE aidl_interface) /* Since 1.6.0 */
{
    if (dev && dev[0] && name && name[0]) {
        /* HIDL and AIDL would use different binder devices */
        char* key = radio_instance_make_key(dev, name, version, aidl_interface);
        RadioInstance* self = NULL;

        if (radio_instance_table) {
            self = g_hash_table_lookup(radio_instance_table, key);
        }
        if (self) {
            g_free(key);
            return radio_instance_ref(self);
        } else {
            self = radio_instance_create(dev, name, key, modem, slot, version,
                                         aidl_interface);
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

    if (dev && dev[0] && name && name[0] && radio_instance_table) {
        char* key = radio_instance_make_key(dev, name, version, RADIO_AIDL_INTERFACE_NONE);

        self = g_hash_table_lookup(radio_instance_table, key);
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

gsize
radio_instance_rpc_header_size(
    RadioInstance* self,
    RADIO_REQ req) /* Since 1.4.3 */
{
    if (G_LIKELY(self)) {
        RadioInstancePriv* priv = self->priv;
        GBytes* header = gbinder_client_rpc_header(priv->client, req);

        if (header) {
            return g_bytes_get_size(header);
        }
    }
    return 0;
}

const char*
radio_instance_req_name(
    RadioInstance* self,
    RADIO_REQ req)
{
    const char* known = radio_req_name2(self, req);

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
    const char* known = radio_resp_name2(self, resp);

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
    const char* known = radio_ind_name2(self, ind);

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
        GBinderClient* client = self->priv->client;
        const RADIO_REQ code = RADIO_REQ_RESPONSE_ACKNOWLEDGEMENT;

        radio_instance_notify_request_observers(self, code, NULL);
        return gbinder_client_transact_sync_oneway(client, code, NULL) >= 0;
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
        GBinderClient* client = self->priv->client;

        radio_instance_notify_request_observers(self, code, args);
        return gbinder_client_transact_sync_oneway(client, code, args) >= 0;
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
radio_instance_add_request_observer(
    RadioInstance* self,
    RADIO_REQ code,
    RadioRequestObserverFunc func,
    gpointer user_data) /* Since 1.4.3 */
{
    return radio_instance_add_request_observer_with_priority(self,
        RADIO_OBSERVER_PRIORITY_DEFAULT, code, func, user_data);
}

gulong
radio_instance_add_request_observer_with_priority(
    RadioInstance* self,
    RADIO_OBSERVER_PRIORITY priority,
    RADIO_REQ code,
    RadioRequestObserverFunc func,
    gpointer user_data) /* Since 1.4.3 */
{
    if (G_LIKELY(self) && G_LIKELY(func)) {
        const guint index = radio_observer_priority_index(priority);
        const RADIO_INSTANCE_SIGNAL sig = SIGNAL_OBSERVE_REQUEST_0 + index;

        /* Register signal on demand */
        if (!radio_instance_signals[sig]) {
            radio_instance_signals[sig] =
                g_signal_new(radio_instance_signal_observe_request_name
                    [index], RADIO_TYPE_INSTANCE,
                    G_SIGNAL_RUN_FIRST | G_SIGNAL_DETAILED,
                    0, NULL, NULL, NULL, G_TYPE_NONE,
                    2, G_TYPE_UINT, G_TYPE_POINTER);
        }

        return g_signal_connect_closure_by_id(self,
            radio_instance_signals[sig],
            radio_instance_req_quark(self, code),
            g_cclosure_new(G_CALLBACK(func), user_data, NULL), FALSE);
    }
    return 0;
}

gulong
radio_instance_add_response_observer(
    RadioInstance* self,
    RADIO_RESP code,
    RadioResponseObserverFunc func,
    gpointer user_data)
{
    return radio_instance_add_response_observer_with_priority(self,
        RADIO_OBSERVER_PRIORITY_DEFAULT, code, func, user_data);
}

gulong
radio_instance_add_response_observer_with_priority(
    RadioInstance* self,
    RADIO_OBSERVER_PRIORITY priority,
    RADIO_RESP code,
    RadioResponseObserverFunc func,
    gpointer user_data) /* Since 1.4.3 */
{
    if (G_LIKELY(self) && G_LIKELY(func)) {
        const guint index = radio_observer_priority_index(priority);
        const RADIO_INSTANCE_SIGNAL sig = SIGNAL_OBSERVE_RESPONSE_0 + index;

        /* Register signal on demand */
        if (!radio_instance_signals[sig]) {
            radio_instance_signals[sig] =
                g_signal_new(radio_instance_signal_observe_response_name
                    [index], RADIO_TYPE_INSTANCE,
                    G_SIGNAL_RUN_FIRST | G_SIGNAL_DETAILED,
                    0, NULL, NULL, NULL, G_TYPE_NONE,
                    3, G_TYPE_UINT, G_TYPE_POINTER, G_TYPE_POINTER);
        }

        return g_signal_connect_closure_by_id(self,
            radio_instance_signals[sig],
            radio_instance_resp_quark(self, code),
            g_cclosure_new(G_CALLBACK(func), user_data, NULL), FALSE);
    }
    return 0;
}

gulong
radio_instance_add_indication_observer(
    RadioInstance* self,
    RADIO_IND code,
    RadioIndicationObserverFunc func,
    gpointer user_data)
{
    return radio_instance_add_indication_observer_with_priority(self,
        RADIO_OBSERVER_PRIORITY_DEFAULT, code, func, user_data);
}

gulong
radio_instance_add_indication_observer_with_priority(
    RadioInstance* self,
    RADIO_OBSERVER_PRIORITY priority,
    RADIO_IND code,
    RadioIndicationObserverFunc func,
    gpointer user_data) /* Since 1.4.3 */
{
    if (G_LIKELY(self) && G_LIKELY(func)) {
        const guint index = radio_observer_priority_index(priority);
        const RADIO_INSTANCE_SIGNAL sig = SIGNAL_OBSERVE_INDICATION_0 + index;

        /* Register signal on demand */
        if (!radio_instance_signals[sig]) {
            radio_instance_signals[sig] =
                g_signal_new(radio_instance_signal_observe_indication_name
                    [index], RADIO_TYPE_INSTANCE,
                    G_SIGNAL_RUN_FIRST | G_SIGNAL_DETAILED,
                    0, NULL, NULL, NULL, G_TYPE_NONE,
                    3, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_POINTER);
        }

        return g_signal_connect_closure_by_id(self,
            radio_instance_signals[sig],
            radio_instance_ind_quark(self, code),
            g_cclosure_new(G_CALLBACK(func), user_data, NULL), FALSE);
    }
    return 0;
}

gulong
radio_instance_add_response_handler(
    RadioInstance* self,
    RADIO_RESP code,
    RadioResponseHandlerFunc func,
    gpointer user_data)
{
    return (G_LIKELY(self) && G_LIKELY(func)) ?
        g_signal_connect_closure_by_id(self,
            radio_instance_signals[SIGNAL_HANDLE_RESPONSE],
            radio_instance_resp_quark(self, code),
            g_cclosure_new(G_CALLBACK(func), user_data, NULL), FALSE) : 0;
}

gulong
radio_instance_add_indication_handler(
    RadioInstance* self,
    RADIO_IND code,
    RadioIndicationHandlerFunc func,
    gpointer user_data)
{
    return (G_LIKELY(self) && G_LIKELY(func)) ?
        g_signal_connect_closure_by_id(self,
            radio_instance_signals[SIGNAL_HANDLE_INDICATION],
            radio_instance_ind_quark(self, code),
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

gulong
radio_instance_add_connected_handler(
    RadioInstance* self,
    RadioInstanceFunc func,
    gpointer user_data) /* Since 1.4.3 */
{
    return (G_LIKELY(self) && G_LIKELY(func)) ? g_signal_connect(self,
        SIGNAL_CONNECTED_NAME, G_CALLBACK(func), user_data) : 0;
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
    priv->req_quarks = g_hash_table_new(g_direct_hash, g_direct_equal);
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
    g_hash_table_destroy(priv->req_quarks);
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

    /* Priority-based signals are registered on demand */
    radio_instance_signals[SIGNAL_HANDLE_RESPONSE] =
        g_signal_new(SIGNAL_HANDLE_RESPONSE_NAME, type,
            G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED, 0,
            g_signal_accumulator_true_handled, NULL, NULL,
            G_TYPE_BOOLEAN, 3, G_TYPE_UINT, G_TYPE_POINTER, G_TYPE_POINTER);
    radio_instance_signals[SIGNAL_HANDLE_INDICATION] =
        g_signal_new(SIGNAL_HANDLE_INDICATION_NAME, type,
            G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED, 0,
            g_signal_accumulator_true_handled, NULL, NULL,
            G_TYPE_BOOLEAN, 3, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_POINTER);
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
    radio_instance_signals[SIGNAL_CONNECTED] =
        g_signal_new(SIGNAL_CONNECTED_NAME, type,
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

