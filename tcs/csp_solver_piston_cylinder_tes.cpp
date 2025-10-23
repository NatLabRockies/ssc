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


#include "csp_solver_piston_cylinder_tes.h"

static C_csp_reported_outputs::S_output_info S_output_info[] =
{
    {C_csp_piston_cylinder_tes::E_Q_DOT_LOSS, C_csp_reported_outputs::TS_WEIGHTED_AVE},		//[MWt] TES thermal losses
    {C_csp_piston_cylinder_tes::E_W_DOT_HEATER, C_csp_reported_outputs::TS_WEIGHTED_AVE},		//[MWe] TES freeze protection power
    {C_csp_piston_cylinder_tes::E_TES_T_HOT, C_csp_reported_outputs::TS_LAST},					//[C] TES final hot tank temperature
    {C_csp_piston_cylinder_tes::E_TES_T_COLD, C_csp_reported_outputs::TS_LAST},				//[C] TES cold temperature at end of timestep      
    {C_csp_piston_cylinder_tes::E_M_DOT_TANK_TO_TANK, C_csp_reported_outputs::TS_WEIGHTED_AVE},		//[MWt] TES thermal losses
    {C_csp_piston_cylinder_tes::E_MASS_COLD_TANK, C_csp_reported_outputs::TS_LAST},			//[kg] Mass in cold tank at end of timestep		
    {C_csp_piston_cylinder_tes::E_MASS_HOT_TANK, C_csp_reported_outputs::TS_LAST},				//[kg] Mass in hot tank at end of timestep
    {C_csp_piston_cylinder_tes::E_HOT_TANK_HTF_PERC_FINAL, C_csp_reported_outputs::TS_LAST},	//[%] Final percent fill of available hot tank mass
    {C_csp_piston_cylinder_tes::E_W_DOT_HTF_PUMP, C_csp_reported_outputs::TS_WEIGHTED_AVE},	//[MWe] 

    {C_csp_piston_cylinder_tes::E_VOL_COLD, C_csp_reported_outputs::TS_LAST},	//[m3]
    {C_csp_piston_cylinder_tes::E_VOL_HOT, C_csp_reported_outputs::TS_LAST},	    //[m3]
    {C_csp_piston_cylinder_tes::E_VOL_TOT, C_csp_reported_outputs::TS_LAST},	    //[m3]
    {C_csp_piston_cylinder_tes::E_PIST_LOC, C_csp_reported_outputs::TS_LAST},	//[m]
    {C_csp_piston_cylinder_tes::E_PIST_FRAC, C_csp_reported_outputs::TS_LAST},	//[]
    {C_csp_piston_cylinder_tes::E_COLD_FRAC, C_csp_reported_outputs::TS_LAST},	//[]
    {C_csp_piston_cylinder_tes::E_MASS_TOT, C_csp_reported_outputs::TS_LAST},	//[kg]
    {C_csp_piston_cylinder_tes::E_SA_COLD, C_csp_reported_outputs::TS_LAST},	//[kg]
    {C_csp_piston_cylinder_tes::E_SA_HOT, C_csp_reported_outputs::TS_LAST},	//[kg]
    {C_csp_piston_cylinder_tes::E_SA_TOT, C_csp_reported_outputs::TS_LAST},	//[kg]
    {C_csp_piston_cylinder_tes::E_ERROR, C_csp_reported_outputs::TS_WEIGHTED_AVE},   //[MWt]
    {C_csp_piston_cylinder_tes::E_ERROR_PERCENT, C_csp_reported_outputs::TS_LAST},   //[%]
    {C_csp_piston_cylinder_tes::E_LEAK_ERROR, C_csp_reported_outputs::TS_WEIGHTED_AVE},	//[MWt]
    {C_csp_piston_cylinder_tes::E_E_HOT, C_csp_reported_outputs::TS_LAST},	//[MJ]
    {C_csp_piston_cylinder_tes::E_E_COLD, C_csp_reported_outputs::TS_LAST},	//[MJ]
    {C_csp_piston_cylinder_tes::E_WALL_ERROR, C_csp_reported_outputs::TS_WEIGHTED_AVE},	//[MW]
    {C_csp_piston_cylinder_tes::E_ERROR_CORRECTED, C_csp_reported_outputs::TS_WEIGHTED_AVE},	//[MW]
    {C_csp_piston_cylinder_tes::E_EXP_WALL_MASS, C_csp_reported_outputs::TS_LAST},	//[kg]
    {C_csp_piston_cylinder_tes::E_EXP_LENGTH, C_csp_reported_outputs::TS_LAST},	//[m]
    csp_info_invalid
};


C_storage_tank_dynamic_cyl::C_storage_tank_dynamic_cyl()
{
    m_V_prev = m_T_prev = m_m_prev = m_E_prev =

        m_V_total = m_V_active = m_V_inactive = 

        m_T_htr = m_max_q_htr = m_radius =
        m_T_design = m_mass_total = m_mass_inactive = m_mass_active = std::numeric_limits<double>::quiet_NaN();
}

void C_storage_tank_dynamic_cyl::init(HTFProperties htf_class_in, double V_tank /*m3*/,
    double h_tank /*m*/, double h_min /*m*/, double u_tank /*W/m2-K*/,
    double tank_pairs /*-*/, double T_htr /*K*/, double max_q_htr /*MWt*/,
    double V_ini /*m3*/, double T_ini /*K*/,
    double T_design /*K*/,
    double tank_wall_cp,            // [J/kg-K] Tank wall specific heat
    double tank_wall_dens,          // [kg/m3] Tank wall density
    double tank_wall_thick,         // [m] Tank wall thickness)
    double nstep,                    // [] Number of time steps for energy balance iteration
    std::vector<double> piston_loss_poly //[] Coefficients to piston loss polynomial
    )
{
    mc_htf = htf_class_in;

    double rho_des = mc_htf.dens(T_design, 1.0);		//[kg/m^3] Density at average temperature

    m_V_total = V_tank;				//[m^3]

    m_mass_total = m_V_total * rho_des;			//[kg]

    m_V_inactive = m_V_total * h_min / h_tank;	//[m^3]

    m_mass_inactive = m_V_inactive * rho_des;	//[kg]

    m_V_active = m_V_total - m_V_inactive;		//[m^3]

    m_mass_active = m_mass_total - m_mass_inactive;	//[kg]

    double A_cs = m_V_total / (h_tank * tank_pairs);		//[m^2] Cross-sectional area of a single tank

    double diameter = pow(A_cs / CSP::pi, 0.5) * 2.0;		//[m] Diameter of a single tank

    m_radius = diameter / 2.0;  // [m] radius
    m_tank_wall_thick = tank_wall_thick; // [m] tank wall thickness
    m_tank_wall_dens = tank_wall_dens;  //[kg/m3]
    m_tank_wall_cp = tank_wall_cp;      //[J/kgK]
    m_nstep = nstep;    //[]
    m_piston_loss_poly = piston_loss_poly;  //[]

    // Calculate tank conductance
    m_u_tank = u_tank;

    m_T_htr = T_htr;
    m_max_q_htr = max_q_htr;

    m_V_prev = V_ini;
    m_T_prev = T_ini;
    m_m_prev = calc_mass_at_prev();
    m_m_wall_prev = calc_mass_wall(m_T_prev, m_m_prev);
}

double C_storage_tank_dynamic_cyl::calc_mass_at_prev()
{
    return m_V_prev * mc_htf.dens(m_T_prev, 1.0);	//[kg] 
}

double C_storage_tank_dynamic_cyl::get_m_UA()
{
    // Calculate UA value
    double SA_prev = calc_SA(m_V_calc);
    double UA = m_u_tank * SA_prev;

    return UA;            //[W/K]
}

double C_storage_tank_dynamic_cyl::get_m_T_prev()
{
    return m_T_prev;		//[K]
}

double C_storage_tank_dynamic_cyl::get_m_T_calc()
{
    return m_T_calc;
}

double C_storage_tank_dynamic_cyl::get_m_m_calc() // Get Current FLUID mass
{
    return m_m_calc;    //[kg]
}

double C_storage_tank_dynamic_cyl::get_m_m_prev() // Get Previous FLUID mass
{
    return m_m_prev;    //[kg]
}

double C_storage_tank_dynamic_cyl::get_m_E_prev()
{
    return m_E_prev;    //[MJ]
}

double C_storage_tank_dynamic_cyl::get_m_E_calc()
{
    return m_E_calc;    //[MJ]
}

double C_storage_tank_dynamic_cyl::get_vol_frac()
{
    return (m_V_prev - m_V_inactive) / m_V_active;
}

double C_storage_tank_dynamic_cyl::get_mass_avail()
{
    return std::max(m_m_prev - m_mass_inactive, 0.0);		//[kg]
}

double C_storage_tank_dynamic_cyl::get_fluid_vol()
{
    return m_V_calc;
}

double C_storage_tank_dynamic_cyl::get_fluid_vol_prev()
{
    return m_V_prev;
}

double C_storage_tank_dynamic_cyl::get_radius()
{
    return m_radius;
}

double C_storage_tank_dynamic_cyl::get_SA_calc()
{
    return m_SA_calc;
}

double C_storage_tank_dynamic_cyl::calc_mass_wall(double T_fluid, double mass_fluid)
{
    double rho_fluid = mc_htf.dens(T_fluid, 0); // kg/m3
    double V_fluid = mass_fluid / rho_fluid;    // m3
    double Ac_fluid = CSP::pi * std::pow(m_radius, 2.0);    // m2
    double L_fluid = V_fluid / Ac_fluid;        // m

    double Ac_wall = (CSP::pi * std::pow(m_radius + m_tank_wall_thick, 2.0)) - (CSP::pi * std::pow(m_radius, 2.0));
    double V_wall = L_fluid * Ac_wall;
    double mass_wall = V_wall * m_tank_wall_dens;

    return mass_wall;
}

double C_storage_tank_dynamic_cyl::get_m_m_wall_prev()
{
    return m_m_wall_prev;
}

double C_storage_tank_dynamic_cyl::get_m_m_wall_calc()
{
    return m_m_wall_calc;
}

double C_storage_tank_dynamic_cyl::get_m_L_calc()
{
    return m_L_calc;
}

double C_storage_tank_dynamic_cyl::m_dot_available(double f_unavail, double timestep)
{
    //double rho = mc_htf.dens(m_T_prev, 1.0);		//[kg/m^3]
    //double V = m_m_prev / rho;						//[m^3] Volume available in tank (one temperature)
    //double V_avail = fmax(V - m_V_inactive, 0.0);	//[m^3] Volume that is active - need to maintain minimum height (corresponding m_V_inactive)

    double mass_avail = get_mass_avail();		//[kg]
    double m_dot_avail = std::max(mass_avail - m_mass_active * f_unavail, 0.0) / timestep;	//[kg/s]

    // "Unavailable" fraction now applied to one temperature tank volume, not total tank volume
    //double m_dot_avail = fmax(V_avail - m_V_active*f_unavail, 0.0)*rho / timestep;		//[kg/s] Max mass flow rate available

    return m_dot_avail;		//[kg/s]
}

void C_storage_tank_dynamic_cyl::converged()
{
    // Reset 'previous' timestep values to 'calculated' values
    m_V_prev = m_V_calc;		//[m^3]
    m_T_prev = m_T_calc;		//[K]
    m_m_prev = m_m_calc;		//[kg]
    m_SA_prev = m_SA_calc;      //[m2]
    m_E_prev = m_E_calc;        //[MJ]
    m_m_wall_prev = m_m_wall_calc;  //[kg]
}

