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

#include "csp_solver_mspt_receiver_222.h"
#include "csp_solver_core.h"
#include "sam_csp_util.h"

#include "Ambient.h"
#include "definitions.h"

C_mspt_receiver_222::C_mspt_receiver_222(double h_tower /*m*/, double epsilon /*-*/,
    double T_htf_hot_des /*C*/, double T_htf_cold_des /*C*/,
    double f_rec_min /*-*/, double q_dot_rec_des /*MWt*/,
    double rec_su_delay /*hr*/, double rec_qf_delay /*-*/,
    double m_dot_htf_max_frac /*-*/, double eta_pump /*-*/,
    double od_tube /*mm*/, double th_tube /*mm*/,
    double piping_loss_coefficient /*Wt/m2-K*/, double pipe_length_add /*m*/, double pipe_length_mult /*-*/,
    int field_fl, util::matrix_t<double> field_fl_props,
    int tube_mat_code /*-*/,
    int night_recirc /*-*/,
    int n_panels /*-*/, double d_rec /*m*/, double h_rec /*m*/,
    int flow_type /*-*/, int crossover_shift /*-*/, double hl_ffact /*-*/,
    double T_salt_hot_target /*C*/, double csky_frac /*-*/,
    bool is_calc_od_tube /*-*/, double W_dot_rec_target /*MWe*/,
    int rec_shutdown_method /**/, double rec_horizon /*min*/, int rec_low_power_flow_method /**/) : C_pt_receiver(h_tower, epsilon,
        T_htf_hot_des, T_htf_cold_des,
        f_rec_min, q_dot_rec_des,
        rec_su_delay, rec_qf_delay,
        m_dot_htf_max_frac, eta_pump,
        od_tube, th_tube,
        piping_loss_coefficient, pipe_length_add,
        pipe_length_mult,
        field_fl, field_fl_props,
        tube_mat_code,
        night_recirc)
{
    // Parameters not shared upstream with C_pt_receiver
    m_n_panels = n_panels;      //[-]
    m_d_rec = d_rec;            //[m]
    m_h_rec = h_rec;            //[m]
    m_flow_type = flow_type;    //[-]
    m_crossover_shift = crossover_shift;    //[-]
    m_hl_ffact = hl_ffact;      //[-]
    m_T_salt_hot_target = T_salt_hot_target + 273.15;   //[K] convert from C
    m_csky_frac = csky_frac;    //[-]

    m_is_calc_od_tube = is_calc_od_tube;    //[-]
    m_W_dot_rec_target = W_dot_rec_target;  //[MWe]

    // Hardcoded (for now?) parameters
    m_tol_od = 0.001;		//[-] Tolerance for over-design iteration
    m_eta_therm_des_est = 0.9;  //[-] Estimated and used to calculate min incident flux
    m_m_mixed = 3.2;        //[-] Exponential for calculating mixed convection

    // Calculated parameters
    m_id_tube = std::numeric_limits<double>::quiet_NaN();
	m_A_tube = std::numeric_limits<double>::quiet_NaN();
	m_n_t = -1;
	m_A_rec_proj = std::numeric_limits<double>::quiet_NaN();
	m_A_node = std::numeric_limits<double>::quiet_NaN();
    m_Rtot_riser = std::numeric_limits<double>::quiet_NaN();
    m_Rtot_downc = std::numeric_limits<double>::quiet_NaN();

    m_Q_dot_piping_loss = std::numeric_limits<double>::quiet_NaN();

    m_flow_pattern = 0;
	m_n_lines = -1;

	m_LoverD = std::numeric_limits<double>::quiet_NaN();
	m_RelRough = std::numeric_limits<double>::quiet_NaN();

    // State variables
    m_mode_initial = C_csp_collector_receiver::E_csp_cr_modes::OFF;
    m_E_su_init = std::numeric_limits<double>::quiet_NaN();
    m_t_su_init = std::numeric_limits<double>::quiet_NaN();

    m_E_su_prev = std::numeric_limits<double>::quiet_NaN();
    m_t_su_prev = std::numeric_limits<double>::quiet_NaN();

    // Private
    m_E_su = std::numeric_limits<double>::quiet_NaN();
	m_t_su = std::numeric_limits<double>::quiet_NaN();

	m_ncall = -1;

    m_use_constant_piping_loss = true;


    // Receiver on/off operational decisions
    m_shutdown_method = rec_shutdown_method;
    m_low_power_flow_method = rec_low_power_flow_method;
    if (m_shutdown_method == 1)
    {
        m_moving_avg_horizon = rec_horizon * 60; // Rolling average time horizon for on/off operational decisions [s]
    }
    else if (m_shutdown_method == 2)
    {
        m_shutdown_time_lag = rec_horizon * 60;
        m_reset_lag = rec_horizon * 60;
    }
    m_is_below_cutoff = false;
    m_m_dot_htf = std::numeric_limits<double>::quiet_NaN();
    m_m_dot_htf_prev = std::numeric_limits<double>::quiet_NaN();
    m_moving_avg_power = std::numeric_limits<double>::quiet_NaN();
    m_q_dot_inc_sum = std::numeric_limits<double>::quiet_NaN();
    m_current_step = std::numeric_limits<double>::quiet_NaN();
    m_low_power_counter = std::numeric_limits<double>::quiet_NaN();
    m_low_power_counter_prev = std::numeric_limits<double>::quiet_NaN();
    m_reset_counter = std::numeric_limits<double>::quiet_NaN();
    m_reset_counter_prev = std::numeric_limits<double>::quiet_NaN();

}

void C_mspt_receiver_222::get_solved_design_common(double& m_dot_rec_total /*kg/s*/,
    double& T_htf_cold_des /*K*/, int& n_panels)
{
    m_dot_rec_total = m_m_dot_htf_des;      //[kg/s]
    T_htf_cold_des = m_T_htf_cold_des;      //[K]
    n_panels = m_n_panels;                  //[-]
}

void C_mspt_receiver_222::init_mspt_common()
{
    m_id_tube = m_od_tube - 2 * m_th_tube;			//[m] Inner diameter of receiver tube
    m_A_tube = CSP::pi * m_od_tube / 2.0 * m_h_rec;	//[m^2] Outer surface area of each tube
    m_n_t = (int)(CSP::pi * m_d_rec / (m_od_tube * m_n_panels));	// The number of tubes per panel, as a function of the number of panels and the desired diameter of the receiver

    int n_tubes = m_n_t * m_n_panels;				//[-] Number of tubes in the system
    m_A_rec_proj = m_od_tube * m_h_rec * n_tubes;		    //[m^2] The projected area of the tubes on a plane parallel to the center lines of the tubes
    m_A_node = CSP::pi * m_d_rec / m_n_panels * m_h_rec;    //[m^2] The area associated with each node

    double c_htf_des = field_htfProps.Cp((m_T_htf_hot_des + m_T_htf_cold_des) / 2.0) * 1000.0;		//[J/kg-K] Specific heat at design conditions
    m_m_dot_htf_des = m_q_rec_des / (c_htf_des * (m_T_htf_hot_des - m_T_htf_cold_des));					//[kg/s]
    m_q_dot_inc_min = m_q_rec_des * m_f_rec_min / m_eta_therm_des_est;	//[W] Minimum receiver thermal power
    
    m_m_dot_htf_max = m_m_dot_htf_max_frac * m_m_dot_htf_des;	//[kg/s]

    double d_inner_piping = std::numeric_limits<double>::quiet_NaN();   //[m]
    CSP::mspt_piping_design(field_htfProps,
        m_h_tower, m_pipe_length_mult,
        m_pipe_length_add, m_piping_loss_coefficient,
        m_T_htf_hot_des, m_T_htf_cold_des,
        m_m_dot_htf_des,
        m_L_piping, d_inner_piping, m_Q_dot_piping_loss);

    m_mode = m_mode_initial;					//[-] 0 = requires startup, 1 = starting up, 2 = running
    m_mode_prev = m_mode;

    m_E_su_prev = m_q_rec_des * m_rec_qf_delay;	//[W-hr] Startup energy
    m_t_su_prev = m_rec_su_delay;				//[hr] Startup time requirement

    if (m_mode_initial == C_csp_collector_receiver::STARTUP) {
        if (std::isfinite(m_E_su_init)) {
            m_E_su_prev = std::fmin(m_q_rec_des * m_rec_qf_delay, std::fmax(0.0, m_E_su_init));
        }
        if (std::isfinite(m_t_su_init)) {
            m_t_su_prev = std::fmin(m_rec_su_delay, std::fmax(0.0, m_t_su_init));
        }
    }
    else if (m_mode_initial != C_csp_collector_receiver::OFF) {
        m_E_su_prev = 0.0;
        m_t_su_prev = 0.0;
    }

    // If no startup requirements, then receiver is always ON
    // ... in the sense that the controller doesn't need to worry about startup
    if (m_E_su_prev == 0.0 && m_t_su_prev == 0.0 && m_mode_initial == C_csp_collector_receiver::OFF) {
        m_mode = C_csp_collector_receiver::OFF_NO_SU_REQ;
        m_mode_prev = C_csp_collector_receiver::OFF_NO_SU_REQ;
    }

    std::string flow_msg;
    if (!CSP::flow_patterns(m_n_panels, m_crossover_shift, m_flow_type, m_n_lines, m_flow_pattern, &flow_msg))
    {
        throw(C_csp_exception(flow_msg, "MSPT receiver initialization"));
    }

    m_LoverD = m_h_rec / m_id_tube;     //[-]
    m_RelRough = (4.5e-5) / m_id_tube;	//[-] Relative roughness of the tubes. http:www.efunda.com/formulae/fluids/roughness.cfm


    m_moving_avg_power = 0.0;
    if (m_shutdown_method == 1)
    {
        size_t n = (m_moving_avg_horizon == 0.0) ? 2 : (size_t)(2 * m_moving_avg_horizon / 60);
        m_recent_power.resize_fill(n, 0.0);   // We don't know the simulation time step here, so for now size the array for 1 min time steps (with a buffer to deal with split time steps)
        m_recent_steps.resize_fill(n, 0.0);
    }

    m_low_power_counter = 0.0;
    m_low_power_counter_prev = 0.0;
    m_reset_counter = 0.0;
    m_reset_counter_prev = 0.0;


    // Initialize output arrays
    m_q_dot_inc.resize(m_n_panels);
    m_q_dot_inc.fill(0.0);

    m_T_s.resize(m_n_panels);
    m_T_s.fill(0.0);

    m_T_panel_out.resize(m_n_panels);
    m_T_panel_out.fill(0.0);

    m_T_panel_in.resize(m_n_panels);
    m_T_panel_in.fill(0.0);

    m_T_panel_ave.resize(m_n_panels);
    m_T_panel_ave.fill(0.0);

    m_q_dot_conv.resize(m_n_panels);
    m_q_dot_conv.fill(0.0);

    m_q_dot_rad.resize(m_n_panels);
    m_q_dot_rad.fill(0.0);

    m_q_dot_loss.resize(m_n_panels);
    m_q_dot_loss.fill(0.0);

    m_q_dot_abs.resize(m_n_panels);
    m_q_dot_abs.fill(0.0);
}

