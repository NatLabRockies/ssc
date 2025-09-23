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


#include "csp_solver_cr_electric_resistance.h"
#include "csp_solver_core.h"

#include "htf_props.h"
#include "lib_physics.h"

static C_csp_reported_outputs::S_output_info S_cr_electric_resistance_output_info[] =
{
    {C_csp_cr_electric_resistance::E_W_DOT_HEATER, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_csp_cr_electric_resistance::E_Q_DOT_HTF, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_csp_cr_electric_resistance::E_Q_DOT_STARTUP, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_csp_cr_electric_resistance::E_M_DOT_HTF, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_csp_cr_electric_resistance::E_T_HTF_IN, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_csp_cr_electric_resistance::E_T_HTF_OUT, C_csp_reported_outputs::TS_WEIGHTED_AVE},

    csp_info_invalid
};

C_csp_cr_electric_resistance::C_csp_cr_electric_resistance(double T_htf_cold_des /*C*/, double T_htf_hot_des /*C*/,
    double q_dot_heater_des /*MWt*/, double heater_efficiency /*-*/, double f_q_dot_min /*-*/,
    double f_q_dot_des_allowable_su /*-*/, double hrs_startup_at_max_rate /*hr*/,
    int htf_code /*-*/, util::matrix_t<double> ud_htf_props, E_elec_resist_startup_mode startup_mode)
{
    // Pass arguements to member data
    m_T_htf_cold_des = T_htf_cold_des;      //[C]
    m_T_htf_hot_des = T_htf_hot_des;        //[C]
    m_q_dot_heater_des = q_dot_heater_des;  //[MWt]
    m_heater_efficiency = heater_efficiency;    //[-]
    m_q_dot_min = f_q_dot_min*m_q_dot_heater_des; //[MWt]

    m_f_q_dot_des_allowable_su = f_q_dot_des_allowable_su;  //[-]
    m_hrs_startup_at_max_rate = hrs_startup_at_max_rate;    //[hr]

    m_htf_code = htf_code;              //[-] htf fluid code
    m_ud_htf_props = ud_htf_props;      //[-] user defined fluid properties

    // Initialize calculated member data
    m_m_dot_htf_des = std::numeric_limits<double>::quiet_NaN();
    m_dP_htf = std::numeric_limits<double>::quiet_NaN();
    m_cp_htf_des = std::numeric_limits<double>::quiet_NaN();
    m_q_dot_su_max = std::numeric_limits<double>::quiet_NaN();
    m_E_su_des = std::numeric_limits<double>::quiet_NaN();
    m_W_dot_heater_des = std::numeric_limits<double>::quiet_NaN();
    m_W_dot_pump_des = std::numeric_limits<double>::quiet_NaN();

    // Initialize state variables
    m_E_su_initial = m_E_su_calculated = std::numeric_limits<double>::quiet_NaN();

    mc_reported_outputs.construct(S_cr_electric_resistance_output_info);

    m_startup_mode = startup_mode;
}

C_csp_cr_electric_resistance::~C_csp_cr_electric_resistance(){}

