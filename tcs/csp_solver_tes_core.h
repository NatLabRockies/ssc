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

#ifndef __csp_solver_tes_core_
#define __csp_solver_tes_core_

#include "csp_solver_core.h"
#include "csp_solver_util.h"

#include "sam_csp_util.h"

void two_tank_tes_sizing(HTFProperties& tes_htf_props, double Q_tes_des /*MWt-hr*/, double T_tes_hot /*K*/,
    double T_tes_cold /*K*/, double h_min /*m*/, double h_tank_in /*m*/, int tank_pairs /*-*/, double u_tank /*W/m^2-K*/,
    double& vol_one_temp_avail /*m3*/, double& vol_one_temp_total /*m3*/, double& d_tank_out /*m*/,
    double& q_dot_loss_des /*MWt*/);

void two_tank_tes_sizing_fixed_diameter(HTFProperties& tes_htf_props, double Q_tes_des /*MWt-hr*/, double T_tes_hot /*K*/,
    double T_tes_cold /*K*/, double h_min /*m*/, double d_tank_in, int tank_pairs /*-*/, double u_tank /*W/m^2-K*/,
    double& vol_one_temp_avail /*m3*/, double& vol_one_temp_total /*m3*/, double& h_tank_out /*m*/,
    double& q_dot_loss_des /*MWt*/);

void piston_cylinder_tes_sizing(HTFProperties& tes_htf_props, double Q_tes_des /*MWt-hr*/, double T_tes_hot /*K*/,
    double T_tes_cold /*K*/, double h_min /*m*/, double h_tank_in /*m*/, int tank_pairs /*-*/, double u_tank /*W/m^2-K*/,
    double& vol_one_temp_avail /*m3*/, double& vol_one_temp_total /*m3*/, double& d_tank_out /*m*/,
    double& q_dot_loss_des /*MWt*/);

void piston_cylinder_tes_sizing_fixed_diameter(HTFProperties& tes_htf_props, double Q_tes_des /*MWt-hr*/, double T_tes_hot /*K*/,
    double T_tes_cold /*K*/, double h_min /*m*/, double d_tank_in, int tank_pairs /*-*/, double u_tank /*W/m^2-K*/,
    double& vol_one_temp_avail /*m3*/, double& vol_one_temp_total /*m3*/, double& h_tank_out /*m*/,
    double& q_dot_loss_des /*MWt*/);



int size_tes_piping(double vel_dsn, util::matrix_t<double> L, double rho_avg, double m_dot_pb, double solarm,
    bool tanks_in_parallel, double& vol_tot, util::matrix_t<double>& v_dot_rel, util::matrix_t<double>& diams,
    util::matrix_t<double>& wall_thk, util::matrix_t<double>& m_dot, util::matrix_t<double>& vel, bool custom_sizes = false);

int size_tes_piping_TandP(HTFProperties& external_htf_props, double T_src_in /*K*/, double T_src_out /*K*/, double P_src_in /*Pa*/, double dP_discharge,
    const util::matrix_t<double>& L, const util::matrix_t<double>& k_tes_loss_coeffs, double pipe_rough,
    bool tanks_in_parallel, const util::matrix_t<double>& diams, const util::matrix_t<double>& vel,
    util::matrix_t<double>& TES_T_des, util::matrix_t<double>& TES_P_des, double& TES_P_in);

#endif   //__csp_solver_tes_core_
