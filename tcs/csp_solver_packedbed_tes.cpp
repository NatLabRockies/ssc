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


#include "csp_solver_packedbed_tes.h"

static C_csp_reported_outputs::S_output_info S_output_info[] =
{
    {C_csp_packedbed_tes::E_Q_DOT_LOSS, C_csp_reported_outputs::TS_WEIGHTED_AVE},		//[MWt] TES thermal losses
    {C_csp_packedbed_tes::E_W_DOT_HEATER, C_csp_reported_outputs::TS_WEIGHTED_AVE},		//[MWe] TES freeze protection power
    {C_csp_packedbed_tes::E_TES_T_HOT, C_csp_reported_outputs::TS_LAST},					//[C] TES final hot tank temperature
    {C_csp_packedbed_tes::E_TES_T_COLD, C_csp_reported_outputs::TS_LAST},				//[C] TES cold temperature at end of timestep      
    {C_csp_packedbed_tes::E_M_DOT_TANK_TO_TANK, C_csp_reported_outputs::TS_WEIGHTED_AVE},		//[MWt] TES thermal losses
    {C_csp_packedbed_tes::E_MASS_COLD_TANK, C_csp_reported_outputs::TS_LAST},			//[kg] Mass in cold tank at end of timestep		
    {C_csp_packedbed_tes::E_MASS_HOT_TANK, C_csp_reported_outputs::TS_LAST},				//[kg] Mass in hot tank at end of timestep
    {C_csp_packedbed_tes::E_HOT_TANK_HTF_PERC_FINAL, C_csp_reported_outputs::TS_LAST},	//[%] Final percent fill of available hot tank mass
    {C_csp_packedbed_tes::E_W_DOT_HTF_PUMP, C_csp_reported_outputs::TS_WEIGHTED_AVE},	//[MWe]
    {C_csp_packedbed_tes::E_VOL_TOT, C_csp_reported_outputs::TS_LAST},	//[m3]
    {C_csp_packedbed_tes::E_MASS_TOT, C_csp_reported_outputs::TS_LAST},	//[kg]
    {C_csp_packedbed_tes::E_T_GRAD_0, C_csp_reported_outputs::TS_WEIGHTED_AVE},	//[C]
    {C_csp_packedbed_tes::E_T_GRAD_1, C_csp_reported_outputs::TS_WEIGHTED_AVE},	//[C]
    {C_csp_packedbed_tes::E_T_GRAD_2, C_csp_reported_outputs::TS_WEIGHTED_AVE},	//[C]
    {C_csp_packedbed_tes::E_T_GRAD_3, C_csp_reported_outputs::TS_WEIGHTED_AVE},	//[C]
    {C_csp_packedbed_tes::E_T_GRAD_4, C_csp_reported_outputs::TS_WEIGHTED_AVE},	//[C]
    {C_csp_packedbed_tes::E_T_GRAD_5, C_csp_reported_outputs::TS_WEIGHTED_AVE},	//[C]
    {C_csp_packedbed_tes::E_T_GRAD_6, C_csp_reported_outputs::TS_WEIGHTED_AVE},	//[C]
    {C_csp_packedbed_tes::E_T_GRAD_7, C_csp_reported_outputs::TS_WEIGHTED_AVE},	//[C]
    {C_csp_packedbed_tes::E_T_GRAD_8, C_csp_reported_outputs::TS_WEIGHTED_AVE},	//[C]
    {C_csp_packedbed_tes::E_T_GRAD_9, C_csp_reported_outputs::TS_WEIGHTED_AVE},	//[C]
    csp_info_invalid
};

// Private Methods

void C_csp_packedbed_tes::size_pb_fixed_height(HTFProperties& tes_htf_props, double Q_tes_des /*MWt-hr*/, double f_oversize,
    double void_frac, double dens_solid /*kg/m3*/, double cp_solid /*J/kg K*/,
    double T_tes_hot /*K*/, double T_tes_cold /*K*/, double h_tank /*m*/,
    double& vol_total /*m3*/, double& d_tank_out /*m*/)
{
    double T_tes_ave = 0.5 * (T_tes_hot + T_tes_cold);		//[K]

    double dens_htf_ave = tes_htf_props.dens(T_tes_ave, 1.0);		//[kg/m^3] Density at average temperature
    double cp_htf_ave = tes_htf_props.Cp_ave(T_tes_cold, T_tes_hot) * 1000.0;//[J/kg-K] Specific heat at average temperature

    // Calculate Total Volume (include void space for HTF)
    double dT = (T_tes_hot - T_tes_cold);   //[dK]
    vol_total = f_oversize * (Q_tes_des * 1e6 * 3600.0) /
        ((((1.0 - void_frac) * dens_solid * cp_solid) + (void_frac * dens_htf_ave * cp_htf_ave)) * dT); // [m3]

    // Calculate Diameter
    d_tank_out = 2.0 * std::sqrt(vol_total / (h_tank * CSP::pi));   // [m]
}

void C_csp_packedbed_tes::size_pb_fixed_diameter(HTFProperties& tes_htf_props, double Q_tes_des /*MWt-hr*/, double f_oversize,
    double void_frac, double dens_solid /*kg/m3*/, double cp_solid /*J/kg K*/,
    double T_tes_hot /*K*/, double T_tes_cold /*K*/, double d_tank /*m*/,
    double& vol_total /*m3*/, double& h_tank_out /*m*/)
{
    double T_tes_ave = 0.5 * (T_tes_hot + T_tes_cold);		//[K]

    double dens_htf_ave = tes_htf_props.dens(T_tes_ave, 1.0);		//[kg/m^3] Density at average temperature
    double cp_htf_ave = tes_htf_props.Cp_ave(T_tes_cold, T_tes_hot) * 1000.0;//[J/kg-K] Specific heat at average temperature

    // Calculate Total Volume
    double dT = (T_tes_hot - T_tes_cold);   //[dK]
    vol_total = f_oversize * (Q_tes_des * 1e6 * 3600.0) /
        ((((1.0 - void_frac) * dens_solid * cp_solid) + (void_frac * dens_htf_ave * cp_htf_ave)) * dT); // [m3]

    // Calculate Height
    h_tank_out = vol_total / (CSP::pi * std::pow(d_tank / 2.0, 2.0));   // m
}

std::vector<double> C_csp_packedbed_tes::reduce_vector_avg(std::vector<double> vec, int out_vec_size)
{
    // Produce new vector of different size, using averages from 'vec'
    std::vector<double> out_vec(out_vec_size, 0);

    for (int i = 0; i < out_vec_size; i++)
    {
        // Calculate average at this location
        double frac = (double)i / (out_vec_size - 1);
        double val_avg = this->get_avg_from_vec(vec, frac);

        // Save value
        out_vec[i] = val_avg;
    }

    return out_vec;
}

