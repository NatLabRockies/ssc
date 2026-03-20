/*
BSD 3-Clause License

Copyright (c) Alliance for Energy Innovation, LLC. See also https://github.com/NREL/ssc/blob/develop/LICENSE
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

#include "sco2_htrbypass_cycle.h"
#include "sco2_cycle_components.h"

#include "CO2_properties.h"


#include "fmin.h"


// ********************************************************************************** C_sco2_htrbp_core CORE MODEL

void C_sco2_htrbp_core::initialize_solve()
{
    m_outputs.Init();
}

int C_sco2_htrbp_core::solve()
{
    initialize_solve();
    m_outputs.m_error_code = -1;

    // Apply scaling to the turbomachinery here
    {
        m_outputs.m_mc_ms.m_r_W_dot_scale = m_inputs.m_W_dot_net_design / 10.E3;	//[-]
        m_outputs.m_rc_ms.m_r_W_dot_scale = m_outputs.m_mc_ms.m_r_W_dot_scale;			//[-]
        m_outputs.m_t.m_r_W_dot_scale = m_outputs.m_mc_ms.m_r_W_dot_scale;				//[-]
    }

    // Initialize Recuperators
    {
        // LTR
        m_outputs.mc_LT_recup.initialize(m_inputs.m_LTR_N_sub_hxrs, m_inputs.m_LTR_od_UA_target_type, m_inputs.m_yr_inflation);
        // HTR
        m_outputs.mc_HT_recup.initialize(m_inputs.m_HTR_N_sub_hxrs, m_inputs.m_HTR_od_UA_target_type, m_inputs.m_yr_inflation);
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
        if (m_inputs.m_DP_LTR[0] < 0.0)
            m_outputs.m_pres[C_sco2_cycle_core::LTR_HP_OUT] = m_outputs.m_pres[C_sco2_cycle_core::MC_OUT] - m_outputs.m_pres[C_sco2_cycle_core::MC_OUT] * std::abs(m_inputs.m_DP_LTR[0]);		// relative pressure drop specified for LT recuperator (cold stream)
        else
            m_outputs.m_pres[C_sco2_cycle_core::LTR_HP_OUT] = m_outputs.m_pres[C_sco2_cycle_core::MC_OUT] - m_inputs.m_DP_LTR[0];				// absolute pressure drop specified for LT recuperator (cold stream)

        if ((m_inputs.m_LTR_target_code == NS_HX_counterflow_eqs::OPTIMIZE_UA && m_inputs.m_LTR_UA < 1.0E-12)
            || (m_inputs.m_LTR_target_code == NS_HX_counterflow_eqs::TARGET_UA && m_inputs.m_LTR_UA < 1.0E-12)
            || (m_inputs.m_LTR_target_code == NS_HX_counterflow_eqs::TARGET_MIN_DT && m_inputs.m_LTR_min_dT < 1.0E-12)
            || (m_inputs.m_LTR_target_code == NS_HX_counterflow_eqs::TARGET_EFFECTIVENESS && m_inputs.m_LTR_eff_target < 1.0E-12))
            m_outputs.m_pres[C_sco2_cycle_core::LTR_HP_OUT] = m_outputs.m_pres[C_sco2_cycle_core::MC_OUT];			// If there is no LT recuperator, there is no pressure drop

        m_outputs.m_pres[C_sco2_cycle_core::MIXER_OUT] = m_outputs.m_pres[C_sco2_cycle_core::LTR_HP_OUT];			// Assume no pressure drop in mixing valve
        m_outputs.m_pres[C_sco2_cycle_core::RC_OUT] = m_outputs.m_pres[C_sco2_cycle_core::LTR_HP_OUT];				// Assume no pressure drop in mixing valve

        if (m_inputs.m_DP_HTR[0] < 0.0)
            m_outputs.m_pres[C_sco2_cycle_core::HTR_HP_OUT] = m_outputs.m_pres[C_sco2_cycle_core::MIXER_OUT]
            - m_outputs.m_pres[C_sco2_cycle_core::MIXER_OUT] * std::abs(m_inputs.m_DP_HTR[0]);	// relative pressure drop specified for HT recuperator (cold stream)
        else
            m_outputs.m_pres[C_sco2_cycle_core::HTR_HP_OUT] = m_outputs.m_pres[C_sco2_cycle_core::MIXER_OUT] - m_inputs.m_DP_HTR[0];				// absolute pressure drop specified for HT recuperator (cold stream)

        if ((m_inputs.m_HTR_target_code == NS_HX_counterflow_eqs::OPTIMIZE_UA && m_inputs.m_HTR_UA < 1.0E-12)
            || (m_inputs.m_HTR_target_code == NS_HX_counterflow_eqs::TARGET_UA && m_inputs.m_HTR_UA < 1.0E-12)
            || (m_inputs.m_HTR_target_code == NS_HX_counterflow_eqs::TARGET_MIN_DT && m_inputs.m_HTR_min_dT < 1.0E-12)
            || (m_inputs.m_HTR_target_code == NS_HX_counterflow_eqs::TARGET_EFFECTIVENESS && m_inputs.m_HTR_eff_target < 1.0E-12))
            m_outputs.m_pres[C_sco2_cycle_core::HTR_HP_OUT] = m_outputs.m_pres[C_sco2_cycle_core::MIXER_OUT];		// If there is no HT recuperator, there is no pressure drop

        if (m_inputs.m_DP_PHX[0] < 0.0)
            m_outputs.m_pres[C_sco2_cycle_core::TURB_IN] = m_outputs.m_pres[C_sco2_cycle_core::HTR_HP_OUT] - m_outputs.m_pres[C_sco2_cycle_core::HTR_HP_OUT] * std::abs(m_inputs.m_DP_PHX[0]);	// relative pressure drop specified for PHX
        else
            m_outputs.m_pres[C_sco2_cycle_core::TURB_IN] = m_outputs.m_pres[C_sco2_cycle_core::HTR_HP_OUT] - m_inputs.m_DP_PHX[0];									// absolute pressure drop specified for PHX

        if (m_inputs.m_DP_PC_main[1] < 0.0)
            m_outputs.m_pres[C_sco2_cycle_core::LTR_LP_OUT] = m_outputs.m_pres[C_sco2_cycle_core::MC_IN] / (1.0 - std::abs(m_inputs.m_DP_PC_main[1]));					// relative pressure drop specified for precooler: P1=P9-P9*rel_DP => P1=P9*(1-rel_DP)
        else
            m_outputs.m_pres[C_sco2_cycle_core::LTR_LP_OUT] = m_outputs.m_pres[C_sco2_cycle_core::MC_IN] + m_inputs.m_DP_PC_main[1];

        if (m_inputs.m_DP_LTR[1] < 0.0)
            m_outputs.m_pres[C_sco2_cycle_core::HTR_LP_OUT] = m_outputs.m_pres[C_sco2_cycle_core::LTR_LP_OUT] / (1.0 - std::abs(m_inputs.m_DP_LTR[1]));	// relative pressure drop specified for LT recuperator (hot stream)
        else
            m_outputs.m_pres[C_sco2_cycle_core::HTR_LP_OUT] = m_outputs.m_pres[C_sco2_cycle_core::LTR_LP_OUT] + m_inputs.m_DP_LTR[1];					// absolute pressure drop specified for LT recuperator (hot stream)

        if ((m_inputs.m_LTR_target_code == NS_HX_counterflow_eqs::OPTIMIZE_UA && m_inputs.m_LTR_UA < 1.0E-12)
            || (m_inputs.m_LTR_target_code == NS_HX_counterflow_eqs::TARGET_UA && m_inputs.m_LTR_UA < 1.0E-12)
            || (m_inputs.m_LTR_target_code == NS_HX_counterflow_eqs::TARGET_MIN_DT && m_inputs.m_LTR_min_dT < 1.0E-12)
            || (m_inputs.m_LTR_target_code == NS_HX_counterflow_eqs::TARGET_EFFECTIVENESS && m_inputs.m_LTR_eff_target < 1.0E-12))
            m_outputs.m_pres[C_sco2_cycle_core::HTR_LP_OUT] = m_outputs.m_pres[C_sco2_cycle_core::LTR_LP_OUT];			// if there is no LT recuperator, there is no pressure drop

        if (m_inputs.m_DP_HTR[1] < 0.0)
            m_outputs.m_pres[C_sco2_cycle_core::TURB_OUT] = m_outputs.m_pres[C_sco2_cycle_core::HTR_LP_OUT] / (1.0 - std::abs(m_inputs.m_DP_HTR[1]));	// relative pressure drop specified for HT recuperator (hot stream)
        else
            m_outputs.m_pres[C_sco2_cycle_core::TURB_OUT] = m_outputs.m_pres[C_sco2_cycle_core::HTR_LP_OUT] + m_inputs.m_DP_HTR[1];				// absolute pressure drop specified for HT recuperator (hot stream)

        if ((m_inputs.m_HTR_target_code == NS_HX_counterflow_eqs::OPTIMIZE_UA && m_inputs.m_HTR_UA < 1.0E-12)
            || (m_inputs.m_HTR_target_code == NS_HX_counterflow_eqs::TARGET_UA && m_inputs.m_HTR_UA < 1.0E-12)
            || (m_inputs.m_HTR_target_code == NS_HX_counterflow_eqs::TARGET_MIN_DT && m_inputs.m_HTR_min_dT < 1.0E-12)
            || (m_inputs.m_HTR_target_code == NS_HX_counterflow_eqs::TARGET_EFFECTIVENESS && m_inputs.m_HTR_eff_target < 1.0E-12))
            m_outputs.m_pres[C_sco2_cycle_core::TURB_OUT] = m_outputs.m_pres[C_sco2_cycle_core::HTR_LP_OUT];		// if there is no HT recuperator, there is no pressure drop


        // Added pressures
        m_outputs.m_pres[C_sco2_cycle_core::BYPASS_OUT] = m_outputs.m_pres[C_sco2_cycle_core::HTR_HP_OUT];
        m_outputs.m_pres[C_sco2_cycle_core::MIXER2_OUT] = m_outputs.m_pres[C_sco2_cycle_core::HTR_HP_OUT];


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

    // Check that this cycle can produce power
    m_outputs.m_w_rc = std::numeric_limits<double>::quiet_NaN();
    {
        double eta_rc_isen = std::numeric_limits<double>::quiet_NaN();

        if (m_inputs.m_recomp_frac >= 1.E-12)
        {
            if (m_inputs.m_eta_rc < 0.0)		// need to convert polytropic efficiency to isentropic efficiency
            {
                int rc_error_code = 0;

                isen_eta_from_poly_eta(m_outputs.m_temp[C_sco2_cycle_core::MC_OUT], m_outputs.m_pres[C_sco2_cycle_core::LTR_LP_OUT], m_outputs.m_pres[C_sco2_cycle_core::RC_OUT], std::abs(m_inputs.m_eta_rc),
                    true, rc_error_code, eta_rc_isen);

                if (rc_error_code != 0)
                {
                    m_outputs.m_error_code = rc_error_code;
                    return m_outputs.m_error_code;
                }
            }
            else
                eta_rc_isen = m_inputs.m_eta_rc;

            int rc_error_code = 0;

            calculate_turbomachinery_outlet_1(m_outputs.m_temp[C_sco2_cycle_core::MC_OUT], m_outputs.m_pres[C_sco2_cycle_core::LTR_LP_OUT], m_outputs.m_pres[C_sco2_cycle_core::RC_OUT], eta_rc_isen,
                true, rc_error_code, m_outputs.m_w_rc);

            if (rc_error_code != 0)
            {
                m_outputs.m_error_code = rc_error_code;
                return m_outputs.m_error_code;
            }
        }
        else
            m_outputs.m_w_rc = 0.0;

        if (m_outputs.m_w_mc + m_outputs.m_w_rc + m_outputs.m_w_t <= 0.0)	// positive net power is impossible; return an error
        {
            m_outputs.m_error_code = (int)C_sco2_cycle_core::E_cycle_error_msg::E_CANNOT_PRODUCE_POWER;
            return m_outputs.m_error_code;
        }
    }

    // Solve the recuperators
    {
        C_mono_htrbp_core_HTR_des HTR_des_eq(this);
        C_monotonic_eq_solver HTR_des_solver(HTR_des_eq);

        {
            double T_HTR_LP_out_lower = m_outputs.m_temp[C_sco2_cycle_core::MC_OUT];		//[K] Coldest possible temperature
            double T_HTR_LP_out_upper = m_outputs.m_temp[C_sco2_cycle_core::TURB_OUT];		//[K] Hottest possible temperature

            double T_HTR_LP_out_guess_lower = std::min(T_HTR_LP_out_upper - 2.0, std::max(T_HTR_LP_out_lower + 15.0, 220.0 + 273.15));	//[K] There is nothing special about these guesses...
            double T_HTR_LP_out_guess_upper = std::min(T_HTR_LP_out_guess_lower + 20.0, T_HTR_LP_out_upper - 1.0);	//[K] There is nothing special about these guesses, either...

            HTR_des_solver.settings(m_inputs.m_des_tol * m_outputs.m_temp[C_sco2_cycle_core::MC_IN], 1000, T_HTR_LP_out_lower, T_HTR_LP_out_upper, false);

            double T_HTR_LP_out_solved, tol_T_HTR_LP_out_solved;
            T_HTR_LP_out_solved = tol_T_HTR_LP_out_solved = std::numeric_limits<double>::quiet_NaN();
            int iter_T_HTR_LP_out = -1;

            int T_HTR_LP_out_code = HTR_des_solver.solve(T_HTR_LP_out_guess_lower, T_HTR_LP_out_guess_upper, 0,
                T_HTR_LP_out_solved, tol_T_HTR_LP_out_solved, iter_T_HTR_LP_out);

            if (T_HTR_LP_out_code != C_monotonic_eq_solver::CONVERGED)
            {
                m_outputs.m_error_code = (int)C_sco2_cycle_core::E_cycle_error_msg::E_HTR_LTR_CONVERGENCE;
                return m_outputs.m_error_code;
            }

            double test = 0;
            solve_HTR(T_HTR_LP_out_solved, &test);
        }

    }

    // State 5 can now be fully defined
    {
        // Check if there is flow through HTR_HP
        if (m_outputs.m_m_dot_htr_hp <= 1e-12)
            m_outputs.m_enth[C_sco2_cycle_core::HTR_HP_OUT] = m_outputs.m_enth[C_sco2_cycle_core::MIXER_OUT];
        else
            m_outputs.m_enth[C_sco2_cycle_core::HTR_HP_OUT] = m_outputs.m_enth[C_sco2_cycle_core::MIXER_OUT] + m_outputs.m_Q_dot_HT / m_outputs.m_m_dot_htr_hp;						// Energy balance on cold stream of high-temp recuperator

        int prop_error_code = CO2_PH(m_outputs.m_pres[C_sco2_cycle_core::HTR_HP_OUT], m_outputs.m_enth[C_sco2_cycle_core::HTR_HP_OUT], &m_co2_props);
        if (prop_error_code != 0)
        {
            m_outputs.m_error_code = prop_error_code;
            return m_outputs.m_error_code;
        }
        m_outputs.m_temp[C_sco2_cycle_core::HTR_HP_OUT] = m_co2_props.temp;
        m_outputs.m_entr[C_sco2_cycle_core::HTR_HP_OUT] = m_co2_props.entr;
        m_outputs.m_dens[C_sco2_cycle_core::HTR_HP_OUT] = m_co2_props.dens;
    }

    // Calculate total work and heat metrics
    {
        // Work
        m_outputs.m_W_dot_mc = m_outputs.m_w_mc * m_outputs.m_m_dot_mc;		//[kWe]
        m_outputs.m_W_dot_rc = m_outputs.m_w_rc * m_outputs.m_m_dot_rc;		//[kWe]
        m_outputs.m_W_dot_t = m_outputs.m_w_t * m_outputs.m_m_dot_t;		//[kWe]
        m_outputs.m_W_dot_net = m_outputs.m_W_dot_mc + m_outputs.m_W_dot_rc + m_outputs.m_W_dot_t;

        // Air Cooler (heat rejection unit)
        m_outputs.m_W_dot_air_cooler = m_inputs.m_frac_fan_power * m_outputs.m_W_dot_net;
        m_outputs.m_Q_dot_air_cooler = m_outputs.m_m_dot_mc * (m_outputs.m_enth[C_sco2_cycle_core::LTR_LP_OUT] - m_outputs.m_enth[C_sco2_cycle_core::MC_IN]);

        // Total Heat Entering sco2
        m_outputs.m_Q_dot_total = m_outputs.m_W_dot_net + m_outputs.m_Q_dot_air_cooler;

        // LTR
        m_outputs.m_Q_dot_LTR_LP = m_outputs.m_m_dot_t * (m_outputs.m_enth[C_sco2_cycle_core::HTR_LP_OUT] - m_outputs.m_enth[C_sco2_cycle_core::LTR_LP_OUT]);
        m_outputs.m_Q_dot_LTR_HP = m_outputs.m_m_dot_mc * (m_outputs.m_enth[C_sco2_cycle_core::LTR_HP_OUT] - m_outputs.m_enth[C_sco2_cycle_core::MC_OUT]);

        // LTR
        m_outputs.m_Q_dot_HTR_LP = m_outputs.m_m_dot_t * (m_outputs.m_enth[C_sco2_cycle_core::TURB_OUT] - m_outputs.m_enth[C_sco2_cycle_core::HTR_LP_OUT]);
        m_outputs.m_Q_dot_HTR_HP = m_outputs.m_m_dot_htr_hp * (m_outputs.m_enth[C_sco2_cycle_core::HTR_HP_OUT] - m_outputs.m_enth[C_sco2_cycle_core::MIXER_OUT]);
    }

    // Calculate Bypass Energy
    {
        // Set Bypass Temp based on HTR_HP_OUT
        m_outputs.m_temp[C_sco2_cycle_core::BYPASS_OUT] = m_outputs.m_temp[C_sco2_cycle_core::HTR_HP_OUT] + m_inputs.m_dT_BP;

        // Calculate BYPASS_OUT properties
        int prop_error_code = CO2_TP(m_outputs.m_temp[C_sco2_cycle_core::BYPASS_OUT], m_outputs.m_pres[C_sco2_cycle_core::BYPASS_OUT], &m_co2_props);
        if (prop_error_code != 0)
        {
            m_outputs.m_error_code = -1;
            return m_outputs.m_error_code;
        }
        m_outputs.m_enth[C_sco2_cycle_core::BYPASS_OUT] = m_co2_props.enth;
        m_outputs.m_entr[C_sco2_cycle_core::BYPASS_OUT] = m_co2_props.entr;
        m_outputs.m_dens[C_sco2_cycle_core::BYPASS_OUT] = m_co2_props.dens;

        // Calculate Heat Transfer in Bypass
        m_outputs.m_Q_dot_BP = m_outputs.m_m_dot_bp * (m_outputs.m_enth[C_sco2_cycle_core::BYPASS_OUT] - m_outputs.m_enth[C_sco2_cycle_core::MIXER_OUT]);
    }

    // Simulate Mixer 2
    {
        // If Bypass and HTR have flow
        if (m_inputs.m_bypass_frac >= 1e-12 && m_inputs.m_bypass_frac <= (1.0 - 1e-12))
        {
            m_outputs.m_enth[C_sco2_cycle_core::MIXER2_OUT] = (1.0 - m_inputs.m_bypass_frac) * m_outputs.m_enth[C_sco2_cycle_core::HTR_HP_OUT] +
                m_inputs.m_bypass_frac * m_outputs.m_enth[C_sco2_cycle_core::BYPASS_OUT];	//[C_sco2_cycle_core::kJ/kg]

            int prop_error_code = CO2_PH(m_outputs.m_pres[C_sco2_cycle_core::MIXER2_OUT], m_outputs.m_enth[C_sco2_cycle_core::MIXER2_OUT], &m_co2_props);
            if (prop_error_code != 0)
            {
                m_outputs.m_error_code = -1;
                return m_outputs.m_error_code;
            }
            m_outputs.m_temp[C_sco2_cycle_core::MIXER2_OUT] = m_co2_props.temp;		//[C_sco2_cycle_core::K]
            m_outputs.m_entr[C_sco2_cycle_core::MIXER2_OUT] = m_co2_props.entr;		//[C_sco2_cycle_core::kJ/kg-K]
            m_outputs.m_dens[C_sco2_cycle_core::MIXER2_OUT] = m_co2_props.dens;		//[C_sco2_cycle_core::kg/m^3]

        }
        // Flow only through HTR
        else if (m_inputs.m_bypass_frac <= (1.0 - 1e-12))
        {
            m_outputs.m_temp[C_sco2_cycle_core::MIXER2_OUT] = m_outputs.m_temp[C_sco2_cycle_core::HTR_HP_OUT];		//[C_sco2_cycle_core::K]
            m_outputs.m_enth[C_sco2_cycle_core::MIXER2_OUT] = m_outputs.m_enth[C_sco2_cycle_core::HTR_HP_OUT];		//[C_sco2_cycle_core::kJ/kg]
            m_outputs.m_entr[C_sco2_cycle_core::MIXER2_OUT] = m_outputs.m_entr[C_sco2_cycle_core::HTR_HP_OUT];		//[C_sco2_cycle_core::kJ/kg-K]
            m_outputs.m_dens[C_sco2_cycle_core::MIXER2_OUT] = m_outputs.m_dens[C_sco2_cycle_core::HTR_HP_OUT];		//[C_sco2_cycle_core::kg/m^3]
        }
        // Flow only through Bypass
        else
        {
            m_outputs.m_temp[C_sco2_cycle_core::MIXER2_OUT] = m_outputs.m_temp[C_sco2_cycle_core::BYPASS_OUT];		//[C_sco2_cycle_core::K]
            m_outputs.m_enth[C_sco2_cycle_core::MIXER2_OUT] = m_outputs.m_enth[C_sco2_cycle_core::BYPASS_OUT];		//[C_sco2_cycle_core::kJ/kg]
            m_outputs.m_entr[C_sco2_cycle_core::MIXER2_OUT] = m_outputs.m_entr[C_sco2_cycle_core::BYPASS_OUT];		//[C_sco2_cycle_core::kJ/kg-K]
            m_outputs.m_dens[C_sco2_cycle_core::MIXER2_OUT] = m_outputs.m_dens[C_sco2_cycle_core::BYPASS_OUT];		//[C_sco2_cycle_core::kg/m^3]
        }
    }

    // Calculate PHX Heat Transfer
    {
        m_outputs.m_Q_dot_PHX = m_outputs.m_m_dot_t * (m_outputs.m_enth[C_sco2_cycle_core::TURB_IN] - m_outputs.m_enth[C_sco2_cycle_core::MIXER2_OUT]);
    }

    // Back Calculate and Check values
    {
        // Bypass Temps
        double bp_temp_in = m_outputs.m_temp[C_sco2_cycle_core::MIXER_OUT];
        double bp_temp_out = m_outputs.m_temp[C_sco2_cycle_core::BYPASS_OUT];

        double real_q_dot_total = m_outputs.m_W_dot_t + m_outputs.m_Q_dot_air_cooler;

        double qSum = m_outputs.m_Q_dot_total;
        double qSum_calc = m_outputs.m_Q_dot_BP + m_outputs.m_Q_dot_PHX;

        int x = 0;
    }

    // HTF
    {
        // Check if HTF mdot is already assigned
        if (m_inputs.m_set_HTF_mdot > 0)
        {
            // Mdot is Set
            m_outputs.m_m_dot_HTF = m_inputs.m_set_HTF_mdot;

            // Calculate PHX HTF Outlet Temperature
            m_outputs.m_T_HTF_PHX_out = m_inputs.m_T_HTF_PHX_inlet - m_outputs.m_Q_dot_PHX / (m_outputs.m_m_dot_HTF * m_inputs.m_cp_HTF);

            // Back Calculate PHX cold approach
            m_outputs.m_HTF_PHX_cold_approach = m_outputs.m_T_HTF_PHX_out - m_outputs.m_temp[C_sco2_cycle_core::MIXER2_OUT];
        }
        else
        {
            // Use HTF Bypass cold approach to calculate PHX outlet Temperature
            m_outputs.m_T_HTF_PHX_out = m_inputs.m_HTF_PHX_cold_approach_input + m_outputs.m_temp[C_sco2_cycle_core::MIXER2_OUT];
            m_outputs.m_HTF_PHX_cold_approach = m_inputs.m_HTF_PHX_cold_approach_input;

            // Calculate HTF mdot
            m_outputs.m_m_dot_HTF = m_outputs.m_Q_dot_PHX / ((m_inputs.m_T_HTF_PHX_inlet - m_outputs.m_T_HTF_PHX_out) * m_inputs.m_cp_HTF);
        }

        // Calculate Bypass Out Temperature
        m_outputs.m_T_HTF_BP_outlet = m_outputs.m_T_HTF_PHX_out - (m_outputs.m_Q_dot_BP / (m_outputs.m_m_dot_HTF * m_inputs.m_cp_HTF));

        // Calculate HTF Bypass Cold Approach
        m_outputs.m_HTF_BP_cold_approach = m_outputs.m_T_HTF_BP_outlet - m_outputs.m_temp[C_sco2_cycle_core::MIXER_OUT];

    }

    // Define Heat Exchangers and Air Cooler
    {
        // PHX
        C_HeatExchanger::S_design_parameters PHX_des_par;
        PHX_des_par.m_DP_design[0] = m_outputs.m_pres[C_sco2_cycle_core::MIXER2_OUT] - m_outputs.m_pres[C_sco2_cycle_core::TURB_IN];
        PHX_des_par.m_DP_design[1] = 0.0;
        PHX_des_par.m_m_dot_design[0] = m_outputs.m_m_dot_t;
        PHX_des_par.m_m_dot_design[1] = 0.0;
        PHX_des_par.m_Q_dot_design = m_outputs.m_m_dot_t * (m_outputs.m_enth[C_sco2_cycle_core::TURB_IN] - m_outputs.m_enth[C_sco2_cycle_core::MIXER2_OUT]);
        m_outputs.m_PHX.initialize(PHX_des_par);

        // BPX
        C_HeatExchanger::S_design_parameters BPX_des_par;
        BPX_des_par.m_DP_design[0] = m_outputs.m_pres[C_sco2_cycle_core::MIXER_OUT] - m_outputs.m_pres[C_sco2_cycle_core::BYPASS_OUT];
        BPX_des_par.m_DP_design[1] = 0.0;
        BPX_des_par.m_m_dot_design[0] = m_outputs.m_m_dot_bp;
        BPX_des_par.m_m_dot_design[1] = 0.0;
        BPX_des_par.m_Q_dot_design = m_outputs.m_m_dot_bp * (m_outputs.m_enth[C_sco2_cycle_core::BYPASS_OUT] - m_outputs.m_enth[C_sco2_cycle_core::MIXER_OUT]);
        m_outputs.m_BPX.initialize(BPX_des_par);

        // Air Cooler
        C_HeatExchanger::S_design_parameters PC_des_par;
        PC_des_par.m_DP_design[0] = 0.0;
        PC_des_par.m_DP_design[1] = m_outputs.m_pres[C_sco2_cycle_core::LTR_LP_OUT] - m_outputs.m_pres[C_sco2_cycle_core::MC_IN];
        PC_des_par.m_m_dot_design[0] = 0.0;
        PC_des_par.m_m_dot_design[1] = m_outputs.m_m_dot_mc;
        PC_des_par.m_Q_dot_design = m_outputs.m_m_dot_mc * (m_outputs.m_enth[C_sco2_cycle_core::LTR_LP_OUT] - m_outputs.m_enth[C_sco2_cycle_core::MC_IN]);
        m_outputs.m_PC.initialize(PC_des_par);
    }

    // Calculate Thermal Efficiency
    {
        m_outputs.m_eta_thermal = m_outputs.m_W_dot_net / m_outputs.m_Q_dot_total;
    }

    m_outputs.m_error_code = 0;
    return m_outputs.m_error_code;
}

int C_sco2_htrbp_core::finalize_design(C_sco2_cycle_core::S_design_solved& design_solved)
{
    // Design Main Compressor
    {
        int mc_design_err = m_outputs.m_mc_ms.design_given_outlet_state(m_inputs.m_mc_comp_model_code, m_outputs.m_temp[C_sco2_cycle_core::MC_IN],
            m_outputs.m_pres[C_sco2_cycle_core::MC_IN],
            m_outputs.m_m_dot_mc,
            m_outputs.m_temp[C_sco2_cycle_core::MC_OUT],
            m_outputs.m_pres[C_sco2_cycle_core::MC_OUT],
            m_inputs.m_des_tol,
            m_inputs.m_yr_inflation);

        if (mc_design_err != 0)
        {
            m_outputs.m_error_code = mc_design_err;
            return m_outputs.m_error_code;
        }
    }

    // Design Recompressor
    if (m_inputs.m_recomp_frac > 0.01)
    {
        int rc_des_err = m_outputs.m_rc_ms.design_given_outlet_state(m_inputs.m_rc_comp_model_code, m_outputs.m_temp[C_sco2_cycle_core::LTR_LP_OUT],
            m_outputs.m_pres[C_sco2_cycle_core::LTR_LP_OUT],
            m_outputs.m_m_dot_rc,
            m_outputs.m_temp[C_sco2_cycle_core::RC_OUT],
            m_outputs.m_pres[C_sco2_cycle_core::RC_OUT],
            m_inputs.m_des_tol,
            m_inputs.m_yr_inflation);

        if (rc_des_err != 0)
        {
            m_outputs.m_error_code = rc_des_err;
            return m_outputs.m_error_code;
        }

        design_solved.m_is_rc = true;
    }
    else
    {
        design_solved.m_is_rc = false;
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
        // Inflation target yr
        t_des_par.m_yr_inflation = m_inputs.m_yr_inflation;


        int turb_size_error_code = 0;
        m_outputs.m_t.turbine_sizing(t_des_par, turb_size_error_code);

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
        s_air_cooler_des_par_dep.m_T_hot_in_des = m_outputs.m_temp[C_sco2_cycle_core::LTR_LP_OUT];  // [K]
        s_air_cooler_des_par_dep.m_P_hot_in_des = m_outputs.m_pres[C_sco2_cycle_core::LTR_LP_OUT];  // [kPa]
        s_air_cooler_des_par_dep.m_m_dot_total = m_outputs.m_m_dot_mc;                // [kg/s]

        // This pressure drop is currently uncoupled from the cycle design
        double cooler_deltaP = m_outputs.m_pres[C_sco2_cycle_core::LTR_LP_OUT] - m_outputs.m_pres[C_sco2_cycle_core::MC_IN];    // [kPa]
        if (cooler_deltaP == 0.0)
            s_air_cooler_des_par_dep.m_delta_P_des = m_inputs.m_deltaP_cooler_frac * m_outputs.m_pres[C_sco2_cycle_core::LTR_LP_OUT];    // [kPa]
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
        s_air_cooler_des_par_ind.m_yr_inflation = m_inputs.m_yr_inflation;     // [-]

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
    design_solved.ms_rc_ms_des_solved = *m_outputs.m_rc_ms.get_design_solved();
    design_solved.ms_t_des_solved = *m_outputs.m_t.get_design_solved();
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
    design_solved.m_m_dot_rc = m_outputs.m_m_dot_rc;
    design_solved.m_m_dot_t = m_outputs.m_m_dot_t;
    design_solved.m_recomp_frac = m_outputs.m_m_dot_rc / m_outputs.m_m_dot_t;
    design_solved.m_bypass_frac = m_inputs.m_bypass_frac;

    design_solved.m_UA_LTR = m_inputs.m_LTR_UA;
    design_solved.m_UA_HTR = m_inputs.m_HTR_UA;

    design_solved.m_W_dot_t = m_outputs.m_W_dot_t;		//[kWe]
    design_solved.m_W_dot_mc = m_outputs.m_W_dot_mc;		//[kWe]
    design_solved.m_W_dot_rc = m_outputs.m_W_dot_rc;		//[kWe]

    design_solved.m_W_dot_cooler_tot = m_outputs.mc_air_cooler.get_design_solved()->m_W_dot_fan * 1.E3;	//[kWe] convert from MWe

    return 0;
}

int C_sco2_htrbp_core::solve_HTR(double T_HTR_LP_OUT_guess, double* diff_T_HTR_LP_out)
{
    m_outputs.m_w_rc = m_outputs.m_m_dot_t = m_outputs.m_m_dot_rc = m_outputs.m_m_dot_mc = m_outputs.m_Q_dot_LT = m_outputs.m_Q_dot_HT = std::numeric_limits<double>::quiet_NaN();

    // Set temperature guess
    m_outputs.m_temp[C_sco2_cycle_core::HTR_LP_OUT] = T_HTR_LP_OUT_guess;		//[K]	

    // Solve HTR_LP_OUT properties
    {
        int prop_error_code = CO2_TP(m_outputs.m_temp[C_sco2_cycle_core::HTR_LP_OUT], m_outputs.m_pres[C_sco2_cycle_core::HTR_LP_OUT], &m_co2_props);
        if (prop_error_code != 0)
        {
            *diff_T_HTR_LP_out = std::numeric_limits<double>::quiet_NaN();
            return prop_error_code;
        }
        m_outputs.m_enth[C_sco2_cycle_core::HTR_LP_OUT] = m_co2_props.enth;
        m_outputs.m_entr[C_sco2_cycle_core::HTR_LP_OUT] = m_co2_props.entr;
        m_outputs.m_dens[C_sco2_cycle_core::HTR_LP_OUT] = m_co2_props.dens;
    }

    // Solve for the LTR solution
    {
        double T_LTR_LP_out_lower = m_outputs.m_temp[C_sco2_cycle_core::MC_OUT];		//[K] Coldest possible outlet temperature
        double T_LTR_LP_out_upper = m_outputs.m_temp[C_sco2_cycle_core::HTR_LP_OUT];	//[K] Hottest possible outlet temperature

        double T_LTR_LP_out_guess_upper = std::min(T_LTR_LP_out_upper, T_LTR_LP_out_lower + 15.0);	//[K] There is nothing special about using 15 here...
        double T_LTR_LP_out_guess_lower = std::min(T_LTR_LP_out_guess_upper * 0.99, T_LTR_LP_out_lower + 2.0);	//[K] There is nothing special about using 2 here...

        C_mono_htrbp_core_LTR_des LTR_des_eq(this);
        C_monotonic_eq_solver LTR_des_solver(LTR_des_eq);

        LTR_des_solver.settings(m_inputs.m_des_tol * m_outputs.m_temp[C_sco2_cycle_core::MC_IN], 1000, T_LTR_LP_out_lower,
            T_LTR_LP_out_upper, false);

        double T_LTR_LP_out_solved = std::numeric_limits<double>::quiet_NaN();
        double tol_T_LTR_LP_out_solved = std::numeric_limits<double>::quiet_NaN();
        int iter_T_LTR_LP_out = -1;

        int T_LTR_LP_out_code = LTR_des_solver.solve(T_LTR_LP_out_guess_lower, T_LTR_LP_out_guess_upper, 0, T_LTR_LP_out_solved,
            tol_T_LTR_LP_out_solved, iter_T_LTR_LP_out);

        if (T_LTR_LP_out_code != C_monotonic_eq_solver::CONVERGED)
        {
            return 31;
        }
    }

    // Know LTR performance so we can calculate the HP outlet (Energy balance on LTR HP stream)
    {
        m_outputs.m_enth[C_sco2_cycle_core::LTR_HP_OUT] = m_outputs.m_enth[C_sco2_cycle_core::MC_OUT] + m_outputs.m_Q_dot_LT / m_outputs.m_m_dot_mc;		//[kJ/kg]
        int prop_error_code = CO2_PH(m_outputs.m_pres[C_sco2_cycle_core::LTR_HP_OUT], m_outputs.m_enth[C_sco2_cycle_core::LTR_HP_OUT], &m_co2_props);
        if (prop_error_code != 0)
        {
            *diff_T_HTR_LP_out = std::numeric_limits<double>::quiet_NaN();
            return prop_error_code;
        }
        m_outputs.m_temp[C_sco2_cycle_core::LTR_HP_OUT] = m_co2_props.temp;	//[K]
        m_outputs.m_entr[C_sco2_cycle_core::LTR_HP_OUT] = m_co2_props.entr;	//[kJ/kg-K]
        m_outputs.m_dens[C_sco2_cycle_core::LTR_HP_OUT] = m_co2_props.dens;	//[kg/m^3]	
    }

    // Simulate the Mixer
    if (m_inputs.m_recomp_frac >= 1.E-12)
    {
        m_outputs.m_enth[C_sco2_cycle_core::MIXER_OUT] = (1.0 - m_inputs.m_recomp_frac) * m_outputs.m_enth[C_sco2_cycle_core::LTR_HP_OUT]
            + m_inputs.m_recomp_frac * m_outputs.m_enth[C_sco2_cycle_core::RC_OUT];	//[kJ/kg]
        int prop_error_code = CO2_PH(m_outputs.m_pres[C_sco2_cycle_core::MIXER_OUT], m_outputs.m_enth[C_sco2_cycle_core::MIXER_OUT], &m_co2_props);
        if (prop_error_code != 0)
        {
            *diff_T_HTR_LP_out = std::numeric_limits<double>::quiet_NaN();
            return prop_error_code;
        }
        m_outputs.m_temp[C_sco2_cycle_core::MIXER_OUT] = m_co2_props.temp;		//[K]
        m_outputs.m_entr[C_sco2_cycle_core::MIXER_OUT] = m_co2_props.entr;		//[kJ/kg-K]
        m_outputs.m_dens[C_sco2_cycle_core::MIXER_OUT] = m_co2_props.dens;		//[kg/m^3]
    }
    else
    {	// No recompressor, so no mixing required, and HTR HP inlet = LTR HP outlet
        m_outputs.m_temp[C_sco2_cycle_core::MIXER_OUT] = m_outputs.m_temp[C_sco2_cycle_core::LTR_HP_OUT];		//[K]
        m_outputs.m_enth[C_sco2_cycle_core::MIXER_OUT] = m_outputs.m_enth[C_sco2_cycle_core::LTR_HP_OUT];		//[kJ/kg]
        m_outputs.m_entr[C_sco2_cycle_core::MIXER_OUT] = m_outputs.m_entr[C_sco2_cycle_core::LTR_HP_OUT];		//[kJ/kg-K]
        m_outputs.m_dens[C_sco2_cycle_core::MIXER_OUT] = m_outputs.m_dens[C_sco2_cycle_core::LTR_HP_OUT];		//[kg/m^3]
    }

    // Solve Mass Flow rates for HTR_HP_OUT and Bypass
    {
        m_outputs.m_m_dot_bp = m_inputs.m_bypass_frac * m_outputs.m_m_dot_t;
        m_outputs.m_m_dot_htr_hp = m_outputs.m_m_dot_t - m_outputs.m_m_dot_bp;
    }

    // Find the design solution of the HTR
    double T_HTR_LP_out_calc = std::numeric_limits<double>::quiet_NaN();
    {
        // If there is no flow through HTR HP side
        if (m_outputs.m_m_dot_htr_hp < 1e-12)
        {
            m_outputs.m_Q_dot_HT = 0;
            T_HTR_LP_out_calc = m_outputs.m_temp[C_sco2_cycle_core::TURB_OUT];
        }

        // If there is flow through HTR HP side
        else
        {
            m_outputs.mc_HT_recup.design_for_target__calc_outlet(m_inputs.m_HTR_target_code,
                m_inputs.m_HTR_UA, m_inputs.m_HTR_min_dT, m_inputs.m_HTR_eff_target,
                m_inputs.m_HTR_eff_max,
                m_outputs.m_temp[C_sco2_cycle_core::MIXER_OUT], m_outputs.m_pres[C_sco2_cycle_core::MIXER_OUT], m_outputs.m_m_dot_htr_hp, m_outputs.m_pres[C_sco2_cycle_core::HTR_HP_OUT],
                m_outputs.m_temp[C_sco2_cycle_core::TURB_OUT], m_outputs.m_pres[C_sco2_cycle_core::TURB_OUT], m_outputs.m_m_dot_t, m_outputs.m_pres[C_sco2_cycle_core::HTR_LP_OUT],
                m_inputs.m_des_tol,
                m_outputs.m_Q_dot_HT, m_outputs.m_temp[C_sco2_cycle_core::HTR_HP_OUT], T_HTR_LP_out_calc);
        }

    }

    *diff_T_HTR_LP_out = T_HTR_LP_out_calc - T_HTR_LP_OUT_guess;

    return 0;
}

int C_sco2_htrbp_core::solve_LTR(double T_LTR_LP_OUT_guess, double* diff_T_LTR_LP_out)
{
    m_outputs.m_w_rc = m_outputs.m_m_dot_t = m_outputs.m_m_dot_rc = m_outputs.m_m_dot_mc = m_outputs.m_Q_dot_LT = m_outputs.m_Q_dot_HT = std::numeric_limits<double>::quiet_NaN();

    // Set LTR_LP_OUT guess
    m_outputs.m_temp[C_sco2_cycle_core::LTR_LP_OUT] = T_LTR_LP_OUT_guess;

    // First, solve the recompressor model as necessary
    if (m_inputs.m_recomp_frac >= 1.E-12)
    {
        double eta_rc_isen = std::numeric_limits<double>::quiet_NaN();

        if (m_inputs.m_eta_rc < 0.0)		// recalculate isen. efficiency of recompressor because inlet temp changes
        {
            int rc_error_code = 0;
            isen_eta_from_poly_eta(m_outputs.m_temp[C_sco2_cycle_core::LTR_LP_OUT], m_outputs.m_pres[C_sco2_cycle_core::LTR_LP_OUT],
                m_outputs.m_pres[C_sco2_cycle_core::RC_OUT], std::abs(m_inputs.m_eta_rc), true,
                rc_error_code, eta_rc_isen);

            if (rc_error_code != 0)
            {
                *diff_T_LTR_LP_out = std::numeric_limits<double>::quiet_NaN();
                return rc_error_code;
            }
        }
        else
        {
            eta_rc_isen = m_inputs.m_eta_rc;
        }

        int rc_error_code = 0;

        calculate_turbomachinery_outlet_1(m_outputs.m_temp[C_sco2_cycle_core::LTR_LP_OUT], m_outputs.m_pres[C_sco2_cycle_core::LTR_LP_OUT], m_outputs.m_pres[C_sco2_cycle_core::RC_OUT], eta_rc_isen, true, rc_error_code,
            m_outputs.m_enth[C_sco2_cycle_core::LTR_LP_OUT], m_outputs.m_entr[C_sco2_cycle_core::LTR_LP_OUT], m_outputs.m_dens[C_sco2_cycle_core::LTR_LP_OUT], m_outputs.m_temp[C_sco2_cycle_core::RC_OUT], m_outputs.m_enth[C_sco2_cycle_core::RC_OUT],
            m_outputs.m_entr[C_sco2_cycle_core::RC_OUT], m_outputs.m_dens[C_sco2_cycle_core::RC_OUT], m_outputs.m_w_rc);

        if (rc_error_code != 0)
        {
            *diff_T_LTR_LP_out = std::numeric_limits<double>::quiet_NaN();
            return rc_error_code;
        }
    }
    else
    {
        m_outputs.m_w_rc = 0.0;		// no recompressor
        int prop_error_code = CO2_TP(m_outputs.m_temp[C_sco2_cycle_core::LTR_LP_OUT], m_outputs.m_pres[C_sco2_cycle_core::LTR_LP_OUT], &m_co2_props);
        if (prop_error_code != 0)
        {
            *diff_T_LTR_LP_out = std::numeric_limits<double>::quiet_NaN();
            return prop_error_code;
        }
        m_outputs.m_enth[C_sco2_cycle_core::LTR_LP_OUT] = m_co2_props.enth;
        m_outputs.m_entr[C_sco2_cycle_core::LTR_LP_OUT] = m_co2_props.entr;
        m_outputs.m_dens[C_sco2_cycle_core::LTR_LP_OUT] = m_co2_props.dens;
        m_outputs.m_temp[C_sco2_cycle_core::RC_OUT] = m_outputs.m_temp[C_sco2_cycle_core::LTR_LP_OUT];
        m_outputs.m_enth[C_sco2_cycle_core::RC_OUT] = m_outputs.m_enth[C_sco2_cycle_core::LTR_LP_OUT];
        m_outputs.m_entr[C_sco2_cycle_core::RC_OUT] = m_outputs.m_entr[C_sco2_cycle_core::LTR_LP_OUT];
        m_outputs.m_dens[C_sco2_cycle_core::RC_OUT] = m_outputs.m_dens[C_sco2_cycle_core::LTR_LP_OUT];
    }

    // Solve Mass Flow Rates
    {
        m_outputs.m_m_dot_t = m_inputs.m_W_dot_net_design / ((m_outputs.m_w_mc * (1.0 - m_inputs.m_recomp_frac) +
            m_outputs.m_w_rc * m_inputs.m_recomp_frac + m_outputs.m_w_t) * m_inputs.m_eta_generator);		//[C_sco2_cycle_core::kg/s]

        m_outputs.m_m_dot_rc = m_outputs.m_m_dot_t * m_inputs.m_recomp_frac;		//[C_sco2_cycle_core::kg/s]
        m_outputs.m_m_dot_mc = m_outputs.m_m_dot_t - m_outputs.m_m_dot_rc;
    }

    // Solve LTR
    *diff_T_LTR_LP_out = std::numeric_limits<double>::quiet_NaN();
    double T_LTR_LP_out_calc = std::numeric_limits<double>::quiet_NaN();
    {
        m_outputs.mc_LT_recup.design_for_target__calc_outlet(m_inputs.m_LTR_target_code,
            m_inputs.m_LTR_UA, m_inputs.m_LTR_min_dT, m_inputs.m_LTR_eff_target,
            m_inputs.m_LTR_eff_max,
            m_outputs.m_temp[C_sco2_cycle_core::MC_OUT], m_outputs.m_pres[C_sco2_cycle_core::MC_OUT], m_outputs.m_m_dot_mc, m_outputs.m_pres[C_sco2_cycle_core::LTR_HP_OUT],
            m_outputs.m_temp[C_sco2_cycle_core::HTR_LP_OUT], m_outputs.m_pres[C_sco2_cycle_core::HTR_LP_OUT], m_outputs.m_m_dot_t, m_outputs.m_pres[C_sco2_cycle_core::LTR_LP_OUT],
            m_inputs.m_des_tol,
            m_outputs.m_Q_dot_LT, m_outputs.m_temp[C_sco2_cycle_core::LTR_HP_OUT], T_LTR_LP_out_calc);

    }

    *diff_T_LTR_LP_out = T_LTR_LP_out_calc - T_LTR_LP_OUT_guess;

    return 0;
}

void C_sco2_htrbp_core::reset()
{
    this->m_inputs = S_sco2_htrbp_in();
    this->m_outputs.Init();
}

// ********************************************************************************** END C_sco2_htrbp_core


// ********************************************************************************** PRIVATE C_HTRBypass_Cycle (: C_sco2_cycle_core) 

/// <summary>
/// Core function to optimize cycle (fixed total UA)
/// </summary>
void C_HTRBypass_Cycle::auto_opt_design_core(int& error_code)
{
    // Reset optimal htrbp model
    m_optimal_htrbp_core.reset();

    // Check that simple/recomp flag is set
    if (ms_auto_opt_des_par.m_is_recomp_ok < -1.0 || (ms_auto_opt_des_par.m_is_recomp_ok > 0 &&
        ms_auto_opt_des_par.m_is_recomp_ok != 1.0 && ms_auto_opt_des_par.m_is_recomp_ok != 2.0))
    {
        throw(C_csp_exception("C_RecompCycle::auto_opt_design_core(...) requires that ms_auto_opt_des_par.m_is_recomp_ok"
            " is either between -1 and 0 (fixed recompression fraction) or equal to 1 (recomp allowed)\n"));
    }

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
        if (ms_auto_opt_des_par.m_is_recomp_ok <= 0.0)
        {   // fixed
            opt_par.m_recomp_frac_guess = std::abs(ms_auto_opt_des_par.m_is_recomp_ok);
            opt_par.m_fixed_recomp_frac = true;
        }
        else
        {   // optimized
            opt_par.m_recomp_frac_guess = 0.3;
            opt_par.m_fixed_recomp_frac = false;
        }

        // Is bypass fraction fixed or optimized?
        if (ms_auto_opt_des_par.m_is_bypass_ok <= 0.0)
        {   // fixed
            opt_par.m_bypass_frac_guess = std::abs(ms_auto_opt_des_par.m_is_bypass_ok);
            opt_par.m_fixed_bypass_frac = true;
        }
        else
        {   // optimized
            opt_par.m_bypass_frac_guess = 0.3;
            opt_par.m_fixed_bypass_frac = false;
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
    
    // Find optimal inputs
    C_sco2_htrbp_core::S_sco2_htrbp_in optimal_inputs_out;
    error_code = optimize_bp(ms_auto_opt_des_par, opt_par, optimal_inputs_out);

    if (error_code != 0)
        return;

    // Run Optimal Case
    m_optimal_htrbp_core.set_inputs(optimal_inputs_out);
    error_code = m_optimal_htrbp_core.solve();

    if (error_code != 0)
        return;

    // don't size system if eta is terrible
    //if (m_optimal_htrbp_core.m_outputs.m_eta_thermal < 0.15)
    //{
    //    error_code = (int)C_sco2_cycle_core::E_cycle_error_msg::E_ETA_THRESHOLD;
    //    return;
    //}

    // Check if cycle is below eta cutoff value
    if (m_optimal_htrbp_core.m_outputs.m_eta_thermal < ms_auto_opt_des_par.m_eta_thermal_cutoff)
    {
        ms_des_solved.m_eta_thermal = m_optimal_htrbp_core.m_outputs.m_eta_thermal;
        error_code = C_sco2_cycle_core::E_cycle_error_msg::E_ETA_THRESHOLD;
        return;
    }

    // Finalize Design (pass in reference to solved parameters)
    error_code = m_optimal_htrbp_core.finalize_design(ms_des_solved);
}

/// <summary>
/// Core function to optimize cycle for target eta (variable total UA)
/// </summary>
void C_HTRBypass_Cycle::auto_opt_design_hit_eta_core(int& error_code, const double eta_thermal_target)
{
    // Create 'ms_opt_des_par' for Design Variables
    S_opt_design_parameters opt_par;
    {
        // Target Thermal Efficiency
        opt_par.m_eta_thermal_target = eta_thermal_target;

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
        if (ms_auto_opt_des_par.m_is_recomp_ok <= 0.0)
        {   // fixed
            opt_par.m_recomp_frac_guess = std::abs(ms_auto_opt_des_par.m_is_recomp_ok);
            opt_par.m_fixed_recomp_frac = true;
        }
        else
        {   // optimized
            opt_par.m_recomp_frac_guess = 0.3;
            opt_par.m_fixed_recomp_frac = false;
        }

        // Is bypass fraction fixed or optimized?
        if (ms_auto_opt_des_par.m_is_bypass_ok <= 0.0)
        {   // fixed
            opt_par.m_bypass_frac_guess = std::abs(ms_auto_opt_des_par.m_is_bypass_ok);
            opt_par.m_fixed_bypass_frac = true;
        }
        else
        {   // optimized
            opt_par.m_bypass_frac_guess = 0.3;
            opt_par.m_fixed_bypass_frac = false;
        }

        // LTR HTR UA Ratio (cannot be fixed because design method is varying total UA)
        opt_par.m_LT_frac_guess = 0.5;
        opt_par.m_fixed_LT_frac = false;

    }

    // Set design method (it is 1, because total UA is optimized for target eta)
    opt_par.m_design_method = 1;

    // Select min and max values
    opt_par.m_UA_recup_total_max = ms_des_limits.m_UA_net_power_ratio_max * m_W_dot_net;		//[kW/K]
    opt_par.m_UA_recup_total_min = ms_des_limits.m_UA_net_power_ratio_min * m_W_dot_net;		//[kW/K]

    S_auto_opt_design_parameters auto_par = ms_auto_opt_des_par;

    // Optimize Total UA
    C_sco2_htrbp_core::S_sco2_htrbp_in optimal_inputs;
    error_code = optimize_totalUA(auto_par, opt_par, optimal_inputs);
    if (error_code != 0)
        return;

    
    // Run Optimal Core model and finalize design
    m_optimal_htrbp_core.reset();
    m_optimal_htrbp_core.set_inputs(optimal_inputs);
    error_code = m_optimal_htrbp_core.solve();
    if (error_code != 0)
        return;

    // Finalize Design (pass in reference to solved parameters)
    error_code = m_optimal_htrbp_core.finalize_design(ms_des_solved);

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
int C_HTRBypass_Cycle::optimize_totalUA(const S_auto_opt_design_parameters& auto_par,
    const S_opt_design_parameters& opt_par,
    C_sco2_htrbp_core::S_sco2_htrbp_in& optimal_inputs)
{
    // Validate Inputs
    if (std::isnan(opt_par.m_eta_thermal_target) ||
        std::isnan(opt_par.m_UA_recup_total_min) ||
        std::isnan(opt_par.m_UA_recup_total_max))
    {
        std::string warning_msg = "The target eta, and max and min UA need to be defined in S_opt_design_parameters";
        return -1;
    }
    

    // Create Optimizer
    nlopt::opt opt_des_cycle(nlopt::GN_DIRECT, 1);
    int error_code = -1;

    std::vector<double> lb = { opt_par.m_UA_recup_total_min };
    std::vector<double> ub = { opt_par.m_UA_recup_total_max };
    double UA_recup_guess = (opt_par.m_UA_recup_total_max + opt_par.m_UA_recup_total_min) / 2.0;    // [kW/K]

    opt_des_cycle.set_lower_bounds(lb);
    opt_des_cycle.set_upper_bounds(ub);
    opt_des_cycle.set_initial_step(1000);
    //opt_des_cycle.set_xtol_rel(ms_auto_opt_des_par.m_des_opt_tol);
    opt_des_cycle.set_xtol_rel(1);
    //opt_des_cycle.set_maxeval(50);

    // Make Tuple to pass in parameters
    std::tuple<C_HTRBypass_Cycle*, const S_auto_opt_design_parameters*, const S_opt_design_parameters*> par_tuple = { this, &auto_par, &opt_par };

    // Set max objective function
    std::vector<double> x;
    x.push_back(UA_recup_guess);
    opt_des_cycle.set_max_objective(nlopt_optimize_totalUA_func, &par_tuple);		// Calls wrapper/callback that calls 'design_point_eta', which optimizes design point eta through repeated calls to 'design'
    double max_f = std::numeric_limits<double>::quiet_NaN();

    nlopt::result result_des_cycle = opt_des_cycle.optimize(x, max_f);

    /// Check to make sure optimizer worked...
    if (opt_des_cycle.get_force_stop())
    {
        int w = 0;
    }

    // Assign Total UA
    double total_UA = x[0];
    ms_auto_opt_des_par.m_UA_rec_total = total_UA;

    // Have Total UA, now rerun optimizer to get other variables (redundant but shouldn't be very costly)
    C_sco2_htrbp_core::S_sco2_htrbp_in optimal_inputs_case;
    error_code = optimize_bp(ms_auto_opt_des_par, opt_par, optimal_inputs_case);

    if (error_code != 0)
        return error_code;

    // Assign Optimal Inputs
    optimal_inputs = optimal_inputs_case;

    return error_code;
}

/// <summary>
/// Optimize Bypass Fraction
/// totalUA -> bp -> UA split, pressure, recomp
/// </summary>
/// <param name="auto_par"></param>
/// <param name="opt_par"></param>
/// <param name="optimal_inputs"></param>
/// <returns></returns>
int C_HTRBypass_Cycle::optimize_bp(const S_auto_opt_design_parameters& auto_par,
    const S_opt_design_parameters& opt_par,
    C_sco2_htrbp_core::S_sco2_htrbp_in& optimal_inputs)
{
    // Set up baseline core inputs
    C_sco2_htrbp_core::S_sco2_htrbp_in core_inputs;
    {

        // From Auto Opt Design Parameters
        core_inputs.m_LTR_target_code = auto_par.m_LTR_target_code;
        core_inputs.m_LTR_UA = auto_par.m_LTR_UA;
        core_inputs.m_LTR_min_dT = auto_par.m_LTR_min_dT;
        core_inputs.m_LTR_eff_target = auto_par.m_LTR_eff_target;
        core_inputs.m_LTR_eff_max = auto_par.m_LTR_eff_max;
        
        core_inputs.m_LTR_od_UA_target_type = auto_par.m_LTR_od_UA_target_type;
        core_inputs.m_HTR_target_code = auto_par.m_HTR_target_code;
        core_inputs.m_HTR_UA = auto_par.m_HTR_UA;
        core_inputs.m_HTR_min_dT = auto_par.m_HTR_min_dT;
        core_inputs.m_HTR_eff_target = auto_par.m_HTR_eff_target;
        core_inputs.m_HTR_eff_max = auto_par.m_HTR_eff_max;
        core_inputs.m_HTR_od_UA_target_type = auto_par.m_HTR_od_UA_target_type;
        core_inputs.m_des_tol = auto_par.m_des_tol;
        core_inputs.m_is_des_air_cooler = auto_par.m_is_des_air_cooler;

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
        core_inputs.m_eta_rc = m_eta_rc;                            // Comes from constructor (constant)
        core_inputs.m_eta_generator = m_eta_generator;              // Comes from constructor (constant)
        core_inputs.m_frac_fan_power = m_frac_fan_power;            // Comes from constructor (constant)
        core_inputs.m_eta_fan = m_eta_fan;                          // Comes from constructor (constant)
        core_inputs.m_deltaP_cooler_frac = m_deltaP_cooler_frac;    // Comes from constructor (constant)
        core_inputs.m_T_amb_des = m_T_amb_des;                      // Comes from constructor (constant)
        core_inputs.m_elevation = m_elevation;                      // Comes from constructor (constant)
        core_inputs.m_N_nodes_pass = m_N_nodes_pass;                // Comes from constructor (constant)
        core_inputs.m_mc_comp_model_code = m_mc_comp_model_code;    // Comes from constructor (constant)
        core_inputs.m_N_turbine = m_N_turbine;                      // Comes from constructor (constant)

        core_inputs.m_rc_comp_model_code = C_comp__psi_eta_vs_phi::E_snl_radial_via_Dyreby; // Constant
        core_inputs.m_yr_inflation = m_yr_inflation;                 // Comes from constructor (constant)

        // From special bypass fraction function (should remove)
        core_inputs.m_dT_BP = m_dT_BP;                              // Comes from bp par function (constant)
        core_inputs.m_set_HTF_mdot = m_set_HTF_mdot;                // Comes from bp par function (constant)
        core_inputs.m_cp_HTF = m_cp_HTF;                            // Comes from bp par function (constant)
        core_inputs.m_T_HTF_PHX_inlet = m_T_HTF_PHX_inlet;          // Comes from bp par function (constant)
        core_inputs.m_HTF_PHX_cold_approach_input = m_HTF_PHX_cold_approach;    // Comes from bp par function (constant)


        // Handle design variables (check if fixed or free)
        {
            // Bypass Fraction
            if (opt_par.m_fixed_bypass_frac == true)
                core_inputs.m_bypass_frac = opt_par.m_bypass_frac_guess;

            // Recompression Fraction
            if (opt_par.m_fixed_recomp_frac == true)
                core_inputs.m_recomp_frac = opt_par.m_recomp_frac_guess;

            // MC Outlet Pressure
            if (opt_par.m_fixed_P_mc_out == true)
                core_inputs.m_P_mc_out = opt_par.m_P_mc_out_guess;

            // Recuperator split fraction
            double LT_frac_local = opt_par.m_LT_frac_guess;
            if (auto_par.m_LTR_target_code == NS_HX_counterflow_eqs::OPTIMIZE_UA || auto_par.m_HTR_target_code == NS_HX_counterflow_eqs::OPTIMIZE_UA)
            {
                core_inputs.m_LTR_UA = auto_par.m_UA_rec_total * LT_frac_local;
                core_inputs.m_HTR_UA = auto_par.m_UA_rec_total * (1.0 - LT_frac_local);
            }
            else
            {
                core_inputs.m_LTR_UA = auto_par.m_LTR_UA;      //[kW/K]
                core_inputs.m_HTR_UA = auto_par.m_HTR_UA;      //[kW/K]
            }


            // Pressure Ratio is calculated in callback
        }


        //core_inputs.m_P_mc_in = ms_des_par.m_P_mc_in;
        //core_inputs.m_P_mc_out = ms_des_par.m_P_mc_out;
        //core_inputs.m_recomp_frac = ms_des_par.m_recomp_frac;
        //core_inputs.m_bypass_frac = ms_opt_des_par.m_bypass_frac_guess;



        //opt_nonbp_par(auto_par, opt_par, core_inputs);
    }

    // Declare Optimal Inputs Case
    C_sco2_htrbp_core::S_sco2_htrbp_in optimal_inputs_final;

    // Bypass is Variable, optimize other variables WITHIN bp optimizer
    if (opt_par.m_fixed_bypass_frac == false)
    {
        // Reset Metrics
        m_opt_obj_internal_only = -1000000000000000;
        m_optimal_inputs_internal_only = C_sco2_htrbp_core::S_sco2_htrbp_in();

        // Create Optimizer
        nlopt::opt opt_des_cycle(nlopt::GN_DIRECT, 1);

        std::vector<double> lb = { 0 };
        std::vector<double> ub = { 0.99 };

        opt_des_cycle.set_lower_bounds(lb);
        opt_des_cycle.set_upper_bounds(ub);
        opt_des_cycle.set_initial_step(0.1);
        //opt_des_cycle.set_xtol_rel(auto_par.m_des_opt_tol);
        opt_des_cycle.set_xtol_rel(0.1);
        //opt_des_cycle.set_maxeval(50);

        // Set up Core Model that will be passed to objective function
        C_sco2_htrbp_core htrbp_core;
        htrbp_core.set_inputs(core_inputs);

        // Make Tuple to pass in parameters
        std::tuple<C_HTRBypass_Cycle*, const S_auto_opt_design_parameters*, const S_opt_design_parameters*, C_sco2_htrbp_core*> par_tuple = { this, &auto_par, &opt_par, &htrbp_core };

        // Set max objective function
        std::vector<double> x;
        x.push_back(0.1);
        opt_des_cycle.set_max_objective(nlopt_optimize_bp_func, &par_tuple);		// Calls wrapper/callback that calls 'design_point_eta', which optimizes design point eta through repeated calls to 'design'
        double max_f = std::numeric_limits<double>::quiet_NaN();

        nlopt::result result_des_cycle = opt_des_cycle.optimize(x, max_f);

        /// Check to make sure optimizer worked...
        if (opt_des_cycle.get_force_stop())
        {
            int w = 0;
        }

        // Return an optimal input case
        optimal_inputs_final = m_optimal_inputs_internal_only;

        // Clear Optimal Input Field
        m_optimal_inputs_internal_only = C_sco2_htrbp_core::S_sco2_htrbp_in();
        
    }

    // Bypass is Fixed, optimize other variables
    else
    {
        C_sco2_htrbp_core::S_sco2_htrbp_in optimal_inputs_final_temporary;
        int error_code = optimize_nonbp(auto_par, opt_par, core_inputs, optimal_inputs_final_temporary);
        if (error_code != 0)
            return error_code;

        // Return optimal input case
        optimal_inputs_final = optimal_inputs_final_temporary;
    }

    optimal_inputs = optimal_inputs_final;

    return 0;
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
int C_HTRBypass_Cycle::optimize_nonbp(const S_auto_opt_design_parameters& auto_par,
    const S_opt_design_parameters& opt_par,
    C_sco2_htrbp_core::S_sco2_htrbp_in& core_inputs,
    C_sco2_htrbp_core::S_sco2_htrbp_in& optimal_inputs)
{
    // Add Applicable Design Variables to Optimizer
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

    if (!opt_par.m_fixed_recomp_frac)
    {
        x.push_back(opt_par.m_recomp_frac_guess);
        lb.push_back(0.0);
        ub.push_back(1.0);
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
    C_sco2_htrbp_core::S_sco2_htrbp_in optimal_inputs_internal;
    if (index > 0)
    {
        // Set up instance of nlopt class and set optimization parameters
        nlopt::opt opt_des_cycle(nlopt::LN_SBPLX, index);
        opt_des_cycle.set_lower_bounds(lb);
        opt_des_cycle.set_upper_bounds(ub);
        opt_des_cycle.set_initial_step(scale);
        opt_des_cycle.set_xtol_rel(auto_par.m_des_opt_tol);
        //opt_des_cycle.set_maxeval(50);

        // Set up Core Model that will be passed to objective function
        C_sco2_htrbp_core htrbp_core;
        htrbp_core.set_inputs(core_inputs);

        // Make Tuple to pass in parameters
        std::tuple<C_HTRBypass_Cycle*, const S_auto_opt_design_parameters*, const S_opt_design_parameters*, C_sco2_htrbp_core*> par_tuple = { this, &auto_par, &opt_par, &htrbp_core };

        // Set max objective function
        
        opt_des_cycle.set_max_objective(nlopt_optimize_nonbp_func, &par_tuple);
        double max_f = std::numeric_limits<double>::quiet_NaN();

        nlopt::result result_des_cycle = opt_des_cycle.optimize(x, max_f);

        // This returns optimal values ^, need to convert to core_inputs

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
    else
    {
        // Define P_mc_in (because the ratio and mc_out are constant)
        core_inputs.m_P_mc_in = core_inputs.m_P_mc_out / opt_par.m_PR_HP_to_LP_guess;

        // Simulate Case (don't actually need to run...)
        C_sco2_htrbp_core core_model;
        core_model.set_inputs(core_inputs);
        error_code = core_model.solve();
    }

    // Set Optimal Inputs
    optimal_inputs = core_inputs;

    return error_code;
}

/// <summary>
/// Take optimizer array 'x', write appropriate values to S_sco2_htrbp_in
/// </summary>
int C_HTRBypass_Cycle::x_to_inputs(const std::vector<double>& x,
    const S_auto_opt_design_parameters auto_par,
    const S_opt_design_parameters opt_par,
    C_sco2_htrbp_core::S_sco2_htrbp_in &core_inputs)
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

    // Recompression fraction
    if (!opt_par.m_fixed_recomp_frac)
    {
        core_inputs.m_recomp_frac = x[index];
        if (core_inputs.m_recomp_frac < 0.0)
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
int C_HTRBypass_Cycle::clear_x_inputs(const std::vector<double>& x,
    const S_auto_opt_design_parameters auto_par,
    const S_opt_design_parameters opt_par,
    C_sco2_htrbp_core::S_sco2_htrbp_in& core_inputs)
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

    // Recompression fraction
    if (!opt_par.m_fixed_recomp_frac)
    {
        core_inputs.m_recomp_frac = std::numeric_limits<double>::quiet_NaN();
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
/// Calculate Temperature penalty, given target and calculated temperature
/// </summary>
double C_HTRBypass_Cycle::calc_T_penalty(double target, double calc, double span)
{
    double percent_error = std::abs(target - calc) / span;
    //double penalty = 10.0 * (100.0 * sigmoid(percent_error) - 0.5);

    double penalty = percent_error;

    return penalty;
}

/// <summary>
/// Calculate Objective Value (does not consider total UA minimization)
/// </summary>
double C_HTRBypass_Cycle::calc_objective(const S_auto_opt_design_parameters& auto_par,
    const S_opt_design_parameters& opt_par,
    const C_sco2_htrbp_core& htrbp_core)
{
    double obj = 0;
    double eta = htrbp_core.m_outputs.m_eta_thermal;

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

    // Penalize for HTF outlet temp (if necessary)
    double penalty = 0;
    if (opt_par.m_fixed_bypass_frac == false || auto_par.m_des_objective_type == 2)
    {
        double temp_calc = 0;
        double span = 0;

        if (m_T_target_is_HTF == 0)
        {
            temp_calc = htrbp_core.m_outputs.m_temp[MIXER_OUT];
            span = htrbp_core.m_outputs.m_temp[TURB_IN] - m_T_target;
        }
        else
        {
            temp_calc = htrbp_core.m_outputs.m_T_HTF_BP_outlet;
            span = htrbp_core.m_inputs.m_T_HTF_PHX_inlet - m_T_target;
        }

        penalty = calc_T_penalty(m_T_target, temp_calc, span);
    }

    obj = obj - penalty;

    return obj;
}

// ********************************************************************************** PUBLIC Methods C_HTRBypass_Cycle (: C_sco2_cycle_core) 

/// <summary>
/// Optimize Cycle Design for FIXED total recuperator UA
/// </summary>
/// <returns></returns>
int C_HTRBypass_Cycle::auto_opt_design(S_auto_opt_design_parameters& auto_opt_des_par_in)
{
    if (m_is_bp_par_set == false)
    {
        std::string warning_msg = "BP parameters are not defined";
        return -1;
    }

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
int C_HTRBypass_Cycle::auto_opt_design_hit_eta(S_auto_opt_design_hit_eta_parameters& auto_opt_des_hit_eta_in, std::string& error_msg)
{
    int error_code = -1;

    if (m_is_bp_par_set == false)
    {
        error_msg = "BP parameters are not defined";
        return -1;
    }

    // Reset Counter
    m_opt_iteration_count = 0;

    // Fill in 'ms_auto_opt_des_par' from input
    {
        ms_auto_opt_des_par.m_UA_rec_total = std::numeric_limits<double>::quiet_NaN();		// ***** This method finds the UA required to hit the input efficiency! *****
        // LTR thermal design
        ms_auto_opt_des_par.m_LTR_target_code = auto_opt_des_hit_eta_in.m_LTR_target_code;  //[-]
        ms_auto_opt_des_par.m_LTR_UA = auto_opt_des_hit_eta_in.m_LTR_UA;                    //[kW/K]
        ms_auto_opt_des_par.m_LTR_min_dT = auto_opt_des_hit_eta_in.m_LTR_min_dT;            //[K]
        ms_auto_opt_des_par.m_LTR_eff_target = auto_opt_des_hit_eta_in.m_LTR_eff_target;    //[-]
        ms_auto_opt_des_par.m_LTR_eff_max = auto_opt_des_hit_eta_in.m_LTR_eff_max;
        ms_auto_opt_des_par.m_LTR_od_UA_target_type = auto_opt_des_hit_eta_in.m_LTR_od_UA_target_type;
        // HTR thermal design
        ms_auto_opt_des_par.m_HTR_target_code = auto_opt_des_hit_eta_in.m_HTR_target_code;  //[-]
        ms_auto_opt_des_par.m_HTR_UA = auto_opt_des_hit_eta_in.m_HTR_UA;                    //[kW/K]
        ms_auto_opt_des_par.m_HTR_min_dT = auto_opt_des_hit_eta_in.m_HTR_min_dT;            //[K]
        ms_auto_opt_des_par.m_HTR_eff_target = auto_opt_des_hit_eta_in.m_HTR_eff_target;    //[-]
        ms_auto_opt_des_par.m_HTR_eff_max = auto_opt_des_hit_eta_in.m_HTR_eff_max;    //[-]
        ms_auto_opt_des_par.m_HTR_od_UA_target_type = auto_opt_des_hit_eta_in.m_HTR_od_UA_target_type;
        //
        ms_auto_opt_des_par.m_des_tol = auto_opt_des_hit_eta_in.m_des_tol;					//[-] Convergence tolerance
        ms_auto_opt_des_par.m_des_opt_tol = auto_opt_des_hit_eta_in.m_des_opt_tol;			//[-] Optimization tolerance
        ms_auto_opt_des_par.m_is_recomp_ok = auto_opt_des_hit_eta_in.m_is_recomp_ok;		//[-] 1 = yes, 0 = no, other = invalid
        ms_auto_opt_des_par.m_is_bypass_ok = auto_opt_des_hit_eta_in.m_is_bypass_ok;        //[-] 1 = yes, 0 = no, other = invalid

        ms_auto_opt_des_par.m_is_des_air_cooler = auto_opt_des_hit_eta_in.m_is_des_air_cooler;	//[-]

        ms_auto_opt_des_par.m_des_objective_type = auto_opt_des_hit_eta_in.m_des_objective_type;	//[-]
        ms_auto_opt_des_par.m_min_phx_deltaT = auto_opt_des_hit_eta_in.m_min_phx_deltaT;			//[C]

        ms_auto_opt_des_par.mf_callback_log = auto_opt_des_hit_eta_in.mf_callback_log;
        ms_auto_opt_des_par.mp_mf_active = auto_opt_des_hit_eta_in.mp_mf_active;

        ms_auto_opt_des_par.m_fixed_P_mc_out = auto_opt_des_hit_eta_in.m_fixed_P_mc_out;	//[-]

        ms_auto_opt_des_par.m_PR_HP_to_LP_guess = auto_opt_des_hit_eta_in.m_PR_HP_to_LP_guess;			//[-] Initial guess for ratio of P_mc_out to P_mc_in
        ms_auto_opt_des_par.m_fixed_PR_HP_to_LP = auto_opt_des_hit_eta_in.m_fixed_PR_HP_to_LP;			//[-] if true, ratio of P_mc_out to P_mc_in is fixed at PR_mc_guess		
    }

    // Check that simple/recomp flag is set
    if (ms_auto_opt_des_par.m_is_recomp_ok < -1.0 || (ms_auto_opt_des_par.m_is_recomp_ok > 0 &&
        ms_auto_opt_des_par.m_is_recomp_ok != 1.0 && ms_auto_opt_des_par.m_is_recomp_ok != 2.0))
    {
        throw(C_csp_exception("C_RecompCycle::auto_opt_design_core(...) requires that ms_auto_opt_des_par.m_is_recomp_ok"
            " is either between -1 and 0 (fixed recompression fraction) or equal to 1 (recomp allowed)\n"));
    }

    // Validate Inputs
    error_msg = "";
    {
        // Check that simple/recomp flag is set
        if (ms_auto_opt_des_par.m_is_recomp_ok < -1.0 || (ms_auto_opt_des_par.m_is_recomp_ok > 0 &&
            ms_auto_opt_des_par.m_is_recomp_ok != 1.0 && ms_auto_opt_des_par.m_is_recomp_ok != 2.0))
        {
            throw(C_csp_exception("C_RecompCycle::auto_opt_design_core(...) requires that ms_auto_opt_des_par.m_is_recomp_ok"
                " is either between -1 and 0 (fixed recompression fraction) or equal to 1 (recomp allowed)\n"));
        }
        // Can't operate compressore in 2-phase region
        if (m_T_mc_in <= N_co2_props::T_crit)
        {
            error_msg.append(util::format("Only single phase cycle operation is allowed in this model."
                "The compressor inlet temperature (%lg [C]) must be great than the critical temperature: %lg [C]",
                m_T_mc_in - 273.15, ((N_co2_props::T_crit)-273.15)));

            return -1;
        }

        // "Reasonable" ceiling on compressor inlet temp
        double T_mc_in_max = 70.0 + 273.15;		//[K] Arbitrary value for max compressor inlet temperature
        if (m_T_mc_in > T_mc_in_max)
        {
            error_msg.append(util::format("The compressor inlet temperature input was %lg [C]. This value was reset internally to the max allowable inlet temperature: %lg [C]\n",
                m_T_mc_in - 273.15, T_mc_in_max - 273.15));

            m_T_mc_in = T_mc_in_max;
        }

        // "Reasonable" floor on turbine inlet temp
        double T_t_in_min = 300.0 + 273.15;		//[K] Arbitrary value for min turbine inlet temperature
        if (m_T_t_in < T_t_in_min)
        {
            error_msg.append(util::format("The turbine inlet temperature input was %lg [C]. This value was reset internally to the min allowable inlet temperature: %lg [C]\n",
                m_T_t_in - 273.15, T_t_in_min - 273.15));

            m_T_t_in = T_t_in_min;
        }

        // Turbine inlet temperature must be hotter than compressor outlet temperature
        if (m_T_t_in <= m_T_mc_in)
        {
            error_msg.append(util::format("The turbine inlet temperature, %lg [C], is colder than the specified compressor inlet temperature %lg [C]",
                m_T_t_in - 273.15, m_T_mc_in - 273.15));

            return -1;
        }

        // Turbine inlet temperature must be colder than property limits
        if (m_T_t_in >= N_co2_props::T_upper_limit)
        {
            error_msg.append(util::format("The turbine inlet temperature, %lg [C], is hotter than the maximum allow temperature in the CO2 property code %lg [C]",
                m_T_t_in - 273.15, N_co2_props::T_upper_limit - 273.15));

            return -1;
        }

        // REMOVED CHECKS (not compatible with negative values)
        // Check for realistic isentropic efficiencies
        /*if (m_eta_mc > 1.0)
        {
            error_msg.append(util::format("The main compressor isentropic efficiency, %lg, was reset to theoretical maximum 1.0\n",
                m_eta_mc));

            m_eta_mc = 1.0;
        }
        if (m_eta_rc > 1.0)
        {
            error_msg.append(util::format("The re-compressor isentropic efficiency, %lg, was reset to theoretical maximum 1.0\n",
                m_eta_rc));

            m_eta_rc = 1.0;
        }
        if (m_eta_t > 1.0)
        {
            error_msg.append(util::format("The turbine isentropic efficiency, %lg, was reset to theoretical maximum 1.0\n",
                m_eta_t));

            m_eta_t = 1.0;
        }
        if (m_eta_mc < 0.1)
        {
            error_msg.append(util::format("The main compressor isentropic efficiency, %lg, was increased to the internal limit of 0.1 to improve solution stability\n",
                m_eta_mc));

            m_eta_mc = 0.1;
        }
        if (m_eta_rc < 0.1)
        {
            error_msg.append(util::format("The re-compressor isentropic efficiency, %lg, was increased to the internal limit of 0.1 to improve solution stability\n",
                m_eta_rc));

            m_eta_rc = 0.1;
        }
        if (m_eta_t < 0.1)
        {
            error_msg.append(util::format("The turbine isentropic efficiency, %lg, was increased to the internal limit of 0.1 to improve solution stability\n",
                m_eta_t));

            m_eta_t = 0.1;
        }*/

        if (ms_auto_opt_des_par.m_LTR_eff_max > 1.0)
        {
            error_msg.append(util::format("The LT recuperator max effectiveness, %lg, was decreased to the limit of 1.0\n", ms_auto_opt_des_par.m_LTR_eff_max));

            ms_auto_opt_des_par.m_LTR_eff_max = 1.0;
        }

        if (ms_auto_opt_des_par.m_LTR_eff_max < 0.70)
        {
            error_msg.append(util::format("The LT recuperator max effectiveness, %lg, was increased to the internal limit of 0.70 improve convergence\n", ms_auto_opt_des_par.m_LTR_eff_max));

            ms_auto_opt_des_par.m_LTR_eff_max = 0.7;
        }

        if (ms_auto_opt_des_par.m_HTR_eff_max > 1.0)
        {
            error_msg.append(util::format("The HT recuperator max effectiveness, %lg, was decreased to the limit of 1.0\n", ms_auto_opt_des_par.m_HTR_eff_max));

            ms_auto_opt_des_par.m_HTR_eff_max = 1.0;
        }

        if (ms_auto_opt_des_par.m_HTR_eff_max < 0.70)
        {
            error_msg.append(util::format("The LT recuperator max effectiveness, %lg, was increased to the internal limit of 0.70 improve convergence\n", ms_auto_opt_des_par.m_HTR_eff_max));

            ms_auto_opt_des_par.m_HTR_eff_max = 0.7;
        }

        // Limits on high pressure limit
        if (m_P_high_limit >= N_co2_props::P_upper_limit)
        {
            error_msg.append(util::format("The upper pressure limit, %lg [MPa], was set to the internal limit in the CO2 properties code %lg [MPa]\n",
                m_P_high_limit, N_co2_props::P_upper_limit));

            m_P_high_limit = N_co2_props::P_upper_limit;
        }
        double P_high_limit_min = 10.0 * 1.E3;	//[kPa]
        if (m_P_high_limit <= P_high_limit_min)
        {
            error_msg.append(util::format("The upper pressure limit, %lg [MPa], must be greater than %lg [MPa] to ensure solution stability",
                m_P_high_limit, P_high_limit_min));

            return -1;
        }

        // Finally, check thermal efficiency
        if (auto_opt_des_hit_eta_in.m_eta_thermal <= 0.0)
        {
            error_msg.append(util::format("The design cycle thermal efficiency, %lg, must be at least greater than 0 ",
                auto_opt_des_hit_eta_in.m_eta_thermal));

            return -1;
        }
        double eta_carnot = 1.0 - m_T_mc_in / m_T_t_in;
        if (auto_opt_des_hit_eta_in.m_eta_thermal >= eta_carnot)
        {
            error_msg.append(util::format("To solve the cycle within the allowable recuperator conductance, the design cycle thermal efficiency, %lg, must be at least less than the Carnot efficiency: %lg ",
                auto_opt_des_hit_eta_in.m_eta_thermal, eta_carnot));

            return -1;
        }

        
    }

    // Send log update upstream
    if (ms_auto_opt_des_par.mf_callback_log && ms_auto_opt_des_par.mp_mf_active)
    {
        std::string msg_log = util::format("Iterate on total recuperator conductance to hit target cycle efficiency: %lg [-]", auto_opt_des_hit_eta_in.m_eta_thermal);
        std::string msg_progress = "Designing cycle...";
        if (!ms_auto_opt_des_par.mf_callback_log(msg_log, msg_progress, ms_auto_opt_des_par.mp_mf_active, 0.0, 2))
        {
            std::string error_msg = "User terminated simulation...";
            std::string loc_msg = "C_MEQ_sco2_design_hit_eta__UA_total";
            throw(C_csp_exception(error_msg, loc_msg, 1));
        }
    }

    // Optimize
    auto_opt_design_hit_eta_core(error_code, auto_opt_des_hit_eta_in.m_eta_thermal);

    // Send log update upstream
    if (ms_auto_opt_des_par.mf_callback_log && ms_auto_opt_des_par.mp_mf_active)
    {
        std::string msg_log = "Optimization finished in " + std::to_string(m_opt_iteration_count) + " iterations.";
        std::string msg_progress = "";
        if (!ms_auto_opt_des_par.mf_callback_log(msg_log, msg_progress, ms_auto_opt_des_par.mp_mf_active, 100.0, 2))
        {
            std::string error_msg = "User terminated simulation...";
            std::string loc_msg = "C_MEQ_sco2_design_hit_eta__UA_total";
            throw(C_csp_exception(error_msg, loc_msg, 1));
        }
    }

    return error_code;
}

