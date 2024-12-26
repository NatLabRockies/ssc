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


#include "lib_battery_dispatch_automatic_fom_test.h"


TEST_F(AutoFOM_lib_battery_dispatch, DispatchFOMInput) {
    double dtHourFOM = 1.0;
    CreateBattery(dtHourFOM);
    dispatchAuto = new dispatch_automatic_front_of_meter_t(batteryModel, dtHourFOM, 15, 95, 1, 999, 999, max_power, max_power,
                                                           max_power, max_power, 1, dispatch_t::FOM_CUSTOM_DISPATCH, dispatch_t::WEATHER_FORECAST_CHOICE::WF_LOOK_AHEAD, dispatch_t::FRONT, 1, 24, 1, true, true, false, true, false, 0,
                                                           replacementCost, 0, cyclingCost, omCost, ppaRate, ur, 98, 98, 98, interconnection_limit,
                                                           cleared_capacity, capacity_forecast_type, cleared_cap_percent);

    std::vector<double> P_batt = {-336.062, 336.062};

    batteryPower = dispatchAuto->getBatteryPower();
    batteryPower->connectionMode = ChargeController::AC_CONNECTED;
    batteryPower->powerSystem = 750;
    batteryPower->powerFuelCell = 300;

    dispatchAuto->set_custom_dispatch(P_batt);

    // battery charging from PV
    EXPECT_FALSE(batteryPower->canGridCharge);
    dispatchAuto->update_dispatch(0, 0, 0, 0);
    EXPECT_NEAR(batteryPower->powerBatteryTarget, -322.6, 0.1);
    dispatchAuto->dispatch(0, 0, 0);

    EXPECT_NEAR(batteryPower->powerBatteryDC, -322.6, 0.1);
    EXPECT_NEAR(batteryPower->powerBatteryAC, -336.1, 0.1);
    EXPECT_NEAR(batteryPower->powerGridToBattery, 0, 0.1);
    EXPECT_NEAR(dispatchAuto->battery_model()->SOC(), 50.2, 1e-2);

    dispatchAuto->update_dispatch(0, 0, 0, 1);
    EXPECT_NEAR(batteryPower->powerBatteryTarget, 350.0, 0.1);
    dispatchAuto->dispatch(0, 1, 0);

    EXPECT_NEAR(batteryPower->powerBatteryDC, 350, 0.1);
    EXPECT_NEAR(batteryPower->powerBatteryAC, 336, 0.1);
    EXPECT_NEAR(batteryPower->powerGridToBattery, 0, 0.1);
}

TEST_F(AutoFOM_lib_battery_dispatch, DispatchFOMInputWithLosses) {
    double dtHourFOM = 1.0;
    CreateBatteryWithLosses(dtHourFOM);
    dispatchAuto = new dispatch_automatic_front_of_meter_t(batteryModel, dtHourFOM, 15, 95, 1, 999, 999, max_power, max_power,
        max_power, max_power, 1, dispatch_t::FOM_CUSTOM_DISPATCH, dispatch_t::WEATHER_FORECAST_CHOICE::WF_LOOK_AHEAD, dispatch_t::FRONT, 1, 24, 1, true, true, false, true, false, 0,
        replacementCost, 0, cyclingCost, omCost, ppaRate, ur, 98, 98, 98, interconnection_limit,
        cleared_capacity, capacity_forecast_type, cleared_cap_percent);

    std::vector<double> P_batt = { -336.062, 336.062 };

    batteryPower = dispatchAuto->getBatteryPower();
    batteryPower->connectionMode = ChargeController::AC_CONNECTED;
    batteryPower->powerSystem = 750;
    batteryPower->powerFuelCell = 300;

    dispatchAuto->set_custom_dispatch(P_batt);

    // battery charging from PV
    EXPECT_FALSE(batteryPower->canGridCharge);
    dispatchAuto->update_dispatch(0, 0, 0, 0);
    EXPECT_NEAR(batteryPower->powerBatteryTarget, -322.6, 0.1);
    dispatchAuto->dispatch(0, 0, 0);

    EXPECT_NEAR(batteryPower->powerBatteryDC, -322.6, 0.1); 
    EXPECT_NEAR(batteryPower->powerBatteryAC, -336.1, 0.1); // Expect charging to remain unchanged, losses will come from the grid
    EXPECT_NEAR(batteryPower->powerGridToBattery, 0, 0.1);
    EXPECT_NEAR(batteryPower->powerSystemLoss, 10.0, 0.1);
    EXPECT_NEAR(dispatchAuto->battery_model()->SOC(), 50.2, 1e-2);

    dispatchAuto->update_dispatch(0, 0, 0, 1);
    EXPECT_NEAR(batteryPower->powerBatteryTarget, 370.9, 0.1);
    dispatchAuto->dispatch(0, 1, 0);

    EXPECT_NEAR(batteryPower->powerBatteryDC, 370.9, 0.1);
    EXPECT_NEAR(batteryPower->powerBatteryAC, 356, 0.1); // Dispatch increases to cover loss
    EXPECT_NEAR(batteryPower->powerGridToBattery, 0, 0.1);
    EXPECT_NEAR(batteryPower->powerSystemLoss, 20.0, 0.1);
}

TEST_F(AutoFOM_lib_battery_dispatch, DispatchFOMInputSubhourly) {
    double dtHourFOM = 0.5;
    CreateBattery(dtHourFOM);
    dispatchAuto = new dispatch_automatic_front_of_meter_t(batteryModel, dtHourFOM, 15, 95, 1, 999, 999, max_power, max_power,
                                                           max_power, max_power, 1, dispatch_t::FOM_CUSTOM_DISPATCH, dispatch_t::WEATHER_FORECAST_CHOICE::WF_LOOK_AHEAD, dispatch_t::FRONT, 1, 24, 1, true, true, false, true, false, 0,
                                                            replacementCost, 0, cyclingCost, omCost, ppaRate, ur, 98, 98, 98, interconnection_limit,
                                                            cleared_capacity, capacity_forecast_type, cleared_cap_percent);

    std::vector<double> P_batt = {-336.062, 336.062};

    batteryPower = dispatchAuto->getBatteryPower();
    batteryPower->connectionMode = ChargeController::AC_CONNECTED;
    batteryPower->powerSystem = 750;
    batteryPower->powerFuelCell = 300;

    dispatchAuto->set_custom_dispatch(P_batt);

    // battery charging from PV to full maximum_SOC
    EXPECT_FALSE(batteryPower->canGridCharge);
    dispatchAuto->update_dispatch(0, 0, 0, 0);
    EXPECT_NEAR(batteryPower->powerBatteryTarget, -322.6, 0.1);
    dispatchAuto->dispatch(0, 0, 0);

    EXPECT_NEAR(batteryPower->powerBatteryDC, -322.6, 0.1);
    EXPECT_NEAR(batteryPower->powerBatteryAC, -336.1, 0.1);
    EXPECT_NEAR(batteryPower->powerGridToBattery, 0, 0.1);
    EXPECT_NEAR(dispatchAuto->battery_model()->SOC(), 50.1, 0.1);

    dispatchAuto->update_dispatch(0, 0, 0, 1);
    EXPECT_NEAR(batteryPower->powerBatteryTarget, 350.0, 0.1);
    dispatchAuto->dispatch(0, 0, 1);

    EXPECT_NEAR(batteryPower->powerBatteryDC, 350, 0.1);
    EXPECT_NEAR(batteryPower->powerBatteryAC, 336, 0.1);
    EXPECT_NEAR(batteryPower->powerGridToBattery, 0, 0.1);
}