double C_csp_packedbed_tes::get_avg_from_vec(std::vector<double> vec, double frac)
{
    double frac_prev = 0.0;
    double frac_next = 0.0;
    double val_prev = vec[0];
    double val_next = std::numeric_limits<double>::quiet_NaN();
    double vec_size = vec.size();
    for (int i = 1; i < vec.size(); i++)
    {
        frac_next = (double)i / (vec_size - 1);
        val_next = vec[i];
        if (frac >= frac_prev && frac <= frac_next)
        {
            break;
        }
        else
        {
            frac_prev = frac_next;
            val_prev = val_next;
        }
    }

    double val_avg = val_next + (frac - frac_next) * ((val_prev - val_next) / (frac_prev - frac_next));

    return val_avg;
}

// Public Methods

C_csp_packedbed_tes::C_csp_packedbed_tes(
    int external_fl,                                // [-] external fluid identifier
    util::matrix_t<double> external_fl_props,       // [-] external fluid properties
    double q_dot_design,                            // [MWt] Design heat rate in and out of tes
    double Q_tes_des,                               // [MWt-hr] design storage capacity
    int size_type,                                  // [] Sizing Method (0) use fixed diameter, (1) use fixed height, (2) use preset inputs
    double h_tank_in,			                    // [m] tank height input
    double d_tank_in,                               // [m] tank diameter input
    double f_oversize,                              // [] Oversize factor
    double T_cold_des_C,	                        // [C] convert to K in constructor()
    double T_hot_des_C,	                            // [C] convert to K in constructor()
    double T_tank_hot_ini_C,	                    // [C] Initial temperature in hot storage tank
    double T_tank_cold_ini_C,	                    // [C] Initial temperature in cold storage cold
    double f_V_hot_ini,                             // [%] Initial fraction of available volume that is hot
    int n_xstep,                                    // number spatial sub steps
    int n_subtimestep,                              // number subtimesteps
    double tes_pump_coef,		                    // [kW/kg/s] Pumping power to move 1 kg/s of HTF through tes loop
    double k_eff,                                   // [W/m K] Effective thermal conductivity
    double void_frac,                               // [] Packed bed void fraction
    double dens_solid,                              // [kg/m3] solid specific heat 
    double cp_solid,                                // [kJ/kg K] solid specific heat
    double T_hot_delta,                             // [C] Max allowable decrease in hot discharge temp
    double T_cold_delta,                            // [C] Max allowable increase in cold discharge temp
    double T_charge_min_C                           // [C] Min allowable charge temperature
)
    :
        m_external_fl(external_fl), m_external_fl_props(external_fl_props), m_q_dot_design(q_dot_design),  m_Q_tes_des(Q_tes_des),
        m_size_type(size_type), m_h_tank_in(h_tank_in), m_d_tank_in(d_tank_in), m_f_oversize(f_oversize),
        m_f_V_hot_ini(f_V_hot_ini),
        m_n_xstep(n_xstep), m_n_subtimestep(n_subtimestep), m_tes_pump_coef(tes_pump_coef),
        m_k_eff(k_eff), m_void_frac(void_frac), m_dens_solid(dens_solid),
        m_T_hot_delta(T_hot_delta), m_T_cold_delta(T_cold_delta)
{
    // Convert Temperature Units
    m_T_cold_des = T_cold_des_C + 273.15;           //[K]
    m_T_hot_des = T_hot_des_C + 273.15;             //[K]
    m_T_tank_hot_ini = T_tank_hot_ini_C + 273.15;   //[K]
    m_T_tank_cold_ini = T_tank_cold_ini_C + 273.15; //[K]
    m_T_charge_min = T_charge_min_C + 273.15;       //[K]

    // Convert Specific heat unit
    m_cp_solid = cp_solid * 1e3;    //[J/kg K]

    // Set Subtimestep
    m_subtimestep_nominal = 3600.0 / m_n_subtimestep;   //[s]

    m_W_dot_pumping_des = std::numeric_limits<double>::quiet_NaN();

    mc_reported_outputs.construct(S_output_info);
}

C_csp_packedbed_tes::C_csp_packedbed_tes()
{
    mc_reported_outputs.construct(S_output_info);
}

void C_csp_packedbed_tes::set_T_grad_init(std::vector<double> T_grad_init_C)
{
    for (double T_C : T_grad_init_C)
    {
        m_T_grad_init.push_back(T_C + 273.15);
        m_T_prev_vec.push_back(T_C + 273.15);
    }

    m_use_T_grad_init = true;
}

