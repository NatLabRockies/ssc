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


#include <cmath>
#include <math.h>
#include <cmath>
#include <algorithm>
#include "cmod_csp_common_eqns.h"
#include "vartab.h"
#include "csp_system_costs.h"
#include "csp_solver_cavity_receiver.h"

#pragma warning(disable: 4297)  // ignore warning: 'function assumed not to throw an exception but does'


SSCEXPORT ssc_bool_t ssc_data_t_get_number(ssc_data_t p_data, const char* name, ssc_number_t* value)
{
    bool success = ssc_data_get_number(p_data, name, value);
    if (!success) {
        // replace any periods in the name with underscores in order to read variables set by the UI
        std::string str_name(name);
        size_t n_replaced = util::replace(str_name, ".", "_");
        if (n_replaced > 0) {
            success = ssc_data_get_number(p_data, str_name.c_str(), value);
        }
    }

    return success;
}

SSCEXPORT void ssc_data_t_set_number(ssc_data_t p_data, const char* name, ssc_number_t value)
{
    ssc_data_set_number(p_data, name, value);

    // replace any periods in the name with underscores so UI equations can read value
    std::string str_name(name);
    size_t n_replaced = util::replace(str_name, ".", "_");
    if (n_replaced > 0) {
        ssc_data_set_number(p_data, str_name.c_str(), value);
    }
}

SSCEXPORT ssc_number_t *ssc_data_t_get_array(ssc_data_t p_data, const char* name, int* length)
{
    ssc_number_t* data;
    data = ssc_data_get_array(p_data, name, length);
    if (data == 0) {
        // replace any periods in the name with underscores in order to read variables set by the UI
        std::string str_name(name);
        size_t n_replaced = util::replace(str_name, ".", "_");
        if (n_replaced > 0) {
            data = ssc_data_get_array(p_data, str_name.c_str(), length);
        }
    }

    return data;
}

SSCEXPORT void ssc_data_t_set_array(ssc_data_t p_data, const char* name, ssc_number_t* pvalues, int length)
{
    ssc_data_set_array(p_data, name, pvalues, length);

    // replace any periods in the name with underscores so UI equations can read value
    std::string str_name(name);
    size_t n_replaced = util::replace(str_name, ".", "_");
    if (n_replaced > 0) {
        ssc_data_set_array(p_data, str_name.c_str(), pvalues, length);
    }
}

SSCEXPORT void ssc_data_t_get_matrix(var_table* vt, std::string name, util::matrix_t<double>& matrix)
{
    try
    {
        vt_get_matrix(vt, name, matrix);
    }
    catch (std::exception&) {
    }

    // replace any periods in the name with underscores in order to read variables set by the UI
    std::string str_name(name);
    size_t n_replaced = util::replace(str_name, ".", "_");
    if (n_replaced > 0) {
        vt_get_matrix(vt, name, matrix);        // allow exceptions to be uncaught
    }
}

SSCEXPORT void ssc_data_t_set_matrix(ssc_data_t data, const std::string& name, const var_data& val)
{
    auto vt = static_cast<var_table*>(data);
    if (!vt) {
        throw std::runtime_error("ssc_data_t data invalid");
    }

    vt->assign(name, val);

    // replace any periods in the name with underscores so UI equations can read value
    std::string str_name(name);
    size_t n_replaced = util::replace(str_name, ".", "_");
    if (n_replaced > 0) {
        vt->assign(str_name.c_str(), val);
    }
}


