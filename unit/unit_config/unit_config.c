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

#include "test_common.h"
#include "test_gbinder.h"

#include "radio_config.h"
#include "radio_request_p.h"

#include <gutil_strv.h>
#include <gutil_log.h>

#define DEV GBINDER_DEFAULT_HWBINDER

static TestOpt test_opt;

static const GBinderClientIfaceInfo radio_config_ind_iface_info[] = {
    {RADIO_CONFIG_INDICATION_1_0, RADIO_CONFIG_1_0_IND_LAST }
};

static const GBinderClientIfaceInfo radio_config_resp_iface_info[] = {
    {RADIO_CONFIG_RESPONSE_1_1, RADIO_CONFIG_1_1_RESP_LAST },
    {RADIO_CONFIG_RESPONSE_1_0, RADIO_CONFIG_1_0_RESP_LAST }
};

static const char* const radio_config_req_ifaces[] = {
    RADIO_CONFIG_1_1,
    RADIO_CONFIG_1_0,
    NULL
};

static const char* const radio_config_fqnames[] = {
    RADIO_CONFIG_1_0_FQNAME,
    RADIO_CONFIG_1_1_FQNAME
};

static
void
test_complete_not_reached(
    RadioRequest* req,
    RADIO_TX_STATUS status,
    RADIO_CONFIG_RESP resp,
    RADIO_ERROR error,
    const GBinderReader* reader,
    gpointer user_data)
{
    g_assert_not_reached();
}

static
void
test_destroy_once(
    gpointer user_data)
{
    gboolean* destroyed = user_data;

    g_assert(!*destroyed);
    *destroyed = TRUE;
}

static
void
test_ind_not_reached(
    RadioConfig* config,
    RADIO_CONFIG_IND code,
    const GBinderReader* args,
    gpointer user_data)
{
    g_assert_not_reached();
}

static
void
test_inc_cb(
    gpointer user_data)
{
    (*((int*)user_data))++;
}

static
void
test_config_inc_cb(
    RadioConfig* config,
    gpointer user_data)
{
    (*((int*)user_data))++;
}

static
RADIO_CONFIG_RESP
test_config_req_resp(
    RADIO_CONFIG_REQ req)
{
    switch (req) {
#define REQ_RESP_(req,resp,Name,NAME) \
    case RADIO_CONFIG_REQ_##NAME: return RADIO_CONFIG_RESP_##NAME;
    RADIO_CONFIG_CALL_1_0(REQ_RESP_)
    RADIO_CONFIG_CALL_1_1(REQ_RESP_)
#undef REQ_RESP_
    case RADIO_CONFIG_REQ_SET_RESPONSE_FUNCTIONS:
        return RADIO_CONFIG_RESP_NONE;
    case RADIO_CONFIG_REQ_ANY:
        break;
    }
    g_assert_not_reached();
    return RADIO_CONFIG_RESP_NONE;
}

/*==========================================================================*
 * Test IRadioConfig service
 *==========================================================================*/

typedef struct test_config_service {
    GBinderLocalObject* obj;
    GBinderClient* resp_client;
    GBinderClient* ind_client;
    GHashTable* req_count;
} TestConfigService;

#define FAIL_REQ RADIO_CONFIG_REQ_GET_PHONE_CAPABILITY
#define ERROR_REQ RADIO_CONFIG_REQ_SET_SIM_SLOTS_MAPPING
#define ERROR_RESP RADIO_CONFIG_RESP_SET_SIM_SLOTS_MAPPING
#define IGNORE_REQ RADIO_CONFIG_REQ_SET_MODEMS_CONFIG

static
int
test_config_service_req_count(
    TestConfigService* service,
    RADIO_CONFIG_REQ req)
{
    return GPOINTER_TO_INT(g_hash_table_lookup(service->req_count,
        GINT_TO_POINTER(req)));
}