// ********************************************************************************** PUBLIC Objective Functions (internal use only)

/// <summary>
/// Objective Function for total UA (outermost layer)
/// Total UA -> Bypass Fraction -> pressure, split UA, recomp frac
/// </summary>
/// <returns>ONLY Objective Value</returns>
double C_HTRBypass_Cycle::optimize_totalUA_return_objective_metric(const std::vector<double>& x,
    const S_auto_opt_design_parameters& auto_par,
    const S_opt_design_parameters& opt_par
)
{
    // Update counter
    m_opt_iteration_count++; // global

    // Copy optimal parameters
    S_auto_opt_design_parameters auto_par_internal = auto_par;
    S_opt_design_parameters opt_par_internal = opt_par;

    // Assign Total Recuperator UA
    double total_UA = x[0];
    auto_par_internal.m_UA_rec_total = total_UA;

    // Optimize all internal variables (bypass_frac -> recomp frac, UA ratio, pressure ratio)
    C_sco2_htrbp_core::S_sco2_htrbp_in optimal_inputs_case;
    int error_code = optimize_bp(auto_par_internal, opt_par_internal, optimal_inputs_case);

    if (error_code != 0)
        return -100000000000000000;

    // ^ Get optimal core_inputs from here

    // Run Optimal Case, return objective function
    C_sco2_htrbp_core htrbp_core;
    htrbp_core.set_inputs(optimal_inputs_case);
    error_code = htrbp_core.solve();
    if (error_code != 0)
        return -100000000000000000;

    // Calculate Objective Value
    double obj = calc_objective(auto_par_internal, opt_par_internal, htrbp_core);

    // This objectve ^ targets a thermal efficiency (and tries to hit a temperature, if applicable)
    // Need to incentivize lower UA value

    // Hitting eta and temperature is more important than low UA
    double UA_percent = (total_UA - opt_par.m_UA_recup_total_min) / (opt_par.m_UA_recup_total_max - opt_par.m_UA_recup_total_min);  // Lower is closer to 0
    double UA_penalty = UA_percent;

    // Increase obj scale (making eta and temp more weighted)
    double obj_weighted = obj * 1e2;

    // Subtract UA_penalty
    obj_weighted = obj_weighted - UA_penalty;

    return obj_weighted;
}

