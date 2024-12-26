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


#include <gtest/gtest.h>

#include "cmod_custom_generation_test.h"

/// Test Custom Generation with Battery for SingleOwner PPA
TEST_F(CMCustomGeneration, SingleOwnerWithBattery_cmod_custom_generation) {

	custom_generation_singleowner_battery_60min(data);

	// Test different dispatch strategies
	std::vector<size_t> dispatch_options{ 0,2,3 };

	// Run with hourly data
	for (size_t i = 0; i < dispatch_options.size(); i++) {
		ssc_data_set_number(data, "batt_dispatch_choice", (ssc_number_t)dispatch_options[i]);
		EXPECT_FALSE(run_module(data, "custom_generation"));
		EXPECT_FALSE(run_module(data, "battery"));
		EXPECT_FALSE(run_module(data, "singleowner"));
	}
	
	// Run with subhourly data
	set_array(data, "energy_output_array", customgenerationtest::gen_path_30min, 8760 * 2);
	set_array(data, "batt_custom_dispatch", customgenerationtest::batt_dispatch_path_30min, 8760 * 2);
	set_array(data, "batt_room_temperature_celsius", customgenerationtest::temperature_path_30min, 8760 * 2);
	for (size_t i = 0; i < dispatch_options.size(); i++) {
		ssc_data_set_number(data, "batt_dispatch_choice", (ssc_number_t)dispatch_options[i]);
		EXPECT_FALSE(run_module(data, "custom_generation"));
		EXPECT_FALSE(run_module(data, "battery"));
		EXPECT_FALSE(run_module(data, "singleowner"));
	}

	// Test with incorrect combo of data sizes
	ssc_data_set_number(data, "batt_dispatch_choice", 3);
	set_array(data, "batt_custom_dispatch", customgenerationtest::batt_dispatch_path_30min, 8760*2); // 8760 or 8760 * 3 fails with execution error on Linux and windows
	EXPECT_FALSE(run_module(data, "custom_generation"));
	EXPECT_FALSE(run_module(data, "battery"));
}

/// Test Generic System with Battery for various timesteps
TEST_F(CMCustomGeneration, CommercialWithBattery_cmod_generic) {

	custom_generation_commerical_battery_60min(data);
	
	// Test different dispatch strategies
	std::vector<size_t> dispatch_options{ 0,2,3 };

	// Run with hourly data, with and without lifetime
	for (size_t l = 0; l < 2; l++) {
		ssc_data_set_number(data, "system_use_lifetime_output", l);
		for (size_t i = 0; i < dispatch_options.size(); i++) {
			ssc_data_set_number(data, "batt_dispatch_choice", (ssc_number_t)dispatch_options[i]);
			EXPECT_FALSE(run_module(data, "custom_generation"));
			EXPECT_FALSE(run_module(data, "battery"));
			EXPECT_FALSE(run_module(data, "utilityrate5"));
			EXPECT_FALSE(run_module(data, "cashloan"));
		}
	}

	// Run with subhourly data
	set_array(data, "energy_output_array", customgenerationtest::gen_path_30min, 8760 * 2);
	set_array(data, "batt_custom_dispatch", customgenerationtest::batt_dispatch_path_30min, 8760 * 2);
	set_array(data, "batt_room_temperature_celsius", customgenerationtest::temperature_path_30min, 8760 * 2);
	set_array(data, "load", customgenerationtest::load_profile_path_30min, 8760 * 2);

	// With and without lifetime
	for (size_t l = 0; l < 2; l++) {
		ssc_data_set_number(data, "system_use_lifetime_output", l);
		for (size_t i = 0; i < dispatch_options.size(); i++) {
			ssc_data_set_number(data, "batt_dispatch_choice", (ssc_number_t)dispatch_options[i]);
			EXPECT_FALSE(run_module(data, "custom_generation"));
			EXPECT_FALSE(run_module(data, "battery"));
			EXPECT_FALSE(run_module(data, "utilityrate5"));
			EXPECT_FALSE(run_module(data, "cashloan"));
		}
	}
	
	// Test with hourly load, subhourly gen
	ssc_data_set_number(data, "batt_dispatch_choice", 3);
	set_array(data, "batt_custom_dispatch", customgenerationtest::batt_dispatch_path_30min, 8760 * 2);
	set_array(data, "energy_output_array", customgenerationtest::gen_path_30min, 2*8760);
	set_array(data, "batt_room_temperature_celsius", customgenerationtest::temperature_path_30min, 8760 * 2);
	set_array(data, "load", customgenerationtest::load_profile_path_60min, 8760);
	
	for (size_t l = 0; l < 2; l++) {
		ssc_data_set_number(data, "system_use_lifetime_output", l);
		EXPECT_FALSE(run_module(data, "custom_generation"));
		EXPECT_FALSE(run_module(data, "battery"));
	}
}

/*
Doesn't work to to outdated exeception handling methods in SSC which can not be 
handled robustly in a cross-platform environment
https://docs.microsoft.com/en-us/cpp/cpp/errors-and-exception-handling-modern-cpp?view=vs-2017#c-exceptions-versus-windows-seh-exceptions
*/
/*
TEST_F(CMCustomGeneration, CommericalWithBatteryWrongSizes)
{
	// Test with hourly gen, subhourly load, should fail everywhere
	generic_commerical_battery_60min(data);
	set_array(data, "load", customgenerationtest::load_profile_path_30min, 2 * 8760);

	EXPECT_TRUE(run_module(data, "generic_system"));
	EXPECT_TRUE(run_module(data, "battery"));
}
*/
