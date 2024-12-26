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


#ifndef SAM_SIMULATION_CORE_LIB_BATTERY_DISPATCH_AUTOMATIC_FOM_TEST_H
#define SAM_SIMULATION_CORE_LIB_BATTERY_DISPATCH_AUTOMATIC_FOM_TEST_H

#include <gtest/gtest.h>
#include <lib_battery_dispatch_automatic_fom.h>
#include "lib_battery_dispatch_test.h"


/**
* \class BatteryDispatchTest
*
* Automatic front of meter with AC or DC coupling of the battery dispatch algorithms
*
*/
class AutoFOM_lib_battery_dispatch : public BatteryProperties , public DispatchProperties
{
protected:
    thermal_t * thermalModel;
    lifetime_t * lifetimeModel;
    BatteryPower * batteryPower;

    capacity_lithium_ion_t * capacityModel;
    voltage_dynamic_t * voltageModel;
    losses_t * lossModel;
    battery_t *batteryModel;

    dispatch_automatic_front_of_meter_t * dispatchAuto{nullptr };

    double max_power = 25000;
    double surface_area = 686;

    /*! Variables to store forecast data */
    std::vector<double> pv_prediction;
    std::vector<double> load_prediction;
    std::vector<double> cliploss_prediction;

    std::vector<double> replacementCost = { 0.0 };
    std::vector<double> cyclingCost = { 0.005 };
    std::vector<double> omCost = { 0.0 };

    double interconnection_limit = 1e+38;
    std::vector<double> cleared_capacity;
    dispatch_t::CAPACITY_FORECAST_TYPE capacity_forecast_type = dispatch_t::PRICE_ONLY;
    double cleared_cap_percent = 0.0;
public:

    void CreateBattery(double dtHour)
    {
        // For testing Automated Front-of-meter DC-coupled
        BatteryProperties::SetUp();

        capacityModel = new capacity_lithium_ion_t(2.25 * 133227, 50, 100, 10, dtHour);
        voltageModel = new voltage_dynamic_t(139, 133227, 3.6, 4.10, 4.05, 3.4, 
                                             2.25, 0.04, 2.00, 0, 0.2, 0.2, dtHour );
        lifetimeModel = new lifetime_calendar_cycle_t(cycleLifeMatrix, dtHour, calendar_q0, calendar_a, calendar_b, calendar_c);
        thermalModel = new thermal_t(1.0, mass, surface_area, resistance, Cp, h, capacityVsTemperature, T_room);
        lossModel = new losses_t();
        batteryModel = new battery_t(dtHour, chemistry, capacityModel, voltageModel, lifetimeModel, thermalModel, lossModel);

        int numberOfInverters = 40;
        m_sharedInverter = new SharedInverter(SharedInverter::SANDIA_INVERTER, numberOfInverters, sandia, partload, ond);
        cleared_capacity.clear();
    }
    void CreateBatteryWithLosses(double dtHour)
    {
        // For testing Automated Front-of-meter DC-coupled
        BatteryProperties::SetUp();

        capacityModel = new capacity_lithium_ion_t(2.25 * 133227, 50, 100, 10, dtHour);
        voltageModel = new voltage_dynamic_t(139, 133227, 3.6, 4.10, 4.05, 3.4, 
            2.25, 0.04, 2.00, 0, 0.2, 0.2, dtHour );
        lifetimeModel = new lifetime_calendar_cycle_t(cycleLifeMatrix, dtHour, calendar_q0, calendar_a, calendar_b, calendar_c);
        thermalModel = new thermal_t(1.0, mass, surface_area, resistance, Cp, h, capacityVsTemperature, T_room);

        std::vector<double> charging_losses(12, 10); // Monthly losses
        std::vector<double> discharging_losses(12, 20);
        std::vector<double> idle_losses(12, 5);
        std::vector<double> adjust_losses(8760, 0);
        lossModel = new losses_t(charging_losses, discharging_losses, idle_losses, adjust_losses);
        batteryModel = new battery_t(dtHour, chemistry, capacityModel, voltageModel, lifetimeModel, thermalModel, lossModel);

        int numberOfInverters = 40;
        m_sharedInverter = new SharedInverter(SharedInverter::SANDIA_INVERTER, numberOfInverters, sandia, partload, ond);
        cleared_capacity.clear();
    }

    void CreateBatteryWithAvailabilityLosses(double dtHour)
    {
        // For testing Automated Front-of-meter DC-coupled
        BatteryProperties::SetUp();

        capacityModel = new capacity_lithium_ion_t(2.25 * 133227, 50, 100, 10, dtHour);
        voltageModel = new voltage_dynamic_t(139, 133227, 3.6, 4.10, 4.05, 3.4,
            2.25, 0.04, 2.00, 0, 0.2, 0.2, dtHour);
        lifetimeModel = new lifetime_calendar_cycle_t(cycleLifeMatrix, dtHour, calendar_q0, calendar_a, calendar_b, calendar_c);
        thermalModel = new thermal_t(1.0, mass, surface_area, resistance, Cp, h, capacityVsTemperature, T_room);

        std::vector<double> charging_losses(12, 0); // Monthly losses
        std::vector<double> discharging_losses(12, 0);
        std::vector<double> idle_losses(12, 0);
        std::vector<double> adjust_losses;
        for (size_t i = 0; i < 8760; i++) {
            if (i < 6) {
                adjust_losses.push_back(0.0);
            }
            else if (i < 48) {
                adjust_losses.push_back(0.25);
            }
            else {
                adjust_losses.push_back(0.0);
            }
        }

        lossModel = new losses_t(charging_losses, discharging_losses, idle_losses, adjust_losses);
        batteryModel = new battery_t(dtHour, chemistry, capacityModel, voltageModel, lifetimeModel, thermalModel, lossModel);

        int numberOfInverters = 40;
        m_sharedInverter = new SharedInverter(SharedInverter::SANDIA_INVERTER, numberOfInverters, sandia, partload, ond);
        cleared_capacity.clear();
    }

    void TearDown()
    {
        BatteryProperties::TearDown();
        delete batteryModel;
        delete dispatchAuto;
    }

};



#endif //SAM_SIMULATION_CORE_LIB_BATTERY_DISPATCH_AUTOMATIC_FOM_TEST_H
