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

#include "csp_solver_core.h"
#include "csp_solver_battery.h"

static C_csp_reported_outputs::S_output_info S_output_info[] =
{
    {C_csp_battery::Voltage, C_csp_reported_outputs::TS_1ST},		             // [V] Battery voltage
    {C_csp_battery::Power, C_csp_reported_outputs::TS_1ST},                      // [kW] Battery power (positive for discharge, negative for charge)
    {C_csp_battery::Capacity, C_csp_reported_outputs::TS_1ST},                   // [Ah] Battery capacity
    {C_csp_battery::MaxCapacity, C_csp_reported_outputs::TS_1ST},                // [Ah] Maximum battery capacity
    {C_csp_battery::Current, C_csp_reported_outputs::TS_1ST},                    // [A] Battery current
    {C_csp_battery::I_dischargeable, C_csp_reported_outputs::TS_1ST},            // [A] Estimated max dischargeable current
    {C_csp_battery::I_chargeable, C_csp_reported_outputs::TS_1ST},               // [A] Estimated max chargeable current
    {C_csp_battery::P_dischargeable, C_csp_reported_outputs::TS_1ST},            // [kW] Estimated max dischargeable power
    {C_csp_battery::P_chargeable, C_csp_reported_outputs::TS_1ST},               // [kW] Estimated max chargeable power
    {C_csp_battery::SOC, C_csp_reported_outputs::TS_1ST},                        // [%] State of Charge
    {C_csp_battery::q0, C_csp_reported_outputs::TS_1ST},                         // [Ah] Cell capacity at timestep
    {C_csp_battery::qmax_lifetime, C_csp_reported_outputs::TS_1ST},              // [Ah] Maximum possible cell capacity
    {C_csp_battery::qmax_thermal, C_csp_reported_outputs::TS_1ST},               // [Ah] Maximum cell capacity adjusted for temperature effects
    {C_csp_battery::cell_current, C_csp_reported_outputs::TS_1ST},               // [A] Cell current
    {C_csp_battery::I_loss, C_csp_reported_outputs::TS_1ST},                     // [A] Current lost to lifetime and thermal losses
    {C_csp_battery::charge_mode, C_csp_reported_outputs::TS_1ST},                // [0/1/2] 0 = Charge, 1 = Idle, 2 = Discharge
    //{C_csp_battery::SOC_prev, C_csp_reported_outputs::TS_1ST},                   // [%] State of Charge of last time step
    //{C_csp_battery::prev_charge, C_csp_reported_outputs::TS_1ST},                // [0/1/2] Charge mode of last time step
    {C_csp_battery::percent_unavailable, C_csp_reported_outputs::TS_1ST},        // [%] Percent of system that is down
    //{C_csp_battery::percent_unavailable_prev, C_csp_reported_outputs::TS_1ST},   // [%] Percent of system that was down last step
    //{C_csp_battery::chargeChange, C_csp_reported_outputs::TS_1ST},               // [0/1] Whether charge mode changed since last step
    {C_csp_battery::q1_0, C_csp_reported_outputs::TS_1ST},                       // [Ah] Lead acid - Cell charge available
    {C_csp_battery::q2_0, C_csp_reported_outputs::TS_1ST},                       // [Ah] Lead acid - Cell charge bound
    {C_csp_battery::qn, C_csp_reported_outputs::TS_1ST},                         // [Ah] Lead acid - Cell capacity at n-hr discharge rate
    {C_csp_battery::q2, C_csp_reported_outputs::TS_1ST},                         // [Ah] Lead acid - Cell capacity at 10-hr discharge rate
    {C_csp_battery::cell_voltage, C_csp_reported_outputs::TS_1ST},               // [V] Cell voltage
    {C_csp_battery::q_relative_thermal, C_csp_reported_outputs::TS_1ST},         // [%] Relative capacity due to thermal effects
    {C_csp_battery::T_batt, C_csp_reported_outputs::TS_1ST},                     // [C] Battery temperature averaged over time step
    {C_csp_battery::T_room, C_csp_reported_outputs::TS_1ST},                     // [C] Room temperature
    {C_csp_battery::heat_dissipated, C_csp_reported_outputs::TS_1ST},            // [kW] Heat dissipated due to flux
    {C_csp_battery::T_batt_prev, C_csp_reported_outputs::TS_1ST},                // [C] Battery temperature at end of last time step
    {C_csp_battery::q_relative, C_csp_reported_outputs::TS_1ST},                 // [%] Overall relative capacity due to lifetime effects
    {C_csp_battery::q_relative_cycle, C_csp_reported_outputs::TS_1ST},           // [%] Relative capacity due to cycling effects
    {C_csp_battery::n_cycles, C_csp_reported_outputs::TS_1ST},                   // [1] Number of cycles
    {C_csp_battery::cycle_range, C_csp_reported_outputs::TS_1ST},                // [%] Range of last cycle
    {C_csp_battery::cycle_DOD, C_csp_reported_outputs::TS_1ST},                  // [%] Depth of Discharge of last cycle
    {C_csp_battery::cycle_counts, C_csp_reported_outputs::TS_1ST},               // [%, cycle] Counts of cycles by DOD
    {C_csp_battery::average_range, C_csp_reported_outputs::TS_1ST},              // [%] Average cycle range
    {C_csp_battery::rainflow_Xlt, C_csp_reported_outputs::TS_1ST},               // [%] Rainflow cycle range of second to last half cycle
    {C_csp_battery::rainflow_Ylt, C_csp_reported_outputs::TS_1ST},               // [%] Rainflow cycle range of last half cycle
    {C_csp_battery::rainflow_jlt, C_csp_reported_outputs::TS_1ST},               // [1] Rainflow number of turning points
    {C_csp_battery::rainflow_peaks, C_csp_reported_outputs::TS_1ST},             // [%] Rainflow peaks of cycle_DOD
    {C_csp_battery::q_relative_calendar, C_csp_reported_outputs::TS_1ST},        // [%] Relative capacity due to calendar effects
    {C_csp_battery::day_age_of_battery, C_csp_reported_outputs::TS_1ST},         // [day] Day age of battery
    {C_csp_battery::dq_relative_calendar_old, C_csp_reported_outputs::TS_1ST},   // [%] Change in capacity of last time step
    {C_csp_battery::DOD_max, C_csp_reported_outputs::TS_1ST},                    // [%] Max DOD of battery for current day
    {C_csp_battery::DOD_min, C_csp_reported_outputs::TS_1ST},                    // [%] Min DOD of battery for current day
    {C_csp_battery::cycle_DOD_max, C_csp_reported_outputs::TS_1ST},              // [%] Max DODs of cycles concluded in current day
    {C_csp_battery::cum_dt, C_csp_reported_outputs::TS_1ST},                     // [day] Elapsed time for current day
    {C_csp_battery::q_relative_li, C_csp_reported_outputs::TS_1ST},              // [%] Relative capacity due to loss of lithium inventory
    {C_csp_battery::q_relative_neg, C_csp_reported_outputs::TS_1ST},             // [%] Relative capacity due to loss of anode material
    {C_csp_battery::dq_relative_li1, C_csp_reported_outputs::TS_1ST},            // [1] Cumulative capacity change from time-dependent Li loss
    {C_csp_battery::dq_relative_li2, C_csp_reported_outputs::TS_1ST},            // [1] Cumulative capacity change from cycle-dependent Li loss
    {C_csp_battery::dq_relative_li3, C_csp_reported_outputs::TS_1ST},            // [1] Cumulative capacity change from BOL Li loss
    {C_csp_battery::dq_relative_neg, C_csp_reported_outputs::TS_1ST},            // [1] Cumulative capacity change from negative electrode
    {C_csp_battery::b1_dt, C_csp_reported_outputs::TS_1ST},                      // [day^-0.5] b1 coefficient cumulated for current day
    {C_csp_battery::b2_dt, C_csp_reported_outputs::TS_1ST},                      // [1/cycle] b2 coefficient cumulated for current day
    {C_csp_battery::b3_dt, C_csp_reported_outputs::TS_1ST},                      // [1] b3 coefficient cumulated for current day
    {C_csp_battery::c0_dt, C_csp_reported_outputs::TS_1ST},                      // [Ah] c0 coefficient cumulated for current day
    {C_csp_battery::c2_dt, C_csp_reported_outputs::TS_1ST},                      // [1/cycle] c2 coefficient cumulated for current day
    {C_csp_battery::temp_dt, C_csp_reported_outputs::TS_1ST},                    // [K] Temperature cumulated for current day
    {C_csp_battery::dq_relative_cal, C_csp_reported_outputs::TS_1ST},            // [%] Cumulative capacity change from calendar degradation
    {C_csp_battery::dq_relative_cyc, C_csp_reported_outputs::TS_1ST},            // [%] Cumulative capacity change from cycling degradation
    {C_csp_battery::EFC, C_csp_reported_outputs::TS_1ST},                        // [1] Total Equivalent Full Cycles
    {C_csp_battery::EFC_dt, C_csp_reported_outputs::TS_1ST},                     // [1] Equivalent Full Cycles cumulated for current day
    {C_csp_battery::temp_avg, C_csp_reported_outputs::TS_1ST},                   // [K] Average temperature for current day
    {C_csp_battery::loss_kw, C_csp_reported_outputs::TS_1ST},                    // [kW] Ancillary power loss (kW DC for DC connected, AC for AC connected)
    {C_csp_battery::n_replacements, C_csp_reported_outputs::TS_1ST},             // [1] Number of replacements at current year
    {C_csp_battery::indices_replaced, C_csp_reported_outputs::TS_1ST},           // [1] Lifetime indices of replacement occurrences
    csp_info_invalid
};


