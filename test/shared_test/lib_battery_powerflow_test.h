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


#ifndef __LIB_BATTERY_POWERFLOW_TEST_H__
#define __LIB_BATTERY_POWERFLOW_TEST_H__

#include <gtest/gtest.h>
#include "../shared/lib_battery_powerflow.h"

#include <lib_ondinv.h>
#include <lib_power_electronics.h>
#include <lib_pvinv.h>
#include <lib_sandia.h>
#include <lib_shared_inverter.h>

// forward declarations
class sandia_inverter_t;
class partload_inverter_t;
class ond_inverter;

class BatteryPowerFlowTest_lib_battery_powerflow : public ::testing::Test
{
protected:
    BatteryPowerFlow* m_batteryPowerFlow;
    BatteryPower* m_batteryPower;
    SharedInverter* m_sharedInverter;
    sandia_inverter_t* sandia;
    partload_inverter_t* partload;
    ond_inverter* ond;
    double error;

public:

    void SetUp();

    double calc_dc_gen() {
        //		return m_batteryPower->powerPVToLoad + m_batteryPower->powerPVToGrid + m_batteryPower->powerBatteryToLoad
        //			   + m_batteryPower->powerBatteryToGrid - m_batteryPower->powerGridToBattery * m_batteryPower->singlePointEfficiencyDCToDC;
        double battery_dc = m_batteryPower->powerBatteryDC;
        if (battery_dc > 0) {
            battery_dc *= m_batteryPower->singlePointEfficiencyDCToDC;
        }
        else {
            battery_dc /= m_batteryPower->singlePointEfficiencyDCToDC;
        }
        double dc_power = battery_dc + m_batteryPower->powerSystem - m_batteryPower->powerSystemLoss;
        if (dc_power > 0.0) {
            dc_power *= m_batteryPower->singlePointEfficiencyDCToAC;
        }
        else if (m_batteryPower->singlePointEfficiencyDCToAC > 0) {
            dc_power /= m_batteryPower->singlePointEfficiencyDCToAC;
        }
        return dc_power;
    }

    double calc_met_load() {
        return m_batteryPower->powerBatteryToLoad + m_batteryPower->powerGridToLoad + m_batteryPower->powerSystemToLoad;
    }

    void check_net_flows(std::string id_string) {
        // increased error due to PV dc to ac conversion for system losses
        double dc_error = 1;
        double gen = calc_dc_gen();
        EXPECT_NEAR(m_batteryPower->powerGeneratedBySystem, gen, dc_error) << id_string;
        double met_load = calc_met_load();
        if (m_batteryPower->isOutageStep) {
            EXPECT_NEAR(met_load + m_batteryPower->powerCritLoadUnmet, m_batteryPower->powerCritLoad, dc_error) << id_string;
        }
        else {
            EXPECT_NEAR(met_load, m_batteryPower->powerLoad, dc_error) << id_string;
        }
    }

    void TearDown()
    {
        if (m_batteryPowerFlow) {
            delete m_batteryPowerFlow;
            m_batteryPowerFlow = nullptr;
        }
        if (m_sharedInverter) {
            delete m_sharedInverter;
            m_sharedInverter = nullptr;
        }
        if (sandia) {
            delete sandia;
            sandia = nullptr;
        }
        if (partload) {
            delete partload;
            partload = nullptr;
        }
        if (ond) {
            delete ond;
            ond = nullptr;
        }
    }

};

#endif
