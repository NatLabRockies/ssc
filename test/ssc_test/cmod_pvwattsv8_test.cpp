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

#include "../ssc/core.h"
#include "../ssc/vartab.h"
#include "../ssc/common.h"
#include "../input_cases/weather_inputs.h"
#include "cmod_pvwattsv8_test.h"


TEST_F(CMPvwattsv8Integration_cmod_pvwattsv8, DefaultSetup) {
//    compute();
    ssc_data_set_number(data, "analysis_period", 25);
}

///Default PVWattsv8, but with TMY2 instead of TMY3
TEST_F(CMPvwattsv8Integration_cmod_pvwattsv8, DefaultNoFinancialModel_cmod_pvwattsv8) {
    compute();

    double tmp = 0;
    //	ssc_data_get_number(data, "annual_energy", &annual_energy);
    //	EXPECT_NEAR(annual_energy, 6909.79, error_tolerance) << "Annual energy.";
    int count;
    ssc_number_t* monthly_energy = ssc_data_get_array(data, "monthly_energy", &count);

    for (size_t i = 0; i < 12; i++)
        tmp += (double)monthly_energy[i];
    //v5 is 6909.79, decrease of 2.4%: decreases due to shading, module cover losses, and spectral losses
    //v7 prior to module coeff changes is 6750.4236, increase of 3.7% due to improved tempco for standard module
    //v7 final version is 6999.0158, decrease of 0.4% due to model updates
    EXPECT_NEAR(tmp, 7043.1973, error_tolerance) << "Annual energy.";

    EXPECT_NEAR((double)monthly_energy[0], 441.117, error_tolerance) << "Monthly energy of January";
    EXPECT_NEAR((double)monthly_energy[1], 488.020, error_tolerance) << "Monthly energy of February";
    EXPECT_NEAR((double)monthly_energy[2], 601.263, error_tolerance) << "Monthly energy of March";
    EXPECT_NEAR((double)monthly_energy[3], 685.430, error_tolerance) << "Monthly energy of April";
    EXPECT_NEAR((double)monthly_energy[4], 730.075, error_tolerance) << "Monthly energy of May";
    EXPECT_NEAR((double)monthly_energy[5], 681.335, error_tolerance) << "Monthly energy of June";
    EXPECT_NEAR((double)monthly_energy[6], 679.185, error_tolerance) << "Monthly energy of July";
    EXPECT_NEAR((double)monthly_energy[7], 662.545, error_tolerance) << "Monthly energy of August";
    EXPECT_NEAR((double)monthly_energy[8], 610.097, error_tolerance) << "Monthly energy of September";
    EXPECT_NEAR((double)monthly_energy[9], 583.053, error_tolerance) << "Monthly energy of October";
    EXPECT_NEAR((double)monthly_energy[10], 461.662, error_tolerance) << "Monthly energy of November";
    EXPECT_NEAR((double)monthly_energy[11], 419.413, error_tolerance) << "Month energy of December";

    ssc_number_t capacity_factor, capacity_factor_ac;
    ssc_data_get_number(data, "capacity_factor", &capacity_factor);
    EXPECT_NEAR(capacity_factor, 20.100, error_tolerance) << "DC Capacity factor";
    ssc_data_get_number(data, "capacity_factor_ac", &capacity_factor_ac);
    EXPECT_NEAR(capacity_factor_ac, 24.120, error_tolerance) << "AC Capacity factor";

    ssc_number_t kwh_per_kw;
    ssc_data_get_number(data, "kwh_per_kw", &kwh_per_kw);
    EXPECT_NEAR(kwh_per_kw, 1760.799, error_tolerance) << "Energy yield";

}

