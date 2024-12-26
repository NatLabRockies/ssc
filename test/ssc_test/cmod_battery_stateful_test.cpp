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


#include <chrono>

#include "cmod_battery_stateful_test.h"

typedef std::chrono::high_resolution_clock Clock;

TEST_F(CMBatteryStatefulIntegration_cmod_battery_stateful, TestStep) {
    CreateModel(1);
    double last_idx, I, SOC, V, P, Q, I_d, I_c, P_d, P_c;

    ssc_module_exec(mod, data);
    ssc_data_get_number(data, "last_idx", &last_idx);
    ssc_data_get_number(data, "V", &V);
    ssc_data_get_number(data, "P", &P);
    ssc_data_get_number(data, "Q", &Q);
    ssc_data_get_number(data, "I", &I);
    ssc_data_get_number(data, "I_dischargeable", &I_d);
    ssc_data_get_number(data, "I_chargeable", &I_c);
    ssc_data_get_number(data, "P_dischargeable", &P_d);
    ssc_data_get_number(data, "P_chargeable", &P_c);
    ssc_data_get_number(data, "SOC", &SOC);
    EXPECT_EQ(last_idx, 1);
    EXPECT_NEAR(V, 550.81, 1e-2);
    EXPECT_NEAR(P, 0.549, 1e-2);
    EXPECT_NEAR(Q, 9.125, 1e-2);
    EXPECT_NEAR(I, 1, 1e-2);
    EXPECT_NEAR(I_d, 8.21, 1e-2);
    EXPECT_NEAR(I_c, -9.34, 1e-2);
    EXPECT_NEAR(P_d, 3.80, 1e-2);
    EXPECT_NEAR(P_c, -5.32, 1e-2);
    EXPECT_NEAR(SOC, 46.94, 1e-2);

    // make a copy
    auto js = ssc_data_to_json(data);
    auto copy = json_to_ssc_data(js);

    delete js;
    
    ssc_module_exec(mod, data);
    ssc_data_get_number(data, "last_idx", &last_idx);
    ssc_data_get_number(data, "V", &V);
    ssc_data_get_number(data, "P", &P);
    ssc_data_get_number(data, "Q", &Q);
    ssc_data_get_number(data, "I", &I);
    ssc_data_get_number(data, "I_dischargeable", &I_d);
    ssc_data_get_number(data, "I_chargeable", &I_c);
    ssc_data_get_number(data, "P_dischargeable", &P_d);
    ssc_data_get_number(data, "P_chargeable", &P_c);
    ssc_data_get_number(data, "SOC", &SOC);
    EXPECT_EQ(last_idx, 2);
    EXPECT_NEAR(V, 548.3, 1e-2);
    EXPECT_NEAR(P, 0.546, 1e-2);
    EXPECT_NEAR(Q, 8.125, 1e-2);
    EXPECT_NEAR(I, 1, 1e-2);
    EXPECT_NEAR(I_d, 7.31, 1e-2);
    EXPECT_NEAR(I_c, -10.34, 1e-2);
    EXPECT_NEAR(P_d, 3.34, 1e-2);
    EXPECT_NEAR(P_c, -5.89, 1e-2);
    EXPECT_NEAR(SOC, 41.79, 1e-2);

    // run the copy, should end up in same place
    ssc_module_exec(mod, copy);
    ssc_data_get_number(copy, "last_idx", &last_idx);
    ssc_data_get_number(copy, "V", &V);
    ssc_data_get_number(copy, "P", &P);
    ssc_data_get_number(copy, "Q", &Q);
    ssc_data_get_number(copy, "I", &I);
    ssc_data_get_number(copy, "I_dischargeable", &I_d);
    ssc_data_get_number(copy, "I_chargeable", &I_c);
    ssc_data_get_number(copy, "P_dischargeable", &P_d);
    ssc_data_get_number(copy, "P_chargeable", &P_c);
    ssc_data_get_number(copy, "SOC", &SOC);
    EXPECT_EQ(last_idx, 2);
    EXPECT_NEAR(V, 548.30, 1e-2);
    EXPECT_NEAR(P, 0.546, 1e-2);
    EXPECT_NEAR(Q, 8.125, 1e-2);
    EXPECT_NEAR(I, 1, 1e-2);
    EXPECT_NEAR(I_d, 7.31, 1e-2);
    EXPECT_NEAR(I_c, -10.34, 1e-2);
    EXPECT_NEAR(P_d, 3.34, 1e-2);
    EXPECT_NEAR(P_c, -5.89, 1e-2);
    EXPECT_NEAR(SOC, 41.79, 1e-2);
    
    ssc_data_free(copy);
}

