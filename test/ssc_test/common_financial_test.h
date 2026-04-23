/*
BSD 3-Clause License

Copyright Alliance for Energy Innovation, LLC. See also https://github.com/NREL/ssc/blob/develop/LICENSE


Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/



#ifndef COMMON_FINANCIAL_TEST_H
#define COMMON_FINANCIAL_TEST_H

#include <gtest/gtest.h>

#include "../ssc/core.h"
#include "../ssc/vartab.h"
#include "../ssc/common_financial.h"

// ---------------------------------------------------------------------------
// Helper: a minimal concrete compute_module subclass that allows injecting a
// var_table directly without running the full SSC pipeline.
// ---------------------------------------------------------------------------
class ItcTestModule : public compute_module
{
public:
    ItcTestModule() = default;

    // Expose m_vartab so tests can set it directly.
    void set_vartab(var_table* vt) { m_vartab = vt; }

protected:
    // Pure-virtual in compute_module – must be implemented, but never called
    // directly in these unit tests.
    void exec() override {}
};

// ---------------------------------------------------------------------------
// Helper: populate a var_table with the ITC / depreciation inputs from
// 2022.08.08_develop_branch_PVWatts_Single_Owner_cmod_singleowner.json
// ---------------------------------------------------------------------------
inline void populate_itc_depr_inputs(var_table& vt)
{
    // --- ITC fixed amounts (array, 1 element = year 1 only) ----------------
    ssc_number_t itc_fed_amount[] = { 0.0f };
    ssc_number_t itc_sta_amount[] = { 0.0f };
    vt.assign("itc_fed_amount", var_data(itc_fed_amount, 1));
    vt.assign("itc_sta_amount", var_data(itc_sta_amount, 1));

    // --- ITC percent (array) -----------------------------------------------
    ssc_number_t itc_fed_percent[] = { 0.0f };
    ssc_number_t itc_sta_percent[] = { 0.0f };
    vt.assign("itc_fed_percent", var_data(itc_fed_percent, 1));
    vt.assign("itc_sta_percent", var_data(itc_sta_percent, 1));

    // --- ITC percent max values (array) ------------------------------------
    ssc_number_t itc_fed_percent_maxvalue[] = { 1e38f };
    ssc_number_t itc_sta_percent_maxvalue[] = { 1e38f };
    vt.assign("itc_fed_percent_maxvalue", var_data(itc_fed_percent_maxvalue, 1));
    vt.assign("itc_sta_percent_maxvalue", var_data(itc_sta_percent_maxvalue, 1));

    // --- ITC depreciable basis flags (fed) ---------------------------------
    vt.assign("itc_fed_percent_deprbas_fed", var_data((ssc_number_t)1));
    vt.assign("itc_fed_percent_deprbas_sta", var_data((ssc_number_t)1));
    vt.assign("itc_fed_amount_deprbas_fed", var_data((ssc_number_t)1));
    vt.assign("itc_fed_amount_deprbas_sta", var_data((ssc_number_t)1));

    // --- ITC depreciable basis flags (sta) ---------------------------------
    vt.assign("itc_sta_percent_deprbas_fed", var_data((ssc_number_t)0));
    vt.assign("itc_sta_percent_deprbas_sta", var_data((ssc_number_t)0));
    vt.assign("itc_sta_amount_deprbas_fed", var_data((ssc_number_t)0));
    vt.assign("itc_sta_amount_deprbas_sta", var_data((ssc_number_t)0));

    // --- Depreciation allocation percentages -------------------------------
    vt.assign("depr_alloc_macrs_5_percent", var_data((ssc_number_t)90.0));
    vt.assign("depr_alloc_macrs_15_percent", var_data((ssc_number_t)1.5));
    vt.assign("depr_alloc_sl_5_percent", var_data((ssc_number_t)0.0));
    vt.assign("depr_alloc_sl_15_percent", var_data((ssc_number_t)2.5));
    vt.assign("depr_alloc_sl_20_percent", var_data((ssc_number_t)3.0));
    vt.assign("depr_alloc_sl_39_percent", var_data((ssc_number_t)0.0));
    vt.assign("depr_alloc_custom_percent", var_data((ssc_number_t)0.0));

    // Custom depreciation schedule (1 element placeholder) -----------------
    ssc_number_t depr_custom_schedule[] = { 0.0f };
    vt.assign("depr_custom_schedule", var_data(depr_custom_schedule, 1));

    // --- Depreciation basis method (1 = proportional allocation) ----------
    vt.assign("depr_stabas_method", var_data((ssc_number_t)1));
    vt.assign("depr_fedbas_method", var_data((ssc_number_t)1));

    // --- ITC qualification flags – state ----------------------------------
    vt.assign("depr_itc_sta_macrs_5", var_data((ssc_number_t)1));
    vt.assign("depr_itc_sta_macrs_15", var_data((ssc_number_t)0));
    vt.assign("depr_itc_sta_sl_5", var_data((ssc_number_t)0));
    vt.assign("depr_itc_sta_sl_15", var_data((ssc_number_t)0));
    vt.assign("depr_itc_sta_sl_20", var_data((ssc_number_t)0));
    vt.assign("depr_itc_sta_sl_39", var_data((ssc_number_t)0));
    vt.assign("depr_itc_sta_custom", var_data((ssc_number_t)0));

    // --- ITC qualification flags – federal --------------------------------
    vt.assign("depr_itc_fed_macrs_5", var_data((ssc_number_t)1));
    vt.assign("depr_itc_fed_macrs_15", var_data((ssc_number_t)0));
    vt.assign("depr_itc_fed_sl_5", var_data((ssc_number_t)0));
    vt.assign("depr_itc_fed_sl_15", var_data((ssc_number_t)0));
    vt.assign("depr_itc_fed_sl_20", var_data((ssc_number_t)0));
    vt.assign("depr_itc_fed_sl_39", var_data((ssc_number_t)0));
    vt.assign("depr_itc_fed_custom", var_data((ssc_number_t)0));

    // --- Bonus depreciation – state ---------------------------------------
    vt.assign("depr_bonus_sta", var_data((ssc_number_t)0.0));
    vt.assign("depr_bonus_sta_macrs_5", var_data((ssc_number_t)1));
    vt.assign("depr_bonus_sta_macrs_15", var_data((ssc_number_t)1));
    vt.assign("depr_bonus_sta_sl_5", var_data((ssc_number_t)0));
    vt.assign("depr_bonus_sta_sl_15", var_data((ssc_number_t)0));
    vt.assign("depr_bonus_sta_sl_20", var_data((ssc_number_t)0));
    vt.assign("depr_bonus_sta_sl_39", var_data((ssc_number_t)0));
    vt.assign("depr_bonus_sta_custom", var_data((ssc_number_t)0));

    // --- Bonus depreciation – federal -------------------------------------
    vt.assign("depr_bonus_fed", var_data((ssc_number_t)0.0));
    vt.assign("depr_bonus_fed_macrs_5", var_data((ssc_number_t)1));
    vt.assign("depr_bonus_fed_macrs_15", var_data((ssc_number_t)1));
    vt.assign("depr_bonus_fed_sl_5", var_data((ssc_number_t)0));
    vt.assign("depr_bonus_fed_sl_15", var_data((ssc_number_t)0));
    vt.assign("depr_bonus_fed_sl_20", var_data((ssc_number_t)0));
    vt.assign("depr_bonus_fed_sl_39", var_data((ssc_number_t)0));
    vt.assign("depr_bonus_fed_custom", var_data((ssc_number_t)0));
}

class common_financial_test : public ::testing::Test {

public:

    var_table       vt;
    ItcTestModule   cm;

    void SetUp() override {
        populate_itc_depr_inputs(vt);
        cm.set_vartab(&vt);
    }

    void TearDown() override {
    }

};


#endif //COMMON_FINANCIAL_TEST_H