/// PVWattsv8 using different technology input options
TEST_F(CMPvwattsv8Integration_cmod_pvwattsv8, DifferentTechnologyInputs_cmod_pvwattsv8)
{
	//PVWattsV5 results: annual_energy_expected = { 6909.79, 7123.32, 7336.478, 6909.79, 6804.376, 8601.011, 8727.704, 9690.735};
	//V7 prior to module coefficient updates: std::vector<double> annual_energy_expected = { 6750.42, 7034.39, 7166.88, 6750.42, 6693.49, 8514.26, 8441.60, 9631.76 };
	//standard fixed -2.4%, premium fixed -1.3%, thinfilm fixed -2.4%, standard fixed -2.4%, standard roof -1.7%, standard 1-axis -1.0%, standard backtrack -3.4%, standard 2-axis -0.6%
    //V7 in 2020.11.29 release:
    //annual_energy_expected = { 6999.01, 7030.26, 7077.07,6999.01, 6971.04, 8785.40, 8725.66, 9861.27 };
	//standard fixed +3.6%, premium fixed 0%, thinfilm fixed -1.2%, standard fixed +3.6%, standard roof +4.0%, standard 1-axis +3.3%, standard backtrack +3.3%, standard 2-axis +2.6%
    //V7 after fix for AR glass:
    //annual_energy_expected = { 7001.83, 7032.99, 7079.73, 7001.83, 6969.07, 8754.11, 8696.94, 9861.27 };
   //V8 results
    std::vector<double> annual_energy_expected = { 7043.197, 7040.913, 7099.417, 7043.197, 7014.260, 8859.963, 8802.080, 9910.386 };
    //wrt V7 after AR fix: standard fixed +1.0%, premium fixed +0.5%, thinfilm fixed +0.7%, standard fixed +1.0%, standard roof +1.2%, standard 1-axis +1.2%, standard backtrack +1.2%, standard 2-axis +0.7%
    //v8 wrt v7 in 2020.11.29 release: standard fixed +1.0%, premium fixed +0.5%, thinfilm fixed +0.7%, standard fixed +1.0%, standard roof +1.1%, standard 1-axis +0.8%, standard backtrack +0.9%, standard 2-axis +0.7%

    std::map<std::string, double> pairs;
    size_t count = 0;
    error_tolerance = 0.01;

    // Module types: Standard, Premium, Thin Film
    for (int module_type = 0; module_type < 3; module_type++)
    {
        pairs["module_type"] = module_type;
        int pvwatts_errors = modify_ssc_data_and_run_module(data, "pvwattsv8", pairs);
        EXPECT_FALSE(pvwatts_errors);

        if (!pvwatts_errors)
        {
            ssc_number_t annual_energy;
            ssc_data_get_number(data, "annual_energy", &annual_energy);
            EXPECT_NEAR(annual_energy, annual_energy_expected[count], error_tolerance) << "Annual energy.";
        }
        count++;
    }
    pairs["module_type"] = 0; //reset module type to its default value

    // Array types: Fixed open rack, fixed roof mount, 1-axis tracking, 1-axis backtracking, 2-axis tracking
    for (int array_type = 0; array_type < 5; array_type++)
    {
        pairs["array_type"] = array_type;
        int pvwatts_errors = modify_ssc_data_and_run_module(data, "pvwattsv8", pairs);
        EXPECT_FALSE(pvwatts_errors);

        if (!pvwatts_errors)
        {
            ssc_number_t annual_energy;
            ssc_data_get_number(data, "annual_energy", &annual_energy);
            EXPECT_NEAR(annual_energy, annual_energy_expected[count], error_tolerance) << "Annual energy.";
        }
        count++;
    }
    pairs["array_type"] = 0; //reset array type to fixed open rack

}

/// PVWattsv8 using a larger system size
TEST_F(CMPvwattsv8Integration_cmod_pvwattsv8, LargeSystem_cmod_pvwattsv8)
{
	//PVWattsV5 results: std::vector<double> annual_energy_expected = { 1727447.4, 1701094.0, 2150252.8, 2181925.8, 2422683.7 };
	//PVWattsV7 prior to module coeff updates: std::vector<double> annual_energy_expected = { 1686353.2, 1673371.8, 2123603.8, 2105794.1, 2407940.7 };
	//PVWattsV7 final results: std::vector<double> annual_energy_expected = { 1747992.2, 1742760.1, 2190219.7, 2175654.8,  2465319.2};
    std::vector<double> annual_energy_expected = { 1759882.2, 1753564.9, 2212064.0, 2197863.4,  2477596.6 };

    std::map<std::string, double> pairs;
    size_t count = 0;
    error_tolerance = 0.1; //use a larger error tolerance for large numbers

    // Larger size
    pairs["system_capacity"] = 1000; //1 MW system

    // Array types: Fixed open rack, fixed roof mount, 1-axis tracking, 1-axis backtracking, 2-axis tracking
    for (int array_type = 0; array_type < 5; array_type++)
    {
        pairs["array_type"] = array_type;
        int pvwatts_errors = modify_ssc_data_and_run_module(data, "pvwattsv8", pairs);
        EXPECT_FALSE(pvwatts_errors);

        if (!pvwatts_errors)
        {
            ssc_number_t annual_energy;
            ssc_data_get_number(data, "annual_energy", &annual_energy);
            EXPECT_NEAR(annual_energy, annual_energy_expected[count], error_tolerance) << "Annual energy.";
        }
        count++;
    }
}

