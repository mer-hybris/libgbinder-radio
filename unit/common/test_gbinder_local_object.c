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

#include <gutil_strv.h>

struct gbinder_local_object {
    guint32 refcount;
    char** ifaces;
    GBinderLocalTransactFunc txproc;
    void* user_data;
    GBINDER_STABILITY_LEVEL stability;
};

static const char hidl_base_interface[] = "android.hidl.base@1.0::IBase";

static
void
test_gbinder_local_object_free(
    GBinderLocalObject* self)
{
    g_strfreev(self->ifaces);
    g_free(self);
}

/*==========================================================================*
 * Internal API
 *==========================================================================*/

GBinderLocalObject*
test_gbinder_local_object_new(
    const char* const* ifaces,
    GBinderLocalTransactFunc txproc,
    void* user_data)
{
    GBinderLocalObject* self = g_new0(GBinderLocalObject, 1);
    guint i = 0, n = gutil_strv_length((char**)ifaces);
    gboolean append_base_interface;

    if (g_strcmp0(gutil_strv_last((char**)ifaces), hidl_base_interface)) {
        append_base_interface = TRUE;
        n++;
    } else {
        append_base_interface = FALSE;
    }

    self->ifaces = g_new(char*, n + 1);
    if (ifaces) {
        while (*ifaces) {
            self->ifaces[i++] = g_strdup(*ifaces++);
        }
    }
    if (append_base_interface) {
        self->ifaces[i++] = g_strdup(hidl_base_interface);
    }
    self->ifaces[i] = NULL;
    self->txproc = txproc;
    self->user_data = user_data;
    g_atomic_int_set(&self->refcount, 1);
    return self;
}

GBinderLocalReply*
test_gbinder_local_object_handle_tx(
    GBinderLocalObject* self,
    GBinderRemoteRequest* req,
    guint code,
    guint flags,
    int* status)
{
    return (self && self->txproc) ?
        self->txproc(self, req, code, flags, status, self->user_data) :
        NULL;
}

/*==========================================================================*
 * libgbinder API
 *==========================================================================*/

GBinderLocalObject*
gbinder_local_object_ref(
    GBinderLocalObject* self)
{
    if (self) {
        g_assert_cmpint(self->refcount, > ,0);
        g_atomic_int_inc(&self->refcount);
    }
    return self;
}

void
gbinder_local_object_unref(
    GBinderLocalObject* self)
{
    if (self) {
        g_assert_cmpint(self->refcount, > ,0);
        if (g_atomic_int_dec_and_test(&self->refcount)) {
            test_gbinder_local_object_free(self);
        }
    }
}

void
gbinder_local_object_drop(
    GBinderLocalObject* self)
{
    if (self) {
        self->txproc = NULL;
        self->user_data = NULL;
        gbinder_local_object_unref(self);
    }
}

void
gbinder_local_object_set_stability(
    GBinderLocalObject* self,
    GBINDER_STABILITY_LEVEL stability)
{
    if (self) {
        self->stability = stability;
    }
}

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
