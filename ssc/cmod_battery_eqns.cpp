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

#include "cmod_battery_eqns.h"
#include "cmod_utilityrate5_eqns.h"

#include "core.h"
#include "vartab.h"

#include <cmath>

bool Size_batterystateful(ssc_data_t data) {
    auto vt = static_cast<var_table*>(data);
    char errmsg[250];
    if (!vt) {
        return false;
    }

    double nominal_energy, desired_voltage, desired_capacity;

    vt_get_number(vt, "nominal_energy", &nominal_energy);
    vt_get_number(vt, "desired_voltage", &desired_voltage);
    vt_get_number(vt, "desired_capacity", &desired_capacity);

    // Cannot specify energy of zero (less than mW, really) due to resulting errors in scaling factors
    if (nominal_energy < 1e-7) {
        sprintf(errmsg, "nominal_energy cannot be less than 1e-7. Current value: %f", nominal_energy);
        vt->assign("error", std::string(errmsg));
        return false;
    }

    if (desired_capacity < 1e-7) {
        sprintf(errmsg, "desired_capacity cannot be less than 1e-7. Current value: %f", desired_capacity);
        vt->assign("error", std::string(errmsg));
        return false;
    }

    vt->assign("original_capacity", nominal_energy);

    bool thermal_success = Calculate_thermal_params(data);

    vt->assign("nominal_energy", desired_capacity);
    vt->assign("nominal_voltage", desired_voltage);

    return thermal_success;
}

bool Calculate_thermal_params(ssc_data_t data) {
    auto vt = static_cast<var_table*>(data);
    if (!vt) {
        return false;
    }

    double mass, surface_area, original_capacity, desired_capacity, module_capacity, module_surface_area;

    vt_get_number(vt, "mass", &mass);
    vt_get_number(vt, "surface_area", &surface_area);
    vt_get_number(vt, "original_capacity", &original_capacity);
    vt_get_number(vt, "desired_capacity", &desired_capacity);

    double mass_per_specific_energy = mass / original_capacity;

    double volume = std::pow((surface_area / 6.0), (3.0 / 2.0));

    double volume_per_specific_energy = volume / original_capacity;

    mass = mass_per_specific_energy * desired_capacity;

    surface_area = std::pow((volume_per_specific_energy * desired_capacity), (2.0 / 3.0)) * 6;

    if (vt->is_assigned("module_capacity") && vt->is_assigned("module_surface_area")) {
        vt_get_number(vt, "module_capacity", &module_capacity);
        vt_get_number(vt, "module_surface_area", &module_surface_area);
        surface_area = module_surface_area * desired_capacity / module_capacity;
    }

    vt->assign("mass", mass);
    vt->assign("surface_area", surface_area);

    return true;
}

