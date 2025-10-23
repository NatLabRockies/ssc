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

class cst_iph_dispatch_opt : public base_dispatch_opt
{
public:
    struct s_params
    {
        // Time dependent parameters
        std::vector<double> elec_price;          //[$/MWhe] Electricity price
        std::vector<double> heat_cost;           //[$/MWht] Cost of back-up heat
        std::vector<double> heat_load;		     //[MWt] Limit on net electricity production
        std::vector<double> q_sfavail_expected;  //[MWt] Expected available solar field energy
        std::vector<double> delta_rs;            //[hr] Expected proportion of time step used for receiver start up
        std::vector<double> eta_sf_expected;     //[-] Expected solar field thermal efficiency (normalized)

        // Parameters
        double dt;                          //[hr] Time step
        double e_tes_min;                   //[MWht] minimum allowable energy capacity in TES
        double e_tes_max;                   //[MWht] maximum allowable energy capacity in TES
        double e_rec_startup;               //[MWht] energy requirement to start up the receiver
        double dt_rec_startup;              //[hr] time requirement to start up the receiver
        double tes_degrade_rate;            //IN [1/hr] Fractional energy loss from tes per hour -> NOT Used
        double q_hs_des;                    //[kWe] design cycle thermal power input
        double eta_hs_des;                  //[-] design cycle efficiency
        double q_hs_max;                    //[MWt] Maximum allowable thermal energy rate to the heat sink (load)
        double q_hs_min;                    //[MWt] Minimum allowable thermal energy rate to the heat sink (load)
        double q_rec_min;                   //[MWt] Minimum allowable power delivery by the receiver when operating
        double w_rec_pump;                  //[MWe/MWt] Pumping parasitic power per thermal energy produced
        double time_weighting;              //[-] Weighting factor that discounts future decisions over more imminent ones

        bool is_parallel_heater;            //[-] Is there a heater parallel to the receiver?
        double q_eh_max;                    //[MWt] Maximum allowable power delivery by the electrical heaters when operating
        double q_eh_min;                    //[MWt] Minimum allowable power delivery by the electrical heaters when operating
        double eta_eh;                      //[-] Electric resistance heating sub-system efficiency

        // Initial Conditions
        bool is_rec_operating0;             //receiver is operating at the initial time step
        bool is_pb_operating0;              //Power block is operating at the initial time step
        bool is_pb_standby0;                //Power block is in standby at the initial time step
        double q_pb0;                       //[MWt] Thermal power consumption in the cycle entering the initial time step
        double e_tes0;                      //[MWht] current stored energy capacity

        // Parasitic loads
		double w_rec_ht;			        //[MW-hr] Heat trace power during receiver startup
		double w_track;				        //[MWe] Heliostat tracing power
		double w_stow;				        //[MWe-hr] Heliostat stow electricity requirement
		double w_hs_pump;		            //[MWe/MWt] Heat sink (load) HTF pumping power per thermal energy consumed

        s_params() {
            is_pb_operating0 = false;
            is_pb_standby0 = false;
            is_rec_operating0 = false;
            dt = 1.;
            q_pb0 = std::numeric_limits<double>::quiet_NaN();
            e_tes0 = std::numeric_limits<double>::quiet_NaN();
            e_tes_min = std::numeric_limits<double>::quiet_NaN();
            e_tes_max = std::numeric_limits<double>::quiet_NaN();
            e_rec_startup = std::numeric_limits<double>::quiet_NaN();
            dt_rec_startup = std::numeric_limits<double>::quiet_NaN();
            tes_degrade_rate = std::numeric_limits<double>::quiet_NaN();
            q_hs_max = std::numeric_limits<double>::quiet_NaN();
            q_hs_min = std::numeric_limits<double>::quiet_NaN();
            q_rec_min = std::numeric_limits<double>::quiet_NaN();
            w_rec_pump = std::numeric_limits<double>::quiet_NaN();
            q_hs_des = std::numeric_limits<double>::quiet_NaN();
            eta_hs_des = std::numeric_limits<double>::quiet_NaN();

            time_weighting = 0.99;
            w_rec_ht = 0.0;
            w_track = std::numeric_limits<double>::quiet_NaN();
            w_stow = std::numeric_limits<double>::quiet_NaN();
            w_hs_pump = std::numeric_limits<double>::quiet_NaN();
            is_parallel_heater = false;
            q_eh_max = 0.0;
            q_eh_min = 0.0;
            eta_eh = 1.0;
        }

        void clear()
        {
            elec_price.clear();
            heat_cost.clear();
            heat_load.clear();
            q_sfavail_expected.clear();
            delta_rs.clear();
            eta_sf_expected.clear();
        }

        void set_user_params(double disp_time_weighting, double rec_heattrace)
        {
            time_weighting = disp_time_weighting;
            w_rec_ht = rec_heattrace;           //TODO: why are this grouped here? - We should create a getter function for the receiver-collector class
        }
    } params;

    struct s_outputs
    {
        std::vector<bool> rec_operation;         // [-] Receiver startup ok?
        std::vector<bool> pb_operation;          // [-] Power block startup ok?
        std::vector<bool> pb_standby;            // [-] Power block standby ok?
        std::vector<double> q_pb_target;         // [MWt] Optimized energy generation (less startup loss)
        std::vector<double> q_sf_expected;       // [MWt] Expected solar field energy generation
        std::vector<double> tes_charge_expected; // [MWht] Expected thermal energy storage charge state
        std::vector<double> q_rec_startup;       // [MWt] Thermal Power going to startup

        std::vector<bool> htr_operation;         // [-] is heater allowed to operate
        std::vector<double> q_eh_target;         // [MWt] Heater target thermal power

        void clear() {
            rec_operation.clear();
            pb_operation.clear();
            pb_standby.clear();
            q_pb_target.clear();
            q_sf_expected.clear();
            tes_charge_expected.clear();
            q_rec_startup.clear();

            htr_operation.clear();
            q_eh_target.clear();
        }

        void resize(int nt) {
            rec_operation.resize(nt, false);
            pb_operation.resize(nt, false);
            pb_standby.resize(nt, false);
            q_pb_target.resize(nt, 0.);
            q_sf_expected.resize(nt, 0.);
            tes_charge_expected.resize(nt, 0.);
            q_rec_startup.resize(nt, 0.);

            htr_operation.resize(nt, false);
            q_eh_target.resize(nt, 0.);
        }

    } outputs;
    
    //----- public member functions ----

    cst_iph_dispatch_opt();

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

    // Calculate parameter values
    void calculate_parameters(unordered_map<std::string, double>& pars);

    //declare dispatch function in csp_dispatch.cpp
    bool optimize();

    // Set outputs struct based on LP solution -> could move to outputs struct
    void set_outputs_from_lp_solution(lprec* lp, unordered_map<std::string, double>& params);

    bool set_dispatch_outputs();
};
