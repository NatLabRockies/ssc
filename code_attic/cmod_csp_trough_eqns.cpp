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


#include "cmod_csp_trough_eqns.h"
#include "cmod_csp_common_eqns.h"
#include "vartab.h"
#include <cmath>

#pragma warning(disable: 4297)  // ignore warning: 'function assumed not to throw an exception but does'


bool Physical_Trough_System_Design_Equations(ssc_data_t data)
{
    auto vt = static_cast<var_table*>(data);
    if (!vt) {
        return false;
    }

    // Inputs
    double P_ref = std::numeric_limits<double>::quiet_NaN();
    double gross_net_conversion_factor = std::numeric_limits<double>::quiet_NaN();
    double eta_ref = std::numeric_limits<double>::quiet_NaN();

    // Outputs
    double csp_dtr_pwrb_nameplate = std::numeric_limits<double>::quiet_NaN();
    double q_pb_design = std::numeric_limits<double>::quiet_NaN();

    // csp_dtr_pwrb_nameplate
    ssc_data_t_get_number(data, "P_ref", &P_ref);
    ssc_data_t_get_number(data, "gross_net_conversion_factor", &gross_net_conversion_factor);
    csp_dtr_pwrb_nameplate = Nameplate(P_ref, gross_net_conversion_factor);
    ssc_data_t_set_number(data, "csp_dtr_pwrb_nameplate", csp_dtr_pwrb_nameplate);

    // q_pb_design
    ssc_data_t_get_number(data, "eta_ref", &eta_ref);
    q_pb_design = Q_pb_design(P_ref, eta_ref);
    ssc_data_t_set_number(data, "q_pb_design", q_pb_design);
    return true;
}