void C_csp_packedbed_tes::init(const C_csp_tes::S_csp_tes_init_inputs init_inputs)
{
    if (!(m_Q_tes_des > 0.0))
    {
        m_is_tes = false;
        m_W_dot_pumping_des = 0.0;
        return;		// No storage!
    }

    m_is_tes = true;

    // Declare instance of fluid class for EXTERNAL fluid
    // Set fluid number and copy over fluid matrix if it makes sense
    if (m_external_fl != HTFProperties::User_defined && m_external_fl < HTFProperties::End_Library_Fluids)
    {
        if (!mc_external_htfProps.SetFluid(m_external_fl))
        {
            throw(C_csp_exception("External HTF code is not recognized", "Two Tank TES Initialization"));
        }
    }
    else if (m_external_fl == HTFProperties::User_defined)
    {
        int n_rows = (int)m_external_fl_props.nrows();
        int n_cols = (int)m_external_fl_props.ncols();
        if (n_rows > 2 && n_cols == 7)
        {
            if (!mc_external_htfProps.SetUserDefinedFluid(m_external_fl_props))
            {
                error_msg = util::format(mc_external_htfProps.UserFluidErrMessage(), n_rows, n_cols);
                throw(C_csp_exception(error_msg, "Two Tank TES Initialization"));
            }
        }
        else
        {
            error_msg = util::format("The user defined external HTF table must contain at least 3 rows and exactly 7 columns. The current table contains %d row(s) and %d column(s)", n_rows, n_cols);
            throw(C_csp_exception(error_msg, "Two Tank TES Initialization"));
        }
    }
    else
    {
        throw(C_csp_exception("External HTF code is not recognized", "Two Tank TES Initialization"));
    }

    // Size Tank

    // Fixed Diameter
    if (m_size_type == 0)
    {
        double h_tank_out;
        size_pb_fixed_diameter(mc_external_htfProps, m_Q_tes_des /*MWt-hr*/, m_f_oversize,
            m_void_frac, m_dens_solid /*kg/m3*/, m_cp_solid /*J/kg K*/,
            m_T_hot_des /*K*/, m_T_cold_des /*K*/, m_d_tank_in /*m*/,
            m_V_tank /*m3*/, h_tank_out /*m*/);
        m_h_tank_calc = h_tank_out;
        m_d_tank_calc = m_d_tank_in;
    }
    // Fixed Height
    else if (m_size_type == 1)
    {
        double d_tank_out;
        size_pb_fixed_height(mc_external_htfProps, m_Q_tes_des /*MWt-hr*/, m_f_oversize,
            m_void_frac, m_dens_solid /*kg/m3*/, m_cp_solid /*J/kg K*/,
            m_T_hot_des /*K*/, m_T_cold_des /*K*/, m_h_tank_in /*m*/,
            m_V_tank /*m3*/, d_tank_out /*m*/);
        m_d_tank_calc = d_tank_out;
        m_h_tank_calc = m_h_tank_in;
    }
    // Diameter and height are defined by user
    else if (m_size_type == 2)
    {
        m_h_tank_calc = m_h_tank_in; //[m]
        m_d_tank_calc = m_d_tank_in; //[m]
    }
    else
    {
        throw(C_csp_exception("Invalid TES sizing type"));
    }

    // Define Cross Sectional Area
    m_Ac = M_PI * std::pow(0.5 * m_d_tank_calc, 2.0);

    // Back calculate packed bed mass
    m_mass_solid = m_V_tank * (1.0 - m_void_frac) * m_dens_solid;   // [kg]

    // Calculate actual storage capacity
    m_Q_tes_actual = m_Q_tes_des * m_f_oversize;    //[MWt-hr]

    // Define initial temperatures
    if (m_use_T_grad_init == false)
    {
        double dx = m_h_tank_calc / m_n_xstep;               // [m]
        double x_end = 0;   //[m]
        double x_mid = 0;   //[m]
        m_T_prev_vec = std::vector<double>(m_n_xstep + 1);
        // Loop through space
        for (int i = 0; i <= m_n_xstep; i++)
        {
            // Update position of center of section
            if (i == 0 || i == m_n_xstep)
            {
                x_end += dx * 0.5;
                x_mid = x_end - (dx * 0.5 * 0.5);
            }
            else
            {
                x_end += dx;
                x_mid = x_end - (dx * 0.5);
            }

            double frac_mid = x_mid / m_h_tank_calc;
            if(frac_mid < m_f_V_hot_ini * 0.01)
                m_T_prev_vec[i] = m_T_tank_hot_ini;
            else
                m_T_prev_vec[i] = m_T_tank_cold_ini;
        }
    }

    // Calculate thermal power to PC at design
    m_q_pb_design = m_q_dot_design * 1.E6;	//[Wt]

    m_ts_hours = m_Q_tes_des / m_q_dot_design;

    double cp_ave = mc_external_htfProps.Cp(0.5*(m_T_cold_des + m_T_hot_des));  //[kJ/kg-K]

    double m_dot_htf_cycle_des = (m_q_dot_design * 1.E-3) / (cp_ave*(m_T_hot_des - m_T_cold_des));

    m_W_dot_pumping_des = pumping_power(m_dot_htf_cycle_des);      //[MWe]
}

bool C_csp_packedbed_tes::does_tes_exist()
{
    return m_is_tes;
}

bool C_csp_packedbed_tes::is_cr_to_cold_allowed()
{
    return false;
}

double C_csp_packedbed_tes::get_hot_temp()
{
    // Return temperature closest to charge inlet (hot)
    return m_T_prev_vec[0];
}

double C_csp_packedbed_tes::get_cold_temp()
{
    // Return temperature furthest from charge inlet (cold)
    return m_T_prev_vec[m_T_prev_vec.size() - 1];
}

double C_csp_packedbed_tes::get_hot_tank_vol_frac()
{
    // Loop through temperatures, starting at hot end, to check temperature threshold
    double n_hot_sections = 0;
    for (int i = 0; i < m_T_prev_vec.size(); i++)
    {
        double T_prev_node = m_T_prev_vec[i];
        double weight = 1;

        // First and last node count for half
        if (i == 0 || i == m_T_prev_vec.size() - 1)
            weight = 0.5;

        if (T_prev_node >= (m_T_hot_des - m_T_hot_delta))
        {
            n_hot_sections += weight;
        }
    }

    // Calculate hot fraction
    // Divide by number of "sections" which is one less than number of nodes
    double hot_vol_frac = n_hot_sections / m_n_xstep;

    return hot_vol_frac;
}

double C_csp_packedbed_tes::get_initial_charge_energy()
{
    //MWh
    return m_q_pb_design * m_ts_hours * (m_f_V_hot_ini / 100.0) * 1.e-6;
    
}

double C_csp_packedbed_tes::get_min_charge_energy()
{
    //MWh
    return 0.;
}

double C_csp_packedbed_tes::get_max_charge_energy()
{
    //MWh
    return m_q_pb_design * m_ts_hours / 1.e6;
}

double C_csp_packedbed_tes::get_degradation_rate()
{
    // No heat loss in packed bed model
    return 0.0;
}

double C_csp_packedbed_tes::get_design_pumping_power() {

    return m_W_dot_pumping_des;     //[MWe]
}

void C_csp_packedbed_tes::reset_storage_to_initial_state()
{
    // Define initial temperatures
    if (m_use_T_grad_init == false)
    {
        double dx = m_h_tank_calc / m_n_xstep;               // [m]
        double x_end = 0;   //[m]
        double x_mid = 0;   //[m]
        m_T_prev_vec = std::vector<double>(m_n_xstep + 1);
        // Loop through space
        for (int i = 0; i <= m_n_xstep; i++)
        {
            // Update position of center of section
            if (i == 0 || i == m_n_xstep)
            {
                x_end += dx * 0.5;
                x_mid = x_end - (dx * 0.5 * 0.5);
            }
            else
            {
                x_end += dx;
                x_mid = x_end - (dx * 0.5);
            }

            double frac_mid = x_mid / m_h_tank_calc;
            if (frac_mid < m_f_V_hot_ini * 0.01)
                m_T_prev_vec[i] = m_T_tank_hot_ini;
            else
                m_T_prev_vec[i] = m_T_tank_cold_ini;
        }
    }
    else
    {
        m_T_prev_vec = m_T_grad_init;
    }
    return;
}

