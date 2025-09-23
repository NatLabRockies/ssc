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


#include "csp_solver_pc_ptes.h"
#include "csp_solver_core.h"

static C_csp_reported_outputs::S_output_info S_ptes_output_info[] =
{
    {C_pc_ptes::E_T_HT_HTF_HOT_IN, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_pc_ptes::E_T_HT_HTF_COLD_OUT, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_pc_ptes::E_T_CT_HTF_COLD_IN, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_pc_ptes::E_T_CT_HTF_HOT_OUT, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_pc_ptes::E_M_DOT_HT_HTF, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_pc_ptes::E_M_DOT_CT_HTF, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_pc_ptes::E_Q_DOT_STARTUP, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_pc_ptes::E_Q_DOT_HOT_IN, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_pc_ptes::E_Q_DOT_THERMO_OUT_TOTAL, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_pc_ptes::E_Q_DOT_TO_COLD_TES, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_pc_ptes::E_Q_DOT_REJECTED, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_pc_ptes::E_W_DOT_THERMO, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_pc_ptes::E_W_DOT_CYCLE_PARASITICS, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_pc_ptes::E_W_DOT_HT_HTF_PUMP, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_pc_ptes::E_W_DOT_CT_HTF_PUMP, C_csp_reported_outputs::TS_WEIGHTED_AVE},

    csp_info_invalid
};

static C_csp_reported_outputs::S_dependent_output_info S_ptes_dependent_output_info[] =
{
    {C_pc_ptes::E_ETA_THERMAL, C_pc_ptes::E_W_DOT_THERMO, C_pc_ptes::E_Q_DOT_HOT_IN, C_csp_reported_outputs::AoverB},

    csp_dep_info_invalid
};

// ***********************
// Inherited methods
// ***********************
C_pc_ptes::C_pc_ptes(double W_dot_thermo /*MWe*/, double eta_therm_mech /*-*/,
    double f_elec_consume_vs_gen /*-*/, double fixed__q_dot_cold__to__q_dot_warm /*-*/,
    double T_HT_HTF_hot /*C*/, double T_HT_HTF_cold /*C*/, double T_CT_HTF_cold /*C*/, double T_CT_HTF_hot /*C*/,
    double cycle_max_frac /*-*/, double cycle_cutoff_frac /*-*/, double q_sby_frac /*-*/,
    double startup_time /*hr*/, double startup_frac /*-*/,
    double HT_htf_pump_coef /*kW/kg/s*/, double CT_htf_pump_coef /*kW/kg/s*/,
    int HT_htf_code /*-*/, util::matrix_t<double> HT_ud_htf_props,
    int CT_htf_code /*-*/, util::matrix_t<double> CT_ud_htf_props) : C_csp_power_cycle(ELEC)
{
    m_W_dot_thermo_des = W_dot_thermo;      //[MWe]
    m_eta_therm_mech_des = eta_therm_mech;              //[-]
    m_f_elec_consume_vs_gen = f_elec_consume_vs_gen;    //[-]
    m_fixed__q_dot_cold__to__q_dot_warm = fixed__q_dot_cold__to__q_dot_warm;  //[-]
    m_T_HT_HTF_hot_des = T_HT_HTF_hot;              //[C]
    m_T_HT_HTF_cold_des = T_HT_HTF_cold;            //[C]
    m_T_CT_HTF_cold_des = T_CT_HTF_cold;            //[C]
    m_T_CT_HTF_hot_des = T_CT_HTF_hot;              //[C]
    m_cycle_max_frac_des = cycle_max_frac;          //[-]
    m_cycle_cutoff_frac_des = cycle_cutoff_frac;    //[-]
    m_q_sby_frac_des = q_sby_frac;                  //[-]
    m_startup_time_des = startup_time;              //[hr]
    m_startup_frac_des = startup_frac;              //[-]

    m_HT_htf_pump_coef_des = HT_htf_pump_coef;      //[kW/kg/s]
    m_CT_htf_pump_coef_des = CT_htf_pump_coef;      //[kW/kg/s]

    m_HT_htf_code = HT_htf_code;
    m_HT_ud_htf_props = HT_ud_htf_props;

    m_CT_htf_code = CT_htf_code;
    m_CT_ud_htf_props = CT_ud_htf_props;

    // Initialize calculated member variables to nan
    m_W_dot_net_des = std::numeric_limits<double>::quiet_NaN();
    m_q_dot_hot_in_des = std::numeric_limits<double>::quiet_NaN();
    m_q_dot_cold_out_thermo_des = std::numeric_limits<double>::quiet_NaN();
    m_W_dot_elec_parasitic_des = std::numeric_limits<double>::quiet_NaN();
    m_eta_overall_des = std::numeric_limits<double>::quiet_NaN();
    m_q_dot_cold_to_CTES = std::numeric_limits<double>::quiet_NaN();
    m_q_dot_cold_to_surroundings = std::numeric_limits<double>::quiet_NaN();

    m_T_HT_HTF_avg_des = std::numeric_limits<double>::quiet_NaN();
    m_cp_HT_HTF_des = std::numeric_limits<double>::quiet_NaN();
    m_T_CT_HTF_avg_des = std::numeric_limits<double>::quiet_NaN();
    m_cp_CT_HTF_des = std::numeric_limits<double>::quiet_NaN();

    m_m_dot_HT_des = std::numeric_limits<double>::quiet_NaN();
    m_m_dot_HT_min = std::numeric_limits<double>::quiet_NaN();
    m_m_dot_HT_max = std::numeric_limits<double>::quiet_NaN();
    m_W_dot_HT_htf_pump_des = std::numeric_limits<double>::quiet_NaN();

    m_q_dot_HT_max = std::numeric_limits<double>::quiet_NaN();
    m_q_dot_HT_min = std::numeric_limits<double>::quiet_NaN();

    m_m_dot_CT_des = std::numeric_limits<double>::quiet_NaN();
    m_W_dot_CT_htf_pump_des = std::numeric_limits<double>::quiet_NaN();

    m_m_dot_CT_to_HT_ratio = std::numeric_limits<double>::quiet_NaN();

    m_E_su_des = std::numeric_limits<double>::quiet_NaN();
    m_E_standby_des = std::numeric_limits<double>::quiet_NaN();

    mc_reported_outputs.construct(S_ptes_output_info, S_ptes_dependent_output_info);
}

