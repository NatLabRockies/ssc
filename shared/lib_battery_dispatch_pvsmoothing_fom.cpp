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


#include "lib_battery_dispatch_pvsmoothing_fom.h"
#include "lib_battery_powerflow.h"
#include "lib_utility_rate.h"

#include <numeric>

dispatch_pvsmoothing_front_of_meter_t::dispatch_pvsmoothing_front_of_meter_t(
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
	double inverter_paco,
    std::vector<double> battReplacementCostPerkWh,
	int battCycleCostChoice,
    std::vector<double> battCycleCost,
    std::vector<double> battOMCost, // required for base class
	double etaPVCharge,
	double etaGridCharge,
	double etaDischarge,
    double batt_dispatch_pvs_nameplate_ac,
    double batt_dispatch_pvs_ac_lb,
    bool batt_dispatch_pvs_ac_lb_enable,
    double batt_dispatch_pvs_ac_ub,
    bool batt_dispatch_pvs_ac_ub_enable,
    bool batt_dispatch_pvs_curtail_as_control,
    bool batt_dispatch_pvs_curtail_if_violation,
    size_t batt_dispatch_pvs_forecast_shift_periods, 
    double batt_dispatch_pvs_kf,
    double batt_dispatch_pvs_ki,
    double batt_dispatch_pvs_kp,
    double batt_dispatch_pvs_max_ramp,
    bool batt_dispatch_pvs_short_forecast_enable,
    double batt_dispatch_pvs_soc_rest,
    size_t batt_dispatch_pvs_timestep_multiplier,
    double interconnection_limit

) : dispatch_automatic_t(Battery, dt_hour, SOC_min, SOC_max, current_choice, Ic_max, Id_max, Pc_max_kwdc, Pd_max_kwdc, Pc_max_kwac, Pd_max_kwac,
		t_min, dispatch_mode, weather_forecast_mode, pv_dispatch, nyears, look_ahead_hours, dispatch_update_frequency_hours,
        can_charge, can_clip_charge, can_grid_charge, can_fuelcell_charge, can_curtail_charge,
        battReplacementCostPerkWh, battCycleCostChoice, battCycleCost, battOMCost, interconnection_limit),
    m_batt_dispatch_pvs_nameplate_ac(batt_dispatch_pvs_nameplate_ac),
    m_batt_dispatch_pvs_ac_lb(batt_dispatch_pvs_ac_lb),
    m_batt_dispatch_pvs_ac_lb_enable(batt_dispatch_pvs_ac_lb_enable),
    m_batt_dispatch_pvs_ac_ub(batt_dispatch_pvs_ac_ub),
    m_batt_dispatch_pvs_ac_ub_enable(batt_dispatch_pvs_ac_ub_enable),
    m_batt_dispatch_pvs_curtail_as_control(batt_dispatch_pvs_curtail_as_control),
    m_batt_dispatch_pvs_curtail_if_violation(batt_dispatch_pvs_curtail_if_violation),
    m_batt_dispatch_pvs_forecast_shift_periods(batt_dispatch_pvs_forecast_shift_periods),
    m_batt_dispatch_pvs_kf(batt_dispatch_pvs_kf),
    m_batt_dispatch_pvs_ki(batt_dispatch_pvs_ki),
    m_batt_dispatch_pvs_kp(batt_dispatch_pvs_kp),
    m_batt_dispatch_pvs_max_ramp(batt_dispatch_pvs_max_ramp),
    m_batt_dispatch_pvs_short_forecast_enable(batt_dispatch_pvs_short_forecast_enable),
    m_batt_dispatch_pvs_soc_rest(batt_dispatch_pvs_soc_rest),
    m_batt_dispatch_pvs_timestep_multiplier(batt_dispatch_pvs_timestep_multiplier)
{

	_inverter_paco = inverter_paco;

	m_etaPVCharge = etaPVCharge * 0.01;
	m_etaGridCharge = etaGridCharge * 0.01;
	m_etaDischarge = etaDischarge * 0.01;

    m_batt_dispatch_pvs_outpower = m_batt_dispatch_pvs_battpower = m_batt_dispatch_pvs_curtail = m_batt_dispatch_pvs_violation_list = 0.0;
    m_batt_dispatch_pvs_battsoc = batt_dispatch_pvs_soc_rest;

    omCost();
    costToCycle();
}
dispatch_pvsmoothing_front_of_meter_t::~dispatch_pvsmoothing_front_of_meter_t(){ /* NOTHING TO DO */}

