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
#include "csp_dispatch.h"
//#include "lp_lib.h" 
//#include "lib_util.h"

// TODO: get rid of all the defines
//#define _WRITE_AMPL_DATA 1
#define SOS_NONE            // What does this do?
//#define SOS_SEQUENCE
//#define SOS_MANUAL
//#define SOS_LPSOLVE

//#define MOD_CYCLE_SHUTDOWN

/*

Careful with namespaces in this file.. importing the LPsolve library introduces new macro definitions
and function definitions.

*/

csp_dispatch_opt::csp_dispatch_opt()
{
    outputs.clear();
    params.clear();
}

void csp_dispatch_opt::init(double cycle_q_dot_des, double cycle_eta_des)
{
    set_default_solver_parameters();

    params.clear();

    params.dt = 1. / (double)solver_params.steps_per_hour;  //hr

    params.dt_pb_startup_cold = pointers.mpc_pc->get_cold_startup_time();
    params.dt_pb_startup_hot = pointers.mpc_pc->get_hot_startup_time();
    params.q_pb_standby = pointers.mpc_pc->get_standby_energy_requirement();
    params.e_pb_startup_cold = pointers.mpc_pc->get_cold_startup_energy();
    params.e_pb_startup_hot = pointers.mpc_pc->get_hot_startup_energy();
    params.q_pb_max = pointers.mpc_pc->get_max_thermal_power();
    params.q_pb_min = pointers.mpc_pc->get_min_thermal_power();
    params.w_cycle_pump = pointers.mpc_pc->get_htf_pumping_parasitic_coef();
    params.w_cycle_standby = params.q_pb_standby * params.w_cycle_pump;

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

    //heater parameters
    if (pointers.par_htr != NULL) {
        params.q_eh_min = pointers.par_htr->get_min_power_delivery() * ( 1 + 1e-8 ); // ensures controller doesn't shut down heater at minimum load
        params.q_eh_max = pointers.par_htr->get_max_power_delivery(std::numeric_limits<double>::quiet_NaN());
        params.eta_eh = pointers.par_htr->get_design_electric_to_heat_cop();
        params.is_parallel_heater = true;
    }
    else {
        params.is_parallel_heater = false;
    }

    params.q_pb_des = cycle_q_dot_des;
    params.eta_pb_des = cycle_eta_des;
    double w_pb_des = cycle_q_dot_des * params.eta_pb_des;

    params.eff_table_load.init_linear_cycle_efficiency_table(params.q_pb_min, params.q_pb_des, params.eta_pb_des, pointers.mpc_pc);
    params.eff_table_Tdb.init_efficiency_ambient_temp_table(params.eta_pb_des, w_pb_des, pointers.mpc_pc, &params.wcondcoef_table_Tdb);
}

