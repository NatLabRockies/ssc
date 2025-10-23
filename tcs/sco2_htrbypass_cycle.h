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

#ifndef __SCO2_HTRBYPASS_
#define __SCO2_HTRBYPASS_

#include "sco2_cycle_components.h"
#include "sco2_cycle_templates.h"

#include "heat_exchangers.h"
#include "CO2_properties.h"

#include "numeric_solvers.h"


#include "nlopt.hpp"
#include "nlopt_callbacks.h"



// This class is purely for solving the cycle
// No optimization
class C_sco2_htrbp_core
{
public:

    // Defines sco2 htr bypass input variables (no optimized variables)
    struct S_sco2_htrbp_in
    {

        double m_P_mc_in;					//[kPa] Compressor inlet pressure
        double m_P_mc_out;					//[kPa] Compressor outlet pressure

        // LTR thermal design
        int m_LTR_target_code;              //[-] 1 = UA, 2 = min dT, 3 = effectiveness
        double m_LTR_UA;					//[kW/K] target LTR conductance
        double m_LTR_min_dT;                //[K] target LTR minimum temperature difference
        double m_LTR_eff_target;            //[-] target LTR effectiveness
        double m_LTR_eff_max;				//[-] Maximum allowable effectiveness in LT recuperator
        NS_HX_counterflow_eqs::E_UA_target_type m_LTR_od_UA_target_type;
        int m_LTR_N_sub_hxrs;               //[-] Number of sub-hxs to use in hx model

        // HTR thermal design
        int m_HTR_target_code;              //[-] 1 = UA, 2 = min dT, 3 = effectiveness
        double m_HTR_UA;					//[kW/K] target HTR conductance
        double m_HTR_min_dT;                //[K] target HTR min temperature difference
        double m_HTR_eff_target;            //[-] target HTR effectiveness
        double m_HTR_eff_max;				//[-] Maximum allowable effectiveness in HT recuperator
        NS_HX_counterflow_eqs::E_UA_target_type m_HTR_od_UA_target_type;
        int m_HTR_N_sub_hxrs;               //[-] Number of sub-hxs to use in hx model

        double m_recomp_frac;				//[-] Fraction of flow that bypasses the precooler and the main compressor at the design point
        double m_bypass_frac;                   //[-] Fraction of flow that bypasses the HTR and passes through the Bypass HX 
        double m_des_tol;						//[-] Convergence tolerance

        // Air cooler parameters
        bool m_is_des_air_cooler;		//[-] False will skip physical air cooler design. UA will not be available for cost models.


        // Added
        double m_W_dot_net_design;                  //[kWe] Target net cycle power
        double m_T_mc_in;                           //[K] Compressor inlet temperature
        double m_T_t_in;			                //[K] Turbine inlet temperature
        double m_dT_BP;                             //[delta K/C] BYPASS_OUT - HTR_HP_OUT
        std::vector<double> m_DP_LTR;		        //(cold, hot) positive values are absolute [kPa], negative values are relative (-)
        std::vector<double> m_DP_HTR;		        //(cold, hot) positive values are absolute [kPa], negative values are relative (-)
        std::vector<double> m_DP_PC_main;	        //(cold, hot) positive values are absolute [kPa], negative values are relative (-)
        std::vector<double> m_DP_PHX;		        //(cold, hot) positive values are absolute [kPa], negative values are relative (-)
        double m_eta_mc;			                //[-] design-point efficiency of the main compressor; isentropic if positive, polytropic if negative
        double m_eta_t;				                //[-] design-point efficiency of the turbine; isentropic if positive, polytropic if negative
        double m_eta_rc;		                    //[-] design-point efficiency of the recompressor; isentropic if positive, polytropic if negative
        double m_eta_generator;                     //[-] Mechanical-to-electrical efficiency of generator
        double m_frac_fan_power;                    //[-] Fraction of total cycle power 'S_des_par_cycle_dep.m_W_dot_fan_des' consumed by air fan
        double m_set_HTF_mdot;                      //[kg/s] > 0 set HTF mass flow, <= 0 use bypass approach temp to calculate HTF mdot
        double m_cp_HTF;                            //[kJ/kg K] HTF specific heat
        double m_T_HTF_PHX_inlet;                   //[K] HTF Inlet Temperature
        double m_eta_fan;                           //[-] Fan isentropic efficiency
        double m_deltaP_cooler_frac;                //[-] Fraction of high side (of cycle, i.e. comp outlet) pressure that is allowed as pressure drop to design the ACC
        double m_T_amb_des;		                    //[K] Design point ambient temperature
        double m_elevation;			                //[m] Elevation (used to calculate ambient pressure)
        int m_N_nodes_pass;                         //[-] Number of nodes per pass
        double m_HTF_PHX_cold_approach_input;       //[delta K] PHX cold approach temperature. Only needed if m_set_HTF_mdot < 0 
        int m_mc_comp_model_code;                   // Main compressor model code
        int m_rc_comp_model_code;                   // Recompressor model code
        int m_N_turbine;                            //[rpm] Turbine rpm
        double m_yr_inflation;                      //[yr] Inflation target year