TEST_F(CMBatteryStatefulIntegration_cmod_battery_stateful, SubMinute) {
    CreateModel(1. / 360);
    double last_idx, current, SOC, V, P;

    ssc_module_exec(mod, data);
    ssc_data_get_number(data, "last_idx", &last_idx);
    ssc_data_get_number(data, "I", &current);
    ssc_data_get_number(data, "P", &P);
    ssc_data_get_number(data, "V", &V);
    ssc_data_get_number(data, "SOC", &SOC);
    EXPECT_EQ(last_idx, 1);
    EXPECT_NEAR(current, 1, 1e-2);
    EXPECT_NEAR(P, 0.551, 0.02);
    EXPECT_NEAR(V, 552.87, 1e-2);
    EXPECT_NEAR(SOC, 52.07, 1e-2);

    // make a copy
    auto js = ssc_data_to_json(data);
    auto copy = json_to_ssc_data(js);

    delete js;
    
    ssc_module_exec(mod, data);
    ssc_data_get_number(data, "last_idx", &last_idx);
    ssc_data_get_number(data, "I", &current);
    ssc_data_get_number(data, "P", &P);
    ssc_data_get_number(data, "V", &V);
    ssc_data_get_number(data, "SOC", &SOC);
    EXPECT_EQ(last_idx, 2);
    EXPECT_NEAR(current, 1, 1e-2);
    EXPECT_NEAR(P, 0.551, 1e-2);
    EXPECT_NEAR(V, 552.87, 1e-2);
    EXPECT_NEAR(SOC, 52.05, 1e-2);

    // run the copy, should end up in same place
    ssc_module_exec(mod, copy);
    ssc_data_get_number(data, "last_idx", &last_idx);
    ssc_data_get_number(data, "I", &current);
    ssc_data_get_number(data, "P", &P);
    ssc_data_get_number(data, "V", &V);
    ssc_data_get_number(data, "SOC", &SOC);
    EXPECT_EQ(last_idx, 2);
    EXPECT_NEAR(current, 1, 1e-2);
    EXPECT_NEAR(P, 0.551, 1e-2);
    EXPECT_NEAR(V, 552.87, 1e-2);
    EXPECT_NEAR(SOC, 52.05, 1e-2);
    
    ssc_data_free(copy);
}

