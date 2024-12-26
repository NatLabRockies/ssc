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

#include "cmod_thirdpartyownership_test.h"


TEST_F(CmodThirdPartyOwnershipTest, PVWatts) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/thirdpartyownership/2022.08.08_develop_branch_PVWatts_Third_Party_cmod_thirdpartyownership.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/thirdpartyownership/2022.08.08_develop_branch_PVWatts_Third_Party_cmod_thirdpartyownership_outputs.json";
    std::vector<std::string> compare_number_variables = { "npv", "lnte_nom" };
    std::vector<std::string> compare_array_variables = { "cf_after_tax_cash_flow", "cf_agreement_cost" };

    Test("thirdpartyownership", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodThirdPartyOwnershipTest, PVWattsBattery) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/thirdpartyownership/2022.08.08_develop_branch_PVWatts_Battery_Third_Party_cmod_thirdpartyownership.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/thirdpartyownership/2022.08.08_develop_branch_PVWatts_Battery_Third_Party_cmod_thirdpartyownership_outputs.json";
    std::vector<std::string> compare_number_variables = { "npv", "lnte_nom" };
    std::vector<std::string> compare_array_variables = { "cf_after_tax_cash_flow", "cf_agreement_cost" };

    Test("thirdpartyownership", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodThirdPartyOwnershipTest, PV) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/thirdpartyownership/2022.08.08_develop_branch_Flat_Plate_PV_Third_Party_cmod_thirdpartyownership.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/thirdpartyownership/2022.08.08_develop_branch_Flat_Plate_PV_Third_Party_cmod_thirdpartyownership_outputs.json";
    std::vector<std::string> compare_number_variables = { "npv", "lnte_nom" };
    std::vector<std::string> compare_array_variables = { "cf_after_tax_cash_flow", "cf_agreement_cost" };

    Test("thirdpartyownership", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}


TEST_F(CmodThirdPartyOwnershipTest, PVBattery) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/thirdpartyownership/2022.08.08_develop_branch_PV_Battery_Third_Party_cmod_thirdpartyownership.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/thirdpartyownership/2022.08.08_develop_branch_PV_Battery_Third_Party_cmod_thirdpartyownership_outputs.json";
    std::vector<std::string> compare_number_variables = { "npv", "lnte_nom" };
    std::vector<std::string> compare_array_variables = { "cf_after_tax_cash_flow", "cf_agreement_cost" };

    Test("thirdpartyownership", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}


TEST_F(CmodThirdPartyOwnershipTest, CustomGeneration) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/thirdpartyownership/2022.08.08_develop_branch_Generic_System_Third_Party_cmod_thirdpartyownership.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/thirdpartyownership/2022.08.08_develop_branch_Generic_System_Third_Party_cmod_thirdpartyownership_outputs.json";
    std::vector<std::string> compare_number_variables = { "npv", "lnte_nom" };
    std::vector<std::string> compare_array_variables = { "cf_after_tax_cash_flow", "cf_agreement_cost" };

    Test("thirdpartyownership", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}


TEST_F(CmodThirdPartyOwnershipTest, CustomGenerationBattery) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/thirdpartyownership/2022.08.08_develop_branch_Generic_Battery_Third_Party_cmod_thirdpartyownership.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/thirdpartyownership/2022.08.08_develop_branch_Generic_Battery_Third_Party_cmod_thirdpartyownership_outputs.json";
    std::vector<std::string> compare_number_variables = { "npv", "lnte_nom" };
    std::vector<std::string> compare_array_variables = { "cf_after_tax_cash_flow", "cf_agreement_cost" };

    Test("thirdpartyownership", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}



TEST_F(CmodThirdPartyOwnershipTest, StandaloneBattery) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/thirdpartyownership/2022.08.08_develop_branch_Standalone_Battery_Third_Party_cmod_thirdpartyownership.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/thirdpartyownership/2022.08.08_develop_branch_Standalone_Battery_Third_Party_cmod_thirdpartyownership_outputs.json";
    std::vector<std::string> compare_number_variables = { "npv", "lnte_nom" };
    std::vector<std::string> compare_array_variables = { "cf_after_tax_cash_flow", "cf_agreement_cost" };

    Test("thirdpartyownership", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

