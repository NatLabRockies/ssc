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

#include "sco2_turbinesplitflow_cycle.h"
#include "sco2_cycle_components.h"

#include "CO2_properties.h"


#include "fmin.h"


// ********************************************************************************** C_sco2_tsf_core CORE MODEL

void C_sco2_tsf_core::initialize_solve()
{
    m_outputs.Init();
}

int C_sco2_tsf_core::solve()
{
    initialize_solve();
    m_outputs.m_error_code = -1;

    // Apply scaling to the turbomachinery here
    {
        m_outputs.m_mc_ms.m_r_W_dot_scale = m_inputs.m_W_dot_net_design / 10.E3;    //[-]
        m_outputs.m_t.m_r_W_dot_scale = m_outputs.m_mc_ms.m_r_W_dot_scale;			//[-]
        m_outputs.m_t2.m_r_W_dot_scale = m_outputs.m_mc_ms.m_r_W_dot_scale;			//[-]
    }

    // Initialize Recuperators
    {
        // LTR
        m_outputs.mc_LT_recup.initialize(m_inputs.m_LTR_N_sub_hxrs, m_inputs.m_LTR_od_UA_target_type, m_inputs.m_f_inflation);
        // HTR
        m_outputs.mc_HT_recup.initialize(m_inputs.m_HTR_N_sub_hxrs, m_inputs.m_HTR_od_UA_target_type, m_inputs.m_f_inflation);
    }

    // Initialize a few variables
    {
        m_outputs.m_temp[C_sco2_cycle_core::MC_IN] = m_inputs.m_T_mc_in;     //[K]
        m_outputs.m_pres[C_sco2_cycle_core::MC_IN] = m_inputs.m_P_mc_in;
        m_outputs.m_pres[C_sco2_cycle_core::MC_OUT] = m_inputs.m_P_mc_out;
        m_outputs.m_temp[C_sco2_cycle_core::TURB_IN] = m_inputs.m_T_t_in; //[K]
    }

    // Apply pressure drops to heat exchangers, fully defining the pressures at all states
    {
        // LTR_HP_OUT
        if (m_inputs.m_DP_LTR[0] < 0.0)
            m_outputs.m_pres[C_sco2_cycle_core::LTR_HP_OUT] = m_outputs.m_pres[C_sco2_cycle_core::MC_OUT] - m_outputs.m_pres[C_sco2_cycle_core::MC_OUT] * std::abs(m_inputs.m_DP_LTR[0]);		// relative pressure drop specified for LT recuperator (cold stream)
        else
            m_outputs.m_pres[C_sco2_cycle_core::LTR_HP_OUT] = m_outputs.m_pres[C_sco2_cycle_core::MC_OUT] - m_inputs.m_DP_LTR[0];				// absolute pressure drop specified for LT recuperator (cold stream)
        if ((m_inputs.m_LTR_target_code == NS_HX_counterflow_eqs::OPTIMIZE_UA && m_inputs.m_LTR_UA < 1.0E-12)
            || (m_inputs.m_LTR_target_code == NS_HX_counterflow_eqs::TARGET_UA && m_inputs.m_LTR_UA < 1.0E-12)
            || (m_inputs.m_LTR_target_code == NS_HX_counterflow_eqs::TARGET_MIN_DT && m_inputs.m_LTR_min_dT < 1.0E-12)
            || (m_inputs.m_LTR_target_code == NS_HX_counterflow_eqs::TARGET_EFFECTIVENESS && m_inputs.m_LTR_eff_target < 1.0E-12))
            m_outputs.m_pres[C_sco2_cycle_core::LTR_HP_OUT] = m_outputs.m_pres[C_sco2_cycle_core::MC_OUT];			// If there is no LT recuperator, there is no pressure drop

        // TURB_IN
        if (m_inputs.m_DP_PHX[0] < 0.0)
            m_outputs.m_pres[C_sco2_cycle_core::TURB_IN] = m_outputs.m_pres[C_sco2_cycle_core::LTR_HP_OUT] - m_outputs.m_pres[C_sco2_cycle_core::LTR_HP_OUT] * std::abs(m_inputs.m_DP_PHX[0]);	// relative pressure drop specified for PHX
        else
            m_outputs.m_pres[C_sco2_cycle_core::TURB_IN] = m_outputs.m_pres[C_sco2_cycle_core::LTR_HP_OUT] - m_inputs.m_DP_PHX[0];									// absolute pressure drop specified for PHX

        // HTR_HP_OUT
        if (m_inputs.m_DP_HTR[0] < 0.0)
            m_outputs.m_pres[C_sco2_cycle_core::HTR_HP_OUT] = m_outputs.m_pres[C_sco2_cycle_core::MC_OUT] - m_outputs.m_pres[C_sco2_cycle_core::MC_OUT] * std::abs(m_inputs.m_DP_HTR[0]);		// relative pressure drop specified for HT recuperator (cold stream)
        else
            m_outputs.m_pres[C_sco2_cycle_core::HTR_HP_OUT] = m_outputs.m_pres[C_sco2_cycle_core::MC_OUT] - m_inputs.m_DP_HTR[0];				// absolute pressure drop specified for HT recuperator (cold stream)
        if ((m_inputs.m_HTR_target_code == NS_HX_counterflow_eqs::OPTIMIZE_UA && m_inputs.m_HTR_UA < 1.0E-12)
            || (m_inputs.m_HTR_target_code == NS_HX_counterflow_eqs::TARGET_UA && m_inputs.m_HTR_UA < 1.0E-12)
            || (m_inputs.m_HTR_target_code == NS_HX_counterflow_eqs::TARGET_MIN_DT && m_inputs.m_HTR_min_dT < 1.0E-12)
            || (m_inputs.m_HTR_target_code == NS_HX_counterflow_eqs::TARGET_EFFECTIVENESS && m_inputs.m_HTR_eff_target < 1.0E-12))
            m_outputs.m_pres[C_sco2_cycle_core::HTR_HP_OUT] = m_outputs.m_pres[C_sco2_cycle_core::MC_OUT];			// If there is no HT recuperator, there is no pressure drop

        // MIXER_OUT
        if (m_inputs.m_DP_PC_main[1] < 0.0)
            m_outputs.m_pres[C_sco2_cycle_core::MIXER_OUT] = m_outputs.m_pres[C_sco2_cycle_core::MC_IN] / (1.0 - std::abs(m_inputs.m_DP_PC_main[1]));					// relative pressure drop specified for precooler: P1=P9-P9*rel_DP => P1=P9*(1-rel_DP)
        else
            m_outputs.m_pres[C_sco2_cycle_core::MIXER_OUT] = m_outputs.m_pres[C_sco2_cycle_core::MC_IN] + m_inputs.m_DP_PC_main[1];

        // LTR_LP_OUT
        m_outputs.m_pres[C_sco2_cycle_core::LTR_LP_OUT] = m_outputs.m_pres[C_sco2_cycle_core::MIXER_OUT];   // Assume no pressure drop in mixer

        // HTR_LP_OUT
        m_outputs.m_pres[C_sco2_cycle_core::HTR_LP_OUT] = m_outputs.m_pres[C_sco2_cycle_core::MIXER_OUT];   // Assume no pressure drop in mixer

        // TURB_OUT
        if (m_inputs.m_DP_HTR[1] < 0.0)
            m_outputs.m_pres[C_sco2_cycle_core::TURB_OUT] = m_outputs.m_pres[C_sco2_cycle_core::HTR_LP_OUT] / (1.0 - std::abs(m_inputs.m_DP_HTR[1]));	// relative pressure drop specified for HT recuperator (hot stream)
        else
            m_outputs.m_pres[C_sco2_cycle_core::TURB_OUT] = m_outputs.m_pres[C_sco2_cycle_core::HTR_LP_OUT] + m_inputs.m_DP_HTR[1];				// absolute pressure drop specified for HT recuperator (hot stream)
        if ((m_inputs.m_HTR_target_code == NS_HX_counterflow_eqs::OPTIMIZE_UA && m_inputs.m_HTR_UA < 1.0E-12)
            || (m_inputs.m_HTR_target_code == NS_HX_counterflow_eqs::TARGET_UA && m_inputs.m_HTR_UA < 1.0E-12)
            || (m_inputs.m_HTR_target_code == NS_HX_counterflow_eqs::TARGET_MIN_DT && m_inputs.m_HTR_min_dT < 1.0E-12)
            || (m_inputs.m_HTR_target_code == NS_HX_counterflow_eqs::TARGET_EFFECTIVENESS && m_inputs.m_HTR_eff_target < 1.0E-12))
            m_outputs.m_pres[C_sco2_cycle_core::TURB_OUT] = m_outputs.m_pres[C_sco2_cycle_core::HTR_LP_OUT];		// if there is no HT recuperator, there is no pressure drop

        // TURB2_OUT
        if (m_inputs.m_DP_LTR[1] < 0.0)
            m_outputs.m_pres[C_sco2_cycle_core::TURB2_OUT] = m_outputs.m_pres[C_sco2_cycle_core::LTR_LP_OUT] / (1.0 - std::abs(m_inputs.m_DP_LTR[1]));	// relative pressure drop specified for LT recuperator (hot stream)
        else
            m_outputs.m_pres[C_sco2_cycle_core::TURB2_OUT] = m_outputs.m_pres[C_sco2_cycle_core::LTR_LP_OUT] + m_inputs.m_DP_LTR[1];				// absolute pressure drop specified for HT recuperator (hot stream)
        if ((m_inputs.m_LTR_target_code == NS_HX_counterflow_eqs::OPTIMIZE_UA && m_inputs.m_LTR_UA < 1.0E-12)
            || (m_inputs.m_LTR_target_code == NS_HX_counterflow_eqs::TARGET_UA && m_inputs.m_LTR_UA < 1.0E-12)
            || (m_inputs.m_LTR_target_code == NS_HX_counterflow_eqs::TARGET_MIN_DT && m_inputs.m_LTR_min_dT < 1.0E-12)
            || (m_inputs.m_LTR_target_code == NS_HX_counterflow_eqs::TARGET_EFFECTIVENESS && m_inputs.m_LTR_eff_target < 1.0E-12))
            m_outputs.m_pres[C_sco2_cycle_core::TURB2_OUT] = m_outputs.m_pres[C_sco2_cycle_core::LTR_LP_OUT];		// if there is no LT recuperator, there is no pressure drop

    }

    // Determine equivalent isentropic efficiencies for main compressor and turbine, if necessary.
    double eta_mc_isen = std::numeric_limits<double>::quiet_NaN();
    double eta_t_isen = std::numeric_limits<double>::quiet_NaN();
    {
        if (m_inputs.m_eta_mc < 0.0)
        {
            int poly_error_code = 0;

            isen_eta_from_poly_eta(m_outputs.m_temp[C_sco2_cycle_core::MC_IN], m_outputs.m_pres[C_sco2_cycle_core::MC_IN], m_outputs.m_pres[C_sco2_cycle_core::MC_OUT], std::abs(m_inputs.m_eta_mc),
                true, poly_error_code, eta_mc_isen);

            if (poly_error_code != 0)
            {
                m_outputs.m_error_code = poly_error_code;
                return m_outputs.m_error_code;
            }
        }
        else
            eta_mc_isen = m_inputs.m_eta_mc;

        if (m_inputs.m_eta_t < 0.0)
        {
            int poly_error_code = 0;

            isen_eta_from_poly_eta(m_outputs.m_temp[C_sco2_cycle_core::TURB_IN], m_outputs.m_pres[C_sco2_cycle_core::TURB_IN], m_outputs.m_pres[C_sco2_cycle_core::TURB_OUT], std::abs(m_inputs.m_eta_t),
                false, poly_error_code, eta_t_isen);

            if (poly_error_code != 0)
            {
                m_outputs.m_error_code = poly_error_code;
                return m_outputs.m_error_code;
            }
        }
        else
            eta_t_isen = m_inputs.m_eta_t;
    }

    // Determine the outlet state and specific work for the main compressor and turbine.

    // Main compressor
    m_outputs.m_w_mc = std::numeric_limits<double>::quiet_NaN();
    {
        int comp_error_code = 0;

        calculate_turbomachinery_outlet_1(m_outputs.m_temp[C_sco2_cycle_core::MC_IN], m_outputs.m_pres[C_sco2_cycle_core::MC_IN], m_outputs.m_pres[C_sco2_cycle_core::MC_OUT], eta_mc_isen, true,
            comp_error_code, m_outputs.m_enth[C_sco2_cycle_core::MC_IN], m_outputs.m_entr[C_sco2_cycle_core::MC_IN], m_outputs.m_dens[C_sco2_cycle_core::MC_IN], m_outputs.m_temp[C_sco2_cycle_core::MC_OUT],
            m_outputs.m_enth[C_sco2_cycle_core::MC_OUT], m_outputs.m_entr[C_sco2_cycle_core::MC_OUT], m_outputs.m_dens[C_sco2_cycle_core::MC_OUT], m_outputs.m_w_mc);

        if (comp_error_code != 0)
        {
            m_outputs.m_error_code = comp_error_code;
            return m_outputs.m_error_code;
        }
    }

    // Turbine
    m_outputs.m_w_t = std::numeric_limits<double>::quiet_NaN();
    {
        int turbine_error_code = 0;

        calculate_turbomachinery_outlet_1(m_outputs.m_temp[C_sco2_cycle_core::TURB_IN], m_outputs.m_pres[C_sco2_cycle_core::TURB_IN], m_outputs.m_pres[C_sco2_cycle_core::TURB_OUT], eta_t_isen, false,
            turbine_error_code, m_outputs.m_enth[C_sco2_cycle_core::TURB_IN], m_outputs.m_entr[C_sco2_cycle_core::TURB_IN], m_outputs.m_dens[C_sco2_cycle_core::TURB_IN], m_outputs.m_temp[C_sco2_cycle_core::TURB_OUT],
            m_outputs.m_enth[C_sco2_cycle_core::TURB_OUT], m_outputs.m_entr[C_sco2_cycle_core::TURB_OUT], m_outputs.m_dens[C_sco2_cycle_core::TURB_OUT], m_outputs.m_w_t);

        if (turbine_error_code != 0)
        {
            m_outputs.m_error_code = turbine_error_code;
            return m_outputs.m_error_code;
        }
    }

    // Solve HTR_HP_OUT Temp (iteratively)
    {
        // Make monotonic solver
        C_mono_tsf_core_HTR_des HTR_des_eq(this);
        C_monotonic_eq_solver HTR_des_solver(HTR_des_eq);

        // Bounds
        double T_HTR_HP_out_lower = m_outputs.m_temp[C_sco2_cycle_core::MC_OUT];    //[K] Coldest possible temp
        double T_HTR_HP_out_upper = m_outputs.m_temp[C_sco2_cycle_core::TURB_IN];   //[K] Coldest possible temp (probably is TURB_OUT)

        // Solution Guess
        double T_HTR_HP_out_guess_lower = 0.25 * (T_HTR_HP_out_upper - T_HTR_HP_out_lower) + T_HTR_HP_out_lower;    // [K]
        double T_HTR_HP_out_guess_upper = 0.75 * (T_HTR_HP_out_upper - T_HTR_HP_out_lower) + T_HTR_HP_out_lower;    // [K]

        // Optimization Settings
        HTR_des_solver.settings(m_inputs.m_des_tol* m_outputs.m_temp[C_sco2_cycle_core::MC_IN], 1000, T_HTR_HP_out_lower, T_HTR_HP_out_upper, false);

        // Optimization Output variables
        double tol_T_HTR_HP_out_solved;
        double T_HTR_HP_out_solved = tol_T_HTR_HP_out_solved = std::numeric_limits<double>::quiet_NaN();
        int iter_T_HTR_LP_out = -1;

        // Optimize
        int T_HTR_HP_out_code = HTR_des_solver.solve(T_HTR_HP_out_guess_lower, T_HTR_HP_out_guess_upper, 0,
            T_HTR_HP_out_solved, tol_T_HTR_HP_out_solved, iter_T_HTR_LP_out);

        // Check if converged
        if (T_HTR_HP_out_code != C_monotonic_eq_solver::CONVERGED)
        {
            m_outputs.m_error_code = 25;
            return m_outputs.m_error_code;
        }
      
        // Run Calculated HTR HP Out (to set correct temps)
        double dummy;
        solve_HTR(T_HTR_HP_out_solved, &dummy);
    }

    // Complete HTR_HP_OUT and HTR_LP_OUT co2 properties
    {
        int prop_error_code = CO2_TP(m_outputs.m_temp[C_sco2_cycle_core::HTR_HP_OUT], m_outputs.m_pres[C_sco2_cycle_core::HTR_HP_OUT], &m_co2_props);
        if (prop_error_code != 0)
        {
            m_outputs.m_error_code = prop_error_code;
            return m_outputs.m_error_code;
        }
        m_outputs.m_enth[C_sco2_cycle_core::HTR_HP_OUT] = m_co2_props.enth;
        m_outputs.m_entr[C_sco2_cycle_core::HTR_HP_OUT] = m_co2_props.entr;
        m_outputs.m_dens[C_sco2_cycle_core::HTR_HP_OUT] = m_co2_props.dens;

        prop_error_code = CO2_TP(m_outputs.m_temp[C_sco2_cycle_core::HTR_LP_OUT], m_outputs.m_pres[C_sco2_cycle_core::HTR_LP_OUT], &m_co2_props);
        if (prop_error_code != 0)
        {
            m_outputs.m_error_code = prop_error_code;
            return m_outputs.m_error_code;
        }
        m_outputs.m_enth[C_sco2_cycle_core::HTR_LP_OUT] = m_co2_props.enth;
        m_outputs.m_entr[C_sco2_cycle_core::HTR_LP_OUT] = m_co2_props.entr;
        m_outputs.m_dens[C_sco2_cycle_core::HTR_LP_OUT] = m_co2_props.dens;
    }

    // Simulate LTR
    {
        m_outputs.mc_LT_recup.design_for_target__calc_outlet(m_inputs.m_LTR_target_code,
            m_inputs.m_LTR_UA, m_inputs.m_LTR_min_dT, m_inputs.m_LTR_eff_target,
            m_inputs.m_LTR_eff_max,
            m_outputs.m_temp[C_sco2_cycle_core::MC_OUT], m_outputs.m_pres[C_sco2_cycle_core::MC_OUT], m_outputs.m_m_dot_t, m_outputs.m_pres[C_sco2_cycle_core::LTR_HP_OUT],
            m_outputs.m_temp[C_sco2_cycle_core::TURB2_OUT], m_outputs.m_pres[C_sco2_cycle_core::TURB2_OUT], m_outputs.m_m_dot_t2, m_outputs.m_pres[C_sco2_cycle_core::LTR_LP_OUT],
            m_inputs.m_des_tol,
            m_outputs.m_Q_dot_LT, m_outputs.m_temp[C_sco2_cycle_core::LTR_HP_OUT], m_outputs.m_temp[C_sco2_cycle_core::LTR_LP_OUT]);
    }

    // Complete LTR_HP_OUT and LTR_LP_OUT co2 properties
    {
        int prop_error_code = CO2_TP(m_outputs.m_temp[C_sco2_cycle_core::LTR_HP_OUT], m_outputs.m_pres[C_sco2_cycle_core::LTR_HP_OUT], &m_co2_props);
        if (prop_error_code != 0)
        {
            m_outputs.m_error_code = prop_error_code;
            return m_outputs.m_error_code;
        }
        m_outputs.m_enth[C_sco2_cycle_core::LTR_HP_OUT] = m_co2_props.enth;
        m_outputs.m_entr[C_sco2_cycle_core::LTR_HP_OUT] = m_co2_props.entr;
        m_outputs.m_dens[C_sco2_cycle_core::LTR_HP_OUT] = m_co2_props.dens;

        prop_error_code = CO2_TP(m_outputs.m_temp[C_sco2_cycle_core::LTR_LP_OUT], m_outputs.m_pres[C_sco2_cycle_core::LTR_LP_OUT], &m_co2_props);
        if (prop_error_code != 0)
        {
            m_outputs.m_error_code = prop_error_code;
            return m_outputs.m_error_code;
        }
        m_outputs.m_enth[C_sco2_cycle_core::LTR_LP_OUT] = m_co2_props.enth;
        m_outputs.m_entr[C_sco2_cycle_core::LTR_LP_OUT] = m_co2_props.entr;
        m_outputs.m_dens[C_sco2_cycle_core::LTR_LP_OUT] = m_co2_props.dens;
    }

    // Simulate Mixer
    {
        m_outputs.m_enth[C_sco2_cycle_core::MIXER_OUT] = ((1.0 - m_inputs.m_split_frac) * m_outputs.m_enth[C_sco2_cycle_core::HTR_LP_OUT])
            + (m_inputs.m_split_frac * m_outputs.m_enth[C_sco2_cycle_core::LTR_LP_OUT]);

        int prop_error_code = CO2_PH(m_outputs.m_pres[C_sco2_cycle_core::MIXER_OUT], m_outputs.m_enth[C_sco2_cycle_core::MIXER_OUT], &m_co2_props);
        if (prop_error_code != 0)
        {
            m_outputs.m_error_code = prop_error_code;
            return m_outputs.m_error_code;
        }
        m_outputs.m_temp[C_sco2_cycle_core::MIXER_OUT] = m_co2_props.temp;		//[K]
        m_outputs.m_entr[C_sco2_cycle_core::MIXER_OUT] = m_co2_props.entr;		//[kJ/kg-K]
        m_outputs.m_dens[C_sco2_cycle_core::MIXER_OUT] = m_co2_props.dens;		//[kg/m^3]
    }

    // Calculate total work and heat metrics
    {
        // Work
        m_outputs.m_W_dot_mc = m_outputs.m_w_mc * m_outputs.m_m_dot_mc;
        m_outputs.m_W_dot_t = m_outputs.m_w_t * m_outputs.m_m_dot_t;
        m_outputs.m_W_dot_t2 = m_outputs.m_w_t2 * m_outputs.m_m_dot_t2;
        m_outputs.m_W_dot_net = m_outputs.m_W_dot_mc + m_outputs.m_W_dot_t + m_outputs.m_W_dot_t2;

        // Air Cooler (heat rejection unit)
        m_outputs.m_W_dot_air_cooler = m_inputs.m_frac_fan_power * m_outputs.m_W_dot_net;
        m_outputs.m_Q_dot_air_cooler = m_outputs.m_m_dot_mc * (m_outputs.m_enth[C_sco2_cycle_core::MIXER_OUT] - m_outputs.m_enth[C_sco2_cycle_core::MC_IN]);

        // Total heat entering sco2
        m_outputs.m_Q_dot_PHX = m_outputs.m_m_dot_t * (m_outputs.m_enth[C_sco2_cycle_core::TURB_IN] - m_outputs.m_enth[C_sco2_cycle_core::LTR_HP_OUT]);
        m_outputs.m_Q_dot_total = m_outputs.m_Q_dot_PHX;

        // LTR
        m_outputs.m_Q_dot_LTR_LP = m_outputs.m_m_dot_t2 * (m_outputs.m_enth[C_sco2_cycle_core::TURB2_OUT] - m_outputs.m_enth[C_sco2_cycle_core::LTR_LP_OUT]);
        m_outputs.m_Q_dot_LTR_HP = m_outputs.m_m_dot_t * (m_outputs.m_enth[C_sco2_cycle_core::LTR_HP_OUT] - m_outputs.m_enth[C_sco2_cycle_core::MC_OUT]);

        // LTR
        m_outputs.m_Q_dot_HTR_LP = m_outputs.m_m_dot_t * (m_outputs.m_enth[C_sco2_cycle_core::TURB_OUT] - m_outputs.m_enth[C_sco2_cycle_core::HTR_LP_OUT]);
        m_outputs.m_Q_dot_HTR_HP = m_outputs.m_m_dot_t2 * (m_outputs.m_enth[C_sco2_cycle_core::HTR_HP_OUT] - m_outputs.m_enth[C_sco2_cycle_core::MC_OUT]);

        // Thermal Efficiency
        m_outputs.m_eta_thermal = m_outputs.m_W_dot_net / m_outputs.m_Q_dot_total;

        // Back Calculate Heat In
        double Q_in_calc = m_outputs.m_W_dot_net + m_outputs.m_Q_dot_air_cooler;
    }

    // Define Heat Exchangers and Air Cooler
    {
        // PHX
        C_HeatExchanger::S_design_parameters PHX_des_par;
        PHX_des_par.m_DP_design[0] = m_outputs.m_pres[C_sco2_cycle_core::LTR_HP_OUT] - m_outputs.m_pres[C_sco2_cycle_core::TURB_IN];
        PHX_des_par.m_DP_design[1] = 0.0;
        PHX_des_par.m_m_dot_design[0] = m_outputs.m_m_dot_t;
        PHX_des_par.m_m_dot_design[1] = 0.0;
        PHX_des_par.m_Q_dot_design = m_outputs.m_m_dot_t * (m_outputs.m_enth[C_sco2_cycle_core::TURB_IN] - m_outputs.m_enth[C_sco2_cycle_core::LTR_HP_OUT]);
        m_outputs.m_PHX.initialize(PHX_des_par);

        // Air Cooler
        C_HeatExchanger::S_design_parameters PC_des_par;
        PC_des_par.m_DP_design[0] = 0.0;
        PC_des_par.m_DP_design[1] = m_outputs.m_pres[C_sco2_cycle_core::MIXER_OUT] - m_outputs.m_pres[C_sco2_cycle_core::MC_IN];
        PC_des_par.m_m_dot_design[0] = 0.0;
        PC_des_par.m_m_dot_design[1] = m_outputs.m_m_dot_mc;
        PC_des_par.m_Q_dot_design = m_outputs.m_m_dot_mc * (m_outputs.m_enth[C_sco2_cycle_core::MIXER_OUT] - m_outputs.m_enth[C_sco2_cycle_core::MC_IN]);
        m_outputs.m_PC.initialize(PC_des_par);
    }

    m_outputs.m_error_code = 0;

    return m_outputs.m_error_code;
}

