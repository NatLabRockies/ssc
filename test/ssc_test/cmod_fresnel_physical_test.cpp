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

#include "csp_common_test.h"

#include "cmod_fresnel_physical_test.h"


TEST_F(CmodFresnelPhysicalTest, MSLFDefault) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/TechnologyModels/fresnel_physical/2025.03.13_develop_branch_MSLF_Single_Owner_cmod_fresnel_physical.json";
    std::string file_outputs = SSCDIR;
    file_outputs +="/test/input_json/TechnologyModels/fresnel_physical/2025.03.13_develop_branch_MSLF_Single_Owner_cmod_fresnel_physical_outputs.json";
    std::vector<std::string> compare_number_variables = { "annual_energy", "q_dot_loss_tes_des" };
    std::vector<std::string> compare_array_variables = { "monthly_energy" };

    Test("fresnel_physical", file_inputs, file_outputs, compare_number_variables, compare_array_variables, 0.001, "tucson_az_32.116521_-110.933042_psmv3_60_tmy.csv");
}

TEST_F(CmodFresnelPhysicalTest, MSLF_IPH_LCOH_Default) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/TechnologyModels/fresnel_physical/2025.06.02_develop_branch_MSLF_IPH_LCOH.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/TechnologyModels/fresnel_physical/2025.06.02_develop_branch_MSLF_IPH_LCOH_outputs.json";
    std::vector<std::string> compare_number_variables = { "annual_energy" };
    std::vector<std::string> compare_array_variables = { "monthly_energy" };

    // Using absolute differences for test, so need to be careful with this input...
    Test("fresnel_physical_iph", file_inputs, file_outputs, compare_number_variables, compare_array_variables, 100., "tucson_az_32.116521_-110.933042_psmv3_60_tmy.csv");
}

TEST_F(CmodFresnelPhysicalTest, MSLF_IPH_LCOH_EMP_STEAM) {
    std::string file_inputs = SSCDIR;
    // Had to decrease default freeze protection temp to get empirical field + steam heat sink to solve
    file_inputs += "/test/input_json/TechnologyModels/fresnel_physical/2025.06.02_develop_branch_MSLF_IPH_LCOH_emp_steam.json";
    std::string file_outputs = SSCDIR;
   
    char solar_resource_path[256];
    std::string sWeatherFile = "%s/test/input_cases/general_data/tucson_az_32.116521_-110.933042_psmv3_60_tmy.csv";
    int npvy1 = sprintf(solar_resource_path, sWeatherFile.c_str(), std::getenv("SSCDIR")); // TODO - update for robustness
    std::ifstream file(file_inputs);
    std::ostringstream tmp;
    tmp << file.rdbuf();
    file.close();
    ssc_data_t dat_inputs = json_to_ssc_data(tmp.str().c_str());
    ssc_data_set_string(dat_inputs, "file_name", solar_resource_path);

    std::string cmod = "fresnel_physical_iph";

    tmp.str("");
    int errors = run_module(dat_inputs, cmod);

    EXPECT_FALSE(errors);
    if (!errors)
    {
        ssc_data_t dat_outputs = json_to_ssc_data(tmp.str().c_str());

        double annual_energy_calc = std::numeric_limits<double>::quiet_NaN();
        ssc_data_get_number(dat_inputs, "annual_energy", &annual_energy_calc);

        EXPECT_NEAR_FRAC(annual_energy_calc, 16651416.0, 0.001);
    }
    ssc_data_free(dat_inputs);
    dat_inputs = nullptr;
}

