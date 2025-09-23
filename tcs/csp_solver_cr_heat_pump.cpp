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


#include "csp_solver_cr_heat_pump.h"
#include "csp_solver_core.h"

static C_csp_reported_outputs::S_output_info S_cr_heat_pump_output_info[] =
{
    {C_csp_cr_heat_pump::E_T_HT_HTF_IN, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_csp_cr_heat_pump::E_T_HT_HTF_OUT, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_csp_cr_heat_pump::E_T_CT_HTF_IN, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_csp_cr_heat_pump::E_T_CT_HTF_OUT, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_csp_cr_heat_pump::E_M_DOT_HT_HTF, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_csp_cr_heat_pump::E_M_DOT_CT_HTF, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_csp_cr_heat_pump::E_Q_DOT_STARTUP, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_csp_cr_heat_pump::E_Q_DOT_HOT_OUT, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_csp_cr_heat_pump::E_Q_DOT_COLD_IN, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_csp_cr_heat_pump::E_W_DOT_IN_THERMO, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_csp_cr_heat_pump::E_W_DOT_CYCLE_PARASITICS, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_csp_cr_heat_pump::E_W_DOT_HT_HTF_PUMP, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_csp_cr_heat_pump::E_W_DOT_CT_HTF_PUMP, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_csp_cr_heat_pump::E_W_DOT_HEATER, C_csp_reported_outputs::TS_WEIGHTED_AVE},

    csp_info_invalid
};

static C_csp_reported_outputs::S_dependent_output_info S_heat_pump_dependent_output_info[] =
{
    {C_csp_cr_heat_pump::E_COP_HOT_THERMO, C_csp_cr_heat_pump::E_Q_DOT_HOT_OUT, C_csp_cr_heat_pump::E_W_DOT_IN_THERMO, C_csp_reported_outputs::AoverB},

    csp_dep_info_invalid
};

C_csp_cr_heat_pump::C_csp_cr_heat_pump(double COP_heat_des /*-*/, double q_dot_hot_out_des /*MWt*/,
    double f_elec_consume_vs_W_dot_thermo /*-*/,
    double T_HT_HTF_hot /*C*/, double T_HT_HTF_cold /*C*/, double T_CT_HTF_cold /*C*/, double T_CT_HTF_hot /*C*/,
    double f_q_dot_min /*-*/, double f_q_dot_des_allowable_su /*-*/, double hrs_startup_at_max_rate /*hr*/,
    double heat_pump_HT_htf_pump_coef /*kW/kg/s*/, double heat_pump_CT_htf_pump_coef /*kW/kg/s*/,
    int HT_htf_code /*-*/, util::matrix_t<double> HT_ud_htf_props,
    int CT_htf_code /*-*/, util::matrix_t<double> CT_ud_htf_props)
{
    // Defined in constructor
    m_COP_heat_des = COP_heat_des;              //[-]
    m_q_dot_hot_out_des = q_dot_hot_out_des;    //[MWt]
    m_f_elec_consume_vs_W_dot_thermo_des = f_elec_consume_vs_W_dot_thermo;  //[-]
    m_T_HT_HTF_hot_des = T_HT_HTF_hot;              //[C]
    m_T_HT_HTF_cold_des = T_HT_HTF_cold;            //[C]
    m_T_CT_HTF_cold_des = T_CT_HTF_cold;            //[C]
    m_T_CT_HTF_hot_des = T_CT_HTF_hot;              //[C]

    m_f_q_dot_min = f_q_dot_min;                    //[-]

    m_f_q_dot_des_allowable_su = f_q_dot_des_allowable_su;  //[-]
    m_hrs_startup_at_max_rate = hrs_startup_at_max_rate;    //[hr]

    m_heat_pump_HT_htf_pump_coef = heat_pump_HT_htf_pump_coef;  //[kW/kg/s]
    m_heat_pump_CT_htf_pump_coef = heat_pump_CT_htf_pump_coef;  //[kW/kg/s]

    m_HT_htf_code = HT_htf_code;
    m_HT_ud_htf_props = HT_ud_htf_props;

    m_CT_htf_code = CT_htf_code;
    m_CT_ud_htf_props = CT_ud_htf_props;

    // Initialize calculated member variables to nan
    m_W_dot_in_thermo_des = std::numeric_limits<double>::quiet_NaN();
    m_q_dot_cold_in_des = std::numeric_limits<double>::quiet_NaN();
    m_W_dot_consume_elec_des = std::numeric_limits<double>::quiet_NaN();
    m_W_dot_in_net_des = std::numeric_limits<double>::quiet_NaN();
    m_COP_net_des = std::numeric_limits<double>::quiet_NaN();

    m_T_HT_HTF_avg_des = std::numeric_limits<double>::quiet_NaN();
    m_cp_HT_HTF_des = std::numeric_limits<double>::quiet_NaN();
    m_T_CT_HTF_avg_des = std::numeric_limits<double>::quiet_NaN();
    m_cp_CT_HTF_des = std::numeric_limits<double>::quiet_NaN();

    m_m_dot_HT_des = std::numeric_limits<double>::quiet_NaN();
    m_W_dot_HT_htf_pump_des = std::numeric_limits<double>::quiet_NaN();

    m_m_dot_CT_des = std::numeric_limits<double>::quiet_NaN();
    m_W_dot_CT_htf_pump_des = std::numeric_limits<double>::quiet_NaN();

    m_m_dot_CT_to_HT_ratio = std::numeric_limits<double>::quiet_NaN();

    m_q_dot_min_des = std::numeric_limits<double>::quiet_NaN();

    m_q_dot_su_max = std::numeric_limits<double>::quiet_NaN();
    m_W_dot_su_max = std::numeric_limits<double>::quiet_NaN();
    m_E_su_des = std::numeric_limits<double>::quiet_NaN();
    m_E_W_dot_su_des = std::numeric_limits<double>::quiet_NaN();
    m_t_su_des = std::numeric_limits<double>::quiet_NaN();

    // Timestep state variables
    m_E_su_initial = std::numeric_limits<double>::quiet_NaN();
    m_E_su_calculated = std::numeric_limits<double>::quiet_NaN();

    mc_reported_outputs.construct(S_cr_heat_pump_output_info, S_heat_pump_dependent_output_info);
}