void C_csp_packedbed_tes::discharge_avail_est(double T_cold_K, double step_s,
    double& q_dot_dc_est /*MWt*/, double& m_dot_external_est /*kg/s*/, double& T_hot_external_est /*K*/)
{
    // Temperatures are at the node between segments
    // if m_n_xstep == 9, there are 10 reported temperatures
    // The first and last node represent HALF volume each

    // Calculate HTF properties
    double T_tes_ave = 0.5 * (m_T_hot_des + m_T_cold_des);		//[K]
    double dens_htf_ave = mc_external_htfProps.dens(T_tes_ave, 1.0);		//[kg/m^3] Density at average temperature
    double cp_htf_ave = mc_external_htfProps.Cp_ave(m_T_cold_des, m_T_hot_des) * 1000.0;//[J/kg-K] Specific heat at average temperature

    // Calculate Available Discharge Energy
    double dx = m_h_tank_calc / m_n_xstep;               // [m]
    double mass_packed_subsection = dx * m_Ac * (1.0 - m_void_frac) * m_dens_solid;   // [kg] Packed bed mass in subsection
    double mass_htf_subsection = dx * m_Ac * m_void_frac * dens_htf_ave;    //[kg] HTF mass in subsection

    // Loop through temperatures, starting at hot end, to check temperature threshold
    double Q_dc_avail = 0;  // [MJt]
    for(int i = 0; i < m_T_prev_vec.size(); i++)
    {
        double T_prev_node = m_T_prev_vec[i];
        double local_packed_mass = mass_packed_subsection;
        double local_htf_mass = mass_htf_subsection;

        // Mass is half if first or last node
        if (i == 0 || i == m_T_prev_vec.size() - 1)
        {
            local_packed_mass = local_packed_mass / 2.0;
            local_htf_mass = local_htf_mass / 2.0;
        }

        // Check if node is above threshold
        if (T_prev_node >= (m_T_hot_des - m_T_hot_delta))
        {
            double Q_avail_packed = local_packed_mass * m_cp_solid * (T_prev_node - T_cold_K) * 1e-6;    // [MJt]
            double Q_avail_htf = local_htf_mass * cp_htf_ave * (T_prev_node - T_cold_K) * 1e-6; // [MJt]
            Q_dc_avail += Q_avail_packed;
            Q_dc_avail += Q_avail_htf;
        }
        else
        {
            //break;
        }
    }

    // Calculate Q dot avail
    double Q_dot_dc_avail = Q_dc_avail / step_s;    // [MWt]

    // Calculate Max Mass Flow Rate
    double mdot_max = (Q_dc_avail * 1e6) / (step_s * cp_htf_ave * (m_T_hot_des - T_cold_K));  //[kg/s]

    // Set Outputs
    q_dot_dc_est = Q_dot_dc_avail;  //[MWt]
    m_dot_external_est = mdot_max;  //[kg/s]
    T_hot_external_est = m_T_prev_vec[0];   // [K] Temperature near charge inlet (hottest)

    return;
}

void C_csp_packedbed_tes::charge_avail_est(double T_hot_K, double step_s,
    double& q_dot_ch_est /*MWt*/, double& m_dot_external_est /*kg/s*/, double& T_cold_external_est /*K*/)
{
    // Temperatures are at the node between segments
    // if m_n_xstep == 9, there are 10 reported temperatures
    // The first and last node represent HALF volume each

    // First check if charge temp is hot enough
    if (T_hot_K < m_T_charge_min)
    {
        q_dot_ch_est = 0;
        m_dot_external_est = 0;
        T_cold_external_est = 0;
        return;
    }

    // Calculate HTF properties
    double T_tes_ave = 0.5 * (m_T_hot_des + m_T_cold_des);		//[K]
    double dens_htf_ave = mc_external_htfProps.dens(T_tes_ave, 1.0);		//[kg/m^3] Density at average temperature
    double cp_htf_ave = mc_external_htfProps.Cp_ave(m_T_cold_des, m_T_hot_des) * 1000.0;//[J/kg-K] Specific heat at average temperature

    // Calculate Available Charge Energy
    double dx = m_h_tank_calc / m_n_xstep;   // [m]
    double mass_packed_subsection = dx * m_Ac * (1.0 - m_void_frac) * m_dens_solid;   // [kg] Packed bed mass in subsection 
    double mass_htf_subsection = dx * m_Ac * m_void_frac * dens_htf_ave;    //[kg] HTF mass in subsection

    // Loop through temperatures, starting at cold end, to check temperature threshold
    double Q_ch_avail = 0;  //[MJt]
    double count = 0;
    for (int i = m_T_prev_vec.size() - 1; i >= 0; i--)
    {
        double T_prev_node = m_T_prev_vec[i];   //[K]
        double local_packed_mass = mass_packed_subsection;
        double local_htf_mass = mass_htf_subsection;

        // Mass is half if first or last node
        if (i == 0 || i == m_T_prev_vec.size() - 1)
        {
            local_packed_mass = local_packed_mass / 2.0;
            local_htf_mass = local_htf_mass / 2.0;
        }

        if (T_prev_node <= (m_T_cold_des + m_T_cold_delta))
        {
            double Q_avail_packed = local_packed_mass * m_cp_solid * (T_hot_K - m_T_cold_des) * 1e-6;   //[MJt]
            double Q_avail_htf = local_htf_mass * cp_htf_ave * (T_hot_K - m_T_cold_des) * 1e-6;         //[MJt]
            Q_ch_avail += Q_avail_packed;
            Q_ch_avail += Q_avail_htf;
            count++;
        }
        else
        {
            //break;
        }
    }

    // Calculate Q dot avail
    double Q_dot_ch_avail = Q_ch_avail / step_s;    // [MWt]

    // Calculate Max Mass Flow Rate
    double mdot_max = (Q_ch_avail * 1e6) / (step_s * cp_htf_ave * (T_hot_K - m_T_cold_des));  //[kg/s]

    // Set Outputs
    q_dot_ch_est = Q_dot_ch_avail;  //[MWt]
    m_dot_external_est = mdot_max;  //[kg/s]
    T_cold_external_est = m_T_prev_vec[m_T_prev_vec.size() - 1];    // [K] Temperature near charge outlet (coldest)

    return;
}

