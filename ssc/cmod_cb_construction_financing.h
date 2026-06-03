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

#ifndef _CMOD_CONST_FIN_H_
#define _CMOD_CONST_FIN_H_

#include "core.h"
#include "sscapi.h"

static var_info _cm_vtab_cb_construction_financing_independent[] = {
    
    //{ SSC_INPUT,  SSC_NUMBER,   "total_installed_cost",                "Total installed cost",                        "$",    "",     "system costs",           "",   "",  "" },
    { SSC_INPUT,  SSC_NUMBER,   "const_per_interest_rate1",            "Interest rate, loan 1",                       "%",    "",     "financial parameters",   "",   "",  "" },
    { SSC_INPUT,  SSC_NUMBER,   "const_per_interest_rate2",            "Interest rate, loan 2",                       "%",    "",     "financial parameters",   "",   "",  "" },
    { SSC_INPUT,  SSC_NUMBER,   "const_per_interest_rate3",            "Interest rate, loan 3",                       "%",    "",     "financial parameters",   "",   "",  "" },
    { SSC_INPUT,  SSC_NUMBER,   "const_per_interest_rate4",            "Interest rate, loan 4",                       "%",    "",     "financial parameters",   "",   "",  "" },
    { SSC_INPUT,  SSC_NUMBER,   "const_per_interest_rate5",            "Interest rate, loan 5",                       "%",    "",     "financial parameters",   "",   "",  "" },
    { SSC_INPUT,  SSC_NUMBER,   "const_per_months1",                   "Months prior to operation, loan 1",           "",     "",     "financial parameters",   "",   "",  "" },
    { SSC_INPUT,  SSC_NUMBER,   "const_per_months2",                   "Months prior to operation, loan 2",           "",     "",     "financial parameters",   "",   "",  "" },
    { SSC_INPUT,  SSC_NUMBER,   "const_per_months3",                   "Months prior to operation, loan 3",           "",     "",     "financial parameters",   "",   "",  "" },
    { SSC_INPUT,  SSC_NUMBER,   "const_per_months4",                   "Months prior to operation, loan 4",           "",     "",     "financial parameters",   "",   "",  "" },
    { SSC_INPUT,  SSC_NUMBER,   "const_per_months5",                   "Months prior to operation, loan 5",           "",     "",     "financial parameters",   "",   "",  "" },
    { SSC_INPUT,  SSC_NUMBER,   "const_per_percent1",                  "Percent of tot. installed cost, loan 1",      "%",    "",     "financial parameters",   "",   "",  "" },
    { SSC_INPUT,  SSC_NUMBER,   "const_per_percent2",                  "Percent of tot. installed cost, loan 2",      "%",    "",     "financial parameters",   "",   "",  "" },
    { SSC_INPUT,  SSC_NUMBER,   "const_per_percent3",                  "Percent of tot. installed cost, loan 3",      "%",    "",     "financial parameters",   "",   "",  "" },
    { SSC_INPUT,  SSC_NUMBER,   "const_per_percent4",                  "Percent of tot. installed cost, loan 4",      "%",    "",     "financial parameters",   "",   "",  "" },
    { SSC_INPUT,  SSC_NUMBER,   "const_per_percent5",                  "Percent of tot. installed cost, loan 5",      "%",    "",     "financial parameters",   "",   "",  "" },
    { SSC_INPUT,  SSC_NUMBER,   "const_per_upfront_rate1",             "Upfront fee on principal, loan 1",            "%",    "",     "financial parameters",   "",   "",  "" },
    { SSC_INPUT,  SSC_NUMBER,   "const_per_upfront_rate2",             "Upfront fee on principal, loan 2",            "%",    "",     "financial parameters",   "",   "",  "" },
    { SSC_INPUT,  SSC_NUMBER,   "const_per_upfront_rate3",             "Upfront fee on principal, loan 3",            "%",    "",     "financial parameters",   "",   "",  "" },
    { SSC_INPUT,  SSC_NUMBER,   "const_per_upfront_rate4",             "Upfront fee on principal, loan 4",            "%",    "",     "financial parameters",   "",   "",  "" },
    { SSC_INPUT,  SSC_NUMBER,   "const_per_upfront_rate5",             "Upfront fee on principal, loan 5",            "%",    "",     "financial parameters",   "",   "",  "" },

    { SSC_OUTPUT, SSC_NUMBER,   "const_per_principal1",                "Principal, loan 1",                           "$",    "",     "financial parameters",   "",   "",  "" },
    { SSC_OUTPUT, SSC_NUMBER,   "const_per_principal2",                "Principal, loan 2",                           "$",    "",     "financial parameters",   "",   "",  "" },
    { SSC_OUTPUT, SSC_NUMBER,   "const_per_principal3",                "Principal, loan 3",                           "$",    "",     "financial parameters",   "",   "",  "" },
    { SSC_OUTPUT, SSC_NUMBER,   "const_per_principal4",                "Principal, loan 4",                           "$",    "",     "financial parameters",   "",   "",  "" },
    { SSC_OUTPUT, SSC_NUMBER,   "const_per_principal5",                "Principal, loan 5",                           "$",    "",     "financial parameters",   "",   "",  "" },
    { SSC_OUTPUT, SSC_NUMBER,   "const_per_interest1",                 "Interest cost, loan 1",                       "$",    "",     "financial parameters",   "",   "",  "" },
    { SSC_OUTPUT, SSC_NUMBER,   "const_per_interest2",                 "Interest cost, loan 2",                       "$",    "",     "financial parameters",   "",   "",  "" },
    { SSC_OUTPUT, SSC_NUMBER,   "const_per_interest3",                 "Interest cost, loan 3",                       "$",    "",     "financial parameters",   "",   "",  "" },
    { SSC_OUTPUT, SSC_NUMBER,   "const_per_interest4",                 "Interest cost, loan 4",                       "$",    "",     "financial parameters",   "",   "",  "" },
    { SSC_OUTPUT, SSC_NUMBER,   "const_per_interest5",                 "Interest cost, loan 5",                       "$",    "",     "financial parameters",   "",   "",  "" },
    { SSC_OUTPUT, SSC_NUMBER,   "const_per_total1",                    "Total financing cost, loan 1",                "$",    "",     "financial parameters",   "",   "",  "" },
    { SSC_OUTPUT, SSC_NUMBER,   "const_per_total2",                    "Total financing cost, loan 2",                "$",    "",     "financial parameters",   "",   "",  "" },
    { SSC_OUTPUT, SSC_NUMBER,   "const_per_total3",                    "Total financing cost, loan 3",                "$",    "",     "financial parameters",   "",   "",  "" },
    { SSC_OUTPUT, SSC_NUMBER,   "const_per_total4",                    "Total financing cost, loan 4",                "$",    "",     "financial parameters",   "",   "",  "" },
    { SSC_OUTPUT, SSC_NUMBER,   "const_per_total5",                    "Total financing cost, loan 5",                "$",    "",     "financial parameters",   "",   "",  "" },
    { SSC_OUTPUT, SSC_NUMBER,   "const_per_percent_total",	           "Total percent of installed costs, all loans", "%",    "",     "financial parameters",   "",   "",  "" },
    { SSC_OUTPUT, SSC_NUMBER,   "const_per_principal_total",           "Total principal, all loans",				  "$",    "",     "financial parameters",   "",   "",  "" },
    { SSC_OUTPUT, SSC_NUMBER,   "const_per_interest_total",	           "Total interest costs, all loans",			  "$",    "",     "financial parameters",   "",   "",  "" },
    { SSC_OUTPUT, SSC_NUMBER,   "construction_financing_cost",         "Total construction financing cost",           "$",    "",     "financial parameters",   "",   "",  "" },

    var_info_invalid };

static var_info _cm_vtab_cb_construction_financing_upstream[] = {

    { SSC_INPUT,  SSC_NUMBER,   "total_installed_cost",                "Total installed cost",                        "$",    "",     "system costs",           "",   "",  "" },

var_info_invalid };

#endif

