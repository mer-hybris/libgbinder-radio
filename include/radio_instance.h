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

#ifndef RADIO_INSTANCE_H
#define RADIO_INSTANCE_H

#include <radio_types.h>

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct radio_instance_priv RadioInstancePriv;

struct radio_instance {
    GObject parent;
    RadioInstancePriv* priv;
    gboolean dead;
    const char* dev;
    const char* slot;
    const char* key;
};

typedef struct radio_response_info {
    RADIO_RESP_TYPE type;
    guint32 serial;
    guint32 error;
} RadioResponseInfo;

typedef
void
(*RadioInstanceFunc)(
    RadioInstance* radio,
    gpointer user_data);

typedef
gboolean
(*RadioResponseHandlerFunc)(
    RadioInstance* radio,
    RADIO_RESP code,
    const RadioResponseInfo* info,
    const GBinderReader* reader,
    gpointer user_data);

typedef
void
(*RadioResponseObserverFunc)(
    RadioInstance* radio,
    RADIO_RESP code,
    const RadioResponseInfo* info,
    const GBinderReader* reader,
    gpointer user_data);

typedef
gboolean
(*RadioIndicationHandlerFunc)(
    RadioInstance* radio,
    RADIO_IND code,
    RADIO_IND_TYPE type,
    const GBinderReader* reader,
    gpointer user_data);

typedef
void
(*RadioIndicationObserverFunc)(
    RadioInstance* radio,
    RADIO_IND code,
    RADIO_IND_TYPE type,
    const GBinderReader* reader,
    gpointer user_data);

typedef
void
(*RadioAckFunc)(
    RadioInstance* radio,
    guint32 serial,
    gpointer user_data);

GType radio_instance_get_type();
#define RADIO_TYPE_INSTANCE (radio_instance_get_type())
#define RADIO_INSTANCE(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
        RADIO_TYPE_INSTANCE, RadioInstance))

RadioInstance*
radio_instance_new(
    const char* dev,
    const char* name);

RadioInstance*
radio_instance_get(
    const char* dev,
    const char* name);

RadioInstance* const*
radio_instance_get_all(
    void);

RadioInstance*
radio_instance_ref(
    RadioInstance* radio);

void
radio_instance_unref(
    RadioInstance* radio);

const char*
radio_instance_req_name(
    RadioInstance* radio,
    RADIO_REQ req);

const char*
radio_instance_resp_name(
    RadioInstance* radio,
    RADIO_RESP resp);

const char*
radio_instance_ind_name(
    RadioInstance* radio,
    RADIO_IND ind);

gboolean
radio_instance_ack(
    RadioInstance* radio);

GBinderLocalRequest*
radio_instance_new_request(
    RadioInstance* radio,
    RADIO_REQ code);

gboolean
radio_instance_send_request_sync(
    RadioInstance* radio,
    RADIO_REQ code,
    GBinderLocalRequest* args);

gulong
radio_instance_add_indication_handler(
    RadioInstance* radio,
    RADIO_IND code,
    RadioIndicationHandlerFunc func,
    gpointer user_data);

gulong
radio_instance_add_indication_observer(
    RadioInstance* radio,
    RADIO_IND code,
    RadioIndicationObserverFunc func,
    gpointer user_data);

gulong
radio_instance_add_response_handler(
    RadioInstance* radio,
    RADIO_RESP code,
    RadioResponseHandlerFunc func,
    gpointer user_data);

gulong
radio_instance_add_response_observer(
    RadioInstance* radio,
    RADIO_RESP code,
    RadioResponseObserverFunc func,
    gpointer user_data);

gulong
radio_instance_add_ack_handler(
    RadioInstance* radio,
    RadioAckFunc func,
    gpointer user_data);

gulong
radio_instance_add_death_handler(
    RadioInstance* radio,
    RadioInstanceFunc func,
    gpointer user_data);

void
radio_instance_remove_handler(
    RadioInstance* radio,
    gulong id);

void
radio_instance_remove_handlers(
    RadioInstance* radio,
    gulong* ids,
    int count);

#define radio_instance_remove_all_handlers(radio,ids) \
    radio_instance_remove_handlers(radio, ids, G_N_ELEMENTS(ids))

G_END_DECLS

#endif /* RADIO_INSTANCE_H */

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
