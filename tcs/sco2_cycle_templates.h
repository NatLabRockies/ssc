#ifndef __SCO2_CYCLE_TEMPLATES_
#define __SCO2_CYCLE_TEMPLATES_

#include "sco2_cycle_components.h"
#include "heat_exchangers.h"
#include <string>
#include "math.h"

class C_sco2_cycle_core
{
public:

	enum E_cycle_state_points
	{
		// index values for c++ 0-based vectors for temperature, pressure, etc.
		MC_IN = 0,		// Main compressor inlet
		MC_OUT,			// Main compressor outlet
		LTR_HP_OUT,		// Low temp recuperator high pressure outlet
		MIXER_OUT,		// Mixer: LTR_HP_OUT + Recompressor outlet
		HTR_HP_OUT,		// High temp recuperator high pressure outlet
		TURB_IN,		// Turbine inlet
		TURB_OUT,		// Turbine outlet
		HTR_LP_OUT,		// High temp recuperator low pressure outlet
		LTR_LP_OUT,		// Low temp recuperator low pressure outlet
		RC_OUT,			// Recompresor outlet
		PC_IN,			// Precompressor inlet (partial cooling cycle)
		PC_OUT,			// Precompressor outlet (partial cooling cycle)
        BYPASS_OUT,     // Bypass outlet (htr bypass cycle)
        MIXER2_OUT,     // Mixer 2 Outlet (htr bypass cycle)
        TURB2_OUT,      // Secondary Turbine Outlet (turbine split flow cycle)

		END_SCO2_STATES
	};

    enum E_turbine_inlet_temp_mode
    {
        E_SOLVE_PHX = 0,    // Model solves co2/HTF PHX to find turbine inlet temperature
        E_SET_T_T_IN        // Model sets turbine inlet temperature to HTF inlet temperature
    };

    class E_cycle_error_msg
    {
    public:

        // Error Types
        enum E_cycle_error_types
        {
            E_CANNOT_PRODUCE_POWER = 200,
            E_CO2_PROPS_ERROR,
            E_ETA_THRESHOLD,
            E_HTR_LTR_CONVERGENCE,
            E_AIR_COOLER_CONVERGENCE,
            E_NO_ERROR
        };

        // Get error message corresponding to error type
        // NO COMMAS in message
        static std::string get_error_string(int error_enum)
        {
            switch (error_enum)
            {
                case((int)E_CANNOT_PRODUCE_POWER):
                    return "Cycle cannot produce power";
                    break;
                case((int)E_CO2_PROPS_ERROR):
                    return "Error calculating sCO2 properties";
                case((int)E_ETA_THRESHOLD):
                    return "Eta below threshold";
                case((int)E_NO_ERROR):
                    return "No error";
                    break;
                case((int)E_HTR_LTR_CONVERGENCE):
                    return "HTR LTR convergence issue";
                case((int)E_AIR_COOLER_CONVERGENCE):
                    return "Air cooler did not converge";
                default:
                    return "Error code not recognized";
                    break;
            }
        }
        
    };

    enum class E_turbo_gen_motor_config
    {
        // Options to apply motor and generator losses

        E_SINGLE_SHAFT,      // Assumes compressors driven by turbine. Generator efficiency applies to net power

        UNSPECIFIED
    };

	struct S_design_limits
	{
		double m_UA_net_power_ratio_max;		//[-/K]
		double m_UA_net_power_ratio_min;		//[-/K]

		double m_T_mc_in_min;					//[K]

		S_design_limits()
		{
			m_UA_net_power_ratio_max = m_UA_net_power_ratio_min = std::numeric_limits<double>::quiet_NaN();
		}
	};

	struct S_design_solved
	{
		std::vector<double> m_temp, m_pres, m_enth, m_entr, m_dens;		// thermodynamic states (K, kPa, kJ/kg, kJ/kg-K, kg/m3)
		double m_eta_thermal;	//[-]
		double m_W_dot_net;		//[kWe]
		double m_m_dot_mc;		//[kg/s]
		double m_m_dot_rc;		//[kg/s]
		double m_m_dot_pc;		//[kg/s]
		double m_m_dot_t;		//[kg/s]
        double m_m_dot_t2;		//[kg/s]
		double m_recomp_frac;	//[-]
		double m_UA_LTR;			//[kW/K]
		double m_UA_HTR;			//[kW/K]
		double m_W_dot_mc;      //[kWe]
		double m_W_dot_rc;		//[kWe]
		double m_W_dot_pc;		//[kWe]
		double m_W_dot_t;		//[kWe]
        double m_W_dot_t2;      //[kWe]
        double m_bypass_frac;       //[-] Bypass Fraction
        double m_turbine_split_frac;    // [-] Turbine split fraction (TSF only)

