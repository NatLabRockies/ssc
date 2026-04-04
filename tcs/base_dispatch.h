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

#ifndef __base_dispatch_
#define __base_dispatch_

#include "csp_solver_core.h"

class base_dispatch_opt
{
protected:
    int  m_nstep_opt;                   //number of time steps in the optimized array
    bool m_is_weather_setup;            //bool indicating whether the weather has been copied
    
    void clear_output();

    void not_implemented_function(std::string function_name);

public:
    int m_current_read_step;           //current step to read from optimization results

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
    } solver_params;

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

    enum termination_flags {
        optimal,
        iteration,
        timelimit,
        mipgap,
        mipgap_lpsolve,
        failed
    };

    struct s_solver_outputs
    {
        bool last_opt_successful;       //last optimization run was successful?
        double objective;
        double objective_relaxed;
        double rel_mip_gap;
        int solve_iter;                 //Number of iterations required to solve
        int solve_state;                // TODO: Set this to NOTRUN or NOT_SOLVED depending on solver
        termination_flags termination_flag;                //Flag specifying information about termination result
        double solve_time;
        int presolve_nconstr;
        int presolve_nvar;

        s_solver_outputs() {
            last_opt_successful = false;
            objective = std::numeric_limits<double>::quiet_NaN();
            objective_relaxed = std::numeric_limits<double>::quiet_NaN();
            rel_mip_gap = std::numeric_limits<double>::quiet_NaN();
            solve_iter = 0;
            solve_state = -1;
            termination_flag = termination_flags::failed;
            presolve_nconstr = 0;
            solve_time = 0.;
            presolve_nvar = 0;
        }

        void clear_output() {
            s_solver_outputs();
        }

    } solver_outputs;

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

    //Used by cmod to get dispatch annual stats on solves
    virtual void count_solutions_by_type(std::vector<int>& flag, int dispatch_freq, std::string& log_msg);

    //Calculates average relative mip gap of suboptimal solutions
    virtual double calc_avg_subopt_gap(std::vector<double>& gap, std::vector<int>& flag, int dispatch_freq);

    //Dispatch update and result print to screen
    virtual void print_dispatch_update();

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
