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


#ifndef _CMOD_JSON_COMPARISON_TEST_H_
#define _CMOD_JSON_COMPARISON_TEST_H_

#include <gtest/gtest.h>
#include <memory>

#include "core.h"
#include "sscapi.h"

#include "vartab.h"
#include "../ssc/common.h"
#include "../input_cases/code_generator_utilities.h"


#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <cmath>
#include "../rapidjson/document.h"
#include "../rapidjson/istreamwrapper.h"


/**
 * JSON Comparison base class for tests
 */
class JSONComparisonTest : public ::testing::Test {

public:

	ssc_number_t calculated_value;
	ssc_number_t * calculated_array;
	double m_error_tolerance_hi = 1.0;
	double m_error_tolerance_lo = 0.1;
	size_t interval = 100;

public:
    void Test(const std::string& compute_module, const std::string& file_inputs, const std::string &file_outputs, const std::vector<std::string> &compare_number_variables, const std::vector<std::string> &compare_array_variables, double tolerance = 0.001, std::string weather_file="phoenix_az_33.450495_-111.983688_psmv3_60_tmy.csv") {
        char solar_resource_path[256];
        std::string sWeatherFile = "%s/test/input_cases/general_data/" + weather_file;
        int npvy1 = sprintf(solar_resource_path, sWeatherFile.c_str(), std::getenv("SSCDIR")); // TODO - update for robustness
        std::ifstream file(file_inputs);
        std::ostringstream tmp;
        tmp << file.rdbuf();
        file.close();
        ssc_data_t dat_inputs = json_to_ssc_data(tmp.str().c_str());
        ssc_data_set_string(dat_inputs, "solar_resource_file", solar_resource_path);

        // Some cmods use "file_name" instead of "solar_resource_file"
        // Brute force approach - define both
        ssc_data_set_string(dat_inputs, "file_name", solar_resource_path);

        tmp.str("");
        int errors = run_module(dat_inputs, compute_module);

        EXPECT_FALSE(errors);
        if (!errors)
        {
            file.open(file_outputs);
            tmp << file.rdbuf();
            file.close();
            ssc_data_t dat_outputs = json_to_ssc_data(tmp.str().c_str());
            tmp.str("");

            std::vector<ssc_number_t> values_to_compare(compare_number_variables.size());
            std::vector<ssc_number_t> values_to_match(compare_number_variables.size());

            std::vector< std::vector<ssc_number_t> > arrays_to_compare(compare_array_variables.size());
            std::vector< std::vector<ssc_number_t> > arrays_to_match(compare_array_variables.size());

            for (size_t i = 0; i < compare_number_variables.size(); i++) {
                ssc_data_get_number(dat_inputs, compare_number_variables[i].c_str(), &values_to_compare[i]);
                ssc_data_get_number(dat_outputs, compare_number_variables[i].c_str(), &values_to_match[i]);
                if (!isnan(values_to_compare[i]) || !isnan(values_to_match[i]))
                    EXPECT_NEAR(values_to_compare[i], values_to_match[i], tolerance) << " number issue at index i=" << i << " for " << compare_number_variables[i];
            }

            int len_currentrun, len_comparerun;
            for (size_t i = 0; i < compare_array_variables.size(); i++) {
                auto pCurrentOutputs = ssc_data_get_array(dat_inputs, compare_array_variables[i].c_str(), &len_currentrun);
                auto pCompareOutputs = ssc_data_get_array(dat_outputs, compare_array_variables[i].c_str(), &len_comparerun);
                EXPECT_EQ(len_currentrun, len_comparerun) << " for " << compare_array_variables[i];
                for (int j = 0; j < len_currentrun && j < len_comparerun; j++) {
                    EXPECT_NEAR(pCurrentOutputs[j], pCompareOutputs[j], tolerance) << " array issue at index i=" << i << " and array index j=" << j << " for " << compare_array_variables[i];
                }
            }

            ssc_data_free(dat_outputs);
            dat_outputs = nullptr;
        }
        ssc_data_free(dat_inputs);
        dat_inputs = nullptr;

    }

};

#endif 
