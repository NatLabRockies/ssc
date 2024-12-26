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
#include "lib_shared_inverter.h"

#include <math.h>
#include <algorithm>

/*
Dispatch base class
*/
dispatch_t::dispatch_t(battery_t* Battery, double dt_hour, double SOC_min, double SOC_max, int current_choice, double Ic_max, double Id_max,
    double Pc_max_kwdc, double Pd_max_kwdc, double Pc_max_kwac, double Pd_max_kwac,
    double t_min, int mode, int battMeterPosition, double interconnection_limit,
    bool chargeOnlySystemExceedLoad, bool dischargeOnlyLoadExceedSystem, double SOC_min_outage)
{
    // initialize battery power flow
    std::unique_ptr<BatteryPowerFlow> tmp(new BatteryPowerFlow(dt_hour));
    m_batteryPowerFlow = std::move(tmp);
    m_batteryPower = m_batteryPowerFlow->getBatteryPower();
    m_batteryPower->setMaxChargeCurrent(Ic_max);
    m_batteryPower->setMaxDischargeCurrent(Id_max);
    m_batteryPower->stateOfChargeMax = SOC_max;
    m_batteryPower->stateOfChargeMin = SOC_min;
    m_batteryPower->depthOfDischargeMax = SOC_max - SOC_min;
    m_batteryPower->setMaxDCChargePower(Pc_max_kwdc);
    m_batteryPower->setMaxDCDischargePower(Pd_max_kwdc);
    m_batteryPower->setMaxACChargePower(Pc_max_kwac);
    m_batteryPower->setMaxACDischargePower(Pd_max_kwac);
    m_batteryPower->meterPosition = battMeterPosition;
    m_batteryPower->powerInterconnectionLimit = interconnection_limit;
    m_batteryPower->chargeOnlySystemExceedLoad = chargeOnlySystemExceedLoad;
    m_batteryPower->dischargeOnlyLoadExceedSystem = dischargeOnlyLoadExceedSystem;

    // initalize Battery and a copy of the Battery for iteration
    _Battery = Battery;
    _Battery_initial = new battery_t(*_Battery);

    m_outage_manager = std::unique_ptr<outage_manager>(new outage_manager(m_batteryPower, _Battery));
    _min_outage_soc = SOC_min_outage;

    // Call the dispatch init method
    init(_Battery, dt_hour, current_choice, t_min, mode);
}

void dispatch_t::init(battery_t* Battery, double dt_hour, int current_choice, double t_min, int mode)
{
    _dt_hour = dt_hour;
    _current_choice = current_choice;
    _t_min = t_min;
    _mode = mode;

    // limit the switch from charging to discharge so that doesn't flip-flop subhourly
    _t_at_mode = 1000;
    _prev_charging = false;
    _charging = false;
    _e_max = Battery->V() * Battery->charge_maximum_lifetime() * util::watt_to_kilowatt * 0.01 * (m_batteryPower->stateOfChargeMax - m_batteryPower->stateOfChargeMin);
    _grid_recharge = false;

	// initialize powerflow model
	m_batteryPower->canClipCharge = false;
	m_batteryPower->canSystemCharge = false;
	m_batteryPower->canGridCharge = false;
	m_batteryPower->canDischarge = false;
}

// deep copy
dispatch_t::dispatch_t(const dispatch_t& dispatch)
{
    std::unique_ptr<BatteryPowerFlow> tmp(new BatteryPowerFlow(*dispatch.m_batteryPowerFlow));
    m_batteryPowerFlow = std::move(tmp);
    m_batteryPower = m_batteryPowerFlow->getBatteryPower();

    _Battery = new battery_t(*dispatch._Battery);
    _Battery_initial = new battery_t(*dispatch._Battery_initial);

    _min_outage_soc = dispatch._min_outage_soc;
    m_outage_manager = std::unique_ptr<outage_manager>(new outage_manager(m_batteryPower, _Battery));
    m_outage_manager->copy(*(dispatch.m_outage_manager));
    init(_Battery, dispatch._dt_hour, dispatch._current_choice, dispatch._t_min, dispatch._mode);
}

// shallow copy from dispatch to this
void dispatch_t::copy(const dispatch_t* dispatch)
{
    _Battery->set_state(dispatch->_Battery->get_state());
    _Battery_initial->set_state(dispatch->_Battery_initial->get_state());
    init(_Battery, dispatch->_dt_hour, dispatch->_current_choice, dispatch->_t_min, dispatch->_mode);

    // can't create shallow copy of unique ptr
    std::unique_ptr<BatteryPowerFlow> tmp(new BatteryPowerFlow(*dispatch->m_batteryPowerFlow));
    m_batteryPowerFlow = std::move(tmp);
    m_batteryPower = m_batteryPowerFlow->getBatteryPower();
    _min_outage_soc = dispatch->_min_outage_soc;
    m_outage_manager = std::unique_ptr<outage_manager>(new outage_manager(m_batteryPower, _Battery));
    m_outage_manager->copy(*(dispatch->m_outage_manager));

}
void dispatch_t::delete_clone()
{
    // need to delete both, since allocated memory for both in deep copy
    if (_Battery) {
        delete _Battery;
    }
    if (_Battery_initial) {
        delete _Battery_initial;
        _Battery_initial = nullptr;
    }
}
dispatch_t::~dispatch_t()
{
    // original _Battery doesn't need deleted, since was a pointer passed in
    delete _Battery_initial;
}
void dispatch_t::finalize(size_t idx, double& I)
{
	_Battery->set_state(_Battery_initial->get_state());
	m_batteryPower->powerBatteryDC = 0;
	m_batteryPower->powerBatteryAC = 0;
	m_batteryPower->powerGridToBattery = 0;
	m_batteryPower->powerBatteryToGrid = 0;
	m_batteryPower->powerSystemToGrid = 0;
	_Battery->run(idx, I);
}

