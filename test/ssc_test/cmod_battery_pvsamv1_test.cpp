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

#include "cmod_battery_pvsamv1_test.h"

#include "../input_cases/weather_inputs.h"
#include "../input_cases/pvsamv1_battery_common_data.h"

void daily_battery_stats::compute(std::vector<ssc_number_t> batt_power_data) {
    size_t index = 0;
    size_t n = batt_power_data.size();
    int cycleState = 0; // -1 for charging, 1 for discharging;
    bool halfCycle = false;
    while (index < n) {
        int cycles = 0;
        for (size_t hour = 0; hour < 24 * steps_per_hour; hour++) {
            ssc_number_t currentPower = batt_power_data[index];

            if (std::abs(currentPower - 0) < 1e-7) {
                currentPower = 0;
            }

            if (currentPower < 0) {
                if (cycleState != -1) {
                    if (halfCycle) {
                        cycles++;
                        halfCycle = false;
                    }
                    else {
                        halfCycle = true;
                    }
                }
                cycleState = -1;
            }

            if (currentPower > 0) {
                if (cycleState != 1) {
                    if (halfCycle) {
                        cycles++;
                        halfCycle = false;
                    }
                    else {
                        halfCycle = true;
                    }
                }
                cycleState = 1;
            }

            index++;
        }
        if (cycles > peakCycles) {
            peakCycles = cycles;
        }
        avgCycles += cycles;

    }
    ssc_number_t days = n / 24.0 / steps_per_hour;
    avgCycles = avgCycles / days;
    peakKwDischarge = *std::max_element(batt_power_data.begin(), batt_power_data.end());
    peakKwCharge = *std::min_element(batt_power_data.begin(), batt_power_data.end());
}

TEST_F(CMPvsamv1BatteryIntegration_cmod_pvsamv1, TestDailyBatteryStats)
{
    // 48 hrs of battery data to test the compute function
    std::vector<ssc_number_t> batt_power_data = { 0, 1, 0, -1, 0, 2, 0, -2, 0, 3, -3, 4, -1, 6, -4, -1, 0, 0, 1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 2, 1, 0, 0, 0, -1, -1, -1 };
    EXPECT_EQ(batt_power_data.size(), 48);
    daily_battery_stats batt_stats = daily_battery_stats(batt_power_data);

    EXPECT_EQ(batt_stats.peakKwCharge, -4);
    EXPECT_EQ(batt_stats.peakKwDischarge, 6);
    EXPECT_EQ(batt_stats.peakCycles, 5);
    EXPECT_NEAR(batt_stats.avgCycles, 3, 0.1);
}

/// Test PVSAMv1 with all defaults and battery enabled with 4 automatic dispatch methods
TEST_F(CMPvsamv1BatteryIntegration_cmod_pvsamv1, ResidentialACBatteryModelIntegration)
{
    pvsamv_nofinancial_default(data);
    battery_data_default(data);

    std::map<std::string, double> pairs;
    pairs["en_batt"] = 1;
    pairs["batt_ac_or_dc"] = 1; //AC
    pairs["analysis_period"] = 1;
    set_array(data, "load", load_profile_path, 8760); // Load is required for peak shaving controllers

    ssc_number_t expectedEnergy[4] = { 8741, 8741, 8831, 8576 };
    ssc_number_t expectedBatteryChargeEnergy[4] = { 1442, 1443, 258, 2973 };
    ssc_number_t expectedBatteryDischargeEnergy[4] = { 1321, 1323, 233, 2715 };

    ssc_number_t peakKwCharge[4] = { -2.91, -2.66, -2.25, -3.30 };
    ssc_number_t peakKwDischarge[4] = { 1.39, 1.73, 0.97, 1.96 };
    ssc_number_t peakCycles[4] = { 1, 1, 1, 3 };
    ssc_number_t avgCycles[4] = { 1, 1, 0.4904, 1.0110 };

    // Test peak shaving look ahead, peak shaving look behind, and automated grid power target, and self-consumption. Others require additional input data
    for (int i = 0; i < 4; i++) {
        switch (i) {
            case 0:
                // Peak shaving, look ahead
                pairs["batt_dispatch_choice"] = 0;
                pairs["batt_dispatch_wf_forecast_choice"] = 0;
                pairs["batt_dispatch_load_forecast_choice"] = 0;
                break;
            case 1:
                // Peak shaving, look behind
                pairs["batt_dispatch_choice"] = 0;
                pairs["batt_dispatch_wf_forecast_choice"] = 1;
                pairs["batt_dispatch_load_forecast_choice"] = 1;
                break;
            case 2:
                // Input grid power targets
                pairs["batt_dispatch_choice"] = 1;
                pairs["batt_dispatch_wf_forecast_choice"] = 0;
                pairs["batt_dispatch_load_forecast_choice"] = 0;
                break;
            case 3:
                // Self-consumption dispatch, which is set internally to grid power targets with a constant target of zero
                pairs["batt_dispatch_choice"] = 5;
                pairs["batt_dispatch_wf_forecast_choice"] = 0;
                pairs["batt_dispatch_load_forecast_choice"] = 0;
                break;
        }

        int pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
        EXPECT_FALSE(pvsam_errors);

        if (!pvsam_errors)
        {
            ssc_number_t annual_energy;
            ssc_data_get_number(data, "annual_energy", &annual_energy);
            EXPECT_NEAR(annual_energy, expectedEnergy[i], m_error_tolerance_hi) << "Annual energy.";

            auto data_vtab = static_cast<var_table*>(data);
            auto annualChargeEnergy = data_vtab->as_vector_ssc_number_t("batt_annual_charge_energy");
            EXPECT_NEAR(annualChargeEnergy[0], expectedBatteryChargeEnergy[i], m_error_tolerance_hi) << "Battery annual charge energy.";

            auto annualDischargeEnergy = data_vtab->as_vector_ssc_number_t("batt_annual_discharge_energy");
            EXPECT_NEAR(annualDischargeEnergy[0], expectedBatteryDischargeEnergy[i], m_error_tolerance_hi) << "Battery annual discharge energy.";

            auto batt_power = data_vtab->as_vector_ssc_number_t("batt_power");
            daily_battery_stats batt_stats = daily_battery_stats(batt_power);

            EXPECT_NEAR(batt_stats.peakKwCharge, peakKwCharge[i], m_error_tolerance_lo);
            EXPECT_NEAR(batt_stats.peakKwDischarge, peakKwDischarge[i], m_error_tolerance_lo);
            EXPECT_NEAR(batt_stats.peakCycles, peakCycles[i], m_error_tolerance_lo);
            EXPECT_NEAR(batt_stats.avgCycles, avgCycles[i], 0.0001);
        }
    }
}

/// Test PVSAMv1 with all defaults and battery enabled with custom dispatch
TEST_F(CMPvsamv1BatteryIntegration_cmod_pvsamv1, ResidentialACDCBatteryModelIntegrationCustomDispatchSparse)
{
    pvsamv_nofinancial_default(data);
    battery_data_default(data);

    std::map<std::string, double> pairs;
    pairs["en_batt"] = 1;

    pairs["analysis_period"] = 1;
    set_array(data, "load", load_profile_path, 8760); // Load is required for peak shaving controllers
    pairs["batt_dispatch_choice"] = 2;
    set_array(data, "batt_custom_dispatch", custom_dispatch_residential_schedule, 8760);

    ssc_number_t expectedEnergy[2] = { 8854, 8854 };
    ssc_number_t expectedBatteryChargeEnergy[2] = { 4.6, 4.7 };
    ssc_number_t expectedBatteryDischargeEnergy[2] = { 0.76, 7.6 };

    ssc_number_t peakKwCharge[2] = { -2.8, -2.9 };
    ssc_number_t peakKwDischarge[2] = { 0.835, 0.836 };
    ssc_number_t peakCycles[2] = { 1, 1 };
    ssc_number_t avgCycles[2] = { 0.0027, 0.0027 };

    // Test both AC and DC using the same dispatch model
    for (int i = 0; i < 2; i++) {
        pairs["batt_ac_or_dc"] = i;

        int pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
        EXPECT_FALSE(pvsam_errors);

        if (!pvsam_errors)
        {
            ssc_number_t annual_energy;
            ssc_data_get_number(data, "annual_energy", &annual_energy);
            EXPECT_NEAR(annual_energy, expectedEnergy[i], m_error_tolerance_hi) << "Annual energy.";

            auto data_vtab = static_cast<var_table*>(data);
            auto annualChargeEnergy = data_vtab->as_vector_ssc_number_t("batt_annual_charge_energy");
            EXPECT_NEAR(annualChargeEnergy[0], expectedBatteryChargeEnergy[i], m_error_tolerance_hi) << "Battery annual charge energy.";

            auto annualDischargeEnergy = data_vtab->as_vector_ssc_number_t("batt_annual_discharge_energy");
            EXPECT_NEAR(annualDischargeEnergy[0], expectedBatteryDischargeEnergy[i], m_error_tolerance_hi) << "Battery annual discharge energy.";

            auto batt_power = data_vtab->as_vector_ssc_number_t("batt_power");
            daily_battery_stats batt_stats = daily_battery_stats(batt_power);

            EXPECT_NEAR(batt_stats.peakKwCharge, peakKwCharge[i], m_error_tolerance_lo);
            EXPECT_NEAR(batt_stats.peakKwDischarge, peakKwDischarge[i], m_error_tolerance_lo);
            EXPECT_NEAR(batt_stats.peakCycles, peakCycles[i], m_error_tolerance_lo);
            EXPECT_NEAR(batt_stats.avgCycles, avgCycles[i], 0.0001); // Runs once per year
        }
    }
}