/// <summary>
/// Objective Function for BYPASS optimization (calls internal optimization for other variables)
/// Total UA -> Bypass Fraction -> pressure, split UA, recomp frac
/// </summary>
/// <returns></returns>
double C_HTRBypass_Cycle::optimize_bp_return_objective_metric(const std::vector<double>& x,
    const S_auto_opt_design_parameters& auto_par,
    const S_opt_design_parameters& opt_par,
    C_sco2_htrbp_core& htrbp_core)
{
    // Update counter
    m_opt_iteration_count++; // global

    if (opt_par.m_fixed_bypass_frac == true)
    {
        //throw std::exception("Optimizing bypass fraction even though it is fixed");
        throw(C_csp_exception("Optimizing bypass fraction even though it is fixed"));
    }

    // Set Bypass Fraction
    htrbp_core.m_inputs.m_bypass_frac = x[0];

    // Optimize all other variables
    C_sco2_htrbp_core::S_sco2_htrbp_in optimal_inputs_case;
    optimize_nonbp(auto_par, opt_par, htrbp_core.m_inputs, optimal_inputs_case);

    // ^ Get optimal core_inputs from here

    // Run Optimal Case, return objective function

    htrbp_core.set_inputs(optimal_inputs_case);
    int error_code = htrbp_core.solve();
    if (error_code != 0)
        return -100000000000000000;

    // Calculate Objective Value
    double obj = calc_objective(auto_par, opt_par, htrbp_core);

    if (obj > m_opt_obj_internal_only)
    {
        m_opt_obj_internal_only = obj;
        m_optimal_inputs_internal_only = optimal_inputs_case;
    }

    // Reset htrbp_core
    htrbp_core.m_inputs.m_bypass_frac = std::numeric_limits<double>::quiet_NaN();
    htrbp_core.m_outputs.Init();

    return obj;
}

