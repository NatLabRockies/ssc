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


#include "cmod_singleowner_test.h"

#include "gtest/gtest.h"

TEST_F(CMSingleOwner, ResidentialDefault_cmod_swh) {

    int errors = run_module(data, "singleowner");
    ASSERT_EQ(errors, 0);

    ssc_number_t npv;
    ssc_data_get_number(data, "project_return_aftertax_npv", &npv);
    EXPECT_NEAR(npv, -647715977.8, 0.1);

}


TEST_F(CmodSingleOwnerTest, ssc_1047) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/singleowner/Default_SO_10_IBI_PVWatts_Single_Owner_cmod_singleowner.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/singleowner/Default_SO_10_IBI_PVWatts_Single_Owner_cmod_singleowner_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "project_return_aftertax_npv", "lcoe_real", "lppa_nom" };
    std::vector<std::string> compare_array_variables = { "cf_project_return_aftertax", "cf_annual_costs" };

    Test("singleowner", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodSingleOwnerTest, ETES) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/singleowner/2022.08.08_develop_branch_ETES_Single_Owner_cmod_singleowner.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/singleowner/2022.08.08_develop_branch_ETES_Single_Owner_cmod_singleowner_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "project_return_aftertax_npv", "lcoe_real", "lppa_nom" };
    std::vector<std::string> compare_array_variables = { "cf_project_return_aftertax", "cf_annual_costs" };

    Test("singleowner", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodSingleOwnerTest, PV) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/singleowner/2022.08.08_develop_branch_Flat_Plate_PV_Single_Owner_cmod_singleowner.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/singleowner/2022.08.08_develop_branch_Flat_Plate_PV_Single_Owner_cmod_singleowner_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "project_return_aftertax_npv", "lcoe_real", "lppa_nom" };
    std::vector<std::string> compare_array_variables = { "cf_project_return_aftertax", "cf_annual_costs" };

    Test("singleowner", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodSingleOwnerTest, FuelCell) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/singleowner/2022.08.08_develop_branch_Fuel_Cell_Single_Owner_cmod_singleowner.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/singleowner/2022.08.08_develop_branch_Fuel_Cell_Single_Owner_cmod_singleowner_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "project_return_aftertax_npv", "lcoe_real", "lppa_nom" };
    std::vector<std::string> compare_array_variables = { "cf_project_return_aftertax", "cf_annual_costs" };

    Test("singleowner", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}
TEST_F(CmodSingleOwnerTest, GenericCSP) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/singleowner/2022.08.08_develop_branch_Generic_CSP_System_Single_Owner_cmod_singleowner.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/singleowner/2022.08.08_develop_branch_Generic_CSP_System_Single_Owner_cmod_singleowner_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "project_return_aftertax_npv", "lcoe_real", "lppa_nom" };
    std::vector<std::string> compare_array_variables = { "cf_project_return_aftertax", "cf_annual_costs" };

    Test("singleowner", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodSingleOwnerTest, Geotherrmal) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/singleowner/2022.08.08_develop_branch_Geothermal_Power_Single_Owner_cmod_singleowner.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/singleowner/2022.08.08_develop_branch_Geothermal_Power_Single_Owner_cmod_singleowner_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "project_return_aftertax_npv", "lcoe_real", "lppa_nom" };
    std::vector<std::string> compare_array_variables = { "cf_project_return_aftertax", "cf_annual_costs" };

    Test("singleowner", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}
TEST_F(CmodSingleOwnerTest, PVBattery) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/singleowner/2022.08.08_develop_branch_PV_Battery_Single_Owner_cmod_singleowner.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/singleowner/2022.08.08_develop_branch_PV_Battery_Single_Owner_cmod_singleowner_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "project_return_aftertax_npv", "lcoe_real", "lppa_nom" };
    std::vector<std::string> compare_array_variables = { "cf_project_return_aftertax", "cf_annual_costs" };

    Test("singleowner", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}


TEST_F(CmodSingleOwnerTest, PVWatts) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/singleowner/2022.08.08_develop_branch_PVWatts_Single_Owner_cmod_singleowner.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/singleowner/2022.08.08_develop_branch_PVWatts_Single_Owner_cmod_singleowner_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "project_return_aftertax_npv", "lcoe_real", "lppa_nom" };
    std::vector<std::string> compare_array_variables = { "cf_project_return_aftertax", "cf_annual_costs" };

    Test("singleowner", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}
