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

#include "csp_solver_falling_particle_receiver.h"
#include "csp_solver_core.h"
#include "sam_csp_util.h"

#include "Ambient.h"
#include "definitions.h"

#include "../splinter/QR"


C_falling_particle_receiver::C_falling_particle_receiver(double h_tower /*m*/, 
    double T_htf_hot_des /*C*/, double T_htf_cold_des /*C*/,
    double f_rec_min /*-*/, double q_dot_rec_des /*MWt*/,
    double rec_su_delay /*hr*/, double rec_qf_delay /*-*/,
    double m_dot_htf_max_frac /*-*/, double eta_pump /*-*/,
    int field_fl, util::matrix_t<double> field_fl_props,
    int model_type /*-*/, double fixed_efficiency /*-*/, int rad_model_type /*-*/, int hadv_model_type /*-*/, double hadv_user  /*-*/,
    double ap_height /*m*/, double ap_width /*m*/, double ap_height_ratio /*-*/, double ap_width_ratio /*-*/, double ap_curtain_depth_ratio /*-*/, double rec_orientation /*deg*/,
    double particle_dp /*m*/, double particle_abs /*-*/, double curtain_emis /*-*/, double dthdy /*-*/, 
    double cav_abs /*-*/, double cav_emis /*-*/, double cav_twall /*m*/, double cav_kwall /*m*/, double cav_hext /*W/m2/K*/,
    double deltaT_transport_cold /*K*/, double deltaT_transport_hot /*K*/,
    double tauc_mult /*-*/, double hadv_mult /*-*/,
    int n_x, int n_y, int n_x_rad, int n_y_rad,
    double T_hot_target /*C*/, double csky_frac /*-*/) : C_pt_receiver(h_tower, 0.0,
        T_htf_hot_des, T_htf_cold_des,
        f_rec_min, q_dot_rec_des,
        rec_su_delay, rec_qf_delay,
        m_dot_htf_max_frac, eta_pump,
        0.0, 0.0,
        0.0, 0.0, 0.0,
        field_fl, field_fl_props,
        2, 0)
{
    // Parameters not shared upstream with C_pt_receiver

    // Model specs
    m_model_type = model_type;
    m_fixed_efficiency = fixed_efficiency;
    m_rad_model_type = rad_model_type;
    m_vf_rad_type_0 = 0.9;
    m_hadv_model_type = hadv_model_type;
    m_hadv_user = hadv_user;
    m_hadv_mult = hadv_mult;

    // Cavity and curtain geometry
    m_ap_height = ap_height;
    m_ap_width = ap_width;
    m_ap_height_ratio = ap_height_ratio;
    m_ap_width_ratio = ap_width_ratio;
    m_ap_curtain_depth_ratio = ap_curtain_depth_ratio;
    m_rec_orientation = rec_orientation;
    if (m_rec_orientation < 0)
        m_rec_orientation += 360;  // Relative wind directions require receiver orientation within [0,360]

    m_is_ap_at_bot = true;          // Hard-coded to true to match SolarPILOT
    m_is_curtain_flat = true;       // Curved curtain is not implemented yet
    m_curtain_rad = std::numeric_limits<double>::quiet_NaN(); // Curved curtain is not implemented yet
    m_initial_fall_height_fraction = 1. / 12.;   //Gonzalez et al.Solar Energy 213 (2021) 211–224(https://doi.org/10.1016/j.solener.2020.11.012)
    m_initial_fall_height_fixed = 0.3;


    // Particle and curtain properties
    m_particle_dp = particle_dp;
    m_particle_abs = particle_abs;
    m_curtain_emis = curtain_emis;
    m_dthdy = dthdy;
    m_tauc_mult = tauc_mult;
    m_phi0 = 0.6;

    // Cavity wall properties
    m_cav_abs = cav_abs;
    m_cav_emis = cav_emis;
    m_cav_twall = cav_twall;
    m_cav_kwall = cav_kwall;
    m_cav_hext = cav_hext;

    // Particle transport thermal loss
    m_deltaT_transport_hot = deltaT_transport_hot;
    m_deltaT_transport_cold = deltaT_transport_cold;

    // Operating parameters
    m_T_particle_hot_target = T_hot_target + 273.15;   //[K] convert from C
    m_csky_frac = csky_frac;  

    // Model, flow control, and discretization
    m_n_x = n_x;
    m_n_y = n_y;
    m_n_zone_control = 1; // Multiple flow control zones are not implemented yet

    m_n_x_rad = min(n_x_rad, m_n_x);  
    m_n_y_rad = min(n_y_rad, m_n_y-1);


    // Hard-coded parameters
    m_tol_od = 0.001;		    //[-] Tolerance for over-design iteration
    m_eta_therm_des_est = 0.9;  //[-] Estimated and used to calculate min incident flux
    m_include_back_wall_convection = false;
    m_include_wall_axial_conduction = false;
    
    m_use_eigen = false;         // Use Eigen for matrix inversion or solving systems of linear equations in the radiative exchange model
    m_invert_matrices = true;    // Invert coefficient matrix for radiative exchange? If false a linear equation solver will be used instead (this is somewhat slower because the same coefficient matrix is used multiple times in any temperature solution iteration)
    if (!m_use_eigen)
        m_invert_matrices = true;



    // Calculated parameters
    m_curtain_height = std::numeric_limits<double>::quiet_NaN();
    m_curtain_width = std::numeric_limits<double>::quiet_NaN();
    m_cav_width = std::numeric_limits<double>::quiet_NaN();
    m_ap_area = std::numeric_limits<double>::quiet_NaN();
    m_curtain_dist = std::numeric_limits<double>::quiet_NaN();
    m_cav_front_area = std::numeric_limits<double>::quiet_NaN();
    m_curtain_area = std::numeric_limits<double>::quiet_NaN();
    m_curtain_elem_area = std::numeric_limits<double>::quiet_NaN();
    m_back_wall_elem_area = std::numeric_limits<double>::quiet_NaN();
    m_vf_curtain_ap_avg = std::numeric_limits<double>::quiet_NaN();

    // State variables
    m_E_su_prev = std::numeric_limits<double>::quiet_NaN();
    m_t_su_prev = std::numeric_limits<double>::quiet_NaN();
    m_E_su = std::numeric_limits<double>::quiet_NaN();
	m_t_su = std::numeric_limits<double>::quiet_NaN();

    // Control variables
    m_is_mode_fixed_to_input_mode = false;
    m_fixed_mode_mflow_method = 0;  // Hard-coded for now, needs to match value in collector-receiver class
    m_m_dot_htf_fixed = std::numeric_limits<double>::quiet_NaN();


    // Stored solutions
    m_n_max_stored_solns = 10;


	m_ncall = -1;

}


void C_falling_particle_receiver::init()
{

    C_pt_receiver::init();

    if (m_n_zone_control > 1)
    {
        throw(C_csp_exception("Flow control in more than one zone is not currently implemented", "Particle receiver initialization"));
    }

    m_curtain_height = m_ap_height_ratio * m_ap_height;
    m_cav_width = m_ap_width_ratio * m_ap_width;
    m_curtain_width = m_cav_width;
    if (!m_is_curtain_flat)
    {
        double curtain_angle = 2 * asin(0.5 * m_cav_width / m_curtain_rad);
        m_curtain_width = m_curtain_rad * curtain_angle;
        if (curtain_angle > CSP::pi)
        {
            throw(C_csp_exception("Calculated particle curtain angle exceeds 180 degrees", "Particle receiver initialization"));
        }
        throw(C_csp_exception("Curved particle curtain is not currently implemented", "Particle receiver initialization"));
    }
    m_curtain_area = m_curtain_height * m_curtain_width;
    m_curtain_elem_area = (m_curtain_height / (m_n_y - 1)) * (m_curtain_width / m_n_x);   // Curtain element area [m2]
    m_back_wall_elem_area = m_curtain_elem_area;                                          // Back wall element area [m2]

    m_curtain_dist = m_ap_curtain_depth_ratio * m_ap_height;
    m_cav_front_area = 2 * (m_curtain_height + m_cav_width) * m_curtain_dist + m_curtain_height * (m_cav_width - m_ap_width) + m_ap_width * (m_curtain_height - m_ap_height);  
    m_ap_area = m_ap_height * m_ap_width;

    m_E_su_prev = m_q_rec_des * m_rec_qf_delay;	//[W-hr] Startup energy
    m_t_su_prev = m_rec_su_delay;				//[hr] Startup time requirement

    double c_htf_des = field_htfProps.Cp((m_T_htf_hot_des + m_T_htf_cold_des) / 2.0) * 1000.0;		//[J/kg-K] Specific heat at design conditions
    m_m_dot_htf_des = m_q_rec_des / (c_htf_des * (m_T_htf_hot_des - m_T_htf_cold_des));				//[kg/s]
    m_m_dot_htf_max = m_m_dot_htf_max_frac * m_m_dot_htf_des;	                                    //[kg/s]
    m_q_dot_inc_min = m_q_rec_des * m_f_rec_min / m_eta_therm_des_est;	//[W] Minimum receiver thermal power

    // If no startup requirements, then receiver is always ON
        // ... in the sense that the controller doesn't need to worry about startup
    if (m_E_su_prev == 0.0 && m_t_su_prev == 0.0) {
        m_mode_prev = C_csp_collector_receiver::OFF_NO_SU_REQ;					//[-] 0 = requires startup, 1 = starting up, 2 = running
    }
    else {
        m_mode_prev = C_csp_collector_receiver::OFF;					//[-] 0 = requires startup, 1 = starting up, 2 = running
    }



    // Calculate view factors
    if (m_model_type == 3 && m_rad_model_type > 0)
    {
        // Set up radiation groups. If discretized elements can't be distributed evenly per group then the groups at the periphery of the receiver will have more elements
        int ny_extra = (m_n_y - 1) % m_n_y_rad;
        int ny_base = (m_n_y - 1 - ny_extra) / m_n_y_rad;

        int nx_extra = m_n_x % m_n_x_rad;
        int nx_base = (m_n_x - nx_extra) / m_n_x_rad;

        m_ny_per_group.resize_fill(m_n_y_rad, ny_base);
        m_nx_per_group.resize_fill(m_n_x_rad, nx_base);

        int n = int(ny_extra / 2) + ny_extra % 2;
        for (int j = 0; j < n; j++)
        {
            m_ny_per_group.at(j) += 1;
            if (j < n - 1 || ny_extra % 2 == 0)
                m_ny_per_group.at(m_n_y_rad - 1 - j) += 1;
        }

        n = int(nx_extra / 2) + nx_extra % 2;
        for (int j = 0; j < n; j++)
        {
            m_nx_per_group.at(j) += 1;
            if (j < n - 1 || nx_extra % 2 == 0)
                m_nx_per_group.at(m_n_x_rad - 1 - j) += 1;
        }

        // Calculate view factors
        calculate_view_factors();

    }

    m_stored_soln_idx = 0;
    m_stored_soln_idx_csky = 0;
    m_soln_cache.resize(m_n_max_stored_solns);
    m_soln_cache_csky.resize(m_n_max_stored_solns);

    m_ncall = -1;

    // Calculate design point performance
    double vwind = 0.0;
    double wdir = 0.0;
    design_point_steady_state(vwind, wdir, m_eta_thermal_des_calc, m_W_dot_lift_des_calc, m_Q_dot_transport_des_calc, m_q_dot_loss_per_m2_des_calc, m_tauc_avg_des_calc);
    m_W_dot_rec_pump_des_calc = m_W_dot_lift_des_calc*1e-6;  // Using salt receiver parent class variable in MWe
    m_Q_dot_piping_loss = m_Q_dot_transport_des_calc;        // Using salt receiver parent class variable in W
    m_W_dot_pumping_tower_share = std::numeric_limits<double>::quiet_NaN();
    m_W_dot_pumping_rec_share = std::numeric_limits<double>::quiet_NaN();
    m_rec_pump_coef = std::numeric_limits<double>::quiet_NaN();
    m_vel_htf_des = std::numeric_limits<double>::quiet_NaN();


    return;
}


void C_falling_particle_receiver::update_sizing(double ap_height, double ap_width, double rec_orientation, double h_tower)
{
    m_ap_height = ap_height;
    m_ap_width = ap_width;
    m_h_tower = h_tower;
    m_rec_orientation = rec_orientation;
    if (m_rec_orientation < 0)
        m_rec_orientation += 360;  // Relative wind directions require receiver orientation within [0,360]
    init();  // Re-initialize receiver with new sizing
    return;
}