void C_pc_ptes::init(C_csp_power_cycle::S_solved_params& solved_params)
{
    pc_ptes_helpers::design_calcs__all(m_W_dot_thermo_des, m_f_elec_consume_vs_gen,
                                        m_eta_therm_mech_des, m_fixed__q_dot_cold__to__q_dot_warm,
                                        m_W_dot_net_des, m_W_dot_elec_parasitic_des,
                                        m_q_dot_hot_in_des, m_q_dot_cold_out_thermo_des, m_eta_overall_des,
                                        m_q_dot_cold_to_CTES, m_q_dot_cold_to_surroundings);

    std::unique_ptr<HTFProperties> HT_htfProps(new HTFProperties());
    m_HT_htfProps = std::move(HT_htfProps);
    m_HT_htfProps->Initialize(m_HT_htf_code, m_HT_ud_htf_props);

    std::unique_ptr<HTFProperties> CT_htfProps(new HTFProperties());
    m_CT_htfProps = std::move(CT_htfProps);
    m_CT_htfProps->Initialize(m_CT_htf_code, m_CT_ud_htf_props);

    double eta_htf_pump = 0.85;     //[-] used to back out pressure drop

    m_T_HT_HTF_avg_des = 0.5*(m_T_HT_HTF_cold_des + m_T_HT_HTF_hot_des);    //[C]
    m_cp_HT_HTF_des = m_HT_htfProps->Cp_ave(m_T_HT_HTF_cold_des + 273.15, m_T_HT_HTF_hot_des + 273.15);  //[kJ/kg-K]
    m_m_dot_HT_des = m_q_dot_hot_in_des * 1.E3 / (m_cp_HT_HTF_des * (m_T_HT_HTF_hot_des - m_T_HT_HTF_cold_des));    //[kg/s]
    double rho_HT_htf_des = m_HT_htfProps->dens(m_T_HT_HTF_avg_des + 273.15, 1.0);   //[kg/m3]  
    m_W_dot_HT_htf_pump_des = m_HT_htf_pump_coef_des * m_m_dot_HT_des * 1.E-3;        //[MWe]
    double HT_htf_deltaP = m_W_dot_HT_htf_pump_des * rho_HT_htf_des / m_m_dot_HT_des * eta_htf_pump;   //[MPa]

    m_T_CT_HTF_avg_des = 0.5*(m_T_CT_HTF_cold_des + m_T_CT_HTF_hot_des);    //[C]
    m_cp_CT_HTF_des = m_CT_htfProps->Cp_ave(m_T_CT_HTF_cold_des + 273.15, m_T_CT_HTF_hot_des + 273.15);   //[kJ/kg-K]
    m_m_dot_CT_des = m_q_dot_cold_to_CTES*1.E3/(m_cp_CT_HTF_des*(m_T_CT_HTF_hot_des-m_T_CT_HTF_cold_des));  //[kg/s]
    double rho_CT_htf_des = m_CT_htfProps->dens(m_T_CT_HTF_avg_des + 273.15, 1.0);   //[kg/m3]
    m_W_dot_CT_htf_pump_des = m_CT_htf_pump_coef_des * m_m_dot_CT_des * 1.E-3;        //[MWe]
    double CT_htf_deltaP = m_W_dot_CT_htf_pump_des * rho_CT_htf_des / m_m_dot_CT_des * eta_htf_pump;  //[MWe]

    m_m_dot_CT_to_HT_ratio = m_m_dot_CT_des / m_m_dot_HT_des;   //[-]

    // Apply min/max fractions
    m_m_dot_HT_min = m_cycle_cutoff_frac_des * m_m_dot_HT_des;    //[kg/s]
    m_m_dot_HT_max = m_cycle_max_frac_des * m_m_dot_HT_des;       //[kg/s]
    m_q_dot_HT_min = m_cycle_cutoff_frac_des * m_q_dot_hot_in_des;  //[MWt]
    m_q_dot_HT_max = m_cycle_max_frac_des * m_q_dot_hot_in_des;     //[MWt]

    // Startup energy
    m_E_su_des = m_startup_frac_des*m_q_dot_hot_in_des;     //[MWt-hr] (implies 1 hr baked into this calc)
    m_E_standby_des = m_q_sby_frac_des*m_q_dot_hot_in_des;  //[MWt]

    // Initialize state variables
    m_operating_mode_prev = OFF;        // Assume cycle is off when simulation begins
    m_startup_energy_remain_prev = m_E_su_des;          //[MWt-hr]
    m_startup_time_remain_prev = m_startup_time_des;    //[hr]
    if(m_startup_frac_des == 0.0 && m_startup_time_des == 0.0 && m_operating_mode_prev == OFF){
        m_operating_mode_prev = OFF_NO_SU_REQ;
    }

    // Construct endo-reversible class
    mp_endo_reverse = std::unique_ptr<pc_ptes_helpers::C_endo_reversible_cycle> ( new pc_ptes_helpers::C_endo_reversible_cycle(m_T_HT_HTF_hot_des, m_T_HT_HTF_cold_des,
        m_T_CT_HTF_hot_des, m_T_CT_HTF_cold_des));

    // Set solved_params values
    solved_params.m_W_dot_des = m_W_dot_thermo_des;		//[MW] *thermo*, don't subtract electrical parasitics
    solved_params.m_eta_des = m_eta_therm_mech_des;		//[-] *thermo*, don't include electrical parasitics
    solved_params.m_q_dot_des = m_q_dot_hot_in_des;		//[MWt]
    solved_params.m_q_startup = m_E_su_des;	            //[MWt-hr]
    solved_params.m_max_frac = m_cycle_max_frac_des;		//[-]
    solved_params.m_cutoff_frac = m_cycle_cutoff_frac_des;	//[-]
    solved_params.m_sb_frac = m_q_sby_frac_des;				//[-]
    solved_params.m_T_htf_hot_ref = m_T_HT_HTF_hot_des;		//[C]
    solved_params.m_m_dot_design = m_m_dot_HT_des*3600.0;	//[kg/hr] *HT HTF*
    solved_params.m_m_dot_min = m_m_dot_HT_min*3600.;		//[kg/hr] *HT HTF*
    solved_params.m_m_dot_max = m_m_dot_HT_max*3600.;		//[kg/hr] *HT HTF*

    return;
}

