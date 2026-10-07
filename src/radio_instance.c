/*
 * Copyright (C) 2025-2026 Jolla Mobile Ltd
 * Copyright (C) 2018-2025 Slava Monich <slava@monich.com>
 * Copyright (C) 2018-2022 Jolla Ltd.
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

#include <gutil_cleanup.h>
#include <gutil_idlepool.h>
#include <gutil_macros.h>
#include <gutil_misc.h>
#include <gutil_strv.h>

#include <glib-object.h>

typedef struct radio_interface_flavor {
    GBINDER_STABILITY_LEVEL stability;
    const RadioResponseInfo* (*read_response_info)(GBinderReader* reader);
} RadioInterfaceFlavor;

typedef struct radio_instance_local_obj {
    RadioInstance* self;
    guint index;
    GBinderLocalObject* local;
    const char* const* ifaces;
} RadioInstanceLocalObj;

typedef struct radio_instance_local_objs {
    RadioInstanceLocalObj resp;
    RadioInstanceLocalObj ind;
    guint connected_ind;
} RadioInstanceLocalObjs;

typedef GObjectClass RadioInstanceClass;
struct radio_instance_priv {
    const RadioInterfaceFlavor* flavor;
    GUtilIdlePool* idle;
    GUtilCleanup* cleanup;
    GBinderClient* client;
    GBinderRemoteObject* remote;
    RadioInstanceLocalObjs* local;
    guint n_local;
    GHashTable* req_quarks;
    GHashTable* resp_quarks;
    GHashTable* ind_quarks;
    GHashTable* req_names;
    GHashTable* resp_names;
    GHashTable* ind_names;
    gulong death_id;
    char* dev;
    char* slot;
    char* key;
    char* modem;
    const char* req_iface;
    guint ack_req;
    guint resp_ack_req;
};

G_DEFINE_TYPE(RadioInstance, radio_instance, G_TYPE_OBJECT)

#define PARENT_CLASS radio_instance_parent_class
#define THIS_TYPE RADIO_TYPE_INSTANCE
#define THIS(obj) RADIO_INSTANCE(obj)

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

static const RadioInterfaceFlavor radio_instance_interface_flavor_hidl = {
    GBINDER_STABILITY_SYSTEM,
    radio_read_response_info_hidl
};

static const RadioInterfaceFlavor radio_instance_interface_flavor_aidl = {
    GBINDER_STABILITY_VINTF,
    radio_read_response_info_aidl
};

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

/* Known RADIO_INTERFACE versions in the opposite order: */
#define RADIO_INTERFACE_VERSIONS(v) v(1_5) v(1_4) v(1_3) v(1_2) v(1_1) v(1_0)
#define RADIO_INTERFACE_INDEX_(x) (RADIO_INTERFACE_COUNT - x - 1)
#define RADIO_INTERFACE_INDEX(v) RADIO_INTERFACE_INDEX_(RADIO_INTERFACE_##v)
#define DEFAULT_INTERFACE RADIO_INTERFACE_1_0

static const RadioClientIfaceInfo radio_hidl_iface_info[] = {
    #define INFO(v) {RADIO_##v, RADIO_##v##_REQ_LAST },
    RADIO_INTERFACE_VERSIONS(INFO)
    #undef INFO
    RADIO_IFACE_INFO_END
};

static const char* const radio_hidl_response_ifaces[] = {
    #define IFACE(v) RADIO_RESPONSE_##v,
    RADIO_INTERFACE_VERSIONS(IFACE)
    #undef IFACE
    NULL
};

static const char* const radio_hidl_indication_ifaces[] = {
    #define IFACE(v) RADIO_INDICATION_##v,
    RADIO_INTERFACE_VERSIONS(IFACE)
    #undef IFACE
    NULL
};

static const RadioTxName radio_hidl_req_names[] = {
    #define REQ(req,resp,Name,NAME) { req, #Name },
    RADIO_CALL_1_0(REQ)
    RADIO_CALL_1_1(REQ)
    RADIO_CALL_1_2(REQ)
    RADIO_CALL_1_3(REQ)
    #undef REQ
    #define REQ(req,resp,Name,NAME,x) { req, #Name #x },
    RADIO_CALL_1_4_(REQ)
    RADIO_CALL_1_5_(REQ)
    #undef REQ
    { RADIO_REQ_SET_RESPONSE_FUNCTIONS, "setResponseFunctions" },
    { RADIO_REQ_RESPONSE_ACKNOWLEDGEMENT, "responseAcknowledgement" },
    { RADIO_REQ_START_NETWORK_SCAN_1_2, "startNetworkScan_1_2" },
    { RADIO_REQ_SET_INDICATION_FILTER_1_2, "setIndicationFilter_1_2" },
    { RADIO_REQ_SETUP_DATA_CALL_1_2, "setupDataCall_1_2" },
    { RADIO_REQ_DEACTIVATE_DATA_CALL_1_2, "deactivateDataCall_1_2" },
    { RADIO_REQ_SET_INITIAL_ATTACH_APN_1_4, "setInitialAttachApn_1_4" },
    { RADIO_REQ_SET_DATA_PROFILE_1_4, "setDataP1rofile_1_4" },
    RADIO_TX_NAMES_END
};