bool Reopt_size_standalone_battery_params(ssc_data_t data) {
    auto vt = static_cast<var_table*>(data);
    if (!vt) {
        return false;
    }
    std::string log;
    auto reopt_params = var_data();
    reopt_params.type = SSC_TABLE;
    var_table* reopt_table = &reopt_params.table;
    var_table reopt_electric, reopt_utility, reopt_load, reopt_fin, reopt_batt, reopt_settings;

    var_data* vd, * vd2;

    double val1, val2, system_cap;
    vt_get_number(vt, "system_capacity", &system_cap);

    // financial inputs
    map_optional_input(vt, "itc_fed_percent", &reopt_batt, "total_itc_fraction", 0., true);
    // TODO: what about reopt vars total_rebate_per_kw?

    vd = vt->lookup("total_installed_cost");
    if (vd) {
        reopt_batt.assign("installed_cost_per_kw", vd->num[0] / system_cap);
    }

    vd = vt->lookup("depr_bonus_fed");
    if (vd) {
        reopt_batt.assign("macrs_bonus_fraction", vd->num[0] / 100.);
    }
    vd = vt->lookup("depr_bonus_fed_macrs_5");
    if (vd && vd->num[0] == 1) {
        reopt_batt.assign("macrs_option_years", 5);
    }

    // These exist in the GUI but not in the default PySAM export
    vd = vt->lookup("battery_per_kW");
    if (vd)
        reopt_batt.assign("installed_cost_per_kw", vd->num[0]);
    vd = vt->lookup("battery_per_kWh");
    if (vd)
        reopt_batt.assign("installed_cost_per_kwh", vd->num[0]);

    vd = vt->lookup("batt_dc_ac_efficiency");
    vd2 = vt->lookup("batt_ac_dc_efficiency");

    if (vd) {
        reopt_batt.assign("inverter_efficiency_fraction", vd->num[0] / 100.);
    }
    if (vd2) {
        reopt_batt.assign("rectifier_efficiency_fraction", vd2->num[0] / 100.);
    }

    vd = vt->lookup("batt_initial_SOC");
    vd2 = vt->lookup("batt_minimum_SOC");
    if (vd && vd2) {
        reopt_batt.assign("soc_init_fraction", vd->num[0] / 100.);
        reopt_batt.assign("soc_min_fraction", vd2->num[0] / 100.);
    }
    else {
        reopt_batt.assign("soc_init_fraction", 0.5);
        reopt_batt.assign("soc_min_fraction", 0.15);
    }

    // battery replacement only enabled for pvsam, use REopt defaults otherwise
    if ((vd = vt->lookup("om_batt_replacement_cost")))
        reopt_batt.assign("replace_cost_per_kwh", vd->num[0]);

    // ReOpt's battery replacement single year versus SAM's array schedule
    std::vector<double> vec;
    if ((vd = vt->lookup("batt_replacement_schedule"))) {
        vec = vd->arr_vector();
        if (vec.size() > 1)
            log += "Warning: only first value of 'batt_replacement_schedule' array is used for the ReOpt input 'battery_replacement_year'.\n";
        reopt_batt.assign("battery_replacement_year", vec[0]);
    }

    if (vt->is_assigned("batt_dispatch_auto_can_gridcharge")) {
        vd = vt->lookup("batt_dispatch_auto_can_gridcharge");
        reopt_batt.assign("can_grid_charge", vd->num[0]);
    }
    else {
        // Grid charging is always on for battwatts
        reopt_batt.assign("can_grid_charge", true);
    }

    //
    // convert required utilityrate5 inputs
    //
    ElectricityRates_format_as_URDBv8(vt);
    auto urdb_data = vt->lookup("urdb_data");
    reopt_utility = urdb_data->table;

    //
    // convert financial inputs and set variables not modeled by SAM to 0
    //
    map_input(vt, "analysis_period", &reopt_fin, "analysis_years");
    map_input(vt, "rate_escalation", &reopt_fin, "elec_cost_escalation_rate_fraction", false, true);
    map_optional_input(vt, "value_of_lost_load", &reopt_fin, "value_of_lost_load_per_kwh", 0);
    reopt_fin.assign("microgrid_upgrade_cost_fraction", 0);



    vd = vt->lookup("federal_tax_rate");
    vd2 = vt->lookup("state_tax_rate");
    if (vd && vd2) {
        reopt_fin.assign("offtaker_tax_rate_fraction", vd->num[0] / 100. + vd2->num[0] / 100.);
    }

    vt_get_number(vt, "inflation_rate", &val1);
    vd = vt->lookup("real_discount_rate");
    if (vd) val2 = vd->num;
    else val2 = 6.4;
    reopt_fin.assign("offtaker_discount_rate_fraction", (1 + val1 / 100.) * (1 + val2 / 100.) - 1);

    vd = vt->lookup("om_fixed_escal");
    vd2 = vt->lookup("om_production_escal");
    if (vd && !vd2) {
        reopt_fin.assign("om_cost_escalation_rate_fraction", vd->num[0] / system_cap);
    }
    else if (!vd && vd2) {
        reopt_fin.assign("om_cost_escalation_rate_fraction", vd2->num[0]);
    }
    else if (vd && vd2) {
        reopt_fin.assign("om_cost_escalation_rate_fraction", (vd->num[0] / system_cap) + vd2->num[0]);
    }

    // convert load profile inputs, which are not net loads
    vt_get_array_vec(vt, "load", vec);
    size_t sim_len = vec.size();
    if (sim_len != 8760 && sim_len != 8760 * 2 && sim_len != 8760 * 4) {
        vt->assign("error", var_data("Load profile must be hourly, 30 min or 15 min data for a single year."));
        return false;
    }
    reopt_load.assign("loads_kw", var_data(&vec[0], sim_len));
    reopt_load.assign("loads_kw_is_net", false);
    reopt_load.assign("year", 2018); // recent common year starting on Monday

    vd = vt->lookup("crit_load");
    if (vd) {
        vt_get_array_vec(vt, "crit_load", vec);
        size_t vec_size = vec.size();
        if (vec_size != sim_len) {
            int analysis_period = vt->as_integer("analysis_period");
            if (vec_size == sim_len * analysis_period) {
                vec_size = sim_len;
            }
            else {
                vt->assign("error", var_data("Critical load profile's length must be same as for load."));
                return false;
            }
        }
        reopt_load.assign("critical_loads_kw", var_data(&vec[0], vec_size));
        reopt_load.assign("year", 2018); // recent common year starting on Monday
        reopt_load.assign("critical_loads_kw_is_net", false);
    }

    reopt_settings.assign_match_case("time_steps_per_hour", var_data((int)(sim_len / 8760)));
    //reopt_settings.assign("solver_name", var_data("SCIP")); // "HiGHS" option does not work with large numbers like 1e38 for tier max values per https://github.com/NREL/SAM/issues/1742

    // assign the reopt parameter table and log messages
    reopt_table->assign_match_case("Settings", reopt_settings);
    reopt_electric.assign_match_case("urdb_response", reopt_utility);
    reopt_table->assign_match_case("ElectricTariff", reopt_electric);
    reopt_table->assign_match_case("ElectricLoad", reopt_load);
    reopt_table->assign_match_case("Financial", reopt_fin);
    reopt_table->assign_match_case("ElectricStorage", reopt_batt);
    vt->assign_match_case("reopt_scenario", reopt_params);
    vt->assign_match_case("log", log);
    return true;
}