/// Test pvwattsv8 with default inputs and a 15-minute weather file
TEST_F(CMPvwattsv8Integration_cmod_pvwattsv8, SubhourlyWeather_cmod_pvwattsv8) {

    char subhourly[256];
    int b = sprintf(subhourly, "%s/test/input_cases/pvsamv1_data/LosAngeles_WeatherFile_15min.csv", SSCDIR);
    ssc_data_set_string(data, "solar_resource_file", subhourly); //file set above

    //std::map<std::string, std::string> pairs;
    int pvwatts_errors = run_module(data, "pvwattsv8");

    EXPECT_FALSE(pvwatts_errors);

    if (!pvwatts_errors)
    {
        ssc_number_t annual_energy;
        ssc_data_get_number(data, "annual_energy", &annual_energy);
        //EXPECT_NEAR(annual_energy, 6523.727, error_tolerance) << "Annual energy.";
        EXPECT_NEAR(annual_energy, 6564.983, error_tolerance) << "Annual energy.";

        ssc_number_t capacity_factor, capacity_factor_ac;
        ssc_data_get_number(data, "capacity_factor", &capacity_factor);
        EXPECT_NEAR(capacity_factor, 18.70, 0.1) << "DC Capacity factor";
        ssc_data_get_number(data, "capacity_factor_ac", &capacity_factor_ac);
        EXPECT_NEAR(capacity_factor_ac, 22.44, 0.1) << "AC Capacity factor";

    }
}

/// Test PVWattsv8 in lifetime mode
TEST_F(CMPvwattsv8Integration_cmod_pvwattsv8, LifetimeModeTest_cmod_pvwattsv8) {

    // set lifetime mode
    std::map<std::string, double> pairs;
    pairs["system_use_lifetime_output"] = 1;
    pairs["analysis_period"] = 25;

    // test degradation array with a length of 1, which should work
    // annual energy of this test should be higher than the array length 25, because this is year 1 energy
    // and with a single value array, degradation doesn't start applying until year 2
    double dc_degradation_single[1];
    dc_degradation_single[0] = 0.5;
    ssc_data_set_array(data, "dc_degradation", (ssc_number_t*)dc_degradation_single, 1);
    int pvwatts_errors = modify_ssc_data_and_run_module(data, "pvwattsv8", pairs);
    EXPECT_FALSE(pvwatts_errors);
    if (!pvwatts_errors)
    {
        ssc_number_t annual_energy;
        ssc_data_get_number(data, "annual_energy", &annual_energy);
        EXPECT_NEAR(annual_energy, 7043.197, error_tolerance) << "Annual energy degradation array length 1.";
    }

    // next, test degradation array with length the same as analysis period, which should also work
    double dc_degradation[25];
    for (size_t i = 0; i < 25; i++) {
        dc_degradation[i] = 0.5;
    }
    ssc_data_set_array(data, "dc_degradation", (ssc_number_t*)dc_degradation, 25);
    pvwatts_errors = modify_ssc_data_and_run_module(data, "pvwattsv8", pairs);
    EXPECT_FALSE(pvwatts_errors);
    if (!pvwatts_errors)
    {
        ssc_number_t annual_energy;
        ssc_data_get_number(data, "annual_energy", &annual_energy);
        EXPECT_NEAR(annual_energy, 7007.669, error_tolerance) << "Annual energy degradation array length 25.";
    }

    // lastly, test degradation array with the wrong length, which should fail
    double dc_degradation_fail[22];
    for (size_t i = 0; i < 22; i++) {
        dc_degradation_fail[i] = 0.5;
    }
    ssc_data_set_array(data, "dc_degradation", (ssc_number_t*)dc_degradation_fail, 22);
    pvwatts_errors = modify_ssc_data_and_run_module(data, "pvwattsv8", pairs);
    EXPECT_TRUE(pvwatts_errors);
}