		double m_W_dot_cooler_tot;	//[kWe]

		bool m_is_rc;

		C_comp_multi_stage::S_des_solved ms_mc_ms_des_solved;
		C_comp_multi_stage::S_des_solved ms_rc_ms_des_solved;
		C_comp_multi_stage::S_des_solved ms_pc_ms_des_solved;
		C_turbine::S_design_solved ms_t_des_solved;
        C_turbine::S_design_solved ms_t2_des_solved;
		C_HX_counterflow_CRM::S_des_solved ms_LTR_des_solved;
		C_HX_counterflow_CRM::S_des_solved ms_HTR_des_solved;

		C_CO2_to_air_cooler::S_des_solved ms_mc_air_cooler; 
		C_CO2_to_air_cooler::S_des_solved ms_pc_air_cooler; 

		S_design_solved()
		{
			m_eta_thermal = m_W_dot_net = m_m_dot_mc = m_m_dot_rc = m_m_dot_t = m_recomp_frac =
				m_UA_LTR = m_UA_HTR =
				m_W_dot_mc = m_W_dot_rc = m_W_dot_pc = m_W_dot_t =
				m_W_dot_cooler_tot = std::numeric_limits<double>::quiet_NaN();

			m_is_rc = true;
		}
	};

	struct S_auto_opt_design_hit_eta_parameters
	{
		double m_eta_thermal;				//[-] Cycle thermal efficiency
		double m_T_pc_in;					//[K] Pre-compressor inlet temperature
		std::vector<double> m_DP_PC_pre;    //(cold, hot) positive values are absolute [kPa], negative values are relative (-)
		    // LTR thermal design
        int m_LTR_target_code;              //[-] 1 = UA, 2 = min dT, 3 = effectiveness
        double m_LTR_UA;					//[kW/K] target LTR conductance
        double m_LTR_min_dT;                //[K] target LTR minimum temperature difference
        double m_LTR_eff_target;            //[-] target LTR effectiveness
        double m_LTR_eff_max;			    //[-] Maximum allowable effectiveness in LT recuperator
        NS_HX_counterflow_eqs::E_UA_target_type m_LTR_od_UA_target_type;
            // HTR thermal design
        int m_HTR_target_code;              //[-] 1 = UA, 2 = min dT, 3 = effectiveness
        double m_HTR_UA;					//[kW/K] target HTR conductance
        double m_HTR_min_dT;                //[K] target HTR min temperature difference
        double m_HTR_eff_target;            //[-] target HTR effectiveness
        double m_HTR_eff_max;		        //[-] Maximum allowable effectiveness in HT recuperator
        NS_HX_counterflow_eqs::E_UA_target_type m_HTR_od_UA_target_type;
            //
		double m_eta_pc;					//[-] design-point efficiency of the pre-compressor; 
		double m_des_tol;					//[-] Convergence tolerance
		double m_des_opt_tol;				//[-] Optimization tolerance

        double m_eta_thermal_cutoff;        //[] Minimum eta to fully design cycle (returns failure below value)

		// Air cooler parameters
		bool m_is_des_air_cooler;		//[-] False will skip physical air cooler design. UA will not be available for cost models.

		double m_is_recomp_ok;          //[-] 1 = Yes, 0 = simple cycle only, < 0 = fix f_recomp to abs(input)
        double m_is_bypass_ok;          //[-] 1 = Yes, 0 = no bp, < 0 = fix f_bypass to abs(input)

		int m_des_objective_type;			//[2] = min phx deltat then max eta, [else] max eta
		double m_min_phx_deltaT;			//[C]

		bool m_fixed_P_mc_out;			//[-] if true, P_mc_out is fixed at 'm_P_high_limit'

