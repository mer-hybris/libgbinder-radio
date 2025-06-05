/*
 * Copyright (C) 2022 Jolla Ltd.
 * Copyright (C) 2022 Slava Monich <slava.monich@jolla.com>
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

#ifndef RADIO_CONFIG_TYPES_H
#define RADIO_CONFIG_TYPES_H

/* This API exists since 1.4.6 */

#include <radio_types.h>

G_BEGIN_DECLS

typedef enum radio_config_interface {
    RADIO_CONFIG_INTERFACE_NONE = -1,
    RADIO_CONFIG_INTERFACE_1_0,
    RADIO_CONFIG_INTERFACE_1_1,
    RADIO_CONFIG_INTERFACE_1_2, /* Since 1.5.0 */
    RADIO_CONFIG_INTERFACE_1_3, /* Since 1.6.1 */
    RADIO_CONFIG_INTERFACE_COUNT
} RADIO_CONFIG_INTERFACE;

#define RADIO_CONFIG_INTERFACE_MAX (RADIO_CONFIG_INTERFACE_COUNT - 1)

#define RADIO_CONFIG_INSTANCE         "default"
#define RADIO_CONFIG_IFACE_PREFIX     "android.hardware.radio.config@"
#define RADIO_CONFIG_IFACE            "IRadioConfig"
#define RADIO_CONFIG_RESPONSE_IFACE   "IRadioConfigResponse"
#define RADIO_CONFIG_INDICATION_IFACE "IRadioConfigIndication"
#define RADIO_CONFIG_IFACE_1_0(x)     RADIO_CONFIG_IFACE_PREFIX "1.0::" x
#define RADIO_CONFIG_IFACE_1_1(x)     RADIO_CONFIG_IFACE_PREFIX "1.1::" x
#define RADIO_CONFIG_IFACE_1_2(x)     RADIO_CONFIG_IFACE_PREFIX "1.2::" x
#define RADIO_CONFIG_IFACE_1_3(x)     RADIO_CONFIG_IFACE_PREFIX "1.3::" x
#define RADIO_CONFIG_1_0              RADIO_CONFIG_IFACE_1_0(RADIO_CONFIG_IFACE)
#define RADIO_CONFIG_1_1              RADIO_CONFIG_IFACE_1_1(RADIO_CONFIG_IFACE)
#define RADIO_CONFIG_1_2              RADIO_CONFIG_IFACE_1_2(RADIO_CONFIG_IFACE)
#define RADIO_CONFIG_1_3              RADIO_CONFIG_IFACE_1_3(RADIO_CONFIG_IFACE)
#define RADIO_CONFIG_1_0_FQNAME       RADIO_CONFIG_1_0 "/" RADIO_CONFIG_INSTANCE
#define RADIO_CONFIG_1_1_FQNAME       RADIO_CONFIG_1_1 "/" RADIO_CONFIG_INSTANCE
#define RADIO_CONFIG_1_2_FQNAME       RADIO_CONFIG_1_2 "/" RADIO_CONFIG_INSTANCE
#define RADIO_CONFIG_1_3_FQNAME       RADIO_CONFIG_1_3 "/" RADIO_CONFIG_INSTANCE
#define RADIO_CONFIG_RESPONSE_1_0 \
        RADIO_CONFIG_IFACE_1_0(RADIO_CONFIG_RESPONSE_IFACE)
#define RADIO_CONFIG_RESPONSE_1_1 \
        RADIO_CONFIG_IFACE_1_1(RADIO_CONFIG_RESPONSE_IFACE)
#define RADIO_CONFIG_RESPONSE_1_2 \
        RADIO_CONFIG_IFACE_1_2(RADIO_CONFIG_RESPONSE_IFACE)
#define RADIO_CONFIG_RESPONSE_1_3 \
        RADIO_CONFIG_IFACE_1_3(RADIO_CONFIG_RESPONSE_IFACE)
#define RADIO_CONFIG_INDICATION_1_0 \
        RADIO_CONFIG_IFACE_1_0(RADIO_CONFIG_INDICATION_IFACE)