C_csp_cr_heat_pump::~C_csp_cr_heat_pump(){}

int C_csp_cr_heat_pump::test_heat_pump_perf_call(double m_dot_ND,
    double& W_dot_gross_ND /*-*/, double& Q_dot_ND /*-*/,
    double& Q_dot_cold_in_ND /*-*/,
    double& T_HT_hot_out /*C*/, double& T_CT_cold /*C*/) {

    double T_HT_cold_des, T_CT_hot_des;
    mp_carnot_heat_pump->get_des_for_perf(T_HT_cold_des, T_CT_hot_des);

    return mp_carnot_heat_pump->performance(T_HT_cold_des, m_dot_ND,
        T_CT_hot_des, m_dot_ND,
        W_dot_gross_ND /*-*/, Q_dot_ND /*-*/,
        Q_dot_cold_in_ND /*-*/,
        T_HT_hot_out /*C*/, T_CT_cold /*C*/);
}

// ***********************
// Inherited methods
// ***********************
void C_csp_cr_heat_pump::init(const C_csp_collector_receiver::S_csp_cr_init_inputs init_inputs,
    C_csp_collector_receiver::S_csp_cr_solved_params& solved_params)
{
    // Not using init_inputs because the parameters are relevant to solar energy

    heat_pump_helpers::design_calcs(m_q_dot_hot_out_des, m_COP_heat_des,
        m_f_elec_consume_vs_W_dot_thermo_des,
        m_W_dot_in_thermo_des, m_q_dot_cold_in_des,
        m_W_dot_consume_elec_des, m_W_dot_in_net_des,
        m_COP_net_des);

    std::unique_ptr<HTFProperties> HT_htfProps(new HTFProperties());
    m_HT_htfProps = std::move(HT_htfProps);
    m_HT_htfProps->Initialize(m_HT_htf_code, m_HT_ud_htf_props);

    std::unique_ptr<HTFProperties> CT_htfProps(new HTFProperties());
    m_CT_htfProps = std::move(CT_htfProps);
    m_CT_htfProps->Initialize(m_CT_htf_code, m_CT_ud_htf_props);

    double eta_htf_pump = 0.85;     //[-] used to back out pressure drop

    m_T_HT_HTF_avg_des = 0.5 * (m_T_HT_HTF_cold_des + m_T_HT_HTF_hot_des);    //[C]
    m_cp_HT_HTF_des = m_HT_htfProps->Cp_ave(m_T_HT_HTF_cold_des + 273.15, m_T_HT_HTF_hot_des + 273.15); //[kJ/kg-K]
    double rho_HT_htf_des = m_HT_htfProps->dens(m_T_HT_HTF_avg_des + 273.15,1.0);   //[kg/m3]  
    m_m_dot_HT_des = m_q_dot_hot_out_des * 1.E3 / (m_cp_HT_HTF_des * (m_T_HT_HTF_hot_des - m_T_HT_HTF_cold_des));    //[kg/s]
    m_W_dot_HT_htf_pump_des = m_heat_pump_HT_htf_pump_coef*m_m_dot_HT_des*1.E-3;        //[MWe]
    double HT_htf_deltaP = m_W_dot_HT_htf_pump_des*rho_HT_htf_des/m_m_dot_HT_des*eta_htf_pump;   //[MPa]

    m_T_CT_HTF_avg_des = 0.5 * (m_T_CT_HTF_cold_des + m_T_CT_HTF_hot_des);    //[C]
    m_cp_CT_HTF_des = m_CT_htfProps->Cp_ave(m_T_CT_HTF_cold_des + 273.15, m_T_CT_HTF_hot_des + 273.15);  //[kJ/kg-K]
    double rho_CT_htf_des = m_CT_htfProps->dens(m_T_CT_HTF_avg_des + 273.15,1.0);   //[kg/m3]
    m_m_dot_CT_des = m_q_dot_cold_in_des*1.E3/(m_cp_CT_HTF_des*(m_T_CT_HTF_hot_des-m_T_CT_HTF_cold_des));  //[kg/s]
    m_W_dot_CT_htf_pump_des = m_heat_pump_CT_htf_pump_coef*m_m_dot_CT_des*1.E-3;        //[MWe]
    double CT_htf_deltaP = m_W_dot_CT_htf_pump_des*rho_CT_htf_des/m_m_dot_CT_des*eta_htf_pump;  //[MWe]

    m_m_dot_CT_to_HT_ratio = m_m_dot_CT_des / m_m_dot_HT_des;   //[-]

    // Set up Carnot Heat Pump class
    mp_carnot_heat_pump = std::shared_ptr<heat_pump_helpers::C_carnot_heat_pump> (new heat_pump_helpers::C_carnot_heat_pump(
                            m_T_HT_HTF_hot_des, m_T_HT_HTF_cold_des, m_T_CT_HTF_hot_des, m_T_CT_HTF_cold_des));

    // Min operating
    m_q_dot_min_des = m_f_q_dot_min * m_q_dot_hot_out_des;  //[MWt]

    // Check startup parameters
    m_f_q_dot_des_allowable_su = std::max(0.0, m_f_q_dot_des_allowable_su); //[-]
    m_hrs_startup_at_max_rate = std::max(0.0, m_hrs_startup_at_max_rate);   //[hr]

    // Calculate design startup requirements
        // Base startup on q_dot_hot even though it's heat out of cycle and to TES
        // Probably the most critical HX in system. also analogous to q_dot_rec?
    m_q_dot_su_max = m_q_dot_hot_out_des * m_f_q_dot_des_allowable_su;  //[MWt]
    m_W_dot_su_max = m_q_dot_su_max / m_COP_heat_des;           //[-]
    m_E_su_des = m_q_dot_su_max * m_hrs_startup_at_max_rate;    //[MWt-hr]
    m_E_W_dot_su_des = m_W_dot_su_max * m_hrs_startup_at_max_rate;  //[MWe-hr]
    m_t_su_des = m_E_su_des / m_q_dot_su_max;   //[hr]

    solved_params.m_T_htf_cold_des = m_T_HT_HTF_cold_des + 273.15; //[K]
    solved_params.m_P_cold_des = std::numeric_limits<double>::quiet_NaN();  //[kPa]
    solved_params.m_x_cold_des = std::numeric_limits<double>::quiet_NaN();  //[-]
    solved_params.m_T_htf_hot_des = m_T_HT_HTF_hot_des + 273.15;   //[K]
    solved_params.m_q_dot_rec_des = m_q_dot_hot_out_des;         //[MWt]
    solved_params.m_A_aper_total = 0.0;                         //[m2]
    solved_params.m_dP_sf = HT_htf_deltaP*1.E1;                //[bar] convert from MPa

    solved_params.m_CT_to_HT_m_dot_ratio = m_m_dot_CT_to_HT_ratio;  //[-]

    // State variables
    m_E_su_initial = m_E_su_des;        //[MWt-hr]
    if (m_E_su_initial == 0.0) {
        m_operating_mode_converged = C_csp_collector_receiver::OFF_NO_SU_REQ;
    }
    else {
        m_operating_mode_converged = C_csp_collector_receiver::OFF;					//
    }

    return;
}

