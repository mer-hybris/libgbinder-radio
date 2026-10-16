/*
 * Copyright (C) 2026 Jolla Mobile Ltd
 * Copyright (C) 2022 Jolla Ltd.
 * Copyright (C) 2022 Slava Monich <slava.monich@jolla.com>
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
#include "radio_config.h"
#include "radio_instance_p.h"
#include "radio_log.h"
#include "radio_request_p.h"
#include "radio_util_p.h"

#include <gbinder.h>

#include <gutil_misc.h>

/* This API exists since 1.4.6 */

typedef enum radio_config_signal {
    #define SIGNAL_(x) SIGNAL_OBSERVE_REQUEST_##x,
    FOREACH_OBSERVER_PRIORITY(SIGNAL_)
    #undef SIGNAL_
    #define SIGNAL_(x) SIGNAL_OBSERVE_RESPONSE_##x,
    FOREACH_OBSERVER_PRIORITY(SIGNAL_)
    #undef SIGNAL_
    #define SIGNAL_(x) SIGNAL_OBSERVE_INDICATION_##x,
    FOREACH_OBSERVER_PRIORITY(SIGNAL_)
    #undef SIGNAL_
    SIGNAL_COUNT
} RADIO_CONFIG_SIGNAL;

typedef struct radio_config_signal_forward {
    RadioConfig* self;
    RADIO_CONFIG_SIGNAL sig;
    gulong id;
} RadioConfigSignalForward;

struct radio_config {
    RadioBase base;
    RadioConfigSignalForward req_fwd[RADIO_OBSERVER_PRIORITY_COUNT];
    RadioConfigSignalForward resp_fwd[RADIO_OBSERVER_PRIORITY_COUNT];
    RadioConfigSignalForward ind_fwd[RADIO_OBSERVER_PRIORITY_COUNT];
};

typedef RadioBaseClass RadioConfigClass;
GType radio_config_get_type() RADIO_INTERNAL;
G_DEFINE_TYPE(RadioConfig, radio_config, RADIO_TYPE_BASE)

#define PARENT_CLASS radio_config_parent_class
#define THIS_TYPE radio_config_get_type()
#define THIS(obj) G_TYPE_CHECK_INSTANCE_CAST(obj, THIS_TYPE, RadioConfig)

static const char* radio_config_signal_observe_request_name[] = {
    #define SIGNAL_(x) "radio-config-observe-request-" #x,
    FOREACH_OBSERVER_PRIORITY(SIGNAL_)
    #undef SIGNAL_
    NULL
};

static const char* radio_config_signal_observe_response_name[] = {
    #define SIGNAL_(x) "radio-config-observe-response-" #x,
    FOREACH_OBSERVER_PRIORITY(SIGNAL_)
    #undef SIGNAL_
    NULL
};

static const char* radio_config_signal_observe_indication_name[] = {
    #define SIGNAL_(x) "radio-config-observe-indication-" #x,
    FOREACH_OBSERVER_PRIORITY(SIGNAL_)
    #undef SIGNAL_
    NULL
};

static guint radio_config_signals[SIGNAL_COUNT] = { 0 };

/*
 * HIDL interface. Note that there is no @1.2::IRadioConfig but there
 * are @1.2::IRadioConfigResponse and #1.2::IRadioConfigIndication
 */
#define radio_config_hidl_iface_info_1_0 (radio_config_hidl_iface_info_1_2 + 1)
#define radio_config_hidl_iface_info_1_1 radio_config_hidl_iface_info_1_2
static const RadioClientIfaceInfo radio_config_hidl_iface_info_1_2[] = {
    { RADIO_CONFIG_1_1, RADIO_CONFIG_1_1_REQ_LAST },
    { RADIO_CONFIG_1_0, RADIO_CONFIG_1_0_REQ_LAST },
    RADIO_IFACE_INFO_END
};

