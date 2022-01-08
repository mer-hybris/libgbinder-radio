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

#include "test_gbinder.h"

#include <gutil_log.h>

#include <stdlib.h>

typedef struct test_gbinder_client_tx {
    GBinderClient* client;
    guint32 code;
    guint32 flags;
    GBinderLocalRequest* req;
    GBinderClientReplyFunc reply;
    GDestroyNotify destroy;
    void* user_data;
} TestGBinderClientTx;

typedef struct test_gbinder_client_iface_range {
    char* iface;
    GBytes* header;
    guint32 last_code;
} TestGBinderClientIfaceRange;

struct gbinder_client {
    guint32 refcount;
    GBinderRemoteObject* remote;
    TestGBinderClientIfaceRange* ranges;
    guint nr;
};

int test_gbinder_client_tx_fail_count = 0;

static
void
test_gbinder_client_free(
    GBinderClient* self)
{
    guint i;

    for (i = 0; i < self->nr; i++) {
        TestGBinderClientIfaceRange* r = self->ranges + i;

        g_bytes_unref(r->header);
        g_free(r->iface);
    }
    g_free(self->ranges);
    gbinder_remote_object_unref(self->remote);
    g_free(self);
}

static
void
test_gbinder_client_init_range(
    TestGBinderClientIfaceRange* r,
    const GBinderClientIfaceInfo* info)
{
    r->header = g_bytes_new(info->iface, strlen(info->iface));
    r->iface = g_strdup(info->iface);
    r->last_code = info->last_code;
}

static
int
test_gbinder_client_sort_ranges(
    const void* p1,
    const void* p2)
{
    const TestGBinderClientIfaceRange* r1 = p1;
    const TestGBinderClientIfaceRange* r2 = p2;

    return (r1->last_code < r2->last_code) ? (-1) :
        (r1->last_code > r2->last_code) ? 1 : 0;
}

static
const TestGBinderClientIfaceRange*
test_gbinder_client_find_range(
    GBinderClient* self,
    guint32 code)
{
    guint i;

    for (i = 0; i < self->nr; i++) {
        const TestGBinderClientIfaceRange* r = self->ranges + i;

        if (r->last_code >= code) {
            return r;
        }
    }
    return NULL;
}

static
GBinderRemoteReply*
test_gbinder_client_transact(
    GBinderClient* self,
    guint32 code,
    guint32 flags,
    GBinderLocalRequest* req,
    int* status)
{
    GBinderLocalObject* obj = test_gbinder_remote_object_to_local(self->remote);
    GBinderRemoteRequest* remote_req = test_gbinder_remote_request_new(req);
    GBinderLocalReply* reply = test_gbinder_local_object_handle_tx(obj,
        remote_req, code, flags, status);
    GBinderRemoteReply* remote_reply = test_gbinder_remote_reply_new(reply);

    gbinder_remote_request_unref(remote_req);
    gbinder_local_reply_unref(reply);
    return remote_reply;
}

static
GBinderRemoteReply*
test_gbinder_client_transact_sync(
    GBinderClient* self,
    guint32 code,
    guint32 flags,
    GBinderLocalRequest* req,
    int* status)
{
    GBinderRemoteReply* reply = NULL;

    if (self) {
        GBinderRemoteObject* obj = self->remote;

        if (!test_gbinder_remote_object_dead(obj)) {
            GBinderLocalRequest* tmp = NULL;

            if (!req) {
                const TestGBinderClientIfaceRange* r =
                    test_gbinder_client_find_range(self, code);

                if (r) {
                    req = tmp = test_gbinder_local_request_new(r->iface);
                }
            }
            if (req) {
                reply = test_gbinder_client_transact(self, code, flags, req,
                    status);
            }
            gbinder_local_request_unref(tmp);
        } else {
            GDEBUG("Refusing to perform transaction with a dead object");
        }
    }
    return reply;
}

static
gboolean
test_gbinder_client_tx_handle(
    gpointer data)
{
    int status = -1;
    TestGBinderClientTx* tx = data;
    GBinderRemoteReply* reply = test_gbinder_client_transact
        (tx->client, tx->code, tx->flags, tx->req, &status);

    if (tx->reply) {
        tx->reply(tx->client, reply, status, tx->user_data);
    }
    gbinder_remote_reply_unref(reply);
    return G_SOURCE_REMOVE;
}

static
void
test_gbinder_client_tx_destroy(
    gpointer data)
{
    TestGBinderClientTx* tx = data;

    if (tx->destroy) {
        tx->destroy(tx->user_data);
    }
    gbinder_local_request_unref(tx->req);
    gbinder_client_unref(tx->client);
    g_free(tx);
}