C_csp_collector_receiver::E_csp_cr_modes C_csp_cr_heat_pump::get_operating_state()
{
    return m_operating_mode_converged;  //[-]
}

double C_csp_cr_heat_pump::get_startup_time()   //[hr]
{
    return m_t_su_des;      //[hr]
}

double C_csp_cr_heat_pump::get_startup_energy() //[MWt-hr]
{
    return m_E_su_des;      //[MWt-hr]
}

double C_csp_cr_heat_pump::get_pumping_parasitic_coef()  //MWe/MWt
{
    throw(C_csp_exception("C_csp_cr_heat_pump::get_pumping_parasitic_coef() is not complete"));
}

double C_csp_cr_heat_pump::get_min_power_delivery()    //MWt
{
    return m_q_dot_min_des;     //[MWt]
}

double C_csp_cr_heat_pump::get_max_power_delivery(double T_cold_in)   //MWt
{
    return m_q_dot_hot_out_des; //[MWt]
}

double C_csp_cr_heat_pump::get_tracking_power()		//MWe
{
    return 0.0;
}

double C_csp_cr_heat_pump::get_col_startup_power()		//MWe-hr
{
    throw(C_csp_exception("C_csp_cr_heat_pump::get_col_startup_power() is not complete"));
}

double C_csp_cr_heat_pump::get_design_pumping_power() {

    // This method returns the pumping power at the design case where the system is generating power
    // Assume that for PTES, the charging unit is off when discharging unit is on, so return 0.
    return 0.0;
}

void C_csp_cr_heat_pump::off(const C_csp_weatherreader::S_outputs& weather,
    const C_csp_solver_htf_1state& htf_state_in,
    C_csp_collector_receiver::S_csp_cr_out_solver& cr_out_solver,
    const C_csp_solver_sim_info& sim_info)
{
    double m_dot_CT_htf = 0.0;      //[kg/s]

    // Set outputs required by solver
    cr_out_solver.m_q_startup = 0.0;                //[MWt-hr]
    cr_out_solver.m_time_required_su = 0.0;         //[s]
    cr_out_solver.m_m_dot_salt_tot = 0.0;           //[kg/hr]
    cr_out_solver.m_q_thermal = 0.0;                //[MWt]
    cr_out_solver.m_T_salt_hot = m_T_HT_HTF_hot_des;//[C]
    cr_out_solver.m_component_defocus = 1.0;        //[-]
    cr_out_solver.m_is_recirculating = false;       //[-]

    cr_out_solver.m_q_dot_heater = 0.0;             //[MWt]

    cr_out_solver.m_W_dot_elec_in_tot = 0.0;        //[MWe]

    cr_out_solver.m_T_CT_htf_cold_out = m_T_CT_HTF_cold_des;    //[C]
    cr_out_solver.m_m_dot_CT_htf = m_dot_CT_htf*3600.0;            //[kg/s]

    m_operating_mode = C_csp_collector_receiver::OFF;
    m_E_su_calculated = m_E_su_des;     //[MWt-hr]

    // Set reported outputs
    mc_reported_outputs.value(E_T_HT_HTF_IN, m_T_HT_HTF_cold_des);  //[C]
    mc_reported_outputs.value(E_T_HT_HTF_OUT, m_T_HT_HTF_hot_des); //[C]
    mc_reported_outputs.value(E_T_CT_HTF_IN, m_T_CT_HTF_hot_des);   //[C]
    mc_reported_outputs.value(E_T_CT_HTF_OUT, m_T_CT_HTF_cold_des); //[C]
    mc_reported_outputs.value(E_M_DOT_HT_HTF, cr_out_solver.m_m_dot_salt_tot/3600.0);   //[kg/s]
    mc_reported_outputs.value(E_M_DOT_CT_HTF, m_dot_CT_htf);        //[kg/s]
    mc_reported_outputs.value(E_Q_DOT_STARTUP, 0.0);                //[MWt]
    mc_reported_outputs.value(E_Q_DOT_HOT_OUT, cr_out_solver.m_q_thermal);  //[MWt]
    mc_reported_outputs.value(E_Q_DOT_COLD_IN, 0.0);                //[MWt]
    mc_reported_outputs.value(E_W_DOT_IN_THERMO, 0.0);              //[MWe]
    mc_reported_outputs.value(E_W_DOT_CYCLE_PARASITICS, 0.0);       //[MWe]
    mc_reported_outputs.value(E_W_DOT_HT_HTF_PUMP, 0.0);            //[MWe]
    mc_reported_outputs.value(E_W_DOT_CT_HTF_PUMP, 0.0);            //[MWe]
    mc_reported_outputs.value(E_W_DOT_HEATER, cr_out_solver.m_W_dot_elec_in_tot);   //[MWe]

    return;
}