void dispatch_pvsmoothing_front_of_meter_t::init_with_pointer(const dispatch_pvsmoothing_front_of_meter_t* tmp)
{
	_forecast_hours = tmp->_forecast_hours;
	_inverter_paco = tmp->_inverter_paco;

	m_etaPVCharge = tmp->m_etaPVCharge;
	m_etaGridCharge = tmp->m_etaGridCharge;
	m_etaDischarge = tmp->m_etaDischarge;

    m_batt_dispatch_pvs_nameplate_ac = tmp->m_batt_dispatch_pvs_nameplate_ac;
    m_batt_dispatch_pvs_ac_lb = tmp->m_batt_dispatch_pvs_ac_lb;
    m_batt_dispatch_pvs_ac_lb_enable = tmp->m_batt_dispatch_pvs_ac_lb_enable;
    m_batt_dispatch_pvs_ac_ub = tmp->m_batt_dispatch_pvs_ac_ub;
    m_batt_dispatch_pvs_ac_ub_enable = tmp->m_batt_dispatch_pvs_ac_ub_enable;
    m_batt_dispatch_pvs_curtail_as_control = tmp->m_batt_dispatch_pvs_curtail_as_control;
    m_batt_dispatch_pvs_curtail_if_violation = tmp->m_batt_dispatch_pvs_curtail_if_violation;
    m_batt_dispatch_pvs_forecast_shift_periods = tmp->m_batt_dispatch_pvs_forecast_shift_periods;
    m_batt_dispatch_pvs_kf = tmp->m_batt_dispatch_pvs_kf;
    m_batt_dispatch_pvs_ki = tmp->m_batt_dispatch_pvs_ki;
    m_batt_dispatch_pvs_kp = tmp->m_batt_dispatch_pvs_kp;
    m_batt_dispatch_pvs_max_ramp = tmp->m_batt_dispatch_pvs_max_ramp;
    m_batt_dispatch_pvs_short_forecast_enable = tmp->m_batt_dispatch_pvs_short_forecast_enable;
    m_batt_dispatch_pvs_soc_rest = tmp->m_batt_dispatch_pvs_soc_rest;
    m_batt_dispatch_pvs_timestep_multiplier = tmp->m_batt_dispatch_pvs_timestep_multiplier;

}


// deep copy from dispatch to this
dispatch_pvsmoothing_front_of_meter_t::dispatch_pvsmoothing_front_of_meter_t(const dispatch_t & dispatch) :
dispatch_automatic_t(dispatch)
{
	const dispatch_pvsmoothing_front_of_meter_t * tmp = dynamic_cast<const dispatch_pvsmoothing_front_of_meter_t *>(&dispatch);
	init_with_pointer(tmp);
}

// shallow copy from dispatch to this
void dispatch_pvsmoothing_front_of_meter_t::copy(const dispatch_t * dispatch)
{
	dispatch_automatic_t::copy(dispatch);
	const dispatch_pvsmoothing_front_of_meter_t * tmp = dynamic_cast<const dispatch_pvsmoothing_front_of_meter_t *>(dispatch);
	init_with_pointer(tmp);
}

void dispatch_pvsmoothing_front_of_meter_t::dispatch(size_t year,
	size_t hour_of_year,
	size_t step)
{
    curr_year = year;
	size_t step_per_hour = (size_t)(1 / _dt_hour);
	size_t lifetimeIndex = util::lifetimeIndex(year, hour_of_year, step, step_per_hour);

	update_dispatch(year, hour_of_year, step, lifetimeIndex);
	dispatch_automatic_t::dispatch(year, hour_of_year, step);
}

