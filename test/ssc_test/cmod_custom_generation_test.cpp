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
#include <fstream>
#include <sstream>

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

TEST_F(CMCustomGeneration, cmod_custom_generation_test_adjust_losses) {

    custom_generation_singleowner_battery_60min(data);

    // Test constant generation
    ssc_data_set_number(data, "adjust_constant", 5.0);
    ssc_data_set_number(data, "system_capacity", 100.0);
    ssc_data_set_number(data, "user_capacity_factor", 100.0);
    ssc_data_set_number(data, "spec_mode", 0);

    EXPECT_FALSE(run_module(data, "custom_generation"));

    ssc_number_t annual_energy;
    ssc_data_get_number(data, "annual_energy", &annual_energy);
    EXPECT_NEAR(annual_energy, 8760 * 100.0 * 0.95, 0.01);

    // Test annual custom schedule
    std::vector<double> custom_schedule(8760, 100.0);
    ssc_data_set_array(data, "energy_output_array", custom_schedule.data(), (int)custom_schedule.size());
    
    EXPECT_FALSE(run_module(data, "custom_generation"));
    ssc_data_set_number(data, "spec_mode", 1);
    ssc_data_get_number(data, "annual_energy", &annual_energy);
    EXPECT_NEAR(annual_energy, 8760 * 100.0 * 0.95, 0.01);

    // Test lifetime custom schedule
    std::vector<double> custom_schedule_lifetime(8760 * 25, 100.0);
    ssc_data_set_array(data, "energy_output_array_lifetime", custom_schedule_lifetime.data(), (int)custom_schedule_lifetime.size());

    EXPECT_FALSE(run_module(data, "custom_generation"));
    ssc_data_set_number(data, "spec_mode", 2);
    ssc_data_get_number(data, "annual_energy", &annual_energy);
    EXPECT_NEAR(annual_energy, 8760 * 100.0 * 0.95, 0.01);
}

/// Runs lifetime custom generation where year 2 generates a tenth of the other years, then the
/// financial model, and checks that fuel expenses follow annual fuel usage instead of staying at year 1 (ssc #1422)
static void check_lifetime_fuel_expense(ssc_data_t& data, const std::string& json_file, const char* financial_module) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/" + json_file;
    std::ifstream file(file_inputs);
    std::ostringstream tmp;
    tmp << file.rdbuf();
    file.close();
    ssc_data_free(data);
    data = json_to_ssc_data(tmp.str().c_str());

    int analysis_period = 25;
    ssc_data_set_number(data, "analysis_period", analysis_period);
    ssc_data_set_number(data, "system_use_lifetime_output", 1);
    ssc_number_t degradation[1] = { 0 };
    ssc_data_set_array(data, "ac_degradation", degradation, 1);
    ssc_data_set_number(data, "adjust_constant", 0);
    ssc_data_set_number(data, "spec_mode", 2);
    ssc_data_set_number(data, "derate", 0);
    ssc_data_set_number(data, "user_capacity_factor", 0);
    ssc_data_set_number(data, "heat_rate", 10);
    ssc_data_set_number(data, "conv_eff", 34.118049621582031);

    // Year 2 generates a tenth of the other years
    std::vector<double> gen_lifetime(8760 * analysis_period, 1000.0);
    for (size_t i = 8760; i < 8760 * 2; i++)
        gen_lifetime[i] = 100.0;
    ssc_data_set_array(data, "energy_output_array_lifetime", gen_lifetime.data(), (int)gen_lifetime.size());

    EXPECT_FALSE(run_module(data, "custom_generation"));

    int n = 0;
    ssc_number_t* fuel_usage = ssc_data_get_array(data, "annual_fuel_usage_lifetime", &n);
    ASSERT_TRUE(fuel_usage != nullptr);
    ASSERT_EQ(n, analysis_period);
    double year1_fuel = 8760 * 1000.0 * 100.0 / 34.118049621582031; // kWht
    EXPECT_NEAR(fuel_usage[0], year1_fuel, 1.0);
    EXPECT_NEAR(fuel_usage[1], year1_fuel / 10.0, 1.0);
    EXPECT_NEAR(fuel_usage[2], year1_fuel, 1.0);

    EXPECT_FALSE(run_module(data, financial_module));

    ssc_number_t* fuel_expense = ssc_data_get_array(data, "cf_om_fuel_expense", &n);
    ASSERT_TRUE(fuel_expense != nullptr);
    ASSERT_EQ(n, analysis_period + 1);
    double escalation = 1.025; // inflation, no fuel cost escalation
    EXPECT_NEAR(fuel_expense[2] / fuel_expense[1], 0.1 * escalation, 1e-6) << financial_module;
    EXPECT_NEAR(fuel_expense[3] / fuel_expense[1], escalation * escalation, 1e-6) << financial_module;
}

TEST_F(CMCustomGeneration, LifetimeFuelUsage_MerchantPlant) {
    check_lifetime_fuel_expense(data, "merchantplant/2022.08.08_develop_branch_Generic_System_Merchant_Plant_cmod_merchantplant.json", "merchantplant");
}

TEST_F(CMCustomGeneration, LifetimeFuelUsage_EquPartFlip) {
    check_lifetime_fuel_expense(data, "equpartflip/2023.10.27_om-expense-cash-flow_Generic_System_All_Equity_Partnership_Flip_cmod_equpartflip.json", "equpartflip");
}

TEST_F(CMCustomGeneration, LifetimeFuelUsage_LevPartFlip) {
    check_lifetime_fuel_expense(data, "levpartflip/2023.10.27_om-expense-cash-flow_Generic_System_Leveraged_Partnership_Flip_cmod_levpartflip.json", "levpartflip");
}

TEST_F(CMCustomGeneration, LifetimeFuelUsage_SaleLeaseback) {
    check_lifetime_fuel_expense(data, "saleleaseback/2023.10.27_om-expense-cash-flow_Generic_System_Sale_Leaseback_cmod_saleleaseback.json", "saleleaseback");
}

TEST_F(CMCustomGeneration, LifetimeFuelUsage_HostDeveloper) {
    check_lifetime_fuel_expense(data, "host_developer/2023.10.27_om-expense-cash-flow_Generic_System_Host_Developer_cmod_host_developer.json", "host_developer");
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
