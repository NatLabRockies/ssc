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
#include "base_dispatch.h"

base_dispatch_opt::s_solver_params::s_solver_params()
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
}

void base_dispatch_opt::s_solver_params::set_user_inputs(int disp_steps_per_hour, int disp_frequency, int disp_horizon,
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

void base_dispatch_opt::s_solver_params::reset()
{
    is_abort_flag = false;
    log_message.clear();
    obj_relaxed = 0.;
}



base_dispatch_opt::base_dispatch_opt()
{
    //initialize member data
    m_nstep_opt = 0;
    m_is_weather_setup = false;
    
    clear_output();
}

void base_dispatch_opt::not_implemented_function(std::string function_name)
{
    throw std::runtime_error(function_name + " is not implemented.");
}

void base_dispatch_opt::clear_output()
{
    m_current_read_step = 0;
    solver_outputs.clear_output();
}

void base_dispatch_opt::init(double cycle_q_dot_des, double cycle_eta_des)
{
    not_implemented_function((std::string)__func__);
}

bool base_dispatch_opt::check_setup()
{
    //check parameters and inputs to make sure everything has been set up correctly
    if( !pointers.siminfo ) return false;
    
    return true;
}

bool base_dispatch_opt::update_horizon_parameters(C_csp_tou &mc_tou)
{
    not_implemented_function((std::string)__func__);
    return false;
}

void base_dispatch_opt::update_initial_conditions(double q_dot_to_pb, double T_htf_cold_des, double pc_state_persist)
{
    not_implemented_function((std::string)__func__);
}

bool base_dispatch_opt::predict_performance(int step_start, int ntimeints, int divs_per_int)
{
    not_implemented_function((std::string)__func__);
    return false;
}

bool base_dispatch_opt::optimize()
{
    not_implemented_function((std::string)__func__);
    return false;
}

bool base_dispatch_opt::set_dispatch_outputs()
{
    not_implemented_function((std::string)__func__);
    return false;
}

void base_dispatch_opt::count_solutions_by_type(std::vector<int>& flag, int dispatch_freq, std::string& log_msg)
{
    not_implemented_function((std::string)__func__);
}

double base_dispatch_opt::calc_avg_subopt_gap(std::vector<double>& gap, std::vector<int>& flag, int dispatch_freq)
{
    not_implemented_function((std::string)__func__);
    return 0.0;
}

void base_dispatch_opt::print_dispatch_update()
{
    not_implemented_function((std::string)__func__);
}

void base_dispatch_opt::print_log_to_file()
{
    std::stringstream outname;
    outname << "Dispatch.log";
    std::ofstream fout(outname.str().c_str());
    fout << solver_params.log_message.c_str();
    fout.close();
}

// -----------------------------------------
// s_efftable class implementation

void s_efftable::clear()
{
    table.clear();
}

void s_efftable::add_point(double x, double eta)
{
    table.push_back(s_effmember(x, eta));
};

bool s_efftable::get_point(int index, double& x, double& eta)
{
    if (index > (int)table.size() - 1 || index < 0) return false;

    x = table.at(index).x;
    eta = table.at(index).eta;
    return true;
}

double s_efftable::get_point_eff(int index)
{
    return table.at(index).eta;
}

double s_efftable::get_point_x(int index)
{
    return table.at(index).x;
}

size_t s_efftable::get_size()
{
    return table.size();
}

double s_efftable::interpolate(double x)
{

    double eff = table.front().eta;

    int ind = 0;
    int ni = (int)table.size();
    while (true)
    {
        if (ind == ni - 1)
        {
            eff = table.back().eta;
            break;
        }

        if (x < table.at(ind).x)
        {
            if (ind == 0)
            {
                eff = table.front().eta;
            }
            else
            {
                eff = table.at(ind - 1).eta + (table.at(ind).eta - table.at(ind - 1).eta) * (x - table.at(ind - 1).x) / (table.at(ind).x - table.at(ind - 1).x);
            }
            break;
        }

        ind++;
    }

    return eff;
}

void s_efftable::init_linear_cycle_efficiency_table(double q_pb_min, double q_pb_des, double eta_pb_des, C_csp_power_cycle* power_cycle)
{
    //Cycle efficiency
    this->clear();
    //add zero point
    this->add_point(0., 0.);    //this is required to allow the model to converge

    int neff = 2;   //mjw: if using something other than 2, the linear approximation assumption and associated code in csp_dispatch.cpp/calculate_parameters() needs to be reformulated.
    for (int i = 0; i < neff; i++)
    {
        double x = q_pb_min + (q_pb_des - q_pb_min) / (double)(neff - 1) * i;
        double xf = x / q_pb_des;

        double eta;
        eta = power_cycle->get_efficiency_at_load(xf);
        // TODO: This is a quick fix for design point power but doesn't fix poor low power estimate
        eta += (eta_pb_des - power_cycle->get_efficiency_at_load(1));  // shift efficiency to specific design point

        this->add_point(x, eta);
    }
}

void s_efftable::init_efficiency_ambient_temp_table(double eta_pb_des, double cycle_w_dot_des, C_csp_power_cycle* power_cycle, s_efftable* wcondcoef_table_Tdb)
{
    /*
    Creates cycle efficiency vs. ambient temperature table and condenser load (normalized by gross cycle power rating) vs. ambient temperature table.
    Varies ambient temperature between -10 C and 60 C using 40 uniform points.

    Parameters:
        eta_pb_des -> cycle design efficiency (fractional) [-]
        cycle_w_dot_des -> cycle design gross generation [MWe]
        power_cycle -> pointer to C_csp_power_cycle class (TODO: the above two parameters should be accessable via this pointer)
        wcondcoef_table_Tdb -> pointer to table for this function to populate with data.
    */
    this->clear();
    wcondcoef_table_Tdb->clear();
    int neffT = 40;

    for (int i = 0; i < neffT; i++)
    {
        double T = -10. + 60. / (double)(neffT - 1) * i;
        double wcond;
        double eta = power_cycle->get_efficiency_at_TPH(T, 1., 30., &wcond) / eta_pb_des;

        this->add_point(T, eta);
        wcondcoef_table_Tdb->add_point(T, wcond / cycle_w_dot_des); //fraction of rated gross gen
    }
}

void s_efftable::get_slope_intercept_cycle_linear_performance(double* slope, double* intercept)
{
    //linear power-heat fit requires that the efficiency table has 3 points.. 0->zero point, 1->min load point, 2->max load point. This is created in csp_solver_core::Ssimulate().
    int m = this->get_size() - 1;
    if (m != 2)
        throw C_csp_exception("Model failure during dispatch optimization problem formulation. Ill-formed load table.");
    //get the two points used to create the linear fit
    double q[2], eta[2];
    this->get_point(1, q[0], eta[0]);
    this->get_point(2, q[1], eta[1]);
    //calculate the rate of change in power output versus heat input
    *slope = (q[1] * eta[1] - q[0] * eta[0]) / (q[1] - q[0]);
    //calculate the y-intercept of the linear fit
    *intercept = q[1] * eta[1] - q[1] * *slope;
}
