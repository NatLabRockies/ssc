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


#include <gtest/gtest.h>

#include "lib_battery_powerflow_test.h"
#include "lib_ondinv.h"
#include "lib_pvinv.h"
#include "lib_sandia.h"
#include "lib_shared_inverter.h"


void BatteryPowerFlowTest_lib_battery_powerflow::SetUp()
{
	error = 0.02;
	double dtHour = 1.0;
	m_batteryPowerFlow = new BatteryPowerFlow(dtHour);
	m_batteryPower = m_batteryPowerFlow->getBatteryPower();
	m_batteryPower->reset();
	m_batteryPower->canDischarge = false;
	m_batteryPower->canSystemCharge = false;
	m_batteryPower->canGridCharge = false;
	m_batteryPower->singlePointEfficiencyACToDC = 0.96;
	m_batteryPower->singlePointEfficiencyDCToAC = 0.96;
	m_batteryPower->singlePointEfficiencyDCToDC = 0.98;
	m_batteryPower->setMaxDCChargePower(100);
	m_batteryPower->setMaxDCDischargePower(50);
	m_batteryPower->connectionMode = ChargeController::AC_CONNECTED;

    // setup Sandia inverter using SMA America: SB3800TL-US-22 (240V) [CEC 2013]
    int numberOfInverters = 100;
    sandia = new sandia_inverter_t();
    partload = new partload_inverter_t();
    ond = new ond_inverter();
    sandia->C0 = -3.18e-6;
    sandia->C1 = -5.12e-5;
    sandia->C2 = 0.000984;
    sandia->C3 = -0.00151;
    sandia->Paco = 3800;
    sandia->Pdco = 3928.11;
    sandia->Vdco = 398.497;
    sandia->Pso = 19.4516;
    sandia->Pntare = 0.99;
    m_sharedInverter = new SharedInverter(SharedInverter::SANDIA_INVERTER, numberOfInverters, sandia, partload, ond);
    m_batteryPower->setSharedInverter(m_sharedInverter);
}

TEST_F(BatteryPowerFlowTest_lib_battery_powerflow, TestInitialize)
{
	// PV Charging Scenario
	m_batteryPower->canSystemCharge = true;
	m_batteryPower->powerSystem = 100;
	m_batteryPower->powerLoad = 50;
	m_batteryPowerFlow->initialize(50, false);
	EXPECT_EQ(m_batteryPower->powerBatteryDC, -50);

    // Grid charging Scenario
    m_batteryPower->canGridCharge = true;
    m_batteryPowerFlow->initialize(50, false);
    EXPECT_EQ(m_batteryPower->powerBatteryDC, -m_batteryPower->getMaxDCChargePower());

	// Discharging Scenario
	m_batteryPower->canDischarge = true;
	m_batteryPower->powerSystem = 50;
	m_batteryPower->powerLoad = 100;
	m_batteryPowerFlow->initialize(50, false);
	EXPECT_EQ(m_batteryPower->powerBatteryDC, m_batteryPower->getMaxDCDischargePower());
}

// Excess PV production
TEST_F(BatteryPowerFlowTest_lib_battery_powerflow, AC_PVCharging_ExcessPV) {
    m_batteryPower->connectionMode = ChargeController::AC_CONNECTED;

    m_batteryPower->canSystemCharge = true;
    m_batteryPower->canDischarge = true;
    m_batteryPower->canGridCharge = false;
    m_batteryPower->powerSystem = 100;
    m_batteryPower->powerLoad = 50;

    // Try to charge
    m_batteryPower->powerBatteryDC = -50 * m_batteryPower->singlePointEfficiencyACToDC;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 50, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    double gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // Try to charge more than is available from PV, disallowed
    m_batteryPower->powerBatteryDC = -100;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 50, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    // Try to discharge
    m_batteryPower->powerBatteryDC = 50;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 48, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 50, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // Redo the above tests, with losses Try to charge
    m_batteryPower->powerBatteryDC = -48 * m_batteryPower->singlePointEfficiencyACToDC;
    m_batteryPower->powerSystemLoss = 2.0;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -48, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 48, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 1.92, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 2.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC - m_batteryPower->powerSystemLoss;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // Try to charge more than is available from PV, disallowed
    m_batteryPower->powerBatteryDC = -100;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -48, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 48, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 1.92, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 2.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC - m_batteryPower->powerSystemLoss;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 2.0, error);

    // Try to discharge
    m_batteryPower->powerBatteryDC = 50;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 48, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 50, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 2.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC - m_batteryPower->powerSystemLoss;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

}

// Not enough PV for load
TEST_F(BatteryPowerFlowTest_lib_battery_powerflow, AC_PVCharging_ExcessLoad) {
    m_batteryPower->connectionMode = ChargeController::AC_CONNECTED;

    m_batteryPower->canSystemCharge = true;
    m_batteryPower->canDischarge = true;
    m_batteryPower->canGridCharge = false;
    m_batteryPower->powerSystem = 25;
    m_batteryPower->powerLoad = 50;

    // do not discharge battery
    m_batteryPower->powerBatteryDC = 0;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 25, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 25, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    double gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // discharge battery
    m_batteryPower->powerBatteryDC = 20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 19.19, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 25, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 5.8, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 19.19, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0.8, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // try to charge battery from grid, not allowed
    m_batteryPower->powerBatteryDC = -20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 25, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 25, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // Redo the above tests with system losses: do not discharge battery
    m_batteryPower->powerBatteryDC = 0;
    m_batteryPower->powerSystemLoss = 0.5; // Idle loss
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 25, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 25, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.5, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC - m_batteryPower->powerSystemLoss;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // discharge battery
    m_batteryPower->powerBatteryDC = 20;
    m_batteryPower->powerSystemLoss = 1.0; // Charging or discharging loss
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 19.19, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 25, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 6.8, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 18.19, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0.8, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC - m_batteryPower->powerSystemLoss;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // try to charge battery from grid, not allowed
    m_batteryPower->powerBatteryDC = -20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 25, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 25, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC - m_batteryPower->powerSystemLoss;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);
}

// Excess PV production
TEST_F(BatteryPowerFlowTest_lib_battery_powerflow, AC_GridCharging_ExcessPV) {
    m_batteryPower->connectionMode = ChargeController::AC_CONNECTED;

    m_batteryPower->canGridCharge = true;
    m_batteryPower->canDischarge = true;
    m_batteryPower->canSystemCharge = false;
    m_batteryPower->powerSystem = 100;
    m_batteryPower->powerLoad = 50;

    // charging will be from grid
    m_batteryPower->powerBatteryDC = -50;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -52.08, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 50, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 52.08, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 50, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.08, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    double gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // discharge to grid
    m_batteryPower->powerBatteryDC = 20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 19.2, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 50, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 50, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 19.2, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0.80, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // charging will be from grid
    m_batteryPower->powerBatteryDC = -50;
    m_batteryPower->powerSystemLoss = 2.0;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -52.08, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 50, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 52.08, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 50, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.08, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 2.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC - m_batteryPower->powerSystemLoss;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // discharge to grid
    m_batteryPower->powerBatteryDC = 20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 19.2, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 50, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 50, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 17.2, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0.80, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 2.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC - m_batteryPower->powerSystemLoss;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);
}

