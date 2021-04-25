/*
 * Copyright (C) 2018-2021 Jolla Ltd.
 * Copyright (C) 2018-2021 Slava Monich <slava.monich@jolla.com>
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

#ifndef RADIO_TYPES_H
#define RADIO_TYPES_H

#include <gbinder_types.h>

G_BEGIN_DECLS

typedef struct radio_instance RadioInstance;
typedef struct radio_registry RadioRegistry;

#define RADIO_IFACE_PREFIX     "android.hardware.radio@"
#define RADIO_IFACE            "IRadio"
#define RADIO_RESPONSE_IFACE   "IRadioResponse"
#define RADIO_INDICATION_IFACE "IRadioIndication"
#define RADIO_IFACE_1_0(x)     RADIO_IFACE_PREFIX "1.0::" x
#define RADIO_IFACE_1_1(x)     RADIO_IFACE_PREFIX "1.1::" x
#define RADIO_IFACE_1_2(x)     RADIO_IFACE_PREFIX "1.2::" x
#define RADIO_IFACE_1_3(x)     RADIO_IFACE_PREFIX "1.3::" x
#define RADIO_IFACE_1_4(x)     RADIO_IFACE_PREFIX "1.4::" x
#define RADIO_1_0              RADIO_IFACE_1_0(RADIO_IFACE)
#define RADIO_1_1              RADIO_IFACE_1_1(RADIO_IFACE)
#define RADIO_1_2              RADIO_IFACE_1_2(RADIO_IFACE)
#define RADIO_1_3              RADIO_IFACE_1_3(RADIO_IFACE)
#define RADIO_1_4              RADIO_IFACE_1_4(RADIO_IFACE)
#define RADIO_RESPONSE_1_0     RADIO_IFACE_1_0(RADIO_RESPONSE_IFACE)
#define RADIO_RESPONSE_1_1     RADIO_IFACE_1_1(RADIO_RESPONSE_IFACE)
#define RADIO_RESPONSE_1_2     RADIO_IFACE_1_2(RADIO_RESPONSE_IFACE)
#define RADIO_RESPONSE_1_3     RADIO_IFACE_1_3(RADIO_RESPONSE_IFACE)
#define RADIO_RESPONSE_1_4     RADIO_IFACE_1_4(RADIO_RESPONSE_IFACE)
#define RADIO_INDICATION_1_0   RADIO_IFACE_1_0(RADIO_INDICATION_IFACE)
#define RADIO_INDICATION_1_1   RADIO_IFACE_1_1(RADIO_INDICATION_IFACE)
#define RADIO_INDICATION_1_2   RADIO_IFACE_1_2(RADIO_INDICATION_IFACE)
#define RADIO_INDICATION_1_3   RADIO_IFACE_1_3(RADIO_INDICATION_IFACE)
#define RADIO_INDICATION_1_4   RADIO_IFACE_1_4(RADIO_INDICATION_IFACE)

/* Types defined in types.hal */

#define RADIO_ALIGNED(x) __attribute__ ((aligned(x)))

typedef enum radio_resp_type {
    RADIO_RESP_SOLICITED,
    RADIO_RESP_SOLICITED_ACK,
    RADIO_RESP_SOLICITED_ACK_EXP
} RADIO_RESP_TYPE;
G_STATIC_ASSERT(sizeof(RADIO_RESP_TYPE) == 4);

typedef enum radio_ind_type {
    RADIO_IND_UNSOLICITED,
    RADIO_IND_ACK_EXP
} RADIO_IND_TYPE;
G_STATIC_ASSERT(sizeof(RADIO_IND_TYPE) == 4);

typedef enum radio_state {
    RADIO_STATE_OFF = 0,
    RADIO_STATE_UNAVAILABLE = 1,
    RADIO_STATE_ON = 10
} RADIO_STATE;
G_STATIC_ASSERT(sizeof(RADIO_STATE) == 4);

typedef enum radio_reg_state {
    RADIO_REG_STATE_NOT_REG_NOT_SEARCHING = 0,
    RADIO_REG_STATE_REG_HOME = 1,
    RADIO_REG_STATE_NOT_REG_MT_SEARCHING = 2,
    RADIO_REG_STATE_REG_DENIED = 3,
    RADIO_REG_STATE_UNKNOWN = 4,
    RADIO_REG_STATE_REG_ROAMING = 5,
    RADIO_REG_STATE_NOT_REG_MT_NOT_SEARCHING_EM = 10,
    RADIO_REG_STATE_NOT_REG_MT_SEARCHING_EM = 12,
    RADIO_REG_STATE_REG_DENIED_EM = 13,
    RADIO_REG_STATE_UNKNOWN_EM = 14
} RADIO_REG_STATE;
G_STATIC_ASSERT(sizeof(RADIO_REG_STATE) == 4);

typedef enum radio_ind_filter {
    RADIO_IND_FILTER_NONE = 0,
    RADIO_IND_FILTER_SIGNAL_STRENGTH = 0x01,
    RADIO_IND_FILTER_FULL_NETWORK_STATE = 0x02,
    RADIO_IND_FILTER_DATA_CALL_DORMANCY = 0x04,
    RADIO_IND_FILTER_ALL =
        RADIO_IND_FILTER_SIGNAL_STRENGTH |
        RADIO_IND_FILTER_FULL_NETWORK_STATE |
        RADIO_IND_FILTER_DATA_CALL_DORMANCY
} RADIO_IND_FILTER;
G_STATIC_ASSERT(sizeof(RADIO_IND_FILTER) == 4);

typedef enum radio_call_state {
    RADIO_CALL_ACTIVE,
    RADIO_CALL_HOLDING,
    RADIO_CALL_DIALING,
    RADIO_CALL_ALERTING,
    RADIO_CALL_INCOMING,
    RADIO_CALL_WAITING
} RADIO_CALL_STATE;
G_STATIC_ASSERT(sizeof(RADIO_CALL_STATE) == 4);

typedef enum radio_operator_status {
    RADIO_OP_STATUS_UNKNOWN = 0,
    RADIO_OP_AVAILABLE,
    RADIO_OP_CURRENT,
    RADIO_OP_FORBIDDEN
} RADIO_OP_STATUS;
G_STATIC_ASSERT(sizeof(RADIO_OP_STATUS) == 4);

typedef enum radio_cell_info_type {
    RADIO_CELL_INFO_GSM = 1,
    RADIO_CELL_INFO_CDMA,
    RADIO_CELL_INFO_LTE,
    RADIO_CELL_INFO_WCDMA,
    RADIO_CELL_INFO_TD_SCDMA
} RADIO_CELL_INFO_TYPE;
G_STATIC_ASSERT(sizeof(RADIO_CELL_INFO_TYPE) == 4);

typedef enum radio_tech {
    RADIO_TECH_UNKNOWN = 0,
    RADIO_TECH_GPRS,
    RADIO_TECH_EDGE,
    RADIO_TECH_UMTS,
    RADIO_TECH_IS95A,
    RADIO_TECH_IS95B,
    RADIO_TECH_ONE_X_RTT,
    RADIO_TECH_EVDO_0,
    RADIO_TECH_EVDO_A,
    RADIO_TECH_HSDPA,
    RADIO_TECH_HSUPA,
    RADIO_TECH_HSPA,
    RADIO_TECH_EVDO_B,
    RADIO_TECH_EHRPD,
    RADIO_TECH_LTE,
    RADIO_TECH_HSPAP,
    RADIO_TECH_GSM,
    RADIO_TECH_TD_SCDMA,
    RADIO_TECH_IWLAN,
    RADIO_TECH_LTE_CA
} RADIO_TECH;
G_STATIC_ASSERT(sizeof(RADIO_TECH) == 4);

typedef enum radio_access_family {
    RAF_UNKNOWN = (1 << RADIO_TECH_UNKNOWN),
    RAF_GPRS = (1 << RADIO_TECH_GPRS),
    RAF_EDGE = (1 << RADIO_TECH_EDGE),
    RAF_UMTS = (1 << RADIO_TECH_UMTS),
    RAF_IS95A = (1 << RADIO_TECH_IS95A),
    RAF_IS95B = (1 << RADIO_TECH_IS95B),
    RAF_ONE_X_RTT = (1 << RADIO_TECH_ONE_X_RTT),
    RAF_EVDO_0 = (1 << RADIO_TECH_EVDO_0),
    RAF_EVDO_A = (1 << RADIO_TECH_EVDO_A),
    RAF_HSDPA = (1 << RADIO_TECH_HSDPA),
    RAF_HSUPA = (1 << RADIO_TECH_HSUPA),
    RAF_HSPA = (1 << RADIO_TECH_HSPA),
    RAF_EVDO_B = (1 << RADIO_TECH_EVDO_B),
    RAF_EHRPD = (1 << RADIO_TECH_EHRPD),
    RAF_LTE = (1 << RADIO_TECH_LTE),
    RAF_HSPAP = (1 << RADIO_TECH_HSPAP),
    RAF_GSM = (1 << RADIO_TECH_GSM),
    RAF_TD_SCDMA = (1 << RADIO_TECH_TD_SCDMA),
    RAF_IWLAN = (1 << RADIO_TECH_IWLAN),
    RAF_LTE_CA = (1 << RADIO_TECH_LTE_CA)
} RADIO_ACCESS_FAMILY;
G_STATIC_ASSERT(sizeof(RADIO_ACCESS_FAMILY) == 4);

typedef enum radio_apn_auth_type {
    RADIO_APN_AUTH_NONE,
    RADIO_APN_AUTH_PAP,
    RADIO_APN_AUTH_CHAP,
    RADIO_APN_AUTH_PAP_CHAP
} RADIO_APN_AUTH_TYPE;
G_STATIC_ASSERT(sizeof(RADIO_APN_AUTH_TYPE) == 4);

typedef enum radio_apn_types {
    RADIO_APN_TYPE_NONE = 0,
    RADIO_APN_TYPE_DEFAULT = 1 << 0,
    RADIO_APN_TYPE_MMS = 1 << 1,
    RADIO_APN_TYPE_SUPL = 1 << 2,
    RADIO_APN_TYPE_DUN = 1 << 3,
    RADIO_APN_TYPE_HIPRI = 1 << 4,
    RADIO_APN_TYPE_FOTA = 1 << 5,
    RADIO_APN_TYPE_IMS = 1 << 6,
    RADIO_APN_TYPE_CBS = 1 << 7,
    RADIO_APN_TYPE_IA = 1 << 8,
    RADIO_APN_TYPE_EMERGENCY = 1 << 9,
    RADIO_APN_TYPE_MCX = 1 << 10, /* Since 1.2.5 */
    RADIO_APN_TYPE_ALL = RADIO_APN_TYPE_DEFAULT | RADIO_APN_TYPE_MMS |
        RADIO_APN_TYPE_SUPL | RADIO_APN_TYPE_DUN | RADIO_APN_TYPE_HIPRI |
        RADIO_APN_TYPE_FOTA | RADIO_APN_TYPE_IMS | RADIO_APN_TYPE_CBS |
        RADIO_APN_TYPE_IA | RADIO_APN_TYPE_EMERGENCY | RADIO_APN_TYPE_MCX
} RADIO_APN_TYPES;
G_STATIC_ASSERT(sizeof(RADIO_APN_TYPES) == 4);