/// Test PVSAMv1 with all defaults and battery enabled with custom dispatch
TEST_F(CMPvsamv1BatteryIntegration_cmod_pvsamv1, ResidentialACDCBatteryModelIntegrationCustomDispatchFull)
{
    pvsamv_nofinancial_default(data);
    battery_data_default(data);

    std::map<std::string, double> pairs;
    pairs["en_batt"] = 1;

    pairs["analysis_period"] = 1;
    set_array(data, "load", load_profile_path, 8760); // Load is required for peak shaving controllers
    pairs["batt_dispatch_choice"] = 2;
    set_array(data, "batt_custom_dispatch", custom_dispatch_residential_hourly_schedule, 8760);

    ssc_number_t expectedEnergy[2] = { 8847, 8821 };
    ssc_number_t expectedBatteryChargeEnergy[2] = { 396.1, 359.95 };
    ssc_number_t expectedBatteryDischargeEnergy[2] = { 395.95, 419.2 };

    ssc_number_t peakKwCharge[2] = { -0.47, -0.47 };
    ssc_number_t peakKwDischarge[2] = { 0.39, 0.41 };
    ssc_number_t peakCycles[2] = { 1, 1 };
    ssc_number_t avgCycles[2] = { 0.8, 0.7972 };

    // Test both AC and DC using the same dispatch model
    for (int i = 0; i < 2; i++) {
        pairs["batt_ac_or_dc"] = i;

        int pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
        EXPECT_FALSE(pvsam_errors);

        if (!pvsam_errors)
        {
            ssc_number_t annual_energy;
            ssc_data_get_number(data, "annual_energy", &annual_energy);
            EXPECT_NEAR(annual_energy, expectedEnergy[i], m_error_tolerance_hi) << "Annual energy for " << i;

            auto data_vtab = static_cast<var_table*>(data);
            auto annualChargeEnergy = data_vtab->as_vector_ssc_number_t("batt_annual_charge_energy");
            EXPECT_NEAR(annualChargeEnergy[0], expectedBatteryChargeEnergy[i], m_error_tolerance_hi) << "Battery annual charge energy for " << i;

            auto annualDischargeEnergy = data_vtab->as_vector_ssc_number_t("batt_annual_discharge_energy");
            EXPECT_NEAR(annualDischargeEnergy[0], expectedBatteryDischargeEnergy[i], m_error_tolerance_hi) << "Battery annual discharge energy for " << i;

            auto batt_power = data_vtab->as_vector_ssc_number_t("batt_power");
            daily_battery_stats batt_stats = daily_battery_stats(batt_power);

            EXPECT_NEAR(batt_stats.peakKwCharge, peakKwCharge[i], m_error_tolerance_lo);
            EXPECT_NEAR(batt_stats.peakKwDischarge, peakKwDischarge[i], m_error_tolerance_lo);
            EXPECT_NEAR(batt_stats.peakCycles, peakCycles[i], m_error_tolerance_lo);
            EXPECT_NEAR(batt_stats.avgCycles, avgCycles[i], 0.0001) << " Battery average cycles for " << i;
        }
    }
}

/// Test PVSAMv1 with all defaults and battery enabled with manual dispatch
TEST_F(CMPvsamv1BatteryIntegration_cmod_pvsamv1, ResidentialACDCBatteryModelIntegrationManualDispatch)
{
    pvsamv_nofinancial_default(data);
    battery_data_default(data);

    std::map<std::string, double> pairs;
    pairs["en_batt"] = 1;

    pairs["analysis_period"] = 1;
    set_array(data, "load", load_profile_path, 8760); // Load is required for peak shaving controllers
    pairs["batt_dispatch_choice"] = 3;

    ssc_number_t expectedEnergy[2] = { 8843, 8815 };
    ssc_number_t expectedBatteryChargeEnergy[2] = { 468, 488 };
    ssc_number_t expectedBatteryDischargeEnergy[2] = { 437, 446 };

    ssc_number_t peakKwCharge[2] = { -2.37, -2.27 };
    ssc_number_t peakKwDischarge[2] = { 1.31, 1.31 };
    ssc_number_t peakCycles[2] = { 2, 2 };
    ssc_number_t avgCycles[2] = { 0.7178, 0.7178 };

    // Test both AC and DC using the same dispatch model
    for (int i = 0; i < 2; i++) {
        pairs["batt_ac_or_dc"] = i;

        int pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
        EXPECT_FALSE(pvsam_errors);

        if (!pvsam_errors)
        {
            ssc_number_t annual_energy;
            ssc_data_get_number(data, "annual_energy", &annual_energy);
            EXPECT_NEAR(annual_energy, expectedEnergy[i], m_error_tolerance_hi) << "Annual energy.";

            auto data_vtab = static_cast<var_table*>(data);
            auto annualChargeEnergy = data_vtab->as_vector_ssc_number_t("batt_annual_charge_energy");
            EXPECT_NEAR(annualChargeEnergy[0], expectedBatteryChargeEnergy[i], m_error_tolerance_hi) << "Battery annual charge energy.";

            auto annualDischargeEnergy = data_vtab->as_vector_ssc_number_t("batt_annual_discharge_energy");
            EXPECT_NEAR(annualDischargeEnergy[0], expectedBatteryDischargeEnergy[i], m_error_tolerance_hi) << "Battery annual discharge energy.";

            auto batt_power = data_vtab->as_vector_ssc_number_t("batt_power");
            daily_battery_stats batt_stats = daily_battery_stats(batt_power);

            EXPECT_NEAR(batt_stats.peakKwCharge, peakKwCharge[i], m_error_tolerance_lo);
            EXPECT_NEAR(batt_stats.peakKwDischarge, peakKwDischarge[i], m_error_tolerance_lo);
            EXPECT_NEAR(batt_stats.peakCycles, peakCycles[i], m_error_tolerance_lo);
            EXPECT_NEAR(batt_stats.avgCycles, avgCycles[i], 0.0001) << " Battery average cycles for " << i;
        }
    }
}