TEST_F(AutoFOM_lib_battery_dispatch, DispatchFOM_DCCustomCharge) {
    double dtHour = 1;
    CreateBattery(dtHour);
    dispatchAuto = new dispatch_automatic_front_of_meter_t(batteryModel, dtHour, 10, 100, 1, 49960, 49960, max_power,
                                                           max_power, max_power, max_power, 1, dispatch_t::FOM_CUSTOM_DISPATCH, dispatch_t::WEATHER_FORECAST_CHOICE::WF_LOOK_AHEAD, dispatch_t::FRONT, 1, 18, 1, true,
                                                           true, true, false, false, 77000, replacementCost, 1, cyclingCost, omCost, ppaRate, ur, 98, 98,
                                                           98, interconnection_limit, cleared_capacity, capacity_forecast_type, cleared_cap_percent);

    std::vector<double> P_batt(6, -25000);

    // battery setup
    dispatchAuto->update_pv_data(pv);
    dispatchAuto->update_cliploss_data(clip);
    dispatchAuto->set_custom_dispatch(P_batt);
    batteryPower = dispatchAuto->getBatteryPower();
    batteryPower->connectionMode = ChargeController::DC_CONNECTED;
    batteryPower->voltageSystem = 600;
    batteryPower->canGridCharge = true;
    batteryPower->setSharedInverter(m_sharedInverter);
    batteryPower->inverterEfficiencyCutoff = 0;

    batteryPower->powerGeneratedBySystem = 0;
    batteryPower->powerSystem = 0;
    batteryPower->powerSystemClipped = 0;

    std::vector<double> SOC = {64.42, 78.77, 93.06, 100., 100., 100.};
    for (size_t h = 0; h < 6; h++) {
        dispatchAuto->update_dispatch(0, 0, h, 0);
        EXPECT_NEAR(batteryPower->powerBatteryTarget, -25000, 0.1) << "error in expected target at hour " << h;

        dispatchAuto->dispatch(0, h, 0);

        EXPECT_NEAR(dispatchAuto->battery_soc(), SOC[h], 0.1);

        if (h < 3){
            EXPECT_NEAR(batteryPower->powerBatteryDC, -25000, 100) << "error in dispatched power at hour " << h;
            EXPECT_NEAR(batteryPower->powerGridToBattery, 25868, 100) << "hour " << h;
            EXPECT_NEAR(batteryPower->sharedInverter->efficiencyAC, 97.6, 0.1);
        }
        else if (h == 3){
            EXPECT_NEAR(batteryPower->powerBatteryDC, -12207, 100) << "error in dispatched power at hour " << h;
            EXPECT_NEAR(batteryPower->powerGridToBattery, 12589, 100) << "hour " << h;
            EXPECT_NEAR(batteryPower->sharedInverter->efficiencyAC, 97.9, 0.1);
        }
        else{
            EXPECT_NEAR(batteryPower->powerBatteryDC, 0, 1e-3) << "error in dispatched power at hour " << h;
            EXPECT_NEAR(batteryPower->powerGridToBattery, 0, 1) << "hour " << h;
            EXPECT_NEAR(batteryPower->sharedInverter->efficiencyAC, 0, 0.1);
        }
    }
}

TEST_F(AutoFOM_lib_battery_dispatch, DispatchFOM_DCCustomChargeSubhourly) {
    double dtHour = 0.5;
    CreateBattery(dtHour);
    dispatchAuto = new dispatch_automatic_front_of_meter_t(batteryModel, dtHour, 10, 100, 1, 49960, 49960, max_power,
                                                           max_power, max_power, max_power, 1, dispatch_t::FOM_CUSTOM_DISPATCH, dispatch_t::WEATHER_FORECAST_CHOICE::WF_LOOK_AHEAD, dispatch_t::FRONT, 1, 18, 1, true,
                                                           true, true, false, false, 77000, replacementCost, 1, cyclingCost, omCost, ppaRate, ur, 98, 98,
                                                           98, interconnection_limit, cleared_capacity, capacity_forecast_type, cleared_cap_percent);

    std::vector<double> P_batt(12, -25000);

    // battery setup
    dispatchAuto->update_pv_data(pv);
    dispatchAuto->update_cliploss_data(clip);
    dispatchAuto->set_custom_dispatch(P_batt);
    batteryPower = dispatchAuto->getBatteryPower();
    batteryPower->connectionMode = ChargeController::DC_CONNECTED;
    batteryPower->voltageSystem = 600;
    batteryPower->canGridCharge = true;
    batteryPower->setSharedInverter(m_sharedInverter);
    batteryPower->inverterEfficiencyCutoff = 0;

    batteryPower->powerGeneratedBySystem = 0;
    batteryPower->powerSystem = 0;
    batteryPower->powerSystemClipped = 0;

    std::vector<double> SOC = {57.24, 64.45, 71.64, 78.81, 85.97, 93.12, 100.00, 100.00, 100.00, 100.00, 100.00, 100.00};
    for (size_t h = 0; h < 12; h++) {
        size_t hour_of_year = hour_of_year_from_index(h, dtHour);
        size_t step = step_from_index(h, dtHour);

        dispatchAuto->update_dispatch(0, hour_of_year, step, h);
        EXPECT_NEAR(batteryPower->powerBatteryTarget, -25000, 0.1) << "error in expected target at hour " << h;

        dispatchAuto->dispatch(0, hour_of_year, step);

        EXPECT_NEAR(dispatchAuto->battery_soc(), SOC[h], 0.1) << "hour " << h;

        if (h < 6){
            EXPECT_NEAR(batteryPower->powerBatteryDC, -25000, 1) << "error in dispatched power at hour " << h;
            EXPECT_NEAR(batteryPower->powerGridToBattery, 25877, 1) << "hour " << h;
            EXPECT_NEAR(batteryPower->sharedInverter->efficiencyAC, 97.6, 0.1);
        }
        else if (h == 6){
            EXPECT_NEAR(batteryPower->powerBatteryDC, -24392, 1) << "error in dispatched power at hour " << h;
            EXPECT_NEAR(batteryPower->powerGridToBattery, 25241, 1) << "hour " << h;
            EXPECT_NEAR(batteryPower->sharedInverter->efficiencyAC, 97.64, 0.1);
        }
        else{
            EXPECT_NEAR(batteryPower->powerBatteryDC, 0, 1e-3) << "error in dispatched power at hour " << h;
            EXPECT_NEAR(batteryPower->powerGridToBattery, 0, 1) << "hour " << h;
            EXPECT_NEAR(batteryPower->sharedInverter->efficiencyAC, 0, 0.1);
        }
    }
}

