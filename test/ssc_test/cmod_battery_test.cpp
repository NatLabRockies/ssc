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


#include <numeric>

#include <gtest/gtest.h>

#include "vartab.h"

#include "cmod_battery_test.h"


/// Test standalone battery compute modeule with a input lifetime generation and commercial load
TEST_F(CMBattery_cmod_battery, CommercialLifetimePeakShaving) {

	// Run with fixed output
	ssc_number_t n_years;
	ssc_data_get_number(data, "analysis_period", &n_years);
	size_t n_lifetime = (size_t)(n_years) * 8760;

	int errors = run_module(data, "battery");
	EXPECT_FALSE(errors);

	if (!errors)
	{
		// roundtrip efficiency test will ensure that the battery cycled
		ssc_number_t roundtripEfficiency;
		ssc_data_get_number(data, "average_battery_roundtrip_efficiency", &roundtripEfficiency);
		EXPECT_NEAR(roundtripEfficiency, 94.42, 2);

		// test that lifetime output is achieved
		int n;
		calculated_array = ssc_data_get_array(data, "gen", &n);
		EXPECT_EQ(n_lifetime, (size_t)n);

		// test that battery was replaced at some point
		calculated_array = ssc_data_get_array(data, "batt_bank_replacement", &n);
		int replacements = std::accumulate(calculated_array, calculated_array + n, 0);

		EXPECT_GT(replacements, 0);

		// test temperature
		double* arr = ssc_data_get_array(data, "batt_temperature", &n);
		auto temp_array = std::vector<double>(arr, arr + n);
		double max_temp = *std::max_element(temp_array.begin(), temp_array.end());
		EXPECT_NEAR(max_temp, 33, 1);
	}
}

TEST_F(CMBattery_cmod_battery, ResilienceMetricsFullLoad){
    auto data_vtab = static_cast<var_table*>(data);
    data_vtab->assign("crit_load", data_vtab->as_vector_ssc_number_t("load"));
    data_vtab->assign("system_use_lifetime_output", 0);
    data_vtab->assign("analysis_period", 1);
    data_vtab->assign("gen", var_data(data_vtab->as_array("gen", nullptr), 8760));
    data_vtab->assign("batt_replacement_option", 0);
    data_vtab->assign("run_resiliency_calcs", 1);

    int errors = run_module(data, "battery");
    EXPECT_FALSE(errors);

    auto resilience_hours = data_vtab->as_vector_ssc_number_t("resilience_hrs");
    double resilience_hrs_min = data_vtab->as_number("resilience_hrs_min");
    double resilience_hrs_max = data_vtab->as_number("resilience_hrs_max");
    double resilience_hrs_avg = data_vtab->as_number("resilience_hrs_avg");
    auto outage_durations = data_vtab->as_vector_ssc_number_t("outage_durations");
    auto pdf_of_surviving = data_vtab->as_vector_ssc_number_t("pdf_of_surviving");
    double avg_critical_load = data_vtab->as_double("avg_critical_load");

    EXPECT_EQ(resilience_hours[0], 0); // Max current restrictions prevent this battery from meeting the outage until day 2 (hr 46)
    EXPECT_EQ(resilience_hours[46], 5);
    EXPECT_NEAR(avg_critical_load, 683.90, 0.1);
    EXPECT_NEAR(resilience_hrs_avg, 1.11, 0.01);
    EXPECT_EQ(resilience_hrs_min, 0);
    EXPECT_EQ(outage_durations[0], 0);
    EXPECT_EQ(resilience_hrs_max, 17);
    EXPECT_EQ(outage_durations[17], 17);
    EXPECT_NEAR(pdf_of_surviving[0], 0.756, 1e-3);
    EXPECT_NEAR(pdf_of_surviving[1], 0.0314, 1e-3);

    auto batt_power = data_vtab->as_vector_ssc_number_t("batt_power");
    auto power_max = *std::max_element(batt_power.begin(), batt_power.end());
    EXPECT_NEAR(power_max, 167.40, 1e-2);

    std::vector<size_t> max_indices;
    for (size_t i = 0; i < batt_power.size(); i++){
        if (power_max - batt_power[i] < 0.1)
            max_indices.push_back(i);
    }
    EXPECT_EQ(max_indices.size(), 25);
    EXPECT_EQ(max_indices[0], 2743);

    auto batt_q0 = data_vtab->as_vector_ssc_number_t("batt_q0");
    auto cap_max = *std::max_element(batt_q0.begin(), batt_q0.end());
    EXPECT_NEAR(cap_max, 11540, 10) << "Cap max should be 95% SOC";

    max_indices.clear();
    for (size_t i = 0; i < batt_q0.size(); i++){
        if (cap_max - batt_q0[i] < 0.01)
            max_indices.push_back(i);
    }
    EXPECT_EQ(max_indices[0], 2);
}