/// <summary>
/// Objective Function for NON Bypass optimization
/// Total UA -> Bypass Fraction -> pressure, split UA, recomp frac
/// </summary>
/// <returns>ONLY Objective Value</returns>
double C_HTRBypass_Cycle::optimize_nonbp_return_objective_metric(const std::vector<double>& x,
    const S_auto_opt_design_parameters& auto_par,
    const S_opt_design_parameters& opt_par,
    C_sco2_htrbp_core& htrbp_core)
{
    // Update counter
    m_opt_iteration_count++; // global

    // Modify Core Inputs with Variable Parameters
    int error_code = x_to_inputs(x, auto_par, opt_par, htrbp_core.m_inputs);

    if (error_code != 0)
        return -1000000000;

    // AT this point, will have fully defined core input struct
    // Run the core model
    error_code = htrbp_core.solve();

    // Set Objective
    double objective_metric = -10000000000.0;
    if (error_code == 0)
    {
        objective_metric = calc_objective(auto_par, opt_par, htrbp_core);
    }

    // Clear Optimized Inputs
    clear_x_inputs(x, auto_par, opt_par, htrbp_core.m_inputs);

    return objective_metric;
}

// ********************************************************************************** Off Design Functions


void C_HTRBypass_Cycle::reset_ms_od_turbo_bal_csp_solved()
{
}

