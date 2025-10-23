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


#include "lib_battery_dispatch.h"
#include "lib_battery_powerflow.h"
#include "lib_power_electronics.h"
#include "lib_shared_inverter.h"

// 0.005 W when applied to power. Some inverters have a night time loss of 0.01 W
double powerflow_tolerance = 0.000005;

BatteryPower::BatteryPower(double dtHour) :
		dtHour(dtHour),
		powerSystem(0),
		powerSystemThroughSharedInverter(0),
		powerLoad(0),
        powerCritLoad(0),
		powerBatteryDC(0),
		powerBatteryAC(0),
		powerBatteryTarget(0),
		powerGrid(0),
		powerGeneratedBySystem(0),
		powerSystemToLoad(0),
		powerSystemToBatteryAC(0),
        powerSystemToBatteryDC(0),
		powerSystemToGrid(0),
		powerSystemClipped(0),
		powerClippedToBattery(0),
		powerGridToBattery(0),
		powerGridToLoad(0),
		powerBatteryToLoad(0),
		powerBatteryToGrid(0),
        powerBatteryToSystemLoad(0),
        powerBatteryToInverterDC(0),
        powerCritLoadUnmet(0),
        powerLossesUnmet(0),
		powerFuelCell(0),
		powerFuelCellToGrid(0),
		powerFuelCellToLoad(0),
		powerFuelCellToBattery(0),
		powerPVInverterDraw(0),
		powerBatteryChargeMaxDC(0),
		powerBatteryDischargeMaxDC(0),
		powerBatteryChargeMaxAC(0),
		powerBatteryDischargeMaxAC(0),
		powerSystemLoss(0),
		powerConversionLoss(0),
        powerInterconnectionLimit(1e+38),
        powerInterconnectionLoss(0),
        powerCurtailmentLimit(1e+38),
        voltageSystem(0),
        acLossWiring(0.0),
        acLossSystemAvailability(0.0),
        adjustLosses(0.0),
        acXfmrLoadLoss(0.0),
        acXfmrNoLoadLoss(0.0),
        acXfmrRating(0.0),
        isOutageStep(false),
		connectionMode(0),
        meterPosition(0),
		singlePointEfficiencyACToDC(0.96),
		singlePointEfficiencyDCToAC(0.96),
		singlePointEfficiencyDCToDC(0.99),
        sharedInverter(NULL),
        inverterEfficiencyCutoff(5),
		canSystemCharge(false),
		canClipCharge(false),
        canCurtailCharge(false),
		canGridCharge(false),
		canDischarge(false),
        canDischargeToGrid(false),
		canFuelCellCharge(false),
        chargeOnlySystemExceedLoad(true),
        dischargeOnlyLoadExceedSystem(true),
		stateOfChargeMax(1),
		stateOfChargeMin(0),
		depthOfDischargeMax(1),
        currentChargeMax(0),
        currentDischargeMax(0),
		tolerance(powerflow_tolerance){}

BatteryPower::BatteryPower(const BatteryPower& orig) {
    sharedInverter = orig.sharedInverter;
    dtHour = orig.dtHour;
    powerSystem = orig.powerSystem;
    powerSystemThroughSharedInverter = orig.powerSystemThroughSharedInverter;
    powerLoad = orig.powerLoad;
    powerCritLoad = orig.powerCritLoad;
    powerBatteryDC = orig.powerBatteryDC;
    powerBatteryAC = orig.powerBatteryAC;
    powerBatteryTarget = orig.powerBatteryTarget;
    powerGrid = orig.powerGrid;
    powerGeneratedBySystem = orig.powerGeneratedBySystem;
    powerSystemToLoad = orig.powerSystemToLoad;
    powerSystemToBatteryAC = orig.powerSystemToBatteryAC;
    powerSystemToBatteryDC = orig.powerSystemToBatteryDC;
    powerSystemToGrid = orig.powerSystemToGrid;
    powerSystemClipped = orig.powerSystemClipped;
    powerClippedToBattery = orig.powerClippedToBattery;
    powerGridToBattery = orig.powerGridToBattery;
    powerGridToLoad = orig.powerGridToLoad;
    powerBatteryToLoad = orig.powerBatteryToLoad;
    powerBatteryToGrid = orig.powerBatteryToGrid;
    powerBatteryToSystemLoad = orig.powerBatteryToSystemLoad;
    powerBatteryToInverterDC = orig.powerBatteryToInverterDC;
    powerCritLoadUnmet = orig.powerCritLoadUnmet;
    powerLossesUnmet = orig.powerLossesUnmet;
    powerFuelCell = orig.powerFuelCell;
    powerFuelCellToGrid = orig.powerFuelCellToGrid;
    powerFuelCellToLoad = orig.powerFuelCellToLoad;
    powerFuelCellToBattery = orig.powerFuelCellToBattery;
    powerPVInverterDraw = orig.powerPVInverterDraw;
    powerBatteryChargeMaxDC = orig.powerBatteryChargeMaxDC;
    powerBatteryDischargeMaxDC = orig.powerBatteryDischargeMaxDC;
    powerBatteryChargeMaxAC = orig.powerBatteryChargeMaxAC;
    powerBatteryDischargeMaxAC = orig.powerBatteryDischargeMaxAC;
    powerSystemLoss = orig.powerSystemLoss;
    powerConversionLoss = orig.powerConversionLoss;
    powerInterconnectionLimit = orig.powerInterconnectionLimit;
    powerInterconnectionLoss = orig.powerInterconnectionLoss;
    powerCurtailmentLimit = orig.powerCurtailmentLimit;
    voltageSystem = orig.voltageSystem;
    acLossWiring = orig.acLossWiring;
    acLossSystemAvailability = orig.acLossSystemAvailability;
    adjustLosses = orig.adjustLosses;
    acXfmrLoadLoss = orig.acXfmrLoadLoss;
    acXfmrNoLoadLoss = orig.acXfmrNoLoadLoss;
    acXfmrRating = orig.acXfmrRating;
    isOutageStep = orig.isOutageStep;
    connectionMode = orig.connectionMode;
    meterPosition = orig.meterPosition;
    singlePointEfficiencyACToDC = orig.singlePointEfficiencyACToDC;
    singlePointEfficiencyDCToAC = orig.singlePointEfficiencyDCToAC;
    singlePointEfficiencyDCToDC = orig.singlePointEfficiencyDCToDC;
    inverterEfficiencyCutoff = orig.inverterEfficiencyCutoff;
    canSystemCharge = orig.canSystemCharge;
    canClipCharge = orig.canClipCharge;
    canCurtailCharge = orig.canCurtailCharge;
    canGridCharge = orig.canGridCharge;
    canDischarge = orig.canDischarge;
    canDischargeToGrid = orig.canDischargeToGrid;
    canFuelCellCharge = orig.canFuelCellCharge;
    chargeOnlySystemExceedLoad = orig.chargeOnlySystemExceedLoad;
    dischargeOnlyLoadExceedSystem = orig.dischargeOnlyLoadExceedSystem;
    stateOfChargeMax = orig.stateOfChargeMax;
    stateOfChargeMin = orig.stateOfChargeMin;
    depthOfDischargeMax = orig.depthOfDischargeMax;
    currentChargeMax = orig.currentChargeMax;
    currentDischargeMax = orig.currentDischargeMax;
    tolerance = orig.tolerance;
}

