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

#include "radio_client.h"
#include "radio_instance.h"
#include "radio_request_p.h"
#include "radio_request_group_p.h"
#include "radio_util.h"

#include <gutil_strv.h>
#include <gutil_log.h>

#define DEFAULT_INTERFACE RADIO_INTERFACE_1_0
#define DEV GBINDER_DEFAULT_HWBINDER

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
void
test_ind_not_reached(
    RadioClient* client,
    RADIO_IND code,
    const GBinderReader* reader,
    gpointer user_data)
{
    g_assert_not_reached();
}

static
void
test_complete_not_reached(
    RadioRequest* req,
    RADIO_TX_STATUS status,
    RADIO_RESP resp,
    RADIO_ERROR error,
    const GBinderReader* reader,
    gpointer user_data)
{
    g_assert_not_reached();
}

static
gboolean
test_retry_not_reached(
    RadioRequest* req,
    RADIO_TX_STATUS status,
    RADIO_RESP resp,
    RADIO_ERROR error,
    const GBinderReader* reader,
    void* user_data)
{
    g_assert_not_reached();
    return FALSE;
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
test_inc_cb(
    gpointer user_data)
{
    (*((int*)user_data))++;
}

static
void
test_client_inc_cb(
    RadioClient* client,
    gpointer user_data)
{
    (*((int*)user_data))++;
}

static
gboolean
test_unref_request_later(
    gpointer user_data)
{
    radio_request_unref((RadioRequest*)user_data);
    return G_SOURCE_REMOVE;
}

/*==========================================================================*
 * Test IRadio service
 *==========================================================================*/

typedef struct test_radio_service {
    GBinderLocalObject* obj;
    GBinderClient* resp_client;
    GBinderClient* ind_client;
    GHashTable* req_count;
    gboolean mute;
} TestRadioService;

#define FAIL_REQ RADIO_REQ_GET_ICC_CARD_STATUS
#define ERROR_REQ RADIO_REQ_GET_IMS_REGISTRATION_STATE
#define ERROR_RESP RADIO_RESP_GET_IMS_REGISTRATION_STATE
#define IGNORE_REQ RADIO_REQ_GET_SIGNAL_STRENGTH

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
        if (code == RADIO_REQ_SET_RESPONSE_FUNCTIONS) {
            GBinderRemoteObject* resp_obj = gbinder_reader_read_object(&reader);
            GBinderRemoteObject* ind_obj = gbinder_reader_read_object(&reader);

            g_assert(resp_obj);
            g_assert(ind_obj);
            gbinder_client_unref(service->resp_client);
            gbinder_client_unref(service->ind_client);
            service->resp_client = gbinder_client_new2(resp_obj,
                TEST_ARRAY_AND_COUNT(radio_resp_iface_info));
            service->ind_client = gbinder_client_new2(ind_obj,
                TEST_ARRAY_AND_COUNT(radio_ind_iface_info));
            gbinder_remote_object_unref(resp_obj);
            gbinder_remote_object_unref(ind_obj);
        } else if (code == FAIL_REQ) {
            GDEBUG("failing request transaction");
            *status = GBINDER_STATUS_FAILED;
            return NULL;
        } else if (code == IGNORE_REQ) {
            GDEBUG("ignoring request transaction");
        } else if (code != RADIO_REQ_RESPONSE_ACKNOWLEDGEMENT) {
            RadioResponseInfo info;
            GBinderWriter writer;
            GBinderLocalRequest* resp = gbinder_client_new_request2
                (service->resp_client, radio_req_resp(code));

            memset(&info, 0, sizeof(info));
            info.type = RADIO_RESP_SOLICITED;
            info.error = (code == ERROR_REQ) ?
                RADIO_ERROR_GENERIC_FAILURE :
                RADIO_ERROR_NONE;
            g_assert(gbinder_reader_read_uint32(&reader, &info.serial));

            g_assert(resp);
            gbinder_local_request_init_writer(resp, &writer);
            gbinder_writer_append_buffer_object(&writer, &info, sizeof(info));

            switch (code) {
            case RADIO_REQ_GET_MUTE:
                gbinder_writer_append_bool(&writer, service->mute);
                g_assert(gbinder_client_transact(service->resp_client,
                    RADIO_RESP_GET_MUTE, GBINDER_TX_FLAG_ONEWAY,
                    resp, NULL, NULL, NULL));
                break;
            case RADIO_REQ_GET_IMS_REGISTRATION_STATE:
                gbinder_writer_append_bool(&writer, FALSE);
                g_assert(gbinder_client_transact(service->resp_client,
                    RADIO_RESP_GET_IMS_REGISTRATION_STATE,
                    GBINDER_TX_FLAG_ONEWAY, resp, NULL, NULL, NULL));
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
    gbinder_client_unref(service->resp_client);
    gbinder_client_unref(service->ind_client);
    gbinder_local_object_unref(service->obj);
    memset(service, 0, sizeof(*service));
}

/*==========================================================================*
 * Common setup for all tests
 *==========================================================================*/

typedef struct test_common {
    TestRadioService service;
    GBinderServiceManager* sm;
    GBinderRemoteObject* remote;
    RadioInstance* radio;
    RadioClient* client;
} TestCommon;

static
RadioClient*
test_common_init(
    TestCommon* test)
{
    memset(test, 0, sizeof(*test));
    test->sm = gbinder_servicemanager_new(DEV);
    test_service_init(&test->service);
    test->remote = test_gbinder_servicemanager_new_service(test->sm,
        RADIO_1_0 "/slot1", test->service.obj);
    test->radio = radio_instance_new_with_version(DEV, "slot1",
        RADIO_INTERFACE_1_0);
    test->client = radio_client_new(test->radio);
    g_assert(test->client);
    return test->client;
}

static
void
test_common_connected(
    TestCommon* test)
{
    const RADIO_IND ind = RADIO_IND_RIL_CONNECTED;
    GBinderClient* ind_client  = test->service.ind_client;
    GBinderLocalRequest* req = gbinder_client_new_request2(ind_client, ind);

    gbinder_local_request_append_int32(req, RADIO_IND_ACK_EXP);
    g_assert_cmpint(gbinder_client_transact_sync_oneway(ind_client, ind, req),
        == ,GBINDER_STATUS_OK);
    gbinder_local_request_unref(req);
    g_assert(radio_client_connected(test->client));
}

static
void
test_common_cleanup(
    TestCommon* test)
{
    radio_client_unref(test->client);
    radio_instance_unref(test->radio);
    test_service_cleanup(&test->service);
    gbinder_remote_object_unref(test->remote);
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
RadioClient*
test_simple_init(
    TestSimple* test)
{
    memset(test, 0, sizeof(*test));
    test->loop = g_main_loop_new(NULL, FALSE);
    return test_common_init(&test->common);
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

static
void
test_simple_complete_fail_cb(
    RadioRequest* req,
    RADIO_TX_STATUS status,
    RADIO_RESP resp,
    RADIO_ERROR error,
    const GBinderReader* reader,
    gpointer user_data)
{
    TestSimple* test = user_data;

    GDEBUG("status %u", status);
    g_assert_cmpint(status, == ,RADIO_TX_STATUS_FAILED);
    g_assert_cmpint(resp, == ,RADIO_RESP_NONE);
    g_assert_cmpint(error, == ,RADIO_ERROR_NONE);
    g_assert(!test->completed);
    g_assert(!test->destroyed);
    test->completed = TRUE;
}

static
void
test_simple_complete_error_cb(
    RadioRequest* req,
    RADIO_TX_STATUS status,
    RADIO_RESP resp,
    RADIO_ERROR error,
    const GBinderReader* reader,
    gpointer user_data)
{
    TestSimple* test = user_data;

    GDEBUG("error %d", error);
    g_assert_cmpint(status, == ,RADIO_TX_STATUS_OK);
    g_assert_cmpint(resp, == ,ERROR_RESP);
    g_assert_cmpint(error, == ,RADIO_ERROR_GENERIC_FAILURE);
    g_assert(!test->completed);
    g_assert(!test->destroyed);
    test->completed = TRUE;
}

/*==========================================================================*
 * null
 *==========================================================================*/

static
void
test_null(
    void)
{
    radio_client_unref(NULL);
    radio_client_set_default_timeout(NULL, 0);
    radio_client_remove_handler(NULL, 0);
    radio_client_remove_handler(NULL, 1);
    radio_client_remove_handlers(NULL, NULL, 0);

    g_assert(!radio_client_new(NULL));
    g_assert(!radio_client_ref(NULL));
    g_assert(!radio_client_slot(NULL));
    g_assert(!radio_client_connected(NULL));
    g_assert(radio_client_dead(NULL));
    g_assert(!radio_client_add_indication_handler(NULL, 0, NULL, NULL));
    g_assert(!radio_client_add_owner_changed_handler(NULL, NULL, NULL));
    g_assert(!radio_client_add_death_handler(NULL, NULL, NULL));
    g_assert(!radio_client_add_connected_handler(NULL, NULL, NULL));
    g_assert_cmpint(radio_client_interface(NULL), == ,RADIO_INTERFACE_NONE);

    radio_request_unref(NULL);
    radio_request_drop(NULL);
    radio_request_cancel(NULL);
    radio_request_set_blocking(NULL, FALSE);
    radio_request_set_timeout(NULL, 0);
    radio_request_set_retry(NULL, 0, 0);
    radio_request_set_retry_func(NULL, NULL);

    g_assert(!radio_request_new(NULL, 0, NULL, NULL, NULL, NULL));
    g_assert(!radio_request_new2(NULL, 0, NULL, NULL, NULL, NULL));
    g_assert(!radio_request_ref(NULL));
    g_assert(!radio_request_submit(NULL));
    g_assert(!radio_request_retry(NULL));

    radio_request_group_cancel(NULL);
    radio_request_group_unref(NULL);
    radio_request_group_unblock(NULL);

    g_assert(!radio_request_group_new(NULL));
    g_assert(!radio_request_group_ref(NULL));
    g_assert_cmpint(radio_request_group_block(NULL), ==, RADIO_BLOCK_NONE);
    g_assert_cmpint(radio_request_group_block_status(NULL), ==,
        RADIO_BLOCK_NONE);
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
    RadioRequest* req;
    RadioClient* client = test_common_init(&test);
    gboolean destroyed = FALSE;

    g_assert_cmpstr(radio_client_slot(client), == ,test.radio->slot);
    g_assert_cmpint(radio_client_interface(client), == , test.radio->version);
    g_assert(!radio_client_connected(client));

    /* Adding NULL handler is a nop */
    g_assert(!radio_client_add_indication_handler(client, RADIO_IND_ANY,
        NULL, NULL));
    g_assert(!radio_client_add_owner_changed_handler(client, NULL, NULL));
    g_assert(!radio_client_add_death_handler(client, NULL, NULL));
    g_assert(!radio_client_add_connected_handler(client, NULL, NULL));

    /* Create a request */
    req = radio_request_new(client, RADIO_REQ_GET_MUTE, NULL, NULL, NULL, NULL);
    g_assert(req);

    /* Test ref/unref */
    g_assert(radio_request_ref(req) == req);
    g_assert(radio_client_ref(client) == client);
    radio_client_unref(client);

    /* These are OK to cancel more than once */
    radio_request_cancel(req);
    radio_request_cancel(req);
    g_assert_cmpint(req->state, == ,RADIO_REQUEST_STATE_CANCELLED);
    radio_request_drop(req);  /* Matches radio_request_ref() above */
    radio_request_drop(req);  /* Releases the final ref */

    /* Make sure destroy callback is invoked */
    req = radio_request_new(client, RADIO_REQ_GET_MUTE, NULL, NULL,
        test_destroy_once, &destroyed);
    radio_request_unref(req);
    g_assert(destroyed);

    /* Request survives the client but can't be submitted */
    destroyed = FALSE;
    req = radio_request_new(client, RADIO_REQ_GET_MUTE, NULL, NULL,
        test_destroy_once, &destroyed);
    radio_client_unref(client);
    test.client = NULL;

    g_assert(!destroyed);
    g_assert(!radio_request_submit(req));
    radio_request_set_timeout(req, 100); /* No effect, the client is gone */
    radio_request_drop(req);

    test_common_cleanup(&test);
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
    RadioRequest* req;
    gboolean destroyed = FALSE;

    test_common_init(&test);
    req = radio_request_new(test.client, RADIO_REQ_GET_MUTE, NULL,
        test_complete_not_reached, test_destroy_once, &destroyed);

    g_assert(radio_request_submit(req));
    radio_request_cancel(req);
    g_assert_cmpint(req->state, == ,RADIO_REQUEST_STATE_CANCELLED);
    radio_request_unref(req);

    g_assert(destroyed);
    test_common_cleanup(&test);
}

/*==========================================================================*
 * cancelq
 *==========================================================================*/

static
void
test_cancel_queue(
    void)
{
    TestCommon test;
    RadioRequest* req[3];
    gboolean destroyed[G_N_ELEMENTS(req)];
    RadioRequestGroup* group = radio_request_group_new(test_common_init(&test));
    guint i;

    /* Make sure these requests get queued */
    g_assert_cmpint(radio_request_group_block(group), ==,
        RADIO_BLOCK_ACQUIRED);

    for (i = 0; i < G_N_ELEMENTS(req); i++) {
        destroyed[i] = FALSE;
        req[i] = radio_request_new(test.client, RADIO_REQ_GET_MUTE, NULL,
            test_complete_not_reached, test_destroy_once, destroyed + i);
        g_assert(radio_request_submit(req[i]));
    }

    radio_request_cancel(req[0]);
    radio_request_cancel(req[2]);
    radio_request_cancel(req[1]);

    for (i = 0; i < G_N_ELEMENTS(req); i++) {
        g_assert_cmpint(req[i]->state, == ,RADIO_REQUEST_STATE_CANCELLED);
        radio_request_unref(req[i]);
        g_assert(destroyed[i]);
    }

    radio_request_group_unref(group);
    test_common_cleanup(&test);
}

/*==========================================================================*
 * ind
 *==========================================================================*/

typedef struct test_ind_data {
    TestCommon common;
    GMainLoop* loop;
    RADIO_IND ind;
    RADIO_IND ind2;
} TestInd;

static
void
test_ind_cb(
    RadioClient* client,
    RADIO_IND code,
    const GBinderReader* reader,
    gpointer user_data)
{
    TestInd* test = user_data;

    g_assert_cmpint(test->ind, == ,RADIO_IND_NONE);
    g_assert_cmpint(code, == ,RADIO_IND_RIL_CONNECTED);
    GDEBUG("%d", code);
    test->ind = code;
}

static
void
test_ind_cb2(
    RadioClient* client,
    RADIO_IND code,
    const GBinderReader* reader,
    gpointer user_data)
{
    TestInd* test = user_data;

    g_assert_cmpint(test->ind2, == ,RADIO_IND_NONE);
    g_assert_cmpint(code, == ,RADIO_IND_RIL_CONNECTED);
    test->ind2 = code;
    GDEBUG("%d (done)", code);
    test_quit_later(test->loop);
}

static
void
test_ind(
    void)
{
    GBinderLocalRequest* req;
    GBinderClient* ind_client;
    RadioClient* client;
    TestInd test;
    int connected  = 0;
    gulong id[4];

    memset(&test, 0, sizeof(test));
    test_common_init(&test.common);
    test.loop = g_main_loop_new(NULL, FALSE);

    client = test.common.client;
    ind_client = test.common.service.ind_client;

    /* Register and unregister one listener */
    id[0] = radio_client_add_indication_handler(client, RADIO_IND_ANY,
        test_ind_not_reached, NULL);
    radio_client_remove_handler(client, id[0]);

    /* Register actual listeners */
    id[0] = radio_client_add_indication_handler(client, RADIO_IND_ANY,
        test_ind_cb, &test);
    id[1] = radio_client_add_indication_handler(client, RADIO_IND_RIL_CONNECTED,
        test_ind_cb2, &test);
    id[2] = radio_client_add_indication_handler(client,
        RADIO_IND_CALL_STATE_CHANGED, test_ind_not_reached, NULL);
    id[3] = radio_client_add_connected_handler(client,
        test_client_inc_cb, &connected);

    /* Submit the indication */
    req = gbinder_client_new_request2(ind_client, RADIO_IND_RIL_CONNECTED);
    gbinder_local_request_append_int32(req, RADIO_IND_ACK_EXP);
    g_assert_cmpint(gbinder_client_transact(ind_client, RADIO_IND_RIL_CONNECTED,
        GBINDER_TX_FLAG_ONEWAY, req, NULL, NULL, NULL), != ,0);
    gbinder_local_request_unref(req);

    /* And wait for test_ind_cb2 to terminate the loop */
    test_run(&test_opt, test.loop);
    g_assert_cmpint(test.ind, == ,RADIO_IND_RIL_CONNECTED);
    g_assert_cmpint(test.ind2, == ,RADIO_IND_RIL_CONNECTED);
    g_assert_cmpint(connected, == ,1);

    /* Cleanup */
    radio_client_remove_all_handlers(client, id);
    g_main_loop_unref(test.loop);
    test_common_cleanup(&test.common);
}

/*==========================================================================*
 * resp
 *==========================================================================*/

static
void
test_resp_complete_cb(
    RadioRequest* req,
    RADIO_TX_STATUS status,
    RADIO_RESP resp,
    RADIO_ERROR error,
    const GBinderReader* reader,
    gpointer user_data)
{
    TestSimple* test = user_data;

    GDEBUG("resp %d", resp);
    g_assert_cmpint(status, == ,RADIO_TX_STATUS_OK);
    g_assert_cmpint(resp, == ,RADIO_RESP_GET_MUTE);
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
    GBinderLocalRequest* ack;
    GBinderLocalRequest* resp;
    GBinderWriter writer;
    RadioResponseInfo info;
    TestSimple test;
    RadioClient* client = test_simple_init(&test);
    GBinderClient* resp_client = test.common.service.resp_client;
    RadioRequest* req = radio_request_new(client, RADIO_REQ_GET_MUTE, NULL,
        test_resp_complete_cb, test_simple_destroy_cb, &test);

    g_assert(req);
    g_assert(req->serial);
    radio_request_submit(req);
    radio_request_submit(req); /* Second submit is ignored */
    test_common_connected(&test.common);

    /* Ack the request*/
    ack = gbinder_client_new_request2(resp_client,
        RADIO_RESP_ACKNOWLEDGE_REQUEST);
    gbinder_local_request_append_int32(ack, req->serial);
    g_assert_cmpint(gbinder_client_transact_sync_oneway(resp_client,
        RADIO_RESP_ACKNOWLEDGE_REQUEST, ack), == ,GBINDER_STATUS_OK);
    g_assert(req->acked);

    /* Release our ref, the internal one will be dropped when req is done */
    radio_request_unref(req);

    test_run(&test_opt, test.loop);

    g_assert(test.completed);
    g_assert(test.destroyed);

    /* This ack is invalid and will be ignored */
    g_assert_cmpint(gbinder_client_transact_sync_oneway(resp_client,
        RADIO_RESP_ACKNOWLEDGE_REQUEST, ack), == ,GBINDER_STATUS_OK);
    gbinder_local_request_unref(ack);

    /* This response will be ignored */
    memset(&info, 0, sizeof(info));
    info.type = RADIO_RESP_SOLICITED;
    info.serial = 123;

    resp = gbinder_client_new_request2(resp_client, RADIO_RESP_GET_MUTE);
    g_assert(resp);
    gbinder_local_request_init_writer(resp, &writer);
    gbinder_writer_append_buffer_object(&writer, &info, sizeof(info));
    gbinder_writer_append_bool(&writer, test.common.service.mute);
    g_assert_cmpint(gbinder_client_transact_sync_oneway(resp_client,
        RADIO_RESP_GET_MUTE, resp), == ,GBINDER_STATUS_OK);
    gbinder_local_request_unref(resp);

    /* Cleanup */
    test_simple_cleanup(&test);
}

/*==========================================================================*
 * group
 *==========================================================================*/

static
void
test_group(
    void)
{
    TestCommon test;
    RadioRequest* req;
    RadioClient* client = test_common_init(&test);
    RadioRequestGroup* group = radio_request_group_new(client);
    RadioRequestGroup* group2;
    gulong id;
    int destroyed = 0;
    int blocked = 0;

    g_assert(group);
    test_common_connected(&test);

    /* Test ref/unref */
    g_assert(radio_request_group_ref(group) == group);
    radio_request_group_unref(group);

    /* Block status */
    g_assert_cmpint(radio_request_group_block_status(group), ==,
        RADIO_BLOCK_NONE);
    id = radio_client_add_owner_changed_handler(client,
        test_client_inc_cb, &blocked);
    g_assert(id);
    g_assert_cmpint(radio_request_group_block(group), ==,
        RADIO_BLOCK_ACQUIRED);
    g_assert_cmpint(blocked, == ,1);
    g_assert_cmpint(radio_request_group_block(group), ==,
        RADIO_BLOCK_ACQUIRED);
    g_assert_cmpint(blocked, == ,1);
    g_assert_cmpint(radio_request_group_block_status(group), ==,
        RADIO_BLOCK_ACQUIRED);

    /* Second group can't become an owner */
    group2 = radio_request_group_new(client);
    g_assert(group2);
    g_assert_cmpint(radio_request_group_block(group2), ==,
        RADIO_BLOCK_QUEUED);
    g_assert_cmpint(radio_request_group_block(group2), ==,
        RADIO_BLOCK_QUEUED);
    g_assert_cmpint(blocked, == ,1);
    g_assert_cmpint(radio_request_group_block_status(group2), ==,
        RADIO_BLOCK_QUEUED);
    radio_request_group_unref(group2);

    /* Create two requests */
    req = radio_request_new2(group, RADIO_REQ_GET_MUTE, NULL,
        test_complete_not_reached, test_inc_cb, &destroyed);
    g_assert(req);
    radio_request_submit(req);
    radio_request_unref(req);

    req = radio_request_new2(group, RADIO_REQ_GET_MUTE, NULL,
        test_complete_not_reached, test_inc_cb, &destroyed);
    g_assert(req);
    radio_request_submit(req);
    radio_request_unref(req);

    /* And cancel them all in one shot */
    g_assert(!destroyed);
    radio_request_group_cancel(group);
    g_assert_cmpint(destroyed, == ,2);

    /* Cancel another one explicitly */
    req = radio_request_new2(group, RADIO_REQ_GET_MUTE, NULL,
        test_complete_not_reached, test_inc_cb, &destroyed);
    g_assert(req);
    radio_request_cancel(req);
    g_assert_cmpint(req->state, == ,RADIO_REQUEST_STATE_CANCELLED);
    radio_request_unref(req);
    g_assert_cmpint(destroyed, == ,3);

    /* Cleanup */
    radio_request_group_unblock(group);
    radio_request_group_unref(group);
    test_common_cleanup(&test);
}

/*==========================================================================*
 * group2
 *==========================================================================*/

static
void
test_group2(
    void)
{
    TestCommon test;
    RadioRequest* req;
    RadioRequestGroup* group;
    gulong id;
    int destroyed = 0;
    int blocked = 0;

    test_common_init(&test);
    test_common_connected(&test);

    group = radio_request_group_new(test.client);
    g_assert(group);

    /* Submit a request */
    req = radio_request_new2(group, RADIO_REQ_GET_MUTE, NULL,
        test_complete_not_reached, test_inc_cb, &destroyed);
    g_assert(req);
    radio_request_submit(req);
    g_assert_cmpint(req->state, == ,RADIO_REQUEST_STATE_PENDING);

    /* The group immediately becomes an owner because the pending
     * request is associated with it */
    g_assert_cmpint(radio_request_group_block_status(group), ==,
        RADIO_BLOCK_NONE);
    id = radio_client_add_owner_changed_handler(test.client,
        test_client_inc_cb, &blocked);
    g_assert(id);
    g_assert_cmpint(radio_request_group_block(group), ==,
        RADIO_BLOCK_ACQUIRED);
    g_assert_cmpint(blocked, == ,1);

    /* Destroying the group drops the ownership */
    radio_request_group_unref(group);
    g_assert_cmpint(blocked, == ,2);
    g_assert(!destroyed);

    /* Cancel the request */
    radio_request_cancel(req);
    g_assert_cmpint(req->state, == ,RADIO_REQUEST_STATE_CANCELLED);
    radio_request_unref(req);
    g_assert_cmpint(destroyed, == ,1);

    /* Cleanup */
    test_common_cleanup(&test);
}

/*==========================================================================*
 * group3
 *==========================================================================*/

typedef struct test_group3_data {
    TestCommon common;
    GMainLoop* loop;
    int completed;
    int destroyed;
} TestGroup3;

static
void
test_group3_complete_cb(
    RadioRequest* req,
    RADIO_TX_STATUS status,
    RADIO_RESP resp,
    RADIO_ERROR error,
    const GBinderReader* reader,
    gpointer user_data)
{
    TestGroup3* test = user_data;

    g_assert_cmpint(status, == ,RADIO_TX_STATUS_OK);
    g_assert_cmpint(resp, == ,RADIO_RESP_GET_MUTE);
    g_assert_cmpint(error, == ,RADIO_ERROR_NONE);
    g_assert(!test->completed);
    g_assert(!test->destroyed);
    g_assert(!req->group);
    test->completed++;
    GDEBUG("completion %d", test->completed);
}

static
void
test_group3_complete2_cb(
    RadioRequest* req,
    RADIO_TX_STATUS status,
    RADIO_RESP resp,
    RADIO_ERROR error,
    const GBinderReader* reader,
    gpointer user_data)
{
    TestGroup3* test = user_data;

    g_assert_cmpint(status, == ,RADIO_TX_STATUS_OK);
    g_assert_cmpint(resp, == ,RADIO_RESP_GET_MUTE);
    g_assert_cmpint(error, == ,RADIO_ERROR_NONE);
    g_assert(req->group);
    test->completed++;
    GDEBUG("completion %d", test->completed);
}

static
void
test_group3_destroy_continue_cb(
    gpointer user_data)
{
    TestGroup3* test = user_data;

    test->destroyed++;
    g_assert_cmpint(test->destroyed, == ,test->completed);
    GDEBUG("destruction %d", test->destroyed);
    test_quit_later(test->loop);
}

static
void
test_group3(
    void)
{
    TestGroup3 test;
    RadioRequest* req;
    RadioRequest* req1;
    RadioRequest* req2;
    RadioRequest* req3;
    RadioRequestGroup* group1;
    RadioRequestGroup* group2;
    RadioRequestGroup* group3;
    RadioClient* client;
    int owner_changed = 0;
    gulong id;

    memset(&test, 0, sizeof(test));
    test_common_init(&test.common);
    test_common_connected(&test.common);
    test.loop = g_main_loop_new(NULL, FALSE);
    client = test.common.client;

    group1 = radio_request_group_new(client);
    g_assert(group1);

    /* This one will prevent the group from immediately becoming the owner */
    req = radio_request_new(client, RADIO_REQ_GET_MUTE, NULL,
        test_group3_complete_cb, test_group3_destroy_continue_cb, &test);
    g_assert(req);
    radio_request_submit(req);
    g_assert_cmpint(req->state, == ,RADIO_REQUEST_STATE_PENDING);
    radio_request_unref(req);

    /* No group can become the owner until pending request gets completed */
    g_assert_cmpint(radio_request_group_block_status(group1), ==,
        RADIO_BLOCK_NONE);
    id = radio_client_add_owner_changed_handler(client,
        test_client_inc_cb, &owner_changed);
    g_assert(id);
    g_assert_cmpint(radio_request_group_block(group1), ==,
        RADIO_BLOCK_QUEUED);
    g_assert_cmpint(radio_request_group_block_status(group1), ==,
        RADIO_BLOCK_QUEUED);
    g_assert_cmpint(owner_changed, == ,0);

    group2 = radio_request_group_new(client);
    g_assert(group2);
    g_assert_cmpint(radio_request_group_block(group2), ==,
        RADIO_BLOCK_QUEUED);
    g_assert_cmpint(radio_request_group_block(group2), ==,
        RADIO_BLOCK_QUEUED);
    g_assert_cmpint(radio_request_group_block_status(group2), ==,
        RADIO_BLOCK_QUEUED);

    group3 = radio_request_group_new(client);
    g_assert(group3);
    g_assert_cmpint(radio_request_group_block(group3), ==,
        RADIO_BLOCK_QUEUED);
    g_assert_cmpint(radio_request_group_block_status(group3), ==,
        RADIO_BLOCK_QUEUED);

    /* Create more requests (in the opposite order) */
    req3 = radio_request_new2(group3, RADIO_REQ_GET_MUTE, NULL,
        test_group3_complete2_cb, test_group3_destroy_continue_cb, &test);
    g_assert(req3);
    radio_request_submit(req3);
    g_assert_cmpint(req3->state, == ,RADIO_REQUEST_STATE_QUEUED);

    req2 = radio_request_new2(group2, RADIO_REQ_GET_MUTE, NULL,
        test_group3_complete2_cb, test_group3_destroy_continue_cb, &test);
    g_assert(req2);
    radio_request_submit(req2);
    g_assert_cmpint(req2->state, == ,RADIO_REQUEST_STATE_QUEUED);

    req1 = radio_request_new2(group1, RADIO_REQ_GET_MUTE, NULL,
        test_group3_complete2_cb, test_group3_destroy_continue_cb, &test);
    g_assert(req1);
    radio_request_submit(req1);
    g_assert_cmpint(req1->state, == ,RADIO_REQUEST_STATE_QUEUED);

    /* Wait for the first request to complete */
    test_run(&test_opt, test.loop);

    /* The first group becomes the owner */
    g_assert_cmpint(owner_changed, == ,1);
    g_assert_cmpint(radio_request_group_block_status(group1), ==,
        RADIO_BLOCK_ACQUIRED);

    /* And its request gets submitted */
    g_assert_cmpint(req1->state, > ,RADIO_REQUEST_STATE_QUEUED);
    g_idle_add(test_unref_request_later, req1);
    test_run(&test_opt, test.loop);

    /* Freeing the group allows the next one to become the owner */
    g_assert_cmpint(radio_request_group_block_status(group2), ==,
        RADIO_BLOCK_QUEUED);
    g_assert_cmpint(owner_changed, == ,1);
    radio_request_group_unref(group1);
    g_assert_cmpint(owner_changed, == ,2);
    g_assert_cmpint(radio_request_group_block_status(group2), ==,
        RADIO_BLOCK_ACQUIRED);

    /* And its request gets submitted */
    g_assert_cmpint(req2->state, > ,RADIO_REQUEST_STATE_QUEUED);
    g_idle_add(test_unref_request_later, req2);
    test_run(&test_opt, test.loop);

    /* Drop group2 */
    g_assert_cmpint(radio_request_group_block_status(group3), ==,
        RADIO_BLOCK_QUEUED);
    g_assert_cmpint(owner_changed, == ,2);
    radio_request_group_unblock(group2);
    g_assert_cmpint(owner_changed, == ,3);
    g_assert_cmpint(radio_request_group_block_status(group3), ==,
        RADIO_BLOCK_ACQUIRED);
    radio_request_group_unref(group2);

    /* Wait for the last request to complete */
    g_assert_cmpint(req3->state, > ,RADIO_REQUEST_STATE_QUEUED);
    g_idle_add(test_unref_request_later, req3);
    test_run(&test_opt, test.loop);

    /* Cleanup */
    radio_request_group_unref(group3);
    g_main_loop_unref(test.loop);
    test_common_cleanup(&test.common);
}

/*==========================================================================*
 * block
 *==========================================================================*/

typedef struct test_block_data {
    TestCommon common;
    GMainLoop* loop;
    int completed;
    int destroyed;
} TestBlock;

static
void
test_block_complete_cb(
    RadioRequest* req,
    RADIO_TX_STATUS status,
    RADIO_RESP resp,
    RADIO_ERROR error,
    const GBinderReader* reader,
    gpointer user_data)
{
    TestBlock* test = user_data;

    g_assert_cmpint(status, == ,RADIO_TX_STATUS_OK);
    g_assert_cmpint(resp, == ,RADIO_RESP_GET_MUTE);
    g_assert_cmpint(error, == ,RADIO_ERROR_NONE);
    g_assert(req->blocking || test->completed > 0);
    test->completed++;
    GDEBUG("completion %u", test->completed);
    g_assert_cmpint(test->destroyed, < ,test->completed);
}

static
void
test_block_destroy_cb(
    gpointer user_data)
{
    TestBlock* test = user_data;

    g_assert(test->completed > 0);
    g_assert_cmpint(test->destroyed, < ,test->completed);
    test->destroyed++;
    GDEBUG("destruction %u", test->destroyed);
    if (test->destroyed == 3) {
        test_quit_later(test->loop);
    }
}

static
void
test_block(
    void)
{
    RadioClient* client;
    RadioRequest* req;
    TestBlock test;

    memset(&test, 0, sizeof(test));
    test_common_init(&test.common);
    test_common_connected(&test.common);
    test.loop = g_main_loop_new(NULL, FALSE);
    client = test.common.client;

    /* One blocking request */
    req = radio_request_new(client, RADIO_REQ_GET_MUTE, NULL,
        test_block_complete_cb, test_block_destroy_cb, &test);
    g_assert(req);
    g_assert(req->serial);
    radio_request_set_blocking(req, TRUE);
    radio_request_submit(req);
    g_assert_cmpint(req->state, == ,RADIO_REQUEST_STATE_PENDING);
    radio_request_unref(req);

    /* And two non-blocking */
    req = radio_request_new(client, RADIO_REQ_GET_MUTE, NULL,
        test_block_complete_cb, test_block_destroy_cb, &test);
    g_assert(req);
    g_assert(req->serial);
    radio_request_submit(req);
    g_assert_cmpint(req->state, == ,RADIO_REQUEST_STATE_QUEUED);
    radio_request_unref(req);

    req = radio_request_new(client, RADIO_REQ_GET_MUTE, NULL,
        test_block_complete_cb, test_block_destroy_cb, &test);
    g_assert(req);
    g_assert(req->serial);
    radio_request_submit(req);
    g_assert_cmpint(req->state, == ,RADIO_REQUEST_STATE_QUEUED);
    radio_request_unref(req);

    test_run(&test_opt, test.loop);

    g_assert_cmpint(test.completed, == ,3);
    g_assert_cmpint(test.destroyed, == ,3);

    /* Cleanup */
    g_main_loop_unref(test.loop);
    test_common_cleanup(&test.common);
}

/*==========================================================================*
 * block_timeout
 *==========================================================================*/

static
void
test_block_timeout_complete1_cb(
    RadioRequest* req,
    RADIO_TX_STATUS status,
    RADIO_RESP resp,
    RADIO_ERROR error,
    const GBinderReader* reader,
    gpointer user_data)
{
    TestSimple* test = user_data;

    g_assert_cmpint(status, == ,RADIO_TX_STATUS_TIMEOUT);
    g_assert_cmpint(resp, == ,RADIO_RESP_NONE);
    g_assert_cmpint(error, == ,RADIO_ERROR_NONE);
    g_assert(req->blocking);
    g_assert(!test->completed);
    test->completed++;
    GDEBUG("block timed out");
}

static
void
test_block_timeout_complete2_cb(
    RadioRequest* req,
    RADIO_TX_STATUS status,
    RADIO_RESP resp,
    RADIO_ERROR error,
    const GBinderReader* reader,
    gpointer user_data)
{
    TestSimple* test = user_data;

    g_assert_cmpint(status, == ,RADIO_TX_STATUS_OK);
    g_assert_cmpint(resp, == ,RADIO_RESP_GET_MUTE);
    g_assert_cmpint(error, == ,RADIO_ERROR_NONE);
    g_assert_cmpint(test->completed, == ,1);
    test->completed++;
    GDEBUG("second request completed");
}

static
void
test_block_timeout_destroy_cb(
    gpointer user_data)
{
    TestSimple* test = user_data;

    test->destroyed++;
    GDEBUG("destruction %u", test->destroyed);
    g_assert_cmpint(test->completed, == ,test->destroyed);
    if (test->destroyed == 2) {
        test_quit_later(test->loop);
    }
}

static
void
test_block_timeout(
    void)
{
    TestSimple test;
    RadioClient* client = test_simple_init(&test);
    RadioRequest* req = radio_request_new(client, IGNORE_REQ, NULL,
        test_block_timeout_complete1_cb, test_block_timeout_destroy_cb, &test);

    test_common_connected(&test.common);

    /* Blocking request */
    g_assert(req);
    g_assert(req->serial);
    radio_request_set_blocking(req, TRUE);
    radio_request_set_timeout(req, 100);
    radio_request_submit(req);
    g_assert_cmpint(req->state, == ,RADIO_REQUEST_STATE_PENDING);
    radio_request_unref(req);

    /* And non-blocking */
    req = radio_request_new(client, RADIO_REQ_GET_MUTE, NULL,
        test_block_timeout_complete2_cb, test_block_timeout_destroy_cb, &test);
    g_assert(req);
    g_assert(req->serial);
    radio_request_submit(req);
    g_assert_cmpint(req->state, == ,RADIO_REQUEST_STATE_QUEUED);
    radio_request_unref(req);

    /* And wait for both request to get destroyed */
    test_run(&test_opt, test.loop);

    g_assert_cmpint(test.completed, == ,2);
    g_assert_cmpint(test.destroyed, == ,2);

    /* Cleanup */
    test_simple_cleanup(&test);
}

/*==========================================================================*
 * retry
 *==========================================================================*/

#define TEST_RETRY_COUNT 2

static
void
test_retry_complete_cb(
    RadioRequest* req,
    RADIO_TX_STATUS status,
    RADIO_RESP resp,
    RADIO_ERROR error,
    const GBinderReader* reader,
    gpointer user_data)
{
    TestSimple* test = user_data;

    GDEBUG("status %u, retry count %d", status, req->retry_count);
    g_assert_cmpuint(req->serial, != ,req->serial2);
    g_assert_cmpuint(req->retry_count, == ,TEST_RETRY_COUNT);
    g_assert_cmpint(status, == ,RADIO_TX_STATUS_OK);
    g_assert_cmpint(resp, == ,ERROR_RESP);
    g_assert_cmpint(error, == ,RADIO_ERROR_GENERIC_FAILURE);
    g_assert(!test->completed);
    g_assert(!test->destroyed);
    test->completed = TRUE;
}

static
void
test_retry(
    void)
{
    TestSimple test;
    RadioClient* client = test_simple_init(&test);
    RadioRequest* req = radio_request_new(client, ERROR_REQ, NULL,
        test_retry_complete_cb, test_simple_destroy_cb, &test);

    test_common_connected(&test.common);

    radio_request_set_retry_func(req, NULL); /* Use the default */
    radio_request_set_retry(req, 10, TEST_RETRY_COUNT);
    radio_request_submit(req);
    radio_request_unref(req);

    test_run(&test_opt, test.loop);

    g_assert(test.completed);
    g_assert(test.destroyed);

    /* Cleanup */
    test_simple_cleanup(&test);
}

static
void
test_retry2(
    void)
{
    int i;
    TestSimple test;
    RadioClient* client = test_simple_init(&test);
    RadioRequest* req = radio_request_new(client, ERROR_REQ, NULL,
        test_retry_complete_cb, test_simple_destroy_cb, &test);

    test_common_connected(&test.common);

    /* Long timeout (longer than the test timeout) */
    radio_request_set_timeout(req, TEST_TIMEOUT_MS * 2);
    radio_request_set_retry(req, 10, TEST_RETRY_COUNT);
    g_assert(radio_request_submit(req));
    g_assert_cmpint(req->state, == ,RADIO_REQUEST_STATE_PENDING);

    /* Use all retries */
    for (i = 0; i < TEST_RETRY_COUNT; i++) {
        g_assert(radio_request_retry(req));
        g_assert_cmpint(req->state, == ,RADIO_REQUEST_STATE_PENDING);
    }

    g_assert(!radio_request_retry(req));
    radio_request_unref(req);

    test_run(&test_opt, test.loop);

    g_assert(test.completed);
    g_assert(test.destroyed);

    /* Cleanup */
    test_simple_cleanup(&test);
}

/*==========================================================================*
 * fail
 *==========================================================================*/

static
void
test_fail(
    void)
{
    TestCommon test;
    RadioRequest* req;
    gboolean destroyed = FALSE;

    test_common_init(&test);
    test_common_connected(&test);

    req = radio_request_new(test.client, RADIO_REQ_GET_MUTE, NULL,
        test_complete_not_reached, test_destroy_once, &destroyed);
    g_assert(req);
    g_assert(req->serial);

    /* Kill the remote object */
    g_assert(!test.radio->dead);
    test_gbinder_remote_object_kill(test.remote);
    g_assert(test.radio->dead);

    /* Submit stays in the NEW state since the remote object is dead */
    g_assert(!radio_request_submit(req));
    g_assert_cmpint(req->state, == ,RADIO_REQUEST_STATE_NEW);

    g_assert(!destroyed);
    radio_request_drop(req);
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
    RadioRequest* req;
    gboolean destroyed = FALSE;

    test_common_init(&test);
    test_common_connected(&test);

    req = radio_request_new(test.client, RADIO_REQ_GET_MUTE, NULL,
        test_complete_not_reached, test_destroy_once, &destroyed);
    g_assert(req);
    g_assert(req->serial);

    /* Fail one transaction */
    test_gbinder_client_tx_fail_count = 1;

    /* Request switches in the FAILED state */
    g_assert(!radio_request_submit(req));
    g_assert_cmpint(req->state, == ,RADIO_REQUEST_STATE_FAILED);

    g_assert(!destroyed);
    radio_request_drop(req);
    g_assert(destroyed);

    test_common_cleanup(&test);
}

/*==========================================================================*
 * err
 *==========================================================================*/

static
void
test_err(
    void)
{
    TestSimple test;
    RadioClient* client = test_simple_init(&test);
    RadioRequest* req = radio_request_new(client, FAIL_REQ, NULL,
        test_simple_complete_fail_cb, test_simple_destroy_cb, &test);

    g_assert(req);
    g_assert(req->serial);

    /* Just setting retry function has no effect until non-zero number of
     * retries is set, too. */
    radio_request_set_retry_func(req, test_retry_not_reached);
    radio_request_submit(req);
    radio_request_unref(req);

    test_common_connected(&test.common);
    test_run(&test_opt, test.loop);

    g_assert(test.completed);
    g_assert(test.destroyed);

    /* Cleanup */
    test_simple_cleanup(&test);
}

static
void
test_err2(
    void)
{
    TestSimple test;
    RadioClient* client = test_simple_init(&test);
    RadioRequest* req = radio_request_new(client, ERROR_REQ, NULL,
        test_simple_complete_error_cb, test_simple_destroy_cb, &test);

    g_assert(req);
    g_assert(req->serial);

    /* Will be retried once */
    radio_request_set_retry_func(req, NULL);
    radio_request_set_retry(req, 0, 1);
    g_assert(radio_request_submit(req));
    radio_request_unref(req);

    test_common_connected(&test.common);
    test_run(&test_opt, test.loop);

    g_assert(test.completed);
    g_assert(test.destroyed);

    /* Cleanup */
    test_simple_cleanup(&test);
}

/*==========================================================================*
 * timeout
 *==========================================================================*/

static
void
test_timeout_complete_cb(
    RadioRequest* req,
    RADIO_TX_STATUS status,
    RADIO_RESP resp,
    RADIO_ERROR error,
    const GBinderReader* reader,
    gpointer user_data)
{
    TestSimple* test = user_data;

    GDEBUG("status %u", status);
    g_assert_cmpint(status, == ,RADIO_TX_STATUS_TIMEOUT);
    g_assert_cmpint(resp, == ,RADIO_RESP_NONE);
    g_assert_cmpint(error, == ,RADIO_ERROR_NONE);
    g_assert(!test->completed);
    g_assert(!test->destroyed);
    test->completed = TRUE;
}

static
void
test_timeout(
    void)
{
    TestSimple test;
    RadioClient* client = test_simple_init(&test);
    RadioRequest* req = radio_request_new(client, IGNORE_REQ, NULL,
        test_timeout_complete_cb, test_simple_destroy_cb, &test);

    g_assert(req);
    g_assert(req->serial);
    radio_request_set_timeout(req, 2 * TEST_TIMEOUT_MS);
    radio_request_submit(req);
    radio_request_set_timeout(req, 100); /* Resets the timeout */
    radio_request_set_timeout(req, 100); /* Has no effect */
    radio_request_unref(req);

    /* And expect the request to fail */
    test_common_connected(&test.common);
    test_run(&test_opt, test.loop);

    g_assert(test.completed);
    g_assert(test.destroyed);

    /* Cleanup */
    test_simple_cleanup(&test);
}

/*==========================================================================*
 * timeout2
 *==========================================================================*/

static
void
test_timeout2(
    void)
{
    TestSimple test;
    RadioClient* client = test_simple_init(&test);
    /* NOTE: reusing test_timeout_complete_cb */
    RadioRequest* req = radio_request_new(client, IGNORE_REQ, NULL,
        test_timeout_complete_cb, test_simple_destroy_cb, &test);

    g_assert(req);
    g_assert(req->serial);
    radio_request_set_timeout(req, 2000 * TEST_TIMEOUT_SEC);
    radio_request_submit(req);
    radio_request_set_timeout(req, 100); /* Resets the timeout */
    radio_request_set_timeout(req, 100); /* Has no effect */
    radio_request_unref(req);

    /* And expect the request to fail */
    test_run(&test_opt, test.loop);

    g_assert(test.completed);
    g_assert(test.destroyed);

    /* Cleanup */
    test_simple_cleanup(&test);
}

/*==========================================================================*
 * timeout3
 *==========================================================================*/

static
void
test_timeout3_complete_cb(
    RadioRequest* req,
    RADIO_TX_STATUS status,
    RADIO_RESP resp,
    RADIO_ERROR error,
    const GBinderReader* reader,
    gpointer user_data)
{
    TestSimple* test = user_data;

    GDEBUG("status %u", status);
    g_assert_cmpuint(req->retry_count, == ,1);
    g_assert_cmpint(status, == ,RADIO_TX_STATUS_TIMEOUT);
    g_assert_cmpint(resp, == ,RADIO_RESP_NONE);
    g_assert_cmpint(error, == ,RADIO_ERROR_NONE);
    g_assert(!test->completed);
    g_assert(!test->destroyed);
    test->completed = TRUE;
    test_quit_later(test->loop);
}

static
void
test_timeout3_destroy_cb(
    gpointer user_data)
{
    TestSimple* test = user_data;

    GDEBUG("done");
    g_assert(test->completed);
    g_assert(!test->destroyed);
    test->destroyed = TRUE;
}

static
void
test_timeout3(
    void)
{
    TestSimple test;
    RadioClient* client = test_simple_init(&test);
    RadioRequest* req = radio_request_new(client, ERROR_REQ, NULL,
        test_timeout3_complete_cb, test_timeout3_destroy_cb, &test);

    g_assert(req);
    g_assert(req->serial);
    radio_request_set_retry(req, TEST_TIMEOUT_MS, 2);
    radio_request_set_timeout(req, 2 * TEST_TIMEOUT_MS);
    radio_request_submit(req);
    radio_request_set_timeout(req, 100); /* Resets the timeout */
    radio_request_set_timeout(req, 100); /* Has no effect */

    /* And expect the request to fail */
    test_common_connected(&test.common);
    test_run(&test_opt, test.loop);

    g_assert(test.completed);
    g_assert(!test.destroyed);
    radio_request_unref(req);
    g_assert(test.destroyed);

    /* Cleanup */
    test_simple_cleanup(&test);
}

/*==========================================================================*
 * timeout4
 *==========================================================================*/

static
void
test_timeout4(
    void)
{
    TestSimple test;
    RadioRequest* req1;
    RadioRequest* req2;
    RadioClient* client = test_simple_init(&test);

    test_common_connected(&test.common);

    /* No effect, already using the default value */
    radio_client_set_default_timeout(client, 0);

    /* This request will use long timeout which won't expire */
    req1 = radio_request_new(client, IGNORE_REQ, NULL,
        test_complete_not_reached, NULL, NULL);
    g_assert(req1);
    g_assert(req1->serial);
    radio_request_set_timeout(req1, TEST_TIMEOUT_MS * 2);
    radio_request_submit(req1);

    /* And this one will time out quickly */
    req2 = radio_request_new(client, IGNORE_REQ, NULL,
        test_timeout_complete_cb, test_simple_destroy_cb, &test);
    g_assert(req2);
    g_assert(req2->serial);
    radio_request_submit(req2);
    radio_request_unref(req2);
    radio_client_set_default_timeout(client, 100);
    radio_client_set_default_timeout(client, 100); /* Has no effect */

    /* Wait for the secong request to time out */
    test_run(&test_opt, test.loop);

    g_assert(test.completed);
    g_assert(test.destroyed);

    /* The first one is still pending */
    g_assert_cmpint(req1->state, == ,RADIO_REQUEST_STATE_PENDING);
    radio_request_cancel(req1);
    g_assert_cmpint(req1->state, == ,RADIO_REQUEST_STATE_CANCELLED);
    g_assert(!radio_request_retry(req1));
    radio_request_unref(req1);

    /* Cleanup */
    test_simple_cleanup(&test);
}

/*==========================================================================*
 * destroy
 *==========================================================================*/

static
void
test_destroy(
    void)
{
    TestSimple test;
    RadioClient* client = test_simple_init(&test);
    RadioRequest* req = radio_request_new(client, RADIO_REQ_GET_MUTE, NULL,
        test_simple_complete_fail_cb, test_simple_destroy_cb, &test);

    g_assert(req);
    g_assert(req->serial);
    radio_request_submit(req);
    radio_request_unref(req);

    /* Destroy the client without waiting for request to complete */
    radio_client_unref(client);
    test.common.client = NULL;

    /* And expect it to fail */
    test_run(&test_opt, test.loop);

    g_assert(test.completed);
    g_assert(test.destroyed);

    /* Cleanup */
    test_simple_cleanup(&test);
}

/*==========================================================================*
 * death
 *==========================================================================*/

#define TEST_DEATH_REQ_COUNT 5

static
void
test_death_destroy_cb(
    gpointer user_data)
{
    TestSimple* test = user_data;

    test->destroyed++;
    GDEBUG("req %d done", test->destroyed);
    g_assert_cmpint(test->completed, == ,test->destroyed);
    if (test->destroyed == TEST_DEATH_REQ_COUNT) {
        test_quit_later(test->loop);
    }
}

static
void
test_death_complete_cb(
    RadioRequest* req,
    RADIO_TX_STATUS status,
    RADIO_RESP resp,
    RADIO_ERROR error,
    const GBinderReader* reader,
    gpointer user_data)
{
    TestSimple* test = user_data;

    test->completed++;
    GDEBUG("req %d status %u", test->completed, status);
    g_assert_cmpint(status, == ,RADIO_TX_STATUS_FAILED);
    g_assert_cmpint(resp, == ,RADIO_RESP_NONE);
    g_assert_cmpint(error, == ,RADIO_ERROR_NONE);
}

static
void
test_death(
    void)
{
    TestSimple test;
    RadioClient* client = test_simple_init(&test);
    RadioRequest* req = radio_request_new(client, RADIO_REQ_GET_MUTE, NULL,
        test_death_complete_cb, test_death_destroy_cb, &test);
    int i, death_count = 0;
    gulong id = radio_client_add_death_handler(client, test_client_inc_cb,
        &death_count);

    /* Make this one blocking */
    radio_request_set_blocking(req, TRUE);
    g_assert(req);
    g_assert(req->serial);
    g_assert(radio_request_submit(req));
    radio_request_unref(req);

    /* And let these sit in the queue */
    for (i = 1; i < TEST_DEATH_REQ_COUNT; i++) {
        req = radio_request_new(client, RADIO_REQ_GET_MUTE, NULL,
            test_death_complete_cb, test_death_destroy_cb, &test);
        g_assert(req);
        g_assert(req->serial);
        g_assert(radio_request_submit(req));
        radio_request_unref(req);
    }

    /* Kill the remote object */
    g_assert(!test.common.radio->dead);
    g_assert(!radio_client_dead(client));
    test_gbinder_remote_object_kill(test.common.remote);
    g_assert(test.common.radio->dead);
    g_assert(radio_client_dead(client));
    g_assert_cmpint(death_count, == ,1);

    /* And expect all requests to fail */
    test_run(&test_opt, test.loop);

    g_assert_cmpint(test.completed, == ,TEST_DEATH_REQ_COUNT);
    g_assert_cmpint(test.destroyed, == ,TEST_DEATH_REQ_COUNT);
    g_assert_cmpint(death_count, == ,1);

    /* This one is going to fail immediatelt because the client is dead */
    req = radio_request_new(client, RADIO_REQ_GET_MUTE, NULL,
        test_complete_not_reached, NULL, NULL);
    g_assert(req);
    g_assert(req->serial);
    g_assert(!radio_request_submit(req));
    radio_request_drop(req);

     /* Cleanup */
    radio_client_remove_handler(client, id);
    test_simple_cleanup(&test);
}

/*==========================================================================*
 * Common
 *==========================================================================*/

#define TEST_PREFIX "/client/"
#define TEST_(t) TEST_PREFIX t

int main(int argc, char* argv[])
{
    g_test_init(&argc, &argv, NULL);
    g_test_add_func(TEST_("null"), test_null);
    g_test_add_func(TEST_("basic"), test_basic);
    g_test_add_func(TEST_("cancel"), test_cancel);
    g_test_add_func(TEST_("cancel_queue"), test_cancel_queue);
    g_test_add_func(TEST_("ind"), test_ind);
    g_test_add_func(TEST_("resp"), test_resp);
    g_test_add_func(TEST_("group"), test_group);
    g_test_add_func(TEST_("group2"), test_group2);
    g_test_add_func(TEST_("group3"), test_group3);
    g_test_add_func(TEST_("block"), test_block);
    g_test_add_func(TEST_("block_timeout"), test_block_timeout);
    g_test_add_func(TEST_("retry"), test_retry);
    g_test_add_func(TEST_("retry2"), test_retry2);
    g_test_add_func(TEST_("fail"), test_fail);
    g_test_add_func(TEST_("fail_tx"), test_fail_tx);
    g_test_add_func(TEST_("err"), test_err);
    g_test_add_func(TEST_("err2"), test_err2);
    g_test_add_func(TEST_("timeout"), test_timeout);
    g_test_add_func(TEST_("timeout2"), test_timeout2);
    g_test_add_func(TEST_("timeout3"), test_timeout3);
    g_test_add_func(TEST_("timeout4"), test_timeout4);
    g_test_add_func(TEST_("death"), test_death);
    g_test_add_func(TEST_("destroy"), test_destroy);
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