int C_sco2_tsf_core::finalize_design(C_sco2_cycle_core::S_design_solved& design_solved)
{
    // Design Main Compressor
    {
        int mc_design_err = m_outputs.m_mc_ms.design_given_outlet_state(m_inputs.m_mc_comp_model_code, m_outputs.m_temp[C_sco2_cycle_core::MC_IN],
            m_outputs.m_pres[C_sco2_cycle_core::MC_IN],
            m_outputs.m_m_dot_mc,
            m_outputs.m_temp[C_sco2_cycle_core::MC_OUT],
            m_outputs.m_pres[C_sco2_cycle_core::MC_OUT],
            m_inputs.m_des_tol,
            m_inputs.m_f_inflation);

        if (mc_design_err != 0)
        {
            m_outputs.m_error_code = mc_design_err;
            return m_outputs.m_error_code;
        }
    }

    // Size Turbine
    {
        C_turbine::S_design_parameters t_des_par;
        // Set turbine shaft speed
        t_des_par.m_N_design = m_inputs.m_N_turbine;
        t_des_par.m_N_comp_design_if_linked = m_outputs.m_mc_ms.get_design_solved()->m_N_design; //[rpm] m_mc.get_design_solved()->m_N_design;
        // Turbine inlet state
        t_des_par.m_P_in = m_outputs.m_pres[C_sco2_cycle_core::TURB_IN];
        t_des_par.m_T_in = m_outputs.m_temp[C_sco2_cycle_core::TURB_IN];
        t_des_par.m_D_in = m_outputs.m_dens[C_sco2_cycle_core::TURB_IN];
        t_des_par.m_h_in = m_outputs.m_enth[C_sco2_cycle_core::TURB_IN];
        t_des_par.m_s_in = m_outputs.m_entr[C_sco2_cycle_core::TURB_IN];
        // Turbine outlet state
        t_des_par.m_P_out = m_outputs.m_pres[C_sco2_cycle_core::TURB_OUT];
        t_des_par.m_h_out = m_outputs.m_enth[C_sco2_cycle_core::TURB_OUT];
        // Mass flow
        t_des_par.m_m_dot = m_outputs.m_m_dot_t;
        // Inflation factor
        t_des_par.m_f_inflation = m_inputs.m_f_inflation;

        int turb_size_error_code = 0;
        m_outputs.m_t.turbine_sizing(t_des_par, turb_size_error_code);

        if (turb_size_error_code != 0)
        {
            m_outputs.m_error_code = turb_size_error_code;
            return m_outputs.m_error_code;
        }
    }

    // Size Secondary Turbine
    {
        C_turbine::S_design_parameters t2_des_par;
        // Set turbine shaft speed
        t2_des_par.m_N_design = m_inputs.m_N_turbine;
        t2_des_par.m_N_comp_design_if_linked = m_outputs.m_mc_ms.get_design_solved()->m_N_design; //[rpm] m_mc.get_design_solved()->m_N_design;
        // Turbine inlet state
        t2_des_par.m_P_in = m_outputs.m_pres[C_sco2_cycle_core::HTR_HP_OUT];
        t2_des_par.m_T_in = m_outputs.m_temp[C_sco2_cycle_core::HTR_HP_OUT];
        t2_des_par.m_D_in = m_outputs.m_dens[C_sco2_cycle_core::HTR_HP_OUT];
        t2_des_par.m_h_in = m_outputs.m_enth[C_sco2_cycle_core::HTR_HP_OUT];
        t2_des_par.m_s_in = m_outputs.m_entr[C_sco2_cycle_core::HTR_HP_OUT];
        // Turbine outlet state
        t2_des_par.m_P_out = m_outputs.m_pres[C_sco2_cycle_core::TURB2_OUT];
        t2_des_par.m_h_out = m_outputs.m_enth[C_sco2_cycle_core::TURB2_OUT];
        // Mass flow
        t2_des_par.m_m_dot = m_outputs.m_m_dot_t2;
        // Inflation factor
        t2_des_par.m_f_inflation = m_inputs.m_f_inflation;

        int turb_size_error_code = 0;
        m_outputs.m_t2.turbine_sizing(t2_des_par, turb_size_error_code);

        if (turb_size_error_code != 0)
        {
            m_outputs.m_error_code = turb_size_error_code;
            return m_outputs.m_error_code;
        }
    }

    // Design air cooler
    {
        // Structure for design parameters that are dependent on cycle design solution
        C_CO2_to_air_cooler::S_des_par_cycle_dep s_air_cooler_des_par_dep;
        // Set air cooler design parameters that are dependent on the cycle design solution
        s_air_cooler_des_par_dep.m_T_hot_in_des = m_outputs.m_temp[C_sco2_cycle_core::MIXER_OUT];  // [K]
        s_air_cooler_des_par_dep.m_P_hot_in_des = m_outputs.m_pres[C_sco2_cycle_core::MIXER_OUT];  // [kPa]
        s_air_cooler_des_par_dep.m_m_dot_total = m_outputs.m_m_dot_mc;                // [kg/s]

        // This pressure drop is currently uncoupled from the cycle design
        double cooler_deltaP = m_outputs.m_pres[C_sco2_cycle_core::MIXER_OUT] - m_outputs.m_pres[C_sco2_cycle_core::MC_IN];    // [kPa]
        if (cooler_deltaP == 0.0)
            s_air_cooler_des_par_dep.m_delta_P_des = m_inputs.m_deltaP_cooler_frac * m_outputs.m_pres[C_sco2_cycle_core::MIXER_OUT];    // [kPa]
        else
            s_air_cooler_des_par_dep.m_delta_P_des = cooler_deltaP; // [kPa]

        s_air_cooler_des_par_dep.m_T_hot_out_des = m_outputs.m_temp[C_sco2_cycle_core::MC_IN];                          // [K]
        s_air_cooler_des_par_dep.m_W_dot_fan_des = m_inputs.m_frac_fan_power * m_outputs.m_W_dot_net / 1000.0;     // [MWe]
        // Structure for design parameters that are independent of cycle design solution
        C_CO2_to_air_cooler::S_des_par_ind s_air_cooler_des_par_ind;
        s_air_cooler_des_par_ind.m_T_amb_des = m_inputs.m_T_amb_des;         // [K]
        s_air_cooler_des_par_ind.m_elev = m_inputs.m_elevation;              // [m]
        s_air_cooler_des_par_ind.m_eta_fan = m_inputs.m_eta_fan;             // [-]
        s_air_cooler_des_par_ind.m_N_nodes_pass = m_inputs.m_N_nodes_pass;   // [-]
        s_air_cooler_des_par_ind.m_f_inflation = m_inputs.m_f_inflation;     // [-]

        if (m_inputs.m_is_des_air_cooler && std::isfinite(m_inputs.m_deltaP_cooler_frac) && std::isfinite(m_inputs.m_frac_fan_power)
            && std::isfinite(m_inputs.m_T_amb_des) && std::isfinite(m_inputs.m_elevation) && std::isfinite(m_inputs.m_eta_fan) && m_inputs.m_N_nodes_pass > 0)
        {
            try
            {
                m_outputs.mc_air_cooler.design_hx(s_air_cooler_des_par_ind, s_air_cooler_des_par_dep, m_inputs.m_des_tol);
            }
            catch (...)
            {
                design_solved.m_eta_thermal = m_outputs.m_eta_thermal;
                m_outputs.m_error_code = C_sco2_cycle_core::E_cycle_error_msg::E_AIR_COOLER_CONVERGENCE;
                return m_outputs.m_error_code;
            }
            
        }
    }

    // Get 'design_solved' structure from component classes
    design_solved.ms_mc_ms_des_solved = *m_outputs.m_mc_ms.get_design_solved();
    design_solved.ms_t_des_solved = *m_outputs.m_t.get_design_solved();
    design_solved.ms_t2_des_solved = *m_outputs.m_t2.get_design_solved();
    design_solved.ms_LTR_des_solved = m_outputs.mc_LT_recup.ms_des_solved;
    design_solved.ms_HTR_des_solved = m_outputs.mc_HT_recup.ms_des_solved;
    design_solved.ms_mc_air_cooler = *m_outputs.mc_air_cooler.get_design_solved();

    // Set solved design point metrics
    design_solved.m_temp = m_outputs.m_temp;
    design_solved.m_pres = m_outputs.m_pres;
    design_solved.m_enth = m_outputs.m_enth;
    design_solved.m_entr = m_outputs.m_entr;
    design_solved.m_dens = m_outputs.m_dens;

    design_solved.m_eta_thermal = m_outputs.m_eta_thermal;
    design_solved.m_W_dot_net = m_outputs.m_W_dot_net;
    design_solved.m_m_dot_mc = m_outputs.m_m_dot_mc;
    design_solved.m_m_dot_t = m_outputs.m_m_dot_t;
    design_solved.m_m_dot_t2 = m_outputs.m_m_dot_t2;
    design_solved.m_turbine_split_frac = m_inputs.m_split_frac;

    design_solved.m_UA_LTR = m_inputs.m_LTR_UA;
    design_solved.m_UA_HTR = m_inputs.m_HTR_UA;

    design_solved.m_W_dot_t = m_outputs.m_W_dot_t;		//[kWe]
    design_solved.m_W_dot_t2 = m_outputs.m_W_dot_t2;    //[kWe]
    design_solved.m_W_dot_mc = m_outputs.m_W_dot_mc;	//[kWe]

    design_solved.m_is_rc = false;

    design_solved.m_W_dot_cooler_tot = m_outputs.mc_air_cooler.get_design_solved()->m_W_dot_fan * 1.E3;	//[kWe] convert from MWe

    return 0;
}

