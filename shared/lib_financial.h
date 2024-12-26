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

#ifndef __lib_financial_h
#define __lib_financial_h

#include <vector>
#include <sstream>

#ifndef WIN32
#include <float.h>
#endif


namespace libfin {
    double min(double a, double b);
    double max(double a, double b);
    long round_irs(double number);
    double irr(const std::vector<double>& cf, int count, double initial_guess = -2, double tolerance = 1e-6, int max_iterations = 100);
    double npv(const std::vector<double>& cf, int nyears, double rate);
    double payback(const std::vector<double> &CumulativePayback, const std::vector<double> &Payback, int Count);
    double ppmt(double rate, double per, double nper, double pv, double fv, int type);
    double payback(const std::vector<double>& CumulativePayback, const std::vector<double>& Payback, int Count);
}

// functions used by libfin::ppmpt()
double pow1pm1(double x, double y);
double pow1p(double x, double y);
double fvifa(double rate, double nper);
double pvif(double rate, double nper);
double pmt(double rate, double nper, double pv, double fv, int type);
double ipmt(double rate, double per, double nper, double pv, double fv, int type);
double irr_calc(const std::vector<double>& cf_vector, int count, double initial_guess, double tolerance, int max_iterations, double scale_factor, int& number_of_iterations, double& residual);

// functions used by libfin::irr()
bool is_valid_iter_bound(double estimated_return_rate);
double irr_poly_sum(double estimated_return_rate, const std::vector<double>& cf_vector, int count);
double irr_derivative_sum(double estimated_return_rate, const std::vector<double>& cf_vector, int count);
double irr_scale_factor(const std::vector<double>& cf_vector_unscaled, int count);
bool is_valid_irr(const std::vector<double>& cf_vector, int count, double residual, double tolerance, int number_of_iterations, int max_iterations, double calculated_irr, double scale_factor);

#endif