typedef enum radio_data_profile_id {
    RADIO_DATA_PROFILE_INVALID = -1,
    RADIO_DATA_PROFILE_DEFAULT = 0,
    RADIO_DATA_PROFILE_TETHERED = 1,
    RADIO_DATA_PROFILE_IMS = 2,
    RADIO_DATA_PROFILE_FOTA = 3,
    RADIO_DATA_PROFILE_CBS = 4,
    RADIO_DATA_PROFILE_OEM_BASE = 1000
} RADIO_DATA_PROFILE_ID;
G_STATIC_ASSERT(sizeof(RADIO_DATA_PROFILE_ID) == 4);

typedef enum radio_card_state {
    RADIO_CARD_STATE_ABSENT,
    RADIO_CARD_STATE_PRESENT,
    RADIO_CARD_STATE_ERROR,
    RADIO_CARD_STATE_RESTRICTED
} RADIO_CARD_STATE;
G_STATIC_ASSERT(sizeof(RADIO_CARD_STATE) == 4);

typedef enum radio_pin_state {
    RADIO_PIN_STATE_UNKNOWN,
    RADIO_PIN_STATE_ENABLED_NOT_VERIFIED,
    RADIO_PIN_STATE_ENABLED_VERIFIED,
    RADIO_PIN_STATE_DISABLED,
    RADIO_PIN_STATE_ENABLED_BLOCKED,
    RADIO_PIN_STATE_ENABLED_PERM_BLOCKED
} RADIO_PIN_STATE;
G_STATIC_ASSERT(sizeof(RADIO_PIN_STATE) == 4);

typedef enum radio_app_type {
    RADIO_APP_TYPE_UNKNOWN,
    RADIO_APP_TYPE_SIM,
    RADIO_APP_TYPE_USIM,
    RADIO_APP_TYPE_RUIM,
    RADIO_APP_TYPE_CSIM,
    RADIO_APP_TYPE_ISIM
} RADIO_APP_TYPE;
G_STATIC_ASSERT(sizeof(RADIO_APP_TYPE) == 4);

typedef enum radio_app_state {
    RADIO_APP_STATE_UNKNOWN,
    RADIO_APP_STATE_DETECTED,
    RADIO_APP_STATE_PIN,
    RADIO_APP_STATE_PUK,
    RADIO_APP_STATE_SUBSCRIPTION_PERSO,
    RADIO_APP_STATE_READY
} RADIO_APP_STATE;
G_STATIC_ASSERT(sizeof(RADIO_APP_STATE) == 4);

typedef enum radio_perso_substate {
    RADIO_PERSO_SUBSTATE_UNKNOWN,
    RADIO_PERSO_SUBSTATE_IN_PROGRESS,
    RADIO_PERSO_SUBSTATE_READY,
    RADIO_PERSO_SUBSTATE_SIM_NETWORK,
    RADIO_PERSO_SUBSTATE_SIM_NETWORK_SUBSET,
    RADIO_PERSO_SUBSTATE_SIM_CORPORATE,
    RADIO_PERSO_SUBSTATE_SIM_SERVICE_PROVIDER,
    RADIO_PERSO_SUBSTATE_SIM_SIM,
    RADIO_PERSO_SUBSTATE_SIM_NETWORK_PUK,
    RADIO_PERSO_SUBSTATE_SIM_NETWORK_SUBSET_PUK,
    RADIO_PERSO_SUBSTATE_SIM_CORPORATE_PUK,
    RADIO_PERSO_SUBSTATE_SIM_SERVICE_PROVIDER_PUK,
    RADIO_PERSO_SUBSTATE_SIM_SIM_PUK,
    RADIO_PERSO_SUBSTATE_RUIM_NETWORK1,
    RADIO_PERSO_SUBSTATE_RUIM_NETWORK2,
    RADIO_PERSO_SUBSTATE_RUIM_HRPD,
    RADIO_PERSO_SUBSTATE_RUIM_CORPORATE,
    RADIO_PERSO_SUBSTATE_RUIM_SERVICE_PROVIDER,
    RADIO_PERSO_SUBSTATE_RUIM_RUIM,
    RADIO_PERSO_SUBSTATE_RUIM_NETWORK1_PUK,
    RADIO_PERSO_SUBSTATE_RUIM_NETWORK2_PUK,
    RADIO_PERSO_SUBSTATE_RUIM_HRPD_PUK,
    RADIO_PERSO_SUBSTATE_RUIM_CORPORATE_PUK,
    RADIO_PERSO_SUBSTATE_RUIM_SERVICE_PROVIDER_PUK,
    RADIO_PERSO_SUBSTATE_RUIM_RUIM_PUK,
} RADIO_PERSO_SUBSTATE;
G_STATIC_ASSERT(sizeof(RADIO_PERSO_SUBSTATE) == 4);

typedef enum radio_capability_phase {
    RADIO_CAPABILITY_PHASE_CONFIGURED,
    RADIO_CAPABILITY_PHASE_START,
    RADIO_CAPABILITY_PHASE_APPLY,
    RADIO_CAPABILITY_PHASE_UNSOL_RSP,
    RADIO_CAPABILITY_PHASE_FINISH
} RADIO_CAPABILITY_PHASE;
G_STATIC_ASSERT(sizeof(RADIO_CAPABILITY_PHASE) == 4);

typedef enum radio_capability_status {
    RADIO_CAPABILITY_STATUS_NONE,
    RADIO_CAPABILITY_STATUS_SUCCESS,
    RADIO_CAPABILITY_STATUS_FAIL
} RADIO_CAPABILITY_STATUS;
G_STATIC_ASSERT(sizeof(RADIO_CAPABILITY_STATUS) == 4);

typedef enum radio_device_state {
    RADIO_DEVICE_STATE_POWER_SAVE_MODE,
    RADIO_DEVICE_STATE_CHARGING_STATE,
    RADIO_DEVICE_STATE_LOW_DATA_EXPECTED
} RADIO_DEVICE_STATE;
G_STATIC_ASSERT(sizeof(RADIO_DEVICE_STATE) == 4);

typedef enum radio_data_request_reason {
    RADIO_DATA_REQUEST_REASON_NORMAL = 1,
    RADIO_DATA_REQUEST_REASON_SHUTDOWN,
    RADIO_DATA_REQUEST_REASON_HANDOVER
} RADIO_DATA_REQUEST_REASON; /* Since 1.2.0 */
G_STATIC_ASSERT(sizeof(RADIO_DATA_REQUEST_REASON) == 4);

typedef enum radio_access_network {
    RADIO_ACCESS_NETWORK_UNKNOWN,
    RADIO_ACCESS_NETWORK_GERAN,
    RADIO_ACCESS_NETWORK_UTRAN,
    RADIO_ACCESS_NETWORK_EUTRAN,
    RADIO_ACCESS_NETWORK_CDMA2000,
    RADIO_ACCESS_NETWORK_IWLAN
} RADIO_ACCESS_NETWORK; /* Since 1.2.0 */
G_STATIC_ASSERT(sizeof(RADIO_ACCESS_NETWORK) == 4);

typedef enum radio_data_profile_type {
    RADIO_DATA_PROFILE_COMMON,
    RADIO_DATA_PROFILE_3GPP,
    RADIO_DATA_PROFILE_3GPP2
} RADIO_DATA_PROFILE_TYPE; /* Since 1.2.5 */
G_STATIC_ASSERT(sizeof(RADIO_DATA_PROFILE_TYPE) == 4);

typedef enum radio_pdp_protocol_type {
    RADIO_PDP_PROTOCOL_UNKNOWN = -1,
    RADIO_PDP_PROTOCOL_IP,
    RADIO_PDP_PROTOCOL_IPV6,
    RADIO_PDP_PROTOCOL_IPV4V6,
    RADIO_PDP_PROTOCOL_PPP,
    RADIO_PDP_PROTOCOL_NON_IP,
    RADIO_PDP_PROTOCOL_UNSTRUCTURED
} RADIO_PDP_PROTOCOL_TYPE; /* Since 1.2.5 */
G_STATIC_ASSERT(sizeof(RADIO_PDP_PROTOCOL_TYPE) == 4);

typedef enum radio_emergency_service_category {
    RADIO_EMERGENCY_SERVICE_UNSPECIFIED = 0,
    RADIO_EMERGENCY_SERVICE_POLICE = 1 << 0,
    RADIO_EMERGENCY_SERVICE_AMBULANCE = 1 << 1,
    RADIO_EMERGENCY_SERVICE_FIRE_BRIGADE = 1 << 2,
    RADIO_EMERGENCY_SERVICE_MARINE_GUARD = 1 << 3,
    RADIO_EMERGENCY_SERVICE_MOUNTAIN_RESCUE = 1 << 4,
    RADIO_EMERGENCY_SERVICE_MIEC = 1 << 5,
    RADIO_EMERGENCY_SERVICE_AIEC = 1 << 6
} RADIO_EMERGENCY_SERVICE_CATEGORY; /* Since 1.2.5 */
G_STATIC_ASSERT(sizeof(RADIO_EMERGENCY_SERVICE_CATEGORY) == 4);

typedef enum radio_emergency_number_source {
    RADIO_EMERGENCY_NUMBER_NETWORK_SIGNALING = 1 << 0,
    RADIO_EMERGENCY_NUMBER_SIM = 1 << 1,
    RADIO_EMERGENCY_NUMBER_MODEM_CONFIG = 1 << 2,
    RADIO_EMERGENCY_NUMBER_DEFAULT = 1 << 3
} RADIO_EMERGENCY_NUMBER_SOURCE; /* Since 1.2.5 */
G_STATIC_ASSERT(sizeof(RADIO_EMERGENCY_NUMBER_SOURCE) == 4);

typedef enum radio_cell_connection_status {
    RADIO_CELL_CONNECTION_NONE,
    RADIO_CELL_CONNECTION_PRIMARY_SERVING,
    RADIO_CELL_CONNECTION_SECONDARY_SERVING
} RADIO_CELL_CONNECTION_STATUS; /* Since 1.2.5 */
G_STATIC_ASSERT(sizeof(RADIO_CELL_CONNECTION_STATUS) == 4);

typedef enum radio_scan_status {
    RADIO_SCAN_PARTIAL = 1,
    RADIO_SCAN_COMPLETE = 2
} RADIO_SCAN_STATUS; /* Since 1.2.5 */
G_STATIC_ASSERT(sizeof(RADIO_SCAN_STATUS) == 4);

typedef struct radio_response_info {
    RADIO_RESP_TYPE type RADIO_ALIGNED(4);
    guint32 serial RADIO_ALIGNED(4);
    guint32 error RADIO_ALIGNED(4);
} RadioResponseInfo;
G_STATIC_ASSERT(sizeof(RadioResponseInfo) == 12);

typedef struct radio_card_status {
    RADIO_CARD_STATE cardState RADIO_ALIGNED(4);
    RADIO_PIN_STATE universalPinState RADIO_ALIGNED(4);
    gint32 gsmUmtsSubscriptionAppIndex RADIO_ALIGNED(4);
    gint32 cdmaSubscriptionAppIndex RADIO_ALIGNED(4);
    gint32 imsSubscriptionAppIndex RADIO_ALIGNED(4);
    GBinderHidlVec apps RADIO_ALIGNED(8); /* vec<RadioAppStatus> */
} RADIO_ALIGNED(8) RadioCardStatus;
G_STATIC_ASSERT(sizeof(RadioCardStatus) == 40);

typedef struct radio_card_status_1_2 {
    RadioCardStatus base RADIO_ALIGNED(8);
    gint32 physicalSlotId RADIO_ALIGNED(4);
    GBinderHidlString atr RADIO_ALIGNED(8);
    GBinderHidlString iccid RADIO_ALIGNED(8);
} RADIO_ALIGNED(8) RadioCardStatus_1_2; /* Since 1.2.3 */
G_STATIC_ASSERT(sizeof(RadioCardStatus_1_2) == 80);

