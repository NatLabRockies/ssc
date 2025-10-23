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

#include "ud_power_cycle.h"
#include "csp_solver_util.h"

#include "sam_csp_util.h"
#include <algorithm>
#include <set>
#include <fstream>
#include <unordered_set>
#include <map>

C_ud_power_cycle::C_ud_power_cycle()
{
    m_E_udpc_max_output_correction_mode = C_ud_power_cycle::E_udpc_max_output_correction_mode::SIMPLE_HEURISTIC;
    m_is_sco2_design_set = false;

    m_T_htf_cold_des_sco2_regr = m_deltaT_HTF_des = std::numeric_limits<double>::quiet_NaN();
}

void C_ud_power_cycle::set_is_sco2_regr(C_ud_power_cycle::E_udpc_max_output_correction_mode udpc_max_output_correction_mode)
{
    m_E_udpc_max_output_correction_mode = udpc_max_output_correction_mode;
}

void C_ud_power_cycle::set_sco2_design_for_sco2_regr(double T_htf_hot_des /*C*/, double T_htf_cold_des /*C*/)
{
    m_T_htf_cold_des_sco2_regr = T_htf_cold_des;        //[C]
    m_deltaT_HTF_des = T_htf_hot_des - T_htf_cold_des;  //[C]

    if (std::isfinite(m_deltaT_HTF_des)) {
        m_is_sco2_design_set = true;
    }
}