void C_falling_particle_receiver::call(const C_csp_weatherreader::S_outputs& weather,
    const C_csp_solver_htf_1state& htf_state_in,
    const C_pt_receiver::S_inputs& inputs,
    const C_csp_solver_sim_info& sim_info)
{
    // Increase call-per-timestep counter
    // Converge() sets it to -1, so on first call this line will adjust it = 0
    m_ncall++;

    double step = sim_info.ms_ts.m_step;	//[s]
    double time = sim_info.ms_ts.m_time;	//[s]

    double plant_defocus = inputs.m_plant_defocus;          //[-]
    const util::matrix_t<double>* flux_map_input = inputs.m_flux_map_input;  // Flux distributions (ny,nx) [kW/m2] 
    C_csp_collector_receiver::E_csp_cr_modes input_operation_mode = inputs.m_input_operation_mode;
    double clearsky_dni = inputs.m_clearsky_dni;

    double T_particle_cold_in = htf_state_in.m_temp + 273.15;	//[K] Cold particle inlet temp, convert from C
    double P_amb = weather.m_pres * 100.0;	//[Pa] Ambient pressure, convert from mbar
    double T_dp = weather.m_tdew + 273.15;	//[K] Dew point temperature, convert from C
    double T_amb = weather.m_tdry + 273.15;	//[K] Dry bulb temperature, convert from C
    double I_bn = weather.m_beam;           //[W/m2]
    double v_wind_10 = weather.m_wspd;      //[m/s]
    double wind_direc = weather.m_wdir;      // Wind direction [deg]
    double hour = time / 3600.0;			//[hr] Hour of the year
    double T_sky = CSP::skytemp(T_amb, T_dp, hour);     //[K]

    double clearsky_adj = std::fmax(clearsky_dni, I_bn);  // Adjust clear-sky DNI to be the actual DNI, if the actual DNI is higher
    double clearsky_to_input_dni = clearsky_adj / I_bn;
    if (I_bn < 1.E-6) {
        clearsky_to_input_dni = 1.0;
    }

    bool rec_is_off = false;
    double od_control = std::numeric_limits<double>::quiet_NaN();
    s_steady_state_soln soln;

    double m_dot_tot, T_particle_hot, T_particle_hot_rec, eta, T_htf_prop, cp_htf, W_lift, T_cav_wall_max, T_cav_wall_avg;
    m_dot_tot = T_particle_hot = T_particle_hot_rec = eta = T_htf_prop = cp_htf = W_lift = T_cav_wall_max = T_cav_wall_avg = std::numeric_limits<double>::quiet_NaN();
    double Q_inc, Q_refl, Q_adv, Q_rad, Q_cond, Q_transport, Q_thermal, Q_inc_pre_defocus;
    Q_inc = Q_refl = Q_adv = Q_rad = Q_cond = Q_transport = Q_thermal = Q_inc_pre_defocus = std::numeric_limits<double>::quiet_NaN();


    // Set current timestep stored values to NaN so we know that code solved for them
    m_mode = C_csp_collector_receiver::OFF;
    m_E_su = std::numeric_limits<double>::quiet_NaN();
    m_t_su = std::numeric_limits<double>::quiet_NaN();

    if (input_operation_mode < C_csp_collector_receiver::OFF || input_operation_mode > C_csp_collector_receiver::STEADY_STATE) {
        error_msg = util::format("Input operation mode must be either [0,1,2], but value is %d", input_operation_mode);
        throw(C_csp_exception(error_msg, "Falling particle receiver timestep performance call"));
    }

    // Check that wind direction is defined
    if (wind_direc != wind_direc)
    {
        throw(C_csp_exception("Wind direction is not defined", "Falling particle receiver timestep performance call"));
    }

    // Calculate total flux input prior to interpolation
    int n_flux_y = (int)flux_map_input->nrows();
    int n_flux_x = (int)flux_map_input->ncols();
    double flux_sum = 0.0;
    for (int i = 0; i < n_flux_y; i++) {
        for (int j = 0; j < n_flux_x; j++) {
            flux_sum += flux_map_input->at(i, j);
        }
    }


    bool rec_is_defocusing = false;

    // Do an initial check to make sure the solar position called is valid
    // If it's not, return the output equal to zeros. Also check to make sure
    // the solar flux is at a certain level, otherwise the correlations aren't valid
    if (input_operation_mode == C_csp_collector_receiver::OFF)
    {
        rec_is_off = true;
    }

    if (plant_defocus == 0.0 || flux_sum <= 1.E-6) // I_bn <= 1.E-6)
    {
        m_mode = C_csp_collector_receiver::OFF;
        rec_is_off = true;
    }


    //if (field_eff < m_eta_field_iter_prev && m_od_control < 1.0)
    //{	// In a prior call-iteration this timestep, the component control was set < 1.0
    //    //     indicating that receiver requested defocus due to mass flow rate constraint
    //    // This call, the plant requests the field to defocus. Under the new plant defocus
    //    //     the corresponding required *component* defocus decreases, because less flux on receiver
    //    // So this line helps "correctly" allocate defocus from the component to the controller
    //    // But, this likely makes the defocus iteration trickier because the iterator
    //    //    won't see a response in output mass flow or heat until m_od_control is back to 1.0
    //    // Component defocus also depends on inlet temperature, which can make current method tricky
    //    //    because the mass_flow_and_defocus code will only adjust m_od_control down, not up
    //    //    and then following calls-iterations use the previous m_od_control as a baseline
    //    //    unless adjusted here.
    //	m_od_control = fmin(m_od_control + (1.0 - field_eff / m_eta_field_iter_prev), 1.0);
    //}
    // So maybe just try resetting m_od_control each call?
    //      This will also force correct allocation of defocus. Might be a bit slower because it's calling
    //           the mspt component defocus method more frequently, but seems more straight-forward
    //           and might make the upstream problem easier to solve or at least easier to understand
    od_control = 1.0;

    // Initialize steady state solutions with current weather, DNI, field efficiency, and inlet conditions
    s_steady_state_soln soln_actual, soln_clearsky;
    soln.T_amb = T_amb;             //[K]
    soln.v_wind_10 = v_wind_10;     //[m/s]
    soln.wind_dir = wind_direc;
    soln.p_amb = P_amb;             //[Pa]
    soln.T_sky = T_sky;             //[K]

    soln.flux_sum = flux_sum;       //[W/m2]
    soln.dni_applied_to_measured = 1.0;     //[-]
    soln.plant_defocus = plant_defocus;     //[-]
    soln.clearsky_to_input_dni = clearsky_to_input_dni;     // clearsky_adj / I_bn;   //[-]
    soln.T_particle_cold_in = T_particle_cold_in;   //[K]	
    soln.od_control = od_control;           //[-] Initial component defocus control (may be adjusted during the solution)
    soln.rec_is_off = rec_is_off;

    if (m_is_mode_fixed_to_input_mode)  // Require solution to keep the operating mode specified in input_operation_mode, even if the receiver can't achieve the target exit temperature
    {
        soln.require_operating_mode = true;
        soln.required_mode = input_operation_mode;
    }

    if ((std::isnan(clearsky_to_input_dni) || clearsky_to_input_dni < 0.9999) && m_csky_frac > 0.0001)
        throw(C_csp_exception("Clear-sky DNI to measured is NaN or less than 1 but is required >= 1 in the clear-sky receiver model"));

    // Get total incident flux before defocus is applied
    util::matrix_t<double> mt_q_dot_inc_pre_defocus = calculate_flux_profiles(flux_sum, 1.0, 1.0, 1.0, flux_map_input);  // W/m2
    Q_inc_pre_defocus = sum_over_rows_and_cols(mt_q_dot_inc_pre_defocus, true) * m_curtain_elem_area;



    if (rec_is_off)  // Receiver either called in "OFF" mode, or set to off above because no solar flux 
    {
        soln.q_dot_inc.resize_fill(m_n_y, m_n_x, 0.0);
    }
    else
    {
        int prev_soln_id = -1;

        //--- Solve for mass flow at actual and/or clear-sky DNI extremes
        if (m_csky_frac <= 0.9999 || clearsky_to_input_dni < 1.0001) // Solve for mass flow at actual DNI?
        {
            soln_actual = soln;  // Sets initial solution properties (inlet T, initial defocus control, etc.)
            soln_actual.dni_applied_to_measured = 1.0;

            prev_soln_id = use_previous_solution(soln_actual, m_soln_cache);  // Check if these conditions are identical to any of the stored solutions
            if (prev_soln_id >= 0)
                soln_actual = m_soln_cache.at(prev_soln_id);
            else
            {
                solve_for_mass_flow_and_defocus(soln_actual, m_m_dot_htf_max, flux_map_input);
                m_soln_cache.at(m_stored_soln_idx) = soln_actual;
                m_stored_soln_idx = m_stored_soln_idx < (m_n_max_stored_solns - 1) ? m_stored_soln_idx + 1 : 0;
            }
            
        }

        if (m_csky_frac >= 0.0001) // Solve for mass flow at clear-sky DNI?
        {
            if (clearsky_to_input_dni < 1.0001)
                soln_clearsky = soln_actual;
            else
            {
                soln_clearsky = soln;
                soln_clearsky.dni_applied_to_measured = soln.clearsky_to_input_dni; 

                prev_soln_id = use_previous_solution(soln_clearsky, m_soln_cache_csky);  // Check if these conditions are identical to any of the stored solutions
                if (prev_soln_id >= 0)
                    soln_clearsky = m_soln_cache_csky.at(prev_soln_id);
                else
                {
                    solve_for_mass_flow_and_defocus(soln_clearsky, m_m_dot_htf_max, flux_map_input);
                    m_soln_cache_csky.at(m_stored_soln_idx_csky) = soln_clearsky;
                    m_stored_soln_idx_csky = m_stored_soln_idx_csky < (m_n_max_stored_solns - 1) ? m_stored_soln_idx_csky + 1 : 0;
                }

            }
        }

        //--- Set mass flow and calculate final solution
        if (clearsky_to_input_dni < 1.0001 || m_csky_frac < 0.0001)  // Flow control is based only on actual DNI -> use steady state solution as is
            soln = soln_actual;

        else if (soln_clearsky.rec_is_off)    // Receiver can't operate at this time point 
        {
            soln.rec_is_off = true;
            soln.q_dot_inc = soln_clearsky.q_dot_inc;
        }

        else if (m_csky_frac > 0.9999)   // Receiver can operate and flow control based only on clear-sky DNI -> use clear-sky flow rate and solve steady state solution with actual DNI
        {
            soln.m_dot_tot = soln_clearsky.m_dot_tot;
            soln.rec_is_off = soln_clearsky.rec_is_off;
            soln.od_control = soln_clearsky.od_control;
            soln.q_dot_inc = calculate_flux_profiles(flux_sum, 1.0, plant_defocus, soln_clearsky.od_control, flux_map_input);  // Absorbed flux profiles at actual DNI and clear-sky defocus
            calculate_steady_state_soln(soln, 0.00025, false);  // Solve energy balances at clear-sky mass flow rate and actual DNI conditions
        }

        else  // Receiver can operate, and mass flow control is based on a weighted average of clear-sky and actual DNI
        {

            if (soln_actual.rec_is_off)  // Receiver is off in actual DNI solution -> Set mass flow to the minimum value
            {
                soln_actual.m_dot_tot = m_f_rec_min * m_m_dot_htf_max;
                soln_actual.od_control = 1.0;
            }

            soln.rec_is_off = false;
            soln.m_dot_tot = (1.0 - m_csky_frac) * soln_actual.m_dot_tot + m_csky_frac * soln_clearsky.m_dot_tot;  // weighted average of clear-sky and actual DNI mass flow rates

            if (soln_clearsky.od_control >= 0.9999)  // No defocus in either clear-sky or actual DNI solutions
            {
                soln.od_control = soln_clearsky.od_control;
                soln.q_dot_inc = soln_actual.q_dot_inc;
                calculate_steady_state_soln(soln, 0.00025, false); // Solve energy balances at this mass flow rate and actual DNI conditions
            }
            else
            {
                soln.od_control = (1.0 - m_csky_frac) * soln_actual.od_control + m_csky_frac * soln_clearsky.od_control;
                solve_for_defocus_given_flow(soln, flux_map_input);     // Solve for defocus to achieve as close as possible to target outlet T with this mass flow and actual DNI conditions
            }

        }
    }

    // Set variables for use in the rest of the solution

    rec_is_off = soln.rec_is_off;
    m_mode = soln.rec_is_off ? C_csp_collector_receiver::OFF : input_operation_mode;
    od_control = soln.od_control;

    m_dot_tot = soln.m_dot_tot;
    T_particle_hot = soln.T_particle_hot;
    T_particle_hot_rec = soln.T_particle_hot_rec;
    eta = soln.eta;     // Efficiency not including transport loss

    T_cav_wall_max = soln.T_back_wall_max;
    T_cav_wall_avg = soln.T_back_wall_avg;

    T_htf_prop = (T_particle_hot + T_particle_cold_in) / 2.0;
    cp_htf = field_htfProps.Cp(T_htf_prop) * 1000.0;

    Q_inc = soln.Q_inc;
    Q_refl = soln.Q_refl;
    Q_adv = soln.Q_adv;
    Q_rad = soln.Q_rad;
    Q_cond = soln.Q_cond;
    Q_transport = soln.Q_transport;
    Q_thermal = soln.Q_thermal;  // Thermal power including transport loss

    // Calculate total absorbed solar energy and minimum absorbed per panel if not calculated in the steady state solutions
    if (soln.Q_inc != soln.Q_inc)
    {
        Q_inc = sum_over_rows_and_cols(soln.q_dot_inc, true)* m_curtain_elem_area;
    }



    double DELTAP, Pres_D, ratio_dP_tower_to_rec, W_dot_pump, q_startup;
    DELTAP = Pres_D = ratio_dP_tower_to_rec = W_dot_pump = q_startup = std::numeric_limits<double>::quiet_NaN();

    q_startup = 0.0;

    double time_required_su = step / 3600.0;

    if (!rec_is_off)
    {
        switch (input_operation_mode)
        {
        case C_csp_collector_receiver::STARTUP:
        {
            double qdot = m_dot_tot * cp_htf * (T_particle_hot - T_particle_cold_in);

            double time_require_su_energy = m_E_su_prev / qdot;	//[hr]
            double time_require_su_ramping = m_t_su_prev;

            double time_required_max = fmax(time_require_su_energy, time_require_su_ramping);	//[hr]

            double time_step_hrs = step / 3600.0;		//[hr]

            if (time_required_max > time_step_hrs)		// Can't completely startup receiver in maximum allowable timestep
            {											// Need to advance timestep and try again
                time_required_su = time_step_hrs;
                m_mode = C_csp_collector_receiver::STARTUP;
                q_startup = qdot * step / 3600.0;
            }
            else
            {
                time_required_su = time_required_max;		//[hr]
                m_mode = C_csp_collector_receiver::ON;

                double q_startup_energy_req = m_E_su_prev;	//[W-hr]
                double q_startup_ramping_req = qdot * m_t_su_prev;	//[W-hr]
                q_startup = fmax(q_startup_energy_req, q_startup_ramping_req);
            }

            m_E_su = fmax(0.0, m_E_su_prev - qdot * step / 3600.0);
            m_t_su = fmax(0.0, m_t_su_prev - step / 3600.0);
        }

        rec_is_off = true;

        break;

        case C_csp_collector_receiver::ON:

            m_E_su = m_E_su_prev;
            m_t_su = m_t_su_prev;
            m_mode = C_csp_collector_receiver::ON;
            q_startup = 0.0;

            Q_thermal = m_dot_tot * cp_htf * (T_particle_hot - T_particle_cold_in);

            // If output here is less than specified allowed minimum, then need to shut off receiver, unless state is required to be fixed to ON
            if (Q_inc < m_q_dot_inc_min)
            {
                m_mode = C_csp_collector_receiver::OFF;
            }

            break;

        case C_csp_collector_receiver::STEADY_STATE:

            m_mode = C_csp_collector_receiver::STEADY_STATE;

            break;

        }	// End switch() on input_operation_mode


        Q_thermal = m_dot_tot * cp_htf * (T_particle_hot - T_particle_cold_in);

        // After convergence, determine whether the mass flow rate falls below the lower limit
        if (Q_inc < m_q_dot_inc_min)
        {
            // Steady State always reports Q_thermal (even when much less than min) because model is letting receiver begin startup with this energy
            // Should be a way to communicate to controller that Q_thermal is less than q_min without losing this functionality
            if (m_mode != C_csp_collector_receiver::STEADY_STATE || m_mode_prev == C_csp_collector_receiver::ON || m_mode_prev == C_csp_collector_receiver::OFF_NO_SU_REQ)
                rec_is_off = true;
        }
    }
    else
    {	// If receiver was off BEFORE startup deductions
        m_mode = C_csp_collector_receiver::OFF;
    }

    // Particle lift power
    W_lift = calculate_lift_power(m_dot_tot); 

    if (rec_is_off)
    {
        m_dot_tot = 0.0;
        eta = 0.0;
        Q_refl = 0.0;
        Q_adv = 0.0;
        Q_rad = 0.0;
        Q_cond = 0.0;
        Q_transport = 0.0;
        Q_thermal = 0.0;
        W_lift = 0.0;

        // Set the receiver outlet temperature equal to the inlet design temperature
        T_particle_hot = m_T_htf_cold_des;

        T_cav_wall_max = 273.15;
        T_cav_wall_avg = 273.15;

        Q_inc_pre_defocus = 0.0;
        Q_inc = 0.0;

        // Reset m_od_control (for reporting)
        od_control = 1.0;		//[-]
    }


    outputs.m_m_dot_salt_tot = m_dot_tot * 3600.0;		//[kg/hr] convert from kg/s
    outputs.m_eta_therm = eta;						    //[-] Receiver efficiency (includes curtain reflection, radiation and convective losses)
    outputs.m_W_dot_pump =  W_lift / 1.E6;				//[MW] convert from W
    outputs.m_q_conv_sum = (Q_adv+Q_cond) / 1.E6;		//[MW] convert from W  // TODO: Lumping reported advection loss and conduction loss for now, would be better to separate
    outputs.m_q_rad_sum = Q_rad / 1.E6;					//[MW] convert from W
    outputs.m_q_dot_refl_loss = Q_refl / 1.E6;
    outputs.m_Q_thermal = Q_thermal / 1.E6;				//[MW] convert from W
    outputs.m_T_salt_hot = T_particle_hot - 273.15;		//[C] convert from K
    outputs.m_component_defocus = od_control;			//[-]
    outputs.m_q_dot_rec_inc_pre_defocus = Q_inc_pre_defocus / 1.E6;    //[MWt]
    outputs.m_q_dot_rec_inc = Q_inc / 1.E6;			    //[MW] convert from W
    outputs.m_q_startup = q_startup / 1.E6;				//[MW-hr] convert from W-hr
    outputs.m_T_salt_cold = T_particle_cold_in - 273.15;	//[C] convert from K
    outputs.m_time_required_su = time_required_su * 3600.0;	//[s], convert from hr
    if (Q_thermal > 0.0)
        outputs.m_q_dot_piping_loss = Q_transport / 1.E6;	//[MWt]
    else
        outputs.m_q_dot_piping_loss = 0.0;		//[MWt]

    outputs.m_max_T_cav_wall = T_cav_wall_max - 273.15;
    outputs.m_avg_T_cav_wall = T_cav_wall_avg - 273.15;

    // Outputs in parent class, but not used or not relevant for falling particle receiver model
    outputs.m_dP_receiver = 0.0;	                    //[bar] receiver pressure drop, convert from Pa
    outputs.m_dP_total = 0.0;						    //[bar] total pressure drop, convert from MPa
    outputs.m_ratio_dP_tower_to_rec = 0.0;              //[-] ratio of total pressure drop that is caused by tower height
    outputs.m_vel_htf = 0.0;							//[m/s]
    outputs.m_q_heattrace = 0.0;

    outputs.m_Q_thermal_csky_ss = 0.0; //[MWt]
    outputs.m_Q_thermal_ss = 0.0; //[MWt]

    ms_outputs = outputs;
}


