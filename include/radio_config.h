/*
 * Copyright (C) 2022 Jolla Ltd.
 * Copyright (C) 2022 Slava Monich <slava.monich@jolla.com>
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

#ifndef RADIO_CONFIG_H
#define RADIO_CONFIG_H

/* This API exists since 1.4.6 */

#include <radio_config_types.h>
#include <radio_config_aidl_types.h>

G_BEGIN_DECLS

typedef
void
(*RadioConfigFunc)(
    RadioConfig* config,
    gpointer user_data);

typedef
void
(*RadioConfigRequestObserverFunc)(
    RadioConfig* config,
    RADIO_CONFIG_REQ code,
    GBinderLocalRequest* args,
    gpointer user_data);

typedef
void
(*RadioConfigResponseObserverFunc)(
    RadioConfig* config,
    RADIO_CONFIG_RESP code,
    const RadioResponseInfo* info,
    const GBinderReader* args,
    gpointer user_data);

typedef
void
(*RadioConfigIndicationObserverFunc)(
    RadioConfig* config,
    RADIO_CONFIG_IND code,
    const GBinderReader* args,
    gpointer user_data);

RadioConfig*
radio_config_new(void)
    G_GNUC_WARN_UNUSED_RESULT;

RadioConfig*
radio_config_new_with_version(
    RADIO_CONFIG_INTERFACE max_version)
    G_GNUC_WARN_UNUSED_RESULT;

RadioConfig*
radio_config_new_with_version_and_interface_type(
    RADIO_CONFIG_INTERFACE max_version,
    RADIO_INTERFACE_TYPE interface_type)
    G_GNUC_WARN_UNUSED_RESULT; /* Since 1.6.0 */

RadioConfig*
radio_config_ref(
    RadioConfig* config);

void
radio_config_unref(
    RadioConfig* config);

gboolean
radio_config_dead(
    RadioConfig* config);

RADIO_INTERFACE_TYPE
radio_config_interface_type(
    RadioConfig* self); /* Since 1.6.0 */

RADIO_CONFIG_INTERFACE
radio_config_interface(
    RadioConfig* config);

gsize
radio_config_rpc_header_size(
    RadioConfig* config,
    RADIO_CONFIG_REQ req);

const char*
radio_config_req_name(
    RadioConfig* config,
    RADIO_CONFIG_REQ req);

const char*
radio_config_resp_name(
    RadioConfig* config,
    RADIO_CONFIG_RESP resp);

const char*
radio_config_ind_name(
    RadioConfig* config,
    RADIO_CONFIG_IND ind);

gulong
radio_config_add_death_handler(
    RadioConfig* config,
    RadioConfigFunc func,
    gpointer user_data);

gulong
radio_config_add_request_observer(
    RadioConfig* config,
    RADIO_CONFIG_REQ code,
    RadioConfigRequestObserverFunc func,
    gpointer user_data);

gulong
radio_config_add_request_observer_with_priority(
    RadioConfig* config,
    RADIO_OBSERVER_PRIORITY priority,
    RADIO_CONFIG_REQ code,
    RadioConfigRequestObserverFunc func,
    gpointer user_data);

gulong
radio_config_add_response_observer(
    RadioConfig* config,
    RADIO_CONFIG_RESP code,
    RadioConfigResponseObserverFunc func,
    gpointer user_data);

gulong
radio_config_add_response_observer_with_priority(
    RadioConfig* config,
    RADIO_OBSERVER_PRIORITY priority,
    RADIO_CONFIG_RESP code,
    RadioConfigResponseObserverFunc func,
    gpointer user_data);

gulong
radio_config_add_indication_observer(
    RadioConfig* config,
    RADIO_CONFIG_IND code,
    RadioConfigIndicationObserverFunc func,
    gpointer user_data);

gulong
radio_config_add_indication_observer_with_priority(
    RadioConfig* config,
    RADIO_OBSERVER_PRIORITY priority,
    RADIO_CONFIG_IND code,
    RadioConfigIndicationObserverFunc func,
    gpointer user_data);

void
radio_config_remove_handler(
    RadioConfig* config,
    gulong id);

void
radio_config_remove_handlers(
    RadioConfig* config,
    gulong* ids,
    int count);

#define radio_config_remove_all_handlers(config,ids) \
    radio_config_remove_handlers(config, ids, G_N_ELEMENTS(ids))

G_END_DECLS

#endif /* RADIO_CONFIG_H */

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