int C_csp_packedbed_tes::solve_tes_off_design(double timestep /*s*/, double  T_amb /*K*/,
    double m_dot_cr_to_cv_hot /*kg/s*/, double m_dot_cv_hot_to_sink /*kg/s*/, double m_dot_cr_to_cv_cold /*kg/s*/,
    double T_cr_out_hot /*K*/, double T_sink_out_cold /*K*/,
    double& T_sink_htf_in_hot /*K*/, double& T_cr_in_cold /*K*/,
    C_csp_tes::S_csp_tes_outputs& s_outputs)		//, C_csp_solver_htf_state & s_tes_ch_htf, C_csp_solver_htf_state & s_tes_dc_htf)
{
    // Enthalpy balance on inlet to cold cv
    double T_htf_cold_cv_in = T_sink_out_cold;     //[K]
    double m_dot_total_to_cv_cold = m_dot_cv_hot_to_sink + m_dot_cr_to_cv_cold;    //[kg/s]
    if (m_dot_total_to_cv_cold > 0.0) {
        T_htf_cold_cv_in = (m_dot_cv_hot_to_sink * T_sink_out_cold + m_dot_cr_to_cv_cold * T_cr_out_hot) / (m_dot_total_to_cv_cold);
    }

    s_outputs = S_csp_tes_outputs();

    double m_dot_cr_to_tes_hot, m_dot_cr_to_tes_cold, m_dot_tes_hot_out, m_dot_pc_to_tes_cold, m_dot_tes_cold_out, m_dot_tes_cold_in;
    m_dot_cr_to_tes_hot = m_dot_cr_to_tes_cold = m_dot_tes_hot_out = m_dot_pc_to_tes_cold = m_dot_tes_cold_out = m_dot_tes_cold_in = std::numeric_limits<double>::quiet_NaN();
    double m_dot_src_to_sink, m_dot_sink_to_src;
    m_dot_src_to_sink = m_dot_sink_to_src = std::numeric_limits<double>::quiet_NaN();

    // Receiver bypass is possible in a parallel configuration,
       //  but need to determine if it actually makes sense and how to model it
    if (m_dot_cr_to_cv_cold != 0.0) {
        throw(C_csp_exception("Receiver output to cold tank not allowed in parallel TES configuration"));
    }
    m_dot_cr_to_tes_cold = 0.0;

    if (m_dot_cr_to_cv_hot >= m_dot_cv_hot_to_sink)
    {
        m_dot_cr_to_tes_hot = m_dot_cr_to_cv_hot - m_dot_cv_hot_to_sink;		//[kg/s]
        m_dot_tes_hot_out = 0.0;							//[kg/s]
        m_dot_pc_to_tes_cold = 0.0;							//[kg/s]
        m_dot_tes_cold_out = m_dot_cr_to_tes_hot;			//[kg/s]
        m_dot_src_to_sink = m_dot_cv_hot_to_sink;		//[kg/s]
        m_dot_sink_to_src = m_dot_cv_hot_to_sink;		//[kg/s]
    }
    else
    {
        m_dot_cr_to_tes_hot = 0.0;							//[kg/s]
        m_dot_tes_hot_out = m_dot_cv_hot_to_sink - m_dot_cr_to_cv_hot;		//[kg/s]
        m_dot_pc_to_tes_cold = m_dot_tes_hot_out;			//[kg/s]
        m_dot_tes_cold_out = 0.0;							//[kg/s]
        m_dot_src_to_sink = m_dot_cr_to_cv_hot;			//[kg/s]
        m_dot_sink_to_src = m_dot_cr_to_cv_hot;			//[kg/s]
    }
    m_dot_tes_cold_in = m_dot_pc_to_tes_cold;

    double q_dot_heater = std::numeric_limits<double>::quiet_NaN();			//[MWe]  Heating power required to keep tanks at a minimum temperature
    double m_dot_cold_tank_to_hot_tank = std::numeric_limits<double>::quiet_NaN();	//[kg/s] Hot tank mass flow rate, valid for direct and indirect systems
    double W_dot_rhtf_pump = std::numeric_limits<double>::quiet_NaN();		//[MWe]  Pumping power, just for tank-to-tank in indirect storage
    double q_dot_loss = std::numeric_limits<double>::quiet_NaN();			//[MWt]  Storage thermal losses
    double q_dot_dc_to_htf = std::numeric_limits<double>::quiet_NaN();		//[MWt]  Thermal power to the HTF from storage
    double q_dot_ch_from_htf = std::numeric_limits<double>::quiet_NaN();	//[MWt]  Thermal power from the HTF to storage
    double T_hot_ave = std::numeric_limits<double>::quiet_NaN();		    //[K]    Average hot tank temperature over timestep
    double T_cold_ave = std::numeric_limits<double>::quiet_NaN();			//[K]    Average cold tank temperature over timestep
    double T_hot_final = std::numeric_limits<double>::quiet_NaN();			//[K]    Hot tank temperature at end of timestep
    double T_cold_final = std::numeric_limits<double>::quiet_NaN();			//[K]    Cold tank temperature at end of timestep

    if (m_dot_cr_to_cv_hot >= m_dot_cv_hot_to_sink) // Charging
    {
        T_sink_htf_in_hot = T_cr_out_hot;       //[K]
        double m_dot_tes_ch = m_dot_cr_to_cv_hot - m_dot_cv_hot_to_sink;    //[kg/s]
        double T_htf_tes_cold = std::numeric_limits<double>::quiet_NaN();   //[K]
        bool ch_solved = charge(timestep,
            T_amb,
            m_dot_tes_ch,
            T_cr_out_hot,
            T_htf_tes_cold,
            q_dot_heater, m_dot_cold_tank_to_hot_tank, W_dot_rhtf_pump,
            q_dot_loss, q_dot_dc_to_htf, q_dot_ch_from_htf,
            T_hot_ave, T_cold_ave, T_hot_final, T_cold_final);

        // Check if TES.charge method solved
        if (!ch_solved)
        {
            return -3;
        }

        // Enthalpy balance to calculate T_htf_cold to CR
        if (m_dot_cr_to_cv_hot == 0.0)
        {
            T_cr_in_cold = T_htf_tes_cold;       //[K]
        }
        else
        {
            T_cr_in_cold = (m_dot_tes_ch * T_htf_tes_cold + m_dot_cv_hot_to_sink * T_sink_out_cold) / m_dot_cr_to_cv_hot;       //[K]
        }
    }
    else
    {
        T_cr_in_cold = T_sink_out_cold;     //[K]
        double m_dot_tes_dc = m_dot_cv_hot_to_sink - m_dot_cr_to_cv_hot;    //[kg/s]
        double T_htf_tes_hot = std::numeric_limits<double>::quiet_NaN();
        bool is_tes_success = discharge(timestep,
            T_amb,
            m_dot_tes_dc,
            T_sink_out_cold,
            T_htf_tes_hot,
            q_dot_heater, m_dot_cold_tank_to_hot_tank, W_dot_rhtf_pump,
            q_dot_loss, q_dot_dc_to_htf, q_dot_ch_from_htf,
            T_hot_ave, T_cold_ave, T_hot_final, T_cold_final);

        m_dot_cold_tank_to_hot_tank *= -1.0;

        // Check if discharge method solved
        if (!is_tes_success)
        {
            return -4;
        }

        T_sink_htf_in_hot = (m_dot_tes_dc * T_htf_tes_hot + m_dot_cr_to_cv_hot * T_cr_out_hot) / m_dot_cv_hot_to_sink;   //[K]
    }

    // No need to call pumping_power because pumping power is same as solved in charge/discharge
    /*double W_dot_htf_pump = pumping_power(m_dot_cr_to_cv_hot, m_dot_cv_hot_to_sink, std::abs(m_dot_cold_tank_to_hot_tank),
        T_cr_in_cold, T_cr_out_hot, T_sink_htf_in_hot, T_sink_out_cold,
        false);*/

    // Set values not part of packed bed model
    s_outputs.m_q_heater = q_dot_heater;                        //[MWe] Heater
    s_outputs.m_W_dot_elec_in_tot = W_dot_rhtf_pump;            //[MWe] Use this pumping power?

    s_outputs.m_q_dot_dc_to_htf = q_dot_dc_to_htf;              //[MWt] Thermal power to the HTF from storage
    s_outputs.m_q_dot_ch_from_htf = q_dot_ch_from_htf;          //[MWt]  Thermal power from the HTF to storage
    s_outputs.m_m_dot_cr_to_tes_hot = m_dot_cr_to_tes_hot;		//[kg/s]
    s_outputs.m_m_dot_cr_to_tes_cold = m_dot_cr_to_tes_cold;    //[kg/s]
    s_outputs.m_m_dot_tes_hot_out = m_dot_tes_hot_out;			//[kg/s]
    s_outputs.m_m_dot_pc_to_tes_cold = m_dot_pc_to_tes_cold;	//[kg/s]
    s_outputs.m_m_dot_tes_cold_out = m_dot_tes_cold_out;		//[kg/s]
    s_outputs.m_m_dot_tes_cold_in = m_dot_tes_cold_in;          //[kg/s]
    s_outputs.m_m_dot_src_to_sink = m_dot_src_to_sink;	        //[kg/s]
    s_outputs.m_m_dot_sink_to_src = m_dot_sink_to_src;	        //[kg/s]

    s_outputs.m_T_tes_cold_in = T_htf_cold_cv_in;       //[K]

    s_outputs.m_m_dot_cold_tank_to_hot_tank = m_dot_cold_tank_to_hot_tank;

    mc_reported_outputs.value(E_Q_DOT_LOSS, q_dot_loss);		//[MWt]
    mc_reported_outputs.value(E_W_DOT_HEATER, q_dot_heater);		//[MWt]
    mc_reported_outputs.value(E_TES_T_HOT, T_hot_final - 273.15);	//[C]
    mc_reported_outputs.value(E_TES_T_COLD, T_cold_final - 273.15);	//[C]
    mc_reported_outputs.value(E_M_DOT_TANK_TO_TANK, m_dot_cold_tank_to_hot_tank);	//[kg/s]
    //mc_reported_outputs.value(E_MASS_COLD_TANK, mc_cold_tank.get_m_m_calc());		//[kg]
    //mc_reported_outputs.value(E_MASS_HOT_TANK, mc_hot_tank.get_m_m_calc());			//[kg]
    mc_reported_outputs.value(E_W_DOT_HTF_PUMP, W_dot_rhtf_pump);    //[MWe]
    //mc_reported_outputs.value(E_VOL_TOT, vol_total);    //[m3]
    //mc_reported_outputs.value(E_MASS_TOT, mc_cold_tank.get_m_m_calc() + mc_hot_tank.get_m_m_calc());    //[m3]
    mc_reported_outputs.value(E_VOL_TOT, m_V_tank);    //[m3]
    mc_reported_outputs.value(E_MASS_TOT, m_mass_solid);    //[m3]

    // Report average thermocline temperatures
    std::vector<double> reduced_T_vec = reduce_vector_avg(m_T_calc_vec, 10);    //[K]
    mc_reported_outputs.value(E_T_GRAD_0, reduced_T_vec[0] - 273.15);		    //[C]
    mc_reported_outputs.value(E_T_GRAD_1, reduced_T_vec[1] - 273.15);		    //[C]
    mc_reported_outputs.value(E_T_GRAD_2, reduced_T_vec[2] - 273.15);		    //[C]
    mc_reported_outputs.value(E_T_GRAD_3, reduced_T_vec[3] - 273.15);		    //[C]
    mc_reported_outputs.value(E_T_GRAD_4, reduced_T_vec[4] - 273.15);		    //[C]
    mc_reported_outputs.value(E_T_GRAD_5, reduced_T_vec[5] - 273.15);		    //[C]
    mc_reported_outputs.value(E_T_GRAD_6, reduced_T_vec[6] - 273.15);		    //[C]
    mc_reported_outputs.value(E_T_GRAD_7, reduced_T_vec[7] - 273.15);		    //[C]
    mc_reported_outputs.value(E_T_GRAD_8, reduced_T_vec[8] - 273.15);		    //[C]
    mc_reported_outputs.value(E_T_GRAD_9, reduced_T_vec[9] - 273.15);		    //[C]

    return 0;
}

