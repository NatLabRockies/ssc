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


#include "base_dispatch.h"

class etes_dispatch_opt : public base_dispatch_opt
{
public:
    struct s_params
    {
        // Time-indexed parameters
        std::vector<double> time_elapsed; //[hr] Cumulative time elapsed at the end of period t
        std::vector<double> sell_price;   //[$/MWh] Electricity sell price in time t
        std::vector<double> buy_price;    //[$/MWh] Electricity purchase price in time t
        std::vector<double> eta_pb_expected; //
        std::vector<double> w_condf_expected; //

        // initial conditions
        bool is_pb_operating0;      //[-] Power block is operating at the initial time step
        double q_pb0;               //[MWt] Thermal power consumption in the cycle entering the initial time step
        bool is_eh_operating0;      //[-] Electric heaters are operating at the initial time step

        bool is_pb_starting0;       //[-] Power block is starting at the initial time step
        double e_pb_start0;         //[-] Power block start energy consumed before initial time step 
        bool is_eh_starting0;       //[-] Electric heaters are starting at the initial time step 
        double e_eh_start0;         //[-] Power block start energy consumed before initial time step 

        double down_time0;          //[hr] Time that has passed since the cycle has been down before the initial time step
        double up_time0;            //[hr] Time that has passed since the cycle has been up before the initial time step
        double e_tes0;              //[MWht] current stored energy capacity

        // scaler parameters
        double time_weighting;      //[-] Weighting factor that discounts future decisions over more imminent ones
        double dt;                  //[hr] Time step
        double eta_eh;              //[MWt/MWe]  Electric heater efficiency
        double dt_pb_startup_cold;  //[hr] time requirement to start up the power block
        double e_pb_startup_cold;   //[MWht] energy requirement to start up the power block
        double dt_rec_startup;      //[hr] time requirement to start up the electrical heaters
        double e_rec_startup;       //[MWht] energy requirement to start up the electrical heaters
        double e_tes_max;           //[MWht] maximum allowable energy capacity in TES
        double e_tes_min;           //[MWht] minimum allowable energy capacity in TES
        double q_pb_des;            //[MWt] design cycle thermal power input
        double eta_pb_des;          //[MWe/MWt]  Design-point power cycle efficiency
        double q_pb_max;            //[MWt] Maximum allowable thermal energy rate to the cycle
        double q_pb_min;            //[MWt] Minimum allowable thermal energy rate to the cycle
        double q_eh_max;            //[MWt] Maximum allowable power delivery by the electrical heaters when operating
        double q_eh_min;            //[MWt] Minimum allowable power delivery by the electrical heaters when operating

        double csu_cost;            //[$/start] Cycle startup cost
        double hsu_cost;            //[$/start] electrical heater startup cost
        double pen_delta_w;         //[$/MWe-change] Cycle production change penalty

        double down_time_min;       //[hr] Minimum required power cycle down-time
        double up_time_min;         //[hr] Minimum required power cycle up-time


        //double ppa_price_y1;        //[$/MWh] Assumed ppa price for year 1 dispatch

        s_efftable eff_table_load, eff_table_Tdb, wcondcoef_table_Tdb; //Efficiency of the power cycle, condenser power coefs

        s_params() {
            dt = 1.;
            time_weighting = 0.99;
            csu_cost = 10000.;
            hsu_cost = 10.;
            pen_delta_w = 0.1;

            //parameters
            is_pb_operating0 = false;
            q_pb0 = std::numeric_limits<double>::quiet_NaN();
            is_eh_operating0 = false;
            is_pb_starting0 = false;
            e_pb_start0 = std::numeric_limits<double>::quiet_NaN();
            is_eh_starting0 = false;
            e_eh_start0 = std::numeric_limits<double>::quiet_NaN();
            down_time0 = std::numeric_limits<double>::quiet_NaN();
            up_time0 = std::numeric_limits<double>::quiet_NaN();
            e_tes0 = std::numeric_limits<double>::quiet_NaN();
            eta_eh = std::numeric_limits<double>::quiet_NaN();
            dt_pb_startup_cold = std::numeric_limits<double>::quiet_NaN();
            e_pb_startup_cold = std::numeric_limits<double>::quiet_NaN();
            dt_rec_startup = std::numeric_limits<double>::quiet_NaN();   // TODO: Change naming
            e_rec_startup = std::numeric_limits<double>::quiet_NaN();
            e_tes_min = std::numeric_limits<double>::quiet_NaN();     // TODO: this needs to be accounted for       
            e_tes_max = std::numeric_limits<double>::quiet_NaN();
            q_pb_des = std::numeric_limits<double>::quiet_NaN();
            eta_pb_des = std::numeric_limits<double>::quiet_NaN();
            q_pb_max = std::numeric_limits<double>::quiet_NaN();
            q_pb_min = std::numeric_limits<double>::quiet_NaN();
            q_eh_max = std::numeric_limits<double>::quiet_NaN();
            q_eh_min = std::numeric_limits<double>::quiet_NaN();
            down_time_min = std::numeric_limits<double>::quiet_NaN();
            up_time_min = std::numeric_limits<double>::quiet_NaN();
            //ppa_price_y1 = std::numeric_limits<double>::quiet_NaN();
        }