int C_HTRBypass_Cycle::off_design_fix_shaft_speeds(S_od_par& od_phi_par_in, double od_tol)
{
    return 0;
}

int C_HTRBypass_Cycle::solve_OD_all_coolers_fan_power(double T_amb, double od_tol, double& W_dot_fan)
{
    return 0;
}

int C_HTRBypass_Cycle::solve_OD_mc_cooler_fan_power(double T_amb, double od_tol, double& W_dot_mc_cooler_fan, double& P_co2_out)
{
    return 0;
}

int C_HTRBypass_Cycle::solve_OD_pc_cooler_fan_power(double T_amb, double od_tol, double& W_dot_pc_cooler_fan, double& P_co2_out)
{
    return 0;
}

double C_HTRBypass_Cycle::get_od_temp(int n_state_point)
{
    return 0.0;
}

double C_HTRBypass_Cycle::get_od_pres(int n_state_point)
{
    return 0.0;
}

void C_HTRBypass_Cycle::check_od_solution(double& diff_m_dot, double& diff_E_cycle, double& diff_Q_LTR, double& diff_Q_HTR)
{
}

void C_HTRBypass_Cycle::set_od_temp(int n_state_point, double temp_K)
{
}

void C_HTRBypass_Cycle::set_od_pres(int n_state_point, double pres_kPa)
{
}

