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

#ifndef RADIO_VOICE_TYPES_H
#define RADIO_VOICE_TYPES_H

#include <gbinder_types.h>

G_BEGIN_DECLS

#define RADIO_VOICE_INTERFACE_MAX (RADIO_VOICE_INTERFACE_COUNT - 1)

#define RADIO_VOICE_INSTANCE         "default"
#define RADIO_VOICE_IFACE_PREFIX     "android.hardware.radio.voice."
#define RADIO_VOICE_IFACE            "IRadioVoice"
#define RADIO_VOICE_RESPONSE_IFACE   "IRadioVoiceResponse"
#define RADIO_VOICE_INDICATION_IFACE "IRadioVoiceIndication"
#define RADIO_VOICE                  RADIO_VOICE_IFACE_PREFIX RADIO_VOICE_IFACE
#define RADIO_VOICE_FQNAME           RADIO_VOICE "/" RADIO_VOICE_INSTANCE
#define RADIO_VOICE_RESPONSE         RADIO_VOICE_IFACE_PREFIX RADIO_VOICE_RESPONSE_IFACE
#define RADIO_VOICE_INDICATION       RADIO_VOICE_IFACE_PREFIX RADIO_VOICE_INDICATION_IFACE

/* Transaction codes */

/* c(req,resp,Name,CALL_NAME) */
#define RADIO_VOICE_CALL_1(c) \
    c(1,1,acceptCall,ACCEPT_CALL) \
    c(2,3,cancelPendingUssd,CANCEL_PENDING_USSD) \
    c(3,4,conference,CONFERENCE) \
    c(4,5,dial,DIAL) \
    c(5,6,emergencyDial,EMERGENCY_DIAL) \
    c(6,7,exitEmergencyCallbackMode,EXIT_EMERGENCY_CALLBACK_MODE) \
    c(7,8,explicitCallTransfer,EXPLICIT_CALL_TRANSFER) \
    c(8,9,getCallForwardStatus,GET_CALL_FORWARD_STATUS) \
    c(9,10,getCallWaiting,GET_CALL_WAITING) \
    c(10,11,getClip,GET_CLIP) \
    c(11,12,getClir,GET_CLIR) \
    c(12,13,getCurrentCalls,GET_CURRENT_CALLS) \
    c(13,14,getLastCallFailCause,GET_LAST_CALL_FAIL_CAUSE) \
    c(14,15,getMute,GET_MUTE) \
    c(15,16,getPreferredVoicePrivacy,GET_PREFERRED_VOICE_PRIVACY) \
    c(16,17,getTtyMode,GET_TTY_MODE) \
    c(17,18,handleStkCallSetupRequestFromSim,HANDLE_STK_CALL_SETUP_REQUEST_FROM_SIM) \
    c(19,20,hangupForegroundResumeBackground,HANGUP_FOREGROUND_RESUME_BACKGROUND) \
    c(20,21,hangupWaitingOrBackground,HANGUP_WAITING_OR_BACKGROUND) \
    c(21,22,isVoNrEnabled,IS_VO_NR_ENABLED) \
    c(22,23,rejectCall,REJECT_CALL) \
    c(24,24,sendBurstDtmf,SEND_BURST_DTMF) \
    c(25,25,sendCdmaFeatureCode,SEND_CDMA_FEATURE_CODE) \
    c(26,26,sendDtmf,SEND_DTMF) \
    c(27,27,sendUssd,SEND_USSD) \
    c(28,28,separateConnection,SEPARATE_CONNECTION) \
    c(29,29,setCallForward,SET_CALL_FORWARD) \
    c(30,30,setCallWaiting,SET_CALL_WAITING) \
    c(31,31,setClir,SET_CLIR) \
    c(32,32,setMute,SET_MUTE) \
    c(33,33,setPreferredVoicePrivacy,SET_PREFERRED_VOICE_PRIVACY) \
    c(35,34,setTtyMode,SET_TTY_MODE) \
    c(36,35,setVoNrEnabled,SET_VO_NR_ENABLED) \
    c(37,36,startDtmf,START_DTMF) \
    c(38,37,stopDtmf,STOP_DTMF) \
    c(39,38,switchWaitingOrHoldingAndActive,SWITCH_WAITING_OR_HOLDING_AND_ACTIVE) \

