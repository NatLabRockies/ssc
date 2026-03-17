/*
BSD 3-Clause License

Copyright (c) Alliance for Energy Innovation, LLC. See also https://github.com/NREL/ssc/blob/develop/LICENSE
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


#include <gtest/gtest.h>
#include <lib_utility_rate_equations.h>

TEST(lib_utility_rate_equations_test, test_copy)
{
	ssc_number_t p_ur_ec_sched_weekday[288] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 4, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 4, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 4, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 4, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 4, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 4, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 1, 1, 1, 1 };
	ssc_number_t p_ur_ec_sched_weekend[288] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
    ssc_number_t p_ur_ec_tou_mat[24] = { 1, 1, 9.9999999999999998e+37, 0, 0.10000000000000001, 0,
        2, 1, 9.9999999999999998e+37, 0, 0.050000000000000003, 0,
        3, 1, 9.9999999999999998e+37, 0, 0.20000000000000001, 0,
        4, 1, 9.9999999999999998e+37, 0, 0.25, 0 };
	size_t tou_rows = 4;
	bool sell_eq_buy = false;
    size_t start_day_of_year = 0;

	rate_data data;
	data.init(8760);
	data.setup_energy_rates(&p_ur_ec_sched_weekday[0], &p_ur_ec_sched_weekend[0], tou_rows, &p_ur_ec_tou_mat[0], sell_eq_buy, start_day_of_year);

	rate_data data2(data);

	data.m_month[0].update_net_and_peak(-1.0, -1.0, 0);

	EXPECT_NEAR(data.m_month[0].energy_net, -1.0, 0.01);
	EXPECT_NEAR(data.m_month[0].dc_flat_peak, 1.0, 0.01);

	EXPECT_NEAR(data2.m_month[0].energy_net, 0.0, 0.01);
	EXPECT_NEAR(data2.m_month[0].dc_flat_peak, 0.0, 0.01);
}

TEST(lib_utility_rate_equations_test, test_demand_charges)
{
    ssc_number_t p_ur_ec_sched_weekday[288] = { 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 4, 4, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 4, 4, 4, 4 };
    ssc_number_t p_ur_ec_sched_weekend[288] = { 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 };
    ssc_number_t p_ur_ec_tou_mat[24] = { 1, 1, 9.9999999999999998e+37, 0, 0.050000000000000003, 0, 
        2, 1, 9.9999999999999998e+37, 0, 0.074999999999999997, 0, 
        3, 1, 9.9999999999999998e+37, 0, 0.059999999999999998, 0, 
        4, 1, 9.9999999999999998e+37, 0, 0.050000000000000003, 0 };
    size_t tou_rows = 4;
    bool sell_eq_buy = false;

    ssc_number_t p_ur_dc_sched_weekday[288] = { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2 };
    ssc_number_t p_ur_dc_sched_weekend[288] = { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };
    ssc_number_t p_ur_dc_tou_mat[16] = { 1, 1, 100, 20, 
                                         1, 2, 9.9999999999999998e+37, 15, 
                                         2, 1, 100, 10, 
                                         2, 2, 9.9999999999999998e+37, 5 };
    ssc_number_t p_ur_dc_flat_mat[48] = { 0, 1, 9.9999999999999998e+37, 0, 
                                        1, 1, 9.9999999999999998e+37, 0, 
                                        2, 1, 9.9999999999999998e+37, 0, 
                                        3, 1, 9.9999999999999998e+37, 0, 
                                        4, 1, 9.9999999999999998e+37, 0, 
                                        5, 1, 9.9999999999999998e+37, 0, 
                                        6, 1, 9.9999999999999998e+37, 0, 
                                        7, 1, 9.9999999999999998e+37, 0, 
                                        8, 1, 9.9999999999999998e+37, 0, 
                                        9, 1, 9.9999999999999998e+37, 0, 
                                        10, 1, 9.9999999999999998e+37, 0, 
                                        11, 1, 9.9999999999999998e+37, 0 };
    size_t dc_flat_rows = 12;
    size_t start_day_of_year = 0;

    rate_data data;
    data.m_num_rec_yearly = 8760;
    data.rate_scale = { 1 };
    data.init(8760);
    data.setup_demand_charges(&p_ur_dc_sched_weekday[0], &p_ur_dc_sched_weekend[0], tou_rows, &p_ur_dc_tou_mat[0], dc_flat_rows, &p_ur_dc_flat_mat[0], start_day_of_year);
    data.setup_energy_rates(&p_ur_ec_sched_weekday[0], &p_ur_ec_sched_weekend[0], tou_rows, &p_ur_ec_tou_mat[0], sell_eq_buy, start_day_of_year);
    data.init_energy_rates_all_months(false);
    data.init_dc_peak_vectors(0);

    // Peak period 1: 5 kW, peak period 2: 10 kW
    std::vector<double> day_one_power = { -1, -1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -9, -8, -7, -6, -5, -4, -3, -2, -1, -1, -1, -1, -1 };

    ASSERT_EQ(24, day_one_power.size());
    int step = 0;
    int month = 0;
    for (step = 0; step < day_one_power.size(); step++)
    {
        // Hourly, so power and energy are the same number
        data.sort_energy_to_periods(month, day_one_power.at(step), step);
        data.find_dc_tou_peak(month, day_one_power.at(step), step);
    }

    ASSERT_NEAR(200.0, data.get_demand_charge(0, 0), 0.1);

    // Check whether or not it's safe to call this function twice on the same data
    ASSERT_NEAR(200.0, data.get_demand_charge(0, 0), 0.1);
}

TEST(lib_utility_rate_equations_test, test_seasonal_demand_charges)
{
    ssc_number_t p_ur_ec_sched_weekday[288] = { 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 4, 4, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 4, 4, 4, 4 };
    ssc_number_t p_ur_ec_sched_weekend[288] = { 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 };
    ssc_number_t p_ur_ec_tou_mat[24] = { 1, 1, 9.9999999999999998e+37, 0, 0.050000000000000003, 0,
        2, 1, 9.9999999999999998e+37, 0, 0.074999999999999997, 0,
        3, 1, 9.9999999999999998e+37, 0, 0.059999999999999998, 0,
        4, 1, 9.9999999999999998e+37, 0, 0.050000000000000003, 0 };
    size_t tou_rows = 4;
    bool sell_eq_buy = false;

    ssc_number_t p_ur_dc_sched_weekday[288] = { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2 };
    ssc_number_t p_ur_dc_sched_weekend[288] = { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };
    ssc_number_t p_ur_dc_tou_mat[16] = { 1, 1, 100, 0,
                                         1, 2, 9.9999999999999998e+37, 0,
                                         2, 1, 100, 0,
                                         2, 2, 9.9999999999999998e+37, 0 };
    size_t dc_tou_rows = 4;
    // Demand charges are only active June through September
    ssc_number_t p_ur_dc_flat_mat[48] = { 0, 1, 9.9999999999999998e+37, 0,
                                        1, 1, 9.9999999999999998e+37, 0,
                                        2, 1, 9.9999999999999998e+37, 0,
                                        3, 1, 9.9999999999999998e+37, 0,
                                        4, 1, 9.9999999999999998e+37, 0,
                                        5, 1, 9.9999999999999998e+37, 1,
                                        6, 1, 9.9999999999999998e+37, 1,
                                        7, 1, 9.9999999999999998e+37, 1,
                                        8, 1, 9.9999999999999998e+37, 1,
                                        9, 1, 9.9999999999999998e+37, 0,
                                        10, 1, 9.9999999999999998e+37, 0,
                                        11, 1, 9.9999999999999998e+37, 0 };
    size_t dc_flat_rows = 12;

    ssc_number_t prev_monthly_peaks[12] = { 5, 5, 5, 5, 5, 5,
                                    5, 5, 5, 5, 5, 5 };
    size_t start_day_of_year = 0;

    rate_data data;
    data.m_num_rec_yearly = 8760;
    data.rate_scale = { 1 };
    data.init(8760);
    data.setup_demand_charges(&p_ur_dc_sched_weekday[0], &p_ur_dc_sched_weekend[0], dc_tou_rows, &p_ur_dc_tou_mat[0], dc_flat_rows, &p_ur_dc_flat_mat[0], start_day_of_year);
    data.setup_energy_rates(&p_ur_ec_sched_weekday[0], &p_ur_ec_sched_weekend[0], tou_rows, &p_ur_ec_tou_mat[0], sell_eq_buy, start_day_of_year);
    data.init_energy_rates_all_months(false);
    data.uses_billing_demand = true;
    data.en_billing_demand_lookback = false;
    data.init_dc_peak_vectors(0);

    // Peak period 1: 5 kW, peak period 2: 10 kW
    std::vector<double> day_one_power = { -1, -1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -9, -8, -7, -6, -5, -4, -3, -2, -1, -1, -1, -1, -1 };

    ASSERT_EQ(24, day_one_power.size());
    int base_step = 0;
    int step = 0;
    int month = 0;
    ur_month& curr_month = data.m_month[month];
    for (step = 0; step < day_one_power.size(); step++)
    {
        double power = day_one_power.at(step);
        // Hourly, so power and energy are the same number
        curr_month.update_net_and_peak(power, power, step + base_step);
        data.sort_energy_to_periods(month, power, step + base_step);
        data.find_dc_tou_peak(month, power, step + base_step);
    }

    data.init_dc_peak_vectors(5);
    base_step = 3648; // 12 am June 1st
    month = 5;
    ur_month& month_5 = data.m_month[month];
    for (step = 0; step < day_one_power.size(); step++)
    {
        double power = day_one_power.at(step);
        // Hourly, so power and energy are the same number
        month_5.update_net_and_peak(power, power, step + base_step);
        data.sort_energy_to_periods(month, power, step + base_step);
        data.find_dc_tou_peak(month, power, step + base_step);
    }

    if (data.uses_billing_demand) {
        if (data.en_billing_demand_lookback) {
            data.setup_prev_demand(prev_monthly_peaks);
        }
        for (int m = 0; m < (int)data.m_month.size(); m++) {
            double flat_peak = data.m_month[m].dc_flat_peak;
            if (data.en_billing_demand_lookback) {
                // If ratchets are present the peak used here might be the actual peak, or something based on a previous month.
                flat_peak = data.get_billing_demand(m);
            }
            data.billing_demand[m] = flat_peak;
        }
    }

    // No demand charges in January
    ASSERT_NEAR(0.0, data.get_demand_charge(0, 0), 0.1);

    // Same profile provides demand charges in June
    ASSERT_NEAR(10.0, data.get_demand_charge(5, 0), 0.1);
}

// Excel version of this test: https://github.com/NREL/SAM-documentation/blob/master/Unit%20Testing/Utility%20Rates/block_step/GPC_PLL_Tiered_Bill_Calc_Example_v3_btm_tests.xlsx
TEST(lib_utility_rate_equations_test, test_block_step_tiers)
{
    ssc_number_t p_ur_ec_sched_weekday[288] = { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };
    ssc_number_t p_ur_ec_sched_weekend[288] = { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };
    ssc_number_t p_ur_ec_tou_mat[96] = { 1, 1, 200, 1, 0.031718000000000003, 0,
                                         1, 2, 3000, 0, 0.132655, 0,
                                         1, 3, 10000, 0, 0.120303, 0,
                                         1, 4, 200000, 0, 0.102607, 0,
                                         1, 5, 9.9999999999999998e+37, 0, 0.079109, 0,
                                         1, 6, 400, 1, 0.013627, 0,
                                         1, 7, 600, 1, 0.010275, 0,
                                         1, 8, 9.9999999999999998e+37, 1, 0.00771, 0,
                                         2, 1, 200, 1, 0.028812999999999998, 0,
                                         2, 2, 3000, 0, 0.132655, 0,
                                         2, 3, 10000, 0, 0.120303, 0,
                                         2, 4, 200000, 0, 0.102607, 0,
                                         2, 5, 9.9999999999999998e+37, 0, 0.079109, 0,
                                         2, 6, 400, 1, 0.013627, 0,
                                         2, 7, 600, 1, 0.010275, 0,
                                         2, 8, 9.9999999999999998e+37, 1, 0.00771, 0 };
    size_t tou_rows = 16;
    bool sell_eq_buy = false;

    ssc_number_t p_ur_dc_sched_weekday[288] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
    ssc_number_t p_ur_dc_sched_weekend[288] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
    ssc_number_t p_ur_dc_tou_mat[4] = { 1, 1, 9.9999999999999998e+37, 0 };
    size_t dc_tou_rows = 1;

    ssc_number_t p_ur_dc_flat_mat[48] = { 0, 1, 9.9999999999999998e+37, 0,
                                          1, 1, 9.9999999999999998e+37, 0,
                                          2, 1, 9.9999999999999998e+37, 0,
                                          3, 1, 9.9999999999999998e+37, 0,
                                          4, 1, 9.9999999999999998e+37, 0,
                                          5, 1, 9.9999999999999998e+37, 0,
                                          6, 1, 9.9999999999999998e+37, 0,
                                          7, 1, 9.9999999999999998e+37, 0,
                                          8, 1, 9.9999999999999998e+37, 0,
                                          9, 1, 9.9999999999999998e+37, 0,
                                          10, 1, 9.9999999999999998e+37, 0,
                                          11, 1, 9.9999999999999998e+37, 0 };

    size_t dc_flat_rows = 12;
    size_t start_day_of_year = 0;

    ssc_number_t prev_monthly_peaks[12] = { 500, 500, 500, 500, 500, 500,
                                        500, 500, 500, 500, 500, 500 };


    rate_data data;
    data.m_num_rec_yearly = 8760;
    data.rate_scale = { 1 };
    data.init(8760);
    data.setup_demand_charges(&p_ur_dc_sched_weekday[0], &p_ur_dc_sched_weekend[0], dc_tou_rows, &p_ur_dc_tou_mat[0], dc_flat_rows, &p_ur_dc_flat_mat[0], start_day_of_year);
    data.setup_energy_rates(&p_ur_ec_sched_weekday[0], &p_ur_ec_sched_weekend[0], tou_rows, &p_ur_ec_tou_mat[0], sell_eq_buy, start_day_of_year);
    data.init_energy_rates_all_months(false); // This gets called once to set up all the vectors
    data.init_dc_peak_vectors(0);
    data.uses_billing_demand = true;
    data.en_billing_demand_lookback = false;

    // Only really need one power number to set up the peak, but include a couple extras as a test
    std::vector<double> day_one_power = { -500, -600, -658, };

    int base_step = 0;
    int step = 0;
    int month = 0;
    ur_month& curr_month = data.m_month[month];
    for (step = 0; step < day_one_power.size(); step++)
    {
        double power = day_one_power.at(step);
        // Hourly, so power and energy are the same number
        curr_month.update_net_and_peak(power, power, step + base_step);
    }

    day_one_power = { -500, -1413, -1000, };
    data.init_dc_peak_vectors(5);
    base_step = 3648; // 12 am June 1st
    month = 5;
    ur_month& month_5 = data.m_month[month];
    for (step = 0; step < day_one_power.size(); step++)
    {
        double power = day_one_power.at(step);
        // Hourly, so power and energy are the same number
        month_5.update_net_and_peak(power, power, step + base_step);
    }

    if (data.uses_billing_demand) {
        if (data.en_billing_demand_lookback) {
            data.setup_prev_demand(prev_monthly_peaks);
        }
        for (int m = 0; m < (int)data.m_month.size(); m++) {
            double flat_peak = data.m_month[m].dc_flat_peak;
            if (data.en_billing_demand_lookback) {
                // If ratchets are present the peak used here might be the actual peak, or something based on a previous month.
                flat_peak = data.get_billing_demand(m);
            }
            data.billing_demand[m] = flat_peak;
        }
    }

    // Recompute the tiers based on actual peaks
    data.init_energy_rates_all_months(false);

    // Each month is only going to have one TOU period in this schedule
    EXPECT_NEAR(3000, curr_month.ec_tou_ub.at(0, 0), 0.1);
    EXPECT_NEAR(10000, curr_month.ec_tou_ub.at(0, 1), 0.1);
    EXPECT_NEAR(131600, curr_month.ec_tou_ub.at(0, 2), 0.1);
    EXPECT_NEAR(263200, curr_month.ec_tou_ub.at(0, 3), 0.1);
    EXPECT_NEAR(394800, curr_month.ec_tou_ub.at(0, 4), 0.1);
    EXPECT_NEAR(9.9999999999999998e+37, curr_month.ec_tou_ub.at(0, 5), 0.1);

    EXPECT_NEAR(0.132655, curr_month.ec_tou_br.at(0, 0), 0.0001);
    EXPECT_NEAR(0.120303, curr_month.ec_tou_br.at(0, 1), 0.0001);
    EXPECT_NEAR(0.102607, curr_month.ec_tou_br.at(0, 2), 0.0001);
    EXPECT_NEAR(0.013627, curr_month.ec_tou_br.at(0, 3), 0.0001);
    EXPECT_NEAR(0.010275, curr_month.ec_tou_br.at(0, 4), 0.0001);
    EXPECT_NEAR(0.00771, curr_month.ec_tou_br.at(0, 5), 0.0001);

    EXPECT_NEAR(3000, month_5.ec_tou_ub.at(0, 0), 0.1);
    EXPECT_NEAR(10000, month_5.ec_tou_ub.at(0, 1), 0.1);
    EXPECT_NEAR(200000, month_5.ec_tou_ub.at(0, 2), 0.1);
    EXPECT_NEAR(282600, month_5.ec_tou_ub.at(0, 3), 0.1);
    EXPECT_NEAR(565200, month_5.ec_tou_ub.at(0, 4), 0.1);
    EXPECT_NEAR(847800, month_5.ec_tou_ub.at(0, 5), 0.1);
    EXPECT_NEAR(9.9999999999999998e+37, month_5.ec_tou_ub.at(0, 6), 0.1);

    EXPECT_NEAR(0.132655, month_5.ec_tou_br.at(0, 0), 0.0001);
    EXPECT_NEAR(0.120303, month_5.ec_tou_br.at(0, 1), 0.0001);
    EXPECT_NEAR(0.102607, month_5.ec_tou_br.at(0, 2), 0.0001);
    EXPECT_NEAR(0.079109, month_5.ec_tou_br.at(0, 3), 0.0001);
    EXPECT_NEAR(0.013627, month_5.ec_tou_br.at(0, 4), 0.0001);
    EXPECT_NEAR(0.010275, month_5.ec_tou_br.at(0, 5), 0.0001);
    EXPECT_NEAR(0.00771, month_5.ec_tou_br.at(0, 6), 0.0001);
}

TEST(lib_utility_rate_equations_test, test_kwh_per_kw_only)
{
    ssc_number_t p_ur_ec_sched_weekday[288] = { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };
    ssc_number_t p_ur_ec_sched_weekend[288] = { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };
    ssc_number_t p_ur_ec_tou_mat[96] = { 1, 1, 200, 1, 0.031718000000000003, 0,
                                         1, 6, 400, 1, 0.013627, 0,
                                         1, 7, 600, 1, 0.010275, 0,
                                         1, 8, 9.9999999999999998e+37, 1, 0.00771, 0,
                                         2, 1, 200, 1, 0.028812999999999998, 0,
                                         2, 6, 400, 1, 0.013627, 0,
                                         2, 7, 600, 1, 0.010275, 0,
                                         2, 8, 9.9999999999999998e+37, 1, 0.00771, 0 };
    size_t tou_rows = 16;
    bool sell_eq_buy = false;

    ssc_number_t p_ur_dc_sched_weekday[288] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
    ssc_number_t p_ur_dc_sched_weekend[288] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
    ssc_number_t p_ur_dc_tou_mat[4] = { 1, 1, 9.9999999999999998e+37, 0 };
    size_t dc_tou_rows = 1;

    ssc_number_t p_ur_dc_flat_mat[48] = { 0, 1, 9.9999999999999998e+37, 0,
                                          1, 1, 9.9999999999999998e+37, 0,
                                          2, 1, 9.9999999999999998e+37, 0,
                                          3, 1, 9.9999999999999998e+37, 0,
                                          4, 1, 9.9999999999999998e+37, 0,
                                          5, 1, 9.9999999999999998e+37, 0,
                                          6, 1, 9.9999999999999998e+37, 0,
                                          7, 1, 9.9999999999999998e+37, 0,
                                          8, 1, 9.9999999999999998e+37, 0,
                                          9, 1, 9.9999999999999998e+37, 0,
                                          10, 1, 9.9999999999999998e+37, 0,
                                          11, 1, 9.9999999999999998e+37, 0 };

    size_t dc_flat_rows = 12;

    ssc_number_t prev_monthly_peaks[12] = { -500, -500, -500, -500, -500, -500,
                                            -500, -500, -500, -500, -500, -500 };

    size_t start_day_of_year = 0;

    rate_data data;
    data.m_num_rec_yearly = 8760;
    data.rate_scale = { 1 };
    data.init(8760);
    data.setup_demand_charges(&p_ur_dc_sched_weekday[0], &p_ur_dc_sched_weekend[0], dc_tou_rows, &p_ur_dc_tou_mat[0], dc_flat_rows, &p_ur_dc_flat_mat[0], start_day_of_year);
    data.setup_energy_rates(&p_ur_ec_sched_weekday[0], &p_ur_ec_sched_weekend[0], tou_rows, &p_ur_ec_tou_mat[0], sell_eq_buy, start_day_of_year);
    data.init_energy_rates_all_months(false); // This gets called once to set up all the vectors
    data.init_dc_peak_vectors(0);
    data.uses_billing_demand = true;
    data.en_billing_demand_lookback = false;

    // Only really need one power number to set up the peak, but include a couple extras as a test
    std::vector<double> day_one_power = { -500, -600, -658, };

    int base_step = 0;
    int step = 0;
    int month = 0;
    ur_month& curr_month = data.m_month[month];
    for (step = 0; step < day_one_power.size(); step++)
    {
        double power = day_one_power.at(step);
        // Hourly, so power and energy are the same number
        curr_month.update_net_and_peak(power, power, step + base_step);
    }

    day_one_power = { -500, -1413, -1000, };
    data.init_dc_peak_vectors(5);
    base_step = 3648; // 12 am June 1st
    month = 5;
    ur_month& month_5 = data.m_month[month];
    for (step = 0; step < day_one_power.size(); step++)
    {
        double power = day_one_power.at(step);
        // Hourly, so power and energy are the same number
        month_5.update_net_and_peak(power, power, step + base_step);
    }

    if (data.uses_billing_demand) {
        if (data.en_billing_demand_lookback) {
            data.setup_prev_demand(prev_monthly_peaks);
        }
        for (int m = 0; m < (int)data.m_month.size(); m++) {
            double flat_peak = data.m_month[m].dc_flat_peak;
            if (data.en_billing_demand_lookback) {
                // If ratchets are present the peak used here might be the actual peak, or something based on a previous month.
                flat_peak = data.get_billing_demand(m);
            }
            data.billing_demand[m] = flat_peak;
        }
    }

    // Recompute the tiers based on actual peaks
    data.init_energy_rates_all_months(false);

    // Each month is only going to have one TOU period in this schedule
    EXPECT_NEAR(131600, curr_month.ec_tou_ub.at(0, 0), 0.1);
    EXPECT_NEAR(263200, curr_month.ec_tou_ub.at(0, 1), 0.1);
    EXPECT_NEAR(394800, curr_month.ec_tou_ub.at(0, 2), 0.1);
    EXPECT_NEAR(9.9999999999999998e+37, curr_month.ec_tou_ub.at(0, 3), 0.1);

    EXPECT_NEAR(0.02881, curr_month.ec_tou_br.at(0, 0), 0.0001);
    EXPECT_NEAR(0.013627, curr_month.ec_tou_br.at(0, 1), 0.0001);
    EXPECT_NEAR(0.010275, curr_month.ec_tou_br.at(0, 2), 0.0001);
    EXPECT_NEAR(0.00771, curr_month.ec_tou_br.at(0, 3), 0.0001);

    EXPECT_NEAR(282600, month_5.ec_tou_ub.at(0, 0), 0.1);
    EXPECT_NEAR(565200, month_5.ec_tou_ub.at(0, 1), 0.1);
    EXPECT_NEAR(847800, month_5.ec_tou_ub.at(0, 2), 0.1);
    EXPECT_NEAR(9.9999999999999998e+37, curr_month.ec_tou_ub.at(0, 3), 0.1);

    EXPECT_NEAR(0.031718, month_5.ec_tou_br.at(0, 0), 0.0001);
    EXPECT_NEAR(0.013627, month_5.ec_tou_br.at(0, 1), 0.0001);
    EXPECT_NEAR(0.010275, month_5.ec_tou_br.at(0, 2), 0.0001);
    EXPECT_NEAR(0.00771, month_5.ec_tou_br.at(0, 3), 0.0001);
}

TEST(lib_utility_rate_equations_test, test_billing_demand_calcs)
{
    ssc_number_t p_ur_ec_sched_weekday[288] = { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };
    ssc_number_t p_ur_ec_sched_weekend[288] = { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };
    ssc_number_t p_ur_ec_tou_mat[96] = { 1, 1, 200, 1, 0.031718000000000003, 0,
                                         1, 6, 400, 1, 0.013627, 0,
                                         1, 7, 600, 1, 0.010275, 0,
                                         1, 8, 9.9999999999999998e+37, 1, 0.00771, 0,
                                         2, 1, 200, 1, 0.028812999999999998, 0,
                                         2, 6, 400, 1, 0.013627, 0,
                                         2, 7, 600, 1, 0.010275, 0,
                                         2, 8, 9.9999999999999998e+37, 1, 0.00771, 0 };
    size_t tou_rows = 16;
    bool sell_eq_buy = false;

    ssc_number_t p_ur_dc_sched_weekday[288] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
    ssc_number_t p_ur_dc_sched_weekend[288] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
    ssc_number_t p_ur_dc_tou_mat[4] = { 1, 1, 9.9999999999999998e+37, 0 };
    size_t dc_tou_rows = 1;

    ssc_number_t p_ur_dc_flat_mat[48] = { 0, 1, 9.9999999999999998e+37, 0,
                                          1, 1, 9.9999999999999998e+37, 0,
                                          2, 1, 9.9999999999999998e+37, 0,
                                          3, 1, 9.9999999999999998e+37, 0,
                                          4, 1, 9.9999999999999998e+37, 0,
                                          5, 1, 9.9999999999999998e+37, 0,
                                          6, 1, 9.9999999999999998e+37, 0,
                                          7, 1, 9.9999999999999998e+37, 0,
                                          8, 1, 9.9999999999999998e+37, 0,
                                          9, 1, 9.9999999999999998e+37, 0,
                                          10, 1, 9.9999999999999998e+37, 0,
                                          11, 1, 9.9999999999999998e+37, 0 };

    size_t dc_flat_rows = 12;
    size_t start_day_of_year = 0;

    rate_data data;
    data.m_num_rec_yearly = 8760;
    data.rate_scale = { 1 };
    data.init(8760);
    data.setup_demand_charges(&p_ur_dc_sched_weekday[0], &p_ur_dc_sched_weekend[0], dc_tou_rows, &p_ur_dc_tou_mat[0], dc_flat_rows, &p_ur_dc_flat_mat[0], start_day_of_year);
    data.setup_energy_rates(&p_ur_ec_sched_weekday[0], &p_ur_ec_sched_weekend[0], tou_rows, &p_ur_ec_tou_mat[0], sell_eq_buy, start_day_of_year);
    data.init_energy_rates_all_months(false); // This gets called once to set up all the vectors
    for (size_t i = 0; i < 12; i++) {
        data.init_dc_peak_vectors(i);
    }

    // Only need one power number per month to set up the peak
    ssc_number_t year_zero_power[12] = { 1200,
                                            1100,
                                            900,
                                            700,
                                            800,
                                            950,
                                            1050,
                                            1150,
                                            850,
                                            400,
                                            600,
                                            750 };

    ssc_number_t p_ur_ec_billing_demand_lookback_percentages [24] = { 60, 0,
                                          60, 0,
                                          60, 0,
                                          60, 0,
                                          60, 0,
                                          95, 1,
                                          95, 1,
                                          95, 1,
                                          95, 1,
                                          60, 0,
                                          60, 0,
                                          60, 0 };

    ssc_number_t p_ur_billing_demand_tou_matrix[2] = { 1, 1 };

    data.setup_ratcheting_demand(p_ur_ec_billing_demand_lookback_percentages, p_ur_billing_demand_tou_matrix);
    data.bd_minimum = 500;
    data.en_billing_demand_lookback = true;
    data.uses_billing_demand = true;
    data.bd_lookback_months = 11;

    std::vector<ssc_number_t> year_one_power = {  -1200,
                                            -1100,
                                            -900,
                                            -700,
                                            -800,
                                            -950,
                                            -1050,
                                            -1150,
                                            -850,
                                            -400,
                                            -600,
                                            -750 };

    int step = 0;
    int month = 0;
    ur_month& curr_month = data.m_month[month];
    for (month = 0; month < year_one_power.size(); month++)
    {
        double power = year_one_power.at(month);
        // Hourly, so power and energy are the same number
        curr_month = data.m_month[month];
        curr_month.update_net_and_peak(power, power, step);
        data.m_month[month] = curr_month;
        data.find_dc_tou_peak(month, power, step);
    }

    data.setup_prev_demand(year_zero_power);

    std::vector<double> billing_demands = { 1092.5,
                                            1092.5,
                                            1092.5,
                                            1092.5,
                                            1092.5,
                                            1092.5,
                                            1092.5,
                                            1150,
                                            1092.5,
                                            1092.5,
                                            1092.5,
                                            1092.5 };

    for (month = 0; month < year_one_power.size(); month++)
    {
        EXPECT_NEAR(billing_demands[month], data.get_billing_demand(month), 0.1) << " at month " << month;
    }
}

// APS large rate: https://openei.org/apps/IURDB/rate/view/5caf91045457a3c4357780e3
// https://www.aps.com/-/media/APS/APSCOM-PDFs/Utility/Regulatory-and-Legal/Regulatory-Plan-Details-Tariffs/Business/TOU-Business-NonRes-Plans/e32_TimeOfUseLarge.ashx?la=en
TEST(lib_utility_rate_equations_test, test_billing_demand_calcs_w_tou)
{
   
    ssc_number_t p_ur_ec_sched_weekday[288] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1 };
    ssc_number_t p_ur_ec_sched_weekend[288] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
    ssc_number_t p_ur_ec_tou_mat[24] = { 1, 1, 9.9999999999999998e+37, 0, 0.044822000000000001, 0,
                                         2, 1, 9.9999999999999998e+37, 0, 0.057702000000000003, 0,
                                         3, 1, 9.9999999999999998e+37, 0, 0.059481999999999993, 0,
                                         4, 1, 9.9999999999999998e+37, 0, 0.07236200000000001, 0 };

    size_t tou_rows = 4;
    bool sell_eq_buy = false;

    ssc_number_t p_ur_dc_sched_weekday[288] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1 };
    ssc_number_t p_ur_dc_sched_weekend[288] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
    ssc_number_t p_ur_dc_tou_mat[16] = { 1, 1, 100, 6.6539999999999999,
                                         1, 2, 9.9999999999999998e+37, 3.6280000000000001,
                                         2, 1, 100, 17.765999999999998,
                                         2, 2, 9.9999999999999998e+37, 12.053000000000001 };
    size_t dc_tou_rows = 4;

    ssc_number_t p_ur_dc_flat_mat[48] = { 0, 1, 9.9999999999999998e+37, 0,
                                          1, 1, 9.9999999999999998e+37, 0,
                                          2, 1, 9.9999999999999998e+37, 0,
                                          3, 1, 9.9999999999999998e+37, 0,
                                          4, 1, 9.9999999999999998e+37, 0,
                                          5, 1, 9.9999999999999998e+37, 0,
                                          6, 1, 9.9999999999999998e+37, 0,
                                          7, 1, 9.9999999999999998e+37, 0,
                                          8, 1, 9.9999999999999998e+37, 0,
                                          9, 1, 9.9999999999999998e+37, 0,
                                          10, 1, 9.9999999999999998e+37, 0,
                                          11, 1, 9.9999999999999998e+37, 0 };
    

    size_t dc_flat_rows = 12;
    size_t start_day_of_year = 0;

    rate_data data;
    data.m_num_rec_yearly = 8760;
    data.rate_scale = { 1 };
    data.init(8760);
    data.setup_demand_charges(&p_ur_dc_sched_weekday[0], &p_ur_dc_sched_weekend[0], dc_tou_rows, &p_ur_dc_tou_mat[0], dc_flat_rows, &p_ur_dc_flat_mat[0], start_day_of_year);
    data.setup_energy_rates(&p_ur_ec_sched_weekday[0], &p_ur_ec_sched_weekend[0], tou_rows, &p_ur_ec_tou_mat[0], sell_eq_buy, start_day_of_year);
    data.init_energy_rates_all_months(false); // This gets called once to set up all the vectors
    for (size_t i = 0; i < 12; i++) {
        data.init_dc_peak_vectors(i);
    }
    data.enable_nm = false;
    data.nm_credits_w_rollover = false;

    ssc_number_t p_ur_billing_demand_lookback_percentages[24] = { 0, 1,
                                                                  0, 1,
                                                                  0, 1,
                                                                  0, 1,
                                                                  80, 1,
                                                                  80, 1,
                                                                  80, 1,
                                                                  80, 1,
                                                                  80, 1,
                                                                  80, 1,
                                                                  0, 1,
                                                                  0, 1 };

    ssc_number_t p_ur_billing_demand_tou_matrix[4] = { 1, 0,
                                                   2, 1 };

    data.setup_ratcheting_demand(p_ur_billing_demand_lookback_percentages, p_ur_billing_demand_tou_matrix);
    data.bd_minimum = 500;
    data.en_billing_demand_lookback = true;
    data.uses_billing_demand = true;
    data.bd_lookback_months = 11;

    ssc_number_t year_zero_power[12] = { 1200,
                                            1100,
                                            900,
                                            700,
                                            800,
                                            950,
                                            1050,
                                            1300, // 80% of this is 1040 kW, which is higher than the peak in Jan (below)
                                            850,
                                            400,
                                            600,
                                            750 };

    // Peak period 1: 900 kW, peak period 2: 1000 kW
    std::vector<double> day_one_power = { -100, -100, -200, -300, -400, -500, // Period 1
                                            -600, -700, -800, -900, -800, -900, // Period 1
                                            -800, -700, -600, -1000, -950, -900, // first 3 period 1, last 3 period 2
                                            -850, -800, -700, -500, -500, -500 }; // First 3 period 2, last 3 period 1

    ASSERT_EQ(24, day_one_power.size());
    int step = 24; // Jan 1st is a Sunday, need to hit on peak period on Monday
    int month = 0;
    ur_month& curr_month = data.m_month[month];
    for (int i = 0; i < day_one_power.size(); i++)
    {
        curr_month.update_net_and_peak(day_one_power.at(i), day_one_power.at(i), step + i);
        // Hourly, so power and energy are the same number
        data.sort_energy_to_periods(month, day_one_power.at(i), step + i);
        data.find_dc_tou_peak(month, day_one_power.at(i), step + i);
    }

    if (data.uses_billing_demand) {
        if (data.en_billing_demand_lookback) {
            data.setup_prev_demand(year_zero_power);
        }
        for (int m = 0; m < (int)data.m_month.size(); m++) {
            double flat_peak = data.m_month[m].dc_flat_peak;
            if (data.en_billing_demand_lookback) {
                // If ratchets are present the peak used here might be the actual peak, or something based on a previous month.
                flat_peak = data.get_billing_demand(m);
            }
            data.billing_demand[m] = flat_peak;
        }
    }

    ASSERT_NEAR(16674.22, data.get_demand_charge(0, 0), 0.01);
}

TEST(lib_utility_rate_equations_test, test_demand_hourly_tou_charges)
{
    ssc_number_t p_ur_ec_sched_weekday[288] = { 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 4, 4, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 4, 4, 4, 4 };
    ssc_number_t p_ur_ec_sched_weekend[288] = { 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 };
    ssc_number_t p_ur_ec_tou_mat[24] = { 1, 1, 9.9999999999999998e+37, 0, 0.050000000000000003, 0,
        2, 1, 9.9999999999999998e+37, 0, 0.074999999999999997, 0,
        3, 1, 9.9999999999999998e+37, 0, 0.059999999999999998, 0,
        4, 1, 9.9999999999999998e+37, 0, 0.050000000000000003, 0 };
    size_t tou_rows = 4;
    bool sell_eq_buy = false;

    // Schedyles don't have period 1 in March
    ssc_number_t p_ur_dc_sched_weekday[288] = { 3, 3, 3, 3, 3, 3, 2, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 2, 2, 3, 3, 4, 4, 4, 4, 4, 4, 3, 2, 2, 2, 2, 2, 2, 2, 3, 3, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 3, 2, 2, 2, 2, 2, 2, 2, 3, 3, 2, 2, 2, 2, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 3, 2, 2, 2, 2, 2, 2, 2, 3, 3, 2, 2, 2, 2, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 3, 2, 2, 2, 2, 2, 2, 2, 3, 3, 2, 2, 2, 2, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 3, 2, 2, 2, 2, 2, 2, 2, 3, 3, 2, 2, 2, 2, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 3, 2, 2, 2, 2, 2, 2, 2, 3, 3, 2, 2, 2, 2, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 3, 2, 2, 2, 2, 2, 2, 2, 3, 3, 2, 2, 2, 2, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 3, 2, 2, 2, 2, 2, 2, 2, 3, 3, 2, 2, 2, 2, 3, 3, 4, 4, 3, 3, 3, 3, 3, 3, 2, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 2, 2, 3, 3 };
    ssc_number_t p_ur_dc_sched_weekend[288] = { 4, 4, 4, 4, 4, 4, 3, 2, 2, 2, 2, 2, 2, 2, 3, 3, 2, 2, 2, 2, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 3, 2, 2, 2, 2, 2, 2, 2, 3, 3, 2, 2, 2, 2, 3, 3, 4, 4, 5, 5, 5, 5, 5, 5, 4, 3, 3, 3, 3, 3, 3, 3, 4, 4, 3, 3, 3, 3, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3, 3, 3, 3, 3, 3, 3, 4, 4, 3, 3, 3, 3, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3, 3, 3, 3, 3, 3, 3, 4, 4, 3, 3, 3, 3, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3, 3, 3, 3, 3, 3, 3, 4, 4, 3, 3, 3, 3, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3, 3, 3, 3, 3, 3, 3, 4, 4, 3, 3, 3, 3, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3, 3, 3, 3, 3, 3, 3, 4, 4, 3, 3, 3, 3, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3, 3, 3, 3, 3, 3, 3, 4, 4, 3, 3, 3, 3, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3, 3, 3, 3, 3, 3, 3, 4, 4, 3, 3, 3, 3, 4, 4, 5, 5, 4, 4, 4, 4, 4, 4, 3, 2, 2, 2, 2, 2, 2, 2, 3, 3, 2, 2, 2, 2, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 3, 2, 2, 2, 2, 2, 2, 2, 3, 3, 2, 2, 2, 2, 3, 3, 4, 4 };
    ssc_number_t p_ur_dc_tou_mat[20] = { 1, 1, 9.9999996802856925e+37, 3.3640100955963135,
                                         2, 1, 9.9999996802856925e+37, 0.8336300253868103,
                                         3, 1, 9.9999996802856925e+37, 0.1803400069475174,
                                         4, 1, 9.9999996802856925e+37, 0.012780000455677509,
                                         5, 1, 9.9999996802856925e+37, 0 };

    size_t dc_tou_rows = 5;
    // Also test incomplete flat mat
    ssc_number_t p_ur_dc_flat_mat[8] = { 0, 1, 9.9999999999999998e+37, 0, 1, 1, 9.9999996802856925e+37, 0 };


    /*
    ssc_number_t p_ur_dc_flat_mat[48] = { 0, 1, 9.9999999999999998e+37, 0,
                                        1, 1, 9.9999999999999998e+37, 0,
                                        2, 1, 9.9999999999999998e+37, 0,
                                        3, 1, 9.9999999999999998e+37, 0,
                                        4, 1, 9.9999999999999998e+37, 0,
                                        5, 1, 9.9999999999999998e+37, 0,
                                        6, 1, 9.9999999999999998e+37, 0,
                                        7, 1, 9.9999999999999998e+37, 0,
                                        8, 1, 9.9999999999999998e+37, 0,
                                        9, 1, 9.9999999999999998e+37, 0,
                                        10, 1, 9.9999999999999998e+37, 0,
                                        11, 1, 9.9999999999999998e+37, 0 };
                                        */
    size_t dc_flat_rows = 2;
    size_t start_day_of_year = 0;

    rate_data data;
    data.m_num_rec_yearly = 8760;
    data.rate_scale = { 1 };
    data.init(8760);
    data.setup_demand_charges(&p_ur_dc_sched_weekday[0], &p_ur_dc_sched_weekend[0], dc_tou_rows, &p_ur_dc_tou_mat[0], dc_flat_rows, &p_ur_dc_flat_mat[0], start_day_of_year);
    data.setup_energy_rates(&p_ur_ec_sched_weekday[0], &p_ur_ec_sched_weekend[0], tou_rows, &p_ur_ec_tou_mat[0], sell_eq_buy, start_day_of_year);
    data.init_energy_rates_all_months(false);
    data.init_dc_peak_vectors(2);

    // Peak period 1: 5 kW, peak period 2: 10 kW
    std::vector<double> day_one_power = { -1, -1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -9, -8, -7, -6, -5, -4, -3, -2, -1, -1, -1, -1, -1 };

    ASSERT_EQ(24, day_one_power.size());
    int step = 1416; // 12 am March 1st
    int month = 2;
    for (int i = 0; i < day_one_power.size(); i++)
    {
        // Hourly, so power and energy are the same number
        data.sort_energy_to_periods(month, day_one_power.at(i), step + i);
        data.find_dc_tou_peak(month, day_one_power.at(i), step + i);
    }

    ASSERT_NEAR(9.482, data.get_demand_charge(2, 0), 0.01);

    // Check whether or not it's safe to call this function twice on the same data
    ASSERT_NEAR(9.482, data.get_demand_charge(2, 0), 0.01);
}

