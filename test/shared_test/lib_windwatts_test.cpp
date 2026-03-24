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


#include <gtest/gtest.h>

#include <iostream>
#include <vector>

#include <lib_windwatts.h>
#include <lib_windwakemodel.h>
#include "lib_windwakemodel_test.h"


/**
 * windPowerCalculatorTest requires an initialized windTurbine and wakeModel, and XCoords & YCoords.
 * SetUp() allocates the vectors for input and output variables for windPowerUsingResource.
 */

class windPowerCalculatorTest : public ::testing::Test{
protected:
	windPowerCalculator wpc;
	windTurbine wt;
	int nTurbines;
	// weather data
	double windSpeedData, windDirData, pressureData, tempData;

	 //farm data
	double farmPower, farmPowerGross;
	std::vector<double> power, thrust, eff, windSpeed;
	std::vector<double> turbulenceCoeff, distX, distY, distDownwind, distCrosswind;

public:
	double e = 1000;
	void SetUp(){
		// allocate arrays
		nTurbines = 3;
		distDownwind.resize(nTurbines);
		distCrosswind.resize(nTurbines);
		thrust.resize(nTurbines, 0.47669);
		power.resize(nTurbines, 1190);
		eff.resize(nTurbines, 0);
		windSpeed.resize(nTurbines);
		turbulenceCoeff.resize(nTurbines);
		distX = { 0, 5, 10 };
		distY = { 0, 5, 10 };
		distDownwind.resize(nTurbines);
		distCrosswind.resize(nTurbines);

		// initialize values & classes
		for (int i = 0; i < nTurbines; i++){
			windSpeed[i] = 10.;
		}
		createDefaultTurbine(&wt);

		wpc.nTurbines = nTurbines;
		wpc.turbulenceIntensity = 1.0 / 7.0;
		wpc.windTurb = &wt;
		wpc.XCoords = distX;
		wpc.YCoords = distY;
	}
};

TEST_F(windPowerCalculatorTest, windPowerUsingResource_lib_windwatts){
	// weather inputs
	windSpeedData = 10.;
	windDirData = 180;
	tempData = 25;
	pressureData = 1.0;

	std::shared_ptr<fakeWakeModel> fakeWM(new fakeWakeModel());
	wpc.InitializeModel(fakeWM);
	int run = wpc.windPowerUsingResource(windSpeedData, windDirData, pressureData, tempData, &farmPower,
                                         &farmPowerGross, &power[0], &thrust[0],
                                         &eff[0], &windSpeed[0], &turbulenceCoeff[0], &distDownwind[0],
                                         &distCrosswind[0]); // runs method we want to test
	EXPECT_EQ(run, 3);
}

TEST_F(windPowerCalculatorTest, windPowerUsingWeibull_lib_windwatts){
	// weather inputs
	windSpeedData = 10.;
	windDirData = 180;
	tempData = 25;
	pressureData = 1.0;
	// weibull data
	double weibullK = 2.;
	double avgSpeed = 7.25;
	double refHeight = 50.;
	std::vector<double> energy(wpc.windTurb->powerCurveArrayLength);

	double energyTotal = wpc.windPowerUsingWeibull(weibullK, avgSpeed, refHeight, &energy[0]); // runs method we want to test
	EXPECT_NEAR(energyTotal, 5639180, e);
}

TEST_F(windPowerCalculatorTest, windPowerUsingDistribution_lib_windwatts){
    // mimic a weibull with k factor 2 and avg speed 7.25 for comparison -> scale param : 8.181
    std::vector<std::vector<double>> dst = {{1.5, 180, .12583},
                                            {5, 180, .3933},
                                            {8, 180, .18276},
                                            {10, 180, .1341},
                                            {13.5, 180, .14217},
                                            {19, 180, .0211}};
    std::shared_ptr<wakeModelBase> wakeModel = std::make_shared<fakeWakeModel>();
    wpc.InitializeModel(wakeModel);
    wpc.windPowerUsingDistribution(dst, &farmPower, &farmPowerGross);
    EXPECT_NEAR(farmPower, 15075000, e);
    EXPECT_NEAR(farmPowerGross, 15075000, e);
}

TEST_F(windPowerCalculatorTest, windPowerUsingDistribution_1turbine_lib_windwatts){
    // mimic a weibull with k factor 2 and avg speed 7.25 for comparison -> scale param : 8.181
    std::vector<std::vector<double>> dst = {{1.5, 180, .12583},
                                            {5, 180, .3933},
                                            {8, 180, .18276},
                                            {10, 180, .1341},
                                            {13.5, 180, .14217},
                                            {19, 180, .0211}};
    std::shared_ptr<wakeModelBase> wakeModel = std::make_shared<fakeWakeModel>();

    wpc.nTurbines = 1;
    wpc.XCoords = {0.};
    wpc.YCoords = {0.};

    wpc.InitializeModel(wakeModel);
    wpc.windPowerUsingDistribution(dst, &farmPower, &farmPowerGross);
    EXPECT_NEAR(farmPower, 15075000. / 3, e);
    EXPECT_NEAR(farmPowerGross, 15075000. / 3, e);
}
