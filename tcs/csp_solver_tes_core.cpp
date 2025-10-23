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

#include "csp_solver_tes_core.h"



void two_tank_tes_sizing(HTFProperties& tes_htf_props, double Q_tes_des /*MWt-hr*/, double T_tes_hot /*K*/,
    double T_tes_cold /*K*/, double h_min /*m*/, double h_tank_in /*m*/, int tank_pairs /*-*/, double u_tank /*W/m^2-K*/,
    double& vol_one_temp_avail /*m3*/, double& vol_one_temp_total /*m3*/, double& d_tank_out /*m*/,
    double& q_dot_loss_des /*MWt*/)
{
    double T_tes_ave = 0.5 * (T_tes_hot + T_tes_cold);		//[K]

    double rho_ave = tes_htf_props.dens(T_tes_ave, 1.0);		//[kg/m^3] Density at average temperature
    double cp_ave = tes_htf_props.Cp_ave(T_tes_cold, T_tes_hot);//[kJ/kg-K] Specific heat at average temperature

    // Volume required to supply design hours of thermal energy storage
        //[m^3] = [MJ/s-hr] * [sec]/[hr] = [MJ] / (kg/m^3 * MJ/kg-K * K 
    vol_one_temp_avail = Q_tes_des * 3600.0 / (rho_ave * cp_ave / 1000.0 * (T_tes_hot - T_tes_cold));

    // Additional volume necessary due to minimum tank limits
    vol_one_temp_total = vol_one_temp_avail / (1.0 - h_min / h_tank_in);	//[m^3]

    double A_cs = vol_one_temp_total / (h_tank_in * tank_pairs);		//[m^2] Cross-sectional area of a single tank

    d_tank_out = pow(A_cs / CSP::pi, 0.5) * 2.0;			//[m] Diameter of a single tank

    double UA_tanks_one_temp = u_tank * (A_cs + CSP::pi * d_tank_out * h_tank_in) * tank_pairs;		//[W/K]

    double T_amb_des = 15.0 + 273.15;       //[K]
    double q_dot_loss_cold = UA_tanks_one_temp * (T_tes_cold - T_amb_des) * 1.E-6;	//[MWt]
    double q_dot_loss_hot = UA_tanks_one_temp * (T_tes_hot - T_amb_des) * 1.E-6;	//[MWt]
    q_dot_loss_des = q_dot_loss_cold + q_dot_loss_hot;	//[MWt]

}

void two_tank_tes_sizing_fixed_diameter(HTFProperties& tes_htf_props, double Q_tes_des /*MWt-hr*/, double T_tes_hot /*K*/,
    double T_tes_cold /*K*/, double h_min /*m*/, double d_tank_in, int tank_pairs /*-*/, double u_tank /*W/m^2-K*/,
    double& vol_one_temp_avail /*m3*/, double& vol_one_temp_total /*m3*/, double& h_tank_out /*m*/,
    double& q_dot_loss_des /*MWt*/)
{
    double T_tes_ave = 0.5 * (T_tes_hot + T_tes_cold);		//[K]

    double rho_ave = tes_htf_props.dens(T_tes_ave, 1.0);		//[kg/m^3] Density at average temperature
    double cp_ave = tes_htf_props.Cp_ave(T_tes_cold, T_tes_hot);//[kJ/kg-K] Specific heat at average temperature

    // Volume required to supply design hours of thermal energy storage
        //[m^3] = [MJ/s-hr] * [sec]/[hr] = [MJ] / (kg/m^3 * MJ/kg-K * K 
    vol_one_temp_avail = Q_tes_des * 3600.0 / (rho_ave * cp_ave / 1000.0 * (T_tes_hot - T_tes_cold));

    double A_cs = CSP::pi * std::pow(d_tank_in, 2.0) * 0.25 ;		//[m^2] Cross-sectional area of a single tank

    // Additional volume necessary due to minimum tank limits
    double vol_added_min = A_cs * h_min * tank_pairs; //[m^3]

    // Total Volume
    vol_one_temp_total = vol_one_temp_avail + vol_added_min;	//[m^3]

    // Tank Height
    h_tank_out = vol_one_temp_total / (A_cs * tank_pairs); // [m]

    double UA_tanks_one_temp = u_tank * (A_cs + CSP::pi * d_tank_in * h_tank_out) * tank_pairs;		//[W/K]

    double T_amb_des = 15.0 + 273.15;       //[K]
    double q_dot_loss_cold = UA_tanks_one_temp * (T_tes_cold - T_amb_des) * 1.E-6;	//[MWt]
    double q_dot_loss_hot = UA_tanks_one_temp * (T_tes_hot - T_amb_des) * 1.E-6;	//[MWt]
    q_dot_loss_des = q_dot_loss_cold + q_dot_loss_hot;	//[MWt]

}