// Not enough PV, pull from grid
TEST_F(BatteryPowerFlowTest_lib_battery_powerflow, AC_GridCharging_ExcessLoad)
{
    m_batteryPower->connectionMode = ChargeController::AC_CONNECTED;

    m_batteryPower->canGridCharge = true;
    m_batteryPower->canDischarge = true;
    m_batteryPower->canSystemCharge = false;
    m_batteryPower->powerSystem = 10;
    m_batteryPower->powerLoad = 50;

    // don't dispatch
    m_batteryPower->powerBatteryDC = 0;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 10, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 40, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    double gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // charging will happen from grid
    m_batteryPower->powerBatteryDC = -20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -20.83, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 10, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 20.83, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 40, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0.83, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // discharge
    m_batteryPower->powerBatteryDC = 20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 19.19, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 10, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 20.8, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 19.19, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0.8, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // don't dispatch
    m_batteryPower->powerBatteryDC = 0;
    m_batteryPower->powerSystemLoss = 0.5; // Idle loss
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 10.0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 40, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.5, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC - m_batteryPower->powerSystemLoss;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // charging will happen from grid
    m_batteryPower->powerBatteryDC = -20;
    m_batteryPower->powerSystemLoss = 1.0; // Charging or discharging loss
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -20.83, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 10, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 20.83, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 40, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0.83, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC - m_batteryPower->powerSystemLoss;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // discharge
    m_batteryPower->powerBatteryDC = 20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 19.19, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 10, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 21.8, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 18.19, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0.8, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC - m_batteryPower->powerSystemLoss;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);
}

// Excess PV production
TEST_F(BatteryPowerFlowTest_lib_battery_powerflow, AC_GridPVCharging_ExcessPV) {
    m_batteryPower->connectionMode = ChargeController::AC_CONNECTED;

    m_batteryPower->canGridCharge = true;
    m_batteryPower->canDischarge = true;
    m_batteryPower->canSystemCharge = true;
    m_batteryPower->powerSystem = 100;
    m_batteryPower->powerLoad = 50;

    // charging will be from PV
    m_batteryPower->powerBatteryDC = -40;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -41.66, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 41.66, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 8.33, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 8.33, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 1.66, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    double gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // charging will be from both PV and grid
    m_batteryPower->powerBatteryDC = -100;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC,  -104.16, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 54.16, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 4.16, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // discharge to grid
    m_batteryPower->powerBatteryDC = 20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 19.2, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 50, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 50, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 19.2, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0.80, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // charging will be from PV, with losses
    m_batteryPower->powerBatteryDC = -40;
    m_batteryPower->powerSystemLoss = 1.0;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -41.66, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 41.66, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 7.33, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 1.66, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC - m_batteryPower->powerSystemLoss;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // charging will be from both PV and grid
    m_batteryPower->powerBatteryDC = -100;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -104.16, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 49, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 55.16, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 4.16, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC - m_batteryPower->powerSystemLoss;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // discharge to grid
    m_batteryPower->powerBatteryDC = 20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 19.2, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 50, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 50, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 18.2, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0.80, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC - m_batteryPower->powerSystemLoss;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);
}

// Not enough PV, pull from grid
TEST_F(BatteryPowerFlowTest_lib_battery_powerflow, AC_GridPVCharging_ExcessLoad)
{
    m_batteryPower->connectionMode = ChargeController::AC_CONNECTED;

    m_batteryPower->canGridCharge = true;
    m_batteryPower->canDischarge = true;
    m_batteryPower->canSystemCharge = true;
    m_batteryPower->powerSystem = 10;
    m_batteryPower->powerLoad = 50;

    // don't dispatch
    m_batteryPower->powerBatteryDC = 0;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 10, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 40, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    double gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC - m_batteryPower->powerSystemLoss;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // charging will happen from grid since pv first goes to load
    m_batteryPower->powerBatteryDC = -20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -20.83, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 10, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 20.83, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 40, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0.83, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC - m_batteryPower->powerSystemLoss;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // discharge
    m_batteryPower->powerBatteryDC = 20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 19.19, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 10, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 20.8, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 19.19, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0.8, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC - m_batteryPower->powerSystemLoss;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // don't dispatch
    m_batteryPower->powerBatteryDC = 0;
    m_batteryPower->powerSystemLoss = 0.5; // Idle loss
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 10, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 40, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.5, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC - m_batteryPower->powerSystemLoss;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // charging will happen from grid since pv first goes to load
    m_batteryPower->powerBatteryDC = -20;
    m_batteryPower->powerSystemLoss = 1.0;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -20.83, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 10, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 20.83, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 40, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0.83, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC - m_batteryPower->powerSystemLoss;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // discharge
    m_batteryPower->powerBatteryDC = 20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 19.19, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 10, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 21.8, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 18.19, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0.8, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC - m_batteryPower->powerSystemLoss;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);
}

TEST_F(BatteryPowerFlowTest_lib_battery_powerflow, AC_outage_ExcessPV) {
    m_batteryPower->connectionMode = ChargeController::AC_CONNECTED;

    m_batteryPower->canGridCharge = false;
    m_batteryPower->canDischarge = true;
    m_batteryPower->canSystemCharge = true;
    m_batteryPower->isOutageStep = true;
    m_batteryPower->powerSystem = 100;
    m_batteryPower->powerLoad = 50;
    m_batteryPower->powerCritLoad = 50;

    // charging will be from PV
    m_batteryPower->powerBatteryDC = -40;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -41.66, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 41.66, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 1.66, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerCritLoadUnmet, 0, error);
    EXPECT_NEAR(m_batteryPower->powerInterconnectionLoss, 8.33, error);
    EXPECT_NEAR(m_batteryPower->powerGrid, 0.0, error);

    double gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // charging is reduced since the grid is unavailable
    m_batteryPower->powerBatteryDC = -100;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerCritLoadUnmet, 0, error);
    EXPECT_NEAR(m_batteryPower->powerInterconnectionLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerGrid, 0.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // attempt to discharge to grid - battery is curtailed since grid is unavailable
    m_batteryPower->powerBatteryDC = 20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerCritLoadUnmet, 0, error);
    EXPECT_NEAR(m_batteryPower->powerInterconnectionLoss, 50, error);
    EXPECT_NEAR(m_batteryPower->powerGrid, 0.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // charging will be from PV, with losses
    m_batteryPower->powerBatteryDC = -40;
    m_batteryPower->powerSystemLoss = 1.0;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -41.66, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 41.66, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 1.66, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);
    EXPECT_NEAR(m_batteryPower->powerCritLoadUnmet, 0, error);
    EXPECT_NEAR(m_batteryPower->powerInterconnectionLoss, 7.33, error);
    EXPECT_NEAR(m_batteryPower->powerGrid, 0.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC - m_batteryPower->powerSystemLoss;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // charging will be from both PV and grid
    m_batteryPower->powerBatteryDC = -100;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -49, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 49, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 1.96, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);
    EXPECT_NEAR(m_batteryPower->powerCritLoadUnmet, 0, error);
    EXPECT_NEAR(m_batteryPower->powerInterconnectionLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerGrid, 0.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC - m_batteryPower->powerSystemLoss;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // discharge to grid
    m_batteryPower->powerBatteryDC = 20;
    m_batteryPowerFlow->calculate();

    // Battery covers own losses in this case - in real case dispatch should set power DC to zero so losses don't occur or PV covers
    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 1.0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0.038, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);
    EXPECT_NEAR(m_batteryPower->powerCritLoadUnmet, 0, error);
    EXPECT_NEAR(m_batteryPower->powerInterconnectionLoss, 50.0, error);
    EXPECT_NEAR(m_batteryPower->powerGrid, 0.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC - m_batteryPower->powerSystemLoss;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);
}

