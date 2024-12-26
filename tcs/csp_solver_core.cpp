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

#include "csp_solver_core.h"
#include "csp_solver_util.h"

#include "lib_util.h"
#include "csp_dispatch.h"
#include "etes_dispatch.h"

#include <algorithm>

#include <sstream>

#undef min
#undef max

void C_timestep_fixed::init(double time_start /*s*/, double step /*s*/)
{
	ms_timestep.m_time_start = time_start;	//[s]
	ms_timestep.m_step = step;				//[s]
	ms_timestep.m_time = ms_timestep.m_time_start + ms_timestep.m_step;	//[s]
}

double C_timestep_fixed::get_end_time()
{
	return ms_timestep.m_time;	//[s]
}

void C_timestep_fixed::step_forward()
{
	ms_timestep.m_time_start = ms_timestep.m_time;	//[s] Set start time to previous end time
	ms_timestep.m_time += ms_timestep.m_step;		//[s] Step forward to step new end time
}

double C_timestep_fixed::get_step()
{
	return ms_timestep.m_step;
}

void C_csp_solver::C_csp_solver_kernel::init(C_csp_solver::S_sim_setup & sim_setup, double wf_step /*s*/, double baseline_step /*s*/, C_csp_messages & csp_messages)
{
	ms_sim_setup = sim_setup;
	
	// Compare steps: if necessary, set baseline to = weather file
	if(baseline_step > wf_step)
	{
		std::string msg = util::format("The input Baseline Simulation Timestep (%lg [s]) must be less than or equal to " 
								"the Weatherfile Timestep (%lg [s]). It was reset to the Weatherfile Timestep", baseline_step, wf_step);
		csp_messages.add_message(C_csp_messages::WARNING, msg);
		baseline_step = wf_step;
	}
	else if( (int)wf_step % (int)baseline_step != 0)
	{
		double wf_over_bl = wf_step / baseline_step;
		double wf_over_bl_new = ceil(wf_over_bl);
		double baseline_step_new = wf_step / wf_over_bl_new;

		std::string msg = util::format("The Weatherfile Timestep (%lg [s]) must be divisible by the "
								"input Baseline Simulation Timestep (%lg [s]). It was reset to %lg [s].", wf_step, baseline_step, baseline_step_new);

		csp_messages.add_message(C_csp_messages::WARNING, msg);
		baseline_step = baseline_step_new;
	}

	// Define start and steps for weatherfile, baseline, and sim_info timesteps
	double wf_time_start = ms_sim_setup.m_sim_time_start;			//[s]
	double baseline_time_start = ms_sim_setup.m_sim_time_start;		//[s]

	// Initialize the private C_timestep_fixed classes
	mc_ts_weatherfile.init(wf_time_start, wf_step);
	mc_ts_sim_baseline.init(baseline_time_start, baseline_step);

	// Set up 'mc_sim_info'
	mc_sim_info.ms_ts.m_time_start = ms_sim_setup.m_sim_time_start;	//[s]
	mc_sim_info.ms_ts.m_step = baseline_step;						//[s]
	mc_sim_info.ms_ts.m_time = mc_sim_info.ms_ts.m_time_start + mc_sim_info.ms_ts.m_step;	//[s]
}

double C_csp_solver::C_csp_solver_kernel::get_wf_end_time()
{
	return mc_ts_weatherfile.get_end_time();
}

double C_csp_solver::C_csp_solver_kernel::get_baseline_end_time()
{
	return mc_ts_sim_baseline.get_end_time();
}

void C_csp_solver::C_csp_solver_kernel::wf_step_forward()
{
	mc_ts_weatherfile.step_forward();
}

const C_csp_solver::S_sim_setup * C_csp_solver::C_csp_solver_kernel::get_sim_setup()
{
	return &ms_sim_setup;
}

double C_csp_solver::C_csp_solver_kernel::get_wf_step()
{
	return mc_ts_weatherfile.get_step();
}

double C_csp_solver::C_csp_solver_kernel::get_baseline_step()
{
	return mc_ts_sim_baseline.get_step();
}

void C_csp_solver::C_csp_solver_kernel::baseline_step_forward()
{
	mc_ts_sim_baseline.step_forward();
}

void C_csp_collector_receiver::on(const C_csp_weatherreader::S_outputs& weather,
    const C_csp_solver_htf_1state& htf_state_in,
    double T_CT_htf_hot_in /*C*/,
    double q_dot_elec_to_CR_heat /*MWt*/, double field_control,
    C_csp_collector_receiver::S_csp_cr_out_solver& cr_out_solver,
    const C_csp_solver_sim_info& sim_info)
{
    on(weather,
        htf_state_in,
        q_dot_elec_to_CR_heat, field_control,
        cr_out_solver,
        sim_info);
}

void C_csp_power_cycle::call(const C_csp_weatherreader::S_outputs& weather,
    C_csp_solver_htf_1state& htf_state_in,
    double T_CT_htf_cold_in /*C*/,
    const C_csp_power_cycle::S_control_inputs& inputs,
    C_csp_power_cycle::S_csp_pc_out_solver& out_solver,
    const C_csp_solver_sim_info& sim_info)
{
    call(weather,
        htf_state_in,
        inputs,
        out_solver,
        sim_info);
}

static C_csp_reported_outputs::S_output_info S_solver_output_info[] =
{
	// Ouputs that are NOT reported as weighted averages
		// Simulation
	{C_csp_solver::C_solver_outputs::TIME_FINAL, C_csp_reported_outputs::TS_LAST},	//[hr]
		// Weather Reader
	{ C_csp_solver::C_solver_outputs::MONTH, C_csp_reported_outputs::TS_1ST},		//[-] Month of year
	{ C_csp_solver::C_solver_outputs::HOUR_DAY, C_csp_reported_outputs::TS_1ST},    //[hr] hour of day
		// Controller, TES, & Dispatch
	{C_csp_solver::C_solver_outputs::ERR_M_DOT, C_csp_reported_outputs::TS_1ST},		          //[-] Relative mass conservation error
	{C_csp_solver::C_solver_outputs::ERR_Q_DOT, C_csp_reported_outputs::TS_1ST},		          //[-] Relative energy conservation error
	{C_csp_solver::C_solver_outputs::N_OP_MODES, C_csp_reported_outputs::TS_LAST},	              //[-] Number of subtimesteps in reporting timestep
	{C_csp_solver::C_solver_outputs::OP_MODE_1, C_csp_reported_outputs::TS_1ST},                  //[-] Operating mode in first subtimestep
	{C_csp_solver::C_solver_outputs::OP_MODE_2, C_csp_reported_outputs::TS_1ST},		          //[-] Operating mode in second subtimestep
	{C_csp_solver::C_solver_outputs::OP_MODE_3, C_csp_reported_outputs::TS_1ST},		          //[-] Operating mode in third subtimestep
	{C_csp_solver::C_solver_outputs::TOU_PERIOD, C_csp_reported_outputs::TS_1ST},                 //[-] CSP operating TOU period
	{C_csp_solver::C_solver_outputs::PRICING_MULT, C_csp_reported_outputs::TS_1ST},				  //[-] PPA price multiplier
	{C_csp_solver::C_solver_outputs::PC_Q_DOT_SB, C_csp_reported_outputs::TS_1ST},				  //[MWt] PC required standby thermal power
	{C_csp_solver::C_solver_outputs::PC_Q_DOT_MIN, C_csp_reported_outputs::TS_1ST},				  //[MWt] PC required min thermal power
	{C_csp_solver::C_solver_outputs::PC_Q_DOT_TARGET, C_csp_reported_outputs::TS_WEIGHTED_AVE},	  //[MWt] PC target thermal power
	{C_csp_solver::C_solver_outputs::PC_Q_DOT_MAX, C_csp_reported_outputs::TS_WEIGHTED_AVE},	  //[MWt] PC allowable max thermal power
    {C_csp_solver::C_solver_outputs::PC_Q_DOT_TARGET_SU, C_csp_reported_outputs::TS_MAX},		  //[MWt] PC target thermal power for startup
    {C_csp_solver::C_solver_outputs::PC_Q_DOT_TARGET_ON, C_csp_reported_outputs::TS_MAX},		  //[MWt] PC target thermal power for startup
	{C_csp_solver::C_solver_outputs::CTRL_IS_REC_SU, C_csp_reported_outputs::TS_1ST},			  //[-] Control decision: is receiver startup allowed?
	{C_csp_solver::C_solver_outputs::CTRL_IS_PC_SU, C_csp_reported_outputs::TS_1ST},			  //[-] Control decision: is power cycle startup allowed?
	{C_csp_solver::C_solver_outputs::CTRL_IS_PC_SB, C_csp_reported_outputs::TS_1ST},			  //[-] Control decision: is power cycle standby allowed?
	{C_csp_solver::C_solver_outputs::CTRL_IS_PAR_HTR_SU, C_csp_reported_outputs::TS_1ST},		  //[-] Control decision: is parallel electric heater startup allowed?
	{C_csp_solver::C_solver_outputs::PAR_HTR_Q_DOT_TARGET, C_csp_reported_outputs::TS_1ST},		  //[MWt] Parallel electric heater target thermal power
	{C_csp_solver::C_solver_outputs::EST_Q_DOT_CR_SU, C_csp_reported_outputs::TS_1ST},			  //[MWt] Estimate receiver startup thermal power
	{C_csp_solver::C_solver_outputs::EST_Q_DOT_CR_ON, C_csp_reported_outputs::TS_1ST},			  //[MWt] Estimate receiver thermal power to HTF
	{C_csp_solver::C_solver_outputs::EST_Q_DOT_DC, C_csp_reported_outputs::TS_1ST},				  //[MWt] Estimate max TES dc thermal power
	{C_csp_solver::C_solver_outputs::EST_Q_DOT_CH, C_csp_reported_outputs::TS_1ST},				  //[MWt] Estimate max TES ch thermal power
	{C_csp_solver::C_solver_outputs::CTRL_OP_MODE_SEQ_A, C_csp_reported_outputs::TS_1ST},		  //[-] First 3 operating modes tried
	{C_csp_solver::C_solver_outputs::CTRL_OP_MODE_SEQ_B, C_csp_reported_outputs::TS_1ST},		  //[-] Next 3 operating modes tried
	{C_csp_solver::C_solver_outputs::CTRL_OP_MODE_SEQ_C, C_csp_reported_outputs::TS_1ST},		  //[-] Final 3 operating modes tried
	{C_csp_solver::C_solver_outputs::DISPATCH_REL_MIP_GAP, C_csp_reported_outputs::TS_1ST},		  //[-] Relative MIP gap from optimization solver
	{C_csp_solver::C_solver_outputs::DISPATCH_SOLVE_STATE, C_csp_reported_outputs::TS_1ST},		  //[-] The status of the dispatch optimization solver
    {C_csp_solver::C_solver_outputs::DISPATCH_SUBOPT_FLAG, C_csp_reported_outputs::TS_1ST},		  //[-] Flag specifing information about LPSolve suboptimal result
	{C_csp_solver::C_solver_outputs::DISPATCH_SOLVE_ITER, C_csp_reported_outputs::TS_1ST},		  //[-] Number of iterations before completing dispatch optimization
	{C_csp_solver::C_solver_outputs::DISPATCH_SOLVE_OBJ, C_csp_reported_outputs::TS_1ST},		  //[?] Objective function value achieved by the dispatch optimization solver
	{C_csp_solver::C_solver_outputs::DISPATCH_SOLVE_OBJ_RELAX, C_csp_reported_outputs::TS_1ST},	  //[?] Objective function value for the relaxed continuous problem 
	{C_csp_solver::C_solver_outputs::DISPATCH_QSF_EXPECT, C_csp_reported_outputs::TS_1ST},		  //[MWt] Expected total solar field energy generation in dispatch model
	{C_csp_solver::C_solver_outputs::DISPATCH_QSFPROD_EXPECT, C_csp_reported_outputs::TS_1ST},	  //[MWt] Expected useful solar field energy generation in dispatch model
	{C_csp_solver::C_solver_outputs::DISPATCH_QSFSU_EXPECT, C_csp_reported_outputs::TS_1ST},      //[MWt] Solar field startup energy in dispatch model
	{C_csp_solver::C_solver_outputs::DISPATCH_TES_EXPECT, C_csp_reported_outputs::TS_1ST},		  //[MWht] Thermal energy storage charge state in dispatch model
	{C_csp_solver::C_solver_outputs::DISPATCH_PCEFF_EXPECT, C_csp_reported_outputs::TS_1ST},	  //[-] Expected power cycle efficiency adjustment in dispatch model
	{C_csp_solver::C_solver_outputs::DISPATCH_SFEFF_EXPECT, C_csp_reported_outputs::TS_1ST},	  //[-] Expected solar field thermal efficiency adjustment in dispatch model
	{C_csp_solver::C_solver_outputs::DISPATCH_QPBSU_EXPECT, C_csp_reported_outputs::TS_1ST},	  //[MWt] Power cycle startup energy consumption in dispatch model
	{C_csp_solver::C_solver_outputs::DISPATCH_WPB_EXPECT, C_csp_reported_outputs::TS_1ST},		  //[MWe] Power cycle electricity production in dispatch model
	{C_csp_solver::C_solver_outputs::DISPATCH_REV_EXPECT, C_csp_reported_outputs::TS_1ST},		  //[MWe*fact] Power cycle electricity production times revenue factor in dispatch model
	{C_csp_solver::C_solver_outputs::DISPATCH_PRES_NCONSTR, C_csp_reported_outputs::TS_1ST},	  //[-] Number of constraint relationships in dispatch model formulation
	{C_csp_solver::C_solver_outputs::DISPATCH_PRES_NVAR, C_csp_reported_outputs::TS_1ST},		  //[-] Number of variables in dispatch model formulation
	{C_csp_solver::C_solver_outputs::DISPATCH_SOLVE_TIME, C_csp_reported_outputs::TS_1ST},		  //[sec]   Time required to solve the dispatch model at each instance

	// **************************************************************
	//      Outputs that are reported as weighted averages if 
	//       multiple csp-timesteps for one reporting timestep
	// **************************************************************

	{C_csp_solver::C_solver_outputs::SOLZEN, C_csp_reported_outputs::TS_WEIGHTED_AVE},			//[deg] Solar zenith angle
	{C_csp_solver::C_solver_outputs::SOLAZ, C_csp_reported_outputs::TS_WEIGHTED_AVE},			//[deg] Solar azimuth angle
	{C_csp_solver::C_solver_outputs::BEAM, C_csp_reported_outputs::TS_WEIGHTED_AVE},			//[W/m^2] Resource beam normal irradiance
	{C_csp_solver::C_solver_outputs::TDRY, C_csp_reported_outputs::TS_WEIGHTED_AVE},			//[C] Dry bulb temperature
	{C_csp_solver::C_solver_outputs::TWET, C_csp_reported_outputs::TS_WEIGHTED_AVE},			//[C] Wet bulb temperature
	{C_csp_solver::C_solver_outputs::RH, C_csp_reported_outputs::TS_WEIGHTED_AVE},				//[-] Relative humidity
	{C_csp_solver::C_solver_outputs::WSPD, C_csp_reported_outputs::TS_WEIGHTED_AVE},            //[m/s] Wind speed
    {C_csp_solver::C_solver_outputs::WDIR, C_csp_reported_outputs::TS_WEIGHTED_AVE},            //[deg] Wind direction
	{C_csp_solver::C_solver_outputs::PRES, C_csp_reported_outputs::TS_WEIGHTED_AVE}, 			//[mbar] Atmospheric pressure
		
		// Controller and TES
	{C_csp_solver::C_solver_outputs::CR_DEFOCUS, C_csp_reported_outputs::TS_WEIGHTED_AVE},		//[-] Field optical focus fraction
	{C_csp_solver::C_solver_outputs::TES_Q_DOT_DC, C_csp_reported_outputs::TS_WEIGHTED_AVE},	//[MWt] TES discharge thermal power
	{C_csp_solver::C_solver_outputs::TES_Q_DOT_CH, C_csp_reported_outputs::TS_WEIGHTED_AVE},	//[MWt] TES charge thermal power
	{C_csp_solver::C_solver_outputs::TES_E_CH_STATE, C_csp_reported_outputs::TS_LAST},			//[MWht] TES charge state at the end of the time step
    {C_csp_solver::C_solver_outputs::TES_T_COLD_IN, C_csp_reported_outputs::TS_WEIGHTED_AVE},	//[MWt] Inlet temperature to cold TES

	{C_csp_solver::C_solver_outputs::M_DOT_CR_TO_TES_HOT, C_csp_reported_outputs::TS_WEIGHTED_AVE},		//[kg/s]
    {C_csp_solver::C_solver_outputs::M_DOT_CR_TO_TES_COLD, C_csp_reported_outputs::TS_WEIGHTED_AVE},	//[kg/s]
	{C_csp_solver::C_solver_outputs::M_DOT_TES_HOT_OUT, C_csp_reported_outputs::TS_WEIGHTED_AVE},		//[kg/s]
	{C_csp_solver::C_solver_outputs::M_DOT_PC_TO_TES_COLD, C_csp_reported_outputs::TS_WEIGHTED_AVE},	//[kg/s]
	{C_csp_solver::C_solver_outputs::M_DOT_TES_COLD_OUT, C_csp_reported_outputs::TS_WEIGHTED_AVE},		//[kg/s]
    {C_csp_solver::C_solver_outputs::M_DOT_TES_COLD_IN, C_csp_reported_outputs::TS_WEIGHTED_AVE},		//[kg/s]
	{C_csp_solver::C_solver_outputs::M_DOT_FIELD_TO_CYCLE, C_csp_reported_outputs::TS_WEIGHTED_AVE},	//[kg/s]
	{C_csp_solver::C_solver_outputs::M_DOT_CYCLE_TO_FIELD, C_csp_reported_outputs::TS_WEIGHTED_AVE},	//[kg/s]
	
	{C_csp_solver::C_solver_outputs::SYS_W_DOT_FIXED, C_csp_reported_outputs::TS_WEIGHTED_AVE},	  //[MWe] Parasitic fixed power consumption
	{C_csp_solver::C_solver_outputs::SYS_W_DOT_BOP, C_csp_reported_outputs::TS_WEIGHTED_AVE},		  //[MWe] Parasitic BOP power consumption
	{C_csp_solver::C_solver_outputs::W_DOT_NET, C_csp_reported_outputs::TS_WEIGHTED_AVE},			  //[MWe] System total electric power to grid

	csp_info_invalid
};

C_csp_solver::C_csp_solver(C_csp_weatherreader &weather,
	C_csp_collector_receiver &collector_receiver,
	C_csp_power_cycle &power_cycle,
	C_csp_tes &tes,
	C_csp_tou &tou,
    base_dispatch_opt &dispatch,
	S_csp_system_params &system,
    C_csp_collector_receiver* heater,
    std::shared_ptr<C_csp_tes> c_CT_tes,
	bool(*pf_callback)(std::string &log_msg, std::string &progress_msg, void *data, double progress, int out_type),
	void *p_cmod_active) :
	mc_weather(weather), 
	mc_collector_receiver(collector_receiver), 
	mc_power_cycle(power_cycle),
	mc_tes(tes),
	mc_tou(tou),
    mc_dispatch(dispatch),
	ms_system_params(system)
{
    // Assign remaining member data
    mp_heater = heater;
    mc_CT_tes = c_CT_tes;
    mpf_callback = pf_callback;
    mp_cmod_active = p_cmod_active;

    // Default to a system without a parallel heater
    if (mp_heater == NULL) {
        m_is_parallel_heater = false;
    }
    else {
        m_is_parallel_heater = true;
    }

	// Hierarchy logic
	//reset_hierarchy_logic();
    mc_operating_modes.reset_all_availability();

	// Inititalize non-reference member data
	m_T_htf_cold_des = m_P_cold_des = m_x_cold_des =
		m_q_dot_rec_des = m_A_aperture = m_CT_to_HT_m_dot_ratio =
        m_PAR_HTR_T_htf_cold_des = m_PAR_HTR_P_cold_des = m_PAR_HTR_x_cold_des = m_PAR_HTR_q_dot_rec_des = m_PAR_HTR_A_aperture =
		m_cycle_W_dot_des = m_cycle_eta_des = m_cycle_q_dot_des = m_cycle_max_frac = m_cycle_cutoff_frac =
		m_cycle_sb_frac_des = m_cycle_T_htf_hot_des =
		m_cycle_P_hot_des = m_cycle_x_hot_des = 
		m_m_dot_pc_des = m_m_dot_pc_min =
        m_m_dot_pc_max = m_m_dot_pc_max_startup =
        m_W_dot_bop_design = m_W_dot_fixed_design = m_T_htf_pc_cold_est = std::numeric_limits<double>::quiet_NaN();

    m_is_cr_config_recirc = true;

	// Reporting and Output Tracking
	mc_reported_outputs.construct(S_solver_output_info);

	m_i_reporting = -1;
	//m_sim_time_start = 
	//m_sim_time_end = 
	//m_sim_step_size_baseline =
	m_report_time_start = m_report_time_end = m_report_step = std::numeric_limits<double>::quiet_NaN();

	m_step_tolerance = 10.0;		//[s] For adjustable timesteps, if within 10 seconds, assume it equals baseline timestep

	m_op_mode_tracking.resize(0);

	error_msg = "";

	mv_time_local.reserve(10);

	

	// Solved Controller Variables
	m_defocus = std::numeric_limits<double>::quiet_NaN();
    m_q_dot_pc_max = std::numeric_limits<double>::quiet_NaN();  //[MWt]
}

void C_csp_solver::send_callback(double percent)
{
	if (mpf_callback && mp_cmod_active)
	{
		int out_type = 1;
		std::string out_msg = "";
		std::string prg_msg = "Simulation Progress";

		while (mc_csp_messages.get_message(&out_type, &out_msg))
		{
			mpf_callback(out_msg, prg_msg, mp_cmod_active, percent, out_type);
		}

		out_msg = "";
		bool cmod_ret = mpf_callback(out_msg, prg_msg, mp_cmod_active, percent, out_type);

		if (!cmod_ret)
		{
			std::string error_msg = "User terminated simulation...";
			std::string loc_msg = "C_csp_solver";
			throw(C_csp_exception(error_msg, loc_msg, 1));
		}
	}
}

double C_csp_solver::get_cr_aperture_area()
{
	return m_A_aperture;	//[m2]
}