typedef struct radio_app_status {
    RADIO_APP_TYPE appType RADIO_ALIGNED(4);
    RADIO_APP_STATE appState RADIO_ALIGNED(4);
    RADIO_PERSO_SUBSTATE persoSubstate RADIO_ALIGNED(4);
    GBinderHidlString aid RADIO_ALIGNED(8);
    GBinderHidlString label RADIO_ALIGNED(8);
    gint32 pinReplaced RADIO_ALIGNED(4);
    RADIO_PIN_STATE pin1 RADIO_ALIGNED(4);
    RADIO_PIN_STATE pin2 RADIO_ALIGNED(4);
} RADIO_ALIGNED(8) RadioAppStatus;
G_STATIC_ASSERT(sizeof(RadioAppStatus) == 64);

typedef struct radio_uus_info {
    gint32 uusType RADIO_ALIGNED(4);
    gint32 uusDcs RADIO_ALIGNED(4);
    GBinderHidlString uusData RADIO_ALIGNED(8);
} RADIO_ALIGNED(8) RadioUusInfo;
G_STATIC_ASSERT(sizeof(RadioUusInfo) == 24);

typedef struct radio_call {
    RADIO_CALL_STATE state RADIO_ALIGNED(4);
    gint32 index RADIO_ALIGNED(4);
    gint32 toa RADIO_ALIGNED(4);
    guint8 isMpty RADIO_ALIGNED(1);
    guint8 isMT RADIO_ALIGNED(1);
    guint8 als RADIO_ALIGNED(1);
    guint8 isVoice RADIO_ALIGNED(1);
    guint8 isVoicePrivacy RADIO_ALIGNED(1);
    GBinderHidlString number RADIO_ALIGNED(8);
    gint32 numberPresentation RADIO_ALIGNED(4);
    GBinderHidlString name RADIO_ALIGNED(8);
    gint32 namePresentation RADIO_ALIGNED(4);
    GBinderHidlVec uusInfo RADIO_ALIGNED(8); /* vec<RadioUusInfo> */
} RADIO_ALIGNED(8) RadioCall;
G_STATIC_ASSERT(sizeof(RadioCall) == 88);

typedef struct radio_call_1_2 {
    RadioCall base RADIO_ALIGNED(8);
    gint32 audioQuality RADIO_ALIGNED(4);
} RADIO_ALIGNED(8) RadioCall_1_2; /* Since 1.2.3 */
G_STATIC_ASSERT(sizeof(RadioCall_1_2) == 96);

typedef struct radio_dial {
    GBinderHidlString address RADIO_ALIGNED(8);
    gint32 clir RADIO_ALIGNED(4);
    GBinderHidlVec uusInfo RADIO_ALIGNED(8); /* vec<RadioUusInfo> */
} RADIO_ALIGNED(8) RadioDial;
G_STATIC_ASSERT(sizeof(RadioDial) == 40);

typedef struct radio_last_call_fail_cause_info {
    gint32 causeCode RADIO_ALIGNED(4);
    GBinderHidlString vendorCause RADIO_ALIGNED(8);
} RADIO_ALIGNED(8) RadioLastCallFailCauseInfo;
G_STATIC_ASSERT(sizeof(RadioLastCallFailCauseInfo) == 24);

typedef struct radio_operator_info {
    GBinderHidlString alphaLong RADIO_ALIGNED(8);
    GBinderHidlString alphaShort RADIO_ALIGNED(8);
    GBinderHidlString operatorNumeric RADIO_ALIGNED(8);
    RADIO_OP_STATUS status RADIO_ALIGNED(4);
} RADIO_ALIGNED(8) RadioOperatorInfo;
G_STATIC_ASSERT(sizeof(RadioOperatorInfo) == 56);

typedef struct radio_data_profile {
    RADIO_DATA_PROFILE_ID profileId RADIO_ALIGNED(4);
    GBinderHidlString apn RADIO_ALIGNED(8);
    GBinderHidlString protocol RADIO_ALIGNED(8);
    GBinderHidlString roamingProtocol RADIO_ALIGNED(8);
    RADIO_APN_AUTH_TYPE authType RADIO_ALIGNED(4);
    GBinderHidlString user RADIO_ALIGNED(8);
    GBinderHidlString password RADIO_ALIGNED(8);
    RADIO_DATA_PROFILE_TYPE type RADIO_ALIGNED(4);
    gint32 maxConnsTime RADIO_ALIGNED(4);
    gint32 maxConns RADIO_ALIGNED(4);
    gint32 waitTime RADIO_ALIGNED(4);
    guint8 enabled RADIO_ALIGNED(1);
    RADIO_APN_TYPES supportedApnTypesBitmap RADIO_ALIGNED(4);
    RADIO_ACCESS_FAMILY bearerBitmap RADIO_ALIGNED(4);
    gint32 mtu RADIO_ALIGNED(4);
    gint32 mvnoType RADIO_ALIGNED(4);
    GBinderHidlString mvnoMatchData RADIO_ALIGNED(8);
} RADIO_ALIGNED(8) RadioDataProfile;
G_STATIC_ASSERT(sizeof(RadioDataProfile) == 152);

typedef struct radio_data_profile_1_4 {
    RADIO_DATA_PROFILE_ID profileId RADIO_ALIGNED(4);
    GBinderHidlString apn RADIO_ALIGNED(8);
    RADIO_PDP_PROTOCOL_TYPE protocol RADIO_ALIGNED(4);
    RADIO_PDP_PROTOCOL_TYPE roamingProtocol RADIO_ALIGNED(4);
    RADIO_APN_AUTH_TYPE authType RADIO_ALIGNED(4);
    GBinderHidlString user RADIO_ALIGNED(8);
    GBinderHidlString password RADIO_ALIGNED(8);
    RADIO_DATA_PROFILE_TYPE type RADIO_ALIGNED(4);
    gint32 maxConnsTime RADIO_ALIGNED(4);
    gint32 maxConns RADIO_ALIGNED(4);
    gint32 waitTime RADIO_ALIGNED(4);
    guint8 enabled RADIO_ALIGNED(1);
    RADIO_APN_TYPES supportedApnTypesBitmap RADIO_ALIGNED(4);
    RADIO_ACCESS_FAMILY bearerBitmap RADIO_ALIGNED(4);
    gint32 mtu RADIO_ALIGNED(4);
    guint8 preferred RADIO_ALIGNED(1);
    guint8 persistent RADIO_ALIGNED(1);
} RADIO_ALIGNED(8) RadioDataProfile_1_4; /* Since 1.2.5 */
G_STATIC_ASSERT(sizeof(RadioDataProfile_1_4) == 112);

typedef struct radio_data_call {
    gint32 status RADIO_ALIGNED(4);
    gint32 suggestedRetryTime RADIO_ALIGNED(4);
    gint32 cid RADIO_ALIGNED(4);
    gint32 active RADIO_ALIGNED(4);
    GBinderHidlString type RADIO_ALIGNED(8);
    GBinderHidlString ifname RADIO_ALIGNED(8);
    GBinderHidlString addresses RADIO_ALIGNED(8);
    GBinderHidlString dnses RADIO_ALIGNED(8);
    GBinderHidlString gateways RADIO_ALIGNED(8);
    GBinderHidlString pcscf RADIO_ALIGNED(8);
    gint32 mtu RADIO_ALIGNED(4);
} RADIO_ALIGNED(8) RadioDataCall;
G_STATIC_ASSERT(sizeof(RadioDataCall) == 120);

typedef struct radio_data_call_1_4 {
    gint32 cause RADIO_ALIGNED(4);
    gint32 suggestedRetryTime RADIO_ALIGNED(4);
    gint32 cid RADIO_ALIGNED(4);
    gint32 active RADIO_ALIGNED(4);
    RADIO_PDP_PROTOCOL_TYPE type RADIO_ALIGNED(4);
    GBinderHidlString ifname RADIO_ALIGNED(8);
    GBinderHidlVec addresses RADIO_ALIGNED(8); /* vec<GBinderHidlString> */
    GBinderHidlVec dnses RADIO_ALIGNED(8);     /* vec<GBinderHidlString> */
    GBinderHidlVec gateways RADIO_ALIGNED(8);  /* vec<GBinderHidlString> */
    GBinderHidlVec pcscf RADIO_ALIGNED(8);     /* vec<GBinderHidlString> */
    gint32 mtu RADIO_ALIGNED(4);
} RADIO_ALIGNED(8) RadioDataCall_1_4; /* Since 1.2.5 */
G_STATIC_ASSERT(sizeof(RadioDataCall_1_4) == 112);

#define DATA_CALL_VERSION (11)

typedef struct radio_sms_write_args {
    gint32 status RADIO_ALIGNED(4);
    GBinderHidlString pdu RADIO_ALIGNED(8);
    GBinderHidlString smsc RADIO_ALIGNED(8);
} RADIO_ALIGNED(8) RadioSmsWriteArgs;
G_STATIC_ASSERT(sizeof(RadioSmsWriteArgs) == 40);

typedef struct radio_gsm_sms_message {
    GBinderHidlString smscPdu RADIO_ALIGNED(8);
    GBinderHidlString pdu RADIO_ALIGNED(8);
} RADIO_ALIGNED(8) RadioGsmSmsMessage;
G_STATIC_ASSERT(sizeof(RadioGsmSmsMessage) == 32);

typedef struct radio_send_sms_result {
    gint32 messageRef RADIO_ALIGNED(4);
    GBinderHidlString ackPDU RADIO_ALIGNED(8);
    gint32 errorCode RADIO_ALIGNED(4);
} RADIO_ALIGNED(8) RadioSendSmsResult;
G_STATIC_ASSERT(sizeof(RadioSendSmsResult) == 32);

typedef struct radio_icc_io {
    gint32 command RADIO_ALIGNED(4);
    gint32 fileId RADIO_ALIGNED(4);
    GBinderHidlString path RADIO_ALIGNED(8);
    gint32 p1 RADIO_ALIGNED(4);
    gint32 p2 RADIO_ALIGNED(4);
    gint32 p3 RADIO_ALIGNED(4);
    GBinderHidlString data RADIO_ALIGNED(8);
    GBinderHidlString pin2 RADIO_ALIGNED(8);
    GBinderHidlString aid RADIO_ALIGNED(8);
} RADIO_ALIGNED(8) RadioIccIo;
G_STATIC_ASSERT(sizeof(RadioIccIo) == 88);

typedef struct radio_sim_apdu {
    gint32 sessionId RADIO_ALIGNED(4);
    gint32 cla RADIO_ALIGNED(4);
    gint32 instruction RADIO_ALIGNED(4);
    gint32 p1 RADIO_ALIGNED(4);
    gint32 p2 RADIO_ALIGNED(4);
    gint32 p3 RADIO_ALIGNED(4);
    GBinderHidlString data RADIO_ALIGNED(8);
} RADIO_ALIGNED(8) RadioSimApdu;
G_STATIC_ASSERT(sizeof(RadioSimApdu) == 40); /* Since 1.2.6 */

typedef struct radio_icc_io_result {
    gint32 sw1 RADIO_ALIGNED(4);
    gint32 sw2 RADIO_ALIGNED(4);
    GBinderHidlString response RADIO_ALIGNED(8);
} RADIO_ALIGNED(8) RadioIccIoResult;
G_STATIC_ASSERT(sizeof(RadioIccIoResult) == 24);

