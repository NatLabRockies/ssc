/*
BSD 3-Clause License

Copyright (c) Alliance for Sustainable Energy, LLC. See also https://github.com/NatLabRockies/ssc/blob/develop/LICENSE
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

#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <algorithm>
#include "base_dispatch_ortools.h"

using namespace operations_research;

void base_dispatch_opt_ortools::set_solver_outputs(double time_elapsed_sec)
{
    if (solver_outputs.solve_state == MPSolver::ResultStatus::NOT_SOLVED)
        throw(C_csp_exception("OR-Tools must be solved and solve_state must be set before running set_solver_outputs()", "base_dispatch_ortools"));

    if (solver_outputs.solve_state == MPSolver::ResultStatus::MODEL_INVALID)
        throw(C_csp_exception("OR-Tools model is invalid, check the model setup and constraints (NaN coefficients, etc)", "base_dispatch_ortools"));

    //keep track of problem efficiency
    solver_outputs.presolve_nconstr = solver->NumConstraints();
    solver_outputs.presolve_nvar = solver->NumVariables();
    solver_outputs.solve_time = time_elapsed_sec;
    solver_outputs.solve_iter = solver->iterations();         //get number of iterations

    //if the optimization wasn't successful, just set the objective values to zero - otherwise they are NAN
    solver_outputs.objective = 0.;
    solver_outputs.objective_relaxed = 0.;
    if (solver_outputs.solve_state == MPSolver::ResultStatus::OPTIMAL || solver_outputs.solve_state == MPSolver::ResultStatus::FEASIBLE) {
        solver_outputs.objective = solver->Objective().Value();
        solver_outputs.objective_relaxed = solver->Objective().BestBound();
    }

    // When solve_state is 0, I believe this is the last known gap before tree was prune. Therefore, not reporting
    solver_outputs.rel_mip_gap = std::abs(solver_outputs.objective - solver_outputs.objective_relaxed) / (1.0 + std::abs(solver_outputs.objective_relaxed));

    // Set suboptimal state flag
    if (solver_outputs.solve_state == MPSolver::ResultStatus::FEASIBLE) {       // feasible, or stopped by limit.
        if (solver_outputs.solve_time > solver_params.solution_timeout) {
            solver_outputs.termination_flag = termination_flags::timelimit;   // stop due to time limit
        }
        else {
            solver_outputs.termination_flag = termination_flags::mipgap;   // stop due to mip gap internal of LPSolve
        }
    }
    else if (solver_outputs.solve_state == MPSolver::ResultStatus::OPTIMAL) {
        solver_outputs.termination_flag = termination_flags::optimal;
    }
    else {
        solver_outputs.termination_flag = termination_flags::failed;
    }
}

void base_dispatch_opt_ortools::print_dispatch_update()
{
    std::stringstream s;
    int time_start = (int)(pointers.siminfo->ms_ts.m_time / 3600.);
    s << "Time " << time_start << " - " << time_start + m_nstep_opt << ": ";

    int type = 0;

    switch (solver_outputs.solve_state)
    {
    case MPSolver::ResultStatus::OPTIMAL:
        type = C_csp_messages::NOTICE;
        s << "Optimal solution identified.";
        break;
    case MPSolver::ResultStatus::FEASIBLE:
        type = C_csp_messages::NOTICE;
        s << "Suboptimal (feasible) solution identified.";
        break;
    case MPSolver::ResultStatus::INFEASIBLE:
        type = C_csp_messages::WARNING;
        s << "Dispatch optimization failed: Proven infeasible.";
        break;
    case MPSolver::ResultStatus::UNBOUNDED:
        type = C_csp_messages::WARNING;
        s << "Dispatch optimization failed: Proven unbounded.";
        break;
    case MPSolver::ResultStatus::ABNORMAL:
        type = C_csp_messages::WARNING;
        s << "Dispatch optimization failed: Abnormal, i.e., error of some kind";
        break;
    case MPSolver::ResultStatus::MODEL_INVALID:
        type = C_csp_messages::WARNING;
        s << "Dispatch optimization failed: Model is trivially invalid (NAN coefficients, etc.)";
        break;
    default:
        break;
    }

    pointers.messages->add_message(type, s.str());
}

void base_dispatch_opt_ortools::count_solutions_by_type(std::vector<int>& flag, int dispatch_freq, std::string& log_msg)
{
    int opt = 0, iter = 0, timeout = 0, user_gap = 0, failed = 0;
    for (size_t i = 0; i < flag.size(); i += dispatch_freq)
    {
        // Mapping flags to solve state condition
        if (flag[i] == 0) {
            opt += 1;
        }
        else if (flag[i] == termination_flags::iteration) {
            iter += 1;
        }
        else if (flag[i] == termination_flags::timelimit) {
            timeout += 1;
        }
        else if (flag[i] == termination_flags::mipgap) {
            user_gap += 1;
        }
        else {
            failed += 1;
        }
    }

    log_msg = util::format("====== Dispatch Optimization Summary ======\n"
        "Optimal solves: %d\n"
        "Suboptimal iteration limit: %d\n"
        "Suboptimal time limit: %d\n"
        "Suboptimal user gap: %d\n"
        "Failed solve: %d", opt, iter, timeout, user_gap, failed);
}

double base_dispatch_opt_ortools::calc_avg_subopt_gap(std::vector<double>& gap, std::vector<int>& flag, int dispatch_freq)
{
    double avg_gap = 0.;
    int count = 0;
    for (size_t i = 0; i < gap.size(); i += dispatch_freq)
    {
        // Calculating average gap for suboptimal solutions
        if (flag[i] != 0) {
            avg_gap += gap[i];
            count += 1;
        }
    }
    avg_gap /= (double)count;
    avg_gap *= 100.;
    return avg_gap;
}