void BatteryPower::setSharedInverter(SharedInverter* a_sharedInverter) {
    sharedInverter = a_sharedInverter;
}

void BatteryPower::reset()
{
	powerFuelCell = 0;
	powerFuelCellToGrid = 0;
	powerFuelCellToLoad = 0;
	powerFuelCellToBattery = 0;
	powerBatteryDC = 0;
	powerBatteryAC = 0;
	powerBatteryTarget = 0;
	powerBatteryToGrid = 0;
	powerBatteryToLoad = 0;
    powerBatteryToSystemLoad = 0;
    powerBatteryToInverterDC = 0;
	powerClippedToBattery = 0;
	powerConversionLoss = 0;
	powerGeneratedBySystem = 0;
	powerGrid = 0;
	powerGridToBattery = 0;
	powerGridToLoad = 0;
	powerLoad = 0;
    powerCritLoad = 0;
    powerCritLoadUnmet = 0;
    powerLossesUnmet = 0;
	powerSystem = 0;
	powerSystemThroughSharedInverter = 0;
	powerSystemClipped = 0;
	powerPVInverterDraw = 0;
	powerSystemToBatteryAC = 0;
    powerSystemToBatteryDC = 0;
	powerSystemToGrid = 0;
	powerSystemToLoad = 0;
    powerInterconnectionLoss = 0;
    powerCurtailmentLimit = 1e+38;
	voltageSystem = 0;
    acLossWiring = 0.0;
    acLossSystemAvailability = 0.0;
    adjustLosses = 0.0;
    acXfmrLoadLoss = 0.0;
    acXfmrNoLoadLoss = 0.0;
    isOutageStep = false;
}

double BatteryPower::adjustForACEfficiencies(double power, double loss) {
    if (power > 0) {
        return (power + loss) / singlePointEfficiencyDCToAC;
    }
    else {
        return power * singlePointEfficiencyACToDC;
    }
}

// DC-connected is harder to convert from AC, must make assumptions about inverter efficiency and charge shource
double BatteryPower::adjustForDCEfficiencies(double power, double loss) {
    if (power > 0) {
        return (power + loss) / (singlePointEfficiencyDCToDC * singlePointEfficiencyACToDC);

    }
    // Need to bring ac load and charging values to DC side. Assume current inverter efficiency continues through dispatch forecast
    else {
        double ac_to_dc_eff = singlePointEfficiencyACToDC;
        if (sharedInverter->efficiencyAC > 5) // 5% is the cutoff in lib_battery_powerflow
        {
            ac_to_dc_eff = sharedInverter->efficiencyAC * 0.01;
        }
        return power * singlePointEfficiencyDCToDC / ac_to_dc_eff;
    }
}

double BatteryPower::getMaxACChargePower() {
    return (1 - adjustLosses) * powerBatteryChargeMaxAC;
}
double BatteryPower::getMaxACDischargePower() {
    double max_discharge_ac = (1 - adjustLosses) * powerBatteryDischargeMaxAC;
    if (connectionMode == DC_CONNECTED) {
        max_discharge_ac *= (1 - acLossSystemAvailability);
    }
    return max_discharge_ac;
}
double BatteryPower::getMaxDCChargePower() {
    return (1 - adjustLosses) * powerBatteryChargeMaxDC;
}
double BatteryPower::getMaxDCDischargePower() {
    return (1 - adjustLosses) * powerBatteryDischargeMaxDC;
}
double BatteryPower::getMaxChargeCurrent() {
    return (1 - adjustLosses) * currentChargeMax;
}
double BatteryPower::getMaxDischargeCurrent() {
    return (1 - adjustLosses) * currentDischargeMax;
}

BatteryPowerFlow::BatteryPowerFlow(double dtHour)
{
    std::unique_ptr<BatteryPower> tmp(new BatteryPower(dtHour));
    m_BatteryPower = std::move(tmp);
}
BatteryPowerFlow::BatteryPowerFlow(const BatteryPowerFlow& powerFlow)
{
    std::unique_ptr<BatteryPower> tmp(new BatteryPower(*powerFlow.m_BatteryPower));
    m_BatteryPower = std::move(tmp);
}
BatteryPower* BatteryPowerFlow::getBatteryPower()
{
    return m_BatteryPower.get();
}
void BatteryPowerFlow::calculate()
{
    if (m_BatteryPower->connectionMode == ChargeController::AC_CONNECTED) {
        calculateACConnected();
    }
    else if (m_BatteryPower->connectionMode == ChargeController::DC_CONNECTED) {
        calculateDCConnected();
    }
}
void BatteryPowerFlow::initialize(double stateOfCharge, bool systemPriorityCharge)
{
	// If the battery is allowed to discharge, do so
	if (m_BatteryPower->canDischarge && stateOfCharge > m_BatteryPower->stateOfChargeMin + 1.0 &&
		(m_BatteryPower->powerSystem < m_BatteryPower->powerLoad || !m_BatteryPower->dischargeOnlyLoadExceedSystem || m_BatteryPower->meterPosition == dispatch_t::FRONT))
	{
		// try to discharge full amount.  Will only use what battery can provide
		m_BatteryPower->powerBatteryDC = m_BatteryPower->getMaxDCDischargePower();
	}
	// Is there extra power from system
	else if ((((m_BatteryPower->powerSystem > m_BatteryPower->powerLoad) || !m_BatteryPower->chargeOnlySystemExceedLoad) && m_BatteryPower->canSystemCharge) || m_BatteryPower->canGridCharge || m_BatteryPower->canClipCharge || m_BatteryPower->canCurtailCharge)
	{
        if (m_BatteryPower->canClipCharge) {
            m_BatteryPower->powerBatteryDC = -m_BatteryPower->powerSystemClipped;
        }

        if (m_BatteryPower->canCurtailCharge) {
            double interconnectionCapacity = std::fmin(m_BatteryPower->powerInterconnectionLimit, m_BatteryPower->powerCurtailmentLimit) - m_BatteryPower->powerSystem;
            if (interconnectionCapacity < 0.0 )
            {
                m_BatteryPower->powerBatteryDC = interconnectionCapacity * m_BatteryPower->singlePointEfficiencyACToDC;
            }
        }

		if (m_BatteryPower->canSystemCharge)
		{
            if (systemPriorityCharge) {
                m_BatteryPower->powerBatteryDC = -1.0 * m_BatteryPower->powerSystem;
            }
            else {
                // use all power available, it will only use what it can handle
                m_BatteryPower->powerBatteryDC = -(m_BatteryPower->powerSystem - m_BatteryPower->powerLoad);
            }
		}
		// if we want to charge from grid in addition to, or without array, we can always charge at max power
		if (m_BatteryPower->canGridCharge) {
			m_BatteryPower->powerBatteryDC = -m_BatteryPower->getMaxDCChargePower();
		}
	}
    m_BatteryPower->powerBatteryTarget = m_BatteryPower->powerBatteryDC;
}