TEST_F(CMBatteryStatefulIntegration_cmod_battery_stateful, ReadJson) {
    CreateModel(1./60.);
    std::string js = "{\"control_mode\": 1.0, \"dt_hr\": 0.016666666666666666, \"input_power\": -1608.9696000000001, \"run_sequentially\": 1.0, \"C_rate\": 0.2, \"Qexp\": 0.04, \"Qfull\": 2.25, \"Qnom\": 2.0, \"Vexp\": 4.05, \"Vfull\": 4.1, \"Vnom\": 3.4, \"Vcut\": 2.706, \"Vnom_default\": 3.6, \"life_model\": 0, \"calendar_a\": 0.003, \"calendar_b\": -7280.0, \"calendar_c\": 930.0, \"calendar_choice\": 1.0, \"calendar_matrix\": [[-3.1e+231]], \"calendar_q0\": 1.02, \"chem\": 1.0, \"cycling_matrix\": [[20.0, 0.0, 100.0], [20.0, 5000.0, 80.0], [20.0, 10000.0, 60.0], [80.0, 0.0, 100.0], [80.0, 1000.0, 80.0], [80.0, 2000.0, 60.0]], \"initial_SOC\": 50.0, \"maximum_SOC\": 90.0, \"minimum_SOC\": 10.0, \"resistance\": 0.0002, \"voltage_choice\": 0.0, \"Cp\": 1500.0, \"T_room_init\": 20.0, \"cap_vs_temp\": [[-10.0, 60.0], [0.0, 80.0], [25.0, 100.0], [40.0, 100.0]], \"h\": 7.5, \"loss_choice\": 0.0, \"mass\": 98980.0, \"monthly_charge_loss\": [0.0], \"monthly_discharge_loss\": [0.0], \"monthly_idle_loss\": [0.0], \"nominal_energy\": 10000.0, \"nominal_voltage\": 500.0, \"replacement_capacity\": 0.0, \"replacement_option\": 0.0, \"replacement_schedule\": [-1.4916681462400413e-154], \"replacement_schedule_percent\": [-1.4916681462400413e-154], \"schedule_loss\": [0.0], \"surface_area\": 2071.0, \"I\": 0.0, \"I_chargeable\": -939699.5036648216, \"I_dischargeable\": 0.0, \"P\": 0.0, \"P_chargeable\": -538310.3372051578, \"P_dischargeable\": 0.0, \"Q\": 1957.713280026735, \"Q_max\": 19577.07963826714, \"SOC\": 10.000027155224982, \"T_batt\": 54.667972372468775, \"T_room\": 16.5319492435796, \"V\": 0.0, \"heat_dissipated\": 592184.2210545398, \"indices_replaced\": [0.0], \"last_idx\": 1261.0, \"loss_kw\": 0.0, \"n_replacements\": 0.0, \"I_loss\": 0.0, \"SOC_prev\": 10.000026847325115, \"T_batt_prev\": 54.54844020525578, \"average_range\": 0.0, \"cell_current\": 0.0, \"cell_voltage\": 0.0, \"chargeChange\": 0.0, \"charge_mode\": 1.0, \"n_cycles\": 0.0, \"prev_charge\": 2.0, \"q0\": 1957.713280026735, \"q1_0\": 0.0, \"q2\": 0.0, \"q2_0\": 0.0, \"q_relative\": 97.96131821295074, \"q_relative_calendar\": 97.96131821295074, \"q_relative_cycle\": 100.0, \"q_relative_thermal\": 100.0, \"qmax_lifetime\": 19577.07963826714, \"qmax_thermal\": 19577.13280026734, \"qn\": 0.0, \"rainflow_Xlt\": 0.0, \"rainflow_Ylt\": 0.040386817870492635, \"rainflow_jlt\": 2.0, \"rainflow_peaks\": [0.0, 47.9929307855364, 10.000000000000014], \"cycle_range\": 0.0, \"day_age_of_battery\" : 3, \"dq_relative_calendar_old\" : 0.001}";

    auto copy = json_to_ssc_data(js.c_str());
    double P, V, SOC;

    EXPECT_TRUE(ssc_stateful_module_setup(mod, copy));
    EXPECT_TRUE(ssc_module_exec(mod, copy));

    ssc_data_get_number(copy, "P", &P);
    ssc_data_get_number(copy, "V", &V);
    ssc_data_get_number(copy, "SOC", &SOC);

    int length;
    ssc_number_t* rainflow_peaks = ssc_data_get_array(copy, "rainflow_peaks", &length);
    EXPECT_EQ(length, 1);

    EXPECT_TRUE(1);     // means the variable retrievals all succeeded
    
    ssc_data_free(copy);

}

TEST_F(CMBatteryStatefulIntegration_cmod_battery_stateful, RunCurrentControl) {
    double dt_hour = 1.0 / 360;

    CreateKokamModel(dt_hour);

    double range, avg_range, n_cycles, q_max, q_rel;

    std::vector<double> currents = getCurrentData();

    EXPECT_EQ(currents.size(), 2000);

    for (double current : currents) {
        ssc_data_set_number(data, "input_current", current);
        ssc_module_exec(mod, data);
    }

    ssc_data_get_number(data, "cycle_range", &range);
    ssc_data_get_number(data, "average_range", &avg_range);
    ssc_data_get_number(data, "n_cycles", &n_cycles);
    ssc_data_get_number(data, "Q_max", &q_max);
    ssc_data_get_number(data, "q_relative", &q_rel);

    EXPECT_NEAR(range, 75.330, 0.01);
    EXPECT_NEAR(avg_range, 61.535, 0.01);
    EXPECT_NEAR(n_cycles, 3.0, 0.01);
    EXPECT_NEAR(q_max, 75.56, 0.01);
    EXPECT_NEAR(q_rel, 100.0, 0.01);
}