TEST_F(AutoFOM_lib_battery_dispatch, DispatchFOM_DCAuto) {
    double dtHour = 1;
    CreateBattery(dtHour);
    dispatchAuto = new dispatch_automatic_front_of_meter_t(batteryModel, dtHour, 10, 100, 1, 49960, 49960, max_power,
                                                           max_power, max_power, max_power, 1, dispatch_t::FOM_AUTOMATED_ECONOMIC, dispatch_t::WEATHER_FORECAST_CHOICE::WF_LOOK_AHEAD, dispatch_t::FRONT, 1, 18, 1, true, true, false,
                                                           false, false, 77000, replacementCost, 1, cyclingCost, omCost, ppaRate, ur, 98, 98, 98, interconnection_limit, cleared_capacity, capacity_forecast_type, cleared_cap_percent);

    // battery setup
    dispatchAuto->update_pv_data(pv);
    dispatchAuto->update_cliploss_data(clip);
    batteryPower = dispatchAuto->getBatteryPower();
    batteryPower->connectionMode = ChargeController::DC_CONNECTED;
    batteryPower->voltageSystem = 600;
    batteryPower->setSharedInverter(m_sharedInverter);

    std::vector<double> targetkW = { 55396.69, 6901.80, 32515.30, -9767.19, -10052.39, -9202.19, // 0 - 5
                                        -84205.39, -78854.6, -54551.36, -29551.38, 0., 0., // 6 - 11
                                        0., 0., 0., 0., 0., 77000, // 12 - 17
                                        77000, 77000, 77000, 77000, 77000, 0. }; // 18 - 23
    std::vector<double> dispatchedkW = { 27100.68, 6901.79, 24259.38, -9767.20, -10052.40, -9202.19, // 0 - 5
                                        -28633.67, -28948.33, -29132.38, -29255.23, 0., 0., // 6 - 11
                                        0., 0., 0., 0., 0., 28090.71, // 18
                                        27906.71, 27592.06, 26931.25, 24654.69, 5135.31, 0. }; // 24
    std::vector<double> SOC = { 33.33, 29.11, 12.44, 18.59, 24.74, 30.29, // 6
                                46.96, 63.63, 80.29, 96.96, 96.96, 96.96, // 12
                                96.96, 96.96, 96.96, 96.96, 96.96, 80.30, // 12 - 17
                                63.63, 46.96, 30.30, 13.63, 10, 10 }; // 18 - 23
    for (size_t h = 0; h < 24; h++) {
        batteryPower->powerGeneratedBySystem = pv[h];
        batteryPower->powerSystem = pv[h];
        batteryPower->powerSystemClipped = clip[h];

        dispatchAuto->update_dispatch(0, h, 0, h);
        EXPECT_NEAR(batteryPower->powerBatteryTarget, targetkW[h], 0.1) << "error in expected target at hour " << h;

        dispatchAuto->dispatch(0, h, 0);
        EXPECT_NEAR(batteryPower->powerBatteryDC, dispatchedkW[h], 0.1) << "error in dispatched power at hour " << h;
        EXPECT_NEAR(dispatchAuto->battery_soc(), SOC[h], 0.1) << "error in SOC at hour " << h;

    }
}

TEST_F(AutoFOM_lib_battery_dispatch, DispatchFOM_DCAutoWithLosses) {
    double dtHour = 1;
    CreateBatteryWithLosses(dtHour);
    dispatchAuto = new dispatch_automatic_front_of_meter_t(batteryModel, dtHour, 10, 100, 1, 49960, 49960, max_power,
        max_power, max_power, max_power, 1, dispatch_t::FOM_AUTOMATED_ECONOMIC, dispatch_t::WEATHER_FORECAST_CHOICE::WF_LOOK_AHEAD, dispatch_t::FRONT, 1, 18, 1, true, true, false,
        false, false, 77000, replacementCost, 1, cyclingCost, omCost, ppaRate, ur, 98, 98, 98, interconnection_limit, cleared_capacity, capacity_forecast_type, cleared_cap_percent);

    // battery setup
    dispatchAuto->update_pv_data(pv);
    dispatchAuto->update_cliploss_data(clip);
    batteryPower = dispatchAuto->getBatteryPower();
    batteryPower->connectionMode = ChargeController::DC_CONNECTED;
    batteryPower->voltageSystem = 600;
    batteryPower->setSharedInverter(m_sharedInverter);

    std::vector<double> targetkW = { 55401.69, 6906.80, 32520.30, -9762.19, -10047.39, -9197.19, // 0 - 5
                                        -84205.39, -78854.6, -54568.93, -29568.95, 0., 0., // 6 - 11
                                        0., 0., 0., 0., 0., 77005, // 12 - 17
                                    77005, 77005, 77005, 77005, 77005, 0. }; // 18 - 23
    std::vector<double> dispatchedkW = { 27100.68, 6906.79, 24258.25, -9762.20, -10047.40, -9197.19, // 0 - 5
                                        -28633.37, -28948.17, -29132.28, -29255.12, 0., 0., // 6 - 11
                                        0., 0., 0., 0., 0., 28090.71, // 18
                                        27906.54, 27591.76, 26930.53, 24651.13, 5118.83, 0 }; // 24
    std::vector<double> SOC = { 33.33, 29.11, 12.44, 18.59, 24.74, 30.29, // 6
                                46.96, 63.63, 80.29, 96.96, 96.96, 96.96, // 12
                                96.96, 96.96, 96.96, 96.96, 96.96, 80.30, // 12 - 17
                                63.63, 46.96, 30.30, 13.63, 10, 10 }; // 18 - 23
    for (size_t h = 0; h < 24; h++) {
        batteryPower->powerGeneratedBySystem = pv[h];
        batteryPower->powerSystem = pv[h];
        batteryPower->powerSystemClipped = clip[h];

        dispatchAuto->update_dispatch(0, h, 0, h);
        EXPECT_NEAR(batteryPower->powerBatteryTarget, targetkW[h], 0.1) << "error in expected target at hour " << h;

        dispatchAuto->dispatch(0, h, 0);
        EXPECT_NEAR(batteryPower->powerBatteryDC, dispatchedkW[h], 0.1) << "error in dispatched power at hour " << h;
        EXPECT_NEAR(dispatchAuto->battery_soc(), SOC[h], 0.1) << "error in SOC at hour " << h;

    }
}
TEST_F(AutoFOM_lib_battery_dispatch, DispatchFOM_DCAutoSubhourly) {
    double dtHour = 0.5;
    CreateBattery(dtHour);
    dispatchAuto = new dispatch_automatic_front_of_meter_t(batteryModel, dtHour, 10, 100, 1, 49960, 49960, max_power,
                                                           max_power, max_power, max_power, 1, dispatch_t::FOM_AUTOMATED_ECONOMIC, dispatch_t::WEATHER_FORECAST_CHOICE::WF_LOOK_AHEAD, dispatch_t::FRONT, 1, 18, 1, true, true, false,
                                                           false, false, 77000, replacementCost, 1, cyclingCost, omCost, ppaRate, ur, 98, 98, 98, interconnection_limit, cleared_capacity, capacity_forecast_type, cleared_cap_percent);

    // battery setup
    dispatchAuto->update_pv_data(pv);
    dispatchAuto->update_cliploss_data(clip);
    batteryPower = dispatchAuto->getBatteryPower();
    batteryPower->connectionMode = ChargeController::DC_CONNECTED;
    batteryPower->voltageSystem = 600;
    batteryPower->setSharedInverter(m_sharedInverter);

    std::vector<double> targetkW = { 55396.69, 6901.80, 32515.30, -9767.19, -10052.39, -9202.19 };
    std::vector<double> dispatchkW = { 27439.22, 6901.79, 26988.49, -9767.20, -10052.39, -9202.19 };
    std::vector<double> SOC = {41.66, 39.59, 31.26, 34.18, 37.17, 39.90 };
    for (size_t h = 0; h < 6; h++) {
        size_t hour_of_year = hour_of_year_from_index(h, dtHour);
        size_t step = step_from_index(h, dtHour);

        batteryPower->powerGeneratedBySystem = pv[h];
        batteryPower->powerSystem = pv[h];
        batteryPower->powerSystemClipped = clip[h];

        dispatchAuto->update_dispatch(0, hour_of_year, step, h);
        EXPECT_NEAR(batteryPower->powerBatteryTarget, targetkW[h], 0.1) << "error in expected target at hour " << h;

        dispatchAuto->dispatch(0, hour_of_year, step);
        EXPECT_NEAR(batteryPower->powerBatteryDC, dispatchkW[h], 0.1) << "error in dispatched power at hour " << h;
        EXPECT_NEAR(dispatchAuto->battery_soc(), SOC[h], 0.1) << "error in SOC at hour " << h;
    }
}


