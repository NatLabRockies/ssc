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


#include <cmath>
#include <limits>
#include "lib_financial.h"

#ifndef WIN32
#include <float.h>
#endif

// find minimum cash flow value when some values are NaN, e.g., DSCR is NaN after end of repayment period
// https://stackoverflow.com/questions/72296440/how-to-prevent-stdmin-and-max-to-return-nan-if-the-first-element-of-the-array
double libfin::min(double a, double b)
{
    if ((a != a) || (b != b))
        return 0;
    else
        return (a < b) ? a : b;
}

// find maximum cash flow value when some values are NaN, e.g., IRR is NaN in Year zero
// https://stackoverflow.com/questions/72296440/how-to-prevent-stdmin-and-max-to-return-nan-if-the-first-element-of-the-array
double libfin::max(double a, double b)
{
    if ((a != a) || (b != b))
        return 0;
    else
        return (a > b) ? a : b;
}


// irs rounding rules https://www.irs.gov/pub/irs-pdf/n1036.pdf
// original code from http://www.codeproject.com/Articles/58289/C-Round-Function.aspx
long libfin::round_irs(double number)
{
    return (number >= 0) ? (long)(number + 0.5) : (long)(number - 0.5);
}

// irr functions
// orignally ported from http://code.google.com/p/irr-newtonraphson-calculator/

bool is_valid_iter_bound(double estimated_return_rate)
{
    return estimated_return_rate != -1 && (estimated_return_rate < std::numeric_limits<int>::max()) && (estimated_return_rate > std::numeric_limits<int>::min());
}

double irr_poly_sum(double estimated_return_rate, const std::vector<double>& cf_vector, int count)
{
    double sum_of_polynomial = 0;
    if (is_valid_iter_bound(estimated_return_rate))
    {
        for (int j = 0; j <= count; j++)
        {
            double val = (pow((1 + estimated_return_rate), j));
            if (val != 0.0)
                sum_of_polynomial += cf_vector.at(j) / val;
            else
                break;
        }
    }
    return sum_of_polynomial;
}

double irr_derivative_sum(double estimated_return_rate, const std::vector<double>& cf_vector, int count)
{
    double sum_of_derivative = 0;
    if (is_valid_iter_bound(estimated_return_rate))
        for (int i = 1; i <= count; i++)
        {
            sum_of_derivative += cf_vector.at(i) * (i) / pow((1 + estimated_return_rate), i + 1);
        }
    return sum_of_derivative * -1;
}

double irr_scale_factor(const std::vector<double>& cf_vector_unscaled, int count)
{
    if (count < 1) return 1.0;
    int i = 0;
    double max = std::abs(cf_vector_unscaled.at(0));
    for (i = 0; i <= count; i++)
        if (std::abs(cf_vector_unscaled.at(i)) > max) max = std::abs(cf_vector_unscaled.at(i));
    return (max > 0 ? max : 1);
}

bool is_valid_irr(const std::vector<double>& cf_vector, int count, double residual, double tolerance, int number_of_iterations, int max_iterations, double calculated_irr, double scale_factor)
{
    double npv_of_irr = libfin::npv(cf_vector, count, calculated_irr) + cf_vector.at(0);
    double npv_of_irr_plus_delta = libfin::npv(cf_vector, count, calculated_irr + 0.001) + cf_vector.at(0);
    bool is_valid = ((number_of_iterations < max_iterations) && (std::abs(residual) < tolerance) && (npv_of_irr > npv_of_irr_plus_delta) && (std::abs(npv_of_irr / scale_factor) < tolerance));
    //if (!is_valid)
    //{
    //std::stringstream outm;
    //outm <<  "cf_line=" << cf_line << "count=" << count << "residual=" << residual << "number_of_iterations=" << number_of_iterations << "calculated_irr=" << calculated_irr
    //	<< "npv of irr=" << npv_of_irr << "npv of irr plus delta=" << npv_of_irr_plus_delta;
    //log( outm.str() );
    //}
    return is_valid;
}