TEST_F(CMBatteryStatefulIntegration_cmod_battery_stateful, RunCurrentControlLMOLTO) {
    double dt_hour = 1.0 / 360;

    CreateLMOLTOModel(dt_hour);

    double range, avg_range, n_cycles, q_max, q_rel;

    std::vector<double> currents = getCurrentData();

    EXPECT_EQ(currents.size(), 2000);

    for (double current : currents) {
        ssc_data_set_number(data, "input_current", current);
        ssc_module_exec(mod, data);
    }

    ssc_data_get_number(data, "cycle_range", &range);
    ssc_data_get_number(data, "average_range", &avg_range);
    ssc_data_get_number(data, "n_cycles", &n_cycles);
    ssc_data_get_number(data, "Q_max", &q_max);
    ssc_data_get_number(data, "q_relative", &q_rel);

    EXPECT_NEAR(range, 75.330, 0.01);
    EXPECT_NEAR(avg_range, 61.535, 0.01);
    EXPECT_NEAR(n_cycles, 3.0, 0.01);
    EXPECT_NEAR(q_max, 75.56, 0.01);
    EXPECT_NEAR(q_rel, 100.0, 0.01);
}

TEST_F(CMBatteryStatefulIntegration_cmod_battery_stateful, AdaptiveTimestep) {
    CreateModel(1.);

    double power = 1;
    ssc_data_set_number(data, "control_mode", 1);
    ssc_data_set_number(data, "input_power", power);

    // create adaptive
    var_table data_copy;
    data_copy = *static_cast<var_table*>(data);
    ssc_data_set_number(&data_copy, "input_power", power);
    auto adaptive_batt = ssc_module_create("battery_stateful");
    ssc_stateful_module_setup(adaptive_batt, &data_copy);

    double P, hourly_E = 0, adaptive_E = 0;

    // run both at hourly
    ssc_module_exec(mod, data);
    ssc_data_get_number(data, "P", &P);
    hourly_E += P;

    ssc_module_exec(adaptive_batt, &data_copy);
    ssc_data_get_number(&data_copy, "P", &P);
    adaptive_E += P;

    // run at hourly and adaptive at 15 min
    ssc_module_exec(mod, data);
    ssc_data_get_number(data, "P", &P);
    hourly_E += P;

    size_t steps_per_hour = 4;
    ssc_data_set_number(&data_copy, "dt_hr", 1. / (double)steps_per_hour);
    for (size_t i = 0; i < steps_per_hour; i++) {
        ssc_module_exec(adaptive_batt, &data_copy);
        ssc_data_get_number(&data_copy, "P", &P);
        adaptive_E += P / (double)steps_per_hour;
        auto js = ssc_data_to_json(data);
        auto js_adaptive = ssc_data_to_json(&data_copy);
        printf("%s\n", js);
        printf("%s\n", js_adaptive);
        delete js;
        delete js_adaptive;

    }

    // run both at hourly
    ssc_module_exec(mod, data);
    ssc_data_get_number(data, "P", &P);
    hourly_E += P;

    ssc_data_set_number(&data_copy, "dt_hr", 1.);
    ssc_module_exec(adaptive_batt, &data_copy);
    ssc_data_get_number(&data_copy, "P", &P);
    adaptive_E += P;

    auto js = ssc_data_to_json(data);
    auto js_adaptive = ssc_data_to_json(&data_copy);
    printf("%s\n", js);
    printf("%s\n", js_adaptive);
    delete js;
    delete js_adaptive;
    
    double hourly_SOC, adaptive_SOC;
    ssc_data_get_number(data, "SOC", &hourly_SOC);
    ssc_data_get_number(&data_copy, "SOC", &adaptive_SOC);

    EXPECT_NEAR(hourly_E, 2.995, 1e-3);
    EXPECT_NEAR(adaptive_E, 2.995, 1e-3);
    EXPECT_NEAR(hourly_SOC, 23.614, 1e-3);
    EXPECT_NEAR(adaptive_SOC, 23.657, 1e-3);

    ssc_module_free(adaptive_batt);
}

