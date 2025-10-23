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
#include "cst_iph_dispatch.h"

/*

Careful with namespaces in this file.. importing the LPsolve library introduces new macro definitions
and function definitions.

*/

cst_iph_dispatch_opt::cst_iph_dispatch_opt()
{
    outputs.clear();
    params.clear();
}

void cst_iph_dispatch_opt::init(double hs_q_dot_des, double hs_eta_des)
{
    set_default_solver_parameters();

    params.clear();

    params.dt = 1. / (double)solver_params.steps_per_hour;  //hr

    params.q_hs_max = pointers.mpc_pc->get_max_thermal_power();
    params.q_hs_min = pointers.mpc_pc->get_min_thermal_power();
    params.w_hs_pump = pointers.mpc_pc->get_htf_pumping_parasitic_coef();

    params.dt_rec_startup = pointers.col_rec->get_startup_time() / 3600.;
    params.e_rec_startup = pointers.col_rec->get_startup_energy();
    params.q_rec_min = pointers.col_rec->get_min_power_delivery();
    params.w_rec_pump = pointers.col_rec->get_pumping_parasitic_coef();
    params.w_track = pointers.col_rec->get_tracking_power();
    params.w_stow = pointers.col_rec->get_col_startup_power();

    params.e_tes0 = pointers.tes->get_initial_charge_energy();
    params.e_tes_min = pointers.tes->get_min_charge_energy();
    params.e_tes_max = pointers.tes->get_max_charge_energy();
    params.tes_degrade_rate = pointers.tes->get_degradation_rate();

    //heater params
    if (pointers.par_htr != NULL) {
        params.q_eh_min = pointers.par_htr->get_min_power_delivery() * ( 1 + 1e-8 ); // ensures controller doesn't shut down heater at minimum load
        params.q_eh_max = pointers.par_htr->get_max_power_delivery(std::numeric_limits<double>::quiet_NaN());
        params.eta_eh = pointers.par_htr->get_design_electric_to_heat_cop();
        params.is_parallel_heater = true;
    }
    else {
        params.is_parallel_heater = false;
    }

    params.q_hs_des = hs_q_dot_des;
    params.eta_hs_des = hs_eta_des;

}

void cst_iph_dispatch_opt::set_default_solver_parameters()
{
    /*
    The pre-solve options have been tested and show that the optimal combination of options is as set below.

    Optimality was measured by observing the number of constraints + number of variables that resulted an an
    annual-averaged basis from each combination.
    */

    /*
    From the genetic algorithm:

    Presolve        512
    Branch&Bound    0 32 64 128 256 1024
    Scaling         7 16 32 64 128


    ----- keep a record of what's been tried historically for each setting ----

    >>> set_presolve
        PRESOLVE_ROWS + PRESOLVE_COLS + PRESOLVE_REDUCEMIP + PRESOLVE_ELIMEQ2 :: original from 2015
        PRESOLVE_ROWS + PRESOLVE_COLS + PRESOLVE_ELIMEQ2 + PRESOLVE_PROBEFIX :: version used as of 12/5/2016
        PRESOLVE_IMPLIEDFREE :: genetic algorithm from 2015

    >> set_bb_rule
        -- combos set for older problem formulation, appropriate as of mid-2015
        NODE_PSEUDOCOSTSELECT + NODE_RCOSTFIXING :: original
        NODE_PSEUDORATIOSELECT + NODE_BREADTHFIRSTMODE :: original v2
        NODE_PSEUDONONINTSELECT + NODE_GREEDYMODE + NODE_DYNAMICMODE + NODE_RCOSTFIXING :: 5m30s, 10.24c
        NODE_PSEUDOCOSTSELECT + NODE_RANDOMIZEMODE + NODE_RCOSTFIXING :: 5m20s, 10.17c
        NODE_GREEDYMODE + NODE_PSEUDOCOSTMODE + NODE_DEPTHFIRSTMODE + NODE_RANDOMIZEMODE + NODE_DYNAMICMODE :: optimal from genetic algorithm
        NODE_PSEUDOCOSTSELECT + NODE_RANDOMIZEMODE :: optimal from independent optimization, THIS VERSION CURRENT AS OF 12/5/2016
    */
    // If user did not set solver parameters, set defaults specific to CSP dispatch model
    if (solver_params.presolve_type < 0)
        solver_params.presolve_type = PRESOLVE_ROWS + PRESOLVE_COLS + PRESOLVE_ELIMEQ2 + PRESOLVE_PROBEFIX;
    if (solver_params.bb_type < 0)
        solver_params.bb_type = NODE_PSEUDOCOSTSELECT + NODE_DYNAMICMODE;
        //solver_params.bb_type = NODE_PSEUDOCOSTSELECT + NODE_AUTOORDER;
    if (solver_params.scaling_type < 0)
        solver_params.scaling_type = SCALE_MEAN + SCALE_LOGARITHMIC + SCALE_POWER2 + SCALE_EQUILIBRATE + SCALE_INTEGERS;
    //SCALE_CURTISREID + SCALE_LOGARITHMIC + SCALE_POWER2 + SCALE_EQUILIBRATE + SCALE_INTEGERS   //genetic algorithm
}

