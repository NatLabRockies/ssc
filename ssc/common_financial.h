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

#ifndef __common_financial_h
#define __common_financial_h

#include <vector>
#include "core.h"


double Const_per_principal(double const_per_percent /*%*/, double total_installed_cost /*$*/);		// [$]

double Const_per_interest(double const_per_principal /*$*/, double const_per_interest_rate /*$*/,
	double const_per_months /*months*/);		// [$]

double Const_per_total(double const_per_interest /*$*/, double const_per_principal /*$*/,
	double const_per_upfront_rate /*%*/);		// [$]


void save_cf(compute_module *cm, util::matrix_t<double>& mat, int cf_line, int nyears, const std::string &name);
void save_cf(int cf_line, int nyears, const std::string& name, util::matrix_t<double> cf, compute_module* cm); //LCOS version

extern var_info vtab_lcos_inputs[]; //LCOS var table
extern var_info vtab_update_tech_outputs[];

void lcos_calc(compute_module* cm, util::matrix_t<double> cf, int nyears, double nom_discount_rate, double inflation_rate, double lcoe_real, double total_cost, double real_discount_rate, int grid_charging_cost_version); //LCOS function

// Prepend the 0 to relevant outputs
void update_battery_outputs(compute_module* cm, size_t nyears);
// Prepend the 0 to relevant outputs
void update_fuelcell_outputs(compute_module* cm, size_t nyears);

class dispatch_calculations
{
private:
	compute_module *m_cm;
	std::vector<int> m_periods;
	std::string m_error;
	util::matrix_t<double> m_cf;
	std::vector<double> m_degradation;
	std::vector<double> m_hourly_energy;
    std::vector<double> m_dispatch_tod_factors;
	int m_nyears;
	bool m_timestep;
	ssc_number_t *m_gen; // Time series power
	ssc_number_t *m_multipliers; // Time series ppa multiplers
	size_t m_ngen; // Number of records in gen
	size_t m_nmultipliers; // Number of records in m_multipliers

public:
	dispatch_calculations() {};
	dispatch_calculations(compute_module *cm, std::vector<double>& degradation, std::vector<double>& hourly_energy);
	bool init(compute_module *cm, std::vector<double>& degradation, std::vector<double>& hourly_energy);
	bool setup();
	bool setup_ts();
	bool compute_outputs(std::vector<double>& ppa);
	bool compute_outputs_ts(std::vector<double>& ppa);
	int operator()(size_t time);
	std::string error() { return m_error; }
	bool process_dispatch_output();
	bool compute_dispatch_output();
	bool process_lifetime_dispatch_output();
	bool compute_lifetime_dispatch_output();
	bool compute_dispatch_output_ts();
	bool compute_lifetime_dispatch_output_ts();
	util::matrix_t<double>& dispatch_output();
	double tod_energy(int period, int year);
	double tod_energy_value(int period, int year);
	double tod_energy_value(int year);
};


class hourly_energy_calculation
{
private:
	compute_module *m_cm;
	std::vector<double> m_hourly_energy; // Energy used in PPA calculations
    std::vector<double> m_energy_sales; // Hourly gen values > 0
    std::vector<double> m_energy_purchases; // Hourly gen values < 0
    std::vector<double> m_energy_without_battery;
	std::string m_error;
	size_t m_nyears;
    ssc_number_t m_ts_hour_gen;
    size_t m_step_per_hour_gen;

public:
	bool calculate(compute_module *cm, bool heat = false);
	std::vector<double>& hourly_energy() {
		return m_hourly_energy;
	}
    std::vector<double>& hourly_sales() {
        return m_energy_sales;
    }
    std::vector<double>& hourly_purchases() {
        return m_energy_purchases;
    }
    std::vector<double>& hourly_energy_without_battery() {
        return m_energy_without_battery;
    }
	std::string error() { return m_error; }
    void sum_ts_to_hourly(ssc_number_t* timestep_power, std::vector<double>& hourly);
};


class check_financial_metrics
{
private:
    compute_module* m_cm;

public:
    void check_irr(compute_module* cm, ssc_number_t& irr);
    void check_irr_flip(compute_module* cm, ssc_number_t& irr);
    void check_npv(compute_module* cm, ssc_number_t& npv_metric);
    void check_debt_percentage(compute_module* cm, ssc_number_t& debt_percentage);
};


class itc_and_depreciation_calculations
{
public:
    itc_and_depreciation_calculations(compute_module* cm, size_t n_years);

