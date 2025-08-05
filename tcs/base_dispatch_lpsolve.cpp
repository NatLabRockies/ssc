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

#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <algorithm>
#include "base_dispatch_lpsolve.h"

void __WINAPI opt_logfunction(lprec* lp, void* userhandle, char* buf)
{
    base_dispatch_opt::s_solver_params* par = static_cast<base_dispatch_opt::s_solver_params*>(userhandle);
    std::string line = buf;
    par->log_message.append(line);
}

int __WINAPI opt_abortfunction(lprec* lp, void* userhandle)
{
    base_dispatch_opt::s_solver_params* par = static_cast<base_dispatch_opt::s_solver_params*>(userhandle);
    return par->is_abort_flag ? TRUE : FALSE;
}

void __WINAPI opt_iter_function(lprec* lp, void* userhandle, int msg)
{
    base_dispatch_opt::s_solver_params* par = static_cast<base_dispatch_opt::s_solver_params*>(userhandle);

    /*if( get_timeout(lp) > 0 )
        par->is_abort_flag = true;*/

    if (msg == MSG_MILPBETTER)
    {
        par->obj_relaxed = get_bb_relaxed_objective(lp);

        double cur = get_working_objective(lp);

        if (par->obj_relaxed > 0.)
            if (cur / par->obj_relaxed > 1. - par->mip_gap)
                par->is_abort_flag = true;
    }

    if (get_total_iter(lp) > par->max_bb_iter)
        par->is_abort_flag = true;
}


lprec* base_dispatch_lpsolve::construct_lp_model(optimization_vars* opt_vars)
{
    opt_vars->construct();  //allocates memory for data array
    int nvar = opt_vars->get_total_var_count(); //total number of variables in the problem
    lprec* lp = make_lp(0, nvar);  //build the context
    set_add_rowmode(lp, TRUE);  //set the row mode

    if (lp == NULL)
        throw C_csp_exception("Failed to create a new dispatch optimization problem context.");

    int nt = (int)m_nstep_opt;

    /*
    --------------------------------------------------------------------------------
    set up the variable properties
    --------------------------------------------------------------------------------
    */

    //set variable names and types for each column
    for (int i = 0; i < opt_vars->get_num_varobjs(); i++)
    {
        optimization_vars::opt_var* v = opt_vars->get_var(i);

        std::string name_base = v->name;

        if (v->var_dim == optimization_vars::VAR_DIM::DIM_T)
        {
            for (int t = 0; t < nt; t++)
            {
                char s[40];
                sprintf(s, "%s-%d", name_base.c_str(), t);
                set_col_name(lp, opt_vars->column(i, t), s);

            }
        }
        else if (v->var_dim == optimization_vars::VAR_DIM::DIM_NT)
        {
            for (int t1 = 0; t1 < v->var_dim_size; t1++)
            {
                for (int t2 = 0; t2 < v->var_dim_size2; t2++)
                {
                    char s[40];
                    sprintf(s, "%s-%d-%d", name_base.c_str(), t1, t2);
                    set_col_name(lp, opt_vars->column(i, t1, t2), s);
                }
            }
        }
        else
        {
            for (int t1 = 0; t1 < nt; t1++)
            {
                for (int t2 = t1; t2 < nt; t2++)
                {
                    char s[40];
                    sprintf(s, "%s-%d-%d", name_base.c_str(), t1, t2);
                    set_col_name(lp, opt_vars->column(i, t1, t2), s);
                }
            }
        }
    }

    // set variable bounds
    for (int i = 0; i < opt_vars->get_num_varobjs(); i++)
    {
        optimization_vars::opt_var* v = opt_vars->get_var(i);
        if (v->var_type == optimization_vars::VAR_TYPE::BINARY_T)
        {
            for (int i = v->ind_start; i < v->ind_end; i++)
                set_binary(lp, i + 1, TRUE);
        }
        //upper and lower variable bounds
        for (int i = v->ind_start; i < v->ind_end; i++)
        {
            set_upbo(lp, i + 1, v->upper_bound);
            set_lowbo(lp, i + 1, v->lower_bound);
        }
    }

    return lp;
}

