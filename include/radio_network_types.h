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

#ifndef RADIO_NETWORK_TYPES_H
#define RADIO_NETWORK_TYPES_H

#include <gbinder_types.h>

G_BEGIN_DECLS

#define RADIO_NETWORK_INTERFACE_MAX (RADIO_NETWORK_INTERFACE_COUNT - 1)

#define RADIO_NETWORK_INSTANCE         "default"
#define RADIO_NETWORK_IFACE_PREFIX     "android.hardware.radio.network."
#define RADIO_NETWORK_IFACE            "IRadioNetwork"
#define RADIO_NETWORK_RESPONSE_IFACE   "IRadioNetworkResponse"
#define RADIO_NETWORK_INDICATION_IFACE "IRadioNetworkIndication"
#define RADIO_NETWORK                  RADIO_NETWORK_IFACE_PREFIX RADIO_NETWORK_IFACE
#define RADIO_NETWORK_FQNAME           RADIO_NETWORK "/" RADIO_NETWORK_INSTANCE
#define RADIO_NETWORK_RESPONSE         RADIO_NETWORK_IFACE_PREFIX RADIO_NETWORK_RESPONSE_IFACE
#define RADIO_NETWORK_INDICATION       RADIO_NETWORK_IFACE_PREFIX RADIO_NETWORK_INDICATION_IFACE

/* Transaction codes */

/* c(req,resp,Name,CALL_NAME) */
#define RADIO_NETWORK_CALL_1(c) \
    c(1,2,getAllowedNetworkTypesBitmap,GET_ALLOWED_NETWORK_TYPES_BITMAP) \
    c(2,3,getAvailableBandModes,GET_AVAILABLE_BAND_MODES) \
    c(3,4,getAvailableNetworks,GET_AVAILABLE_NETWORKS) \
    c(4,5,getBarringInfo,GET_BARRING_INFO) \
    c(5,6,getCdmaRoamingPreference,GET_CDMA_ROAMING_PREFERENCE) \
    c(6,7,getCellInfoList,GET_CELL_INFO_LIST) \
    c(7,8,getDataRegistrationState,GET_DATA_REGISTRATION_STATE) \
    c(8,9,getImsRegistrationState,GET_IMS_REGISTRATION_STATE) \
    c(9,10,getNetworkSelectionMode,GET_NETWORK_SELECTION_MODE) \
    c(10,11,getOperator,GET_OPERATOR) \
    c(11,12,getSignalStrength,GET_SIGNAL_STRENGTH) \
    c(12,13,getSystemSelectionChannels,GET_SYSTEM_SELECTION_CHANNELS) \
    c(13,14,getVoiceRadioTechnology,GET_VOICE_RADIO_TECHNOLOGY) \
    c(14,15,getVoiceRegistrationState,GET_VOICE_REGISTRATION_STATE) \
    c(15,16,isNrDualConnectivityEnabled,IS_NR_DUAL_CONNECTIVITY_ENABLED) \
    c(17,17,setAllowedNetworkTypesBitmap,SET_ALLOWED_NETWORK_TYPES_BITMAP) \
    c(18,18,setBandMode,SET_BAND_MODE) \
    c(19,19,setBarringPassword,SET_BARRING_PASSWORD) \
    c(20,20,setCdmaRoamingPreference,SET_CDMA_ROAMING_PREFERENCE) \
    c(21,21,setCellInfoListRate,SET_CELL_INFO_LIST_RATE) \
    c(22,22,setIndicationFilter,SET_INDICATION_FILTER) \
    c(23,23,setLinkCapacityReportingCriteria,SET_LINK_CAPACITY_REPORTING_CRITERIA) \
    c(24,24,setLocationUpdates,SET_LOCATION_UPDATES) \
    c(25,25,setNetworkSelectionModeAutomatic,SET_NETWORK_SELECTION_MODE_AUTOMATIC) \
    c(26,26,setNetworkSelectionModeManual,SET_NETWORK_SELECTION_MODE_MANUAL) \
    c(27,27,setNrDualConnectivityState,SET_NR_DUAL_CONNECTIVITY_STATE) \
    c(29,28,setSignalStrengthReportingCriteria,SET_SIGNAL_STRENGTH_REPORTING_CRITERIA) \
    c(30,29,setSuppServiceNotifications,SET_SUPP_SERVICE_NOTIFICATIONS) \
    c(31,30,setSystemSelectionChannels,SET_SYSTEM_SELECTION_CHANNELS) \
    c(32,31,startNetworkScan,START_NETWORK_SCAN) \
    c(33,32,stopNetworkScan,STOP_NETWORK_SCAN) \
    c(34,33,supplyNetworkDepersonalization,SUPPLY_NETWORK_DEPERSONALIZATION) \
    c(35,34,setUsageSetting,SET_USAGE_SETTING) \
    c(36,35,getUsageSetting,GET_USAGE_SETTING) \

