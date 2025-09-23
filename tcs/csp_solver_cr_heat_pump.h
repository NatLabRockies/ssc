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


#ifndef __csp_solver_cr_heat_pump
#define __csp_solver_cr_heat_pump

#include "csp_solver_core.h"

#include "htf_props.h"

namespace heat_pump_helpers
{
    void design_calcs(double q_dot_hot_out /*MWt*/, double COP_heat /*-*/,
        double f_elec_consume_vs_W_dot_thermo /*-*/,
        double& W_dot_in_thermo /*MWe*/, double& q_dot_cold_in /*MWt*/,
        double& W_dot_consume_elec /*MWe*/, double& W_dot_in_net /*MWe*/,
        double& COP_heat_net /*-*/);

    class C_carnot_heat_pump
    {
    private:

        double m_T_HT_hot_des;      //[C]
        double m_T_HT_cold_des;     //[C]
        double m_T_CT_hot_des;      //[C]
        double m_T_CT_cold_des;     //[C]
        double m_cop_carnot_des;    //[-]

    public:

        C_carnot_heat_pump(double T_HT_hot_des /*C*/, double T_HT_cold_des /*C*/,
            double T_CT_hot_des /*C*/, double T_CT_cold_des /*C*/);

        double cop_carnot(double T_HT_hot /*C*/, double T_HT_cold /*C*/,
            double T_CT_hot /*C*/, double T_CT_cold /*C*/);

        int performance(double T_HT_cold_in /*C*/, double m_dot_HT_ND /*-*/,
            double T_CT_hot /*C*/, double m_dot_CT_ND /*-*/,
            double& W_dot_gross_ND /*-*/, double& Q_dot_ND /*-*/,
            double& Q_dot_cold_in_ND /*-*/,
            double& T_HT_hot_out /*C*/, double& T_CT_cold /*C*/);

        void get_des_for_perf(double& T_HT_cold_des /*C*/,
            double& T_CT_hot_des /*C*/);

        double get_cop_des()
        {
            return m_cop_carnot_des;
        }
    };

    class C_MEQ__T_CT_cold : public C_monotonic_equation
    {
    private:
        //std::shared_ptr<heat_pump_helpers::C_carnot_heat_pump> mpc_carnot_heat_pump;
        heat_pump_helpers::C_carnot_heat_pump* mpc_carnot_heat_pump;

        double m_T_HT_hot;      //[C]
        double m_T_HT_cold;     //[C]
        double m_m_dot_HT_ND;   //[-]
        double m_T_CT_hot;     //[C]
        double m_m_dot_CT_ND;   //[-]
        double m_Q_dot_hot_out_ND;  //[-]

        double m_deltaT_CT_des;     //[C]
        double m_cop_des;           //[-]

    public:

        double m_W_dot_gross_ND;    //[-]
        double m_Q_dot_cold_ND;     //[-]
        double m_T_CT_cold_calc;    //[C]

        C_MEQ__T_CT_cold(heat_pump_helpers::C_carnot_heat_pump* pc_carnot_heat_pump,
            double T_HT_hot /*C*/, double T_HT_cold /*C*/, double m_dot_HT_ND /*-*/,
            double T_CT_hot /*C*/, double m_dot_CT_ND /*-*/,
            double Q_dot_hot_out_ND /*-*/,
            double deltaT_CT_des /*C*/)
        {
            mpc_carnot_heat_pump = pc_carnot_heat_pump;
            m_T_HT_hot = T_HT_hot;      //[C]
            m_T_HT_cold = T_HT_cold;    //[C]
            m_m_dot_HT_ND = m_dot_HT_ND;    //[-]
            m_T_CT_hot = T_CT_hot;          //[C]
            m_m_dot_CT_ND = m_dot_CT_ND;    //[-]
            m_Q_dot_hot_out_ND = Q_dot_hot_out_ND;  //[-]

            m_deltaT_CT_des = deltaT_CT_des;    //[C]

            m_cop_des = mpc_carnot_heat_pump->get_cop_des();    //[-]

            m_W_dot_gross_ND = std::numeric_limits<double>::quiet_NaN();
            m_Q_dot_cold_ND = std::numeric_limits<double>::quiet_NaN();
            m_T_CT_cold_calc = std::numeric_limits<double>::quiet_NaN();
        }

        virtual int operator()(double T_CT_cold /*C*/, double* diff_T_CT_cold /*C*/);
    };
}

class C_csp_cr_heat_pump : public C_csp_collector_receiver
{

private:

    // Defined in constructor
    double m_COP_heat_des;      //[-]
    double m_q_dot_hot_out_des; //[MWt]
    double m_f_elec_consume_vs_W_dot_thermo_des;    //[-]
    double m_T_HT_HTF_hot_des;      //[C] High temp HTF hot temperature
    double m_T_HT_HTF_cold_des;     //[C] High temp HTF cold temperature
    double m_T_CT_HTF_cold_des;     //[C] Cold temp HTF cold temperature
    double m_T_CT_HTF_hot_des;      //[C] Cold temp HTF hot temperature

    double m_f_q_dot_min;           //[-] min allowable heater output (fraction of des)

    double m_f_q_dot_des_allowable_su;  //[-] fraction of design thermal power allowed for startup
    double m_hrs_startup_at_max_rate;   //[hr]

    double m_heat_pump_HT_htf_pump_coef;  //[kW/kg/s]
    double m_heat_pump_CT_htf_pump_coef;  //[kW/kg/s]

    int m_HT_htf_code;
    util::matrix_t<double> m_HT_ud_htf_props;

    int m_CT_htf_code;
    util::matrix_t<double> m_CT_ud_htf_props;
    // *******************************************
    // *******************************************


    // *******************************************
    // *******************************************
    // Calculated system design parameters
    double m_W_dot_in_thermo_des;   //[MWe] power into cycle working fluid. does not consider electric parasitics (e.g. cooling fan, motor inefficiencies, etc.)
    double m_q_dot_cold_in_des;     //[MWt]
    double m_W_dot_consume_elec_des;//[MWe]
    double m_W_dot_in_net_des;      //[MWe]
    double m_COP_net_des;           //[-] q_dot_hot_out / W_dot_in_thermo

    double m_T_HT_HTF_avg_des;      //[C]
    double m_cp_HT_HTF_des;         //[kJ/kg-K]
    double m_T_CT_HTF_avg_des;      //[C]
    double m_cp_CT_HTF_des;         //[kJ/kg-K]

    double m_m_dot_HT_des;          //[kg/s]
    double m_W_dot_HT_htf_pump_des; //[MWe]

    double m_m_dot_CT_des;          //[kg/s]
    double m_W_dot_CT_htf_pump_des; //[MWe]

    double m_m_dot_CT_to_HT_ratio;  //[-]

    double m_q_dot_min_des;         //[MWt]

    double m_q_dot_su_max;          //[MWt]
    double m_W_dot_su_max;          //[MWe]
    double m_E_su_des;              //[MWt-hr]
    double m_E_W_dot_su_des;        //[MWe-hr]
    double m_t_su_des;              //[hr]

    // Member points/classes
    std::unique_ptr<HTFProperties> m_HT_htfProps;
    std::unique_ptr<HTFProperties> m_CT_htfProps;

    std::shared_ptr<heat_pump_helpers::C_carnot_heat_pump> mp_carnot_heat_pump;
    // *******************************************
    // *******************************************


    // *******************************************
    // *******************************************
    // Timestep state variables
    C_csp_collector_receiver::E_csp_cr_modes m_operating_mode_converged;
    C_csp_collector_receiver::E_csp_cr_modes m_operating_mode;

    double m_E_su_initial;      //[MWt-hr] Startup energy at beginning of timestep
    double m_E_su_calculated;   //[MWt-hr] Startup energy at end of timestep



public:

    int test_heat_pump_perf_call(double m_dot_ND,
        double& W_dot_gross_ND /*-*/, double& Q_dot_ND /*-*/,
        double& Q_dot_cold_in_ND /*-*/,
        double& T_HT_hot_out /*C*/, double& T_CT_cold /*C*/);

    enum {
        E_T_HT_HTF_IN,         //[C] HT HTF inlet temperature
        E_T_HT_HTF_OUT,        //[C] HT HTF outlet temperature
        E_T_CT_HTF_IN,         //[C] Cold HTF inlet temperature
        E_T_CT_HTF_OUT,        //[C] Cold HTF outlet temperature
        E_M_DOT_HT_HTF,     //[kg/s] HT HTF mass flow rate
        E_M_DOT_CT_HTF,     //[kg/s] CT HTF mass flow rate
        E_Q_DOT_STARTUP,    //[MWt] Heat consumed during startup
        E_Q_DOT_HOT_OUT,    //[MWt] Heat to HT HTF
        E_Q_DOT_COLD_IN,    //[MWt] Heat from CT HTF
        E_W_DOT_IN_THERMO,  //[MWe] Power to thermo cycle
        E_W_DOT_CYCLE_PARASITICS,   //[MWe] Thermo parasitics (e.g. motors, generator losses
        E_W_DOT_HT_HTF_PUMP,    //[MWe] HT HTF pump
        E_W_DOT_CT_HTF_PUMP,    //[MWe] CT HTF pump
        E_W_DOT_HEATER,          //[MWe] Total power consumed by heat pump

        // Dependent output variables
        E_COP_HOT_THERMO,		//[-] Cycle thermal efficiency (gross)
    };