void C_storage_tank_dynamic_cyl::energy_balance_core(double timestep /*s*/, double mdot_fluid_in_before_leak /*kg/s*/, double mdot_fluid_out_before_leak /*kg/s*/,
    double T_fluid_in /*K*/, double T_amb /*K*/, double mass_fluid_prev_inner /*kg*/,
    double T_tank_in /*K*/, double T_prev_inner /*K*/,
    double T_leak_in /*K*/,
    double& T_ave /*K*/, double& q_heater /*MW*/, double& q_dot_loss /*MW*/,
    double& mass_fluid_calc_inner /*kg*/, double& T_calc_inner /*K*/, double& q_dot_out /*MW*/,
    double& q_dot_error_inner /*MW*/)
{
    // Get properties from tank state at the end of last time step
    double rho_fluid_prev = mc_htf.dens(T_prev_inner, 1.0);	//[kg/m^3]
    double cp_fluid_prev = mc_htf.Cp(T_prev_inner) * 1000.0;		//[J/kg-K] spec heat, convert from kJ/kg-K

    // Calculate Leakage
    double mdot_leak_in;
    double mdot_leak_out;
    {
        double leak_frac_in = calc_leakage_fraction(mdot_fluid_out_before_leak);
        double leak_frac_out = calc_leakage_fraction(mdot_fluid_in_before_leak);

        mdot_leak_in = leak_frac_in * mdot_fluid_out_before_leak;
        mdot_leak_out = leak_frac_out * mdot_fluid_in_before_leak;
    }

    // Calculate Net Mass Flows
    double mdot_fluid_in_net = mdot_fluid_in_before_leak + mdot_leak_in;
    double mdot_fluid_out_net = mdot_fluid_out_before_leak + mdot_leak_out;

    // Get Fluid Beginning volume
    double V_prev_inner = mass_fluid_prev_inner / rho_fluid_prev;     // [m3]

    // Calculate Fluid ending volume levels
    mass_fluid_calc_inner = mass_fluid_prev_inner + timestep * (mdot_fluid_in_net - mdot_fluid_out_net);	//[kg] Available mass at the end of this timestep

    double mass_fluid_min, mdot_fluid_out_adj;
    bool tank_is_empty = false;

    mass_fluid_min = 0.00;                              //[kg] minimum tank mass for use in the calculations
    if (mass_fluid_calc_inner < mass_fluid_min) {
        mass_fluid_calc_inner = mass_fluid_min;
        tank_is_empty = true;
        mdot_fluid_out_adj = mdot_fluid_in_net - (mass_fluid_min - mass_fluid_prev_inner) / timestep;
    }
    else {
        mdot_fluid_out_adj = mdot_fluid_out_net;
    }
    m_V_calc = mass_fluid_calc_inner / rho_fluid_prev;					//[m^3] Available volume at end of timestep (using initial temperature...)
    m_m_calc = mass_fluid_calc_inner;

    // Check for continual empty tank
    if (mass_fluid_prev_inner <= 1e-4 && tank_is_empty == true) {
        if (mdot_fluid_in_net > 0) {
            T_calc_inner = T_ave = T_fluid_in;
        }
        else {
            T_calc_inner = T_ave = T_prev_inner;
        }
        q_dot_loss = m_V_calc = mass_fluid_calc_inner = q_heater = 0.;
        return;
    }

    double diff_m_dot = mdot_fluid_in_net - mdot_fluid_out_adj;   //[kg/s]
    if (diff_m_dot >= 0.0)
    {
        diff_m_dot = std::max(diff_m_dot, 1.E-5);
    }
    else
    {
        diff_m_dot = std::min(diff_m_dot, -1.E-5);
    }

    // Calculate Cross Sectional Areas
    double Ac_fluid = CSP::pi * std::pow(m_radius, 2.0);
    double Ac_wall = (CSP::pi * std::pow(m_radius + m_tank_wall_thick, 2.0)) - (CSP::pi * std::pow(m_radius, 2.0));

    // Calculate Lengths
    double L_prev = V_prev_inner / Ac_fluid;
    m_L_calc = m_V_calc / Ac_fluid;
    double L_change = (m_V_calc - V_prev_inner) / Ac_fluid;
    double L_change_validate = m_L_calc - L_prev;

    // Calculate Beginning and End Wall Volumes
    double V_wall_prev = L_prev * Ac_wall;
    double V_wall_calc = m_L_calc * Ac_wall;
    double V_wall_change = L_change * Ac_wall;

    // Calculate Wall Mass
    double mass_wall_prev = m_tank_wall_dens * V_wall_prev;
    double mass_wall_calc = m_tank_wall_dens * V_wall_calc;
    double mass_wall_change = m_tank_wall_dens * V_wall_change;
    double mdot_wall_change = mass_wall_change / timestep;
    m_m_wall_calc = mass_wall_calc;

    // Calculate Wall Surface Areas
    double SA_wall_prev = L_prev * 2.0 * CSP::pi * (m_radius + m_tank_wall_thick);
    m_SA_calc = m_L_calc * 2.0 * CSP::pi * (m_radius + m_tank_wall_thick);
    double SA_wall_change = L_change * 2.0 * CSP::pi * (m_radius + m_tank_wall_thick);


    double mdot_in_wall = 0; // [kg/s] mass gained from expanding
    double mdot_out_wall = 0; // [kg/s] mass lost from contracting

    if (mdot_wall_change > 0)
    {
        mdot_in_wall = mdot_wall_change;
        mdot_out_wall = 0;
    }
    else if (mdot_wall_change < 0)
    {
        mdot_in_wall = 0;
        mdot_out_wall = std::abs(mdot_wall_change);
    }

    double T_in_weighted;
    double cp_in_weighted;
    double cp_out_weighted;
    double cp_bulk_weighted_calc;
    double mdot_in_total;
    double mdot_out_total;

    // If Fluid is coming in (leak going out)
    if((mdot_fluid_in_before_leak - mdot_fluid_out_before_leak) > 0)
    {
        // Fluid In
        double cp_fluid_in = mc_htf.Cp(T_fluid_in) * 1000.0; // J/kg K
        double mass_fluid_in = mdot_fluid_in_before_leak * timestep;
        //double T_fluid_in = T_fluid_in;

        // Wall In
        double cp_wall_in = m_tank_wall_cp;
        double mass_wall_in = mdot_in_wall * timestep;
        double T_wall_in = T_tank_in;

        // Adjust Wall Mass IN (to allow for cp correction)
        double cp_wall_in_corrected = cp_fluid_in; // J/kg K
        double mass_wall_in_corrected = mass_wall_in * (cp_wall_in / cp_wall_in_corrected);   // kg

        // Stagnant Fluid
        //double cp_fluid_prev = cp_fluid_prev;
        double mass_fluid_stagnant = mass_fluid_prev_inner;
        double T_fluid_stagnant = T_prev_inner;

        // Stagnant Wall
        double cp_wall_stagnant = m_tank_wall_cp;
        double mass_wall_stagnant = mass_wall_prev;
        double T_wall_stagnant = T_prev_inner;

        // Adjust Wall Mass STAGNANT (to allow for cp correction)
        double cp_wall_stagnant_corrected = cp_fluid_prev;  // J/kg K
        double mass_wall_stagnant_corrected = mass_wall_stagnant * (cp_wall_stagnant / cp_wall_stagnant_corrected); // kg

        // Total Inlet
        double mass_in_total = mass_fluid_in + mass_wall_in_corrected;
        mdot_in_total = mass_in_total / timestep;
        double mass_cp_in_total = (mass_fluid_in * cp_fluid_in) + (mass_wall_in_corrected * cp_wall_in_corrected);
        cp_in_weighted = ((cp_fluid_in * mass_fluid_in) +
                          (cp_wall_in_corrected * mass_wall_in_corrected))
                          / mass_in_total;

        T_in_weighted = ((T_fluid_in * cp_fluid_in * mass_fluid_in) +
                         (T_wall_in * cp_wall_in_corrected * mass_wall_in_corrected))
                         / (mass_cp_in_total);

        // Total Outlet
        mdot_out_total = mdot_leak_out;
        cp_out_weighted = cp_fluid_prev;


        // Total Bulk at end of timestep
        double mass_total_calc = mass_fluid_in + mass_wall_in_corrected + mass_fluid_stagnant + mass_wall_stagnant_corrected;
        cp_bulk_weighted_calc = ((cp_fluid_in * mass_fluid_in) +
                                 (cp_wall_in_corrected * mass_wall_in_corrected) +
                                 (cp_fluid_prev * mass_fluid_stagnant) +
                                 (cp_wall_stagnant_corrected * mass_wall_stagnant_corrected))
                                 / mass_total_calc;
    }

    // If Fluid is leaving (leak coming in)
    else
    {
        // Leak In
        double cp_leak_in = mc_htf.Cp(T_leak_in) * 1000.0;   // J/kg K
        double mass_leak_in = mdot_leak_in * timestep;

        // Stagnant Fluid
        double cp_fluid_stagnant = cp_fluid_prev;
        double mass_fluid_stagnant = mass_fluid_calc_inner - mass_leak_in;

        // Stagnant Wall
        double cp_wall_stagnant = m_tank_wall_cp;
        double mass_wall_stagnant = mass_wall_calc;

        // Corrected Stagnant Wall
        double cp_wall_stagnant_corrected = cp_fluid_stagnant;
        double mass_wall_stagnant_corrected = mass_wall_stagnant * (cp_wall_stagnant / cp_wall_stagnant_corrected);

        // Fluid Out
        double mass_fluid_out = mdot_fluid_out_adj * timestep;
        double cp_fluid_out = cp_fluid_prev;

        // Wall Out
        double mass_wall_out = mdot_out_wall * timestep;
        double cp_wall_out = m_tank_wall_cp;

        // Corrected Wall Mass Out
        double cp_wall_out_corrected = cp_fluid_out;
        double mass_wall_out_corrected = mass_wall_out * (cp_wall_out / cp_wall_out_corrected);

        // Total Inlet
        double mass_in_total = mass_leak_in;
        mdot_in_total = mass_leak_in / timestep;
        cp_in_weighted = cp_leak_in;
        T_in_weighted = T_leak_in;

        // Total Outlet
        double mass_out_total = mass_fluid_out + mass_wall_out_corrected;
        mdot_out_total = mass_out_total / timestep;
        cp_out_weighted = ((cp_fluid_out * mass_fluid_out) +
                           (cp_wall_out_corrected * mass_wall_out_corrected))
                           / mass_out_total;

        // Total Bulk at end of timestep
        double mass_total_calc = mass_leak_in + mass_fluid_stagnant + mass_wall_stagnant_corrected;
        cp_bulk_weighted_calc = ((cp_leak_in * mass_leak_in) +
                                 (cp_fluid_stagnant * mass_fluid_stagnant) +
                                 (cp_wall_stagnant_corrected * mass_wall_stagnant_corrected))
                                 / mass_total_calc;
    }
        
    // Terms used in final calculation
    double diff_m_dot_total = mdot_in_total - mdot_out_total;
    double mass_total_prev = mass_fluid_prev_inner + mass_wall_prev;
    double UA_calc = m_u_tank * m_SA_calc;

    // Adjust Wall Mass Prev Term
    double cp_wall_prev_corrected = cp_fluid_prev;
    double mass_wall_prev_corrected = mass_wall_prev * (m_tank_wall_cp / cp_wall_prev_corrected);

    // OVERWRITE mass_total_prev
    mass_total_prev = mass_fluid_prev_inner + mass_wall_prev_corrected;

    // Validate Mass Balance
    {
        double mass_total_prev = mass_wall_prev + mass_fluid_prev_inner;
        double mass_in = mdot_in_total * timestep;
        double mass_out = mdot_out_total * timestep;
        double mass_total_calc = mass_wall_calc + mass_fluid_calc_inner;

        double balance = (mass_total_calc - mass_total_prev) - (mass_in - mass_out);
        if (std::abs(balance) >= 1e-5)
        {
            int x = 0;
        }

    }

    // Tank is either expanding or contracting
    if (diff_m_dot_total != 0.0)
    {
        // NEW
        // m_dot_in (NOW wall mass in and fluid mass in)
        // T_in (NOW wall temp in (from cold or hot side) and fluid Temp in)
        // cp   (NOW weighted cp_weighted of wall and fluid)
        // diff_m_dot   (NOW total mass difference from wall and fluid)
        // m_m_prev (WAS previous bulk fluid mass, NOW needs to be previous bulk fluid and wall mass)

        double a_coef_old = (mdot_in_total - mdot_in_wall) * T_fluid_in + UA_calc / cp_fluid_prev * T_amb;
        double b_coef_old = (mdot_in_total - mdot_in_wall) + UA_calc / cp_fluid_prev;
        double c_coef_old = diff_m_dot;

        double a_coef = mdot_in_total * T_in_weighted + (UA_calc / cp_bulk_weighted_calc) * T_amb;
        double b_coef = mdot_in_total + UA_calc / cp_bulk_weighted_calc;
        double c_coef = diff_m_dot_total;

        double a_coef_beta = ((cp_in_weighted / cp_bulk_weighted_calc) * mdot_in_total * T_in_weighted)
                        + ((UA_calc / cp_bulk_weighted_calc) * T_amb);
        double b_coef_beta = mdot_in_total - mdot_out_total
                        + ((cp_out_weighted / cp_bulk_weighted_calc) * mdot_out_total)
                        + (UA_calc / cp_bulk_weighted_calc);
        double c_coef_beta = mdot_in_total - mdot_out_total;


        /*if (std::abs(a_coef - a_coef_beta) > 1e-4)
        {
            int l = 0;
        }
        if (std::abs(b_coef - b_coef_beta) > 1e-4)
        {
            int l = 0;
        }
        if (std::abs(c_coef - c_coef_beta) > 1e-4)
        {
            int l = 0;
        }*/


        T_calc_inner = a_coef / b_coef;
        if(mass_total_prev > 0)
            T_calc_inner += (T_prev_inner - a_coef / b_coef) * pow(std::max((timestep * c_coef / mass_total_prev + 1), 0.0), -b_coef / c_coef);

        T_ave = a_coef / b_coef;
        if(mass_total_prev > 0 && b_coef != c_coef)
            T_ave += mass_total_prev * (T_prev_inner - a_coef / b_coef) / ((c_coef - b_coef) * timestep)
                    * (pow(std::max((timestep * c_coef / mass_total_prev + 1.0), 0.0), 1.0 - b_coef / c_coef) - 1.0);

        if (timestep < 1.e-6)
        {
            T_ave = a_coef / b_coef;
            if (mass_total_prev > 0 && b_coef != c_coef)
                T_ave += (T_prev_inner - a_coef / b_coef) * pow(std::max((timestep * c_coef / mass_total_prev + 1.0), 0.0), -b_coef / c_coef);	// Limiting expression for small time step
        }
           
        q_dot_loss = UA_calc * (T_ave - T_amb) / 1.E6;		//[MW]

        //******* DEBUG

        double T_calc_inner_old = a_coef_old / b_coef_old + (T_prev_inner - a_coef_old / b_coef_old) * pow(std::max((timestep * c_coef_old / mass_fluid_prev_inner + 1), 0.0), -b_coef_old / c_coef_old);
        double T_ave_old = a_coef_old / b_coef_old + mass_fluid_prev_inner * (T_prev_inner - a_coef_old / b_coef_old) / ((c_coef_old - b_coef_old) * timestep) * (pow(std::max((timestep * c_coef_old / mass_fluid_prev_inner + 1.0), 0.0), 1.0 - b_coef_old / c_coef_old) - 1.0);
        if (timestep < 1.e-6)
            T_ave_old = a_coef_old / b_coef_old + (T_prev_inner - a_coef_old / b_coef_old) * pow(std::max((timestep * c_coef_old / mass_fluid_prev_inner + 1.0), 0.0), -b_coef_old / c_coef_old);	// Limiting expression for small time step	
        double q_dot_loss_old = UA_calc * (T_ave_old - T_amb) / 1.E6;		//[MW]

        //************

        if (T_calc_inner < m_T_htr)
        {
            q_heater = b_coef * ((m_T_htr - T_prev_inner * pow(std::max((timestep * c_coef / mass_total_prev + 1), 0.0), -b_coef / c_coef)) /
                (-pow(std::max((timestep * c_coef / mass_total_prev + 1), 0.0), -b_coef / c_coef) + 1)) - a_coef;

            q_heater = q_heater * cp_bulk_weighted_calc;

            q_heater /= 1.E6;

            if (q_heater > m_max_q_htr)
            {
                q_heater = m_max_q_htr;
            }

            a_coef += q_heater * 1.E6 / cp_bulk_weighted_calc;

            T_calc_inner = a_coef / b_coef;
            if (mass_total_prev > 0)
                T_calc_inner += (T_prev_inner - a_coef / b_coef) * pow(std::max((timestep * c_coef / mass_total_prev + 1), 0.0), -b_coef / c_coef);

            T_ave = a_coef / b_coef;
            if (mass_total_prev > 0 && b_coef != c_coef)
                T_ave += mass_total_prev * (T_prev_inner - a_coef / b_coef) / ((c_coef - b_coef) * timestep) * (pow(std::max((timestep * c_coef / mass_total_prev + 1.0), 0.0), 1.0 - b_coef / c_coef) - 1.0);


            if (timestep < 1.e-6)
            {
                T_ave = a_coef / b_coef;
                if(mass_total_prev > 0 && b_coef != c_coef)
                    T_ave += (T_prev_inner - a_coef / b_coef) * pow(std::max((timestep * c_coef / mass_total_prev + 1.0), 0.0), -b_coef / c_coef);  // Limiting expression for small time step	
            }

            q_dot_loss = UA_calc * (T_ave - T_amb) / 1.E6;		//[MW]
        }
        else
        {
            q_heater = 0.0;
        }
    }

    // Tank is stagnant
    else	// No mass flow rate, tank is idle
    {
        // NEW
        // cp   (NOW weighted cp_weighted of wall and fluid)
        // m_m_prev (WAS previous bulk fluid mass, NOW needs to be previous bulk fluid and wall mass)

        double b_coef = UA_calc / (cp_bulk_weighted_calc * mass_total_prev);
        double c_coef = UA_calc / (cp_bulk_weighted_calc * mass_total_prev) * T_amb;

        T_calc_inner = c_coef / b_coef + (T_prev_inner - c_coef / b_coef) * exp(-b_coef * timestep);
        T_ave = c_coef / b_coef - (T_prev_inner - c_coef / b_coef) / (b_coef * timestep) * (exp(-b_coef * timestep) - 1.0);
        if (timestep < 1.e-6)
            T_ave = c_coef / b_coef + (T_prev_inner - c_coef / b_coef) * exp(-b_coef * timestep);  // Limiting expression for small time step	
        q_dot_loss = UA_calc * (T_ave - T_amb) / 1.E6;

        if (T_calc_inner < m_T_htr)
        {
            q_heater = (b_coef * (m_T_htr - T_prev_inner * exp(-b_coef * timestep)) / (-exp(-b_coef * timestep) + 1.0) - c_coef) * cp_bulk_weighted_calc * mass_total_prev;
            q_heater /= 1.E6;	//[MW]

            if (q_heater > m_max_q_htr)
            {
                q_heater = m_max_q_htr;
            }

            c_coef += q_heater * 1.E6 / (cp_bulk_weighted_calc * mass_total_prev);

            T_calc_inner = c_coef / b_coef + (T_prev_inner - c_coef / b_coef) * exp(-b_coef * timestep);
            T_ave = c_coef / b_coef - (T_prev_inner - c_coef / b_coef) / (b_coef * timestep) * (exp(-b_coef * timestep) - 1.0);
            if (timestep < 1.e-6)
                T_ave = c_coef / b_coef + (T_prev_inner - c_coef / b_coef) * exp(-b_coef * timestep);  // Limiting expression for small time step	
            q_dot_loss = UA_calc * (T_ave - T_amb) / 1.E6;		//[MW]
        }
        else
        {
            q_heater = 0.0;
        }

        
    }

    // Validate Energy Balance
    {
        double mass_total_prev = mass_wall_prev + mass_fluid_prev_inner;
        double cp_bulk_prev = 0;
        if (mass_total_prev > 0)
        {
            cp_bulk_prev = ((mass_wall_prev * m_tank_wall_cp)
                            + (mass_fluid_prev_inner * cp_fluid_prev))
                            / (mass_wall_prev + mass_fluid_prev_inner);
        }

        double E_bulk_prev = mass_total_prev * cp_bulk_prev * T_prev_inner * 1e-9;
        double mass_total_calc = mass_wall_calc + mass_fluid_calc_inner;
        double E_bulk_calc = mass_total_calc * cp_bulk_weighted_calc * T_calc_inner * 1e-9;

        double T_out_weighted = T_ave; // Change to AVG temperature

        double E_out = 0;
        if(mdot_out_total > 1e-4)
            E_out = (mdot_out_total * timestep) * cp_out_weighted * T_out_weighted * 1e-9;
        double E_loss_out = q_dot_loss * 1e6 * timestep * 1e-9;
        double E_out_total = E_out + E_loss_out;

        double E_in = 0;
        if(mdot_in_total > 1e-4)
            E_in = (mdot_in_total * timestep) * cp_in_weighted * T_in_weighted * 1e-9;
        double E_heater_in = q_heater * 1e6 * timestep * 1e-9;
        double E_in_total = E_in + E_heater_in;

        q_dot_error_inner = ((E_bulk_calc - E_bulk_prev) - (E_in_total - E_out_total)) / timestep;  //[MWt]

        if (std::abs(q_dot_error_inner) >= 1e-4)
        {
            int x = 0;
        }

        if (std::isnan(q_dot_error_inner))
        {
            int x = 0;
        }

    }

    // Define Q_out
    q_dot_out = mdot_fluid_out_before_leak * T_ave * cp_fluid_prev;

    // Define Internal energy
    {
        double wall_E_calc = m_tank_wall_cp * T_calc_inner * mass_wall_calc * 1e-6;    // [MJ]
        double fluid_E_calc = mc_htf.Cp(T_calc_inner) * T_calc_inner * mass_fluid_calc_inner * 1e-3;    // [MJ]
        m_E_calc = wall_E_calc + fluid_E_calc;  // [MJ]
    }

    // Define calculated variables
    m_T_calc = T_calc_inner;
    m_m_calc = mass_fluid_calc_inner;

    if (tank_is_empty) {
        // set to actual values
        m_V_calc = 0.;
        mass_fluid_calc_inner = 0.;
    }
}