TEST_F(AutoFOM_lib_battery_dispatch, DispatchFOM_ACCustomCharge) {
    double dtHour = 1;
    CreateBattery(dtHour);
    dispatchAuto = new dispatch_automatic_front_of_meter_t(batteryModel, dtHour, 10, 100, 1, 49960, 49960, max_power,
                                                           max_power, max_power, max_power, 1, dispatch_t::FOM_CUSTOM_DISPATCH, dispatch_t::WEATHER_FORECAST_CHOICE::WF_LOOK_AHEAD, dispatch_t::FRONT, 1, 18, 1, true,
                                                             true, true, false, false, 77000, replacementCost, 1, cyclingCost, omCost, ppaRate, ur, 98, 98,
                                                             98, interconnection_limit, cleared_capacity, capacity_forecast_type, cleared_cap_percent);

    std::vector<double> P_batt(6, -25000);

    // battery setup
    dispatchAuto->update_pv_data(pv);
    dispatchAuto->update_cliploss_data(clip);
    dispatchAuto->set_custom_dispatch(P_batt);
    batteryPower = dispatchAuto->getBatteryPower();
    batteryPower->connectionMode = ChargeController::AC_CONNECTED;
    batteryPower->voltageSystem = 600;
    batteryPower->canGridCharge = true;
    batteryPower->setSharedInverter(m_sharedInverter);

    batteryPower->powerGeneratedBySystem = 0;
    batteryPower->powerSystem = 0;
    batteryPower->powerSystemClipped = 0;

    std::vector<double> SOC = {63.86, 77.64, 91.37, 100.00, 100.00, 100.00};
    for (size_t h = 0; h < 6; h++) {
        dispatchAuto->update_dispatch(0, 0, h, 0);
        EXPECT_NEAR(batteryPower->powerBatteryTarget, -24000, 0.1) << "error in expected target at hour " << h;

        dispatchAuto->dispatch(0, h, 0);

        EXPECT_NEAR(dispatchAuto->battery_soc(), SOC[h],0.1);

        if (h < 3){
            EXPECT_NEAR(batteryPower->powerBatteryDC, -24000, 1) << "error in dispatched power at hour " << h;
            EXPECT_NEAR(batteryPower->powerGridToBattery, 25000, 1) << "hour " << h;
            EXPECT_NEAR(batteryPower->sharedInverter->efficiencyAC, 96, 0.1);
        }
        else if (h == 3){
            EXPECT_NEAR(batteryPower->powerBatteryDC, -15195, 1) << "error in dispatched power at hour " << h;
            EXPECT_NEAR(batteryPower->powerGridToBattery, 15828., 1) << "hour " << h;
            EXPECT_NEAR(batteryPower->sharedInverter->efficiencyAC, 96, 0.1);
        }
        else{
            EXPECT_NEAR(batteryPower->powerBatteryDC, 0, 1e-3) << "error in dispatched power at hour " << h;
            EXPECT_NEAR(batteryPower->powerGridToBattery, 0, 1) << "hour " << h;
            EXPECT_NEAR(batteryPower->sharedInverter->efficiencyAC, 96, 0.1);
        }
    }
}

TEST_F(AutoFOM_lib_battery_dispatch, DispatchFOM_ACCustomChargeSubhourly) {
    double dtHour = 0.5;
    CreateBattery(dtHour);
    dispatchAuto = new dispatch_automatic_front_of_meter_t(batteryModel, dtHour, 10, 100, 1, 49960, 49960, max_power,
                                                           max_power, max_power, max_power, 1, dispatch_t::FOM_CUSTOM_DISPATCH, dispatch_t::WEATHER_FORECAST_CHOICE::WF_LOOK_AHEAD, dispatch_t::FRONT, 1, 18, 1, true,
                                                           true, true, false, false, 77000, replacementCost, 1, cyclingCost, omCost, ppaRate, ur, 98, 98,
                                                           98, interconnection_limit, cleared_capacity, capacity_forecast_type, cleared_cap_percent);

    std::vector<double> P_batt(12, -25000);

    // battery setup
    dispatchAuto->update_pv_data(pv);
    dispatchAuto->update_cliploss_data(clip);
    dispatchAuto->set_custom_dispatch(P_batt);
    batteryPower = dispatchAuto->getBatteryPower();
    batteryPower->connectionMode = ChargeController::AC_CONNECTED;
    batteryPower->voltageSystem = 600;
    batteryPower->canGridCharge = true;
    batteryPower->setSharedInverter(m_sharedInverter);

    batteryPower->powerGeneratedBySystem = 0;
    batteryPower->powerSystem = 0;
    batteryPower->powerSystemClipped = 0;

    std::vector<double> SOC = {56.95, 63.88, 70.79, 77.68, 84.56, 91.43, 98.28, 100.00, 100.00, 100.00, 100.00, 100.00};
    for (size_t h = 0; h < 12; h++) {
        size_t hour_of_year = hour_of_year_from_index(h, dtHour);
        size_t step = step_from_index(h, dtHour);

        dispatchAuto->update_dispatch(0, hour_of_year, step, h);
        EXPECT_NEAR(batteryPower->powerBatteryTarget, -24000, 0.1) << "error in expected target at hour " << h;

        dispatchAuto->dispatch(0, hour_of_year, step);

        EXPECT_NEAR(dispatchAuto->battery_soc(), SOC[h], 0.1);

        if (h < 7){
            EXPECT_NEAR(batteryPower->powerBatteryDC, -24000, 1) << "error in dispatched power at hour " << h;
            EXPECT_NEAR(batteryPower->powerGridToBattery, 25000, 1) << "hour " << h;
            EXPECT_NEAR(batteryPower->sharedInverter->efficiencyAC, 96, 0.1);
        }
        else if (h == 7){
            EXPECT_NEAR(batteryPower->powerBatteryDC, -6022, 1) << "error in dispatched power at hour " << h;
            EXPECT_NEAR(batteryPower->powerGridToBattery, 6273, 1) << "hour " << h;
            EXPECT_NEAR(batteryPower->sharedInverter->efficiencyAC, 96, 0.1);
        }
        else{
            EXPECT_NEAR(batteryPower->powerBatteryDC, 0, 1e-3) << "error in dispatched power at hour " << h;
            EXPECT_NEAR(batteryPower->powerGridToBattery, 0, 1) << "hour " << h;
            EXPECT_NEAR(batteryPower->sharedInverter->efficiencyAC, 96, 0.1);
        }
    }
}