void C_mspt_receiver_222::init()
{
    C_pt_receiver::init();

    if (m_is_calc_od_tube && std::isfinite(m_W_dot_rec_target)) {

        // Guess OD
        double od_tube_init = m_od_tube;    //[m]

        C_MEQ__calc_OD_des_for_W_dot_pump_rec c_rec_des_eq(this);
        C_monotonic_eq_solver c_rec_des_solver(c_rec_des_eq);

        c_rec_des_solver.settings(1.E-4, 25, 0.002, 0.5, true);

        double od_tol_solved = std::numeric_limits<double>::quiet_NaN();
        double od_solved = std::numeric_limits<double>::quiet_NaN();
        int od_iter_solved = -1;
        int od_code = 0;

        try {
            od_code = c_rec_des_solver.solve(m_od_tube, 0.9 * m_od_tube, m_W_dot_rec_target, od_solved, od_tol_solved, od_iter_solved);
        }
        catch (C_csp_exception) {
            throw(C_csp_exception("receiver tube design iteration failed"));
        }

    }
    else {

        init_mspt_common();

        design_point_steady_state(m_eta_thermal_des_calc,
            m_W_dot_rec_pump_des_calc,
            m_W_dot_pumping_tower_share, m_W_dot_pumping_rec_share,
            m_rec_pump_coef, m_vel_htf_des);

    }
	
	m_ncall = -1;

	return;
}

void C_mspt_receiver_222::set_inital_state(C_csp_collector_receiver::E_csp_cr_modes mode_initial,
    double E_su_init /*W-hr*/, double t_su_init /*hr*/)
{
    m_mode_initial = mode_initial;
    m_E_su_init = E_su_init;
    m_t_su_init = t_su_init;
}

void C_mspt_receiver_222::call_common(double P_amb /*Pa*/, double T_amb /*K*/,
    double clearsky_to_input_dni /*adjusted clear-sky dni over actual dni*/,
    double clearsky_to_nominal_clearsky /* adjusted clear-sky dni over nominal clear-sky dni used to create flux_map_input_clearsky*/,
    double v_wind_10 /*m/s*/, double T_sky /*K*/,
    double T_salt_cold_in /*K*/,
    double plant_defocus /*-*/,
    const util::matrix_t<double>* flux_map_input /* Flux map from actual DNI [kW/m2]*/,
    const util::matrix_t<double>* flux_map_input_clearsky /* Flux map from nominal clearsky DNI [kW/m2]*/,
    C_csp_collector_receiver::E_csp_cr_modes input_operation_mode,
    double step /*s*/,
    // outputs:
    bool& rec_is_off,   
    double& eta_therm /*-*/, double& m_dot_salt_tot /*kg/s*/,
    double& T_salt_hot /*K*/, 
    double& T_coolant_prop /*K*/, double& T_salt_hot_rec /*K*/,
    double& c_p_coolant /*J/kg-K*/, double& u_coolant /*m/s*/,
    double& rho_coolant /*kg/m3*/, double& f /*-*/,
    double& q_dot_inc_pre_defocus /*Wt*/,
    double& q_dot_inc_sum /*Wt*/, double& q_conv_sum /*Wt*/,
    double& q_rad_sum /*Wt*/, double& q_dot_piping_loss /*Wt*/,
    double& q_dot_inc_min_panel /*Wt*/,
    double& q_thermal_csky /*Wt*/, double& q_thermal_steadystate /*Wt*/,
    double& od_control /*-*/,
    s_steady_state_soln& soln)
{
    if (input_operation_mode < C_csp_collector_receiver::OFF || input_operation_mode > C_csp_collector_receiver::STEADY_STATE)
    {
        error_msg = util::format("Input operation mode must be either [0,1,2], but value is %d", input_operation_mode);
        throw(C_csp_exception(error_msg, "MSPT receiver timestep performance call"));
    }

    int n_flux_y = (int)flux_map_input->nrows();
    if (n_flux_y > 1)
    {
        error_msg = util::format("The Molten Salt External Receiver (Type222) model does not currently support 2-dimensional "
            "flux maps. The flux profile in the vertical dimension will be averaged. NY=%d", n_flux_y);
        csp_messages.add_message(C_csp_messages::WARNING, error_msg);
    }
    int n_flux_x = (int)flux_map_input->ncols();

    double flux_sum = 0.0;
    for (int i = 0; i < n_flux_y; i++) {
        for (int j = 0; j < n_flux_x; j++) {
            flux_sum += flux_map_input->at(i, j);
        }
    }

    // Set current timestep stored values to NaN so we know that code solved for them
    m_mode = C_csp_collector_receiver::OFF;
    m_E_su = std::numeric_limits<double>::quiet_NaN();
    m_t_su = std::numeric_limits<double>::quiet_NaN();

    double v_wind = log((m_h_tower + m_h_rec / 2) / 0.003) / log(10.0 / 0.003) * v_wind_10;

    eta_therm = m_dot_salt_tot = T_salt_hot = std::numeric_limits<double>::quiet_NaN();

    c_p_coolant = rho_coolant = f = u_coolant = q_conv_sum = q_rad_sum = q_dot_inc_sum = q_dot_piping_loss = q_dot_inc_min_panel = std::numeric_limits<double>::quiet_NaN();
    T_salt_hot_rec = std::numeric_limits<double>::quiet_NaN();

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
        if (m_night_recirc == 1)
        {
            flux_sum = 0.0;
        }
        else
        {
            m_mode = C_csp_collector_receiver::OFF;
            rec_is_off = true;
        }
    }

    T_coolant_prop = (m_T_salt_hot_target + T_salt_cold_in) / 2.0;		//[K] The temperature at which the coolant properties are evaluated. Validated as constant (mjw)
    c_p_coolant = field_htfProps.Cp(T_coolant_prop) * 1000.0;						//[J/kg-K] Specific heat of the coolant

    //if (field_eff < m_eta_field_iter_prev && m_od_control < 1.0)
    //{	// In a prior call-iteration this timestep, the component control was set < 1.0
    //    //     indicating that receiver requested defocus due to mass flow rate constraint
    //    // This call, the plant requests the field to defocus. Under the new plant defocus
    //    //     the corresponding required *component* defocus decreases, because less flux on receiver
    //    // So this line helps "correctly" allocate defocus from the component to the controller
    //    // But, this likely makes the defocus iteration trickier because the iterator
    //    //    won't see a reponse in output mass flow or heat until m_od_control is back to 1.0
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
    soln.p_amb = P_amb;             //[Pa]
    soln.T_sky = T_sky;             //[K]

    soln.flux_sum = flux_sum;       //[W/m2]

    //soln.dni = I_bn;                //[W/m2]
    soln.dni_applied_to_measured = 1.0;     //[-]
    soln.plant_defocus = plant_defocus;     //[-]
    soln.clearsky_to_input_dni = clearsky_to_input_dni;     // clearsky_adj / I_bn;   //[-]
    soln.T_salt_cold_in = T_salt_cold_in;   //[K]	
    soln.od_control = od_control;           //[-] Initial defocus control (may be adjusted during the solution)
    soln.mode = input_operation_mode;
    soln.rec_is_off = rec_is_off;

    
    //if (std::isnan(clearsky_dni) && m_csky_frac > 0.0001)
    if((std::isnan(clearsky_to_input_dni) || clearsky_to_input_dni < 0.9999) && m_csky_frac > 0.0001)
        throw(C_csp_exception("Clearsky DNI to measured is NaN or less than 1 but is required >= 1 in the clearsky receiver model"));

    // Get total incident flux before defocus is applied
    util::matrix_t<double> mt_q_dot_inc_pre_defocus = calculate_flux_profiles(flux_sum, 1.0, 1.0, 1.0, flux_map_input);
    q_dot_inc_pre_defocus = 0.0;
    for (int i = 0; i < m_n_panels; i++) {
        q_dot_inc_pre_defocus += mt_q_dot_inc_pre_defocus.at(i);
    }

    double csky_tol = 0.0001;

    if (rec_is_off)
        soln.q_dot_inc.resize_fill(m_n_panels, 0.0);

    else
    {
        //--- Solve for mass flow at actual and/or clear-sky DNI extremes
        if (m_csky_frac <= 1.0-csky_tol || clearsky_to_input_dni < 1.0001) // Solve for mass flow at actual DNI?
        {
            soln_actual = soln;  // Sets initial solution properties (inlet T, initial defocus control, etc.)
            //soln_actual.dni = I_bn;
            soln_actual.dni_applied_to_measured = 1.0;

            if (use_previous_solution(soln_actual, m_mflow_soln_prev))  // Same conditions were solved in the previous call to this method
                soln_actual = m_mflow_soln_prev;
            else
                solve_for_mass_flow_and_defocus(soln_actual, m_m_dot_htf_max, flux_map_input);

            m_mflow_soln_prev = soln_actual;
        }

        if (m_csky_frac >= csky_tol) // Solve for mass flow at clear-sky DNI?
        {
            if (clearsky_to_input_dni < 1.0001) 
                soln_clearsky = soln_actual;
            else
            {
                soln_clearsky = soln;
                //soln_clearsky.dni = clearsky_adj;
                soln_clearsky.dni_applied_to_measured = soln.clearsky_to_input_dni;     // clearsky_adj / soln.dni;    //[-]

                if (use_previous_solution(soln_clearsky, m_mflow_soln_csky_prev))  // Same conditions were solved in the previous call to this method
                    soln_clearsky = m_mflow_soln_csky_prev;
                else
                    solve_for_mass_flow_and_defocus(soln_clearsky, m_m_dot_htf_max, flux_map_input);

                m_mflow_soln_csky_prev = soln_clearsky;
            }
        }

        //--- Set mass flow and calculate final solution
        if ( clearsky_to_input_dni < 1.0001 || m_csky_frac < csky_tol)  // Flow control based on actual DNI
            soln = soln_actual;

        else if (soln_clearsky.rec_is_off)    // Receiver can't operate at this time point 
        {
            soln.rec_is_off = true;
            soln.q_dot_inc = soln_clearsky.q_dot_inc;
        }

        else if (m_csky_frac > 1-csky_tol)   // Flow control based only on clear-sky DNI
        {
            soln.m_dot_salt = soln_clearsky.m_dot_salt;
            soln.rec_is_off = soln_clearsky.rec_is_off;
            soln.od_control = soln_clearsky.od_control;
            soln.q_dot_inc = calculate_flux_profiles(flux_sum, 1.0, plant_defocus, soln_clearsky.od_control, flux_map_input);  // Absorbed flux profiles at actual DNI and clear-sky defocus
            calculate_steady_state_soln(soln, 0.00025, m_use_constant_piping_loss);  // Solve energy balances at clearsky mass flow rate and actual DNI conditions
        }

        else  // Receiver can operate and flow control based on a weighted average of clear-sky and actual DNI
        {

            if (soln_actual.rec_is_off)  // Receiver is off in actual DNI solution -> Set mass flow to the minimum value
            {
                soln_actual.m_dot_salt = m_f_rec_min * m_m_dot_htf_max;
                soln_actual.od_control = 1.0;
            }

            soln.rec_is_off = false;
            soln.m_dot_salt = (1.0 - m_csky_frac) * soln_actual.m_dot_salt + m_csky_frac * soln_clearsky.m_dot_salt;  // weighted average of clear-sky and actual DNI

            if (soln_clearsky.od_control >= 0.9999)  // No defocus in either clear-sky or actual DNI solutions
            {
                soln.od_control = soln_clearsky.od_control;
                soln.q_dot_inc = soln_actual.q_dot_inc;
                calculate_steady_state_soln(soln, 0.00025, m_use_constant_piping_loss); // Solve energy balances at this mass flow rate and actual DNI conditions
            }
            else
            {
                soln.od_control = (1.0 - m_csky_frac) * soln_actual.od_control + m_csky_frac * soln_clearsky.od_control;
                solve_for_defocus_given_flow(soln, flux_map_input);     // Solve for defocus to achieve as close as possible to target outlet T with this mass flow and actual DNI conditions
            }

        }
    }

    // Check if receiver can operate at existing DNI conditions
    rec_is_off = soln.rec_is_off;

    // Calculate total absorbed solar energy and minimum absorbed per panel (if not already calculated during steady state solution)
    q_dot_inc_sum = soln.Q_inc_sum;
    q_dot_inc_min_panel = soln.Q_inc_min;
    if (soln.Q_inc_sum != soln.Q_inc_sum)
    {
        q_dot_inc_sum = 0.0;
        q_dot_inc_min_panel = soln.q_dot_inc.at(0);
        for (int i = 0; i < m_n_panels; i++)
        {
            q_dot_inc_sum += soln.q_dot_inc.at(i);
            q_dot_inc_min_panel = fmin(q_dot_inc_min_panel, soln.q_dot_inc.at(i));
        }
    }


    // Calculate moving average power (if needed) for shutdown decisions
    m_moving_avg_power = 0.0;
    if (m_shutdown_method == 1)
    {
        double sum_time = step;
        double sum_power = step * q_dot_inc_sum;
        if (step < m_moving_avg_horizon)
        {
            for (size_t j = 0; j < m_recent_steps.length(); j++)
            {
                if (m_recent_steps.at(j) == 0.0)
                    break;

                double f = 1.0;
                if (sum_time + m_recent_steps.at(j) - m_moving_avg_horizon > 0)
                    f = (m_moving_avg_horizon - sum_time) / m_recent_steps.at(j);
                sum_time += f*m_recent_steps.at(j);
                sum_power += f*m_recent_steps.at(j) *m_recent_power.at(j);
                if (sum_time >= m_moving_avg_horizon)
                    break;
            }
        }
        m_moving_avg_power = sum_power / sum_time;
    }

    // Check for conditions where receiver will be allowed to continue operating despite current incident power below the cutoff, and solve again with different mass flow
    if ( plant_defocus > 0.0 && (m_mode_prev == C_csp_collector_receiver::ON || m_mode_prev == C_csp_collector_receiver::STARTUP) && (rec_is_off || q_dot_inc_sum < m_q_dot_inc_min))
    {
        if ((m_shutdown_method == 1 && m_moving_avg_power >= m_q_dot_inc_min) || (m_shutdown_method == 2 && m_low_power_counter_prev + step <= m_shutdown_time_lag))  // Continued receiver operation is allowed
        {
            // Set new mass flow
            if (m_low_power_flow_method == 0)        // Mass flow from minimum turndown
                soln.m_dot_salt = (m_m_dot_htf_des * m_f_rec_min) / m_n_lines;  
            else if (m_low_power_flow_method == 1)   // Mass flow from previous solution above incident power cutoff
                soln.m_dot_salt = m_m_dot_htf_prev / m_n_lines;
            else if (m_low_power_flow_method == 2)   // Mass flow from clear-sky DNI
            {
                if (soln_clearsky.m_dot_salt == soln_clearsky.m_dot_salt)  // Mass flow with clearsky DNI has already been solved
                    soln.m_dot_salt = soln_clearsky.m_dot_salt;
                else
                {
                    s_steady_state_soln soln_clearsky2;
                    soln_clearsky2.T_amb = T_amb;
                    soln_clearsky2.v_wind_10 = v_wind_10;
                    soln_clearsky2.p_amb = P_amb;
                    soln_clearsky2.T_sky = T_sky;
                    soln_clearsky2.T_salt_cold_in = T_salt_cold_in;
                    soln_clearsky2.plant_defocus = plant_defocus;
                    soln_clearsky2.od_control = od_control;
                    soln_clearsky2.rec_is_off = false;
                    soln_clearsky2.mode = input_operation_mode;
                    soln_clearsky2.flux_sum = 2.0;      // Actual value doesn't matter, only need a value > 1.0 to force it to solve with no DNI
                    soln_clearsky2.dni_applied_to_measured = clearsky_to_nominal_clearsky;  // Need to use this ratio with and scaled clear-sky flux profile below since actual DNI might be zero
                    solve_for_mass_flow_and_defocus(soln_clearsky2, m_m_dot_htf_max, flux_map_input_clearsky);
                    soln.m_dot_salt = soln_clearsky2.m_dot_salt;
                }
            }

            // Solve for temperature profiles with new mass flow
            if (soln.m_dot_salt == soln.m_dot_salt) 
            {
                soln.mode = input_operation_mode;
                soln.rec_is_off = false;
                soln.od_control = 1.0;
                calculate_steady_state_soln(soln, 0.00025, m_use_constant_piping_loss);
                rec_is_off = soln.rec_is_off;
            }
        }
    }
    

    // Set variables for use in the rest of the solution
    m_mode = soln.mode;
    od_control = soln.od_control;

    m_dot_salt_tot = soln.m_dot_salt_tot;
    T_salt_hot = soln.T_salt_hot;
    T_salt_hot_rec = soln.T_salt_hot_rec;
    eta_therm = soln.eta_therm;

    u_coolant = soln.u_salt;
    f = soln.f;
    T_coolant_prop = (T_salt_hot + T_salt_cold_in) / 2.0;
    c_p_coolant = field_htfProps.Cp(T_coolant_prop) * 1000.0;
    rho_coolant = field_htfProps.dens(T_coolant_prop, 1.0);

    q_conv_sum = soln.Q_conv_sum;
    q_rad_sum = soln.Q_rad_sum;
    q_dot_piping_loss = soln.Q_dot_piping_loss;


    m_T_s = soln.T_s;
    m_T_panel_in = soln.T_panel_in;
    m_T_panel_out = soln.T_panel_out;
    m_T_panel_ave = soln.T_panel_ave;

    m_q_dot_conv = soln.q_dot_conv;
    m_q_dot_rad = soln.q_dot_rad;
    m_q_dot_loss = soln.q_dot_conv + soln.q_dot_rad;
    m_q_dot_abs = soln.q_dot_abs;
    m_q_dot_inc = soln.q_dot_inc;

    q_thermal_steadystate = soln.Q_thermal;
    q_thermal_csky = 0.0;
    if (m_csky_frac > 0.0001)
        q_thermal_csky = soln_clearsky.Q_thermal;  // Steady state thermal power with clearsky DNI

}

