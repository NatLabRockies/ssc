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


#include "core.h"
#include "water_properties.h"
#include "heat_exchangers.h"

static var_info _cm_vtab_csp_heatsink[] = {
    /* VARTYPE          DATATYPE         NAME                         LABEL                                                                               UNITS           META              GROUP             REQUIRED_IF                CONSTRAINTS         UI_HINTS*/
    // Inputs
    { SSC_INPUT,        SSC_NUMBER,      "t_step",                    "Timestep duration",                                                                "s",            "",               "system",         "",                       "",                      "" },
    
    var_info_invalid };

class cm_csp_heatsink : public compute_module
{
public:
    cm_csp_heatsink()
    {
        add_var_info(_cm_vtab_csp_heatsink);
    }

    void exec()
    {
        // Test HTF-CO2 HX design
        if (false) {
            C_HX_co2_to_htf mc_phx;

            int hot_fl = 17;

            NS_HX_counterflow_eqs::E_UA_target_type ua_type = NS_HX_counterflow_eqs::E_UA_target_type::E_constant_UA;

            mc_phx.initialize(hot_fl, 10, ua_type, 2024.0);

            double q_dot = 1000.0;      //[kWt]
            double T_co2_hot = 700.0;   //[C]
            double P_co2 = 25000.0;     //[kPa]
            double T_co2_cold = 500.0;  //[C]

            CO2_state co2_props;
            CO2_TP(T_co2_hot + 273.17, P_co2, &co2_props);

            double h_co2_hot = co2_props.enth;

            CO2_TP(T_co2_cold + 273.15, P_co2, &co2_props);

            double h_co2_cold = co2_props.enth;

            double m_dot_co2 = q_dot / (h_co2_hot - h_co2_cold);

            C_HX_counterflow_CRM::S_des_calc_UA_par des_par;
            des_par.m_T_h_in = 720.0;
            des_par.m_P_h_in = 100.0;   //[kPa]
            des_par.m_P_h_out = 100.0;  //[kPa]
            des_par.m_m_dot_cold_des = m_dot_co2;	//[kg/s] cold fluid design mass flow rate

            des_par.m_T_c_in = T_co2_cold;      //[K] Design-point cold inlet temperature
            des_par.m_P_c_in = P_co2;			//[kPa] Cold fluid inlet temperature
            des_par.m_P_c_out = P_co2;			//[kPa] Cold fluid outlet temperature

            /*
            double m_m_dot_hot_des;		//[kg/s] hot fluid design mass flow rate
            double m_eff_max;			//[-] Max allowable effectiveness
            */
            C_HX_counterflow_CRM::S_des_solved ms_phx_des_solved;
            mc_phx.design_and_calc_m_dot_htf(des_par, q_dot, 20, ms_phx_des_solved);

            double UA_calc = ms_phx_des_solved.m_UA_design;
            double T_htf_out = ms_phx_des_solved.m_T_h_out;
            double m_dot_htf = des_par.m_m_dot_hot_des;

            double blahhh = 1.23;

        }

        // Define Steam Inlet Conditions
        double T_ext_cold = 120;            //[C] Steam inlet temp
        double P_ext_cold = 4.762 * 100.0;  //[kPa] Inlet steam pressure

            // Get inlet steam properties
        water_state ms_water_props;
        int prop_error_code = water_TP(T_ext_cold + 273.15, P_ext_cold, &ms_water_props);
        double h_ext_cold = ms_water_props.enth;    // [kJ/kg] Inlet water enthalpy
        double Q_ext_cold = ms_water_props.qual;    // [] Inlet water quality (should be 0, n/a)
        double dens_ext_cold = ms_water_props.dens; // [kg/m3] Inlet water density

        // Define Outlet Steam Conditions
        double Q_ext_hot = 0.75;            // Outlet Steam Quality
        double P_ext_hot = P_ext_cold;      //[kPa] Outlet Steam Pressure

            // Outlet steam properties
        prop_error_code = water_PQ(P_ext_hot, Q_ext_hot, &ms_water_props);
        double h_ext_hot = ms_water_props.enth;             // [kJ/kg] Outlet Steam Enthalpy
        double dens_ext_hot = ms_water_props.dens;          //[kg/m3] Outlet steam density
        double T_ext_hot = ms_water_props.temp - 273.15;    // [C] Outlet Steam Temp

        // Initialize
        C_HX_htf_to_steam m_hx;
        int hot_fl = 21;    // HTF fl id
        int N_sub_hx = 50;
        NS_HX_counterflow_eqs::E_UA_target_type od_target_type = NS_HX_counterflow_eqs::E_UA_target_type::E_constant_UA;
        m_hx.initialize(hot_fl, N_sub_hx, od_target_type);

        // Design
        double T_htf_hot = 300;     //[C]
        double T_htf_cold = 200;    //[C]
        double q_design = 5;        //[MW]
        C_HX_counterflow_CRM::S_des_solved des_solved;
        m_hx.design_w_TP_PH(T_htf_hot + 273.15, 1.0, T_htf_cold + 273.15, 1.0,
            P_ext_cold, h_ext_cold, P_ext_hot, h_ext_hot, q_design * 1e3, des_solved);

        // Off Design
        double T_htf_hot_od = 295.9725; //[C]
        double od_tol = 1e-5;
        double mdot_htf_od = 22.90448;  //[kg/s]
        double h_htf_hot_od = m_hx.mc_hot_fl.enth(T_htf_hot_od + 273.15) * 1e-3;   //[kJ/kg]

        double q_dot_calc, h_ext_out_calc, h_htf_out_calc;

        std::vector<double> mdot_vec;
        std::vector<double> h_vec;
        double mdot_min = 0.75 * m_hx.ms_des_calc_UA_par.m_m_dot_cold_des;
        double mdot_max = 1.5 * m_hx.ms_des_calc_UA_par.m_m_dot_cold_des;

        // Manually run range of steam mass flow rates
        if (true)
        {
            int total_runs = 200;
            for (int i = 0; i < total_runs; i++)
            {
                double frac = (double)i / (double)total_runs;
                double mdot = mdot_min + (frac * (mdot_max - mdot_min));
                try
                {
                    m_hx.off_design_solution_fixed_dP_enth(h_ext_cold, P_ext_cold, mdot, P_ext_hot,
                        h_htf_hot_od, 1.0, mdot_htf_od, 1.0, od_tol,
                        q_dot_calc, h_ext_out_calc, h_htf_out_calc);
                }
                catch (C_csp_exception exc)
                {
                    h_ext_out_calc = 0;
                }

                h_vec.push_back(h_ext_out_calc);
                mdot_vec.push_back(mdot);
            }
        }


        // Optimize to find steam mdot
        double mdot_ext_calc, tol_solved, T_c_out, x_c_out, hx_min_dT;
        int solve_code = m_hx.off_design_target_cold_PH_out(h_ext_hot, mdot_min, mdot_max, P_ext_cold, h_ext_cold,
            P_ext_hot, 1.0, h_htf_hot_od, 1.0, mdot_htf_od, od_tol,
            q_dot_calc, h_ext_out_calc, h_htf_out_calc, mdot_ext_calc, tol_solved, T_c_out, x_c_out, hx_min_dT);

        // Off design Outlet steam properties
        prop_error_code = water_PH(P_ext_hot, h_ext_out_calc, &ms_water_props);
        double Q_ext_hot_od = ms_water_props.qual;             // [] Outlet Steam Quality
        double T_ext_hot_od = ms_water_props.temp - 273.15;    // [C] Outlet Steam Temp

        int x = 0;
    }
};

DEFINE_MODULE_ENTRY(csp_heatsink, "CSP heat sink", 1)
