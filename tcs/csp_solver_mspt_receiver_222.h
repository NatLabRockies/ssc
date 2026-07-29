/*
BSD 3-Clause License

Copyright (c) Alliance for Energy Innovation, LLC. See also https://github.com/NatLabRockies/ssc/blob/develop/LICENSE
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

#ifndef __csp_solver_mspt_receiver_222_
#define __csp_solver_mspt_receiver_222_

#include "csp_solver_util.h"

#include "htf_props.h"
#include "csp_solver_core.h"
#include "csp_solver_pt_receiver.h"

class C_mspt_receiver_222 : public C_pt_receiver
{
// The steady-state receiver (as opposed to the transient, for example)

protected:

    struct s_steady_state_soln
    {
        C_csp_collector_receiver::E_csp_cr_modes mode;
        bool rec_is_off;

        double T_amb;				// Dry bulb temperature (K)
        double v_wind_10;			// Wind speed at 10m (m/s)
        double p_amb;				// Ambient pressure (Pa)
        double T_sky;               // Sky temperature (K)

        double flux_sum;            // Flux map summed (W/m2)

        double dni_applied_to_measured; //[-] Ratio of DNI used to measured DNI
        double plant_defocus;       // plant defocus
        double clearsky_to_input_dni;   //[-]

        double od_control;          // Defocus control

        double m_dot_salt;			// Salt mass flow per path (kg/s)
        double m_dot_salt_tot;      // Total salt mass flow (kg/s)
        double T_salt_cold_in;		// Cold salt inlet temperature (K)
        double T_salt_hot;			// Receiver outlet T including piping loss (K)
        double T_salt_hot_rec;      // Receiver outlet T before piping loss (K)
        double T_salt_props;		// Temperature at which salt properties are evaluated

        double u_salt;				// Salt velocity (m/s)
        double f;					// Friction factor

        double Q_inc_sum;			// Total absorbed solar energy (W)
        double Q_conv_sum;			// Total convection loss (W)
        double Q_rad_sum;			// Total radiation loss (W)
        double Q_abs_sum;			// Total energy transferred to HTF, not including piping loss (W)
        double Q_dot_piping_loss;   // Piping loss (W)
        double Q_inc_min;			// Minimum absorbed solar energy on any panel (W)
        double Q_thermal;			// Thermal power delivered to fluid (less piping loss) (W)

        double eta_therm;			// Receiver thermal efficiency (energy to HTF not including piping loss / Absorbed solar energy)

        util::matrix_t<double> T_s;			// Average external tube T (K)
        util::matrix_t<double> T_panel_out; // Panel HTF outlet T (K)
        util::matrix_t<double> T_panel_in;	// Panel HTF inlet T (K)
        util::matrix_t<double> T_panel_ave; // Panel average HTF T (k)

        util::matrix_t<double> q_dot_inc;  // Panel absorbed solar energy (W)
        util::matrix_t<double> q_dot_conv; // Panel convection loss (W)
        util::matrix_t<double> q_dot_rad;  // Panel radiation loss (W)
        util::matrix_t<double> q_dot_loss; // Panel convection + radiation loss (W)
        util::matrix_t<double> q_dot_abs;  // Panel energy to HTF (W)

        s_steady_state_soln()
        {
            clear();
        }

        void clear()
        {
            T_amb = v_wind_10 = p_amb = T_sky = std::numeric_limits<double>::quiet_NaN();
            dni_applied_to_measured = od_control = plant_defocus = clearsky_to_input_dni =
                m_dot_salt = m_dot_salt_tot = T_salt_cold_in = T_salt_hot = T_salt_hot_rec = T_salt_props = std::numeric_limits<double>::quiet_NaN();
            u_salt = f = Q_inc_sum = Q_conv_sum = Q_rad_sum = Q_abs_sum = Q_dot_piping_loss = Q_inc_min = Q_thermal = eta_therm = std::numeric_limits<double>::quiet_NaN();

            mode = C_csp_collector_receiver::E_csp_cr_modes::OFF;
            rec_is_off = true;
        }
    };

    // Input parameters defined in constructor
    int m_n_panels;			    //[-]
    double m_d_rec;			    //[m]
    double m_h_rec;			    //[m]
    double m_hl_ffact;		    //[-]
    int m_flow_type;            //[-]
    int m_crossover_shift;      //[-]
    double m_T_salt_hot_target; //[K], converted from C in constructor
    double m_csky_frac;         //[-]

    bool m_is_calc_od_tube;     //[-]
    double m_W_dot_rec_target;    //[-]

    // Hardcoded parameters
    double m_tol_od;            //[-]
    double m_eta_therm_des_est; //[-]
        // Derived class should overwrite
    bool m_use_constant_piping_loss;

    // Calculated parameters
    // MSPT common external (steady state and transient)
    double m_id_tube;           //[m]
	double m_A_tube;            //[m2]
	int m_n_t;                  //[-]
	double m_A_rec_proj;        //[m2]
	double m_A_node;            //[m2]
    double m_Rtot_riser;		//[K*m/W]
    double m_Rtot_downc;		//[K*m/W]

    util::matrix_t<int> m_flow_pattern;     //[-] 
    int m_n_lines;                          //[-]

    double m_m_mixed;       //[-] Exponential for calculating mixed convection
    double m_LoverD;        //[-]
    double m_RelRough;      //[-]

    // Initial State variables
    C_csp_collector_receiver::E_csp_cr_modes m_mode_initial;
    double m_E_su_init;         //[W-hr] Initial startup energy
    double m_t_su_init;         //[hr] Startup time requirement

    // Stored solutions
    s_steady_state_soln m_mflow_soln_prev;  // Steady state solution using actual DNI from the last call to the model
    s_steady_state_soln m_mflow_soln_csky_prev;  // Steady state solution using clear-sky DNI from the last call to the model

    // Model output arrays
    // Member variables so don't waste time constructing each call?
    util::matrix_t<double> m_q_dot_inc;
    util::matrix_t<double> m_T_s;
    util::matrix_t<double> m_T_panel_out;
    util::matrix_t<double> m_T_panel_in;
    util::matrix_t<double> m_T_panel_ave;
    util::matrix_t<double> m_q_dot_conv;
    util::matrix_t<double> m_q_dot_rad;
    util::matrix_t<double> m_q_dot_loss;
    util::matrix_t<double> m_q_dot_abs;

private:

	// track number of calls per timestep, reset = -1 in converged() call
	int m_ncall;
     
    class C_MEQ__calc_OD_des_for_W_dot_pump_rec : public C_monotonic_equation
    {
    private:
        C_mspt_receiver_222* mpc_rec;

    public:

        C_MEQ__calc_OD_des_for_W_dot_pump_rec(C_mspt_receiver_222* pc_rec);

        virtual int operator()(double OD /*m*/, double* W_dot_pump_des /*MWe*/) override;

    };

    class C_MEQ__q_dot_des : public C_monotonic_equation
    {
    private:
        C_mspt_receiver_222* mpc_rec;
        util::matrix_t<double> m_flux_map_input;

        double m_min_to_max_flux_ratio;
        double m_step;      //[s]
        double m_plant_defocus;  //[-]
        C_csp_collector_receiver::E_csp_cr_modes m_input_operation_mode;

    public:

        C_MEQ__q_dot_des(C_mspt_receiver_222* pc_rec);

        virtual int operator()(double flux_max /*kW/m2*/, double *q_dot_des /*MWt*/) override;
    };