void C_mspt_receiver_222::call(const C_csp_weatherreader::S_outputs& weather,
    const C_csp_solver_htf_1state& htf_state_in,
    const C_pt_receiver::S_inputs& inputs,
    const C_csp_solver_sim_info& sim_info)
{
    double step = sim_info.ms_ts.m_step;	//[s]
    double time = sim_info.ms_ts.m_time;	//[s]

    double plant_defocus = inputs.m_plant_defocus;          //[-]
    const util::matrix_t<double>* flux_map_input = inputs.m_flux_map_input;
    const util::matrix_t<double>* flux_map_input_clearsky = inputs.m_flux_map_input_clearsky;
    C_csp_collector_receiver::E_csp_cr_modes input_operation_mode = inputs.m_input_operation_mode;
    double clearsky_dni = inputs.m_clearsky_dni;

    double T_salt_cold_in = htf_state_in.m_temp + 273.15;	//[K] Cold salt inlet temp, convert from C

    double P_amb = weather.m_pres * 100.0;	//[Pa] Ambient pressure, convert from mbar
    double T_dp = weather.m_tdew + 273.15;	//[K] Dewpoint temperature, convert from C
    double T_amb = weather.m_tdry + 273.15;	//[K] Dry bulb temperature, convert from C
    double I_bn = weather.m_beam;           //[W/m2]
    double v_wind_10 = weather.m_wspd;      //[m/s]

    double hour = time / 3600.0;			//[hr] Hour of the year
    double T_sky = CSP::skytemp(T_amb, T_dp, hour);     //[K]

    double clearsky_adj = std::fmax(clearsky_dni, I_bn);
    double clearsky_to_input_dni = clearsky_adj / I_bn;
    double clearsky_to_nom_clearsky = clearsky_adj / clearsky_dni;
    if (I_bn < 1.E-6) {
        clearsky_to_input_dni = 1.0;
    }

    call(step,
        P_amb, T_amb, T_sky,
        clearsky_to_input_dni,
        clearsky_to_nom_clearsky,
        v_wind_10,
        plant_defocus,
        flux_map_input, flux_map_input_clearsky,
        input_operation_mode,
        T_salt_cold_in);
}
 
C_mspt_receiver_222::C_MEQ__calc_OD_des_for_W_dot_pump_rec::C_MEQ__calc_OD_des_for_W_dot_pump_rec(C_mspt_receiver_222* pc_rec)
{
    mpc_rec = pc_rec;
}

