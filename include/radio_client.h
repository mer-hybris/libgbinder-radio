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

#ifndef RADIO_CLIENT_H
#define RADIO_CLIENT_H

/* This API exists since 1.4.3 */

#include <radio_types.h>

G_BEGIN_DECLS

typedef
void
(*RadioClientIndicationFunc)(
    RadioClient* client,
    RADIO_IND code,
    const GBinderReader* reader,
    gpointer user_data);

typedef
void
(*RadioClientFunc)(
    RadioClient* client,
    gpointer user_data);

RadioClient*
radio_client_new(
    RadioInstance* instance)
    G_GNUC_WARN_UNUSED_RESULT;

RadioClient*
radio_client_ref(
    RadioClient* client);

void
radio_client_unref(
    RadioClient* client);

RADIO_INTERFACE
radio_client_interface(
    RadioClient* client);

RADIO_AIDL_INTERFACE
radio_client_aidl_interface(
    RadioClient* client); /* Since 1.6.0 */

const char*
radio_client_slot(
    RadioClient* client);

gboolean
radio_client_dead(
    RadioClient* client);

gboolean
radio_client_connected(
    RadioClient* client);

void
radio_client_set_default_timeout(
    RadioClient* client,
    int milliseconds);

gulong
radio_client_add_indication_handler(
    RadioClient* client,
    RADIO_IND code,
    RadioClientIndicationFunc func,
    gpointer user_data);

gulong
radio_client_add_owner_changed_handler(
    RadioClient* client,
    RadioClientFunc func,
    gpointer user_data);

gulong
radio_client_add_death_handler(
    RadioClient* client,
    RadioClientFunc func,
    gpointer user_data);

gulong
radio_client_add_connected_handler(
    RadioClient* client,
    RadioClientFunc func,
    gpointer user_data);

void
radio_client_remove_handler(
    RadioClient* client,
    gulong id);

void
radio_client_remove_handlers(
    RadioClient* client,
    gulong* ids,
    int count);

#define radio_client_remove_all_handlers(client,ids) \
    radio_client_remove_handlers(client, ids, G_N_ELEMENTS(ids))

G_END_DECLS

#endif /* RADIO_CLIENT_H */

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
