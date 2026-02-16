/*
BSD 3-Clause License

Copyright Alliance for Energy Innovation, LLC. See also https://github.com/NREL/ssc/blob/develop/LICENSE


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



#ifndef __WINDPOWER_TEST__
#define __WINDPOWER_TEST__

#include <gtest/gtest.h>

#include "../ssc/core.h"
#include "vartab.h"
#include "../ssc/common.h"
#include "../ssc/cmod_windpower_eqns.h"
#include "../input_cases/weather_inputs.h"
#include "../input_cases/windpower_cases.h"


/**
 * CMWindPower tests the cmod_windpower using wind resource files and data arrays. SetUp() creates default case,
 * which can be modified within each individual test before running compute() and tests.
 */
class CMWindPowerIntegration : public ::testing::Test{
protected:
	ssc_data_t data;
	double e = 1000;

	bool compute(bool errorNotExpected = true);
	void SetUp(){
		data = ssc_data_create();
		int errors = windpower_nofinancial_testfile(data);
		EXPECT_FALSE(errors);
	}
	void TearDown(){
		ssc_data_free(data);
		data = nullptr;
	}
};

bool CMWindPowerIntegration::compute(bool errorNotExpected){
	ssc_module_t module = ssc_module_create("windpower");
	if (NULL == module)
	{
		if (errorNotExpected) printf("error: could not create 'windpower' module.");
		return false;
	}
	if (ssc_module_exec(module, data) == 0)
	{
		if (errorNotExpected) printf("error during simulation.");
		ssc_module_free(module);
		return false;
	}
	ssc_module_free(module);
	return true;
}


#endif