TEST_F(AutoFOM_lib_battery_dispatch, DispatchFOM_ACAuto) {
    double dtHour = 1;
    CreateBattery(dtHour);
    dispatchAuto = new dispatch_automatic_front_of_meter_t(batteryModel, dtHour, 10, 100, 1, 49960, 49960, max_power,
                                                           max_power, max_power, max_power, 1, dispatch_t::FOM_AUTOMATED_ECONOMIC, dispatch_t::WEATHER_FORECAST_CHOICE::WF_LOOK_AHEAD, dispatch_t::FRONT, 1, 18, 1, true, true, false,
                                                           false, false, 77000, replacementCost, 1, cyclingCost, omCost, ppaRate, ur, 98, 98, 98, interconnection_limit, cleared_capacity, capacity_forecast_type, cleared_cap_percent);

    // battery setup
    dispatchAuto->update_pv_data(pv); // PV Resource is available for the 1st 10 hrs
    dispatchAuto->update_cliploss_data(clip); // Clip charging is available for the 1st 5 hrs
    batteryPower = dispatchAuto->getBatteryPower();
    batteryPower->connectionMode = ChargeController::AC_CONNECTED;
    batteryPower->voltageSystem = 600;
    batteryPower->setSharedInverter(m_sharedInverter);

    // Note on hour 3: the price here is both the 2nd highest price and the 2nd lowest price. A non-zero replacement cost could prevent cycling here.
    std::vector<double> targetkW = { 77000, 77000, 77000, -7205.42, 77000, 0.,
                                    -84205.39, -78854.60, -67702.19, -31516.09, 0., 0.,
                                    0., 0., 0., 0., 0., 77000,
                                    77000, 77000, 77000, 0., 0., 0. };
    std::vector<double> dispatchedkW = { 27100.68, 25407.98, 9385.55, -7205.42, 6616.55, 0.,
                                -27719.16, -28532.97, -28894.66, -29099.09, 0., 0.,
                                0., 0., 0., 0., 0., 27852.99,
                                27491.37, 26677.62, 23151.35, 0., 0., 0. };
    std::vector<double> SOC = { 33.3, 16.66, 10.0, 14.68, 10.0, 10.0,
                                26.66, 43.33, 60.0, 76.66, 76.66, 76.66,
                                76.66, 76.66, 76.66, 76.66, 76.66, 60.00,
                                43.33, 26.66, 10.0, 10.0, 10.0, 10.0 };
    for (size_t h = 0; h < 24; h++) {
        batteryPower->powerGeneratedBySystem = pv[h];
        batteryPower->powerSystem = pv[h];
        batteryPower->powerSystemClipped = clip[h];

        dispatchAuto->update_dispatch(0, h, 0, h);
        EXPECT_NEAR(batteryPower->powerBatteryTarget, targetkW[h], 0.1) << "error in expected target at hour " << h;

        dispatchAuto->dispatch(0, h, 0);

        EXPECT_NEAR(batteryPower->powerBatteryDC, dispatchedkW[h], 0.1) << "error in dispatched power at hour " << h;
        EXPECT_NEAR(dispatchAuto->battery_soc(), SOC[h], 0.1) << "error in SOC at hour " << h;
    }
}

TEST_F(AutoFOM_lib_battery_dispatch, DispatchFOM_ACAutoWithLosses) {
    double dtHour = 1;
    CreateBatteryWithLosses(dtHour);
    dispatchAuto = new dispatch_automatic_front_of_meter_t(batteryModel, dtHour, 10, 100, 1, 49960, 49960, max_power,
        max_power, max_power, max_power, 1, dispatch_t::FOM_AUTOMATED_ECONOMIC, dispatch_t::WEATHER_FORECAST_CHOICE::WF_LOOK_AHEAD, dispatch_t::FRONT, 1, 18, 1, true, true, false,
        false, false, 77000, replacementCost, 1, cyclingCost, omCost, ppaRate, ur, 98, 98, 98, interconnection_limit, cleared_capacity, capacity_forecast_type, cleared_cap_percent);

    // battery setup
    dispatchAuto->update_pv_data(pv); // PV Resource is available for the 1st 10 hrs
    dispatchAuto->update_cliploss_data(clip); // Clip charging is available for the 1st 5 hrs
    batteryPower = dispatchAuto->getBatteryPower();
    batteryPower->connectionMode = ChargeController::AC_CONNECTED;
    batteryPower->voltageSystem = 600;
    batteryPower->setSharedInverter(m_sharedInverter);

    std::vector<double> targetkW = { 77000, 77000, 77000, -7205.42, 77000, 0.,
                                    -84205.39, -78854.60, -67702.19, -31516.09, 0., 0.,
                                    0., 0., 0., 0., 0., 77000,
                                    77000, 77000, 77000, 0., 0., 0. };
    std::vector<double> dispatchedkW = { 27100.68, 25407.98, 9385.55, -7205.42, 6616.55, 0.,
                                -27719.16, -28532.97, -28894.66, -29099.09, 0., 0.,
                                0., 0., 0., 0., 0., 27852.99,
                                27491.37, 26677.62, 23151.35, 0., 0., 0. };
    std::vector<double> SOC = { 33.3, 16.66, 10.0, 14.68, 10.0, 10.0,
                                26.66, 43.33, 60.0, 76.66, 76.66, 76.66,
                                76.66, 76.66, 76.66, 76.66, 76.66, 60.00,
                                43.33, 26.66, 10.0, 10.0, 10.0, 10.0 };
    // Battery was already discharging at max power, it stays unchanged
    for (size_t h = 0; h < 24; h++) {
        batteryPower->powerGeneratedBySystem = pv[h];
        batteryPower->powerSystem = pv[h];
        batteryPower->powerSystemClipped = clip[h];

        dispatchAuto->update_dispatch(0, h, 0, h);
        EXPECT_NEAR(batteryPower->powerBatteryTarget, targetkW[h], 0.1) << "error in expected target at hour " << h;

        dispatchAuto->dispatch(0, h, 0);

        EXPECT_NEAR(batteryPower->powerBatteryDC, dispatchedkW[h], 0.1) << "error in dispatched power at hour " << h;
        EXPECT_NEAR(dispatchAuto->battery_soc(), SOC[h], 0.1) << "error in SOC at hour " << h;
    }
}

TEST_F(AutoFOM_lib_battery_dispatch, DispatchFOM_ACAutoSubhourly) {
    double dtHour = 0.5;
    CreateBattery(dtHour);
    dispatchAuto = new dispatch_automatic_front_of_meter_t(batteryModel, dtHour, 10, 100, 1, 49960, 49960, max_power,
                                                           max_power, max_power, max_power, 1, dispatch_t::FOM_AUTOMATED_ECONOMIC, dispatch_t::WEATHER_FORECAST_CHOICE::WF_LOOK_AHEAD, dispatch_t::FRONT, 1, 18, 1, true, true, false,
                                                           false, false, 77000, replacementCost, 1, cyclingCost, omCost, ppaRate, ur, 98, 98, 98, interconnection_limit, cleared_capacity, capacity_forecast_type, cleared_cap_percent);

    // battery setup
    dispatchAuto->update_pv_data(pv);
    dispatchAuto->update_cliploss_data(clip);
    batteryPower = dispatchAuto->getBatteryPower();
    batteryPower->connectionMode = ChargeController::AC_CONNECTED;
    batteryPower->voltageSystem = 600;
    batteryPower->setSharedInverter(m_sharedInverter);

    std::vector<double> targetkW = { 77000, 77000, 77000, 77000, 77000, 0. };
    std::vector<double> dispatchedkW = { 27439.22, 27100.68, 26536.45, 25407.98, 18604.42, 0. };
    std::vector<double> SOC = { 41.66, 33.33, 25.0, 16.66, 10.0, 10.0};
    for (size_t h = 0; h < 6; h++) {
        size_t hour_of_year = hour_of_year_from_index(h, dtHour);
        size_t step = step_from_index(h, dtHour);

        batteryPower->powerGeneratedBySystem = pv[h];
        batteryPower->powerSystem = pv[h];
        batteryPower->powerSystemClipped = clip[h];

        dispatchAuto->update_dispatch(0, hour_of_year, step, h);
        EXPECT_NEAR(batteryPower->powerBatteryTarget, targetkW[h], 0.1) << "error in expected target at hour " << h;

        dispatchAuto->dispatch(0, hour_of_year, step);
        EXPECT_NEAR(batteryPower->powerBatteryDC, dispatchedkW[h], 0.1) << "error in dispatched power at hour " << h;
        EXPECT_NEAR(dispatchAuto->battery_soc(), SOC[h], 0.1) << "error in SOC at hour " << h;

    }
}

