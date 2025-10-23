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
#include "etes_dispatch.h"
#include "lp_lib.h" 
#include "lib_util.h"

#undef min
#undef max

/*

Careful with namespaces in this file.. importing the LPsolve library introduces new macro definitions
and function definitions.

*/

etes_dispatch_opt::etes_dispatch_opt()
{
    outputs.clear();
    params.clear();
}

void etes_dispatch_opt::init(double cycle_q_dot_des, double cycle_eta_des)
{
    set_default_solver_parameters();

    params.clear();

    params.dt = 1. / (double)solver_params.steps_per_hour;  //hr

    params.dt_pb_startup_cold = pointers.mpc_pc->get_cold_startup_time();
    params.e_pb_startup_cold = pointers.mpc_pc->get_cold_startup_energy();
    params.q_pb_max = pointers.mpc_pc->get_max_thermal_power();
    params.q_pb_min = pointers.mpc_pc->get_min_thermal_power();

    params.dt_rec_startup = pointers.col_rec->get_startup_time(); // / 3600.;
    params.e_rec_startup = pointers.col_rec->get_startup_energy();
    params.q_eh_min = pointers.col_rec->get_min_power_delivery() * (1 + 1e-8); // ensures controller doesn't shut down heater at minimum load
    params.q_eh_max = pointers.col_rec->get_max_power_delivery(std::numeric_limits<double>::quiet_NaN());
    params.eta_eh = pointers.col_rec->get_design_electric_to_heat_cop();

    params.e_tes0 = pointers.tes->get_initial_charge_energy();
    params.e_tes_min = pointers.tes->get_min_charge_energy();
    params.e_tes_max = pointers.tes->get_max_charge_energy();
    //params.tes_degrade_rate = pointers.tes->get_degradation_rate();

    params.q_pb_des = cycle_q_dot_des; // MW
    params.eta_pb_des = cycle_eta_des; 
    double w_pb_des = cycle_q_dot_des * params.eta_pb_des;  // keep in MW

    params.eff_table_load.init_linear_cycle_efficiency_table(params.q_pb_min, params.q_pb_des, params.eta_pb_des, pointers.mpc_pc);
    params.eff_table_Tdb.init_efficiency_ambient_temp_table(params.eta_pb_des, w_pb_des, pointers.mpc_pc, &params.wcondcoef_table_Tdb);
}

void etes_dispatch_opt::set_default_solver_parameters()
{
    // If user did not set solver parameters, set defaults specific to ETES dispatch model
    if (solver_params.presolve_type < 0)
        solver_params.presolve_type = PRESOLVE_ROWS + PRESOLVE_COLS + PRESOLVE_ELIMEQ2 + PRESOLVE_PROBEFIX;
        //PRESOLVE_SOS + PRESOLVE_LINDEP //genetic algorithm
    if (solver_params.bb_type < 0)
        solver_params.bb_type = NODE_PSEUDOCOSTSELECT + NODE_AUTOORDER; // This works better for ETES dispatch
        //NODE_RCOSTFIXING + NODE_AUTOORDER + NODE_BREADTHFIRSTMODE + NODE_RESTARTMODE + NODE_DYNAMICMODE + NODE_RANDOMIZEMODE + NODE_DEPTHFIRSTMODE + NODE_PSEUDORATIOSELECT //genetic algorithm
    if (solver_params.scaling_type < 0)
        solver_params.scaling_type = SCALE_MEAN + SCALE_LOGARITHMIC + SCALE_POWER2 + SCALE_EQUILIBRATE + SCALE_INTEGERS;
        // SCALE_LOGARITHMIC + SCALE_QUADRATIC + SCALE_EXTREME //genetic algorithm
}

bool etes_dispatch_opt::check_setup(int nstep)
{
    //check parameters and inputs to make sure everything has been set up correctly
    if ((int)params.sell_price.size() < nstep)   return false;
    // TODO: add other checks

    return base_dispatch_opt::check_setup();
}

bool etes_dispatch_opt::update_horizon_parameters(C_csp_tou& mc_tou)
{
    //get the new price signal
    int num_steps = solver_params.optimize_horizon * solver_params.steps_per_hour;
    params.sell_price.clear();
    params.sell_price.resize(num_steps, 1.);
    params.buy_price.clear();
    params.buy_price.resize(num_steps, 1.);

    double sec_per_step = 3600. / (double)solver_params.steps_per_hour;
    for (int t = 0; t < num_steps; t++) {
        C_csp_tou::S_csp_tou_outputs tou_outputs;
        mc_tou.call(pointers.siminfo->ms_ts.m_time + t * sec_per_step, tou_outputs);
        params.sell_price.at(t) = tou_outputs.m_elec_price * 1000.0; // $/kWhe -> $/MWhe
        params.buy_price.at(t) = params.sell_price.at(t);     //TODO: make these unique if specified by user
    }
    return true;
}

