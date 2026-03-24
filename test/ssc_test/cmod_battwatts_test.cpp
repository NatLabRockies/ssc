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

#include "../input_cases/code_generator_utilities.h"
#include "vartab.h"
#include "cmod_battwatts_test.h"
#include "battwatts_cases.h"
#include "lib_util.h"

TEST_F(CMBattwatts_cmod_battwatts, ResilienceMetricsHalfLoad){
    CreateData(1);

    auto ssc_dat = static_cast<ssc_data_t>(&data);
    int errors = run_module(ssc_dat, "battwatts");
    EXPECT_FALSE(errors);

    auto resilience_hours = data.as_vector_ssc_number_t("resilience_hrs");
    double resilience_hrs_min = data.as_number("resilience_hrs_min");
    double resilience_hrs_max = data.as_number("resilience_hrs_max");
    double resilience_hrs_avg = data.as_number("resilience_hrs_avg");
    auto outage_durations = data.as_vector_ssc_number_t("outage_durations");
    auto pdf_of_surviving = data.as_vector_ssc_number_t("pdf_of_surviving");
    double avg_critical_load = data.as_double("avg_critical_load"); // Load met

    EXPECT_EQ(resilience_hours[0], 14);
    EXPECT_EQ(resilience_hours[1], 15);
    EXPECT_NEAR(avg_critical_load, 7.98, 0.1);
    EXPECT_NEAR(resilience_hrs_avg, 31.95, 0.01);
    EXPECT_EQ(resilience_hrs_min, 14);
    EXPECT_EQ(outage_durations[0], 14);
    EXPECT_EQ(resilience_hrs_max, 32);
    EXPECT_EQ(outage_durations[16], 30);
    EXPECT_NEAR(pdf_of_surviving[0], 0.00068, 1e-3);
    EXPECT_NEAR(pdf_of_surviving[1], 0.00034, 1e-3);

}

TEST_F(CMBattwatts_cmod_battwatts, ResilienceMetricsHalfLoadLifetime){
    CreateData(2);

    auto ssc_dat = static_cast<ssc_data_t>(&data);
    int errors = run_module(ssc_dat, "battwatts");
    EXPECT_FALSE(errors);

    auto resilience_hours = data.as_vector_ssc_number_t("resilience_hrs");
    double resilience_hrs_min = data.as_number("resilience_hrs_min");
    double resilience_hrs_max = data.as_number("resilience_hrs_max");
    double resilience_hrs_avg = data.as_number("resilience_hrs_avg");
    auto outage_durations = data.as_vector_ssc_number_t("outage_durations");
    auto pdf_of_surviving = data.as_vector_ssc_number_t("pdf_of_surviving");
    auto cdf_of_surviving = data.as_vector_ssc_number_t("cdf_of_surviving");
    double avg_critical_load = data.as_double("avg_critical_load");

    EXPECT_EQ(resilience_hours[0], 14);
    EXPECT_EQ(resilience_hours[1], 15);
    EXPECT_NEAR(avg_critical_load, 8.026, 0.1);
    EXPECT_NEAR(resilience_hrs_avg, 31.975, 0.01);
    EXPECT_EQ(resilience_hrs_min, 14);
    EXPECT_EQ(resilience_hrs_max, 32);
    EXPECT_EQ(outage_durations[0], 14);
    EXPECT_EQ(outage_durations[16], 30);
    EXPECT_NEAR(pdf_of_surviving[0], 5.707e-05, 1e-5);
    EXPECT_NEAR(pdf_of_surviving[1], 0.000171, 1e-5);

}

