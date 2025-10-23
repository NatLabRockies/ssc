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


#pragma once
#pragma warning(disable: 4290)  // ignore warning: 'C++ exception specification ignored except to indicate a function is not __declspec(nothrow)'

#include <vector>
#include <string>
#include <unordered_map>
#include "../lpsolve/lp_lib.h"
#include <limits>

void __WINAPI opt_logfunction(lprec* lp, void* userhandle, char* buf);
int __WINAPI opt_abortfunction(lprec* lp, void* userhandle);
void __WINAPI opt_iter_function(lprec* lp, void* userhandle, int msg);

struct s_solver_params
{
    bool is_abort_flag;         //optimization flagged for abort
    std::string log_message;
    double obj_relaxed;

    //user settings
    int steps_per_hour;         //[-] Number of time steps per hour
    int optimize_frequency;
    int optimize_horizon;

    int max_bb_iter;            //Maximum allowable iterations for B&B algorithm
    double mip_gap;             //convergence tolerance - gap between relaxed MIP solution and current best solution
    double solution_timeout;    //[s] Max solve time for each solution
    int presolve_type;
    int bb_type;
    int disp_reporting;
    int scaling_type;

    bool is_write_ampl_dat;     //write ampl data files?
    bool is_ampl_engine;        //run with external AMPL engine
    std::string ampl_data_dir;  //directory to write ampl data files
    std::string ampl_exec_call; //system call for running ampl

    s_solver_params();
    void set_user_inputs(int disp_steps_per_hour, int disp_frequency, int disp_horizon,
        int disp_max_iter, double disp_mip_gap, double disp_timeout,
        int disp_spec_presolve, int disp_spec_bb, int disp_spec_scaling, int disp_spec_reporting);
    void set_ampl_inputs(bool is_write_ampl_dat_spec, bool is_ampl_engine_spec, std::string ampl_data_dir_spec, std::string ampl_exec_call_spec);
    void reset();
};

class optimization_vars
{
    int current_mem_pos;
    int alloc_mem_size;

    REAL* data;

public:
    struct opt_var
    {
        std::string name;
        int var_type;
        int var_dim;
        int var_dim_size;
        int var_dim_size2;
        int ind_start;
        int ind_end;
        REAL upper_bound;
        REAL lower_bound;
    };
private:
    std::vector<opt_var> var_objects;

    std::unordered_map<std::string, opt_var*> var_by_name;

public:
    struct VAR_TYPE { enum A { REAL_T, INT_T, BINARY_T }; };
    struct VAR_DIM { enum A { DIM_T, DIM_NT, DIM_T2, DIM_2T_TRI }; };

    optimization_vars();
    //~optimization_vars();

    void add_var(const std::string& vname, int var_type /* VAR_TYPE enum */, int var_dim /* VAR_DIM enum */, int var_dim_size, REAL lowbo = -DEF_INFINITE, REAL upbo = DEF_INFINITE);
    void add_var(const std::string& vname, int var_type /* VAR_TYPE enum */, int var_dim /* VAR_DIM enum */, int var_dim_size, int var_dim_size2, REAL lowbo = -DEF_INFINITE, REAL upbo = DEF_INFINITE);

    bool construct();

    int get_num_varobjs();
    int get_total_var_count();

    REAL& operator()(char* varname, int ind);    //Access for 1D var
    REAL& operator()(char* varname, int ind1, int ind2);     //Access for 2D var
    REAL& operator()(int varindex, int ind);
    REAL& operator()(int varindex, int ind1, int ind2);

    int column(const std::string& varname, int ind);
    int column(const std::string& varname, int ind1, int ind2);
    int column(int varindex, int ind);
    int column(int varindex, int ind1, int ind2);

    REAL* get_variable_array();

    opt_var* get_var(const std::string& varname);
    opt_var* get_var(int varindex);
};
