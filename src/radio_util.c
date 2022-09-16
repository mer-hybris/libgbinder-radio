/*
 * Copyright (C) 2018-2022 Jolla Ltd.
 * Copyright (C) 2018-2022 Slava Monich <slava.monich@jolla.com>
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

#include "radio_util_p.h"
#include "radio_log.h"

GLOG_MODULE_DEFINE("gbinder-radio");

guint
radio_observer_priority_index(
    RADIO_OBSERVER_PRIORITY priority)
{
    if (priority < RADIO_OBSERVER_PRIORITY_LOWEST) {
        return 0;
    } else if (priority > RADIO_OBSERVER_PRIORITY_HIGHEST) {
        return RADIO_OBSERVER_PRIORITY_COUNT - 1;
    } else {
        return priority - RADIO_OBSERVER_PRIORITY_LOWEST;
    }
}

const char*
radio_req_name(
    RADIO_REQ req)
{
    switch (req) {
    case RADIO_REQ_SET_RESPONSE_FUNCTIONS:   return "setResponseFunctions";
    case RADIO_REQ_RESPONSE_ACKNOWLEDGEMENT: return "responseAcknowledgement";
#define RADIO_REQ_(req,resp,Name,NAME) \
    case RADIO_REQ_##NAME: return #Name;
    RADIO_CALL_1_0(RADIO_REQ_)
    RADIO_CALL_1_1(RADIO_REQ_)
    RADIO_CALL_1_2(RADIO_REQ_)
    RADIO_CALL_1_3(RADIO_REQ_)
    RADIO_CALL_1_4(RADIO_REQ_)
    RADIO_CALL_1_5(RADIO_REQ_)
#undef RADIO_REQ_
    case RADIO_REQ_START_NETWORK_SCAN_1_2:     return "startNetworkScan_1_2";
    case RADIO_REQ_SET_INDICATION_FILTER_1_2:  return "setIndicationFilter_1_2";
    case RADIO_REQ_SETUP_DATA_CALL_1_2:        return "setupDataCall_1_2";
    case RADIO_REQ_DEACTIVATE_DATA_CALL_1_2:   return "deactivateDataCall_1_2";
    case RADIO_REQ_SET_INITIAL_ATTACH_APN_1_4: return "setInitialAttachApn_1_4";
    case RADIO_REQ_SET_DATA_PROFILE_1_4:       return "setDataProfile_1_4";
    case RADIO_REQ_ANY:
        break;
    }
    return NULL;
}

const char*
radio_resp_name(
    RADIO_RESP resp)
{
    switch (resp) {
    case RADIO_RESP_ACKNOWLEDGE_REQUEST: return "acknowledgeRequest";
#define RADIO_RESP_(req,resp,Name,NAME) \
    case RADIO_RESP_##NAME: return #Name "Response";
    RADIO_CALL_1_0(RADIO_RESP_)
    RADIO_CALL_1_1(RADIO_RESP_)
    RADIO_CALL_1_2(RADIO_RESP_)
    RADIO_CALL_1_3(RADIO_RESP_)
    RADIO_CALL_1_4(RADIO_RESP_)
    RADIO_CALL_1_5(RADIO_RESP_)
#undef RADIO_RESP_
    case RADIO_RESP_GET_CELL_INFO_LIST_1_2:
        return "getCellInfoListResponse_1_2";
    case RADIO_RESP_GET_ICC_CARD_STATUS_1_2:
        return "getIccCardStatusResponse_1_2";
    case RADIO_RESP_GET_CURRENT_CALLS_1_2:
        return "getCurrentCallsResponse_1_2";
    case RADIO_RESP_GET_SIGNAL_STRENGTH_1_2:
        return "getSignalStrengthResponse_1_2";
    case RADIO_RESP_GET_VOICE_REGISTRATION_STATE_1_2:
        return "getVoiceRegistrationStateResponse_1_2";
    case RADIO_RESP_GET_DATA_REGISTRATION_STATE_1_2:
        return "getDataRegistrationStateResponse_1_2";
    case RADIO_RESP_GET_CELL_INFO_LIST_RESPONSE_1_4:
        return "getCellInfoListResponse_1_4";
    case RADIO_RESP_GET_DATA_REGISTRATION_STATE_RESPONSE_1_4:
        return "getDataRegistrationStateResponse_1_4";
    case RADIO_RESP_GET_ICC_CARD_STATUS_RESPONSE_1_4:
        return "getIccCardStatusResponse_1_4";
    case RADIO_RESP_GET_DATA_CALL_LIST_RESPONSE_1_4:
        return "getDataCallListResponse_1_4";
    case RADIO_RESP_GET_DATA_CALL_LIST_1_5:
        return "getDataCallList_1_5";
    case RADIO_RESP_GET_CELL_INFO_LIST_1_5:
        return "getCellInfoListResponse_1_5";
    case RADIO_RESP_GET_ICC_CARD_STATUS_1_5:
        return "getIccCardStatus_1_5";
    case RADIO_RESP_ANY:
        break;
    }
    return NULL;
}

const char*
radio_ind_name(
    RADIO_IND ind)
{
    switch (ind) {
#define RADIO_IND_(code,Name,NAME) \
    case RADIO_IND_##NAME: return #Name;
    RADIO_EVENT_1_0(RADIO_IND_)
    RADIO_EVENT_1_1(RADIO_IND_)
    RADIO_EVENT_1_2(RADIO_IND_)
    RADIO_EVENT_1_4(RADIO_IND_)
    RADIO_EVENT_1_5(RADIO_IND_)
#undef RADIO_IND_
    case RADIO_IND_ANY:
        break;
    }
    return NULL;
}

/**
 * This function no longer makes as much sense as it did in IRadio 1.0 times.
 * Later it turned out that that same call may produce different responses
 * under different circumstances. For example, getIccCardStatus call may
 * cause getIccCardStatusResponse or getIccCardStatusResponse_1_2 to be
 * sent back, depending on which interfaces are supported by the caller.
 * There's no longer one-to-one match between requests and responses,
 * that would be too easy and straightforward for Google designers :)
 *
 * Use this function carefully or better don't use it at all.
 */