void C_ud_power_cycle::init(C_ud_power_cycle::E_udpc_max_output_correction_mode udpc_max_output_correction_mode,
    const util::matrix_t<double>& udpc_table,
    int& n_T_htf_pars, int& n_T_amb_pars, int& n_m_dot_pars,
    double& T_htf_ref_calc /*C*/, double& T_htf_low_calc /*C*/, double& T_htf_high_calc /*C*/,
    double& T_amb_ref_calc /*C*/, double& T_amb_low_calc /*C*/, double& T_amb_high_calc /*C*/,
    double& m_dot_htf_ND_ref_calc, double& m_dot_htf_ND_low_calc /*-*/, double& m_dot_htf_ND_high_calc /*-*/,
    std::vector<double>& Y_at_T_htf_ref, std::vector<double>& Y_at_T_amb_ref,
    std::vector<double>& Y_at_m_dot_htf_ND_ref, std::vector<double>& Y_avg_at_refs)
{
    m_E_udpc_max_output_correction_mode = udpc_max_output_correction_mode;

    util::matrix_t<double> T_htf_ind_table, m_dot_htf_ND_ind_table, T_amb_ind_table;    

    N_udpc_common::split_ind_tbl(udpc_table,
        T_htf_ind_table, m_dot_htf_ND_ind_table, T_amb_ind_table,
        mv_T_htf_unique, mv_m_dot_unique,
        mv_T_amb_unique,
        n_T_htf_pars, n_T_amb_pars, n_m_dot_pars,
        m_dot_htf_ND_low_calc, m_dot_htf_ND_ref_calc, m_dot_htf_ND_high_calc,
        T_htf_low_calc, T_htf_ref_calc, T_htf_high_calc,
        T_amb_low_calc, T_amb_ref_calc, T_amb_high_calc);

    // Set up Linear Interp class
    int error_index = -2;
    int column_index_array[1] = { 0 };
    if (!mc_T_htf_ind.Set_1D_Lookup_Table(T_htf_ind_table, column_index_array, 1, error_index))
    {
        if (error_index == -1)
        {
            throw(C_csp_exception("Table representing Hot HTF Temperature parametric results must have"
                "at least 3 rows", "User defined power cycle initialization"));
        }
        else
        {
            throw(C_csp_exception("The Hot HTF Temperature must monotonically increase in the table",
                "User defined power cycle initialization"));
        }
    }

    if (!mc_T_amb_ind.Set_1D_Lookup_Table(T_amb_ind_table, column_index_array, 1, error_index))
    {
        if (error_index == -1)
        {
            throw(C_csp_exception("Table representing Ambient Temperature parametric results must have"
                "at least 3 rows", "User defined power cycle initialization"));
        }
        else
        {
            throw(C_csp_exception("The Ambient Temperature must monotonically increase in the table",
                "User defined power cycle initialization"));
        }
    }

    if (!mc_m_dot_htf_ind.Set_1D_Lookup_Table(m_dot_htf_ND_ind_table, column_index_array, 1, error_index))
    {
        if (error_index == -1)
        {
            throw(C_csp_exception("Table representing HTF mass flow rate parametric results must have"
                "at least 3 rows", "User defined power cycle initialization"));
        }
        else
        {
            throw(C_csp_exception("The HTF mass flow rate must monotonically increase in the table",
                "User defined power cycle initialization"));
        }
    }

    // Set member data for reference and upper and lower bounds of independent variables
    m_T_htf_ref = T_htf_ref_calc;
    m_T_htf_low = T_htf_low_calc;
    m_T_htf_high = T_htf_high_calc;

    m_T_amb_ref = T_amb_ref_calc;
    m_T_amb_low = T_amb_low_calc;
    m_T_amb_high = T_amb_high_calc;

    m_m_dot_htf_ref = m_dot_htf_ND_ref_calc;
    m_m_dot_htf_low = m_dot_htf_ND_low_calc;
    m_m_dot_htf_high = m_dot_htf_ND_high_calc;

    // Check that the reference (design) value and upper and lower levels for each independent variable are contained within the x-range of the corresponding table
        // T_HTF
    if (!mc_T_htf_ind.check_x_value_x_col_0(m_T_htf_ref))
    {
        m_error_msg = util::format("The user defined power cycle table containing parametric runs on the hot HTF temperature"
            " must contain the design HTF temperature %lg [C]. %s [C]", m_T_htf_ref, mc_T_htf_ind.get_error_msg().c_str());
        throw(C_csp_exception(m_error_msg, "User defined power cycle initialization"));
    }
    if (!mc_T_htf_ind.check_x_value_x_col_0(m_T_htf_low))
    {
        m_error_msg = util::format("The user defined power cycle table containing parametric runs on the hot HTF temperature"
            " must contain the lower level HTF temperature %lg [C]. %s [C]", m_T_htf_low, mc_T_htf_ind.get_error_msg().c_str());
        throw(C_csp_exception(m_error_msg, "User defined power cycle initialization"));
    }
    if (!mc_T_htf_ind.check_x_value_x_col_0(m_T_htf_high))
    {
        m_error_msg = util::format("The user defined power cycle table containing parametric runs on the hot HTF temperature"
            " must contain the upper level HTF temperature %lg [C]. %s [C]", m_T_htf_high, mc_T_htf_ind.get_error_msg().c_str());
        throw(C_csp_exception(m_error_msg, "User defined power cycle initialization"));
    }

    // T_amb
    if (!mc_T_amb_ind.check_x_value_x_col_0(m_T_amb_ref))
    {
        m_error_msg = util::format("The user defined power cycle table containing parametric runs on the ambient temperature"
            " must contain the design ambient temperature %lg [C]. %s [C]", m_T_amb_ref, mc_T_amb_ind.get_error_msg().c_str());
        throw(C_csp_exception(m_error_msg, "User defined power cycle initialization"));
    }
    if (!mc_T_amb_ind.check_x_value_x_col_0(m_T_amb_low))
    {
        m_error_msg = util::format("The user defined power cycle table containing parametric runs on the ambient temperature"
            " must contain the lower level ambient temperature %lg [C]. %s [C]", m_T_amb_low, mc_T_amb_ind.get_error_msg().c_str());
        throw(C_csp_exception(m_error_msg, "User defined power cycle initialization"));
    }
    if (!mc_T_amb_ind.check_x_value_x_col_0(m_T_amb_high))
    {
        m_error_msg = util::format("The user defined power cycle table containing parametric runs on the ambient temperature"
            " must contain the upper level ambient temperature %lg [C]. %s [C]", m_T_amb_high, mc_T_amb_ind.get_error_msg().c_str());
        throw(C_csp_exception(m_error_msg, "User defined power cycle initialization"));
    }

    // m_dot_HTF
    if (!mc_m_dot_htf_ind.check_x_value_x_col_0(m_m_dot_htf_ref))
    {
        m_error_msg = util::format("The user defined power cycle table containing parametric runs on the normalized HTF mass flow rate"
            " must contain the design normalized HTF mass flow rate %lg [-]. %s [-]", m_m_dot_htf_ref, mc_m_dot_htf_ind.get_error_msg().c_str());
        throw(C_csp_exception(m_error_msg, "User defined power cycle initialization"));
    }
    if (!mc_m_dot_htf_ind.check_x_value_x_col_0(m_m_dot_htf_low))
    {
        m_error_msg = util::format("The user defined power cycle table containing parametric runs on the normalized HTF mass flow rate"
            " must contain the lower level normalized HTF mass flow rate %lg [-]. %s [-]", m_m_dot_htf_low, mc_m_dot_htf_ind.get_error_msg().c_str());
        throw(C_csp_exception(m_error_msg, "User defined power cycle initialization"));
    }
    if (!mc_m_dot_htf_ind.check_x_value_x_col_0(m_m_dot_htf_high))
    {
        m_error_msg = util::format("The user defined power cycle table containing parametric runs on the normalized HTF mass flow rate"
            " must contain the upper level normalized HTF mass flow rate %lg [-]. %s [-]", m_m_dot_htf_high, mc_m_dot_htf_ind.get_error_msg().c_str());
        throw(C_csp_exception(m_error_msg, "User defined power cycle initialization"));
    }

    // ************************************************************************
    // ************************************************************************

    // Calculate main effects of each independent variable at its upper and lower levels
    Y_at_T_htf_ref.resize(4);
    Y_at_T_amb_ref.resize(4);
    Y_at_m_dot_htf_ND_ref.resize(4);
    m_Y_at_ref.resize(4);

    m_ME_T_htf_low.resize(4);
    m_ME_T_htf_high.resize(4);

    m_ME_T_amb_low.resize(4);
    m_ME_T_amb_high.resize(4);

    m_ME_m_dot_htf_low.resize(4);
    m_ME_m_dot_htf_high.resize(4);

    for (int i = 0; i < 4; i++)
    {
        int i_col = i * 3 + 2;

        Y_at_T_htf_ref[i] = mc_T_htf_ind.interpolate_x_col_0(i_col, m_T_htf_ref);
        Y_at_T_amb_ref[i] = mc_T_amb_ind.interpolate_x_col_0(i_col, m_T_amb_ref);
        Y_at_m_dot_htf_ND_ref[i] = mc_m_dot_htf_ind.interpolate_x_col_0(i_col, m_m_dot_htf_ref);
        m_Y_at_ref[i] = (Y_at_T_htf_ref[i] + Y_at_T_amb_ref[i] + Y_at_m_dot_htf_ND_ref[i]) / 3.0;

        m_ME_T_htf_low[i] = mc_T_htf_ind.interpolate_x_col_0(i_col, m_T_htf_low) - m_Y_at_ref[i];
        m_ME_T_htf_high[i] = mc_T_htf_ind.interpolate_x_col_0(i_col, m_T_htf_high) - m_Y_at_ref[i];

        m_ME_T_amb_low[i] = mc_T_amb_ind.interpolate_x_col_0(i_col, m_T_amb_low) - m_Y_at_ref[i];
        m_ME_T_amb_high[i] = mc_T_amb_ind.interpolate_x_col_0(i_col, m_T_amb_high) - m_Y_at_ref[i];

        m_ME_m_dot_htf_low[i] = mc_m_dot_htf_ind.interpolate_x_col_0(i_col, m_m_dot_htf_low) - m_Y_at_ref[i];
        m_ME_m_dot_htf_high[i] = mc_m_dot_htf_ind.interpolate_x_col_0(i_col, m_m_dot_htf_high) - m_Y_at_ref[i];
    }
    Y_avg_at_refs = m_Y_at_ref;

    // Set up 2D tables to store calculated Interactions	
    int n_T_htf_runs = mc_T_htf_ind.get_number_of_rows();
    int n_T_amb_runs = mc_T_amb_ind.get_number_of_rows();
    int n_m_dot_htf_runs = mc_m_dot_htf_ind.get_number_of_rows();

    // 2 interaction effects (upper and lower) for each output
    util::matrix_t<double> T_htf_int_on_T_amb(n_T_amb_runs, 9);
    util::matrix_t<double> T_amb_int_on_m_dot_htf(n_m_dot_htf_runs, 9);
    util::matrix_t<double> m_dot_htf_int_on_T_htf(n_T_htf_runs, 9);

    // Calculate interaction effects
    for (int i = 0; i < 4; i++)
    {
        // T_HTF interaction on ambient temperature
        for (int j = 0; j < n_T_amb_runs; j++)
        {
            if (i == 0)
            {
                T_htf_int_on_T_amb(j, 0) = mc_T_amb_ind.get_x_value_x_col_0(j);
            }
            // lower level interaction effect
            double aa = mc_T_amb_ind.Get_Value(i * 3 + 1, j);
            double bb = m_ME_T_htf_low[i];
            double cc = mc_T_amb_ind.Get_Value(i * 3 + 2, j);
            T_htf_int_on_T_amb(j, i * 2 + 1) = -(mc_T_amb_ind.Get_Value(i * 3 + 1, j) - m_Y_at_ref[i] - m_ME_T_htf_low[i] - (mc_T_amb_ind.Get_Value(i * 3 + 2, j) - m_Y_at_ref[i]));
            // upper level interaction
            aa = mc_T_amb_ind.Get_Value(i * 3 + 3, j);
            bb = m_ME_T_htf_high[i];
            cc = mc_T_amb_ind.Get_Value(i * 3 + 2, j);
            T_htf_int_on_T_amb(j, i * 2 + 2) = -(mc_T_amb_ind.Get_Value(i * 3 + 3, j) - m_Y_at_ref[i] - m_ME_T_htf_high[i] - (mc_T_amb_ind.Get_Value(i * 3 + 2, j) - m_Y_at_ref[i]));
        }

        // Ambient temperature interaction on HTF mass flow rate
        for (int j = 0; j < n_m_dot_htf_runs; j++)
        {
            if (i == 0)
            {
                T_amb_int_on_m_dot_htf(j, 0) = mc_m_dot_htf_ind.get_x_value_x_col_0(j);
            }
            // lower level interaction effect
            double aa = mc_m_dot_htf_ind.Get_Value(i * 3 + 1, j);
            double bb = m_ME_T_amb_low[i];
            double cc = mc_m_dot_htf_ind.Get_Value(i * 3 + 2, j);
            T_amb_int_on_m_dot_htf(j, i * 2 + 1) = -(mc_m_dot_htf_ind.Get_Value(i * 3 + 1, j) - m_Y_at_ref[i] - m_ME_T_amb_low[i] - (mc_m_dot_htf_ind.Get_Value(i * 3 + 2, j) - m_Y_at_ref[i]));
            // upper level interaction effect
            aa = mc_m_dot_htf_ind.Get_Value(i * 3 + 3, j);
            bb = m_ME_T_amb_high[i];
            cc = mc_m_dot_htf_ind.Get_Value(i * 3 + 2, j);
            T_amb_int_on_m_dot_htf(j, i * 2 + 2) = -(mc_m_dot_htf_ind.Get_Value(i * 3 + 3, j) - m_Y_at_ref[i] - m_ME_T_amb_high[i] - (mc_m_dot_htf_ind.Get_Value(i * 3 + 2, j) - m_Y_at_ref[i]));
        }

        // HTF mass flow
        for (int j = 0; j < n_T_htf_runs; j++)
        {
            if (i == 0)
            {
                m_dot_htf_int_on_T_htf(j, 0) = mc_T_htf_ind.get_x_value_x_col_0(j);
            }
            // lower level interaction effect
            double aa = mc_T_htf_ind.Get_Value(i * 3 + 1, j);
            double bb = m_ME_m_dot_htf_low[i];
            double cc = mc_T_htf_ind.Get_Value(i * 3 + 2, j);
            m_dot_htf_int_on_T_htf(j, i * 2 + 1) = -(mc_T_htf_ind.Get_Value(i * 3 + 1, j) - m_Y_at_ref[i] - m_ME_m_dot_htf_low[i] - (mc_T_htf_ind.Get_Value(i * 3 + 2, j) - m_Y_at_ref[i]));
            // upper level interaction effect
            aa = mc_T_htf_ind.Get_Value(i * 3 + 3, j);
            bb = m_ME_m_dot_htf_high[i];
            cc = mc_T_htf_ind.Get_Value(i * 3 + 2, j);
            m_dot_htf_int_on_T_htf(j, i * 2 + 2) = -(mc_T_htf_ind.Get_Value(i * 3 + 3, j) - m_Y_at_ref[i] - m_ME_m_dot_htf_high[i] - (mc_T_htf_ind.Get_Value(i * 3 + 2, j) - m_Y_at_ref[i]));
        }
    }

    // Initialize Linear_Interp classes for interaction effects
    if (!mc_T_htf_on_T_amb.Set_1D_Lookup_Table(T_htf_int_on_T_amb, column_index_array, 1, error_index))
    {
        throw(C_csp_exception("Initialization of interpolation table for the interaction effect of T_HTF levels"
            "on the ambient temperature failed", "User defined power cycle initialization"));
    }
    if (!mc_T_amb_on_m_dot_htf.Set_1D_Lookup_Table(T_amb_int_on_m_dot_htf, column_index_array, 1, error_index))
    {
        throw(C_csp_exception("Initialization of interpolation table for the interaction effect of T_amb levels"
            "on HTF mass flow rate failed", "User defined power cycle initialization"));
    }
    if (!mc_m_dot_htf_on_T_htf.Set_1D_Lookup_Table(m_dot_htf_int_on_T_htf, column_index_array, 1, error_index))
    {
        throw(C_csp_exception("Initialization of interpolation table for the interaction effect of m_dot_HTF levels"
            "on the HTF temperature failed", "User defined power cycle initialization"));
    }

}

double C_ud_power_cycle::get_W_dot_gross_ND_interp(double T_htf_hot /*C*/, double T_amb /*C*/, double m_dot_htf_ND /*-*/)
{
	// This call needs to define which columns to search
	// Then use 'get_interpolated_ND_output' to get ND total effect
	
	return get_interpolated_ND_output(i_W_dot_gross, T_htf_hot, T_amb, m_dot_htf_ND);

	// Also, maybe want to check parameters against max/min, or if extrapolating, or something?
}