		double m_PR_HP_to_LP_guess;     //[-] Initial guess for ratio of P_mc_out to P_LP_in
		bool m_fixed_PR_HP_to_LP;       //[-] if true, ratio of P_mc_out to P_mc_in is fixed at PR_mc_guess

        double m_f_PR_HP_to_IP_guess;       //[-] Initial guess fraction of HP-to-LP deltaP for HP-to-IP (partial cooling cycle)
        bool m_fixed_f_PR_HP_to_IP;         //[-] if true, use guess

		// Callback function only log
		bool(*mf_callback_log)(std::string &log_msg, std::string &progress_msg, void *data, double progress, int out_type);
		void *mp_mf_active;

		S_auto_opt_design_hit_eta_parameters()
		{
            m_eta_thermal = m_T_pc_in =
                m_LTR_UA = m_LTR_min_dT = m_LTR_eff_target = m_LTR_eff_max = 
                m_HTR_UA = m_HTR_min_dT = m_HTR_eff_target = m_HTR_eff_max =
                m_eta_pc = 
				m_des_tol = m_des_opt_tol = 
                m_is_recomp_ok = m_is_bypass_ok =
				m_PR_HP_to_LP_guess = m_f_PR_HP_to_IP_guess = std::numeric_limits<double>::quiet_NaN();

            // Recuperator design target codes
            m_LTR_target_code = 1;      // default to target conductance
            m_LTR_od_UA_target_type = NS_HX_counterflow_eqs::E_UA_target_type::E_calc_UA;
            m_HTR_target_code = 1;      // default to target conductance
            m_HTR_od_UA_target_type = NS_HX_counterflow_eqs::E_UA_target_type::E_calc_UA;

			// Air cooler default
			m_is_des_air_cooler = true;

			// Default to standard optimization to maximize cycle efficiency
			m_des_objective_type = 1;
			m_min_phx_deltaT = 0.0;		//[C]

            m_fixed_P_mc_out = false;       //[-] If fasle, then should default to optimizing this parameter
            m_fixed_PR_HP_to_LP = false;    //[-] If false, then should default to optimizing this parameter
            m_fixed_f_PR_HP_to_IP = false;  //[-] If false, then should default to optimizing this parameter

			mf_callback_log = 0;
			mp_mf_active = 0;

			m_DP_PC_pre.resize(2);
			std::fill(m_DP_PC_pre.begin(), m_DP_PC_pre.end(), std::numeric_limits<double>::quiet_NaN());
		}
	};

	struct S_auto_opt_design_parameters
	{
		double m_T_pc_in;					//[K] Pre-compressor inlet temperature
		std::vector<double> m_DP_PC_pre;    //(cold, hot) positive values are absolute [kPa], negative values are relative (-)
		double m_UA_rec_total;				//[kW/K] Total design-point recuperator UA
            // LTR thermal design
        int m_LTR_target_code;              //[-] 1 = UA, 2 = min dT, 3 = effectiveness
        double m_LTR_UA;					//[kW/K] target LTR conductance
        double m_LTR_min_dT;                //[K] target LTR minimum temperature difference
        double m_LTR_eff_target;            //[-] target LTR effectiveness
		double m_LTR_eff_max;			    //[-] Maximum allowable effectiveness in LT recuperator
        NS_HX_counterflow_eqs::E_UA_target_type m_LTR_od_UA_target_type;
            // HTR thermal design
        int m_HTR_target_code;              //[-] 1 = UA, 2 = min dT, 3 = effectiveness
        double m_HTR_UA;					//[kW/K] target HTR conductance
        double m_HTR_min_dT;                //[K] target HTR min temperature difference
        double m_HTR_eff_target;            //[-] target HTR effectiveness
        double m_HTR_eff_max;			    //[-] Maximum allowable effectiveness in HT recuperator
        NS_HX_counterflow_eqs::E_UA_target_type m_HTR_od_UA_target_type;
            // 
		double m_eta_pc;					//[-] design-point efficiency of the pre-compressor; 

		double m_des_tol;					//[-] Convergence tolerance
		double m_des_opt_tol;				//[-] Optimization tolerance

        double m_eta_thermal_cutoff;        //[] Minimum eta to fully design cycle (returns failure below value)