    void calc_basis(double in_pre_depr_alloc_basis, double in_pre_itc_qual_basis);
    void set_depr_schedules(compute_module* cm);
    void major_equipment_depreciation(compute_module* cm, util::matrix_t<double>& main_cf, int cf_equipment_expenditure, int expenditure_year, int analysis_period, int cf_equipment_depreciation);

    void calc_annual_depreciation(int i);

    void write_outputs(compute_module* cm);

    ssc_number_t get_stadepr(int i) const;
    ssc_number_t get_feddepr(int i) const;
    ssc_number_t get_itc_sta(int i) const;
    ssc_number_t get_itc_fed(int i) const;
    ssc_number_t get_itc_total(int i) const;

    void depreciation_sched_5_year_macrs_half_year(int cf_line);
    void depreciation_sched_15_year_macrs_half_year(int cf_line);
    void depreciation_sched_5_year_straight_line_half_year(int cf_line);
    void depreciation_sched_15_year_straight_line_half_year(int cf_line);
    void depreciation_sched_20_year_straight_line_half_year(int cf_line);
    void depreciation_sched_39_year_straight_line_half_year(int cf_line);
    void depreciation_sched_custom(compute_module* cm, int cf_line, const std::string& custom);

    util::matrix_t<double> m_cf;

    size_t nyears;

    double itc_fed_amount;
    double itc_sta_amount;
    double itc_fed_per; // Still in $, even though it has per in the name
    double itc_sta_per;

    double pre_depr_alloc_basis; // Total costs that could qualify for depreciation before allocations
    double pre_itc_qual_basis; // Total costs that could qualify for ITC before allocations

    // Variables with 'frac' in the name have % as units, others are in $
    double depr_alloc_macrs_5_frac;
    double depr_alloc_macrs_15_frac;
    double depr_alloc_sl_5_frac;
    double depr_alloc_sl_15_frac;
    double depr_alloc_sl_20_frac;
    double depr_alloc_sl_39_frac;
    double depr_alloc_custom_frac;
    double depr_alloc_total_frac;

    double depr_alloc_none_frac;

    double depr_stabas_macrs_5_frac;
    double depr_stabas_macrs_15_frac;
    double depr_stabas_sl_5_frac;
    double depr_stabas_sl_15_frac;
    double depr_stabas_sl_20_frac;
    double depr_stabas_sl_39_frac;
    double depr_stabas_custom_frac;

    double depr_fedbas_macrs_5_frac;
    double depr_fedbas_macrs_15_frac;
    double depr_fedbas_sl_5_frac;
    double depr_fedbas_sl_15_frac;
    double depr_fedbas_sl_20_frac;
    double depr_fedbas_sl_39_frac;
    double depr_fedbas_custom_frac;

    double depr_alloc_macrs_5;
    double depr_alloc_macrs_15;
    double depr_alloc_sl_5;
    double depr_alloc_sl_15;
    double depr_alloc_sl_20;
    double depr_alloc_sl_39;
    double depr_alloc_custom;
    double depr_alloc_none;
    double depr_alloc_total;

    double itc_sta_qual_macrs_5_frac;
    double itc_sta_qual_macrs_15_frac;
    double itc_sta_qual_sl_5_frac;
    double itc_sta_qual_sl_15_frac;
    double itc_sta_qual_sl_20_frac;
    double itc_sta_qual_sl_39_frac;
    double itc_sta_qual_custom_frac;

    double itc_sta_qual_total;

    double itc_sta_qual_macrs_5;
    double itc_sta_qual_macrs_15;
    double itc_sta_qual_sl_5;
    double itc_sta_qual_sl_15;
    double itc_sta_qual_sl_20;
    double itc_sta_qual_sl_39;
    double itc_sta_qual_custom;

    double itc_sta_disallow_factor;


    double itc_disallow_sta_percent_macrs_5;
    double itc_disallow_sta_percent_macrs_15;
    double itc_disallow_sta_percent_sl_5;
    double itc_disallow_sta_percent_sl_15;
    double itc_disallow_sta_percent_sl_20;
    double itc_disallow_sta_percent_sl_39;
    double itc_disallow_sta_percent_custom;

    double itc_disallow_sta_fixed_macrs_5;
    double itc_disallow_sta_fixed_macrs_15;
    double itc_disallow_sta_fixed_sl_5;
    double itc_disallow_sta_fixed_sl_15;
    double itc_disallow_sta_fixed_sl_20;
    double itc_disallow_sta_fixed_sl_39;
    double itc_disallow_sta_fixed_custom;