/* Known RADIO_INTERFACE versions in the opposite order: */
#define RADIO_CONFIG_INTERFACE_VERSIONS(v) v(1_2) v(1_1) v(1_0)
#define RADIO_CONFIG_INTERFACE_INDEX_(x) (RADIO_CONFIG_INTERFACE_COUNT - x - 1)
#define RADIO_CONFIG_INTERFACE_INDEX(v) \
    RADIO_CONFIG_INTERFACE_INDEX_(RADIO_CONFIG_INTERFACE_##v)

static const RADIO_CONFIG_INTERFACE radio_config_hidl_interface[] = {
    #define IFACE(v) RADIO_CONFIG_INTERFACE_##v,
    RADIO_CONFIG_INTERFACE_VERSIONS(IFACE)
    #undef IFACE
};

static const char* const radio_config_hidl_response_ifaces[] = {
    #define IFACE(v) RADIO_CONFIG_RESPONSE_##v,
    RADIO_CONFIG_INTERFACE_VERSIONS(IFACE)
    #undef IFACE
    NULL
};

static const char* const radio_config_hidl_indication_ifaces[] = {
    #define IFACE(v) RADIO_CONFIG_INDICATION_##v,
    RADIO_CONFIG_INTERFACE_VERSIONS(IFACE)
    #undef IFACE
    NULL
};

static const RadioTxName radio_config_hidl_req_names[] = {
    #define REQ(req,Name,NAME) { req, #Name },
    RADIO_CONFIG_REQ_1_0(REQ)
    #undef REQ
    #define REQ(req,resp,Name,NAME) { req, #Name },
    RADIO_CONFIG_CALL_1_0(REQ)
    RADIO_CONFIG_CALL_1_1(REQ)
    #undef REQ
    RADIO_TX_NAMES_END
};

static const RadioTxName radio_config_hidl_resp_names[] = {
    #define RESP(req,resp,Name,NAME) { resp, #Name "Response" },
    RADIO_CONFIG_CALL_1_0(RESP)
    RADIO_CONFIG_CALL_1_1(RESP)
    #undef RESP
    #define RESP(resp,Name,NAME) { resp, #Name },
    RADIO_CONFIG_RESP_1_2(RESP)
    #undef RESP
    RADIO_TX_NAMES_END
};

static const RadioTxName radio_config_hidl_ind_names[] = {
    #define IND(code,Name,NAME) { code, #Name },
    RADIO_CONFIG_IND_1_0(IND)
    /* 1.1 defines no new indications */
    RADIO_CONFIG_IND_1_2(IND)
    #undef IND
    RADIO_TX_NAMES_END
};

static const RadioInterfaceDesc radio_config_hidl_resp_fn[] = {
    #define FN(v) { \
        radio_config_hidl_response_ifaces + RADIO_CONFIG_INTERFACE_INDEX(v),\
        radio_config_hidl_resp_names },
    RADIO_CONFIG_INTERFACE_VERSIONS(FN)
    #undef FN
};

static const RadioInterfaceDesc radio_config_hidl_ind_fn[] = {
    #define FN(v) { \
        radio_config_hidl_indication_ifaces + RADIO_CONFIG_INTERFACE_INDEX(v), \
        radio_config_hidl_ind_names },
    RADIO_CONFIG_INTERFACE_VERSIONS(FN)
    #undef FN
};

static const RadioServiceDesc radio_config_hidl_desc[] = {
    #define DESC(v) { \
        RADIO_INTERFACE_TYPE_HIDL, \
        RADIO_CONFIG_##v, \
        radio_config_hidl_req_names, \
        radio_config_hidl_iface_info_##v, \
        radio_config_hidl_resp_fn + RADIO_CONFIG_INTERFACE_INDEX(v), \
        radio_config_hidl_ind_fn + RADIO_CONFIG_INTERFACE_INDEX(v), \
        RADIO_CONFIG_REQ_SET_RESPONSE_FUNCTIONS },
    RADIO_CONFIG_INTERFACE_VERSIONS(DESC)
    #undef DESC
};

/* AIDL interface */

static const RadioClientIfaceInfo radio_config_aidl_iface_info[] = {
    { RADIO_CONFIG_AIDL, RADIO_CONFIG_1_1_REQ_LAST },
    RADIO_IFACE_INFO_END
};