//HTFProperties GetHtfProperties(int fluid_number, const util::matrix_t<double> &specified_fluid_properties) {       // [-]
//
//    HTFProperties htf_properties;
//
//    if (fluid_number != HTFProperties::User_defined)
//    {
//        if (!htf_properties.SetFluid(fluid_number))
//        {
//            throw("Fluid number is not recognized");
//        }
//    }
//    else if (fluid_number == HTFProperties::User_defined)
//    {
//        std::size_t n_rows = specified_fluid_properties.nrows();
//        std::size_t n_cols = specified_fluid_properties.ncols();
//        if (n_rows > 2 && n_cols == 7)
//        {
//            if (!htf_properties.SetUserDefinedFluid(specified_fluid_properties))
//            {
//                std::string error_msg = util::format(htf_properties.UserFluidErrMessage(), n_rows, n_cols);
//                throw(error_msg);
//            }
//        }
//        else
//        {
//            std::string error_msg = util::format("The user defined fluid properties table must contain at least 3 rows and exactly 7 columns. The current table contains %d row(s) and %d column(s)", n_rows, n_cols);
//            throw(error_msg);
//        }
//    }
//    else
//    {
//        throw("Fluid code is not recognized");
//    }
//
//    return htf_properties;
//}
//
//
//
//
///////////////////////////////////////////////////////////////////////////////////////////////
//// Power Tower //////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
//
//// Originally from 'MSPT System Design' UI form
//double Nameplate(double P_ref /*MWe*/, double gross_net_conversion_factor /*-*/) {      // MWe
//    return P_ref * gross_net_conversion_factor;
//}
//
//double Q_pb_design(double P_ref /*MWe*/, double design_eff /*-*/) {     // MWt
//    return P_ref / design_eff;
//}
//
//double Q_rec_des(double solarm /*-*/, double q_pb_design /*MWt*/) {     // MWt
//    return solarm * q_pb_design;
//}
//
//double Tshours_sf(double tshours /*hr*/, double solarm /*-*/) {         // hr
//    return tshours / solarm;
//}
//
//
//
//// Originally from 'Tower SolarPilot Solar Field' UI Form
//double Land_max_calc(double land_max /*-*/, double h_tower /*m*/) {      // [m]
//    return land_max * h_tower;
//}
//
//int N_hel(const util::matrix_t<ssc_number_t> &helio_positions /*m*/) {      // [-]
//    return static_cast<int>(helio_positions.nrows());
//}
//
//double Csp_pt_sf_heliostat_area(double helio_height /*m*/, double helio_width /*m*/, double dens_mirror /*-*/) {     // [m2]
//    return helio_height * helio_width * dens_mirror;
//}
//
//double Csp_pt_sf_total_reflective_area(int n_hel /*-*/, double csp_pt_sf_heliostat_area /*m2*/) {     // [m2]
//    return n_hel * csp_pt_sf_heliostat_area;
//}
//
//double Land_min_calc(double land_min /*-*/, double h_tower /*m*/) {      // [m]
//    return land_min * h_tower;
//}
//
//double Csp_pt_sf_total_land_area(double csp_pt_sf_fixed_land_area /*acres*/, double land_area_base /*acres*/,
//    double csp_pt_sf_land_overhead_factor /*-*/) {       // [acres]
//    
//    return csp_pt_sf_fixed_land_area + land_area_base * csp_pt_sf_land_overhead_factor;
//}
//
//double A_sf_UI(double helio_width /*m*/, double helio_height /*m*/, double dens_mirror /*-*/, int n_hel /*-*/) {  // [m2]
//    return helio_width * helio_height * dens_mirror * n_hel;
//}
//
//double Helio_area_tot(double A_sf_UI /*m2*/) {     // [m2]
//    return A_sf_UI;
//}
//
//double Csp_pt_sf_tower_height(double h_tower /*m*/) {        // [m]
//    return h_tower;
//}
//
//double C_atm_info(const util::matrix_t<ssc_number_t> &helio_positions /*m*/,
//    double c_atm_0 /*-*/, double c_atm_1 /*-*/, double c_atm_2 /*-*/, double c_atm_3 /*-*/, double h_tower /*m*/) {  // [%]
//    
//    double tht2 = h_tower * h_tower;
//    std::size_t n_hel = helio_positions.nrows();
//
//    double tot_att = 0.;
//    for (std::size_t i = 0; i < n_hel; i++) {
//        double x = helio_positions.at(i, 0);
//        double y = helio_positions.at(i, 1);
//        double r = std::sqrt(x*x + y*y);
//        double r2 = r*r;
//
//        double s = std::sqrt(tht2 + r2) * 0.001;    // [km]
//        double s2 = s*s;
//        double s3 = s2*s;
//
//        tot_att += c_atm_0 + c_atm_1*s + c_atm_2*s2 + c_atm_3*s3;
//    }
//
//    return 100. * tot_att / n_hel;
//}
//
//double Error_equiv(double helio_optical_error_mrad /*mrad*/) {       // [mrad]
//    return std::sqrt(2. * helio_optical_error_mrad * 2. * helio_optical_error_mrad * 2.);
//}
//
//bool Is_optimize(bool override_opt /*-*/) {      // [-]
//    if (override_opt) {
//        return true;
//    }
//    else {
//        return false;
//    }
//}
//
//int Field_model_type(bool is_optimize /*-*/, bool override_layout /*-*/, int assigned_field_model_type /*-*/) {      // [-]
//    if (is_optimize) {  // "override_opt"
//        return 0;
//    }
//    else if (override_layout) {
//        return 1;
//    }
//    else if (assigned_field_model_type >= 0) {       // if valid
//        return assigned_field_model_type;
//    }
//    else {
//        return 2;
//    }
//}
//
//double Q_design(double Q_rec_des /*MWt*/) {      // [MWt]
//    return Q_rec_des;
//}
//
//double Dni_des_calc(double dni_des /*W/m2*/) {       // [W/m2]
//    return dni_des;
//}
//
//// Originally from 'MSPT Receiver' UI Form
//double Csp_pt_rec_htf_t_avg(double T_htf_cold_des /*C*/, double T_htf_hot_des /*C*/) {       // [C]
//    return (T_htf_cold_des + T_htf_hot_des) / 2.;
//}
//
//double Csp_pt_rec_htf_c_avg(double csp_pt_rec_htf_t_avg /*C*/, int rec_htf /*-*/,
//    const util::matrix_t<ssc_number_t> &field_fl_props /*-*/) {      // [kJ/kg-K]
//    
//    HTFProperties htf_properties = GetHtfProperties(rec_htf, field_fl_props);
//    return htf_properties.Cp(csp_pt_rec_htf_t_avg + 273.15);
//}
//
//double Csp_pt_rec_max_flow_to_rec(double csp_pt_rec_max_oper_frac /*-*/, double Q_rec_des /*MWt*/,
//    double csp_pt_rec_htf_c_avg /*kJ/kg-K*/, double T_htf_hot_des /*C*/, double T_htf_cold_des /*C*/) {      // [kg/s]
//
//    return (csp_pt_rec_max_oper_frac * Q_rec_des * 1.e6) /
//        (csp_pt_rec_htf_c_avg * 1.e3 * (T_htf_hot_des - T_htf_cold_des));
//}
//
//double Rec_aspect(double D_rec /*m*/, double rec_height /*m*/) {     // [-]
//    double aspect;
//    if (D_rec != 0.) {
//        aspect = rec_height / D_rec;
//    }
//    else {
//        aspect = 1.;
//    }
//
//    return aspect;
//}
//
//// Originally from 'MSPT System Control'
//double Csp_pt_par_calc_bop(double bop_par /*MWe/MWcap*/, double bop_par_f /*-*/, double bop_par_0 /*-*/,
//    double bop_par_1 /*-*/, double bop_par_2 /*-*/, double p_ref /*MWe*/) {      // [MWe]
//
//    return bop_par * bop_par_f * ( bop_par_0 + bop_par_1 + bop_par_2 ) * p_ref;
//}
//
//double Csp_pt_par_calc_aux(double aux_par /*MWe/MWcap*/, double aux_par_f /*-*/, double aux_par_0 /*-*/,
//    double aux_par_1 /*-*/, double aux_par_2 /*-*/, double p_ref /*MWe*/) {      // [MWe]
//
//    return aux_par * aux_par_f * (aux_par_0 + aux_par_1 + aux_par_2) * p_ref;
//}
//
//double Disp_wlim_max(double disp_wlim_maxspec /**/, double constant /*%*/) {        // [MWe]
//    return disp_wlim_maxspec * (1. - constant / 100.);
//}
//
//util::matrix_t<double> Wlim_series(double disp_wlim_max /*MWe*/) {    // [kWe]
//    const int kHoursInYear = 8760;
//
//    double disp_wlim_max_kW = disp_wlim_max * 1000.;
//    util::matrix_t<double> wlim_series(1, kHoursInYear, disp_wlim_max_kW);
//
//    return wlim_series;
//}
//
//
//
//
//
//// Originally from 'Tower SolarPilot Capital Costs'
////double Ui_tower_height(TowerTypes tower_type, double height) {
////
////}
//
//void Csp_pt_cost_receiver_area(TowerTypes tower_type /*-*/, double d_rec /*m*/,
//    double rec_height /*m*/, int receiver_type /*-*/, double cav_rec_height /*m*/,
//    double cav_rec_width /*m*/, double rec_span_deg /*deg*/, int n_cav_panels,
//    double& area /*m2*/, double& cav_panel_width /*m*/,
//    double& cav_radius /*m*/, double& cav_offset) {      // [m2]
//
//    area = cav_panel_width = cav_radius = cav_offset = std::numeric_limits<double>::quiet_NaN();
//
//    if (tower_type == TowerTypes::kMoltenSalt || tower_type == TowerTypes::kIscc) {
//        switch (receiver_type) {
//        case 0:
//            area = rec_height * d_rec * M_PI;
//            break;
//        case 1:
//        {
//            double rec_span_rad = rec_span_deg * M_PI / 180.0;
//            double theta0, panelSpan;
//            theta0 = panelSpan = std::numeric_limits<double>::quiet_NaN();
//            cavity_receiver_helpers::calc_receiver_macro_geometry(cav_rec_height, cav_rec_width,
//                rec_span_rad, n_cav_panels,
//                theta0, panelSpan, cav_panel_width, area, cav_radius, cav_offset);
//        }
//            break;
//        default:
//            throw std::runtime_error("Receiver type not supported.");
//        }
//    }
//    else if (tower_type == TowerTypes::kDirectSteam) {
//        area = d_rec * rec_height * M_PI;
//    }
//
//    return;
//}
//
//double Csp_pt_cost_storage_mwht(TowerTypes tower_type /*-*/, double p_ref /*MWe*/, double design_eff /*-*/,
//    double tshours /*hr*/) {      // [MWht]
//
//    double nameplate = std::numeric_limits<double>::quiet_NaN();
//
//    if (tower_type == TowerTypes::kMoltenSalt) {
//        nameplate = p_ref / design_eff * tshours;
//    }
//    else {
//        nameplate = 0.;
//    }
//
//    return nameplate;
//}
//
//double Csp_pt_cost_power_block_mwe(TowerTypes tower_type /*-*/, double p_ref /*MWe*/, double demand_var /*MWe*/)       // [MWe]
//{
//    double pb = std::numeric_limits<double>::quiet_NaN();
//
//    if (tower_type == TowerTypes::kMoltenSalt) {
//        pb = p_ref;
//    }
//    else {
//        pb = demand_var;
//    }
//
//    return pb;
//}
//
//void Tower_SolarPilot_Capital_Costs_Equations(ssc_data_t data)
//{
//    auto vt = static_cast<var_table*>(data);
//    if (!vt) {
//        throw std::runtime_error("ssc_data_t data invalid");
//    }
//
//    double A_sf_refl, site_improv_spec_cost, heliostat_spec_cost, heliostat_fixed_cost,
//        h_tower, h_rec, h_helio, tower_fixed_cost, tower_cost_scaling_exp,
//        A_rec, rec_ref_cost, A_rec_ref, rec_cost_scaling_exp,
//        Q_storage, tes_spec_cost,
//        W_dot_design, power_cycle_spec_cost,
//        bop_spec_cost,
//        fossil_backup_spec_cost,
//        contingency_rate,
//        total_land_area, plant_net_capacity, EPC_land_spec_cost, EPC_land_perc_direct_cost,
//        EPC_land_per_power_cost, EPC_land_fixed_cost, total_land_spec_cost, total_land_perc_direct_cost,
//        total_land_per_power_cost, total_land_fixed_cost, sales_tax_basis, sales_tax_rate;
//
//    //sys_costs.ms_par.A_sf_refl = as_double("A_sf");
//    ssc_data_t_get_number(data, "a_sf_ui", &A_sf_refl);
//    ssc_data_t_get_number(data, "site_spec_cost", &site_improv_spec_cost);
//    ssc_data_t_get_number(data, "heliostat_spec_cost", &heliostat_spec_cost);
//    ssc_data_t_get_number(data, "cost_sf_fixed", &heliostat_fixed_cost);
//    ssc_data_t_get_number(data, "h_tower", &h_tower);                            // set different for other techs
//    ssc_data_t_get_number(data, "rec_height", &h_rec);
//    ssc_data_t_get_number(data, "helio_height", &h_helio);
//    ssc_data_t_get_number(data, "tower_fixed_cost", &tower_fixed_cost);
//    ssc_data_t_get_number(data, "tower_exp", &tower_cost_scaling_exp);
//    ssc_data_t_get_number(data, "csp.pt.cost.receiver.area", &A_rec);            // calculation specific to each tech
//    ssc_data_t_get_number(data, "rec_ref_cost", &rec_ref_cost);
//    ssc_data_t_get_number(data, "rec_ref_area", &A_rec_ref);
//    ssc_data_t_get_number(data, "rec_cost_exp", &rec_cost_scaling_exp);
//    ssc_data_t_get_number(data, "csp.pt.cost.storage_mwht", &Q_storage);         // calculation specific to each tech
//    ssc_data_t_get_number(data, "tes_spec_cost", &tes_spec_cost);
//
//    ssc_data_t_get_number(data, "csp.pt.cost.power_block_mwe", &W_dot_design);   // calculation specific to each tech
//    ssc_data_t_get_number(data, "plant_spec_cost", &power_cycle_spec_cost);
//
//    double q_dot_heater_des_calc, heater_spec_cost;
//    ssc_data_t_get_number(data, "q_dot_heater_des_calc", &q_dot_heater_des_calc);   //[MWt]
//    ssc_data_t_get_number(data, "heater_spec_cost", &heater_spec_cost);
//
//    ssc_data_t_get_number(data, "bop_spec_cost", &bop_spec_cost);
//    ssc_data_t_get_number(data, "fossil_spec_cost", &fossil_backup_spec_cost);
//    ssc_data_t_get_number(data, "contingency_rate", &contingency_rate);
//    ssc_data_t_get_number(data, "csp.pt.sf.total_land_area", &total_land_area);
//    ssc_data_t_get_number(data, "nameplate", &plant_net_capacity);
//    ssc_data_t_get_number(data, "csp.pt.cost.epc.per_acre", &EPC_land_spec_cost);
//    ssc_data_t_get_number(data, "csp.pt.cost.epc.percent", &EPC_land_perc_direct_cost);
//    ssc_data_t_get_number(data, "csp.pt.cost.epc.per_watt", &EPC_land_per_power_cost);
//    ssc_data_t_get_number(data, "csp.pt.cost.epc.fixed", &EPC_land_fixed_cost);
//    ssc_data_t_get_number(data, "land_spec_cost", &total_land_spec_cost);
//    ssc_data_t_get_number(data, "csp.pt.cost.plm.percent", &total_land_perc_direct_cost);
//    ssc_data_t_get_number(data, "csp.pt.cost.plm.per_watt", &total_land_per_power_cost);
//    ssc_data_t_get_number(data, "csp.pt.cost.plm.fixed", &total_land_fixed_cost);
//    ssc_data_t_get_number(data, "sales_tax_frac", &sales_tax_basis);
//    ssc_data_t_get_number(data, "sales_tax_rate", &sales_tax_rate);
//
//
//    double site_improvement_cost, heliostat_cost, tower_cost, receiver_cost, tes_cost, CT_tes_cost, power_cycle_cost, heater_cost,
//        bop_cost, fossil_backup_cost,
//        direct_capital_precontingency_cost, contingency_cost, total_direct_cost, epc_and_owner_cost, total_land_cost,
//        sales_tax_cost, total_indirect_cost, total_installed_cost, estimated_installed_cost_per_cap;
//
//    site_improvement_cost = heliostat_cost = tower_cost = receiver_cost = tes_cost = CT_tes_cost = power_cycle_cost = heater_cost =
//        bop_cost = fossil_backup_cost =
//        direct_capital_precontingency_cost = contingency_cost = total_direct_cost = epc_and_owner_cost = total_land_cost =
//        sales_tax_cost = total_indirect_cost = total_installed_cost = estimated_installed_cost_per_cap = std::numeric_limits<double>::quiet_NaN();
//
//    // Hybrid MSPT-ETES currently not configured for PTES
//    double Q_CT_tes = 0.0;
//    double CT_tes_spec_cost = 0.0;
//
//    N_mspt::calculate_mspt_etes__no_rad_cool__costs(
//        A_sf_refl,
//        site_improv_spec_cost,
//        heliostat_spec_cost,
//        heliostat_fixed_cost,
//
//        h_tower,
//        h_rec,
//        h_helio,
//        tower_fixed_cost,
//        tower_cost_scaling_exp,
//
//        A_rec,
//        rec_ref_cost,
//        A_rec_ref,
//        rec_cost_scaling_exp,
//
//        Q_storage,
//        tes_spec_cost,
//
//        Q_CT_tes,
//        CT_tes_spec_cost,
//
//        W_dot_design,
//        power_cycle_spec_cost,
//
//        q_dot_heater_des_calc,
//        heater_spec_cost,
//
//        bop_spec_cost,
//
//        fossil_backup_spec_cost,
//
//        contingency_rate,
//
//        total_land_area,
//        plant_net_capacity,
//        EPC_land_spec_cost,
//        EPC_land_perc_direct_cost,
//        EPC_land_per_power_cost,
//        EPC_land_fixed_cost,
//        total_land_spec_cost,
//        total_land_perc_direct_cost,
//        total_land_per_power_cost,
//        total_land_fixed_cost,
//        sales_tax_basis,
//        sales_tax_rate,
//
//        site_improvement_cost,
//        heliostat_cost,
//        tower_cost,
//        receiver_cost,
//        tes_cost,
//        CT_tes_cost,
//        power_cycle_cost,
//        heater_cost,
//        bop_cost,
//        fossil_backup_cost,
//        direct_capital_precontingency_cost,
//        contingency_cost,
//        total_direct_cost,
//        total_land_cost,
//        epc_and_owner_cost,
//        sales_tax_cost,
//        total_indirect_cost,
//        total_installed_cost,
//        estimated_installed_cost_per_cap
//    );
//
//    ssc_data_t_set_number(data, "csp.pt.cost.site_improvements", (ssc_number_t)site_improvement_cost);
//    ssc_data_t_set_number(data, "csp.pt.cost.heliostats", (ssc_number_t)heliostat_cost);
//    ssc_data_t_set_number(data, "csp.pt.cost.tower", (ssc_number_t)tower_cost);
//    ssc_data_t_set_number(data, "csp.pt.cost.receiver", (ssc_number_t)receiver_cost);
//    ssc_data_t_set_number(data, "csp.pt.cost.storage", (ssc_number_t)tes_cost);
//    ssc_data_t_set_number(data, "csp.pt.cost.power_block", (ssc_number_t)power_cycle_cost);
//    ssc_data_t_set_number(data, "heater_cost_calc", (ssc_number_t)heater_cost);
//    ssc_data_t_set_number(data, "csp.pt.cost.bop", (ssc_number_t)bop_cost);
//    ssc_data_t_set_number(data, "csp.pt.cost.fossil", (ssc_number_t)fossil_backup_cost);
//    ssc_data_t_set_number(data, "ui_direct_subtotal", (ssc_number_t)direct_capital_precontingency_cost);
//    ssc_data_t_set_number(data, "csp.pt.cost.contingency", (ssc_number_t)contingency_cost);
//    ssc_data_t_set_number(data, "total_direct_cost", (ssc_number_t)total_direct_cost);
//    ssc_data_t_set_number(data, "csp.pt.cost.epc.total", (ssc_number_t)epc_and_owner_cost);
//    ssc_data_t_set_number(data, "csp.pt.cost.plm.total", (ssc_number_t)total_land_cost);
//    ssc_data_t_set_number(data, "csp.pt.cost.sales_tax.total", (ssc_number_t)sales_tax_cost);
//    ssc_data_t_set_number(data, "total_indirect_cost", (ssc_number_t)total_indirect_cost);
//    ssc_data_t_set_number(data, "total_installed_cost", (ssc_number_t)total_installed_cost);
//    ssc_data_t_set_number(data, "csp.pt.cost.installed_per_capacity", (ssc_number_t)estimated_installed_cost_per_cap);
//}
//
//
//
///////////////////////////////////////////////////////////////////////////////////////////////
//// Physical Trough //////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
//
//
//double Solar_mult(int use_solar_mult_or_aperture_area, double field_thermal_output, double q_pb_design, double specified_solar_multiple, double total_aperture, double total_required_aperture_for_SM1)
//{
//    double solar_mult = std::numeric_limits<double>::quiet_NaN();
//
//    if (use_solar_mult_or_aperture_area == -1) {
//        solar_mult = field_thermal_output / q_pb_design;
//    }
//    else if (use_solar_mult_or_aperture_area == 0) {
//        solar_mult = specified_solar_multiple;
//    }
//    else if (use_solar_mult_or_aperture_area == 1) {
//        solar_mult = total_aperture / total_required_aperture_for_SM1;
//    }
//    else {
//        throw std::runtime_error("Physical Trough. Solar multiple calculation failed, invalid option.");
//    }
//
//    return solar_mult;
//}
//
//double Max_field_flow_velocity(double m_dot_htfmax, double min_inner_diameter,
//    double T_out /*C*/, int rec_htf /*-*/, const util::matrix_t<ssc_number_t>& field_fl_props /*-*/)
//{
//    HTFProperties htf_properties = GetHtfProperties(rec_htf, field_fl_props);
//    double density = htf_properties.dens(T_out + 273.15, std::numeric_limits<double>::quiet_NaN());
//
//    return m_dot_htfmax * 4 / (density * M_PI *
//        min_inner_diameter * min_inner_diameter);
//}
//
//double Min_field_flow_velocity(double m_dot_htfmin, double min_inner_diameter,
//    double T_in /*C*/, int rec_htf /*-*/, const util::matrix_t<ssc_number_t>& field_fl_props /*-*/)
//{
//    HTFProperties htf_properties = GetHtfProperties(rec_htf, field_fl_props);
//    double density = htf_properties.dens(T_in + 273.15, std::numeric_limits<double>::quiet_NaN());
//
//    return m_dot_htfmin * 4 / (density * M_PI *
//        min_inner_diameter * min_inner_diameter);
//}
//
//double Min_htf_temp(int rec_htf /*-*/, const util::matrix_t<ssc_number_t>& field_fl_props /*-*/)       // [C]
//{
//    HTFProperties htf_properties = GetHtfProperties(rec_htf, field_fl_props);
//    return htf_properties.min_temp() - 273.15;
//}
//
//double Max_htf_temp(int rec_htf /*-*/, const util::matrix_t<ssc_number_t>& field_fl_props /*-*/)       // [C]
//{
//    HTFProperties htf_properties = GetHtfProperties(rec_htf, field_fl_props);
//    return htf_properties.max_temp() - 273.15;
//}
//
//double Field_htf_cp_avg(double T_in /*C*/, double T_out /*C*/, int rec_htf /*-*/,
//    const util::matrix_t<ssc_number_t>& field_fl_props /*-*/)      // [kJ/kg-K]
//{
//    double T_avg = 0.5 * (T_in + T_out);
//    HTFProperties htf_properties = GetHtfProperties(rec_htf, field_fl_props);
//    return htf_properties.Cp(T_avg + 273.15);
//}
//
//double Min_inner_diameter(const util::matrix_t<ssc_number_t>& trough_loop_control, const util::matrix_t<ssc_number_t>& D_2)
//{
//    double minval = D_2[0];
//    int hce_t = -1;
//    for (int i = 0; i < static_cast<int>(trough_loop_control.at(0)); i++)
//    {
//        hce_t = std::min(std::max(static_cast<int>(trough_loop_control.at(i * 3 + 2)), 1), 4) - 1;
//        if (D_2[hce_t] < minval) {
//            minval = D_2[hce_t];
//        }
//    }
//
//    return minval;
//}
//
//double Single_loop_aperature(const util::matrix_t<ssc_number_t>& trough_loop_control, const util::matrix_t<ssc_number_t>& A_aperture)
//{
//    int nsca = static_cast<int>(trough_loop_control.at(0));
//
//    double total_ap = 0.;
//    int sca_t = -1;
//    for (int i = 0; i < nsca; i++)
//    {
//        sca_t = std::min(std::max(static_cast<int>(trough_loop_control.at(1 + i * 3)), 1), 4) - 1;
//        total_ap = total_ap + A_aperture[sca_t];
//    }
//
//    return total_ap;
//}
//
//double Cspdtr_loop_hce_heat_loss(const util::matrix_t<ssc_number_t>& trough_loop_control, double I_bn_des,
//    const util::matrix_t<ssc_number_t>& csp_dtr_hce_design_heat_losses,
//    const util::matrix_t<ssc_number_t>& L_SCA,
//    const util::matrix_t<ssc_number_t>& A_aperture)
//{
//    int ncol = static_cast<int>(trough_loop_control.at(0));
//    double total_len = 0.;
//
//    double derate = 0.;
//    for (int i = 0; i < ncol; i++)
//    {
//        int sca_t = std::min(std::max(static_cast<int>(trough_loop_control.at(1 + i * 3)), 1), 4) - 1;
//        int hce_t = std::min(std::max(static_cast<int>(trough_loop_control.at(2 + i * 3)), 1), 4) - 1;
//        total_len = total_len + L_SCA[sca_t];
//        derate = derate + L_SCA[sca_t] * (1 - (csp_dtr_hce_design_heat_losses[hce_t] / (I_bn_des * A_aperture[sca_t] / L_SCA[sca_t])));
//    }
//
//    if (total_len != 0.0) {
//        derate = derate / total_len;
//    }
//    else {
//        derate = -777.7;
//    }
//
//    return derate;
//}
//
//double Nloops(int use_solar_mult_or_aperture_area, double specified_solar_multiple, double total_required_aperture_for_SM1,
//    double specified_total_aperture, double single_loop_aperture)
//{
//    double total_aperture = std::numeric_limits<double>::quiet_NaN();
//    double n_loops = std::numeric_limits<double>::quiet_NaN();
//
//    if (use_solar_mult_or_aperture_area == -1 || use_solar_mult_or_aperture_area == 0) {    // includes -1 for IPH
//        total_aperture = specified_solar_multiple * total_required_aperture_for_SM1;
//    }
//    else if (use_solar_mult_or_aperture_area == 1) {
//        total_aperture = specified_total_aperture;
//    }
//    else {
//        throw std::runtime_error("Physical Trough. Number of loops calculation failed, invalid option.");
//    }
//
//    n_loops = std::ceil(total_aperture / single_loop_aperture);
//    return n_loops;
//}
//
//double Total_aperture(double single_loop_aperature, double nloops)
//{
//    return single_loop_aperature * nloops;
//};
//
//double Required_number_of_loops_for_SM1(double total_required_aperture_for_SM1, double single_loop_aperature)
//{
//    return std::ceil(total_required_aperture_for_SM1 / single_loop_aperature);
//};
//
//double Loop_optical_efficiency(const util::matrix_t<ssc_number_t>& trough_loop_control,
//    const util::matrix_t<ssc_number_t>& csp_dtr_sca_calc_sca_effs,
//    const util::matrix_t<ssc_number_t>& L_SCA,
//    const util::matrix_t<ssc_number_t>& csp_dtr_hce_optical_effs)
//{
//    int ncol = static_cast<int>(trough_loop_control.at(0));
//
//    if (trough_loop_control.ncells() != (size_t)ncol * 3 + 1) {
//        return -888.8;
//    }
//
//    double total_len = 0.;
//    double weighted_sca_eff = 0.0;
//    for (int i = 0; i < ncol; i++)
//    {
//        int sca_t = std::min(std::max(static_cast<int>(trough_loop_control.at(1 + i * 3)), 1), 4) - 1;
//        total_len = total_len + L_SCA[sca_t];
//        weighted_sca_eff = weighted_sca_eff + L_SCA[sca_t] * csp_dtr_sca_calc_sca_effs[sca_t];
//    }
//
//    if (total_len != 0.0) {
//        weighted_sca_eff = weighted_sca_eff / total_len;
//    }
//    else {
//        weighted_sca_eff = -777.7;
//    }
//
//    total_len = 0;
//    double weighted_hce_eff = 0.0;
//    for (int i = 0; i < ncol; i++)
//    {
//        int hce_t = std::min(std::max(static_cast<int>(trough_loop_control.at(2 + i * 3)), 1), 4) - 1;
//        int sca_t = std::min(std::max(static_cast<int>(trough_loop_control.at(1 + i * 3)), 1), 4) - 1;
//        total_len = total_len + L_SCA[sca_t];
//        weighted_hce_eff = weighted_hce_eff + L_SCA[sca_t] * csp_dtr_hce_optical_effs[hce_t];
//    }
//
//    if (total_len != 0.0) {
//        weighted_hce_eff = weighted_hce_eff / total_len;
//    }
//    else {
//        weighted_hce_eff = -777.7;
//    }
//
//    return weighted_hce_eff * weighted_sca_eff;
//}
//
//double Total_loop_conversion_efficiency(double loop_optical_efficiency, double cspdtr_loop_hce_heat_loss)
//{
//    return loop_optical_efficiency * cspdtr_loop_hce_heat_loss;
//}
//
//double Field_thermal_output(double I_bn_des, double total_loop_conversion_efficiency, double total_aperture)
//{
//    return I_bn_des * total_loop_conversion_efficiency * total_aperture / 1.E6;  // default now = 626.008
//    //return solar_mult * P_ref / eta_ref;   // default = 623.596
//}
//
//double Total_required_aperture_for_sm1(double q_pb_design, double I_bn_des, double total_loop_conversion_efficiency)
//{
//    return q_pb_design / (I_bn_des * total_loop_conversion_efficiency) * 1000000.0;
//}
//
//double Fixed_land_area(double total_aperture, double row_distance, util::matrix_t<ssc_number_t> sca_info_array,
//    util::matrix_t<ssc_number_t> W_aperture)
//{
//    double max_collector_width = 0.;
//    for (size_t i = 0; i < sca_info_array.nrows(); i++) {
//        max_collector_width = std::max(max_collector_width, W_aperture.at((size_t)sca_info_array.at(i, 0) - 1));
//    }
//
//    return total_aperture * row_distance / max_collector_width * 0.0002471;
//}
//
//double Total_land_area(double fixed_land_area, double non_solar_field_land_area_multiplier)
//{
//    return fixed_land_area * non_solar_field_land_area_multiplier;
//}
//
//util::matrix_t<ssc_number_t> Sca_info_array(const util::matrix_t<ssc_number_t>& trough_loop_control)
//{
//    int assemblies = static_cast<int>(trough_loop_control.at(0));
//    util::matrix_t<ssc_number_t> p1(assemblies, 2, std::numeric_limits<double>::quiet_NaN());
//    for (int i = 0; i < assemblies; i++) {
//        p1.at(i, 1) = static_cast<int>(trough_loop_control.at(1 + 3 * i));
//        p1.at(i, 0) = static_cast<int>(trough_loop_control.at(2 + 3 * i));
//    }
//
//    return p1;
//}
//
//util::matrix_t<ssc_number_t> Sca_defocus_array(const util::matrix_t<ssc_number_t>& trough_loop_control)
//{
//    int assemblies = static_cast<int>(trough_loop_control.at(0));
//    util::matrix_t<ssc_number_t> p1(assemblies, 1, std::numeric_limits<double>::quiet_NaN());
//    for (int i = 0; i < assemblies; i++) {
//        p1.at(i) = static_cast<int>(trough_loop_control.at(3 + 3 * i));
//    }
//
//    return p1;
//}
//
//double Total_tracking_power(int nSCA, int nLoops, double SCA_drives_elec)
//{
//    return nSCA * nLoops * SCA_drives_elec;
//}
//
//util::matrix_t<ssc_number_t> K_Cpnt(int nSCA)
//{
//    std::vector<double> K_cpnt_0 = { 0.9, 0, 0.19, 0, 0.9, -1, -1, -1, -1, -1, -1 };
//    std::vector<double> K_cpnt_1 = { 0, 0.6, 0.05, 0, 0.6, 0, 0.6, 0, 0.42, 0, 0.15 };
//    std::vector<double> K_cpnt_i = { 0.05, 0, 0.42, 0, 0.6, 0, 0.6, 0, 0.42, 0, 0.15 };
//    std::vector<double> K_cpnt_x_2 = { 0.05, 0, 0.42, 0, 0.6, 0, 0.6, 0, 0.15, 0.6, 0 };
//    std::vector<double> K_cpnt_x_1 = { 0.9, 0, 0.19, 0, 0.9, -1, -1, -1, -1, -1, -1 };
//
//    util::matrix_t<ssc_number_t> K(nSCA + 3, 11, std::numeric_limits<double>::quiet_NaN());
//
//    // After cold header before SCAs
//    for (size_t j = 0; j < K_cpnt_0.size(); j++) {
//        K.at(0, j) = K_cpnt_0.at(j);
//        K.at(1, j) = K_cpnt_1.at(j);
//    }
//
//    // Between SCAs
//    for (size_t i = 0; i < (size_t)nSCA - 1; i++) {
//        for (size_t j = 0; j < K_cpnt_i.size(); j++) {
//            K.at(i + 2, j) = K_cpnt_i.at(j);
//        }
//    }
//
//    // After SCAs before hot header
//    for (size_t j = 0; j < K_cpnt_x_2.size(); j++) {
//        K.at(nSCA + 1, j) = K_cpnt_x_2.at(j);
//        K.at(nSCA + 2, j) = K_cpnt_x_1.at(j);
//    }
//
//    return K;
//}
//
//util::matrix_t<ssc_number_t> D_Cpnt(int nSCA)
//{
//    std::vector<double> D_cpnt_0 = { 0.085, 0.0635, 0.085, 0.0635, 0.085, -1, -1, -1, -1, -1, -1 };
//    std::vector<double> D_cpnt_1 = { 0.085, 0.085, 0.085, 0.0635, 0.0635, 0.0635, 0.0635, 0.0635, 0.0635, 0.0635, 0.085 };
//    std::vector<double> D_cpnt_i = { 0.085, 0.0635, 0.0635, 0.0635, 0.0635, 0.0635, 0.0635, 0.0635, 0.0635, 0.0635, 0.085 };
//    std::vector<double> D_cpnt_x_2 = { 0.085, 0.0635, 0.0635, 0.0635, 0.0635, 0.0635, 0.0635, 0.0635, 0.085, 0.085, 0.085 };
//    std::vector<double> D_cpnt_x_1 = { 0.085, 0.0635, 0.085, 0.0635, 0.085, -1, -1, -1, -1, -1, -1 };
//
//    util::matrix_t<ssc_number_t> D(nSCA + 3, 11, std::numeric_limits<double>::quiet_NaN());
//
//    // After cold header before SCAs
//    for (size_t j = 0; j < D_cpnt_0.size(); j++) {
//        D.at(0, j) = D_cpnt_0.at(j);
//        D.at(1, j) = D_cpnt_1.at(j);
//    }
//
//    // Between SCAs
//    for (size_t i = 0; i < (size_t)nSCA - 1; i++) {
//        for (size_t j = 0; j < D_cpnt_i.size(); j++) {
//            D.at(i + 2, j) = D_cpnt_i.at(j);
//        }
//    }
//
//    // After SCAs before hot header
//    for (size_t j = 0; j < D_cpnt_x_2.size(); j++) {
//        D.at(nSCA + 1, j) = D_cpnt_x_2.at(j);
//        D.at(nSCA + 2, j) = D_cpnt_x_1.at(j);
//    }
//
//    return D;
//}
//
//util::matrix_t<ssc_number_t> L_Cpnt(int nSCA)
//{
//    std::vector<double> L_cpnt_0 = { 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1 };
//    std::vector<double> L_cpnt_1 = { 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0 };
//    std::vector<double> L_cpnt_i = { 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0 };
//    std::vector<double> L_cpnt_x_2 = { 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0 };
//    std::vector<double> L_cpnt_x_1 = { 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1 };
//
//    util::matrix_t<ssc_number_t> L(nSCA + 3, 11, std::numeric_limits<double>::quiet_NaN());
//
//    // After cold header before SCAs
//    for (size_t j = 0; j < L_cpnt_0.size(); j++) {
//        L.at(0, j) = L_cpnt_0.at(j);
//        L.at(1, j) = L_cpnt_1.at(j);
//    }
//
//    // Between SCAs
//    for (size_t i = 0; i < (size_t)nSCA - 1; i++) {
//        for (size_t j = 0; j < L_cpnt_i.size(); j++) {
//            L.at(i + 2, j) = L_cpnt_i.at(j);
//        }
//    }
//
//    // After SCAs before hot header
//    for (size_t j = 0; j < L_cpnt_x_2.size(); j++) {
//        L.at(nSCA + 1, j) = L_cpnt_x_2.at(j);
//        L.at(nSCA + 2, j) = L_cpnt_x_1.at(j);
//    }
//
//    return L;
//}
//
//util::matrix_t<ssc_number_t> Type_Cpnt(int nSCA)
//{
//    std::vector<double> Type_cpnt_0 = { 0, 1, 0, 1, 0, -1, -1, -1, -1, -1, -1 };
//    std::vector<double> Type_cpnt_1 = { 1, 0, 0, 2, 0, 1, 0, 2, 0, 2, 0 };
//    std::vector<double> Type_cpnt_i = { 0, 2, 0, 2, 0, 1, 0, 2, 0, 2, 0 };
//    std::vector<double> Type_cpnt_x_2 = { 0, 2, 0, 2, 0, 1, 0, 2, 0, 0, 1 };
//    std::vector<double> Type_cpnt_x_1 = { 0, 1, 0, 1, 0, -1, -1, -1, -1, -1, -1 };
//
//    util::matrix_t<ssc_number_t> Type(nSCA + 3, 11, std::numeric_limits<double>::quiet_NaN());
//
//    // After cold header before SCAs
//    for (size_t j = 0; j < Type_cpnt_0.size(); j++) {
//        Type.at(0, j) = Type_cpnt_0.at(j);
//        Type.at(1, j) = Type_cpnt_1.at(j);
//    }
//
//    // Between SCAs
//    for (size_t i = 0; i < (size_t)nSCA - 1; i++) {
//        for (size_t j = 0; j < Type_cpnt_i.size(); j++) {
//            Type.at(i + 2, j) = Type_cpnt_i.at(j);
//        }
//    }
//
//    // After SCAs before hot header
//    for (size_t j = 0; j < Type_cpnt_x_2.size(); j++) {
//        Type.at(nSCA + 1, j) = Type_cpnt_x_2.at(j);
//        Type.at(nSCA + 2, j) = Type_cpnt_x_1.at(j);
//    }
//
//    return Type;
//}
//
//// Originally from 'Physical Trough Collector Type 1' (and 2, 3, 4)
//util::matrix_t<ssc_number_t> Csp_dtr_sca_ap_lengths(const util::matrix_t<ssc_number_t>& csp_dtr_sca_lengths, const util::matrix_t<ssc_number_t>& csp_dtr_sca_ncol_per_scas) {
//    size_t n = csp_dtr_sca_lengths.ncells();
//
//    util::matrix_t<ssc_number_t> result(n);             // NOTE!: You must do a separate 'fill', probably with how this is eventually set to an array instead of a matrix. This fails:  result(n, 1, std::numeric_limits<double>::quiet_NaN())
//    result.fill(std::numeric_limits<double>::quiet_NaN());
//    for (size_t i = 0; i < n; i++) {
//        result.at(i) = csp_dtr_sca_lengths.at(i) / csp_dtr_sca_ncol_per_scas.at(i);
//    }
//    return result;
//}
//
//util::matrix_t<ssc_number_t> Csp_dtr_sca_calc_end_gains(const util::matrix_t<ssc_number_t>& csp_dtr_sca_ave_focal_lens, double csp_dtr_sca_calc_theta, const util::matrix_t<ssc_number_t>& csp_dtr_sca_piping_dists) {
//    size_t n = csp_dtr_sca_ave_focal_lens.ncells();
//
//    util::matrix_t<ssc_number_t> result(n);             // NOTE!: You must do a separate 'fill', probably with how this is eventually set to an array instead of a matrix. This fails:  result(n, 1, std::numeric_limits<double>::quiet_NaN())
//    result.fill(std::numeric_limits<double>::quiet_NaN());
//    for (size_t i = 0; i < n; i++) {
//        result.at(i) = std::max(csp_dtr_sca_ave_focal_lens.at(i) * tan(csp_dtr_sca_calc_theta) - csp_dtr_sca_piping_dists.at(i), 0.);
//    }
//    return result;
//}
//
//double Csp_dtr_sca_calc_costh(double csp_dtr_sca_calc_zenith, double tilt, double azimuth) {
//    return  sqrt(1 - pow(cos(1.57 - csp_dtr_sca_calc_zenith - tilt)
//        - cos(tilt)
//        * cos(1.57 - csp_dtr_sca_calc_zenith)
//        * (1. - cos(0. - azimuth)), 2)
//    );
//}
//
//util::matrix_t<ssc_number_t> Csp_dtr_sca_calc_end_losses(const util::matrix_t<ssc_number_t>& csp_dtr_sca_ave_focal_lens, double csp_dtr_sca_calc_theta, double nSCA,
//    const util::matrix_t<ssc_number_t>& csp_dtr_sca_calc_end_gains, const util::matrix_t<ssc_number_t>& csp_dtr_sca_lengths, const util::matrix_t<ssc_number_t>& csp_dtr_sca_ncol_per_scas) {
//    size_t n = csp_dtr_sca_ave_focal_lens.ncells();
//
//    util::matrix_t<ssc_number_t> result(n);
//    result.fill(std::numeric_limits<double>::quiet_NaN());
//    for (size_t i = 0; i < n; i++) {
//        result.at(i) =  1 - (csp_dtr_sca_ave_focal_lens.at(i) * tan(csp_dtr_sca_calc_theta)
//            - (nSCA - 1) / nSCA * csp_dtr_sca_calc_end_gains.at(i))
//            / (csp_dtr_sca_lengths.at(i) * csp_dtr_sca_ncol_per_scas.at(i));
//    }
//    return result;
//}
//
//util::matrix_t<ssc_number_t> Csp_dtr_sca_calc_sca_effs(const util::matrix_t<ssc_number_t>& csp_dtr_sca_tracking_errors, const util::matrix_t<ssc_number_t>& csp_dtr_sca_geometry_effects,
//    const util::matrix_t<ssc_number_t>& csp_dtr_sca_clean_reflectivities, const util::matrix_t<ssc_number_t>& csp_dtr_sca_mirror_dirts, const util::matrix_t<ssc_number_t>& csp_dtr_sca_general_errors) {
//    size_t n = csp_dtr_sca_tracking_errors.ncells();
//
//    util::matrix_t<ssc_number_t> result(n);
//    result.fill(std::numeric_limits<double>::quiet_NaN());
//    for (size_t i = 0; i < n; i++) {
//        result.at(i) =  csp_dtr_sca_tracking_errors.at(i) * csp_dtr_sca_geometry_effects.at(i) *
//            csp_dtr_sca_clean_reflectivities.at(i) * csp_dtr_sca_mirror_dirts.at(i) * csp_dtr_sca_general_errors.at(i);
//    }
//    return result;
//}
//
//double Csp_dtr_sca_calc_latitude(double lat) {
//    return lat;
//}
//
//double Csp_dtr_sca_calc_zenith(double lat) {
//    return M_PI / 180. * (90. - (90. - (lat - 23.5)));
//}
//
//util::matrix_t<ssc_number_t> Csp_dtr_sca_calc_iams(const util::matrix_t<ssc_number_t>& IAMs, double csp_dtr_sca_calc_theta, double csp_dtr_sca_calc_costh) {
//
//    util::matrix_t<ssc_number_t> result(IAMs.nrows());
//    result.fill(std::numeric_limits<double>::quiet_NaN());
//    for (size_t i = 0; i < IAMs.nrows(); i++) {
//        if (IAMs.ncols() < 2) {                            // not sure this actually captures varying lengths of the different 1-D arrays in this matrix
//            result.at(i) = IAMs.at(i, 0);
//        }
//        else {
//            double IAM = IAMs.at(i, 0);
//            for (size_t j = 1; j < IAMs.ncols(); j++) {
//                IAM = IAM + IAMs.at(i, j) * pow(csp_dtr_sca_calc_theta, j) / csp_dtr_sca_calc_costh;
//            }
//            result.at(i) = IAM;
//        }
//    }
//    return result;
//}
//
//double Csp_dtr_sca_calc_theta(double csp_dtr_sca_calc_costh) {
//    return acos(csp_dtr_sca_calc_costh);
//}
//
//
//// Originally from 'Physical Trough Receiver Type 1' (and 2, 3, 4)
//util::matrix_t<ssc_number_t> Csp_dtr_hce_design_heat_losses(
//    const util::matrix_t<ssc_number_t>& HCE_FieldFrac, const util::matrix_t<ssc_number_t>& Design_loss) {
//
//    size_t n = HCE_FieldFrac.nrows();
//
//    util::matrix_t<ssc_number_t> result(n);
//    result.fill(std::numeric_limits<double>::quiet_NaN());
//    for (size_t i = 0; i < n; i++) {
//        result.at(i) =
//            HCE_FieldFrac.at(i, 0)
//            * Design_loss.at(i, 0)
//            + HCE_FieldFrac.at(i, 1)
//            * Design_loss.at(i, 1)
//            + HCE_FieldFrac.at(i, 2)
//            * Design_loss.at(i, 2)
//            + HCE_FieldFrac.at(i, 3)
//            * Design_loss.at(i, 3);
//    }
//    return result;
//}
//
//util::matrix_t<ssc_number_t> Csp_dtr_hce_optical_effs(
//    const util::matrix_t<ssc_number_t>& HCE_FieldFrac,
//    const util::matrix_t<ssc_number_t>& Shadowing,
//    const util::matrix_t<ssc_number_t>& Dirt_HCE,
//    const util::matrix_t<ssc_number_t>& alpha_abs,
//    const util::matrix_t<ssc_number_t>& Tau_envelope) {
//
//    size_t n = HCE_FieldFrac.nrows();
//
//    util::matrix_t<ssc_number_t> result(n);
//    result.fill(std::numeric_limits<double>::quiet_NaN());
//    for (size_t i = 0; i < n; i++) {
//        result.at(i) =
//            HCE_FieldFrac.at(i, 0)
//            * Shadowing.at(i, 0)
//            * Dirt_HCE.at(i, 0)
//            * alpha_abs.at(i, 0)
//            * Tau_envelope.at(i, 0)
//            + HCE_FieldFrac.at(i, 1)
//            * Shadowing.at(i, 1)
//            * Dirt_HCE.at(i, 1)
//            * alpha_abs.at(i, 1)
//            * Tau_envelope.at(i, 1)
//            + HCE_FieldFrac.at(i, 2)
//            * Shadowing.at(i, 2)
//            * Dirt_HCE.at(i, 2)
//            * alpha_abs.at(i, 2)
//            * Tau_envelope.at(i, 2)
//            + HCE_FieldFrac.at(i, 3)
//            * Shadowing.at(i, 3)
//            * Dirt_HCE.at(i, 3)
//            * alpha_abs.at(i, 3)
//            * Tau_envelope.at(i, 3);
//    }
//    return result;
//}
//
//// Originally from 'Physical Trough System Control'
//double Is_wlim_series(double is_dispatch) {
//    return is_dispatch;
//}
//
