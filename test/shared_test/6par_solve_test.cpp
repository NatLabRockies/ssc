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


#include <vector>
#include <string>
#include <gtest/gtest.h>

#include "6par_solve.h"

TEST(SixParSolve_6par_solve, NewMonoSiModules) {
    // Vmp, Imp, Voc, Isc, alpha_isc, beta_voc, gamma_pmp, Nser, Tref
    std::vector<std::vector<double>> datasheet_values {
            {31.8, 17.29, 38.1, 18.39, 0.007356, -0.09525, -0.34, 110, 43},     // TSM-DEG19C.20 550 Vertex
            {31.7, 17.29, 38.1, 18.39, 0.0007356, -0.09525, -0.34, 110, 43},    // Similar to  TSM-DEG19C.20
            {34.6, 17.34, 41.7, 18.42, 0.007368, -0.10425, -0.34, 120, 43},     // TSM-DEG20C.20-600 Vertex
            {34.85, 17.22, 41.4, 18.5, 0.0074, -0.11178, -0.35, 120, 44}        // RSM120-8-600BMDG
    };
    int tech_id = module6par::monoSi;

    for (size_t i = 0; i < datasheet_values.size(); i++) {
        auto mod = datasheet_values[i];

        double Vmp = mod[0];
        double Imp = mod[1];
        double Voc = mod[2];
        double Isc = mod[3];
        double alpha_isc = mod[4];
        double beta_voc = mod[5];
        double gamma_pmp = mod[6];
        int Nser = (int)mod[7];
        double Tref = mod[8];

        module6par m( tech_id, Vmp, Imp, Voc, Isc, beta_voc, alpha_isc, gamma_pmp, Nser, Tref+273.15 );
        int err = m.solve_with_sanity_and_heuristics<double>(300,1e-7);
        EXPECT_GT(err, -1);
    }
}


TEST(SixParSolve_6par_solve, CIGSModules) {
    // Vmp, Imp, Voc, Isc, alpha_isc, beta_voc, gamma_pmp, Nser, Tref
    std::vector<std::vector<double>> datasheet_values {
            {88.3, 1.7, 108.9, 1.83, 0.000183, -0.29403, -0.32, 96, 42},     // TSM-DEG19C.20 550 Vertex
    };
    int tech_id = module6par::CIGS;

    for (size_t i = 0; i < datasheet_values.size(); i++) {
        auto mod = datasheet_values[i];

        double Vmp = mod[0];
        double Imp = mod[1];
        double Voc = mod[2];
        double Isc = mod[3];
        double alpha_isc = mod[4];
        double beta_voc = mod[5];
        double gamma_pmp = mod[6];
        int Nser = (int)mod[7];
        double Tref = mod[8];

        module6par m( tech_id, Vmp, Imp, Voc, Isc, beta_voc, alpha_isc, gamma_pmp, Nser, Tref+273.15 );
        int err = m.solve_with_sanity_and_heuristics<double>(300,1e-7);
        EXPECT_GT(err, -1);
    }
}
