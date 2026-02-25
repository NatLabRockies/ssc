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


#ifndef _CMOD_BATTERY_EQNS_H_
#define _CMOD_BATTERY_EQNS_H_

#include "sscapi.h"

#ifdef __cplusplus
extern "C" {
#endif

static const char* size_batterystateful_doc =
    "Resizes the battery for a battery_stateful data object \\n\\n"
    "Input: var_table with key-value pairs\\n"
    "     'nominal_energy': double [kWh]\\n"
    "     'desired_capacity': double [kWh]\\n"
    "     'desired_voltage': double [V]\\n"
    "     'mass': double [kg] \\n"
    "     'surface_area': double [m^2],\\n"
    "     'module_capacity': double [kWh], optional\\n"
    "     'module_surface_area': double [m^2], optional\\n"
    "Output: key-value pairs added to var_table, mass, surface_area, and nominal_energy will be modified\\n"
    "     'original_capacity': kWh [kWh]\\n";


SSCEXPORT bool Size_batterystateful(ssc_data_t data);

static const char* calculate_thermal_params_doc =
    "Resizes the battery for a battery_stateful data object \\n\\n"
    "Input: var_table with key-value pairs\\n"
    "     'original_capacity': double [kWh]\\n"
    "     'desired_capacity': double [kWh]\\n"
    "     'mass': double [kg] \\n"
    "     'surface_area': double [m^2],\\n"
    "     'module_capacity': double [kWh], optional\\n"
    "     'module_surface_area': double [m^2], optional\\n"
    "Output: mass and surface_area will be modified\\n";

SSCEXPORT bool Calculate_thermal_params(ssc_data_t data);

static const char* Reopt_size_standalone_battery_params_doc =
"Given battery costs and a load, get the optimal PV and battery sizes. Wind is disabled.\\n"
"Maps SAM compute module inputs to those of the ReOpt API:\\n"
"Battery technology paired with Residential, Commercial, Third Party or Host Developer financing.\\n\\n"
"Optional: if missing, variable will be set to default value if documented, to REopt defaults otherwise.\\n"
"Conditional: only required if the variable it's meant to replace is missing.\\n"
"REopt's 'microgrid_upgrade_cost_pct' set to 0 because SAM doesn't apply this cost multiplier.\\n"
"REopt inputs not used in this function will be REopt's defaults.\\n"
"The post produced here is not a complete submission. Additional variables uch as lon and lat are required.\\n"
"Function PVSamV1.Reopt_size_battery_params provides a valid post, or users can provide additional inputs via Python or LK.\\n"
"See https ://nrel.github.io/REopt.jl/dev/reopt/inputs/ for a full set of inputs\\n\\n"
"Input: var_table with key-value pairs:  \\n"
"     ++ Battery inputs ++\\n"
"         'batt_dc_ac_efficiency': optional double [%], Battery DC to AC efficiency, 0-100\\n"
"         'batt_ac_dc_efficiency': optional double [%], Inverter AC to battery DC efficiency, 0-100\\n"
"         'batt_dispatch_auto_can_gridcharge: optional boolean, Whether the battery is allowed to charge from the grid. Default is True.\\n"
"         'battery_per_kW': optional double [$/kW], Battery cost per kW\\n"
"         'battery_per_kWh': optional double [$/kWh], Battery cost per kWh\\n"
"         'batt_initial_SOC': optional double [%], Initial State-of-Charge, 0-100\\n"
"         'batt_minimum_SOC': optional double [%], Minimum State-of-Charge, 0-100. Default is 20.\\n"
"         'batt_replacement_schedule_percent': optional array [%], Percentage in each year from start of analysis period to replace battery. Default is [0]\\n"
"         'om_batt_replacement_cost': optional double [$/kWh], Cost to replace battery per kWh\\n"
"     ++ Utility Rate inputs ++\\n"
"         'ur_monthly_fixed_charge': double [$], Monthly fixed charge\\n"
"         'ur_dc_sched_weekday': matrix [tiers], Demand charge weekday schedule, count starts at 1, 12mx24hr\\n"
"         'ur_dc_sched_weekend': matrix [tiers], Demand charge weekend schedule, count starts at 1, 12mx24hr\\n"
"         'ur_dc_tou_mat': matrix [[period, tier, kWh, 'kWh', $/kWh], Energy rates (TOU), each row provides period, tier, max usage, 'kWh' units, and charge\\n"
"         'ur_dc_flat_mat' -matrix [[month, tier, kW, $]] - Demand rates (flat), each row provides month, tier, peak demand and charge \\n"
"         'ur_ec_sched_weekday': matrix [tiers], Energy charge weekday schedule, count starts at 1, 12mx24hr\\n"
"         'ur_ec_sched_weekend': matrix [tiers], Energy charge weekend schedule, count starts at 1, 12mx24hr\\n"
"         'ur_ec_tou_mat': matrix [[period, tier, kw, $], Demand rates (TOU), each row provides period, tier, peak power, and charge\\n"
"         'load': array [kW], Electricity load (year 1)\\n"
"         'crit_load': optional array [kW], Critical electricity load (year 1)\\n"
"     ++ Financial inputs ++\\n"
"         'analysis_period': double [years]\\n"
"         'rate_escalation': double [%/year], Annual electricity rate escalation, 0-100\\n"
"         'inflation_rate': double [%], 0-100\\n"
"         'federal_tax_rate': optional double [%], Used to calculate offtaker tax percent, 0-100\\n"
"         'state_tax_rate': optional double [%], Used to calculate offtaker tax percent, 0-100\\n"
"         'real_discount_rate': optional double [%], 0-100. Default is 6.4\\n"
"         'om_fixed_escal': optional double [%/year], Fixed O&M escalation\\n"
"         'om_production_escal': optional double [%/year], Production-based O&M escalation\\n"
"         'total_installed_cost': optional double [$]\\n"
"         'value_of_lost_load': optional double [$/kWh], Value placed on unmet site load during grid outages\\n\\n"
"Output: key-value pairs added to var_table\\n"
"         'reopt_scenario': table, Scenario inputs to Reopt API\\n"
"         'log': string";

SSCEXPORT bool Reopt_size_standalone_battery_params(ssc_data_t data);

#ifdef __cplusplus
}
#endif

#endif