C_csp_power_cycle::E_csp_power_cycle_modes C_pc_ptes::get_operating_state()
{
    return m_operating_mode_prev;
}

double C_pc_ptes::get_cold_startup_time()
{
    return m_startup_time_des;  //[hr]
}

double C_pc_ptes::get_warm_startup_time()
{
    return m_startup_time_des;  //[hr]
}

double C_pc_ptes::get_hot_startup_time()
{
    return m_startup_time_des;  //[hr]
}

double C_pc_ptes::get_standby_energy_requirement()    //[MW]
{
    return m_E_standby_des;     //[MWt]
}

double C_pc_ptes::get_cold_startup_energy()    //[MWh]
{
    return m_E_su_des;  //[MWt-hr]
}

double C_pc_ptes::get_warm_startup_energy()    //[MWh]
{
    return m_E_su_des;  //[MWt-hr]
}

double C_pc_ptes::get_hot_startup_energy()    //[MWh]
{
    return m_E_su_des;  //[MWt-hr]
}

double C_pc_ptes::get_max_thermal_power()     //[MWt]
{
    return m_q_dot_HT_max;      //[MWt]
}

double C_pc_ptes::get_min_thermal_power()     //[MWt]
{
    return m_q_dot_HT_min;      //[MWt]
}

void C_pc_ptes::get_max_power_output_operation_constraints(double T_amb /*C*/, double& m_dot_HTF_ND_max, double& W_dot_ND_max)	//[-] Normalized over design power
{
    m_dot_HTF_ND_max = m_cycle_max_frac_des;	//[-]
    W_dot_ND_max = m_dot_HTF_ND_max;
}

double C_pc_ptes::get_efficiency_at_TPH(double T_degC, double P_atm, double relhum_pct, double* w_dot_condenser)
{
    // Assumptions
    // 1) Design HT mass flow rate
    // 2) Design HT hot inlet temp
    // 3) Design CT cold inlet temp
    double m_dot_HT_htf_ND = 1.0;     //[-]
    double T_HT_htf_hot = m_T_HT_HTF_hot_des;       //[C]
    double T_CT_htf_cold = m_T_CT_HTF_cold_des;     //[C]
    double W_dot_thermo_ND, Q_dot_ND, T_HT_htf_cold, T_CT_htf_hot;
    mp_endo_reverse->performance(T_HT_htf_hot, m_dot_HT_htf_ND, T_CT_htf_cold, W_dot_thermo_ND, Q_dot_ND,
        T_HT_htf_cold, T_CT_htf_hot);

    double m_dot_HT_htf = m_dot_HT_htf_ND * m_m_dot_HT_des; //[kg/s]
    double q_dot_HT_htf = m_dot_HT_htf * m_cp_HT_HTF_des * (T_HT_htf_hot - T_HT_htf_cold) * 1.E-3;      //[MWt]
    double W_dot_thermo = m_W_dot_thermo_des * W_dot_thermo_ND;    //[MWe]
    double eta_thermo = W_dot_thermo / q_dot_HT_htf;
    if (w_dot_condenser != 0)
        *w_dot_condenser = m_W_dot_elec_parasitic_des * W_dot_thermo_ND;

    return eta_thermo;
}

double C_pc_ptes::get_efficiency_at_load(double load_frac, double* w_dot_condenser)
{
    // Assumptions
    // 1) Design ambient temperature
    // 2) Design HT hot inlet temp
    // 3) Design CT cold inlet temp
    double m_dot_HT_htf_ND = load_frac;     //[-]
    double T_HT_htf_hot = m_T_HT_HTF_hot_des;       //[C]
    double T_CT_htf_cold = m_T_CT_HTF_cold_des;     //[C]
    double W_dot_thermo_ND, Q_dot_ND, T_HT_htf_cold, T_CT_htf_hot;
    mp_endo_reverse->performance(T_HT_htf_hot, m_dot_HT_htf_ND, T_CT_htf_cold, W_dot_thermo_ND, Q_dot_ND,
        T_HT_htf_cold, T_CT_htf_hot);

    double m_dot_HT_htf = m_dot_HT_htf_ND * m_m_dot_HT_des; //[kg/s]
    double q_dot_HT_htf = m_dot_HT_htf * m_cp_HT_HTF_des * (T_HT_htf_hot - T_HT_htf_cold) * 1.E-3;      //[MWt]
    double W_dot_thermo = m_W_dot_thermo_des * W_dot_thermo_ND;    //[MWe]
    double eta_thermo = W_dot_thermo / q_dot_HT_htf;
    if (w_dot_condenser != 0)
        *w_dot_condenser = m_W_dot_elec_parasitic_des * W_dot_thermo_ND;

    return eta_thermo;
}