bool cst_iph_dispatch_opt::check_setup(int num_step)
{
    //check parameters and inputs to make sure everything has been set up correctly
    if ((int)params.elec_price.size() < num_step)   return false;
    if ((int)params.heat_cost.size() < num_step)   return false;
    if ((int)params.heat_load.size() < num_step)   return false;

    if ((int)params.q_sfavail_expected.size() < num_step)   return false;

    return base_dispatch_opt::check_setup();
}

bool cst_iph_dispatch_opt::update_horizon_parameters(C_csp_tou& mc_tou)
{
    //get price signal and electricity generation limits
    int num_steps = solver_params.optimize_horizon * solver_params.steps_per_hour;
    params.elec_price.clear();
    params.elec_price.resize(num_steps, 1.);
    params.heat_cost.clear();
    params.heat_cost.resize(num_steps, 1.);
    params.heat_load.clear();
    params.heat_load.resize(num_steps, 1.e99);

    double sec_per_step = 3600. / (double)solver_params.steps_per_hour;
    double q_dot_max = params.q_hs_max * params.eta_hs_des;                 //[kWe] TODO: Change this to heat only (remove efficiency)
    for (int t = 0; t < num_steps; t++) {
        C_csp_tou::S_csp_tou_outputs tou_outputs;
        mc_tou.call(pointers.siminfo->ms_ts.m_time + t * sec_per_step, tou_outputs);
        params.elec_price.at(t) = tou_outputs.m_elec_price * 1000.0;    // $/kWhe -> $/MWhe
        params.heat_cost.at(t) = tou_outputs.m_heat_price * 1000.0;     // $/kWht -> $/MWht
        params.heat_load.at(t) = tou_outputs.m_wlim_dispatch * q_dot_max;
    }
    return true;
}

void cst_iph_dispatch_opt::update_initial_conditions(double q_dot_to_pb, double T_htf_cold_des, double pc_state_persist)
{
    //note the states of the power cycle and receiver
    params.is_pb_operating0 = pointers.mpc_pc->get_operating_state() == C_csp_power_cycle::ON;
    params.is_pb_standby0 = pointers.mpc_pc->get_operating_state() == C_csp_power_cycle::STANDBY;
    params.is_rec_operating0 = pointers.col_rec->get_operating_state() == C_csp_collector_receiver::ON;

    params.q_pb0 = q_dot_to_pb;

    //Note the state of the thermal energy storage system
    double q_disch, m_dot_disch, T_tes_return;
    pointers.tes->discharge_avail_est(T_htf_cold_des, pointers.siminfo->ms_ts.m_step, q_disch, m_dot_disch, T_tes_return);
    params.e_tes0 = q_disch * pointers.siminfo->ms_ts.m_step / 3600. + params.e_tes_min;        //MWh
    if (params.e_tes0 < params.e_tes_min)
        params.e_tes0 = params.e_tes_min;
    if (params.e_tes0 > params.e_tes_max)
        params.e_tes0 = params.e_tes_max;
}