void C_storage_tank_dynamic_cyl::energy_balance_iterated(double timestep /*s*/, double m_dot_in /*kg/s*/, double m_dot_out /*kg/s*/,
    double T_in /*K*/, double T_amb /*K*/,
    double T_tank_in, /*K*/
    double T_leak_in, /*K*/
    double& T_ave /*K*/, double& q_heater /*MW*/, double& q_dot_loss /*MW*/, double& q_dot_out /*MW*/, double& q_dot_error /*MW*/)
{
    double ministep = timestep / m_nstep;

    double q_heater_summed = 0;
    double q_dot_loss_summed = 0;
    double T_ave_innerstep = 0;

    double mass_prev_inner = m_m_prev;
    double mass_calc_inner = 0;

    double T_prev_inner = m_T_prev;
    double T_calc_inner = 0;
    double T_ave_weighted = 0;

    double q_dot_out_inner = 0;
    double q_dot_out_summed = 0;
    double q_dot_error_summed = 0;

    // Run Energy Balance
    for (int i = 0; i < m_nstep; i++)
    {
        double q_heater_innerstep, q_dot_loss_innerstep, q_dot_error_innerstep;

        
        energy_balance_core(ministep, m_dot_in, m_dot_out, T_in, T_amb, mass_prev_inner, T_tank_in, T_prev_inner, T_leak_in,
            T_ave_innerstep, q_heater_innerstep, q_dot_loss_innerstep, mass_calc_inner, T_calc_inner, q_dot_out_inner, q_dot_error_innerstep);

        q_heater_summed += q_heater_innerstep * (ministep / timestep);
        q_dot_loss_summed += q_dot_loss_innerstep * (ministep / timestep);
        q_dot_out_summed += q_dot_out_inner;
        q_dot_error_summed += q_dot_error_innerstep;

        mass_prev_inner = mass_calc_inner;
        T_prev_inner = T_calc_inner;
        T_ave_weighted += T_ave_innerstep * (ministep / timestep);
    }

    T_ave = T_ave_weighted;
    q_heater = q_heater_summed;
    q_dot_loss = q_dot_loss_summed;
    q_dot_out = q_dot_out_summed;
    q_dot_error = q_dot_error_summed;


    m_m_calc = mass_calc_inner;
    m_T_calc = T_calc_inner;

}


double C_storage_tank_dynamic_cyl::calc_SA_rate(double mdot_htf /*kg/s*/, double T_htf /*K*/)
{
    // Get Density
    double rho = mc_htf.dens(T_htf, 1.0);

    double r = m_radius;

    double SA_rate = (2.0 * mdot_htf) / (r * rho);

    return SA_rate;
}

double C_storage_tank_dynamic_cyl::calc_mdot_expansion(double piston_rate /*m/s*/)
{
    double A_outer = CSP::pi * std::pow(m_radius + m_tank_wall_thick, 2.0);
    double A_inner = CSP::pi * std::pow(m_radius, 2.0);

    double A_cross_section = A_outer - A_inner;
    double volume_expansion_rate = A_cross_section * piston_rate;
    double mdot_rate = volume_expansion_rate * m_tank_wall_dens;

    return mdot_rate;
}

double C_storage_tank_dynamic_cyl::calc_tank_wall_volume(double fluid_mass /*kg*/, double T_htf /*K*/)
{
    double A_outer = CSP::pi * std::pow(m_radius + m_tank_wall_thick, 2.0);
    double A_inner = CSP::pi * std::pow(m_radius, 2.0);
    double A_cross_section = A_outer - A_inner;

    // Get Density
    double rho_htf = mc_htf.dens(T_htf, 1.0);

    double fluid_volume = fluid_mass / rho_htf;
    double fluid_L = fluid_volume / (CSP::pi * std::pow(m_radius, 2.0));

    double tank_wall_vol = A_cross_section * fluid_L;

    return tank_wall_vol;
}

double C_storage_tank_dynamic_cyl::calc_SA(double volume /*m3*/)
{
    return 2.0 * volume / m_radius;
}

double C_storage_tank_dynamic_cyl::calc_leakage_fraction(double mdot)
{
    double N_terms = m_piston_loss_poly.size();
    double frac = 0;
    for (int i = 0; i < N_terms; i++)
    {
        frac += m_piston_loss_poly[i] * std::pow(mdot, i);
    }

    frac *= 0.01;   // Convert from % to fraction

    return frac;
}



