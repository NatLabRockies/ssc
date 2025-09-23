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

#ifndef __csp_solver_pc_Rankine_indirect_224_
#define __csp_solver_pc_Rankine_indirect_224_

#include "csp_solver_util.h"
#include "csp_solver_core.h"

#include "lib_util.h"
#include "htf_props.h"

#include "ud_power_cycle.h"
#include "csp_solver_two_tank_tes.h"
#include "csp_radiator.h"
#include "csp_solver_stratified_tes.h"

#include <set>
#include <algorithm>

class C_pc_Rankine_indirect_224 : public C_csp_power_cycle
{
private:
	bool m_is_initialized;

	double m_F_wcMax;
	double m_F_wcMin;
	double m_delta_h_steam;
	double m_startup_energy_required;
    double m_Psat_ref;      //[Pa]
    double m_P_ND_ref;      //[-]
    double m_Q_ND_ref;      //[-]

    // Design-point conditions
    double m_rh_des;        //[%]
    double m_P_amb_des;     //[Pa]
    double m_T_wb_des;      //[C]

    // Cooler design - hardcoded
    double m_evap_dt_out;	//[C/K] Temperature difference at hot side of the condenser


	double m_m_dot_design;				//[kg/hr]
	double m_m_dot_max;					//[kg/hr]
	double m_m_dot_min;					//[kg/hr]
	double m_q_dot_design;				//[MWt]
    double m_q_dot_reject_des;          //[MWt]
	double m_cp_htf_design;				//[kJ/kg-K]
    double m_W_dot_htf_pump_des;          //[MWe]
    double m_W_dot_cooling_des;         //[MWe]

    double m_T_boil_des;                //[K]
    double m_delatT_hot_to_boil_des;    //[C/K]

	C_csp_power_cycle::E_csp_power_cycle_modes m_operating_mode_prev;
	double m_startup_time_remain_prev;		//[hr]
	double m_startup_energy_remain_prev;	//[kW-hr]

    C_csp_power_cycle::E_csp_power_cycle_modes m_operating_mode_calc;
	double m_startup_time_remain_calc;
	double m_startup_energy_remain_calc;

	util::matrix_t<double> m_db;

    // UDPC calculated design metrics
    int m_n_T_htf_pars, m_n_T_amb_pars, m_n_m_dot_pars;
    double m_T_htf_ref_udpc_calc, m_T_htf_low_udpc_calc, m_T_htf_high_udpc_calc;
    double m_T_amb_ref_udpc_calc, m_T_amb_low_udpc_calc, m_T_amb_high_udpc_calc;
    double m_m_dot_htf_ref_udpc_calc, m_m_dot_htf_low_udpc_calc, m_m_dot_htf_high_udpc_calc;
    double m_W_dot_gross_ND_des;
    double m_Q_dot_HTF_ND_des;
    double m_W_dot_cooling_ND_des;
    double m_m_dot_water_ND_des;

	HTFProperties mc_pc_htfProps;

    std::shared_ptr<C_air_cooled_condenser> m_ACC;
    std::shared_ptr<C_evap_tower> m_evap_tower;
    std::shared_ptr<C_hybrid_cooling> m_hybrid_cooling;

	// member string for exception messages
	std::string m_error_msg;

	// member class for User Defined Power Cycle
	C_ud_power_cycle mc_user_defined_pc;

	// track number of calls per timestep, reset = -1 in converged() call
	int m_ncall;

	double GetFieldToTurbineTemperatureDropC() { return 25.0; }

    void RankineCycle_V2(double T_db /*K*/, double T_wb /*K*/,
        double P_amb /*Pa*/, double T_htf_hot /*C*/, double m_dot_htf /*kg/hr*/,
        double F_wc /*-*/, double F_wcmin /*-*/, double F_wcmax /*-*/, double T_cold /*C*/, double dT_cw /*C*/,
        //outputs
        double& P_cycle /*kWe*/, double& eta, double& T_htf_cold, double& m_dot_demand, double& m_dot_htf_ref,
        double& m_dot_makeup, double& W_cool_par /*MWe*/, double& f_hrsys, double& P_cond /*Pa*/, double& T_cond_out /*C*/,
        double& P_cond_iter_rel_err /*-*/);

    void cycle_Rankine_ND(double T_htf_hot_ND /*-*/, double P_cond_iter_guess /*Pa*/, double m_dot_htf_ND /*-*/,
        double& P_ND_tot /*-*/, double& Q_ND_tot /*-*/);

