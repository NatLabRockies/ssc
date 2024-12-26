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


#include "csp_solver_pt_receiver.h"
#include "csp_solver_core.h"

#include "Ambient.h"
#include "definitions.h"

C_pt_receiver::C_pt_receiver(double h_tower /*m*/, double epsilon /*-*/,
    double T_htf_hot_des /*C*/, double T_htf_cold_des /*C*/,
    double f_rec_min /*-*/, double q_dot_rec_des /*MWt*/,
    double rec_su_delay /*hr*/, double rec_qf_delay /*-*/,
    double m_dot_htf_max_frac /*-*/, double eta_pump /*-*/,
    double od_tube /*mm*/, double th_tube /*mm*/,
    double piping_loss_coef /*Wt/m2-K*/, double pipe_length_add /*m*/,
    double pipe_length_mult /*-*/,
    int field_fl /*-*/, util::matrix_t<double> field_fl_props,
    int tube_mat_code /*-*/,
    int night_recirc /*-*/)
{
    // Design parameters
    m_h_tower = h_tower;    //[m]
    m_epsilon = epsilon;    //[-]
    m_T_htf_hot_des = T_htf_hot_des + 273.15;       //[K] convert from C
    m_T_htf_cold_des = T_htf_cold_des + 273.15;     //[K] convert from C
    m_f_rec_min = f_rec_min;    //[-]
    m_q_rec_des = q_dot_rec_des*1.E6;       //[Wt] convert from MWt
    m_rec_su_delay = std::max(0.0, rec_su_delay);  //[hr]
    m_rec_qf_delay = std::max(0.0, rec_qf_delay);  //[-]
    m_m_dot_htf_max_frac = m_dot_htf_max_frac;  //[-]
    m_eta_pump = eta_pump;  //[-]
    m_od_tube = od_tube*1.E-3;  //[m] convert from mm
    m_th_tube = th_tube*1.E-3;  //[m] convert from mm

    m_piping_loss_coefficient = piping_loss_coef;    //[Wt/m2-K]
    m_pipe_length_add = pipe_length_add;    //[m]
    m_pipe_length_mult = pipe_length_mult;  //[-]

    m_field_fl = field_fl;  //[-]
    m_field_fl_props = field_fl_props;

    m_tube_mat_code = tube_mat_code;    //[-]

    m_night_recirc = night_recirc;  //[-]

    // Define design ambient conditions
    m_T_amb_des = 15.0 + 273.15;          //[K]
    m_T_sky_des = m_T_amb_des - 20.0;       //[K]
    m_v_wind_10_des = 5.0;     //[m/s]
    m_P_amb_des = 101000.0;    //[Pa]

    // Calculated design geometry/dimensions
    m_L_piping = std::numeric_limits<double>::quiet_NaN();
    m_Q_dot_piping_loss = std::numeric_limits<double>::quiet_NaN();

    // Calculated design point performance
    m_q_dot_inc_min = std::numeric_limits<double>::quiet_NaN();
    m_eta_thermal_des_calc = std::numeric_limits<double>::quiet_NaN();
    m_W_dot_rec_pump_des_calc = std::numeric_limits<double>::quiet_NaN();
    m_W_dot_pumping_tower_share = std::numeric_limits<double>::quiet_NaN();
    m_W_dot_pumping_rec_share = std::numeric_limits<double>::quiet_NaN();
    m_rec_pump_coef = std::numeric_limits<double>::quiet_NaN();
    m_vel_htf_des = std::numeric_limits<double>::quiet_NaN();
    m_m_dot_htf_des = std::numeric_limits<double>::quiet_NaN();
    m_m_dot_htf_max = std::numeric_limits<double>::quiet_NaN();

    // State variables
    m_mode = C_csp_collector_receiver::E_csp_cr_modes::OFF;
    m_mode_prev = C_csp_collector_receiver::E_csp_cr_modes::OFF;

    m_E_su = std::numeric_limits<double>::quiet_NaN();
    m_E_su_prev = std::numeric_limits<double>::quiet_NaN();
    m_t_su = std::numeric_limits<double>::quiet_NaN();
    m_t_su_prev = std::numeric_limits<double>::quiet_NaN();

	error_msg = "";
}

C_csp_collector_receiver::E_csp_cr_modes C_pt_receiver::get_operating_state()
{
    return m_mode_prev;
}

void C_pt_receiver::init()
{
    ambient_air.SetFluid(ambient_air.Air);

    // Declare instance of fluid class for FIELD fluid
    if (m_field_fl != HTFProperties::User_defined && m_field_fl < HTFProperties::End_Library_Fluids)
    {
        if (!field_htfProps.SetFluid(m_field_fl))
        {
            throw(C_csp_exception("Receiver HTF code is not recognized", "MSPT receiver"));
        }
    }
    else if (m_field_fl == HTFProperties::User_defined)
    {
        // Check that 'm_field_fl_props' is allocated and correct dimensions
        int n_rows = (int)m_field_fl_props.nrows();
        int n_cols = (int)m_field_fl_props.ncols();
        if (n_rows > 2 && n_cols == 7)
        {
            if (!field_htfProps.SetUserDefinedFluid(m_field_fl_props))
            {
                error_msg = util::format(field_htfProps.UserFluidErrMessage(), n_rows, n_cols);
                throw(C_csp_exception(error_msg, "MSPT receiver"));
            }
        }
        else
        {
            error_msg = util::format("The user defined field HTF table must contain at least 3 rows and exactly 7 columns. The current table contains %d row(s) and %d column(s)", n_rows, n_cols);
            throw(C_csp_exception(error_msg, "MSPT receiver"));
        }
    }
    else
    {
        throw(C_csp_exception("Receiver HTF code is not recognized", "MSPT receiver"));
    }

    // Declare instance of htf class for receiver tube material
    if (m_tube_mat_code == HTFProperties::Stainless_AISI316 || m_tube_mat_code == HTFProperties::T91_Steel ||
        m_tube_mat_code == HTFProperties::N06230 || m_tube_mat_code == HTFProperties::N07740)
    {
        if (!tube_material.SetFluid(m_tube_mat_code))
        {
            throw(C_csp_exception("Tube material code not recognized", "MSPT receiver"));
        }
    }
    else if (m_tube_mat_code == HTFProperties::User_defined)
    {
        throw(C_csp_exception("Receiver material currently does not accept user defined properties", "MSPT receiver"));
    }
    else
    {
        error_msg = util::format("Receiver material code, %d, is not recognized", m_tube_mat_code);
        throw(C_csp_exception(error_msg, "MSPT receiver"));
    }

    return;
}

