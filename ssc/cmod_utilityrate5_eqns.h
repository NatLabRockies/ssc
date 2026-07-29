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


#ifndef SYSTEM_ADVISOR_MODEL_CMOD_UTILITYRATE5_EQNS_H
#define SYSTEM_ADVISOR_MODEL_CMOD_UTILITYRATE5_EQNS_H

#include "sscapi.h"

#ifdef __cplusplus

extern "C" {
#endif

static const char *ElectricityRates_format_as_URDBv8_doc =
        "Format the electricty rate information in cmod_utilityrate5 to the format specified in:\\n"
        "https://openei.org/services/doc/rest/util_rates/?version=7\\n"
        "Input: var_table with key-value pairs:  \\n"
        "     'ur_metering_option': int, [0=net energy metering,1=net energy metering with $ credits,2=net billing,3=net billing with carryover to next month,4=buy all - sell all]"
        "     'ur_monthly_fixed_charge': double [$], Monthly fixed charge\\n"
        "     'ur_monthly_min_charge': double [$], Monthly minimum charge\\n"
        "     'ur_annual_min_charge': optional double [$], Annual minimum charge\\n"
        "     'ur_ec_sched_weekday': optional matrix [period], Energy charge weekday schedule, count starts at 1, 12mx24hr\\n"
        "     'ur_ec_sched_weekend': optional matrix [period], Energy charge weekend schedule, count starts at 1, 12mx24hr\\n"
        "     'ur_ec_tou_mat': optional matrix [[period, tier, kwh, $], Demand rates (TOU), each row: (period, tier, max usage, units=='kWh', buy rate, sell rate)\\n"
        "     'ur_dc_flat_mat' optional matrix [[month, tier, kW, $]] - Demand rates (flat), each row: (month, tier, peak demand, buy rate) \\n"
        "     'ur_dc_sched_weekday': optional matrix [period], Demand charge weekday schedule, count starts at 1, 12mx24hr\\n"
        "     'ur_dc_sched_weekend': optional matrix [period], Demand charge weekend schedule, count starts at 1, 12mx24hr\\n"
        "     'ur_dc_tou_mat': optional matrix [[period, tier, kWh, bool, $/kWh, $/kWh], Energy rates (TOU), each row: (period, tier, max usage, units=='kW', buy rate, sell rate)\\n"
        "     'ur_billing_demand_lookback_percentages': optional matrix [[lookback percent, actual peak demand used], Energy rates (TOU), each row: (%, 0/1), 12mx2\\n"
        "     'ur_billing_demand_lookback_period': optional integer, Billing demand lookback months. Required when ur_billing_demand_lookback_percentages has uniform values in column 0\\n"
        "Output: key-value pairs added to var_table\\n"
        "     'urdb_data' - table, data formatted as URDB v7 response\\n"
        "     'log' - string";

SSCEXPORT bool ElectricityRates_format_as_URDBv8(ssc_data_t data);

}

#endif //SYSTEM_ADVISOR_MODEL_CMOD_UTILITYRATE5_EQNS_H
