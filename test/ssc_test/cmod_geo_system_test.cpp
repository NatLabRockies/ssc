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

    std::string test_name = "-Hydro Moderate Binary_ATB 2025_Geothermal_Power_Single_Owner_cmod_geothermal";

    std::string file_inputs = SSCDIR + geo_system_test_ns::file_location +
        geo_system_test_ns::input_file_prefix + test_name + ".json";


    std::string file_outputs = SSCDIR + geo_system_test_ns::file_location +
        geo_system_test_ns::output_file_prefix + test_name + "_outputs.json";

    // Use a permissive absolute tolerance for large numbers similar to other tests
    Test("geothermal", file_inputs, file_outputs, geo_system_test_ns::compare_number_variables,
        geo_system_test_ns::compare_array_variables, 100.0, geo_system_test_ns::weather_file);
}

TEST_F(CmodGeoSystemTest, GeoSystem_Hydro_Mod_Flash) {

    std::string test_name = "-Hydro Moderate Flash_ATB 2025_Geothermal_Power_Single_Owner_cmod_geothermal";

    std::string file_inputs = SSCDIR + geo_system_test_ns::file_location +
        geo_system_test_ns::input_file_prefix + test_name + ".json";

    std::string file_outputs = SSCDIR + geo_system_test_ns::file_location +
        geo_system_test_ns::output_file_prefix + test_name + "_outputs.json";

    Test("geothermal", file_inputs, file_outputs, geo_system_test_ns::compare_number_variables,
        geo_system_test_ns::compare_array_variables, 100.0, geo_system_test_ns::weather_file);

}

TEST_F(CmodGeoSystemTest, GeoSystem_Hydro_Adv_Binary) {

    std::string test_name = "-Hydro Advanced Binary_ATB 2025_Geothermal_Power_Single_Owner_cmod_geothermal";

    std::string file_inputs = SSCDIR + geo_system_test_ns::file_location +
        geo_system_test_ns::input_file_prefix + test_name + ".json";

    std::string file_outputs = SSCDIR + geo_system_test_ns::file_location +
        geo_system_test_ns::output_file_prefix + test_name + "_outputs.json";

    Test("geothermal", file_inputs, file_outputs, geo_system_test_ns::compare_number_variables,
        geo_system_test_ns::compare_array_variables, 100.0, geo_system_test_ns::weather_file);
}

TEST_F(CmodGeoSystemTest, GeoSystem_Hydro_Adv_Flash) {

    std::string test_name = "-Hydro Advanced Flash_ATB 2025_Geothermal_Power_Single_Owner_cmod_geothermal";

    std::string file_inputs = SSCDIR + geo_system_test_ns::file_location +
        geo_system_test_ns::input_file_prefix + test_name + ".json";

    std::string file_outputs = SSCDIR + geo_system_test_ns::file_location +
        geo_system_test_ns::output_file_prefix + test_name + "_outputs.json";

    Test("geothermal", file_inputs, file_outputs, geo_system_test_ns::compare_number_variables,
        geo_system_test_ns::compare_array_variables, 100.0, geo_system_test_ns::weather_file);
}

TEST_F(CmodGeoSystemTest, GeoSystem_EGS_Mod_Binary) {

    std::string test_name = "-EGS Moderate Binary_ATB 2025_Geothermal_Power_Single_Owner_cmod_geothermal";

    std::string file_inputs = SSCDIR + geo_system_test_ns::file_location +
        geo_system_test_ns::input_file_prefix + test_name + ".json";

    std::string file_outputs = SSCDIR + geo_system_test_ns::file_location +
        geo_system_test_ns::output_file_prefix + test_name + "_outputs.json";

    Test("geothermal", file_inputs, file_outputs, geo_system_test_ns::compare_number_variables,
        geo_system_test_ns::compare_array_variables, 100.0, geo_system_test_ns::weather_file);
}

TEST_F(CmodGeoSystemTest, GeoSystem_EGS_Mod_Flash) {

    std::string test_name = "-EGS Moderate Flash_ATB 2025_Geothermal_Power_Single_Owner_cmod_geothermal";

    std::string file_inputs = SSCDIR + geo_system_test_ns::file_location +
        geo_system_test_ns::input_file_prefix + test_name + ".json";

    std::string file_outputs = SSCDIR + geo_system_test_ns::file_location +
        geo_system_test_ns::output_file_prefix + test_name + "_outputs.json";

    Test("geothermal", file_inputs, file_outputs, geo_system_test_ns::compare_number_variables,
        geo_system_test_ns::compare_array_variables, 100.0, geo_system_test_ns::weather_file);
}