// Not enough PV for load
TEST_F(BatteryPowerFlowTest_lib_battery_powerflow, AC_outage_ExcessLoad) {
    m_batteryPower->connectionMode = ChargeController::AC_CONNECTED;

    m_batteryPower->canSystemCharge = true;
    m_batteryPower->canDischarge = true;
    m_batteryPower->canGridCharge = false;
    m_batteryPower->isOutageStep = true;
    m_batteryPower->powerSystem = 25;
    m_batteryPower->powerLoad = 50;
    m_batteryPower->powerCritLoad = 50;

    // do not discharge battery
    m_batteryPower->powerBatteryDC = 0;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 25, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerCritLoadUnmet, 25, error);
    EXPECT_NEAR(m_batteryPower->powerInterconnectionLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerGrid, 0.0, error);

    double gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // discharge battery
    m_batteryPower->powerBatteryDC = 20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 19.19, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 25, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 19.19, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0.8, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerCritLoadUnmet, 5.8, error);
    EXPECT_NEAR(m_batteryPower->powerInterconnectionLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerGrid, 0.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // try to charge battery from grid, not allowed
    m_batteryPower->powerBatteryDC = -20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 25, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerCritLoadUnmet, 25, error);
    EXPECT_NEAR(m_batteryPower->powerInterconnectionLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerGrid, 0.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // Redo the above tests with system losses: do not discharge battery
    m_batteryPower->powerBatteryDC = 0;
    m_batteryPower->powerSystemLoss = 0.5; // Idle loss
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 24.5, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.5, error);
    EXPECT_NEAR(m_batteryPower->powerCritLoadUnmet, 25.5, error);
    EXPECT_NEAR(m_batteryPower->powerInterconnectionLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerGrid, 0.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC - m_batteryPower->powerSystemLoss;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // discharge battery
    m_batteryPower->powerBatteryDC = 20;
    m_batteryPower->powerSystemLoss = 1.0; // Charging or discharging loss
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 19.19, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 25, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 18.19, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0.8, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);
    EXPECT_NEAR(m_batteryPower->powerCritLoadUnmet, 6.8, error);
    EXPECT_NEAR(m_batteryPower->powerInterconnectionLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerGrid, 0.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC - m_batteryPower->powerSystemLoss;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // try to charge battery from grid, not allowed
    m_batteryPower->powerBatteryDC = -20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 24, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);
    EXPECT_NEAR(m_batteryPower->powerCritLoadUnmet, 26, error);
    EXPECT_NEAR(m_batteryPower->powerInterconnectionLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerGrid, 0.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC - m_batteryPower->powerSystemLoss;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // discharge battery w/inverter night time losses
    m_batteryPower->powerSystem = 0.0;
    m_batteryPower->powerSystemLoss = 0.0;
    m_batteryPower->powerPVInverterDraw = -1.0;
    m_batteryPower->powerBatteryDC = 53.125;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 51, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.08, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerCritLoadUnmet, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerInterconnectionLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToSystemLoad, 1.0, error);
    EXPECT_NEAR(m_batteryPower->powerGrid, 0.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC + m_batteryPower->powerPVInverterDraw - m_batteryPower->powerSystemLoss;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // Loss accounting when system is totally off
    m_batteryPower->powerSystem = 0.0;
    m_batteryPower->powerSystemLoss = 1.0;
    m_batteryPower->powerPVInverterDraw = 0.0;
    m_batteryPower->powerBatteryDC = 0.0;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);
    EXPECT_NEAR(m_batteryPower->powerCritLoadUnmet, 50.0, error);
    EXPECT_NEAR(m_batteryPower->powerInterconnectionLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToSystemLoad, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerGrid, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerLossesUnmet, 1.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC + m_batteryPower->powerPVInverterDraw - m_batteryPower->powerSystemLoss + m_batteryPower->powerLossesUnmet;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);
}

TEST_F(BatteryPowerFlowTest_lib_battery_powerflow, AC_grid_limits) {
    m_batteryPower->connectionMode = ChargeController::AC_CONNECTED;

    m_batteryPower->canGridCharge = false;
    m_batteryPower->canDischarge = true;
    m_batteryPower->canSystemCharge = true;
    m_batteryPower->isOutageStep = false;
    m_batteryPower->powerSystem = 100;
    m_batteryPower->powerLoad = 25;
    m_batteryPower->powerCritLoad = 0;

    // charging will be from PV, not all PV will be exported
    m_batteryPower->powerInterconnectionLimit = 25;
    m_batteryPower->powerCurtailmentLimit = 1e+38;
    m_batteryPower->powerBatteryDC = -40;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -41.66, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryDC, -40, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 25, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 41.66, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryDC, 40, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 25.0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 1.66, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerCritLoadUnmet, 0, error);
    EXPECT_NEAR(m_batteryPower->powerInterconnectionLoss, 8.33, error);
    EXPECT_NEAR(m_batteryPower->powerGrid, 25.0, error);

    double gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 25, error);

    // system to grid is reduced due to curtailment limit
    m_batteryPower->powerInterconnectionLimit = 1e+38;
    m_batteryPower->powerCurtailmentLimit = 25;
    m_batteryPower->powerBatteryDC = -40;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -41.66, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryDC, -40, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 25, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 41.66, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryDC, 40, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 25.0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 1.66, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerCritLoadUnmet, 0, error);
    EXPECT_NEAR(m_batteryPower->powerInterconnectionLoss, 8.33, error);
    EXPECT_NEAR(m_batteryPower->powerGrid, 25.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 25, error);

    // export is limited due to interconnection, battery discharge is curtailed second
    m_batteryPower->powerInterconnectionLimit = 10;
    m_batteryPower->powerCurtailmentLimit = 25;
    m_batteryPower->powerBatteryDC = 20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 19.2, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 25, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 10, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0.8, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerCritLoadUnmet, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerInterconnectionLoss, 84.2, error);
    EXPECT_NEAR(m_batteryPower->powerGrid, 10.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 25, error);

    // when fuel cell is present, battery is curtailed third
    m_batteryPower->powerInterconnectionLimit = 10;
    m_batteryPower->powerCurtailmentLimit = 25;
    m_batteryPower->powerFuelCell = 20;
    m_batteryPower->powerBatteryDC = 20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 19.2, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 25, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 10, error);
    EXPECT_NEAR(m_batteryPower->powerFuelCellToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0.8, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerCritLoadUnmet, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerInterconnectionLoss, 104.2, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC + m_batteryPower->powerFuelCell;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 25, error);

    // higher limit allows fuel cell and battery through, but not PV
    m_batteryPower->powerInterconnectionLimit = 40;
    m_batteryPower->powerCurtailmentLimit = 40;
    m_batteryPower->powerFuelCell = 20;
    m_batteryPower->powerBatteryDC = 20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 19.2, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 25, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0.8, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 19.2, error);
    EXPECT_NEAR(m_batteryPower->powerFuelCellToGrid, 20, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0.8, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerCritLoadUnmet, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerInterconnectionLoss, 74.2, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC + m_batteryPower->powerFuelCell;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 25, error);

}