TEST_F(CMBattwatts_cmod_battwatts, ResidentialDefaults) {
    auto ssc_dat = static_cast<ssc_data_t>(&data);
    pvwatts_pv_defaults(ssc_dat);
    simple_battery_data(ssc_dat);

    int errors = run_module(ssc_dat, "battwatts");
    EXPECT_FALSE(errors);

    double charge_percent = data.as_number("batt_system_charge_percent");
    EXPECT_NEAR(charge_percent, 96.52, 0.1);

    auto batt_power_data = data.as_vector_ssc_number_t("batt_power");
    ssc_number_t peakKwDischarge = *std::max_element(batt_power_data.begin(), batt_power_data.end());
    ssc_number_t peakKwCharge = *std::min_element(batt_power_data.begin(), batt_power_data.end());

    EXPECT_NEAR(peakKwDischarge, 2.16, 0.1);
    EXPECT_NEAR(peakKwCharge, -3.04, 0.1);

    auto batt_voltage = data.as_vector_ssc_number_t("batt_voltage");
    ssc_number_t peakVoltage = *std::max_element(batt_voltage.begin(), batt_voltage.end());
    EXPECT_NEAR(peakVoltage, 579.3, 0.1);

    auto cycles = data.as_vector_ssc_number_t("batt_cycles");
    ssc_number_t maxCycles = *std::max_element(cycles.begin(), cycles.end());
    EXPECT_NEAR(maxCycles, 477, 0.1);
}

TEST_F(CMBattwatts_cmod_battwatts, ResidentialDefaultsLeadAcid) {
    auto ssc_dat = static_cast<ssc_data_t>(&data);
    pvwatts_pv_defaults(ssc_dat);
    simple_battery_data(ssc_dat);

    // Set lead acid
    ssc_data_set_number(ssc_dat, "batt_simple_chemistry", 0);

    int errors = run_module(ssc_dat, "battwatts");
    EXPECT_FALSE(errors);

    double charge_percent = data.as_number("batt_system_charge_percent");
    EXPECT_NEAR(charge_percent, 96.94, 0.1);

    auto batt_power_data = data.as_vector_ssc_number_t("batt_power");
    ssc_number_t peakKwDischarge = *std::max_element(batt_power_data.begin(), batt_power_data.end());
    ssc_number_t peakKwCharge = *std::min_element(batt_power_data.begin(), batt_power_data.end());

    EXPECT_NEAR(peakKwDischarge, 1.97, 0.1);
    EXPECT_NEAR(peakKwCharge, -2.57, 0.1);

    auto batt_voltage = data.as_vector_ssc_number_t("batt_voltage");
    ssc_number_t peakVoltage = *std::max_element(batt_voltage.begin(), batt_voltage.end());
    EXPECT_NEAR(peakVoltage, 61.43, 0.1);

    auto cycles = data.as_vector_ssc_number_t("batt_cycles");
    ssc_number_t maxCycles = *std::max_element(cycles.begin(), cycles.end());
    EXPECT_NEAR(maxCycles, 477, 0.1);
}

TEST_F(CMBattwatts_cmod_battwatts, NoPV) {
    auto ssc_dat = static_cast<ssc_data_t>(&data);
    pvwatts_pv_defaults(ssc_dat);
    simple_battery_data(ssc_dat);

    std::vector<double> ac(8760, 0);
    data.assign("ac", ac);

    int errors = run_module(ssc_dat, "battwatts");
    EXPECT_FALSE(errors);

    double charge_percent = data.as_number("batt_system_charge_percent");
    EXPECT_NEAR(charge_percent, 0.0, 0.1);

    auto batt_power_data = data.as_vector_ssc_number_t("batt_power");
    ssc_number_t peakKwDischarge = *std::max_element(batt_power_data.begin(), batt_power_data.end());
    ssc_number_t peakKwCharge = *std::min_element(batt_power_data.begin(), batt_power_data.end());

    EXPECT_NEAR(peakKwDischarge, 1.035, 0.1);
    EXPECT_NEAR(peakKwCharge, -0.7, 0.1);

    auto batt_voltage = data.as_vector_ssc_number_t("batt_voltage");
    ssc_number_t peakVoltage = *std::max_element(batt_voltage.begin(), batt_voltage.end());
    EXPECT_NEAR(peakVoltage, 573.5, 0.1);

    auto cycles = data.as_vector_ssc_number_t("batt_cycles");
    ssc_number_t maxCycles = *std::max_element(cycles.begin(), cycles.end());
    EXPECT_NEAR(maxCycles, 522, 3);
}
