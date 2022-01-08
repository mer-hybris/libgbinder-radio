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

#include "radio_base.h"
#include "radio_request_p.h"
#include "radio_request_group_p.h"
#include "radio_log.h"

#include <gbinder_local_request.h>
#include <gbinder_writer.h>

#include <gutil_macros.h>

typedef struct radio_request_object {
    RadioRequest pub;
    GDestroyNotify destroy;
    gsize serial_offset;
    gboolean dropped;
    gint refcount;
} RadioRequestObject;

static inline RadioRequestObject* radio_request_cast(RadioRequest* req)
    { return req ? G_CAST(req, RadioRequestObject, pub) : NULL; }

/*==========================================================================*
 * Implementation
 *==========================================================================*/

static
void
radio_request_object_cancel(
    RadioRequestObject* self)
{
    RadioRequest* req = &self->pub;

    radio_base_cancel_request(req->object, req);
    if (!self->dropped) {
        self->dropped = TRUE;
        radio_request_group_remove(req->group, req);
        radio_base_request_dropped(req);
    }
    radio_base_unregister_request(req->object, req);
}

static
void
radio_request_free(
    RadioRequestObject* self)
{
    RadioRequest* req = &self->pub;

    radio_request_object_cancel(self);
    if (req->complete) {
        RadioRequestCompleteFunc complete = req->complete;

        /* Request is being freed too early, before completion */
        req->complete = NULL;
        complete(req, RADIO_TX_STATUS_FAILED, RADIO_RESP_NONE,
            RADIO_ERROR_NONE, NULL, req->user_data);
    }
    if (self->destroy) {
        GDestroyNotify destroy = self->destroy;

        self->destroy = NULL;
        destroy(req->user_data);
    }
    gbinder_local_request_unref(req->args);
    gutil_slice_free(self);
}

static
void
radio_request_object_unref(
    RadioRequestObject* self)
{
    if (G_LIKELY(self)) {
        GASSERT(self->refcount > 0);
        if (g_atomic_int_dec_and_test(&self->refcount)) {
            radio_request_free(self);
        }
    }
}

static
gboolean
radio_request_default_retry(
    RadioRequest* req,
    RADIO_TX_STATUS status,
    RADIO_RESP resp,
    RADIO_ERROR error,
    const GBinderReader* reader,
    void* user_data)
{
    return status != RADIO_TX_STATUS_OK || error != RADIO_ERROR_NONE;
}

static
RadioRequest*
radio_request_object_new(
    RadioBase* base,
    RadioRequestGroup* group,
    RADIO_REQ code,
    GBinderWriter* writer,
    RadioRequestGenericCompleteFunc complete,
    GDestroyNotify destroy,
    void* user_data)
{
    RadioRequestObject* self = g_slice_new0(RadioRequestObject);
    RadioRequest* req = &self->pub;
    GBinderWriter tmp;

    self->destroy = destroy;
    g_atomic_int_set(&self->refcount, 1);

    req->state = RADIO_REQUEST_STATE_NEW;
    req->code = code;
    req->complete = complete;
    req->user_data = user_data;
    req->retry = radio_request_default_retry;

    /* Assign serial and add to the group */
    radio_base_register_request(base, req);
    radio_request_group_add(group, req);

    /* Build the argument list */
    if (!writer) writer = &tmp;
    req->args = RADIO_BASE_GET_CLASS(base)->new_request(base, code);
    gbinder_local_request_init_writer(req->args, writer);
    self->serial_offset = gbinder_writer_bytes_written(writer);
    gbinder_writer_append_int32(writer, req->serial);
    return req;
}

/*==========================================================================*
 * Internal API
 *==========================================================================*/

void
radio_request_unref_func(
    gpointer req)
{
    radio_request_object_unref(radio_request_cast(req));
}

void
radio_request_update_serial(
    RadioRequest* req,
    guint32 serial)
{
    GBinderWriter writer;

    gbinder_local_request_init_writer(req->args, &writer);
    gbinder_writer_overwrite_int32(&writer,
        radio_request_cast(req)->serial_offset, serial);
}