    class C_MEQ__P_cond_OD : public C_monotonic_equation
    {
    private:
        C_pc_Rankine_indirect_224* mpc_pc;
        double m_T_htf_hot_ND;      //[-]
        double m_m_dot_htf_ND;      //[-]
        double m_T_db;              //[K]
        double m_T_wb;              //[K]
        double m_P_amb;             //[Pa]
        double m_F_wc;              //[-]
        double m_F_wcmin;           //[-]
        double m_F_wcmax;           //[-]
        double m_T_cold_rad;        //[C]
        double m_dT_cw_rad_cooling;   //[C]

        // Calculated
        double m_P_cycle;       //[kWe]
        double m_eta;           //[-]
        double m_W_dot_cooling; //[MWe]
        double m_m_dot_makeup;  //[kg/s]
        double m_f_hrsys;       //[-]
        double m_T_cond_out_rad;//[C]

    public:

        C_MEQ__P_cond_OD(C_pc_Rankine_indirect_224* pc_pc,
            double T_htf_hot_ND /*-*/, double m_dot_htf_ND /*-*/,
            double T_db /*K*/, double T_wb /*K*/, double P_amb /*Pa*/,
            double F_wc /*-*/, double F_wcmin /*-*/, double F_wcmax /*-*/,
            double T_cold_rad /*C*/, double dT_cw_rad_cooling /*C*/);

        virtual int operator()(double P_cond /*Pa*/, double* diff_P_cond /*-*/) override;

        void get_solved_values(double& P_cycle /*kWe*/, double& eta /*-*/, double& W_dot_cooling /*MWe*/,
            double& m_dot_makeup /*kg/s*/, double& f_hrsys /*-*/, double& T_cond_out_rad /*C*/);
    };

	double Interpolate(int YT, int XT, double X, double Z = std::numeric_limits<double>::quiet_NaN());

    double Calculate_T_htf_cold_Converge_Cp(double q_dot_htf /*kWt*/, double T_htf_hot /*K*/, double m_dot_htf /*kg/s*/);

	// Isopentane
	double T_sat4(double P/*Bar*/) 
	{
		return 284.482349 + 20.8848464*P - 1.5898147*P*P + 0.0655241456*P*P*P - 0.0010168822*P*P*P*P; /*return value in Kelvin*/
	}
   
public:
	
    enum
	{
		E_Q_DOT_HTF,		//[MWt] Cycle thermal power input
		E_M_DOT_HTF,		//[kg/hr] Cycle HTF mass flow rate
		E_Q_DOT_STARTUP,	//[MWt] Cycle startup thermal power
		E_W_DOT,			//[MWe] Cycle electricity output (gross)
		E_T_HTF_IN,			//[C] Cycle HTF inlet temperature
		E_T_HTF_OUT,		//[C] Cycle HTF outlet temperature
		E_T_COND_OUT,		//[C] Cycle condenser water outlet temperature
		E_T_COLD,			//[C] Cold storage temperature
		E_M_COLD,			//[C] Cold storage mass
		E_M_WARM,			//[C] Cold storage warm (return) tank mass
		E_T_WARM,			//[C] Cold storage warm (return) tank temperature
		E_T_RADOUT,			//[C] Radiator outlet temperature
		E_M_DOT_WATER,		//[kg/hr] Cycle water consumption: makeup + cooling	
		E_P_COND,			//[Pa] Cycle condensing pressure
		E_RADCOOL_CNTRL,	//Code showing the status of radiative cooling with cold storage
        E_W_DOT_HTF_PUMP,   //[MWe] HTF pump power
        E_W_DOT_COOLER,     //[MWe] Cooling parasitic
        E_P_COND_ITER_ERR,  //[-] Relative iteration error on condenser pressure

        // State
        E_PC_OP_MODE_FINAL, //[-] Final receiver operating mode (see E_csp_power_cycle_modes)
        E_PC_STARTUP_TIME_REMAIN_FINAL,     //[hr] Final receiver startup time remaining
        E_PC_STARTUP_ENERGY_REMAIN_FINAL,   //[W-hr] Final reeiver startup energy remaining

		// Variables added for backwards compatability with TCS
        E_ETA_THERMAL_STEP_AVERAGED,    //[-]
		E_M_DOT_HTF_REF,		//[kg/hr] HTF mass flow rate at design

        // Dependent output variables
        E_ETA_THERMAL,		//[-] Cycle thermal efficiency (gross)
	};

	C_csp_reported_outputs mc_reported_outputs;
	