static const RadioTxName radio_hidl_resp_names[] = {
    #define RESP(req,resp,Name,NAME) { resp, #Name "Response" },
    RADIO_CALL_1_0(RESP)
    RADIO_CALL_1_1(RESP)
    RADIO_CALL_1_2(RESP)
    RADIO_CALL_1_3(RESP)
    #undef RESP
    #define RESP(req,resp,Name,NAME,x) { resp, #Name "Response" #x },
    RADIO_CALL_1_4_(RESP)
    RADIO_CALL_1_5_(RESP)
    #undef RESP
    { RADIO_RESP_ACKNOWLEDGE_REQUEST, "acknowledgeRequest" },
    { RADIO_RESP_GET_CELL_INFO_LIST_1_2, "getCellInfoListResponse_1_2" },
    { RADIO_RESP_GET_ICC_CARD_STATUS_1_2, "getIccCardStatusResponse_1_2" },
    { RADIO_RESP_GET_CURRENT_CALLS_1_2, "getCurrentCallsResponse_1_2" },
    { RADIO_RESP_GET_SIGNAL_STRENGTH_1_2, "getSignalStrengthResponse_1_2" },
    { RADIO_RESP_GET_VOICE_REGISTRATION_STATE_1_2,
      "getVoiceRegistrationStateResponse_1_2"},
    { RADIO_RESP_GET_DATA_REGISTRATION_STATE_1_2,
      "getDataRegistrationStateResponse_1_2" },
    { RADIO_RESP_GET_CELL_INFO_LIST_RESPONSE_1_4,
      "getCellInfoListResponse_1_4" },
    { RADIO_RESP_GET_DATA_REGISTRATION_STATE_RESPONSE_1_4,
      "getDataRegistrationStateResponse_1_4" },
    { RADIO_RESP_GET_ICC_CARD_STATUS_RESPONSE_1_4,
      "getIccCardStatusResponse_1_4" },
    { RADIO_RESP_GET_DATA_CALL_LIST_RESPONSE_1_4,
      "getDataCallListResponse_1_4" },
    { RADIO_RESP_GET_DATA_CALL_LIST_1_5, "getDataCallList_1_5" },
    { RADIO_RESP_GET_CELL_INFO_LIST_1_5, "getCellInfoListResponse_1_5" },
    { RADIO_RESP_GET_ICC_CARD_STATUS_1_5, "getIccCardStatus_1_5" },
    RADIO_TX_NAMES_END
};

static const RadioTxName radio_hidl_ind_names[] = {
    #define IND(code,Name,NAME) { code, #Name },
    RADIO_EVENT_1_0(IND)
    RADIO_EVENT_1_1(IND)
    RADIO_EVENT_1_2(IND)
    RADIO_EVENT_1_4(IND)
    RADIO_EVENT_1_5(IND)
    #undef IND
    RADIO_TX_NAMES_END
};

static const RadioInterfaceDesc radio_hidl_resp_fn[] = {
    #define FN(v) { \
        radio_hidl_response_ifaces + RADIO_INTERFACE_INDEX(v), \
        radio_hidl_resp_names },
    RADIO_INTERFACE_VERSIONS(FN)
    #undef FN
};

static const RadioInterfaceDesc radio_hidl_ind_fn[] = {
    #define FN(v) { \
        radio_hidl_indication_ifaces + RADIO_INTERFACE_INDEX(v), \
        radio_hidl_ind_names },
    RADIO_INTERFACE_VERSIONS(FN)
    #undef FN
};

static const RadioServiceDesc radio_hidl_desc[] = {
    #define DESC(v) { \
        RADIO_INTERFACE_TYPE_HIDL, \
        RADIO_##v, radio_hidl_req_names, radio_hidl_iface_info, \
        radio_hidl_resp_fn + RADIO_INTERFACE_INDEX(v), \
        radio_hidl_ind_fn + RADIO_INTERFACE_INDEX(v), \
        RADIO_REQ_SET_RESPONSE_FUNCTIONS, \
        RADIO_REQ_RESPONSE_ACKNOWLEDGEMENT, \
        RADIO_RESP_ACKNOWLEDGE_REQUEST, \
        RADIO_IND_RIL_CONNECTED },
    RADIO_INTERFACE_VERSIONS(DESC)
    #undef DESC
};

/* The order of these must match the RADIO_AIDL_INTERFACE enum */
#define AIDL_INTERFACES(i) \
    i(DATA) i(IMS) i(MESSAGING) i(MODEM) i(NETWORK) i(SIM) i(VOICE)

#define RADIO_REQ_(req,resp,Name,NAME) { req, #Name },

static const RadioTxName radio_aidl_DATA_req_names[] = {
    RADIO_DATA_CALL_1(RADIO_REQ_)
    { RADIO_DATA_REQ_SET_RESPONSE_FUNCTIONS, "setResponseFunctions" },
    { RADIO_DATA_REQ_RESPONSE_ACKNOWLEDGEMENT, "responseAcknowledgement" },
    RADIO_TX_NAMES_END
};

static const RadioTxName radio_aidl_IMS_req_names[] = {
    #define RADIO_IMS_REQ_(req,resp,Name,NAME) { req, #Name },
    RADIO_IMS_CALL_1(RADIO_IMS_REQ_)
    { RADIO_IMS_REQ_SET_RESPONSE_FUNCTIONS, "setResponseFunctions" },
    RADIO_TX_NAMES_END
};