typedef struct radio_call_forward_info {
    gint32 status RADIO_ALIGNED(4);
    gint32 reason RADIO_ALIGNED(4);
    gint32 serviceClass RADIO_ALIGNED(4);
    gint32 toa RADIO_ALIGNED(4);
    GBinderHidlString number RADIO_ALIGNED(8);
    gint32 timeSeconds RADIO_ALIGNED(4);
} RADIO_ALIGNED(8) RadioCallForwardInfo;
G_STATIC_ASSERT(sizeof(RadioCallForwardInfo) == 40);

typedef struct radio_emergency_number {
    GBinderHidlString number RADIO_ALIGNED(8);
    GBinderHidlString mcc RADIO_ALIGNED(8);
    GBinderHidlString mnc RADIO_ALIGNED(8);
    RADIO_EMERGENCY_SERVICE_CATEGORY categories RADIO_ALIGNED(4);
    GBinderHidlVec urns RADIO_ALIGNED(8); /* vec<GBinderHidlString> */
    RADIO_EMERGENCY_NUMBER_SOURCE sources RADIO_ALIGNED(4);
} RADIO_ALIGNED(8) RadioEmergencyNumber; /* Since 1.2.5 */
G_STATIC_ASSERT(sizeof(RadioEmergencyNumber) == 80);

#define RADIO_CELL_INVALID_VALUE (INT_MAX)

typedef struct radio_cell_identity {
    RADIO_CELL_INFO_TYPE cellInfoType RADIO_ALIGNED(4);
    GBinderHidlVec gsm RADIO_ALIGNED(8);     /* vec<RadioCellIdentityGsm> */
    GBinderHidlVec wcdma RADIO_ALIGNED(8);   /* vec<RadioCellIdentityWcdma> */
    GBinderHidlVec cdma RADIO_ALIGNED(8);    /* vec<RadioCellIdentityCdma> */
    GBinderHidlVec lte RADIO_ALIGNED(8);     /* vec<RadioCellIdentityLte> */
    GBinderHidlVec tdscdma RADIO_ALIGNED(8); /* vec<RadioCellIdentityTdscdma> */
} RADIO_ALIGNED(8) RadioCellIdentity;
G_STATIC_ASSERT(sizeof(RadioCellIdentity) == 88);

typedef struct radio_cell_identity_1_2 {
    RADIO_CELL_INFO_TYPE cellInfoType RADIO_ALIGNED(4);
    GBinderHidlVec gsm RADIO_ALIGNED(8);   /* vec<RadioCellIdentityGsm_1_2> */
    GBinderHidlVec wcdma RADIO_ALIGNED(8); /* vec<RadioCellIdentityWcdma_1_2> */
    GBinderHidlVec cdma RADIO_ALIGNED(8);  /* vec<RadioCellIdentityCdma_1_2> */
    GBinderHidlVec lte RADIO_ALIGNED(8);   /* vec<RadioCellIdentityLte_1_2> */
    GBinderHidlVec tdscdma RADIO_ALIGNED(8);/*vec<RadioCellIdentityTdscdma_1_2>*/
} RADIO_ALIGNED(8) RadioCellIdentity_1_2;  /* Since 1.2.4 */
G_STATIC_ASSERT(sizeof(RadioCellIdentity_1_2) == 88);

typedef struct radio_cell_info {
    RADIO_CELL_INFO_TYPE cellInfoType RADIO_ALIGNED(4);
    guint8 registered RADIO_ALIGNED(1);
    gint32 timeStampType RADIO_ALIGNED(4);
    guint64 timeStamp RADIO_ALIGNED(8);
    GBinderHidlVec gsm RADIO_ALIGNED(8);     /* vec<RadioCellInfoGsm> */
    GBinderHidlVec cdma RADIO_ALIGNED(8);    /* vec<RadioCellInfoCdma> */
    GBinderHidlVec lte RADIO_ALIGNED(8);     /* vec<RadioCellInfoLte> */
    GBinderHidlVec wcdma RADIO_ALIGNED(8);   /* vec<RadioCellInfoWcdma> */
    GBinderHidlVec tdscdma RADIO_ALIGNED(8); /* vec<RadioCellInfoTdscdma> */
} RADIO_ALIGNED(8) RadioCellInfo;
G_STATIC_ASSERT(sizeof(RadioCellInfo) == 104);

typedef struct radio_cell_info_1_2 {
    RADIO_CELL_INFO_TYPE cellInfoType RADIO_ALIGNED(4);
    guint8 registered RADIO_ALIGNED(1);
    gint32 timeStampType RADIO_ALIGNED(4);
    guint64 timeStamp RADIO_ALIGNED(8);
    GBinderHidlVec gsm RADIO_ALIGNED(8);     /* vec<RadioCellInfoGsm> */
    GBinderHidlVec cdma RADIO_ALIGNED(8);    /* vec<RadioCellInfoCdma> */
    GBinderHidlVec lte RADIO_ALIGNED(8);     /* vec<RadioCellInfoLte> */
    GBinderHidlVec wcdma RADIO_ALIGNED(8);   /* vec<RadioCellInfoWcdma> */
    GBinderHidlVec tdscdma RADIO_ALIGNED(8); /* vec<RadioCellInfoTdscdma> */
    RADIO_CELL_CONNECTION_STATUS connectionStatus RADIO_ALIGNED(4);
} RADIO_ALIGNED(8) RadioCellInfo_1_2; /* Since 1.2.0 */
G_STATIC_ASSERT(sizeof(RadioCellInfo_1_2) == 112);

typedef struct radio_cell_identity_operator_names {
    GBinderHidlString alphaLong RADIO_ALIGNED(8);
    GBinderHidlString alphaShort RADIO_ALIGNED(8);
} RADIO_ALIGNED(8) RadioCellIdentityOperatorNames; /* Since 1.2.0 */
G_STATIC_ASSERT(sizeof(RadioCellIdentityOperatorNames) == 32);

typedef struct radio_cell_identity_gsm {
    GBinderHidlString mcc RADIO_ALIGNED(8);
    GBinderHidlString mnc RADIO_ALIGNED(8);
    gint32 lac RADIO_ALIGNED(4);
    gint32 cid RADIO_ALIGNED(4);
    gint32 arfcn RADIO_ALIGNED(4);
    guint8 bsic RADIO_ALIGNED(1);
} RADIO_ALIGNED(8) RadioCellIdentityGsm;
G_STATIC_ASSERT(sizeof(RadioCellIdentityGsm) == 48);

typedef struct radio_cell_identity_gsm_1_2 {
    RadioCellIdentityGsm base RADIO_ALIGNED(8);
    RadioCellIdentityOperatorNames operatorNames RADIO_ALIGNED(8);
} RADIO_ALIGNED(8) RadioCellIdentityGsm_1_2; /* Since 1.2.3 */
G_STATIC_ASSERT(sizeof(RadioCellIdentityGsm_1_2) == 80);

typedef struct radio_cell_identity_wcdma {
    GBinderHidlString mcc RADIO_ALIGNED(8);
    GBinderHidlString mnc RADIO_ALIGNED(8);
    gint32 lac RADIO_ALIGNED(4);
    gint32 cid RADIO_ALIGNED(4);
    gint32 psc RADIO_ALIGNED(4);
    gint32 uarfcn RADIO_ALIGNED(4);
} RADIO_ALIGNED(8) RadioCellIdentityWcdma;
G_STATIC_ASSERT(sizeof(RadioCellIdentityWcdma) == 48);

typedef struct radio_cell_identity_wcdma_1_2 {
    RadioCellIdentityWcdma base RADIO_ALIGNED(8);
    RadioCellIdentityOperatorNames operatorNames RADIO_ALIGNED(8);
} RADIO_ALIGNED(8) RadioCellIdentityWcdma_1_2; /* Since 1.2.3 */
G_STATIC_ASSERT(sizeof(RadioCellIdentityWcdma_1_2) == 80);

typedef struct radio_cell_identity_cdma {
    gint32 networkId RADIO_ALIGNED(4);
    gint32 systemId RADIO_ALIGNED(4);
    gint32 baseStationId RADIO_ALIGNED(4);
    gint32 longitude RADIO_ALIGNED(4);
    gint32 latitude RADIO_ALIGNED(4);
} RADIO_ALIGNED(4) RadioCellIdentityCdma;
G_STATIC_ASSERT(sizeof(RadioCellIdentityCdma) == 20);

typedef struct radio_cell_identity_cdma_1_2 {
    RadioCellIdentityCdma base RADIO_ALIGNED(4);
    RadioCellIdentityOperatorNames operatorNames RADIO_ALIGNED(8);
} RADIO_ALIGNED(4) RadioCellIdentityCdma_1_2; /* Since 1.2.3 */
G_STATIC_ASSERT(sizeof(RadioCellIdentityCdma_1_2) == 56);

typedef struct radio_cell_identity_lte {
    GBinderHidlString mcc RADIO_ALIGNED(8);
    GBinderHidlString mnc RADIO_ALIGNED(8);
    gint32 ci RADIO_ALIGNED(4);
    gint32 pci RADIO_ALIGNED(4);
    gint32 tac RADIO_ALIGNED(4);
    gint32 earfcn RADIO_ALIGNED(4);
} RADIO_ALIGNED(8) RadioCellIdentityLte;
G_STATIC_ASSERT(sizeof(RadioCellIdentityLte) == 48);

typedef struct radio_cell_identity_lte_1_2 {
    RadioCellIdentityLte base RADIO_ALIGNED(8);
    RadioCellIdentityOperatorNames operatorNames RADIO_ALIGNED(8);
    gint32 bandwidth RADIO_ALIGNED(4);
} RADIO_ALIGNED(8) RadioCellIdentityLte_1_2; /* Since 1.2.3 */
G_STATIC_ASSERT(sizeof(RadioCellIdentityLte_1_2) == 88);

typedef struct radio_cell_identity_tdscdma {
    GBinderHidlString mcc RADIO_ALIGNED(8);
    GBinderHidlString mnc RADIO_ALIGNED(8);
    gint32 lac RADIO_ALIGNED(4);
    gint32 cid RADIO_ALIGNED(4);
    gint32 cpid RADIO_ALIGNED(4);
} RADIO_ALIGNED(8) RadioCellIdentityTdscdma;
G_STATIC_ASSERT(sizeof(RadioCellIdentityTdscdma) == 48);

typedef struct radio_cell_identity_tdscdma_1_2 {
    RadioCellIdentityTdscdma base RADIO_ALIGNED(8);
    gint32 uarfcn RADIO_ALIGNED(8);
    RadioCellIdentityOperatorNames operatorNames RADIO_ALIGNED(8);
} RADIO_ALIGNED(8) RadioCellIdentityTdscdma_1_2; /* Since 1.2.3 */
G_STATIC_ASSERT(sizeof(RadioCellIdentityTdscdma_1_2) == 88);

typedef struct radio_cell_identity_nr {
    GBinderHidlString mcc RADIO_ALIGNED(8);
    GBinderHidlString mnc RADIO_ALIGNED(8);
    guint64 nci RADIO_ALIGNED(8);
    guint32 pci RADIO_ALIGNED(4);
    gint32 tac RADIO_ALIGNED(4);
    gint32 nrarfcn RADIO_ALIGNED(4);
    RadioCellIdentityOperatorNames operatorNames RADIO_ALIGNED(8);
} RADIO_ALIGNED(8) RadioCellIdentityNr; /* Since 1.2.5 */
G_STATIC_ASSERT(sizeof(RadioCellIdentityNr) == 88);