// Inherited methods
void C_csp_cr_electric_resistance::init(const C_csp_collector_receiver::S_csp_cr_init_inputs init_inputs,
    C_csp_collector_receiver::S_csp_cr_solved_params& solved_params)
{
    // Not using init_inputs because the parameters are relevant to solar energy

    // Declare instance of fluid class for FIELD fluid
    if (m_htf_code != HTFProperties::User_defined && m_htf_code < HTFProperties::End_Library_Fluids)
    {
        if (!mc_pc_htfProps.SetFluid(m_htf_code))
        {
            throw(C_csp_exception("C_csp_cr_electric_resistance::init HTF code is not recognized"));
        }
    }
    else if (m_htf_code == HTFProperties::User_defined)
    {
        // Check that 'm_field_fl_props' is allocated and correct dimensions
        int n_rows = (int)m_ud_htf_props.nrows();
        int n_cols = (int)m_ud_htf_props.ncols();
        if (n_rows > 2 && n_cols == 7)
        {
            if (!mc_pc_htfProps.SetUserDefinedFluid(m_ud_htf_props))
            {
                std::string error_msg = util::format(mc_pc_htfProps.UserFluidErrMessage(), n_rows, n_cols);
                throw(C_csp_exception(error_msg, "Heat Sink Initialization"));
            }
        }
        else
        {
            std::string error_msg = util::format("The user defined field HTF table must contain at least 3 rows and exactly 7 columns. The current table contains %d row(s) and %d column(s)", n_rows, n_cols);
            throw(C_csp_exception(error_msg, "Heat Sink Initialization"));
        }
    }
    else
    {
        throw(C_csp_exception("Power cycle HTF code is not recognized", "Heat Sink Initialization"));
    }

    // For now, set pressure drop (and then pumping power) through heater and piping = 0
    // The pumping power is converted to heat, so will capture in the total electricity consumption
    m_dP_htf = 0.0;

    // Calculate the design point HTF mass flow rate
    m_cp_htf_des = mc_pc_htfProps.Cp_ave(physics::CelciusToKelvin(m_T_htf_cold_des), physics::CelciusToKelvin(m_T_htf_hot_des));	//[kJ/kg-K]
    m_m_dot_htf_des = m_q_dot_heater_des*1.E3 / (m_cp_htf_des*(m_T_htf_hot_des - m_T_htf_cold_des));	//[kg/s]
    m_W_dot_heater_des = m_q_dot_heater_des / m_heater_efficiency;      //[MWe]

    // For now, we're not modeling pumping power through the heater
    m_W_dot_pump_des = 0.0;     //[MWe]

    // Check startup parameters
    m_f_q_dot_des_allowable_su = std::max(0.0, m_f_q_dot_des_allowable_su); //[-]
    m_hrs_startup_at_max_rate = std::max(0.0, m_hrs_startup_at_max_rate);   //[hr]

    // Calculate design startup requirements
    m_q_dot_su_max = m_q_dot_heater_des*m_f_q_dot_des_allowable_su;  //[MWt]
    m_E_su_des = m_q_dot_su_max*m_hrs_startup_at_max_rate;   //[MWt-hr] 
    m_t_su_des = m_E_su_des / m_q_dot_su_max;   //[hr]

    solved_params.m_T_htf_cold_des = physics::CelciusToKelvin(m_T_htf_cold_des); //[K]
    solved_params.m_P_cold_des = std::numeric_limits<double>::quiet_NaN();  //[kPa]
    solved_params.m_x_cold_des = std::numeric_limits<double>::quiet_NaN();  //[-]
    solved_params.m_T_htf_hot_des = physics::CelciusToKelvin(m_T_htf_hot_des);   //[K]
    solved_params.m_q_dot_rec_des = m_q_dot_heater_des;         //[MWt]
    solved_params.m_A_aper_total = 0.0;                         //[m2]
    solved_params.m_dP_sf = m_dP_htf;                           //[bar]

    // State variables
    m_E_su_initial = m_E_su_des;        //[MWt-hr]
    if (m_E_su_initial == 0.0 || m_startup_mode == INSTANTANEOUS_NO_MAX_ELEC_IN) {
        m_operating_mode_converged = C_csp_collector_receiver::OFF_NO_SU_REQ;
    }
    else {
        m_operating_mode_converged = C_csp_collector_receiver::OFF;					//
    }
    

}

void C_csp_cr_electric_resistance::get_design_parameters(double& E_su_design /*MWt-hr*/,
                double& W_dot_heater_design /*MWe*/)
{
    E_su_design = m_E_su_des;                   //[MWt-hr]
    W_dot_heater_design = m_W_dot_heater_des;   //[MWe]
}

C_csp_collector_receiver::E_csp_cr_modes C_csp_cr_electric_resistance::get_operating_state()
{
    return m_operating_mode_converged;	//[-]
}

double C_csp_cr_electric_resistance::get_startup_time() // hr
{
    return m_t_su_des;
}

double C_csp_cr_electric_resistance::get_startup_energy() // MWh
{
    return m_E_su_des;
}

double C_csp_cr_electric_resistance::get_pumping_parasitic_coef()  //MWe/MWt
{
    throw(C_csp_exception("C_csp_cr_electric_resistance::get_pumping_parasitic_coef(...) is not complete"));
    return std::numeric_limits<double>::quiet_NaN();
}

double C_csp_cr_electric_resistance::get_min_power_delivery()    //MWt
{
    return m_q_dot_min;     //[MWt]
}

double C_csp_cr_electric_resistance::get_max_power_delivery(double T_htf_cold_in /*C*/)    //MWt
{
    return m_q_dot_heater_des;
}