/// Test PVSAMv1 with all defaults and DC battery enabled with 4 automatic dispatch methods
TEST_F(CMPvsamv1BatteryIntegration_cmod_pvsamv1, ResidentialDCBatteryModelIntegration)
{
    pvsamv_nofinancial_default(data);
    battery_data_default(data);

    std::map<std::string, double> pairs;
    pairs["en_batt"] = 1;
    pairs["batt_ac_or_dc"] = 0; //DC
    pairs["analysis_period"] = 1;
    set_array(data, "load", load_profile_path, 8760); // Load is required for peak shaving controllers

    ssc_number_t expectedEnergy[4] = { 8781, 8784, 8846, 8757 };
    ssc_number_t expectedBatteryChargeEnergy[4] = { 1412.75, 1414.89, 253.2, 2810.8 };
    ssc_number_t expectedBatteryDischargeEnergy[4] = { 1283.8, 1285.88, 226.3, 2692.5 };

    ssc_number_t peakKwCharge[4] = { -3.06, -2.91, -2.51, -3.32 };
    ssc_number_t peakKwDischarge[4] = { 1.40, 1.74, 0.967, 1.97 };
    ssc_number_t peakCycles[4] = { 1, 1, 1, 3 };
    ssc_number_t avgCycles[4] = { 1.0, 1.0, 0.4794, 1.0110 };

    ssc_number_t q_rel[4] = { 95.461, 95.426, 96.777, 92.489 };
    ssc_number_t cyc_avg[4] = { 35.022, 35.218, 12.381, 72.29 };

    // Test peak shaving look ahead, peak shaving look behind, and automated grid power target. Others require additional input data
    for (int i = 0; i < 4; i++) {
        switch (i) {
        case 0:
            // Peak shaving, look ahead
            pairs["batt_dispatch_choice"] = 0;
            pairs["batt_dispatch_wf_forecast_choice"] = 0;
            pairs["batt_dispatch_load_forecast_choice"] = 0;
            break;
        case 1:
            // Peak shaving, look behind
            pairs["batt_dispatch_choice"] = 0;
            pairs["batt_dispatch_wf_forecast_choice"] = 1;
            pairs["batt_dispatch_load_forecast_choice"] = 1;
            break;
        case 2:
            // Input grid power targets
            pairs["batt_dispatch_choice"] = 1;
            pairs["batt_dispatch_wf_forecast_choice"] = 0;
            pairs["batt_dispatch_load_forecast_choice"] = 0;
            break;
        case 3:
            // Self-consumption
            pairs["batt_dispatch_choice"] = 5;
            pairs["batt_dispatch_wf_forecast_choice"] = 0;
            pairs["batt_dispatch_load_forecast_choice"] = 0;
            break;
        }

        int pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
        EXPECT_FALSE(pvsam_errors);

        if (!pvsam_errors)
        {
            ssc_number_t annual_energy;
            ssc_data_get_number(data, "annual_energy", &annual_energy);
            EXPECT_NEAR(annual_energy, expectedEnergy[i], m_error_tolerance_hi) << "Annual energy.";

            auto data_vtab = static_cast<var_table*>(data);
            auto annualChargeEnergy = data_vtab->as_vector_ssc_number_t("batt_annual_charge_energy");
            EXPECT_NEAR(annualChargeEnergy[0], expectedBatteryChargeEnergy[i], m_error_tolerance_hi) << "Battery annual charge energy.";

            auto annualDischargeEnergy = data_vtab->as_vector_ssc_number_t("batt_annual_discharge_energy");
            EXPECT_NEAR(annualDischargeEnergy[0], expectedBatteryDischargeEnergy[i], m_error_tolerance_hi) << "Battery annual discharge energy.";

            auto batt_power = data_vtab->as_vector_ssc_number_t("batt_power");
            daily_battery_stats batt_stats = daily_battery_stats(batt_power);

            EXPECT_NEAR(batt_stats.peakKwCharge, peakKwCharge[i], m_error_tolerance_lo);
            EXPECT_NEAR(batt_stats.peakKwDischarge, peakKwDischarge[i], m_error_tolerance_lo);
            EXPECT_NEAR(batt_stats.peakCycles, peakCycles[i], m_error_tolerance_lo);
            EXPECT_NEAR(batt_stats.avgCycles, avgCycles[i], 0.0001);

            auto batt_q_rel = data_vtab->as_vector_ssc_number_t("batt_capacity_percent");
            auto batt_cyc_avg = data_vtab->as_vector_ssc_number_t("batt_DOD_cycle_average");
            EXPECT_NEAR(batt_q_rel.back(), q_rel[i], 2e-2) << " with dispatch mode " << i;
            EXPECT_NEAR(batt_cyc_avg.back(), cyc_avg[i], 1.0) << " with dispatch mode " << i;
        }
    }
}

TEST_F(CMPvsamv1BatteryIntegration_cmod_pvsamv1, LCOS_test_singleowner)
{
    pvsamv1_pv_defaults(data);
    pvsamv1_battery_defaults(data);
    grid_and_rate_defaults(data);
    singleowner_defaults(data);

    int pvsam_errors = run_pvsam1_battery_ppa(data);
    EXPECT_FALSE(pvsam_errors);

    ssc_number_t lcos_real;
    ssc_data_get_number(data, "lcos_real", &lcos_real);
    EXPECT_NEAR(lcos_real, 23.41, 0.1);

}

TEST_F(CMPvsamv1BatteryIntegration_cmod_pvsamv1, LCOS_test_levpartflip)
{
    pvsamv1_pv_defaults(data);
    pvsamv1_battery_defaults(data);
    grid_and_rate_defaults(data);
    singleowner_defaults(data);

    int pvsam_errors = run_pvsam1_battery_fom(data);
    EXPECT_FALSE(pvsam_errors);

    ssc_number_t lcos_real;
    ssc_data_get_number(data, "lcos_real", &lcos_real);
    EXPECT_NEAR(lcos_real, 23.43, 0.1);
}

TEST_F(CMPvsamv1BatteryIntegration_cmod_pvsamv1, LCOS_test_cashloan)
{
    pvsamv1_pv_defaults(data);
    pvsamv1_battery_defaults(data);
    grid_and_rate_defaults(data);
    ssc_data_set_number(data, "en_electricity_rates", 1);
    commercial_multiarray_default(data);

    int pvsam_errors = run_pvsam1_battery_cashloan(data);
    EXPECT_FALSE(pvsam_errors);

    ssc_number_t lcos_real;
    ssc_data_get_number(data, "lcos_real", &lcos_real);
    EXPECT_NEAR(lcos_real, 583.83, 0.1);
}

/// Test PVSAMv1 with all defaults and battery enabled with 3 automatic dispatch methods
TEST_F(CMPvsamv1BatteryIntegration_cmod_pvsamv1, PPA_ACBatteryModelIntegration)
{
    pvsamv1_pv_defaults(data);
    pvsamv1_battery_defaults(data);
    grid_and_rate_defaults(data);
    singleowner_defaults(data);

    ssc_number_t expectedEnergy[3] = { 37839193, 37838301, 37308139 };
    ssc_number_t expectedBatteryChargeEnergy[3] = { 14779, 24265, 14779 }; // No rate model means battery use is low
    ssc_number_t expectedBatteryDischargeEnergy[3] = { 14808, 23415, 14808 };

    ssc_number_t peakKwCharge[3] = { -1040.2, -1051.5, -1051.5 };
    ssc_number_t peakKwDischarge[3] = { 967.5, 969.5, 969.5 };
    ssc_number_t peakCycles[3] = { 1, 1, 1 };
    ssc_number_t avgCycles[3] = { 0.0039, 0.0042, 0.003 };

    ssc_data_set_number(data, "batt_dispatch_choice", 0);
    // Test economoic dispatch look ahead, economoic dispatch look behind. Others require additional input data
    for (int i = 0; i < 2; i++) {
        ssc_data_set_number(data, "batt_dispatch_wf_forecast_choice", i);

        int pvsam_errors = run_pvsam1_battery_ppa(data);
        EXPECT_FALSE(pvsam_errors);

        if (!pvsam_errors)
        {
            ssc_number_t annual_energy;
            ssc_data_get_number(data, "annual_energy", &annual_energy);
            EXPECT_NEAR(annual_energy, expectedEnergy[i], m_error_tolerance_hi) << "Annual energy for " << i;

            auto data_vtab = static_cast<var_table*>(data);
            auto annualChargeEnergy = data_vtab->as_vector_ssc_number_t("batt_annual_charge_energy");
            EXPECT_NEAR(annualChargeEnergy[1], expectedBatteryChargeEnergy[i], m_error_tolerance_hi) << "Battery annual charge energy for " << i;

            auto annualDischargeEnergy = data_vtab->as_vector_ssc_number_t("batt_annual_discharge_energy");
            EXPECT_NEAR(annualDischargeEnergy[1], expectedBatteryDischargeEnergy[i], m_error_tolerance_hi) << "Battery annual discharge energy for " << i;

            auto batt_power = data_vtab->as_vector_ssc_number_t("batt_power");
            daily_battery_stats batt_stats = daily_battery_stats(batt_power);

            EXPECT_NEAR(batt_stats.peakKwCharge, peakKwCharge[i], m_error_tolerance_hi);
            EXPECT_NEAR(batt_stats.peakKwDischarge, peakKwDischarge[i], m_error_tolerance_hi);
            EXPECT_NEAR(batt_stats.peakCycles, peakCycles[i], m_error_tolerance_lo);
            EXPECT_NEAR(batt_stats.avgCycles, avgCycles[i], 0.0001);

            // test temperature
            auto temp_array = data_vtab->as_vector_ssc_number_t("batt_temperature");
            double max_temp = *std::max_element(temp_array.begin(), temp_array.end());
            EXPECT_LT(max_temp, 26);
        }
    }
}

