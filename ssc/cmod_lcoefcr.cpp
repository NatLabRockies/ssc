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

#include "core.h"
#include "lib_financial.h"

#include <numeric>

static var_info vtab_lcoefcr[] = 
{	
/*   VARTYPE            DATATYPE         NAME                        LABEL                             UNITS     META      GROUP          REQUIRED_IF    CONSTRAINTS UI_HINTS*/
	{ SSC_INPUT,        SSC_NUMBER,      "capital_cost",             "Capital cost",                   "$",      "",       "Simple LCOE", "*",           "",         "" },
	{ SSC_INPUT,        SSC_NUMBER,      "fixed_operating_cost",     "Annual fixed operating cost",    "$",      "",       "Simple LCOE", "*",           "",         "" },
	{ SSC_INPUT,        SSC_NUMBER,      "variable_operating_cost",  "Annual variable operating cost", "$/kWh",  "",       "Simple LCOE", "*",           "",         "" },
	{ SSC_INPUT,        SSC_NUMBER,      "fixed_charge_rate",        "Fixed charge rate",              "",       "",       "Simple LCOE", "*",           "",         "" },
	{ SSC_INPUT,        SSC_NUMBER,      "annual_energy",            "Annual energy production",       "kWh",    "",       "Simple LCOE",          "*",           "",         "" },
	
	{ SSC_OUTPUT,       SSC_NUMBER,      "lcoe_fcr",                 "LCOE Levelized cost of energy",       "$/kWh",  "",	   "Simple LCOE",          "*",           "",         "" },
var_info_invalid };

class cm_lcoefcr : public compute_module
{
private:
public:
	cm_lcoefcr()
	{
		add_var_info( vtab_lcoefcr );
	}

	void exec( )
	{
		double aep = 1; // annual output, get from performance model
		double fcr = 0; // fixed charge rate, before tax revenues required
		double icc = 0; // initial investment, or capital cost
		double voc = 0; // variable operating cost
		double foc = 0; // fixed operating cost

		aep = as_double("annual_energy");           // kWh
		foc = as_double("fixed_operating_cost");    // $
		voc = as_double("variable_operating_cost"); // $/kWh
		fcr = as_double("fixed_charge_rate");       // unitless fraction
		icc = as_double("capital_cost");            // $
		
		double lcoe = (fcr*icc + foc) / aep + voc; //$/kWh

		assign("lcoe_fcr", var_data((ssc_number_t)lcoe));
	}


};

DEFINE_MODULE_ENTRY( lcoefcr, "Calculate levelized cost of energy using fixed charge rate method.", 1 )