bool dispatch_t::check_constraints(double& I, size_t count)
{
    bool iterate;
    double I_initial = I;
    bool current_iterate = false;
    bool power_iterate = false;

    // don't allow any changes to violate current limits
    if (restrict_current(I))
    {
        current_iterate = true;
    }
    // don't allow violations of power limits
    else if (restrict_power(I))
    {
        power_iterate = true;
    }
    // decrease the current draw if took too much
    if (I > 0 && _Battery->SOC() < m_batteryPower->stateOfChargeMin - tolerance)
    {
        m_batteryPower->powerBatteryTarget = _Battery_initial->calculate_max_discharge_kw(&I);
    }
    // decrease the current charging if charged too much
    else if (I < 0 && _Battery->SOC() > m_batteryPower->stateOfChargeMax + tolerance)
    {
        m_batteryPower->powerBatteryTarget = _Battery_initial->calculate_max_charge_kw(&I);
    }
    // Don't allow grid charging unless explicitly allowed (reduce charging)
    if (!m_batteryPower->canGridCharge && I < 0 && m_batteryPower->powerGridToBattery > powerflow_tolerance)
    {
        m_batteryPower->powerBatteryTarget += m_batteryPower->powerGridToBattery * m_batteryPower->singlePointEfficiencyACToDC;
        I = _Battery->calculate_current_for_power_kw(m_batteryPower->powerBatteryTarget);
        m_batteryPower->powerGridToBattery = 0;
	}
	// Don't allow grid charging if producing PV
	else if (m_batteryPower->connectionMode == dispatch_t::DC_CONNECTED &&
		m_batteryPower->powerGridToBattery > 0 &&
		(m_batteryPower->powerSystemToGrid > 0 || m_batteryPower->powerSystemToLoad > 0))
	{
        m_batteryPower->powerBatteryTarget += m_batteryPower->powerGridToBattery * m_batteryPower->singlePointEfficiencyACToDC;
        I = _Battery->calculate_current_for_power_kw(m_batteryPower->powerBatteryTarget);
    }
    // Error checking for battery charging
    double power_to_batt = m_batteryPower->powerBatteryDC;
	if (m_batteryPower->connectionMode == dispatch_t::DC_CONNECTED){
	    power_to_batt = -(m_batteryPower->powerSystemToBatteryDC + m_batteryPower->powerFuelCellToBattery * m_batteryPower->singlePointEfficiencyACToDC); // System to batt in DC, convert fuel cell
	    if (m_batteryPower->sharedInverter->powerDC_kW < 0)
	        power_to_batt += m_batteryPower->sharedInverter->powerDC_kW;    // charging from grid
	    power_to_batt *= m_batteryPower->singlePointEfficiencyDCToDC;
	    // if error is from from numerical solution, may not need to adjust battery
	}
	else {
	    power_to_batt = -(m_batteryPower->powerGridToBattery + m_batteryPower->powerFuelCellToBattery); // AC components
	    power_to_batt *= m_batteryPower->singlePointEfficiencyACToDC;
        power_to_batt -= m_batteryPower->powerSystemToBatteryDC;
    }

    if (m_batteryPower->powerBatteryTarget < 0 && std::abs(power_to_batt - m_batteryPower->powerBatteryTarget) > 0.005 * std::abs(power_to_batt)) {
        m_batteryPower->powerBatteryTarget = power_to_batt;
        m_batteryPower->powerBatteryDC = m_batteryPower->powerBatteryTarget;
        I = _Battery_initial->calculate_current_for_power_kw(m_batteryPower->powerBatteryTarget);
    }

    // Don't allow battery to discharge if it gets wasted due to inverter efficiency limitations
// Typically, this would be due to low power flow, so just cut off battery.
    if (m_batteryPower->connectionMode == dispatch_t::DC_CONNECTED && m_batteryPower->sharedInverter->efficiencyAC < m_batteryPower->inverterEfficiencyCutoff)
    {
        // The requested DC power
        double powerBatterykWdc = _Battery->I() * _Battery->V() * util::watt_to_kilowatt;

        // if battery discharging, see if can back off to get higher efficiency
        if (m_batteryPower->powerBatteryDC > 0) {
            double max_dc = m_batteryPower->powerSystem + powerBatterykWdc; // Only used by "inverter::NONE"
            double inverter_max_dc = m_batteryPower->sharedInverter->getInverterDCMaxPower(max_dc) * util::watt_to_kilowatt * (1 - m_batteryPower->acLossSystemAvailability);
            if (powerBatterykWdc + m_batteryPower->powerSystem > inverter_max_dc) {
                powerBatterykWdc = inverter_max_dc - m_batteryPower->powerSystem;
                powerBatterykWdc = fmax(powerBatterykWdc, 0);
                m_batteryPower->powerBatteryTarget = powerBatterykWdc;
                I = _Battery->calculate_current_for_power_kw(m_batteryPower->powerBatteryTarget);
            }
        }
        // if charging, this will also be due to low powerflow from grid-charging, just cut off that component
        else if (m_batteryPower->powerBatteryDC < 0 && m_batteryPower->powerGridToBattery > 0) {
            I *= fmax(1.0 - std::abs(m_batteryPower->powerGridToBattery * m_batteryPower->sharedInverter->efficiencyAC * 0.01 / m_batteryPower->powerBatteryDC), 0);
            m_batteryPower->powerBatteryTarget = _Battery->calculate_voltage_for_current(I) * I * util::watt_to_kilowatt;
        }     
    }

    iterate = std::abs(I_initial - I) > tolerance;

    // update constraints for current, power, if they are now violated
    if (!current_iterate) {
        current_iterate = restrict_current(I);
    }
    if (!power_iterate) {
        power_iterate = restrict_power(I);
    }

    // iterate if any of the conditions are met
    if (iterate || current_iterate || power_iterate)
        iterate = true;

    // stop iterating after n tries
    if (count > battery_dispatch::constraintCount)
        iterate = false;

    // don't allow battery to flip from charging to discharging or vice versa
    if (std::abs(I) > tolerance && (I_initial / I) < 0) {
        I = 0;
        iterate = false;
    }


    // reset
    if (iterate)
    {
        _Battery->set_state(_Battery_initial->get_state());
        m_batteryPowerFlow->calculate();
    }

    return iterate;
}
void dispatch_t::SOC_controller()
{
    _charging = _prev_charging;

    // Implement minimum SOC cut-off
    if (m_batteryPower->powerBatteryDC > 0)
    {
        if (_Battery->SOC() <= m_batteryPower->stateOfChargeMin + tolerance) {
            m_batteryPower->powerBatteryDC = 0;
        }
        else {
            _charging = false;
        }
    }
    // Maximum SOC cut-off
    else if (m_batteryPower->powerBatteryDC < 0)
    {
        if (_Battery->SOC() >= m_batteryPower->stateOfChargeMax - tolerance) {
            m_batteryPower->powerBatteryDC = 0;
        }
        else {
            _charging = true;
        }
    }
}

