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


#ifndef _CMOD_WINDPOWER_BUILDER_H_
#define _CMOD_WINDPOWER_BUILDER_H_

#include "sscapi.h"

#ifdef __cplusplus
extern "C" {
#endif

static const char* Turbine_calculate_powercurve_doc =
    "Calculates the power produced by a wind turbine at windspeeds incremented by 0.25 m/s\\n\\n"
    "Input: var_table with key-value pairs\\n"
    "     'turbine_size': double [kW]\\n"
    "     'rotor_diameter': int [m]\\n"
    "     'elevation': double [m], required if using Weibull resource model, otherwise 0\\n"
    "     'max_cp': double max Cp [-],\\n"
    "     'max_tip_speed': double [m/s]\\n"
    "     'max_tip_sp_ratio': double max tip speed ratio [-]\\n"
    "     'cut_in': double cut in speed [m/s]\\n"
    "     'cut_out': double cut out speed [m/s]\\n"
    "     'drive_train': int 0: 3 Stage Planetary, 1: Single Stage - Low Speed Generator, 2: Multi-Generator, 3: Direct Drive\\n\\n"
    "Output: key-value pairs added to var_table\\n"
    "     'wind_turbine_powercurve_windspeeds': array [m/s]\\n"
    "     'wind_turbine_powercurve_powerout': array [m/s]\\n"
    "     'rated_wind_speed': double [m/s[\\n"
    "     'hub_efficiency': array [m/s]";

SSCEXPORT bool Turbine_calculate_powercurve(ssc_data_t data);


#ifdef __cplusplus
}
#endif

#endif
