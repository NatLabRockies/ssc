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


#ifndef _LIB_PV_SPECTRAL_CORRECTION_H_
#define _LIB_PV_SPECTRAL_CORRECTION_H_

#include <memory>
#include <vector>
#include "../ssc/core.h"

#include "lib_util.h"

#define AOI_MIN 0.5
#define AOI_MAX 89.5

extern var_info vtab_spectral_correction[];
static double amavec[5] = { 0.918093, 0.086257, -0.024459, 0.002816, -0.000126 };	// !Air mass modifier coefficients as indicated for polycrystalline modules in Table A1 of DeSoto paper published in Solar Energy  Vol 80 Issue 1 January 2006


//static double amavec[5] = { 0.918093, 0.086257, -0.024459, 0.002816, -0.000126 };	// !Air mass modifier coefficients as indicated for polycrystalline modules in Table A1 of DeSoto paper published in Solar Energy  Vol 80 Issue 1 January 2006

double spectral_correction_lee(double prec_water, double abs_airmass, int celltech, std::vector<double> coeff_inputs = { 0.0 },
    double min_prec_water=0.1, double max_prec_water=8, double min_abs_airmass=0.58, double max_abs_airmass=10);

double spectral_correction_king(double abs_airmass, double Zenith_deg, double Elev_m, double a[5] = amavec);

double spectral_correction_pelland(double abs_airmass, double csky_index, int celltech, std::vector<double> coeff_inputs = { 0.0 });

static double sandia_absolute_air_mass(double SolZen, double Altitude);


double spectral_correction_factor(compute_module* cm, double pwater, double solzen = 0.0, double alt = 0.0, double csky_index = 0.0);
/// Calculate Irradiance through the cover using the DeSoto model


#endif
