/*
BSD 3-Clause License

Copyright (c) Alliance for Energy Innovation, LLC. See also https://github.com/NatLabRockies/ssc/blob/develop/LICENSE
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

#include "cmod_grid_test.h"

TEST_F(CMGrid_cmod_grid, SingleYearHourly_cmod_grid) {

	grid_default_60_min(data);

	// Run with fixed output
	int errors = run_module(data, "grid");
	EXPECT_FALSE(errors);
	if (!errors)
	{
		ssc_number_t capacity_factor, annual_energy_pre_interconnect, annual_energy_interconnect;
		int gen_size;
		ssc_data_get_number(data, "capacity_factor_interconnect_ac", &capacity_factor);
		ssc_data_get_number(data, "annual_energy_pre_interconnect_ac", &annual_energy_pre_interconnect);
		ssc_data_get_number(data, "annual_energy", &annual_energy_interconnect);
		ssc_number_t * system_kwac = GetArray("gen", gen_size);
		ssc_number_t * system_pre_interconnect_kwac = GetArray("system_pre_interconnect_kwac", gen_size);

		EXPECT_EQ(gen_size, 8760);
		EXPECT_NEAR(capacity_factor, 29.0167, 0.001);
		EXPECT_NEAR(annual_energy_interconnect, 457534759, 10);
		EXPECT_NEAR(annual_energy_pre_interconnect, 460351000, 10);
	}
}

TEST_F(CMGrid_cmod_grid, SingleYearSubHourly_cmod_grid) {

	grid_default_30_min(data);
	int errors = run_module(data, "grid");
	EXPECT_FALSE(errors);
	if (!errors)
	{
		ssc_number_t capacity_factor, annual_energy_pre_interconnect, annual_energy, annual_energy_pre_curtailment_ac;
		int gen_size;
		ssc_data_get_number(data, "capacity_factor_interconnect_ac", &capacity_factor);
		ssc_data_get_number(data, "annual_energy_pre_interconnect_ac", &annual_energy_pre_interconnect);
		ssc_data_get_number(data, "annual_energy", &annual_energy);
		ssc_data_get_number(data, "annual_energy_pre_curtailment_ac", &annual_energy_pre_curtailment_ac);
		ssc_number_t * system_kwac = GetArray("gen", gen_size);
		ssc_number_t * system_pre_interconnect_kwac = GetArray("system_pre_interconnect_kwac", gen_size);

		EXPECT_EQ(gen_size, 8760*2);
		EXPECT_NEAR(capacity_factor, 30.78777, 0.001);
		EXPECT_NEAR(annual_energy, 485461500, 10);
		EXPECT_NEAR(annual_energy_pre_interconnect, 498820000, 10);
		EXPECT_NEAR(annual_energy_pre_curtailment_ac, 485461500, 10);
	}
}

TEST_F(CMGrid_cmod_grid, SingleYearSubHourlyLifetime_cmod_grid) {

	grid_default_30_min_lifetime(data);
	int errors = run_module(data, "grid");
	EXPECT_FALSE(errors);
	if (!errors)
	{
		ssc_number_t capacity_factor, annual_energy_pre_interconnect, annual_energy_interconnect;
		int gen_size;
		ssc_data_get_number(data, "capacity_factor_interconnect_ac", &capacity_factor);
		ssc_data_get_number(data, "annual_energy_pre_interconnect_ac", &annual_energy_pre_interconnect);
		ssc_data_get_number(data, "annual_energy", &annual_energy_interconnect);
		ssc_number_t * system_kwac = GetArray("gen", gen_size);
		ssc_number_t * system_pre_interconnect_kwac = GetArray("system_pre_interconnect_kwac", gen_size);

		EXPECT_EQ(gen_size, 8760 * 2 * 2);
		EXPECT_NEAR(capacity_factor, 30.78777, 0.001);
		EXPECT_NEAR(annual_energy_interconnect, 485461500, 10);
		EXPECT_NEAR(annual_energy_pre_interconnect, 498820000, 10);
	}
}

TEST_F(CMGrid_cmod_grid, SingleYearNoFinancial_cmod_grid) {

	grid_default_60_min_no_financial(data);
	int errors = run_module(data, "grid");
	EXPECT_FALSE(errors);
	if (!errors)
	{
		ssc_number_t capacity_factor, annual_energy_pre_interconnect, annual_energy_interconnect;
		int gen_size;
		ssc_data_get_number(data, "capacity_factor_interconnect_ac", &capacity_factor);
		ssc_data_get_number(data, "annual_energy_pre_interconnect_ac", &annual_energy_pre_interconnect);
		ssc_data_get_number(data, "annual_energy", &annual_energy_interconnect);
		ssc_number_t * system_kwac = GetArray("gen", gen_size);
		ssc_number_t * system_pre_interconnect_kwac = GetArray("system_pre_interconnect_kwac", gen_size);

		EXPECT_EQ(gen_size, 8760);
		EXPECT_NEAR(capacity_factor, 29.0167, 0.001);
		EXPECT_NEAR(annual_energy_interconnect, 457534759, 10);
		EXPECT_NEAR(annual_energy_pre_interconnect, 460351000, 10);
	}
}
