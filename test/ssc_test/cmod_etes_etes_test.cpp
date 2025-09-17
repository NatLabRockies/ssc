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


#include <gtest/gtest.h>
#include "etes_etes_defaults.h"
#include "csp_common_test.h"
#include "vs_google_test_explorer_namespace.h"

namespace etes_etes_test {}
using namespace etes_etes_test;

//========Tests===================================================================================
NAMESPACE_TEST(etes_etes_test, EtesEtesCmod, Default_NoFinancial)
{
    ssc_data_t defaults = etes_etes_defaults();
    CmodUnderTest etes_system = CmodUnderTest("etes_electric_resistance", defaults);

    etes_system.SetInput("is_dispatch", 0);
    int errors = etes_system.RunModule();
    double ann_energy = etes_system.GetOutput("annual_energy");
    EXPECT_FALSE(errors);
    if (!errors) {
        EXPECT_NEAR_FRAC(std::abs(etes_system.GetOutput("annual_energy")), std::abs(-739728801.), kErrorToleranceHi);
    }

    etes_system.SetInput("is_dispatch", 1);
    errors = etes_system.RunModule();
    ann_energy = etes_system.GetOutput("annual_energy");
    EXPECT_FALSE(errors);
    if (!errors) {
        EXPECT_NEAR_FRAC(std::abs(etes_system.GetOutput("annual_energy")), std::abs(-497554825), kErrorToleranceHi);
    }

    //if (!errors) {
    //    EXPECT_NEAR_FRAC(power_tower.GetOutput("annual_energy"), 602247767, kErrorToleranceHi);
    //    EXPECT_NEAR_FRAC(power_tower.GetOutput("land_area_base_calc"), 1847, kErrorToleranceHi);
    //    EXPECT_NEAR_FRAC(power_tower.GetOutput("capacity_factor"), 67.45, kErrorToleranceHi);
    //    EXPECT_NEAR_FRAC(power_tower.GetOutput("annual_W_cycle_gross"), 670611403, kErrorToleranceHi);
    //    EXPECT_NEAR_FRAC(power_tower.GetOutput("kwh_per_kw"), 5908, kErrorToleranceHi);
    //    EXPECT_NEAR_FRAC(power_tower.GetOutput("conversion_factor"), 89.558, kErrorToleranceHi);
    //    EXPECT_NEAR_FRAC(power_tower.GetOutput("N_hel_calc"), 8790, kErrorToleranceHi);
    //    EXPECT_NEAR_FRAC(power_tower.GetOutput("rec_height"), 21.60, kErrorToleranceHi);
    //    EXPECT_NEAR_FRAC(power_tower.GetOutput("A_sf"), 1269054, kErrorToleranceHi);
    //    EXPECT_NEAR_FRAC(power_tower.GetOutput("D_rec"), 17.65, kErrorToleranceHi);
    //    EXPECT_NEAR_FRAC(power_tower.GetOutput("annual_total_water_use"), 98573, kErrorToleranceHi);
    //    EXPECT_NEAR_FRAC(power_tower.GetOutput("total_land_area"), 1892, kErrorToleranceHi);
    //    EXPECT_NEAR_FRAC(power_tower.GetOutput("h_tower"), 193.5, kErrorToleranceHi);
    //}

}