void base_dispatch_lpsolve::setup_solver_presolve_bbrules(lprec* lp)
{
    //reset the row mode
    set_add_rowmode(lp, FALSE);

    //set the log function
    solver_params.reset();

    put_msgfunc(lp, opt_iter_function, (void*)(&solver_params), MSG_ITERATION | MSG_MILPBETTER | MSG_MILPFEASIBLE);
    put_abortfunc(lp, opt_abortfunction, (void*)(&solver_params));
    if (solver_params.disp_reporting > 0)
    {
        put_logfunc(lp, opt_logfunction, (void*)(&solver_params));
        set_verbose(lp, solver_params.disp_reporting); //http://web.mit.edu/lpsolve/doc/set_verbose.htm
    }
    else
    {
        set_verbose(lp, 0);
    }

    set_presolve(lp, solver_params.presolve_type, get_presolveloops(lp));
    set_mip_gap(lp, FALSE, solver_params.mip_gap);
    set_timeout(lp, solver_params.solution_timeout);  //max solution time

    //Debugging parameters
    //set_bb_depthlimit(lp, -10);   //max branch depth
    //set_solutionlimit(lp, 1);     //only look for 1 optimal solution
    //set_outputfile(lp, "c://users//mwagner//documents//dropbox//nrel//formulation//trace.txt");
    //set_debug(lp, TRUE);

    //Different Basis Factorization Package:
        /* Using these packages did not seem to help reduce solution times. */
    //set_BFP(lp, "bfp_etaPFI");    // original lp_solve product form of the inverse.
    //set_BFP(lp, "bfp_LUSOL");     // LU decomposition. (LP solve manual suggests using this one) Seems to increase solve times
    //set_BFP(lp, "bfp_GLPK");      // GLPK LU decomposition.

    //branch and bound rule. This one has a big impact on solver performance.
    set_bb_rule(lp, solver_params.bb_type);
}

bool base_dispatch_lpsolve::problem_scaling_solve_loop(lprec* lp)
{
    //Problem scaling loop
    int scaling_iter = 0;
    bool is_opt_or_subopt = false;
    while (scaling_iter < 5)
    {
        //Scaling algorithm
        switch (scaling_iter)
        {
        case 0:
            set_scaling(lp, solver_params.scaling_type);
            break;
        case 1:
            set_scaling(lp, SCALE_NONE);
            break;
        case 2:
            set_scaling(lp, SCALE_CURTISREID | SCALE_LINEAR | SCALE_EQUILIBRATE | SCALE_INTEGERS);
            //set_scaling(lp, SCALE_FUTURE1); 
            break;
        case 3:
            set_scaling(lp, SCALE_CURTISREID);
            break;
        case 4:
            set_scaling(lp, SCALE_INTEGERS | SCALE_LINEAR | SCALE_GEOMETRIC | SCALE_EQUILIBRATE);  //default
            break;
        }

        //record the solve state
        lp_outputs.solve_state = solve(lp);

        is_opt_or_subopt = lp_outputs.solve_state == OPTIMAL || lp_outputs.solve_state == SUBOPTIMAL;

        if (is_opt_or_subopt)
            break;      //break the scaling loop

        //If the problem was reported as unbounded, this probably has to do with poor scaling. Try again with no scaling.
        std::string fail_type;
        switch (lp_outputs.solve_state)
        {
        case UNBOUNDED:
            fail_type = "... Unbounded";
            //set_outputfile(lp, "C:\\Users\\WHamilt2\\Documents\\SAM\\ZZZ_working_directory\\unbounded_setup.txt");
            //print_lp(lp);
            break;
        case NUMFAILURE:
            fail_type = "... Numerical failure in";
            //set_outputfile(lp, "C:\\Users\\WHamilt2\\Documents\\SAM\\ZZZ_working_directory\\numerical_failure_setup.txt");
            //print_lp(lp);
            break;
        case INFEASIBLE:
            fail_type = "... Infeasible";
            //set_outputfile(lp, "C:\\Users\\WHamilt2\\Documents\\SAM\\ZZZ_working_directory\\infeasable_setup.txt");
            //print_lp(lp);
            break;
        }
        pointers.messages->add_message(C_csp_messages::NOTICE, fail_type + " dispatch optimization problem. Retrying with modified problem scaling.");

        unscale(lp);
        default_basis(lp);

        scaling_iter++;
    }
    return is_opt_or_subopt;
}