static
GBinderLocalReply*
test_config_service_txproc(
    GBinderLocalObject* obj,
    GBinderRemoteRequest* req,
    guint code,
    guint flags,
    int* status,
    void* user_data)
{
    TestConfigService* service = user_data;
    const char* iface = gbinder_remote_request_interface(req);

    if (gutil_strv_contains((const GStrV*)radio_config_req_ifaces, iface)) {
        const int count = test_config_service_req_count(service, code) + 1;
        GBinderReader reader;

        GDEBUG("%s %s %d", iface, radio_config_req_name(NULL, code), count);
        g_hash_table_insert(service->req_count, GINT_TO_POINTER(code),
            GINT_TO_POINTER(count));

        gbinder_remote_request_init_reader(req, &reader);
        if (code == RADIO_CONFIG_REQ_SET_RESPONSE_FUNCTIONS) {
            GBinderRemoteObject* resp_obj = gbinder_reader_read_object(&reader);
            GBinderRemoteObject* ind_obj = gbinder_reader_read_object(&reader);

            g_assert(resp_obj);
            g_assert(ind_obj);
            gbinder_client_unref(service->resp_client);
            gbinder_client_unref(service->ind_client);
            service->resp_client = gbinder_client_new2(resp_obj,
                TEST_ARRAY_AND_COUNT(radio_config_resp_iface_info));
            service->ind_client = gbinder_client_new2(ind_obj,
                TEST_ARRAY_AND_COUNT(radio_config_ind_iface_info));
            gbinder_remote_object_unref(resp_obj);
            gbinder_remote_object_unref(ind_obj);
        } else if (code == FAIL_REQ) {
            GDEBUG("failing request transaction");
            *status = GBINDER_STATUS_FAILED;
            return NULL;
        } else if (code == IGNORE_REQ) {
            GDEBUG("ignoring request transaction");
        } else {
            RadioResponseInfo info;
            GBinderWriter writer;
            RADIO_CONFIG_RESP resp_code = test_config_req_resp(code);
            GBinderLocalRequest* resp = gbinder_client_new_request2
                (service->resp_client, resp_code);

            memset(&info, 0, sizeof(info));
            info.type = RADIO_RESP_SOLICITED;
            info.error = (code == ERROR_REQ) ?
                RADIO_ERROR_GENERIC_FAILURE :
                RADIO_ERROR_NONE;
            g_assert(gbinder_reader_read_uint32(&reader, &info.serial));
            GDEBUG("serial %08x", info.serial);

            g_assert(resp);
            gbinder_local_request_init_writer(resp, &writer);
            gbinder_writer_append_buffer_object(&writer, &info, sizeof(info));

            switch (code) {
            case RADIO_CONFIG_REQ_SET_PREFERRED_DATA_MODEM:
                g_assert(gbinder_client_transact(service->resp_client,
                    resp_code, GBINDER_TX_FLAG_ONEWAY, resp, NULL, NULL, NULL));
                break;
            default:
                /* No expecting anything else */
                g_assert_not_reached();
                break;
            }
            gbinder_local_request_unref(resp);
        }
        *status = GBINDER_STATUS_OK;
        return NULL;
    } else {
        GDEBUG("%s %u", iface, code);
        *status = GBINDER_STATUS_FAILED;
        return NULL;
    }
}

static
void
test_config_service_init(
    TestConfigService* service)
{
    memset(service, 0, sizeof(*service));
    service->obj = test_gbinder_local_object_new(NULL,
        test_config_service_txproc, service);
    service->req_count = g_hash_table_new(g_direct_hash, g_direct_equal);
}

static
void
test_config_service_cleanup(
    TestConfigService* service)
{
    g_hash_table_destroy(service->req_count);
    gbinder_client_unref(service->resp_client);
    gbinder_client_unref(service->ind_client);
    gbinder_local_object_unref(service->obj);
    memset(service, 0, sizeof(*service));
}

/*==========================================================================*
 * Common setup for all tests
 *==========================================================================*/

typedef struct test_common {
    TestConfigService service;
    GBinderServiceManager* sm;
    GBinderRemoteObject* remote[RADIO_CONFIG_INTERFACE_COUNT];
    RadioConfig* client;
} TestCommon;

static
RadioConfig*
test_common_init(
    TestCommon* test,
    RADIO_CONFIG_INTERFACE version)
{
    RADIO_CONFIG_INTERFACE v;

    memset(test, 0, sizeof(*test));
    test->sm = gbinder_servicemanager_new(DEV);
    test_config_service_init(&test->service);
    for (v = RADIO_CONFIG_INTERFACE_1_0; v <= version; v++) {
        test->remote[v] = test_gbinder_servicemanager_new_service(test->sm,
            radio_config_fqnames[v], test->service.obj);
    }
    test->client = radio_config_new();
    g_assert(test->client);
    return test->client;
}

