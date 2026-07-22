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

#ifndef __csp_solver_battery_
#define __csp_solver_battery_

#include "lib_battery.h"
#include "../ssc/cmod_battery.h"

struct battstor_csp : public battstor {
    battstor_csp(var_table& vt, bool setup_model, size_t nrec, double dt_hr,
                 const std::shared_ptr<batt_variables>& batt_vars_in = 0);
    battstor_csp(const battstor_csp& orig);
    ~battstor_csp() override = default;
    
};

class C_csp_battery
{
public:

    enum
    {
        Voltage,                    // [V] Battery voltage
        Power,                      // [kW] Battery power (positive for discharge, negative for charge)
        Capacity,                   // [Ah] Battery capacity
        MaxCapacity,                // [Ah] Maximum battery capacity
        Current,                    // [A] Battery current
        I_dischargeable,            // [A] Estimated max dischargeable current
        I_chargeable,               // [A] Estimated max chargeable current
        P_dischargeable,            // [kW] Estimated max dischargeable power
        P_chargeable,               // [kW] Estimated max chargeable power
        SOC,                        // [%] State of Charge
        q0,                         // [Ah] Cell capacity at timestep
        qmax_lifetime,              // [Ah] Maximum possible cell capacity
        qmax_thermal,               // [Ah] Maximum cell capacity adjusted for temperature effects
        cell_current,               // [A] Cell current
        I_loss,                     // [A] Current lost to lifetime and thermal losses
        charge_mode,                // [0/1/2] 0 = Charge, 1 = Idle, 2 = Discharge
        //SOC_prev,                   // [%] State of Charge of last time step
        //prev_charge,                // [0/1/2] Charge mode of last time step
        percent_unavailable,        // [%] Percent of system that is down
        //percent_unavailable_prev,   // [%] Percent of system that was down last step
        //chargeChange,                // [0/1] Whether charge mode changed since last step
        q1_0,                       // [Ah] Lead acid - Cell charge available
        q2_0,                       // [Ah] Lead acid - Cell charge bound
        qn,                         // [Ah] Lead acid - Cell capacity at n-hr discharge rate
        q2,                         // [Ah] Lead acid - Cell capacity at 10-hr discharge rate
        cell_voltage,               // [V] Cell voltage
        q_relative_thermal,         // [%] Relative capacity due to thermal effects
        T_batt,                     // [C] Battery temperature averaged over time step
        T_room,                     // [C] Room temperature
        heat_dissipated,            // [kW] Heat dissipated due to flux
        T_batt_prev,                // [C] Battery temperature at end of last time step
        q_relative,                 // [%] Overall relative capacity due to lifetime effects
        q_relative_cycle,           // [%] Relative capacity due to cycling effects
        n_cycles,                   // [1] Number of cycles
        cycle_range,                // [%] Range of last cycle
        cycle_DOD,                  // [%] Depth of Discharge of last cycle
        cycle_counts,               // [%, cycle] Counts of cycles by DOD
        average_range,              // [%] Average cycle range
        rainflow_Xlt,               // [%] Rainflow cycle range of second to last half cycle
        rainflow_Ylt,               // [%] Rainflow cycle range of last half cycle
        rainflow_jlt,               // [1] Rainflow number of turning points
        rainflow_peaks,             // [%] Rainflow peaks of cycle_DOD
        q_relative_calendar,        // [%] Relative capacity due to calendar effects
        day_age_of_battery,         // [day] Day age of battery
        dq_relative_calendar_old,   // [%] Change in capacity of last time step
        DOD_max,                    // [%] Max DOD of battery for current day
        DOD_min,                    // [%] Min DOD of battery for current day
        cycle_DOD_max,              // [%] Max DODs of cycles concluded in current day
        cum_dt,                     // [day] Elapsed time for current day
        q_relative_li,              // [%] Relative capacity due to loss of lithium inventory
        q_relative_neg,             // [%] Relative capacity due to loss of anode material
        dq_relative_li1,            // [1] Cumulative capacity change from time-dependent Li loss
        dq_relative_li2,            // [1] Cumulative capacity change from cycle-dependent Li loss
        dq_relative_li3,            // [1] Cumulative capacity change from BOL Li loss
        dq_relative_neg,            // [1] Cumulative capacity change from negative electrode
        b1_dt,                      // [day^-0.5] b1 coefficient cumulated for current day
        b2_dt,                      // [1/cycle] b2 coefficient cumulated for current day
        b3_dt,                      // [1] b3 coefficient cumulated for current day
        c0_dt,                      // [Ah] c0 coefficient cumulated for current day
        c2_dt,                      // [1/cycle] c2 coefficient cumulated for current day
        temp_dt,                    // [K] Temperature cumulated for current day
        dq_relative_cal,            // [%] Cumulative capacity change from calendar degradation
        dq_relative_cyc,            // [%] Cumulative capacity change from cycling degradation
        EFC,                        // [1] Total Equivalent Full Cycles
        EFC_dt,                     // [1] Equivalent Full Cycles cumulated for current day
        temp_avg,                   // [K] Average temperature for current day
        loss_kw,                    // [kW] Ancillary power loss (kW DC for DC connected, AC for AC connected)
        n_replacements,             // [1] Number of replacements at current year
        indices_replaced,           // [1] Lifetime indices of replacement occurrences
    };

    double dt_hr = 1.0;
    int control_mode = 1;
    int last_idx = 0;   // TODO: where is this suppose to be initialized
    battery_t* battery = nullptr;           // Non-owning: caller retains ownership
    int chem = 0;                           // battery_params::CHEM value, used in write_outputs
    int life_model = 0;                     // lifetime_params::MODEL_CHOICE value, used in write_outputs
    C_csp_reported_outputs mc_reported_outputs;

    /// Construct from a non-owning battery_t pointer plus the chemistry and life-model
    /// selectors needed for reporting. The caller (typically an SSC cmod) owns the battery_t.
    C_csp_battery(battery_t* battery_in, int chem_in, int life_model_in, double dt_hr_in);

    ~C_csp_battery(){};

    //void init();

    void call(double target_power);

    void write_outputs();

    void write_output_intervals(double report_time_start, const std::vector<double>& v_temp_ts_time_end, double report_time_end);
};

#endif // __csp_solver_battery_