        S_sco2_htrbp_in()
        {
            m_P_mc_in = m_P_mc_out =
                m_LTR_UA = m_LTR_min_dT = m_LTR_eff_target = m_LTR_eff_max =
                m_HTR_UA = m_HTR_min_dT = m_HTR_eff_target = m_HTR_eff_max =
                m_recomp_frac =
                m_bypass_frac =
                m_des_tol =
                m_W_dot_net_design =
                m_T_mc_in =
                m_T_t_in =
                m_eta_mc =
                m_eta_t =
                m_eta_rc =
                m_eta_generator =
                m_frac_fan_power =
                m_set_HTF_mdot =
                m_cp_HTF =
                m_T_HTF_PHX_inlet =
                m_eta_fan =
                m_deltaP_cooler_frac =
                m_T_amb_des =
                m_elevation =
                m_dT_BP =
                m_HTF_PHX_cold_approach_input =
                m_yr_inflation = 
                std::numeric_limits<double>::quiet_NaN();

            m_N_nodes_pass = 0;
            m_LTR_N_sub_hxrs = 0;
            m_HTR_N_sub_hxrs = 0;
            m_mc_comp_model_code = -1;
            m_rc_comp_model_code = -1;
            m_N_turbine = -1;

            // Recuperator design target codes
            m_LTR_target_code = 1;      // default to target conductance
            m_LTR_od_UA_target_type = NS_HX_counterflow_eqs::E_UA_target_type::E_calc_UA;
            m_HTR_target_code = 1;      // default to target conductance
            m_HTR_od_UA_target_type = NS_HX_counterflow_eqs::E_UA_target_type::E_calc_UA;


            // Air cooler default
            m_is_des_air_cooler = true;

        }

    };