void etes_dispatch_opt::update_initial_conditions(double q_dot_to_pb, double T_htf_cold_des, double pc_state_persist)
{
    //TODO: update start-up energy already consumed
    params.e_pb_start0 = 0;     //these are set assuming no carry over
    params.e_eh_start0 = 0;

    //note the states of the power cycle and receiver
    params.is_pb_operating0 = pointers.mpc_pc->get_operating_state() == 1;
    params.is_eh_operating0 = pointers.col_rec->get_operating_state() == C_csp_collector_receiver::ON;

    params.q_pb0 = q_dot_to_pb;

    if (params.is_pb_operating0 == 1) {
        params.up_time0 = pc_state_persist;  // cycle is up
        params.down_time0 = 0;
    }
    else {
        params.up_time0 = 0;
        params.down_time0 = pc_state_persist; // cycle is down
    }
        
    //Note the state of the thermal energy storage system
    double q_disch, m_dot_disch, T_tes_return;
    pointers.tes->discharge_avail_est(T_htf_cold_des, pointers.siminfo->ms_ts.m_step, q_disch, m_dot_disch, T_tes_return);
    params.e_tes0 = q_disch * pointers.siminfo->ms_ts.m_step / 3600. + params.e_tes_min;        //kWh
    if (params.e_tes0 < params.e_tes_min)
        params.e_tes0 = params.e_tes_min;
    if (params.e_tes0 > params.e_tes_max)
        params.e_tes0 = params.e_tes_max;
}

bool etes_dispatch_opt::predict_performance(int step_start, int ntimeints, int divs_per_int)
{
    //Step number - 1-based index for first hour of the year.

    //save step count
    m_nstep_opt = ntimeints;

    //Predict performance out nstep values.
    params.eta_pb_expected.clear();
    params.w_condf_expected.clear();

    if (!check_setup(m_nstep_opt))
        throw C_csp_exception("Dispatch optimization precheck failed.");

    //create the sim info
    C_csp_solver_sim_info simloc;
    simloc.ms_ts.m_step = pointers.siminfo->ms_ts.m_step;

    double ave_weight = 1. / (double)divs_per_int;

    for (int i = 0; i < m_nstep_opt; i++)
    {
        //initialize hourly average values
        double cycle_eff_ave = 0.;
        double wcond_ave = 0.;

        for (int j = 0; j < divs_per_int; j++)     //take averages over hour if needed
        {
            //jump to the current step
            if (!pointers.m_weather.read_time_step(step_start + i * divs_per_int + j, simloc))
                return false;

            //store the power cycle efficiency
            double cycle_eff = params.eff_table_Tdb.interpolate(pointers.m_weather.ms_outputs.m_tdry);
            cycle_eff *= params.eta_pb_des;
            cycle_eff_ave += cycle_eff * ave_weight;

            //store the condenser parasitic power fraction
            double wcond_f = params.wcondcoef_table_Tdb.interpolate(pointers.m_weather.ms_outputs.m_tdry);
            wcond_ave += wcond_f * ave_weight;

            simloc.ms_ts.m_time += simloc.ms_ts.m_step;
            pointers.m_weather.converged();
        }

        //-----report hourly averages
        //power cycle efficiency
        params.eta_pb_expected.push_back(cycle_eff_ave);
        //condenser power
        params.w_condf_expected.push_back(wcond_ave);
    }
    return true;
}

static void calculate_parameters(etes_dispatch_opt *optinst, unordered_map<std::string, double> &pars, int nt)
{
    /* 
    A central location for making sure the parameters from the model are accurately calculated for use in
    the dispatch optimization model.
    */

        pars["T"] = nt ;
        pars["delta"] = optinst->params.dt;

        optinst->params.time_elapsed.clear();
        for (int t = 0; t < nt; t++)
        {
            optinst->params.time_elapsed.push_back(pars["delta"] * (t + 1));
        }

        pars["Ec"] = optinst->params.e_pb_startup_cold;
        pars["Eeh"] = optinst->params.e_rec_startup;
        pars["Eu"] = optinst->params.e_tes_max;

        pars["Qu"] = optinst->params.q_pb_des;
        pars["Ql"] = optinst->params.q_pb_min ;
        pars["Qcsu"] = optinst->params.e_pb_startup_cold / ceil(optinst->params.dt_pb_startup_cold / pars["delta"]) / pars["delta"];

        pars["eta_eh"] = optinst->params.eta_eh;
        pars["Qehu"] = optinst->params.q_eh_max;
        pars["Qehl"] = optinst->params.q_eh_min;
        pars["Qhsu"] = optinst->params.e_rec_startup / ceil(optinst->params.dt_rec_startup / pars["delta"]) / pars["delta"];

        double delta_hsu = optinst->params.dt_rec_startup;
        while (delta_hsu > pars["delta"])
        {
            delta_hsu -= pars["delta"];
        }
        pars["delta_hsu"] = delta_hsu;

        double delta_csu = optinst->params.dt_pb_startup_cold;
        while (delta_csu > pars["delta"])
        {
            delta_csu -= pars["delta"];
        }
        pars["delta_csu"] = delta_csu;

        // dispatch user inputs
        pars["disp_time_weighting"] = optinst->params.time_weighting;
        pars["csu_cost"] = optinst->params.csu_cost;
        pars["hsu_cost"] = optinst->params.hsu_cost;
        pars["pen_delta_w"] = optinst->params.pen_delta_w;
        pars["Yd"] = optinst->params.down_time_min;
        pars["Yu"] = optinst->params.up_time_min;

        /*
        ----------------------
        Initial conditions
        ----------------------
        */
        pars["y0"] = (optinst->params.is_pb_operating0 ? 1 : 0);
        pars["q0"] = optinst->params.q_pb0;

        pars["yeh0"] = (optinst->params.is_eh_operating0 ? 1 : 0);
        pars["ycsu0"] = (optinst->params.is_pb_starting0 ? 1 : 0);
        pars["ucsu0"] = optinst->params.e_pb_start0;
        pars["yhsu0"] = (optinst->params.is_eh_starting0 ? 1 : 0);
        pars["uhsu0"] = optinst->params.e_eh_start0;

        pars["Yd0"] = optinst->params.down_time0;
        pars["Yu0"] = optinst->params.up_time0;
        pars["s0"] = optinst->params.e_tes0 ;

        // power cycle linear performance curve
        double intercept;
        optinst->params.eff_table_load.get_slope_intercept_cycle_linear_performance(&pars["etap"], &intercept);

        // maximum power based on linear fit
        pars["Wdotu"] = (pars["etap"] * pars["Qu"] + intercept);
        // minimum power based on linear fit
        pars["Wdotl"] = (pars["etap"] * pars["Ql"] + intercept);

        pars["Wdot0"] = 0.;
        if (pars["q0"] >= pars["Ql"])
            pars["Wdot0"] = (pars["etap"] * pars["q0"] + intercept) * optinst->params.eta_pb_expected.at(0) / optinst->params.eta_pb_des;
};