/// Test PVSAMv1 with all defaults and battery enabled with manual dispatch and PPA financial model
TEST_F(CMPvsamv1BatteryIntegration_cmod_pvsamv1, PPA_ManualDispatchBatteryModelIntegration)
{
    pvsamv1_pv_defaults(data);
    pvsamv1_battery_defaults(data);
    grid_and_rate_defaults(data);
    singleowner_defaults(data);

    ssc_number_t expectedEnergy = 37715607;
    ssc_number_t expectedBatteryChargeEnergy = 1299674;
    ssc_number_t expectedBatteryDischargeEnergy = 1176096;

    ssc_number_t peakKwCharge = -1051.9;
    ssc_number_t peakKwDischarge = 848.6;
    ssc_number_t peakCycles = 1;
    ssc_number_t avgCycles = 1;

    ssc_data_set_number(data, "batt_dispatch_choice", 3);

    // Modify utility rate to Salt River Project Super Peak
    ssc_number_t p_ur_ec_sched_weekday_srp[288] = { 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6 };
    ssc_data_set_matrix(data, "ur_ec_sched_weekday", p_ur_ec_sched_weekday_srp, 12, 24);
    ssc_number_t p_ur_ec_sched_weekend_srp[288] = { 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6 };
    ssc_data_set_matrix(data, "ur_ec_sched_weekend", p_ur_ec_sched_weekend_srp, 12, 24);
    ssc_number_t p_ur_ec_tou_mat_srp[36] = { 1, 1, 9.9999999999999998e+37, 0, 0.2969, 0, 2, 1, 9.9999999999999998e+37, 0, 0.081900000000000001, 0, 3, 1, 9.9999999999999998e+37, 0, 0.34989999999999999, 0, 4, 1, 9.9999999999999998e+37, 0, 0.083599999999999994, 0, 5, 1, 9.9999999999999998e+37, 0, 0.123, 0, 6, 1, 9.9999999999999998e+37, 0, 0.074999999999999997, 0 };
    ssc_data_set_matrix(data, "ur_ec_tou_mat", p_ur_ec_tou_mat_srp, 6, 6);

    int pvsam_errors = run_pvsam1_battery_ppa(data);
    EXPECT_FALSE(pvsam_errors);

    if (!pvsam_errors)
    {
        ssc_number_t annual_energy;
        ssc_data_get_number(data, "annual_energy", &annual_energy);
        EXPECT_NEAR(annual_energy, expectedEnergy, m_error_tolerance_hi) << "Annual energy.";

        auto data_vtab = static_cast<var_table*>(data);
        auto annualChargeEnergy = data_vtab->as_vector_ssc_number_t("batt_annual_charge_energy");
        EXPECT_NEAR(annualChargeEnergy[1], expectedBatteryChargeEnergy, 10) << "Battery annual charge energy.";

        auto annualDischargeEnergy = data_vtab->as_vector_ssc_number_t("batt_annual_discharge_energy");
        EXPECT_NEAR(annualDischargeEnergy[1], expectedBatteryDischargeEnergy, 10) << "Battery annual discharge energy.";

        auto batt_power = data_vtab->as_vector_ssc_number_t("batt_power");
        daily_battery_stats batt_stats = daily_battery_stats(batt_power);

        EXPECT_NEAR(batt_stats.peakKwCharge, peakKwCharge, m_error_tolerance_lo);
        EXPECT_NEAR(batt_stats.peakKwDischarge, peakKwDischarge, m_error_tolerance_lo);
        EXPECT_NEAR(batt_stats.peakCycles, peakCycles, m_error_tolerance_lo);
        EXPECT_NEAR(batt_stats.avgCycles, avgCycles, 0.0001);
    }
}

/// Test PVSAMv1 with all defaults and DC battery enabled with custom dispatch and PPA financial model
TEST_F(CMPvsamv1BatteryIntegration_cmod_pvsamv1, PPA_CustomDispatchBatteryModelDCIntegrationSparse)
{
    pvsamv1_pv_defaults(data);
    pvsamv1_battery_defaults(data);
    grid_and_rate_defaults(data);
    singleowner_defaults(data);

    ssc_number_t expectedEnergy = 37839912;
    ssc_number_t expectedBatteryChargeEnergy = 2040;
    ssc_number_t expectedBatteryDischargeEnergy = 3254.;

    ssc_number_t peakKwCharge = -992.86;
    ssc_number_t peakKwDischarge = 946.98;
    ssc_number_t peakCycles = 1;
    ssc_number_t avgCycles = 0.0027;

    ssc_data_set_number(data, "batt_dispatch_choice", 2);
    ssc_data_set_number(data, "batt_ac_or_dc", 0);
    set_array(data, "batt_custom_dispatch", custom_dispatch_singleowner_schedule, 8760);

    int pvsam_errors = run_pvsam1_battery_ppa(data);
    EXPECT_FALSE(pvsam_errors);

    if (!pvsam_errors)
    {
        ssc_number_t annual_energy;
        ssc_data_get_number(data, "annual_energy", &annual_energy);
        EXPECT_NEAR(annual_energy, expectedEnergy, m_error_tolerance_hi) << "Annual energy.";

        auto data_vtab = static_cast<var_table*>(data);
        auto annualChargeEnergy = data_vtab->as_vector_ssc_number_t("batt_annual_charge_energy");
        EXPECT_NEAR(annualChargeEnergy[1], expectedBatteryChargeEnergy, m_error_tolerance_hi) << "Battery annual charge energy.";

        auto annualDischargeEnergy = data_vtab->as_vector_ssc_number_t("batt_annual_discharge_energy");
        EXPECT_NEAR(annualDischargeEnergy[1], expectedBatteryDischargeEnergy, m_error_tolerance_hi) << "Battery annual discharge energy.";

        auto batt_power = data_vtab->as_vector_ssc_number_t("batt_power");
        daily_battery_stats batt_stats = daily_battery_stats(batt_power);

        EXPECT_NEAR(batt_stats.peakKwCharge, peakKwCharge, m_error_tolerance_lo);
        EXPECT_NEAR(batt_stats.peakKwDischarge, peakKwDischarge, m_error_tolerance_lo);
        EXPECT_NEAR(batt_stats.peakCycles, peakCycles, m_error_tolerance_lo);
        EXPECT_NEAR(batt_stats.avgCycles, avgCycles, 0.0001); // Runs once per year
    }

}

/// Test PVSAMv1 with all defaults and DC battery enabled with custom dispatch and PPA financial model
TEST_F(CMPvsamv1BatteryIntegration_cmod_pvsamv1, PPA_CustomDispatchBatteryModelDCIntegrationFull)
{
    pvsamv1_pv_defaults(data);
    pvsamv1_battery_defaults(data);
    grid_and_rate_defaults(data);
    singleowner_defaults(data);

    //ssc_number_t expectedEnergy = 37264228;
    ssc_number_t expectedEnergy = 37783998;
    ssc_number_t expectedBatteryChargeEnergy = 414366;
    ssc_number_t expectedBatteryDischargeEnergy = 348966;
    ssc_number_t roundtripEfficiency = 80.6;

    ssc_number_t peakKwCharge = -841.39;
    ssc_number_t peakKwDischarge = 652.0;
    ssc_number_t peakCycles = 3;
    ssc_number_t avgCycles = 1.1836;

    ssc_data_set_number(data, "batt_dispatch_choice", 2);
    ssc_data_set_number(data, "batt_ac_or_dc", 0);
    set_array(data, "batt_custom_dispatch", custom_dispatch_singleowner_hourly_schedule, 8760);

    int pvsam_errors = run_pvsam1_battery_ppa(data);
    EXPECT_FALSE(pvsam_errors);

    if (!pvsam_errors)
    {
        ssc_number_t annual_energy;
        ssc_data_get_number(data, "annual_energy", &annual_energy);
        EXPECT_NEAR(annual_energy, expectedEnergy, m_error_tolerance_hi) << "Annual energy.";

        auto data_vtab = static_cast<var_table*>(data);
        auto annualChargeEnergy = data_vtab->as_vector_ssc_number_t("batt_annual_charge_energy");
        EXPECT_NEAR(annualChargeEnergy[1], expectedBatteryChargeEnergy, m_error_tolerance_hi) << "Battery annual charge energy.";

        auto annualDischargeEnergy = data_vtab->as_vector_ssc_number_t("batt_annual_discharge_energy");
        EXPECT_NEAR(annualDischargeEnergy[1], expectedBatteryDischargeEnergy, m_error_tolerance_hi) << "Battery annual discharge energy.";

        EXPECT_NEAR(data_vtab->lookup("average_battery_roundtrip_efficiency")->num[0], roundtripEfficiency, m_error_tolerance_hi) << "Battery roundtrip efficiency.";

        auto batt_power = data_vtab->as_vector_ssc_number_t("batt_power");
        daily_battery_stats batt_stats = daily_battery_stats(batt_power);

        EXPECT_NEAR(batt_stats.peakKwCharge, peakKwCharge, m_error_tolerance_lo);
        EXPECT_NEAR(batt_stats.peakKwDischarge, peakKwDischarge, m_error_tolerance_lo);
        EXPECT_NEAR(batt_stats.peakCycles, peakCycles, m_error_tolerance_lo);
        EXPECT_NEAR(batt_stats.avgCycles, avgCycles, 0.0001);
    }

}