int C_mspt_receiver_222::C_MEQ__calc_OD_des_for_W_dot_pump_rec::operator()(double OD /*m*/, double* W_dot_pump_des /*MWe*/)
{
    mpc_rec->m_od_tube = OD;        //[m]

    mpc_rec->init_mspt_common();

    mpc_rec->design_point_steady_state(mpc_rec->m_eta_thermal_des_calc,
        mpc_rec->m_W_dot_rec_pump_des_calc,
        mpc_rec->m_W_dot_pumping_tower_share, mpc_rec->m_W_dot_pumping_rec_share,
        mpc_rec->m_rec_pump_coef, mpc_rec->m_vel_htf_des);

    // Check design pressure drop against some target
    *W_dot_pump_des = mpc_rec->m_W_dot_pumping_rec_share;       //[MWe]

    return 0;
}

C_mspt_receiver_222::C_MEQ__q_dot_des::C_MEQ__q_dot_des(C_mspt_receiver_222* pc_rec)
{
    mpc_rec = pc_rec;

    m_flux_map_input.resize_fill(1, mpc_rec->m_n_panels, std::numeric_limits<double>::quiet_NaN());

    // Member constants
    m_min_to_max_flux_ratio = 0.45; //[-]
    m_step = 3600.0;        //[s]
    m_plant_defocus = 1.0;   //[-]
    m_input_operation_mode = C_csp_collector_receiver::STEADY_STATE;
}

int C_mspt_receiver_222::C_MEQ__q_dot_des::operator()(double flux_max /*kW/m2*/, double *q_dot_des /*MWt*/)
{
    double min_panel_flux = m_min_to_max_flux_ratio * flux_max;
    double max_less_min = flux_max - min_panel_flux;

    for (int i = 0; i < mpc_rec->m_n_panels; i++) {
        m_flux_map_input(0, i) = max_less_min * sind(i / (double)(mpc_rec->m_n_panels - 1) * 180) + min_panel_flux;
    }

    mpc_rec->call(m_step,
        mpc_rec->m_P_amb_des, mpc_rec->m_T_amb_des, mpc_rec->m_T_sky_des,
        1.0, 1.0,
        mpc_rec->m_v_wind_10_des,
        m_plant_defocus,
        &m_flux_map_input, &m_flux_map_input, m_input_operation_mode,
        mpc_rec->m_T_htf_cold_des);

    *q_dot_des = mpc_rec->ms_outputs.m_Q_thermal;   //[MWt]

    return 0;
}

void C_mspt_receiver_222::design_point_steady_state(double& eta_thermal_des_calc /*-*/,
    double& W_dot_rec_pump_des_calc /*MWe*/,
    double& W_dot_rec_pump__tower_only /*MWe*/, double& W_dot_rec_pump__rec_only /*MWe*/,
    double& rec_pump_coef /*MWe/MWt*/, double& vel_htf_des /*m/s*/)
{
    C_MEQ__q_dot_des c_qot_des_eq(this);
    C_monotonic_eq_solver c_q_dot_des_solver(c_qot_des_eq);

    c_q_dot_des_solver.settings(1.E-3, 5, 0.1, 10000, true);
    double tol_solved = std::numeric_limits<double>::quiet_NaN();
    double max_flux_solved = std::numeric_limits<double>::quiet_NaN();
    int iter_solved = -1;
    int q_dot_des_code = 0;
    try{
        q_dot_des_code = c_q_dot_des_solver.solve(775.0, 700.0, m_q_rec_des*1.E-6, max_flux_solved, tol_solved, iter_solved);
    }
    catch (C_csp_exception) {

        eta_thermal_des_calc = std::numeric_limits<double>::quiet_NaN();
        W_dot_rec_pump_des_calc = std::numeric_limits<double>::quiet_NaN();
        rec_pump_coef = std::numeric_limits<double>::quiet_NaN();

        return;
    }

    eta_thermal_des_calc = ms_outputs.m_eta_therm;       //[-]
    W_dot_rec_pump_des_calc = ms_outputs.m_W_dot_pump;   //[MWe]
    W_dot_rec_pump__tower_only = W_dot_rec_pump_des_calc * ms_outputs.m_ratio_dP_tower_to_rec;  //[MWe]
    W_dot_rec_pump__rec_only = W_dot_rec_pump_des_calc - W_dot_rec_pump__tower_only;               //[MWe]

    // Should q term include piping losses?
    rec_pump_coef = W_dot_rec_pump_des_calc / ms_outputs.m_Q_thermal;    //[MWe/MWt]

    vel_htf_des = ms_outputs.m_vel_htf;       //[m/s]

    return;
}

void C_mspt_receiver_222::call(double step /*s*/,
    double P_amb /*Pa*/, double T_amb /*K*/, double T_sky /*K*/,
    double clearsky_to_input_dni /*-*/,
    double clearsky_to_nominal_clearsky /*-*/,
    double v_wind_10 /*m/s*/,
    double plant_defocus /*-*/,
    const util::matrix_t<double>* flux_map_input,
    const util::matrix_t<double>* flux_map_input_clearsky,
    C_csp_collector_receiver::E_csp_cr_modes input_operation_mode,
    double T_salt_cold_in /*K*/)
{
	// Increase call-per-timestep counter
	// Converge() sets it to -1, so on first call this line will adjust it = 0
	m_ncall++;

    // Variables set in common call
    bool rec_is_off = false;

    double eta_therm, m_dot_salt_tot, T_salt_hot, T_coolant_prop, T_salt_hot_rec, c_p_coolant, u_coolant, rho_coolant, f;
    eta_therm = m_dot_salt_tot = T_salt_hot = T_coolant_prop = T_salt_hot_rec = c_p_coolant = u_coolant = rho_coolant = f = std::numeric_limits<double>::quiet_NaN();
    double q_dot_inc_pre_defocus, q_dot_inc_sum, q_conv_sum, q_rad_sum, q_dot_piping_loss, q_dot_inc_min_panel, q_thermal_csky, q_thermal_steadystate;
    q_dot_inc_pre_defocus = q_dot_inc_sum = q_conv_sum = q_rad_sum = q_dot_piping_loss = q_dot_inc_min_panel = q_thermal_csky = q_thermal_steadystate = std::numeric_limits<double>::quiet_NaN();

    double od_control = std::numeric_limits<double>::quiet_NaN();
    s_steady_state_soln soln;

    call_common(P_amb, T_amb,
        clearsky_to_input_dni,
        clearsky_to_nominal_clearsky,
        v_wind_10, T_sky,
        T_salt_cold_in,
        plant_defocus,
        flux_map_input,
        flux_map_input_clearsky,
        input_operation_mode,
        step,
        // outputs
        rec_is_off,
        eta_therm /*-*/, m_dot_salt_tot /*kg/s*/,
        T_salt_hot /*K*/,
        T_coolant_prop /*K*/, T_salt_hot_rec /*K*/,
        c_p_coolant /*J/kg-K*/, u_coolant /*m/s*/,
        rho_coolant /*kg/m3*/, f /*-*/,
        q_dot_inc_pre_defocus /*Wt*/,
        q_dot_inc_sum /*Wt*/, q_conv_sum /*Wt*/,
        q_rad_sum /*Wt*/, q_dot_piping_loss /*Wt*/,
        q_dot_inc_min_panel /*Wt*/,
        q_thermal_csky /*Wt*/, q_thermal_steadystate /*Wt*/,
        od_control /*-*/,
        soln);

	double DELTAP, Pres_D, ratio_dP_tower_to_rec, W_dot_pump, q_thermal, q_startup;
	DELTAP = Pres_D = ratio_dP_tower_to_rec = W_dot_pump = q_thermal = q_startup = std::numeric_limits<double>::quiet_NaN();

	q_startup = 0.0;

	double time_required_su = step/3600.0;

    m_current_step = step;
    m_q_dot_inc_sum = q_dot_inc_sum;
    m_m_dot_htf = m_dot_salt_tot;

	if( !rec_is_off )
	{
		switch( input_operation_mode )
		{
		case C_csp_collector_receiver::STARTUP:
			{
				double time_require_su_energy = m_E_su_prev / (m_dot_salt_tot*c_p_coolant*(T_salt_hot - T_salt_cold_in));	//[hr]
				double time_require_su_ramping = m_t_su_prev;

				double time_required_max = fmax(time_require_su_energy, time_require_su_ramping);	//[hr]

				double time_step_hrs = step / 3600.0;		//[hr]

				if( time_required_max  > time_step_hrs )		// Can't completely startup receiver in maximum allowable timestep
				{											// Need to advance timestep and try again
					time_required_su = time_step_hrs;		
					m_mode = C_csp_collector_receiver::STARTUP;
					q_startup = m_dot_salt_tot*c_p_coolant*(T_salt_hot - T_salt_cold_in)*step / 3600.0;
				}
				else
				{
					time_required_su = time_required_max;		//[hr]
					m_mode = C_csp_collector_receiver::ON;

					double q_startup_energy_req = m_E_su_prev;	//[W-hr]
					double q_startup_ramping_req = m_dot_salt_tot*c_p_coolant*(T_salt_hot - T_salt_cold_in)*m_t_su_prev;	//[W-hr]
					q_startup = fmax(q_startup_energy_req, q_startup_ramping_req);
				}

				m_E_su = fmax(0.0, m_E_su_prev - m_dot_salt_tot*c_p_coolant*(T_salt_hot - T_salt_cold_in)*step / 3600.0);
				m_t_su = fmax(0.0, m_t_su_prev - step / 3600.0);
			}

			rec_is_off = true;

			break;

		case C_csp_collector_receiver::ON:
			
			m_E_su = m_E_su_prev;
			m_t_su = m_t_su_prev;
			m_mode = C_csp_collector_receiver::ON;
			q_startup = 0.0;

			q_thermal = m_dot_salt_tot*c_p_coolant*(T_salt_hot - T_salt_cold_in);

			break;

		case C_csp_collector_receiver::STEADY_STATE:

			m_mode = C_csp_collector_receiver::STEADY_STATE;

			break;
		
		}	// End switch() on input_operation_mode

		// Pressure drop calculations
        calc_pump_performance(rho_coolant, m_dot_salt_tot, f, Pres_D, W_dot_pump, ratio_dP_tower_to_rec);

		q_thermal = m_dot_salt_tot*c_p_coolant*(T_salt_hot - T_salt_cold_in);


		// After convergence, determine whether the incident power falls below the lower limit and if receiver is allowed to continue operating
        bool allow_low_power_operation = check_min_turndown(q_dot_inc_sum, step, input_operation_mode, rec_is_off);
        if (m_is_below_cutoff && !allow_low_power_operation && input_operation_mode == C_csp_collector_receiver::ON)
        {
            // Include here outputs that are ONLY set to zero if receiver is completely off, and not attempting to start-up
            W_dot_pump = 0.0;
            DELTAP = 0.0; Pres_D = 0.0; u_coolant = 0.0; ratio_dP_tower_to_rec = 0.0;
        }


	}
	else
	{	// If receiver was off BEFORE startup deductions
		m_mode = C_csp_collector_receiver::OFF;

		// Include here outputs that are ONLY set to zero if receiver completely off, and not attempting to start-up
		W_dot_pump = 0.0;
		// Pressure drops
		DELTAP = 0.0; Pres_D = 0.0; u_coolant = 0.0, ratio_dP_tower_to_rec = 0.0;
	}

	if( rec_is_off )
	{
		m_dot_salt_tot = 0.0;
        eta_therm = 0.0;
		q_conv_sum = 0.0;
        q_rad_sum = 0.0;
        m_T_s.fill(0.0);
        q_thermal = 0.0;

        // Set the receiver outlet temperature equal to the inlet design temperature
		T_salt_hot = m_T_htf_cold_des;

        q_dot_inc_pre_defocus = 0.0;
        q_dot_inc_sum = 0.0;
		q_thermal_csky = q_thermal_steadystate = 0.0;
		
		// Reset m_od_control (for reporting)
		od_control = 1.0;		//[-]
	}

	outputs.m_m_dot_salt_tot = m_dot_salt_tot*3600.0;		//[kg/hr] convert from kg/s
	outputs.m_eta_therm = eta_therm;						//[-] RECEIVER thermal efficiency (includes radiation and convective losses. reflection losses are contained in receiver flux model)
	outputs.m_W_dot_pump = W_dot_pump / 1.E6;				//[MW] convert from W
	outputs.m_q_conv_sum = q_conv_sum / 1.E6;				//[MW] convert from W
	outputs.m_q_rad_sum = q_rad_sum / 1.E6;					//[MW] convert from W
	outputs.m_Q_thermal = q_thermal / 1.E6;					//[MW] convert from W
	outputs.m_T_salt_hot = T_salt_hot - 273.15;				//[C] convert from K
	outputs.m_component_defocus = od_control;				//[-]
    outputs.m_q_dot_rec_inc_pre_defocus = q_dot_inc_pre_defocus / 1.E6;    //[MWt]
	outputs.m_q_dot_rec_inc = q_dot_inc_sum / 1.E6;			//[MW] convert from W
	outputs.m_q_startup = q_startup/1.E6;					//[MW-hr] convert from W-hr
	outputs.m_dP_receiver = DELTAP*m_n_panels / m_n_lines / 1.E5;	//[bar] receiver pressure drop, convert from Pa
	outputs.m_dP_total = Pres_D*10.0;						//[bar] total pressure drop, convert from MPa
    outputs.m_ratio_dP_tower_to_rec = ratio_dP_tower_to_rec;    //[-] ratio of total pressure drop that is caused by tower height
	outputs.m_vel_htf = u_coolant;							//[m/s]
	outputs.m_T_salt_cold = T_salt_cold_in - 273.15;		//[C] convert from K
	outputs.m_time_required_su = time_required_su*3600.0;	//[s], convert from hr in code
	if(q_thermal > 0.0)
		outputs.m_q_dot_piping_loss = q_dot_piping_loss/1.E6;	//[MWt]
	else
		outputs.m_q_dot_piping_loss = 0.0;		//[MWt]
    outputs.m_q_heattrace = 0.0;

	outputs.m_Q_thermal_csky_ss = q_thermal_csky / 1.e6; //[MWt]
	outputs.m_Q_thermal_ss = q_thermal_steadystate / 1.e6; //[MWt]

    ms_outputs = outputs;

}