C_csp_piston_cylinder_tes::C_csp_piston_cylinder_tes(
    int external_fl,                             // [-] external fluid identifier
    util::matrix_t<double> external_fl_props,    // [-] external fluid properties
    //int tes_fl,                                  // [-] tes fluid identifier
    //util::matrix_t<double> tes_fl_props,         // [-] tes fluid properties
    double q_dot_design,                         // [MWt] Design heat rate in and out of tes
    double frac_max_q_dot,                       // [-] the max design heat rate as a fraction of the nominal
    double Q_tes_des,			                 // [MWt-hr] design storage capacity
    bool is_h_fixed,                             // [] [true] Height is input, calculate diameter, [false] diameter input, calculate height
    double h_tank_in,			                 // [m] tank height input
    double d_tank_in,                            // [m] tank diameter input
    double u_tank,			                     // [W/m^2-K]
    int tank_pairs,			                     // [-]
    double hot_tank_Thtr,		                 // [C] convert to K in init()
    double hot_tank_max_heat,	                 // [MW]
    double cold_tank_Thtr,	                     // [C] convert to K in init()
    double cold_tank_max_heat,                   // [MW]
    double T_cold_des,	                         // [C] convert to K in init()
    double T_hot_des,	                         // [C] convert to K in init()
    double T_tank_hot_ini,	                     // [C] Initial temperature in hot storage tank
    double T_tank_cold_ini,	                     // [C] Initial temperature in cold storage cold
    double h_tank_min,		                     // [m] Minimum allowable HTF height in storage tank
    double f_V_hot_ini,                          // [%] Initial fraction of available volume that is hot
    double htf_pump_coef,		                 // [kW/kg/s] Pumping power to move 1 kg/s of HTF through sink
    double tank_wall_cp,                         // [J/kg-K] Tank wall specific heat
    double tank_wall_dens,                       // [kg/m3] Tank wall density
    double tank_wall_thick,                      // [m] Tank wall thickness
    int nstep,                                   // [] Number of time steps for energy balance iteration
    std::vector<double> piston_loss_poly,        // [] Coefficients to piston loss polynomial (0*x^0 + 1*x^1 ....)    
    double V_tes_des,                            // [m/s] Design-point velocity for sizing the diameters of the TES piping
    bool calc_design_pipe_vals,                  // [-] Should the HTF state be calculated at design conditions
    double tes_pump_coef,		                 // [kW/kg/s] Pumping power to move 1 kg/s of HTF through tes loop
    double eta_pump,                             // [-] Pump efficiency, for newer pumping calculations
    bool has_hot_tank_bypass,                    // [-] True if the bypass valve causes the source htf to bypass just the hot tank and enter the cold tank before flowing back to the external system.
    double T_tank_hot_inlet_min,                 // [C] Minimum source htf temperature that may enter the hot tank
    bool custom_tes_p_loss,                      // [-] True if the TES piping losses should be calculated using the TES pipe lengths and minor loss coeffs, false if using the pumping loss parameters
    bool custom_tes_pipe_sizes,                  // [-] True if the TES diameters and wall thicknesses parameters should be used instead of calculating them
    util::matrix_t<double> k_tes_loss_coeffs,    // [-] Combined minor loss coefficients of the fittings and valves in the collection (including bypass) and generation loops in the TES 
    util::matrix_t<double> tes_diams,            // [m] Imported inner diameters for the TES piping as read from the modified output files
    util::matrix_t<double> tes_wallthicks,       // [m] Imported wall thicknesses for the TES piping as read from the modified output files
    util::matrix_t<double> tes_lengths,          // [m] Imported lengths for the TES piping as read from the modified output files
    double pipe_rough,                           // [m] Pipe absolute roughness
    double dP_discharge                          // [bar] Pressure drop on the TES discharge side (e.g., within the steam generator)
    )
    :
        m_external_fl(external_fl), m_external_fl_props(external_fl_props), //m_tes_fl(tes_fl), m_tes_fl_props(tes_fl_props),
        m_q_dot_design(q_dot_design), m_frac_max_q_dot(frac_max_q_dot), m_Q_tes_des(Q_tes_des),
        m_is_h_fixed(is_h_fixed), m_h_tank_in(h_tank_in), m_d_tank_in(d_tank_in),
        m_u_tank(u_tank), m_tank_pairs(tank_pairs), m_hot_tank_Thtr(hot_tank_Thtr), m_hot_tank_max_heat(hot_tank_max_heat),
        m_cold_tank_Thtr(cold_tank_Thtr), m_cold_tank_max_heat(cold_tank_max_heat), m_T_cold_des(T_cold_des),
        m_T_hot_des(T_hot_des), m_T_tank_hot_ini(T_tank_hot_ini), m_T_tank_cold_ini(T_tank_cold_ini),
        m_h_tank_min(h_tank_min), m_f_V_hot_ini(f_V_hot_ini), m_htf_pump_coef(htf_pump_coef),
        m_tank_wall_cp(tank_wall_cp), m_tank_wall_dens(tank_wall_dens), m_tank_wall_thick(tank_wall_thick), m_nstep(nstep),
        m_piston_loss_poly(piston_loss_poly),
        V_tes_des(V_tes_des), calc_design_pipe_vals(calc_design_pipe_vals), m_tes_pump_coef(tes_pump_coef),
        eta_pump(eta_pump), has_hot_tank_bypass(has_hot_tank_bypass), T_tank_hot_inlet_min(T_tank_hot_inlet_min),
        custom_tes_p_loss(custom_tes_p_loss), custom_tes_pipe_sizes(custom_tes_pipe_sizes), k_tes_loss_coeffs(k_tes_loss_coeffs),
        tes_diams(tes_diams), tes_wallthicks(tes_wallthicks), tes_lengths(tes_lengths),
        pipe_rough(pipe_rough), dP_discharge(dP_discharge), tanks_in_parallel(true)
{

    if (tes_lengths.ncells() < 11) {
        double lengths[11] = { 0., 90., 100., 120., 0., 30., 90., 80., 80., 120., 80. };
        this->tes_lengths.assign(lengths, 11);
    }

    m_vol_tank = m_V_tank_active = m_q_pb_design = m_ts_hours =
        m_V_tank_hot_ini = m_mass_total_active = m_h_tank_calc = m_d_tank_calc = m_q_dot_loss_des =
        m_cp_external_avg = m_rho_store_avg = m_m_dot_tes_des_over_m_dot_external_des = std::numeric_limits<double>::quiet_NaN();

    mc_reported_outputs.construct(S_output_info);
}

C_csp_piston_cylinder_tes::C_csp_piston_cylinder_tes()
{
    m_vol_tank = m_V_tank_active = m_q_pb_design = m_Q_tes_des =
        m_V_tank_hot_ini = m_mass_total_active = m_h_tank_calc = m_d_tank_calc = m_q_dot_loss_des =
        m_cp_external_avg = m_rho_store_avg = m_m_dot_tes_des_over_m_dot_external_des = std::numeric_limits<double>::quiet_NaN();

    mc_reported_outputs.construct(S_output_info);
}