    // Defines sco2 htr bypass output variables (no optimized variables)
    struct S_sco2_htrbp_out
    {
        int m_error_code;
        C_turbine m_t;                          // Turbine model
        C_comp_multi_stage m_mc_ms;             // Main Compressor Model
        C_comp_multi_stage m_rc_ms;             // Recompressor Model
        C_HeatExchanger m_PHX, m_PC, m_BPX;     // Primary, Cooler, Bypass Heat Exchanger Models
        C_HX_co2_to_co2_CRM mc_LT_recup;        // LTR
        C_HX_co2_to_co2_CRM mc_HT_recup;        // HTR
        C_CO2_to_air_cooler mc_air_cooler;      // Air Cooler
        std::vector<double> m_temp, m_pres, m_enth, m_entr, m_dens;		// thermodynamic states (K, kPa, kJ/kg, kJ/kg-K, kg/m3)
        double m_w_t, m_w_mc, m_w_rc;                       // [kJ/kg] specific work of turbine, main compressor, recompressor
        double m_m_dot_t, m_m_dot_mc, m_m_dot_rc;           // [kg/s] sco2 Mass flow in main compressor, recompressor, turbine
        double m_m_dot_bp, m_m_dot_htr_hp;                  // [kg/s] sco2 Mass flow through bypass, hot side HTR
        double m_Q_dot_LT, m_Q_dot_HT;                      // [kWt]  Heat Transfer in LTR, HTR
        double m_W_dot_mc, m_W_dot_rc, m_W_dot_t;           // [kWt] Energy consumed by main compressor, recompressor, produced by turbine
        double m_W_dot_net;                                 // [kWt] ACTUAL produced net work in system
        double m_W_dot_air_cooler;                          // [kWe] Energy consumed by air cooler
        double m_Q_dot_air_cooler;                          // [kWt] Heat rejected by air cooler
        double m_Q_dot_LTR_LP, m_Q_dot_LTR_HP, m_Q_dot_HTR_LP, m_Q_dot_HTR_HP;  // kWt Heat change on LTR low pressure, etc...
        double m_Q_dot_total;                               // [kWt] Total heat entering sco2
        double m_Q_dot_PHX, m_Q_dot_BP;                     // [kWt] Energy exchange in PHX, BPX
        double m_m_dot_HTF;                                 // [kg/s] HTF mass flow rate
        double m_T_HTF_PHX_out;                             // [K] HTF PHX outlet temperature
        double m_HTF_PHX_cold_approach;                     // [delta K/C] PHX cold approach temperature
        double m_T_HTF_BP_outlet;                           // [K] HTF BPX outlet temperature
        double m_HTF_BP_cold_approach;                      // [K] BPX cold approach temperature
        double m_eta_thermal;                               // Thermal Efficiency

        S_sco2_htrbp_out()
        {
            Init();
        }

        void Init()
        {
            m_w_t = m_w_mc = m_w_rc
                = m_m_dot_t = m_m_dot_mc = m_m_dot_rc
                = m_m_dot_bp = m_m_dot_htr_hp
                = m_Q_dot_LT = m_Q_dot_HT
                = m_W_dot_mc = m_W_dot_rc = m_W_dot_t
                = m_W_dot_net = m_W_dot_air_cooler = m_Q_dot_air_cooler
                = m_Q_dot_LTR_LP = m_Q_dot_LTR_HP = m_Q_dot_HTR_LP = m_Q_dot_HTR_HP
                = m_Q_dot_total = m_Q_dot_PHX = m_Q_dot_BP
                = m_m_dot_HTF = m_T_HTF_PHX_out = m_HTF_PHX_cold_approach
                = m_T_HTF_BP_outlet = m_HTF_BP_cold_approach = m_eta_thermal
                = std::numeric_limits<double>::quiet_NaN();

            m_error_code = -1;

            // Clear and Size Output Vectors
            m_temp.resize(C_sco2_cycle_core::END_SCO2_STATES);
            std::fill(m_temp.begin(), m_temp.end(), std::numeric_limits<double>::quiet_NaN());
            m_pres = m_enth = m_entr = m_dens = m_temp;
        }
    };


private:
    CO2_state m_co2_props;

    class C_mono_htrbp_core_HTR_des : public C_monotonic_equation
    {
    private:
        C_sco2_htrbp_core* m_htr_bypass_cycle;

    public:
        C_mono_htrbp_core_HTR_des(C_sco2_htrbp_core* htr_bypass_cycle)
        {
            m_htr_bypass_cycle = htr_bypass_cycle;
        }

        virtual int operator()(double T_HTR_LP_OUT_guess /*K*/, double* diff_T_HTR_LP_out /*K*/)
        {
            return m_htr_bypass_cycle->solve_HTR(T_HTR_LP_OUT_guess, diff_T_HTR_LP_out);
        };
    };

    class C_mono_htrbp_core_LTR_des : public C_monotonic_equation
    {
    private:
        C_sco2_htrbp_core* m_htr_bypass_cycle;

    public:
        C_mono_htrbp_core_LTR_des(C_sco2_htrbp_core* htr_bypass_cycle)
        {
            m_htr_bypass_cycle = htr_bypass_cycle;
        }

        virtual int operator()(double T_LTR_LP_OUT_guess /*K*/, double* diff_T_LTR_LP_out /*K*/)
        {
            return m_htr_bypass_cycle->solve_LTR(T_LTR_LP_OUT_guess, diff_T_LTR_LP_out);
        };
    };

