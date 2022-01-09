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

#ifndef RADIO_BASE_H
#define RADIO_BASE_H

#include "radio_types_p.h"

#include <glib-object.h>

/* RadioBaseFunc must be compatible with RadioClientFunc */
typedef
void
(*RadioBaseFunc)(
    RadioBase* base,
    gpointer user_data);

typedef
void
(*RadioBaseRequestSentFunc)(
    RadioBase* base,
    RadioRequest* req,
    int status);

typedef struct radio_base_priv RadioBasePriv;

struct radio_base {
    GObject object;
    RadioBasePriv* priv;
};

typedef struct radio_base_class {
    GObjectClass parent;
    gboolean (*is_dead)(RadioBase* base);
    gboolean (*can_submit_requests)(RadioBase* base);
    GBinderLocalRequest* (*new_request)(RadioBase* base, guint32 code);
    gulong (*send_request)(RadioBase* base, RadioRequest* req,
        RadioBaseRequestSentFunc sent);
    void (*cancel_request)(RadioBase* base, gulong id);
} RadioBaseClass;

GType radio_base_get_type(void) RADIO_INTERNAL;
#define RADIO_TYPE_BASE radio_base_get_type()
#define RADIO_BASE(obj) G_TYPE_CHECK_INSTANCE_CAST((obj), \
        RADIO_TYPE_BASE, RadioBase)
#define RADIO_BASE_CLASS(klass) G_TYPE_CHECK_CLASS_CAST((klass), \
        RADIO_TYPE_BASE, RadioBaseClass)
#define RADIO_BASE_GET_CLASS(obj) G_TYPE_INSTANCE_GET_CLASS((obj), \
        THIS_TYPE, RadioBaseClass)

void
radio_base_initialize(
    RadioBase* base)
    RADIO_INTERNAL;

void
radio_base_register_request(
    RadioBase* base,
    RadioRequest* req)
    RADIO_INTERNAL;

void
radio_base_unregister_request(
    RadioBase* base,
    RadioRequest* req)
    RADIO_INTERNAL;

gboolean
radio_base_submit_request(
    RadioBase* base,
    RadioRequest* req)
    RADIO_INTERNAL;

gboolean
radio_base_retry_request(
    RadioBase* base,
    RadioRequest* req)
    RADIO_INTERNAL;

void
radio_base_request_dropped(
    RadioRequest* req)
    RADIO_INTERNAL;

guint
radio_base_timeout_ms(
    RadioBase* base,
    RadioRequest* req)
    RADIO_INTERNAL;

void
radio_base_reset_timeout(
    RadioBase* base)
    RADIO_INTERNAL;

RADIO_BLOCK
radio_base_block_status(
    RadioBase* base,
    RadioRequestGroup* group)
    RADIO_INTERNAL;

RADIO_BLOCK
radio_base_block(
    RadioBase* base,
    RadioRequestGroup* group)
    RADIO_INTERNAL;

void
radio_base_unblock(
    RadioBase* base,
    RadioRequestGroup* group)
    RADIO_INTERNAL;

gboolean
radio_base_handle_resp(
    RadioBase* base,
    guint32 code,
    const RadioResponseInfo* info,
    const GBinderReader* reader)
    RADIO_INTERNAL;

void
radio_base_handle_ack(
    RadioBase* base,
    guint32 serial)
    RADIO_INTERNAL;

void
radio_base_handle_death(
    RadioBase* base)
    RADIO_INTERNAL;

void
radio_base_submit_requests(
    RadioBase* base)
    RADIO_INTERNAL;

void
radio_base_cancel_request(
    RadioBase* base,
    RadioRequest* req)
    RADIO_INTERNAL;

void
radio_base_set_default_timeout(
    RadioBase* self,
    int ms)
    RADIO_INTERNAL;

gulong
radio_base_add_owner_changed_handler(
    RadioBase* base,
    RadioBaseFunc func,
    gpointer user_data)
    RADIO_INTERNAL;

#endif /* RADIO_BASE_H */

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
