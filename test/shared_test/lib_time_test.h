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


#ifndef __LIB_TIME_TEST_H__
#define __LIB_TIME_TEST_H__

#include <math.h>

#include "lib_util.h"
#include <gtest/gtest.h>
#include <vector>

class libTimeTest_lib_time : public ::testing::Test
{
protected:

	bool is_lifetime;
	size_t n_years;
	size_t increment;
	std::vector<float> lifetime60min;
	std::vector<float> lifetime30min;
	std::vector<float> singleyear60min;
	std::vector<float> singleyear30min;
    std::vector<float> scaleFactors;

	size_t * sched;
	util::matrix_t<size_t> schedule;
	std::vector<double> sched_values = { 0.1, 0.0, 0.3 };
	double multiplier = 2.0;
    double interpolation_factor = 1.0;

    size_t start_day_of_year = 0;

	void SetUp()
	{
		is_lifetime = true;
		n_years = 25;
		increment = 500;

		// doesn't actually matter what is in here, just the size
		lifetime60min.reserve(n_years * util::hours_per_year);
		for (size_t i = 0; i < n_years * util::hours_per_year; i++) {
			lifetime60min.push_back(0);
		}

		// doesn't actually matter what is in here, just the size
		lifetime30min.reserve(2 * n_years * util::hours_per_year);
		for (size_t i = 0; i < n_years * 2 * util::hours_per_year; i++) {
			lifetime30min.push_back(0);
		}

		singleyear60min.reserve(util::hours_per_year);
		for (size_t i = 0; i <  util::hours_per_year; i++) {
			singleyear60min.push_back(100 * sin(i));
		}

		singleyear30min.reserve(util::hours_per_year * 2);
		for (size_t i = 0; i < util::hours_per_year * 2; i++) {
			singleyear30min.push_back(100 * sin(i));
		}

        scaleFactors.reserve(n_years);
        for (size_t i = 0; i < n_years; i++)
        {
            scaleFactors.push_back(1.0);
        }

		sched = new size_t[24 * 12];

		size_t i = 0;
		for (size_t m = 0; m < 12; m++) {
			for (size_t h = 0; h < 24; h++) {
				sched[i] = 1;
				if (h > 11 && h < 19) {
					sched[i] = 3;
				}
				i++;
			}
		}
		schedule.assign(sched, 12, 24);

	}
    
    void TearDown() {
        if (sched)
            delete [] sched;
    }
};


#endif // !__LIB_TIME_TEST_H__