    int solve_HTR(double T_HTR_LP_OUT_guess, double* diff_T_HTR_LP_out);
    int solve_LTR(double T_LTR_LP_OUT_guess, double* diff_T_LTR_LP_out);

    void initialize_solve();

public:
    // Inputs Struct
    S_sco2_htrbp_in m_inputs;

    // Outputs Struct
    S_sco2_htrbp_out m_outputs;

    // Public Methods
    C_sco2_htrbp_core()
    {
        m_outputs.Init();
        m_co2_props = CO2_state();
    }

    void set_inputs(S_sco2_htrbp_in inputs) { m_inputs = inputs; };

    int solve();

    int finalize_design(C_sco2_cycle_core::S_design_solved& design_solved);

    void reset();
};



class C_HTRBypass_Cycle : public C_sco2_cycle_core
{
public:

    // Struct to store optimization variables
    struct S_opt_design_parameters
    {

        double m_P_mc_out_guess;			//[kPa] Initial guess for main compressor outlet pressure
        bool m_fixed_P_mc_out;				//[-] if true, P_mc_out is fixed at P_mc_out_guess

        double m_PR_HP_to_LP_guess;         //[-] Initial guess for ratio of P_mc_out to P_LP_in
        bool m_fixed_PR_HP_to_LP;					//[-] if true, ratio of P_mc_out to P_mc_in is fixed at PR_mc_guess

        double m_recomp_frac_guess;			//[-] Initial guess for design-point recompression fraction
        bool m_fixed_recomp_frac;			//[-] if true, recomp_frac is fixed at recomp_frac_guess

        double m_bypass_frac_guess;			//[-] Initial guess for design-point bypass fraction
        bool m_fixed_bypass_frac;           //[-] if true, bypass_frac is fixed at bypass_frac_guess

        double m_LT_frac_guess;				//[-] Initial guess for fraction of UA_rec_total that is in the low-temperature recuperator
        bool m_fixed_LT_frac;				//[-] if true, LT_frac is fixed at LT_frac_guess

        // ADDED
        int m_design_method;                //[] Design Method [1] Optimize total UA for target eta, [2] Optimize UA split ratio, [3] set LTR HTR directly
        double m_eta_thermal_target;        //[] Cycle thermal efficiency target (used by total UA optimization)
        double m_UA_recup_total_max;        //[kW/K] Maximum recuperator conductance (for total UA optimizer)
        double m_UA_recup_total_min;        //[kW/K] Minimum recuperator conductance (for total UA optimizer)


        S_opt_design_parameters()
        {
            m_P_mc_out_guess = m_PR_HP_to_LP_guess = m_recomp_frac_guess = m_LT_frac_guess =
                m_bypass_frac_guess = m_eta_thermal_target =
                m_UA_recup_total_max = m_UA_recup_total_min =
                std::numeric_limits<double>::quiet_NaN();


            m_design_method = -1;
            m_fixed_recomp_frac = false;
            m_fixed_bypass_frac = false;
            m_fixed_LT_frac = false;
            m_fixed_PR_HP_to_LP = false;
            m_fixed_P_mc_out = false;
            
        }


    };

private:

    // Optimal inputs, for bypass optimizer DO NOT USE
    C_sco2_htrbp_core::S_sco2_htrbp_in m_optimal_inputs_internal_only;
    double m_opt_obj_internal_only;

    int m_opt_iteration_count;          // Counter of bypass iterations

    // Bypass Specific HTF variables
    int m_T_target_is_HTF;              // Target Temperature is HTF (1) or cold sco2 at BP
    double m_T_target;                  // [K] Target temperature (either HTF or sco2)
    double m_T_HTF_PHX_inlet;           // [K] HTF Primary Heat Exchanger Inlet Temperature
    double m_set_HTF_mdot;              // [kg/s] [0] calculate HTF mdot (need to set dT_PHX_cold_approach) [>0] mdot
    double m_HTF_PHX_cold_approach;     // [K] PHX cold approach temperature (need if m_set_HTF_mdot == 0)
    double m_dT_BP;                     // [K] Temperature difference at second mixer inlet
    double m_cp_HTF;                    // [kJ/kg K] HTF specific heat
    bool m_is_bp_par_set;               // Are bp parameters set

