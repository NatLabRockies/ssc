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


#include <string>
#include <cmath>
#include <gtest/gtest.h>

#include "common_financial_test.h"

TEST_F(common_financial_test, test_itc_and_depreciation_calculations_no_itc) {

    itc_and_depreciation_calculations itc_depr(&cm, 25);
    itc_depr.depr_fed_reduction = 0;
    itc_depr.depr_sta_reduction = 0;

    // Test reallocation to depreciable basis
    EXPECT_NEAR(itc_depr.depr_alloc_macrs_5_frac
        + itc_depr.depr_alloc_macrs_15_frac
        + itc_depr.depr_alloc_sl_5_frac
        + itc_depr.depr_alloc_sl_15_frac
        + itc_depr.depr_alloc_sl_20_frac
        + itc_depr.depr_alloc_sl_39_frac
        + itc_depr.depr_alloc_custom_frac,
        1.0, 1e-9);

    // Example installed cost from PVWatts Single Owner scenario: $1,000,000
    const double installed_cost = 1'000'000.0;
    itc_depr.calc_basis(installed_cost, installed_cost);

    // Total depreciable allocation must be 97 % of installed cost
    // (100 % - 3 % not depreciable since depr_alloc_total_frac = 0.97).
    EXPECT_NEAR(itc_depr.depr_alloc_total, 0.97 * installed_cost, 1.0);

    // With zero ITC percent and zero fixed ITC, both ITC totals must be zero.
    EXPECT_DOUBLE_EQ(itc_depr.itc_sta_per, 0.0);
    EXPECT_DOUBLE_EQ(itc_depr.itc_fed_per, 0.0);
    EXPECT_DOUBLE_EQ(itc_depr.itc_fed_qual_total, installed_cost * 0.9);
    EXPECT_DOUBLE_EQ(itc_depr.itc_disallow_fed_percent_macrs_5, 0.0);
    EXPECT_DOUBLE_EQ(itc_depr.get_itc_fed(1), 0.0);
}

TEST_F(common_financial_test, test_itc_and_depreciation_calculations_w_itc) {

    ssc_number_t itc_fed_percent[] = { 30.0f };
    cm.assign("itc_fed_percent", var_data(itc_fed_percent, 1));

    itc_and_depreciation_calculations itc_depr(&cm, 25);
    itc_depr.depr_fed_reduction = 0;
    itc_depr.depr_sta_reduction = 0;

    // Test reallocation to depreciable basis
    EXPECT_NEAR(itc_depr.depr_alloc_macrs_5_frac
        + itc_depr.depr_alloc_macrs_15_frac
        + itc_depr.depr_alloc_sl_5_frac
        + itc_depr.depr_alloc_sl_15_frac
        + itc_depr.depr_alloc_sl_20_frac
        + itc_depr.depr_alloc_sl_39_frac
        + itc_depr.depr_alloc_custom_frac,
        1.0, 1e-9);

    // Example installed cost from PVWatts Single Owner scenario: $1,000,000
    const double installed_cost = 1'000'000.0;
    itc_depr.calc_basis(installed_cost, installed_cost);

    // Total depreciable allocation must be 97 % of installed cost
    // (100 % - 3 % not depreciable since depr_alloc_total_frac = 0.97).
    EXPECT_NEAR(itc_depr.depr_alloc_total, 0.97 * installed_cost, 1.0);

    EXPECT_DOUBLE_EQ(itc_depr.itc_sta_per, 0.0);
    EXPECT_DOUBLE_EQ(itc_depr.itc_fed_per, installed_cost * 0.3 * 0.9);
    EXPECT_DOUBLE_EQ(itc_depr.itc_fed_qual_total,installed_cost * 0.9);
    EXPECT_DOUBLE_EQ(itc_depr.itc_disallow_fed_percent_macrs_5, installed_cost * 0.3 * 0.9 * 0.5);
    EXPECT_DOUBLE_EQ(itc_depr.get_itc_fed(1), installed_cost * 0.3 * 0.9);
}

