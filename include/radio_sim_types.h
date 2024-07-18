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

#ifndef RADIO_SIM_TYPES_H
#define RADIO_SIM_TYPES_H

#include <gbinder_types.h>

G_BEGIN_DECLS

#define RADIO_SIM_INTERFACE_MAX (RADIO_SIM_INTERFACE_COUNT - 1)

#define RADIO_SIM_INSTANCE         "default"
#define RADIO_SIM_IFACE_PREFIX     "android.hardware.radio.sim."
#define RADIO_SIM_IFACE            "IRadioSim"
#define RADIO_SIM_RESPONSE_IFACE   "IRadioSimResponse"
#define RADIO_SIM_INDICATION_IFACE "IRadioSimIndication"
#define RADIO_SIM                  RADIO_SIM_IFACE_PREFIX RADIO_SIM_IFACE
#define RADIO_SIM_FQNAME           RADIO_SIM "/" RADIO_SIM_INSTANCE
#define RADIO_SIM_RESPONSE         RADIO_SIM_IFACE_PREFIX RADIO_SIM_RESPONSE_IFACE
#define RADIO_SIM_INDICATION       RADIO_SIM_IFACE_PREFIX RADIO_SIM_INDICATION_IFACE

/* Transaction codes */

/* c(req,resp,Name,CALL_NAME) */
#define RADIO_SIM_CALL_1(c) \
    c(1,2,areUiccApplicationsEnabled,ARE_UICC_APPLICATIONS_ENABLED) \
    c(2,3,changeIccPin2ForApp,CHANGE_ICC_PIN2_FOR_APP) \
    c(3,4,changeIccPinForApp,CHANGE_ICC_PIN_FOR_APP) \
    c(4,5,enableUiccApplications,ENABLE_UICC_APPLICATIONS) \
    c(5,6,getAllowedCarriers,GET_ALLOWED_CARRIERS) \
    c(6,7,getCdmaSubscription,GET_CDMA_SUBSCRIPTION) \
    c(7,8,getCdmaSubscriptionSource,GET_CDMA_SUBSCRIPTION_SOURCE) \
    c(8,9,getFacilityLockForApp,GET_FACILITY_LOCK_FOR_APP) \
    c(9,10,getIccCardStatus,GET_ICC_CARD_STATUS) \
    c(10,11,getImsiForApp,GET_IMSI_FOR_APP) \
    c(11,12,getSimPhonebookCapacity,GET_SIM_PHONEBOOK_CAPACITY) \
    c(12,13,getSimPhonebookRecords,GET_SIM_PHONEBOOK_RECORDS) \
    c(13,14,iccCloseLogicalChannel,ICC_CLOSE_LOGICAL_CHANNEL) \
    c(14,15,iccIoForApp,ICC_IO_FOR_APP) \
    c(15,16,iccOpenLogicalChannel,ICC_OPEN_LOGICAL_CHANNEL) \
    c(16,17,iccTransmitApduBasicChannel,ICC_TRANSMIT_APDU_BASIC_CHANNEL) \
    c(17,18,iccTransmitApduLogicalChannel,ICC_TRANSMIT_APDU_LOGICAL_CHANNEL) \
    c(18,19,reportStkServiceIsRunning,REPORT_STK_SERVICE_IS_RUNNING) \
    c(19,20,requestIccSimAuthentication,REQUEST_ICC_SIM_AUTHENTICATION) \
    c(21,21,sendEnvelope,SEND_ENVELOPE) \
    c(22,22,sendEnvelopeWithStatus,SEND_ENVELOPE_WITH_STATUS) \
    c(23,23,sendTerminalResponseToSim,SEND_TERMINAL_RESPONSE_TO_SIM) \
    c(24,24,setAllowedCarriers,SET_ALLOWED_CARRIERS) \
    c(25,25,setCarrierInfoForImsiEncryption,SET_CARRIER_INFO_FOR_IMSI_ENCRYPTION) \
    c(26,26,setCdmaSubscriptionSource,SET_CDMA_SUBSCRIPTION_SOURCE) \
    c(27,27,setFacilityLockForApp,SET_FACILITY_LOCK_FOR_APP) \
    c(29,28,setSimCardPower,SET_SIM_CARD_POWER) \
    c(30,29,setUiccSubscription,SET_UICC_SUBSCRIPTION) \
    c(31,30,supplyIccPin2ForApp,SUPPLY_ICC_PIN2_FOR_APP) \
    c(32,31,supplyIccPinForApp,SUPPLY_ICC_PIN_FOR_APP) \
    c(33,32,supplyIccPuk2ForApp,SUPPLY_ICC_PUK2_FOR_APP) \
    c(34,33,supplyIccPukForApp,SUPPLY_ICC_PUK_FOR_APP) \
    c(35,34,supplySimDepersonalization,SUPPLY_SIM_DEPERSONALIZATION) \
    c(36,35,updateSimPhonebookRecords,UPDATE_SIM_PHONEBOOK_RECORDS) \

