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

#include "test_common.h"

#include "radio_util.h"

#define UNKNOWN_VALUE (0x7fffffff)
#define UNKNOWN_REQ ((RADIO_REQ)UNKNOWN_VALUE)
#define UNKNOWN_IND ((RADIO_REQ)UNKNOWN_VALUE)
#define UNKNOWN_RESP ((RADIO_RESP)UNKNOWN_VALUE)

static TestOpt test_opt;

/*==========================================================================*
 * req_name
 *==========================================================================*/

static
void
test_req_name(
    void)
{
    g_assert(!radio_req_name(UNKNOWN_REQ));
    g_assert(!radio_req_name(RADIO_REQ_ANY));
    g_assert_cmpstr(radio_req_name(RADIO_REQ_GET_ICC_CARD_STATUS),==,
        "getIccCardStatus");
    g_assert_cmpstr(radio_req_name(RADIO_REQ_START_NETWORK_SCAN),==,
        "startNetworkScan");
    g_assert_cmpstr(radio_req_name(RADIO_REQ_SET_SIGNAL_STRENGTH_REPORTING_CRITERIA),==,
        "setSignalStrengthReportingCriteria");
    g_assert_cmpstr(radio_req_name(RADIO_REQ_SET_SYSTEM_SELECTION_CHANNELS),==,
        "setSystemSelectionChannels");
    g_assert_cmpstr(radio_req_name(RADIO_REQ_EMERGENCY_DIAL),==,
        "emergencyDial");
    g_assert_cmpstr(radio_req_name(RADIO_REQ_ENABLE_UICC_APPLICATIONS),==,
        "enableUiccApplications");
    g_assert_cmpstr(radio_req_name(RADIO_REQ_START_NETWORK_SCAN_1_4),==,
        "startNetworkScan_1_4");
    g_assert_cmpstr(radio_req_name(RADIO_REQ_START_NETWORK_SCAN_1_5),==,
        "startNetworkScan_1_5");
}

/*==========================================================================*
 * resp_name
 *==========================================================================*/

static
void
test_resp_name(
    void)
{
    g_assert(!radio_resp_name(UNKNOWN_RESP));
    g_assert(!radio_resp_name(RADIO_RESP_ANY));
    g_assert_cmpstr(radio_resp_name(RADIO_RESP_GET_ICC_CARD_STATUS),==,
        "getIccCardStatusResponse");
    g_assert_cmpstr(radio_resp_name(RADIO_RESP_START_NETWORK_SCAN),==,
        "startNetworkScanResponse");
    g_assert_cmpstr(radio_resp_name(RADIO_RESP_SET_SIGNAL_STRENGTH_REPORTING_CRITERIA),==,
        "setSignalStrengthReportingCriteriaResponse");
    g_assert_cmpstr(radio_resp_name(RADIO_RESP_SET_SYSTEM_SELECTION_CHANNELS),==,
        "setSystemSelectionChannelsResponse");
    g_assert_cmpstr(radio_resp_name(RADIO_RESP_EMERGENCY_DIAL),==,
        "emergencyDialResponse");
    g_assert_cmpstr(radio_resp_name(RADIO_RESP_ENABLE_UICC_APPLICATIONS),==,
        "enableUiccApplicationsResponse");
    g_assert_cmpstr(radio_resp_name(RADIO_RESP_START_NETWORK_SCAN_1_4),==,
        "startNetworkScanResponse_1_4");
    g_assert_cmpstr(radio_resp_name(RADIO_RESP_START_NETWORK_SCAN_1_5),==,
        "startNetworkScanResponse_1_5");
}

/*==========================================================================*
 * ind_name
 *==========================================================================*/

static
void
test_ind_name(
    void)
{
    g_assert(!radio_ind_name(UNKNOWN_IND));
    g_assert(!radio_ind_name(RADIO_IND_ANY));
    g_assert_cmpstr(radio_ind_name(RADIO_IND_RADIO_STATE_CHANGED),==,
        "radioStateChanged");
    g_assert_cmpstr(radio_ind_name(RADIO_IND_NETWORK_SCAN_RESULT),==,
        "networkScanResult");
    g_assert_cmpstr(radio_ind_name(RADIO_IND_CURRENT_LINK_CAPACITY_ESTIMATE),==,
        "currentLinkCapacityEstimate");
    g_assert_cmpstr(radio_ind_name(RADIO_IND_CURRENT_EMERGENCY_NUMBER_LIST),==,
        "currentEmergencyNumberList");
    g_assert_cmpstr(radio_ind_name(RADIO_IND_REGISTRATION_FAILED),==,
        "registrationFailed");
}