void base_dispatch_lpsolve::set_lp_solve_outputs(lprec* lp)
{
    if (lp_outputs.solve_state == NOTRUN)
    {
        throw std::runtime_error("LPSolve must be solved and solve_state must be set before running set_lp_solve_outputs()");
    }

    //keep track of problem efficiency
    lp_outputs.presolve_nconstr = get_Nrows(lp);
    lp_outputs.presolve_nvar = get_Ncolumns(lp);
    lp_outputs.solve_time = time_elapsed(lp);
    lp_outputs.solve_iter = (int)get_total_iter(lp);         //get number of iterations


    if (lp_outputs.solve_state == OPTIMAL || lp_outputs.solve_state == SUBOPTIMAL)
    {
        lp_outputs.objective = get_objective(lp);
        lp_outputs.objective_relaxed = get_bb_relaxed_objective(lp);
    }
    else
    {
        //if the optimization wasn't successful, just set the objective values to zero - otherwise they are NAN
        lp_outputs.objective = 0.;
        lp_outputs.objective_relaxed = 0.;
    }

    // When solve_state is 0, I believe this is the last known gap before tree was prune. Therefore, not reporting
    if (lp_outputs.solve_state == SUBOPTIMAL)
        lp_outputs.rel_mip_gap = std::abs(lp_outputs.objective - lp_outputs.objective_relaxed) / (1.0 + std::abs(lp_outputs.objective_relaxed));
    else
        lp_outputs.rel_mip_gap = get_mip_gap(lp, FALSE);

    // Set suboptimal state flag
    if ((lp_outputs.solve_state == SUBOPTIMAL) && (solver_params.is_abort_flag)) {
        if (lp_outputs.solve_iter > solver_params.max_bb_iter) {
            lp_outputs.subopt_flag = INTERATION;    // stop due to iteration count
        }
        else {
            lp_outputs.subopt_flag = MIPGAP;   // stop due to mip gap from abort function
        }
    }
    else if (lp_outputs.solve_state == SUBOPTIMAL) {
        if (lp_outputs.solve_time > solver_params.solution_timeout) {
            lp_outputs.subopt_flag = TIMELIMIT;   // stop due to time limit
        }
        else {
            lp_outputs.subopt_flag = MIPGAPLPSOLVE;   // stop due to mip gap internal of LPSolve
        }
    }
    else if (lp_outputs.solve_state == OPTIMAL) {
        lp_outputs.subopt_flag = OPTIMAL;
    }
    else {
        lp_outputs.subopt_flag = FAILED;
    }
}