double C_pc_ptes::get_htf_pumping_parasitic_coef()		//[kWe/kWt]
{
    // Need to include both HT and CT pumps
    return (m_HT_htf_pump_coef_des*m_m_dot_HT_des + m_CT_htf_pump_coef_des*m_m_dot_CT_des) / (m_q_dot_hot_in_des * 1.E3);   //[kWe/kWt]
}

double C_pc_ptes::get_design_pumping_power() {

    // Need to include both HTF and CT pumps
    return m_W_dot_CT_htf_pump_des + m_W_dot_HT_htf_pump_des;   //[MWe]
}

double C_pc_ptes::get_design_cooling_power() {

    return m_W_dot_elec_parasitic_des;  //[MWe]
}

// This can vary between timesteps for Type224, depending on remaining startup energy and time
double C_pc_ptes::get_max_q_pc_startup()		//[MWt]
{
    if (m_startup_time_remain_prev > 0.0) {
        return fmin(m_q_dot_HT_max,
            m_startup_energy_remain_prev / 1.E3 / m_startup_time_remain_prev);		//[MWt]
    }
    else if (m_startup_energy_remain_prev > 0.0)
    {
        return m_q_dot_HT_max;    //[MWt]
    }
    else
    {
        return 0.0;
    }
}

void C_pc_ptes::call(const C_csp_weatherreader::S_outputs& weather,
    C_csp_solver_htf_1state& htf_state_in,
    const C_csp_power_cycle::S_control_inputs& inputs,
    C_csp_power_cycle::S_csp_pc_out_solver& out_solver,
    const C_csp_solver_sim_info& sim_info)
{
    throw(C_csp_exception("C_pc_ptes must use 'call' method with T_CT_htf_cold_in argument"));
}