void C_falling_particle_receiver::off(const C_csp_weatherreader::S_outputs &weather,
	const C_csp_solver_htf_1state &htf_state_in,
	const C_csp_solver_sim_info &sim_info)
{
	// Don't currently need *any* of these inputs, but if we add recirculation or thermal capacitance it would be helpful to have in place
	m_mode = C_csp_collector_receiver::OFF;

	// Assuming no night recirculation, so... these should be zero
	outputs.m_m_dot_salt_tot = 0.0;		//[kg/hr] convert from kg/s
	outputs.m_eta_therm = 0.0;			//[-] RECEIVER thermal efficiency (includes radiation and convective losses. reflection losses are contained in receiver flux model)
	outputs.m_W_dot_pump = 0.0;			//[MW] convert from W
	outputs.m_q_conv_sum = 0.0;			//[MW] convert from W
	outputs.m_q_rad_sum = 0.0;			//[MW] convert from W
    outputs.m_q_dot_refl_loss = 0.0;
	outputs.m_Q_thermal = 0.0;			//[MW] convert from W
	outputs.m_T_salt_hot = 0.0;			//[C] convert from K
	outputs.m_component_defocus = 1.0;	//[-]
	outputs.m_q_dot_rec_inc = 0.0;		//[MW] convert from kW
	outputs.m_q_startup = 0.0;			//[MW-hr] convert from W-hr
	outputs.m_dP_receiver = 0.0;			//[bar] receiver pressure drop, convert from Pa
	outputs.m_dP_total = 0.0;			//[bar] total pressure drop, convert from MPa
	outputs.m_vel_htf = 0.0;				//[m/s]
	outputs.m_T_salt_cold = htf_state_in.m_temp;	//[C] convert from K
	outputs.m_time_required_su = sim_info.ms_ts.m_step;	//[s], convert from hr in code
	outputs.m_q_dot_piping_loss = 0.0;	//[MWt]
    outputs.m_q_heattrace = 0.0;
	
	outputs.m_Q_thermal_csky_ss = 0.0; //[MWt]
	outputs.m_Q_thermal_ss = 0.0; //[MWt]

    ms_outputs = outputs;
	
	return;
}

void C_falling_particle_receiver::overwrite_startup_requirements_to_on()
{
    m_mode_prev = C_csp_collector_receiver::ON;
    m_E_su_prev = 0.0;
    m_t_su_prev = 0.0;
}

double C_falling_particle_receiver::get_pumping_parasitic_coef()
{
    double W_lift = calculate_lift_power(m_m_dot_htf_des); // [W]
    return W_lift / m_q_rec_des; //[W/Wt]
}

double C_falling_particle_receiver::area_proj()
{
    return m_ap_area; //[m^2] aperture area of the receiver
}

double C_falling_particle_receiver::calculate_lift_power(double m_dot_tot)
{
    return m_dot_tot * (m_h_tower + m_curtain_height) * 9.8067 / m_eta_pump;  //[W]
}


void C_falling_particle_receiver::converged()
{

	if( m_mode == C_csp_collector_receiver::STEADY_STATE )
	{
		throw(C_csp_exception("Receiver should only be run at STEADY STATE mode for estimating output. It must be run at a different mode before exiting a timestep",
			"Falling particle receiver converged method"));
    }

    if (m_mode == C_csp_collector_receiver::OFF)
    {
        m_E_su = m_q_rec_des * m_rec_qf_delay;
        m_t_su = m_rec_su_delay;

        // If no startup requirements, then receiver is always ON
        // ... in the sense that the controller doesn't need to worry about startup
        if (m_E_su == 0.0 && m_t_su == 0.0) {
            m_mode = C_csp_collector_receiver::OFF_NO_SU_REQ;
        }
    }

    m_mode_prev = m_mode;
    m_E_su_prev = m_E_su;
    m_t_su_prev = m_t_su;

    m_ncall = -1;

    ms_outputs = outputs;
}


void C_falling_particle_receiver::set_state_requirement(bool is_mode_fixed_to_input_mode)
{
    m_is_mode_fixed_to_input_mode = is_mode_fixed_to_input_mode;
}

void C_falling_particle_receiver::set_fixed_mflow(double mflow)  // kg/s
{
    m_m_dot_htf_fixed = mflow;
    return;
}

void C_falling_particle_receiver::design_point_steady_state(double v_wind_10, double wind_direc, double& eta_thermal, double& W_lift, double &Q_transport_loss, double& q_dot_loss_per_m2_ap, double& tauc_avg)
{
    eta_thermal = std::numeric_limits<double>::quiet_NaN();
    W_lift = std::numeric_limits<double>::quiet_NaN();
    Q_transport_loss = std::numeric_limits<double>::quiet_NaN();
    q_dot_loss_per_m2_ap = std::numeric_limits<double>::quiet_NaN();
    tauc_avg = std::numeric_limits<double>::quiet_NaN();

    s_steady_state_soln soln_des;
    soln_des.T_amb = m_T_amb_des;
    soln_des.T_sky = m_T_sky_des;
    soln_des.p_amb = m_P_amb_des;
    soln_des.v_wind_10 = v_wind_10;
    soln_des.wind_dir = wind_direc;
    soln_des.T_particle_cold_in = m_T_htf_cold_des;
    soln_des.rec_is_off = false;

    // Solve for incident power needed to achieve design point thermal power to particles (assuming uniform solar flux on the curtain)
    double q_dot_inc_avg, Qtot, tol;
    q_dot_inc_avg = m_q_rec_des / 0.85 / m_curtain_area;  // Initial guess for average incident solar flux on curtain [W/m2]  
    tol = 1e-6;

    for (int j = 0; j < 40; j++)
    {
        soln_des.q_dot_inc.resize_fill(m_n_y, m_n_x, q_dot_inc_avg);
        solve_for_mass_flow(soln_des);
        
        if (soln_des.eta < 1e-6) // Thermal model failed to solve
            break;

        Qtot = soln_des.Q_thermal;      // Thermal power including transport loss
        if (fabs(Qtot - m_q_rec_des)/m_q_rec_des < tol)
        {
            eta_thermal = soln_des.eta;  // Efficiency without transport loss
            q_dot_loss_per_m2_ap = (soln_des.Q_inc - soln_des.Q_thermal_without_transport) / m_ap_area;  // Loss only from receiver, not including particle transport
            W_lift = calculate_lift_power(soln_des.m_dot_tot);
            Q_transport_loss = soln_des.Q_transport;
            tauc_avg = soln_des.tauc_avg;
            break;
        }
        q_dot_inc_avg = ((m_q_rec_des + soln_des.Q_transport) / soln_des.eta) / m_curtain_area;
    }

    return;
}

double C_falling_particle_receiver::scale_wind_speed(double v_wind_10)
{
    return log((m_h_tower + m_curtain_height / 2) / 0.003) / log(10.0 / 0.003) * v_wind_10;
}



int C_falling_particle_receiver::use_previous_solution(const s_steady_state_soln& soln, const std::vector<s_steady_state_soln>& stored_solns)
{
    int soln_id = -1;

    // Are these conditions identical to those used in any of the stored solutions?
    for (int i = 0; i < m_n_max_stored_solns; i++)
    {
        if (!stored_solns.at(i).rec_is_off &&
            soln.dni_applied_to_measured == stored_solns.at(i).dni_applied_to_measured &&
            soln.T_particle_cold_in == stored_solns.at(i).T_particle_cold_in &&
            soln.plant_defocus == stored_solns.at(i).plant_defocus &&
            soln.od_control == stored_solns.at(i).od_control &&
            soln.T_amb == stored_solns.at(i).T_amb &&
            soln.v_wind_10 == stored_solns.at(i).v_wind_10 &&
            soln.wind_dir == stored_solns.at(i).wind_dir &&
            soln.p_amb == stored_solns.at(i).p_amb &&
            soln.flux_sum == stored_solns.at(i).flux_sum)
        {
            soln_id = i;
            break;
        }
    }

    return soln_id;

}

util::matrix_t<double> C_falling_particle_receiver::interpolate_flux_1d(util::matrix_t<double>& flux_input, int n, bool is_rows)
{
    int n1, n2, nflux, nc, i0, i1, nelem;
    double dx, dxflux, d1, d2, x;
    util::matrix_t<double> flux_input_loc, flux_interp, flux;

    flux_input_loc = flux_input;
    if (!is_rows)  // Transpose input array
    {
        n1 = flux_input.nrows();
        n2 = flux_input.ncols();
        flux_input_loc.resize_fill(n2, n1, 0.0);
        for (int i = 0; i < n2; i++)
        {
            for (int j = 0; j < n1; j++)
                flux_input_loc.at(i, j) = flux_input.at(j, i);
        }
    }
    
    nflux = flux_input_loc.nrows();
    nc = flux_input_loc.ncols();
    flux_interp.resize_fill(n, nc, 0.0);
    dx = 1.0 / n;
    dxflux = 1.0 / nflux;
    for (int i = 0; i < n; i++)
    {
        if (nflux == 1)
        {
            for (int j = 0; j < nc; j++)
                flux_interp.at(i, j) = flux_input_loc.at(0, j);
        }

        else if (n <= nflux)  // Curtain resolution < flux profile resolution
        {
            i0 = fmin(floor(i * dx / dxflux), nflux - 1);           // Flux profile element containing lower bound of element j
            i1 = fmin(floor((i + 1) * dx / dxflux), nflux - 1);     // Flux profile element containing upper bound of element j
            d1 = ((i0 + 1) * dxflux - (i * dx));                    // Distance from upper bound of flux element j0 to lower bound of element j
            d2 = ((i + 1) * dx - i1 * dxflux);                      // Distance from upper bound of element j to lower bound of element j1
            nelem = i1 - i0;
            for (int j = 0; j < nc; j++)
            {
                if (nelem < 1)
                    flux_interp.at(i, j) = flux_input_loc.at(i0, j);
                else
                {
                    flux_interp.at(i, j) = d1 * flux_input_loc.at(i0, j) + d2 * flux_input_loc.at(i1, j);
                    for (int k = 0; k < nelem - 1; k++)
                        flux_interp.at(i, j) += dxflux * flux_input_loc.at(i0+1+k, j);
                    flux_interp.at(i, j) /= dx;
                    flux_interp.at(i, j) = fmax(flux_interp.at(i, j), 0.0);
                }
            }
        }
        else  // Curtain resolution > flux profile resolution
        {
            x = (i + 0.5) * dx;
            i0 = fmax(fmin(floor(x / dxflux - 0.5), nflux - 2), 0);
            for (int j = 0; j < nc; j++)
            {
                if (x < 0.5 * dxflux)
                    flux_interp.at(i, j) = flux_input_loc.at(0, j);
                else if (x > 1 - 0.5 * dxflux)
                    flux_interp.at(i, j) = flux_input_loc.at(nflux - 1, j);
                else
                    flux_interp.at(i, j) = flux_input_loc.at(i0, j) + (flux_input_loc.at(i0+1, j) - flux_input_loc.at(i0, j)) / dxflux * (x - (i0 + 0.5) * dxflux);
                flux_interp.at(i, j) = fmax(flux_interp.at(i, j), 0.0);
            }
        }
    }

    // Transpose interpolated array
    flux = flux_interp;
    if (!is_rows)  // Tranpose interpolated array
    {
        n1 = flux_interp.nrows();
        n2 = flux_interp.ncols();
        flux.resize_fill(n2, n1, 0.0);
        for (int i = 0; i < n2; i++)
        {
            for (int j = 0; j < n1; j++)
                flux.at(i, j) = flux_interp.at(j, i);
        }
    }
    return flux;
}



// Calculate flux profiles (interpolated to curtain elements)
util::matrix_t<double> C_falling_particle_receiver::calculate_flux_profiles(double flux_sum /*W/m2*/, double flux_scale /*-*/, double plant_defocus /*-*/,
    double od_control, const util::matrix_t<double>* flux_map_input)
{
    util::matrix_t<double> q_dot_inc;
    q_dot_inc.resize_fill(m_n_y, m_n_x, 0.0);    
    double total_defocus = plant_defocus * od_control;  // Combine controller-specified defocus (plant_defocus) and receiver defocus (od_control)

    if (flux_sum > 1.0)
    {
        //--- Scale input flux
        int n_flux_y = (int)flux_map_input->nrows();
        int n_flux_x = (int)flux_map_input->ncols();
        util::matrix_t<double>flux_scaled(n_flux_y, n_flux_x);
        for (int j = 0; j < n_flux_y; j++)
        {
            for (int i = 0; i < n_flux_x; i++)
            {
                flux_scaled.at(j,i) = (*flux_map_input)(j, i) * total_defocus * flux_scale * 1000;  // Incident flux adjusted for DNI and defocus [W/m2]
            }
        }

        //--- Interpolate
        util::matrix_t<double> flux_x = (m_n_y-1 == n_flux_y) ? flux_scaled : interpolate_flux_1d(flux_scaled, m_n_y - 1, true);    // Interpolate to curtain y-resolution
        util::matrix_t<double> flux = (m_n_x == n_flux_x) ? flux_x : interpolate_flux_1d(flux_x, m_n_x, false);                    // Interpolate to curtain x-resolution
        for (int j = 0; j < m_n_y; j++)
        {
            for (int i = 0; i < m_n_x; i++)
            {
                if (j < m_n_y - 1)
                    q_dot_inc.at(j, i) = flux.at(j, i);
                else
                    q_dot_inc.at(j, i) = flux.at(j - 1, i);  // Note the flux at the last height node doesn't contribute to the energy balance, this value will only be used to calculation the wall temperature solution at the last node (which also doesn't contribute to the energy balance)
            }
        }
    }

    return q_dot_inc;
}

