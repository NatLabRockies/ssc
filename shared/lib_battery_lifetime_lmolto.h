/*
BSD 3-Clause License

Copyright (c) Alliance for Energy Innovation, LLC. See also https://github.com/NREL/ssc/blob/develop/LICENSE
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


#ifndef SAM_SIMULATION_CORE_LIB_BATTERY_LIFETIME_LMOLTO_H
#define SAM_SIMULATION_CORE_LIB_BATTERY_LIFETIME_LMOLTO_H

#include "lib_util.h"
#include "lib_battery_lifetime_calendar_cycle.h"

/**
 * LMO/LTO Life Model
 *
 * Based on the model developed by NREL with data from INL:
 *  Reference after published
 */

extern const double Rug;
extern const double T_ref;
extern const double F;

struct lifetime_lmolto_state {
    double dq_relative_cal;                 // [%]
    double dq_relative_cyc;                 // [%]
    double EFC;                             // [Equivalent Full Cycles]

    // lifetime capacity updated after 24 hours elapsed using below values, which are all reset each day
    double EFC_dt;                          // [Equivalent Full Cycles / day]
    double temp_avg;                        // [K]

    friend std::ostream& operator<<(std::ostream& os, const lifetime_lmolto_state& p);
};

class lifetime_lmolto_t : public lifetime_t {
public:
    lifetime_lmolto_t(double dt);

    lifetime_lmolto_t(std::shared_ptr<lifetime_params> params_pt);

    lifetime_lmolto_t(std::shared_ptr<lifetime_params> params_pt, std::shared_ptr<lifetime_state> state_pt);

    lifetime_lmolto_t(const lifetime_lmolto_t& rhs);

    lifetime_lmolto_t &operator=(const lifetime_lmolto_t& rhs);

    lifetime_t *clone() override;

    /// Execute the lifetime models given the current lifetime run index, capacity model, and temperature
    void runLifetimeModels(size_t lifetimeIndex, bool charge_changed, double prev_DOD, double DOD, double T_battery) override;

    double estimateCycleDamage() override;

    void replaceBattery(double percent_to_replace) override;

protected:

    std::unique_ptr<lifetime_cycle_t> cycle_model;

    /// Capacity degradation due to calendar loss
    double q2 = 0.6224;                 // 1
    double q1_b0 = 3.4984e-5;           // day^-q2
    double q1_b1 = -1.0704e9;           // K^3
    double q1_b2 = 3.7839e6;            // K^2

    /// Capacity degradation due to cycling loss
    double q4 = 0.5539;                 // 1
    double q3_b0 = -7.146e-4;           // cycles^-q4
    double q3_b1 = 1.071e-13;           // cycles^-q4 K^-4

    /// Update the relative capacity loss due to calendar effects
    double runQcal();

    /// Update the relative capacity loss due to cycling effects
    double runQcyc();

    /// Compute lifetime degradation coefficients for current time step
    void integrateDegParams(double dt_day, double prev_DOD, double DOD, double T_battery);

    /// Integrate degradation from QLi and Qneg over one day, resets `x_dt` values
    void integrateDegLoss();

    void initialize();
};


#endif //SAM_SIMULATION_CORE_LIB_BATTERY_LIFETIME_LMOLTO_H