	// Instantiate two fully mixed tanks class for cold storage AND three node model
	C_csp_cold_tes mc_two_tank_ctes;
	C_csp_cold_tes::S_csp_cold_tes_outputs mc_two_tank_ctes_outputs;	// for outputs
	C_csp_stratified_tes mc_stratified_ctes;
	C_csp_stratified_tes::S_csp_strat_tes_outputs mc_stratified_ctes_outputs;

	double m_dot_cold_avail;
	double m_dot_warm_avail;
	double m_dot_condenser;
	double T_warm_prev_K;
	double T_cold_prev_K;
	double T_cold_prev;
	double dT_cw_design;
	double T_s_measured;
	double T_s_corr;
	double T_s_K;
	int idx_time;

	// Instantiate radiator model
	C_csp_radiator mc_radiator;
	double m_dot_radfield;
	double m_dot_radact;
	double W_radpumptest;
	struct S_params
	{
			// Parameters with common SSCINPUT name for both Rankine Cycle and User Defined Cycle
		double m_P_ref;				//[MW] design electric power output, converted to kW in init()
		double m_eta_ref;			//[%] design conversion efficiency
		double m_T_htf_hot_ref;		//[C] design HTF inlet temperature
		double m_T_htf_cold_ref;	//[C] design HTF output temperature
		double m_cycle_max_frac;	//[-] Maximum turbine over-design operation fraction
		double m_cycle_cutoff_frac;	//[-] Minimum turbine operation fraction
		double m_q_sby_frac;		//[-] fraction of thermal power required for standby mode
		double m_startup_time;		//[hr] time needed for power block startup
		double m_startup_frac;		//[-] fraction of design thermal power needed for startup
		double m_htf_pump_coef;		//[kW/kg/s] Pumping power to move 1 kg/s of HTF through power cycle

        // Initial state parameters
        C_csp_power_cycle::E_csp_power_cycle_modes m_operating_mode_initial;			//Operating mode at start of simulation
        double m_startup_time_remain_init;		//[hr]
        double m_startup_energy_remain_init;	//[kW-hr]

		int m_pc_fl;				//[-] integer flag identifying Heat Transfer Fluid (HTF) in power block {1-27}
		util::matrix_t<double> m_pc_fl_props;
        double DP_SGS;              //[bar] pressure drop within the steam generator system

        bool m_is_calc_htf_pump_coef;   //[-] Default false, use m_htf_pump_coef
        double m_W_dot_htf_pump_target; //[MWe]

		// Steam Rankine or User-Defined
		bool m_is_user_defined_pc;				//[-] True: user-defined power cycle, False: Built-in Rankine Cycle model
        int m_is_udpc_sco2_regr;               //[-] False: default, base udpc interpolation, True: use sco2 heuristic regression

			// Parameters that have different SSCINPUT names for Rankine Cycle and User Defined Cycle
		double m_dT_cw_ref;			//[C] design temp difference between cooling water inlet/outlet
		double m_T_amb_des;			//[C] design ambient temperature
		double m_P_boil_des;     	//[bar] boiler operating pressure
		int m_CT;					//[-] integer flag for cooling technology type {1=evaporative cooling, 2=air cooling, 3=hybrid cooling}
		int m_tech_type;			//[-] Flag indicating which coef. set to use. (1=tower,2=trough,3=user) 
		double m_T_approach;		//[C] cooling tower approach temp
		double m_T_ITD_des;			//[C] design ITD for dry system
		double m_P_cond_ratio;		//[-] condenser pressure ratio
		double m_pb_bd_frac;		//[-] blowdown steam fraction
		double m_P_cond_min;		//[inHG] minimum condenser pressure, converted to Pa in code
		int m_n_pl_inc;				//[-] Number of part-load increments for the heat rejection system
		
		
		std::vector<double> m_F_wc;		//[-] hybrid cooling dispatch fractions 1 thru 9 (array index 0-8)
		
		// Parameters for user-defined power cycle
            // Lookup table that is the combination of the three above T_htf_hot, T_amb, and m_dot_htf tables (this is the newer table format)
        util::matrix_t<double> mc_combined_ind;
		double m_W_dot_cooling_des;		//[MW] Cooling parasitic at design conditions
		double m_m_dot_water_des;		//[kg/s] Power cycle water use at design conditions

