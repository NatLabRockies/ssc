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

#ifndef _CMOD_GETEM_BUILDER_H_
#define _CMOD_GETEM_BUILDER_H_


#include "sscapi.h"

#ifdef __cplusplus
extern "C" {
#endif

static const char* getem_om_cost_calc_doc =    
    "Calculate annual O&M costs for geothermal power plants \\n\\n"
    "Input: var_table with key-value pairs\\n"
    "     'gross_output': double [MW]\\n"
    "     'conversion_type': double [-]\\n"
    "     'baseline_cost': double [$] \\n"
    "     'ppi_base_year': double [-] \\n"
    "     'cwflow': double [kg/s] \\n"
    "     'drilling_cost': double [$] \\n"
    "     'field_gathering_system_cost': double [$] \\n"
    "     'GF_flowrate': double [kg/s] \\n"
    "     'num_wells': double [-] \\n"
    "     'water_loss': double [%] \\n"
    "     'total_installed_cost': double [$] \\n"
    "     'pump_cost_install': double [$] \\n"
    "     'pump_only_cost': double [$] \\n"
    "     'pump_type': double [-] \\n"
    "     'pump_depth': double [m] \\n"
    "Output: key-value pairs added to var_table\\n"
    "     'total_getem_om_cost': double [$]";

SSCEXPORT bool getem_om_cost_calc(ssc_data_t data);



#ifdef __cplusplus
}
#endif
#endif