void piston_cylinder_tes_sizing(HTFProperties& tes_htf_props, double Q_tes_des /*MWt-hr*/, double T_tes_hot /*K*/,
    double T_tes_cold /*K*/, double h_min /*m*/, double h_tank_in /*m*/, int tank_pairs /*-*/, double u_tank /*W/m^2-K*/,
    double& vol_one_temp_avail /*m3*/, double& vol_one_temp_total /*m3*/, double& d_tank_out /*m*/,
    double& q_dot_loss_des /*MWt*/)
{
    double T_tes_ave = 0.5 * (T_tes_hot + T_tes_cold);		//[K]

    double rho_ave = tes_htf_props.dens(T_tes_ave, 1.0);		//[kg/m^3] Density at average temperature
    double cp_ave = tes_htf_props.Cp_ave(T_tes_cold, T_tes_hot);//[kJ/kg-K] Specific heat at average temperature

    // Calculate required mass to store design hours of thermal energy storage
    double mass_avail = (Q_tes_des * 3600.0) / (cp_ave * 1E-3 * (T_tes_hot - T_tes_cold));

    // Calculate Main Tank Volume using hot density (so full charge can store hot density)
    vol_one_temp_avail = mass_avail / tes_htf_props.dens(T_tes_hot, 1.0);


    // Volume required to supply design hours of thermal energy storage
        //[m^3] = [MJ/s-hr] * [sec]/[hr] = [MJ] / (kg/m^3 * MJ/kg-K * K 
    double vol_one_temp_avail_orig = Q_tes_des * 3600.0 / (rho_ave * cp_ave / 1000.0 * (T_tes_hot - T_tes_cold));

    // Additional volume necessary due to minimum tank limits
    vol_one_temp_total = vol_one_temp_avail / (1.0 - h_min / h_tank_in);	//[m^3]

    double A_cs = vol_one_temp_total / (h_tank_in * tank_pairs);		//[m^2] Cross-sectional area of a single tank

    d_tank_out = pow(A_cs / CSP::pi, 0.5) * 2.0;			//[m] Diameter of a single tank

    double UA_tanks_one_temp = u_tank * (A_cs + CSP::pi * d_tank_out * h_tank_in) * tank_pairs;		//[W/K]

    double T_amb_des = 15.0 + 273.15;       //[K]
    double q_dot_loss_cold = UA_tanks_one_temp * (T_tes_cold - T_amb_des) * 1.E-6;	//[MWt]
    double q_dot_loss_hot = UA_tanks_one_temp * (T_tes_hot - T_amb_des) * 1.E-6;	//[MWt]
    q_dot_loss_des = q_dot_loss_cold + q_dot_loss_hot;	//[MWt]

}

