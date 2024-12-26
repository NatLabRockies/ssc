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


#ifndef _CMOD_MHK_BUILDER_H_
#define _CMOD_MHK_BUILDER_H_

#include "sscapi.h"

#ifdef __cplusplus
extern "C" {
#endif

static const char* me_array_cable_length_doc =
    "Calculates the cable length in an ME array\\n"
    "Input: var_table with key-value pairs\\n"
    "     'devices_per_row' - double [-]\\n"
    "     'device_spacing_in_row' - double [m]\\n"
	"     'number_rows' - double [-]\\n"
	"     'row_spacing' - double [m]\\n"
	"     'cable_system_overbuild' - double [%]\\n"
    "     'floating_array - int [-]\\n"
    "     'export_cable_redundancy - int [-]\\n"
    "     'water_depth - double [m]\\n"
    "     'number_devices - int [-]\\n"
    "     'distance_to_shore - double [m]\\n"
    "Output: key-value pairs added to var_table\\n"
    "     'inter_array_cable_length' - double [m]\\n";

SSCEXPORT bool me_array_cable_length(ssc_data_t data);

static const char* tidal_turbine_calculate_powercurve_doc =
"Calculates the tidal energy converter power output for tidal velocity bins in an ME array\\n"
"Input: var_table with key-value pairs\\n"
"     'tidal_turbine_rotor_diameter' - double [m]\\n"
"     'number_rotors' - integer [-]\\n"
"     'tidal_turbine_max_cp' - double [-]\\n"
"     'pto_efficiency' - double [%]\\n"
"     'cut_in' - double [m/s]\\n"
"     'cut_out' - double [m/s]\\n"
"     'tidal_resource' - matrix [-]\\n"
"Output: key-value pairs added to var_table\\n"
"     'tidal_turbine_powercurve_tidespeeds' - array [m/s]\\n"
"     'tidal_turbine_powercurve_powerout' - array [kW]\\n"
"     'tidal_turine_powercurve_powerout_rated' - array [kW]\\n"
"     'tidal_turbine_rated_power' - double [kW]\\n"
"     'tidal_turbine_rated_power_rotor' - double [kW]\\n"
"     'error - string [-]\\n";


SSCEXPORT bool tidal_turbine_calculate_powercurve(ssc_data_t data);

static const char* me_array_cable_voltage_doc =
"Calculates the cable voltages in an ME array\\n"
"Input: var_table with key-value pairs\\n"
"     'devices_per_row' - double [-]\\n"
"     'device_rated_power' - double [kW]\\n"
"     'system_capacity' - double [kW]\\n"
"     'device_spacing_in_row' - double [m]\\n"
"     'row_spacing' - double [m]\\n"
"     'inter_array_cable_length' - double [m]\\n"
"     'riser_cable_length' - double [m]\\n"
"     'export_cable_length' - double [m]\\n"
"     'use_onshore_substation' - double [-]\\n"
"     'load_grid_voltage' - double [-]\\n"
"Output: key-value pairs added to var_table\\n"
"     'array_cable_voltage' - double [V]\\n"
"     'array_cable_cost' - double [$]\\n"
"     'array_cable_cost_total' - double [$]\\n"
"     'export_cable_voltage' - double [V]\\n"
"     'export_cable_cost' - double [$]\\n"
"     'export_cable_cost_total' - double [$]\\n"
"     'riser_cable_voltage' - double [V]\\n"
"     'riser_cable_cost' - double [$]\\n"
"     'riser_cable_cost_total' - double [$]\\n"
"     'onshore_substation_cost_total' - double [$]\\n"
"     'offshore_substation_cost_total' - double [$]\\n";

SSCEXPORT bool me_array_cable_voltage(ssc_data_t data);


#ifdef __cplusplus
}
#endif

#endif