protected:

    void init_mspt_common();

    void design_point_steady_state(double& eta_thermal_des_calc /*-*/,
        double& W_dot_rec_pump_des_calc /*MWe*/,
        double& W_dot_rec_pump__tower_only /*MWe*/, double& W_dot_rec_pump__rec_only /*MWe*/,
        double& rec_pump_coef /*MWe/MWt*/, double& vel_htf_des /*m/s*/);

    bool use_previous_solution(const s_steady_state_soln& soln, const s_steady_state_soln& soln_prev);
    util::matrix_t<double> calculate_flux_profiles(double flux_sum /*W/m2*/, double dni_scale /*-*/, double plant_defocus /*-*/,
        double od_control /*-*/, const util::matrix_t<double>* flux_map_input);
    void calculate_steady_state_soln(s_steady_state_soln& soln, double tol, bool use_constant_piping_loss, int max_iter = 50);
    void solve_for_mass_flow(s_steady_state_soln& soln);
    void solve_for_mass_flow_and_defocus(s_steady_state_soln& soln, double m_dot_htf_max, const util::matrix_t<double>* flux_map_input);
    void solve_for_defocus_given_flow(s_steady_state_soln& soln, const util::matrix_t<double>* flux_map_input);

    void call_common(double P_amb /*Pa*/, double T_amb /*K*/,
        //double I_bn /*W/m2*/,
        double clearsky_to_input_dni /*-*/,
        double v_wind_10 /*m/s*/, double T_sky /*K*/,
        //double clearsky_dni /*W/m2*/,
        double T_salt_cold_in /*K*/,
        double plant_defocus /*-*/,
        const util::matrix_t<double>* flux_map_input,
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
        double& q_thermal_cksy /*Wt*/, double& q_thermal_steadystate /*Wt*/,
        double& od_control /*-*/,
        s_steady_state_soln& soln);

public:
	// Class to save messages for up stream classes
	C_csp_messages csp_messages;

	// Methods
	C_mspt_receiver_222(double h_tower /*m*/, double epsilon /*-*/,
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
        bool is_calc_od_tube /*-*/, double W_dot_rec_target /*MWe*/);

	~C_mspt_receiver_222(){};

	virtual void init() override;

    void set_inital_state(C_csp_collector_receiver::E_csp_cr_modes mode_initial,
        double E_su_init /*W-hr*/, double t_su_init /*hr*/);

	virtual void call(const C_csp_weatherreader::S_outputs &weather, 
		const C_csp_solver_htf_1state &htf_state_in, 
		const C_pt_receiver::S_inputs &inputs,
		const C_csp_solver_sim_info &sim_info) override;

    virtual void call(double step /*s*/,
        double P_amb /*Pa*/, double T_amb /*K*/, double T_sky /*K*/,
        double clearsky_to_input_dni /*-*/,
        double v_wind_10 /*m/s*/,
        double plant_defocus /*-*/,
        const util::matrix_t<double>* flux_map_input, C_csp_collector_receiver::E_csp_cr_modes input_operation_mode,
        double T_salt_cold_in /*K*/);

	virtual void off(const C_csp_weatherreader::S_outputs &weather,
		const C_csp_solver_htf_1state &htf_state_in,
		const C_csp_solver_sim_info &sim_info) override;

	virtual void converged() override;

    void calc_pump_performance(double rho_f, double mdot /*kg/s*/, double ffact, double& PresDrop_calc /*MPa*/, double& WdotPump_calc /*W*/);

    void calc_pump_performance(double rho_f, double mdot /*kg/s*/, double ffact, double& PresDrop_calc /*MPa*/, double& WdotPump_calc /*W*/, double& ratio_dP_tower_to_rec /*-*/);

    double get_pumping_parasitic_coef() override;

    double area_proj() override;

    void get_solved_design_common(double& m_dot_rec_total /*kg/s*/,
        double& T_htf_cold_des /*K*/, int& n_panels);

    void overwrite_startup_requirements_to_on();
};

#endif // __csp_solver_mspt_receiver_222_
