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

#include "core.h"
#include "lib_financial.h"
#include "lib_util.h"
#include "common_financial.h"

static var_info vtab_cashloan[] = {
/*   VARTYPE           DATATYPE          NAME                        LABEL                                  UNITS         META                      GROUP            REQUIRED_IF                 CONSTRAINTS                      UI_HINTS*/

	{ SSC_INPUT,        SSC_NUMBER,     "market",                    "Residential or Commercial Market",   "0/1",          "0=residential,1=comm.",     "Financial Parameters",      "?=1",                     "INTEGER,MIN=0,MAX=1",            "" },
	{ SSC_INPUT,        SSC_NUMBER,     "mortgage",                  "Use mortgage style loan (res. only)","0/1",          "0=standard loan,1=mortgage","Financial Parameters", "?=0", "INTEGER,MIN=0,MAX=1", "" },

    { SSC_INPUT,        SSC_ARRAY,      "utility_bill_w_sys",          "Electricity bill for system", "$", "", "Charges by Month", "*", "", "" },
    { SSC_INPUT, SSC_MATRIX,           "charge_w_sys_ec_ym", "Energy charge with system", "$", "", "Charges by Month", "", "", "COL_LABEL=MONTHS,FORMAT_SPEC=CURRENCY,GROUP=UR_AM" },
    { SSC_INPUT, SSC_MATRIX,           "true_up_credits_ym",     "Net annual true-up payments", "$", "", "Charges by Month", "", "", "COL_LABEL=MONTHS,FORMAT_SPEC=CURRENCY,GROUP=UR_AM" },
    { SSC_INPUT, SSC_MATRIX, "nm_dollars_applied_ym", "Net metering credit", "$", "", "Charges by Month", "*", "", "COL_LABEL=MONTHS,FORMAT_SPEC=CURRENCY,GROUP=UR_AM" },
    { SSC_INPUT, SSC_MATRIX, "net_billing_credits_ym", "Net billing credit", "$", "", "Charges by Month", "*", "", "COL_LABEL=MONTHS,FORMAT_SPEC=CURRENCY,GROUP=UR_AM" },

     { SSC_INPUT,        SSC_ARRAY,      "batt_capacity_percent",                      "Battery relative capacity to nameplate",                 "%",        "",                     "Battery",       "",                           "",                              "" },
    { SSC_INPUT,        SSC_ARRAY,      "monthly_grid_to_batt",                       "Energy to battery from grid",                           "kWh",      "",                      "Battery",       "",                          "LENGTH=12",                     "" },
    { SSC_INPUT,        SSC_ARRAY,      "monthly_batt_to_grid",                       "Energy to grid from battery",                           "kWh",      "",                      "Battery",       "",                          "LENGTH=12",                     "" },
    { SSC_INPUT,        SSC_ARRAY,      "monthly_grid_to_load",                       "Energy to load from grid",                              "kWh",      "",                      "Battery",       "",                          "LENGTH=12",                     "" },
    { SSC_INPUT, SSC_MATRIX, "charge_w_sys_dc_tou_ym", "Demand charge with system (TOU)", "$", "", "Charges by Month", "*", "", "COL_LABEL=MONTHS,FORMAT_SPEC=CURRENCY,GROUP=UR_AM" },
    { SSC_INPUT, SSC_ARRAY, "year1_hourly_ec_with_system", "Energy charge with system (year 1 hourly)", "$", "", "Time Series", "*", "", "" },
    { SSC_INPUT, SSC_ARRAY, "year1_hourly_dc_with_system", "Demand charge with system (year 1 hourly)", "$", "", "Time Series", "*", "", "" },

//    { SSC_OUTPUT,       SSC_ARRAY,       "gen_purchases",                              "Electricity from grid",                                    "kW",      "",                       "System Output",       "",                           "",                              "" },
    { SSC_INPUT, SSC_MATRIX, "charge_w_sys_fixed_ym", "Fixed monthly charge with system", "$", "", "Charges by Month", "*", "", "COL_LABEL=MONTHS,FORMAT_SPEC=CURRENCY,GROUP=UR_AM" },

    { SSC_OUTPUT, SSC_ARRAY, "cf_utility_bill", "Electricity purchase", "$", "", "", "", "LENGTH_EQUAL=cf_length", "" },
    { SSC_INPUT, SSC_ARRAY, "year1_hourly_e_fromgrid", "Electricity from grid (year 1 hourly)", "kWh", "", "Time Series", "*", "", "" },

	{ SSC_INPUT,        SSC_NUMBER,      "total_installed_cost",     "Total installed cost",               "$",            "",                      "System Costs",            "*",                      "MIN=0",                                         "" },
	{ SSC_INPUT,        SSC_NUMBER,      "salvage_percentage",       "Salvage value percentage",           "%",            "",                      "Financial Parameters",      "?=0.0",                  "MIN=0,MAX=100",                 "" },
    //{ SSC_INPUT,        SSC_NUMBER,     "batt_salvage_percentage",                     "Net pre-tax cash battery salvage value",	                               "%",	 "",					  "Financial Parameters",             "?=0",                     "MIN=0,MAX=100",      			"" },

	{ SSC_INPUT,        SSC_ARRAY,       "annual_energy_value",      "Energy value",                       "$",            "",                      "System Output",      "*",                       "",                                         "" },
	{ SSC_INPUT,        SSC_ARRAY,       "annual_thermal_value",      "Energy value",                       "$",            "",                      "System Output",      "",                       "",                                         "" },
	{ SSC_INPUT,        SSC_ARRAY,       "gen",                      "Power generated by renewable resource",                    "kW", "", "System Output", "*", "", "" },

    { SSC_INPUT,        SSC_ARRAY,       "degradation",              "Annual degradation", "%", "", "System Output", "*", "", "" },
	{ SSC_INPUT,        SSC_NUMBER,      "system_use_lifetime_output","Lifetime hourly system outputs", "0/1", "0=hourly first year,1=hourly lifetime", "Lifetime", "*", "INTEGER,MIN=0", "" },

	/* financial outputs */
	{ SSC_OUTPUT,        SSC_NUMBER,     "cf_length",                "Number of periods in cash flow",      "",             "",                      "Cash Flow",      "*",                       "INTEGER",                                  "" },

	{ SSC_OUTPUT,        SSC_NUMBER,     "lcoe_real",                "LCOE Levelized cost of energy real",                          "cents/kWh",    "",                      "Cash Flow",      "*",                       "",                                         "" },

    { SSC_OUTPUT,        SSC_NUMBER,     "lcoe_nom",                 "LCOE Levelized cost of energy nominal",                       "cents/kWh",    "",                      "Cash Flow",      "*",                       "",                                         "" },
	{ SSC_OUTPUT,        SSC_NUMBER,     "payback",                  "Payback period",                            "years",        "",                      "Cash Flow",      "*",                       "",                                         "" },
	// added 9/26/16 for Owen Zinaman Mexico
	{ SSC_OUTPUT, SSC_NUMBER, "discounted_payback", "Discounted payback period", "years", "", "Cash Flow", "*", "", "" },
    
    { SSC_OUTPUT, SSC_NUMBER, "npv", "NPV Net present value", "$", "", "Cash Flow", "*", "", "" },
    { SSC_OUTPUT, SSC_NUMBER, "irr", "IRR Internal rate of return", "%", "", "Cash Flow", "*", "", "" },

	{ SSC_OUTPUT,        SSC_NUMBER,     "present_value_oandm",                      "Present value of O&M expenses",				   "$",            "",                      "Financial Metrics",      "*",                       "",                                         "" },
	{ SSC_OUTPUT,        SSC_NUMBER,     "present_value_oandm_nonfuel",              "Present value of non-fuel O&M expenses",				   "$",            "",                      "Financial Metrics",      "*",                       "",                                         "" },
	{ SSC_OUTPUT,        SSC_NUMBER,     "present_value_fuel",                      "Present value of fuel expenses",				   "$",            "",                      "Financial Metrics",      "*",                       "",                                         "" },
	{ SSC_OUTPUT,        SSC_NUMBER,     "present_value_insandproptax",             "Present value of insurance and property tax",				   "$",            "",                      "Financial Metrics",      "*",                       "",                                         "" },
    
    { SSC_OUTPUT, SSC_NUMBER, "npv_energy_lcos_nom", "Present value of annual stored energy (nominal)", "kWh", "", "LCOE calculations", "", "", "" },

	{ SSC_OUTPUT, SSC_NUMBER, "adjusted_installed_cost", "Net capital cost", "$", "", "Financial Metrics", "*", "", "" },
	{ SSC_OUTPUT, SSC_NUMBER, "loan_amount", "Debt", "$", "", "Financial Metrics", "*", "", "" },
	{ SSC_OUTPUT, SSC_NUMBER, "first_cost", "Equity", "$", "", "Financial Metrics", "*", "", "" },
    { SSC_OUTPUT, SSC_NUMBER, "total_cost", "Total installed cost", "$", "", "Financial Metrics", "*", "", "" },
		

    { SSC_OUTPUT,       SSC_ARRAY,      "cf_energy_net",            "Electricity net generation",                               "kWh", "", "Cash Flow Electricity", "*", "LENGTH_EQUAL=cf_length", "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "cf_energy_sales",          "Electricity generation",                                   "kWh", "", "Cash Flow Electricity", "*", "LENGTH_EQUAL=cf_length", "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "cf_energy_purchases",      "Electricity from grid to system",                          "kWh", "", "Cash Flow Electricity", "*", "LENGTH_EQUAL=cf_length", "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "cf_energy_without_battery","Electricity generated without the battery or curtailment", "kWh", "", "Cash Flow Electricity", "",  "LENGTH_EQUAL=cf_length", "" },

    { SSC_OUTPUT,        SSC_ARRAY,      "cf_energy_value",      "Value of electricity savings",                  "$",            "",                      "Cash Flow",      "*",                     "LENGTH_EQUAL=cf_length",                "" },
	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_thermal_value",      "Value of thermal savings",                  "$",            "",                      "Cash Flow",      "*",                     "LENGTH_EQUAL=cf_length",                "" },

	// real estate value added 6/24/13
	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_value_added",      "Real estate value added",                  "$",            "",                     "Cash Flow",      "*",                     "LENGTH_EQUAL=cf_length",                "" },

	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_om_fixed_expense",       "O&M fixed expense",                  "$",            "",                      "Cash Flow",      "*",                     "LENGTH_EQUAL=cf_length",                "" },
	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_om_production_expense",  "O&M production-based expense",       "$",            "",                      "Cash Flow",      "*",                     "LENGTH_EQUAL=cf_length",                "" },
	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_om_capacity_expense",    "O&M capacity-based expense",         "$",            "",                      "Cash Flow",      "*",                     "LENGTH_EQUAL=cf_length",                "" },
	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_om_fixed1_expense",      "O&M battery fixed expense",                  "$",            "",              "Cash Flow",      "",                     "LENGTH_EQUAL=cf_length",                "" },
	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_om_production1_expense", "O&M battery production-based expense",       "$",            "",              "Cash Flow",      "",                     "LENGTH_EQUAL=cf_length",                "" },
	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_om_capacity1_expense",   "O&M battery capacity-based expense",         "$",            "",              "Cash Flow",      "",                     "LENGTH_EQUAL=cf_length",                "" },
	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_om_fixed2_expense",      "O&M fuel cell fixed expense",                  "$",            "",            "Cash Flow",      "",                     "LENGTH_EQUAL=cf_length",                "" },
	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_om_production2_expense", "O&M fuel cell production-based expense",       "$",            "",            "Cash Flow",      "",                     "LENGTH_EQUAL=cf_length",                "" },
	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_om_capacity2_expense",   "O&M fuel cell capacity-based expense",         "$",            "",            "Cash Flow",      "",                     "LENGTH_EQUAL=cf_length",                "" },
	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_om_fuel_expense",        "Fuel expense",                   "$",            "",                          "Cash Flow",      "*",                     "LENGTH_EQUAL=cf_length",                "" },
	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_om_opt_fuel_1_expense",  "Feedstock biomass expense",                   "$",            "",             "Cash Flow",      "*",                     "LENGTH_EQUAL=cf_length",                "" },
	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_om_opt_fuel_2_expense",  "Feedstock coal expense",                   "$",            "",                "Cash Flow",      "*",                     "LENGTH_EQUAL=cf_length",                "" },


	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_property_tax_assessed_value","Property tax net assessed value", "$",            "",                      "Cash Flow",      "*",                     "LENGTH_EQUAL=cf_length",                "" },
	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_property_tax_expense",  "Property tax expense",               "$",            "",                      "Cash Flow",      "*",                     "LENGTH_EQUAL=cf_length",                "" },
	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_insurance_expense",     "Insurance expense",                  "$",            "",                      "Cash Flow",      "*",                     "LENGTH_EQUAL=cf_length",                "" },
	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_net_salvage_value",     "Net salvage value",                  "$",            "",                      "Cash Flow",      "*",                     "LENGTH_EQUAL=cf_length",                "" },
	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_operating_expenses",    "Total operating expense",            "$",            "",                      "Cash Flow",      "*",                     "LENGTH_EQUAL=cf_length",                "" },

	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_deductible_expenses",   "Deductible expenses",                "$",            "",                      "Cash Flow",      "*",                     "LENGTH_EQUAL=cf_length",                "" },
		
	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_debt_balance",          "Debt balance",                       "$",            "",                      "Cash Flow",      "*",                     "LENGTH_EQUAL=cf_length",                "" },
	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_debt_payment_interest", "Interest payment",                   "$",            "",                      "Cash Flow",      "*",                     "LENGTH_EQUAL=cf_length",                "" },
	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_debt_payment_principal","Principal payment",                  "$",            "",                      "Cash Flow",      "*",                     "LENGTH_EQUAL=cf_length",                "" },
	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_debt_payment_total",    "Total P&I debt payment",             "$",            "",                      "Cash Flow",      "*",                     "LENGTH_EQUAL=cf_length",                "" },

	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_sta_depr_sched",                        "State depreciation schedule",              "%",            "",                      "Cash Flow",      "*",                     "LENGTH_EQUAL=cf_length",                "" },
	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_sta_depreciation",                      "State depreciation",                       "$",            "",                      "Cash Flow",      "*",                     "LENGTH_EQUAL=cf_length",                "" },
	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_sta_incentive_income_less_deductions",  "State incentive income less deductions",   "$",            "",                      "Cash Flow",      "*",                     "LENGTH_EQUAL=cf_length",                "" },
	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_sta_taxable_income_less_deductions",    "State taxable income less deductions",     "$",            "",                      "Cash Flow",      "*",                     "LENGTH_EQUAL=cf_length",                "" },
	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_sta_tax_savings",                       "State tax savings",                        "$",            "",                      "Cash Flow",      "*",                     "LENGTH_EQUAL=cf_length",                "" },

	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_sta_taxable_incentive_income",    "State taxable incentive income",     "$",            "",                      "Cash Flow",      "*",                     "LENGTH_EQUAL=cf_length",                "" },
	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_fed_taxable_incentive_income",    "Federal taxable incentive income",     "$",            "",                      "Cash Flow",      "*",                     "LENGTH_EQUAL=cf_length",                "" },

	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_fed_depr_sched",                        "Federal depreciation schedule",            "%",            "",                      "Cash Flow",      "*",                     "LENGTH_EQUAL=cf_length",                "" },
	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_fed_depreciation",                      "Federal depreciation",                     "$",            "",                      "Cash Flow",      "*",                     "LENGTH_EQUAL=cf_length",                "" },
	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_fed_incentive_income_less_deductions",  "Federal incentive income less deductions", "$",            "",                      "Cash Flow",      "*",                     "LENGTH_EQUAL=cf_length",                "" },
	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_fed_taxable_income_less_deductions",    "Federal taxable income less deductions",   "$",            "",                      "Cash Flow",      "*",                     "LENGTH_EQUAL=cf_length",                "" },
	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_fed_tax_savings",                       "Federal tax savings",                      "$",            "",                      "Cash Flow",      "*",                     "LENGTH_EQUAL=cf_length",                "" },

	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_sta_and_fed_tax_savings",               "Total tax savings (federal and state)",      "$",            "",                      "Cash Flow",      "*",                     "LENGTH_EQUAL=cf_length",                "" },
	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_after_tax_net_equity_cost_flow",        "After-tax annual costs",           "$",            "",                      "Cash Flow",      "*",                     "LENGTH_EQUAL=cf_length",                "" },
	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_after_tax_cash_flow",                   "After-tax cash flow",                      "$",            "",                      "Cash Flow",      "*",                     "LENGTH_EQUAL=cf_length",                "" },

	{ SSC_OUTPUT, SSC_ARRAY, "cf_discounted_costs", "Discounted costs", "$", "", "Cash Flow", "*", "LENGTH_EQUAL=cf_length", "" },
	{ SSC_OUTPUT, SSC_ARRAY, "cf_discounted_savings", "Discounted savings", "$", "", "Cash Flow", "*", "LENGTH_EQUAL=cf_length", "" },

    { SSC_OUTPUT, SSC_ARRAY, "cf_parasitic_cost", "Parasitic load costs", "$", "", "Cash Flow", "*", "LENGTH_EQUAL=cf_length", "" },


	{ SSC_OUTPUT, SSC_ARRAY, "cf_discounted_payback", "Discounted payback", "$", "", "Cash Flow", "*", "LENGTH_EQUAL=cf_length", "" },
	{ SSC_OUTPUT, SSC_ARRAY, "cf_discounted_cumulative_payback", "Cumulative discounted payback", "$", "", "Cash Flow", "*", "LENGTH_EQUAL=cf_length", "" },

	{ SSC_OUTPUT, SSC_ARRAY, "cf_payback_with_expenses", "Simple payback with expenses", "$", "", "Cash Flow", "*", "LENGTH_EQUAL=cf_length", "" },
	{ SSC_OUTPUT, SSC_ARRAY, "cf_cumulative_payback_with_expenses", "Cumulative simple payback with expenses", "$", "", "Cash Flow", "*", "LENGTH_EQUAL=cf_length", "" },

	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_payback_without_expenses",              "Simple payback without expenses",                 "$",            "",                      "Cash Flow",      "*",                     "LENGTH_EQUAL=cf_length",                "" },
	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_cumulative_payback_without_expenses",   "Cumulative simple payback without expenses",      "$",            "",                      "Cash Flow",      "*",                     "LENGTH_EQUAL=cf_length",                "" },

	{ SSC_OUTPUT,        SSC_NUMBER,     "lcoptc_fed_real",                "Levelized federal PTC real",                          "cents/kWh",    "",                      "Financial Metrics",      "*",                       "",                                         "" },
	{ SSC_OUTPUT,        SSC_NUMBER,     "lcoptc_fed_nom",                 "Levelized federal PTC nominal",                       "cents/kWh",    "",                      "Financial Metrics",      "*",                       "",                                         "" },
	{ SSC_OUTPUT,        SSC_NUMBER,     "lcoptc_sta_real",                "Levelized state PTC real",                          "cents/kWh",    "",                      "Financial Metrics",      "*",                       "",                                         "" },
	{ SSC_OUTPUT,        SSC_NUMBER,     "lcoptc_sta_nom",                 "Levelized state PTC nominal",                       "cents/kWh",    "",                      "Financial Metrics",      "*",                       "",                                         "" },

	{ SSC_OUTPUT,        SSC_NUMBER,     "wacc",                "WACC Weighted average cost of capital",                          "",    "",                      "Financial Metrics",      "*",                       "",                                         "" },
	{ SSC_OUTPUT,        SSC_NUMBER,     "effective_tax_rate",                 "Effective tax rate",                       "%",    "",                      "Financial Metrics",      "*",                       "",                                         "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "nominal_discount_rate",                  "Nominal discount rate",            "%",     "",					  "Financial Metrics",			 "*",                         "",                             "" },

// NTE additions 8/10/17
	{ SSC_OUTPUT,        SSC_ARRAY,      "cf_nte",      "NTE Not to exceed",         "cents/kWh",            "",                      "Cash Flow",      "*",                     "LENGTH_EQUAL=cf_length",                "" },
	{ SSC_OUTPUT,        SSC_NUMBER,     "year1_nte",                "NTE Not to exceed Year 1",                          "cents/kWh",    "",                      "Cash Flow",      "*",                       "",                                         "" },
	{ SSC_OUTPUT,        SSC_NUMBER,     "lnte_real",                "NTE Not to exceed real",                          "cents/kWh",    "",                      "Cash Flow",      "*",                       "",                                         "" },
	{ SSC_OUTPUT,        SSC_NUMBER,     "lnte_nom",                 "NTE Not to exceed nominal",                       "cents/kWh",    "",                      "Cash Flow",      "*",                       "",                                         "" },


var_info_invalid };

extern var_info
    vtab_standard_financial[],
    vtab_standard_loan[],
    vtab_oandm[],
    vtab_depreciation[],
    vtab_battery_replacement_cost[],
    vtab_fuelcell_replacement_cost[],
    vtab_tax_credits[],
    vtab_payment_incentives[],
    vtab_lcos_inputs[],
    vtab_update_tech_outputs[],
    vtab_utility_rate_common[];
    

enum {
	CF_degradation,
	CF_energy_net,
	CF_energy_value,
	CF_thermal_value,
	CF_value_added,


	CF_om_opt_fuel_2_expense,
	CF_om_opt_fuel_1_expense,

	CF_federal_tax_frac,
	CF_state_tax_frac,
	CF_effective_tax_frac,

	CF_property_tax_assessed_value,
	CF_property_tax_expense,
	CF_insurance_expense,
	CF_net_salvage_value,
	CF_operating_expenses,

	CF_deductible_expenses,

	CF_debt_balance,
	CF_debt_payment_interest,
	CF_debt_payment_principal,
	CF_debt_payment_total,
	
	CF_pbi_fed,
	CF_pbi_sta,
	CF_pbi_uti,
	CF_pbi_oth,
	CF_pbi_total,
	
	CF_ptc_fed,
	CF_ptc_sta,
	
	CF_sta_depr_sched,
	CF_sta_depreciation,
	CF_sta_incentive_income_less_deductions,
	CF_sta_taxable_income_less_deductions,
	CF_sta_tax_savings,

	CF_sta_taxable_incentive_income,
	CF_fed_taxable_incentive_income,

	CF_fed_depr_sched,
	CF_fed_depreciation,
	CF_fed_incentive_income_less_deductions,
	CF_fed_taxable_income_less_deductions,
	CF_fed_tax_savings,

	CF_sta_and_fed_tax_savings,
	CF_after_tax_net_equity_cost_flow,
	CF_after_tax_cash_flow,

	CF_payback_with_expenses,
	CF_cumulative_payback_with_expenses,

	// Added for Owen Zinaman for Mexico Rates and analyses 9/26/16
	//- see C:\Projects\SAM\Documentation\Payback\DiscountedPayback_2016.9.26
	CF_discounted_costs,
	CF_discounted_savings,
	CF_discounted_payback,
	CF_discounted_cumulative_payback,

	
	CF_payback_without_expenses,
	CF_cumulative_payback_without_expenses,

	CF_battery_replacement_cost_schedule,
	CF_battery_replacement_cost,

	CF_fuelcell_replacement_cost_schedule,
	CF_fuelcell_replacement_cost,

    CF_energy_sales,
    CF_energy_purchases,

    CF_energy_without_battery,

	CF_nte,

    CF_om_fixed_expense,
    CF_om_production_expense,
    CF_om_capacity_expense,
    CF_om_fixed1_expense,
    CF_om_production1_expense,
    CF_om_capacity1_expense,
    CF_om_fixed2_expense,
    CF_om_production2_expense,
    CF_om_capacity2_expense,
    CF_om_fuel_expense,
    CF_energy_charged_grid,
    CF_energy_charged_pv,
    CF_energy_discharged,
    CF_charging_cost_pv,
    CF_charging_cost_grid,
    CF_om_cost_lcos,
    CF_salvage_cost_lcos,
    CF_investment_cost_lcos,
    CF_annual_cost_lcos,
    CF_util_escal_rate,

    CF_utility_bill,
    CF_parasitic_cost,

    // SAM 1038
    CF_itc_fed_amount,
    CF_itc_fed_percent_amount,
    CF_itc_fed_percent_maxvalue,
    CF_itc_fed,
    CF_itc_sta_amount,
    CF_itc_sta_percent_amount,
    CF_itc_sta_percent_maxvalue,
    CF_itc_sta,
    CF_itc_total,

    CF_max,
};




class cm_cashloan : public compute_module
{
private:
	util::matrix_t<double> cf;
    util::matrix_t<double> cf_lcos;
	double ibi_fed_amount;
	double ibi_sta_amount;
	double ibi_uti_amount;
	double ibi_oth_amount;
	double ibi_fed_per;
	double ibi_sta_per;
	double ibi_uti_per;
	double ibi_oth_per;
	double cbi_fed_amount;
	double cbi_sta_amount;
	double cbi_uti_amount;
	double cbi_oth_amount;

	hourly_energy_calculation hourly_energy_calcs;


public:
	cm_cashloan()
	{
		add_var_info( vtab_standard_financial );
		add_var_info( vtab_standard_loan );
		add_var_info( vtab_oandm );
		add_var_info( vtab_depreciation );
		add_var_info( vtab_tax_credits );
		add_var_info( vtab_payment_incentives );
		add_var_info(vtab_battery_replacement_cost);
		add_var_info(vtab_fuelcell_replacement_cost);
		add_var_info(vtab_cashloan);
        add_var_info(vtab_lcos_inputs);
        add_var_info(vtab_update_tech_outputs);
	}

	void exec( )
	{
		int i;

		bool is_commercial = (as_integer("market")==1);
		bool is_mortgage = (as_integer("mortgage")==1);

//		throw exec_error("cmod_cashloan", "mortgage = " + util::to_string(as_integer("mortgage")));
//		if (is_commercial) log("commercial market"); else log("residential market");
//		if (is_mortgage) log("mortgage loan"); else log("standard loan");
		

		int nyears = as_integer("analysis_period");

		// initialize cashflow matrix
		cf.resize_fill( CF_max, nyears+1, 0.0 );
        cf_lcos.resize_fill(CF_max, nyears + 1, 0.0);

		// initialize energy and revenue
		size_t count = 0;
		ssc_number_t *arrp = 0;
		

        double first_year_energy = 0.0;
        double first_year_sales = 0.0;
        double first_year_purchases = 0.0;


        // degradation
        // degradation starts in year 2 for single value degradation - no degradation in year 1 - degradation =1.0
        // lifetime degradation applied in technology compute modules
        if (as_integer("system_use_lifetime_output") == 1)
        {
            for (i = 1; i <= nyears; i++) cf.at(CF_degradation, i) = 1.0;
        }
        else
        {
            size_t count_degrad = 0;
            ssc_number_t* degrad = 0;
            degrad = as_array("degradation", &count_degrad);

            if (count_degrad == 1)
            {
                for (i = 1; i <= nyears; i++) cf.at(CF_degradation, i) = pow((1.0 - degrad[0] / 100.0), i - 1);
            }
            else if (count_degrad > 0)
            {
                for (i = 0; i < nyears && i < (int)count_degrad; i++) cf.at(CF_degradation, i + 1) = (1.0 - degrad[i] / 100.0);
            }
        }

        

        hourly_energy_calcs.calculate(this);

        // dispatch
        if (as_integer("system_use_lifetime_output") == 1)
        {
            // hourly_enet includes all curtailment, availability
            for (size_t y = 1; y <= (size_t)nyears; y++)
            {
                for (size_t h = 0; h < 8760; h++)
                {
                    cf.at(CF_energy_net, y) += hourly_energy_calcs.hourly_energy()[(y - 1) * 8760 + h] * cf.at(CF_degradation, y);
                    cf.at(CF_energy_sales, y) += hourly_energy_calcs.hourly_sales()[(y - 1) * 8760 + h] * cf.at(CF_degradation, y);
                    cf.at(CF_energy_purchases, y) += hourly_energy_calcs.hourly_purchases()[(y - 1) * 8760 + h] * cf.at(CF_degradation, y);
                }
            }
        }
        else
        {
            for (i = 0; i < 8760; i++) {
                first_year_energy += hourly_energy_calcs.hourly_energy()[i]; // sum up hourly kWh to get total annual kWh first year production includes first year curtailment, availability
                first_year_sales += hourly_energy_calcs.hourly_sales()[i];
                first_year_purchases += hourly_energy_calcs.hourly_purchases()[i];
            }
            cf.at(CF_energy_net, 1) = first_year_energy;
            cf.at(CF_energy_sales, 1) = first_year_sales;
            cf.at(CF_energy_purchases, 1) = first_year_purchases;
            for (i = 1; i <= nyears; i++) {
                cf.at(CF_energy_net, i) = first_year_energy * cf.at(CF_degradation, i);
                cf.at(CF_energy_sales, i) = first_year_sales * cf.at(CF_degradation, i);
                cf.at(CF_energy_purchases, i) = first_year_purchases * cf.at(CF_degradation, i);
            }

        }

        if (is_assigned("gen_without_battery") && as_vector_double("gen_without_battery").size() > 0)
        {
            ssc_number_t first_year_energy_without_battery = 0.0;
            if (as_integer("system_use_lifetime_output") == 1)
            {
                // hourly_enet includes all curtailment, availability
                for (size_t y = 1; y <= (size_t)nyears; y++)
                {
                    for (size_t h = 0; h < 8760; h++)
                    {
                        cf.at(CF_energy_without_battery, y) += hourly_energy_calcs.hourly_energy_without_battery()[(y - 1) * 8760 + h] * cf.at(CF_degradation, y);
                    }
                }
            }
            else
            {
                for (i = 0; i < 8760; i++) {
                    first_year_energy_without_battery += hourly_energy_calcs.hourly_energy_without_battery()[i];
                }
                cf.at(CF_energy_without_battery, 1) = first_year_energy_without_battery;
                for (i = 1; i <= nyears; i++) {
                    cf.at(CF_energy_without_battery, i) = first_year_energy_without_battery * cf.at(CF_degradation, i);
                }

            }
        }

		if (is_assigned("annual_thermal_value"))
		{
			arrp = as_array("annual_thermal_value", &count);
			i = 0;
			while (i < nyears && i < (int)count)
			{
				cf.at(CF_thermal_value, i + 1) = (double)arrp[i +1];
				i++;
			}
		}


		arrp = as_array("annual_energy_value", &count);
		i=0;
		while ( i < nyears && i < (int)count )
		{
			cf.at(CF_energy_value, i+1) = (double) arrp[i+1];
			i++;
		}
		
		double year1_fuel_use = as_double("annual_fuel_usage"); // kWht
		std::vector<double> fuel_use;
		if ((as_integer("system_use_lifetime_output") == 1) && is_assigned("annual_fuel_usage_lifetime")) {
			fuel_use = as_vector_double("annual_fuel_usage_lifetime");
			if (fuel_use.size() != (size_t)(nyears)) {
				throw exec_error("cashloan", util::format("fuel usage years (%d) not equal to analysis period years (%d).", (int)fuel_use.size(), nyears));
			}
		}
		else {
			for (size_t y = 0; y < (size_t)(nyears); y++) {
				fuel_use.push_back(year1_fuel_use);
			}
		}
    	double nameplate = as_double("system_capacity"); // kW
		
		double inflation_rate = as_double("inflation_rate")*0.01;
		double property_tax = as_double("property_tax_rate")*0.01;
		double property_tax_decline_percentage = as_double("prop_tax_assessed_decline");
		double insurance_rate = as_double("insurance_rate")*0.01;
		double salvage_frac = as_double("salvage_percentage")*0.01;

		//double federal_tax_rate = as_double("federal_tax_rate")*0.01;
		//double state_tax_rate = as_double("state_tax_rate")*0.01;
		//double effective_tax_rate = state_tax_rate + (1.0-state_tax_rate)*federal_tax_rate;
		arrp = as_array("federal_tax_rate", &count);
		if (count > 0)
		{
			if (count == 1) // single value input
			{
				for (i = 0; i < nyears; i++)
					cf.at(CF_federal_tax_frac, i + 1) = arrp[0]*0.01;
			}
			else // schedule
			{
				for (i = 0; i < nyears && i < (int)count; i++)
					cf.at(CF_federal_tax_frac, i + 1) = arrp[i] * 0.01;
			}
		}
		arrp = as_array("state_tax_rate", &count);
		if (count > 0)
		{
			if (count == 1) // single value input
			{
				for (i = 0; i < nyears; i++)
					cf.at(CF_state_tax_frac, i + 1) = arrp[0] * 0.01;
			}
			else // schedule
			{
				for (i = 0; i < nyears && i < (int)count; i++)
					cf.at(CF_state_tax_frac, i + 1) = arrp[i] * 0.01;
			}
		}
		for (i = 0; i <= nyears;i++)
			cf.at(CF_effective_tax_frac, i) = cf.at(CF_state_tax_frac, i) +
				(1.0 - cf.at(CF_state_tax_frac, i))*cf.at(CF_federal_tax_frac, i);



		
		double real_discount_rate = as_double("real_discount_rate")*0.01;
		double nom_discount_rate = (1.0 + real_discount_rate) * (1.0 + inflation_rate) - 1.0;


//		double hard_cost = as_double("total_hard_cost");
//		double total_sales_tax = as_double("percent_of_cost_subject_sales_tax")*0.01*hard_cost*as_double("sales_tax_rate")*0.01;
//		double soft_cost = as_double("total_soft_cost") + total_sales_tax;
//		double total_cost = hard_cost + soft_cost;
		double total_cost = as_double("total_installed_cost");
		double property_tax_assessed_value = total_cost * as_double("prop_tax_cost_assessed_percent") * 0.01;

		int loan_term = as_integer("loan_term");
		double loan_rate = as_double("loan_rate")*0.01;
		double debt_frac = as_double("debt_fraction")*0.01;
				
		// precompute expenses from annual schedules or value+escalation
		escal_or_annual( CF_om_fixed_expense, nyears, "om_fixed", inflation_rate, 1.0, false, as_double("om_fixed_escal")*0.01 );
		escal_or_annual( CF_om_production_expense, nyears, "om_production", inflation_rate, 0.001, false, as_double("om_production_escal")*0.01 );  
		escal_or_annual( CF_om_capacity_expense, nyears, "om_capacity", inflation_rate, 1.0, false, as_double("om_capacity_escal")*0.01 );  
		escal_or_annual( CF_om_fuel_expense, nyears, "om_fuel_cost", inflation_rate, as_double("system_heat_rate")*0.001, false, as_double("om_fuel_cost_escal")*0.01 );

        // additional o and m sub types (e.g. batteries and fuel cells)
        int add_om_num_types = as_integer("add_om_num_types");
        ssc_number_t nameplate1 = 0;
        ssc_number_t nameplate2 = 0;
        std::vector<double> battery_discharged(nyears,0);
        std::vector<double> fuelcell_discharged(nyears,0);

        if (add_om_num_types > 0) //PV Battery
        {
            escal_or_annual(CF_om_fixed1_expense, nyears, "om_batt_fixed_cost", inflation_rate, 1.0, false, as_double("om_fixed_escal") * 0.01);
            escal_or_annual(CF_om_production1_expense, nyears, "om_batt_variable_cost", inflation_rate, 0.001, false, as_double("om_production_escal") * 0.01); //$/MWh
            escal_or_annual(CF_om_capacity1_expense, nyears, "om_batt_capacity_cost", inflation_rate, 1.0, false, as_double("om_capacity_escal") * 0.01);
            nameplate1 = as_number("om_batt_nameplate");
            if (as_integer("en_batt") == 1 || as_integer("en_standalone_batt") == 1)
                battery_discharged = as_vector_double("batt_annual_discharge_energy");
		}
        if (battery_discharged.size() == 1) { // ssc #992
            double first_val = battery_discharged[0];
            battery_discharged.resize(nyears, first_val);
        }
        if (battery_discharged.size() != nyears)
            throw exec_error("cashloan", util::format("battery_discharged size (%d) incorrect",(int)battery_discharged.size()));

		if (add_om_num_types > 1)
		{
			escal_or_annual(CF_om_fixed2_expense, nyears, "om_fuelcell_fixed_cost", inflation_rate, 1.0, false, as_double("om_fixed_escal")*0.01);
			escal_or_annual(CF_om_production2_expense, nyears, "om_fuelcell_variable_cost", inflation_rate, 0.001, false, as_double("om_production_escal")*0.01);
			escal_or_annual(CF_om_capacity2_expense, nyears, "om_fuelcell_capacity_cost", inflation_rate, 1.0, false, as_double("om_capacity_escal")*0.01);
			nameplate2 = as_number("om_fuelcell_nameplate");
            fuelcell_discharged = as_vector_double("fuelcell_annual_energy_discharged");
		}
        if (fuelcell_discharged.size()== 1) { // ssc #992
            double first_val = fuelcell_discharged[0];
            fuelcell_discharged.resize(nyears, first_val);
         }
        if (fuelcell_discharged.size() != nyears)
            throw exec_error("cashloan", util::format("fuelcell_discharged size (%d) incorrect",(int)fuelcell_discharged.size()));

        
        // battery cost - replacement from lifetime analysis
        if ((as_integer("en_batt") == 1 || as_integer("en_standalone_batt") == 1) && (as_integer("batt_replacement_option") > 0))
        {
            ssc_number_t* batt_rep = 0;
            std::vector<ssc_number_t> replacement_percent;

            batt_rep = as_array("batt_bank_replacement", &count); // replacements per year calculated

            // replace at capacity percent
            if (as_integer("batt_replacement_option") == 1) {

                for (i = 0; i < (int)count; i++) {
                    replacement_percent.push_back(100);
                }
            }
            else {// user specified
                replacement_percent = as_vector_ssc_number_t("batt_replacement_schedule_percent");
            }
            double batt_cap = as_double("batt_computed_bank_capacity");
            // updated 10/17/15 per 10/14/15 meeting
            escal_or_annual(CF_battery_replacement_cost_schedule, nyears, "om_batt_replacement_cost", inflation_rate, batt_cap, false, as_double("om_replacement_cost_escal") * 0.01);

            for (i = 0; i < nyears && i < (int)count; i++) {
                // batt_rep and the cash flow sheets are 1 indexed, replacement_percent is zero indexed
                cf.at(CF_battery_replacement_cost, i + 1) = batt_rep[i] * replacement_percent[i] * 0.01 *
                    cf.at(CF_battery_replacement_cost_schedule, i + 1);
            }
        }

		// fuelcell cost - replacement from lifetime analysis
		if (is_assigned("fuelcell_replacement_option") && (as_integer("fuelcell_replacement_option") > 0))
		{
			ssc_number_t *fuelcell_rep = 0;
			if (as_integer("fuelcell_replacement_option") == 1)
				fuelcell_rep = as_array("fuelcell_replacement", &count); // replacements per year calculated
			else // user specified
				fuelcell_rep = as_array("fuelcell_replacement_schedule", &count); // replacements per year user-defined
			escal_or_annual(CF_fuelcell_replacement_cost_schedule, nyears, "om_fuelcell_replacement_cost", inflation_rate, nameplate2, false, as_double("om_replacement_cost_escal")*0.01);
			
			for ( i = 0; i < nyears && i < (int)count; i++) {
				cf.at(CF_fuelcell_replacement_cost, i + 1) = fuelcell_rep[i] *
					cf.at(CF_fuelcell_replacement_cost_schedule, i + 1);
			}
		}


		escal_or_annual( CF_om_opt_fuel_1_expense, nyears, "om_opt_fuel_1_cost", inflation_rate, 1.0, false, as_double("om_opt_fuel_1_cost_escal")*0.01 );  
		escal_or_annual( CF_om_opt_fuel_2_expense, nyears, "om_opt_fuel_2_cost", inflation_rate, 1.0, false, as_double("om_opt_fuel_2_cost_escal")*0.01 );  

		double om_opt_fuel_1_usage = as_double("om_opt_fuel_1_usage");
		double om_opt_fuel_2_usage = as_double("om_opt_fuel_2_usage");
		
		// ibi fixed
		ibi_fed_amount = as_double("ibi_fed_amount");
		ibi_sta_amount = as_double("ibi_sta_amount");
		ibi_uti_amount = as_double("ibi_uti_amount");
		ibi_oth_amount = as_double("ibi_oth_amount");

		// ibi percent
		ibi_fed_per = as_double("ibi_fed_percent")*0.01*total_cost;
		if (ibi_fed_per > as_double("ibi_fed_percent_maxvalue")) ibi_fed_per = as_double("ibi_fed_percent_maxvalue"); 
		ibi_sta_per = as_double("ibi_sta_percent")*0.01*total_cost;
		if (ibi_sta_per > as_double("ibi_sta_percent_maxvalue")) ibi_sta_per = as_double("ibi_sta_percent_maxvalue"); 
		ibi_uti_per = as_double("ibi_uti_percent")*0.01*total_cost;
		if (ibi_uti_per > as_double("ibi_uti_percent_maxvalue")) ibi_uti_per = as_double("ibi_uti_percent_maxvalue"); 
		ibi_oth_per = as_double("ibi_oth_percent")*0.01*total_cost;
		if (ibi_oth_per > as_double("ibi_oth_percent_maxvalue")) ibi_oth_per = as_double("ibi_oth_percent_maxvalue"); 

		// cbi
		cbi_fed_amount = 1000.0*nameplate*as_double("cbi_fed_amount");
		if (cbi_fed_amount > as_double("cbi_fed_maxvalue")) cbi_fed_amount = as_double("cbi_fed_maxvalue"); 
		cbi_sta_amount = 1000.0*nameplate*as_double("cbi_sta_amount");
		if (cbi_sta_amount > as_double("cbi_sta_maxvalue")) cbi_sta_amount = as_double("cbi_sta_maxvalue"); 
		cbi_uti_amount = 1000.0*nameplate*as_double("cbi_uti_amount");
		if (cbi_uti_amount > as_double("cbi_uti_maxvalue")) cbi_uti_amount = as_double("cbi_uti_maxvalue"); 
		cbi_oth_amount = 1000.0*nameplate*as_double("cbi_oth_amount");
		if (cbi_oth_amount > as_double("cbi_oth_maxvalue")) cbi_oth_amount = as_double("cbi_oth_maxvalue"); 
		
		// precompute pbi
		compute_production_incentive( CF_pbi_fed, nyears, "pbi_fed_amount", "pbi_fed_term", "pbi_fed_escal" );
		compute_production_incentive( CF_pbi_sta, nyears, "pbi_sta_amount", "pbi_sta_term", "pbi_sta_escal" );
		compute_production_incentive( CF_pbi_uti, nyears, "pbi_uti_amount", "pbi_uti_term", "pbi_uti_escal" );
		compute_production_incentive( CF_pbi_oth, nyears, "pbi_oth_amount", "pbi_oth_term", "pbi_oth_escal" );

		// precompute ptc
		compute_production_incentive_IRS_2010_37( CF_ptc_sta, nyears, "ptc_sta_amount", "ptc_sta_term", "ptc_sta_escal" );
		compute_production_incentive_IRS_2010_37( CF_ptc_fed, nyears, "ptc_fed_amount", "ptc_fed_term", "ptc_fed_escal" );
	
		// reduce itc bases
		double federal_itc_basis = total_cost
			- ( as_boolean("ibi_fed_amount_deprbas_fed")  ? ibi_fed_amount : 0 )
			- ( as_boolean("ibi_sta_amount_deprbas_fed")  ? ibi_sta_amount : 0 )
			- ( as_boolean("ibi_uti_amount_deprbas_fed")  ? ibi_uti_amount : 0 )
			- ( as_boolean("ibi_oth_amount_deprbas_fed")  ? ibi_oth_amount : 0 )
			- ( as_boolean("ibi_fed_percent_deprbas_fed") ? ibi_fed_per : 0 )
			- ( as_boolean("ibi_sta_percent_deprbas_fed") ? ibi_sta_per : 0 )
			- ( as_boolean("ibi_uti_percent_deprbas_fed") ? ibi_uti_per : 0 )
			- ( as_boolean("ibi_oth_percent_deprbas_fed") ? ibi_oth_per : 0 )
			- ( as_boolean("cbi_fed_deprbas_fed")  ? cbi_fed_amount : 0 )
			- ( as_boolean("cbi_sta_deprbas_fed")  ? cbi_sta_amount : 0 )
			- ( as_boolean("cbi_uti_deprbas_fed")  ? cbi_uti_amount : 0 )
			- ( as_boolean("cbi_oth_deprbas_fed")  ? cbi_oth_amount : 0 );


        // SAM 1038
         // itc fixed
        double itc_fed_amount = 0.0;
        double_vec vitc_fed_amount = as_vector_double("itc_fed_amount");
        for (size_t k = 0; k < vitc_fed_amount.size() && k < nyears; k++) {
            cf.at(CF_itc_fed_amount, k + 1) = vitc_fed_amount[k];
            itc_fed_amount += vitc_fed_amount[k];
        }

        double itc_sta_amount = 0.0;
        double_vec vitc_sta_amount = as_vector_double("itc_sta_amount");
        for (size_t k = 0; k < vitc_sta_amount.size() && k < nyears; k++) {
            cf.at(CF_itc_sta_amount, k + 1) = vitc_sta_amount[k];
            itc_sta_amount += vitc_sta_amount[k];
        }

        // itc percent - max value used for comparison to qualifying costs
        double_vec vitc_fed_frac = as_vector_double("itc_fed_percent");
        for (size_t k = 0; k < vitc_fed_frac.size(); k++)
            cf.at(CF_itc_fed_percent_amount, k + 1) = vitc_fed_frac[k] * 0.01;
        double itc_fed_per;
        double_vec vitc_sta_frac = as_vector_double("itc_sta_percent");
        for (size_t k = 0; k < vitc_sta_frac.size(); k++)
            cf.at(CF_itc_sta_percent_amount, k + 1) = vitc_sta_frac[k] * 0.01;
        double itc_sta_per;

        double_vec itc_sta_percent_maxvalue = as_vector_double("itc_sta_percent_maxvalue");
        if (itc_sta_percent_maxvalue.size() == 1) {
            for (size_t k = 0; k < nyears; k++)
                cf.at(CF_itc_sta_percent_maxvalue, k + 1) = itc_sta_percent_maxvalue[0];
        }
        else {
            for (size_t k = 0; k < itc_sta_percent_maxvalue.size() && k < nyears; k++)
                cf.at(CF_itc_sta_percent_maxvalue, k + 1) = itc_sta_percent_maxvalue[k];
        }

        double_vec itc_fed_percent_maxvalue = as_vector_double("itc_fed_percent_maxvalue");
        if (itc_fed_percent_maxvalue.size() == 1) {
            for (size_t k = 0; k < nyears; k++)
                cf.at(CF_itc_fed_percent_maxvalue, k + 1) = itc_fed_percent_maxvalue[0];
        }
        else {
            for (size_t k = 0; k < itc_fed_percent_maxvalue.size() && k < nyears; k++)
                cf.at(CF_itc_fed_percent_maxvalue, k + 1) = itc_fed_percent_maxvalue[k];
        }



		double state_itc_basis = total_cost
			- ( as_boolean("ibi_fed_amount_deprbas_sta")  ? ibi_fed_amount : 0 )
			- ( as_boolean("ibi_sta_amount_deprbas_sta")  ? ibi_sta_amount : 0 )
			- ( as_boolean("ibi_uti_amount_deprbas_sta")  ? ibi_uti_amount : 0 )
			- ( as_boolean("ibi_oth_amount_deprbas_sta")  ? ibi_oth_amount : 0 )
			- ( as_boolean("ibi_fed_percent_deprbas_sta") ? ibi_fed_per : 0 )
			- ( as_boolean("ibi_sta_percent_deprbas_sta") ? ibi_sta_per : 0 )
			- ( as_boolean("ibi_uti_percent_deprbas_sta") ? ibi_uti_per : 0 )
			- ( as_boolean("ibi_oth_percent_deprbas_sta") ? ibi_oth_per : 0 )
			- ( as_boolean("cbi_fed_deprbas_sta")  ? cbi_fed_amount : 0 )
			- ( as_boolean("cbi_sta_deprbas_sta")  ? cbi_sta_amount : 0 )
			- ( as_boolean("cbi_uti_deprbas_sta")  ? cbi_uti_amount : 0 )
			- ( as_boolean("cbi_oth_deprbas_sta")  ? cbi_oth_amount : 0 );


        // SAM 1038
        itc_sta_per = 0.0;
        for (size_t k = 0; k <= nyears; k++) {
            cf.at(CF_itc_sta_percent_amount, k) = libfin::min(cf.at(CF_itc_sta_percent_maxvalue, k), cf.at(CF_itc_sta_percent_amount, k) * state_itc_basis);
            itc_sta_per += cf.at(CF_itc_sta_percent_amount, k);
        }

        // SAM 1038
        itc_fed_per = 0.0;
        for (size_t k = 0; k <= nyears; k++) {
            cf.at(CF_itc_fed_percent_amount, k) = libfin::min(cf.at(CF_itc_fed_percent_maxvalue, k), cf.at(CF_itc_fed_percent_amount, k) * federal_itc_basis);
            itc_fed_per += cf.at(CF_itc_fed_percent_amount, k);
        }

		double federal_depr_basis = federal_itc_basis
			- ( as_boolean("itc_fed_amount_deprbas_fed")   ? 0.5*itc_fed_amount : 0)
			- ( as_boolean("itc_fed_percent_deprbas_fed")  ? 0.5*itc_fed_per : 0 )
			- ( as_boolean("itc_sta_amount_deprbas_fed")   ? 0.5*itc_sta_amount : 0)
			- ( as_boolean("itc_sta_percent_deprbas_fed")  ? 0.5*itc_sta_per : 0 );

		double state_depr_basis = state_itc_basis 
			- ( as_boolean("itc_fed_amount_deprbas_sta")   ? 0.5*itc_fed_amount : 0)
			- ( as_boolean("itc_fed_percent_deprbas_sta")  ? 0.5*itc_fed_per : 0 )
			- ( as_boolean("itc_sta_amount_deprbas_sta")   ? 0.5*itc_sta_amount : 0)
			- ( as_boolean("itc_sta_percent_deprbas_sta")  ? 0.5*itc_sta_per : 0 );

		if (is_commercial)
		{
			// only compute depreciation for commercial market

			switch( as_integer("depr_sta_type") )
			{
			case 1: depreciation_sched_macrs_half_year( CF_sta_depr_sched, nyears ); break;
			case 2: depreciation_sched_straight_line( CF_sta_depr_sched, nyears, as_integer("depr_sta_sl_years") ); break;
			case 3: 
				{
					size_t arr_len;
					ssc_number_t *arr_cust = as_array( "depr_sta_custom", &arr_len );
					depreciation_sched_custom( CF_sta_depr_sched, nyears, arr_cust, (int)arr_len );
					break;
				}
			}

			switch( as_integer("depr_fed_type") )
			{
			case 1: depreciation_sched_macrs_half_year( CF_fed_depr_sched, nyears ); break;
			case 2: depreciation_sched_straight_line( CF_fed_depr_sched, nyears, as_integer("depr_fed_sl_years") ); break;
			case 3: 
				{
					size_t arr_len;
					ssc_number_t *arr_cust = as_array( "depr_fed_custom", &arr_len );
					depreciation_sched_custom( CF_fed_depr_sched, nyears, arr_cust, (int)arr_len );
					break;
				}
			}
		}
		
		double state_tax_savings = 0.0;
		double federal_tax_savings = 0.0;

		double adjusted_installed_cost = total_cost
			- ibi_fed_amount
			- ibi_sta_amount
			- ibi_uti_amount
			- ibi_oth_amount
			- ibi_fed_per
			- ibi_sta_per
			- ibi_uti_per
			- ibi_oth_per
			- cbi_fed_amount
			- cbi_sta_amount
			- cbi_uti_amount
			- cbi_oth_amount;

		double loan_amount = debt_frac * adjusted_installed_cost;
		double first_cost = adjusted_installed_cost - loan_amount;  //cpg: What is the difference between adjusted_installed_cost and capital_investment?
		double capital_investment = loan_amount + first_cost;
		
		cf.at(CF_after_tax_net_equity_cost_flow,0) = -first_cost + state_tax_savings + federal_tax_savings;
		cf.at(CF_after_tax_cash_flow,0) = cf.at(CF_after_tax_net_equity_cost_flow,0);

		cf.at(CF_payback_with_expenses, 0) = -capital_investment;
		cf.at(CF_cumulative_payback_with_expenses, 0) = -capital_investment;

		cf.at(CF_discounted_costs, 0) = capital_investment;
		cf.at(CF_discounted_payback, 0) = cf.at(CF_discounted_savings, 0) - cf.at(CF_discounted_costs, 0);
		cf.at(CF_discounted_cumulative_payback, 0) = cf.at(CF_discounted_payback, 0);

		cf.at(CF_payback_without_expenses,0) = -capital_investment;
		cf.at(CF_cumulative_payback_without_expenses,0) = -capital_investment;

		double ibi_total = ibi_fed_amount + ibi_fed_per + ibi_sta_amount + ibi_sta_per + ibi_uti_amount + ibi_uti_per + ibi_oth_amount + ibi_oth_per;
		double cbi_total = cbi_fed_amount + cbi_sta_amount + cbi_uti_amount + cbi_oth_amount;
//		double itc_total_fed = itc_fed_amount + itc_fed_per;
//		double itc_total_sta = itc_sta_amount + itc_sta_per;

        // SAM 1038
        for (size_t k = 0; k <= nyears; k++) {
            cf.at(CF_itc_fed, k) = cf.at(CF_itc_fed_amount, k) + cf.at(CF_itc_fed_percent_amount, k);
            cf.at(CF_itc_sta, k) = cf.at(CF_itc_sta_amount, k) + cf.at(CF_itc_sta_percent_amount, k);
            cf.at(CF_itc_total, k) = cf.at(CF_itc_fed, k) + cf.at(CF_itc_sta, k);
        }

		for (i=1; i<=nyears; i++)
		{			
			// compute expenses
            if (is_assigned("gen_without_battery"))
            {
                // TODO: this does not include curtailment, but CF_energy_net does. Which should be used for VOM?
                cf.at(CF_om_production_expense, i) *= cf.at(CF_energy_without_battery, i);
            }
            else
            {
                cf.at(CF_om_production_expense, i) *= cf.at(CF_energy_sales, i);
            }
            cf.at(CF_om_capacity_expense,i) *= nameplate;

			cf.at(CF_om_capacity1_expense, i) *= nameplate1;
			cf.at(CF_om_capacity2_expense, i) *= nameplate2;
			// TODO additional production o and m here


			cf.at(CF_om_fuel_expense,i) *= fuel_use[i-1];

            //Battery Production OM Costs
            cf.at(CF_om_production1_expense, i) *= battery_discharged[i - 1]; //$/MWh * 0.001 MWh/kWh * kWh = $
            cf.at(CF_om_production2_expense, i) *= fuelcell_discharged[i-1];

			cf.at(CF_om_opt_fuel_1_expense,i) *= om_opt_fuel_1_usage;
			cf.at(CF_om_opt_fuel_2_expense,i) *= om_opt_fuel_2_usage;
			double decline_percent = 100 - (i-1)*property_tax_decline_percentage;
			cf.at(CF_property_tax_assessed_value,i) = (decline_percent > 0) ? property_tax_assessed_value * decline_percent * 0.01:0.0;
			cf.at(CF_property_tax_expense,i) = cf.at(CF_property_tax_assessed_value,i) * property_tax;
			
			cf.at(CF_insurance_expense,i) = total_cost * insurance_rate * pow( 1 + inflation_rate, i-1 );

			cf.at(CF_net_salvage_value, i) = 0.0;
			if (i == nyears) /* salvage value handled as negative operating expense in last year */
				cf.at(CF_net_salvage_value,i) = total_cost * salvage_frac;

			cf.at(CF_operating_expenses,i) = 
				+cf.at(CF_om_fixed_expense, i)
				+ cf.at(CF_om_production_expense, i)
				+ cf.at(CF_om_capacity_expense, i)
				+ cf.at(CF_om_fixed1_expense, i)
				+ cf.at(CF_om_production1_expense, i)
				+ cf.at(CF_om_capacity1_expense, i)
				+ cf.at(CF_om_fixed2_expense, i)
				+ cf.at(CF_om_production2_expense, i)
				+ cf.at(CF_om_capacity2_expense, i)
				+ cf.at(CF_om_fuel_expense,i)
				+ cf.at(CF_om_opt_fuel_1_expense,i)
				+ cf.at(CF_om_opt_fuel_2_expense,i)
				+ cf.at(CF_property_tax_expense,i)
				+ cf.at(CF_insurance_expense,i)
				+ cf.at(CF_battery_replacement_cost, i)
				+ cf.at(CF_fuelcell_replacement_cost, i)
				- cf.at(CF_net_salvage_value,i);

			
			if (is_commercial)
				cf.at(CF_deductible_expenses,i) = -cf.at(CF_operating_expenses,i);  // commercial
			else
				cf.at(CF_deductible_expenses,i) = -cf.at(CF_property_tax_expense,i); // residential

			if (i == 1)
			{
				cf.at(CF_debt_balance, i-1) = loan_amount;
				cf.at(CF_debt_payment_interest, i) = loan_amount * loan_rate;
				cf.at(CF_debt_payment_principal,i) = -libfin::ppmt( loan_rate,       // Rate
																i,           // Period
																loan_term,   // Number periods
																loan_amount, // Present Value
																0,           // future Value
																0 );         // cash flow at end of period
				cf.at(CF_debt_balance, i) = cf.at(CF_debt_balance, i - 1) - cf.at(CF_debt_payment_principal, i);
			}
			else
			{
				if (i <= loan_term) 
				{
					cf.at(CF_debt_payment_interest, i) = loan_rate * cf.at(CF_debt_balance, i-1);

					if (loan_rate != 0.0)
					{
						cf.at(CF_debt_payment_principal,i) = loan_rate * loan_amount/(1 - pow((1 + loan_rate),-loan_term))
							- cf.at(CF_debt_payment_interest,i);
					}
					else
					{
						cf.at(CF_debt_payment_principal,i) = loan_amount / loan_term - cf.at(CF_debt_payment_interest,i);
					}
					cf.at(CF_debt_balance, i) = cf.at(CF_debt_balance, i - 1) - cf.at(CF_debt_payment_principal, i);
				}
			}

			cf.at(CF_debt_payment_total,i) = cf.at(CF_debt_payment_principal,i) + cf.at(CF_debt_payment_interest,i);
			
			// compute pbi total		
			cf.at(CF_pbi_total, i) = cf.at(CF_pbi_fed, i) + cf.at(CF_pbi_sta, i) + cf.at(CF_pbi_uti, i) + cf.at(CF_pbi_oth, i);
			
			// compute depreciation from basis and precalculated schedule
			cf.at(CF_sta_depreciation,i) = cf.at(CF_sta_depr_sched,i)*state_depr_basis;
			cf.at(CF_fed_depreciation,i) = cf.at(CF_fed_depr_sched,i)*federal_depr_basis;

			
			// ************************************************
			// tax effect on equity (state)

			cf.at(CF_sta_incentive_income_less_deductions, i) =
				+ cf.at(CF_deductible_expenses, i) 
				+ cf.at(CF_pbi_total,i)
				- cf.at(CF_sta_depreciation,i);

			if (i==1) cf.at(CF_sta_incentive_income_less_deductions, i) += ibi_total + cbi_total;

// sales tax is in depreciable bases and is already written off according to depreciation schedule.
//			if (is_commercial && i == 1) cf.at(CF_sta_incentive_income_less_deductions, i) -= total_sales_tax;


			if (is_commercial || is_mortgage) // interest only deductible if residential mortgage or commercial
				cf.at(CF_sta_incentive_income_less_deductions, i) -= cf.at(CF_debt_payment_interest,i);

			cf.at(CF_sta_taxable_income_less_deductions, i) = taxable_incentive_income( i, "sta" )
				+ cf.at(CF_deductible_expenses,i)
				- cf.at(CF_sta_depreciation,i);

			cf.at(CF_sta_taxable_incentive_income, i) = taxable_incentive_income(i, "sta");
			
			// sales tax is incf_fed_taxable_incentive_income" depreciable bases and is already written off according to depreciation schedule.
//			if (is_commercial && i == 1) cf.at(CF_sta_taxable_income_less_deductions,i) -= total_sales_tax;

			if (is_commercial || is_mortgage) // interest only deductible if residential mortgage or commercial
				cf.at(CF_sta_taxable_income_less_deductions, i) -= cf.at(CF_debt_payment_interest,i);

			cf.at(CF_sta_tax_savings, i) = cf.at(CF_ptc_sta,i) - cf.at(CF_state_tax_frac,i)*cf.at(CF_sta_taxable_income_less_deductions,i);
// SAM 1038			if (i==1) cf.at(CF_sta_tax_savings, i) += itc_sta_amount + itc_sta_per;
            cf.at(CF_sta_tax_savings, i) += cf.at(CF_itc_sta_amount,i) + cf.at(CF_itc_sta_percent_amount,i);

			// ************************************************
			//	tax effect on equity (federal)

			cf.at(CF_fed_incentive_income_less_deductions, i) =
				+ cf.at(CF_deductible_expenses, i)
				+ cf.at(CF_pbi_total,i)
				- cf.at(CF_fed_depreciation,i)
				+ cf.at(CF_sta_tax_savings, i);
			
			if (i==1) cf.at(CF_fed_incentive_income_less_deductions, i) += ibi_total + cbi_total;

// sales tax is in depreciable bases and is already written off according to depreciation schedule.
//			if (is_commercial && i == 1) cf.at(CF_fed_incentive_income_less_deductions, i) -= total_sales_tax;
			
			if (is_commercial || is_mortgage) // interest only deductible if residential mortgage or commercial
				cf.at(CF_fed_incentive_income_less_deductions, i) -= cf.at(CF_debt_payment_interest,i);

			cf.at(CF_fed_taxable_income_less_deductions, i) = taxable_incentive_income( i, "fed" )
				+ cf.at(CF_deductible_expenses,i)
				- cf.at(CF_fed_depreciation,i)
				+ cf.at(CF_sta_tax_savings, i);

			cf.at(CF_fed_taxable_incentive_income, i) = taxable_incentive_income(i, "fed");

// sales tax is in depreciable bases and is already written off according to depreciation schedule.
//			if (is_commercial && i == 1) cf.at(CF_fed_taxable_income_less_deductions, i) -= total_sales_tax;

			if (is_commercial || is_mortgage) // interest only deductible if residential mortgage or commercial
				cf.at(CF_fed_taxable_income_less_deductions, i) -= cf.at(CF_debt_payment_interest,i);
			
			cf.at(CF_fed_tax_savings, i) = cf.at(CF_ptc_fed,i) - cf.at(CF_federal_tax_frac,i)*cf.at(CF_fed_taxable_income_less_deductions,i);
//  SAM 1038          if (i == 1) cf.at(CF_fed_tax_savings, i) += itc_fed_amount + itc_fed_per;
            cf.at(CF_fed_tax_savings, i) += cf.at(CF_itc_fed_amount,i) + cf.at(CF_itc_fed_percent_amount,i);

			// ************************************************
			// combined tax savings and cost/cash flows
				
			cf.at(CF_sta_and_fed_tax_savings,i) = cf.at(CF_sta_tax_savings, i)+cf.at(CF_fed_tax_savings, i);

			cf.at(CF_after_tax_net_equity_cost_flow, i) =
				+ (is_commercial ? cf.at(CF_deductible_expenses, i) : -cf.at(CF_operating_expenses,i) )
				- cf.at(CF_debt_payment_total, i)
				+ cf.at(CF_pbi_total, i)
				+ cf.at(CF_sta_and_fed_tax_savings,i);

			/*
			Calculate discounted payback period from March 1995 NREL/TP-462-5173 p.58
			CF_discounted_costs,
			CF_discounted_savings,
			CF_discounted_payback,
			CF_discounted_cumulative_payback,
			*/
			// take costs to be positive in this context
			cf.at(CF_discounted_costs, i) = -cf.at(CF_after_tax_net_equity_cost_flow, i) - cf.at(CF_debt_payment_total, i);
			// interest already deducted and accounted for in tax savings (so add back here)
			if (is_commercial || is_mortgage)
				cf.at(CF_discounted_costs, i) += cf.at(CF_debt_payment_interest, i) * cf.at(CF_effective_tax_frac,i);
			// discount at nominal discount rate
			cf.at(CF_discounted_costs, i) /= pow((1.0 + nom_discount_rate), (i));
			// savings reduced by effective tax rate for commercial since already included in tax savings
			cf.at(CF_discounted_savings, i) = ((is_commercial ? (1.0 - cf.at(CF_effective_tax_frac, i)) : 1.0)*cf.at(CF_energy_value, i)) / pow((1.0 + nom_discount_rate), (i))
				+ ((is_commercial ? (1.0 - cf.at(CF_effective_tax_frac, i)) : 1.0)*cf.at(CF_thermal_value, i)) / pow((1.0 + nom_discount_rate), (i));
			cf.at(CF_discounted_payback, i) = cf.at(CF_discounted_savings, i) - cf.at(CF_discounted_costs, i);
			cf.at(CF_discounted_cumulative_payback, i) =
				cf.at(CF_discounted_cumulative_payback, i - 1)
				+ cf.at(CF_discounted_payback, i);



			cf.at(CF_after_tax_cash_flow,i) = 
				cf.at(CF_after_tax_net_equity_cost_flow, i)
				+ ((is_commercial ? (1.0 - cf.at(CF_effective_tax_frac, i)) : 1.0)*cf.at(CF_energy_value, i))
				+((is_commercial ? (1.0 - cf.at(CF_effective_tax_frac, i)) : 1.0)*cf.at(CF_thermal_value, i));

			if ( is_commercial || is_mortgage )
				cf.at(CF_payback_with_expenses,i) =
					cf.at(CF_after_tax_cash_flow,i)
					+ cf.at(CF_debt_payment_interest, i) * (1 - cf.at(CF_effective_tax_frac, i))
					+ cf.at(CF_debt_payment_principal,i);
			else
				cf.at(CF_payback_with_expenses,i) =
					cf.at(CF_after_tax_cash_flow,i)
					+ cf.at(CF_debt_payment_interest,i)
					+ cf.at(CF_debt_payment_principal,i);

			cf.at(CF_cumulative_payback_with_expenses,i) = 
				cf.at(CF_cumulative_payback_with_expenses,i-1)
				+cf.at(CF_payback_with_expenses,i);
	
			if ( is_commercial || is_mortgage )
				cf.at(CF_payback_without_expenses,i) =
					+ cf.at(CF_after_tax_cash_flow,i)
					+ cf.at(CF_debt_payment_interest, i) * (1.0 - cf.at(CF_effective_tax_frac, i))
					+ cf.at(CF_debt_payment_principal,i)
					- cf.at(CF_deductible_expenses,i)
					+ cf.at(CF_deductible_expenses, i) * cf.at(CF_effective_tax_frac, i);
			else
				cf.at(CF_payback_without_expenses,i) =
					+ cf.at(CF_after_tax_cash_flow,i)
					+ cf.at(CF_debt_payment_interest,i)
					+ cf.at(CF_debt_payment_principal,i)
					- cf.at(CF_deductible_expenses,i)
					+ cf.at(CF_deductible_expenses, i) * cf.at(CF_effective_tax_frac, i);


			cf.at(CF_cumulative_payback_without_expenses,i) =
				+ cf.at(CF_cumulative_payback_without_expenses,i-1)
				+ cf.at(CF_payback_without_expenses,i);	
		}


        util::matrix_t<double> monthly_energy_charge; //monthly energy charges at 12 month x nyears matrix ($)
        util::matrix_t<double> net_annual_true_up; //net annual true up payments as 12 month x nyears matrix ($)
        util::matrix_t<double> net_billing_credit; //net annual true up payments as 12 month x nyears matrix ($)
        util::matrix_t<double> net_metering_credit; 
        net_annual_true_up = as_matrix("true_up_credits_ym"); //Use net annual true up payments regardless of billing mode ($)
        net_billing_credit = as_matrix("net_billing_credits_ym"); //Use net annual true up payments regardless of billing mode ($)
        net_metering_credit = as_matrix("nm_dollars_applied_ym");
        size_t n_year1_hourly_ec;
        size_t n_year1_hourly_dc;
        size_t n_gen_purchases;
        ssc_number_t* year1_hourly_ec = as_array("year1_hourly_ec_with_system", &n_year1_hourly_ec);
        ssc_number_t* year1_hourly_dc = as_array("year1_hourly_dc_with_system", &n_year1_hourly_dc);
        ssc_number_t* gen_purchases = as_array("gen_purchases", &n_gen_purchases);
        if (is_assigned("rate_escalation")) //Create rate escalation nyears array with inflation and specified rate escalation %
            escal_or_annual(CF_util_escal_rate, nyears, "rate_escalation", inflation_rate, 0.01, true, 0);
        save_cf(CF_util_escal_rate, nyears, "cf_util_escal_rate");
        cf.at(CF_parasitic_cost, 0) = 0;
        double monthly_hourly_purchases = 0;
        double monthly_hourly_fromgrid = 0;
        int n_steps_per_hour = n_year1_hourly_ec / 8760;
        size_t n_e_fromgrid;
        ssc_number_t* year1_hourly_e_from_grid = as_array("year1_hourly_e_fromgrid", &n_e_fromgrid);
        ssc_number_t* monthly_gen_purchases = allocate("monthly_gen_purchases", 12 * nyears);
        ssc_number_t* monthly_e_fromgrid = allocate("monthly_e_from_grid", 12);
        for (size_t a = 1; a <= nyears; a++) {
            if (as_integer("system_use_lifetime_output") == 1) {
                for (size_t m = 1; m <= 12; m++) {
                    monthly_hourly_purchases = 0;
                    monthly_hourly_fromgrid = 0;
                    for (size_t d = 1; d <= util::days_in_month(int(m - 1)); d++) {
                        for (size_t h = 0; h < 24; h++) { //monthly iteration for each year
                            for (size_t n = 0; n < n_steps_per_hour; n++) {
                                monthly_e_fromgrid[m-1] += year1_hourly_e_from_grid[util::hour_of_year(m, d, h) * n_steps_per_hour + n];
                                monthly_gen_purchases[(a - 1) * 12 + m-1] += -gen_purchases[(a - 1) * 8760 * n_steps_per_hour + n_steps_per_hour * util::hour_of_year(m, d, h) + n];
                                if (year1_hourly_e_from_grid[h] != 0.0) {
                                    cf.at(CF_parasitic_cost, a) += -gen_purchases[(a - 1) * 8760 * n_steps_per_hour + n_steps_per_hour * util::hour_of_year(m, d, h) + n] * cf.at(CF_degradation, a) / year1_hourly_e_from_grid[h] * (year1_hourly_ec[h * n_steps_per_hour + n] + year1_hourly_dc[h * n_steps_per_hour + n]) * cf.at(CF_util_escal_rate, a); //use the electricity rate data by year (also trueup) //* charged_grid[a] / charged_grid[1] * cf.at(CF_util_escal_rate, a);
                                }
                                if (d == util::days_in_month(int(m - 1)) && h == 23 && monthly_e_fromgrid[m - 1] > 0) cf.at(CF_parasitic_cost, a) += -monthly_gen_purchases[(a - 1) * 12 + m - 1] / monthly_e_fromgrid[m - 1] * (net_annual_true_up.at(a, m - 1) + net_billing_credit.at(a, m - 1) + net_metering_credit.at(a, m - 1));
                            }
                        }
                    }
                }
            }
            else {
                for (size_t m = 1; m <= 12; m++) {
                    monthly_hourly_purchases = 0;
                    monthly_hourly_fromgrid = 0;
                    for (size_t d = 1; d <= util::days_in_month(int(m - 1)); d++) {
                        for (size_t h = 0; h < 24; h++) { //monthly iteration for each year
                            for (size_t n = 0; n < n_steps_per_hour; n++) {
                                monthly_e_fromgrid[m-1] += year1_hourly_e_from_grid[util::hour_of_year(m, d, h) * n_steps_per_hour + n];
                                monthly_gen_purchases[(a - 1) * 12 + m-1] += -gen_purchases[n_steps_per_hour * util::hour_of_year(m, d, h) + n];
                                if (year1_hourly_e_from_grid[h] != 0.0) {
                                    cf.at(CF_parasitic_cost, a) += -gen_purchases[n_steps_per_hour * util::hour_of_year(m, d, h) + n] * cf.at(CF_degradation, a) / year1_hourly_e_from_grid[h] * (year1_hourly_ec[h * n_steps_per_hour + n] + year1_hourly_dc[h * n_steps_per_hour + n]) * cf.at(CF_util_escal_rate, a); //use the electricity rate data by year (also trueup) //* charged_grid[a] / charged_grid[1] * cf.at(CF_util_escal_rate, a);
                                }
                                if (d == util::days_in_month(int(m - 1)) && h == 23 && monthly_e_fromgrid[m - 1] > 0) cf.at(CF_parasitic_cost, a) += -monthly_gen_purchases[(size_t(a) - 1) * 12 + m - 1] / monthly_e_fromgrid[m - 1] * (net_annual_true_up.at(a, m - 1) + net_billing_credit.at(a, m - 1) + net_metering_credit.at(a, m - 1));
                            }
                        }
                    }
                }
            }
        }

        save_cf(CF_parasitic_cost, nyears, "cf_parasitic_cost");

        double npv_energy_real = libfin::npv(cf.row(CF_energy_sales).to_vector(), nyears, real_discount_rate);
//		if (npv_energy_real == 0.0) throw general_error("lcoe real failed because energy npv is zero");
//		double lcoe_real = -( cf.at(CF_after_tax_net_equity_cost_flow,0) + libfin::npv(CF_after_tax_net_equity_cost_flow, nyears, nom_discount_rate) ) * 100 / npv_energy_real;
		double lcoe_real = -( cf.at(CF_after_tax_net_equity_cost_flow,0) + libfin::npv(cf.row(CF_after_tax_net_equity_cost_flow).to_vector(), nyears, nom_discount_rate) - libfin::npv(cf.row(CF_parasitic_cost).to_vector(), nyears, nom_discount_rate)) * 100;
		if (npv_energy_real == 0.0) 
		{
			lcoe_real = std::numeric_limits<double>::quiet_NaN();
		}
		else
		{
			lcoe_real /= npv_energy_real;
		}

		double npv_energy_nom = libfin::npv(cf.row(CF_energy_sales).to_vector(), nyears, nom_discount_rate );
//		if (npv_energy_nom == 0.0) throw general_error("lcoe nom failed because energy npv is zero");
//		double lcoe_nom = -( cf.at(CF_after_tax_net_equity_cost_flow,0) + libfin::npv(CF_after_tax_net_equity_cost_flow, nyears, nom_discount_rate) ) * 100 / npv_energy_nom;
		double lcoe_nom = -( cf.at(CF_after_tax_net_equity_cost_flow,0) + libfin::npv(cf.row(CF_after_tax_net_equity_cost_flow).to_vector(), nyears, nom_discount_rate) - libfin::npv(cf.row(CF_parasitic_cost).to_vector(), nyears, nom_discount_rate)) * 100;
		if (npv_energy_nom == 0.0) 
		{
			lcoe_nom = std::numeric_limits<double>::quiet_NaN();
		}
		else
		{
			lcoe_nom /= npv_energy_nom;
		}

		double net_present_value = cf.at(CF_after_tax_cash_flow, 0) + libfin::npv(cf.row(CF_after_tax_cash_flow).to_vector(), nyears, nom_discount_rate);
        double irr = libfin::irr(cf.row(CF_after_tax_cash_flow).to_vector(),nyears)*100;

		double payback = libfin::payback(cf.row(CF_cumulative_payback_with_expenses).to_vector(), cf.row(CF_payback_with_expenses).to_vector(), nyears);
		// Added for Owen Zinaman for Mexico Rates and analyses 9/26/16
		//- see C:\Projects\SAM\Documentation\Payback\DiscountedPayback_2016.9.26
		double discounted_payback = libfin::payback(cf.row(CF_discounted_cumulative_payback).to_vector(), cf.row(CF_discounted_payback).to_vector(), nyears);

		// save outputs


	double npv_fed_ptc = libfin::npv(cf.row(CF_ptc_fed).to_vector(), nyears, nom_discount_rate);
	double npv_sta_ptc = libfin::npv(cf.row(CF_ptc_sta).to_vector(), nyears, nom_discount_rate);

	// TODO check this
//	npv_fed_ptc /= (1.0 - effective_tax_rate);
//	npv_sta_ptc /= (1.0 - effective_tax_rate);
	npv_fed_ptc /= (1.0 - cf.at(CF_effective_tax_frac,1));
	npv_sta_ptc /= (1.0 - cf.at(CF_effective_tax_frac, 1));

	double lcoptc_fed_nom=0.0;
	if (npv_energy_nom != 0) lcoptc_fed_nom = npv_fed_ptc / npv_energy_nom * 100.0;
	double lcoptc_fed_real=0.0;
	if (npv_energy_real != 0) lcoptc_fed_real = npv_fed_ptc / npv_energy_real * 100.0;

	double lcoptc_sta_nom=0.0;
	if (npv_energy_nom != 0) lcoptc_sta_nom = npv_sta_ptc / npv_energy_nom * 100.0;
	double lcoptc_sta_real=0.0;
	if (npv_energy_real != 0) lcoptc_sta_real = npv_sta_ptc / npv_energy_real * 100.0;

	assign("lcoptc_fed_nom", var_data((ssc_number_t) lcoptc_fed_nom));
	assign("lcoptc_fed_real", var_data((ssc_number_t) lcoptc_fed_real));
	assign("lcoptc_sta_nom", var_data((ssc_number_t) lcoptc_sta_nom));
	assign("lcoptc_sta_real", var_data((ssc_number_t) lcoptc_sta_real));

    ///////////////////////////////////////////////////////////////////////
//LCOS Calculations
    if (is_assigned("battery_total_cost_lcos") && as_double("battery_total_cost_lcos") != 0) {
        for (int y = 0; y <= nyears; y++) {
            cf_lcos.at(0, y) = cf.at(CF_battery_replacement_cost, y);
            cf_lcos.at(1, y) = cf.at(CF_battery_replacement_cost_schedule, y);
            cf_lcos.at(6, y) = cf.at(CF_om_fixed1_expense, y); //Fixed OM Battery cost
            cf_lcos.at(7, y) = cf.at(CF_om_production1_expense, y); //Produciton OM Battery cost
            cf_lcos.at(8, y) = cf.at(CF_om_capacity1_expense, y); //Capacity OM Battery Cost
        }
        int grid_charging_cost_version = 0;
        lcos_calc(this, cf_lcos, nyears, nom_discount_rate, inflation_rate, lcoe_real, total_cost, real_discount_rate, grid_charging_cost_version);
    }
    /////////////////////////////////////////////////////////////////////////////////////////

    if (as_integer("en_batt") == 1 || as_integer("en_standalone_batt") == 1) {
        update_battery_outputs(this, nyears);
    }
    update_fuelcell_outputs(this, nyears);

	double wacc = 0.0;
	wacc = (1.0 - debt_frac)*nom_discount_rate + debt_frac*loan_rate*(1.0 - cf.at(CF_effective_tax_frac, 1));

	wacc *= 100.0;
//	effective_tax_rate *= 100.0;


	assign("wacc", var_data( (ssc_number_t) wacc));
	assign("effective_tax_rate", var_data((ssc_number_t)(cf.at(CF_effective_tax_frac, 1)*100.0)));



		// NTE
		ssc_number_t *ub_w_sys = 0;
        ub_w_sys = as_array("utility_bill_w_sys", &count);
		if (count != (size_t)(nyears+1))
			throw exec_error("cashloan", util::format("utility bill with system input wrong length (%d) should be (%d)",count, nyears+1));
		ssc_number_t *ub_wo_sys = 0;
        ub_wo_sys = as_array("utility_bill_wo_sys", &count);
		if (count != (size_t)(nyears+1))
			throw exec_error("cashloan", util::format("utility bill without system input wrong length (%d) should be (%d)",count, nyears+1));

		for (i = 0; i < (int)count; i++)
			cf.at(CF_nte, i) = (double) (ub_wo_sys[i] - ub_w_sys[i]) *100.0;// $ to cents
		double lnte_real = libfin::npv(cf.row(CF_nte).to_vector(), nyears, nom_discount_rate);

		for (i = 0; i < (int)count; i++)
			if (cf.at(CF_energy_net,i) > 0) cf.at(CF_nte,i) /= cf.at(CF_energy_net,i);

		double lnte_nom = lnte_real;
		if (npv_energy_real == 0.0) 
			lnte_real = std::numeric_limits<double>::quiet_NaN();
		else
			lnte_real /= npv_energy_real;
		if (npv_energy_nom == 0.0) 
			lnte_nom = std::numeric_limits<double>::quiet_NaN();
		else
			lnte_nom /= npv_energy_nom;

		assign( "lnte_real", var_data((ssc_number_t)lnte_real) );
		assign( "lnte_nom", var_data((ssc_number_t)lnte_nom) );
		save_cf(CF_nte, nyears, "cf_nte");
		assign( "year1_nte", var_data((ssc_number_t)cf.at(CF_nte,1)) );

		assign( "cf_length", var_data( (ssc_number_t) nyears+1 ));

		assign("payback", var_data((ssc_number_t)payback));
		assign("discounted_payback", var_data((ssc_number_t)discounted_payback));
		assign("lcoe_real", var_data((ssc_number_t)lcoe_real));
		assign( "lcoe_nom", var_data((ssc_number_t)lcoe_nom) );
		assign( "npv",  var_data((ssc_number_t)net_present_value) );
        assign("irr", var_data((ssc_number_t)irr));

//		assign("first_year_energy_net", var_data((ssc_number_t) cf.at(CF_energy_net,1)));

		assign( "depr_basis_fed", var_data((ssc_number_t)federal_depr_basis ));
		assign( "depr_basis_sta", var_data((ssc_number_t)state_depr_basis ));
		assign( "nominal_discount_rate", var_data((ssc_number_t)(nom_discount_rate*100.0) ));		
//		assign( "sales_tax_deduction", var_data((ssc_number_t)total_sales_tax ));		
		assign( "adjusted_installed_cost", var_data((ssc_number_t)adjusted_installed_cost ));		
		assign( "first_cost", var_data((ssc_number_t)first_cost ));		
		assign( "total_cost", var_data((ssc_number_t)total_cost ));		
		assign( "loan_amount", var_data((ssc_number_t)loan_amount ));		
		
		save_cf(CF_energy_value, nyears, "cf_energy_value");

        save_cf(CF_energy_net, nyears, "cf_energy_net");
        save_cf(CF_energy_sales, nyears, "cf_energy_sales");
        save_cf(CF_energy_purchases, nyears, "cf_energy_purchases");

        if (is_assigned("gen_without_battery"))
        {
            save_cf(CF_energy_without_battery, nyears, "cf_energy_without_battery");
        }

		save_cf(CF_thermal_value, nyears, "cf_thermal_value");


// real estate value added 6/24/13
		for ( i=1;i<nyears+1;i++)
		{
			double rr = 1.0;
			if (nom_discount_rate != -1.0) rr = 1.0/(1.0+nom_discount_rate);
			double result = 0;
			for (int j=nyears;j>=i;j--) 
			result = rr * result + cf.at(CF_energy_value, j) + cf.at(CF_thermal_value, j);
			cf.at(CF_value_added,i) = result*rr + cf.at(CF_net_salvage_value,i);
		}
		save_cf( CF_value_added, nyears, "cf_value_added" );

		save_cf(CF_federal_tax_frac, nyears, "cf_federal_tax_frac");
		save_cf(CF_state_tax_frac, nyears, "cf_state_tax_frac");
		save_cf(CF_effective_tax_frac, nyears, "cf_effective_tax_frac");


		save_cf(CF_om_fixed_expense, nyears, "cf_om_fixed_expense");
		save_cf(CF_om_production_expense, nyears, "cf_om_production_expense");
		save_cf(CF_om_capacity_expense, nyears, "cf_om_capacity_expense");
        if (add_om_num_types > 0) {
            save_cf(CF_om_fixed1_expense, nyears, "cf_om_fixed1_expense");
            save_cf(CF_om_production1_expense, nyears, "cf_om_production1_expense");
            save_cf(CF_om_capacity1_expense, nyears, "cf_om_capacity1_expense");
        }
        if (add_om_num_types > 1) {
            save_cf(CF_om_fixed2_expense, nyears, "cf_om_fixed2_expense");
            save_cf(CF_om_production2_expense, nyears, "cf_om_production2_expense");
            save_cf(CF_om_capacity2_expense, nyears, "cf_om_capacity2_expense");
        }
		save_cf( CF_om_fuel_expense, nyears, "cf_om_fuel_expense" );
		save_cf( CF_om_opt_fuel_1_expense, nyears, "cf_om_opt_fuel_1_expense" );
		save_cf( CF_om_opt_fuel_2_expense, nyears, "cf_om_opt_fuel_2_expense" );
		save_cf( CF_property_tax_assessed_value, nyears, "cf_property_tax_assessed_value" );
		save_cf( CF_property_tax_expense, nyears, "cf_property_tax_expense" );
		save_cf( CF_insurance_expense, nyears, "cf_insurance_expense" );
		save_cf( CF_net_salvage_value, nyears, "cf_net_salvage_value" );
        if (as_integer("en_batt") == 1 || as_integer("en_standalone_batt") == 1) {
            save_cf(CF_battery_replacement_cost, nyears, "cf_battery_replacement_cost");
            save_cf(CF_battery_replacement_cost_schedule, nyears, "cf_battery_replacement_cost_schedule");
        }
        if (is_assigned("fuelcell_replacement_option")) {
            save_cf(CF_fuelcell_replacement_cost, nyears, "cf_fuelcell_replacement_cost");
            save_cf(CF_fuelcell_replacement_cost_schedule, nyears, "cf_fuelcell_replacement_cost_schedule");
        }
		save_cf(CF_operating_expenses, nyears, "cf_operating_expenses");

		save_cf( CF_deductible_expenses, nyears, "cf_deductible_expenses");
		
		save_cf( CF_debt_balance, nyears, "cf_debt_balance" );
		save_cf( CF_debt_payment_interest, nyears, "cf_debt_payment_interest" );
		save_cf( CF_debt_payment_principal, nyears, "cf_debt_payment_principal" );
		save_cf( CF_debt_payment_total, nyears, "cf_debt_payment_total" );

		assign( "ibi_total_fed", var_data((ssc_number_t) (ibi_fed_amount + ibi_fed_per)));
		assign( "ibi_total_sta", var_data((ssc_number_t) (ibi_sta_amount + ibi_sta_per)));
		assign( "ibi_total_uti", var_data((ssc_number_t) (ibi_uti_amount + ibi_uti_per)));
		assign( "ibi_total_oth", var_data((ssc_number_t) (ibi_oth_amount + ibi_oth_per)));
		assign( "ibi_total", var_data((ssc_number_t) ibi_total));

		assign( "cbi_total_fed", var_data((ssc_number_t) (cbi_fed_amount)));
		assign( "cbi_total_sta", var_data((ssc_number_t) (cbi_sta_amount)));
		assign( "cbi_total_uti", var_data((ssc_number_t) (cbi_uti_amount)));
		assign( "cbi_total_oth", var_data((ssc_number_t) (cbi_oth_amount)));
		assign( "cbi_total", var_data((ssc_number_t) cbi_total));
		
		
		save_cf( CF_pbi_fed, nyears, "cf_pbi_total_fed" );
		save_cf( CF_pbi_sta, nyears, "cf_pbi_total_sta" );
		save_cf( CF_pbi_uti, nyears, "cf_pbi_total_uti" );
		save_cf( CF_pbi_oth, nyears, "cf_pbi_total_oth" );
		save_cf( CF_pbi_total, nyears, "cf_pbi_total" );
	
		save_cf( CF_ptc_fed, nyears, "cf_ptc_fed" );
		save_cf( CF_ptc_sta, nyears, "cf_ptc_sta" );


        // SAM 1038
        double itc_fed_total = 0.0;
        double itc_sta_total = 0.0;
        double itc_total = 0.0;

        for (size_t k = 0; k <= nyears; k++) {
            itc_fed_total += cf.at(CF_itc_fed, k);
            itc_sta_total += cf.at(CF_itc_sta, k);
            itc_total += cf.at(CF_itc_total, k);
        }
        assign("itc_total_fed", var_data((ssc_number_t)itc_fed_total));
        assign("itc_total_sta", var_data((ssc_number_t)itc_sta_total));
        assign("itc_total", var_data((ssc_number_t)itc_total));

		save_cf( CF_sta_depr_sched, nyears, "cf_sta_depr_sched" );
		save_cf( CF_sta_depreciation, nyears, "cf_sta_depreciation" );
		save_cf( CF_sta_incentive_income_less_deductions, nyears, "cf_sta_incentive_income_less_deductions" );
		save_cf( CF_sta_taxable_income_less_deductions, nyears, "cf_sta_taxable_income_less_deductions" );
		save_cf( CF_sta_tax_savings, nyears, "cf_sta_tax_savings" );
	
		save_cf( CF_sta_taxable_incentive_income, nyears, "cf_sta_taxable_incentive_income");
		save_cf( CF_fed_taxable_incentive_income, nyears, "cf_fed_taxable_incentive_income");

		save_cf( CF_fed_depr_sched, nyears, "cf_fed_depr_sched" );
		save_cf( CF_fed_depreciation, nyears, "cf_fed_depreciation" );
		save_cf( CF_fed_incentive_income_less_deductions, nyears, "cf_fed_incentive_income_less_deductions" );
		save_cf( CF_fed_taxable_income_less_deductions, nyears, "cf_fed_taxable_income_less_deductions" );
		save_cf( CF_fed_tax_savings, nyears, "cf_fed_tax_savings" );

		save_cf( CF_sta_and_fed_tax_savings, nyears, "cf_sta_and_fed_tax_savings" );
		save_cf( CF_after_tax_net_equity_cost_flow, nyears, "cf_after_tax_net_equity_cost_flow" );
		save_cf( CF_after_tax_cash_flow, nyears, "cf_after_tax_cash_flow" );

		save_cf( CF_payback_with_expenses, nyears, "cf_payback_with_expenses" );
		save_cf( CF_cumulative_payback_with_expenses, nyears, "cf_cumulative_payback_with_expenses" );

		// For Owen and discounted payback period
		save_cf(CF_discounted_costs, nyears, "cf_discounted_costs");
		save_cf(CF_discounted_savings, nyears, "cf_discounted_savings");
		save_cf(CF_discounted_payback, nyears, "cf_discounted_payback");
		save_cf(CF_discounted_cumulative_payback, nyears, "cf_discounted_cumulative_payback");

		save_cf( CF_payback_without_expenses, nyears, "cf_payback_without_expenses" );
		save_cf( CF_cumulative_payback_without_expenses, nyears, "cf_cumulative_payback_without_expenses" );

	// for cost stacked bars
		//libfin::npv(CF_energy_value, nyears, nom_discount_rate)
		// present value of o and m value - note - present value is distributive - sum of pv = pv of sum
		double pvAnnualOandM = libfin::npv(cf.row(CF_om_fixed_expense).to_vector(), nyears, nom_discount_rate);
		double pvFixedOandM = libfin::npv(cf.row(CF_om_capacity_expense).to_vector(), nyears, nom_discount_rate);
		double pvVariableOandM = libfin::npv(cf.row(CF_om_production_expense).to_vector(), nyears, nom_discount_rate);
		double pvFuelOandM = libfin::npv(cf.row(CF_om_fuel_expense).to_vector(), nyears, nom_discount_rate);
		double pvOptFuel1OandM = libfin::npv(cf.row(CF_om_opt_fuel_1_expense).to_vector(), nyears, nom_discount_rate);
		double pvOptFuel2OandM = libfin::npv(cf.row(CF_om_opt_fuel_2_expense).to_vector(), nyears, nom_discount_rate);
	//	double pvWaterOandM = NetPresentValue(sv[svNominalDiscountRate], cf[cfAnnualWaterCost], analysis_period);

		assign( "present_value_oandm",  var_data((ssc_number_t)(pvAnnualOandM + pvFixedOandM + pvVariableOandM + pvFuelOandM))); // + pvWaterOandM);

		assign( "present_value_oandm_nonfuel", var_data((ssc_number_t)(pvAnnualOandM + pvFixedOandM + pvVariableOandM)));
		assign( "present_value_fuel", var_data((ssc_number_t)(pvFuelOandM + pvOptFuel1OandM + pvOptFuel2OandM)));

		// present value of insurance and property tax
		double pvInsurance = libfin::npv(cf.row(CF_insurance_expense).to_vector(), nyears, nom_discount_rate);
		double pvPropertyTax = libfin::npv(cf.row(CF_property_tax_expense).to_vector(), nyears, nom_discount_rate);

		assign( "present_value_insandproptax", var_data((ssc_number_t)(pvInsurance + pvPropertyTax)));


        // SAM 1038
        save_cf(CF_itc_fed_amount, nyears, "cf_itc_fed_amount");
        save_cf(CF_itc_fed_percent_amount, nyears, "cf_itc_fed_percent_amount");
        save_cf(CF_itc_fed, nyears, "cf_itc_fed");
        save_cf(CF_itc_sta_amount, nyears, "cf_itc_sta_amount");
        save_cf(CF_itc_sta_percent_amount, nyears, "cf_itc_sta_percent_amount");
        save_cf(CF_itc_sta, nyears, "cf_itc_sta");
        save_cf(CF_itc_total, nyears, "cf_itc_total");


	}

/* These functions can be placed in common financial library with matrix and constants passed? */

	void save_cf(int cf_line, int nyears, const std::string &name)
	{
		ssc_number_t *arrp = allocate( name, nyears+1 );
		for (int i=0;i<=nyears;i++)
			arrp[i] = (ssc_number_t)cf.at(cf_line, i);
	}
    /*
	double compute_payback( int cf_cpb, int cf_pb, int nyears )
	{	
		// may need to determine last negative to positive transition for high replacement costs - see C:\Projects\SAM\Documentation\FinancialIssues\Payback_2015.9.8
		double dPayback = std::numeric_limits<double>::quiet_NaN(); // report as > analysis period
		bool bolPayback = false;
		int iPayback = 0;
		int i = 1; 
		while ((i<=nyears) && (!bolPayback))
		{
			if (cf.at(cf_cpb,i) > 0)
			{
				bolPayback = true;
				iPayback = i;
			}
			i++;
		}

		if (bolPayback)
		{
			dPayback = iPayback;
			if (cf.at(cf_pb, iPayback) != 0.0)
				dPayback -= cf.at(cf_cpb,iPayback) / cf.at(cf_pb,iPayback);
		}

		return dPayback;
	}
    */
	void compute_production_incentive( int cf_line, int nyears, const std::string &s_val, const std::string &s_term, const std::string &s_escal )
	{
		size_t len = 0;
		ssc_number_t *parr = as_array(s_val, &len);
		int term = as_integer(s_term);
		double escal = as_double(s_escal)/100.0;

		if (len == 1)
		{
			for (int i=1;i<=nyears;i++)
				cf.at(cf_line, i) = (i <= term) ? parr[0] * cf.at(CF_energy_sales,i) * pow(1 + escal, i-1) : 0.0;
		}
		else
		{
			for (int i=1;i<=nyears && i <= (int)len;i++)
				cf.at(cf_line, i) = parr[i-1]*cf.at(CF_energy_sales,i);
		}
	}

		void compute_production_incentive_IRS_2010_37( int cf_line, int nyears, const std::string &s_val, const std::string &s_term, const std::string &s_escal )
	{
		// rounding based on IRS document and emails 2/24/2011
		size_t len = 0;
		ssc_number_t *parr = as_array(s_val, &len);
		int term = as_integer(s_term);
		double escal = as_double(s_escal)/100.0;

		if (len == 1)
		{
			for (int i=1;i<=nyears;i++)
				cf.at(cf_line, i) = (i <= term) ? cf.at(CF_energy_sales,i) / 1000.0 * libfin::round_irs(1000.0 * parr[0] * pow(1 + escal, i-1)) : 0.0;
		}
		else
		{
			for (int i=1;i<=nyears && i <= (int)len;i++)
				cf.at(cf_line, i) = parr[i-1]*cf.at(CF_energy_sales,i);
		}
	}


	void single_or_schedule( int cf_line, int nyears, double scale, const std::string &name )
	{
		size_t len = 0;
		ssc_number_t *p = as_array(name, &len);
		for (int i=1;i<=(int)len && i <= nyears;i++)
			cf.at(cf_line, i) = scale*p[i-1];
	}
	
	void single_or_schedule_check_max( int cf_line, int nyears, double scale, const std::string &name, const std::string &maxvar )
	{
		double max = as_double(maxvar);
		size_t len = 0;
		ssc_number_t *p = as_array(name, &len);
		for (int i=1;i<=(int)len && i <= nyears;i++)
			cf.at(cf_line, i) = libfin::min( scale*p[i-1], max );
	}

	double taxable_incentive_income(int year, const std::string &fed_or_sta)
	{
		double ti = 0.0;
		if (year==1) 
		{
			if ( as_boolean("ibi_fed_amount_tax_"+fed_or_sta) ) ti += ibi_fed_amount;
			if ( as_boolean("ibi_sta_amount_tax_"+fed_or_sta) ) ti += ibi_sta_amount;
			if ( as_boolean("ibi_uti_amount_tax_"+fed_or_sta) ) ti += ibi_uti_amount;
			if ( as_boolean("ibi_oth_amount_tax_"+fed_or_sta) ) ti += ibi_oth_amount;
		
			if ( as_boolean("ibi_fed_percent_tax_"+fed_or_sta) ) ti += ibi_fed_per;
			if ( as_boolean("ibi_sta_percent_tax_"+fed_or_sta) ) ti += ibi_sta_per;
			if ( as_boolean("ibi_uti_percent_tax_"+fed_or_sta) ) ti += ibi_uti_per;
			if ( as_boolean("ibi_oth_percent_tax_"+fed_or_sta) ) ti += ibi_oth_per;

			if ( as_boolean("cbi_fed_tax_"+fed_or_sta) ) ti += cbi_fed_amount;
			if ( as_boolean("cbi_sta_tax_"+fed_or_sta) ) ti += cbi_sta_amount;
			if ( as_boolean("cbi_uti_tax_"+fed_or_sta) ) ti += cbi_uti_amount;
			if ( as_boolean("cbi_oth_tax_"+fed_or_sta) ) ti += cbi_oth_amount;
		}

		if ( as_boolean("pbi_fed_tax_"+fed_or_sta) ) ti += cf.at( CF_pbi_fed, year );
		if ( as_boolean("pbi_sta_tax_"+fed_or_sta) ) ti += cf.at( CF_pbi_sta, year );
		if ( as_boolean("pbi_uti_tax_"+fed_or_sta) ) ti += cf.at( CF_pbi_uti, year );
		if ( as_boolean("pbi_oth_tax_"+fed_or_sta) ) ti += cf.at( CF_pbi_oth, year );

		return ti;
	}
	
	void depreciation_sched_macrs_half_year( int cf_line, int nyears )
	{
		for (int i=1; i<=nyears; i++)
		{
			double factor = 0.0;
			switch(i)
			{
			case 1: factor = 0.2000; break;
			case 2: factor = 0.3200; break;
			case 3: factor = 0.1920; break;
			case 4: factor = 0.1152; break;
			case 5: factor = 0.1152; break;
			case 6: factor = 0.0576; break;
			default: factor = 0.0; break;
			}
			cf.at(cf_line, i) = factor;
		}
	}

	void depreciation_sched_straight_line( int cf_line, int nyears, int slyears )
	{
		double depr_per_year = (slyears!=0) ? 1.0 / ((double)slyears) : 0;
		for (int i=1; i<=nyears; i++)
			cf.at(cf_line, i) = (i<=slyears) ? depr_per_year : 0.0;
	}
	
	void depreciation_sched_custom( int cf_line, int nyears, ssc_number_t *custp, int custp_len )
	{
		// note - allows for greater than or less than 100% depreciation - warning to user in samsim
		if (custp_len < 2)
		{
			cf.at(cf_line, 1) = custp[0]/100.0;
		}
		else
		{
			for (int i=1;i<=nyears && i-1 < custp_len;i++)
				cf.at(cf_line, i) = custp[i-1]/100.0;
		}
	}

	void escal_or_annual( int cf_line, int nyears, const std::string &variable, 
			double inflation_rate, double scale, bool as_rate=true, double escal = 0.0)
	{
		size_t count;
		ssc_number_t *arrp = as_array(variable, &count);

		if (as_rate)
		{
			if (count == 1)
			{
				escal = inflation_rate + scale*arrp[0];
				for (int i=0; i < nyears; i++)
					cf.at(cf_line, i+1) = pow( 1+escal, i );
			}
			else
			{
				for (int i=0; i < nyears && i < (int)count; i++)
					cf.at(cf_line, i+1) = 1 + arrp[i]*scale;
			}
		}
		else
		{
			if (count == 1)
			{
				for (int i=0;i<nyears;i++)
					cf.at(cf_line, i+1) = arrp[0]*scale*pow( 1+escal+inflation_rate, i );
			}
			else
			{
				for (int i=0;i<nyears && i<(int)count;i++)
					cf.at(cf_line, i+1) = arrp[i]*scale;
			}
		}
	}

};

DEFINE_MODULE_ENTRY( cashloan, "Residential/Commerical Finance model.", 1 );