RADIO_RESP
radio_req_resp(
    RADIO_REQ req)
{
    switch (req) {
#define RADIO_REQ_(req,resp,Name,NAME) \
    case RADIO_REQ_##NAME: return RADIO_RESP_##NAME;
    RADIO_CALL_1_0(RADIO_REQ_)
    RADIO_CALL_1_1(RADIO_REQ_)
    RADIO_CALL_1_2(RADIO_REQ_)
    RADIO_CALL_1_3(RADIO_REQ_)
    RADIO_CALL_1_4(RADIO_REQ_)
    RADIO_CALL_1_5(RADIO_REQ_)
#undef RADIO_REQ_
    case RADIO_REQ_SETUP_DATA_CALL_1_2:
        return RADIO_RESP_SETUP_DATA_CALL;
    case RADIO_REQ_DEACTIVATE_DATA_CALL_1_2:
        return RADIO_RESP_DEACTIVATE_DATA_CALL;
    case RADIO_REQ_START_NETWORK_SCAN_1_2:
        return RADIO_RESP_START_NETWORK_SCAN;
    case RADIO_REQ_SET_INITIAL_ATTACH_APN_1_4:
        return RADIO_RESP_SET_INITIAL_ATTACH_APN;
    case RADIO_REQ_SET_DATA_PROFILE_1_4:
        return RADIO_RESP_SET_DATA_PROFILE;
    case RADIO_REQ_SET_INDICATION_FILTER_1_2:
        return RADIO_RESP_SET_INDICATION_FILTER;

    /*
     * All these still need to be listed here to ensure a compilation
     * warnings when something gets added to RADIO_REQ enum.
     */
    case RADIO_REQ_SET_RESPONSE_FUNCTIONS:
    case RADIO_REQ_RESPONSE_ACKNOWLEDGEMENT:
    case RADIO_REQ_ANY:
        break;
    }
    return RADIO_RESP_NONE;
}

