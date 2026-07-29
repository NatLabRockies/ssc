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


#include <algorithm>
#include <set>

#include "vartab.h"
#include "cmod_utilityrate5_eqns.h"

bool try_get_rate_schedule(var_table* vt, const std::string& ssc_name, util::matrix_t<double>& schedule_matrix){
    schedule_matrix.clear();
    auto vd = vt->lookup(ssc_name);
    if (!vd) return false;

    auto mat = vd->num;
    schedule_matrix.copy(mat);
    for (size_t i = 0; i < mat.nrows(); i++){
        for (size_t j = 0; j < mat.ncols(); j++ ) {
            schedule_matrix.at(i, j) -= 1;
        }
    }
    return true;
}

bool try_get_rate_structure(var_table* vt, const std::string& ssc_name, bool power_units,
                            std::vector<std::vector<var_data>>& rate_structure) {
    rate_structure.clear();
    auto vd = vt->lookup(ssc_name);
    if (!vd) return false;

    std::vector<std::vector<double>> rate_matrix = vd->matrix_vector();
    size_t n_periods = (size_t)(*(std::max_element(rate_matrix.begin(), rate_matrix.end(),
                                                   [] (const std::vector<double>& lhs, const std::vector<double>& rhs) {
                                                       return lhs[0] < rhs[0];
                                                   })))[0];
    rate_structure.resize(n_periods);

    // sort by tiers then by period
    std::sort(rate_matrix.begin(), rate_matrix.end(),
              [] (const std::vector<double>& lhs, const std::vector<double>& rhs) {
                  return lhs[1] < rhs[1];
              });
    std::sort(rate_matrix.begin(), rate_matrix.end(),
              [] (const std::vector<double>& lhs, const std::vector<double>& rhs) {
                  return lhs[0] < rhs[0];
              });

    if (power_units) {
        vt->assign("demandrateunit", var_data("kW"));
    }

    for (const auto& row : rate_matrix) {
        int period = row[0];
        double max = row[2];
        double buy;
        var_data rate_data;
        rate_data.type = SSC_TABLE;
        if (!power_units){
            int unit_type = row[3];
            buy = row[4];
            double sell = row[5];
            if (unit_type == 0 || ((unit_type == -1 && max > 1e36)))
                rate_data.table.assign("unit", var_data("kWh"));
            else if (unit_type == 2)
                rate_data.table.assign("unit", var_data("kWh daily"));
            else{
                vt->assign("error", var_data("ElectricityRates_format_as_URDB error. Unit type in " + ssc_name + " not allowed."));
                return false;
            }
            rate_data.table.assign("sell", sell);
        }
        else{
            buy = row[3];
        }
        rate_data.table.assign("max", max);
        rate_data.table.assign("rate", buy);

        rate_structure[period - 1].push_back(rate_data);
    }
    return true;
}