void base_dispatch_lpsolve::count_solutions_by_type(std::vector<int>& flag, int dispatch_freq, std::string& log_msg)
{
    int opt = 0, iter = 0, timeout = 0, user_gap = 0, lpsolve_gap = 0, failed = 0;
    for (size_t i = 0; i < flag.size(); i += dispatch_freq)
    {
        // Mapping flags to solve state condition
        if (flag[i] == OPTIMAL) {
            opt += 1;
        }
        else if (flag[i] == INTERATION) {
            iter += 1;
        }
        else if (flag[i] == TIMELIMIT) {
            timeout += 1;
        }
        else if (flag[i] == MIPGAP) {
            user_gap += 1;
        }
        else if (flag[i] == MIPGAPLPSOLVE) {
            lpsolve_gap += 1;
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
        "Suboptimal lpsolve gap: %d\n"
        "Failed solve: %d", opt, iter, timeout, user_gap, lpsolve_gap, failed);
}

double base_dispatch_lpsolve::calc_avg_subopt_gap(std::vector<double>& gap, std::vector<int>& flag, int dispatch_freq)
{
    double avg_gap = 0.;
    int count = 0;
    for (size_t i = 0; i < gap.size(); i += dispatch_freq)
    {
        // Calculating average gap for suboptimal solutions
        if (flag[i] != OPTIMAL) {
            avg_gap += gap[i];
            count += 1;
        }
    }
    avg_gap /= (double)count;
    avg_gap *= 100.;
    return avg_gap;
}

void base_dispatch_lpsolve::save_problem_solution_debug(lprec* lp)
{
    // Saving problem and solution for debugging
    set_outputfile(lp, "setup.txt");
    print_lp(lp);
    set_outputfile(lp, "solution.txt");
    print_solution(lp, 1);
}

void base_dispatch_lpsolve::print_dispatch_update()
{
    std::stringstream s;
    int time_start = (int)(pointers.siminfo->ms_ts.m_time / 3600.);
    s << "Time " << time_start << " - " << time_start + m_nstep_opt << ": ";

    int type = OPTIMAL;

    switch (lp_outputs.solve_state)
    {
    case UNKNOWNERROR:
        type = C_csp_messages::WARNING;
        s << "... An unknown error occurred while attempting to solve the dispatch optimization problem.";
        break;
    case DATAIGNORED:
        type = C_csp_messages::WARNING;
        s << "Dispatch optimization failed: Data ignored.";
        break;
    case NOBFP:
        type = C_csp_messages::WARNING;
        s << "Dispatch optimization failed: No BFP.";
        break;
    case NOMEMORY:
        type = C_csp_messages::WARNING;
        s << "Dispatch optimization failed: Out of memory.";
        break;
    case NOTRUN:
        type = C_csp_messages::WARNING;
        s << "Dispatch optimization failed: Simulation did not run.";
        break;
    case SUBOPTIMAL:
        type = C_csp_messages::NOTICE;
        s << "Suboptimal solution identified.";
        break;
    case INFEASIBLE:
        type = C_csp_messages::WARNING;
        s << "Dispatch optimization failed: Infeasible problem.";
        break;
    case UNBOUNDED:
        type = C_csp_messages::WARNING;
        s << "Dispatch optimization failed: Unbounded problem.";
        break;
    case DEGENERATE:
        type = C_csp_messages::WARNING;
        s << "Dispatch optimization failed: Degenerate problem.";
        break;
    case NUMFAILURE:
        type = C_csp_messages::WARNING;
        s << "Dispatch optimization failed: Numerical failure.";
        break;
    case USERABORT:
    case TIMEOUT:
        type = C_csp_messages::WARNING;
        s << "Dispatch optimization failed: Iteration or time limit reached before identifying a solution.";
        break;
    case OPTIMAL:
        type = C_csp_messages::NOTICE;
        s << "Optimal solution identified.";
    default:
        break;
    }

    pointers.messages->add_message(type, s.str());
}

bool base_dispatch_lpsolve::parse_column_name(char* colname, char* root, char* ind)
{
    int i;
    for (i = 0; i < 15; i++)
    {
        if (colname[i] == '-')
        {
            root[i] = '\0';
            break;
        }
        else
            root[i] = colname[i];
    }
    int i1 = 1 + i++;
    bool not_interested = false;
    for (i = i1; i < 15; i++)
    {
        if (colname[i] == '-')
        {
            //2D variable. Not interested at the moment..
            not_interested = true;
            break;
        }
        else if (colname[i] == 0)
        {
            ind[i - i1] = '\0';
            break;
        }
        else
            ind[i - i1] = colname[i];
    }

    return not_interested;
}

bool base_dispatch_lpsolve::strcompare(std::string a, std::string b)
{
    return util::lower_case(a) < util::lower_case(b);
}
