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

#ifndef RADIO_MODEM_TYPES_H
#define RADIO_MODEM_TYPES_H

#include <gbinder_types.h>

G_BEGIN_DECLS

#define RADIO_MODEM_INTERFACE_MAX (RADIO_MODEM_INTERFACE_COUNT - 1)

#define RADIO_MODEM_INSTANCE         "default"
#define RADIO_MODEM_IFACE_PREFIX     "android.hardware.radio.modem."
#define RADIO_MODEM_IFACE            "IRadioModem"
#define RADIO_MODEM_RESPONSE_IFACE   "IRadioModemResponse"
#define RADIO_MODEM_INDICATION_IFACE "IRadioModemIndication"
#define RADIO_MODEM                  RADIO_MODEM_IFACE_PREFIX RADIO_MODEM_IFACE
#define RADIO_MODEM_FQNAME           RADIO_MODEM "/" RADIO_MODEM_INSTANCE
#define RADIO_MODEM_RESPONSE         RADIO_MODEM_IFACE_PREFIX RADIO_MODEM_RESPONSE_IFACE
#define RADIO_MODEM_INDICATION       RADIO_MODEM_IFACE_PREFIX RADIO_MODEM_INDICATION_IFACE

/* Transaction codes */

/* c(req,resp,Name,CALL_NAME) */
#define RADIO_MODEM_CALL_1(c) \
    c(1,2,enableModem,ENABLE_MODEM) \
    c(2,3,getBasebandVersion,GET_BASEBAND_VERSION) \
    c(3,4,getDeviceIdentity,GET_DEVICE_IDENTITY) \
    c(4,5,getHardwareConfig,GET_HARDWARE_CONFIG) \
    c(5,6,getModemActivityInfo,GET_MODEM_ACTIVITY_INFO) \
    c(6,7,getModemStackStatus,GET_MODEM_STACK_STATUS) \
    c(7,8,getRadioCapability,GET_RADIO_CAPABILITY) \
    c(8,9,nvReadItem,NV_READ_ITEM) \
    c(9,10,nvResetConfig,NV_RESET_CONFIG) \
    c(10,11,nvWriteCdmaPrl,NV_WRITE_CDMA_PRL) \
    c(11,12,nvWriteItem,NV_WRITE_ITEM) \
    c(12,13,requestShutdown,REQUEST_SHUTDOWN) \
    c(14,14,sendDeviceState,SEND_DEVICE_STATE) \
    c(15,15,setRadioCapability,SET_RADIO_CAPABILITY) \
    c(16,16,setRadioPower,SET_RADIO_POWER) \

/* i(code,Name,IND_NAME) */
#define RADIO_MODEM_IND_1(i) \
    i(1,hardwareConfigChanged,HARDWARE_CONFIG_CHANGED) \
    i(2,modemReset,MODEM_RESET) \
    i(3,radioCapabilityIndication,RADIO_CAPABILITY_INDICATION) \
    i(4,radioStateChanged,RADIO_STATE_CHANGED) \
    i(5,rilConnected,RIL_CONNECTED) \

typedef enum radio_modem_req {
    RADIO_MODEM_REQ_ANY = 0,
    RADIO_MODEM_REQ_NONE = 0,
#define RADIO_MODEM_REQ_(req,resp,Name,NAME) RADIO_MODEM_REQ_##NAME = req,

    /* android.hardware.radio.modem.IRadioModem v1 */
    RADIO_MODEM_CALL_1(RADIO_MODEM_REQ_)
    RADIO_MODEM_REQ_RESPONSE_ACKNOWLEDGEMENT = 13, /* responseAcknowledgement */
    RADIO_MODEM_REQ_SET_RESPONSE_FUNCTIONS = 17, /* setResponseFunctions */
    RADIO_MODEM_1_REQ_LAST = RADIO_MODEM_REQ_SET_RESPONSE_FUNCTIONS,

#undef RADIO_MODEM_REQ_
} RADIO_MODEM_REQ;
G_STATIC_ASSERT(sizeof(RADIO_MODEM_REQ) == 4);

typedef enum radio_modem_resp {
    RADIO_MODEM_RESP_ANY = 0,
    RADIO_MODEM_RESP_NONE = 0,
#define RADIO_MODEM_RESP_(req,resp,Name,NAME) RADIO_MODEM_RESP_##NAME = resp,

    /* android.hardware.radio.modem.IRadioModemResponse v1 */
    RADIO_MODEM_CALL_1(RADIO_MODEM_RESP_)
    RADIO_MODEM_RESP_ACKNOWLEDGE_REQUEST = 1, /* acknowledgeRequest */
    RADIO_MODEM_1_RESP_LAST = RADIO_MODEM_RESP_SET_RADIO_POWER,

#undef RADIO_MODEM_RESP_
} RADIO_MODEM_RESP;
G_STATIC_ASSERT(sizeof(RADIO_MODEM_RESP) == 4);

typedef enum radio_modem_ind {
    RADIO_MODEM_IND_ANY = 0,
    RADIO_MODEM_IND_NONE = 0,
#define RADIO_MODEM_IND_(code,Name,NAME) RADIO_MODEM_IND_##NAME = code,

    /* android.hardware.radio.modem.IRadioModemIndication v1 */
    RADIO_MODEM_IND_1(RADIO_MODEM_IND_)
    RADIO_MODEM_1_IND_LAST = RADIO_MODEM_IND_RIL_CONNECTED,

#undef RADIO_MODEM_IND_
} RADIO_MODEM_IND;
G_STATIC_ASSERT(sizeof(RADIO_MODEM_IND) == 4);

G_END_DECLS

#endif /* RADIO_MODEM_TYPES_H */

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
