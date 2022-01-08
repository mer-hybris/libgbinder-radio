/*
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
#include "radio_log.h"
#include "radio_request_p.h"
#include "radio_util_p.h"

#include <gbinder.h>

#include <gutil_idlepool.h>
#include <gutil_macros.h>
#include <gutil_misc.h>
#include <gutil_strv.h>

/* This API exists since 1.4.6 */

typedef struct radio_config {
    RadioBase base;
    GUtilIdlePool* pool;
    GBinderClient* client;
    GBinderRemoteObject* remote;
    GBinderLocalObject* response;
    GBinderLocalObject* indication;
    RADIO_CONFIG_INTERFACE version;
    GHashTable* req_quarks;
    GHashTable* resp_quarks;
    GHashTable* ind_quarks;
    gulong death_id;
    gboolean dead;
} RadioConfig;

typedef RadioBaseClass RadioConfigClass;
GType radio_config_get_type() RADIO_INTERNAL;
G_DEFINE_TYPE(RadioConfig, radio_config, RADIO_TYPE_BASE)

#define PARENT_CLASS radio_config_parent_class
#define THIS_TYPE radio_config_get_type()
#define THIS(obj) G_TYPE_CHECK_INSTANCE_CAST(obj, THIS_TYPE, RadioConfig)

typedef enum radio_config_signal {
    #define SIGNAL_INDEX(x) SIGNAL_OBSERVE_REQUEST_##x,
    FOREACH_OBSERVER_PRIORITY(SIGNAL_INDEX)
    #undef SIGNAL_INDEX
    #define SIGNAL_INDEX(x) SIGNAL_OBSERVE_RESPONSE_##x,
    FOREACH_OBSERVER_PRIORITY(SIGNAL_INDEX)
    #undef SIGNAL_INDEX
    #define SIGNAL_INDEX(x) SIGNAL_OBSERVE_INDICATION_##x,
    FOREACH_OBSERVER_PRIORITY(SIGNAL_INDEX)
    #undef SIGNAL_INDEX
    SIGNAL_INDICATION,
    SIGNAL_DEATH,
    SIGNAL_COUNT
} RADIO_CONFIG_SIGNAL;

static const char* radio_config_signal_observe_request_name[] = {
    #define SIGNAL_NAME(x) "radio-config-observe-request-" #x,
    FOREACH_OBSERVER_PRIORITY(SIGNAL_NAME)
    #undef SIGNAL_NAME
    NULL
};

static const char* radio_config_signal_observe_response_name[] = {
    #define SIGNAL_NAME(x) "radio-config-observe-response-" #x,
    FOREACH_OBSERVER_PRIORITY(SIGNAL_NAME)
    #undef SIGNAL_NAME
    NULL
};

static const char* radio_config_signal_observe_indication_name[] = {
    #define SIGNAL_NAME(x) "radio-config-observe-indication-" #x,
    FOREACH_OBSERVER_PRIORITY(SIGNAL_NAME)
    #undef SIGNAL_NAME
    NULL
};

#define SIGNAL_INDICATION_NAME "radio-config-indication"
#define SIGNAL_DEATH_NAME      "radio-config-death"

static guint radio_config_signals[SIGNAL_COUNT] = { 0 };

static const GBinderClientIfaceInfo radio_config_iface_info[] = {
    {RADIO_CONFIG_1_1, RADIO_CONFIG_1_1_REQ_LAST },
    {RADIO_CONFIG_1_0, RADIO_CONFIG_1_0_REQ_LAST }
};
G_STATIC_ASSERT(G_N_ELEMENTS(radio_config_iface_info) ==
    RADIO_CONFIG_INTERFACE_COUNT);

static const char* const radio_config_indication_ifaces[] = {
    RADIO_CONFIG_INDICATION_1_1,
    RADIO_CONFIG_INDICATION_1_0,
    NULL
};
G_STATIC_ASSERT(G_N_ELEMENTS(radio_config_indication_ifaces) ==
    RADIO_CONFIG_INTERFACE_COUNT + 1);