// Excess PV production
TEST_F(BatteryPowerFlowTest_lib_battery_powerflow, DC_PVCharging_ExcessPV) {
    m_batteryPower->connectionMode = ChargeController::DC_CONNECTED;

    m_batteryPower->canSystemCharge = true;
    m_batteryPower->canDischarge = true;
    m_batteryPower->canGridCharge = false;
    m_batteryPower->powerSystem = 100;
    m_batteryPower->powerLoad = 50;

    // Try to charge
    m_batteryPower->powerBatteryDC = -50;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -49.36, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 46.24, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 49.36, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 3.75, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 3.75, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    check_net_flows(std::string());

    // Charge battery from PV and meet load from grid
    m_batteryPower->powerBatteryDC = -100;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -96.76, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 96.76, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.099, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    check_net_flows(std::string());


    // Try to charge more than available from PV, disallowed
    m_batteryPower->powerBatteryDC = -150;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -96.76, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 96.76, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.099, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    check_net_flows(std::string());


    // Try to discharge
    m_batteryPower->powerBatteryDC = 50;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 47.39, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 46.71, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 5.89, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    check_net_flows(std::string());

    // Redo tests from above with losses: Try to charge
    m_batteryPower->powerBatteryDC = -50;
    m_batteryPower->powerSystemLoss = 1.0;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -49.36, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 45.24, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 49.36, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 4.75, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 3.75, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);

    check_net_flows(std::string());

    // Charge battery from PV and meet load from grid
    m_batteryPower->powerBatteryDC = -100;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -95.79, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 95.79, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.079, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);

    check_net_flows(std::string());


    // Try to charge more than available from PV, disallowed
    m_batteryPower->powerBatteryDC = -150;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -95.79, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 95.79, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.079, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);

    check_net_flows(std::string());


    // Try to discharge
    m_batteryPower->powerBatteryDC = 50;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 46.42, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 46.71, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 5.87, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);

    check_net_flows(std::string());

}

// Not enough PV for load
TEST_F(BatteryPowerFlowTest_lib_battery_powerflow, DC_PVCharging_ExcessLoad) {
    m_batteryPower->connectionMode = ChargeController::DC_CONNECTED;

    m_batteryPower->canSystemCharge = true;
    m_batteryPower->canDischarge = true;
    m_batteryPower->canGridCharge = false;
    m_batteryPower->powerSystem = 25;
    m_batteryPower->powerLoad = 50;

    // do not discharge battery
    m_batteryPower->powerBatteryDC = 0;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 22.68, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 27.31, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.32, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    check_net_flows(std::string());


    // discharge battery
    m_batteryPower->powerBatteryDC = 20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 18.43, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 23.50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 8.05, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad,  18.43, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 3.05, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    check_net_flows(std::string());


    // charging happens from pv
    m_batteryPower->powerBatteryDC = -20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -19.74, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 2.60, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 19.74, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 47.39, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.39, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    check_net_flows(std::string());

    // do not discharge battery
    m_batteryPower->powerBatteryDC = 0;
    m_batteryPower->powerSystemLoss = 0.5;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 22.18, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 27.81, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.32, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.5, error);

    check_net_flows(std::string());


    // discharge battery
    m_batteryPower->powerBatteryDC = 20;
    m_batteryPower->powerSystemLoss = 1.0;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 17.47, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 23.50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 9.05, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 17.47, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 3.05, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);

    check_net_flows(std::string());


    // charging happens from pv
    m_batteryPower->powerBatteryDC = -20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -19.74, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 1.62, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 19.74, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 48.39, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.39, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);

    check_net_flows(std::string());
}

// Excess PV production
TEST_F(BatteryPowerFlowTest_lib_battery_powerflow, DC_GridCharging_ExcessPV) {
    m_batteryPower->connectionMode = ChargeController::DC_CONNECTED;

    m_batteryPower->canGridCharge = true;
    m_batteryPower->canDischarge = true;
    m_batteryPower->canSystemCharge = false;
    m_batteryPower->powerSystem = 100;
    m_batteryPower->powerLoad = 50;

    // charging will be from grid
    m_batteryPower->powerBatteryDC = -50;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -54.04, error) << "1st case";
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 50, error) << "1st case";
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error) << "1st case";
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 44.40, error) << "1st case";
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 54.04, error) << "1st case";
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error) << "1st case";
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error) << "1st case";
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 3.75, error) << "1st case";
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error) << "1st case";

    check_net_flows("1st case");


    // discharge to grid
    m_batteryPower->powerBatteryDC = 20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 18.91, error) << "2nd case";
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 50, error) << "2nd case";
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error) << "2nd case";
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 46.49, error) << "2nd case";
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error) << "2nd case";
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error) << "2nd case";
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error) << "2nd case";
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 18.91, error) << "2nd case";
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 4.59, error) << "2nd case";
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error) << "2nd case";

    check_net_flows("2nd case");

    // Redo the above with losses: charging will be from grid
    m_batteryPower->powerSystemLoss = 1.0;
    m_batteryPower->powerBatteryDC = -50;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -54.08, error) << "1st case";
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 50, error) << "1st case";
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error) << "1st case";
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 43.40, error) << "1st case";
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 54.08, error) << "1st case";
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error) << "1st case";
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error) << "1st case";
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 3.75, error) << "1st case";
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error) << "1st case";

    check_net_flows("1st case");


    // discharge to grid
    m_batteryPower->powerBatteryDC = 20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 17.95, error) << "2nd case";
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 50, error) << "2nd case";
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error) << "2nd case";
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 46.49, error) << "2nd case";
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error) << "2nd case";
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error) << "2nd case";
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error) << "2nd case";
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 17.95, error) << "2nd case";
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 4.59, error) << "2nd case";
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error) << "2nd case";

    check_net_flows("2nd case");
}