static
void
test_common_cleanup(
    TestCommon* test)
{
    int i;

    radio_config_unref(test->client);
    test_config_service_cleanup(&test->service);
    for (i = 0; i < G_N_ELEMENTS(test->remote); i++) {
        gbinder_remote_object_unref(test->remote[i]);
    }
    gbinder_servicemanager_unref(test->sm);
}

/*==========================================================================*
 * Another common setup
 *==========================================================================*/

typedef struct test_simple_data {
    TestCommon common;
    GMainLoop* loop;
    int completed; /* Typically used as a boolean */
    int destroyed; /* Typically used as a boolean */
} TestSimple;

static
RadioConfig*
test_simple_init(
    TestSimple* test)
{
    memset(test, 0, sizeof(*test));
    test->loop = g_main_loop_new(NULL, FALSE);
    return test_common_init(&test->common, RADIO_CONFIG_INTERFACE_1_1);
}

static
void
test_simple_cleanup(
    TestSimple* test)
{
    g_main_loop_unref(test->loop);
    test_common_cleanup(&test->common);
}

static
void
test_simple_destroy_cb(
    gpointer user_data)
{
    TestSimple* test = user_data;

    GDEBUG("done");
    g_assert(test->completed);
    g_assert(!test->destroyed);
    test->destroyed = TRUE;
    test_quit_later(test->loop);
}

/*==========================================================================*
 * null
 *==========================================================================*/

static
void
test_null(
    void)
{
    g_assert(!radio_config_ref(NULL));
    g_assert(radio_config_dead(NULL));
    g_assert_cmpint(radio_config_interface(NULL),==,RADIO_CONFIG_INTERFACE_NONE);
    g_assert(!radio_config_rpc_header_size(NULL, RADIO_CONFIG_REQ_NONE));
    g_assert(!radio_config_req_name(NULL, RADIO_CONFIG_REQ_NONE));
    g_assert(!radio_config_resp_name(NULL, RADIO_CONFIG_RESP_NONE));
    g_assert(!radio_config_ind_name(NULL, RADIO_CONFIG_IND_NONE));
    g_assert(!radio_config_add_death_handler(NULL, NULL, NULL));
    g_assert(!radio_config_add_request_observer(NULL,
        RADIO_CONFIG_REQ_ANY, NULL, NULL));
    g_assert(!radio_config_add_request_observer_with_priority(NULL,
        RADIO_CONFIG_REQ_ANY, RADIO_OBSERVER_PRIORITY_LOWEST, NULL, NULL));
    radio_config_unref(NULL);
    g_assert(!radio_config_add_response_observer(NULL,
        RADIO_CONFIG_RESP_ANY, NULL, NULL));
    g_assert(!radio_config_add_response_observer_with_priority(NULL,
        RADIO_CONFIG_RESP_ANY, RADIO_OBSERVER_PRIORITY_LOWEST, NULL, NULL));
    g_assert(!radio_config_add_indication_observer(NULL,
        RADIO_CONFIG_IND_ANY, NULL, NULL));
    g_assert(!radio_config_add_indication_observer_with_priority(NULL,
        RADIO_CONFIG_IND_ANY, RADIO_OBSERVER_PRIORITY_LOWEST, NULL, NULL));
    g_assert(!radio_config_request_new(NULL, ERROR_REQ, NULL, NULL, NULL, NULL));

    radio_config_unref(NULL);
    radio_config_remove_handler(NULL, 0);
    radio_config_remove_handlers(NULL, NULL, 0);
}

/*==========================================================================*
 * name
 *==========================================================================*/