void BatteryPowerFlow::reset()
{
    m_BatteryPower->reset();
}


void BatteryPowerFlow::calculateACConnected()
{
    // The battery power is initially a DC power, which must be converted to AC for powerflow
    double P_battery_dc = m_BatteryPower->powerBatteryDC;

    // These quantities are all AC quantities in KW unless otherwise specified
    double P_pv_ac = m_BatteryPower->powerSystem;
    double P_fuelcell_ac = m_BatteryPower->powerFuelCell;
    double P_inverter_draw_ac = m_BatteryPower->powerPVInverterDraw;
    double P_load_ac = m_BatteryPower->powerLoad;
    double P_crit_load_ac = m_BatteryPower->powerCritLoad;
    double P_system_loss_ac = m_BatteryPower->powerSystemLoss;
    double P_pv_to_batt_ac, P_grid_to_batt_ac, P_fuelcell_to_batt_ac,
        P_batt_to_load_ac, P_grid_to_load_ac, P_pv_to_load_ac, P_fuelcell_to_load_ac, P_available_pv,
        P_pv_to_grid_ac, P_batt_to_grid_ac, P_fuelcell_to_grid_ac, P_gen_ac, P_grid_ac,
        P_grid_to_batt_loss_ac, P_batt_to_load_loss_ac, P_batt_to_grid_loss_ac, P_pv_to_batt_loss_ac,
        P_batt_to_system_loss, P_batt_to_system_loss_conversion_loss, P_interconnection_loss_ac, P_crit_load_unmet_ac,
        P_batt_to_pv_inverter, P_unmet_losses;
    P_pv_to_batt_ac = P_grid_to_batt_ac = P_fuelcell_to_batt_ac =
        P_batt_to_load_ac = P_grid_to_load_ac = P_pv_to_load_ac = P_fuelcell_to_load_ac = P_available_pv =
        P_pv_to_grid_ac = P_batt_to_grid_ac = P_fuelcell_to_grid_ac = P_gen_ac = P_grid_ac =
        P_grid_to_batt_loss_ac = P_batt_to_load_loss_ac = P_batt_to_grid_loss_ac = P_pv_to_batt_loss_ac =
        P_batt_to_system_loss = P_batt_to_system_loss_conversion_loss = P_interconnection_loss_ac = P_crit_load_unmet_ac =
        P_batt_to_pv_inverter = P_unmet_losses = 0;

    double P_ac_losses = 0.0; // These are reported by the loss variables in the system model, just record here for powerflow calcs

    // Dispatch code treats these independetly, here we only need to know the smaller number
    double P_grid_limit_ac = std::fmin(m_BatteryPower->powerInterconnectionLimit, m_BatteryPower->powerCurtailmentLimit);

    // convert the calculated DC power to AC, considering the microinverter efficiences
    double P_battery_ac = 0;
    if (P_battery_dc < 0)
        P_battery_ac = P_battery_dc / m_BatteryPower->singlePointEfficiencyACToDC;
    else if (P_battery_dc > 0)
        P_battery_ac = P_battery_dc * m_BatteryPower->singlePointEfficiencyDCToAC;

    if (std::abs(P_battery_ac) < tolerance) {
        P_battery_ac = 0;
    }

    // Code simplification to remove redundancy for code that should use either critical load or actual load
    double calc_load_ac = (m_BatteryPower->isOutageStep ? P_crit_load_ac : P_load_ac);

    // charging and idle
    if (P_battery_ac <= 0)
    {
        // Test if battery is charging erroneously
        if (!(m_BatteryPower->canSystemCharge || m_BatteryPower->canGridCharge || m_BatteryPower->canFuelCellCharge || m_BatteryPower->canCurtailCharge) && P_battery_ac < 0) {
            P_pv_to_batt_ac = P_grid_to_batt_ac = P_fuelcell_to_batt_ac = 0;
            P_battery_ac = 0;
        }

        // If PV must meet load first, assign right away
        if (m_BatteryPower->chargeOnlySystemExceedLoad) {
            P_pv_to_load_ac = P_pv_ac;

            if (P_pv_to_load_ac > calc_load_ac) {
                P_pv_to_load_ac = calc_load_ac;
            }
            // Fuel cell goes to load next
            P_fuelcell_to_load_ac = std::fmin(calc_load_ac - P_pv_to_load_ac, P_fuelcell_ac);
        }

        // Excess PV can go to battery, if PV can cover charging losses
        if (m_BatteryPower->canSystemCharge || m_BatteryPower->canCurtailCharge) {
            P_pv_to_batt_ac = std::abs(P_battery_ac);
            P_available_pv = P_pv_ac - P_pv_to_load_ac - P_system_loss_ac;
            if (P_pv_to_batt_ac > P_available_pv)
            {
                if (P_available_pv < 0.0) {
                    P_pv_to_batt_ac = 0.0;
                  }
                else {
                    P_pv_to_batt_ac = P_available_pv;
                }
            }
        }
        // Apply PV to losses if possible, grid (via gen) if not
        bool pv_handles_losses = P_pv_to_batt_ac > 0 || P_available_pv > 0.0;

        // If flexible charging powerflow, assign PV now
        if (!m_BatteryPower->chargeOnlySystemExceedLoad) {
            if (pv_handles_losses) {
                P_pv_to_load_ac = P_pv_ac - P_pv_to_batt_ac - P_system_loss_ac;
            }
            else {
                P_pv_to_load_ac = P_pv_ac - P_pv_to_batt_ac;
            }

            if (P_pv_to_load_ac > calc_load_ac) {
                P_pv_to_load_ac = calc_load_ac;
            }
            // Fuel cell goes to load next
            P_fuelcell_to_load_ac = std::fmin(calc_load_ac - P_pv_to_load_ac, P_fuelcell_ac);
        }

        // Fuelcell can also charge battery
        if (m_BatteryPower->canFuelCellCharge) {
            P_fuelcell_to_batt_ac = std::fmin(std::fmax(0, std::abs(P_battery_ac) - P_pv_to_batt_ac), P_fuelcell_ac - P_fuelcell_to_load_ac);
        }
        // Grid can also charge battery
        if (m_BatteryPower->canGridCharge) {
            P_grid_to_batt_ac = std::fmax(0, std::abs(P_battery_ac) - P_pv_to_batt_ac - P_fuelcell_to_batt_ac);
        }

        if (m_BatteryPower->isOutageStep && !m_BatteryPower->canFuelCellCharge) {
            // Need to cover idle loss by reducing PV to load
            if (P_available_pv < 0.0 && std::abs(P_battery_ac) < tolerance && P_pv_ac > P_system_loss_ac) {
                P_pv_to_load_ac = P_pv_ac - P_system_loss_ac;
                pv_handles_losses = true;
            }
        }

        if (pv_handles_losses) {
            P_pv_to_grid_ac = P_pv_ac - P_pv_to_batt_ac - P_pv_to_load_ac - P_system_loss_ac; // PV meets charging or idle losses or no losses, export to grid if possible
        }
        else {
            P_pv_to_grid_ac = P_pv_ac - P_pv_to_batt_ac - P_pv_to_load_ac; // PV is fully allocated to load, grid must handle losses
        }
        P_fuelcell_to_grid_ac = P_fuelcell_ac - P_fuelcell_to_load_ac - P_fuelcell_to_batt_ac;

        if (m_BatteryPower->isOutageStep) {
            P_interconnection_loss_ac = P_pv_to_grid_ac + P_fuelcell_to_grid_ac;
            P_pv_to_grid_ac = 0;
            P_fuelcell_to_grid_ac = 0;
        }
    }
    // discharging, not idle
    else
    {

        // Test if battery is discharging erroneously
        if (!m_BatteryPower->canDischarge && P_battery_ac > 0) {
            P_batt_to_grid_ac = P_batt_to_load_ac = 0;
            P_battery_ac = 0;
        }
        if (m_BatteryPower->dischargeOnlyLoadExceedSystem) {
            P_pv_to_load_ac = P_pv_ac;

            // Excess PV production, no other component meets load
            if (P_pv_ac >= calc_load_ac)
            {
                P_pv_to_load_ac = calc_load_ac;
                P_fuelcell_to_load_ac = 0;
                P_batt_to_load_ac = 0;

                // discharging to grid
                P_pv_to_grid_ac = P_pv_ac - P_pv_to_load_ac;
                P_fuelcell_to_grid_ac = P_fuelcell_ac;
            }
            else {
                P_fuelcell_to_load_ac = std::fmin(P_fuelcell_ac, calc_load_ac - P_pv_to_load_ac);
                P_batt_to_load_ac = std::fmax(0, std::fmin(P_battery_ac - P_system_loss_ac, calc_load_ac - P_pv_to_load_ac - P_fuelcell_to_load_ac));
            }
        }
        else {
            P_batt_to_load_ac = std::fmin(P_battery_ac, calc_load_ac);
            P_fuelcell_to_load_ac = std::fmin(P_fuelcell_ac, calc_load_ac - P_batt_to_load_ac);
            P_pv_to_load_ac = std::fmin(std::fmax(0, calc_load_ac - P_fuelcell_to_load_ac - P_batt_to_load_ac), P_pv_ac);
            P_pv_to_grid_ac = std::fmax(0, P_pv_ac - P_pv_to_load_ac);
            P_fuelcell_to_grid_ac = std::fmax(0, P_fuelcell_ac - P_fuelcell_to_load_ac);
        }

        if (m_BatteryPower->isOutageStep) {
            if (P_battery_ac - P_batt_to_load_ac < -1.0 * P_inverter_draw_ac) {
                double diff = -1.0 * P_inverter_draw_ac - (P_battery_ac - P_batt_to_load_ac);
                P_batt_to_load_ac -= diff;
                P_batt_to_load_ac = fmax(P_batt_to_load_ac, 0.0); // Prevent small discharges from driving this number negative
            }
            P_batt_to_pv_inverter = -1.0 * P_inverter_draw_ac;
            P_interconnection_loss_ac = P_pv_to_grid_ac + P_fuelcell_to_grid_ac;
            P_pv_to_grid_ac = 0;
            P_fuelcell_to_grid_ac = 0;
        }

        // Preliminary batt to grid for DC losses
        P_batt_to_grid_ac = P_battery_ac - P_system_loss_ac - P_batt_to_load_ac - P_batt_to_pv_inverter;

        P_fuelcell_to_grid_ac = P_fuelcell_ac - P_fuelcell_to_load_ac;
        P_batt_to_system_loss = P_system_loss_ac;
    }

    // compute DC losses
    P_pv_to_batt_loss_ac = P_pv_to_batt_ac * (1 - m_BatteryPower->singlePointEfficiencyACToDC);
    P_grid_to_batt_loss_ac = P_grid_to_batt_ac * (1 - m_BatteryPower->singlePointEfficiencyACToDC);
    P_batt_to_load_loss_ac = P_batt_to_load_ac * (1 / m_BatteryPower->singlePointEfficiencyDCToAC - 1);
    P_batt_to_grid_loss_ac = P_batt_to_grid_ac * (1 / m_BatteryPower->singlePointEfficiencyDCToAC - 1);
    P_batt_to_system_loss_conversion_loss = P_batt_to_system_loss * (1 / m_BatteryPower->singlePointEfficiencyDCToAC - 1); // system losses are AC losses for an AC battery, so need to do DC to AC conversion if the battery is meeting those losses

    // Compute total system output and grid power flow
    P_gen_ac = P_pv_ac + P_fuelcell_ac + P_inverter_draw_ac + P_battery_ac - P_system_loss_ac;

    // Final batt to grid for outage accounting
    if (P_battery_ac > 0)
    {
        P_batt_to_grid_ac = P_battery_ac - P_system_loss_ac - P_batt_to_load_ac - P_batt_to_pv_inverter;
        if (m_BatteryPower->isOutageStep && P_batt_to_grid_ac > tolerance) {
            m_BatteryPower->powerBatteryDC = (P_battery_ac - P_batt_to_grid_ac) / m_BatteryPower->singlePointEfficiencyDCToAC;
            return calculateACConnected();
        }
    }

    P_ac_losses = 0.0; // TODO?

    double P_loss_coverage = 0;
    if (m_BatteryPower->isOutageStep) {
        if (P_ac_losses < 0) {
            P_ac_losses = 0; // Inverter losses are already counted in gen, don't double count below. PVSAMV1 code will do the right thing after this calc
        }
        double gen_for_ac_losses = P_gen_ac - P_batt_to_load_ac - P_pv_to_load_ac - P_fuelcell_to_load_ac;
        // If gen is greater than load and losses, then there's curtailment and we don't need to worry about it
        if (gen_for_ac_losses > P_ac_losses) {
            P_loss_coverage = 0;
        }
        else {
            P_loss_coverage = P_ac_losses - gen_for_ac_losses;
        }
        P_crit_load_unmet_ac = P_crit_load_ac - P_pv_to_load_ac - P_batt_to_load_ac - P_fuelcell_to_load_ac + P_loss_coverage;
        if (P_crit_load_unmet_ac > P_crit_load_ac) {
            P_unmet_losses = P_crit_load_unmet_ac - P_crit_load_ac;
            P_crit_load_unmet_ac = P_crit_load_ac;
        }
        P_grid_to_load_ac = 0;
        P_grid_ac = P_gen_ac - P_crit_load_ac - P_interconnection_loss_ac + P_crit_load_unmet_ac + P_unmet_losses; // This should be zero, but if it's not the error checking below will fix it
        if (P_gen_ac < 0.0 && P_unmet_losses > 0.0) {
            P_gen_ac += P_unmet_losses; // Unmet losses should be categorized as such, not in gen
        }
    }
    else {
        P_grid_to_load_ac = P_load_ac - P_pv_to_load_ac - P_batt_to_load_ac - P_fuelcell_to_load_ac;
        // Grid charging loss accounted for in P_battery_ac
        P_grid_ac = P_gen_ac - P_load_ac;

        if (P_grid_ac > P_grid_limit_ac) {
            double grid_diff = P_grid_ac - P_grid_limit_ac - P_inverter_draw_ac;
            // Update grid variables first
            P_grid_ac -= grid_diff;
            P_interconnection_loss_ac += grid_diff;
            if (grid_diff > P_pv_to_grid_ac) {
                // Curtail PV "first"
                grid_diff -= P_pv_to_grid_ac;
                P_pv_to_grid_ac = 0;
                if (grid_diff > P_fuelcell_to_grid_ac) {
                    grid_diff -= P_fuelcell_to_grid_ac;
                    P_fuelcell_to_grid_ac = 0;
                    // Then curtail battery
                    P_batt_to_grid_ac -= grid_diff;
                }
                else {
                    P_fuelcell_to_grid_ac -= grid_diff;
                }

            }
            else {
                P_pv_to_grid_ac -= grid_diff;
            }
        }
    }

    // Error checking trying to charge from grid when not allowed
    if (!m_BatteryPower->canGridCharge && P_battery_ac < -tolerance) {
        if (((std::abs(P_grid_ac - P_grid_to_load_ac) > tolerance) && (-P_grid_ac > P_grid_to_load_ac + tolerance)) || (std::abs(P_loss_coverage) > tolerance)) {
            P_battery_ac = P_pv_ac - P_pv_to_grid_ac - P_pv_to_load_ac - P_system_loss_ac;
            P_battery_ac = P_battery_ac > 0 ? P_battery_ac : 0; // Don't swap from charging to discharging
            m_BatteryPower->powerBatteryDC = -P_battery_ac * m_BatteryPower->singlePointEfficiencyACToDC;
            return calculateACConnected();
        }
    }

    // check tolerances
    if (std::abs(P_grid_to_load_ac) < m_BatteryPower->tolerance)
        P_grid_to_load_ac = 0;
    if (std::abs(P_grid_to_batt_ac) < m_BatteryPower->tolerance)
        P_grid_to_batt_ac = 0;
    if (std::abs(P_grid_ac) < m_BatteryPower->tolerance)
        P_grid_ac = 0;
    if (std::abs(P_crit_load_unmet_ac) < m_BatteryPower->tolerance)
        P_crit_load_unmet_ac = 0;
    if (std::abs(P_interconnection_loss_ac) < m_BatteryPower->tolerance) {
        P_interconnection_loss_ac = 0;
    }
   
	// assign outputs
	m_BatteryPower->powerBatteryAC = P_battery_ac;
	m_BatteryPower->powerGrid = P_grid_ac;
	m_BatteryPower->powerGeneratedBySystem = P_gen_ac;
	m_BatteryPower->powerSystemToLoad = P_pv_to_load_ac;
	m_BatteryPower->powerSystemToBatteryAC = P_pv_to_batt_ac;
    m_BatteryPower->powerSystemToBatteryDC = P_pv_to_batt_ac * m_BatteryPower->singlePointEfficiencyACToDC; // Convert to DC for output consistency
	m_BatteryPower->powerSystemToGrid = P_pv_to_grid_ac;
	m_BatteryPower->powerGridToBattery = P_grid_to_batt_ac;
	m_BatteryPower->powerGridToLoad = P_grid_to_load_ac;
	m_BatteryPower->powerBatteryToLoad = P_batt_to_load_ac;
	m_BatteryPower->powerBatteryToGrid = P_batt_to_grid_ac;
    m_BatteryPower->powerBatteryToSystemLoad = P_batt_to_pv_inverter;
	m_BatteryPower->powerFuelCellToBattery = P_fuelcell_to_batt_ac;
	m_BatteryPower->powerFuelCellToLoad= P_fuelcell_to_load_ac;
	m_BatteryPower->powerFuelCellToGrid = P_fuelcell_to_grid_ac;
	m_BatteryPower->powerConversionLoss = P_batt_to_load_loss_ac + P_batt_to_grid_loss_ac + P_grid_to_batt_loss_ac + P_pv_to_batt_loss_ac + P_batt_to_system_loss_conversion_loss;
    m_BatteryPower->powerInterconnectionLoss = P_interconnection_loss_ac;
    m_BatteryPower->powerCritLoadUnmet = P_crit_load_unmet_ac;
    m_BatteryPower->powerLossesUnmet = P_unmet_losses;
}