TEST_F(CMBatteryStatefulIntegration_cmod_battery_stateful, TestCycleCount) {
    std::string js = "{\"control_mode\": 0.0, \"dt_hr\": 0.002777777777777778, \"input_current\": 0.0, \"C_rate\": 0.2, \"Qexp\": 60.75, \"Qfull\": 75.56, \"Qnom\": 73.58, \"Vcut\": 3.0, \"Vexp\": 3.529, \"Vfull\": 4.2, \"Vnom\": 3.35, \"Vnom_default\": 3.6, \"chem\": 1.0, \"initial_SOC\": 66.0, \"life_model\": 1.0, \"maximum_SOC\": 100.0, \"minimum_SOC\": 0.0, \"resistance\": 0.001155, \"voltage_choice\": 0.0, \"Cp\": 980.0, \"T_room_init\": 29.69998526573181, \"h\": 8.066, \"loss_choice\": 0.0, \"mass\": 1.55417, \"monthly_charge_loss\": [0.0], \"monthly_discharge_loss\": [0.0], \"monthly_idle_loss\": [0.0], \"nominal_energy\": 0.272, \"nominal_voltage\": 3.6, \"replacement_option\": 0.0, \"schedule_loss\": [0.0], \"surface_area\": 0.1548, \"I\": 0.0, \"I_chargeable\": -8045.99999999998, \"I_dischargeable\": 643.841000000002, \"P\": 0.0, \"P_chargeable\": -108.70616176055955, \"P_dischargeable\": 1.9341550347606316, \"Q\": 53.21000000000006, \"Q_max\": 75.56, \"SOC\": 70.42085759661204, \"T_batt\": 30.07627155118435, \"T_room\": 29.69998526573181, \"V\": 3.7667917861703755, \"heat_dissipated\": 0.0004698373776256373, \"indices_replaced\": [0.0], \"last_idx\": 8639.0, \"loss_kw\": 0.0, \"n_replacements\": 0.0, \"DOD_max\": 1.0, \"DOD_min\": 0.0, \"I_loss\": 0.0, \"SOC_prev\": 70.42085759661204, \"T_batt_prev\": 30.0747312729467, \"average_range\": 33.594321796748574, \"b1_dt\": 0.008196217640552396, \"b2_dt\": 1.1539245050321905e-05, \"b3_dt\": 0.0421665242517969, \"c0_dt\": 76.8158487840016, \"c2_dt\": 3.772090139902601e-05, \"cell_current\": 0.0, \"cell_voltage\": 3.7667917861703755, \"chargeChange\": 0.0, \"charge_mode\": 1.0, \"cum_dt\": 0.9998842592591438, \"temp_dt\": 291.947118, \"cycle_DOD\": 100.0, \"cycle_DOD_max\": [0.0, 100.0, 100.0, 100.0], \"cycle_counts\": [[0.0, 1], [0.16094315625949207, 1], [100.0, 1], [0.6220222339862289, 1]], \"cycle_range\": 0.6220222339862289, \"day_age_of_battery\": 0.9998842592591438, \"dq_relative_li1\": 0.0, \"dq_relative_li2\": 0.0, \"dq_relative_li3\": 0.0, \"dq_relative_neg\": 0.0, \"n_cycles\": 3.0, \"prev_charge\": 2.0, \"q0\": 53.21000000000006, \"q_relative\": 100.0, \"q_relative_cycle\": 0.0, \"q_relative_li\": 100.0, \"q_relative_neg\": 100.0, \"q_relative_thermal\": 100.0, \"qmax_lifetime\": 75.56, \"qmax_thermal\": 75.56, \"rainflow_Xlt\": 0.6220222339862431, \"rainflow_Ylt\": 20.103229221810423, \"rainflow_jlt\": 4.0, \"rainflow_peaks\": [100.0, 0.0, 20.103229221810423, 19.48120698782418], \"percent_unavailable\": 0.0,  \"percent_unavailable_prev\": 0.0}";
    data = json_to_ssc_data(js.c_str());

    mod = ssc_module_create("battery_stateful");
    EXPECT_TRUE(ssc_stateful_module_setup(mod, data));
    
    ssc_data_free(data);
    
    data = json_to_ssc_data(js.c_str()); // Refresh the vtable data since setup overwrites some of the history

    var_table* vt = static_cast<var_table*>(data);

    EXPECT_TRUE(vt->is_assigned("cycle_counts"));
    EXPECT_TRUE(vt->is_assigned("cycle_DOD_max"));

    EXPECT_TRUE(ssc_module_exec(mod, data)); // Executing one step with 0 input_current (default) will trip the next day using the nmc lifetime model

    vt = static_cast<var_table*>(data);
    EXPECT_FALSE(vt->is_assigned("cycle_counts"));
    EXPECT_FALSE(vt->is_assigned("cycle_DOD_max"));

}

TEST_F(CMBatteryStatefulIntegration_cmod_battery_stateful, TestReplacementbySchedule) {
    // test replacement by schedule
    CreateModel(1);

    ssc_number_t schedule[3] = {0, 50, 0};
    ssc_data_set_array(data, "replacement_schedule_percent", schedule, 2);
    ssc_data_set_number(data, "replacement_option", 2);
    ssc_data_set_number(data, "input_current", 0);
    EXPECT_TRUE(ssc_stateful_module_setup(mod, data));

    ssc_data_set_number(data, "q_relative_cycle", 50);

    for (size_t i = 0; i < 365 * 24 + 2; i++) {
        ssc_module_exec(mod, data);
    }

    var_table* vt = static_cast<var_table*>(data);

    EXPECT_EQ(vt->as_integer("n_replacements"), 1);
    EXPECT_EQ(vt->as_vector_ssc_number_t("indices_replaced")[1], 8760);
    EXPECT_EQ(vt->as_number("q_relative"), 100);
}