void C_csp_packedbed_tes::converged()
{
    m_T_prev_vec = m_T_calc_vec;

    mc_reported_outputs.value(E_HOT_TANK_HTF_PERC_FINAL, get_hot_tank_vol_frac() * 100.0);

    mc_reported_outputs.set_timestep_outputs();

    return;
}

void C_csp_packedbed_tes::write_output_intervals(double report_time_start,
    const std::vector<double>& v_temp_ts_time_end, double report_time_end)
{
    mc_reported_outputs.send_to_reporting_ts_array(report_time_start,
        v_temp_ts_time_end, report_time_end);
}

void C_csp_packedbed_tes::assign(int index, double* p_reporting_ts_array, size_t n_reporting_ts_array)
{
    mc_reported_outputs.assign(index, p_reporting_ts_array, n_reporting_ts_array);
}

void C_csp_packedbed_tes::get_design_parameters(double& vol_one_temp_avail /*m3*/, double& vol_one_temp_total /*m3*/,
    double& h_tank_calc /*m*/, double& d_tank_calc /*m*/,
    double& q_dot_loss_des /*MWt*/, double& dens_store_htf_at_T_ave /*kg/m3*/, double& Q_tes /*MWt-hr*/)
{
    vol_one_temp_avail = m_V_tank;  //[m3]
    vol_one_temp_total = m_V_tank;  //[m3]
    h_tank_calc = m_h_tank_calc;    //[m]
    d_tank_calc = m_d_tank_calc;    //[m]
    q_dot_loss_des = 0.0;           //[MWt]
    dens_store_htf_at_T_ave = m_dens_solid; //[kg/m3] This is just the density of the solid media (?)
    Q_tes = m_Q_tes_actual;         //[MWt-hr]

    return;
}

