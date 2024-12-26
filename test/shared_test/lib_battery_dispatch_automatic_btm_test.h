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


#ifndef SAM_SIMULATION_CORE_LIB_BATTERY_DISPATCH_AUTOMATIC_BTM_TEST_H
#define SAM_SIMULATION_CORE_LIB_BATTERY_DISPATCH_AUTOMATIC_BTM_TEST_H

#include <gtest/gtest.h>
#include <lib_battery_dispatch_automatic_btm.h>
#include "lib_battery_dispatch_test.h"

/**
* \class BatteryDispatchTest
*
* Automatic behind the meter battery dispatch algorithms
*
*/
class AutoBTMTest_lib_battery_dispatch : public BatteryProperties , public DispatchProperties
{
protected:

    capacity_lithium_ion_t * capacityModel;
    voltage_dynamic_t * voltageModel;
    thermal_t * thermalModel;
    lifetime_t * lifetimeModel;
    losses_t * lossModel;
    battery_t * batteryModel;
    BatteryPower * batteryPower;

    rate_data* util_rate{ nullptr };
    dispatch_automatic_behind_the_meter_t * dispatchAutoBTM{nullptr};

    double max_power = 50;
    double max_current = 500;
    double surface_area = 1.2 * 1.2 * 6;
    int n_series = 139;
    int n_strings = 89;

    std::vector<double> replacementCost = { 0.0 };
    int cyclingChoice = 1;
    std::vector<double> cyclingCost = { 0.0 };
    std::vector<double> omCost = { 0.0 };

    double interconnection_limit = 1e+38;

    bool chargeOnlySystemExceedLoad = true;
    bool dischargeOnlyLoadExceedSystem = true;
    bool dischargeToGrid = false;

    double min_outage_soc = 0.0;

    /*! Variables to store forecast data */
    std::vector<double> pv_prediction;
    std::vector<double> load_prediction;
    std::vector<double> cliploss_prediction;

public:

    void CreateBattery(double dtHour)
    {
        // For Manual Dispatch Test
        BatteryProperties::SetUp();
        n_strings = 445;

        capacityModel = new capacity_lithium_ion_t(Qfull * n_strings, SOC_init, SOC_max, SOC_min, dtHour);
        voltageModel = new voltage_dynamic_t(n_series, n_strings, Vnom_default, Vfull, Vexp, Vnom, Qfull, Qexp, Qnom, Vcut,
                                             C_rate, resistance, dtHour);
        lifetimeModel = new lifetime_calendar_cycle_t(cycleLifeMatrix, dtHour, calendar_q0, calendar_a, calendar_b, calendar_c);
        thermalModel = new thermal_t(1.0, mass, surface_area, resistance, Cp, h, capacityVsTemperature, T_room);
        lossModel = new losses_t();
        batteryModel = new battery_t(dtHour, chemistry, capacityModel, voltageModel, lifetimeModel, thermalModel, lossModel);

        int numberOfInverters = 40;
        m_sharedInverter = new SharedInverter(SharedInverter::SANDIA_INVERTER, numberOfInverters, sandia, partload, ond);
    }

    void CreateResidentialBattery(double dtHour)
    {
        n_strings = 9;
        CreateBattery(dtHour);
        delete m_sharedInverter;
        int numberOfInverters = 1;
        m_sharedInverter = new SharedInverter(SharedInverter::SANDIA_INVERTER, numberOfInverters, sandia, partload, ond);
    }


    void CreateBatteryWithLosses(double dtHour)
    {
        // For Manual Dispatch Test
        BatteryProperties::SetUp();
        n_strings = 445;

        capacityModel = new capacity_lithium_ion_t(Qfull * n_strings, SOC_init, SOC_max, SOC_min, dtHour);
        voltageModel = new voltage_dynamic_t(n_series, n_strings, Vnom_default, Vfull, Vexp, Vnom, Qfull, Qexp, Qnom, Vcut,
            C_rate, resistance, dtHour );
        lifetimeModel = new lifetime_calendar_cycle_t(cycleLifeMatrix, dtHour, calendar_q0, calendar_a, calendar_b, calendar_c);
        thermalModel = new thermal_t(1.0, mass, surface_area, resistance, Cp, h, capacityVsTemperature, T_room);

        std::vector<double> charging_losses(12, 1); // Monthly losses
        std::vector<double> discharging_losses(12, 2);
        std::vector<double> idle_losses(12, 0.5);
        std::vector<double> adjust_losses(8760, 0);
        lossModel = new losses_t(charging_losses, discharging_losses, idle_losses, adjust_losses);
        batteryModel = new battery_t(dtHour, chemistry, capacityModel, voltageModel, lifetimeModel, thermalModel, lossModel);

        int numberOfInverters = 40;
        m_sharedInverter = new SharedInverter(SharedInverter::SANDIA_INVERTER, numberOfInverters, sandia, partload, ond);
    }

    void TearDown()
    {
        BatteryProperties::TearDown();
        delete batteryModel;
        delete dispatchAutoBTM;
////        delete capacityModel;
    }
};


#endif //SAM_SIMULATION_CORE_LIB_BATTERY_DISPATCH_AUTOMATIC_BTM_TEST_H