TEST_F(CMBattery_cmod_battery, ResilienceMetricsFullLoadLifetime){
    int nyears = 3;
    auto data_vtab = static_cast<var_table*>(data);
    data_vtab->assign("crit_load", data_vtab->as_vector_ssc_number_t("load"));
    data_vtab->assign("system_use_lifetime_output", 1);
    data_vtab->assign("analysis_period", nyears);
    data_vtab->assign("gen", var_data(data_vtab->as_array("gen", nullptr), 8760 * nyears));
    data_vtab->assign("batt_replacement_option", 0);
    data_vtab->assign("run_resiliency_calcs", 1);

    int errors = run_module(data, "battery");
    EXPECT_FALSE(errors);

    auto resilience_hours = data_vtab->as_vector_ssc_number_t("resilience_hrs");
    double resilience_hrs_min = data_vtab->as_number("resilience_hrs_min");
    double resilience_hrs_max = data_vtab->as_number("resilience_hrs_max");
    double resilience_hrs_avg = data_vtab->as_number("resilience_hrs_avg");
    auto outage_durations = data_vtab->as_vector_ssc_number_t("outage_durations");
    auto pdf_of_surviving = data_vtab->as_vector_ssc_number_t("pdf_of_surviving");
    double avg_critical_load = data_vtab->as_double("avg_critical_load");

    EXPECT_EQ(resilience_hours[0], 0); // Max current restrictions prevent this battery from meeting the outage until day 2 (hr 46)
    EXPECT_EQ(resilience_hours[46], 5);
    EXPECT_NEAR(avg_critical_load, 674.536, 0.1);
    EXPECT_NEAR(resilience_hrs_avg, 1.076, 0.01);
    EXPECT_EQ(resilience_hrs_min, 0);
    EXPECT_EQ(outage_durations[0], 0);
    EXPECT_EQ(resilience_hrs_max, 17);
    EXPECT_EQ(outage_durations[17], 17);
    EXPECT_NEAR(pdf_of_surviving[0], 0.754, 1e-3);
    EXPECT_NEAR(pdf_of_surviving[1], 0.0314, 1e-3);

    auto batt_power = data_vtab->as_vector_ssc_number_t("batt_power");
    auto power_max = *std::max_element(batt_power.begin(), batt_power.end());
    EXPECT_NEAR(power_max, 167.40, 1e-2);

    std::vector<size_t> max_indices;
    for (size_t i = 0; i < batt_power.size(); i++){
        if (power_max - batt_power[i] < 0.1)
            max_indices.push_back(i);
    }
    EXPECT_EQ(max_indices[0], 2743);

    auto batt_q0 = data_vtab->as_vector_ssc_number_t("batt_q0");
    auto cap_max = *std::max_element(batt_q0.begin(), batt_q0.end());
    EXPECT_NEAR(cap_max, 11540, 10) << "Cap max should be 95% SOC";

    max_indices.clear();
    for (size_t i = 0; i < batt_q0.size(); i++){
        if (cap_max - batt_q0[i] < 0.01)
            max_indices.push_back(i);
    }
    EXPECT_EQ(max_indices[0], 2);
}

/// Test standalone battery compute modeule with merchant plant prices
TEST_F(CMBattery_cmod_battery_fom, MerchantPlantAutomatic) {

    // Run with fixed output
    ssc_number_t n_years;
    ssc_data_get_number(data, "analysis_period", &n_years);
    size_t n_lifetime = (size_t)(n_years) * 8760;

    int errors = run_module(data, "battery"); // Happy path - expect no errors
    EXPECT_FALSE(errors);

    if (!errors)
    {
        // roundtrip efficiency test will ensure that the battery cycled
        ssc_number_t roundtripEfficiency;
        ssc_data_get_number(data, "average_battery_roundtrip_efficiency", &roundtripEfficiency);
        EXPECT_NEAR(roundtripEfficiency, 87.47, 2);

        // test that lifetime output is achieved
        int n;
        calculated_array = ssc_data_get_array(data, "gen", &n);
        EXPECT_EQ(n_lifetime, (size_t)n);

        // test that battery was replaced at some point
        calculated_array = ssc_data_get_array(data, "batt_bank_replacement", &n);
        int replacements = std::accumulate(calculated_array, calculated_array + n, 0);

        EXPECT_EQ(replacements, 0);

        // test temperature
        double* arr = ssc_data_get_array(data, "batt_temperature", &n);
        auto temp_array = std::vector<double>(arr, arr + n);
        double max_temp = *std::max_element(temp_array.begin(), temp_array.end());
        EXPECT_NEAR(max_temp, 34.3, 1);
    }
}

/// Test standalone battery compute modeule with merchant plant price errors (ssc throws as expected)
TEST_F(CMBattery_cmod_battery_fom, MerchantPlantAutomaticDimensionError) {

    // Run with fixed output
    ssc_number_t n_years;
    ssc_data_get_number(data, "analysis_period", &n_years);
    size_t n_lifetime = (size_t)(n_years) * 8760;

    set_matrix(data, "mp_energy_market_revenue_single", mp_path, 1, 8760); // Swap the dimensions of the array (this is super easy to do by accident in PySAM)

    int errors = run_module(data, "battery"); // Expect an error
    EXPECT_TRUE(errors);

}
