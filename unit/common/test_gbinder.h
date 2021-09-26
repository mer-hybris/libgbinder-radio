/*
 * Copyright (C) 2021 Jolla Ltd.
 * Copyright (C) 2021 Slava Monich <slava.monich@jolla.com>
 *
 * You may use this file under the terms of BSD license as follows:
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
 */

#ifndef TEST_GBINDER_H
#define TEST_GBINDER_H

#include <gbinder.h>

typedef struct test_gbinder_data TestGBinderData;

/* test_gbinder_reader_writer.c */

TestGBinderData*
test_gbinder_data_new(
    const char* iface);

TestGBinderData*
test_gbinder_data_ref(
    TestGBinderData* data);

void
test_gbinder_data_unref(
    TestGBinderData* data);

void
test_gbinder_data_add_int32(
    TestGBinderData* data,
    guint32 value);

void
test_gbinder_data_add_hidl_struct(
    TestGBinderData* data,
    const void* buf,
    gsize size);

void
test_gbinder_data_init_reader(
    TestGBinderData* data,
    GBinderReader* reader);

void
test_gbinder_data_init_writer(
    TestGBinderData* data,
    GBinderWriter* writer);

/* test_gbinder_local_request.c */

GBinderLocalRequest*
test_gbinder_local_request_new(
    const char* iface);

const char*
test_gbinder_local_request_interface(
    GBinderLocalRequest* local);

TestGBinderData*
test_gbinder_local_request_data(
    GBinderLocalRequest* local);

/* test_gbinder_local_reply.c */

GBinderLocalReply*
test_gbinder_local_reply_new(
    void);

TestGBinderData*
test_gbinder_local_reply_data(
    GBinderLocalReply* reply);

/* test_gbinder_remote_request.c */

GBinderRemoteRequest*
test_gbinder_remote_request_new(
    GBinderLocalRequest* req);

/* test_gbinder_remote_reply.c */

GBinderRemoteReply*
test_gbinder_remote_reply_new(
    GBinderLocalReply* reply);

/* test_gbinder_local_object.c */

GBinderLocalObject*
test_gbinder_local_object_new(
    const char* const* ifaces,
    GBinderLocalTransactFunc txproc,
    void* user_data);

GBinderLocalReply*
test_gbinder_local_object_handle_tx(
    GBinderLocalObject* self,
    GBinderRemoteRequest* req,
    guint code,
    guint flags,
    int* status);

/* test_gbinder_remote_object.c */

GBinderRemoteObject*
test_gbinder_remote_object_new(
    GBinderLocalObject* local);

void
test_gbinder_remote_object_kill(
    GBinderRemoteObject* remote);

gboolean
test_gbinder_remote_object_dead(
    GBinderRemoteObject* remote);

GBinderLocalObject*
test_gbinder_remote_object_to_local(
    GBinderRemoteObject* remote);

/* test_gbinder_servicemanager.c */

GBinderRemoteObject*
test_gbinder_servicemanager_new_service(
    GBinderServiceManager* manager,
    const char* name,
    GBinderLocalObject* local);

#endif /* TEST_GBINDER_H */

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