int C_sco2_tsf_core::solve_HTR(double T_HTR_HP_OUT_guess, double* diff_T_HTR_HP_out)
{
    // Set HTR_HP_OUT temp
    m_outputs.m_temp[C_sco2_cycle_core::HTR_HP_OUT] = T_HTR_HP_OUT_guess;

    // Simulate Secondary Turbine
    {
        // Determine equivalent isentropic efficiencies for secondary turbine, if necessary
        double eta_t2_isen = std::numeric_limits<double>::quiet_NaN();
        {
            if (m_inputs.m_eta_t2 < 0.0)
            {
                int poly_error_code = 0;

                isen_eta_from_poly_eta(m_outputs.m_temp[C_sco2_cycle_core::HTR_HP_OUT], m_outputs.m_pres[C_sco2_cycle_core::HTR_HP_OUT], m_outputs.m_pres[C_sco2_cycle_core::TURB2_OUT], std::abs(m_inputs.m_eta_t2),
                    false, poly_error_code, eta_t2_isen);

                if (poly_error_code != 0)
                {
                    m_outputs.m_error_code = poly_error_code;
                    return m_outputs.m_error_code;
                }
            }
            else
                eta_t2_isen = m_inputs.m_eta_t2;
        }

        // Simulate Secondary Turbine
        {
            int turbine2_error_code = 0;

            calculate_turbomachinery_outlet_1(m_outputs.m_temp[C_sco2_cycle_core::HTR_HP_OUT], m_outputs.m_pres[C_sco2_cycle_core::HTR_HP_OUT], m_outputs.m_pres[C_sco2_cycle_core::TURB2_OUT], eta_t2_isen, false,
                turbine2_error_code, m_outputs.m_enth[C_sco2_cycle_core::HTR_HP_OUT], m_outputs.m_entr[C_sco2_cycle_core::HTR_HP_OUT], m_outputs.m_dens[C_sco2_cycle_core::HTR_HP_OUT], m_outputs.m_temp[C_sco2_cycle_core::TURB2_OUT],
                m_outputs.m_enth[C_sco2_cycle_core::TURB2_OUT], m_outputs.m_entr[C_sco2_cycle_core::TURB2_OUT], m_outputs.m_dens[C_sco2_cycle_core::TURB2_OUT], m_outputs.m_w_t2);

            if (turbine2_error_code != 0)
            {
                m_outputs.m_error_code = turbine2_error_code;
                return m_outputs.m_error_code;
            }
        }
    }

    // Calculate Mass Flow Rates
    m_outputs.m_m_dot_mc = m_inputs.m_W_dot_net_design
        / (((m_outputs.m_w_t * (1.0 - m_inputs.m_split_frac))
            + (m_outputs.m_w_t2 * m_inputs.m_split_frac)
            + (m_outputs.m_w_mc))
            * m_inputs.m_eta_generator);    //[kg/s]

    m_outputs.m_m_dot_t = m_outputs.m_m_dot_mc * (1.0 - m_inputs.m_split_frac); //[kg/s]
    m_outputs.m_m_dot_t2 = m_outputs.m_m_dot_mc * m_inputs.m_split_frac;        //[kg/s]

    // Solve HTR
    double T_HTR_HP_out_calc = std::numeric_limits<double>::quiet_NaN();
    m_outputs.mc_HT_recup.design_for_target__calc_outlet(m_inputs.m_HTR_target_code,
        m_inputs.m_HTR_UA, m_inputs.m_HTR_min_dT, m_inputs.m_HTR_eff_target,
        m_inputs.m_HTR_eff_max,
        m_outputs.m_temp[C_sco2_cycle_core::MC_OUT], m_outputs.m_pres[C_sco2_cycle_core::MC_OUT], m_outputs.m_m_dot_t2, m_outputs.m_pres[C_sco2_cycle_core::HTR_HP_OUT],
        m_outputs.m_temp[C_sco2_cycle_core::TURB_OUT], m_outputs.m_pres[C_sco2_cycle_core::TURB_OUT], m_outputs.m_m_dot_t, m_outputs.m_pres[C_sco2_cycle_core::HTR_LP_OUT],
        m_inputs.m_des_tol,
        m_outputs.m_Q_dot_HT, T_HTR_HP_out_calc, m_outputs.m_temp[C_sco2_cycle_core::HTR_LP_OUT]);

    *diff_T_HTR_HP_out =  T_HTR_HP_OUT_guess - T_HTR_HP_out_calc;

    return 0;
}