C_csp_battery::C_csp_battery(std::shared_ptr<battery_params> parameters) {
    params = parameters;        // Battery parameters
    dt_hr = params->dt_hr;     // [hr] time step for battery model
    battery = std::unique_ptr<battery_t>(new battery_t(params));
    mc_reported_outputs.construct(S_output_info);
};

void C_csp_battery::call(double target_power) {
    // to change timestep -> see stateful
    //battery->ChangeTimestep(dt_hr);

    // TODO: FIXME
    // Replacements
    //size_t lifetime_index = as_integer("last_idx"); //TODO make last_idx a member?
    size_t lifetime_index = 8760;
    size_t steps_per_hour = (size_t)(1 / dt_hr);
    size_t steps_per_year = (size_t)(8760 * steps_per_hour);
    size_t year = (size_t)(lifetime_index / steps_per_year);
    size_t year_one_index = lifetime_index - (year * steps_per_year);
    size_t hour = (size_t)(year_one_index / steps_per_hour);
    size_t step_of_hour = year_one_index - (hour * steps_per_hour);

    battery->runReplacement(year, hour, step_of_hour);
    battery->runPower(target_power);
    write_outputs();
};

void C_csp_battery::write_outputs() {
    // Save outputs...
    // Follow write_battery_state()
    battery_state state = battery->get_state();
    last_idx = (int)state.last_idx;
    // set all outputs here
    mc_reported_outputs.value(C_csp_battery::Voltage, state.V);             // [V] Battery voltage
    mc_reported_outputs.value(C_csp_battery::Power, state.P);               // [kW] Battery power (positive for discharge, negative for charge)
    mc_reported_outputs.value(C_csp_battery::Capacity, state.Q);            // [Ah] Battery capacity
    mc_reported_outputs.value(C_csp_battery::MaxCapacity, state.Q_max);     // [Ah] Maximum battery capacity
    mc_reported_outputs.value(Current, state.I);                            // [A] Battery current
    mc_reported_outputs.value(I_dischargeable, state.I_dischargeable);      // [A] Estimated max dischargeable current
    mc_reported_outputs.value(I_chargeable, state.I_chargeable);            // [A] Estimated max chargeable current
    mc_reported_outputs.value(P_dischargeable, state.P_dischargeable);      // [kW] Estimated max dischargeable power
    mc_reported_outputs.value(P_chargeable, state.P_chargeable);            // [kW] Estimated max chargeable power

    auto cap = state.capacity;
    mc_reported_outputs.value(SOC, cap->SOC);                               // [%] State of Charge
    mc_reported_outputs.value(q0, cap->q0);                                 // [Ah] Cell capacity at timestep
    mc_reported_outputs.value(qmax_lifetime, cap->qmax_lifetime);           // [Ah] Maximum possible cell capacity
    mc_reported_outputs.value(qmax_thermal, cap->qmax_thermal);             // [Ah] Maximum cell capacity adjusted for temperature effects
    mc_reported_outputs.value(cell_current, cap->cell_current);             // [A] Cell current
    mc_reported_outputs.value(I_loss, cap->I_loss);                         // [A] Current lost to lifetime and thermal losses
    mc_reported_outputs.value(charge_mode, cap->charge_mode);               // [0/1/2] 0 = Charge, 1 = Idle, 2 = Discharge
    //mc_reported_outputs.value(SOC_prev, cap->SOC_prev);                     // [%] State of Charge of last time step
    //mc_reported_outputs.value(prev_charge, cap->prev_charge);               // [0/1/2] Charge mode of last time step
    mc_reported_outputs.value(percent_unavailable, cap->percent_unavailable);               // [%] Percent of system that is down
    //mc_reported_outputs.value(percent_unavailable_prev, cap->percent_unavailable_prev);     // [%] Percent of system that was down last step
    //mc_reported_outputs.value(chargeChange, cap->chargeChange);             // [0/1] Whether charge mode changed since last step

    if (params->chem == battery_params::CHEM::LEAD_ACID) {
        mc_reported_outputs.value(q1_0, cap->leadacid.q1_0);                // [Ah] Lead acid - Cell charge available
        mc_reported_outputs.value(q2_0, cap->leadacid.q2_0);                // [Ah] Lead acid - Cell charge bound
        mc_reported_outputs.value(qn, cap->leadacid.q1);                    // [Ah] Lead acid - Cell capacity at n-hr discharge rate
        mc_reported_outputs.value(q2, cap->leadacid.q2);                    // [Ah] Lead acid - Cell capacity at 10-hr discharge rate
    }

    mc_reported_outputs.value(cell_voltage, state.voltage->cell_voltage);   // [V] Cell voltage

    auto thermal = state.thermal;
    mc_reported_outputs.value(q_relative_thermal, thermal->q_relative_thermal);     // [%] Relative capacity due to thermal effects
    mc_reported_outputs.value(T_batt, thermal->T_batt);                             // [C] Battery temperature averaged over time step
    mc_reported_outputs.value(T_room, thermal->T_room);                             // [C] Room temperature
    mc_reported_outputs.value(heat_dissipated, thermal->heat_dissipated);           // [kW] Heat dissipated due to flux
    mc_reported_outputs.value(T_batt_prev, thermal->T_batt_prev);                   // [C] Battery temperature at end of last time step

    auto lifetime = state.lifetime;
    mc_reported_outputs.value(q_relative, lifetime->q_relative);                    // [%] Overall relative capacity due to lifetime effects
    mc_reported_outputs.value(q_relative_cycle, lifetime->cycle->q_relative_cycle); // [%] Relative capacity due to cycling effects
    mc_reported_outputs.value(n_cycles, lifetime->n_cycles);                        // [1] Number of cycles
    mc_reported_outputs.value(cycle_range, lifetime->cycle_range);                  // [%] Range of last cycle
    mc_reported_outputs.value(cycle_DOD, lifetime->cycle_DOD);                      // [%] Depth of Discharge of last cycle
    mc_reported_outputs.value(day_age_of_battery, lifetime->day_age_of_battery);    // [day] Day age of battery

    //mc_reported_outputs.value(cycle_counts,               // [%, cycle] Counts of cycles by DOD
    mc_reported_outputs.value(average_range, lifetime->average_range);              // [%] Average cycle range
    mc_reported_outputs.value(rainflow_Xlt, lifetime->cycle->rainflow_Xlt);         // [%] Rainflow cycle range of second to last half cycle
    mc_reported_outputs.value(rainflow_Ylt, lifetime->cycle->rainflow_Ylt);         // [%] Rainflow cycle range of last half cycle
    mc_reported_outputs.value(rainflow_jlt, lifetime->cycle->rainflow_jlt);         // [1] Rainflow number of turning points
    //mc_reported_outputs.value(rainflow_peaks,             // [%] Rainflow peaks of cycle_DOD
    if (params->lifetime->model_choice == lifetime_params::CALCYC) {
        mc_reported_outputs.value(q_relative_calendar, lifetime->calendar->q_relative_calendar);            // [%] Relative capacity due to calendar effects
        mc_reported_outputs.value(dq_relative_calendar_old, lifetime->calendar->dq_relative_calendar_old);  // [%] Change in capacity of last time step
    }
    else {
        mc_reported_outputs.value(DOD_max, lifetime->cycle->cum_dt);                // [%] Max DOD of battery for current day
        mc_reported_outputs.value(DOD_min, lifetime->cycle->DOD_min);               // [%] Min DOD of battery for current day
        //mc_reported_outputs.value(cycle_DOD_max, lifetime->cycle->cycle_DOD_max);   // [%] Max DODs of cycles concluded in current day
        mc_reported_outputs.value(cum_dt, lifetime->cycle->cum_dt);                 // [day] Elapsed time for current day
    }

    if (params->lifetime->model_choice == lifetime_params::NMC) {
        mc_reported_outputs.value(q_relative_li, lifetime->nmc_li_neg->q_relative_li);      // [%] Relative capacity due to loss of lithium inventory
        mc_reported_outputs.value(q_relative_neg, lifetime->nmc_li_neg->q_relative_neg);    // [%] Relative capacity due to loss of anode material
        mc_reported_outputs.value(dq_relative_li1, lifetime->nmc_li_neg->dq_relative_li1);  // [1] Cumulative capacity change from time-dependent Li loss
        mc_reported_outputs.value(dq_relative_li2, lifetime->nmc_li_neg->dq_relative_li2);  // [1] Cumulative capacity change from cycle-dependent Li loss
        mc_reported_outputs.value(dq_relative_li3, lifetime->nmc_li_neg->dq_relative_li3);  // [1] Cumulative capacity change from BOL Li loss
        mc_reported_outputs.value(dq_relative_neg, lifetime->nmc_li_neg->dq_relative_neg);  // [1] Cumulative capacity change from negative electrode
        mc_reported_outputs.value(b1_dt, lifetime->nmc_li_neg->b1_dt);                      // [day^-0.5] b1 coefficient cumulated for current day
        mc_reported_outputs.value(b2_dt, lifetime->nmc_li_neg->b2_dt);                      // [1/cycle] b2 coefficient cumulated for current day
        mc_reported_outputs.value(b3_dt, lifetime->nmc_li_neg->b3_dt);                      // [1] b3 coefficient cumulated for current day
        mc_reported_outputs.value(c0_dt, lifetime->nmc_li_neg->c0_dt);                      // [Ah] c0 coefficient cumulated for current day
        mc_reported_outputs.value(c2_dt, lifetime->nmc_li_neg->c2_dt);                      // [1/cycle] c2 coefficient cumulated for current day
        mc_reported_outputs.value(temp_dt, lifetime->nmc_li_neg->temp_dt);                  // [K] Temperature cumulated for current day
    }
    //else {
    //    mc_reported_outputs.value(dq_relative_cal, lifetime->lmo_lto->dq_relative_cal);     // [%] Cumulative capacity change from calendar degradation
    //    mc_reported_outputs.value(dq_relative_cyc, lifetime->lmo_lto->dq_relative_cyc);     // [%] Cumulative capacity change from cycling degradation
    //    mc_reported_outputs.value(EFC, lifetime->lmo_lto->EFC);                             // [1] Total Equivalent Full Cycles
    //    mc_reported_outputs.value(EFC_dt, lifetime->lmo_lto->EFC_dt);                       // [1] Equivalent Full Cycles cumulated for current day
    //    mc_reported_outputs.value(temp_avg, lifetime->lmo_lto->temp_avg);                   // [K] Average temperature for current day
    //}

    mc_reported_outputs.value(loss_kw, state.losses->ancillary_loss_kw);                    // [kW] Ancillary power loss (kW DC for DC connected, AC for AC connected)
    mc_reported_outputs.value(n_replacements, state.replacement->n_replacements);           // [1] Number of replacements at current year
    //mc_reported_outputs.value(indices_replaced, state.replacement->indices_replaced);       // [1] Lifetime indices of replacement occurrences

    mc_reported_outputs.set_timestep_outputs();

};


void C_csp_battery::write_output_intervals(double report_time_start,
    const std::vector<double>& v_temp_ts_time_end, double report_time_end)
{
    mc_reported_outputs.send_to_reporting_ts_array(report_time_start,
        v_temp_ts_time_end, report_time_end);
}