void C_csp_solver::init()
{
	// First, initialize each component and update solver-level membe data as necessary
		// Weather reader
	mc_weather.init();
		// Collector-receiver
	C_csp_collector_receiver::S_csp_cr_init_inputs init_inputs;
	init_inputs.m_latitude = mc_weather.ms_solved_params.m_lat;		//[deg]
	init_inputs.m_longitude = mc_weather.ms_solved_params.m_lon;	//[deg]
    init_inputs.m_tz = mc_weather.ms_solved_params.m_tz;	    	//[hr]
	init_inputs.m_shift = mc_weather.ms_solved_params.m_shift;		//[deg]
    init_inputs.m_elev = mc_weather.ms_solved_params.m_elev;		//[m]
	C_csp_collector_receiver::S_csp_cr_solved_params cr_solved_params;
	
	mc_collector_receiver.init(init_inputs, cr_solved_params);
	mc_csp_messages.transfer_messages(mc_collector_receiver.mc_csp_messages);
	
	m_T_htf_cold_des = cr_solved_params.m_T_htf_cold_des;		//[K]
	m_P_cold_des = cr_solved_params.m_P_cold_des;				//[kPa]
	m_x_cold_des = cr_solved_params.m_x_cold_des;				//[-]
	m_q_dot_rec_des = cr_solved_params.m_q_dot_rec_des;			//[MW]
	m_A_aperture = cr_solved_params.m_A_aper_total;				//[m2]

        // Parallel Heater
    if (m_is_parallel_heater) {
        C_csp_collector_receiver::S_csp_cr_solved_params par_htr_solved_params;

        mp_heater->init(init_inputs, par_htr_solved_params);
        mc_csp_messages.transfer_messages(mp_heater->mc_csp_messages);

        m_PAR_HTR_T_htf_cold_des = par_htr_solved_params.m_T_htf_cold_des;      //[K]
        m_PAR_HTR_P_cold_des = par_htr_solved_params.m_P_cold_des;              //[kPa]
        m_PAR_HTR_x_cold_des = par_htr_solved_params.m_x_cold_des;              //[-]
        m_PAR_HTR_q_dot_rec_des = par_htr_solved_params.m_q_dot_rec_des;        //[MWt]
        m_PAR_HTR_A_aperture = par_htr_solved_params.m_A_aper_total;            //[m2]
    }
    else {
        m_PAR_HTR_q_dot_rec_des = 0.0;
    }

		// Power cycle
	C_csp_power_cycle::S_solved_params pc_solved_params;
	mc_power_cycle.init(pc_solved_params);
    mc_csp_messages.transfer_messages(mc_power_cycle.mc_csp_messages);
	m_cycle_W_dot_des = pc_solved_params.m_W_dot_des;					//[MW]
	m_cycle_eta_des = pc_solved_params.m_eta_des;						//[-]
	m_cycle_q_dot_des = pc_solved_params.m_q_dot_des;					//[MW]
	m_cycle_max_frac = pc_solved_params.m_max_frac;						//[-]
	m_cycle_cutoff_frac = pc_solved_params.m_cutoff_frac;				//[-]
	m_cycle_sb_frac_des = pc_solved_params.m_sb_frac;					//[-]
	m_cycle_T_htf_hot_des = pc_solved_params.m_T_htf_hot_ref + 273.15;	//[K] convert from C
	m_m_dot_pc_des = pc_solved_params.m_m_dot_design;					//[kg/hr]
				
	m_m_dot_pc_min = 0.0 * pc_solved_params.m_m_dot_min;		//[kg/hr]
	m_m_dot_pc_max_startup = pc_solved_params.m_m_dot_max;		//[kg/hr]				
	
	m_cycle_P_hot_des = pc_solved_params.m_P_hot_des;					//[kPa]
	m_cycle_x_hot_des = pc_solved_params.m_x_hot_des;					//[-]
		// TES
    C_csp_tes::S_csp_tes_init_inputs tes_init_inputs;
    tes_init_inputs.T_to_cr_at_des = cr_solved_params.m_T_htf_cold_des;
    tes_init_inputs.T_from_cr_at_des = cr_solved_params.m_T_htf_hot_des;
    tes_init_inputs.P_to_cr_at_des = cr_solved_params.m_dP_sf;
	mc_tes.init(tes_init_inputs);
    mc_csp_messages.transfer_messages(mc_tes.mc_csp_messages);
        // Check Cold TES
    if (mc_CT_tes.get() != nullptr) {
        m_is_CT_tes = true;

        C_csp_tes::S_csp_tes_init_inputs CT_tes_init_inputs;
        mc_CT_tes->init(CT_tes_init_inputs);
        mc_csp_messages.transfer_messages(mc_CT_tes->mc_csp_messages);

        m_CT_to_HT_m_dot_ratio = cr_solved_params.m_CT_to_HT_m_dot_ratio;   //[-]
    }
    else {
        m_is_CT_tes = false;
    }
		// TOU
	mc_tou.init(mc_weather.ms_solved_params.m_leapyear);
		// Thermal Storage
	m_is_tes = mc_tes.does_tes_exist();
    bool m_does_tes_enable_cr_to_cold_tank = mc_tes.is_cr_to_cold_allowed();

        // System control logic
    m_is_rec_to_coldtank_allowed = ms_system_params.m_is_rec_to_coldtank_allowed;
    m_T_htf_hot_tank_in_min = (ms_system_params.f_htf_hot_des__T_htf_hot_tank_in_min * cr_solved_params.m_T_htf_cold_des +
        (1.0 - ms_system_params.f_htf_hot_des__T_htf_hot_tank_in_min) * cr_solved_params.m_T_htf_hot_des) - 273.15;     //[C] convert from K
    //m_T_htf_hot_tank_in_min = (0.5 * cr_solved_params.m_T_htf_cold_des + 0.5 * cr_solved_params.m_T_htf_hot_des) - 273.15;  //[C] convert from K

    // Can't send HTF outlet to cold tank if no cold tank
    // or if TES class isn't configured to do so (parallel tanks in two-tank tes can't at the moment)
    // or if hot tank in min is nan or "too hot"
    m_is_rec_to_coldtank_allowed = m_is_rec_to_coldtank_allowed && m_is_tes &&
                                    m_does_tes_enable_cr_to_cold_tank &&
                                    std::isfinite(m_T_htf_hot_tank_in_min) &&
                                    m_T_htf_hot_tank_in_min < (m_cycle_T_htf_hot_des);

    m_is_cr_config_recirc = true;

    if (mc_tou.m_dispatch_model_type == C_csp_tou::C_dispatch_model_type::E_dispatch_model_type::UNDEFINED) {
        throw(C_csp_exception("Either heuristic, imported dispatch targets, or dispatch optimization must be specified", "CSP Solver"));
    }

    if (mc_dispatch.solver_params.dispatch_optimize)
    {
        mc_dispatch.pointers.set_pointers(mc_weather, &mc_collector_receiver, &mc_power_cycle, &mc_tes, &mc_csp_messages, &mc_kernel.mc_sim_info, mp_heater);
        mc_dispatch.init(m_cycle_q_dot_des, m_cycle_eta_des);
    }

    // Value helps solver get out of T_field_htf_cold iteration when weird conditions cause the solution to be a very cold value
    // Should update with technology-specific htf freeze protection values
    m_T_field_cold_limit = -100.0;      //[C]
    m_T_field_in_hot_limit = (0.9*m_cycle_T_htf_hot_des + 0.1*m_T_htf_cold_des) - 273.15;   //[C]

    double W_dot_ratio_des = 1.0;       //[-]
    m_W_dot_bop_design = m_cycle_W_dot_des * ms_system_params.m_bop_par * ms_system_params.m_bop_par_f *
        (ms_system_params.m_bop_par_0 + ms_system_params.m_bop_par_1 * W_dot_ratio_des + ms_system_params.m_bop_par_2 * pow(W_dot_ratio_des, 2));   //[MWe]

    m_W_dot_fixed_design = ms_system_params.m_pb_fixed_par* m_cycle_W_dot_des;			//[MWe]

	if( mc_collector_receiver.m_is_sensible_htf != mc_power_cycle.m_is_sensible_htf )
	{
		throw(C_csp_exception("The collector-receiver and power cycle models have incompatible HTF - direct/indirect assumptions", "CSP Solver"));
	}

    if (!mc_collector_receiver.m_is_sensible_htf && m_is_parallel_heater) {
        throw(C_csp_exception("Model does not allow parallel heater with latent heat receivers", "CSP Solver"));
    }

    if (m_is_parallel_heater && m_is_rec_to_coldtank_allowed) {
        throw(C_csp_exception("Model does not allow parallel heater when receiver is configured to send HTF to cold tank", "CSP Solver"));
    }

    /* 
    If no TES exists, initialize values to zero. They won't be touched again
    */

	if(!m_is_tes)
	{	// Set constant values for tes HTF states

		mc_tes_outputs.m_q_heater = 0.0;		//[MW]
		mc_tes_outputs.m_q_dot_dc_to_htf = 0.0;	//[MW]
		mc_tes_outputs.m_q_dot_ch_from_htf = 0.0;	//[MW]
		
		mc_tes_outputs.m_m_dot_cr_to_tes_hot = 0.0;		//[kg/s]
		mc_tes_outputs.m_m_dot_tes_hot_out = 0.0;		//[kg/s]
		mc_tes_outputs.m_m_dot_pc_to_tes_cold = 0.0;	//[kg/s]
		mc_tes_outputs.m_m_dot_tes_cold_out = 0.0;		//[kg/s]
		mc_tes_outputs.m_m_dot_src_to_sink = 0.0;	    //[kg/s]
		mc_tes_outputs.m_m_dot_sink_to_src = 0.0;	    //[kg/s]

		mc_tes_outputs.m_m_dot_cold_tank_to_hot_tank = 0.0;
	}
}

void C_csp_solver::get_design_parameters(double& W_dot_bop_design /*MWe*/,
                                    double& W_dot_fixed_design /*MWe*/)
{
    W_dot_bop_design = m_W_dot_bop_design;      //[MWe]
    W_dot_fixed_design = m_W_dot_fixed_design;  //[MWe]
}

int C_csp_solver::steps_per_hour()
{
	// Get number of records in weather file
	int n_wf_records = (int)mc_weather.m_weather_data_provider->nrecords();
	int step_per_hour = n_wf_records / 8760;
	return step_per_hour;
}

