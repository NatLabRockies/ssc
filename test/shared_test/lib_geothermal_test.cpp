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


#include "lib_geothermal_test.h"



TEST_F(GeothermalPlantAnalyzer, TestBinaryPlant_lib_geothermal)
{
    GeothermalPlantAnalyzer::TearDown(); // delete first setup instance to prevent memory leaks (setup and teardown called at start and end of test)
	conversion_type = 0;
	GeothermalPlantAnalyzer::SetUp(); // uses values set (conversion_type)

	EXPECT_NEAR(geoPlant_outputs.max_secondlaw, 0.4, 0.2);
	EXPECT_NEAR(geoPlant_outputs.md_GrossPlantOutputMW, 34.004, 3);
	EXPECT_NEAR(geoPlant_outputs.GF_flowrate, 5002143, 200000);
}


TEST_F(GeothermalPlantAnalyzer, TestFlashPlant_lib_geothermal) {
    GeothermalPlantAnalyzer::TearDown(); // delete first setup instance to prevent memory leaks (setup and teardown called at start and end of test)
	conversion_type = 1;
	GeothermalPlantAnalyzer::SetUp(); // uses values set (conversion_type)

	EXPECT_EQ(geoPlant_outputs.flash_count, 2);	//Dual Flash (Constrained) Plant Type
	EXPECT_NEAR(geoPlant_outputs.md_GrossPlantOutputMW, 32.609, 1);	//Expected value of 33.978 taken from GETEM
	EXPECT_NEAR(geoPlant_outputs.max_secondlaw, 0.5, 0.3);
}