bool Physical_Trough_Solar_Field_Equations(ssc_data_t data)
{
    auto vt = static_cast<var_table*>(data);
    if (!vt) {
        return false;
    }

    // Inputs
    double T_loop_in_des = std::numeric_limits<double>::quiet_NaN();
    double T_loop_out = std::numeric_limits<double>::quiet_NaN();
    double Fluid = std::numeric_limits<double>::quiet_NaN();
    double I_bn_des = std::numeric_limits<double>::quiet_NaN();
    double m_dot_htfmax = std::numeric_limits<double>::quiet_NaN();
    double m_dot_htfmin = std::numeric_limits<double>::quiet_NaN();
    double use_solar_mult_or_aperture_area = std::numeric_limits<double>::quiet_NaN();
    double specified_solar_multiple = std::numeric_limits<double>::quiet_NaN();
    double specified_total_aperture = std::numeric_limits<double>::quiet_NaN();
    double tshours = std::numeric_limits<double>::quiet_NaN();
    double Row_Distance = std::numeric_limits<double>::quiet_NaN();
    double non_solar_field_land_area_multiplier = std::numeric_limits<double>::quiet_NaN();
    double nSCA = std::numeric_limits<double>::quiet_NaN();
    double SCA_drives_elec = std::numeric_limits<double>::quiet_NaN();
    double q_pb_design = std::numeric_limits<double>::quiet_NaN();
    util::matrix_t<ssc_number_t> field_fl_props(1, 1, std::numeric_limits<double>::quiet_NaN());
    util::matrix_t<ssc_number_t> trough_loop_control(1, 1, std::numeric_limits<double>::quiet_NaN());
    util::matrix_t<ssc_number_t> A_aperture(1, 1, std::numeric_limits<double>::quiet_NaN());
    util::matrix_t<ssc_number_t> D_2(1, 1, std::numeric_limits<double>::quiet_NaN());
    util::matrix_t<ssc_number_t> HCE_FieldFrac(1, 1, std::numeric_limits<double>::quiet_NaN());
    util::matrix_t<ssc_number_t> Design_loss(1, 1, std::numeric_limits<double>::quiet_NaN());
    util::matrix_t<ssc_number_t> L_SCA(1, 1, std::numeric_limits<double>::quiet_NaN());
    util::matrix_t<ssc_number_t> TrackingError(1, 1, std::numeric_limits<double>::quiet_NaN());
    util::matrix_t<ssc_number_t> GeomEffects(1, 1, std::numeric_limits<double>::quiet_NaN());
    util::matrix_t<ssc_number_t> Rho_mirror_clean(1, 1, std::numeric_limits<double>::quiet_NaN());
    util::matrix_t<ssc_number_t> Dirt_mirror(1, 1, std::numeric_limits<double>::quiet_NaN());
    util::matrix_t<ssc_number_t> Error(1, 1, std::numeric_limits<double>::quiet_NaN());
    util::matrix_t<ssc_number_t> Shadowing(1, 1, std::numeric_limits<double>::quiet_NaN());
    util::matrix_t<ssc_number_t> Dirt_HCE(1, 1, std::numeric_limits<double>::quiet_NaN());
    util::matrix_t<ssc_number_t> alpha_abs(1, 1, std::numeric_limits<double>::quiet_NaN());
    util::matrix_t<ssc_number_t> Tau_envelope(1, 1, std::numeric_limits<double>::quiet_NaN());
    util::matrix_t<ssc_number_t> W_aperture(1, 1, std::numeric_limits<double>::quiet_NaN());


    // Outputs
    double field_htf_min_operating_temp = std::numeric_limits<double>::quiet_NaN();
    double field_htf_max_operating_temp = std::numeric_limits<double>::quiet_NaN();
    double field_htf_cp_avg = std::numeric_limits<double>::quiet_NaN();
    double single_loop_aperature = std::numeric_limits<double>::quiet_NaN();
    double min_inner_diameter = std::numeric_limits<double>::quiet_NaN();
    double cspdtr_loop_hce_heat_loss = std::numeric_limits<double>::quiet_NaN();
    double loop_optical_efficiency = std::numeric_limits<double>::quiet_NaN();
    double max_field_flow_velocity = std::numeric_limits<double>::quiet_NaN();
    double min_field_flow_velocity = std::numeric_limits<double>::quiet_NaN();
    double required_number_of_loops_for_SM1 = std::numeric_limits<double>::quiet_NaN();
    double total_loop_conversion_efficiency = std::numeric_limits<double>::quiet_NaN();
    double total_required_aperture_for_SM1 = std::numeric_limits<double>::quiet_NaN();
    double nLoops = std::numeric_limits<double>::quiet_NaN();
    double total_aperture = std::numeric_limits<double>::quiet_NaN();
    double field_thermal_output = std::numeric_limits<double>::quiet_NaN();
    double solar_mult = std::numeric_limits<double>::quiet_NaN();
    double q_rec_des = std::numeric_limits<double>::quiet_NaN();
    double tshours_sf = std::numeric_limits<double>::quiet_NaN();
    double fixed_land_area = std::numeric_limits<double>::quiet_NaN();
    double total_land_area = std::numeric_limits<double>::quiet_NaN();
    double total_tracking_power = std::numeric_limits<double>::quiet_NaN();
    util::matrix_t<ssc_number_t> csp_dtr_hce_design_heat_losses(1, 1, std::numeric_limits<double>::quiet_NaN());
    util::matrix_t<ssc_number_t> csp_dtr_sca_calc_sca_effs(1, 1, std::numeric_limits<double>::quiet_NaN());
    util::matrix_t<ssc_number_t> csp_dtr_hce_optical_effs(1, 1, std::numeric_limits<double>::quiet_NaN());
    util::matrix_t<ssc_number_t> SCAInfoArray(1, 1, std::numeric_limits<double>::quiet_NaN());
    util::matrix_t<ssc_number_t> SCADefocusArray(1, 1, std::numeric_limits<double>::quiet_NaN());
    util::matrix_t<ssc_number_t> K_cpnt(1, 1, std::numeric_limits<double>::quiet_NaN());
    util::matrix_t<ssc_number_t> D_cpnt(1, 1, std::numeric_limits<double>::quiet_NaN());
    util::matrix_t<ssc_number_t> L_cpnt(1, 1, std::numeric_limits<double>::quiet_NaN());
    util::matrix_t<ssc_number_t> Type_cpnt(1, 1, std::numeric_limits<double>::quiet_NaN());



    // field_htf_min_operating_temp
    ssc_data_t_get_number(data, "Fluid", &Fluid);
    ssc_data_t_get_matrix(vt, "field_fl_props", field_fl_props);
    try {
        field_htf_min_operating_temp = Min_htf_temp((int)Fluid, field_fl_props);
    }
    catch (...) {
        field_htf_min_operating_temp = std::numeric_limits<double>::quiet_NaN();
    }
    ssc_data_t_set_number(data, "field_htf_min_operating_temp", field_htf_min_operating_temp);

    // field_htf_max_operating_temp
    try {
        field_htf_max_operating_temp = Max_htf_temp((int)Fluid, field_fl_props);
    }
    catch (...) {
        field_htf_max_operating_temp = std::numeric_limits<double>::quiet_NaN();
    }
    ssc_data_t_set_number(data, "field_htf_max_operating_temp", field_htf_max_operating_temp);

    // field_htf_cp_avg
    ssc_data_t_get_number(data, "T_loop_in_des", &T_loop_in_des);
    ssc_data_t_get_number(data, "T_loop_out", &T_loop_out);
    try {
        field_htf_cp_avg = Field_htf_cp_avg(T_loop_in_des, T_loop_out, (int)Fluid, field_fl_props);      // [kJ/kg-K]
    }
    catch (...) {
        field_htf_cp_avg = std::numeric_limits<double>::quiet_NaN();
    }
    ssc_data_t_set_number(data, "field_htf_cp_avg", field_htf_cp_avg);

    // single_loop_aperature
    ssc_data_t_get_matrix(vt, "trough_loop_control", trough_loop_control);
    ssc_data_t_get_matrix(vt, "A_aperture", A_aperture);
    single_loop_aperature = Single_loop_aperature(trough_loop_control, A_aperture);
    ssc_data_t_set_number(data, "single_loop_aperature", single_loop_aperature);

    // min_inner_diameter
    ssc_data_t_get_matrix(vt, "D_2", D_2);
    min_inner_diameter = Min_inner_diameter(trough_loop_control, D_2);
    ssc_data_t_set_number(data, "min_inner_diameter", min_inner_diameter);

    // csp_dtr_hce_design_heat_loss_1
    ssc_data_t_get_matrix(vt, "HCE_FieldFrac", HCE_FieldFrac);
    ssc_data_t_get_matrix(vt, "Design_loss", Design_loss);
    csp_dtr_hce_design_heat_losses = Csp_dtr_hce_design_heat_losses(HCE_FieldFrac, Design_loss);
    ssc_data_t_set_matrix(data, "csp_dtr_hce_design_heat_losses", csp_dtr_hce_design_heat_losses);

    // cspdtr_loop_hce_heat_loss
    ssc_data_t_get_number(data, "I_bn_des", &I_bn_des);
    ssc_data_t_get_matrix(vt, "L_SCA", L_SCA);
    cspdtr_loop_hce_heat_loss = Cspdtr_loop_hce_heat_loss(trough_loop_control, I_bn_des,
        csp_dtr_hce_design_heat_losses,
        L_SCA, A_aperture);
    ssc_data_t_set_number(data, "cspdtr_loop_hce_heat_loss", cspdtr_loop_hce_heat_loss);

    // csp_dtr_sca_calc_sca_eff
    ssc_data_t_get_matrix(vt, "TrackingError", TrackingError);
    ssc_data_t_get_matrix(vt, "GeomEffects", GeomEffects);
    ssc_data_t_get_matrix(vt, "Rho_mirror_clean", Rho_mirror_clean);
    ssc_data_t_get_matrix(vt, "Dirt_mirror", Dirt_mirror);
    ssc_data_t_get_matrix(vt, "Error", Error);
    csp_dtr_sca_calc_sca_effs = Csp_dtr_sca_calc_sca_effs(TrackingError, GeomEffects,
        Rho_mirror_clean, Dirt_mirror, Error);
    ssc_data_t_set_matrix(data, "csp_dtr_sca_calc_sca_effs", csp_dtr_sca_calc_sca_effs);

    // csp_dtr_hce_optical_eff_1
    ssc_data_t_get_matrix(vt, "Shadowing", Shadowing);
    ssc_data_t_get_matrix(vt, "Dirt_HCE", Dirt_HCE);
    ssc_data_t_get_matrix(vt, "alpha_abs", alpha_abs);
    ssc_data_t_get_matrix(vt, "Tau_envelope", Tau_envelope);
    csp_dtr_hce_optical_effs = Csp_dtr_hce_optical_effs(HCE_FieldFrac, Shadowing, Dirt_HCE, alpha_abs, Tau_envelope);
    ssc_data_t_set_matrix(data, "csp_dtr_hce_optical_effs", csp_dtr_hce_optical_effs);

    // loop_optical_efficiency
    loop_optical_efficiency = Loop_optical_efficiency(trough_loop_control,
        csp_dtr_sca_calc_sca_effs,
        L_SCA,
        csp_dtr_hce_optical_effs);
    ssc_data_t_set_number(data, "loop_optical_efficiency", loop_optical_efficiency);

    // sca_info_array
    SCAInfoArray = Sca_info_array(trough_loop_control);
    ssc_data_t_set_matrix(data, "scainfoarray", SCAInfoArray);

    // sca_defocus_array
    SCADefocusArray = Sca_defocus_array(trough_loop_control);
    ssc_data_t_set_array(data, "scadefocusarray", SCADefocusArray.data(), (int)SCADefocusArray.ncells());

    // max_field_flow_velocity
    ssc_data_t_get_number(data, "m_dot_htfmax", &m_dot_htfmax);
    try {
        max_field_flow_velocity = Max_field_flow_velocity(m_dot_htfmax, min_inner_diameter,
            T_loop_out, (int)Fluid, field_fl_props);
    }
    catch (...) {
        max_field_flow_velocity = std::numeric_limits<double>::quiet_NaN();
    }
    ssc_data_t_set_number(data, "max_field_flow_velocity", max_field_flow_velocity);

    // min_field_flow_velocity
    ssc_data_t_get_number(data, "m_dot_htfmin", &m_dot_htfmin);
    try {
        min_field_flow_velocity = Min_field_flow_velocity(m_dot_htfmin, min_inner_diameter,
            T_loop_in_des, (int)Fluid, field_fl_props);
    }
    catch (...) {
        min_field_flow_velocity = std::numeric_limits<double>::quiet_NaN();
    }
    ssc_data_t_set_number(data, "min_field_flow_velocity", min_field_flow_velocity);

    // total_loop_conversion_efficiency
    total_loop_conversion_efficiency = Total_loop_conversion_efficiency(loop_optical_efficiency, cspdtr_loop_hce_heat_loss);
    ssc_data_t_set_number(data, "total_loop_conversion_efficiency", total_loop_conversion_efficiency);

    // total_required_aperture_for_SM1
    ssc_data_t_get_number(data, "q_pb_design", &q_pb_design);
    total_required_aperture_for_SM1 = Total_required_aperture_for_sm1(q_pb_design, I_bn_des, total_loop_conversion_efficiency);
    ssc_data_t_set_number(data, "total_required_aperture_for_sm1", total_required_aperture_for_SM1);

    // nloops
    ssc_data_t_get_number(data, "use_solar_mult_or_aperture_area", &use_solar_mult_or_aperture_area);
    if (std::isnan(use_solar_mult_or_aperture_area)) {
        use_solar_mult_or_aperture_area = -1.;                   // IPH model
    }
    ssc_data_t_get_number(data, "specified_solar_multiple", &specified_solar_multiple);
    ssc_data_t_get_number(data, "specified_total_aperture", &specified_total_aperture);
    nLoops = Nloops(static_cast<int>(use_solar_mult_or_aperture_area), specified_solar_multiple, total_required_aperture_for_SM1, specified_total_aperture, single_loop_aperature);
    ssc_data_t_set_number(data, "nloops", nLoops);

    // total_aperture
    total_aperture = Total_aperture(single_loop_aperature, nLoops);
    ssc_data_t_set_number(data, "total_aperture", total_aperture);

    // field_thermal_output
    field_thermal_output = Field_thermal_output(I_bn_des, total_loop_conversion_efficiency, total_aperture);
    ssc_data_t_set_number(data, "field_thermal_output", field_thermal_output);

    // solar_mult
    solar_mult = Solar_mult(static_cast<int>(use_solar_mult_or_aperture_area), field_thermal_output, q_pb_design, specified_solar_multiple, total_aperture, total_required_aperture_for_SM1);
    ssc_data_t_set_number(data, "solar_mult", solar_mult);

    // required_number_of_loops_for_SM1
    required_number_of_loops_for_SM1 = Required_number_of_loops_for_SM1(total_required_aperture_for_SM1, single_loop_aperature);
    ssc_data_t_set_number(data, "required_number_of_loops_for_sm1", required_number_of_loops_for_SM1);

    // Q_rec_des
    q_rec_des = Q_rec_des(solar_mult, q_pb_design);
    ssc_data_t_set_number(data, "q_rec_des", q_rec_des);

    // tshours_sf
    ssc_data_t_get_number(data, "tshours", &tshours);
    tshours_sf = Tshours_sf(tshours, solar_mult);
    ssc_data_t_set_number(data, "tshours_sf", tshours_sf);

    // fixed_land_area
    ssc_data_t_get_number(data, "Row_Distance", &Row_Distance);
    ssc_data_t_get_matrix(vt, "W_aperture", W_aperture);
    fixed_land_area = Fixed_land_area(total_aperture, Row_Distance, SCAInfoArray, W_aperture);
    ssc_data_t_set_number(data, "fixed_land_area", fixed_land_area);

    // total_land_area
    ssc_data_t_get_number(data, "non_solar_field_land_area_multiplier", &non_solar_field_land_area_multiplier);
    total_land_area = Total_land_area(fixed_land_area, non_solar_field_land_area_multiplier);
    ssc_data_t_set_number(data, "total_land_area", total_land_area);

    // total_tracking_power
    ssc_data_t_get_number(data, "nSCA", &nSCA);
    ssc_data_t_get_number(data, "SCA_drives_elec", &SCA_drives_elec);
    total_tracking_power = Total_tracking_power(static_cast<int>(nSCA), static_cast<int>(nLoops), SCA_drives_elec);
    ssc_data_t_set_number(data, "total_tracking_power", total_tracking_power);

    // K_cpnt
    K_cpnt = K_Cpnt(static_cast<int>(nSCA));
    ssc_data_t_set_matrix(data, "k_cpnt", K_cpnt);

    // D_cpnt
    D_cpnt = D_Cpnt(static_cast<int>(nSCA));
    ssc_data_t_set_matrix(data, "d_cpnt", D_cpnt);

    // L_cpnt
    L_cpnt = L_Cpnt(static_cast<int>(nSCA));
    ssc_data_t_set_matrix(data, "l_cpnt", L_cpnt);

    // Type_cpnt
    Type_cpnt = Type_Cpnt(static_cast<int>(nSCA));
    ssc_data_t_set_matrix(data, "type_cpnt", Type_cpnt);
    return true;
}