void C_pc_ptes::call(const C_csp_weatherreader::S_outputs& weather,
    C_csp_solver_htf_1state& htf_state_in,
    double T_CT_htf_cold_in /*C*/,
    const C_csp_power_cycle::S_control_inputs& inputs,
    C_csp_power_cycle::S_csp_pc_out_solver& out_solver,
    const C_csp_solver_sim_info& sim_info)
{
    // Get sim info
    double time = sim_info.ms_ts.m_time;			//[s]
    double step_sec = sim_info.ms_ts.m_step;		//[s]

    // Get inputs
    double T_HT_htf_hot = htf_state_in.m_temp;		//[C]
    double m_dot_HT_htf = inputs.m_m_dot/3600.0;	//[kg/s]
    int standby_control = inputs.m_standby_control;	//[-] 1: On, 2: Standby, 3: Off

    //double T_CT_htf_cold = m_T_CT_HTF_cold_des; //[C]

    // Cold temp HTF mass flow rate is always constrained to design ratio of input high temp HTF mass flow rate
    double m_dot_CT_htf = m_dot_HT_htf * m_m_dot_CT_to_HT_ratio;

    double time_required_su = 0.0;
    double time_required_max = 0.0;

    m_operating_mode_calc = (C_csp_power_cycle::E_csp_power_cycle_modes)standby_control;

    // cycle operating method output variables
    double q_startup = std::numeric_limits<double>::quiet_NaN();                //[kWt-hr]
    double q_dot_HT_htf = std::numeric_limits<double>::quiet_NaN();                //[MWt]
    double m_dot_HT_htf_calc = std::numeric_limits<double>::quiet_NaN();           //[kg/s]
    double W_dot_thermo = std::numeric_limits<double>::quiet_NaN();             //[MWe]
    double eta_thermo = std::numeric_limits<double>::quiet_NaN();               //[-]
    double T_HT_htf_cold = std::numeric_limits<double>::quiet_NaN();            //[C]
    double T_CT_htf_hot = std::numeric_limits<double>::quiet_NaN();             //[C]
    double W_dot_cycle_parasitics = std::numeric_limits<double>::quiet_NaN();   //[MWe]

    double q_dot_to_cold_htf = std::numeric_limits<double>::quiet_NaN();    //[MWt]
    double q_dot_rejected = std::numeric_limits<double>::quiet_NaN();       //[MWt]

    bool was_method_successful = false;

    switch (standby_control)
    {
    case STARTUP:
        {
            double time_required_su_energy = m_startup_energy_remain_prev*1.E3 / (m_dot_HT_htf*m_cp_HT_HTF_des*(T_HT_htf_hot - m_T_HT_HTF_cold_des));	//[hr]
            double time_required_su_ramping = m_startup_time_remain_prev;	//[hr]

            time_required_max = fmax(time_required_su_energy, time_required_su_ramping);	//[hr]

            double time_step_hrs = step_sec / 3600.0;	//[hr]

            if (time_required_max > time_step_hrs)
            {
                time_required_su = time_step_hrs;		//[hr]
                m_operating_mode_calc = STARTUP;	//[-] Power cycle requires additional startup next timestep
                q_startup = m_dot_HT_htf * m_cp_HT_HTF_des * (T_HT_htf_hot - m_T_HT_HTF_cold_des) * time_step_hrs;	//[kW-hr]
            }
            else
            {
                time_required_su = time_required_max;	//[hr]
                m_operating_mode_calc = ON;	//[-] Power cycle has started up, next time step it will be ON

                double q_startup_energy_req = m_startup_energy_remain_prev;	//[MWt-hr]
                double q_startup_ramping_req = m_dot_HT_htf *m_cp_HT_HTF_des*(T_HT_htf_hot - m_T_HT_HTF_cold_des)*m_startup_time_remain_prev*1.E-3;	//[MWt-hr]
                q_startup = fmax(q_startup_energy_req, q_startup_ramping_req);	//[MWt-hr]
            }

            m_startup_time_remain_calc = fmax(m_startup_time_remain_prev - time_required_su, 0.0);	//[hr]
            m_startup_energy_remain_calc = fmax(m_startup_energy_remain_prev - q_startup, 0.0);		//[MWt-hr]
        }

        // Set output variabless
        q_dot_HT_htf = q_startup / time_required_su;	//[MWt]
        W_dot_thermo = 0.0;     //[MWe]
        eta_thermo = 0.0;       //[-]
        T_HT_htf_cold = m_T_HT_HTF_cold_des;    //[C]
        T_CT_htf_hot = m_T_CT_HTF_hot_des;      //[C]
        W_dot_cycle_parasitics = 0.0;           //[MWe]

        q_dot_to_cold_htf = m_dot_CT_htf*m_cp_CT_HTF_des*(T_CT_htf_hot - T_CT_htf_cold_in)*1.E-3;    //[MWt] convert from kWt
            // Assume balance of q_dot_HT_htf goes to internal energy of components
        q_dot_rejected = 0.0;       //[MWt]

        was_method_successful = true;

        break;

    case ON:

        {
            double m_dot_HT_htf_ND = m_dot_HT_htf / m_m_dot_HT_des;     //[-]
            double W_dot_thermo_ND, Q_dot_ND;
            mp_endo_reverse->performance(T_HT_htf_hot, m_dot_HT_htf_ND, T_CT_htf_cold_in, W_dot_thermo_ND, Q_dot_ND,
                                T_HT_htf_cold, T_CT_htf_hot);

            q_dot_HT_htf = m_dot_HT_htf*m_cp_HT_HTF_des*(T_HT_htf_hot - T_HT_htf_cold)*1.E-3;      //[MWt]
            W_dot_thermo = m_W_dot_thermo_des * W_dot_thermo_ND;    //[MWe]
            eta_thermo = W_dot_thermo / q_dot_HT_htf;
            W_dot_cycle_parasitics = m_W_dot_elec_parasitic_des * W_dot_thermo_ND;  //[MWe]

            q_startup = 0.0;    //[MWt-hr]

            double q_dot_out_thermo = q_dot_HT_htf - W_dot_thermo;  //[MWt]
            q_dot_to_cold_htf = m_dot_CT_htf*m_cp_CT_HTF_des*(T_CT_htf_hot - T_CT_htf_cold_in)*1.E-3;    //[MWt]
            q_dot_rejected = q_dot_out_thermo - q_dot_to_cold_htf;  //[MWt]

            was_method_successful = true;
        }

        break;

    case STANDBY:
        
        // Set output variabless
        q_dot_HT_htf = m_dot_HT_htf*m_cp_HT_HTF_des*(T_HT_htf_hot - m_T_HT_HTF_cold_des)*1.E-3;      //[MWt]
        W_dot_thermo = 0.0;                     //[MWe]
        eta_thermo = 0.0;                       //[-]
        T_HT_htf_cold = m_T_HT_HTF_cold_des;    //[C]
        T_CT_htf_hot = m_T_CT_HTF_hot_des;      //[C]
        W_dot_cycle_parasitics = 0.0;           //[MWe]

        q_startup = 0.0;    //[MWt-hr]

        q_dot_to_cold_htf = m_dot_CT_htf*m_cp_CT_HTF_des*(T_CT_htf_hot - T_CT_htf_cold_in)*1.E-3;    //[MWt] convert from kWt
            // modeling standby as steady state to need to reject balance heat
        q_dot_rejected = q_dot_HT_htf - q_dot_to_cold_htf;       //[MWt]

        was_method_successful = true;

        break;

    case OFF:

        // Set output variabless
        q_dot_HT_htf = 0.0;	                    //[MWt]
        W_dot_thermo = 0.0;                     //[MWe]
        eta_thermo = 0.0;                       //[-]
        T_HT_htf_cold = m_T_HT_HTF_cold_des;    //[C]
        T_CT_htf_hot = m_T_CT_HTF_hot_des;      //[C]
        W_dot_cycle_parasitics = 0.0;           //[MWe]

        // Cycle is off, so reset startup parameters!
        m_startup_time_remain_calc = m_startup_time_des;    //[hr]
        m_startup_energy_remain_calc = m_E_su_des;		    //[MWt-hr]

        q_startup = 0.0;    //[MWt-hr]

        q_dot_to_cold_htf = 0.0;    //[MWt-hr]
        q_dot_rejected = 0.0;       //[MWt-hr]

        was_method_successful = true;

        break;

    case STARTUP_CONTROLLED:
        // Thermal input can be controlled (e.g. TES mass flow rate is adjustable, rather than direct connection
        //     to the receiver), so find the mass flow rate that results in the required energy input can be achieved
        //     simultaneously with the required startup time. If the timestep is less than the required startup time
        //     scale the mass flow rate appropriately

            // Maximum thermal power to power cycle based on heat input constraint parameters:
        double q_dot_to_pc_max_q_constraint = m_cycle_max_frac_des * m_W_dot_thermo_des / m_eta_therm_mech_des;	//[MWt]
        //    // Maximum thermal power to power cycle based on mass flow rate constraint parameters:
        double q_dot_to_pc_max_m_constraint = m_m_dot_HT_max*m_cp_HT_HTF_des*(T_HT_htf_hot - m_T_HT_HTF_cold_des)*1.E-3;	//[MWt]
        //    // Choose smaller of two values
        double q_dot_to_pc_max = fmin(q_dot_to_pc_max_q_constraint, q_dot_to_pc_max_m_constraint);	//[MWt]

        double time_required_su_energy = m_startup_energy_remain_prev / q_dot_to_pc_max;		//[hr]
        double time_required_su_ramping = m_startup_time_remain_prev;		//[hr]

        time_required_max = fmax(time_required_su_energy, time_required_su_ramping);	//[hr]

        double q_dot_to_pc = std::numeric_limits<double>::quiet_NaN();      //[MWt]

        if (time_required_su_energy > time_required_su_ramping)	// Meeting energy requirements (at design thermal input) will require more time than time requirements
        {
            // Can the power cycle startup within the timestep?
            if (time_required_su_energy > step_sec / 3600.0)	// No: the power cycle startup will require another timestep
            {
                time_required_su = step_sec / 3600.0;	//[hr]
                m_operating_mode_calc = STARTUP;		//[-] Power cycle requires additional startup next timestep

            }
            else	// Yes: the power cycle will complete startup within this timestep
            {
                time_required_su = time_required_su_energy;	//[hr]
                m_operating_mode_calc = ON;				//[-] Power cycle has started up, next time step it will be ON
            }
            // If the thermal energy requirement is the limiting factor, then send max q_dot to power cycle
            q_dot_to_pc = q_dot_to_pc_max;		//[MWt]
        }
        else		// Meeting time requirements will require more time than energy requirements (at design thermal input)
        {
            // Can the power cycle startup within the timestep?
            if (time_required_su_ramping > step_sec / 3600.0)	// No: the power cycle startup will require another timestep
            {
                time_required_su = step_sec / 3600.0;			//[hr]
                m_operating_mode_calc = STARTUP;		//[-] Power cycle requires additional startup next timestep
            }
            else	// Yes: the power cycle will complete startup within this timestep
            {
                time_required_su = time_required_su_ramping;	//[hr]
                m_operating_mode_calc = ON;					//[-] Power cycle has started up, next time step it will be ON
            }

            q_dot_to_pc = m_startup_energy_remain_prev / time_required_su_ramping;		//[MWt]
        }
        q_startup = q_dot_to_pc * time_required_su;	//[MWt-hr]

        double m_dot_htf_required = (q_startup / time_required_su) / (m_cp_HT_HTF_des * (T_HT_htf_hot - m_T_HT_HTF_cold_des));	//[kg/s]

        m_startup_time_remain_calc = fmax(m_startup_time_remain_prev - time_required_su, 0.0);	//[hr]
        m_startup_energy_remain_calc = fmax(m_startup_energy_remain_prev - q_startup, 0.0);		//[MWt-hr]

        // Set output variabless
        q_dot_HT_htf = q_startup / (time_required_su);	   //[MWt]
        m_dot_HT_htf_calc = m_dot_htf_required;           //[kg/s]
        W_dot_thermo = 0.0;                     //[MWe]
        eta_thermo = 0.0;                       //[-]
        T_HT_htf_cold = m_T_HT_HTF_cold_des;    //[C]
        T_CT_htf_hot = m_T_CT_HTF_hot_des;      //[C]
        W_dot_cycle_parasitics = 0.0;           //[MWe]

        q_dot_to_cold_htf = m_dot_CT_htf*m_cp_CT_HTF_des*(T_CT_htf_hot - T_CT_htf_cold_in)*1.E-3;    //[MWt] convert from kWt
            // Assume balance of q_dot_HT_htf goes to internal energy of components
        q_dot_rejected = 0.0;       //[MWt]

        was_method_successful = true;

        break;

    }	// end switch() on standby control

    // Calculate HT and CT pumping power, which is only a function of m_dot_HT_htf and not operating mode
    double W_dot_HT_htf_pump = m_HT_htf_pump_coef_des * m_dot_HT_htf * 1.E-3;       //[MWe]
    double W_dot_CT_htf_pump = m_CT_htf_pump_coef_des * m_dot_CT_htf * 1.E-3;       //[MWe]

    // Post-process calcs
    // ***********************************
    double q_dot_startup = 0.0;     //[MWt]
    if (q_startup > 0.0) {
        q_dot_startup = q_startup / time_required_su;   //[MWt]
    }
    else {
        q_dot_startup = 0.0;
    }

    double q_dot_thermo_out_total = q_dot_to_cold_htf + q_dot_rejected; //[MWt]
    // ***********************************


    // Set outputs required by solver
    // ***********************************
    out_solver.m_P_cycle = W_dot_thermo;                //[MWe]
    out_solver.m_T_htf_cold = T_HT_htf_cold;            //[C]
    out_solver.m_m_dot_htf = m_dot_HT_htf*3600.0;       //[kg/hr]
    out_solver.m_W_cool_par = W_dot_cycle_parasitics;   //[MWe]
    out_solver.m_time_required_su = time_required_su * 3600.0;	    //[s]
    out_solver.m_time_required_max = time_required_max * 3600.0;	//[s]
    out_solver.m_q_dot_htf = q_dot_HT_htf;			    //[MWt] Thermal power from HTF (= thermal power into cycle)
    out_solver.m_W_dot_elec_parasitics_tot = out_solver.m_W_cool_par + W_dot_CT_htf_pump + W_dot_HT_htf_pump; //[MWe]

    out_solver.m_T_CT_htf_hot_out = T_CT_htf_hot;       //[C]
    out_solver.m_m_dot_CT_htf = m_dot_CT_htf*3600.0;    //[kg/hr]

    out_solver.m_was_method_successful = was_method_successful;	//[-]
    // ***********************************

    // Set reported outputs
    mc_reported_outputs.value(E_T_HT_HTF_HOT_IN, T_HT_htf_hot);     //[C]
    mc_reported_outputs.value(E_T_HT_HTF_COLD_OUT, T_HT_htf_cold);  //[C]
    mc_reported_outputs.value(E_T_CT_HTF_COLD_IN, T_CT_htf_cold_in);   //[C]
    mc_reported_outputs.value(E_T_CT_HTF_HOT_OUT, T_CT_htf_hot);    //[C]
    mc_reported_outputs.value(E_M_DOT_HT_HTF, m_dot_HT_htf);        //[kg/s]
    mc_reported_outputs.value(E_M_DOT_CT_HTF, m_dot_CT_htf);        //[kg/s]
    mc_reported_outputs.value(E_Q_DOT_STARTUP, q_dot_startup);      //[MWt]
    mc_reported_outputs.value(E_Q_DOT_HOT_IN, q_dot_HT_htf);        //[MWt]
    mc_reported_outputs.value(E_Q_DOT_THERMO_OUT_TOTAL, q_dot_thermo_out_total);      //[MWt]
    mc_reported_outputs.value(E_Q_DOT_TO_COLD_TES, q_dot_to_cold_htf);  //[MWt]
    mc_reported_outputs.value(E_Q_DOT_REJECTED, q_dot_rejected);        //[MWt]
    mc_reported_outputs.value(E_W_DOT_THERMO, W_dot_thermo);            //[MWe]
    mc_reported_outputs.value(E_W_DOT_CYCLE_PARASITICS, W_dot_cycle_parasitics);    //[MWe]
    mc_reported_outputs.value(E_W_DOT_HT_HTF_PUMP, W_dot_HT_htf_pump);  //[MWe]
    mc_reported_outputs.value(E_W_DOT_CT_HTF_PUMP, W_dot_CT_htf_pump);  //[MWe]

    return;
}