void C_sco2_tsf_core::reset()
{
    this->m_inputs = S_sco2_tsf_in();
    this->m_outputs.Init();
}

// ********************************************************************************** END C_sco2_tsf_core


// ********************************************************************************** PRIVATE C_TurbineSplitFlow_Cycle (: C_sco2_cycle_core) 

/// <summary>
/// Core function to optimize cycle (fixed total UA)
/// </summary>
void C_TurbineSplitFlow_Cycle::auto_opt_design_core(int& error_code)
{
    // Create 'ms_opt_des_par' for Design Variables
    S_opt_design_parameters opt_par;
    {
        // Max Pressure
        double best_P_high = m_P_high_limit;		//[kPa]
        double PR_mc_guess = 2.5;				//[-]

        opt_par.m_fixed_P_mc_out = ms_auto_opt_des_par.m_fixed_P_mc_out;		//[-]
        if (!opt_par.m_fixed_P_mc_out)
        {
            double P_low_limit = std::min(m_P_high_limit, std::max(10.E3, m_P_high_limit * 0.2));		//[kPa]

            //best_P_high = fminbr(P_low_limit, m_P_high_limit, &fmin_cb_opt_des_fixed_P_high, this, 1.0);
            best_P_high = m_P_high_limit;
        }
        opt_par.m_P_mc_out_guess = best_P_high;      //[kPa]
        //ms_opt_des_par.m_fixed_P_mc_out = true;

        // Pressure Ratio (min pressure)
        opt_par.m_fixed_PR_HP_to_LP = ms_auto_opt_des_par.m_fixed_PR_HP_to_LP;			//[-]
        if (opt_par.m_fixed_PR_HP_to_LP)
        {
            opt_par.m_PR_HP_to_LP_guess = ms_auto_opt_des_par.m_PR_HP_to_LP_guess;	//[-]
        }
        else
        {
            opt_par.m_PR_HP_to_LP_guess = PR_mc_guess;		//[-]
        }

        // Is recompression fraction fixed or optimized?
        if (ms_auto_opt_des_par.m_is_turbinesplit_ok <= 0.0)
        {   // fixed
            opt_par.m_split_frac_guess = std::abs(ms_auto_opt_des_par.m_is_turbinesplit_ok);
            opt_par.m_fixed_split_frac = true;
        }
        else
        {   // optimized
            opt_par.m_split_frac_guess = 0.5;
            opt_par.m_fixed_split_frac = false;
        }

        // LTR HTR UA Ratio
        opt_par.m_LT_frac_guess = 0.5;
        opt_par.m_fixed_LT_frac = false;
        if (ms_auto_opt_des_par.m_LTR_target_code != NS_HX_counterflow_eqs::OPTIMIZE_UA || ms_auto_opt_des_par.m_HTR_target_code != NS_HX_counterflow_eqs::OPTIMIZE_UA)
        {
            opt_par.m_fixed_LT_frac = true;
        }

        // Set Design Method
        if (opt_par.m_fixed_LT_frac == true)
            opt_par.m_design_method = 3;
        else
            opt_par.m_design_method = 2;

    }

    C_sco2_tsf_core::S_sco2_tsf_in optimal_inputs_out;
    error_code = optimize_par(ms_auto_opt_des_par, opt_par, optimal_inputs_out);

    if (error_code != 0)
        return;

    // Run Optimal Case
    m_optimal_tsf_core.set_inputs(optimal_inputs_out);
    error_code = m_optimal_tsf_core.solve();

    if (error_code != 0)
        return;

    // Check if cycle is below eta cutoff value
    if (m_optimal_tsf_core.m_outputs.m_eta_thermal < ms_auto_opt_des_par.m_eta_thermal_cutoff)
    {
        ms_des_solved.m_eta_thermal = m_optimal_tsf_core.m_outputs.m_eta_thermal;
        error_code = C_sco2_cycle_core::E_cycle_error_msg::E_ETA_THRESHOLD;
        return;
    }

    // Finalize Design (pass in reference to solved parameters)
    error_code = m_optimal_tsf_core.finalize_design(ms_des_solved);

}