// Calculate steady state temperature and heat loss profiles for a given mass flow and incident flux
void C_falling_particle_receiver::calculate_steady_state_soln(s_steady_state_soln& soln, double tol, bool init_from_existing, int max_iter)
{

    double tolTp = 0.1;  // Particle temperature tolerance [K] for convergence of solution iterations

    double P_amb = soln.p_amb;
    double T_amb = soln.T_amb;
    double v_wind_10 = soln.v_wind_10;
    double wdir = soln.wind_dir;
    double T_sky = soln.T_sky;
    double T_cold_in = soln.T_particle_cold_in;

    double v_wind = scale_wind_speed(v_wind_10);  // Wind speed at receiver height
    double dy = m_curtain_height / (m_n_y - 1);
    double dx = m_curtain_width / m_n_x;

    double Q_inc, Q_refl, Q_rad, Q_adv, Q_cond, Q_thermal, Q_imbalance;
    double Tp_out, T_particle_prop, T_cold_in_rec, cp, cp_cold, cp_hot, particle_density, err, hadv_with_wind, Twavg, Twmax, Twf;
    double qnetc_avg, qnetc_sol_avg, qnetw_avg, qnetw_sol_avg;
    double tauc_avg, rhoc_avg;

    Q_refl = Q_rad = Q_adv = Q_cond = Q_thermal = Q_imbalance = 0.0;
    Twavg = Twmax = Twf = Tp_out = hadv_with_wind = 0.0;
    qnetc_avg = qnetc_sol_avg = qnetw_avg = qnetw_sol_avg = 0.0;
    tauc_avg = rhoc_avg = 0.0;

    Q_inc = sum_over_rows_and_cols(soln.q_dot_inc, true) * m_curtain_elem_area;  // Total solar power incident on curtain [W]
    T_particle_prop = (m_T_htf_hot_des + T_cold_in) / 2.0; 
    particle_density = field_htfProps.dens(T_particle_prop, 1.0);
    cp = field_htfProps.Cp(T_particle_prop) * 1000.0;			        // Particle specific heat  [J/kg-K] evaluated at average of current inlet temperature and design outlet temperature
    cp_cold = field_htfProps.Cp(T_cold_in) * 1000.0;
    cp_hot = field_htfProps.Cp(m_T_htf_hot_des) * 1000.0;
    T_cold_in_rec = T_cold_in - m_deltaT_transport_cold;  // Inlet temperature to receiver accounting from loss from cold particle transport

    double rhow_solar = 1.0 - m_cav_abs;
    double rhow_IR = 1.0 - m_cav_emis;

    bool rec_is_off = false;
    bool converged = false;
    bool soln_exists = (soln.T_particle_hot == soln.T_particle_hot) && (soln.converged);  // Does a solution already exist from a previous iteration?

    if (m_model_type == 0) // Fixed user-defined receiver efficiency
    {
        Q_thermal = m_fixed_efficiency * Q_inc;  // Thermal power to particles in receiver [W]
        Tp_out = T_cold_in_rec + Q_thermal / (soln.m_dot_tot * cp);
        converged = true;

        Q_refl = 0.0;
        Q_rad = 0.0;
        Q_cond = 0.0;
        Q_adv = (1.0 - m_fixed_efficiency)*Q_inc; // Artificially assign all losses as advection losses. These are only reported through the compute module as "thermal" losses 

    }
    else if (m_model_type == 1 || m_model_type == 2) // Receiver efficiency correlations from Sandia (https://www.osti.gov/biblio/1890267, page 43)
    {
        double eta = sandia_efficiency_correlation(m_model_type == 2, Q_inc, v_wind, wdir);
        Q_thermal = eta*Q_inc;
        Tp_out = T_cold_in_rec + Q_thermal / (soln.m_dot_tot * cp);
        converged = true;

        Q_refl = 0.0;
        Q_rad = 0.0;
        Q_cond = 0.0;
        Q_adv = (1.0 - eta) * Q_inc; // Artificially assign all losses as advection losses. These are only reported through the compute module as combined "thermal" losses

    }
    else if (m_model_type == 3)        // Quasi-2D physics-based receiver model
    {
        double vel_out, Tfilm, hadv, fwind, Rwall, tauc1, rhoc1;
        double qnet_ap, qnet_wf, hwconv, qadv, qtot, dh, Tcond_prev, Tcond_next;
        double Twfnew, Tpdiff, Twdiff, Twfdiff;
        util::matrix_t<double> Tp, Tw, Tpnew, Twnew;

        Rwall = 1.0 / m_cav_hext + m_cav_twall / m_cav_kwall;  // Cavity wall thermal resistance
        
        //--- Solve mass and momentum equations for curtain velocity, thickness, void fraction
        util::matrix_t<double> mdot_per_elem(m_n_x);  // Mass flow (kg/s) per element
        for (int i = 0; i < m_n_x; i++)
        {
            mdot_per_elem.at(i) = soln.m_dot_tot / m_n_x;  // TODO: Update to allow for multiple mass flow control zones
        }
        solve_particle_flow(mdot_per_elem, soln.phip, soln.vel, soln.thc);
        vel_out = calculate_mass_wtd_avg_exit(mdot_per_elem, soln.vel);  // Mass-weighted average exit velocity

        //--- Calculate curtain optical properties from solution for curtain void fraction and thickness
        soln.tauc.resize_fill(m_n_y, m_n_x, 0.0);
        soln.rhoc.resize_fill(m_n_y, m_n_x, 0.0);
        for (int j = 0; j < m_n_y; j++)
        {
            for (int i = 0; i < m_n_x; i++)
            {
                calculate_local_curtain_optical_properties(soln.thc.at(j, i), soln.phip.at(j, i), rhoc1, tauc1);
                soln.rhoc.at(j, i) = rhoc1;
                soln.tauc.at(j, i) = tauc1;
            }
        }
        tauc_avg = sum_over_rows_and_cols(soln.tauc, false) / (m_n_x * m_n_y);  // Average curtain transmittance
        rhoc_avg = sum_over_rows_and_cols(soln.rhoc, false) / (m_n_x * m_n_y);  // Average curtain reflectance

        //--- Initialize solutions for particle and wall temperatures
        Tp.resize_fill(m_n_y, m_n_x, T_cold_in_rec);        // Particle temperature [K]
        Tpnew.resize_fill(m_n_y, m_n_x, T_cold_in_rec);
        Tw.resize_fill(m_n_y, m_n_x, T_cold_in_rec);       // Back wall temperature [K]
        Twnew.resize_fill(m_n_y, m_n_x, T_cold_in_rec);


        //--- Set initial guess for particle and wall temperatures
        if (soln_exists && init_from_existing)
        {
            Tp = soln.T_p;
            Tw = soln.T_back_wall;
            Twf = soln.T_front_wall;
        }
        else
        {
            double qabs_approx, qnet_approx, dh_approx, rhoc_avg, flux_avg, Ec_avg, vf_to_ap;

            if (m_hadv_model_type == 0)
            {
                hadv = m_hadv_user;
                fwind = 1.0;
            }
            else if (m_hadv_model_type == 1) // Use Sandia's correlations for advective loss
            {
                Tfilm = 0.5 * (0.5 * (m_T_htf_hot_des + T_cold_in_rec) + T_amb);
                calculate_advection_coeff_sandia(vel_out, Tfilm, v_wind, wdir, P_amb, hadv, fwind);
                hadv = fmax(hadv, 0.0);
                hadv *= m_hadv_mult;
            }
            else  // TODO: Need an error here... eventually expand to include a lookup table
            {
                hadv = 0.0;
                fwind = 1.0;
            }
            hadv_with_wind = hadv * fwind;

            rhoc_avg = flux_avg = Ec_avg = 0.0;
            vf_to_ap = m_rad_model_type == 1 ? m_vf_curtain_ap_avg : m_vf_rad_type_0;
            for (int i = 0; i < m_n_x; i++)
            {
                Tp.at(0, i) = T_cold_in_rec;
                for (int j=0; j<m_n_y; j++)
                {
                    if (j < m_n_y - 1)
                    {
                        qabs_approx = (1.0 - soln.rhoc.at(j, i) - soln.tauc.at(j, i) + rhow_solar * soln.tauc.at(j, i) + rhow_solar * (1.0 - vf_to_ap) * soln.rhoc.at(j, i)) * soln.q_dot_inc.at(j, i); // Approximate solar energy absorbed by the particle curtain (W/m2)
                        qnet_approx = qabs_approx - hadv_with_wind * (Tp.at(j, i) - soln.T_amb) - m_curtain_emis * CSP::sigma * (pow(Tp.at(j, i), 4) - pow(soln.T_amb,4));  //Approximate net heat transfer rate using curtain temperature at prior element
                        dh_approx = qnet_approx * (dy / (soln.phip.at(j + 1, i) * soln.thc.at(j + 1, i) * soln.vel.at(j + 1, i) * particle_density));
                        Tp.at(j + 1, i) = fmax(T_cold_in_rec, Tp.at(j, i) + dh_approx / cp);
                        if (Tp.at(j + 1, i) < soln.T_amb)
                            Tp.at(j + 1, i) = soln.T_amb;
                    }
                    qnet_approx = (1.0 - rhow_solar) * (1.0 + rhow_solar * soln.rhoc.at(j, i)) * (soln.tauc.at(j, i) * soln.q_dot_inc.at(j, i));  // Approximate solar energy absorbed at back wall (W/m2)
                    qnet_approx += (1.0 - rhow_IR)*(1.0 + rhow_IR*soln.rhoc.at(j,i)) * (m_curtain_emis * CSP::sigma * pow(Tp.at(j, i), 4));             // Add approximate IR radiative heat transfer incoming to the back wall (W/m2)
                    Tw.at(j, i) = fmax(T_cold_in_rec, pow(qnet_approx / (m_cav_emis * CSP::sigma), 0.25));

                    flux_avg += soln.q_dot_inc.at(j, i) / (m_n_x*m_n_y);
                    rhoc_avg += soln.rhoc.at(j, i) / (m_n_x * m_n_y);
                    Ec_avg += (m_curtain_emis * CSP::sigma * pow(Tp.at(j, i), 4)) / (m_n_x * m_n_y);
                }
            }
            qnet_approx = (1.0 - vf_to_ap) * m_cav_emis * (rhoc_avg * flux_avg + Ec_avg);
            Twf = fmax(T_cold_in_rec, pow(qnet_approx / (m_cav_emis * CSP::sigma), 0.25));  // Initial guess for front wall temperature
        }


        //--- Calculate coefficient matrix for radiative exchange
        // TODO: exclude this calculation if the mass flow rates haven't changed?
        util::matrix_t<double> K_solar, Kinv_solar, K_IR, Kinv_IR;

        if (m_rad_model_type == 1)
        {
            calculate_coeff_matrix(soln.rhoc, soln.tauc, rhow_solar, K_solar, Kinv_solar);
            if (fabs(rhow_solar - rhow_IR) < 0.001)
            {
                K_IR = K_solar;
                Kinv_IR = Kinv_solar;
            }
            else
            {
                calculate_coeff_matrix(soln.rhoc, soln.tauc, rhow_IR, K_IR, Kinv_IR);
            }

        }

        //--- Initialize quantities needed in radiation models
        double qnet_ap_sol, qnet_wf_sol, Eap, Ewf;
        util::matrix_t<double> qnetc_sol, qnetw_sol, qnetc, qnetw, Ecf, Ecb, Ebw;
        qnetc_sol.resize_fill(m_n_y, m_n_x, 0.0);
        qnetw_sol.resize_fill(m_n_y, m_n_x, 0.0);
        qnetc.resize_fill(m_n_y, m_n_x, 0.0);
        qnetw.resize_fill(m_n_y, m_n_x, 0.0);
        Ecf.resize_fill(m_n_y, m_n_x, 0.0);
        Ecb.resize_fill(m_n_y, m_n_x, 0.0);
        Ebw.resize_fill(m_n_y, m_n_x, 0.0);

        

        //-- Solve radiative exchange equation for solar energy (this is independent of temperature)
        if (m_rad_model_type == 0)
        {
            Q_refl = 0.0;
            double jcback_sol, jw_sol, jcfront_sol;
            for (int i = 0; i < m_n_x; i++)
            {
                for (int j = 0; j < m_n_y; j++)
                {
                    jcback_sol = soln.tauc.at(j, i) * soln.q_dot_inc.at(j, i) / (1 - soln.rhoc.at(j, i) * rhow_solar);
                    jw_sol = rhow_solar * jcback_sol;
                    jcfront_sol = m_vf_rad_type_0 * (soln.rhoc.at(j, i) * soln.q_dot_inc.at(j, i) + soln.tauc.at(j, i) * jw_sol);
                    qnetc_sol.at(j, i) = (soln.q_dot_inc.at(j, i) - jcfront_sol) + (jw_sol - jcback_sol);
                    qnetw_sol.at(j, i) = (1.0-rhow_solar) * soln.tauc.at(j, i) * soln.q_dot_inc.at(j, i) / (1.0 - soln.rhoc.at(j, i) * rhow_solar);
                    if (j < m_n_y - 1)
                        Q_refl += jcfront_sol * m_curtain_elem_area;
                }
            }
        }
        else if (m_rad_model_type == 1)
        {
            //-- Solve radiative exchange equation for solar energy (this is independent of temperature)
            //   Here the energy "source" is solar flux reflected by the curtain on the first pass, and solar flux transmitted through the curtain and reflected by the back wall on the first pass
            //   This assumes that solar flux transmitted through the curtain hits the back wall at the same discretized element
            Ecf.fill(0.0);
            Ecb.fill(0.0);
            Ebw.fill(0.0);
            for (int i = 0; i < m_n_x; i++)
            {
                for (int j = 0; j < m_n_y; j++)
                {
                    Ecf.at(j,i) = soln.rhoc.at(j, i) * soln.q_dot_inc.at(j, i);         // Energy "source" at front curtain surface = reflected solar energy
                    Ebw.at(j,i) = rhow_solar * soln.tauc.at(j, i) * soln.q_dot_inc.at(j, i);  // Energy "source" at back wall surface = transmitted and reflected solar energy
                }
            }
            calculate_radiative_exchange(Ecf, Ecb, Ebw, 0.0, 0.0, K_solar, Kinv_solar, soln.rhoc, soln.tauc, rhow_solar, qnetc_sol, qnetw_sol, qnet_wf_sol, qnet_ap_sol);  // Calculates net incoming radiative energy to each element (total incoming - total outgoing)

            // Radiative exchange model provides the net incoming energy to each surface (net outgoing is -qnet). Now calculate the total net incoming solar energy
            for (int i = 0; i < m_n_x; i++)
            {
                for (int j = 0; j < m_n_y; j++)
                {
                    qnetc_sol.at(j,i) += (1 - soln.tauc.at(j, i)) * soln.q_dot_inc.at(j, i);
                    qnetw_sol.at(j,i) += soln.tauc.at(j, i) * soln.q_dot_inc.at(j, i);
                }
            }
            Q_refl = qnet_ap_sol * m_ap_area;   // Solar reflection loss [W]
        }
        qnetc_sol_avg = sum_over_rows_and_cols(qnetc_sol, true) / (m_n_x * (m_n_y - 1));  // Average net incoming solar energy to the particle curtain
        qnetw_sol_avg = sum_over_rows_and_cols(qnetw_sol, true) / (m_n_x * (m_n_y - 1));  // Average net incoming solar energy to the back wall


        //--- Temperature solution iterations
        for (int q = 0; q < max_iter; q++)
        {
            Q_rad = Q_adv = Q_cond = Q_thermal = 0.0;
            Tpdiff = Twdiff = Twfdiff = 0.0;

            Twavg = sum_over_rows_and_cols(Tw, true) / (m_n_x * (m_n_y-1));  // Current average back wall temperature (neglecting last element)
            Twmax = max_over_rows_and_cols(Tw, true);  // Current maximum back wall temperature (neglecting last element)

            //-- Calculate advection loss coefficient
            if (m_hadv_model_type == 0)
            {
                hadv = m_hadv_user;
                fwind = 1.0;
            }
            else if (m_hadv_model_type == 1) // Use Sandia's correlations for advective loss
            {
                Tp_out = calculate_mass_wtd_avg_exit(mdot_per_elem, Tp);
                Tfilm = 0.5 * (0.5 * (Tp_out + T_cold_in_rec) + T_amb);
                calculate_advection_coeff_sandia(vel_out,  Tfilm, v_wind, wdir, P_amb, hadv, fwind);
                hadv = fmax(hadv, 0.0);
                hadv *= m_hadv_mult;
            }
            else  // TODO: Need an error here... eventually expand to include a lookup table
            {
                hadv = 0.0;
                fwind = 1.0;
            }
            hadv_with_wind = hadv * fwind;


            //-- Solve IR radiative exchange and calculate net incoming radiative energy to each curtain and wall element
            if (m_rad_model_type == 0)
            {
                double Ew, Ec, jcback_IR, jw_IR, jcfront_IR;
                for (int i = 0; i < m_n_x; i++)
                {
                    for (int j = 0; j < m_n_y; j++)
                    {
                        Ew = m_cav_emis * CSP::sigma * pow(Tw.at(j, i), 4);
                        Ec = m_curtain_emis * CSP::sigma * pow(Tp.at(j, i), 4);
                        jcback_IR = (Ec + soln.rhoc.at(j, i) * Ew) / (1 - soln.rhoc.at(j, i) * rhow_IR);
                        jw_IR = Ew + rhow_IR * jcback_IR;
                        jcfront_IR = m_vf_rad_type_0 * (Ec + soln.tauc.at(j, i) * jw_IR);
                        qnetc.at(j, i) = qnetc_sol.at(j, i) - jcfront_IR + (jw_IR - jcback_IR);
                        qnetw.at(j, i) = qnetw_sol.at(j, i) + ((1 - rhow_IR) * Ec - (1 - soln.rhoc.at(j, i)) * Ew) / (1.0 - soln.rhoc.at(j, i) * rhow_IR);
                        if (j < m_n_y - 1)
                            Q_rad += jcfront_IR * m_curtain_elem_area;
                    }
                }
            }
            else if (m_rad_model_type == 1)
            {
                Eap = CSP::sigma * pow(T_sky, 4);
                Ewf = m_cav_emis * CSP::sigma * pow(Twf, 4);  // Front cavity wall
                Ecf.fill(0.0);
                Ecb.fill(0.0);
                Ebw.fill(0.0);
                for (int i = 0; i < m_n_x; i++)
                {
                    for (int j = 0; j < m_n_y; j++)
                    {
                        Ecf.at(j,i) = m_curtain_emis * CSP::sigma * pow(Tp.at(j, i), 4); // Front curtain surface
                        Ecb.at(j, i) = Ecf.at(j, i);                                     // Back curtain surface
                        Ebw.at(j,i) = m_cav_emis * CSP::sigma * pow(Tw.at(j, i), 4);     // Back wall surface
                    }
                }
                calculate_radiative_exchange(Ecf, Ecb, Ebw, Eap, Ewf, K_IR, Kinv_IR, soln.rhoc, soln.tauc, rhow_IR, qnetc, qnetw, qnet_wf, qnet_ap);    // Calculates net incoming radiative energy to each element (total incoming - total outgoing)
                Q_rad = qnet_ap * m_ap_area;    // IR radiation loss [W]

                //--- Combine solar and IR exchange
                qnet_ap += qnet_ap_sol;
                qnet_wf += qnet_wf_sol;
                qnetc = matrix_addition(qnetc, qnetc_sol);
                qnetw = matrix_addition(qnetw, qnetw_sol);

            }
            qnetc_avg = sum_over_rows_and_cols(qnetc, true) / (m_n_x * (m_n_y - 1));
            qnetw_avg = sum_over_rows_and_cols(qnetw, true) / (m_n_x * (m_n_y - 1));



            //--- Calculate new back wall and particle temperature solutions, Q_adv, Q_thermal, and Q_cond
            for (int i = 0; i < m_n_x; i++)
            {
                for (int j = 0; j < m_n_y; j++)
                {
                    hwconv = m_include_back_wall_convection ? calculate_wall_convection_coeff(soln.vel.at(j, i), (j + 0.5) * dy, 0.5 * (Tw.at(j, i) + Tp.at(j, i)), P_amb) : 0.0;

                    // Solve for particle temperature at next vertical node
                    qadv = hadv_with_wind* (Tp.at(j, i) - T_amb);
                    if (j < m_n_y - 1)
                    {
                        qtot = qnetc.at(j, i) - qadv - hwconv * (Tp.at(j, i) - Tw.at(j, i));  // Net heat transfer rate into particles [W/m2]
                        dh = qtot * (dy / (soln.phip.at(j + 1, i) * soln.thc.at(j + 1, i) * soln.vel.at(j + 1, i) * particle_density));
                        Tpnew.at(j + 1, i) = Tpnew.at(j, i) + dh / cp;
                        Q_adv += qadv * m_curtain_elem_area;            // Advection loss [W] from this curtain element
                        Q_thermal += dh * mdot_per_elem.at(i);          // Energy into particles [W] in this curtain elemtn
                    }
                    Tpdiff = fmax(Tpdiff, fabs(Tpnew.at(j, i) - Tp.at(j, i)));

                    // Solve for back wall temperature at current node
                    Tcond_prev = Tcond_next = 0.0;
                    if (m_include_wall_axial_conduction)
                    {
                        if (j == m_n_y - 1)
                            Tcond_next = Twnew.at(j - 1, i);  // Tw(j+1) = Tw(j-1) for heat flux = 0 boundary condition at last node
                        else if (i == 0)  // First iteration doesn't have a good value for Tw(j+1) already defined
                            Tcond_next = (j == 0) ? Tw.at(j, i) : Tw.at(j, i) + (Twnew.at(j, i) - Twnew.at(j - 1, i));  //As a first guess, assume wall temperature rise from (j to j+1) is the same as that from (j-1 to j)
                        else
                            Tcond_next = Tw.at(j + 1, i);
                        Tcond_prev = (j > 0) ? Twnew.at(j - 1, i) : Tcond_next;
                    }
                    Twnew.at(j, i) = calculate_passive_surface_T(Tw.at(j, i), qnetw.at(j, i), hwconv, Tp.at(j, i), T_amb, m_include_wall_axial_conduction, Tcond_prev, Tcond_next);
                    if (j < m_n_y - 1)
                    {
                        Q_cond += ((Tw.at(j, i) - T_amb) / Rwall) * m_back_wall_elem_area;   // Conduction loss through back wall element [W]
                        Twdiff = fmax(Twdiff, fabs(Twnew.at(j, i) - Tw.at(j, i)));
                    }
                    
                }
            }

            //--- Calculate new front wall temperature solution and conduction loss
            Twfdiff = Twfnew = 0.0;
            if (m_rad_model_type > 0)
            {
                Twfnew = calculate_passive_surface_T(Twf, qnet_wf, 0.0, 0.0, T_amb, false, 0.0, 0.0);
                Q_cond += ((Twf - T_amb) / Rwall) * m_cav_front_area;
                Twfdiff = fabs(Twfnew - Twf);
            }

            //--- Check convergence and update temperature solution
            Q_imbalance = Q_inc - Q_refl - Q_rad - Q_adv - Q_cond - Q_thermal;
            err = Q_imbalance / Q_inc;  // 
            if (Tpdiff < tolTp && fabs(err) < tol)
            {
                converged = true;
                soln.T_p = Tpnew;
                soln.T_back_wall = Twnew;
                soln.T_front_wall = Twfnew;
                soln.T_back_wall_avg = Twavg;
                soln.T_back_wall_max = Twmax;
                Tp = Tpnew;
                Tw = Twnew;
                Twf = Twfnew;
                break;
            }

            Tp = Tpnew;
            Tw = Twnew;
            Twf = Twfnew;

            // Stop iterations for very low outlet temperature, or if the solution in the previous iteration failed 
            if (Q_thermal != Q_thermal)
                break;

            Tp_out = calculate_mass_wtd_avg_exit(mdot_per_elem, Tp);  // Mass-weighted average particle exit temperature from receiver [K]
            if (Tp_out < 0.0)
                break;
        }

        if (!converged)
            rec_is_off = true;  // Shut off receiver if temperature solution failed

        Tp_out = calculate_mass_wtd_avg_exit(mdot_per_elem, Tp);  // Mass-weighted average particle exit temperature from receiver [K]
    }

    double Tp_out_after_transport = Tp_out - m_deltaT_transport_hot;  // Outlet temperature accounting from loss from hot particle transport

    double Q_dot_transport_loss_hot = soln.m_dot_tot * cp * m_deltaT_transport_hot;
    double Q_dot_transport_loss_cold = soln.m_dot_tot * cp * m_deltaT_transport_cold;

    if (Tp_out <= T_cold_in || Q_thermal != Q_thermal)
    {
        rec_is_off = true;
    }


	// Save solution
    soln.Q_inc = Q_inc;
    soln.Q_thermal = Q_thermal - Q_dot_transport_loss_cold - Q_dot_transport_loss_hot;
    soln.Q_thermal_without_transport = Q_thermal;
	soln.Q_refl = Q_refl;
	soln.Q_rad = Q_rad;
	soln.Q_adv = Q_adv;
    soln.Q_cond = Q_cond;
    soln.Q_transport = Q_dot_transport_loss_cold + Q_dot_transport_loss_hot;
    soln.eta = soln.Q_inc>0 ? soln.Q_thermal_without_transport / soln.Q_inc : 0.0;
    soln.eta_with_transport = soln.Q_inc > 0 ? soln.Q_thermal / soln.Q_inc : 0.0;
    soln.hadv = hadv_with_wind;
    soln.converged = converged;
	soln.rec_is_off = rec_is_off;

    soln.T_particle_hot = Tp_out_after_transport;
    soln.T_particle_hot_rec = Tp_out;

    soln.tauc_avg = tauc_avg;
    soln.rhoc_avg = rhoc_avg;
    soln.qnetc_sol_avg = qnetc_sol_avg;
    soln.qnetw_sol_avg = qnetw_sol_avg;
    soln.qnetc_avg = qnetc_avg;
    soln.qnetw_avg = qnetw_avg;

	return;

}

// Calculate mass flow rate needed to achieve target outlet temperature (m_T_salt_hot_target) given incident flux profiles
void C_falling_particle_receiver::solve_for_mass_flow(s_steady_state_soln &soln)
{

	bool soln_exists = (soln.m_dot_tot == soln.m_dot_tot);
    double T_particle_prop = (m_T_particle_hot_target + soln.T_particle_cold_in) / 2.0; // Temperature for particle property evaluation [K].  
    double cp = field_htfProps.Cp(T_particle_prop) * 1000.0;   //[J/kg-K]

    double T_cold_in_rec = soln.T_particle_cold_in - m_deltaT_transport_cold;    // Temperature at the receiver inlet (after cold particle transport)
    double T_target_out_rec = m_T_particle_hot_target + m_deltaT_transport_hot;  // Temperature at the receiver exit (before hot particle transport)

    double err = -999.9;				//[-] Relative outlet temperature error
    double tol = 1.0e-4;
    int nmax = 50;

    bool allow_Tmax_stopping = true;   // Allow mass flow iteration loop to stop based on estimated upper bound of particle outlet temperature (assumes efficiency increases monotonically with mass flow)
    if (soln.require_operating_mode && soln.required_mode == C_csp_collector_receiver::E_csp_cr_modes::ON && m_fixed_mode_mflow_method == 1)
        allow_Tmax_stopping = false;      // If receiver can't reach target exit temperature the solution will return the mass flow required for the maximum exit temperature


    //--- Set initial guess for particle flow
    double m_dot_guess, m_dot_guess_new;
	if (soln_exists)  // Use existing solution as initial guess
	{
		m_dot_guess = soln.m_dot_tot;
	}
	else  // Set initial guess for mass flow solution
	{
        double Qinc = sum_over_rows_and_cols(soln.q_dot_inc, true) * m_curtain_elem_area;  // Total solar power incident on curtain [W]
        double eta_guess = m_model_type == 0 ? m_fixed_efficiency : 0.85;
        m_dot_guess = eta_guess * Qinc / (cp * (T_target_out_rec - T_cold_in_rec));	//[kg/s] Particle mass flow rate
	}



    //--- Solve for mass flow
    //    Note: Outlet temperature is non-monotonic with respect to mass flow because the curtain transparency changes with mass flow 
    //    The solution here relies on mass flow bounds that are derived assuming that there is only one maximum in the behavior of particle outlet temperature vs. mass flow rate
    //    The efficiency (Q_thermal / Q_inc) is assumed to increase monotonically to with respect to mass flow and the upper bound on efficiency is used to calculate an upper bound on outlet temperature
    //    Two mass flow solutions will exist that can reach the target outlet tempeature (for those cases where the target outlet temperature can be reached at all). The solution with higher mass flow is returned here. 

    double lower_bound = 0.0;                   // Lower bound for particle mass flow rate (kg/s)
    double upper_bound = 1e10;                  // Upper bound for particle mass flow rate  (kg/s)
    double upper_bound_eta = 1.0;               // Upper bound for receiver efficiency (assumed to occur at upper bound for mass flow)
    bool is_upper_bound = false;                // Has upper bound been defined?
    bool is_lower_bound_above_Ttarget = false;  // Has a solution already been found with outlet T > target T?

    double bound_tol = 0.01;                  // Stopping tolerance based on mass flow bounds (typically solution will terminate because of calculated maximum outlet T first)
    double Tout_tol = 1;
    double lower_bound_Tout = std::numeric_limits<double>::quiet_NaN();  // Outlet temperature at current lower bound for mass flow
    double upper_bound_Tout = std::numeric_limits<double>::quiet_NaN();  // Outlet temperature at current upper bound for mass flow

    double Tout_max = std::numeric_limits<double>::quiet_NaN();  // Current maximum possible outlet temperature [K] (based on upper bound for efficiency, lower bound for mass flow)
    double eta_max_sim = std::numeric_limits<double>::quiet_NaN();  // Maximum simulated receiver efficiency

    util::matrix_t<double> mflow_history, Tout_history, eta_history;
    util::matrix_t<bool> converged_history;
    mflow_history.resize_fill(nmax, 0.0);       // Mass flow iteration history
    Tout_history.resize_fill(nmax, 0.0);        // Outlet temperature iteration history
    eta_history.resize_fill(nmax, 0.0);         // Receiver efficiency iteration history
    converged_history.resize_fill(nmax, false); // Steady state solution convergence history
   

    int qq = -1;
    bool converged = false;
    bool init_from_existing = false;  // Use current solution for particle/wall temperatures as the initial guess for the next steady state solution
	while (!converged)
	{
		qq++;

        //-- Solve model at current mass flow guess
		soln.m_dot_tot = m_dot_guess;
		calculate_steady_state_soln(soln, tol, init_from_existing, 50);   // Solve steady state thermal model
		err = (soln.T_particle_hot - m_T_particle_hot_target) / m_T_particle_hot_target;
        mflow_history.at(qq) = soln.m_dot_tot;
        Tout_history.at(qq) = soln.T_particle_hot_rec;   // Outlet temperature from receiver (before hot particle transport)
        eta_history.at(qq) = soln.eta;                   // Efficiency not including transport losses
        converged_history.at(qq) = soln.converged;
        eta_max_sim = (qq == 0) ? soln.eta : fmax(eta_max_sim, soln.eta);

        init_from_existing = false;
        if (qq>0 && !soln.rec_is_off && std::abs(soln.T_particle_hot_rec - Tout_history.at(qq - 1)) < 20)  
            init_from_existing = true;
      

        //--- Check mass flow convergence
        if (std::abs(err) < tol)  // Solution outlet temperature is at the target
        {
            if (is_lower_bound_above_Ttarget)  // A different solution has already been found with a lower mass flow and an outlet T above the target
            {
                converged = true;
                break;
            }
            else  // Current mass flow produces the target outlet T, but can't be sure that it's the higher of the two possible mass flow solutions
            {
                s_steady_state_soln soln_candidate = soln;  // Store the current solution as a possible candidate solution
                soln.m_dot_tot = (1 - 0.015) * soln.m_dot_tot; // If this is the higher mass flow solution than a decrease in mass flow should increase outlet temperature
                calculate_steady_state_soln(soln, tol, true, 50);
                bool is_correct_soln = (soln.T_particle_hot_rec > soln_candidate.T_particle_hot_rec);
                soln = soln_candidate;
                if (is_correct_soln)
                {
                    converged = true;
                    break;
                }
                else  // Decreasing mass flow decreased the outlet temperature, this is the lower of the two mass flow solutions -> Set lower bound and continue iterating
                {
                    lower_bound = fmax(lower_bound, soln.m_dot_tot);
                    is_lower_bound_above_Ttarget = true;
                    m_dot_guess = is_upper_bound ? 0.5 * (lower_bound + upper_bound) : 1.05 * m_dot_guess;
                    continue;
                }
            }
        }


        //--- Update mass flow bounds

        // If efficiency is negative, this is always a lower bound for mass flow
        if (soln.eta < 0.0)
            lower_bound = soln.m_dot_tot;


        // If outlet temperature is above the target this is always a lower bound for mass flow 
        if (soln.T_particle_hot_rec > T_target_out_rec)
        {
            lower_bound = soln.m_dot_tot;
            is_lower_bound_above_Ttarget = true;  
        }

        // If outlet temperature is below the target, compare current solution with previously stored solutions to identify bounds
        else if (soln.converged)
        {
            for (int i = 0; i < qq; i++)  
            {
                if (converged_history.at(i))
                {
                    // Any point with an exit temperature below the target is a lower bound if another point has been sampled with higher mass flow and a higher outlet T
                    if (soln.m_dot_tot < mflow_history.at(i) && soln.T_particle_hot_rec < Tout_history.at(i))
                    {
                        lower_bound = soln.m_dot_tot;
                        lower_bound_Tout = soln.T_particle_hot_rec;
                    }
                    if (mflow_history.at(i) > lower_bound && Tout_history.at(i) < T_target_out_rec && mflow_history.at(i) < soln.m_dot_tot && Tout_history.at(i) < soln.T_particle_hot_rec)
                    {
                        lower_bound = mflow_history.at(i);
                        lower_bound_Tout = Tout_history.at(i);
                    }

                    // Any point with an exit temperature below the target is an upper bound if another point has been sampled with lower mass flow and higher outlet temperature 
                    if (soln.m_dot_tot > mflow_history.at(i) && soln.T_particle_hot_rec < Tout_history.at(i))
                    {
                        upper_bound = soln.m_dot_tot;
                        upper_bound_eta = soln.eta;
                        upper_bound_Tout = soln.T_particle_hot_rec;
                        is_upper_bound = true;
                    }
                    if (mflow_history.at(i) < upper_bound && Tout_history.at(i) < T_target_out_rec && mflow_history.at(i) > soln.m_dot_tot && Tout_history.at(i) < soln.T_particle_hot_rec)
                    {
                        upper_bound = mflow_history.at(i);
                        upper_bound_eta = eta_history.at(i);
                        upper_bound_Tout = Tout_history.at(i);
                        is_upper_bound = true;
                    }    
                }

            }
        }


        //--- Next solution guess
        Tout_max = 5000;
        if (lower_bound > 0.001)
            Tout_max = T_cold_in_rec + (upper_bound_eta * soln.Q_inc) / (cp * lower_bound);  //  Maximum possible outlet temperature at current lower bound for flow and upper bound for efficiency

        if (soln.eta < 0.0)  // If efficiency at current mass flow guess was negative, try again with a substantially higher flow
        {
            m_dot_guess_new = 2 * soln.m_dot_tot;
        }
        else if (Tout_max < T_target_out_rec - 0.01)  // If maximum possible outlet T is below the target, then the iterations need to find the maximum outlet T instead of the target outlet T
        {
            m_dot_guess_new = is_upper_bound ? 0.5 * (lower_bound + upper_bound) : 1.5 * soln.m_dot_tot;
        }
        else  // If it's still possible that the receiver can achieve the target exit temperature, guess new mass flow based on the value needed to hit the target
        {
            m_dot_guess_new = soln.Q_thermal_without_transport / (cp * (T_target_out_rec - T_cold_in_rec));   //[kg/s]
            // Solution can converge slowly, after a few iterations switch to approximation for mass flow using linear approximation for efficiency from last two guesses
            if (qq >= 2 && eta_history.at(qq) > 0 && eta_history.at(qq - 1) > 0 && fabs(eta_history.at(qq) - eta_history.at(qq - 1)) < 0.3)
            {
                double c1 = (eta_history.at(qq) - eta_history.at(qq - 1)) / (mflow_history.at(qq) - mflow_history.at(qq - 1));  // Slope of linear equation for eta = f(m)
                double c2 = soln.eta - c1 * soln.m_dot_tot;                                                                     // Intercept of linear equation for eta = f(m)
                m_dot_guess_new = (soln.Q_inc * c2) / (cp * (T_target_out_rec - T_cold_in_rec) - c1 * soln.Q_inc);              // Solution for m from: (c1*m+c2)*Qinc = m*Cp*dT
            }

            // Limit downward changed in mass flow (avoids excessive under-shoot that can happen with very low power)
            if (m_dot_guess_new < soln.m_dot_tot)
                m_dot_guess_new = fmax(0.5 * soln.m_dot_tot, m_dot_guess_new);
        }

        //-- Check next solution guess relative to bounds
        if (is_upper_bound && (m_dot_guess_new <= lower_bound || m_dot_guess_new >= upper_bound))  // New guess is out of bounds and lower/upper bounds are both defined
            m_dot_guess_new = 0.5 * (lower_bound + upper_bound);
        else if (m_dot_guess_new <= lower_bound)  // New guess is below lower bound with no defined upper bound
            m_dot_guess_new = (m_dot_guess_new < m_dot_guess) ? fmax(1.5 * lower_bound, 0.5 * m_dot_guess) : 1.5 * m_dot_guess;

        if (fabs((m_dot_guess_new - soln.m_dot_tot) / soln.m_dot_tot) < 0.001)  // Next guess is identical to the last one (this can occur with bisection when the new solution at the midpoint doesn't change the known bounds)
            m_dot_guess_new *= 1.1;


        m_dot_guess = m_dot_guess_new;


        //--- Stopping criteria
        if (m_dot_guess < 1.E-5 || qq >= nmax-1)
        {
            soln.rec_is_off = true;
            break;
        }

        if (soln.Q_thermal != soln.Q_thermal) 
        {
            soln.rec_is_off = true;
            break;
        }

        // Stop solution if outlet temperature was not achieved and mass flow bounds are within tolerance
        if (is_upper_bound && (upper_bound - lower_bound) <= bound_tol * lower_bound && fabs(lower_bound_Tout - upper_bound_Tout)<=Tout_tol)
        {
            // Shut off receiver unless the operating mode is required to remain ON. 
            if (!soln.require_operating_mode || soln.required_mode != C_csp_collector_receiver::E_csp_cr_modes::ON || m_fixed_mode_mflow_method != 1)
                soln.rec_is_off = true; 
            break;
        }

        // Stop solution if the maximum possible particle outlet temperature is below the target
        // This assumes that the efficiency (Q_thermal / Q_inc) increases monotonically with mass flow for fixed solar incidence and ambient conditions
        if (allow_Tmax_stopping && lower_bound > 0.001)
        {
            Tout_max = T_cold_in_rec + (upper_bound_eta * soln.Q_inc) / (cp * lower_bound);
            if (Tout_max < T_target_out_rec -0.01)
            {
                soln.rec_is_off = true;
                break;
            }

        }

	}

    // Solve at fixed mass flow if receiver cannot achieve exit temperature, but is required to remain ON 
    if (soln.rec_is_off && soln.require_operating_mode && soln.required_mode == C_csp_collector_receiver::E_csp_cr_modes::ON && m_fixed_mode_mflow_method == 0)
    {
        soln.m_dot_tot = m_m_dot_htf_fixed;
        calculate_steady_state_soln(soln, tol, false, 50);
        soln.rec_is_off = false;
    }

    // Require final solution to have positive energy to particles, regardless of operating state requirements
    if (soln.Q_thermal < 0.0)
        soln.rec_is_off = true; 

	return;
}