void C_pc_ptes::converged()
{
    m_operating_mode_prev = m_operating_mode_calc;
    m_startup_time_remain_prev = m_startup_time_remain_calc;
    m_startup_energy_remain_prev = m_startup_energy_remain_calc;

    if (m_startup_time_des == 0.0 && m_E_su_des == 0.0 && m_operating_mode_prev == OFF) {
        m_operating_mode_prev = OFF_NO_SU_REQ;
    }

    mc_reported_outputs.set_timestep_outputs();
}

void C_pc_ptes::write_output_intervals(double report_time_start,
    const std::vector<double>& v_temp_ts_time_end, double report_time_end)
{
    mc_reported_outputs.send_to_reporting_ts_array(report_time_start,
        v_temp_ts_time_end, report_time_end);
}

void C_pc_ptes::assign(int index, double* p_reporting_ts_array, size_t n_reporting_ts_array)
{
    mc_reported_outputs.assign(index, p_reporting_ts_array, n_reporting_ts_array);
}

void C_pc_ptes::get_design_parameters(double& W_dot_net /*MWe*/, double& q_dot_hot_in /*MWt*/,
    double& q_dot_cold_out_thermo /*MWt*/, double& W_dot_elec_parasitic /*MWe*/,
    double& eta_net /*-*/, double& q_dot_cold_to_CTES /*MWt*/, double& q_dot_cold_to_surr /*MWt*/,
    double& m_dot_HT_htf /*kg/s*/, double& cp_HT_htf /*kJ/kg-K*/, double& W_dot_HT_htf_pump /*MWe*/,
    double& m_dot_CT_htf /*kg/s*/, double& cp_CT_htf /*kJ/kg-K*/, double& W_dot_CT_htf_pump /*MWe*/)
{
    W_dot_net = m_W_dot_net_des;        //[MWe]
    q_dot_hot_in = m_q_dot_hot_in_des;  //[MWt]
    q_dot_cold_out_thermo = m_q_dot_cold_out_thermo_des;    //[MWt]
    W_dot_elec_parasitic = m_W_dot_elec_parasitic_des;      //[MWe]
    eta_net = m_eta_overall_des;        //[-]
    q_dot_cold_to_CTES = m_q_dot_cold_to_CTES;              //[MWt]
    q_dot_cold_to_surr = m_q_dot_cold_to_surroundings;      //[MWt]

    m_dot_HT_htf = m_m_dot_HT_des;      //[kg/s]
    cp_HT_htf = m_cp_HT_HTF_des;        //[kJ/kg-K]
    W_dot_HT_htf_pump = m_W_dot_HT_htf_pump_des;    //[MWe]
    m_dot_CT_htf = m_m_dot_CT_des;      //[kg/s]
    cp_CT_htf = m_cp_CT_HTF_des;        //[kJ/kg-K]
    W_dot_CT_htf_pump = m_W_dot_CT_htf_pump_des;    //[MWe]
}

