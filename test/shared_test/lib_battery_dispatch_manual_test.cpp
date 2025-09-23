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


#include "lib_battery_dispatch_manual_test.h"

size_t year = 0;
size_t hour_of_year = 0;
size_t step_of_hour = 0;

TEST_F(ManualTest_lib_battery_dispatch, PowerLimitsDispatchManualAC) {
    dispatchManual = new dispatch_manual_t(batteryModel, dtHour, SOC_min, SOC_max, currentChoice, currentChargeMax,
                                           currentDischargeMax, powerChargeMax, powerDischargeMax, powerChargeMax,
                                           powerDischargeMax, minimumModeTime,
                                           dispatchChoice, meterPosition, scheduleWeekday, scheduleWeekend, canCharge,
                                           canDischarge, canGridcharge, canDischargeToGrid, canGridcharge, percentDischarge,
                                           percentGridcharge, canClipCharge, canCurtailCharge, interconnection_limit);

    batteryPower = dispatchManual->getBatteryPower();
    batteryPower->connectionMode = ChargeController::AC_CONNECTED;
    double powerToFill = dispatchManual->battery_power_to_fill();
    EXPECT_NEAR(dispatchManual->battery_soc(), 50, 0.1);

    // Test max charge power constraint
    batteryPower->powerSystem = 1000;
    batteryPower->voltageSystem = 600;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    EXPECT_NEAR(batteryPower->powerBatteryAC, -powerChargeMax, 2.0);
    EXPECT_LT(dispatchManual->battery_power_to_fill(),
              powerToFill); // Confirm battery power moves in the expected direction

    powerToFill = dispatchManual->battery_power_to_fill();
    // Test max discharge power constraint
    batteryPower->powerSystem = 0;
    batteryPower->voltageSystem = 600;
    batteryPower->powerLoad = 1000;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    EXPECT_NEAR(batteryPower->powerBatteryDC, powerDischargeMax, 2.0);
    EXPECT_GT(dispatchManual->battery_power_to_fill(), powerToFill);
}

TEST_F(ManualTest_lib_battery_dispatch, PowerLimitsDispatchManualDC) {
    dispatchManual = new dispatch_manual_t(batteryModel, dtHour, SOC_min, SOC_max, currentChoice, currentChargeMax,
        currentDischargeMax, powerChargeMax, powerDischargeMax, powerChargeMax,
        powerDischargeMax, minimumModeTime,
        dispatchChoice, meterPosition, scheduleWeekday, scheduleWeekend, canCharge,
        canDischarge, canGridcharge, canDischargeToGrid, canGridcharge, percentDischarge,
        percentGridcharge, canClipCharge, canCurtailCharge, interconnection_limit);

    batteryPower = dispatchManual->getBatteryPower();
    batteryPower->connectionMode = ChargeController::DC_CONNECTED;
    batteryPower->setSharedInverter(m_sharedInverter);

    // Test max charge power constraint
    batteryPower->powerSystem = 1000;
    batteryPower->voltageSystem = 600;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    EXPECT_NEAR(batteryPower->powerBatteryAC, -powerChargeMax * batteryPower->sharedInverter->getMaxPowerEfficiency() * 0.01, 2.0);

    // Test max discharge power constraint
    batteryPower->powerSystem = 0;
    batteryPower->voltageSystem = 600;
    batteryPower->powerLoad = 1000;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    EXPECT_NEAR(batteryPower->powerBatteryDC, powerDischargeMax, 2.0);
}

TEST_F(ManualTest_lib_battery_dispatch, CurrentLimitsDispatchManualAC) {
    dispatch_t::CURRENT_CHOICE testChoice = dispatch_t::CURRENT_CHOICE::RESTRICT_CURRENT;
    double testChargeMax = 20;
    double testDischargeMax = 20;
    dispatchManual = new dispatch_manual_t(batteryModel, dtHour, SOC_min, SOC_max, testChoice, testChargeMax,
        testDischargeMax, powerChargeMax, powerDischargeMax, powerChargeMax,
        powerDischargeMax, minimumModeTime,
        dispatchChoice, meterPosition, scheduleWeekday, scheduleWeekend, canCharge,
        canDischarge, canGridcharge, canDischargeToGrid, canGridcharge, percentDischarge,
        percentGridcharge, canClipCharge, canCurtailCharge, interconnection_limit);

    batteryPower = dispatchManual->getBatteryPower();
    batteryPower->connectionMode = ChargeController::AC_CONNECTED;

    // Test max charge current constraint
    batteryPower->powerSystem = 1000;
    batteryPower->voltageSystem = 600;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    double current = batteryPower->powerBatteryDC * util::kilowatt_to_watt / batteryPower->voltageSystem;
    EXPECT_NEAR(current, -testChargeMax, 2.0);

    // Test max discharge current constraint
    batteryPower->powerSystem = 0;
    batteryPower->voltageSystem = 600;
    batteryPower->powerLoad = 1000;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    current = batteryPower->powerBatteryDC * util::kilowatt_to_watt / batteryPower->voltageSystem;
    EXPECT_NEAR(current, testDischargeMax, 2.0);
}