static const char* const radio_config_aidl_response_ifaces[] = {
    RADIO_CONFIG_AIDL_RESPONSE,
    NULL
};

static const char* const radio_config_aidl_indication_ifaces[] = {
    RADIO_CONFIG_AIDL_INDICATION,
    NULL
};

static const RadioTxName radio_config_aidl_req_names[] = {
    #define REQ(req,Name,NAME) { req, #Name },
    RADIO_CONFIG_AIDL_REQ_1(REQ)
    #undef REQ
    #define REQ(req,resp,Name,NAME) { req, #Name },
    RADIO_CONFIG_AIDL_CALL_1(REQ)
    #undef REQ
    RADIO_TX_NAMES_END
};

static const RadioTxName radio_config_aidl_resp_names[] = {
    #define RESP(req,resp,Name,NAME) { resp, #Name "Response" },
    RADIO_CONFIG_AIDL_CALL_1(RESP)
    #undef RESP
    RADIO_TX_NAMES_END
};

static const RadioTxName radio_config_aidl_ind_names[] = {
    #define IND(code,Name,NAME) { code, #Name },
    RADIO_CONFIG_AIDL_IND_1(IND)
    #undef IND
    RADIO_TX_NAMES_END
};

static const RadioInterfaceDesc radio_config_aidl_resp_fn = {
    radio_config_aidl_response_ifaces,
    radio_config_aidl_resp_names
};

static const RadioInterfaceDesc radio_config_aidl_ind_fn = {
    radio_config_aidl_indication_ifaces,
    radio_config_aidl_ind_names
};

static const RadioServiceDesc radio_config_aidl_desc = {
    RADIO_INTERFACE_TYPE_AIDL,
    RADIO_CONFIG_AIDL,
    radio_config_aidl_req_names,
    radio_config_aidl_iface_info,
    &radio_config_aidl_resp_fn,
    &radio_config_aidl_ind_fn,
    RADIO_CONFIG_AIDL_REQ_SET_RESPONSE_FUNCTIONS
};

/*==========================================================================*
 * Implementation
 *==========================================================================*/

static
void
radio_config_request_observer(
    RadioInstance* inst,
    RADIO_REQ code,
    GBinderLocalRequest* args,
    gpointer user_data)
{
    RadioConfigSignalForward* fwd = user_data;
    RadioConfig* self = fwd->self;

    g_signal_emit(self, radio_config_signals[fwd->sig],
        radio_instance_req_quark(inst, code), code, args);
}

static
void
radio_config_response_observer(
    RadioInstance* inst,
    RADIO_RESP code,
    const RadioResponseInfo* info,
    const GBinderReader* args,
    gpointer user_data)
{
    RadioConfigSignalForward* fwd = user_data;
    RadioConfig* self = fwd->self;

    g_signal_emit(self, radio_config_signals[fwd->sig],
        radio_instance_resp_quark(inst, code), code, info, args);
}

static
void
radio_config_indication_observer(
    RadioInstance* inst,
    RADIO_IND code,
    RADIO_IND_TYPE type,
    const GBinderReader* args,
    gpointer user_data)
{
    RadioConfigSignalForward* fwd = user_data;
    RadioConfig* self = fwd->self;

    g_signal_emit(self, radio_config_signals[fwd->sig],
        radio_instance_ind_quark(inst, code), code, args);
}

/*==========================================================================*
 * API
 *==========================================================================*/

RadioConfig*
radio_config_new(
    void)
{
    return radio_config_new_with_version(RADIO_CONFIG_INTERFACE_MAX);
}

RadioConfig*
radio_config_new_with_version(
    RADIO_CONFIG_INTERFACE max_version)
{
    return radio_config_new_with_version_and_interface_type(max_version,
        RADIO_INTERFACE_TYPE_HIDL);
}