/* i(code,Name,IND_NAME) */
#define RADIO_NETWORK_IND_1(i) \
    i(1,barringInfoChanged,BARRING_INFO_CHANGED) \
    i(2,cdmaPrlChanged,CDMA_PRL_CHANGED) \
    i(3,cellInfoList,CELL_INFO_LIST) \
    i(4,currentLinkCapacityEstimate,CURRENT_LINK_CAPACITY_ESTIMATE) \
    i(5,currentPhysicalChannelConfigs,CURRENT_PHYSICAL_CHANNEL_CONFIGS) \
    i(6,currentSignalStrength,CURRENT_SIGNAL_STRENGTH) \
    i(7,imsNetworkStateChanged,IMS_NETWORK_STATE_CHANGED) \
    i(8,networkScanResult,NETWORK_SCAN_RESULT) \
    i(9,networkStateChanged,NETWORK_STATE_CHANGED) \
    i(10,nitzTimeReceived,NITZ_TIME_RECEIVED) \
    i(11,registrationFailed,REGISTRATION_FAILED) \
    i(12,restrictedStateChanged,RESTRICTED_STATE_CHANGED) \
    i(13,suppSvcNotify,SUPP_SVC_NOTIFY) \
    i(14,voiceRadioTechChanged,VOICE_RADIO_TECH_CHANGED) \

typedef enum radio_network_req {
    RADIO_NETWORK_REQ_ANY = 0,
    RADIO_NETWORK_REQ_NONE = 0,
#define RADIO_NETWORK_REQ_(req,resp,Name,NAME) RADIO_NETWORK_REQ_##NAME = req,

    /* android.hardware.radio.network.IRadioNetwork v1 */
    RADIO_NETWORK_CALL_1(RADIO_NETWORK_REQ_)
    RADIO_NETWORK_REQ_RESPONSE_ACKNOWLEDGEMENT = 16, /* responseAcknowledgement */
    RADIO_NETWORK_REQ_SET_RESPONSE_FUNCTIONS = 28, /* setResponseFunctions */
    RADIO_NETWORK_1_REQ_LAST = RADIO_NETWORK_REQ_GET_USAGE_SETTING,

#undef RADIO_NETWORK_REQ_
} RADIO_NETWORK_REQ;
G_STATIC_ASSERT(sizeof(RADIO_NETWORK_REQ) == 4);

typedef enum radio_network_resp {
    RADIO_NETWORK_RESP_ANY = 0,
    RADIO_NETWORK_RESP_NONE = 0,
#define RADIO_NETWORK_RESP_(req,resp,Name,NAME) RADIO_NETWORK_RESP_##NAME = resp,

    /* android.hardware.radio.network.IRadioNetworkResponse v1 */
    RADIO_NETWORK_CALL_1(RADIO_NETWORK_RESP_)
    RADIO_NETWORK_RESP_ACKNOWLEDGE_REQUEST = 1, /* acknowledgeRequest */
    RADIO_NETWORK_1_RESP_LAST = RADIO_NETWORK_RESP_GET_USAGE_SETTING,

#undef RADIO_NETWORK_RESP_
} RADIO_NETWORK_RESP;
G_STATIC_ASSERT(sizeof(RADIO_NETWORK_RESP) == 4);

typedef enum radio_network_ind {
    RADIO_NETWORK_IND_ANY = 0,
    RADIO_NETWORK_IND_NONE = 0,
#define RADIO_NETWORK_IND_(code,Name,NAME) RADIO_NETWORK_IND_##NAME = code,

    /* android.hardware.radio.network.IRadioNetworkIndication v1 */
    RADIO_NETWORK_IND_1(RADIO_NETWORK_IND_)
    RADIO_NETWORK_1_IND_LAST = RADIO_NETWORK_IND_VOICE_RADIO_TECH_CHANGED,

#undef RADIO_NETWORK_IND_
} RADIO_NETWORK_IND;
G_STATIC_ASSERT(sizeof(RADIO_NETWORK_IND) == 4);

G_END_DECLS

#endif /* RADIO_NETWORK_TYPES_H */

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
