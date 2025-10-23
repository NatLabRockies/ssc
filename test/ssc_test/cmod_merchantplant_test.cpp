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


#include "cmod_merchantplant_test.h"

#include "gtest/gtest.h"


TEST_F(CmodMerchantPlantTest, Biopower) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/merchantplant/2022.08.08_develop_branch_Biopower_Merchant_Plant_cmod_merchantplant.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/merchantplant/2022.08.08_develop_branch_Biopower_Merchant_Plant_cmod_merchantplant_outputs.json";
    std::vector<std::string> compare_number_variables = { "npv_curtailment_revenue", "npv_capacity_revenue", "npv_energy_market_revenue", "npv_ancillary_services_1_revenue", "npv_ancillary_services_2_revenue", "npv_ancillary_services_3_revenue", "npv_ancillary_services_4_revenue"};
    std::vector<std::string> compare_array_variables = {"mp_energy_market_generated_revenue", "mp_ancillary_services1_generated_revenue", "mp_ancillary_services2_generated_revenue", "mp_ancillary_services3_generated_revenue", "mp_ancillary_services4_generated_revenue"};

    Test("merchantplant", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}
TEST_F(CmodMerchantPlantTest, PV) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/merchantplant/2022.08.08_develop_branch_Flat_Plate_PV_Merchant_Plant_cmod_merchantplant.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/merchantplant/2022.08.08_develop_branch_Flat_Plate_PV_Merchant_Plant_cmod_merchantplant_outputs.json";
    std::vector<std::string> compare_number_variables = { "npv_curtailment_revenue", "npv_capacity_revenue", "npv_energy_market_revenue", "npv_ancillary_services_1_revenue", "npv_ancillary_services_2_revenue", "npv_ancillary_services_3_revenue", "npv_ancillary_services_4_revenue"};
    std::vector<std::string> compare_array_variables = {"mp_energy_market_generated_revenue", "mp_ancillary_services1_generated_revenue", "mp_ancillary_services2_generated_revenue", "mp_ancillary_services3_generated_revenue", "mp_ancillary_services4_generated_revenue"};

    Test("merchantplant", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodMerchantPlantTest, CustomGenerationBattery_LCOS) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/merchantplant/2022.10.21_develop_branch_Generic_Battery_Merchant_Plant_cmod_merchantplant.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/merchantplant/2022.10.21_develop_branch_Generic_Battery_Merchant_Plant_cmod_merchantplant_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcos_real" };
    std::vector<std::string> compare_array_variables = { "mp_energy_market_generated_revenue", "cf_charging_cost_grid" };

    Test("merchantplant", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodMerchantPlantTest, GenericCSP) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/merchantplant/2022.08.08_develop_branch_Generic_CSP_System_Merchant_Plant_cmod_merchantplant.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/merchantplant/2022.08.08_develop_branch_Generic_CSP_System_Merchant_Plant_cmod_merchantplant_outputs.json";
    std::vector<std::string> compare_number_variables = { "npv_curtailment_revenue", "npv_capacity_revenue", "npv_energy_market_revenue", "npv_ancillary_services_1_revenue", "npv_ancillary_services_2_revenue", "npv_ancillary_services_3_revenue", "npv_ancillary_services_4_revenue"};
    std::vector<std::string> compare_array_variables = {"mp_energy_market_generated_revenue", "mp_ancillary_services1_generated_revenue", "mp_ancillary_services2_generated_revenue", "mp_ancillary_services3_generated_revenue", "mp_ancillary_services4_generated_revenue"};

    Test("merchantplant", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodMerchantPlantTest, CustomGeneration) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/merchantplant/2022.08.08_develop_branch_Generic_System_Merchant_Plant_cmod_merchantplant.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/merchantplant/2022.08.08_develop_branch_Generic_System_Merchant_Plant_cmod_merchantplant_outputs.json";
    std::vector<std::string> compare_number_variables = { "npv_curtailment_revenue", "npv_capacity_revenue", "npv_energy_market_revenue", "npv_ancillary_services_1_revenue", "npv_ancillary_services_2_revenue", "npv_ancillary_services_3_revenue", "npv_ancillary_services_4_revenue"};
    std::vector<std::string> compare_array_variables = {"mp_energy_market_generated_revenue", "mp_ancillary_services1_generated_revenue", "mp_ancillary_services2_generated_revenue", "mp_ancillary_services3_generated_revenue", "mp_ancillary_services4_generated_revenue"};

    Test("merchantplant", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}


TEST_F(CmodMerchantPlantTest, Geotherrmal) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/merchantplant/2022.08.08_develop_branch_Geothermal_Power_Merchant_Plant_cmod_merchantplant.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/merchantplant/2022.08.08_develop_branch_Geothermal_Power_Merchant_Plant_cmod_merchantplant_outputs.json";
    std::vector<std::string> compare_number_variables = { "npv_curtailment_revenue", "npv_capacity_revenue", "npv_energy_market_revenue", "npv_ancillary_services_1_revenue", "npv_ancillary_services_2_revenue", "npv_ancillary_services_3_revenue", "npv_ancillary_services_4_revenue"};
    std::vector<std::string> compare_array_variables = {"mp_energy_market_generated_revenue", "mp_ancillary_services1_generated_revenue", "mp_ancillary_services2_generated_revenue", "mp_ancillary_services3_generated_revenue", "mp_ancillary_services4_generated_revenue"};

    Test("merchantplant", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}
TEST_F(CmodMerchantPlantTest, PVBattery) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/merchantplant/2022.08.08_develop_branch_PV_Battery_Merchant_Plant_cmod_merchantplant.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/merchantplant/2022.08.08_develop_branch_PV_Battery_Merchant_Plant_cmod_merchantplant_outputs.json";
    std::vector<std::string> compare_number_variables = { "npv_curtailment_revenue", "npv_capacity_revenue", "npv_energy_market_revenue", "npv_ancillary_services_1_revenue", "npv_ancillary_services_2_revenue", "npv_ancillary_services_3_revenue", "npv_ancillary_services_4_revenue"};
    std::vector<std::string> compare_array_variables = {"mp_energy_market_generated_revenue", "mp_ancillary_services1_generated_revenue", "mp_ancillary_services2_generated_revenue", "mp_ancillary_services3_generated_revenue", "mp_ancillary_services4_generated_revenue"};

    Test("merchantplant", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