void pc_ptes_helpers::design_calcs__all(double W_dot_thermo /*MWe*/, double f_elec_consume_vs_gen /*-*/,
    double eta_therm_mech /*-*/, double fixed__q_dot_cold__to__q_dot_warm /*-*/,
    double& W_dot_net /*MWe*/, double& W_dot_consume_elec /*MWe*/,
    double& q_dot_hot_in /*MWt*/, double& q_dot_cold_out_thermo /*MWt*/, double& eta_net /*-*/,
    double& q_dot_cold_to_CTES, double& q_dot_reject_to_surroundings /*MWt*/)
{
    design_calcs__no_ctes(W_dot_thermo, f_elec_consume_vs_gen,
        eta_therm_mech,
        W_dot_net, W_dot_consume_elec,
        q_dot_hot_in, q_dot_cold_out_thermo, eta_net);

    design_calcs__q_dot_ctes(q_dot_hot_in, fixed__q_dot_cold__to__q_dot_warm, q_dot_cold_out_thermo,
        q_dot_cold_to_CTES, q_dot_reject_to_surroundings);
}

void pc_ptes_helpers::design_calcs__no_ctes(double W_dot_thermo /*MWe*/, double f_elec_consume_vs_gen /*-*/,
    double eta_therm_mech /*-*/,
    double& W_dot_net /*MWe*/, double& W_dot_consume_elec /*MWe*/,
    double& q_dot_hot_in /*MWt*/, double& q_dot_cold_out_thermo /*MWt*/, double& eta_net /*-*/)
{
    W_dot_consume_elec = W_dot_thermo * f_elec_consume_vs_gen;  //[MWe]
    W_dot_net = W_dot_thermo - W_dot_consume_elec;      //[MWe]
    q_dot_hot_in = W_dot_thermo / eta_therm_mech;       //[MWt]
    q_dot_cold_out_thermo = W_dot_thermo * (1. / eta_therm_mech - 1.);   //[MWt]
    eta_net = W_dot_net / q_dot_hot_in;
}

