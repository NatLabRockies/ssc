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


#include "lib_battery_lifetime.h"
#include "lib_battery_lifetime_calendar_cycle.h"
#include "lib_battery_lifetime_nmc.h"
#include "lib_battery_lifetime_lmolto.h"
#include <cmath>

lifetime_params::lifetime_params() {
    model_choice = CALCYC;
    cal_cyc = std::make_shared<calendar_cycle_params>();
}

lifetime_params &lifetime_params::operator=(const lifetime_params &rhs) {
    if (this != &rhs) {
        dt_hr = rhs.dt_hr;
        model_choice = rhs.model_choice;
        *cal_cyc = *rhs.cal_cyc;
    }
    return *this;
}

lifetime_state::lifetime_state(int model_choice) {
    q_relative = 0;
    n_cycles = 0;
    cycle_range = 0;
    cycle_DOD = 0;
    average_range = 0;
    day_age_of_battery = 0;
    cycle = std::make_shared<cycle_state>();
    if (model_choice == lifetime_params::CALCYC) {
        calendar = std::make_shared<calendar_state>();
    }
    else if (model_choice == lifetime_params::NMC)
        nmc_li_neg = std::make_shared<lifetime_nmc_state>();
    else if (model_choice == lifetime_params::LMOLTO)
        lmo_lto = std::make_shared<lifetime_lmolto_state>();
}

lifetime_state::lifetime_state(const lifetime_state &rhs) :
        lifetime_state(-1) {
    operator=(rhs);
}

lifetime_state &lifetime_state::operator=(const lifetime_state &rhs) {
    if (this != &rhs) {
        q_relative = rhs.q_relative;
        n_cycles = rhs.n_cycles;
        cycle_range = rhs.cycle_range;
        cycle_DOD = rhs.cycle_DOD;
        average_range = rhs.average_range;
        day_age_of_battery = rhs.day_age_of_battery;
        *cycle = *rhs.cycle;
        if (rhs.calendar) {
            if (!calendar) calendar = std::make_shared<calendar_state>();
            *calendar = *rhs.calendar;
        }
        if (rhs.nmc_li_neg) {
            if (!nmc_li_neg) nmc_li_neg = std::make_shared<lifetime_nmc_state>();
            *nmc_li_neg = *rhs.nmc_li_neg;
        }
        if (rhs.lmo_lto) {
            if (!lmo_lto) lmo_lto = std::make_shared<lifetime_lmolto_state>();
            *lmo_lto = *rhs.lmo_lto;
        }
    }
    return *this;
}

lifetime_t::lifetime_t(const lifetime_t &rhs) {
    state = std::make_shared<lifetime_state>(*rhs.state);
    params = std::make_shared<lifetime_params>(*rhs.params);
}

lifetime_t &lifetime_t::operator=(const lifetime_t &rhs) {
    if (this != &rhs) {
        *params = *rhs.params;
        *state = *rhs.state;
    }
    return *this;
}

double lifetime_t::capacity_percent() { return state->q_relative; }

double lifetime_t::day_age_of_battery() { return state->day_age_of_battery; }

lifetime_params lifetime_t::get_params() { return *params; }

lifetime_state lifetime_t::get_state() {
    lifetime_state state_copy = *state;
    return state_copy;
}

void lifetime_t::set_state(const lifetime_state& new_state) {
    *state = new_state;
}