bool cst_iph_dispatch_opt::predict_performance(int step_start, int ntimeints, int divs_per_int)
{
    //Step number - 1-based index for first hour of the year.

    //save step count
    m_nstep_opt = ntimeints;

    //Predict performance out nstep values.
    params.eta_sf_expected.clear();         //thermal efficiency
    params.q_sfavail_expected.clear();      //predicted field energy output

    //create the sim info
    C_csp_solver_sim_info simloc;
	simloc.ms_ts.m_step = pointers.siminfo->ms_ts.m_step;

    double Asf = pointers.col_rec->get_collector_area();

    double ave_weight = 1./(double)divs_per_int;

    for(int i=0; i<m_nstep_opt; i++)
    {
        //initialize hourly average values
        double therm_eff_ave = 0.;
        double q_inc_ave = 0.;

        for(int j=0; j<divs_per_int; j++)     //take averages over hour if needed
        {

            //jump to the current step
            if(! pointers.m_weather.read_time_step( step_start+i*divs_per_int+j, simloc ) )
                return false;

            //get DNI
            double dni = pointers.m_weather.ms_outputs.m_beam;
            if( pointers.m_weather.ms_outputs.m_solzen > 90. || dni < 0. )
                dni = 0.;

            //get optical efficiency
            double opt_eff = pointers.col_rec->calculate_optical_efficiency(pointers.m_weather.ms_outputs, simloc);

            double q_inc = Asf * opt_eff * dni * 1.e-6; //MW

            //get thermal efficiency
            double therm_eff = pointers.col_rec->calculate_thermal_efficiency_approx(pointers.m_weather.ms_outputs, q_inc, simloc);
            therm_eff_ave += therm_eff * ave_weight;

            //store the predicted field energy output
            // use the cold tank temperature as a surrogate for the loop inlet temperature, as it
            //  closely follows the loop inlet temperature, and is more representative over the
            //  two-day lookahead period than the loop inlet temperature (design or actual) at the
            //  same point in time
            double T_tank_cold = pointers.tes->get_cold_temp() - 273.15;   // [C]
            double q_max = pointers.col_rec->get_max_power_delivery(T_tank_cold);     // [kW]
            q_inc_ave += (std::min)(q_max, q_inc * therm_eff * ave_weight);

		    simloc.ms_ts.m_time += simloc.ms_ts.m_step;
            pointers.m_weather.converged();
        }

        //-----report hourly averages
        //thermal efficiency
        params.eta_sf_expected.push_back(therm_eff_ave);
        //predicted field energy output
        params.q_sfavail_expected.push_back( q_inc_ave );
    }

    if(! check_setup(m_nstep_opt) )
        throw C_csp_exception("Dispatch optimization precheck failed.");
    
    return true;
}

void cst_iph_dispatch_opt::calculate_parameters(unordered_map<std::string, double> &pars)
{
    /* 
    A central location for making sure the parameters from the model are accurately calculated for use in
    the dispatch optimization model.
    */
        pars["delta"] = params.dt;
        pars["Eu"] = params.e_tes_max ;
        pars["Er"] = params.e_rec_startup ;
        pars["Qu"] = params.q_hs_des ;
        pars["Ql"] = params.q_hs_min ;
        pars["Qru"] = params.e_rec_startup / params.dt_rec_startup;
        pars["Qrl"] = params.q_rec_min ;
        pars["Lr"] = params.w_rec_pump ;
        pars["Lc"] = params.w_hs_pump;
        pars["Wh"] = params.w_track;
        pars["Ehs"] = params.w_stow;
        pars["Wrsb"] = params.w_rec_ht;

        if (params.is_parallel_heater) {
            pars["Qehu"] = params.q_eh_max;
            pars["Qehl"] = params.q_eh_min;
            pars["eta_eh"] = params.eta_eh;
        }

        // Initial conditions
        pars["s0"] = params.e_tes0;

        // Receiver start-up time - TODO: We might be able to remove this
        params.delta_rs.resize(m_nstep_opt);
        for(int t=0; t<m_nstep_opt-1; t++)
        {
            double delta_rec_startup = (std::min)(1., (std::max)(params.e_rec_startup / (std::max)(params.q_sfavail_expected.at(t + 1)*pars["delta"], 1.), params.dt_rec_startup / pars["delta"]));
            params.delta_rs.at(t) = delta_rec_startup;
        }

        //Set by user
        pars["disp_time_weighting"] = params.time_weighting;
};