    C_csp_reported_outputs mc_reported_outputs;

    C_csp_cr_heat_pump(double COP_heat_des /*-*/, double q_dot_hot_out_des /*MWt*/,
        double f_elec_consume_vs_W_dot_thermo /*-*/,
        double T_HT_HTF_hot /*C*/, double T_HT_HTF_cold /*C*/, double T_CT_HTF_cold /*C*/, double T_CT_HTF_hot /*C*/,
        double f_q_dot_min /*-*/, double f_q_dot_des_allowable_su /*-*/, double hrs_startup_at_max_rate /*hr*/,
        double heat_pump_HT_htf_pump_coef /*kW/kg/s*/, double heat_pump_LT_htf_pump_coef /*kW/kg/s*/,
        int HT_htf_code /*-*/, util::matrix_t<double> HT_ud_htf_props,
        int CT_htf_code /*-*/, util::matrix_t<double> CT_ud_htf_props);

    ~C_csp_cr_heat_pump();

    // ***********************
    // Inherited methods
    // ***********************
    virtual void init(const C_csp_collector_receiver::S_csp_cr_init_inputs init_inputs,
        C_csp_collector_receiver::S_csp_cr_solved_params& solved_params);

    virtual C_csp_collector_receiver::E_csp_cr_modes get_operating_state();

    virtual double get_startup_time();
    virtual double get_startup_energy(); //MWh
    virtual double get_pumping_parasitic_coef();  //MWe/MWt
    virtual double get_min_power_delivery();    //MWt
    virtual double get_max_power_delivery(double T_cold_in);    //MWt
    virtual double get_tracking_power();		//MWe
    virtual double get_col_startup_power();		//MWe-hr
    virtual double get_design_pumping_power();  //[MWe]

    virtual void off(const C_csp_weatherreader::S_outputs& weather,
        const C_csp_solver_htf_1state& htf_state_in,
        C_csp_collector_receiver::S_csp_cr_out_solver& cr_out_solver,
        const C_csp_solver_sim_info& sim_info);

    virtual void startup(const C_csp_weatherreader::S_outputs& weather,
        const C_csp_solver_htf_1state& htf_state_in,
        C_csp_collector_receiver::S_csp_cr_out_solver& cr_out_solver,
        const C_csp_solver_sim_info& sim_info);

    virtual void on(const C_csp_weatherreader::S_outputs& weather,
        const C_csp_solver_htf_1state& htf_state_in,
        double q_dot_elec_to_CR_heat /*MWt*/, double field_control,
        C_csp_collector_receiver::S_csp_cr_out_solver& cr_out_solver,
        const C_csp_solver_sim_info& sim_info);

    virtual void on(const C_csp_weatherreader::S_outputs& weather,
        const C_csp_solver_htf_1state& htf_state_in,
        double T_CT_htf_hot_in /*C*/,
        double q_dot_elec_to_CR_heat /*MWt*/, double field_control,
        C_csp_collector_receiver::S_csp_cr_out_solver& cr_out_solver,
        const C_csp_solver_sim_info& sim_info) override;

    virtual void estimates(const C_csp_weatherreader::S_outputs& weather,
        const C_csp_solver_htf_1state& htf_state_in,
        C_csp_collector_receiver::S_csp_cr_est_out& est_out,
        const C_csp_solver_sim_info& sim_info);

    virtual void converged();

    virtual void write_output_intervals(double report_time_start,
        const std::vector<double>& v_temp_ts_time_end, double report_time_end);

    virtual double calculate_optical_efficiency(const C_csp_weatherreader::S_outputs& weather, const C_csp_solver_sim_info& sim);

    virtual double calculate_thermal_efficiency_approx(const C_csp_weatherreader::S_outputs& weather, double q_incident /*MW*/, const C_csp_solver_sim_info& sim);

    virtual double get_collector_area();

    virtual double get_design_electric_to_heat_cop() override;

    // ***************************************************************
    // ***************************************************************

    void get_design_parameters(double& W_dot_in /*MWe*/,
            double& q_dot_cold_in /*MWt*/, double& q_dot_hot_out /*MWt*/,
            double& W_dot_elec_parasitic /*MWe*/, double& W_dot_in_net /*MWe*/,
            double& COP_net /*-*/,
            double& m_dot_HT_htf /*kg/s*/, double& cp_HT_htf /*kJ/kg-K*/, double& W_dot_HT_htf_pump /*MWe*/,
            double& m_dot_CT_htf /*kg/s*/, double& cp_CT_htf /*kJ/kg-K*/, double& W_dot_CT_htf_pump /*MWe*/,
            double& E_su /*MWt-hr*/);
};



#endif // !__csp_solver_cr_heat_pump