    // Optimal htrbp core class (contains all results and component data)
    C_sco2_htrbp_core m_optimal_htrbp_core;

    void auto_opt_design_core(int& error_code);

    void auto_opt_design_hit_eta_core(int& error_code, const double eta_thermal_target);

    int optimize_totalUA(const S_auto_opt_design_parameters& auto_par, const S_opt_design_parameters& opt_par, C_sco2_htrbp_core::S_sco2_htrbp_in& optimal_inputs);

    int optimize_bp(const S_auto_opt_design_parameters& auto_par, const S_opt_design_parameters& opt_par, C_sco2_htrbp_core::S_sco2_htrbp_in& optimal_inputs);

    int optimize_nonbp(const S_auto_opt_design_parameters& auto_par, const S_opt_design_parameters& opt_par, C_sco2_htrbp_core::S_sco2_htrbp_in& core_inputs, C_sco2_htrbp_core::S_sco2_htrbp_in& optimal_inputs);

    int x_to_inputs(const std::vector<double>& x, const S_auto_opt_design_parameters auto_par, const S_opt_design_parameters opt_par, C_sco2_htrbp_core::S_sco2_htrbp_in& core_inputs);

    int clear_x_inputs(const std::vector<double>& x, const S_auto_opt_design_parameters auto_par, const S_opt_design_parameters opt_par, C_sco2_htrbp_core::S_sco2_htrbp_in& core_inputs);

    double calc_T_penalty(double target, double calc, double span);

    double calc_objective(const S_auto_opt_design_parameters& auto_par, const S_opt_design_parameters& opt_par, const C_sco2_htrbp_core& htrbp_core);

protected:
    

public:

    C_HTRBypass_Cycle(C_sco2_cycle_core::E_turbo_gen_motor_config turbo_gen_motor_config,
        double eta_generator,
        double T_mc_in,
        double W_dot_net,
        double T_t_in, double P_high_limit,
        std::vector<double> DP_LTR, std::vector<double> DP_HTR,
        std::vector<double> DP_PC_main, std::vector<double> DP_PHX,
        int LTR_N_sub_hxrs, int HTR_N_sub_hxrs,
        double eta_mc, int mc_comp_model_code,
        double eta_rc,
        double eta_t, double N_turbine,
        double frac_fan_power, double eta_fan, double deltaP_cooler_frac,
        int N_nodes_pass,
        double T_amb_des, double elevation,
        double yr_inflation) :
        C_sco2_cycle_core(turbo_gen_motor_config,
            eta_generator,
            T_mc_in,
            W_dot_net,
            T_t_in, P_high_limit,
            DP_LTR, DP_HTR,
            DP_PC_main, DP_PHX,
            LTR_N_sub_hxrs, HTR_N_sub_hxrs,
            eta_mc, mc_comp_model_code,
            eta_rc,
            eta_t, N_turbine,
            frac_fan_power, eta_fan, deltaP_cooler_frac,
            N_nodes_pass,
            T_amb_des, elevation,
            yr_inflation)
    {
        m_T_target = m_T_HTF_PHX_inlet = m_set_HTF_mdot
            = m_HTF_PHX_cold_approach = m_dT_BP
            = m_cp_HTF = m_opt_obj_internal_only
            = std::numeric_limits<double>::quiet_NaN();

        m_T_target_is_HTF = -1;
        m_opt_iteration_count = 0;

        m_is_bp_par_set = false;
    }

    ~C_HTRBypass_Cycle() {};

