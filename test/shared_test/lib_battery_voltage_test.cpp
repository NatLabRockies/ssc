/*
BSD 3-Clause License

Copyright (c) Alliance for Energy Innovation, LLC. See also https://github.com/NatLabRockies/ssc/blob/develop/LICENSE
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


#include <gtest/gtest.h>
#include <cmath>

//#include "lib_battery_capacity.h"
#include "lib_battery.h"
#include "lib_battery_voltage_test.h"

TEST_F(voltage_dynamic_lib_battery_voltage_test, SetUpTest) {
    CreateModel(1);

    EXPECT_NEAR(model->cell_voltage(), 4.059, 1e-3);
}

TEST_F(voltage_dynamic_lib_battery_voltage_test, NickelMetalHydrideFromPaperTest){
    CreateModel(1);

    // Figure 3 from A Generic Battery Model for the Dynamic Simulation of Hybrid Electric Vehicles
    cap = std::unique_ptr<capacity_lithium_ion_t>(new capacity_lithium_ion_t(6.5, 100, 100, 0, 1));

    model = std::unique_ptr<voltage_t>(new voltage_dynamic_t(1, 1, 1.2, 1.4,
                                                             1.25, 1.2, 6.5, 1.3, 5.2, 0.66*1.4, 0.2, 0.0046, 1 ));
    std::vector<double> dt_hr = {1./6., 1./3., 1./3.};
    // testing with 1lt curve
    std::vector<double> voltages = {1.25, 1.22, 1.17};
    for (size_t i = 0; i < 3; i++){
        double I = 6.5;
        cap->updateCapacity(I, dt_hr[i]);
        model->updateVoltage(cap->q0(), cap->qmax(), cap->I(), 0, dt_hr[i]);
        EXPECT_NEAR(model->battery_voltage(), voltages[i], 0.05) << "NickelMetalHydrideFromPaperTest: " + std::to_string(i);
    }
}

TEST_F(voltage_dynamic_lib_battery_voltage_test, updateCapacityTest){
    double dt_hour = 1;
    CreateModel(dt_hour);

    double I = 2;
    cap->updateCapacity(I, dt_hour); // qmx = 10, q0 = 3
    model->updateVoltage(cap->q0(), cap->qmax(), cap->I(), 0, dt_hour);
    EXPECT_NEAR(model->cell_voltage(), 3.889, tol);
    EXPECT_NEAR(cap->q0(), 3.625, tol);

    I = -2;
    cap->updateCapacity(I, dt_hour); // qmx = 10, q0 = 5
    model->updateVoltage(cap->q0(), cap->qmax(), cap->I(), 0, dt_hour);
    EXPECT_NEAR(model->cell_voltage(), 4.139, tol);
    EXPECT_NEAR(cap->q0(), 5.625, tol);

    I = 5;
    cap->updateCapacity(I, dt_hour); // qmx = 10, I = 4.5, q0 = 0.5
    model->updateVoltage(cap->q0(), cap->qmax(), cap->I(), 0, dt_hour);
    EXPECT_NEAR(model->cell_voltage(), 2.559, tol);
    EXPECT_NEAR(cap->q0(), 0.625, tol);

}

TEST_F(voltage_dynamic_lib_battery_voltage_test, updateCapacitySubHourly){
    double dt_hour = 1. / 2;
    CreateModel(dt_hour);

    double I = 2;
    cap->updateCapacity(I, dt_hour); // qmx = 10, q0 = 3
    model->updateVoltage(cap->q0(), cap->qmax(), cap->I(), 0, dt_hour);
    EXPECT_NEAR(model->cell_voltage(), 3.94, tol);
    EXPECT_NEAR(cap->q0(), 4.625, tol);

    I = -2;
    cap->updateCapacity(I, dt_hour); // qmx = 10, q0 = 5
    model->updateVoltage(cap->q0(), cap->qmax(), cap->I(), 0, dt_hour);
    EXPECT_NEAR(model->cell_voltage(), 4.14, tol);
    EXPECT_NEAR(cap->q0(), 5.625, tol);

    I = 5;
    cap->updateCapacity(I, dt_hour); // qmx = 10, I = 4.5, q0 = 0.5
    model->updateVoltage(cap->q0(), cap->qmax(), cap->I(), 0, dt_hour);
    EXPECT_NEAR(model->cell_voltage(), 3.73, tol);
    EXPECT_NEAR(cap->q0(), 3.125, tol);

}

TEST_F(voltage_dynamic_lib_battery_voltage_test, updateCapacitySubMinute){
    double dt_hour = 1. / 200;
    CreateModel(dt_hour);

    double I = 2;
    cap->updateCapacity(I, dt_hour); // qmx = 10, q0 = 3
    model->updateVoltage(cap->q0(), cap->qmax(), cap->I(), 0, dt_hour);
    EXPECT_NEAR(model->cell_voltage(), 3.978, 1e-3);
    EXPECT_NEAR(cap->q0(), 5.615, 1e-3);

    I = -2;
    cap->updateCapacity(I, dt_hour); // qmx = 10, q0 = 5
    model->updateVoltage(cap->q0(), cap->qmax(), cap->I(), 0, dt_hour);
    EXPECT_NEAR(model->cell_voltage(), 4.139, 1e-3);
    EXPECT_NEAR(cap->q0(), 5.625, 1e-3);

    I = 5;
    cap->updateCapacity(I, dt_hour); // qmx = 10, I = 4.5, q0 = 0.5
    model->updateVoltage(cap->q0(), cap->qmax(), cap->I(), 0, dt_hour);
    EXPECT_NEAR(model->cell_voltage(), 3.858, 1e-3);
    EXPECT_NEAR(cap->q0(), 5.600, 1e-3);
}

TEST_F(voltage_dynamic_lib_battery_voltage_test, calculateMaxChargeHourly){
    double dt_hour = 1;
    CreateModel(dt_hour);

    // start at half SOC
    double max_current;
    double power = model->calculate_max_charge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -3452, 1);
    double max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 1e-3);
    // max current reduced to enforce SOC
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 95, 1e-3);


    // start at full SOC
    power = model->calculate_max_charge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -329, 1);
    max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 1e-3);
    // max current reduced to enforce SOC
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 95, 1e-3);


    // start at empty SOC
    double I = 2;
    while (cap->SOC() > 5)
        cap->updateCapacity(I, dt_hour);
    power = model->calculate_max_charge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -6860, 1);
    max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 1e-3);
    // max current reduced to enforce SOC
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 95, 1e-3);
}

TEST_F(voltage_dynamic_lib_battery_voltage_test, calculateMaxChargeSubHourly){
    double dt_hour = 0.5;
    CreateModel(dt_hour);

    // start at half SOC
    double max_current;
    double power = model->calculate_max_charge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -7256, 1);
    double max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 1e-3);
    // max current reduced to enforce SOC
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 95, 1e-3);

    // start at full SOC
    power = model->calculate_max_charge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -662, 1);
    max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 1e-3);
    // max current reduced to enforce SOC
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 95, 1e-3);


    // start at empty SOC
    double I = 2;
    while (cap->SOC() > 5)
        cap->updateCapacity(I, dt_hour);
    power = model->calculate_max_charge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -14989, 1);
    max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 1e-3);
    // max current reduced to enforce SOC
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 95, 1e-3);
}

TEST_F(voltage_dynamic_lib_battery_voltage_test, calculateMaxChargeSubMinute){
    double dt_hour = 1. / 360;
    CreateModel(dt_hour);

    // start at half SOC
    double max_current;
    double power = model->calculate_max_charge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -23978855, 1);
    double max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 1e-3);
    // max current reduced to enforce SOC
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 95, 1e-3);

    // start at full SOC
    power = model->calculate_max_charge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -345933, 1);
    max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 1e-3);
    // max current reduced to enforce SOC
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 95, 1e-3);


    // start at empty SOC
    double I = 2;
    while (cap->SOC() > 5)
        cap->updateCapacity(I, dt_hour);
    power = model->calculate_max_charge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -84546927, 1);
    max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 1e-3);
    // max current reduced to enforce SOC
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 95, 1e-3);
}

TEST_F(voltage_dynamic_lib_battery_voltage_test, calculateMaxDischargeHourly){
    double dt_hour = 1;
    CreateModel(dt_hour);

    // start at half SOC
    double max_current;
    double power = model->calculate_max_discharge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, 2020, 1);        // current ~4
    //EXPECT_NEAR(power / max_current, 0, 1); //voltage test
    double max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 1e-1);
    //EXPECT_NEAR(power / max_current_calc, 0, 1); //voltage test 2
    // Does not empty battery for highest power
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 10, 1e-3);

    // start at empty SOC
    power = model->calculate_max_discharge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, 203, 1);
    //EXPECT_NEAR(power / max_current, 0, 1); //voltage test
    max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 1e-1);
    //EXPECT_NEAR(power / max_current_calc, 0, 1); //voltage test 2
    // Empties battery for highest power
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 5, 1e-3);

    // start at full SOC
    double I = -2;
    while (cap->SOC() < 95)
        cap->updateCapacity(I, dt_hour);
    power = model->calculate_max_discharge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, 4102, 1);
    max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 1e-2);
    // Does not empty battery for highest power
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 19, 1e-3);
}

TEST_F(voltage_dynamic_lib_battery_voltage_test, calculateMaxDischargeSubHourly){
    double dt_hour = 0.5;
    CreateModel(dt_hour);

    // start at half SOC
    double max_current;
    double power = model->calculate_max_discharge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, 3814, 1);        // current ~8
    double max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 0.4);
    // Does not empty battery for highest power
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 10, 1e-3);

    // start at empty SOC
    power = model->calculate_max_discharge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, 407, 1);
    max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 1e-1);
    // Empties battery for highest power
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 5, 1e-3);

    // start at full SOC
    double I = -2;
    while (cap->SOC() < 95)
        cap->updateCapacity(I, dt_hour);
    power = model->calculate_max_discharge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, 7391, 1);
    max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 0.3);
    // Does not empty battery for highest power
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 19, 1e-3);
}

TEST_F(voltage_dynamic_lib_battery_voltage_test, calculateMaxDischargeSubMinute){
    double dt_hour = 1. / 200;
    CreateModel(dt_hour);

    // start at half SOC
    double max_current;
    //double q0 = cap->q0();
    //double qmax = cap->qmax();
    double power = model->calculate_max_discharge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, 14257, 1);        // current ~8
    //EXPECT_NEAR(power / max_current, 0, 1); // voltage test 1
    //q0 = cap->q0();
    //qmax = cap->qmax();
    double max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 0.2);
    //EXPECT_NEAR(power / max_current_calc, 0, 1); // voltage test 2
    // Does not empty battery for highest power
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 47.75, 1e-3);

    // start at empty SOC
    power = model->calculate_max_discharge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, 14198, 1);
    max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 2e-1);
    // Does not empty battery for highest power
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 45.506, 1e-3);

    // start at full SOC
    double I = -2;
    while (cap->SOC() < 95)
        cap->updateCapacity(I, dt_hour);
    power = model->calculate_max_discharge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, 14843, 1);
    max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 0.6);
    // Does not empty battery for highest power
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 92.736, 1e-3);
}

TEST_F(voltage_dynamic_lib_battery_voltage_cutoff_test, calculateMaxChargeHourly) {
    double dt_hour = 1;
    CreateModel(dt_hour);

    // start at half SOC
    double max_current;
    double power = model2->calculate_max_charge_w(cap2->q0(), cap2->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -3452, 1);
    double max_current_calc = model2->calculate_current_for_target_w(power, cap2->q0(), cap2->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 1e-3);
    // max current reduced to enforce SOC
    cap2->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap2->SOC(), 95, 1e-3);


    // start at full SOC
    power = model2->calculate_max_charge_w(cap2->q0(), cap2->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -329, 1);
    max_current_calc = model2->calculate_current_for_target_w(power, cap2->q0(), cap2->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 1e-3);
    // max current reduced to enforce SOC
    cap2->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap2->SOC(), 95, 1e-3);


    // start at empty SOC
    double I = 2;
    while (cap2->SOC() > 5)
        cap2->updateCapacity(I, dt_hour);
    power = model2->calculate_max_charge_w(cap2->q0(), cap2->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -6860, 1);
    max_current_calc = model2->calculate_current_for_target_w(power, cap2->q0(), cap2->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 1e-3);
    // max current reduced to enforce SOC
    cap2->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap2->SOC(), 95, 1e-3);
}

TEST_F(voltage_dynamic_lib_battery_voltage_cutoff_test, calculateMaxChargeSubHourly) {
    double dt_hour = 0.5;
    CreateModel(dt_hour);

    // start at half SOC
    double max_current;
    double power = model2->calculate_max_charge_w(cap2->q0(), cap2->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -7256, 1);
    double max_current_calc = model2->calculate_current_for_target_w(power, cap2->q0(), cap2->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 1e-3);
    // max current reduced to enforce SOC
    cap2->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap2->SOC(), 95, 1e-3);

    // start at full SOC
    power = model2->calculate_max_charge_w(cap2->q0(), cap2->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -662, 1);
    max_current_calc = model2->calculate_current_for_target_w(power, cap2->q0(), cap2->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 1e-3);
    // max current reduced to enforce SOC
    cap2->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap2->SOC(), 95, 1e-3);


    // start at empty SOC
    double I = 2;
    while (cap2->SOC() > 5)
        cap2->updateCapacity(I, dt_hour);
    power = model2->calculate_max_charge_w(cap2->q0(), cap2->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -14989, 1);
    max_current_calc = model2->calculate_current_for_target_w(power, cap2->q0(), cap2->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 1e-3);
    // max current reduced to enforce SOC
    cap2->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap2->SOC(), 95, 1e-3);
}

TEST_F(voltage_dynamic_lib_battery_voltage_cutoff_test, calculateMaxChargeSubMinute) {
    double dt_hour = 1. / 360;
    CreateModel(dt_hour);

    // start at half SOC
    double max_current;
    double power = model2->calculate_max_charge_w(cap2->q0(), cap2->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -23978855, 1);
    double max_current_calc = model2->calculate_current_for_target_w(power, cap2->q0(), cap2->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 1e-3);
    // max current reduced to enforce SOC
    cap2->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap2->SOC(), 95, 1e-3);

    // start at full SOC
    power = model2->calculate_max_charge_w(cap2->q0(), cap2->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -345933, 1);
    max_current_calc = model2->calculate_current_for_target_w(power, cap2->q0(), cap2->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 1e-3);
    // max current reduced to enforce SOC
    cap2->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap2->SOC(), 95, 1e-3);


    // start at empty SOC
    double I = 2;
    while (cap2->SOC() > 5)
        cap2->updateCapacity(I, dt_hour);
    power = model2->calculate_max_charge_w(cap2->q0(), cap2->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -84546927, 1);
    max_current_calc = model2->calculate_current_for_target_w(power, cap2->q0(), cap2->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 1e-3);
    // max current reduced to enforce SOC
    cap2->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap2->SOC(), 95, 1e-3);
}


TEST_F(voltage_dynamic_lib_battery_voltage_cutoff_test, calculateMaxDischargeHourly) {
    double dt_hour = 1;
    CreateModel(dt_hour);

    // start at half SOC
    double max_current;
    double power = model2->calculate_max_discharge_w(cap2->q0(), cap2->qmax(), 0, &max_current);
    EXPECT_NEAR(power, 2278, 1); 
    model2->updateVoltage(cap2->q0() - (max_current *dt_hour), cap2->qmax(), max_current, 0, dt_hour);
    double max_current_calc = model2->calculate_current_for_target_w(power, cap2->q0(), cap2->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 0.1);
    // Does not empty battery for highest power
    cap2->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap2->SOC(), 5, 1e-3);

    // start at empty SOC
    power = model2->calculate_max_discharge_w(cap2->q0(), cap2->qmax(), 0, &max_current);
    EXPECT_NEAR(power, 203, 1);
    model2->updateVoltage(cap2->q0() - (max_current * dt_hour), cap2->qmax(), max_current, 0, dt_hour);
    max_current_calc = model2->calculate_current_for_target_w(power, cap2->q0(), cap2->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 0.1);
    // Empties battery for highest power
    cap2->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap2->SOC(), 5, 1e-3);

    // start at full SOC
    double I = -2;
    while (cap2->SOC() < 95)
        cap2->updateCapacity(I, dt_hour);
    model2->updateVoltage(cap2->q0() - (max_current * dt_hour), cap2->qmax(), max_current, 0, dt_hour);
    power = model2->calculate_max_discharge_w(cap2->q0(), cap2->qmax(), 0, &max_current);
    EXPECT_NEAR(power, 4387, 1);
    max_current_calc = model2->calculate_current_for_target_w(power, cap2->q0(), cap2->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 0.1);
    // Does not empty battery for highest power
    cap2->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap2->SOC(), 9.500, 1e-3);
}

TEST_F(voltage_dynamic_lib_battery_voltage_cutoff_test, calculateMaxDischargeSubHourly) {
    double dt_hour = 0.5;
    CreateModel(dt_hour);

    // start at half SOC
    double max_current;
    double power = model2->calculate_max_discharge_w(cap2->q0(), cap2->qmax(), 0, &max_current);
    EXPECT_NEAR(power, 4271, 1);        
    model2->updateVoltage(cap2->q0() - (max_current * dt_hour), cap2->qmax(), max_current, 0, dt_hour);
    double max_current_calc = model2->calculate_current_for_target_w(power, cap2->q0(), cap2->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 0.4);
    // Does not empty battery for highest power
    cap2->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap2->SOC(), 5, 1e-3);

    // start at empty SOC
    power = model2->calculate_max_discharge_w(cap2->q0(), cap2->qmax(), 0, &max_current);
    EXPECT_NEAR(power, 416, 1);
    max_current_calc = model2->calculate_current_for_target_w(power, cap2->q0(), cap2->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 0.1);
    // Empties battery for highest power
    cap2->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap2->SOC(), 5, 1e-3);

    // start at full SOC
    double I = -2;
    while (cap2->SOC() < 95)
        cap2->updateCapacity(I, dt_hour);
    power = model2->calculate_max_discharge_w(cap2->q0(), cap2->qmax(), 0, &max_current);
    EXPECT_NEAR(power, 7752, 1);
    max_current_calc = model2->calculate_current_for_target_w(power, cap2->q0(), cap2->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 0.1);
    // Does not empty battery for highest power
    cap2->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap2->SOC(), 14.25, 1e-3);
}

TEST_F(voltage_dynamic_lib_battery_voltage_cutoff_test, calculateMaxDischargeSubMinute) {
    double dt_hour = 1. / 200;
    CreateModel(dt_hour);
    
    // start at half SOC
    double max_current;
    double power = model2->calculate_max_discharge_w(cap2->q0(), cap2->qmax(), 0, &max_current);
    EXPECT_NEAR(power, 12729, 1);
    model2->updateVoltage(cap2->q0(), cap2->qmax(), max_current, 0, dt_hour);
    double max_current_calc = model2->calculate_current_for_target_w(power, cap2->q0(), cap2->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 0.1);
    cap2->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap2->SOC(), 48.5, 1e-3);

    // start at empty SOC
    power = model2->calculate_max_discharge_w(cap2->q0(), cap2->qmax(), 0, &max_current);
    EXPECT_NEAR(power, 12722, 1);
    max_current_calc = model2->calculate_current_for_target_w(power, cap2->q0(), cap2->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 0.1);
    // Does not empty battery for highest power
    cap2->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap2->SOC(), 46.996, 1e-3);

    // start at full SOC
    double I = -2;
    while (cap2->SOC() < 95)
        cap2->updateCapacity(I, dt_hour);
    power = model2->calculate_max_discharge_w(cap2->q0(), cap2->qmax(), 0, &max_current);
    EXPECT_NEAR(power, 13353, 1);
    max_current_calc = model2->calculate_current_for_target_w(power, cap2->q0(), cap2->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 0.1);
    // Does not empty battery for highest power
    cap2->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap2->SOC(), 93.432, 1e-3);
}

TEST_F(voltage_table_lib_battery_voltage_test, updateCapacityTest){
    double dt_hour = 1;
    CreateModel(dt_hour);

    double I = 2;
    cap->updateCapacity(I, dt_hour); // qmx = 10, q0 = 3
    model->updateVoltage(cap->q0(), cap->qmax(), cap->I(), 0, dt_hour);
    EXPECT_NEAR(model->cell_voltage(), 3.54, tol);
    EXPECT_NEAR(cap->q0(), 3, tol);

    I = -2;
    cap->updateCapacity(I, dt_hour); // qmx = 10, q0 = 5
    model->updateVoltage(cap->q0(), cap->qmax(), cap->I(), 0, dt_hour);
    EXPECT_NEAR(model->cell_voltage(), 3.7, tol);
    EXPECT_NEAR(cap->q0(), 5, tol);

    I = 5;
    cap->updateCapacity(I, dt_hour); // qmx = 10, I = 4.5, q0 = 0.5
    model->updateVoltage(cap->q0(), cap->qmax(), cap->I(), 0, dt_hour);
    EXPECT_NEAR(model->cell_voltage(), 1.336, tol);
    EXPECT_NEAR(cap->q0(), 0.5, tol);
}

TEST_F(voltage_table_lib_battery_voltage_test, updateCapacitySubHourly){
    double dt_hour = 1. / 2;
    CreateModel(dt_hour);

    double I = 2;
    cap->updateCapacity(I, dt_hour); // qmx = 10, q0 = 3
    model->updateVoltage(cap->q0(), cap->qmax(), cap->I(), 0, dt_hour);
    EXPECT_NEAR(model->cell_voltage(), 3.616, tol);
    EXPECT_NEAR(cap->q0(), 4, tol);

    I = -2;
    cap->updateCapacity(I, dt_hour); // qmx = 10, q0 = 5
    model->updateVoltage(cap->q0(), cap->qmax(), cap->I(), 0, dt_hour);
    EXPECT_NEAR(model->cell_voltage(), 3.7, tol);
    EXPECT_NEAR(cap->q0(), 5, tol);

    I = 5;
    cap->updateCapacity(I, dt_hour); // qmx = 10, I = 4.5, q0 = 0.5
    model->updateVoltage(cap->q0(), cap->qmax(), cap->I(), 0, dt_hour);
    EXPECT_NEAR(model->cell_voltage(), 3.492, tol);
    EXPECT_NEAR(cap->q0(), 2.5, tol);
}

TEST_F(voltage_table_lib_battery_voltage_test, updateCapacitySubMinute){
    double dt_hour = 1. / 200;
    CreateModel(dt_hour);

    double I = 2;
    cap->updateCapacity(I, dt_hour); // qmx = 10, q0 = 3
    model->updateVoltage(cap->q0(), cap->qmax(), cap->I(), 0, dt_hour);
    EXPECT_NEAR(model->cell_voltage(), 3.689, tol);
    EXPECT_NEAR(cap->q0(), 4.99, 1e-3);

    I = -2;
    cap->updateCapacity(I, dt_hour); // qmx = 10, q0 = 5
    model->updateVoltage(cap->q0(), cap->qmax(), cap->I(), 0, dt_hour);
    EXPECT_NEAR(model->cell_voltage(), 3.69, tol);
    EXPECT_NEAR(cap->q0(), 5, 1e-3);

    I = 5;
    cap->updateCapacity(I, dt_hour); // qmx = 10, I = 4.5, q0 = 0.5
    model->updateVoltage(cap->q0(), cap->qmax(), cap->I(), 0, dt_hour);
    EXPECT_NEAR(model->cell_voltage(), 3.677, tol);
    EXPECT_NEAR(cap->q0(), 4.975, 1e-3);
}

TEST_F(voltage_table_lib_battery_voltage_test, calculateMaxChargeHourly1){
    double dt_hour = 1;
    CreateModel(dt_hour);
    cap->change_SOC_limits(0, 100);

    // start at half SOC
    double max_current;
    double power = model->calculate_max_charge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -2849, 1);

    // check current estimated for max power
    double max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, -5, 1e-3);
    cap->updateCapacity(max_current_calc, dt_hour);
    EXPECT_NEAR(cap->SOC(), 100, 1e-3);

    // re-discharge to 50 SOC
    max_current_calc *= -1;
    cap->updateCapacity(max_current_calc, dt_hour);

    // little less than max power
    max_current_calc = model->calculate_current_for_target_w(power + 1, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, -4.99, 1e-2);
    cap->updateCapacity(max_current_calc, dt_hour);
    EXPECT_NEAR(cap->SOC(), 99.99, 2e-2);
    EXPECT_NEAR(cap->I() * model->battery_voltage(), -2564, 1);
}

TEST_F(voltage_table_lib_battery_voltage_test, calculateMaxChargeHourly2){
    double dt_hour = 1;
    CreateModel(dt_hour);
    cap->change_SOC_limits(0, 100);

    // start at 90 SOC
    double max_current = -4;
    cap->updateCapacity(max_current, dt_hour);

    double power = model->calculate_max_charge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -569, 1);

    double max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, -1, 1e-3);

    cap->updateCapacity(max_current_calc, dt_hour);
    EXPECT_NEAR(cap->SOC(), 100, 1e-3);

    // re-discharge to 90 SOC
    max_current_calc = 1;
    cap->updateCapacity(max_current_calc, dt_hour);

    // little less than max power
    power = model->calculate_max_charge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -569, 1);

    max_current_calc = model->calculate_current_for_target_w(power + 1, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, -0.998, 2e-3);

    // max current reduced to enforce SOC
    cap->updateCapacity(max_current_calc, dt_hour);
    EXPECT_NEAR(cap->SOC(), 99.984, 2e-2); // for Apple Silicon
    EXPECT_NEAR(cap->I() * model->battery_voltage(), -512, 1);

    // start at empty SOC
    double I = 2;
    while (cap->SOC() > 5)
        cap->updateCapacity(I, dt_hour);
    power = model->calculate_max_charge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -5699, 1);
    max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 1e-3);
    // max current reduced to enforce SOC
    cap->updateCapacity(max_current_calc, dt_hour);
    EXPECT_NEAR(cap->SOC(), 100, 1e-3);
}

TEST_F(voltage_table_lib_battery_voltage_test, calculateMaxChargeHourly3){
    double dt_hour = 1;
    CreateModel(dt_hour);
    cap->change_SOC_limits(0, 100);

    // start at 10 SOC
    double max_current = 4;
    cap->updateCapacity(max_current, dt_hour);

    double power = model->calculate_max_charge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -5129, 1);

    double max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, -9, 1e-3);

    cap->updateCapacity(max_current_calc, dt_hour);
    EXPECT_NEAR(cap->SOC(), 100, 1e-3);

    // re-charge to 10 SOC
    max_current_calc = 9;
    cap->updateCapacity(max_current_calc, dt_hour);

    // little less than max power
    power = model->calculate_max_charge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -5129, 1);

    max_current_calc = model->calculate_current_for_target_w(power + 1, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, -8.99, 1e-2);

    // max current reduced to enforce SOC
    cap->updateCapacity(max_current_calc, dt_hour);
    EXPECT_NEAR(cap->SOC(), 99.99, 2e-2);
    EXPECT_NEAR(cap->I() * model->battery_voltage(), -4615, 2);
}

TEST_F(voltage_table_lib_battery_voltage_test, calculateMaxChargeSubHourly1){
    double dt_hour = .5;
    CreateModel(dt_hour);
    cap->change_SOC_limits(0, 100);

    // start at half SOC
    double max_current;
    double power = model->calculate_max_charge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -5699, 1);

    // check current estimated for max power
    double max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, -10, 1e-3);
    // max current reduced to enforce SOC
    cap->updateCapacity(max_current_calc, dt_hour);
    EXPECT_NEAR(cap->SOC(), 100, 1e-3);

    // re-discharge to 50 SOC
    max_current_calc *= -1;
    cap->updateCapacity(max_current_calc, dt_hour);

    // little less than max power
    max_current_calc = model->calculate_current_for_target_w(power + 1, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, -9.999, 1e-3);
    // max current reduced to enforce SOC
    cap->updateCapacity(max_current_calc, dt_hour);
    EXPECT_NEAR(cap->SOC(), 99.99, 2e-2);
    EXPECT_NEAR(cap->I() * model->battery_voltage(), -5128, 2);
}

TEST_F(voltage_table_lib_battery_voltage_test, calculateMaxChargeSubHourly2){
    double dt_hour = 0.5;
    CreateModel(dt_hour);
    cap->change_SOC_limits(0, 100);

    // start at 90 SOC
    double max_current = -8;
    cap->updateCapacity(max_current, dt_hour);

    double power = model->calculate_max_charge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -1139, 1);

    double max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, -2, 1e-3);

    cap->updateCapacity(max_current_calc, dt_hour);
    EXPECT_NEAR(cap->SOC(), 100, 1e-3);

    // re-discharge to 90 SOC
    max_current_calc = 2;
    cap->updateCapacity(max_current_calc, dt_hour);

    // little less than max power
    power = model->calculate_max_charge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -1139, 1);

    max_current_calc = model->calculate_current_for_target_w(power + 1, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, -2, 1e-2);

    cap->updateCapacity(max_current_calc, dt_hour);
    EXPECT_NEAR(cap->SOC(), 99.99, 2e-2);
    EXPECT_NEAR(cap->I() * model->battery_voltage(), -1025, 1);

    // start at empty SOC
    double I = 2;
    while (cap->SOC() > 5)
        cap->updateCapacity(I, dt_hour);
    power = model->calculate_max_charge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -11398, 1);
    max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 1e-3);
    cap->updateCapacity(max_current_calc, dt_hour);
    EXPECT_NEAR(cap->SOC(), 100, 1e-3);
}

TEST_F(voltage_table_lib_battery_voltage_test, calculateMaxChargeSubMinute){
    double dt_hour = 1. / 120;
    CreateModel(dt_hour);

    // start at half SOC
    double max_current;
    double power = model->calculate_max_charge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -341940, 1);
    double max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 1e-3);
    // max current reduced to enforce SOC
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 95, 1e-3);

    // start at full SOC
    power = model->calculate_max_charge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -34193, 1);
    max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 1e-3);
    // max current reduced to enforce SOC
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 95, 1e-3);


    // start at empty SOC
    double I = 2;
    while (cap->SOC() > 5)
        cap->updateCapacity(I, dt_hour);
    power = model->calculate_max_charge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -649686, 1);
    max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 1e-3);
    // max current reduced to enforce SOC
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 95, 1e-3);
}

TEST_F(voltage_table_lib_battery_voltage_test, calculateMaxDischargeHourly){
    double dt_hour = 1;
    CreateModel(dt_hour);
    cap->change_SOC_limits(0, 100);

    // start at half SOC
    double max_current;
    double power = model->calculate_max_discharge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, 1192.6, 1);        // current ~4
    double max_current_calc = model->calculate_current_for_target_w(power - 1, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, 2.44, 1e-2);
    // Does not empty battery for highest power
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 25.5, 1e-3);
    // Check power
    model->updateVoltage(cap->q0(), cap->qmax(), max_current, 0, dt_hour);
    EXPECT_NEAR(max_current * model->battery_voltage(), power, 2);

    // start at empty SOC
    power = model->calculate_max_discharge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, 581, 1);
    max_current_calc = model->calculate_current_for_target_w(power - 1, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, 1.22, 1e-1);
    // Empties battery for highest power
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 13.25, 1e-3);
    // Check power
    model->updateVoltage(cap->q0(), cap->qmax(), max_current_calc, 0, dt_hour);
    EXPECT_NEAR(max_current_calc * model->battery_voltage(), power, 2);

    // start at full SOC
    double I = -2;
    while (cap->SOC() < 95)
        cap->updateCapacity(I, dt_hour);
    power = model->calculate_max_discharge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, 3553, 1);
    max_current_calc = model->calculate_current_for_target_w(power - 1, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 1e-1);
    // Does not empty battery for highest power
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 27.02, 1e-2);
    // Check power
    model->updateVoltage(cap->q0(), cap->qmax(), max_current, 0, dt_hour);
    EXPECT_NEAR(max_current * model->battery_voltage(), power, 2);
}

TEST_F(voltage_table_lib_battery_voltage_test, calculateMaxDischargeHourlyInputErrors) {
    double dt_hour = 1;
    CreateModel_SSC_565(dt_hour);
    cap->change_SOC_limits(5, 100);

    // start at half SOC
    double max_current;
    // Use larger than available qmax to simulate a mismatch between voltage table and nominal voltage in cell calcs
    double power = model->calculate_max_discharge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, 329.4, 1); // Assumes 4.5 A of discharge at ~12V across 6 cells
    double max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 1e-2); // Show that two methods agree on possible power

    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 5, 1e-3); // Discharge limit is not exceeded
    EXPECT_NEAR(cap->q0(), 0.5, 1e-3);
    EXPECT_NEAR(cap->qmax(), 10, 1e-3);

    // start at empty SOC
    power = model->calculate_max_discharge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, 32.9, 1); // Power decreases as we approach SOC Limits
    max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 1e-1);
    // Empties battery for highest power
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 5, 1e-3);
}

TEST_F(voltage_table_lib_battery_voltage_test, calculateMaxDischargeSubHourly){
    double dt_hour = 0.5;
    CreateModel(dt_hour);
    cap->change_SOC_limits(0, 100);

    // start at half SOC
    double max_current;
    double power = model->calculate_max_discharge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, 2381, 1);        // current ~4
    double max_current_calc = model->calculate_current_for_target_w(power - 1, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current, 4.90, 1e-2);
    EXPECT_NEAR(max_current_calc, 4.90, 1e-1); //4.88 as of 10/9/2023
    // Does not empty battery for highest power
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 25.5, 1e-3);

    // start at empty SOC
    power = model->calculate_max_discharge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, 1161, 1);
    max_current_calc = model->calculate_current_for_target_w(power - 1, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current, 2.44, 1e-1);
    EXPECT_NEAR(max_current_calc, 2.44, 1e-1);
    // Empties battery for highest power
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 13.25, 1e-3);

    // start at full SOC
    double I = -2;
    while (cap->SOC() < 95)
        cap->updateCapacity(I, dt_hour);
    power = model->calculate_max_discharge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, 7073, 1);
    max_current_calc = model->calculate_current_for_target_w(power - 1, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 2e-1);
    // Does not empty battery for highest power
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 27.02, 1e-2);
}

TEST_F(voltage_table_lib_battery_voltage_test, calculateMaxDischargeHourly_table_2) {
    double dt_hour = 1;
    CreateModel_SSC_412(dt_hour);

    // start at half SOC
    double max_current;
    double power = model->calculate_max_discharge_w(cap->q0(), cap->qmax(), 293, &max_current);
    EXPECT_NEAR(power, 714.44, 1);        // current ~4
    double max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 293);
    EXPECT_NEAR(max_current, 3.94, 1e-2);
    EXPECT_NEAR(max_current_calc, 3.94, 1e-2);
    // Does not empty battery for highest power
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 10.5, 1e-3);
    // Check power
    model->updateVoltage(cap->q0(), cap->qmax(), max_current_calc, 293, dt_hour);
    EXPECT_NEAR(max_current_calc * model->battery_voltage(), power, 1e-2);

    // start at empty SOC
    power = model->calculate_max_discharge_w(cap->q0(), cap->qmax(), 293, &max_current);
    EXPECT_NEAR(power, 83.48, 1);
    max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 293);
    EXPECT_NEAR(max_current_calc, max_current, 1e-1);
    // Empties battery for highest power but also runs into SOC limits reducing current
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 5, 1e-3);
    EXPECT_LT(max_current, max_current_calc);
    model->updateVoltage(cap->q0(), cap->qmax(), max_current, 293, dt_hour);
    EXPECT_LT(max_current * model->battery_voltage(), power);

    // start at full SOC
    double I = -2;
    while (cap->SOC() < 95)
        cap->updateCapacity(I, dt_hour);
    power = model->calculate_max_discharge_w(cap->q0(), cap->qmax(), 293, &max_current);
    EXPECT_NEAR(power, 1462.98, 1);
    max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 293);
    EXPECT_NEAR(max_current, 7.5, 1e-2);
    EXPECT_NEAR(max_current_calc, 7.5, 1e-2);
    // Does not empty battery for highest power
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 20, 1e-3);
    // Check power
    model->updateVoltage(cap->q0(), cap->qmax(), max_current_calc, 293, dt_hour);
    EXPECT_NEAR(max_current_calc * model->battery_voltage(), power, 1e-2);
}

TEST_F(voltage_table_lib_battery_voltage_test, calculateMaxDischargeSubMinute){
    double dt_hour = 1. / 200;
    CreateModel(dt_hour);

    // start at half SOC
    double max_current;
    double power = model->calculate_max_discharge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, 164727, 1);        // current ~8
    double max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 0.2);
    // Does not empty battery for highest power
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 25.5, 1e-3);
    // Check power
    model->updateVoltage(cap->q0(), cap->qmax(), max_current_calc, 293, dt_hour);
    EXPECT_NEAR(max_current_calc * model->battery_voltage(), power, 1e-2);

    // start at empty SOC
    power = model->calculate_max_discharge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, 97792, 1);
    max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 1e-1);
    // Does not empty battery for highest power
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 13.25, 1e-3);
    // Check power
    model->updateVoltage(cap->q0(), cap->qmax(), max_current_calc, 293, dt_hour);
    EXPECT_NEAR(max_current_calc * model->battery_voltage(), power, 1e-2);

    // start at full SOC
    double I = -2;
    while (cap->SOC() < 95)
        cap->updateCapacity(I, dt_hour);
    power = model->calculate_max_discharge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, 207281, 1);
    max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 0);
    EXPECT_NEAR(max_current_calc, max_current, 0.3);
    // Does not empty battery for highest power
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 48.0, 1e-3);
    // Check power
    model->updateVoltage(cap->q0(), cap->qmax(), max_current_calc, 293, dt_hour);
    EXPECT_NEAR(max_current_calc * model->battery_voltage(), power, 1e-2);
}

TEST_F(voltage_table_lib_battery_voltage_test, calculate_discharging_past_limits) {
    double dt_hour = 1;
    CreateModel_SSC_412(dt_hour);


    // start at half SOC and run battery nearly empty
    double I = 2;
    while (cap->SOC() > 8)
        cap->updateCapacity(I, dt_hour);

    double max_current;
    double power = model->calculate_max_discharge_w(cap->q0(), cap->qmax(), 293, &max_current);

    // Try using too much power, should get low positive current
    double max_current_calc = model->calculate_current_for_target_w(1000.0, cap->q0(), cap->qmax(), 293);
    EXPECT_NEAR(max_current_calc, 0.5, 1e-2);

    // Power limited by SOC limits in capacity
    cap->updateCapacity(max_current_calc, dt_hour);
    EXPECT_NEAR(cap->SOC(), 5, 1e-3);
    model->updateVoltage(cap->q0(), cap->qmax(), max_current_calc, 293, dt_hour);
    EXPECT_NEAR(model->battery_voltage() * max_current_calc, 0, 1e-2);
}


TEST_F(voltage_vanadium_lib_battery_voltage_test, SetUpTest) {
    CreateModel(1);

    EXPECT_EQ(model->cell_voltage(), 3.6);
}

TEST_F(voltage_vanadium_lib_battery_voltage_test, updateCapacityTest){
    double dt_hour = 1;
    CreateModel(dt_hour);

    double I = 2;
    cap->updateCapacity(I, dt_hour); // qmx = 10, q0 = 3
    model->updateVoltage(cap->q0(), cap->qmax(), cap->I(), 293, dt_hour);
    EXPECT_NEAR(model->cell_voltage(), 3.53, tol);
    EXPECT_NEAR(cap->q0(), 3, tol);

    I = -2;
    cap->updateCapacity(I, dt_hour); // qmx = 10, q0 = 5
    model->updateVoltage(cap->q0(), cap->qmax(), cap->I(), 293, dt_hour);
    EXPECT_NEAR(model->cell_voltage(), 3.64, tol);
    EXPECT_NEAR(cap->q0(), 5, tol);

    I = 5;
    cap->updateCapacity(I, dt_hour); // qmx = 10, I = 4.5, q0 = 0.5
    model->updateVoltage(cap->q0(), cap->qmax(), cap->I(), 293, dt_hour);
    EXPECT_NEAR(model->cell_voltage(), 3.3, tol);
    EXPECT_NEAR(cap->q0(), 0.5, tol);
}

TEST_F(voltage_vanadium_lib_battery_voltage_test, updateCapacitySubMinute){
    double dt_hour = 1. / 200;
    CreateModel(dt_hour);

    double I = 2;
    cap->updateCapacity(I, dt_hour); // qmx = 10, q0 = 3
    model->updateVoltage(cap->q0(), cap->qmax(), cap->I(), 293, dt_hour);
    EXPECT_NEAR(model->cell_voltage(), 3.644, tol);
    EXPECT_NEAR(cap->q0(), 4.99, 1e-3);

    I = -2;
    cap->updateCapacity(I, dt_hour); // qmx = 10, q0 = 5
    model->updateVoltage(cap->q0(), cap->qmax(), cap->I(), 293, dt_hour);
    EXPECT_NEAR(model->cell_voltage(), 3.64, tol);
    EXPECT_NEAR(cap->q0(), 5, 1e-3);

    I = 5;
    cap->updateCapacity(I, dt_hour); // qmx = 10, I = 4.5, q0 = 0.5
    model->updateVoltage(cap->q0(), cap->qmax(), cap->I(), 293, dt_hour);
    EXPECT_NEAR(model->cell_voltage(), 3.71, tol);
    EXPECT_NEAR(cap->q0(), 4.975, 1e-3);
}

TEST_F(voltage_vanadium_lib_battery_voltage_test, calculateMaxChargeHourly){
    double dt_hour = 1;
    CreateModel(dt_hour);

    // start at half SOC
    double max_current;
    double power = model->calculate_max_charge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -2579, 1);
    double max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 293);
    EXPECT_NEAR(max_current_calc, -4.70, 1e-2);
    // max current reduced to enforce SOC
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 95, 1e-2);


    // start at full SOC
    power = model->calculate_max_charge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -251, 1);
    max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 293);
    EXPECT_NEAR(max_current_calc, -0.45, 1e-2);
    // max current reduced to enforce SOC
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 95, 1e-2);


    // start at 5 SOC
    double I = 2;
    while (cap->SOC() > 5)
        cap->updateCapacity(I, dt_hour);
    power = model->calculate_max_charge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -5032, 1);
    max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 293);
    EXPECT_NEAR(max_current_calc, -9.02, 1e-2);
    // max current reduced to enforce SOC
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 95, 1e-2);
}

TEST_F(voltage_vanadium_lib_battery_voltage_test, calculateMaxChargeSubHourly){
    double dt_hour = 0.5;
    CreateModel(dt_hour);

    // start at half SOC
    double max_current;
    double power = model->calculate_max_charge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -5312, 1);
    double max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 293);
    EXPECT_NEAR(max_current_calc, -9.426, 1e-3);
    // max current reduced to enforce SOC
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 95, 1e-3);


    // start at full SOC
    power = model->calculate_max_charge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -503, 1);
    max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 293);
    EXPECT_NEAR(max_current_calc, -0.907, 1e-3);
    // max current reduced to enforce SOC
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 95, 1e-3);


    // start at 5 SOC
    double I = 2;
    while (cap->SOC() > 5)
        cap->updateCapacity(I, dt_hour);
    power = model->calculate_max_charge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -10622, 1);
    max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 293);
    EXPECT_NEAR(max_current_calc, -18.12, 1e-2);
    // max current reduced to enforce SOC
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 95, 1e-3);
}

TEST_F(voltage_vanadium_lib_battery_voltage_test, calculateMaxChargeSubMinute){
    double dt_hour = 1. / 360;
    CreateModel(dt_hour);

    // start at half SOC
    double max_current;
    double power = model->calculate_max_charge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -10908720, 1);
    double max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 293);
    EXPECT_NEAR(max_current_calc, max_current, 1e-2 * std::abs(max_current));
    // max current reduced to enforce SOC
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 95, 1e-3);


    // start at full SOC
    power = model->calculate_max_charge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -190152, 1);
    max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 293);
    EXPECT_NEAR(max_current_calc, max_current, 0.1 * std::abs(max_current));
    // max current reduced to enforce SOC
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 95, 1e-3);


    // start at 5 SOC
    double I = 2;
    while (cap->SOC() > 5)
        cap->updateCapacity(I, dt_hour);
    power = model->calculate_max_charge_w(cap->q0(), cap->qmax(), 0, &max_current);
    EXPECT_NEAR(power, -37840248, 1);
    max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 293);
    EXPECT_NEAR(max_current_calc, max_current, 1e-2 * std::abs(max_current));
    // max current reduced to enforce SOC
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 95, 1e-3);
}

TEST_F(voltage_vanadium_lib_battery_voltage_test, calculateMaxDischargeHourly){
    double dt_hour = 1;
    CreateModel(dt_hour);

    // start at half SOC
    double max_current;
    double power = model->calculate_max_discharge_w(cap->q0(), cap->qmax(), 293, &max_current);
    EXPECT_NEAR(power, 2308, 1);        // current ~4
    double max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 293);
    EXPECT_NEAR(max_current_calc, 4.89, 1e-2);
    // Does not empty battery for highest power
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 5, 1e-3);

    // start at empty SOC
    power = model->calculate_max_discharge_w(cap->q0(), cap->qmax(), 293, &max_current);
    EXPECT_NEAR(power, 213, 1);
    max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 293);
    EXPECT_NEAR(max_current_calc, max_current, 1e-1);
    // Empties battery for highest power
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 5, 1e-3);

    // start at full SOC
    double I = -2;
    while (cap->SOC() < 95)
        cap->updateCapacity(I, dt_hour);
    power = model->calculate_max_discharge_w(cap->q0(), cap->qmax(), 293, &max_current);
    EXPECT_NEAR(power, 4570, 1);
    max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 293);
    EXPECT_NEAR(max_current_calc, 9.316, 1e-2);
    // Does not empty battery for highest power
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 5, 1e-3);
}

TEST_F(voltage_vanadium_lib_battery_voltage_test, calculateMaxDischargeSubHourly){
    double dt_hour = 0.5;
    CreateModel(dt_hour);

    // start at half SOC
    double max_current;
    double power = model->calculate_max_discharge_w(cap->q0(), cap->qmax(), 293, &max_current);
    EXPECT_NEAR(power, 4729, 1);        // current ~4
    double max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 293);
    EXPECT_NEAR(max_current_calc, 9.611, 1e-2);
    // Does not empty battery for highest power
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 5, 1e-3);

    // start at empty SOC
    power = model->calculate_max_discharge_w(cap->q0(), cap->qmax(), 293, &max_current);
    EXPECT_NEAR(power, 425, 1);
    max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 293);
    EXPECT_NEAR(max_current_calc, max_current, 1e-1);
    // Empties battery for highest power
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 5, 1e-3);

    // start at full SOC
    double I = -2;
    while (cap->SOC() < 95)
        cap->updateCapacity(I, dt_hour);
    power = model->calculate_max_discharge_w(cap->q0(), cap->qmax(), 293, &max_current);
    EXPECT_NEAR(power, 9617, 1);
    max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 293);
    EXPECT_NEAR(max_current_calc, max_current, 1e-2);
    // Does not empty battery for highest power
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 5, 1e-3);
}

TEST_F(voltage_vanadium_lib_battery_voltage_test, calculateMaxDischargeSubMinute){
    double dt_hour = 1. / 200;
    CreateModel(dt_hour);

    // start at half SOC
    double max_current;
    double power = model->calculate_max_discharge_w(cap->q0(), cap->qmax(), 293, &max_current);
    EXPECT_NEAR(power, 2015656, 1);        // current ~4
    double max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 293);
    EXPECT_NEAR(max_current_calc, 733.51, 1e-2);
    // Does not empty battery for highest power
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 13.324, 1e-3);

    // start at empty SOC
    power = model->calculate_max_discharge_w(cap->q0(), cap->qmax(), 293, &max_current);
    EXPECT_NEAR(power, 71831, 1);
    max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 293);
    EXPECT_NEAR(max_current_calc, max_current, 1e-1);
    // Does not empty battery for highest power
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 8.641, 1e-3);

    // start at full SOC
    double I = -2;
    while (cap->SOC() < 95)
        cap->updateCapacity(I, dt_hour);
    power = model->calculate_max_discharge_w(cap->q0(), cap->qmax(), 293, &max_current);
    EXPECT_NEAR(power, 8903223, 1);
    max_current_calc = model->calculate_current_for_target_w(power, cap->q0(), cap->qmax(), 293);
    EXPECT_NEAR(max_current_calc, 1621.4, 1e-2);
    // Does not empty battery for highest power
    cap->updateCapacity(max_current, dt_hour);
    EXPECT_NEAR(cap->SOC(), 13.93, 1e-3);
}
