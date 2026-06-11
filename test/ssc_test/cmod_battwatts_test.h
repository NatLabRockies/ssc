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



#ifndef SAM_SIMULATION_CORE_CMOD_BATTWATTS_TEST_H
#define SAM_SIMULATION_CORE_CMOD_BATTWATTS_TEST_H

#include <gtest/gtest.h>

#include "../input_cases/sscapi.h"
#include "vartab.h"


class CMBattwatts_cmod_battwatts : public ::testing::Test {

public:

    var_table data;
    std::vector<double> ac;
    std::vector<double> load;
    std::vector<double> crit_load;
    std::vector<double> dispatch;

    double m_error_tolerance_hi = 1.0;
    double m_error_tolerance_lo = 0.1;

    void CreateData(size_t nyears) {
        for (size_t i = 0; i < 8760 * nyears; i++){
            size_t hr = i % 24;
            if (hr > 7 && hr < 18 )
                ac.push_back(1.); // Watts
            else
                ac.push_back(0.);
            load.push_back(0.5); // kW
            crit_load.push_back(0.25); // kW
            if (i < 8760) {
                dispatch.push_back(-0.1); // kW - keep the battery fully charged
            }
        }


        data.assign("system_use_lifetime_output", nyears > 1);
        data.assign("analysis_period", (int)nyears);
        data.assign("batt_simple_enable", 1);
        data.assign("batt_simple_kwh", 10);
        data.assign("batt_simple_kw", 5);
        data.assign("batt_simple_chemistry", 1);
        data.assign("batt_simple_dispatch", 2);
        data.assign("batt_custom_dispatch", dispatch);
        data.assign("batt_simple_meter_position", 0);
        data.assign("ac", ac);
        data.assign("load", load);
        data.assign("crit_load", crit_load);
        data.assign("inverter_model", 0);
        data.assign("inverter_efficiency", 96);
        data.assign("run_resiliency_calcs", 1);
    }
};

#endif //SAM_SIMULATION_CORE_CMOD_BATTWATTS_TEST_H