TEST(lib_utility_rate_equations_test, test_demand_subhourly_tou_charges)
{
    ssc_number_t p_ur_ec_sched_weekday[288] = { 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 4, 4, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 4, 4, 4, 4 };
    ssc_number_t p_ur_ec_sched_weekend[288] = { 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 };
    ssc_number_t p_ur_ec_tou_mat[24] = { 1, 1, 9.9999999999999998e+37, 0, 0.050000000000000003, 0,
        2, 1, 9.9999999999999998e+37, 0, 0.074999999999999997, 0,
        3, 1, 9.9999999999999998e+37, 0, 0.059999999999999998, 0,
        4, 1, 9.9999999999999998e+37, 0, 0.050000000000000003, 0 };
    size_t tou_rows = 4;
    bool sell_eq_buy = false;

    // Schedyles don't have period 1 in March
    ssc_number_t p_ur_dc_sched_weekday[288] = { 3, 3, 3, 3, 3, 3, 2, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 2, 2, 3, 3, 4, 4, 4, 4, 4, 4, 3, 2, 2, 2, 2, 2, 2, 2, 3, 3, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 3, 2, 2, 2, 2, 2, 2, 2, 3, 3, 2, 2, 2, 2, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 3, 2, 2, 2, 2, 2, 2, 2, 3, 3, 2, 2, 2, 2, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 3, 2, 2, 2, 2, 2, 2, 2, 3, 3, 2, 2, 2, 2, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 3, 2, 2, 2, 2, 2, 2, 2, 3, 3, 2, 2, 2, 2, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 3, 2, 2, 2, 2, 2, 2, 2, 3, 3, 2, 2, 2, 2, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 3, 2, 2, 2, 2, 2, 2, 2, 3, 3, 2, 2, 2, 2, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 3, 2, 2, 2, 2, 2, 2, 2, 3, 3, 2, 2, 2, 2, 3, 3, 4, 4, 3, 3, 3, 3, 3, 3, 2, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 2, 2, 3, 3 };
    ssc_number_t p_ur_dc_sched_weekend[288] = { 4, 4, 4, 4, 4, 4, 3, 2, 2, 2, 2, 2, 2, 2, 3, 3, 2, 2, 2, 2, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 3, 2, 2, 2, 2, 2, 2, 2, 3, 3, 2, 2, 2, 2, 3, 3, 4, 4, 5, 5, 5, 5, 5, 5, 4, 3, 3, 3, 3, 3, 3, 3, 4, 4, 3, 3, 3, 3, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3, 3, 3, 3, 3, 3, 3, 4, 4, 3, 3, 3, 3, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3, 3, 3, 3, 3, 3, 3, 4, 4, 3, 3, 3, 3, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3, 3, 3, 3, 3, 3, 3, 4, 4, 3, 3, 3, 3, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3, 3, 3, 3, 3, 3, 3, 4, 4, 3, 3, 3, 3, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3, 3, 3, 3, 3, 3, 3, 4, 4, 3, 3, 3, 3, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3, 3, 3, 3, 3, 3, 3, 4, 4, 3, 3, 3, 3, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3, 3, 3, 3, 3, 3, 3, 4, 4, 3, 3, 3, 3, 4, 4, 5, 5, 4, 4, 4, 4, 4, 4, 3, 2, 2, 2, 2, 2, 2, 2, 3, 3, 2, 2, 2, 2, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 3, 2, 2, 2, 2, 2, 2, 2, 3, 3, 2, 2, 2, 2, 3, 3, 4, 4 };
    ssc_number_t p_ur_dc_tou_mat[20] = { 1, 1, 9.9999996802856925e+37, 3.3640100955963135,
                                         2, 1, 9.9999996802856925e+37, 0.8336300253868103,
                                         3, 1, 9.9999996802856925e+37, 0.1803400069475174,
                                         4, 1, 9.9999996802856925e+37, 0.012780000455677509,
                                         5, 1, 9.9999996802856925e+37, 0 };

    size_t dc_tou_rows = 5;
    // Also test incomplete flat mat
    ssc_number_t p_ur_dc_flat_mat[8] = { 0, 1, 9.9999999999999998e+37, 0, 1, 1, 9.9999996802856925e+37, 0 };


    /*
    ssc_number_t p_ur_dc_flat_mat[48] = { 0, 1, 9.9999999999999998e+37, 0,
                                        1, 1, 9.9999999999999998e+37, 0,
                                        2, 1, 9.9999999999999998e+37, 0,
                                        3, 1, 9.9999999999999998e+37, 0,
                                        4, 1, 9.9999999999999998e+37, 0,
                                        5, 1, 9.9999999999999998e+37, 0,
                                        6, 1, 9.9999999999999998e+37, 0,
                                        7, 1, 9.9999999999999998e+37, 0,
                                        8, 1, 9.9999999999999998e+37, 0,
                                        9, 1, 9.9999999999999998e+37, 0,
                                        10, 1, 9.9999999999999998e+37, 0,
                                        11, 1, 9.9999999999999998e+37, 0 };
                                        */
    size_t dc_flat_rows = 2;
    size_t start_day_of_year = 0;

    rate_data data;
    data.m_num_rec_yearly = 8760*4;
    data.rate_scale = { 1 };
    data.init(8760*4);
    data.setup_demand_charges(&p_ur_dc_sched_weekday[0], &p_ur_dc_sched_weekend[0], dc_tou_rows, &p_ur_dc_tou_mat[0], dc_flat_rows, &p_ur_dc_flat_mat[0], start_day_of_year);
    data.setup_energy_rates(&p_ur_ec_sched_weekday[0], &p_ur_ec_sched_weekend[0], tou_rows, &p_ur_ec_tou_mat[0], sell_eq_buy, start_day_of_year);
    data.init_energy_rates_all_months(false); // This gets called once to set up all the vectors
    data.init_dc_peak_vectors(2);

    // Peak period 1: 5 kW, peak period 2: 10 kW
    std::vector<double> day_one_power = { -1, -1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -9, -8, -7, -6, -5, -4, -3, -2, -1, -1, -1, -1, -1 };

    ASSERT_EQ(24, day_one_power.size());
    int step = 1416 * 4; // 12 am March 1st
    int month = 2;
    for (int i = 0; i < day_one_power.size(); i++)
    {
        // Hourly, so power and energy are the same number
        data.sort_energy_to_periods(month, day_one_power.at(i), step + i);
        data.find_dc_tou_peak(month, day_one_power.at(i), step + i);
    }

    // Only ran 6 hours of data, so expect different charges
    ASSERT_NEAR(0.128, data.get_demand_charge(2, 0), 0.01);

    // Check whether or not it's safe to call this function twice on the same data
    ASSERT_NEAR(0.128, data.get_demand_charge(2, 0), 0.01);
}