void C_csp_solver::Ssimulate(C_csp_solver::S_sim_setup & sim_setup)
{
	// Get number of records in weather file
	int n_wf_records = (int)mc_weather.m_weather_data_provider->nrecords();
	int step_per_hour = n_wf_records / 8760;    // TODO: this is in multiple places (here and dispatch (moved over from tou))

	double wf_step = 3600.0 / step_per_hour;	//[s] Weather file time step - would like to check this against weather file, some day
	
    m_is_first_timestep = true;
	double baseline_step = wf_step;		//[s] Baseline timestep of the simulation - this should probably be technology/model specific
	// Check the collector-receiver model for a maximum step
	if(mc_collector_receiver.m_max_step > 0.0)
	{
		baseline_step = std::max(m_step_tolerance, std::min(baseline_step, mc_collector_receiver.m_max_step));
	}
	
	mc_kernel.init(sim_setup, wf_step, baseline_step, mc_csp_messages);
    
	double tol_mode_switching = 0.10;		// Give buffer to account for uncertainty in estimates

	// Reset vector that tracks operating modes
	m_op_mode_tracking.resize(0);

	// Reset Controller Variables to Defaults
	m_defocus = 1.0;		//[-]  

	m_i_reporting = 0;
	m_report_time_start = mc_kernel.get_sim_setup()->m_sim_time_start;			//[s]
	m_report_step = sim_setup.m_report_step;		//[s]
	m_report_time_end = m_report_time_start + m_report_step;	//[s]

	double progress_msg_interval_frac = 0.02;
	double progress_msg_frac_current = progress_msg_interval_frac;
	double V_hot_tank_frac_initial;

    double pc_heat_prev = 0.;   // [MWt] Heat into power cycle in previous time step
	double pc_state_persist = 0.;  // Time [hr] that current pc operating state (on/off/standby) has persisted
	double rec_state_persist = 0.;  // Time [hr] that current receiver operating state (on/off/standby) has persisted
	//int prev_pc_state = mc_power_cycle.get_operating_state();
	//int prev_rec_state = mc_collector_receiver.get_operating_state();

	double q_pb_last = 0.0;   // Cycle thermal input at end of last time step [kWt]
	double w_pb_last = 0.0;   // Cycle gross generation at end of last time step [kWt]
	double f_op_last = 0.0;	  // Fraction of last time step that cycle was operating or in standby

    //************************** MAIN TIME-SERIES LOOP **************************
	// Block dispatch saved variables
	bool is_q_dot_pc_target_overwrite = false;

	//mf_callback(m_cdata, 0.0, 0, 0.0);

    double start_time = mc_kernel.get_sim_setup()->m_sim_time_start;
    if( start_time != 0. )
        mc_csp_messages.add_message(C_csp_messages::WARNING, util::format("Start time: %f", start_time) );

    double end_time = mc_kernel.get_sim_setup()->m_sim_time_end;
    if(end_time != 8760*3600.)
        mc_csp_messages.add_message(C_csp_messages::WARNING, util::format("End time: %f", end_time) );

    C_system_operating_modes::E_operating_modes operating_mode = C_system_operating_modes::CR_OFF__PC_OFF__TES_OFF__AUX_OFF;

	while( mc_kernel.mc_sim_info.ms_ts.m_time <= mc_kernel.get_sim_setup()->m_sim_time_end )
	{
		// Report simulation progress
		double calc_frac_current = (mc_kernel.mc_sim_info.ms_ts.m_time - mc_kernel.get_sim_setup()->m_sim_time_start) / (mc_kernel.get_sim_setup()->m_sim_time_end - mc_kernel.get_sim_setup()->m_sim_time_start);
		if( calc_frac_current > progress_msg_frac_current )
		{
			send_callback( (float)calc_frac_current*100.f );

			progress_msg_frac_current += progress_msg_interval_frac;
		}
		
		// Get tou for timestep
		mc_tou.call(mc_kernel.mc_sim_info.ms_ts.m_time, mc_tou_outputs);
		size_t f_turb_tou_period = mc_tou_outputs.m_csp_op_tou;	//[-]
        size_t pricing_tou_period = mc_tou_outputs.m_pricing_tou;   //[-]
        mc_kernel.mc_sim_info.m_tou = f_turb_tou_period;	    //[base 1] used ONLY by power cycle model for hybrid cooling - may also want to move this to controller
        double f_turbine_tou = mc_tou_outputs.m_f_turbine;	//[-]
		double pricing_mult = mc_tou_outputs.m_price_mult;	//[-]
        double purchase_mult = pricing_mult;
        //if (!mc_tou.mc_dispatch_params.m_is_purchase_mult_same_as_price) {
        //    throw(C_csp_exception("CSP Solver not yet setup to handle purchase schedule separate from price schedule"));
        //}

		// Get collector/receiver & power cycle operating states at start of time step (end of last time step)
            // collector/receiver
        C_csp_collector_receiver::E_csp_cr_modes cr_operating_state_prev = mc_collector_receiver.get_operating_state();
		if( cr_operating_state_prev < C_csp_collector_receiver::OFF ||
			cr_operating_state_prev > C_csp_collector_receiver::ON )
		{
			std::string msg = util::format("The collector-receiver operating state at time %lg [hr] is %d. Recognized"
				" values are from %d to %d\n", mc_kernel.mc_sim_info.ms_ts.m_step/ 3600.0, cr_operating_state_prev, C_csp_collector_receiver::OFF, C_csp_collector_receiver::ON);
			throw(C_csp_exception(msg,"CSP Solver Core"));
		}
        C_csp_collector_receiver::E_csp_cr_modes cr_operating_state_to_controller = cr_operating_state_prev;
        // If component is off but does not require startup to switch to on,
        // Then for the purposed of the controller hierarchy, the component is on
        if (cr_operating_state_to_controller == C_csp_collector_receiver::OFF_NO_SU_REQ) {
            cr_operating_state_to_controller = C_csp_collector_receiver::ON;
        }

            // power cycle
        C_csp_power_cycle::E_csp_power_cycle_modes pc_operating_state_prev = mc_power_cycle.get_operating_state();
        C_csp_power_cycle::E_csp_power_cycle_modes pc_operating_state_to_controller = pc_operating_state_prev;
        // If component is off but does not require startup to switch to on,
        // Then for the purposed of the controller hierarchy, the component is on
        if (pc_operating_state_to_controller == C_csp_power_cycle::OFF_NO_SU_REQ) {
            pc_operating_state_to_controller = C_csp_power_cycle::ON;
        }

		double q_pb_last = mc_pc_out_solver.m_q_dot_htf * 1000.; //[kWt]
		double w_pb_last = mc_pc_out_solver.m_P_cycle * 1000.;   //[kWt]

		// Calculate maximum thermal power to power cycle for startup. This will be zero if power cycle is on.
		double q_dot_pc_su_max = mc_power_cycle.get_max_q_pc_startup();		//[MWt]

		// Get weather at this timestep. Should only be called once per timestep. (Except converged() function)
        mc_weather.timestep_call(mc_kernel.mc_sim_info);

		// Get volume of hot tank, for debugging
		V_hot_tank_frac_initial = mc_tes.get_hot_tank_vol_frac();


        // Get max HTF mass flow rate to the cycle as a function of ambient temperature
		double m_dot_htf_ND_max = std::numeric_limits<double>::quiet_NaN();
		double W_dot_ND_max = std::numeric_limits<double>::quiet_NaN();
		mc_power_cycle.get_max_power_output_operation_constraints(mc_weather.ms_outputs.m_tdry, m_dot_htf_ND_max, W_dot_ND_max);
		m_m_dot_pc_max = m_dot_htf_ND_max * m_m_dot_pc_des;

		// Then call power cycle at ambient temperature and min(des_m_dot, max_m_dot) to get a guess of HTF return temperature
		mc_pc_htf_state_in.m_temp = m_cycle_T_htf_hot_des - 273.15; //[C]
		mc_pc_htf_state_in.m_pres = m_cycle_P_hot_des;	//[kPa]
		mc_pc_htf_state_in.m_qual = m_cycle_x_hot_des;	//[-]
		mc_pc_inputs.m_m_dot = (std::min)(m_m_dot_pc_max, m_m_dot_pc_des);				//[kg/hr]
		// Inputs
		mc_pc_inputs.m_standby_control = C_csp_power_cycle::ON;
		//mc_pc_inputs.m_tou = tou_timestep;
		// Performance Call

        double T_CT_htf_cold_in = std::numeric_limits<double>::quiet_NaN();
        if (m_is_CT_tes) {
            T_CT_htf_cold_in = mc_CT_tes->get_cold_temp() - 273.15;    //[C] convert from K
        }

		mc_power_cycle.call(mc_weather.ms_outputs,
			mc_pc_htf_state_in,
            T_CT_htf_cold_in,
			mc_pc_inputs,
			mc_pc_out_solver,
			mc_kernel.mc_sim_info);

        // Next, estimate receiver performance using estimated power cycle performance
        // If the return temperature is hotter than design, then the mass flow from the receiver will be bigger than expected
        bool is_rec_outlet_to_hottank = true;
		m_T_htf_pc_cold_est = mc_pc_out_solver.m_T_htf_cold;	//[C]
		// Solve collector/receiver at steady state with design inputs and weather to estimate output
		mc_cr_htf_state_in.m_temp = m_T_htf_pc_cold_est;	//[C]
		C_csp_collector_receiver::S_csp_cr_est_out est_out;
		mc_collector_receiver.estimates(mc_weather.ms_outputs,
			mc_cr_htf_state_in,
			est_out,
			mc_kernel.mc_sim_info);
		double q_dot_cr_startup = est_out.m_q_startup_avail;    //[MWt]
		double q_dot_cr_on = est_out.m_q_dot_avail;     //[MWt]
		double m_dot_cr_on = est_out.m_m_dot_avail;		//[kg/hr]
		double T_htf_hot_cr_on = est_out.m_T_htf_hot;	//[C]
		if (cr_operating_state_to_controller != C_csp_collector_receiver::ON)
			T_htf_hot_cr_on = m_cycle_T_htf_hot_des - 273.15;	//[C]

        // Is receiver on and will it likely remain on
        if (cr_operating_state_to_controller == C_csp_collector_receiver::ON && m_dot_cr_on > 0.0
            && m_is_rec_to_coldtank_allowed
            && T_htf_hot_cr_on < m_T_htf_hot_tank_in_min) {
            is_rec_outlet_to_hottank = false;
        }

        // If parallel heater, estimate performance
        double q_dot_PAR_HTR_on = std::numeric_limits<double>::quiet_NaN(); //[MWt]
        if (m_is_parallel_heater) {
            C_csp_collector_receiver::S_csp_cr_est_out par_htr_est_out;
            mp_heater->estimates(mc_weather.ms_outputs,
                mc_cr_htf_state_in,
                par_htr_est_out,
                mc_kernel.mc_sim_info);

            q_dot_PAR_HTR_on = par_htr_est_out.m_q_dot_avail;   //[MWt]
        }

		// Get TES operating state info at end of last time step
		double q_dot_tes_dc, q_dot_tes_ch;      //[MWt]
		q_dot_tes_dc = q_dot_tes_ch = std::numeric_limits<double>::quiet_NaN();
		double m_dot_tes_dc_est, m_dot_tes_ch_est;
		if (m_is_tes)
		{
			//predict estimated amount of charge/discharge available
			double T_hot_field_dc_est;	//[K]
			T_hot_field_dc_est = std::numeric_limits<double>::quiet_NaN();
			mc_tes.discharge_avail_est(m_T_htf_pc_cold_est + 273.15, mc_kernel.mc_sim_info.ms_ts.m_step, q_dot_tes_dc, m_dot_tes_dc_est, T_hot_field_dc_est);
			m_dot_tes_dc_est *= 3600.0;	//[kg/hr] convert from kg/s

			double T_cold_field_ch_est;	//[K]
			T_cold_field_ch_est = std::numeric_limits<double>::quiet_NaN();
			mc_tes.charge_avail_est(T_htf_hot_cr_on + 273.15, mc_kernel.mc_sim_info.ms_ts.m_step, q_dot_tes_ch, m_dot_tes_ch_est, T_cold_field_ch_est);
			m_dot_tes_ch_est *= 3600.0;	//[kg/hr] convert from kg/s
		}
		else
		{
			q_dot_tes_dc = q_dot_tes_ch = 0.0;
			m_dot_tes_dc_est = m_dot_tes_ch_est = 0.0;
		}

        // Check that q_dot_tes_ch is not "too close" to 0
        if (q_dot_tes_ch < std::max(m_PAR_HTR_q_dot_rec_des, m_q_dot_rec_des) * 1.E-4) {
            q_dot_tes_ch = 0.0;
        }

        // Check that there is enough discharge energy to operate cycle for a 'reasonable' fraction of the timestep
        double t_q_dot_min = std::max(0.05*mc_kernel.mc_sim_info.ms_ts.m_step, m_step_tolerance);   //[s]
        if (q_dot_tes_dc * mc_kernel.mc_sim_info.ms_ts.m_step < m_cycle_q_dot_des * t_q_dot_min)
        {
            q_dot_tes_dc = 0.0;     //[s
        }
		// Can add the following code to simulate with no storage charge/discharge, but IDLE calcs
		//q_dot_tes_dc = q_dot_tes_ch = 0.0;

        // Get standby fraction and min operating fraction
            // Could eventually be a method in PC class...
        double cycle_sb_frac = m_cycle_sb_frac_des;				//[-]

        // *** If standby not allowed, then reset q_pc_sb = q_pc_min ?? *** 
            //or is this too confusing and not helpful enough?
        double q_pc_sb = cycle_sb_frac * m_cycle_q_dot_des;		//[MW]
        double q_pc_min = m_cycle_cutoff_frac * m_cycle_q_dot_des;	//[MW]

        // Initialize to NaN - block or dispatch needs to set
        double q_pc_target = std::numeric_limits<double>::quiet_NaN();
        m_q_dot_pc_max = q_pc_target;

        // Get or set decision variables
        bool is_rec_su_allowed = false;
        bool is_pc_su_allowed = false;
        bool is_pc_sb_allowed = false;
        bool is_PAR_HTR_allowed = false;

        double q_dot_elec_to_CR_heat = std::numeric_limits<double>::quiet_NaN();    //[MWt]
        double q_dot_pc_max = std::numeric_limits<double>::quiet_NaN();     //[MWt]
        double q_dot_elec_to_PAR_HTR = std::numeric_limits<double>::quiet_NaN();

        calc_timestep_plant_control_and_targets(
            f_turbine_tou, q_pc_min, q_dot_tes_ch, pc_heat_prev, pc_state_persist,
            pc_operating_state_to_controller, purchase_mult, pricing_mult,
            calc_frac_current, baseline_step,
            is_q_dot_pc_target_overwrite,
            q_pc_target, q_dot_pc_max, q_dot_elec_to_CR_heat,
            is_rec_su_allowed, is_pc_su_allowed, is_pc_sb_allowed,
            q_dot_elec_to_PAR_HTR, is_PAR_HTR_allowed);

        // Avoid setting member data in method, so set here
        m_q_dot_pc_max = q_dot_pc_max;

        // Split up reported q_dot_pc target into 'startup' and 'on' so input dispatch can specify both for a single full timestep
        double q_dot_pc_su_target_reporting = 0.0;
        double q_dot_pc_on_target_reporting = 0.0;
        if (pc_operating_state_to_controller == C_csp_power_cycle::OFF || pc_operating_state_to_controller == C_csp_power_cycle::STARTUP) {
            q_dot_pc_su_target_reporting = q_pc_target;
        }
        else {
            q_dot_pc_on_target_reporting = q_pc_target;
        }

        //------------ Controller/Solver iteration loop -------------

		bool are_models_converged = false;

        // Reset operating mode availability
        mc_operating_modes.reset_all_availability();

        // Reset operating mode tracker		
		m_op_mode_tracking.resize(0);
					
		// Check if CR startup should be solved before entering hierarchy
		double q_dot_tes_dc_t_CR_su = 0.0;
		double m_dot_tes_dc_t_CR_su = 0.0;
		if( (cr_operating_state_to_controller == C_csp_collector_receiver::OFF || cr_operating_state_to_controller == C_csp_collector_receiver::STARTUP) &&
			q_dot_cr_startup > 0.0 &&
			is_rec_su_allowed && 
			m_is_tes )
		{
			// Set startup conditions
			mc_cr_htf_state_in.m_temp = m_T_htf_cold_des - 273.15;		//[C], convert from [K]

			mc_collector_receiver.startup(mc_weather.ms_outputs,
				mc_cr_htf_state_in,
				mc_cr_out_solver,
				mc_kernel.mc_sim_info);

			// Check that startup happened
			// Because for all modes w/ startup, the startup occurs under the same conditions for any given timestep
			// then if startup fails here, it won't succeed downstream
			// so set is_rec_su_allowed = false
			if( mc_cr_out_solver.m_q_startup == 0.0 || mc_cr_out_solver.m_time_required_su != mc_cr_out_solver.m_time_required_su )
			{	// Collector/receiver can't produce useful energy
				
				is_rec_su_allowed = false;
			}
			else
			{
				double t_CR_su = mc_cr_out_solver.m_time_required_su;		//[s] Receiver model returns MIN(time required to completely startup, full timestep duration)

				// Use minimum of CR startup timestep and initial simulation timestep
				t_CR_su = std::min(t_CR_su, mc_kernel.mc_sim_info.ms_ts.m_step);			//[s]

				// Predict estimated amount of discharage available with new timestep
				if( m_is_tes )
				{
					double T_hot_field_dc_est;	//[kg/s, K]
					T_hot_field_dc_est = std::numeric_limits<double>::quiet_NaN();
					mc_tes.discharge_avail_est(m_T_htf_cold_des, t_CR_su, q_dot_tes_dc_t_CR_su, m_dot_tes_dc_t_CR_su, T_hot_field_dc_est);
					m_dot_tes_dc_t_CR_su *= 3600.0;		//[kg/hr] convert from kg/s
				}
				else
				{
					q_dot_tes_dc_t_CR_su = 0.0;
					m_dot_tes_dc_t_CR_su = 0.0;
				}
			} 
		}

		// Check if receiver can be defocused enough to stay under cycle+TES max thermal power and mass flow (if cold recirculation is not enabled)
        // (this will usually be the case unless using clear-sky control or constrained cycle thermal input)
		if (cr_operating_state_to_controller == C_csp_collector_receiver::ON && (q_dot_cr_on >0.0 || m_dot_cr_on > 0.0) && is_rec_su_allowed && is_rec_outlet_to_hottank && m_is_tes)
		{
			double qpcmax = m_q_dot_pc_max;
			if (pc_operating_state_to_controller == C_csp_power_cycle::OFF || C_csp_power_cycle::STARTUP)
				qpcmax = q_dot_pc_su_max;

            double qmax = q_dot_tes_ch / (1.0 - tol_mode_switching);
            double mmax = m_dot_tes_ch_est / (1.0 - tol_mode_switching);
            if (is_pc_su_allowed)
            {
                qmax += m_q_dot_pc_max / (1.0 - tol_mode_switching);
                mmax += m_m_dot_pc_max / (1.0 - tol_mode_switching);
            }

            if (q_dot_cr_on > qmax || m_dot_cr_on > mmax)  // Receiver will need to be defocused
            {
                double df = std::min(qmax / q_dot_cr_on, mmax / m_dot_cr_on);
                if (q_dot_elec_to_CR_heat > 0. && !m_is_parallel_heater) //Heater is on and not the CSP+ETES case
                {
                    q_dot_elec_to_CR_heat = q_dot_cr_on;  // Setting to the max and allowing controller to defocus
                }

                double T_CT_htf_hot_in = std::numeric_limits<double>::quiet_NaN();
                if (m_is_CT_tes) {
                    T_CT_htf_hot_in = mc_CT_tes->get_hot_temp() - 273.15;    //[C] convert from K
                }

                mc_collector_receiver.on(mc_weather.ms_outputs, mc_cr_htf_state_in, T_CT_htf_hot_in, q_dot_elec_to_CR_heat, df, mc_cr_out_solver, mc_kernel.mc_sim_info);
                if (mc_cr_out_solver.m_q_thermal == 0.0)  // Receiver solution wasn't successful 
                    is_rec_su_allowed = false;
            }

		}

		while(!are_models_converged)		// Solve for correct operating mode and performance in following loop:
		{
			// Reset timestep info for iterations on the operating mode...
			mc_kernel.mc_sim_info.ms_ts.m_time = mc_kernel.get_baseline_end_time();
			mc_kernel.mc_sim_info.ms_ts.m_step = mc_kernel.mc_sim_info.ms_ts.m_time - mc_kernel.mc_sim_info.ms_ts.m_time_start;

            operating_mode = mc_operating_modes.find_operating_mode(
                cr_operating_state_to_controller, pc_operating_state_to_controller,
                q_dot_cr_startup /*MWt*/, q_dot_tes_dc /*MWt*/,
                q_dot_cr_on /*MWt*/, q_dot_tes_ch /*MWt*/,
                q_dot_pc_su_max /*MWt*/, q_pc_target /*MWt*/,
                q_dot_tes_dc_t_CR_su /*MWt*/, q_pc_min /*MWt*/,
                q_pc_sb /*MWt*/, q_dot_pc_max,
                m_dot_cr_on /*kg/s*/, m_dot_tes_ch_est /*kg/s*/,
                m_m_dot_pc_max /*kg/s*/, m_dot_tes_dc_t_CR_su /*kg/s*/,
                m_m_dot_pc_min /*kg/s*/, m_dot_tes_dc_est /*kg/s*/,
                tol_mode_switching /*-*/,
                is_rec_su_allowed, is_pc_su_allowed,
                is_rec_outlet_to_hottank, is_pc_sb_allowed,
                q_dot_PAR_HTR_on, is_PAR_HTR_allowed);

			// Store operating mode
			m_op_mode_tracking.push_back((int)operating_mode);

            double t_ts_initial = mc_kernel.mc_sim_info.ms_ts.m_step;   //[s]
            double defocus_solved = std::numeric_limits<double>::quiet_NaN();

            bool is_op_mode_avail = true;
            bool is_turn_off_plant = false;
            bool is_turn_off_rec_su = false;

            are_models_converged = mc_operating_modes.solve(operating_mode, this, is_rec_outlet_to_hottank,
                q_pc_target, q_dot_pc_su_max, q_pc_sb,
                q_pc_min, m_q_dot_pc_max, q_dot_pc_su_max,
                m_m_dot_pc_max_startup, m_m_dot_pc_max, m_m_dot_pc_min,
                q_dot_elec_to_CR_heat, q_dot_elec_to_PAR_HTR, 1.E-3,
                defocus_solved, is_op_mode_avail, is_turn_off_plant, is_turn_off_rec_su);
            if (is_turn_off_rec_su) {
                is_rec_su_allowed = false;
            }

            if (!are_models_converged) {
                reset_time(t_ts_initial);
                if (is_turn_off_plant) {
                    mc_operating_modes.turn_off_plant();
                }
            }
            m_defocus = defocus_solved;
		
		}	        
        /* 
        ------------ End loop to find correct operating mode and system performance --------
        */

		// Calculate system-level parasitics: can happen after controller/solver converges
		//double W_dot_fixed = ms_system_params.m_pb_fixed_par*m_cycle_W_dot_des;			//[MWe]

		double W_dot_ratio = mc_pc_out_solver.m_P_cycle / std::max(0.001, m_cycle_W_dot_des);		//[-]

		double W_dot_bop = m_cycle_W_dot_des*ms_system_params.m_bop_par*ms_system_params.m_bop_par_f *
			(ms_system_params.m_bop_par_0 + ms_system_params.m_bop_par_1*W_dot_ratio + ms_system_params.m_bop_par_2*pow(W_dot_ratio,2));
			// [MWe]

        double W_dot_cr_freeze_protection = 0.0;
        if (ms_system_params.m_is_field_freeze_protection_electric) {
            W_dot_cr_freeze_protection = mc_cr_out_solver.m_q_dot_heater;       //[MWe]
        }

        double W_dot_tes_pump = 0.0;        //[MWe]
        if (m_is_tes) {
            W_dot_tes_pump = mc_tes_outputs.m_W_dot_elec_in_tot;    //[MWe]
        }

        double W_dot_par_htr_elec_load = 0.0;
        if (m_is_parallel_heater) {
            W_dot_par_htr_elec_load = mc_par_htr_out_solver.m_W_dot_elec_in_tot +
                                    mc_par_htr_out_solver.m_q_dot_heater;       //[MWe]
        }

        double W_dot_net = mc_pc_out_solver.m_P_cycle -
            mc_cr_out_solver.m_W_dot_elec_in_tot -
            mc_pc_out_solver.m_W_dot_elec_parasitics_tot -
            W_dot_tes_pump -
			W_dot_cr_freeze_protection -
            W_dot_par_htr_elec_load -
			mc_tes_outputs.m_q_heater - 
			m_W_dot_fixed_design -
			W_dot_bop;	//[MWe]


        // Timestep solved: run post-processing, converged()		
		mc_collector_receiver.converged();
		mc_power_cycle.converged();
		mc_tes.converged();
        if (m_is_parallel_heater) {
            mp_heater->converged();
        }
        if (m_is_CT_tes) {
            mc_CT_tes->converged();
        }
		
        //Update the estimated thermal energy storage charge state
        double e_tes_disch = 0.;
		double mhot_avail = 0.;
		double mcold_avail = 0.;
        if(m_is_tes)
        {
            double mdot_disch, Tdisch;
			mc_tes.discharge_avail_est(m_T_htf_cold_des, mc_kernel.mc_sim_info.ms_ts.m_step, e_tes_disch, mdot_disch, Tdisch);

            e_tes_disch *= mc_kernel.mc_sim_info.ms_ts.m_step / 3600.;  //MWh
			mhot_avail = mdot_disch * mc_kernel.mc_sim_info.ms_ts.m_step;  //kg

			double e_tes_ch, mdot_ch, Tch;
			mc_tes.charge_avail_est(m_cycle_T_htf_hot_des, mc_kernel.mc_sim_info.ms_ts.m_step, e_tes_ch, mdot_ch, Tch);
			mcold_avail = mdot_ch * mc_kernel.mc_sim_info.ms_ts.m_step;  //kg
        }

        pc_heat_prev = mc_pc_out_solver.m_q_dot_htf;

		// Update the cycle state persistance
		if (mc_power_cycle.get_operating_state() == pc_operating_state_prev)
			pc_state_persist += mc_kernel.mc_sim_info.ms_ts.m_step / 3600.; //[hr]
		else
		{
			pc_state_persist = 0.;
		}

		// Update the receiver state persistance
		if (mc_collector_receiver.get_operating_state() == cr_operating_state_prev)
			rec_state_persist += mc_kernel.mc_sim_info.ms_ts.m_step / 3600.;
		else
		{
			rec_state_persist = 0.;
		}

		// Save timestep outputs
		// This is after timestep convergence, so be sure convergence() methods don't unexpectedly change outputs
		
			// Simulation outputs
		mv_time_local.push_back(mc_kernel.mc_sim_info.ms_ts.m_time);
		mc_reported_outputs.value(C_solver_outputs::TIME_FINAL, mc_kernel.mc_sim_info.ms_ts.m_time/3600.0);
		mc_reported_outputs.value(C_solver_outputs::MONTH, mc_weather.ms_outputs.m_month);	//[-]
		mc_reported_outputs.value(C_solver_outputs::HOUR_DAY, (int)(m_report_time_end/3600) % 24);	//[hr]


		int n_sub_ts = (int)mv_time_local.size();
        mc_reported_outputs.value(C_solver_outputs::N_OP_MODES, n_sub_ts);
		if( n_sub_ts == 1 )
		{
			mc_reported_outputs.value(C_solver_outputs::OP_MODE_1, operating_mode);
			mc_reported_outputs.value(C_solver_outputs::OP_MODE_2, 0.0);
			mc_reported_outputs.value(C_solver_outputs::OP_MODE_3, 0.0);
		}
		else if( n_sub_ts == 2 )
		{
			mc_reported_outputs.value(C_solver_outputs::OP_MODE_1, 0.0);
			mc_reported_outputs.overwrite_vector_to_constant(C_solver_outputs::OP_MODE_2, operating_mode);
			mc_reported_outputs.value(C_solver_outputs::OP_MODE_3, 0.0);
		}
		else if( n_sub_ts == 3 )
		{
			mc_reported_outputs.value(C_solver_outputs::OP_MODE_1, 0.0);
			mc_reported_outputs.value(C_solver_outputs::OP_MODE_2, 0.0);
			mc_reported_outputs.overwrite_vector_to_constant(C_solver_outputs::OP_MODE_3, operating_mode);
		}
		else
		{
			mc_reported_outputs.value(C_solver_outputs::OP_MODE_1, 0.0);
			mc_reported_outputs.value(C_solver_outputs::OP_MODE_2, 0.0);
			mc_reported_outputs.value(C_solver_outputs::OP_MODE_3, 0.0);
		}
		

		mc_reported_outputs.value(C_solver_outputs::TOU_PERIOD, (double)f_turb_tou_period);             //[-]       
		mc_reported_outputs.value(C_solver_outputs::PRICING_MULT, pricing_mult);	                    //[-] 
		mc_reported_outputs.value(C_solver_outputs::PC_Q_DOT_SB, q_pc_sb);                              //[MW]     
		mc_reported_outputs.value(C_solver_outputs::PC_Q_DOT_MIN, q_pc_min);                            //[MW]    
		mc_reported_outputs.value(C_solver_outputs::PC_Q_DOT_TARGET, q_pc_target);                      //[MW]
		mc_reported_outputs.value(C_solver_outputs::PC_Q_DOT_MAX, m_q_dot_pc_max);                      //[MW]
        mc_reported_outputs.value(C_solver_outputs::PC_Q_DOT_TARGET_SU, q_dot_pc_su_target_reporting);  //[MW]
        mc_reported_outputs.value(C_solver_outputs::PC_Q_DOT_TARGET_ON, q_dot_pc_on_target_reporting);  //[MW]
		mc_reported_outputs.value(C_solver_outputs::CTRL_IS_REC_SU, is_rec_su_allowed);                 //[-] 
		mc_reported_outputs.value(C_solver_outputs::CTRL_IS_PC_SU, is_pc_su_allowed);                   //[-] 
		mc_reported_outputs.value(C_solver_outputs::CTRL_IS_PC_SB, is_pc_sb_allowed);                   //[-]
		mc_reported_outputs.value(C_solver_outputs::CTRL_IS_PAR_HTR_SU, is_PAR_HTR_allowed);            //[-]
		mc_reported_outputs.value(C_solver_outputs::PAR_HTR_Q_DOT_TARGET, q_dot_elec_to_PAR_HTR);       //[MW]
		mc_reported_outputs.value(C_solver_outputs::EST_Q_DOT_CR_SU, q_dot_cr_startup);                 //[-]
		mc_reported_outputs.value(C_solver_outputs::EST_Q_DOT_CR_ON, q_dot_cr_on);                      //[MWt]
		mc_reported_outputs.value(C_solver_outputs::EST_Q_DOT_DC, q_dot_tes_dc);                        //[MWt]    
		mc_reported_outputs.value(C_solver_outputs::EST_Q_DOT_CH, q_dot_tes_ch);                        //[MWt]    

        double m_dot_cr_out_to_tes_hot = mc_cr_out_solver.m_m_dot_salt_tot;     //[kg/hr]
        double m_dot_cr_out_to_tes_cold = 0.0;
        if (!is_rec_outlet_to_hottank) {
            m_dot_cr_out_to_tes_cold = mc_cr_out_solver.m_m_dot_salt_tot;       //[kg/hr]
            m_dot_cr_out_to_tes_hot = 0.0;
        }

        double m_dot_par_htr_out_to_tes_hot = 0.0;      //[kg/hr]
        if (m_is_parallel_heater) {
            m_dot_par_htr_out_to_tes_hot = mc_par_htr_out_solver.m_m_dot_salt_tot;  //[kg/hr]
        }

		double m_dot_bal_hot = (m_dot_cr_out_to_tes_hot + m_dot_par_htr_out_to_tes_hot +
							mc_tes_outputs.m_m_dot_tes_hot_out*3600.0 -
							mc_pc_inputs.m_m_dot -
							mc_tes_outputs.m_m_dot_cr_to_tes_hot*3600.0) / m_m_dot_pc_des;		//[-]

		double m_dot_bal_cold = (m_dot_cr_out_to_tes_cold + mc_pc_inputs.m_m_dot +
							mc_tes_outputs.m_m_dot_tes_cold_out*3600.0 -
							mc_cr_out_solver.m_m_dot_salt_tot - m_dot_par_htr_out_to_tes_hot -
							mc_tes_outputs.m_m_dot_tes_cold_in*3600.0) / m_m_dot_pc_des;	//[-]

		double m_dot_bal_max = std::max(std::abs(m_dot_bal_hot), std::abs(m_dot_bal_cold));

        double q_dot_par_htr = 0.0;
        if (m_is_parallel_heater) {
            q_dot_par_htr = mc_par_htr_out_solver.m_q_thermal;  //[MWt]
        }

		double q_dot_bal = (mc_cr_out_solver.m_q_thermal + q_dot_par_htr +
							mc_tes_outputs.m_q_dot_dc_to_htf -
							mc_pc_out_solver.m_q_dot_htf -
							mc_tes_outputs.m_q_dot_ch_from_htf) / m_cycle_q_dot_des;	//[-]

		mc_reported_outputs.value(C_solver_outputs::ERR_M_DOT, m_dot_bal_max);
		mc_reported_outputs.value(C_solver_outputs::ERR_Q_DOT, q_dot_bal);

		mc_reported_outputs.value(C_solver_outputs::SOLZEN, mc_weather.ms_outputs.m_solzen);	//[deg] Solar zenith
		mc_reported_outputs.value(C_solver_outputs::SOLAZ, mc_weather.ms_outputs.m_solazi);		//[deg] Solar azimuth
		mc_reported_outputs.value(C_solver_outputs::BEAM, mc_weather.ms_outputs.m_beam);		//[W/m2] DNI
		mc_reported_outputs.value(C_solver_outputs::TDRY, mc_weather.ms_outputs.m_tdry);		//[C] Dry bulb temperature
		mc_reported_outputs.value(C_solver_outputs::TWET, mc_weather.ms_outputs.m_twet);		//[C] Wet bulb temperature
		mc_reported_outputs.value(C_solver_outputs::RH, mc_weather.ms_outputs.m_rhum);			//[-] Relative humidity
		mc_reported_outputs.value(C_solver_outputs::WSPD, mc_weather.ms_outputs.m_wspd);		//[m/s]
        mc_reported_outputs.value(C_solver_outputs::WDIR, mc_weather.ms_outputs.m_wdir);		//[deg]
		mc_reported_outputs.value(C_solver_outputs::PRES, mc_weather.ms_outputs.m_pres);		//[mbar]
		
		mc_reported_outputs.value(C_solver_outputs::CR_DEFOCUS, m_defocus);						//[-] Controller defocus
			// Thermal energy storage outputs
		mc_reported_outputs.value(C_solver_outputs::TES_Q_DOT_DC, mc_tes_outputs.m_q_dot_dc_to_htf);    //[MWt] TES discharge thermal power   
		mc_reported_outputs.value(C_solver_outputs::TES_Q_DOT_CH, mc_tes_outputs.m_q_dot_ch_from_htf);  //[MWt] TES charge thermal power    
		mc_reported_outputs.value(C_solver_outputs::TES_E_CH_STATE, e_tes_disch);                       //[MWht] TES charge state 
        mc_reported_outputs.value(C_solver_outputs::TES_T_COLD_IN, mc_tes_outputs.m_T_tes_cold_in - 273.15);    //[C]

		mc_reported_outputs.value(C_solver_outputs::M_DOT_CR_TO_TES_HOT, mc_tes_outputs.m_m_dot_cr_to_tes_hot);		//[kg/s]
        mc_reported_outputs.value(C_solver_outputs::M_DOT_CR_TO_TES_COLD, mc_tes_outputs.m_m_dot_cr_to_tes_cold);	//[kg/s]
		mc_reported_outputs.value(C_solver_outputs::M_DOT_TES_HOT_OUT, mc_tes_outputs.m_m_dot_tes_hot_out);			//[kg/s]
		mc_reported_outputs.value(C_solver_outputs::M_DOT_PC_TO_TES_COLD, mc_tes_outputs.m_m_dot_pc_to_tes_cold);	//[kg/s]
		mc_reported_outputs.value(C_solver_outputs::M_DOT_TES_COLD_OUT, mc_tes_outputs.m_m_dot_tes_cold_out);		//[kg/s]
        mc_reported_outputs.value(C_solver_outputs::M_DOT_TES_COLD_IN, mc_tes_outputs.m_m_dot_tes_cold_in);		    //[kg/s]
		mc_reported_outputs.value(C_solver_outputs::M_DOT_FIELD_TO_CYCLE, mc_tes_outputs.m_m_dot_src_to_sink);	    //[kg/s]
		mc_reported_outputs.value(C_solver_outputs::M_DOT_CYCLE_TO_FIELD, mc_tes_outputs.m_m_dot_sink_to_src);	    //[kg/s]


			// Parasitics outputs
        mc_reported_outputs.value(C_solver_outputs::SYS_W_DOT_FIXED, m_W_dot_fixed_design);						//[MWe] Fixed electric parasitic power load 

		mc_reported_outputs.value(C_solver_outputs::SYS_W_DOT_BOP, W_dot_bop);									//[MWe] Balance-of-plant electric parasitic power load   
		mc_reported_outputs.value(C_solver_outputs::W_DOT_NET, W_dot_net);								//[MWe] Total electric power output to grid        
		
            //Dispatch optimization outputs
		mc_reported_outputs.value(C_solver_outputs::DISPATCH_REL_MIP_GAP, mc_dispatch.lp_outputs.rel_mip_gap);
		mc_reported_outputs.value(C_solver_outputs::DISPATCH_SOLVE_STATE, mc_dispatch.lp_outputs.solve_state);
		mc_reported_outputs.value(C_solver_outputs::DISPATCH_SUBOPT_FLAG, mc_dispatch.lp_outputs.subopt_flag);
		mc_reported_outputs.value(C_solver_outputs::DISPATCH_SOLVE_ITER, mc_dispatch.lp_outputs.solve_iter);
		mc_reported_outputs.value(C_solver_outputs::DISPATCH_SOLVE_OBJ, mc_dispatch.lp_outputs.objective);
		mc_reported_outputs.value(C_solver_outputs::DISPATCH_SOLVE_OBJ_RELAX, mc_dispatch.lp_outputs.objective_relaxed);
		mc_reported_outputs.value(C_solver_outputs::DISPATCH_QSF_EXPECT, mc_dispatch.disp_outputs.qsf_expect);
		mc_reported_outputs.value(C_solver_outputs::DISPATCH_QSFPROD_EXPECT, mc_dispatch.disp_outputs.qsfprod_expect);
		mc_reported_outputs.value(C_solver_outputs::DISPATCH_QSFSU_EXPECT, mc_dispatch.disp_outputs.qsfsu_expect);
		mc_reported_outputs.value(C_solver_outputs::DISPATCH_TES_EXPECT, mc_dispatch.disp_outputs.tes_expect);
		mc_reported_outputs.value(C_solver_outputs::DISPATCH_PCEFF_EXPECT, mc_dispatch.disp_outputs.etapb_expect);
		mc_reported_outputs.value(C_solver_outputs::DISPATCH_SFEFF_EXPECT, mc_dispatch.disp_outputs.etasf_expect);
		mc_reported_outputs.value(C_solver_outputs::DISPATCH_QPBSU_EXPECT, mc_dispatch.disp_outputs.qpbsu_expect);
		mc_reported_outputs.value(C_solver_outputs::DISPATCH_WPB_EXPECT, mc_dispatch.disp_outputs.wpb_expect);
		mc_reported_outputs.value(C_solver_outputs::DISPATCH_REV_EXPECT, mc_dispatch.disp_outputs.rev_expect);
		mc_reported_outputs.value(C_solver_outputs::DISPATCH_PRES_NCONSTR, mc_dispatch.lp_outputs.presolve_nconstr);
		mc_reported_outputs.value(C_solver_outputs::DISPATCH_PRES_NVAR, mc_dispatch.lp_outputs.presolve_nvar);
		mc_reported_outputs.value(C_solver_outputs::DISPATCH_SOLVE_TIME, mc_dispatch.lp_outputs.solve_time);

		// Report series of operating modes attempted during the timestep as a 'double' so can see in hourly outputs
        // Key will start with 1 then add two digits for each operating mode. Single digits enumerations will add a 0 before the number
        // So a sequence that tries CR_ON__PC_TARGET__TES_CH then CR_ON__PC_TARGET__TES_DC then CR_ON__PC_RM_LO__TES_EMPTY
        //    will result in key: 1091011
		int n_op_modes = (int)m_op_mode_tracking.size();
		double op_mode_key = 1.0;
		for( int i = 0; i < std::min(3,n_op_modes); i++ )
		{
			double op_mode_step = m_op_mode_tracking[i];
            op_mode_key = 100.0 * op_mode_key + op_mode_step;
		}
		mc_reported_outputs.value(C_solver_outputs::CTRL_OP_MODE_SEQ_A, op_mode_key);

		op_mode_key = 0.0;
		for( int i = 3; i < std::min(6,n_op_modes); i++ )
		{
            double op_mode_step = m_op_mode_tracking[i];
            op_mode_key = 100.0 * op_mode_key + op_mode_step;
		}
		mc_reported_outputs.value(C_solver_outputs::CTRL_OP_MODE_SEQ_B, op_mode_key);

		op_mode_key = 0.0;
		for( int i = 6; i < n_op_modes; i++ )
		{
            double op_mode_step = m_op_mode_tracking[i];
            op_mode_key = 100.0 * op_mode_key + op_mode_step;
		}
		mc_reported_outputs.value(C_solver_outputs::CTRL_OP_MODE_SEQ_C, op_mode_key);

		mc_reported_outputs.set_timestep_outputs();

		// ****************************************************
		//          End saving timestep outputs
		// ****************************************************

		// Now check if internal csp timestep matches reporting timestep
		do
		{			
			if(mc_kernel.mc_sim_info.ms_ts.m_time >= m_report_time_end)
			{
				mc_collector_receiver.write_output_intervals(m_report_time_start, mv_time_local, m_report_time_end);
				mc_power_cycle.write_output_intervals(m_report_time_start, mv_time_local, m_report_time_end);
				mc_tes.write_output_intervals(m_report_time_start, mv_time_local, m_report_time_end);
                if (m_is_parallel_heater) {
                    mp_heater->write_output_intervals(m_report_time_start, mv_time_local, m_report_time_end);
                }
                if (m_is_CT_tes) {
                    mc_CT_tes->write_output_intervals(m_report_time_start, mv_time_local, m_report_time_end);
                }

				// Overwrite TIME_FINAL
				mc_reported_outputs.overwrite_most_recent_timestep(C_solver_outputs::TIME_FINAL, m_report_time_end / 3600.0);	//[hr]
				mc_reported_outputs.send_to_reporting_ts_array(m_report_time_start, mv_time_local, m_report_time_end);

				// Check if the most recent csp solver timestep aligns with the end of the reporting timestep
				bool delete_last_step = false;
				int pop_back_start = 1;

				int n_time_local = (int)mv_time_local.size();
				if( mv_time_local[n_time_local - 1] == m_report_time_end )
				{
					delete_last_step = true;
					pop_back_start = 0;
				}

				// If more than 1 element in temp vectors, only keep most recent value
				if( n_time_local > 1 || delete_last_step )
				{
					if( !delete_last_step )
					{
						mv_time_local[0] = mv_time_local[n_time_local - 1];
					}

					for( int i = pop_back_start; i < n_time_local; i++ )
					{
						mv_time_local.pop_back();
					}
				}

				int n_time_local_refresh = (int)mv_time_local.size();
				if(n_time_local_refresh > 0)
				{
					mc_reported_outputs.value(C_solver_outputs::N_OP_MODES, 1);	//[-]
					mc_reported_outputs.value(C_solver_outputs::OP_MODE_1, operating_mode);
					mc_reported_outputs.value(C_solver_outputs::OP_MODE_2, 0.0);
					mc_reported_outputs.value(C_solver_outputs::OP_MODE_3, 0.0);
				}

				// Advance time_reporting_hr index
				m_i_reporting++;
				m_report_time_start = m_report_time_end;	//[s]
				m_report_time_end += m_report_step;	//[s]			
			}
			else
			{
				break;
			}

		} while(true);

		
		// Don't converge weather file if working with partial timesteps
		if( mc_kernel.mc_sim_info.ms_ts.m_time < mc_kernel.get_baseline_end_time() )
		{
			mc_kernel.mc_sim_info.ms_ts.m_time_start = mc_kernel.mc_sim_info.ms_ts.m_time;	//[s]
			mc_kernel.mc_sim_info.ms_ts.m_time = mc_kernel.get_baseline_end_time();			//[s]
			mc_kernel.mc_sim_info.ms_ts.m_step = mc_kernel.mc_sim_info.ms_ts.m_time - mc_kernel.mc_sim_info.ms_ts.m_time_start;	//[s]
		}
		else if( mc_kernel.mc_sim_info.ms_ts.m_time == mc_kernel.get_baseline_end_time() )
		{
			if( mc_kernel.get_baseline_end_time() == mc_kernel.get_wf_end_time () )
			{
				mc_weather.converged();

				mc_kernel.wf_step_forward();
			}
			else if( mc_kernel.get_baseline_end_time() > mc_kernel.get_wf_end_time() )
			{
				throw(C_csp_exception("Baseline end time is larger than the weather file end time. This shouldn't happen"));
			}
					
			mc_kernel.baseline_step_forward();

			mc_kernel.mc_sim_info.ms_ts.m_time_start = mc_kernel.mc_sim_info.ms_ts.m_time;	//[s]
			mc_kernel.mc_sim_info.ms_ts.m_time = mc_kernel.get_baseline_end_time();			//[s]
			mc_kernel.mc_sim_info.ms_ts.m_step = mc_kernel.mc_sim_info.ms_ts.m_time - mc_kernel.mc_sim_info.ms_ts.m_time_start;	//[s]
		}
		else
		{
			throw(C_csp_exception("Kernel end time is larger than the baseline end time. This shouldn't happen"));
		}
		
        m_is_first_timestep = false;
	}	// End timestep loop

}	// End simulate() method

