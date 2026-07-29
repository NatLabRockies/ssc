/*
BSD 3-Clause License

Copyright Alliance for Energy Innovation, LLC. See also https://github.com/NatLabRockies/ssc/blob/develop/LICENSE


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

#include "cmod_pvyield_test.h"
#include "../input_cases/pvyield_cases.h"
#include "../input_cases/weather_inputs.h"

using std::cout;
using std::endl;

/// Test PVSAMv1 with inputs from PVYield
TEST_F(CMPvYieldTimo, DefaultTimoModel_cmod_pvsamv1)
{
    pvyield_no_financial_meteo(data);

    int pvsam_errors = pvyield_test(data);
    EXPECT_FALSE(pvsam_errors);
    //printf("ssc version %d build information %s", ssc_version(), ssc_build_info());

    if (!pvsam_errors)
    {
        ssc_number_t annual_energy;
        ssc_data_get_number(data, "annual_energy", &annual_energy);
        EXPECT_NEAR(annual_energy, 7382053, 7380478e-4) << "Annual energy.";

        ssc_number_t capacity_factor;
        ssc_data_get_number(data, "capacity_factor", &capacity_factor);
        EXPECT_NEAR(capacity_factor, 20.219496, m_error_tolerance_lo) << "Capacity factor";

        ssc_number_t kwh_per_kw;
        ssc_data_get_number(data, "kwh_per_kw", &kwh_per_kw);
        EXPECT_NEAR(kwh_per_kw, 1764.669, m_error_tolerance_hi) << "Energy yield";

        ssc_number_t performance_ratio;
        ssc_data_get_number(data, "performance_ratio", &performance_ratio);
        EXPECT_NEAR(performance_ratio, 0.84380863, m_error_tolerance_lo) << "Energy yield";
    }
}


TEST_F(CMPvYieldTimo, Bifacial_cmod_pvsamv1)
{
    double desired_annual_energy = 345965378.92463976;
    pvyield_bifacial_case(data);
    int pvsam_errors = run_module(data, "pvsamv1");
    EXPECT_FALSE(pvsam_errors);

    ssc_number_t annual_energy_6;
    ssc_data_get_number(data, "annual_energy", &annual_energy_6);
    EXPECT_NEAR(annual_energy_6, desired_annual_energy, desired_annual_energy * m_error_tolerance_lo / 100. );

    ssc_number_t annual_dc_nominal;
    ssc_data_get_number(data, "annual_dc_nominal", &annual_dc_nominal);
    EXPECT_GT(annual_dc_nominal, desired_annual_energy);  // make sure dc_nominal isn't nan/negative/nonsense

    // Check to see if increasing bifaciality increase energy
    ssc_data_set_number(data, "mlm_bifaciality", 0.8);
    pvsam_errors = run_module(data, "pvsamv1");
    EXPECT_FALSE(pvsam_errors);
    ssc_number_t annual_energy_8;
    ssc_data_get_number(data, "annual_energy", &annual_energy_8);
    EXPECT_GT(annual_energy_8, desired_annual_energy + 1);
}


/// Test PVSAMv1 with inputs from PVYield and user support 80603 with meteo weather file
TEST_F(CMPvYieldTimo, TimoModel80603_meteo_cmod_pvsamv1)
{
    // first set of results for Phoenix and second set for meteo weather file.
    pvyield_user_support_80603_meteo(data);

    int pvsam_errors = pvyield_test_user_support_80603_meteo(data);
    EXPECT_FALSE(pvsam_errors);
    //printf("ssc version %d build information %s", ssc_version(), ssc_build_info());

    if (!pvsam_errors)
    {
        ssc_number_t annual_energy;
        ssc_data_get_number(data, "annual_energy", &annual_energy);
        EXPECT_NEAR(annual_energy, 7449046, 7441557e-4) << "Annual energy.";

        ssc_number_t capacity_factor;
        ssc_data_get_number(data, "capacity_factor", &capacity_factor);
        EXPECT_NEAR(capacity_factor, 20.399, m_error_tolerance_lo) << "Capacity factor";

        ssc_number_t kwh_per_kw;
        ssc_data_get_number(data, "kwh_per_kw", &kwh_per_kw);
        EXPECT_NEAR(kwh_per_kw, 1781.06, m_error_tolerance_hi) << "Energy yield";

        ssc_number_t performance_ratio;
        ssc_data_get_number(data, "performance_ratio", &performance_ratio);
        EXPECT_NEAR(performance_ratio, 0.84380863, m_error_tolerance_lo) << "Energy yield";
    }
}

/// Test PVSAMv1 with inputs from PVYield and user support 80603 AZ weather file
TEST_F(CMPvYieldTimo, TimoModel80603_AZ_cmod_pvsamv1)
{
    // first set of results for Phoenix and second set for meteo weather file.
    pvyield_user_support_80603_AZ(data);

    int pvsam_errors = pvyield_test_user_support_80603_AZ(data);
    EXPECT_FALSE(pvsam_errors);
    //printf("ssc version %d build information %s", ssc_version(), ssc_build_info());

    if (!pvsam_errors)
    {
        ssc_number_t annual_energy;
        ssc_data_get_number(data, "annual_energy", &annual_energy);
        EXPECT_NEAR(annual_energy, 8205905, 8198434e-4) << "Annual energy.";

        ssc_number_t capacity_factor;
        ssc_data_get_number(data, "capacity_factor", &capacity_factor);
        EXPECT_NEAR(capacity_factor, 22.456, m_error_tolerance_lo) << "Capacity factor";

        ssc_number_t kwh_per_kw;
        ssc_data_get_number(data, "kwh_per_kw", &kwh_per_kw);
        EXPECT_NEAR(kwh_per_kw, 1962.02, m_error_tolerance_hi) << "Energy yield";

        ssc_number_t performance_ratio;
        ssc_data_get_number(data, "performance_ratio", &performance_ratio);
        EXPECT_NEAR(performance_ratio, 0.8147345055, m_error_tolerance_lo) << "Energy yield";
    }
}