TEST_F(CmodGeoSystemTest, GeoSystem_EGS_Adv_Binary) {

    std::string test_name = "-EGS Advanced Binary_ATB 2025_Geothermal_Power_Single_Owner_cmod_geothermal";

    std::string file_inputs = SSCDIR + geo_system_test_ns::file_location +
        geo_system_test_ns::input_file_prefix + test_name + ".json";

    std::string file_outputs = SSCDIR + geo_system_test_ns::file_location +
        geo_system_test_ns::output_file_prefix + test_name + "_outputs.json";

    Test("geothermal", file_inputs, file_outputs, geo_system_test_ns::compare_number_variables,
        geo_system_test_ns::compare_array_variables, 100.0, geo_system_test_ns::weather_file);
}

TEST_F(CmodGeoSystemTest, GeoSystem_EGS_Adv_Flash) {

    std::string test_name = "-EGS Advanced Flash_ATB 2025_Geothermal_Power_Single_Owner_cmod_geothermal";

    std::string file_inputs = SSCDIR + geo_system_test_ns::file_location +
        geo_system_test_ns::input_file_prefix + test_name + ".json";

    std::string file_outputs = SSCDIR + geo_system_test_ns::file_location +
        geo_system_test_ns::output_file_prefix + test_name + "_outputs.json";

    Test("geothermal", file_inputs, file_outputs, geo_system_test_ns::compare_number_variables,
        geo_system_test_ns::compare_array_variables, 100.0, geo_system_test_ns::weather_file);
}

TEST_F(CmodGeoSystemTest, GeoSystem_NF_EGS_Mod_Binary) {

    std::string test_name = "-NF EGS Moderate Binary_ATB 2025_Geothermal_Power_Single_Owner_cmod_geothermal";

    std::string file_inputs = SSCDIR + geo_system_test_ns::file_location +
        geo_system_test_ns::input_file_prefix + test_name + ".json";

    std::string file_outputs = SSCDIR + geo_system_test_ns::file_location +
        geo_system_test_ns::output_file_prefix + test_name + "_outputs.json";

    Test("geothermal", file_inputs, file_outputs, geo_system_test_ns::compare_number_variables,
        geo_system_test_ns::compare_array_variables, 100.0, geo_system_test_ns::weather_file);
}

TEST_F(CmodGeoSystemTest, GeoSystem_NF_EGS_Mod_Flash) {

    std::string test_name = "-NF EGS Moderate Flash_ATB 2025_Geothermal_Power_Single_Owner_cmod_geothermal";

    std::string file_inputs = SSCDIR + geo_system_test_ns::file_location +
        geo_system_test_ns::input_file_prefix + test_name + ".json";

    std::string file_outputs = SSCDIR + geo_system_test_ns::file_location +
        geo_system_test_ns::output_file_prefix + test_name + "_outputs.json";

    Test("geothermal", file_inputs, file_outputs, geo_system_test_ns::compare_number_variables,
        geo_system_test_ns::compare_array_variables, 100.0, geo_system_test_ns::weather_file);
}

TEST_F(CmodGeoSystemTest, GeoSystem_NF_EGS_Adv_Binary) {

    std::string test_name = "-NF EGS Advanced Binary_ATB 2025_Geothermal_Power_Single_Owner_cmod_geothermal";

    std::string file_inputs = SSCDIR + geo_system_test_ns::file_location +
        geo_system_test_ns::input_file_prefix + test_name + ".json";

    std::string file_outputs = SSCDIR + geo_system_test_ns::file_location +
        geo_system_test_ns::output_file_prefix + test_name + "_outputs.json";

    Test("geothermal", file_inputs, file_outputs, geo_system_test_ns::compare_number_variables,
        geo_system_test_ns::compare_array_variables, 100.0, geo_system_test_ns::weather_file);
}

TEST_F(CmodGeoSystemTest, GeoSystem_NF_EGS_Adv_Flash) {

    std::string test_name = "-NF EGS Advanced Flash_ATB 2025_Geothermal_Power_Single_Owner_cmod_geothermal";

    std::string file_inputs = SSCDIR + geo_system_test_ns::file_location +
        geo_system_test_ns::input_file_prefix + test_name + ".json";

    std::string file_outputs = SSCDIR + geo_system_test_ns::file_location +
        geo_system_test_ns::output_file_prefix + test_name + "_outputs.json";

    Test("geothermal", file_inputs, file_outputs, geo_system_test_ns::compare_number_variables,
        geo_system_test_ns::compare_array_variables, 100.0, geo_system_test_ns::weather_file);
}


