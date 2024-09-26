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

#ifndef RADIO_DATA_TYPES_H
#define RADIO_DATA_TYPES_H

#include <gbinder_types.h>

G_BEGIN_DECLS

#define RADIO_DATA_INTERFACE_MAX (RADIO_DATA_INTERFACE_COUNT - 1)

#define RADIO_DATA_INSTANCE         "default"
#define RADIO_DATA_IFACE_PREFIX     "android.hardware.radio.data."
#define RADIO_DATA_IFACE            "IRadioData"
#define RADIO_DATA_RESPONSE_IFACE   "IRadioDataResponse"
#define RADIO_DATA_INDICATION_IFACE "IRadioDataIndication"
#define RADIO_DATA                  RADIO_DATA_IFACE_PREFIX RADIO_DATA_IFACE
#define RADIO_DATA_FQNAME           RADIO_DATA "/" RADIO_DATA_INSTANCE
#define RADIO_DATA_RESPONSE         RADIO_DATA_IFACE_PREFIX RADIO_DATA_RESPONSE_IFACE
#define RADIO_DATA_INDICATION       RADIO_DATA_IFACE_PREFIX RADIO_DATA_INDICATION_IFACE

/* Transaction codes */

/* c(req,resp,Name,CALL_NAME) */
#define RADIO_DATA_CALL_1(c) \
    c(1,2,allocatePduSessionId,ALLOCATE_PDU_SESSION_ID) \
    c(2,3,cancelHandover,CANCEL_HANDOVER) \
    c(3,4,deactivateDataCall,DEACTIVATE_DATA_CALL) \
    c(4,5,getDataCallList,GET_DATA_CALL_LIST) \
    c(5,6,getSlicingConfig,GET_SLICING_CONFIG) \
    c(6,7,releasePduSessionId,RELEASE_PDU_SESSION_ID) \
    c(8,8,setDataAllowed,SET_DATA_ALLOWED) \
    c(9,9,setDataProfile,SET_DATA_PROFILE) \
    c(10,10,setDataThrottling,SET_DATA_THROTTLING) \
    c(11,11,setInitialAttachApn,SET_INITIAL_ATTACH_APN) \
    c(13,12,setupDataCall,SETUP_DATA_CALL) \
    c(14,13,startHandover,START_HANDOVER) \
    c(15,14,startKeepalive,START_KEEPALIVE) \
    c(16,15,stopKeepalive,STOP_KEEPALIVE) \

/* i(code,Name,IND_NAME) */
#define RADIO_DATA_IND_1(i) \
    i(1,dataCallListChanged,DATA_CALL_LIST_CHANGED) \
    i(2,keepaliveStatus,KEEPALIVE_STATUS) \
    i(3,pcoData,PCO_DATA) \
    i(4,unthrottleApn,UNTHROTTLE_APN) \
    i(5,slicingConfigChanged,SLICING_CONFIG_CHANGED) \

typedef enum radio_data_req {
    RADIO_DATA_REQ_ANY = 0,
    RADIO_DATA_REQ_NONE = 0,
#define RADIO_DATA_REQ_(req,resp,Name,NAME) RADIO_DATA_REQ_##NAME = req,

    /* android.hardware.radio.data.IRadioData v1 */
    RADIO_DATA_CALL_1(RADIO_DATA_REQ_)
    RADIO_DATA_REQ_RESPONSE_ACKNOWLEDGEMENT = 7, /* responseAcknowledgement */
    RADIO_DATA_REQ_SET_RESPONSE_FUNCTIONS = 12, /* setResponseFunctions */
    RADIO_DATA_1_REQ_LAST = RADIO_DATA_REQ_STOP_KEEPALIVE,

#undef RADIO_DATA_REQ_
} RADIO_DATA_REQ;
G_STATIC_ASSERT(sizeof(RADIO_DATA_REQ) == 4);

typedef enum radio_data_resp {
    RADIO_DATA_RESP_ANY = 0,
    RADIO_DATA_RESP_NONE = 0,
#define RADIO_DATA_RESP_(req,resp,Name,NAME) RADIO_DATA_RESP_##NAME = resp,

    /* android.hardware.radio.data.IRadioDataResponse v1 */
    RADIO_DATA_CALL_1(RADIO_DATA_RESP_)
    RADIO_DATA_RESP_ACKNOWLEDGE_REQUEST = 1, /* acknowledgeRequest */
    RADIO_DATA_1_RESP_LAST = RADIO_DATA_RESP_STOP_KEEPALIVE,

#undef RADIO_DATA_RESP_
} RADIO_DATA_RESP;
G_STATIC_ASSERT(sizeof(RADIO_DATA_RESP) == 4);

typedef enum radio_data_ind {
    RADIO_DATA_IND_ANY = 0,
    RADIO_DATA_IND_NONE = 0,
#define RADIO_DATA_IND_(code,Name,NAME) RADIO_DATA_IND_##NAME = code,

    /* android.hardware.radio.data.IRadioDataIndication v1 */
    RADIO_DATA_IND_1(RADIO_DATA_IND_)
    RADIO_DATA_1_IND_LAST = RADIO_DATA_IND_SLICING_CONFIG_CHANGED,

#undef RADIO_DATA_IND_
} RADIO_DATA_IND;
G_STATIC_ASSERT(sizeof(RADIO_DATA_IND) == 4);

G_END_DECLS

#endif /* RADIO_DATA_TYPES_H */

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