void C_csp_solver::calc_timestep_plant_control_and_targets(
    double f_turbine_tou /*-*/, double q_dot_pc_min /*MWt*/, double q_dot_tes_ch /*MWt*/, double pc_heat_prev /*MWt*/,  double pc_state_persist /*hours*/,
    C_csp_power_cycle::E_csp_power_cycle_modes pc_operating_state, double purchase_mult /*-*/, double sale_mult /*-*/,
    double calc_frac_current /*-*/, double baseline_step /*s*/,
    bool& is_q_dot_pc_target_overwrite,
    double& q_dot_pc_target /*MWt*/, double& q_dot_pc_max /*MWt*/, double& q_dot_elec_to_CR_heat /*MWt*/,
    bool& is_rec_su_allowed, bool& is_pc_su_allowed, bool& is_pc_sb_allowed,
    double& q_dot_elec_to_PAR_HTR /*MWt*/, bool& is_PAR_HTR_allowed)
{
    // Optional rules for TOD Block Plant Control
    if (mc_tou.m_dispatch_model_type == C_csp_tou::C_dispatch_model_type::E_dispatch_model_type::HEURISTIC)
    {
        is_rec_su_allowed = true;
        is_pc_su_allowed = true;
        is_pc_sb_allowed = true;

        // Set PC target and max thermal power
        q_dot_pc_target = f_turbine_tou * m_cycle_q_dot_des;	//[MW]
        if (mc_tou.m_is_tod_pc_target_also_pc_max) {
            q_dot_pc_max = q_dot_pc_target;     //[MW]
        }
        else {
            q_dot_pc_max = m_cycle_max_frac * m_cycle_q_dot_des;		//[MWt]
        }

        // Rule 1: if the sun sets (or does not rise) in __ [hours], then do not allow power cycle standby
            //double standby_time_buffer = 2.0;
        if (mc_tou.m_use_rule_1 &&
            (mc_weather.ms_outputs.m_hour + mc_tou.m_standby_off_buffer <= mc_weather.ms_outputs.m_time_rise ||
                mc_weather.ms_outputs.m_hour + mc_tou.m_standby_off_buffer >= mc_weather.ms_outputs.m_time_set))
        {
            is_pc_sb_allowed = false;
        }

        // Rule 2:
        if (mc_tou.m_use_rule_2 &&
            ((q_dot_pc_target < q_dot_pc_min && q_dot_tes_ch < m_q_dot_rec_des * mc_tou.m_q_dot_rec_des_mult) ||
                is_q_dot_pc_target_overwrite))
        {
            // If overwrite was previously true, but now power cycle is off, set to false
            if (is_q_dot_pc_target_overwrite &&
                (pc_operating_state == C_csp_power_cycle::OFF || q_dot_pc_target >= q_dot_pc_min))
            {
                is_q_dot_pc_target_overwrite = false;
            }
            else
            {
                is_q_dot_pc_target_overwrite = true;
            }

            if (is_q_dot_pc_target_overwrite)
            {
                q_dot_pc_target = mc_tou.m_f_q_dot_pc_overwrite * m_cycle_q_dot_des;
            }
        }

        // After rules, reset booleans if necessary
        if (q_dot_pc_target < q_dot_pc_min || q_dot_pc_target <= 0.)
        {
            is_pc_su_allowed = false;
            is_pc_sb_allowed = false;
            q_dot_pc_target = 0.0;
        }

        q_dot_elec_to_PAR_HTR = 0.0;
        is_PAR_HTR_allowed = false;
        if (m_is_parallel_heater && !is_pc_su_allowed && !is_pc_sb_allowed &&
            purchase_mult < 1.0 && q_dot_tes_ch > 0.0) {

            is_PAR_HTR_allowed = true;
            q_dot_elec_to_PAR_HTR = m_PAR_HTR_q_dot_rec_des;    //[MWt]
        }
    }
    // Use simply policy to govern arbitrage operation
    else if (mc_tou.m_dispatch_model_type == C_csp_tou::C_dispatch_model_type::E_dispatch_model_type::ARBITRAGE_CUTOFF) {

        // Check purchase multiplier
        // If less than 1, then allow charging
        q_dot_elec_to_PAR_HTR = 0.0;
        is_PAR_HTR_allowed = false;
        if (purchase_mult < 1.0 && q_dot_tes_ch > 0.0) {
            is_rec_su_allowed = true;
            q_dot_elec_to_CR_heat = m_q_dot_rec_des;    //[MWt]
            if (m_is_parallel_heater) {
                is_PAR_HTR_allowed = true;
                q_dot_elec_to_PAR_HTR = m_PAR_HTR_q_dot_rec_des;    //[MWt]
            }
        }
        else {
            is_rec_su_allowed = false;
            q_dot_elec_to_CR_heat = 0.0;
        }

        // Check (sale) price multiplier
        // If greater than 1, the allow discharging
        if (sale_mult > 1.0) {
            is_pc_su_allowed = true;
            is_pc_sb_allowed = false;

            q_dot_pc_target = m_cycle_q_dot_des;	//[MWt]
            if (mc_tou.m_is_tod_pc_target_also_pc_max) {
                q_dot_pc_max = q_dot_pc_target;     //[MWt]
            }
            else {
                q_dot_pc_max = m_cycle_max_frac * m_cycle_q_dot_des;		//[MWt]
            }
        }
        else {
            is_pc_su_allowed = false;
            is_pc_sb_allowed = false;

            q_dot_pc_target = 0.0;
            q_dot_pc_max = 0.0;
        }
    }
    // Use external dispatch targets
    else if (mc_tou.m_dispatch_model_type == C_csp_tou::C_dispatch_model_type::E_dispatch_model_type::IMPORT_DISPATCH_TARGETS) {
        int p = (int)ceil((mc_kernel.mc_sim_info.ms_ts.m_time - mc_kernel.get_sim_setup()->m_sim_time_start) / baseline_step) - 1;

        if (pc_operating_state == C_csp_power_cycle::OFF || pc_operating_state == C_csp_power_cycle::STARTUP) {
            q_dot_pc_target = mc_tou.mc_dispatch_params.m_q_pc_target_su_in.at(p);
        }
        else {
            q_dot_pc_target = mc_tou.mc_dispatch_params.m_q_pc_target_on_in.at(p) + mc_tou.mc_dispatch_params.m_q_pc_target_su_in.at(p);  // Dispatch can miss startup timing -> Use total target to avoid attempted solutions with q_pc_target = 0
        }

        q_dot_pc_max = mc_tou.mc_dispatch_params.m_q_pc_max_in.at(p);
        is_rec_su_allowed = mc_tou.mc_dispatch_params.m_is_rec_su_allowed_in.at(p);
        is_pc_su_allowed = mc_tou.mc_dispatch_params.m_is_pc_su_allowed_in.at(p);
        is_pc_sb_allowed = mc_tou.mc_dispatch_params.m_is_pc_sb_allowed_in.at(p);

        is_PAR_HTR_allowed = mc_tou.mc_dispatch_params.m_is_PAR_HTR_allowed_in.at(p);
        q_dot_elec_to_PAR_HTR = mc_tou.mc_dispatch_params.m_q_dot_elec_to_PAR_HTR_in.at(p);
        q_dot_elec_to_CR_heat = 0.0;

    }
    // Run dispatch optimization?
    else if (mc_tou.m_dispatch_model_type == C_csp_tou::C_dispatch_model_type::E_dispatch_model_type::DISPATCH_OPTIMIZATION) {
        q_dot_elec_to_PAR_HTR = 0.0;
        is_PAR_HTR_allowed = false;

        //time to reoptimize
        //reoptimize when the time is equal to multiples of the first time step
        if ((int)mc_kernel.mc_sim_info.ms_ts.m_time % (int)(3600. * mc_dispatch.solver_params.optimize_frequency) == baseline_step
            && mc_dispatch.disp_outputs.time_last != mc_kernel.mc_sim_info.ms_ts.m_time
            )
        {
            int opt_horizon = mc_dispatch.solver_params.optimize_horizon;
            double hour_now = mc_kernel.mc_sim_info.ms_ts.m_time / 3600.;

            //if this is the last day of the year, update the optimization horizon to be no more than the last 24 hours. 
            if (hour_now >= (8760. - opt_horizon))
                mc_dispatch.solver_params.optimize_horizon = (int)std::min((double)opt_horizon, (double)(8761. - hour_now));

            //message
            std::stringstream ss;
            ss << "Optimizing thermal energy dispatch profile for time window "
                << (int)(mc_kernel.mc_sim_info.ms_ts.m_time / 3600.) << " - "
                << (int)(mc_kernel.mc_sim_info.ms_ts.m_time / 3600.) + mc_dispatch.solver_params.optimize_frequency;

            mc_csp_messages.add_message(C_csp_messages::NOTICE, ss.str());
            send_callback((float)calc_frac_current * 100.f);
            ss.flush();

            // Update horizon parameter values and inital conition parameters
            if (!mc_dispatch.update_horizon_parameters(mc_tou)) {
                throw(C_csp_exception("Dispatch failed to update horizon parameter values"));
            }
            mc_dispatch.update_initial_conditions(pc_heat_prev, m_T_htf_cold_des, pc_state_persist);

            //predict performance for the time horizon
            if (
                mc_dispatch.predict_performance((int)
                    (mc_kernel.mc_sim_info.ms_ts.m_time / baseline_step - 1),
                    (int)(mc_dispatch.solver_params.optimize_horizon * mc_dispatch.solver_params.steps_per_hour),
                    (int)((3600. / baseline_step) / mc_dispatch.solver_params.steps_per_hour)
                )
                )
            {
                //call the optimize method
                bool opt_complete = mc_dispatch.lp_outputs.last_opt_successful = mc_dispatch.optimize();

                if (mc_dispatch.solver_params.disp_reporting && (!mc_dispatch.solver_params.log_message.empty()))
                {
                    mc_csp_messages.add_message(C_csp_messages::NOTICE, mc_dispatch.solver_params.log_message.c_str());
                    send_callback((float)calc_frac_current * 100.f);
                }

                mc_dispatch.m_current_read_step = 0;   //reset
            }
            else
            {
                throw(C_csp_exception("Dispatch failed to predict performance over the dispatch horizon"));
            }

            //call again to go back to original state
            mc_tou.call(mc_kernel.mc_sim_info.ms_ts.m_time, mc_tou_outputs);
        }

        //running from the optimized profile
        mc_dispatch.set_dispatch_outputs();

        //setting binaries and targets
        is_rec_su_allowed = mc_dispatch.disp_outputs.is_rec_su_allowed;
        is_pc_sb_allowed = mc_dispatch.disp_outputs.is_pc_sb_allowed;
        is_pc_su_allowed = mc_dispatch.disp_outputs.is_pc_su_allowed;
        q_dot_pc_target = mc_dispatch.disp_outputs.q_pc_target;
        q_dot_elec_to_CR_heat = mc_dispatch.disp_outputs.q_dot_elec_to_CR_heat;
        q_dot_pc_max = mc_dispatch.disp_outputs.q_dot_pc_max;
        is_PAR_HTR_allowed = mc_dispatch.disp_outputs.is_eh_su_allowed;
        q_dot_elec_to_PAR_HTR = mc_dispatch.disp_outputs.q_eh_target;
    }
}

void C_csp_solver::C_operating_mode_core::turn_off_mode_availability()
{
    m_is_mode_available = false;
    m_is_HI_SIDE_mode_available = false;
    m_is_LO_SIDE_mode_available = false;
}

void C_csp_solver::C_operating_mode_core::turn_on_mode_availability()
{
    m_is_mode_available = true;
    m_is_HI_SIDE_mode_available = true;
    m_is_LO_SIDE_mode_available = true;
}

C_csp_solver::C_operating_mode_core::C_operating_mode_core(C_csp_collector_receiver::E_csp_cr_modes cr_mode,
    C_csp_power_cycle::E_csp_power_cycle_modes pc_mode,
    C_MEQ__m_dot_tes::E_m_dot_solver_modes solver_mode,
    C_MEQ__timestep::E_timestep_target_modes step_target_mode,
    bool is_defocus,
    std::string op_mode_name,
    cycle_targets cycle_target_type,
    bool is_sensible_htf_only)
{
    m_cr_mode = cr_mode;
    m_pc_mode = pc_mode;
    m_solver_mode = solver_mode;
    m_step_target_mode = step_target_mode;
    m_is_defocus = is_defocus;
    m_op_mode_name = op_mode_name;
    m_cycle_target_type = cycle_target_type;
    m_is_sensible_htf_only = is_sensible_htf_only;

    m_htr_mode = C_csp_collector_receiver::E_csp_cr_modes::OFF;

    turn_on_mode_availability();
}

C_csp_solver::C_operating_mode_core::C_operating_mode_core(C_csp_collector_receiver::E_csp_cr_modes cr_mode,
    C_csp_power_cycle::E_csp_power_cycle_modes pc_mode,
    C_MEQ__m_dot_tes::E_m_dot_solver_modes solver_mode,
    C_MEQ__timestep::E_timestep_target_modes step_target_mode,
    bool is_defocus,
    std::string op_mode_name,
    cycle_targets cycle_target_type,
    bool is_sensible_htf_only,
    C_csp_collector_receiver::E_csp_cr_modes htr_mode)
{
    m_cr_mode = cr_mode;
    m_pc_mode = pc_mode;
    m_solver_mode = solver_mode;
    m_step_target_mode = step_target_mode;
    m_is_defocus = is_defocus;
    m_op_mode_name = op_mode_name;
    m_cycle_target_type = cycle_target_type;
    m_is_sensible_htf_only = is_sensible_htf_only;

    m_htr_mode = htr_mode;

    turn_on_mode_availability();
}

void C_csp_solver::C_operating_mode_core::handle_solve_error(double time /*hr*/, bool& is_turn_off_rec_su)
{
    m_is_mode_available = false;
    is_turn_off_rec_su = false;
}

void C_csp_solver::C_operating_mode_core::check_system_limits(C_csp_solver* pc_csp_solver,
    double q_dot_pc_su_max /*MWt*/, double m_dot_pc_max_startup /*kg/hr*/,
    double q_dot_pc_solve_target /*MWt*/, double q_dot_pc_on_dispatch_target,
    double q_dot_pc_max /*MWt*/, double q_dot_pc_min /*MWt*/, double q_dot_pc_sb /*MWt*/,
    double m_dot_pc_max /*kg/hr*/, double m_dot_pc_min /*kg/hr*/,
    double limit_comp_tol /*-*/,
    bool& is_model_converged, bool& is_turn_off_plant)
{
    is_model_converged = true;
    is_turn_off_plant = false;
}

bool C_csp_solver::C_operating_mode_core::solve(C_csp_solver* pc_csp_solver, bool is_rec_outlet_to_hottank,
    double q_dot_pc_on_dispatch_target /*MWt*/, double q_dot_pc_startup /*MWt*/, double q_dot_pc_standby /*MWt*/,
    double q_dot_pc_min /*MWt*/, double q_dot_pc_max /*MWt*/, double q_dot_pc_startup_max /*MWt*/,
    double m_dot_pc_startup_max /*kg/hr*/, double m_dot_pc_max /*kg/hr*/, double m_dot_pc_min /*kg/hr*/,
    double q_dot_elec_to_CR_heat /*MWt*/, double q_dot_elec_to_PAR_HTR /*MWt*/, double limit_comp_tol /*-*/,
    double& defocus_solved, bool& is_op_mode_avail /*-*/, bool& is_turn_off_plant, bool& is_turn_off_rec_su)
{
    if (!pc_csp_solver->mc_collector_receiver.m_is_sensible_htf && m_is_sensible_htf_only) {
        std::string error_msg = util::format("At time = %lg ", pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time / 3600.0);
        error_msg += " controller chose operating mode " + m_op_mode_name + " which is not configured for DSG systems";
        throw(C_csp_exception(error_msg, "CSP Solver"));
    }

    double q_dot_pc_solve = std::numeric_limits<double>::quiet_NaN();

    switch (m_cycle_target_type)
    {
    case QUIETNAN:
    {
        q_dot_pc_solve = std::numeric_limits<double>::quiet_NaN();
        break;
    }
    case Q_DOT_PC_TARGET:
    {
        q_dot_pc_solve = q_dot_pc_on_dispatch_target;       //[MWt]
        break;
    }
    case Q_DOT_PC_STARTUP:
    {
        q_dot_pc_solve = q_dot_pc_startup;          //[MWt]
        break;
    }
    case Q_DOT_PC_STANDBY:
    {
        q_dot_pc_solve = q_dot_pc_standby;          //[MWt]
        break;
    }
    case Q_DOT_PC_MIN:
    {
        q_dot_pc_solve = q_dot_pc_min;              //[MWt]
        break;
    }
    case Q_DOT_PC_MAX:
    {
        q_dot_pc_solve = q_dot_pc_max;              //[MWt]
        break;
    }
    default:
        throw(C_csp_exception("Unknown cycle target type"));
    }

    int solve_error_code = pc_csp_solver->solve_operating_mode(m_cr_mode,
        m_pc_mode, m_htr_mode,
        m_solver_mode, m_step_target_mode,
        q_dot_pc_solve, m_is_defocus, is_rec_outlet_to_hottank,
        q_dot_elec_to_CR_heat, q_dot_elec_to_PAR_HTR,
        m_op_mode_name, defocus_solved);

    bool is_converged = true;
    bool is_turn_off_plant_local = false;
    bool is_turn_off_rec_su_local = false;
    if (solve_error_code != 0) {
        handle_solve_error(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time, is_turn_off_rec_su_local);
        is_converged = false;
    }
    else {
        check_system_limits(pc_csp_solver,
            q_dot_pc_startup_max, m_dot_pc_startup_max,
            q_dot_pc_solve, q_dot_pc_on_dispatch_target,
            q_dot_pc_max, q_dot_pc_min, q_dot_pc_standby,
            m_dot_pc_max, m_dot_pc_min,
            limit_comp_tol,
            is_converged, is_turn_off_plant_local);
    }

    is_turn_off_plant = is_turn_off_plant_local;
    is_op_mode_avail = m_is_mode_available;
    is_turn_off_rec_su = is_turn_off_rec_su_local;

    return is_converged;
}

std::string C_csp_solver::C_operating_mode_core::time_and_op_mode_to_string(double time /*s*/)
{
    return util::format("At time = %lg ", time / 3600.0) + " " + m_op_mode_name;
}

void C_csp_solver::C_CR_OFF__PC_OFF__TES_OFF__AUX_OFF::handle_solve_error(double time /*hr*/, bool& is_turn_off_rec_su)
{
    throw(C_csp_exception(util::format("At time = %lg ", time / 3600.0) + " operating mode " + m_op_mode_name + " failed", ""));
}

void C_csp_solver::C_CR_ON__PC_SU__TES_OFF__AUX_OFF::check_system_limits(C_csp_solver* pc_csp_solver,
    double q_dot_pc_su_max /*MWt*/, double m_dot_pc_max_startup /*kg/hr*/,
    double q_dot_pc_solve_target /*MWt*/, double q_dot_pc_on_dispatch_target,
    double q_dot_pc_max /*MWt*/, double q_dot_pc_min /*MWt*/, double q_dot_pc_sb /*MWt*/,
    double m_dot_pc_max /*kg/hr*/, double m_dot_pc_min /*kg/hr*/,
    double limit_comp_tol,
    bool& is_model_converged, bool& is_turn_off_plant)
{
    // Compare q_dot_to_pc to q_dot_pc_su_max
    
    if (pc_csp_solver->mc_pc_out_solver.m_q_dot_htf > q_dot_pc_su_max || pc_csp_solver->mc_pc_out_solver.m_m_dot_htf > m_dot_pc_max_startup)
    {
        std::string error_msg;
        if (pc_csp_solver->mc_pc_out_solver.m_q_dot_htf > q_dot_pc_su_max)
        {
            error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) + " method converged to a power cycle";
            error_msg += util::format(" thermal input, %lg [MWt], greater than the target %lg [MWt].", pc_csp_solver->mc_pc_out_solver.m_q_dot_htf, q_dot_pc_su_max);
        }
        if (pc_csp_solver->mc_pc_out_solver.m_m_dot_htf > m_dot_pc_max_startup)
        {
            error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) + " method converged to a power cycle";
            error_msg += util::format(" mass flow rate input, %lg [kg/s], greater than the maximum allowable %lg [kg/s].", pc_csp_solver->mc_pc_out_solver.m_m_dot_htf / 3600.0, pc_csp_solver->m_m_dot_pc_max_startup / 3600.0);
        }
        pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);
    }
    is_model_converged = true;
    is_turn_off_plant = false;
    m_is_mode_available = true;
}

void C_csp_solver::C_CR_OFF__PC_TARGET__TES_DC__AUX_OFF::check_system_limits(C_csp_solver* pc_csp_solver,
    double q_dot_pc_su_max /*MWt*/, double m_dot_pc_max_startup /*kg/hr*/,
    double q_dot_pc_solve_target /*MWt*/, double q_dot_pc_on_dispatch_target,
    double q_dot_pc_max /*MWt*/, double q_dot_pc_min /*MWt*/, double q_dot_pc_sb /*MWt*/,
    double m_dot_pc_max /*kg/hr*/, double m_dot_pc_min /*kg/hr*/,
    double limit_comp_tol /*-*/,
    bool& is_model_converged, bool& is_turn_off_plant)
{
    double q_dot_pc_solved = pc_csp_solver->mc_pc_out_solver.m_q_dot_htf;	//[MWt]
    double m_dot_pc_solved = pc_csp_solver->mc_pc_out_solver.m_m_dot_htf;	//[kg/hr]

    // Check if solved thermal power is greater than target
    if ((q_dot_pc_solved - q_dot_pc_on_dispatch_target) / q_dot_pc_on_dispatch_target > limit_comp_tol)
    {
        if ((q_dot_pc_solved - q_dot_pc_max) / q_dot_pc_max > limit_comp_tol)
        {
            std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
                util::format(" converged to a PC thermal power %lg [MWt]"
                    " larger than the maximum PC thermal power %lg [MWt]. Controller shut off plant",
                    q_dot_pc_solved, q_dot_pc_max);

            pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);

            is_model_converged = false;
            is_turn_off_plant = true;
            m_is_mode_available = false;

            return;
        }
        else
        {
            std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
                util::format(" converged to a PC thermal power %lg [MWt] larger than the target PC thermal power %lg [MWt]"
                    " but less than the maximum thermal power %lg [MWt]", q_dot_pc_solved, q_dot_pc_on_dispatch_target, q_dot_pc_max);

            pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);

            is_model_converged = true;
            is_turn_off_plant = false;
            m_is_mode_available = true;
        }
    }
    else if ((q_dot_pc_solved - q_dot_pc_on_dispatch_target) / q_dot_pc_on_dispatch_target < -limit_comp_tol)
    {
        if (m_dot_pc_solved < m_dot_pc_max)
        {	// TES cannot provide enough thermal power - step down to next operating mode

            m_is_mode_available = false;
            is_model_converged = false;
            is_turn_off_plant = false;

            return;
        }
    }
}

void C_csp_solver::C_CR_ON__PC_TARGET__TES_DC__AUX_OFF::check_system_limits(C_csp_solver* pc_csp_solver,
    double q_dot_pc_su_max /*MWt*/, double m_dot_pc_max_startup /*kg/hr*/,
    double q_dot_pc_solve_target /*MWt*/, double q_dot_pc_on_dispatch_target,
    double q_dot_pc_max /*MWt*/, double q_dot_pc_min /*MWt*/, double q_dot_pc_sb /*MWt*/,
    double m_dot_pc_max /*kg/hr*/, double m_dot_pc_min /*kg/hr*/,
    double limit_comp_tol /*-*/,
    bool& is_model_converged, bool& is_turn_off_plant)
{
    double q_dot_pc_solved = pc_csp_solver->mc_pc_out_solver.m_q_dot_htf;	//[MWt]
    double m_dot_pc_solved = pc_csp_solver->mc_pc_out_solver.m_m_dot_htf;	//[kg/hr]

    // Check bounds on solved thermal power and mass flow rate
    if ((q_dot_pc_solved - q_dot_pc_on_dispatch_target) / q_dot_pc_on_dispatch_target > limit_comp_tol)
    {
        if ((q_dot_pc_solved - q_dot_pc_max) / q_dot_pc_max > limit_comp_tol)
        {
            std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
                util::format(" solved with a PC thermal power %lg [MWt] greater than the maximum %lg [MWt]. Controller shut off plant",
                    q_dot_pc_solved, q_dot_pc_max);
            pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);

            m_is_mode_available = false;
            is_model_converged = false;
            is_turn_off_plant = true;

            return;
        }
        else
        {
            std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
                util::format(" solved with a PC thermal power %lg [MWt] greater than the target %lg [MWt], but less than the maximum %lg [MWt].",
                    q_dot_pc_solved, q_dot_pc_on_dispatch_target, q_dot_pc_max);
            pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);

            m_is_mode_available = true;
            is_model_converged = true;
            is_turn_off_plant = false;
        }
    }
    if (m_dot_pc_solved < m_dot_pc_min)
    {	// If we're already hitting the minimum mass flow rate, then trying next operating mode won't help
        std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
            util::format(" solved with a PC HTF mass flow rate %lg [kg/s] less than the minimum %lg [kg/s]. Controller shut off plant",
                m_dot_pc_solved / 3600.0, m_dot_pc_min / 3600.0);
        pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);

        m_is_mode_available = false;
        is_model_converged = false;
        is_turn_off_plant = true;

        return;
    }

    if ((q_dot_pc_solved - q_dot_pc_on_dispatch_target) / q_dot_pc_on_dispatch_target < -limit_comp_tol
        && (m_dot_pc_solved - m_dot_pc_max) / m_dot_pc_max < -limit_comp_tol)
    {
        m_is_mode_available = false;
        is_model_converged = false;
        is_turn_off_plant = false;

        return;
    }

    if (m_dot_pc_solved > m_dot_pc_max)
    {	// Shouldn't happen but can try next operating mode
        m_is_mode_available = false;
        is_model_converged = false;
        is_turn_off_plant = false;

        return;
    }
}

