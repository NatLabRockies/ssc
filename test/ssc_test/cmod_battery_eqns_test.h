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



#ifndef SAM_SIMULATION_CORE_CMOD_BATTERY_EQNS_TEST_H
#define SAM_SIMULATION_CORE_CMOD_BATTERY_EQNS_TEST_H

#include <gtest/gtest.h>
#include "core.h"

#include "vartab.h"

class CMBatteryEqns_cmod_battery_eqns : public ::testing::Test {

public:

    ssc_data_t data = NULL;
    ssc_module_t mod = NULL;
    std::string params_str;
    double m_error_tolerance_hi = 0.1;
    double m_error_tolerance_lo = 0.0001;

    void CreateModel(double dt_hour = 1.) {
        params_str = R"({ "control_mode": 0, "input_current": 1, "chem": 1, "nominal_energy": 10, "nominal_voltage": 500, "qmax_init": 1000.000, "initial_SOC": 50.000, "maximum_SOC": 95.000, "minimum_SOC": 5.000, "dt_hr": 1.000, "leadacid_tn": 0.000, "leadacid_qn": 0.000, "leadacid_q10": 0.000, "leadacid_q20": 0.000, "voltage_choice": 0, "Vnom_default": 3.600, "resistance": 0.000, "Vfull": 4.100, "Vexp": 4.050, "Vnom": 3.400, "Vcut": 2.706, "Qfull": 2.250, "Qexp": 0.040, "Qnom": 2.000, "C_rate": 0.200, "mass": 507.000, "surface_area": 2.018, "Cp": 1004.000, "h": 20.000, "cap_vs_temp": [ [ -10, 60 ], [ 0, 80 ], [ 25, 1E+2 ], [ 40, 1E+2 ] ], "option": 1, "T_room_init": 20, "life_model": 0, "cycling_matrix": [ [ 20, 0, 1E+2 ], [ 20, 5E+3, 80 ], [ 20, 1E+4, 60 ], [ 80, 0, 1E+2 ], [ 80, 1E+3, 80 ], [ 80, 2E+3, 60 ] ], "calendar_choice": 1, "calendar_q0": 1.020, "calendar_a": 0.003, "calendar_b": -7280.000, "calendar_c": 930.000, "calendar_matrix": [ [ -3.1E+231 ] ], "loss_choice": 0, "monthly_charge_loss": [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 ], "monthly_discharge_loss": [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 ], "monthly_idle_loss": [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 ], "schedule_loss": [], "replacement_option": 0, "replacement_capacity": 0.000, "replacement_schedule": [], "replacement_schedule_percent": [], "analysis_period": 1, "load_escalation": [0]})";
        data = json_to_ssc_data(params_str.c_str());
        ssc_data_set_number(data, "dt_hr", dt_hour);
        mod = ssc_module_create("battery_stateful");
        EXPECT_TRUE(ssc_stateful_module_setup(mod, data));
    }

    void TearDown() override {
        ssc_data_free(data);
        if (mod != NULL) {
            ssc_module_free(mod);
        }
    }

};


#endif //SAM_SIMULATION_CORE_CMOD_BATTERY_EQNS_TEST_H
