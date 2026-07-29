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


#include <string>
#include <cmath>
#include <gtest/gtest.h>

#include "vartab_test.h"
#include "common.h"

TEST_F(vartab_test, test_resize) {
    // Test array first
    std::string arr_name = "lifetime_array";
    var->allocate(arr_name, 25);
    size_t count = 0;
    ssc_number_t* arr = var->as_array(arr_name, &count);
    arr[0] = 1.0;
    arr[24] = 25.0;
    ASSERT_EQ(count, 25);
    ASSERT_NEAR(arr[0], 1.0, 0.001);
    ASSERT_NEAR(arr[24], 25.0, 0.001);

    arr = var->resize_array(arr_name, 30);
    arr = var->as_array(arr_name, &count);
    ASSERT_NEAR(arr[0], 1.0, 0.001);
    ASSERT_NEAR(arr[24], 25.0, 0.001);
    ASSERT_NEAR(arr[29], 0.0, 0.001);
    ASSERT_EQ(count, 30);

    std::string mat_name = "matrix";
    var->allocate_matrix(mat_name, 1, 1);
    size_t nrows = 0;
    size_t ncols = 0;

    ssc_number_t* p_mat = var->as_matrix(mat_name, &nrows, &ncols);
    p_mat[0] = 1.0;
    var->resize_matrix(mat_name, 2, 3);
    util::matrix_t<double> mat = var->as_matrix(mat_name);
    ASSERT_EQ(mat.nrows(), 2);
    ASSERT_EQ(mat.ncols(), 3);
    ASSERT_NEAR(mat.at(0, 0), 1.0, 0.001);
}