TEST_F(AutoFOM_lib_battery_dispatch, DispatchFOM_DCAuto_ClipChargeFlatRate) {
    double dtHour = 1;
    CreateBattery(dtHour);

    pv = { 21603.3, 70098.2, 44484.7, 86767.2, 87052.4, 86202.2,   
            84205.4, 78854.6, 78854.6, 78854.6, 0, 0,
            0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0 };
    clip = { 0, 0, 0, 9767.18, 10052.4, 9202.19,
            7205.42, 1854.6, 1854.6, 1854.6, 0, 0,
            0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0 };


    ppaRate = { 0.04938, 0.04938, 0.04938, 0.04938, 0.04938, 0.04938,
                0.04938, 0.04938, 0.04938, 0.04938, 0.04938, 0.04938,
                0.04938, 0.04938, 0.04938, 0.04938, 0.04938, 0.04938,
                0.04938, 0.04938, 0.04938, 0.04938, 0.04938, 0.04938,
                0.04938, 0.04938, 0.04938, 0.04938, 0.04938, 0.04938,
                0.04938, 0.04938, 0.04938, 0.04938, 0.04938, 0.04938,
                0.04938, 0.04938, 0.04938, 0.04938, 0.04938, 0.04938,
                0.04938, 0.04938, 0.04938, 0.04938, 0.04938, 0.04938 };

    cyclingCost = { 0.0 };

    dispatchAuto = new dispatch_automatic_front_of_meter_t(batteryModel, dtHour, 10, 100, 1, 49960, 49960, max_power,
        max_power, max_power, max_power, 1, dispatch_t::FOM_AUTOMATED_ECONOMIC, dispatch_t::WEATHER_FORECAST_CHOICE::WF_LOOK_AHEAD, dispatch_t::FRONT, 1, 18, 1, true, true, false,
        false, false, 77000, replacementCost, 1, cyclingCost, omCost, ppaRate, ur, 98, 98, 98, interconnection_limit, cleared_capacity, capacity_forecast_type, cleared_cap_percent);

    // battery setup
    dispatchAuto->update_pv_data(pv);
    dispatchAuto->update_cliploss_data(clip);
    batteryPower = dispatchAuto->getBatteryPower();
    batteryPower->connectionMode = ChargeController::DC_CONNECTED;
    batteryPower->voltageSystem = 600;
    batteryPower->setSharedInverter(m_sharedInverter);

    // Hours 0 - 2: Discharge until the inverter is running at full capacty
    // Hours 3 - 9: Charge from clipped power
    // Hours 10 - 11: Discharge until min SOC at full inverter capacity
    std::vector<double> targetkW = { 55396.69, 6901.80, 32515.3, -9767.18, -10052.4, -9202.19, // 0 - 5
                                    -7205.42, -1854.6, -1854.6, -1854.6, 77000, 77000, // 6 - 11
                                        0., 0., 0., 0., 0.,0., // 12 - 17
                                        0., 0., 0., 0., 0., 0. }; // 18 - 23
    // Differences bettween target numbers and dispatched numbers are due to battery only being 25 MW AC, and SOC differences (steps 2, 11)
    std::vector<double> dispatchedkW = { 27100.68, 6901.80,  24259.38, -9767.18, -10052.4, -9202.19, // 0 - 5
                                    -7205.42, -1854.6, -1854.6, -1854.6, 26142.28, 15787.38, // 6 - 11
                                        0., 0., 0., 0., 0.,0., // 12 - 17
                                        0., 0., 0., 0., 0., 0. }; // 24
    std::vector<double> SOC = { 33.33, 29.11, 12.44, 18.59, 24.74, 30.29, // 0 - 5
                                34.61, 35.72, 36.84, 37.94, 21.28, 10.0, // 6 - 11
                                10.0, 10.0, 10.0, 10.0, 10.0, 10.0, // 12 - 17
                                10.0, 10.0, 10.0, 10.0, 10, 10 }; // 18 - 23
    for (size_t h = 0; h < 24; h++) {
        batteryPower->powerGeneratedBySystem = pv[h];
        batteryPower->powerSystem = pv[h];
        batteryPower->powerSystemClipped = clip[h];

        dispatchAuto->update_dispatch(0, h, 0, h);
        EXPECT_NEAR(batteryPower->powerBatteryTarget, targetkW[h], 0.1) << "error in expected target at hour " << h;

        dispatchAuto->dispatch(0, h, 0);
        EXPECT_NEAR(batteryPower->powerBatteryDC, dispatchedkW[h], 0.1) << "error in dispatched power at hour " << h;
        EXPECT_NEAR(dispatchAuto->battery_soc(), SOC[h], 0.1) << "error in SOC at hour " << h;

    }
}

TEST_F(AutoFOM_lib_battery_dispatch, DispatchFOM_DCAutoWithInterconnectionLimit) {
    double dtHour = 1;
    CreateBattery(dtHour);
    interconnection_limit = 50000;
    dispatchAuto = new dispatch_automatic_front_of_meter_t(batteryModel, dtHour, 10, 100, 1, 49960, 49960, max_power,
        max_power, max_power, max_power, 1, dispatch_t::FOM_AUTOMATED_ECONOMIC, dispatch_t::WEATHER_FORECAST_CHOICE::WF_LOOK_AHEAD, dispatch_t::FRONT, 1, 18, 1, true, true, false,
        false, false, 77000, replacementCost, 1, cyclingCost, omCost, ppaRate, ur, 98, 98, 98, interconnection_limit, cleared_capacity, capacity_forecast_type, cleared_cap_percent);

    // battery setup
    dispatchAuto->update_pv_data(pv);
    dispatchAuto->update_cliploss_data(clip);
    batteryPower = dispatchAuto->getBatteryPower();
    batteryPower->connectionMode = ChargeController::DC_CONNECTED;
    batteryPower->voltageSystem = 600;
    batteryPower->setSharedInverter(m_sharedInverter);

    std::vector<double> targetkW = { 29579.89, -19294.27, 9062.46, -35296.51, -35570.30, -34754.11, // 0 - 5
                                         -16196.2, -2262.25, -314.26, -44.14, 0., 0., // 6 - 11
                                        0., 0., 0., 0., 0., 77000, // 12 - 17: 77000 is due to AC/DC efficiency issue - doesn't effect dispatched kWh (caught later by constraints)
                                        51300.39, 51296.17, 51289.15, 51275.27, 51234.85, 0. }; // 18 - 23
    std::vector<double> dispatchedkW = { 27100.68, -19294.27, 9062.46, -28825.12, -29057.17, -29202.51, // 0 - 5
                                         -16196.2, -2262.25, -314.26, -44.14, 0., 0., // 6 - 11
                                        0., 0., 0., 0., 0., 28116.27, // 18
                                        27947.03, 27664.92, 27100.53, 25407.20, 9380.09, 0. }; // 24
    std::vector<double> SOC = { 33.33, 44.65, 39.20, 55.86, 72.53, 89.20, //  0 - 5
                                98.49, 99.8, 99.97, 99.99, 99.99, 99.99, // 6 - 11
                                99.99, 99.99, 99.99, 99.99, 99.99, 83.33, // 12 - 17
                                66.66, 50.0, 33.33, 16.66, 10.0, 10.0 }; // 18 - 23
    for (size_t h = 0; h < 24; h++) {
        batteryPower->powerGeneratedBySystem = pv[h];
        batteryPower->powerSystem = pv[h];
        batteryPower->powerSystemClipped = clip[h];

        dispatchAuto->update_dispatch(0, h, 0, h);
        EXPECT_NEAR(batteryPower->powerBatteryTarget, targetkW[h], 0.1) << "error in expected target at hour " << h;

        dispatchAuto->dispatch(0, h, 0);
        EXPECT_NEAR(batteryPower->powerBatteryDC, dispatchedkW[h], 0.1) << "error in dispatched power at hour " << h;
        EXPECT_NEAR(dispatchAuto->battery_soc(), SOC[h], 0.1) << "error in SOC at hour " << h;

    }
}