TEST_F(CMBatteryStatefulIntegration_cmod_battery_stateful, TestReplacementbyScheduleSubhourly) {
    // test subhourly
    CreateModel(0.5);

    ssc_number_t schedule[3] = {0, 50, 0};
    ssc_data_set_array(data, "replacement_schedule_percent", schedule, 2);
    ssc_data_set_number(data, "replacement_option", 2);
    ssc_data_set_number(data, "input_current", 0);
    EXPECT_TRUE(ssc_stateful_module_setup(mod, data));

    ssc_data_set_number(data, "q_relative_cycle", 50);

    for (size_t i = 0; i < 365 * 24 * 2 + 2; i++) {
        ssc_module_exec(mod, data);
    }

    var_table *vt = static_cast<var_table*>(data);

    EXPECT_EQ(vt->as_integer("n_replacements"), 1);
    EXPECT_EQ(vt->as_vector_ssc_number_t("indices_replaced")[1], 17520);
    EXPECT_EQ(vt->as_number("q_relative"), 100);
}

TEST_F(CMBatteryStatefulIntegration_cmod_battery_stateful, TestReplacementByCapacity) {
    // test replacement by capacity
    CreateModel(1);

    ssc_data_set_number(data, "replacement_option", 1);
    ssc_data_set_number(data, "replacement_capacity", 50);
    EXPECT_TRUE(ssc_stateful_module_setup(mod, data));

    ssc_data_set_number(data, "q_relative_cycle", 50);
    ssc_data_set_number(data, "q_relative_calendar", 50);

    for (size_t i = 0; i < 2; i++) {
        ssc_module_exec(mod, data);
    }

    var_table* vt = static_cast<var_table*>(data);

    EXPECT_EQ(vt->as_integer("n_replacements"), 1);
    EXPECT_EQ(vt->as_vector_ssc_number_t("indices_replaced")[1], 1);
    EXPECT_EQ(vt->as_number("q_relative"), 100);
}

TEST_F(CMBatteryStatefulIntegration_cmod_battery_stateful, TestReplacementByCapacitySubhourly) {
    // test subhourly
    CreateModel(0.5);

    ssc_data_set_number(data, "replacement_option", 1);
    ssc_data_set_number(data, "replacement_capacity", 50);
    EXPECT_TRUE(ssc_stateful_module_setup(mod, data));

    ssc_data_set_number(data, "q_relative_cycle", 50);
    ssc_data_set_number(data, "q_relative_calendar", 50);

    for (size_t i = 0; i < 5; i++) {
        ssc_module_exec(mod, data);
    }

    var_table* vt = static_cast<var_table*>(data);

    EXPECT_EQ(vt->as_integer("n_replacements"), 1);
    EXPECT_EQ(vt->as_vector_ssc_number_t("indices_replaced")[1], 2);
    EXPECT_EQ(vt->as_number("q_relative"), 100);
}



TEST_F(CMBatteryStatefulIntegration_cmod_battery_stateful, ssc_1023) {
    double dt_hour = 1.0 / 60;
    ssc_number_t power, soc, current, temp;

    CreateLMOLTOssc1023Model(dt_hour);

    for (size_t i = 0; i < 50; i++) {
        ssc_data_set_number(data, "input_power", -0.1);
        ssc_module_exec(mod, data);
        ssc_data_get_number(data, "P", &power);
        ssc_data_get_number(data, "SOC", &soc);
        ssc_data_get_number(data, "I", &current);
        ssc_data_get_number(data, "T_batt", &temp);
        // uncomment following to check all iterations
        //std::cout << i << ": Power=" << power << ", SOC=" << soc << ", I=" << current << ", T_Batt=" << temp << "\n";
    }
    EXPECT_NEAR(power, -0.1, 1e-4);
    EXPECT_NEAR(soc, 20.8265, 1e-4);
    EXPECT_NEAR(current, -0.495549, 1e-6);
    EXPECT_NEAR(temp, 20, 1e-4);
}

