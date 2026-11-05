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


#include "gtest/gtest.h"

#include "cmod_swh_test.h"
#include "../ssc/cmod_swh.h"
#include "input_cases/weather_inputs.h"

TEST_F(CM_SWH, ResidentialDefault_cmod_swh) {

    int swh_errors = run_module(data, "swh");
    ASSERT_EQ(swh_errors, 0);

    ssc_number_t annual_energy;
    ssc_data_get_number(data, "annual_energy", &annual_energy);
    EXPECT_NEAR(annual_energy, 2359.2, 0.1);

}

TEST_F(CM_SWH, ResidentialDefaultUsingData_cmod_swh) {
    auto weather_data = create_weatherdata_array(8760);
    ssc_data_unassign(data, "solar_resource_file");
    ssc_data_set_table(data, "solar_resource_data", weather_data);

    int swh_errors = run_module(data, "swh");
    ASSERT_EQ(swh_errors, 0);

    ssc_number_t annual_energy;
    ssc_data_get_number(data, "annual_energy", &annual_energy);
    EXPECT_NEAR(annual_energy, 1375, 1);

    free_weatherdata_array(weather_data);
}

// ssc issues 1029 and 1030
TEST(SWH_IncidenceAngleModifier, CoverModifier_cmod_swh) {
    double iam = 0.1;
    EXPECT_NEAR(swh_incidence_angle_modifier(0, iam), 1, 1e-12);
    EXPECT_NEAR(swh_incidence_angle_modifier(45, iam), 1 - iam * (sqrt(2.) - 1), 1e-12);
    // continuous at 60 degrees, then falls linearly to zero at 90 degrees
    EXPECT_NEAR(swh_incidence_angle_modifier(60, iam), 1 - iam, 1e-12);
    EXPECT_NEAR(swh_incidence_angle_modifier(60.001, iam), 1 - iam, 1e-4);
    EXPECT_NEAR(swh_incidence_angle_modifier(75, iam), (1 - iam) / 2, 1e-12);
    EXPECT_NEAR(swh_incidence_angle_modifier(90, iam), 0, 1e-12);
    EXPECT_EQ(swh_incidence_angle_modifier(95, iam), 0);
}

TEST(SWH_IncidenceAngleModifier, EffectiveIncidenceAngles_cmod_swh) {
    // D&B eqns 5.4.1 and 5.4.2 (Figure 5.4.1), in degrees
    double diffuse, ground;
    swh_effective_incidence_angles(0, diffuse, ground);
    EXPECT_NEAR(diffuse, 59.7, 1e-9);
    EXPECT_NEAR(ground, 90, 1e-9);
    swh_effective_incidence_angles(45, diffuse, ground);
    EXPECT_NEAR(diffuse, 56.49, 0.01);
    EXPECT_NEAR(ground, 69.41, 0.01);
    swh_effective_incidence_angles(90, diffuse, ground);
    EXPECT_NEAR(diffuse, 59.33, 0.01);
    EXPECT_NEAR(ground, 59.72, 0.01);
}

TEST_F(CM_SWH, TransmittedIrradiance_cmod_swh) {
    int swh_errors = run_module(data, "swh");
    ASSERT_EQ(swh_errors, 0);

    int n = 0;
    ssc_number_t* incident = ssc_data_get_array(data, "I_incident", &n);
    ssc_number_t* transmitted = ssc_data_get_array(data, "I_transmitted", nullptr);
    ASSERT_EQ(n, 8760);
    double total_incident = 0, total_transmitted = 0;
    for (int i = 0; i < n; i++) {
        EXPECT_LE(transmitted[i], incident[i] + 1e-6) << "hour " << i;
        total_incident += incident[i];
        total_transmitted += transmitted[i];
    }
    // was 0.848: the cover modifier dropped all beam irradiance beyond 60 degrees and all ground reflected irradiance
    EXPECT_NEAR(total_transmitted / total_incident, 0.8775, 0.0005);
}