void C_mspt_receiver_222::off(const C_csp_weatherreader::S_outputs &weather,
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
	outputs.m_Q_thermal = 0.0;			//[MW] convert from W
	outputs.m_T_salt_hot = 0.0;			//[C] convert from K
	outputs.m_component_defocus = 1.0;	//[-]
	outputs.m_q_dot_rec_inc = 0.0;		//[MW] convert from kW
	outputs.m_q_startup = 0.0;			//[MW-hr] convert from W-hr
	outputs.m_dP_receiver = 0.0;			//[bar] receiver pressure drop, convert from Pa
	outputs.m_dP_total = 0.0;			//[bar] total pressure drop, convert from MPa
	outputs.m_vel_htf = 0.0;				//[m/s]
	outputs.m_T_salt_cold = 0.0;			//[C] convert from K
	outputs.m_time_required_su = sim_info.ms_ts.m_step;	//[s], convert from hr in code
	outputs.m_q_dot_piping_loss = 0.0;	//[MWt]
    outputs.m_q_heattrace = 0.0;
	
	outputs.m_Q_thermal_csky_ss = 0.0; //[MWt]
	outputs.m_Q_thermal_ss = 0.0; //[MWt]

    ms_outputs = outputs;

    m_current_step = sim_info.ms_ts.m_step;	//[s]
    m_q_dot_inc_sum = 0.0;
	
	return;
}

void C_mspt_receiver_222::overwrite_startup_requirements_to_on()
{
    m_mode_prev = C_csp_collector_receiver::ON;
    m_E_su_prev = 0.0;
    m_t_su_prev = 0.0;
}

void C_mspt_receiver_222::converged()
{
	// Check HTF props?
	//!MJW 9.8.2010 :: Call the property range check subroutine with the inlet and outlet HTF temps to make sure they're in the valid range
	//call check_htf(Coolant,T_salt_hot)
	//call check_htf(Coolant,T_salt_cold)

	if( m_mode == C_csp_collector_receiver::STEADY_STATE )
	{
		throw(C_csp_exception("Receiver should only be run at STEADY STATE mode for estimating output. It must be run at a different mode before exiting a timestep",
			"MSPT receiver converged method"));
	}

	if( m_mode == C_csp_collector_receiver::OFF )
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

    if (m_shutdown_method == 1 && m_moving_avg_horizon > 0.0)
    {
        // Update arrays of recent time points
        size_t ilast = 0;  // Index position of last time point in array that needs to be retained
        double sum_time = m_current_step;
        for (size_t j = 0; j < m_recent_steps.length() - 1; j++)
        {
            sum_time += m_recent_steps.at(j);
            if (sum_time >= m_moving_avg_horizon)
            {
                ilast = j;
                break;
            }
            else if (j > 0 && m_recent_steps.at(j) == 0.0)
            {
                ilast = j - 1;
                break;
            }
        }

        for (size_t j = ilast + 1; j > 0; j--)
        {
            m_recent_steps.at(j) = m_recent_steps.at(j - 1);
            m_recent_power.at(j) = m_recent_power.at(j - 1);
        }
        m_recent_steps.at(0) = m_current_step;
        m_recent_power.at(0) = m_q_dot_inc_sum;
    }
    
    else if (m_shutdown_method == 2)
    {
        if ((m_mode == C_csp_collector_receiver::STARTUP || m_mode == C_csp_collector_receiver::ON) && !m_is_below_cutoff && m_reset_counter >= m_reset_lag)
        {
            m_low_power_counter = 0.0;     // Reset shutdown counter if receiver is able to operate and isn't below the cutoff
        }
        if (m_mode == C_csp_collector_receiver::OFF || m_mode == C_csp_collector_receiver::OFF_NO_SU_REQ || m_is_below_cutoff)
        {
            m_reset_counter = 0.0;
        }
        m_low_power_counter_prev = m_low_power_counter;
        m_reset_counter_prev = m_reset_counter;
    }

    if ((m_mode == C_csp_collector_receiver::STARTUP || m_mode == C_csp_collector_receiver::ON) && !m_is_below_cutoff)
        m_m_dot_htf_prev = m_m_dot_htf;


 

}


bool C_mspt_receiver_222::use_previous_solution(const s_steady_state_soln& soln, const s_steady_state_soln& soln_prev)
{
	// Are these conditions identical to those used in the last solution?
	if (!soln_prev.rec_is_off && 
		//soln.dni == soln_prev.dni &&
        soln.dni_applied_to_measured == soln_prev.dni_applied_to_measured &&
		soln.T_salt_cold_in == soln_prev.T_salt_cold_in &&
        soln.plant_defocus == soln_prev.plant_defocus &&
		soln.od_control == soln_prev.od_control &&
		soln.T_amb == soln_prev.T_amb && 
		soln.v_wind_10 == soln_prev.v_wind_10 &&
		soln.p_amb == soln_prev.p_amb &&
        soln.T_sky == soln_prev.T_sky &&
        soln.flux_sum == soln_prev.flux_sum )
	{
		return true;
	}
	else
		return false;
}


