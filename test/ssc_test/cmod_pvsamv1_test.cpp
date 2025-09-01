/*
BSD 3-Clause License

Copyright Alliance for Sustainable Energy, LLC. See also https://github.com/NREL/ssc/blob/develop/LICENSE


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
#include <cmath>

#include "cmod_pvsamv1_eqns.h"
#include "cmod_pvsamv1_test.h"

#include "../input_cases/weather_inputs.h"

/// Test PVSAMv1 with all defaults and no-financial model
TEST_F(CMPvsamv1PowerIntegration_cmod_pvsamv1, DefaultNoFinancialModel) {

    int pvsam_errors = run_module(data, "pvsamv1");

    EXPECT_FALSE(pvsam_errors);
    if (!pvsam_errors)
    {
        ssc_number_t annual_energy;
        ssc_data_get_number(data, "annual_energy", &annual_energy);
        EXPECT_NEAR(annual_energy, 8837.3, m_error_tolerance_hi) << "Annual energy.";

        ssc_number_t capacity_factor;
        ssc_data_get_number(data, "capacity_factor", &capacity_factor);
        EXPECT_NEAR(capacity_factor, 21.4, m_error_tolerance_lo) << "Capacity factor";

        ssc_number_t kwh_per_kw;
        ssc_data_get_number(data, "kwh_per_kw", &kwh_per_kw);
        EXPECT_NEAR(kwh_per_kw, 1883, m_error_tolerance_hi) << "Energy yield";

        ssc_number_t performance_ratio;
        ssc_data_get_number(data, "performance_ratio", &performance_ratio);
        EXPECT_NEAR(performance_ratio, 0.79, m_error_tolerance_lo) << "Energy yield";
    }
}

TEST_F(CMPvsamv1PowerIntegration_cmod_pvsamv1, DefaultNoFinancialModel_Lac) {


    std::map<std::string, double> pairs;
    pairs["cec_temp_corr_mode"] = 1;
    pairs["cec_lacunarity_length"] = 1;
    pairs["cec_lacunarity_h"] = 1;
    pairs["cec_ground_clearance_height"] = 0.5;
    int pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);

    EXPECT_FALSE(pvsam_errors);
    
}

/// Run PVSAMv1 with all defaults and lifetime mode for no-financial model
TEST_F(CMPvsamv1PowerIntegration_cmod_pvsamv1, DefaultLifetimeNoFinancialModel) {

    std::map<std::string, double> pairs;
    pairs["system_use_lifetime_output"] = 1;
    pairs["save_full_lifetime_variables"] = 1;
    pairs["analysis_period"] = 25;

    double dc_degradation[25];
    for (size_t i = 0; i < 25; i++) {
        dc_degradation[i] = 0.5;
    }

    ssc_data_set_array(data, "dc_degradation", (ssc_number_t*)dc_degradation, 25);

    int pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);

    EXPECT_FALSE(pvsam_errors);
    if (!pvsam_errors)
    {
        ssc_number_t annual_energy;
        ssc_data_get_number(data, "annual_energy", &annual_energy);
        EXPECT_NEAR(annual_energy, 8837.3, m_error_tolerance_hi) << "Annual energy.";

        ssc_number_t capacity_factor;
        ssc_data_get_number(data, "capacity_factor", &capacity_factor);
        EXPECT_NEAR(capacity_factor, 21.4, m_error_tolerance_lo) << "Capacity factor";

        ssc_number_t kwh_per_kw;
        ssc_data_get_number(data, "kwh_per_kw", &kwh_per_kw);
        EXPECT_NEAR(kwh_per_kw, 1883, m_error_tolerance_hi) << "Energy yield";

        ssc_number_t performance_ratio;
        ssc_data_get_number(data, "performance_ratio", &performance_ratio);
        EXPECT_NEAR(performance_ratio, 0.79, m_error_tolerance_lo) << "Energy yield";
    }
}


/// Test PVSAMv1 with all defaults and residential financial model
TEST_F(CMPvsamv1PowerIntegration_cmod_pvsamv1, DefaultResidentialModel)
{

    int pvsam_errors = pvsam_residential_pheonix(data);
    EXPECT_FALSE(pvsam_errors);

    if (!pvsam_errors)
    {
        ssc_number_t annual_energy;
        ssc_data_get_number(data, "annual_energy", &annual_energy);
        EXPECT_NEAR(annual_energy, 8837.3, m_error_tolerance_hi) << "Annual energy.";

        ssc_number_t capacity_factor;
        ssc_data_get_number(data, "capacity_factor", &capacity_factor);
        EXPECT_NEAR(capacity_factor, 21.4, m_error_tolerance_lo) << "Capacity factor";

        ssc_number_t kwh_per_kw;
        ssc_data_get_number(data, "kwh_per_kw", &kwh_per_kw);
        EXPECT_NEAR(kwh_per_kw, 1883, m_error_tolerance_hi) << "Energy yield";

        ssc_number_t performance_ratio;
        ssc_data_get_number(data, "performance_ratio", &performance_ratio);
        EXPECT_NEAR(performance_ratio, 0.79, m_error_tolerance_lo) << "Performance ratio";

        ssc_number_t lcoe_nom;
        ssc_data_get_number(data, "lcoe_nom", &lcoe_nom);
        EXPECT_NEAR(lcoe_nom, 12.05, m_error_tolerance_lo) << "Levelized COE (nominal)";

        ssc_number_t lcoe_real;
        ssc_data_get_number(data, "lcoe_real", &lcoe_real);
        EXPECT_NEAR(lcoe_real, 11.61, m_error_tolerance_lo) << "Levelized COE (real)";

        ssc_number_t utility_bill_wo_sys_year1;
        ssc_data_get_number(data, "utility_bill_wo_sys_year1", &utility_bill_wo_sys_year1);
        EXPECT_NEAR(utility_bill_wo_sys_year1, 973, m_error_tolerance_hi) << "Electricity bill without system (year 1)";

        ssc_number_t utility_bill_w_sys_year1;
        ssc_data_get_number(data, "utility_bill_w_sys_year1", &utility_bill_w_sys_year1);
        EXPECT_NEAR(utility_bill_w_sys_year1, 122, m_error_tolerance_hi) << "Electricity bill with system (year 1)";

        ssc_number_t savings_year1;
        ssc_data_get_number(data, "savings_year1", &savings_year1);
        EXPECT_NEAR(savings_year1, 851, m_error_tolerance_hi) << "Net savings with system (year 1)";

        ssc_number_t npv;
        ssc_data_get_number(data, "npv", &npv);
        EXPECT_NEAR(npv, -358.2, m_error_tolerance_hi) << "Net present value";

        ssc_number_t payback;
        ssc_data_get_number(data, "payback", &payback);
        EXPECT_TRUE(std::isnan(payback)) << "Payback period"; 

        ssc_number_t discounted_payback;
        ssc_data_get_number(data, "discounted_payback", &discounted_payback);
        EXPECT_TRUE(std::isnan(discounted_payback)); // ssc issue 616 - discounted to year 0 instead of year 1, so discounted payback is greater than the analysis period (=NaN)

        ssc_number_t adjusted_installed_cost;
        ssc_data_get_number(data, "adjusted_installed_cost", &adjusted_installed_cost);
        EXPECT_NEAR(adjusted_installed_cost, 13758, m_error_tolerance_hi) << "Net capital cost";

        ssc_number_t first_cost;
        ssc_data_get_number(data, "first_cost", &first_cost);
        EXPECT_NEAR(first_cost, 0, m_error_tolerance_lo) << "Equity";

        ssc_number_t loan_amount;
        ssc_data_get_number(data, "loan_amount", &loan_amount);
        EXPECT_NEAR(loan_amount, 13758, m_error_tolerance_hi) << "Debt";
    }
}

TEST_F(CMPvsamv1PowerIntegration_cmod_pvsamv1, LossAdjustmentLifetime) {
    std::map<std::string, double> pairs;
    pairs["system_use_lifetime_output"] = 1;
    pairs["save_full_lifetime_variables"] = 1;
    pairs["analysis_period"] = 2;

    double dc_degradation[25];
    for (size_t i = 0; i < 25; i++) {
        dc_degradation[i] = 0.5;
    }

    ssc_data_set_array(data, "dc_degradation", (ssc_number_t*)dc_degradation, 25);
   
    
    int pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
   
    //Single Value
    double timeindex[1];
    for (int i = 0; i < 1; i++)
        timeindex[i] = 100;

    ssc_data_set_number(data, "adjust_constant", 0.0);
    ssc_data_set_number(data, "adjust_en_timeindex", 1.0);
    ssc_data_set_array(data, "adjust_timeindex", timeindex, 1);


    pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    ssc_number_t annual_energy;
    ssc_data_get_number(data, "annual_energy", &annual_energy);
    EXPECT_NEAR(annual_energy, 0, m_error_tolerance_hi);

    //Annual
    double timeindex_annual[2];
    for (int i = 0; i < 2; i++)
        timeindex_annual[i] = i*100;

    ssc_data_set_array(data, "adjust_timeindex", timeindex_annual, 2);


    pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    ssc_data_get_number(data, "annual_energy", &annual_energy);
    EXPECT_NEAR(annual_energy, 8837.3, m_error_tolerance_hi);
    ssc_number_t kwh_per_kw;
    ssc_data_get_number(data, "kwh_per_kw", &kwh_per_kw);
    EXPECT_NEAR(kwh_per_kw, 1883, m_error_tolerance_hi) << "Energy yield"; // Same as 1 year because year 2 has 0 production

    //Monthly
    double timeindex_monthly[24];
    for (int i = 0; i < 24; i++)
        if (i >= 12)
            timeindex_monthly[i] = 100.0;
        else
            timeindex_monthly[i] = 0.0;
    ssc_data_set_array(data, "adjust_timeindex", timeindex_monthly, 24);


    pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    ssc_data_get_number(data, "annual_energy", &annual_energy);
    EXPECT_NEAR(annual_energy, 8837.3, m_error_tolerance_hi);
    ssc_data_get_number(data, "kwh_per_kw", &kwh_per_kw);
    EXPECT_NEAR(kwh_per_kw, 1883, m_error_tolerance_hi) << "Energy yield"; // Same as 1 year because year 2 has 0 production

    //Weekly
    double timeindex_weekly[104];
    for (int i = 0; i < 104; i++)
        if (i >= 52)
            timeindex_weekly[i] = 100;
        else
            timeindex_weekly[i] = 0.0;

    ssc_data_set_array(data, "adjust_timeindex", timeindex_weekly, 104);


    pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    ssc_data_get_number(data, "annual_energy", &annual_energy);
    EXPECT_NEAR(annual_energy, 8837.3, m_error_tolerance_hi);
    ssc_data_get_number(data, "kwh_per_kw", &kwh_per_kw);
    EXPECT_NEAR(kwh_per_kw, 1883, m_error_tolerance_hi) << "Energy yield"; // Same as 1 year because year 2 has 0 production

    //Daily
    double timeindex_daily[730];
    for (int i = 0; i < 730; i++)
        if (i >= 365)
            timeindex_daily[i] = 100;
        else
            timeindex_daily[i] = 0.0;
    ssc_data_set_array(data, "adjust_timeindex", timeindex_daily, 730);


    pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    ssc_data_get_number(data, "annual_energy", &annual_energy);
    EXPECT_NEAR(annual_energy, 8837.3, m_error_tolerance_hi);
    ssc_data_get_number(data, "kwh_per_kw", &kwh_per_kw);
    EXPECT_NEAR(kwh_per_kw, 1883, m_error_tolerance_hi) << "Energy yield"; // Same as 1 year because year 2 has 0 production

    //Hourly
    double timeindex_hourly[17520];
    for (int i = 0; i < 17520; i++)
        if (i >= 8760)
            timeindex_hourly[i] = 100;
        else
            timeindex_hourly[i] = 0.0;
    ssc_data_set_array(data, "adjust_timeindex", timeindex_hourly, 17520);

    pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    ssc_data_get_number(data, "annual_energy", &annual_energy);
    EXPECT_NEAR(annual_energy, 8837.3, m_error_tolerance_hi);
    ssc_data_get_number(data, "kwh_per_kw", &kwh_per_kw);
    EXPECT_NEAR(kwh_per_kw, 1883, m_error_tolerance_hi) << "Energy yield"; // Same as 1 year because year 2 has 0 production

    //Subhourly
    double timeindex_subhourly[35040];
    for (int i = 0; i < 35040; i++)
        if (i >= 17520)
            timeindex_subhourly[i] = 100;
        else
            timeindex_subhourly[i] = 0.0;
    ssc_data_set_array(data, "adjust_timeindex", timeindex_subhourly, 35040);

    pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    ssc_data_get_number(data, "annual_energy", &annual_energy);
    EXPECT_NEAR(annual_energy, 8837.3, m_error_tolerance_hi);
    ssc_data_get_number(data, "kwh_per_kw", &kwh_per_kw);
    EXPECT_NEAR(kwh_per_kw, 1883, m_error_tolerance_hi) << "Energy yield"; // Same as 1 year because year 2 has 0 production

}

TEST_F(CMPvsamv1PowerIntegration_cmod_pvsamv1, LossAdjustmentNonLifetime) {
    
   

    //Single Value
    double timeindex[1];
    for (int i = 0; i < 1; i++)
        timeindex[i] = 100;

    ssc_data_set_number(data, "adjust_constant", 0.0);
    ssc_data_set_number(data, "adjust_en_timeindex", 1.0);
    ssc_data_set_array(data, "adjust_timeindex", timeindex, 1);


    int pvsam_errors = run_module(data, "pvsamv1");
    ssc_number_t annual_energy;
    ssc_data_get_number(data, "annual_energy", &annual_energy);
    EXPECT_NEAR(annual_energy, 0, m_error_tolerance_hi);

    //Annual
    double timeindex_annual[1];
    for (int i = 0; i < 1; i++)
        timeindex_annual[i] = 100;
    ssc_data_set_array(data, "adjust_timeindex", timeindex_annual, 1);


    pvsam_errors = run_module(data, "pvsamv1");
    ssc_data_get_number(data, "annual_energy", &annual_energy);
    EXPECT_NEAR(annual_energy, 0, m_error_tolerance_hi);
    ssc_number_t kwh_per_kw;
    ssc_data_get_number(data, "kwh_per_kw", &kwh_per_kw);
    EXPECT_NEAR(kwh_per_kw, 0, m_error_tolerance_hi) << "Energy yield"; // Same as 1 year because year 2 has 0 production

    //Monthly
    double timeindex_monthly[12];
    for (int i = 0; i < 12; i++)
        if (i >= 6)
            timeindex_monthly[i] = 100.0;
        else
            timeindex_monthly[i] = 0.0;
    ssc_data_set_array(data, "adjust_timeindex", timeindex_monthly, 12);


    pvsam_errors = run_module(data, "pvsamv1");
    ssc_data_get_number(data, "annual_energy", &annual_energy);
    EXPECT_NEAR(annual_energy, 4533.9, m_error_tolerance_hi);
    ssc_data_get_number(data, "kwh_per_kw", &kwh_per_kw);
    EXPECT_NEAR(kwh_per_kw, 965.7, m_error_tolerance_hi) << "Energy yield"; // Same as 1 year because year 2 has 0 production

    //Weekly
    double timeindex_weekly[52];
    for (int i = 0; i < 52; i++)
        if (i >= 26)
            timeindex_weekly[i] = 100;
        else
            timeindex_weekly[i] = 0.0;

    ssc_data_set_array(data, "adjust_timeindex", timeindex_weekly, 52);

    pvsam_errors = run_module(data, "pvsamv1");
    ssc_data_get_number(data, "annual_energy", &annual_energy);
    EXPECT_NEAR(annual_energy, 4562.5, m_error_tolerance_hi);
    ssc_data_get_number(data, "kwh_per_kw", &kwh_per_kw);
    EXPECT_NEAR(kwh_per_kw, 971.8, m_error_tolerance_hi) << "Energy yield"; // Same as 1 year because year 2 has 0 production

    //Daily
    double timeindex_daily[365];
    for (int i = 0; i < 365; i++)
        if (i >= 180)
            timeindex_daily[i] = 100;
        else
            timeindex_daily[i] = 0.0;
    ssc_data_set_array(data, "adjust_timeindex", timeindex_daily, 365);

    pvsam_errors = run_module(data, "pvsamv1");
    ssc_data_get_number(data, "annual_energy", &annual_energy);
    EXPECT_NEAR(annual_energy, 4504.9, m_error_tolerance_hi);
    ssc_data_get_number(data, "kwh_per_kw", &kwh_per_kw);
    EXPECT_NEAR(kwh_per_kw, 959.6, m_error_tolerance_hi) << "Energy yield"; // Same as 1 year because year 2 has 0 production

    //Hourly
    double timeindex_hourly[8760];
    for (int i = 0; i < 8760; i++)
        if (i >= 5000)
            timeindex_hourly[i] = 100;
        else
            timeindex_hourly[i] = 0.0;
    ssc_data_set_array(data, "adjust_timeindex", timeindex_hourly, 8760);

    pvsam_errors = run_module(data, "pvsamv1");
    ssc_data_get_number(data, "annual_energy", &annual_energy);
    EXPECT_NEAR(annual_energy, 5296.1, m_error_tolerance_hi);
    ssc_data_get_number(data, "kwh_per_kw", &kwh_per_kw);
    EXPECT_NEAR(kwh_per_kw, 1128.0, m_error_tolerance_hi) << "Energy yield"; // Same as 1 year because year 2 has 0 production

    //Subhourly
    double timeindex_subhourly[17520];
    for (int i = 0; i < 17520; i++)
        if (i >= 10000)
            timeindex_subhourly[i] = 100;
        else
            timeindex_subhourly[i] = 0.0;
    ssc_data_set_array(data, "adjust_timeindex", timeindex_subhourly, 17520);

    pvsam_errors = run_module(data, "pvsamv1");
    EXPECT_TRUE(pvsam_errors); //this should throw an error because we are not allowing losses at a more granular timestep than weather
}



/// Test PVSAMv1 with default no-financial model and a 15-minute weather file
TEST_F(CMPvsamv1PowerIntegration_cmod_pvsamv1, NoFinancialModelCustomWeatherFile) {

    std::map<std::string, std::string> pairs;
    pairs["solar_resource_file"] = solar_resource_path_15_min;
    int pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);

    EXPECT_FALSE(pvsam_errors);

    if (!pvsam_errors)
    {
        ssc_number_t annual_energy;
        ssc_data_get_number(data, "annual_energy", &annual_energy);
        EXPECT_NEAR(annual_energy, 8193.5, m_error_tolerance_hi) << "Annual energy.";

        ssc_number_t capacity_factor;
        ssc_data_get_number(data, "capacity_factor", &capacity_factor);
        EXPECT_NEAR(capacity_factor, 19.9, m_error_tolerance_lo) << "Capacity factor";

        ssc_number_t kwh_per_kw;
        ssc_data_get_number(data, "kwh_per_kw", &kwh_per_kw);
        EXPECT_NEAR(kwh_per_kw, 1746, m_error_tolerance_hi) << "Energy yield";

        ssc_number_t performance_ratio;
        ssc_data_get_number(data, "performance_ratio", &performance_ratio);
        EXPECT_NEAR(performance_ratio, 0.80, m_error_tolerance_lo) << "Energy yield";
    }

    pairs["solar_resource_file"] = solar_resource_path_15min_fail;
    pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);

    //should fail with an error message about needing the minute column to be 0-59
    EXPECT_TRUE(pvsam_errors);
}

/// Test PVSAMv1 with default no-financial model and combinations of Sky Diffuse Model and Weather File Irradiance
TEST_F(CMPvsamv1PowerIntegration_cmod_pvsamv1, NoFinancialModelSkyDiffuseAndIrradModels)
{
    std::vector<double> annual_energy_expected = { 8634, 8644, 8649, 8758, 8768, 8773, 8837, 8847, 8851, 7622, 7401 };
    std::map<std::string, double> pairs;
    size_t count = 0;

    // Sky diffuse models: isotropic, hdkr, perez
    for (int sky_diffuse_model = 0; sky_diffuse_model < 3; sky_diffuse_model++)
    {
        // Weather file irradiance: DNI & DHI, DNI & GHI, GHI & DHI
        for (int irrad_mode = 0; irrad_mode < 3; irrad_mode++)
        {
            pairs["irrad_mode"] = irrad_mode;
            pairs["sky_model"] = sky_diffuse_model;
            int pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
            EXPECT_FALSE(pvsam_errors);
            if (!pvsam_errors)
            {
                ssc_number_t annual_energy;
                ssc_data_get_number(data, "annual_energy", &annual_energy);
                EXPECT_NEAR(annual_energy, annual_energy_expected[count], m_error_tolerance_hi) << "Annual energy.";
            }
            count++;
        }
    }

    // Perez with POA reference cell
    pairs["sky_model"] = 2;
    pairs["irrad_mode"] = 3;
    int pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    EXPECT_FALSE(pvsam_errors);
    if (!pvsam_errors) {
        SetCalculated("annual_energy");
        EXPECT_NEAR(calculated_value, annual_energy_expected[count], m_error_tolerance_hi);
        count++;
    }

    // Perez with POA pyranometer
    pairs["irrad_mode"] = 4;
    pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    EXPECT_FALSE(pvsam_errors);
    if (!pvsam_errors) {
        SetCalculated("annual_energy");
        EXPECT_NEAR(calculated_value, annual_energy_expected[count], m_error_tolerance_hi);
    }
}

/// Test PVSAMv1 with default no-financial model and combinations of module and inverter models
TEST_F(CMPvsamv1PowerIntegration_cmod_pvsamv1, NoFinancialModelModuleAndInverterModels)
{
    std::vector<double> annual_energy_expected = { 2515, 2547, 2474, 2515, 8837, 8821, 8788, 8837, 58, 62, 64, 58, 5396, 5398, 5345, 5396, 1726, 1765, 1695, 1726 };
    std::map<std::string, double> pairs;
    size_t count = 0;

    // Module models: Simple Efficiency, CEC Performance Database, CEC User Entered, Sandia, IEC61853
    for (int module_model = 0; module_model < 5; module_model++)
    {
        // Inverter models: CEC, Datasheet, Partload Curve, Coefficient Generator
        for (int inverter_model = 0; inverter_model < 4; inverter_model++)
        {
            pairs["module_model"] = module_model;
            pairs["inverter_model"] = inverter_model;
            int pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
            EXPECT_FALSE(pvsam_errors);

            if (!pvsam_errors)
            {
                ssc_number_t annual_energy;
                ssc_data_get_number(data, "annual_energy", &annual_energy);
                EXPECT_NEAR(annual_energy, annual_energy_expected[count], m_error_tolerance_hi) << "Annual energy.";
            }
            count++;
        }
    }
}

/// Test PVSAMv1 with default no-financial model and combinations of module thermal, spectral, and reflection models
//This test can be expanded when we allow different combinations of thermal, spectral, and reflection models with different module models
TEST_F(CMPvsamv1PowerIntegration_cmod_pvsamv1, NoFinancialModelModuleThermalSpectralReflection)
{
    std::vector<double> annual_energy_expected = { 8871, 8837 };
    std::map<std::string, double> pairs;
    size_t count = 0;
    int pvsam_errors = 1;

    // Module thermal models: NOCT, Heat Transfer Method
    for (int cec_temp_corr_mode = 1; cec_temp_corr_mode >= 0; cec_temp_corr_mode--) //decrement counter to avoid re-running with default (0) in next test
    {
        pairs["cec_temp_corr_mode"] = cec_temp_corr_mode;
        pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
        EXPECT_FALSE(pvsam_errors);
        if (!pvsam_errors)
        {
            ssc_number_t annual_energy;
            ssc_data_get_number(data, "annual_energy", &annual_energy);
            EXPECT_NEAR(annual_energy, annual_energy_expected[count], m_error_tolerance_hi) << "Annual energy.";
        }
        count++;
    }

    // Test reflection (IAM) loss for fixed tilt
    ssc_number_t reflection_loss = 0;
    if (!pvsam_errors)
    {
        ssc_data_get_number(data, "annual_poa_cover_loss_percent", &reflection_loss);
        EXPECT_NEAR(reflection_loss, 1.488, 0.01);
    }
    // Test reflection (IAM) loss for one-axis tracker
    pairs["subarray1_track_mode"] = 1;
    pairs["subarray1_tilt"] = 0;
    pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    EXPECT_FALSE(pvsam_errors);
    if (!pvsam_errors)
    {
        ssc_data_get_number(data, "annual_poa_cover_loss_percent", &reflection_loss);
        EXPECT_NEAR(reflection_loss, 0.751, 0.01);
    }
}

/// Test PVSAMv1 with default no-financial model and sytem design page changes
TEST_F(CMPvsamv1PowerIntegration_cmod_pvsamv1, NoFinancialModelSystemDesign)
{
    pvsamv_nofinancial_default(data);

    // Specify modules and inverters with tracking options
    // Tracking options: Fixed, 1-axis, 2-axis, Azimuth Axis, Seasonal Tilt
    std::map<std::string, double> pairs;
    pairs["subarray1_modules_per_string"] = 6;
    pairs["subarray2_modules_per_string"] = 6;
    pairs["subarray3_modules_per_string"] = 6;
    pairs["subarray4_modules_per_string"] = 6;
    pairs["subarray1_nstrings"] = 49;
    pairs["inverter_count"] = 22;
    pairs["subarray1_track_mode"] = 0;

    std::vector<double> annual_energy_expected = { 185732, 243325, 259072, 217960, 195177 };

    for (int tracking_option = 0; tracking_option < 5; tracking_option++)
    {
        // update tracking option
        pairs["subarray1_track_mode"] = (double)tracking_option;
        int pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
        EXPECT_FALSE(pvsam_errors);
        if (!pvsam_errors)
        {
            ssc_number_t annual_energy;
            ssc_data_get_number(data, "annual_energy", &annual_energy);
            EXPECT_NEAR(annual_energy, annual_energy_expected[tracking_option], m_error_tolerance_hi) << "Annual energy.";
        }
    }

    // Test fixed-tilt with backtracking
    pairs["subarray1_track_mode"] = 1;
    pairs["subarray1_backtrack"] = 1;

    int pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    EXPECT_FALSE(pvsam_errors);
    if (!pvsam_errors)
    {
        ssc_number_t annual_energy;
        ssc_data_get_number(data, "annual_energy", &annual_energy);
        EXPECT_NEAR(annual_energy, 238138, m_error_tolerance_hi) << "Annual energy.";
    }

    // Test multiple sub-arrays with different tracking, tilt, azimuth, gcr, tracker rotation limit
    pairs["subarray1_nstrings"] = 14;
    pairs["subarray2_enable"] = 1;
    pairs["subarray2_nstrings"] = 15;
    pairs["subarray3_enable"] = 1;
    pairs["subarray3_nstrings"] = 10;
    pairs["subarray4_enable"] = 1;
    pairs["subarray4_nstrings"] = 10;

    annual_energy_expected.clear();
    double subarray1_azimuth = 180;
    double subarray2_azimuth = 180;
    double subarray3_azimuth = 180;
    double subarray4_azimuth = 180;
    double enable_mismatch = 1;
    double subarray1_gcr = 0.3;
    double subarray2_gcr = 0.3;
    double subarray3_gcr = 0.3;
    double subarray4_gcr = 0.3;
    double subarray1_tilt = 20;
    double subarray2_tilt = 20;
    double subarray3_tilt = 20;
    double subarray4_tilt = 20;
    double subarray1_rotlim = 45;
    double subarray2_rotlim = 45;
    double subarray3_rotlim = 45;
    double subarray4_rotlim = 45;
    double subarray1_track_mode = 0;
    double subarray2_track_mode = 0;
    double subarray3_track_mode = 0;
    double subarray4_track_mode = 0;
    annual_energy_expected = { 185723 };

    pairs["enable_mismatch_vmax_calc"] = enable_mismatch;
    pairs["subarray1_azimuth"] = subarray1_azimuth;
    pairs["subarray2_azimuth"] = subarray2_azimuth;
    pairs["subarray3_azimuth"] = subarray3_azimuth;
    pairs["subarray4_azimuth"] = subarray4_azimuth;
    pairs["subarray1_gcr"] = subarray1_gcr;
    pairs["subarray2_gcr"] = subarray2_gcr;
    pairs["subarray3_gcr"] = subarray3_gcr;
    pairs["subarray4_gcr"] = subarray4_gcr;
    pairs["subarray2_tilt"] = subarray2_tilt;
    pairs["subarray3_tilt"] = subarray3_tilt;
    pairs["subarray1_tilt"] = subarray1_tilt;
    pairs["subarray4_tilt"] = subarray4_tilt;
    pairs["subarray1_rotlim"] = subarray1_rotlim;
    pairs["subarray2_rotlim"] = subarray2_rotlim;
    pairs["subarray3_rotlim"] = subarray3_rotlim;
    pairs["subarray4_rotlim"] = subarray4_rotlim;
    pairs["subarray1_track_mode"] = subarray1_track_mode;
    pairs["subarray2_track_mode"] = subarray2_track_mode;
    pairs["subarray3_track_mode"] = subarray3_track_mode;
    pairs["subarray4_track_mode"] = subarray4_track_mode;

    pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    EXPECT_FALSE(pvsam_errors);
    if (!pvsam_errors)
    {
        ssc_number_t annual_energy;
        ssc_data_get_number(data, "annual_energy", &annual_energy);
        EXPECT_NEAR(annual_energy, annual_energy_expected[0], m_error_tolerance_hi) << "Index: " << 0;
    }
}

/// Test PVSAMv1 with default no-financial model and different shading options
TEST_F(CMPvsamv1PowerIntegration_cmod_pvsamv1, NoFinancialModelShading)
{
    // 0: No Shading, 1: 3D Shading, 2: 3D shading with self shading (non-linear), 3: Snow
// PR 280	std::vector<double> annual_energy_expected = { 12911, 10607, 10579, 10377 };
    // 2 - 3D shading with self-shading reduced from 10579 to 10529
    // 3 - shading with snow reduced from 10377 to 10328
    std::vector<double> annual_energy_expected = { 13137, 10768, 10707, 10501 };
    std::map<std::string, double> pairs;

    // 2 subarrays, one pointing east, one west
    pairs["subarray1_modules_per_string"] = 6;
    pairs["subarray2_modules_per_string"] = 6;
    pairs["subarray3_modules_per_string"] = 6;
    pairs["subarray4_modules_per_string"] = 6;
    pairs["inverter_count"] = 2;
    pairs["subarray1_nstrings"] = 2;
    pairs["subarray1_azimuth"] = 90;
    pairs["subarray2_enable"] = 1;
    pairs["subarray2_nstrings"] = 2;
    pairs["subarray2_azimuth"] = 270;

    // 0. No Shading
    int pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    EXPECT_FALSE(pvsam_errors);
    if (!pvsam_errors) {
        SetCalculated("annual_energy");
        EXPECT_NEAR(calculated_value, annual_energy_expected[0], m_error_tolerance_hi);
    }

    // 4. Face both in same direction, ensure poa is calculated the same
    pairs["subarray1_azimuth"] = 180;
    pairs["subarray2_azimuth"] = 180;
    pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    EXPECT_FALSE(pvsam_errors);
    if (!pvsam_errors) {
        int n1, n2;
        ssc_number_t* subarray1_poa_front = ssc_data_get_array(data, "subarray1_poa_front", &n1);
        ssc_number_t* subarray2_poa_front = ssc_data_get_array(data, "subarray2_poa_front", &n2);
        EXPECT_EQ(n1, n2);
        for (int i = 0; i < n1; i++) {
            EXPECT_EQ(subarray1_poa_front[i], subarray2_poa_front[i]);
        }
    }

    // 1. Add 3D Shading
    pairs["subarray1_azimuth"] = 90;
    pairs["subarray2_azimuth"] = 270;


    pairs["subarray1_shading_en_timestep"] = 1;
    pairs["subarray2_shading_en_timestep"] = 1;
    pairs["subarray1_shading_en_diff"] = 1;
    pairs["subarray2_shading_en_diff"] = 1;
    set_matrix(data, "subarray1_shading_timestep", subarray1_shading, 8760, 2);
    set_matrix(data, "subarray2_shading_timestep", subarray2_shading, 8760, 2);
    pairs["subarray1_shading_diff"] = 10.010875701904297;
    pairs["subarray2_shading_diff"] = 10.278481483459473;

    pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    EXPECT_FALSE(pvsam_errors);
    if (!pvsam_errors) {
        SetCalculated("annual_energy");
        EXPECT_NEAR(calculated_value, annual_energy_expected[1], m_error_tolerance_hi);
    }

    // 2. Add Self Shading to 3D shading
    pairs["subarray1_shade_mode"] = 1;
    pairs["subarray1_mod_orient"] = 1;
    pairs["subarray1_nmody"] = 1;
    pairs["subarray1_nmodx"] = 6;
    pairs["subarray2_shade_mode"] = 1;
    pairs["subarray2_mod_orient"] = 1;
    pairs["subarray2_nmody"] = 1;
    pairs["subarray2_nmodx"] = 6;

    pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    EXPECT_FALSE(pvsam_errors);
    if (!pvsam_errors) {
        SetCalculated("annual_energy");
        EXPECT_NEAR(calculated_value, annual_energy_expected[2], m_error_tolerance_hi);
    }

    // 3. Add Snow losses to all shading
    pairs["en_snow_model"] = 1;

    pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    EXPECT_FALSE(pvsam_errors);
    if (!pvsam_errors) {
        SetCalculated("annual_energy");
        EXPECT_NEAR(calculated_value, annual_energy_expected[3], m_error_tolerance_hi);
    }
}

/// Test PVSAMv1 with default no-financial model and different loss options
TEST_F(CMPvsamv1PowerIntegration_cmod_pvsamv1, NoFinancialModelLosses)
{
    // 0: Default Losses, 1: Modify Point Losses, 2: Modify Availability
    std::vector<double> annual_energy_expected = { 8837, 7991, 7720 };
    std::map<std::string, double> pairs;

    // 0: Default losses
    int pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    EXPECT_FALSE(pvsam_errors);
    if (!pvsam_errors) {
        SetCalculated("annual_energy");
        EXPECT_NEAR(calculated_value, annual_energy_expected[0], m_error_tolerance_hi);
    }

    // 1: Modify Point Losses
    ssc_number_t p_subarray1_soiling[12] = { 5, 5, 5, 5, 6, 6, 6, 6, 5, 5, 5, 5 };
    ssc_data_set_array(data, "subarray1_soiling", p_subarray1_soiling, 12);

    pairs["subarray1_mismatch_loss"] = 3;
    pairs["subarray1_diodeconn_loss"] = 0.6;
    pairs["subarray1_dcwiring_loss"] = 2;
    pairs["subarray1_tracking_loss"] = 1;
    pairs["subarray1_nameplate_loss"] = 1;
    pairs["dcoptimizer_loss"] = 1;
    pairs["acwiring_loss"] = 2;
    pairs["transformer_no_load_loss"] = 1;
    pairs["transformer_load_loss"] = 1;

    pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    EXPECT_FALSE(pvsam_errors);
    if (!pvsam_errors) {
        SetCalculated("annual_energy");
        EXPECT_NEAR(calculated_value, annual_energy_expected[1], m_error_tolerance_hi);
    }

    // 2. Modify availability losses
    ssc_number_t p_adjust[3] = { 5268, 5436, 50 };
    ssc_data_set_number(data, "adjust_constant", 0.0);
    ssc_data_set_number(data, "adjust_en_periods", 1.0);
    ssc_data_set_matrix(data, "adjust_periods", p_adjust, 1, 3);



    ssc_number_t p_dc_adjust[3] = { 5088, 5256, 100 };
    ssc_data_set_number(data, "dc_adjust_constant", 0.0);
    ssc_data_set_number(data, "dc_adjust_en_periods", 1.0);
    ssc_data_set_matrix(data, "dc_adjust_periods", p_dc_adjust, 1, 3);

    pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    EXPECT_FALSE(pvsam_errors);
    if (!pvsam_errors) {
        SetCalculated("annual_energy");
        EXPECT_NEAR(calculated_value, annual_energy_expected[2], m_error_tolerance_hi);
    }
}

/// Test PVSAMv1 with default no-financial model and transmission losses to test interaction of transmission and ac wiring losses
TEST_F(CMPvsamv1PowerIntegration_cmod_pvsamv1, NoFinancialModelSequentialLosses)
{
    // 0: Default Losses, 1: Modify Point Losses
    std::vector<double> annual_energy_expected = { 8837, 2226};
    std::map<std::string, double> pairs;

    // 0: Default losses
    int pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    EXPECT_FALSE(pvsam_errors);
    if (!pvsam_errors) {
        SetCalculated("annual_energy");
        EXPECT_NEAR(calculated_value, annual_energy_expected[0], m_error_tolerance_hi);
    }

    // 1: Modify Point Losses
    pairs["acwiring_loss"] = 50;
    pairs["transmission_loss"] = 50;

    pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    EXPECT_FALSE(pvsam_errors);
    if (!pvsam_errors) {
        SetCalculated("annual_energy");
        EXPECT_NEAR(calculated_value, annual_energy_expected[1], m_error_tolerance_hi);
    }

    SetCalculated("annual_ac_wiring_loss_percent");
    EXPECT_NEAR(calculated_value, 50, m_error_tolerance_lo);
    SetCalculated("annual_transmission_loss_percent");
    EXPECT_NEAR(calculated_value, 25, m_error_tolerance_lo); // Both of these values are compared to ac_gross, so since this is applied second it is lower

}

/// Change half of all temperatures so that inv eff is derated by ~50% for half the year
/// DC production & inverter efficiency both decrease as result
TEST_F(CMPvsamv1PowerIntegration_cmod_pvsamv1, InvTempDerate) {
    auto weatherData = create_weatherdata_array(8760);
    ssc_data_unassign(data, "solar_resource_file");
    var_table* vt = static_cast<var_table*>(data);

    double temp[8760];
    for (size_t i = 0; i < 8760; i++) {
        temp[i] = 26.f;
    }
    for (size_t i = 0; i < 4380; i++) {
        temp[i] = 76.6f;
    }

    var_data tdry_vd = var_data(temp, 8760);
    tdry_vd = var_data(temp, 8760);
    weatherData->assign("tdry", tdry_vd);
    ssc_data_set_table(data,"solar_resource_data", weatherData);

    EXPECT_FALSE(run_module(data, "pvsamv1"));

    ssc_number_t annual_energy, loss, percent_loss, monthly_energy;

    ssc_data_get_number(data, "annual_energy", &annual_energy);
    EXPECT_NEAR(annual_energy, 4485, 10) << "Annual energy reduced";

    ssc_data_get_number(data, "annual_inv_tdcloss", &loss);
    EXPECT_NEAR(loss, 163, 10) << "Annual loss";

    ssc_data_get_number(data, "annual_dc_inv_tdc_loss_percent", &percent_loss);
    EXPECT_NEAR(percent_loss, 3, 2);

    monthly_energy = ssc_data_get_array(data, "monthly_energy", nullptr)[0];
    EXPECT_NEAR(monthly_energy, 520, 10) << "Monthly energy of January reduced";

    monthly_energy = ssc_data_get_array(data, "monthly_energy", nullptr)[11];
    EXPECT_NEAR(monthly_energy, 755, 10) << "Month energy of December not reduced";

    free_weatherdata_array(weatherData);
}

/// Test PVSAMv1 multiple MPPT inverter, otherwise using default no financial model inputs
TEST_F(CMPvsamv1PowerIntegration_cmod_pvsamv1, NoFinancialModelMultipleMPPT)
{
    std::vector<double> annual_energy_expected = { 7811 };
    std::map<std::string, double> pairs;

    pairs["inv_num_mppt"] = 2;
    pairs["subarray1_nstrings"] = 1;
    pairs["subarray1_modules_per_string"] = 7;
    pairs["subarray1_mppt_input"] = 1;
    pairs["subarray1_tilt"] = 20;
    pairs["subarray2_enable"] = 1;
    pairs["subarray2_nstrings"] = 1;
    pairs["subarray2_modules_per_string"] = 6;
    pairs["subarray2_tilt"] = 0;
    pairs["subarray2_mppt_input"] = 2;

    int pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    EXPECT_FALSE(pvsam_errors);

    if (!pvsam_errors)
    {
        ssc_number_t annual_energy;
        ssc_data_get_number(data, "annual_energy", &annual_energy);
        EXPECT_NEAR(annual_energy, annual_energy_expected[0], m_error_tolerance_hi) << "Annual energy.";
    }

}

/// Test albedo user inputs, weather file (wf) input for valid and invalid albedo values
TEST_F(CMPvsamv1PowerIntegration_cmod_pvsamv1, AlbedoTest_cmod_pvsamv1) {

    // albedo user input values for testing
    ssc_number_t albedo_monthly[12] = { 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 0.90, 0.91, 0.92 };
    ssc_number_t albedo_monthly_invalid[12] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 91, 92 };

    // weather files for albedo tests
    // pv_albedo_test.csv time steps 1, 2, and 3229 have invalid albedo values for testing
    // time steps 0, 1, and 2 have snow depth = 14.2, time steps 3228 and 3229 have snow depth = 0 (snow is for pvwattsv8)
    char wf_albedo[256];
    int a1 = sprintf(wf_albedo, "%s/test/input_cases/pvsamv1_data/pv_albedo_test.csv", SSCDIR);
    char wf_no_albedo[256];
    int a2 = sprintf(wf_no_albedo, "%s/test/input_cases/pvsamv1_data/pv_albedo_test_no_albedo.csv", SSCDIR);

    // albedo values from wf
    ssc_number_t albedo_ts_0 = 0.87; // jan 1 1:00 am

    // Test set 1: weather file with no albedo.
    ssc_data_set_string(data, "solar_resource_file", wf_no_albedo);

    ssc_data_set_number(data, "use_wf_albedo", 1);

    ssc_data_set_array(data, "albedo", albedo_monthly, 12);
    EXPECT_FALSE(run_module(data, "pvsamv1"));
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[0], albedo_monthly[0], m_error_tolerance_lo) << "Time step 0: Albedo should be albedo input for jan.";
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[1], albedo_monthly[0], m_error_tolerance_lo) << "Time step 1: Albedo should be albedo input for jan.";

    ssc_data_set_array(data, "albedo", albedo_monthly_invalid, 1);
    EXPECT_TRUE(run_module(data, "pvsamv1"));

    ssc_data_set_number(data, "use_wf_albedo", 0);

    ssc_data_set_array(data, "albedo", albedo_monthly, 12);
    EXPECT_FALSE(run_module(data, "pvsamv1"));
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[0], albedo_monthly[0], m_error_tolerance_lo) << "Time step 0: Albedo should be albedo input for jan.";
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[1], albedo_monthly[0], m_error_tolerance_lo) << "Time step 1: Albedo should be albedo input for jan.";

    ssc_data_set_array(data, "albedo", albedo_monthly_invalid, 12);
    EXPECT_TRUE(run_module(data, "pvsamv1"));

    // Test set 2: weather file with albedo data
    ssc_data_set_string(data, "solar_resource_file", wf_albedo);

    ssc_data_set_number(data, "use_wf_albedo", 1);

    ssc_data_set_array(data, "albedo", albedo_monthly, 12);
    EXPECT_FALSE(run_module(data, "pvsamv1"));
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[0], albedo_ts_0, m_error_tolerance_lo) << "Time step 0: Albedo should be wf albedo.";
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[1], albedo_monthly[0], m_error_tolerance_lo) << "Time step 1: Albedo should be albedo input for jan.";

    ssc_data_set_array(data, "albedo", albedo_monthly_invalid, 12);
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[0], albedo_ts_0, m_error_tolerance_lo) << "Time step 0: Albedo should be wf albedo.";
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[1], NULL, m_error_tolerance_lo) << "Time step 1: Albedo should be albedo input for jan.";

    ssc_data_set_number(data, "use_wf_albedo", 0);

    ssc_data_set_array(data, "albedo", albedo_monthly, 12);
    EXPECT_FALSE(run_module(data, "pvsamv1"));
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[0], albedo_monthly[0], m_error_tolerance_lo) << "Time step 0: Albedo should be albedo input for jan.";
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[1], albedo_monthly[0], m_error_tolerance_lo) << "Time step 1: Albedo should be albedo input for jan.";

    ssc_data_set_array(data, "albedo", albedo_monthly_invalid, 12);
    EXPECT_TRUE(run_module(data, "pvsamv1"));

}

/// Test PVSAMv1 with Snow Model enabled and set to 1-axis Tracking
TEST_F(CMPvsamv1PowerIntegration_cmod_pvsamv1, SnowModel)
{
    std::map<std::string, double> pairs;

    pairs["en_snow_model"] = 1;
    pairs["subarray1_track_mode"] = 1;
    int pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    EXPECT_FALSE(pvsam_errors);

    ssc_number_t annual_energy;
    ssc_data_get_number(data, "annual_energy", &annual_energy);
    EXPECT_NEAR(annual_energy, 11395, m_error_tolerance_hi) << "Annual energy.";


    //Check bifacial - snow model interaction
    //Bifaciality = 0, results should be same as monofacial
    pairs["cec_is_bifacial"] = 1;
    pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    ssc_data_get_number(data, "annual_energy", &annual_energy);
    EXPECT_NEAR(annual_energy, 11395, m_error_tolerance_hi) << "Annual energy.";


    //Bifacial - scale DC snow loss by poa_front / poa_total (including bifaciality)
    pairs["cec_bifaciality"] = 0.70;
    pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    ssc_data_get_number(data, "annual_energy", &annual_energy);
    EXPECT_NEAR(annual_energy, 11635, m_error_tolerance_hi) << "Annual energy.";

}

TEST_F(CMPvsamv1PowerIntegration_cmod_pvsamv1, SubhourlyClippingCorrectionModel)
{
    std::map<std::string, double> pairs;

    //Run with default matrix stored in ssc
    pairs["enable_subhourly_clipping"] = 1;
    int pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    EXPECT_FALSE(pvsam_errors);
    //check answers for subhourly clipping annual loss
    ssc_number_t subhourly_clipping_loss;
    ssc_data_get_number(data, "annual_subhourly_clipping_loss", &subhourly_clipping_loss);
    EXPECT_NEAR(subhourly_clipping_loss, 16.633622, m_error_tolerance_lo);

    //Modify matrix, re-run test with user entered matrix (SDK only)
    ssc_number_t Subhourly_Clipping_Matrix[441] =
    {
        0, -2, -0.4, -0.3, -0.2, -0.1, 0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1, 1.1, 1.2, 1.3, 1.4,
        -0.001, 1.65e-08, 2.17e-08, 2.48e-08, 3.09e-08, 4.49e-08, 6.7e-08, 1.06e-07, 1.74e-07, 2.67e-07, 3.97e-07, 5.71e-07, 8.09e-07, 1.11e-06, 1.55e-06, 2.03e-06, 2.38e-06, 2.84e-06, 2.85e-06, 4.86e-06, 3.12e-08,
        0.05, 6.14e-08, 1.07e-07, 1.39e-07, 2.05e-07, 3.72e-07, 7.5e-07, 1.49e-06, 2.57e-06, 4.02e-06, 5.87e-06, 8.09e-06, 1.06e-05, 1.33e-05, 1.64e-05, 1.91e-05, 2.15e-05, 2.45e-05, 2.65e-05, 2.75e-05, 0,
        0.1, 7.63e-08, 1.45e-07, 1.89e-07, 2.84e-07, 5.77e-07, 1.28e-06, 2.72e-06, 4.89e-06, 7.72e-06, 1.12e-05, 1.5e-05, 1.9e-05, 2.28e-05, 2.66e-05, 2.96e-05, 3.04e-05, 3.09e-05, 3e-05, 2.9e-05, 0,
        0.15, 8.58e-08, 1.67e-07, 2.16e-07, 3.22e-07, 6.78e-07, 1.67e-06, 3.81e-06, 7.09e-06, 1.12e-05, 1.59e-05, 2.1e-05, 2.54e-05, 2.92e-05, 3.24e-05, 3.44e-05, 3.31e-05, 3.36e-05, 3.13e-05, 9.65e-05, 0,
        0.2, 9.56e-08, 1.76e-07, 2.34e-07, 3.51e-07, 8e-5, 2.05e-06, 4.87e-06, 9.19e-06, 1.48e-05, 2.07e-05, 2.68e-05, 3.13e-05, 3.43e-05, 3.62e-05, 3.67e-05, 3.39e-05, 2.86e-05, 1.8e-05, 1.22e-05, 0,
        0.25, 8.82e-08, 1.83e-07, 2.52e-07, 3.9e-07, 7.96e-07, 2.18e-06, 5.35e-06, 1.06e-05, 1.71e-05, 2.41e-05, 3.01e-05, 3.42e-05, 3.66e-05, 3.76e-05, 3.66e-05, 3.36e-05, 2.87e-05, 3.02e-05, 2.5e-06, 0,
        0.3, 8.79e-08, 1.77e-07, 2.35e-07, 3.56e-07, 7.78e-07, 2.28e-06, 6.03e-06, 1.22e-05, 1.98e-05, 2.69e-05, 3.25e-05, 3.56e-05, 3.64e-05, 3.59e-05, 3.33e-05, 2.89e-05, 2.49e-05, 1.66e-05, 7.82e-06, 0,
        0.35, 8.77e-08, 1.74e-07, 2.35e-07, 3.6e-07, 7.71e-07, 2.3e-06, 6.38e-06, 1.35e-05, 2.19e-05, 2.94e-05, 3.38e-05, 3.54e-05, 3.49e-05, 3.3e-05, 2.96e-05, 2.48e-05, 2e-05, 1.24e-05, 5.89e-06, 0,
        0.4, 7.67e-08, 1.5e-07, 2.04e-07, 3.19e-07, 6.85e-07, 2.3e-06, 6.85e-06, 1.46e-05, 2.35e-05, 3e-05, 3.31e-05, 3.29e-05, 3.07e-05, 2.81e-05, 2.49e-05, 2.13e-05, 1.66e-05, 1.55e-05, 8.9e-06, 0,
        0.45, 6.85e-08, 1.26e-07, 1.74e-07, 2.54e-07, 5.75e-07, 2.05e-06, 6.49e-06, 1.43e-05, 2.28e-05, 2.81e-05, 2.93e-05, 0, 2.47e-05, 2.22e-05, 1.95e-05, 1.72e-05, 1.48e-05, 1.13e-05, 9.97e-06, 0,
        0.5, 6e-08, 1.07e-07, 1.46e-07, 2.14e-07, 4.76e-07, 1.87e-06, 6.42e-06, 1.44e-05, 2.19e-05, 2.5e-05, 2.44e-05, 2.15e-05, 1.85e-05, 1.67e-05, 1.51e-05, 1.23e-05, 1.08e-05, 6.83e-06, 3.94e-06, 0,
        0.55, 5.45e-08, 8.43e-08, 1.1e-07, 1.62e-07, 3.57e-07, 1.44e-06, 5.31e-06, 1.24e-05, 0, 1.92e-05, 1.74e-05, 1.45e-05, 1.24e-05, 1.11e-05, 1.02e-05, 9.23e-06, 8.33e-06, 7.41e-06, 1.03e-05, 0,
        0.6, 5.04e-08, 6.61e-08, 7.87e-08, 1.04e-07, 2.43e-07, 1.08e-06, 4.34e-06, 1e-05, 1.35e-05, 1.28e-05, 1.05e-05, 8.57e-06, 7.34e-06, 6.72e-06, 6.32e-06, 5.98e-06, 5.48e-06, 5.91e-06, 7.29e-06, 1.29e-05,
        0.65, 4.65e-08, 5.31e-08, 5.61e-08, 6.96e-08, 1.64e-07, 7.59e-07, 3.32e-06, 7.41e-06, 9e-06, 7.31e-06, 5.61e-06, 4.63e-06, 4.02e-06, 3.77e-06, 3.62e-06, 3.75e-06, 4.05e-06, 4.68e-06, 1.28e-06, 0,
        0.7, 4.32e-08, 4.14e-08, 3.93e-08, 4.27e-08, 8.67e-08, 4.76e-07, 2.34e-06, 5.12e-06, 4.83e-06, 3.1e-06, 2.29e-06, 1.92e-06, 1.7e-06, 1.6e-06, 1.51e-06, 1.76e-06, 1.86e-06, 2.76e-06, 2.81e-06, 1.47e-05,
        0.75, 3.67e-08, 2.98e-08, 2.64e-08, 2.72e-08, 5.66e-08, 3.51e-07, 1.83e-06, 3.31e-06, 1.93e-06, 9.42e-07, 6.97e-07, 6.06e-07, 5.35e-07, 5.02e-07, 4.92e-07, 5.28e-07, 6.97e-07, 1.11e-06, 1.85e-06, 0,
        0.8, 2.4e-08, 1.66e-08, 1.46e-08, 1.32e-08, 2.8e-08, 2.74e-07, 1.42e-06, 1.67e-06, 5.05e-07, 1.83e-07, 1.31e-07, 1.1e-07, 9.81e-08, 9.13e-08, 9.04e-08, 8.69e-08, 9.37e-08, 3.13e-08, -6.82e-21, 0,
        0.85, 1.02e-08, 4.91e-09, 5.46e-09, 6.62e-09, 1.35e-08, 2.19e-07, 8.5e-07, 4.94e-07, 7e-08, 1.92e-08, 1.37e-08, 1.12e-08, 1e-08, 8.96e-09, 9.5e-09, 8.62e-09, 1.65e-08, 3.26e-08, 1.5e-07, 0,
        0.9, 9.96e-10, 3.15e-09, 2.89e-09, 3.44e-09, 3.53e-09, 1.49e-07, 4.09e-07, 9.15e-08, 1.53e-09, -2.09e-20, -6.38e-21, -4.12e-21, -1.64e-20, -1.97e-20, -1.74e-20, -1.63e-20, -6.11e-21, -1.17e-20, 0, 0,
        0.95, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0

    };
    
    ssc_data_set_matrix(data, "subhourly_clipping_matrix", Subhourly_Clipping_Matrix, 21, 21);
    //pairs["subhourly_clipping_matrix"] = sub_clipping_matrix;
    pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    EXPECT_FALSE(pvsam_errors);
    //check answers for subhourly clipping annual loss
    ssc_data_get_number(data, "annual_subhourly_clipping_loss", &subhourly_clipping_loss);
    EXPECT_NEAR(subhourly_clipping_loss, 21.698475, m_error_tolerance_lo);

    //Test different matrix size
    ssc_number_t Subhourly_Clipping_Matrix_Short[272];
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 17; j++) {
            Subhourly_Clipping_Matrix_Short[i * 17 + j] = Subhourly_Clipping_Matrix[i * 21 + j];
        }
    }
    ssc_data_set_matrix(data, "subhourly_clipping_matrix", Subhourly_Clipping_Matrix_Short, 16, 17);
    pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    EXPECT_FALSE(pvsam_errors);
    //check answers for subhourly clipping annual loss
    double subhourly_clipping_loss_short;
    ssc_data_get_number(data, "annual_subhourly_clipping_loss", &subhourly_clipping_loss_short);
    double subhourly_clipping_loss_percent;
    EXPECT_NEAR(subhourly_clipping_loss_short, 24.746267, m_error_tolerance_lo);
    ssc_data_get_number(data, "annual_subhourly_clipping_loss_percent", &subhourly_clipping_loss_percent);
    EXPECT_NEAR(subhourly_clipping_loss_percent, 0.271086, m_error_tolerance_lo);
}

TEST_F(CMPvsamv1PowerIntegration_cmod_pvsamv1, useCustomCellTemp) {

    std::map<std::string, double> pairs;

    pairs["subarray1_use_custom_cell_temp"] = 1;
    int pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);

    EXPECT_FALSE(pvsam_errors);
    ssc_number_t annualEnergy;
    int n1;
    ssc_number_t* subarray1_cell_temp = ssc_data_get_array(data, "subarray1_celltemp", &n1);
    EXPECT_NEAR(subarray1_cell_temp[8], 17.8071, 0.001);
    ssc_data_get_number(data, "annual_energy", &annualEnergy);
    EXPECT_NEAR(annualEnergy, 8725, 1.0);

    pairs["subarray1_use_custom_cell_temp"] = 0;
    pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);

    EXPECT_FALSE(pvsam_errors);
    ssc_data_get_number(data, "annual_energy", &annualEnergy);
    EXPECT_NEAR(annualEnergy, 8837, 1.0);



}

TEST_F(CMPvsamv1PowerIntegration_cmod_pvsamv1, UseCustomAngles) {

    std::map<std::string, double> pairs;

    pairs["subarray1_use_custom_rot_angles"] = 1;
    int pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);

    EXPECT_FALSE(pvsam_errors);
    ssc_number_t annualEnergy;
    int n1;
    ssc_number_t* subarray1_axis_rot = ssc_data_get_array(data, "subarray1_axisrot", &n1);
    EXPECT_NEAR(subarray1_axis_rot[11], -24.8588, 0.001);
    ssc_data_get_number(data, "annual_energy", &annualEnergy);
    EXPECT_NEAR(annualEnergy, 11517, 1.0);

    pairs["subarray1_use_custom_rot_angles"] = 0;
    pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);

    EXPECT_FALSE(pvsam_errors);
    ssc_data_get_number(data, "annual_energy", &annualEnergy);
    EXPECT_NEAR(annualEnergy, 8837, 1.0);



}

TEST_F(CMPvsamv1PowerIntegration_cmod_pvsamv1, DistributionClippingMethod) {
    std::map<std::string, double> pairs;

    //Run with model enabled
    pairs["enable_subinterval_distribution"] = 1;
    int pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    EXPECT_FALSE(pvsam_errors);
    //check answers for subhourly clipping annual loss
    ssc_number_t distribution_clipping_loss;
    ssc_data_get_number(data, "annual_distribution_clipping_loss", &distribution_clipping_loss);
    EXPECT_NEAR(distribution_clipping_loss, 139.283444, m_error_tolerance_lo);
}

/// Test PVSAMv1 with all defaults and no-financial model- look at MPPT input 1 voltage at night
TEST_F(CMPvsamv1PowerIntegration_cmod_pvsamv1, InverterNighttime) {

    int pvsam_errors = run_module(data, "pvsamv1");

    EXPECT_FALSE(pvsam_errors);
    if (!pvsam_errors)
    {
        ssc_number_t inverterMppt1Voltage;
        inverterMppt1Voltage = ssc_data_get_array(data, "inverterMPPT1_DCVoltage", nullptr)[0];
        EXPECT_NEAR(inverterMppt1Voltage, 0.0, 0.001) << "MPPT Voltage should be 0 at night.";
    }
}

/// Test PVSAMv1 tilt equals latitude input
TEST_F(CMPvsamv1PowerIntegration_cmod_pvsamv1, TiltEqualsLat) {

    std::map<std::string, double> pairs;

    pairs["subarray1_tilt_eq_lat"] = 1;
    int pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);

    EXPECT_FALSE(pvsam_errors);
    if (!pvsam_errors)
    {
        ssc_number_t subarray1SurfaceTilt;
        subarray1SurfaceTilt = ssc_data_get_array(data, "subarray1_surf_tilt", nullptr)[12];
        EXPECT_NEAR(subarray1SurfaceTilt, 33.4, 0.1) << "Subarray 1 tilt should be equal to latitude.";
    }
}

/// Integration test for bifacial model in SAM
TEST_F(CMPvsamv1PowerIntegration_cmod_pvsamv1, bifacial) {

    std::map<std::string, double> pairs;

    //update bifacial inputs
    pairs["cec_is_bifacial"] = 1;
    pairs["cec_bifacial_transmission_factor"] = 0.013;
    pairs["cec_bifaciality"] = 0.65;
    pairs["cec_bifacial_ground_clearance_height"] = 1;

    //these are the inputs that need to be updated from pvsaamv1_common_data to make inverter inputs match version 2018.11.11 defaults
    pairs["cec_adjust"] = 4.86;
    pairs["cec_i_o_ref"] = 3.9880000000000000e-12;
    pairs["mppt_low_inverter"] = 100;
    pairs["inv_snl_c0"] = -3.0810000000000000e-06;
    pairs["inv_snl_c1"] = -4.8000000000000000e-05;
    pairs["inv_snl_c2"] = 0.000123;
    pairs["inv_snl_c3"] = -0.00163;
    pairs["inv_snl_paco"] = 3850;
    pairs["inv_snl_pdco"] = 3964.0;
    pairs["inv_snl_pnt"] = 1.15;
    pairs["inv_snl_pso"] = 17.9;
    pairs["inv_snl_vdco"] = 400.0;
    pairs["inv_snl_vdcmax"] = 480.0;

    //run the test
    int pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);

    EXPECT_FALSE(pvsam_errors);
    if (!pvsam_errors)
    {
        ssc_number_t annualEnergy;
        ssc_data_get_number(data, "annual_energy", &annualEnergy);
        EXPECT_NEAR(annualEnergy, 9261, 1.0) << "Bifacial annual energy from SAM version 2018.11.11 using Phoenix TMY2";
    }


    // test monofacial
    pairs["cec_is_bifacial"] = 0;
    double expected_monofacial_annual_energy = 8881.;
    pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    EXPECT_FALSE(pvsam_errors);
    if (!pvsam_errors)
    {
        ssc_number_t annualEnergy;
        ssc_data_get_number(data, "annual_energy", &annualEnergy);
        EXPECT_NEAR(annualEnergy, expected_monofacial_annual_energy, 1.0) << "Bifacial annual energy from SAM version 2018.11.11 using Phoenix TMY2";
    }

    // test bifacial but effectively monofacial
    pairs["cec_is_bifacial"] = 1;
    pairs["cec_bifaciality"] = 0.;
    pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);
    EXPECT_FALSE(pvsam_errors);
    if (!pvsam_errors)
    {
        ssc_number_t annualEnergy;
        ssc_data_get_number(data, "annual_energy", &annualEnergy);
        EXPECT_NEAR(annualEnergy, expected_monofacial_annual_energy, 1.0) << "Bifacial annual energy from SAM version 2018.11.11 using Phoenix TMY2";
    }
}

TEST_F(CMPvsamv1PowerIntegration_cmod_pvsamv1, reopt_sizing) {
    ssc_data_clear(data);
    pvsamv1_with_residential_default(data);
    utility_rate5_default(data);
    belpe_default(data);
    ssc_data_set_number(data, "lat", 30);
    ssc_data_set_number(data, "lon", -30);
    ssc_number_t gen[8760] = { 0 };
    ssc_data_set_array(data, "gen", gen, 8760);

    Reopt_size_battery_params(data);

    auto vd = static_cast<var_table*>(data);
    ASSERT_TRUE(vd->is_assigned("reopt_scenario"));
    auto site = vd->lookup("reopt_scenario");

    auto settings = site->table.lookup("Settings");
    assert(settings->table.lookup("time_steps_per_hour")->num == 1);

    std::vector<std::string> sections = { "ElectricTariff", "ElectricLoad", "Financial", "ElectricStorage", "PV" };
    for (const auto& s : sections)
        ASSERT_TRUE(site->table.is_assigned(s));
}

TEST_F(CMPvsamv1PowerIntegration_cmod_pvsamv1, reopt_sizing_w_outage) {
    ssc_data_clear(data);
    pvsamv1_with_residential_default(data);
    utility_rate5_default(data);
    belpe_default(data);
    ssc_data_set_number(data, "lat", 30);
    ssc_data_set_number(data, "lon", -30);
    ssc_number_t gen[8760] = { 0 };
    ssc_data_set_array(data, "gen", gen, 8760);

    ssc_number_t grid_outage[8760] = { 0 };

    for (int i = 6; i < 24; i++) {
        if (i > 6 && i < 9) {
            grid_outage[i] = 1;
        }
        else if (i > 17 && i < 24) {
            grid_outage[i] = 1;
        }
    }
    ssc_data_set_array(data, "grid_outage", grid_outage, 8760);
    ssc_data_set_number(data, "size_for_grid_outage", 1);

    Reopt_size_battery_params(data);

    auto vd = static_cast<var_table*>(data);
    ASSERT_TRUE(vd->is_assigned("reopt_scenario"));
    auto site = vd->lookup("reopt_scenario");

    auto settings = site->table.lookup("Settings");
    assert(settings->table.lookup("time_steps_per_hour")->num == 1);

    std::vector<std::string> sections = { "ElectricTariff", "ElectricLoad", "Financial", "ElectricStorage", "PV", "ElectricUtility"};
    for (const auto& s : sections)
        ASSERT_TRUE(site->table.is_assigned(s));

    auto reopt_utility = site->table.lookup("ElectricUtility");
    var_table* utility_table = &reopt_utility->table;

    std::vector<int> outage_start_times = utility_table->as_vector_integer("outage_start_time_steps");
    ASSERT_EQ(outage_start_times.size(), 2);
    ASSERT_EQ(outage_start_times[0], 8);
    ASSERT_EQ(outage_start_times[1], 19);

    std::vector<int> outage_durations = utility_table->as_vector_integer("outage_durations");
    ASSERT_EQ(outage_durations.size(), 2);
    ASSERT_EQ(outage_durations[0], 2);
    ASSERT_EQ(outage_durations[1], 6);
}

/// Integration test for lifetime vs year 1 outputs in SAM
TEST_F(CMPvsamv1PowerIntegration_cmod_pvsamv1, lifetime_outputs)
{
    ssc_data_clear(data);
    pvsamv1_with_residential_default(data);
    ssc_data_set_number(data, "system_use_lifetime_output", 1);
    ssc_data_set_number(data, "save_full_lifetime_variables", 1);
    std::map<std::string, std::string> pairs;
    pairs["solar_resource_file"] = solar_resource_path;

    //run the test
    int pvsam_errors = modify_ssc_data_and_run_module(data, "pvsamv1", pairs);

    EXPECT_FALSE(pvsam_errors);
    if (!pvsam_errors)
    {
        ssc_number_t annual_dc_module_loss_percent;
        ssc_data_get_number(data, "annual_dc_module_loss_percent", &annual_dc_module_loss_percent);
        EXPECT_NEAR(annual_dc_module_loss_percent, 6.381, 0.1) << "Module loss should reflect only year 1 DC gross energy";
    }

}


TEST_F(CMPvsamv1PowerIntegration_cmod_pvsamv1, NonAnnual)
{
    //set up a weather data array and unassign the solar resource file

    auto weather_data = create_weatherdata_array(24);
    ssc_data_unassign(data, "solar_resource_file");
    ssc_data_set_table(data, "solar_resource_data", weather_data);

    std::vector<double> load(24, 1);
    ssc_data_set_array(data, "load", &load[0], (int)load.size());

    //run the tests
    EXPECT_FALSE(run_module(data, "pvsamv1"));

    ssc_number_t dc_net, gen;
    dc_net = ssc_data_get_array(data, "dc_net", nullptr)[12];
    EXPECT_NEAR(dc_net, 3.213, 0.01) << "DC Net Energy at noon";

    gen = ssc_data_get_array(data, "gen", nullptr)[12];
    EXPECT_NEAR(gen, 3.078, 0.01) << "Gen at noon";

    //free the weather data
    free_weatherdata_array(weather_data);
}


//test non-annual run that includes Feb 29
TEST_F(CMPvsamv1PowerIntegration_cmod_pvsamv1, NonAnnualWithLeapDay)
{
    //set up a weather data array containing leap day and assign it to the solar resource data
    const int length = 24;
    double month[length] = { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };
    double day[length] = { 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29 };

    var_data month_vd = var_data(month, length);
    var_data day_vd = var_data(day, length);

    auto weather_data = create_weatherdata_array(length);
    weather_data->assign("month", month_vd);
    weather_data->assign("day", day_vd);

    ssc_data_unassign(data, "solar_resource_file");
    ssc_data_set_table(data, "solar_resource_data", weather_data);

    std::vector<double> load(length, 1);
    ssc_data_set_array(data, "load", &load[0], (int)load.size());

    //run the tests
    EXPECT_FALSE(run_module(data, "pvsamv1"));

    ssc_number_t dc_net, gen;
    dc_net = ssc_data_get_array(data, "dc_net", nullptr)[12];
    EXPECT_NEAR(dc_net, 2.7772, 0.01) << "DC Net Energy at noon";

    gen = ssc_data_get_array(data, "gen", nullptr)[12];
    EXPECT_NEAR(gen, 2.6623, 0.01) << "Gen at noon";
    free_weatherdata_array(weather_data);
}

//test single timestep that doesn't start at hour 0
TEST_F(CMPvsamv1PowerIntegration_cmod_pvsamv1, SingleTimestepNoon)
{
    //set up a weather data array containing a single data point at noon
    const int length = 1;
    double month[length] = { 6 };
    double day[length] = { 21 };
    double hour[length] = { 12 };
    double dn[length] = { 700 }; //set dn and df to a value to get power out
    double df[length] = { 100 };

    var_data month_vd = var_data(month, length);
    var_data day_vd = var_data(day, length);
    var_data hour_vd = var_data(hour, length);
    var_data dn_vd = var_data(dn, length);
    var_data df_vd = var_data(df, length);

    auto weather_data = create_weatherdata_array(length);
    weather_data->assign("month", month_vd);
    weather_data->assign("day", day_vd);
    weather_data->assign("hour", hour_vd);
    weather_data->assign("dn", dn_vd);
    weather_data->assign("df", df_vd);

    ssc_data_unassign(data, "solar_resource_file");
    ssc_data_set_table(data, "solar_resource_data", weather_data);

    std::vector<double> load(length, 1);
    ssc_data_set_array(data, "load", &load[0], (int)load.size());

    //run the tests
    EXPECT_FALSE(run_module(data, "pvsamv1"));

    ssc_number_t dc_net, gen;
    dc_net = ssc_data_get_array(data, "dc_net", nullptr)[0];
    EXPECT_NEAR(dc_net, 0.743, 0.01) << "DC Net Energy at noon";

    gen = ssc_data_get_array(data, "gen", nullptr)[0];
    EXPECT_NEAR(gen, 0.704, 0.01) << "Gen at noon";
    free_weatherdata_array(weather_data);
}


//a couple of 8760 weather data array tests, one base case and one that includes Feb 29 but not Dec 31
TEST_F(CMPvsamv1PowerIntegration_cmod_pvsamv1, WeatherDataCases)
{
    //set up a weather data array assign it to the solar resource data
    const int length = 8760;
    auto weather_data = create_weatherdata_array(length);
    ssc_data_unassign(data, "solar_resource_file");
    ssc_data_set_table(data, "solar_resource_data", weather_data);

    //assign the required load variable
    std::vector<double> load(length, 1);
    ssc_data_set_array(data, "load", &load[0], (int)load.size());

    //run a base case test
    EXPECT_FALSE(run_module(data, "pvsamv1"));
    ssc_number_t annualEnergy;
    ssc_data_get_number(data, "annual_energy", &annualEnergy);
    EXPECT_NEAR(annualEnergy, 5121.6, 1.0) << "Weather data base case test, annual energy produced by 8760 array";

    //now change the weather data array to include Feb 29 but not Dec 31, so still 8760 in length
    double month[length] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12 };
    double day[length] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30 };
    var_data month_vd = var_data(month, length);
    var_data day_vd = var_data(day, length);
    weather_data->assign("month", month_vd);
    weather_data->assign("day", day_vd);
    ssc_data_set_table(data, "solar_resource_data", weather_data);

    //run this new case
    EXPECT_FALSE(run_module(data, "pvsamv1"));
    ssc_data_get_number(data, "annual_energy", &annualEnergy);
    //annual energy should be a little different because the solar position angles are calculated at different times with the new dates, but not too different
    EXPECT_NEAR(annualEnergy, 5118.5, 1.0) << "Annual energy produced by 8760 with Feb29 without Dec 31";

    free_weatherdata_array(weather_data);
}