void pc_ptes_helpers::design_calcs__q_dot_ctes(double q_dot_hot_in /*MWt*/, double fixed__q_dot_cold__to__q_dot_warm /*-*/,
    double q_dot_cold_out_thermo /*MWt*/,
    double& q_dot_cold_to_CTES, double& q_dot_reject_to_surroundings /*MWt*/)
{
    q_dot_cold_to_CTES = fixed__q_dot_cold__to__q_dot_warm * q_dot_hot_in;  //[MWt]
    q_dot_reject_to_surroundings = q_dot_cold_out_thermo - q_dot_cold_to_CTES;    //[MWt]
}

pc_ptes_helpers::C_endo_reversible_cycle::C_endo_reversible_cycle(double T_HT_hot_des /*C*/, double T_HT_cold_des /*C*/,
    double T_CT_hot_des /*C*/, double T_CT_cold_des /*C*/)
{
    m_T_HT_hot_des = T_HT_hot_des;      //[C]
    m_T_HT_cold_des = T_HT_cold_des;    //[C]
    m_T_CT_hot_des = T_CT_hot_des;      //[C]
    m_T_CT_cold_des = T_CT_cold_des;    //[C]

    m_eta_endo_des = eta_endo(m_T_HT_hot_des, m_T_HT_cold_des,
        m_T_CT_hot_des, m_T_CT_cold_des);
}

double pc_ptes_helpers::C_endo_reversible_cycle::eta_endo(double T_HT_hot /*C*/, double T_HT_cold /*C*/,
    double T_CT_hot /*C*/, double T_CT_cold /*C*/)
{
    double T_HT_avg = 0.5 * (T_HT_hot + T_HT_cold) + 273.15;  //[K]
    double T_CT_avg = 0.5 * (T_CT_hot + T_CT_cold) + 273.15;  //[K]

    return 1.0 - sqrt(T_CT_avg / T_HT_avg); //[-]
}

void pc_ptes_helpers::C_endo_reversible_cycle::performance(double T_HT_hot /*C*/, double m_dot_HT_ND /*-*/, double T_CT_cold /*C*/,
    double& W_dot_gross_ND /*-*/, double& Q_dot_ND /*-*/,
    double& T_HT_cold /*C*/, double& T_CT_hot /*C*/)
{
    // Assume T_HT_cold is always equal to design
    T_HT_cold = m_T_HT_cold_des;     //[C]

    // Calculate normalized HT deltaT for part-load estimate
    double deltaT_HT_ND = (T_HT_hot - T_HT_cold) / (m_T_HT_hot_des - m_T_HT_cold_des);    //[-]
    Q_dot_ND = m_dot_HT_ND * deltaT_HT_ND;

    // Assume T_CT_hot is always equal to design
    // This equation doesn't overconstrain energy balance
    //    because m_dot_CT_HTF is variable - a fixed part goes to CTES and the balance goes to ambient
    // Perhaps worth revisiting this when testing system model
    T_CT_hot = m_T_CT_hot_des;       //[C]

    // Calculate new endo-reversible efficiency and adjust for part-load
    double eta_temp = eta_endo(T_HT_hot, T_HT_cold, T_CT_hot, T_CT_cold);
    double eta_pl = std::pow(1 - std::abs(1 - Q_dot_ND), 0.2);
    double eta = eta_temp * eta_pl;

    // calculate power by scaling by ratio of calculated and design endo-reversible efficiencies
    // eta / eta_des = (W_dot_gross_ND / Q_dot_ND) / (1.0 / 1.0)
    W_dot_gross_ND = eta / m_eta_endo_des * Q_dot_ND;
}