/*==========================================================================*
 * libgbinder API
 *==========================================================================*/

GBinderClient*
gbinder_client_new2(
    GBinderRemoteObject* remote,
    const GBinderClientIfaceInfo* ifaces,
    gsize count)
{
    if (remote) {
        GBinderClient* self = g_new0(GBinderClient, 1);

        g_atomic_int_set(&self->refcount, 1);
        self->remote = gbinder_remote_object_ref(remote);
        if (count > 0) {
            gsize i;

            self->nr = count;
            self->ranges = g_new(TestGBinderClientIfaceRange, self->nr);
            for (i = 0; i < count; i++) {
               test_gbinder_client_init_range(self->ranges + i, ifaces + i);
            }
            qsort(self->ranges, count, sizeof(TestGBinderClientIfaceRange),
                test_gbinder_client_sort_ranges);
        } else {
            /* No interface info */
            self->nr = 1;
            self->ranges = g_new0(TestGBinderClientIfaceRange, 1);
            self->ranges[0].last_code = UINT_MAX;
        }
        return self;
    }
    return NULL;
}

GBinderClient*
gbinder_client_ref(
    GBinderClient* self)
{
    if (self) {
        g_assert_cmpint(self->refcount, > ,0);
        g_atomic_int_inc(&self->refcount);
    }
    return self;
}

void
gbinder_client_unref(
    GBinderClient* self)
{
    if (self) {
        g_assert_cmpint(self->refcount, > ,0);
        if (g_atomic_int_dec_and_test(&self->refcount)) {
            test_gbinder_client_free(self);
        }
    }
}

GBytes*
gbinder_client_rpc_header(
    GBinderClient* self,
    guint32 code)
{
    if (self) {
        const TestGBinderClientIfaceRange* r =
            test_gbinder_client_find_range(self, code);

        if (r) {
            return r->header;
        }
    }
    return NULL;
}

GBinderLocalRequest*
gbinder_client_new_request2(
    GBinderClient* self,
    guint32 code)
{
    if (self) {
        const TestGBinderClientIfaceRange* r =
            test_gbinder_client_find_range(self, code);

        if (r) {
            return test_gbinder_local_request_new(r->iface);
        }
    }
    return NULL;
}

GBinderRemoteReply*
gbinder_client_transact_sync_reply(
    GBinderClient* self,
    guint32 code,
    GBinderLocalRequest* req,
    int* status)
{
    return test_gbinder_client_transact_sync(self, code, 0, req, status);
}

int
gbinder_client_transact_sync_oneway(
    GBinderClient* self,
    guint32 code,
    GBinderLocalRequest* req)
{
    int status = -1;

    g_assert(!test_gbinder_client_transact_sync(self, code,
        GBINDER_TX_FLAG_ONEWAY, req, &status));
    return status;
}

gulong
gbinder_client_transact(
    GBinderClient* self,
    guint32 code,
    guint32 flags,
    GBinderLocalRequest* req,
    GBinderClientReplyFunc reply,
    GDestroyNotify destroy,
    void* user_data)
{
    gulong id = 0;

    if (self) {
        GBinderRemoteObject* obj = self->remote;

        if (!test_gbinder_remote_object_dead(obj)) {
            if (test_gbinder_client_tx_fail_count) {
                if (test_gbinder_client_tx_fail_count > 0) {
                    test_gbinder_client_tx_fail_count--;
                }
                GDEBUG("Simulating transaction failure");
            } else {
                GBinderLocalRequest* tmp = NULL;

                if (!req) {
                    const TestGBinderClientIfaceRange* r =
                        test_gbinder_client_find_range(self, code);

                    if (r) {
                        req = tmp = test_gbinder_local_request_new(r->iface);
                    }
                }
                if (req) {
                    TestGBinderClientTx* tx = g_new0(TestGBinderClientTx, 1);

                    tx->client = gbinder_client_ref(self);
                    tx->code = code;
                    tx->flags = flags;
                    tx->req = gbinder_local_request_ref(req);
                    tx->reply = reply;
                    tx->destroy = destroy;
                    tx->user_data = user_data;
                    id = g_idle_add_full(G_PRIORITY_DEFAULT,
                        test_gbinder_client_tx_handle, tx,
                        test_gbinder_client_tx_destroy);
                }
                gbinder_local_request_unref(tmp);
            }
        } else {
            GDEBUG("Refusing to perform transaction with a dead object");
        }
    }
    return id;
}

void
gbinder_client_cancel(
    GBinderClient* self,
    gulong id)
{
    if (id) {
        g_source_remove((guint)id);
    }
}

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
