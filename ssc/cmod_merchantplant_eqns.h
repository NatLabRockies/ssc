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



#ifndef _CMOD_MERCHANTPLANT_BUILDER_H_
#define _CMOD_MERCHANTPLANT_BUILDER_H_

#include "vartab.h"
#include "sscapi.h"

#ifdef __cplusplus
extern "C" {
#endif

static const char* mp_ancillary_services_doc =
    "Checks the capacities specified in the Ancillary Services markets against the system capacity\\n\\n"
    "Input: var_table with key-value pairs\\n"
    "     'analysis_period': double [-], Years in project lifetime to simulate\\n"
    "     'mp_enable_energy_market_revenue': boolean [-], Enable energy market revenue, 0 is false\\n"
	"     'mp_enable_ancserv1': boolean [-]\\n"
	"     'mp_enable_ancserv2': boolean [-]\\n"
	"     'mp_enable_ancserv3': boolean [-]\\n"
	"     'mp_enable_ancserv4': boolean [-]\\n"
    "     'mp_energy_market_revenue': matrix [MW, $/MW]\\n"
    "     'mp_ancserv1_revenue': matrix [MW, $/MW]\\n"
    "     'mp_ancserv2_revenue': matrix [MW, $/MW]\\n"
    "     'mp_ancserv3_revenue': matrix [MW, $/MW]\\n"
    "     'mp_ancserv4_revenue': matrix [MW, $/MW]\\n"
    "     'mp_energy_market_revenue_single': matrix [MW, $/MW]\\n"
    "     'mp_ancserv1_revenue_single': matrix [MW, $/MW]\\n"
    "     'mp_ancserv2_revenue_single': matrix [MW, $/MW]\\n"
    "     'mp_ancserv3_revenue_single': matrix [MW, $/MW]\\n"
    "     'mp_ancserv4_revenue_single': matrix [MW, $/MW]\\n"
    "     'system_capacity': conditional double [kW], required if gen is not provided\\n"
    "     'gen': conditional array [kW], required if system_capacity is not provided\\n"
    "     'mp_calculate_revenue': boolean [-], 0 is false\\n\\n"
    "Output: key-value pairs added to var_table\\n"
	"     'mp_capacity_check': boolean\\n"
    "     'mp_capacity_check_error': string\\n"
    "     'mp_capacity_check_warning': string\\n";

SSCEXPORT bool mp_ancillary_services(ssc_data_t data);


#ifdef __cplusplus
}
#endif

#endif
