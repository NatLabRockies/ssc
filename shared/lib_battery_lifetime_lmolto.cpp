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


#include <cmath>
#include <numeric>

#include "lib_battery_lifetime_lmolto.h"

void lifetime_lmolto_t::initialize() {
    state = std::make_shared<lifetime_state>(params->model_choice);
    // cycle model for counting cycles only, no cycle-only degradation
    cycle_model = std::unique_ptr<lifetime_cycle_t>(new lifetime_cycle_t(params, state));
    cycle_model->resetDailyCycles();
    state->lmo_lto->dq_relative_cal = 0;
    state->lmo_lto->dq_relative_cyc = 0;
    state->lmo_lto->temp_avg = 0;
    state->lmo_lto->EFC = 0;
    state->lmo_lto->EFC_dt = 0;
    state->q_relative = 100. - state->lmo_lto->dq_relative_cal - state->lmo_lto->dq_relative_cyc;
}


lifetime_lmolto_t::lifetime_lmolto_t(double dt_hr) {
    params = std::make_shared<lifetime_params>();
    params->model_choice = lifetime_params::LMOLTO;
    params->dt_hr = dt_hr;
    initialize();
}

lifetime_lmolto_t::lifetime_lmolto_t(std::shared_ptr<lifetime_params> params_pt) {
    params = std::move(params_pt);
    initialize();
}

lifetime_lmolto_t::lifetime_lmolto_t(std::shared_ptr<lifetime_params> params_pt, std::shared_ptr<lifetime_state> state_pt) {
    params = std::move(params_pt);
    state = std::move(state_pt);
    cycle_model = std::unique_ptr<lifetime_cycle_t>(new lifetime_cycle_t(params, state));
}

lifetime_lmolto_t::lifetime_lmolto_t(const lifetime_lmolto_t &rhs) :
        lifetime_t(rhs){
    operator=(rhs);
}

lifetime_lmolto_t& lifetime_lmolto_t::operator=(const lifetime_lmolto_t& rhs) {
    if (this != &rhs) {
        *params = *rhs.params;
        *state = *rhs.state;
    }
    return *this;
}

lifetime_t * lifetime_lmolto_t::clone() {
    return new lifetime_lmolto_t(*this);
}

double lifetime_lmolto_t::runQcal() {
    double SOC_avg = cycle_model->predictAvgSOC((state->cycle->DOD_max + state->cycle->DOD_min) / 2 * 100);

    // trajectory eqn: qLossCal=q1*t^q2; rate eqn: dqLossCal=q1*q2*(q1 / qLossCal)^((1-q2)/q2))
    double dqLossCal = 0;
    double q1 = q1_b0 * exp(q1_b1 * (1 / pow(state->lmo_lto->temp_avg, 3.)) * sqrt(SOC_avg)) *
                      exp(q1_b2 * (1 / pow(state->lmo_lto->temp_avg, 2.)) * sqrt(SOC_avg));
    if (state->lmo_lto->dq_relative_cal == 0.) {
        if (state->day_age_of_battery > 0) {
            dqLossCal = q1;
        }
    }
    else {
        dqLossCal = q1 * q2 * (double)pow(state->lmo_lto->dq_relative_cal * 0.01 / q1, (q2 - 1.) / q2);
    }
    state->lmo_lto->dq_relative_cal += dqLossCal * 100.;
    return state->lmo_lto->dq_relative_cal;
}