		// Air cooler parameters
		bool m_is_des_air_cooler;		//[-] False will skip physical air cooler design. UA will not be available for cost models.

		double m_is_recomp_ok;			//[-] 1 = Yes, 0 = simple cycle only, < 0 = fix f_recomp to abs(input)
        double m_is_bypass_ok;          //[-] 1 = Yes, 0 = no bypass, < 0 = fix bp_frac to abs(input)
        double m_is_turbinesplit_ok;    //[-] 1 = Yes, 0 = no secondary turbine, < 0 = fix split_frac to abs(input) (Turbine split flow ONLY)

		bool m_fixed_P_mc_out;			//[-] if true, P_mc_out is fixed at 'm_P_high_limit'

		double m_PR_HP_to_LP_guess;     //[-] Initial guess for ratio of P_mc_out to P_LP_in
		bool m_fixed_PR_HP_to_LP;       //[-] if true, ratio of P_mc_out to P_LP_in is fixed at PR_mc_guess
		
        double m_f_PR_HP_to_IP_guess;     //[-] Initial guess fraction of HP-to-LP deltaP for HP-to-IP (partial cooling cycle)
        bool m_fixed_f_PR_HP_to_IP;       //[-] if true, fix at guess

		int m_des_objective_type;		//[2] = min phx deltat then max eta, [else] max eta
		double m_min_phx_deltaT;		//[C]

		// Callback function only log
		bool(*mf_callback_log)(std::string &log_msg, std::string &progress_msg, void *data, double progress, int out_type);
		void *mp_mf_active;


		S_auto_opt_design_parameters()
		{
            m_T_pc_in =
				m_UA_rec_total = 
                m_LTR_UA = m_LTR_min_dT = m_LTR_eff_target = m_LTR_eff_max = 
                m_HTR_UA = m_HTR_min_dT = m_HTR_eff_target = m_HTR_eff_max =
                m_eta_pc = m_des_tol = m_des_opt_tol = 
                m_is_recomp_ok = m_is_bypass_ok =
				m_PR_HP_to_LP_guess = m_f_PR_HP_to_IP_guess = std::numeric_limits<double>::quiet_NaN();

            // Recuperator design target codes
            m_LTR_target_code = 1;      // default to target conductance
            m_LTR_od_UA_target_type = NS_HX_counterflow_eqs::E_UA_target_type::E_calc_UA;
            m_HTR_target_code = 1;      // default to target conductance
            m_HTR_od_UA_target_type = NS_HX_counterflow_eqs::E_UA_target_type::E_calc_UA;

			// Air cooler default
			m_is_des_air_cooler = true;

            m_fixed_P_mc_out = false;       //[-] If false, then should default to optimizing this parameter
            m_fixed_PR_HP_to_LP = false;    //[-] If false, then should default to optimizing this parameter
            m_fixed_f_PR_HP_to_IP = false;  //[-] If false, then should default to optimizing this parameter

			// Default to standard optimization to maximize cycle efficiency
			m_des_objective_type = 1;
			m_min_phx_deltaT = 0.0;		//[C]

			mf_callback_log = 0;
			mp_mf_active = 0;

			m_DP_PC_pre.resize(2);
			std::fill(m_DP_PC_pre.begin(), m_DP_PC_pre.end(), std::numeric_limits<double>::quiet_NaN());
		}
	};

	struct S_od_solved
	{
		std::vector<double> m_temp, m_pres, m_enth, m_entr, m_dens;		// thermodynamic states (K, kPa, kJ/kg, kJ/kg-K, kg/m3)
		double m_eta_thermal;	//[-]
		double m_W_dot_net;		//[kWe]
		double m_Q_dot;			//[kWt]
        double m_Q_dot_mc_cooler;   //[MWt]
        double m_Q_dot_pc_cooler;   //[MWt]
		double m_m_dot_mc;		//[kg/s]
		double m_m_dot_rc;		//[kg/s]
		double m_m_dot_pc;		//[kg/s]
		double m_m_dot_t;		//[kg/s]
		double m_recomp_frac;	//[-]

		double m_mc_f_bypass;	//[-]
		double m_pc_f_bypass;	//[-]

