/*
BSD 3-Clause License

Copyright (c) Alliance for Energy Innovation, LLC. See also https://github.com/NREL/ssc/blob/develop/LICENSE
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

#ifndef __csp_solver_mspt_receiver_
#define __csp_solver_mspt_receiver_

#include "csp_solver_pt_receiver.h"
#include "csp_solver_util.h"
#include "csp_solver_mspt_receiver_222.h"

class C_mspt_receiver : public C_mspt_receiver_222
{
// The transient receiver, including legacy steady-state receiver code for either non-transient startup or non-transient operation

private:
	
	// track number of calls per timestep, reset = -1 in converged() call
	int m_ncall;

	//-------------------
    // Set in constructor
    bool m_is_transient;			// Use transient model?
    bool m_is_startup_transient;	// Use transient startup model?
    double m_rec_tm_mult;			//[-], receiver thermal mass multiplier
    double m_u_riser;				//[m/s], 
    double m_th_riser;				//[mm], convert to [m] in init()
    double m_riser_tm_mult;			//[-], riser thermal mass multiplier
    double m_downc_tm_mult;			//[-], downcomer thermal mass multiplier
    double m_heat_trace_power;		//[kW/m], convert to [W/m] in init()
    double m_tube_flux_preheat;		//[kW/m2]
    double m_min_preheat_time;      //[s] Minimum time required in preheat startup stage
    double m_fill_time;				//[s] Time requirement to fill receiver
    double m_flux_ramp_time;		//[s] 
    double m_preheat_target;		//[K]
    double m_startup_target_delta;	//[C/K], (target temperature at end of startup) - (steady state temperature at current conditions )
    double m_initial_temperature;	//[K]

    bool m_is_startup_from_solved_profile;  // Begin receiver startup from solved temperature profiles?
    bool m_is_enforce_min_startup;		// Always enforce minimum startup time?  If false, minimum startup time is ignored when receiver starts above preheat temperature

    // Transient model parameters
	int m_startup_mode;
	int m_startup_mode_initial;
	int m_n_call_fill;
	int m_n_call_fill_initial;


	double m_id_riser;				//[m]
	double m_od_riser;				//[m]
	double m_id_downc;				//[m]
	double m_od_downc;				//[m]
    double m_th_downc;				//[m]

	double m_total_startup_time; // [s]
	double m_total_startup_time_initial; //[s]
	double m_minimum_startup_time; //s
	double m_total_ramping_time; //s
	double m_total_ramping_time_initial; //s
	double m_total_fill_time;
	double m_total_fill_time_initial;
	double m_total_preheat_time;
	double m_total_preheat_time_initial;
	int m_crossover_index;


	int m_n_elem;
	int m_nz_tot;
	std::vector<double> m_tm;		 //[J/K/m]
    std::vector<double> m_tm_solid;	//[J/K/m]
    std::vector<double> m_od;		 //[m]
    std::vector<double> m_id;		 //[m]
	util::matrix_t<int> m_flowelem_type;
	util::matrix_t<double> m_tinit;
	util::matrix_t<double> m_tinit_wall;

	struct transient_inputs
	{
		size_t nelem;
		size_t nztot;
		size_t npath;
		double inlet_temp;
		util::matrix_t<double> lam1, lam2, cval, aval, tinit, tinit_wall, Rtube;
        std::vector<double> length, zpts;
        std::vector<int> nz, startpt;

		transient_inputs()
		{
			nelem = nztot = npath = 0;
			inlet_temp = std::numeric_limits<double>::quiet_NaN();
		}

	} trans_inputs;

	struct transient_outputs
	{
		double timeavg_tout;					// Time-averaged downcomer outlet T [K]
		double tout;							// Downcomer outlet T at the end of the time step [K] 
		double max_tout;						// Max downcomer outlet T during the time step [K]
		double min_tout;						// Min downcomer outlet T during the time step [K]
		double max_rec_tout;					// Max receiver outlet T during the time step [K]
		double timeavg_conv_loss;				// Time-averaged convection loss from the receiver panels [W]
		double timeavg_rad_loss;				// Time-averaged radiation loss
		double timeavg_piping_loss;				// Time-averaged thermal loss from piping [W]
		double timeavg_qthermal;				// Average thermal power sent to power cycle or storage during the time step [W]
		double timeavg_qnet;					// Average net thermal power absorbed by the receiver during the time step [W]
		double timeavg_qheattrace;				// Average heat trace thermal input during the time step [W]
		double timeavg_eta_therm;				// Time-averaged thermal efficiency of the receiver 
		double time_min_tout;					// Time at which minimum downcomer outlet T occurs
		double tube_temp_inlet;					// Receiver inlet tube wall temperature at the end of the the time step [K]
		double tube_temp_outlet;				// Receiver outlet tube wall temperature at the end of the the time step [K]

		util::matrix_t<double> t_profile;		// Axial temperature profile at the end of the time step[K]
		util::matrix_t<double> t_profile_wall;	// Axial wall temperature profile at the end of the time step[K]
		util::matrix_t<double> timeavg_temp;	// Time-average outlet temperature of each flow element [K]

		transient_outputs()
		{
			timeavg_tout = tout = max_tout = min_tout = max_rec_tout = timeavg_conv_loss = timeavg_rad_loss = timeavg_piping_loss = std::numeric_limits<double>::quiet_NaN();
			timeavg_qthermal = timeavg_qnet = timeavg_qheattrace  = timeavg_eta_therm = time_min_tout = tube_temp_inlet = tube_temp_outlet = std::numeric_limits<double>::quiet_NaN();
		}

	} trans_outputs;

	struct parameter_eval_inputs
	{
		double T_amb, T_sky, pres, wspd, c_htf, rho_htf, mu_htf, k_htf, Pr_htf, mflow_tot, finitial, ffinal, ramptime;
		std::vector<double> tm;
		util::matrix_t<double> Tfeval, Tseval, qinc, qheattrace;
		
		parameter_eval_inputs()
		{
			T_amb = T_sky = pres = wspd = c_htf = rho_htf = mu_htf = k_htf = Pr_htf = mflow_tot = finitial = ffinal = ramptime = std::numeric_limits<double>::quiet_NaN();
		}

	} param_inputs;



	void initialize_transient_parameters();


	double calc_external_convection_coeff(double T_amb, double P_amb, double wspd, double Twall);
	void calc_thermal_loss(double Ts, double T_amb, double T_sky, double P_amb, double wspd, double &hext, double &qconv, double &qrad);
	void calc_surface_temperature(double Tf, double qabs, double Rtube, double OD, double T_amb, double T_sky, double P_amb, double wspd, double &Tsguess);
	void calc_header_size(double pdrop, double mdot, double rhof, double muf, double Lh, double &id_calc, double &th_calc, double &od_calc);
	double interpolate(double x, const std::vector<double> &xarray, const std::vector<double> &yarray, int klow, int khigh);
	double integrate(double xlow, double xhigh, const std::vector<double> &xarray, const std::vector<double> &yarray, int klow, int khigh);
	void cubic_splines(const std::vector<double> &xarray, const std::vector<double> &yarray, util::matrix_t<double> &splines);
	double calc_single_pt(double tpt, double zpt, int flowid, int pathid, const transient_inputs &tinputs);
	double calc_timeavg_exit_temp(double tstep, int flowid, int pathid, const transient_inputs &tinputs);

	
	void calc_ss_profile(const transient_inputs &tinputs, util::matrix_t<double> &tprofile, util::matrix_t<double> &tprofile_wall);
	void calc_axial_profile( double tpt, const transient_inputs &tinputs, util::matrix_t<double> &tprofile);
	void calc_extreme_outlet_values(double tstep, int flowid, const transient_inputs &tinputs, util::matrix_t<double> &textreme, util::matrix_t<double> &tpt);	
	void initialize_transient_param_inputs(const s_steady_state_soln &soln, parameter_eval_inputs &pinputs);
	void update_pde_parameters(bool use_initial_t, parameter_eval_inputs &pinputs, transient_inputs &tinputs);
	void solve_transient_model(double tstep, double allowable_Trise, parameter_eval_inputs &pinputs, transient_inputs &tinputs, transient_outputs &toutputs);
	void solve_transient_startup_model(parameter_eval_inputs &pinputs, transient_inputs &tinputs, int startup_mode, double target_temperature, double min_time, double max_time, transient_outputs &toutputs, double &startup_time, double &energy, double &parasitic);
	void set_heattrace_power(bool is_maintain_T, double Ttarget, double time, parameter_eval_inputs &pinputs, transient_inputs &tinputs);

	enum startup_modes
	{
		HEAT_TRACE = 0,		// No flux on receiver, riser/downcomer heated with heat tracing
		PREHEAT,			// Low flux on receiver, no HTF flow
		PREHEAT_HOLD,		// Preheat temperature requirement has been met, but not time requirement.  Low flux on receiver, no HTF flow
		FILL,				// User-defined time delay for filling the receiver/piping
		CIRCULATE,			// Full available power on receiver (with optional ramp rate), HTF mass flow rate selected to hit target hot at steady state
		HOLD				// Models predict that startup has been completed, but minimum startup time has not yet been reached.  Fluid continues to circulate through the receiver.  
	};


public:

	// Methods
	C_mspt_receiver(double h_tower, double epsilon /*-*/,
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
        bool is_transient /*-*/, bool is_startup_transient /*-*/,
        double rec_tm_mult /*-*/, double u_riser /*m/s*/,
        double th_riser /*mm*/, double riser_tm_mult /*-*/,
        double downc_tm_mult /*-*/, double heat_trace_power /*kW/m*/,
        double tube_flux_preheat /*kW/m2*/, double min_preheat_time /*hr*/,
        double fill_time /*hr*/, double flux_ramp_time /*hr*/,
        double preheat_target /*C*/, double startup_target_delta /*C*/,
        double initial_temperature /*C*/,
        bool is_startup_from_solved_profile, bool is_enforce_min_startup);

	~C_mspt_receiver(){};

	void init() override;

    void call(double step /*s*/,
        double P_amb /*Pa*/, double T_amb /*K*/, double T_sky /*K*/,
        double clearsky_to_input_dni /*-*/,
        double v_wind_10 /*m/s*/, 
        double plant_defocus /*-*/,
        const util::matrix_t<double>* flux_map_input, C_csp_collector_receiver::E_csp_cr_modes input_operation_mode,
        double T_salt_cold_in /*K*/) override;

	void off(const C_csp_weatherreader::S_outputs &weather,
		const C_csp_solver_htf_1state &htf_state_in,
		const C_csp_solver_sim_info &sim_info) override;

	void converged() override;

	void est_startup_time_energy(double fract, double &est_time, double &est_energy);

	double est_heattrace_energy();

    virtual double get_startup_time();

    virtual double get_startup_energy();
};

#endif // __csp_solver_mspt_receiver_222_