// Not enough PV, pull from grid
TEST_F(BatteryPowerFlowTest_lib_battery_powerflow, DC_GridCharging_ExcessLoad)
{
    m_batteryPower->connectionMode = ChargeController::DC_CONNECTED;

    m_batteryPower->canGridCharge = true;
    m_batteryPower->canDischarge = true;
    m_batteryPower->canSystemCharge = false;
    m_batteryPower->isOutageStep = false;
    m_batteryPower->powerSystem = 10;
    m_batteryPower->powerLoad = 50;

    // don't dispatch
    m_batteryPower->powerBatteryDC = 0;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 7.93, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 42.07, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.07, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    check_net_flows(std::string());


    // charging will happen from grid
    m_batteryPower->powerBatteryDC = -20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -25.49, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 8.00, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 25.49, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 41.99, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 3.00, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    check_net_flows(std::string());


    // discharge
    m_batteryPower->powerBatteryDC = 20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 18.01, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 9.19, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 22.79, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 18.01, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.79, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    check_net_flows(std::string());

    // Redo the above, with losses: don't dispatch
    m_batteryPower->powerBatteryDC = 0;
    m_batteryPower->powerSystemLoss = 0.5;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 7.44, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 42.56, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.07, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.5, error);

    check_net_flows(std::string());


    // charging will happen from grid (though dispatch constraints would prevent grid charging with PV output)
    m_batteryPower->powerBatteryDC = -20;
    m_batteryPower->powerSystemLoss = 1.0;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -24.99, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 7.35, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 24.99, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 42.65, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.97, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);

    check_net_flows(std::string());


    // discharge
    m_batteryPower->powerBatteryDC = 20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 17.05, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 9.17, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 23.79, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 17.05, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.79, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);

    check_net_flows(std::string());

}

// Excess PV production
TEST_F(BatteryPowerFlowTest_lib_battery_powerflow, DC_GridPVCharging_ExcessPV) {
    m_batteryPower->connectionMode = ChargeController::DC_CONNECTED;

    m_batteryPower->canGridCharge = true;
    m_batteryPower->canDischarge = true;
    m_batteryPower->canSystemCharge = true;
    m_batteryPower->powerSystem = 100;
    m_batteryPower->powerLoad = 50;

    // charging will be from PV
    m_batteryPower->powerBatteryDC = -40;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -39.49, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 39.49, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 6.25, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 3.74, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    check_net_flows("1st case");

    // charging will be from PV then grid
    m_batteryPower->powerBatteryDC = -100;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -137.58, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 96.76, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 40.81, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 40.81, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    check_net_flows("2nd case");


    // discharge to grid
    m_batteryPower->powerBatteryDC = 20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 18.91, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 46.49, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 18.91, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 4.59, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    check_net_flows("3rd case");

    // Redo the above, with losses: charging will be from PV
    m_batteryPower->powerSystemLoss = 1.0;
    m_batteryPower->powerBatteryDC = -40;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -39.49, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 39.49, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 5.27, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 3.74, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);

    check_net_flows("4th case");

    // charging will be from PV then grid
    m_batteryPower->powerBatteryDC = -100;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -104.36, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 95.79, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 8.57, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 7.57, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);

    check_net_flows("5th case");


    // discharge to grid
    m_batteryPower->powerBatteryDC = 20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 17.95, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 46.49, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 17.95, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 4.59, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);

    check_net_flows("6th case");

}

// Not enough PV, pull from grid
TEST_F(BatteryPowerFlowTest_lib_battery_powerflow, DC_GridPVCharging_ExcessLoad)
{
    m_batteryPower->connectionMode = ChargeController::DC_CONNECTED;

    m_batteryPower->canGridCharge = true;
    m_batteryPower->canDischarge = true;
    m_batteryPower->canSystemCharge = true;
    m_batteryPower->powerSystem = 10;
    m_batteryPower->powerLoad = 50;

    // don't dispatch
    m_batteryPower->powerBatteryDC = 0;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 7.92, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 42.07, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.07, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    check_net_flows(std::string());


    // charging will happen from pv first
    m_batteryPower->powerBatteryDC = -20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -22.68, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryDC, -20.0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 9.68, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryDC, 10, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 13.00, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 3.00, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    check_net_flows(std::string());

    // discharge
    m_batteryPower->powerBatteryDC = 20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 18.01, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 9.19, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 22.79, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 18.01, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.79, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    check_net_flows(std::string());

    // Redo the above, with losses: don't dispatch
    m_batteryPower->powerSystemLoss = 0.5;
    m_batteryPower->powerBatteryDC = 0;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 7.42, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 42.57, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.07, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.5, error);

    check_net_flows(std::string());

    // charging will happen from pv first
    m_batteryPower->powerSystemLoss = 1.0;
    m_batteryPower->powerBatteryDC = -20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -22.68, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryDC, -20, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 8.71, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryDC, 9, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 13.97, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.97, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);

    check_net_flows(std::string());

    // discharge
    m_batteryPower->powerBatteryDC = 20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 17.05, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 9.17, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 23.79, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 17.05, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.79, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);

    check_net_flows(std::string());
}


TEST_F(BatteryPowerFlowTest_lib_battery_powerflow, TestDCConnected)
{
    m_batteryPower->connectionMode = ChargeController::DC_CONNECTED;

	// PV and Grid Charging Scenario
	m_batteryPower->canSystemCharge = true;
	m_batteryPower->powerSystem = 300;
	m_batteryPower->powerLoad = 200;
	m_batteryPowerFlow->initialize(50, false);
	m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -98.73, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 191.78, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 98.73, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0.00, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 8.22, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

	// Exclusive Grid Charging Scenario
	m_batteryPower->canGridCharge = true;
	m_batteryPower->canSystemCharge = false;
	m_batteryPower->powerSystem = 300;
	m_batteryPower->powerLoad = 200;
	m_batteryPowerFlow->initialize(50, false);
	m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -105.33, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 200, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 90.63, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 105.33, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 8.22, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

	// Discharging Scenario
	m_batteryPower->canDischarge = true;
	m_batteryPower->powerSystem = 200;
	m_batteryPower->powerLoad = 300;
	m_batteryPowerFlow->initialize(50, false);
	m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 47.49, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 47.49, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 193.83, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 58.68, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 8.68, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    // Redo the above, with losses: PV and Grid Charging Scenario
    m_batteryPower->canSystemCharge = true;
    m_batteryPower->powerSystem = 300;
    m_batteryPower->powerLoad = 200;
    m_batteryPower->powerSystemLoss = 1.0;
    m_batteryPowerFlow->initialize(50, false);
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -98.73, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 190.80, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 98.73, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0.00, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 8.19, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);

    // Exclusive Grid Charging Scenario
    m_batteryPower->canGridCharge = true;
    m_batteryPower->canSystemCharge = false;
    m_batteryPower->powerSystem = 300;
    m_batteryPower->powerLoad = 200;
    m_batteryPowerFlow->initialize(50, false);
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -105.33, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 200, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 89.66, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 105.33, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 8.19, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);

    // Discharging Scenario
    m_batteryPower->canDischarge = true;
    m_batteryPower->powerSystem = 200;
    m_batteryPower->powerLoad = 300;
    m_batteryPowerFlow->initialize(50, false);
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 46.52, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 46.52, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 193.83, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 59.65, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 8.65, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);
}