void dispatch_t::switch_controller()
{
    // Implement rapid switching check
    if (_charging != _prev_charging)
    {
        if (_t_at_mode <= _t_min)
        {
            m_batteryPower->powerBatteryDC = 0.;
            _charging = _prev_charging;
        }
        else
            _t_at_mode = 0;
    }
    _t_at_mode += (int)(round(_dt_hour * util::hour_to_min));
}
double dispatch_t::current_controller(double power_kw)
{
    double I = _Battery->calculate_current_for_power_kw(power_kw);
    restrict_current(I);
    return I;
}
bool dispatch_t::restrict_current(double& I)
{
    bool iterate = false;
    if (_current_choice == RESTRICT_CURRENT || _current_choice == RESTRICT_BOTH)
    {
        if (I < 0)
        {
            double max_current_charge = m_batteryPower->getMaxChargeCurrent();
            if (std::abs(I) > max_current_charge)
            {
                I = -max_current_charge;
                iterate = true;
            }
        }
        else
        {
            double max_current_discharge = m_batteryPower->getMaxDischargeCurrent();
            if (I > max_current_discharge)
            {
                I = max_current_discharge;
                iterate = true;
            }
        }
    }
    return iterate;
}
bool dispatch_t::restrict_power(double& I)
{
    bool iterate = false;
    if (_current_choice == RESTRICT_POWER || _current_choice == RESTRICT_BOTH)
    {
        double powerBattery = I * _Battery->V() * util::watt_to_kilowatt;
        double powerBatteryAC = powerBattery;
        if (powerBattery < 0)
            powerBatteryAC = powerBattery / m_batteryPower->singlePointEfficiencyACToDC;
        else if (powerBattery > 0)
            powerBatteryAC = powerBattery * m_batteryPower->singlePointEfficiencyDCToAC;

        double dP = 0.;

        // charging
        if (powerBattery < 0)
        {
            double max_charge_dc = m_batteryPower->getMaxDCChargePower();
            double max_charge_ac = m_batteryPower->getMaxACChargePower();
            if (std::abs(powerBattery) > max_charge_dc * (1 + low_tolerance))
            {
                dP = std::abs(max_charge_dc - std::abs(powerBattery));

                // increase (reduce) charging magnitude by percentage
                I -= (dP / std::abs(powerBattery)) * I;
                iterate = true;
            }
            else if (m_batteryPower->connectionMode == m_batteryPower->AC_CONNECTED &&
                std::fabs(powerBatteryAC) > max_charge_ac * (1 + low_tolerance))
            {
                dP = std::abs(max_charge_ac - std::abs(powerBatteryAC));

                // increase (reduce) charging magnitude by percentage
                I -= (dP / std::abs(powerBattery)) * I;
                iterate = true;
            }
            // This could just be grid power since that's technically the only AC component.  But, limit all to this
            else if (m_batteryPower->connectionMode == m_batteryPower->DC_CONNECTED &&
                std::abs(powerBatteryAC) > max_charge_ac * (1 + low_tolerance))
            {
                dP = std::abs(max_charge_ac - std::abs(powerBatteryAC));

                // increase (reduce) charging magnitude by percentage
                I -= (dP / std::abs(powerBattery)) * I;
                iterate = true;
            }
        }
        else
        {
            double max_discharge_dc = m_batteryPower->getMaxDCDischargePower();
            double max_discharge_ac = m_batteryPower->getMaxACDischargePower();
            
            if (std::abs(powerBattery) > max_discharge_dc * (1 + low_tolerance))
            {
                dP = std::abs(max_discharge_dc - powerBattery);

                // decrease discharging magnitude
                I -= (dP / std::abs(powerBattery)) * I;
                iterate = true;
            }
            else if (std::abs(powerBatteryAC) > max_discharge_ac * (1 + low_tolerance))
            {
                dP = std::abs(max_discharge_ac - powerBatteryAC);

                // decrease discharging magnitude
                I -= (dP / std::abs(powerBattery)) * I;
                iterate = true;
            }
        }
    }
    return iterate;
}

void dispatch_t::runDispatch(size_t lifetimeIndex)
{
    // Ensure the battery operates within the state-of-charge limits
    SOC_controller();

    // Ensure the battery isn't switching rapidly between charging and dischaging
    switch_controller();

    // Calculate current, and ensure the battery falls within the current limits
    double I = current_controller(m_batteryPower->powerBatteryDC);

    // Setup battery iteration
    _Battery_initial->set_state(_Battery->get_state());

    bool iterate = true;
    size_t count = 0;

    do {

        // Run Battery Model to update charge based on charge/discharge
        m_batteryPower->powerBatteryDC = _Battery->run(lifetimeIndex, I);
        m_batteryPower->powerSystemLoss = _Battery->getAncillaryLoss();

        // Update power flow calculations, calculate AC power, and check the constraints
        m_batteryPowerFlow->calculate();
        iterate = check_constraints(I, count);

        // If current changed during last iteration of constraints checker, recalculate internal battery state
        if (!iterate) {
            finalize(lifetimeIndex, I);
            m_batteryPower->powerBatteryDC = I * _Battery->V() * util::watt_to_kilowatt;
        }
        else {
            _Battery->set_state(_Battery_initial->get_state());
        }
        count++;

    } while (iterate);

    // finalize AC power flow calculation and update for next step
    m_batteryPowerFlow->calculate();
    _prev_charging = _charging;
}

void dispatch_t::run_outage_step(size_t lifetimeIndex) {
    if (m_batteryPower->connectionMode == DC_CONNECTED) {
        dispatch_dc_outage_step(lifetimeIndex);
    }
    else {
        dispatch_ac_outage_step(lifetimeIndex);
    }
}

