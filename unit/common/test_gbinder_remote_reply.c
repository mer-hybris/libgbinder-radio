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

struct gbinder_remote_reply {
    guint32 refcount;
    TestGBinderData* data;
};

static
void
test_gbinder_remote_reply_free(
    GBinderRemoteReply* self)
{
    test_gbinder_data_unref(self->data);
    g_free(self);
}

/*==========================================================================*
 * Internal API
 *==========================================================================*/

GBinderRemoteReply*
test_gbinder_remote_reply_new(
    GBinderLocalReply* reply)
{
    if (reply) {
        GBinderRemoteReply* self = g_new0(GBinderRemoteReply, 1);
        TestGBinderData* data = test_gbinder_local_reply_data(reply);

        g_atomic_int_set(&self->refcount, 1);
        self->data = test_gbinder_data_ref(data);
        return self;
    } else {
        return NULL;
    }
}

/*==========================================================================*
 * libgbinder API
 *==========================================================================*/

GBinderRemoteReply*
gbinder_remote_reply_ref(
    GBinderRemoteReply* self)
{
    if (self) {
        g_assert_cmpint(self->refcount, > ,0);
        g_atomic_int_inc(&self->refcount);
    }
    return self;
}

void
gbinder_remote_reply_unref(
    GBinderRemoteReply* self)
{
    if (self) {
        g_assert_cmpint(self->refcount, > ,0);
        if (g_atomic_int_dec_and_test(&self->refcount)) {
            test_gbinder_remote_reply_free(self);
        }
    }
}

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