// DC connected outage
TEST_F(BatteryPowerFlowTest_lib_battery_powerflow, DC_outage_excessPV) {
    m_batteryPower->connectionMode = ChargeController::DC_CONNECTED;

    m_batteryPower->canGridCharge = false;
    m_batteryPower->canDischarge = true;
    m_batteryPower->canSystemCharge = true;
    m_batteryPower->isOutageStep = true;
    m_batteryPower->powerSystem = 100;
    m_batteryPower->powerLoad = 50;
    m_batteryPower->powerCritLoad = 50;

    // charging will be from PV, no export because of outage
    m_batteryPower->powerBatteryDC = -40;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -39.49, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 39.49, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 3.74, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerCritLoadUnmet, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerInterconnectionLoss, 6.25, error);

    check_net_flows("1st case");

    // battery steals all of the energy and leaves critical load unmet - dispatch controller needs to avoid this situation
    m_batteryPower->powerBatteryDC = -100;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -96.76, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 96.76, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.099, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerCritLoadUnmet, 50.0, error);
    EXPECT_NEAR(m_batteryPower->powerInterconnectionLoss, 0.0, error);

    check_net_flows("2nd case");

    // Redo the above, with losses: charging will be from PV
    m_batteryPower->powerSystemLoss = 1.0;
    m_batteryPower->powerBatteryDC = -40;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -39.49, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 39.49, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 3.74, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);
    EXPECT_NEAR(m_batteryPower->powerCritLoadUnmet, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerInterconnectionLoss, 5.27, error);

    check_net_flows("3th case");

    // battery steals all of the energy and leaves critical load unmet - dispatch controller needs to avoid this situation
    m_batteryPower->powerBatteryDC = -100;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -95.79, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 95.79, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.099, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);
    EXPECT_NEAR(m_batteryPower->powerCritLoadUnmet, 50.0, error);
    EXPECT_NEAR(m_batteryPower->powerInterconnectionLoss, 0.0, error);

    check_net_flows("4th case");
}

TEST_F(BatteryPowerFlowTest_lib_battery_powerflow, DC_PVCharging_ExcessLoad_outage) {
    m_batteryPower->connectionMode = ChargeController::DC_CONNECTED;

    m_batteryPower->canSystemCharge = true;
    m_batteryPower->canDischarge = true;
    m_batteryPower->canGridCharge = false;
    m_batteryPower->isOutageStep = true;
    m_batteryPower->powerSystem = 25;
    m_batteryPower->powerLoad = 50;
    m_batteryPower->powerCritLoad = 50;

    // do not discharge battery
    m_batteryPower->powerBatteryDC = 0;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 22.68, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.32, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerCritLoadUnmet, 27.31, error);
    EXPECT_NEAR(m_batteryPower->powerInterconnectionLoss, 0.0, error);

    check_net_flows("1st case");


    // discharge battery
    m_batteryPower->powerBatteryDC = 20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 18.43, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 23.50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 18.43, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 3.05, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerCritLoadUnmet, 8.05, error);
    EXPECT_NEAR(m_batteryPower->powerInterconnectionLoss, 0.0, error);

    check_net_flows("2nd case");


    // charging happens from pv
    m_batteryPower->powerBatteryDC = -20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -19.74, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 2.60, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 19.74, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.39, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerCritLoadUnmet, 47.39, error);
    EXPECT_NEAR(m_batteryPower->powerInterconnectionLoss, 0.0, error);

    check_net_flows("3rd case");

    // do not discharge battery
    m_batteryPower->powerBatteryDC = 0;
    m_batteryPower->powerSystemLoss = 0.5;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 22.18, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.32, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.5, error);
    EXPECT_NEAR(m_batteryPower->powerCritLoadUnmet, 27.81, error);
    EXPECT_NEAR(m_batteryPower->powerInterconnectionLoss, 0.0, error);

    check_net_flows("4th case");


    // discharge battery
    m_batteryPower->powerBatteryDC = 20;
    m_batteryPower->powerSystemLoss = 1.0;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 17.47, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 23.50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 17.47, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 3.05, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);
    EXPECT_NEAR(m_batteryPower->powerCritLoadUnmet, 9.05, error);
    EXPECT_NEAR(m_batteryPower->powerInterconnectionLoss, 0.0, error);

    check_net_flows("5th case");


    // charging happens from pv
    m_batteryPower->powerBatteryDC = -20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -19.74, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 1.62, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 19.74, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.39, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);
    EXPECT_NEAR(m_batteryPower->powerCritLoadUnmet, 48.39, error);
    EXPECT_NEAR(m_batteryPower->powerInterconnectionLoss, 0.0, error);

    check_net_flows("6th case");

    // excess discharging is curtailed due to interconnection loss during outage
    m_batteryPower->powerBatteryDC = 50;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 26.33, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 23.67, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 26.32, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 3.39, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);
    EXPECT_NEAR(m_batteryPower->powerCritLoadUnmet, 0, error);
    EXPECT_NEAR(m_batteryPower->powerInterconnectionLoss, 0.0, error);

    check_net_flows("7th case");

    // system off during an outage
    m_batteryPower->powerSystem = 0.0;
    m_batteryPower->powerBatteryDC = 0.0;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 1.13, error); // Inverter night time losses - will be cleaned up by pv ac code as appropriate
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);
    EXPECT_NEAR(m_batteryPower->powerLossesUnmet, 1.13, error);
    EXPECT_NEAR(m_batteryPower->powerCritLoadUnmet, 50.0, error);
    EXPECT_NEAR(m_batteryPower->powerInterconnectionLoss, 0.0, error);

    check_net_flows("8th case");
}

// DC connected outage
TEST_F(BatteryPowerFlowTest_lib_battery_powerflow, DC_interconnection_limit_excessPV) {
    m_batteryPower->connectionMode = ChargeController::DC_CONNECTED;

    m_batteryPower->canGridCharge = false;
    m_batteryPower->canDischarge = true;
    m_batteryPower->canSystemCharge = true;
    m_batteryPower->isOutageStep = false;
    m_batteryPower->powerSystem = 100;
    m_batteryPower->powerLoad = 25;
    m_batteryPower->powerCritLoad = 0;

    // export is limited due to interconnection
    m_batteryPower->powerInterconnectionLimit = 25;
    m_batteryPower->powerCurtailmentLimit = 1e+38;
    m_batteryPower->powerBatteryDC = -40;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -39.49, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 25, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 39.49, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 25, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 3.74, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerCritLoadUnmet, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerInterconnectionLoss, 6.25, error);

    check_net_flows("1st case");

    // export is limited due to curtailment
    m_batteryPower->powerInterconnectionLimit = 1e+38;
    m_batteryPower->powerCurtailmentLimit = 25;
    m_batteryPower->powerBatteryDC = -40;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -39.49, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 25, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 39.49, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 25, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 3.74, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerCritLoadUnmet, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerInterconnectionLoss, 6.25, error);

    check_net_flows("2nd case");

    // export is limited due to interconnection, battery discharge is curtailed second
    m_batteryPower->powerInterconnectionLimit = 10;
    m_batteryPower->powerCurtailmentLimit = 25;
    m_batteryPower->powerBatteryDC = 20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 18.91, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 25, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 10, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 4.59, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerCritLoadUnmet, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerInterconnectionLoss, 80.41, error);

    check_net_flows("3rd case");
}