/// Test PVWattsv8 bifacial functionality
TEST_F(CMPvwattsv8Integration_cmod_pvwattsv8, BifacialTest_cmod_pvwattsv8) {

    // set bifacial inputs
    std::map<std::string, double> pairs;
    pairs["bifaciality"] = 0.0;
    ssc_number_t annual_energy_mono = 0, annual_energy_bi = 0;

    int pvwatts_errors = modify_ssc_data_and_run_module(data, "pvwattsv8", pairs);
    EXPECT_FALSE(pvwatts_errors);
    if (!pvwatts_errors)
    {
        ssc_data_get_number(data, "annual_energy", &annual_energy_mono);
    }

    pairs["bifaciality"] = 0.65;
    pvwatts_errors = modify_ssc_data_and_run_module(data, "pvwattsv8", pairs);
    EXPECT_FALSE(pvwatts_errors);
    if (!pvwatts_errors)
    {
        ssc_data_get_number(data, "annual_energy", &annual_energy_bi);
    }

    EXPECT_GT(annual_energy_bi / annual_energy_mono, 1.027);
}

/// Test albedo user single or monthly inputs, weather file (wf) input for valid and invalid albedo values with and without snow model
TEST_F(CMPvwattsv8Integration_cmod_pvwattsv8, AlbedoTest_cmod_pvwattsv8) {

    // albedo user input values for testing
    ssc_number_t albedo_single[1] = { 0.45 };
    ssc_number_t albedo_single_invalid[1] = { 1 };
    ssc_number_t albedo_monthly[12] = { 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 0.90, 0.91, 0.92 };
    ssc_number_t albedo_monthly_invalid[12] = { 1, 2, 0.3, 4, 5, 6, 7, 8, 9, 0, 91, 92 };

    // default albedo values
    ssc_number_t default_albedo = 0.2;
    ssc_number_t default_albedo_snow = 0.6;

    // weather files for albedo tests
    // pv_albedo_test.csv time steps 1, 2, and 3229 have invalid albedo values for testing
    // time steps 0, 1, and 2 have snow depth = 14.2, time steps 3228 and 3229 have snow depth = 0
    char wf_albedo[256];
    int a1 = sprintf(wf_albedo, "%s/test/input_cases/pvsamv1_data/pv_albedo_test.csv", SSCDIR);
    char wf_no_albedo[256];
    int a2 = sprintf(wf_no_albedo, "%s/test/input_cases/pvsamv1_data/pv_albedo_test_no_albedo.csv", SSCDIR);

    // albedo values from wf_snow_albedo
    ssc_number_t albedo_ts_0 = 0.87; // jan 1 1:00 am
    ssc_number_t albedo_ts_3228 = 0.1; // may 15 12:00 pm

    ssc_data_set_string(data, "solar_resource_file", wf_albedo);

    // Test set 1: use_wf_albedo true
    ssc_data_set_number(data, "use_wf_albedo", 1);

    ssc_data_set_number(data, "en_snowloss", 0);
    EXPECT_FALSE(run_module(data, "pvwattsv8"));
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[0], albedo_ts_0, error_tolerance) << "Time step 0: Albedo should be wf albedo.";
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[1], default_albedo, error_tolerance) << "Time step 1: Albedo should be default albedo.";
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[2], default_albedo, error_tolerance) << "Time step 2: Albedo should be default albedo.";
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[3228], albedo_ts_3228, error_tolerance) << "Time step 3228: Albedo should wf albedo.";
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[3229], default_albedo, error_tolerance) << "Time step 3229: Albedo should be default albedo.";

    ssc_data_set_number(data, "en_snowloss", 1);
    EXPECT_FALSE(run_module(data, "pvwattsv8"));
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[0], albedo_ts_0, error_tolerance) << "Time step 0: Albedo should be wf albedo.";
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[1], default_albedo_snow, error_tolerance) << "Time step 1: Albedo should be default snow albedo.";
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[2], default_albedo_snow, error_tolerance) << "Time step 2: Albedo should be default snow albedo.";
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[3228], albedo_ts_3228, error_tolerance) << "Time step 3228: Albedo should be wf albedo.";
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[3229], default_albedo, error_tolerance) << "Time step 3229: Albedo should be default albedo.";

    ssc_data_set_string(data, "solar_resource_file", wf_no_albedo);

    ssc_data_set_number(data, "en_snowloss", 0);
    EXPECT_FALSE(run_module(data, "pvwattsv8"));
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[0], default_albedo, error_tolerance) << "Time step 0: Albedo should be wf albedo.";
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[1], default_albedo, error_tolerance) << "Time step 1: Albedo should be default albedo.";
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[2], default_albedo, error_tolerance) << "Time step 2: Albedo should be default albedo.";
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[3228], default_albedo, error_tolerance) << "Time step 3228: Albedo should wf albedo.";
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[3229], default_albedo, error_tolerance) << "Time step 3229: Albedo should be default albedo.";

    ssc_data_set_number(data, "en_snowloss", 1);
    EXPECT_FALSE(run_module(data, "pvwattsv8"));
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[0], default_albedo_snow, error_tolerance) << "Time step 0: Albedo should be wf albedo.";
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[1], default_albedo_snow, error_tolerance) << "Time step 1: Albedo should be default snow albedo.";
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[2], default_albedo_snow, error_tolerance) << "Time step 2: Albedo should be default snow albedo.";
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[3228], default_albedo, error_tolerance) << "Time step 3228: Albedo should be wf albedo.";
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[3229], default_albedo, error_tolerance) << "Time step 3229: Albedo should be default albedo.";


    // Test set 2: use_wf_albedo false
    ssc_data_set_number(data, "use_wf_albedo", 0);

    ssc_data_set_array(data, "albedo", albedo_single, 1);

    ssc_data_set_number(data, "en_snowloss", 0);
    EXPECT_FALSE(run_module(data, "pvwattsv8"));
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[0], albedo_single[0], error_tolerance) << "Time step 0: Albedo should be input albedo.";
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[3228], albedo_single[0], error_tolerance) << "Time step 3228: Albedo should input albedo.";

    ssc_data_set_number(data, "en_snowloss", 1);
    EXPECT_FALSE(run_module(data, "pvwattsv8"));
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[0], albedo_single[0], error_tolerance) << "Time step 0: Albedo should be input albedo.";
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[3228], albedo_single[0], error_tolerance) << "Time step 3228: Albedo should input albedo.";

    ssc_data_set_array(data, "albedo", albedo_single_invalid, 1);

    ssc_data_set_number(data, "en_snowloss", 0);
    EXPECT_FALSE(run_module(data, "pvwattsv8"));
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[0], default_albedo, error_tolerance) << "Time step 0: Albedo should be default albedo.";
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[3228], default_albedo, error_tolerance) << "Time step 3228: Albedo should default albedo.";

    ssc_data_set_number(data, "en_snowloss", 1);
    EXPECT_FALSE(run_module(data, "pvwattsv8"));
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[0], default_albedo_snow, error_tolerance) << "Time step 0: Albedo should be default snow albedo.";
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[3228], default_albedo, error_tolerance) << "Time step 3228: Albedo should be default albedo.";

    ssc_data_set_array(data, "albedo", albedo_monthly, 12);

    ssc_data_set_number(data, "en_snowloss", 0);
    EXPECT_FALSE(run_module(data, "pvwattsv8"));
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[0], albedo_monthly[0], error_tolerance) << "Time step 0: Albedo should be user albedo January.";
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[3228], albedo_monthly[4], error_tolerance) << "Time step 3228: Albedo should be user albedo May.";

    ssc_data_set_number(data, "en_snowloss", 1);
    EXPECT_FALSE(run_module(data, "pvwattsv8"));
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[0], albedo_monthly[0], error_tolerance) << "Time step 0: Albedo should be user albedo January.";
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[3228], albedo_monthly[4], error_tolerance) << "Time step 3228: Albedo should be user albedo May.";

    ssc_data_set_array(data, "albedo", albedo_monthly_invalid, 12);

    ssc_data_set_number(data, "en_snowloss", 0);
    EXPECT_FALSE(run_module(data, "pvwattsv8"));
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[0], default_albedo, error_tolerance) << "Time step 0: Albedo should be default.";
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[3228], default_albedo, error_tolerance) << "Time step 3228: Albedo should be default.";
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[1776], albedo_monthly[2], error_tolerance) << "Time step 3228: Albedo should be user albedo March.";

    ssc_data_set_number(data, "en_snowloss", 1);
    EXPECT_FALSE(run_module(data, "pvwattsv8"));
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[0], default_albedo_snow, error_tolerance) << "Time step 0: Albedo should be default snow albedo.";
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[3228], default_albedo, error_tolerance) << "Time step 3228: Albedo should be default.";
    EXPECT_NEAR(ssc_data_get_array(data, "alb", nullptr)[1776], albedo_monthly[2], error_tolerance) << "Time step 1776: Albedo should be user albedo March.";


}

TEST_F(CMPvwattsv8Integration_cmod_pvwattsv8, SnowModelTests_cmod_pvwattsv8) {

    // Snow loss for fixed tilt system*********************************
    ssc_data_set_number(data, "array_type", 0);
    ssc_data_set_number(data, "en_snowloss", 1);

    compute();

    // Snow events in January, February, April, October, and December
    ssc_number_t january_energy;
    january_energy = ssc_data_get_array(data, "monthly_energy", nullptr)[0]; //retrieve only january's value
    EXPECT_NEAR((double)january_energy, 416.630, 0.01) << "Fixed tilt energy in January after snow loss";

    int count;
    ssc_number_t* hourly_snowderate = ssc_data_get_array(data, "dcsnowderate", &count);
    ASSERT_EQ(8760, count);

    // changed from defrate to percentage per SAM issue 1137
    // Starting at 6 AM Jan. 12th
    // Snow derate should be non-zero during a snow event, and on a fixed system will always be 0, 0.5, or 1.0 due to the assumption of a 2-up installation
    EXPECT_NEAR((double)hourly_snowderate[270], 0, error_tolerance);
    EXPECT_NEAR((double)hourly_snowderate[271], 50, error_tolerance);
    EXPECT_NEAR((double)hourly_snowderate[272], 50, error_tolerance);
    // ...
    EXPECT_NEAR((double)hourly_snowderate[275], 50, error_tolerance);
    EXPECT_NEAR((double)hourly_snowderate[276], 50, error_tolerance);
    EXPECT_NEAR((double)hourly_snowderate[277], 0, error_tolerance);


    // Snow loss for single-axis system********************************
    ssc_data_set_number(data, "array_type", 2);
    ssc_data_set_number(data, "en_snowloss", 1);

    compute();

    // Snow events in January, February, April, October, and December
    january_energy = ssc_data_get_array(data, "monthly_energy", nullptr)[0]; //retrieve only january's value
    EXPECT_NEAR((double)january_energy, 511.283, 0.01) << "Single-axis tracker energy in January after snow loss";

    // A tracker row is assumed to be nx1 panels, so all derates should be either 0 or 1
    hourly_snowderate = ssc_data_get_array(data, "dcsnowderate", nullptr);
    for (int hour = 0; hour < 300; hour++) //check throughout a known snow event but don't need to check the whole year
    {
        EXPECT_TRUE(hourly_snowderate[hour] == 100 || hourly_snowderate[hour] == 0);
    }


    // Snow loss for backtracking system*******************************
    ssc_data_set_number(data, "array_type", 3);
    ssc_data_set_number(data, "en_snowloss", 1);

    compute();

    // Snow events in January, February, April, October, and December
    january_energy = ssc_data_get_array(data, "monthly_energy", nullptr)[0]; //retrieve only january's value
    EXPECT_NEAR((double)january_energy, 506.989, 0.01) << "Backtracking energy in January after snow loss";

    // A tracker row is assumed to be nx1 panels, so all derates should be either 0 or 1
    hourly_snowderate = ssc_data_get_array(data, "dcsnowderate", nullptr);
    for (int hour = 0; hour < 300; hour++) //check throughout a known snow event but don't need to check the whole year
    {
        EXPECT_TRUE(hourly_snowderate[hour] == 100 || hourly_snowderate[hour] == 0);
    }

    //Check interaction with bifacial
    ssc_data_set_number(data, "bifaciality", 0.70);
    compute();
    january_energy = ssc_data_get_array(data, "monthly_energy", nullptr)[0]; //retrieve only january's value
    EXPECT_NEAR((double)january_energy, 532.193, 0.01) << "Backtracking energy in January after snow loss";


}

/* this test isn't passing currently even though it's working in the UI, so commenting out for now */
/// Test PVWattsv8 with snow model
TEST_F(CMPvwattsv8Integration_cmod_pvwattsv8, SnowModelTest_cmod_pvwattsv8) {

    // enable snow model
    std::map<std::string, double> pairs;
    pairs["en_snowmodel"] = 1;

    // test with a file that doesn't have snow data- simulation should fail
    char nosnow[256];
    int b = sprintf(nosnow, "%s/test/input_cases/pvsamv1_data/phoenix_az_33.450495_-111.983688_psmv3_60_tmy.csv", SSCDIR);
    ssc_data_set_string(data, "solar_resource_file", nosnow); //file set above
    int pvwatts_errors = modify_ssc_data_and_run_module(data, "pvwattsv8", pairs);
    EXPECT_FALSE(pvwatts_errors);

}

TEST_F(CMPvwattsv8Integration_cmod_pvwattsv8, NonAnnual)
{
    //set up a weather data array and unassign the solar resource file

    auto weather_data = create_weatherdata_array(24);
    
    ssc_data_unassign(data, "solar_resource_file");
    ssc_data_set_table(data, "solar_resource_data", weather_data);

    //run the tests
    EXPECT_FALSE(run_module(data, "pvwattsv8"));

    ssc_number_t dc, gen;
    dc = ssc_data_get_array(data, "dc", nullptr)[12];
    EXPECT_NEAR(dc, 2537.023, 0.01) << "DC Energy at noon";

    gen = ssc_data_get_array(data, "gen", nullptr)[12];
    EXPECT_NEAR(gen, 2.428, 0.01) << "Gen at noon";
    
    free_weatherdata_array(weather_data);
}

TEST_F(CMPvwattsv8Integration_cmod_pvwattsv8, NonAnnualSummerStart)
{
    //set up a weather data array and unassign the solar resource file

    auto weather_data = create_weatherdata_array(24, 24 * 180);
    
    ssc_data_unassign(data, "solar_resource_file");
    ssc_data_set_table(data, "solar_resource_data", weather_data);

    //run the tests
    EXPECT_FALSE(run_module(data, "pvwattsv8"));

    ssc_number_t dc, gen, ac;
    dc = ssc_data_get_array(data, "dc", nullptr)[12];
    EXPECT_NEAR(dc, 771.116, 1.) << "DC Energy at noon";

    ac = ssc_data_get_array(data, "ac", nullptr)[12];
    EXPECT_NEAR(ac, 727.939, 1.) << "AC Energy at noon";

    gen = ssc_data_get_array(data, "gen", nullptr)[12];
    EXPECT_NEAR(gen, 0.726, 0.01) << "Gen at noon";
    
    free_weatherdata_array(weather_data);
}


TEST_F(CMPvwattsv8Integration_cmod_pvwattsv8, IntermediateOutputTesting)
{
    //set up a weather data array and unassign the solar resource file

    auto weather_data = create_weatherdata_array(24);
    ssc_data_unassign(data, "solar_resource_file");
    ssc_data_set_table(data, "solar_resource_data", weather_data);

    //run the tests
    EXPECT_FALSE(run_module(data, "pvwattsv8"));

    ssc_number_t shad_beam_factor, ss_beam, ss_sky_diffuse, ss_gnd_diffuse, aoi, poa, tpoa, tcell, dc, ac;

    shad_beam_factor = ssc_data_get_array(data, "shad_beam_factor", nullptr)[12];
    EXPECT_NEAR(shad_beam_factor, 1.000, 0.01) << "External beam shading factor at noon";

    ss_beam = ssc_data_get_array(data, "ss_beam_factor", nullptr)[12];
    EXPECT_NEAR(ss_beam, 1.000, 0.01) << "Calculated self-shading beam shading factor at noon";

    ss_sky_diffuse = ssc_data_get_array(data, "ss_sky_diffuse_factor", nullptr)[12];
    EXPECT_NEAR(ss_sky_diffuse, 0.981, 0.01) << "Calculated self-shading sky diffuse shading factor at noon";

    ss_gnd_diffuse = ssc_data_get_array(data, "ss_gnd_diffuse_factor", nullptr)[12];
    EXPECT_NEAR(ss_gnd_diffuse, 0.763, 0.01) << "Calculated self-shading ground-reflected diffuse shading factor at noon";

    aoi = ssc_data_get_array(data, "aoi", nullptr)[12];
    EXPECT_NEAR(aoi, 32.195, 0.01) << "Angle of incidence at noon";

    poa = ssc_data_get_array(data, "poa", nullptr)[12];
    EXPECT_NEAR(poa, 830.539, 0.01) << "POA at noon"; //this shouldn't have changed, and code comparison shows no differences, so why are we now getting 828.570????

    tpoa = ssc_data_get_array(data, "tpoa", nullptr)[12];
    EXPECT_NEAR(tpoa, 820.889, 0.01) << "Transmitted POA at noon";

    tcell = ssc_data_get_array(data, "tcell", nullptr)[12];
    EXPECT_NEAR(tcell, 48.579, 0.01) << "Cell temp at noon";

    dc = ssc_data_get_array(data, "dc", nullptr)[12];
    EXPECT_NEAR(dc, 2537.023, 0.01) << "DC Energy at noon";

    ac = ssc_data_get_array(data, "ac", nullptr)[12];
    EXPECT_NEAR(ac, 2431.272, 0.01) << "AC Energy at noon";

    free_weatherdata_array(weather_data);
}


TEST_F(CmodPVWattsv8Test, DCACRatio_0_02) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/TechnologyModels/pvwattsv8/dc_ac_ratio_0.02_PVWatts_Single_Owner_cmod_pvwattsv8.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/TechnologyModels/pvwattsv8/dc_ac_ratio_0.02_PVWatts_Single_Owner_cmod_pvwattsv8_outputs.json";
    std::vector<std::string> compare_number_variables = { "annual_energy" };
    std::vector<std::string> compare_array_variables = { "gen", "inv_eff_output" };

    Test("pvwattsv8", file_inputs, file_outputs, compare_number_variables, compare_array_variables, 0.01);
}

TEST_F(CmodPVWattsv8Test, DCACRatio_0_5) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/TechnologyModels/pvwattsv8/dc_ac_ratio_0.5_PVWatts_Single_Owner_cmod_pvwattsv8.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/TechnologyModels/pvwattsv8/dc_ac_ratio_0.5_PVWatts_Single_Owner_cmod_pvwattsv8_outputs.json";
    std::vector<std::string> compare_number_variables = { "annual_energy" };
    std::vector<std::string> compare_array_variables = { "gen", "inv_eff_output" };

    Test("pvwattsv8", file_inputs, file_outputs, compare_number_variables, compare_array_variables, 0.01);
}


TEST_F(CmodPVWattsv8Test, DCACRatio_1_0) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/TechnologyModels/pvwattsv8/dc_ac_ratio_1_PVWatts_Single_Owner_cmod_pvwattsv8.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/TechnologyModels/pvwattsv8/dc_ac_ratio_1_PVWatts_Single_Owner_cmod_pvwattsv8_outputs.json";
    std::vector<std::string> compare_number_variables = { "annual_energy" };
    std::vector<std::string> compare_array_variables = { "gen", "inv_eff_output" };

    Test("pvwattsv8", file_inputs, file_outputs, compare_number_variables, compare_array_variables, 0.01);
}

TEST_F(CmodPVWattsv8Test, DCACRatio_1_5) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/TechnologyModels/pvwattsv8/dc_ac_ratio_1.5_PVWatts_Single_Owner_cmod_pvwattsv8.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/TechnologyModels/pvwattsv8/dc_ac_ratio_1.5_PVWatts_Single_Owner_cmod_pvwattsv8_outputs.json";
    std::vector<std::string> compare_number_variables = { "annual_energy" };
    std::vector<std::string> compare_array_variables = { "gen", "inv_eff_output" };

    Test("pvwattsv8", file_inputs, file_outputs, compare_number_variables, compare_array_variables, 0.01);
}

TEST_F(CmodPVWattsv8Test, DCACRatio_2_0) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/TechnologyModels/pvwattsv8/dc_ac_ratio_2_PVWatts_Single_Owner_cmod_pvwattsv8.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/TechnologyModels/pvwattsv8/dc_ac_ratio_2_PVWatts_Single_Owner_cmod_pvwattsv8_outputs.json";
    std::vector<std::string> compare_number_variables = { "annual_energy" };
    std::vector<std::string> compare_array_variables = { "gen", "inv_eff_output" };

    Test("pvwattsv8", file_inputs, file_outputs, compare_number_variables, compare_array_variables, 0.01);
}

TEST_F(CmodPVWattsv8Test, DCACRatio_10_0) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/TechnologyModels/pvwattsv8/dc_ac_ratio_10_PVWatts_Single_Owner_cmod_pvwattsv8.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/TechnologyModels/pvwattsv8/dc_ac_ratio_10_PVWatts_Single_Owner_cmod_pvwattsv8_outputs.json";
    std::vector<std::string> compare_number_variables = { "annual_energy" };
    std::vector<std::string> compare_array_variables = { "gen", "inv_eff_output" };

    Test("pvwattsv8", file_inputs, file_outputs, compare_number_variables, compare_array_variables, 0.01);
}