void C_csp_cr_heat_pump::startup(const C_csp_weatherreader::S_outputs& weather,
    const C_csp_solver_htf_1state& htf_state_in,
    C_csp_collector_receiver::S_csp_cr_out_solver& cr_out_solver,
    const C_csp_solver_sim_info& sim_info)
{
    double m_dot_CT_htf = 0.0;      //[kg/s]

    double step_hrs = sim_info.ms_ts.m_step / 3600.0;    //[hr]

    // Assume startup is always at max startup rate
    double q_dot_su = m_q_dot_su_max;   //[MWt]
    double time_remaining_su = m_E_su_initial / q_dot_su; //[hr]

    double time_required_su = std::numeric_limits<double>::quiet_NaN();

    if (time_remaining_su > step_hrs) {
        time_required_su = step_hrs;
        m_operating_mode = C_csp_collector_receiver::STARTUP;
    }
    else {
        time_required_su = time_remaining_su;
        m_operating_mode = C_csp_collector_receiver::ON;
    }

    double q_startup = q_dot_su * time_required_su;         //[MWt-hr]

    // Apply net COP to get electricity consumption at startup
    double W_dot_in_thermo = q_dot_su / m_COP_heat_des;     //[MWe]
    double W_dot_in_parasitics = q_dot_su / m_COP_net_des - W_dot_in_thermo;     //[MWe]
    // But don't calculate or estimate HT & CT pumping power
    //    because we don't really know mechanisms of startup
    double W_dot_HT_htf_pump = 0.0;     //[MWe]
    double W_dot_CT_htf_pump = 0.0;     //[MWe]
    double W_dot_htf_pumps = W_dot_HT_htf_pump + W_dot_CT_htf_pump; //[MWe]

    m_E_su_calculated = fmax(0.0, m_E_su_initial - q_startup);  //[MWt-hr]

    cr_out_solver.m_q_startup = q_startup;                  //[MWt-hr]
    cr_out_solver.m_time_required_su = time_required_su * 3600.0; //[s]
    cr_out_solver.m_m_dot_salt_tot = 0.0;                   //[kg/hr]
    cr_out_solver.m_q_thermal = 0.0;                        //[MWt]
    cr_out_solver.m_T_salt_hot = m_T_HT_HTF_hot_des;        //[C]
    cr_out_solver.m_component_defocus = 1.0;                //[-]
    cr_out_solver.m_is_recirculating = false;               //[-]

    cr_out_solver.m_q_dot_heater = 0.0;             //[MWt]

    cr_out_solver.m_W_dot_elec_in_tot = W_dot_in_thermo + W_dot_in_parasitics + W_dot_htf_pumps;        //[MWe]

    cr_out_solver.m_T_CT_htf_cold_out = m_T_CT_HTF_cold_des;      //[C]
    cr_out_solver.m_m_dot_CT_htf = m_dot_CT_htf*3600.0;       //[kg/s]

    // Set reported outputs
    mc_reported_outputs.value(E_T_HT_HTF_IN, m_T_HT_HTF_cold_des);      //[C]
    mc_reported_outputs.value(E_T_HT_HTF_OUT, m_T_HT_HTF_hot_des);     //[C]
    mc_reported_outputs.value(E_T_CT_HTF_IN, m_T_CT_HTF_hot_des);       //[C]
    mc_reported_outputs.value(E_T_CT_HTF_OUT, m_T_CT_HTF_cold_des);     //[C]
    mc_reported_outputs.value(E_M_DOT_HT_HTF, cr_out_solver.m_m_dot_salt_tot / 3600.0);   //[kg/s]
    mc_reported_outputs.value(E_M_DOT_CT_HTF, m_dot_CT_htf);        //[kg/s]
    mc_reported_outputs.value(E_Q_DOT_STARTUP, q_dot_su);                //[MWt]
    mc_reported_outputs.value(E_Q_DOT_HOT_OUT, cr_out_solver.m_q_thermal);  //[MWt]
    mc_reported_outputs.value(E_Q_DOT_COLD_IN, 0.0);                //[MWt]
    mc_reported_outputs.value(E_W_DOT_IN_THERMO, W_dot_in_thermo);  //[MWe]
    mc_reported_outputs.value(E_W_DOT_CYCLE_PARASITICS, W_dot_in_parasitics);       //[MWe]
    mc_reported_outputs.value(E_W_DOT_HT_HTF_PUMP, W_dot_HT_htf_pump);            //[MWe]
    mc_reported_outputs.value(E_W_DOT_CT_HTF_PUMP, W_dot_CT_htf_pump);            //[MWe]
    mc_reported_outputs.value(E_W_DOT_HEATER, cr_out_solver.m_W_dot_elec_in_tot);   //[MWe]


    return;
}