void dispatch_t::dispatch_dc_outage_step(size_t lifetimeIndex) {

    double dc_dc_eff = m_batteryPower->singlePointEfficiencyDCToDC;
    double pv_kwdc = m_batteryPower->powerSystem;
    double V_pv = m_batteryPower->voltageSystem;
    double pv_clipped = m_batteryPower->powerSystemClipped;
    double crit_load_kwac = m_batteryPower->powerCritLoad;
    double ac_loss_percent = 1 - (1 - m_batteryPower->acLossWiring) * (1 - m_batteryPower->acLossSystemAvailability);

    m_batteryPower->sharedInverter->calculateACPower(pv_kwdc, V_pv, m_batteryPower->sharedInverter->Tdry_C);
    double dc_ac_eff = m_batteryPower->sharedInverter->efficiencyAC * 0.01;
    double pv_kwac = m_batteryPower->sharedInverter->powerAC_kW;

    double max_discharge_kwdc = _Battery->calculate_max_discharge_kw();
    max_discharge_kwdc = std::fmin(max_discharge_kwdc, m_batteryPower->getMaxDCDischargePower());
    double max_charge_kwdc = _Battery->calculate_max_charge_kw();
    max_charge_kwdc = std::fmax(max_charge_kwdc, -1.0 * m_batteryPower->getMaxDCChargePower()); // Max, since charging numbers are negative
    double batt_losses = _Battery->calculate_loss(max_charge_kwdc, lifetimeIndex);

    // Setup battery iteration
    auto Battery_initial = _Battery->get_state();

    if ((pv_kwac - batt_losses) * (1 - ac_loss_percent) > crit_load_kwac) {
        double remaining_kwdc = -(pv_kwac * (1 - ac_loss_percent) - crit_load_kwac) / dc_ac_eff + pv_clipped;
        remaining_kwdc = fmax((remaining_kwdc + batt_losses), max_charge_kwdc);
        m_batteryPower->powerBatteryTarget = remaining_kwdc;
        m_batteryPower->powerBatteryDC = remaining_kwdc;
        runDispatch(lifetimeIndex);
        double dc_input = pv_kwdc + remaining_kwdc;
        double est_crit_load_unmet = m_batteryPower->powerCritLoadUnmet;
        while (m_batteryPower->powerCritLoadUnmet > powerflow_tolerance) {
            _Battery->set_state(Battery_initial);
            dc_input = pv_kwdc + remaining_kwdc + (m_batteryPower->powerCritLoadUnmet) / dc_ac_eff;
            // remaining_kw_dc is a negative number, so add it to pv_kwdc to reduce inverter dc power
            m_batteryPower->sharedInverter->calculateACPower(dc_input, V_pv, m_batteryPower->sharedInverter->Tdry_C);
            dc_ac_eff = m_batteryPower->sharedInverter->efficiencyAC * 0.01;
            pv_kwac = m_batteryPower->sharedInverter->powerAC_kW; // Re-estimate AC output based on new input power
            est_crit_load_unmet = fmax(m_batteryPower->powerCritLoad - pv_kwac * (1 - ac_loss_percent), 0.0);
            remaining_kwdc = (dc_input - pv_kwdc) + (est_crit_load_unmet) / dc_ac_eff + pv_clipped; // Reduce remaining_kwdc by any errors between crit load and pv output
            remaining_kwdc = fmax((remaining_kwdc + batt_losses), max_charge_kwdc);
            remaining_kwdc = fmin(remaining_kwdc, 0.0);
            m_batteryPower->powerBatteryTarget = remaining_kwdc;
            m_batteryPower->powerBatteryDC = remaining_kwdc;
            runDispatch(lifetimeIndex);
        }
    }
    else {
        // find dc power required from pv + battery discharge to meet load, then get just the power required from battery
        double required_kwdc = (m_batteryPower->sharedInverter->calculateRequiredDCPower(crit_load_kwac * (1 + ac_loss_percent), V_pv, m_batteryPower->sharedInverter->Tdry_C) - pv_kwdc) / dc_dc_eff;
        required_kwdc = required_kwdc < powerflow_tolerance ? powerflow_tolerance : required_kwdc; // Cover for the fact that the loss percent can occasionally drive the above number negative

        if (required_kwdc < max_discharge_kwdc) {
            batt_losses = _Battery->calculate_loss(required_kwdc, lifetimeIndex);
            required_kwdc = fmin(required_kwdc + batt_losses, max_discharge_kwdc);
            double discharge_kwdc = required_kwdc;

            m_batteryPower->powerBatteryTarget = discharge_kwdc;
            m_batteryPower->powerBatteryDC = discharge_kwdc;
            runDispatch(lifetimeIndex);
            if (m_batteryPower->powerCritLoadUnmet > powerflow_tolerance) {
                while (discharge_kwdc < max_discharge_kwdc) {
                    if (m_batteryPower->powerCritLoadUnmet < powerflow_tolerance)
                        break;
                    discharge_kwdc *= 1.01;
                    _Battery->set_state(Battery_initial);
                    m_batteryPower->powerBatteryTarget = discharge_kwdc;
                    m_batteryPower->powerBatteryDC = discharge_kwdc;
                    runDispatch(lifetimeIndex);
                }
            }
        }
        else {
            m_batteryPower->powerBatteryTarget = max_discharge_kwdc;
            m_batteryPower->powerBatteryDC = max_discharge_kwdc;
            runDispatch(lifetimeIndex);
        }
    }
}

void dispatch_t::dispatch_ac_outage_step(size_t lifetimeIndex) {
    double crit_load_kwac = m_batteryPower->powerCritLoad;
    double pv_kwac = m_batteryPower->powerSystem;
    double fuel_cell_kwac = m_batteryPower->powerFuelCell;
    double ac_loss_percent = m_batteryPower->acLossSystemAvailability;

    double max_discharge_kwdc = _Battery->calculate_max_discharge_kw();
    max_discharge_kwdc = std::fmin(max_discharge_kwdc, m_batteryPower->getMaxDCDischargePower());
    double max_discharge_kwac = max_discharge_kwdc * m_batteryPower->singlePointEfficiencyDCToDC;
    max_discharge_kwac = std::fmin(max_discharge_kwac, m_batteryPower->getMaxACDischargePower());
    double max_charge_kwdc = _Battery->calculate_max_charge_kw();
    max_charge_kwdc = std::fmax(max_charge_kwdc, -1.0 * m_batteryPower->getMaxDCChargePower()); // Max, since charging numbers are negative

    if ((pv_kwac + fuel_cell_kwac) * (1 - ac_loss_percent) > crit_load_kwac) {
        double remaining_kwdc = -((pv_kwac + fuel_cell_kwac) * (1 - ac_loss_percent) - crit_load_kwac) * m_batteryPower->singlePointEfficiencyACToDC;
        remaining_kwdc = fmax(remaining_kwdc, max_charge_kwdc);
        m_batteryPower->powerBatteryTarget = remaining_kwdc;
        m_batteryPower->powerBatteryDC = remaining_kwdc;
        runDispatch(lifetimeIndex);
    }
    else {
        double max_to_load_kwac = (max_discharge_kwac + pv_kwac + fuel_cell_kwac) * (1 - ac_loss_percent);
        double required_kwdc = (crit_load_kwac - (pv_kwac + fuel_cell_kwac) * (1 - ac_loss_percent)) / m_batteryPower->singlePointEfficiencyDCToAC;
        required_kwdc = fmin(required_kwdc, max_discharge_kwdc);

        if (max_to_load_kwac > crit_load_kwac) {
            double discharge_kwdc = required_kwdc;

            // iterate in case the dispatched power is slightly less (by tolerance) than required
            auto Battery_initial = _Battery->get_state();
            m_batteryPower->powerBatteryTarget = discharge_kwdc;
            m_batteryPower->powerBatteryDC = discharge_kwdc;
            runDispatch(lifetimeIndex);
            if (m_batteryPower->powerCritLoadUnmet > powerflow_tolerance) {
                while (discharge_kwdc < max_discharge_kwdc) {
                    if (m_batteryPower->powerCritLoadUnmet < powerflow_tolerance)
                        break;
                    discharge_kwdc *= 1.01;
                    _Battery->set_state(Battery_initial);
                    m_batteryPower->powerBatteryTarget = discharge_kwdc;
                    m_batteryPower->powerBatteryDC = discharge_kwdc;
                    runDispatch(lifetimeIndex);
                }
            }
        }
        else {
            m_batteryPower->powerBatteryTarget = max_discharge_kwdc;
            m_batteryPower->powerBatteryDC = max_discharge_kwdc;
            runDispatch(lifetimeIndex);
        }
    }
}