// Excess PV production
TEST_F(BatteryPowerFlowTest_lib_battery_powerflow, AC_PVCharging_ExcessPV_Flexible_Powerflow) {
    m_batteryPower->connectionMode = ChargeController::AC_CONNECTED;

    m_batteryPower->canSystemCharge = true;
    m_batteryPower->canDischarge = true;
    m_batteryPower->canGridCharge = false;
    m_batteryPower->chargeOnlySystemExceedLoad = false;
    m_batteryPower->dischargeOnlyLoadExceedSystem = false;
    m_batteryPower->powerSystem = 100;
    m_batteryPower->powerLoad = 50;

    // Try to charge
    m_batteryPower->powerBatteryDC = -50 * m_batteryPower->singlePointEfficiencyACToDC;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 50, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    double gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // Try to charge more than is available from PV less load, allowed due to powerflow
    m_batteryPower->powerBatteryDC = -100;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -100, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 100, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 4.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    // Try to charge more than is available from PV total, disallowed due to no grid charging
    m_batteryPower->powerBatteryDC = -150;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -100, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 100, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 4.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    // Try to discharge
    m_batteryPower->powerBatteryDC = 50;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 48, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 2, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 98, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 48, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // Redo the above tests, with losses Try to charge
    m_batteryPower->powerBatteryDC = -100;
    m_batteryPower->powerSystemLoss = 2.0;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -98, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 98, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 3.92, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 2.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC - m_batteryPower->powerSystemLoss;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 2.0, error);

    // Try to discharge
    m_batteryPower->reset();
    m_batteryPower->powerSystem = 100;
    m_batteryPower->powerLoad = 50;
    m_batteryPower->powerBatteryDC = 50;
    m_batteryPower->powerSystemLoss = 2.0;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 48, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 2, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 98, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 48, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 2.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC - m_batteryPower->powerSystemLoss;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

}

// Not enough PV for load
TEST_F(BatteryPowerFlowTest_lib_battery_powerflow, AC_PVCharging_ExcessLoad_Flexible_Charging) {
    m_batteryPower->connectionMode = ChargeController::AC_CONNECTED;

    m_batteryPower->canSystemCharge = true;
    m_batteryPower->canDischarge = true;
    m_batteryPower->canGridCharge = false;
    m_batteryPower->chargeOnlySystemExceedLoad = false;
    m_batteryPower->dischargeOnlyLoadExceedSystem = false;
    m_batteryPower->powerSystem = 25;
    m_batteryPower->powerLoad = 50;
    m_batteryPower->powerFuelCell = 0;

    // do not discharge battery
    m_batteryPower->powerBatteryDC = 0;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 25, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 25, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    double gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // discharge battery
    m_batteryPower->powerBatteryDC = 20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 19.19, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 25, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 5.8, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 19.19, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0.8, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // Charge battery from PV, grid meets load
    m_batteryPower->powerBatteryDC = -20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -20.83, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 4.166, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 20.83, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 45.83, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0.833, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // charge battery from PV and meet load from grid
    m_batteryPower->powerBatteryDC = -20;
    m_batteryPower->powerSystemLoss = 1.0; // Charging or discharging loss
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -20.833, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 3.16, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 20.83, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 46.83, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0.83, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC - m_batteryPower->powerSystemLoss;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // Try to charge so much that the grid would have to be involved, not allowed
    m_batteryPower->powerBatteryDC = -50;
    m_batteryPower->powerSystemLoss = 1.0; // Charging or discharging loss
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -24, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 24, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0.96, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);

    gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC - m_batteryPower->powerSystemLoss;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);
}

// Excess PV production
TEST_F(BatteryPowerFlowTest_lib_battery_powerflow, DC_PVCharging_ExcessPV_Flexible_Charging) {
    m_batteryPower->connectionMode = ChargeController::DC_CONNECTED;

    m_batteryPower->canSystemCharge = true;
    m_batteryPower->canDischarge = true;
    m_batteryPower->canGridCharge = false;
    m_batteryPower->dischargeOnlyLoadExceedSystem = false;
    m_batteryPower->powerSystem = 100;
    m_batteryPower->powerLoad = 50;

    // Try to charge
    m_batteryPower->powerBatteryDC = -50;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -49.36, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 46.24, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 49.36, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 3.75, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 3.75, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    check_net_flows(std::string());

    // Charge battery from PV and meet load from grid
    m_batteryPower->powerBatteryDC = -100;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -96.76, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 96.76, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.099, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    check_net_flows(std::string());


    // Try to charge more than available from PV, disallowed
    m_batteryPower->powerBatteryDC = -150;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -96.76, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 96.76, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.099, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    check_net_flows(std::string());


    // Try to discharge
    m_batteryPower->powerBatteryDC = 50;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 47.39, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 2.61, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 94.10, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 47.39, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 5.89, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    check_net_flows(std::string());

    // Try to discharge with losses
    m_batteryPower->powerBatteryDC = 50;
    m_batteryPower->powerSystemLoss = 1.0;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 46.42, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 3.57, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 93.13, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 46.42, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 5.87, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);

    check_net_flows(std::string());

}

// Not enough PV for load
TEST_F(BatteryPowerFlowTest_lib_battery_powerflow, DC_PVCharging_ExcessLoad_Flexible_Charging) {
    m_batteryPower->connectionMode = ChargeController::DC_CONNECTED;

    m_batteryPower->canSystemCharge = true;
    m_batteryPower->canDischarge = true;
    m_batteryPower->canGridCharge = false;
    m_batteryPower->dischargeOnlyLoadExceedSystem = false;
    m_batteryPower->powerSystem = 25;
    m_batteryPower->powerLoad = 50;

    // do not discharge battery
    m_batteryPower->powerBatteryDC = 0;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 22.68, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 27.31, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.32, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    check_net_flows(std::string());


    // discharge battery
    m_batteryPower->powerBatteryDC = 20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 18.43, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 23.50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 8.05, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 18.43, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 3.05, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    check_net_flows(std::string());


    // charging happens from pv
    m_batteryPower->powerBatteryDC = -20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -19.74, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 2.60, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 19.74, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 47.39, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.39, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    check_net_flows(std::string());

    // do not discharge battery
    m_batteryPower->powerBatteryDC = 0;
    m_batteryPower->powerSystemLoss = 0.5;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 22.18, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 27.81, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.32, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.5, error);

    check_net_flows(std::string());


    // discharge battery
    m_batteryPower->powerBatteryDC = 20;
    m_batteryPower->powerSystemLoss = 1.0;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 17.47, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 23.50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 9.05, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 17.47, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 3.05, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);

    check_net_flows(std::string());


    // charging happens from pv
    m_batteryPower->powerBatteryDC = -20;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -19.74, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 1.62, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 19.74, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 48.39, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 2.39, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1.0, error);

    check_net_flows(std::string());
}