static const RadioTxName radio_aidl_MESSAGING_req_names[] = {
    RADIO_MESSAGING_CALL_1(RADIO_REQ_)
    { RADIO_MESSAGING_REQ_SET_RESPONSE_FUNCTIONS, "setResponseFunctions" },
    { RADIO_MESSAGING_REQ_RESPONSE_ACKNOWLEDGEMENT, "responseAcknowledgement" },
    RADIO_TX_NAMES_END
};

static const RadioTxName radio_aidl_MODEM_req_names[] = {
    RADIO_MODEM_CALL_1(RADIO_REQ_)
    { RADIO_MODEM_REQ_SET_RESPONSE_FUNCTIONS, "setResponseFunctions" },
    { RADIO_MODEM_REQ_RESPONSE_ACKNOWLEDGEMENT, "responseAcknowledgement" },
    RADIO_TX_NAMES_END
};

static const RadioTxName radio_aidl_NETWORK_req_names[] = {
    RADIO_NETWORK_CALL_1(RADIO_REQ_)
    { RADIO_NETWORK_REQ_SET_RESPONSE_FUNCTIONS, "setResponseFunctions" },
    { RADIO_NETWORK_REQ_RESPONSE_ACKNOWLEDGEMENT, "responseAcknowledgement" },
    RADIO_TX_NAMES_END
};

static const RadioTxName radio_aidl_SIM_req_names[] = {
    RADIO_SIM_CALL_1(RADIO_REQ_)
    { RADIO_SIM_REQ_SET_RESPONSE_FUNCTIONS, "setResponseFunctions" },
    { RADIO_SIM_REQ_RESPONSE_ACKNOWLEDGEMENT, "responseAcknowledgement" },
    RADIO_TX_NAMES_END
};

static const RadioTxName radio_aidl_VOICE_req_names[] = {
    RADIO_VOICE_CALL_1(RADIO_REQ_)
    { RADIO_VOICE_REQ_HANGUP, "hangup" },
    { RADIO_VOICE_REQ_SET_RESPONSE_FUNCTIONS, "setResponseFunctions" },
    { RADIO_VOICE_REQ_RESPONSE_ACKNOWLEDGEMENT, "responseAcknowledgement" },
    RADIO_TX_NAMES_END
};

#undef RADIO_REQ_
#define RADIO_RESP_(req,resp,Name,NAME) { resp, #Name "Response" },

static const RadioTxName radio_aidl_DATA_resp_names[] = {
    RADIO_DATA_CALL_1(RADIO_RESP_)
    { RADIO_DATA_RESP_ACKNOWLEDGE_REQUEST, "acknowledgeRequest" },
    RADIO_TX_NAMES_END
};

static const RadioTxName radio_aidl_IMS_resp_names[] = {
    RADIO_IMS_CALL_1(RADIO_RESP_)
    RADIO_TX_NAMES_END
};

static const RadioTxName radio_aidl_MESSAGING_resp_names[] = {
    RADIO_MESSAGING_CALL_1(RADIO_RESP_)
    { RADIO_MESSAGING_RESP_ACKNOWLEDGE_REQUEST, "acknowledgeRequest" },
    RADIO_TX_NAMES_END
};

static const RadioTxName radio_aidl_MODEM_resp_names[] = {
    RADIO_MODEM_CALL_1(RADIO_RESP_)
    { RADIO_MODEM_RESP_ACKNOWLEDGE_REQUEST, "acknowledgeRequest" },
    RADIO_TX_NAMES_END
};

static const RadioTxName radio_aidl_NETWORK_resp_names[] = {
    RADIO_NETWORK_CALL_1(RADIO_RESP_)
    { RADIO_NETWORK_RESP_ACKNOWLEDGE_REQUEST, "acknowledgeRequest" },
    RADIO_TX_NAMES_END
};

static const RadioTxName radio_aidl_SIM_resp_names[] = {
    RADIO_SIM_CALL_1(RADIO_RESP_)
    { RADIO_SIM_RESP_ACKNOWLEDGE_REQUEST, "acknowledgeRequest" },
    RADIO_TX_NAMES_END
};

static const RadioTxName radio_aidl_VOICE_resp_names[] = {
    RADIO_VOICE_CALL_1(RADIO_RESP_)
    { RADIO_VOICE_RESP_ACKNOWLEDGE_REQUEST, "acknowledgeRequest" },
    { RADIO_VOICE_RESP_HANGUP_CONNECTION_RESPONSE, "hangupConnectionResponse" },
    RADIO_TX_NAMES_END
};

#undef RADIO_RESP_

/* Indication names follow the same pattern, can be generated */
#define RADIO_IND_(code,Name,NAME) { code, #Name },
#define IND_NAMES(TYPE) \
    static const RadioTxName radio_aidl_##TYPE##_ind_names[] = { \
        RADIO_##TYPE##_IND_1(RADIO_IND_) { 0, NULL } \
    };
AIDL_INTERFACES(IND_NAMES)
#undef IND_NAMES
#undef RADIO_IND_

static const RadioClientIfaceInfo radio_aidl_iface_info[] = {
    #define INFO(x) { RADIO_##x, RADIO_##x##_1_REQ_LAST }, RADIO_IFACE_INFO_END,
    AIDL_INTERFACES(INFO)
    #undef INFO
};