double C_ud_power_cycle::get_Q_dot_HTF_ND_interp(double T_htf_hot /*C*/, double T_amb /*C*/, double m_dot_htf_ND /*-*/)
{
	// This call needs to define which columns to search
	// Then use 'get_interpolated_ND_output' to get ND total effect

	return get_interpolated_ND_output(i_Q_dot_HTF, T_htf_hot, T_amb, m_dot_htf_ND);

	// Also, maybe want to check parameters against max/min, or if extrapolating, or something?
}

double C_ud_power_cycle::get_W_dot_cooling_ND_interp(double T_htf_hot /*C*/, double T_amb /*C*/, double m_dot_htf_ND /*-*/)
{
	// This call needs to define which columns to search
	// Then use 'get_interpolated_ND_output' to get ND total effect

	return get_interpolated_ND_output(i_W_dot_cooling, T_htf_hot, T_amb, m_dot_htf_ND);

	// Also, maybe want to check parameters against max/min, or if extrapolating, or something?
}

double C_ud_power_cycle::get_m_dot_water_ND_interp(double T_htf_hot /*C*/, double T_amb /*C*/, double m_dot_htf_ND /*-*/)
{
	// This call needs to define which columns to search
	// Then use 'get_interpolated_ND_output' to get ND total effect

    return get_interpolated_ND_output(i_m_dot_water, T_htf_hot, T_amb, m_dot_htf_ND);
	// Also, maybe want to check parameters against max/min, or if extrapolating, or something?
}

double C_ud_power_cycle::get_W_dot_gross_nd(double T_htf_hot /*C*/, double T_amb /*C*/,
    double m_dot_htf_ND /*-*/, double max_frac /*-*/)
{
    if (m_E_udpc_max_output_correction_mode == C_ud_power_cycle::SCO2_HEURISTIC) {
        double W_dot_gross_ND, q_dot_ND, W_dot_cooling_ND, m_dot_water_ND;
        udpc_sco2_regr_off_design(T_htf_hot, T_amb, m_dot_htf_ND, max_frac,
            W_dot_gross_ND, q_dot_ND, W_dot_cooling_ND, m_dot_water_ND);
        return W_dot_gross_ND;
    }
    else {
        return get_W_dot_gross_ND_interp(T_htf_hot, T_amb, m_dot_htf_ND);
    }
}

double C_ud_power_cycle::get_Q_dot_HTF_nd(double T_htf_hot /*C*/, double T_amb /*C*/,
    double m_dot_htf_ND /*-*/, double max_frac /*-*/)
{
    if (m_E_udpc_max_output_correction_mode == C_ud_power_cycle::SCO2_HEURISTIC) {
        double W_dot_gross_ND, q_dot_ND, W_dot_cooling_ND, m_dot_water_ND;
        udpc_sco2_regr_off_design(T_htf_hot, T_amb, m_dot_htf_ND, max_frac,
            W_dot_gross_ND, q_dot_ND, W_dot_cooling_ND, m_dot_water_ND);
        return q_dot_ND;
    }
    else {
        return get_Q_dot_HTF_ND_interp(T_htf_hot, T_amb, m_dot_htf_ND);
    }
}

double C_ud_power_cycle::get_W_dot_cooling_nd(double T_htf_hot /*C*/, double T_amb /*C*/,
    double m_dot_htf_ND /*-*/, double max_frac /*-*/)
{
    if (m_E_udpc_max_output_correction_mode == C_ud_power_cycle::SCO2_HEURISTIC) {
        double W_dot_gross_ND, q_dot_ND, W_dot_cooling_ND, m_dot_water_ND;
        udpc_sco2_regr_off_design(T_htf_hot, T_amb, m_dot_htf_ND, max_frac,
            W_dot_gross_ND, q_dot_ND, W_dot_cooling_ND, m_dot_water_ND);
        return W_dot_cooling_ND;
    }
    else {
        return get_W_dot_cooling_ND_interp(T_htf_hot, T_amb, m_dot_htf_ND);
    }
}

double C_ud_power_cycle::get_m_dot_water_nd(double T_htf_hot /*C*/, double T_amb /*C*/,
    double m_dot_htf_ND /*-*/, double max_frac /*-*/)
{
    if (m_E_udpc_max_output_correction_mode == C_ud_power_cycle::SCO2_HEURISTIC) {
        double W_dot_gross_ND, q_dot_ND, W_dot_cooling_ND, m_dot_water_ND;
        udpc_sco2_regr_off_design(T_htf_hot, T_amb, m_dot_htf_ND, max_frac,
            W_dot_gross_ND, q_dot_ND, W_dot_cooling_ND, m_dot_water_ND);
        return m_dot_water_ND;
    }
    else {
        return get_m_dot_water_ND_interp(T_htf_hot, T_amb, m_dot_htf_ND);
    }
}

void C_ud_power_cycle::get_sco2_regr_max_ND_q_dot(double T_htf_hot /*C*/, double T_amb /*C*/, double m_dot_max_ND /*-*/,
    double& delta_T_HTF_OD /*C*/, double& m_dot_htf_ND_max /*-*/, double& q_dot_htf_ND_max /*-*/)
{
    if (!m_is_sco2_design_set) {
        throw(C_csp_exception("You must set design information via set_sco2_design_for_sco2_regr method before"
            " calling udpc_sco2_regr_off_design method."));
    }

    // 0)
    delta_T_HTF_OD = T_htf_hot - m_T_htf_cold_des_sco2_regr;    //[C]

    // 1)
    q_dot_htf_ND_max = get_Q_dot_HTF_ND_interp(T_htf_hot, T_amb, m_dot_max_ND);

    // 2)
    m_dot_htf_ND_max = q_dot_htf_ND_max / (delta_T_HTF_OD / m_deltaT_HTF_des);

    return;
}

void C_ud_power_cycle::udpc_sco2_regr_off_design(double T_htf_hot /*C*/, double T_amb /*C*/, double m_dot_htf_ND /*-*/,
    double m_dot_max_ND,
    double& W_dot_gross_ND, double& q_dot_ND, double& W_dot_cooling_ND, double& m_dot_water_ND)
{
    // 0) Calculate off-design temperature difference over HTF using OD hot htf temperature
    // 1) Get q_dot_ND_max at m_dot_ND = 1
    // 2) m_dot_ND_max = q_dot_ND_max / (delta_T_HTF_OD/m_deltaT_HTF_des)
    // 3) New performance model as f(m_dot_ND, T_amb, T_htf)
    // --- a) eta_gross_ND = 'original' udpc model = w_dot_gross(udpc) / q_dot(udpc)
    // --- b) if m_dot_ND > q_dot_ND_max
    // --------- q_dot_ND = q_dot_ND_max
    // -------else
    // --------- q_dot_ND = m_dot_ND
    //     c) W_dot_ND = q_dot_ND * eta_ND
    //     d) W_dot_parasitics = 1.0
    //     e) m_dot_water_ND = interpolate

    // ----------------------------------------------------------------------------

    if (!m_is_sco2_design_set) {
        throw(C_csp_exception("You must set design information via set_sco2_design_for_sco2_regr method before"
            " calling udpc_sco2_regr_off_design method."));
    }

    // 0, 1, 2)
    double delta_T_HTF_OD, q_dot_htf_ND_max_regr, m_dot_htf_ND_max_regr;
    delta_T_HTF_OD = q_dot_htf_ND_max_regr =  m_dot_htf_ND_max_regr = std::numeric_limits<double>::quiet_NaN();
    get_sco2_regr_max_ND_q_dot(T_htf_hot /*C*/, T_amb /*C*/, m_dot_max_ND /*-*/,
        delta_T_HTF_OD, m_dot_htf_ND_max_regr, q_dot_htf_ND_max_regr);

    // 0)
    //delta_T_HTF_OD = T_htf_hot - m_T_htf_cold_des_sco2_regr;    //[C]
    //
    //// 1)
    //double q_dot_htf_ND_max_regr = get_Q_dot_HTF_ND_interp(T_htf_hot, T_amb, m_dot_max_ND);
    //
    //// 2)
    //double m_dot_htf_ND_max_regr = q_dot_htf_ND_max_regr / (delta_T_HTF_OD/m_deltaT_HTF_des);

    // 3.a)
    double q_dot_ND_udpc = get_Q_dot_HTF_ND_interp(T_htf_hot, T_amb, m_dot_htf_ND);
    double w_dot_gross_ND_udpc = get_W_dot_gross_ND_interp(T_htf_hot, T_amb, m_dot_htf_ND);
    double eta_gross_ND_udpc = w_dot_gross_ND_udpc / q_dot_ND_udpc;

    // 3.b)
    q_dot_ND = std::numeric_limits<double>::quiet_NaN();
    if (m_dot_htf_ND > m_dot_htf_ND_max_regr) {
        q_dot_ND = q_dot_htf_ND_max_regr;
    }
    else {
        q_dot_ND = m_dot_htf_ND*(delta_T_HTF_OD / m_deltaT_HTF_des);
    }

    // 3.c)
    W_dot_gross_ND = q_dot_ND * eta_gross_ND_udpc;

    // 3.d)
    W_dot_cooling_ND = get_W_dot_cooling_ND_interp(T_htf_hot, T_amb, m_dot_htf_ND);

    // 3.e)
    m_dot_water_ND = get_m_dot_water_ND_interp(T_htf_hot, T_amb, m_dot_htf_ND);
}