/// Test PVSAMv1 clipping with multiple subarrays
TEST_F(CMPvsamv1BatteryIntegration_cmod_pvsamv1, CommercialMultipleSubarrayBatteryIntegration)
{
    commercial_multiarray_default(data); // AC connected

    std::map<std::string, double> pairs;
    pairs["analysis_period"] = 1;

    ssc_number_t expectedEnergy = 543888;
    ssc_number_t expectedBatteryChargeEnergy = 785.1;
    ssc_number_t expectedBatteryDischargeEnergy = 715.1;
    ssc_number_t expectedClipLoss = 545.8;

    ssc_number_t peakKwCharge = -9.93;
    ssc_number_t peakKwDischarge = 1.24;
    ssc_number_t peakCycles = 1;
    ssc_number_t avgCycles = 1;

    // Test peak shaving look ahead
    int pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    EXPECT_FALSE(pvsam_errors);

    if (!pvsam_errors)
    {
        ssc_number_t annual_energy;
        ssc_data_get_number(data, "annual_energy", &annual_energy);
        EXPECT_NEAR(annual_energy, expectedEnergy, m_error_tolerance_hi) << "Annual energy.";

        auto data_vtab = static_cast<var_table*>(data);
        auto annualChargeEnergy = data_vtab->as_vector_ssc_number_t("batt_annual_charge_energy");
        EXPECT_NEAR(annualChargeEnergy[0], expectedBatteryChargeEnergy, m_error_tolerance_hi) << "Battery annual charge energy.";

        auto annualDischargeEnergy = data_vtab->as_vector_ssc_number_t("batt_annual_discharge_energy");
        EXPECT_NEAR(annualDischargeEnergy[0], expectedBatteryDischargeEnergy, m_error_tolerance_hi) << "Battery annual discharge energy.";

        auto dcInverterLoss = data_vtab->as_vector_ssc_number_t("dc_invmppt_loss");
        ssc_number_t totalLoss = 0;
        for (int i = 0; i < dcInverterLoss.size(); i++) {
            totalLoss += dcInverterLoss[i];
        }
        EXPECT_NEAR(totalLoss, expectedClipLoss, m_error_tolerance_lo);

        auto batt_power = data_vtab->as_vector_ssc_number_t("batt_power");
        daily_battery_stats batt_stats = daily_battery_stats(batt_power);

        EXPECT_NEAR(batt_stats.peakKwCharge, peakKwCharge, m_error_tolerance_lo);
        EXPECT_NEAR(batt_stats.peakKwDischarge, peakKwDischarge, m_error_tolerance_lo);
        EXPECT_NEAR(batt_stats.peakCycles, peakCycles, m_error_tolerance_lo);
        EXPECT_NEAR(batt_stats.avgCycles, avgCycles, 0.0001);

        auto batt_q_rel = data_vtab->as_vector_ssc_number_t("batt_capacity_percent");
        auto batt_cyc_avg = data_vtab->as_vector_ssc_number_t("batt_DOD_cycle_average");
        EXPECT_NEAR(batt_q_rel.back(), 99.314, 2e-2);
        EXPECT_NEAR(batt_cyc_avg.back(), 9.394, m_error_tolerance_lo);
    }

}

/// Test Clipping forecast and dispatch
TEST_F(CMPvsamv1BatteryIntegration_cmod_pvsamv1, ClippingForecastTest1_DC_Dispatch)
{
    commercial_multiarray_default(data);

    std::map<std::string, double> pairs;
    pairs["analysis_period"] = 1;
    pairs["batt_ac_or_dc"] = 0;

    ssc_number_t expectedEnergy = 543485;
    ssc_number_t expectedBatteryChargeEnergy = 929;
    ssc_number_t expectedBatteryDischargeEnergy = 343.96;
    ssc_number_t expectedClipLoss = 545.8;

    ssc_number_t peakKwCharge = -9.04;
    ssc_number_t peakKwDischarge = 1.1;
    ssc_number_t peakCycles = 1;
    ssc_number_t avgCycles = 1;

    int pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    EXPECT_FALSE(pvsam_errors);

    if (!pvsam_errors)
    {
        ssc_number_t annual_energy;
        ssc_data_get_number(data, "annual_energy", &annual_energy);
        EXPECT_NEAR(annual_energy, expectedEnergy, m_error_tolerance_hi) << "Annual energy.";

        auto data_vtab = static_cast<var_table*>(data);
        auto annualChargeEnergy = data_vtab->as_vector_ssc_number_t("batt_annual_charge_energy");
        EXPECT_NEAR(annualChargeEnergy[0], expectedBatteryChargeEnergy, m_error_tolerance_hi) << "Battery annual charge energy.";

        auto annualDischargeEnergy = data_vtab->as_vector_ssc_number_t("batt_annual_discharge_energy");
        EXPECT_NEAR(annualDischargeEnergy[0], expectedBatteryDischargeEnergy, m_error_tolerance_hi) << "Battery annual discharge energy.";

        auto dcInverterLoss = data_vtab->as_vector_ssc_number_t("dc_invmppt_loss");
        ssc_number_t totalLoss = 0;
        for (int i = 0; i < dcInverterLoss.size(); i++) {
            totalLoss += dcInverterLoss[i];
        }
        EXPECT_NEAR(totalLoss, expectedClipLoss, m_error_tolerance_lo);

        auto batt_power = data_vtab->as_vector_ssc_number_t("batt_power");
        daily_battery_stats batt_stats = daily_battery_stats(batt_power);

        EXPECT_NEAR(batt_stats.peakKwCharge, peakKwCharge, m_error_tolerance_lo);
        EXPECT_NEAR(batt_stats.peakKwDischarge, peakKwDischarge, m_error_tolerance_lo);
        EXPECT_NEAR(batt_stats.peakCycles, peakCycles, m_error_tolerance_lo);
        EXPECT_NEAR(batt_stats.avgCycles, avgCycles, 0.0001);
    }

}

TEST_F(CMPvsamv1BatteryIntegration_cmod_pvsamv1, ClippingForecastTest2_DC_Dispatch_w_forecast)
{
    commercial_multiarray_default(data);

    std::map<std::string, double> pairs;
    pairs["analysis_period"] = 1;
    pairs["batt_ac_or_dc"] = 0;
    set_array(data, "batt_pv_clipping_forecast", clipping_forecast, 8760);
    // TODO: consider changing the dispatch algorithm for this and the previous test. Peak shaving does not use clipped energy, meaning the results are the same for this and the previous test

    ssc_number_t expectedEnergy = 543485;
    ssc_number_t expectedBatteryChargeEnergy = 929;
    ssc_number_t expectedBatteryDischargeEnergy = 343.96;
    ssc_number_t expectedClipLoss = 545.8;

    ssc_number_t peakKwCharge = -9.04;
    ssc_number_t peakKwDischarge = 1.1;
    ssc_number_t peakCycles = 1;
    ssc_number_t avgCycles = 1;

    int pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    EXPECT_FALSE(pvsam_errors);

    if (!pvsam_errors)
    {
        ssc_number_t annual_energy;
        ssc_data_get_number(data, "annual_energy", &annual_energy);
        EXPECT_NEAR(annual_energy, expectedEnergy, m_error_tolerance_hi) << "Annual energy.";

        auto data_vtab = static_cast<var_table*>(data);
        auto annualChargeEnergy = data_vtab->as_vector_ssc_number_t("batt_annual_charge_energy");
        EXPECT_NEAR(annualChargeEnergy[0], expectedBatteryChargeEnergy, m_error_tolerance_hi) << "Battery annual charge energy.";

        auto annualDischargeEnergy = data_vtab->as_vector_ssc_number_t("batt_annual_discharge_energy");
        EXPECT_NEAR(annualDischargeEnergy[0], expectedBatteryDischargeEnergy, m_error_tolerance_hi) << "Battery annual discharge energy.";

        auto dcInverterLoss = data_vtab->as_vector_ssc_number_t("dc_invmppt_loss");
        ssc_number_t totalLoss = 0;
        for (int i = 0; i < dcInverterLoss.size(); i++) {
            totalLoss += dcInverterLoss[i];
        }
        EXPECT_NEAR(totalLoss, expectedClipLoss, m_error_tolerance_lo);

        auto batt_power = data_vtab->as_vector_ssc_number_t("batt_power");
        daily_battery_stats batt_stats = daily_battery_stats(batt_power);

        EXPECT_NEAR(batt_stats.peakKwCharge, peakKwCharge, m_error_tolerance_lo);
        EXPECT_NEAR(batt_stats.peakKwDischarge, peakKwDischarge, m_error_tolerance_lo);
        EXPECT_NEAR(batt_stats.peakCycles, peakCycles, m_error_tolerance_lo);
        EXPECT_NEAR(batt_stats.avgCycles, avgCycles, 0.0001);
    }

}

