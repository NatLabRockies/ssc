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

#include "cmod_host_developer_test.h"


TEST_F(CmodHostDeveloperTest, ssc_1047) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/host_developer/Default_HD_10_IBI_PVWatts_Host_Developer_cmod_host_developer.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/host_developer/Default_HD_10_IBI_PVWatts_Host_Developer_cmod_host_developer_outputs.json";
    std::vector<std::string> compare_number_variables = { "project_return_aftertax_npv", "npv", "lnte_nom", "pre_depr_alloc_basis", "pre_itc_qual_basis"};
    std::vector<std::string> compare_array_variables = { "CF_ppa_price" };

    Test("host_developer", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodHostDeveloperTest, sam_1477) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/host_developer/Default_HD_10_IBI_no_tax_reduce_basis_PVWatts_Host_Developer_cmod_host_developer.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/host_developer/Default_HD_10_IBI_no_tax_reduce_basis_PVWatts_Host_Developer_cmod_host_developer_outputs.json";
    std::vector<std::string> compare_number_variables = { "project_return_aftertax_npv", "npv", "lnte_nom", "pre_depr_alloc_basis", "pre_itc_qual_basis"};
    std::vector<std::string> compare_array_variables = { "CF_ppa_price" };

    Test("host_developer", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodHostDeveloperTest, sam_1477_no_tax_no_basis) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/host_developer/Default_HD_10_IBI_no_tax_no_basis_change_PVWatts_Host_Developer_cmod_host_developer.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/host_developer/Default_HD_10_IBI_no_tax_no_basis_change_PVWatts_Host_Developer_cmod_host_developer_outputs.json";
    std::vector<std::string> compare_number_variables = { "project_return_aftertax_npv", "npv", "lnte_nom", "pre_depr_alloc_basis", "pre_itc_qual_basis" };
    std::vector<std::string> compare_array_variables = { "CF_ppa_price" };

    Test("host_developer", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodHostDeveloperTest, sam_1477_utility_ibi) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/host_developer/Default_HD_10_utility_ibi_PVWatts_Host_Developer_cmod_host_developer.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/host_developer/Default_HD_10_utility_ibi_PVWatts_Host_Developer_cmod_host_developer_outputs.json";
    std::vector<std::string> compare_number_variables = { "project_return_aftertax_npv", "npv", "lnte_nom", "pre_depr_alloc_basis", "pre_itc_qual_basis" };
    std::vector<std::string> compare_array_variables = { "CF_ppa_price" };

    Test("host_developer", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodHostDeveloperTest, sam_1477_cbi) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/host_developer/Default_HD_10_CBI_PVWatts_Host_Developer_cmod_host_developer.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/host_developer/Default_HD_10_CBI_PVWatts_Host_Developer_cmod_host_developer_outputs.json";
    std::vector<std::string> compare_number_variables = { "project_return_aftertax_npv", "npv", "lnte_nom", "pre_depr_alloc_basis", "pre_itc_qual_basis" };
    std::vector<std::string> compare_array_variables = { "CF_ppa_price" };

    Test("host_developer", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodHostDeveloperTest, sam_1477_cbi_no_tax_reduce_basis) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/host_developer/Default_HD_10_CBI_no_tax_reduce_basis_PVWatts_Host_Developer_cmod_host_developer.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/host_developer/Default_HD_10_CBI_no_tax_reduce_basis_PVWatts_Host_Developer_cmod_host_developer_outputs.json";
    std::vector<std::string> compare_number_variables = { "project_return_aftertax_npv", "npv", "lnte_nom", "pre_depr_alloc_basis", "pre_itc_qual_basis" };
    std::vector<std::string> compare_array_variables = { "CF_ppa_price" };

    Test("host_developer", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodHostDeveloperTest, PVWatts) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/host_developer/2023.10.27_om-expense-cash-flow_PVWatts_Host_Developer_cmod_host_developer.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/host_developer/2023.10.27_om-expense-cash-flow_PVWatts_Host_Developer_cmod_host_developer_outputs.json";
    std::vector<std::string> compare_number_variables = { "project_return_aftertax_npv", "npv", "lnte_nom" };
    std::vector<std::string> compare_array_variables = { "cf_project_return_aftertax", "cf_project_return_aftertax_npv" };

    Test("host_developer", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodHostDeveloperTest, PVWattsBattery) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/host_developer/2023.10.27_om-expense-cash-flow_PVWatts_Battery_Host_Developer_cmod_host_developer.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/host_developer/2023.10.27_om-expense-cash-flow_PVWatts_Battery_Host_Developer_cmod_host_developer_outputs.json";
    std::vector<std::string> compare_number_variables = { "project_return_aftertax_npv", "npv", "lnte_nom" };
    std::vector<std::string> compare_array_variables = { "cf_project_return_aftertax", "cf_project_return_aftertax_npv" };

    Test("host_developer", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodHostDeveloperTest, PV) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/host_developer/2023.10.27_om-expense-cash-flow_Flat_Plate_PV_Host_Developer_cmod_host_developer.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/host_developer/2023.10.27_om-expense-cash-flow_Flat_Plate_PV_Host_Developer_cmod_host_developer_outputs.json";
    std::vector<std::string> compare_number_variables = { "project_return_aftertax_npv", "npv", "lnte_nom" };
    std::vector<std::string> compare_array_variables = { "cf_project_return_aftertax", "cf_project_return_aftertax_npv" };

    Test("host_developer", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}


