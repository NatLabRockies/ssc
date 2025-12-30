/*
BSD 3-Clause License

Copyright (c) Alliance for Sustainable Energy, LLC. See also https://github.com/NREL/ssc/blob/develop/LICENSE
All rights reserved.

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
#include <gtest/gtest.h>
#include <lib_util.h>
#include "sscapi.h"

#include "vartab.h"

TEST(libUtilTests, testFormat_lib_util)
{
	// test single input
	std::string str = "invalid number of data records (43): must be an integer multiple of 8760";
	ASSERT_EQ(util::format("invalid number of data records (%d): must be an integer multiple of 8760", 43), str);

	// test multiple inputs
	str = "query point (301.3, 10.4) is too far out of convex hull of data (dist=4.3)... estimating value from 5 parameter modele at (2.2, 2.1)=2.4";
	ASSERT_EQ(util::format("query point (%lg, %lg) is too far out of convex hull of data (dist=%lg)... estimating value from 5 parameter modele at (%lg, %lg)=%lg",
		301.3, 10.4, 4.3, 2.2, 2.1, 2.4), str);
}

TEST(libUtilTests, testNearestColValue) {
    util::matrix_t<double> cycles_vs_DOD;
    double table_vals[27] = { 20, 0, 100, 20, 5000, 80, 20, 10000, 60, 60, 0, 100, 60, 2500, 80, 60, 5000, 60, 80, 0, 100, 80, 1000, 80, 80, 2000, 60 };
    cycles_vs_DOD.assign(table_vals, 9, 3);

    ASSERT_EQ(0, util::nearest_col_index(cycles_vs_DOD, 0, 0));
    ASSERT_EQ(0, util::nearest_col_index(cycles_vs_DOD, 0, 20));
    ASSERT_EQ(2, util::nearest_col_index(cycles_vs_DOD, 0, 35));
    ASSERT_EQ(3, util::nearest_col_index(cycles_vs_DOD, 0, 40));
    ASSERT_EQ(3, util::nearest_col_index(cycles_vs_DOD, 0, 45));
    ASSERT_EQ(8, util::nearest_col_index(cycles_vs_DOD, 0, 100));
}

TEST(libUtilTests, testCharScheduleStartonMonday) {
    const char* wkday = "111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111";
    const char* wknd = "222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222";

    int tod[8760];
    int start_day = 0; // Monday
    bool result = util::translate_schedule(tod, wkday, wknd, 0, 8, start_day);
    ASSERT_TRUE(result);

    ASSERT_EQ(0, tod[0]);
    ASSERT_EQ(0, tod[24]);
    ASSERT_EQ(1, tod[24 * 6]);
}

TEST(libUtilTests, testMatrixScheduleStartonMonday) {
    ssc_number_t p_ur_ec_sched_weekday[288] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
    ssc_number_t p_ur_ec_sched_weekend[288] = { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };

    util::matrix_t<double> wkdy(12, 24, p_ur_ec_sched_weekday[0]);
    util::matrix_t<double> wknd(12, 24, p_ur_ec_sched_weekend[0]);

    int tod[8760];
    int start_day = 0; // Monday
    bool result = util::translate_schedule(tod, wkdy, wknd, 0, 8, start_day);
    ASSERT_TRUE(result);

    ASSERT_EQ(1, tod[0]);
    ASSERT_EQ(1, tod[24]);
    ASSERT_EQ(2, tod[24 * 6]);
}

TEST(libUtilTests, testCharScheduleStartonFriday) {
    const char* wkday = "111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111";
    const char* wknd = "222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222";

    int tod[8760];
    int start_day = 4; // Friday
    bool result = util::translate_schedule(tod, wkday, wknd, 0, 8, start_day);
    ASSERT_TRUE(result);

    ASSERT_EQ(0, tod[0]);
    ASSERT_EQ(1, tod[24]);
    ASSERT_EQ(0, tod[24 * 6]);
}

TEST(libUtilTests, testMatrixScheduleStartonFriday) {
    ssc_number_t p_ur_ec_sched_weekday[288] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
    ssc_number_t p_ur_ec_sched_weekend[288] = { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };

    util::matrix_t<double> wkdy(12, 24, p_ur_ec_sched_weekday[0]);
    util::matrix_t<double> wknd(12, 24, p_ur_ec_sched_weekend[0]);

    int tod[8760];
    int start_day = 4; // Friday
    bool result = util::translate_schedule(tod, wkdy, wknd, 0, 8, start_day);
    ASSERT_TRUE(result);

    ASSERT_EQ(1, tod[0]);
    ASSERT_EQ(2, tod[24]);
    ASSERT_EQ(1, tod[24 * 6]);
}

TEST(sscapiTest, SSC_DATARR_test)
{
    // create data entries
    ssc_var_t vd[2];
    for (size_t i = 0; i < 2; i++){
        vd[i] = ssc_var_create();
        ssc_var_set_number(vd[i], 2 + i);
    }

    // set using ssc_data
    auto data = ssc_data_create();
    ssc_data_set_data_array(data, "array", &vd[0], 2);

    // get using ssc_data
    int n;
    ssc_var_t data_arr = ssc_data_get_data_array(data, "array", &n);

    ssc_var_size(data_arr, &n, nullptr);
    EXPECT_EQ(n, 2);
//    ssc_var_size(data_arr, &n, nullptr);
    for (int i = 0; i < n; i++){
        double var = ssc_var_get_number(ssc_var_get_var_array(data_arr, i));
        EXPECT_EQ(var, 2 + i);
    }

    for (size_t i = 0; i < 2; i++)
        ssc_var_free(vd[i]);
    ssc_data_free(data);
}

TEST(sscapiTest, SSC_DATMAT_test)
{
    // create data entries
    ssc_var_t vd[4];
    for (size_t i = 0; i < 4; i++){
        vd[i] = ssc_var_create();
        ssc_var_set_number(vd[i], 2 + i);
    }

    // set using ssc_data
    auto data = ssc_data_create();
    ssc_data_set_data_matrix(data, "matrix", &vd[0], 2, 2);

    // get using ssc_data
    int n, m;
    ssc_var_t data_mat = ssc_data_get_data_matrix(data, "matrix", &n, &m);

    ssc_var_size(data_mat, &n, &m);
    EXPECT_EQ(n, 2);
    EXPECT_EQ(m, 2);

    for (int i = 0; i < n; i++){
        for (int j = 0; j < m; j++){
            double var = ssc_var_get_number(ssc_var_get_var_matrix(data_mat, i, j));
            EXPECT_EQ(var, 2 + i * n + j);
        }
    }

    for (size_t i = 0; i < 4; i++)
        ssc_var_free(vd[i]);
    ssc_data_free(data);
}
