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



#ifndef _BATTERY_COMMON_DATA_H_
#define _BATTERY_COMMON_DATA_H_

#include <stdio.h>
#include "code_generator_utilities.h"

namespace {
	char load_profile_path_batt[256];
	char temperature_path[256];
	char temperature_path_30min[256];
	char gen_path[256];
    char grid_path[256];
    char mp_path[256];
	int nb1 = sprintf(gen_path, "%s/test/input_cases/battery_data/lifetime_gen.csv", SSCDIR);
	int nb2 = sprintf(load_profile_path_batt, "%s/test/input_cases/general_data/commercial_load.csv", SSCDIR);
	int nb3 = sprintf(temperature_path, "%s/test/input_cases/battery_data/batt_room_temperature_celsius_60min.csv", SSCDIR);
	int nb4 = sprintf(temperature_path_30min, "%s/test/input_cases/battery_data/batt_room_temperature_celsius_30min.csv", SSCDIR);
    int nb5 = sprintf(grid_path, "%s/test/input_cases/battery_data/grid_curtailment.csv", SSCDIR);
    int nb6 = sprintf(mp_path, "%s/test/input_cases/battery_data/mp_energy_market_revenue_single.csv", SSCDIR);

	/**
	*  data for commercial peak-shaving battery run that can be further modified
	*/
	void battery_commercial_peak_shaving_lifetime(ssc_data_t &data)
	{
		set_array(data, "gen", gen_path, 175200);
		ssc_data_set_number(data, "system_use_lifetime_output", 1);
		ssc_data_set_number(data, "analysis_period", 20);
		set_array(data, "load", load_profile_path_batt, 8760);
		ssc_data_set_number(data, "en_batt", 1);
        ssc_data_set_number(data, "en_standalone_batt", 0);
		ssc_data_set_number(data, "batt_replacement_option", 2);
		ssc_data_set_number(data, "batt_chem", 1);
		ssc_data_set_number(data, "batt_ac_or_dc", 1);
		ssc_data_set_number(data, "batt_dc_dc_efficiency", 99);
		ssc_data_set_number(data, "batt_dc_ac_efficiency", 98);
		ssc_data_set_number(data, "batt_ac_dc_efficiency", 98);
		ssc_data_set_number(data, "batt_meter_position", 0);
		ssc_number_t p_batt_losses[1] = { 0 };
		ssc_data_set_array(data, "batt_losses", p_batt_losses, 1);
		ssc_number_t p_batt_losses_charging[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
		ssc_data_set_array(data, "batt_losses_charging", p_batt_losses_charging, 12);
		ssc_number_t p_batt_losses_discharging[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
		ssc_data_set_array(data, "batt_losses_discharging", p_batt_losses_discharging, 12);
		ssc_number_t p_batt_losses_idle[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
		ssc_data_set_array(data, "batt_losses_idle", p_batt_losses_idle, 12);
		ssc_data_set_number(data, "batt_loss_choice", 0);
		ssc_data_set_number(data, "batt_current_choice", 1);
		ssc_data_set_number(data, "batt_computed_strings", 12148);
		ssc_data_set_number(data, "batt_computed_series", 14);
		ssc_data_set_number(data, "batt_computed_bank_capacity", 612.2591552734375);
		ssc_data_set_number(data, "batt_current_charge_max", 3037);
		ssc_data_set_number(data, "batt_current_discharge_max", 3037);
		ssc_data_set_number(data, "batt_inverter_efficiency_cutoff", 90);
		ssc_data_set_number(data, "batt_power_charge_max_kwdc", 153.06478881835938);
		ssc_data_set_number(data, "batt_power_discharge_max_kwdc", 153.06478881835938);
		ssc_data_set_number(data, "batt_power_charge_max_kwac", 153.06478881835938);
		ssc_data_set_number(data, "batt_power_discharge_max_kwac", 153.06478881835938);
		ssc_data_set_number(data, "batt_voltage_choice", 0);
		ssc_data_set_number(data, "batt_Vfull", 4.0999999046325684);
		ssc_data_set_number(data, "batt_Vexp", 4.0500001907348633);
		ssc_data_set_number(data, "batt_Vnom", 3.4000000953674316);
        ssc_data_set_number(data, "batt_Vcut", 0.66 * 4.0999999046325684);
		ssc_data_set_number(data, "batt_Vnom_default", 3.5999999046325684);
		ssc_data_set_number(data, "batt_Qfull", 1);
		ssc_data_set_number(data, "batt_Qfull_flow", 12148);
		ssc_data_set_number(data, "batt_Qexp", 0.017799999564886093);
		ssc_data_set_number(data, "batt_Qnom", 0.88899999856948853);
		ssc_data_set_number(data, "batt_C_rate", 0.20000000298023224);
		ssc_data_set_number(data, "batt_resistance", 0.0002);
		ssc_number_t p_batt_voltage_matrix[2] = { 0, 0 };
		ssc_data_set_matrix(data, "batt_voltage_matrix", p_batt_voltage_matrix, 1, 2);
		ssc_data_set_number(data, "LeadAcid_q20_computed", 12148);
		ssc_data_set_number(data, "LeadAcid_q10_computed", 11297.6396484375);
		ssc_data_set_number(data, "LeadAcid_qn_computed", 7288.7998046875);
		ssc_data_set_number(data, "LeadAcid_tn", 1);
		ssc_data_set_number(data, "batt_initial_SOC", 50);
		ssc_data_set_number(data, "batt_minimum_SOC", 15);
		ssc_data_set_number(data, "batt_maximum_SOC", 95);
		ssc_data_set_number(data, "batt_minimum_modetime", 10);
        ssc_data_set_number(data, "batt_life_model", 0);
        ssc_number_t p_batt_lifetime_matrix[18] = { 20, 0, 100, 20, 5000, 80, 20, 10000, 60, 80, 0, 100, 80, 1000, 80, 80, 2000, 60 };
		ssc_data_set_matrix(data, "batt_lifetime_matrix", p_batt_lifetime_matrix, 6, 3);
		ssc_data_set_number(data, "batt_calendar_choice", 1);
		ssc_number_t p_batt_calendar_lifetime_matrix[6] = { 0, 100, 3650, 80, 7300, 50 };
		ssc_data_set_matrix(data, "batt_calendar_lifetime_matrix", p_batt_calendar_lifetime_matrix, 3, 2);
		ssc_data_set_number(data, "batt_calendar_q0", 1.0199999809265137);
		ssc_data_set_number(data, "batt_calendar_a", 0.0026599999982863665);
		ssc_data_set_number(data, "batt_calendar_b", -7280);
		ssc_data_set_number(data, "batt_calendar_c", 930);
		ssc_data_set_number(data, "batt_replacement_capacity", 0);
		ssc_number_t p_batt_replacement_schedule[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
		ssc_data_set_array(data, "batt_replacement_schedule", p_batt_replacement_schedule, 10);
		ssc_number_t p_batt_replacement_schedule_percent[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 100 };
		ssc_data_set_array(data, "batt_replacement_schedule_percent", p_batt_replacement_schedule_percent, 10);
		ssc_number_t p_replacement_cost[1] = { 68 };
		ssc_data_set_array(data, "om_batt_replacement_cost", p_replacement_cost, 1);
		ssc_data_set_number(data, "batt_mass", 15);
		ssc_data_set_number(data, "batt_surface_area", 11.92);
		ssc_data_set_number(data, "batt_Cp", 1004);
		ssc_data_set_number(data, "batt_h_to_ambient", 20);
		set_array(data, "batt_room_temperature_celsius", temperature_path, 8760);
		ssc_number_t p_cap_vs_temp[8] = { -10, 60, 0, 80, 25, 100, 40, 100 };
		ssc_data_set_matrix(data, "cap_vs_temp", p_cap_vs_temp, 4, 2);
		ssc_number_t p_dispatch_manual_charge[6] = { 1, 1, 1, 0, 0, 0 };
		ssc_data_set_array(data, "dispatch_manual_charge", p_dispatch_manual_charge, 6);
		ssc_number_t p_dispatch_manual_discharge[6] = { 0, 0, 1, 0, 0, 0 };
		ssc_data_set_array(data, "dispatch_manual_discharge", p_dispatch_manual_discharge, 6);
        ssc_number_t p_dispatch_manual_discharge_to_grid[6] = { 0, 0, 0, 0, 0, 0 };
        ssc_data_set_array(data, "dispatch_manual_btm_discharge_to_grid", p_dispatch_manual_discharge_to_grid, 6);
		ssc_number_t p_dispatch_manual_gridcharge[6] = { 0, 1, 0, 0, 0, 0 };
		ssc_data_set_array(data, "dispatch_manual_gridcharge", p_dispatch_manual_gridcharge, 6);
		ssc_number_t p_dispatch_manual_percent_discharge[2] = { 25, 0 };
		ssc_data_set_array(data, "dispatch_manual_percent_discharge", p_dispatch_manual_percent_discharge, 2);
		ssc_number_t p_dispatch_manual_percent_gridcharge[2] = { 100, 0 };
		ssc_data_set_array(data, "dispatch_manual_percent_gridcharge", p_dispatch_manual_percent_gridcharge, 2);
		ssc_number_t p_batt_target_power[1] = { 15 };
		ssc_data_set_array(data, "batt_target_power", p_batt_target_power, 1);
		ssc_number_t p_batt_target_power_monthly[1] = { 0 };
		ssc_data_set_array(data, "batt_target_power_monthly", p_batt_target_power_monthly, 1);
		ssc_data_set_number(data, "batt_target_choice", 0);
		ssc_number_t p_batt_custom_dispatch[1] = { 0 };
		ssc_data_set_array(data, "batt_custom_dispatch", p_batt_custom_dispatch, 1);
		ssc_data_set_number(data, "batt_dispatch_choice", 0);
		ssc_data_set_number(data, "batt_dispatch_auto_can_gridcharge", 1);
		ssc_data_set_number(data, "batt_dispatch_auto_can_charge", 1);
		ssc_number_t p_ur_ec_sched_weekday[288] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
		ssc_data_set_matrix(data, "ur_ec_sched_weekday", p_ur_ec_sched_weekday, 12, 24);
		ssc_number_t p_ur_ec_sched_weekend[288] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
		ssc_data_set_matrix(data, "ur_ec_sched_weekend", p_ur_ec_sched_weekend, 12, 24);
		ssc_number_t p_ur_ec_tou_mat[24] = { 1, 1, 200, 1, 0.21174600720405579, 0.028000000864267349, 1, 2, 400, 1, 0.057693801820278168, 0.028000000864267349, 1, 3, 600, 1, 0.052770901471376419, 0.028000000864267349, 1, 4, 10000, 1, 0.049003798514604568, 0.028000000864267349 };
		ssc_data_set_matrix(data, "ur_ec_tou_mat", p_ur_ec_tou_mat, 4, 6);
        ssc_data_set_number(data, "batt_cycle_cost_choice", 0);
        ssc_data_set_number(data, "batt_dispatch_charge_only_system_exceeds_load", 1);
        ssc_data_set_number(data, "batt_dispatch_discharge_only_load_exceeds_system", 1);
        ssc_data_set_number(data, "batt_dispatch_auto_btm_can_discharge_to_grid", 0);
	}

    /**
      *  data for merchant plant automatic dispatch battery run that can be further modified
      */
    void battery_standalone_merchant_plant_lifetime(ssc_data_t& data)
    {
        ssc_data_set_number(data, "system_use_lifetime_output", 1);
        ssc_data_set_number(data, "analysis_period", 1);
        ssc_data_set_number(data, "en_batt", 1);
        ssc_data_set_number(data, "en_standalone_batt", 1);
        ssc_data_set_number(data, "timestep_minutes", 60);
        ssc_data_set_number(data, "batt_chem", 1);
        ssc_data_set_number(data, "batt_ac_or_dc", 1);
        ssc_data_set_number(data, "batt_dc_dc_efficiency", 99);
        ssc_data_set_number(data, "batt_dc_ac_efficiency", 96);
        ssc_data_set_number(data, "batt_ac_dc_efficiency", 96);
        ssc_data_set_number(data, "batt_meter_position", 1);
        ssc_data_set_number(data, "batt_inverter_efficiency_cutoff", 90);
        ssc_number_t p_batt_losses[1] = { 0 };
        ssc_data_set_array(data, "batt_losses", p_batt_losses, 1);
        ssc_number_t p_batt_losses_charging[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        ssc_data_set_array(data, "batt_losses_charging", p_batt_losses_charging, 12);
        ssc_number_t p_batt_losses_discharging[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        ssc_data_set_array(data, "batt_losses_discharging", p_batt_losses_discharging, 12);
        ssc_number_t p_batt_losses_idle[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        ssc_data_set_array(data, "batt_losses_idle", p_batt_losses_idle, 12);
        ssc_data_set_number(data, "batt_loss_choice", 0);
        ssc_data_set_number(data, "batt_current_choice", 0);
        ssc_data_set_number(data, "batt_computed_strings", 149880);
        ssc_data_set_number(data, "batt_computed_series", 139);
        ssc_data_set_number(data, "batt_computed_bank_capacity", 239999.84640000004);
        ssc_data_set_number(data, "batt_current_charge_max", 119904);
        ssc_data_set_number(data, "batt_current_discharge_max", 119904);
        ssc_data_set_number(data, "batt_power_charge_max_kwdc", 59999.96160000001);
        ssc_data_set_number(data, "batt_power_discharge_max_kwdc", 59999.96160000001);
        ssc_data_set_number(data, "batt_power_charge_max_kwac", 62499.960000000014);
        ssc_data_set_number(data, "batt_power_discharge_max_kwac", 57599.963136000006);
        ssc_data_set_number(data, "batt_voltage_choice", 0);
        ssc_data_set_number(data, "batt_Vfull", 4.2000000000000002);
        ssc_data_set_number(data, "batt_Vexp", 3.5299999999999998);
        ssc_data_set_number(data, "batt_Vnom", 3.3420000000000001);
        ssc_data_set_number(data, "batt_Vnom_default", 3.6000000000000001);
        ssc_data_set_number(data, "batt_Qfull", 3.2000000000000002);
        ssc_data_set_number(data, "batt_Qfull_flow", 479616.00000000006);
        ssc_data_set_number(data, "batt_Qexp", 2.5840000000000005);
        ssc_data_set_number(data, "batt_Qnom", 3.1260000000000003);
        ssc_data_set_number(data, "batt_Vcut", 2.7719999999999998);
        ssc_data_set_number(data, "batt_C_rate", 0.20000000000000001);
        ssc_data_set_number(data, "batt_resistance", 0.001155);
        ssc_number_t p_batt_voltage_matrix[2] = { 0, 0 };
        ssc_data_set_matrix(data, "batt_voltage_matrix", p_batt_voltage_matrix, 1, 2);
        ssc_data_set_number(data, "LeadAcid_q20_computed", 479616);
        ssc_data_set_number(data, "LeadAcid_q10_computed", 446042.88);
        ssc_data_set_number(data, "LeadAcid_qn_computed", 287769.59999999998);
        ssc_data_set_number(data, "LeadAcid_tn", 1);
        ssc_data_set_number(data, "batt_initial_SOC", 50);
        ssc_data_set_number(data, "batt_minimum_SOC", 15);
        ssc_data_set_number(data, "batt_maximum_SOC", 95);
        ssc_data_set_number(data, "batt_minimum_modetime", 10);
        ssc_data_set_number(data, "batt_life_model", 1);
        ssc_number_t p_batt_lifetime_matrix[75] = { 10, 0, 100.85299999999999, 10, 1250, 94.884, 10, 2500, 88.914699999999996, 10, 3750, 82.945400000000006, 10, 5000, 76.976100000000002, 20, 0, 100.85299999999999, 20, 1250, 94.879800000000003, 20, 2500, 88.906300000000002, 20, 3750, 82.9328, 20, 5000, 76.959299999999999, 40, 0, 100.85299999999999, 40, 1250, 94.782200000000003, 40, 2500, 88.710999999999999, 40, 3750, 82.639700000000005, 40, 5000, 76.568200000000004, 80, 0, 100.85299999999999, 80, 1250, 92.483800000000002, 80, 2500, 84.054299999999998, 80, 3750, 75.560000000000002, 80, 5000, 66.995599999999996, 100, 0, 100.85299999999999, 100, 1250, 88.125600000000006, 100, 2500, 74.873199999999997, 100, 3750, 60.951099999999997, 100, 5000, 46.1312 };
        ssc_data_set_matrix(data, "batt_lifetime_matrix", p_batt_lifetime_matrix, 25, 3);
        ssc_data_set_number(data, "batt_calendar_choice", 1);
        ssc_number_t p_batt_calendar_lifetime_matrix[6] = { 0, 100, 3650, 80, 7300, 50 };
        ssc_data_set_matrix(data, "batt_calendar_lifetime_matrix", p_batt_calendar_lifetime_matrix, 3, 2);
        ssc_data_set_number(data, "batt_calendar_q0", 1.02);
        ssc_data_set_number(data, "batt_calendar_a", 0.00266);
        ssc_data_set_number(data, "batt_calendar_b", -7280);
        ssc_data_set_number(data, "batt_calendar_c", 939);
        ssc_data_set_number(data, "batt_replacement_capacity", 50);
        ssc_data_set_number(data, "batt_replacement_option", 1);
        ssc_number_t p_batt_replacement_schedule_percent[25] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        ssc_data_set_array(data, "batt_replacement_schedule_percent", p_batt_replacement_schedule_percent, 25);
        ssc_data_set_number(data, "batt_mass", 2376236.1029702974);
        ssc_data_set_number(data, "batt_surface_area", 17999.988480000004);
        ssc_data_set_number(data, "batt_Cp", 1500);
        ssc_data_set_number(data, "batt_h_to_ambient", 100);
        set_array(data, "batt_room_temperature_celsius", temperature_path, 8760);
        ssc_number_t p_cap_vs_temp[8] = { 0, 80.200000000000003, 23, 100, 30, 103.09999999999999, 45, 105.40000000000001 };
        ssc_data_set_matrix(data, "cap_vs_temp", p_cap_vs_temp, 4, 2);
        ssc_number_t p_dispatch_manual_charge[6] = { 0, 0, 0, 0, 0, 0 };
        ssc_data_set_array(data, "dispatch_manual_charge", p_dispatch_manual_charge, 6);
        ssc_number_t p_dispatch_manual_discharge[6] = { 0, 1, 0, 0, 0, 0 };
        ssc_data_set_array(data, "dispatch_manual_discharge", p_dispatch_manual_discharge, 6);
        ssc_number_t p_dispatch_manual_btm_discharge_to_grid[6] = { 0, 0, 0, 0, 0, 0 };
        ssc_data_set_array(data, "dispatch_manual_btm_discharge_to_grid", p_dispatch_manual_btm_discharge_to_grid, 6);
        ssc_number_t p_dispatch_manual_gridcharge[6] = { 1, 0, 0, 0, 0, 0 };
        ssc_data_set_array(data, "dispatch_manual_gridcharge", p_dispatch_manual_gridcharge, 6);
        ssc_number_t p_dispatch_manual_percent_discharge[2] = { 25, 0 };
        ssc_data_set_array(data, "dispatch_manual_percent_discharge", p_dispatch_manual_percent_discharge, 2);
        ssc_number_t p_dispatch_manual_percent_gridcharge[2] = { 25, 0 };
        ssc_data_set_array(data, "dispatch_manual_percent_gridcharge", p_dispatch_manual_percent_gridcharge, 2);
        ssc_number_t p_dispatch_manual_sched[288] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1 };
        ssc_data_set_matrix(data, "dispatch_manual_sched", p_dispatch_manual_sched, 12, 24);
        ssc_number_t p_dispatch_manual_sched_weekend[288] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1 };
        ssc_data_set_matrix(data, "dispatch_manual_sched_weekend", p_dispatch_manual_sched_weekend, 12, 24);
        ssc_data_set_number(data, "dispatch_manual_system_charge_first", 0);
        ssc_number_t p_batt_custom_dispatch[1] = { 0 };
        ssc_data_set_array(data, "batt_custom_dispatch", p_batt_custom_dispatch, 1);
        ssc_data_set_number(data, "batt_dispatch_choice", 0);
        ssc_data_set_number(data, "batt_dispatch_auto_can_gridcharge", 1);
        ssc_data_set_number(data, "batt_look_ahead_hours", 18);
        ssc_data_set_number(data, "batt_dispatch_update_frequency_hours", 1);
        ssc_data_set_number(data, "batt_dispatch_wf_forecast_choice", 0);
        ssc_number_t p_batt_pv_clipping_forecast[1] = { 0 };
        ssc_data_set_array(data, "batt_pv_clipping_forecast", p_batt_pv_clipping_forecast, 1);
        ssc_number_t p_batt_pv_ac_forecast[1] = { 0 };
        ssc_data_set_array(data, "batt_pv_ac_forecast", p_batt_pv_ac_forecast, 1);
        ssc_data_set_number(data, "batt_cycle_cost_choice", 0);
        ssc_number_t p_batt_cycle_cost[1] = { 0.10000000000000001 };
        ssc_data_set_array(data, "batt_cycle_cost", p_batt_cycle_cost, 1);
        ssc_data_set_number(data, "inflation_rate", 2.5);
        ssc_number_t p_om_batt_replacement_cost[1] = { 282 };
        ssc_data_set_array(data, "om_batt_replacement_cost", p_om_batt_replacement_cost, 1);
        ssc_data_set_number(data, "om_replacement_cost_escal", 0);
        ssc_number_t p_om_batt_variable_cost[1] = { 0 };
        ssc_data_set_array(data, "om_batt_variable_cost", p_om_batt_variable_cost, 1);
        ssc_data_set_number(data, "om_production_escal", 0);
        ssc_data_set_number(data, "forecast_price_signal_model", 1);
        ssc_data_set_number(data, "mp_enable_energy_market_revenue", 1);
        ssc_number_t p_mp_energy_market_revenue[2] = { 0, 0 };
        ssc_data_set_matrix(data, "mp_energy_market_revenue", p_mp_energy_market_revenue, 1, 2);
        ssc_data_set_number(data, "mp_enable_ancserv1", 0);
        ssc_number_t p_mp_ancserv1_revenue[2] = { 0, 0 };
        ssc_data_set_matrix(data, "mp_ancserv1_revenue", p_mp_ancserv1_revenue, 1, 2);
        ssc_data_set_number(data, "mp_enable_ancserv2", 0);
        ssc_number_t p_mp_ancserv2_revenue[2] = { 0, 0 };
        ssc_data_set_matrix(data, "mp_ancserv2_revenue", p_mp_ancserv2_revenue, 1, 2);
        ssc_data_set_number(data, "mp_enable_ancserv3", 0);
        ssc_number_t p_mp_ancserv3_revenue[2] = { 0, 0 };
        ssc_data_set_matrix(data, "mp_ancserv3_revenue", p_mp_ancserv3_revenue, 1, 2);
        ssc_data_set_number(data, "mp_enable_ancserv4", 0);
        ssc_number_t p_mp_ancserv4_revenue[2] = { 0, 0 };
        ssc_data_set_matrix(data, "mp_ancserv4_revenue", p_mp_ancserv4_revenue, 1, 2);
        set_matrix(data, "mp_energy_market_revenue_single", mp_path, 8760, 1);
        ssc_number_t p_mp_ancserv1_revenue_single[1] = { 0 };
        ssc_data_set_matrix(data, "mp_ancserv1_revenue_single", p_mp_ancserv1_revenue_single, 1, 1);
        ssc_number_t p_mp_ancserv2_revenue_single[1] = { 0 };
        ssc_data_set_matrix(data, "mp_ancserv2_revenue_single", p_mp_ancserv2_revenue_single, 1, 1);
        ssc_number_t p_mp_ancserv3_revenue_single[1] = { 0 };
        ssc_data_set_matrix(data, "mp_ancserv3_revenue_single", p_mp_ancserv3_revenue_single, 1, 1);
        ssc_number_t p_mp_ancserv4_revenue_single[1] = { 0 };
        ssc_data_set_matrix(data, "mp_ancserv4_revenue_single", p_mp_ancserv4_revenue_single, 1, 1);
        ssc_data_set_number(data, "mp_enable_market_percent_gen", 1);
        ssc_data_set_number(data, "mp_market_percent_gen", 100);
        ssc_data_set_number(data, "mp_enable_ancserv1_percent_gen", 0);
        ssc_data_set_number(data, "mp_enable_ancserv2_percent_gen", 0);
        ssc_data_set_number(data, "mp_enable_ancserv3_percent_gen", 0);
        ssc_data_set_number(data, "mp_enable_ancserv4_percent_gen", 0);
        ssc_data_set_number(data, "en_electricity_rates", 0);
        ssc_number_t p_rate_escalation[1] = { 0 };
        ssc_data_set_array(data, "rate_escalation", p_rate_escalation, 1);
        ssc_data_set_number(data, "ur_metering_option", 4);
        ssc_data_set_number(data, "ur_nm_yearend_sell_rate", 0);
        ssc_data_set_number(data, "ur_nm_credit_month", 0);
        ssc_data_set_number(data, "ur_nm_credit_rollover", 0);
        ssc_data_set_number(data, "ur_monthly_fixed_charge", 0);
        ssc_data_set_number(data, "ur_monthly_min_charge", 0);
        ssc_data_set_number(data, "ur_annual_min_charge", 0);
        ssc_data_set_number(data, "ur_en_ts_sell_rate", 0);
        ssc_number_t p_ur_ts_sell_rate[1] = { 0 };
        ssc_data_set_array(data, "ur_ts_sell_rate", p_ur_ts_sell_rate, 1);
        ssc_data_set_number(data, "ur_en_ts_buy_rate", 0);
        ssc_number_t p_ur_ts_buy_rate[1] = { 0 };
        ssc_data_set_array(data, "ur_ts_buy_rate", p_ur_ts_buy_rate, 1);
        ssc_number_t p_ur_ec_sched_weekday[288] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
        ssc_data_set_matrix(data, "ur_ec_sched_weekday", p_ur_ec_sched_weekday, 12, 24);
        ssc_number_t p_ur_ec_sched_weekend[288] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
        ssc_data_set_matrix(data, "ur_ec_sched_weekend", p_ur_ec_sched_weekend, 12, 24);
        ssc_number_t p_ur_ec_tou_mat[6] = { 1, 1, 9.9999999999999998e+37, 0, 0.044999999999999998, 0 };
        ssc_data_set_matrix(data, "ur_ec_tou_mat", p_ur_ec_tou_mat, 1, 6);
        ssc_data_set_number(data, "ur_dc_enable", 0);
        ssc_number_t p_ur_dc_sched_weekday[288] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
        ssc_data_set_matrix(data, "ur_dc_sched_weekday", p_ur_dc_sched_weekday, 12, 24);
        ssc_number_t p_ur_dc_sched_weekend[288] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
        ssc_data_set_matrix(data, "ur_dc_sched_weekend", p_ur_dc_sched_weekend, 12, 24);
        ssc_number_t p_ur_dc_tou_mat[4] = { 1, 1, 9.9999999999999998e+37, 0 };
        ssc_data_set_matrix(data, "ur_dc_tou_mat", p_ur_dc_tou_mat, 1, 4);
        ssc_number_t p_ur_dc_flat_mat[48] = { 0, 1, 9.9999999999999998e+37, 0, 1, 1, 9.9999999999999998e+37, 0, 2, 1, 9.9999999999999998e+37, 0, 3, 1, 9.9999999999999998e+37, 0, 4, 1, 9.9999999999999998e+37, 0, 5, 1, 9.9999999999999998e+37, 0, 6, 1, 9.9999999999999998e+37, 0, 7, 1, 9.9999999999999998e+37, 0, 8, 1, 9.9999999999999998e+37, 0, 9, 1, 9.9999999999999998e+37, 0, 10, 1, 9.9999999999999998e+37, 0, 11, 1, 9.9999999999999998e+37, 0 };
        ssc_data_set_matrix(data, "ur_dc_flat_mat", p_ur_dc_flat_mat, 12, 4);
        ssc_data_set_number(data, "ur_enable_billing_demand", 0);
        ssc_data_set_number(data, "ur_billing_demand_minimum", 100);
        ssc_data_set_number(data, "ur_billing_demand_lookback_period", 11);
        ssc_number_t p_ur_billing_demand_lookback_percentages[24] = { 60, 0, 60, 0, 60, 0, 60, 0, 60, 0, 95, 1, 95, 1, 95, 1, 95, 1, 60, 0, 60, 0, 60, 0 };
        ssc_data_set_matrix(data, "ur_billing_demand_lookback_percentages", p_ur_billing_demand_lookback_percentages, 12, 2);
        ssc_number_t p_ur_dc_billing_demand_periods[2] = { 1, 1 };
        ssc_data_set_matrix(data, "ur_dc_billing_demand_periods", p_ur_dc_billing_demand_periods, 1, 2);
        ssc_number_t p_ur_yearzero_usage_peaks[12] = { 234.67599999999999, 173.422, 172.00700000000001, 191.434, 198.29499999999999, 236.46899999999999, 274.23099999999999, 260.33600000000001, 226.751, 185.12299999999999, 156.19999999999999, 184.05000000000001 };
        ssc_data_set_array(data, "ur_yearzero_usage_peaks", p_ur_yearzero_usage_peaks, 12);
        set_array(data, "grid_curtailment", grid_path, 8760);
        ssc_data_set_number(data, "enable_interconnection_limit", 0);
        ssc_data_set_number(data, "grid_interconnection_limit_kwac", 100000);
    }
}
#endif
