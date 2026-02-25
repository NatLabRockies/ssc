/*
BSD 3-Clause License

Copyright (c) Alliance for Energy Innovation, LLC. See also https://github.com/NREL/ssc/blob/develop/LICENSE
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


#ifndef _PVSAMV1_COMMON_DATA_H_
#define _PVSAMV1_COMMON_DATA_H_

#include <stdio.h>

#include "code_generator_utilities.h"

extern char solar_resource_path[256];
extern char solar_resource_path_15_min[256];
extern char load_profile_path[256];
extern char target_power_path[256];
extern char sell_rate_path[256];
extern char subarray1_shading[256];
extern char subarray2_shading[256];
extern char temperature_path[256];
extern char solar_resource_path_15min_fail[256];


/**
*  Default data for no-financial pvsamv1 run that can be further modified
*/
void pvsamv_nofinancial_default(ssc_data_t& data);

/**
*  Default data for belpe run that can be further modified
*  Includes a critical load for resiliency. Will increase runtimes if used
*/
void belpe_default(ssc_data_t& data);

/**
* Default battery data that can be further modified
*/
void battery_data_default(ssc_data_t& data);

/**
*  Default data for pvsamv1 residential run that can be further modified
*  Also runs battery_data_default
*/
void pvsamv1_with_residential_default(ssc_data_t& data);

/**
*  Default data for utility_rate5 run that can be further modified
*/
void utility_rate5_default(ssc_data_t& data);

/**
*  Default data for cashloan run that can be further modified
*/
void cashloan_default(ssc_data_t& data);

void setup_residential_utility_rates(ssc_data_t& data);

#endif
