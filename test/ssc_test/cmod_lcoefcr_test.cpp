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

#include "cmod_lcoefcr_test.h"


TEST_F(CmodLCOEFCRTest, Biopower) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/lcoefcr/2022.08.08_develop_branch_Biopower_LCOE_Calculator_cmod_lcoefcr.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/lcoefcr/2022.08.08_develop_branch_Biopower_LCOE_Calculator_cmod_lcoefcr_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_fcr" };
    std::vector<std::string> compare_array_variables;
    
    Test("lcoefcr", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}
/*
TEST_F(CmodLCOEFCRTest, DSLF) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/lcoefcr/2022.08.08_develop_branch_DSLF_LCOE_Calculator_cmod_lcoefcr.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/lcoefcr/2022.08.08_develop_branch_DSLF_LCOE_Calculator_cmod_lcoefcr_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_fcr" };
    std::vector<std::string> compare_array_variables;

    Test("lcoefcr", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodLCOEFCRTest, EmpiricalTrough) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/lcoefcr/2022.08.08_develop_branch_Empirical_Trough_LCOE_Calculator_cmod_lcoefcr.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/lcoefcr/2022.08.08_develop_branch_Empirical_Trough_LCOE_Calculator_cmod_lcoefcr_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_fcr" };
    std::vector<std::string> compare_array_variables;

    Test("lcoefcr", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}
*/
TEST_F(CmodLCOEFCRTest, PV) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/lcoefcr/2022.08.08_develop_branch_Flat_Plate_PV_LCOE_Calculator_cmod_lcoefcr.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/lcoefcr/2022.08.08_develop_branch_Flat_Plate_PV_LCOE_Calculator_cmod_lcoefcr_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_fcr" };
    std::vector<std::string> compare_array_variables;

    Test("lcoefcr", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}


TEST_F(CmodLCOEFCRTest, GenericCSP) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/lcoefcr/2022.08.08_develop_branch_Generic_CSP_System_LCOE_Calculator_cmod_lcoefcr.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/lcoefcr/2022.08.08_develop_branch_Generic_CSP_System_LCOE_Calculator_cmod_lcoefcr_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_fcr" };
    std::vector<std::string> compare_array_variables;

    Test("lcoefcr", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodLCOEFCRTest, CustomGeneration) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/lcoefcr/2022.08.08_develop_branch_Generic_System_LCOE_Calculator_cmod_lcoefcr.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/lcoefcr/2022.08.08_develop_branch_Generic_System_LCOE_Calculator_cmod_lcoefcr_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_fcr" };
    std::vector<std::string> compare_array_variables;

    Test("lcoefcr", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}


TEST_F(CmodLCOEFCRTest, Geotherrmal) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/lcoefcr/2022.08.08_develop_branch_Geothermal_Power_LCOE_Calculator_cmod_lcoefcr.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/lcoefcr/2022.08.08_develop_branch_Geothermal_Power_LCOE_Calculator_cmod_lcoefcr_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_fcr" };
    std::vector<std::string> compare_array_variables;

    Test("lcoefcr", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}
/*
TEST_F(CmodLCOEFCRTest, CPV) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/lcoefcr/2022.08.08_develop_branch_High-X_Concentrating_PV_LCOE_Calculator_cmod_lcoefcr.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/lcoefcr/2022.08.08_develop_branch_High-X_Concentrating_PV_LCOE_Calculator_cmod_lcoefcr_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_fcr" };
    std::vector<std::string> compare_array_variables;


    Test("lcoefcr", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodLCOEFCRTest, MEtidal) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/lcoefcr/2022.08.08_develop_branch_MEtidal_LCOE_Calculator_cmod_lcoefcr.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/lcoefcr/2022.08.08_develop_branch_MEtidal_LCOE_Calculator_cmod_lcoefcr_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_fcr" };
    std::vector<std::string> compare_array_variables;

    Test("lcoefcr", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}


TEST_F(CmodLCOEFCRTest, MEwave) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/lcoefcr/2022.08.08_develop_branch_MEwave_LCOE_Calculator_cmod_lcoefcr.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/lcoefcr/2022.08.08_develop_branch_MEwave_LCOE_Calculator_cmod_lcoefcr_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_fcr" };
    std::vector<std::string> compare_array_variables;

    Test("lcoefcr", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodLCOEFCRTest, MSLF) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/lcoefcr/2022.08.08_develop_branch_MSLF_LCOE_Calculator_cmod_lcoefcr.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/lcoefcr/2022.08.08_develop_branch_MSLF_LCOE_Calculator_cmod_lcoefcr_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_fcr" };
    std::vector<std::string> compare_array_variables;

    Test("lcoefcr", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodLCOEFCRTest, PhysicalTrough) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/lcoefcr/2022.08.08_develop_branch_Physical_Trough_LCOE_Calculator_cmod_lcoefcr.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/lcoefcr/2022.08.08_develop_branch_Physical_Trough_LCOE_Calculator_cmod_lcoefcr_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_fcr" };
    std::vector<std::string> compare_array_variables;

    Test("lcoefcr", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodLCOEFCRTest, PVWatts) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/lcoefcr/2022.08.08_develop_branch_PVWatts_LCOE_Calculator_cmod_lcoefcr.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/lcoefcr/2022.08.08_develop_branch_PVWatts_LCOE_Calculator_cmod_lcoefcr_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_fcr" };
    std::vector<std::string> compare_array_variables;

    Test("lcoefcr", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodLCOEFCRTest, SWH) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/lcoefcr/2022.08.08_develop_branch_Solar_Water_Heating_LCOE_Calculator_cmod_lcoefcr.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/lcoefcr/2022.08.08_develop_branch_Solar_Water_Heating_LCOE_Calculator_cmod_lcoefcr_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_fcr" };
    std::vector<std::string> compare_array_variables;

    Test("lcoefcr", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodLCOEFCRTest, Wind) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/lcoefcr/2022.08.08_develop_branch_Wind_Power_LCOE_Calculator_cmod_lcoefcr.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/lcoefcr/2022.08.08_develop_branch_Wind_Power_LCOE_Calculator_cmod_lcoefcr_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_fcr" };
    std::vector<std::string> compare_array_variables;

    Test("lcoefcr", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}


TEST_F(CmodLCOEFCRTest, DSGL_IPH_LCOH) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/lcoefcr/2022.08.08_develop_branch_DSGL_IPH_LCOH_Calculator_cmod_lcoefcr.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/lcoefcr/2022.08.08_develop_branch_DSGL_IPH_LCOH_Calculator_cmod_lcoefcr_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_fcr" };
    std::vector<std::string> compare_array_variables;

    Test("lcoefcr", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

*/
TEST_F(CmodLCOEFCRTest, PhysicalTrough_IPH_LCOH) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/lcoefcr/2022.08.08_develop_branch_Physical_Trough_IPH_LCOH_Calculator_cmod_lcoefcr.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/lcoefcr/2022.08.08_develop_branch_Physical_Trough_IPH_LCOH_Calculator_cmod_lcoefcr_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_fcr" };
    std::vector<std::string> compare_array_variables;

    Test("lcoefcr", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}