void C_csp_solver::C_CR_ON__PC_RM_LO__TES_OFF__AUX_OFF::check_system_limits(C_csp_solver* pc_csp_solver,
    double q_dot_pc_su_max /*MWt*/, double m_dot_pc_max_startup /*kg/hr*/,
    double q_dot_pc_solve_target /*MWt*/, double q_dot_pc_on_dispatch_target,
    double q_dot_pc_max /*MWt*/, double q_dot_pc_min /*MWt*/, double q_dot_pc_sb /*MWt*/,
    double m_dot_pc_max /*kg/hr*/, double m_dot_pc_min /*kg/hr*/,
    double limit_comp_tol /*-*/,
    bool& is_model_converged, bool& is_turn_off_plant)
{
    if (pc_csp_solver->mc_pc_out_solver.m_q_dot_htf < q_dot_pc_min)
    {
        m_is_mode_available = false;
        is_model_converged = false;
        is_turn_off_plant = false;
    }
}

void C_csp_solver::C_CR_ON__PC_TARGET__TES_CH__AUX_OFF::handle_solve_error(double time /*hr*/, bool& is_turn_off_rec_su)
{
    // LO side needs to stay 'true' so controller can try modes with TES discharge
    m_is_HI_SIDE_mode_available = false;
    is_turn_off_rec_su = false;
}

void C_csp_solver::C_CR_ON__PC_TARGET__TES_CH__AUX_OFF::check_system_limits(C_csp_solver* pc_csp_solver,
    double q_dot_pc_su_max /*MWt*/, double m_dot_pc_max_startup /*kg/hr*/,
    double q_dot_pc_solve_target /*MWt*/, double q_dot_pc_on_dispatch_target,
    double q_dot_pc_max /*MWt*/, double q_dot_pc_min /*MWt*/, double q_dot_pc_sb /*MWt*/,
    double m_dot_pc_max /*kg/hr*/, double m_dot_pc_min /*kg/hr*/,
    double limit_comp_tol /*-*/,
    bool& is_model_converged, bool& is_turn_off_plant)
{
    double q_dot_pc_solved = pc_csp_solver->mc_pc_out_solver.m_q_dot_htf;		//[MWt]
    double m_dot_pc_solved = pc_csp_solver->mc_pc_out_solver.m_m_dot_htf;		//[kg/hr]

    if (std::abs(q_dot_pc_solved - q_dot_pc_on_dispatch_target) / q_dot_pc_on_dispatch_target < limit_comp_tol)
    {	// If successfully solved for target thermal power, check that mass flow is above minimum
        if ((m_dot_pc_solved - m_dot_pc_min) / std::max(0.01, m_dot_pc_min) < -limit_comp_tol)
        {
            std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
                util::format(" solved with a PC HTF mass flow rate %lg [kg/s] smaller than the minimum %lg [kg/s]. Controller shut off plant",
                    m_dot_pc_solved / 3600.0, m_dot_pc_min / 3600.0);
            pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);

            turn_off_mode_availability();
            is_model_converged = false;
            is_turn_off_plant = true;
            return;
        }
    }
    else if ((q_dot_pc_solved - q_dot_pc_on_dispatch_target) / q_dot_pc_on_dispatch_target < -limit_comp_tol)
    {
        if ((m_dot_pc_solved - m_dot_pc_max) / m_dot_pc_max < -limit_comp_tol)
        {
            // Can send more mass flow to PC from TES
            m_is_LO_SIDE_mode_available = false;
            is_model_converged = false;
            is_turn_off_plant = false;
            return;
        }
    }
}

void C_csp_solver::C_CR_OFF__PC_MIN__TES_EMPTY__AUX_OFF::check_system_limits(C_csp_solver* pc_csp_solver,
    double q_dot_pc_su_max /*MWt*/, double m_dot_pc_max_startup /*kg/hr*/,
    double q_dot_pc_solve_target /*MWt*/, double q_dot_pc_on_dispatch_target,
    double q_dot_pc_max /*MWt*/, double q_dot_pc_min /*MWt*/, double q_dot_pc_sb /*MWt*/,
    double m_dot_pc_max /*kg/hr*/, double m_dot_pc_min /*kg/hr*/,
    double limit_comp_tol /*-*/,
    bool& is_model_converged, bool& is_turn_off_plant)
{
    // Check if solved thermal power is greater than target
    if (pc_csp_solver->mc_pc_out_solver.m_q_dot_htf > q_dot_pc_max)
    {
        std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
            util::format(" converged to a PC thermal power %lg [MWt] larger than the maximum PC thermal power %lg [MWt]. Controller shut off plant",
                pc_csp_solver->mc_pc_out_solver.m_q_dot_htf, q_dot_pc_max);

        pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);

        turn_off_mode_availability();
        is_model_converged = false;
        is_turn_off_plant = true;

        return;
    }

    if (pc_csp_solver->mc_pc_out_solver.m_m_dot_htf > m_dot_pc_max)
    {
        std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
            util::format(" converged to a HTF mass flow rate %lg [kg/s] larger than the maximum PC mass flow rate %lg [kg/s]. Controller shut off plant",
                pc_csp_solver->mc_pc_out_solver.m_m_dot_htf / 3600.0, m_dot_pc_max / 3600.0);

        pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);

        turn_off_mode_availability();
        is_model_converged = false;
        is_turn_off_plant = true;

        return;
    }
}

void C_csp_solver::C_CR_ON__PC_RM_LO__TES_EMPTY__AUX_OFF::check_system_limits(C_csp_solver* pc_csp_solver,
    double q_dot_pc_su_max /*MWt*/, double m_dot_pc_max_startup /*kg/hr*/,
    double q_dot_pc_solve_target /*MWt*/, double q_dot_pc_on_dispatch_target,
    double q_dot_pc_max /*MWt*/, double q_dot_pc_min /*MWt*/, double q_dot_pc_sb /*MWt*/,
    double m_dot_pc_max /*kg/hr*/, double m_dot_pc_min /*kg/hr*/,
    double limit_comp_tol /*-*/,
    bool& is_model_converged, bool& is_turn_off_plant)
{
    // *********************************
        // Check if solved thermal power is greater than target
    if ((pc_csp_solver->mc_pc_out_solver.m_q_dot_htf - q_dot_pc_on_dispatch_target) / q_dot_pc_on_dispatch_target > limit_comp_tol)
    {
        if ((pc_csp_solver->mc_pc_out_solver.m_q_dot_htf - q_dot_pc_max) / q_dot_pc_max > limit_comp_tol)
        {
            std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
                util::format(" converged to a PC thermal power %lg [MWt] larger than the maximum PC thermal power %lg [MWt]. Controller shut off plant",
                    pc_csp_solver->mc_pc_out_solver.m_q_dot_htf, q_dot_pc_max);

            pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);

            turn_off_mode_availability();
            is_model_converged = false;
            is_turn_off_plant = true;
            return;
        }
        else
        {
            std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
                util::format(" converged to a PC thermal power %lg [MWt] larger than the target PC thermal power %lg [MWt] but less than the maximum thermal power %lg [MWt]",
                    pc_csp_solver->mc_pc_out_solver.m_q_dot_htf, q_dot_pc_on_dispatch_target, q_dot_pc_max);

            pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);

            m_is_mode_available = true;
            is_model_converged = true;
            is_turn_off_plant = false;
        }
    }

    if ((pc_csp_solver->mc_pc_out_solver.m_m_dot_htf - m_dot_pc_max) / m_dot_pc_max > limit_comp_tol)
    {
        std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
            util::format(" converged to a HTF mass flow rate %lg [kg/s] larger than the maximum PC mass flow rate %lg [kg/s]. Controller shut off plant",
                pc_csp_solver->mc_pc_out_solver.m_m_dot_htf / 3600.0, m_dot_pc_max / 3600.0);

        pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);

        turn_off_mode_availability();
        is_model_converged = false;
        is_turn_off_plant = true;
        return;
    }

    // *********************************
    // Check PC q_dot is >= MIN!!!!!!!!

    if ((pc_csp_solver->mc_pc_out_solver.m_q_dot_htf - q_dot_pc_min) / q_dot_pc_min < -limit_comp_tol)
    {
        m_is_mode_available = false;
        is_model_converged = false;
        is_turn_off_plant = false;
        return;
    }
    if ((pc_csp_solver->mc_pc_out_solver.m_m_dot_htf - m_dot_pc_min) / m_dot_pc_min < -limit_comp_tol)
    {
        m_is_mode_available = false;
        is_model_converged = false;
        is_turn_off_plant = false;
        return;
    }
}

void C_csp_solver::C_CR_OFF__PC_RM_LO__TES_EMPTY__AUX_OFF::check_system_limits(C_csp_solver* pc_csp_solver,
    double q_dot_pc_su_max /*MWt*/, double m_dot_pc_max_startup /*kg/hr*/,
    double q_dot_pc_solve_target /*MWt*/, double q_dot_pc_on_dispatch_target,
    double q_dot_pc_max /*MWt*/, double q_dot_pc_min /*MWt*/, double q_dot_pc_sb /*MWt*/,
    double m_dot_pc_max /*kg/hr*/, double m_dot_pc_min /*kg/hr*/,
    double limit_comp_tol /*-*/,
    bool& is_model_converged, bool& is_turn_off_plant)
{
    if (pc_csp_solver->mc_pc_out_solver.m_q_dot_htf < q_dot_pc_min || pc_csp_solver->mc_pc_out_solver.m_m_dot_htf < m_dot_pc_min)
    {
        m_is_mode_available = false;
        is_model_converged = false;
        is_turn_off_plant = false;
        return;
    }

    // Check if solved thermal power is greater than target
    if (pc_csp_solver->mc_pc_out_solver.m_q_dot_htf > q_dot_pc_on_dispatch_target)
    {
        if (pc_csp_solver->mc_pc_out_solver.m_q_dot_htf > q_dot_pc_max)
        {
            std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
                util::format(" converged to a PC thermal power %lg [MWt] larger than the maximum PC thermal power %lg [MWt]. Controller shut off plant",
                    pc_csp_solver->mc_pc_out_solver.m_q_dot_htf, pc_csp_solver->m_q_dot_pc_max);

            pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);

            m_is_mode_available = false;
            is_model_converged = false;
            is_turn_off_plant = true;
            return;
        }
        else
        {
            std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
                util::format(" converged to a PC thermal power %lg [MWt] larger than the target PC thermal power %lg [MWt] but less than the maximum thermal power %lg [MWt]",
                    pc_csp_solver->mc_pc_out_solver.m_q_dot_htf, q_dot_pc_on_dispatch_target, q_dot_pc_max);
            pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);
        }
    }

    if (pc_csp_solver->mc_pc_out_solver.m_m_dot_htf > m_dot_pc_max)
    {
        std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
            util::format(" converged to a HTF mass flow rate %lg [kg/s] larger than the maximum PC mass flow rate %lg [kg/s]. Controller shut off plant",
                pc_csp_solver->mc_pc_out_solver.m_m_dot_htf / 3600.0, m_dot_pc_max / 3600.0);

        pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);

        m_is_mode_available = false;
        is_model_converged = false;
        is_turn_off_plant = true;
        return;
    }

    m_is_mode_available = true;
    is_model_converged = true;
    is_turn_off_plant = false;
}

void C_csp_solver::C_CR_ON__PC_MIN__TES_EMPTY__AUX_OFF::check_system_limits(C_csp_solver* pc_csp_solver,
    double q_dot_pc_su_max /*MWt*/, double m_dot_pc_max_startup /*kg/hr*/,
    double q_dot_pc_solve_target /*MWt*/, double q_dot_pc_on_dispatch_target,
    double q_dot_pc_max /*MWt*/, double q_dot_pc_min /*MWt*/, double q_dot_pc_sb /*MWt*/,
    double m_dot_pc_max /*kg/hr*/, double m_dot_pc_min /*kg/hr*/,
    double limit_comp_tol /*-*/,
    bool& is_model_converged, bool& is_turn_off_plant)
{
    if (pc_csp_solver->mc_pc_out_solver.m_m_dot_htf > m_dot_pc_max)
    {
        std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
            util::format(" converged to a HTF mass flow rate %lg [kg/s] larger than the maximum PC mass flow rate %lg [kg/s]. Controller shut off plant",
                pc_csp_solver->mc_pc_out_solver.m_m_dot_htf / 3600.0, m_dot_pc_max / 3600.0);

        pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);

        m_is_mode_available = false;
        is_model_converged = false;
        is_turn_off_plant = true;

        return;
    }

    // Check if solved thermal power is less than target
    if ((pc_csp_solver->mc_pc_out_solver.m_q_dot_htf - q_dot_pc_min) / q_dot_pc_min < -limit_comp_tol)
    {
        std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
            util::format(" converged to a PC thermal power %lg [MWt] less than the minimum PC thermal power %lg [MWt]. Controller moved to next operating mode.",
                pc_csp_solver->mc_pc_out_solver.m_q_dot_htf, q_dot_pc_min);

        pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);

        m_is_mode_available = false;
        is_model_converged = false;
        is_turn_off_plant = false;

        return;
    }

    if ((pc_csp_solver->mc_pc_out_solver.m_m_dot_htf - m_dot_pc_min) / m_dot_pc_min < -limit_comp_tol / 10.)
    {
        std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
            util::format(" converged to a HTF mass flow rate %lg [kg/s] less than the minimum PC HTF mass flow rate %lg [kg/s]. Controller moved to next operating mode.",
                pc_csp_solver->mc_pc_out_solver.m_m_dot_htf / 3600.0, m_dot_pc_min / 3600.0);

        pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);

        m_is_mode_available = false;
        is_model_converged = false;
        is_turn_off_plant = false;

        return;
    }
}

void C_csp_solver::C_CR_SU__PC_TARGET__TES_DC__AUX_OFF::check_system_limits(C_csp_solver* pc_csp_solver,
    double q_dot_pc_su_max /*MWt*/, double m_dot_pc_max_startup /*kg/hr*/,
    double q_dot_pc_solve_target /*MWt*/, double q_dot_pc_on_dispatch_target,
    double q_dot_pc_max /*MWt*/, double q_dot_pc_min /*MWt*/, double q_dot_pc_sb /*MWt*/,
    double m_dot_pc_max /*kg/hr*/, double m_dot_pc_min /*kg/hr*/,
    double limit_comp_tol /*-*/,
    bool& is_model_converged, bool& is_turn_off_plant)
{
    double q_dot_pc_solved = pc_csp_solver->mc_pc_out_solver.m_q_dot_htf;	//[MWt]
    double m_dot_pc_solved = pc_csp_solver->mc_pc_out_solver.m_m_dot_htf;	//[kg/hr]

    // Check if solved thermal power is greater than target
    if ((q_dot_pc_solved - q_dot_pc_on_dispatch_target) / q_dot_pc_on_dispatch_target > limit_comp_tol)
    {
        if ((q_dot_pc_solved - q_dot_pc_max) / q_dot_pc_max > limit_comp_tol)
        {
            std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
                util::format(" converged to a PC thermal power %lg [MWt] larger than the maximum PC thermal power %lg [MWt]. Controller shut off plant",
                    q_dot_pc_solved, q_dot_pc_max);
            pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);

            m_is_mode_available = false;
            is_model_converged = false;
            is_turn_off_plant = true;
            return;
        }
        else
        {
            std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
                util::format(" converged to a PC thermal power %lg [MWt] larger than the target PC thermal power %lg [MWt] but less than the maximum thermal power %lg [MWt]",
                    q_dot_pc_solved, q_dot_pc_on_dispatch_target, q_dot_pc_max);

            pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);
        }
    }
    else if ((q_dot_pc_solved - q_dot_pc_on_dispatch_target) / q_dot_pc_on_dispatch_target < -limit_comp_tol)
    {
        if (m_dot_pc_solved < m_dot_pc_max)
        {	// TES cannot provide enough thermal power - step down to next operating mode

            m_is_mode_available = false;
            is_model_converged = false;
            is_turn_off_plant = false;
            return;
        }
        // Notes:
        //else
        //{	// PC maximum mass flow is constraining the thermal power that TES can send the PC. Changing modes wont' help
        //
        //}
    }
}

void C_csp_solver::C_CR_SU__PC_MIN__TES_EMPTY__AUX_OFF::check_system_limits(C_csp_solver* pc_csp_solver,
    double q_dot_pc_su_max /*MWt*/, double m_dot_pc_max_startup /*kg/hr*/,
    double q_dot_pc_solve_target /*MWt*/, double q_dot_pc_on_dispatch_target,
    double q_dot_pc_max /*MWt*/, double q_dot_pc_min /*MWt*/, double q_dot_pc_sb /*MWt*/,
    double m_dot_pc_max /*kg/hr*/, double m_dot_pc_min /*kg/hr*/,
    double limit_comp_tol /*-*/,
    bool& is_model_converged, bool& is_turn_off_plant)
{
    // Check if solved thermal power is greater than target
    if ((pc_csp_solver->mc_pc_out_solver.m_q_dot_htf - q_dot_pc_max) > limit_comp_tol)
    {
        std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
            util::format(" converged to a PC thermal power %lg [MWt] larger than the maximum PC thermal power %lg [MWt]. Controller shut off plant",
                pc_csp_solver->mc_pc_out_solver.m_q_dot_htf, q_dot_pc_max);

        pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);

        m_is_mode_available = false;
        is_model_converged = false;
        is_turn_off_plant = true;
        return;
    }

    if (pc_csp_solver->mc_pc_out_solver.m_m_dot_htf > m_dot_pc_max)
    {
        std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
            util::format(" converged to a HTF mass flow rate %lg [kg/s] larger than the maximum PC mass flow rate %lg [kg/s]. Controller shut off plant",
                pc_csp_solver->mc_pc_out_solver.m_m_dot_htf / 3600.0, m_dot_pc_max / 3600.0);

        pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);

        m_is_mode_available = false;
        is_model_converged = false;
        is_turn_off_plant = true;
        return;
    }

    // Check if solved thermal power is less than target
    if ((pc_csp_solver->mc_pc_out_solver.m_q_dot_htf - q_dot_pc_min) / q_dot_pc_min < -limit_comp_tol)
    {
        std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
            util::format(" converged to a PC thermal power %lg [MWt] less than the minimum PC thermal power %lg [MWt].",
                pc_csp_solver->mc_pc_out_solver.m_q_dot_htf, q_dot_pc_min);

        pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);

        m_is_mode_available = false;
        is_model_converged = false;
        is_turn_off_plant = false;
        return;
    }

    if (pc_csp_solver->mc_pc_out_solver.m_m_dot_htf < m_dot_pc_min)
    {
        std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
            util::format(" converged to a HTF mass flow rate %lg [kg/s] less than the minimum PC HTF mass flow rate %lg [kg/s].",
                pc_csp_solver->mc_pc_out_solver.m_m_dot_htf / 3600.0, m_dot_pc_min / 3600.0);

        pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);

        m_is_mode_available = false;
        is_model_converged = false;
        is_turn_off_plant = false;
        return;
    }
}

void C_csp_solver::C_CR_SU__PC_RM_LO__TES_EMPTY__AUX_OFF::check_system_limits(C_csp_solver* pc_csp_solver,
    double q_dot_pc_su_max /*MWt*/, double m_dot_pc_max_startup /*kg/hr*/,
    double q_dot_pc_solve_target /*MWt*/, double q_dot_pc_on_dispatch_target,
    double q_dot_pc_max /*MWt*/, double q_dot_pc_min /*MWt*/, double q_dot_pc_sb /*MWt*/,
    double m_dot_pc_max /*kg/hr*/, double m_dot_pc_min /*kg/hr*/,
    double limit_comp_tol /*-*/,
    bool& is_model_converged, bool& is_turn_off_plant)
{
    if (pc_csp_solver->mc_pc_out_solver.m_q_dot_htf < q_dot_pc_min || pc_csp_solver->mc_pc_out_solver.m_m_dot_htf < m_dot_pc_min)
    {
        m_is_mode_available = false;
        is_model_converged = false;
        is_turn_off_plant = false;
        return;
    }

    // Check if solved thermal power is greater than target
    if (pc_csp_solver->mc_pc_out_solver.m_q_dot_htf > q_dot_pc_on_dispatch_target)
    {
        if (pc_csp_solver->mc_pc_out_solver.m_q_dot_htf > q_dot_pc_max)
        {
            std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
                util::format(" converged to a PC thermal power %lg [MWt] larger than the maximum PC thermal power %lg [MWt]. Controller shut off plant",
                    pc_csp_solver->mc_pc_out_solver.m_q_dot_htf, q_dot_pc_max);

            pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);

            m_is_mode_available = false;
            is_model_converged = false;
            is_turn_off_plant = true;
            return;
        }
        else
        {
            std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
                util::format(" converged to a PC thermal power %lg [MWt] larger than the target PC thermal power %lg [MWt] but less than the maximum thermal power %lg [MWt]",
                    pc_csp_solver->mc_pc_out_solver.m_q_dot_htf, q_dot_pc_on_dispatch_target, q_dot_pc_max);

            pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);
        }
    }

    if (pc_csp_solver->mc_pc_out_solver.m_m_dot_htf > m_dot_pc_max)
    {
        std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
            util::format(" converged to a HTF mass flow rate %lg [kg/s] larger than the maximum PC mass flow rate %lg [kg/s]. Controller shut off plant",
                pc_csp_solver->mc_pc_out_solver.m_m_dot_htf / 3600.0, m_dot_pc_max / 3600.0);

        pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);

        m_is_mode_available = false;
        is_model_converged = false;
        is_turn_off_plant = false;
        return;
    }
}

void C_csp_solver::C_CR_OFF__PC_SB__TES_DC__AUX_OFF::check_system_limits(C_csp_solver* pc_csp_solver,
    double q_dot_pc_su_max /*MWt*/, double m_dot_pc_max_startup /*kg/hr*/,
    double q_dot_pc_solve_target /*MWt*/, double q_dot_pc_on_dispatch_target,
    double q_dot_pc_max /*MWt*/, double q_dot_pc_min /*MWt*/, double q_dot_pc_sb /*MWt*/,
    double m_dot_pc_max /*kg/hr*/, double m_dot_pc_min /*kg/hr*/,
    double limit_comp_tol /*-*/,
    bool& is_model_converged, bool& is_turn_off_plant)
{
    double q_dot_pc_solved = pc_csp_solver->mc_pc_out_solver.m_q_dot_htf;	//[MWt]
    double m_dot_pc_solved = pc_csp_solver->mc_pc_out_solver.m_m_dot_htf;	//[kg/hr]

    // Check if solved thermal power is greater than target
    if ((q_dot_pc_solved - q_dot_pc_sb) / q_dot_pc_sb > 1.E-3)
    {
        if ((q_dot_pc_solved - q_dot_pc_max) / q_dot_pc_max > 1.E-3)
        {
            std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
                util::format(" converged to a PC thermal power %lg [MWt] larger than the maximum PC thermal power %lg [MWt]. Controller shut off plant",
                    q_dot_pc_solved, q_dot_pc_max);

            pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);

            m_is_mode_available = false;
            is_model_converged = false;
            is_turn_off_plant = true;
            return;
        }
        else
        {
            std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
                util::format(" converged to a PC thermal power %lg [MWt] larger than the target PC thermal power %lg [MWt] but less than the maximum thermal power %lg [MWt]",
                    q_dot_pc_solved, q_dot_pc_sb, q_dot_pc_max);

            pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);
        }
    }
    else if ((q_dot_pc_solved - q_dot_pc_sb) / q_dot_pc_sb < -1.E-3)
    {
        if (m_dot_pc_solved < m_dot_pc_max)
        {	// TES cannot provide enough thermal power - step down to next operating mode
            m_is_mode_available = false;
            is_model_converged = false;
            is_turn_off_plant = false;
            return;
        }
        // Notes:
        //else
        //{	// PC maximum mass flow is constraining the thermal power that TES can send the PC. Changing modes wont' help
        //
        //}
    }
}

void C_csp_solver::C_CR_DF__PC_MAX__TES_FULL__AUX_OFF::handle_solve_error(double time /*hr*/, bool& is_turn_off_rec_su)
{
    m_is_mode_available = false;
    is_turn_off_rec_su = true;
}

void C_csp_solver::C_CR_ON__PC_RM_HI__TES_OFF__AUX_OFF::handle_solve_error(double time /*hr*/, bool& is_turn_off_rec_su)
{
    m_is_HI_SIDE_mode_available = false;
    is_turn_off_rec_su = false;
}

void C_csp_solver::C_CR_ON__PC_RM_HI__TES_OFF__AUX_OFF::check_system_limits(C_csp_solver* pc_csp_solver,
    double q_dot_pc_su_max /*MWt*/, double m_dot_pc_max_startup /*kg/hr*/,
    double q_dot_pc_solve_target /*MWt*/, double q_dot_pc_on_dispatch_target,
    double q_dot_pc_max /*MWt*/, double q_dot_pc_min /*MWt*/, double q_dot_pc_sb /*MWt*/,
    double m_dot_pc_max /*kg/hr*/, double m_dot_pc_min /*kg/hr*/,
    double limit_comp_tol /*-*/,
    bool& is_model_converged, bool& is_turn_off_plant)
{
    if(pc_csp_solver->mc_pc_out_solver.m_q_dot_htf > q_dot_pc_max || pc_csp_solver->mc_pc_out_solver.m_m_dot_htf > m_dot_pc_max)
    //if (pc_csp_solver->mc_cr_out_solver.m_q_thermal > q_dot_pc_max || pc_csp_solver->mc_cr_out_solver.m_m_dot_salt_tot > m_dot_pc_max)
    {
        m_is_HI_SIDE_mode_available = false;
        is_model_converged = false;
        is_turn_off_plant = false;
        return;
    }
    else if (pc_csp_solver->mc_pc_out_solver.m_q_dot_htf < q_dot_pc_on_dispatch_target)
    //else if (pc_csp_solver->mc_cr_out_solver.m_q_thermal < q_dot_pc_on_dispatch_target)
    {
        m_is_LO_SIDE_mode_available = false;
        is_model_converged = false;
        is_turn_off_plant = false;
        return;
    }
}

void C_csp_solver::C_CR_ON__PC_RM_HI__TES_FULL__AUX_OFF::check_system_limits(C_csp_solver* pc_csp_solver,
    double q_dot_pc_su_max /*MWt*/, double m_dot_pc_max_startup /*kg/hr*/,
    double q_dot_pc_solve_target /*MWt*/, double q_dot_pc_on_dispatch_target,
    double q_dot_pc_max /*MWt*/, double q_dot_pc_min /*MWt*/, double q_dot_pc_sb /*MWt*/,
    double m_dot_pc_max /*kg/hr*/, double m_dot_pc_min /*kg/hr*/,
    double limit_comp_tol /*-*/,
    bool& is_model_converged, bool& is_turn_off_plant)
{
    if ((pc_csp_solver->mc_pc_out_solver.m_q_dot_htf - q_dot_pc_max) / q_dot_pc_max > limit_comp_tol)
    {
        m_is_mode_available = false;
        is_model_converged = false;
        is_turn_off_plant = false;
        return;
    }
    else if (pc_csp_solver->mc_pc_out_solver.m_q_dot_htf < q_dot_pc_on_dispatch_target)
    {
        std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
            util::format(" converged to a power cycle thermal input %lg [MWt] less than the target %lg [MWt].",
                pc_csp_solver->mc_pc_out_solver.m_q_dot_htf, q_dot_pc_on_dispatch_target);
        pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);
    }
}