bool Physical_Trough_Collector_Type_Equations(ssc_data_t data)
{
    auto vt = static_cast<var_table*>(data);
    if (!vt) {
        return false;
    }
    return true;
}

bool Physical_Trough_Collector_Type_UI_Only_Equations(ssc_data_t data)
{
    auto vt = static_cast<var_table*>(data);
    if (!vt) {
        return false;
    }

    // Inputs
    double lat = std::numeric_limits<double>::quiet_NaN();
    double tilt = std::numeric_limits<double>::quiet_NaN();
    double azimuth = std::numeric_limits<double>::quiet_NaN();
    double nSCA = std::numeric_limits<double>::quiet_NaN();
    util::matrix_t<ssc_number_t> L_SCA(1, 1, std::numeric_limits<double>::quiet_NaN());
    util::matrix_t<ssc_number_t> ColperSCA(1, 1, std::numeric_limits<double>::quiet_NaN());
    util::matrix_t<ssc_number_t> Ave_Focal_Length(1, 1, std::numeric_limits<double>::quiet_NaN());
    util::matrix_t<ssc_number_t> Distance_SCA(1, 1, std::numeric_limits<double>::quiet_NaN());
    util::matrix_t<ssc_number_t> IAM_matrix(1, 1, std::numeric_limits<double>::quiet_NaN());

    // Outputs
    double csp_dtr_sca_calc_zenith = std::numeric_limits<double>::quiet_NaN();            // singular
    double csp_dtr_sca_calc_costh = std::numeric_limits<double>::quiet_NaN();             // singular
    double csp_dtr_sca_calc_theta = std::numeric_limits<double>::quiet_NaN();             // singular
    double csp_dtr_sca_calc_latitude = std::numeric_limits<double>::quiet_NaN();          // singular
    util::matrix_t<ssc_number_t> csp_dtr_sca_ap_lengths(1, 1, std::numeric_limits<double>::quiet_NaN());
    util::matrix_t<ssc_number_t> csp_dtr_sca_calc_end_gains(1, 1, std::numeric_limits<double>::quiet_NaN());
    util::matrix_t<ssc_number_t> csp_dtr_sca_calc_end_losses(1, 1, std::numeric_limits<double>::quiet_NaN());
    util::matrix_t<ssc_number_t> csp_dtr_sca_calc_iams(1, 1, std::numeric_limits<double>::quiet_NaN());


    // csp_dtr_sca_ap_lengths
    ssc_data_t_get_matrix(vt, "L_SCA", L_SCA);
    ssc_data_t_get_matrix(vt, "ColperSCA", ColperSCA);
    csp_dtr_sca_ap_lengths = Csp_dtr_sca_ap_lengths(L_SCA, ColperSCA);
    ssc_data_t_set_matrix(data, "csp_dtr_sca_ap_lengths", csp_dtr_sca_ap_lengths);

    // csp_dtr_sca_calc_zenith
    ssc_data_t_get_number(data, "lat", &lat);
    csp_dtr_sca_calc_zenith = Csp_dtr_sca_calc_zenith(lat);
    ssc_data_t_set_number(data, "csp_dtr_sca_calc_zenith", csp_dtr_sca_calc_zenith);

    // csp_dtr_sca_calc_costh
    ssc_data_t_get_number(data, "tilt", &tilt);
    ssc_data_t_get_number(data, "azimuth", &azimuth);
    csp_dtr_sca_calc_costh = Csp_dtr_sca_calc_costh(csp_dtr_sca_calc_zenith, tilt, azimuth);
    ssc_data_t_set_number(data, "csp_dtr_sca_calc_costh", csp_dtr_sca_calc_costh);

    // csp_dtr_sca_calc_theta
    csp_dtr_sca_calc_theta = Csp_dtr_sca_calc_theta(csp_dtr_sca_calc_costh);
    ssc_data_t_set_number(data, "csp_dtr_sca_calc_theta", csp_dtr_sca_calc_theta);

    // csp_dtr_sca_calc_end_gain
    ssc_data_t_get_matrix(vt, "Ave_Focal_Length", Ave_Focal_Length);
    ssc_data_t_get_matrix(vt, "Distance_SCA", Distance_SCA);
    csp_dtr_sca_calc_end_gains = Csp_dtr_sca_calc_end_gains(Ave_Focal_Length, csp_dtr_sca_calc_theta, Distance_SCA);
    ssc_data_t_set_matrix(data, "csp_dtr_sca_calc_end_gains", csp_dtr_sca_calc_end_gains);

    // csp_dtr_sca_calc_end_loss
    ssc_data_t_get_number(data, "nSCA", &nSCA);
    csp_dtr_sca_calc_end_losses = Csp_dtr_sca_calc_end_losses(Ave_Focal_Length, csp_dtr_sca_calc_theta, nSCA,
        csp_dtr_sca_calc_end_gains, L_SCA, ColperSCA);
    ssc_data_t_set_matrix(data, "csp_dtr_sca_calc_end_losses", csp_dtr_sca_calc_end_losses);

    // csp_dtr_sca_calc_latitude
    csp_dtr_sca_calc_latitude = Csp_dtr_sca_calc_latitude(lat);
    ssc_data_t_set_number(data, "csp_dtr_sca_calc_latitude", csp_dtr_sca_calc_latitude);

    // csp_dtr_sca_calc_iam
    ssc_data_t_get_matrix(vt, "IAM_matrix", IAM_matrix);
    csp_dtr_sca_calc_iams = Csp_dtr_sca_calc_iams(IAM_matrix, csp_dtr_sca_calc_theta, csp_dtr_sca_calc_costh);
    ssc_data_t_set_matrix(data, "csp_dtr_sca_calc_iams", csp_dtr_sca_calc_iams);
    return true;
}