TEST_F(ManualTest_lib_battery_dispatch, CurrentLimitsDispatchManualDC) {
    dispatch_t::CURRENT_CHOICE testChoice = dispatch_t::CURRENT_CHOICE::RESTRICT_CURRENT;
    double testChargeMax = 20;
    double testDischargeMax = 20;
    dispatchManual = new dispatch_manual_t(batteryModel, dtHour, SOC_min, SOC_max, testChoice, testChargeMax,
        testDischargeMax, powerChargeMax, powerDischargeMax, powerChargeMax,
        powerDischargeMax, minimumModeTime,
        dispatchChoice, meterPosition, scheduleWeekday, scheduleWeekend, canCharge,
        canDischarge, canGridcharge, canDischargeToGrid, canGridcharge, percentDischarge,
        percentGridcharge, canClipCharge, canCurtailCharge, interconnection_limit);

    batteryPower = dispatchManual->getBatteryPower();
    batteryPower->connectionMode = ChargeController::DC_CONNECTED;
    batteryPower->setSharedInverter(m_sharedInverter);

    // Test max charge constraints
    batteryPower->powerSystem = 1000;
    batteryPower->voltageSystem = 600;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    double current = batteryPower->powerBatteryDC * util::kilowatt_to_watt / batteryPower->voltageSystem;
    EXPECT_NEAR(current, -testChargeMax, 2.0);

    // Test max discharge current constraint
    batteryPower->powerSystem = 0;
    batteryPower->voltageSystem = 600;
    batteryPower->powerLoad = 1000;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    current = batteryPower->powerBatteryDC * util::kilowatt_to_watt / batteryPower->voltageSystem;
    EXPECT_NEAR(current, testDischargeMax, 2.0);
}

TEST_F(ManualTest_lib_battery_dispatch, BothLimitsDispatchManualAC) {
    dispatch_t::CURRENT_CHOICE testChoice = dispatch_t::CURRENT_CHOICE::RESTRICT_BOTH;
    double testChargeMax = 20;
    double testDischargeMax = 20;
    double testDischargeMaxPower = 11; // kW
    double testChargeMaxPower = 12; // kW
    dispatchManual = new dispatch_manual_t(batteryModel, dtHour, SOC_min, SOC_max, testChoice, testChargeMax,
        testDischargeMax, testChargeMaxPower, testDischargeMaxPower, testChargeMaxPower,
        testDischargeMaxPower, minimumModeTime,
        dispatchChoice, meterPosition, scheduleWeekday, scheduleWeekend, canCharge,
        canDischarge, canGridcharge, canDischargeToGrid, canGridcharge, percentDischarge,
        percentGridcharge, canClipCharge, canCurtailCharge, interconnection_limit);

    batteryPower = dispatchManual->getBatteryPower();
    batteryPower->connectionMode = ChargeController::AC_CONNECTED;

    // Test max charge current constraint
    batteryPower->powerSystem = 1000;
    batteryPower->voltageSystem = 600;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    double current = batteryPower->powerBatteryDC * util::kilowatt_to_watt / batteryPower->voltageSystem;
    EXPECT_NEAR(current, -testChargeMax, 2.0);

    hour_of_year += 1;

    // Test max charge power constraint
    batteryPower->powerSystem = 1000;
    batteryPower->voltageSystem = 1200;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    EXPECT_NEAR(batteryPower->powerBatteryAC, -testChargeMaxPower, 2.0);

    hour_of_year += 1;

    // Test max discharge constraints
    batteryPower->powerSystem = 0;
    batteryPower->voltageSystem = 600;
    batteryPower->powerLoad = 1000;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    current = batteryPower->powerBatteryDC * util::kilowatt_to_watt / batteryPower->voltageSystem;
    EXPECT_NEAR(current, testDischargeMax, 2.0);

    hour_of_year += 1;

    // Test max discharge power constraint
    batteryPower->powerSystem = 0;
    batteryPower->voltageSystem = 1200;
    batteryPower->powerLoad = 1000;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    EXPECT_NEAR(batteryPower->powerBatteryAC, testDischargeMaxPower, 2.0);
}

TEST_F(ManualTest_lib_battery_dispatch, BothLimitsDispatchManualDC) {
    dispatch_t::CURRENT_CHOICE testChoice = dispatch_t::CURRENT_CHOICE::RESTRICT_BOTH;
    double testChargeMax = 20; // Amps
    double testDischargeMax = 20; // Amps
    double testDischargeMaxPower = 11; // kW
    double testChargeMaxPower = 11; // kW
    dispatchManual = new dispatch_manual_t(batteryModel, dtHour, SOC_min, SOC_max, testChoice, testChargeMax,
        testDischargeMax, testChargeMaxPower, testDischargeMaxPower, powerChargeMax,
        powerDischargeMax, minimumModeTime,
        dispatchChoice, meterPosition, scheduleWeekday, scheduleWeekend, canCharge,
        canDischarge, canGridcharge, canDischargeToGrid, canGridcharge, percentDischarge,
        percentGridcharge, canClipCharge, canCurtailCharge, interconnection_limit);

    batteryPower = dispatchManual->getBatteryPower();
    batteryPower->connectionMode = ChargeController::DC_CONNECTED;
    batteryPower->setSharedInverter(m_sharedInverter);

    // Test max charge current constraint
    batteryPower->powerSystem = 1000;
    batteryPower->voltageSystem = 600;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    double current = batteryPower->powerBatteryDC * util::kilowatt_to_watt / batteryPower->voltageSystem;
    EXPECT_NEAR(current, -testChargeMax, 2.0);

    hour_of_year += 1;

    // Test max discharge current constraint
    batteryPower->powerSystem = 0;
    batteryPower->voltageSystem = 600;
    batteryPower->powerLoad = 1000;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    current = batteryPower->powerBatteryDC * util::kilowatt_to_watt / batteryPower->voltageSystem;
    EXPECT_NEAR(current, testDischargeMax, 2.0);

    hour_of_year += 1;

    // Test max charge power constraint
    batteryPower->powerSystem = 1000;
    batteryPower->voltageSystem = 1200;
    batteryPower->powerLoad = 0;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    EXPECT_NEAR(batteryPower->powerBatteryDC, -testChargeMaxPower, 2.0);

    hour_of_year += 1;

    // Test max discharge power constraint
    batteryPower->powerSystem = 0;
    batteryPower->voltageSystem = 1200;
    batteryPower->powerLoad = 1000;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    EXPECT_NEAR(batteryPower->powerBatteryDC, testDischargeMaxPower, 2.0);
}