void C_csp_solver::C_CR_ON__PC_SB__TES_CH__AUX_OFF::check_system_limits(C_csp_solver* pc_csp_solver,
    double q_dot_pc_su_max /*MWt*/, double m_dot_pc_max_startup /*kg/hr*/,
    double q_dot_pc_solve_target /*MWt*/, double q_dot_pc_on_dispatch_target,
    double q_dot_pc_max /*MWt*/, double q_dot_pc_min /*MWt*/, double q_dot_pc_sb /*MWt*/,
    double m_dot_pc_max /*kg/hr*/, double m_dot_pc_min /*kg/hr*/,
    double limit_comp_tol /*-*/,
    bool& is_model_converged, bool& is_turn_off_plant)
{
    double q_dot_pc_solved = pc_csp_solver->mc_pc_out_solver.m_q_dot_htf;		//[MWt]
    double m_dot_pc_solved = pc_csp_solver->mc_pc_out_solver.m_m_dot_htf;		//[kg/hr]

    if (std::abs(q_dot_pc_solved - q_dot_pc_sb) / q_dot_pc_sb < limit_comp_tol)
    {	// If successfully solved for target thermal power, check that mass flow is above minimum
        if ((m_dot_pc_solved - m_dot_pc_min) / std::max(0.01, m_dot_pc_min) < -limit_comp_tol)
        {
            std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
                util::format(" solved with a PC HTF mass flow rate %lg [kg/s] smaller than the minimum %lg [kg/s]. Controller shut off plant",
                    m_dot_pc_solved / 3600.0, m_dot_pc_min / 3600.0);

            pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);

            m_is_mode_available = false;
            is_model_converged = false;
            is_turn_off_plant = true;
            return;
        }
    }
    else if ((q_dot_pc_solved - q_dot_pc_sb) / q_dot_pc_sb < -1.E-3)
    {
        m_is_mode_available = false;
        is_model_converged = false;
        is_turn_off_plant = false;
        return;
    }
}

void C_csp_solver::C_CR_ON__PC_SB__TES_FULL__AUX_OFF::check_system_limits(C_csp_solver* pc_csp_solver,
    double q_dot_pc_su_max /*MWt*/, double m_dot_pc_max_startup /*kg/hr*/,
    double q_dot_pc_solve_target /*MWt*/, double q_dot_pc_on_dispatch_target,
    double q_dot_pc_max /*MWt*/, double q_dot_pc_min /*MWt*/, double q_dot_pc_sb /*MWt*/,
    double m_dot_pc_max /*kg/hr*/, double m_dot_pc_min /*kg/hr*/,
    double limit_comp_tol /*-*/,
    bool& is_model_converged, bool& is_turn_off_plant)
{
    if ((pc_csp_solver->mc_pc_out_solver.m_q_dot_htf - q_dot_pc_max) / q_dot_pc_max > limit_comp_tol)
    {
        m_is_mode_available = false;
        is_model_converged = false;
        is_turn_off_plant = false;
        return;
    }
    else if (pc_csp_solver->mc_pc_out_solver.m_q_dot_htf < q_dot_pc_sb)
    {
        m_is_mode_available = false;
        is_model_converged = false;
        is_turn_off_plant = false;
        return;
    }
}

void C_csp_solver::C_CR_ON__PC_SB__TES_DC__AUX_OFF::check_system_limits(C_csp_solver* pc_csp_solver,
    double q_dot_pc_su_max /*MWt*/, double m_dot_pc_max_startup /*kg/hr*/,
    double q_dot_pc_solve_target /*MWt*/, double q_dot_pc_on_dispatch_target,
    double q_dot_pc_max /*MWt*/, double q_dot_pc_min /*MWt*/, double q_dot_pc_sb /*MWt*/,
    double m_dot_pc_max /*kg/hr*/, double m_dot_pc_min /*kg/hr*/,
    double limit_comp_tol /*-*/,
    bool& is_model_converged, bool& is_turn_off_plant)
{
    double q_dot_pc_solved = pc_csp_solver->mc_pc_out_solver.m_q_dot_htf;	//[MWt]
    double m_dot_pc_solved = pc_csp_solver->mc_pc_out_solver.m_m_dot_htf;	//[kg/hr]

    // Check bounds on solved thermal power and mass flow rate
    if ((q_dot_pc_solved - q_dot_pc_sb) / q_dot_pc_sb > limit_comp_tol)
    {
        if ((q_dot_pc_solved - q_dot_pc_max) / q_dot_pc_max > limit_comp_tol)
        {
            std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
                util::format(" solved with a PC thermal power %lg [MWt] greater than the maximum %lg [MWt]. Controller shut off plant",
                    q_dot_pc_solved, q_dot_pc_max);
            pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);

            m_is_mode_available = false;
            is_model_converged = false;
            is_turn_off_plant = true;
            return;
        }
        else
        {
            std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
                util::format(" solved with a PC thermal power %lg [MWt] greater than the target %lg [MWt]",
                    q_dot_pc_solved, q_dot_pc_sb);
            pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);
        }
    }
    if (m_dot_pc_solved < m_dot_pc_min)
    {	// If we're already hitting the minimum mass flow rate, then trying next operating mode won't help
        std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
            util::format(" solved with a PC HTF mass flow rate %lg [kg/s] less than the minimum %lg [kg/s]. Controller shut off plant",
                m_dot_pc_solved / 3600.0, m_dot_pc_min / 3600.0);
        pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);

        m_is_mode_available = false;
        is_model_converged = false;
        is_turn_off_plant = true;
        return;
    }

    if ((q_dot_pc_solved - q_dot_pc_sb) / q_dot_pc_sb < -limit_comp_tol)
    {
        m_is_mode_available = false;
        is_model_converged = false;
        is_turn_off_plant = false;
        return;
    }

    if (m_dot_pc_solved > m_dot_pc_max)
    {	// Shouldn't happen but can try next operating mode
        m_is_mode_available = false;
        is_model_converged = false;
        is_turn_off_plant = false;
        return;
    }
}

void C_csp_solver::C_CR_SU__PC_SB__TES_DC__AUX_OFF::check_system_limits(C_csp_solver* pc_csp_solver,
    double q_dot_pc_su_max /*MWt*/, double m_dot_pc_max_startup /*kg/hr*/,
    double q_dot_pc_solve_target /*MWt*/, double q_dot_pc_on_dispatch_target,
    double q_dot_pc_max /*MWt*/, double q_dot_pc_min /*MWt*/, double q_dot_pc_sb /*MWt*/,
    double m_dot_pc_max /*kg/hr*/, double m_dot_pc_min /*kg/hr*/,
    double limit_comp_tol /*-*/,
    bool& is_model_converged, bool& is_turn_off_plant)
{
    double q_dot_pc_solved = pc_csp_solver->mc_pc_out_solver.m_q_dot_htf;	//[MWt]
    double m_dot_pc_solved = pc_csp_solver->mc_pc_out_solver.m_m_dot_htf;	//[kg/hr]

    // Check if solved thermal power is greater than target
    if ((q_dot_pc_solved - q_dot_pc_sb) / q_dot_pc_sb > limit_comp_tol)
    {
        if ((q_dot_pc_solved - q_dot_pc_max) / q_dot_pc_max > limit_comp_tol)
        {
            std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
                util::format(" converged to a PC thermal power %lg [MWt] larger than the maximum PC thermal power %lg [MWt]. Controller shut off plant",
                    q_dot_pc_solved, q_dot_pc_max);
            pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);

            m_is_mode_available = false;
            is_model_converged = false;
            is_turn_off_plant = true;
            return;
        }
        else
        {
            std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
                util::format(" converged to a PC thermal power %lg [MWt] larger than the target PC thermal power %lg [MWt] but less than the maximum thermal power %lg [MWt]",
                    q_dot_pc_solved, q_dot_pc_sb, q_dot_pc_max);

            pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);
        }
    }
    else if ((q_dot_pc_solved - q_dot_pc_sb) / q_dot_pc_sb < -limit_comp_tol)
    {
        if (m_dot_pc_solved < m_dot_pc_max)
        {	// TES cannot provide enough thermal power - step down to next operating mode
            m_is_mode_available = false;
            is_model_converged = false;
            is_turn_off_plant = false;
            return;
        }
        // Notes:
        //else
        //{	// PC maximum mass flow is constraining the thermal power that TES can send the PC. Changing modes wont' help
        //
        //}
    }
}

void C_csp_solver::C_CR_ON__PC_SB__TES_OFF__AUX_OFF::check_system_limits(C_csp_solver* pc_csp_solver,
    double q_dot_pc_su_max /*MWt*/, double m_dot_pc_max_startup /*kg/hr*/,
    double q_dot_pc_solve_target /*MWt*/, double q_dot_pc_on_dispatch_target,
    double q_dot_pc_max /*MWt*/, double q_dot_pc_min /*MWt*/, double q_dot_pc_sb /*MWt*/,
    double m_dot_pc_max /*kg/hr*/, double m_dot_pc_min /*kg/hr*/,
    double limit_comp_tol /*-*/,
    bool& is_model_converged, bool& is_turn_off_plant)
{
    // Check that cr and pc mass flow rates balance
    //if (fabs(pc_csp_solver->mc_cr_out_solver.m_m_dot_salt_tot - pc_csp_solver->mc_pc_out_solver.m_m_dot_htf) / pc_csp_solver->m_m_dot_pc_des > limit_comp_tol / 10.)
    //{
    //    m_is_mode_available = false;
    //    is_model_converged = false;
    //    is_turn_off_plant = false;
    //    return;
    //}

    // Check if solved thermal power is greater than target
    if ((pc_csp_solver->mc_pc_out_solver.m_q_dot_htf - q_dot_pc_max) > limit_comp_tol)
    {
        std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
            util::format(" converged to a PC thermal power %lg [MWt] larger than the maximum PC thermal power %lg [MWt]. Controller shut off plant",
                pc_csp_solver->mc_pc_out_solver.m_q_dot_htf, q_dot_pc_max);

        pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);

        m_is_mode_available = false;
        is_model_converged = false;
        is_turn_off_plant = true;
        return;
    }

    if (pc_csp_solver->mc_pc_out_solver.m_m_dot_htf > m_dot_pc_max)
    {
        std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
            util::format(" converged to a HTF mass flow rate %lg [kg/s] larger than the maximum PC mass flow rate %lg [kg/s]. Controller shut off plant",
                pc_csp_solver->mc_pc_out_solver.m_m_dot_htf / 3600.0, m_dot_pc_max / 3600.0);

        pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);

        m_is_mode_available = false;
        is_model_converged = false;
        is_turn_off_plant = true;
        return;
    }

    // Check if solved thermal power is less than target
    if ((pc_csp_solver->mc_pc_out_solver.m_q_dot_htf - q_dot_pc_sb) / q_dot_pc_sb < -limit_comp_tol)
    {
        std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
            util::format(" converged to a PC thermal power %lg [MWt] less than the minimum PC thermal power %lg [MWt].",
                pc_csp_solver->mc_pc_out_solver.m_q_dot_htf, q_dot_pc_min);

        pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);

        m_is_mode_available = false;
        is_model_converged = false;
        is_turn_off_plant = false;
        return;
    }

    if (pc_csp_solver->mc_pc_out_solver.m_m_dot_htf < m_dot_pc_min)
    {
        std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
            util::format(" converged to a HTF mass flow rate %lg [kg/s] less than the minimum PC HTF mass flow rate %lg [kg/s].",
                pc_csp_solver->mc_pc_out_solver.m_m_dot_htf / 3600.0, m_dot_pc_min / 3600.0);

        pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);

        m_is_mode_available = false;
        is_model_converged = false;
        is_turn_off_plant = false;
        return;
    }
}

void C_csp_solver::C_CR_TO_COLD__PC_OFF__TES_OFF__AUX_OFF::handle_solve_error(double time /*hr*/, bool& is_rec_su_unchanged)
{
    throw(C_csp_exception(util::format("At time = %lg ", time / 3600.0) + " " + m_op_mode_name + " failed", ""));
}

void C_csp_solver::C_CR_TO_COLD__PC_MIN__TES_EMPTY__AUX_OFF::check_system_limits(C_csp_solver* pc_csp_solver,
    double q_dot_pc_su_max /*MWt*/, double m_dot_pc_max_startup /*kg/hr*/,
    double q_dot_pc_solve_target /*MWt*/, double q_dot_pc_on_dispatch_target,
    double q_dot_pc_max /*MWt*/, double q_dot_pc_min /*MWt*/, double q_dot_pc_sb /*MWt*/,
    double m_dot_pc_max /*kg/hr*/, double m_dot_pc_min /*kg/hr*/,
    double limit_comp_tol /*-*/,
    bool& is_model_converged, bool& is_turn_off_plant)
{
    // Check if solved thermal power is greater than target
    if (pc_csp_solver->mc_pc_out_solver.m_q_dot_htf > q_dot_pc_min)
    {
        if (pc_csp_solver->mc_pc_out_solver.m_q_dot_htf > q_dot_pc_max)
        {
            std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
                util::format(" converged to a PC thermal power %lg [MWt] larger than the maximum PC thermal power %lg [MWt]. Controller shut off plant",
                    pc_csp_solver->mc_pc_out_solver.m_q_dot_htf, q_dot_pc_max);

            pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);

            m_is_mode_available = false;
            is_model_converged = false;
            is_turn_off_plant = true;
            return;
        }
    }

    if (pc_csp_solver->mc_pc_out_solver.m_m_dot_htf > m_dot_pc_max)
    {
        std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
            util::format(" converged to a HTF mass flow rate %lg [kg/s] larger than the maximum PC mass flow rate %lg [kg/s]. Controller shut off plant",
                pc_csp_solver->mc_pc_out_solver.m_m_dot_htf / 3600.0, m_dot_pc_max / 3600.0);

        pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);

        m_is_mode_available = false;
        is_model_converged = false;
        is_turn_off_plant = true;
        return;
    }
}

void C_csp_solver::C_CR_TO_COLD__PC_RM_LO__TES_EMPTY__AUX_OFF::check_system_limits(C_csp_solver* pc_csp_solver,
    double q_dot_pc_su_max /*MWt*/, double m_dot_pc_max_startup /*kg/hr*/,
    double q_dot_pc_solve_target /*MWt*/, double q_dot_pc_on_dispatch_target,
    double q_dot_pc_max /*MWt*/, double q_dot_pc_min /*MWt*/, double q_dot_pc_sb /*MWt*/,
    double m_dot_pc_max /*kg/hr*/, double m_dot_pc_min /*kg/hr*/,
    double limit_comp_tol /*-*/,
    bool& is_model_converged, bool& is_turn_off_plant)
{
    if (pc_csp_solver->mc_pc_out_solver.m_q_dot_htf < q_dot_pc_min || pc_csp_solver->mc_pc_out_solver.m_m_dot_htf < m_dot_pc_min)
    {
        m_is_mode_available = false;
        is_model_converged = false;
        is_turn_off_plant = false;
        return;
    }

    // Check if solved thermal power is greater than target
    if (pc_csp_solver->mc_pc_out_solver.m_q_dot_htf > q_dot_pc_on_dispatch_target)
    {
        if (pc_csp_solver->mc_pc_out_solver.m_q_dot_htf > q_dot_pc_max)
        {
            std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
                util::format(" converged to a PC thermal power %lg [MWt] larger than the maximum PC thermal power %lg [MWt]. Controller shut off plant",
                    pc_csp_solver->mc_pc_out_solver.m_q_dot_htf, q_dot_pc_max);

            pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);

            m_is_mode_available = false;
            is_model_converged = false;
            is_turn_off_plant = true;
            return;
        }
        else
        {
            std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
                util::format(" converged to a PC thermal power %lg [MWt] larger than the target PC thermal power %lg [MWt] but less than the maximum thermal power %lg [MWt]",
                    pc_csp_solver->mc_pc_out_solver.m_q_dot_htf, q_dot_pc_on_dispatch_target, q_dot_pc_max);
            pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);
        }
    }

    if (pc_csp_solver->mc_pc_out_solver.m_m_dot_htf > m_dot_pc_max)
    {
        std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
            util::format(" converged to a HTF mass flow rate %lg [kg/s] larger than the maximum PC mass flow rate %lg [kg/s]. Controller shut off plant",
                pc_csp_solver->mc_pc_out_solver.m_m_dot_htf / 3600.0, m_dot_pc_max / 3600.0);

        pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);

        m_is_mode_available = false;
        is_model_converged = false;
        is_turn_off_plant = true;
        return;
    }
}

void C_csp_solver::C_CR_TO_COLD__PC_TARGET__TES_DC__AUX_OFF::check_system_limits(C_csp_solver* pc_csp_solver,
    double q_dot_pc_su_max /*MWt*/, double m_dot_pc_max_startup /*kg/hr*/,
    double q_dot_pc_solve_target /*MWt*/, double q_dot_pc_on_dispatch_target,
    double q_dot_pc_max /*MWt*/, double q_dot_pc_min /*MWt*/, double q_dot_pc_sb /*MWt*/,
    double m_dot_pc_max /*kg/hr*/, double m_dot_pc_min /*kg/hr*/,
    double limit_comp_tol /*-*/,
    bool& is_model_converged, bool& is_turn_off_plant)
{
    double q_dot_pc_solved = pc_csp_solver->mc_pc_out_solver.m_q_dot_htf;	//[MWt]
    double m_dot_pc_solved = pc_csp_solver->mc_pc_out_solver.m_m_dot_htf;	//[kg/hr]

    // Check if solved thermal power is greater than target
    if ((q_dot_pc_solved - q_dot_pc_on_dispatch_target) / q_dot_pc_on_dispatch_target > limit_comp_tol)
    {
        if ((q_dot_pc_solved - q_dot_pc_max) / q_dot_pc_max > limit_comp_tol)
        {
            std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
                util::format(" converged to a PC thermal power %lg [MWt] larger than the maximum PC thermal power %lg [MWt]. Controller shut off plant",
                    q_dot_pc_solved, q_dot_pc_max);
            pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);

            m_is_mode_available = false;
            is_model_converged = false;
            is_turn_off_plant = true;
            return;
        }
        else
        {
            std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
                util::format(" converged to a PC thermal power %lg [MWt] larger than the target PC thermal power %lg [MWt] but less than the maximum thermal power %lg [MWt]",
                    q_dot_pc_solved, q_dot_pc_on_dispatch_target, q_dot_pc_max);

            pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);
        }
    }
    else if ((q_dot_pc_solved - q_dot_pc_on_dispatch_target) / q_dot_pc_on_dispatch_target < -limit_comp_tol)
    {
        if (m_dot_pc_solved < m_dot_pc_max)
        {	// TES cannot provide enough thermal power - step down to next operating mode
            m_is_mode_available = false;
            is_model_converged = false;
            is_turn_off_plant = false;
            return;
        }
        // Notes:
        //else
        //{	// PC maximum mass flow is constraining the thermal power that TES can send the PC. Changing modes wont' help
        //
        //}
    }
}

void C_csp_solver::C_CR_TO_COLD__PC_SB__TES_DC__AUX_OFF::check_system_limits(C_csp_solver* pc_csp_solver,
    double q_dot_pc_su_max /*MWt*/, double m_dot_pc_max_startup /*kg/hr*/,
    double q_dot_pc_solve_target /*MWt*/, double q_dot_pc_on_dispatch_target,
    double q_dot_pc_max /*MWt*/, double q_dot_pc_min /*MWt*/, double q_dot_pc_sb /*MWt*/,
    double m_dot_pc_max /*kg/hr*/, double m_dot_pc_min /*kg/hr*/,
    double limit_comp_tol /*-*/,
    bool& is_model_converged, bool& is_turn_off_plant)
{
    double q_dot_pc_solved = pc_csp_solver->mc_pc_out_solver.m_q_dot_htf;	//[MWt]
    double m_dot_pc_solved = pc_csp_solver->mc_pc_out_solver.m_m_dot_htf;	//[kg/hr]

    std::string error_msg = util::format("At time = %lg [hr]", pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time / 3600.0) + ", the plant controller tried operating mode " + m_op_mode_name + " which hasn't been tested";
    pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);

    // Check if solved thermal power is greater than target
    if ((q_dot_pc_solved - q_dot_pc_sb) / q_dot_pc_sb > limit_comp_tol)
    {
        if ((q_dot_pc_solved - q_dot_pc_max) / q_dot_pc_max > limit_comp_tol)
        {
            std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
                util::format(" converged to a PC thermal power %lg [MWt] larger than the maximum PC thermal power %lg [MWt]. Controller shut off plant",
                    q_dot_pc_solved, q_dot_pc_max);
            pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);

            m_is_mode_available = false;
            is_model_converged = false;
            is_turn_off_plant = true;
            return;
        }
        else
        {
            std::string error_msg = time_and_op_mode_to_string(pc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time) +
                util::format(" converged to a PC thermal power %lg [MWt] larger than the target PC thermal power %lg [MWt] but less than the maximum thermal power %lg [MWt]",
                    q_dot_pc_solved, q_dot_pc_sb, q_dot_pc_max);

            pc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, error_msg);
        }
    }
    else if ((q_dot_pc_solved - q_dot_pc_sb) / q_dot_pc_sb < -limit_comp_tol)
    {
        if (m_dot_pc_solved < m_dot_pc_max)
        {	// TES cannot provide enough thermal power - step down to next operating mode

            m_is_mode_available = false;
            is_model_converged = false;
            is_turn_off_plant = false;
            return;
        }
        // Notes:
        //else
        //{	// PC maximum mass flow is constraining the thermal power that TES can send the PC. Changing modes wont' help
        //
        //}
    }
}

C_csp_solver::C_operating_mode_core* C_csp_solver::C_system_operating_modes::get_pointer_to_op_mode(E_operating_modes op_mode)
{
    switch (op_mode)
    {
    case CR_OFF__PC_OFF__TES_OFF__AUX_OFF:
        return &mc_CR_OFF__PC_OFF__TES_OFF__AUX_OFF;
    case CR_SU__PC_OFF__TES_OFF__AUX_OFF:
        return &mc_CR_SU__PC_OFF__TES_OFF__AUX_OFF;
    case CR_ON__PC_SU__TES_OFF__AUX_OFF:
        return &mc_CR_ON__PC_SU__TES_OFF__AUX_OFF;
    case CR_ON__PC_SB__TES_OFF__AUX_OFF:
        return &mc_CR_ON__PC_SB__TES_OFF__AUX_OFF;
    case CR_ON__PC_RM_HI__TES_OFF__AUX_OFF:
        return &mc_CR_ON__PC_RM_HI__TES_OFF__AUX_OFF;
    case CR_ON__PC_RM_LO__TES_OFF__AUX_OFF:
        return &mc_CR_ON__PC_RM_LO__TES_OFF__AUX_OFF;
    case CR_DF__PC_MAX__TES_OFF__AUX_OFF:
        return &mc_CR_DF__PC_MAX__TES_OFF__AUX_OFF;
    case CR_OFF__PC_SU__TES_DC__AUX_OFF:
        return &mc_CR_OFF__PC_SU__TES_DC__AUX_OFF;
    case CR_ON__PC_OFF__TES_CH__AUX_OFF:
        return &mc_CR_ON__PC_OFF__TES_CH__AUX_OFF;
    case CR_ON__PC_TARGET__TES_CH__AUX_OFF:
        return &mc_CR_ON__PC_TARGET__TES_CH__AUX_OFF;
    case CR_ON__PC_TARGET__TES_DC__AUX_OFF:
        return &mc_CR_ON__PC_TARGET__TES_DC__AUX_OFF;
    case CR_ON__PC_RM_LO__TES_EMPTY__AUX_OFF:
        return &mc_CR_ON__PC_RM_LO__TES_EMPTY__AUX_OFF;
    case CR_DF__PC_OFF__TES_FULL__AUX_OFF:
        return &mc_CR_DF__PC_OFF__TES_FULL__AUX_OFF;
    case CR_OFF__PC_SB__TES_DC__AUX_OFF:
        return &mc_CR_OFF__PC_SB__TES_DC__AUX_OFF;
    case CR_OFF__PC_MIN__TES_EMPTY__AUX_OFF:
        return &mc_CR_OFF__PC_MIN__TES_EMPTY__AUX_OFF;
    case CR_OFF__PC_RM_LO__TES_EMPTY__AUX_OFF:
        return &mc_CR_OFF__PC_RM_LO__TES_EMPTY__AUX_OFF;
    case CR_ON__PC_SB__TES_CH__AUX_OFF:
        return &mc_CR_ON__PC_SB__TES_CH__AUX_OFF;
    case CR_SU__PC_MIN__TES_EMPTY__AUX_OFF:
        return &mc_CR_SU__PC_MIN__TES_EMPTY__AUX_OFF;
    case CR_SU__PC_SB__TES_DC__AUX_OFF:
        return &mc_CR_SU__PC_SB__TES_DC__AUX_OFF;
    case CR_ON__PC_SB__TES_DC__AUX_OFF:
        return &mc_CR_ON__PC_SB__TES_DC__AUX_OFF;
    case CR_OFF__PC_TARGET__TES_DC__AUX_OFF:
        return &mc_CR_OFF__PC_TARGET__TES_DC__AUX_OFF;
    case CR_SU__PC_TARGET__TES_DC__AUX_OFF:
        return &mc_CR_SU__PC_TARGET__TES_DC__AUX_OFF;
    case CR_ON__PC_RM_HI__TES_FULL__AUX_OFF:
        return &mc_CR_ON__PC_RM_HI__TES_FULL__AUX_OFF;
    case CR_ON__PC_MIN__TES_EMPTY__AUX_OFF:
        return &mc_CR_ON__PC_MIN__TES_EMPTY__AUX_OFF;
    case CR_SU__PC_RM_LO__TES_EMPTY__AUX_OFF:
        return &mc_CR_SU__PC_RM_LO__TES_EMPTY__AUX_OFF;
    case CR_DF__PC_MAX__TES_FULL__AUX_OFF:
        return &mc_CR_DF__PC_MAX__TES_FULL__AUX_OFF;
    case CR_ON__PC_SB__TES_FULL__AUX_OFF:
        return &mc_CR_ON__PC_SB__TES_FULL__AUX_OFF;
    case CR_SU__PC_SU__TES_DC__AUX_OFF:
        return &mc_CR_SU__PC_SU__TES_DC__AUX_OFF;
    case CR_ON__PC_SU__TES_CH__AUX_OFF:
        return &mc_CR_ON__PC_SU__TES_CH__AUX_OFF;
    case CR_DF__PC_SU__TES_FULL__AUX_OFF:
        return &mc_CR_DF__PC_SU__TES_FULL__AUX_OFF;
    case CR_DF__PC_SU__TES_OFF__AUX_OFF:
        return &mc_CR_DF__PC_SU__TES_OFF__AUX_OFF;
    case CR_TO_COLD__PC_TARGET__TES_DC__AUX_OFF:
        return &mc_CR_TO_COLD__PC_TARGET__TES_DC__AUX_OFF;
    case CR_TO_COLD__PC_RM_LO__TES_EMPTY__AUX_OFF:
        return &mc_CR_TO_COLD__PC_RM_LO__TES_EMPTY__AUX_OFF;
    case CR_TO_COLD__PC_SB__TES_DC__AUX_OFF:
        return &mc_CR_TO_COLD__PC_SB__TES_DC__AUX_OFF;
    case CR_TO_COLD__PC_MIN__TES_EMPTY__AUX_OFF:
        return &mc_CR_TO_COLD__PC_MIN__TES_EMPTY__AUX_OFF;
    case CR_TO_COLD__PC_OFF__TES_OFF__AUX_OFF:
        return &mc_CR_TO_COLD__PC_OFF__TES_OFF__AUX_OFF;
    case CR_TO_COLD__PC_SU__TES_DC__AUX_OFF:
        return &mc_CR_TO_COLD__PC_SU__TES_DC__AUX_OFF;
    case CR_OFF__PC_OFF__TES_CH__HTR_ON:
        return &mc_CR_OFF__PC_OFF__TES_CH__HTR_ON;
    case CR_SU__PC_OFF__TES_CH__HTR_ON:
        return &mc_CR_SU__PC_OFF__TES_CH__HTR_ON;
    case CR_ON__PC_OFF__TES_CH__HTR_ON:
        return &mc_CR_ON__PC_OFF__TES_CH__HTR_ON;
    case CR_OFF__PC_OFF__TES_FULL__HTR_DF:
        return &mc_CR_OFF__PC_OFF__TES_FULL__HTR_DF;
    case CR_ON__PC_OFF__TES_FULL__HTR_DF:
        return &mc_CR_ON__PC_OFF__TES_FULL__HTR_DF;
    case CR_SU__PC_OFF__TES_FULL__HTR_DF:
        return &mc_CR_SU__PC_OFF__TES_FULL__HTR_DF;
    default:
        throw(C_csp_exception("Operating mode class not defined"));
    }
}