void C_HTRBypass_Cycle::off_design_recompressor(double T_in, double P_in, double m_dot, double P_out, double tol, int& error_code, double& T_out)
{
}

void C_HTRBypass_Cycle::estimate_od_turbo_operation(double T_mc_in, double P_mc_in, double f_recomp, double T_t_in, double phi_mc, int& mc_error_code, double& mc_w_tip_ratio, double& P_mc_out, int& rc_error_code, double& rc_w_tip_ratio, double& rc_phi, bool is_update_ms_od_solved)
{
}


// ********************************************************************************** PUBLIC Methods defined outside of any class

double nlopt_optimize_totalUA_func(const std::vector<double>& x, std::vector<double>& grad, void* data)
{
    // Unpack Data Tuple
    std::tuple<C_HTRBypass_Cycle*, const C_HTRBypass_Cycle::S_auto_opt_design_parameters*, const C_HTRBypass_Cycle::S_opt_design_parameters*>* data_tuple
        = static_cast<std::tuple<C_HTRBypass_Cycle*, const C_HTRBypass_Cycle::S_auto_opt_design_parameters*, const C_HTRBypass_Cycle::S_opt_design_parameters*>*>(data);

    C_HTRBypass_Cycle* frame = std::get<0>(*data_tuple);
    const C_HTRBypass_Cycle::S_auto_opt_design_parameters* auto_opt_par = std::get<1>(*data_tuple);
    const C_HTRBypass_Cycle::S_opt_design_parameters* opt_par = std::get<2>(*data_tuple);

    if (frame != NULL)
        return frame->optimize_totalUA_return_objective_metric(x, *auto_opt_par, *opt_par);
    else
        return 0.0;
}

