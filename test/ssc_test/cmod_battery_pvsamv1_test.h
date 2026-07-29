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



#ifndef _CMOD_BATTERY_PVSAMV1_TEST_H_
#define _CMOD_BATTERY_PVSAMV1_TEST_H_

#include <gtest/gtest.h>
#include "core.h"

#include "vartab.h"
#include "../ssc/common.h"
#include "../input_cases/pvsamv1_common_data.h"

struct daily_battery_stats {
	daily_battery_stats(std::vector<ssc_number_t> batt_power_data, size_t steps_per_hr=1) {
	    steps_per_hour = steps_per_hr;
	    peakKwDischarge = 0;
	    peakCycles = 0;
	    peakKwCharge = 0;
	    avgCycles = 0;
	    compute(std::move(batt_power_data));
	}
	void compute(std::vector<ssc_number_t> batt_power_data);

	size_t steps_per_hour;
	ssc_number_t peakKwCharge;
	ssc_number_t peakKwDischarge;
	ssc_number_t peakCycles;
	ssc_number_t avgCycles;
};

/**
 * Test the battery dispatch controllers when integrated with various PV systems
 */
class CMPvsamv1BatteryIntegration_cmod_pvsamv1 : public ::testing::Test {

public:

	ssc_data_t data;
	ssc_number_t calculated_value;
	ssc_number_t* calculated_array;
	double m_error_tolerance_hi = 100;
	double m_error_tolerance_lo = 0.1;

	void SetUp()
	{
		data = ssc_data_create();
		pvsamv_nofinancial_default(data);
		calculated_array = new ssc_number_t[8760];
	}
	void TearDown() {
		if (data) {
			ssc_data_free(data);
			data = nullptr;
		}
		if (calculated_array) {
			delete[] calculated_array;
		}
	}
	void SetCalculated(std::string name)
	{
		ssc_data_get_number(data, const_cast<char*>(name.c_str()), &calculated_value);
	}
	void SetCalculatedArray(std::string name)
	{
		int n;
		calculated_array = ssc_data_get_array(data, const_cast<char*>(name.c_str()), &n);
	}
};

#endif // !_CMOD_BATTERY_PVSAMV1_TEST_H_