/* i(code,Name,IND_NAME) */
#define RADIO_VOICE_IND_1(i) \
    i(1,callRing,CALL_RING) \
    i(2,callStateChanged,CALL_STATE_CHANGED) \
    i(3,cdmaCallWaiting,CDMA_CALL_WAITING) \
    i(4,cdmaInfoRec,CDMA_INFO_REC) \
    i(5,cdmaOtaProvisionStatus,CDMA_OTA_PROVISION_STATUS) \
    i(6,currentEmergencyNumberList,CURRENT_EMERGENCY_NUMBER_LIST) \
    i(7,enterEmergencyCallbackMode,ENTER_EMERGENCY_CALLBACK_MODE) \
    i(8,exitEmergencyCallbackMode,EXIT_EMERGENCY_CALLBACK_MODE) \
    i(9,indicateRingbackTone,INDICATE_RINGBACK_TONE) \
    i(10,onSupplementaryServiceIndication,ON_SUPPLEMENTARY_SERVICE_INDICATION) \
    i(11,onUssd,ON_USSD) \
    i(12,resendIncallMute,RESEND_INCALL_MUTE) \
    i(13,srvccStateNotify,SRVCC_STATE_NOTIFY) \
    i(14,stkCallControlAlphaNotify,STK_CALL_CONTROL_ALPHA_NOTIFY) \
    i(15,stkCallSetup,STK_CALL_SETUP) \

typedef enum radio_voice_req {
    RADIO_VOICE_REQ_ANY = 0,
    RADIO_VOICE_REQ_NONE = 0,
#define RADIO_VOICE_REQ_(req,resp,Name,NAME) RADIO_VOICE_REQ_##NAME = req,

    /* android.hardware.radio.voice.IRadioVoice v1 */
    RADIO_VOICE_CALL_1(RADIO_VOICE_REQ_)
    RADIO_VOICE_REQ_HANGUP = 18, /* hangup */
    RADIO_VOICE_REQ_RESPONSE_ACKNOWLEDGEMENT = 23, /* responseAcknowledgement */
    RADIO_VOICE_REQ_SET_RESPONSE_FUNCTIONS = 34, /* setResponseFunctions */
    RADIO_VOICE_1_REQ_LAST = RADIO_VOICE_REQ_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE,

#undef RADIO_VOICE_REQ_
} RADIO_VOICE_REQ;
G_STATIC_ASSERT(sizeof(RADIO_VOICE_REQ) == 4);

typedef enum radio_voice_resp {
    RADIO_VOICE_RESP_ANY = 0,
    RADIO_VOICE_RESP_NONE = 0,
#define RADIO_VOICE_RESP_(req,resp,Name,NAME) RADIO_VOICE_RESP_##NAME = resp,

    /* android.hardware.radio.voice.IRadioVoiceResponse v1 */
    RADIO_VOICE_CALL_1(RADIO_VOICE_RESP_)
    RADIO_VOICE_RESP_ACKNOWLEDGE_REQUEST = 2, /* acknowledgeRequest */
    RADIO_VOICE_RESP_HANGUP_CONNECTION_RESPONSE = 19, /* hangupConnectionResponse */
    RADIO_VOICE_1_RESP_LAST = RADIO_VOICE_RESP_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE,

#undef RADIO_VOICE_RESP_
} RADIO_VOICE_RESP;
G_STATIC_ASSERT(sizeof(RADIO_VOICE_RESP) == 4);

typedef enum radio_voice_ind {
    RADIO_VOICE_IND_ANY = 0,
    RADIO_VOICE_IND_NONE = 0,
#define RADIO_VOICE_IND_(code,Name,NAME) RADIO_VOICE_IND_##NAME = code,

    /* android.hardware.radio.voice.IRadioVoiceIndication v1 */
    RADIO_VOICE_IND_1(RADIO_VOICE_IND_)
    RADIO_VOICE_1_IND_LAST = RADIO_VOICE_IND_STK_CALL_SETUP,

#undef RADIO_VOICE_IND_
} RADIO_VOICE_IND;
G_STATIC_ASSERT(sizeof(RADIO_VOICE_IND) == 4);

G_END_DECLS

#endif /* RADIO_VOICE_TYPES_H */

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