void C_ud_power_cycle::get_max_m_dot_and_W_dot_ND(double T_htf_hot /*C*/, double T_amb /*C*/,
    double max_frac /*-*/, double cutoff_frac /*-*/,
    double& m_dot_HTF_ND_max, double& W_dot_gross_ND_max)
{
    // 22-12-13 try using q_dot_ND instead of W_dot_ND

    // Heuristic sets max ND mass flow to q_dot_ND at global max ND mass flow rate

    if (m_E_udpc_max_output_correction_mode == C_ud_power_cycle::SCO2_HEURISTIC) {

        double delta_T_HTF_OD, q_dot_htf_ND_max;
        delta_T_HTF_OD = m_dot_HTF_ND_max = q_dot_htf_ND_max = std::numeric_limits<double>::quiet_NaN();
        get_sco2_regr_max_ND_q_dot(T_htf_hot /*C*/, T_amb /*C*/, max_frac /*-*/,
            delta_T_HTF_OD, m_dot_HTF_ND_max, q_dot_htf_ND_max);

    }
    else if(m_E_udpc_max_output_correction_mode == C_ud_power_cycle::SIMPLE_HEURISTIC) {
        // Calculate non-dimensional mass flow rate relative to design point
        m_dot_HTF_ND_max = max_frac;		//[-] Use max mass flow rate

        double q_dot_ND_max = get_Q_dot_HTF_ND_interp(T_htf_hot,
            T_amb,
            m_dot_HTF_ND_max);	//[-]

        if (q_dot_ND_max >= m_dot_HTF_ND_max)
        {
            return;
        }

        // set m_dot_ND to q_dot_max
        m_dot_HTF_ND_max = q_dot_ND_max;
    }
    else if (m_E_udpc_max_output_correction_mode == C_ud_power_cycle::NO_CORRECTION) {
        m_dot_HTF_ND_max = max_frac;
    }

    W_dot_gross_ND_max = get_W_dot_gross_nd(T_htf_hot,
        T_amb,
        m_dot_HTF_ND_max,
        max_frac);	//[-]

    return;
}

void C_ud_power_cycle::get_ind_var_params(std::vector<double>& v_T_htf_unique, std::vector<double>& v_m_dot_unique,
    std::vector<double>& v_T_amb_unique)
{
    v_T_htf_unique = mv_T_htf_unique;
    v_T_amb_unique = mv_T_amb_unique;
    v_m_dot_unique = mv_m_dot_unique;
}

double C_ud_power_cycle::get_interpolated_ND_output(int i_ME /*M.E. table index*/, 
							double T_htf_hot /*C*/, double T_amb /*C*/, double m_dot_htf_ND /*-*/)
{
	
	double ME_T_htf = mc_T_htf_ind.interpolate_x_col_0(i_ME*3+2, T_htf_hot) - m_Y_at_ref[i_ME];
	double ME_T_amb = mc_T_amb_ind.interpolate_x_col_0(i_ME*3+2, T_amb) - m_Y_at_ref[i_ME];
	double ME_m_dot_htf = mc_m_dot_htf_ind.interpolate_x_col_0(i_ME*3+2, m_dot_htf_ND) - m_Y_at_ref[i_ME];

	double INT_T_htf_on_T_amb = 0.0;
	if( T_htf_hot < m_T_htf_ref )
	{
		INT_T_htf_on_T_amb = mc_T_htf_on_T_amb.interpolate_x_col_0(i_ME*2+1,T_amb)*(T_htf_hot-m_T_htf_ref)/(m_T_htf_ref-m_T_htf_low);
	}
	if( T_htf_hot > m_T_htf_ref )
	{
		INT_T_htf_on_T_amb = mc_T_htf_on_T_amb.interpolate_x_col_0(i_ME*2+2,T_amb)*(T_htf_hot-m_T_htf_ref)/(m_T_htf_ref-m_T_htf_high);
	}

	double INT_T_amb_on_m_dot_htf = 0.0;
	if( T_amb < m_T_amb_ref )
	{
		INT_T_amb_on_m_dot_htf = mc_T_amb_on_m_dot_htf.interpolate_x_col_0(i_ME*2+1,m_dot_htf_ND)*(T_amb-m_T_amb_ref)/(m_T_amb_ref-m_T_amb_low);
	}
	if( T_amb > m_T_amb_ref )
	{
		INT_T_amb_on_m_dot_htf = mc_T_amb_on_m_dot_htf.interpolate_x_col_0(i_ME*2+2,m_dot_htf_ND)*(T_amb-m_T_amb_ref)/(m_T_amb_ref-m_T_amb_high);
	}

	double INT_m_dot_htf_on_T_htf = 0.0;
	if( m_dot_htf_ND < m_m_dot_htf_ref )
	{
		INT_m_dot_htf_on_T_htf = mc_m_dot_htf_on_T_htf.interpolate_x_col_0(i_ME*2+1,T_htf_hot)*(m_dot_htf_ND-m_m_dot_htf_ref)/(m_m_dot_htf_ref-m_m_dot_htf_low);
	}
	if( m_dot_htf_ND > m_m_dot_htf_ref )
	{
        INT_m_dot_htf_on_T_htf = mc_m_dot_htf_on_T_htf.interpolate_x_col_0(i_ME*2+2,T_htf_hot)*(m_dot_htf_ND-m_m_dot_htf_ref)/(m_m_dot_htf_ref-m_m_dot_htf_high);
	}

	return m_Y_at_ref[i_ME] + ME_T_htf + ME_T_amb + ME_m_dot_htf + INT_T_htf_on_T_amb + INT_T_amb_on_m_dot_htf + INT_m_dot_htf_on_T_htf;
}



C_ud_pc_table_generator::C_ud_pc_table_generator(C_od_pc_function & f_pc_eq) : mf_pc_eq(f_pc_eq)
{
	mf_callback = 0;		// = NULL
	mp_mf_active = 0;			// = NULL
	m_progress_msg = "Power cycle preprocessing...";
	m_log_msg = "Log message";

	return;
}

void C_ud_pc_table_generator::send_callback(bool is_od_model_error, int run_number, int n_runs_total,
	double T_htf_hot, double m_dot_htf_ND, double T_amb,
	double W_dot_gross_ND, double Q_dot_in_ND,
	double W_dot_cooling_ND, double m_dot_water_ND)
{
	if (mf_callback && mp_mf_active)
	{
		std::string od_err_msg = "";
		if (is_od_model_error)
		{
			od_err_msg = "***************\nWarning: off design model failed\n"
				"Using generic off design for this point\n"
				"Check if values are appropriate before running annual simulation\n"
				"***************\n";
		}

		m_log_msg = od_err_msg + util::format("[%d/%d] At T_htf = %lg [C],"
			" normalized m_dot = %lg,"
			" and T_amb = %lg [C]. The normalized outputs are: gross power = %lg,"
			" thermal input = %lg, cooling power = %lg, and water use = %lg",
			run_number, n_runs_total,
			T_htf_hot, m_dot_htf_ND, T_amb,
			W_dot_gross_ND, Q_dot_in_ND,
			W_dot_cooling_ND, m_dot_water_ND);
		if (!mf_callback(m_log_msg, m_progress_msg, mp_mf_active, 100.0*run_number / n_runs_total, 2))
		{
			std::string error_msg = "User terminated simulation...";
			std::string loc_msg = "C_ud_pc_table_generator";
			throw(C_csp_exception(error_msg, loc_msg, 1));
		}
	}
}