// Calculate mass flow rate and defocus needed to achieve target outlet temperature given DNI
void C_falling_particle_receiver::solve_for_mass_flow_and_defocus(s_steady_state_soln &soln, double m_dot_htf_max, const util::matrix_t<double> *flux_map_input)
{

	bool rec_is_defocusing = true;
	double err_od = 999.0;

	while (rec_is_defocusing)
	{
		if (soln.rec_is_off)
			break;

		soln.q_dot_inc = calculate_flux_profiles(soln.flux_sum, soln.dni_applied_to_measured, soln.plant_defocus, soln.od_control, flux_map_input);  // Calculate flux profiles
		solve_for_mass_flow(soln);	// Iterative calculation of mass flow to produce target outlet temperature

		if (soln.rec_is_off)
			break;

		double m_dot_tot = soln.m_dot_tot;

		// Limit the HTF mass flow rate to the maximum, if needed
		rec_is_defocusing = false;
		if ((m_dot_tot > m_dot_htf_max))
		{
			double err_od = (m_dot_tot - m_dot_htf_max) / m_dot_htf_max;
			if (err_od < m_tol_od)
			{
				rec_is_defocusing = false;
			}
			else
			{
				soln.od_control = soln.od_control * pow((m_dot_htf_max / m_dot_tot), 0.8);	//[-] Adjust the over-design defocus control by modifying the current value
				rec_is_defocusing = true;
			}
		}

	}

	return;
}

// Calculate defocus needed to maintain outlet temperature under the target value given DNI and mass flow

void C_falling_particle_receiver::solve_for_defocus_given_flow(s_steady_state_soln &soln, const util::matrix_t<double> *flux_map_input)
{ 
	
	double Tprev, od, odprev, odlow, odhigh;
	double tol = 0.00025;
	double urf = 0.8;

	Tprev = odprev = odlow = std::numeric_limits<double>::quiet_NaN();
	od = soln.od_control;
	odhigh = 1.0;

	od = odprev * (m_T_particle_hot_target - soln.T_particle_cold_in) / (Tprev - soln.T_particle_cold_in);

	int q = 0;
	while (q<50)
	{
		soln.od_control = od;
		if (odprev != odprev)
			soln.q_dot_inc = calculate_flux_profiles(soln.flux_sum, soln.dni_applied_to_measured, soln.plant_defocus, soln.od_control, flux_map_input);
		else
			soln.q_dot_inc = soln.q_dot_inc * soln.od_control / odprev; // Calculate flux profiles (note flux is directly proportional to defocus control)
		
		calculate_steady_state_soln(soln, tol, false);     // Solve steady state thermal model 

		if (soln.od_control > 0.9999 && soln.T_particle_hot < m_T_particle_hot_target)  // Impossible for solution to achieve temperature target
			break;
		else if ((std::abs(soln.T_particle_hot - m_T_particle_hot_target) / m_T_particle_hot_target) < tol)
			break;
		else
		{
			
			if (soln.rec_is_off)
			{
				odlow = soln.od_control;
				od = odlow + 0.5*(odhigh - odlow);
			}
			else if (odprev != odprev)
			{
				od = odprev * (m_T_particle_hot_target - soln.T_particle_cold_in) / (Tprev - soln.T_particle_cold_in);
			}
			else
			{
				double delta_od = (soln.T_particle_hot - m_T_particle_hot_target) / ((soln.T_particle_hot - Tprev) / (soln.od_control - odprev));
				double od = soln.od_control - urf * delta_od;
				if (od < odlow || od > odhigh)
				{
					if (odlow == odlow)
						od = odlow + 0.5*(odhigh - odlow);
					else
						od = od * 0.95*odhigh;
				}
			}

			odprev = soln.od_control;
			Tprev = soln.T_particle_hot;
		}
		q++;
	}

	return;
}

double C_falling_particle_receiver::sandia_efficiency_correlation(bool is_multistage, double Q_inc, double v_wind, double wind_direc)
{
    // Receiver efficiency correlations from Sandia (https://www.osti.gov/biblio/1890267, page 43)
    // TODO: Check/warn about bounds where the correlation is valid?
    double q, theta, val, eta;
    double A, B, C, D, E, F, G, H;
    if (! is_multistage) // Free-falling particle receiver
    {
        A = 0.848109;
        B = 0.249759;
        C = -1.0115660;
        D = -7.942869e-5;
        E = -1.4575091E-07;
        F = 5.5;
        G = 7.5;
        H = 5000.;
    }
    else  // Multistage particle receiver
    {
        A = 0.9351;
        B = -0.0560;
        C = -0.5519;
        D = -6.4055e-5;
        E = -3.1344e-6;
        F = 5.0;
        G = 9.1;
        H = 5000.;
    }

    // Adjust wind direction relative to receiver orientation
    double wdir_eff = wind_direc - m_rec_orientation;  
    if (wdir_eff < 0)
        wdir_eff = 360 + wdir_eff;

    q = exp(-Q_inc * 1.e-6 / m_ap_area);
    val = 180 - fabs(180 - wdir_eff);
    theta = pow(val, F) * exp(-val / G) / H;
    eta = A + B * q + C * pow(q, 2) + D * q * v_wind * theta + E * pow(v_wind, 2) * theta;
    eta = fmax(eta, 0.0);
    return eta;

}

// Solve mass and momentum balances for particle velocity, curtain thickness, particle volume fraction
void C_falling_particle_receiver::solve_particle_flow(util::matrix_t<double>& mdot_per_elem, util::matrix_t<double>& phip, util::matrix_t<double>& vel, util::matrix_t<double>& th)
{
    // Mass balance: -d/dy (phip*th*rhop*vp) = 0
    // Momentum equation: -d/dy (phip*th*rhop*vp^2) + (phip*th*rhop*g) = 0 
    double g = 9.81;
    double dx = m_curtain_width / m_n_x;
    double dy = m_curtain_height / (m_n_y - 1);
    double initial_fall_height = m_initial_fall_height_fraction * m_curtain_height + m_initial_fall_height_fixed;
    double Tavg = 0.5 * (m_T_htf_cold_des + m_T_htf_hot_des);
    double rhop = field_htfProps.dens(Tavg, 1.0);

    phip.resize_fill(m_n_y, m_n_x, 0.0);
    vel.resize_fill(m_n_y, m_n_x, 0.0);
    th.resize_fill(m_n_y, m_n_x, 0.0);
    
    // Initial condition
    for (size_t i = 0; i < m_n_x; i++)
    {
        phip.at(0, i) = m_phi0;
        vel.at(0, i) = pow(2 * g * initial_fall_height, 0.5);                           // Initial particle velocity (m/s) after initial fall height assuming no drag
        th.at(0, i) = mdot_per_elem.at(i) / (phip.at(0, i) * vel.at(0, i) * rhop * dx); // Initial thickness to satisfy curtain mass flow at given initial curtain volume fraction and velocity
    }

    for (size_t j = 1; j < m_n_y; j++)
    {
        for (size_t i = 0; i < m_n_x; i++)
        {
            th.at(j,i) = th.at(j-1,i) + m_dthdy * dy;                   // Curtain thickness assuming a fixed rate of increase along fall dimension
            vel.at(j,i) = vel.at(j-1,i) + dy * g / vel.at(j-1,i);         // From momentum equation: -(phip*th*rhop*vp) * (dvp/dy) + (phip*th*rhop*g) = 0
            phip.at(j,i) = (phip.at(j-1,i) * th.at(j-1,i) * vel.at(j-1,i)) / (th.at(j,i) * vel.at(j,i)); // From mass balance
        }
    }
    return;
}