double nlopt_optimize_bp_func(const std::vector<double>& x, std::vector<double>& grad, void* data)
{
    // Unpack Data Tuple
    std::tuple<C_HTRBypass_Cycle*, const C_HTRBypass_Cycle::S_auto_opt_design_parameters*, const C_HTRBypass_Cycle::S_opt_design_parameters*, C_sco2_htrbp_core*>* data_tuple
        = static_cast<std::tuple<C_HTRBypass_Cycle*, const C_HTRBypass_Cycle::S_auto_opt_design_parameters*, const C_HTRBypass_Cycle::S_opt_design_parameters*, C_sco2_htrbp_core*>*>(data);

    C_HTRBypass_Cycle* frame = std::get<0>(*data_tuple);
    const C_HTRBypass_Cycle::S_auto_opt_design_parameters* auto_opt_par = std::get<1>(*data_tuple);
    const C_HTRBypass_Cycle::S_opt_design_parameters* opt_par = std::get<2>(*data_tuple);
    C_sco2_htrbp_core* htrbp_core = std::get<3>(*data_tuple);

    if (frame != NULL)
        return frame->optimize_bp_return_objective_metric(x, *auto_opt_par, *opt_par, *htrbp_core);
    else
        return 0.0;
}

double nlopt_optimize_nonbp_func(const std::vector<double>& x, std::vector<double>& grad, void* data)
{
    // Unpack Data Tuple
    std::tuple<C_HTRBypass_Cycle*, const C_HTRBypass_Cycle::S_auto_opt_design_parameters*, const C_HTRBypass_Cycle::S_opt_design_parameters*, C_sco2_htrbp_core*>* data_tuple
        = static_cast<std::tuple<C_HTRBypass_Cycle*, const C_HTRBypass_Cycle::S_auto_opt_design_parameters*, const C_HTRBypass_Cycle::S_opt_design_parameters*, C_sco2_htrbp_core*>*>(data);

    C_HTRBypass_Cycle* frame = std::get<0>(*data_tuple);
    const C_HTRBypass_Cycle::S_auto_opt_design_parameters* auto_opt_par = std::get<1>(*data_tuple);
    const C_HTRBypass_Cycle::S_opt_design_parameters* opt_par = std::get<2>(*data_tuple);
    C_sco2_htrbp_core* htrbp_core = std::get<3>(*data_tuple);

    if (frame != NULL)
        return frame->optimize_nonbp_return_objective_metric(x, *auto_opt_par, *opt_par, *htrbp_core);
    else
        return 0.0;
}

double sigmoid(const double val)
{
    return 1.0 / (1.0 + std::exp(-1.0 * val));
}

double logit(const double val)
{
    return std::log(val / (1.0 - val));
}