/// <summary>
/// Core function to optimize cycle for target eta (variable total UA)
/// </summary>
void C_TurbineSplitFlow_Cycle::auto_opt_design_hit_eta_core(int& error_code, const double eta_thermal_target)
{
    return;
}

/// <summary>
/// Optimize Total Recuperator UA
/// totalUA -> bp -> UA split, pressure, recomp
/// </summary>
/// <param name="auto_par"></param>
/// <param name="opt_par"></param>
/// <param name="optimal_inputs"></param>
/// <returns></returns>
int C_TurbineSplitFlow_Cycle::optimize_totalUA(const S_auto_opt_design_parameters& auto_par,
    const S_opt_design_parameters& opt_par,
    C_sco2_tsf_core::S_sco2_tsf_in& optimal_inputs)
{
    return -1;;
}

/// <summary>
/// Optimize internal variables (UA split, pressure, recomp)
/// totalUA -> bp -> UA split, pressure, recomp
/// </summary>
/// <param name="auto_par"></param>
/// <param name="opt_par"></param>
/// <param name="core_inputs"></param>
/// <param name="optimal_inputs"></param>
/// <returns></returns>
int C_TurbineSplitFlow_Cycle::optimize_par(const S_auto_opt_design_parameters& auto_par,
    const S_opt_design_parameters& opt_par,
    C_sco2_tsf_core::S_sco2_tsf_in& optimal_inputs)
{

    // Set up baseline core inputs
    C_sco2_tsf_core::S_sco2_tsf_in core_inputs;
    {
        // From Auto Opt Design Parameters
        core_inputs.m_LTR_target_code = ms_auto_opt_des_par.m_LTR_target_code;
        core_inputs.m_LTR_UA = ms_auto_opt_des_par.m_LTR_UA;
        core_inputs.m_LTR_min_dT = ms_auto_opt_des_par.m_LTR_min_dT;
        core_inputs.m_LTR_eff_target = ms_auto_opt_des_par.m_LTR_eff_target;
        core_inputs.m_LTR_eff_max = ms_auto_opt_des_par.m_LTR_eff_max;

        core_inputs.m_LTR_od_UA_target_type = ms_auto_opt_des_par.m_LTR_od_UA_target_type;
        core_inputs.m_HTR_target_code = ms_auto_opt_des_par.m_HTR_target_code;
        core_inputs.m_HTR_UA = ms_auto_opt_des_par.m_HTR_UA;
        core_inputs.m_HTR_min_dT = ms_auto_opt_des_par.m_HTR_min_dT;
        core_inputs.m_HTR_eff_target = ms_auto_opt_des_par.m_HTR_eff_target;
        core_inputs.m_HTR_eff_max = ms_auto_opt_des_par.m_HTR_eff_max;
        core_inputs.m_HTR_od_UA_target_type = ms_auto_opt_des_par.m_HTR_od_UA_target_type;
        core_inputs.m_des_tol = ms_auto_opt_des_par.m_des_tol;
        core_inputs.m_is_des_air_cooler = ms_auto_opt_des_par.m_is_des_air_cooler;

        // From Constructor
        core_inputs.m_LTR_N_sub_hxrs = m_LTR_N_sub_hxrs;            // Comes from constructor (constant)
        core_inputs.m_HTR_N_sub_hxrs = m_HTR_N_sub_hxrs;            // Comes from constructor (constant)
        core_inputs.m_W_dot_net_design = m_W_dot_net;               // Comes from constructor (constant)
        core_inputs.m_T_mc_in = m_T_mc_in;                          // Comes from constructor (constant)
        core_inputs.m_T_t_in = m_T_t_in;                            // Comes from constructor (constant)
        core_inputs.m_DP_LTR = m_DP_LTR;                            // Comes from constructor (constant)
        core_inputs.m_DP_HTR = m_DP_HTR;                            // Comes from constructor (constant)
        core_inputs.m_DP_PC_main = m_DP_PC_main;                    // Comes from constructor (constant)
        core_inputs.m_DP_PHX = m_DP_PHX;                            // Comes from constructor (constant)
        core_inputs.m_eta_mc = m_eta_mc;                            // Comes from constructor (constant)
        core_inputs.m_eta_t = m_eta_t;                              // Comes from constructor (constant)
        core_inputs.m_eta_t2 = m_eta_t2;                            // Comes from constructor (constant)
        core_inputs.m_eta_generator = m_eta_generator;              // Comes from constructor (constant)
        core_inputs.m_frac_fan_power = m_frac_fan_power;            // Comes from constructor (constant)
        core_inputs.m_eta_fan = m_eta_fan;                          // Comes from constructor (constant)
        core_inputs.m_deltaP_cooler_frac = m_deltaP_cooler_frac;    // Comes from constructor (constant)
        core_inputs.m_T_amb_des = m_T_amb_des;                      // Comes from constructor (constant)
        core_inputs.m_elevation = m_elevation;                      // Comes from constructor (constant)
        core_inputs.m_N_nodes_pass = m_N_nodes_pass;                // Comes from constructor (constant)
        core_inputs.m_mc_comp_model_code = m_mc_comp_model_code;    // Comes from constructor (constant)
        core_inputs.m_N_turbine = m_N_turbine;                      // Comes from constructor (constant)
        core_inputs.m_f_inflation = m_f_inflation;                  // Comes from constructor (constant)

        // Handle design variables (check if fixed or free)
        {
            // Turbine Split Fraction
            if (opt_par.m_fixed_split_frac == true)
                core_inputs.m_split_frac = opt_par.m_split_frac_guess;

            // MC Outlet Pressure
            if (opt_par.m_fixed_P_mc_out == true)
                core_inputs.m_P_mc_out = opt_par.m_P_mc_out_guess;

            // Recuperator split fraction
            double LT_frac_local = opt_par.m_LT_frac_guess;
            if (ms_auto_opt_des_par.m_LTR_target_code == NS_HX_counterflow_eqs::OPTIMIZE_UA || ms_auto_opt_des_par.m_HTR_target_code == NS_HX_counterflow_eqs::OPTIMIZE_UA)
            {
                core_inputs.m_LTR_UA = ms_auto_opt_des_par.m_UA_rec_total * LT_frac_local;
                core_inputs.m_HTR_UA = ms_auto_opt_des_par.m_UA_rec_total * (1.0 - LT_frac_local);
            }
            else
            {
                core_inputs.m_LTR_UA = ms_auto_opt_des_par.m_LTR_UA;      //[kW/K]
                core_inputs.m_HTR_UA = ms_auto_opt_des_par.m_HTR_UA;      //[kW/K]
            }

            // Pressure Ratio is calculated in callback
        }

    }

    // Add applicable design variables to Optimizer
    int index = 0;

    std::vector<double> x(0);
    std::vector<double> lb(0);
    std::vector<double> ub(0);
    std::vector<double> scale(0);

    if (!auto_par.m_fixed_P_mc_out)
    {
        x.push_back(opt_par.m_P_mc_out_guess);
        lb.push_back(100.0);
        ub.push_back(m_P_high_limit);
        scale.push_back(500.0);

        index++;
    }

    if (!auto_par.m_fixed_PR_HP_to_LP)
    {
        x.push_back(opt_par.m_PR_HP_to_LP_guess);
        lb.push_back(0.0001);
        double PR_max = m_P_high_limit / 100.0;
        ub.push_back(PR_max);
        scale.push_back(0.2);

        index++;
    }

    if (!opt_par.m_fixed_split_frac)
    {
        x.push_back(opt_par.m_split_frac_guess);
        lb.push_back(0.0);
        ub.push_back(0.99);
        scale.push_back(0.05);
        index++;
    }

    if (!opt_par.m_fixed_LT_frac)
    {
        x.push_back(opt_par.m_LT_frac_guess);
        lb.push_back(0.0);
        ub.push_back(1.0);
        scale.push_back(0.05);

        index++;
    }

    // Make Optimizer (if there are variables to be optimized)
    int error_code = 0;
    C_sco2_tsf_core::S_sco2_tsf_in optimal_inputs_internal;
    if (index > 0)
    {
        // Set up instance of nlopt class and set optimization parameters
        nlopt::opt opt_des_cycle(nlopt::LN_SBPLX, index);
        opt_des_cycle.set_lower_bounds(lb);
        opt_des_cycle.set_upper_bounds(ub);
        opt_des_cycle.set_initial_step(scale);
        opt_des_cycle.set_xtol_rel(auto_par.m_des_opt_tol);
        opt_des_cycle.set_maxeval(50);

        // Set up core model that will be passed to objective function
        C_sco2_tsf_core tsf_core;
        tsf_core.set_inputs(core_inputs);

        // Make Tuple to pass in parameters
        std::tuple<C_TurbineSplitFlow_Cycle*, const S_auto_opt_design_parameters*, const S_opt_design_parameters*, C_sco2_tsf_core*> par_tuple = { this, &auto_par, &opt_par, &tsf_core };

        // Set max objective function
        opt_des_cycle.set_max_objective(nlopt_tsf_optimize_par_func, &par_tuple);
        double max_f = std::numeric_limits<double>::quiet_NaN();

        // Optimize
        nlopt::result result_des_cycle = opt_des_cycle.optimize(x, max_f);

        // Check if forced stop
        int flag = opt_des_cycle.get_force_stop();
        if (flag == true)
        {
            error_code = -1;
            return error_code;
        }

        // Get Optimal Input Case
        error_code = x_to_inputs(x, auto_par, opt_par, core_inputs);
        if (error_code != 0)
            return error_code;
    }
    // Nothing to optimize
    else
    {
        // Define P_mc_in (because pressure ratio and mc_out are constant)
        core_inputs.m_P_mc_in = core_inputs.m_P_mc_out / opt_par.m_PR_HP_to_LP_guess;

        // Simulate Case (don't actually need to run...)
        C_sco2_tsf_core core_model;
        core_model.set_inputs(core_inputs);
        error_code = core_model.solve();
    }

    // Set Optimal Inputs
    optimal_inputs = core_inputs;

    return error_code;
}

