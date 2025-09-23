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


#ifndef __csp_solver_pc_ptes_
#define __csp_solver_pc_ptes_

#include "csp_solver_core.h"

#include "htf_props.h"


namespace pc_ptes_helpers
{
    void design_calcs__all(double W_dot_thermo /*MWe*/, double f_elec_consume_vs_gen /*-*/,
        double eta_therm_mech /*-*/, double fixed__q_dot_cold__to__q_dot_warm /*-*/,
        double& W_dot_net /*MWe*/, double& W_dot_consume_elec /*MWe*/,
        double& q_dot_hot_in /*MWt*/, double& q_dot_cold_out_thermo /*MWt*/, double& eta_net /*-*/,
        double& q_dot_cold_to_CTES, double& q_dot_reject_to_surroundings /*MWt*/);

    void design_calcs__no_ctes(double W_dot_thermo /*MWe*/, double f_elec_consume_vs_gen /*-*/,
        double eta_therm_mech /*-*/,
        double& W_dot_net /*MWe*/, double& W_dot_consume_elec /*MWe*/,
        double& q_dot_hot_in /*MWt*/, double& q_dot_cold_out_thermo /*MWt*/, double& eta_net /*-*/);

    void design_calcs__q_dot_ctes(double q_dot_hot_in /*MWt*/, double fixed__q_dot_cold__to__q_dot_warm /*-*/,
        double q_dot_cold_out_thermo /*MWt*/,
        double& q_dot_cold_to_CTES, double& q_dot_reject_to_surroundings /*MWt*/);

    class C_endo_reversible_cycle
    {
    private:

        double m_T_HT_hot_des;      //[C]
        double m_T_HT_cold_des;     //[C]
        double m_T_CT_hot_des;      //[C]
        double m_T_CT_cold_des;     //[C]
        double m_eta_endo_des;      //[-]

    public:

        C_endo_reversible_cycle(double T_HT_hot_des /*C*/, double T_HT_cold_des /*C*/,
            double T_CT_hot_des /*C*/, double T_CT_cold_des /*C*/);

        double eta_endo(double T_HT_hot /*C*/, double T_HT_cold /*C*/,
            double T_CT_hot /*C*/, double T_CT_cold /*C*/);

        void performance(double T_HT_hot /*C*/, double m_dot_HT_ND /*-*/, double T_CT_cold /*C*/,
            double& W_dot_gross_ND /*-*/, double& Q_dot_ND /*-*/,
            double& T_HT_cold /*C*/, double& T_CT_hot /*C*/);

    };
}

class C_pc_ptes : public C_csp_power_cycle
{
private:

    // Defined in constructor
    double m_W_dot_thermo_des;          //[MWe]
    double m_eta_therm_mech_des;        //[-]
    double m_f_elec_consume_vs_gen;     //[-] Fraction of thermo generation that cycle uses for parasitics (motors, generators, cooling)
    double m_fixed__q_dot_cold__to__q_dot_warm /*-*/;      //[-]
    double m_T_HT_HTF_hot_des;          //[C]
    double m_T_HT_HTF_cold_des;         //[C]
    double m_T_CT_HTF_cold_des;         //[C]
    double m_T_CT_HTF_hot_des;          //[C]
    double m_cycle_max_frac_des;        //[-]
    double m_cycle_cutoff_frac_des;     //[-]
    double m_q_sby_frac_des;            //[-]
    double m_startup_time_des;          //[hr]
    double m_startup_frac_des;          //[-]
    double m_HT_htf_pump_coef_des;      //[kW/kg/s]
    double m_CT_htf_pump_coef_des;      //[kW/kg/s]

    int m_HT_htf_code;
    util::matrix_t<double> m_HT_ud_htf_props;

    int m_CT_htf_code;
    util::matrix_t<double> m_CT_ud_htf_props;
    // *******************************************
    // *******************************************


    // *******************************************
    // *******************************************
    // Calculated system design parameters
    double m_W_dot_net_des;             //[MWe]
    double m_q_dot_hot_in_des;          //[MWt]
    double m_q_dot_cold_out_thermo_des; //[MWt]
    double m_W_dot_elec_parasitic_des;  //[MWe]
    double m_eta_overall_des;           //[-]
    double m_q_dot_cold_to_CTES;        //[MWt]
    double m_q_dot_cold_to_surroundings;    //[MWt]

    double m_T_HT_HTF_avg_des;          //[C]
    double m_cp_HT_HTF_des;             //[kJ/kg-K]
    double m_T_CT_HTF_avg_des;          //[C]
    double m_cp_CT_HTF_des;             //[kJ/kg-K]

    double m_m_dot_HT_des;              //[kg/s]
    double m_m_dot_HT_min;              //[kg/s]
    double m_m_dot_HT_max;              //[kg/s]
    double m_W_dot_HT_htf_pump_des;     //[MWe]

    double m_q_dot_HT_max;              //[MWt]
    double m_q_dot_HT_min;              //[MWt]

    double m_m_dot_CT_des;              //[kg/s]
    double m_W_dot_CT_htf_pump_des;     //[MWe]

    double m_m_dot_CT_to_HT_ratio;      //[-]

    double m_E_su_des;                  //[MWt-hr]
    double m_E_standby_des;             //[MWt-hr]

    // Member points/classes
    std::unique_ptr<HTFProperties> m_HT_htfProps;
    std::unique_ptr<HTFProperties> m_CT_htfProps;

    std::unique_ptr<pc_ptes_helpers::C_endo_reversible_cycle> mp_endo_reverse;
    // *******************************************
    // *******************************************


