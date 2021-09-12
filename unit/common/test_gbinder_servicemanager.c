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

#include <gutil_idlepool.h>
#include <gutil_log.h>

struct gbinder_servicemanager {
    guint32 refcount;
    GHashTable* services;
    GUtilIdlePool* pool;
    char* dev;
};

static GHashTable* test_servermanagers = NULL;

static
void
test_gbinder_servicemanager_free(
    GBinderServiceManager* self)
{
    /* Update the global table */
    g_assert(test_servermanagers);
    g_assert(g_hash_table_contains(test_servermanagers, self->dev));
    g_hash_table_remove(test_servermanagers, self->dev); /* Frees self->dev */
    if (g_hash_table_size(test_servermanagers) == 0) {
        g_hash_table_unref(test_servermanagers);
        test_servermanagers = NULL;
    }

    gutil_idle_pool_destroy(self->pool);
    g_hash_table_destroy(self->services);
    g_free(self);
}

/*==========================================================================*
 * Internal API
 *==========================================================================*/

GBinderRemoteObject*
test_gbinder_servicemanager_new_service(
    GBinderServiceManager* self,
    const char* name,
    GBinderLocalObject* local)
{
    GBinderRemoteObject* remote = test_gbinder_remote_object_new(local);

    g_hash_table_replace(self->services, g_strdup(name), remote);
    return gbinder_remote_object_ref(remote);
}

/*==========================================================================*
 * libgbinder API
 *==========================================================================*/

GBinderServiceManager*
gbinder_servicemanager_new(
    const char* dev)
{
    GBinderServiceManager* self = NULL;

    g_assert(dev && dev[0]);
    if (test_servermanagers) {
        self = g_hash_table_lookup(test_servermanagers, dev);
    }

    if (self) {
        gbinder_servicemanager_ref(self);
    } else {
        self = g_new0(GBinderServiceManager, 1);
        g_atomic_int_set(&self->refcount, 1);
        self->pool = gutil_idle_pool_new();
        self->dev = g_strdup(dev);  /* Owned by test_servermanagers */
        self->services = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
            (GDestroyNotify) gbinder_remote_object_unref);

        /* Update the global table */
        if (!test_servermanagers) {
            test_servermanagers = g_hash_table_new_full(g_str_hash, g_str_equal,
                g_free, NULL);
        }
        g_hash_table_replace(test_servermanagers, self->dev, self);
    }
    return self;
}

GBinderServiceManager*
gbinder_servicemanager_ref(
    GBinderServiceManager* self)
{
    if (self) {
        g_assert_cmpint(self->refcount, > ,0);
        g_atomic_int_inc(&self->refcount);
    }
    return self;
}

void
gbinder_servicemanager_unref(
    GBinderServiceManager* self)
{
    if (self) {
        g_assert_cmpint(self->refcount, > ,0);
        if (g_atomic_int_dec_and_test(&self->refcount)) {
            test_gbinder_servicemanager_free(self);
        }
    }
}

GBinderRemoteObject* /* autoreleased */
gbinder_servicemanager_get_service_sync(
    GBinderServiceManager* self,
    const char* name,
    int* status)
{
    if (self && name) {
        GBinderRemoteObject* obj = g_hash_table_lookup(self->services, name);

        if (obj) {
            gutil_idle_pool_add(self->pool, gbinder_remote_object_ref(obj),
                (GDestroyNotify) gbinder_remote_object_unref);
            return obj;
        } else {
            GDEBUG("Name %s not found", name);
        }
    }
    return NULL;
}

GBinderLocalObject*
gbinder_servicemanager_new_local_object2(
    GBinderServiceManager* self,
    const char* const* ifaces,
    GBinderLocalTransactFunc fn,
    void* user_data)
{
    return self ? test_gbinder_local_object_new(ifaces, fn, user_data) : NULL;
}

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