/// <summary>
/// Take optimizer array 'x', write appropriate values to S_sco2_tsf_in
/// </summary>
int C_TurbineSplitFlow_Cycle::x_to_inputs(const std::vector<double>& x,
    const S_auto_opt_design_parameters auto_par,
    const S_opt_design_parameters opt_par,
    C_sco2_tsf_core::S_sco2_tsf_in& core_inputs)
{
    // 'x' is array of inputs either being adjusted by optimizer or set constant
    // Finish defining core_inputs based on current 'x' values

    int error_message = 0;
    int index = 0;

    // Main compressor outlet pressure

    if (!auto_par.m_fixed_P_mc_out)
    {
        double P_mc_out = x[index];
        if (P_mc_out > m_P_high_limit)
            return -1;
        index++;

        // assign P_mc_out
        core_inputs.m_P_mc_out = P_mc_out;
    }


    // Main compressor pressure ratio
    double PR_mc_local = -999.9;
    double P_mc_in = -999.9;
    if (!opt_par.m_fixed_PR_HP_to_LP)
    {
        PR_mc_local = x[index];
        if (PR_mc_local > 50.0)
            return -1;
        index++;
        P_mc_in = core_inputs.m_P_mc_out / PR_mc_local;
    }
    else
    {
        if (opt_par.m_PR_HP_to_LP_guess >= 0.0)
        {
            PR_mc_local = opt_par.m_PR_HP_to_LP_guess;
            P_mc_in = core_inputs.m_P_mc_out / PR_mc_local;		//[kPa]
        }
        else
        {
            P_mc_in = std::abs(opt_par.m_PR_HP_to_LP_guess);		//[kPa]
        }
    }

    if (P_mc_in >= core_inputs.m_P_mc_out)
        return -1;
    if (P_mc_in <= 100.0)
        return -1;

    core_inputs.m_P_mc_in = P_mc_in;

    // Turbine split fraction
    if (!opt_par.m_fixed_split_frac)
    {
        core_inputs.m_split_frac = x[index];
        if (core_inputs.m_split_frac < 0.0)
            return -1;
        index++;
    }

    // Recuperator split fraction
    double LT_frac_local = -999.9;
    double LTR_UA, HTR_UA;
    if (!opt_par.m_fixed_LT_frac)
    {
        LT_frac_local = x[index];
        if (LT_frac_local > 1.0 || LT_frac_local < 0.0)
            return -1;
        index++;

        if (auto_par.m_LTR_target_code == NS_HX_counterflow_eqs::OPTIMIZE_UA || auto_par.m_HTR_target_code == NS_HX_counterflow_eqs::OPTIMIZE_UA)
        {
            LTR_UA = auto_par.m_UA_rec_total * LT_frac_local;
            HTR_UA = auto_par.m_UA_rec_total * (1.0 - LT_frac_local);

            // ASSIGN LTR_UA and HTR_UA
            core_inputs.m_LTR_UA = LTR_UA;
            core_inputs.m_HTR_UA = HTR_UA;
        }
    }

    return 0;
}