static const char* const radio_aidl_response_ifaces[] = {
    #define IFACE(x) RADIO_##x##_RESPONSE, NULL,
    AIDL_INTERFACES(IFACE)
    #undef IFACE
};

static const char* const radio_aidl_indication_ifaces[] = {
    #define IFACE(x) RADIO_##x##_INDICATION, NULL,
    AIDL_INTERFACES(IFACE)
    #undef IFACE
};

static const RadioInterfaceDesc radio_aidl_resp_fn[] = {
    #define FN(TYPE) { \
        radio_aidl_response_ifaces + 2*RADIO_##TYPE##_INTERFACE, \
        radio_aidl_##TYPE##_resp_names },
    AIDL_INTERFACES(FN)
    #undef FN
};

static const RadioInterfaceDesc radio_aidl_ind_fn[] = {
    #define FN(TYPE) { \
        radio_aidl_indication_ifaces + 2*RADIO_##TYPE##_INTERFACE, \
        radio_aidl_##TYPE##_ind_names },
    AIDL_INTERFACES(FN)
    #undef FN
};

#define AIDL_INTERFACE_DESC_(TYPE,respAckReq,ackReqResp,connectedInd) \
        RADIO_INTERFACE_TYPE_AIDL, \
        RADIO_##TYPE, \
        radio_aidl_##TYPE##_req_names, \
        radio_aidl_iface_info + 2 * RADIO_##TYPE##_INTERFACE, \
        radio_aidl_resp_fn + RADIO_##TYPE##_INTERFACE, \
        radio_aidl_ind_fn + RADIO_##TYPE##_INTERFACE, \
        RADIO_##TYPE##_REQ_SET_RESPONSE_FUNCTIONS, \
        respAckReq, ackReqResp, connectedInd

#define AIDL_INTERFACE_DESC(TYPE) \
        AIDL_INTERFACE_DESC_(TYPE, \
        RADIO_##TYPE##_REQ_RESPONSE_ACKNOWLEDGEMENT, \
        RADIO_##TYPE##_RESP_ACKNOWLEDGE_REQUEST, \
        RADIO_IND_NONE)