/// Test PVSAMv1 with all defaults and DC battery enabled with custom dispatch and PPA financial model
TEST_F(CMPvsamv1BatteryIntegration_cmod_pvsamv1, PPA_CustomDispatchBatteryModelDCIntegrationFullSubhourly)
{
    pvsamv1_pv_defaults(data);
    pvsamv1_battery_defaults(data);
    grid_and_rate_defaults(data);
    singleowner_defaults(data);

    ssc_number_t expectedEnergy = 37252473;
    ssc_number_t expectedBatteryChargeEnergy = 430570;
    ssc_number_t expectedBatteryDischargeEnergy = 349127;
    ssc_number_t roundtripEfficiency = 80.6;

    ssc_number_t peakKwCharge = -833.74;
    ssc_number_t peakKwDischarge = 651.7;
    ssc_number_t peakCycles = 3;
    ssc_number_t avgCycles = 1.1829;

    ssc_data_set_number(data, "batt_dispatch_choice", 2);
    ssc_data_set_number(data, "batt_ac_or_dc", 0);
    set_array(data, "batt_custom_dispatch", custom_dispatch_singleowner_subhourly_schedule, 8760 * 4);
    set_array(data, "batt_room_temperature_celsius", subhourly_batt_temps, 8760 * 4);
    set_array(data, "dispatch_factors_ts", subhourly_dispatch_factors, 8760 * 4);
    ssc_data_set_string(data, "solar_resource_file", subhourly_weather_file);

    int pvsam_errors = run_pvsam1_battery_ppa(data);
    EXPECT_FALSE(pvsam_errors);

    if (!pvsam_errors)
    {
        double tol = .05;
        ssc_number_t annual_energy;
        ssc_data_get_number(data, "annual_energy", &annual_energy);
        EXPECT_NEAR(annual_energy, expectedEnergy, expectedEnergy * tol) << "Annual energy.";

        auto data_vtab = static_cast<var_table*>(data);
        auto annualChargeEnergy = data_vtab->as_vector_ssc_number_t("batt_annual_charge_energy");
        EXPECT_NEAR(annualChargeEnergy[1], expectedBatteryChargeEnergy, expectedBatteryChargeEnergy * tol) << "Battery annual charge energy.";

        auto annualDischargeEnergy = data_vtab->as_vector_ssc_number_t("batt_annual_discharge_energy");
        EXPECT_NEAR(annualDischargeEnergy[1], expectedBatteryDischargeEnergy, expectedBatteryDischargeEnergy * tol) << "Battery annual discharge energy.";

        EXPECT_NEAR(data_vtab->lookup("average_battery_roundtrip_efficiency")->num[0], roundtripEfficiency, m_error_tolerance_hi) << "Battery roundtrip efficiency.";

        auto batt_power = data_vtab->as_vector_ssc_number_t("batt_power");
        daily_battery_stats batt_stats = daily_battery_stats(batt_power, 4);

        EXPECT_NEAR(batt_stats.peakKwCharge, peakKwCharge, std::abs(peakKwCharge * 0.01));
        EXPECT_NEAR(batt_stats.peakKwDischarge, peakKwDischarge, peakKwDischarge * 0.01);
        EXPECT_NEAR(batt_stats.peakCycles, peakCycles, m_error_tolerance_lo);
        EXPECT_NEAR(batt_stats.avgCycles, avgCycles, 0.05);

        auto batt_q_rel = data_vtab->as_vector_ssc_number_t("batt_capacity_percent");
        auto batt_cyc_avg = data_vtab->as_vector_ssc_number_t("batt_DOD_cycle_average");
        EXPECT_NEAR(batt_q_rel.back(), 85.875, 2e-2);
        EXPECT_NEAR(batt_cyc_avg.back(), 21.81, m_error_tolerance_lo);
    }

}

TEST_F(CMPvsamv1BatteryIntegration_cmod_pvsamv1, ResidentialDCBatteryModelPriceSignalDispatch)
{
    pvsamv_nofinancial_default(data);
    battery_data_default(data);
    setup_residential_utility_rates(data);

    std::map<std::string, double> pairs;
    pairs["en_batt"] = 1;
    pairs["batt_meter_position"] = 0; // Behind the meter
    pairs["batt_ac_or_dc"] = 0; //DC
    pairs["analysis_period"] = 1;
    set_array(data, "load", load_profile_path, 8760); // Load is required for peak shaving controllers

    ssc_number_t expectedEnergy = 8844;
    ssc_number_t expectedBatteryChargeEnergy = 390.9;
    ssc_number_t expectedBatteryDischargeEnergy = 360.2;

    ssc_number_t peakKwCharge = -3.601;
    ssc_number_t peakKwDischarge = 1.99;
    ssc_number_t peakCycles = 1;
    ssc_number_t avgCycles = 0.320;

    pairs["batt_dispatch_choice"] = 4;
    pairs["batt_dispatch_auto_can_clipcharge"] = 1;

    int pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    EXPECT_FALSE(pvsam_errors);

    if (!pvsam_errors)
    {
        ssc_number_t annual_energy;
        ssc_data_get_number(data, "annual_energy", &annual_energy);
        EXPECT_NEAR(annual_energy, expectedEnergy, m_error_tolerance_hi) << "Annual energy.";

        auto data_vtab = static_cast<var_table*>(data);
        auto annualChargeEnergy = data_vtab->as_vector_ssc_number_t("batt_annual_charge_energy");
        EXPECT_NEAR(annualChargeEnergy[0], expectedBatteryChargeEnergy, m_error_tolerance_hi) << "Battery annual charge energy.";

        auto annualDischargeEnergy = data_vtab->as_vector_ssc_number_t("batt_annual_discharge_energy");
        EXPECT_NEAR(annualDischargeEnergy[0], expectedBatteryDischargeEnergy, m_error_tolerance_hi) << "Battery annual discharge energy.";

        auto batt_power = data_vtab->as_vector_ssc_number_t("batt_power");
        daily_battery_stats batt_stats = daily_battery_stats(batt_power);

        EXPECT_NEAR(batt_stats.peakKwCharge, peakKwCharge, m_error_tolerance_lo);
        EXPECT_NEAR(batt_stats.peakKwDischarge, peakKwDischarge, m_error_tolerance_lo);
        EXPECT_NEAR(batt_stats.peakCycles, peakCycles, m_error_tolerance_lo);
        EXPECT_NEAR(batt_stats.avgCycles, avgCycles, 0.010); // Increased tolerance due to https://github.com/NREL/ssc/issues/614

        auto batt_q_rel = data_vtab->as_vector_ssc_number_t("batt_capacity_percent");
        auto batt_cyc_avg = data_vtab->as_vector_ssc_number_t("batt_DOD_cycle_average");
        EXPECT_NEAR(batt_q_rel.back(), 97.241, 5e-2);
        EXPECT_NEAR(batt_cyc_avg.back(), 27.49, 1.0); // High tolerance due to ~ 1% dispatch difference between linux and windows. Tighten in the future by improving the algorithm.
    }
}