double C_csp_cr_electric_resistance::get_tracking_power()	//MWe
{
    return 0.0;
}

double C_csp_cr_electric_resistance::get_col_startup_power()		//MWe-hr
{
    throw(C_csp_exception("C_csp_cr_electric_resistance::get_col_startup_power(...) is not complete"));
    return std::numeric_limits<double>::quiet_NaN();
}

double C_csp_cr_electric_resistance::get_design_pumping_power() {

    return m_W_dot_pump_des;    //[MWe]
}

void C_csp_cr_electric_resistance::off(const C_csp_weatherreader::S_outputs& weather,
    const C_csp_solver_htf_1state& htf_state_in,
    C_csp_collector_receiver::S_csp_cr_out_solver& cr_out_solver,
    const C_csp_solver_sim_info& sim_info)
{
    cr_out_solver.m_q_startup = 0.0;                //[MWt-hr]
    cr_out_solver.m_time_required_su = 0.0;         //[s]
    cr_out_solver.m_m_dot_salt_tot = 0.0;           //[kg/hr]
    cr_out_solver.m_q_thermal = 0.0;                //[MWt]
    cr_out_solver.m_T_salt_hot = m_T_htf_hot_des;   //[C]
    cr_out_solver.m_component_defocus = 1.0;        //[-]
    cr_out_solver.m_is_recirculating = false;       //[-]

    cr_out_solver.m_q_dot_heater = 0.0;             //[MWt]

    cr_out_solver.m_W_dot_elec_in_tot = 0.0;        //[MWe]

    m_operating_mode = C_csp_collector_receiver::OFF;
    m_E_su_calculated = m_E_su_des;     //[MWt-hr]

    // Set reported outputs
    mc_reported_outputs.value(E_W_DOT_HEATER, 0.0);     //[MWe]
    mc_reported_outputs.value(E_Q_DOT_HTF, 0.0);        //[MWt]
    mc_reported_outputs.value(E_Q_DOT_STARTUP, 0.0);    //[MWt]
    mc_reported_outputs.value(E_M_DOT_HTF, cr_out_solver.m_m_dot_salt_tot / 3600.0);
    mc_reported_outputs.value(E_T_HTF_IN, m_T_htf_cold_des);    //[C]
    mc_reported_outputs.value(E_T_HTF_OUT, m_T_htf_hot_des);    //[C]

    return;
}

void C_csp_cr_electric_resistance::startup(const C_csp_weatherreader::S_outputs& weather,
    const C_csp_solver_htf_1state& htf_state_in,
    C_csp_collector_receiver::S_csp_cr_out_solver& cr_out_solver,
    const C_csp_solver_sim_info& sim_info)
{
    if (m_startup_mode == INSTANTANEOUS_NO_MAX_ELEC_IN) {
        throw(C_csp_exception("C_csp_cr_electric_resistance::startup should not be called if startup mode is INSTANTANEOUS_NO_MAX_ELEC_IN"));
    }

    double step_hrs = sim_info.ms_ts.m_step / 3600.0;    //[hr]

    // Assume startup is always at max startup rate
    double time_remaining_su = m_E_su_initial / m_q_dot_su_max; //[hr]

    double time_required_su = std::numeric_limits<double>::quiet_NaN();

    if (time_remaining_su > step_hrs) {
        time_required_su = step_hrs;
        m_operating_mode = C_csp_collector_receiver::STARTUP;
    }
    else {
        time_required_su = time_remaining_su;
        m_operating_mode = C_csp_collector_receiver::ON;
    }

    double q_startup = m_q_dot_su_max * time_required_su;       //[MWt-hr]

    double W_dot_heater = m_q_dot_su_max / m_heater_efficiency;       //[MWe]

    m_E_su_calculated = std::max(0.0, m_E_su_initial - q_startup);  //[MWt-hr]

    cr_out_solver.m_q_startup = q_startup;                  //[MWt-hr]
    cr_out_solver.m_time_required_su = time_required_su*3600.0; //[s]
    cr_out_solver.m_m_dot_salt_tot = 0.0;                   //[kg/hr]
    cr_out_solver.m_q_thermal = 0.0;                        //[MWt]
    cr_out_solver.m_T_salt_hot = m_T_htf_hot_des;           //[C]
    cr_out_solver.m_component_defocus = 1.0;                //[-]
    cr_out_solver.m_is_recirculating = false;               //[-]

    // Treat this as external heat input -> set to 0
    cr_out_solver.m_q_dot_heater = 0.0;          //[MWt]

    cr_out_solver.m_W_dot_elec_in_tot = W_dot_heater;        //[MWe]

    // Set reported outputs
    mc_reported_outputs.value(E_W_DOT_HEATER, W_dot_heater);    //[MWe]
    mc_reported_outputs.value(E_Q_DOT_HTF, 0.0);     //[MWt]
    mc_reported_outputs.value(E_Q_DOT_STARTUP, m_q_dot_su_max); //[MWt]
    mc_reported_outputs.value(E_M_DOT_HTF, cr_out_solver.m_m_dot_salt_tot / 3600.0);    //[kg/s]
    mc_reported_outputs.value(E_T_HTF_IN, m_T_htf_cold_des);    //[C]
    mc_reported_outputs.value(E_T_HTF_OUT, m_T_htf_hot_des);    //[C]
}