typedef struct radio_voice_reg_state_result {
    RADIO_REG_STATE regState RADIO_ALIGNED(4);
    RADIO_TECH rat RADIO_ALIGNED(4);
    guint8 cssSupported RADIO_ALIGNED(1);
    gint32 roamingIndicator RADIO_ALIGNED(4);
    gint32 systemIsInPrl RADIO_ALIGNED(4);
    gint32 defaultRoamingIndicator RADIO_ALIGNED(4);
    gint32 reasonForDenial RADIO_ALIGNED(4);
    RadioCellIdentity cellIdentity RADIO_ALIGNED(8);
} RADIO_ALIGNED(8) RadioVoiceRegStateResult;
G_STATIC_ASSERT(sizeof(RadioVoiceRegStateResult) == 120);

typedef struct radio_voice_reg_state_result_1_2 {
    RADIO_REG_STATE regState RADIO_ALIGNED(4);
    RADIO_TECH rat RADIO_ALIGNED(4);
    guint8 cssSupported RADIO_ALIGNED(1);
    gint32 roamingIndicator RADIO_ALIGNED(4);
    gint32 systemIsInPrl RADIO_ALIGNED(4);
    gint32 defaultRoamingIndicator RADIO_ALIGNED(4);
    gint32 reasonForDenial RADIO_ALIGNED(4);
    RadioCellIdentity_1_2 cellIdentity RADIO_ALIGNED(8);
} RADIO_ALIGNED(8) RadioVoiceRegStateResult_1_2;  /* Since 1.2.4 */
G_STATIC_ASSERT(sizeof(RadioVoiceRegStateResult_1_2) == 120);

typedef struct radio_data_reg_state_result {
    RADIO_REG_STATE regState RADIO_ALIGNED(4);
    RADIO_TECH rat RADIO_ALIGNED(4);
    gint32 reasonDataDenied RADIO_ALIGNED(4);
    gint32 maxDataCalls RADIO_ALIGNED(4);
    RadioCellIdentity cellIdentity RADIO_ALIGNED(8);
} RADIO_ALIGNED(8) RadioDataRegStateResult;
G_STATIC_ASSERT(sizeof(RadioDataRegStateResult) == 104);

typedef struct radio_data_reg_state_result_1_2 {
    RADIO_REG_STATE regState RADIO_ALIGNED(4);
    RADIO_TECH rat RADIO_ALIGNED(4);
    gint32 reasonDataDenied RADIO_ALIGNED(4);
    gint32 maxDataCalls RADIO_ALIGNED(4);
    RadioCellIdentity_1_2 cellIdentity RADIO_ALIGNED(8);
} RADIO_ALIGNED(8) RadioDataRegStateResult_1_2;  /* Since 1.2.4 */
G_STATIC_ASSERT(sizeof(RadioDataRegStateResult_1_2) == 104);

typedef struct radio_signal_strength_gsm {
    guint32 signalStrength RADIO_ALIGNED(4);
    guint32 bitErrorRate RADIO_ALIGNED(4);
    gint32 timingAdvance RADIO_ALIGNED(4);
} RADIO_ALIGNED(4) RadioSignalStrengthGsm;
G_STATIC_ASSERT(sizeof(RadioSignalStrengthGsm) == 12);

typedef struct radio_signal_strength_wcdma {
    gint32 signalStrength RADIO_ALIGNED(4);
    gint32 bitErrorRate RADIO_ALIGNED(4);
} RADIO_ALIGNED(4) RadioSignalStrengthWcdma;
G_STATIC_ASSERT(sizeof(RadioSignalStrengthWcdma) == 8);

typedef struct radio_signal_strength_wcdma_1_2 {
    RadioSignalStrengthWcdma base RADIO_ALIGNED(4);
    gint32 rscp RADIO_ALIGNED(4);
    gint32 ecno RADIO_ALIGNED(4);
} RADIO_ALIGNED(4) RadioSignalStrengthWcdma_1_2; /* Since 1.2.0 */
G_STATIC_ASSERT(sizeof(RadioSignalStrengthWcdma_1_2) == 16);

typedef struct radio_signal_strength_cdma {
    guint32 dbm RADIO_ALIGNED(4);
    guint32 ecio RADIO_ALIGNED(4);
} RADIO_ALIGNED(4) RadioSignalStrengthCdma;
G_STATIC_ASSERT(sizeof(RadioSignalStrengthCdma) == 8);

typedef struct radio_signal_strength_evdo {
    guint32 dbm RADIO_ALIGNED(4);
    guint32 ecio RADIO_ALIGNED(4);
    guint32 signalNoiseRatio RADIO_ALIGNED(4);
} RADIO_ALIGNED(4) RadioSignalStrengthEvdo;
G_STATIC_ASSERT(sizeof(RadioSignalStrengthEvdo) == 12);

typedef struct radio_signal_strength_lte {
    guint32 signalStrength RADIO_ALIGNED(4);
    guint32 rsrp RADIO_ALIGNED(4);
    guint32 rsrq RADIO_ALIGNED(4);
    gint32 rssnr RADIO_ALIGNED(4);
    guint32 cqi RADIO_ALIGNED(4);
    guint32 timingAdvance RADIO_ALIGNED(4);
} RADIO_ALIGNED(4) RadioSignalStrengthLte;
G_STATIC_ASSERT(sizeof(RadioSignalStrengthLte) == 24);

typedef struct radio_signal_strength_tdscdma {
    guint32 rscp RADIO_ALIGNED(4);
} RADIO_ALIGNED(4) RadioSignalStrengthTdScdma;
G_STATIC_ASSERT(sizeof(RadioSignalStrengthTdScdma) == 4);

typedef struct radio_signal_strength_tdscdma_1_2 {
    guint32 signalStrength RADIO_ALIGNED(4);
    guint32 bitErrorRate RADIO_ALIGNED(4);
    guint32 rscp RADIO_ALIGNED(4);
} RADIO_ALIGNED(4) RadioSignalStrengthTdScdma_1_2; /* Since 1.2.0 */
G_STATIC_ASSERT(sizeof(RadioSignalStrengthTdScdma_1_2) == 12);

typedef struct radio_signal_strength {
    RadioSignalStrengthGsm gw RADIO_ALIGNED(4);
    RadioSignalStrengthCdma cdma RADIO_ALIGNED(4);
    RadioSignalStrengthEvdo evdo RADIO_ALIGNED(4);
    RadioSignalStrengthLte lte RADIO_ALIGNED(4);
    RadioSignalStrengthTdScdma tdScdma RADIO_ALIGNED(4);
} RADIO_ALIGNED(4) RadioSignalStrength;
G_STATIC_ASSERT(sizeof(RadioSignalStrength) == 60);

typedef struct radio_signal_strength_1_2 {
    RadioSignalStrengthGsm gw RADIO_ALIGNED(4);
    RadioSignalStrengthCdma cdma RADIO_ALIGNED(4);
    RadioSignalStrengthEvdo evdo RADIO_ALIGNED(4);
    RadioSignalStrengthLte lte RADIO_ALIGNED(4);
    RadioSignalStrengthTdScdma tdScdma RADIO_ALIGNED(4);
    RadioSignalStrengthWcdma_1_2 wcdma RADIO_ALIGNED(4);
} RADIO_ALIGNED(4) RadioSignalStrength_1_2; /* Since 1.2.0 */
G_STATIC_ASSERT(sizeof(RadioSignalStrength_1_2) == 76);

typedef struct radio_signal_strength_nr {
    gint32 ssRsrp RADIO_ALIGNED(4);
    gint32 ssRsrq RADIO_ALIGNED(4);
    gint32 ssSinr RADIO_ALIGNED(4);
    gint32 csiRsrp RADIO_ALIGNED(4);
    gint32 csiRsrq RADIO_ALIGNED(4);
    gint32 csiSinr RADIO_ALIGNED(4);
} RADIO_ALIGNED(4) RadioSignalStrengthNr; /* Since 1.2.5 */
G_STATIC_ASSERT(sizeof(RadioSignalStrengthNr) == 24);

typedef struct radio_signal_strength_1_4 {
    RadioSignalStrengthGsm gsm RADIO_ALIGNED(4);
    RadioSignalStrengthCdma cdma RADIO_ALIGNED(4);
    RadioSignalStrengthEvdo evdo RADIO_ALIGNED(4);
    RadioSignalStrengthLte lte RADIO_ALIGNED(4);
    RadioSignalStrengthTdScdma_1_2 tdscdma RADIO_ALIGNED(4);
    RadioSignalStrengthWcdma_1_2 wcdma RADIO_ALIGNED(4);
    RadioSignalStrengthNr nr RADIO_ALIGNED(4);
} RADIO_ALIGNED(4) RadioSignalStrength_1_4; /* Since 1.2.5 */
G_STATIC_ASSERT(sizeof(RadioSignalStrength_1_4) == 108);

typedef struct radio_cell_info_gsm {
    RadioCellIdentityGsm cellIdentityGsm RADIO_ALIGNED(8);
    RadioSignalStrengthGsm signalStrengthGsm RADIO_ALIGNED(4);
} RADIO_ALIGNED(8) RadioCellInfoGsm;
G_STATIC_ASSERT(sizeof(RadioCellInfoGsm) == 64);

typedef struct radio_cell_info_wcdma {
    RadioCellIdentityWcdma cellIdentityWcdma RADIO_ALIGNED(8);
    RadioSignalStrengthWcdma signalStrengthWcdma RADIO_ALIGNED(4);
} RADIO_ALIGNED(8) RadioCellInfoWcdma;
G_STATIC_ASSERT(sizeof(RadioCellInfoWcdma) == 56);

typedef struct radio_cell_info_cdma {
    RadioCellIdentityCdma cellIdentityCdma RADIO_ALIGNED(4);
    RadioSignalStrengthCdma signalStrengthCdma RADIO_ALIGNED(4);
    RadioSignalStrengthEvdo signalStrengthEvdo RADIO_ALIGNED(4);
} RADIO_ALIGNED(4) RadioCellInfoCdma;
G_STATIC_ASSERT(sizeof(RadioCellInfoCdma) == 40);

typedef struct radio_cell_info_lte {
    RadioCellIdentityLte cellIdentityLte RADIO_ALIGNED(8);
    RadioSignalStrengthLte signalStrengthLte RADIO_ALIGNED(4);
} RADIO_ALIGNED(8) RadioCellInfoLte;
G_STATIC_ASSERT(sizeof(RadioCellInfoLte) == 72);

typedef struct radio_cell_info_tdscdma {
    RadioCellIdentityTdscdma cellIdentityTdscdma RADIO_ALIGNED(8);
    RadioSignalStrengthTdScdma signalStrengthTdscdma RADIO_ALIGNED(4);
} RADIO_ALIGNED(8) RadioCellInfoTdscdma;
G_STATIC_ASSERT(sizeof(RadioCellInfoTdscdma) == 56);

typedef struct radio_cell_info_gsm_1_2 {
    RadioCellIdentityGsm_1_2 cellIdentityGsm RADIO_ALIGNED(8);
    RadioSignalStrengthGsm signalStrengthGsm RADIO_ALIGNED(4);
} RADIO_ALIGNED(8) RadioCellInfoGsm_1_2; /* Since 1.2.0 */
G_STATIC_ASSERT(sizeof(RadioCellInfoGsm_1_2) == 96);