int C_ud_pc_table_generator::generate_tables(double T_htf_ref /*C*/, double T_htf_low /*C*/, double T_htf_high /*C*/, int n_T_htf /*-*/,
	double T_amb_ref /*C*/, double T_amb_low /*C*/, double T_amb_high /*C*/, int n_T_amb /*-*/,
	double m_dot_htf_ND_ref /*-*/, double m_dot_htf_ND_low /*-*/, double m_dot_htf_ND_high /*-*/, int n_m_dot_htf_ND,
	util::matrix_t<double> & T_htf_ind, util::matrix_t<double> & T_amb_ind, util::matrix_t<double> & m_dot_htf_ind)
{
	// Check levels against design values
	if(T_htf_low >= T_htf_ref)
	{
		std::string msg = util::format("The lower level of HTF temperature %lg [C] must be colder than the design temperature %lg [C].",
							T_htf_low, T_htf_ref);
		throw(C_csp_exception(msg, "User defined power cycle, generate tables"));
	}
	if(T_htf_high <= T_htf_ref)
	{
		std::string msg = util::format("The upper level of HTF temperature %lg [C] must be hotter than the design temperature %lg [C].",
							T_htf_high, T_htf_ref);
		throw(C_csp_exception(msg, "User defined power cycle, generate tables"));
	}
	if(T_amb_low >= T_amb_ref)
	{
		std::string msg = util::format("The lower level of ambient temperature %lg [C] must be colder than the design temperatuare %lg [C].",
							T_amb_low, T_amb_ref);
		throw(C_csp_exception(msg, "User defined power cycle, generate tables"));
	}
	if(T_amb_high <= T_amb_ref)
	{
		std::string msg = util::format("The upper level of ambient temperature %lg [C] must be warmer than the design temperature %lg [C].",
							T_amb_high, T_amb_ref);
		throw(C_csp_exception(msg, "User defined power cycle, generate tables"));
	}
	if(m_dot_htf_ND_low >= m_dot_htf_ND_ref)
	{
		std::string msg = util::format("The lower level of the normalized HTF mass flow rate %lg must be less than the design value %lg.",
							m_dot_htf_ND_low, m_dot_htf_ND_ref);
		throw(C_csp_exception(msg, "User defined power cycle, generate tables"));
	}
	if(m_dot_htf_ND_high <= m_dot_htf_ND_ref)
	{
		std::string msg = util::format("The upper level of the normalized HTF mass flow rate %lg must be greater than the design value %lg.",
							m_dot_htf_ND_high, m_dot_htf_ND_ref);
		throw(C_csp_exception(msg, "User defined power cycle, generate tables"));
	}

	C_od_pc_function::S_f_inputs pc_inputs;
	C_od_pc_function::S_f_outputs pc_outputs;

	// ******************************************
	// Setup T_HTF parameteric runs
	if(n_T_htf < 3)
	{
		std::string msg = util::format("The input argument for number of indepedent HTF temperatures is %d."
						" It was reset to the minimum value of 3.", n_T_htf);
		mc_messages.add_notice(msg);
		n_T_htf = 3;
	}

	T_htf_ind.clear();
	T_htf_ind.resize(n_T_htf, 13);		// Set matrix size
	double delta_T_htf = (T_htf_high - T_htf_low)/double(n_T_htf-1);

	double n_runs_total = 3.0*(n_T_htf + n_T_amb + n_m_dot_htf_ND);

	// Set ambient temperature because it is constant for the HTF temperature parametrics
	pc_inputs.m_T_amb = T_amb_ref;	//[C]
	for(int i = 0; i < n_T_htf; i++)
	{
		T_htf_ind(i,0) = T_htf_low + delta_T_htf*i;	//[C]
		pc_inputs.m_T_htf_hot = T_htf_ind(i,0);

		// Call at low, ref, and high ND mass flow rate levels
		std::vector<double> m_dot_htf_ND_levels(3);
		m_dot_htf_ND_levels[0] = m_dot_htf_ND_low;
		m_dot_htf_ND_levels[1] = m_dot_htf_ND_ref;
		m_dot_htf_ND_levels[2] = m_dot_htf_ND_high;
		for(int j = 0; j < 3; j++)
		{
			bool is_od_model_error = false;

			pc_inputs.m_m_dot_htf_ND = m_dot_htf_ND_levels[j];
			int off_design_code = mf_pc_eq(pc_inputs,pc_outputs);

			if( off_design_code == 0 )
			{
				// Save outputs
				T_htf_ind(i,1+j) = pc_outputs.m_W_dot_gross_ND;		//[-]
				T_htf_ind(i,4+j) = pc_outputs.m_Q_dot_in_ND;		//[-]
				T_htf_ind(i,7+j) = pc_outputs.m_W_dot_cooling_ND;	//[-]
				T_htf_ind(i,10+j) = pc_outputs.m_m_dot_water_ND;	//[-]
			}
			else if (off_design_code == -1)
			{
				// Save 'generic' off design model response
				T_htf_ind(i, 1 + j) = pc_inputs.m_m_dot_htf_ND;		//[-]
				T_htf_ind(i, 4 + j) = pc_inputs.m_m_dot_htf_ND;		//[-]
				T_htf_ind(i, 7 + j) = pc_inputs.m_m_dot_htf_ND;		//[-]
				T_htf_ind(i, 10 + j) = pc_inputs.m_m_dot_htf_ND;	//[-]

				is_od_model_error = true;
			}
			else
			{
				std::string err_msg = util::format("The 1st UDPC table (primary: T_htf, interaction: m_dot_htf_ND) generation failed at T_htf = %lg [C] and m_dot_htf = %lg [-]", pc_inputs.m_T_htf_hot, pc_inputs.m_m_dot_htf_ND);
				throw(C_csp_exception(err_msg, "UDPC"));
			}

			double run_number = i*3 + j;

			send_callback(is_od_model_error, (int)run_number + 1, (int)n_runs_total,
				pc_inputs.m_T_htf_hot, pc_inputs.m_m_dot_htf_ND, pc_inputs.m_T_amb,
				T_htf_ind(i, 1 + j), T_htf_ind(i, 4 + j),
				T_htf_ind(i, 7 + j), T_htf_ind(i, 10 + j));

		}

	}
	// ******************************************

	// ******************************************
	// Setup T_amb parametric runs
	if(n_T_amb < 3)
	{
		std::string msg = util::format("The input argument for number of independent ambient temperatures"
						" is %d. It was reset to the minimum value of 3.", n_T_amb);
		mc_messages.add_notice(msg);
		n_T_amb = 3;
	}

	T_amb_ind.clear();
	T_amb_ind.resize(n_T_amb, 13);		// Set matrix size
	double delta_T_amb = (T_amb_high - T_amb_low)/double(n_T_amb-1);

	// Set ND htf mass flow rate because it is constant for the ambient temperature parametrics
	pc_inputs.m_m_dot_htf_ND = m_dot_htf_ND_ref;
	for(int i = 0; i < n_T_amb; i++)
	{
		T_amb_ind(i,0) = T_amb_low + delta_T_amb*i;		//[C]
		pc_inputs.m_T_amb = T_amb_ind(i,0);				//[C]

		// Call at low, ref, and high HTF temperature levels
		std::vector<double> T_htf_levels(3);
		T_htf_levels[0] = T_htf_low;   //[C]
		T_htf_levels[1] = T_htf_ref;   //[C]
		T_htf_levels[2] = T_htf_high;  //[C]
		for(int j = 0; j < 3; j++)
		{
			bool is_od_model_error = false;

			pc_inputs.m_T_htf_hot = T_htf_levels[j];
			int off_design_code = mf_pc_eq(pc_inputs,pc_outputs);

			if( off_design_code == 0 )
			{
				// Save outputs
				T_amb_ind(i,1+j) = pc_outputs.m_W_dot_gross_ND;		//[-]
				T_amb_ind(i,4+j) = pc_outputs.m_Q_dot_in_ND;		//[-]
				T_amb_ind(i,7+j) = pc_outputs.m_W_dot_cooling_ND;	//[-]
				T_amb_ind(i,10+j) = pc_outputs.m_m_dot_water_ND;	//[-]
			}
			else if (off_design_code == -1)
			{
				// Save 'generic' off design model response
				T_amb_ind(i, 1 + j) = pc_inputs.m_m_dot_htf_ND;		//[-]
				T_amb_ind(i, 4 + j) = pc_inputs.m_m_dot_htf_ND;		//[-]
				T_amb_ind(i, 7 + j) = pc_inputs.m_m_dot_htf_ND;		//[-]
				T_amb_ind(i, 10 + j) = pc_inputs.m_m_dot_htf_ND;	//[-]

				is_od_model_error = true;
			}
			else
			{
				std::string err_msg = util::format("The 2nd UDPC table (primary: T_amb, interaction: T_htf) generation failed at T_amb = %lg [C] and T_htf = %lg [C]", pc_inputs.m_T_amb, pc_inputs.m_T_htf_hot);
				throw(C_csp_exception(err_msg, "UDPC"));
			}

			double run_number = 3.0*n_T_htf + i*3 + j;

			send_callback(is_od_model_error, (int)run_number + 1, (int)n_runs_total,
				pc_inputs.m_T_htf_hot, pc_inputs.m_m_dot_htf_ND, pc_inputs.m_T_amb,
				T_amb_ind(i, 1 + j), T_amb_ind(i, 4 + j),
				T_amb_ind(i, 7 + j), T_amb_ind(i, 10 + j));
		}
	}
	// ******************************************

	// ******************************************
	// Setup ND m_dot parametric runs
	if(n_m_dot_htf_ND < 3)
	{
		std::string msg = util::format("The input argument for number of independent normalized HTF mass flow rates"
						" is %d. It was reset to the minimum value of 3.", n_m_dot_htf_ND);
		mc_messages.add_notice(msg);
		n_m_dot_htf_ND = 3;
	}

	m_dot_htf_ind.clear();
	m_dot_htf_ind.resize(n_m_dot_htf_ND,13);		// Set matrix size
	double delta_m_dot = (m_dot_htf_ND_high-m_dot_htf_ND_low)/double(n_m_dot_htf_ND-1);

	// Set HTF temperature because it is constant for the ambient temperature parametrics
	pc_inputs.m_T_htf_hot = T_htf_ref;
	for(int i = 0; i < n_m_dot_htf_ND; i++)
	{
		m_dot_htf_ind(i,0) = m_dot_htf_ND_low + delta_m_dot*i;		//[-]
		pc_inputs.m_m_dot_htf_ND = m_dot_htf_ind(i,0);				//[-]

		// Call at low, ref, and high ambient temperatures
		std::vector<double> T_amb_levels(3);
		T_amb_levels[0] = T_amb_low;    //[C]
		T_amb_levels[1] = T_amb_ref;	//[C]
		T_amb_levels[2] = T_amb_high;	//[C]
		for(int j = 0; j < 3; j++)
		{
			bool is_od_model_error = false;

			pc_inputs.m_T_amb = T_amb_levels[j];
			int off_design_code = mf_pc_eq(pc_inputs, pc_outputs);
		
			if( off_design_code == 0 )
			{
				// Save outputs
				m_dot_htf_ind(i,1+j) = pc_outputs.m_W_dot_gross_ND;		//[-]
				m_dot_htf_ind(i,4+j) = pc_outputs.m_Q_dot_in_ND;		//[-]
				m_dot_htf_ind(i,7+j) = pc_outputs.m_W_dot_cooling_ND;	//[-]
				m_dot_htf_ind(i,10+j) = pc_outputs.m_m_dot_water_ND;	//[-]
			}
			else if (off_design_code == -1)
			{
				// Save 'generic' off design model response
				m_dot_htf_ind(i, 1 + j) = pc_inputs.m_m_dot_htf_ND;		//[-]
				m_dot_htf_ind(i, 4 + j) = pc_inputs.m_m_dot_htf_ND;		//[-]
				m_dot_htf_ind(i, 7 + j) = pc_inputs.m_m_dot_htf_ND;		//[-]
				m_dot_htf_ind(i, 10 + j) = pc_inputs.m_m_dot_htf_ND;	//[-]

				is_od_model_error = true;
			}
			else
			{
				std::string err_msg = util::format("The 3rd UDPC table (primary: m_dot_htf_ND, interaction: T_amb) generation failed at T_amb = %lg [C] and m_dot_htf = %lg [-]", pc_inputs.m_T_amb, pc_inputs.m_m_dot_htf_ND);
				throw(C_csp_exception(err_msg, "UDPC"));
			}

			double run_number = 3.0*n_T_htf + 3.0*n_T_amb  + i*3 + j;

			send_callback(is_od_model_error, (int)run_number + 1, (int)n_runs_total,
				pc_inputs.m_T_htf_hot, pc_inputs.m_m_dot_htf_ND, pc_inputs.m_T_amb,
				m_dot_htf_ind(i, 1 + j), m_dot_htf_ind(i, 4 + j),
				m_dot_htf_ind(i, 7 + j), m_dot_htf_ind(i, 10 + j));
		}
	}
	// ******************************************
	
	return 0;
}