bool etes_dispatch_opt::optimize()
{

    //First check to see whether we should call the AMPL engine instead. 
    if( solver_params.is_ampl_engine )
    {
        return optimize_ampl();
    }

    /* 
    Formulate the optimization problem for dispatch generation. We are trying to maximize revenue subject to inventory
    constraints.
    
    Variables
    -------------------------------------------------------------
    Continuous
    -------------------------------------------------------------
    s           MWht    TES reserve quantity at time t
    wdot        MWe     Electrical power production at time t
    delta_w     MWe     Change in power production at time t w/r/t t-1
    qdot        MWt	    Cycle thermal power consumption at time t
    qeh         MWt     Thermal power delivered by the electric heaters at time t
    ucsu        MWt     Cycle accumulated start-up thermal power at time t
    uhsu        MWt     Heaters accumulated start-up thermal power at time t
    -------------------------------------------------------------
    Binary
    -------------------------------------------------------------
    y               1 if cycle is generating electric power at time t; 0 otherwise
    ycgb            1 if cycle begins electric power generation at time t; 0 otherwise
    ycge            1 if cycle stops electric power generation at time t; 0 otherwise
    ycsu            1 if cycle is starting up at time t; 0 otherwise
    ycsup           1 if cycle startup penalty is enforced at time t; 0 otherwise
    yeh             1 if electrical heaters are operating at time t; 0 otherwise 
    yhsu            1 if electrical heaters are starting up at time t; 0 otherwise
    -------------------------------------------------------------
    */
    lprec *lp;

    try{
        // number of timesteps in dispatch horzion
        int nt = (int)m_nstep_opt;

        unordered_map<std::string, double> P;
        calculate_parameters(this, P, nt);

        //set up the variable structure
        optimization_vars O;
        O.add_var("s", optimization_vars::VAR_TYPE::REAL_T, optimization_vars::VAR_DIM::DIM_T, nt, 0., P["Eu"]);
        O.add_var("wdot", optimization_vars::VAR_TYPE::REAL_T, optimization_vars::VAR_DIM::DIM_T, nt, 0., P["Wdotu"]*1.1);
        O.add_var("delta_w", optimization_vars::VAR_TYPE::REAL_T, optimization_vars::VAR_DIM::DIM_T, nt, 0., P["Wdotu"] * 1.1);
        O.add_var("qdot", optimization_vars::VAR_TYPE::REAL_T, optimization_vars::VAR_DIM::DIM_T, nt, 0., P["Qu"]);
        O.add_var("qeh", optimization_vars::VAR_TYPE::REAL_T, optimization_vars::VAR_DIM::DIM_T, nt, 0., P["Qehu"]);
        O.add_var("ucsu", optimization_vars::VAR_TYPE::REAL_T, optimization_vars::VAR_DIM::DIM_T, nt, 0., P["Ec"] * 1.0001);
        O.add_var("uhsu", optimization_vars::VAR_TYPE::REAL_T, optimization_vars::VAR_DIM::DIM_T, nt, 0., P["Eeh"] * 1.0001);
        O.add_var("zhsu", optimization_vars::VAR_TYPE::REAL_T, optimization_vars::VAR_DIM::DIM_T, nt, 0., P["Qehu"]);

#ifdef ALT_ETES_FORM
        O.add_var("zcsu", optimization_vars::VAR_TYPE::REAL_T, optimization_vars::VAR_DIM::DIM_T, nt, 0., P["Qu"]);
        O.add_var("zwcsu", optimization_vars::VAR_TYPE::REAL_T, optimization_vars::VAR_DIM::DIM_T, nt, 0., P["Wdotu"] * 1.1);
#endif

        O.add_var("y", optimization_vars::VAR_TYPE::BINARY_T, optimization_vars::VAR_DIM::DIM_T, nt);
        O.add_var("ycgb", optimization_vars::VAR_TYPE::BINARY_T, optimization_vars::VAR_DIM::DIM_T, nt);
        O.add_var("ycge", optimization_vars::VAR_TYPE::BINARY_T, optimization_vars::VAR_DIM::DIM_T, nt);
        O.add_var("ycsu", optimization_vars::VAR_TYPE::BINARY_T, optimization_vars::VAR_DIM::DIM_T, nt);
        O.add_var("ycsup", optimization_vars::VAR_TYPE::BINARY_T, optimization_vars::VAR_DIM::DIM_T, nt);
        O.add_var("yeh", optimization_vars::VAR_TYPE::BINARY_T, optimization_vars::VAR_DIM::DIM_T, nt);
        O.add_var("yhsu", optimization_vars::VAR_TYPE::BINARY_T, optimization_vars::VAR_DIM::DIM_T, nt);

        // Construct LP model and set up variable properties
        lp = construct_lp_model(&O);

        /* 
        --------------------------------------------------------------------------------
        set up the objective function first (per lpsolve guidance)
        --------------------------------------------------------------------------------
        */
		{
            int *col = new int[10 * nt];
            REAL *row = new REAL[10 * nt];
            double tadj = P["disp_time_weighting"];
            int i = 0;

            //calculate the mean price to appropriately weight the receiver production timing derate
            double pmean =0;
            for(int t=0; t<(int)params.sell_price.size(); t++)
                pmean += params.sell_price.at(t);
            pmean /= (double)params.sell_price.size();
            //--
            
            for(int t=0; t<nt; t++)
            {
                i = 0;
                row[t + nt * (i)] = P["delta"] * tadj * params.sell_price.at(t) * (1. - params.w_condf_expected.at(t));
                col[t + nt * (i++)] = O.column("wdot", t);

#ifdef ALT_ETES_FORM
                row[t + nt * (i)] = -P["delta_csu"] * tadj * params.sell_price.at(t) * (1. - params.w_condf_expected.at(t));
                col[t + nt * (i++)] = O.column("zwcsu", t);
#endif

                row[t + nt * (i)] = -(P["delta"] * (1 / tadj) * params.buy_price.at(t) * (1 / P["eta_eh"]));
                col[t + nt * (i++)] = O.column("qeh", t);

                row[t + nt * (i)] = -(P["delta"] * (1 / tadj) * params.buy_price.at(t) * (1 / P["eta_eh"]) * P["Qhsu"]);
                col[t + nt * (i++)] = O.column("yhsu", t);

                row[t + nt * (i)] = -(1 / tadj) * P["csu_cost"];
                col[t + nt * (i++)] = O.column("ycsup", t);

                row[t + nt * (i)] = -(1 / tadj) * P["pen_delta_w"];
                col[t + nt * (i++)] = O.column("delta_w", t);

                row[t + nt * (i)] = -(1 / tadj) * P["hsu_cost"];
                col[t + nt * (i++)] = O.column("yhsu", t);

                tadj *= P["disp_time_weighting"];
            }

            set_obj_fnex(lp, i*nt, row, col);

            delete[] col;
            delete[] row;
        }

        /* 
        --------------------------------------------------------------------------------
        set up the constraints
        --------------------------------------------------------------------------------
        */
        // TODO: Should each of these constraints be moved to individual functions to be accessed by both CSP and ETES dispatch models

        // ******************** Electric heater constraints *******************
        {
            REAL row[5];
            int col[5];

            for (int t = 0; t < nt; t++)
            {
                int i = 0; // row and col index, reset for every constraint

                // Electric heater startup inventory
                // uhsu[t] <= uhsu[t-1] + delta * Qhsu * yhsu[t]
                {
                    double rhs = 0.;
                    i = 0;

                    row[i] = 1.;
                    col[i++] = O.column("uhsu", t);

                    row[i] = -P["delta"] * P["Qhsu"];
                    col[i++] = O.column("yhsu", t);

                    if (t > 0)
                    {
                        row[i] = -1.;
                        col[i++] = O.column("uhsu", t - 1);
                    }
                    else
                    {
                        rhs += P["uhsu0"];
                    }
                    add_constraintex(lp, i, row, col, LE, rhs);
                }

                // Heater inventory nonzero
                // uhsu[t] <= Eeh * yhsu[t]
                {
                    i = 0;
                    row[i] = 1.;
                    col[i++] = O.column("uhsu", t);

                    row[i] = -P["Eeh"] * 1.00001;
                    col[i++] = O.column("yhsu", t);

                    add_constraintex(lp, i, row, col, LE, 0.);
                }

                // Heater operation allowed when:
                // yeh[t] <= uhsu[t] / Eeh + yeh[t-1]
                {
                    double rhs = 0.;
                    i = 0;
                    row[i] = P["Eeh"];
                    col[i++] = O.column("yeh", t);

                    row[i] = -1.0;
                    col[i++] = O.column("uhsu", t);

                    if (t > 0)
                    {
                        row[i] = -P["Eeh"];
                        col[i++] = O.column("yeh", t - 1);
                    }
                    else
                    {
                        rhs += P["Eeh"] * P["yeh0"];
                    }
                    add_constraintex(lp, i, row, col, LE, rhs);
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

                // Heater startup can't be enabled after a time step where the heaters was operating
                // yhsu[t] + yeh[t-1] <= 1
                {
                    double rhs = 1.;
                    i = 0;
                    row[i] = 1.;
                    col[i++] = O.column("yhsu", t);

                    if (t > 0)
                    {
                        row[i] = 1.;
                        col[i++] = O.column("yeh", t - 1);
                    }
                    else
                    {
                        rhs += -P["yeh0"];
                    }
                    add_constraintex(lp, i, row, col, LE, rhs);
                }

                // Heater and cycle cannot coincide
                // yeh[t] + y[t] <= 1
                {
                    i = 0;
                    row[i] = 1.;
                    col[i++] = O.column("yeh", t);

                    row[i] = 1.;
                    col[i++] = O.column("y", t);

                    add_constraintex(lp, i, row, col, LE, 1.);
                }
            }
        }
        
        // ******************** Power cycle constraints *******************
        {
            REAL row[7];
            int col[7];

            for (int t = 0; t < nt; t++)
            {
                int i = 0; // row and col index, reset for every constraint

                // Startup Inventory balance
                // ucsu[t] <= ucsu[t-1] + delta * Qcsu * ycsu[t]
                {
                    double rhs = 0.;
                    row[i] = 1.;
                    col[i++] = O.column("ucsu", t);

                    row[i] = -P["delta"] * P["Qcsu"];
                    col[i++] = O.column("ycsu", t);

                    if (t > 0)
                    {
                        row[i] = -1.;
                        col[i++] = O.column("ucsu", t - 1);
                    }

                    add_constraintex(lp, i, row, col, LE, 0.);
                }

                // Inventory nonzero
                // ucsu[t] <= Ec * ycsu[t]
                {
                    i = 0;
                    row[i] = 1.;
                    col[i++] = O.column("ucsu", t);

                    row[i] = -P["Ec"] * 1.00001; //tighter formulation
                    col[i++] = O.column("ycsu", t);

                    add_constraintex(lp, i, row, col, LE, 0.);
                }

                // Cycle operation allowed when startup is complete or already operating
                // hourly:    y[t] <=  ucsu[t  ] / Ec + y[t-1]
                // subhourly: y[t] <=  ucsu[t-1] / Ec + y[t-1]   (startup and production cannot coincide)
                {
                    i = 0;
                    row[i] = P["Ec"];
                    col[i++] = O.column("y", t);

                    row[i] = -1.0;
                    if (P["delta"] >= 1.)
                    {
                        col[i++] = O.column("ucsu", t);                 // for hourly model (delta = 1)
                    }
                    else
                    {
                        col[i++] = O.column("ucsu", t - 1);             // for sub-hourly model (delta < 1)
                    }

                    double rhs = 0.;

                    if (t > 0)
                    {
                        row[i] = -P["Ec"];
                        col[i++] = O.column("y", t - 1);
                    }
                    else
                    {
                        rhs += P["Ec"] * P["y0"];
                    }

                    add_constraintex(lp, i, row, col, LE, rhs);
                }

                // Limits the thermal power to the cycle during periods of startup
                /* NOTE: This is not accurate in terms of thermal power delivered to the power cycle (which should be Qu after start up is completed).
                    However, in practice this constraint (in addition to providing adding startup power to heat target) provides the dispatch model a disincentive to
                    start-up on high value periods as fraction of production is lost due to startup time.  Therefore, it better to startup the hour before a high value period. */
                // qdot[t] + Qcsu * ycsu[t] <= Qu * y[t]
#ifndef ALT_ETES_FORM
                {
                    i = 0;
                    row[i] = 1.;
                    col[i++] = O.column("qdot", t);

                    row[i] = P["Qcsu"];
                    col[i++] = O.column("ycsu", t);

                    row[i] = -P["Qu"];
                    col[i++] = O.column("y", t);

                    add_constraintex(lp, i, row, col, LE, 0.);
                }
#endif

                // Cycle maximum operation limit
                // qdot[t] <= Qu * y[t]
                {
                    i = 0;
                    row[i] = 1.;
                    col[i++] = O.column("qdot", t);

                    row[i] = -P["Qu"];
                    col[i++] = O.column("y", t);

                    add_constraintex(lp, i, row, col, LE, 0.);
                }

                // Cycle minimum operation limit
                // qdot[t] >= Ql * y[t]
                {
                    i = 0;
                    row[i] = 1.;
                    col[i++] = O.column("qdot", t);

                    row[i] = -P["Ql"];
                    col[i++] = O.column("y", t);

                    add_constraintex(lp, i, row, col, GE, 0);
                }

                // Power production linearization (power as function of heat input)
                // wdot[t] = eta_amb[t]/eta_des * ( etap * qdot[t] + ( Wdotu - etap * Qu ) * y[t] )
                {
                    i = 0;
                    row[i] = 1.;
                    col[i++] = O.column("wdot", t);

                    row[i] = -P["etap"] * params.eta_pb_expected.at(t) / params.eta_pb_des;
                    col[i++] = O.column("qdot", t);

                    row[i] = -(P["Wdotu"] - P["etap"] * P["Qu"]) * params.eta_pb_expected.at(t) / params.eta_pb_des;
                    col[i++] = O.column("y", t);

                    add_constraintex(lp, i, row, col, EQ, 0.);
                    //add_constraintex(lp, i, row, col, LE, 0.);
                }

                // Cycle positive production change (i.e., ramping) We might want to penalize thermal ramping instead to remove the ambient correction
                // delta_w[t] >= wdot[t] - wdot[t-1]
                {
                    double rhs = 0.;
                    i = 0;
                    row[i] = 1.;
                    col[i++] = O.column("delta_w", t);

                    row[i] = -1.;
                    col[i++] = O.column("wdot", t);

                    if (t > 0)
                    {
                        row[i] = 1.;
                        col[i++] = O.column("wdot", t - 1);
                    }
                    else
                    {
                        rhs += -P["Wdot0"];
                    }
                    add_constraintex(lp, i, row, col, GE, rhs);
                }

                // Cycle negative production change (i.e., ramping) We might want to penalize thermal ramping instead to remove the ambient correction
                // delta_w[t] >= wdot[t-1] - wdot[t]
                /*{
                    double rhs = 0.;
                    i = 0;
                    row[i] = 1.;
                    col[i++] = O.column("delta_w", t);

                    row[i] = 1.;
                    col[i++] = O.column("wdot", t);

                    if (t > 0)
                    {
                        row[i] = -1.;
                        col[i++] = O.column("wdot", t - 1);
                    }
                    else
                    {
                        rhs += P["Wdot0"];
                    }
                    add_constraintex(lp, i, row, col, GE, rhs);
                }*/

                // Cycle startup can't be enabled after a time step where the cycle was operating
                // ycsu[t] + y[t-1] <= 1
                {
                    double rhs = 1.;
                    i = 0;
                    row[i] = 1.;
                    col[i++] = O.column("ycsu", t);

                    if (t > 0)
                    {
                        row[i] = 1.;
                        col[i++] = O.column("y", t - 1);
                    }
                    else
                    {
                        rhs += -P["y0"];
                    }
                    add_constraintex(lp, i, row, col, LE, rhs);
                }

                // Cycle start penalty
                // ycsup[t] >= ycsu[t] - ycsu[t-1]
                {
                    double rhs = 0.;
                    i = 0;
                    row[i] = 1.;
                    col[i++] = O.column("ycsup", t);

                    row[i] = -1.;
                    col[i++] = O.column("ycsu", t);

                    if (t > 0)
                    {
                        row[i] = 1.;
                        col[i++] = O.column("ycsu", t - 1);
                    }
                    else
                    {
                        rhs += -P["ycsu0"];
                    }
                    add_constraintex(lp, i, row, col, GE, rhs);
                }
            }
        }

        // Minimum up- and down-times
        {
            REAL row[4];
            int col[4];

            // binary logic when switching power cycle state
            // ycgb[t] - ycge[t] = y[t] - y[t-1]
            for (int t = 0; t < nt; t++)
            {
                double rhs = 0.;
                int i = 0;

                row[i] = 1.;
                col[i++] = O.column("ycgb", t);

                row[i] = -1.;
                col[i++] = O.column("ycge", t);

                row[i] = -1.;
                col[i++] = O.column("y", t);

                if (t > 0)
                {
                    row[i] = 1.;
                    col[i++] = O.column("y", t - 1);
                }
                else
                {
                    rhs += -P["y0"];
                }
                add_constraintex(lp, i, row, col, EQ, rhs);
            }

            // minimum up-time constraint
            // sum{tp in Tau : 0 <= deltaE[t] - deltaE[tp] <= Yu} ycgb[tp] <= y[t] forall t in Tau : deltaE[t] > (Yu-Yu0)*y0
            for (int t = 0; t < nt; t++)
            {
                if (params.time_elapsed.at(t) > (P["Yu"] - P["Yu0"]) * P["y0"])
                {
                    REAL* row = new REAL[nt + 2];
                    int* col = new int[nt + 2];

                    int i = 0;
                    for (int tp = 0; tp < nt; tp++)
                    {
                        double delta_time = params.time_elapsed.at(t) - params.time_elapsed.at(tp);
                        if ((delta_time >= 0) && (delta_time < P["Yu"]))
                        {
                            row[i] = 1.;
                            col[i++] = O.column("ycgb", tp);
                        }
                    }
                    row[i] = -1.;
                    col[i++] = O.column("y", t);

                    add_constraintex(lp, i, row, col, LE, 0.);

                    delete[] row;
                    delete[] col;
                }
            }

            // minimum down-time constraint
            // sum{tp in Tau : 0 <= deltaE[t] - deltaE[tp] <= Yd} ycge[tp] <= 1 - y[t] forall t in Tau : deltaE[t] > (Yd-Yd0)*(1-y0)
            for (int t = 0; t < nt; t++)
            {
                if (params.time_elapsed.at(t) > (P["Yd"] - P["Yd0"])* (1 - P["y0"]))
                {
                    REAL* row = new REAL[nt + 2];
                    int* col = new int[nt + 2];

                    int i = 0;
                    for (int tp = 0; tp < nt; tp++)
                    {
                        double delta_time = params.time_elapsed.at(t) - params.time_elapsed.at(tp);
                        if ((delta_time >= 0) && (delta_time < P["Yd"]))
                        {
                            row[i] = 1.;
                            col[i++] = O.column("ycge", tp);
                        }
                    }
                    row[i] = 1.;
                    col[i++] = O.column("y", t);

                    add_constraintex(lp, i, row, col, LE, 1.);

                    delete[] row;
                    delete[] col;
                }
            }

            // cycle minimum up time initial enforcement
            // y[t] = y0 forall t in Tau : deltaE[t] <= max{ (Yu - Yu0) * y0 , (Yd - Yd0) * (1 - y0) }
            for (int t = 0; t < nt; t++)
            {
                if (params.time_elapsed.at(t) <= std::max((P["Yu"] - P["Yu0"]) * P["y0"], (P["Yd"] - P["Yd0"]) * (1 - P["y0"])))
                {
                    int i = 0;
                    row[i] = 1.;
                    col[i++] = O.column("y", t);

                    add_constraintex(lp, i, row, col, EQ, P["y0"]);
                }
                else
                {
                    break;
                }
            }
        }

        // ******************** TES Balance constraints *******************
        {
            REAL row[8];
            int col[8];

            for(int t=0; t<nt; t++)
            {
                int i = 0; // row and col index, reset for every constraint

                // Energy in, out, and stored in the TES system must balance.
                // delta * qeh[t] - delta_hsu * zhsu[t] - delta * qdot[t] - delta * Qcsu * ycsu[t] = s[t] - s[t-1]
                {
                    double rhs = 0.;
                    row[i] = P["delta"];
                    col[i++] = O.column("qeh", t);

                    row[i] = -P["delta_hsu"];
                    col[i++] = O.column("zhsu", t);

#ifdef ALT_ETES_FORM
                    row[i] = P["delta_csu"];
                    col[i++] = O.column("zcsu", t);
#endif

                    row[i] = -P["delta"];
                    col[i++] = O.column("qdot", t);

                    row[i] = -P["delta"] * P["Qcsu"];
                    col[i++] = O.column("ycsu", t);

                    row[i] = -1.;
                    col[i++] = O.column("s", t);

                    if (t > 0)
                    {
                        row[i] = 1.;
                        col[i++] = O.column("s", t - 1);

                        // Heat loss term would need to be on the RHS +
                    }
                    else
                    {
                        rhs += -P["s0"]; //initial storage state (kWh)
                    }
                    add_constraintex(lp, i, row, col, EQ, rhs);
                }

                // Storage maximium limit
                // s[t] <= Eu
                {
                    i = 0;

                    row[i] = 1.;
                    col[i++] = O.column("s", t);

                    add_constraintex(lp, i, row, col, LE, P["Eu"]);
                }
            }
        }

        // ****************** Auxiliary variables constraints ***************
        {
            REAL row[4];
            int col[4];

            for (int t = 0; t < nt; t++)
            {
                int i = 0;  // row and col index, reset for every constraint

                //******* linearization of zhsu[t] = qeh[t] * yhsu[t] ******
                {
                    // Upper bound with Qehu
                    // zhsu[t] <= Qehu * yhsu[t]
                    {
                        i = 0;

                        row[i] = 1.;
                        col[i++] = O.column("zhsu", t);

                        row[i] = -P["Qehu"];
                        col[i++] = O.column("yhsu", t);

                        add_constraintex(lp, i, row, col, LE, 0);
                    }

                    // Upper bound with qeh[t]
                    // zhsu[t] <= qeh[t]
                    {
                        i = 0;

                        row[i] = 1.;
                        col[i++] = O.column("zhsu", t);

                        row[i] = -1;
                        col[i++] = O.column("qeh", t);

                        add_constraintex(lp, i, row, col, LE, 0);
                    }

                    // Lower bound
                    // zhsu[t] >= qeh[t] - Qehu * ( 1 - yhsu[t] )
                    {
                        i = 0;

                        row[i] = 1.;
                        col[i++] = O.column("zhsu", t);

                        row[i] = -1;
                        col[i++] = O.column("qeh", t);

                        row[i] = -P["Qehu"];
                        col[i++] = O.column("yhsu", t);

                        add_constraintex(lp, i, row, col, GE, -P["Qehu"]);
                    }
                }

#ifdef ALT_ETES_FORM
                //******* linearization of zcsu[t] = qdot[t] * ycsu[t] ******
                {
                    // Upper bound with Qu
                    // zcsu[t] <= Qu * ycsu[t]
                    {
                        i = 0;

                        row[i] = 1.;
                        col[i++] = O.column("zcsu", t);

                        row[i] = -P["Qu"];
                        col[i++] = O.column("ycsu", t);

                        add_constraintex(lp, i, row, col, LE, 0);
                    }

                    // Upper bound with qdot[t]
                    // zcsu[t] <= qdot[t]
                    {
                        i = 0;

                        row[i] = 1.;
                        col[i++] = O.column("zcsu", t);

                        row[i] = -1;
                        col[i++] = O.column("qdot", t);

                        add_constraintex(lp, i, row, col, LE, 0);
                    }

                    // Lower bound
                    // zcsu[t] >= qdot[t] - Qu * ( 1 - ycsu[t] )
                    {
                        i = 0;

                        row[i] = 1.;
                        col[i++] = O.column("zcsu", t);

                        row[i] = -1;
                        col[i++] = O.column("qdot", t);

                        row[i] = -P["Qu"];
                        col[i++] = O.column("ycsu", t);

                        add_constraintex(lp, i, row, col, GE, -P["Qu"]);
                    }
                }

                //******* linearization of zwcsu[t] = wdot[t] * ycsu[t] ******
                {
                    // Upper bound with Qu
                    // zwcsu[t] <= (eta^amb / eta^des) Wdotu * ycsu[t]
                    {
                        i = 0;

                        row[i] = 1.;
                        col[i++] = O.column("zcsu", t);

                        row[i] = -(params.eta_pb_expected.at(t) / params.eta_pb_des) * P["Wdotu"];
                        col[i++] = O.column("ycsu", t);

                        add_constraintex(lp, i, row, col, LE, 0);
                    }

                    // Upper bound with wdot[t]
                    // zwcsu[t] <= wdot[t]
                    {
                        i = 0;

                        row[i] = 1.;
                        col[i++] = O.column("zwcsu", t);

                        row[i] = -1;
                        col[i++] = O.column("wdot", t);

                        add_constraintex(lp, i, row, col, LE, 0);
                    }

                    // Lower bound
                    // zwcsu[t] >= wdot[t] - (eta^amb / eta^des) Wdotu * ( 1 - ycsu[t] )
                    {
                        i = 0;

                        row[i] = 1.;
                        col[i++] = O.column("zwcsu", t);

                        row[i] = -1;
                        col[i++] = O.column("wdot", t);

                        row[i] = -(params.eta_pb_expected.at(t) / params.eta_pb_des) * P["Wdotu"];
                        col[i++] = O.column("ycsu", t);

                        add_constraintex(lp, i, row, col, GE, -(params.eta_pb_expected.at(t) / params.eta_pb_des) * P["Wdotu"]);
                    }
                }
#endif
            }
        }
        
        //Set problem to maximize
        set_maxim(lp);

        lp_outputs.clear_output();
        setup_solver_presolve_bbrules(lp);
        bool return_ok = problem_scaling_solve_loop(lp);
        set_lp_solve_outputs(lp);

        // Saving problem and solution for DEBUGGING formulation
        //save_problem_solution_debug(lp);
        //if (solver_params.disp_reporting > 4)
        //    print_log_to_file();

        if (return_ok){
            set_outputs_from_lp_solution(lp, P);
        }

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
        //clean up memory and pass on the exception
        if( lp != NULL )
            delete_lp(lp);

        return false;
    }

    return false;
}

void etes_dispatch_opt::set_outputs_from_lp_solution(lprec* lp, unordered_map<std::string, double>& params)
{
    int nt = (int)m_nstep_opt;

    outputs.clear();
    outputs.resize(nt);

    int ncols = get_Norig_columns(lp);
    int nrows = get_Norig_rows(lp);

    for (int c = 1; c < ncols; c++)
    {
        char* colname = get_origcol_name(lp, c);
        if (!colname) continue;

        char root[15];
        char ind[4];
        if (parse_column_name(colname, root, ind)) continue;  //a 2D variable

        int t = atoi(ind);
        double val = get_var_primalresult(lp, nrows + c);

        if (strcmp(root, "ycsu") == 0)     //Cycle start up
        {
            bool su = (std::abs(1 - val) < 0.001);
            outputs.pb_operation.at(t) = outputs.pb_operation.at(t) || su;
            outputs.q_pb_startup.at(t) = su ? params["Qcsu"] : 0.;
        }
        else if (strcmp(root, "y") == 0)     //Cycle operation
        {
            outputs.pb_operation.at(t) = outputs.pb_operation.at(t) || (std::abs(1. - val) < 0.001);
        }
        else if (strcmp(root, "qdot") == 0)     //Cycle thermal energy consumption
        {
            outputs.q_pb_target.at(t) = val;
        }
        else if (strcmp(root, "yhsu") == 0)     //Receiver start up
        {
            bool su = (std::abs(1 - val) < 0.001);
            outputs.rec_operation.at(t) = outputs.rec_operation.at(t) || su;
            outputs.q_rec_startup.at(t) = su ? params["Qhsu"] : 0.;
        }
        else if (strcmp(root, "yeh") == 0)
        {
            outputs.rec_operation.at(t) = outputs.rec_operation.at(t) || (std::abs(1 - val) < 0.001);
        }
        else if (strcmp(root, "s") == 0)         //Thermal storage charge state
        {
            outputs.tes_charge_expected.at(t) = val;
        }
        else if (strcmp(root, "qeh") == 0)   //receiver production
        {
            outputs.q_sf_expected.at(t) = val * 1.0001;  // small increase to ensure heater starts when minimum power is applied
        }
        else if (strcmp(root, "wdot") == 0) //electricity production
        {
            outputs.w_pb_target.at(t) = val;
        }
    }
}

bool etes_dispatch_opt::set_dispatch_outputs()
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
#ifndef ALT_ETES_FORM
        disp_outputs.q_pc_target += outputs.q_pb_startup.at(m_current_read_step);
#endif
        disp_outputs.q_dot_elec_to_CR_heat = outputs.q_sf_expected.at(m_current_read_step);

        if (disp_outputs.q_pc_target + 1.e-5 < params.q_pb_min)
        {
            disp_outputs.is_pc_su_allowed = false;
            disp_outputs.q_pc_target = 0.0;
        }

        disp_outputs.q_dot_pc_max = params.q_pb_max;
        disp_outputs.etasf_expect = 0.0;
        disp_outputs.qsf_expect = 0.0;
        disp_outputs.qsfprod_expect = outputs.q_sf_expected.at(m_current_read_step);
        disp_outputs.qsfsu_expect = outputs.q_rec_startup.at(m_current_read_step);
        disp_outputs.tes_expect = outputs.tes_charge_expected.at(m_current_read_step);
        disp_outputs.qpbsu_expect = outputs.q_pb_startup.at(m_current_read_step);
        disp_outputs.wpb_expect = outputs.w_pb_target.at(m_current_read_step);
        disp_outputs.rev_expect = disp_outputs.wpb_expect * params.sell_price.at(m_current_read_step);
        disp_outputs.etapb_expect = disp_outputs.wpb_expect / std::max(1.e-6, disp_outputs.q_pc_target)
            * (outputs.pb_operation.at(m_current_read_step) ? 1. : 0.);

        if (m_current_read_step > solver_params.optimize_frequency* solver_params.steps_per_hour)
            throw C_csp_exception("Counter synchronization error in dispatch optimization routine.", "etes_dispatch");
    }
    disp_outputs.time_last = pointers.siminfo->ms_ts.m_time;

    return true;
}