		C_comp_multi_stage::S_od_solved ms_mc_ms_od_solved;
		C_comp_multi_stage::S_od_solved ms_rc_ms_od_solved;
		C_comp_multi_stage::S_od_solved ms_pc_ms_od_solved;
		C_turbine::S_od_solved ms_t_od_solved;
		C_HX_counterflow_CRM::S_od_solved ms_LT_recup_od_solved;
		C_HX_counterflow_CRM::S_od_solved ms_HT_recup_od_solved;

		C_CO2_to_air_cooler::S_od_solved ms_mc_air_cooler_od_solved;  
		C_CO2_to_air_cooler::S_od_solved ms_pc_air_cooler_od_solved;  

		S_od_solved()
		{
			m_eta_thermal = m_W_dot_net = m_Q_dot = m_Q_dot_mc_cooler = m_Q_dot_pc_cooler =
                m_m_dot_mc = m_m_dot_rc = m_m_dot_pc =
				m_m_dot_t = m_recomp_frac = m_mc_f_bypass = m_pc_f_bypass = std::numeric_limits<double>::quiet_NaN();
		}
	};

	struct S_od_par
	{
		double m_T_mc_in;		//[K] Compressor inlet temperature
		double m_T_pc_in;		//[K] Precompressor inlet temperature
		double m_T_t_in;		//[K] Turbine inlet temperature
		double m_P_LP_comp_in;	//[kPa] Compressor inlet pressure (low pressure comp in partial cooling cycle)
		
		double m_f_mc_pc_bypass;	//[-] Fraction of pre and main compressor flow that is bypassed back to the respective compressor cooler

        // Turbine inlet mode
        int m_T_t_in_mode;

        // Shaft speed control options
            // RC shaft speed control option
        bool m_is_rc_N_od_at_design;  //[-] True: rc off design shaft speed set to design shaft speed
                                        // False: = m_rc_N_od_in
        double m_rc_N_od_f_des;        //[-] input RC off design shaft speed fraction of design. used if m_is_rc_N_od_at_design = true
            // MC shaft speed control option
        bool m_is_mc_N_od_at_design;  //[-] True: mc off design shaft speed set to design shaft speed
                                        // False: = m_mc_N_od_in
        double m_mc_N_od_f_des;        //[-] input MC off design shaft speed fraction of design. used if m_is_mc_N_od_at_design = true
            // MC shaft speed control option
        bool m_is_pc_N_od_at_design;  //[-] True: mc off design shaft speed set to design shaft speed
                                        // False: = m_mc_N_od_in
        double m_pc_N_od_f_des;        //[-] input MC off design shaft speed fraction of design. used if m_is_mc_N_od_at_design = true

        // PHX pressure drop options
        bool m_is_PHX_dP_input;     //[-] False: use built-in pressure drop scaling
                                    //[-] True: use input fractional pressure drop
        double m_PHX_f_dP;          //[-] PHX fractional pressure drop

        // Other convergence parameters
		double m_tol;			//[-] Convergence tolerance

        int m_count_off_design_core;

		S_od_par()
		{
			m_T_mc_in = m_T_pc_in = m_T_t_in = m_P_LP_comp_in = 
                m_rc_N_od_f_des = m_mc_N_od_f_des = m_pc_N_od_f_des =
                m_PHX_f_dP =
				m_tol = std::numeric_limits<double>::quiet_NaN();

            m_T_t_in_mode = E_SOLVE_PHX;  //[-] Default to using PHX and HTF temp and mass flow rate

            m_is_rc_N_od_at_design = true;  //[-] Default to using design RC shaft speed
            m_is_mc_N_od_at_design = true;  //[-] Default to using design MC shaft speed
            m_is_pc_N_od_at_design = true;  //[-] Default to using design PC shaft speed

            m_is_PHX_dP_input = false;  //[-] Default to using built-in pressure drop scaling

			m_f_mc_pc_bypass = 0.0;	//[-]
		}
	};
	
    struct S_od_deltaP
    {
        double m_od_diff_P_LTR_HP_out_calc_less_guess;  //[kPa]
        double m_od_diff_P_HTR_HP_out_calc_less_guess;  //[kPa]
        double m_od_diff_P_PHX_out_calc_less_guess;     //[kPa]
        double m_od_diff_P_HTR_LP_out_calc_less_guess;  //[kPa]
        double m_od_diff_P_LTR_LP_out_calc_less_guess;  //[kPa]
        double m_od_diff_P_mc_cooler_out_calc_less_guess;   //[kPa]
        double m_od_diff_P_pc_cooler_out_calc_less_guess;   //[kPa]