void C_csp_piston_cylinder_tes::init(const C_csp_tes::S_csp_tes_init_inputs init_inputs)
{
    if (!(m_Q_tes_des > 0.0))
    {
        m_is_tes = false;
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

    // For Piston Cylinder, storage fluid IS external fluid
    mc_store_htfProps = mc_external_htfProps;

    //// Declare instance of fluid class for STORAGE fluid.
    //// Set fluid number and copy over fluid matrix if it makes sense.
    //if (m_tes_fl != HTFProperties::User_defined && m_tes_fl < HTFProperties::End_Library_Fluids)
    //{
    //    if (!mc_store_htfProps.SetFluid(m_tes_fl))
    //    {
    //        throw(C_csp_exception("Storage HTF code is not recognized", "Two Tank TES Initialization"));
    //    }
    //}
    //else if (m_tes_fl == HTFProperties::User_defined)
    //{
    //    int n_rows = (int)m_tes_fl_props.nrows();
    //    int n_cols = (int)m_tes_fl_props.ncols();
    //    if (n_rows > 2 && n_cols == 7)
    //    {
    //        if (!mc_store_htfProps.SetUserDefinedFluid(m_tes_fl_props))
    //        {
    //            error_msg = util::format(mc_store_htfProps.UserFluidErrMessage(), n_rows, n_cols);
    //            throw(C_csp_exception(error_msg, "Two Tank TES Initialization"));
    //        }
    //    }
    //    else
    //    {
    //        error_msg = util::format("The user defined storage HTF table must contain at least 3 rows and exactly 7 columns. The current table contains %d row(s) and %d column(s)", n_rows, n_cols);
    //        throw(C_csp_exception(error_msg, "Two Tank TES Initialization"));
    //    }
    //}
    //else
    //{
    //    throw(C_csp_exception("Storage HTF code is not recognized", "Two Tank TES Initialization"));
    //}

    /*bool is_hx_calc = false;
    if (m_tes_fl != m_external_fl)
        is_hx_calc = true;
    else if (m_external_fl != HTFProperties::User_defined)
        is_hx_calc = false;
    else
    {
        is_hx_calc = !mc_external_htfProps.equals(&mc_store_htfProps);
    }
    if (is_hx_calc == true)
    {
        throw(C_csp_exception("NT Tank must have the same external and internal fluid", "NT TES Initialization"));
    }*/

    if (tanks_in_parallel) {
        m_is_cr_to_cold_tank_allowed = false;
    }
    else {
        throw C_csp_exception("Tank model must be in parallel");
    }

    // Calculate thermal power to PC at design
    m_q_pb_design = m_q_dot_design * 1.E6;	//[Wt]

    // Convert parameter units
    m_hot_tank_Thtr += 273.15;		//[K] convert from C
    m_cold_tank_Thtr += 273.15;		//[K] convert from C
    m_T_cold_des += 273.15;		    //[K] convert from C
    m_T_hot_des += 273.15;		    //[K] convert from C
    m_T_tank_hot_ini += 273.15;		//[K] convert from C
    m_T_tank_cold_ini += 273.15;		//[K] convert from C

    m_ts_hours = m_Q_tes_des / m_q_dot_design;

    double d_tank_temp = std::numeric_limits<double>::quiet_NaN();
    double q_dot_loss_temp = std::numeric_limits<double>::quiet_NaN();
    double T_tes_hot_des, T_tes_cold_des;

    T_tes_hot_des = m_T_hot_des;
    T_tes_cold_des = m_T_cold_des;

    // Size Tank with Fixed Height
    if (m_is_h_fixed)
    {
        piston_cylinder_tes_sizing(mc_store_htfProps, m_Q_tes_des, T_tes_hot_des, T_tes_cold_des,
            m_h_tank_min, m_h_tank_in, m_tank_pairs, m_u_tank,
            m_V_tank_active, m_vol_tank, m_d_tank_calc, m_q_dot_loss_des);
        m_h_tank_calc = m_h_tank_in;
    }
    // Size Tank with Fixed Diameter
    else
    {
        piston_cylinder_tes_sizing_fixed_diameter(mc_store_htfProps, m_Q_tes_des, T_tes_hot_des, T_tes_cold_des,
            m_h_tank_min, m_d_tank_in, m_tank_pairs, m_u_tank,
            m_V_tank_active, m_vol_tank, m_h_tank_calc, m_q_dot_loss_des);
        m_d_tank_calc = m_d_tank_in;
    }

    // 5.13.15, twn: also be sure that hx is sized such that it can supply full load to sink
    double duty = m_q_pb_design * std::max(1.0, m_frac_max_q_dot);		//[W] Allow all energy from the source to go into storage at any time

    // Calculate initial storage values

    // Initial storage charge based on % mass 
    double T_tes_ave = 0.5 * (T_tes_hot_des + T_tes_cold_des);
    double cp_ave = mc_store_htfProps.Cp_ave(T_tes_cold_des, T_tes_hot_des);				//[kJ/kg-K] Specific heat at average temperature
    m_rho_store_avg = mc_store_htfProps.dens(T_tes_ave, 1.0);

    //m_mass_total_active = m_Q_tes_des * 3600.0 / (cp_ave / 1000.0 * (T_tes_hot_des - T_tes_cold_des));  //[kg] Total HTF mass at design point inlet/outlet T
    m_mass_total_active = m_V_tank_active * mc_store_htfProps.dens(T_tes_cold_des, 1.0);    // [kg] total mass is cold fluid that fills tank

    double V_inactive = m_vol_tank - m_V_tank_active;



    // UPDATE INITIAL MASS
    double rho_hot_des = mc_store_htfProps.dens(T_tes_hot_des, 1.0);
    double rho_cold_des = mc_store_htfProps.dens(T_tes_cold_des, 1.0);
    double rho_hot = mc_store_htfProps.dens(m_T_tank_hot_ini, 1.0);
    double rho_cold = mc_store_htfProps.dens(m_T_tank_cold_ini, 1.0);
    double m_hot_ini = m_f_V_hot_ini * 0.01 * m_mass_total_active + V_inactive * rho_hot_des;  // Updating intiial storage charge calculation to avoid variation in total mass with specified initial T
    double m_cold_ini = (1.0 - m_f_V_hot_ini * 0.01) * m_mass_total_active + V_inactive * rho_cold_des;
    double V_hot_ini = m_hot_ini / rho_hot;
    double V_cold_ini = m_cold_ini / rho_cold;

    double T_hot_ini = m_T_tank_hot_ini;		//[K]
    double T_cold_ini = m_T_tank_cold_ini;	//[K]

    // TMB 12.15.2023 Calculate Total Length
    m_length_total = m_h_tank_calc;
    double volume_combined = m_vol_tank;    // Total volume is actually volume of one tank (total mass can fill 1 'two tank' tank)

    // Initialize cold and hot tanks
            // Hot tank
    mc_hot_tank_cyl.init(mc_store_htfProps, m_vol_tank, m_h_tank_calc, m_h_tank_min,
        m_u_tank, m_tank_pairs, m_hot_tank_Thtr, m_hot_tank_max_heat,
        V_hot_ini, T_hot_ini, T_tes_hot_des, m_tank_wall_cp, m_tank_wall_dens, m_tank_wall_thick, m_nstep, m_piston_loss_poly);
    // Cold tank
    mc_cold_tank_cyl.init(mc_store_htfProps, m_vol_tank, m_h_tank_calc, m_h_tank_min,
        m_u_tank, m_tank_pairs, m_cold_tank_Thtr, m_cold_tank_max_heat,
        V_cold_ini, T_cold_ini, T_tes_cold_des, m_tank_wall_cp, m_tank_wall_dens, m_tank_wall_thick, m_nstep, m_piston_loss_poly);

    double hot_radius = mc_hot_tank_cyl.get_radius();
    double cold_radius = mc_cold_tank_cyl.get_radius();

    if (hot_radius != cold_radius)
    {
        throw C_csp_exception("Tanks have different radius");
    }

    m_radius = hot_radius;

    // Calculate 'nominal' wall mass / volume
    double Ac_wall = (CSP::pi * std::pow(m_radius + m_tank_wall_thick, 2.0)) - (CSP::pi * std::pow(m_radius, 2.0));
    m_V_wall_nominal = Ac_wall * m_length_total;   //[m3]
    m_mass_wall_nominal = m_tank_wall_dens * m_V_wall_nominal;  //[kg]

}

bool C_csp_piston_cylinder_tes::does_tes_exist()
{
	return m_is_tes;
}

bool C_csp_piston_cylinder_tes::is_cr_to_cold_allowed()
{
	return m_is_cr_to_cold_tank_allowed;
}

double C_csp_piston_cylinder_tes::get_hot_temp()
{
	return mc_hot_tank_cyl.get_m_T_prev();	//[K]
}

double C_csp_piston_cylinder_tes::get_cold_temp()
{
	return mc_cold_tank_cyl.get_m_T_prev();	//[K]
}

double C_csp_piston_cylinder_tes::get_hot_tank_vol_frac()
{
	return mc_hot_tank_cyl.get_vol_frac();
}

double C_csp_piston_cylinder_tes::get_initial_charge_energy()
{
    //MWh
    if (std::isnan(m_V_tank_hot_ini))
    {
        return m_q_pb_design * m_ts_hours * (m_f_V_hot_ini / 100.0) * 1.e-6;
    }
    else
    {
        //TODO: m_V_tank_hot_ini does not get initialized to user value...
        return m_q_pb_design * m_ts_hours * m_V_tank_hot_ini / m_vol_tank * 1.e-6;
    }
}

double C_csp_piston_cylinder_tes::get_min_charge_energy()
{
    //MWh
    return 0.; //m_q_pb_design * m_ts_hours * m_h_tank_min / m_h_tank*1.e-6;
}

double C_csp_piston_cylinder_tes::get_max_charge_energy()
{
    return m_q_pb_design * m_ts_hours / 1.e6;
}

double C_csp_piston_cylinder_tes::get_degradation_rate()
{
    //calculates an approximate "average" tank heat loss rate based on some assumptions. Good for simple optimization performance projections.
    double d_tank = sqrt(m_vol_tank / ((double)m_tank_pairs * m_h_tank_calc * 3.14159));
    double e_loss = m_u_tank * 3.14159 * m_tank_pairs * d_tank * (m_T_cold_des + m_T_hot_des - 576.3) * 1.e-6;  //MJ/s  -- assumes full area for loss, Tamb = 15C
    return e_loss / (m_q_pb_design * m_ts_hours * 3600.); //s^-1  -- fraction of heat loss per second based on full charge
}

void C_csp_piston_cylinder_tes::reset_storage_to_initial_state()
{
    // Initial storage charge based on % mass
    double Q_tes_des = m_q_pb_design / 1.E6 * m_ts_hours;		//[MWt-hr] TES thermal capacity at design
    double cp_ave = mc_store_htfProps.Cp_ave(m_T_cold_des, m_T_hot_des);				//[kJ/kg-K] Specific heat at average temperature
    double mtot = Q_tes_des * 3600.0 / (cp_ave / 1000.0 * (m_T_hot_des - m_T_cold_des));  //[kg] Total HTF mass
    double rho_hot = mc_store_htfProps.dens(m_T_hot_des, 1.0);
    double rho_cold = mc_store_htfProps.dens(m_T_cold_des, 1.0);

    double V_inactive = m_vol_tank - m_V_tank_active;
    double V_hot_ini = m_f_V_hot_ini * 0.01 * mtot / rho_hot + V_inactive;			//[m^3]
    double V_cold_ini = (1.0 - m_f_V_hot_ini * 0.01) * mtot / rho_cold + V_inactive;	//[m^3]

    double T_hot_ini = m_T_tank_hot_ini;		//[K]
    double T_cold_ini = m_T_tank_cold_ini;	//[K]

    // Initialize cold and hot tanks
    // Hot tank
    mc_hot_tank_cyl.init(mc_store_htfProps, m_vol_tank, m_h_tank_calc, m_h_tank_min,
        m_u_tank, m_tank_pairs, m_hot_tank_Thtr, m_hot_tank_max_heat,
        V_hot_ini, T_hot_ini, m_T_hot_des, m_tank_wall_cp, m_tank_wall_dens, m_tank_wall_thick, m_nstep, m_piston_loss_poly);
    // Cold tank
    mc_cold_tank_cyl.init(mc_store_htfProps, m_vol_tank, m_h_tank_calc, m_h_tank_min,
        m_u_tank, m_tank_pairs, m_cold_tank_Thtr, m_cold_tank_max_heat,
        V_cold_ini, T_cold_ini, m_T_cold_des, m_tank_wall_cp, m_tank_wall_dens, m_tank_wall_thick, m_nstep, m_piston_loss_poly);
}

void C_csp_piston_cylinder_tes::discharge_avail_est(double T_cold_K, double step_s,
    double& q_dot_dc_est /*MWt*/, double& m_dot_external_est /*kg/s*/, double& T_hot_external_est /*K*/)
{
    double f_storage = 0.0;		// for now, hardcode such that storage always completely discharges

    double m_dot_tank_disch_avail = mc_hot_tank_cyl.m_dot_available(f_storage, step_s);	//[kg/s]

    if (m_dot_tank_disch_avail == 0) {
        q_dot_dc_est = 0.;
        m_dot_external_est = 0.;
        T_hot_external_est = std::numeric_limits<double>::quiet_NaN();
        return;
    }

    double T_hot_ini = mc_hot_tank_cyl.get_m_T_prev();		//[K]

    double cp_T_avg = mc_store_htfProps.Cp_ave(T_cold_K, T_hot_ini);		//[kJ/kg-K] spec heat at average temperature during discharge from hot to cold
    q_dot_dc_est = m_dot_tank_disch_avail * cp_T_avg * (T_hot_ini - T_cold_K) * 1.E-3;	//[MW]
    m_dot_external_est = m_dot_tank_disch_avail;
    T_hot_external_est = T_hot_ini;
    
}

void C_csp_piston_cylinder_tes::charge_avail_est(double T_hot_K, double step_s,
    double& q_dot_ch_est /*MWt*/, double& m_dot_external_est /*kg/s*/, double& T_cold_external_est /*K*/)
{
    // Only allow charge up to the 'main' tank volume
    double dens_cold = mc_store_htfProps.dens(m_T_cold_des, 1.0);
    double dens_hot = mc_store_htfProps.dens(m_T_hot_des, 1.0);

    //double f_ch_storage = 0;	// for now, hardcode such that storage always completely charges
    double f_ch_storage = 1.0 - dens_hot / dens_cold;


    // This is amount of cold mass in tank (available for use - i.e., not dead space volume)
    double m_dot_tank_charge_avail = mc_cold_tank_cyl.m_dot_available(f_ch_storage, step_s);	//[kg/s]

    double T_cold_ini = mc_cold_tank_cyl.get_m_T_prev();	//[K]

    // for debugging
    double T_hot_ini = mc_hot_tank_cyl.get_m_T_prev();      //[K]
    double cp_T_tanks_avg = mc_store_htfProps.Cp_ave(T_cold_ini, T_hot_ini);   // [kJ/kg-K]
    double mass_avail_hot_tank = mc_hot_tank_cyl.m_dot_available(f_ch_storage, step_s) * step_s;   //[kg]
    double tes_charge_state = mass_avail_hot_tank * cp_T_tanks_avg * (T_hot_ini - T_cold_ini) * 1.e-3 / 3600.;  // [MWht]

    double cp_T_avg = mc_store_htfProps.Cp_ave(T_cold_ini, T_hot_K);	//[kJ/kg-K] spec heat at average temperature during charging from cold to hot
    q_dot_ch_est = m_dot_tank_charge_avail * cp_T_avg * (T_hot_K - T_cold_ini) * 1.E-3;	//[MW]
    m_dot_external_est = m_dot_tank_charge_avail;	//[kg/s]
    T_cold_external_est = T_cold_ini;				//[K]
    
}

int C_csp_piston_cylinder_tes::solve_tes_off_design(double timestep /*s*/, double  T_amb /*K*/,
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

    // Total mass flow leaving cold tank to cr
    // One of the RHS should always be 0...
    double m_dot_cv_cold_to_cr = m_dot_cr_to_cv_hot + m_dot_cr_to_cv_cold;

    s_outputs = S_csp_tes_outputs();

    double m_dot_cr_to_tes_hot, m_dot_cr_to_tes_cold, m_dot_tes_hot_out, m_dot_pc_to_tes_cold, m_dot_tes_cold_out, m_dot_tes_cold_in;
    m_dot_cr_to_tes_hot = m_dot_cr_to_tes_cold = m_dot_tes_hot_out = m_dot_pc_to_tes_cold = m_dot_tes_cold_out = m_dot_tes_cold_in = std::numeric_limits<double>::quiet_NaN();
    double m_dot_src_to_sink, m_dot_sink_to_src;
    m_dot_src_to_sink = m_dot_sink_to_src = std::numeric_limits<double>::quiet_NaN();


    if (tanks_in_parallel)
    {
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
    }
    else
    {   // Serial configuration
        throw C_csp_exception("Tank model must be in parallel");
    }

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
    double q_dot_out_hot = std::numeric_limits<double>::quiet_NaN();		//[MW]  Energy leaving in fluid on hot side
    double q_dot_out_cold = std::numeric_limits<double>::quiet_NaN();		//[MW]  Energy leaving in fluid on cold side
    double q_dot_error_hot = std::numeric_limits<double>::quiet_NaN();		//[MW]  Energy Balance Error Hot Side
    double q_dot_error_cold = std::numeric_limits<double>::quiet_NaN();		//[MW]  Energy Balance Error Cold Side
    double q_dot_error_total = std::numeric_limits<double>::quiet_NaN();	//[MW]  Energy Balance Total Error
    double q_dot_error_leak = std::numeric_limits<double>::quiet_NaN();		//[MW]  Energy Balance Error due to Leakage Assumption
    double q_dot_error_wall = std::numeric_limits<double>::quiet_NaN();		//[MW]  Energy Balance Error due to Wall Assumption
    double q_dot_error_corrected = std::numeric_limits<double>::quiet_NaN();//[MW]  Energy Balance Error adjusted for leakage and wall assumption

    if (tanks_in_parallel)
    {

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
                T_hot_ave, T_cold_ave, T_hot_final, T_cold_final, q_dot_out_cold, q_dot_out_hot, q_dot_error_cold, q_dot_error_hot,
                q_dot_error_total, q_dot_error_leak, q_dot_error_wall, q_dot_error_corrected);

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
        else    // Discharging
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
                T_hot_ave, T_cold_ave, T_hot_final, T_cold_final, q_dot_out_cold, q_dot_out_hot, q_dot_error_cold, q_dot_error_hot,
                q_dot_error_total, q_dot_error_leak, q_dot_error_wall, q_dot_error_corrected);

            m_dot_cold_tank_to_hot_tank *= -1.0;

            // Check if discharge method solved
            if (!is_tes_success)
            {
                return -4;
            }

            T_sink_htf_in_hot = (m_dot_tes_dc * T_htf_tes_hot + m_dot_cr_to_cv_hot * T_cr_out_hot) / m_dot_cv_hot_to_sink;   //[K]
        }


    }
    else // Serial tank operation
    {
        throw C_csp_exception("Tank model must be in parallel");
    }

    // TOTAL Energy Balance for total system here
    double energy_balance_error_percent = (q_dot_error_total / m_q_dot_design) * 100.0; // [%] Energy balance power Error / design heat rate

    // Calculate Expansion Tank Size
    double expansion_wall_mass = 0.0;
    double expansion_wall_L = 0.0;
    {
        double wall_mass_hot = mc_hot_tank_cyl.get_m_m_wall_calc();
        double wall_mass_cold = mc_cold_tank_cyl.get_m_m_wall_calc();

        double L_hot = mc_hot_tank_cyl.get_m_L_calc();
        double L_cold = mc_cold_tank_cyl.get_m_L_calc();

        expansion_wall_mass = (wall_mass_hot + wall_mass_cold) - m_mass_wall_nominal;
        expansion_wall_L = (L_hot + L_cold) - m_h_tank_calc; // This is length of 'bonus' tank 
    }


    // Solve pumping power here
    double W_dot_htf_pump = pumping_power(m_dot_cr_to_cv_hot, m_dot_cv_hot_to_sink, std::abs(m_dot_cold_tank_to_hot_tank),
        T_cr_in_cold, T_cr_out_hot, T_sink_htf_in_hot, T_sink_out_cold,
        false);     //[-] C_MEQ__m_dot_tes will not send cr_m_dot to TES if recirculating


    // Calculate NT Specific Outputs
    double mass_cold = mc_cold_tank_cyl.get_m_m_calc();
    double mass_hot = mc_hot_tank_cyl.get_m_m_calc();
    double vol_cold = mc_cold_tank_cyl.get_fluid_vol();
    double vol_hot = mc_hot_tank_cyl.get_fluid_vol();
    double vol_tot = vol_cold + vol_hot;
    double vol_tot_assigned = CSP::pi * std::pow(m_radius, 2.0) * m_length_total;
    double mass_tot = mass_cold + mass_hot;

    double cold_frac = vol_cold / vol_tot;
    double piston_loc, piston_frac;
    calc_piston_location(piston_loc, piston_frac);

    s_outputs.m_q_heater = q_dot_heater;
    s_outputs.m_W_dot_elec_in_tot = W_dot_htf_pump;             //[MWe]
    s_outputs.m_q_dot_dc_to_htf = q_dot_dc_to_htf;
    s_outputs.m_q_dot_ch_from_htf = q_dot_ch_from_htf;
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
    mc_reported_outputs.value(E_MASS_COLD_TANK, mc_cold_tank_cyl.get_m_m_calc());		//[kg]
    mc_reported_outputs.value(E_MASS_HOT_TANK, mc_hot_tank_cyl.get_m_m_calc());			//[kg]
    mc_reported_outputs.value(E_W_DOT_HTF_PUMP, W_dot_htf_pump);    //[MWe]

    // Added NT Outputs
    mc_reported_outputs.value(E_VOL_COLD, vol_cold);        //[m3]
    mc_reported_outputs.value(E_VOL_HOT, vol_hot);          //[m3]
    mc_reported_outputs.value(E_VOL_TOT, vol_tot);          //[m3]
    mc_reported_outputs.value(E_PIST_LOC, piston_loc);      //[m]
    mc_reported_outputs.value(E_PIST_FRAC, piston_frac);    //[]
    mc_reported_outputs.value(E_COLD_FRAC, cold_frac);      //[]
    mc_reported_outputs.value(E_MASS_TOT, mass_tot);        //[kg]
    mc_reported_outputs.value(E_SA_COLD, mc_cold_tank_cyl.get_SA_calc());    //[m2]
    mc_reported_outputs.value(E_SA_HOT, mc_hot_tank_cyl.get_SA_calc());    //[m2]
    mc_reported_outputs.value(E_SA_TOT, mc_cold_tank_cyl.get_SA_calc() + mc_hot_tank_cyl.get_SA_calc());    //[m2]
    mc_reported_outputs.value(E_ERROR, q_dot_error_total); //[MW]
    mc_reported_outputs.value(E_ERROR_PERCENT, energy_balance_error_percent); //[%]
    mc_reported_outputs.value(E_LEAK_ERROR, q_dot_error_leak);  //[MWt]
    mc_reported_outputs.value(E_E_HOT, mc_hot_tank_cyl.get_m_E_calc());  //[MJ]
    mc_reported_outputs.value(E_E_COLD, mc_cold_tank_cyl.get_m_E_calc());//[MJ]
    mc_reported_outputs.value(E_E_COLD, mc_cold_tank_cyl.get_m_E_calc());//[MJ]
    mc_reported_outputs.value(E_WALL_ERROR, q_dot_error_wall);  //[MWt]
    mc_reported_outputs.value(E_ERROR_CORRECTED, q_dot_error_corrected);   //[MW]
    mc_reported_outputs.value(E_EXP_WALL_MASS, expansion_wall_mass);    //[kg]
    mc_reported_outputs.value(E_EXP_LENGTH, expansion_wall_L);      //[m]

    return 0;
}