        void clear()
        {
            time_elapsed.clear();
            sell_price.clear();
            buy_price.clear();
            eta_pb_expected.clear();
            w_condf_expected.clear();
        }

        void set_user_params(double disp_time_weighting, double disp_csu_cost, double disp_pen_delta_w,
            double disp_hsu_cost, double disp_down_time_min, double disp_up_time_min) //, double ppa_price_year1/*$/kWh*/)
        {
            time_weighting = disp_time_weighting;
            csu_cost = disp_csu_cost;
            pen_delta_w = disp_pen_delta_w;
            hsu_cost = disp_hsu_cost;
            down_time_min = disp_down_time_min;
            up_time_min = disp_up_time_min;
            //ppa_price_y1 = ppa_price_year1 * 1000.0;    // $/kWh -> $/MWh
        }

    } params;

    struct s_outputs
    {
        std::vector<bool> rec_operation;         //receiver startup ok?
        std::vector<bool> pb_operation;          //power block startup ok?
        std::vector<bool> pb_standby;            //power block standby ok?
        std::vector<double> q_pb_target;         //optimized energy generation (less startup loss)
        std::vector<double> q_pb_standby;        //standby energy allowed
        std::vector<double> q_sf_expected;       //Expected solar field energy generation
        std::vector<double> tes_charge_expected; //Expected thermal energy storage charge state
        std::vector<double> q_pb_startup;        //thermal power going to startup
        std::vector<double> q_rec_startup;       //thermal power going to startup
        std::vector<double> w_pb_target;         //optimized electricity generation

        void clear() {
            rec_operation.clear();
            pb_operation.clear();
            pb_standby.clear();
            q_pb_target.clear();
            q_pb_standby.clear();
            q_sf_expected.clear();
            tes_charge_expected.clear();
            q_pb_startup.clear();
            q_rec_startup.clear();
            w_pb_target.clear();
        }

        void resize(int nt) {
            rec_operation.resize(nt, false);
            pb_operation.resize(nt, false);
            pb_standby.resize(nt, false);
            q_pb_target.resize(nt, 0.);
            q_pb_standby.resize(nt, 0.);
            q_sf_expected.resize(nt, 0.);
            tes_charge_expected.resize(nt, 0.);
            q_pb_startup.resize(nt, 0.);
            q_rec_startup.resize(nt, 0.);
            w_pb_target.resize(nt, 0.);
        }

    } outputs;

    //----- public member functions ----

    etes_dispatch_opt();

    void init(double cycle_q_dot_des, double cycle_eta_des);

    // Set default solver parameters if user did not set them
    void set_default_solver_parameters();

    //check parameters and inputs to make sure everything has been set up correctly
    bool check_setup(int nstep);

    //Update parameter values within the horizon
    bool update_horizon_parameters(C_csp_tou& mc_tou);

    // update dispatch inital conditions
    void update_initial_conditions(double q_dot_to_pb, double T_htf_cold_des, double pc_state_persist);

    //Predict performance out nstep values. 
    bool predict_performance(int step_start, int ntimeints, int divs_per_int);    

    //declare dispatch function in etes_dispatch.cpp
    bool optimize();

    // Set outputs struct based on LP solution -> could move to outputs struct
    void set_outputs_from_lp_solution(lprec* lp, unordered_map<std::string, double>& params);

    //Populated dispatch outputs for csp solver core
    bool set_dispatch_outputs();
};

