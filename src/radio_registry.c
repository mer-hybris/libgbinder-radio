/*
 * Copyright (C) 2018-2019 Jolla Ltd.
 * Copyright (C) 2018-2019 Slava Monich <slava.monich@jolla.com>
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

#include "radio_registry_p.h"
#include "radio_instance.h"

#include <gutil_misc.h>

typedef GObjectClass RadioRegistryClass;
struct radio_registry {
    GObject parent;
};

G_DEFINE_TYPE(RadioRegistry, radio_registry, G_TYPE_OBJECT)
#define RADIO_TYPE_REGISTRY (radio_registry_get_type())
#define RADIO_REGISTRY(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
        RADIO_TYPE_REGISTRY, RadioRegistry))

enum radio_registry_signal {
    SIGNAL_INSTANCE_ADDED,
    SIGNAL_INSTANCE_REMOVED,
    SIGNAL_COUNT
};

#define SIGNAL_INSTANCE_ADDED_NAME   "radio-instance-added"
#define SIGNAL_INSTANCE_REMOVED_NAME "radio-instance-removed"

static guint radio_registry_signals[SIGNAL_COUNT] = { 0 };

static RadioRegistry* radio_registry_instance = NULL;

/*==========================================================================*
 * Internal API
 *==========================================================================*/

void
radio_registry_instance_added(
    RadioInstance* radio)
{
    RadioRegistry* self = radio_registry_instance;

    if (self) {
        radio_registry_ref(self);
        g_signal_emit(self, radio_registry_signals[SIGNAL_INSTANCE_ADDED],
            g_quark_from_string(radio->key), radio);
        radio_registry_unref(self);
    }
}

void
radio_registry_instance_removed(
    const char* key)
{
    RadioRegistry* self = radio_registry_instance;

    if (self) {
        radio_registry_ref(self);
        g_signal_emit(self, radio_registry_signals[SIGNAL_INSTANCE_REMOVED],
            g_quark_from_string(key), key);
        radio_registry_unref(self);
    }
}

/*==========================================================================*
 * API
 *==========================================================================*/

RadioRegistry*
radio_registry_new(
    void)
{
    if (radio_registry_instance) {
        radio_registry_ref(radio_registry_instance);
    } else {
        radio_registry_instance = g_object_new(RADIO_TYPE_REGISTRY, NULL);
        g_object_add_weak_pointer(G_OBJECT(radio_registry_instance),
            (gpointer*)(&radio_registry_instance));
    }
    return radio_registry_instance;
    
}

RadioRegistry*
radio_registry_ref(
    RadioRegistry* self)
{
    if (G_LIKELY(self)) {
        g_object_ref(RADIO_REGISTRY(self));
        return self;
    } else {
        return NULL;
    }
}

void
radio_registry_unref(
    RadioRegistry* self)
{
    if (G_LIKELY(self)) {
        g_object_unref(RADIO_REGISTRY(self));
    }
}

gulong
radio_registry_add_instance_added_handler(
    RadioRegistry* self,
    const char* key, /* NULL for any */
    RadioRegistryInstanceFunc func,
    gpointer user_data)
{
    if (G_LIKELY(self) && G_LIKELY(func)) {
        return g_signal_connect_closure_by_id(self,
            radio_registry_signals[SIGNAL_INSTANCE_ADDED],
            (key && key[0]) ? g_quark_from_string(key) : 0,
            g_cclosure_new(G_CALLBACK(func), user_data, NULL), FALSE);
    }
    return 0;
}

gulong
radio_registry_add_instance_removed_handler(
    RadioRegistry* self,
    const char* key, /* NULL for any */
    RadioRegistryStrFunc func,
    gpointer user_data)
{
    if (G_LIKELY(self) && G_LIKELY(func)) {
        return g_signal_connect_closure_by_id(self,
            radio_registry_signals[SIGNAL_INSTANCE_REMOVED],
            (key && key[0]) ? g_quark_from_string(key) : 0,
            g_cclosure_new(G_CALLBACK(func), user_data, NULL), FALSE);
    }
    return 0;
}

void
radio_registry_remove_handler(
    RadioRegistry* self,
    gulong id)
{
    if (G_LIKELY(self) && G_LIKELY(id)) {
        g_signal_handler_disconnect(self, id);
    }
}

void
radio_registry_remove_handlers(
    RadioRegistry* self,
    gulong* ids,
    int count)
{
    gutil_disconnect_handlers(self, ids, count);
}

/*==========================================================================*
 * Internals
 *==========================================================================*/

static
void
radio_registry_init(
    RadioRegistry* self)
{
}

static
void
radio_registry_class_init(
    RadioRegistryClass* klass)
{
    radio_registry_signals[SIGNAL_INSTANCE_ADDED] =
        g_signal_new(SIGNAL_INSTANCE_ADDED_NAME, G_OBJECT_CLASS_TYPE(klass),
            G_SIGNAL_RUN_FIRST | G_SIGNAL_DETAILED, 0, NULL, NULL, NULL,
            G_TYPE_NONE, 1, RADIO_TYPE_INSTANCE);
    radio_registry_signals[SIGNAL_INSTANCE_REMOVED] =
        g_signal_new(SIGNAL_INSTANCE_REMOVED_NAME, G_OBJECT_CLASS_TYPE(klass),
            G_SIGNAL_RUN_FIRST | G_SIGNAL_DETAILED, 0, NULL, NULL, NULL,
            G_TYPE_NONE, 1, G_TYPE_STRING);
}

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */

