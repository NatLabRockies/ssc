/*
BSD 3-Clause License

Copyright (c) Alliance for Energy Innovation, LLC. See also https://github.com/NatLabRockies/ssc/blob/develop/LICENSE
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

#include "cmod_geo_system_test.h"

// Tests use input / output JSON files under:
// C:\Users\tneises\Documents\Projects\Repos\ssc\test\input_json\TechnologyModels\geothermal

TEST_F(CmodGeoSystemTest, GeoSystem_Hydro_Mod_Binary) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/TechnologyModels/geothermal/26-09-04-dev-Hydro Moderate Binary_ATB 2025_Geothermal_Power_Single_Owner_cmod_geothermal.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/TechnologyModels/geothermal/26-09-04-dev-Hydro Moderate Binary_ATB 2025_Geothermal_Power_Single_Owner_cmod_geothermal_outputs.json";

    //std::ifstream file(file_inputs);
    //std::ostringstream tmp;
    //tmp << file.rdbuf();
    //file.close();
    //ssc_data_t dat_inputs = json_to_ssc_data(tmp.str().c_str());

    std::vector<std::string> compare_number_variables = { "annual_energy", "net_capital_costs", "fixed_om_costs" };
    std::vector<std::string> compare_array_variables = { "monthly_energy" };

    // Use a permissive absolute tolerance for large numbers similar to other tests
    Test("geothermal", file_inputs, file_outputs, compare_number_variables, compare_array_variables, 100.0, "fargo_nd_46.9_-96.8_mts1_60_tmy.csv");
}

TEST_F(CmodGeoSystemTest, GeoSystem_Hydro_Mod_Flash) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/TechnologyModels/geothermal/26-09-04-dev-Hydro Moderate Flash_ATB 2025_Geothermal_Power_Single_Owner_cmod_geothermal.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/TechnologyModels/geothermal/26-09-04-dev-Hydro Moderate Flash_ATB 2025_Geothermal_Power_Single_Owner_cmod_geothermal_outputs.json";

    std::vector<std::string> compare_number_variables = { "annual_energy", "net_capital_costs", "fixed_om_costs" };
    std::vector<std::string> compare_array_variables = { "monthly_energy" };

    // Use a permissive absolute tolerance for large numbers similar to other tests
    Test("geothermal", file_inputs, file_outputs, compare_number_variables, compare_array_variables, 100.0, "fargo_nd_46.9_-96.8_mts1_60_tmy.csv");
}

TEST_F(CmodGeoSystemTest, GeoSystem_Hydro_Adv_Binary) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/TechnologyModels/geothermal/26-09-04-dev-Hydro Advanced Binary_ATB 2025_Geothermal_Power_Single_Owner_cmod_geothermal.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/TechnologyModels/geothermal/26-09-04-dev-Hydro Advanced Binary_ATB 2025_Geothermal_Power_Single_Owner_cmod_geothermal_outputs.json";

    std::vector<std::string> compare_number_variables = { "annual_energy", "net_capital_costs", "fixed_om_costs" };
    std::vector<std::string> compare_array_variables = { "monthly_energy" };

    // Use a permissive absolute tolerance for large numbers similar to other tests
    Test("geothermal", file_inputs, file_outputs, compare_number_variables, compare_array_variables, 100.0, "fargo_nd_46.9_-96.8_mts1_60_tmy.csv");
}

TEST_F(CmodGeoSystemTest, GeoSystem_Hydro_Adv_Flash) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/TechnologyModels/geothermal/26-09-04-dev-Hydro Advanced Flash_ATB 2025_Geothermal_Power_Single_Owner_cmod_geothermal.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/TechnologyModels/geothermal/26-09-04-dev-Hydro Advanced Flash_ATB 2025_Geothermal_Power_Single_Owner_cmod_geothermal_outputs.json";

    std::vector<std::string> compare_number_variables = { "annual_energy", "net_capital_costs", "fixed_om_costs" };
    std::vector<std::string> compare_array_variables = { "monthly_energy" };

    // Use a permissive absolute tolerance for large numbers similar to other tests
    Test("geothermal", file_inputs, file_outputs, compare_number_variables, compare_array_variables, 100.0, "fargo_nd_46.9_-96.8_mts1_60_tmy.csv");
}

TEST_F(CmodGeoSystemTest, GeoSystem_EGS_Mod_Binary) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/TechnologyModels/geothermal/26-09-04-dev-EGS Moderate Binary_ATB 2025_Geothermal_Power_Single_Owner_cmod_geothermal.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/TechnologyModels/geothermal/26-09-04-dev-EGS Moderate Binary_ATB 2025_Geothermal_Power_Single_Owner_cmod_geothermal_outputs.json";

    std::vector<std::string> compare_number_variables = { "annual_energy", "net_capital_costs", "fixed_om_costs" };
    std::vector<std::string> compare_array_variables = { "monthly_energy" };

    // Use a permissive absolute tolerance for large numbers similar to other tests
    Test("geothermal", file_inputs, file_outputs, compare_number_variables, compare_array_variables, 100.0, "fargo_nd_46.9_-96.8_mts1_60_tmy.csv");
}

TEST_F(CmodGeoSystemTest, GeoSystem_EGS_Mod_Flash) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/TechnologyModels/geothermal/26-09-04-dev-EGS Moderate Flash_ATB 2025_Geothermal_Power_Single_Owner_cmod_geothermal.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/TechnologyModels/geothermal/26-09-04-dev-EGS Moderate Flash_ATB 2025_Geothermal_Power_Single_Owner_cmod_geothermal_outputs.json";

    std::vector<std::string> compare_number_variables = { "annual_energy", "net_capital_costs", "fixed_om_costs" };
    std::vector<std::string> compare_array_variables = { "monthly_energy" };

    // Use a permissive absolute tolerance for large numbers similar to other tests
    Test("geothermal", file_inputs, file_outputs, compare_number_variables, compare_array_variables, 100.0, "fargo_nd_46.9_-96.8_mts1_60_tmy.csv");
}

TEST_F(CmodGeoSystemTest, GeoSystem_EGS_Adv_Binary) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/TechnologyModels/geothermal/26-09-04-dev-EGS Advanced Binary_ATB 2025_Geothermal_Power_Single_Owner_cmod_geothermal.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/TechnologyModels/geothermal/26-09-04-dev-EGS Advanced Binary_ATB 2025_Geothermal_Power_Single_Owner_cmod_geothermal_outputs.json";

    std::vector<std::string> compare_number_variables = { "annual_energy", "net_capital_costs", "fixed_om_costs" };
    std::vector<std::string> compare_array_variables = { "monthly_energy" };

    // Use a permissive absolute tolerance for large numbers similar to other tests
    Test("geothermal", file_inputs, file_outputs, compare_number_variables, compare_array_variables, 100.0, "fargo_nd_46.9_-96.8_mts1_60_tmy.csv");
}

TEST_F(CmodGeoSystemTest, GeoSystem_EGS_Adv_Flash) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/TechnologyModels/geothermal/26-09-04-dev-EGS Advanced Flash_ATB 2025_Geothermal_Power_Single_Owner_cmod_geothermal.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/TechnologyModels/geothermal/26-09-04-dev-EGS Advanced Flash_ATB 2025_Geothermal_Power_Single_Owner_cmod_geothermal_outputs.json";

    std::vector<std::string> compare_number_variables = { "annual_energy", "net_capital_costs", "fixed_om_costs" };
    std::vector<std::string> compare_array_variables = { "monthly_energy" };

    // Use a permissive absolute tolerance for large numbers similar to other tests
    Test("geothermal", file_inputs, file_outputs, compare_number_variables, compare_array_variables, 100.0, "fargo_nd_46.9_-96.8_mts1_60_tmy.csv");
}

TEST_F(CmodGeoSystemTest, GeoSystem_NF_EGS_Mod_Binary) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/TechnologyModels/geothermal/26-09-04-dev-NF EGS Moderate Binary_ATB 2025_Geothermal_Power_Single_Owner_cmod_geothermal.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/TechnologyModels/geothermal/26-09-04-dev-NF EGS Moderate Binary_ATB 2025_Geothermal_Power_Single_Owner_cmod_geothermal_outputs.json";

    std::vector<std::string> compare_number_variables = { "annual_energy", "net_capital_costs", "fixed_om_costs" };
    std::vector<std::string> compare_array_variables = { "monthly_energy" };

    // Use a permissive absolute tolerance for large numbers similar to other tests
    Test("geothermal", file_inputs, file_outputs, compare_number_variables, compare_array_variables, 100.0, "fargo_nd_46.9_-96.8_mts1_60_tmy.csv");
}

TEST_F(CmodGeoSystemTest, GeoSystem_NF_EGS_Mod_Flash) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/TechnologyModels/geothermal/26-09-04-dev-NF EGS Moderate Flash_ATB 2025_Geothermal_Power_Single_Owner_cmod_geothermal.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/TechnologyModels/geothermal/26-09-04-dev-NF EGS Moderate Flash_ATB 2025_Geothermal_Power_Single_Owner_cmod_geothermal_outputs.json";

    std::vector<std::string> compare_number_variables = { "annual_energy", "net_capital_costs", "fixed_om_costs" };
    std::vector<std::string> compare_array_variables = { "monthly_energy" };

    // Use a permissive absolute tolerance for large numbers similar to other tests
    Test("geothermal", file_inputs, file_outputs, compare_number_variables, compare_array_variables, 100.0, "fargo_nd_46.9_-96.8_mts1_60_tmy.csv");
}

TEST_F(CmodGeoSystemTest, GeoSystem_NF_EGS_Adv_Binary) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/TechnologyModels/geothermal/26-09-04-dev-NF EGS Advanced Binary_ATB 2025_Geothermal_Power_Single_Owner_cmod_geothermal.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/TechnologyModels/geothermal/26-09-04-dev-NF EGS Advanced Binary_ATB 2025_Geothermal_Power_Single_Owner_cmod_geothermal_outputs.json";

    std::vector<std::string> compare_number_variables = { "annual_energy", "net_capital_costs", "fixed_om_costs" };
    std::vector<std::string> compare_array_variables = { "monthly_energy" };

    // Use a permissive absolute tolerance for large numbers similar to other tests
    Test("geothermal", file_inputs, file_outputs, compare_number_variables, compare_array_variables, 100.0, "fargo_nd_46.9_-96.8_mts1_60_tmy.csv");
}

TEST_F(CmodGeoSystemTest, GeoSystem_NF_EGS_Adv_Flash) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/TechnologyModels/geothermal/26-09-04-dev-NF EGS Advanced Flash_ATB 2025_Geothermal_Power_Single_Owner_cmod_geothermal.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/TechnologyModels/geothermal/26-09-04-dev-NF EGS Advanced Flash_ATB 2025_Geothermal_Power_Single_Owner_cmod_geothermal_outputs.json";

    std::vector<std::string> compare_number_variables = { "annual_energy", "net_capital_costs", "fixed_om_costs" };
    std::vector<std::string> compare_array_variables = { "monthly_energy" };

    // Use a permissive absolute tolerance for large numbers similar to other tests
    Test("geothermal", file_inputs, file_outputs, compare_number_variables, compare_array_variables, 100.0, "fargo_nd_46.9_-96.8_mts1_60_tmy.csv");
}