void dispatch_pvsmoothing_front_of_meter_t::update_dispatch(size_t year, size_t hour_of_year, size_t , size_t lifetimeIndex)
{
    
        // Initialize
	    m_batteryPower->powerBatteryDC = 0;
	    m_batteryPower->powerBatteryAC = 0;
	    m_batteryPower->powerBatteryTarget = 0;

     // timestep method at ramp interval xval = 0
        if (lifetimeIndex % m_batt_dispatch_pvs_timestep_multiplier == 0)
        {
            // Power to charge (<0) or discharge (>0)
            ssc_number_t out_power = 0;

            /*! Cost to cycle the battery at all, using maximum DOD or user input */
            costToCycle();

            omCost();

            // Compute forecast variables which potentially do change from year to year
            double energyToStoreClipped = 0;
            if (_P_cliploss_dc.size() > lifetimeIndex + _forecast_hours) {
                energyToStoreClipped = std::accumulate(_P_cliploss_dc.begin() + lifetimeIndex, _P_cliploss_dc.begin() + lifetimeIndex + _forecast_hours * _steps_per_hour, 0.0) * _dt_hour;
            }


            /*! Energy need to charge the battery (kWh) */
            double energyNeededToFillBattery = _Battery->energy_to_fill(m_batteryPower->stateOfChargeMax);


            // Always Charge if PV is clipping */
            if (m_batteryPower->canClipCharge && m_batteryPower->powerSystemClipped > 0)
            {
                out_power = -m_batteryPower->powerSystemClipped;
            }


            // PV Smoothing algorithm modified for single timestep
            // forecast period number of resampled pv power outputs at weather file timestep multiplier
            ssc_number_t pv_power = 0;
            size_t num_summed = 0;
            for (size_t i_sampled = 0; i_sampled < m_batt_dispatch_pvs_timestep_multiplier && (lifetimeIndex + i_sampled) < _P_pv_ac.size(); i_sampled++) {
                pv_power += _P_pv_ac[lifetimeIndex + i_sampled];
                num_summed++;
            }
            // take mean value
            pv_power = (num_summed > 0) ? pv_power / num_summed : pv_power;
            // scale by nameplate per ERPI code
            pv_power = m_batt_dispatch_pvs_nameplate_ac > 0 ? pv_power / m_batt_dispatch_pvs_nameplate_ac : pv_power;

            // forecast energy
            ssc_number_t forecast_pv_energy = 0;

            for (size_t i_forecast = 0; i_forecast < m_batt_dispatch_pvs_forecast_shift_periods; i_forecast++) {
                for (size_t i_sampled = 0; i_sampled < m_batt_dispatch_pvs_timestep_multiplier && (lifetimeIndex + i_sampled + i_forecast * m_batt_dispatch_pvs_timestep_multiplier) < _P_pv_ac.size(); i_sampled++) {
                    forecast_pv_energy += _P_pv_ac[lifetimeIndex + i_sampled + i_forecast * m_batt_dispatch_pvs_timestep_multiplier];
                }
            }
            // scale by nameplate per ERPI code
            forecast_pv_energy *= _dt_hour;// energy conversion for weather file (_P_pv_ac) timestep
            forecast_pv_energy = m_batt_dispatch_pvs_nameplate_ac > 0 ? forecast_pv_energy / m_batt_dispatch_pvs_nameplate_ac : forecast_pv_energy;

            m_batt_dispatch_pvs_PV_ramp_interval = pv_power;
            m_batt_dispatch_pvs_forecast_pv_energy = forecast_pv_energy;

            ssc_number_t  power_to_energy_conversion_factor = m_batt_dispatch_pvs_timestep_multiplier * _dt_hour;
            ssc_number_t battery_power_terminal = 0;
            ssc_number_t forecast_power = 0;
            ssc_number_t previous_power = m_batt_dispatch_pvs_outpower;
            ssc_number_t battery_soc = _Battery->SOC()/100.0;
            ssc_number_t battery_energy = _Battery->energy_nominal();
            ssc_number_t batt_half_round_trip_eff = sqrt(m_etaDischarge * m_etaPVCharge);
            ssc_number_t battery_power = m_batteryPower->getMaxACChargePower();
            ssc_number_t soc_min = _Battery->SOC_min() * 0.01;
            ssc_number_t soc_max = _Battery->SOC_max() * 0.01;
            // scale by nameplate per ERPI code
            battery_energy = m_batt_dispatch_pvs_nameplate_ac > 0 ? battery_energy / m_batt_dispatch_pvs_nameplate_ac : battery_energy;
            battery_power = m_batt_dispatch_pvs_nameplate_ac > 0 ? battery_power / m_batt_dispatch_pvs_nameplate_ac : battery_power;
            battery_soc = battery_soc * battery_energy;

            forecast_power = forecast_pv_energy;

            ssc_number_t kf = m_batt_dispatch_pvs_kf;
            if (!m_batt_dispatch_pvs_short_forecast_enable)
                kf = 0;

            // calculate controller error
            ssc_number_t delta_power = pv_power - previous_power; //#proportional error
            ssc_number_t soc_increment = battery_soc + (pv_power - previous_power) * power_to_energy_conversion_factor;// #integral error
            ssc_number_t future_error = previous_power * m_batt_dispatch_pvs_forecast_shift_periods * power_to_energy_conversion_factor - forecast_power; //#derivitive error
            ssc_number_t error = m_batt_dispatch_pvs_kp * delta_power + m_batt_dispatch_pvs_ki * (soc_increment - m_batt_dispatch_pvs_soc_rest * battery_energy) - kf * future_error;

            // calculate the desired output power, enforce ramp rate limit
            if (error > 0)
                out_power = previous_power + std::min(m_batt_dispatch_pvs_max_ramp, std::abs(error));
            else
                out_power = previous_power - std::min(m_batt_dispatch_pvs_max_ramp, std::abs(error));

            // enforce grid power limits
            if (m_batt_dispatch_pvs_ac_ub_enable) {
                if (out_power > m_batt_dispatch_pvs_ac_ub)
                    out_power = m_batt_dispatch_pvs_ac_ub;
            }
            if (m_batt_dispatch_pvs_ac_lb_enable) {
                if (out_power < m_batt_dispatch_pvs_ac_lb)
                    out_power = m_batt_dispatch_pvs_ac_lb;
            }

            // calculate desired(unconstrained) battery power
            battery_power_terminal = out_power - pv_power; // positive is power leaving battery(discharging)

            // If grid charging is not allowed, restrict charging to PV output power
            if (!m_batteryPower->canGridCharge && battery_power_terminal < 0) {
                if (std::abs(battery_power_terminal) > std::abs(pv_power)) {
                    battery_power_terminal = -1.0 * pv_power;
                }
            }

            // adjust battery power to factor in battery constraints
            // check SOC limit - reduce battery power if either soc exceeds either soc_min or soc_max
            // check full
            if ((battery_soc - battery_power_terminal * batt_half_round_trip_eff * power_to_energy_conversion_factor) > soc_max * battery_energy)
                battery_power_terminal = -1.0 * (soc_max * battery_energy - battery_soc) / power_to_energy_conversion_factor / batt_half_round_trip_eff;
            // check empty
            else if ((battery_soc - battery_power_terminal * power_to_energy_conversion_factor) < soc_min * battery_energy)
                battery_power_terminal = (battery_soc - soc_min * battery_energy) / power_to_energy_conversion_factor / batt_half_round_trip_eff;

            // enforce battery power limits
            // discharging too fast
            if (battery_power_terminal > battery_power)
                battery_power_terminal = battery_power;
            // charging too fast
            else if (battery_power_terminal < -1.0 * battery_power)
                battery_power_terminal = -1.0 * battery_power;

            // update output power after battery constraints are applied
            out_power = pv_power + battery_power_terminal;

            // flag if a ramp rate violation has occurred - up or down - because limits of battery prevented smoothing
            int violation = 0;
            if (std::abs(out_power - previous_power) > (m_batt_dispatch_pvs_max_ramp + 0.00001)) // check units and scale
                violation = 1;

            // curtailment
            ssc_number_t curtail_power = 0;

            // if curtailment is considered part of the control - don't count up-ramp violations
            if (m_batt_dispatch_pvs_curtail_as_control) {
                if ((out_power - previous_power) > (m_batt_dispatch_pvs_max_ramp - 0.00001)) {
                    out_power = previous_power + m_batt_dispatch_pvs_max_ramp;// reduce output to a non - violation
                    curtail_power = pv_power + battery_power_terminal - out_power;// curtail the remainder
                    violation = 0;
                }
            }

            // with this setting, curtail output power upon an upramp violation - rather than sending excess power to the grid
            // curtailment still counts as a violation
            // sum total of energy output is reduced
            if (m_batt_dispatch_pvs_curtail_if_violation) {
                if ((out_power - previous_power) > (m_batt_dispatch_pvs_max_ramp - 0.00001)) {
                    out_power = previous_power + m_batt_dispatch_pvs_max_ramp; //#reduce output to a non - violation
                    curtail_power = pv_power + battery_power_terminal - out_power;// #curtail the remainder
                }
            }

            // Update SOC to match convention from mainline battery code
            battery_soc -= battery_power_terminal * power_to_energy_conversion_factor;

            // unscaled in public functions
            m_batt_dispatch_pvs_outpower = out_power;
            m_batt_dispatch_pvs_battpower = battery_power_terminal;
             m_batt_dispatch_pvs_battsoc = battery_soc / battery_energy; // plot scaling from Python code
            m_batt_dispatch_pvs_violation_list = violation;
            m_batt_dispatch_pvs_curtail = curtail_power;

        }
        else {
            m_batt_dispatch_pvs_violation_list = 0; // only record violations at ramp interval and reset otherwise
        }

        m_batt_dispatch_pvs_P_pv_ac = _P_pv_ac[lifetimeIndex]; // unsmoothed pv system output.

        // adjust for loss and efficiencies
        m_batteryPower->powerBatteryTarget = (m_batt_dispatch_pvs_nameplate_ac > 0 ? m_batt_dispatch_pvs_nameplate_ac * m_batt_dispatch_pvs_battpower : m_batt_dispatch_pvs_battpower);
        double ancillary_loss_kw = _Battery->calculate_loss(m_batteryPower->powerBatteryTarget, lifetimeIndex); // Battery is responsible for covering discharge losses
        if (m_batteryPower->connectionMode == AC_CONNECTED) {
            m_batteryPower->powerBatteryTarget = m_batteryPower->adjustForACEfficiencies(m_batteryPower->powerBatteryTarget, ancillary_loss_kw);
        }
        else if (m_batteryPower->powerBatteryTarget > 0) {
            // Adjust for DC discharge losses
            m_batteryPower->powerBatteryTarget += ancillary_loss_kw;
        }

        m_batteryPower->powerBatteryDC = m_batteryPower->powerBatteryTarget;
    

}

void dispatch_pvsmoothing_front_of_meter_t::update_pv_data(double_vec P_pv_ac)
{
	_P_pv_ac = P_pv_ac;

	// append to end to allow for look-ahead
	for (size_t i = 0; i != _forecast_hours * _steps_per_hour; i++)
		_P_pv_ac.push_back(P_pv_ac[i]);

 }

void dispatch_pvsmoothing_front_of_meter_t::costToCycle()
{
    // Calculate assuming maximum depth of discharge (most conservative assumption)
    if (m_battCycleCostChoice == dispatch_t::MODEL_CYCLE_COST)
    {
        double capacityPercentDamagePerCycle = _Battery->estimateCycleDamage();
        m_cycleCost = 0.01 * capacityPercentDamagePerCycle * m_battReplacementCostPerKWH[curr_year];
    }
    else if (m_battCycleCostChoice == dispatch_t::INPUT_CYCLE_COST)
    {
        m_cycleCost = cycle_costs_by_year[curr_year];
    }
}

void dispatch_pvsmoothing_front_of_meter_t::omCost()
{
        m_omCost = om_costs_by_year[curr_year];
}