typedef struct radio_cell_info_wcdma_1_2 {
    RadioCellIdentityWcdma_1_2 cellIdentityWcdma RADIO_ALIGNED(8);
    RadioSignalStrengthWcdma_1_2 signalStrengthWcdma RADIO_ALIGNED(4);
} RADIO_ALIGNED(8) RadioCellInfoWcdma_1_2; /* Since 1.2.0 */
G_STATIC_ASSERT(sizeof(RadioCellInfoWcdma_1_2) == 96);

typedef struct radio_cell_info_cdma_1_2 {
    RadioCellIdentityCdma_1_2 cellIdentityCdma RADIO_ALIGNED(4);
    RadioSignalStrengthCdma signalStrengthCdma RADIO_ALIGNED(4);
    RadioSignalStrengthEvdo signalStrengthEvdo RADIO_ALIGNED(4);
} RADIO_ALIGNED(4) RadioCellInfoCdma_1_2; /* Since 1.2.0 */
G_STATIC_ASSERT(sizeof(RadioCellInfoCdma_1_2) == 80);

typedef struct radio_cell_info_lte_1_2 {
    RadioCellIdentityLte_1_2 cellIdentityLte RADIO_ALIGNED(8);
    RadioSignalStrengthLte signalStrengthLte RADIO_ALIGNED(4);
} RADIO_ALIGNED(8) RadioCellInfoLte_1_2; /* Since 1.2.0 */
G_STATIC_ASSERT(sizeof(RadioCellInfoLte_1_2) == 112);

typedef struct radio_cell_info_lte_1_4 {
    RadioCellInfoLte_1_2 base RADIO_ALIGNED(8);
    guint8 isEndcAvailable RADIO_ALIGNED(1);
} RADIO_ALIGNED(8) RadioCellInfoLte_1_4; /* Since 1.2.5 */
G_STATIC_ASSERT(sizeof(RadioCellInfoLte_1_4) == 120);

typedef struct radio_cell_info_tdscdma_1_2 {
    RadioCellIdentityTdscdma_1_2 cellIdentityTdscdma RADIO_ALIGNED(8);
    RadioSignalStrengthTdScdma_1_2 signalStrengthTdscdma RADIO_ALIGNED(4);
} RADIO_ALIGNED(8) RadioCellInfoTdscdma_1_2; /* Since 1.2.0 */
G_STATIC_ASSERT(sizeof(RadioCellInfoTdscdma_1_2) == 104);

typedef struct radio_cell_info_nr {
    RadioSignalStrengthNr signalStrength RADIO_ALIGNED(4);
    RadioCellIdentityNr cellidentity RADIO_ALIGNED(8);
} RADIO_ALIGNED(8) RadioCellInfoNr; /* Since 1.2.5 */
G_STATIC_ASSERT(sizeof(RadioCellInfoNr) == 112);

typedef struct radio_cell_info_1_4 {
    guint8 cellInfoType RADIO_ALIGNED(1);
    guint8 registered RADIO_ALIGNED(1);
    RADIO_CELL_CONNECTION_STATUS connectionStatus RADIO_ALIGNED(4);
    union {
        RadioCellInfoGsm_1_2 gsm RADIO_ALIGNED(8);
        RadioCellInfoCdma_1_2 cdma RADIO_ALIGNED(8);
        RadioCellInfoWcdma_1_2 wcdma RADIO_ALIGNED(8);
        RadioCellInfoTdscdma_1_2 tdscdma RADIO_ALIGNED(8);
        RadioCellInfoLte_1_4 lte RADIO_ALIGNED(8);
        RadioCellInfoNr nr RADIO_ALIGNED(8);
    } info RADIO_ALIGNED(8);
} RADIO_ALIGNED(8) RadioCellInfo_1_4; /* Since 1.2.5 */
G_STATIC_ASSERT(sizeof(RadioCellInfo_1_4) == 128);

typedef struct radio_gsm_broadcast_sms_config {
    gint32 fromServiceId RADIO_ALIGNED(4);
    gint32 toServiceId RADIO_ALIGNED(4);
    gint32 fromCodeScheme RADIO_ALIGNED(4);
    gint32 toCodeScheme RADIO_ALIGNED(4);
    guint8 selected RADIO_ALIGNED(1);
} RADIO_ALIGNED(4) RadioGsmBroadcastSmsConfig;
G_STATIC_ASSERT(sizeof(RadioGsmBroadcastSmsConfig) == 20);

typedef struct radio_select_uicc_sub {
    gint32 slot RADIO_ALIGNED(4);
    gint32 appIndex RADIO_ALIGNED(4);
    gint32 subType RADIO_ALIGNED(4);
    gint32 actStatus RADIO_ALIGNED(4);
} RADIO_ALIGNED(4) RadioSelectUiccSub;
G_STATIC_ASSERT(sizeof(RadioSelectUiccSub) == 16);

typedef struct radio_supp_svc_notification {
    guint8 isMT RADIO_ALIGNED(1);
    gint32 code RADIO_ALIGNED(4);
    gint32 index RADIO_ALIGNED(4);
    gint32 type RADIO_ALIGNED(4);
    GBinderHidlString number RADIO_ALIGNED(8);
} RADIO_ALIGNED(8) RadioSuppSvcNotification;
G_STATIC_ASSERT(sizeof(RadioSuppSvcNotification) == 32);

typedef struct radio_sim_refresh {
    gint32 type RADIO_ALIGNED(4);
    gint32 efId RADIO_ALIGNED(4);
    GBinderHidlString aid RADIO_ALIGNED(8);
} RADIO_ALIGNED(8) RadioSimRefresh;
G_STATIC_ASSERT(sizeof(RadioSimRefresh) == 24);

typedef struct radio_capability {
    gint32 session RADIO_ALIGNED(4);
    RADIO_CAPABILITY_PHASE phase RADIO_ALIGNED(4);
    gint32 raf RADIO_ALIGNED(4);
    GBinderHidlString logicalModemUuid RADIO_ALIGNED(8);
    RADIO_CAPABILITY_STATUS status RADIO_ALIGNED(4);
} RADIO_ALIGNED(8) RadioCapability;
G_STATIC_ASSERT(sizeof(RadioCapability) == 40);

typedef struct radio_lce_status_info {
    guint32 lceStatus RADIO_ALIGNED(4);
    guint8 actualIntervalMs RADIO_ALIGNED(1);
} RADIO_ALIGNED(4) RadioLceStatusInfo;
G_STATIC_ASSERT(sizeof(RadioLceStatusInfo) == 8);

typedef struct radio_activity_stats_info {
    guint32 sleepModeTimeMs RADIO_ALIGNED(4);
    guint32 idleModeTimeMs RADIO_ALIGNED(4);
    guint32 txmModetimeMs[5 /* NUM_TX_POWER_LEVELS */] RADIO_ALIGNED(4);
    guint32 rxModeTimeMs RADIO_ALIGNED(4);
} RADIO_ALIGNED(4) RadioActivityStatsInfo;
G_STATIC_ASSERT(sizeof(RadioActivityStatsInfo) == 32);

typedef struct radio_hardware_config {
    gint32 type RADIO_ALIGNED(4);
    GBinderHidlString uuid RADIO_ALIGNED(8);
    gint32 state RADIO_ALIGNED(4);
    GBinderHidlVec modem RADIO_ALIGNED(8); /* vec<RadioHardwareConfigModem> */
    GBinderHidlVec sim RADIO_ALIGNED(8);   /* vec<RadioHardwareConfigSim> */
} RADIO_ALIGNED(8) RadioHardwareConfig;
G_STATIC_ASSERT(sizeof(RadioHardwareConfig) == 64);

typedef struct radio_hardware_config_modem {
    gint32 rilModel RADIO_ALIGNED(4);
    guint32 rat RADIO_ALIGNED(4);
    gint32 maxVoice RADIO_ALIGNED(4);
    gint32 maxData RADIO_ALIGNED(4);
    gint32 maxStandby RADIO_ALIGNED(4);
} RADIO_ALIGNED(4) RadioHardwareConfigModem;
G_STATIC_ASSERT(sizeof(RadioHardwareConfigModem) == 20);

typedef struct radio_hardware_config_sim {
    GBinderHidlString modemUuid RADIO_ALIGNED(8);
} RADIO_ALIGNED(8) RadioHardwareConfigSim;
G_STATIC_ASSERT(sizeof(RadioHardwareConfigSim) == 16);

typedef struct radio_network_scan_result {
    RADIO_SCAN_STATUS status RADIO_ALIGNED(4);
    guint32 error RADIO_ALIGNED(4);
    GBinderHidlVec networkInfos RADIO_ALIGNED(8); /* vec<RadioCellInfo> */
                                               /* or vec<RadioCellInfo_1_4> */
} RADIO_ALIGNED(8) RadioNetworkScanResult; /* Since 1.2.5 */
G_STATIC_ASSERT(sizeof(RadioNetworkScanResult) == 24);