TEST_F(BatteryPowerFlowTest_lib_battery_powerflow, DC_system_w_ac_losses) {
    m_batteryPower->connectionMode = ChargeController::DC_CONNECTED;

    m_batteryPower->canSystemCharge = true;
    m_batteryPower->canDischarge = true;
    m_batteryPower->canGridCharge = false;
    m_batteryPower->powerSystem = 100;
    m_batteryPower->powerLoad = 50;

    // Try to charge - allowed to charge from PV since loss is on AC side
    m_batteryPower->acLossSystemAvailability = 1;
    m_batteryPower->powerBatteryDC = -50;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -49.36, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 49.36, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 3.758, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);
    // Not testing gen here - things get a little weird with the various percentages

    // Try to charge when inverter loss is 100%, allowed for DC connected
    m_batteryPower->powerSystem = 100;
    m_batteryPower->powerLoad = 50;
    m_batteryPower->acLossSystemAvailability = 0;
    m_batteryPower->acLossWiring = 1;

    m_batteryPower->powerBatteryDC = -50;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, -49.36, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 49.36, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 3.758, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    // Allow grid charging, no power from system
    m_batteryPower->powerSystem = 0;
    m_batteryPower->powerSystem = 0;
    m_batteryPower->powerLoad = 50;
    m_batteryPower->acLossSystemAvailability = 0;
    m_batteryPower->acLossWiring = 1;
    m_batteryPower->canGridCharge = true;

    // Try to grid charge - not allowed with inverter loss
    m_batteryPower->powerBatteryDC = -50;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0.099, error); // Inverter night time loss
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    // Cannot grid charge with post batt loss
    m_batteryPower->powerSystem = 0;
    m_batteryPower->powerLoad = 50;
    m_batteryPower->acLossSystemAvailability = 1;
    m_batteryPower->acLossWiring = 0;
    m_batteryPower->canGridCharge = true;

    // Try to charge - but 100% system loss
    m_batteryPower->powerBatteryDC = -50;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 0.099, error); // Inverter night time loss
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);

    // Try to discharge to load w/ inverter loss - not allowed
    m_batteryPower->acLossSystemAvailability = 0;
    m_batteryPower->acLossWiring = 1;
    m_batteryPower->powerBatteryDC = 50;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 46.26, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 3.738, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // Cannot discharge to load w/ post batt loss
    m_batteryPower->acLossSystemAvailability = 1;
    m_batteryPower->acLossWiring = 0;
    m_batteryPower->powerBatteryDC = 50;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 46.26, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 50, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 3.738, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // Try to discharge to grid w/ inverter loss - not allowed
    m_batteryPower->acLossSystemAvailability = 0;
    m_batteryPower->acLossWiring = 1;
    m_batteryPower->powerLoad = 0;
    m_batteryPower->powerBatteryDC = 50;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 46.26, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 3.738, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 0, error);

    // Cannot discharge w/ post batt loss
    m_batteryPower->acLossSystemAvailability = 1;
    m_batteryPower->acLossWiring = 0;
    m_batteryPower->powerBatteryDC = 50;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 46.26, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 3.738, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 0, error);

    // Transformer losses applied to battery
    m_batteryPower->acLossSystemAvailability = 0;
    m_batteryPower->acXfmrLoadLoss = 0.5;
    m_batteryPower->acXfmrRating = 50;
    m_batteryPower->acLossWiring = 0;
    m_batteryPower->powerBatteryDC = 50;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 46.26, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 24.86, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 3.738, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 0, error);

    // Transformer no load loss is in kw
    m_batteryPower->acLossSystemAvailability = 0;
    m_batteryPower->acXfmrLoadLoss = 0.01;
    m_batteryPower->acXfmrNoLoadLoss = 10.0;
    m_batteryPower->acXfmrRating = 50;
    m_batteryPower->acLossWiring = 0;
    m_batteryPower->powerBatteryDC = 50;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 46.26, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToGrid, 35.83, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 3.738, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 0, error);

    m_batteryPower->acXfmrLoadLoss = 0.0;
    m_batteryPower->acXfmrNoLoadLoss = 0.0;

    // Post batt loss affects meeting critical load
    m_batteryPower->acLossSystemAvailability = 0.05;
    m_batteryPower->acLossWiring = 0;
    m_batteryPower->powerBatteryDC = 50;
    m_batteryPower->isOutageStep = true;
    m_batteryPower->powerCritLoad = 50;
    m_batteryPower->powerLoad = 50;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 46.26, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 43.947, error);
    EXPECT_NEAR(m_batteryPower->powerCritLoadUnmet, 6.053, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 3.738, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);

    // Increasing batt power allows meeting critical load
    m_batteryPower->acLossSystemAvailability = 0.05;
    m_batteryPower->acLossWiring = 0;
    m_batteryPower->powerBatteryDC = 60;
    m_batteryPower->isOutageStep = true;
    m_batteryPower->powerLoad = 50;
    m_batteryPower->powerCritLoad = 50;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 52.63, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 50.0, error);
    EXPECT_NEAR(m_batteryPower->powerCritLoadUnmet, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 3.99, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 0.0, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 50, error);
}

// Test to reproduce https://github.com/NREL/sam/issues/1914
TEST_F(BatteryPowerFlowTest_lib_battery_powerflow, AC_ExcessSystemLoss) {
    m_batteryPower->connectionMode = ChargeController::AC_CONNECTED;

    m_batteryPower->canSystemCharge = true;
    m_batteryPower->canDischarge = true;
    m_batteryPower->canGridCharge = false;
    m_batteryPower->chargeOnlySystemExceedLoad = true;
    m_batteryPower->dischargeOnlyLoadExceedSystem = true;
    m_batteryPower->powerSystem = 0;
    m_batteryPower->powerLoad = 5000;
    m_batteryPower->powerFuelCell = 0;
    m_batteryPower->powerSystemLoss = 1000;

    // do not discharge battery
    m_batteryPower->powerBatteryDC = 30;
    m_batteryPowerFlow->calculate();

    EXPECT_NEAR(m_batteryPower->powerBatteryAC, 28.8, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToBatteryAC, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToBattery, 0, error);
    EXPECT_NEAR(m_batteryPower->powerGridToLoad, 5000, error);
    EXPECT_NEAR(m_batteryPower->powerSystemToGrid, 0, error);
    EXPECT_NEAR(m_batteryPower->powerBatteryToLoad, 0, error);
    EXPECT_NEAR(m_batteryPower->powerConversionLoss, 1.2, error);
    EXPECT_NEAR(m_batteryPower->powerSystemLoss, 1000, error);

    double gen = m_batteryPower->powerSystem + m_batteryPower->powerBatteryAC - m_batteryPower->powerSystemLoss;
    EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, error);
    EXPECT_NEAR(m_batteryPower->powerLoad, 5000, error);

}