/**
 * And this is a version of radio_req_resp which takes IRadio interface
 * version into account. This one is OK to use.
 */
RADIO_RESP
radio_req_resp2(
    RADIO_REQ req,
    RADIO_INTERFACE iface) /* Since 1.4.5 */
{
    switch (req) {
    /*
     * Requests expecting a response from a previous version of the
     * interface.
     */
    case RADIO_REQ_SETUP_DATA_CALL_1_2:
        return RADIO_RESP_SETUP_DATA_CALL;
    case RADIO_REQ_DEACTIVATE_DATA_CALL_1_2:
        return RADIO_RESP_DEACTIVATE_DATA_CALL;
    case RADIO_REQ_START_NETWORK_SCAN_1_2:
        return RADIO_RESP_START_NETWORK_SCAN;
    case RADIO_REQ_SET_INITIAL_ATTACH_APN_1_4:
        return RADIO_RESP_SET_INITIAL_ATTACH_APN;
    case RADIO_REQ_SET_DATA_PROFILE_1_4:
        return RADIO_RESP_SET_DATA_PROFILE;
    case RADIO_REQ_SET_INDICATION_FILTER_1_2:
 /* case RADIO_REQ_SET_INDICATION_FILTER_1_5: */
        return RADIO_RESP_SET_INDICATION_FILTER;

    /*
     * Requests which may receive a response from a higher version of
     * the interface.
     */

    /*
     * getIccCardStatus
     * getIccCardStatusResponse
     * getIccCardStatusResponse_1_2
     * getIccCardStatusResponse_1_4
     * getIccCardStatusResponse_1_5
     * ...
     */
    case RADIO_REQ_GET_ICC_CARD_STATUS:
        switch (iface) {
        case RADIO_INTERFACE_1_0:
        case RADIO_INTERFACE_1_1:
            return RADIO_RESP_GET_ICC_CARD_STATUS;
        case RADIO_INTERFACE_1_2:
        case RADIO_INTERFACE_1_3:
            return RADIO_RESP_GET_ICC_CARD_STATUS_1_2;
        case RADIO_INTERFACE_1_4:
            return RADIO_RESP_GET_ICC_CARD_STATUS_1_4;
        case RADIO_INTERFACE_1_5:
            return RADIO_RESP_GET_ICC_CARD_STATUS_1_5;
        case RADIO_INTERFACE_NONE:
        case RADIO_INTERFACE_COUNT:
            break;
        }
        return RADIO_RESP_NONE;

    /*
     * getCellInfoList
     * getCellInfoListResponse
     * getCellInfoListResponse_1_2
     * getCellInfoListResponse_1_4
     * getCellInfoListResponse_1_5 <= the last one
     * getCellInfoList_1_6
     */
    case RADIO_REQ_GET_CELL_INFO_LIST:
        switch (iface) {
        case RADIO_INTERFACE_1_0:
        case RADIO_INTERFACE_1_1:
            return RADIO_RESP_GET_CELL_INFO_LIST;
        case RADIO_INTERFACE_1_2:
        case RADIO_INTERFACE_1_3:
            return RADIO_RESP_GET_CELL_INFO_LIST_1_2;
        case RADIO_INTERFACE_1_4:
            return RADIO_RESP_GET_CELL_INFO_LIST_1_4;
        default:
            return RADIO_RESP_GET_CELL_INFO_LIST_1_5;
        case RADIO_INTERFACE_NONE:
            break;
        }
        return RADIO_RESP_NONE;

    /*
     * getCurrentCalls
     * getCurrentCallsResponse
     * getCurrentCallsResponse_1_2 <= the last one
     * getCurrentCalls_1_6
     */
    case RADIO_REQ_GET_CURRENT_CALLS:
        switch (iface) {
        case RADIO_INTERFACE_1_0:
        case RADIO_INTERFACE_1_1:
            return RADIO_RESP_GET_CURRENT_CALLS;
        default: /* The last one */
            return RADIO_RESP_GET_CURRENT_CALLS_1_2;
        case RADIO_INTERFACE_NONE:
            break;
        }
        return RADIO_RESP_NONE;

    /*
     * getSignalStrength
     * getSignalStrengthResponse
     * getSignalStrengthResponse_1_2 <= the last one
     * getSignalStrength_1_4
     */
    case RADIO_REQ_GET_SIGNAL_STRENGTH:
        switch (iface) {
        case RADIO_INTERFACE_1_0:
        case RADIO_INTERFACE_1_1:
            return RADIO_RESP_GET_SIGNAL_STRENGTH;
        default: /* The last one */
            return RADIO_RESP_GET_SIGNAL_STRENGTH_1_2;
        case RADIO_INTERFACE_NONE:
            break;
        }
        return RADIO_RESP_NONE;

    /*
     * getVoiceRegistrationState
     * getVoiceRegistrationStateResponse
     * getVoiceRegistrationStateResponse_1_2 <= the last one
     * getVoiceRegistrationState_1_5
     */
    case RADIO_REQ_GET_VOICE_REGISTRATION_STATE:
        switch (iface) {
        case RADIO_INTERFACE_1_0:
        case RADIO_INTERFACE_1_1:
            return RADIO_RESP_GET_VOICE_REGISTRATION_STATE;
        default: /* The last one */
            return RADIO_RESP_GET_VOICE_REGISTRATION_STATE_1_2;
        case RADIO_INTERFACE_NONE:
            break;
        }
        return RADIO_RESP_NONE;

    /*
     * getDataRegistrationState
     * getDataRegistrationStateResponse
     * getDataRegistrationStateResponse_1_2
     * getDataRegistrationStateResponse_1_4 <= the last one
     * getDataRegistrationState_1_5
     */
    case RADIO_REQ_GET_DATA_REGISTRATION_STATE:
        switch (iface) {
        case RADIO_INTERFACE_1_0:
        case RADIO_INTERFACE_1_1:
            return RADIO_RESP_GET_DATA_REGISTRATION_STATE;
        case RADIO_INTERFACE_1_2:
        case RADIO_INTERFACE_1_3:
            return RADIO_RESP_GET_DATA_REGISTRATION_STATE_1_2;
        default: /* The last one */
            return RADIO_RESP_GET_DATA_REGISTRATION_STATE_1_4;
        case RADIO_INTERFACE_NONE:
            break;
        }
        return RADIO_RESP_NONE;

    /*
     * getDataCallList
     * getDataCallListResponse
     * getDataCallListResponse_1_4
     * getDataCallListResponse_1_5 <= the last one
     * getDataCallList_1_6
     */
    case RADIO_REQ_GET_DATA_CALL_LIST:
        switch (iface) {
        case RADIO_INTERFACE_1_0:
        case RADIO_INTERFACE_1_1:
        case RADIO_INTERFACE_1_2:
        case RADIO_INTERFACE_1_3:
            return RADIO_RESP_GET_DATA_CALL_LIST;
        case RADIO_INTERFACE_1_4:
            return RADIO_RESP_GET_DATA_CALL_LIST_1_4;
        default: /* The last one */
            return RADIO_RESP_GET_DATA_CALL_LIST_1_5;
        case RADIO_INTERFACE_NONE:
            break;
        }
        return RADIO_RESP_NONE;

    default:
        break;
    }

    /* Fallback */
    G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    return radio_req_resp(req);
    G_GNUC_END_IGNORE_DEPRECATIONS
}

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */

