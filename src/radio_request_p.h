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

#ifndef RADIO_REQUEST_PRIVATE_H
#define RADIO_REQUEST_PRIVATE_H

#include "radio_types_p.h"
#include "radio_request.h"

/*
 * Request lifecycle
 * =================
 *
 *        +=====+
 *        | NEW | ----------------------[cancel]------------+
 *        +=====+                                           |
 *           |                                              |
 *        [submit]                                          |
 *           |                                              |
 *           |                                              |
 *    +----> +---------------+                              |
 *    |      |               |                              |
 *    |      |           (blocked)                          |
 *    |  (unblocked)         |                              |
 *    |      |               v                              v
 *    |      |           +========+                   +===========+
 * (retry)   |           | QUEUED | ----[cancel]----> | CANCELLED |
 *    |      |           +========+                   +===========+
 *    |      |               |                              ^
 *    |      |           (unblocked)                        |
 *    |      |               |                              |
 *    |      |    +----------+                              |
 *    |      |    |                                         |
 *    |      v    v                                         |
 *    |  +-------------+                                    |
 *    |  |   submit    |                +========+          |
 *    |  |  request    | ---(error)---> | FAILED |          |
 *    |  | transaction |                +========+          |
 *    |  +-------------+                    ^               |
 *    |         |                           |               |
 *    |        (ok)     +---(error)---------+               |
 *    |         |      /                                    |
 *    |         v     /                                     |
 *    |    +=========+                  +=========+         |
 *    +--- | PENDING | ---(timeout)---> | TIMEOUT |         |
 *         +=========+                  +=========+         |
 *           |        \                                     |
 *           |         +----------------[cancel]------------+
 *      (response)
 *           |
 *           v
 *        +======+
 *        | DONE |
 *        +======+
 *
 * Timeout starts ticking when request enters the PENDING state.
 * The library maintains an internal reference to the request in
 * QUEUED and PENDING states.
 */

typedef enum radio_request_state {
    RADIO_REQUEST_STATE_INVALID,
    RADIO_REQUEST_STATE_NEW,
    RADIO_REQUEST_STATE_QUEUED,
    RADIO_REQUEST_STATE_PENDING,
    /*
     * Reorder states carefully or better don't reorder at all.
     * States >= RADIO_REQUEST_STATE_FAILED are assumed to be
     * terminal states in the state machine.
     */
    RADIO_REQUEST_STATE_FAILED,
    RADIO_REQUEST_STATE_CANCELLED,
    RADIO_REQUEST_STATE_TIMEOUT,
    RADIO_REQUEST_STATE_DONE
} RADIO_REQUEST_STATE;

struct radio_request {
    RADIO_REQUEST_STATE state;
    RADIO_REQ code;
    GBinderLocalRequest* args;
    RadioRequestCompleteFunc complete;
    RadioRequestRetryFunc retry;
    void* user_data;
    guint32 serial;             /* Immutable, generated at creation time */
    guint32 serial2;            /* Mutable, used by the last transaction */
    int max_retries;            /* Negative = retry indefinitely */
    int retry_count;            /* Number of times we have already retried */
    guint retry_delay_ms;       /* Delay before each retry, in milliseconds */
    guint timeout_ms;           /* Timeout, in milliseconds (0 = default) */
    gint64 deadline;            /* Monotonic time, in microseconds */
    gulong tx_id;               /* Id of the request transaction */
    gboolean blocking;          /* TRUE if this request blocks all others */
    gboolean acked;
    RadioClient* client;        /* Not a reference */
    RadioRequestGroup* group;   /* Not a reference */
    RadioRequest* queue_next;
};

void
radio_request_unref_func(
    gpointer req)
    RADIO_INTERNAL;

void
radio_request_update_serial(
    RadioRequest* req,
    guint32 serial)
    RADIO_INTERNAL;

#endif /* RADIO_REQUEST_PRIVATE_H */

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