bool C_csp_packedbed_tes::charge(double timestep /*s*/, double T_amb /*K*/, double m_dot_htf_in /*kg/s*/,
    double T_htf_hot_in /*K*/, double& T_htf_cold_out /*K*/,
    double& q_dot_heater /*MWe*/, double& m_dot_tank_to_tank /*kg/s*/, double& W_dot_rhtf_pump /*MWe*/,
    double& q_dot_loss /*MWt*/, double& q_dot_dc_to_htf /*MWt*/, double& q_dot_ch_from_htf /*MWt*/,
    double& T_hot_ave /*K*/, double& T_cold_ave /*K*/, double& T_hot_final /*K*/, double& T_cold_final /*K*/
)
{
    

    // Calculate subtimestep
    int N_subtimesteps_local = (int)std::ceil(timestep / m_subtimestep_nominal);
    double dt = timestep / (double)N_subtimesteps_local;    //[s]

    // Define timestep and spatial step
    double dx = m_h_tank_calc / m_n_xstep;           // [m]

    

    // Initialize Temperature Vectors
    std::vector<double> T_calc_vec(m_n_xstep + 1);          // [K] Temperature gradient at end of subtimestep
    std::vector<double> T_prev_vec_subtime = m_T_prev_vec;  // [K] Temperature gradient at beginning of subtimestep
    std::vector<double> T_out_vec(N_subtimesteps_local, 0.0);    // [K] Temperature at Outlet
    std::vector<double> T_hot_vec(N_subtimesteps_local, 0.0);    // [K] Temperature closest to inlet

    // Loop through subtimesteps
    for (int n = 0; n < N_subtimesteps_local; n++)
    {
        // Loop through space
        for (int i = 0; i <= m_n_xstep; i++)
        {
            // HTF Properties
            double dens_fluid = mc_external_htfProps.dens(T_prev_vec_subtime[i], 1);   //[kg/m3]
            double cp_fluid = mc_external_htfProps.Cp(T_prev_vec_subtime[i]) * 1.E3;        //[J/kg-K]

            // Calculate Coefficients (assume constant for now)
            double cp_eff = m_void_frac * dens_fluid * cp_fluid
                + (1.0 - m_void_frac) * m_dens_solid * m_cp_solid;  // [J/m3 K]
            double u0 = (m_dot_htf_in / m_Ac) / dens_fluid;          // [kg/s m3] --> [m/s]
            double alpha = (dens_fluid * u0 * cp_fluid * dt) / (cp_eff * dx);
            double beta = (m_k_eff * dt) / (cp_eff * std::pow(dx, 2.0));

            // Charge INLET
            if (i == 0)
            {
                double T_in_prev = T_prev_vec_subtime[i];   // Replace T_htf_in with current node previous timestep
                /*if (m_dot_htf_in > 0)
                    T_in_dummy = T_htf_hot_in;*/
                T_calc_vec[i] = (T_prev_vec_subtime[0] + (alpha * T_htf_hot_in)
                    + (beta * (T_prev_vec_subtime[i + 1] - (2.0 * T_prev_vec_subtime[i]) + T_in_prev)))
                    / (1.0 + alpha);
            }

            // Charge OUTLET
            else if (i == m_n_xstep)
            {

                double T_out_prev = T_prev_vec_subtime[i - 1];  // Replace T_prev_vec_subtime[i - 2] with current node previous timestep
                T_calc_vec[i] = (T_prev_vec_subtime[i] + (alpha * T_calc_vec[i - 1])
                    + (beta * (T_prev_vec_subtime[i - 1] - T_prev_vec_subtime[i])))
                    / (1.0 + alpha);
            }
            else
            {
                T_calc_vec[i] = (T_prev_vec_subtime[i] + (alpha * T_calc_vec[i - 1])
                    + (beta * (T_prev_vec_subtime[i + 1] - 2.0 * T_prev_vec_subtime[i] + T_prev_vec_subtime[i - 1])))
                    / (1.0 + alpha);
            }
        }

        // Reset Prev Vec
        T_prev_vec_subtime = T_calc_vec;

        // Save outlet temp
        T_out_vec[n] = T_calc_vec[m_n_xstep];

        // Save temp closest to inlet
        T_hot_vec[n] = T_calc_vec[0];
    }

    m_T_calc_vec = T_calc_vec;

    // Calculate Time Average Outlet Temp
    double T_out_avg = std::accumulate(T_out_vec.begin(), T_out_vec.end(), 0.0) / T_out_vec.size(); //[K]

    // Calculate Time Average Closest to Inlet Temp
    double T_hot_avg = std::accumulate(T_hot_vec.begin(), T_hot_vec.end(), 0.0) / T_hot_vec.size(); //[K]

    // No Heater (for now)
    q_dot_heater = 0.0;     //[MWt]

    // No tank to tank mass (only one tank)
    m_dot_tank_to_tank = std::numeric_limits<double>::quiet_NaN();  //[kg/s]

    // Pumping Power
    W_dot_rhtf_pump = pumping_power(m_dot_htf_in);    //[MWe] Pumping power through tank

    // Cold out = Average Outlet Temp
    T_htf_cold_out = T_out_avg; //[K]

    // Heat Loss
    q_dot_loss = 0;  //[MWt]

    // Heat transferred from storage to htf
    q_dot_dc_to_htf = 0.0;  //[MWt] <- should this be calculated?

    // Average Hot Temperature in Tank
    T_hot_ave = T_hot_avg;  //[K] Average temperature in tank location closest to inlet

    // Average Cold outlet temperature
    T_cold_ave = T_out_avg;    //[K] 

    // Final Hot Temperature
    T_hot_final = T_hot_vec[T_hot_vec.size() - 1];  // [K]

    // Final Cold Temperature
    T_cold_final = T_out_vec[T_out_vec.size() - 1]; // [K]

    // Charge power from htf to storage
    q_dot_ch_from_htf = 0.0;    // [MWt]
    double cp_fluid_avg = mc_external_htfProps.Cp_ave(m_T_cold_des, m_T_hot_des) * 1.E3; //[J/kg-K]
    for (double T_out : T_out_vec)
    {
        q_dot_ch_from_htf += m_dot_htf_in * cp_fluid_avg * (T_htf_hot_in - T_out) * 1.E-6 * (dt / timestep);  // [MWt]
    }

    return true;
}