void C_csp_cr_heat_pump::on(const C_csp_weatherreader::S_outputs& weather,
    const C_csp_solver_htf_1state& htf_state_in,
    double q_dot_elec_to_CR_heat /*MWt*/, double field_control,
    C_csp_collector_receiver::S_csp_cr_out_solver& cr_out_solver,
    const C_csp_solver_sim_info& sim_info)
{
    throw(C_csp_exception("C_csp_cr_heat_pump must use 'on' method with T_CT_htf_hot_in argument"));
}

void C_csp_cr_heat_pump::on(const C_csp_weatherreader::S_outputs& weather,
    const C_csp_solver_htf_1state& htf_state_in,
    double T_CT_HTF_hot_in /*C*/,
    double q_dot_elec_to_CR_heat /*MWt*/, double field_control,
    C_csp_collector_receiver::S_csp_cr_out_solver& cr_out_solver,
    const C_csp_solver_sim_info& sim_info)
{
    double T_HT_HTF_cold_in = htf_state_in.m_temp;      //[C]
    //double T_CT_HTF_hot_in = m_T_CT_HTF_hot_des;        //[C]

    // Assume:
    // 1) no dependence between available heater output and weather (for now)
    // 2) heater is always capable of design output
    // 3) no mass flow rate bounds (for now)
    // 4) heater is controlled to always return HTF at design hot temperature (for now)

    // Control may send separate q_dot_elec_to_CR_heat and field_control signals
    // May eventually also want to apply a "component turn-down"
    // .... e.g. inlet HTF temp is warm relative to design, causing mass flow rate to be too high
    double heater_turn_down = 1.0;  //[-]
    double q_dot_elec = q_dot_elec_to_CR_heat * field_control * heater_turn_down;  //[MWt]

    double T_HT_HTF_hot_out = std::numeric_limits<double>::quiet_NaN();
    double T_CT_HTF_cold_out = std::numeric_limits<double>::quiet_NaN();
    double m_dot_HT_htf = std::numeric_limits<double>::quiet_NaN();
    double m_dot_CT_htf = std::numeric_limits<double>::quiet_NaN();
    double q_dot_HT_htf = std::numeric_limits<double>::quiet_NaN();
    double q_dot_CT_htf = std::numeric_limits<double>::quiet_NaN();
    double W_dot_thermo = std::numeric_limits<double>::quiet_NaN();
    double cop_thermo = std::numeric_limits<double>::quiet_NaN();
    double W_dot_cycle_parasitics = std::numeric_limits<double>::quiet_NaN();

    // Check if value is less than min allowed
    if (q_dot_elec < m_q_dot_min_des) {

        m_operating_mode = C_csp_collector_receiver::OFF;
        q_dot_elec = 0.0;       //[MWt]

        T_HT_HTF_hot_out = m_T_HT_HTF_hot_des;      //[C]
        T_CT_HTF_cold_out = m_T_CT_HTF_cold_des;    //[C]
        m_dot_HT_htf = 0.0;     //[kg/s]
        m_dot_CT_htf = 0.0;     //[kg/s]
        q_dot_HT_htf = 0.0;     //[MWt]
        q_dot_CT_htf = 0.0;     //[MWt]
        W_dot_thermo = 0.0;     //[MWe]
        cop_thermo = 0.0;       //[-]

        W_dot_cycle_parasitics = 0.0;   //[MWe]
    }
    else {

        m_operating_mode = C_csp_collector_receiver::ON;

        m_dot_HT_htf = q_dot_elec * 1.E3 / (m_cp_HT_HTF_des * (m_T_HT_HTF_hot_des - T_HT_HTF_cold_in));       //[kg/s]
        m_dot_CT_htf = m_dot_HT_htf * m_m_dot_CT_to_HT_ratio;

        double m_dot_HT_ND = m_dot_HT_htf / m_m_dot_HT_des;
        double m_dot_CT_ND = m_dot_CT_htf / m_m_dot_CT_des;

        // know T_HT_HTF_cold_in, T_CT_HTF_hot_in, and m_dot_HT_htf
        //    so can use carnot scaling model to calculate off design COP
        // know q_dot_hot and get W_dot_in from COP so calc q_dot_cold
        //    we're also setting m_dot_CT_htf and know T_CT_HTF_hot_in
        // which means that T_CT_HTF_cold_out must float

        double W_dot_gross_ND, Q_dot_hot_out_ND, Q_dot_cold_in_ND;
        W_dot_gross_ND = Q_dot_hot_out_ND = Q_dot_cold_in_ND = std::numeric_limits<double>::quiet_NaN();
        mp_carnot_heat_pump->performance(T_HT_HTF_cold_in, m_dot_HT_ND,
                                    T_CT_HTF_hot_in, m_dot_CT_ND,
                                    W_dot_gross_ND, Q_dot_hot_out_ND,
                                    Q_dot_cold_in_ND,
                                    T_HT_HTF_hot_out, T_CT_HTF_cold_out);

        q_dot_HT_htf = m_dot_HT_htf*m_cp_HT_HTF_des*(T_HT_HTF_hot_out - T_HT_HTF_cold_in)*1.E-3;    //[MWt]
        W_dot_thermo = m_W_dot_in_thermo_des * W_dot_gross_ND;          //[MWe]
        cop_thermo = q_dot_HT_htf / W_dot_thermo;                       //[-]
        W_dot_cycle_parasitics = m_W_dot_consume_elec_des * W_dot_gross_ND;     //[MWe]

        q_dot_CT_htf = m_dot_CT_htf*m_cp_CT_HTF_des*(T_CT_HTF_hot_in - T_CT_HTF_cold_out)*1.E-3;    //[kWt]

        double q_dot_CT_htf_ND_check = q_dot_CT_htf / m_q_dot_cold_in_des;
    }

    double W_dot_HT_htf_pump = m_heat_pump_HT_htf_pump_coef * m_dot_HT_htf * 1.E-3;     //[MWe]
    double W_dot_CT_htf_pump = m_heat_pump_CT_htf_pump_coef * m_dot_CT_htf * 1.E-3;     //[MWe]

    double W_dot_heater = W_dot_thermo + W_dot_cycle_parasitics + W_dot_HT_htf_pump + W_dot_CT_htf_pump;    //[MWe]

    m_E_su_calculated = 0.0;        //[MWt-hr]

    // Set solver outputs and return
    cr_out_solver.m_q_startup = 0.0;            //[MWt-hr]
    cr_out_solver.m_time_required_su = 0.0;     //[s]
    cr_out_solver.m_m_dot_salt_tot = m_dot_HT_htf*3600.0;   //[kg/hr] convert from kg/s
    cr_out_solver.m_q_thermal = q_dot_HT_htf;   //[MWt]
    cr_out_solver.m_T_salt_hot = T_HT_HTF_hot_out;   //[C]
    cr_out_solver.m_component_defocus = heater_turn_down;   //[-]

    // Treat this as external heat input -> set to 0
    cr_out_solver.m_q_dot_heater = 0.0;

    cr_out_solver.m_W_dot_elec_in_tot = W_dot_heater;       //[MWe]

    cr_out_solver.m_T_CT_htf_cold_out = T_CT_HTF_cold_out;  //[C]
    cr_out_solver.m_m_dot_CT_htf = m_dot_CT_htf*3600.0;     //[kg/hr]

    // Set reported outputs
    mc_reported_outputs.value(E_T_HT_HTF_IN, T_HT_HTF_cold_in);      //[C]
    mc_reported_outputs.value(E_T_HT_HTF_OUT, T_HT_HTF_hot_out);     //[C]
    mc_reported_outputs.value(E_T_CT_HTF_IN, T_CT_HTF_hot_in);       //[C]
    mc_reported_outputs.value(E_T_CT_HTF_OUT, T_CT_HTF_cold_out);    //[C]
    mc_reported_outputs.value(E_M_DOT_HT_HTF, cr_out_solver.m_m_dot_salt_tot / 3600.0);   //[kg/s]
    mc_reported_outputs.value(E_M_DOT_CT_HTF, m_dot_CT_htf);        //[kg/s]
    mc_reported_outputs.value(E_Q_DOT_STARTUP, 0.0);                //[MWt]
    mc_reported_outputs.value(E_Q_DOT_HOT_OUT, cr_out_solver.m_q_thermal);  //[MWt]
    mc_reported_outputs.value(E_Q_DOT_COLD_IN, q_dot_CT_htf);                //[MWt]
    mc_reported_outputs.value(E_W_DOT_IN_THERMO, W_dot_thermo);     //[MWe]
    mc_reported_outputs.value(E_W_DOT_CYCLE_PARASITICS, W_dot_cycle_parasitics);       //[MWe]
    mc_reported_outputs.value(E_W_DOT_HT_HTF_PUMP, W_dot_HT_htf_pump);            //[MWe]
    mc_reported_outputs.value(E_W_DOT_CT_HTF_PUMP, W_dot_CT_htf_pump);            //[MWe]
    mc_reported_outputs.value(E_W_DOT_HEATER, cr_out_solver.m_W_dot_elec_in_tot);   //[MWe]
}