void csp_dispatch_opt::set_default_solver_parameters()
{
    /*
    The presolve options have been tested and show that the optimal combination of options is as set below.

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

bool csp_dispatch_opt::check_setup(int nstep)
{
    //check parameters and inputs to make sure everything has been set up correctly
    if( (int)params.sell_price.size() < nstep )   return false;
    if ((int)params.w_lim.size() < nstep)   return false;

    if ((int)params.q_sfavail_expected.size() < nstep)   return false;
    if ((int)params.eta_pb_expected.size() < nstep)   return false;
    if ((int)params.f_pb_op_limit.size() < nstep)   return false;
    if ((int)params.w_condf_expected.size() < nstep)   return false;

    //if ((int)params.wnet_lim_min.size() < nstep)   return false;
    //if ((int)params.delta_rs.size() < nstep)   return false;
    
    // TODO: add other checks

    return base_dispatch_opt::check_setup();
}

bool csp_dispatch_opt::update_horizon_parameters(C_csp_tou& mc_tou)
{
    //get the new price signal
    params.sell_price.clear();
    params.sell_price.resize(solver_params.optimize_horizon * solver_params.steps_per_hour, 1.);

    for (int t = 0; t < solver_params.optimize_horizon * solver_params.steps_per_hour; t++)
    {
        C_csp_tou::S_csp_tou_outputs mc_tou_outputs;

        mc_tou.call(pointers.siminfo->ms_ts.m_time + t * 3600. / (double)solver_params.steps_per_hour, mc_tou_outputs);
        params.sell_price.at(t) = mc_tou_outputs.m_elec_price * 1000.0; // $/kWhe -> $/Mhe     //.m_price_mult * params.ppa_price_y1;
    }

    // get the new electricity generation limits
    params.w_lim.clear();
    params.w_lim.resize((int)solver_params.optimize_horizon * (int)solver_params.steps_per_hour, 1.e99);
    int hour_start = (int)(ceil(pointers.siminfo->ms_ts.m_time / 3600. - 1.e-6)) - 1;
    for (int t = 0; t < solver_params.optimize_horizon * solver_params.steps_per_hour; t++)
    {
        for (int d = 0; d < solver_params.steps_per_hour; d++) {
            double W_dot_max = params.q_pb_max * params.eta_pb_des;     //[kWe]
            C_csp_tou::S_csp_tou_outputs tou_outputs;
            mc_tou.call((hour_start + t + 1)*3600.0, tou_outputs);
            params.w_lim.at(t * solver_params.steps_per_hour + d) = tou_outputs.m_wlim_dispatch * W_dot_max;
                //params.w_lim.at(t * solver_params.steps_per_hour + d) = mc_tou.mc_dispatch_params.m_w_lim_full.at(hour_start + t);
        }
    }

    return true;
}

void csp_dispatch_opt::update_initial_conditions(double q_dot_to_pb, double T_htf_cold_des, double pc_state_persist)
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

bool csp_dispatch_opt::predict_performance(int step_start, int ntimeints, int divs_per_int)
{
    //Step number - 1-based index for first hour of the year.

    //save step count
    m_nstep_opt = ntimeints;

    //Predict performance out nstep values.
    params.eta_sf_expected.clear();         //thermal efficiency
    params.q_sfavail_expected.clear();      //predicted field energy output
    params.eta_pb_expected.clear();         //power cycle efficiency
    params.f_pb_op_limit.clear();           // Maximum power cycle output (normalized)
    params.w_condf_expected.clear();        //condenser power

    //create the sim info
    C_csp_solver_sim_info simloc;
	simloc.ms_ts.m_step = pointers.siminfo->ms_ts.m_step;

    double Asf = pointers.col_rec->get_collector_area();

    double ave_weight = 1./(double)divs_per_int;

    for(int i=0; i<m_nstep_opt; i++)
    {
        //initialize hourly average values
        double therm_eff_ave = 0.;
        double cycle_eff_ave = 0.;
        double q_inc_ave = 0.;
        double wcond_ave = 0.;
		double f_pb_op_lim_ave = 0.0;

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
            therm_eff *= params.sf_effadj;
            therm_eff_ave += therm_eff * ave_weight;

            //C_csp_fresnel_collector_receiver x;

            //store the predicted field energy output
            // use the cold tank temperature as a surrogate for the loop inlet temperature, as it
            //  closely follows the loop inlet temperature, and is more representative over the
            //  two-day lookahead period than the loop inlet temperature (design or actual) at the
            //  same point in time
            double T_tank_cold = pointers.tes->get_cold_temp() - 273.15;   // [C]
            double q_max = pointers.col_rec->get_max_power_delivery(T_tank_cold);     // [kW]
            q_inc_ave += (std::min)(q_max, q_inc * therm_eff * ave_weight);

            //store the power cycle efficiency
            double cycle_eff = params.eff_table_Tdb.interpolate( pointers.m_weather.ms_outputs.m_tdry );
            cycle_eff *= params.eta_pb_des;
            cycle_eff_ave += cycle_eff * ave_weight;

			double f_pb_op_lim_local = std::numeric_limits<double>::quiet_NaN();
			double m_dot_htf_max_local = std::numeric_limits<double>::quiet_NaN();
            pointers.mpc_pc->get_max_power_output_operation_constraints(pointers.m_weather.ms_outputs.m_tdry, m_dot_htf_max_local, f_pb_op_lim_local);
			f_pb_op_lim_ave += f_pb_op_lim_local * ave_weight;	//[-]

            //store the condenser parasitic power fraction
            double wcond_f = params.wcondcoef_table_Tdb.interpolate( pointers.m_weather.ms_outputs.m_tdry );
            wcond_ave += wcond_f * ave_weight;

		    simloc.ms_ts.m_time += simloc.ms_ts.m_step;
            pointers.m_weather.converged();
        }

        //-----report hourly averages
        //thermal efficiency
        params.eta_sf_expected.push_back(therm_eff_ave);
        //predicted field energy output
        params.q_sfavail_expected.push_back( q_inc_ave );
        //power cycle efficiency
        params.eta_pb_expected.push_back( cycle_eff_ave );
		// Maximum power cycle output (normalized)
        params.f_pb_op_limit.push_back(f_pb_op_lim_ave);
        //condenser power
        params.w_condf_expected.push_back( wcond_ave );
    }

    if(! check_setup(m_nstep_opt) )
        throw C_csp_exception("Dispatch optimization precheck failed.");
    
    return true;
}

static void calculate_parameters(csp_dispatch_opt *optinst, unordered_map<std::string, double> &pars, int nt)
{
    /* 
    A central location for making sure the parameters from the model are accurately calculated for use in
    the dispatch optimization model. 
    */

        pars["T"] = nt ;
        pars["delta"] = optinst->params.dt;
        pars["Eu"] = optinst->params.e_tes_max ;
        pars["Er"] = optinst->params.e_rec_startup ;
        pars["Ec"] = optinst->params.e_pb_startup_cold ;
        pars["Qu"] = optinst->params.q_pb_des ;
        pars["Ql"] = optinst->params.q_pb_min ;
        pars["Qru"] = optinst->params.e_rec_startup / optinst->params.dt_rec_startup;
        pars["Qrl"] = optinst->params.q_rec_min ;
        pars["Qc"] = optinst->params.e_pb_startup_cold / ceil(optinst->params.dt_pb_startup_cold/pars["delta"]) / pars["delta"];
        pars["Qb"] = optinst->params.q_pb_standby ;
        pars["Lr"] = optinst->params.w_rec_pump ;
        pars["Lc"] = optinst->params.w_cycle_pump;
        pars["Wh"] = optinst->params.w_track;
        pars["Wb"] = optinst->params.w_cycle_standby;
        pars["Ehs"] = optinst->params.w_stow;
        pars["Wrsb"] = optinst->params.w_rec_ht;
        pars["eta_cycle"] = optinst->params.eta_pb_des;
        pars["Qrsd"] = 0.;      //<< not yet modeled, passing temporarily as zero

        if (optinst->params.is_parallel_heater) {
            pars["Qehu"] = optinst->params.q_eh_max;
            pars["Qehl"] = optinst->params.q_eh_min;
            pars["eta_eh"] = optinst->params.eta_eh;
            pars["hsu_cost"] = optinst->params.hsu_cost;
        }

        // Initial conditions
        pars["s0"] = optinst->params.e_tes0;
        pars["ursu0"] = 0.;
        pars["ucsu0"] = 0.;
        pars["y0"] = (optinst->params.is_pb_operating0 ? 1 : 0);
        pars["ycsb0"] = (optinst->params.is_pb_standby0 ? 1 : 0);
        pars["q0"] =  optinst->params.q_pb0;

        pars["qrecmaxobs"] = 1.;
        for(int i=0; i<(int)optinst->params.q_sfavail_expected.size(); i++)
            pars["qrecmaxobs"] = optinst->params.q_sfavail_expected.at(i) > pars["qrecmaxobs"] ? optinst->params.q_sfavail_expected.at(i) : pars["qrecmaxobs"];

        pars["Qrsb"] = optinst->params.q_rec_standby; // * dq_rsu;
        pars["W_dot_cycle"] = optinst->params.q_pb_des * optinst->params.eta_pb_des;

        // power cycle linear performance curve
        double intercept;
        optinst->params.eff_table_load.get_slope_intercept_cycle_linear_performance(&pars["etap"], &intercept);

        // maximum power based on linear fit
        pars["Wdotu"] = (pars["etap"] * pars["Qu"] + intercept);
        // minimum power based on linear fit
        pars["Wdotl"] = (pars["etap"] * pars["Ql"] + intercept);

        pars["Wdot0"] = 0.;
        if( pars["q0"] >= pars["Ql"] )
            pars["Wdot0"] = (pars["etap"] * pars["q0"] + intercept) * optinst->params.eta_pb_expected.at(0) / optinst->params.eta_pb_des;

        //TODO: Ramp rate -> User input
        pars["Wdlim"] = pars["W_dot_cycle"] * 0.03 * 60. * pars["delta"];      //Cycle Power Ramping Limit = Rated cycle power * 3%/min (ramp limit "User Input") * 60 mins/hr * hr

        // TODO: This should be moved...
        // Adjust wlim if specified value is too low to permit cycle operation
        optinst->params.wnet_lim_min.resize(nt);
        optinst->params.delta_rs.resize(nt);
        for(int t=0; t<nt; t++)
        {
		    double wmin = (pars["Ql"] * pars["etap"]*optinst->params.eta_pb_expected.at(t) / pars["eta_cycle"]) +
                            (pars["Wdotu"] - pars["etap"]*pars["Qu"])*optinst->params.eta_pb_expected.at(t) / pars["eta_cycle"]; // Electricity generation at minimum pb thermal input
		    double max_parasitic = 
                    pars["Lr"] * optinst->params.q_sfavail_expected.at(t) 
                + (pars["Wrsb"] / pars["delta"])
                + (pars["Ehs"] / pars["delta"])
                + pars["Wh"]
                + pars["Wb"]
                + pars["Lc"] * pars["Qu"]
                + optinst->params.w_condf_expected.at(t)*pars["W_dot_cycle"];  // Largest possible parasitic load at time t

            //save for writing to ampl
            optinst->params.wnet_lim_min.at(t) =  wmin - max_parasitic;
            if( t < nt-1 )
            {
                double delta_rec_startup = (std::min)(1., (std::max)(optinst->params.e_rec_startup / (std::max)(optinst->params.q_sfavail_expected.at(t + 1)*pars["delta"], 1.), (std::max)(optinst->params.dt_rec_startup / pars["delta"], 1.e-6)));
                optinst->params.delta_rs.at(t) = delta_rec_startup;
            }
        }

        //Set by user
        pars["disp_time_weighting"] = optinst->params.time_weighting;
        pars["rsu_cost"] = optinst->params.rsu_cost;
        pars["csu_cost"] = optinst->params.csu_cost;
        pars["pen_delta_w"] = optinst->params.pen_delta_w;
};

