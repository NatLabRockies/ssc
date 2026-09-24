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

#ifndef _CMOD_GEO_SYSTEM_TEST_H_
#define _CMOD_GEO_SYSTEM_TEST_H_

#include "cmod_json_comparison_test.h"

/**
 * Host Developer tests
 */
class CmodGeoSystemTest : public JSONComparisonTest {};

namespace geo_system_test_ns
{
    //const std::string input_file_prefix = "26-09-04-dev";
    const std::string input_file_prefix = "2026-09-20-merge-dev-to-eqs-branch";

    const std::string output_file_prefix = "26-09-04-dev";

    const std::vector<std::string> compare_number_variables = { "annual_energy", "total_installed_cost" }; //, "total_capital_cost", "total_getem_om_cost"

    const std::vector<std::string> compare_array_variables = { "monthly_energy" };

    const std::string file_location = "/test/input_json/TechnologyModels/geothermal/";

    const std::string weather_file = "fargo_nd_46.9_-96.8_mts1_60_tmy.csv";
};

#endif