bool cst_iph_dispatch_opt::optimize()
{

    //First check to see whether we should call the AMPL engine instead. 
    if( solver_params.is_ampl_engine )
    {
        return optimize_ampl();
    }

    /* 
    Formulate the optimization problem for dispatch generation. We are trying to minimize operating cost subject to heat load and inventory
    constraints.
    
    
    Variables
    -------------------------------------------------------------
    Continuous
    -------------------------------------------------------------
    xr          MWt     Power delivered by the receiver to TES at time t
    xrsu        MWt     Power used by the receiver for start-up
    ursu        MWt     Receiver accumulated start-up thermal power at time t
    x           MWt	    Thermal power from TES to heat load at time t 
    s           MWht    TES reserve quantity at time t

    ======= When parallel heater is enabled =========
    qeh         MWt     Thermal power to TES by the electric heaters at time t

    -------------------------------------------------------------
    Binary
    -------------------------------------------------------------
    yr              1 if receiver is generating ``usable'' thermal power at time t; 0 otherwise 
    yrsu            1 if receiver is starting up at time t; 0 otherwise

    ======= When parallel heater is enabled =========
    yeh             1 if electrical heaters are operating at time t; 0 otherwise
    yreh            1 if receiver and electrical heaters are operating at time t; 0 otherwise (for defocusing rule)
    yhsup           1 if electrical heaters startup cost is enforced at time t; 0 otherwise

    -------------------------------------------------------------
    Questions to be answered:
        - Do we want to model receiver and heat process startup?
            Solar field: Yes
            Heat load: No
                Would the TES/heat load HX have a "stand-by" or a "start-up"? -> going to ignore to start
        - What should be assumed for the auxiliary heating technology? Natural gas?
        - What costs are we minimizing?
            - Field pumps
            - Field Tracking
            - TES pumps
            - Electric heat charging -> TES
            - Fuel costs for back-up
        - Parallel Heater -> directly meeting load instead of charging TES
        - Value of heat not the cost of heat?
    */
    lprec *lp;
    int ret = 0;


    try{

        //Calculate the number of variables
        int nt = (int)m_nstep_opt;

        unordered_map<std::string, double> P;
        calculate_parameters(P);

        //set up the variable structure
        optimization_vars O;
        O.add_var("xr", optimization_vars::VAR_TYPE::REAL_T, optimization_vars::VAR_DIM::DIM_T, nt, 0.);
        O.add_var("xrsu", optimization_vars::VAR_TYPE::REAL_T, optimization_vars::VAR_DIM::DIM_T, nt, 0., P["Qru"]);
        O.add_var("ursu", optimization_vars::VAR_TYPE::REAL_T, optimization_vars::VAR_DIM::DIM_T, nt, 0., P["Er"] * 1.0001);
        O.add_var("yr", optimization_vars::VAR_TYPE::BINARY_T, optimization_vars::VAR_DIM::DIM_T, nt);
        O.add_var("yrsu", optimization_vars::VAR_TYPE::BINARY_T, optimization_vars::VAR_DIM::DIM_T, nt);

        O.add_var("x", optimization_vars::VAR_TYPE::REAL_T, optimization_vars::VAR_DIM::DIM_T, nt, 0., P["Qu"]);
        O.add_var("s", optimization_vars::VAR_TYPE::REAL_T, optimization_vars::VAR_DIM::DIM_T, nt, 0., P["Eu"]);

        if (params.is_parallel_heater) {
            O.add_var("qeh", optimization_vars::VAR_TYPE::REAL_T, optimization_vars::VAR_DIM::DIM_T, nt, 0., P["Qehu"]);
            O.add_var("yeh", optimization_vars::VAR_TYPE::BINARY_T, optimization_vars::VAR_DIM::DIM_T, nt);
            O.add_var("yreh", optimization_vars::VAR_TYPE::BINARY_T, optimization_vars::VAR_DIM::DIM_T, nt);
        }
        
        // Construct LP model and set up variable properties
        lp = construct_lp_model(&O);

        /* 
        --------------------------------------------------------------------------------
        set up the objective function first (per lpsolve guidance)
        --------------------------------------------------------------------------------
        */
		{
            REAL* row = new REAL[8 * nt + 1];
            int *col = new int[8 * nt + 1];
            double tadj = P["disp_time_weighting"];
            int i = 0;

            //calculate the mean price to appropriately weight the receiver production timing derate
            double pmean =0;
            for(int t=0; t<(int)params.elec_price.size(); t++)
                pmean += params.elec_price.at(t);
            pmean /= (double)params.elec_price.size();
            
            for(int t=0; t<nt; t++)
            {
                i = 0;
                // Receiver/Field pumping power
                row[t + nt * (i)] = P["delta"] * params.elec_price.at(t) * (1 / tadj) * P["Lr"]; // tadj added to prefer receiver production sooner (i.e. delay dumping)
                col[t + nt * (i++)] = O.column("xr", t);

                row[t + nt * (i)] = P["delta"] * params.elec_price.at(t) * (1 / tadj) * P["Lr"];    
                col[t + nt * (i++)] = O.column("xrsu", t);

                // Receiver/Field heat trace and stow
                row[t + nt * (i)] = params.elec_price.at(t) * (1 / tadj) * (P["Wrsb"] + P["Ehs"]);
                col[t + nt * (i++)] = O.column("yrsu", t);

                // Field Tracking
                row[t + nt * (i)] = P["delta"] * params.elec_price.at(t) * (1 / tadj) * P["Wh"];
                col[t + nt * (i++)] = O.column("yr", t);

                // TES pumping power
                row[t + nt * (i)] = P["delta"] * params.elec_price.at(t) * (1 / tadj) * P["Lc"];
                col[t + nt * (i++)] = O.column("x", t);

                // Back-up fuel cost (You actually don't need this...)
                // TODO: We could add another variable for heat from fuel which would balance heat load...
                row[t + nt * (i)] = - P["delta"] * params.heat_cost.at(t) * tadj;
                col[t + nt * (i++)] = O.column("x", t);

                // Electric heater charging
                if (params.is_parallel_heater) {
                    row[t + nt * (i)] = P["delta"] * params.elec_price.at(t) * (1 / tadj) * (1 / P["eta_eh"]);
                    col[t + nt * (i++)] = O.column("qeh", t);
                }

                tadj *= P["disp_time_weighting"];
            }

            // new terminal inventory incentive
            //row[i * nt] = P["delta"] * tadj * pmean * params.inventory_incentive;
            //col[i * nt] = O.column("s", nt - 1);

            set_obj_fnex(lp, i*nt, row, col);

            delete[] col;
            delete[] row;
        }

        /* 
        --------------------------------------------------------------------------------
        set up the constraints
        --------------------------------------------------------------------------------
        */

        // ******************** Receiver constraints *******************
        {
            REAL row[5];
            int col[5];

            for(int t=0; t<nt; t++)
            {
                int i = 0;  // row and column index

                // Receiver startup energy inventory
                // ursu[t] <= ursu[t-1] + Delta * xrsu[t]
                {
                    i = 0;
                    row[i] = 1.;
                    col[i++] = O.column("ursu", t);

                    row[i] = -P["delta"];
                    col[i++] = O.column("xrsu", t);

                    if (t > 0)
                    {
                        row[i] = -1.;
                        col[i++] = O.column("ursu", t - 1);
                    }

                    add_constraintex(lp, i, row, col, LE, 0);
                }

                // Receiver inventory bound when starting
                // ursu[t] <= Er * yrsu[t]
                {
                    i = 0;

                    row[i] = 1.;
                    col[i++] = O.column("ursu", t);

                    row[i] = -P["Er"];
                    col[i++] = O.column("yrsu", t);

                    add_constraintex(lp, i, row, col, LE, 0.);
                    // NOTES: Turning off this constraint helps align the trough model when multiple starts occur in a day.
                    // However, it does result in more starts and stops within the solution. This could be fixed by reintroducting the field startup cost
                }

                // Receiver operation allowed when start-up is complete or if receiver was operating
                // NOTE: tighter formulation when Er is distributed
                // yr[t] <= ursu[t] / Er + yr[t-1]
                {
                    i = 0;
                    row[i] = P["Er"];
                    col[i++] = O.column("yr", t);

                    row[i] = -1.0;
                    col[i++] = O.column("ursu", t);

                    double rhs = 0.;
                    if (t > 0)
                    {
                        row[i] = -P["Er"];
                        col[i++] = O.column("yr", t - 1);
                    }
                    else
                    {
                        rhs = (params.is_rec_operating0 ? P["Er"] : 0.);
                    }

                    add_constraintex(lp, i, row, col, LE, rhs);
                }

                // Receiver startup can't be enabled after a time step where the Receiver was operating
                // yrsu[t] + yr[t-1] <= 1
                {
                    if (t > 0) {
                        i = 0;
                        row[i] = 1.;
                        col[i++] = O.column("yrsu", t);

                        row[i] = 1.;
                        col[i++] = O.column("yr", t - 1);

                        add_constraintex(lp, i, row, col, LE, 1.);
                    }
                }

                // Receiver startup energy limit
                // xrsu[t] <= Qru * yrsu[t]
                {
                    i = 0;
                    row[i] = 1.;
                    col[i++] = O.column("xrsu", t);

                    row[i] = -P["Qru"];
                    col[i++] = O.column("yrsu", t);

                    add_constraintex(lp, i, row, col, LE, 0.);
                }

                // Receiver startup and operation consumption limit
                // xr[t] + xrsu[t] <= Qin[t]
                {
                    i = 0;
                    row[i] = 1.;
                    col[i++] = O.column("xr", t);

                    row[i] = 1.;
                    col[i++] = O.column("xrsu", t);

                    add_constraintex(lp, i, row, col, LE, params.q_sfavail_expected.at(t));
                }

                // Receiver maximum operation limit
                // xr[t] <= Qin[t] * yr[t]
                {
                    i = 0;
                    row[i] = 1.;
                    col[i++] = O.column("xr", t);

                    row[i] = -params.q_sfavail_expected.at(t);
                    col[i++] = O.column("yr", t);

                    add_constraintex(lp, i, row, col, LE, 0.);
                }

                // Receiver minimum operation limit
                // xr[t] >= Qrl * yr[t]
                {
                    i = 0;
                    row[i] = 1.;
                    col[i++] = O.column("xr", t);

                    row[i] = -P["Qrl"];
                    col[i++] = O.column("yr", t);

                    add_constraintex(lp, i, row, col, GE, 0.);
                }

                // Receiver startup only during solar positive periods
                // TODO: This relies on pre-solve to remove variables (might want to create a special set of solar hours)
                // yrsu[t] <= 0 when Qin[t] = 0
                {
                    i = 0;
                    row[i] = 1.;
                    col[i++] = O.column("yrsu", t);

                    add_constraintex(lp, i, row, col, LE, (std::min)(P["Qru"] * params.q_sfavail_expected.at(t), 1.0));
                }

                // Receiver can't continue operating when no energy is available
                // TODO: Can we combine these two constraints?
                // yr[t] <= Qin[t] / Qrl
                {
                    i = 0;
                    row[i] = 1.;
                    col[i++] = O.column("yr", t);

                    add_constraintex(lp, i, row, col, LE, (std::min)(floor(params.q_sfavail_expected.at(t) / P["Qrl"]), 1.0)); //tighter formulation
                }
            }
        }

        // ******************** Electric Heater constraints ****************
        if (params.is_parallel_heater)
        {
            REAL row[5];
            int col[5];

            for (int t = 0; t < nt; t++)
            {
                int i = 0;  // row and column index

                // Heater power limit
                // qeh[t] <= Qehu * yeh[t]
                {
                    i = 0;
                    row[i] = 1.;
                    col[i++] = O.column("qeh", t);

                    row[i] = -P["Qehu"];
                    col[i++] = O.column("yeh", t);

                    add_constraintex(lp, i, row, col, LE, 0.);
                }

                // Heater minimum operation requirement
                // qeh[t] >= Qehl * yeh[t]
                {
                    i = 0;
                    row[i] = 1.;
                    col[i++] = O.column("qeh", t);

                    row[i] = -P["Qehl"];
                    col[i++] = O.column("yeh", t);

                    add_constraintex(lp, i, row, col, GE, 0.);
                }

                // Heaters must be off before field defocus
                // xr[t] + xrsu[t] >= Qin[t] * yreh[t]
                {
                    i = 0;
                    row[i] = 1.;
                    col[i++] = O.column("xr", t);

                    row[i] = 1.;
                    col[i++] = O.column("xrsu", t);

                    row[i] = -params.q_sfavail_expected.at(t);
                    col[i++] = O.column("yreh", t);

                    add_constraintex(lp, i, row, col, GE, 0.);
                }

                //******* linearization of yreh[t] = yr[t] * yeh[t] ******

                // Upper bound with yr
                // yreh[t] <= yr[t]
                {
                    i = 0;
                    row[i] = 1.;
                    col[i++] = O.column("yreh", t);

                    row[i] = -1.;
                    col[i++] = O.column("yr", t);

                    add_constraintex(lp, i, row, col, LE, 0.);
                }

                // Upper bound with yeh
                // yreh[t] <= yeh[t]
                {
                    i = 0;
                    row[i] = 1.;
                    col[i++] = O.column("yreh", t);

                    row[i] = -1.;
                    col[i++] = O.column("yeh", t);

                    add_constraintex(lp, i, row, col, LE, 0.);
                }

                // Lower bound
                // yreh[t] >= yr[t] + yeh[t] - 1
                {
                    i = 0;
                    row[i] = 1.;
                    col[i++] = O.column("yreh", t);

                    row[i] = -1.;
                    col[i++] = O.column("yr", t);

                    row[i] = -1.;
                    col[i++] = O.column("yeh", t);

                    add_constraintex(lp, i, row, col, GE, -1);
                }
            }
        }

        // ******************** Power cycle constraints *******************
        {
            REAL row[7];
            int col[7];

            for (int t = 0; t < nt; t++)
            {
                int i = 0;  // row and column index

                // Heat sink maximum operation limit
                // x[t] <= Qu
                {
                    i = 0;
                    row[i] = 1.;
                    col[i++] = O.column("x", t);

                    add_constraintex(lp, i, row, col, LE, P["Qu"]);
                }

                // Heat sink load maximum operation limit
                // x[t] <= Q_hl
                {
                    i = 0;
                    row[i] = 1.;
                    col[i++] = O.column("x", t);

                    add_constraintex(lp, i, row, col, LE, params.heat_load.at(t));
                }

                // Heat sink minimum operation limit
                // x[t] >= Ql
                {
                    i = 0;
                    row[i] = 1.;
                    col[i++] = O.column("x", t);

                    add_constraintex(lp, i, row, col, GE, P["Ql"]);
                }
            }
        }

        // ******************** TES Balance constraints *******************       
        {
            REAL row[10];
            int col[10];

            for(int t=0; t<nt; t++)
            {
                int i = 0; // row and column index

                // Energy in, out, and stored in the TES system must balance.
                // delta * ( xr[t] + qeh[t] - x[t] ) = s[t] - s[t-1]
                {
                    double rhs = 0.;
                    i = 0;
                    row[i] = P["delta"];
                    col[i++] = O.column("xr", t);

                    if (params.is_parallel_heater) {
                        row[i] = P["delta"];
                        col[i++] = O.column("qeh", t);
                    }

                    row[i] = -P["delta"];
                    col[i++] = O.column("x", t);

                    row[i] = -1.;
                    col[i++] = O.column("s", t);

                    if (t > 0)
                    {
                        row[i] = 1.;
                        col[i++] = O.column("s", t - 1);
                    }
                    else
                    {
                        rhs += - P["s0"];  //initial storage state (kWh)
                    }

                    add_constraintex(lp, i, row, col, EQ, rhs);
                }

                // Max cycle thermal input is required in time periods where cycle operates and receiver is starting up
                // x[t+1] + Qb * ycsb[t+1] <= s[t] / delta_rs[t+1] - M * ( -3 + yrsu[t+1] + y[t] + y[t+1] )
                //{
                //    if (t < nt - 1)
                //    {
                //        double t_rec_startup = params.delta_rs.at(t) * P["delta"];

                //        i = 0;
                //        row[i] = 1.;
                //        col[i++] = O.column("x", t + 1);

                //        row[i] = -1. / t_rec_startup;
                //        col[i++] = O.column("s", t);

                //        row[i] = P["Qu"];                       //tighter formulation
                //        col[i++] = O.column("yrsu", t + 1);

                //        row[i] = 1.0;
                //        col[i++] = O.column("x", t);

                //        row[i] = 1.0;
                //        col[i++] = O.column("x", t + 1);

                //        add_constraintex(lp, i, row, col, LE, 3.0 * P["Qu"]);
                //    }
                //}

            }
        }

        //Set problem to minimize (operating cost)
        set_minim(lp);

        setup_solver_presolve_bbrules(lp);
        bool return_ok = problem_scaling_solve_loop(lp);
        set_lp_solve_outputs(lp);

        // Saving problem and solution for DEBUGGING formulation
        //save_problem_solution_debug(lp);
        //if (solver_params.disp_reporting > 4)
        //    print_log_to_file();

        if(return_ok)
            set_outputs_from_lp_solution(lp, P);

        delete_lp(lp);
        lp = NULL;
        print_dispatch_update();

        return return_ok;
    }
    catch(std::exception &e)
    {
        //clean up memory and pass on the exception
        if( lp != NULL )
            delete_lp(lp);
        
        throw e;
    }
    catch(...)
    {
        //clean up memory
        if( lp != NULL )
            delete_lp(lp);

        return false;
    }

    return false;
}