void piston_cylinder_tes_sizing_fixed_diameter(HTFProperties& tes_htf_props, double Q_tes_des /*MWt-hr*/, double T_tes_hot /*K*/,
    double T_tes_cold /*K*/, double h_min /*m*/, double d_tank_in, int tank_pairs /*-*/, double u_tank /*W/m^2-K*/,
    double& vol_one_temp_avail /*m3*/, double& vol_one_temp_total /*m3*/, double& h_tank_out /*m*/,
    double& q_dot_loss_des /*MWt*/)
{
    double T_tes_ave = 0.5 * (T_tes_hot + T_tes_cold);		//[K]

    double rho_ave = tes_htf_props.dens(T_tes_ave, 1.0);		//[kg/m^3] Density at average temperature
    double cp_ave = tes_htf_props.Cp_ave(T_tes_cold, T_tes_hot);//[kJ/kg-K] Specific heat at average temperature

    // Calculate required mass to store design hours of thermal energy storage
    double mass_avail = (Q_tes_des * 3600.0) / (cp_ave * 1E-3 * (T_tes_hot - T_tes_cold));

    // Calculate Main Tank Volume using hot density (so full charge can store hot density)
    vol_one_temp_avail = mass_avail / tes_htf_props.dens(T_tes_hot, 1.0);

    // Volume required to supply design hours of thermal energy storage
        //[m^3] = [MJ/s-hr] * [sec]/[hr] = [MJ] / (kg/m^3 * MJ/kg-K * K 
    //vol_one_temp_avail = Q_tes_des * 3600.0 / (rho_ave * cp_ave / 1000.0 * (T_tes_hot - T_tes_cold));

    double A_cs = CSP::pi * std::pow(d_tank_in, 2.0) * 0.25;		//[m^2] Cross-sectional area of a single tank

    // Additional volume necessary due to minimum tank limits
    double vol_added_min = A_cs * h_min * tank_pairs; //[m^3]

    // Total Volume
    vol_one_temp_total = vol_one_temp_avail + vol_added_min;	//[m^3]

    // Tank Height
    h_tank_out = vol_one_temp_total / (A_cs * tank_pairs); // [m]

    double UA_tanks_one_temp = u_tank * (A_cs + CSP::pi * d_tank_in * h_tank_out) * tank_pairs;		//[W/K]

    double T_amb_des = 15.0 + 273.15;       //[K]
    double q_dot_loss_cold = UA_tanks_one_temp * (T_tes_cold - T_amb_des) * 1.E-6;	//[MWt]
    double q_dot_loss_hot = UA_tanks_one_temp * (T_tes_hot - T_amb_des) * 1.E-6;	//[MWt]
    q_dot_loss_des = q_dot_loss_cold + q_dot_loss_hot;	//[MWt]

}


int size_tes_piping(double vel_dsn, util::matrix_t<double> L, double rho_avg, double m_dot_pb, double solarm,
    bool tanks_in_parallel, double& vol_tot, util::matrix_t<double>& v_dot_rel, util::matrix_t<double>& diams,
    util::matrix_t<double>& wall_thk, util::matrix_t<double>& m_dot, util::matrix_t<double>& vel, bool custom_sizes)
{
    const std::size_t bypass_index = 4;
    const std::size_t gen_first_index = 5;      // first generation section index in combined col. gen. loops
    double m_dot_sf;
    double v_dot_src, v_dot_sink;               // source and sink vol. flow rates
    double v_dot_ref;
    double v_dot;                               // volumetric flow rate
    double Area;
    vol_tot = 0.0;                              // total volume in SGS piping
    std::size_t nPipes = L.ncells();
    v_dot_rel.resize_fill(nPipes, 0.0);         // volumetric flow rate relative to the source or sink flow
    m_dot.resize_fill(nPipes, 0.0);
    vel.resize_fill(nPipes, 0.0);
    std::vector<int> sections_no_bypass;
    if (!custom_sizes) {
        diams.resize_fill(nPipes, 0.0);
        wall_thk.resize_fill(nPipes, 0.0);
    }

    m_dot_sf = m_dot_pb * solarm;
    v_dot_src = m_dot_sf / rho_avg;
    v_dot_sink = m_dot_pb / rho_avg;

    //The volumetric flow rate relative to the source for each collection section (v_rel = v_dot / v_dot_sink)
    v_dot_rel.at(0) = 1.0 / 2;                  // 1 - Source pump suction header to individual source pump inlet
    //     50% -> "/2.0" . The flow rate (i.e., diameter) is sized here for the case when one pump is down.
    v_dot_rel.at(1) = 1.0 / 2;                  // 2 - Individual SF pump discharge to SF pump discharge header
    v_dot_rel.at(2) = 1.0;                      // 3 - Source pump discharge header to collection source section headers (i.e., runners)
    v_dot_rel.at(3) = 1.0;                      // 4 - Source section outlet headers (i.e., runners) to expansion vessel (indirect storage) or
    //     hot thermal storage tank (direct storage)
    v_dot_rel.at(4) = 1.0;                      // 5 - Bypass branch - Source section outlet headers (i.e., runners) to pump suction header (indirect) or
    //     cold thermal storage tank (direct)

//The volumetric flow rate relative to the power block for each generation section
    v_dot_rel.at(5) = 1.0 / 2;                  // 2 - SGS pump suction header to individual SGS pump inlet (applicable only for storage in series with SF)
    //     50% -> "/2.0" . The flow rate (i.e., diameter) is sized here for the case when one pump is down.
    v_dot_rel.at(6) = 1.0 / 2;                  // 3 - Individual SGS pump discharge to SGS pump discharge header (only for series storage)
    v_dot_rel.at(7) = 1.0;                      // 4 - SGS pump discharge header to steam generator supply header (only for series storage)

    v_dot_rel.at(8) = 1.0;                      // 5 - Steam generator supply header to inter-steam generator piping
    v_dot_rel.at(9) = 1.0;                      // 6 - Inter-steam generator piping to steam generator outlet header
    v_dot_rel.at(10) = 1.0;                     // 7 - Steam generator outlet header to SF pump suction header (indirect) or cold thermal storage tank (direct)

    if (tanks_in_parallel) {
        sections_no_bypass = { 0, 1, 2, 3, 8, 9, 10 };
    }
    else {  // tanks in series
        sections_no_bypass = { 0, 1, 2, 3, 5, 6, 7, 8, 9, 10 };
    }

    // Collection loop followed by generation loop
    for (std::size_t i = 0; i < nPipes; i++) {
        if (L.at(i) > 0) {
            i < gen_first_index ? v_dot_ref = v_dot_src : v_dot_ref = v_dot_sink;
            v_dot = v_dot_ref * v_dot_rel.at(i);
            if (!custom_sizes) {
                diams.at(i) = CSP::pipe_sched(sqrt(4.0 * v_dot / (vel_dsn * CSP::pi)));
                wall_thk.at(i) = CSP::WallThickness(diams.at(i));
            }
            m_dot.at(i) = v_dot * rho_avg;
            Area = CSP::pi * pow(diams.at(i), 2) / 4.;
            vel.at(i) = v_dot / Area;

            // Calculate total volume, excluding bypass branch
            if (std::find(sections_no_bypass.begin(), sections_no_bypass.end(), i) != sections_no_bypass.end()) {
                vol_tot += Area * L.at(i);
            }
        }
    }

    return 0;
}

int size_tes_piping_TandP(HTFProperties& external_htf_props, double T_src_in, double T_src_out, double P_src_in, double dP_discharge,
    const util::matrix_t<double>& L, const util::matrix_t<double>& k_tes_loss_coeffs, double pipe_rough,
    bool tanks_in_parallel, const util::matrix_t<double>& diams, const util::matrix_t<double>& vel,
    util::matrix_t<double>& TES_T_des, util::matrix_t<double>& TES_P_des, double& TES_P_in)
{
    std::size_t nPipes = L.ncells();
    TES_T_des.resize_fill(nPipes, 0.0);
    TES_P_des.resize_fill(nPipes, 0.0);

    // Calculate Design Temperatures, in C
    TES_T_des.at(0) = T_src_in - 273.15;
    TES_T_des.at(1) = T_src_in - 273.15;
    TES_T_des.at(2) = T_src_in - 273.15;
    TES_T_des.at(3) = T_src_out - 273.15;
    TES_T_des.at(4) = T_src_out - 273.15;
    if (tanks_in_parallel) {
        TES_T_des.at(5) = 0;
        TES_T_des.at(6) = 0;
        TES_T_des.at(7) = 0;
    }
    else {
        TES_T_des.at(5) = T_src_out - 273.15;
        TES_T_des.at(6) = T_src_out - 273.15;
        TES_T_des.at(7) = T_src_out - 273.15;
    }
    TES_T_des.at(8) = T_src_out - 273.15;
    TES_T_des.at(9) = T_src_in - 273.15;
    TES_T_des.at(10) = T_src_in - 273.15;


    // Calculate Design Pressures, in Pa
    double ff;
    double rho_avg = external_htf_props.dens((T_src_in + T_src_out) / 2, 9 / 1.e-5);
    const double P_hi = 17 / 1.e-5;               // downstream SF pump pressure [Pa]
    const double P_lo = 1 / 1.e-5;                // atmospheric pressure [Pa]

    // P_10
    ff = CSP::FrictionFactor(pipe_rough / diams.at(10), external_htf_props.Re(TES_T_des.at(10), P_lo, vel.at(10), diams.at(10)));
    TES_P_des.at(10) = 0 +
        CSP::MajorPressureDrop(vel.at(10), rho_avg, ff, L.at(10), diams.at(10)) +
        CSP::MinorPressureDrop(vel.at(10), rho_avg, k_tes_loss_coeffs.at(10));

    // P_9
    ff = CSP::FrictionFactor(pipe_rough / diams.at(9), external_htf_props.Re(TES_T_des.at(9), P_lo, vel.at(9), diams.at(9)));
    TES_P_des.at(9) = TES_P_des.at(10) +
        CSP::MajorPressureDrop(vel.at(9), rho_avg, ff, L.at(9), diams.at(9)) +
        CSP::MinorPressureDrop(vel.at(9), rho_avg, k_tes_loss_coeffs.at(9));

    // P_8
    ff = CSP::FrictionFactor(pipe_rough / diams.at(8), external_htf_props.Re(TES_T_des.at(8), P_hi, vel.at(8), diams.at(8)));
    TES_P_des.at(8) = TES_P_des.at(9) + dP_discharge +
        CSP::MajorPressureDrop(vel.at(8), rho_avg, ff, L.at(8), diams.at(8)) +
        CSP::MinorPressureDrop(vel.at(8), rho_avg, k_tes_loss_coeffs.at(8));

    if (tanks_in_parallel) {
        TES_P_des.at(7) = 0;
        TES_P_des.at(6) = 0;
        TES_P_des.at(5) = 0;
    }
    else {
        // P_7
        ff = CSP::FrictionFactor(pipe_rough / diams.at(7), external_htf_props.Re(TES_T_des.at(7), P_hi, vel.at(7), diams.at(7)));
        TES_P_des.at(7) = TES_P_des.at(8) +
            CSP::MajorPressureDrop(vel.at(7), rho_avg, ff, L.at(7), diams.at(7)) +
            CSP::MinorPressureDrop(vel.at(7), rho_avg, k_tes_loss_coeffs.at(7));

        // P_6
        ff = CSP::FrictionFactor(pipe_rough / diams.at(6), external_htf_props.Re(TES_T_des.at(6), P_hi, vel.at(6), diams.at(6)));
        TES_P_des.at(6) = TES_P_des.at(7) +
            CSP::MajorPressureDrop(vel.at(6), rho_avg, ff, L.at(6), diams.at(6)) +
            CSP::MinorPressureDrop(vel.at(6), rho_avg, k_tes_loss_coeffs.at(6));

        // P_5
        TES_P_des.at(5) = 0;
    }

    // P_3
    ff = CSP::FrictionFactor(pipe_rough / diams.at(3), external_htf_props.Re(TES_T_des.at(3), P_lo, vel.at(3), diams.at(3)));
    TES_P_des.at(3) = 0 +
        CSP::MajorPressureDrop(vel.at(3), rho_avg, ff, L.at(3), diams.at(3)) +
        CSP::MinorPressureDrop(vel.at(3), rho_avg, k_tes_loss_coeffs.at(3));

    // P_4
    TES_P_des.at(4) = TES_P_des.at(3);

    // P_2
    ff = CSP::FrictionFactor(pipe_rough / diams.at(2), external_htf_props.Re(TES_T_des.at(2), P_hi, vel.at(2), diams.at(2)));
    TES_P_des.at(2) = P_src_in +
        CSP::MajorPressureDrop(vel.at(2), rho_avg, ff, L.at(2), diams.at(2)) +
        CSP::MinorPressureDrop(vel.at(2), rho_avg, k_tes_loss_coeffs.at(2));

    // P_1
    ff = CSP::FrictionFactor(pipe_rough / diams.at(1), external_htf_props.Re(TES_T_des.at(1), P_hi, vel.at(1), diams.at(1)));
    TES_P_des.at(1) = TES_P_des.at(2) +
        CSP::MajorPressureDrop(vel.at(1), rho_avg, ff, L.at(1), diams.at(1)) +
        CSP::MinorPressureDrop(vel.at(1), rho_avg, k_tes_loss_coeffs.at(1));

    // P_0
    TES_P_des.at(0) = 0;

    // Convert Pa to bar
    for (int i = 0; i < nPipes; i++) {
        TES_P_des.at(i) = TES_P_des.at(i) / 1.e5;
    }
    TES_P_in = TES_P_des.at(3);     // pressure at the inlet to the TES, at the source side

    return 0;
}