TEST_F(AutoFOM_lib_battery_dispatch, DispatchFOM_ACAutoWithInterconnectionLimit) {
    double dtHour = 1;
    CreateBattery(dtHour);
    interconnection_limit = 50000;
    dispatchAuto = new dispatch_automatic_front_of_meter_t(batteryModel, dtHour, 10, 100, 1, 49960, 49960, max_power,
        max_power, max_power, max_power, 1, dispatch_t::FOM_AUTOMATED_ECONOMIC, dispatch_t::WEATHER_FORECAST_CHOICE::WF_LOOK_AHEAD, dispatch_t::FRONT, 1, 18, 1, true, false, false,
        false, false, 77000, replacementCost, 1, cyclingCost, omCost, ppaRate, ur, 98, 98, 98, interconnection_limit, cleared_capacity, capacity_forecast_type, cleared_cap_percent);

    // battery setup
    dispatchAuto->update_pv_data(pv); // PV Resource is available for the 1st 10 hrs
    dispatchAuto->update_cliploss_data(clip); // Clip charging is available for the 1st 5 hrs
    batteryPower = dispatchAuto->getBatteryPower();
    batteryPower->connectionMode = ChargeController::AC_CONNECTED;
    batteryPower->voltageSystem = 600;
    batteryPower->setSharedInverter(m_sharedInverter);

    // A few hours at the beginnging flip from discharge to charge since the PV will be curtailed
    // Note on hour 3: the price here is both the 2nd highest price and the 2nd lowest price. A non-zero replacement cost could prevent cycling here.
    std::vector<double> targetkW = { 29579.89, -19294.27, 5745.10, -35296.51, -35570.30, -34754.11, // 0 - 5
                                    -84205.39, -78854.6, -67702.19, -31516.09, 0., 0., // 6 - 11
                                    0., 0., 0., 0., 0., 52083.3, // 12 - 17
                                    52083.3, 52083.3, 52083.3, 52083.3, 52083.3, 0. }; // 18 - 23
    std::vector<double> dispatchedkW = { 27100.68, -19294.27, 5745.10, -28860.23, -29078.17, -29216.46, // 0 - 5
                                -15482.32, 0.0, 0.0, 0.0, 0., 0.,// 6 - 11
                                0., 0., 0., 0., 0., 28116.30, // 12 - 17
                                27947.03, 27664.92, 27100.69, 25408.03, 9385.81, 0. }; // 18 - 23
    std::vector<double> SOC = { 33.3, 44.65, 41.21, 57.88, 74.54, 91.21, // 0 - 5
                                100.0, 100.0, 100.0, 100.0, 100.0, 100.0, // 6 - 11
                                100.0, 100.0, 100.0, 100.0, 100.0, 83.33, // 12 - 17
                                66.66, 50.0, 33.33, 16.66, 10.0, 10.0 }; // 18 - 23
    for (size_t h = 0; h < 24; h++) {
        batteryPower->powerGeneratedBySystem = pv[h];
        batteryPower->powerSystem = pv[h];
        batteryPower->powerSystemClipped = clip[h];

        dispatchAuto->update_dispatch(0, h, 0, h);
        EXPECT_NEAR(batteryPower->powerBatteryTarget, targetkW[h], 0.1) << "error in expected target at hour " << h;

        dispatchAuto->dispatch(0, h, 0);

        EXPECT_NEAR(batteryPower->powerBatteryDC, dispatchedkW[h], 0.1) << "error in dispatched power at hour " << h;
        EXPECT_NEAR(dispatchAuto->battery_soc(), SOC[h], 0.1) << "error in SOC at hour " << h;
    }
}

TEST_F(AutoFOM_lib_battery_dispatch, DispatchFOM_ACAutoWithClearedCapacity) {
    double dtHour = 1;
    CreateBattery(dtHour);
    capacity_forecast_type = dispatch_t::CAPACITY_ONLY;
    for (int i = 0; i < 48; i++) {
        cleared_capacity.push_back(5000);
    }
    cleared_cap_percent = 1.0;
    dispatchAuto = new dispatch_automatic_front_of_meter_t(batteryModel, dtHour, 10, 100, 1, 49960, 49960, max_power,
        max_power, max_power, max_power, 1, dispatch_t::FOM_AUTOMATED_ECONOMIC, dispatch_t::WEATHER_FORECAST_CHOICE::WF_LOOK_AHEAD, dispatch_t::FRONT, 1, 18, 1, true, false, false,
        false, false, 77000, replacementCost, 1, cyclingCost, omCost, ppaRate, ur, 98, 98, 98, interconnection_limit, cleared_capacity, capacity_forecast_type, cleared_cap_percent);

    // battery setup
    dispatchAuto->update_pv_data(pv); // PV Resource is available for the 1st 10 hrs
    dispatchAuto->update_cliploss_data(clip); // Clip charging is available for the 1st 5 hrs
    batteryPower = dispatchAuto->getBatteryPower();
    batteryPower->connectionMode = ChargeController::AC_CONNECTED;
    batteryPower->voltageSystem = 600;
    batteryPower->setSharedInverter(m_sharedInverter);

    std::vector<double> targetkW = { -16603.29, -65098.19, -37905.31, -78496.51, -78770.30, -77954.11, // 0 - 5
                                    -79205.39, -73854.6, -62702.19, -26516.09, 5208.33, 5208.33, // 6 - 11
                                    5208.33, 5208.33, 5208.33, 5208.33, 5208.33, 5208.33, // 12 - 17
                                    5208.33, 5208.33, 5208.33, 5208.33, 5208.33, 5208.33 }; // 18 - 23
    std::vector<double> dispatchedkW = { -16603.29, -29095.73, -29228.24, -12338.96, 0.0, 0.0, // 0 - 5
                               0.0, 0.0, 0.0, 0.0, 5208.33, 5208.33,// 6 - 11
                                5208.33, 5208.33, 5208.33, 5208.33, 5208.33, 5208.33, // 12 - 17
                                5208.33, 5208.33, 5208.33, 5208.33, 5208.33, 5208.33 }; // 18 - 23
    std::vector<double> SOC = { 59.65, 76.31, 92.98, 100.0, 100.0, 100.0, // 0 - 5
                                100.0, 100.0, 100.0, 100.0, 96.96, 93.93, // 6 - 11
                                90.90, 87.86, 84.82, 81.78, 78.73, 75.68, // 12 - 17
                                72.63, 69.57, 66.51, 63.45, 60.38, 57.31 }; // 18 - 23
    for (size_t h = 0; h < 24; h++) {
        batteryPower->powerGeneratedBySystem = pv[h];
        batteryPower->powerSystem = pv[h];
        batteryPower->powerSystemClipped = clip[h];

        dispatchAuto->update_dispatch(0, h, 0, h);
        EXPECT_NEAR(batteryPower->powerBatteryTarget, targetkW[h], 0.1) << "error in expected target at hour " << h;

        dispatchAuto->dispatch(0, h, 0);

        EXPECT_NEAR(batteryPower->powerBatteryDC, dispatchedkW[h], 0.1) << "error in dispatched power at hour " << h;
        EXPECT_NEAR(dispatchAuto->battery_soc(), SOC[h], 0.1) << "error in SOC at hour " << h;
    }
}