void BatteryPowerFlow::calculateDCConnected()
{
    // Quantities are AC in KW unless otherwise specified
    double P_load_ac = m_BatteryPower->powerLoad;
    double P_crit_load_ac = m_BatteryPower->powerCritLoad;
    double P_battery_ac, P_pv_ac, P_gen_ac, P_grid_to_batt_ac, 
        P_batt_to_load_ac, P_grid_to_load_ac, P_pv_to_load_ac,
        P_pv_to_grid_ac, P_pv_to_batt_ac, P_batt_to_grid_ac, P_grid_ac, P_conversion_loss_ac,
        P_interconnection_loss_ac, P_crit_load_unmet_ac, P_unmet_losses,
        P_batt_to_inverter_dc;
    P_battery_ac = P_pv_ac = P_gen_ac = P_grid_to_batt_ac =
        P_batt_to_load_ac = P_grid_to_load_ac = P_pv_to_load_ac =
        P_pv_to_grid_ac = P_pv_to_batt_ac = P_batt_to_grid_ac =  P_grid_ac = P_conversion_loss_ac =
        P_interconnection_loss_ac = P_crit_load_unmet_ac = P_unmet_losses =
        P_batt_to_inverter_dc = 0;

    double ac_loss_percent = 1 - (1 - m_BatteryPower->acLossWiring) * (1 - m_BatteryPower->acLossSystemAvailability); // Combine the loss types into one number - they're all on the AC side of the inverter

    if (m_BatteryPower->isOutageStep) {
        m_BatteryPower->acXfmrNoLoadLoss = 0;
    }
    else {
        ac_loss_percent *= (1 - m_BatteryPower->acXfmrLoadLoss);
    }

    double P_ac_losses = 0.0; // These are reported by the loss variables in the system model, just record here for powerflow calcs

    // Quantities are DC in KW unless otherwise specified
    double P_pv_to_batt_dc, P_grid_to_batt_dc, P_pv_to_inverter_dc;
    P_pv_to_batt_dc = P_grid_to_batt_dc = P_pv_to_inverter_dc = 0;
    double P_system_loss_dc = m_BatteryPower->powerSystemLoss; // Units of power sytem loss match battery connection type, see help for details

    // Dispatch code treats these independetly, here we only need to know the smaller number
    double P_grid_limit_ac = std::fmin(m_BatteryPower->powerInterconnectionLimit, m_BatteryPower->powerCurtailmentLimit);

	// The battery power and PV power are initially DC, which must be converted to AC for powerflow
	double P_battery_dc_pre_bms = m_BatteryPower->powerBatteryDC;
	double P_battery_dc = m_BatteryPower->powerBatteryDC;
	double P_pv_dc = m_BatteryPower->powerSystem;

    // convert the calculated DC power to DC at the PV system voltage
    if (P_battery_dc_pre_bms < 0)
        P_battery_dc = P_battery_dc_pre_bms / m_BatteryPower->singlePointEfficiencyDCToDC;
    else if (P_battery_dc > 0)
        P_battery_dc = P_battery_dc_pre_bms * m_BatteryPower->singlePointEfficiencyDCToDC;

    bool pv_handles_loss = P_pv_dc > P_system_loss_dc && P_battery_dc <= tolerance; // Idle losses need to be handled by PV to keep inverter flows consistent, even if PV charging is disallowed
    double P_gen_dc = P_pv_dc + P_battery_dc - P_system_loss_dc;

    if (std::abs(P_gen_dc) < tolerance) {
        P_gen_dc = 0.0;
    }

    // in the event that PV system isn't operating, assume battery BMS converts battery voltage to nominal inverter input at the weighted efficiency
    double voltage = m_BatteryPower->voltageSystem;
    double efficiencyDCAC = m_BatteryPower->sharedInverter->efficiencyAC * 0.01;
    double maxEfficiencyDCAC = m_BatteryPower->sharedInverter->getMaxPowerEfficiency() * 0.01;
    if (voltage <= 0) {
        voltage = m_BatteryPower->sharedInverter->getInverterDCNominalVoltage();
    }
    if (std::isnan(efficiencyDCAC) || m_BatteryPower->sharedInverter->efficiencyAC <= 0) {
        efficiencyDCAC = maxEfficiencyDCAC;
    }

    double P_battery_ac_post_loss = 0;
    // charging, not idle
    if (P_battery_dc < 0)
    {
        // First check whether battery charging came from PV.
        // Assumes that if battery is charging and can charge from PV, that it will charge from PV before using the grid
        if (m_BatteryPower->canSystemCharge || m_BatteryPower->canClipCharge || m_BatteryPower->canCurtailCharge) {
            P_pv_to_batt_dc = std::abs(P_battery_dc);
            if (P_pv_to_batt_dc > P_pv_dc - P_system_loss_dc) {
                P_pv_to_batt_dc = P_pv_dc - P_system_loss_dc;
                if (P_pv_to_batt_dc < 0) {
                    P_pv_to_batt_dc = 0.0; // Don't allow losses to drive this negative
                }
            }   
        }
        
        if (P_pv_dc >= P_pv_to_batt_dc + P_system_loss_dc)
        {
            P_pv_to_inverter_dc = P_pv_dc - P_pv_to_batt_dc - P_system_loss_dc;
        }
        else if (m_BatteryPower->isOutageStep) {
            if (P_pv_dc > 0.0) {
                P_pv_to_inverter_dc = P_pv_dc - P_pv_to_batt_dc - P_system_loss_dc;
                if (P_pv_to_inverter_dc > 0.0) {
                    pv_handles_loss = true;
                }
                else {
                    P_pv_to_inverter_dc = 0.0;
                    P_unmet_losses = std::abs(P_pv_to_inverter_dc);
                    pv_handles_loss = false;
                }
            }
            else {
                P_unmet_losses = P_system_loss_dc;
                pv_handles_loss = false;
            }
        }
        else {
            P_pv_to_inverter_dc = P_pv_dc - P_pv_to_batt_dc;
            pv_handles_loss = false;
        }

        // Any remaining charge comes from grid if allowed
        P_grid_to_batt_dc = std::abs(P_battery_dc) - P_pv_to_batt_dc;

        if (std::abs(P_grid_to_batt_dc) < tolerance) {
            P_grid_to_batt_dc = 0.0;
        }

        if ((!m_BatteryPower->canGridCharge || m_BatteryPower->isOutageStep) && P_grid_to_batt_dc > 0.0) {
            m_BatteryPower->powerBatteryDC = -P_pv_to_batt_dc * m_BatteryPower->singlePointEfficiencyDCToDC;
            return calculateDCConnected();
        }

        // Apply AC side charging losses here so inverter can pull in more power as needed
        double grid_losses = P_grid_to_batt_dc * ac_loss_percent;
        if (grid_losses > 0) {
            P_grid_to_batt_dc -= grid_losses;
            P_battery_dc += grid_losses; // Bring negative numbers closer to zero
            P_gen_dc += grid_losses;
            P_battery_dc_pre_bms += grid_losses * m_BatteryPower->singlePointEfficiencyDCToDC;
        }

        // Assume inverter only "sees" the net flow in one direction, though practically
        // there should never be case where P_pv_dc - P_pv_to_batt_dc > 0 and P_grid_to_batt_dc > 0 simultaneously
        double P_gen_dc_inverter = P_pv_to_inverter_dc - P_grid_to_batt_dc;
        if (!pv_handles_loss && !m_BatteryPower->isOutageStep) {
            P_gen_dc_inverter -= P_system_loss_dc; // Losses are coming from grid through inverter in this case
        }

        // Record any power from inverter to battery during charging
        P_batt_to_inverter_dc = -1.0 * P_grid_to_batt_dc;

        // convert the DC power to AC
        m_BatteryPower->sharedInverter->calculateACPower(P_gen_dc_inverter, voltage, m_BatteryPower->sharedInverter->Tdry_C);

        // Only update inverter efficiency if the inverter is running. Otherwise use max efficency from above
        if (m_BatteryPower->sharedInverter->powerAC_kW > 0.0 && P_gen_dc_inverter > 0 || std::abs(m_BatteryPower->sharedInverter->powerAC_kW) > m_BatteryPower->sharedInverter->powerNightLoss_kW) {
            
            efficiencyDCAC = m_BatteryPower->sharedInverter->efficiencyAC * 0.01;

            // Restrict low efficiency so don't get infinites - can sometimes have PV charging scenarios where inverter isn't running
            if (efficiencyDCAC <= 0.05 && (P_grid_to_batt_dc > 0 || P_pv_to_inverter_dc > 0)) {
                efficiencyDCAC = 0.05;
            }

            // This is a traditional DC/AC efficiency loss
            if (P_gen_dc_inverter > 0) {
                m_BatteryPower->sharedInverter->powerAC_kW = P_gen_dc_inverter * efficiencyDCAC;
            }
            // if we are charging from grid, then we actually care about the amount of grid power it took to achieve the DC value
            else {
                m_BatteryPower->sharedInverter->powerAC_kW = P_gen_dc_inverter / efficiencyDCAC;
            }
            m_BatteryPower->sharedInverter->efficiencyAC = efficiencyDCAC * 100;
            P_grid_to_batt_ac = P_grid_to_batt_dc / efficiencyDCAC; // If only charging from grid this should match powerAC_kW (don't apply wiring losses here)
        }

        // Compute the AC quantities
        P_gen_ac = m_BatteryPower->sharedInverter->powerAC_kW; // Gen will have ac losses applied elsewhere
        if (std::isnan(P_gen_ac) && m_BatteryPower->sharedInverter->powerDC_kW == 0) {
            P_gen_ac = 0;
            P_grid_to_batt_ac = 0;
        }

        double P_xfmr_ll = m_BatteryPower->acXfmrLoadLoss;
        double P_transformer_loss = 0;
        if (!m_BatteryPower->isOutageStep && P_gen_ac > 0) {
            P_transformer_loss = Transformer::transformerLoss(P_gen_ac * (1 - m_BatteryPower->acLossWiring), m_BatteryPower->acXfmrLoadLoss, m_BatteryPower->acXfmrRating, P_xfmr_ll, m_BatteryPower->acXfmrNoLoadLoss);
        }
        P_ac_losses = P_gen_ac - (P_gen_ac * (1 - m_BatteryPower->acLossWiring) - P_transformer_loss) * (1 - m_BatteryPower->acLossSystemAvailability);
        if (P_gen_ac > tolerance) {
            ac_loss_percent = P_ac_losses / P_gen_ac;
        }

        P_pv_ac = P_pv_to_inverter_dc * efficiencyDCAC * (1 - ac_loss_percent);

        if (m_BatteryPower->isOutageStep) {
            P_pv_to_load_ac = P_crit_load_ac;
            // Future feature: if PV can meet non-critical load during an outage instead of being curtailed, do so.
        }
        else {
            P_pv_to_load_ac = P_load_ac;
        }
        if (P_pv_to_load_ac > P_pv_ac) {
            P_pv_to_load_ac = P_pv_ac;
        }

        if (m_BatteryPower->isOutageStep) {
            P_pv_to_grid_ac = 0;
            P_interconnection_loss_ac = P_pv_ac - P_pv_to_load_ac;
        }
        else {
            P_pv_to_grid_ac = P_pv_ac - P_pv_to_load_ac;
        }

        // In this case, we have a combo of Battery DC power from the PV array, and potentially AC power from the grid
        if (P_pv_to_batt_dc + P_grid_to_batt_ac > 0) {
            P_battery_ac = -(P_pv_to_batt_dc * maxEfficiencyDCAC + P_grid_to_batt_ac);
        }

        if (std::abs(P_battery_ac) < tolerance) {
            P_battery_ac = 0.0;
        }

        // Using the actual inverter efficiency here (as opposed to max) causes the charging power to seem low in some timesteps
        // Directing additional power to the inverter would increase the efficiency, which makes this a more reasonable assumption
        P_pv_to_batt_ac = P_pv_to_batt_dc * maxEfficiencyDCAC;
        P_battery_ac_post_loss = P_battery_ac;
    }
    else
    {
        // Can't draw from the grid during outage
        if (m_BatteryPower->isOutageStep && P_gen_dc < 0.0) {
            P_unmet_losses = std::abs(P_gen_dc);
            P_gen_dc = 0.0;
        }

        // Run this first - sharedInverter->getMaxPowerEfficiency will overwrite the values from calculateACPower
        double max_eff = m_BatteryPower->sharedInverter->getMaxPowerEfficiency();

        // convert the DC power to AC
        m_BatteryPower->sharedInverter->calculateACPower(P_gen_dc, voltage, m_BatteryPower->sharedInverter->Tdry_C);
        efficiencyDCAC = m_BatteryPower->sharedInverter->efficiencyAC * 0.01;
        P_gen_ac = m_BatteryPower->sharedInverter->powerAC_kW;

        if (m_BatteryPower->isOutageStep && P_gen_ac < 0.0) {
            P_gen_ac -= P_unmet_losses / (max_eff * 0.01);
        }

        if (pv_handles_loss) {
            P_pv_ac = (P_pv_dc - P_system_loss_dc) * efficiencyDCAC;
            P_battery_ac = P_battery_dc * efficiencyDCAC;
            P_batt_to_inverter_dc = P_battery_dc;
        }
        else { // TODO - what if the grid needs to handle idle losses?
            P_pv_ac = P_pv_dc * efficiencyDCAC;
            P_battery_ac = (P_battery_dc - P_system_loss_dc) * efficiencyDCAC;
            P_batt_to_inverter_dc = P_battery_dc - P_system_loss_dc;
        }

        if (std::abs(P_battery_ac) < tolerance) {
            P_battery_ac = 0.0;
        }

        double P_xfmr_ll = m_BatteryPower->acXfmrLoadLoss;
        double P_transformer_loss = 0;
        if (!m_BatteryPower->isOutageStep && P_gen_ac > 0) {
            P_transformer_loss = Transformer::transformerLoss(P_gen_ac * (1 - m_BatteryPower->acLossWiring), m_BatteryPower->acXfmrLoadLoss, m_BatteryPower->acXfmrRating, P_xfmr_ll, m_BatteryPower->acXfmrNoLoadLoss);
        }
        P_ac_losses = P_gen_ac - (P_gen_ac * (1 - m_BatteryPower->acLossWiring) - P_transformer_loss) * (1 - m_BatteryPower->acLossSystemAvailability);
        if (P_gen_ac > tolerance) {
            ac_loss_percent = P_ac_losses / P_gen_ac;
        }

        P_pv_ac *= (1 - ac_loss_percent);  // P_pv_ac isn't reported out directly, so we can use it
        P_battery_ac_post_loss = P_battery_ac * (1 - ac_loss_percent); // P_battery_ac is used to calculate annual charging and discharging. Apply to another variable so post batt AC losses aren't included in that value

        // Test if battery is discharging erroneously
        if (!m_BatteryPower->canDischarge && P_battery_ac > 0) {
            P_batt_to_grid_ac = P_batt_to_load_ac = 0;
            P_battery_ac = P_battery_ac_post_loss = 0;
        }

        if (m_BatteryPower->isOutageStep) {
            if (P_pv_ac >= P_crit_load_ac + P_ac_losses)
            {
                P_pv_to_load_ac = P_crit_load_ac;
                P_batt_to_load_ac = 0;
                P_batt_to_grid_ac = 0;

                // curtailed due to grid outage - see if this can be utilized for non-critcal loads in future
                P_interconnection_loss_ac = P_pv_ac - P_pv_to_load_ac - P_ac_losses;
            }
            else {
                P_pv_to_load_ac = std::fmax(0, std::fmin(P_pv_ac, P_crit_load_ac));
                P_batt_to_load_ac = std::fmax(0, std::fmin(P_battery_ac_post_loss, P_crit_load_ac - P_pv_to_load_ac));
                P_batt_to_grid_ac = P_battery_ac_post_loss - P_batt_to_load_ac; 
            }
            
            if (P_batt_to_grid_ac > tolerance) {
                // Technically the ac power should be converted back to DC here, but if you include the efficiency it overshoots
                // This way it can iterate and match the critical load exactly
                m_BatteryPower->powerBatteryDC -= P_batt_to_grid_ac;
                return calculateDCConnected();
            }
        }
        else {
            if (m_BatteryPower->dischargeOnlyLoadExceedSystem) {
                P_pv_to_load_ac = P_pv_ac;
                if (P_pv_ac >= P_load_ac)
                {
                    P_pv_to_load_ac = P_load_ac;
                    P_batt_to_load_ac = 0;

                    // discharging to grid
                    P_pv_to_grid_ac = P_pv_ac - P_pv_to_load_ac;
                }
                else {
                    P_batt_to_load_ac = std::fmin(P_battery_ac_post_loss, P_load_ac - P_pv_to_load_ac);
                }

                P_batt_to_grid_ac = std::fmax(0, P_battery_ac_post_loss - P_batt_to_load_ac);
            }
            else {
                P_batt_to_load_ac = std::fmin(P_battery_ac_post_loss, P_load_ac);
                P_pv_to_load_ac = std::fmin(std::fmax(0, P_load_ac - P_batt_to_load_ac), P_pv_ac);
                P_pv_to_grid_ac = std::fmax(0, P_pv_ac - P_pv_to_load_ac);
            }
        }
    }

    // compute losses
    P_conversion_loss_ac = P_gen_dc - P_gen_ac + P_battery_dc_pre_bms - P_battery_dc;
    
    double P_loss_coverage = 0;
    if (m_BatteryPower->isOutageStep) {
        if (P_ac_losses < 0) {
            P_ac_losses = 0; // Inverter losses are already counted in gen, don't double count below. PVSAMV1 code will do the right thing after this calc
        }
        double gen_for_ac_losses = P_gen_ac - P_batt_to_load_ac - P_pv_to_load_ac;
        // If gen is greater than load and losses, then there's curtailment and we don't need to worry about it
        if (gen_for_ac_losses > P_ac_losses) {
            P_loss_coverage = 0;
        }
        else {
            P_loss_coverage = P_ac_losses - gen_for_ac_losses;
        }
        P_crit_load_unmet_ac = P_crit_load_ac - P_pv_to_load_ac - P_batt_to_load_ac + P_loss_coverage;
        if (P_crit_load_unmet_ac > P_crit_load_ac) {
            P_unmet_losses = P_crit_load_unmet_ac - P_crit_load_ac;
            P_crit_load_unmet_ac = P_crit_load_ac;
        }
        P_grid_to_load_ac = 0;
        if (P_gen_ac < 0.0 && P_unmet_losses > 0.0) {
            P_gen_ac += P_unmet_losses; // Unmet losses should be categorized as such, not in gen
        }
        P_grid_to_load_ac = 0;
        P_grid_ac = 0;
    }
    else {
        // Compute total system output and grid power flow, inverter draw is built into P_pv_ac
        P_grid_to_load_ac = P_load_ac - P_pv_to_load_ac - P_batt_to_load_ac;

        // Grid charging loss accounted for in P_battery_ac
        P_grid_ac = P_gen_ac - P_load_ac - P_ac_losses;

        // Error checking for power to load
        if (P_pv_to_load_ac + P_grid_to_load_ac + P_batt_to_load_ac != P_load_ac)
            P_grid_to_load_ac = P_load_ac - P_pv_to_load_ac - P_batt_to_load_ac;

        if (P_grid_ac > P_grid_limit_ac) {
            double grid_diff = P_grid_ac - P_grid_limit_ac;
            // Update grid variables first
            P_grid_ac -= grid_diff;
            P_interconnection_loss_ac += grid_diff;
            if (grid_diff > P_pv_to_grid_ac) {
                // Curtail PV "first"
                grid_diff -= P_pv_to_grid_ac;
                P_pv_to_grid_ac = 0;
                // Then curtail battery
                P_batt_to_grid_ac -= grid_diff;
            }
            else {
                P_pv_to_grid_ac -= grid_diff;
            }
        }
    }

    // check tolerances
    if (std::abs(P_grid_to_load_ac) < m_BatteryPower->tolerance)
        P_grid_to_load_ac = 0;
    if (std::abs(P_grid_to_batt_ac) < m_BatteryPower->tolerance)
        P_grid_to_batt_ac = 0;
    if (std::abs(P_grid_ac) < m_BatteryPower->tolerance)
        P_grid_ac = 0;
    if (std::abs(P_crit_load_unmet_ac) < m_BatteryPower->tolerance)
        P_crit_load_unmet_ac = 0;
    if (std::abs(P_interconnection_loss_ac) < m_BatteryPower->tolerance) {
        P_interconnection_loss_ac = 0;
    }
    if (std::abs(P_batt_to_grid_ac) < m_BatteryPower->tolerance) {
        P_batt_to_grid_ac = 0;
    }

	// assign outputs
    m_BatteryPower->singlePointEfficiencyDCToAC = efficiencyDCAC;
	m_BatteryPower->powerBatteryAC = P_battery_ac; // battery DC is an input, and not adjusted here
	m_BatteryPower->powerGrid = P_grid_ac;
	m_BatteryPower->powerGeneratedBySystem = P_gen_ac;
	m_BatteryPower->powerSystemToLoad = P_pv_to_load_ac;
	m_BatteryPower->powerSystemToBatteryAC = P_pv_to_batt_ac;
    m_BatteryPower->powerSystemToBatteryDC = P_pv_to_batt_dc;
	m_BatteryPower->powerSystemToGrid = P_pv_to_grid_ac;
	m_BatteryPower->powerGridToBattery = P_grid_to_batt_ac;
	m_BatteryPower->powerGridToLoad = P_grid_to_load_ac;
	m_BatteryPower->powerBatteryToLoad = P_batt_to_load_ac;
	m_BatteryPower->powerBatteryToGrid = P_batt_to_grid_ac;
    m_BatteryPower->powerBatteryToInverterDC = P_batt_to_inverter_dc;
	m_BatteryPower->powerConversionLoss = P_conversion_loss_ac;
    m_BatteryPower->powerInterconnectionLoss = P_interconnection_loss_ac;
    m_BatteryPower->powerCritLoadUnmet = P_crit_load_unmet_ac;
    m_BatteryPower->powerLossesUnmet = P_unmet_losses;
}