bool C_csp_packedbed_tes::discharge(double timestep /*s*/, double T_amb /*K*/, double m_dot_htf_in /*kg/s*/,
    double T_htf_cold_in /*K*/, double& T_htf_hot_out /*K*/,
    double& q_dot_heater /*MWe*/, double& m_dot_tank_to_tank /*kg/s*/, double& W_dot_rhtf_pump /*MWe*/,
    double& q_dot_loss /*MWt*/, double& q_dot_dc_to_htf /*MWt*/, double& q_dot_ch_from_htf /*MWt*/,
    double& T_hot_ave /*K*/, double& T_cold_ave /*K*/, double& T_hot_final /*K*/, double& T_cold_final /*K*/
)
{
    // Calculate subtimestep
    int N_subtimesteps_local = (int)std::ceil(timestep / m_subtimestep_nominal);
    double dt = timestep / (double)N_subtimesteps_local;    //[s]

    // Define timestep and spatial step
    double dx = m_h_tank_calc / m_n_xstep;               // [m]

    // Initialize Temperature Vectors
    std::vector<double> T_calc_vec(m_n_xstep + 1);              // [K] Temperature gradient at end of subtimestep
    std::vector<double> T_prev_vec_subtime = m_T_prev_vec;      // [K] Temperature gradient at beginning of subtimestep
    std::vector<double> T_out_vec(N_subtimesteps_local, 0.0);        // [K] Temperature at Outlet
    std::vector<double> T_cold_vec(N_subtimesteps_local, 0.0);       // [K] Temperature closest to inlet (cold)

    // Loop through subtimesteps
    for (int n = 0; n < N_subtimesteps_local; n++)
    {
        // Loop through space
        for (int i = m_n_xstep; i >= 0; i--)
        {
            // HTF Properties
            double dens_fluid = mc_external_htfProps.dens(T_prev_vec_subtime[i], 1);   //[kg/m3]
            double cp_fluid = mc_external_htfProps.Cp(T_prev_vec_subtime[i]) * 1.E3;        //[J/kg-K]

            // Calculate Coefficients (assume constant for now)
            double cp_eff = m_void_frac * dens_fluid * cp_fluid
                + (1.0 - m_void_frac) * m_dens_solid * m_cp_solid;  // [J/m3 K]
            double u0 = (m_dot_htf_in / m_Ac) / dens_fluid;          // [kg/s m3] --> [m/s]
            double alpha = (dens_fluid * u0 * cp_fluid * dt) / (cp_eff * dx);
            double beta = (m_k_eff * dt) / (cp_eff * std::pow(dx, 2.0));

            // Discharge OUTLET
            if (i == 0)
            {
                T_calc_vec[i] = (T_prev_vec_subtime[i] + (alpha * T_calc_vec[i + 1])
                    + (beta * (T_prev_vec_subtime[i + 1] - T_prev_vec_subtime[i])))
                    / (1.0 + alpha);
            }

            // Discharge INLET
            else if (i == m_n_xstep)
            {
                double T_in_prev = T_prev_vec_subtime[i];   // Replace T_htf_in with current node previous timestep
                T_calc_vec[i] = (T_prev_vec_subtime[i] + (alpha * T_htf_cold_in)
                    + (beta * (T_prev_vec_subtime[i - 1] - (2.0 * T_prev_vec_subtime[i]) + T_in_prev)))
                    / (1.0 + alpha);
            }

            // Middle space
            else
            {
                T_calc_vec[i] = (T_prev_vec_subtime[i] + (alpha * T_calc_vec[i + 1])
                    + (beta * (T_prev_vec_subtime[i - 1] - 2.0 * T_prev_vec_subtime[i] + T_prev_vec_subtime[i + 1])))
                    / (1.0 + alpha);
            }
        }

        // Reset Prev Vec
        T_prev_vec_subtime = T_calc_vec;

        // Save outlet temp
        T_out_vec[n] = T_calc_vec[0];

        // Save Temperature closest to inlet (cold temp)
        T_cold_vec[n] = T_calc_vec[T_calc_vec.size() - 1];
    }

    m_T_calc_vec = T_calc_vec;

    // Calculate Time Average Outlet Temp (hot)
    double T_out_avg = std::accumulate(T_out_vec.begin(), T_out_vec.end(), 0.0) / T_out_vec.size();

    // Calculate Time Average Cold Temp (closest to inlet)
    double T_cold_avg = std::accumulate(T_cold_vec.begin(), T_cold_vec.end(), 0.0) / T_cold_vec.size();

    // No Heater (for now)
    q_dot_heater = 0.0;     //[MWt]

    // No tank to tank mass (only one tank)
    m_dot_tank_to_tank = std::numeric_limits<double>::quiet_NaN();  //[kg/s]

    // Pumping Power
    W_dot_rhtf_pump = pumping_power(m_dot_htf_in);    //[MWe] Pumping power through tank

    // Cold out = Average Outlet Temp
    T_htf_hot_out = T_out_avg; //[K]

    // Heat Loss (need to calculate)
    q_dot_loss = 0;  //[MWt]

    // Heat transferred from storage to htf
    q_dot_dc_to_htf = 0.0;  //[MWt]
    double cp_fluid_avg = mc_external_htfProps.Cp_ave(m_T_cold_des, m_T_hot_des) * 1.E3; //[J/kg-K]
    for (double T_out : T_out_vec)
    {
        q_dot_dc_to_htf += m_dot_htf_in * cp_fluid_avg * (T_out - T_htf_cold_in) * 1.E-6 * (dt / timestep);  // [MWt]
    }

    // Average Hot Temperature in Tank
    T_hot_ave = T_out_avg;  //[K] Average temperature in tank location closest to inlet

    // Average Cold outlet temperature
    T_cold_ave = T_cold_avg;    //[K] 

    // Final Hot Temperature
    T_hot_final = T_out_vec[T_out_vec.size() - 1];  // [K]

    // Final Cold Temperature
    T_cold_final = T_cold_vec[T_cold_vec.size() - 1]; // [K]

    // Charge power from htf to storage
    q_dot_ch_from_htf = 0.0;    // [MWt]

    return true;
}

double C_csp_packedbed_tes::pumping_power(double m_dot_htf /*kg/s*/) {

    return m_dot_htf * m_tes_pump_coef * 1.E-3;     //[MWe] pumping power through packed bed
}