    double itc_fed_qual_macrs_5_frac;
    double itc_fed_qual_macrs_15_frac;
    double itc_fed_qual_sl_5_frac;
    double itc_fed_qual_sl_15_frac;
    double itc_fed_qual_sl_20_frac;
    double itc_fed_qual_sl_39_frac;
    double itc_fed_qual_custom_frac;

    double itc_fed_qual_total;

    double itc_fed_qual_macrs_5;
    double itc_fed_qual_macrs_15;
    double itc_fed_qual_sl_5;
    double itc_fed_qual_sl_15;
    double itc_fed_qual_sl_20;
    double itc_fed_qual_sl_39;
    double itc_fed_qual_custom;

    double itc_fed_disallow_factor;


    double itc_disallow_fed_percent_macrs_5;
    double itc_disallow_fed_percent_macrs_15;
    double itc_disallow_fed_percent_sl_5;
    double itc_disallow_fed_percent_sl_15;
    double itc_disallow_fed_percent_sl_20;
    double itc_disallow_fed_percent_sl_39;
    double itc_disallow_fed_percent_custom;

    double itc_disallow_fed_fixed_macrs_5;
    double itc_disallow_fed_fixed_macrs_15;
    double itc_disallow_fed_fixed_sl_5;
    double itc_disallow_fed_fixed_sl_15;
    double itc_disallow_fed_fixed_sl_20;
    double itc_disallow_fed_fixed_sl_39;
    double itc_disallow_fed_fixed_custom;


    // Depreciation
    // State depreciation
    double depr_sta_reduction_ibi;

    double depr_sta_reduction_cbi;

    double depr_sta_reduction;

    double depr_stabas_macrs_5;
    double depr_stabas_macrs_15;
    double depr_stabas_sl_5;
    double depr_stabas_sl_15;
    double depr_stabas_sl_20;
    double depr_stabas_sl_39;
    double depr_stabas_custom;

    // ITC reduction
    double itc_fed_percent_deprbas_sta;
    double itc_fed_amount_deprbas_sta;
    double itc_sta_percent_deprbas_sta;
    double itc_sta_amount_deprbas_sta;


    // Bonus depreciation
    double depr_stabas_macrs_5_bonus_frac;
    double depr_stabas_macrs_15_bonus_frac;
    double depr_stabas_sl_5_bonus_frac;
    double depr_stabas_sl_15_bonus_frac;
    double depr_stabas_sl_20_bonus_frac;
    double depr_stabas_sl_39_bonus_frac;
    double depr_stabas_custom_bonus_frac;

    double depr_stabas_macrs_5_bonus;
    double depr_stabas_macrs_15_bonus;
    double depr_stabas_sl_5_bonus;
    double depr_stabas_sl_15_bonus;
    double depr_stabas_sl_20_bonus;
    double depr_stabas_sl_39_bonus;
    double depr_stabas_custom_bonus;

    double depr_stabas_total;

    // Federal depreciation
    double depr_fed_reduction_ibi;

    double depr_fed_reduction_cbi;

    double depr_fed_reduction;

    double depr_fedbas_macrs_5;
    double depr_fedbas_macrs_15;
    double depr_fedbas_sl_5;
    double depr_fedbas_sl_15;
    double depr_fedbas_sl_20;
    double depr_fedbas_sl_39;
    double depr_fedbas_custom;

    // ITC reduction
    double itc_fed_percent_deprbas_fed;
    double itc_fed_amount_deprbas_fed;
    double itc_sta_percent_deprbas_fed;
    double itc_sta_amount_deprbas_fed;

    // Bonus depreciation
    double depr_fedbas_macrs_5_bonus_frac;
    double depr_fedbas_macrs_15_bonus_frac;
    double depr_fedbas_sl_5_bonus_frac;
    double depr_fedbas_sl_15_bonus_frac;
    double depr_fedbas_sl_20_bonus_frac;
    double depr_fedbas_sl_39_bonus_frac;
    double depr_fedbas_custom_bonus_frac;

    double depr_fedbas_macrs_5_bonus;
    double depr_fedbas_macrs_15_bonus;
    double depr_fedbas_sl_5_bonus;
    double depr_fedbas_sl_15_bonus;
    double depr_fedbas_sl_20_bonus;
    double depr_fedbas_sl_39_bonus;
    double depr_fedbas_custom_bonus;

    double depr_fedbas_total;
};



/*
extern var_info vtab_advanced_financing_cost[];


class advanced_financing_cost
{
private:
	compute_module *m_cm;
	
public:
	advanced_financing_cost(compute_module *cm);
	bool compute_cost(double cost_installed, double equity, double debt, double cbi, double ibi);
};
*/


#endif