TEST_F(ManualTest_lib_battery_dispatch, DispatchChangeFrequency) {
    double testTimestep = 1.0 / 60.0; // Minute timesteps
    double testMinTime = 4.0; // Only allow dispatch to change every 3 mins
    dispatchManual = new dispatch_manual_t(batteryModel, testTimestep, SOC_min, SOC_max, currentChoice, currentChargeMax,
        currentDischargeMax, powerChargeMax, powerDischargeMax, powerChargeMax,
        powerDischargeMax, testMinTime,
        dispatchChoice, meterPosition, scheduleWeekday, scheduleWeekend, canCharge,
        canDischarge, canGridcharge, canDischargeToGrid, canGridcharge, percentDischarge,
        percentGridcharge, canClipCharge, canCurtailCharge, interconnection_limit);

    batteryPower = dispatchManual->getBatteryPower();
    batteryPower->connectionMode = ChargeController::AC_CONNECTED;

    // Start by charging (0 minutes)
    batteryPower->powerSystem = 1000;
    batteryPower->voltageSystem = 600;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    EXPECT_NEAR(batteryPower->powerBatteryAC, -powerChargeMax, 2.0);

    // Abruptly cut off the PV and increase the load. Power should go to zero (1 minute)
    step_of_hour += 1;
    batteryPower->powerSystem = 0;
    batteryPower->voltageSystem = 600;
    batteryPower->powerLoad = 1000;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    EXPECT_NEAR(batteryPower->powerBatteryDC, 0.0, 0.1);

    // Same status (2nd minute)
    step_of_hour += 1;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    EXPECT_NEAR(batteryPower->powerBatteryDC, 0.0, 0.1);

    // Same status (3rd minute)
    step_of_hour += 1;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    EXPECT_NEAR(batteryPower->powerBatteryDC, 0.0, 0.1);

    // Same status (4th minute)
    step_of_hour += 1;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    EXPECT_NEAR(batteryPower->powerBatteryDC, 0.0, 2.0);

    // Should dispatch to load now (5th minute)
    step_of_hour += 1;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    EXPECT_NEAR(batteryPower->powerBatteryDC, powerChargeMax, 2.0);
}

TEST_F(ManualTest_lib_battery_dispatch, SOCLimitsOnDispatch) {
    hour_of_year = 0; step_of_hour = 0;
    dispatchManual = new dispatch_manual_t(batteryModel, dtHour, SOC_min, SOC_max, currentChoice, currentChargeMax,
        currentDischargeMax, powerChargeMax, powerDischargeMax, powerChargeMax,
        powerDischargeMax, minimumModeTime,
        dispatchChoice, meterPosition, scheduleWeekday, scheduleWeekend, canCharge,
        canDischarge, canGridcharge, canDischargeToGrid, canGridcharge, percentDischarge,
        percentGridcharge, canClipCharge, canCurtailCharge, interconnection_limit);

    batteryPower = dispatchManual->getBatteryPower();
    batteryPower->connectionMode = ChargeController::AC_CONNECTED;
    double soc = dispatchManual->battery_soc();
    EXPECT_NEAR(dispatchManual->battery_soc(), 50, 0.1);

    // Test dispatch iterations to fully charge
    batteryPower->powerSystem = 1000; batteryPower->voltageSystem = 600;
    while (soc < SOC_max && hour_of_year < 100) {
        dispatchManual->dispatch(year, hour_of_year, step_of_hour);
        hour_of_year += 1;
        EXPECT_GT(dispatchManual->battery_soc(), soc);
        soc = dispatchManual->battery_soc();
    }
    EXPECT_NEAR(SOC_max, dispatchManual->battery_soc(), 0.1);
    EXPECT_NEAR(6, hour_of_year, 0.1);

    // Attempt dispatch one more time, should not charge
    hour_of_year += 1;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    EXPECT_NEAR(batteryPower->powerBatteryDC, 0.0, 0.1);

    // Cut off PV and provide load
    batteryPower->powerSystem = 0; batteryPower->voltageSystem = 600; batteryPower->powerLoad = 1000;
    while (soc > SOC_min + tolerance && hour_of_year < 100) {
        dispatchManual->dispatch(year, hour_of_year, step_of_hour);
        hour_of_year += 1;
        soc = dispatchManual->battery_soc();
    }
    EXPECT_NEAR(SOC_min, dispatchManual->battery_soc(), 0.1);
    EXPECT_NEAR(16, hour_of_year, 0.1);

    // Cut off PV and provide load
    batteryPower->powerSystem = 0;
    batteryPower->voltageSystem = 600;
    batteryPower->powerLoad = 1000;
    while (soc > SOC_min + tolerance && hour_of_year < 100) {
        dispatchManual->dispatch(year, hour_of_year, step_of_hour);
        hour_of_year += 1;
        soc = dispatchManual->battery_soc();
    }
    EXPECT_NEAR(SOC_min, dispatchManual->battery_soc(), 0.1);
    EXPECT_NEAR(16, hour_of_year, 0.1);

    // Attempt dispatch one more time, should not discharge
    hour_of_year += 1;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    EXPECT_NEAR(batteryPower->powerBatteryDC, 0.0, 0.1);
}