/*==========================================================================*
 * API
 *==========================================================================*/

RadioRequest*
radio_request_new(
    RadioClient* client,
    RADIO_REQ code,
    GBinderWriter* writer, /* NULL if serial is the only arg */
    RadioRequestCompleteFunc complete,
    GDestroyNotify destroy,
    void* user_data)
{
    return client ? radio_request_object_new(RADIO_BASE(client), NULL,
        code, writer, (RadioRequestGenericCompleteFunc) complete,
        destroy, user_data) : NULL;
}

RadioRequest*
radio_request_new2(
    RadioRequestGroup* group,
    RADIO_REQ code,
    GBinderWriter* writer, /* NULL if serial is the only arg */
    RadioRequestCompleteFunc complete,
    GDestroyNotify destroy,
    void* user_data)
{
    return group ? radio_request_object_new(RADIO_BASE(group->client), group,
        code, writer, (RadioRequestGenericCompleteFunc) complete,
        destroy, user_data) : NULL;
}

RadioRequest*
radio_config_request_new(
    RadioConfig* config,
    RADIO_CONFIG_REQ code,
    GBinderWriter* writer, /* NULL if serial is the only arg */
    RadioConfigRequestCompleteFunc complete,
    GDestroyNotify destroy,
    void* user_data) /* Since 1.4.6 */
{
    return config ? radio_request_object_new(RADIO_BASE(config), NULL,
        code, writer, (RadioRequestGenericCompleteFunc) complete,
        destroy, user_data) : NULL;
}

RadioRequest*
radio_request_ref(
    RadioRequest* req)
{
    RadioRequestObject* self = radio_request_cast(req);

    if (G_LIKELY(self)) {
        GASSERT(self->refcount > 0);
        g_atomic_int_inc(&self->refcount);
    }
    return req;
}

void
radio_request_unref(
    RadioRequest* req)
{
    radio_request_object_unref(radio_request_cast(req));
}

void
radio_request_set_blocking(
    RadioRequest* req,
    gboolean blocking)
{
    if (G_LIKELY(req)) {
        req->blocking = blocking;
    }
}

void
radio_request_set_timeout(
    RadioRequest* req,
    guint ms)
{
    if (G_LIKELY(req) && req->timeout_ms != ms) {
        RadioBase* base = req->object;

        req->timeout_ms = ms;
        if (base && req->state >= RADIO_REQUEST_STATE_QUEUED) {
            const uint timeout = radio_base_timeout_ms(base, req);

            req->deadline = g_get_monotonic_time() + MICROSEC(timeout);
            radio_base_reset_timeout(base);
        }
    }
}

void
radio_request_set_retry(
    RadioRequest* req,
    guint delay_ms,     /* Delay before each retry, in milliseconds */
    int max_count)      /* Negative count to keep retrying indefinitely */
{
    if (G_LIKELY(req)) {
        req->retry_delay_ms = delay_ms;
        req->max_retries = max_count;
    }
}

void
radio_request_set_retry_func(
    RadioRequest* req,
    RadioRequestRetryFunc retry)
{
    if (G_LIKELY(req)) {
        req->retry = retry ? retry : radio_request_default_retry;
    }
}

gboolean
radio_request_submit(
    RadioRequest* req)
{
    return req && req->object && radio_base_submit_request(req->object, req);
}

gboolean
radio_request_retry(
    RadioRequest* req)
{
    return req && req->object && radio_base_retry_request(req->object, req);
}

void
radio_request_cancel(
    RadioRequest* req)
{
    RadioRequestObject* self = radio_request_cast(req);

    if (G_LIKELY(self)) {
        req->complete = NULL;
        radio_request_object_cancel(self);
    }
}

void
radio_request_drop(
    RadioRequest* req)
{
    RadioRequestObject* self = radio_request_cast(req);

    if (G_LIKELY(self)) {
        req->complete = NULL;
        radio_request_object_cancel(self);
        radio_request_object_unref(self);
    }
}

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
