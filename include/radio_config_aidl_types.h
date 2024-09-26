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

#ifndef RADIO_CONFIG_AIDL_TYPES_H
#define RADIO_CONFIG_AIDL_TYPES_H

#include <radio_types.h>

G_BEGIN_DECLS

typedef enum radio_config_aidl_interface {
    RADIO_CONFIG_AIDL_INTERFACE_NONE = -1,
    RADIO_CONFIG_AIDL_INTERFACE_1,
    RADIO_CONFIG_AIDL_INTERFACE_COUNT
} RADIO_CONFIG_AIDL_INTERFACE;

#define RADIO_CONFIG_AIDL_INTERFACE_MAX (RADIO_CONFIG_AIDL_INTERFACE_COUNT - 1)

#define RADIO_CONFIG_AIDL_INSTANCE         "default"
#define RADIO_CONFIG_AIDL_IFACE_PREFIX     "android.hardware.radio.config."
#define RADIO_CONFIG_AIDL_IFACE            "IRadioConfig"
#define RADIO_CONFIG_AIDL_RESPONSE_IFACE   "IRadioConfigResponse"
#define RADIO_CONFIG_AIDL_INDICATION_IFACE "IRadioConfigIndication"
#define RADIO_CONFIG_AIDL                  RADIO_CONFIG_AIDL_IFACE_PREFIX RADIO_CONFIG_AIDL_IFACE
#define RADIO_CONFIG_AIDL_FQNAME           RADIO_CONFIG_AIDL "/" RADIO_CONFIG_AIDL_INSTANCE
#define RADIO_CONFIG_AIDL_RESPONSE         RADIO_CONFIG_AIDL_IFACE_PREFIX RADIO_CONFIG_AIDL_RESPONSE_IFACE
#define RADIO_CONFIG_AIDL_INDICATION       RADIO_CONFIG_AIDL_IFACE_PREFIX RADIO_CONFIG_AIDL_INDICATION_IFACE

/* Types defined in android.hardware.radio.config package */

/* PhoneCapability */
typedef struct radio_aidl_phone_capability {
    guint8 maxActiveData RADIO_ALIGNED(4);
    guint8 maxActiveInternetData RADIO_ALIGNED(4);
    guint8 isInternetLingeringSupported RADIO_ALIGNED(4);
    struct {
        guint32 length;
        guint8 data[];
    } logicalModemList;
} RadioAidlPhoneCapability;

/* SimPortInfo */
typedef struct radio_aidl_sim_port_info {
    struct {
        guint32 length;
        gchar data[];
    } iccId;
    gint32 logicalSlotId;
    gboolean portActive;
} RadioAidlSimPortInfo;

/* SimSlotStatus */
typedef struct radio_aidl_sim_slot_status {
    RADIO_CARD_STATE cardState;
    GString* atr;
    GString* eid;
    struct {
        guint32 length;
        RadioAidlSimPortInfo data[];
    } portInfo;
} RadioAidlSimSlotStatus;

/* Transaction codes */

/* c(req,resp,Name,CALL_NAME) */
#define RADIO_CONFIG_AIDL_CALL_1(c) \
    c(1,1,getHalDeviceCapabilities,GET_HAL_DEVICE_CAPABILITIES) \
    c(2,2,getNumOfLiveModems,GET_NUM_OF_LIVE_MODEMS) \
    c(3,3,getPhoneCapability,GET_PHONE_CAPABILITY) \
    c(4,4,getSimSlotsStatus,GET_SIM_SLOTS_STATUS) \
    c(5,5,setNumOfLiveModems,SET_NUM_OF_LIVE_MODEMS) \
    c(6,6,setPreferredDataModem,SET_PREFERRED_DATA_MODEM) \
    c(8,7,setSimSlotsMapping,SET_SIM_SLOTS_MAPPING) \

/* i(code,Name,IND_NAME) */
#define RADIO_CONFIG_AIDL_IND_1(i) \
    i(1,simSlotsStatusChanged,SIM_SLOTS_STATUS_CHANGED) \

typedef enum radio_aidl_config_req {
    RADIO_CONFIG_AIDL_REQ_ANY = 0,
    RADIO_CONFIG_AIDL_REQ_NONE = 0,
#define RADIO_CONFIG_AIDL_REQ_(req,resp,Name,NAME) RADIO_CONFIG_AIDL_REQ_##NAME = req,

    /* android.hardware.radio.config.IRadioConfig v1 */
    RADIO_CONFIG_AIDL_REQ_SET_RESPONSE_FUNCTIONS = 7, /* setResponseFunctions */
    RADIO_CONFIG_AIDL_CALL_1(RADIO_CONFIG_AIDL_REQ_)
    RADIO_CONFIG_AIDL_1_REQ_LAST = RADIO_CONFIG_AIDL_REQ_SET_SIM_SLOTS_MAPPING,

#undef RADIO_CONFIG_AIDL_REQ_
} RADIO_CONFIG_AIDL_REQ;
G_STATIC_ASSERT(sizeof(RADIO_CONFIG_AIDL_REQ) == 4);

typedef enum radio_aidl_config_resp {
    RADIO_CONFIG_AIDL_RESP_ANY = 0,
    RADIO_CONFIG_AIDL_RESP_NONE = 0,
#define RADIO_CONFIG_AIDL_RESP_(req,resp,Name,NAME) RADIO_CONFIG_AIDL_RESP_##NAME = resp,

    /* android.hardware.radio.config.IRadioConfigResponse v1 */
    RADIO_CONFIG_AIDL_CALL_1(RADIO_CONFIG_AIDL_RESP_)
    RADIO_CONFIG_AIDL_1_RESP_LAST = RADIO_CONFIG_AIDL_RESP_SET_SIM_SLOTS_MAPPING,

#undef RADIO_CONFIG_AIDL_RESP_
} RADIO_CONFIG_AIDL_RESP;
G_STATIC_ASSERT(sizeof(RADIO_CONFIG_AIDL_RESP) == 4);

typedef enum radio_aidl_config_ind {
    RADIO_CONFIG_AIDL_IND_ANY = 0,
    RADIO_CONFIG_AIDL_IND_NONE = 0,
#define RADIO_CONFIG_AIDL_IND_(code,Name,NAME) RADIO_CONFIG_AIDL_IND_##NAME = code,

    /* android.hardware.radio.config.IRadioConfigIndication v1 */
    RADIO_CONFIG_AIDL_IND_1(RADIO_CONFIG_AIDL_IND_)
    RADIO_CONFIG_AIDL_1_IND_LAST = RADIO_CONFIG_AIDL_IND_SIM_SLOTS_STATUS_CHANGED,

#undef RADIO_CONFIG_AIDL_IND_
} RADIO_CONFIG_AIDL_IND;
G_STATIC_ASSERT(sizeof(RADIO_CONFIG_AIDL_IND) == 4);

G_END_DECLS

#endif /* RADIO_CONFIG_AIDL_TYPES_H */

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