double irr_calc(const std::vector<double>& cf_vector, int count, double initial_guess, double tolerance, int max_iterations, double scale_factor, int& number_of_iterations, double& residual) {
    double calculated_irr = std::numeric_limits<double>::quiet_NaN();
    double deriv_sum = irr_derivative_sum(initial_guess, cf_vector, count);
    if (deriv_sum != 0.0)
        calculated_irr = initial_guess - irr_poly_sum(initial_guess, cf_vector, count) / deriv_sum;
    else
        return initial_guess;

    number_of_iterations++;

    residual = irr_poly_sum(calculated_irr, cf_vector, count) / scale_factor;

    while (!(std::abs(residual) <= tolerance) && (number_of_iterations < max_iterations))
    {
        deriv_sum = irr_derivative_sum(initial_guess, cf_vector, count);
        if (deriv_sum != 0.0)
            calculated_irr = calculated_irr - irr_poly_sum(calculated_irr, cf_vector, count) / deriv_sum;
        else
            break;

        number_of_iterations++;
        residual = irr_poly_sum(calculated_irr, cf_vector, count) / scale_factor;
    }
    return calculated_irr;
}

double libfin::irr(const std::vector<double>& cf_vector, int count, double initial_guess, double tolerance, int max_iterations)
{
    int number_of_iterations = 0;
    double calculated_irr = std::numeric_limits<double>::quiet_NaN();

    if (count < 1)
        return calculated_irr;

    // only possible for first value negative
    if ((cf_vector.at(0) <= 0))
    {
        // initial guess from http://zainco.blogspot.com/2008/08/internal-rate-of-return-using-newton.html
        if ((initial_guess < -1) && (count > 1)) // second order
        {
            if (cf_vector.at(0) != 0)
            {
                double b = 2.0 + cf_vector.at(1) / cf_vector.at(0);
                double c = 1.0 + cf_vector.at(1) / cf_vector.at(0) + cf_vector.at(2) / cf_vector.at(0);
                initial_guess = -0.5 * b - 0.5 * sqrt(b * b - 4.0 * c);
                if ((initial_guess <= 0) || (initial_guess >= 1)) initial_guess = -0.5 * b + 0.5 * sqrt(b * b - 4.0 * c);
            }
        }
        else if (initial_guess < 0) // first order
        {
            if (cf_vector.at(0) != 0) initial_guess = -(1.0 + cf_vector.at(1) / cf_vector.at(0));
        }

        double scale_factor = irr_scale_factor(cf_vector, count);
        double residual = DBL_MAX;

        calculated_irr = irr_calc(cf_vector, count, initial_guess, tolerance, max_iterations, scale_factor, number_of_iterations, residual);

        if (!is_valid_irr(cf_vector, count, residual, tolerance, number_of_iterations, max_iterations, calculated_irr, scale_factor)) // try 0.1 as initial guess
        {
            initial_guess = 0.1;
            number_of_iterations = 0;
            residual = 0;
            calculated_irr = irr_calc(cf_vector, count, initial_guess, tolerance, max_iterations, scale_factor, number_of_iterations, residual);
        }

        if (!is_valid_irr(cf_vector, count, residual, tolerance, number_of_iterations, max_iterations, calculated_irr, scale_factor)) // try -0.1 as initial guess
        {
            initial_guess = -0.1;
            number_of_iterations = 0;
            residual = 0;
            calculated_irr = irr_calc(cf_vector, count, initial_guess, tolerance, max_iterations, scale_factor, number_of_iterations, residual);
        }

        if (!is_valid_irr(cf_vector, count, residual, tolerance, number_of_iterations, max_iterations, calculated_irr, scale_factor)) // try 0 as initial guess
        {
            initial_guess = 0;
            number_of_iterations = 0;
            residual = 0;
            calculated_irr = irr_calc(cf_vector, count, initial_guess, tolerance, max_iterations, scale_factor, number_of_iterations, residual);
        }

        if (!is_valid_irr(cf_vector, count, residual, tolerance, number_of_iterations, max_iterations, calculated_irr, scale_factor)) // try 0.1 as initial guess
        {
            calculated_irr = std::numeric_limits<double>::quiet_NaN(); // did not converge
        }

    }
    return calculated_irr;
}