static
void
test_name(
    void)
{
    TestCommon test;
    RadioConfig* client = test_common_init(&test, RADIO_CONFIG_INTERFACE_1_1);

    g_assert_cmpstr(radio_config_req_name(client,
        RADIO_CONFIG_REQ_SET_RESPONSE_FUNCTIONS), == ,
        "setResponseFunctions");
    g_assert_cmpstr(radio_config_req_name(client,
        RADIO_CONFIG_REQ_GET_SIM_SLOTS_STATUS), == ,
        "getSimSlotsStatus");
    g_assert_cmpstr(radio_config_req_name(client,
        RADIO_CONFIG_REQ_GET_PHONE_CAPABILITY), == ,
        "getPhoneCapability");
    g_assert_cmpstr(radio_config_req_name(client,
        (RADIO_CONFIG_REQ)123), == ,
        "123");

    g_assert_cmpstr(radio_config_resp_name(client,
        RADIO_CONFIG_RESP_GET_SIM_SLOTS_STATUS), == ,
        "getSimSlotsStatusResponse");
    g_assert_cmpstr(radio_config_resp_name(client,
        RADIO_CONFIG_RESP_GET_PHONE_CAPABILITY), == ,
        "getPhoneCapabilityResponse");
    g_assert_cmpstr(radio_config_resp_name(client,
        (RADIO_CONFIG_RESP)1234), == ,
        "1234");

    g_assert_cmpstr(radio_config_ind_name(client,
        RADIO_CONFIG_IND_SIM_SLOTS_STATUS_CHANGED), == ,
        "simSlotsStatusChanged");
    g_assert_cmpstr(radio_config_ind_name(client,
        (RADIO_CONFIG_IND)12345), == ,
        "12345");

    test_common_cleanup(&test);
}

/*==========================================================================*
 * none
 *==========================================================================*/

static
void
test_none(
    void)
{
    /* No service => no client */
    g_assert(!radio_config_new());
}

/*==========================================================================*
 * basic
 *==========================================================================*/

static
void
test_basic(
    void)
{
    TestCommon test;
    RADIO_CONFIG_INTERFACE version = RADIO_CONFIG_INTERFACE_1_0;
    RadioConfig* client = test_common_init(&test, version);
    RadioRequest* req;
    int destroyed = 0;

    g_assert(!radio_config_dead(client));
    g_assert(radio_config_rpc_header_size(client,
        RADIO_CONFIG_REQ_GET_SIM_SLOTS_STATUS));
    g_assert_cmpint(radio_config_interface(client), == ,version);
    g_assert(radio_config_ref(client) == client);
    radio_config_unref(client);

    /* Instances are reused */
    g_assert(radio_config_new() == client);
    radio_config_unref(client);
    g_assert(radio_config_new_with_version(version) == client);
    radio_config_unref(client);
    g_assert(radio_config_new_with_version(RADIO_CONFIG_INTERFACE_COUNT) ==
        client);
    radio_config_unref(client);

    /* Adding NULL observer is a nop */
    g_assert(!radio_config_add_death_handler(client, NULL, NULL));
    g_assert(!radio_config_add_request_observer(client,
        RADIO_CONFIG_REQ_ANY, NULL, NULL));
    g_assert(!radio_config_add_response_observer(client,
        RADIO_CONFIG_RESP_ANY, NULL, NULL));
    g_assert(!radio_config_add_indication_observer(client,
        RADIO_CONFIG_IND_ANY, NULL, NULL));

    /* Zero handler id is tolerated */
    radio_config_remove_handler(client, 0);

    /* Create and destroy the request */
    req = radio_config_request_new(client, RADIO_CONFIG_REQ_GET_MODEMS_CONFIG,
        NULL, NULL, test_inc_cb, &destroyed);
    g_assert(req);
    radio_request_unref(req);
    g_assert(destroyed);

    test_common_cleanup(&test);
}

/*==========================================================================*
 * ind
 *==========================================================================*/

typedef struct test_ind_data {
    TestCommon common;
    GMainLoop* loop;
    RADIO_CONFIG_IND ind;
} TestInd;

static
void
test_ind_cb(
    RadioConfig* config,
    RADIO_CONFIG_IND code,
    const GBinderReader* args,
    gpointer user_data)
{
    TestInd* test = user_data;

    /* This one is invoked first */
    GDEBUG("first indication  %d", code);
    g_assert_cmpint(code, == ,RADIO_CONFIG_IND_SIM_SLOTS_STATUS_CHANGED);
    g_assert_cmpint(test->ind, == ,RADIO_CONFIG_IND_NONE);
    test->ind = code;
}