/// Test PVSAMv1 with an interconnection limit to ensure powerflow calcluations are working properly
TEST_F(CMPvsamv1BatteryIntegration_cmod_pvsamv1, ResidentialACBatteryModelInterconnectionLimits)
{
    pvsamv_nofinancial_default(data);
    battery_data_default(data);

    std::map<std::string, double> pairs;
    pairs["en_batt"] = 1;
    pairs["batt_ac_or_dc"] = 1; //AC
    pairs["analysis_period"] = 1;
    set_array(data, "load", load_profile_path, 8760); // Load is required for peak shaving controllers
    pairs["batt_dispatch_choice"] = 0; // Peak shaving
    pairs["batt_dispatch_wf_forecast_choice"] = 0; // Look ahead

    pairs["enable_interconnection_limit"] = true;
    double interconnection_limit = 1.0; // kWac
    pairs["grid_interconnection_limit_kwac"] = interconnection_limit;

    ssc_number_t expectedEnergy = 8741;
    ssc_number_t expectedBatteryChargeEnergy = 1442;
    ssc_number_t expectedBatteryDischargeEnergy = 1321;

    ssc_number_t peakKwCharge = -2.91;
    ssc_number_t peakKwDischarge = 1.39;
    ssc_number_t peakCycles = 1;
    ssc_number_t avgCycles = 1;

    int pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    EXPECT_FALSE(pvsam_errors);

    if (!pvsam_errors)
    {
        ssc_number_t annual_energy;
        ssc_data_get_number(data, "annual_energy", &annual_energy);
        EXPECT_NEAR(annual_energy, expectedEnergy, m_error_tolerance_hi) << "Annual energy.";

        auto data_vtab = static_cast<var_table*>(data);
        auto annualChargeEnergy = data_vtab->as_vector_ssc_number_t("batt_annual_charge_energy");
        EXPECT_NEAR(annualChargeEnergy[0], expectedBatteryChargeEnergy, m_error_tolerance_hi) << "Battery annual charge energy.";

        auto annualDischargeEnergy = data_vtab->as_vector_ssc_number_t("batt_annual_discharge_energy");
        EXPECT_NEAR(annualDischargeEnergy[0], expectedBatteryDischargeEnergy, m_error_tolerance_hi) << "Battery annual discharge energy.";

        auto batt_power = data_vtab->as_vector_ssc_number_t("batt_power");
        daily_battery_stats batt_stats = daily_battery_stats(batt_power);

        EXPECT_NEAR(batt_stats.peakKwCharge, peakKwCharge, m_error_tolerance_lo);
        EXPECT_NEAR(batt_stats.peakKwDischarge, peakKwDischarge, m_error_tolerance_lo);
        EXPECT_NEAR(batt_stats.peakCycles, peakCycles, m_error_tolerance_lo);
        EXPECT_NEAR(batt_stats.avgCycles, avgCycles, 0.0001);

        std::vector<double> grid_power = data_vtab->as_vector_double("grid_power");
        EXPECT_NEAR(*std::max_element(grid_power.begin(), grid_power.end()), interconnection_limit, 0.0001);

        std::vector<double> pv_to_grid_power = data_vtab->as_vector_double("system_to_grid");
        EXPECT_NEAR(*std::max_element(pv_to_grid_power.begin(), pv_to_grid_power.end()), interconnection_limit, 0.0001);

        std::vector<double> interconnection_loss = data_vtab->as_vector_double("interconnection_loss");
        std::vector<double> gen = data_vtab->as_vector_double("gen");
        std::vector<double> gen_without_battery = data_vtab->as_vector_double("gen_without_battery");
        std::vector<double> pv_to_load = data_vtab->as_vector_double("system_to_load");
        std::vector<double> batt_to_load = data_vtab->as_vector_double("batt_to_load");
        std::vector<double> grid_to_load = data_vtab->as_vector_double("grid_to_load");
        std::vector<double> pv_to_battery = data_vtab->as_vector_double("system_to_batt");

        for (int i = 0; i < grid_power.size(); i++) {
            EXPECT_NEAR(gen[i], grid_power[i] + pv_to_load[i] + batt_to_load[i] + grid_to_load[i] + interconnection_loss[i], 0.001) << " at step " << i;
            EXPECT_NEAR(grid_power[i] + grid_to_load[i], pv_to_grid_power[i], 0.001) << " at step " << i;
            EXPECT_NEAR(gen_without_battery[i], grid_power[i] + grid_to_load[i] + pv_to_load[i] + pv_to_battery[i] + interconnection_loss[i], 0.001) << " at step " << i;
        }
    }

}

TEST_F(CMPvsamv1BatteryIntegration_cmod_pvsamv1, ResidentialACBatteryModelInterconnectionLimitsWAvailabilityLoss)
{
    pvsamv_nofinancial_default(data);
    battery_data_default(data);

    std::map<std::string, double> pairs;
    pairs["en_batt"] = 1;
    pairs["batt_ac_or_dc"] = 1; //AC
    pairs["analysis_period"] = 1;
    set_array(data, "load", load_profile_path, 8760); // Load is required for peak shaving controllers
    pairs["batt_dispatch_choice"] = 0; // Peak shaving
    pairs["batt_dispatch_wf_forecast_choice"] = 0; // Look ahead

    pairs["enable_interconnection_limit"] = true;
    double interconnection_limit = 1.0; // kWac
    pairs["grid_interconnection_limit_kwac"] = interconnection_limit;
    pairs["system_use_lifetime_output"] = 1;
    pairs["save_full_lifetime_variables"] = 1;
    pairs["en_ac_lifetime_losses"] = true;
    ssc_number_t degradation[1] = { 0.5 };
    ssc_data_set_array(data, "dc_degradation", degradation, 1);
    ssc_number_t losses[365] = { 10 };
    ssc_data_set_array(data, "ac_lifetime_losses", losses, 365);

    ssc_number_t expectedEnergy = 8740;
    ssc_number_t expectedBatteryChargeEnergy = 1442;
    ssc_number_t expectedBatteryDischargeEnergy = 1321;

    ssc_number_t peakKwCharge = -2.78;
    ssc_number_t peakKwDischarge = 1.39;
    ssc_number_t peakCycles = 1;
    ssc_number_t avgCycles = 1;

    int pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    EXPECT_FALSE(pvsam_errors);

    if (!pvsam_errors)
    {
        ssc_number_t annual_energy;
        ssc_data_get_number(data, "annual_energy", &annual_energy);
        EXPECT_NEAR(annual_energy, expectedEnergy, m_error_tolerance_hi) << "Annual energy.";

        auto data_vtab = static_cast<var_table*>(data);
        auto annualChargeEnergy = data_vtab->as_vector_ssc_number_t("batt_annual_charge_energy");
        EXPECT_NEAR(annualChargeEnergy[0], expectedBatteryChargeEnergy, m_error_tolerance_hi) << "Battery annual charge energy.";

        auto annualDischargeEnergy = data_vtab->as_vector_ssc_number_t("batt_annual_discharge_energy");
        EXPECT_NEAR(annualDischargeEnergy[0], expectedBatteryDischargeEnergy, m_error_tolerance_hi) << "Battery annual discharge energy.";

        auto batt_power = data_vtab->as_vector_ssc_number_t("batt_power");
        daily_battery_stats batt_stats = daily_battery_stats(batt_power);

        EXPECT_NEAR(batt_stats.peakKwCharge, peakKwCharge, m_error_tolerance_lo);
        EXPECT_NEAR(batt_stats.peakKwDischarge, peakKwDischarge, m_error_tolerance_lo);
        EXPECT_NEAR(batt_stats.peakCycles, peakCycles, m_error_tolerance_lo);
        EXPECT_NEAR(batt_stats.avgCycles, avgCycles, 0.0001);

        std::vector<double> grid_power = data_vtab->as_vector_double("grid_power");
        EXPECT_NEAR(*std::max_element(grid_power.begin(), grid_power.end()), interconnection_limit, 0.0001);

        std::vector<double> pv_to_grid_power = data_vtab->as_vector_double("system_to_grid");
        EXPECT_NEAR(*std::max_element(pv_to_grid_power.begin(), pv_to_grid_power.end()), interconnection_limit, 0.0001);

        std::vector<double> interconnection_loss = data_vtab->as_vector_double("interconnection_loss");
        std::vector<double> gen = data_vtab->as_vector_double("gen");
        std::vector<double> gen_without_battery = data_vtab->as_vector_double("gen_without_battery");
        std::vector<double> pv_to_load = data_vtab->as_vector_double("system_to_load");
        std::vector<double> batt_to_load = data_vtab->as_vector_double("batt_to_load");
        std::vector<double> grid_to_load = data_vtab->as_vector_double("grid_to_load");
        std::vector<double> pv_to_battery = data_vtab->as_vector_double("system_to_batt");
        std::vector<double> performance_loss = data_vtab->as_vector_double("ac_perf_adj_loss");
        std::vector<double> ac_lifetime_loss = data_vtab->as_vector_double("ac_lifetime_loss");

        for (int i = 0; i < grid_power.size(); i++) {
            double ac_losses = performance_loss[i] + ac_lifetime_loss[i];
            EXPECT_NEAR(gen[i], grid_power[i] + pv_to_load[i] + batt_to_load[i] + grid_to_load[i] + interconnection_loss[i], 0.001) << " at step " << i;
            // The ac losses can either come from the power delivered to load or grid, so it is hard to write universal rules that apply to every step
        }
    }

}