double dispatch_t::power_tofrom_battery_ac() { return m_batteryPower->powerBatteryAC; }
double dispatch_t::power_tofrom_battery_dc() { return m_batteryPower->powerBatteryDC; }
double dispatch_t::power_tofrom_grid() { return m_batteryPower->powerGrid; }
double dispatch_t::power_gen() { return m_batteryPower->powerGeneratedBySystem; }
double dispatch_t::power_pv_to_load() { return m_batteryPower->powerSystemToLoad; }
double dispatch_t::power_battery_to_load() { return m_batteryPower->powerBatteryToLoad; }
double dispatch_t::power_grid_to_load() { return m_batteryPower->powerGridToLoad; }
double dispatch_t::power_fuelcell_to_load() { return m_batteryPower->powerFuelCellToLoad; }
double dispatch_t::power_pv_to_batt_ac() { return m_batteryPower->powerSystemToBatteryAC; }
double dispatch_t::power_pv_to_batt_dc() { return m_batteryPower->powerSystemToBatteryDC; }
double dispatch_t::power_grid_to_batt() { return m_batteryPower->powerGridToBattery; }
double dispatch_t::power_fuelcell_to_batt() { return m_batteryPower->powerFuelCellToBattery; }
double dispatch_t::power_pv_to_grid() { return m_batteryPower->powerSystemToGrid; }
double dispatch_t::power_battery_to_grid() { return m_batteryPower->powerBatteryToGrid; }
double dispatch_t::power_battery_to_system_load() { return m_batteryPower->powerBatteryToSystemLoad; }
double dispatch_t::power_battery_to_inverter_dc() { return m_batteryPower->powerBatteryToInverterDC; }
double dispatch_t::power_fuelcell_to_grid() { return m_batteryPower->powerFuelCellToGrid; }
double dispatch_t::power_conversion_loss() { return m_batteryPower->powerConversionLoss; }
double dispatch_t::power_system_loss() { return m_batteryPower->powerSystemLoss; }
double dispatch_t::power_interconnection_loss() { return m_batteryPower->powerInterconnectionLoss; }
double dispatch_t::power_crit_load_unmet() { return m_batteryPower->powerCritLoadUnmet; }
double dispatch_t::power_crit_load() { return m_batteryPower->powerCritLoad; }
double dispatch_t::power_losses_unmet() { return m_batteryPower->powerLossesUnmet; }
double dispatch_t::battery_power_to_fill() { return _Battery->power_to_fill(m_batteryPower->stateOfChargeMax); }
double dispatch_t::battery_soc() { return _Battery->SOC(); }
BatteryPowerFlow * dispatch_t::getBatteryPowerFlow() { return m_batteryPowerFlow.get(); }
BatteryPower * dispatch_t::getBatteryPower() { return m_batteryPower; }

dispatch_automatic_t::dispatch_automatic_t(
	battery_t * Battery,
	double dt_hour,
	double SOC_min,
	double SOC_max,
	int current_choice,
	double Ic_max,
	double Id_max,
	double Pc_max_kwdc,
	double Pd_max_kwdc,
	double Pc_max_kwac,
	double Pd_max_kwac,
	double t_min,
	int dispatch_mode,
    int weather_forecast_mode,
	int pv_dispatch,
	size_t nyears,
	size_t look_ahead_hours,
	double dispatch_update_frequency_hours,
	bool can_charge,
	bool can_clip_charge,
	bool can_grid_charge,
	bool can_fuelcell_charge,
    bool can_curtail_charge,
    std::vector<double> battReplacementCostPerkWh,
    int battCycleCostChoice,
    std::vector<double> battCycleCost,
    std::vector<double> battOMCost,
    double interconnection_limit,
    bool chargeOnlySystemExceedLoad,
    bool dischargeOnlyLoadExceedSystem,
    bool behindTheMeterDischargeToGrid,
    double SOC_min_outage    
	) : dispatch_t(Battery, dt_hour, SOC_min, SOC_max, current_choice, Ic_max, Id_max, Pc_max_kwdc, Pd_max_kwdc, Pc_max_kwac, Pd_max_kwac,

    t_min, dispatch_mode, pv_dispatch, interconnection_limit, chargeOnlySystemExceedLoad, dischargeOnlyLoadExceedSystem, SOC_min_outage)
{

    _dt_hour = dt_hour;
    _dt_hour_update = dispatch_update_frequency_hours;

    _hour_last_updated = SIZE_MAX;

    _forecast_hours = look_ahead_hours;
    _steps_per_hour = (size_t)(1. / dt_hour);
    _num_steps = 24 * _steps_per_hour;

	_day_index = 0;
	_month = 1;
	_nyears = nyears;
    curr_year = 0;

    _mode = dispatch_mode;
    _weather_forecast_mode = weather_forecast_mode;
    _safety_factor = 0.0;

	m_batteryPower->canClipCharge = can_clip_charge;
    m_batteryPower->canCurtailCharge = can_curtail_charge;
	m_batteryPower->canSystemCharge = can_charge;
	m_batteryPower->canGridCharge = can_grid_charge;
	m_batteryPower->canFuelCellCharge = can_fuelcell_charge;
	m_batteryPower->canDischarge = true;
    m_batteryPower->canDischargeToGrid = behindTheMeterDischargeToGrid;
    m_battReplacementCostPerKWH = battReplacementCostPerkWh;
    m_battCycleCostChoice = battCycleCostChoice;
    cycle_costs_by_year = battCycleCost;
    om_costs_by_year = battOMCost;
}