static const RadioServiceDesc radio_aidl_desc[] = {
    { AIDL_INTERFACE_DESC(DATA) },
    { AIDL_INTERFACE_DESC_(IMS,
        RADIO_REQ_NONE,
        RADIO_RESP_NONE,
        RADIO_IND_NONE) },
    { AIDL_INTERFACE_DESC(MESSAGING) },
    { AIDL_INTERFACE_DESC_(MODEM,
        RADIO_MODEM_REQ_RESPONSE_ACKNOWLEDGEMENT,
        RADIO_MODEM_RESP_ACKNOWLEDGE_REQUEST,
        RADIO_MODEM_IND_RIL_CONNECTED) },
    { AIDL_INTERFACE_DESC(NETWORK) },
    { AIDL_INTERFACE_DESC(SIM) },
    { AIDL_INTERFACE_DESC(VOICE) }
};
G_STATIC_ASSERT(G_N_ELEMENTS(radio_aidl_desc) == RADIO_AIDL_INTERFACE_COUNT);

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
    GBinderLocalObject* local,
    GBinderRemoteRequest* req,
    guint code,
    guint flags,
    int* status,
    void* user_data)
{
    RadioInstanceLocalObj* obj = user_data;
    RadioInstance* self = obj->self;
    RadioInstancePriv* priv = self->priv;
    const char* iface = gbinder_remote_request_interface(req);

    if (gutil_strv_contains((const GStrV*)obj->ifaces, iface)) {
        GBinderReader reader;
        guint type;

        /* All these should be one-way */
        GASSERT(flags & GBINDER_TX_FLAG_ONEWAY);
        gbinder_remote_request_init_reader(req, &reader);
        if (gbinder_reader_read_uint32(&reader, &type)) {
            if (type == RADIO_IND_UNSOLICITED || type == RADIO_IND_ACK_EXP) {
                const guint ext = RADIO_TX_EXT_CODE(obj->index, code);
                const GQuark quark = radio_instance_ind_quark(self, ext);
                const guint* signals = radio_instance_signals +
                    SIGNAL_OBSERVE_INDICATION_0;
                int p = RADIO_OBSERVER_PRIORITY_HIGHEST;
                gboolean handled = FALSE;

                /* High-priority observers are notified first */
                for (; p > RADIO_OBSERVER_PRIORITY_DEFAULT; p--) {
                    if (signals[RADIO_OBSERVER_PRIORITY_INDEX(p)]) {
                        g_signal_emit(self, signals
                            [RADIO_OBSERVER_PRIORITY_INDEX(p)],
                            quark, ext, type, &reader);
                    }
                }

                /* rilConnected is a special case */
                if (code && code == priv->local[obj->index].connected_ind) {
                    if (G_UNLIKELY(self->connected)) {
                        /* We are only supposed to receive it once */
                        GWARN("%s received unexpected Connected indication",
                            self->slot);
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
                    quark, ext, type, &reader, &handled);

                /* And then remaining observers in their priority order */
                for (; p >= RADIO_OBSERVER_PRIORITY_LOWEST; p--) {
                    if (signals[RADIO_OBSERVER_PRIORITY_INDEX(p)]) {
                        g_signal_emit(self, signals
                            [RADIO_OBSERVER_PRIORITY_INDEX(p)],
                            quark, ext, type, &reader);
                    }
                }

                /* Ack unhandled indications */
                if (type == RADIO_IND_ACK_EXP && !handled) {
                    if (radio_instance_ack(self)) {
                        GDEBUG("acked unhandled indication %s %u", iface, code);
                    } else {
                        GDEBUG("indication %s %u requires an ack", iface, code);
                    }
                }
                *status = GBINDER_STATUS_OK;
                return NULL;
            } else {
                GWARN("Invalid indication %s %u type 0x%x", iface, code, type);
            }
        } else {
            GWARN("Failed to decode indication %s %u", iface, code);
        }
    } else {
        GWARN("Unexpected indication %s %u", iface, code);
    }
    *status = GBINDER_STATUS_FAILED;
    return NULL;
}

static
GBinderLocalReply*
radio_instance_response(
    GBinderLocalObject* local,
    GBinderRemoteRequest* req,
    guint code,
    guint flags,
    int* status,
    void* user_data)
{
    RadioInstanceLocalObj* obj = user_data;
    RadioInstance* self = obj->self;
    RadioInstancePriv* priv = self->priv;
    const char* iface = gbinder_remote_request_interface(req);
    const RadioResponseInfo* info = NULL;
    GBinderReader reader;
    gint32 ack_serial = 0;

    gbinder_remote_request_init_reader(req, &reader);
    if (gutil_strv_contains((GStrV*)obj->ifaces, iface)) {
        /* Weird terminology - response that requests an ack (request) */
        if (code == priv->resp_ack_req) {
            gbinder_reader_read_int32(&reader, &ack_serial);
        } else {
            /* All other responses have RadioResponseInfo */
            info = priv->flavor->read_response_info(&reader);
        }
    } else {
        GWARN("Unexpected response %s %u", iface, code);
        *status = GBINDER_STATUS_FAILED;
        return NULL;
    }

    /* All these should be one-way transactions */
    GASSERT(flags & GBINDER_TX_FLAG_ONEWAY);

    if (ack_serial) {
        GDEBUG("%s %u ack", iface, code);
        g_signal_emit(self, radio_instance_signals[SIGNAL_ACK], 0,
            ack_serial);
    } else if (info) {
        const guint ext = RADIO_TX_EXT_CODE(obj->index, code);
        const GQuark quark = radio_instance_resp_quark(self, ext);
        const guint* signals = radio_instance_signals +
            SIGNAL_OBSERVE_RESPONSE_0;
        int p = RADIO_OBSERVER_PRIORITY_HIGHEST;
        gboolean handled = FALSE;

        /* High-priority observers are notified first */
        for (; p > RADIO_OBSERVER_PRIORITY_DEFAULT; p--) {
            if (signals[RADIO_OBSERVER_PRIORITY_INDEX(p)]) {
                g_signal_emit(self, signals
                    [RADIO_OBSERVER_PRIORITY_INDEX(p)],
                    quark, ext, info, &reader);
            }
        }

        /* Then handlers */
        g_signal_emit(self, radio_instance_signals
            [SIGNAL_HANDLE_RESPONSE],
            quark, ext, info, &reader, &handled);

        /* And then remaining observers in their priority order */
        for (; p >= RADIO_OBSERVER_PRIORITY_LOWEST; p--) {
            if (signals[RADIO_OBSERVER_PRIORITY_INDEX(p)]) {
                g_signal_emit(self, signals
                    [RADIO_OBSERVER_PRIORITY_INDEX(p)],
                    quark, ext, info, &reader);
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
radio_instance_local_obj_drop(
    RadioInstanceLocalObj* obj)
{
    if (obj->local) {
        gbinder_local_object_drop(obj->local);
        obj->local = NULL;
    }
}

static
void
radio_instance_drop_binder(
    RadioInstance* self)
{
    RadioInstancePriv* priv = self->priv;
    guint i;

    for (i = 0; i < priv->n_local; i++) {
        RadioInstanceLocalObjs* objs = priv->local + i;

        radio_instance_local_obj_drop(&objs->resp);
        radio_instance_local_obj_drop(&objs->ind);
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
    RadioInstance* self = THIS(user_data);

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
void
radio_instance_store_names(
    const RadioTxName* in,
    guint index,
    GHashTable* out,
    GUtilCleanup* cleanup)
{
    if (in) {
        const RadioTxName* ptr;

        for (ptr = in; ptr->code; ptr++) {
            const uint ext_code = RADIO_TX_EXT_CODE(index, ptr->code);

            /*
             * Internal static strings are stored as pointers,
             * external get dupped.
             */
            g_hash_table_insert(out, GUINT_TO_POINTER(ext_code), cleanup ?
                gutil_cleanup_add_ptr(cleanup, g_strdup(ptr->name)) :
                (gpointer) ptr->name);
        }
    }
}

static
void
radio_instance_local_obj_init(
    RadioInstanceLocalObj* obj,
    GBinderServiceManager* sm,
    RadioInstance* self,
    guint index,
    GHashTable* names,
    GBinderLocalTransactFunc handler,
    const RadioInterfaceDesc* desc,
    gboolean internal)
{
    obj->self = self;
    obj->index = index;
    if (desc) {
        RadioInstancePriv* priv = self->priv;
        GUtilCleanup* cleanup = internal ?  NULL : priv->cleanup;

        gbinder_local_object_set_stability(obj->local =
            gbinder_servicemanager_new_local_object2(sm, desc->ifaces,
                handler, obj), priv->flavor->stability);
        radio_instance_store_names(desc->tx_names, index, names, cleanup);
        if (cleanup) {
            obj->ifaces = (const char**) gutil_cleanup_add_strv(cleanup,
                g_strdupv((char**) desc->ifaces));
        } else {
            obj->ifaces = desc->ifaces;
        }
    }
}

static
RadioInstance*
radio_instance_create_internal(
    GBinderServiceManager* sm,
    GBinderRemoteObject* remote,
    const char* dev,
    const char* slot,
    const char* key,
    const char* modem,
    int slot_index,
    RADIO_INTERFACE hidl_interface,
    RADIO_AIDL_INTERFACE aidl_interface,
    const RadioServiceDesc2* desc,
    gboolean internal)
{
    RadioInstance* self = g_object_new(THIS_TYPE, NULL);
    RadioInstancePriv* priv = self->priv;
    guint i, ni = 0;
    int status;
    const RadioResponseFunctionsDesc* fn;
    const RadioInterfaceFlavor* flavor =
        (desc->interface_type == RADIO_INTERFACE_TYPE_AIDL) ?
            &radio_instance_interface_flavor_aidl :
            &radio_instance_interface_flavor_hidl;

    if (desc->iface_info) {
        const RadioClientIfaceInfo* ptr;

        /* Count client interfaces */
        for (ptr = desc->iface_info; ptr->iface; ptr++, ni++);
    }

    self->slot = priv->slot = g_strdup(slot);
    self->dev = priv->dev = g_strdup(dev);
    self->key = priv->key = g_strdup(key);
    self->modem = priv->modem = g_strdup(modem);
    self->slot_index = slot_index;
    self->version = hidl_interface;
    self->interface_aidl = aidl_interface;
    self->interface_type = desc->interface_type;
    self->connected = TRUE; /* no signal => connected */

    priv->flavor = flavor;
    priv->ack_req = desc->ack_req;
    priv->resp_ack_req = desc->resp_ack_req;
    priv->remote = gbinder_remote_object_ref(remote);
    priv->death_id = gbinder_remote_object_add_death_handler(remote,
        radio_instance_died, self);
    priv->client = gbinder_client_new2(remote, desc->iface_info, ni);

    if (desc->resp_fn) {
        /* Count response functions */
        for (fn = desc->resp_fn; fn->resp || fn->ind; fn++, priv->n_local++);
    }
    priv->local = g_new0(RadioInstanceLocalObjs, priv->n_local);
    for (i = 0; i < priv->n_local; i++) {
        RadioInstanceLocalObjs* objs = priv->local + i;

        fn = desc->resp_fn + i;
        radio_instance_local_obj_init(&objs->resp, sm, self, i,
            priv->resp_names, radio_instance_response, fn->resp, internal);
        radio_instance_local_obj_init(&objs->ind, sm, self, i,
            priv->ind_names, radio_instance_indication, fn->ind, internal);
        objs->connected_ind = fn->connected_ind;
        if (fn->connected_ind) {
            self->connected = FALSE; /* Expect the indication */
        }
    }

    radio_instance_store_names(desc->req_names, 0, priv->req_names,
        internal ?  NULL : priv->cleanup);
    if (internal) {
        priv->req_iface = desc->req_iface;
    } else {
        priv->req_iface = gutil_cleanup_add_ptr(priv->cleanup,
            g_strdup(desc->req_iface));
    }

    /* Issue interface-specific setResponseFunctions calls */
    for (i = 0; i < priv->n_local; i++) {
        const guint tx_code = desc->resp_fn[i].set_response_functions_req;
        RadioInstanceLocalObjs* objs = priv->local + i;
        GBinderWriter writer;
        GBinderLocalRequest* req = gbinder_client_new_request2(priv->client,
            tx_code);

        gbinder_local_request_init_writer(req, &writer);
        if (objs->resp.local) {
            gbinder_writer_append_local_object(&writer, objs->resp.local);
        }
        if (objs->ind.local) {
            gbinder_writer_append_local_object(&writer, objs->ind.local);
        }
        gbinder_remote_reply_unref(gbinder_client_transact_sync_reply
            (priv->client, tx_code, req, &status));
        GVERBOSE_("%s %s status %d", (char*)
            g_hash_table_lookup(priv->req_names,
            GUINT_TO_POINTER(tx_code)), slot, status);
        gbinder_local_request_unref(req);
    }

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
char*
radio_instance_make_key(
    const char* dev,
    const char* fqname)
{
    return g_strdup_printf("%s:%s", dev, fqname);
}

static
char*
radio_instance_make_desc_key(
    const char* dev,
    const char* slot,
    const RadioServiceDesc* desc)
{
    if (desc) {
        char* fqname = g_strconcat(desc->req_iface, "/", slot, NULL);
        char* key = radio_instance_make_key(dev, fqname);

        g_free(fqname);
        return key;
    }
    return NULL;
}

static
RadioInstance*
radio_instance_new_internal2(
    const char* dev,
    const char* name,
    const char* modem,
    int slot,
    RADIO_INTERFACE hidl_interface,
    RADIO_AIDL_INTERFACE aidl_interface,
    const RadioServiceDesc2* desc,
    gboolean internal)
{
    RadioInstance* self = NULL;
    GBinderServiceManager* sm  = NULL;

    if (desc && dev && dev[0] && name && name[0] &&
       (sm = gbinder_servicemanager_new(dev)) != NULL) {
        char* fqname = g_strconcat(desc->req_iface, "/", name, NULL);
        char* key = radio_instance_make_key(dev, fqname);

        if (radio_instance_table) {
            self = g_hash_table_lookup(radio_instance_table, key);
        }

        if (self) {
            /* It's already there */
            radio_instance_ref(self);
        } else {
            GBinderRemoteObject* obj = /* autoreleased */
                gbinder_servicemanager_get_service_sync(sm, fqname, NULL);

            if (obj) {
                GINFO("Connected to %s", fqname);
                GVERBOSE_("%s", key);
                self = radio_instance_create_internal(sm, obj, dev, name, key,
                    modem, slot, hidl_interface, aidl_interface, desc,
                    internal);

                /* Create the instance table on demand */
                if (!radio_instance_table) {
                    radio_instance_table = g_hash_table_new_full
                        (g_str_hash, g_str_equal, g_free, NULL);
                }
                g_hash_table_replace(radio_instance_table, g_strdup(key), self);
                g_object_weak_ref(G_OBJECT(self), radio_instance_gone, key);
                radio_registry_instance_added(self);
                key = NULL; /* will be freed by radio_instance_gone() */
            }
        }
        g_free(fqname);
        g_free(key);
        gbinder_servicemanager_unref(sm);
    }

    return self;
}

static
const RadioServiceDesc*
radio_instance_find_desc(
    RADIO_INTERFACE hidl_version,
    RADIO_AIDL_INTERFACE aidl_interface)
{
    /* Find the builtin descriptor */
    if (aidl_interface > RADIO_AIDL_INTERFACE_NONE &&
        aidl_interface < RADIO_AIDL_INTERFACE_COUNT) {
        return radio_aidl_desc + aidl_interface;
    } else if (hidl_version > RADIO_INTERFACE_NONE &&
        hidl_version < RADIO_INTERFACE_COUNT) {
        return radio_hidl_desc + RADIO_INTERFACE_INDEX_(hidl_version);
    }
    return NULL;
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

RadioInstance*
radio_instance_new_internal(
    const char* dev,
    const char* name,
    const char* modem,
    int slot,
    RADIO_INTERFACE hidl_interface,
    RADIO_AIDL_INTERFACE aidl_interface,
    const RadioServiceDesc* desc,
    gboolean internal)
{
    if (desc) {
        RadioServiceDesc2 desc2;
        RadioResponseFunctionsDesc fns[2];
        RadioResponseFunctionsDesc* fn = fns;

        /* Convert RadioServiceDesc -> RadioServiceDesc2 */
        memset(&desc2, 0, sizeof(desc2));
        desc2.interface_type = desc->interface_type;
        desc2.req_iface = desc->req_iface;
        desc2.req_names = desc->req_names;
        desc2.resp_fn = fn;
        desc2.iface_info = desc->iface_info;
        desc2.ack_req = desc->ack_req;
        desc2.resp_ack_req = desc->resp_ack_req;

        memset(fns, 0, sizeof(fns));
        fn->set_response_functions_req = desc->set_response_functions_req;
        fn->connected_ind = desc->connected_ind;
        fn->resp = desc->resp;
        fn->ind = desc->ind;

        return radio_instance_new_internal2(dev, name, modem, slot,
            hidl_interface, aidl_interface, &desc2, internal);
    }
    return NULL;
}

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
radio_instance_req_quark(
    RadioInstance* self,
    guint req)
{
    GQuark q = 0;

    if (req) {
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

GQuark
radio_instance_resp_quark(
    RadioInstance* self,
    guint resp)
{
    GQuark q = 0;

    if (resp) {
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

GQuark
radio_instance_ind_quark(
    RadioInstance* self,
    guint ind)
{
    GQuark q = 0;

    if (ind != RADIO_IND_ANY) {
        RadioInstancePriv* priv = self->priv;
        gpointer key = GUINT_TO_POINTER(ind);

        q = GPOINTER_TO_UINT(g_hash_table_lookup(priv->ind_quarks, key));
        if (!q) {
            const char* known = g_hash_table_lookup(priv->ind_names,
                GUINT_TO_POINTER(ind));

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
    RADIO_INTERFACE max_version,
    RADIO_AIDL_INTERFACE aidl_interface) /* Since 1.6.0 */
{
    RadioInstance* self = NULL;

    if (dev && dev[0] && name && name[0]) {
        GBinderServiceManager* sm = gbinder_servicemanager_new(dev);

        if (sm) {

            if (aidl_interface > RADIO_AIDL_INTERFACE_NONE &&
                aidl_interface < RADIO_AIDL_INTERFACE_COUNT) {
                self = radio_instance_new_internal(dev, name, modem, slot,
                    RADIO_INTERFACE_NONE, aidl_interface,
                    radio_aidl_desc + aidl_interface,
                    TRUE);
            } else {
                RADIO_INTERFACE i;

                for (i = MIN(max_version, RADIO_INTERFACE_COUNT - 1);
                     i > RADIO_INTERFACE_NONE && !self;
                     i--) {
                    self = radio_instance_new_internal(dev, name, modem, slot,
                        i, RADIO_AIDL_INTERFACE_NONE,
                        radio_hidl_desc + RADIO_INTERFACE_INDEX_(i),
                        TRUE);
                }
            }
            gbinder_servicemanager_unref(sm);
        }
    }
    return self;
}

RadioInstance*
radio_instance_new_with_service_desc(
    const char* dev,
    const char* name,
    const char* modem,
    int slot,
    const RadioServiceDesc* desc) /* Since 1.7.0 */
{
    return radio_instance_new_internal(dev, name, modem, slot,
        RADIO_INTERFACE_NONE, RADIO_AIDL_INTERFACE_NONE, desc, FALSE);
}

RadioInstance*
radio_instance_new_with_service_desc2(
    const char* dev,
    const char* name,
    const char* modem,
    int slot,
    const RadioServiceDesc2* desc) /* Since 1.7.0 */
{
    return radio_instance_new_internal2(dev, name, modem, slot,
        RADIO_INTERFACE_NONE, RADIO_AIDL_INTERFACE_NONE, desc, FALSE);
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
        char* key = radio_instance_make_desc_key(dev, name,
            radio_instance_find_desc(version, RADIO_AIDL_INTERFACE_NONE));

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
        g_object_ref(THIS(self));
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
        g_object_unref(THIS(self));
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
        RadioInstancePriv* priv = self->priv;
        const guint32 ack = priv->ack_req;

        if (ack) {
            GBinderClient* client = priv->client;

            radio_instance_notify_request_observers(self, ack, NULL);
            if (gbinder_client_transact_sync_oneway(client, ack, NULL) >= 0) {
                return TRUE;
            }
        }
    }
    return FALSE;
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
                    [index], THIS_TYPE, G_SIGNAL_RUN_FIRST | G_SIGNAL_DETAILED,
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
                    [index], THIS_TYPE, G_SIGNAL_RUN_FIRST | G_SIGNAL_DETAILED,
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
                    [index], THIS_TYPE, G_SIGNAL_RUN_FIRST | G_SIGNAL_DETAILED,
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

/*
 * These are defined in radio_util.h but need access to RadioInstance
 * internals:
 */

const char*
radio_req_name2(
    RadioInstance* instance,
    RADIO_REQ req) /* Since 1.6.0 */
{
    if (G_LIKELY(instance)) {
        return g_hash_table_lookup(instance->priv->req_names,
            GUINT_TO_POINTER(req));
    } else {
        /* Backward compatible behavior */
        return radio_find_tx_name(radio_hidl_req_names, req);
    }
}

const char*
radio_resp_name2(
    RadioInstance* instance,
    RADIO_RESP resp) /* Since 1.6.0 */
{
    if (G_LIKELY(instance)) {
        return g_hash_table_lookup(instance->priv->resp_names,
            GUINT_TO_POINTER(resp));
    } else {
        /* Backward compatible behavior */
        return radio_find_tx_name(radio_hidl_resp_names, resp);
    }
}

const char*
radio_ind_name2(
    RadioInstance* instance,
    RADIO_IND ind) /* Since 1.6.0 */
{
    if (G_LIKELY(instance)) {
        return g_hash_table_lookup(instance->priv->ind_names,
            GUINT_TO_POINTER(ind));
    } else {
        /* Backward compatible behavior */
        return radio_find_tx_name(radio_hidl_ind_names, ind);
    }
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
        (self, THIS_TYPE, RadioInstancePriv);

    self->priv = priv;
    priv->idle = gutil_idle_pool_new();
    priv->cleanup = gutil_cleanup_new();
    priv->req_quarks = g_hash_table_new(g_direct_hash, g_direct_equal);
    priv->resp_quarks = g_hash_table_new(g_direct_hash, g_direct_equal);
    priv->ind_quarks = g_hash_table_new(g_direct_hash, g_direct_equal);
    priv->req_names = g_hash_table_new(g_direct_hash, g_direct_equal);
    priv->resp_names = g_hash_table_new(g_direct_hash, g_direct_equal);
    priv->ind_names = g_hash_table_new(g_direct_hash, g_direct_equal);
}

static
void
radio_instance_finalize(
    GObject* object)
{
    RadioInstance* self = THIS(object);
    RadioInstancePriv* priv = self->priv;

    GVERBOSE_("%s", priv->key);
    radio_instance_drop_binder(self);
    gbinder_client_unref(priv->client);
    g_hash_table_destroy(priv->req_quarks);
    g_hash_table_destroy(priv->resp_quarks);
    g_hash_table_destroy(priv->ind_quarks);
    g_hash_table_destroy(priv->req_names);
    g_hash_table_destroy(priv->resp_names);
    g_hash_table_destroy(priv->ind_names);
    gutil_idle_pool_destroy(priv->idle);
    gutil_cleanup_free(priv->cleanup);
    g_free(priv->local);
    g_free(priv->slot);
    g_free(priv->dev);
    g_free(priv->key);
    g_free(priv->modem);
    G_OBJECT_CLASS(PARENT_CLASS)->finalize(object);
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

