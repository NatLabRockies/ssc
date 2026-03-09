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


#include "cmod_hybrid_steps_test.h"

#include "gtest/gtest.h"

// TODO - update input JSON for test paths for resource files

TEST_F(CmodHybridStepsTest, PVWattsv8) {

    char file_path[256];
    int nfc1 = sprintf(file_path, "%s/test/input_json/hybrids/pvwattsv8.json", SSCDIR);
    std::ifstream file(file_path);
    std::ostringstream tmp;
    tmp << file.rdbuf();
    file.close();
    ssc_data_t dat = json_to_ssc_data(tmp.str().c_str());
    tmp.str("");

    auto table = ssc_data_get_table(dat, "input");
    auto pv_table = ssc_data_get_table(table, "pvwattsv8");
    char solar_resource_path[256];
    sprintf(solar_resource_path, "%s/test/input_cases/general_data/phoenix_az_33.450495_-111.983688_psmv3_60_tmy.csv", std::getenv("SSCDIR"));
    ssc_data_set_string(pv_table, "solar_resource_file", solar_resource_path);

    int errors = run_module(dat, "hybrid_steps");
    EXPECT_FALSE(errors);
    if (!errors)
    {
        ssc_number_t annualenergy;
        auto outputs = ssc_data_get_table(dat, "output");
        auto pv_outputs = ssc_data_get_table(outputs, "pvwattsv8");
        ssc_data_get_number(pv_outputs, "annual_energy", &annualenergy);
        EXPECT_NEAR(annualenergy, 165112880, 165112880 * 0.01);
    }
    ssc_data_free(dat);
    dat = nullptr;
}

TEST_F(CmodHybridStepsTest, Wind) {

    char file_path[256];
    int nfc1 = sprintf(file_path, "%s/test/input_json/hybrids/wind.json", SSCDIR);
    std::ifstream file(file_path);
    std::ostringstream tmp;
    tmp << file.rdbuf();
    file.close();
    ssc_data_t dat = json_to_ssc_data(tmp.str().c_str());
    tmp.str("");

    auto table = ssc_data_get_table(dat, "input");
    auto wind_table = ssc_data_get_table(table, "windpower");
    char wind_resource_path[256];
    sprintf(wind_resource_path, "%s/test/input_cases/general_data/WY_Southern-Flat_Lands.srw", std::getenv("SSCDIR"));
    ssc_data_set_string(wind_table, "wind_resource_filename", wind_resource_path);

    int errors = run_module(dat, "hybrid_steps");
    EXPECT_FALSE(errors);
    if (!errors)
    {
        ssc_number_t annualenergy;
        auto outputs = ssc_data_get_table(dat, "output");
        auto wind_outputs = ssc_data_get_table(outputs, "windpower");
        ssc_data_get_number(wind_outputs, "annual_energy", &annualenergy);

        EXPECT_NEAR(annualenergy, 201595968, 201595968 * 0.01);
    }
    ssc_data_free(dat);
    dat = nullptr;
}


TEST_F(CmodHybridStepsTest, PVWattsv8Wind) {

    char file_path[256];
    int nfc1 = sprintf(file_path, "%s/test/input_json/hybrids/pvwattsv8wind.json", SSCDIR);
    std::ifstream file(file_path);
    std::ostringstream tmp;
    tmp << file.rdbuf();
    file.close();
    ssc_data_t dat = json_to_ssc_data(tmp.str().c_str());
    tmp.str("");

    auto table = ssc_data_get_table(dat, "input");
    auto pv_table = ssc_data_get_table(table, "pvwattsv8");
    char solar_resource_path[256];
    sprintf(solar_resource_path, "%s/test/input_cases/general_data/phoenix_az_33.450495_-111.983688_psmv3_60_tmy.csv", std::getenv("SSCDIR"));
    ssc_data_set_string(pv_table, "solar_resource_file", solar_resource_path);

    auto wind_table = ssc_data_get_table(table, "windpower");
    char wind_resource_path[256];
    sprintf(wind_resource_path, "%s/test/input_cases/general_data/WY_Southern-Flat_Lands.srw", std::getenv("SSCDIR"));
    ssc_data_set_string(wind_table, "wind_resource_filename", wind_resource_path);

    int errors = run_module(dat, "hybrid_steps");

    EXPECT_FALSE(errors);
    if (!errors)
    {
        ssc_number_t annualenergy;
        auto outputs = ssc_data_get_table(dat, "output");
        ssc_data_get_number(outputs, "cumulative_annual_energy", &annualenergy);
        EXPECT_NEAR(annualenergy, 366708848, 366708848 * 0.01);

        ssc_data_get_number(outputs, "annual_energy", &annualenergy);
        EXPECT_NEAR(annualenergy, 366708848, 366708848 * 0.01);

        auto pv_outputs = ssc_data_get_table(outputs, "pvwattsv8");
        ssc_data_get_number(pv_outputs, "annual_energy", &annualenergy);
        EXPECT_NEAR(annualenergy, 165112880, 165112880 * 0.01);

        auto wind_outputs = ssc_data_get_table(outputs, "windpower");
        ssc_data_get_number(wind_outputs, "annual_energy", &annualenergy);
        EXPECT_NEAR(annualenergy, 201595968, 201595968 * 0.01);
    }
    ssc_data_free(dat);
    dat = nullptr;
}
