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

#ifndef __base_dispatch_lpsolve_
#define __base_dispatch_lpsolve_

#include "base_dispatch.h"
#include "dispatch_builder.h"

void __WINAPI opt_logfunction(lprec* lp, void* userhandle, char* buf);
int __WINAPI opt_abortfunction(lprec* lp, void* userhandle);
void __WINAPI opt_iter_function(lprec* lp, void* userhandle, int msg);

class base_dispatch_lpsolve : public base_dispatch_opt
{
public:
    // Set default solver parameters if user did not set them
    virtual void set_default_solver_parameters();

    //Constructs lp model
    lprec* construct_lp_model(optimization_vars* opt_vars);

    //Set LPsolve solver presolve settings and bb rules
    void setup_solver_presolve_bbrules(lprec* lp);

    //Problem scaling loop algorithm
    bool problem_scaling_solve_loop(lprec* lp);

    //Set LPsolve outputs
    void set_solver_outputs(lprec* lp);

    //Used by cmod to get dispatch annual stats on solves
    void count_solutions_by_type(std::vector<int>& flag, int dispatch_freq, std::string& log_msg);

    //Calculates average relative mip gap of suboptimal solutions
    double calc_avg_subopt_gap(std::vector<double>& gap, std::vector<int>& flag, int dispatch_freq);

    // Saving problem and solution for debugging
    void save_problem_solution_debug(lprec* lp);

    //Dispatch update and result print to screen
    void print_dispatch_update();

    // Parse column name to get variable name (root) and index (ind)
    bool parse_column_name(char* colname, char* root, char* ind);

    // simple string compare
    bool strcompare(std::string a, std::string b);

};

#endif //__base_dispatch_lpsolve_