SSCEXPORT bool ElectricityRates_format_as_URDBv8(ssc_data_t data) {
    auto vt = static_cast<var_table*>(data);
    if (!vt){
        return false;
    }
    auto urdb_data = var_table();
    std::string log;

    int net_metering;
    vt_get_int(vt, "ur_metering_option", &net_metering);
    std::string dgrules;
    switch(net_metering) {
        case 0: // net energy metering
            dgrules = "Net Metering";
            break;
        case 1: // net energy metering with $ credits
            throw(std::runtime_error(util::format("ElectricityRates_format_as_URDB error. ur_net_metering_option=%d not available in URDB. Net Metering with $ credits is not supported.", net_metering)));
        case 2: // net billing
            dgrules = "Net Billing Hourly";
            break;
        case 3: // net billing with carryover to next month
            throw(std::runtime_error(util::format("ElectricityRates_format_as_URDB error. ur_net_metering_option=%d not available in URDB. Net Billing with Carryover to Next Month is not supported.", net_metering)));
        case 4: // buy all / sell all
            dgrules = "Buy All Sell All";
            break;
        default:
            vt->assign("error", var_data(util::format("ElectricityRates_format_as_URDB error. ur_net_metering_option=%d not recognized.", net_metering)));
    }
    urdb_data.assign("dgrules", dgrules);

    double monthly_fixed, monthly_min;
    vt_get_number(vt, "ur_monthly_fixed_charge", &monthly_fixed);
    urdb_data.assign("fixedmonthlycharge", monthly_fixed);
    vt_get_number(vt, "ur_monthly_min_charge", &monthly_min);
    urdb_data.assign("minmonthlycharge", monthly_min);

    try{
        double annual_min;
        vt_get_number(vt, "ur_annual_min_charge", &annual_min);
        urdb_data.assign("annualmincharge", annual_min);
    }
    catch(std::exception&){}

    // energy rates
    util::matrix_t<double> sched_matrix;
    if (try_get_rate_schedule(vt, "ur_ec_sched_weekday", sched_matrix))
        urdb_data.assign("energyweekdayschedule", sched_matrix);
    if (try_get_rate_schedule(vt, "ur_ec_sched_weekend", sched_matrix))
        urdb_data.assign("energyweekendschedule", sched_matrix);

    std::vector<std::vector<var_data>> rate_structure;
    if (try_get_rate_structure(vt, "ur_ec_tou_mat", false, rate_structure))
        urdb_data.assign("energyratestructure", rate_structure);

    // flat demand structure
    sched_matrix.clear();
    if (vt->is_assigned("ur_dc_flat_mat")){
        sched_matrix = vt->lookup("ur_dc_flat_mat")->num;

        size_t n_rows = sched_matrix.nrows();
        std::vector<std::vector<double>> flatdemand;
        for (size_t i = 0; i < n_rows; i++){
            std::vector<double> row;
            row.push_back(sched_matrix.at(i, 0));
            row.push_back(sched_matrix.at(i, 1));
            row.push_back(sched_matrix.at(i, 2));
            row.push_back(sched_matrix.at(i, 3));
            flatdemand.emplace_back(row);
        }

        std::vector<std::vector<var_data>> flat_demand_structure;
        std::vector<double> flat_demand_months;
        flat_demand_months.resize(12);

        // pull out first tier of periods
        for (size_t i = 0; i < n_rows; i++){
            double tier = sched_matrix.at(i, 1);

            if (tier != 1)
                continue;

            double month = sched_matrix.at(i, 0);
            double max = sched_matrix.at(i, 2);
            double charge = sched_matrix.at(i, 3);
            std::vector<var_data> row;

            // see if a period with a matching first tier exists
            size_t period = -1;
            for (size_t j = 0; j < flat_demand_structure.size(); j++){
                double j_max = flat_demand_structure[j][0].table.lookup("max")->num[0];
                double j_charge = flat_demand_structure[j][0].table.lookup("rate")->num[0];
                if (std::abs(max - j_max) < 1e-3 && std::abs(charge - j_charge) < 1e-3){
                    period = j;
                    break;
                }
            }
            // doesn't exist so add it
            if (period == -1){
                var_data rate_data;
                rate_data.type = SSC_TABLE;
                rate_data.table.assign("max", max);
                rate_data.table.assign("rate", charge);
                row.emplace_back(rate_data);
                flat_demand_structure.emplace_back(row);
                period = flat_demand_structure.size() - 1;
            }
            flat_demand_months[month] = period;
        }
        // do other tiers
        for (size_t i = 0; i < n_rows; i++){
            double tier = sched_matrix.at(i, 1);

            if (tier == 1)
                continue;

            double month = sched_matrix.at(i, 0);
            double max = sched_matrix.at(i, 2);
            double charge = sched_matrix.at(i, 3);

            // see if a period with a matching first tier exists
            double period = flat_demand_months[month];
            var_data rate_data;
            rate_data.type = SSC_TABLE;
            rate_data.table.assign("max", max);
            rate_data.table.assign("rate", charge);
            flat_demand_structure[period].emplace_back(rate_data);
        }
        urdb_data.assign("flatdemandstructure", flat_demand_structure);
        urdb_data.assign("flatdemandmonths", flat_demand_months);
    }

    if (vt->is_assigned("ur_billing_demand_lookback_percentages") && vt->is_assigned("ur_enable_billing_demand")) {
        if (vt->as_boolean("ur_enable_billing_demand")) {
            std::vector<int> billing_demand(12);
            util::matrix_t<double> billing_demand_matrix = vt->as_matrix("ur_billing_demand_lookback_percentages");
            bool any_zeroes = false;
            bool all_same = true;
            double reference_demand = billing_demand_matrix.at(0, 0);
            double tol = 1e-7;
            for (size_t i = 0; i < 12; i++) {
                double monthly_demand_percent = billing_demand_matrix.at(i, 0);
                if (monthly_demand_percent < tol) {
                    any_zeroes = true;
                    billing_demand[i] = 0;
                }
                else {
                    if (any_zeroes) {
                        if (reference_demand < tol) {
                            reference_demand = monthly_demand_percent;
                        }
                    }

                    if (std::abs(reference_demand - monthly_demand_percent) < tol) {
                        all_same = false;
                    }

                    billing_demand[i] = 1;
                }
            }

            if (!all_same) {
                // SAM supports 12 different percentages, but REopt and URDB only support one. Flag this for users
                urdb_data.assign("warning", var_data("ur_billing_demand_lookback_percentages has multiple non-zero percentages. REopt/URDB only supports a single percentage for lookbackPercent. Using the last non-zero percent"));
            }

            // If zeroes are present, use the lookbackMonths structure to indicate which months are relevant
            if (any_zeroes) {
                urdb_data.assign("lookbackMonths", var_data(billing_demand));
            }
            else { // Else specify the lookback range in integer months. SAM supports these in tandem, REopt/URDB only supports one

                if (vt->is_assigned("ur_billing_demand_lookback_period")) {
                    urdb_data.assign("lookbackRange", vt->as_integer("ur_billing_demand_lookback_period"));
                }
                else {
                    urdb_data.assign("error", var_data("ur_billing_demand_lookback_period is not assigned when the structure of ur_billing_demand_lookback_percentages requires it. please assign the lookback period, in months"));
                }
            }
            urdb_data.assign("lookbackPercent", var_data(reference_demand / 100.0));
        }
    }

    // tou
    if (try_get_rate_schedule(vt, "ur_dc_sched_weekday", sched_matrix))
        urdb_data.assign("demandweekdayschedule", sched_matrix);
    if (try_get_rate_schedule(vt, "ur_dc_sched_weekend", sched_matrix))
        urdb_data.assign("demandweekendschedule", sched_matrix);

    if (try_get_rate_structure(vt, "ur_dc_tou_mat", true, rate_structure))
        urdb_data.assign("demandratestructure", rate_structure);

    if (vt->is_assigned("error"))
        return false;

    vt->assign("urdb_data", urdb_data);
    return true;
}
