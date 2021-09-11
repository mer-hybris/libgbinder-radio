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

#include <glib-object.h>

struct gbinder_remote_object {
    GObject parent;
    GBinderLocalObject* local;
    gboolean dead;
};

typedef GObjectClass GBinderRemoteObjectClass;
G_DEFINE_TYPE(GBinderRemoteObject, gbinder_remote_object, G_TYPE_OBJECT)

#define PARENT_CLASS gbinder_remote_object_parent_class
#define THIS_TYPE (gbinder_remote_object_get_type())
#define THIS(obj) G_TYPE_CHECK_INSTANCE_CAST(obj,THIS_TYPE,GBinderRemoteObject)

enum gbinder_remote_object_signal {
    SIGNAL_DEATH,
    SIGNAL_COUNT
};

#define SIGNAL_DEATH_NAME "death"

static guint remote_object_signals[SIGNAL_COUNT] = { 0 };

static
void
gbinder_remote_object_init(
    GBinderRemoteObject* self)
{
}

static
void
gbinder_remote_object_finalize(
    GObject* object)
{
    GBinderRemoteObject* self = THIS(object);

    gbinder_local_object_unref(self->local);
    G_OBJECT_CLASS(PARENT_CLASS)->finalize(object);
}

static
void
gbinder_remote_object_class_init(
    GBinderRemoteObjectClass* klass)
{
    G_OBJECT_CLASS(klass)->finalize = gbinder_remote_object_finalize;
    remote_object_signals[SIGNAL_DEATH] =
        g_signal_new(SIGNAL_DEATH_NAME, G_OBJECT_CLASS_TYPE(klass),
            G_SIGNAL_RUN_FIRST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}

/*==========================================================================*
 * Internal API
 *==========================================================================*/

GBinderRemoteObject*
test_gbinder_remote_object_new(
    GBinderLocalObject* local)
{
    GBinderRemoteObject* self = g_object_new(THIS_TYPE, NULL);

    g_assert(local);
    self->local = gbinder_local_object_ref(local);
    return self;
}

void
test_gbinder_remote_object_kill(
    GBinderRemoteObject* self)
{
    if (self && !self->dead) {
        self->dead = TRUE;
        g_signal_emit(self, remote_object_signals[SIGNAL_DEATH], 0);
    }
}

gboolean
test_gbinder_remote_object_dead(
    GBinderRemoteObject* self)
{
    return !self || self->dead;
}

GBinderLocalObject*
test_gbinder_remote_object_to_local(
    GBinderRemoteObject* self)
{
    return self ? self->local : NULL;
}

/*==========================================================================*
 * libgbinder API
 *==========================================================================*/

GBinderRemoteObject*
gbinder_remote_object_ref(
    GBinderRemoteObject* self)
{
    if (self) {
        g_object_ref(THIS(self));
    }
    return self;
}

void
gbinder_remote_object_unref(
    GBinderRemoteObject* self)
{
    if (self) {
        g_object_unref(THIS(self));
    }
}

gulong
gbinder_remote_object_add_death_handler(
    GBinderRemoteObject* self,
    GBinderRemoteObjectNotifyFunc fn,
    void* data)
{
    return (self && fn) ? g_signal_connect(self, SIGNAL_DEATH_NAME,
        G_CALLBACK(fn), data) : 0;
}

void
gbinder_remote_object_remove_handler(
    GBinderRemoteObject* self,
    gulong id)
{
    if (self && id) {
        g_signal_handler_disconnect(self, id);
    }
}

/*
 * Remote Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