TEST_F(CmodHostDeveloperTest, PVBattery) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/host_developer/2023.10.27_om-expense-cash-flow_PV_Battery_Host_Developer_cmod_host_developer.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/host_developer/2023.10.27_om-expense-cash-flow_PV_Battery_Host_Developer_cmod_host_developer_outputs.json";
    std::vector<std::string> compare_number_variables = { "project_return_aftertax_npv", "npv", "lnte_nom" };
    std::vector<std::string> compare_array_variables = { "cf_project_return_aftertax", "cf_project_return_aftertax_npv" };

    Test("host_developer", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}


TEST_F(CmodHostDeveloperTest, CustomGeneration) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/host_developer/2023.10.27_om-expense-cash-flow_Generic_System_Host_Developer_cmod_host_developer.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/host_developer/2023.10.27_om-expense-cash-flow_Generic_System_Host_Developer_cmod_host_developer_outputs.json";
    std::vector<std::string> compare_number_variables = { "project_return_aftertax_npv", "npv", "lnte_nom" };
    std::vector<std::string> compare_array_variables = { "cf_project_return_aftertax", "cf_project_return_aftertax_npv" };

    Test("host_developer", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}


TEST_F(CmodHostDeveloperTest, CustomGenerationBattery) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/host_developer/2023.10.27_om-expense-cash-flow_Generic_Battery_Host_Developer_cmod_host_developer.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/host_developer/2023.10.27_om-expense-cash-flow_Generic_Battery_Host_Developer_cmod_host_developer_outputs.json";
    std::vector<std::string> compare_number_variables = { "project_return_aftertax_npv", "npv", "lnte_nom" };
    std::vector<std::string> compare_array_variables = { "cf_project_return_aftertax", "cf_project_return_aftertax_npv" };

    Test("host_developer", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}



TEST_F(CmodHostDeveloperTest, StandaloneBattery) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/host_developer/2023.10.27_om-expense-cash-flow_Standalone_Battery_Host_Developer_cmod_host_developer.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/host_developer/2023.10.27_om-expense-cash-flow_Standalone_Battery_Host_Developer_cmod_host_developer_outputs.json";
    std::vector<std::string> compare_number_variables = { "project_return_aftertax_npv", "npv", "lnte_nom" };
    std::vector<std::string> compare_array_variables = { "cf_project_return_aftertax", "cf_project_return_aftertax_npv" };

    Test("host_developer", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