    // *******************************************
    // *******************************************
    // State variables
    C_csp_power_cycle::E_csp_power_cycle_modes m_operating_mode_prev;
    double m_startup_time_remain_prev;		//[hr]
    double m_startup_energy_remain_prev;	//[MWt-hr]

    C_csp_power_cycle::E_csp_power_cycle_modes m_operating_mode_calc;
    double m_startup_time_remain_calc;      //[hr]
    double m_startup_energy_remain_calc;    //[MWt-hr]

public:

    enum
    {
        E_T_HT_HTF_HOT_IN,      //[C] HT HTF hot inlet temperature
        E_T_HT_HTF_COLD_OUT,    //[C] HT HTF cold outlet temperature
        E_T_CT_HTF_COLD_IN,     //[C] Cold HTF cold inlet temperature
        E_T_CT_HTF_HOT_OUT,     //[C] Cold HTF hot outlet temperature
        E_M_DOT_HT_HTF,         //[kg/s] HT HTF mass flow rate
        E_M_DOT_CT_HTF,         //[kg/s] CT HTF mass flow rate
        E_Q_DOT_STARTUP,        //[MWt] Heat consumed during startup
        E_Q_DOT_HOT_IN,         //[MWt] Heat from HT HTF
        E_Q_DOT_THERMO_OUT_TOTAL,   //[MWt] Total heat leaving the thermodynamic cycle
        E_Q_DOT_TO_COLD_TES,    //[MWt] Heat to CT HTF
        E_Q_DOT_REJECTED,       //[MWt] Heat rejected to ambient
        E_W_DOT_THERMO,         //[MWe] Cycle thermodynamic output (not including motor/generator losses, cooling parasitics, or HTF pumps
        E_W_DOT_CYCLE_PARASITICS,   //[MWe] Thermo parasitics (e.g. *cooling power*, motors, generator losses)
        E_W_DOT_HT_HTF_PUMP,    //[MWe] HT HTF pump
        E_W_DOT_CT_HTF_PUMP,     //[MWe] CT HTF pump

        // Dependent output variables
        E_ETA_THERMAL,		//[-] Cycle thermal efficiency (gross)
    };

    C_csp_reported_outputs mc_reported_outputs;

    // ***********************
    // Inherited methods
    // ***********************
    C_pc_ptes(double W_dot_gen_thermo /*MWe*/, double eta_therm_mech /*-*/,
        double f_elec_consume_vs_gen /*-*/, double fixed__q_dot_cold__to__q_dot_warm /*-*/,
        double T_HT_HTF_hot /*C*/, double T_HT_HTF_cold /*C*/, double T_CT_HTF_cold /*C*/, double T_CT_HTF_hot /*C*/,
        double cycle_max_frac /*-*/, double cycle_cutoff_frac /*-*/, double q_sby_frac /*-*/,
        double startup_time /*hr*/, double startup_frac /*-*/,
        double HT_htf_pump_coef /*kW/kg/s*/, double CT_htf_pump_coef /*kW/kg/s*/,
        int HT_htf_code /*-*/, util::matrix_t<double> HT_ud_htf_props,
        int CT_htf_code /*-*/, util::matrix_t<double> CT_ud_htf_props);

    ~C_pc_ptes() {};

    virtual void init(C_csp_power_cycle::S_solved_params& solved_params);

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
    virtual void get_max_power_output_operation_constraints(double T_amb /*C*/, double& m_dot_HTF_ND_max, double& W_dot_ND_max);	//[-] Normalized over design power
    virtual double get_efficiency_at_TPH(double T_degC, double P_atm, double relhum_pct, double* w_dot_condenser = 0);
    virtual double get_efficiency_at_load(double load_frac, double* w_dot_condenser = 0);
    virtual double get_htf_pumping_parasitic_coef();		//[kWe/kWt]

    virtual double get_design_pumping_power();      //[MWe]
    virtual double get_design_cooling_power();      //[MWe]

    // This can vary between timesteps for Type224, depending on remaining startup energy and time
    virtual double get_max_q_pc_startup();		//[MWt]


    virtual void call(const C_csp_weatherreader::S_outputs& weather,
        C_csp_solver_htf_1state& htf_state_in,
        const C_csp_power_cycle::S_control_inputs& inputs,
        C_csp_power_cycle::S_csp_pc_out_solver& out_solver,
        const C_csp_solver_sim_info& sim_info);

    virtual void call(const C_csp_weatherreader::S_outputs& weather,
        C_csp_solver_htf_1state& htf_state_in,
        double T_CT_htf_cold_in /*C*/,
        const C_csp_power_cycle::S_control_inputs& inputs,
        C_csp_power_cycle::S_csp_pc_out_solver& out_solver,
        const C_csp_solver_sim_info& sim_info) override;

    virtual void converged();

    virtual void write_output_intervals(double report_time_start,
        const std::vector<double>& v_temp_ts_time_end, double report_time_end);

    virtual void assign(int index, double* p_reporting_ts_array, size_t n_reporting_ts_array);

    // ********************************************************
    // ********************************************************

    void get_design_parameters(double& W_dot_net /*MWe*/, double& q_dot_hot_in /*MWt*/,
        double& q_dot_cold_out_thermo /*MWt*/, double& W_dot_elec_parasitic /*MWe*/,
        double& eta_net /*-*/, double& q_dot_cold_to_CTES /*MWt*/, double& q_dot_cold_to_surr /*MWt*/,
        double& m_dot_HT_htf /*kg/s*/, double& cp_HT_htf /*kJ/kg-K*/, double& W_dot_HT_htf_pump /*MWe*/,
        double& m_dot_CT_htf /*kg/s*/, double& cp_CT_htf /*kJ/kg-K*/, double& W_dot_CT_htf_pump /*MWe*/);
};



#endif // !__csp_solver_pc_ptes_