void C_csp_cr_heat_pump::estimates(const C_csp_weatherreader::S_outputs& weather,
    const C_csp_solver_htf_1state& htf_state_in,
    C_csp_collector_receiver::S_csp_cr_est_out& est_out,
    const C_csp_solver_sim_info& sim_info)
{
    // Assume:
    // 1) no dependence between available heater output and weather
    // 2) heater is always capable of design output
    // 3) no mass flow rate bounds (for now)
    // 4) heater is controlled to always return HTF at design hot temperature

    double m_dot_htf = m_q_dot_hot_out_des * 1.E3 / (m_cp_HT_HTF_des * (m_T_HT_HTF_hot_des - htf_state_in.m_temp));    //[kg/s]

    E_csp_cr_modes mode = get_operating_state();

    if (mode == C_csp_collector_receiver::ON || mode == C_csp_collector_receiver::OFF_NO_SU_REQ)
    {
        est_out.m_q_dot_avail = m_q_dot_hot_out_des;	//[MWt]
        est_out.m_m_dot_avail = m_dot_htf * 3600.0;		//[kg/hr]
        est_out.m_T_htf_hot = m_T_HT_HTF_hot_des;		//[C]
        est_out.m_q_startup_avail = 0.0;                //[MWt]
    }
    else
    {
        est_out.m_q_startup_avail = m_q_dot_hot_out_des;	//[MWt]
        est_out.m_q_dot_avail = 0.0;
        est_out.m_m_dot_avail = 0.0;
        est_out.m_T_htf_hot = 0.0;
    }

    return;
}

void C_csp_cr_heat_pump::converged()
{
    m_operating_mode_converged = m_operating_mode;

    // Operating mode methods should handle this, but can check here too
    if (m_operating_mode_converged == OFF) {

        m_E_su_calculated = m_E_su_des;
    }

    if (m_E_su_des == 0.0 && m_operating_mode_converged == OFF) {
        m_operating_mode_converged = OFF_NO_SU_REQ;
    }

    m_E_su_initial = m_E_su_calculated;

    mc_reported_outputs.set_timestep_outputs();
}

