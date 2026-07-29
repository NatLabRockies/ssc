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


#ifndef _ETES_ETES_DEFAULTS_H_
#define _ETES_ETES_DEFAULTS_H_

#include "../input_cases/code_generator_utilities.h"

char etes_dispatch_factors_path[512];
int etes1 = sprintf(etes_dispatch_factors_path, "%s/test/input_cases/etes_etes_data/dispatch_factors_ts.csv", std::getenv("SSCDIR"));


ssc_data_t etes_etes_defaults()
{
    ssc_data_t data = ssc_data_create();

    set_array(data, "dispatch_factors_ts", etes_dispatch_factors_path, 8760);

    char solar_resource_path[512];
    // This is a copy of the actual weather file used, which has been copied to the ssc repo so it can be found by Travis CI for its tests.
    //  The actual weather file used by SAM could change and thus change the UI output values (different input (i.e., weather file) -> different outputs)
    int n1 = sprintf(solar_resource_path, "%s/test/input_cases/moltensalt_data/daggett_ca_34.865371_-116.783023_psmv3_60_tmy.csv", std::getenv("SSCDIR"));

    ssc_data_set_string(data, "solar_resource_file", solar_resource_path);
    ssc_data_set_number(data, "is_dispatch", 1);
    ssc_data_set_number(data, "etes_financial_model", 1);
    ssc_data_set_number(data, "T_htf_cold_des", 290);
    ssc_data_set_number(data, "T_htf_hot_des", 574);
    ssc_data_set_number(data, "P_ref", 115);
    ssc_data_set_number(data, "design_eff", 0.41199999999999998);
    ssc_data_set_number(data, "tshours", 10);
    ssc_data_set_number(data, "heater_mult", 2.3999999999999999);
    ssc_data_set_number(data, "pc_config", 0);
    ssc_data_set_number(data, "pb_pump_coef", 0.55000000000000004);
    ssc_data_set_number(data, "startup_time", 0.5);
    ssc_data_set_number(data, "startup_frac", 0.5);
    ssc_data_set_number(data, "cycle_max_frac", 1.05);
    ssc_data_set_number(data, "cycle_cutoff_frac", 0.20000000000000001);
    ssc_data_set_number(data, "q_sby_frac", 0.20000000000000001);
    ssc_data_set_number(data, "dT_cw_ref", 10);
    ssc_data_set_number(data, "T_amb_des", 42);
    ssc_data_set_number(data, "CT", 2);
    ssc_data_set_number(data, "T_approach", 5);
    ssc_data_set_number(data, "T_ITD_des", 16);
    ssc_data_set_number(data, "P_cond_ratio", 1.0027999999999999);
    ssc_data_set_number(data, "pb_bd_frac", 0.02);
    ssc_data_set_number(data, "P_cond_min", 2);
    ssc_data_set_number(data, "n_pl_inc", 8);
    ssc_data_set_number(data, "tech_type", 1);
    ssc_data_set_number(data, "ud_f_W_dot_cool_des", 0);
    ssc_data_set_number(data, "ud_m_dot_water_cool_des", 0);
    ssc_data_set_number(data, "hot_htf_code", 17);
    ssc_data_set_number(data, "tes_init_hot_htf_percent", 0);
    ssc_data_set_number(data, "h_tank", 12);
    ssc_data_set_number(data, "cold_tank_max_heat", 15);
    ssc_data_set_number(data, "u_tank", 0.40000000000000002);
    ssc_data_set_number(data, "tank_pairs", 1);
    ssc_data_set_number(data, "cold_tank_Thtr", 280);
    ssc_data_set_number(data, "h_tank_min", 1);
    ssc_data_set_number(data, "hot_tank_Thtr", 500);
    ssc_data_set_number(data, "hot_tank_max_heat", 30);
    ssc_data_set_number(data, "heater_efficiency", 100);
    ssc_data_set_number(data, "f_q_dot_des_allowable_su", 1);
    ssc_data_set_number(data, "hrs_startup_at_max_rate", 0.25);
    ssc_data_set_number(data, "f_q_dot_heater_min", 0.25);
    ssc_data_set_number(data, "disp_horizon", 48);
    ssc_data_set_number(data, "disp_frequency", 24);
    ssc_data_set_number(data, "disp_max_iter", 100000);
    ssc_data_set_number(data, "disp_timeout", 5);
    ssc_data_set_number(data, "disp_mip_gap", 0.001);
    ssc_data_set_number(data, "disp_pen_delta_w", 1);
    ssc_data_set_number(data, "disp_csu_cost", 87);
    ssc_data_set_number(data, "disp_hsu_cost", 0.014999999999999999);
    ssc_data_set_number(data, "disp_time_weighting", 0.999);
    ssc_data_set_number(data, "disp_down_time_min", 2);
    ssc_data_set_number(data, "disp_up_time_min", 2);
    ssc_data_set_number(data, "pb_fixed_par", 0.0054999999999999997);
    ssc_data_set_number(data, "bop_par", 0);
    ssc_data_set_number(data, "bop_par_f", 1);
    ssc_data_set_number(data, "bop_par_0", 0);
    ssc_data_set_number(data, "bop_par_1", 0.48299999999999998);
    ssc_data_set_number(data, "bop_par_2", 0);
    ssc_data_set_number(data, "ppa_multiplier_model", 1);
    ssc_number_t p_dispatch_tod_factors[9] = { 0.25, 1.75, 1, 1, 1, 1, 1, 1, 1 };
    ssc_data_set_array(data, "dispatch_tod_factors", p_dispatch_tod_factors, 9);
    ssc_number_t p_ppa[1] = { 0.074999999999999997 };
    ssc_data_set_array(data, "ppa_price_input", p_ppa, 1);
    ssc_data_set_number(data, "cycle_spec_cost", 1040);
    ssc_data_set_number(data, "tes_spec_cost", 22);
    ssc_data_set_number(data, "heater_spec_cost", 104);
    ssc_data_set_number(data, "bop_spec_cost", 0);
    ssc_data_set_number(data, "contingency_rate", 7);
    ssc_data_set_number(data, "sales_tax_frac", 80);
    ssc_data_set_number(data, "epc_cost_perc_of_direct", 13);
    ssc_data_set_number(data, "epc_cost_per_watt", 0);
    ssc_data_set_number(data, "epc_cost_fixed", 0);
    ssc_data_set_number(data, "land_cost_perc_of_direct", 0);
    ssc_data_set_number(data, "land_cost_per_watt", 0);
    ssc_data_set_number(data, "land_cost_fixed", 0);
    ssc_data_set_number(data, "sales_tax_rate", 5);
    ssc_data_set_number(data, "const_per_interest_rate1", 3.5);
    ssc_data_set_number(data, "const_per_interest_rate2", 0);
    ssc_data_set_number(data, "const_per_interest_rate3", 0);
    ssc_data_set_number(data, "const_per_interest_rate4", 0);
    ssc_data_set_number(data, "const_per_interest_rate5", 0);
    ssc_data_set_number(data, "const_per_months1", 24);
    ssc_data_set_number(data, "const_per_months2", 0);
    ssc_data_set_number(data, "const_per_months3", 0);
    ssc_data_set_number(data, "const_per_months4", 0);
    ssc_data_set_number(data, "const_per_months5", 0);
    ssc_data_set_number(data, "const_per_percent1", 100);
    ssc_data_set_number(data, "const_per_percent2", 0);
    ssc_data_set_number(data, "const_per_percent3", 0);
    ssc_data_set_number(data, "const_per_percent4", 0);
    ssc_data_set_number(data, "const_per_percent5", 0);
    ssc_data_set_number(data, "const_per_upfront_rate1", 1);
    ssc_data_set_number(data, "const_per_upfront_rate2", 0);
    ssc_data_set_number(data, "const_per_upfront_rate3", 0);
    ssc_data_set_number(data, "const_per_upfront_rate4", 0);
    ssc_data_set_number(data, "const_per_upfront_rate5", 0);

    ssc_data_set_number(data, "adjust_constant", 0.0);

    ssc_data_set_number(data, "ppa_soln_mode", 1);

    /*
    ssc_data_set_number(data, "ppa_escalation", 1);
    ssc_data_set_number(data, "analysis_period", 25);
    ssc_data_set_number(data, "federal_tax_rate", [21]);
    ssc_data_set_number(data, "state_tax_rate", [7]);
    ssc_data_set_number(data, "property_tax_rate", 0);
    ssc_data_set_number(data, "prop_tax_cost_assessed_percent", 100);
    ssc_data_set_number(data, "prop_tax_assessed_decline", 0);
    ssc_data_set_number(data, "real_discount_rate", 6.4000000000000004);
    ssc_data_set_number(data, "inflation_rate", 2.5);
    ssc_data_set_number(data, "insurance_rate", 0.5);
    ssc_data_set_number(data, "system_capacity", 110665.73537064472);
    ssc_data_set_number(data, "om_fixed", [0]);
    ssc_data_set_number(data, "om_fixed_escal", 0);
    ssc_data_set_number(data, "om_production", [3.5]);
    ssc_data_set_number(data, "om_production_escal", 0);
    ssc_data_set_number(data, "om_capacity", [66]);
    ssc_data_set_number(data, "om_capacity_escal", 0);
    ssc_data_set_number(data, "reserves_interest", 1.25);
    ssc_data_set_number(data, "equip1_reserve_cost", 0);
    ssc_data_set_number(data, "equip1_reserve_freq", 12);
    ssc_data_set_number(data, "equip2_reserve_cost", 0);
    ssc_data_set_number(data, "equip2_reserve_freq", 15);
    ssc_data_set_number(data, "equip3_reserve_cost", 0);
    ssc_data_set_number(data, "equip3_reserve_freq", 3);
    ssc_data_set_number(data, "equip_reserve_depr_sta", 0);
    ssc_data_set_number(data, "equip_reserve_depr_fed", 0);
    ssc_data_set_number(data, "itc_fed_amount", [0]);
    ssc_data_set_number(data, "itc_fed_amount_deprbas_fed", 1);
    ssc_data_set_number(data, "itc_fed_amount_deprbas_sta", 1);
    ssc_data_set_number(data, "itc_sta_amount", [0]);
    ssc_data_set_number(data, "itc_sta_amount_deprbas_fed", 0);
    ssc_data_set_number(data, "itc_sta_amount_deprbas_sta", 0);
    ssc_data_set_number(data, "itc_fed_percent", [26]);
    ssc_data_set_number(data, "itc_fed_percent_maxvalue", [9.9999999999999998e+37]);
    ssc_data_set_number(data, "itc_fed_percent_deprbas_fed", 1);
    ssc_data_set_number(data, "itc_fed_percent_deprbas_sta", 1);
    ssc_data_set_number(data, "itc_sta_percent", [0]);
    ssc_data_set_number(data, "itc_sta_percent_maxvalue", [9.9999999999999998e+37]);
    ssc_data_set_number(data, "itc_sta_percent_deprbas_fed", 0);
    ssc_data_set_number(data, "itc_sta_percent_deprbas_sta", 0);
    ssc_data_set_number(data, "ptc_fed_amount", [0]);
    ssc_data_set_number(data, "ptc_fed_term", 10);
    ssc_data_set_number(data, "ptc_fed_escal", 0);
    ssc_data_set_number(data, "ptc_sta_amount", [0]);
    ssc_data_set_number(data, "ptc_sta_term", 10);
    ssc_data_set_number(data, "ptc_sta_escal", 0);
    ssc_data_set_number(data, "depr_alloc_macrs_5_percent", 90);
    ssc_data_set_number(data, "depr_alloc_macrs_15_percent", 1.5);
    ssc_data_set_number(data, "depr_alloc_sl_5_percent", 0);
    ssc_data_set_number(data, "depr_alloc_sl_15_percent", 2.5);
    ssc_data_set_number(data, "depr_alloc_sl_20_percent", 3);
    ssc_data_set_number(data, "depr_alloc_sl_39_percent", 0);
    ssc_data_set_number(data, "depr_alloc_custom_percent", 0);
    ssc_data_set_number(data, "depr_custom_schedule", [0]);
    ssc_data_set_number(data, "depr_bonus_sta", 0);
    ssc_data_set_number(data, "depr_bonus_sta_macrs_5", 1);
    ssc_data_set_number(data, "depr_bonus_sta_macrs_15", 1);
    ssc_data_set_number(data, "depr_bonus_sta_sl_5", 0);
    ssc_data_set_number(data, "depr_bonus_sta_sl_15", 0);
    ssc_data_set_number(data, "depr_bonus_sta_sl_20", 0);
    ssc_data_set_number(data, "depr_bonus_sta_sl_39", 0);
    ssc_data_set_number(data, "depr_bonus_sta_custom", 0);
    ssc_data_set_number(data, "depr_bonus_fed", 0);
    ssc_data_set_number(data, "depr_bonus_fed_macrs_5", 1);
    ssc_data_set_number(data, "depr_bonus_fed_macrs_15", 1);
    ssc_data_set_number(data, "depr_bonus_fed_sl_5", 0);
    ssc_data_set_number(data, "depr_bonus_fed_sl_15", 0);
    ssc_data_set_number(data, "depr_bonus_fed_sl_20", 0);
    ssc_data_set_number(data, "depr_bonus_fed_sl_39", 0);
    ssc_data_set_number(data, "depr_bonus_fed_custom", 0);
    ssc_data_set_number(data, "depr_itc_sta_macrs_5", 1);
    ssc_data_set_number(data, "depr_itc_sta_macrs_15", 0);
    ssc_data_set_number(data, "depr_itc_sta_sl_5", 0);
    ssc_data_set_number(data, "depr_itc_sta_sl_15", 0);
    ssc_data_set_number(data, "depr_itc_sta_sl_20", 0);
    ssc_data_set_number(data, "depr_itc_sta_sl_39", 0);
    ssc_data_set_number(data, "depr_itc_sta_custom", 0);
    ssc_data_set_number(data, "depr_itc_fed_macrs_5", 1);
    ssc_data_set_number(data, "depr_itc_fed_macrs_15", 0);
    ssc_data_set_number(data, "depr_itc_fed_sl_5", 0);
    ssc_data_set_number(data, "depr_itc_fed_sl_15", 0);
    ssc_data_set_number(data, "depr_itc_fed_sl_20", 0);
    ssc_data_set_number(data, "depr_itc_fed_sl_39", 0);
    ssc_data_set_number(data, "depr_itc_fed_custom", 0);
    ssc_data_set_number(data, "ibi_fed_amount", 0);
    ssc_data_set_number(data, "ibi_fed_amount_tax_fed", 1);
    ssc_data_set_number(data, "ibi_fed_amount_tax_sta", 1);
    ssc_data_set_number(data, "ibi_fed_amount_deprbas_fed", 0);
    ssc_data_set_number(data, "ibi_fed_amount_deprbas_sta", 0);
    ssc_data_set_number(data, "ibi_sta_amount", 0);
    ssc_data_set_number(data, "ibi_sta_amount_tax_fed", 1);
    ssc_data_set_number(data, "ibi_sta_amount_tax_sta", 1);
    ssc_data_set_number(data, "ibi_sta_amount_deprbas_fed", 0);
    ssc_data_set_number(data, "ibi_sta_amount_deprbas_sta", 0);
    ssc_data_set_number(data, "ibi_uti_amount", 0);
    ssc_data_set_number(data, "ibi_uti_amount_tax_fed", 1);
    ssc_data_set_number(data, "ibi_uti_amount_tax_sta", 1);
    ssc_data_set_number(data, "ibi_uti_amount_deprbas_fed", 0);
    ssc_data_set_number(data, "ibi_uti_amount_deprbas_sta", 0);
    ssc_data_set_number(data, "ibi_oth_amount", 0);
    ssc_data_set_number(data, "ibi_oth_amount_tax_fed", 1);
    ssc_data_set_number(data, "ibi_oth_amount_tax_sta", 1);
    ssc_data_set_number(data, "ibi_oth_amount_deprbas_fed", 0);
    ssc_data_set_number(data, "ibi_oth_amount_deprbas_sta", 0);
    ssc_data_set_number(data, "ibi_fed_percent", 0);
    ssc_data_set_number(data, "ibi_fed_percent_maxvalue", 9.9999999999999998e+37);
    ssc_data_set_number(data, "ibi_fed_percent_tax_fed", 1);
    ssc_data_set_number(data, "ibi_fed_percent_tax_sta", 1);
    ssc_data_set_number(data, "ibi_fed_percent_deprbas_fed", 0);
    ssc_data_set_number(data, "ibi_fed_percent_deprbas_sta", 0);
    ssc_data_set_number(data, "ibi_sta_percent", 0);
    ssc_data_set_number(data, "ibi_sta_percent_maxvalue", 9.9999999999999998e+37);
    ssc_data_set_number(data, "ibi_sta_percent_tax_fed", 1);
    ssc_data_set_number(data, "ibi_sta_percent_tax_sta", 1);
    ssc_data_set_number(data, "ibi_sta_percent_deprbas_fed", 0);
    ssc_data_set_number(data, "ibi_sta_percent_deprbas_sta", 0);
    ssc_data_set_number(data, "ibi_uti_percent", 0);
    ssc_data_set_number(data, "ibi_uti_percent_maxvalue", 9.9999999999999998e+37);
    ssc_data_set_number(data, "ibi_uti_percent_tax_fed", 1);
    ssc_data_set_number(data, "ibi_uti_percent_tax_sta", 1);
    ssc_data_set_number(data, "ibi_uti_percent_deprbas_fed", 0);
    ssc_data_set_number(data, "ibi_uti_percent_deprbas_sta", 0);
    ssc_data_set_number(data, "ibi_oth_percent", 0);
    ssc_data_set_number(data, "ibi_oth_percent_maxvalue", 9.9999999999999998e+37);
    ssc_data_set_number(data, "ibi_oth_percent_tax_fed", 1);
    ssc_data_set_number(data, "ibi_oth_percent_tax_sta", 1);
    ssc_data_set_number(data, "ibi_oth_percent_deprbas_fed", 0);
    ssc_data_set_number(data, "ibi_oth_percent_deprbas_sta", 0);
    ssc_data_set_number(data, "cbi_fed_amount", 0);
    ssc_data_set_number(data, "cbi_fed_maxvalue", 9.9999999999999998e+37);
    ssc_data_set_number(data, "cbi_fed_tax_fed", 1);
    ssc_data_set_number(data, "cbi_fed_tax_sta", 1);
    ssc_data_set_number(data, "cbi_fed_deprbas_fed", 0);
    ssc_data_set_number(data, "cbi_fed_deprbas_sta", 0);
    ssc_data_set_number(data, "cbi_sta_amount", 0);
    ssc_data_set_number(data, "cbi_sta_maxvalue", 9.9999999999999998e+37);
    ssc_data_set_number(data, "cbi_sta_tax_fed", 1);
    ssc_data_set_number(data, "cbi_sta_tax_sta", 1);
    ssc_data_set_number(data, "cbi_sta_deprbas_fed", 0);
    ssc_data_set_number(data, "cbi_sta_deprbas_sta", 0);
    ssc_data_set_number(data, "cbi_uti_amount", 0);
    ssc_data_set_number(data, "cbi_uti_maxvalue", 9.9999999999999998e+37);
    ssc_data_set_number(data, "cbi_uti_tax_fed", 1);
    ssc_data_set_number(data, "cbi_uti_tax_sta", 1);
    ssc_data_set_number(data, "cbi_uti_deprbas_fed", 0);
    ssc_data_set_number(data, "cbi_uti_deprbas_sta", 0);
    ssc_data_set_number(data, "cbi_oth_amount", 0);
    ssc_data_set_number(data, "cbi_oth_maxvalue", 9.9999999999999998e+37);
    ssc_data_set_number(data, "cbi_oth_tax_fed", 1);
    ssc_data_set_number(data, "cbi_oth_tax_sta", 1);
    ssc_data_set_number(data, "cbi_oth_deprbas_fed", 0);
    ssc_data_set_number(data, "cbi_oth_deprbas_sta", 0);
    ssc_data_set_number(data, "pbi_fed_amount", [0]);
    ssc_data_set_number(data, "pbi_fed_term", 0);
    ssc_data_set_number(data, "pbi_fed_escal", 0);
    ssc_data_set_number(data, "pbi_fed_tax_fed", 1);
    ssc_data_set_number(data, "pbi_fed_tax_sta", 1);
    ssc_data_set_number(data, "pbi_sta_amount", [0]);
    ssc_data_set_number(data, "pbi_sta_term", 0);
    ssc_data_set_number(data, "pbi_sta_escal", 0);
    ssc_data_set_number(data, "pbi_sta_tax_fed", 1);
    ssc_data_set_number(data, "pbi_sta_tax_sta", 1);
    ssc_data_set_number(data, "pbi_uti_amount", [0]);
    ssc_data_set_number(data, "pbi_uti_term", 0);
    ssc_data_set_number(data, "pbi_uti_escal", 0);
    ssc_data_set_number(data, "pbi_uti_tax_fed", 1);
    ssc_data_set_number(data, "pbi_uti_tax_sta", 1);
    ssc_data_set_number(data, "pbi_oth_amount", [0]);
    ssc_data_set_number(data, "pbi_oth_term", 0);
    ssc_data_set_number(data, "pbi_oth_escal", 0);
    ssc_data_set_number(data, "pbi_oth_tax_fed", 1);
    ssc_data_set_number(data, "pbi_oth_tax_sta", 1);
    ssc_data_set_number(data, "term_tenor", 18);
    ssc_data_set_number(data, "term_int_rate", 4);
    ssc_data_set_number(data, "dscr", 1.3);
    ssc_data_set_number(data, "dscr_limit_debt_fraction", 1);
    ssc_data_set_number(data, "dscr_maximum_debt_fraction", 60);
    ssc_data_set_number(data, "dscr_reserve_months", 6);
    ssc_data_set_number(data, "debt_percent", 50);
    ssc_data_set_number(data, "debt_option", 1);
    ssc_data_set_number(data, "payment_option", 0);
    ssc_data_set_number(data, "cost_debt_closing", 450000);
    ssc_data_set_number(data, "cost_debt_fee", 2.75);
    ssc_data_set_number(data, "months_working_reserve", 6);
    ssc_data_set_number(data, "months_receivables_reserve", 0);
    ssc_data_set_number(data, "cost_other_financing", 0);
    ssc_data_set_number(data, "flip_target_year", 20);
    ssc_data_set_number(data, "pbi_fed_for_ds", 0);
    ssc_data_set_number(data, "pbi_sta_for_ds", 0);
    ssc_data_set_number(data, "pbi_uti_for_ds", 0);
    ssc_data_set_number(data, "pbi_oth_for_ds", 0);
    ssc_data_set_number(data, "degradation", [0]);
    ssc_data_set_number(data, "loan_moratorium", 0);
    ssc_data_set_number(data, "system_use_recapitalization", 0);
    ssc_data_set_number(data, "system_use_lifetime_output", 0);
    ssc_data_set_number(data, "total_installed_cost", 313823374.95145631);
    ssc_data_set_number(data, "salvage_percentage", 0);
    ssc_data_set_number(data, "construction_financing_cost", 14122051.872815534);
    ssc_data_set_number(data, "depr_stabas_method", 1);
    ssc_data_set_number(data, "depr_fedbas_method", 1);
    ssc_data_set_number(data, "cp_capacity_payment_esc", 0);
    ssc_data_set_number(data, "cp_capacity_payment_type", 0);
    ssc_data_set_number(data, "cp_capacity_payment_amount", [150000]);
    ssc_data_set_number(data, "cp_capacity_credit_percent", [100]);
    ssc_data_set_number(data, "cp_system_nameplate", 110.66573537064471);
    ssc_data_set_number(data, "cp_battery_nameplate", 110.66573537064471);
    ssc_data_set_number(data, "batt_salvage_percentage", 0);
    */

    return data;
}






#endif