// Calculate curtain optical properties based on approximation in Gonzalez et al. Solar Energy 213 (2021) 211–224 (https://doi.org/10.1016/j.solener.2020.11.012)
void C_falling_particle_receiver::calculate_local_curtain_optical_properties(double th, double phip, double &rhoc, double &tauc)
{
    double dp = m_particle_dp;
    double length = pow((4. / 3.) * CSP::pi * pow(0.5 * dp, 3) / phip, (1. / 3.));  // Side length of virtual cube containing one particle
    double phis = CSP::pi * pow(0.5 * dp, 2) / pow(length, 2);  // Probability of beam hitting particle in cube (assumes beam strikes cube head-on)
    double Pb = 0.5 * (1.0 - m_particle_abs);   // Probability of backward reflection
    double Ps = 0.125 * (1.0 - m_particle_abs); // Probability of side reflection
    double Nl = th / length;                    // Number of layers in curtain
    double f = 1.0 / (1.0 - Pb - 2 * Ps) + (Pb + 2 * Ps) / pow(1 - Pb - 2 * Ps, 2);  
    double rhol1 = Pb * phis + 4 * f * pow(Ps * phis, 2) / phis;
    rhoc = rhol1 * (1.0 - pow(1.0-phis, 2*Nl)) / (1.0 - pow(1.0-phis, 2)); // Curtain reflectance

    double tc0 = pow(1-phis, Nl);  // Transmittance without hitting any layers
    double tcs = Nl*tc0*4*f*pow(Ps*phis, 2) / phis;
    double tcbf = pow(rhol1, 2) * tc0 * (pow(1-phis, 2*Nl) - Nl*pow(1-phis, 2) + Nl - 1) / pow(pow(phis, 2) - 2*phis, 2);
    tauc = tc0 + tcs + tcbf;  //Curtain transmittance

    // Optionally adjust by a user-provided multiplier for transmissivity
    tauc *= m_tauc_mult;
    tauc = min(tauc, 1.0 - rhoc);

    return;
}


// Advective loss coefficients from the best-fit correlations to Sandia's CFD model based on Gonzalez et al. Solar Energy 255 (2023) 301–313 (https://doi.org/10.1016/j.solener.2023.03.046)
void C_falling_particle_receiver::calculate_advection_coeff_sandia(double vel, double Tprop, double wspd, double wdir, double Pamb, double &hadv, double &fwind)
{
    
    double A, B, C, D, E, F, G, H;
    A = -12331;
    B = 1.949;
    C = 0.7002;
    D = 0.2370;
    E = 0.0089;
    F = 178.7;
    G = 134.3;
    H = 27.49;

    double k_air = ambient_air.cond(Tprop);				//[W/m-K] The conductivity of the ambient air
    double mu_air = ambient_air.visc(Tprop);			//[kg/m-s] Dynamic viscosity of the ambient air
    double rho_air = ambient_air.dens(Tprop, Pamb);		//[kg/m^3] Density of the ambient air
    double Re = rho_air * vel * m_curtain_height / mu_air;
    double Nu = A + B * pow(Re, C);
    hadv = Nu * k_air / m_curtain_height;   // Advective loss coefficient without wind (W/m2/K)

    double wind_factor = 1.0;
    double wdir_eff = wdir - m_rec_orientation;  // Adjust wind direction relative to receiver orientation
    if (wdir_eff < 0)
        wdir_eff = 360 + wdir_eff;
    double phi_wind = exp(-pow((std::abs(wdir_eff - F) - G) / H, 2));
    fwind = 1.0 + (D - E * m_ap_height) * wspd * phi_wind;

    // Scale loss coefficient to account for differences in aperture and curtain area
    // Loss coefficient from Gonzalez et al. assumes equal aperture area and curtain area and will overestimate convective losses if the loss coefficient is applied for cases where curtain area > aperture area
    hadv *= m_ap_area / m_curtain_area;

    return;
}


// Wall convection coefficient from flat-plate correlations
double C_falling_particle_receiver::calculate_wall_convection_coeff(double vel, double len, double Tprop, double P_amb)
{
    double k_air = ambient_air.cond(Tprop);				//[W/m-K] The conductivity of the ambient air
    double mu_air = ambient_air.visc(Tprop);			//[kg/m-s] Dynamic viscosity of the ambient air
    double rho_air = ambient_air.dens(Tprop, P_amb);		//[kg/m^3] Density of the ambient air
    double cp_air = ambient_air.Cp(Tprop) * 1000;	    //[J/kg-K] Specific heat of the ambient air

    double Pr = cp_air * mu_air / k_air;
    double Re = rho_air * vel * len / mu_air;
    double Nu = 0.0296 * pow(Re, 0.8) * pow(Pr, 1. / 3.);
    double h = Nu * k_air / len;
    return h;
}


int C_falling_particle_receiver::get_nelem()
{
    int nelem = 1 + 3 * m_n_x_rad * m_n_y_rad + 1;   // One element for the aperture, nx*ny elements on each side of the curtain and the back wall, one element for front wall
    return nelem;
}


void C_falling_particle_receiver::calculate_view_factors()
{
    int i, j, k, isym, ksym, ib, iw;

    int ny = m_n_y_rad;  
    int nx = m_n_x_rad;
    int nelem = get_nelem();
    double dy = m_curtain_height / (m_n_y-1);
    double dx = m_curtain_width / m_n_x;

    double top_height;
    if (m_is_ap_at_bot)
        top_height = m_curtain_height - m_ap_height;  // Distance between top of cavity and top of aperture
    else
        top_height = 0.5 * (m_curtain_height - m_ap_height);
    double side_width = 0.5 * (m_cav_width - m_ap_width);

    m_vf.resize_fill(nelem, nelem, 0.0);  // View factor matrix. Curtain height/width elements are ordered as (y0,x0), (y0,x1)... (y0,xn), (y1,x0)...

    // View factors from each curtain element to the aperture and front wall(note, this assumes the curtain is flat!)
    // TODO: Update for a curved curtain
    int nxsim = nx;
    bool is_symmetric = false;

    /*
    bool is_symmetric = (m_n_x % nx == 0);   // If the number of elements per group is the same for all width groups only need to calculate view factors for half of them
    if (is_symmetric && nx>1)
    {
        if (nx % 2 == 0)
            nxsim = (int)(nx / 2);
        else
            nxsim = (int)((nx + 1) / 2);
    }
    */

    double vf_ap_sum = 0.0;
    double vf_fw_sum = 0.0;
    int y0, x0;
    double dyg, dxg;
    int ifw = nelem - 1;   // Index of front wall

    m_vf_curtain_ap_avg = 0.0;
    y0 = 0;
    for (j = 0; j < ny; j++)
    {
        dyg = m_ny_per_group.at(j) * dy;     // y-extent of this group

        x0 = 0;
        for (k = 0; k < nxsim; k++)
        {
            dxg = m_nx_per_group.at(k) * dx;     // x-extent of this group

            i = 1 + j * nx + k;  // Front curtain element index in flattened array
            m_vf.at(i, 0) = vf_parallel_rect(x0*dx, y0*dy, side_width, top_height, dxg, dyg, m_ap_width, m_ap_height, m_curtain_dist);  // View factor from front curtain element to full aperture
            m_vf.at(0, i) = m_vf.at(i, 0) * (dxg * dyg / m_ap_area);                 // View factor from aperture to front curtain element
            m_vf.at(i, ifw) = 1.0 - m_vf.at(i, 0);                                   // View factor from front curtain element to front wall. Each element can only see the aperture or the front wall 
            m_vf.at(ifw, i) = m_vf.at(i, ifw) * (dxg * dyg / m_cav_front_area);     // View factor from front wall to front curtain element
            vf_ap_sum += m_vf.at(0, i);             // Sum of view factors from aperture to front curtain
            vf_fw_sum += m_vf.at(ifw, i);           // Sum of view factors from front wall to front curtain
            m_vf_curtain_ap_avg += m_vf.at(i, 0) / (nx * ny);  // Average view factor from front curtain to aperture

            if (is_symmetric && nx > 1)
            {
                ksym = nx - k - 1;
                isym = 1 + j * nx + ksym;   // Symmetric curtain element
                if (isym != i)
                {
                    m_vf.at(isym, 0) = m_vf.at(i, 0);
                    m_vf.at(0, isym) = m_vf.at(0, i);
                    m_vf.at(isym, ifw) = m_vf.at(i, ifw);
                    m_vf.at(ifw, isym) = m_vf.at(ifw, i);
                    vf_ap_sum += m_vf.at(0, isym);
                    vf_fw_sum += m_vf.at(ifw, isym);
                    m_vf_curtain_ap_avg += m_vf.at(isym, 0) / (nx * ny);
                }
            }
            x0 += m_nx_per_group.at(k);
        }
        y0 += m_ny_per_group.at(j);
    }

    // View factors from curtain back to back wall.  Currently assumes that curtain is close enough to the back wall that each curtain element can only see the back wall element immediately behind it.
    // TODO: Add option to calculate all curtain-to-back-wall view factors
    for (j = 0; j < ny; j++)
    {
        for (k = 0; k < nx; k++)
        {
            ib = 1 + (nx * ny) + j * nx + k;
            iw = 1 + 2 * (nx * ny) + j * nx + k;
            m_vf.at(ib, iw) = 1.0;
            m_vf.at(iw, ib) = 1.0;
        }
    }

    // View factors from aperture to front wall, and front wall to itself
    m_vf.at(0, ifw) = 1.0 - vf_ap_sum;
    m_vf.at(ifw, 0) = m_vf.at(0, ifw) * (m_ap_area / m_cav_front_area);
    m_vf.at(ifw, ifw) = 1.0 - vf_fw_sum - m_vf.at(ifw, 0);

    return;
}

// View factors between parallel rectangles (http://www.thermalradiation.net/sectionc/C-13.html)
// x1, x2=x1+dx, y1, y2=y1+dy are extents of surface 1; e1, e2=e1+de, n1, n2=n1+dn are extents of surface 2, z is the separation distance between rectangles 
double C_falling_particle_receiver::vf_parallel_rect(double x1, double y1, double e1, double n1, double dx, double dy, double de, double dn, double z)
{
    int i, j, k, m;
    double x, y, n, e, v1, v2, f;
    double vf = 0.0;
    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < 2; j++)
        {
            for (k = 0; k < 2; k++)
            {
                for (m = 0; m < 2; m++)
                {
                    x = x1 + i * dx;
                    y = y1 + j * dy;
                    n = n1 + k * dn;
                    e = e1 + m * de;
                    v1 = pow(pow(x - e, 2) + pow(z, 2), 0.5);
                    v2 = pow(pow(y - n, 2) + pow(z, 2), 0.5);
                    f = (y - n) * v1 * atan((y - n) / v1) + (x - e) * v2 * atan((x - e) / v2) - 0.5 * pow(z, 2) * log(pow(x - e, 2) + pow(y - n, 2) + pow(z, 2));
                    vf += pow(-1.0, i + j + k + m) * (1. / 2. / CSP::pi) * f;
                }
            }
        }
    }
    vf /= (dx * dy);
    return vf;
}

// Set up array containing reflectivity of all surface elements
util::matrix_t<double> C_falling_particle_receiver::get_reflectivity_vector(util::matrix_t<double>& rhoc_per_group, double rhow)
{
    int ny = m_n_y_rad; 
    int nx = m_n_x_rad;
    int nelem = get_nelem();
    util::matrix_t<double> rho(nelem);
    int x, y, i0;
    for (int i = 0; i < nelem; i++)
    {
        if (i == 0)
            rho.at(i) = 0.0;  // Aperture
        else if (i < 1 + (nx * ny))  // Front of curtain
        {
            i0 = 1;
            x = (i - i0) % nx;        // Width position on curtain
            y = (i - i0 - x) / nx;    // Height position on curtain
            rho.at(i) = rhoc_per_group.at(y, x);
        }
        else if (i < 1 + 2 * (nx * ny))  // Back of curtain
        {
            i0 = 1 + (nx * ny);
            x = (i - i0) % nx;
            y = (i - i0 - x) / nx;    // Height position on curtain
            rho.at(i) = rhoc_per_group.at(y, x);
        }
        else            // Back cavity wall or front cavity wall
            rho.at(i) = rhow;
    }
    return rho;
}

// Coefficient matrix for radiative exchange including all discretized elements
void C_falling_particle_receiver::calculate_coeff_matrix(util::matrix_t<double>& rhoc, util::matrix_t<double>& tauc, double rhow, util::matrix_t<double>& K, util::matrix_t<double>& Kinv)
{

    // TODO: Generalize to include view factors between each curtain element and all back wall elements
    //       Simplify loops below if each curtain element can only see one back wall element (most of the elements in K are zero)

    int ny = m_n_y_rad;  
    int nx = m_n_x_rad;
    int nelem = get_nelem();

    // Calculate average curtain properties per element groupings
    util::matrix_t<double> rhoc_per_group, tauc_per_group;
    rhoc_per_group.resize_fill(ny, nx, 0.0);
    tauc_per_group.resize_fill(ny, nx, 0.0);
    int j0, i0;
    j0 = 0;
    for (int j = 0; j < ny; j++)
    {
        i0 = 0;
        for (int i = 0; i < nx; i++)
        {
            int npg = m_ny_per_group.at(j) * m_nx_per_group.at(i);
            for (int j1 = 0; j1 < m_ny_per_group.at(j); j1++)
            {
                for (int i1 = 0; i1 < m_nx_per_group.at(i); i1++)
                {
                    rhoc_per_group.at(j, i) += rhoc.at(j0 + j1, i0 + i1) / npg;
                    tauc_per_group.at(j, i) += tauc.at(j0 + j1, i0 + i1) / npg;
                }
            }
            i0 += m_nx_per_group.at(i);
        }
        j0 += m_ny_per_group.at(j);
    }

    // Create vector with reflectivity of all elements
    util::matrix_t<double> rho(nelem);
    rho = get_reflectivity_vector(rhoc_per_group, rhow);

    //--- Calculate coefficient matrix
    int iback, ifront, x, y;
    K.resize(nelem, nelem);  // Curtain height/width elements are ordered as (y0,x0), (y0,x1)... (y0,xn), (y1,x0)...
    for (int i1 = 0; i1 < nelem; i1++)
    {
        for (int i2 = 0; i2 < nelem; i2++)
        {
            K.at(i1, i2) = (i2 == i1) ? 1.0 : 0.0;
            K.at(i1, i2) -= rho.at(i1) * m_vf.at(i1, i2);

            // Add modifications accounting for transmissivity of curtain
            if (i1 >= 1 && i1 < 1 + (nx * ny))  // Element on front of curtain
            {
                iback = i1 + (nx * ny);       // Corresponding element on the back of the curtain
                i0 = 1;
                x = (i1 - i0) % nx;           // Width position on curtain
                y = (i1 - i0 - x) / nx;       // Height position on curtain
                K.at(i1, i2) -= tauc_per_group.at(y, x) * m_vf.at(iback, i2);
            }

            else if (i1 >= 1 + (nx * ny) && i1 < 1 + 2*(nx * ny))  // Element on back of curtain
            {
                ifront = i1 - (nx * ny);  // Corresponding element on the front of the curtain
                i0 = 1 + (nx * ny);
                x = (i1 - i0) % nx;       // Width position on curtain
                y = (i1 - i0 - x) / nx;   // Height position on curtain
                K.at(i1, i2) -= tauc_per_group.at(y, x) * m_vf.at(ifront, i2);
            }
        }
    }

    //--- Invert coefficient matrix
    if (m_invert_matrices)
    {
        if (!m_use_eigen)
            Kinv = matrix_inverse(K);
        else
        {
            Eigen::MatrixXd K_eigen, Kinv_eigen;
            K_eigen.resize(nelem, nelem);
            for (int i1 = 0; i1 < nelem; i1++)
            {
                for (int i2 = 0; i2 < nelem; i2++)
                    K_eigen(i1, i2) = K.at(i1, i2);
            }
            Kinv_eigen = K_eigen.inverse();

            Kinv.resize(nelem, nelem);
            for (int i1 = 0; i1 < nelem; i1++)
            {
                for (int i2 = 0; i2 < nelem; i2++)
                    Kinv.at(i1, i2) = Kinv_eigen(i1, i2);
            }
        }
    }

    return;
}


