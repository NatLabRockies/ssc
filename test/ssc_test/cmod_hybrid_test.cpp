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


#include "cmod_hybrid_test.h"

#include "gtest/gtest.h"




TEST_F(CmodHybridTest, PVWattsv8WindBatterySingleOwner) {

    char file_path[256];
    int nfc1 = sprintf(file_path, "%s/test/input_json/hybrids/PVWatts Wind Battery Hybrid_Single Owner.json", SSCDIR);
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
    sprintf(wind_resource_path, "%s/test/input_cases/general_data/AZ Eastern-Rolling Hills.srw", std::getenv("SSCDIR"));
    ssc_data_set_string(wind_table, "wind_resource_filename", wind_resource_path);

    int errors = run_module(dat, "hybrid");

    EXPECT_FALSE(errors);
    if (!errors)
    {
        int len;

        ssc_number_t pvannualenergy, windannualenergy, battannualenergy, npv, total_energy;
        ssc_number_t* battchargeenergy;
        ssc_number_t* battdischargeenergy;
        auto outputs = ssc_data_get_table(dat, "output");
        auto inputs = ssc_data_get_table(dat, "input");

        auto pv_outputs = ssc_data_get_table(outputs, "pvwattsv8");
        auto pv_inputs = ssc_data_get_table(inputs, "pvwattsv8");
        ssc_data_get_number(pv_outputs, "annual_energy", &pvannualenergy);
        EXPECT_NEAR(pvannualenergy, 211907456, 211907456 * 0.01);

        auto wind_outputs = ssc_data_get_table(outputs, "windpower");
        auto wind_inputs = ssc_data_get_table(inputs, "windpower");
        ssc_data_get_number(wind_outputs, "annual_energy", &windannualenergy);
        EXPECT_NEAR(windannualenergy, 366975552, 366975552 * 0.01);

        auto batt_outputs = ssc_data_get_table(outputs, "battery");
        auto batt_inputs = ssc_data_get_table(inputs, "battery");
        ssc_data_get_number(batt_outputs, "annual_energy", &battannualenergy);
        battchargeenergy = ssc_data_get_array(batt_outputs, "batt_annual_charge_energy", &len);
        battdischargeenergy = ssc_data_get_array(batt_outputs, "batt_annual_discharge_energy", &len);
        EXPECT_NEAR(battannualenergy, 570565000, 570565000 * 0.01);
        EXPECT_NEAR(battchargeenergy[1], 81116970, 81116970 * 0.001);
        EXPECT_NEAR(battdischargeenergy[1], 72917429, 72917429 * 0.001);

        auto hybrid_outputs = ssc_data_get_table(outputs, "Hybrid");
        ssc_data_get_number(hybrid_outputs, "annual_energy", &total_energy);

        auto ebitda = ssc_data_get_array(hybrid_outputs, "cf_ebitda", &len);
        auto revenue = ssc_data_get_array(hybrid_outputs, "cf_total_revenue", &len);
        auto om_expenses = ssc_data_get_array(hybrid_outputs, "cf_operating_expenses", &len);
        ssc_data_get_number(hybrid_outputs, "project_return_aftertax_npv", &npv);

        EXPECT_NEAR(om_expenses[1], 10425847, 1);
        EXPECT_NEAR(revenue[1], 33062517, 1);
        EXPECT_NEAR(ebitda[1], 22636670, 1);
        EXPECT_NEAR(npv, -227222606, 227222606 * 0.001);

        EXPECT_NEAR(total_energy, battannualenergy, total_energy * 0.001);
        EXPECT_NEAR(total_energy, pvannualenergy + windannualenergy - battchargeenergy[1] + battdischargeenergy[1], total_energy * 0.001);
    }
    ssc_data_free(dat);
    dat = nullptr;
}




TEST_F(CmodHybridTest, PVWattsv8WindBatteryHostDeveloper) {

    char file_path[256];
    int nfc1 = sprintf(file_path, "%s/test/input_json/hybrids/PVWatts Wind Battery Hybrid_Host Developer.json", SSCDIR);
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
    sprintf(wind_resource_path, "%s/test/input_cases/general_data/AZ Eastern-Rolling Hills.srw", std::getenv("SSCDIR"));
    ssc_data_set_string(wind_table, "wind_resource_filename", wind_resource_path);

    int errors = run_module(dat, "hybrid");

    EXPECT_FALSE(errors);
    if (!errors)
    {
        int len;
        ssc_number_t pvannualenergy, windannualenergy, battannualenergy, npv, total_energy;
        ssc_number_t* battchargeenergy;
        ssc_number_t* battdischargeenergy;
        auto outputs = ssc_data_get_table(dat, "output");

        auto pv_outputs = ssc_data_get_table(outputs, "pvwattsv8");
        ssc_data_get_number(pv_outputs, "annual_energy", &pvannualenergy);
        EXPECT_NEAR(pvannualenergy, 938557, 938557 * 0.01);

        auto wind_outputs = ssc_data_get_table(outputs, "windpower");
        ssc_data_get_number(wind_outputs, "annual_energy", &windannualenergy);
        EXPECT_NEAR(windannualenergy, 187767, 187767 * 0.01);

        //, 
        auto battery_outputs = ssc_data_get_table(outputs, "battery");
        ssc_data_get_number(battery_outputs, "annual_energy", &battannualenergy);
        battchargeenergy = ssc_data_get_array(battery_outputs, "batt_annual_charge_energy", &len);
        battdischargeenergy = ssc_data_get_array(battery_outputs, "batt_annual_discharge_energy", &len);
        EXPECT_NEAR(battannualenergy, 1118877, 1118877 * 0.01);
        EXPECT_NEAR(battchargeenergy[1], 83565, 83565 * 0.001);
        EXPECT_NEAR(battdischargeenergy[1], 76334, 76334 * 0.001);

        auto hybrid_outputs = ssc_data_get_table(outputs, "Hybrid");
        ssc_data_get_number(hybrid_outputs, "project_return_aftertax_npv", &npv);
        EXPECT_NEAR(npv, -168769, 168769 * 0.001);

        ssc_data_get_number(hybrid_outputs, "annual_energy", &total_energy);

        EXPECT_NEAR(total_energy, battannualenergy, total_energy * 0.001);
        EXPECT_NEAR(total_energy, pvannualenergy + windannualenergy - battchargeenergy[1] + battdischargeenergy[1], total_energy * 0.001);
    }
    ssc_data_free(dat);
    dat = nullptr;
}

TEST_F(CmodHybridTest, CustomGenerationPVWattsWindFuelCellBatteryHybrid_SingleOwner) {
    char file_path[256];
    int nfc1 = sprintf(file_path, "%s/test/input_json/hybrids/CustomGeneration PVWatts Wind FuelCell Battery Hybrid_Single Owner.json", SSCDIR);
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
    sprintf(wind_resource_path, "%s/test/input_cases/general_data/AZ Eastern-Rolling Hills.srw", std::getenv("SSCDIR"));
    ssc_data_set_string(wind_table, "wind_resource_filename", wind_resource_path);

    int errors = run_module(dat, "hybrid");

    EXPECT_FALSE(errors);
    if (!errors)
    {
        ssc_number_t genericannualenergy, pvannualenergy, windannualenergy, battannualenergy, fuelcellannualenergy, npv, total_energy;
        ssc_number_t* battchargeenergy;
        ssc_number_t* battdischargeenergy;

        int len;
        auto outputs = ssc_data_get_table(dat, "output");

        auto gs_outputs = ssc_data_get_table(outputs, "custom_generation");
        ssc_data_get_number(gs_outputs, "annual_energy", &genericannualenergy);
        auto gs_om_expenses = ssc_data_get_array(gs_outputs, "cf_operating_expenses", &len);

        auto pv_outputs = ssc_data_get_table(outputs, "pvwattsv8");
        ssc_data_get_number(pv_outputs, "annual_energy", &pvannualenergy);
        auto pv_om_expenses = ssc_data_get_array(pv_outputs, "cf_operating_expenses", &len);

        auto wind_outputs = ssc_data_get_table(outputs, "windpower");
        ssc_data_get_number(wind_outputs, "annual_energy", &windannualenergy);
        auto wind_om_expenses = ssc_data_get_array(wind_outputs, "cf_operating_expenses", &len);

        auto fuelcell_outputs = ssc_data_get_table(outputs, "fuelcell");
        ssc_data_get_number(fuelcell_outputs, "annual_energy_discharged", &fuelcellannualenergy);

        auto batt_outputs = ssc_data_get_table(outputs, "battery");
        ssc_data_get_number(batt_outputs, "annual_energy", &battannualenergy);
        auto batt_om_expenses = ssc_data_get_array(batt_outputs, "cf_operating_expenses", &len);
        battchargeenergy = ssc_data_get_array(batt_outputs, "batt_annual_charge_energy", &len);
        battdischargeenergy = ssc_data_get_array(batt_outputs, "batt_annual_discharge_energy", &len);

        auto hybrid_outputs = ssc_data_get_table(outputs, "Hybrid");
        ssc_data_get_number(hybrid_outputs, "project_return_aftertax_npv", &npv);
        auto ebitda = ssc_data_get_array(hybrid_outputs, "cf_ebitda", &len);
        ssc_data_get_number(hybrid_outputs, "annual_energy", &total_energy);

        auto revenue = ssc_data_get_array(hybrid_outputs, "cf_total_revenue", &len);
        auto om_expenses = ssc_data_get_array(hybrid_outputs, "cf_operating_expenses", &len);


        EXPECT_NEAR(genericannualenergy, 756864000., 1e6);
        EXPECT_NEAR(pvannualenergy, 211907456., 1e6);
        EXPECT_NEAR(windannualenergy, 366975552., 1e6);
        EXPECT_NEAR(fuelcellannualenergy, 1561993, 1e6);
        EXPECT_NEAR(battannualenergy, 1331720000., 1e6);
        EXPECT_NEAR(battchargeenergy[1], 55372248, 55372248 * 0.001);
        EXPECT_NEAR(battdischargeenergy[1], 49818321, 49818321 * 0.001);
        EXPECT_NEAR(total_energy, battannualenergy, total_energy * 0.001);
        EXPECT_NEAR(total_energy, pvannualenergy + windannualenergy + genericannualenergy + fuelcellannualenergy - battchargeenergy[1] + battdischargeenergy[1], total_energy * 0.001);
        
        EXPECT_NEAR(om_expenses[1], 90224679., 1e5);
        EXPECT_NEAR(revenue[1], 66590988., 1e5);
        EXPECT_NEAR(ebitda[1], -23633690., 1e5);
        EXPECT_NEAR(npv, -1750593259., 1e6);
    }
    ssc_data_free(dat);
    dat = nullptr;
}


