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



#ifndef _BATTWATTS_CASES_
#define _BATTWATTS_CASES_

#include <stdio.h>
#include <string>
#include "code_generator_utilities.h"
#include "pvsamv1_common_data.h"

/**
* Default data from 2020.2.29 R1, pvwatts-battery residential
*/

char ac_power_path[256];

int v1 = sprintf(ac_power_path, "%s/test/input_cases/battwatts_data/ac_power.csv", SSCDIR); // Normally output by PVWatts

void pvwatts_pv_defaults(ssc_data_t& data) {
	ssc_data_set_string(data, "solar_resource_file", solar_resource_path);
	ssc_data_set_number(data, "system_use_lifetime_output", 0);
    ssc_data_set_number(data, "analysis_period", 2); 
    ssc_data_set_number(data, "system_capacity", 4.6928700000000001);
	ssc_data_set_number(data, "module_type", 0);
	ssc_data_set_number(data, "dc_ac_ratio", 1.2);
	ssc_data_set_number(data, "array_type", 0);
	ssc_data_set_number(data, "tilt", 20);
	ssc_data_set_number(data, "azimuth", 180);
	ssc_data_set_number(data, "gcr", 0.40000000000000002);
	ssc_data_set_number(data, "losses", 14.075660705566406);
	ssc_data_set_number(data, "en_snowloss", 0);
	ssc_data_set_number(data, "inv_eff", 96);
	ssc_data_set_number(data, "batt_simple_enable", 1);

    ssc_data_set_number(data, "adjust_constant", 0.0);
}

void simple_battery_data(ssc_data_t& data) {
	set_array(data, "load", load_profile_path, 8760);
	set_array(data, "ac", ac_power_path, 8760);
	ssc_data_set_number(data, "batt_simple_kwh", 10);
	ssc_data_set_number(data, "batt_simple_kw", 5);
	ssc_data_set_number(data, "batt_simple_chemistry", 1);
	ssc_data_set_number(data, "batt_simple_dispatch", 0);
	ssc_number_t p_batt_custom_dispatch[1] = { 0 };
	ssc_data_set_array(data, "batt_custom_dispatch", p_batt_custom_dispatch, 1);
	ssc_data_set_number(data, "batt_simple_meter_position", 0);
	ssc_data_set_number(data, "inverter_efficiency", 96);

	// Grid
	ssc_data_set_number(data, "enable_interconnection_limit", 0);
}

#endif // _BATTWATTS_CASES_
