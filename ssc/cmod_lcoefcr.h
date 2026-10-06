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

#ifndef _CMOD_LCOEFCR_H_
#define _CMOD_LCOEFCR_H_

#include "sscapi.h"

#include "core.h"

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


static var_info vtab_lcoefcr_elec_consumption_inputs[] =
{

    { SSC_INPUT,        SSC_NUMBER,      "annual_electricity_consumption","Annual electricity consumption with avail derate",       "kWe-hr",  "",       "IPH LCOH",      "sim_type=1",     "",        "SIMULATION_PARAMETER" },
    { SSC_INPUT,        SSC_NUMBER,      "electricity_rate",              "Cost of electricity used to operate pumps and trackers", "$/kWe-hr","",       "IPH LCOH",      "sim_type=1",     "",        "SIMULATION_PARAMETER" },

    var_info_invalid
};



#endif