TEST_F(ManualTest_lib_battery_dispatch, ManualGridChargingOffTest)
{
    // canGridCharge is false by default
    dispatchManual = new dispatch_manual_t(batteryModel, dtHour, SOC_min, SOC_max, currentChoice, currentChargeMax,
        currentDischargeMax, powerChargeMax, powerDischargeMax, powerChargeMax,
        powerDischargeMax, minimumModeTime,
        dispatchChoice, meterPosition, scheduleWeekday, scheduleWeekend, canCharge,
        canDischarge, canGridcharge, canDischargeToGrid, canGridcharge, percentDischarge,
        percentGridcharge, canClipCharge, canCurtailCharge, interconnection_limit);

    batteryPower = dispatchManual->getBatteryPower();
    batteryPower->connectionMode = ChargeController::DC_CONNECTED;
    batteryPower->setSharedInverter(m_sharedInverter);

    // Should not charge since grid charging is disallowed
    batteryPower->powerSystem = 0; batteryPower->voltageSystem = 600; batteryPower->powerGridToBattery = 1000;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    EXPECT_NEAR(batteryPower->powerBatteryDC, 0, 0.1);
    EXPECT_NEAR(batteryPower->powerGridToBattery, 0, 0.1);
}

TEST_F(ManualTest_lib_battery_dispatch, ManualGridChargingOnTest)
{
    std::vector<bool> testCanGridcharge;
    std::map<size_t, double> testPercentGridCharge;
    for (int p = 0; p < 6; p++) {
        testCanGridcharge.push_back(1);
        testPercentGridCharge[p] = 100;
    }
    dispatchManual = new dispatch_manual_t(batteryModel, dtHour, SOC_min, SOC_max, currentChoice, currentChargeMax,
        currentDischargeMax, powerChargeMax, powerDischargeMax, powerChargeMax,
        powerDischargeMax, minimumModeTime,
        dispatchChoice, meterPosition, scheduleWeekday, scheduleWeekend, canCharge,
        canDischarge, testCanGridcharge, canDischargeToGrid, canGridcharge, percentDischarge,
        testPercentGridCharge, canClipCharge, canCurtailCharge, interconnection_limit);

    batteryPower = dispatchManual->getBatteryPower();
    batteryPower->connectionMode = ChargeController::AC_CONNECTED;
    batteryPower->inverterEfficiencyCutoff = 0;

    // Test grid charging. AC to DC losses come into play
    batteryPower->powerSystem = 0; batteryPower->voltageSystem = 600; batteryPower->powerGridToBattery = 1000;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    EXPECT_NEAR(batteryPower->powerBatteryDC, -powerChargeMax * batteryPower->singlePointEfficiencyACToDC, 1.0);
    EXPECT_NEAR(batteryPower->powerGridToBattery, powerChargeMax, 2.0);
}

TEST_F(ManualTest_lib_battery_dispatch, ManualGridChargingOnDCConnectedTest)
{
    std::vector<bool> testCanGridcharge;
    std::map<size_t, double> testPercentGridCharge;
    for (int p = 0; p < 6; p++) {
        testCanGridcharge.push_back(1);
        testPercentGridCharge[p] = 100;
    }
    dispatchManual = new dispatch_manual_t(batteryModel, dtHour, SOC_min, SOC_max, currentChoice, currentChargeMax,
        currentDischargeMax, powerChargeMax, powerDischargeMax, powerChargeMax,
        powerDischargeMax, minimumModeTime,
        dispatchChoice, meterPosition, scheduleWeekday, scheduleWeekend, canCharge,
        canDischarge, testCanGridcharge, canDischargeToGrid, canGridcharge, percentDischarge,
        testPercentGridCharge, canClipCharge, canCurtailCharge, interconnection_limit);

    batteryPower = dispatchManual->getBatteryPower();
    batteryPower->connectionMode = ChargeController::DC_CONNECTED;
    batteryPower->setSharedInverter(m_sharedInverter);
    batteryPower->inverterEfficiencyCutoff = 0;

    // Test grid charging. AC to DC losses come into play
    batteryPower->powerSystem = 0; batteryPower->voltageSystem = 600; batteryPower->powerGridToBattery = 1000;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    EXPECT_NEAR(batteryPower->powerBatteryDC, -powerChargeMax * batteryPower->singlePointEfficiencyACToDC, 1.0);
    EXPECT_NEAR(batteryPower->powerGridToBattery, powerChargeMax, 2.0);
}

TEST_F(ManualTest_lib_battery_dispatch, NoGridChargingWhilePVIsOnTest)
{
    std::vector<bool> testCanGridcharge;
    std::map<size_t, double> testPercentGridCharge;
    for (int p = 0; p < 6; p++) {
        testCanGridcharge.push_back(1);
        testPercentGridCharge[p] = 100;
    }
    dispatchManual = new dispatch_manual_t(batteryModel, dtHour, SOC_min, SOC_max, currentChoice, currentChargeMax,
        currentDischargeMax, powerChargeMax, powerDischargeMax, powerChargeMax,
        powerDischargeMax, minimumModeTime,
        dispatchChoice, meterPosition, scheduleWeekday, scheduleWeekend, canCharge,
        canDischarge, testCanGridcharge, canDischargeToGrid, canGridcharge, percentDischarge,
        testPercentGridCharge, canClipCharge, canCurtailCharge, interconnection_limit);

    batteryPower = dispatchManual->getBatteryPower();
    batteryPower->connectionMode = ChargeController::AC_CONNECTED;

    // Test no grid charging while PV is on
    batteryPower->powerSystem = 1000; batteryPower->voltageSystem = 600; batteryPower->powerGridToBattery = 1000;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    EXPECT_NEAR(batteryPower->powerBatteryAC, -powerChargeMax, 1.0);
    EXPECT_NEAR(batteryPower->powerGridToBattery, 0.0, 2.0);
}