/// <summary>
/// Set optimized variables to NaN, to protect them from misuse
/// </summary>
int C_TurbineSplitFlow_Cycle::clear_x_inputs(const std::vector<double>& x,
    const S_auto_opt_design_parameters auto_par,
    const S_opt_design_parameters opt_par,
    C_sco2_tsf_core::S_sco2_tsf_in& core_inputs)
{
    // 'x' is array of inputs either being adjusted by optimizer or set constant
    // Finish defining core_inputs based on current 'x' values

    int error_message = 0;
    int index = 0;

    // Main compressor outlet pressure

    if (!auto_par.m_fixed_P_mc_out)
    {
        core_inputs.m_P_mc_out = std::numeric_limits<double>::quiet_NaN();
    }


    core_inputs.m_P_mc_in = std::numeric_limits<double>::quiet_NaN();

    // Turbine split fraction
    if (!opt_par.m_fixed_split_frac)
    {
        core_inputs.m_split_frac = std::numeric_limits<double>::quiet_NaN();
    }

    // Recuperator split fraction
    if (!opt_par.m_fixed_LT_frac)
    {
        if (auto_par.m_LTR_target_code == NS_HX_counterflow_eqs::OPTIMIZE_UA || auto_par.m_HTR_target_code == NS_HX_counterflow_eqs::OPTIMIZE_UA)
        {
            // ASSIGN LTR_UA and HTR_UA
            core_inputs.m_LTR_UA = std::numeric_limits<double>::quiet_NaN();;
            core_inputs.m_HTR_UA = std::numeric_limits<double>::quiet_NaN();;
        }
    }

    return 0;
}