void C_csp_cr_heat_pump::write_output_intervals(double report_time_start,
    const std::vector<double>& v_temp_ts_time_end, double report_time_end)
{
    mc_reported_outputs.send_to_reporting_ts_array(report_time_start,
        v_temp_ts_time_end, report_time_end);
}

double C_csp_cr_heat_pump::calculate_optical_efficiency(const C_csp_weatherreader::S_outputs& weather, const C_csp_solver_sim_info& sim)
{
    throw(C_csp_exception("C_csp_cr_heat_pump::calculate_optical_efficiency() is not complete"));
}

double C_csp_cr_heat_pump::calculate_thermal_efficiency_approx(const C_csp_weatherreader::S_outputs& weather, double q_incident /*MW*/, const C_csp_solver_sim_info& sim)
{
    throw(C_csp_exception("C_csp_cr_heat_pump::calculate_thermal_efficiency() is not complete"));
}

double C_csp_cr_heat_pump::get_collector_area()
{
    // Collector area is not a relevant metric for a heat pump
    return std::numeric_limits<double>::quiet_NaN();
}

double C_csp_cr_heat_pump::get_design_electric_to_heat_cop()
{
    return m_COP_heat_des;  //[-]
}

void C_csp_cr_heat_pump::get_design_parameters(double& W_dot_in /*MWe*/,
    double& q_dot_cold_in /*MWt*/, double& q_dot_hot_out /*MWt*/,
    double& W_dot_elec_parasitic /*MWe*/, double& W_dot_in_net /*MWe*/,
    double& COP_net /*-*/,
    double& m_dot_HT_htf /*kg/s*/, double& cp_HT_htf /*kJ/kg-K*/, double& W_dot_HT_htf_pump /*MWe*/,
    double& m_dot_CT_htf /*kg/s*/, double& cp_CT_htf /*kJ/kg-K*/, double& W_dot_CT_htf_pump /*MWe*/,
    double& E_su /*MWt-hr*/)
{
    W_dot_in = m_W_dot_in_thermo_des;       //[MWe]
    q_dot_cold_in = m_q_dot_cold_in_des;    //[MWt]
    q_dot_hot_out = m_q_dot_hot_out_des;    //[MWt]
    W_dot_elec_parasitic = m_W_dot_consume_elec_des;    //[MWe]
    W_dot_in_net = m_W_dot_in_net_des;      //[MWe]
    COP_net = m_COP_net_des;                //[-]

    m_dot_HT_htf = m_m_dot_HT_des;          //[kg/s]
    cp_HT_htf = m_cp_HT_HTF_des;            //[kJ/kg-K]
    W_dot_HT_htf_pump = m_W_dot_HT_htf_pump_des;    //[MWe]

    m_dot_CT_htf = m_m_dot_CT_des;          //[kg/s]
    cp_CT_htf = m_cp_CT_HTF_des;            //[kJ/kg-K]
    W_dot_CT_htf_pump = m_W_dot_CT_htf_pump_des;    //[MWe]

    E_su = m_E_su_des;                      //[MWt-hr]
}

// ***************************************************************
// ***************************************************************

void heat_pump_helpers::design_calcs(double q_dot_hot_out /*MWt*/, double COP_heat /*-*/,
    double f_elec_consume_vs_W_dot_thermo /*-*/,
    double& W_dot_in_thermo /*MWe*/, double& q_dot_cold_in /*MWt*/,
    double& W_dot_consume_elec /*MWe*/, double& W_dot_in_net /*MWe*/,
    double& COP_heat_net /*-*/)
{
    W_dot_in_thermo = q_dot_hot_out / COP_heat;             //[MWe]
    q_dot_cold_in = W_dot_in_thermo * (COP_heat - 1.0);     //[MWt]
    W_dot_consume_elec = f_elec_consume_vs_W_dot_thermo * W_dot_in_thermo;  //[MWe]
    W_dot_in_net = W_dot_in_thermo + W_dot_consume_elec;    //[MWe]
    COP_heat_net = q_dot_hot_out / W_dot_in_net;            //[-]
}

heat_pump_helpers::C_carnot_heat_pump::C_carnot_heat_pump(double T_HT_hot_des /*C*/, double T_HT_cold_des /*C*/,
    double T_CT_hot_des /*C*/, double T_CT_cold_des /*C*/)
{
    m_T_HT_hot_des = T_HT_hot_des;      //[C]
    m_T_HT_cold_des = T_HT_cold_des;    //[C]
    m_T_CT_hot_des = T_CT_hot_des;      //[C]
    m_T_CT_cold_des = T_CT_cold_des;    //[C]

    m_cop_carnot_des = cop_carnot(m_T_HT_hot_des, m_T_HT_cold_des,
        m_T_CT_hot_des, m_T_CT_cold_des);
}

double heat_pump_helpers::C_carnot_heat_pump::cop_carnot(double T_HT_hot /*C*/, double T_HT_cold /*C*/,
    double T_CT_hot /*C*/, double T_CT_cold /*C*/)
{
    double T_HT_avg = 0.5 * (T_HT_hot + T_HT_cold) + 273.15;  //[K]
    double T_CT_avg = 0.5 * (T_CT_hot + T_CT_cold) + 273.15;  //[K]

    return T_HT_avg / (T_HT_avg - T_CT_avg);
}

void heat_pump_helpers::C_carnot_heat_pump::get_des_for_perf(double& T_HT_cold_des /*C*/,
    double& T_CT_hot_des /*C*/)
{
    T_HT_cold_des = m_T_HT_cold_des;
    T_CT_hot_des = m_T_CT_hot_des;
}

int heat_pump_helpers::C_carnot_heat_pump::performance(double T_HT_cold_in /*C*/, double m_dot_HT_ND /*-*/,
    double T_CT_hot /*C*/, double m_dot_CT_ND /*-*/,
    double& W_dot_gross_ND /*-*/, double& Q_dot_hot_out_ND /*-*/,
    double& Q_dot_cold_in_ND /*-*/,
    double& T_HT_hot_out /*C*/, double& T_CT_cold /*C*/)
{
    // Assume T_HT_hot_out is always equal to design
    T_HT_hot_out = m_T_HT_hot_des;        //[C]

    // Calculate normalized HT deltaT for part-load estimate
    double deltaT_HT_ND = (T_HT_hot_out - T_HT_cold_in) / (m_T_HT_hot_des - m_T_HT_cold_des);    //[-]
    Q_dot_hot_out_ND = m_dot_HT_ND * deltaT_HT_ND;

    double deltaT_CT_des = m_T_CT_hot_des - m_T_CT_cold_des;    //[C]
    // Guess T_CT_cold
    // This 1) constraints q_dot_cold_in, which then sets W_dot_in and COP
    // and 2) allows calculation of carnot COP
    //       and COP allows us to calculate W_dot_in, q_dot_cold_in, and T_CT_hot
    // So iterate on T_CT_hot
    heat_pump_helpers::C_MEQ__T_CT_cold c_eq(this,
                            T_HT_hot_out, T_HT_cold_in, m_dot_HT_ND,
                            T_CT_hot, m_dot_CT_ND,
                            Q_dot_hot_out_ND, deltaT_CT_des);

    C_monotonic_eq_solver c_solver(c_eq);

    double T_CT_cold_guess = m_T_CT_cold_des;     //[C]
    double diff_T_CT_cold = std::numeric_limits<double>::quiet_NaN();
    int T_CT_cold_code = c_solver.test_member_function(T_CT_cold_guess, &diff_T_CT_cold);

    if (T_CT_cold_code != 0) {
        return -1;
    }

    double tol_T_CT_cold = 0.1;  //[C]

    if (std::abs(diff_T_CT_cold) > tol_T_CT_cold) {

        double T_CT_cold_guess_2 = c_eq.m_T_CT_cold_calc;    //[C]
        double diff_T_CT_cold_guess_2 = std::numeric_limits<double>::quiet_NaN();
        int T_CT_cold_code2 = c_solver.test_member_function(T_CT_cold_guess_2, &diff_T_CT_cold_guess_2);

        if (T_CT_cold_code2 != 0) {
            return -1;
        }

        if (std::abs(diff_T_CT_cold_guess_2) > tol_T_CT_cold) {

            C_monotonic_eq_solver::S_xy_pair xy1;
            xy1.x = T_CT_cold_guess;
            xy1.y = diff_T_CT_cold;

            C_monotonic_eq_solver::S_xy_pair xy2;
            xy2.x = T_CT_cold_guess_2;
            xy2.y = diff_T_CT_cold_guess_2;

            c_solver.settings(tol_T_CT_cold, 50, std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN(), false);

            double T_CT_cold_solved, tol_solved;
            T_CT_cold_solved = tol_solved = std::numeric_limits<double>::quiet_NaN();
            int T_CT_cold_solve_code = -1;
            int iter_solved = -1;

            try {
                T_CT_cold_solve_code = c_solver.solve(xy1, xy2, 0.0, T_CT_cold_solved, tol_solved, iter_solved);
            }
            catch (C_csp_exception){
                return -2;
            }

            if (T_CT_cold_solve_code != C_monotonic_eq_solver::CONVERGED){
                return -3;
            }
        }
    }

    W_dot_gross_ND = c_eq.m_W_dot_gross_ND;     //[-]
    Q_dot_cold_in_ND = c_eq.m_Q_dot_cold_ND;    //[-]
    T_CT_cold = c_eq.m_T_CT_cold_calc;          //[C]

    return 0;

}

int heat_pump_helpers::C_MEQ__T_CT_cold::operator()(double T_CT_cold /*C*/, double* diff_T_CT_cold /*C*/)
{
    // Calculate new carnot cop and adjust for part-load
    // using input T_CT_hot
    double cop_temp = mpc_carnot_heat_pump->cop_carnot(m_T_HT_hot, m_T_HT_cold,
                                            m_T_CT_hot, T_CT_cold);
    double cop_pl = std::pow(1. - std::abs(1. - m_Q_dot_hot_out_ND), 0.2);
    double cop = cop_temp * cop_pl;

    // Calculate power by scaling by ratio of calculated and design carnot cop
    // COP_heat_net = q_dot_hot_out / W_dot_in_net;            //[-]
    // cop / cop_des = (q_dot_hot_ND / W_dot_gross_ND) / (1.0 / 1.0)
    m_W_dot_gross_ND = m_Q_dot_hot_out_ND / (cop / m_cop_des);

    // q_dot_cold_in = W_dot_in_thermo * (COP_heat - 1.0);     //[MWt]
    // Design: q_dot_cold_ND = W_dot_cold_ND * COP_heat_less_1_ND -> 1.0 = 1.0 * 1.0
    // Off-design: q_dot_cold_ND = W_dot_cold_ND * (cop - 1.)/(m_cop_des - 1.)
    m_Q_dot_cold_ND = m_W_dot_gross_ND * ((cop - 1.) / (m_cop_des - 1.));

    double deltaT_CT_ND = m_Q_dot_cold_ND / m_m_dot_CT_ND;
    m_T_CT_cold_calc = m_T_CT_hot - deltaT_CT_ND * (m_deltaT_CT_des);  //[C]

    // Normalize by design temperature difference?
    *diff_T_CT_cold = (m_T_CT_cold_calc - T_CT_cold);  //[C]

    return 0;
}

