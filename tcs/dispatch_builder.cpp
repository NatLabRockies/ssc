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
#include "dispatch_builder.h"

//#include "lp_lib.h" 
//#include "lib_util.h"

/*

Careful with namespaces in this file.. importing the LPsolve library introduces new macro definitions
and function definitions.

*/

void __WINAPI opt_logfunction(lprec* lp, void* userhandle, char* buf)
{
    s_solver_params* par = static_cast<s_solver_params*>(userhandle);
    std::string line = buf;
    par->log_message.append(line);
}

int __WINAPI opt_abortfunction(lprec* lp, void* userhandle)
{
    s_solver_params* par = static_cast<s_solver_params*>(userhandle);
    return par->is_abort_flag ? TRUE : FALSE;
}

void __WINAPI opt_iter_function(lprec* lp, void* userhandle, int msg)
{
    s_solver_params* par = static_cast<s_solver_params*>(userhandle);

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

s_solver_params::s_solver_params()
{
    is_abort_flag = false;
    log_message = "";
    obj_relaxed = std::numeric_limits<double>::quiet_NaN();

    //user settings
    steps_per_hour = 1;
    optimize_frequency = 24;
    optimize_horizon = 48;

    max_bb_iter = 10000;
    mip_gap = 0.055;
    solution_timeout = 5.;

    presolve_type = -1;
    bb_type = -1;
    disp_reporting = -1;
    scaling_type = -1;

    is_write_ampl_dat = false;
    is_ampl_engine = false;
    ampl_data_dir = "";
    ampl_exec_call = "";
}

void s_solver_params::set_user_inputs(int disp_steps_per_hour, int disp_frequency, int disp_horizon,
    int disp_max_iter, double disp_mip_gap, double disp_timeout,
    int disp_spec_presolve, int disp_spec_bb, int disp_spec_scaling, int disp_spec_reporting)
{
    //user settings
    steps_per_hour = disp_steps_per_hour;
    optimize_frequency = disp_frequency;
    optimize_horizon = disp_horizon;

    max_bb_iter = disp_max_iter;
    mip_gap = disp_mip_gap;
    solution_timeout = disp_timeout;

    presolve_type = disp_spec_presolve;
    bb_type = disp_spec_bb;
    scaling_type = disp_spec_scaling;
    disp_reporting = disp_spec_reporting;
}


void s_solver_params::set_ampl_inputs(bool is_write_ampl_dat_spec, bool is_ampl_engine_spec, std::string ampl_data_dir_spec, std::string ampl_exec_call_spec)
{
    is_write_ampl_dat = is_write_ampl_dat_spec;
    is_ampl_engine = is_ampl_engine_spec;
    ampl_data_dir = ampl_data_dir_spec;
    ampl_exec_call = ampl_exec_call_spec;
}

void s_solver_params::reset()
{
    is_abort_flag = false;
    log_message.clear();
    obj_relaxed = 0.;
}

// -----------------------------------------

optimization_vars::optimization_vars()
{
    current_mem_pos = 0;
    alloc_mem_size = 0;

    data = nullptr;
}

void optimization_vars::add_var(const std::string& vname, int var_type /* VAR_TYPE enum */, int var_dim /* VAR_DIM enum */, int var_dim_size, REAL lobo, REAL upbo)
{
    if (var_dim == VAR_DIM::DIM_T2)
        add_var(vname, var_type, VAR_DIM::DIM_NT, var_dim_size, var_dim_size, lobo, upbo);
    else
        add_var(vname, var_type, var_dim, var_dim_size, 1, lobo, upbo);

}

void optimization_vars::add_var(const std::string& vname, int var_type /* VAR_TYPE enum */, int var_dim /* VAR_DIM enum */, int var_dim_size, int var_dim_size2, REAL lobo, REAL upbo)
{
    var_objects.push_back(optimization_vars::opt_var());
    optimization_vars::opt_var* v = &var_objects.back();
    v->name = vname;
    v->ind_start = current_mem_pos;
    v->var_type = var_type;
    v->var_dim = var_dim;
    v->var_dim_size = var_dim_size;
    v->var_dim_size2 = var_dim_size2;
    if (v->var_type == optimization_vars::VAR_TYPE::BINARY_T)
    {
        v->upper_bound = 1.;
        v->lower_bound = 0.;
    }
    else
    {
        v->upper_bound = upbo;
        v->lower_bound = lobo;
    }

    //calculate the required memory space for this type of variable
    int mem_size = 0;
    switch (var_dim)
    {
    case optimization_vars::VAR_DIM::DIM_T:
        mem_size = var_dim_size;
        break;
    case optimization_vars::VAR_DIM::DIM_NT:
        mem_size = var_dim_size * var_dim_size2;
        break;
    case optimization_vars::VAR_DIM::DIM_T2:
        throw std::runtime_error("Invalid var dimension in add_var");
    case optimization_vars::VAR_DIM::DIM_2T_TRI:
        mem_size = (var_dim_size + 1) * var_dim_size / 2;
        break;
    }

    v->ind_end = v->ind_start + mem_size;

    current_mem_pos += mem_size;


}

bool optimization_vars::construct()
{
    if (current_mem_pos < 0 || current_mem_pos > 1000000)
        throw std::runtime_error("Bad memory allocation when constructing variable table for dispatch optimization.");

    data = new REAL[current_mem_pos];

    alloc_mem_size = current_mem_pos;

    for (int i = 0; i < (int)var_objects.size(); i++)
        var_by_name[var_objects.at(i).name] = &var_objects.at(i);

    return true;
}

REAL& optimization_vars::operator()(char* varname, int ind)    //Access for 1D var
{
    return data[var_by_name[varname]->ind_start + ind];

}

REAL& optimization_vars::operator()(char* varname, int ind1, int ind2)     //Access for 2D var
{
    return data[column(varname, ind1, ind1) - 1];
}

REAL& optimization_vars::operator()(int varind, int ind)    //Access for 1D var
{
    return data[var_objects.at(varind).ind_start + ind];

}

REAL& optimization_vars::operator()(int varind, int ind1, int ind2)     //Access for 2D var
{
    return data[column(varind, ind1, ind2) - 1];
}

int optimization_vars::column(const std::string& varname, int ind)
{
    return var_by_name[varname]->ind_start + ind + 1;
}

int optimization_vars::column(const std::string& varname, int ind1, int ind2)
{
    opt_var* v = var_by_name[std::string(varname)];
    switch (v->var_dim)
    {
    case VAR_DIM::DIM_T:
        throw std::runtime_error("Attempting to access optimization variable memory via 2D call when referenced variable is 1D.");
    case VAR_DIM::DIM_NT:
        return v->ind_start + v->var_dim_size2 * ind1 + ind2 + 1;
    default:
    {
        int ind = v->var_dim_size * ind1 + ind2 - ((ind1 - 1) * ind1 / 2);
        return v->ind_start + ind + 1;
    }
    break;
    }
}

int optimization_vars::column(int varindex, int ind)
{
    return var_objects[varindex].ind_start + ind + 1;
}

int optimization_vars::column(int varindex, int ind1, int ind2)
{
    opt_var* v = &var_objects[varindex];
    switch (v->var_dim)
    {
    case VAR_DIM::DIM_T:
        throw std::runtime_error("Attempting to access optimization variable memory via 2D call when referenced variable is 1D.");
    case VAR_DIM::DIM_NT:
        return v->ind_start + v->var_dim_size2 * ind1 + ind2 + 1;
    default:
    {
        int ind = v->var_dim_size * ind1 + ind2 - ((ind1 - 1) * ind1 / 2);
        return v->ind_start + ind + 1;
    } break;
    }
}

int optimization_vars::get_num_varobjs()
{
    return (int)var_objects.size();
}

int optimization_vars::get_total_var_count()
{
    return alloc_mem_size;
}

REAL* optimization_vars::get_variable_array()
{
    return data;
}

optimization_vars::opt_var* optimization_vars::get_var(const std::string& varname)
{
    return var_by_name[varname];
}

optimization_vars::opt_var* optimization_vars::get_var(int varindex)
{
    return &var_objects[varindex];
}
