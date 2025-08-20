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
#include "trough_physical_iph_defaults.h"
#include "cmod_trough_physical_iph.h"
#include "csp_common_test.h"
#include "vs_google_test_explorer_namespace.h"

namespace csp_trough {}
using namespace csp_trough;

//========Tests===================================================================================
NAMESPACE_TEST(csp_trough, HeatTroughCmod, Default_NoFinancial)
{
    ssc_data_t defaults = trough_physical_iph_defaults();
    CmodUnderTest heat_trough = CmodUnderTest("trough_physical_iph", defaults);
    int errors = heat_trough.RunModule();
    EXPECT_FALSE(errors);
    if (!errors) {

        //double annual_gross_energy = heat_trough.GetOutput("annual_gross_energy");
        double annual_energy = heat_trough.GetOutput("annual_energy");
        double annual_electricity_consumption = heat_trough.GetOutput("annual_electricity_consumption");
        double annual_thermal_consumption = heat_trough.GetOutput("annual_thermal_consumption");
        double annual_tes_freeze_protection = heat_trough.GetOutput("annual_tes_freeze_protection");
        double annual_field_freeze_protection = heat_trough.GetOutput("annual_field_freeze_protection");
        double annual_total_water_use = heat_trough.GetOutput("annual_total_water_use");

        //EXPECT_NEAR_FRAC(heat_trough.GetOutput("annual_gross_energy"), 24267285, kErrorToleranceHi);
        EXPECT_NEAR_FRAC(heat_trough.GetOutput("annual_energy"), 24142307, kErrorToleranceHi);
        EXPECT_NEAR_FRAC(heat_trough.GetOutput("annual_electricity_consumption"), 94120, kErrorToleranceHi);
        EXPECT_NEAR_FRAC(heat_trough.GetOutput("annual_thermal_consumption"), 182.81, kErrorToleranceHi);
        EXPECT_NEAR_FRAC(heat_trough.GetOutput("annual_tes_freeze_protection"), 182.81, kErrorToleranceHi);
        EXPECT_NEAR(heat_trough.GetOutput("annual_field_freeze_protection"), 0., kErrorToleranceHi);
        EXPECT_NEAR_FRAC(heat_trough.GetOutput("annual_total_water_use"), 176.3, kErrorToleranceHi);

    }
}

TEST_F(CmodPhysicalTroughIPHTest, TROUGH_IPH_NOFIN_THERMOCLINE) {
    std::string file_inputs = SSCDIR;
    // Had to decrease default freeze protection temp to get empirical field + steam heat sink to solve
    file_inputs += "/test/input_json/TechnologyModels/physical_trough/2025.06.02_develop_phys_trough_iph_nofin_thermocline.json";
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

    std::string cmod = "trough_physical_iph";

    tmp.str("");
    int errors = run_module(dat_inputs, cmod);

    EXPECT_FALSE(errors);
    if (!errors)
    {
        ssc_data_t dat_outputs = json_to_ssc_data(tmp.str().c_str());

        double annual_energy_calc = std::numeric_limits<double>::quiet_NaN();
        ssc_data_get_number(dat_inputs, "annual_energy", &annual_energy_calc);

        EXPECT_NEAR_FRAC(annual_energy_calc, 20655216.0, 0.001);
    }
    ssc_data_free(dat_inputs);
    dat_inputs = nullptr;
}