bool C_csp_solver::C_system_operating_modes::solve(C_system_operating_modes::E_operating_modes op_mode, C_csp_solver* pc_csp_solver, bool is_rec_outlet_to_hottank,
    double q_dot_pc_on_target /*MWt*/, double q_dot_pc_startup /*MWt*/, double q_dot_pc_standby /*MWt*/,
    double q_dot_pc_min /*MWt*/, double q_dot_pc_max /*MWt*/, double q_dot_pc_startup_max /*MWt*/,
    double m_dot_pc_startup_max /*kg/hr*/, double m_dot_pc_max /*kg/hr*/, double m_dot_pc_min /*kg/hr*/,
    double q_dot_elec_to_CR_heat /*MWt*/, double q_dot_elec_to_PAR_HTR /*MWt*/, double limit_comp_tol /*-*/,
    double& defocus_solved, bool& is_op_mode_avail /*-*/, bool& is_turn_off_plant, bool& is_turn_off_rec_su)
{
    return get_pointer_to_op_mode(op_mode)->solve(pc_csp_solver, is_rec_outlet_to_hottank,
        q_dot_pc_on_target, q_dot_pc_startup, q_dot_pc_standby,
        q_dot_pc_min, q_dot_pc_max, q_dot_pc_startup_max,
        m_dot_pc_startup_max, m_dot_pc_max, m_dot_pc_min,
        q_dot_elec_to_CR_heat, q_dot_elec_to_PAR_HTR, limit_comp_tol,
        defocus_solved, is_op_mode_avail, is_turn_off_plant, is_turn_off_rec_su);
}

void C_csp_solver::C_system_operating_modes::reset_all_availability()
{
    for (int it = ITER_START + 1; it != E_operating_modes::ITER_END; it++) {
        get_pointer_to_op_mode(static_cast<E_operating_modes>(it))->turn_on_mode_availability();
    }
}

void C_csp_solver::C_system_operating_modes::turn_off_plant()
{
    for (int it = ITER_START + 1; it != E_operating_modes::ITER_END; it++) {
        get_pointer_to_op_mode(static_cast<E_operating_modes>(it))->turn_off_mode_availability();
    }

}

C_csp_solver::C_system_operating_modes::E_operating_modes C_csp_solver::C_system_operating_modes::cr_and_pc_stay_off__try_htr
(double q_dot_tes_ch /*MWt*/, double tol_mode_switching /*-*/,
 bool is_PAR_HTR_allowed, double q_dot_PAR_HTR_on /*MWt*/)
{
    C_csp_solver::C_system_operating_modes::E_operating_modes operating_mode;

    if (is_PAR_HTR_allowed && q_dot_PAR_HTR_on > 0.0 && q_dot_tes_ch > 0.0) {
        if (q_dot_PAR_HTR_on * (1. - tol_mode_switching) < q_dot_tes_ch &&
            is_mode_avail(CR_OFF__PC_OFF__TES_CH__HTR_ON)) {

            operating_mode = C_system_operating_modes::CR_OFF__PC_OFF__TES_CH__HTR_ON;
        }
        else if (is_mode_avail(CR_OFF__PC_OFF__TES_FULL__HTR_DF)) {

            operating_mode = CR_OFF__PC_OFF__TES_FULL__HTR_DF;
        }
        else {
            operating_mode = C_system_operating_modes::CR_OFF__PC_OFF__TES_OFF__AUX_OFF;
        }
    }
    else {
        operating_mode = C_system_operating_modes::CR_OFF__PC_OFF__TES_OFF__AUX_OFF;
    }

    return operating_mode;
}

C_csp_solver::C_system_operating_modes::E_operating_modes C_csp_solver::C_system_operating_modes::pc_off__try_cr_su_with_htr_combs
(double q_dot_tes_ch /*MWt*/, double tol_mode_switching /*-*/,
 bool is_PAR_HTR_allowed, double q_dot_PAR_HTR_on /*MWt*/)
{
    C_csp_solver::C_system_operating_modes::E_operating_modes operating_mode;

    if (is_PAR_HTR_allowed && q_dot_tes_ch > 0.0 && q_dot_PAR_HTR_on > 0.0 &&
    is_mode_avail(C_system_operating_modes::CR_SU__PC_OFF__TES_CH__HTR_ON))
    {
        if (q_dot_PAR_HTR_on * (1. + tol_mode_switching) > q_dot_tes_ch &&
            is_mode_avail(C_system_operating_modes::CR_SU__PC_OFF__TES_FULL__HTR_DF)) {

            operating_mode = C_system_operating_modes::CR_SU__PC_OFF__TES_FULL__HTR_DF;
        }
        else {
            operating_mode = C_system_operating_modes::CR_SU__PC_OFF__TES_CH__HTR_ON;
        }
    }
    else
    {
        operating_mode = C_system_operating_modes::CR_SU__PC_OFF__TES_OFF__AUX_OFF;
    }

    return operating_mode;
}

C_csp_solver::C_system_operating_modes::E_operating_modes C_csp_solver::C_system_operating_modes::cr_on_pc_off_tes_ch_avail__try_htr
(double q_dot_cr_on /*MWt*/, double q_dot_tes_ch /*MWt*/, double tol_mode_switching /*-*/,
 bool is_PAR_HTR_allowed, double q_dot_PAR_HTR_on /*MWt*/)
{
    C_csp_solver::C_system_operating_modes::E_operating_modes operating_mode;

    if (is_PAR_HTR_allowed && q_dot_PAR_HTR_on > 0.0 &&
        is_mode_avail(C_system_operating_modes::CR_DF__PC_OFF__TES_FULL__AUX_OFF)) {

        if ((q_dot_cr_on + q_dot_PAR_HTR_on) * (1.0 - tol_mode_switching) < q_dot_tes_ch &&
            is_mode_avail(C_system_operating_modes::CR_ON__PC_OFF__TES_CH__HTR_ON)) {

            operating_mode = C_system_operating_modes::CR_ON__PC_OFF__TES_CH__HTR_ON;
        }
        else if (q_dot_cr_on * (1. - tol_mode_switching) < q_dot_tes_ch &&
            is_mode_avail(C_system_operating_modes::CR_ON__PC_OFF__TES_FULL__HTR_DF)) {

            operating_mode = C_system_operating_modes::CR_ON__PC_OFF__TES_FULL__HTR_DF;
        }
        else {
            operating_mode = C_system_operating_modes::CR_DF__PC_OFF__TES_FULL__AUX_OFF;
        }
    }
    else {
        if (q_dot_cr_on * (1.0 - tol_mode_switching) < q_dot_tes_ch &&
            is_mode_avail(C_system_operating_modes::CR_ON__PC_OFF__TES_CH__AUX_OFF))
        {
            operating_mode = C_system_operating_modes::CR_ON__PC_OFF__TES_CH__AUX_OFF;
        }
        else if (is_mode_avail(C_system_operating_modes::CR_DF__PC_OFF__TES_FULL__AUX_OFF)) // m_is_CR_DF__PC_OFF__TES_FULL__AUX_OFF_avail)
        {
            operating_mode = C_system_operating_modes::CR_DF__PC_OFF__TES_FULL__AUX_OFF;
        }
        else
        {
            operating_mode = C_system_operating_modes::CR_OFF__PC_OFF__TES_OFF__AUX_OFF;
        }
    }

    return operating_mode;
}