		S_params()
		{
			m_P_ref = m_eta_ref = m_T_htf_hot_ref = m_T_htf_cold_ref = m_dT_cw_ref = m_T_amb_des =
				m_q_sby_frac = m_P_boil_des = m_startup_time = m_startup_frac = m_T_approach = m_T_ITD_des =
				m_P_cond_ratio = m_pb_bd_frac = m_P_cond_min = m_htf_pump_coef = std::numeric_limits<double>::quiet_NaN();

            m_W_dot_htf_pump_target = std::numeric_limits<double>::quiet_NaN();

			m_pc_fl = m_CT = m_tech_type = m_n_pl_inc = -1;
            m_is_calc_htf_pump_coef = false;

			// Initialize parameters for user-defined power cycle
			m_is_user_defined_pc = false;
            m_is_udpc_sco2_regr = C_ud_power_cycle::SIMPLE_HEURISTIC;
				
			m_W_dot_cooling_des = m_m_dot_water_des = std::numeric_limits<double>::quiet_NaN();

            m_operating_mode_initial = C_csp_power_cycle::E_csp_power_cycle_modes::OFF;
            m_startup_time_remain_init = std::numeric_limits<double>::quiet_NaN();		//[hr]
            m_startup_energy_remain_init = std::numeric_limits<double>::quiet_NaN();	//[kW-hr]
		}
	};

	S_params ms_params;

	C_pc_Rankine_indirect_224();

	~C_pc_Rankine_indirect_224(){};

	virtual void init(C_csp_power_cycle::S_solved_params &solved_params);

	virtual C_csp_power_cycle::E_csp_power_cycle_modes get_operating_state();

    virtual double get_cold_startup_time(); 
    virtual double get_warm_startup_time();
    virtual double get_hot_startup_time();
    virtual double get_standby_energy_requirement();    //[MW]
    virtual double get_cold_startup_energy();    //[MWh]
    virtual double get_warm_startup_energy();    //[MWh]
    virtual double get_hot_startup_energy();    //[MWh]
    virtual double get_max_thermal_power();     //MW
    virtual double get_min_thermal_power();     //MW
	virtual void get_max_power_output_operation_constraints(double T_amb /*C*/, double & m_dot_HTF_ND_max, double & W_dot_ND_max);	//[-] Normalized over design power
	virtual double get_efficiency_at_TPH(double T_degC, double P_atm, double relhum_pct, double *w_dot_condenser = 0);
    virtual double get_efficiency_at_load(double load_frac, double *w_dot_condenser=0);
	virtual double get_htf_pumping_parasitic_coef();		//[kWe/kWt]

    virtual double get_design_pumping_power();      //[MWe]
    virtual double get_design_cooling_power();      //[MWe]

	// This can vary between timesteps for Type224, depending on remaining startup energy and time
	virtual double get_max_q_pc_startup();		//[MWt]


	virtual void call(const C_csp_weatherreader::S_outputs &weather, 
		C_csp_solver_htf_1state &htf_state_in,
		const C_csp_power_cycle::S_control_inputs &inputs,
		C_csp_power_cycle::S_csp_pc_out_solver &out_solver,
		//C_csp_power_cycle::S_csp_pc_out_report &out_report,
		const C_csp_solver_sim_info &sim_info);

	virtual void converged();

	virtual void write_output_intervals(double report_time_start,
		const std::vector<double> & v_temp_ts_time_end, double report_time_end);

	virtual void assign(int index, double *p_reporting_ts_array, size_t n_reporting_ts_array);

    void get_design_parameters(double& m_dot_htf_des /*kg/hr*/,
        double& cp_htf_des_at_T_ave /*kJ/kg-K*/,
        double& W_dot_htf_pump /*MWe*/, double& W_dot_cooling /*MWe*/,
        // UDPC
        int& n_T_htf_pars, int& n_T_amb_pars, int& n_m_dot_pars,
        double& T_htf_ref_calc /*C*/, double& T_htf_low_calc /*C*/, double& T_htf_high_calc /*C*/,
        double& T_amb_ref_calc /*C*/, double& T_amb_low_calc /*C*/, double& T_amb_high_calc /*C*/,
        double& m_dot_htf_ND_ref_calc, double& m_dot_htf_ND_low_calc /*-*/, double& m_dot_htf_ND_high_calc /*-*/,
        double& W_dot_gross_ND_des, double& Q_dot_HTF_ND_des, double& W_dot_cooling_ND_des, double& m_dot_water_ND_des);

    double get_design_input_cooling_power()
    {
        // For the UDPC model, the calculated design value can be different
        // than the input design value due to the UDPC data not = 1 at the design independent variables

        // This will return NaN if not using the UDPC model
        return ms_params.m_W_dot_cooling_des;   //[MWe]
    }
};


#endif //__csp_solver_pc_Rankine_indirect_224_