    // Set Bypass Specific Parameters
    void set_bp_par(double T_htf_phx_in, double T_target, double cp_htf, double dT_bp,
        double htf_phx_cold_approach, double set_HTF_mdot, int T_target_is_HTF)
    {
        m_T_HTF_PHX_inlet = T_htf_phx_in;  // K
        m_T_target = T_target; // K
        m_T_target_is_HTF = T_target_is_HTF;
        m_cp_HTF = cp_htf;  // kJ/kg K
        m_dT_BP = dT_bp;
        m_HTF_PHX_cold_approach = htf_phx_cold_approach;
        m_set_HTF_mdot = set_HTF_mdot;

        m_is_bp_par_set = true;
    }

    // Overridden - Optimize Cycle (fixed total UA)
    int auto_opt_design(S_auto_opt_design_parameters& auto_opt_des_par_in);

    // Overridden - Optimize Cycle for target eta (variable total UA)
    int auto_opt_design_hit_eta(S_auto_opt_design_hit_eta_parameters& auto_opt_des_hit_eta_in, std::string& error_msg);

    // Objective Functions (internal use only)
    double optimize_totalUA_return_objective_metric(const std::vector<double>& x, const S_auto_opt_design_parameters& auto_par, const S_opt_design_parameters& opt_par);
    double optimize_bp_return_objective_metric(const std::vector<double>& x, const S_auto_opt_design_parameters& auto_par, const S_opt_design_parameters& opt_par, C_sco2_htrbp_core& htrbp_core);
    double optimize_nonbp_return_objective_metric(const std::vector<double>& x, const S_auto_opt_design_parameters& auto_par, const S_opt_design_parameters& opt_par, C_sco2_htrbp_core& htrbp_core);

    // Off Design

    void reset_ms_od_turbo_bal_csp_solved();

    int off_design_fix_shaft_speeds(S_od_par& od_phi_par_in, double od_tol /*-*/);

    virtual int solve_OD_all_coolers_fan_power(double T_amb /*K*/, double od_tol /*-*/, double& W_dot_fan /*MWe*/);

    virtual int solve_OD_mc_cooler_fan_power(double T_amb /*K*/, double od_tol /*-*/, double& W_dot_mc_cooler_fan /*MWe*/, double& P_co2_out /*kPa*/);

    virtual int solve_OD_pc_cooler_fan_power(double T_amb /*K*/, double od_tol /*-*/, double& W_dot_pc_cooler_fan /*MWe*/, double& P_co2_out /*kPa*/);

    double get_od_temp(int n_state_point);

    double get_od_pres(int n_state_point);

    virtual void check_od_solution(double& diff_m_dot, double& diff_E_cycle,
        double& diff_Q_LTR, double& diff_Q_HTR);

    void set_od_temp(int n_state_point, double temp_K);

    void set_od_pres(int n_state_point, double pres_kPa);

    void off_design_recompressor(double T_in, double P_in, double m_dot, double P_out, double tol /*-*/, int& error_code, double& T_out);

    void estimate_od_turbo_operation(double T_mc_in /*K*/, double P_mc_in /*kPa*/, double f_recomp /*-*/, double T_t_in /*K*/, double phi_mc /*-*/,
        int& mc_error_code, double& mc_w_tip_ratio /*-*/, double& P_mc_out /*kPa*/,
        int& rc_error_code, double& rc_w_tip_ratio /*-*/, double& rc_phi /*-*/,
        bool is_update_ms_od_solved = false);

    const C_comp_multi_stage::S_od_solved* get_rc_od_solved()
    {
        return m_optimal_htrbp_core.m_outputs.m_rc_ms.get_od_solved();
        //return m_rc_ms.get_od_solved();
    }

    /*const S_od_turbo_bal_csp_solved* get_od_turbo_bal_csp_solved()
    {
        return &ms_od_turbo_bal_csp_solved;
    }

    double get_max_target()
    {
        return m_biggest_target;
    }*/



};

// Nlopt objective functions
double nlopt_optimize_totalUA_func(const std::vector<double>& x, std::vector<double>& grad, void* data);

double nlopt_optimize_bp_func(const std::vector<double>& x, std::vector<double>& grad, void* data);

double nlopt_optimize_nonbp_func(const std::vector<double>& x, std::vector<double>& grad, void* data);




// Penalty value methods
double sigmoid(const double val);

double logit(const double val);

#endif
