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



#ifndef _PVWATTS_CASES_
#define _PVWATTS_CASES_

#include <stdio.h>
#include <string>
#include "code_generator_utilities.h"

/**
*   Data for high-level integration test that verifies whether results for a no-financials 
*	PVWatts case matches expected results.  
*	Data generated from code-generator (Shift+F5) within SAM UI.
*   Test uses SSCAPI interfaces (similiar to SDK usage) to pass and receive data to PVWattsV7
*/

static int pvwatts_nofinancial_testfile(ssc_data_t &data)
{
	//this sets whether or not the status prints
	ssc_module_exec_set_print(0);

	//check for out of memory
	if (data == NULL)
	{
		printf("error: out of memory.");
		return -1;
	}

	//set the solar resource file name, using the weather file in the input folder
	//ifdef is so that it can run on multiple operating systems
	char hourly[256];
	int a = sprintf(hourly, "%s/test/input_cases/pvsamv1_data/USA AZ Phoenix (TMY2).csv", std::getenv("SSCDIR"));

	//set the variables for the PVWatts default case
	ssc_data_set_number(data, "system_use_lifetime_output", 0);
    ssc_data_set_number(data, "analysis_period", 2);

	ssc_data_set_string(data, "solar_resource_file", hourly); //file set above
	ssc_data_set_number(data, "system_capacity", 4);
	ssc_data_set_number(data, "module_type", 0);
	ssc_data_set_number(data, "dc_ac_ratio", 1.2000000476837158);
	ssc_data_set_number(data, "bifaciality", 0);

	ssc_data_set_number(data, "array_type", 0);
	ssc_data_set_number(data, "tilt", 20);
	ssc_data_set_number(data, "azimuth", 180);
	ssc_data_set_number(data, "gcr", 0.40000000596046448);

	ssc_data_set_number(data, "losses", 14.075660705566406);
	//ssc_data_set_number(data, "enable_wind_stow", 0);
	ssc_data_set_number(data, "inv_eff", 96);

    ssc_data_set_number(data, "adjust_constant", 0.0);

	return 0;

}

#endif