TEST_F(AutoFOM_lib_battery_dispatch, DispatchFOM_ACAutoWithClearedCapacityAndPrice) {
    double dtHour = 1;
    CreateBattery(dtHour);
    capacity_forecast_type = dispatch_t::CAPACITY_ONLY;
    for (int i = 0; i < 48; i++) {
        cleared_capacity.push_back(2500);
    }
    cleared_cap_percent = 0.5;
    dispatchAuto = new dispatch_automatic_front_of_meter_t(batteryModel, dtHour, 10, 100, 1, 49960, 49960, max_power,
        max_power, max_power, max_power, 1, dispatch_t::FOM_AUTOMATED_ECONOMIC, dispatch_t::WEATHER_FORECAST_CHOICE::WF_LOOK_AHEAD, dispatch_t::FRONT, 1, 18, 1, true, false, false,
        false, false, 77000, replacementCost, 1, cyclingCost, omCost, ppaRate, ur, 98, 98, 98, interconnection_limit, cleared_capacity, capacity_forecast_type, cleared_cap_percent);

    // battery setup
    dispatchAuto->update_pv_data(pv); // PV Resource is available for the 1st 10 hrs
    dispatchAuto->update_cliploss_data(clip); // Clip charging is available for the 1st 5 hrs
    batteryPower = dispatchAuto->getBatteryPower();
    batteryPower->connectionMode = ChargeController::AC_CONNECTED;
    batteryPower->voltageSystem = 600;
    batteryPower->setSharedInverter(m_sharedInverter);

    std::vector<double> targetkW = { -15939.16, -62494.27, -37905.31, -78496.51, -78770.30, -77954.11, // 0 - 5
                                    -79205.39, -73854.6, -62702.19, -26516.09, 5208.33, 5208.33, // 6 - 11
                                    5208.33, 5208.33, 5208.33, 5208.33, 5208.33, 5208.33, // 12 - 17
                                    5208.33, 5208.33, 5208.33, 5208.33, 5208.33, 5208.33 }; // 18 - 23
    std::vector<double> dispatchedkW = { -15939.16, -29092.03, -29225.756, -13013.12, 0.0, 0.0, // 0 - 5
                               0.0, 0.0, 0.0, 0.0, 5208.33, 5208.33,// 6 - 11
                                5208.33, 5208.33, 5208.33, 5208.33, 5208.33, 5208.33, // 12 - 17
                                5208.33, 5208.33, 5208.33, 5208.33, 5208.33, 5208.33 }; // 18 - 23
    std::vector<double> SOC = { 59.27, 75.93, 92.60, 100.0, 100.0, 100.0, // 0 - 5
                                100.0, 100.0, 100.0, 100.0, 96.96, 93.93, // 6 - 11
                                90.90, 87.86, 84.82, 81.78, 78.73, 75.68, // 12 - 17
                                72.63, 69.57, 66.51, 63.45, 60.38, 57.31 }; // 18 - 23
    for (size_t h = 0; h < 24; h++) {
        batteryPower->powerGeneratedBySystem = pv[h];
        batteryPower->powerSystem = pv[h];
        batteryPower->powerSystemClipped = clip[h];

        dispatchAuto->update_dispatch(0, h, 0, h);
        EXPECT_NEAR(batteryPower->powerBatteryTarget, targetkW[h], 0.1) << "error in expected target at hour " << h;

        dispatchAuto->dispatch(0, h, 0);

        EXPECT_NEAR(batteryPower->powerBatteryDC, dispatchedkW[h], 0.1) << "error in dispatched power at hour " << h;
        EXPECT_NEAR(dispatchAuto->battery_soc(), SOC[h], 0.1) << "error in SOC at hour " << h;
    }
}

TEST_F(AutoFOM_lib_battery_dispatch, DispatchFOM_ACAutoWithAvailabilityLosses) {
    double dtHour = 1;
    CreateBatteryWithAvailabilityLosses(dtHour);
    dispatchAuto = new dispatch_automatic_front_of_meter_t(batteryModel, dtHour, 10, 100, 1, 49960, 49960, max_power,
        max_power, max_power, max_power, 1, dispatch_t::FOM_AUTOMATED_ECONOMIC, dispatch_t::WEATHER_FORECAST_CHOICE::WF_LOOK_AHEAD, dispatch_t::FRONT, 1, 18, 1, true, true, false,
        false, false, 77000, replacementCost, 1, cyclingCost, omCost, ppaRate, ur, 98, 98, 98, interconnection_limit, cleared_capacity, capacity_forecast_type, cleared_cap_percent);

    // battery setup
    dispatchAuto->update_pv_data(pv); // PV Resource is available for the 1st 10 hrs
    dispatchAuto->update_cliploss_data(clip); // Clip charging is available for the 1st 5 hrs
    batteryPower = dispatchAuto->getBatteryPower();
    batteryPower->connectionMode = ChargeController::AC_CONNECTED;
    batteryPower->voltageSystem = 600;
    batteryPower->setSharedInverter(m_sharedInverter);

    std::vector<double> targetkW = { 77000, 77000, 77000, -7205.42, 77000, 0.,
                                    -84205.39, -78854.60, -67702.19, -31516.09, 0., 0.,
                                    0., 0., 0., 0., 0., 77000,
                                    77000, 77000, 77000, 0., 0., 0. };
    std::vector<double> dispatchedkW = { 27100.68, 25407.98, 9385.55, -7205.42, 6616.55, 0.,
                                -17308.3, -20907.06, -21302.5, -21520.97, 0., 0.,
                                0., 0., 0., 0., 0., 20716.59,
                                20321.20, 19387.11, 13348.65, 0., 0., 0. };
    std::vector<double> SOC = { 33.3, 16.66, 10.0, 14.68, 10.0, 10.0,
                                18.3, 30.85, 43.35, 55.85, 55.85, 55.85,
                                55.85, 55.85, 55.85, 55.85, 55.85, 43.35,
                                30.85, 18.35, 8.35, 8.35, 8.35, 8.35 };
    // Battery was already discharging at max power, it stays unchanged
    for (size_t h = 0; h < 24; h++) {
        batteryPower->powerGeneratedBySystem = pv[h];
        batteryPower->powerSystem = pv[h];
        batteryPower->powerSystemClipped = clip[h];
        batteryPower->adjustLosses = batteryModel->getAvailabilityLoss(h);

        dispatchAuto->update_dispatch(0, h, 0, h);
        EXPECT_NEAR(batteryPower->powerBatteryTarget, targetkW[h], 0.1) << "error in expected target at hour " << h;

        dispatchAuto->dispatch(0, h, 0);

        EXPECT_NEAR(batteryPower->powerBatteryDC, dispatchedkW[h], 0.1) << "error in dispatched power at hour " << h;
        EXPECT_NEAR(dispatchAuto->battery_soc(), SOC[h], 0.1) << "error in SOC at hour " << h;
    }
}