TEST_F(CMPvsamv1BatteryIntegration_cmod_pvsamv1, ResidentialACBatteryModelGridOutage)
{
    pvsamv_nofinancial_default(data);
    battery_data_default(data);

    std::map<std::string, double> pairs;
    pairs["en_batt"] = 1;
    pairs["batt_ac_or_dc"] = 1; //AC
    pairs["analysis_period"] = 1;
    set_array(data, "load", load_profile_path, 8760); // Load is required for peak shaving controllers
    set_array(data, "crit_load", load_profile_path, 8760); // 100% critical load
    pairs["batt_dispatch_choice"] = 0; // Peak shaving
    pairs["batt_dispatch_wf_forecast_choice"] = 0; // Look ahead

    pairs["system_use_lifetime_output"] = 1;
    pairs["save_full_lifetime_variables"] = 1;
    ssc_number_t degradation[1] = { 0.5 };
    ssc_data_set_array(data, "dc_degradation", degradation, 1);
    ssc_number_t grid_outage[8760];
    std::fill_n(grid_outage, 8760, 1);
    ssc_data_set_array(data, "grid_outage", grid_outage, 8760);

    ssc_number_t expectedEnergy = 8521.00;
    ssc_number_t expectedBatteryChargeEnergy = 3290.77;
    ssc_number_t expectedBatteryDischargeEnergy = 2974.91;
    ssc_number_t expectedCritLoadUnmet = 502.003;

    ssc_number_t peakKwCharge = -3.4;
    ssc_number_t peakKwDischarge = 1.964;
    ssc_number_t peakCycles = 3;
    ssc_number_t avgCycles = 1.0109;

    int pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    EXPECT_FALSE(pvsam_errors);

    if (!pvsam_errors)
    {
        ssc_number_t annual_energy;
        ssc_data_get_number(data, "annual_energy", &annual_energy);
        EXPECT_NEAR(annual_energy, expectedEnergy, m_error_tolerance_hi) << "Annual energy.";

        auto data_vtab = static_cast<var_table*>(data);
        auto annualChargeEnergy = data_vtab->as_vector_ssc_number_t("batt_annual_charge_energy");
        EXPECT_NEAR(annualChargeEnergy[0], expectedBatteryChargeEnergy, m_error_tolerance_hi) << "Battery annual charge energy.";

        auto annualDischargeEnergy = data_vtab->as_vector_ssc_number_t("batt_annual_discharge_energy");
        EXPECT_NEAR(annualDischargeEnergy[0], expectedBatteryDischargeEnergy, m_error_tolerance_hi) << "Battery annual discharge energy.";

        ssc_number_t crit_load_unmet;
        ssc_data_get_number(data, "annual_crit_load_unmet", &crit_load_unmet);
        EXPECT_NEAR(crit_load_unmet, expectedCritLoadUnmet, m_error_tolerance_lo);

        auto batt_power = data_vtab->as_vector_ssc_number_t("batt_power");
        daily_battery_stats batt_stats = daily_battery_stats(batt_power);

        EXPECT_NEAR(batt_stats.peakKwCharge, peakKwCharge, m_error_tolerance_lo);
        EXPECT_NEAR(batt_stats.peakKwDischarge, peakKwDischarge, m_error_tolerance_lo);
        EXPECT_NEAR(batt_stats.peakCycles, peakCycles, m_error_tolerance_lo);
        EXPECT_NEAR(batt_stats.avgCycles, avgCycles, 0.0001);

        std::vector<double> grid_power = data_vtab->as_vector_double("grid_power");
        EXPECT_NEAR(*std::max_element(grid_power.begin(), grid_power.end()), 0.0, 0.0001);

        std::vector<double> pv_to_grid_power = data_vtab->as_vector_double("system_to_grid");
        EXPECT_NEAR(*std::max_element(pv_to_grid_power.begin(), pv_to_grid_power.end()), 0.0, 0.0001);
    }

}

/* - Commented out until we better define the effects of availability loss on the battery targeted for the Fall 2022 release
TEST_F(CMPvsamv1BatteryIntegration_cmod_pvsamv1, ResidentialACBatteryModelGridOutageWAvailabilityLoss)
{
    pvsamv_nofinancial_default(data);
    battery_data_default(data);

    std::map<std::string, double> pairs;
    pairs["en_batt"] = 1;
    pairs["batt_ac_or_dc"] = 1; //AC
    pairs["analysis_period"] = 1;
    set_array(data, "load", load_profile_path, 8760); // Load is required for peak shaving controllers
    set_array(data, "crit_load", load_profile_path, 8760); // 100% critical load
    pairs["batt_dispatch_choice"] = 0; // Peak shaving
    pairs["batt_dispatch_wf_forecast_choice"] = 0; // Look ahead

    pairs["system_use_lifetime_output"] = 1;
    pairs["save_full_lifetime_variables"] = 1;
    pairs["en_ac_lifetime_losses"] = true;
    ssc_number_t degradation[1] = { 0.5 };
    ssc_data_set_array(data, "dc_degradation", degradation, 1);
    ssc_number_t losses[365];
    std::fill_n(losses, 365, 10);
    ssc_data_set_array(data, "ac_lifetime_losses", losses, 365);
    ssc_number_t grid_outage[8760];
    std::fill_n(grid_outage, 8760, 1);
    ssc_data_set_array(data, "grid_outage", grid_outage, 8760);

    ssc_number_t expectedEnergy = 7690.65;
    ssc_number_t expectedBatteryChargeEnergy = 433.73;
    ssc_number_t expectedBatteryDischargeEnergy = 138.40;
    ssc_number_t expectedCritLoadUnmet = 3590.85;

    ssc_number_t peakKwCharge = -2.91;
    ssc_number_t peakKwDischarge = 1.39;
    ssc_number_t peakCycles = 1;
    ssc_number_t avgCycles = 1;

    int pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    EXPECT_FALSE(pvsam_errors);

    if (!pvsam_errors)
    {
        ssc_number_t annual_energy;
        ssc_data_get_number(data, "annual_energy", &annual_energy);
        EXPECT_NEAR(annual_energy, expectedEnergy, m_error_tolerance_hi) << "Annual energy.";

        auto data_vtab = static_cast<var_table*>(data);
        auto annualChargeEnergy = data_vtab->as_vector_ssc_number_t("batt_annual_charge_energy");
        EXPECT_NEAR(annualChargeEnergy[0], expectedBatteryChargeEnergy, m_error_tolerance_hi) << "Battery annual charge energy.";

        auto annualDischargeEnergy = data_vtab->as_vector_ssc_number_t("batt_annual_discharge_energy");
        EXPECT_NEAR(annualDischargeEnergy[0], expectedBatteryDischargeEnergy, m_error_tolerance_hi) << "Battery annual discharge energy.";

        ssc_number_t crit_load_unmet;
        ssc_data_get_number(data, "annual_crit_load_unmet", &crit_load_unmet);
        EXPECT_NEAR(crit_load_unmet, expectedCritLoadUnmet, m_error_tolerance_lo);

        auto batt_power = data_vtab->as_vector_ssc_number_t("batt_power");
        daily_battery_stats batt_stats = daily_battery_stats(batt_power);

        EXPECT_NEAR(batt_stats.peakKwCharge, peakKwCharge, m_error_tolerance_lo);
        EXPECT_NEAR(batt_stats.peakKwDischarge, peakKwDischarge, m_error_tolerance_lo);
        EXPECT_NEAR(batt_stats.peakCycles, peakCycles, m_error_tolerance_lo);
        EXPECT_NEAR(batt_stats.avgCycles, avgCycles, 0.0001);

        std::vector<double> grid_power = data_vtab->as_vector_double("grid_power");
        EXPECT_NEAR(*std::max_element(grid_power.begin(), grid_power.end()), 0.0, 0.0001);

        std::vector<double> pv_to_grid_power = data_vtab->as_vector_double("system_to_grid");
        EXPECT_NEAR(*std::max_element(pv_to_grid_power.begin(), pv_to_grid_power.end()), 0.0, 0.0001);
    }

}
*/