void C_csp_piston_cylinder_tes::converged()
{
    mc_cold_tank_cyl.converged();
    mc_hot_tank_cyl.converged();
    //mc_hx.converged();

    // Set reported sink converged values
    mc_reported_outputs.value(E_HOT_TANK_HTF_PERC_FINAL, mc_hot_tank_cyl.get_mass_avail() / m_mass_total_active * 100.0);

    mc_reported_outputs.set_timestep_outputs();

    // The max charge and discharge flow rates should be set at the beginning of each timestep
    //   during the q_dot_xx_avail_est calls
    // m_m_dot_tes_dc_max = m_m_dot_tes_ch_max = std::numeric_limits<double>::quiet_NaN();
}

void C_csp_piston_cylinder_tes::write_output_intervals(double report_time_start,
    const std::vector<double>& v_temp_ts_time_end, double report_time_end)
{
    mc_reported_outputs.send_to_reporting_ts_array(report_time_start,
        v_temp_ts_time_end, report_time_end);
}

void C_csp_piston_cylinder_tes::assign(int index, double* p_reporting_ts_array, size_t n_reporting_ts_array)
{
    mc_reported_outputs.assign(index, p_reporting_ts_array, n_reporting_ts_array);
}

double /*MWe*/ C_csp_piston_cylinder_tes::pumping_power(double m_dot_sf /*kg/s*/, double m_dot_pb /*kg/s*/, double m_dot_tank /*kg/s*/,
    double T_sf_in /*K*/, double T_sf_out /*K*/, double T_pb_in /*K*/, double T_pb_out /*K*/, bool recirculating)
{
    // Piston Cylinder never uses a hx -> pumping power = 0 (following two tank logic)
    // Might want to use tes_pump_coef here?

    double htf_pump_power = 0.0;	//[MWe]

    return htf_pump_power;
}


void C_csp_piston_cylinder_tes::get_design_parameters(double& vol_one_temp_avail /*m3*/, double& vol_one_temp_total /*m3*/,
    double& h_tank_calc /*m*/, double& d_tank_calc /*m*/,
    double& q_dot_loss_des /*MWt*/, double& dens_store_htf_at_T_ave /*kg/m3*/, double& Q_tes /*MWt-hr*/)
{
    vol_one_temp_avail = m_V_tank_active;   //[m3]
    vol_one_temp_total = m_vol_tank;        //[m3]
    h_tank_calc = m_h_tank_calc;            //[m]
    d_tank_calc = m_d_tank_calc;            //[m]
    q_dot_loss_des = m_q_dot_loss_des;      //[MWt]
    dens_store_htf_at_T_ave = m_rho_store_avg;  //[kg/m3]
    Q_tes = m_Q_tes_des;                    //[MWt-hr]
}

bool C_csp_piston_cylinder_tes::charge(double timestep /*s*/, double T_amb /*K*/, double m_dot_htf_in /*kg/s*/,
    double T_htf_hot_in /*K*/, double& T_htf_cold_out /*K*/,
    double& q_dot_heater /*MWe*/, double& m_dot_tank_to_tank /*kg/s*/, double& W_dot_rhtf_pump /*MWe*/,
    double& q_dot_loss /*MWt*/, double& q_dot_dc_to_htf /*MWt*/, double& q_dot_ch_from_htf /*MWt*/,
    double& T_hot_ave /*K*/, double& T_cold_ave /*K*/, double& T_hot_final /*K*/, double& T_cold_final /*K*/,
    double& q_dot_out_cold /*MW*/, double& q_dot_out_hot /*MW*/, double& q_dot_error_cold, double& q_dot_error_hot,
    double& q_dot_error_total /*MW*/, double& q_dot_error_leak /*MW*/, double& q_dot_error_wall /*MW*/,
    double& q_dot_error_corrected /*MW*/)
{
    // This method calculates the timestep-average cold charge return temperature of the TES system.
    // This is out of the external side of the heat exchanger (HX), opposite the tank (or 'TES') side,
    // or if no HX (direct storage), this is equal to the cold tank outlet temperature.

    // The method returns FALSE if the system input mass flow rate is greater than the allowable charge 

    // Inputs are:
    // 1) Mass flow rate of HTF into TES system (equal to that exiting the system)
    // 2) Temperature of HTF into TES system. If no heat exchanger, this temperature
    //	   is of the HTF directly entering the hot tank

    // DEBUG
    double piston_loc, piston_frac;
    calc_piston_location(piston_loc, piston_frac);

    double q_dot_ch_est, m_dot_tes_ch_max, T_cold_to_src_est;
    q_dot_ch_est = m_dot_tes_ch_max = T_cold_to_src_est = std::numeric_limits<double>::quiet_NaN();
    charge_avail_est(T_htf_hot_in, timestep, q_dot_ch_est, m_dot_tes_ch_max, T_cold_to_src_est);

    double m_dot_htf_in_net = m_dot_htf_in * (1.0 - mc_hot_tank_cyl.calc_leakage_fraction(m_dot_htf_in));

    if (m_dot_htf_in_net > 1.0001 * m_dot_tes_ch_max && m_dot_htf_in_net > 1.E-6)
    {
        q_dot_heater = std::numeric_limits<double>::quiet_NaN();
        m_dot_tank_to_tank = std::numeric_limits<double>::quiet_NaN();
        W_dot_rhtf_pump = std::numeric_limits<double>::quiet_NaN();
        q_dot_loss = std::numeric_limits<double>::quiet_NaN();
        q_dot_dc_to_htf = std::numeric_limits<double>::quiet_NaN();
        q_dot_ch_from_htf = std::numeric_limits<double>::quiet_NaN();
        T_hot_ave = std::numeric_limits<double>::quiet_NaN();
        T_cold_ave = std::numeric_limits<double>::quiet_NaN();
        T_hot_final = std::numeric_limits<double>::quiet_NaN();
        T_cold_final = std::numeric_limits<double>::quiet_NaN();

        return false;
    }

    double q_heater_cold, q_heater_hot, q_dot_loss_cold, q_dot_loss_hot, m_dot_src,
        T_src_hot_in, T_src_cold_out, m_dot_tank, T_hot_tank_in;
    q_heater_cold = q_heater_hot = q_dot_loss_cold = q_dot_loss_hot = T_cold_ave = T_hot_ave = m_dot_src =
        T_src_hot_in = T_src_cold_out = m_dot_tank = T_hot_tank_in = std::numeric_limits<double>::quiet_NaN();

    m_dot_src = m_dot_tank = m_dot_htf_in;
    T_src_hot_in = T_hot_tank_in = T_htf_hot_in;

    double T_hot_prev = mc_hot_tank_cyl.get_m_T_prev();
    double T_cold_prev = mc_cold_tank_cyl.get_m_T_prev();

    solve_tanks_iterative(timestep, m_nstep, m_dot_tank, 0, T_hot_tank_in, 0, T_amb,
        T_cold_ave, q_heater_cold, q_dot_loss_cold, q_dot_out_cold, q_dot_error_cold,
        T_hot_ave, q_heater_hot, q_dot_loss_hot, q_dot_out_hot, q_dot_error_hot,
        q_dot_error_total, q_dot_error_leak, q_dot_error_wall, q_dot_error_corrected);

    q_dot_heater = q_heater_cold + q_heater_hot;			//[MWt]


    m_dot_tank_to_tank = 0.0;
    W_dot_rhtf_pump = 0;                          //[MWe] Just tank-to-tank pumping power
    T_htf_cold_out = T_cold_ave;

    q_dot_loss = q_dot_loss_cold + q_dot_loss_hot;	//[MWt]
    q_dot_dc_to_htf = 0.0;							//[MWt]
    T_hot_ave = T_hot_ave;							//[K]
    T_cold_ave = T_cold_ave;  						//[K]
    T_hot_final = mc_hot_tank_cyl.get_m_T_calc();			//[K]
    T_cold_final = mc_cold_tank_cyl.get_m_T_calc();		//[K]

    // Calculate thermal power to HTF
    double cp_htf_ave = mc_external_htfProps.Cp_ave(T_htf_cold_out, T_htf_hot_in);		//[kJ/kg-K]
    q_dot_ch_from_htf = m_dot_htf_in * cp_htf_ave * (T_htf_hot_in - T_htf_cold_out) / 1000.0;		//[MWt]


    return true;

}

