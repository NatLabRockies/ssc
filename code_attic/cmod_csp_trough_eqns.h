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



#ifndef _CMOD_CSP_TROUGH_EQNS_H_
#define _CMOD_CSP_TROUGH_EQNS_H_

#include "sscapi.h"

#ifdef __cplusplus
extern "C" {
#endif

    static const char* Physical_Trough_System_Design_Equations_doc =
        "Sizes the design point system parameters of a power trough plant, as used on the Physical Trough System Design UI form\\n"
        "Input: var_table with key-value pairs\\n"
        "     'P_ref' - double [MWe]\\n"
        "     'gross_net_conversion_factor' - double [-]\\n"
        "     'eta_ref' - double [-]\\n"
        "Output: key-value pairs added to var_table\\n"
        "     'csp_dtr_pwrb_nameplate' - double [MWe]\\n"
        "     'q_pb_design' - double [MWt]";

    SSCEXPORT bool Physical_Trough_System_Design_Equations(ssc_data_t data);


    static const char* Physical_Trough_Solar_Field_Equations_doc =
        "Sizes and lays out the trough field for a physical power or heat trough plant, as used on the Physical Trough Solar Field UI form\\n"
        "Input: var_table with key-value pairs\\n"
        "     'T_loop_in_des' - double [C]\\n"
        "     'T_loop_out' - double [C]\\n"
        "     'Fluid' - double [-]\\n"
        "     'I_bn_des' - double [W/m2]\\n"
        "     'm_dot_htfmax' - double [kg/s]\\n"
        "     'm_dot_htfmin' - double [kg/s]\\n"
        "     'use_solar_mult_or_aperture_area' - double [-]\\n"
        "     'specified_solar_multiple' - double [-]\\n"
        "     'specified_total_aperture' - double [m2]\\n"
        "     'tshours' - double [hr]\\n"
        "     'Row_Distance' - double [m]\\n"
        "     'non_solar_field_land_area_multiplier' - double [-]\\n"
        "     'nSCA' - double [-]\\n"
        "     'SCA_drives_elec' - double [W]\\n"
        "     'q_pb_design' - double [MWt]\\n"
        "     'field_fl_props' - double [-]\\n"
        "     'trough_loop_control' - double [-]\\n"
        "     'A_aperture' - double [m2]\\n"
        "     'D_2' - double [m]\\n"
        "     'HCE_FieldFrac' - double [-]\\n"
        "     'Design_loss' - double [W/m]\\n"
        "     'L_SCA' - double [m]\\n"
        "     'TrackingError' - double [-]\\n"
        "     'GeomEffects' - double [-]\\n"
        "     'Rho_mirror_clean' - double [-]\\n"
        "     'Dirt_mirror' - double [-]\\n"
        "     'Error' - double [-]\\n"
        "     'Shadowing' - double [-]\\n"
        "     'Dirt_HCE' - double [-]\\n"
        "     'alpha_abs' - double [-]\\n"
        "     'Tau_envelope' - double [-]\\n"
        "     'W_aperture' - double [m]\\n"
        "Output: key-value pairs added to var_table\\n"
        "     'field_htf_cp_avg' - double [kJ/kg-K]\\n"
        "     'single_loop_aperature' - double [m2]\\n"
        "     'min_inner_diameter' - double [m]\\n"
        "     'cspdtr_loop_hce_heat_loss' - double [-]\\n"
        "     'loop_optical_efficiency' - double [-]\\n"
        "     'max_field_flow_velocity' - double [m/s]\\n"
        "     'min_field_flow_velocity' - double [m/s]\\n"
        "     'required_number_of_loops_for_SM1' - double [-]\\n"
        "     'total_loop_conversion_efficiency' - double [-]\\n"
        "     'total_required_aperture_for_SM1' - double [m2]\\n"
        "     'nLoops' - double [-]\\n"
        "     'total_aperture' - double [m2]\\n"
        "     'field_thermal_output' - double [MWt]\\n"
        "     'solar_mult' - double [-]\\n"
        "     'q_rec_des' - double [MWt]\\n"
        "     'tshours_sf' - double [hr]\\n"
        "     'fixed_land_area' - double [m2]\\n"
        "     'total_land_area' - double [m2]\\n"
        "     'total_tracking_power' - double [W]\\n"
        "     'csp_dtr_hce_design_heat_losses' - double [-]\\n"
        "     'csp_dtr_sca_calc_sca_effs' - double [-]\\n"
        "     'csp_dtr_hce_optical_effs' - double [-]\\n"
        "     'SCAInfoArray' - double [-]\\n"
        "     'SCADefocusArray' - double [-]\\n"
        "     'K_cpnt' - double [-]\\n"
        "     'D_cpnt' - double [m]\\n"
        "     'L_cpnt' - double [m]\\n"
        "     'Type_cpnt' - double [-]\\n";

    SSCEXPORT bool Physical_Trough_Solar_Field_Equations(ssc_data_t data);


    static const char* Physical_Trough_Collector_Type_Equations_doc =
        "\\n"
        "Input: var_table with key-value pairs\\n"
        "     'lat' - double [deg]\\n"
        "     'tilt' - double [deg]\\n"
        "     'azimuth' - double [deg]\\n"
        "     'nSCA' - double [-]\\n"
        "     'L_SCA' - double [m]\\n"
        "     'ColperSCA' - double [-]\\n"
        "     'Ave_Focal_Length' - double [m]\\n"
        "     'Distance_SCA' - double [m]\\n"
        "     'IAM_matrix' - double [-]\\n"
        "Output: key-value pairs added to var_table\\n"
        "     'csp_dtr_sca_calc_zenith' - double [deg]\\n"
        "     'csp_dtr_sca_calc_costh' - double [-]\\n"
        "     'csp_dtr_sca_calc_theta' - double [deg]\\n"
        "     'csp_dtr_sca_calc_latitude' - double [deg]\\n"
        "     'csp_dtr_sca_ap_lengths' - double [m]\\n"
        "     'csp_dtr_sca_calc_end_gains' - double []\\n"
        "     'csp_dtr_sca_calc_end_losses' - double []\\n"
        "     'csp_dtr_sca_calc_iams' - double [-]\\n";

    SSCEXPORT bool Physical_Trough_Collector_Type_Equations(ssc_data_t data);


    static const char* Physical_Trough_Collector_Type_UI_Only_Equations_doc =
        "\\n"
        "Input: var_table with key-value pairs\\n"
        "     'lat' - double [deg]\\n"
        "     'tilt' - double [deg]\\n"
        "     'azimuth' - double [deg]\\n"
        "     'nSCA' - double [-]\\n"
        "     'L_SCA' - double [m]\\n"
        "     'ColperSCA' - double [-]\\n"
        "     'Ave_Focal_Length' - double [m]\\n"
        "     'Distance_SCA' - double [m]\\n"
        "     'IAM_matrix' - double [-]\\n"
        "Output: key-value pairs added to var_table\\n"
        "     'csp_dtr_sca_calc_zenith' - double [deg]\\n"
        "     'csp_dtr_sca_calc_costh' - double [-]\\n"
        "     'csp_dtr_sca_calc_theta' - double [deg]\\n"
        "     'csp_dtr_sca_calc_latitude' - double [deg]\\n"
        "     'csp_dtr_sca_ap_lengths' - double [m]\\n"
        "     'csp_dtr_sca_calc_end_gains' - double []\\n"
        "     'csp_dtr_sca_calc_end_losses' - double []\\n"
        "     'csp_dtr_sca_calc_iams' - double [-]\\n";

    SSCEXPORT bool Physical_Trough_Collector_Type_UI_Only_Equations(ssc_data_t data);


    static const char* Physical_Trough_System_Control_Equations_doc =
        "\\n"
        "Input: var_table with key-value pairs\\n"
        "     'is_dispatch' - double [-]\\n"
        "     'disp_wlim_maxspec' - double [-]\\n"
        "     'constant' - double [%]\\n"
        "Output: key-value pairs added to var_table\\n"
        "     'is_wlim_series' - double [-]\\n"
        "     'disp_wlim_max' - double [MWe]\\n"
        "     'wlim_series' - double [kWe]\\n";

    SSCEXPORT bool Physical_Trough_System_Control_Equations(ssc_data_t data);

#ifdef __cplusplus
}
#endif

#endif