/*==========================================================================*
 * req_resp
 *==========================================================================*/

static
void
test_req_resp(
    void)
{
    static const struct radio_req_resp_data {
        RADIO_REQ req;
        RADIO_RESP resp;
    } tests[] = {
        { UNKNOWN_REQ, RADIO_RESP_NONE },
        { RADIO_REQ_ANY, RADIO_RESP_NONE },
        { RADIO_REQ_SETUP_DATA_CALL_1_2, RADIO_RESP_SETUP_DATA_CALL },
        { RADIO_REQ_DEACTIVATE_DATA_CALL_1_2, RADIO_RESP_DEACTIVATE_DATA_CALL },
        { RADIO_REQ_START_NETWORK_SCAN_1_2, RADIO_RESP_START_NETWORK_SCAN },
        { RADIO_REQ_SET_INITIAL_ATTACH_APN_1_4,
          RADIO_RESP_SET_INITIAL_ATTACH_APN },
        { RADIO_REQ_SET_DATA_PROFILE_1_4, RADIO_RESP_SET_DATA_PROFILE },
        { RADIO_REQ_SET_INDICATION_FILTER_1_2,
          RADIO_RESP_SET_INDICATION_FILTER },
        { RADIO_REQ_GET_ICC_CARD_STATUS, RADIO_RESP_GET_ICC_CARD_STATUS },
        { RADIO_REQ_START_NETWORK_SCAN, RADIO_RESP_START_NETWORK_SCAN },
        { RADIO_REQ_SET_SIGNAL_STRENGTH_REPORTING_CRITERIA,
          RADIO_RESP_SET_SIGNAL_STRENGTH_REPORTING_CRITERIA },
        { RADIO_REQ_SET_SYSTEM_SELECTION_CHANNELS,
          RADIO_RESP_SET_SYSTEM_SELECTION_CHANNELS },
        { RADIO_REQ_EMERGENCY_DIAL, RADIO_RESP_EMERGENCY_DIAL }
    };

    int i;

    for (i = 0; i < G_N_ELEMENTS(tests); i++) {
        g_assert_cmpint(radio_req_resp(tests[i].req), ==, tests[i].resp);
    }
}

/*==========================================================================*
 * req_resp2
 *==========================================================================*/