void dispatch_automatic_t::init_with_pointer(const dispatch_automatic_t* tmp)
{
	_day_index = tmp->_day_index;
	_month = tmp->_month;
	_num_steps = tmp->_num_steps;
	_hour_last_updated = tmp->_hour_last_updated;
	_dt_hour = tmp->_dt_hour;
	_dt_hour_update = tmp->_dt_hour_update;
	_steps_per_hour = tmp->_steps_per_hour;
	_nyears = tmp->_nyears;
    curr_year = tmp->curr_year;
	_mode = tmp->_mode;
    _weather_forecast_mode = tmp->_weather_forecast_mode;
	_safety_factor = tmp->_safety_factor;
	_forecast_hours = tmp->_forecast_hours;
    m_battReplacementCostPerKWH = tmp->m_battReplacementCostPerKWH;
    m_battCycleCostChoice = tmp->m_battCycleCostChoice;
    m_cycleCost = tmp->m_cycleCost;
    cycle_costs_by_year = tmp->cycle_costs_by_year;
    om_costs_by_year = tmp->om_costs_by_year;
}

// deep copy from dispatch to this
dispatch_automatic_t::dispatch_automatic_t(const dispatch_t& dispatch) :
    dispatch_t(dispatch)
{
    const dispatch_automatic_t* tmp = dynamic_cast<const dispatch_automatic_t*>(&dispatch);
    init_with_pointer(tmp);
}

// shallow copy from dispatch to this
void dispatch_automatic_t::copy(const dispatch_t* dispatch)
{
    dispatch_t::copy(dispatch);
    const dispatch_automatic_t* tmp = dynamic_cast<const dispatch_automatic_t*>(dispatch);
    init_with_pointer(tmp);
}

void dispatch_automatic_t::update_pv_data(std::vector<double> P_pv_ac){ _P_pv_ac = P_pv_ac;}

void dispatch_automatic_t::update_cliploss_data(double_vec P_cliploss)
{
    _P_cliploss_dc = P_cliploss;

    // append to end to allow for look-ahead
    for (size_t i = 0; i != _forecast_hours * _steps_per_hour; i++)
        _P_cliploss_dc.push_back(P_cliploss[i]);
}
void dispatch_automatic_t::set_custom_dispatch(std::vector<double> P_batt_dc) { _P_battery_use = P_batt_dc; }
int dispatch_automatic_t::get_mode() { return _mode; }
double dispatch_automatic_t::power_batt_target() { return m_batteryPower->powerBatteryTarget; }

void dispatch_automatic_t::dispatch(size_t year,
    size_t hour_of_year,
    size_t step)
{
    size_t lifetimeIndex = util::lifetimeIndex(year, hour_of_year, step, static_cast<size_t>(1 / _dt_hour));
    runDispatch(lifetimeIndex);
}


bool dispatch_automatic_t::check_constraints(double& I, size_t count)
{
    // check common constraints before checking automatic dispatch specific ones
    bool iterate = dispatch_t::check_constraints(I, count);

    if (!iterate)
    {
        double I_initial = I;
        double P_battery = I * _Battery->V() * util::watt_to_kilowatt;
        double P_target = m_batteryPower->powerBatteryTarget;
        double charge_max_dc = m_batteryPower->getMaxDCChargePower();
        double charge_max_ac = m_batteryPower->getMaxACChargePower();
        double discharge_max_dc = m_batteryPower->getMaxDCDischargePower();
        double discharge_max_ac = m_batteryPower->getMaxACDischargePower();

        // Common to automated behind the meter and front of meter
        iterate = true;


		// Don't respect target if bidirectional inverter efficiency is low while charging
		if (m_batteryPower->connectionMode == dispatch_t::DC_CONNECTED &&
			m_batteryPower->sharedInverter->efficiencyAC <= m_batteryPower->inverterEfficiencyCutoff &&
			P_target < 0)
		{
			iterate = false;
			// Power adjustments were handled in dispatch_t::check_constraints, don't iterate, move along
		}

        // Try and force controller to meet target or custom dispatch
        else if (P_battery > P_target + powerflow_tolerance || P_battery < P_target - powerflow_tolerance)
        {
            // Difference between the dispatch and the desired dispatch
            double dP = P_battery - m_batteryPower->powerBatteryTarget;
            double SOC = _Battery->SOC();

            // Case 1: Charging, need to increase charging to meet target (P_battery < 0, dP > 0)
            if (P_battery <= 0 && dP > 0) {
                // Don't charge more if can't grid charge (assumes PV and fuel cell have met max possible)
                if (!m_batteryPower->canGridCharge) {
                    iterate = false;
                }
                // Don't charge more if battery is already close to full
                if (SOC > m_batteryPower->stateOfChargeMax - tolerance) {
                    iterate = false;
                }
                // Don't charge more if would violate current or power charge limits
                if (I > m_batteryPower->getMaxChargeCurrent() - tolerance ||
                    std::abs(P_battery) > charge_max_dc - powerflow_tolerance ||
                    std::abs(m_batteryPower->powerBatteryAC) > charge_max_ac - powerflow_tolerance) {
                    iterate = false;
                }
                // restrict based on power limits
                else {
                    double dP_max = fmin(fmin(dP, charge_max_dc - std::abs(P_battery)),
                        charge_max_ac - std::abs(m_batteryPower->powerBatteryAC));
                    dP = fmax(dP_max, 0);
                }
            }
            // Case 2: Discharging, need to increase discharge to meet target (P_battery > 0, dP < 0)
            else if (P_battery > 0 && dP < 0) {
                // Don't discharge more if already near min SOC
                if (SOC < m_batteryPower->stateOfChargeMin + tolerance) {
                    iterate = false;
                }
                // Don't discharge more if would violate current or power discharge limits
                if (I > m_batteryPower->getMaxDischargeCurrent() - tolerance ||
                    P_battery > discharge_max_dc - powerflow_tolerance ||
                    m_batteryPower->powerBatteryAC > discharge_max_ac - powerflow_tolerance) {
                    iterate = false;
                }
                // restrict based on power limits
                else {
                    double dP_max = fmax(fmax(dP, P_battery - discharge_max_dc),
                        m_batteryPower->powerBatteryAC - discharge_max_ac);
                    dP = fmin(dP_max, 0);
                }
            }
            // Otherwise safe, (decreasing charging, decreasing discharging)
            double dQ = dP * _dt_hour * util::kilowatt_to_watt / _Battery->V();
            double dSOC = 100 * dQ / _Battery->charge_maximum_lifetime();

            if (iterate) {

				double dI = dP * util::kilowatt_to_watt / _Battery->V();
				if (SOC + dSOC > m_batteryPower->stateOfChargeMax + tolerance) {
					double dSOC_use = (m_batteryPower->stateOfChargeMax - SOC);
					double dQ_use = dSOC_use * 0.01 * _Battery->charge_maximum_lifetime();
					dI = dQ_use / _dt_hour;
				}
				else if (SOC + dSOC < m_batteryPower->stateOfChargeMin - tolerance) {
					double dSOC_use = (m_batteryPower->stateOfChargeMin - SOC);
					double dQ_use = dSOC_use * 0.01 * _Battery->charge_maximum_lifetime();
					dI = dQ_use / _dt_hour;
				}
				I -= dI;
			}
		}

		// Behind the meter
		if (m_batteryPower->meterPosition == dispatch_t::BEHIND)
		{
			// Don't let PV export to grid if can still charge battery (increase charging) (unless following custom dispatch)
			if (_mode != dispatch_t::CUSTOM_DISPATCH && m_batteryPower->powerSystemToGrid  > powerflow_tolerance && m_batteryPower->canSystemCharge &&
                    _Battery->SOC() < m_batteryPower->stateOfChargeMax - tolerance && std::abs(I) < std::abs(m_batteryPower->getMaxChargeCurrent()))
			{
				if (std::abs(m_batteryPower->powerBatteryAC) < powerflow_tolerance)
					I -= (m_batteryPower->powerSystemToGrid / m_batteryPower->singlePointEfficiencyDCToAC * util::kilowatt_to_watt / _Battery->V());
				else
					I -= (m_batteryPower->powerSystemToGrid  / std::abs(m_batteryPower->powerBatteryAC)) * std::abs(I);
			}
			// Don't let battery export to the grid if behind the meter
			else if (m_batteryPower->powerBatteryToGrid > powerflow_tolerance && !m_batteryPower->canDischargeToGrid)
			{
                if (std::abs(m_batteryPower->powerBatteryAC) < powerflow_tolerance) {
                    I -= (m_batteryPower->powerBatteryToGrid / m_batteryPower->singlePointEfficiencyDCToAC * util::kilowatt_to_watt / _Battery->V());
                }
                else {
                    I -= (m_batteryPower->powerBatteryToGrid / std::abs(m_batteryPower->powerBatteryAC)) * std::abs(I);
                }
                m_batteryPower->powerBatteryTarget -= m_batteryPower->powerBatteryToGrid / m_batteryPower->singlePointEfficiencyDCToAC;
                m_batteryPower->powerBatteryAC -= m_batteryPower->powerBatteryToGrid; // Target was too large given PV, reduce
			}
			else
				iterate = std::abs(I_initial - I) > tolerance;;
		}
		else
			iterate = std::abs(I_initial - I) > tolerance;;

        // don't allow any changes to violate current limits
        bool current_iterate = restrict_current(I);

        // don't allow any changes to violate power limites
        bool power_iterate = restrict_power(I);

        // iterate if any of the conditions are met
        if (iterate || current_iterate || power_iterate)
            iterate = true;

        // stop iterating after n tries
        if (count > battery_dispatch::constraintCount)
            iterate = false;

        // don't allow battery to flip from charging to discharging or vice versa
        if ((I_initial / I) < 0)
            I = 0;

        // reset
        if (iterate)
        {
            _Battery->set_state(_Battery_initial->get_state());
            //			m_batteryPower->powerBatteryAC = 0;
            //			m_batteryPower->powerGridToBattery = 0;
            //			m_batteryPower->powerBatteryToGrid = 0;
            //			m_batteryPower->powerPVToGrid  = 0;
        }
    }
    return iterate;
}