// Calculate flux profiles (interpolated to receiver panels at specified DNI)
util::matrix_t<double> C_mspt_receiver_222::calculate_flux_profiles(double flux_sum /*W/m2*/, double flux_scale /*-*/, double plant_defocus /*-*/,
                                        double od_control, const util::matrix_t<double> *flux_map_input)
{
	util::matrix_t<double> q_dot_inc, flux;
	q_dot_inc.resize_fill(m_n_panels, 0.0);

    double total_defocus = plant_defocus * od_control;

	// Set flux at flux map resolution
	int n_flux_y = (int)flux_map_input->nrows();
	int n_flux_x = (int)flux_map_input->ncols();
	flux.resize_fill(n_flux_x, 0.0);

	if (flux_sum > 1.0)
	{
		for (int j = 0; j<n_flux_x; j++)
		{
			flux.at(j) = 0.;
			for (int i = 0; i<n_flux_y; i++)
			{
                flux.at(j) += (*flux_map_input)(i, j) * total_defocus * flux_scale; //[kW/m^2];
			}
		}
	}
	else
	{
		flux.fill(0.0);
	}

	double n_flux_x_d = (double)n_flux_x;
	double n_panels_d = (double)m_n_panels;

	// Translate flux to panels
	if (m_n_panels >= n_flux_x)
	{
		// Translate to the number of panels, so each panel has its own linearly interpolated flux value
		for (int i = 0; i < m_n_panels; i++)
		{
			double ppos = (n_flux_x_d / n_panels_d * i + n_flux_x_d * 0.5 / n_panels_d);
			int flo = (int)floor(ppos);
			int ceiling = (int)ceil(ppos);
			double ind = (int)((ppos - flo) / fmax((double)ceiling - (double)flo, 1.e-6));
			if (ceiling > n_flux_x - 1) ceiling = 0;

			double psp_field = (ind*(flux.at(ceiling) - flux.at(flo)) + flux.at(flo));		//[kW/m^2] Average area-specific power for each node			
			q_dot_inc.at(i) = m_A_node * psp_field*1000;									//[W] The power incident on each node

		}
	}
	else
	{
		/*
		The number of panels is always even, therefore the receiver panels are symmetric about the N-S plane.

		The number of flux points may be even or odd. The distribution is assumed to be symmetric
		about North, therefore:
		(a) A distribution with an odd number of points includes a center point (n_flux_x - 1)/2+1
		whose normal faces exactly north
		(b) A distribution with an even number of points includes 2 points n_flux_x/2, n_flux_x/2+1
		which straddle the North vector.
		In either scenario, two points straddle the South vector and no scenario allows a point to fall
		directly on the South vector. Hence, the first and last flux points fall completely on the first
		and last panel, respectively.
		*/

		double leftovers = 0.;
		int index_start = 0; int index_stop = 0;
		double q_flux_sum = 0.0;

		double panel_step = n_flux_x_d / n_panels_d;   //how many flux points are stepped over by each panel?

		for (size_t i = 0; i<m_n_panels; i++)
		{
			double panel_pos = panel_step * (i + 1);   //Where does the current panel end in the flux array?

			index_start = (int)floor(panel_step*i);
			index_stop = (int)floor(panel_pos);

			q_flux_sum = 0.;

			for (int j = index_start; j<index_stop + 1; j++)
			{
				if (j == n_flux_x)
				{
					if (leftovers > 0.)
					{
						csp_messages.add_message(C_csp_messages::WARNING, "An error occurred during interpolation of the receiver flux map. The results may be inaccurate! Contact SAM support to resolve this issue.");
					}

					break;
				}
				if (j == 0)
				{
					q_flux_sum = flux.at(j);
					leftovers = 0.;
				}
				else if (j == index_start)
				{
					q_flux_sum += leftovers;
					leftovers = 0.;
				}
				else if (j == index_stop)
				{
					double stop_mult = (panel_pos - floor(panel_pos));
					q_flux_sum += stop_mult * flux.at(j);
					leftovers = (1 - stop_mult)*flux.at(j);
				}
				else
				{
					q_flux_sum += flux[j];
				}
			}
			q_dot_inc.at(i) = q_flux_sum * m_A_node / n_flux_x_d * n_panels_d*1000;
		}

	}

	return q_dot_inc;
}

// Calculate steady state temperature and heat loss profiles for a given mass flow and incident flux
void C_mspt_receiver_222::calculate_steady_state_soln(s_steady_state_soln &soln, double tol, bool use_constant_piping_loss, int max_iter)
{
	double P_amb = soln.p_amb;	
	double T_amb = soln.T_amb;	
	double v_wind_10 = soln.v_wind_10;
	double T_sky = soln.T_sky;
	double v_wind = log((m_h_tower + m_h_rec / 2) / 0.003) / log(10.0 / 0.003)*v_wind_10;

	util::matrix_t<double> T_s_guess(m_n_panels);
	util::matrix_t<double> T_panel_out_guess(m_n_panels);
	util::matrix_t<double> T_panel_in_guess(m_n_panels);
	util::matrix_t<double> T_film(m_n_panels);

	bool soln_exists = (soln.T_salt_hot == soln.T_salt_hot);

	soln.m_dot_salt_tot = soln.m_dot_salt * m_n_lines;

	// Set initial guess
	double T_salt_hot_guess;
	if (soln_exists)    // Use existing solution as the inital guess
	{
		T_salt_hot_guess = soln.T_salt_hot;    // Initial guess for outlet T
		T_s_guess = soln.T_s;
		T_panel_out_guess = soln.T_panel_out;
		T_panel_in_guess = soln.T_panel_in;
	}
	else // Initialize solution from scratch
	{
		T_salt_hot_guess = m_T_salt_hot_target;    // Initial guess for outlet T

		soln.T_s.resize(m_n_panels);
		soln.T_panel_out.resize(m_n_panels);
		soln.T_panel_in.resize(m_n_panels);
		soln.q_dot_conv.resize(m_n_panels);
		soln.q_dot_rad.resize(m_n_panels);
		soln.q_dot_loss.resize(m_n_panels);
		soln.q_dot_abs.resize(m_n_panels);
		soln.T_panel_ave.resize(m_n_panels);

		if (m_night_recirc == 1)
		{
			T_s_guess.fill(m_T_salt_hot_target);										//[K] Guess the temperature for the surface nodes
			T_panel_out_guess.fill((m_T_salt_hot_target + soln.T_salt_cold_in) / 2.0);	//[K] Guess values for the fluid temp coming out of the control volume
			T_panel_in_guess.fill((m_T_salt_hot_target + soln.T_salt_cold_in) / 2.0);	//[K] Guess values for the fluid temp coming into the control volume
		}
		else
		{
			T_s_guess.fill(m_T_salt_hot_target);			//[K] Guess the temperature for the surface nodes
			T_panel_out_guess.fill(soln.T_salt_cold_in);	//[K] Guess values for the fluid temp coming out of the control volume
			T_panel_in_guess.fill(soln.T_salt_cold_in);		//[K] Guess values for the fluid temp coming into the control volume
		}
	}


	// Temperature solution iterations
	for (int q = 0; q < max_iter; q++)
	{
		double T_coolant_prop;
		if (soln.T_salt_props == soln.T_salt_props)   // Temperature for property evaluation exists (calling from within loop over salt mass flow rate)
			T_coolant_prop = soln.T_salt_props;
		else    
			T_coolant_prop = (T_salt_hot_guess + soln.T_salt_cold_in) / 2.0;
		double c_p_coolant = field_htfProps.Cp(T_coolant_prop)*1000.;	

		for (int i = 0; i < m_n_panels; i++)
		{
			soln.T_s.at(i) = T_s_guess.at(i);
			soln.T_panel_out.at(i) = T_panel_out_guess.at(i);
			soln.T_panel_in.at(i) = T_panel_in_guess.at(i);
			soln.T_panel_ave.at(i) = (soln.T_panel_in.at(i) + soln.T_panel_out.at(i)) / 2.0;		//[K] The average coolant temperature in each control volume
			T_film.at(i) = (soln.T_s.at(i) + T_amb) / 2.0;											//[K] Film temperature
		}

		// Calculate the average surface temperature
		double T_s_sum = 0.0;
		for (int i = 0; i < m_n_panels; i++)
			T_s_sum += soln.T_s.at(i);
		double T_film_ave = (T_amb + T_salt_hot_guess) / 2.0;


		// Convective coefficient for external forced convection using Siebers & Kraabel
		double k_film = ambient_air.cond(T_film_ave);				//[W/m-K] The conductivity of the ambient air
		double mu_film = ambient_air.visc(T_film_ave);				//[kg/m-s] Dynamic viscosity of the ambient air
		double rho_film = ambient_air.dens(T_film_ave, P_amb);		//[kg/m^3] Density of the ambient air
		double c_p_film = ambient_air.Cp(T_film_ave);				//[kJ/kg-K] Specific heat of the ambient air
		double Re_for = rho_film * v_wind*m_d_rec / mu_film;		//[-] Reynolds number
		double ksD = (m_od_tube / 2.0) / m_d_rec;					//[-] The effective roughness of the cylinder [Siebers, Kraabel 1984]
		double Nusselt_for = CSP::Nusselt_FC(ksD, Re_for);			//[-] S&K
		double h_for = Nusselt_for * k_film / m_d_rec * m_hl_ffact;	//[W/m^2-K] Forced convection heat transfer coefficient

		// Convection coefficient for external natural convection using Siebers & Kraabel
		// Note: This relationship applies when the surrounding properties are evaluated at ambient conditions [S&K]
		double beta = 1.0 / T_amb;													//[1/K] Volumetric expansion coefficient
		double nu_amb = ambient_air.visc(T_amb) / ambient_air.dens(T_amb, P_amb);	//[m^2/s] Kinematic viscosity		

		for (size_t j = 0; j < m_n_lines; j++)   
		{
			for (size_t  i = 0; i < m_n_panels / m_n_lines; i++)
			{
				int i_fp = m_flow_pattern.at(j, i);

				// Natural convection
				double Gr_nat = fmax(0.0, CSP::grav*beta*(soln.T_s.at(i_fp) - T_amb)*pow(m_h_rec, 3) / pow(nu_amb, 2));	//[-] Grashof Number at ambient conditions
				double Nusselt_nat = 0.098*pow(Gr_nat, (1.0 / 3.0))*pow(soln.T_s.at(i_fp) / T_amb, -0.14);				//[-] Nusselt number
				double h_nat = Nusselt_nat * ambient_air.cond(T_amb) / m_h_rec * m_hl_ffact;							//[W/m^-K] Natural convection coefficient

				// Mixed convection
				double h_mixed = pow((pow(h_for, m_m_mixed) + pow(h_nat, m_m_mixed)), 1.0 / m_m_mixed)*4.0;		//(4.0) is a correction factor to match convection losses at Solar II (correspondance with G. Kolb, SNL)
				soln.q_dot_conv.at(i_fp) = h_mixed * m_A_node*(soln.T_s.at(i_fp) - T_film.at(i_fp));			//[W] Convection losses per node

				// Radiation from the receiver - Calculate the radiation node by node
				soln.q_dot_rad.at(i_fp) = 0.5*CSP::sigma*m_epsilon*m_A_node*(2.0*pow(soln.T_s.at(i_fp), 4) - pow(T_amb, 4) - pow(T_sky, 4))*m_hl_ffact;	//[W] Total radiation losses per node
				soln.q_dot_loss.at(i_fp) = soln.q_dot_rad.at(i_fp) + soln.q_dot_conv.at(i_fp);			//[W] Total overall losses per node
				soln.q_dot_abs.at(i_fp) = soln.q_dot_inc.at(i_fp) - soln.q_dot_loss.at(i_fp);			//[W] Absorbed flux at each node

				// Calculate the temperature drop across the receiver tube wall
				double T_wall = (soln.T_s.at(i_fp) + soln.T_panel_ave.at(i_fp)) / 2.0;				//[K] The temperature at which the conductivity of the wall is evaluated
				double k_tube = tube_material.cond(T_wall);											//[W/m-K] The conductivity of the wall

                double R_tube_wall = m_th_tube / (k_tube*m_h_rec*m_d_rec*pow(CSP::pi, 2) / 2.0 / (double)m_n_panels);	//[K/W] The thermal resistance of the wall

                // switching to using cylindrical resistance term. Remove '2' from before CSP::pi because only using front half of tube
                // in default case, makes 1.0002 difference in annual energy
                //double R_tube_wall = log(m_od_tube/m_id_tube)/(CSP::pi*m_h_rec*k_tube)/(double)m_n_t;

				// Calculations for the inside of the tube						
				double mu_coolant = field_htfProps.visc(T_coolant_prop);							//[kg/m-s] Absolute viscosity of the coolant
				double k_coolant = field_htfProps.cond(T_coolant_prop);								//[W/m-K] Conductivity of the coolant
				double rho_coolant = field_htfProps.dens(T_coolant_prop, 1.0);						//[kg/m^3] Density of the coolant
				double u_coolant = soln.m_dot_salt / (m_n_t*rho_coolant*pow((m_id_tube / 2.0), 2)*CSP::pi);	//[m/s] Average velocity of the coolant through the receiver tubes
				double Re_inner = rho_coolant * u_coolant*m_id_tube / mu_coolant;					//[-] Reynolds number of internal flow
				double Pr_inner = c_p_coolant * mu_coolant / k_coolant;								//[-] Prandtl number of internal flow

				double Nusselt_t, f;
				CSP::PipeFlow(Re_inner, Pr_inner, m_LoverD, m_RelRough, Nusselt_t, f);
				if (Nusselt_t <= 0.0)
				{
					soln.mode = C_csp_collector_receiver::OFF;	
					break;
				}
				double h_inner = Nusselt_t * k_coolant / m_id_tube;								//[W/m^2-K] Convective coefficient between the inner tube wall and the coolant
				double R_conv_inner = 1.0 / (h_inner*CSP::pi*m_id_tube / 2.0*m_h_rec*m_n_t);	//[K/W] Thermal resistance associated with this value

				soln.u_salt = u_coolant;
				soln.f = f;

				// Update panel inlet/outlet temperature guess
				if (i > 0)
				{
					int i_prev = m_flow_pattern.at(j, i - 1);   // Previous panel in flow order
					T_panel_in_guess.at(i_fp) = T_panel_out_guess.at(i_prev);
				}
				else
					T_panel_in_guess.at(i_fp) = soln.T_salt_cold_in;
				

				T_panel_out_guess.at(i_fp) = T_panel_in_guess.at(i_fp) + soln.q_dot_abs.at(i_fp) / (soln.m_dot_salt*c_p_coolant);		//[K] Energy balance for each node		
				double Tavg = (T_panel_out_guess.at(i_fp) + T_panel_in_guess.at(i_fp)) / 2.0;											//[K] Panel average temperature
				T_s_guess.at(i_fp) = Tavg + soln.q_dot_abs.at(i_fp)*(R_conv_inner + R_tube_wall);										//[K] Surface temperature based on the absorbed heat
				if (T_s_guess.at(i_fp) < 1.0)
				{
					soln.mode = C_csp_collector_receiver::OFF;
				}

			}  // End loop over panels per flow path
		}  // End loop over flow paths
		

		if (soln.mode == C_csp_collector_receiver::OFF)
			break;

		// Calculate average receiver outlet temperature
		int klast = m_n_panels / m_n_lines - 1;
		double T_salt_hot_guess_sum = 0.0;
		for (int j = 0; j < m_n_lines; j++)
			T_salt_hot_guess_sum += T_panel_out_guess.at(m_flow_pattern.at(j, klast));		//[K] Update the calculated hot salt outlet temp
		soln.T_salt_hot = T_salt_hot_guess_sum / (double)m_n_lines;


		// Calculate outlet temperature after piping losses
		soln.Q_dot_piping_loss = 0.0;
		if (m_Q_dot_piping_loss > 0.0)
		{
			double m_dot_salt_tot_temp = soln.m_dot_salt * m_n_lines;		//[kg/s]

            if (use_constant_piping_loss)   // Calculate piping loss from constant loss per m (m_Q_dot_piping_loss)
                soln.Q_dot_piping_loss = m_Q_dot_piping_loss;
            else
            {
                double riser_loss = 2.0 * CSP::pi * (soln.T_salt_cold_in - T_amb) / m_Rtot_riser; //[W/m]
                double downc_loss = 2.0 * CSP::pi * (soln.T_salt_hot - T_amb) / m_Rtot_downc; //[W/m]
                soln.Q_dot_piping_loss = 0.5 * (riser_loss + downc_loss) * (m_h_tower * m_pipe_length_mult + m_pipe_length_add); // Total piping thermal loss [W]
            }
            double delta_T_piping = soln.Q_dot_piping_loss / (m_dot_salt_tot_temp * c_p_coolant);	//[K]
            soln.T_salt_hot_rec = soln.T_salt_hot;
            soln.T_salt_hot -= delta_T_piping;	//[K]
		}
		
		
		// Check convergence
		double err = (soln.T_salt_hot - T_salt_hot_guess) / T_salt_hot_guess;
		T_salt_hot_guess = soln.T_salt_hot;
		if (std::abs(err) < tol && q>0)
			break;

	} // End iterations

    if (soln.T_salt_hot < soln.T_salt_cold_in)
    {
        if (m_shutdown_method == 0)
            soln.mode = C_csp_collector_receiver::OFF;
        else if (m_shutdown_method == 1 && m_moving_avg_horizon < 1e-6)
            soln.mode = C_csp_collector_receiver::OFF;
        else if (m_shutdown_method == 2 && m_shutdown_time_lag < 1e-6)
            soln.mode = C_csp_collector_receiver::OFF;
    }


	// Save overall energy loss
	soln.Q_inc_sum = 0.0;
	soln.Q_conv_sum = 0.0;
	soln.Q_rad_sum = 0.0;
	soln.Q_abs_sum = 0.0;
	soln.Q_inc_min = soln.q_dot_inc.at(0);
	for (int i = 0; i < m_n_panels; i++)
	{
		soln.Q_inc_sum += soln.q_dot_inc.at(i);
		soln.Q_conv_sum += soln.q_dot_conv.at(i);
		soln.Q_rad_sum += soln.q_dot_rad.at(i);
		soln.Q_abs_sum += soln.q_dot_abs.at(i);
		soln.Q_inc_min = fmin(soln.Q_inc_min, soln.q_dot_inc.at(i));
	}
	soln.Q_thermal = soln.Q_abs_sum - soln.Q_dot_piping_loss;

	if (soln.Q_inc_sum > 0.0)
		soln.eta_therm = soln.Q_abs_sum / soln.Q_inc_sum;
	else
		soln.eta_therm = 0.0;

	soln.rec_is_off = false;
	if (soln.mode == C_csp_collector_receiver::OFF)
		soln.rec_is_off = true;

	// Save final temperature profile solution
	if (!soln.rec_is_off)
	{
		soln.T_s = T_s_guess;
		soln.T_panel_out = T_panel_out_guess;
		soln.T_panel_in = T_panel_in_guess;
		for (int i = 0; i < m_n_panels; i++)
			soln.T_panel_ave.at(i) = (soln.T_panel_in.at(i) + soln.T_panel_out.at(i)) / 2.0;
	}

	return;

}