/// <summary>
/// Calculate Objective Value (does not consider total UA minimization)
/// </summary>
double C_TurbineSplitFlow_Cycle::calc_objective(const S_auto_opt_design_parameters& auto_par,
    const S_opt_design_parameters& opt_par,
    const C_sco2_tsf_core& tsf_core)
{
    double obj = 0;
    double eta = tsf_core.m_outputs.m_eta_thermal;

    // Hit a target thermal efficiency
    if (opt_par.m_design_method == 1)
    {
        double eta_error = std::min(eta - opt_par.m_eta_thermal_target, 0.0);
        obj = 1.0 - std::abs(eta_error);
    }
    // Maximize thermal efficiency
    else
    {
        obj = eta;
    }

    // Penalize for PHX sco2 temp diff (if necessary)
    double penalty = 0;
    if (auto_par.m_des_objective_type == 2)
    {
        double phx_deltaT = tsf_core.m_outputs.m_temp[C_sco2_cycle_core::TURB_IN] - tsf_core.m_outputs.m_temp[C_sco2_cycle_core::LTR_HP_OUT];
        double under_min_deltaT = std::max(0.0, auto_par.m_min_phx_deltaT - phx_deltaT);
        
        double percent_err = under_min_deltaT / auto_par.m_min_phx_deltaT;

        penalty = percent_err;
    }

    obj = obj - penalty;

    return obj;
}

// ********************************************************************************** PUBLIC Methods C_TurbineSplitFlow_Cycle (: C_sco2_cycle_core) 

/// <summary>
/// Optimize Cycle Design for FIXED total recuperator UA
/// </summary>
/// <returns></returns>
int C_TurbineSplitFlow_Cycle::auto_opt_design(S_auto_opt_design_parameters& auto_opt_des_par_in)
{
    // Reset Counter
    m_opt_iteration_count = 0;

    // Collect auto_opt_des parameters
    ms_auto_opt_des_par = auto_opt_des_par_in;

    int auto_opt_des_error_code = 0;

    // Design cycle
    auto_opt_design_core(auto_opt_des_error_code);

    return auto_opt_des_error_code;
}

/// <summary>
/// Optimize Cycle design to hit target eta (optimize total recuperator UA)
/// </summary>
/// <param name="auto_opt_des_hit_eta_in"></param>
/// <param name="error_msg"></param>
/// <returns></returns>
int C_TurbineSplitFlow_Cycle::auto_opt_design_hit_eta(S_auto_opt_design_hit_eta_parameters& auto_opt_des_hit_eta_in, std::string& error_msg)
{
    return -1;
}

// ********************************************************************************** PUBLIC Objective Functions (internal use only)

/// <summary>
/// Objective Function for total UA (outermost layer)
/// Total UA -> Bypass Fraction -> pressure, split UA, recomp frac
/// </summary>
/// <returns>ONLY Objective Value</returns>
double C_TurbineSplitFlow_Cycle::optimize_totalUA_return_objective_metric(const std::vector<double>& x,
    const S_auto_opt_design_parameters& auto_par,
    const S_opt_design_parameters& opt_par
)
{
    return 0;
}

/// <summary>
/// Objective Function for general parameters 
/// Total UA -> pressure, split UA, recomp frac
/// </summary>
/// <returns>ONLY Objective Value</returns>
double C_TurbineSplitFlow_Cycle::optimize_par_return_objective_metric(const std::vector<double>& x,
    const S_auto_opt_design_parameters& auto_par,
    const S_opt_design_parameters& opt_par,
    C_sco2_tsf_core& tsf_core)
{
    // Update counter
    m_opt_iteration_count++; // global

    // Modify core inputs with Variable Parameters
    int error_code = x_to_inputs(x, auto_par, opt_par, tsf_core.m_inputs);

    if (error_code != 0)
        return -100000;

    // At this point, have fully defined core input struct
    // Run the core model
    error_code = tsf_core.solve();

    // Set Objective
    double objective_metric = -10000000000.0;
    if (error_code == 0)
    {
        objective_metric = calc_objective(auto_par, opt_par, tsf_core);
    }

    // Clear optimized inputs
    clear_x_inputs(x, auto_par, opt_par, tsf_core.m_inputs);

    return objective_metric;
}

// ********************************************************************************** Off Design Functions


void C_TurbineSplitFlow_Cycle::reset_ms_od_turbo_bal_csp_solved()
{
}

int C_TurbineSplitFlow_Cycle::off_design_fix_shaft_speeds(S_od_par& od_phi_par_in, double od_tol)
{
    return 0;
}

int C_TurbineSplitFlow_Cycle::solve_OD_all_coolers_fan_power(double T_amb, double od_tol, double& W_dot_fan)
{
    return 0;
}

int C_TurbineSplitFlow_Cycle::solve_OD_mc_cooler_fan_power(double T_amb, double od_tol, double& W_dot_mc_cooler_fan, double& P_co2_out)
{
    return 0;
}

int C_TurbineSplitFlow_Cycle::solve_OD_pc_cooler_fan_power(double T_amb, double od_tol, double& W_dot_pc_cooler_fan, double& P_co2_out)
{
    return 0;
}

double C_TurbineSplitFlow_Cycle::get_od_temp(int n_state_point)
{
    return 0.0;
}

double C_TurbineSplitFlow_Cycle::get_od_pres(int n_state_point)
{
    return 0.0;
}

void C_TurbineSplitFlow_Cycle::check_od_solution(double& diff_m_dot, double& diff_E_cycle, double& diff_Q_LTR, double& diff_Q_HTR)
{
}

void C_TurbineSplitFlow_Cycle::set_od_temp(int n_state_point, double temp_K)
{
}

void C_TurbineSplitFlow_Cycle::set_od_pres(int n_state_point, double pres_kPa)
{
}

void C_TurbineSplitFlow_Cycle::off_design_recompressor(double T_in, double P_in, double m_dot, double P_out, double tol, int& error_code, double& T_out)
{
}

void C_TurbineSplitFlow_Cycle::estimate_od_turbo_operation(double T_mc_in, double P_mc_in, double f_recomp, double T_t_in, double phi_mc, int& mc_error_code, double& mc_w_tip_ratio, double& P_mc_out, int& rc_error_code, double& rc_w_tip_ratio, double& rc_phi, bool is_update_ms_od_solved)
{
}


// ********************************************************************************** PUBLIC Methods defined outside of any class

double nlopt_tsf_optimize_totalUA_func(const std::vector<double>& x, std::vector<double>& grad, void* data)
{
    return 0;
}

double nlopt_tsf_optimize_par_func(const std::vector<double>& x, std::vector<double>& grad, void* data)
{
    // Unpack Data Tuple
    std::tuple<C_TurbineSplitFlow_Cycle*, const C_TurbineSplitFlow_Cycle::S_auto_opt_design_parameters*, const C_TurbineSplitFlow_Cycle::S_opt_design_parameters*, C_sco2_tsf_core*>* data_tuple
        = static_cast<std::tuple<C_TurbineSplitFlow_Cycle*, const C_TurbineSplitFlow_Cycle::S_auto_opt_design_parameters*, const C_TurbineSplitFlow_Cycle::S_opt_design_parameters*, C_sco2_tsf_core*>* > (data);

    C_TurbineSplitFlow_Cycle* frame = std::get<0>(*data_tuple);
    const C_TurbineSplitFlow_Cycle::S_auto_opt_design_parameters* auto_opt_par = std::get<1>(*data_tuple);
    const C_TurbineSplitFlow_Cycle::S_opt_design_parameters* opt_par = std::get<2>(*data_tuple);
    C_sco2_tsf_core* tsf_core = std::get<3>(*data_tuple);

    if (frame != NULL)
        return frame->optimize_par_return_objective_metric(x, *auto_opt_par, *opt_par, *tsf_core);
    else
        return 0.0;
}

