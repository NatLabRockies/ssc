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


#include <gtest/gtest.h>

#include <iostream>
#include <vector>

#include "core.h"
#include <lib_windfile.h>
#include "cmod_windpower.h"
#include "../input_cases/weather_inputs.h"

/**
 * Tests windfile's interpolation of measurement height's pres, tmp, speed, & dir data points to required hub height.
 */

class windDataProviderCalculatorTest : public ::testing::Test {
protected:
	winddata_provider* windDataProvider;

public: 
	double e = .1;
	void SetUp() {}
	void TearDown() {
		if (windDataProvider) delete windDataProvider;
	}
};

TEST_F(windDataProviderCalculatorTest, FindClosestUsingData_lib_windfile_test) {
	// measurement heights: 80, 90
    auto table = create_winddata_array(1, 2);
    var_data windresourcedata = var_data(*table);

    windDataProvider = new winddata(&windresourcedata);

	//// Case 1: hubheight: 85, can interpolate
	double pres, temp, spd, dir, heightOfClosestMeasuredSpd, heightOfClosestMeasuredDir;
	windDataProvider->read(85, &spd, &dir, &temp, &pres, &heightOfClosestMeasuredSpd, &heightOfClosestMeasuredDir, true);
	EXPECT_NEAR(pres, 0.975, e) << "case 1: hub height can be interpolated.";
	EXPECT_NEAR(temp, 52.5, e) << "case 1: hub height can be interpolated.";
	EXPECT_NEAR(spd, 2.5, e) << "case 1: hub height can be interpolated.";
	EXPECT_NEAR(dir, 190, e) << "case 1: hub height can be interpolated.";
	EXPECT_NEAR(heightOfClosestMeasuredSpd, 85, e) << "case 1: hub height can be interpolated.";

	//// Case 2: hubheight: 95, cannot interpolate, gives closest
	windDataProvider->read(95, &spd, &dir, &temp, &pres, &heightOfClosestMeasuredSpd, &heightOfClosestMeasuredDir, true);
	EXPECT_NEAR(pres, 1.0, e) << "case 2";
	EXPECT_NEAR(temp, 55, e) << "case 2";
	EXPECT_NEAR(spd, 5, e) << "case 2";
	EXPECT_NEAR(dir, 200, e) << "case 2";
	EXPECT_NEAR(heightOfClosestMeasuredSpd, 90, e) << "case 2";

    free_winddata_array(table);
}