void cst_iph_dispatch_opt::set_outputs_from_lp_solution(lprec* lp, unordered_map<std::string, double>& model_params)
{
    int nt = (int)m_nstep_opt;

    outputs.clear();
    outputs.resize(nt);

    int ncols = get_Norig_columns(lp);
    int nrows = get_Norig_rows(lp);

    for (int c = 1; c <= ncols; c++)
    {
        char* colname = get_origcol_name(lp, c);
        if (!colname) continue;

        char root[15];
        char ind[4];
        if (parse_column_name(colname, root, ind)) continue;  //a 2D variable

        int t = atoi(ind);
        double val = get_var_primalresult(lp, nrows + c);

        if (strcmp(root, "x") == 0)  // cycle thermal energy consumption
        {
            outputs.q_pb_target.at(t) = val;
            outputs.pb_operation.at(t) = val > 0.0 ? 1.0 : 0.0;
        }
        else if (strcmp(root, "yrsu") == 0)  // is receiver starting
        {
            outputs.rec_operation.at(t) = outputs.rec_operation.at(t) || (std::abs(1 - val) < 0.001);
        }
        else if (strcmp(root, "xrsu") == 0)  // receiver startup energy
        {
            outputs.q_rec_startup.at(t) = val;
        }
        else if (strcmp(root, "yr") == 0)  // is receiver operating
        {
            outputs.rec_operation.at(t) = outputs.rec_operation.at(t) || (std::abs(1 - val) < 0.001);
        }
        else if (strcmp(root, "s") == 0)  // thermal storage charge state
        {
            outputs.tes_charge_expected.at(t) = val;
        }
        else if (strcmp(root, "xr") == 0)  // receiver production
        {
            outputs.q_sf_expected.at(t) = val;
        }
        else if (strcmp(root, "yeh") == 0)  // is parallel heater on
        {
            outputs.htr_operation.at(t) = outputs.htr_operation.at(t) || (std::abs(1 - val) < 0.001);
        }
        else if (strcmp(root, "qeh") == 0)  // heater target power
        {
            outputs.q_eh_target.at(t) = val;
        }
    }
}