// Solve radiative exchange equations and calculate net radiative energy incoming to each element
void C_falling_particle_receiver::calculate_radiative_exchange(util::matrix_t<double>& Ecf, util::matrix_t<double>& Ecb, util::matrix_t<double>& Ebw, double Eap, double Efw,
                                                               util::matrix_t<double>& K, util::matrix_t<double>& Kinv,
                                                               util::matrix_t<double>& rhoc, util::matrix_t<double>& tauc, double rhow,
                                                               util::matrix_t<double>& qnetc, util::matrix_t<double>& qnetw, double& qnetwf, double& qnetap)
{
    // Curtain height/width elements are ordered as (y0,x0), (y0,x1)... (y0,xn), (y1,x0)...
    int ny = m_n_y_rad;  
    int nx = m_n_x_rad;
    int nelem = get_nelem();

    util::matrix_t<double> Ee, Je;
    Ee.resize(nelem);
    Je.resize(nelem);

    //--- Calculate average values per element groupings and set up vector of radiative energy source
    int j0, i0;
    double Ecf_per_group, Ecb_per_group, Ebw_per_group;
    util::matrix_t<double> rhoc_per_group, tauc_per_group;
    rhoc_per_group.resize_fill(ny, nx, 0.0);
    tauc_per_group.resize_fill(ny, nx, 0.0);
    j0 = 0;
    for (int j = 0; j < ny; j++)      // Radiation groups in y-dimension
    {
        i0 = 0;
        for (int i = 0; i < nx; i++)  // Radiation groups in x-dimension
        {
            // Calculate average per group
            int npg = m_ny_per_group.at(j) * m_nx_per_group.at(i);
            Ecf_per_group = Ecb_per_group = Ebw_per_group = 0.0;
            for (int j1 = 0; j1 < m_ny_per_group.at(j); j1++)       // Discretized elements contained in this radiation group
            {
                for (int i1 = 0; i1 < m_nx_per_group.at(i); i1++)   // Discretized elements contained in this radiation group
                {
                    rhoc_per_group.at(j, i) += rhoc.at(j0 + j1, i0 + i1) / npg;
                    tauc_per_group.at(j, i) += tauc.at(j0 + j1, i0 + i1) / npg;
                    Ecf_per_group += Ecf.at(j0 + j1, i0 + i1) / npg;
                    Ecb_per_group += Ecb.at(j0 + j1, i0 + i1) / npg;
                    Ebw_per_group += Ebw.at(j0 + j1, i0 + i1) / npg;
                }
            }

            // Put values in flattened array
            Ee.at(1 + j*nx + i) = Ecf_per_group;
            Ee.at(1 + ny*nx + j*nx + i) = Ecb_per_group;
            Ee.at(1 + 2*ny*nx + j*nx + i) = Ebw_per_group;

            i0 += m_nx_per_group.at(i);
        }
        j0 += m_ny_per_group.at(j);
    }
    Ee.at(0) = Eap;
    Ee.at(nelem - 1) = Efw;


    //--- Solve for radiosity (total outgoing energy)
    if (m_invert_matrices)
    {
        for (int i = 0; i < nelem; i++)
        {
            Je.at(i) = 0.0;
            for (int j = 0; j < nelem; j++)
            {
                Je.at(i) += Kinv.at(i, j) * Ee.at(j);
            }
        }
    }

    else if (m_use_eigen)
    {
        Eigen::MatrixXd K_eigen;
        Eigen::VectorXd Ee_eigen, Je_eigen;
        Ee_eigen.resize(nelem);
        Je_eigen.resize(nelem);
        K_eigen.resize(nelem, nelem);
        for (int i = 0; i < nelem; i++)
        {
            Ee_eigen(i) = Ee.at(i);
            for (int j = 0; j < nelem; j++)
                K_eigen(i, j) = K.at(i, j);
        }

        Je_eigen = K_eigen.colPivHouseholderQr().solve(Ee_eigen);  // TODO: try different methods?
        for (int i = 0; i < nelem; i++)
            Je.at(i) = Je_eigen(i);
    }




    //-- Solve for total incoming energy and net incoming energy
    util::matrix_t<double> rho = get_reflectivity_vector(rhoc_per_group, rhow);
    util::matrix_t<double> qin, qnet;
    qin.resize_fill(nelem, 0.0);            // Total incoming energy [W/m2]
    qnet.resize_fill(nelem, 0.0);           // Net incoming energy
    int x, y;
    for (int i = 0; i < nelem; i++)
    {
        for (int j = 0; j < nelem; j++)
            qin.at(i) += m_vf.at(i, j) * Je.at(j);
        qnet.at(i) = (1.0 - rho.at(i)) * qin.at(i) - Ee.at(i);

        // Modification to net incoming energy for curtain elements
        if (i > 0 && i < 1 + 2*nx*ny)
        {
            int k = (i < 1 + nx * ny) ? 1 : 1 + nx * ny;
            x = (i - k) % nx;           // Width position on curtain
            y = (i - k - x) / nx;       // Height position on curtain
            qnet.at(i) -= tauc_per_group.at(y, x) * qin.at(i);
        }
    }

    //-- Separate net incoming energy into separate outputs for curtain elements, back wall elements, front wall, aperture
    //   qnet = (1-rho)*qin - E for any given element
    //   If using radiation groups, assume (1-rho)*qin is the same for all elements in the group, and re-calculate qnet for the local element E instead of the group-average E
    qnetc.resize_fill(m_n_y, m_n_x, 0.0);   // Net incoming energy to the curtain (both sides)
    qnetw.resize_fill(m_n_y, m_n_x, 0.0);   // Net incoming energy to the back wall
    qnetap = qnet.at(0);                    // Net energy incoming to aperture
    qnetwf = qnet.at(nelem - 1);            // Net energy incoming to front wall
    i0 = 0;
    for (int i = 0; i < nx; i++)    // Radiation groups in x-dimension
    {
        j0 = 0;
        for (int j = 0; j < ny; j++)    // Radiation groups in y-dimension
        {
            int kf = 1 + j * nx + i;                // Element on front of curtain
            int kb = 1 + nx * ny + j * nx + i;      // Element on back of curtain
            int kw = 1 + 2 * nx * ny + j * nx + i;  // Element on back wall

            for (int i1 = 0; i1 < m_nx_per_group.at(i); i1++)           // Discretized x-elements contained in this radiation group
            {
                for (int j1 = 0; j1 < m_ny_per_group.at(j); j1++)       // Discretized y-elements contained in this radiation group
                {
                    qnetc.at(j0 + j1, i0 + i1) += qnet.at(kf) + Ee.at(kf) - Ecf.at(j0 + j1, i0 + i1);
                    qnetc.at(j0 + j1, i0 + i1) += qnet.at(kb) + Ee.at(kb) - Ecb.at(j0 + j1, i0 + i1);
                    qnetw.at(j0 + j1, i0 + i1) += qnet.at(kw) + Ee.at(kw) - Ebw.at(j0 + j1, i0 + i1);
                }

                // Assume last vertical node is equal to previous node (this node is not used in the energy balance, this value is just to define something reasonable to complete wall temperature arrays)
                if (j == ny - 1)
                {
                    qnetc.at(m_n_y - 1, i0 + i1) += qnet.at(kf) + Ee.at(kf) - Ecf.at(m_n_y - 1, i0 + i1);
                    qnetc.at(m_n_y - 1, i0 + i1) += qnet.at(kb) + Ee.at(kb) - Ecb.at(m_n_y - 1, i0 + i1);
                    qnetw.at(m_n_y - 1, i0 + i1) += qnet.at(kw) + Ee.at(kw) - Ebw.at(m_n_y - 1, i0 + i1);
                }

            }
            j0 += m_ny_per_group.at(j);
        }
        i0 += m_nx_per_group.at(i);
    }

    return;
}


// Calculate passive surface temperature
double C_falling_particle_receiver::calculate_passive_surface_T(double Tguess, double qnet_rad, double hconv, double Tconv, double Tamb, bool is_axial_conduction, double Tcond_prev, double Tcond_next)
{
    // Twguess = current guess for wall temperature
    // qnet_rad = current estimate of net radiative heat flux into wall (incoming - outgoing including emission)
    // Tamb = ambient temperature
    // hconv, Tconv = heat transfer coefficient and temperature
    // is_axial_conduction = include axial condition?  If True, Tcond_prev and Tcond_next are current guess for solution at previous/next wall temperature nodes

    // Wall energy balance: d/dy(kdTw/dy) + qrad - hconv*(Tw-Tconv) - qext = 0  (with axial conduction) where qrad = net incoming radiative flux
    // Solution is based on current guess for cavity radiative exchange with wall emission recomputed:
    //      qrad = qrad* - (Ew - Ew*) where qrad* = current estimate of radiative flux at Tguess, Ew* = wall emission at Tguess, Ew = wall emission at new wall T
    //      New wall temperature solution is found by iterating on linearization temperature (Tw^4 = Tlin^4 * 4Tlin^3*(Tw-Tlin) for numerical stability


    double Rwall, dy, num, den, Tlin, Tw, Tdiff, tol, urf;
    int niter = 10;  // Max number of iterations
    tol = 0.1;      // Tolerance for wall temperature solution
    Rwall = 1.0 / m_cav_hext + m_cav_twall / m_cav_kwall;  // Cavity wall thermal resistance
    dy = m_curtain_height / (m_n_y - 1);
    urf = 1.0;

    Tlin = Tguess;
    for (int m = 0; m < niter; m++)
    {
        den = 4 * m_cav_emis * CSP::sigma * pow(Tlin, 3) + hconv + 1.0 / Rwall;
        num = qnet_rad + m_cav_emis * CSP::sigma * (3 * pow(Tlin, 4) + pow(Tguess, 4)) + hconv * Tconv + Tamb / Rwall;
        if (is_axial_conduction)
        {
            den += 2 * m_cav_kwall / pow(dy, 2);
            num += m_cav_kwall * (Tcond_next + Tcond_prev) / (pow(dy, 2));
        }
        Tw = num / den;
        Tdiff = std::abs(Tw - Tlin);
        if (Tdiff < tol)
            break;
        Tlin = (1.0 - urf) * Tlin + urf*Tw;
    }
    return Tw;
}

// Calculate mass-weighted average particle exit temperature
double C_falling_particle_receiver::calculate_mass_wtd_avg_exit(util::matrix_t<double>& m_flow_per_elem, util::matrix_t<double>& mat)
{
    int nx = (int)mat.ncols();
    int ny = (int)mat.nrows();
    double mtot = 0.0;
    double outlet = 0.0;
    for (int i = 0; i < nx; i++)
    {
        mtot += m_flow_per_elem.at(i);
        outlet += m_flow_per_elem.at(i) * mat.at(ny - 1, i);
    }
    outlet /= mtot;
    return outlet;
}

util::matrix_t<double> C_falling_particle_receiver::sum_over_rows(util::matrix_t<double>& mat, bool exclude_last)
{
    int nr = mat.nrows();
    int nc = mat.ncols();
    int nrsum = nr;
    if (exclude_last)
        nrsum -= 1;

    util::matrix_t<double> msum;
    msum.resize_fill(nc, 0.0);
    for (int i = 0; i < nc; i++)
    {
        for (int j = 0; j < nrsum; j++)
            msum.at(i) += mat.at(j, i);
    }
    return msum;
}

util::matrix_t<double> C_falling_particle_receiver::sum_over_cols(util::matrix_t<double>& mat)
{
    int nr = mat.nrows();
    int nc = mat.ncols();
    util::matrix_t<double> msum;
    msum.resize_fill(nr, 0.0);
    for (int j = 0; j < nr; j++)
    {
        for (int i = 0; i < nc; i++)
            msum.at(j) += mat.at(j, i);
    }
    return msum;
}

double C_falling_particle_receiver::sum_over_rows_and_cols(util::matrix_t<double>& mat, bool exclude_last_row)
{
    int nr = mat.nrows();
    int nc = mat.ncols();
    int nrsum = nr;
    if (exclude_last_row)
        nrsum -= 1;

    double msum = 0.0;
    for (int j = 0; j < nrsum; j++) {
        for (int i = 0; i < nc; i++) {
            msum += mat.at(j, i);
        }
    }
    return msum;
}

double C_falling_particle_receiver::max_over_rows_and_cols(util::matrix_t<double>& mat, bool exclude_last_row)
{
    int nr = mat.nrows();
    int nc = mat.ncols();
    if (exclude_last_row)
        nr -= 1;

    double mmax = -1e10;
    for (int j = 0; j < nr; j++) {
        for (int i = 0; i < nc; i++) {
            mmax = fmax(mmax, mat.at(j, i));
        }
    }
    return mmax;
}

util::matrix_t<double> C_falling_particle_receiver::matrix_addition(util::matrix_t<double>& m1, util::matrix_t<double>& m2)
{

    size_t nrow = m1.nrows();
    size_t ncol = m1.ncols();
    if (m1.nrows() != nrow || m1.ncols() != ncol)
        throw(C_csp_exception("Array dimensions don't match ", "Particle receiver initialization"));

    util::matrix_t<double> m3;
    m3.resize(nrow, ncol);
    for (int i = 0; i < ncol; i++)
    {
        for (int j = 0; j < nrow; j++)
        {
           m3.at(j,i) = m1.at(j,i) + m2.at(j,i); 
        }
    }
    return m3;
}


util::matrix_t<double> C_falling_particle_receiver::matrix_inverse(util::matrix_t<double>& m)
{
    // Gauss-Jordan method for matrix inversion
    size_t n = m.nrows();
    util::matrix_t<double> A = m;
    util::matrix_t<double> B(n, n, 0.0);
    for (int i = 0; i < n; i++)
        B.at(i, i) = 1.0;

    double v;
    for (int i = 0; i < n; i++)
    {
        v = A.at(i, i);
        if (v != 1.0)  // Multiply row i by (1/v) to make diagonal entry in column i equal to 1.0
        {
            for (int j = i; j < n; j++)
                A.at(i, j) *= (1.0 / v);  //(A[i, j] is already zero for j < i)
            for (int j = 0; j<=i; j++)
                B.at(i, j) *= (1.0 / v);  //(B[i, j] is already zero for j > i)
        }

        for (int j = 0; j < n; j++)  // Use row i to eliminate entries in all other rows
        {
            v = A.at(j, i);
            if (i != j && v!=0.0)
            {
                for (int k = i; k<n; k++)
                    A.at(j, k) -= v * A.at(i, k);  //A[i, k] is zero for k < i
                for (int k = 0; k<=i; k++)
                    B.at(j, k) -= v * B.at(i, k); // B[i,k] is zero for k > i
            }
        }
    }
    return B;
}


double C_falling_particle_receiver::estimate_thermal_efficiency(const C_csp_weatherreader::S_outputs& weather, double q_inc)
{
    // q_inc in MWt
    double eta, v_wind, wdir;
    v_wind = scale_wind_speed(weather.m_wspd);  // Wind speed at receiver height
    wdir = weather.m_wdir;

    if (m_model_type == 0) // Fixed user-defined receiver efficiency
    {
        return m_fixed_efficiency;
    }
    else if (m_model_type == 1 || m_model_type == 2) // Receiver efficiency correlations from Sandia (https://www.osti.gov/biblio/1890267, page 43)
    {
        eta = 0.0;
        if (q_inc > 0.0)
            eta = sandia_efficiency_correlation(m_model_type == 2, q_inc * 1e6, v_wind, wdir);
        return eta;
    }
    else if (m_model_type == 3)        // Quasi-2D physics-based receiver model
    {
        throw(C_csp_exception("Estimated thermal efficiency is not set up for particle receiver with quasi-2D model", "Falling particle receiver"));
    }
}