RadioConfig*
radio_config_new_with_version_and_interface_type(
    RADIO_CONFIG_INTERFACE max_version,
    RADIO_INTERFACE_TYPE interface_type)
{
    const char* dev;
    const RadioServiceDesc* desc;
    const RADIO_CONFIG_INTERFACE* radio_iface_id;
    GWeakRef* cache;
    RadioInstance* inst = NULL;
    guint i, n_desc = 0;

    if (interface_type == RADIO_INTERFACE_TYPE_HIDL) {
        /* Cached instances */
        static GWeakRef hidl_cache[RADIO_CONFIG_INTERFACE_COUNT];
        /* Validate the requested version to avoid out-of-bounds access */
        const guint offset = (max_version > RADIO_CONFIG_INTERFACE_MAX) ? 0 :
            (max_version < RADIO_CONFIG_INTERFACE_1_0) ?
            (RADIO_CONFIG_INTERFACE_COUNT - 1) :
            (RADIO_CONFIG_INTERFACE_COUNT - max_version - 1);

        dev = GBINDER_DEFAULT_HWBINDER;
        desc = radio_config_hidl_desc + offset;
        radio_iface_id = radio_config_hidl_interface + offset;
        cache = hidl_cache + offset;
        n_desc = RADIO_CONFIG_INTERFACE_COUNT - offset;
    } else {
        /* Only one AIDL interface is supported so far */
        static GWeakRef aidl_cache;
        static const RADIO_CONFIG_INTERFACE none = RADIO_CONFIG_INTERFACE_NONE;

        dev = GBINDER_DEFAULT_BINDER;
        desc = &radio_config_aidl_desc;
        radio_iface_id = &none;
        cache = &aidl_cache;
        n_desc = 1;
    }

    for (i = 0; i < n_desc; i++) {
        GWeakRef* ref = cache + i;
        RadioConfig* obj = g_weak_ref_get(ref);

        if (obj) {
            if (obj->base.instance->dead) {
                /* Somebody is holding a referent to the dead object */
                g_weak_ref_set(ref, NULL);
            } else {
                /* The requested object already exists and it's still alive */
                return obj;
            }
        }

        inst = radio_instance_new_internal(dev, RADIO_CONFIG_INSTANCE,
                NULL, -1, radio_iface_id[i], RADIO_AIDL_INTERFACE_NONE,
                desc + i, TRUE);
        if (inst) {
            /* Create the new one */
            obj = g_object_new(THIS_TYPE, NULL);
            radio_base_initialize(&obj->base, inst);
            g_weak_ref_set(ref, obj);
            radio_instance_unref(inst);
            return obj;
        }
    }

    return NULL;
}

RadioConfig*
radio_config_ref(
    RadioConfig* self)
{
    if (G_LIKELY(self)) {
        g_object_ref(self);
    }
    return self;
}

void
radio_config_unref(
    RadioConfig* self)
{
    if (G_LIKELY(self)) {
        g_object_unref(self);
    }
}

gboolean
radio_config_dead(
    RadioConfig* self)
{
    return G_UNLIKELY(!self) || self->base.instance->dead;
}

RADIO_INTERFACE_TYPE
radio_config_interface_type(
    RadioConfig* self)
{
    return G_LIKELY(self) ? self->base.instance->interface_type :
        RADIO_INTERFACE_TYPE_NONE;
}

RADIO_CONFIG_INTERFACE
radio_config_interface(
    RadioConfig* self)
{
    return G_LIKELY(self) ? self->base.instance->version :
        RADIO_CONFIG_INTERFACE_NONE;
}

gsize
radio_config_rpc_header_size(
    RadioConfig* self,
    RADIO_CONFIG_REQ req)
{
    return G_LIKELY(self) ?
        radio_instance_rpc_header_size(self->base.instance, req) : 0;
}

const char*
radio_config_req_name(
    RadioConfig* self,
    RADIO_CONFIG_REQ req)
{
    return G_LIKELY(self) ? radio_req_name2(self->base.instance, req) :
        radio_find_tx_name(radio_config_hidl_req_names, req);
}

