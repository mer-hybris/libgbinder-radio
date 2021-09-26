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

#ifndef RADIO_REQUEST_H
#define RADIO_REQUEST_H

/* This API exists since 1.4.3 */

#include <radio_types.h>

/*
 * Basic workflow
 *
 * 1. radio_request_new() or radio_request_new2() creates the request.
 *    That assigns a serial and initializes GBinderWriter for appending
 *    more arguments. GBinderWriter pointer can be NULL if serial is the
 *    only argument.
 * 2. The caller (optionally) uses GBinderWriter to add more arguments
 *    to the request.
 * 3. radio_request_submit() submits the request
 * 4. radio_request_unref() can be called at this point to release the
 *    reference produced by radio_request_new() unless the caller needs
 *    to keep it. In any case, radio_request_unref() has to be called
 *    sooner or later for each request created by radio_request_new().
 *    The library keeps its own internal reference while the request is
 *    being processed.
 * 5. RadioRequestCompleteFunc receives the response from the radio service.
 */

G_BEGIN_DECLS

typedef enum radio_tx_status {
    RADIO_TX_STATUS_OK,       /* Successful completion, no error */
    RADIO_TX_STATUS_FAILED,   /* Request transaction failed */
    RADIO_TX_STATUS_TIMEOUT   /* No response transaction received */
} RADIO_TX_STATUS;

/*
 * RadioRequestCompleteFunc
 *
 * Invoked upon completion of each request. If an error occurs,
 * resp is set to RADIO_RESP_NONE (zero) and args is NULL.
 *
 * The status argument is the status of the request transaction.
 * If it's anything other than RADIO_TX_STATUS_OK, the request
 * transaction failed (or response didn't arrive in time) and
 * the other arguments can be ignored.
 *
 * If status is RADIO_TX_STATUS_OK then the resp, error and args
 * arguments contain the information received in the response.
 */
typedef
void
(*RadioRequestCompleteFunc)(
    RadioRequest* req,
    RADIO_TX_STATUS status,
    RADIO_RESP resp,
    RADIO_ERROR error,
    const GBinderReader* args,
    gpointer user_data);

/*
 * RadioRequestRetryFunc
 *
 * If retries are enabled with radio_request_set_retry_func(), then this
 * callback is invoiked to check whether the request should be retried,
 * based on the status received from the radio service and the contents
 * of the reply. If such callback returns TRUE, the request is retried
 * at some point in the future with a new serial, otherwise it gets
 * completed right away.
 *
 * user_data is the pointer passed to radio_request_new() when the request
 * was created.
 */
typedef
gboolean
(*RadioRequestRetryFunc)(
    RadioRequest* req,
    RADIO_TX_STATUS status,
    RADIO_RESP resp,
    RADIO_ERROR error,
    const GBinderReader* args,
    void* user_data);

RadioRequest*
radio_request_new(
    RadioClient* client,
    RADIO_REQ code,
    GBinderWriter* writer, /* NULL if serial is the only arg */
    RadioRequestCompleteFunc complete,
    GDestroyNotify destroy,
    void* user_data)
    G_GNUC_WARN_UNUSED_RESULT;

RadioRequest*
radio_request_new2(
    RadioRequestGroup* group,
    RADIO_REQ code,
    GBinderWriter* writer, /* NULL if serial is the only arg */
    RadioRequestCompleteFunc complete,
    GDestroyNotify destroy,
    void* user_data)
    G_GNUC_WARN_UNUSED_RESULT;

RadioRequest*
radio_request_ref(
    RadioRequest* req);

void
radio_request_unref(
    RadioRequest* req);

void
radio_request_set_blocking(
    RadioRequest* req,
    gboolean blocking);

void
radio_request_set_timeout(
    RadioRequest* req,
    guint milliseconds); /* Zero to use the default timeout */

void
radio_request_set_retry(
    RadioRequest* req,
    guint delay_ms,     /* Delay before each retry, in milliseconds */
    int max_count);     /* Negative count to keep retrying indefinitely */

void
radio_request_set_retry_func(
    RadioRequest* req,
    RadioRequestRetryFunc retry);

gboolean
radio_request_submit(
    RadioRequest* req);

gboolean
radio_request_retry(
    RadioRequest* req);

void
radio_request_cancel(
    RadioRequest* req);

void
radio_request_drop( /* cancel and unref */
    RadioRequest* req);

void
radio_request_set_retry_func(
    RadioRequest* req,
    RadioRequestRetryFunc retry);

G_END_DECLS

#endif /* RADIO_REQUEST_H */

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