bool C_csp_piston_cylinder_tes::discharge(double timestep /*s*/, double T_amb /*K*/, double m_dot_htf_in /*kg/s*/,
    double T_htf_cold_in /*K*/, double& T_htf_hot_out /*K*/,
    double& q_dot_heater /*MWe*/, double& m_dot_tank_to_tank /*kg/s*/, double& W_dot_rhtf_pump /*MWe*/,
    double& q_dot_loss /*MWt*/, double& q_dot_dc_to_htf /*MWt*/, double& q_dot_ch_from_htf /*MWt*/,
    double& T_hot_ave /*K*/, double& T_cold_ave /*K*/, double& T_hot_final /*K*/, double& T_cold_final /*K*/,
    double& q_dot_out_cold /*MW*/, double& q_dot_out_hot /*MW*/, double& q_dot_error_cold, double& q_dot_error_hot,
    double& q_dot_error_total /*MW*/, double& q_dot_error_leak /*MW*/, double& q_dot_error_wall /*MW*/,
    double& q_dot_error_corrected /*MW*/)
{
    // This method calculates the timestep-average hot discharge temperature of the TES system. This is out of the external side of the heat exchanger (HX), opposite the tank (or 'TES') side,
    // or if no HX (direct storage), this is equal to the hot tank outlet temperature.

    // The method returns FALSE if the system output (same as input) mass flow rate is greater than that available

    // Inputs are:
    // 1) Mass flow rate of HTF into TES system (equal to that exiting the system)
    // 2) Temperature of HTF into TES system. If no heat exchanger, this temperature
    //	   is of the HTF directly entering the cold tank

    double q_dot_dc_est, m_dot_tes_dc_max, T_hot_to_pc_est;
    q_dot_dc_est = m_dot_tes_dc_max = T_hot_to_pc_est = std::numeric_limits<double>::quiet_NaN();
    discharge_avail_est(T_htf_cold_in, timestep, q_dot_dc_est, m_dot_tes_dc_max, T_hot_to_pc_est);

    double m_dot_htf_in_net = m_dot_htf_in * (1.0 - mc_hot_tank_cyl.calc_leakage_fraction(m_dot_htf_in));

    if (m_dot_htf_in_net > 1.0001 * m_dot_tes_dc_max && m_dot_htf_in_net > 1.E-6)      // mass flow in = mass flow out
    {
        q_dot_heater = std::numeric_limits<double>::quiet_NaN();
        m_dot_tank_to_tank = std::numeric_limits<double>::quiet_NaN();
        W_dot_rhtf_pump = std::numeric_limits<double>::quiet_NaN();
        q_dot_loss = std::numeric_limits<double>::quiet_NaN();
        q_dot_dc_to_htf = std::numeric_limits<double>::quiet_NaN();
        q_dot_ch_from_htf = std::numeric_limits<double>::quiet_NaN();
        T_hot_ave = std::numeric_limits<double>::quiet_NaN();
        T_cold_ave = std::numeric_limits<double>::quiet_NaN();
        T_hot_final = std::numeric_limits<double>::quiet_NaN();
        T_cold_final = std::numeric_limits<double>::quiet_NaN();

        return false;
    }

    double q_heater_cold, q_heater_hot, q_dot_loss_cold, q_dot_loss_hot, m_dot_src,
        T_src_cold_in, T_src_hot_out, m_dot_tank, T_cold_tank_in;
    q_heater_cold = q_heater_hot = q_dot_loss_cold = q_dot_loss_hot = T_cold_ave = T_hot_ave =
        m_dot_src = T_src_cold_in = T_src_hot_out = m_dot_tank = T_cold_tank_in = std::numeric_limits<double>::quiet_NaN();

    m_dot_src = m_dot_tank = m_dot_htf_in;
    T_src_cold_in = T_cold_tank_in = T_htf_cold_in;

    double T_hot_prev = mc_hot_tank_cyl.get_m_T_prev();
    double T_cold_prev = mc_cold_tank_cyl.get_m_T_prev();

    solve_tanks_iterative(timestep, m_nstep, 0, m_dot_tank, 0, T_cold_tank_in, T_amb,
        T_cold_ave, q_heater_cold, q_dot_loss_cold, q_dot_out_cold, q_dot_error_cold,
        T_hot_ave, q_heater_hot, q_dot_loss_hot, q_dot_out_hot, q_dot_error_hot,
        q_dot_error_total, q_dot_error_leak, q_dot_error_wall, q_dot_error_corrected);

    q_dot_heater = q_heater_cold + q_heater_hot;			//[MWt]

    m_dot_tank_to_tank = 0.0;
    W_dot_rhtf_pump = 0;                          //[MWe] Just tank-to-tank pumping power
    T_htf_hot_out = T_hot_ave;

    q_dot_loss = q_dot_loss_cold + q_dot_loss_hot;	//[MWt]
    q_dot_ch_from_htf = 0.0;		                    //[MWt]
    T_hot_ave = T_hot_ave;						    //[K]
    T_cold_ave = T_cold_ave;							//[K]
    T_hot_final = mc_hot_tank_cyl.get_m_T_calc();			//[K]
    T_cold_final = mc_cold_tank_cyl.get_m_T_calc();		//[K]

    // Calculate thermal power to HTF
    double cp_htf_ave = mc_external_htfProps.Cp_ave(T_htf_cold_in, T_htf_hot_out);		//[kJ/kg-K]
    q_dot_dc_to_htf = m_dot_htf_in * cp_htf_ave * (T_htf_hot_out - T_htf_cold_in) / 1000.0;		//[MWt]

    return true;
}