bool csp_dispatch_opt::optimize()
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
    xr          MWt     Power delivered by the receiver at time t
    xrsu        MWt     Power used by the reciever for start up
    ursu        MWt     Receiver accumulated start-up thermal power at time t
    x           MWt	    Cycle thermal power consumption at time t 
    ucsu        MWt     Cycle accumulated start-up thermal power at time t
    s           MWht    TES reserve quantity at time t (auxiliary variable) 
    wdot        MWe     Electrical power production at time t
    delta_w     MWe     Positive change in power production at time t w/r/t t-1

    ======= When parallel heater is enbled =========
    qeh         MWt     Thermal power delivered by the electric heaters at time t

    -------------------------------------------------------------
    Binary
    -------------------------------------------------------------
    yr              1 if receiver is generating ``usable'' thermal power at time t; 0 otherwise 
    yrsu            1 if receiver is starting up at time t; 0 otherwise 
    yrsb            1 if receiver is in standby at time t; 0 otherwise
    yrsup           1 if reciever startup penalty is enforced at time t; 0 otherwise
    yrhsp           1 if receiver hot startup penalty is enforced at time t; 0 otherwise
    y               1 if cycle is generating electric power at time t; 0 otherwise
    ycsu            1 if cycle is starting up at time t; 0 otherwise
    ycsup           1 if cycle startup penalty is enforced at time t; 0 otherwise

    ======= When cycle standby is enbled =========
    ycsb            1 if cycle is in standby mode at time t; 0 otherwise
    ychsp           1 if cycle hot startup penalty is enforced at time t; 0 otherwise
    yoff            1 if cycle is off at time t; 0 otherwise

    ======= When parallel heater is enbled =========
    yeh             1 if electrical heaters are operating at time t; 0 otherwise
    yreh            1 if receiver and electrical heaters are operating at time t; 0 otherwise (for defocusing rule)
    yhsup           1 if electrical heaters startup cost is enforced at time t; 0 otherwise

    -------------------------------------------------------------
    */
    lprec *lp;
    int ret = 0;


    try{

        //Calculate the number of variables
        int nt = (int)m_nstep_opt;

        unordered_map<std::string, double> P;
        calculate_parameters(this, P, nt);

        //set up the variable structure
        optimization_vars O;
        O.add_var("xr", optimization_vars::VAR_TYPE::REAL_T, optimization_vars::VAR_DIM::DIM_T, nt, 0.);
        O.add_var("xrsu", optimization_vars::VAR_TYPE::REAL_T, optimization_vars::VAR_DIM::DIM_T, nt, 0., P["Qru"]);
        O.add_var("ursu", optimization_vars::VAR_TYPE::REAL_T, optimization_vars::VAR_DIM::DIM_T, nt, 0., P["Er"] * 1.0001);
        O.add_var("yr", optimization_vars::VAR_TYPE::BINARY_T, optimization_vars::VAR_DIM::DIM_T, nt);
        O.add_var("yrsu", optimization_vars::VAR_TYPE::BINARY_T, optimization_vars::VAR_DIM::DIM_T, nt);
        O.add_var("yrsup", optimization_vars::VAR_TYPE::BINARY_T, optimization_vars::VAR_DIM::DIM_T, nt);

        O.add_var("x", optimization_vars::VAR_TYPE::REAL_T, optimization_vars::VAR_DIM::DIM_T, nt, 0., P["Qu"]);
        O.add_var("wdot", optimization_vars::VAR_TYPE::REAL_T, optimization_vars::VAR_DIM::DIM_T, nt, 0., P["Wdotu"] * 1.1);
        O.add_var("delta_w", optimization_vars::VAR_TYPE::REAL_T, optimization_vars::VAR_DIM::DIM_T, nt, 0., P["Wdotu"] * 1.1);
        O.add_var("s", optimization_vars::VAR_TYPE::REAL_T, optimization_vars::VAR_DIM::DIM_T, nt, 0., P["Eu"]);
        O.add_var("ucsu", optimization_vars::VAR_TYPE::REAL_T, optimization_vars::VAR_DIM::DIM_T, nt, 0., P["Ec"] * 1.0001);
        O.add_var("y", optimization_vars::VAR_TYPE::BINARY_T, optimization_vars::VAR_DIM::DIM_T, nt);
        O.add_var("ycsu", optimization_vars::VAR_TYPE::BINARY_T, optimization_vars::VAR_DIM::DIM_T, nt);
        O.add_var("ycsup", optimization_vars::VAR_TYPE::BINARY_T, optimization_vars::VAR_DIM::DIM_T, nt);

#ifdef MOD_CYCLE_SHUTDOWN
        O.add_var("ycsd", optimization_vars::VAR_TYPE::BINARY_T, optimization_vars::VAR_DIM::DIM_T, nt);
#endif
        if (params.can_cycle_use_standby) {
            O.add_var("ycsb", optimization_vars::VAR_TYPE::BINARY_T, optimization_vars::VAR_DIM::DIM_T, nt);
            O.add_var("ychsp", optimization_vars::VAR_TYPE::BINARY_T, optimization_vars::VAR_DIM::DIM_T, nt);
            O.add_var("yoff", optimization_vars::VAR_TYPE::BINARY_T, optimization_vars::VAR_DIM::DIM_T, nt);
        }

        if (params.is_parallel_heater) {
            O.add_var("qeh", optimization_vars::VAR_TYPE::REAL_T, optimization_vars::VAR_DIM::DIM_T, nt, 0., P["Qehu"]);
            O.add_var("yeh", optimization_vars::VAR_TYPE::BINARY_T, optimization_vars::VAR_DIM::DIM_T, nt);
            O.add_var("yreh", optimization_vars::VAR_TYPE::BINARY_T, optimization_vars::VAR_DIM::DIM_T, nt);
            O.add_var("yhsup", optimization_vars::VAR_TYPE::BINARY_T, optimization_vars::VAR_DIM::DIM_T, nt);
        }
        
        // Construct LP model and set up variable properties
        lp = construct_lp_model(&O);

        /* 
        --------------------------------------------------------------------------------
        set up the objective function first (per lpsolve guidance)
        --------------------------------------------------------------------------------
        */
		{
            REAL* row = new REAL[14 * nt + 1];
            int *col = new int[14 * nt + 1];
            double tadj = P["disp_time_weighting"];
            int i = 0;

            //calculate the mean price to appropriately weight the receiver production timing derate
            double pmean =0;
            for(int t=0; t<(int)params.sell_price.size(); t++)
                pmean += params.sell_price.at(t);
            pmean /= (double)params.sell_price.size();
            
            for(int t=0; t<nt; t++)
            {
                i = 0;
                row[t + nt * (i)] = P["delta"] * params.sell_price.at(t) * tadj * (1. - params.w_condf_expected.at(t));
                col[t + nt * (i++)] = O.column("wdot", t);

                row[t + nt * (i)] = -(P["delta"] * params.sell_price.at(t) * (1 / tadj) * P["Lr"]); // tadj added to prefer receiver production sooner (i.e. delay dumping)
                col[t + nt * (i++)] = O.column("xr", t);

                row[t + nt * (i)] = -P["delta"] * params.sell_price.at(t) * (1 / tadj) * P["Lr"];
                col[t + nt * (i++)] = O.column("xrsu", t);

                row[t + nt * (i)] = -params.sell_price.at(t) * (1 / tadj) * (P["Wrsb"] + P["Ehs"]);
                col[t + nt * (i++)] = O.column("yrsu", t);

                row[t + nt * (i)] = -(P["delta"] * params.sell_price.at(t) * (1 / tadj) * P["Wh"]); // tadj added to prefer receiver operation in nearer term to longer term
                col[t + nt * (i++)] = O.column("yr", t);

                row[t + nt * (i)] = -P["delta"] * params.sell_price.at(t) * (1 / tadj) * P["Lc"];
                col[t + nt * (i++)] = O.column("x", t);

                row[t + nt * (i)] = -P["pen_delta_w"] * (1 / tadj);
                col[t + nt * (i++)] = O.column("delta_w", t);

                row[t + nt * (i)] = -P["rsu_cost"] * (1 / tadj);
                col[t + nt * (i++)] = O.column("yrsup", t);

                row[t + nt * (i)] = -P["csu_cost"] * (1 / tadj);
                col[t + nt * (i++)] = O.column("ycsup", t);

                if (params.can_cycle_use_standby) {
                    row[t + nt * (i)] = -P["delta"] * params.sell_price.at(t) * (1 / tadj) * P["Wb"];
                    col[t + nt * (i++)] = O.column("ycsb", t);

                    row[t + nt * (i)] = -P["csu_cost"] * (1 / tadj) * 0.1;
                    col[t + nt * (i++)] = O.column("ychsp", t);
                }

                if (params.is_parallel_heater) {
                    row[t + nt * (i)] = -(P["delta"] * params.sell_price.at(t) * (1 / tadj) * (1 / P["eta_eh"]));
                    col[t + nt * (i++)] = O.column("qeh", t);

                    row[t + nt * (i)] = -(1 / tadj) * P["hsu_cost"];
                    col[t + nt * (i++)] = O.column("yhsup", t);
                }

                tadj *= P["disp_time_weighting"];
            }

            // new terminal inventory incentive
            row[i * nt] = P["delta"] * tadj * pmean * P["eta_cycle"] * params.inventory_incentive;
            col[i * nt] = O.column("s", nt - 1);

            set_obj_fnex(lp, i*nt+1, row, col);

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
                // yrsu[t] + yr[t] <= 1
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
                // TODO: This relies on presolve to remove variables (might want to get a special set of solar hours)
                // yrsu[t] <= 0 when Qin[t] = 0
                {
                    i = 0;
                    row[i] = 1.;
                    col[i++] = O.column("yrsu", t);

                    add_constraintex(lp, i, row, col, LE, (std::min)(P["Qru"] * params.q_sfavail_expected.at(t), 1.0));
                }

                // Receiver can't continue operating when no energy is available
                // TODO: Can we combine these two constraints 
                // yr[t] <= Qin[t] / Qrl
                {
                    i = 0;
                    row[i] = 1.;
                    col[i++] = O.column("yr", t);

                    add_constraintex(lp, i, row, col, LE, (std::min)(floor(params.q_sfavail_expected.at(t) / P["Qrl"]), 1.0)); //tighter formulation
                }

                // Receiver startup penalty
                // yrsup[t] >= yrsu[t] - yrsu[t-1]
                {
                    if (t > 0)
                    {
                        i = 0;
                        row[i] = 1.;
                        col[i++] = O.column("yrsup", t);

                        row[i] = -1.;
                        col[i++] = O.column("yrsu", t);

                        row[i] = 1.;
                        col[i++] = O.column("yrsu", t - 1);

                        add_constraintex(lp, i, row, col, GE, 0.);
                    }
                }

                // --- Receiver Standby constraints - Turned off ---

                // Receiver startup/standby persist
                /*row[0] = 1.;
                col[0] = O.column("yrsu", t);

                row[1] = 1.;
                col[1] = O.column("yrsb", t);

                add_constraintex(lp, 2, row, col, LE, 1.);*/

                // Receiver standby partition
                /*row[0] = 1.;
                col[0] = O.column("yr", t);

                row[1] = 1.;
                col[1] = O.column("yrsb", t);

                add_constraintex(lp, 2, row, col, LE, 1.);*/

                // Receiver standby persist
                /*if (t > 0)
                {
                    
                    row[0] = 1.;
                    col[0] = O.column("yrsb", t);

                    row[1] = -1.;
                    col[1] = O.column("yr", t-1);

                    row[2] = -1.;
                    col[2] = O.column("yrsb", t-1);

                    add_constraintex(lp, 3, row, col, LE, 0.);
                }*/

                //receiver hot startup penalty
                /*if (t > 0)
                {
                    row[0] = 1.;
                    col[0] = O.column("yrhsp", t);

                    row[1] = -1.;
                    col[1] = O.column("yr", t);

                    row[2] = -1.;
                    col[2] = O.column("yrsb", t-1);

                    add_constraintex(lp, 3, row, col, GE, -1);
                }*/

                //receiver shutdown energy 
                /*if (t > 0)
                {
                    /*row[0] = 1.;
                    //condition off of Delta[t] for the two constraint forms
                    if (P["delta"] >= 1.)
                    {
                        col[0] = O.column("yrsd", t - 1);       //(hourly -> Delta = 1)
                    }
                    else
                    {
                        col[0] = O.column("yrsd", t);           //(sub-hourly -> Delta < 1)
                    }
                    row[1] = -1.;
                    col[1] = O.column("yr", t-1);

                    row[2] = 1.;
                    col[2] = O.column("yr", t);

                    row[3] = -1.;
                    col[3] = O.column("yrsb", t-1);

                    row[4] = 1.;
                    col[4] = O.column("yrsb", t);

                    add_constraintex(lp, 5, row, col, GE, 0.);
                } */
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

                // Heaters and cycle cannot coincide
                // yeh[t] + y[t] <= 1
                {
                    i = 0;
                    row[i] = 1.;
                    col[i++] = O.column("yeh", t);

                    row[i] = 1.;
                    col[i++] = O.column("y", t);

                    add_constraintex(lp, i, row, col, LE, 1.);
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

                // Heater startup penalty
                // yhsup[t] >= yeh[t] - yeh[t-1]
                {
                    i = 0;
                    row[i] = 1.;
                    col[i++] = O.column("yhsup", t);

                    row[i] = -1.;
                    col[i++] = O.column("yeh", t);

                    if (t > 0)
                    {
                        row[i] = 1.;
                        col[i++] = O.column("yeh", t - 1);
                    }

                    add_constraintex(lp, i, row, col, GE, 0.);
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

                // Startup inventory balance
                // ucsu[t] <= ucsu[t] + delta * Qc * ycsu[t]
                {
                    row[i] = 1.;
                    col[i++] = O.column("ucsu", t);

                    row[i] = -P["delta"] * P["Qc"];
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

                // Cycle operation allowed when startup is complete or already operating or in standby
                // hourly:    y[t] <=  ucsu[t  ] / Ec + y[t-1] + ycsb[t-1]
                // subhourly: y[t] <=  ucsu[t-1] / Ec + y[t-1] + ycsb[t-1]   (startup and production cannot coincide)
                // NOTE: tighter formulation when Ec is distributed
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
                        col[i++] = O.column("ucsu", t - 1);             // for sub-hourly model (delta < 1) TODO: Add initial conditions
                    }

                    double rhs = 0.;

                    if (t > 0) {
                        row[i] = -P["Ec"];
                        col[i++] = O.column("y", t - 1);

                        if (params.can_cycle_use_standby)
                        {
                            row[i] = -P["Ec"];
                            col[i++] = O.column("ycsb", t - 1);
                        }
                    }
                    else {
                        rhs += P["Ec"] * P["y0"];

                        if (params.can_cycle_use_standby)
                        {
                            rhs += P["Ec"] * P["ycsb0"];
                        }
                    }

                    add_constraintex(lp, i, row, col, LE, rhs);
                }

                // Cycle consumption limit (valid only for hourly model -> Delta == 1)
                // x[t] + Qc * ycsu[t] <= Qu * y[t]
                {
                    if (P["delta"] >= 1.)
                    {
                        i = 0;
                        row[i] = 1.;
                        col[i++] = O.column("x", t);

                        row[i] = P["Qc"];
                        col[i++] = O.column("ycsu", t);

                        row[i] = -P["Qu"];
                        col[i++] = O.column("y", t);

                        add_constraintex(lp, i, row, col, LE, 0.);
                    }
                }

                // Cycle maximum operation limit
                // x[t] <= Qu * y[t]
                {
                    i = 0;
                    row[i] = 1.;
                    col[i++] = O.column("x", t);

                    row[i] = -P["Qu"];
                    col[i++] = O.column("y", t);

                    add_constraintex(lp, i, row, col, LE, 0.);
                }

                // Cycle minimum operation limit
                // x[t] >= Ql * y[t]
                {
                    i = 0;
                    row[i] = 1.;
                    col[i++] = O.column("x", t);

                    row[i] = -P["Ql"];
                    col[i++] = O.column("y", t);

                    add_constraintex(lp, i, row, col, GE, 0);
                }

                // Power production linearization (power as function of heat input)
                // wdot[t] = eta_amb[t]/eta_des * ( etap * x[t] + ( Wdotu - etap * Qu ) * y[t] )
                {
                    i = 0;
                    row[i] = 1.;
                    col[i++] = O.column("wdot", t);

                    row[i] = -P["etap"] * params.eta_pb_expected.at(t) / P["eta_cycle"];
                    col[i++] = O.column("x", t);

                    row[i] = -(P["Wdotu"] - P["etap"] * P["Qu"]) * params.eta_pb_expected.at(t) / P["eta_cycle"];
                    col[i++] = O.column("y", t);

                    add_constraintex(lp, i, row, col, EQ, 0.);
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

                // Cycle sub-hourly (Delta < 1) ramping limit that doesn't hold when starting up, (or going into standby), (or shutdown)
                // delta_t[t] <= Wdlim + temp_coef * ( y[t] - y[t-1] + 2 * ycsb[t] + 2 * yoff[t] )
                {
                    if (P["delta"] < 1.)
                    {
                        double temp_coef = (params.eta_pb_expected.at(t) / P["eta_cycle"]) * P["Wdotl"] - P["Wdlim"];

                        double rhs = P["Wdlim"];
                        i = 0;
                        row[i] = 1.;
                        col[i++] = O.column("delta_w", t);

                        row[i] = -temp_coef;
                        col[i++] = O.column("y", t);

                        if (t > 0)
                        {
                            row[i] = temp_coef;
                            col[i++] = O.column("y", t - 1);
                        }
                        else
                        {
                            rhs +=  - temp_coef * P["y0"];
                        }

                        if (params.can_cycle_use_standby) {
                            row[i] = -temp_coef * 2.;
                            col[i++] = O.column("ycsb", t);

                            row[i] = -temp_coef * 2.;
                            col[i++] = O.column("yoff", t);
                        }

#ifdef MOD_CYCLE_SHUTDOWN
                        row[i] = -temp_coef * 2.;
                        col[i++] = O.column("ycsd", t);
#endif

                        add_constraintex(lp, i, row, col, LE, rhs);
                    }
                }

                // Cycle startup and operation cannot coincide (valid for sub-hourly model Delta < 1)
                // ycsu[t] + y[t] <= 1
                {
                    if (P["delta"] < 1)
                    {
                        i = 0;
                        row[i] = 1.;
                        col[i++] = O.column("ycsu", t);

                        row[i] = 1.;
                        col[i++] = O.column("y", t);

                        add_constraintex(lp, i, row, col, LE, 1.);
                    }
                }

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
                        rhs += - P["y0"];
                    }

                    add_constraintex(lp, i, row, col, LE, rhs);
                }

                // Cycle start penalty
                // ycsup[t] >= ycsu[t] - ycsu[t-1]
                {
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

                    add_constraintex(lp, i, row, col, GE, 0.);
                }

                // Maximum gross electricity production constraint
                // wdot[t] <= f_limit * W_dot_cycle
                {
                    i = 0;
                    row[i] = 1.;
                    col[i++] = O.column("wdot", t);

                    add_constraintex(lp, i, row, col, LE, params.f_pb_op_limit.at(t) * P["W_dot_cycle"]);
                }

                // Maximum net electricity production constraint
                {
                    //check if cycle should be able to operate
                    if (params.wnet_lim_min.at(t) > params.w_lim.at(t))      // power cycle operation is impossible at t
                    {
                        if (params.w_lim.at(t) > 0)
                            pointers.messages->add_message(C_csp_messages::NOTICE, "Power cycle operation not possible at time " + util::to_string(t + 1) + ": power limit below minimum operation");
                        params.w_lim.at(t) = 0.;
                    }

                    if (params.w_lim.at(t) > 0.)	// Power cycle operation is possible
                    {
                        i = 0;

                        row[i] = 1.0 - params.w_condf_expected.at(t);
                        col[i++] = O.column("wdot", t);

                        row[i] = - P["Lr"];
                        col[i++] = O.column("xr", t);

                        row[i] = - P["Lr"];
                        col[i++] = O.column("xrsu", t);

                        row[i] = - (P["Wrsb"] / P["delta"]) - (P["Ehs"] / P["delta"]);	//MWe
                        col[i++] = O.column("yrsu", t);

                        row[i] = - P["Wh"];
                        col[i++] = O.column("yr", t);

                        if (params.can_cycle_use_standby) {
                            row[i] = - P["Wb"];
                            col[i++] = O.column("ycsb", t);
                        }

                        row[i] = - P["Lc"];
                        col[i++] = O.column("x", t);

                        //row[i  ] = -( P["Lr"] * params.q_rec_min) - (P["Ehs"] / P["delta"]); //kWe
                        //col[i++] = O.column("yrsb", t);
                        //row[i  ] - P["Ehs"] / P["delta"];	//kWe
                        //col[i++] = O.column("yrsd", t);

                        add_constraintex(lp, i, row, col, LE, params.w_lim.at(t));
                    }
                    else // Power cycle operation is impossible at current constrained wlim
                    {
                        i = 0;

                        row[i] = 1.0;
                        col[i++] = O.column("wdot", t);

                        add_constraintex(lp, i, row, col, EQ, 0.);
                    }
                }

                // Cycle standby mode entry
                if (params.can_cycle_use_standby)
                {

                    // Cycle standby mode persistence
                    // ycsb[t] <= y[t-1] + ycsb[t-1]
                    {
                        double rhs = 0.;
                        i = 0;
                        row[i] = 1.;
                        col[i++] = O.column("ycsb", t);

                        if (t > 0)
                        {
                            row[i] = -1.;
                            col[i++] = O.column("y", t - 1);

                            row[i] = -1.;
                            col[i++] = O.column("ycsb", t - 1);
                        }
                        else
                        {
                            rhs += P["y0"] + P["ycsb0"];
                        }

                        add_constraintex(lp, i, row, col, LE, rhs);
                    }

                    // Cycle operation and standby cannot coincide
                    // y[t] + ycsb[t] <= 1
                    {
                        i = 0;
                        row[i] = 1.;
                        col[i++] = O.column("y", t);

                        row[i] = 1.;
                        col[i++] = O.column("ycsb", t);

                        add_constraintex(lp, i, row, col, LE, 1);
                    }

                    // Cycle start-up and standby can't coincide (hourly model only)
                    // ycsu[t] + ycsb[t] <= 1
                    {
                        if (P["delta"] >= 1)
                        {
                            i = 0;
                            row[i] = 1.;
                            col[i++] = O.column("ycsu", t);

                            row[i] = 1.;
                            col[i++] = O.column("ycsb", t);

                            add_constraintex(lp, i, row, col, LE, 1);
                        }
                    }

                    // Set partitioning constraint (with cycle off state)
                    // hourly:               y[t] + ycsb[t] + yoff[t] = 1
                    // sub-hourly: ycsu[t] + y[t] + ycsb[t] + yoff[t] = 1
                    {
                        i = 0;
                        if (P["delta"] < 1)    // sub-hourly model
                        {
                            row[i] = 1.;
                            col[i++] = O.column("ycsu", t);
                        }

                        row[i] = 1.;
                        col[i++] = O.column("y", t);

                        row[i] = 1.;
                        col[i++] = O.column("ycsb", t);

                        row[i] = 1.;
                        col[i++] = O.column("yoff", t);

                        add_constraintex(lp, i, row, col, EQ, 1);
                    }

                    // Standby cut
                    // ycsb[t] + x[t] / Qu <= 1
                    {
                        i = 0;
                        row[i] = 1.;
                        col[i++] = O.column("ycsb", t);

                        row[i] = 1.0 / P["Qu"];
                        col[i++] = O.column("x", t);

                        add_constraintex(lp, i, row, col, LE, 1);
                    }

                    // Cycle standby start penalty
                    // ychsp[t] >= y[t] - (1 - ycsb[t-1]) 
                    {
                        double rhs = -1.;
                        i = 0;
                        row[i] = 1.;
                        col[i++] = O.column("ychsp", t);

                        row[i] = -1.;
                        col[i++] = O.column("y", t);

                        if (t > 0)
                        {
                            row[i] = -1.;
                            col[i++] = O.column("ycsb", t - 1);
                        }
                        else
                        {
                            rhs += P["ycsb0"];
                        }

                        add_constraintex(lp, i, row, col, GE, rhs);
                    }
                }

#ifdef MOD_CYCLE_SHUTDOWN
                if (t > 0)
                {

                    //cycle shutdown energy penalty
                    row[0] = 1.;
                    col[0] = O.column("ycsd", t - 1);

                    row[1] = -1.;
                    col[1] = O.column("y", t - 1);

                    row[2] = 1.;
                    col[2] = O.column("y", t);

                    row[3] = -1.;
                    col[3] = O.column("ycsb", t - 1);

                    row[4] = 1.;
                    col[4] = O.column("ycsb", t);

                    add_constraintex(lp, 5, row, col, GE, 0.);


                }
#endif
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
                // delta * ( xr[t] + qeh[t] - x[t] - Qc * ycsu[t] - Qb * ycsb[t] ) = s[t] - s[t-1]
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

                    row[i] = -P["delta"] * P["Qc"];
                    col[i++] = O.column("ycsu", t);

                    if (params.can_cycle_use_standby) {
                        row[i] = -P["delta"] * P["Qb"];
                        col[i++] = O.column("ycsb", t);
                    }

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

#ifdef MOD_REC_STANDBY                
                    row[i] = -delta * Qrsb;
                    col[i++] = O.column("yrsb", t);
#endif

                    add_constraintex(lp, i, row, col, EQ, rhs);
                }

                // Storage maximium limit
                // s[t] <= Eu
                // TODO: Test if this can be removed using the variable defination
                {
                    i = 0;
                    row[i] = 1.;
                    col[i++] = O.column("s", t);

                    add_constraintex(lp, i, row, col, LE, P["Eu"]);
                }

                // Max cycle thermal input is required in time periods where cycle operates and receiver is starting up
                // x[t+1] + Qb * ycsb[t+1] <= s[t] / delta_rs[t+1] - M * ( -3 + yrsu[t+1] + y[t] + y[t+1] + ycsb[t] + ycsb[t+1] )
                {
                    if (t < nt - 1)
                    {
                        double t_rec_startup = params.delta_rs.at(t) * P["delta"];
                        double large = (std::max)(P["Qu"], P["Qb"]); //tighter formulation

                        i = 0;
                        row[i] = 1.;
                        col[i++] = O.column("x", t + 1);

                        row[i] = -1. / t_rec_startup;
                        col[i++] = O.column("s", t);

                        row[i] = large;
                        col[i++] = O.column("yrsu", t + 1);

                        row[i] = large;
                        col[i++] = O.column("y", t);

                        row[i] = large;
                        col[i++] = O.column("y", t + 1);

                        if (params.can_cycle_use_standby) {
                            row[i] = large;
                            col[i++] = O.column("ycsb", t);

                            row[i] = P["Qb"] + large;
                            col[i++] = O.column("ycsb", t + 1);
                        }

                        add_constraintex(lp, i, row, col, LE, 3.0 * large);
                    }
                }

            }
        }

        //Set problem to maximize
        set_maxim(lp);

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

        //TODO: why is this here?
        //if(return_ok)
        //    write_ampl(); 

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