/* c(req,resp,callName,CALL_NAME) */
#define RADIO_CALL_1_0(c) \
    c(2,1,getIccCardStatus,GET_ICC_CARD_STATUS) \
    c(3,2,supplyIccPinForApp,SUPPLY_ICC_PIN_FOR_APP) \
    c(4,3,supplyIccPukForApp,SUPPLY_ICC_PUK_FOR_APP) \
    c(5,4,supplyIccPin2ForApp,SUPPLY_ICC_PIN2_FOR_APP) \
    c(6,5,supplyIccPuk2ForApp,SUPPLY_ICC_PUK2_FOR_APP) \
    c(7,6,changeIccPinForApp,CHANGE_ICC_PIN_FOR_APP) \
    c(8,7,changeIccPin2ForApp,CHANGE_ICC_PIN2_FOR_APP) \
    c(9,8,supplyNetworkDepersonalization,SUPPLY_NETWORK_DEPERSONALIZATION) \
    c(10,9,getCurrentCalls,GET_CURRENT_CALLS) \
    c(11,10,dial,DIAL) \
    c(12,11,getImsiForApp,GET_IMSI_FOR_APP) \
    c(13,12,hangup,HANGUP) \
    c(14,13,hangupWaitingOrBackground,HANGUP_WAITING_OR_BACKGROUND) \
    c(15,14,hangupForegroundResumeBackground,HANGUP_FOREGROUND_RESUME_BACKGROUND) \
    c(16,15,switchWaitingOrHoldingAndActive,SWITCH_WAITING_OR_HOLDING_AND_ACTIVE) \
    c(17,16,conference,CONFERENCE) \
    c(18,17,rejectCall,REJECT_CALL) \
    c(19,18,getLastCallFailCause,GET_LAST_CALL_FAIL_CAUSE) \
    c(20,19,getSignalStrength,GET_SIGNAL_STRENGTH) \
    c(21,20,getVoiceRegistrationState,GET_VOICE_REGISTRATION_STATE) \
    c(22,21,getDataRegistrationState,GET_DATA_REGISTRATION_STATE) \
    c(23,22,getOperator,GET_OPERATOR) \
    c(24,23,setRadioPower,SET_RADIO_POWER) \
    c(25,24,sendDtmf,SEND_DTMF) \
    c(26,25,sendSms,SEND_SMS) \
    c(27,26,sendSMSExpectMore,SEND_SMS_EXPECT_MORE) \
    c(28,27,setupDataCall,SETUP_DATA_CALL) \
    c(29,28,iccIOForApp,ICC_IO_FOR_APP) \
    c(30,29,sendUssd,SEND_USSD) \
    c(31,30,cancelPendingUssd,CANCEL_PENDING_USSD) \
    c(32,31,getClir,GET_CLIR) \
    c(33,32,setClir,SET_CLIR) \
    c(34,33,getCallForwardStatus,GET_CALL_FORWARD_STATUS) \
    c(35,34,setCallForward,SET_CALL_FORWARD) \
    c(36,35,getCallWaiting,GET_CALL_WAITING) \
    c(37,36,setCallWaiting,SET_CALL_WAITING) \
    c(38,37,acknowledgeLastIncomingGsmSms,ACKNOWLEDGE_LAST_INCOMING_GSM_SMS) \
    c(39,38,acceptCall,ACCEPT_CALL) \
    c(40,39,deactivateDataCall,DEACTIVATE_DATA_CALL) \
    c(41,40,getFacilityLockForApp,GET_FACILITY_LOCK_FOR_APP) \
    c(42,41,setFacilityLockForApp,SET_FACILITY_LOCK_FOR_APP) \
    c(43,42,setBarringPassword,SET_BARRING_PASSWORD) \
    c(44,43,getNetworkSelectionMode,GET_NETWORK_SELECTION_MODE) \
    c(45,44,setNetworkSelectionModeAutomatic,SET_NETWORK_SELECTION_MODE_AUTOMATIC) \
    c(46,45,setNetworkSelectionModeManual,SET_NETWORK_SELECTION_MODE_MANUAL) \
    c(47,46,getAvailableNetworks,GET_AVAILABLE_NETWORKS) \
    c(48,47,startDtmf,START_DTMF) \
    c(49,48,stopDtmf,STOP_DTMF) \
    c(50,49,getBasebandVersion,GET_BASEBAND_VERSION) \
    c(51,50,separateConnection,SEPARATE_CONNECTION) \
    c(52,51,setMute,SET_MUTE) \
    c(53,52,getMute,GET_MUTE) \
    c(54,53,getClip,GET_CLIP) \
    c(55,54,getDataCallList,GET_DATA_CALL_LIST) \
    c(56,55,setSuppServiceNotifications,SET_SUPP_SERVICE_NOTIFICATIONS) \
    c(57,56,writeSmsToSim,WRITE_SMS_TO_SIM) \
    c(58,57,deleteSmsOnSim,DELETE_SMS_ON_SIM) \
    c(59,58,setBandMode,SET_BAND_MODE) \
    c(60,59,getAvailableBandModes,GET_AVAILABLE_BAND_MODES) \
    c(61,60,sendEnvelope,SEND_ENVELOPE) \
    c(62,61,sendTerminalResponseToSim,SEND_TERMINAL_RESPONSE_TO_SIM) \
    c(63,62,handleStkCallSetupRequestFromSim,HANDLE_STK_CALL_SETUP_REQUEST_FROM_SIM) \
    c(64,63,explicitCallTransfer,EXPLICIT_CALL_TRANSFER) \
    c(65,64,setPreferredNetworkType,SET_PREFERRED_NETWORK_TYPE) \
    c(66,65,getPreferredNetworkType,GET_PREFERRED_NETWORK_TYPE) \
    c(67,66,getNeighboringCids,GET_NEIGHBORING_CIDS) \
    c(68,67,setLocationUpdates,SET_LOCATION_UPDATES) \
    c(69,68,setCdmaSubscriptionSource,SET_CDMA_SUBSCRIPTION_SOURCE) \
    c(70,69,setCdmaRoamingPreference,SET_CDMA_ROAMING_PREFERENCE) \
    c(71,70,getCdmaRoamingPreference,GET_CDMA_ROAMING_PREFERENCE) \
    c(72,71,setTTYMode,SET_TTY_MODE) \
    c(73,72,getTTYMode,GET_TTY_MODE) \
    c(74,73,setPreferredVoicePrivacy,SET_PREFERRED_VOICE_PRIVACY) \
    c(75,74,getPreferredVoicePrivacy,GET_PREFERRED_VOICE_PRIVACY) \
    c(76,75,sendCDMAFeatureCode,SEND_CDMA_FEATURE_CODE) \
    c(77,76,sendBurstDtmf,SEND_BURST_DTMF) \
    c(78,77,sendCdmaSms,SEND_CDMA_SMS) \
    c(79,78,acknowledgeLastIncomingCdmaSms,ACKNOWLEDGE_LAST_INCOMING_CDMA_SMS) \
    c(80,79,getGsmBroadcastConfig,GET_GSM_BROADCAST_CONFIG) \
    c(81,80,setGsmBroadcastConfig,SET_GSM_BROADCAST_CONFIG) \
    c(82,81,setGsmBroadcastActivation,SET_GSM_BROADCAST_ACTIVATION) \
    c(83,82,getCdmaBroadcastConfig,GET_CDMA_BROADCAST_CONFIG) \
    c(84,83,setCdmaBroadcastConfig,SET_CDMA_BROADCAST_CONFIG) \
    c(85,84,setCdmaBroadcastActivation,SET_CDMA_BROADCAST_ACTIVATION) \
    c(86,85,getCDMASubscription,GET_CDMA_SUBSCRIPTION) \
    c(87,86,writeSmsToRuim,WRITE_SMS_TO_RUIM) \
    c(88,87,deleteSmsOnRuim,DELETE_SMS_ON_RUIM) \
    c(89,88,getDeviceIdentity,GET_DEVICE_IDENTITY) \
    c(90,89,exitEmergencyCallbackMode,EXIT_EMERGENCY_CALLBACK_MODE) \
    c(91,90,getSmscAddress,GET_SMSC_ADDRESS) \
    c(92,91,setSmscAddress,SET_SMSC_ADDRESS) \
    c(93,92,reportSmsMemoryStatus,REPORT_SMS_MEMORY_STATUS) \
    c(94,93,reportStkServiceIsRunning,REPORT_STK_SERVICE_IS_RUNNING) \
    c(95,94,getCdmaSubscriptionSource,GET_CDMA_SUBSCRIPTION_SOURCE) \
    c(96,95,requestIsimAuthentication,REQUEST_ISIM_AUTHENTICATION) \
    c(97,96,acknowledgeIncomingGsmSmsWithPdu,ACKNOWLEDGE_INCOMING_GSM_SMS_WITH_PDU) \
    c(98,97,sendEnvelopeWithStatus,SEND_ENVELOPE_WITH_STATUS) \
    c(99,98,getVoiceRadioTechnology,GET_VOICE_RADIO_TECHNOLOGY) \
    c(100,99,getCellInfoList,GET_CELL_INFO_LIST) \
    c(101,100,setCellInfoListRate,SET_CELL_INFO_LIST_RATE) \
    c(102,101,setInitialAttachApn,SET_INITIAL_ATTACH_APN) \
    c(103,102,getImsRegistrationState,GET_IMS_REGISTRATION_STATE) \
    c(104,103,sendImsSms,SEND_IMS_SMS) \
    c(105,104,iccTransmitApduBasicChannel,ICC_TRANSMIT_APDU_BASIC_CHANNEL) \
    c(106,105,iccOpenLogicalChannel,ICC_OPEN_LOGICAL_CHANNEL) \
    c(107,106,iccCloseLogicalChannel,ICC_CLOSE_LOGICAL_CHANNEL) \
    c(108,107,iccTransmitApduLogicalChannel,ICC_TRANSMIT_APDU_LOGICAL_CHANNEL) \
    c(109,108,nvReadItem,NV_READ_ITEM) \
    c(110,109,nvWriteItem,NV_WRITE_ITEM) \
    c(111,110,nvWriteCdmaPrl,NV_WRITE_CDMA_PRL) \
    c(112,111,nvResetConfig,NV_RESET_CONFIG) \
    c(113,112,setUiccSubscription,SET_UICC_SUBSCRIPTION) \
    c(114,113,setDataAllowed,SET_DATA_ALLOWED) \
    c(115,114,getHardwareConfig,GET_HARDWARE_CONFIG) \
    c(116,115,requestIccSimAuthentication,REQUEST_ICC_SIM_AUTHENTICATION) \
    c(117,116,setDataProfile,SET_DATA_PROFILE) \
    c(118,117,requestShutdown,REQUEST_SHUTDOWN) \
    c(119,118,getRadioCapability,GET_RADIO_CAPABILITY) \
    c(120,119,setRadioCapability,SET_RADIO_CAPABILITY) \
    c(121,120,startLceService,START_LCE_SERVICE) \
    c(122,121,stopLceService,STOP_LCE_SERVICE) \
    c(123,122,pullLceData,PULL_LCE_DATA) \
    c(124,123,getModemActivityInfo,GET_MODEM_ACTIVITY_INFO) \
    c(125,124,setAllowedCarriers,SET_ALLOWED_CARRIERS) \
    c(126,125,getAllowedCarriers,GET_ALLOWED_CARRIERS) \
    c(127,126,sendDeviceState,SEND_DEVICE_STATE) \
    c(128,127,setIndicationFilter,SET_INDICATION_FILTER) \
    c(129,128,setSimCardPower,SET_SIM_CARD_POWER)

#define RADIO_CALL_1_1(c) \
    c(131,130,setCarrierInfoForImsiEncryption,SET_CARRIER_INFO_FOR_IMSI_ENCRYPTION) \
    c(132,131,setSimCardPower_1_1,SET_SIM_CARD_POWER_1_1) \
    c(133,132,startNetworkScan,START_NETWORK_SCAN) \
    c(134,133,stopNetworkScan,STOP_NETWORK_SCAN) \
    c(135,134,startKeepalive,START_KEEPALIVE) \
    c(136,135,stopKeepalive,STOP_KEEPALIVE)

#define RADIO_CALL_1_2(c) /* Since 1.2.0 */ \
    c(139,138,setSignalStrengthReportingCriteria,SET_SIGNAL_STRENGTH_REPORTING_CRITERIA) \
    c(140,139,setLinkCapacityReportingCriteria,SET_LINK_CAPACITY_REPORTING_CRITERIA)

#define RADIO_CALL_1_3(c) /* Since 1.2.5 */ \
    c(143,144,setSystemSelectionChannels,SET_SYSTEM_SELECTION_CHANNELS) \
    c(144,145,enableModem,ENABLE_MODEM) \
    c(145,146,getModemStackStatus,GET_MODEM_STACK_STATUS)

#define RADIO_CALL_1_4(c) /* Since 1.2.5 */ \
    c(149,147,emergencyDial,EMERGENCY_DIAL) \
    c(150,148,startNetworkScan_1_4,START_NETWORKSCAN_1_4) \
    c(151,152,getPreferredNetworkTypeBitmap,GET_PREFERRED_NETWORK_TYPE_BITMAP) \
    c(152,153,setPreferredNetworkTypeBitmap,SET_PREFERRED_NETWORK_TYPE_BITMAP) \
    c(153,156,setAllowedCarriers_1_4,SET_ALLOWED_CARRIERS_1_4) \
    c(154,157,getAllowedCarriers_1_4,GET_ALLOWED_CARRIERS_1_4) \
    c(155,158,getSignalStrength_1_4,GET_SIGNAL_STRENGTH_1_4)

