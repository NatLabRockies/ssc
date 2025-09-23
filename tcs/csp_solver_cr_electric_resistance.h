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


#ifndef  __csp_solver_cr_electric_resistance_
#define  __csp_solver_cr_electric_resistance_

#include "csp_solver_core.h"

#include "htf_props.h"

class C_csp_cr_electric_resistance : public C_csp_collector_receiver
{
public:

    enum E_elec_resist_startup_mode
    {
        SEQUENCED = 0,
        INSTANTANEOUS_NO_MAX_ELEC_IN
    };

private:

    // Defined in constructor
    double m_T_htf_cold_des;        //[C]
    double m_T_htf_hot_des;         //[C]
    double m_q_dot_heater_des;      //[MWt]
    double m_q_dot_min;             //[MWt] min allowable heater output

    double m_f_q_dot_des_allowable_su;//[-] fraction of design thermal power allowed for startup
    double m_hrs_startup_at_max_rate; //[hr]

    int m_htf_code;
    util::matrix_t<double> m_ud_htf_props;

    E_elec_resist_startup_mode m_startup_mode;

    // Hardcoded in constructor

        // Heater electric-to-heat efficiency just set up to pass to getter
        // Not threaded through to design or performance call
    double m_heater_efficiency;     //[-]

    // ********************************
    // ********************************


    // ********************************
    // Calculated system design parameters
    HTFProperties mc_pc_htfProps;
    double m_m_dot_htf_des;         //[kg/s]
    double m_dP_htf;                //[bar]
    double m_cp_htf_des;            //[kJ/kg-K]
    double m_q_dot_su_max;          //[MWt]
    double m_E_su_des;              //[MWt-hr]
    double m_t_su_des;              //[hr]
    double m_W_dot_heater_des;      //[MWe]

    double m_W_dot_pump_des;        //[MWe]

    // ********************************
    // ********************************

    // Timestep state variables
    C_csp_collector_receiver::E_csp_cr_modes m_operating_mode_converged;
    C_csp_collector_receiver::E_csp_cr_modes m_operating_mode;

    double m_E_su_initial;      //[MWt-hr] Startup energy at beginning of timestep
    double m_E_su_calculated;   //[MWt-hr] Startup energy at end of timestep

public:
    
    enum
    {
        E_W_DOT_HEATER,     //[MWe] Electricity consumed by heater
        E_Q_DOT_HTF,        //[MWt] Heat transferred to HTF
        E_Q_DOT_STARTUP,    //[MWt] Heat consumed during startup
        E_M_DOT_HTF,        //[kg/s] HTF mass flow rate
        E_T_HTF_IN,         //[C] HTF inlet temperature
        E_T_HTF_OUT         //[C] HTF outlet temperature
    };

    C_csp_reported_outputs mc_reported_outputs;

    C_csp_cr_electric_resistance(double T_htf_cold_des /*C*/, double T_htf_hot_des /*C*/,
        double q_dot_heater_des /*MWt*/, double heater_efficiency /*-*/, double f_q_dot_min /*- rel design*/,
        double f_q_dot_des_allowable_su /*-*/, double hrs_startup_at_max_rate /*hr*/,
        int htf_code /*-*/, util::matrix_t<double> ud_htf_props, E_elec_resist_startup_mode startup_mode);

    ~C_csp_cr_electric_resistance();

    // Inherited methods
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

    void get_design_parameters(double& E_su_design /*MWt-hr*/,
        double& W_dot_heater_design /*MWe*/);

    // ***************************************************************
    // ***************************************************************
    // ***************************************************************


};

#endif // ! __csp_solver_cr_electric_resistance_