double lifetime_lmolto_t::runQcyc() {
    // use max DOD range regardless of if cycle has completed or not since using EFCs
    double DOD_range = state->cycle->DOD_max - state->cycle->DOD_min;

    // trajectory eqn: qLossCyc=q3*EFC^q4; rate eqn: dqLossCyc / dEFC = q3*q4*(qLossCyc / q3)^((1-q4)/q4))
    // dEFC = EFC_dt * dt_day, and dt_day is always 1 day
    double dqLossCyc = 0;
    double q3 = q3_b0 + q3_b1 * (double)pow(state->lmo_lto->temp_avg, 4.) * (double)pow(DOD_range, 0.25);
    if (state->lmo_lto->dq_relative_cyc == 0.) {
        if (state->lmo_lto->EFC_dt > 0) {
            dqLossCyc = q3 / state->lmo_lto->EFC_dt;
        }
    }
    else {
        if (state->lmo_lto->EFC_dt > 0) {
            dqLossCyc = q3 * q4 * (double)pow(state->lmo_lto->dq_relative_cyc * 0.01 / q3, (q4 - 1) / q4);
        }
    }
    // NaN can happen if q3 <0 since pow with a negative base is a complex not real number.
    // q3 < 0 when DOD_range ~0
    if (std::isnan(dqLossCyc * state->lmo_lto->EFC_dt * 100) || dqLossCyc < 0)
        dqLossCyc = 0.;
    state->lmo_lto->dq_relative_cyc += dqLossCyc * state->lmo_lto->EFC_dt * 100;
    return state->lmo_lto->dq_relative_cyc;
}

void lifetime_lmolto_t::integrateDegLoss() {
    runQcal();
    runQcyc();
    state->q_relative = 100. - state->lmo_lto->dq_relative_cyc - state->lmo_lto->dq_relative_cal;

    // reset daily cycle tracking
    state->lmo_lto->EFC_dt = 0;
    state->lmo_lto->temp_avg = 0;
    cycle_model->resetDailyCycles();
}

void lifetime_lmolto_t::integrateDegParams(double dt_day, double prev_DOD, double DOD, double T_battery) {
    double energy_throughput = std::abs(DOD - prev_DOD) * 0.01 * 0.5;
    state->lmo_lto->EFC_dt += energy_throughput;
    state->lmo_lto->EFC += energy_throughput;
    state->lmo_lto->temp_avg += T_battery * dt_day;
    state->cycle->cum_dt += dt_day;
}

void lifetime_lmolto_t::runLifetimeModels(size_t lifetimeIndex, bool charge_changed, double prev_DOD, double DOD,
                                          double T_battery) {
    T_battery += 273.15;

    cycle_model->updateDailyCycles(prev_DOD, DOD, charge_changed);

    double dt_day = (1. / (double)util::hours_per_day) * params->dt_hr;
    // Run capacity degradation model after every 24 hours
    double new_cum_dt = state->cycle->cum_dt + dt_day;
    // Check if adaptive time stepping has caused new timestep to not hit each day-end
    double dt_day_to_end_of_day = 1 - state->cycle->cum_dt;
    if (new_cum_dt > 1 + 1e-7) {
        double DOD_at_end_of_day = (DOD - prev_DOD) / dt_day * dt_day_to_end_of_day + prev_DOD;
        state->day_age_of_battery += dt_day_to_end_of_day;

        integrateDegParams(dt_day_to_end_of_day, prev_DOD, DOD_at_end_of_day, T_battery);
        integrateDegLoss();

        dt_day = new_cum_dt - 1;
    }

    state->day_age_of_battery += dt_day;
    integrateDegParams(dt_day, prev_DOD, DOD, T_battery);

    if (std::abs(state->cycle->cum_dt - 1.) < 1e-7) {
        integrateDegLoss();
    }
}

void lifetime_lmolto_t::replaceBattery(double percent_to_replace) {
    state->day_age_of_battery = 0;
    state->lmo_lto->dq_relative_cal -= percent_to_replace;
    state->lmo_lto->dq_relative_cyc -= percent_to_replace;
    state->lmo_lto->dq_relative_cal = fmax(0, state->lmo_lto->dq_relative_cal);
    state->lmo_lto->dq_relative_cyc = fmax(0, state->lmo_lto->dq_relative_cyc);
    state->q_relative = 100. - state->lmo_lto->dq_relative_cal - state->lmo_lto->dq_relative_cyc;
    state->lmo_lto->EFC = 0;
    state->lmo_lto->EFC_dt = 0;
    state->lmo_lto->temp_avg = 0;
    cycle_model->replaceBattery(percent_to_replace);
    cycle_model->resetDailyCycles();
    state->cycle->q_relative_cycle = 0;
}

double lifetime_lmolto_t::estimateCycleDamage() {
    return state->lmo_lto->dq_relative_cyc / fmax(1.0, (double)state->n_cycles);
}
