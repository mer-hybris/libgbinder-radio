/*
 * Copyright (C) 2021 Jolla Ltd.
 * Copyright (C) 2021 Slava Monich <slava.monich@jolla.com>
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
}

/*==========================================================================*
 * req_resp
 *==========================================================================*/

static
void
test_req_resp(
    void)
{
    g_assert_cmpint(radio_req_resp(UNKNOWN_REQ), == ,RADIO_RESP_NONE);
    g_assert_cmpint(radio_req_resp(RADIO_REQ_ANY), == ,RADIO_RESP_NONE);
    g_assert_cmpint(radio_req_resp(RADIO_REQ_GET_ICC_CARD_STATUS),==,
        RADIO_RESP_GET_ICC_CARD_STATUS);
    g_assert_cmpint(radio_req_resp(RADIO_REQ_START_NETWORK_SCAN),==,
        RADIO_RESP_START_NETWORK_SCAN);
    g_assert_cmpint(radio_req_resp(RADIO_REQ_SET_SIGNAL_STRENGTH_REPORTING_CRITERIA),==,
        RADIO_RESP_SET_SIGNAL_STRENGTH_REPORTING_CRITERIA);
    g_assert_cmpint(radio_req_resp(RADIO_REQ_SET_SYSTEM_SELECTION_CHANNELS),==,
        RADIO_RESP_SET_SYSTEM_SELECTION_CHANNELS);
    g_assert_cmpint(radio_req_resp(RADIO_REQ_EMERGENCY_DIAL),==,
        RADIO_RESP_EMERGENCY_DIAL);
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
