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



#ifndef _CMOD_CSP_COMMON_EQNS_H_
#define _CMOD_CSP_COMMON_EQNS_H_

#include "sscapi.h"
#include "../shared/lib_util.h"
#include "htf_props.h"
#include "vartab.h"


enum class TowerTypes {
    kMoltenSalt,
    kDirectSteam,
    kIscc,
};


SSCEXPORT ssc_bool_t ssc_data_t_get_number(ssc_data_t p_data, const char* name, ssc_number_t* value);

SSCEXPORT void ssc_data_t_set_number(ssc_data_t p_data, const char* name, ssc_number_t value);

SSCEXPORT ssc_number_t* ssc_data_t_get_array(ssc_data_t p_data, const char* name, int* length);

SSCEXPORT void ssc_data_t_set_array(ssc_data_t p_data, const char* name, ssc_number_t* pvalues, int length);

SSCEXPORT void ssc_data_t_get_matrix(var_table* vt, std::string name, util::matrix_t<double>& matrix);

SSCEXPORT void ssc_data_t_set_matrix(ssc_data_t data, const std::string& name, const var_data& val);


//HTFProperties GetHtfProperties(int fluid_number, const util::matrix_t<double> &specified_fluid_properties);       // [-]
//
//
//
//// Originally from 'MSPT System Design' UI Form
//double Nameplate(double P_ref /*MWe*/, double gross_net_conversion_factor /*-*/);       // [MWe]
//
//double Q_pb_design(double P_ref /*MWe*/, double design_eff /*-*/);      // [MWt]
//
//double Q_rec_des(double solarm /*-*/, double q_pb_design /*MWt*/);      // [MWt]
//
//double Tshours_sf(double tshours /*hr*/, double solarm /*-*/);          // [hr]
//
//
//
//// Originally from 'Tower SolarPilot Solar Field' UI Form
//double Land_max_calc(double land_max /*-*/, double h_tower /*m*/);      // [m]
//
//int N_hel(const util::matrix_t<ssc_number_t> &helio_positions /*m*/);      // [-]
//
//double Csp_pt_sf_heliostat_area(double helio_height /*m*/, double helio_width /*m*/, double dens_mirror /*-*/);     // [m2]
//
//double Csp_pt_sf_total_reflective_area(int n_hel /*-*/, double csp_pt_sf_heliostat_area /*m2*/);     // [m2]
//
//double Land_min_calc(double land_min /*-*/, double h_tower /*m*/);      // [m]
//
//double Csp_pt_sf_total_land_area(double csp_pt_sf_fixed_land_area /*acres*/, double land_area_base /*acres*/,
//    double csp_pt_sf_land_overhead_factor /*-*/);       // [acres]
//
//double A_sf_UI(double helio_width /*m*/, double helio_height /*m*/, double dens_mirror /*-*/, int n_hel /*-*/);  // [m2]
//
//double Helio_area_tot(double A_sf_UI /*m2*/);     // [m2]
//
//double Csp_pt_sf_tower_height(double h_tower /*m*/);        // [m]
//
//double C_atm_info(const util::matrix_t<ssc_number_t> &helio_positions /*m*/,
//    double c_atm_0 /*-*/, double c_atm_1 /*-*/, double c_atm_2 /*-*/, double c_atm_3 /*-*/, double h_tower /*m*/);  // [%]
//
//double Error_equiv(double helio_optical_error_mrad /*mrad*/);       // [mrad]
//
//bool Is_optimize(bool override_opt /*-*/);      // [-]
//
//int Field_model_type(bool is_optimize /*-*/, bool override_layout /*-*/, int assigned_field_model_type /*-*/);      // [-]
//
//double Q_design(double Q_rec_des /*MWt*/);      // [MWt]
//
//double Dni_des_calc(double dni_des /*W/m2*/);       // [W/m2]
//
//
//// Originally from 'MSPT Receiver' UI Form
//double Csp_pt_rec_max_flow_to_rec(double csp_pt_rec_max_oper_frac /*-*/, double Q_rec_des /*MWt*/,
//    double csp_pt_rec_htf_c_avg /*kJ/kg-K*/, double T_htf_hot_des /*C*/, double T_htf_cold_des /*C*/);      // [kg/s]
//
//double Csp_pt_rec_htf_t_avg(double T_htf_cold_des /*C*/, double T_htf_hot_des /*C*/);       // [C]
//
//double Csp_pt_rec_htf_c_avg(double csp_pt_rec_htf_t_avg /*C*/, int rec_htf /*-*/,
//    const util::matrix_t<ssc_number_t> &field_fl_props /*-*/);      // [kJ/kg-K]
//
//double Rec_aspect(double D_rec /*m*/, double rec_height /*m*/);     // [-]
//
//
//// Originally from 'MSPT System Control'
//double Csp_pt_par_calc_bop(double bop_par /*MWe/MWcap*/, double bop_par_f /*-*/, double bop_par_0 /*-*/,
//    double bop_par_1 /*-*/, double bop_par_2 /*-*/, double p_ref /*MWe*/);      // [MWe]
//
//double Csp_pt_par_calc_aux(double aux_par /*MWe/MWcap*/, double aux_par_f /*-*/, double aux_par_0 /*-*/,
//    double aux_par_1 /*-*/, double aux_par_2 /*-*/, double p_ref /*MWe*/);      // [MWe]
//
//double Disp_wlim_max(double disp_wlim_maxspec /**/, double constant /*%*/);        // [MWe]
//
//util::matrix_t<double> Wlim_series(double disp_wlim_max /*MWe*/);    // [kWe]
//
//
//
//// Originally from 'Tower SolarPilot Capital Costs'
////double Ui_tower_height(TowerTypes tower_type, double height);
//
//void Csp_pt_cost_receiver_area(TowerTypes tower_type /*-*/, double d_rec /*m*/,
//    double rec_height /*m*/, int receiver_type /*-*/, double cav_rec_height /*m*/,
//    double cav_rec_width /*m*/, double rec_span_deg /*deg*/, int n_cav_panels,
//    double& area /*m2*/, double& cav_panel_width /*m*/,
//    double& cav_radius /*m*/, double& cav_offset);        // [m2]
//
//double Csp_pt_cost_storage_mwht(TowerTypes tower_type /*-*/, double p_ref = std::numeric_limits<double>::quiet_NaN() /*MWe*/,
//    double design_eff = std::numeric_limits<double>::quiet_NaN() /*-*/,
//    double tshours = std::numeric_limits<double>::quiet_NaN() /*hr*/);      // [MWht]
//
//double Csp_pt_cost_power_block_mwe(TowerTypes tower_type /*-*/, double p_ref = std::numeric_limits<double>::quiet_NaN() /*MWe*/,
//    double demand_var = std::numeric_limits<double>::quiet_NaN()) /*MWe*/;      // [MWe]
//
//void Tower_SolarPilot_Capital_Costs_Equations(ssc_data_t data);
//
//
//
///////////////////////////////////////////////////////////////////////////////////////////////
//// Physical Trough //////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
//
//// Originally from 'Physical Trough System Design'
//double Solar_mult(int use_solar_mult_or_aperture_area, double field_thermal_output, double q_pb_design, double specified_solar_multiple, double total_aperture, double total_required_aperture_for_SM1);
//
//double Nloops(int use_solar_mult_or_aperture_area, double specified_solar_multiple, double total_required_aperture_for_SM1,
//    double specified_total_aperture, double single_loop_aperature);
//
//double Max_field_flow_velocity(double m_dot_htfmax, double min_inner_diameter,
//    double T_out /*C*/, int rec_htf /*-*/, const util::matrix_t<ssc_number_t>& field_fl_props /*-*/);
//
//double Min_field_flow_velocity(double m_dot_htfmin, double min_inner_diameter,
//    double T_in /*C*/, int rec_htf /*-*/, const util::matrix_t<ssc_number_t>& field_fl_props /*-*/);
//
//double Min_htf_temp(int rec_htf /*-*/, const util::matrix_t<ssc_number_t>& field_fl_props /*-*/);   // [C]
//
//double Max_htf_temp(int rec_htf /*-*/, const util::matrix_t<ssc_number_t>& field_fl_props /*-*/);   // [C]
//
//double Field_htf_cp_avg(double T_in /*C*/, double T_out /*C*/, int rec_htf /*-*/,
//    const util::matrix_t<ssc_number_t>& field_fl_props /*-*/);      // [kJ/kg-K]
//
//double Min_inner_diameter(const util::matrix_t<ssc_number_t>& trough_loop_control, const util::matrix_t<ssc_number_t>& D_2);
//
//double Single_loop_aperature(const util::matrix_t<ssc_number_t>& trough_loop_control, const util::matrix_t<ssc_number_t>& A_aperture);
//
//double Cspdtr_loop_hce_heat_loss(const util::matrix_t<ssc_number_t>& trough_loop_control, double I_bn_des,
//    const util::matrix_t<ssc_number_t>& csp_dtr_hce_design_heat_losses,
//    const util::matrix_t<ssc_number_t>& L_SCA,
//    const util::matrix_t<ssc_number_t>& A_aperture);
//
//double Total_aperture(double single_loop_aperature, double nloops);
//
//double Required_number_of_loops_for_SM1(double total_required_aperture_for_SM1, double single_loop_aperature);
//
//double Loop_optical_efficiency(const util::matrix_t<ssc_number_t>& trough_loop_control,
//    const util::matrix_t<ssc_number_t>& csp_dtr_sca_calc_sca_effs,
//    const util::matrix_t<ssc_number_t>& L_SCA,
//    const util::matrix_t<ssc_number_t>& csp_dtr_hce_optical_effs);
//
//double Total_loop_conversion_efficiency(double loop_optical_efficiency, double cspdtr_loop_hce_heat_loss);
//
//double Field_thermal_output(double I_bn_des, double total_loop_conversion_efficiency, double total_aperture);
//
//double Total_required_aperture_for_sm1(double q_pb_design, double I_bn_des, double total_loop_conversion_efficiency);
//
//double Fixed_land_area(double total_aperture, double row_distance, util::matrix_t<ssc_number_t> sca_info_array,
//    util::matrix_t<ssc_number_t> W_aperture);
//
//double Total_land_area(double fixed_land_area, double non_solar_field_land_area_multiplier);
//
//util::matrix_t<ssc_number_t> Sca_info_array(const util::matrix_t<ssc_number_t>& trough_loop_control);
//
//util::matrix_t<ssc_number_t> Sca_defocus_array(const util::matrix_t<ssc_number_t>& trough_loop_control);
//
//double Total_tracking_power(int nSCA, int nLoops, double SCA_drives_elec);
//
//util::matrix_t<ssc_number_t> K_Cpnt(int nSCA);
//
//util::matrix_t<ssc_number_t> D_Cpnt(int nSCA);
//
//util::matrix_t<ssc_number_t> L_Cpnt(int nSCA);
//
//util::matrix_t<ssc_number_t> Type_Cpnt(int nSCA);
//
//
//// Originally from 'Physical Trough Collector Type 1' (and 2, 3, 4)
//util::matrix_t<ssc_number_t> Csp_dtr_sca_ap_lengths(const util::matrix_t<ssc_number_t>& csp_dtr_sca_lengths, const util::matrix_t<ssc_number_t>& csp_dtr_sca_ncol_per_scas);
//
////double Csp_dtr_sca_calc_end_gain(double csp_dtr_sca_ave_focal_len, double csp_dtr_sca_calc_theta, double csp_dtr_sca_piping_dist);
//
//util::matrix_t<ssc_number_t> Csp_dtr_sca_calc_end_gains(const util::matrix_t<ssc_number_t>& csp_dtr_sca_ave_focal_lens, double csp_dtr_sca_calc_theta, const util::matrix_t<ssc_number_t>& csp_dtr_sca_piping_dists);
//
//double Csp_dtr_sca_calc_costh(double csp_dtr_sca_calc_zenith, double tilt, double azimuth);
//
//util::matrix_t<ssc_number_t> Csp_dtr_sca_calc_end_losses(const util::matrix_t<ssc_number_t>& csp_dtr_sca_ave_focal_lens, double csp_dtr_sca_calc_theta, double nSCA,
//    const util::matrix_t<ssc_number_t>& csp_dtr_sca_calc_end_gains, const util::matrix_t<ssc_number_t>& csp_dtr_sca_lengths, const util::matrix_t<ssc_number_t>& csp_dtr_sca_ncol_per_scas);
//
//util::matrix_t<ssc_number_t> Csp_dtr_sca_calc_sca_effs(const util::matrix_t<ssc_number_t>& csp_dtr_sca_tracking_errors, const util::matrix_t<ssc_number_t>& csp_dtr_sca_geometry_effects,
//    const util::matrix_t<ssc_number_t>& csp_dtr_sca_clean_reflectivities, const util::matrix_t<ssc_number_t>& csp_dtr_sca_mirror_dirts, const util::matrix_t<ssc_number_t>& csp_dtr_sca_general_errors);
//
//double Csp_dtr_sca_calc_latitude(double lat);
//
//double Csp_dtr_sca_calc_zenith(double lat);
//
//util::matrix_t<ssc_number_t> Csp_dtr_sca_calc_iams(const util::matrix_t<ssc_number_t>& IAMs, double csp_dtr_sca_calc_theta, double csp_dtr_sca_calc_costh);
//
//double Csp_dtr_sca_calc_theta(double csp_dtr_sca_calc_costh);
//
//
//// Originally from 'Physical Trough Receiver Type 1' (and 2, 3, 4)
//util::matrix_t<ssc_number_t> Csp_dtr_hce_design_heat_losses(
//    const util::matrix_t<ssc_number_t>& HCE_FieldFrac,
//    const util::matrix_t<ssc_number_t>& Design_loss);
//
//util::matrix_t<ssc_number_t> Csp_dtr_hce_optical_effs(
//    const util::matrix_t<ssc_number_t>& HCE_FieldFrac,
//    const util::matrix_t<ssc_number_t>& Shadowing,
//    const util::matrix_t<ssc_number_t>& Dirt_HCE,
//    const util::matrix_t<ssc_number_t>& alpha_abs,
//    const util::matrix_t<ssc_number_t>& Tau_envelope);
//
//// Originally from 'Physical Trough System Control'
//double Is_wlim_series(double is_dispatch);

#endif