const char*
radio_config_resp_name(
    RadioConfig* self,
    RADIO_CONFIG_RESP resp)
{
    return G_LIKELY(self) ? radio_resp_name2(self->base.instance, resp) :
        radio_find_tx_name(radio_config_hidl_resp_names, resp);
}

const char*
radio_config_ind_name(
    RadioConfig* self,
    RADIO_CONFIG_IND ind)
{
    return G_LIKELY(self) ? radio_ind_name2(self->base.instance, ind) :
        radio_find_tx_name(radio_config_hidl_ind_names, ind);
}

gulong
radio_config_add_death_handler(
    RadioConfig* self,
    RadioConfigFunc func,
    gpointer user_data)
{
    return G_LIKELY(self) ? radio_base_add_death_handler(&self->base,
        (RadioBaseFunc) func, user_data) : 0;
}

gulong
radio_config_add_request_observer(
    RadioConfig* self,
    RADIO_CONFIG_REQ code,
    RadioConfigRequestObserverFunc func,
    gpointer user_data)
{
    return radio_config_add_request_observer_with_priority(self,
        RADIO_OBSERVER_PRIORITY_DEFAULT, code, func, user_data);
}

gulong
radio_config_add_request_observer_with_priority(
    RadioConfig* self,
    RADIO_OBSERVER_PRIORITY priority,
    RADIO_CONFIG_REQ code,
    RadioConfigRequestObserverFunc func,
    gpointer user_data)
{
    /*
     * This is not very useful API but it's been available since 1.4.6 and
     * has to be maintained.
     */
    if (G_LIKELY(self) && G_LIKELY(func)) {
        const guint index = radio_observer_priority_index(priority);
        const RADIO_CONFIG_SIGNAL sig = SIGNAL_OBSERVE_REQUEST_0 + index;
        RadioConfigSignalForward* fwd = self->req_fwd + index;
        RadioInstance* inst = self->base.instance;

        /* Register signal on demand */
        if (!radio_config_signals[sig]) {
            radio_config_signals[sig] =
                g_signal_new(radio_config_signal_observe_request_name[index],
                    THIS_TYPE, G_SIGNAL_RUN_FIRST|G_SIGNAL_DETAILED,
                    0, NULL, NULL, NULL, G_TYPE_NONE,
                    2, G_TYPE_UINT, G_TYPE_POINTER);
        }

        /* Register forwarder on demand, too */
        if (!fwd->id) {
            fwd->self = self;
            fwd->sig = sig;
            fwd->id = radio_instance_add_request_observer_with_priority(inst,
                priority, RADIO_REQ_ANY, radio_config_request_observer, fwd);
        }

        return g_signal_connect_closure_by_id(self,
            radio_config_signals[sig],
            radio_instance_req_quark(inst, code),
            g_cclosure_new(G_CALLBACK(func), user_data, NULL), FALSE);
    }
    return 0;
}

gulong
radio_config_add_response_observer(
    RadioConfig* self,
    RADIO_CONFIG_RESP code,
    RadioConfigResponseObserverFunc func,
    gpointer user_data)
{
    return radio_config_add_response_observer_with_priority(self,
        RADIO_OBSERVER_PRIORITY_DEFAULT, code, func, user_data);
}

