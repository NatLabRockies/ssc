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


#ifndef SAM_SIMULATION_CORE_LIB_BATTERY_LIFETIME_NMC_H
#define SAM_SIMULATION_CORE_LIB_BATTERY_LIFETIME_NMC_H

#include <memory>

#include "lib_util.h"
#include "lib_battery_lifetime_calendar_cycle.h"

/**
 * NMC Life Model
 *
 * Based on the model developed by NREL:
 *  K. Smith, A. Saxon, M. Keyser, B. Lundstrom, Ziwei Cao, A. Roc
 *  Life prediction model for grid-connected li-ion battery energy storage system
 *  2017 American Control Conference (ACC) (2017), pp. 4062-4068
 *  https://ieeexplore.ieee.org/document/7963578
 */

// Gas Constant
static const double Rug = 8.314;       // J K mol-1
static const double T_ref = 298.15;    // K
static const double F = 96485;         // A s mol−1


struct lifetime_nmc_state {
    double q_relative_li;                   // SEI degradation, %
    double q_relative_neg;                  // Negative electrode degradation, %
    double dq_relative_li1;                 // cumulative dq from time-dependent Li loss, [0-1]
    double dq_relative_li2;                 // cumulative dq from cycle-dependent Li loss, [0-1]
    double dq_relative_li3;                 // cumulative dq from BOL Li loss, [0-1]
    double dq_relative_neg;                 // cumulative dq from negative electrode, [0-1]
    double temp_dt;                          // cumulative temp [K]

    // lifetime capacity updated after 24 hours elapsed using below values, which are all reset each day
    double b1_dt;
    double b2_dt;
    double b3_dt;
    double c0_dt;
    double c2_dt;

    friend std::ostream& operator<<(std::ostream& os, const lifetime_nmc_state& p);
};

class lifetime_nmc_t : public lifetime_t {
public:
    lifetime_nmc_t(double dt_hr);

    lifetime_nmc_t(std::shared_ptr<lifetime_params> params_pt);

    lifetime_nmc_t(std::shared_ptr<lifetime_params> params_pt, std::shared_ptr<lifetime_state> state_pt);

    lifetime_nmc_t(const lifetime_nmc_t& rhs);

    lifetime_nmc_t &operator=(const lifetime_nmc_t& rhs);

    lifetime_t *clone() override;

    /// Execute the lifetime models given the current lifetime run index, capacity model, and temperature
    void runLifetimeModels(size_t lifetimeIndex, bool charge_changed, double prev_DOD, double DOD, double T_battery) override;

    double estimateCycleDamage() override;

    void replaceBattery(double percent_to_replace) override;

    /// Calculate negative electrode voltage from SOC
    double calculate_Uneg(double SOC);

    /// Calculate open circuit voltage from SOC
    double calculate_Voc(double SOC);

protected:

    std::unique_ptr<lifetime_cycle_t> cycle_model;

    /// Reference Anode and Cell potential
    double Uneg_ref = 0.08;     // V
    double V_ref = 3.7;         // V

    /// Open-circuit voltage and anode potential for SOCs from 0 - 1 in increments of 0.1
    const std::vector<double> ocvs = {3.0000, 3.4679, 3.5394, 3.5950, 3.6453, 3.6876, 3.7469, 3.8400, 3.9521, 4.0668, 4.193};
    const std::vector<double> unegs = {1.2868, 0.2420, 0.1818, 0.1488, 0.1297, 0.1230, 0.1181, 0.1061, 0.0925, 0.0876, 0.0859};

    /// Capacity degradation due to positive electrode-site-limit
    double d0_ref = 75.075;     // Ah
    double Ea_d0_1 = 4126.0;    // J/mol
    double Ea_d0_2 = 9752000.0; // J/mol
    double Ah_ref = 75.;        // Ah

    /// Capacity degradation due to SEI
    double b0 = 1.07;           // 1
    double b1_ref = 0.0035;     // day^-0.5
    double Ea_b1 = 35392.;      // J mol^-1
    double alpha_a_b1 = -1;     // 1
    double beta_b1 = 2.157;     // 1
    double gamma = 2.472;       // 1

    double b2_ref = 0.00001541; // 1
    double Ea_b2 = -42800.;     // J mol^-1

    double b3_ref = 0.02805;    // 1
    double Ea_b3 = 42800.;      // J mol^-1
    double alpha_a_b3 = 0.0066; // 1
    double tau_b3 = 5;          // 1
    double theta = 0.135;       // 1

    double runQli(double T_battery_K);

    /// Capacity degradation due to cycles
    double c0_ref = 75.675;     // Ah
    double Ea_c0_ref = 2224.;   // J mol^-1
    double c2_ref = 5.226e-5;   // Ah cycle^-1
    double Ea_c2 = -48260.;     // J mol^-1
    double beta_c2 = 4.54;      // 1

    double runQneg();

    /// compute lifetime degradation coefficients for current time step
    void integrateDegParams(double dt_day, double DOD, double T_battery);

    /// Integrate degradation from QLi and Qneg over one day, resets `x_dt` values
    void integrateDegLoss();

    void initialize();
};


#endif //SAM_SIMULATION_CORE_LIB_BATTERY_LIFETIME_NMC_H