// Calculate mass flow rate needed to achieve target outlet temperature (m_T_salt_hot_target) given incident flux profiles
void C_mspt_receiver_222::solve_for_mass_flow(s_steady_state_soln &soln)
{

	bool soln_exists = (soln.m_dot_salt == soln.m_dot_salt);

	soln.T_salt_props = (m_T_salt_hot_target + soln.T_salt_cold_in) / 2.0;		//[K] The temperature at which the coolant properties are evaluated. Validated as constant (mjw)
	double c_p_coolant = field_htfProps.Cp(soln.T_salt_props)*1000.0;				//[J/kg-K] Specific heat of the coolant

	double m_dot_salt_guess;
	if (soln_exists)  // Use existing solution as intial guess
	{
		m_dot_salt_guess = soln.m_dot_salt;
	}
	else  // Set inital guess for mass flow solution
	{

		double q_dot_inc_sum = 0.0;
		for (int i = 0; i < m_n_panels; i++)
			q_dot_inc_sum += soln.q_dot_inc.at(i);		//[kW] Total power absorbed by receiver

		double c_guess = field_htfProps.Cp((m_T_salt_hot_target + soln.T_salt_cold_in) / 2.0)*1000.;	//[kJ/kg-K] Estimate the specific heat of the fluid in receiver

		if(soln.flux_sum > 1.E-6) // (soln.dni > 1.E-6)
		{
			double q_guess = 0.85*q_dot_inc_sum;		//[kW] Estimate the thermal power produced by the receiver			
			m_dot_salt_guess = q_guess / (c_guess*(m_T_salt_hot_target - soln.T_salt_cold_in)*m_n_lines);	//[kg/s] Mass flow rate for each flow path			
		}
		else	// The tower recirculates at night (based on earlier conditions)
		{
			// Enter recirculation mode, where inlet/outlet temps switch
			double T_salt_hot = m_T_salt_hot_target;
			m_T_salt_hot_target = soln.T_salt_cold_in;
			soln.T_salt_cold_in = T_salt_hot;
			m_dot_salt_guess = -3500.0 / (c_guess*(m_T_salt_hot_target - soln.T_salt_cold_in) / 2.0);

		}
	}


	// Set soluion tolerance
	double T_salt_hot_guess = 9999.9;		//[K] Initial guess value for error calculation
	double err = -999.9;					//[-] Relative outlet temperature error
	double tol = std::numeric_limits<double>::quiet_NaN();
	if (m_night_recirc == 1)
		tol = 0.0057;
	else
		tol = 0.00025;

	int qq_max = 50;
	int qq = 0;

	bool converged = false;
	while (!converged)
	{
		qq++;

		// if the problem fails to converge after 50 iterations, then the power is likely negligible and the zero set can be returned
		if (qq > qq_max)
		{
			soln.mode = C_csp_collector_receiver::OFF;  // Set the startup mode
			soln.rec_is_off = true;
			break;
		}

		soln.m_dot_salt = m_dot_salt_guess;
		double tolT = tol;
		calculate_steady_state_soln(soln, tolT, m_use_constant_piping_loss, 50);   // Solve steady state thermal model		
		err = (soln.T_salt_hot - m_T_salt_hot_target) / m_T_salt_hot_target;
		
		if (soln.rec_is_off)  // SS solution was unsuccessful or resulted in an infeasible exit temperature -> remove outlet T for solution to start next iteration from the default intial guess
			soln.T_salt_hot = std::numeric_limits<double>::quiet_NaN();

		if (std::abs(err) > tol)
		{
			m_dot_salt_guess = (soln.Q_abs_sum - soln.Q_dot_piping_loss) / (m_n_lines * c_p_coolant * (m_T_salt_hot_target - soln.T_salt_cold_in));			//[kg/s]

			if (m_dot_salt_guess < 1.E-5)
			{
				soln.mode = C_csp_collector_receiver::OFF;
				soln.rec_is_off = true;
				break;
			}
		}
		else if (err > 0.0)  // Solution has converged but outlet T is above target.  CSP solver seems to perform better with slighly under-design temperature than with slighly over-design temperatures. 
			m_dot_salt_guess *= (soln.T_salt_hot - soln.T_salt_cold_in) / ((1.0 - 0.5*tol) * m_T_salt_hot_target - soln.T_salt_cold_in);
		else
			converged = true;

	}

	soln.m_dot_salt_tot = soln.m_dot_salt * m_n_lines;

	return;
}