TEST_F(ManualTest_lib_battery_dispatch, EfficiencyLimitsDispatchManualDC)
{
    dispatchManual = new dispatch_manual_t(batteryModel, dtHour, SOC_min, SOC_max, currentChoice, currentChargeMax,
        currentDischargeMax, powerChargeMax, powerDischargeMax, powerChargeMax,
        powerDischargeMax, minimumModeTime,
        dispatchChoice, meterPosition, scheduleWeekday, scheduleWeekend, canCharge,
        canDischarge, canGridcharge, canDischargeToGrid, canGridcharge, percentDischarge,
        percentGridcharge, canClipCharge, canCurtailCharge, interconnection_limit);

    batteryPower = dispatchManual->getBatteryPower();
    batteryPower->connectionMode = ChargeController::DC_CONNECTED;
    batteryPower->setSharedInverter(m_sharedInverter);

    // Test max charge power constraint
    batteryPower->powerSystem = 1000; batteryPower->voltageSystem = 600;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    EXPECT_NEAR(batteryPower->powerBatteryAC, -powerChargeMax * batteryPower->sharedInverter->getMaxPowerEfficiency() * 0.01, 2.0);

    // Test max discharge power constraint
    batteryPower->powerSystem = 0; batteryPower->voltageSystem = 600; batteryPower->powerLoad = 1000;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    EXPECT_NEAR(batteryPower->powerBatteryDC, powerDischargeMax, 2.0);
}

TEST_F(ManualTest_lib_battery_dispatch, InverterEfficiencyCutoffDC)
{
    std::vector<bool> testCanGridcharge;
    std::map<size_t, double> testPercentGridCharge;
    for (int p = 0; p < 6; p++) {
        testCanGridcharge.push_back(1);
        testPercentGridCharge[p] = 1;
    }
    testPercentGridCharge[1] = 1;
    testPercentGridCharge[3] = 100;
    dispatchManual = new dispatch_manual_t(batteryModel, dtHour, SOC_min, SOC_max, currentChoice, currentChargeMax,
        currentDischargeMax, powerChargeMax, powerDischargeMax, powerChargeMax,
        powerDischargeMax, minimumModeTime,
        dispatchChoice, meterPosition, scheduleWeekday, scheduleWeekend, canCharge,
        canDischarge, testCanGridcharge, canDischargeToGrid, canGridcharge, testPercentGridCharge, testPercentGridCharge, canClipCharge, canCurtailCharge, interconnection_limit);

    batteryPower = dispatchManual->getBatteryPower();
    batteryPower->connectionMode = ChargeController::DC_CONNECTED;
    batteryPower->setSharedInverter(m_sharedInverter);
    batteryPower->inverterEfficiencyCutoff = 80;
    batteryPower->canGridCharge = true;

    // Test inverter efficiency cutoff on grid charging
    batteryPower->powerSystem = 0; batteryPower->voltageSystem = 600; batteryPower->powerGridToBattery = 7;
    dispatchManual->dispatch(year, 0, step_of_hour);
    EXPECT_NEAR(batteryPower->sharedInverter->efficiencyAC, 0.0, 0.1); // Efficiency is 0 when system is not running
    EXPECT_NEAR(batteryPower->powerBatteryDC, 0.0, 0.1);

    batteryPower->powerSystem = 0; batteryPower->voltageSystem = 600; batteryPower->powerGridToBattery = 1000;
    dispatchManual->dispatch(year, 12, step_of_hour);
    EXPECT_NEAR(batteryPower->sharedInverter->efficiencyAC, 93.7, 0.1);
    EXPECT_NEAR(batteryPower->powerBatteryDC, -47.9, 0.1);

    // Test discharge constraints. First constraint does not hit backoff
    batteryPower->powerSystem = 0; batteryPower->voltageSystem = 600; batteryPower->powerGridToBattery = 0; batteryPower->powerLoad = 7;
    dispatchManual->dispatch(year, 0, step_of_hour);
    EXPECT_NEAR(batteryPower->sharedInverter->efficiencyAC, 35.96, 0.1); // Not enforced becasue no PV
    EXPECT_NEAR(batteryPower->powerBatteryDC, 4.43, 0.1);

    batteryPower->powerSystem = 770; batteryPower->voltageSystem = 600; batteryPower->powerGridToBattery = 0; batteryPower->powerLoad = 1000;
    dispatchManual->dispatch(year, 12, step_of_hour);
    EXPECT_NEAR(batteryPower->sharedInverter->efficiencyAC, 93.9, 0.1);
    EXPECT_NEAR(batteryPower->powerBatteryDC, 49.9, 0.1); // Dispatch both battery and PV

    batteryPower->powerSystem = 1000; batteryPower->voltageSystem = 600; batteryPower->powerGridToBattery = 0; batteryPower->powerLoad = 1100;
    dispatchManual->dispatch(year, 12, step_of_hour);
    EXPECT_NEAR(batteryPower->sharedInverter->efficiencyAC, 77, 0.1);
    EXPECT_NEAR(batteryPower->powerBatteryDC, 0.0, 2.0); // Overwhelm inverter with PV, back off battery
}

