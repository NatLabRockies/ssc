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



#ifndef _GRID_COMMON_DATA_H_
#define _GRID_COMMON_DATA_H_

#include <stdio.h>
#include "code_generator_utilities.h"

namespace {
	char load_profile_path_grid[256];
	char gen_path_grid[256];
	char load_profile_path_grid_30[256];
	char gen_path_grid_30[256];
	char gen_path_grid_30_2yr[256];
	char grid_curtailment_default_MW[256];

	int nfc1 = sprintf(gen_path_grid, "%s/test/input_cases/custom_generation_data/energy_output_array.csv", SSCDIR);
	int nfc2 = sprintf(load_profile_path_grid, "%s/test/input_cases/general_data/commercial_load.csv", SSCDIR);
	int nfc3 = sprintf(gen_path_grid_30, "%s/test/input_cases/custom_generation_data/energy_output_array_30min.csv", SSCDIR);
	int nfc4 = sprintf(load_profile_path_grid_30, "%s/test/input_cases/custom_generation_data/load_30min.csv", SSCDIR);
	int nfc5 = sprintf(gen_path_grid_30_2yr, "%s/test/input_cases/custom_generation_data/energy_output_array_30min-2yr.csv", SSCDIR);
	int nfc6 = sprintf(grid_curtailment_default_MW, "%s/test/input_cases/general_data/grid_curtailment_default_MW.csv", SSCDIR);

}

void grid_default_60_min(ssc_data_t &data)
{
	set_array(data, "gen", gen_path_grid, 8760);
	ssc_data_set_number(data, "annual_energy", 0);
	ssc_data_set_number(data, "system_use_lifetime_output", 0);
	ssc_data_set_number(data, "analysis_period", 30);
	set_array(data, "load", load_profile_path_grid, 8760);
    ssc_number_t p_load_escalation[1] = { 0 };
    ssc_data_set_array(data, "load_escalation", p_load_escalation, 1);
    
	ssc_data_set_number(data, "grid_interconnection_limit_kwac", 180000);
	ssc_data_set_number(data, "enable_interconnection_limit", 1);
	set_array(data, "grid_curtailment", grid_curtailment_default_MW, 8760);
}

void grid_default_30_min(ssc_data_t &data)
{
	set_array(data, "gen", gen_path_grid_30, 8760*2);
	ssc_data_set_number(data, "system_use_lifetime_output", 0);
	ssc_data_set_number(data, "annual_energy", 0);
	ssc_data_set_number(data, "analysis_period", 30);
	set_array(data, "load", load_profile_path_grid_30, 8760 * 2);
    ssc_number_t p_load_escalation[1] = { 0 };
    ssc_data_set_array(data, "load_escalation", p_load_escalation, 1);
	ssc_data_set_number(data, "grid_interconnection_limit_kwac", 180000);
	ssc_data_set_number(data, "enable_interconnection_limit", 1);
	set_array(data, "grid_curtailment", grid_curtailment_default_MW, 8760);
}

void grid_default_30_min_lifetime(ssc_data_t &data)
{
	set_array(data, "gen", gen_path_grid_30_2yr, 8760 * 2 * 2);
	ssc_data_set_number(data, "system_use_lifetime_output", 1);
	ssc_data_set_number(data, "annual_energy", 0);
	ssc_data_set_number(data, "analysis_period", 2);
	set_array(data, "load", load_profile_path_grid_30, 8760 * 2);
    ssc_number_t p_load_escalation[1] = { 0 };
    ssc_data_set_array(data, "load_escalation", p_load_escalation, 1);
	ssc_data_set_number(data, "grid_interconnection_limit_kwac", 180000);
	ssc_data_set_number(data, "enable_interconnection_limit", 1);
	set_array(data, "grid_curtailment", grid_curtailment_default_MW, 8760);
}

/// Test to mimic a "no-financial" case, which specifies no analysis period 
void grid_default_60_min_no_financial(ssc_data_t &data)
{
	ssc_data_clear(data);
    ssc_data_set_number(data, "analysis_period", 1);
	set_array(data, "gen", gen_path_grid, 8760);
	set_array(data, "load", load_profile_path_grid, 8760);
    ssc_number_t p_load_escalation[1] = { 0 };
    ssc_data_set_array(data, "load_escalation", p_load_escalation, 1);
	ssc_data_set_number(data, "annual_energy", 0);
	ssc_data_set_number(data, "grid_interconnection_limit_kwac", 180000);
	ssc_data_set_number(data, "enable_interconnection_limit", 1);
	set_array(data, "grid_curtailment", grid_curtailment_default_MW, 8760);
}

#endif
