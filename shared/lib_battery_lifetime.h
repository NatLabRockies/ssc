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


#ifndef SAM_SIMULATION_CORE_LIB_BATTERY_LIFETIME_H
#define SAM_SIMULATION_CORE_LIB_BATTERY_LIFETIME_H

#include <memory>


struct calendar_cycle_params;

struct lifetime_params {
    double dt_hr;

    enum MODEL_CHOICE {
        CALCYC,
        NMC,         // K. Smith: Life Prediction model coefficients
        LMOLTO,
    } model_choice;

    std::shared_ptr<calendar_cycle_params> cal_cyc;

    lifetime_params();

    lifetime_params &operator=(const lifetime_params &rhs);

    friend std::ostream &operator<<(std::ostream &os, const lifetime_params &p);
};

struct cycle_state;
struct calendar_state;
struct lifetime_nmc_state;
struct lifetime_lmolto_state;

struct lifetime_state {
    double q_relative;                      // total lifetime relative capacity %
    int n_cycles;
    double cycle_range;                     // DOD range of last completed cycle, %
    double cycle_DOD;                       // max DOD of last completed cycle, %
    double average_range;                   // average cycle_DOD cycle_range of all cycles, %
    double day_age_of_battery;

    // Cycling model state, used for cycle counting in all the MODEL_CHOICE classes
    std::shared_ptr<cycle_state> cycle;

    // Calendar model state, only exists when model_choice is CALCYC
    std::shared_ptr<calendar_state> calendar;

    // NMC model state, only exists when model_choice is NMC
    std::shared_ptr<lifetime_nmc_state> nmc_li_neg;

    // LMO/LTO model state, only exists when model_choice is LMOLTO
    std::shared_ptr<lifetime_lmolto_state> lmo_lto;

    // Creates the appropriate sub-state structs for the model_choice, -1 default creates none of the sub-states
    explicit lifetime_state(int model_choice = -1);

    lifetime_state(const lifetime_state &rhs);

    lifetime_state &operator=(const lifetime_state &rhs);

    friend std::ostream &operator<<(std::ostream &os, const lifetime_state &p);
};

class lifetime_t {
public:
    lifetime_t() = default;

    lifetime_t(const lifetime_t &rhs);

    virtual lifetime_t &operator=(const lifetime_t &rhs);

    virtual lifetime_t *clone() = 0;

    virtual ~lifetime_t() = default;

    /// Execute the lifetime models given the current lifetime run index, capacity model, and temperature
    virtual void runLifetimeModels(size_t lifetimeIndex, bool charge_changed, double prev_DOD, double DOD, double T_battery) = 0;

    /// Return the relative capacity percentage of nominal (%)
    double capacity_percent();

    double day_age_of_battery();

    virtual double estimateCycleDamage() = 0;

    virtual void replaceBattery(double percent_to_replace) = 0;

    lifetime_params get_params();

    lifetime_state get_state();

    void set_state(const lifetime_state &new_state);

protected:

    std::shared_ptr<lifetime_state> state;
    std::shared_ptr<lifetime_params> params;

    friend class battery_t;
};


#endif //SAM_SIMULATION_CORE_LIB_BATTERY_LIFETIME_H