TEST_F(common_financial_test, test_calculated_matrix) {

    //col 0=tech no, col 1=basis amount ($), col 2=% of total installed cost, col 3=fed itc qual 0/1, col 4=state itc qual 0/1, col 5=depreciation selection(0=macrs 5, 1=macrs 15, 2=sl 5, 3=sl 15, 4=sl 20, 5=sl 39, 6=custom)
    ssc_number_t p_depr_basis_mat[24] = { 1, 1000000, 25, 1, 1, 0,
    2, 1000000, 25, 1, 1, 0,
    3, 1000000, 25, 0, 0, 2,
    4, 1000000, 25, 0, 0, 2 };
    cm.assign("depr_basis_mat", var_data(p_depr_basis_mat, 4, 6));
    cm.assign("depr_en_basis_mat", var_data(1));

    ssc_number_t itc_fed_percent[] = { 30.0f };
    cm.assign("itc_fed_percent", var_data(itc_fed_percent, 1));

    itc_and_depreciation_calculations itc_depr(&cm, 25);
    itc_depr.depr_fed_reduction = 0;
    itc_depr.depr_sta_reduction = 0;

    // Test reallocation to depreciable basis
    EXPECT_NEAR(itc_depr.depr_alloc_macrs_5_frac
        + itc_depr.depr_alloc_macrs_15_frac
        + itc_depr.depr_alloc_sl_5_frac
        + itc_depr.depr_alloc_sl_15_frac
        + itc_depr.depr_alloc_sl_20_frac
        + itc_depr.depr_alloc_sl_39_frac
        + itc_depr.depr_alloc_custom_frac,
        1.0, 1e-9);

    // Example installed cost from PVWatts Single Owner scenario: $4,000,000
    const double installed_cost = 4'000'000.0;
    itc_depr.calc_basis(installed_cost, installed_cost);

    // Fully allocated the assets for the text matrix
    EXPECT_NEAR(itc_depr.depr_alloc_total, installed_cost, 1.0);

    EXPECT_DOUBLE_EQ(itc_depr.itc_sta_per, 0.0);
    EXPECT_DOUBLE_EQ(itc_depr.itc_fed_per, installed_cost * 0.3 * 0.5);
    EXPECT_DOUBLE_EQ(itc_depr.itc_fed_qual_total, installed_cost * 0.5);
    EXPECT_DOUBLE_EQ(itc_depr.itc_disallow_fed_percent_macrs_5, installed_cost * 0.3 * 0.5 * 0.5);
    EXPECT_DOUBLE_EQ(itc_depr.get_itc_fed(1), installed_cost * 0.3 * 0.5);
}

TEST_F(common_financial_test, test_calculated_matrix_all_macrs) {

    //col 0=tech no, col 1=basis amount ($), col 2=% of total installed cost, col 3=fed itc qual 0/1, col 4=state itc qual 0/1, col 5=depreciation selection(0=macrs 5, 1=macrs 15, 2=sl 5, 3=sl 15, 4=sl 20, 5=sl 39, 6=custom)
    ssc_number_t p_depr_basis_mat[24] = { 1, 1000000, 50, 1, 1, 0,
    2, 1000000, 50, 0, 0, 0};
    cm.assign("depr_basis_mat", var_data(p_depr_basis_mat, 2, 6));
    cm.assign("depr_en_basis_mat", var_data(1));

    ssc_number_t itc_fed_percent[] = { 30.0f };
    cm.assign("itc_fed_percent", var_data(itc_fed_percent, 1));

    itc_and_depreciation_calculations itc_depr(&cm, 25);
    itc_depr.depr_fed_reduction = 0;
    itc_depr.depr_sta_reduction = 0;

    // Test reallocation to depreciable basis
    EXPECT_NEAR(itc_depr.depr_alloc_macrs_5_frac
        + itc_depr.depr_alloc_macrs_15_frac
        + itc_depr.depr_alloc_sl_5_frac
        + itc_depr.depr_alloc_sl_15_frac
        + itc_depr.depr_alloc_sl_20_frac
        + itc_depr.depr_alloc_sl_39_frac
        + itc_depr.depr_alloc_custom_frac,
        1.0, 1e-9);

    // Example installed cost from PVWatts Single Owner scenario: $4,000,000
    const double installed_cost = 2'000'000.0;
    itc_depr.calc_basis(installed_cost, installed_cost);

    // Fully allocated the assets for the text matrix
    EXPECT_NEAR(itc_depr.depr_alloc_total, installed_cost, 1.0);

    EXPECT_DOUBLE_EQ(itc_depr.itc_sta_per, 0.0);
    EXPECT_DOUBLE_EQ(itc_depr.itc_fed_per, installed_cost * 0.3 * 0.5);
    EXPECT_DOUBLE_EQ(itc_depr.itc_fed_qual_total, installed_cost * 0.5);
    EXPECT_DOUBLE_EQ(itc_depr.itc_disallow_fed_percent_macrs_5, installed_cost * 0.3 * 0.5 * 0.5);
    EXPECT_DOUBLE_EQ(itc_depr.get_itc_fed(1), installed_cost * 0.3 * 0.5);
}