void C_csp_cr_electric_resistance::on(const C_csp_weatherreader::S_outputs& weather,
    const C_csp_solver_htf_1state& htf_state_in,
    double q_dot_elec_to_CR_heat /*MWt*/, double field_control,
    C_csp_collector_receiver::S_csp_cr_out_solver& cr_out_solver,
    const C_csp_solver_sim_info& sim_info)
{
    // Assume:
    // 1) no dependence between available heater output and weather
    // 2) heater is always capable of design output
    // 3) no mass flow rate bounds (for now)
    // 4) heater is controlled to always return HTF at design hot temperature

    // Control may send separate q_dot_elec_to_CR_heat and field_control signals
    // May eventually also want to apply a "component turn-down"
    // .... e.g. inlet HTF temp is warm relative to design, causing mass flow rate to be too high
    double heater_turn_down = 1.0;  //[-]
    double q_dot_elec = q_dot_elec_to_CR_heat * field_control * heater_turn_down;  //[MWt]

    // Check if value is less than min allowed
    if (q_dot_elec < m_q_dot_min) {
        m_operating_mode = C_csp_collector_receiver::OFF;
        q_dot_elec = 0.0;       //[MWt]
        m_E_su_calculated = m_E_su_des;     //[MWt-hr]
    }
    else {
        m_operating_mode = C_csp_collector_receiver::ON;
        m_E_su_calculated = 0.0;        //[MWt-hr]
    }

    double W_dot_heater = q_dot_elec;       //[MWe]

    double cp_ave = mc_pc_htfProps.Cp_ave(physics::CelciusToKelvin(htf_state_in.m_temp), physics::CelciusToKelvin(m_T_htf_hot_des));
    double m_dot_htf = q_dot_elec * 1.E3 / (cp_ave*(m_T_htf_hot_des - htf_state_in.m_temp));  //[kg/s]

    double q_startup = 0.0;         //[MWt-hr]
    double q_dot_startup = 0.0;     //[MWt-hr]
    // Apply startup if in INSTANTANEOUS startup mode
    if (m_E_su_initial > 0.0 && m_startup_mode == INSTANTANEOUS_NO_MAX_ELEC_IN) {
        q_startup = m_E_su_initial;
        q_dot_startup = q_startup / (sim_info.ms_ts.m_step / 3600.0);   //[MWt]
    }

    double W_dot_heater_total = (q_dot_elec + q_dot_startup) / m_heater_efficiency;       //[MWe]

    // Set solver outputs and return
    cr_out_solver.m_q_startup = q_startup;        //[MWt-hr]
    cr_out_solver.m_time_required_su = 0.0; //[s]
    cr_out_solver.m_m_dot_salt_tot = m_dot_htf*3600.0;  //[kg/hr]
    cr_out_solver.m_q_thermal = q_dot_elec; //[MWt]
    cr_out_solver.m_T_salt_hot = m_T_htf_hot_des;   //[C]
    cr_out_solver.m_component_defocus = heater_turn_down;   //[-]

    // Treat this as external heat input -> set to 0
    // Used for freeze protection in CSP collector-receiver models
    cr_out_solver.m_q_dot_heater = 0.0; // q_dot_elec + q_dot_startup; //[MWt]

    cr_out_solver.m_W_dot_elec_in_tot = W_dot_heater_total;        //[MWe]

    // Set reported outputs
    mc_reported_outputs.value(E_W_DOT_HEATER, W_dot_heater_total);    //[MWe]
    mc_reported_outputs.value(E_Q_DOT_HTF, q_dot_elec);         //[MWt] only heat to HTF - doesn't include startup in INST mode
    mc_reported_outputs.value(E_Q_DOT_STARTUP, q_dot_startup);            //[MWt]
    mc_reported_outputs.value(E_M_DOT_HTF, cr_out_solver.m_m_dot_salt_tot / 3600.0);    //[kg/s]
    mc_reported_outputs.value(E_T_HTF_IN, htf_state_in.m_temp);    //[C]
    mc_reported_outputs.value(E_T_HTF_OUT, cr_out_solver.m_T_salt_hot);    //[C]

    return;
}