double libfin::npv(const std::vector<double>& cf_vector, int nyears, double rate)
{
    //if (rate == -1.0) throw general_error("cannot calculate NPV with discount rate equal to -1.0");
    double rr = 1.0;
    if (rate != -1.0) rr = 1.0 / (1.0 + rate);
    double result = 0;
    for (int i = nyears; i > 0; i--)
        result = rr * result + cf_vector.at(i);  //result = rr * result + cf.at(cf_line, i);

    return result * rr;
}

// Return payback in years of inputs streams
// Payback occures when cumulative stream is > 0
// Find exact payback by subtracting cumulative / payback
double libfin::payback(const std::vector<double> &cumulative_payback, const std::vector<double> &payback, int nyears)
{

    double dpb = std::numeric_limits<double>::quiet_NaN();
    bool bpb = false;
    int ipb = 0;
    int i = 1;
    while ((i<nyears) && (!bpb))
    {
        if (cumulative_payback[i] > 0)
	    {
            bpb = true;
            ipb = i;
	    }
	    i++;
    }

  if (bpb)
  {
	  if (payback[ipb] !=0)
	  {
		  dpb = ipb - cumulative_payback[ipb] / payback[ipb];
	  }
	  else
	  {
		  dpb = ipb;
	  }
  }

  return dpb;
}

// ppmpt and supporting functions
/* original code source http://www.linkedin.com/answers/technology/software-development/TCH_SFT/445353-4527099?browseCategory=TCH_SFT
updated link as of October 2024: https://corporatefinanceinstitute.com/resources/excel/ppmt-function/

Returns the payment on the principal for a given period for an investment based on periodic, constant payments and a constant interest rate.

Syntax

libfin::ppmt(rate,per,nper,pv,fv,type)

For a more complete description of the arguments in libfin::ppmt, see PV.

Rate   is the interest rate per period.

Per   specifies the period and must be in the range 1 to nper.

Nper   is the total number of payment periods in an annuity.

Pv   is the present value � the total amount that a series of future payments is worth now.

Fv   is the future value, or a cash balance you want to attain after the last payment is made. If fv is omitted, it is assumed to be 0 (zero), that is, the future value of a loan is 0.

Type   is the number 0 or 1 and indicates when payments are due.

Set type equal to If payments are due
0 or omitted At the end of the period
1 At the beginning of the period

Remark
Make sure that you are consistent about the units you use for specifying rate and nper. If you make monthly payments on a four-year loan at 12 percent annual interest, use 12%/12 for rate and 4*12 for nper. If you make annual payments on the same loan, use 12% for rate and 4 for nper.
*/
double pow1pm1(double x, double y)
{
	return (x <= -1) ? pow (1 + x, y) - 1 : exp(y * log(1.0 + x)) - 1;
}
double pow1p(double x, double y)
{
	return (std::abs(x) > 0.5) ? pow (1 + x, y) : exp (y * log(1.0 + x));
}
double fvifa(double rate, double nper)
{
	return (rate == 0) ? nper : pow1pm1 (rate, nper) / rate;
}

double pvif(double rate, double nper)
{
	return pow1p (rate, nper);
}

double pmt(double rate, double nper, double pv, double fv, int type)
{
	return ((-pv * pvif (rate, nper) - fv ) / ((1.0 + rate * type) * fvifa (rate, nper)));
}

double ipmt(double rate, double per, double nper, double pv, double fv, int type)
{
	double p = pmt (rate, nper, pv, fv, 0);
	double ip = -(pv * pow1p (rate, per - 1) * rate + p * pow1pm1 (rate, per - 1));
	return (type == 0) ? ip : ip / (1 + rate);
}

double libfin::ppmt (double rate, double per, double nper, double pv, double fv, int type)
{
	// exception identified 1/26/2010 by Aron and J.MacKnick when term (nper = 0)
	if (nper == 0) return 0.0;
	double p = pmt (rate, nper, pv, fv, type);
	double ip = ipmt (rate, per, nper, pv, fv, type);
	return p - ip;
}