        S_od_deltaP()
        {
            m_od_diff_P_LTR_HP_out_calc_less_guess = 0.0;
            m_od_diff_P_HTR_HP_out_calc_less_guess = 0.0;
            m_od_diff_P_PHX_out_calc_less_guess = 0.0;
            m_od_diff_P_HTR_LP_out_calc_less_guess = 0.0;
            m_od_diff_P_LTR_LP_out_calc_less_guess = 0.0;
            m_od_diff_P_mc_cooler_out_calc_less_guess = 0.0;
            m_od_diff_P_pc_cooler_out_calc_less_guess = 0.0;
        }
    };

protected:

	S_design_solved ms_des_solved;

	S_auto_opt_design_parameters ms_auto_opt_des_par;

	S_od_solved ms_od_solved;

	S_od_par ms_od_par;

	S_design_limits ms_des_limits;

	void clear_ms_od_solved()
	{
		S_od_solved s_od_solved_temp;
		ms_od_solved = s_od_solved_temp;
	}

    C_sco2_cycle_core::E_turbo_gen_motor_config m_turbo_gen_motor_config;
    double m_eta_generator;     //[-] Mechanical-to-electrical efficiency of generator
    double m_T_mc_in;           //[K] Compressor inlet temperature
    double m_W_dot_net;			//[kWe] Target net cycle power
    double m_T_t_in;			//[K] Turbine inlet temperature
    double m_P_high_limit;		//[kPa] maximum allowable pressure in cycle
    std::vector<double> m_DP_LTR;		//(cold, hot) positive values are absolute [kPa], negative values are relative (-)
    std::vector<double> m_DP_HTR;		//(cold, hot) positive values are absolute [kPa], negative values are relative (-)
    std::vector<double> m_DP_PC_main;	//(cold, hot) positive values are absolute [kPa], negative values are relative (-)
    std::vector<double> m_DP_PHX;		//(cold, hot) positive values are absolute [kPa], negative values are relative (-)

    int m_LTR_N_sub_hxrs;               //[-] Number of sub-hxs to use in hx model

    int m_HTR_N_sub_hxrs;               //[-] Number of sub-hxs to use in hx model

    double m_eta_mc;			//[-] design-point efficiency of the main compressor; isentropic if positive, polytropic if negative
    int m_mc_comp_model_code;   //[-] Main compressor model - see sco2_cycle_components.h 
    double m_eta_rc;		    //[-] design-point efficiency of the recompressor; isentropic if positive, polytropic if negative
    int m_rc_comp_model_code;   //[-] Recompressor model - see sco2_cycle_components.h 
    double m_eta_t;				//[-] design-point efficiency of the turbine; isentropic if positive, polytropic if negative
    double m_N_turbine;			//[rpm] Turbine shaft speed (negative values link turbine to compressor)

    double m_frac_fan_power;    //[-] Fraction of total cycle power 'S_des_par_cycle_dep.m_W_dot_fan_des' consumed by air fan
    double m_eta_fan;           //[-] Fan isentropic efficiency
    double m_deltaP_cooler_frac;//[-] Fraction of high side (of cycle, i.e. comp outlet) pressure that is allowed as pressure drop to design the ACC
    double m_T_amb_des;		    //[K] Design point ambient temperature
    double m_elevation;			//[m] Elevation (used to calculate ambient pressure)
    int m_N_nodes_pass;         //[-] Number of nodes per pass
    double m_yr_inflation;      //[yr] Inflation target year

public:

	C_sco2_cycle_core(C_sco2_cycle_core::E_turbo_gen_motor_config turbo_gen_motor_config,
        double eta_generator /*-*/,
        double T_mc_in /*K*/,
        double W_dot_net /*kWe*/,
        double T_t_in /*K*/, double P_high_limit /*kPa*/,
        std::vector<double> DP_LTR, std::vector<double> DP_HTR,
        std::vector<double> DP_PC_main, std::vector<double> DP_PHX,
        int LTR_N_sub_hxrs /*-*/, int HTR_N_sub_hxrs /*-*/,
        double eta_mc /*-*/, int mc_comp_model_code /*-*/,
        double eta_rc /*-*/,
        double eta_t /*-*/, double N_turbine /*rpm*/,
        double frac_fan_power /*-*/, double eta_fan /*-*/, double deltaP_cooler_frac /*-*/,
        int N_nodes_pass /*-*/,
        double T_amb_des /*K*/, double elevation /*m*/,
        double yr_inflation /*yr*/)
	{
        m_turbo_gen_motor_config = turbo_gen_motor_config;
        m_eta_generator = eta_generator;    //[-]
        m_W_dot_net = W_dot_net;            //[kWe]

        m_T_mc_in = T_mc_in;        //[K]
        m_T_t_in = T_t_in;          //[K]
        m_P_high_limit = P_high_limit;  //[kPa]

        m_DP_LTR = DP_LTR;
        m_DP_HTR = DP_HTR;
        m_DP_PC_main = DP_PC_main;
        m_DP_PHX = DP_PHX;

        m_LTR_N_sub_hxrs = LTR_N_sub_hxrs;  //[-]
        m_HTR_N_sub_hxrs = HTR_N_sub_hxrs;  //[-]

        m_eta_mc = eta_mc;          //[-]
        m_mc_comp_model_code = mc_comp_model_code;  //[-]
        m_eta_rc = eta_rc;          //[-]
        m_rc_comp_model_code = C_comp__psi_eta_vs_phi::E_snl_radial_via_Dyreby;
        m_eta_t = eta_t;            //[-]
        m_N_turbine = N_turbine;    //[rpm]

        m_frac_fan_power = frac_fan_power;          //[-]
        m_eta_fan = eta_fan;                        //[-]
        m_deltaP_cooler_frac = deltaP_cooler_frac;  //[-]
        m_N_nodes_pass = N_nodes_pass;              //[-]

        m_T_amb_des = T_amb_des;                    //[K]
        m_elevation = elevation;                    //[m]

        m_yr_inflation = yr_inflation;                //[]

        // Set design limits!!!!
		ms_des_limits.m_UA_net_power_ratio_max = 2.0;		//[-/K]
		ms_des_limits.m_UA_net_power_ratio_min = 1.E-5;		//[-/K]

		// Set minimum main compressor inlet temperature
		CO2_info s_co2_info;

		get_CO2_info(&s_co2_info);

		ms_des_limits.m_T_mc_in_min = ceil(s_co2_info.T_critical);		//[K]
	}

    S_od_deltaP ms_od_deltaP;

	const S_design_solved * get_design_solved()
	{
		return &ms_des_solved;
	}

	virtual int auto_opt_design(S_auto_opt_design_parameters & auto_opt_des_par_in) = 0;
	
	virtual int auto_opt_design_hit_eta(S_auto_opt_design_hit_eta_parameters & auto_opt_des_hit_eta_in, std::string & error_msg) = 0;

	const S_od_solved * get_od_solved()
	{
		return &ms_od_solved;
	}

	virtual int off_design_fix_shaft_speeds(S_od_par & od_phi_par_in, double od_tol) = 0;

	virtual int solve_OD_all_coolers_fan_power(double T_amb /*K*/, double od_tol /*-*/, double & W_dot_fan /*MWe*/) = 0;

    virtual int solve_OD_mc_cooler_fan_power(double T_amb /*K*/, double od_tol /*-*/, double & W_dot_mc_cooler_fan /*MWe*/, double & P_co2_out /*kPa*/) = 0;

    virtual int solve_OD_pc_cooler_fan_power(double T_amb /*K*/, double od_tol /*-*/, double & W_dot_pc_cooler_fan /*MWe*/, double & P_co2_out /*kPa*/) = 0;

	virtual const C_comp_multi_stage::S_od_solved * get_rc_od_solved() = 0;

    virtual void check_od_solution(double & diff_m_dot, double & diff_E_cycle,
        double & diff_Q_LTR, double & diff_Q_HTR) = 0;

	const S_design_limits & get_design_limits()
	{
		return ms_des_limits;
	}
};


#endif // !__SCO2_CYCLE_TEMPLATES_