C_csp_solver::C_system_operating_modes::E_operating_modes C_csp_solver::C_system_operating_modes::find_operating_mode
(C_csp_collector_receiver::E_csp_cr_modes cr_operating_state,
    C_csp_power_cycle::E_csp_power_cycle_modes pc_operating_state,
    double q_dot_cr_startup /*MWt*/, double q_dot_tes_dc /*MWt*/,
    double q_dot_cr_on /*MWt*/, double q_dot_tes_ch /*MWt*/,
    double q_dot_pc_su_max /*MWt*/, double q_dot_pc_target /*MWt*/,
    double q_dot_tes_dc_t_CR_su /*MWt*/, double q_dot_pc_min /*MWt*/,
    double q_dot_pc_sb /*MWt*/, double q_dot_pc_max /*MWt*/,
    double m_dot_cr_on /*kg/s*/, double m_dot_tes_ch_est /*kg/s*/,
    double m_dot_pc_max /*kg/s*/, double m_dot_tes_dc_t_CR_su /*kg/s*/,
    double m_dot_pc_min /*kg/s*/, double m_dot_tes_dc_est /*kg/s*/,
    double tol_mode_switching /*-*/,
    bool is_rec_su_allowed, bool is_pc_su_allowed,
    bool is_rec_outlet_to_hottank, bool is_pc_sb_allowed,
    double q_dot_PAR_HTR_on /*MWt*/, bool is_PAR_HTR_allowed)
{
    C_system_operating_modes::E_operating_modes operating_mode = C_system_operating_modes::CR_OFF__PC_OFF__TES_OFF__AUX_OFF;

    if ((cr_operating_state == C_csp_collector_receiver::OFF || cr_operating_state == C_csp_collector_receiver::STARTUP)
        && (pc_operating_state == C_csp_power_cycle::OFF || pc_operating_state == C_csp_power_cycle::STARTUP))
    {	// At start of this timestep, are power cycle AND collector/receiver off?

        if (q_dot_cr_startup > 0.0 && is_rec_su_allowed &&
            is_mode_avail(C_system_operating_modes::CR_SU__PC_OFF__TES_OFF__AUX_OFF))
        {	// Receiver startup is allowed and possible (will generate net energy)

            if (q_dot_tes_dc > 0.0 && is_pc_su_allowed &&
                is_mode_avail(C_system_operating_modes::CR_SU__PC_SU__TES_DC__AUX_OFF))
            {
                operating_mode = C_system_operating_modes::CR_SU__PC_SU__TES_DC__AUX_OFF;
            }
            else {
                operating_mode = pc_off__try_cr_su_with_htr_combs(q_dot_tes_ch, tol_mode_switching, is_PAR_HTR_allowed, q_dot_PAR_HTR_on);
            }
            //else if (is_PAR_HTR_allowed && q_dot_tes_ch > 0.0 && q_dot_PAR_HTR_on > 0.0 &&
            //    is_mode_avail(C_system_operating_modes::CR_SU__PC_OFF__TES_CH__HTR_ON))
            //{
            //    if (q_dot_PAR_HTR_on * (1. + tol_mode_switching) > q_dot_tes_ch &&
            //        is_mode_avail(C_system_operating_modes::CR_SU__PC_OFF__TES_FULL__HTR_DF)) {
            //
            //        operating_mode = C_system_operating_modes::CR_SU__PC_OFF__TES_FULL__HTR_DF;
            //    }
            //    else {
            //        operating_mode = C_system_operating_modes::CR_SU__PC_OFF__TES_CH__HTR_ON;
            //    }
            //}
            //else
            //{
            //    operating_mode = C_system_operating_modes::CR_SU__PC_OFF__TES_OFF__AUX_OFF;
            //}
        }
        else
        {
            if (q_dot_tes_dc > 0.0 && is_pc_su_allowed &&
                is_mode_avail(C_system_operating_modes::CR_OFF__PC_SU__TES_DC__AUX_OFF))
            {
                operating_mode = C_system_operating_modes::CR_OFF__PC_SU__TES_DC__AUX_OFF;
            }
            else {
                operating_mode = cr_and_pc_stay_off__try_htr(q_dot_tes_ch, tol_mode_switching, is_PAR_HTR_allowed, q_dot_PAR_HTR_on);
                //if (is_PAR_HTR_allowed && q_dot_PAR_HTR_on > 0.0 && q_dot_tes_ch > 0.0) {
                //    if (q_dot_PAR_HTR_on * (1. - tol_mode_switching) < q_dot_tes_ch &&
                //        is_mode_avail(CR_OFF__PC_OFF__TES_CH__HTR_ON)) {
                //
                //        operating_mode = C_system_operating_modes::CR_OFF__PC_OFF__TES_CH__HTR_ON;
                //    }
                //    else if (is_mode_avail(CR_OFF__PC_OFF__TES_FULL__HTR_DF)) {
                //        operating_mode = CR_OFF__PC_OFF__TES_FULL__HTR_DF;
                //    }
                //    else {
                //        operating_mode = C_system_operating_modes::CR_OFF__PC_OFF__TES_OFF__AUX_OFF;
                //    }
                //}
                //else {
                //    operating_mode = C_system_operating_modes::CR_OFF__PC_OFF__TES_OFF__AUX_OFF;
                //}
            }
        }
    }	// End logic for CR_state == OFF or STARTUP    AND     PC_state == OFF or STARTUP

    else if (cr_operating_state == C_csp_collector_receiver::ON &&
        (pc_operating_state == C_csp_power_cycle::OFF || pc_operating_state == C_csp_power_cycle::STARTUP))
    {
        if (q_dot_cr_on > 0.0 && is_rec_su_allowed && is_rec_outlet_to_hottank)
        {	// Receiver is allowed to remain on, and it can produce useful energy. Now, need to find a home for it

            if (is_pc_su_allowed &&
                is_mode_avail(C_system_operating_modes::CR_ON__PC_SU__TES_OFF__AUX_OFF)) // Can receiver output go to power cycle?
            {
                if (q_dot_tes_ch > 0.0)
                {
                    if (((q_dot_cr_on - q_dot_tes_ch) * (1.0 + tol_mode_switching) > q_dot_pc_su_max
                        || (m_dot_cr_on - m_dot_tes_ch_est) * (1.0 + tol_mode_switching) > m_dot_pc_max) &&
                        is_mode_avail(C_system_operating_modes::CR_DF__PC_SU__TES_FULL__AUX_OFF))
                    {
                        operating_mode = C_system_operating_modes::CR_DF__PC_SU__TES_FULL__AUX_OFF;
                    }
                    else if ((q_dot_cr_on * (1.0 + tol_mode_switching) > q_dot_pc_su_max
                        || m_dot_cr_on * (1.0 + tol_mode_switching) > m_dot_pc_max) &&
                        is_mode_avail(C_system_operating_modes::CR_ON__PC_SU__TES_CH__AUX_OFF))
                    {
                        operating_mode = C_system_operating_modes::CR_ON__PC_SU__TES_CH__AUX_OFF;
                    }
                    else
                    {
                        operating_mode = C_system_operating_modes::CR_ON__PC_SU__TES_OFF__AUX_OFF;
                    }
                }
                else
                {
                    if ((q_dot_cr_on * (1.0 + tol_mode_switching) > q_dot_pc_su_max ||
                        m_dot_cr_on * (1.0 + tol_mode_switching) > m_dot_pc_max) &&
                        is_mode_avail(C_system_operating_modes::CR_DF__PC_SU__TES_OFF__AUX_OFF))
                    {
                        operating_mode = C_system_operating_modes::CR_DF__PC_SU__TES_OFF__AUX_OFF;
                    }
                    else
                    {
                        operating_mode = C_system_operating_modes::CR_ON__PC_SU__TES_OFF__AUX_OFF;
                    }
                }
            }
            else if (q_dot_tes_ch > 0.0)
            {
                operating_mode = cr_on_pc_off_tes_ch_avail__try_htr(q_dot_cr_on, q_dot_tes_ch, tol_mode_switching,
                    is_PAR_HTR_allowed, q_dot_PAR_HTR_on);

                //if (is_PAR_HTR_allowed && q_dot_PAR_HTR_on > 0.0 &&
                //    is_mode_avail(C_system_operating_modes::CR_DF__PC_OFF__TES_FULL__AUX_OFF)) {
                //
                //    if ((q_dot_cr_on + q_dot_PAR_HTR_on) * (1.0 - tol_mode_switching) < q_dot_tes_ch &&
                //        is_mode_avail(C_system_operating_modes::CR_ON__PC_OFF__TES_CH__HTR_ON)) {
                //
                //        operating_mode = C_system_operating_modes::CR_ON__PC_OFF__TES_CH__HTR_ON;
                //    }
                //    else if (q_dot_cr_on * (1. - tol_mode_switching) < q_dot_tes_ch &&
                //        is_mode_avail(C_system_operating_modes::CR_ON__PC_OFF__TES_FULL__HTR_DF)) {
                //
                //        operating_mode = C_system_operating_modes::CR_ON__PC_OFF__TES_FULL__HTR_DF;
                //    }
                //    else {
                //        operating_mode = C_system_operating_modes::CR_DF__PC_OFF__TES_FULL__AUX_OFF;
                //    }
                //}
                //else {
                //    if (q_dot_cr_on * (1.0 - tol_mode_switching) < q_dot_tes_ch &&
                //        is_mode_avail(C_system_operating_modes::CR_ON__PC_OFF__TES_CH__AUX_OFF))
                //    {
                //        operating_mode = C_system_operating_modes::CR_ON__PC_OFF__TES_CH__AUX_OFF;
                //    }
                //    else if (is_mode_avail(C_system_operating_modes::CR_DF__PC_OFF__TES_FULL__AUX_OFF)) // m_is_CR_DF__PC_OFF__TES_FULL__AUX_OFF_avail)
                //    {
                //        operating_mode = C_system_operating_modes::CR_DF__PC_OFF__TES_FULL__AUX_OFF;
                //    }
                //    else
                //    {
                //        operating_mode = C_system_operating_modes::CR_OFF__PC_OFF__TES_OFF__AUX_OFF;
                //    }
                //}
            }
            else
            {
                operating_mode = C_system_operating_modes::CR_OFF__PC_OFF__TES_OFF__AUX_OFF;
            }
        }
        else
        {
            if ((q_dot_cr_on > 0.0 || m_dot_cr_on > 0.0) && is_rec_su_allowed)
            {
                if (q_dot_tes_dc > 0.0 && is_pc_su_allowed &&
                    is_mode_avail(C_system_operating_modes::CR_TO_COLD__PC_SU__TES_DC__AUX_OFF))
                {	// Can power cycle startup using TES?

                    operating_mode = C_system_operating_modes::CR_TO_COLD__PC_SU__TES_DC__AUX_OFF;
                }
                else if (is_mode_avail(C_system_operating_modes::CR_TO_COLD__PC_OFF__TES_OFF__AUX_OFF)) // m_is_CR_TO_COLD__PC_OFF__TES_OFF__AUX_OFF_avail)
                {
                    operating_mode = C_system_operating_modes::CR_TO_COLD__PC_OFF__TES_OFF__AUX_OFF;
                }
                else
                {
                    operating_mode = cr_and_pc_stay_off__try_htr(q_dot_tes_ch, tol_mode_switching, is_PAR_HTR_allowed, q_dot_PAR_HTR_on);
                }
            }
            else
            {
                if (q_dot_tes_dc > 0.0 && is_pc_su_allowed &&
                    is_mode_avail(C_system_operating_modes::CR_OFF__PC_SU__TES_DC__AUX_OFF))
                {	// Can power cycle startup using TES?

                    operating_mode = C_system_operating_modes::CR_OFF__PC_SU__TES_DC__AUX_OFF;
                }
                else
                {
                    operating_mode = cr_and_pc_stay_off__try_htr(q_dot_tes_ch, tol_mode_switching, is_PAR_HTR_allowed, q_dot_PAR_HTR_on);
                }
            }
        }
    }

    else if ((cr_operating_state == C_csp_collector_receiver::OFF || cr_operating_state == C_csp_collector_receiver::STARTUP) &&
    (pc_operating_state == C_csp_power_cycle::ON || pc_operating_state == C_csp_power_cycle::STANDBY))
    {
        if (q_dot_cr_startup > 0.0 && is_rec_su_allowed &&
            is_mode_avail(C_system_operating_modes::CR_SU__PC_OFF__TES_OFF__AUX_OFF))
        {	// Receiver startup is allowed and possible (will generate net energy) - determine if power cycle can remain on

            if ((is_pc_su_allowed || is_pc_sb_allowed))
            {
                if (((q_dot_tes_dc_t_CR_su * (1.0 + tol_mode_switching) > q_dot_pc_target
                    && m_dot_tes_dc_t_CR_su * (1.0 + tol_mode_switching) > m_dot_pc_min)
                    || m_dot_tes_dc_t_CR_su * (1.0 + tol_mode_switching) > m_dot_pc_max)
                    && is_pc_su_allowed &&
                    is_mode_avail(C_system_operating_modes::CR_SU__PC_TARGET__TES_DC__AUX_OFF))
                {	// Tolerance is applied so that if TES is *close* to matching target, the controller tries that mode

                    operating_mode = C_system_operating_modes::CR_SU__PC_TARGET__TES_DC__AUX_OFF;
                }
                else if (q_dot_tes_dc_t_CR_su * (1.0 + tol_mode_switching) > q_dot_pc_min
                    && m_dot_tes_dc_t_CR_su * (1.0 + tol_mode_switching) > m_dot_pc_min
                    && is_pc_su_allowed &&
                    is_mode_avail(C_system_operating_modes::CR_SU__PC_RM_LO__TES_EMPTY__AUX_OFF))
                {	// Tolerance is applied so that if TES is *close* to reaching min fraction, the controller tries that mode

                    operating_mode = C_system_operating_modes::CR_SU__PC_RM_LO__TES_EMPTY__AUX_OFF;
                }
                else if (q_dot_tes_dc_t_CR_su * (1.0 + tol_mode_switching) > q_dot_pc_sb
                    && m_dot_tes_dc_t_CR_su * (1.0 + tol_mode_switching) > m_dot_pc_min
                    && is_pc_sb_allowed &&
                    is_mode_avail(C_system_operating_modes::CR_SU__PC_SB__TES_DC__AUX_OFF))
                {	// Tolerance is applied so that if TES is *close* to reaching min fraction, the controller tries that mode

                    operating_mode = C_system_operating_modes::CR_SU__PC_SB__TES_DC__AUX_OFF;
                }
                else if (q_dot_tes_dc_t_CR_su > 0.0 && is_pc_su_allowed &&
                    is_mode_avail(C_system_operating_modes::CR_SU__PC_MIN__TES_EMPTY__AUX_OFF))
                {
                    operating_mode = C_system_operating_modes::CR_SU__PC_MIN__TES_EMPTY__AUX_OFF;
                }
                else {
                    operating_mode = pc_off__try_cr_su_with_htr_combs(q_dot_tes_ch, tol_mode_switching, is_PAR_HTR_allowed, q_dot_PAR_HTR_on);
                }
                //else if (is_mode_avail(C_system_operating_modes::CR_SU__PC_OFF__TES_OFF__AUX_OFF)) // m_is_CR_SU__PC_OFF__TES_OFF__AUX_OFF_avail )
                //{
                //    operating_mode = C_system_operating_modes::CR_SU__PC_OFF__TES_OFF__AUX_OFF;
                //}
                // If no solutions in this branch, return to 'is_rec_su_allowed' branch and try NO path
            }	// End 'is_pc_su_allowed' logic
            else
            {	// power cycle startup/operation not allowed
                operating_mode = pc_off__try_cr_su_with_htr_combs(q_dot_tes_ch, tol_mode_switching, is_PAR_HTR_allowed, q_dot_PAR_HTR_on);
                //if (is_mode_avail(C_system_operating_modes::CR_SU__PC_OFF__TES_OFF__AUX_OFF)) // m_is_CR_SU__PC_OFF__TES_OFF__AUX_OFF_avail )
                //{
                //    operating_mode = C_system_operating_modes::CR_SU__PC_OFF__TES_OFF__AUX_OFF;
                //}
                // If no solutions in this branch, return to 'is_rec_su_allowed' branch and try NO path
            }
        }
        else	// Receiver remains OFF - determine if power cycle can remain on
        {
            if (is_pc_su_allowed || is_pc_sb_allowed)
            {

                if (((q_dot_tes_dc * (1.0 + tol_mode_switching) > q_dot_pc_target
                    && m_dot_tes_dc_est * (1.0 + tol_mode_switching) > m_dot_pc_min)
                    || m_dot_tes_dc_est * (1.0 + tol_mode_switching) > m_dot_pc_max)
                    && is_pc_su_allowed &&
                    is_mode_avail(C_system_operating_modes::CR_OFF__PC_TARGET__TES_DC__AUX_OFF))
                {	// Tolerance is applied so that if TES is *close* to matching target, the controller tries that mode

                    operating_mode = C_system_operating_modes::CR_OFF__PC_TARGET__TES_DC__AUX_OFF;
                }
                else if (q_dot_tes_dc * (1.0 + tol_mode_switching) > q_dot_pc_min
                    && m_dot_tes_dc_est * (1.0 + tol_mode_switching) > m_dot_pc_min
                    && is_pc_su_allowed &&
                    is_mode_avail(C_system_operating_modes::CR_OFF__PC_RM_LO__TES_EMPTY__AUX_OFF))
                {	// Tolerance is applied so that if TES is *close* to reaching min fraction, the controller tries that mode

                    operating_mode = C_system_operating_modes::CR_OFF__PC_RM_LO__TES_EMPTY__AUX_OFF;
                }
                else if (q_dot_tes_dc * (1.0 + tol_mode_switching) > q_dot_pc_sb
                    && m_dot_tes_dc_est * (1.0 + tol_mode_switching) > m_dot_pc_min
                    && is_pc_sb_allowed &&
                    is_mode_avail(C_system_operating_modes::CR_OFF__PC_SB__TES_DC__AUX_OFF))
                {	// Tolerance is applied so that if TES is *close* to reaching min fraction, the controller tries that mode

                    operating_mode = C_system_operating_modes::CR_OFF__PC_SB__TES_DC__AUX_OFF;
                }
                else if (q_dot_tes_dc > 0.0 && is_pc_su_allowed &&
                    is_mode_avail(C_system_operating_modes::CR_OFF__PC_MIN__TES_EMPTY__AUX_OFF))
                {
                    operating_mode = C_system_operating_modes::CR_OFF__PC_MIN__TES_EMPTY__AUX_OFF;
                }
                else
                {
                    operating_mode = cr_and_pc_stay_off__try_htr(q_dot_tes_ch, tol_mode_switching, is_PAR_HTR_allowed, q_dot_PAR_HTR_on);
                }
            }	// end logic on 'is_pc_su_allowed'
            else
            {

                operating_mode = cr_and_pc_stay_off__try_htr(q_dot_tes_ch, tol_mode_switching, is_PAR_HTR_allowed, q_dot_PAR_HTR_on);
            }
        }
    }

    else if (cr_operating_state == C_csp_collector_receiver::ON &&
        (pc_operating_state == C_csp_power_cycle::ON || pc_operating_state == C_csp_power_cycle::STANDBY))
    {
        if (q_dot_cr_on > 0.0 && is_rec_su_allowed && is_rec_outlet_to_hottank)
        {	// Receiver operation is allowed and possible - find a home for output

            if (is_pc_su_allowed || is_pc_sb_allowed)
            {
                if ((q_dot_cr_on * (1.0 + tol_mode_switching) > q_dot_pc_target || m_dot_cr_on * (1.0 + tol_mode_switching) > m_dot_pc_max) &&
                    is_pc_su_allowed &&
                    is_LO_SIDE_mode_avail(C_system_operating_modes::CR_ON__PC_RM_HI__TES_OFF__AUX_OFF) &&
                    is_LO_SIDE_mode_avail(C_system_operating_modes::CR_ON__PC_TARGET__TES_CH__AUX_OFF))
                {	// The power cycle cannot accept the entire receiver output
                    // Tolerance is applied so that if CR is *close* to reaching the PC target, the controller tries modes that fill TES

                    // Can storage be charged?
                    if (q_dot_tes_ch > 0.0)
                    {
                        // 1) Try to fill storage while hitting power cycle target
                        if ((q_dot_cr_on - q_dot_tes_ch) * (1.0 - tol_mode_switching) < q_dot_pc_target
                            && (m_dot_cr_on - m_dot_tes_ch_est) * (1.0 - tol_mode_switching) < m_dot_pc_max &&
                            is_HI_SIDE_mode_avail(C_system_operating_modes::CR_ON__PC_TARGET__TES_CH__AUX_OFF))
                        {	// Storage can accept the remaining receiver output
                            // Tolerance is applied so that if CR + TES is *close* to reaching PC target, the controller tries that mode

                            operating_mode = C_system_operating_modes::CR_ON__PC_TARGET__TES_CH__AUX_OFF;
                        }

                        // 2) Try operating power cycle at maximum capacity
                        // Assume we want to completely fill storage, so the power cycle operation should float to meet that condition
                        else if ((q_dot_cr_on - q_dot_tes_ch) * (1.0 - tol_mode_switching) < q_dot_pc_max
                            && (m_dot_cr_on - m_dot_tes_ch_est) * (1.0 - tol_mode_switching) < m_dot_pc_max &&
                            is_mode_avail(C_system_operating_modes::CR_ON__PC_RM_HI__TES_FULL__AUX_OFF))
                        {	// Storage and the power cycle operating between target and max can accept the remaining receiver output
                            // Tolerance is applied so that if CR + TES is *close* to reaching PC  max, the controller tries that mode

                            operating_mode = C_system_operating_modes::CR_ON__PC_RM_HI__TES_FULL__AUX_OFF;
                        }

                        // 3) Try defocusing the CR and operating the power cycle at maximum capacity
                        else if (is_mode_avail(C_system_operating_modes::CR_DF__PC_MAX__TES_FULL__AUX_OFF))  // m_is_CR_DF__PC_MAX__TES_FULL__AUX_OFF_avail )
                        {

                            operating_mode = C_system_operating_modes::CR_DF__PC_MAX__TES_FULL__AUX_OFF;
                        }
                        else
                        {
                            operating_mode = C_system_operating_modes::CR_OFF__PC_OFF__TES_OFF__AUX_OFF;;
                        }
                    }	// End if(q_dot_tes_ch > 0.0) logic

                    else
                    {	// No storage available for dispatch

                        // 1) Try operating power cycle at maximum capacity
                        if ((q_dot_cr_on * (1.0 - tol_mode_switching) < q_dot_pc_max && m_dot_cr_on * (1.0 - tol_mode_switching)) &&
                            is_HI_SIDE_mode_avail(C_system_operating_modes::CR_ON__PC_RM_HI__TES_OFF__AUX_OFF))
                        {	// Tolerance is applied so that if CR + TES is *close* to reaching PC  max, the controller tries that mode

                            operating_mode = C_system_operating_modes::CR_ON__PC_RM_HI__TES_OFF__AUX_OFF;
                        }
                        else if (is_mode_avail(C_system_operating_modes::CR_DF__PC_MAX__TES_OFF__AUX_OFF))   // m_is_CR_DF__PC_MAX__TES_OFF__AUX_OFF_avail )
                        {
                            operating_mode = C_system_operating_modes::CR_DF__PC_MAX__TES_OFF__AUX_OFF;
                        }
                        else
                        {
                            operating_mode = C_system_operating_modes::CR_OFF__PC_OFF__TES_OFF__AUX_OFF;
                        }
                    }	// End else 'no storage available for dispatch'
                }
                else
                {	// Power cycle is asking for more output than the receiver can supply

                    if (q_dot_tes_dc > 0.0)
                    {	// Storage dispatch is available

                        if ((((q_dot_cr_on + q_dot_tes_dc) * (1.0 + tol_mode_switching) > q_dot_pc_target
                            && (m_dot_cr_on + m_dot_tes_dc_est) * (1.0 + tol_mode_switching) > m_dot_pc_min)
                            || (m_dot_cr_on + m_dot_tes_dc_est) * (1.0 + tol_mode_switching) > m_dot_pc_max)
                            && is_pc_su_allowed &&
                            is_mode_avail(C_system_operating_modes::CR_ON__PC_TARGET__TES_DC__AUX_OFF))
                        {	// Storage can provide enough dispatch to reach power cycle target
                            // Tolerance is applied so that if CR + TES is *close* to reaching PC target, the controller tries that mode

                            operating_mode = C_system_operating_modes::CR_ON__PC_TARGET__TES_DC__AUX_OFF;
                        }
                        else if ((q_dot_cr_on + q_dot_tes_dc) * (1.0 + tol_mode_switching) > q_dot_pc_min
                            && is_pc_su_allowed
                            && (m_dot_cr_on + m_dot_tes_dc_est) * (1.0 + tol_mode_switching) > m_dot_pc_min &&
                            is_mode_avail(C_system_operating_modes::CR_ON__PC_RM_LO__TES_EMPTY__AUX_OFF))
                        {	// Storage can provide enough dispatch to at least meet power cycle minimum operation fraction
                            // Run at highest possible PC fraction by dispatch all remaining storage
                            // Tolerance is applied so that if CR + TES is *close* to reaching PC min, the controller tries that mode

                            operating_mode = C_system_operating_modes::CR_ON__PC_RM_LO__TES_EMPTY__AUX_OFF;
                        }
                        else if (q_dot_cr_on * (1.0 + tol_mode_switching) > q_dot_pc_sb
                            && m_dot_cr_on * (1.0 + tol_mode_switching) > m_dot_pc_min
                            && is_pc_sb_allowed &&
                            is_mode_avail(C_system_operating_modes::CR_ON__PC_SB__TES_OFF__AUX_OFF) &&
                            is_mode_avail(C_system_operating_modes::CR_ON__PC_SB__TES_CH__AUX_OFF))
                        {
                            if (q_dot_tes_ch > 0.0)
                            {
                                if (((q_dot_cr_on - q_dot_tes_ch) * (1.0 + tol_mode_switching) > q_dot_pc_sb
                                    || (m_dot_cr_on - m_dot_tes_ch_est) * (1.0 + tol_mode_switching) > m_dot_pc_min) &&
                                    is_mode_avail(C_system_operating_modes::CR_ON__PC_SB__TES_FULL__AUX_OFF))
                                {	// Tolerance is applied so that if CR output is *close* to operating at standby AND completely filling storage, controller tries that mode

                                    operating_mode = C_system_operating_modes::CR_ON__PC_SB__TES_FULL__AUX_OFF;
                                }
                                else
                                {
                                    operating_mode = C_system_operating_modes::CR_ON__PC_SB__TES_CH__AUX_OFF;
                                }
                            }
                            else
                            {
                                // This could *technically* use defocus, but can argue the energy is just being thrown away in power cycle anyway
                                operating_mode = C_system_operating_modes::CR_ON__PC_SB__TES_OFF__AUX_OFF;
                            }
                        }
                        else if ((q_dot_cr_on + q_dot_tes_dc) * (1.0 + tol_mode_switching) > q_dot_pc_sb
                            && (m_dot_cr_on + m_dot_tes_dc_est) * (1.0 + tol_mode_switching) > m_dot_pc_min
                            && is_pc_sb_allowed &&
                            is_mode_avail(C_system_operating_modes::CR_ON__PC_SB__TES_DC__AUX_OFF))
                        {
                            operating_mode = C_system_operating_modes::CR_ON__PC_SB__TES_DC__AUX_OFF;
                        }
                        else if (is_pc_su_allowed && is_mode_avail(C_system_operating_modes::CR_ON__PC_MIN__TES_EMPTY__AUX_OFF))
                        {
                            operating_mode = C_system_operating_modes::CR_ON__PC_MIN__TES_EMPTY__AUX_OFF;
                        }
                        else if (q_dot_tes_ch > 0.0)
                        {
                            if (q_dot_cr_on * (1.0 - tol_mode_switching) < q_dot_tes_ch &&
                                is_mode_avail(C_system_operating_modes::CR_ON__PC_OFF__TES_CH__AUX_OFF))
                            {	// Tolerance is applied so that if CR is *close* to being less than a full TES charge, the controller tries normal operation (no defocus)

                                operating_mode = C_system_operating_modes::CR_ON__PC_OFF__TES_CH__AUX_OFF;
                            }
                            else if (is_mode_avail(C_system_operating_modes::CR_DF__PC_OFF__TES_FULL__AUX_OFF))
                            {	// The CR output will overcharge storage, so it needs to defocus.
                                // However, because the CR output is already part-load, it may be close to shutting down before defocus...

                                operating_mode = C_system_operating_modes::CR_DF__PC_OFF__TES_FULL__AUX_OFF;
                            }
                            else
                            {
                                operating_mode = C_system_operating_modes::CR_OFF__PC_OFF__TES_OFF__AUX_OFF;
                            }
                        }
                        else
                        {	// No home for receiver output, and not enough thermal power for power cycle

                            operating_mode = C_system_operating_modes::CR_OFF__PC_OFF__TES_OFF__AUX_OFF;
                        }
                    }
                    else
                    {	// Storage dispatch is not available

                        // Can the power cycle operate at or above the minimum operation fraction?
                        if (((q_dot_cr_on * (1.0 + tol_mode_switching) > q_dot_pc_min
                            && m_dot_cr_on * (1.0 + tol_mode_switching) > m_dot_pc_min)
                            || m_dot_cr_on * (1.0 + tol_mode_switching) > m_dot_pc_max)
                            && is_pc_su_allowed &&
                            is_mode_avail(C_system_operating_modes::CR_ON__PC_RM_LO__TES_OFF__AUX_OFF))
                        {	// Tolerance is applied so that if CR is *close* to reaching PC min, the controller tries that mode

                            operating_mode = C_system_operating_modes::CR_ON__PC_RM_LO__TES_OFF__AUX_OFF;
                        }
                        else if (is_pc_sb_allowed
                            && q_dot_cr_on * (1.0 + tol_mode_switching) > q_dot_pc_sb
                            && m_dot_cr_on * (1.0 + tol_mode_switching) > m_dot_pc_min &&
                            is_mode_avail(C_system_operating_modes::CR_ON__PC_SB__TES_OFF__AUX_OFF) &&
                            is_mode_avail(C_system_operating_modes::CR_ON__PC_SB__TES_CH__AUX_OFF))
                        {	// Receiver can likely operate in standby
                            // Tolerance is applied so that if CR is *close* to reaching PC standby, the controller tries that mode

                            if (q_dot_tes_ch > 0.0)
                            {
                                if (((q_dot_cr_on - q_dot_tes_ch) * (1.0 + tol_mode_switching) > q_dot_pc_sb
                                    || (m_dot_cr_on - m_dot_tes_ch_est) * (1.0 + tol_mode_switching) > m_dot_pc_min) &&
                                    is_mode_avail(C_system_operating_modes::CR_ON__PC_SB__TES_FULL__AUX_OFF))
                                {	// Tolerance is applied so that if CR output is *close* to operating at standby AND completely filling storage, controller tries that mode

                                    operating_mode = C_system_operating_modes::CR_ON__PC_SB__TES_FULL__AUX_OFF;
                                }
                                else
                                {
                                    operating_mode = C_system_operating_modes::CR_ON__PC_SB__TES_CH__AUX_OFF;
                                }
                            }
                            else
                            {
                                // This could *technically* use defocus, but can argue the energy is just being thrown away in power cycle anyway
                                operating_mode = C_system_operating_modes::CR_ON__PC_SB__TES_OFF__AUX_OFF;
                            }
                        }
                        else if (q_dot_tes_ch > 0.0)
                        {	// Charge storage with receiver output

                            if (q_dot_cr_on * (1.0 - tol_mode_switching) < q_dot_tes_ch &&
                                is_mode_avail(C_system_operating_modes::CR_ON__PC_OFF__TES_CH__AUX_OFF))
                            {	// Tolerance is applied so that if CR is *close* to being less than a full TES charge, the controller tries normal operation (no defocus)

                                operating_mode = C_system_operating_modes::CR_ON__PC_OFF__TES_CH__AUX_OFF;
                            }
                            else if (is_mode_avail(C_system_operating_modes::CR_DF__PC_OFF__TES_FULL__AUX_OFF))
                            {	// The CR output will overcharge storage, so it needs to defocus.
                                // However, because the CR output is already part-load, it may be close to shutting down before defocus...

                                operating_mode = C_system_operating_modes::CR_DF__PC_OFF__TES_FULL__AUX_OFF;
                            }
                            else
                            {
                                operating_mode = C_system_operating_modes::CR_OFF__PC_OFF__TES_OFF__AUX_OFF;
                            }
                        }
                        else
                        {	// No home for receiver output, and not enough thermal power for power cycle

                            operating_mode = C_system_operating_modes::CR_OFF__PC_OFF__TES_OFF__AUX_OFF;
                        }
                    }	// End logic else 'storage dispatch not available'
                }	// End logic else 'power cycle requires more q_dot than receiver can supply'				
            }	// End logic if(is_rec_su_allowed)
            else
            {	// Power cycle startup is not allowed - see if receiver output can go to storage

                if (q_dot_tes_ch > 0.0)
                {
                    operating_mode = cr_on_pc_off_tes_ch_avail__try_htr(q_dot_cr_on, q_dot_tes_ch, tol_mode_switching,
                        is_PAR_HTR_allowed, q_dot_PAR_HTR_on);

                    //if (q_dot_cr_on * (1.0 - tol_mode_switching) < q_dot_tes_ch &&          
                    //    is_mode_avail(C_system_operating_modes::CR_ON__PC_OFF__TES_CH__AUX_OFF))
                    //{
                    //    operating_mode = C_system_operating_modes::CR_ON__PC_OFF__TES_CH__AUX_OFF;
                    //}
                    //else if (is_mode_avail(C_system_operating_modes::CR_DF__PC_OFF__TES_FULL__AUX_OFF))
                    //{
                    //    operating_mode = C_system_operating_modes::CR_DF__PC_OFF__TES_FULL__AUX_OFF;
                    //}
                    //else
                    //{
                    //    operating_mode = C_system_operating_modes::CR_OFF__PC_OFF__TES_OFF__AUX_OFF;
                    //}
                }
                else
                {
                    operating_mode = C_system_operating_modes::CR_OFF__PC_OFF__TES_OFF__AUX_OFF;
                }

            }	// End logic else 'pc su is NOT allowed'		
        }	// End logic if(q_dot_cr_output > 0.0 && is_rec_su_allowed)

        // 'else if' loop for receiver 'on' but sending htf to cold tank
        else if ((q_dot_cr_on > 0.0 || m_dot_cr_on > 0.0) && is_rec_su_allowed) {

            if (is_pc_su_allowed || is_pc_sb_allowed)
            {
                if (q_dot_tes_dc > 0.0)
                {	// Storage dispatch is available

                    if (((q_dot_tes_dc * (1.0 + tol_mode_switching) > q_dot_pc_target
                        && m_dot_tes_dc_est * (1.0 + tol_mode_switching) > m_dot_pc_min)
                        || m_dot_tes_dc_est * (1.0 + tol_mode_switching) > m_dot_pc_max)
                        && is_pc_su_allowed &&
                        is_mode_avail(C_system_operating_modes::CR_TO_COLD__PC_TARGET__TES_DC__AUX_OFF))
                    {	// Storage can provide enough dispatch to reach power cycle target
                        // Tolerance is applied so that if TES is *close* to reaching PC target, the controller tries that mode

                        operating_mode = C_system_operating_modes::CR_TO_COLD__PC_TARGET__TES_DC__AUX_OFF;
                    }
                    else if (q_dot_tes_dc * (1.0 + tol_mode_switching) > q_dot_pc_min
                        && m_dot_tes_dc_est * (1.0 + tol_mode_switching) > m_dot_pc_min
                        && is_pc_su_allowed &&
                        is_mode_avail(C_system_operating_modes::CR_TO_COLD__PC_RM_LO__TES_EMPTY__AUX_OFF))
                    {	// Storage can provide enough dispatch to at least meet power cycle minimum operation fraction
                        // Run at highest possible PC fraction by dispatching all remaining storage
                        // Tolerance is applied so that if CR + TES is *close* to reaching PC min, the controller tries that mode

                        operating_mode = C_system_operating_modes::CR_TO_COLD__PC_RM_LO__TES_EMPTY__AUX_OFF;
                    }
                    else if (q_dot_tes_dc * (1.0 + tol_mode_switching) > q_dot_pc_sb
                        && m_dot_tes_dc_est * (1.0 + tol_mode_switching) > m_dot_pc_min
                        && is_pc_sb_allowed &&
                        is_mode_avail(C_system_operating_modes::CR_TO_COLD__PC_SB__TES_DC__AUX_OFF))
                    {	// Tolerance is applied so that if CR + TES is *close* to reaching standby, the controller tries that mode

                        operating_mode = C_system_operating_modes::CR_TO_COLD__PC_SB__TES_DC__AUX_OFF;
                    }
                    else if (is_pc_su_allowed && is_mode_avail(C_system_operating_modes::CR_TO_COLD__PC_MIN__TES_EMPTY__AUX_OFF))
                    {	// If not enough thermal power to stay in standby, then run at min PC load until TES is fully discharged

                        operating_mode = C_system_operating_modes::CR_TO_COLD__PC_MIN__TES_EMPTY__AUX_OFF;
                    }
                    else if (is_mode_avail(C_system_operating_modes::CR_TO_COLD__PC_OFF__TES_OFF__AUX_OFF))
                    {
                        operating_mode = C_system_operating_modes::CR_TO_COLD__PC_OFF__TES_OFF__AUX_OFF;
                    }
                    else
                    {
                        operating_mode = C_system_operating_modes::CR_OFF__PC_OFF__TES_OFF__AUX_OFF;
                    }
                }	// End logic for if( q_dot_tes_dc > 0.0 )
                else if (is_mode_avail(C_system_operating_modes::CR_TO_COLD__PC_OFF__TES_OFF__AUX_OFF))
                {	// Storage dispatch is not available

                    // No thermal power available to power cycle
                    operating_mode = C_system_operating_modes::CR_TO_COLD__PC_OFF__TES_OFF__AUX_OFF;
                }
                else
                {
                    operating_mode = C_system_operating_modes::CR_OFF__PC_OFF__TES_OFF__AUX_OFF;
                }
            }	// End logic if( is_pc_su_allowed )
            else if (is_mode_avail(C_system_operating_modes::CR_TO_COLD__PC_OFF__TES_OFF__AUX_OFF))
            {	// If neither receiver nor power cycle operation is allowed, then shut everything off

                operating_mode = C_system_operating_modes::CR_TO_COLD__PC_OFF__TES_OFF__AUX_OFF;
            }
            else
            {
                operating_mode = C_system_operating_modes::CR_OFF__PC_OFF__TES_OFF__AUX_OFF;
            }

        }
        else	// Receiver is off - determine if power cycle can remain on
        {
            if (is_pc_su_allowed || is_pc_sb_allowed)
            {
                if (q_dot_tes_dc > 0.0)
                {	// Storage dispatch is available

                    if (((q_dot_tes_dc * (1.0 + tol_mode_switching) > q_dot_pc_target
                        && m_dot_tes_dc_est * (1.0 + tol_mode_switching) > m_dot_pc_min)
                        || m_dot_tes_dc_est * (1.0 + tol_mode_switching) > m_dot_pc_max)
                        && is_pc_su_allowed &&
                        is_mode_avail(C_system_operating_modes::CR_OFF__PC_TARGET__TES_DC__AUX_OFF))
                    {	// Storage can provide enough dispatch to reach power cycle target
                        // Tolerance is applied so that if TES is *close* to reaching PC target, the controller tries that mode

                        operating_mode = C_system_operating_modes::CR_OFF__PC_TARGET__TES_DC__AUX_OFF;
                    }
                    else if (q_dot_tes_dc * (1.0 + tol_mode_switching) > q_dot_pc_min
                        && m_dot_tes_dc_est * (1.0 + tol_mode_switching) > m_dot_pc_min
                        && is_pc_su_allowed &&
                        is_mode_avail(C_system_operating_modes::CR_OFF__PC_RM_LO__TES_EMPTY__AUX_OFF))
                    {	// Storage can provide enough dispatch to at least meet power cycle minimum operation fraction
                        // Run at highest possible PC fraction by dispatching all remaining storage
                        // Tolerance is applied so that if CR + TES is *close* to reaching PC min, the controller tries that mode

                        operating_mode = C_system_operating_modes::CR_OFF__PC_RM_LO__TES_EMPTY__AUX_OFF;
                    }
                    else if (q_dot_tes_dc * (1.0 + tol_mode_switching) > q_dot_pc_sb
                        && m_dot_tes_dc_est * (1.0 + tol_mode_switching) > m_dot_pc_min
                        && is_pc_sb_allowed &&
                        is_mode_avail(C_system_operating_modes::CR_OFF__PC_SB__TES_DC__AUX_OFF))
                    {	// Tolerance is applied so that if CR + TES is *close* to reaching standby, the controller tries that mode

                        operating_mode = C_system_operating_modes::CR_OFF__PC_SB__TES_DC__AUX_OFF;
                    }
                    else if (is_pc_su_allowed && is_mode_avail(C_system_operating_modes::CR_OFF__PC_MIN__TES_EMPTY__AUX_OFF))
                    {	// If not enough thermal power to stay in standby, then run at min PC load until TES is fully discharged

                        operating_mode = C_system_operating_modes::CR_OFF__PC_MIN__TES_EMPTY__AUX_OFF;
                    }
                    else
                    {
                        operating_mode = cr_and_pc_stay_off__try_htr(q_dot_tes_ch, tol_mode_switching, is_PAR_HTR_allowed, q_dot_PAR_HTR_on);
                    }
                }	// End logic for if( q_dot_tes_dc > 0.0 )
                else
                {	// Storage dispatch is not available

                    // No thermal power available to power cycle
                    operating_mode = cr_and_pc_stay_off__try_htr(q_dot_tes_ch, tol_mode_switching, is_PAR_HTR_allowed, q_dot_PAR_HTR_on);
                }
            }	// End logic if( is_pc_su_allowed )
            else
            {	// If neither receiver nor power cycle operation is allowed, then shut everything off

                operating_mode = cr_and_pc_stay_off__try_htr(q_dot_tes_ch, tol_mode_switching, is_PAR_HTR_allowed, q_dot_PAR_HTR_on);
            }
        }
    }

    return operating_mode;
}
