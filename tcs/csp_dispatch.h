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

class csp_dispatch_opt : public base_dispatch_opt
{
public:
    struct s_params
    {
        // Time dependent parameters
        std::vector<double> sell_price;          //[- or $/MWh] Price factor indicating market value of generated energy
        std::vector<double> w_lim;		         //[kWe] Limit on net electricity production
        std::vector<double> q_sfavail_expected;  //Expected available solar field energy
        std::vector<double> eta_pb_expected;     //Expected power cycle conversion efficiency (normalized)
        std::vector<double> w_condf_expected;    //Expected condenser loss coefficient
        std::vector<double> wnet_lim_min;        //minimum expected net power at time t before cycle gross falls before limit
        std::vector<double> delta_rs;            //expected proportion of time step used for receiver start up
        std::vector<double> f_pb_op_limit;		//[-] Maximum normalized cycle output

        //TODO: This is not used and probably should be removed.
        std::vector<double> eta_sf_expected;     //Expected solar field thermal efficiency (normalized)

        // Parameters
        double dt;                          //[hr] Time step
        double e_tes_min;                   //[kWht] minimum allowable energy capacity in TES
        double e_tes_max;                   //[kWht] maximum allowable energy capacity in TES
        double e_pb_startup_cold;           //[kWht] energy requirement to start up the power block from cold state
        double e_pb_startup_hot;            //[kWht] energy requirement to start up the power block from standby
        double e_rec_startup;               //[kWht] energy requirement to start up the receiver
        double dt_pb_startup_cold;          //[hr] time requirement to start up the power block from cold state
        double dt_pb_startup_hot;           //[hr] time requirement to start up the power block from hot state
        double dt_rec_startup;              //[hr] time requirement to start up the receiver
        double tes_degrade_rate;            //IN [1/hr] Fractional energy loss from tes per hour
        double q_pb_standby;                //[kWt] power requirement to maintain the power block in standby mode
        double q_pb_des;                    //[kWe] design cycle thermal power input
        double eta_pb_des;                  //[-] design cycle efficiency
        double q_pb_max;                    //[kWt] Maximum allowable thermal energy rate to the cycle
        double q_pb_min;                    //[kWt] Minimum allowable thermal energy rate to the cycle
        double q_rec_min;                   //[kWt] Minimum allowable power delivery by the receiver when operating
        double w_rec_pump;                  //[kWe/kWt] Pumping parasitic power per thermal energy produced
        double sf_effadj;                   //[-] 0..1 Solar field efficiency adjustment
        double time_weighting;              //[-] Weighting factor that discounts future decisions over more imminent ones
        double rsu_cost;                    //[$/start] Receiver startup cost
        double csu_cost;                    //[$/start] Cycle startup cost
        double pen_delta_w;                 //[$/kWe-change] Cycle production change penalty
        double q_rec_standby;               //[kWt] Receiver standby thermal power consumption fraction

        bool can_cycle_use_standby;         //[-] Can the cycle use standby operation?
        bool is_parallel_heater;            //[-] Is there a heater parallel to the receiver?
        double q_eh_max;                    //[kWt] Maximum allowable power delivery by the electrical heaters when operating
        double q_eh_min;                    //[kWt] Minimum allowable power delivery by the electrical heaters when operating
        double eta_eh;                      //[-] Electric resistance heating sub-system efficiency
        double hsu_cost;                    //[$/start] Heater startup cost

        // Initial Conditions
        bool is_rec_operating0;             //receiver is operating at the initial time step
        bool is_pb_operating0;              //Power block is operating at the initial time step
        bool is_pb_standby0;                //Power block is in standby at the initial time step
        double q_pb0;                       //[kWt] Thermal power consumption in the cycle entering the initial time step
        double e_tes0;                      //[kWht] current stored energy capacity

        // Parasitic loads
		double w_rec_ht;			        //[kW-hr] Heat trace power during receiver startup
		double w_track;				        //[kWe] Heliostat tracing power
		double w_stow;				        //[kWe-hr] Heliostat stow electricity requirement
		double w_cycle_standby;		        //[kWe] Cycle HTF pumping power during standby
		double w_cycle_pump;		        //[kWe/kWt] Cycle HTF pumping power per thermal energy consumed

        double inventory_incentive;         //[-]   Terminal storage inventory objective incentive multiplier
        //double ppa_price_y1;                //[$/MWh] Assumed ppa price for year 1 dispatch

        s_efftable eff_table_load, eff_table_Tdb, wcondcoef_table_Tdb;  //Efficiency of the power cycle, condenser power coefs