/* i(code,Name,IND_NAME) */
#define RADIO_SIM_IND_1(i) \
    i(1,carrierInfoForImsiEncryption,CARRIER_INFO_FOR_IMSI_ENCRYPTION) \
    i(2,cdmaSubscriptionSourceChanged,CDMA_SUBSCRIPTION_SOURCE_CHANGED) \
    i(3,simPhonebookChanged,SIM_PHONEBOOK_CHANGED) \
    i(4,simPhonebookRecordsReceived,SIM_PHONEBOOK_RECORDS_RECEIVED) \
    i(5,simRefresh,SIM_REFRESH) \
    i(6,simStatusChanged,SIM_STATUS_CHANGED) \
    i(7,stkEventNotify,STK_EVENT_NOTIFY) \
    i(8,stkProactiveCommand,STK_PROACTIVE_COMMAND) \
    i(9,stkSessionEnd,STK_SESSION_END) \
    i(10,subscriptionStatusChanged,SUBSCRIPTION_STATUS_CHANGED) \
    i(11,uiccApplicationsEnablementChanged,UICC_APPLICATIONS_ENABLEMENT_CHANGED) \

typedef enum radio_sim_req {
    RADIO_SIM_REQ_ANY = 0,
    RADIO_SIM_REQ_NONE = 0,
#define RADIO_SIM_REQ_(req,resp,Name,NAME) RADIO_SIM_REQ_##NAME = req,

    /* android.hardware.radio.sim.IRadioSim v1 */
    RADIO_SIM_CALL_1(RADIO_SIM_REQ_)
    RADIO_SIM_REQ_RESPONSE_ACKNOWLEDGEMENT = 20, /* responseAcknowledgement */
    RADIO_SIM_REQ_SET_RESPONSE_FUNCTIONS = 28, /* setResponseFunctions */
    RADIO_SIM_1_REQ_LAST = RADIO_SIM_REQ_UPDATE_SIM_PHONEBOOK_RECORDS,

#undef RADIO_SIM_REQ_
} RADIO_SIM_REQ;
G_STATIC_ASSERT(sizeof(RADIO_SIM_REQ) == 4);

typedef enum radio_sim_resp {
    RADIO_SIM_RESP_ANY = 0,
    RADIO_SIM_RESP_NONE = 0,
#define RADIO_SIM_RESP_(req,resp,Name,NAME) RADIO_SIM_RESP_##NAME = resp,

    /* android.hardware.radio.sim.IRadioSimResponse v1 */
    RADIO_SIM_CALL_1(RADIO_SIM_RESP_)
    RADIO_SIM_RESP_ACKNOWLEDGE_REQUEST = 1, /* acknowledgeRequest */
    RADIO_SIM_1_RESP_LAST = RADIO_SIM_RESP_UPDATE_SIM_PHONEBOOK_RECORDS,

#undef RADIO_SIM_RESP_
} RADIO_SIM_RESP;
G_STATIC_ASSERT(sizeof(RADIO_SIM_RESP) == 4);

typedef enum radio_sim_ind {
    RADIO_SIM_IND_ANY = 0,
    RADIO_SIM_IND_NONE = 0,
#define RADIO_SIM_IND_(code,Name,NAME) RADIO_SIM_IND_##NAME = code,

    /* android.hardware.radio.sim.IRadioSimIndication v1 */
    RADIO_SIM_IND_1(RADIO_SIM_IND_)
    RADIO_SIM_1_IND_LAST = RADIO_SIM_IND_UICC_APPLICATIONS_ENABLEMENT_CHANGED,

#undef RADIO_SIM_IND_
} RADIO_SIM_IND;
G_STATIC_ASSERT(sizeof(RADIO_SIM_IND) == 4);

G_END_DECLS

#endif /* RADIO_SIM_TYPES_H */

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