HTFProperties *C_pt_receiver::get_htf_property_object()
{
    return &field_htfProps;
}

double C_pt_receiver::get_startup_time()
{
    return m_rec_su_delay * 3600.; // sec
}

double C_pt_receiver::get_startup_energy()
{
    return m_rec_qf_delay * m_q_rec_des * 1.e-6;  // MWh
}

void C_pt_receiver::get_converged_values(C_csp_collector_receiver::E_csp_cr_modes& m_mode_final,
    double& E_su_final, double& t_su_final)
{
    m_mode_final = m_mode_prev;
    E_su_final = m_E_su_prev;
    t_su_final = m_t_su_prev;
}

double C_pt_receiver::estimate_thermal_efficiency(const C_csp_weatherreader::S_outputs& weather, double q_inc)
{
    /*
    A very approximate thermal efficiency used for quick optimization performance projections
    */

    double T_eff = (m_T_htf_cold_des + m_T_htf_hot_des) * .55;  //[K]

    double T_amb = weather.m_tdry + 273.15;
    double T_eff4 = T_eff * T_eff;
    T_eff4 *= T_eff4;
    double T_amb4 = T_amb * T_amb;
    T_amb4 *= T_amb4;

    double Arec = area_proj();

    double q_rad = 5.67e-8 * m_epsilon * Arec * (T_eff4 - T_amb4) * 1.e-6;   //MWt

    double v = weather.m_wspd;
    double v2 = v * v;
    double v3 = v2 * v;

    double q_conv = q_rad / 2. * (-0.001129 * v3 + 0.031229 * v2 - 0.01822 * v + 0.962476);  //convection is about half radiation, scale by wind speed. surrogate regression from molten salt run.

    return std::max(1. - (q_rad + q_conv) / q_inc, 0.);

}


double C_pt_receiver::getHeatLossPerApertureArea()
{
    throw std::runtime_error((std::string)__func__ + " is not implemented.");
};

double C_pt_receiver::get_min_power_delivery() //[MWt]
{
    return m_f_rec_min * m_q_rec_des * 1.E-6;   //[MWt]
}

double C_pt_receiver::get_max_power_delivery() //[MWt]
{
    return m_m_dot_htf_max_frac * m_q_rec_des * 1.E-6;  //[MWt]
}

double C_pt_receiver::get_T_htf_cold_des()    //[K]
{
    return m_T_htf_cold_des;    //[K]
}

double C_pt_receiver::get_T_htf_hot_des()    //[K]
{
    return m_T_htf_hot_des;    //[K]
}

double C_pt_receiver::get_q_dot_rec_des()     //[MWt]
{
    return m_q_rec_des * 1.E-6;     //[MWt]
}

void C_pt_receiver::get_design_geometry(double& L_tower_piping /*m*/, double& od_tube_calc /*m*/)
{
    L_tower_piping = m_L_piping;    //[m]
    od_tube_calc = m_od_tube;       //[m]
}

void C_pt_receiver::get_design_performance(double& eta_thermal /*-*/,
    double& W_dot_rec_pump /*MWe*/, double& W_dot_pumping_tower_share /*MWe*/, double& W_dot_pumping_rec_share /*MWe*/,
    double& rec_pump_coef /*MWe/MWt*/, double& rec_vel_htf_des /*m/s*/,
    double& m_dot_htf_rec /*kg/s*/, double& m_dot_htf_max /*kg/s*/,
    double& q_dot_piping_loss_des /*MWt*/)
{
    eta_thermal = m_eta_thermal_des_calc;   //[-]
    W_dot_rec_pump = m_W_dot_rec_pump_des_calc; //[MWe]
    W_dot_pumping_tower_share = m_W_dot_pumping_tower_share;    //[MWe]
    W_dot_pumping_rec_share = m_W_dot_pumping_rec_share;        //[MWe]
    rec_pump_coef = m_rec_pump_coef;            //[MWe/MWt]
    rec_vel_htf_des = m_vel_htf_des;            //[m/s]
    m_dot_htf_rec = m_m_dot_htf_des;            //[kg/s]
    q_dot_piping_loss_des = m_Q_dot_piping_loss*1.E-6;  //[MWt]
    m_dot_htf_max = m_m_dot_htf_max;            //[kg/s]
}


void C_pt_receiver::update_sizing(double val1, double val2, double val3, double val4)
{
    throw std::runtime_error((std::string)__func__ + " is not implemented.");
};

