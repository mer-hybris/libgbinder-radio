/*
 * Copyright (C) 2024 Jollyboys Ltd
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

#ifndef RADIO_IMS_TYPES_H
#define RADIO_IMS_TYPES_H

#include <gbinder_types.h>

G_BEGIN_DECLS

#define RADIO_IMS_INTERFACE_MAX (RADIO_IMS_INTERFACE_COUNT - 1)

#define RADIO_IMS_INSTANCE         "default"
#define RADIO_IMS_IFACE_PREFIX     "android.hardware.radio.ims."
#define RADIO_IMS_IFACE            "IRadioIms"
#define RADIO_IMS_RESPONSE_IFACE   "IRadioImsResponse"
#define RADIO_IMS_INDICATION_IFACE "IRadioImsIndication"
#define RADIO_IMS                  RADIO_IMS_IFACE_PREFIX RADIO_IMS_IFACE
#define RADIO_IMS_FQNAME           RADIO_IMS "/" RADIO_IMS_INSTANCE
#define RADIO_IMS_RESPONSE         RADIO_IMS_IFACE_PREFIX RADIO_IMS_RESPONSE_IFACE
#define RADIO_IMS_INDICATION       RADIO_IMS_IFACE_PREFIX RADIO_IMS_INDICATION_IFACE

/* Transaction codes */

/* c(req,resp,Name,CALL_NAME) */
#define RADIO_IMS_CALL_1(c) \
    c(1,1,setSrvccCallInfo,SET_SRVCC_CALL_INFO) \
    c(2,2,updateImsRegistrationInfo,UPDATE_IMS_REGISTRATION_INFO) \
    c(3,3,startImsTraffic,START_IMS_TRAFFIC) \
    c(4,4,stopImsTraffic,STOP_IMS_TRAFFIC) \
    c(5,5,triggerEpsFallback,TRIGGER_EPS_FALLBACK) \
    c(7,6,sendAnbrQuery,SEND_ANBR_QUERY) \
    c(8,7,updateImsCallStatus,UPDATE_IMS_CALL_STATUS) \

/* i(code,Name,IND_NAME) */
#define RADIO_IMS_IND_1(i) \
    i(1,onConnectionSetupFailure,ON_CONNECTION_SETUP_FAILURE) \
    i(2,notifyAnbr,NOTIFY_ANBR) \
    i(3,triggerImsDeregistration,TRIGGER_IMS_DEREGISTRATION) \

typedef enum radio_ims_req {
    RADIO_IMS_REQ_ANY = 0,
    RADIO_IMS_REQ_NONE = 0,
#define RADIO_IMS_REQ_(req,resp,Name,NAME) RADIO_IMS_REQ_##NAME = req,

    /* android.hardware.radio.ims.IRadioIms v1 */
    RADIO_IMS_CALL_1(RADIO_IMS_REQ_)
    RADIO_IMS_REQ_SET_RESPONSE_FUNCTIONS = 6, /* setResponseFunctions */
    RADIO_IMS_1_REQ_LAST = RADIO_IMS_REQ_UPDATE_IMS_CALL_STATUS,

#undef RADIO_IMS_REQ_
} RADIO_IMS_REQ;
G_STATIC_ASSERT(sizeof(RADIO_IMS_REQ) == 4);

typedef enum radio_ims_resp {
    RADIO_IMS_RESP_ANY = 0,
    RADIO_IMS_RESP_NONE = 0,
#define RADIO_IMS_RESP_(req,resp,Name,NAME) RADIO_IMS_RESP_##NAME = resp,

    /* android.hardware.radio.ims.IRadioImsResponse v1 */
    RADIO_IMS_CALL_1(RADIO_IMS_RESP_)
    RADIO_IMS_1_RESP_LAST = RADIO_IMS_RESP_UPDATE_IMS_CALL_STATUS,

#undef RADIO_IMS_RESP_
} RADIO_IMS_RESP;
G_STATIC_ASSERT(sizeof(RADIO_IMS_RESP) == 4);

typedef enum radio_ims_ind {
    RADIO_IMS_IND_ANY = 0,
    RADIO_IMS_IND_NONE = 0,
#define RADIO_IMS_IND_(code,Name,NAME) RADIO_IMS_IND_##NAME = code,

    /* android.hardware.radio.ims.IRadioImsIndication v1 */
    RADIO_IMS_IND_1(RADIO_IMS_IND_)
    RADIO_IMS_1_IND_LAST = RADIO_IMS_IND_TRIGGER_IMS_DEREGISTRATION,

#undef RADIO_IMS_IND_
} RADIO_IMS_IND;
G_STATIC_ASSERT(sizeof(RADIO_IMS_IND) == 4);

G_END_DECLS

#endif /* RADIO_IMS_TYPES_H */

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