#define RADIO_CONFIG_INDICATION_1_1 \
        RADIO_CONFIG_IFACE_1_1(RADIO_CONFIG_INDICATION_IFACE)
#define RADIO_CONFIG_INDICATION_1_2 \
        RADIO_CONFIG_IFACE_1_2(RADIO_CONFIG_INDICATION_IFACE)

/* Types defined in types.hal */

/* SlotState */
typedef enum radio_slot_state {
    RADIO_SLOT_STATE_INACTIVE,
    RADIO_SLOT_STATE_ACTIVE
} RADIO_SLOT_STATE;
G_STATIC_ASSERT(sizeof(RADIO_SLOT_STATE) == 4);

/* SimSlotStatus */
typedef struct radio_sim_slot_status {
    RADIO_CARD_STATE cardState RADIO_ALIGNED(4);
    RADIO_SLOT_STATE slotState RADIO_ALIGNED(4);
    GBinderHidlString atr RADIO_ALIGNED(8);
    guint32 logicalSlotId RADIO_ALIGNED(4);
    GBinderHidlString iccid RADIO_ALIGNED(8);
} RADIO_ALIGNED(8) RadioSimSlotStatus;
G_STATIC_ASSERT(sizeof(RadioSimSlotStatus) == 48);

/* ModemInfo */
typedef struct radio_modem_info {
    guint8 modemId RADIO_ALIGNED(1);
} RADIO_ALIGNED(1) RadioModemInfo;
G_STATIC_ASSERT(sizeof(RadioModemInfo) == 1);

/* PhoneCapability */
typedef struct radio_phone_capability {
    guint8 maxActiveData RADIO_ALIGNED(1);
    guint8 maxActiveInternetData RADIO_ALIGNED(1);
    guint8 isInternetLingeringSupported RADIO_ALIGNED(1);
    GBinderHidlVec logicalModemList RADIO_ALIGNED(8); /* vec<ModemInfo> */
} RADIO_ALIGNED(8) RadioPhoneCapability;
G_STATIC_ASSERT(sizeof(RadioPhoneCapability) == 24);

/* ModemsConfig */
typedef struct radio_modems_config {
    guint8 numOfLiveModems RADIO_ALIGNED(1);
} RADIO_ALIGNED(1) RadioModemsConfig;
G_STATIC_ASSERT(sizeof(RadioModemsConfig) == 1);

/* Transaction codes */

/* c(req,resp,Name,CALL_NAME) */
#define RADIO_CONFIG_CALL_1_0(c) \
    c(2,1,getSimSlotsStatus,GET_SIM_SLOTS_STATUS) \
    c(3,2,setSimSlotsMapping,SET_SIM_SLOTS_MAPPING)

#define RADIO_CONFIG_CALL_1_1(c) \
    c(4,3,getPhoneCapability,GET_PHONE_CAPABILITY) \
    c(5,4,setPreferredDataModem,SET_PREFERRED_DATA_MODEM) \
    c(6,5,setModemsConfig,SET_MODEMS_CONFIG) \
    c(7,6,getModemsConfig,GET_MODEMS_CONFIG)

#define RADIO_CONFIG_CALL_1_3(c) \
    c(8,8,getHalDeviceCapabilities,GET_HAL_DEVICE_CAPABILITIES) /* Since 1.6.1 */

/* i(code,Name,IND_NAME) */
#define RADIO_CONFIG_IND_1_0(i) \
    i(1,simSlotsStatusChanged,SIM_SLOTS_STATUS_CHANGED)

#define RADIO_CONFIG_IND_1_2(i) \
    i(2,simSlotsStatusChanged_1_2,SIM_SLOTS_STATUS_CHANGED_1_2) /* Since 1.5.0 */