void N_udpc_common::get_var_setup(const std::vector<double>& vec_unique, const std::vector<double>& var_vec,
    double& var_des, double& var_low, double& var_high)
{
    std::unordered_map<double, int> var_val_count;
    std::vector<int> v_var_count;
    int n_var_unique = vec_unique.size();

    // For each unique value in vec_unique, count instances in var_vec
    for (int i = 0; i < n_var_unique; i++)
    {
        var_val_count.insert(std::pair<double, int>(vec_unique[i], std::count(var_vec.begin(), var_vec.end(), vec_unique[i])));
        v_var_count.push_back(std::count(var_vec.begin(), var_vec.end(), vec_unique[i]));
    }

    // Sort instance counts
    std::sort(v_var_count.begin(), v_var_count.end());
    int var_count_max = v_var_count[n_var_unique - 1];      // highest count
    int var_count_2 = v_var_count[n_var_unique - 2];        // 2nd highest count
    int var_count_3 = v_var_count[n_var_unique - 3];        // 3rd highest count
    int var_count_4 = 0;
    if (v_var_count.size() > 3) {
        var_count_4 = v_var_count[n_var_unique - 4];        // 4th highest count
    }

    // Map independent variable with count, then check against some udpc rules
    var_des = std::numeric_limits<double>::quiet_NaN();
    double var_level_1 = std::numeric_limits<double>::quiet_NaN();
    double var_level_2 = std::numeric_limits<double>::quiet_NaN();
    std::unordered_map<double, int>::iterator it_map = var_val_count.begin();
    for (int i = 0; i < n_var_unique; i++)
    {
        if (it_map->second == var_count_max)
        {
            var_des = it_map->first;
        }
        if (it_map->second == var_count_2 && !std::isfinite(var_level_1))
        {
            var_level_1 = it_map->first;
        }
        if (it_map->second == var_count_3)
        {
            var_level_2 = it_map->first;
        }
        it_map++;
    }
    if (var_level_1 < var_level_2)
    {
        var_low = var_level_1;
        var_high = var_level_2;
    }
    else
    {
        var_low = var_level_2;
        var_high = var_level_1;
    }

    if (var_count_3 < 4) {
        throw(C_csp_exception("UDPC parametric for each variable must contain at least 4 unique values"));
    }
    if (var_count_4 == var_count_3) {
        throw(C_csp_exception("UDPC parametric must have more instances for 3rd most common var then 4th most common var"));
    }
    if (var_count_max == var_count_2) {
        throw(C_csp_exception("UDPC parametric must have more instances for most common var (design) then 2nd most common var"));
    }
}

bool N_udpc_common::is_level_in_par(const std::vector<std::vector<double>> test_combs,
    const std::vector<std::vector<double>> full_table)
{
    int n_tbl_rows = full_table.size();
    bool des__low = false;
    bool des__des = false;
    bool des__high = false;
    for (int i = 0; i < n_tbl_rows; i++)
    {
        // Is T_amb_design in a row with T_htf_low and m_dot_des?
        if (test_combs[0] == std::vector<double>{ full_table[i][C_ud_power_cycle::E_COL_T_HTF], full_table[i][C_ud_power_cycle::E_COL_M_DOT], full_table[i][C_ud_power_cycle::E_COL_T_AMB] })
        {
            des__low = true;
        }
        // Is T_amb_design in a row with T_htf_des and m_dot_des?
        if (test_combs[1] == std::vector<double>{ full_table[i][C_ud_power_cycle::E_COL_T_HTF], full_table[i][C_ud_power_cycle::E_COL_M_DOT], full_table[i][C_ud_power_cycle::E_COL_T_AMB] })
        {
            des__des = true;
        }
        // Is T_amb_design in a row with T_htf_high and m_dot_des?
        if (test_combs[2] == std::vector<double>{ full_table[i][C_ud_power_cycle::E_COL_T_HTF], full_table[i][C_ud_power_cycle::E_COL_M_DOT], full_table[i][C_ud_power_cycle::E_COL_T_AMB] })
        {
            des__high = true;
        }

        if (des__low && des__des && des__high)
        {
            break;
        }
    }

    return des__low && des__des && des__high;
}

int N_udpc_common::split_ind_tbl(const util::matrix_t<double>& cmbd_ind, util::matrix_t<double>& T_htf_ind,
    util::matrix_t<double>& m_dot_ind, util::matrix_t<double>& T_amb_ind)
{
    int n_T_htf_pars, n_T_amb_pars, n_m_dot_pars;
    n_T_htf_pars = n_T_amb_pars = n_m_dot_pars = -1;
    double m_dot_low, m_dot_des, m_dot_high, T_htf_low, T_htf_des, T_htf_high, T_amb_low, T_amb_des, T_amb_high;
    m_dot_low = m_dot_des = m_dot_high = T_htf_low = T_htf_des = T_htf_high = T_amb_low = T_amb_des = T_amb_high = std::numeric_limits<double>::quiet_NaN();

    std::vector<double> v_T_htf_unique;
    std::vector<double> v_m_dot_unique;
    std::vector<double> v_T_amb_unique;

    return split_ind_tbl(cmbd_ind, T_htf_ind, m_dot_ind, T_amb_ind,
        v_T_htf_unique, v_m_dot_unique,
        v_T_amb_unique,
        n_T_htf_pars, n_T_amb_pars, n_m_dot_pars,
        m_dot_low, m_dot_des, m_dot_high,
        T_htf_low, T_htf_des, T_htf_high,
        T_amb_low, T_amb_des, T_amb_high);
}

