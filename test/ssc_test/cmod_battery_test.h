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



#ifndef _CMOD_BATTERY_TEST_H_
#define _CMOD_BATTERY_TEST_H_

#include <gtest/gtest.h>
#include <memory>

#include "core.h"
#include "sscapi.h"

#include "vartab.h"
#include "../ssc/common.h"
#include "../input_cases/code_generator_utilities.h"
#include "../input_cases/battery_common_data.h"

/**
 * CMBattery tests the cmod_battery using the SAM code generator to generate data
 * Eventually a method can be written to write this data to a vartable so that lower-level methods of pvsamv1 can be tested
 * For now, this uses the SSCAPI interfaces to run the compute module and compare results
 */
class CMBattery_cmod_battery : public ::testing::Test {

public:

	ssc_data_t data;
	ssc_number_t calculated_value;
	ssc_number_t * calculated_array;
	double m_error_tolerance_hi = 1.0;
	double m_error_tolerance_lo = 0.1;

	void SetUp()
	{
		data = ssc_data_create();
		battery_commercial_peak_shaving_lifetime(data);

	}
	void TearDown() {
		if (data) {
			ssc_data_free(data);
			data = nullptr;
		}
	}
	void SetCalculated(std::string name)
	{
		ssc_data_get_number(data, const_cast<char *>(name.c_str()), &calculated_value);
	}
	// memory of the array is managed internally to the sscapi.
	void SetCalculatedArray(std::string name)
	{
		int n;
		calculated_array = ssc_data_get_array(data, const_cast<char *>(name.c_str()), &n);
	}
};

class CMBattery_cmod_battery_fom : public CMBattery_cmod_battery {
    void SetUp()
    {
        data = ssc_data_create();
        battery_standalone_merchant_plant_lifetime(data);

    }
};

#endif 
