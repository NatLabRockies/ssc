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
//#pragma once
//#pragma warning(disable: 4290)  // ignore warning: 'C++ exception specification ignored except to indicate a function is not __declspec(nothrow)'

#ifndef __base_dispatch_
#define __base_dispatch_

#include "dispatch_builder.h"
#include "csp_solver_core.h"

/* Suboptimal dispatch flags */
#define INTERATION              1
#define TIMELIMIT               2
#define MIPGAP                  3
#define MIPGAPLPSOLVE           4
#define FAILED                  5

class base_dispatch_opt
{
protected:
    int  m_nstep_opt;                   //number of time steps in the optimized array
    bool m_is_weather_setup;            //bool indicating whether the weather has been copied
    
    void clear_output();

private:
    void not_implemented_function(std::string function_name);

public:
    int m_current_read_step;           //current step to read from optimization results

    s_solver_params solver_params;

    struct S_pointers
    {
        C_csp_weatherreader m_weather;       //Pointer to weather file
        C_csp_solver_sim_info *siminfo;      //Pointer to existing simulation info object
        C_csp_collector_receiver *col_rec;   //Pointer to collector/receiver object
		C_csp_power_cycle *mpc_pc;	         //Pointer to csp power cycle class object
        C_csp_tes *tes;                      //Pointer to tes class object
		C_csp_messages *messages;            //Pointer to message structure
        C_csp_collector_receiver* par_htr;   //Pointer to parallel heater if it exist, else NULL

        S_pointers()
        {
            //m_weather = nullptr;
            siminfo = nullptr;
            col_rec = nullptr;
            mpc_pc = nullptr;
            tes = nullptr;
            messages = nullptr;
            par_htr = nullptr;
        }

        void set_pointers(C_csp_weatherreader &weather,
            C_csp_collector_receiver *collector_receiver,
            C_csp_power_cycle *power_cycle,
            C_csp_tes *thermal_es,
            C_csp_messages *csp_messages,
            C_csp_solver_sim_info *sim_info,
            C_csp_collector_receiver *heater)
        {
            m_weather = weather;    // Todo: technically not a pointer
            col_rec = collector_receiver;
            mpc_pc = power_cycle;
            tes = thermal_es;
            messages = csp_messages;
            siminfo = sim_info;
            par_htr = heater;
        }

    } pointers;

    struct s_lp_outputs
    {
        bool last_opt_successful;     //last optimization run was successful?
        double objective;
        double objective_relaxed;
        double rel_mip_gap;
        int solve_iter;                 //Number of iterations required to solve
        int solve_state;
        int subopt_flag;                //Flag specifing information about LPSolve suboptimal result
        double solve_time;
        int presolve_nconstr;
        int presolve_nvar;

        s_lp_outputs() {
            last_opt_successful = false;
            objective = std::numeric_limits<double>::quiet_NaN();
            objective_relaxed = std::numeric_limits<double>::quiet_NaN();
            rel_mip_gap = std::numeric_limits<double>::quiet_NaN();
            solve_iter = 0;
            solve_state = NOTRUN;
            subopt_flag = OPTIMAL;
            presolve_nconstr = 0;
            solve_time = 0.;
            presolve_nvar = 0;
        }

        void clear_output() {
            s_lp_outputs();
        }

    } lp_outputs;

    struct s_disp_outputs
    {
        double time_last = -9999.;

        bool is_rec_su_allowed = false;
        bool is_pc_sb_allowed = false;
        bool is_pc_su_allowed = false;

        double q_pc_target = 0.;
        double q_dot_pc_max = 0.;
        double q_dot_elec_to_CR_heat = 0.;
        double qsf_expect = 0.;
        double qsfprod_expect = 0.;
        double qsfsu_expect = 0.;
        double tes_expect = 0.;
        double etasf_expect = 0.;
        double etapb_expect = 0.;
        double qpbsu_expect = 0.;
        double wpb_expect = 0.;
        double rev_expect = 0.;

        bool is_eh_su_allowed = false;
        double q_eh_target = 0.;

    } disp_outputs;

    //----- public member functions ----
    base_dispatch_opt();

    virtual void init(double cycle_q_dot_des, double cycle_eta_des);

    // Set default solver parameters if user did not set them
    virtual void set_default_solver_parameters();

    //check parameters and inputs to make sure everything has been set up correctly
    bool check_setup();

    //Update parameters for the horizon
    virtual bool update_horizon_parameters(C_csp_tou &mc_tou);

    //Predict performance out nstep values. 
    virtual bool predict_performance(int step_start, int ntimeints, int divs_per_int);

    //Updated dispatch initial conditions
    virtual void update_initial_conditions(double q_dot_to_pb, double T_htf_cold_des, double pc_state_persist);

    //declare dispatch function
    virtual bool optimize();

    //Functions to write AMPL data files and solve AMPL model
    virtual std::string write_ampl();
    virtual bool optimize_ampl();

    //Populated dispatch outputs for csp solver core
    virtual bool set_dispatch_outputs();

    //Constructs lp model
    lprec* construct_lp_model(optimization_vars* opt_vars);

    //Set LPsolve solver presolve settings and bb rules
    void setup_solver_presolve_bbrules(lprec* lp);

    //Problem scaling loop algorithm
    bool problem_scaling_solve_loop(lprec* lp);

    //Set LPsolve outputs
    void set_lp_solve_outputs(lprec* lp);

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

    // Print dispatch solver log to file for debugging solver
    void print_log_to_file();
};

struct s_efftable
{
private:
    struct s_effmember
    {
        double x;
        double eta;

        s_effmember() {};
        s_effmember(double _x, double _eta)
        {
            x = _x;
            eta = _eta;
        };
    };
    std::vector<s_effmember> table;

public:

    void clear();

    void add_point(double x, double eta);

    bool get_point(int index, double& x, double& eta);

    double get_point_eff(int index);

    double get_point_x(int index);

    size_t get_size();

    double interpolate(double x);

    // Initialize linear cycle performance approximation -> used to calculate slope and intercept parameters
    void init_linear_cycle_efficiency_table(double q_pb_min, double q_pb_des, double eta_pb_des, C_csp_power_cycle* power_cycle);

    // Initializes cycle efficiency vs. ambient temperature table and normalized condenser power  vs. ambient temperature table.
    void init_efficiency_ambient_temp_table(double eta_pb_des, double cycle_w_dot_des, C_csp_power_cycle* power_cycle, s_efftable* wcondcoef_table_Tdb);

    void get_slope_intercept_cycle_linear_performance(double* slope, double* intercept);
};


#endif //__base_dispatch_