int N_udpc_common::split_ind_tbl(const util::matrix_t<double>& cmbd_ind, util::matrix_t<double>& T_htf_ind,
    util::matrix_t<double>& m_dot_ind, util::matrix_t<double>& T_amb_ind,
    std::vector<double>& v_T_htf_unique, std::vector<double>& v_m_dot_unique,
    std::vector<double>& v_T_amb_unique,
    int& n_T_htf_pars, int& n_T_amb_pars, int& n_m_dot_pars,
    double& m_dot_low, double& m_dot_des, double& m_dot_high,
    double& T_htf_low, double& T_htf_des, double& T_htf_high,
    double& T_amb_low, double& T_amb_des, double& T_amb_high)
{
    // check for minimum length
    int n_par_min = 3;
    int n_levels = 3;
    int n_ind_vars = 3;
    int n_min_runs = n_par_min * n_levels * n_ind_vars;
    int n_table_rows = cmbd_ind.nrows();
    if (n_table_rows < n_min_runs)
    {
        throw(C_csp_exception("Not enough UDPC table rows", "UDPC Table Importation"));
    }

    // get T_htf, m_dot_htf, and T_amb vectors
    util::matrix_t<double> T_htf_col, m_dot_col, T_amb_col;
    T_htf_col = cmbd_ind.col(0);
    m_dot_col = cmbd_ind.col(1);
    T_amb_col = cmbd_ind.col(2);
    std::vector<double> T_htf_vec(T_htf_col.data(), T_htf_col.data() + T_htf_col.ncells());
    std::vector<double> m_dot_vec(m_dot_col.data(), m_dot_col.data() + m_dot_col.ncells());
    std::vector<double> T_amb_vec(T_amb_col.data(), T_amb_col.data() + T_amb_col.ncells());

    // get unique values for each independent variable
    set<double, std::less<double>> T_htf_unique(T_htf_col.data(), T_htf_col.data() + T_htf_col.ncells());
    set<double, std::less<double>> m_dot_unique(m_dot_col.data(), m_dot_col.data() + m_dot_col.ncells());
    set<double, std::less<double>> T_amb_unique(T_amb_col.data(), T_amb_col.data() + T_amb_col.ncells());
    v_T_htf_unique = std::vector<double>(T_htf_unique.begin(), T_htf_unique.end());
    v_m_dot_unique = std::vector<double>(m_dot_unique.begin(), m_dot_unique.end());
    v_T_amb_unique = std::vector<double>(T_amb_unique.begin(), T_amb_unique.end());

    // Get HTF temperature levels
    T_htf_des = T_htf_low = T_htf_high = std::numeric_limits<double>::quiet_NaN();
    get_var_setup(v_T_htf_unique, T_htf_vec, T_htf_des, T_htf_low, T_htf_high);

    // Get HTF mass flow rate levels
    m_dot_des = m_dot_low = m_dot_high = std::numeric_limits<double>::quiet_NaN();
    get_var_setup(v_m_dot_unique, m_dot_vec, m_dot_des, m_dot_low, m_dot_high);

    // Get ambient temperature levels
    T_amb_des = T_amb_low = T_amb_high = std::numeric_limits<double>::quiet_NaN();
    get_var_setup(v_T_amb_unique, T_amb_vec, T_amb_des, T_amb_low, T_amb_high);

    // convert combined matrix_t to a vector of vectors
    // inner vector: single row, outer vector: rows
    // check and throw exception for rows with nan values 
    std::vector<std::vector<double>> cmbd_tbl;
    double* row_start;
    double* row_end;
    for (std::size_t i = 0; i < cmbd_ind.nrows(); i++) {
        util::matrix_t<double> i_row = cmbd_ind.row(i);
        row_start = i_row.data();
        row_end = row_start + i_row.ncols();

        std::vector<double> mat_row(row_start, row_end);
        for (size_t j = 0; j < mat_row.size(); j++) {
            if(std::isnan(mat_row[j])){
                throw(C_csp_exception("UDPC table data contains NaN inputs"));
            }
        }
        
        cmbd_tbl.push_back(mat_row);        
    }

    // Check for inputs runs that don't match udpc rules
    // e.g. parametric variable not available at all 3 levels of secondary var level with constant var
    std::vector<std::vector<double>> vv_test(3);
    std::vector<std::vector<double>::iterator> v_it_erase;

    // Check ambient temperatures
    for (std::vector<double>::iterator i_it = v_T_amb_unique.begin(); i_it < v_T_amb_unique.end(); i_it++){
        vv_test[0] = (std::vector<double>{T_htf_low, m_dot_des, * i_it});
        vv_test[1] = (std::vector<double>{T_htf_des, m_dot_des, * i_it});
        vv_test[2] = (std::vector<double>{T_htf_high, m_dot_des, * i_it});
        if (!is_level_in_par(vv_test, cmbd_tbl)){
            v_it_erase.push_back(i_it);
        }
    }
    for (int i = 0; i < v_it_erase.size(); i++){
        v_T_amb_unique.erase(v_it_erase[v_it_erase.size() - 1 - i]);
    }
    v_it_erase.resize(0);

    // Check HTF temperatures
    for (std::vector<double>::iterator i_it = v_T_htf_unique.begin(); i_it < v_T_htf_unique.end(); i_it++){
        vv_test[0] = std::vector<double>{ *i_it, m_dot_low, T_amb_des };
        vv_test[1] = std::vector<double>{ *i_it, m_dot_des, T_amb_des };
        vv_test[2] = std::vector<double>{ *i_it, m_dot_high, T_amb_des };
        if (!is_level_in_par(vv_test, cmbd_tbl)){
            v_it_erase.push_back(i_it);
        }
    }
    for (int i = 0; i < v_it_erase.size(); i++){
        v_T_htf_unique.erase(v_it_erase[v_it_erase.size() - 1 - i]);
    }
    v_it_erase.resize(0);

    // Check HTF mass flow rates
    for (std::vector<double>::iterator i_it = v_m_dot_unique.begin(); i_it < v_m_dot_unique.end(); i_it++){
        vv_test[0] = std::vector<double>{ T_htf_des, *i_it, T_amb_low };
        vv_test[1] = std::vector<double>{ T_htf_des, *i_it, T_amb_des };
        vv_test[2] = std::vector<double>{ T_htf_des, *i_it, T_amb_high };
        if (!is_level_in_par(vv_test, cmbd_tbl)){
            v_it_erase.push_back(i_it);
        }
    }
    for (int i = 0; i < v_it_erase.size(); i++){
        v_m_dot_unique.erase(v_it_erase[v_it_erase.size() - 1 - i]);
    }

    int total_row_check = 3 * (v_m_dot_unique.size() + v_T_amb_unique.size() + v_T_htf_unique.size());

    n_m_dot_pars = v_m_dot_unique.size();
    n_T_amb_pars = v_T_amb_unique.size();
    n_T_htf_pars = v_T_htf_unique.size();

    if (n_m_dot_pars < n_par_min || n_T_amb_pars < n_par_min || n_T_htf_pars < n_par_min)
    {
        std::string error_msg = util::format("The filtered UDPC parametric for each variable must contain at least %d unique values",
            n_par_min);
        throw(C_csp_exception(error_msg, "User defined power cycle initialization"));
    }

    // Build tables for each independent variable
    const int ncols = 13;

    // HTF temperature table w/ HTF mass flow rate levels
    T_htf_ind.resize_fill(n_T_htf_pars, ncols, 0.0);
    std::vector<double> m_dot_levels = std::vector<double>{ m_dot_low, m_dot_des, m_dot_high };

    // For each T_htf (i) and m_dot_htf_ND level (j) combination...
    for (int i = 0; i < n_T_htf_pars; i++)
    {
        for (int j = 0; j < m_dot_levels.size(); j++)
        {
            // Search through combined table and find corresponding row
            for (int k = 0; k < n_table_rows; k++)
            {
                if (std::vector<double>{v_T_htf_unique[i], m_dot_levels[j], T_amb_des} ==
                    std::vector<double>{ cmbd_tbl[k][C_ud_power_cycle::E_COL_T_HTF], cmbd_tbl[k][C_ud_power_cycle::E_COL_M_DOT], cmbd_tbl[k][C_ud_power_cycle::E_COL_T_AMB] })
                {
                    // Set values in independent variable table
                    //    Independent |    Gross Power Output   |   HTF Thermal Power	|   Cooling Parasitics  |	 Water Use 
                    // 0)  Variable   |  1) -   2) 0     3) +   |  4) -   5) 0    6) +  |  7) -    8) 0    9) + | 10) -  11) 0   12) + 
                    T_htf_ind.set_value(cmbd_tbl[k][C_ud_power_cycle::E_COL_T_HTF], i, 0);
                    T_htf_ind.set_value(cmbd_tbl[k][C_ud_power_cycle::E_COL_W_CYL], i, 3 * C_ud_power_cycle::i_W_dot_gross + 1 + j);
                    T_htf_ind.set_value(cmbd_tbl[k][C_ud_power_cycle::E_COL_Q_CYL], i, 3 * C_ud_power_cycle::i_Q_dot_HTF + 1 + j);
                    T_htf_ind.set_value(cmbd_tbl[k][C_ud_power_cycle::E_COL_W_COOL], i, 3 * C_ud_power_cycle::i_W_dot_cooling + 1 + j);
                    T_htf_ind.set_value(cmbd_tbl[k][C_ud_power_cycle::E_COL_M_H2O], i, 3 * C_ud_power_cycle::i_m_dot_water + 1 + j);
                }
            }
        }
    }

    // HTF mass flow rate w/ ambient temperature levels
    m_dot_ind.resize_fill(n_m_dot_pars, ncols, 0.0);
    std::vector<double> T_amb_levels = std::vector<double>{ T_amb_low, T_amb_des, T_amb_high };

    // For each m_dot_htf_ND (i) and T_amb level (j) combination...
    for (int i = 0; i < n_m_dot_pars; i++)
    {
        for (int j = 0; j < T_amb_levels.size(); j++)
        {
            // Search through combined table and find corresponding row
            for (int k = 0; k < n_table_rows; k++)
            {
                if (std::vector<double>{T_htf_des, v_m_dot_unique[i], T_amb_levels[j]} ==
                    std::vector<double>{ cmbd_tbl[k][C_ud_power_cycle::E_COL_T_HTF], cmbd_tbl[k][C_ud_power_cycle::E_COL_M_DOT], cmbd_tbl[k][C_ud_power_cycle::E_COL_T_AMB] })
                {
                    // Set values in independent variable table
                    //    Independent |    Gross Power Output   |   HTF Thermal Power	|   Cooling Parasitics  |	 Water Use 
                    // 0)  Variable   |  1) -   2) 0     3) +   |  4) -   5) 0    6) +  |  7) -    8) 0    9) + | 10) -  11) 0   12) + 
                    m_dot_ind.set_value(cmbd_tbl[k][C_ud_power_cycle::E_COL_M_DOT], i, 0);
                    m_dot_ind.set_value(cmbd_tbl[k][C_ud_power_cycle::E_COL_W_CYL], i, 3 * C_ud_power_cycle::i_W_dot_gross + 1 + j);
                    m_dot_ind.set_value(cmbd_tbl[k][C_ud_power_cycle::E_COL_Q_CYL], i, 3 * C_ud_power_cycle::i_Q_dot_HTF + 1 + j);
                    m_dot_ind.set_value(cmbd_tbl[k][C_ud_power_cycle::E_COL_W_COOL], i, 3 * C_ud_power_cycle::i_W_dot_cooling + 1 + j);
                    m_dot_ind.set_value(cmbd_tbl[k][C_ud_power_cycle::E_COL_M_H2O], i, 3 * C_ud_power_cycle::i_m_dot_water + 1 + j);
                }
            }
        }
    }

    // Ambient temperature w/ HTF temperature levels
    T_amb_ind.resize_fill(n_T_amb_pars, ncols, 0.0);
    std::vector<double> T_htf_levels = std::vector<double>{ T_htf_low, T_htf_des, T_htf_high };

    // For each T_amb (i) and T_HTF level (j) combination...
    for (int i = 0; i < n_T_amb_pars; i++)
    {
        for (int j = 0; j < T_htf_levels.size(); j++)
        {
            // Search through combined table and find corresponding row
            for (int k = 0; k < n_table_rows; k++)
            {
                if (std::vector<double>{T_htf_levels[j], m_dot_des, v_T_amb_unique[i]} ==
                    std::vector<double>{ cmbd_tbl[k][C_ud_power_cycle::E_COL_T_HTF], cmbd_tbl[k][C_ud_power_cycle::E_COL_M_DOT], cmbd_tbl[k][C_ud_power_cycle::E_COL_T_AMB] })
                {
                    // Set values in independent variable table
                    //    Independent |    Gross Power Output   |   HTF Thermal Power	|   Cooling Parasitics  |	 Water Use 
                    // 0)  Variable   |  1) -   2) 0     3) +   |  4) -   5) 0    6) +  |  7) -    8) 0    9) + | 10) -  11) 0   12) + 
                    T_amb_ind.set_value(cmbd_tbl[k][C_ud_power_cycle::E_COL_T_AMB], i, 0);
                    T_amb_ind.set_value(cmbd_tbl[k][C_ud_power_cycle::E_COL_W_CYL], i, 3 * C_ud_power_cycle::i_W_dot_gross + 1 + j);
                    T_amb_ind.set_value(cmbd_tbl[k][C_ud_power_cycle::E_COL_Q_CYL], i, 3 * C_ud_power_cycle::i_Q_dot_HTF + 1 + j);
                    T_amb_ind.set_value(cmbd_tbl[k][C_ud_power_cycle::E_COL_W_COOL], i, 3 * C_ud_power_cycle::i_W_dot_cooling + 1 + j);
                    T_amb_ind.set_value(cmbd_tbl[k][C_ud_power_cycle::E_COL_M_H2O], i, 3 * C_ud_power_cycle::i_m_dot_water + 1 + j);
                }
            }
        }
    }

    return 0;
}