battery_metrics_t::battery_metrics_t(double dt_hour)
{
    _dt_hour = dt_hour;

    // single value metrics
    _e_charge_accumulated = 0;
    _e_charge_from_pv = 0.;
    _e_charge_from_grid = _e_charge_accumulated; // assumes initial charge from grid
    _e_discharge_accumulated = 0.;
    _e_loss_system = 0.;
    _average_efficiency = 100.;
    _average_roundtrip_efficiency = 100.;
    _pv_charge_percent = 0.;
    _grid_charge_percent = 0.;

    // annual metrics
    _e_charge_from_pv_annual = 0.;
    _e_charge_from_grid_annual = _e_charge_from_grid;
    _e_charge_annual = _e_charge_accumulated;
    _e_discharge_annual = 0.;
    _e_loss_system_annual = _e_loss_system;
    _e_grid_import_annual = 0.;
    _e_grid_export_annual = 0.;
    _e_loss_annual = 0.;
}
double battery_metrics_t::average_battery_conversion_efficiency() { return _average_efficiency; }
double battery_metrics_t::average_battery_roundtrip_efficiency() { return _average_roundtrip_efficiency; }
double battery_metrics_t::pv_charge_percent() { return _pv_charge_percent; }
double battery_metrics_t::grid_charge_percent() { return _grid_charge_percent; }
double battery_metrics_t::energy_pv_charge_annual() { return _e_charge_from_pv_annual; }
double battery_metrics_t::energy_grid_charge_annual() { return _e_charge_from_grid_annual; }
double battery_metrics_t::energy_charge_annual() { return _e_charge_annual; }
double battery_metrics_t::energy_discharge_annual() { return _e_discharge_annual; }
double battery_metrics_t::energy_grid_import_annual() { return _e_grid_import_annual; }
double battery_metrics_t::energy_grid_export_annual() { return _e_grid_export_annual; }
double battery_metrics_t::energy_loss_annual() { return _e_loss_annual; }
double battery_metrics_t::energy_system_loss_annual() { return _e_loss_system_annual; };

void battery_metrics_t::compute_metrics_ac(const BatteryPower* batteryPower)
{
	accumulate_grid_annual(batteryPower->powerGrid);
	accumulate_battery_charge_components(batteryPower->powerBatteryAC, batteryPower->powerSystemToBatteryAC, batteryPower->powerGridToBattery);
	accumulate_energy_charge(batteryPower->powerBatteryAC);
	accumulate_energy_discharge(batteryPower->powerBatteryAC);
	accumulate_energy_system_loss(batteryPower->powerSystemLoss);
	compute_annual_loss();
}
void battery_metrics_t::compute_annual_loss()
{
    double e_conversion_loss = 0.;
    if (_e_charge_annual > _e_discharge_annual)
        e_conversion_loss = _e_charge_annual - _e_discharge_annual;
    _e_loss_annual = e_conversion_loss + _e_loss_system_annual;
}
void battery_metrics_t::accumulate_energy_charge(double P_tofrom_batt)
{
    if (P_tofrom_batt < 0.)
    {
        _e_charge_accumulated += (-P_tofrom_batt) * _dt_hour;
        _e_charge_annual += (-P_tofrom_batt) * _dt_hour;
    }
}
void battery_metrics_t::accumulate_energy_discharge(double P_tofrom_batt)
{
    if (P_tofrom_batt > 0.)
    {
        _e_discharge_accumulated += P_tofrom_batt * _dt_hour;
        _e_discharge_annual += P_tofrom_batt * _dt_hour;
    }
}
void battery_metrics_t::accumulate_energy_system_loss(double P_system_loss)
{
    _e_loss_system += P_system_loss * _dt_hour;
    _e_loss_system_annual += P_system_loss * _dt_hour;
}
void battery_metrics_t::accumulate_battery_charge_components(double P_tofrom_batt, double P_pv_to_batt, double P_grid_to_batt)
{
    if (P_tofrom_batt < 0.)
    {
        _e_charge_from_pv += P_pv_to_batt * _dt_hour;
        _e_charge_from_pv_annual += P_pv_to_batt * _dt_hour;
        _e_charge_from_grid += P_grid_to_batt * _dt_hour;
        _e_charge_from_grid_annual += P_grid_to_batt * _dt_hour;
    }
    _average_efficiency = 100. * (_e_discharge_accumulated / _e_charge_accumulated);
    _average_roundtrip_efficiency = 100. * (_e_discharge_accumulated / (_e_charge_accumulated + _e_loss_system));
    _pv_charge_percent = 100. * (_e_charge_from_pv / _e_charge_accumulated);
    _grid_charge_percent = 100. * (_e_charge_from_grid / _e_charge_accumulated);
}
void battery_metrics_t::accumulate_grid_annual(double P_tofrom_grid)
{
    // e_grid > 0 (export to grid)
    // e_grid < 0 (import from grid)

    if (P_tofrom_grid > 0)
        _e_grid_export_annual += P_tofrom_grid * _dt_hour;
    else
        _e_grid_import_annual += (-P_tofrom_grid) * _dt_hour;
}