TEST_F(ManualTest_lib_battery_dispatch_losses, TestLossesWithDispatch)
{
    dispatchManual = new dispatch_manual_t(batteryModel, dtHour, SOC_min, SOC_max, currentChoice, currentChargeMax,
        currentDischargeMax, powerChargeMax, powerDischargeMax, powerChargeMax,
        powerDischargeMax, minimumModeTime,
        dispatchChoice, meterPosition, scheduleWeekday, scheduleWeekend, canCharge,
        canDischarge, canGridcharge, canDischargeToGrid, canGridcharge, percentDischarge,
        percentGridcharge, canClipCharge, canCurtailCharge, interconnection_limit);

    batteryPower = dispatchManual->getBatteryPower();
    batteryPower->connectionMode = ChargeController::DC_CONNECTED;
    batteryPower->setSharedInverter(m_sharedInverter);

    // Test max charge power constraint
    batteryPower->powerSystem = 40; batteryPower->voltageSystem = 600;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    EXPECT_NEAR(batteryPower->powerSystemToBatteryAC, batteryPower->powerSystem * batteryPower->sharedInverter->getMaxPowerEfficiency() * 0.01 - batteryPower->powerSystemLoss, 1);

    // Test max discharge power constraint
    batteryPower->powerSystem = 0; batteryPower->voltageSystem = 600; batteryPower->powerLoad = 40;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    EXPECT_NEAR(batteryPower->powerGeneratedBySystem, batteryPower->powerLoad, 0.5); // Constraints drive efficiency lower, meaning some grid power is used to meet load (<0.5 kW)
    EXPECT_NEAR(batteryPower->powerBatteryToLoad, batteryPower->powerLoad, 0.5);
}

TEST_F(ManualTest_lib_battery_dispatch_availability_losses, TestAvailabilityLossesWithDispatch)
{
    dispatchManual = new dispatch_manual_t(batteryModel, dtHour, SOC_min, SOC_max, currentChoice, currentChargeMax,
        currentDischargeMax, powerChargeMax, powerDischargeMax, powerChargeMax,
        powerDischargeMax, minimumModeTime,
        dispatchChoice, meterPosition, scheduleWeekday, scheduleWeekend, canCharge,
        canDischarge, canGridcharge, canDischargeToGrid, canGridcharge, percentDischarge,
        percentGridcharge, canClipCharge, canCurtailCharge, interconnection_limit);

    batteryPower = dispatchManual->getBatteryPower();
    batteryPower->connectionMode = ChargeController::DC_CONNECTED;
    batteryPower->setSharedInverter(m_sharedInverter);

    // Test max charge power constraint
    batteryPower->powerSystem = 40; batteryPower->voltageSystem = 600; batteryPower->adjustLosses = 0.5;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    EXPECT_NEAR(batteryPower->powerSystemToBatteryAC, powerChargeMax * batteryPower->adjustLosses - batteryPower->powerSystemLoss, 1);

    // Test max discharge power constraint
    batteryPower->powerSystem = 0; batteryPower->voltageSystem = 600; batteryPower->powerLoad = 40; batteryPower->adjustLosses = 0.5;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    EXPECT_NEAR(batteryPower->powerGeneratedBySystem, powerDischargeMax * batteryPower->adjustLosses * batteryPower->singlePointEfficiencyDCToAC, 0.5); // Constraints drive efficiency lower, meaning some grid power is used to meet load (<0.5 kW)
    EXPECT_NEAR(batteryPower->powerBatteryToLoad, powerDischargeMax * batteryPower->adjustLosses * batteryPower->singlePointEfficiencyDCToAC, 0.5);
}

TEST_F(ManualTest_lib_battery_dispatch, TestDischargeToGrid)
{
    std::vector<bool> testCanDischargeToGrid;
    for (int p = 0; p < 6; p++) {
        testCanDischargeToGrid.push_back(1);
    }
    dispatchManual = new dispatch_manual_t(batteryModel, dtHour, SOC_min, SOC_max, currentChoice, currentChargeMax,
        currentDischargeMax, powerChargeMax, powerDischargeMax, powerChargeMax,
        powerDischargeMax, minimumModeTime,
        dispatchChoice, meterPosition, scheduleWeekday, scheduleWeekend, canCharge,
        canDischarge, canGridcharge, canGridcharge, testCanDischargeToGrid, percentDischarge,
        percentGridcharge, canClipCharge, canCurtailCharge, interconnection_limit);

    batteryPower = dispatchManual->getBatteryPower();
    batteryPower->connectionMode = ChargeController::AC_CONNECTED;
    batteryPower->inverterEfficiencyCutoff = 0;

    // Test discharge to grid
    batteryPower->powerSystem = 0; batteryPower->powerLoad = 30; batteryPower->voltageSystem = 600; batteryPower->powerBatteryDC = 1000;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    EXPECT_NEAR(batteryPower->powerBatteryDC, powerDischargeMax, 1.0);
    EXPECT_NEAR(batteryPower->powerBatteryToLoad, batteryPower->powerLoad, 2.0);
    EXPECT_NEAR(batteryPower->powerBatteryToGrid, powerDischargeMax * batteryPower->singlePointEfficiencyDCToAC - batteryPower->powerLoad, 2.0);
}