typedef enum radio_config_req {
    RADIO_CONFIG_REQ_ANY = 0,
    RADIO_CONFIG_REQ_NONE = 0,
#define RADIO_CONFIG_REQ_(req,resp,Name,NAME) RADIO_CONFIG_REQ_##NAME = req,

    /* android.hardware.radio.config@1.0::IRadioConfig */
    RADIO_CONFIG_REQ_SET_RESPONSE_FUNCTIONS = 1, /* setResponseFunctions */
    RADIO_CONFIG_CALL_1_0(RADIO_CONFIG_REQ_)
    RADIO_CONFIG_1_0_REQ_LAST = RADIO_CONFIG_REQ_SET_SIM_SLOTS_MAPPING,

    /* android.hardware.radio.config@1.1::IRadioConfig */
    RADIO_CONFIG_CALL_1_1(RADIO_CONFIG_REQ_)
    RADIO_CONFIG_1_1_REQ_LAST = RADIO_CONFIG_REQ_GET_MODEMS_CONFIG,

    /* android.hardware.radio.config@1.3::IRadioConfig */
    RADIO_CONFIG_CALL_1_3(RADIO_CONFIG_REQ_)
    RADIO_CONFIG_1_3_REQ_LAST = RADIO_CONFIG_REQ_GET_HAL_DEVICE_CAPABILITIES

#undef RADIO_CONFIG_REQ_
} RADIO_CONFIG_REQ;
G_STATIC_ASSERT(sizeof(RADIO_CONFIG_REQ) == 4);

typedef enum radio_config_resp {
    RADIO_CONFIG_RESP_ANY = 0,
    RADIO_CONFIG_RESP_NONE = 0,
#define RADIO_CONFIG_RESP_(req,resp,Name,NAME) RADIO_CONFIG_RESP_##NAME = resp,

    /* android.hardware.radio.config@1.0::IRadioConfigResponse */
    RADIO_CONFIG_CALL_1_0(RADIO_CONFIG_RESP_)
    RADIO_CONFIG_1_0_RESP_LAST = RADIO_CONFIG_RESP_SET_SIM_SLOTS_MAPPING,

    /* android.hardware.radio.config@1.1::IRadioConfigResponse */
    RADIO_CONFIG_CALL_1_1(RADIO_CONFIG_RESP_)
    RADIO_CONFIG_1_1_RESP_LAST = RADIO_CONFIG_RESP_GET_MODEMS_CONFIG,

    /* android.hardware.radio.config@1.2::IRadioConfigResponse */
    RADIO_CONFIG_RESP_GET_SIM_SLOTS_STATUS_1_2 = 7, /* Since 1.5.0 */
    RADIO_CONFIG_1_2_RESP_LAST = RADIO_CONFIG_RESP_GET_SIM_SLOTS_STATUS_1_2,

    /* android.hardware.radio.config@1.3::IRadioConfigResponse */
    RADIO_CONFIG_CALL_1_3(RADIO_CONFIG_RESP_)
    RADIO_CONFIG_1_3_RESP_LAST = RADIO_CONFIG_RESP_GET_HAL_DEVICE_CAPABILITIES
#undef RADIO_CONFIG_RESP_
} RADIO_CONFIG_RESP;
G_STATIC_ASSERT(sizeof(RADIO_CONFIG_RESP) == 4);

typedef enum radio_config_ind {
    RADIO_CONFIG_IND_ANY = 0,
    RADIO_CONFIG_IND_NONE = 0,
#define RADIO_CONFIG_IND_(code,Name,NAME) RADIO_CONFIG_IND_##NAME = code,

    /* android.hardware.radio.config@1.0::IRadioConfigIndication */
    RADIO_CONFIG_IND_1_0(RADIO_CONFIG_IND_)
    RADIO_CONFIG_1_0_IND_LAST = RADIO_CONFIG_IND_SIM_SLOTS_STATUS_CHANGED,

    /* android.hardware.radio.config@1.2::IRadioConfigIndication */
    RADIO_CONFIG_IND_1_2(RADIO_CONFIG_IND_) /* Since 1.5.0 */
    RADIO_CONFIG_1_2_IND_LAST = RADIO_CONFIG_IND_SIM_SLOTS_STATUS_CHANGED_1_2
#undef RADIO_CONFIG_IND_
} RADIO_CONFIG_IND;
G_STATIC_ASSERT(sizeof(RADIO_CONFIG_IND) == 4);

G_END_DECLS

#endif /* RADIO_CONFIG_TYPES_H */

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
