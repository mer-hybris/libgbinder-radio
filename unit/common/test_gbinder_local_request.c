/*
 * Copyright (C) 2021 Jolla Ltd.
 * Copyright (C) 2021 Slava Monich <slava.monich@jolla.com>
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

struct gbinder_local_request {
    guint32 refcount;
    TestGBinderData* data;
    char* iface;
};

static
void
test_gbinder_local_request_free(
    GBinderLocalRequest* self)
{
    test_gbinder_data_unref(self->data);
    g_free(self->iface);
    g_free(self);
}

/*==========================================================================*
 * Internal API
 *==========================================================================*/

GBinderLocalRequest*
test_gbinder_local_request_new(
    const char* iface)
{
    GBinderLocalRequest* self = g_new0(GBinderLocalRequest, 1);

    g_assert(iface);
    g_atomic_int_set(&self->refcount, 1);
    self->data = test_gbinder_data_new();
    self->iface = g_strdup(iface);
    return self;
}

const char*
test_gbinder_local_request_interface(
    GBinderLocalRequest* self)
{
    return self ? self->iface : NULL;
}

TestGBinderData*
test_gbinder_local_request_data(
    GBinderLocalRequest* self)
{
    return self ? self->data : NULL;
}

/*==========================================================================*
 * libgbinder API
 *==========================================================================*/

GBinderLocalRequest*
gbinder_local_request_ref(
    GBinderLocalRequest* self)
{
    if (self) {
        g_assert_cmpint(self->refcount, > ,0);
        g_atomic_int_inc(&self->refcount);
    }
    return self;
}

void
gbinder_local_request_unref(
    GBinderLocalRequest* self)
{
    if (self) {
        g_assert_cmpint(self->refcount, > ,0);
        if (g_atomic_int_dec_and_test(&self->refcount)) {
            test_gbinder_local_request_free(self);
        }
    }
}

void
gbinder_local_request_init_writer(
    GBinderLocalRequest* self,
    GBinderWriter* writer)
{
    test_gbinder_data_init_writer(self->data, writer);
}

GBinderLocalRequest*
gbinder_local_request_append_int32(
    GBinderLocalRequest* self,
    guint32 value)
{
    if (self) {
        GBinderWriter writer;

        test_gbinder_data_init_writer(self->data, &writer);
        gbinder_writer_append_int32(&writer, value);
    }
    return self;
}

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