bool cst_iph_dispatch_opt::set_dispatch_outputs()
{
    if (lp_outputs.last_opt_successful && m_current_read_step < (int)outputs.q_pb_target.size())
    {
        //calculate the current read step, account for number of dispatch steps per hour and the simulation time step
        m_current_read_step = (int)(pointers.siminfo->ms_ts.m_time * solver_params.steps_per_hour / 3600. - .001)
            % (solver_params.optimize_frequency * solver_params.steps_per_hour);

        disp_outputs.is_rec_su_allowed = outputs.rec_operation.at(m_current_read_step);
        disp_outputs.is_pc_sb_allowed = outputs.pb_standby.at(m_current_read_step);
        disp_outputs.is_pc_su_allowed = outputs.pb_operation.at(m_current_read_step) || disp_outputs.is_pc_sb_allowed;

        disp_outputs.q_pc_target = outputs.q_pb_target.at(m_current_read_step);

        // Artificially set target higher to deal with end of TES effects
        if (m_current_read_step > 1) {
            if ((outputs.tes_charge_expected.at(m_current_read_step - 1) > 0.)
                && (outputs.tes_charge_expected.at(m_current_read_step) == 0.)) {       // Did we run out of TES this time step?
                disp_outputs.q_pc_target = params.heat_load.at(m_current_read_step);        // Set to heat load this time step
            }
            else if ((outputs.tes_charge_expected.at(m_current_read_step - 1) == 0.0)   // Did we run out of TES last time step?
                && (outputs.q_pb_target.at(m_current_read_step - 1) > 0.0)) {               // and we generating last time step?
                disp_outputs.q_pc_target = params.heat_load.at(m_current_read_step);        // Set to heat load this time step
                disp_outputs.is_pc_su_allowed = true;
            }
        }

        if (disp_outputs.q_pc_target + 1.e-5 < params.q_hs_min) {
            disp_outputs.is_pc_su_allowed = false;
            disp_outputs.q_pc_target = 0.0;
        }
        disp_outputs.q_dot_pc_max = disp_outputs.q_pc_target;

        disp_outputs.q_dot_elec_to_CR_heat = outputs.q_sf_expected.at(m_current_read_step);     // TODO: I don't really understand what this one does to the solver...

        disp_outputs.q_eh_target = outputs.q_eh_target.at(m_current_read_step);
        disp_outputs.is_eh_su_allowed = outputs.htr_operation.at(m_current_read_step);

        disp_outputs.etasf_expect = params.eta_sf_expected.at(m_current_read_step);
        disp_outputs.qsf_expect = params.q_sfavail_expected.at(m_current_read_step);
        disp_outputs.qsfprod_expect = outputs.q_sf_expected.at(m_current_read_step);
        disp_outputs.qsfsu_expect = outputs.q_rec_startup.at(m_current_read_step);
        disp_outputs.tes_expect = outputs.tes_charge_expected.at(m_current_read_step);

        if (m_current_read_step > solver_params.optimize_frequency* solver_params.steps_per_hour)
            throw C_csp_exception("Counter synchronization error in dispatch optimization routine.", "csp_dispatch");
    }
    disp_outputs.time_last = pointers.siminfo->ms_ts.m_time;

    return true;
}
