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

#ifndef RADIO_MESSAGING_TYPES_H
#define RADIO_MESSAGING_TYPES_H

#include <gbinder_types.h>

G_BEGIN_DECLS

#define RADIO_MESSAGING_INTERFACE_MAX (RADIO_MESSAGING_INTERFACE_COUNT - 1)

#define RADIO_MESSAGING_INSTANCE         "default"
#define RADIO_MESSAGING_IFACE_PREFIX     "android.hardware.radio.messaging."
#define RADIO_MESSAGING_IFACE            "IRadioMessaging"
#define RADIO_MESSAGING_RESPONSE_IFACE   "IRadioMessagingResponse"
#define RADIO_MESSAGING_INDICATION_IFACE "IRadioMessagingIndication"
#define RADIO_MESSAGING                  RADIO_MESSAGING_IFACE_PREFIX RADIO_MESSAGING_IFACE
#define RADIO_MESSAGING_FQNAME           RADIO_MESSAGING "/" RADIO_MESSAGING_INSTANCE
#define RADIO_MESSAGING_RESPONSE         RADIO_MESSAGING_IFACE_PREFIX RADIO_MESSAGING_RESPONSE_IFACE
#define RADIO_MESSAGING_INDICATION       RADIO_MESSAGING_IFACE_PREFIX RADIO_MESSAGING_INDICATION_IFACE

/* Transaction codes */

/* c(req,resp,Name,CALL_NAME) */
#define RADIO_MESSAGING_CALL_1(c) \
    c(1,1,acknowledgeIncomingGsmSmsWithPdu,ACKNOWLEDGE_INCOMING_GSM_SMS_WITH_PDU) \
    c(2,2,acknowledgeLastIncomingCdmaSms,ACKNOWLEDGE_LAST_INCOMING_CDMA_SMS) \
    c(3,3,acknowledgeLastIncomingGsmSms,ACKNOWLEDGE_LAST_INCOMING_GSM_SMS) \
    c(4,5,deleteSmsOnRuim,DELETE_SMS_ON_RUIM) \
    c(5,6,deleteSmsOnSim,DELETE_SMS_ON_SIM) \
    c(6,7,getCdmaBroadcastConfig,GET_CDMA_BROADCAST_CONFIG) \
    c(7,8,getGsmBroadcastConfig,GET_GSM_BROADCAST_CONFIG) \
    c(8,9,getSmscAddress,GET_SMSC_ADDRESS) \
    c(9,10,reportSmsMemoryStatus,REPORT_SMS_MEMORY_STATUS) \
    c(11,12,sendCdmaSms,SEND_CDMA_SMS) \
    c(12,11,sendCdmaSmsExpectMore,SEND_CDMA_SMS_EXPECT_MORE) \
    c(13,13,sendImsSms,SEND_IMS_SMS) \
    c(14,15,sendSms,SEND_SMS) \
    c(15,14,sendSmsExpectMore,SEND_SMS_EXPECT_MORE) \
    c(16,16,setCdmaBroadcastActivation,SET_CDMA_BROADCAST_ACTIVATION) \
    c(17,17,setCdmaBroadcastConfig,SET_CDMA_BROADCAST_CONFIG) \
    c(18,18,setGsmBroadcastActivation,SET_GSM_BROADCAST_ACTIVATION) \
    c(19,19,setGsmBroadcastConfig,SET_GSM_BROADCAST_CONFIG) \
    c(21,20,setSmscAddress,SET_SMSC_ADDRESS) \
    c(22,21,writeSmsToRuim,WRITE_SMS_TO_RUIM) \
    c(23,22,writeSmsToSim,WRITE_SMS_TO_SIM) \

/* i(code,Name,IND_NAME) */
#define RADIO_MESSAGING_IND_1(i) \
    i(1,cdmaNewSms,CDMA_NEW_SMS) \
    i(2,cdmaRuimSmsStorageFull,CDMA_RUIM_SMS_STORAGE_FULL) \
    i(3,newBroadcastSms,NEW_BROADCAST_SMS) \
    i(4,newSms,NEW_SMS) \
    i(5,newSmsOnSim,NEW_SMS_ON_SIM) \
    i(6,newSmsStatusReport,NEW_SMS_STATUS_REPORT) \
    i(7,simSmsStorageFull,SIM_SMS_STORAGE_FULL) \

typedef enum radio_messaging_req {
    RADIO_MESSAGING_REQ_ANY = 0,
    RADIO_MESSAGING_REQ_NONE = 0,
#define RADIO_MESSAGING_REQ_(req,resp,Name,NAME) RADIO_MESSAGING_REQ_##NAME = req,

    /* android.hardware.radio.messaging.IRadioMessaging v1 */
    RADIO_MESSAGING_CALL_1(RADIO_MESSAGING_REQ_)
    RADIO_MESSAGING_REQ_RESPONSE_ACKNOWLEDGEMENT = 10, /* responseAcknowledgement */
    RADIO_MESSAGING_REQ_SET_RESPONSE_FUNCTIONS = 20, /* setResponseFunctions */
    RADIO_MESSAGING_1_REQ_LAST = RADIO_MESSAGING_REQ_WRITE_SMS_TO_SIM,

#undef RADIO_MESSAGING_REQ_
} RADIO_MESSAGING_REQ;
G_STATIC_ASSERT(sizeof(RADIO_MESSAGING_REQ) == 4);

typedef enum radio_messaging_resp {
    RADIO_MESSAGING_RESP_ANY = 0,
    RADIO_MESSAGING_RESP_NONE = 0,
#define RADIO_MESSAGING_RESP_(req,resp,Name,NAME) RADIO_MESSAGING_RESP_##NAME = resp,

    /* android.hardware.radio.messaging.IRadioMessagingResponse v1 */
    RADIO_MESSAGING_CALL_1(RADIO_MESSAGING_RESP_)
    RADIO_MESSAGING_RESP_ACKNOWLEDGE_REQUEST = 4, /* acknowledgeRequest */
    RADIO_MESSAGING_1_RESP_LAST = RADIO_MESSAGING_RESP_WRITE_SMS_TO_SIM,

#undef RADIO_MESSAGING_RESP_
} RADIO_MESSAGING_RESP;
G_STATIC_ASSERT(sizeof(RADIO_MESSAGING_RESP) == 4);

typedef enum radio_messaging_ind {
    RADIO_MESSAGING_IND_ANY = 0,
    RADIO_MESSAGING_IND_NONE = 0,
#define RADIO_MESSAGING_IND_(code,Name,NAME) RADIO_MESSAGING_IND_##NAME = code,

    /* android.hardware.radio.messaging.IRadioMessagingIndication v1 */
    RADIO_MESSAGING_IND_1(RADIO_MESSAGING_IND_)
    RADIO_MESSAGING_1_IND_LAST = RADIO_MESSAGING_IND_SIM_SMS_STORAGE_FULL,

#undef RADIO_MESSAGING_IND_
} RADIO_MESSAGING_IND;
G_STATIC_ASSERT(sizeof(RADIO_MESSAGING_IND) == 4);

G_END_DECLS

#endif /* RADIO_MESSAGING_TYPES_H */

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