void C_csp_piston_cylinder_tes::solve_tanks_iterative(double timestep /*s*/, double n_substep /**/, double mdot_charge /*kg/s*/,
    double mdot_discharge /*kg/s*/, double T_charge /*K*/, double T_discharge /*K*/, double T_amb /*K*/,
    double& T_ave_cold /*K*/, double& q_heater_cold /*MW*/, double& q_dot_loss_cold /*MW*/,
    double& q_dot_out_cold /*MW*/, double& q_dot_error_cold /*MW*/,
    double& T_ave_hot /*K*/, double& q_heater_hot /*MW*/, double& q_dot_loss_hot /*MW*/,
    double& q_dot_out_hot /*MW*/, double& q_dot_error_hot /*MW*/,
    double& q_dot_error_total /*MW*/, double& q_dot_error_leak /*MW*/, double& q_dot_error_wall /*MW*/,
    double& q_dot_error_corrected /*MW*/)
{
    double ministep = timestep / n_substep;

    // Define Cold internal variables
    double q_heater_summed_cold = 0;
    double q_dot_loss_summed_cold = 0;
    double T_ave_innerstep_cold = 0;
    double mass_prev_inner_cold = mc_cold_tank_cyl.get_m_m_prev();
    double mass_calc_inner_cold = std::numeric_limits<double>::quiet_NaN();
    double T_prev_inner_cold = mc_cold_tank_cyl.get_m_T_prev();
    double T_calc_inner_cold = std::numeric_limits<double>::quiet_NaN();
    double T_ave_weighted_cold = 0;
    double q_dot_out_summed_cold = 0;
    double q_dot_error_summed_cold = 0;

    // Define Hot internal variables
    double q_heater_summed_hot = 0;
    double q_dot_loss_summed_hot = 0;
    double T_ave_innerstep_hot = 0;
    double mass_prev_inner_hot = mc_hot_tank_cyl.get_m_m_prev();
    double mass_calc_inner_hot = std::numeric_limits<double>::quiet_NaN();
    double T_prev_inner_hot = mc_hot_tank_cyl.get_m_T_prev();
    double T_calc_inner_hot = std::numeric_limits<double>::quiet_NaN();
    double T_ave_weighted_hot = 0;
    double q_dot_out_summed_hot = 0;
    double q_dot_error_summed_hot = 0;

    // Define Energy Balance Error Variables
    double error_avg = 0; //[MW]
    double error_leak_avg = 0;   // [MW]
    double error_wall_avg = 0;   // [MW]
    double error_corrected_avg = 0;

    // Sort out charge/discharge params
    double mdot_in_cold = 0;
    double mdot_out_cold = 0;
    double mdot_in_hot = 0;
    double mdot_out_hot = 0;
    double T_in_cold = 0;
    double T_in_hot = 0;
    {
        if (mdot_charge != 0 && mdot_discharge != 0)
        {
            throw(C_csp_exception("NT Tank cannot charge and discharge at once", "NT Tank Energy Balance"));
        }
        // Charging 
        if (mdot_charge > 0)
        {
            mdot_in_cold = 0;
            mdot_out_cold = mdot_charge;
            T_in_cold = 0; // (no temperature coming in)
            mdot_in_hot = mdot_charge;
            mdot_out_hot = 0;
            T_in_hot = T_charge;
        }
        // Discharging
        else
        {
            mdot_in_cold = mdot_discharge;
            mdot_out_cold = 0;
            T_in_cold = T_discharge;
            mdot_in_hot = 0;
            mdot_out_hot = mdot_discharge;
            T_in_hot = 0; // (no temperature coming in)
        }
    }

    // Run Energy Balance
    for (int i = 0; i < n_substep; i++)
    {
        double q_heater_innerstep_cold, q_dot_loss_innerstep_cold, q_dot_error_innerstep_cold, q_dot_out_inner_cold
            = std::numeric_limits<double>::quiet_NaN();

        double q_heater_innerstep_hot, q_dot_loss_innerstep_hot, q_dot_error_innerstep_hot, q_dot_out_inner_hot
            = std::numeric_limits<double>::quiet_NaN();

        // Simulate Cold Tank
        mc_cold_tank_cyl.energy_balance_core(ministep, mdot_in_cold, mdot_out_cold, T_in_cold, T_amb, mass_prev_inner_cold,
            T_prev_inner_hot, T_prev_inner_cold, T_prev_inner_hot,
            T_ave_innerstep_cold, q_heater_innerstep_cold, q_dot_loss_innerstep_cold, mass_calc_inner_cold, T_calc_inner_cold, q_dot_out_inner_cold,
            q_dot_error_innerstep_cold);

        // Simulate Hot Tank
        mc_hot_tank_cyl.energy_balance_core(ministep, mdot_in_hot, mdot_out_hot, T_in_hot, T_amb, mass_prev_inner_hot,
            T_prev_inner_cold, T_prev_inner_hot, T_prev_inner_cold,
            T_ave_innerstep_hot, q_heater_innerstep_hot, q_dot_loss_innerstep_hot, mass_calc_inner_hot, T_calc_inner_hot, q_dot_out_inner_hot,
            q_dot_error_innerstep_hot);

        // TOTAL Energy Balance for TES system here
        double energy_balance_error = 0;    // [MW]
        double energy_error_leakage = 0;    // [MW]
        double energy_error_wall = 0;   // [MW]
        double energy_error_corrected = 0;  //[MW]
        {
            // Cold Fluid Energy In (if Discharging)
            double Q_cold_in;   // MJ
            { 
                double mdot_cold_in = mdot_discharge;       // kg/s
                double cp_cold_in = mc_external_htfProps.Cp(T_in_cold) * 1e-3;  // MJ/kg K

                Q_cold_in = T_in_cold * mdot_cold_in * cp_cold_in * ministep;   // MJ
            }

            // Hot Fluid Energy In (if Charging)
            double Q_hot_in;    // MJ
            {
                double mdot_hot_in = mdot_charge;          // kg/s
                double cp_hot_in = mc_external_htfProps.Cp(T_in_hot) * 1e-3;  // MJ/kg K

                Q_hot_in = T_in_hot * mdot_hot_in * cp_hot_in * ministep;   // MJ
            }

            // Cold Fluid Energy Out (if Charging)
            double Q_cold_out; // MJ
            {
                double mdot_cold_out = mdot_charge;          // kg/s
                double cp_cold_out = mc_external_htfProps.Cp(T_ave_innerstep_cold) * 1e-3;    // MJ/kg K

                Q_cold_out = T_ave_innerstep_cold * mdot_cold_out * cp_cold_out * ministep;  // MJ
            }

            // Hot Fluid Energy Out (if Discharging)
            double Q_hot_out; // MJ
            {
                double mdot_hot_out = mdot_discharge;   // kg/s
                double cp_hot_out = mc_external_htfProps.Cp(T_ave_innerstep_hot) * 1e-3;  // MJ/kg K

                Q_hot_out = T_ave_innerstep_hot * mdot_hot_out * cp_hot_out * ministep;   // MJ
            }

            // Q Loss (to environment)
            double q_dot_loss = q_dot_loss_innerstep_cold + q_dot_loss_innerstep_hot;
            double Q_loss = q_dot_loss * ministep;  // MJ

            // Q Heater (Input energy)
            double q_dot_heater = q_heater_innerstep_cold + q_heater_innerstep_hot;
            double Q_heater = q_dot_heater * ministep; // MJ

            // Cold Side Energy Change
            double dQ_cold; // MJ
            double mass_wall_cold_prev;
            double mass_wall_cold_final;
            double Q_cold_wall_prev;
            double Q_cold_wall_final;
            {
                // Initial Energy
                double T_cold_prev = T_prev_inner_cold;
                double mass_fluid_cold_prev = mass_prev_inner_cold;
                mass_wall_cold_prev = mc_cold_tank_cyl.calc_mass_wall(T_cold_prev, mass_fluid_cold_prev); // kg
                double cp_cold_fluid_prev = mc_external_htfProps.Cp(T_cold_prev) * 1e-3;   // MJ/kg K

                double Q_cold_fluid_prev = T_cold_prev * mass_fluid_cold_prev * cp_cold_fluid_prev; // MJ
                Q_cold_wall_prev = T_cold_prev * mass_wall_cold_prev * m_tank_wall_cp * 1e-6;    // MJ

                // Final Energy
                double mass_fluid_cold_final = mass_calc_inner_cold;
                mass_wall_cold_final = mc_cold_tank_cyl.get_m_m_wall_calc();
                double cp_cold_fluid_final = mc_external_htfProps.Cp(T_calc_inner_cold) * 1e-3;   // MJ/kg K

                double Q_cold_fluid_final = T_calc_inner_cold * mass_fluid_cold_final * cp_cold_fluid_final; // MJ
                Q_cold_wall_final = T_calc_inner_cold * mass_wall_cold_final * m_tank_wall_cp * 1e-6;    // MJ

                dQ_cold = (Q_cold_fluid_final + Q_cold_wall_final) - (Q_cold_fluid_prev + Q_cold_wall_prev);   // MJ
            }

            // Hot Side Energy Change
            double dQ_hot; // MJ
            double mass_wall_hot_prev;
            double mass_wall_hot_final;
            double Q_hot_wall_final;
            double Q_hot_wall_prev;
            {
                // Initial Energy
                double T_hot_prev = T_prev_inner_hot;
                double mass_fluid_hot_prev = mass_prev_inner_hot;
                mass_wall_hot_prev = mc_hot_tank_cyl.calc_mass_wall(T_hot_prev, mass_fluid_hot_prev); // kg
                double cp_hot_fluid_prev = mc_external_htfProps.Cp(T_hot_prev) * 1e-3;   // MJ/kg K

                double Q_hot_fluid_prev = T_hot_prev * mass_fluid_hot_prev * cp_hot_fluid_prev; // MJ
                Q_hot_wall_prev = T_hot_prev * mass_wall_hot_prev * m_tank_wall_cp * 1e-6;    // MJ

                // Final Energy
                double T_hot = T_calc_inner_hot;
                double mass_fluid_hot_final = mass_calc_inner_hot;
                mass_wall_hot_final = mc_hot_tank_cyl.get_m_m_wall_calc();
                double cp_hot_fluid_final = mc_external_htfProps.Cp(T_hot) * 1e-3;   // MJ/kg K

                double Q_hot_fluid_final = T_hot * mass_fluid_hot_final * cp_hot_fluid_final; // MJ
                Q_hot_wall_final = T_hot * mass_wall_hot_final * m_tank_wall_cp * 1e-6;    // MJ

                dQ_hot = (Q_hot_fluid_final + Q_hot_wall_final) - (Q_hot_fluid_prev + Q_hot_wall_prev);   // MJ
            }

            // Energy In
            double Q_in_sum = Q_cold_in + Q_hot_in + Q_heater;  // MJ

            // Energy Out
            double Q_out_sum = Q_cold_out + Q_hot_out + Q_loss; // MJ

            // Net Energy
            double Q_net = Q_in_sum - Q_out_sum;    // MJ

            // Change in Energy
            double dQ_total = dQ_cold + dQ_hot; // MJ

            // Balance
            energy_balance_error = (dQ_total - Q_net) / ministep; // MW (should be zero)

            // Tank Wall Energy
            double mass_wall_prev = mass_wall_cold_prev + mass_wall_hot_prev;
            double mass_wall_final = mass_wall_cold_final + mass_wall_hot_final;
            double Q_wall_prev = Q_cold_wall_prev + Q_hot_wall_prev;
            double Q_wall_final = Q_cold_wall_final + Q_hot_wall_final;

            // Leakage Temperature Discrepancy
            {
                // Leakage is assumed at a constant temp, while the other side has variable temps
                // i.e. During charge, cold side takes on leakage at constant temp, while hot is leaking variable temp
                double mdot_leakage;
                double mdot_net = mdot_charge + mdot_discharge;
                {
                    double N_terms = m_piston_loss_poly.size();
                    double frac = 0;
                    for (int i = 0; i < N_terms; i++)
                    {
                        frac += m_piston_loss_poly[i] * std::pow(mdot_net, i);
                    }

                    frac *= 0.01;   // Convert from % to fraction
                    mdot_leakage = mdot_net * frac;
                }

                // Charge
                if (mdot_charge > 0)
                {
                    double T_leakage_assumed = T_prev_inner_hot;   // [K] Cold tank assumes leakage is constant temp at prev timestep from hot tank
                    double cp_leakage_assumed = mc_store_htfProps.Cp(T_leakage_assumed) * 1e-3;   // [MJ/kg K]

                    double T_leakage_actual = T_ave_innerstep_hot;                      // [K] Actual leakage temp will be hot tank average
                    double cp_leakage_actual = mc_store_htfProps.Cp(T_leakage_actual) * 1e-3;   // [MJ/kg K]

                    double Q_dot_leakage_assumed = T_leakage_assumed * cp_leakage_assumed * mdot_leakage;   // [MW]
                    double Q_dot_leakage_actual = T_leakage_actual * cp_leakage_actual * mdot_leakage;      // [MW]

                    energy_error_leakage = Q_dot_leakage_actual - Q_dot_leakage_assumed;    // [MW]
                }
                // Discharge
                else if (mdot_discharge > 0)
                {
                    double T_leakage_assumed = T_prev_inner_cold;   // [K] Hot tank assumes leakage is constant temp at prev timestep from cold tank
                    double cp_leakage_assumed = mc_store_htfProps.Cp(T_leakage_assumed) * 1e-3;   // [MJ/kg K]

                    double T_leakage_actual = T_ave_innerstep_cold;                      // [K] Actual leakage temp will be hot tank average
                    double cp_leakage_actual = mc_store_htfProps.Cp(T_leakage_actual) * 1e-3;   // [MJ/kg K]

                    double Q_dot_leakage_assumed = T_leakage_assumed * cp_leakage_assumed * mdot_leakage;   // [MW]
                    double Q_dot_leakage_actual = T_leakage_actual * cp_leakage_actual * mdot_leakage;      // [MW]

                    energy_error_leakage = Q_dot_leakage_actual - Q_dot_leakage_assumed;    // [MW]
                }
            }

            // Wall Temperature Discrepancy
            {
                // Tank wall mass is assumed at a constant temp, while the other side has variable temp
                // i.e. During charge, hot side takes on cold mass at constant temp. Cold side actually has variable temp

                // Charge
                if (mdot_charge > 0)
                {
                    // Hot side is expanding, taking on cold wall
                    double T_wall_assumed = T_prev_inner_cold; //[K]
                    double T_wall_actual = T_ave_innerstep_cold;  //[K]
                    double mdot_wall = (mc_hot_tank_cyl.get_m_m_wall_calc() - mass_wall_hot_prev) / ministep;

                    energy_error_wall = mdot_wall * m_tank_wall_cp * (T_wall_actual - T_wall_assumed) * 1e-6;   //[MW]
                }
                // Discharge
                else if (mdot_discharge > 0)
                {
                    // Cold side is expanding, taking on hot wall
                    double T_wall_assumed = T_prev_inner_hot; //[K]
                    double T_wall_actual = T_ave_innerstep_hot;  //[K]
                    double mdot_wall = (mc_cold_tank_cyl.get_m_m_wall_calc() - mass_wall_cold_prev) / ministep;

                    energy_error_wall = mdot_wall * m_tank_wall_cp * (T_wall_actual - T_wall_assumed) * 1e-6;   //[MW]
                }
            }

            // Corrected Error (accounting for leakage and wall temp assumptions)
            energy_error_corrected = std::abs(energy_balance_error) - std::abs(energy_error_leakage + energy_error_wall);   //[MW]
        }

        // Collect Cold Results
        q_heater_summed_cold += q_heater_innerstep_cold * (ministep / timestep);
        q_dot_loss_summed_cold += q_dot_loss_innerstep_cold * (ministep / timestep);
        q_dot_out_summed_cold += q_dot_out_inner_cold;
        q_dot_error_summed_cold += q_dot_error_innerstep_cold;
        mass_prev_inner_cold = mass_calc_inner_cold;
        T_prev_inner_cold = T_calc_inner_cold;
        T_ave_weighted_cold += T_ave_innerstep_cold * (ministep / timestep);

        // Collect Hot Results
        q_heater_summed_hot += q_heater_innerstep_hot * (ministep / timestep);
        q_dot_loss_summed_hot += q_dot_loss_innerstep_hot * (ministep / timestep);
        q_dot_out_summed_hot += q_dot_out_inner_hot;
        q_dot_error_summed_hot += q_dot_error_innerstep_hot;
        mass_prev_inner_hot = mass_calc_inner_hot;
        T_prev_inner_hot = T_calc_inner_hot;
        T_ave_weighted_hot += T_ave_innerstep_hot * (ministep / timestep);

        error_avg += energy_balance_error * (ministep / timestep);       //[MW]
        error_leak_avg += energy_error_leakage * (ministep / timestep);  //[MW]
        error_wall_avg += energy_error_wall * (ministep / timestep);     //[MW]
        error_corrected_avg += energy_error_corrected * (ministep / timestep);  //[MW]
    }

    T_ave_cold = T_ave_weighted_cold;
    q_heater_cold = q_heater_summed_cold;
    q_dot_loss_cold = q_dot_loss_summed_cold;
    q_dot_out_cold = q_dot_out_summed_cold;
    q_dot_error_cold = q_dot_error_summed_cold;


    T_ave_hot = T_ave_weighted_hot;
    q_heater_hot = q_heater_summed_hot;
    q_dot_loss_hot = q_dot_loss_summed_hot;
    q_dot_out_hot = q_dot_out_summed_hot;
    q_dot_error_hot = q_dot_error_summed_hot;

    q_dot_error_total = error_avg;
    q_dot_error_leak = error_leak_avg;
    q_dot_error_wall = error_wall_avg;
    q_dot_error_corrected = error_corrected_avg;

}

double C_csp_piston_cylinder_tes::get_min_storage_htf_temp()
{
    return mc_store_htfProps.min_temp();
}

double C_csp_piston_cylinder_tes::get_max_storage_htf_temp()
{
    return mc_store_htfProps.max_temp();
}

double C_csp_piston_cylinder_tes::get_storage_htf_density()
{
    double avg_temp = (m_T_cold_des + m_T_hot_des) / 2.0;
    return mc_store_htfProps.dens(avg_temp, 0);
}

double C_csp_piston_cylinder_tes::get_storage_htf_cp()
{
    double avg_temp = (m_T_cold_des + m_T_hot_des) / 2.0;
    return mc_store_htfProps.Cp(avg_temp);
}

void C_csp_piston_cylinder_tes::calc_piston_location(double &piston_loc, double &piston_frac)
{
    double vol_cold = mc_cold_tank_cyl.get_fluid_vol_prev();
    double vol_hot = mc_hot_tank_cyl.get_fluid_vol_prev();

    double length_cold = vol_cold / (CSP::pi * std::pow(m_radius, 2.0));
    double length_hot = vol_hot / (CSP::pi * std::pow(m_radius, 2.0));
    double length_tot = length_cold + length_hot;
    //if (length_tot != m_length_total)
    //{
    //    throw C_csp_exception("Calculated total length does not equal assigned total length");
    //}

    piston_loc = length_cold;   // Distance from left (cold) side
    piston_frac = length_cold / length_tot;
    
}