static
void
test_ind_cb2(
    RadioConfig* config,
    RADIO_CONFIG_IND code,
    const GBinderReader* args,
    gpointer user_data)
{
    TestInd* test = user_data;

    /* This one is invoked second */
    GDEBUG("second indication %d", code);
    g_assert_cmpint(test->ind, == ,RADIO_CONFIG_IND_SIM_SLOTS_STATUS_CHANGED);
    g_assert_cmpint(code, == ,RADIO_CONFIG_IND_SIM_SLOTS_STATUS_CHANGED);
    test_quit_later(test->loop);
}

static
void
test_ind(
    void)
{
    GBinderLocalRequest* req;
    GBinderClient* ind_client;
    RadioConfig* client;
    TestInd test;
    gulong id[2];

    memset(&test, 0, sizeof(test));
    test.loop = g_main_loop_new(NULL, FALSE);
    client = test_common_init(&test.common, RADIO_CONFIG_INTERFACE_1_1);
    ind_client = test.common.service.ind_client;

    /* Register and unregister one listener */
    id[0] = radio_config_add_indication_observer(client, RADIO_CONFIG_IND_ANY,
        test_ind_not_reached, NULL);
    radio_config_remove_handler(client, id[0]);

    /* Register actual listeners */
    id[0] = radio_config_add_indication_observer_with_priority(client,
        RADIO_OBSERVER_PRIORITY_HIGHEST,
        RADIO_CONFIG_IND_SIM_SLOTS_STATUS_CHANGED,
        test_ind_cb, &test);
    id[1] = radio_config_add_indication_observer_with_priority(client,
        RADIO_OBSERVER_PRIORITY_DEFAULT,
        RADIO_CONFIG_IND_SIM_SLOTS_STATUS_CHANGED,
        test_ind_cb2, &test);

    /* This one will be ignored because type is missing */
    req = gbinder_client_new_request2(ind_client,
        RADIO_CONFIG_IND_SIM_SLOTS_STATUS_CHANGED);
    g_assert_cmpint(gbinder_client_transact(ind_client,
        RADIO_CONFIG_IND_SIM_SLOTS_STATUS_CHANGED, GBINDER_TX_FLAG_ONEWAY,
        req, NULL, NULL, NULL), != ,0);
    gbinder_local_request_unref(req);

    /* This one will be ignored because RADIO_IND_ACK_EXP is not expected */
    req = gbinder_client_new_request2(ind_client,
        RADIO_CONFIG_IND_SIM_SLOTS_STATUS_CHANGED);
    gbinder_local_request_append_int32(req, RADIO_IND_ACK_EXP);
    g_assert_cmpint(gbinder_client_transact(ind_client,
        RADIO_CONFIG_IND_SIM_SLOTS_STATUS_CHANGED, GBINDER_TX_FLAG_ONEWAY,
        req, NULL, NULL, NULL), != ,0);
    gbinder_local_request_unref(req);

    /* And this one will be handled */
    req = gbinder_client_new_request2(ind_client,
        RADIO_CONFIG_IND_SIM_SLOTS_STATUS_CHANGED);
    gbinder_local_request_append_int32(req, RADIO_IND_UNSOLICITED);
    /*
     * RadioIndicationType should be followed by vec<SimSlotStatus> but
     * that's not required for the purposes of this unit test.
     */
    g_assert_cmpint(gbinder_client_transact(ind_client,
        RADIO_CONFIG_IND_SIM_SLOTS_STATUS_CHANGED, GBINDER_TX_FLAG_ONEWAY,
        req, NULL, NULL, NULL), != ,0);
    gbinder_local_request_unref(req);

    /* And wait for test_ind_cb2 to terminate the loop */
    test_run(&test_opt, test.loop);
    g_assert_cmpint(test.ind, == ,RADIO_CONFIG_IND_SIM_SLOTS_STATUS_CHANGED);

    /* Cleanup */
    radio_config_remove_all_handlers(client, id);
    g_main_loop_unref(test.loop);
    test_common_cleanup(&test.common);
}

/*==========================================================================*
 * resp
 *==========================================================================*/

static
void
test_resp_observe_req1(
    RadioConfig* config,
    RADIO_CONFIG_REQ code,
    GBinderLocalRequest* args,
    gpointer user_data)
{
    int* observed = user_data;

    GDEBUG("high prio observed req %d", code);
    g_assert(!*observed);
    *observed = -((int)code);
}

static
void
test_resp_observe_req2(
    RadioConfig* config,
    RADIO_CONFIG_REQ code,
    GBinderLocalRequest* args,
    gpointer user_data)
{
    int* observed = user_data;

    GDEBUG("low prio observed req %d", code);
    g_assert_cmpint(*observed, == ,-((int)code));
    *observed = code;
}

static
void
test_resp_observe_resp1(
    RadioConfig* config,
    RADIO_CONFIG_RESP code,
    const RadioResponseInfo* info,
    const GBinderReader* args,
    gpointer user_data)
{
    int* observed = user_data;

    GDEBUG("high prio observed resp %d", code);
    g_assert(!*observed);
    *observed = -((int)code);
}

static
void
test_resp_observe_resp2(
    RadioConfig* config,
    RADIO_CONFIG_RESP code,
    const RadioResponseInfo* info,
    const GBinderReader* args,
    gpointer user_data)
{
    int* observed = user_data;

    GDEBUG("low prio observed resp %d", code);
    g_assert_cmpint(*observed, == ,-((int)code));
    *observed = code;
}

static
void
test_resp_complete_cb(
    RadioRequest* req,
    RADIO_TX_STATUS status,
    RADIO_CONFIG_RESP resp,
    RADIO_ERROR error,
    const GBinderReader* reader,
    gpointer user_data)
{
    TestSimple* test = user_data;

    GDEBUG("resp %d", resp);
    g_assert_cmpint(status, == ,RADIO_TX_STATUS_OK);
    g_assert_cmpint(resp, == ,RADIO_CONFIG_RESP_SET_PREFERRED_DATA_MODEM);
    g_assert_cmpint(error, == ,RADIO_ERROR_NONE);
    g_assert(!test->completed);
    g_assert(!test->destroyed);
    test->completed = TRUE;
}

static
void
test_resp(
    void)
{
    TestSimple test;
    RadioConfig* client = test_simple_init(&test);
    RadioRequest* req = radio_config_request_new(client,
        RADIO_CONFIG_REQ_SET_PREFERRED_DATA_MODEM, NULL,
        test_resp_complete_cb, test_simple_destroy_cb, &test);
    int observed_req = 0, observed_resp = 0;
    gulong id[4];

    id[0] = radio_config_add_request_observer_with_priority(client,
        RADIO_OBSERVER_PRIORITY_HIGHEST, RADIO_CONFIG_REQ_ANY,
        test_resp_observe_req1, &observed_req);
    id[1] = radio_config_add_request_observer_with_priority(client,
        RADIO_OBSERVER_PRIORITY_LOWEST, RADIO_CONFIG_REQ_ANY,
        test_resp_observe_req2, &observed_req);
    id[2] = radio_config_add_response_observer_with_priority(client,
        RADIO_OBSERVER_PRIORITY_HIGHEST, RADIO_CONFIG_RESP_ANY,
        test_resp_observe_resp1, &observed_resp);
    id[3] = radio_config_add_response_observer_with_priority(client,
        RADIO_OBSERVER_PRIORITY_LOWEST, RADIO_CONFIG_RESP_ANY,
        test_resp_observe_resp2, &observed_resp);

    g_assert(radio_request_submit(req));
    radio_request_unref(req);

    test_run(&test_opt, test.loop);

    g_assert(test.completed);
    g_assert(test.destroyed);
    g_assert_cmpint(observed_req,==,RADIO_CONFIG_REQ_SET_PREFERRED_DATA_MODEM);
    g_assert_cmpint(observed_resp,==,RADIO_CONFIG_RESP_SET_PREFERRED_DATA_MODEM);

    /* Cleanup */
    radio_config_remove_all_handlers(client, id);
    test_simple_cleanup(&test);
}

/*==========================================================================*
 * cancel
 *==========================================================================*/

static
void
test_cancel(
    void)
{
    TestCommon test;
    gboolean destroyed = FALSE;
    RadioConfig* client = test_common_init(&test, RADIO_CONFIG_INTERFACE_1_0);
    RadioRequest* req = radio_config_request_new(client,
        RADIO_CONFIG_REQ_GET_MODEMS_CONFIG, NULL,
        test_complete_not_reached, test_destroy_once, &destroyed);

    g_assert(radio_request_submit(req));
    radio_request_cancel(req);
    g_assert_cmpint(req->state, == ,RADIO_REQUEST_STATE_CANCELLED);
    radio_request_unref(req);

    g_assert(destroyed);
    test_common_cleanup(&test);
}

/*==========================================================================*
 * fail_tx
 *==========================================================================*/

static
void
test_fail_tx(
    void)
{
    TestCommon test;
    gboolean destroyed = FALSE;
    RadioConfig* client = test_common_init(&test, RADIO_CONFIG_INTERFACE_1_0);
    RadioRequest* req = radio_config_request_new(client,
        RADIO_CONFIG_REQ_GET_MODEMS_CONFIG, NULL,
        test_complete_not_reached, test_destroy_once, &destroyed);

    g_assert(req);
    g_assert(req->serial);

    /* Fail one transaction */
    test_gbinder_client_tx_fail_count = 1;

    /* Request switches in the FAILED state */
    g_assert(req);
    g_assert(!radio_request_submit(req));
    g_assert_cmpint(req->state, == ,RADIO_REQUEST_STATE_FAILED);

    g_assert(!destroyed);
    radio_request_drop(req);
    g_assert(destroyed);

    test_common_cleanup(&test);
}

/*==========================================================================*
 * death
 *==========================================================================*/

static
void
test_death_complete_cb(
    RadioRequest* req,
    RADIO_TX_STATUS status,
    RADIO_CONFIG_RESP resp,
    RADIO_ERROR error,
    const GBinderReader* reader,
    gpointer user_data)
{
    TestSimple* test = user_data;

    GDEBUG("status %u", status);
    g_assert_cmpint(status, == ,RADIO_TX_STATUS_FAILED);
    g_assert_cmpint(resp, == ,RADIO_CONFIG_RESP_NONE);
    g_assert_cmpint(error, == ,RADIO_ERROR_NONE);
    test->completed++;
}

static
void
test_death_destroy_cb(
    gpointer user_data)
{
    TestSimple* test = user_data;

    test->destroyed++;
    GDEBUG("done");
    g_assert_cmpint(test->completed, == ,test->destroyed);
    test_quit_later(test->loop);
}

static
void
test_death(
    void)
{
    TestSimple test;
    RadioConfig* client = test_simple_init(&test);
    RadioRequest* req = radio_config_request_new(client,
        RADIO_CONFIG_REQ_GET_MODEMS_CONFIG, NULL,
        test_death_complete_cb, test_death_destroy_cb, &test);
    RADIO_CONFIG_INTERFACE v;
    int death_count = 0;
    gulong id = radio_config_add_death_handler(client, test_config_inc_cb,
        &death_count);

    g_assert(radio_request_submit(req));
    radio_request_unref(req);

    /* Kill the remote objects */
    g_assert(!radio_config_dead(client));
    for (v = RADIO_CONFIG_INTERFACE_1_0;
         v <= radio_config_interface(client); v++) {
        test_gbinder_remote_object_kill(test.common.remote[v]);
    }
    g_assert(radio_config_dead(client));
    g_assert_cmpint(death_count, == ,1);

    /* Now expect the request to fail */
    test_run(&test_opt, test.loop);
    g_assert(test.completed);
    g_assert(test.destroyed);

    /* Cleanup */
    radio_config_remove_handler(client, id);
    test_simple_cleanup(&test);
}

/*==========================================================================*
 * Common
 *==========================================================================*/

#define TEST_PREFIX "/config/"
#define TEST_(t) TEST_PREFIX t

int main(int argc, char* argv[])
{
    g_test_init(&argc, &argv, NULL);
    g_test_add_func(TEST_("null"), test_null);
    g_test_add_func(TEST_("name"), test_name);
    g_test_add_func(TEST_("none"), test_none);
    g_test_add_func(TEST_("basic"), test_basic);
    g_test_add_func(TEST_("ind"), test_ind);
    g_test_add_func(TEST_("resp"), test_resp);
    g_test_add_func(TEST_("cancel"), test_cancel);
    g_test_add_func(TEST_("fail_tx"), test_fail_tx);
    g_test_add_func(TEST_("death"), test_death);
    test_init(&test_opt, argc, argv);
    return g_test_run();
}

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