static var_info vtab_lcoefcr_design[] =
{
    /*   VARTYPE            DATATYPE         NAME                        LABEL                                     UNITS     META      GROUP                   REQUIRED_IF             CONSTRAINTS UI_HINTS*/
    { SSC_INPUT,        SSC_NUMBER,      "sim_type",                 "1 (default): timeseries, 2: design only",    "",       "",       "System Control",       "?=1",                  "",         "SIMULATION_PARAMETER"},
                                                                                                                                                                                       
    { SSC_INPUT,        SSC_NUMBER,      "ui_fcr_input_option",         "0: fixed charge rate; 1: calculate",         "",       "",       "Simple LCOE",          "*",                    "",         ""},

        // FCR Input Option = 0: Fixed fixed charge rate
    { SSC_INPUT,        SSC_NUMBER,      "ui_fixed_charge_rate",     "Input fixed charge rate",                    "",       "",       "Simple LCOE",          "ui_fcr_input_option=0",   "",         ""},

        // FCR Input Option = 1: Calculated fixed charge rate
    { SSC_INPUT,        SSC_NUMBER,      "c_inflation",              "Input fixed charge rate",                    "%",      "",       "Simple LCOE",          "ui_fcr_input_option=1",   "",         ""},
    { SSC_INPUT,        SSC_NUMBER,      "c_equity_return",          "IRR (nominal)",                              "%",      "",       "Simple LCOE",          "ui_fcr_input_option=1",   "",         ""},
    { SSC_INPUT,        SSC_NUMBER,      "c_debt_percent",           "Project term debt (% of capital)",           "%",      "",       "Simple LCOE",          "ui_fcr_input_option=1",   "",         ""},
    { SSC_INPUT,        SSC_NUMBER,      "c_nominal_interest_rate",  "Nominal debt interest rate",                 "%",      "",       "Simple LCOE",          "ui_fcr_input_option=1",   "",         ""},
    { SSC_INPUT,        SSC_NUMBER,      "c_tax_rate",               "Effective tax rate",                         "%",      "",       "Simple LCOE",          "ui_fcr_input_option=1",   "",         ""},
    { SSC_INPUT,        SSC_NUMBER,      "c_lifetime",               "Analysis period",                            "years",  "",       "Simple LCOE",          "ui_fcr_input_option=1",   "",         ""},
    { SSC_INPUT,        SSC_ARRAY,       "c_depreciation_schedule",  "Depreciation schedule",                      "%",      "",       "Simple LCOE",          "ui_fcr_input_option=1",   "",         ""},
    { SSC_INPUT,        SSC_NUMBER,      "c_construction_interest",  "Nominal construction interest rate",         "%",      "",       "Simple LCOE",          "ui_fcr_input_option=1",   "",         ""},
    { SSC_INPUT,        SSC_ARRAY,       "c_construction_cost",      "Construction cost schedule",                 "%",      "",       "Simple LCOE",          "ui_fcr_input_option=1",   "",         ""},

        // General Inputs

        // "Performance" Inputs
    { SSC_INPUT,        SSC_NUMBER,      "total_installed_cost",     "Total installed cost",                                        "$",       "",       "System Costs",  "sim_type=1",     "",        "SIMULATION_PARAMETER" },
                                                                                                                                                                                           
    { SSC_INPUT,        SSC_NUMBER,      "annual_electricity_consumption","Annual electricity consumption with avail derate",       "kWe-hr",  "",       "IPH LCOH",      "sim_type=1",     "",        "SIMULATION_PARAMETER" },
    { SSC_INPUT,        SSC_NUMBER,      "electricity_rate",              "Cost of electricity used to operate pumps and trackers", "$/kWe-hr","",       "IPH LCOH",      "sim_type=1",     "",        "SIMULATION_PARAMETER" },
    { SSC_INPUT,        SSC_NUMBER,      "fixed_operating_cost",     "Annual fixed operating cost",                                 "$",       "",       "Simple LCOE",   "sim_type=1",     "",        "SIMULATION_PARAMETER" },

    { SSC_INPUT,        SSC_NUMBER,      "variable_operating_cost",  "Annual variable operating cost",             "$/kWh",  "",       "Simple LCOE",          "sim_type=1",               "",         "SIMULATION_PARAMETER" },
    { SSC_INPUT,        SSC_NUMBER,      "annual_energy",            "Annual energy production",                   "kWh",    "",       "Simple LCOE",          "sim_type=1",               "",         "SIMULATION_PARAMETER" },

    // "Design" outputs
    { SSC_OUTPUT,       SSC_NUMBER,      "crf",                      "Capital recovery factor",                    "",       "",	   "Simple LCOE",          "*",                        "",         "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "pfin",                     "Project financing factor",                   "",       "",	   "Simple LCOE",          "*",                        "",         "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "cfin",                     "Construction financing factor",              "",       "",	   "Simple LCOE",          "*",                        "",         "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "wacc",                     "WACC",                                       "",       "",	   "Simple LCOE",          "*",                        "",         "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "fixed_charge_rate_calc",   "Calculated fixed charge rate",               "",       "",	   "Simple LCOE",          "*",                        "",         "" },

    { SSC_OUTPUT,       SSC_NUMBER,      "lcoe_fcr",                 "LCOE Levelized cost of energy",              "$/kWh",  "",	   "Simple LCOE",          "sim_type=1",               "",         "" },

    // "Performance" outputs

    var_info_invalid };

class cm_lcoefcr_design : public compute_module
{
private:
public:
    cm_lcoefcr_design()
    {
        add_var_info(vtab_lcoefcr_design);
    }