bool Physical_Trough_System_Control_Equations(ssc_data_t data)
{
    auto vt = static_cast<var_table*>(data);
    if (!vt) {
        return false;
    }

    // Inputs
    double is_dispatch = std::numeric_limits<double>::quiet_NaN();
    double disp_wlim_maxspec = std::numeric_limits<double>::quiet_NaN();
    double constant = std::numeric_limits<double>::quiet_NaN();

    // Outputs
    double is_wlim_series = std::numeric_limits<double>::quiet_NaN();
    double disp_wlim_max = std::numeric_limits<double>::quiet_NaN();
    util::matrix_t<ssc_number_t> wlim_series(1, 1, std::numeric_limits<double>::quiet_NaN());

    // is_wlim_series
    ssc_data_t_get_number(data, "is_dispatch", &is_dispatch);
    is_wlim_series = Is_wlim_series(is_dispatch);
    ssc_data_t_set_number(data, "is_wlim_series", is_wlim_series);

    // disp_wlim_max
    ssc_data_t_get_number(data, "disp_wlim_maxspec", &disp_wlim_maxspec);       // if coming from UI
    if (std::isnan(disp_wlim_maxspec)) {
        disp_wlim_maxspec = 1.;                                                 // not passed to LK script
    }
    ssc_data_t_get_number(data, "constant", &constant);                         // if coming from UI
    if (std::isnan(constant)) {
        ssc_data_t_get_number(data, "adjust:constant", &constant);              // if coming from LK script
    }
    disp_wlim_max = Disp_wlim_max(disp_wlim_maxspec, constant);
    ssc_data_t_set_number(data, "disp_wlim_max", disp_wlim_max);

    // wlim_series
    if (!vt->is_assigned("wlim_series")) {
        wlim_series = Wlim_series(disp_wlim_max);
        ssc_data_t_set_array(data, "wlim_series", wlim_series.data(), (int)wlim_series.ncells());
    }
    return true;
}
