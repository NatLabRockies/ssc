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
#include "cmod_geothermal_test.h"

TEST_F(CMGeothermal, SingleOwnerDefault_cmod_geothermal) {
	int geo_errors = run_module(data, "geothermal");
	ASSERT_EQ(geo_errors, 0);
	int grid_errors = run_module(data, "grid");
	EXPECT_EQ(grid_errors, 0);
	int singleowner_errors = run_module(data, "singleowner");
	EXPECT_EQ(singleowner_errors, 0);

	if (!geo_errors)	//(!=geothermal_errors) == True;
	{
		ssc_number_t annual_energy, eff_secondlaw;
		ssc_data_get_number(data, "annual_energy", &annual_energy);
		ssc_data_get_number(data, "eff_secondlaw", &eff_secondlaw);
        EXPECT_NEAR(annual_energy, 261301614.984375, 0.1);
        EXPECT_GE(eff_secondlaw, 0);
	}

}