TEST_F(ManualTest_lib_battery_dispatch, TestClipCharging)
{
    std::vector<bool> testDoNothingExceptClipCharge;
    for (int p = 0; p < 6; p++) {
        testDoNothingExceptClipCharge.push_back(0);
    }
    canClipCharge = true;
    dispatchManual = new dispatch_manual_t(batteryModel, dtHour, SOC_min, SOC_max, currentChoice, currentChargeMax,
        currentDischargeMax, powerChargeMax, powerDischargeMax, powerChargeMax,
        powerDischargeMax, minimumModeTime,
        dispatchChoice, meterPosition, scheduleWeekday, scheduleWeekend, testDoNothingExceptClipCharge,
        testDoNothingExceptClipCharge, testDoNothingExceptClipCharge, testDoNothingExceptClipCharge, testDoNothingExceptClipCharge, percentDischarge,
        percentGridcharge, canClipCharge, canCurtailCharge, interconnection_limit);

    batteryPower = dispatchManual->getBatteryPower();
    batteryPower->connectionMode = ChargeController::DC_CONNECTED;
    batteryPower->setSharedInverter(m_sharedInverter);

    // Test discharge to grid
    double clipped_power = 10;
    batteryPower->powerSystem = 100; batteryPower->powerLoad = 30; batteryPower->voltageSystem = 600; batteryPower->powerSystemClipped = clipped_power; batteryPower->powerBatteryDC = -clipped_power;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    EXPECT_NEAR(batteryPower->powerBatteryDC, -clipped_power, 0.1);
    EXPECT_NEAR(batteryPower->powerSystemToLoad, batteryPower->powerLoad, 0.1);
    EXPECT_NEAR(batteryPower->powerSystemToBatteryAC, clipped_power * batteryPower->sharedInverter->getMaxPowerEfficiency() * 0.01, 0.1);
}

TEST_F(ManualTest_lib_battery_dispatch, OutageWithManualDispatch) {
    hour_of_year = 0; step_of_hour = 0;
    dispatchManual = new dispatch_manual_t(batteryModel, dtHour, SOC_min, SOC_max, currentChoice, currentChargeMax,
        currentDischargeMax, powerChargeMax, powerDischargeMax, powerChargeMax,
        powerDischargeMax, minimumModeTime,
        dispatchChoice, meterPosition, scheduleWeekday, scheduleWeekend, canCharge,
        canDischarge, canGridcharge, canGridcharge, canGridcharge, percentDischarge,
        percentGridcharge, canClipCharge, canCurtailCharge, interconnection_limit);

    batteryPower = dispatchManual->getBatteryPower();
    batteryPower->connectionMode = ChargeController::AC_CONNECTED;
    double soc = dispatchManual->battery_soc();
    EXPECT_NEAR(dispatchManual->battery_soc(), 50, 0.1);

    // Test dispatch iterations to dishcarge during outage

    while (soc > 0.5 && hour_of_year < 100) {
        batteryPower->powerLoad = 50;
        batteryPower->powerSystem = 0;
        batteryPower->isOutageStep = true;
        batteryPower->powerCritLoad = 50;
        dispatchManual->dispatch(year, hour_of_year, step_of_hour);
        hour_of_year += 1;
        EXPECT_LT(dispatchManual->battery_soc(), soc);
        soc = dispatchManual->battery_soc();
    }
    EXPECT_NEAR(0.1, dispatchManual->battery_soc(), 0.1);
    EXPECT_NEAR(6, hour_of_year, 0.1);

    // Attempt dispatch one more time after outage, should not discharge
    batteryPower->isOutageStep = false;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    hour_of_year += 1;
    EXPECT_NEAR(batteryPower->powerBatteryDC, 0.0, 0.1);

    // Can charge below min SOC
    batteryPower->powerSystem = 700;
    batteryPower->powerLoad = 50;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    hour_of_year += 1;
    EXPECT_NEAR(batteryPower->powerBatteryDC, -47.7, 0.1);

    // Turn outage back on, should charge to 100%
    batteryPower->powerSystem = 700; batteryPower->voltageSystem = 600; batteryPower->powerLoad = 50;
    batteryPower->powerCritLoad = 50; batteryPower->isOutageStep = true;
    while (soc < 95 + tolerance && hour_of_year < 100) {
        dispatchManual->dispatch(year, hour_of_year, step_of_hour);
        hour_of_year += 1;
        soc = dispatchManual->battery_soc();
    }
    EXPECT_NEAR(100, dispatchManual->battery_soc(), 0.1);
    EXPECT_NEAR(19, hour_of_year, 0.1);

    // With outage off, can discharge above max SOC
    batteryPower->powerSystem = 0; batteryPower->voltageSystem = 600; batteryPower->powerLoad = 14;
    batteryPower->powerCritLoad = 14; batteryPower->isOutageStep = false;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    hour_of_year += 1;
    EXPECT_NEAR(batteryPower->powerBatteryDC, 14.6, 0.1);

    // With outage off, cannot charge above max soc
    EXPECT_NEAR(97.42, dispatchManual->battery_soc(), 0.1);
    batteryPower->powerSystem = 700; batteryPower->voltageSystem = 600; batteryPower->powerLoad = 14;
    batteryPower->powerCritLoad = 14; batteryPower->isOutageStep = false;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    hour_of_year += 1;
    EXPECT_NEAR(batteryPower->powerBatteryDC, 0.0, 0.1);
}

TEST_F(ManualTest_lib_battery_dispatch, PVPriorityLoadFirst)
{
    bool chargeOnlySystemExceedLoad = false;
    bool dischargeOnlyLoadExceedSystem = true;
    double SOC_min_outage = 1.0;
    bool priorityChargeBattery = false;
    dispatchManual = new dispatch_manual_t(batteryModel, dtHour, SOC_min, SOC_max, currentChoice, currentChargeMax,
        currentDischargeMax, powerChargeMax, powerDischargeMax, powerChargeMax,
        powerDischargeMax, minimumModeTime,
        dispatchChoice, meterPosition, scheduleWeekday, scheduleWeekend, canCharge,
        canDischarge, canGridcharge, canDischargeToGrid, canGridcharge, percentDischarge,
        percentGridcharge, canClipCharge, canCurtailCharge, interconnection_limit,
        chargeOnlySystemExceedLoad, dischargeOnlyLoadExceedSystem, SOC_min_outage, priorityChargeBattery);

    batteryPower = dispatchManual->getBatteryPower();
    batteryPower->connectionMode = ChargeController::AC_CONNECTED;

    batteryPower->powerSystem = 400; batteryPower->voltageSystem = 600; batteryPower->powerLoad = 400;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    EXPECT_NEAR(batteryPower->powerBatteryAC, 0.0, 1.0);
    EXPECT_NEAR(batteryPower->powerGridToBattery, 0.0, 1.0);
    EXPECT_NEAR(batteryPower->powerSystemToBatteryAC, 0.0, 1.0);
    EXPECT_NEAR(batteryPower->powerSystemToLoad, 400.0, 1.0);
    EXPECT_NEAR(batteryPower->powerGridToLoad, 0.0, 1.0);
    
}

