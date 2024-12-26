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


#include "cmod_pvsamv1_eqns.h"
#include "cmod_battery_eqns.h"
#include "cmod_utilityrate5_eqns.h"

#include "vartab.h"

SSCEXPORT bool Reopt_size_battery_params(ssc_data_t data) {
    auto vt = static_cast<var_table*>(data);
    if (!vt) {
        return false;
    }
    var_table reopt_site, reopt_pv, reopt_utility;

    bool result = Reopt_size_standalone_battery_params(data);
    // Continue error handling for the prior function
    if (!result) {
        return result;
    }

    bool size_for_grid_outage = false;
    if (vt->is_assigned("size_for_grid_outage")) {
        size_for_grid_outage = vt->as_boolean("size_for_grid_outage");
    }

    auto reopt_params = *(vt->lookup("reopt_scenario"));
    std::string log = vt->as_string("log");
    var_table* reopt_table = &reopt_params.table;

    // site lat and lon
    map_input(vt, "lat", &reopt_site, "latitude");
    map_input(vt, "lon", &reopt_site, "longitude");

    //
    // convert required pvsamv1 or pvwatts inputs
    //

    // Use existing PV size for economic runs, allow REopt to return PV size for grid outage runs
    if (!size_for_grid_outage) {
        // use existing pv system from SAM, not allowing additional PV
        // do not specify both 'existing_kw' and 'max_kw' per https://github.com/NREL/SAM/issues/1742#issuecomment-2125878338
        //map_input(vt, "system_capacity", &reopt_pv, "existing_kw");
        map_input(vt, "system_capacity", &reopt_pv, "max_kw");
    }
    map_optional_input(vt, "degradation", &reopt_pv, "degradation_fraction", 0.5, true);

    map_optional_input(vt, "module_type", &reopt_pv, "module_type", 1);

    int opt1, opt2;
    var_data* vd, * vd2;
    vd = vt->lookup("subarray1_track_mode");
    vd2 = vt->lookup("subarray1_backtrack");
    if (vd && vd2) {
        opt1 = (int)vd->num[0];
        opt2 = (int)vd2->num[0];
        if (opt1 == 2 && opt2 == 1)
            opt1 = 3;
        std::vector<int> opt_map = { 0, 0, 2, 3, 4 };
        reopt_pv.assign("array_type", opt_map[opt1]);
    }
    else {
        map_input(vt, "array_type", &reopt_pv, "array_type");
    }

    auto assign_matching_pv_vars = [&vt](var_table& dest, std::string pvwatts_var, std::string pvsam_var, bool ratio = false) {
        try {
            map_input(vt, pvsam_var, &dest, pvwatts_var, false, ratio);
        }
        catch (std::runtime_error&) {
            map_input(vt, pvwatts_var, &dest, pvwatts_var, false, ratio);
        }
        };

    assign_matching_pv_vars(reopt_pv, "azimuth", "subarray1_azimuth");
    assign_matching_pv_vars(reopt_pv, "tilt", "subarray1_tilt");
    assign_matching_pv_vars(reopt_pv, "gcr", "subarray1_gcr");

    // Get appropriate inverter efficiency input and transform to ratio from percent
    int inv_model = 0;
    double val1, val2, system_cap;
    vt_get_number(vt, "system_capacity", &system_cap);
    vd = vt->lookup("inverter_model");
    if (vd) {
        std::vector<std::string> inv_eff_names = { "inv_snl_eff_cec", "inv_ds_eff", "inv_pd_eff", "inv_cec_cg_eff" };
        double eff;
        inv_model = (int)vd->num[0];
        if (inv_model == 4) {
            vt->assign("error", var_data("Inverter Mermoud Lejeune Model not supported."));
            return false;
        }
        vt_get_number(vt, inv_eff_names[inv_model], &eff);
        eff /= 100.;
        reopt_pv.assign("inv_eff", eff);

        // calculate the dc ac ratio
        std::vector<std::string> inv_power_names = { "inv_snl_paco", "inv_ds_paco", "inv_pd_paco", "inv_cec_cg_paco" };
        vt_get_number(vt, inv_power_names[inv_model], &val1);
        vt_get_number(vt, "inverter_count", &val2);
        reopt_pv.assign("dc_ac_ratio", system_cap * 1000. / (val2 * val1));
    }
    else {
        map_input(vt, "inv_eff", &reopt_pv, "inv_eff", false, true);
        map_input(vt, "dc_ac_ratio", &reopt_pv, "dc_ac_ratio");
    }

    // If gen is assigned, use REopt's prod_factor_series_kw number, if not use lat/lon and losses with the PVWatts weather files (as called by REopt)
    if (vt->is_assigned("gen_without_battery")) {
        std::vector<double> gen;
        vt_get_array_vec(vt, "gen_without_battery", gen);
        bool lifetime_mode;
        vt_get_bool(vt, "system_use_lifetime_output", &lifetime_mode);
        size_t year_one_values = gen.size();
        if (lifetime_mode) {
            int analysis_period;
            vt_get_int(vt, "analysis_period", &analysis_period);
            year_one_values /= analysis_period;
        }
        std::vector<double> kwac_per_kwdc(year_one_values);
        for (size_t i = 0; i < year_one_values; i++) {
            kwac_per_kwdc[i] = gen[i] / system_cap;
            kwac_per_kwdc[i] = std::max(0.0, kwac_per_kwdc[i]);
        }
        reopt_pv.assign("production_factor_series", kwac_per_kwdc);
        // The above already includes losses
        reopt_pv.assign("losses", 0.0);
    }
    else if (vt->is_assigned("losses")) {
        map_input(vt, "losses", &reopt_pv, "losses");
    }
    // Else use REopt default losses (14%)

    // financial inputs
    map_optional_input(vt, "itc_fed_percent", &reopt_pv, "federal_itc_fraction", 0., true);
    map_optional_input(vt, "pbi_fed_amount", &reopt_pv, "production_incentive_per_kwh", 0.);
    map_optional_input(vt, "pbi_fed_term", &reopt_pv, "production_incentive_years", 0.);
    vd = reopt_pv.lookup("production_incentive_years");
    if (vd->num[0] < 1) vd->num[0] = 1;    // minimum is 1, set pbi_fed_amount to 0 to disable

    map_optional_input(vt, "ibi_sta_percent", &reopt_pv, "state_ibi_fraction", 0., true);
    map_optional_input(vt, "ibi_sta_percent_maxvalue", &reopt_pv, "state_ibi_max", 10000000000);
    vd = reopt_pv.lookup("state_ibi_max");
    if (vd->num[0] > 10000000000) vd->num[0] = 10000000000;

    map_optional_input(vt, "ibi_uti_percent", &reopt_pv, "utility_ibi_fraction", 0., true);
    map_optional_input(vt, "ibi_uti_percent_maxvalue", &reopt_pv, "utility_ibi_max", 10000000000);
    vd = reopt_pv.lookup("utility_ibi_max");
    if (vd->num[0] > 10000000000) vd->num[0] = 10000000000;

    vd = vt->lookup("om_fixed");
    vd2 = vt->lookup("om_production");

    if (vd && !vd2) {
        reopt_pv.assign("om_cost_per_kw", vd->num[0] / system_cap);
    }
    else if (!vd && vd2) {
        reopt_pv.assign("om_cost_per_kw", vd2->num[0]);
    }
    else if (vd && vd2) {
        reopt_pv.assign("om_cost_per_kw", (vd->num[0] / system_cap) + vd2->num[0]);
    }

    vd = vt->lookup("total_installed_cost");
    if (vd) {
        reopt_pv.assign("installed_cost_per_kw", vd->num[0] / system_cap);
    }

    vd = vt->lookup("depr_bonus_fed");
    if (vd) {
        reopt_pv.assign("macrs_bonus_fraction", vd->num[0] / 100.);
    }
    vd = vt->lookup("depr_bonus_fed_macrs_5");
    if (vd && vd->num[0] == 1) {
        reopt_pv.assign("macrs_option_years", 5);
    }

    // Translate SAM timestep grid outage format into REopt outage start times format
    if (size_for_grid_outage) {
        std::vector<bool> outage_steps = vt->as_vector_bool("grid_outage");
        std::vector<int> outage_start_times;
        std::vector<int> outage_durations;

        bool existing_outage = false;
        int outage_duration = 0;
        for (size_t i = 0; i < outage_steps.size(); i++) {
            bool outage_status = outage_steps[i];
            if (outage_status) {
                // Outage starts this step
                if (!existing_outage) {
                    outage_start_times.push_back(i + 1); // SAM is zero indexed, REopt (Julia) is 1 indexed
                    existing_outage = true;
                }
                outage_duration++;
            }
            else if (existing_outage) {
                // Outage ends this step
                outage_durations.push_back(outage_duration);
                outage_duration = 0;
                existing_outage = false;
            }
        }
        reopt_utility.assign("outage_start_time_steps", outage_start_times);
        reopt_utility.assign("outage_durations", outage_durations);
    }

    // set net metering limit to system capacity per https://github.com/NREL/SAM/issues/1742
    vd = vt->lookup("ur_metering_option");
    if ((int)vd->num[0] == 0) // net metering
        reopt_utility.assign("net_metering_limit_kw", system_cap);

    // assign the reopt parameter table and log messages
    reopt_table->assign_match_case("PV", reopt_pv);
    reopt_table->assign_match_case("Site", reopt_site);
    reopt_table->assign_match_case("ElectricUtility", reopt_utility);
    vt->assign_match_case("reopt_scenario", reopt_params);
    vt->assign_match_case("log", log);
    return result;
}