void battery_metrics_t::new_year()
{
    _e_charge_from_pv_annual = 0.;
    _e_charge_from_grid_annual = 0;
    _e_charge_annual = 0.;
    _e_discharge_annual = 0.;
    _e_grid_import_annual = 0.;
    _e_grid_export_annual = 0.;
    _e_loss_system_annual = 0.;
}

outage_manager::outage_manager(BatteryPower* batteryPower, battery_t* battery) {
    m_batteryPower = batteryPower;
    _Battery = battery;
    canSystemChargeWhenGrid = m_batteryPower->canSystemCharge;
    canClipChargeWhenGrid = m_batteryPower->canClipCharge;
    canGridChargeWhenGrid = m_batteryPower->canGridCharge;
    canDischargeWhenGrid = m_batteryPower->canDischarge;

    stateOfChargeMaxWhenGrid = m_batteryPower->stateOfChargeMax;
    stateOfChargeMinWhenGrid = m_batteryPower->stateOfChargeMin;
    last_step_was_outage = false;
    recover_from_outage = false;
}

outage_manager::~outage_manager() {
    m_batteryPower = NULL;
    _Battery = NULL;
}

void outage_manager::copy(const outage_manager& tmp) {
    // Do not copy battery power - that belongs to a different constructor
    canSystemChargeWhenGrid = tmp.canSystemChargeWhenGrid;
    canClipChargeWhenGrid = tmp.canClipChargeWhenGrid;
    canGridChargeWhenGrid = tmp.canGridChargeWhenGrid;
    canDischargeWhenGrid = tmp.canDischargeWhenGrid;

    stateOfChargeMaxWhenGrid = tmp.stateOfChargeMaxWhenGrid;
    stateOfChargeMinWhenGrid = tmp.stateOfChargeMinWhenGrid;
    last_step_was_outage = tmp.last_step_was_outage;
    recover_from_outage = tmp.recover_from_outage;
}

void outage_manager::update(bool isAutomated, double min_outage_soc) {
    recover_from_outage = false;
    if (m_batteryPower->isOutageStep && !last_step_was_outage) {
        startOutage(min_outage_soc);
    }
    else if (!m_batteryPower->isOutageStep && last_step_was_outage) {
        endOutage(isAutomated);
        recover_from_outage = true; // True for one timestep so dispatch can re-plan
    }
}


void outage_manager::startOutage(double min_outage_soc) {
    canSystemChargeWhenGrid = m_batteryPower->canSystemCharge;	
    canClipChargeWhenGrid = m_batteryPower->canClipCharge;
    canGridChargeWhenGrid = m_batteryPower->canGridCharge;
    canDischargeWhenGrid = m_batteryPower->canDischarge;

    stateOfChargeMaxWhenGrid = m_batteryPower->stateOfChargeMax; 
    stateOfChargeMinWhenGrid = m_batteryPower->stateOfChargeMin;

    if (m_batteryPower->connectionMode == m_batteryPower->DC_CONNECTED) {
        m_batteryPower->canClipCharge = true;
    }
    m_batteryPower->canSystemCharge = true;
    m_batteryPower->canGridCharge = false;
    m_batteryPower->canDischarge = true;

    m_batteryPower->stateOfChargeMax = 100.0;
    m_batteryPower->stateOfChargeMin = min_outage_soc;

    _Battery->changeSOCLimits(min_outage_soc, 100.);

    last_step_was_outage = true;
}

void outage_manager::endOutage(bool isAutomated) {
    if (isAutomated) {
        m_batteryPower->canSystemCharge = canSystemChargeWhenGrid;
        m_batteryPower->canClipCharge = canClipChargeWhenGrid;
        m_batteryPower->canGridCharge = canGridChargeWhenGrid;
        m_batteryPower->canDischarge = canDischargeWhenGrid;
    }

    m_batteryPower->stateOfChargeMax = stateOfChargeMaxWhenGrid;
    m_batteryPower->stateOfChargeMin = stateOfChargeMinWhenGrid;

    _Battery->changeSOCLimits(stateOfChargeMinWhenGrid, stateOfChargeMaxWhenGrid);

    last_step_was_outage = false;
}

bool byGrid:: operator()(grid_point const& a, grid_point const& b)
{
    return a.Grid() > b.Grid();
}

bool byCost::operator() (grid_point const& a, grid_point const& b)
{
    if (a.Cost() == b.Cost())
    {
        return a.Grid() > b.Grid();
    }
    return a.Cost() > b.Cost();
}

bool byLowestMarginalCost::operator() (grid_point const& a, grid_point const& b)
{

    if (std::abs(a.MarginalCost() - b.MarginalCost()) < 1e-7)
    {
        if (std::abs(a.Grid()) < 1e-7 || std::abs(b.Grid()) < 1e-7)
        {
            return a.Grid() < b.Grid();
        }
        else if (std::abs((a.Cost() / a.Grid()) - (b.Cost() / b.Grid())) < 1e-7)
        {
            return a.Grid() < b.Grid();
        }
        return (a.Cost() / a.Grid()) < (b.Cost() / b.Grid());
    }

    return a.MarginalCost() < b.MarginalCost();

}