int N_udpc_common::combine_ind_tbl(util::matrix_t<double>& combined, const util::matrix_t<double>& T_htf_ind,
    const util::matrix_t<double>& m_dot_ind, const util::matrix_t<double>& T_amb_ind,
    double m_dot_low, double m_dot_des, double m_dot_high,
    double T_htf_low, double T_htf_des, double T_htf_high,
    double T_amb_low, double T_amb_des, double T_amb_high)
{
    // Get number of rows in each table
    int n_T_htf_pars = T_htf_ind.nrows();
    int n_m_dot_pars = m_dot_ind.nrows();
    int n_T_amb_pars = T_amb_ind.nrows();

    // Put the low, design, and high ind values in vectors
    std::vector<double> v_T_htf_levels = std::vector<double>{ T_htf_low, T_htf_des, T_htf_high };
    std::vector<double> v_m_dot_levels = std::vector<double>{ m_dot_low, m_dot_des, m_dot_high };
    std::vector<double> v_T_amb_levels = std::vector<double>{ T_amb_low, T_amb_des, T_amb_high };

    size_t total_rows = 3 * (n_T_htf_pars + n_m_dot_pars + n_T_amb_pars);
    const int ncols = 7;

    combined.resize_fill(total_rows, ncols, std::numeric_limits<double>::quiet_NaN());

    for (int j = 0; j < v_m_dot_levels.size(); j++)
    {
        for (int i = 0; i < n_T_htf_pars; i++)
        {
            int r_comb = j * n_T_htf_pars + i;
            double m_dot = v_m_dot_levels[j];

            combined.set_value(T_htf_ind(i, 0), r_comb, C_ud_power_cycle::E_COL_T_HTF);			// Independent variable
            combined.set_value(m_dot, r_comb, C_ud_power_cycle::E_COL_M_DOT);						// Level variable
            combined.set_value(T_amb_des, r_comb, C_ud_power_cycle::E_COL_T_AMB);					// Constant variable

            combined.set_value(T_htf_ind(i, 3 * C_ud_power_cycle::i_W_dot_gross + 1 + j), r_comb, C_ud_power_cycle::E_COL_W_CYL);
            combined.set_value(T_htf_ind(i, 3 * C_ud_power_cycle::i_Q_dot_HTF + 1 + j), r_comb, C_ud_power_cycle::E_COL_Q_CYL);
            combined.set_value(T_htf_ind(i, 3 * C_ud_power_cycle::i_W_dot_cooling + 1 + j), r_comb, C_ud_power_cycle::E_COL_W_COOL);
            combined.set_value(T_htf_ind(i, 3 * C_ud_power_cycle::i_m_dot_water + 1 + j), r_comb, C_ud_power_cycle::E_COL_M_H2O);
        }
    }

    for (int j = 0; j < v_T_amb_levels.size(); j++)
    {
        for (int i = 0; i < n_m_dot_pars; i++)
        {
            int r_comb = n_T_htf_pars * v_m_dot_levels.size() + j * n_m_dot_pars + i;
            double T_amb = v_T_amb_levels[j];

            combined.set_value(m_dot_ind(i, 0), r_comb, C_ud_power_cycle::E_COL_M_DOT);		// Independent variable
            combined.set_value(T_amb, r_comb, C_ud_power_cycle::E_COL_T_AMB);					// Level variable
            combined.set_value(T_htf_des, r_comb, C_ud_power_cycle::E_COL_T_HTF);				// Constant variable

            combined.set_value(m_dot_ind(i, 3 * C_ud_power_cycle::i_W_dot_gross + 1 + j), r_comb, C_ud_power_cycle::E_COL_W_CYL);
            combined.set_value(m_dot_ind(i, 3 * C_ud_power_cycle::i_Q_dot_HTF + 1 + j), r_comb, C_ud_power_cycle::E_COL_Q_CYL);
            combined.set_value(m_dot_ind(i, 3 * C_ud_power_cycle::i_W_dot_cooling + 1 + j), r_comb, C_ud_power_cycle::E_COL_W_COOL);
            combined.set_value(m_dot_ind(i, 3 * C_ud_power_cycle::i_m_dot_water + 1 + j), r_comb, C_ud_power_cycle::E_COL_M_H2O);
        }
    }

    for (int j = 0; j < v_T_htf_levels.size(); j++)
    {
        for (int i = 0; i < n_T_amb_pars; i++)
        {
            int r_comb = n_T_htf_pars * v_m_dot_levels.size() + n_m_dot_pars * v_T_amb_levels.size() + j * n_T_amb_pars + i;
            double T_htf = v_T_htf_levels[j];

            combined.set_value(T_amb_ind(i, 0), r_comb, C_ud_power_cycle::E_COL_T_AMB);		// Independent variable
            combined.set_value(T_htf, r_comb, C_ud_power_cycle::E_COL_T_HTF);					// Level variable
            combined.set_value(m_dot_des, r_comb, C_ud_power_cycle::E_COL_M_DOT);				// Constant variable

            combined.set_value(T_amb_ind(i, 3 * C_ud_power_cycle::i_W_dot_gross + 1 + j), r_comb, C_ud_power_cycle::E_COL_W_CYL);
            combined.set_value(T_amb_ind(i, 3 * C_ud_power_cycle::i_Q_dot_HTF + 1 + j), r_comb, C_ud_power_cycle::E_COL_Q_CYL);
            combined.set_value(T_amb_ind(i, 3 * C_ud_power_cycle::i_W_dot_cooling + 1 + j), r_comb, C_ud_power_cycle::E_COL_W_COOL);
            combined.set_value(T_amb_ind(i, 3 * C_ud_power_cycle::i_m_dot_water + 1 + j), r_comb, C_ud_power_cycle::E_COL_M_H2O);
        }
    }

    return 0;
}