/* e(code,eventName,EVENT_NAME) */
#define RADIO_EVENT_1_0(e) \
    e(1,radioStateChanged,RADIO_STATE_CHANGED) \
    e(2,callStateChanged,CALL_STATE_CHANGED) \
    e(3,networkStateChanged,NETWORK_STATE_CHANGED) \
    e(4,newSms,NEW_SMS) \
    e(5,newSmsStatusReport,NEW_SMS_STATUS_REPORT) \
    e(6,newSmsOnSim,NEW_SMS_ON_SIM) \
    e(7,onUssd,ON_USSD) \
    e(8,nitzTimeReceived,NITZ_TIME_RECEIVED) \
    e(9,currentSignalStrength,CURRENT_SIGNAL_STRENGTH) \
    e(10,dataCallListChanged,DATA_CALL_LIST_CHANGED) \
    e(11,suppSvcNotify,SUPP_SVC_NOTIFY) \
    e(12,stkSessionEnd,STK_SESSION_END) \
    e(13,stkProactiveCommand,STK_PROACTIVE_COMMAND) \
    e(14,stkEventNotify,STK_EVENT_NOTIFY) \
    e(15,stkCallSetup,STK_CALL_SETUP) \
    e(16,simSmsStorageFull,SIM_SMS_STORAGE_FULL) \
    e(17,simRefresh,SIM_REFRESH) \
    e(18,callRing,CALL_RING) \
    e(19,simStatusChanged,SIM_STATUS_CHANGED) \
    e(20,cdmaNewSms,CDMA_NEW_SMS) \
    e(21,newBroadcastSms,NEW_BROADCAST_SMS) \
    e(22,cdmaRuimSmsStorageFull,CDMA_RUIM_SMS_STORAGE_FULL) \
    e(23,restrictedStateChanged,RESTRICTED_STATE_CHANGED) \
    e(24,enterEmergencyCallbackMode,ENTER_EMERGENCY_CALLBACK_MODE) \
    e(25,cdmaCallWaiting,CDMA_CALL_WAITING) \
    e(26,cdmaOtaProvisionStatus,CDMA_OTA_PROVISION_STATUS) \
    e(27,cdmaInfoRec,CDMA_INFO_REC) \
    e(28,indicateRingbackTone,INDICATE_RINGBACK_TONE) \
    e(29,resendIncallMute,RESEND_INCALL_MUTE) \
    e(30,cdmaSubscriptionSourceChanged,CDMA_SUBSCRIPTION_SOURCE_CHANGED) \
    e(31,cdmaPrlChanged,CDMA_PRL_CHANGED) \
    e(32,exitEmergencyCallbackMode,EXIT_EMERGENCY_CALLBACK_MODE) \
    e(33,rilConnected,RIL_CONNECTED) \
    e(34,voiceRadioTechChanged,VOICE_RADIO_TECH_CHANGED) \
    e(35,cellInfoList,CELL_INFO_LIST) \
    e(36,imsNetworkStateChanged,IMS_NETWORK_STATE_CHANGED) \
    e(37,subscriptionStatusChanged,SUBSCRIPTION_STATUS_CHANGED) \
    e(38,srvccStateNotify,SRVCC_STATE_NOTIFY) \
    e(39,hardwareConfigChanged,HARDWARE_CONFIG_CHANGED) \
    e(40,radioCapabilityIndication,RADIO_CAPABILITY_INDICATION) \
    e(41,onSupplementaryServiceIndication,ON_SUPPLEMENTARY_SERVICE_INDICATION) \
    e(42,stkCallControlAlphaNotify,STK_CALL_CONTROL_ALPHA_NOTIFY) \
    e(43,lceData,LCE_DATA) \
    e(44,pcoData,PCO_DATA) \
    e(45,modemReset,MODEM_RESET)

#define RADIO_EVENT_1_1(e) \
    e(46,carrierInfoForImsiEncryption,CARRIER_INFO_FOR_IMSI_ENCRYPTION) \
    e(47,networkScanResult,NETWORK_SCAN_RESULT) \
    e(48,keepaliveStatus,KEEPALIVE_STATUS)

#define RADIO_EVENT_1_2(e)  /* Since 1.2.0 */ \
    e(49,networkScanResult_1_2,NETWORK_SCAN_RESULT_1_2) \
    e(50,cellInfoList_1_2,CELL_INFO_LIST_1_2) \
    e(51,currentLinkCapacityEstimate,CURRENT_LINK_CAPACITY_ESTIMATE) \
    e(52,currentPhysicalChannelConfigs,CURRENT_PHYSICAL_CHANNEL_CONFIGS) \
    e(53,currentSignalStrength_1_2,CURRENT_SIGNAL_STRENGTH_1_2)

#define RADIO_EVENT_1_4(e)  /* Since 1.2.5 */ \
    e(54,currentEmergencyNumberList,CURRENT_EMERGENCY_NUMBER_LIST) \
    e(55,cellInfoList_1_4,CELL_INFO_LIST_1_4) \
    e(56,networkScanResult_1_4,NETWORK_SCAN_RESULT_1_4) \
    e(57,currentPhysicalChannelConfigs_1_4,CURRENT_PHYSICAL_CHANNEL_CONFIGS_1_4) \
    e(58,dataCallListChanged_1_4,DATA_CALL_LIST_CHANGED_1_4) \
    e(59,currentSignalStrength_1_4,CURRENT_SIGNAL_STRENGTH_1_4)

typedef enum radio_req {
    RADIO_REQ_ANY = 0,
    RADIO_REQ_NONE = 0,
#define RADIO_REQ_(req,resp,Name,NAME) RADIO_REQ_##NAME = req,

    /* android.hardware.radio@1.0::IRadio */
    RADIO_REQ_SET_RESPONSE_FUNCTIONS = 1, /* setResponseFunctions */
    RADIO_CALL_1_0(RADIO_REQ_)
    RADIO_REQ_RESPONSE_ACKNOWLEDGEMENT = 130,  /* responseAcknowledgement */
    RADIO_1_0_REQ_LAST = RADIO_REQ_RESPONSE_ACKNOWLEDGEMENT,

    /* android.hardware.radio@1.1::IRadio */
    RADIO_CALL_1_1(RADIO_REQ_)
    RADIO_1_1_REQ_LAST = RADIO_REQ_STOP_KEEPALIVE,

    /* android.hardware.radio@1.2::IRadio */
    RADIO_CALL_1_2(RADIO_REQ_)
    RADIO_REQ_START_NETWORK_SCAN_1_2 = 137,
    RADIO_REQ_SET_INDICATION_FILTER_1_2 = 138,
    RADIO_REQ_SETUP_DATA_CALL_1_2 = 141,
    RADIO_REQ_DEACTIVATE_DATA_CALL_1_2 = 142,
    RADIO_1_2_REQ_LAST = RADIO_REQ_DEACTIVATE_DATA_CALL_1_2,

    /* android.hardware.radio@1.3::IRadio */
    RADIO_CALL_1_3(RADIO_REQ_) /* Since 1.2.5 */
    RADIO_1_3_REQ_LAST = RADIO_REQ_GET_MODEM_STACK_STATUS,

    /* android.hardware.radio@1.4::IRadio */
    RADIO_CALL_1_4(RADIO_REQ_) /* Since 1.2.5 */
    RADIO_REQ_SETUP_DATA_CALL_1_4 = 146,
    RADIO_REQ_SET_INITIAL_ATTACH_APN_1_4 = 147,
    RADIO_REQ_SET_DATA_PROFILE_1_4 = 148,
    RADIO_1_4_REQ_LAST = RADIO_REQ_GET_SIGNAL_STRENGTH_1_4
#undef RADIO_REQ_
} RADIO_REQ;

typedef enum radio_resp {
    RADIO_RESP_ANY = 0,
    RADIO_RESP_NONE = 0,
#define RADIO_RESP_(req,resp,Name,NAME) RADIO_RESP_##NAME = resp,

    /* android.hardware.radio@1.0::IRadioResponse */
    RADIO_CALL_1_0(RADIO_RESP_)
    RADIO_RESP_ACKNOWLEDGE_REQUEST = 129, /* acknowledgeRequest */
    RADIO_1_0_RESP_LAST = RADIO_RESP_ACKNOWLEDGE_REQUEST, /* Since 1.2.4 */

    /* android.hardware.radio@1.1::IRadioResponse */
    RADIO_CALL_1_1(RADIO_RESP_)
    RADIO_1_1_RESP_LAST = RADIO_RESP_STOP_KEEPALIVE,      /* Since 1.2.4 */

    /* android.hardware.radio@1.2::IRadioResponse */
    RADIO_CALL_1_2(RADIO_RESP_)
    RADIO_RESP_GET_CELL_INFO_LIST_1_2 = 136,
    RADIO_RESP_GET_ICC_CARD_STATUS_1_2 = 137,
    RADIO_RESP_GET_CURRENT_CALLS_1_2 = 140,
    RADIO_RESP_GET_SIGNAL_STRENGTH_1_2 = 141,
    RADIO_RESP_GET_VOICE_REGISTRATION_STATE_1_2 = 142,
    RADIO_RESP_GET_DATA_REGISTRATION_STATE_1_2 = 143,
    RADIO_1_2_RESP_LAST = RADIO_RESP_GET_DATA_REGISTRATION_STATE_1_2, /* Since 1.2.4 */

    /* android.hardware.radio@1.3::IRadioResponse */
    RADIO_CALL_1_3(RADIO_RESP_) /* Since 1.2.5 */
    RADIO_1_3_RESP_LAST = RADIO_RESP_GET_MODEM_STACK_STATUS,

    /* android.hardware.radio@1.4::IRadioResponse */
    RADIO_CALL_1_4(RADIO_RESP_) /* Since 1.2.5 */
    RADIO_RESP_GET_CELL_INFO_LIST_RESPONSE_1_4 = 149,
    RADIO_RESP_GET_DATA_REGISTRATION_STATE_RESPONSE_1_4 = 150,
    RADIO_RESP_GET_ICC_CARD_STATUS_RESPONSE_1_4 = 151,
    RADIO_RESP_GET_DATA_CALL_LIST_RESPONSE_1_4 = 154,
    RADIO_RESP_SETUP_DATA_CALL_RESPONSE_1_4 = 155,
    RADIO_1_4_RESP_LAST = RADIO_RESP_GET_SIGNAL_STRENGTH_1_4
#undef RADIO_RESP_
} RADIO_RESP;

typedef enum radio_ind {
    RADIO_IND_ANY = 0,
    RADIO_IND_NONE = 0,
#define RADIO_IND_(code,Name,NAME) RADIO_IND_##NAME = code,

    /* android.hardware.radio@1.0::IRadioIndication */
    RADIO_EVENT_1_0(RADIO_IND_)
    RADIO_1_0_IND_LAST = RADIO_IND_MODEM_RESET,  /* Since 1.2.4 */

    /* android.hardware.radio@1.1::IRadioIndication */
    RADIO_EVENT_1_1(RADIO_IND_)
    RADIO_1_1_IND_LAST = RADIO_IND_KEEPALIVE_STATUS,  /* Since 1.2.4 */

    /* android.hardware.radio@1.2::IRadioIndication */
    RADIO_EVENT_1_2(RADIO_IND_)
    RADIO_1_2_IND_LAST = RADIO_IND_CURRENT_SIGNAL_STRENGTH_1_2, /* Since 1.2.4 */

    /* android.hardware.radio@1.3::IRadioIndication */
    RADIO_1_3_IND_LAST = RADIO_1_2_IND_LAST, /* Since 1.2.5 */

    /* android.hardware.radio@1.4::IRadioIndication */
    RADIO_EVENT_1_4(RADIO_IND_)
    RADIO_1_4_IND_LAST = RADIO_IND_CURRENT_SIGNAL_STRENGTH_1_4 /* Since 1.2.5 */
#undef RADIO_IND_
} RADIO_IND;

/* Logging */

#define GBINDER_RADIO_LOG_MODULE gbinder_radio_log

extern GLogModule GBINDER_RADIO_LOG_MODULE;

G_END_DECLS

#endif /* RADIO_TYPES_H */

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */

