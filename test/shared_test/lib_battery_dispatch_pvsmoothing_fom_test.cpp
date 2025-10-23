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

#include "lib_battery_dispatch_pvsmoothing_fom_test.h"

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <cmath>
#include "../rapidjson/document.h"
#include "../rapidjson/istreamwrapper.h"
 
// more can be added but these tests take a while...
TEST_F(PVSmoothing_lib_battery_dispatch, PV_Phoenix_all_on) {

    char file_path[256];
    int nfc1 = sprintf(file_path, "%s/test/input_cases/general_data/pvsmoothing_Phoenix_Validation_alloptions.json", SSCDIR);
    std::ifstream file(file_path);
    std::ostringstream tmp;
    tmp << file.rdbuf();
    file.close();
    ssc_data_t dat = json_to_ssc_data(tmp.str().c_str());
    tmp.str("");
    // setup path to weather file
    nfc1 = sprintf(file_path, "%s/test/input_cases/general_data/phoenix_az_33.450495_-111.983688_psmv3_60_tmy_1mInterpolated.csv", SSCDIR);
    ssc_data_set_string(dat, "solar_resource_file", file_path);



    // Run with fixed output
    int errors = run_module(dat, "pvsamv1");
    // minimize memory usage for Travis
 //   errors = run_module(dat, "grid");
 //   errors = run_module(dat, "utilityrate5");
 //   errors = run_module(dat, "singleowner");

    EXPECT_FALSE(errors);
    if (!errors)
    {
        ssc_number_t violation_count, violation_percent, grid_percent, grid_percent_sam;
        ssc_data_get_number(dat, "batt_pvs_violation_count", &violation_count);
        ssc_data_get_number(dat, "batt_pvs_violation_percent", &violation_percent);
        ssc_data_get_number(dat, "batt_pvs_energy_to_grid_percent", &grid_percent);
        ssc_data_get_number(dat, "batt_pvs_energy_to_grid_percent_sam", &grid_percent_sam);

        // values from Phoenix_Validation_testing.py EPRI code
        EXPECT_NEAR(violation_count, 15, 0.001);
        EXPECT_NEAR(violation_percent, 0.0285, 0.001);
        EXPECT_NEAR(grid_percent, 99.89, 0.01);
        EXPECT_NEAR(grid_percent_sam, 98.89, 0.01);
    }
    ssc_data_free(dat);
    dat = nullptr;
}

TEST_F(PVSmoothing_lib_battery_dispatch, Generic_w_PV_input_all_on) {

    char file_path[256];
    int nfc1 = sprintf(file_path, "%s/test/input_cases/general_data/pvsmoothing_Generic_alloptions.json", SSCDIR);
    std::ifstream file(file_path);
    std::ostringstream tmp;
    tmp << file.rdbuf();
    file.close();
    ssc_data_t dat = json_to_ssc_data(tmp.str().c_str());
    tmp.str("");

    // Run with fixed output
//    auto mod1 = ssc_module_create("generic_system");

    int errors = run_module(dat, "custom_generation");
    errors += run_module(dat, "battery");
    // minimize memory usage for Travis
//    errors = run_module(dat, "grid");
//    errors = run_module(dat, "utilityrate5");
//    errors = run_module(dat, "singleowner");

    EXPECT_FALSE(errors);
    if (!errors)
    {
        ssc_number_t violation_count, violation_percent, grid_percent, grid_percent_sam;
        ssc_data_get_number(dat, "batt_pvs_violation_count", &violation_count);
        ssc_data_get_number(dat, "batt_pvs_violation_percent", &violation_percent);
        ssc_data_get_number(dat, "batt_pvs_energy_to_grid_percent", &grid_percent);
        ssc_data_get_number(dat, "batt_pvs_energy_to_grid_percent_sam", &grid_percent_sam);

        // values from Phoenix_Validation_testing.py EPRI code
        EXPECT_NEAR(violation_count, 13, 0.001);
        EXPECT_NEAR(violation_percent, 0.025, 0.001);
        EXPECT_NEAR(grid_percent, 99.90, 0.01);
        EXPECT_NEAR(grid_percent_sam, 99.90, 0.01);
    }
    ssc_data_free(dat);
    dat = nullptr;
}
/*
TEST_F(PVSmoothing_lib_battery_dispatch, FuelCell_PV_Phoenix_all_on) {

    char file_path[256];
    int nfc1 = sprintf(file_path, "%s/test/input_cases/general_data/pvsmoothing_Fuel_Cell_Phoenix_Validation_alloptions.json", SSCDIR);
    std::ifstream file(file_path);
    std::ostringstream tmp;
    tmp << file.rdbuf();
    file.close();
    ssc_data_t dat = json_to_ssc_data(tmp.str().c_str());
    tmp.str("");

    // setup path to weather file
    nfc1 = sprintf(file_path, "%s/test/input_cases/general_data/phoenix_az_33.450495_-111.983688_psmv3_60_tmy_1mInterpolated.csv", SSCDIR);
    ssc_data_set_string(dat, "solar_resource_file", file_path);

    // Run with fixed output
    int errors = run_module(dat, "pvwattsv7");
    errors = run_module(dat, "fuelcell");
    errors = run_module(dat, "battery");
    // minimize memory usage for Travis
//    errors = run_module(dat, "grid");
//    errors = run_module(dat, "utilityrate5");
//    errors = run_module(dat, "thermalrate");
//    errors = run_module(dat, "singleowner");

    EXPECT_FALSE(errors);
    if (!errors)
    {
        ssc_number_t violation_count, violation_percent, grid_percent, grid_percent_sam;
        ssc_data_get_number(dat, "batt_pvs_violation_count", &violation_count);
        ssc_data_get_number(dat, "batt_pvs_violation_percent", &violation_percent);
        ssc_data_get_number(dat, "batt_pvs_energy_to_grid_percent", &grid_percent);
        ssc_data_get_number(dat, "batt_pvs_energy_to_grid_percent_sam", &grid_percent_sam);

        // values from Phoenix_FuelCell_Validation_testing.sam
        EXPECT_NEAR(violation_count, 4, 0.001);
        EXPECT_NEAR(violation_percent, 0.0076, 0.001);
        EXPECT_NEAR(grid_percent, 99.63, 0.01);
        EXPECT_NEAR(grid_percent_sam, 99.96, 0.01);
    }
    ssc_data_free(dat);
    dat = nullptr;
}
*/