TEST_F(ManualTest_lib_battery_dispatch, PVPriorityBatteryFirst)
{
    bool chargeOnlySystemExceedLoad = false;
    bool dischargeOnlyLoadExceedSystem = true;
    double SOC_min_outage = 1.0;
    bool priorityChargeBattery = true;
    dispatchManual = new dispatch_manual_t(batteryModel, dtHour, SOC_min, SOC_max, currentChoice, currentChargeMax,
        currentDischargeMax, powerChargeMax, powerDischargeMax, powerChargeMax,
        powerDischargeMax, minimumModeTime,
        dispatchChoice, meterPosition, scheduleWeekday, scheduleWeekend, canCharge,
        canDischarge, canGridcharge, canDischargeToGrid, canGridcharge, percentDischarge,
        percentGridcharge, canClipCharge, canCurtailCharge, interconnection_limit,
        chargeOnlySystemExceedLoad, dischargeOnlyLoadExceedSystem, SOC_min_outage, priorityChargeBattery);

    batteryPower = dispatchManual->getBatteryPower();
    batteryPower->connectionMode = ChargeController::AC_CONNECTED;

    batteryPower->powerSystem = 400; batteryPower->voltageSystem = 600; batteryPower->powerLoad = 400;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    EXPECT_NEAR(batteryPower->powerBatteryAC, -50.0, 1.0);
    EXPECT_NEAR(batteryPower->powerGridToBattery, 0.0, 1.0);
    EXPECT_NEAR(batteryPower->powerSystemToBatteryAC, 50.0, 1.0);
    EXPECT_NEAR(batteryPower->powerSystemToLoad, 350.0, 1.0);
    EXPECT_NEAR(batteryPower->powerGridToLoad, 50.0, 1.0);

}

TEST_F(ManualTest_lib_battery_dispatch, ManualInterconnectionLimitTest)
{
    interconnection_limit = 500;
    canCurtailCharge = true;
    // canGridCharge is false by default
    dispatchManual = new dispatch_manual_t(batteryModel, dtHour, SOC_min, SOC_max, currentChoice, currentChargeMax,
        currentDischargeMax, powerChargeMax, powerDischargeMax, powerChargeMax,
        powerDischargeMax, minimumModeTime,
        dispatchChoice, meterPosition, scheduleWeekday, scheduleWeekend, canCharge,
        canDischarge, canGridcharge, canDischargeToGrid, canGridcharge, percentDischarge,
        percentGridcharge, canClipCharge, canCurtailCharge, interconnection_limit);

    batteryPower = dispatchManual->getBatteryPower();
    batteryPower->connectionMode = ChargeController::DC_CONNECTED;
    batteryPower->setSharedInverter(m_sharedInverter);

    // Should charge since PV is being curtailed
    batteryPower->powerSystem = 600; batteryPower->voltageSystem = 600; batteryPower->powerGridToBattery = 0;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    EXPECT_NEAR(batteryPower->powerBatteryDC, -47.9, 0.1);
    EXPECT_NEAR(batteryPower->powerSystemToBatteryDC, 48.4, 0.1);
}

TEST_F(ManualTest_lib_battery_dispatch, TestDischargeToGridInterconnectionLimit)
{
    interconnection_limit = 10;
    std::vector<bool> testCanDischargeToGrid;
    for (int p = 0; p < 6; p++) {
        testCanDischargeToGrid.push_back(1);
    }
    dispatchManual = new dispatch_manual_t(batteryModel, dtHour, SOC_min, SOC_max, currentChoice, currentChargeMax,
        currentDischargeMax, powerChargeMax, powerDischargeMax, powerChargeMax,
        powerDischargeMax, minimumModeTime,
        dispatchChoice, meterPosition, scheduleWeekday, scheduleWeekend, canCharge,
        canDischarge, canGridcharge, canGridcharge, testCanDischargeToGrid, percentDischarge,
        percentGridcharge, canClipCharge, canCurtailCharge, interconnection_limit);

    batteryPower = dispatchManual->getBatteryPower();
    batteryPower->connectionMode = ChargeController::AC_CONNECTED;
    batteryPower->inverterEfficiencyCutoff = 0;

    // Test discharge to grid - should only charge at interconnection limit
    batteryPower->powerSystem = 0; batteryPower->powerLoad = 30; batteryPower->voltageSystem = 600; batteryPower->powerBatteryDC = 1000;
    dispatchManual->dispatch(year, hour_of_year, step_of_hour);
    EXPECT_NEAR(batteryPower->powerBatteryDC, 41.66, 1.0);
    EXPECT_NEAR(batteryPower->powerBatteryToLoad, batteryPower->powerLoad, 2.0);
    EXPECT_NEAR(batteryPower->powerBatteryToGrid, 10, 0.1);
}
