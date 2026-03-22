/*
BSD 3-Clause License

Copyright Alliance for Energy Innovation, LLC. See also https://github.com/NREL/ssc/blob/develop/LICENSE


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


#include <gtest/gtest.h>
//#include "tcsmolten_salt_defaults.h"
#include "csp_common_test.h"
#include "vs_google_test_explorer_namespace.h"
#include <unordered_map>

//#include "../input_cases/code_generator_utilities.h"

namespace sco2_tests {}
using namespace sco2_tests;

double kTol = 0.01;

// Helper functions

ssc_data_t get_default_sco2_pars()
{
    ssc_data_t data = ssc_data_create();

    // Basic design parameters
    ssc_data_set_number(data, "htf", 17);  // Solar salt
    ssc_data_set_number(data, "T_htf_hot_des", 670.0);  // [C] HTF design hot temperature
    ssc_data_set_number(data, "dT_PHX_hot_approach", 20.0);  // [C/K] Temperature difference between hot HTF and turbine inlet
    ssc_data_set_number(data, "T_amb_des", 35.0);  // [C] Ambient temperature at design
    ssc_data_set_number(data, "dT_mc_approach", 6.0);  // [C] Temperature difference between main compressor CO2 inlet and ambient air
    ssc_data_set_number(data, "site_elevation", 588);  // [m] Elevation
    ssc_data_set_number(data, "W_dot_net_des", 50.0);  // [MWe] Design cycle power output

    // Cycle configuration
    ssc_data_set_number(data, "cycle_config", 1);  // [1] = RC, [2] = PC
    ssc_data_set_number(data, "design_method", 2);  // [-] 1 = specify efficiency, 2 = specify total recup UA, 3 = Specify each recup design
    ssc_data_set_number(data, "eta_thermal_des", 0.44);  // [-] Target power cycle thermal efficiency
    ssc_data_set_number(data, "UA_recup_tot_des", 15000.0);  // [kW/K] Total recuperator UA (15 * 1000 * 50.0 / 50.0)

    // Pressure and recompression settings
    ssc_data_set_number(data, "is_recomp_ok", 1);  // 1 = Yes, 0 = simple cycle only
    ssc_data_set_number(data, "is_P_high_fixed", 1);  // 0 = No, optimize. 1 = Yes
    ssc_data_set_number(data, "is_PR_fixed", 0);  // 0 = No, >0 = fixed pressure ratio
    ssc_data_set_number(data, "is_IP_fixed", 0);  // 0 = No, >0 = fixed HP-IP pressure ratio

    // Convergence criteria
    ssc_data_set_number(data, "rel_tol", 3);  // [-] Solver relative tolerance exponent

    // Component efficiencies
    ssc_data_set_number(data, "eta_isen_mc", 0.85);  // [-] Main compressor isentropic efficiency
    ssc_data_set_number(data, "eta_isen_rc", 0.85);  // [-] Recompressor isentropic efficiency
    ssc_data_set_number(data, "eta_isen_pc", 0.85);  // [-] Precompressor isentropic efficiency
    ssc_data_set_number(data, "eta_isen_t", 0.90);  // [-] Turbine isentropic efficiency

    // Pressure limits
    ssc_data_set_number(data, "P_high_limit", 25);  // [MPa] Cycle high pressure limit

    // Recuperators
    double eff_max = 1.0;
    double deltaP_recup_HP = 0.0056;    // [-] = 0.14[MPa]/25[MPa]
    double deltaP_recup_LP = 0.0311;    // [-] = 0.28[MPa]/9[MPa]

    // LTR (Low Temperature Recuperator)
    ssc_data_set_number(data, "LTR_design_code", 3);  // 1 = UA, 2 = min dT, 3 = effectiveness
    ssc_data_set_number(data, "LTR_UA_des_in", 2200.0);  // [kW/K]
    ssc_data_set_number(data, "LTR_min_dT_des_in", 12.0);  // [C]
    ssc_data_set_number(data, "LTR_eff_des_in", 0.895);  // [-]
    ssc_data_set_number(data, "LT_recup_eff_max", eff_max);  // [-]
    ssc_data_set_number(data, "LTR_LP_deltaP_des_in", deltaP_recup_LP);  // [-]
    ssc_data_set_number(data, "LTR_HP_deltaP_des_in", deltaP_recup_HP);  // [-]

    // HTR (High Temperature Recuperator)
    ssc_data_set_number(data, "HTR_design_code", 3);  // 1 = UA, 2 = min dT, 3 = effectiveness
    ssc_data_set_number(data, "HTR_UA_des_in", 2800.0);  // [kW/K]
    ssc_data_set_number(data, "HTR_min_dT_des_in", 19.2);  // [C]
    ssc_data_set_number(data, "HTR_eff_des_in", 0.945);  // [-]
    ssc_data_set_number(data, "HT_recup_eff_max", eff_max);  // [-]
    ssc_data_set_number(data, "HTR_LP_deltaP_des_in", deltaP_recup_LP);  // [-]
    ssc_data_set_number(data, "HTR_HP_deltaP_des_in", deltaP_recup_HP);  // [-]

    // PHX (Primary Heat Exchanger)
    ssc_data_set_number(data, "PHX_co2_deltaP_des_in", deltaP_recup_HP);  // [-]
    ssc_data_set_number(data, "dT_PHX_cold_approach", 20);  // [C/K]

    // Air Cooler
    ssc_data_set_number(data, "deltaP_cooler_frac", 0.005);  // [-]
    ssc_data_set_number(data, "fan_power_frac", 0.02);  // [-]

    return data;
}

void check_result_vals(CmodUnderTest& sco2, std::unordered_map<std::string, double> result_map)
{
    for (const auto& result : result_map)
    {
        std::string key = result.first;
        double result_expected = result.second;
        double result_actual = sco2.GetOutput(key);

        ASSERT_NEAR(result_expected, result_actual, kTol);
    }

    return;
}

//========Tests===================================================================================
NAMESPACE_TEST(sco2_tests, SCO2Cycle, Parametrics)
{
    
    ssc_data_t data = ssc_data_create();

    ssc_data_set_number(data, "t_amb_des", 26);
    ssc_data_set_number(data, "dt_mc_approach", 6);
    ssc_data_set_number(data, "t_htf_hot_des", 720);
    ssc_number_t p_od_cases[12] = { 720, 1, 26, 1, 1, 1, 720, 1, 20, 1, 1, 1 };

    ssc_data_set_number(data, "n_nodes_air_cooler_pass", 10);
    ssc_data_set_number(data, "htf", 6);
    ssc_data_set_number(data, "design_method", 3);
    ssc_data_set_number(data, "fan_power_frac", 0.02);
    ssc_data_set_number(data, "deltap_counterhx_frac", -1);
    ssc_data_set_number(data, "w_dot_net_des", 50);
    ssc_data_set_number(data, "ltr_ua_des_in", -1);
    ssc_data_set_number(data, "dt_phx_hot_approach", 20);
    ssc_data_set_number(data, "site_elevation", 588);
    ssc_data_set_number(data, "ua_recup_tot_des", -1);
    ssc_data_set_number(data, "eta_thermal_des", -1);
    ssc_data_set_number(data, "rel_tol", 3);
    ssc_data_set_number(data, "ltr_design_code", 2);
    ssc_data_set_number(data, "is_gen_od_polynomials", 0);
    ssc_data_set_number(data, "ltr_min_dt_des_in", 10);
    ssc_data_set_number(data, "lt_recup_eff_max", 1);
    ssc_data_set_number(data, "ltr_eff_des_in", -1);
    ssc_data_set_number(data, "p_high_limit", 25);
    ssc_data_set_number(data, "eta_isen_mc", 0.84999999999999998);
    ssc_data_set_number(data, "ltr_lp_deltap_des_in", 0.031099999999999999);
    ssc_data_set_number(data, "ltr_hp_deltap_des_in", 0.0055999999999999999);
    ssc_data_set_number(data, "htr_design_code", 2);
    ssc_data_set_number(data, "htr_ua_des_in", -1);
    ssc_data_set_number(data, "od_rel_tol", 3);
    ssc_data_set_number(data, "htr_min_dt_des_in", 10);
    ssc_data_set_number(data, "od_opt_objective", 0);
    ssc_data_set_number(data, "ht_recup_eff_max", 1);
    ssc_data_set_number(data, "htr_eff_des_in", -1);
    ssc_data_set_number(data, "htr_lp_deltap_des_in", 0.031099999999999999);
    ssc_data_set_number(data, "htr_hp_deltap_des_in", 0.0055999999999999999);
    
    ssc_data_set_matrix(data, "od_cases", p_od_cases, 2, 6);
    ssc_data_set_number(data, "cycle_config", 1);
    ssc_data_set_number(data, "des_objective", 1);
    ssc_data_set_number(data, "is_recomp_ok", 1);
    ssc_data_set_number(data, "is_p_high_fixed", 1);
    ssc_data_set_number(data, "is_pr_fixed", 0);
    ssc_data_set_number(data, "od_t_t_in_mode", 0);
    ssc_data_set_number(data, "is_ip_fixed", 0);
    ssc_data_set_number(data, "min_phx_deltat", 1000);
    ssc_data_set_number(data, "ltr_od_model", 1);
    ssc_data_set_number(data, "deltap_cooler_frac", 0.0050000000000000001);
    ssc_data_set_number(data, "eta_isen_rc", 0.84999999999999998);
    ssc_data_set_number(data, "eta_isen_pc", 0.84999999999999998);
    ssc_data_set_number(data, "eta_isen_t", 0.90000000000000002);
    ssc_data_set_number(data, "phx_co2_deltap_des_in", 0.0055999999999999999);
    ssc_data_set_number(data, "mc_comp_type", 1);
    ssc_data_set_number(data, "dt_phx_cold_approach", 20);
    ssc_data_set_number(data, "ltr_n_sub_hx", 10);
    ssc_data_set_number(data, "htr_n_sub_hx", 10);
    ssc_data_set_number(data, "htr_od_model", 1);
    ssc_data_set_number(data, "phx_n_sub_hx", 10);
    ssc_data_set_number(data, "phx_od_model", 1);
    ssc_data_set_number(data, "is_design_air_cooler", 1);
    ssc_data_set_number(data, "eta_air_cooler_fan", 0.5);
    
    CmodUnderTest sco2 = CmodUnderTest("sco2_csp_system", data);
    
    int errors = sco2.RunModule();
    EXPECT_FALSE(errors);
    
    if (!errors) {
        EXPECT_NEAR_FRAC(sco2.GetOutput("T_htf_cold_des"), 529.6897, kErrorToleranceLo);
        EXPECT_NEAR_FRAC(sco2.GetOutput("eta_thermal_calc"), 0.5071197, kErrorToleranceLo);
        EXPECT_NEAR_FRAC(sco2.GetOutput("m_dot_htf_des"), 513.344, kErrorToleranceLo);
        EXPECT_NEAR_FRAC(sco2.GetOutput("m_dot_co2_full"), 410.528, kErrorToleranceLo);
        EXPECT_NEAR_FRAC(sco2.GetOutput("P_comp_in"), 7.67490, kErrorToleranceLo);
        EXPECT_NEAR_FRAC(sco2.GetOutput("cycle_cost"), 61.122, kErrorToleranceLo);

        std::vector<ssc_number_t> eta_thermal_od_exp{ 0.50689, 0.507197 };
        EXPECT_FLOATS_NEARLY_EQ(sco2.GetOutputVector("eta_thermal_od"), eta_thermal_od_exp, kErrorToleranceLo*eta_thermal_od_exp[0]);

        std::vector<ssc_number_t> P_comp_in_od_exp{ 7.68424, 7.64808 };
        EXPECT_FLOATS_NEARLY_EQ(sco2.GetOutputVector("P_comp_in_od"), P_comp_in_od_exp, kErrorToleranceLo*P_comp_in_od_exp[0]);

        std::vector<ssc_number_t> W_dot_net_less_cooling_od_exp{ 48.9136, 49.98451 };
        EXPECT_FLOATS_NEARLY_EQ(sco2.GetOutputVector("W_dot_net_less_cooling_od"), W_dot_net_less_cooling_od_exp, kErrorToleranceLo*W_dot_net_less_cooling_od_exp[0]);

    }
    
}


// Design method 2 (optimize with fixed total UA)
NAMESPACE_TEST(sco2_design_tests, SCO2Design, recompression_default)
{
    // This test is design method 2, maximizing efficiency using a fixed total UA,
    // optimizing min pressure, UA split, and flow fractions.

    ssc_data_t data = get_default_sco2_pars();
    CmodUnderTest sco2 = CmodUnderTest("sco2_csp_system", data);
    int errors = sco2.RunModule();

    // Check for errors
    ASSERT_TRUE(errors == 0);

    // Define known results
    std::unordered_map<std::string, double> result_dict;
    result_dict["eta_thermal_calc"] = 0.46326288645840497;
    result_dict["T_htf_cold_des"] = 509.7489245863944;
    result_dict["cycle_cost"] = 62.85323351704139;

    // Check expected vs actual results
    check_result_vals(sco2, result_dict);

    // Validate back work ratio calculation
    double comp_W_dot = sco2.GetOutput("mc_W_dot") + sco2.GetOutput("rc_W_dot");
    double t_W_dot = sco2.GetOutput("t_W_dot");
    double back_work_ratio = comp_W_dot / t_W_dot;

    double back_work_actual = sco2.GetOutput("back_work_ratio");
    ASSERT_NEAR(back_work_ratio, back_work_actual, kTol);
}

NAMESPACE_TEST(sco2_design_tests, SCO2Design, simple_default)
{
    // This test is design method 2, maximizing efficiency using a fixed total UA,
    // optimizing min pressure, UA split, and flow fractions.

    // Get default parameters
    ssc_data_t data = get_default_sco2_pars();

    ssc_data_set_string(data, "coolprop_fluid", "CarbonDioxide");
    //ssc_data_set_string(data, "coolprop_backend", "TTSE");

    // Set test specific values
    ssc_data_set_number(data, "is_recomp_ok", 0);

    // Call cmod
    CmodUnderTest sco2 = CmodUnderTest("sco2_csp_system", data);
    int errors = sco2.RunModule();

    // Check for errors
    ASSERT_TRUE(errors == 0);

    // Define known results
    std::unordered_map<std::string, double> result_dict;
    result_dict["eta_thermal_calc"] = 0.43185849042562524;
    result_dict["T_htf_cold_des"] = 466.8855038441511;
    result_dict["cycle_cost"] = 57.323164238332325;

    // Check expected vs actual results
    check_result_vals(sco2, result_dict);

    // Validate back work ratio calculation
    double comp_W_dot = sco2.GetOutput("mc_W_dot");
    double t_W_dot = sco2.GetOutput("t_W_dot");
    double back_work_ratio = comp_W_dot / t_W_dot;

    double back_work_actual = sco2.GetOutput("back_work_ratio");
    ASSERT_NEAR(back_work_ratio, back_work_actual, kTol);
}

NAMESPACE_TEST(sco2_design_tests, SCO2Design, partial_default)
{
    // This test is design method 2, maximizing efficiency using a fixed total UA,
    // optimizing min pressure, UA split, and flow fractions.

    // Get default parameters
    ssc_data_t data = get_default_sco2_pars();

    // Set test specific values
    ssc_data_set_number(data, "cycle_config", 2);

    // Call cmod
    CmodUnderTest sco2 = CmodUnderTest("sco2_csp_system", data);
    int errors = sco2.RunModule();

    // Check for errors
    ASSERT_TRUE(errors == 0);

    // Define known results
    std::unordered_map<std::string, double> result_dict;
    result_dict["eta_thermal_calc"] = 0.4680242988429887;
    result_dict["T_htf_cold_des"] = 454.4556200450701;
    result_dict["cycle_cost"] = 66.2574259286688;

    // Check expected vs actual results
    check_result_vals(sco2, result_dict);

    // Validate back work ratio calculation
    double comp_W_dot = sco2.GetOutput("mc_W_dot") + sco2.GetOutput("rc_W_dot") + sco2.GetOutput("pc_W_dot");
    double t_W_dot = sco2.GetOutput("t_W_dot");
    double back_work_ratio = comp_W_dot / t_W_dot;

    double back_work_actual = sco2.GetOutput("back_work_ratio");
    ASSERT_NEAR(back_work_ratio, back_work_actual, kTol);
}

NAMESPACE_TEST(sco2_design_tests, SCO2Design, htrbp_default)
{
    // This test is design method 2, maximizing efficiency using a fixed total UA,
    // optimizing min pressure, UA split, and flow fractions.
    // ALSO, htr bp is varying the bp frac to hit the target outlet temperature

    // Get default parameters
    ssc_data_t data = get_default_sco2_pars();

    // Set test specific values
    ssc_data_set_number(data, "cycle_config", 3);
    ssc_data_set_number(data, "is_bypass_ok", 1);
    ssc_data_set_number(data, "T_bypass_target", 400);
    ssc_data_set_number(data, "T_target_is_HTF", 1);
    ssc_data_set_number(data, "deltaT_bypass", 0);
    ssc_data_set_number(data, "set_HTF_mdot", 0);

    // Call cmod
    CmodUnderTest sco2 = CmodUnderTest("sco2_csp_system", data);
    int errors = sco2.RunModule();

    // Check for errors
    ASSERT_TRUE(errors == 0);

    // Define known results
    std::unordered_map<std::string, double> result_dict;
    result_dict["eta_thermal_calc"] = 0.33776551440239994;
    result_dict["T_htf_cold_des"] = 400.0;
    result_dict["cycle_cost"] = 68.82623980549134;

    // Check expected vs actual results
    check_result_vals(sco2, result_dict);

    // Validate back work ratio calculation
    double comp_W_dot = sco2.GetOutput("mc_W_dot") + sco2.GetOutput("rc_W_dot");
    double t_W_dot = sco2.GetOutput("t_W_dot");
    double back_work_ratio = comp_W_dot / t_W_dot;

    double back_work_actual = sco2.GetOutput("back_work_ratio");
    ASSERT_NEAR(back_work_ratio, back_work_actual, kTol);
}

NAMESPACE_TEST(sco2_design_tests, SCO2Design, tsf_default)
{
    // This test is design method 2, maximizing efficiency using a fixed total UA,
    // optimizing min pressure, UA split, and flow fractions.

    // Get default parameters
    ssc_data_t data = get_default_sco2_pars();

    // Set test specific values
    ssc_data_set_number(data, "cycle_config", 4);
    ssc_data_set_number(data, "is_turbine_split_ok", 1);
    ssc_data_set_number(data, "eta_isen_t2", 0.90);

    // Call cmod
    CmodUnderTest sco2 = CmodUnderTest("sco2_csp_system", data);
    int errors = sco2.RunModule();

    // Check for errors
    ASSERT_TRUE(errors == 0);

    // Define known results
    std::unordered_map<std::string, double> result_dict;
    result_dict["eta_thermal_calc"] = 0.40642770633106834;
    result_dict["T_htf_cold_des"] = 306.45863972890197;
    result_dict["cycle_cost"] = 60.89798090322589;

    // Check expected vs actual results
    check_result_vals(sco2, result_dict);

    // Validate back work ratio calculation
    double comp_W_dot = sco2.GetOutput("mc_W_dot");
    double t_W_dot = sco2.GetOutput("t_W_dot") + sco2.GetOutput("t2_W_dot");
    double back_work_ratio = comp_W_dot / t_W_dot;

    double back_work_actual = sco2.GetOutput("back_work_ratio");
    ASSERT_NEAR(back_work_ratio, back_work_actual, kTol);
}

// Design method 1 (hit target eta by varying total UA)
NAMESPACE_TEST(sco2_design_tests, SCO2Design, htrbp_des1)
{
    // Design method 2, vary total UA to hit target eta,
    // AND vary bypass frac to hit target outlet temp

    // Get default parameters
    ssc_data_t data = get_default_sco2_pars();

    // Set test specific values
    ssc_data_set_number(data, "cycle_config", 3);
    ssc_data_set_number(data, "design_method", 1);          // Hit target eta, vary total UA
    ssc_data_set_number(data, "eta_thermal_des", 0.33);     // Target eta
    ssc_data_set_number(data, "is_bypass_ok", 1);
    ssc_data_set_number(data, "T_bypass_target", 400);
    ssc_data_set_number(data, "T_target_is_HTF", 1);
    ssc_data_set_number(data, "deltaT_bypass", 0);
    ssc_data_set_number(data, "set_HTF_mdot", 0);

    // Call cmod
    CmodUnderTest sco2 = CmodUnderTest("sco2_csp_system", data);
    int errors = sco2.RunModule();

    // Check for errors
    ASSERT_TRUE(errors == 0);

    // Define known results
    std::unordered_map<std::string, double> result_dict;
    result_dict["eta_thermal_calc"] = 0.3377427765707046;
    result_dict["T_htf_cold_des"] = 400.00000000000114;
    result_dict["cycle_cost"] = 92.73527550752384;

    // Check expected vs actual results
    check_result_vals(sco2, result_dict);

    // Validate back work ratio calculation
    double comp_W_dot = sco2.GetOutput("mc_W_dot") + sco2.GetOutput("rc_W_dot");
    double t_W_dot = sco2.GetOutput("t_W_dot");
    double back_work_ratio = comp_W_dot / t_W_dot;

    double back_work_actual = sco2.GetOutput("back_work_ratio");
    ASSERT_NEAR(back_work_ratio, back_work_actual, kTol);
}


// Fail tests
NAMESPACE_TEST(sco2_design_tests, SCO2Design, tsf_des1_fail)
{
    // This test purposefully fails, by trying to run TSF with design method 1

    // Get default parameters
    ssc_data_t data = get_default_sco2_pars();

    // Assign TSF specific pars
    ssc_data_set_number(data, "cycle_config", 4);
    ssc_data_set_number(data, "is_turbine_split_ok", 1);
    ssc_data_set_number(data, "eta_isen_t2", 0.90);

    // Set design method 1 (which TSF does not support)
    ssc_data_set_number(data, "design_method", 1);

    // Run cmod
    CmodUnderTest sco2 = CmodUnderTest("sco2_csp_system", data);
    int errors = sco2.RunModule();

    // Expect to have errors
    ASSERT_FALSE(errors == 0);
}