static const char* const radio_config_response_ifaces[] = {
    RADIO_CONFIG_RESPONSE_1_1,
    RADIO_CONFIG_RESPONSE_1_0,
    NULL
};
G_STATIC_ASSERT(G_N_ELEMENTS(radio_config_response_ifaces) ==
    RADIO_CONFIG_INTERFACE_COUNT + 1);

typedef struct radio_config_interface_desc {
    RADIO_CONFIG_INTERFACE version;
    const char* fqname;
    const char* radio_iface;
    const char* const* ind_ifaces;
    const char* const* resp_ifaces;
} RadioConfigInterfaceDesc;

#define RADIO_CONFIG_INTERFACE_INDEX(x) (RADIO_CONFIG_INTERFACE_COUNT - x - 1)

#define RADIO_CONFIG_INTERFACE_DESC(v) \
        RADIO_CONFIG_INTERFACE_##v, \
        RADIO_CONFIG_##v##_FQNAME, \
        RADIO_CONFIG_##v, \
        radio_config_indication_ifaces + \
        RADIO_CONFIG_INTERFACE_INDEX(RADIO_CONFIG_INTERFACE_##v), \
        radio_config_response_ifaces + \
        RADIO_CONFIG_INTERFACE_INDEX(RADIO_CONFIG_INTERFACE_##v)

static const RadioConfigInterfaceDesc radio_config_interfaces[] = {
   { RADIO_CONFIG_INTERFACE_DESC(1_1) },
   { RADIO_CONFIG_INTERFACE_DESC(1_0) }
};
G_STATIC_ASSERT(G_N_ELEMENTS(radio_config_interfaces) ==
    RADIO_CONFIG_INTERFACE_COUNT);

/* Must have a separate instance for each interface version */
static RadioConfig* radio_config_instance[RADIO_CONFIG_INTERFACE_COUNT] =
    { NULL };

typedef struct radio_config_call {
    RadioRequest* req;
    RadioBaseRequestSentFunc callback;
    RadioBase* object;
} RadioConfigCall;

/*==========================================================================*
 * Implementation
 *==========================================================================*/

static
void
radio_config_call_free(
    RadioConfigCall* call)
{
    radio_request_unref(call->req);
    gutil_slice_free(call);
}

static
void
radio_config_call_complete(
    GBinderClient* client,
    GBinderRemoteReply* reply,
    int status,
    void* user_data)
{
    RadioConfigCall* call = user_data;

    call->callback(call->object, call->req, status);
}

static
const char*
radio_config_known_req_name(
    RADIO_CONFIG_REQ req)
{
    switch (req) {
    case RADIO_CONFIG_REQ_SET_RESPONSE_FUNCTIONS: return "setResponseFunctions";
#define RADIO_CONFIG_REQ_(req,resp,Name,NAME) \
    case RADIO_CONFIG_REQ_##NAME: return #Name;
    RADIO_CONFIG_CALL_1_0(RADIO_CONFIG_REQ_)
    RADIO_CONFIG_CALL_1_1(RADIO_CONFIG_REQ_)
#undef RADIO_CONFIG_REQ_
    case RADIO_CONFIG_REQ_ANY:
        break;
    }
    return NULL;
}

static
const char*
radio_config_known_resp_name(
    RADIO_CONFIG_RESP resp)
{
    switch (resp) {
#define RADIO_CONFIG_RESP_(req,resp,Name,NAME) \
    case RADIO_CONFIG_RESP_##NAME: return #Name "Response";
    RADIO_CONFIG_CALL_1_0(RADIO_CONFIG_RESP_)
    RADIO_CONFIG_CALL_1_1(RADIO_CONFIG_RESP_)
#undef RADIO_CONFIG_RESP_
    case RADIO_CONFIG_RESP_ANY:
        break;
    }
    return NULL;
}

static
const char*
radio_config_known_ind_name(
    RADIO_CONFIG_IND ind)
{
    switch (ind) {
#define RADIO_CONFIG_IND_(code,Name,NAME) \
    case RADIO_CONFIG_IND_##NAME: return #Name;
    RADIO_CONFIG_IND_1_0(RADIO_CONFIG_IND_)
    /* 1.1 defines no new indications */
#undef RADIO_CONFIG_IND_
    case RADIO_CONFIG_IND_ANY:
        break;
    }
    return NULL;
}

static
GQuark
radio_config_req_quark(
    RadioConfig* self,
    RADIO_CONFIG_REQ req)
{
    GQuark q = 0;

    if (req != RADIO_CONFIG_REQ_ANY) {
        gpointer key = GUINT_TO_POINTER(req);

        q = GPOINTER_TO_UINT(g_hash_table_lookup(self->req_quarks, key));
        if (!q) {
            const char* known = radio_config_known_req_name(req);

            if (known) {
                q = g_quark_from_static_string(known);
            } else {
                q = g_quark_from_string(radio_config_req_name(self, req));
            }
            g_hash_table_insert(self->req_quarks, key, GUINT_TO_POINTER(q));
        }
    }
    return q;
}

static
GQuark
radio_config_resp_quark(
    RadioConfig* self,
    RADIO_CONFIG_RESP resp)
{
    GQuark q = 0;

    if (resp != RADIO_CONFIG_RESP_ANY) {
        gpointer key = GUINT_TO_POINTER(resp);

        q = GPOINTER_TO_UINT(g_hash_table_lookup(self->resp_quarks, key));
        if (!q) {
            const char* known = radio_config_known_resp_name(resp);

            if (known) {
                q = g_quark_from_static_string(known);
            } else {
                q = g_quark_from_string(radio_config_resp_name(self, resp));
            }
            g_hash_table_insert(self->resp_quarks, key, GUINT_TO_POINTER(q));
        }
    }
    return q;
}

static
GQuark
radio_config_ind_quark(
    RadioConfig* self,
    RADIO_CONFIG_IND ind)
{
    GQuark q = 0;

    if (ind != RADIO_CONFIG_IND_ANY) {
        gpointer key = GUINT_TO_POINTER(ind);

        q = GPOINTER_TO_UINT(g_hash_table_lookup(self->ind_quarks, key));
        if (!q) {
            const char* known = radio_config_known_ind_name(ind);

            if (known) {
                q = g_quark_from_static_string(known);
            } else {
                q = g_quark_from_string(radio_config_ind_name(self, ind));
            }
            g_hash_table_insert(self->ind_quarks, key, GUINT_TO_POINTER(q));
        }
    }
    return q;
}

static
GBinderLocalReply*
radio_config_indication(
    GBinderLocalObject* obj,
    GBinderRemoteRequest* req,
    guint code,
    guint flags,
    int* status,
    void* user_data)
{
    RadioConfig* self = THIS(user_data);
    const char* iface = gbinder_remote_request_interface(req);

    if (gutil_strv_contains((GStrV*)radio_config_indication_ifaces, iface)) {
        GBinderReader args;
        guint type;

        /*
         * All these should be one-way and have RADIO_IND_UNSOLICITED type
         * as the first parameter. RADIO_IND_ACK_EXP is not expected because
         * IRadioConfig doesn't have responseAcknowledgement
         */
        GASSERT(flags & GBINDER_TX_FLAG_ONEWAY);
        gbinder_remote_request_init_reader(req, &args);
        if (gbinder_reader_read_uint32(&args, &type) &&
            type == RADIO_IND_UNSOLICITED) {
            int i;
            const GQuark quark = radio_config_ind_quark(self, code);
            const guint* signals = radio_config_signals +
                SIGNAL_OBSERVE_INDICATION_0;

            for (i = RADIO_OBSERVER_PRIORITY_COUNT - 1; i >=0; i--) {
                if (signals[i]) {
                    g_signal_emit(self, signals[i], quark, code, &args);
                }
            }
            *status = GBINDER_STATUS_OK;
        } else {
            GWARN("Failed to decode IRadioConfig indication %u", code);
            *status = GBINDER_STATUS_FAILED;
        }
    }
    return NULL;
}

static
GBinderLocalReply*
radio_config_response(
    GBinderLocalObject* obj,
    GBinderRemoteRequest* req,
    guint code,
    guint flags,
    int* status,
    void* user_data)
{
    RadioConfig* self = THIS(user_data);
    const char* iface = gbinder_remote_request_interface(req);

    if (gutil_strv_contains((GStrV*)radio_config_response_ifaces, iface)) {
        GBinderReader args;
        const RadioResponseInfo* info;

        /* All responses must be one-way and have RadioResponseInfo */
        gbinder_remote_request_init_reader(req, &args);
        info = gbinder_reader_read_hidl_struct(&args, RadioResponseInfo);
        GASSERT(flags & GBINDER_TX_FLAG_ONEWAY);
        if (info) {
            int p = RADIO_OBSERVER_PRIORITY_HIGHEST;
            const GQuark quark = radio_config_resp_quark(self, code);
            const guint* signals = radio_config_signals +
                SIGNAL_OBSERVE_RESPONSE_0;

            radio_config_ref(self);

            /* High-priority observers get notified first */
            for (; p > RADIO_OBSERVER_PRIORITY_DEFAULT; p--) {
                const int i = RADIO_OBSERVER_PRIORITY_INDEX(p);

                if (signals[i]) {
                    g_signal_emit(self, signals[i], quark, code, info, &args);
                }
            }

            /* Then the response is actually processed */
            if (!radio_base_handle_resp(&self->base, code, info, &args)) {
                const char* name = radio_config_known_resp_name(code);

                /* Most likely this is a response to a cancelled request */
                GDEBUG("Ignoring IRadioConfig response [%08x] %u %s",
                    info->serial, code, name ? name : "");
            }

            /* Followed by the remaining observers in their priority order */
            for (; p >= RADIO_OBSERVER_PRIORITY_LOWEST; p--) {
                const int i = RADIO_OBSERVER_PRIORITY_INDEX(p);

                if (signals[i]) {
                    g_signal_emit(self, signals[i], quark, code, info, &args);
                }
            }
            radio_config_unref(self);
            *status = GBINDER_STATUS_OK;
        }
    }
    return NULL;
}

static
void
radio_config_drop_binder(
    RadioConfig* self)
{
    if (self->indication) {
        gbinder_local_object_drop(self->indication);
        self->indication = NULL;
    }
    if (self->response) {
        gbinder_local_object_drop(self->response);
        self->response = NULL;
    }
    if (self->remote) {
        gbinder_remote_object_remove_handler(self->remote, self->death_id);
        gbinder_remote_object_unref(self->remote);
        self->death_id = 0;
        self->remote = NULL;
    }
}

static
void
radio_config_died(
    GBinderRemoteObject* obj,
    void* user_data)
{
    RadioConfig* self = THIS(user_data);

    GWARN("IRadioConfig died");
    self->dead = TRUE;
    radio_config_ref(self);
    radio_config_drop_binder(self);
    if (radio_config_instance[self->version] == self) {
        radio_config_instance[self->version] = NULL; /* Clear the cache */
    }
    g_signal_emit(self, radio_config_signals[SIGNAL_DEATH], 0);
    radio_base_handle_death(&self->base);
    radio_config_unref(self);
}

static
void
radio_config_gone(
    gpointer user_data,
    GObject* dead)
{
    GObject** shared_instance = user_data;

    if (*shared_instance == dead) {
        *shared_instance = NULL;
        GVERBOSE_("IRadioConfig gone");
    }
}

static
RadioConfig*
radio_config_create(
    GBinderServiceManager* sm,
    GBinderRemoteObject* remote,
    const RadioConfigInterfaceDesc* desc)
{
    RadioConfig* self = g_object_new(THIS_TYPE, NULL);
    GBinderLocalRequest* req;
    GBinderWriter writer;
    int status;

    radio_base_initialize(&self->base);
    self->version = desc->version;
    self->remote = gbinder_remote_object_ref(remote);
    self->client = gbinder_client_new2(remote, radio_config_iface_info,
        G_N_ELEMENTS(radio_config_iface_info));
    self->indication = gbinder_servicemanager_new_local_object2(sm,
        desc->ind_ifaces, radio_config_indication, self);
    self->response = gbinder_servicemanager_new_local_object2(sm,
        desc->resp_ifaces, radio_config_response, self);
    self->death_id = gbinder_remote_object_add_death_handler(remote,
        radio_config_died, self);

    /* IRadioConfig::setResponseFunctions */
    req = gbinder_client_new_request2(self->client,
        RADIO_CONFIG_REQ_SET_RESPONSE_FUNCTIONS);
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_local_object(&writer, self->response);
    gbinder_writer_append_local_object(&writer, self->indication);
    gbinder_remote_reply_unref(gbinder_client_transact_sync_reply(self->client,
        RADIO_CONFIG_REQ_SET_RESPONSE_FUNCTIONS, req, &status));
    GVERBOSE_("IRadioConfig::setResponseFunctions status %d", status);
    gbinder_local_request_unref(req);
    return self;
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
    /* Validate the requested version to avoid out-of-bounds access */
    if (max_version < RADIO_CONFIG_INTERFACE_1_0) {
        max_version = RADIO_CONFIG_INTERFACE_1_0;
    } else if (max_version > RADIO_CONFIG_INTERFACE_MAX) {
        max_version = RADIO_CONFIG_INTERFACE_MAX;
    }

    if (radio_config_instance[max_version]) {
        /* The requested instance already exists */
        return radio_config_ref(radio_config_instance[max_version]);
    } else {
        /* Assume /dev/hwbinder */
        GBinderServiceManager* sm =
            gbinder_servicemanager_new(GBINDER_DEFAULT_HWBINDER);

        if (sm) {
            guint i;
            GBinderRemoteObject* obj = NULL; /* autoreleased */
            const RadioConfigInterfaceDesc* desc;
            RadioConfig* config = NULL;

            /* Find maximum available version not exceeding the requested one */
            for (i=0; i<G_N_ELEMENTS(radio_config_interfaces) && !obj; i++) {
                desc = radio_config_interfaces + i;
                if (desc->version <= max_version) {
                    obj = gbinder_servicemanager_get_service_sync(sm,
                        desc->fqname, NULL);
                    if (obj) {
                        /*
                         * desc->version isn't necessarily equal to
                         * max_version
                         */
                        if (radio_config_instance[desc->version]) {
                            config = radio_config_ref(radio_config_instance
                                [desc->version]);
                        } else {
                            GINFO("Connected to %s", desc->fqname);
                            config = radio_config_create(sm, obj, desc);
                        }
                        break;
                    }
                }
            }

            gbinder_servicemanager_unref(sm);
            if (config) {
                radio_config_instance[desc->version] = config;
                g_object_weak_ref(G_OBJECT(config), radio_config_gone,
                    radio_config_instance + desc->version);
                return config;
            }
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
    return G_UNLIKELY(!self) || self->dead;
}

RADIO_CONFIG_INTERFACE
radio_config_interface(
    RadioConfig* self)
{
    return G_LIKELY(self) ? self->version : RADIO_CONFIG_INTERFACE_NONE;
}

gsize
radio_config_rpc_header_size(
    RadioConfig* self,
    RADIO_CONFIG_REQ req)
{
    if (G_LIKELY(self)) {
        GBytes* header = gbinder_client_rpc_header(self->client, req);

        if (header) {
            return g_bytes_get_size(header);
        }
    }
    return 0;
}

const char*
radio_config_req_name(
    RadioConfig* self,
    RADIO_CONFIG_REQ req)
{
    const char* known = radio_config_known_req_name(req);

    if (known) {
        return known;
    } else if (G_LIKELY(self)) {
        char* str = g_strdup_printf("%u", (guint)req);

        gutil_idle_pool_add(self->pool, str, g_free);
        return str;
    } else {
        return NULL;
    }
}

const char*
radio_config_resp_name(
    RadioConfig* self,
    RADIO_CONFIG_RESP resp)
{
    const char* known = radio_config_known_resp_name(resp);

    if (known) {
        return known;
    } else if (G_LIKELY(self)) {
        char* str = g_strdup_printf("%u", (guint)resp);

        gutil_idle_pool_add(self->pool, str, g_free);
        return str;
    } else {
        return NULL;
    }
}

const char*
radio_config_ind_name(
    RadioConfig* self,
    RADIO_CONFIG_IND ind)
{
    const char* known = radio_config_known_ind_name(ind);

    if (known) {
        return known;
    } else if (G_LIKELY(self)) {
        char* str = g_strdup_printf("%u", (guint)ind);

        gutil_idle_pool_add(self->pool, str, g_free);
        return str;
    } else {
        return NULL;
    }
}

gulong
radio_config_add_death_handler(
    RadioConfig* self,
    RadioConfigFunc fn,
    gpointer user_data)
{
    return (G_LIKELY(self) && G_LIKELY(fn)) ?
        g_signal_connect_closure_by_id(self,
             radio_config_signals[SIGNAL_DEATH], 0,
             g_cclosure_new(G_CALLBACK(fn), user_data, NULL), FALSE) : 0;
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
    if (G_LIKELY(self) && G_LIKELY(func)) {
        const guint index = radio_observer_priority_index(priority);
        const RADIO_CONFIG_SIGNAL sig = SIGNAL_OBSERVE_REQUEST_0 + index;

        /* Register signal on demand */
        if (!radio_config_signals[sig]) {
            radio_config_signals[sig] =
                g_signal_new(radio_config_signal_observe_request_name[index],
                    THIS_TYPE, G_SIGNAL_RUN_FIRST | G_SIGNAL_DETAILED,
                    0, NULL, NULL, NULL, G_TYPE_NONE,
                    2, G_TYPE_UINT, G_TYPE_POINTER);
        }

        return g_signal_connect_closure_by_id(self,
            radio_config_signals[sig],
            radio_config_req_quark(self, code),
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
    if (G_LIKELY(self) && G_LIKELY(func)) {
        const guint index = radio_observer_priority_index(priority);
        const RADIO_CONFIG_SIGNAL sig = SIGNAL_OBSERVE_RESPONSE_0 + index;

        /* Register signal on demand */
        if (!radio_config_signals[sig]) {
            radio_config_signals[sig] =
                g_signal_new(radio_config_signal_observe_response_name[index],
                    THIS_TYPE, G_SIGNAL_RUN_FIRST | G_SIGNAL_DETAILED,
                    0, NULL, NULL, NULL, G_TYPE_NONE,
                    3, G_TYPE_UINT, G_TYPE_POINTER, G_TYPE_POINTER);
        }

        return g_signal_connect_closure_by_id(self,
            radio_config_signals[sig],
            radio_config_resp_quark(self, code),
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
    gpointer user_data) /* Since 1.4.3 */
{
    if (G_LIKELY(self) && G_LIKELY(func)) {
        const guint pi = radio_observer_priority_index(priority);
        const RADIO_CONFIG_SIGNAL sig = SIGNAL_OBSERVE_INDICATION_0 + pi;

        /* Register signal on demand */
        if (!radio_config_signals[sig]) {
            radio_config_signals[sig] =
                g_signal_new(radio_config_signal_observe_indication_name[pi],
                    THIS_TYPE, G_SIGNAL_RUN_FIRST | G_SIGNAL_DETAILED,
                    0, NULL, NULL, NULL, G_TYPE_NONE,
                    2, G_TYPE_UINT, G_TYPE_POINTER);
        }

        return g_signal_connect_closure_by_id(self,
            radio_config_signals[sig],
            radio_config_ind_quark(self, code),
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
 * Methods
 *==========================================================================*/

static
gboolean
radio_config_is_dead(
    RadioBase* base)
{
    return THIS(base)->dead;
}

static
gboolean
radio_config_can_submit_requests(
    RadioBase* base)
{
    return !radio_config_is_dead(base);
}

static
GBinderLocalRequest*
radio_config_new_request(
    RadioBase* base,
    guint32 code)
{
    return gbinder_client_new_request2(THIS(base)->client, code);
}

static
gulong
radio_config_send_request(
    RadioBase* base,
    RadioRequest* req,
    RadioBaseRequestSentFunc callback)
{
    RadioConfig* self = THIS(base);
    RadioConfigCall* call = g_slice_new(RadioConfigCall);
    GQuark quark = 0;
    gulong tx_id;
    int i;

    call->object = base;
    call->callback = callback;
    call->req = radio_request_ref(req);

    /* Notify the observers first */
    for (i = RADIO_OBSERVER_PRIORITY_COUNT - 1; i >= 0; i--) {
        guint id = radio_config_signals[SIGNAL_OBSERVE_REQUEST_0 + i];

        if (id) {
            if (!quark) {
                quark = radio_config_req_quark(self, req->code);
            }
            g_signal_emit(self, id, quark, req->code, req->args);
        }
    }

    /* Then actually submit the request */
    tx_id = gbinder_client_transact(self->client, req->code,
        GBINDER_TX_FLAG_ONEWAY, req->args, radio_config_call_complete,
        (GDestroyNotify) radio_config_call_free, call);
    if (tx_id) {
        return tx_id;
    } else {
        radio_config_call_free(call);
        return 0;
    }
}

static
void
radio_config_cancel_request(
    RadioBase* base,
    gulong id)
{
    gbinder_client_cancel(THIS(base)->client, id);
}

/*==========================================================================*
 * Internals
 *==========================================================================*/

static
void
radio_config_init(
    RadioConfig* self)
{
    self->version = RADIO_CONFIG_INTERFACE_NONE;
    self->pool = gutil_idle_pool_new();
    self->req_quarks = g_hash_table_new(g_direct_hash, g_direct_equal);
    self->resp_quarks = g_hash_table_new(g_direct_hash, g_direct_equal);
    self->ind_quarks = g_hash_table_new(g_direct_hash, g_direct_equal);
}

static
void
radio_config_finalize(
    GObject* object)
{
    RadioConfig* self = THIS(object);

    radio_config_drop_binder(self);
    gbinder_client_unref(self->client);
    gutil_idle_pool_destroy(self->pool);
    g_hash_table_destroy(self->req_quarks);
    g_hash_table_destroy(self->resp_quarks);
    g_hash_table_destroy(self->ind_quarks);
    G_OBJECT_CLASS(PARENT_CLASS)->finalize(object);
}

static
void
radio_config_class_init(
    RadioConfigClass* klass)
{
    RadioBaseClass* base_class = RADIO_BASE_CLASS(klass);
    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    GType type = G_OBJECT_CLASS_TYPE(klass);

    base_class->is_dead = radio_config_is_dead;
    base_class->can_submit_requests = radio_config_can_submit_requests;
    base_class->new_request = radio_config_new_request;
    base_class->send_request = radio_config_send_request;
    base_class->cancel_request = radio_config_cancel_request;
    object_class->finalize = radio_config_finalize;

    /* Priority-based signals are registered on demand */
    radio_config_signals[SIGNAL_INDICATION] =
        g_signal_new(SIGNAL_INDICATION_NAME, type,
            G_SIGNAL_RUN_FIRST | G_SIGNAL_DETAILED, 0, NULL, NULL, NULL,
            G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_POINTER);
    radio_config_signals[SIGNAL_DEATH] =
        g_signal_new(SIGNAL_DEATH_NAME, type, G_SIGNAL_RUN_FIRST, 0,
            NULL, NULL, NULL, G_TYPE_NONE, 0);
}

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