std::string csp_dispatch_opt::write_ampl()
{
    /* 
    Write the par file for ampl input

    return name of output file, if error, return empty string.
    */

    std::string sname;

    //write out a data file
    if(solver_params.is_write_ampl_dat || solver_params.is_ampl_engine)
    {
		int day = (int)pointers.siminfo->ms_ts.m_time / 3600 / 24;
        //char outname[200];
        //sprintf(outname, "%sdata_%d.dat", solver_params.ampl_data_dir.c_str(), day);

        std::stringstream outname;
        //outname << solver_params.ampl_data_dir << "data_" << day << ".dat";        
        outname << solver_params.ampl_data_dir << "sdk_data.dat";
        
        sname = outname.str();    //save string

        std::ofstream fout(outname.str().c_str());

        int nt = m_nstep_opt;

        unordered_map<std::string, double> pars;
        calculate_parameters(this, pars, nt);

        fout << "#data file\n\n";
        fout << "# --- scalar parameters ----\n";
        fout << "param day_of_year := " << day << ";\n";

        std::vector<std::string> keys;

        for( unordered_map<std::string, double>::iterator parval = pars.begin(); parval != pars.end(); parval++)
            keys.push_back( parval->first );

        //TODO: causes compiler error
        //std::sort( keys.begin(), keys.end(), base_dispatch_opt::strcompare );

        for(size_t k=0; k<keys.size(); k++)
            fout << "param " << keys.at(k) << " := " << pars[keys.at(k)] << ";\n";

        fout << "# --- indexed parameters ---\n";
        fout << "param Qin := \n";
        for(int t=0; t<nt; t++)
            fout << t+1 << "\t" << params.q_sfavail_expected.at(t) << "\n";
        fout << ";\n\n";

        fout << "param P := \n";
        for(int t=0; t<nt; t++)
            fout << t+1 << "\t" << params.sell_price.at(t) << "\n";
        fout << ";\n\n";

        fout << "param etaamb := \n";   //power block ambient adjustment
        for(int t=0; t<nt; t++)
            fout << t+1 << "\t" << params.eta_pb_expected.at(t) << "\n";
        fout << ";\n\n";

        //net power limit
        fout << "param Wdotnet := \n";
        for(int t=0; t<nt; t++)
            fout << t+1 << "\t" << params.w_lim.at(t) << "\n";
        fout << ";\n\n";
        
        //condenser parasitic loss coefficient
        fout << "param etac := \n";
        for(int t=0; t<nt; t++)
            fout << t+1 << "\t" << params.w_condf_expected.at(t) << "\n";
        fout << ";\n\n";

        //cycle net production lower limit
        fout << "param wnet_lim_min := \n";
        for(int t=0; t<nt; t++)
            fout << t+1 << "\t" << params.wnet_lim_min.at(t) << "\n";
        fout << ";\n\n";

        //cycle net production lower limit
        fout << "param delta_rs := \n";
        for(int t=0; t<nt; t++)
            fout << t+1 << "\t" << params.delta_rs.at(t) << "\n";
        fout << ";\n\n";

        fout.close();
    }

    return sname;
}

