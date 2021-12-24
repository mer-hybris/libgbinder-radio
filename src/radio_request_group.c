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

#include "radio_base.h"
#include "radio_client.h"
#include "radio_request_group_p.h"
#include "radio_request_p.h"
#include "radio_log.h"

#include <gutil_macros.h>

typedef struct radio_request_group_object {
    RadioRequestGroup pub;
    GHashTable* requests;
    gint refcount;
} RadioRequestGroupObject;

static inline RadioRequestGroupObject*
radio_request_group_cast(RadioRequestGroup* group)
    { return group ? G_CAST(group, RadioRequestGroupObject, pub) : NULL; }

/*==========================================================================*
 * Implementation
 *==========================================================================*/

void
radio_request_group_unlink_func(
    gpointer req)
{
    ((RadioRequest*)req)->group = NULL;
}

static
void
radio_request_group_free(
    RadioRequestGroupObject* self)
{
    RadioRequestGroup* group = &self->pub;
    RadioBase* base = RADIO_BASE(group->client);

    radio_base_unblock(base, group);
    g_hash_table_destroy(self->requests);
    radio_base_unref(base);
    gutil_slice_free(self);
}

/*==========================================================================*
 * Internal API
 *==========================================================================*/

void
radio_request_group_add(
    RadioRequestGroup* group,
    RadioRequest* req)
{
    RadioRequestGroupObject* self = radio_request_group_cast(group);

    /* Request is never NULL but the group may be */
    if (self) {
        g_hash_table_insert(self->requests, req, req);
        req->group = group;
    }
}

void
radio_request_group_remove(
    RadioRequestGroup* group,
    RadioRequest* req)
{
    RadioRequestGroupObject* self = radio_request_group_cast(group);

    /* Request is never NULL but the group may be */
    if (self) {
        g_hash_table_remove(self->requests, req);
    }
}

/*==========================================================================*
 * API
 *==========================================================================*/

RadioRequestGroup*
radio_request_group_new(
    RadioClient* client)
{
    if (G_LIKELY(client)) {
        RadioRequestGroupObject* self = g_slice_new0(RadioRequestGroupObject);
        RadioRequestGroup* group = &self->pub;

        group->client = radio_client_ref(client);
        self->requests = g_hash_table_new_full(g_direct_hash, g_direct_equal,
            NULL, radio_request_group_unlink_func);
        g_atomic_int_set(&self->refcount, 1);
        return group;
    }
    return NULL;
}

RadioRequestGroup*
radio_request_group_ref(
    RadioRequestGroup* group)
{
    RadioRequestGroupObject* self = radio_request_group_cast(group);

    if (G_LIKELY(self)) {
        GASSERT(self->refcount > 0);
        g_atomic_int_inc(&self->refcount);
    }
    return group;
}

void
radio_request_group_unref(
    RadioRequestGroup* group)
{
    RadioRequestGroupObject* self = radio_request_group_cast(group);

    if (G_LIKELY(self)) {
        GASSERT(self->refcount > 0);
        if (g_atomic_int_dec_and_test(&self->refcount)) {
            radio_request_group_free(self);
        }
    }
}

void
radio_request_group_cancel(
    RadioRequestGroup* group)
{
    RadioRequestGroupObject* self = radio_request_group_cast(group);

    if (G_LIKELY(self)) {
        GHashTableIter it;
        gpointer value;
        GSList* list = NULL;
        GSList* l;

        /*
         * Move requests to the list and temporarily reference them
         * before invoking any callbacks.
         */
        g_hash_table_iter_init(&it, self->requests);
        while (g_hash_table_iter_next(&it, NULL, &value)) {
            list = g_slist_prepend(list, radio_request_ref(value));
            g_hash_table_iter_remove(&it);
        }

        /*
         * Actually cancel the requests. This invokes completion callbacks.
         * The group is already empty at this point.
         */
        for (l = list; l; l = l->next) {
            radio_request_cancel(l->data);
        }

        /* Release the temporary references */
        g_slist_free_full(list, radio_request_unref_func);
    }
}

RADIO_BLOCK
radio_request_group_block_status(
    RadioRequestGroup* group)
{
    return group ? radio_base_block_status(RADIO_BASE(group->client), group) :
        RADIO_BLOCK_NONE;
}

RADIO_BLOCK
radio_request_group_block(
    RadioRequestGroup* group)
{
    return group ? radio_base_block(RADIO_BASE(group->client), group) :
        RADIO_BLOCK_NONE;
}

void
radio_request_group_unblock(
    RadioRequestGroup* group)
{
    if (group) {
        radio_base_unblock(RADIO_BASE(group->client), group);
    }
}

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