// Calculate mass flow rate and defocus needed to achieve target outlet temperature given DNI
void C_mspt_receiver_222::solve_for_mass_flow_and_defocus(s_steady_state_soln &soln, double m_dot_htf_max, const util::matrix_t<double> *flux_map_input)
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

		double m_dot_salt_tot = soln.m_dot_salt * m_n_lines;
		double m_dot_tube = soln.m_dot_salt / (double)m_n_t;		//[kg/s] The mass flow through each individual tube

		// Limit the HTF mass flow rate to the maximum, if needed
		rec_is_defocusing = false;
		if ((m_dot_salt_tot > m_dot_htf_max))
		{
			double err_od = (m_dot_salt_tot - m_dot_htf_max) / m_dot_htf_max;
			if (err_od < m_tol_od)
			{
				rec_is_defocusing = false;
			}
			else
			{
				soln.od_control = soln.od_control * pow((m_dot_htf_max / m_dot_salt_tot), 0.8);	//[-] Adjust the over-design defocus control by modifying the current value
				rec_is_defocusing = true;
			}
		}

	}

	return;
}

// Calculate defocus needed to maintain outlet temperature under the target value given DNI and mass flow
void C_mspt_receiver_222::solve_for_defocus_given_flow(s_steady_state_soln &soln, const util::matrix_t<double> *flux_map_input)
{ 
	
	double Tprev, od, odprev, odlow, odhigh;
	double tolT = 0.00025;
	double urf = 0.8;

	Tprev = odprev = odlow = std::numeric_limits<double>::quiet_NaN();
	od = soln.od_control;
	odhigh = 1.0;

	od = odprev * (m_T_salt_hot_target - soln.T_salt_cold_in) / (Tprev - soln.T_salt_cold_in);

	int q = 0;
	while (q<50)
	{
		soln.od_control = od;
		if (odprev != odprev)
			soln.q_dot_inc = calculate_flux_profiles(soln.flux_sum, soln.dni_applied_to_measured, soln.plant_defocus, soln.od_control, flux_map_input);
		else
			soln.q_dot_inc = soln.q_dot_inc * soln.od_control / odprev; // Calculate flux profiles (note flux is directly proportional to defocus control)
		
		calculate_steady_state_soln(soln, tolT, m_use_constant_piping_loss);     // Solve steady state thermal model 

		if (soln.od_control > 0.9999 && soln.T_salt_hot < m_T_salt_hot_target)  // Impossible for solution to achieve temperature target
			break;
		else if ((std::abs(soln.T_salt_hot - m_T_salt_hot_target) / m_T_salt_hot_target) < tolT)
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
				od = odprev * (m_T_salt_hot_target - soln.T_salt_cold_in) / (Tprev - soln.T_salt_cold_in);
			}
			else
			{
				double delta_od = (soln.T_salt_hot - m_T_salt_hot_target) / ((soln.T_salt_hot - Tprev) / (soln.od_control - odprev));
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
			Tprev = soln.T_salt_hot;
		}
		q++;
	}

	return;
}



void C_mspt_receiver_222::calc_pump_performance(double rho_f, double mdot, double ffact, double& PresDrop_calc, double& WdotPump_calc)
{
    double ratio_dP_tower_to_rec = std::numeric_limits<double>::quiet_NaN();
    calc_pump_performance(rho_f, mdot, ffact, PresDrop_calc, WdotPump_calc, ratio_dP_tower_to_rec);
}


void C_mspt_receiver_222::calc_pump_performance(double rho_f, double mdot, double ffact, double &PresDrop_calc, double &WdotPump_calc, double& ratio_dP_tower_to_rec)
{

    // Pressure drop calculations
	double mpertube = mdot / ((double)m_n_lines * (double)m_n_t);
	double u_coolant = mpertube / (rho_f * m_id_tube * m_id_tube * 0.25 * CSP::pi);	//[m/s] Average velocity of the coolant through the receiver tubes

	double L_e_45 = 16.0;						// The equivalent length produced by the 45 degree bends in the tubes - Into to Fluid Mechanics, Fox et al.
	double L_e_90 = 30.0;						// The equivalent length produced by the 90 degree bends in the tubes
	double DELTAP_tube = rho_f*(ffact*m_h_rec / m_id_tube*pow(u_coolant, 2) / 2.0);	//[Pa] Pressure drop across the tube, straight length
	double DELTAP_45 = rho_f*(ffact*L_e_45*pow(u_coolant, 2) / 2.0);					//[Pa] Pressure drop across 45 degree bends
	double DELTAP_90 = rho_f*(ffact*L_e_90*pow(u_coolant, 2) / 2.0);					//[Pa] Pressure drop across 90 degree bends
	double DELTAP = DELTAP_tube + 2 * DELTAP_45 + 4 * DELTAP_90;						//[Pa] Total pressure drop across the tube with (4) 90 degree bends, (2) 45 degree bends
	double DELTAP_h_tower = rho_f*m_h_tower*CSP::grav;						//[Pa] The pressure drop from pumping up to the receiver
	double DELTAP_net = DELTAP*m_n_panels / (double)m_n_lines + DELTAP_h_tower;		//[Pa] The new pressure drop across the receiver panels
    ratio_dP_tower_to_rec = DELTAP_h_tower / DELTAP_net;            //[-] ratio of total pressure drop that is caused by tower height
	PresDrop_calc = DELTAP_net*1.E-6;			//[MPa]
	double est_load = fmax(0.25, mdot / m_m_dot_htf_des) * 100;		//[%] Relative pump load. Limit to 25%
	double eta_pump_adj = m_eta_pump*(-2.8825E-9*pow(est_load, 4) + 6.0231E-7*pow(est_load, 3) - 1.3867E-4*pow(est_load, 2) + 2.0683E-2*est_load);	//[-] Adjusted pump efficiency
	WdotPump_calc = DELTAP_net*mdot / rho_f / eta_pump_adj;

}

double C_mspt_receiver_222::get_pumping_parasitic_coef()
{
    double Tavg = (m_T_htf_cold_des + m_T_htf_hot_des) / 2.;
    
    double mu_coolant = field_htfProps.visc(Tavg);				//[kg/m-s] Absolute viscosity of the coolant
    double k_coolant = field_htfProps.cond(Tavg);				//[W/m-K] Conductivity of the coolant
    double rho_coolant = field_htfProps.dens(Tavg, 1.0);        //[kg/m^3] Density of the coolant
    double c_p_coolant = field_htfProps.Cp(Tavg)*1e3;           //[J/kg-K] Specific heat

    double m_dot_salt = m_q_rec_des / (c_p_coolant * (m_T_htf_hot_des - m_T_htf_cold_des));

    double n_t = (int)(CSP::pi*m_d_rec / (m_od_tube*m_n_panels));   // The number of tubes per panel, as a function of the number of panels and the desired diameter of the receiver
    double id_tube = m_od_tube - 2 * m_th_tube;                 //[m] Inner diameter of receiver tube


    double u_coolant = m_dot_salt / (n_t*rho_coolant*pow((id_tube / 2.0), 2)*CSP::pi);	//[m/s] Average velocity of the coolant through the receiver tubes
    double Re_inner = rho_coolant * u_coolant*id_tube / mu_coolant;				        //[-] Reynolds number of internal flow
    double Pr_inner = c_p_coolant * mu_coolant / k_coolant;						        //[-] Prandtl number of internal flow
    double Nusselt_t, f;
    double LoverD = m_h_rec / id_tube;
    double RelRough = (4.5e-5) / id_tube;   //[-] Relative roughness of the tubes. http:www.efunda.com/formulae/fluids/roughness.cfm
    CSP::PipeFlow(Re_inner, Pr_inner, LoverD, RelRough, Nusselt_t, f);

    double deltap, wdot;
    calc_pump_performance(rho_coolant, m_dot_salt, f, deltap, wdot);

    return wdot / m_q_rec_des;
}

double C_mspt_receiver_222::area_proj()
{
    return CSP::pi * m_d_rec * m_h_rec; //[m^2] projected or aperture area of the receiver
}

bool C_mspt_receiver_222::check_min_turndown(double q_dot_inc_sum, double step, C_csp_collector_receiver::E_csp_cr_modes input_operation_mode, bool &rec_is_off)
{
    m_is_below_cutoff = false;
    bool allow_low_power_operation = false;
    if (q_dot_inc_sum < m_q_dot_inc_min)
    {
        m_is_below_cutoff = true;
        if ((m_mode_prev == C_csp_collector_receiver::ON || m_mode_prev == C_csp_collector_receiver::STARTUP))
        {
            if (m_shutdown_method == 1 && m_moving_avg_power >= m_q_dot_inc_min)
                allow_low_power_operation = true;
            else if (m_shutdown_method == 2 && m_low_power_counter_prev + step <= m_shutdown_time_lag)
                allow_low_power_operation = true;
        }

        if (allow_low_power_operation)
        {
            if (m_mode == C_csp_collector_receiver::ON)  // Receiver was already operating and is allowed to continue operating despite low power.  Note that startup isn't included here becuase we've always allowed startup to happen with low power
            {
                m_low_power_counter = m_low_power_counter_prev + step;
            }
        }
        else
        {
            // Receiver can't operate
            // Steady State always reports q_thermal (even when much less than min) because model is letting receiver begin startup with this energy
            // Should be a way to communicate to controller that q_thermal is less than q_min without losing this functionality
            if (m_mode != C_csp_collector_receiver::STEADY_STATE || m_mode_prev == C_csp_collector_receiver::ON || m_mode_prev == C_csp_collector_receiver::OFF_NO_SU_REQ)
                rec_is_off = true;

            if (input_operation_mode == C_csp_collector_receiver::ON)   //Use input_operation_mode instead of m_mode to allow startup (m_mode is updated to "ON" in the startup case above when startup is completed)
            {
                m_mode = C_csp_collector_receiver::OFF;
            }

        }
    }
    else if (m_shutdown_method == 2)
    {
        m_reset_counter = m_reset_counter_prev + step;
    }
    return allow_low_power_operation;
}