        s_params() {
            is_pb_operating0 = false;
            is_pb_standby0 = false;
            is_rec_operating0 = false;
            dt = 1.;
            q_pb0 = std::numeric_limits<double>::quiet_NaN();
            e_tes0 = std::numeric_limits<double>::quiet_NaN();
            e_tes_min = std::numeric_limits<double>::quiet_NaN();
            e_tes_max = std::numeric_limits<double>::quiet_NaN();
            q_pb_standby = std::numeric_limits<double>::quiet_NaN();
            e_pb_startup_cold = std::numeric_limits<double>::quiet_NaN();
            e_pb_startup_hot = std::numeric_limits<double>::quiet_NaN();
            e_rec_startup = std::numeric_limits<double>::quiet_NaN();
            dt_pb_startup_cold = std::numeric_limits<double>::quiet_NaN();
            dt_pb_startup_hot = std::numeric_limits<double>::quiet_NaN();
            dt_rec_startup = std::numeric_limits<double>::quiet_NaN();
            tes_degrade_rate = std::numeric_limits<double>::quiet_NaN();
            q_pb_max = std::numeric_limits<double>::quiet_NaN();
            q_pb_min = std::numeric_limits<double>::quiet_NaN();
            q_rec_min = std::numeric_limits<double>::quiet_NaN();
            w_rec_pump = std::numeric_limits<double>::quiet_NaN();
            q_pb_des = std::numeric_limits<double>::quiet_NaN();
            eta_pb_des = std::numeric_limits<double>::quiet_NaN();
            inventory_incentive = 0.;
            sf_effadj = 1.;

            time_weighting = 0.99;
            rsu_cost = 952.;
            csu_cost = 10000;
            pen_delta_w = 0.1;
            q_rec_standby = 9.e99;
            w_rec_ht = 0.0;
            w_track = std::numeric_limits<double>::quiet_NaN();
            w_stow = std::numeric_limits<double>::quiet_NaN();
            w_cycle_standby = std::numeric_limits<double>::quiet_NaN();
            w_cycle_pump = std::numeric_limits<double>::quiet_NaN();
            is_parallel_heater = false;
            q_eh_max = 0.0;
            q_eh_min = 0.0;
            eta_eh = 1.0;
            hsu_cost = 10.;
            can_cycle_use_standby = false;
            //ppa_price_y1 = std::numeric_limits<double>::quiet_NaN();
        }

        void clear()
        {
            sell_price.clear();
            w_lim.clear();
            q_sfavail_expected.clear();
            eta_pb_expected.clear();
            w_condf_expected.clear();
            wnet_lim_min.clear();
            delta_rs.clear();
            f_pb_op_limit.clear();
            eta_sf_expected.clear();
        }

        void set_user_params(bool cycle_use_standby, double disp_time_weighting,
            double disp_rsu_cost, double disp_hsu_cost, double disp_csu_cost, double disp_pen_delta_w, double disp_inventory_incentive,
            double rec_standby_loss, double rec_heattrace) //, double ppa_price_year1/*$/kWh*/)
        {
            can_cycle_use_standby = cycle_use_standby;
            time_weighting = disp_time_weighting;
            rsu_cost = disp_rsu_cost;
            hsu_cost = disp_hsu_cost;
            csu_cost = disp_csu_cost;
            pen_delta_w = disp_pen_delta_w;
            inventory_incentive = disp_inventory_incentive;
            q_rec_standby = rec_standby_loss;   //TODO: why are these grouped here?
            w_rec_ht = rec_heattrace;           //TODO: why are these grouped here?
            //ppa_price_y1 = ppa_price_year1 * 1000.0;    // $/kWh -> $/MWh
        }
    } params;

    struct s_outputs
    {
        std::vector<bool> rec_operation;         // [-] Receiver startup ok?
        std::vector<bool> pb_operation;          // [-] Power block startup ok?
        std::vector<bool> pb_standby;            // [-] Power block standby ok?
        std::vector<double> q_pb_target;         // [MWt] Optimized energy generation (less startup loss)
        std::vector<double> q_pb_standby;        // [MWt] standby energy allowed
        std::vector<double> q_sf_expected;       // [MWt] Expected solar field energy generation
        std::vector<double> tes_charge_expected; // [MWht] Expected thermal energy storage charge state
        std::vector<double> q_pb_startup;        // [MWt] Thermal power going to startup
        std::vector<double> q_rec_startup;       // [MWt] Thermal Power going to startup
        std::vector<double> w_pb_target;         // [MWe] Optimized electricity generation

        std::vector<bool> htr_operation;         // [-] is heater allowed to operate
        std::vector<double> q_eh_target;         // [MWt] Heater target thermal power

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

            htr_operation.clear();
            q_eh_target.clear();
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

            htr_operation.resize(nt, false);
            q_eh_target.resize(nt, 0.);
        }

    } outputs;
    
    //----- public member functions ----

    csp_dispatch_opt();

    void init(double cycle_q_dot_des, double cycle_eta_des);

    // Set default solver parameters if user did not set them
    void set_default_solver_parameters();

    //check parameters and inputs to make sure everything has been set up correctly
    bool check_setup(int nstep);

    //Update parameter values within the horizon
    bool update_horizon_parameters(C_csp_tou &mc_tou);

    // update dispatch initial conditions
    void update_initial_conditions(double q_dot_to_pb, double T_htf_cold_des, double pc_state_persist);

    //Predict performance out nstep values. 
    bool predict_performance(int step_start, int ntimeints, int divs_per_int);    

    //declare dispatch function in csp_dispatch.cpp
    bool optimize();

    std::string write_ampl();
    bool optimize_ampl();

    // Set outputs struct based on LP solution -> could move to outputs struct
    void set_outputs_from_lp_solution(lprec* lp, unordered_map<std::string, double>& params);

    bool set_dispatch_outputs();
};