static
void
test_req_resp2(
    void)
{
    static const struct radio_req_resp2_data {
        RADIO_REQ req;
        RADIO_INTERFACE iface;
        RADIO_RESP resp;
    } tests[] = {
        { UNKNOWN_REQ, RADIO_INTERFACE_NONE, RADIO_RESP_NONE },
        { RADIO_REQ_SUPPLY_ICC_PIN_FOR_APP, RADIO_INTERFACE_1_0,
          RADIO_RESP_SUPPLY_ICC_PIN_FOR_APP },
        { RADIO_REQ_SUPPLY_ICC_PUK_FOR_APP, RADIO_INTERFACE_1_1,
          RADIO_RESP_SUPPLY_ICC_PUK_FOR_APP },
        { RADIO_REQ_SUPPLY_ICC_PIN2_FOR_APP, RADIO_INTERFACE_1_2,
          RADIO_RESP_SUPPLY_ICC_PIN2_FOR_APP },
        { RADIO_REQ_SUPPLY_ICC_PUK2_FOR_APP, RADIO_INTERFACE_1_3,
          RADIO_RESP_SUPPLY_ICC_PUK2_FOR_APP },
        { RADIO_REQ_CHANGE_ICC_PIN_FOR_APP, RADIO_INTERFACE_1_4,
          RADIO_RESP_CHANGE_ICC_PIN_FOR_APP },
        { RADIO_REQ_CHANGE_ICC_PIN2_FOR_APP, RADIO_INTERFACE_COUNT,
          RADIO_RESP_CHANGE_ICC_PIN2_FOR_APP },

        { RADIO_REQ_SETUP_DATA_CALL_1_2, RADIO_INTERFACE_1_2,
          RADIO_RESP_SETUP_DATA_CALL },
        { RADIO_REQ_SETUP_DATA_CALL_1_2, RADIO_INTERFACE_1_4,
          RADIO_RESP_SETUP_DATA_CALL },

        { RADIO_REQ_DEACTIVATE_DATA_CALL_1_2, RADIO_INTERFACE_1_2,
          RADIO_RESP_DEACTIVATE_DATA_CALL },
        { RADIO_REQ_DEACTIVATE_DATA_CALL_1_2, RADIO_INTERFACE_1_4,
          RADIO_RESP_DEACTIVATE_DATA_CALL },

        { RADIO_REQ_START_NETWORK_SCAN_1_2, RADIO_INTERFACE_1_2,
          RADIO_RESP_START_NETWORK_SCAN },
        { RADIO_REQ_START_NETWORK_SCAN_1_2, RADIO_INTERFACE_1_4,
          RADIO_RESP_START_NETWORK_SCAN },

        { RADIO_REQ_SET_INITIAL_ATTACH_APN_1_4, RADIO_INTERFACE_1_4,
          RADIO_RESP_SET_INITIAL_ATTACH_APN },

        { RADIO_REQ_SET_DATA_PROFILE_1_4, RADIO_INTERFACE_1_4,
          RADIO_RESP_SET_DATA_PROFILE },

        { RADIO_REQ_SET_INDICATION_FILTER_1_2, RADIO_INTERFACE_1_2,
          RADIO_RESP_SET_INDICATION_FILTER },
        { RADIO_REQ_SET_INDICATION_FILTER_1_2, RADIO_INTERFACE_1_4,
          RADIO_RESP_SET_INDICATION_FILTER },
        { RADIO_REQ_SET_INDICATION_FILTER_1_5, RADIO_INTERFACE_1_5,
          RADIO_RESP_SET_INDICATION_FILTER_1_5 },


        { RADIO_REQ_GET_ICC_CARD_STATUS, RADIO_INTERFACE_1_0,
          RADIO_RESP_GET_ICC_CARD_STATUS },
        { RADIO_REQ_GET_ICC_CARD_STATUS, RADIO_INTERFACE_1_1,
          RADIO_RESP_GET_ICC_CARD_STATUS },
        { RADIO_REQ_GET_ICC_CARD_STATUS, RADIO_INTERFACE_1_2,
          RADIO_RESP_GET_ICC_CARD_STATUS_1_2 },
        { RADIO_REQ_GET_ICC_CARD_STATUS, RADIO_INTERFACE_1_3,
          RADIO_RESP_GET_ICC_CARD_STATUS_1_2 },
        { RADIO_REQ_GET_ICC_CARD_STATUS, RADIO_INTERFACE_1_4,
          RADIO_RESP_GET_ICC_CARD_STATUS_1_4 },
        { RADIO_REQ_GET_ICC_CARD_STATUS, RADIO_INTERFACE_1_5,
          RADIO_RESP_GET_ICC_CARD_STATUS_1_5 },
        { RADIO_REQ_GET_ICC_CARD_STATUS, RADIO_INTERFACE_COUNT,
          RADIO_RESP_NONE },
        { RADIO_REQ_GET_ICC_CARD_STATUS, RADIO_INTERFACE_NONE,
          RADIO_RESP_NONE },

        { RADIO_REQ_GET_CELL_INFO_LIST, RADIO_INTERFACE_1_0,
          RADIO_RESP_GET_CELL_INFO_LIST },
        { RADIO_REQ_GET_CELL_INFO_LIST, RADIO_INTERFACE_1_1,
          RADIO_RESP_GET_CELL_INFO_LIST },
        { RADIO_REQ_GET_CELL_INFO_LIST, RADIO_INTERFACE_1_2,
          RADIO_RESP_GET_CELL_INFO_LIST_1_2 },
        { RADIO_REQ_GET_CELL_INFO_LIST, RADIO_INTERFACE_1_3,
          RADIO_RESP_GET_CELL_INFO_LIST_1_2 },
        { RADIO_REQ_GET_CELL_INFO_LIST, RADIO_INTERFACE_1_4,
          RADIO_RESP_GET_CELL_INFO_LIST_1_4 },
        { RADIO_REQ_GET_CELL_INFO_LIST, RADIO_INTERFACE_1_5,
          RADIO_RESP_GET_CELL_INFO_LIST_1_5 },
        { RADIO_REQ_GET_CELL_INFO_LIST, RADIO_INTERFACE_COUNT,
          RADIO_RESP_GET_CELL_INFO_LIST_1_5 },
        { RADIO_REQ_GET_CELL_INFO_LIST, RADIO_INTERFACE_NONE,
          RADIO_RESP_NONE },

        { RADIO_REQ_GET_CURRENT_CALLS, RADIO_INTERFACE_1_0,
          RADIO_RESP_GET_CURRENT_CALLS },
        { RADIO_REQ_GET_CURRENT_CALLS, RADIO_INTERFACE_1_1,
          RADIO_RESP_GET_CURRENT_CALLS },
        { RADIO_REQ_GET_CURRENT_CALLS, RADIO_INTERFACE_1_2,
          RADIO_RESP_GET_CURRENT_CALLS_1_2 },
        { RADIO_REQ_GET_CURRENT_CALLS, RADIO_INTERFACE_1_3,
          RADIO_RESP_GET_CURRENT_CALLS_1_2 },
        { RADIO_REQ_GET_CURRENT_CALLS, RADIO_INTERFACE_1_4,
          RADIO_RESP_GET_CURRENT_CALLS_1_2 },
        { RADIO_REQ_GET_CURRENT_CALLS, RADIO_INTERFACE_COUNT,
          RADIO_RESP_GET_CURRENT_CALLS_1_2 },
        { RADIO_REQ_GET_CURRENT_CALLS, RADIO_INTERFACE_NONE,
          RADIO_RESP_NONE },

        { RADIO_REQ_GET_SIGNAL_STRENGTH, RADIO_INTERFACE_1_0,
          RADIO_RESP_GET_SIGNAL_STRENGTH },
        { RADIO_REQ_GET_SIGNAL_STRENGTH, RADIO_INTERFACE_1_1,
          RADIO_RESP_GET_SIGNAL_STRENGTH },
        { RADIO_REQ_GET_SIGNAL_STRENGTH, RADIO_INTERFACE_1_2,
          RADIO_RESP_GET_SIGNAL_STRENGTH_1_2 },
        { RADIO_REQ_GET_SIGNAL_STRENGTH, RADIO_INTERFACE_1_3,
          RADIO_RESP_GET_SIGNAL_STRENGTH_1_2 },
        { RADIO_REQ_GET_SIGNAL_STRENGTH, RADIO_INTERFACE_1_4,
          RADIO_RESP_GET_SIGNAL_STRENGTH_1_2 },
        { RADIO_REQ_GET_SIGNAL_STRENGTH, RADIO_INTERFACE_COUNT,
          RADIO_RESP_GET_SIGNAL_STRENGTH_1_2 },
        { RADIO_REQ_GET_SIGNAL_STRENGTH, RADIO_INTERFACE_NONE,
          RADIO_RESP_NONE },

        { RADIO_REQ_GET_VOICE_REGISTRATION_STATE, RADIO_INTERFACE_1_0,
          RADIO_RESP_GET_VOICE_REGISTRATION_STATE },
        { RADIO_REQ_GET_VOICE_REGISTRATION_STATE, RADIO_INTERFACE_1_1,
          RADIO_RESP_GET_VOICE_REGISTRATION_STATE },
        { RADIO_REQ_GET_VOICE_REGISTRATION_STATE, RADIO_INTERFACE_1_2,
          RADIO_RESP_GET_VOICE_REGISTRATION_STATE_1_2 },
        { RADIO_REQ_GET_VOICE_REGISTRATION_STATE, RADIO_INTERFACE_1_3,
          RADIO_RESP_GET_VOICE_REGISTRATION_STATE_1_2 },
        { RADIO_REQ_GET_VOICE_REGISTRATION_STATE, RADIO_INTERFACE_1_4,
          RADIO_RESP_GET_VOICE_REGISTRATION_STATE_1_2 },
        { RADIO_REQ_GET_VOICE_REGISTRATION_STATE, RADIO_INTERFACE_COUNT,
          RADIO_RESP_GET_VOICE_REGISTRATION_STATE_1_2 },
        { RADIO_REQ_GET_VOICE_REGISTRATION_STATE, RADIO_INTERFACE_NONE,
          RADIO_RESP_NONE },

        { RADIO_REQ_GET_DATA_REGISTRATION_STATE, RADIO_INTERFACE_1_0,
          RADIO_RESP_GET_DATA_REGISTRATION_STATE },
        { RADIO_REQ_GET_DATA_REGISTRATION_STATE, RADIO_INTERFACE_1_1,
          RADIO_RESP_GET_DATA_REGISTRATION_STATE },
        { RADIO_REQ_GET_DATA_REGISTRATION_STATE, RADIO_INTERFACE_1_2,
          RADIO_RESP_GET_DATA_REGISTRATION_STATE_1_2 },
        { RADIO_REQ_GET_DATA_REGISTRATION_STATE, RADIO_INTERFACE_1_3,
          RADIO_RESP_GET_DATA_REGISTRATION_STATE_1_2 },
        { RADIO_REQ_GET_DATA_REGISTRATION_STATE, RADIO_INTERFACE_1_4,
          RADIO_RESP_GET_DATA_REGISTRATION_STATE_1_4 },
        { RADIO_REQ_GET_DATA_REGISTRATION_STATE, RADIO_INTERFACE_COUNT,
          RADIO_RESP_GET_DATA_REGISTRATION_STATE_1_4 },
        { RADIO_REQ_GET_DATA_REGISTRATION_STATE, RADIO_INTERFACE_NONE,
          RADIO_RESP_NONE },

        { RADIO_REQ_GET_DATA_CALL_LIST, RADIO_INTERFACE_1_0,
          RADIO_RESP_GET_DATA_CALL_LIST },
        { RADIO_REQ_GET_DATA_CALL_LIST, RADIO_INTERFACE_1_1,
          RADIO_RESP_GET_DATA_CALL_LIST },
        { RADIO_REQ_GET_DATA_CALL_LIST, RADIO_INTERFACE_1_2,
          RADIO_RESP_GET_DATA_CALL_LIST },
        { RADIO_REQ_GET_DATA_CALL_LIST, RADIO_INTERFACE_1_3,
          RADIO_RESP_GET_DATA_CALL_LIST },
        { RADIO_REQ_GET_DATA_CALL_LIST, RADIO_INTERFACE_1_4,
          RADIO_RESP_GET_DATA_CALL_LIST_1_4 },
        { RADIO_REQ_GET_DATA_CALL_LIST, RADIO_INTERFACE_1_5,
          RADIO_RESP_GET_DATA_CALL_LIST_1_5 },
        { RADIO_REQ_GET_DATA_CALL_LIST, RADIO_INTERFACE_COUNT,
          RADIO_RESP_GET_DATA_CALL_LIST_1_5 },
        { RADIO_REQ_GET_DATA_CALL_LIST, RADIO_INTERFACE_NONE,
          RADIO_RESP_NONE }
    };

    int i;

    for (i = 0; i < G_N_ELEMENTS(tests); i++) {
        g_assert_cmpint(radio_req_resp2(tests[i].req, tests[i].iface), ==,
            tests[i].resp);
    }
}

/*==========================================================================*
 * Common
 *==========================================================================*/

#define TEST_PREFIX "/util/"
#define TEST_(t) TEST_PREFIX t

int main(int argc, char* argv[])
{
    g_test_init(&argc, &argv, NULL);
    g_test_add_func(TEST_("req_name"), test_req_name);
    g_test_add_func(TEST_("resp_name"), test_resp_name);
    g_test_add_func(TEST_("ind_name"), test_ind_name);
    g_test_add_func(TEST_("req_resp"), test_req_resp);
    g_test_add_func(TEST_("req_resp2"), test_req_resp2);
    test_init(&test_opt, argc, argv);
    return g_test_run();
}

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
