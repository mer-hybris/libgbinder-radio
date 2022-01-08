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

#include "test_common.h"
#include "test_gbinder.h"

#include "radio_instance_p.h"
#include "radio_util.h"

#include <gutil_strv.h>
#include <gutil_log.h>

#define DEFAULT_INTERFACE RADIO_INTERFACE_1_0
#define DEV GBINDER_DEFAULT_BINDER

#define UNKNOWN_VALUE (0x7fffffff)
#define UNKNOWN_VALUE_STR "2147483647" /* 0x7fffffff */
#define UNKNOWN_REQ ((RADIO_REQ)UNKNOWN_VALUE)
#define UNKNOWN_REQ_STR UNKNOWN_VALUE_STR
#define UNKNOWN_IND ((RADIO_IND)UNKNOWN_VALUE)
#define UNKNOWN_IND_STR UNKNOWN_VALUE_STR
#define UNKNOWN_RESP ((RADIO_RESP)UNKNOWN_VALUE)
#define UNKNOWN_RESP_STR UNKNOWN_VALUE_STR
#define INVALID_IND_TYPE ((RADIO_IND_TYPE)UNKNOWN_VALUE)

RadioInstance*
radio_instance_get_with_interface(
    const char* dev,
    const char* name,
    RADIO_INTERFACE version); /* Deprecated and removed from the .h file */

gboolean
radio_instance_is_dead(
    RadioInstance* self); /* No sure why this one is missing */

typedef struct test_radio_service {
    GBinderLocalObject* obj;
    GBinderRemoteObject* resp_obj;
    GBinderRemoteObject* ind_obj;
    GHashTable* req_count;
} TestRadioService;

static TestOpt test_opt;

static const GBinderClientIfaceInfo radio_ind_iface_info[] = {
    {RADIO_INDICATION_1_4, RADIO_1_4_IND_LAST },
    {RADIO_INDICATION_1_3, RADIO_1_3_IND_LAST },
    {RADIO_INDICATION_1_2, RADIO_1_2_IND_LAST },
    {RADIO_INDICATION_1_1, RADIO_1_1_IND_LAST },
    {RADIO_INDICATION_1_0, RADIO_1_0_IND_LAST }
};

static const GBinderClientIfaceInfo radio_resp_iface_info[] = {
    {RADIO_RESPONSE_1_4, RADIO_1_4_RESP_LAST },
    {RADIO_RESPONSE_1_3, RADIO_1_3_RESP_LAST },
    {RADIO_RESPONSE_1_2, RADIO_1_2_RESP_LAST },
    {RADIO_RESPONSE_1_1, RADIO_1_1_RESP_LAST },
    {RADIO_RESPONSE_1_0, RADIO_1_0_RESP_LAST }
};

static const char* const radio_req_ifaces[] = {
    RADIO_1_4,
    RADIO_1_3,
    RADIO_1_2,
    RADIO_1_1,
    RADIO_1_0,
    NULL
};

static
gboolean
test_response_not_handled(
    RadioInstance* radio,
    RADIO_RESP code,
    const RadioResponseInfo* info,
    const GBinderReader* reader,
    gpointer user_data)
{
    g_assert_not_reached();
    return FALSE;
}

static
void
test_response_not_observed(
    RadioInstance* radio,
    RADIO_RESP code,
    const RadioResponseInfo* info,
    const GBinderReader* reader,
    gpointer user_data)
{
    g_assert_not_reached();
}

/*==========================================================================*
 * Test IRadio service
 *==========================================================================*/

static
int
test_service_req_count(
    TestRadioService* service,
    RADIO_REQ req)
{
    return GPOINTER_TO_INT(g_hash_table_lookup(service->req_count,
        GINT_TO_POINTER(req)));
}

static
GBinderLocalReply*
test_service_txproc(
    GBinderLocalObject* obj,
    GBinderRemoteRequest* req,
    guint code,
    guint flags,
    int* status,
    void* user_data)
{
    TestRadioService* service = user_data;
    const char* iface = gbinder_remote_request_interface(req);

    if (gutil_strv_contains((const GStrV*)radio_req_ifaces, iface)) {
        const int count = test_service_req_count(service, code) + 1;
        GBinderReader reader;

        GDEBUG("%s %s %d", iface, radio_req_name(code), count);
        g_hash_table_insert(service->req_count, GINT_TO_POINTER(code),
            GINT_TO_POINTER(count));

        gbinder_remote_request_init_reader(req, &reader);
        switch (code) {
        case RADIO_REQ_SET_RESPONSE_FUNCTIONS:
            gbinder_remote_object_unref(service->resp_obj);
            gbinder_remote_object_unref(service->ind_obj);
            service->resp_obj = gbinder_reader_read_object(&reader);
            service->ind_obj = gbinder_reader_read_object(&reader);
            g_assert(service->resp_obj);
            g_assert(service->ind_obj);
            break;
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
test_service_init(
    TestRadioService* service)
{
    memset(service, 0, sizeof(*service));
    service->obj = test_gbinder_local_object_new(NULL,
        test_service_txproc, service);
    service->req_count = g_hash_table_new(g_direct_hash, g_direct_equal);
}

static
void
test_service_cleanup(
    TestRadioService* service)
{
    g_hash_table_destroy(service->req_count);
    gbinder_remote_object_unref(service->resp_obj);
    gbinder_remote_object_unref(service->ind_obj);
    gbinder_local_object_unref(service->obj);
    memset(service, 0, sizeof(*service));
}

/*==========================================================================*
 * null
 *==========================================================================*/

static
void
test_null(
    void)
{
    radio_instance_set_enabled(NULL, FALSE);
    radio_instance_remove_handler(NULL, 0);
    radio_instance_remove_handlers(NULL, NULL, 0);
    radio_instance_unref(NULL);
    radio_instance_cancel_request(NULL, 0);

    g_assert(radio_instance_is_dead(NULL));
    g_assert(!radio_instance_get(NULL, NULL));
    g_assert(!radio_instance_get("", NULL));
    g_assert(!radio_instance_get("/dev/binder", NULL));
    g_assert(!radio_instance_get("/dev/binder", ""));
    g_assert(!radio_instance_get_with_interface("", "", DEFAULT_INTERFACE));
    g_assert(!radio_instance_get_with_version("foo", "bar", DEFAULT_INTERFACE));
    g_assert(!radio_instance_new(NULL, NULL));
    g_assert(!radio_instance_new_with_modem_and_slot(NULL, NULL, NULL, 0));
    g_assert(!radio_instance_new_with_version(NULL, NULL, DEFAULT_INTERFACE));
    g_assert(!radio_instance_new_with_version(NULL, "", DEFAULT_INTERFACE));
    g_assert(!radio_instance_new_with_version(NULL, "foo", DEFAULT_INTERFACE));
    g_assert(!radio_instance_new_request(NULL, 0));
    g_assert(!radio_instance_ack(NULL));
    g_assert(!radio_instance_ref(NULL));
    g_assert(!radio_instance_rpc_header_size(NULL, 0));
    g_assert(!radio_instance_send_request_sync(NULL, 0, NULL));
    g_assert(!radio_instance_add_request_observer(NULL, 0, NULL, NULL));
    g_assert(!radio_instance_add_indication_handler(NULL, 0, NULL, NULL));
    g_assert(!radio_instance_add_indication_observer(NULL, 0, NULL, NULL));
    g_assert(!radio_instance_add_response_handler(NULL, 0, NULL, NULL));
    g_assert(!radio_instance_add_response_observer(NULL, 0, NULL, NULL));
    g_assert(!radio_instance_add_ack_handler(NULL, NULL, NULL));
    g_assert(!radio_instance_add_death_handler(NULL, NULL, NULL));
    g_assert(!radio_instance_add_enabled_handler(NULL, NULL, NULL));
    g_assert(!radio_instance_add_connected_handler(NULL, NULL, NULL));
    g_assert(!radio_instance_req_name(NULL, UNKNOWN_REQ));
    g_assert(!radio_instance_resp_name(NULL, UNKNOWN_RESP));
    g_assert(!radio_instance_ind_name(NULL, UNKNOWN_IND));
    g_assert(!radio_instance_send_request(NULL,0,NULL,NULL,NULL,NULL,NULL));
}

/*==========================================================================*
 * basic
 *==========================================================================*/

static
void
test_basic(
    void)
{
    GBinderServiceManager* sm = gbinder_servicemanager_new(DEV);
    GBinderRemoteObject* remote;
    RadioInstance* radio;
    RadioInstance* const* radios;
    TestRadioService service;
    const RADIO_INTERFACE version = RADIO_INTERFACE_1_4;
    const char* slot = "slot1";
    const char* fqname = RADIO_1_0 "/slot1";
    GQuark q;

    /* This fails because there's no radio service */
    g_assert(!radio_instance_new_with_version(DEV, slot, DEFAULT_INTERFACE));
    g_assert(!radio_instance_get_all());

    /* Register the service to create an instance */
    test_service_init(&service);
    remote = test_gbinder_servicemanager_new_service(sm, fqname, service.obj);
    radio = radio_instance_new_with_version(DEV, slot, version);
    g_assert(radio);
    g_assert(service.ind_obj);
    g_assert(service.resp_obj);

    /* The second call returns new reference to the same instance */
    g_assert(radio == radio_instance_new_with_version(DEV, slot, version));
    radio_instance_unref(radio);

    /* Test quarks */
    q = radio_instance_ind_quark(radio, UNKNOWN_IND);
    g_assert(q);
    g_assert(q == radio_instance_ind_quark(radio, UNKNOWN_IND));

    /* Expecting non-zero RPC header size for a valid request code */
    g_assert(radio_instance_rpc_header_size(radio, RADIO_REQ_DIAL));
    g_assert_cmpuint(radio_instance_rpc_header_size(radio, UNKNOWN_REQ),==,0);

    /* The one we have created must still be there */
    g_assert(radio == radio_instance_get_with_version(DEV, slot, version));

    /* NULL callbacks are ignored */
    g_assert(!radio_instance_add_request_observer(radio, 0, NULL, NULL));
    g_assert(!radio_instance_add_indication_handler(radio, 0, NULL, NULL));
    g_assert(!radio_instance_add_indication_observer(radio, 0, NULL, NULL));
    g_assert(!radio_instance_add_response_handler(radio, 0, NULL, NULL));
    g_assert(!radio_instance_add_response_observer(radio, 0, NULL, NULL));
    g_assert(!radio_instance_add_ack_handler(radio, NULL, NULL));
    g_assert(!radio_instance_add_death_handler(radio, NULL, NULL));
    g_assert(!radio_instance_add_enabled_handler(radio, NULL, NULL));
    g_assert(!radio_instance_add_connected_handler(radio, NULL, NULL));

    /* Formatting unknown codes (RadioInstance owns the string) */
    g_assert_cmpstr(radio_instance_req_name(radio, UNKNOWN_REQ), == ,
        UNKNOWN_REQ_STR);
    g_assert_cmpstr(radio_instance_resp_name(radio, UNKNOWN_RESP), == ,
        UNKNOWN_RESP_STR);
    g_assert_cmpstr(radio_instance_ind_name(radio, UNKNOWN_IND), == ,
        UNKNOWN_IND_STR);
    g_assert_cmpstr(radio_instance_req_name(radio,
        RADIO_REQ_DIAL), == ,"dial");
    g_assert_cmpstr(radio_instance_resp_name(radio,
        RADIO_RESP_DIAL), == ,"dialResponse");
    g_assert_cmpstr(radio_instance_ind_name(radio,
        RADIO_IND_MODEM_RESET), == ,"modemReset");

    /* The entire list consists of that one instance */
    radios = radio_instance_get_all();
    g_assert(radios);
    g_assert(radios[0] == radio);
    g_assert(!radios[1]);

    radio_instance_unref(radio);
    test_service_cleanup(&service);
    gbinder_remote_object_unref(remote);
    gbinder_servicemanager_unref(sm);
}

/*==========================================================================*
 * connected
 *==========================================================================*/

typedef struct test_connected_data {
    int observed;
    int connected;
} TestConnected;

static
void
test_connected_observer_high(
    RadioInstance* radio,
    RADIO_IND code,
    RADIO_IND_TYPE type,
    const GBinderReader* reader,
    gpointer user_data)
{
    TestConnected* test = user_data;

    g_assert_cmpint(test->observed % 3, == ,0);
    g_assert(!test->observed || test->connected);
    test->observed++;
    GDEBUG_("%d", test->observed);
}

static
void
test_connected_observer_default(
    RadioInstance* radio,
    RADIO_IND code,
    RADIO_IND_TYPE type,
    const GBinderReader* reader,
    gpointer user_data)
{
    TestConnected* test = user_data;

    g_assert_cmpint(test->observed % 3, == ,1);
    g_assert_cmpint(test->connected, == ,1);
    test->observed++;
    GDEBUG_("%d", test->observed);
}

static
void
test_connected_observer_low(
    RadioInstance* radio,
    RADIO_IND code,
    RADIO_IND_TYPE type,
    const GBinderReader* reader,
    gpointer user_data)
{
    TestConnected* test = user_data;

    g_assert_cmpint(test->observed % 3, == ,2);
    g_assert_cmpint(test->connected, == ,1);
    test->observed++;
    GDEBUG_("%d", test->observed);
}

static
void
test_connected_cb(
    RadioInstance* radio,
    gpointer user_data)
{
    TestConnected* test = user_data;

    g_assert(!test->connected);
    test->connected++;
    GDEBUG_("");
}

static
void
test_connected(
    void)
{
    GBinderServiceManager* sm = gbinder_servicemanager_new(DEV);
    GBinderRemoteObject* remote;
    RadioInstance* radio;
    TestRadioService service;
    GBinderClient* ind;
    GBinderLocalRequest* req;
    const RADIO_INTERFACE version = RADIO_INTERFACE_1_4;
    const char* slot = "slot1";
    const char* fqname = RADIO_1_0 "/slot1";
    TestConnected test;
    gulong id[4];

    memset(&test, 0, sizeof(test));

    /* Register the service to create an instance */
    test_service_init(&service);
    remote = test_gbinder_servicemanager_new_service(sm, fqname, service.obj);
    radio = radio_instance_new_with_version(DEV, slot, version);
    g_assert(radio);
    g_assert(!radio->connected);

    id[0] = radio_instance_add_indication_observer_with_priority(radio,
        RADIO_INSTANCE_PRIORITY_LOWEST,  RADIO_IND_ANY,
        test_connected_observer_low, &test);
    id[1] = radio_instance_add_indication_observer(radio,RADIO_IND_ANY,
        test_connected_observer_default, &test);
    id[2] = radio_instance_add_indication_observer_with_priority(radio,
        RADIO_INSTANCE_PRIORITY_HIGHEST + 1 /* becomes HIGHEST */,
        RADIO_IND_ANY, test_connected_observer_high, &test);
    id[3] = radio_instance_add_connected_handler(radio, test_connected_cb,
        &test);

    /* Issue rilConnected */
    g_assert(service.ind_obj);
    ind = gbinder_client_new2(service.ind_obj,
        TEST_ARRAY_AND_COUNT(radio_ind_iface_info));
    g_assert(ind);
    req = gbinder_client_new_request2(ind, RADIO_IND_RIL_CONNECTED);
    gbinder_local_request_append_int32(req, RADIO_IND_ACK_EXP);

    g_assert_cmpint(gbinder_client_transact_sync_oneway(ind,
        RADIO_IND_RIL_CONNECTED, req), == ,GBINDER_STATUS_OK);
    g_assert_cmpint(test.observed, == ,3);
    g_assert_cmpint(test.connected, == ,1);
    g_assert(radio->connected);

    /* Second time around observer is still called but connect handler isn't */
    g_assert_cmpint(gbinder_client_transact_sync_oneway(ind,
        RADIO_IND_RIL_CONNECTED, req), == ,GBINDER_STATUS_OK);
    g_assert_cmpint(test.observed, == ,6);
    g_assert_cmpint(test.connected, == ,1);
    g_assert(radio->connected);

    gbinder_local_request_unref(req);
    gbinder_client_unref(ind);

    radio_instance_remove_all_handlers(radio, id);
    radio_instance_unref(radio);
    test_service_cleanup(&service);
    gbinder_remote_object_unref(remote);
    gbinder_servicemanager_unref(sm);
}

/*==========================================================================*
 * ind
 *==========================================================================*/

static
gboolean
test_ind_handle(
    RadioInstance* radio,
    RADIO_IND code,
    RADIO_IND_TYPE type,
    const GBinderReader* reader,
    gpointer user_data)
{
    int* expected = user_data;

    g_assert_cmpint(code, == ,*expected);
    *expected = RADIO_IND_NONE;
    radio_instance_ack(radio);
    return TRUE;
}

static
void
test_ind_observe(
    RadioInstance* radio,
    RADIO_IND code,
    RADIO_IND_TYPE type,
    const GBinderReader* reader,
    gpointer user_data)
{
    int* expected = user_data;

    g_assert_cmpint(code, == ,*expected);
    *expected = RADIO_IND_NONE;
}

static
void
test_ind(
    void)
{
    GBinderServiceManager* sm = gbinder_servicemanager_new(DEV);
    GBinderRemoteObject* remote;
    RadioInstance* radio;
    TestRadioService service;
    GBinderClient* ind;
    GBinderLocalRequest* req;
    const RADIO_INTERFACE version = RADIO_INTERFACE_1_4;
    const char* slot = "slot1";
    const char* fqname = RADIO_1_0 "/slot1";
    int code[2];
    gulong id[2];

    /* Register the service to create an instance */
    test_service_init(&service);
    remote = test_gbinder_servicemanager_new_service(sm, fqname, service.obj);
    radio = radio_instance_new_with_version(DEV, slot, version);
    g_assert(radio);

    /* Issue invalid indication (no type) */
    code[0] = code[1] = RADIO_IND_RIL_CONNECTED;
    id[0] = radio_instance_add_indication_handler(radio, RADIO_IND_RIL_CONNECTED,
        test_ind_handle, code + 0);
    id[1] = radio_instance_add_indication_observer(radio, RADIO_IND_ANY,
        test_ind_observe, code + 1);
    g_assert(service.ind_obj);
    ind = gbinder_client_new2(service.ind_obj,
        TEST_ARRAY_AND_COUNT(radio_ind_iface_info));
    req = gbinder_client_new_request2(ind, RADIO_IND_RIL_CONNECTED);
    g_assert_cmpint(gbinder_client_transact_sync_oneway(ind,
        RADIO_IND_RIL_CONNECTED, req), == ,GBINDER_STATUS_FAILED);

    /* No signals issued and no acks sent */
    g_assert_cmpint(code[0], == ,RADIO_IND_RIL_CONNECTED);
    g_assert_cmpint(code[1], == ,RADIO_IND_RIL_CONNECTED);
    g_assert_cmpint(test_service_req_count(&service,
        RADIO_REQ_RESPONSE_ACKNOWLEDGEMENT), == ,0);

    /* Another invalid indication (invalid type) */
    gbinder_local_request_append_int32(req, INVALID_IND_TYPE);
    g_assert_cmpint(gbinder_client_transact_sync_oneway(ind,
        RADIO_IND_RIL_CONNECTED, req), == ,GBINDER_STATUS_FAILED);

    /* No signals issued and no acks sent */
    g_assert_cmpint(code[0], == ,RADIO_IND_RIL_CONNECTED);
    g_assert_cmpint(code[1], == ,RADIO_IND_RIL_CONNECTED);
    g_assert_cmpint(test_service_req_count(&service,
        RADIO_REQ_RESPONSE_ACKNOWLEDGEMENT), == ,0);

    /* Build a valid request and try again */
    gbinder_local_request_unref(req);
    req = gbinder_client_new_request2(ind, RADIO_IND_RIL_CONNECTED);
    gbinder_local_request_append_int32(req, RADIO_IND_ACK_EXP);
    g_assert_cmpint(gbinder_client_transact_sync_oneway(ind,
        RADIO_IND_RIL_CONNECTED, req), == ,GBINDER_STATUS_OK);

    /* This time both handler and observer are notified, ack is sent */
    g_assert_cmpint(code[0], == ,RADIO_IND_NONE);
    g_assert_cmpint(code[1], == ,RADIO_IND_NONE);
    g_assert_cmpint(test_service_req_count(&service,
        RADIO_REQ_RESPONSE_ACKNOWLEDGEMENT), == ,1);

    /* Now issue callStateChanged but only observe it (don't handle) */
    radio_instance_remove_handlers(radio, id, 1);
    code[1] = RADIO_IND_CALL_STATE_CHANGED;
    g_assert_cmpint(gbinder_client_transact_sync_oneway(ind,
        RADIO_IND_CALL_STATE_CHANGED, req), == ,GBINDER_STATUS_OK);
    g_assert_cmpint(code[1], == ,RADIO_IND_NONE);
    g_assert_cmpint(test_service_req_count(&service,
        RADIO_REQ_RESPONSE_ACKNOWLEDGEMENT), == ,2); /* Unhandled but acked */

    /* Same thing but without ack */
    gbinder_local_request_unref(req);
    req = gbinder_client_new_request2(ind, RADIO_IND_CALL_STATE_CHANGED);
    gbinder_local_request_append_int32(req, RADIO_IND_UNSOLICITED);
    code[1] = RADIO_IND_CALL_STATE_CHANGED;
    g_assert_cmpint(gbinder_client_transact_sync_oneway(ind,
        RADIO_IND_CALL_STATE_CHANGED, req), == ,GBINDER_STATUS_OK);
    g_assert_cmpint(code[1], == ,RADIO_IND_NONE);
    g_assert_cmpint(test_service_req_count(&service,
        RADIO_REQ_RESPONSE_ACKNOWLEDGEMENT), == ,2); /* Still 2 (not acked) */

    /* Unsupported interface */
    gbinder_local_request_unref(req);
    req = test_gbinder_local_request_new("foo");
    g_assert_cmpint(gbinder_client_transact_sync_oneway(ind,
        UNKNOWN_IND, req), == ,GBINDER_STATUS_FAILED);
    gbinder_local_request_unref(req);
    gbinder_client_unref(ind);

    radio_instance_remove_all_handlers(radio, id);
    radio_instance_unref(radio);
    test_service_cleanup(&service);
    gbinder_remote_object_unref(remote);
    gbinder_servicemanager_unref(sm);
}

/*==========================================================================*
 * req
 *==========================================================================*/

#define TEST_REQ RADIO_REQ_DIAL

static
void
test_req_observe(
    RadioInstance* radio,
    RADIO_REQ code,
    GBinderLocalRequest* args,
    gpointer user_data)
{
    int* count = user_data;

    (*count)++;
    GDEBUG_("%d", *count);
}

static
void
test_req(
    void)
{
    const char* slot = "slot1";
    const char* fqname = RADIO_1_0 "/slot1";
    TestRadioService service;
    GBinderServiceManager* sm = gbinder_servicemanager_new(DEV);
    GBinderRemoteObject* remote;
    GBinderLocalRequest* req;
    RadioInstance* radio;
    int count[3];
    gulong id[3];

    test_service_init(&service);
    remote = test_gbinder_servicemanager_new_service(sm, fqname, service.obj);
    radio = radio_instance_new_with_version(DEV, slot, RADIO_INTERFACE_1_4);

    memset(count, 0, sizeof(count));
    id[0] = radio_instance_add_request_observer(radio, RADIO_REQ_ANY,
        test_req_observe, count + 0);
    id[1] = radio_instance_add_request_observer(radio, TEST_REQ,
        test_req_observe, count + 1);
    id[2] = radio_instance_add_request_observer(radio, TEST_REQ + 1,
        test_req_observe, count + 2); /* Won't be called */
    g_assert(id[0]);
    g_assert(id[1]);
    g_assert(id[2]);

    req = radio_instance_new_request(radio, TEST_REQ);
    gbinder_local_request_append_int32(req, 123);
    g_assert(radio_instance_send_request_sync(radio, TEST_REQ, req));
    g_assert_cmpint(test_service_req_count(&service, TEST_REQ), == ,1);
    g_assert_cmpint(count[0], == ,1);
    g_assert_cmpint(count[1], == ,1);
    g_assert_cmpint(count[2], == ,0);
    radio_instance_remove_all_handlers(radio, id);

    radio_instance_unref(radio);
    test_service_cleanup(&service);
    gbinder_local_request_unref(req);
    gbinder_remote_object_unref(remote);
    gbinder_servicemanager_unref(sm);
}

/*==========================================================================*
 * resp
 *==========================================================================*/

#define TEST_RESP RADIO_RESP_DIAL

static
void
test_resp_ack_observe(
    RadioInstance* radio,
    RADIO_REQ code,
    GBinderLocalRequest* args,
    gpointer user_data)
{
    int* count = user_data;

    (*count)++;
    GDEBUG_("%d", *count);
    g_assert_cmpint(code, == ,RADIO_REQ_RESPONSE_ACKNOWLEDGEMENT);
}

static
void
test_resp_observe1(
    RadioInstance* radio,
    RADIO_RESP code,
    const RadioResponseInfo* info,
    const GBinderReader* reader,
    gpointer user_data)
{
    guint* expected = user_data;

    GDEBUG("Observing resp %u (high prio)", code);
    g_assert_cmpuint(info->serial, == ,*expected);
    *expected = 0;
}

static
void
test_resp_observe2(
    RadioInstance* radio,
    RADIO_RESP code,
    const RadioResponseInfo* info,
    const GBinderReader* reader,
    gpointer user_data)
{
    guint* expected = user_data;

    /* Serial must be already cleared by test_resp_observe1 */
    GDEBUG("Observing resp %u (default prio)", code);
    g_assert_cmpuint(*expected, == ,0);
}

static
gboolean
test_resp_handle(
    RadioInstance* radio,
    RADIO_RESP code,
    const RadioResponseInfo* info,
    const GBinderReader* reader,
    gpointer user_data)
{
    guint* expected = user_data;

    GDEBUG("Handling resp %u", code);
    g_assert_cmpuint(info->serial, == ,*expected);
    *expected = 0;
    if (info->type == RADIO_RESP_SOLICITED_ACK_EXP) {
        radio_instance_ack(radio);
    }
    return TRUE;
}

static
void
test_resp(
    void)
{
    const char* slot = "slot2";
    const char* fqname = RADIO_1_0 "/slot2";
    TestRadioService service;
    GBinderServiceManager* sm = gbinder_servicemanager_new(DEV);
    GBinderRemoteObject* remote;
    GBinderLocalRequest* req;
    GBinderClient* resp;
    RadioInstance* radio;
    RadioResponseInfo info;
    GBinderWriter writer;
    guint handle_serial, observe_serial;
    int ack_count = 0;
    gulong id[4];

    test_service_init(&service);

    memset(&info, 0, sizeof(info));
    info.type = RADIO_RESP_SOLICITED_ACK_EXP;
    handle_serial = observe_serial = info.serial = 123;

    remote = test_gbinder_servicemanager_new_service(sm, fqname, service.obj);
    radio = radio_instance_new_with_version(DEV, slot, RADIO_INTERFACE_1_4);
    id[0] = radio_instance_add_response_handler(radio, TEST_RESP,
        test_resp_handle, &handle_serial);
    id[1] = radio_instance_add_response_observer_with_priority(radio,
        RADIO_INSTANCE_PRIORITY_HIGHEST, TEST_RESP,
        test_resp_observe1, &observe_serial);
    id[2] = radio_instance_add_response_observer(radio, TEST_RESP,
        test_resp_observe2, &observe_serial);
    id[3] = radio_instance_add_request_observer(radio,
        RADIO_REQ_RESPONSE_ACKNOWLEDGEMENT, test_resp_ack_observe, &ack_count);

    g_assert(service.resp_obj);
    resp = gbinder_client_new2(service.resp_obj,
        TEST_ARRAY_AND_COUNT(radio_resp_iface_info));

    /* Submit broken respose first (without info) */
    req = gbinder_client_new_request2(resp, TEST_RESP);
    g_assert_cmpint(gbinder_client_transact_sync_oneway(resp, TEST_RESP, req),
        == ,GBINDER_STATUS_OK);
    g_assert_cmpint(ack_count, == ,0);
    g_assert_cmpint(test_service_req_count(&service,
        RADIO_REQ_RESPONSE_ACKNOWLEDGEMENT), == ,ack_count);

    /* Add the info and try again */
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_buffer_object(&writer, &info, sizeof(info));
    g_assert_cmpint(gbinder_client_transact_sync_oneway(resp, TEST_RESP, req),
        == ,GBINDER_STATUS_OK);
    g_assert_cmpint(ack_count, == ,1);
    g_assert_cmpint(test_service_req_count(&service,
        RADIO_REQ_RESPONSE_ACKNOWLEDGEMENT), == ,ack_count);
    g_assert(!handle_serial); /* Cleared by the handler */
    g_assert(!observe_serial); /* Cleared by the observer */

    /* Remove the handler and check auto-ack */
    radio_instance_remove_handlers(radio, id, 1);
    handle_serial = observe_serial = info.serial = 124;

    gbinder_local_request_unref(req);
    req = gbinder_client_new_request2(resp, TEST_RESP);
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_buffer_object(&writer, &info, sizeof(info));

    g_assert_cmpint(gbinder_client_transact_sync_oneway(resp, TEST_RESP, req),
        == ,GBINDER_STATUS_OK);
    g_assert_cmpint(ack_count, == ,2);  /* Acked */
    g_assert_cmpint(test_service_req_count(&service,
        RADIO_REQ_RESPONSE_ACKNOWLEDGEMENT), == ,ack_count);
    g_assert_cmpuint(handle_serial, == ,info.serial); /* No handler */
    g_assert(!observe_serial); /* Cleared by the observer */

    /* RADIO_RESP_SOLICITED won't be acked */
    info.type = RADIO_RESP_SOLICITED;
    handle_serial = observe_serial = info.serial = 125;

    gbinder_local_request_unref(req);
    req = gbinder_client_new_request2(resp, TEST_RESP);
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_buffer_object(&writer, &info, sizeof(info));

    g_assert_cmpint(gbinder_client_transact_sync_oneway(resp, TEST_RESP, req),
        == ,GBINDER_STATUS_OK);
    g_assert_cmpint(ack_count, == ,2); /* Not acked */
    g_assert_cmpint(test_service_req_count(&service,
        RADIO_REQ_RESPONSE_ACKNOWLEDGEMENT), == ,ack_count);
    g_assert_cmpuint(handle_serial, == ,info.serial); /* No handler */
    g_assert(!observe_serial); /* Cleared by the observer */

    /* Unsupported interface */
    gbinder_local_request_unref(req);
    req = test_gbinder_local_request_new("foo");
    g_assert_cmpint(gbinder_client_transact_sync_oneway(resp, TEST_RESP, req),
        == ,GBINDER_STATUS_FAILED);
    g_assert_cmpint(test_service_req_count(&service,
        RADIO_REQ_RESPONSE_ACKNOWLEDGEMENT), == ,2); /* Didn't change */

    gbinder_local_request_unref(req);
    gbinder_client_unref(resp);

    radio_instance_remove_all_handlers(radio, id);
    radio_instance_unref(radio);
    test_service_cleanup(&service);
    gbinder_remote_object_unref(remote);
    gbinder_servicemanager_unref(sm);
}

/*==========================================================================*
 * ack
 *==========================================================================*/

static
void
test_ack_cb(
    RadioInstance* radio,
    guint32 serial,
    gpointer user_data)
{
    guint* expected = user_data;

    GDEBUG("ack %u", serial);
    g_assert_cmpuint(serial, == ,*expected);
    *expected = 0;
}

static
void
test_ack(
    void)
{
    const char* slot = "slot1";
    const char* fqname = RADIO_1_0 "/slot1";
    TestRadioService service;
    GBinderServiceManager* sm = gbinder_servicemanager_new(DEV);
    GBinderRemoteObject* remote;
    GBinderLocalRequest* req;
    GBinderClient* resp;
    RadioInstance* radio;
    guint serial = 123;
    gulong id[3];

    test_service_init(&service);
    remote = test_gbinder_servicemanager_new_service(sm, fqname, service.obj);
    radio = radio_instance_new_with_version(DEV, slot, RADIO_INTERFACE_1_4);
    id[0] = radio_instance_add_ack_handler(radio, test_ack_cb, &serial);
    id[1] = radio_instance_add_response_handler(radio, RADIO_RESP_ANY,
        test_response_not_handled, NULL);
    id[2] = radio_instance_add_response_observer(radio,
        RADIO_RESP_ACKNOWLEDGE_REQUEST, test_response_not_observed, NULL);

    g_assert(service.resp_obj);
    resp = gbinder_client_new2(service.resp_obj,
        TEST_ARRAY_AND_COUNT(radio_resp_iface_info));

    /* Submit broken ack first (without serial) */
    req = gbinder_client_new_request2(resp, RADIO_RESP_ACKNOWLEDGE_REQUEST);
    g_assert_cmpint(gbinder_client_transact_sync_oneway(resp,
        RADIO_RESP_ACKNOWLEDGE_REQUEST, req), == ,GBINDER_STATUS_OK);
    g_assert(serial); /* Transaction succeeds but handler is not called */

    /* Add the serial and try again */
    gbinder_local_request_append_int32(req, serial);
    g_assert_cmpint(gbinder_client_transact_sync_oneway(resp,
        RADIO_RESP_ACKNOWLEDGE_REQUEST, req), == ,GBINDER_STATUS_OK);
    g_assert(!serial); /* Cleared by the handler */
    gbinder_local_request_unref(req);
    gbinder_client_unref(resp);

    radio_instance_remove_all_handlers(radio, id);
    radio_instance_unref(radio);
    test_service_cleanup(&service);
    gbinder_remote_object_unref(remote);
    gbinder_servicemanager_unref(sm);
}

/*==========================================================================*
 * send_req
 *==========================================================================*/

static
void
test_send_req_complete_cb(
    RadioInstance* instance,
    gulong id,
    int status,
    void* user_data1,
    void* user_data2)
{
    gulong* expected_id = user_data1;

    GDEBUG("tx %lu completed", id);
    g_assert_cmpuint(id, == ,*expected_id);
    *expected_id = 0;
    test_quit_later((GMainLoop*)user_data2);
}

static
void
test_send_req_destroy_cb(
    void* user_data1,
    void* user_data2)
{
    gulong* id = user_data1;

    GDEBUG("tx %lu done", *id);
    g_assert(*id);
    *id = 0;
    test_quit_later((GMainLoop*)user_data2);
}

static
void
test_send_req_complete_not_reached(
    RadioInstance* instance,
    gulong id,
    int status,
    void* user_data1,
    void* user_data2)
{
    g_assert_not_reached();
}

static
void
test_send_req_destroy_not_reached(
    void* user_data1,
    void* user_data2)
{
    g_assert_not_reached();
}

static
void
test_send_req(
    void)
{
    const char* slot = "slot1";
    const char* fqname = RADIO_1_0 "/slot1";
    TestRadioService service;
    GMainLoop* loop = g_main_loop_new(NULL, FALSE);
    GBinderServiceManager* sm = gbinder_servicemanager_new(DEV);
    GBinderRemoteObject* remote;
    GBinderLocalRequest* req;
    RadioInstance* radio;
    guint serial = 123;
    gulong tx;

    test_service_init(&service);
    remote = test_gbinder_servicemanager_new_service(sm, fqname, service.obj);
    radio = radio_instance_new_with_version(DEV, slot, RADIO_INTERFACE_1_4);

    /* Submit and wait for the completion callback to be invoked */
    req = radio_instance_new_request(radio, RADIO_REQ_GET_MUTE);
    gbinder_local_request_append_int32(req, serial);
    tx = radio_instance_send_request(radio, RADIO_REQ_GET_MUTE, req,
        test_send_req_complete_cb, NULL, &tx, loop);
    gbinder_local_request_unref(req);
    g_assert(tx);
    GDEBUG("tx %lu submitted", tx);

    test_run(&test_opt, loop);
    g_assert(!tx); /* Cleared by the completion handler */

    /* Submit and wait for the destroy callback to be invoked */
    serial = 124;
    req = radio_instance_new_request(radio, RADIO_REQ_GET_MUTE);
    gbinder_local_request_append_int32(req, serial);
    tx = radio_instance_send_request(radio, RADIO_REQ_GET_MUTE, req,
        NULL, test_send_req_destroy_cb, &tx, loop);
    gbinder_local_request_unref(req);
    g_assert(tx);
    GDEBUG("tx %lu submitted", tx);

    test_run(&test_opt, loop);
    g_assert(!tx); /* Cleared by the destroy callback */

    /* Submit, cancel and wait for the destroy callback to be invoked */
    serial = 125;
    req = radio_instance_new_request(radio, RADIO_REQ_GET_MUTE);
    gbinder_local_request_append_int32(req, serial);
    tx = radio_instance_send_request(radio, RADIO_REQ_GET_MUTE, req,
       test_send_req_complete_not_reached, test_send_req_destroy_cb,
       &tx, loop);
    gbinder_local_request_unref(req);
    g_assert(tx);
    GDEBUG("canceling tx %lu and waiting for destroy callback", tx);
    radio_instance_cancel_request(radio, tx);

    test_run(&test_opt, loop);
    g_assert(!tx); /* Cleared by the destroy callback */

    /* Submit without callbacks and cancel */
    req = radio_instance_new_request(radio, RADIO_REQ_GET_MUTE);
    gbinder_local_request_append_int32(req, serial);
    tx = radio_instance_send_request(radio, RADIO_REQ_GET_MUTE, req,
        NULL, NULL, NULL, NULL);
    gbinder_local_request_unref(req);
    g_assert(tx);
    GDEBUG("canceling tx %lu", tx);
    radio_instance_cancel_request(radio, tx);

    /* radio_instance_send_request() fails if the remote is dead */
    test_gbinder_remote_object_kill(remote);
    req = radio_instance_new_request(radio, RADIO_REQ_GET_MUTE);
    gbinder_local_request_append_int32(req, serial);
    tx = radio_instance_send_request(radio, RADIO_REQ_GET_MUTE, req,
        NULL, test_send_req_destroy_not_reached, NULL, NULL);
    gbinder_local_request_unref(req);
    g_assert(!tx);

    radio_instance_unref(radio);
    test_service_cleanup(&service);
    gbinder_remote_object_unref(remote);
    gbinder_servicemanager_unref(sm);
    g_main_loop_unref(loop);
}

/*==========================================================================*
 * enabled
 *==========================================================================*/

static
void
test_enabled_cb(
    RadioInstance* radio,
    gpointer user_data)
{
    GDEBUG("%sabled", radio->enabled ? "En" : "Dis");
    (*((int*)user_data))++;
}

static
void
test_enabled(
    void)
{
    const char* slot = "slot1";
    const char* fqname = RADIO_1_0 "/slot1";
    TestRadioService service;
    GBinderServiceManager* sm = gbinder_servicemanager_new(DEV);
    GBinderRemoteObject* remote;
    RadioInstance* radio;
    int n = 0;
    gulong id;

    test_service_init(&service);
    remote = test_gbinder_servicemanager_new_service(sm, fqname, service.obj);
    radio = radio_instance_new_with_version(DEV, slot, RADIO_INTERFACE_1_4);
    id = radio_instance_add_enabled_handler(radio, test_enabled_cb, &n);

    g_assert(id);
    g_assert(!radio->enabled);
    radio_instance_set_enabled(radio, TRUE);
    g_assert_cmpint(n, == ,1);
    g_assert(radio->enabled);

    radio_instance_set_enabled(radio, TRUE);
    g_assert(radio->enabled);
    g_assert_cmpint(n, == ,1); /* Nothing changed */

    radio_instance_set_enabled(radio, FALSE);
    g_assert_cmpint(n, == ,2);
    g_assert(!radio->enabled);

    radio_instance_remove_handler(radio, id);
    radio_instance_unref(radio);
    test_service_cleanup(&service);
    gbinder_remote_object_unref(remote);
    gbinder_servicemanager_unref(sm);
}

/*==========================================================================*
 * death
 *==========================================================================*/

static
void
test_death_cb(
    RadioInstance* radio,
    gpointer user_data)
{
    GDEBUG("Boom");
    (*((int*)user_data))++;
}

static
void
test_death(
    void)
{
    const char* slot = "slot1";
    const char* fqname = RADIO_1_0 "/slot1";
    TestRadioService service;
    GBinderServiceManager* sm = gbinder_servicemanager_new(DEV);
    GBinderRemoteObject* remote;
    RadioInstance* radio;
    int n = 0;
    gulong id;

    test_service_init(&service);
    remote = test_gbinder_servicemanager_new_service(sm, fqname, service.obj);
    radio = radio_instance_new_with_version(DEV, slot, RADIO_INTERFACE_1_4);
    id = radio_instance_add_death_handler(radio, test_death_cb, &n);

    g_assert(id);
    g_assert(!radio_instance_is_dead(radio));
    test_gbinder_remote_object_kill(remote);
    g_assert_cmpint(n, == ,1);
    g_assert(radio_instance_is_dead(radio));

    radio_instance_remove_handler(radio, 0); /* no effect */
    radio_instance_remove_handler(radio, id);
    radio_instance_unref(radio);
    test_service_cleanup(&service);
    gbinder_remote_object_unref(remote);
    gbinder_servicemanager_unref(sm);
}

/*==========================================================================*
 * Common
 *==========================================================================*/

#define TEST_PREFIX "/instance/"
#define TEST_(t) TEST_PREFIX t

int main(int argc, char* argv[])
{
    g_test_init(&argc, &argv, NULL);
    g_test_add_func(TEST_("null"), test_null);
    g_test_add_func(TEST_("basic"), test_basic);
    g_test_add_func(TEST_("connected"), test_connected);
    g_test_add_func(TEST_("ind"), test_ind);
    g_test_add_func(TEST_("req"), test_req);
    g_test_add_func(TEST_("resp"), test_resp);
    g_test_add_func(TEST_("ack"), test_ack);
    g_test_add_func(TEST_("send_req"), test_send_req);
    g_test_add_func(TEST_("enabled"), test_enabled);
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