bool csp_dispatch_opt::optimize_ampl()
{
    /* 
    handle the process of writing an input file, running ampl, handling results, and loading solution

    writes
        dat_<day #>.dat
    runs
        sdk_dispatch.run
    expects
        sdk_solution.txt input file
    */

    //check whether the ampl parameters have been configured correctly. If not, throw.
    if(! util::dir_exists( solver_params.ampl_data_dir.c_str() ) )
        throw C_csp_exception("The specified AMPL data directory is invalid.");
    
    std::stringstream tstring;

    std::string datfile = write_ampl();
    if( datfile.empty() )
        throw C_csp_exception("An error occured when writing the AMPL input file.");
    
    ////call ampl
    //tstring << "ampl \"" << solver_params.ampl_data_dir << "sdk_dispatch.run\"";

    if( system(NULL) ) puts ("Ok");
    else exit(EXIT_FAILURE);

    //int sysret = system(tstring.str().c_str());
    int sysret = system( solver_params.ampl_exec_call.c_str() );

    //read back ampl solution
    tstring.str(std::string()); //clear

    tstring << solver_params.ampl_data_dir << "sdk_solution.txt";
    std::ifstream infile(tstring.str().c_str());

    if(! infile.is_open() )
        return false;
    
    std::vector< std::string > F;

    char line[1000];
    while( true )
    {
        infile.getline( line, 1000 );

        if( infile.eof() )
            break;

        F.push_back( line );
    }

    /* 
    expects:
    1.  objective (1)
    2.  relaxed objective (1)
    3.  y_t^{csb} (nt)
    4.  y_t (nt)
    5.  energy used for cycle standby (nt)
    6.  x_t (nt)
    7.  y_t^r (nt)
    8.  s_t (nt)
    9.  energy used for cycle startup (nt)
    10. energy used for receiver startup (nt)
    11. x_t^r   solar field energy produced (nt)

    Each item will be on it's own line. Values in arrays are separated by commas.
    */
    
    int nt = m_nstep_opt;
    outputs.clear();
    outputs.resize(nt);
    
    util::to_double(F.at(0), &lp_outputs.objective);
    util::to_double(F.at(1), &lp_outputs.objective_relaxed);
    
    std::vector< std::string > svals;

    svals = util::split( F.at(2), "," );
    for(int i=0; i<nt; i++)
    {
        int v;
        util::to_integer(svals.at(i), &v );
        outputs.pb_standby.at(i) = v == 1;
    }
    svals = util::split( F.at(3), "," );
    for(int i=0; i<nt; i++)
    {
        int v;
        util::to_integer(svals.at(i), &v );
        outputs.pb_operation.at(i) = v == 1;
    }
    svals = util::split( F.at(4), "," );
    for(int i=0; i<nt; i++)
        util::to_double( svals.at(i), &outputs.q_pb_standby.at(i) );
    svals = util::split( F.at(5), "," );
    for(int i=0; i<nt; i++)
        util::to_double( svals.at(i), &outputs.q_pb_target.at(i) );
    svals = util::split( F.at(6), "," );
    for(int i=0; i<nt; i++)
    {
        int v;
        util::to_integer(svals.at(i), &v );
        outputs.rec_operation.at(i) = v == 1;
    }
    svals = util::split( F.at(7), "," );
    for(int i=0; i<nt; i++)
        util::to_double( svals.at(i), &outputs.tes_charge_expected.at(i) );
    svals = util::split( F.at(8), "," );
    for(int i=0; i<nt; i++)
        util::to_double( svals.at(i), &outputs.q_pb_startup.at(i) );
    svals = util::split( F.at(9), "," );
    for(int i=0; i<nt; i++)
        util::to_double( svals.at(i), &outputs.q_rec_startup.at(i) );
    svals = util::split( F.at(10), "," );
    for(int i=0; i<nt; i++)
        util::to_double( svals.at(i), &outputs.q_sf_expected.at(i) );
    svals = util::split( F.at(11), "," );
    for(int i=0; i<nt; i++)
        util::to_double( svals.at(i), &outputs.w_pb_target.at(i) );

    return true;
}

