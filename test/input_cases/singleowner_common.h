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


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sscapi.h"

#include "code_generator_utilities.h"

int singleowner_common(ssc_data_t &data)
{
    double energy[8760] = {1};
    ssc_data_set_array( data, "gen", energy, 8760);
	ssc_data_set_number( data, "system_use_lifetime_output", 0 );
    ssc_data_set_number( data, "system_capacity", 1 );
	ssc_data_set_number( data, "analysis_period", 25 );
    ssc_number_t p_dispatch_sched_weekday[288] ={ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
    ssc_data_set_matrix( data, "dispatch_sched_weekday", p_dispatch_sched_weekday, 12, 24 );
    ssc_number_t p_dispatch_sched_weekend[288] ={ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
    ssc_data_set_matrix( data, "dispatch_sched_weekend", p_dispatch_sched_weekend, 12, 24 );

    ssc_number_t p_federal_tax_rate[1] ={ 21 };
	ssc_data_set_array( data, "federal_tax_rate", p_federal_tax_rate, 1 );
	ssc_number_t p_state_tax_rate[1] ={ 7 };
	ssc_data_set_array( data, "state_tax_rate", p_state_tax_rate, 1 );
	ssc_data_set_number( data, "property_tax_rate", 0 );
	ssc_data_set_number( data, "prop_tax_cost_assessed_percent", 100 );
	ssc_data_set_number( data, "prop_tax_assessed_decline", 0 );
	ssc_data_set_number( data, "real_discount_rate", 6.4000000953674316 );
	ssc_data_set_number( data, "inflation_rate", 2.5 );
	ssc_data_set_number( data, "insurance_rate", 0.5 );
	ssc_number_t p_om_fixed[1] ={ 0 };
	ssc_data_set_array( data, "om_fixed", p_om_fixed, 1 );
	ssc_data_set_number( data, "om_fixed_escal", 0 );
	ssc_number_t p_om_production[1] ={ 0 };
	ssc_data_set_array( data, "om_production", p_om_production, 1 );
	ssc_data_set_number( data, "om_production_escal", 0 );
	ssc_number_t p_om_capacity[1] ={ 25 };
	ssc_data_set_array( data, "om_capacity", p_om_capacity, 1 );
	ssc_data_set_number( data, "om_capacity_escal", 0 );
	ssc_number_t p_om_fuel_cost[1] ={ 8 };
	ssc_data_set_array( data, "om_fuel_cost", p_om_fuel_cost, 1 );
	ssc_data_set_number( data, "om_fuel_cost_escal", 0 );
    ssc_number_t itc_amount[1] = { 0 };
    ssc_number_t itc_amount_max[1] = { 1e+38 };
    ssc_data_set_array(data, "itc_fed_amount", itc_amount, 1);
    ssc_data_set_array(data, "itc_sta_amount", itc_amount, 1);
    ssc_data_set_array(data, "itc_fed_percent", itc_amount, 1);
    ssc_data_set_array(data, "itc_sta_percent", itc_amount, 1);
    ssc_data_set_array(data, "itc_fed_percent_maxvalue", itc_amount_max, 1);
    ssc_data_set_array(data, "itc_sta_percent_maxvalue", itc_amount_max, 1);
    ssc_data_set_number( data, "itc_fed_amount_deprbas_fed", 1 );
	ssc_data_set_number( data, "itc_fed_amount_deprbas_sta", 1 );
	ssc_data_set_number( data, "itc_sta_amount_deprbas_fed", 0 );
	ssc_data_set_number( data, "itc_sta_amount_deprbas_sta", 0 );
	ssc_data_set_number( data, "itc_fed_percent_deprbas_fed", 1 );
	ssc_data_set_number( data, "itc_fed_percent_deprbas_sta", 1 );
	ssc_data_set_number( data, "itc_sta_percent_deprbas_fed", 0 );
	ssc_data_set_number( data, "itc_sta_percent_deprbas_sta", 0 );
	ssc_number_t p_ptc_fed_amount[1] ={ 0 };
	ssc_data_set_array( data, "ptc_fed_amount", p_ptc_fed_amount, 1 );
	ssc_data_set_number( data, "ptc_fed_term", 10 );
	ssc_data_set_number( data, "ptc_fed_escal", 0 );
	ssc_number_t p_ptc_sta_amount[1] ={ 0 };
	ssc_data_set_array( data, "ptc_sta_amount", p_ptc_sta_amount, 1 );
	ssc_data_set_number( data, "ptc_sta_term", 10 );
	ssc_data_set_number( data, "ptc_sta_escal", 0 );
	ssc_data_set_number( data, "ibi_fed_amount", 0 );
	ssc_data_set_number( data, "ibi_fed_amount_tax_fed", 1 );
	ssc_data_set_number( data, "ibi_fed_amount_tax_sta", 1 );
	ssc_data_set_number( data, "ibi_fed_amount_deprbas_fed", 0 );
	ssc_data_set_number( data, "ibi_fed_amount_deprbas_sta", 0 );
	ssc_data_set_number( data, "ibi_sta_amount", 0 );
	ssc_data_set_number( data, "ibi_sta_amount_tax_fed", 1 );
	ssc_data_set_number( data, "ibi_sta_amount_tax_sta", 1 );
	ssc_data_set_number( data, "ibi_sta_amount_deprbas_fed", 0 );
	ssc_data_set_number( data, "ibi_sta_amount_deprbas_sta", 0 );
	ssc_data_set_number( data, "ibi_uti_amount", 0 );
	ssc_data_set_number( data, "ibi_uti_amount_tax_fed", 1 );
	ssc_data_set_number( data, "ibi_uti_amount_tax_sta", 1 );
	ssc_data_set_number( data, "ibi_uti_amount_deprbas_fed", 0 );
	ssc_data_set_number( data, "ibi_uti_amount_deprbas_sta", 0 );
	ssc_data_set_number( data, "ibi_oth_amount", 0 );
	ssc_data_set_number( data, "ibi_oth_amount_tax_fed", 1 );
	ssc_data_set_number( data, "ibi_oth_amount_tax_sta", 1 );
	ssc_data_set_number( data, "ibi_oth_amount_deprbas_fed", 0 );
	ssc_data_set_number( data, "ibi_oth_amount_deprbas_sta", 0 );
	ssc_data_set_number( data, "ibi_fed_percent", 0 );
	ssc_data_set_number( data, "ibi_fed_percent_maxvalue", 9.9999996802856925e+37 );
	ssc_data_set_number( data, "ibi_fed_percent_tax_fed", 1 );
	ssc_data_set_number( data, "ibi_fed_percent_tax_sta", 1 );
	ssc_data_set_number( data, "ibi_fed_percent_deprbas_fed", 0 );
	ssc_data_set_number( data, "ibi_fed_percent_deprbas_sta", 0 );
	ssc_data_set_number( data, "ibi_sta_percent", 0 );
	ssc_data_set_number( data, "ibi_sta_percent_maxvalue", 9.9999996802856925e+37 );
	ssc_data_set_number( data, "ibi_sta_percent_tax_fed", 1 );
	ssc_data_set_number( data, "ibi_sta_percent_tax_sta", 1 );
	ssc_data_set_number( data, "ibi_sta_percent_deprbas_fed", 0 );
	ssc_data_set_number( data, "ibi_sta_percent_deprbas_sta", 0 );
	ssc_data_set_number( data, "ibi_uti_percent", 0 );
	ssc_data_set_number( data, "ibi_uti_percent_maxvalue", 9.9999996802856925e+37 );
	ssc_data_set_number( data, "ibi_uti_percent_tax_fed", 1 );
	ssc_data_set_number( data, "ibi_uti_percent_tax_sta", 1 );
	ssc_data_set_number( data, "ibi_uti_percent_deprbas_fed", 0 );
	ssc_data_set_number( data, "ibi_uti_percent_deprbas_sta", 0 );
	ssc_data_set_number( data, "ibi_oth_percent", 0 );
	ssc_data_set_number( data, "ibi_oth_percent_maxvalue", 9.9999996802856925e+37 );
	ssc_data_set_number( data, "ibi_oth_percent_tax_fed", 1 );
	ssc_data_set_number( data, "ibi_oth_percent_tax_sta", 1 );
	ssc_data_set_number( data, "ibi_oth_percent_deprbas_fed", 0 );
	ssc_data_set_number( data, "ibi_oth_percent_deprbas_sta", 0 );
	ssc_data_set_number( data, "cbi_fed_amount", 0 );
	ssc_data_set_number( data, "cbi_fed_maxvalue", 9.9999996802856925e+37 );
	ssc_data_set_number( data, "cbi_fed_tax_fed", 1 );
	ssc_data_set_number( data, "cbi_fed_tax_sta", 1 );
	ssc_data_set_number( data, "cbi_fed_deprbas_fed", 0 );
	ssc_data_set_number( data, "cbi_fed_deprbas_sta", 0 );
	ssc_data_set_number( data, "cbi_sta_amount", 0 );
	ssc_data_set_number( data, "cbi_sta_maxvalue", 9.9999996802856925e+37 );
	ssc_data_set_number( data, "cbi_sta_tax_fed", 1 );
	ssc_data_set_number( data, "cbi_sta_tax_sta", 1 );
	ssc_data_set_number( data, "cbi_sta_deprbas_fed", 0 );
	ssc_data_set_number( data, "cbi_sta_deprbas_sta", 0 );
	ssc_data_set_number( data, "cbi_uti_amount", 0 );
	ssc_data_set_number( data, "cbi_uti_maxvalue", 9.9999996802856925e+37 );
	ssc_data_set_number( data, "cbi_uti_tax_fed", 1 );
	ssc_data_set_number( data, "cbi_uti_tax_sta", 1 );
	ssc_data_set_number( data, "cbi_uti_deprbas_fed", 0 );
	ssc_data_set_number( data, "cbi_uti_deprbas_sta", 0 );
	ssc_data_set_number( data, "cbi_oth_amount", 0 );
	ssc_data_set_number( data, "cbi_oth_maxvalue", 9.9999996802856925e+37 );
	ssc_data_set_number( data, "cbi_oth_tax_fed", 1 );
	ssc_data_set_number( data, "cbi_oth_tax_sta", 1 );
	ssc_data_set_number( data, "cbi_oth_deprbas_fed", 0 );
	ssc_data_set_number( data, "cbi_oth_deprbas_sta", 0 );
	ssc_number_t p_pbi_fed_amount[1] ={ 0 };
	ssc_data_set_array( data, "pbi_fed_amount", p_pbi_fed_amount, 1 );
	ssc_data_set_number( data, "pbi_fed_term", 0 );
	ssc_data_set_number( data, "pbi_fed_escal", 0 );
	ssc_data_set_number( data, "pbi_fed_tax_fed", 1 );
	ssc_data_set_number( data, "pbi_fed_tax_sta", 1 );
	ssc_number_t p_pbi_sta_amount[1] ={ 0 };
	ssc_data_set_array( data, "pbi_sta_amount", p_pbi_sta_amount, 1 );
	ssc_data_set_number( data, "pbi_sta_term", 0 );
	ssc_data_set_number( data, "pbi_sta_escal", 0 );
	ssc_data_set_number( data, "pbi_sta_tax_fed", 1 );
	ssc_data_set_number( data, "pbi_sta_tax_sta", 1 );
	ssc_number_t p_pbi_uti_amount[1] ={ 0 };
	ssc_data_set_array( data, "pbi_uti_amount", p_pbi_uti_amount, 1 );
	ssc_data_set_number( data, "pbi_uti_term", 0 );
	ssc_data_set_number( data, "pbi_uti_escal", 0 );
	ssc_data_set_number( data, "pbi_uti_tax_fed", 1 );
	ssc_data_set_number( data, "pbi_uti_tax_sta", 1 );
	ssc_number_t p_pbi_oth_amount[1] ={ 0 };
	ssc_data_set_array( data, "pbi_oth_amount", p_pbi_oth_amount, 1 );
	ssc_data_set_number( data, "pbi_oth_term", 0 );
	ssc_data_set_number( data, "pbi_oth_escal", 0 );
	ssc_data_set_number( data, "pbi_oth_tax_fed", 1 );
	ssc_data_set_number( data, "pbi_oth_tax_sta", 1 );
	ssc_number_t p_degradation[1] ={ 0 };
	ssc_data_set_array( data, "degradation", p_degradation, 1 );
	ssc_data_set_number( data, "loan_moratorium", 0 );
	ssc_data_set_number( data, "system_use_recapitalization", 0 );
    ssc_number_t p_dispatch_tod_factors[9] = { 1, 1, 1, 1, 1, 1, 1, 1, 1 };
    ssc_data_set_array(data, "dispatch_tod_factors", p_dispatch_tod_factors, 9);
	ssc_data_set_number( data, "total_installed_cost", 740249984 );
	ssc_data_set_number( data, "reserves_interest", 1.75 );
	ssc_data_set_number( data, "equip1_reserve_cost", 0.25 );
	ssc_data_set_number( data, "equip1_reserve_freq", 12 );
	ssc_data_set_number( data, "equip2_reserve_cost", 0 );
	ssc_data_set_number( data, "equip2_reserve_freq", 15 );
	ssc_data_set_number( data, "equip3_reserve_cost", 0 );
	ssc_data_set_number( data, "equip3_reserve_freq", 3 );
	ssc_data_set_number( data, "equip_reserve_depr_sta", 0 );
	ssc_data_set_number( data, "equip_reserve_depr_fed", 0 );
	ssc_data_set_number( data, "salvage_percentage", 0 );
	ssc_number_t p_ppa_price_input[1] = { 0 };
	ssc_data_set_array(data, "ppa_price_input", p_ppa_price_input, 1);
	ssc_data_set_number(data, "cp_capacity_payment_esc", 0);
	ssc_data_set_number(data, "cp_capacity_payment_type", 0);
	ssc_data_set_number(data, "cp_system_nameplate", 0);
	ssc_data_set_number(data, "cp_battery_nameplate", 0);
	ssc_data_set_array(data, "cp_capacity_credit_percent", p_ppa_price_input, 1);
	ssc_data_set_array(data, "cp_capacity_payment_amount", p_ppa_price_input, 1);
	ssc_data_set_number(data, "ppa_soln_mode", 1);
	ssc_data_set_number( data, "ppa_escalation", 1 );
	ssc_data_set_number( data, "construction_financing_cost", 14805000 );
	ssc_data_set_number( data, "term_tenor", 18 );
	ssc_data_set_number( data, "term_int_rate", 7 );
	ssc_data_set_number( data, "dscr", 1.2999999523162842 );
	ssc_data_set_number( data, "dscr_reserve_months", 6 );
	ssc_data_set_number( data, "debt_percent", 50 );
	ssc_data_set_number( data, "debt_option", 1 );
	ssc_data_set_number( data, "payment_option", 0 );
	ssc_data_set_number( data, "cost_debt_closing", 450000 );
	ssc_data_set_number( data, "cost_debt_fee", 2.75 );
	ssc_data_set_number( data, "months_working_reserve", 6 );
	ssc_data_set_number( data, "months_receivables_reserve", 0 );
	ssc_data_set_number( data, "cost_other_financing", 0 );
	ssc_data_set_number( data, "flip_target_percent", 11 );
	ssc_data_set_number( data, "flip_target_year", 20 );
	ssc_data_set_number( data, "depr_alloc_macrs_5_percent", 90 );
	ssc_data_set_number( data, "depr_alloc_macrs_15_percent", 1.5 );
	ssc_data_set_number( data, "depr_alloc_sl_5_percent", 0 );
	ssc_data_set_number( data, "depr_alloc_sl_15_percent", 2.5 );
	ssc_data_set_number( data, "depr_alloc_sl_20_percent", 3 );
	ssc_data_set_number( data, "depr_alloc_sl_39_percent", 0 );
	ssc_data_set_number( data, "depr_alloc_custom_percent", 0 );
	ssc_number_t p_depr_custom_schedule[1] ={ 0 };
	ssc_data_set_array( data, "depr_custom_schedule", p_depr_custom_schedule, 1 );
	ssc_data_set_number( data, "depr_bonus_sta", 0 );
	ssc_data_set_number( data, "depr_bonus_sta_macrs_5", 0 );
	ssc_data_set_number( data, "depr_bonus_sta_macrs_15", 0 );
	ssc_data_set_number( data, "depr_bonus_sta_sl_5", 0 );
	ssc_data_set_number( data, "depr_bonus_sta_sl_15", 0 );
	ssc_data_set_number( data, "depr_bonus_sta_sl_20", 0 );
	ssc_data_set_number( data, "depr_bonus_sta_sl_39", 0 );
	ssc_data_set_number( data, "depr_bonus_sta_custom", 0 );
	ssc_data_set_number( data, "depr_bonus_fed", 0 );
	ssc_data_set_number( data, "depr_bonus_fed_macrs_5", 0 );
	ssc_data_set_number( data, "depr_bonus_fed_macrs_15", 0 );
	ssc_data_set_number( data, "depr_bonus_fed_sl_5", 0 );
	ssc_data_set_number( data, "depr_bonus_fed_sl_15", 0 );
	ssc_data_set_number( data, "depr_bonus_fed_sl_20", 0 );
	ssc_data_set_number( data, "depr_bonus_fed_sl_39", 0 );
	ssc_data_set_number( data, "depr_bonus_fed_custom", 0 );
	ssc_data_set_number( data, "depr_itc_sta_macrs_5", 0 );
	ssc_data_set_number( data, "depr_itc_sta_macrs_15", 0 );
	ssc_data_set_number( data, "depr_itc_sta_sl_5", 0 );
	ssc_data_set_number( data, "depr_itc_sta_sl_15", 0 );
	ssc_data_set_number( data, "depr_itc_sta_sl_20", 0 );
	ssc_data_set_number( data, "depr_itc_sta_sl_39", 0 );
	ssc_data_set_number( data, "depr_itc_sta_custom", 0 );
	ssc_data_set_number( data, "depr_itc_fed_macrs_5", 0 );
	ssc_data_set_number( data, "depr_itc_fed_macrs_15", 0 );
	ssc_data_set_number( data, "depr_itc_fed_sl_5", 0 );
	ssc_data_set_number( data, "depr_itc_fed_sl_15", 0 );
	ssc_data_set_number( data, "depr_itc_fed_sl_20", 0 );
	ssc_data_set_number( data, "depr_itc_fed_sl_39", 0 );
	ssc_data_set_number( data, "depr_itc_fed_custom", 0 );
	ssc_data_set_number( data, "pbi_fed_for_ds", 0 );
	ssc_data_set_number( data, "pbi_sta_for_ds", 0 );
	ssc_data_set_number( data, "pbi_uti_for_ds", 0 );
	ssc_data_set_number( data, "pbi_oth_for_ds", 0 );
	ssc_data_set_number( data, "depr_stabas_method", 1 );
	ssc_data_set_number( data, "depr_fedbas_method", 1 );
	ssc_data_set_number( data, "battery_per_kWh", 300 );
	ssc_data_set_number( data, "batt_replacement_cost_escal", 0 );
	return 0;
}
