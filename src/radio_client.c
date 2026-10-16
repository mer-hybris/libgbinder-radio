/*
 * Copyright (C) 2026 Jolla Mobile Ltd
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

#include "radio_base.h"
#include "radio_client.h"
#include "radio_instance_p.h"
#include "radio_request_p.h"
#include "radio_util.h"
#include "radio_log.h"

#include <gutil_misc.h>

/* This API exists since 1.4.3 */

struct radio_client {
    RadioBase base;
};

typedef RadioBaseClass RadioClientClass;
GType radio_client_get_type() RADIO_INTERNAL;
G_DEFINE_TYPE(RadioClient, radio_client, RADIO_TYPE_BASE)

#define PARENT_CLASS radio_client_parent_class
#define THIS_TYPE radio_client_get_type()
#define THIS(obj) G_TYPE_CHECK_INSTANCE_CAST(obj, THIS_TYPE, RadioClient)

/*==========================================================================*
 * API (mostly wrappers around the internal RadioBase API)
 *==========================================================================*/

RadioClient*
radio_client_new(
    RadioInstance* instance)
{
    RadioClient* self = NULL;

    if (G_LIKELY(instance)) {
        self = g_object_new(THIS_TYPE, NULL);
        radio_base_initialize(&self->base, instance);
    }
    return self;
}

RadioClient*
radio_client_ref(
    RadioClient* self)
{
    if (G_LIKELY(self)) {
        g_object_ref(self);
    }
    return self;
}

void
radio_client_unref(
    RadioClient* self)
{
    if (G_LIKELY(self)) {
        g_object_unref(self);
    }
}

const char*
radio_client_slot(
    RadioClient* self)
{
    return G_LIKELY(self) ? self->base.instance->slot : NULL;
}

const char*
radio_client_req_name(
    RadioClient* self,
    RADIO_REQ req) /* Since 1.6.5 */
{
    return radio_req_name2(self ? self->base.instance : NULL, req);
}

const char*
radio_client_resp_name(
    RadioClient* self,
    RADIO_RESP resp) /* Since 1.6.5 */
{
    return radio_resp_name2(self ? self->base.instance : NULL, resp);
}

const char*
radio_client_ind_name(
    RadioClient* self,
    RADIO_IND ind) /* Since 1.6.5 */
{
    return radio_ind_name2(self ? self->base.instance : NULL, ind);
}

gboolean
radio_client_dead(
    RadioClient* self)
{
    return !self || self->base.instance->dead;
}

gboolean
radio_client_connected(
    RadioClient* self)
{
    return self && self->base.instance->connected;
}

RADIO_INTERFACE
radio_client_interface(
    RadioClient* self)
{
    return G_LIKELY(self) ? self->base.instance->version :
        RADIO_INTERFACE_NONE;
}

RADIO_AIDL_INTERFACE
radio_client_aidl_interface(
    RadioClient* self)
{
    return G_LIKELY(self) ? self->base.instance->interface_aidl :
        RADIO_AIDL_INTERFACE_NONE;
}

void
radio_client_set_default_timeout(
    RadioClient* self,
    int milliseconds)
{
    if (G_LIKELY(self)) {
        radio_base_set_default_timeout(&self->base, milliseconds);
    }
}

gulong
radio_client_add_indication_handler(
    RadioClient* self,
    RADIO_IND code,
    RadioClientIndicationFunc fn,
    gpointer user_data)
{
    return G_LIKELY(self) ? radio_base_add_indication_handler(&self->base,
        code, (RadioBaseIndicationFunc) fn, user_data) : 0;
}

gulong
radio_client_add_owner_changed_handler(
    RadioClient* self,
    RadioClientFunc fn,
    gpointer user_data)
{
    return G_LIKELY(self) ? radio_base_add_owner_changed_handler(&self->base,
        (RadioBaseFunc) fn, user_data) : 0;
}

gulong
radio_client_add_death_handler(
    RadioClient* self,
    RadioClientFunc fn,
    gpointer user_data)
{
    return G_LIKELY(self) ? radio_base_add_death_handler(&self->base,
        (RadioBaseFunc) fn, user_data) : 0;
}

gulong
radio_client_add_connected_handler(
    RadioClient* self,
    RadioClientFunc fn,
    gpointer user_data)
{
    return G_LIKELY(self) ? radio_base_add_connected_handler(&self->base,
        (RadioBaseFunc) fn, user_data) : 0;
}

void
radio_client_remove_handler(
    RadioClient* self,
    gulong id)
{
    if (G_LIKELY(id) && G_LIKELY(self)) {
        g_signal_handler_disconnect(self, id);
    }
}

void
radio_client_remove_handlers(
    RadioClient* self,
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
radio_client_init(
    RadioClient* self)
{
}

static
void
radio_client_class_init(
    RadioClientClass* klass)
{
}

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