void C_csp_cr_electric_resistance::estimates(const C_csp_weatherreader::S_outputs& weather,
    const C_csp_solver_htf_1state& htf_state_in,
    C_csp_collector_receiver::S_csp_cr_est_out& est_out,
    const C_csp_solver_sim_info& sim_info)
{
    // Assume:
    // 1) no dependence between available heater output and weather
    // 2) heater is always capable of design output
    // 3) no mass flow rate bounds (for now)
    // 4) heater is controlled to always return HTF at design hot temperature
    double cp_ave = mc_pc_htfProps.Cp_ave(physics::CelciusToKelvin(htf_state_in.m_temp), physics::CelciusToKelvin(m_T_htf_hot_des));
    double m_dot_htf = m_q_dot_heater_des * 1.E3 / (cp_ave * (m_T_htf_hot_des - htf_state_in.m_temp));    //[kg/s]

    E_csp_cr_modes mode = get_operating_state();

    if (mode == C_csp_collector_receiver::ON || mode == C_csp_collector_receiver::OFF_NO_SU_REQ)
    {
        est_out.m_q_dot_avail = m_q_dot_heater_des;		//[MWt]
        est_out.m_m_dot_avail = m_dot_htf*3600.0;		//[kg/hr]
        est_out.m_T_htf_hot = m_T_htf_hot_des;			//[C]
        est_out.m_q_startup_avail = 0.0;                //[MWt]
    }
    else
    {
        est_out.m_q_startup_avail = m_q_dot_heater_des;		//[MWt]
        est_out.m_q_dot_avail = 0.0;
        est_out.m_m_dot_avail = 0.0;
        est_out.m_T_htf_hot = 0.0;
    }
    
    return;
}

void C_csp_cr_electric_resistance::converged()
{
    m_operating_mode_converged = m_operating_mode;

    // Operating mode methods should handle this, but can check here too
    if (m_operating_mode_converged == OFF) {

        m_E_su_calculated = m_E_su_des;
    }

    if ((m_startup_mode == INSTANTANEOUS_NO_MAX_ELEC_IN || m_E_su_des == 0.0)
        && m_operating_mode_converged == OFF) {
        m_operating_mode_converged = OFF_NO_SU_REQ;
    }

    m_E_su_initial = m_E_su_calculated;

    mc_reported_outputs.set_timestep_outputs();
}

void C_csp_cr_electric_resistance::write_output_intervals(double report_time_start,
    const std::vector<double>& v_temp_ts_time_end, double report_time_end)
{
    mc_reported_outputs.send_to_reporting_ts_array(report_time_start,
        v_temp_ts_time_end, report_time_end);
}

double C_csp_cr_electric_resistance::get_design_electric_to_heat_cop()
{
    return m_heater_efficiency;
}

double C_csp_cr_electric_resistance::calculate_optical_efficiency(const C_csp_weatherreader::S_outputs& weather, const C_csp_solver_sim_info& sim)
{
    // Optical efficiency is not a relevant metric for electric resistance heater
    return std::numeric_limits<double>::quiet_NaN();
}

double C_csp_cr_electric_resistance::calculate_thermal_efficiency_approx(const C_csp_weatherreader::S_outputs& weather, double q_incident /*MW*/, const C_csp_solver_sim_info& sim)
{
    throw(C_csp_exception("C_csp_cr_electric_resistance::calculate_thermal_efficiency_approx(...) is not complete"));
    return std::numeric_limits<double>::quiet_NaN();
}

double C_csp_cr_electric_resistance::get_collector_area()
{
    // Collector area is not a relevant metric for an electric resistance heater
    return std::numeric_limits<double>::quiet_NaN();
}