void csp_dispatch_opt::set_outputs_from_lp_solution(lprec* lp, unordered_map<std::string, double>& model_params)
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

        if (strcmp(root, "ycsu") == 0)     //Cycle start up
        {
            bool su = (std::abs(1 - val) < 0.001);
            outputs.pb_operation.at(t) = outputs.pb_operation.at(t) || su;
            outputs.q_pb_startup.at(t) = su ? model_params["Qc"] : 0.;
        }
        else if (strcmp(root, "y") == 0)  // is cycle operating
        {
            outputs.pb_operation.at(t) = outputs.pb_operation.at(t) || (std::abs(1. - val) < 0.001);
        }
        else if (strcmp(root, "x") == 0)  // cycle thermal energy consumption
        {
            outputs.q_pb_target.at(t) = val;
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
        else if (strcmp(root, "wdot") == 0)  // electricity production
        {
            outputs.w_pb_target.at(t) = val;
        }
        else if (strcmp(root, "ycsb") == 0)  // cycle standby
        {
            outputs.pb_standby.at(t) = val == 1.;
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

bool csp_dispatch_opt::set_dispatch_outputs()
{
    if (lp_outputs.last_opt_successful && m_current_read_step < (int)outputs.q_pb_target.size())
    {
        //calculate the current read step, account for number of dispatch steps per hour and the simulation time step
        m_current_read_step = (int)(pointers.siminfo->ms_ts.m_time * solver_params.steps_per_hour / 3600. - .001)
            % (solver_params.optimize_frequency * solver_params.steps_per_hour);


        disp_outputs.is_rec_su_allowed = outputs.rec_operation.at(m_current_read_step);
        disp_outputs.is_pc_sb_allowed = outputs.pb_standby.at(m_current_read_step);
        disp_outputs.is_pc_su_allowed = outputs.pb_operation.at(m_current_read_step) || disp_outputs.is_pc_sb_allowed;

        disp_outputs.q_pc_target = outputs.q_pb_target.at(m_current_read_step) + outputs.q_pb_startup.at(m_current_read_step);

        disp_outputs.q_dot_elec_to_CR_heat = outputs.q_sf_expected.at(m_current_read_step);

        disp_outputs.q_eh_target = outputs.q_eh_target.at(m_current_read_step);
        disp_outputs.is_eh_su_allowed = outputs.htr_operation.at(m_current_read_step);

        //quality checks
        /*
        if(!is_pc_sb_allowed && (q_pc_target + 1.e-5 < q_pc_min))
            is_pc_su_allowed = false;
        if(is_pc_sb_allowed)
            q_pc_target = dispatch.params.q_pb_standby*1.e-3;
        */

        if (disp_outputs.q_pc_target + 1.e-5 < params.q_pb_min)
        {
            disp_outputs.is_pc_su_allowed = false;
            disp_outputs.q_pc_target = 0.0;
        }

        // Calculate approximate upper limit for power cycle thermal input at current electricity generation limit
        if (params.w_lim.at(m_current_read_step) < 1.e-6)
        {
            disp_outputs.q_dot_pc_max = 0.0;
        }
        else
        {
            double wcond;
            double eta_corr = pointers.mpc_pc->get_efficiency_at_TPH(pointers.m_weather.ms_outputs.m_tdry, 1., 30., &wcond) / params.eta_pb_des;
            double eta_calc = params.eta_pb_des * eta_corr;
            double eta_diff = 1.;
            int i = 0;
            while (eta_diff > 0.001 && i < 20)
            {
                double q_pc_est = params.w_lim.at(m_current_read_step) * 1.e-3 / eta_calc;			// Estimated power cycle thermal input at w_lim
                double eta_new = pointers.mpc_pc->get_efficiency_at_load(q_pc_est / params.q_pb_des) * eta_corr;		// Calculated power cycle efficiency
                eta_diff = std::abs(eta_calc - eta_new);
                eta_calc = eta_new;
                i++;
            }
            disp_outputs.q_dot_pc_max = fmin(disp_outputs.q_dot_pc_max, params.w_lim.at(m_current_read_step) / eta_calc); // Restrict max pc thermal input to *approximate* current allowable value (doesn't yet account for parasitics)
            disp_outputs.q_dot_pc_max = fmax(disp_outputs.q_dot_pc_max, disp_outputs.q_pc_target);								  // calculated q_pc_target accounts for parasitics --> can be higher than approximate limit 
        }

        disp_outputs.etasf_expect = params.eta_sf_expected.at(m_current_read_step);
        disp_outputs.qsf_expect = params.q_sfavail_expected.at(m_current_read_step);
        disp_outputs.qsfprod_expect = outputs.q_sf_expected.at(m_current_read_step);
        disp_outputs.qsfsu_expect = outputs.q_rec_startup.at(m_current_read_step);
        disp_outputs.tes_expect = outputs.tes_charge_expected.at(m_current_read_step);
        disp_outputs.qpbsu_expect = outputs.q_pb_startup.at(m_current_read_step);
        disp_outputs.wpb_expect = outputs.w_pb_target.at(m_current_read_step);
        disp_outputs.rev_expect = disp_outputs.wpb_expect * params.sell_price.at(m_current_read_step);
        disp_outputs.etapb_expect = disp_outputs.wpb_expect / (std::max)(1.e-6, outputs.q_pb_target.at(m_current_read_step))
            * (outputs.pb_operation.at(m_current_read_step) ? 1. : 0.);

        if (m_current_read_step > solver_params.optimize_frequency* solver_params.steps_per_hour)
            throw C_csp_exception("Counter synchronization error in dispatch optimization routine.", "csp_dispatch");
    }
    disp_outputs.time_last = pointers.siminfo->ms_ts.m_time;

    return true;
}