    void exec()
    {
        int fcr_input_option = as_integer("ui_fcr_input_option");

        double fixed_charge_rate = std::numeric_limits<double>::quiet_NaN();
        double crf = std::numeric_limits<double>::quiet_NaN();
        double pfin = std::numeric_limits<double>::quiet_NaN();
        double cfin = std::numeric_limits<double>::quiet_NaN();
        double wacc = std::numeric_limits<double>::quiet_NaN();
        if (fcr_input_option == 0) {
            fixed_charge_rate = as_double("ui_fixed_charge_rate");
        }
        else {
            double i = as_double("c_inflation") / 100.0;
            double nroe = as_double("c_equity_return") / 100.0;
            double rroe = (1. + nroe) / (1. + i) - 1.0;         // real return on equity
            double df = as_double("c_debt_percent") / 100.0;
            double inom = as_double("c_nominal_interest_rate") / 100.0;
            double ireal = (1. + inom) / (1. + i) - 1.0;        // real interest rate
            double tr = as_double("c_tax_rate") / 100.0;
            wacc = ((1. + ((1. - df) * ((1. + rroe) * (1. + i) - 1.)) +
                (df * ((1. + ireal) * (1. + i) - 1.) * (1. - tr))) / (1. + i)) - 1.;

            double t = as_double("c_lifetime");
            crf = wacc / (1.0 - (1.0 / std::pow((1.0 + wacc), t))); // real crf

            std::vector<double> dep = as_vector_double("c_depreciation_schedule");
            int n_dep = dep.size();
            std::vector<double> arr_dep(n_dep);
            for (int iii = 0; iii < n_dep; iii++) {
                arr_dep[iii] = dep[iii] / 100.0 * (1.0 / std::pow((1.+wacc)*(1.+i), iii+1.0));
            }

            double pvd = std::accumulate(arr_dep.begin(), arr_dep.end(), 0.0);
            pfin = (1.0 - tr*pvd) / (1.0 - tr);

            double cint = as_double("c_construction_interest") / 100.0;
            std::vector<double> ccon = as_vector_double("c_construction_cost");
            int n_con = ccon.size();
            std::vector<double> arr_con(n_con);
            for (int iii = 0; iii < n_con; iii++) {
                arr_con[iii] = ccon[iii] / 100.0 * (1.0 + (1.0 - tr)*(std::pow(1. + cint, iii + 0.5)-1.0));
            }

            cfin = std::accumulate(arr_con.begin(), arr_con.end(), 0.0);
            fixed_charge_rate = crf * pfin * cfin;

        }

        assign("fixed_charge_rate_calc", fixed_charge_rate);
        assign("crf", crf);
        assign("pfin", pfin);
        assign("cfin", cfin);
        assign("wacc", wacc);

        // *****************************************************
        // If calling cmod to run design only, return here
        if (as_integer("sim_type") != 1) {
            return;
        }
        // *****************************************************
        // *****************************************************

        double aep = as_double("annual_energy");           // kWh annual output, get from performance model
        double foc_in = as_double("fixed_operating_cost");    // $
        double voc = as_double("variable_operating_cost"); // $/kWh
        double icc = as_double("total_installed_cost");            // $

        double electricity_rate = as_number("electricity_rate");                                //[$/kWe-hr]
        double annual_electricity_consumption = as_number("annual_electricity_consumption");    //[kWe-hr]
        double annual_electricity_cost = electricity_rate*annual_electricity_consumption;       //[$]

        double foc = foc_in + annual_electricity_cost;      //[$]

        double lcoe = (fixed_charge_rate * icc + foc) / aep + voc; //$/kWh

        assign("lcoe_fcr", var_data((ssc_number_t)lcoe));

        // For reference: code from UI page 7/25/23
        /*
        if (${ ui_fcr_input_option } == 0)
            ${ fixed_charge_rate } = ${ ui_fixed_charge_rate };
        else {
            i = ${ c_inflation } / 100;
            nroe = ${ c_equity_return } / 100;
            rroe = (1 + nroe) / (1 + i) - 1; // real return on equity
            df = ${ c_debt_percent } / 100;
            inom = ${ c_nominal_interest_rate } / 100;
            ireal = (1 + inom) / (1 + i) - 1; // real interest rate 
            tr = ${ c_tax_rate } / 100;
            wacc = ((1 + ((1 - df) * ((1 + rroe) * (1 + i) - 1)) +
                (df * ((1 + ireal) * (1 + i) - 1) * (1 - tr))) / (1 + i)) - 1; // real wacc
            ${ ui_wacc } = wacc;
            t = ${ c_lifetime };
            crf = wacc / (1 - (1 / (1 + wacc) ^ t)); // real crf
            dep = ${ c_depreciation_schedule };
            arr = alloc(#dep);
            for (n = 0; n < #arr; n++)
                arr[n] = dep[n] / 100 * (1 / ((1 + wacc) * (1 + i)) ^ (n + 1));
            pvd = sum(arr);
            pfin = (1 - tr * pvd) / (1 - tr);
            ccon = ${ c_construction_cost };
            cint = ${ c_construction_interest } / 100;
            arr = alloc(#ccon);
            for (n = 0; n < #arr; n++)
                arr[n] = ccon[n] / 100 * (1 + (1 - tr) * ((1 + cint) ^ (n + 0.5) - 1));
            cfin = sum(arr);
            fcr = crf * pfin * cfin;
            ${ ui_crf } = crf;
            ${ ui_pfin } = pfin;
            ${ ui_cfin } = cfin;
            ${ ui_wacc } = wacc;
            ${ ui_ireal } = ireal;
            ${ fixed_charge_rate } = fcr;
        }

        */

    }

};

DEFINE_MODULE_ENTRY(lcoefcr_design, "Calculate levelized cost of energy using fixed charge rate method.", 1)
