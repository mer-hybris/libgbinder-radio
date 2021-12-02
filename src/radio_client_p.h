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

#ifndef RADIO_CLIENT_PRIVATE_H
#define RADIO_CLIENT_PRIVATE_H

#include "radio_types_p.h"
#include "radio_client.h"

#include <glib-object.h>

struct radio_client {
    GObject object;
    RadioInstance* instance;
};

void
radio_client_register_request(
    RadioClient* client,
    RadioRequest* req)
    RADIO_INTERNAL;

void
radio_client_unregister_request(
    RadioClient* client,
    RadioRequest* req)
    RADIO_INTERNAL;

gboolean
radio_client_submit_request(
    RadioClient* client,
    RadioRequest* req)
    RADIO_INTERNAL;

gboolean
radio_client_retry_request(
    RadioClient* client,
    RadioRequest* req)
    RADIO_INTERNAL;

void
radio_client_request_dropped(
    RadioRequest* req)
    RADIO_INTERNAL;

guint
radio_client_timeout_ms(
    RadioClient* client,
    RadioRequest* req)
    RADIO_INTERNAL;

void
radio_client_reset_timeout(
    RadioClient* client)
    RADIO_INTERNAL;

void
radio_client_reset_timeout(
    RadioClient* client)
    RADIO_INTERNAL;

RADIO_BLOCK
radio_client_block_status(
    RadioClient* client,
    RadioRequestGroup* group)
    RADIO_INTERNAL;

RADIO_BLOCK
radio_client_block(
    RadioClient* client,
    RadioRequestGroup* group)
    RADIO_INTERNAL;

void
radio_client_unblock(
    RadioClient* client,
    RadioRequestGroup* group)
    RADIO_INTERNAL;

#endif /* RADIO_CLIENT_PRIVATE_H */

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