gulong
radio_config_add_response_observer_with_priority(
    RadioConfig* self,
    RADIO_OBSERVER_PRIORITY priority,
    RADIO_CONFIG_RESP code,
    RadioConfigResponseObserverFunc func,
    gpointer user_data)
{
    /*
     * This is not very useful API but it's been available since 1.4.6 and
     * has to be maintained.
     */
    if (G_LIKELY(self) && G_LIKELY(func)) {
        const guint index = radio_observer_priority_index(priority);
        const RADIO_CONFIG_SIGNAL sig = SIGNAL_OBSERVE_RESPONSE_0 + index;
        RadioConfigSignalForward* fwd = self->resp_fwd + index;
        RadioInstance* inst = self->base.instance;

        /* Register signal on demand */
        if (!radio_config_signals[sig]) {
            radio_config_signals[sig] =
                g_signal_new(radio_config_signal_observe_response_name[index],
                    THIS_TYPE, G_SIGNAL_RUN_FIRST|G_SIGNAL_DETAILED,
                    0, NULL, NULL, NULL, G_TYPE_NONE,
                    3, G_TYPE_UINT, G_TYPE_POINTER, G_TYPE_POINTER);
        }

        /* Register forwarder on demand, too */
        if (!fwd->id) {
            fwd->self = self;
            fwd->sig = sig;
            fwd->id = radio_instance_add_response_observer_with_priority(inst,
                priority, RADIO_RESP_ANY, radio_config_response_observer, fwd);
        }

        return g_signal_connect_closure_by_id(self,
            radio_config_signals[sig],
            radio_instance_resp_quark(inst, code),
            g_cclosure_new(G_CALLBACK(func), user_data, NULL), FALSE);
    }
    return 0;
}

gulong
radio_config_add_indication_observer(
    RadioConfig* self,
    RADIO_CONFIG_IND code,
    RadioConfigIndicationObserverFunc func,
    gpointer user_data)
{
    return radio_config_add_indication_observer_with_priority(self,
        RADIO_OBSERVER_PRIORITY_DEFAULT, code, func, user_data);
}

gulong
radio_config_add_indication_observer_with_priority(
    RadioConfig* self,
    RADIO_OBSERVER_PRIORITY priority,
    RADIO_CONFIG_IND code,
    RadioConfigIndicationObserverFunc func,
    gpointer user_data)
{
    /*
     * This is not very useful API but it's been available since 1.4.6 and
     * has to be maintained.
     */
    if (G_LIKELY(self) && G_LIKELY(func)) {
        const guint index = radio_observer_priority_index(priority);
        const RADIO_CONFIG_SIGNAL sig = SIGNAL_OBSERVE_INDICATION_0 + index;
        RadioConfigSignalForward* fwd = self->ind_fwd + index;
        RadioInstance* inst = self->base.instance;

        /* Register signal on demand */
        if (!radio_config_signals[sig]) {
            radio_config_signals[sig] =
                g_signal_new(radio_config_signal_observe_indication_name[index],
                    THIS_TYPE, G_SIGNAL_RUN_FIRST|G_SIGNAL_DETAILED,
                    0, NULL, NULL, NULL, G_TYPE_NONE,
                    2, G_TYPE_UINT, G_TYPE_POINTER);
        }

        /* Register forwarder on demand, too */
        if (!fwd->id) {
            fwd->self = self;
            fwd->sig = sig;
            fwd->id = radio_instance_add_indication_observer_with_priority(inst,
                priority, RADIO_IND_ANY, radio_config_indication_observer, fwd);
        }

        return g_signal_connect_closure_by_id(self,
            radio_config_signals[sig],
            radio_instance_ind_quark(inst, code),
            g_cclosure_new(G_CALLBACK(func), user_data, NULL), FALSE);
    }
    return 0;
}

void
radio_config_remove_handler(
    RadioConfig* self,
    gulong id)
{
    if (G_LIKELY(self) && G_LIKELY(id)) {
        g_signal_handler_disconnect(self, id);
    }
}

void
radio_config_remove_handlers(
    RadioConfig* self,
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
radio_config_init(
    RadioConfig* self)
{
}

static
void
radio_config_finalize(
    GObject* object)
{
    RadioConfig* self = THIS(object);
    RadioInstance* inst = self->base.instance;
    int i;

    for (i = 0; i < RADIO_OBSERVER_PRIORITY_COUNT; i++) {
        radio_instance_remove_handler(inst, self->req_fwd[i].id);
        radio_instance_remove_handler(inst, self->resp_fwd[i].id);
        radio_instance_remove_handler(inst, self->ind_fwd[i].id);
    }
    G_OBJECT_CLASS(PARENT_CLASS)->finalize(object);
}

static
void
radio_config_class_init(
    RadioConfigClass* klass)
{
    G_OBJECT_CLASS(klass)->finalize = radio_config_finalize;
}

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
