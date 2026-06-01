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

#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <algorithm>
#include "csp_dispatch_ortools.h"

using namespace operations_research;

csp_dispatch_ortools::csp_dispatch_ortools()
{
    outputs.clear();
}

void csp_dispatch_ortools::init(double cycle_q_dot_des, double cycle_eta_des, double fixed_parasitic)
{
    // Moved to the init call because solver is not being initialized when in debug mode
    // TODO: Create a switch to select the solver type based on input
    solver = std::unique_ptr<MPSolver>(MPSolver::CreateSolver("CBC"));          // Best open-source solver available
    //solver = std::unique_ptr<MPSolver>(MPSolver::CreateSolver("XPRESS"));     // Requires license
    //solver = std::unique_ptr<MPSolver>(MPSolver::CreateSolver("GUROBI"));     // Requires license
    //solver = std::unique_ptr<MPSolver>(MPSolver::CreateSolver("SCIP"));
    //solver = std::unique_ptr<MPSolver>(MPSolver::CreateSolver("GLPK"));

    // MPSolver interface does not support Highs solver. MathOpt does...
    //solver = std::unique_ptr<MPSolver>(MPSolver::CreateSolver("HIGHS"));  // Crashes... https://github.com/google/or-tools/issues/4297

    if (!solver) {
        throw(C_csp_exception("CSP dispatch solver via OR-Tools could not be created.", "CSP_dispatch_ortools"));
    }

    //TODO
    // Abnormal termination with SCIP solver
    //solver->SetNumThreads(8); // TODO: add to solver parameters?

    // This works for SCIP solver
    //solver->SetSolverSpecificParametersAsString("parallel/maxnthreads = 16\n");
    //solver->SetSolverSpecificParametersAsString("parallel/minnthreads = 2\n");

    //solver->EnableOutput();

    solver->set_time_limit(solver_params.solution_timeout * 1000); //micro-seconds
    MPsolver_params.SetDoubleParam(MPSolverParameters::DoubleParam::RELATIVE_MIP_GAP, solver_params.mip_gap);
    // Maximum number of iterations cannot be set easily in OR-Tools, I believe we can set it specific to the solver.

    params.dt = 1. / (double)solver_params.steps_per_hour;  //hr

    params.e_tes_min = pointers.tes->get_min_charge_energy();
    params.e_tes_max = pointers.tes->get_max_charge_energy();
    params.e_pb_startup_cold = pointers.mpc_pc->get_cold_startup_energy();
    params.e_pb_startup_hot = pointers.mpc_pc->get_hot_startup_energy();
    params.e_rec_startup = pointers.col_rec->get_startup_energy();
    params.dt_pb_startup_cold = pointers.mpc_pc->get_cold_startup_time();
    params.dt_pb_startup_hot = pointers.mpc_pc->get_hot_startup_time();
    params.dt_rec_startup = pointers.col_rec->get_startup_time() / 3600.;
    params.tes_degrade_rate = pointers.tes->get_degradation_rate();
    params.q_pb_standby = pointers.mpc_pc->get_standby_energy_requirement();
    params.q_pb_des = cycle_q_dot_des;
    params.eta_pb_des = cycle_eta_des;
    params.q_pb_max = pointers.mpc_pc->get_max_thermal_power();
    params.q_pb_min = std::min(pointers.mpc_pc->get_min_thermal_power() * 1.5, pointers.mpc_pc->get_max_thermal_power());   // Add a buffer to prevent controller from failing to operate in net power mode
    params.q_rec_min = pointers.col_rec->get_min_power_delivery();
    params.w_rec_pump = pointers.col_rec->get_pumping_parasitic_coef();

    params.w_track = pointers.col_rec->get_tracking_power();
    params.w_stow = pointers.col_rec->get_col_startup_power();
    params.w_cycle_pump = pointers.mpc_pc->get_htf_pumping_parasitic_coef();
    params.w_cycle_standby = params.q_pb_standby * params.w_cycle_pump;

    // Heater parameters
    if (pointers.par_htr != NULL) {
        params.is_parallel_heater = true;
        params.q_eh_min = pointers.par_htr->get_min_power_delivery() * ( 1 + 1e-8 ); // ensures controller doesn't shut down heater at minimum load
        params.q_eh_max = pointers.par_htr->get_max_power_delivery(std::numeric_limits<double>::quiet_NaN());
        params.e_eh_su = pointers.par_htr->get_startup_energy(); // [MWht]
        params.dt_eh_startup = pointers.par_htr->get_startup_time();
        params.eta_eh = pointers.par_htr->get_design_electric_to_heat_cop();
        // TODO: Add parameters for heater startup power??
    }
    else {
        params.is_parallel_heater = false;
    }

    //Battery storage parameters
    if (pointers.battery != NULL) {
        params.is_battery_included = true;
        double min_soc = pointers.battery->battery->SOC_min();
        double max_soc = pointers.battery->battery->SOC_max();
        params.batt_soc_min = min_soc / 100.0;      // TODO: We could make these percentages within the dispatch model
        params.batt_soc_max = max_soc / 100.0;
        params.batt_capacity = pointers.battery->battery->energy_max(100.0, 0.0) / 1.e3; // [kWh] -> [MWh]

        battery_state state = pointers.battery->battery->get_state();
        battery_params batt_params = pointers.battery->battery->get_params();
        double crate = batt_params.voltage->dynamic.C_rate;

        double battery_max = pointers.battery->battery->nominal_energy() * crate;
        params.batt_charge_power_max = battery_max / 1.e3;
        params.batt_discharge_power_max = battery_max / 1.e3;

        // TODO: These are not going to work if the initial SOC near the bounds
        //params.batt_charge_power_max = std::abs(pointers.battery->battery->calculate_max_charge_kw() / 1.e3);   // kW -> MW
        //params.batt_discharge_power_max = pointers.battery->battery->calculate_max_discharge_kw() / 1.e3; // kW -> MW

        // TODO: update the parameters below.
        params.batt_charge_efficiency = 1.0;        //0.99; // 0.938;
        params.batt_discharge_efficiency = 0.978919;//0.99; // 0.938;
        params.batt_charge_cost = 0.9;
        params.batt_discharge_cost = 0.9;
        params.batt_lifecycle_cost = 26.5;
    }
    else {
        params.is_battery_included = false;
    }

    double w_pb_des = cycle_q_dot_des * params.eta_pb_des;
    params.sys_par_fixed = fixed_parasitic;

    params.eff_table_load.init_linear_cycle_efficiency_table(params.q_pb_min, params.q_pb_des, params.eta_pb_des, pointers.mpc_pc);
    params.eff_table_Tdb.init_efficiency_ambient_temp_table(params.eta_pb_des, w_pb_des, pointers.mpc_pc, &params.wcondcoef_table_Tdb);

    // Time-series parameters -> will update during rolling horizon
    ts_params.clear();
    ts_params.resize(solver_params.optimize_horizon * solver_params.steps_per_hour);

    // Initial conditions -> will update during rolling horizon
    init_conditions.e_tes0 = pointers.tes->get_initial_charge_energy();
    init_conditions.q_pb0 = 0.0;
    init_conditions.wdot_pb0 = 0.0;

    if (params.is_battery_included) {
        init_conditions.batt_soc0 = pointers.battery->battery->SOC() / 100.0;
    }

    // All constant parameter values must be set before building the model
    build_dispatch_model(); // Built once at the beginning of the simulation
}

void csp_dispatch_ortools::update_objective_function(unordered_map<std::string, double>& P) {
    //calculate the mean price to appropriately weight the receiver production timing derate
    double pmean = 0;
    for (int t = 0; t < (int)ts_params.sell_price.size(); t++)
        pmean += ts_params.sell_price.at(t);
    pmean /= (double)ts_params.sell_price.size();

    // Modify objective function - Maximize revenue from power sales, minimize startup costs and penalties
    MPObjective* objective = solver->MutableObjective();                   // OR-Tools objective function
    double tadj = P["disp_time_weighting"];
    for (int t = 0; t < m_nstep_opt; t++) {
        // TODO: We could provide various objective functions for the user to select from
        objective->SetCoefficient(cont_vars.wdot[t], P["delta"] * ts_params.sell_price.at(t) * tadj * (1. - ts_params.w_condf_expected.at(t)));
        objective->SetCoefficient(cont_vars.xr[t], -P["delta"] * ts_params.sell_price.at(t) * (1. / tadj) * P["Lr"]);      // tadj added to prefer receiver production sooner (i.e. delay dumping)
        objective->SetCoefficient(cont_vars.xrsu[t], -P["delta"] * ts_params.sell_price.at(t) * (1. / tadj) * P["Lr"]);
        objective->SetCoefficient(bin_vars.yrsu[t], -ts_params.sell_price.at(t) * (1. / tadj) * (P["Wrsb"] + P["Ehs"]));
        objective->SetCoefficient(bin_vars.yr[t], -P["delta"] * ts_params.sell_price.at(t) * (1. / tadj) * P["Wh"]);      // tadj added to prefer receiver operation in nearer term to longer term
        objective->SetCoefficient(cont_vars.x[t], -P["delta"] * ts_params.sell_price.at(t) * (1. / tadj) * P["Lc"]);
        objective->SetCoefficient(cont_vars.delta_w[t], -P["pen_delta_w"] * (1. / tadj));
        objective->SetCoefficient(bin_vars.yrsup[t], -P["rsu_cost"] * (1. / tadj));
        objective->SetCoefficient(bin_vars.ycsup[t], -P["csu_cost"] * (1. / tadj));

        if (params.can_cycle_use_standby) {
            objective->SetCoefficient(bin_vars.ycsb[t], -P["delta"] * ts_params.sell_price.at(t) * (1. / tadj) * P["Wb"]);
            objective->SetCoefficient(bin_vars.ychsp[t], -P["csu_cost"] * (1. / tadj) * 0.1);
        }

        if (params.is_parallel_heater) {
            objective->SetCoefficient(cont_vars.qeh[t], -(P["delta"] * ts_params.sell_price.at(t) * (1. / tadj) * (1 / P["eta_eh"])));
            // Assumes heater startup energy happens instantaneously at beginning of time step (we would need to reformulate if we wanted to model startup over multiple time steps)
            objective->SetCoefficient(bin_vars.yhsup[t], -(ts_params.sell_price.at(t) * params.e_eh_su * (1. / tadj) * (1 / P["eta_eh"])));
            objective->SetCoefficient(bin_vars.yhsup[t], -(1. / tadj) * P["hsu_cost"]);
        }

        if (params.is_battery_included) {
            objective->SetCoefficient(cont_vars.pdis[t], P["delta"] * (ts_params.sell_price.at(t) * tadj - (1. / tadj) * P["discharge_cost"]));
            objective->SetCoefficient(cont_vars.pchar[t], -P["delta"] * (ts_params.sell_price.at(t) * tadj + (1. / tadj) * P["charge_cost"]));
        }

        if (params.is_pv_included) {
            objective->SetCoefficient(cont_vars.w_pv[t], P["delta"] * ts_params.sell_price.at(t) * tadj);
        }

        tadj *= P["disp_time_weighting"];
    }
    // Add the final term to value storage at end of optimization horizon
    objective->SetCoefficient(cont_vars.s[m_nstep_opt - 1], P["delta"] * tadj * pmean * P["eta_cycle"] * params.inventory_incentive);

    // Battery cycle cost
    if (params.is_battery_included) objective->SetCoefficient(cont_vars.batt_cycle_count, -P["batt_lifecycle_cost"]);

    // Set to a maximization problem
    objective->SetMaximization();   
}

void csp_dispatch_ortools::build_dispatch_model()
{
    m_nstep_opt = (int)(solver_params.optimize_horizon * solver_params.steps_per_hour);
    unordered_map<std::string, double> P;
    params.create_parameter_map(P, m_nstep_opt);

    // Continuous variables
    solver->MakeNumVarArray(m_nstep_opt, 0.0, MPSolver::infinity(), "x_r",      &cont_vars.xr);
    solver->MakeNumVarArray(m_nstep_opt, 0.0, MPSolver::infinity(), "x_rsu",    &cont_vars.xrsu);
    solver->MakeNumVarArray(m_nstep_opt, 0.0, P["Er"] * 1.0001,     "u_rsu",    &cont_vars.ursu);
    solver->MakeNumVarArray(m_nstep_opt, 0.0, P["Qu"],              "x",        &cont_vars.x);
    solver->MakeNumVarArray(m_nstep_opt, 0.0, P["Ec"] * 1.0001,     "u_csu",    &cont_vars.ucsu);
    solver->MakeNumVarArray(m_nstep_opt, 0.0, P["Wdotu"] * 1.1,     "wdot",     &cont_vars.wdot);
    solver->MakeNumVarArray(m_nstep_opt, 0.0, P["Wdotu"] * 1.1,     "delta_w",  &cont_vars.delta_w);
    solver->MakeNumVarArray(m_nstep_opt, 0.0, P["Eu"],              "s",        &cont_vars.s);

    // Binary variables
    solver->MakeBoolVarArray(m_nstep_opt, "y_r",    &bin_vars.yr);
    solver->MakeBoolVarArray(m_nstep_opt, "y_rsu",  &bin_vars.yrsu);
    solver->MakeBoolVarArray(m_nstep_opt, "y_rsup", &bin_vars.yrsup);
    solver->MakeBoolVarArray(m_nstep_opt, "y_rhsp", &bin_vars.yrhsp);
    solver->MakeBoolVarArray(m_nstep_opt, "y",      &bin_vars.y);
    solver->MakeBoolVarArray(m_nstep_opt, "y_csu",  &bin_vars.ycsu);
    solver->MakeBoolVarArray(m_nstep_opt, "y_csup", &bin_vars.ycsup);

    // Cycle standby variables
    if (params.can_cycle_use_standby) {
        solver->MakeBoolVarArray(m_nstep_opt, "y_csb",  &bin_vars.ycsb);
        solver->MakeBoolVarArray(m_nstep_opt, "y_chsp", &bin_vars.ychsp);
        solver->MakeBoolVarArray(m_nstep_opt, "y_off",  &bin_vars.yoff);
    }

    // Parallel heater variables
    if (params.is_parallel_heater) {
        solver->MakeNumVarArray(m_nstep_opt, 0.0, P["Qehu"], "q_eh", &cont_vars.qeh);
        solver->MakeBoolVarArray(m_nstep_opt, "y_eh",   &bin_vars.yeh);
        solver->MakeBoolVarArray(m_nstep_opt, "y_reh",  &bin_vars.yreh);
        solver->MakeBoolVarArray(m_nstep_opt, "y_hsup", &bin_vars.yhsup);
    }

    if (params.is_battery_included) {
        solver->MakeNumVarArray(m_nstep_opt, 0.0, params.batt_charge_power_max, "p_char", &cont_vars.pchar);
        solver->MakeNumVarArray(m_nstep_opt, 0.0, params.batt_discharge_power_max, "p_dis", &cont_vars.pdis);
        solver->MakeNumVarArray(m_nstep_opt, params.batt_soc_min, params.batt_soc_max, "soc", &cont_vars.soc);
        solver->MakeBoolVarArray(m_nstep_opt, "y_bchar", &bin_vars.ybchar);
        solver->MakeBoolVarArray(m_nstep_opt, "y_bdis", &bin_vars.ybdis);
        cont_vars.batt_cycle_count = solver->MakeNumVar(0.0, MPSolver::infinity(), "batt_cycle_count");
    }

    if (params.is_pv_included) {
        solver->MakeNumVarArray(m_nstep_opt, 0.0, P["w_pv_max"], "w_pv", &cont_vars.w_pv);
    }

    // Objective function
    update_objective_function(P);

    // Create constraints
    constraints.resize(m_nstep_opt);
    MPConstraint* c;
    // ******************** Receiver constraints *******************
    // Receiver startup energy inventory
    // ursu[t] <= ursu[t-1] + Delta * xrsu[t]
    for (int t = 0; t < m_nstep_opt; ++t) {
        std::string name = "receiver_startup_inventory_" + std::to_string(t);
        c = solver->MakeRowConstraint(name);
        c->SetCoefficient(cont_vars.ursu[t], 1.0);
        c->SetCoefficient(cont_vars.xrsu[t], -P["delta"]);
        if (t > 0) {
            c->SetCoefficient(cont_vars.ursu[t - 1], -1.0);
            c->SetUB(0.0);
        }
        else {
            c->SetUB(init_conditions.rec_startup_energy0);
            constraints.receiver_startup_inventory0 = c;
        }
    }

    // Receiver inventory bound when starting
    // ursu[t] <= Er * yrsu[t]
    for (int t = 0; t < m_nstep_opt; ++t) {
        std::string name = "receiver_startup_nonzero_" + std::to_string(t);
        c = solver->MakeRowConstraint(name);
        c->SetCoefficient(cont_vars.ursu[t], 1.0);
        c->SetCoefficient(bin_vars.yrsu[t], -P["Er"]);
        c->SetUB(0.0);
    }

    // Receiver operation allowed when start-up is complete or if receiver was operating
    // NOTE: tighter formulation when Er is distributed
    // yr[t] <= ursu[t] / Er + yr[t-1]
    for (int t = 0; t < m_nstep_opt; ++t) {
        std::string name = "receiver_operation_allowed_" + std::to_string(t);
        c = solver->MakeRowConstraint(name);
        c->SetCoefficient(bin_vars.yr[t], P["Er"]);
        c->SetCoefficient(cont_vars.ursu[t], -1.0);
        if (t > 0) {
            c->SetCoefficient(bin_vars.yr[t - 1], -P["Er"]);
            c->SetUB(0.0);
        }
        else {
            c->SetUB((init_conditions.is_rec_operating0 ? P["Er"] : 0.));
            constraints.receiver_operation_allowed0 = c;
        }
    }

    // Receiver startup can't be enabled after a time step where the Receiver was operating
    // yrsu[t] + yr[t-1] <= 1
    for (int t = 0; t < m_nstep_opt; ++t) {
        std::string name = "receiver_startup_wait_" + std::to_string(t);
        c = solver->MakeRowConstraint(-MPSolver::infinity(), 1.0, name);
        c->SetCoefficient(bin_vars.yrsu[t], 1.0);
        if (t > 0) {
            c->SetCoefficient(bin_vars.yr[t - 1], 1.0);
            c->SetUB(1.0);
        }
        else {
            c->SetUB((init_conditions.is_rec_operating0 ? 0. : 1.));
            constraints.receiver_startup_wait0 = c;
        }
    }

    // Receiver startup energy limit
    // xrsu[t] <= Qru * yrsu[t]
    for (int t = 0; t < m_nstep_opt; ++t) {
        std::string name = "receiver_startup_limit_" + std::to_string(t);
        c = solver->MakeRowConstraint(name);
        c->SetCoefficient(cont_vars.xrsu[t], 1.0);
        c->SetCoefficient(bin_vars.yrsu[t], -P["Qru"]);
        c->SetUB(0.0);
    }

    // Enforces receiver startup time requirement
    // Aux. Receiver startup energy limit -> forces startup time requirement
    // xrsu[t] >= Delta^su * Qin[t] * (yrsu[t] - yrsu[t-1])
    for (int t = 0; t < m_nstep_opt; ++t) {
        std::string name = "receiver_startup_time_req_" + std::to_string(t);
        c = solver->MakeRowConstraint(name);
        c->SetCoefficient(cont_vars.xrsu[t], 1.0);
        c->SetCoefficient(bin_vars.yrsu[t], - params.dt_rec_startup * ts_params.q_sfavail_expected.at(t));
        if (t > 0) {
            c->SetCoefficient(bin_vars.yrsu[t - 1], params.dt_rec_startup * ts_params.q_sfavail_expected.at(t));
            c->SetLB(0.0);
        }
        else
            c->SetLB(-(init_conditions.is_rec_starting0 ? 1. : 0.) * params.dt_rec_startup * ts_params.q_sfavail_expected.at(t));
        constraints.receiver_startup_time_req[t] = c;
    }

    // Removes receiver start-ups requiring more than 2 time periods
    // yrsu[t-1] + yrsu[t] + yrsu[t+1] <= 2 (only enforce hourly for now)
    // NOTE: last time step not enforced
    if (P["delta"] >= 1.) {
        for (int t = 0; t < m_nstep_opt - 1; ++t) {
            std::string name = "rec_SU_duration_limit_" + std::to_string(t);
            c = solver->MakeRowConstraint(name);
            c->SetCoefficient(bin_vars.yrsu[t], 1.0);
            c->SetCoefficient(bin_vars.yrsu[t + 1], 1.0);
            if (t > 0) {
                c->SetCoefficient(bin_vars.yrsu[t - 1], 1.0);
                c->SetUB(2.0);
            }
            else {
                c->SetUB(2.0 - (init_conditions.is_rec_starting0 ? 1. : 0.));
                constraints.receiver_SU_duration_limit0 = c;
            }
        }
    }

    // Receiver startup and operation consumption limit
    // xr[t] + xrsu[t] <= Qin[t]
    for (int t = 0; t < m_nstep_opt; ++t) {
        std::string name = "receiver_power_limit_" + std::to_string(t);
        c = solver->MakeRowConstraint(name);
        c->SetCoefficient(cont_vars.xr[t], 1.0);
        c->SetCoefficient(cont_vars.xrsu[t], 1.0);
        c->SetUB(ts_params.q_sfavail_expected.at(t));
        constraints.receiver_power_limit[t] = c;
    }

    // Receiver maximum operation limit
    // xr[t] <= Qin[t] * yr[t]
    for (int t = 0; t < m_nstep_opt; ++t) {
        std::string name = "receiver_production_limit_" + std::to_string(t);
        c = solver->MakeRowConstraint(name);
        c->SetCoefficient(cont_vars.xr[t], 1.0);
        c->SetCoefficient(bin_vars.yr[t], -ts_params.q_sfavail_expected.at(t));
        c->SetUB(0.0);
        constraints.receiver_production_limit[t] = c;
    }

    // Receiver minimum operation limit (startup + operation)
    // xr[t] + xrsu[t] >= Qrl * yr[t]
    for (int t = 0; t < m_nstep_opt; ++t) {
        std::string name = "receiver_minimum_limit_" + std::to_string(t);
        c = solver->MakeRowConstraint(name);
        c->SetCoefficient(cont_vars.xr[t], 1.0);
        c->SetCoefficient(cont_vars.xrsu[t], 1.0);
        c->SetCoefficient(bin_vars.yr[t], -P["Qrl"]);
        c->SetLB(0.0);
    }

    // Receiver startup only during solar positive periods
    // yrsu[t] <= 0 when Qin[t] = 0
    for (int t = 0; t < m_nstep_opt; ++t) {
        std::string name = "receiver_startup_cut_" + std::to_string(t);
        c = solver->MakeRowConstraint( name);
        c->SetCoefficient(bin_vars.yrsu[t], 1.0);
        c->SetUB((std::min)(P["Qru"] * ts_params.q_sfavail_expected.at(t), 1.0));
        constraints.receiver_startup_cut[t] = c;
    }

    // Receiver can't continue operating when no energy is available
    // yr[t] <= Qin[t] / Qrl
    for (int t = 0; t < m_nstep_opt; ++t) {
        std::string name = "receiver_operation_limit_cut_" + std::to_string(t);
        c = solver->MakeRowConstraint(name);
        c->SetCoefficient(bin_vars.yr[t], 1.0);
        c->SetUB((std::min)(floor(ts_params.q_sfavail_expected.at(t) / P["Qrl"]), 1.0));
        constraints.receiver_operation_limit_cut[t] = c;
    }

    // Receiver startup penalty
    // yrsup[t] >= yrsu[t] - yrsu[t-1]
    for (int t = 0; t < m_nstep_opt; ++t) {
        std::string name = "receiver_startup_penalty_" + std::to_string(t);
        c = solver->MakeRowConstraint(name);
        c->SetCoefficient(bin_vars.yrsup[t], 1.0);
        c->SetCoefficient(bin_vars.yrsu[t], -1.0);
        if (t > 0) {
            c->SetCoefficient(bin_vars.yrsu[t - 1], 1.0);
            c->SetLB(0.0);
        }
        else {
            c->SetLB((init_conditions.is_rec_starting0 ? -1. : 0.));
            constraints.receiver_startup_penalty0 = c;
        }
    }

    // ******************** Electric Heater constraints ****************
    if (params.is_parallel_heater) {
        // Heater power limit
        // qeh[t] <= Qehu * yeh[t]
        for (int t = 0; t < m_nstep_opt; ++t) {
            std::string name = "heater_power_limit_" + std::to_string(t);
            c = solver->MakeRowConstraint(name);
            c->SetCoefficient(cont_vars.qeh[t], 1.0);
            c->SetCoefficient(bin_vars.yeh[t], -P["Qehu"]);
            c->SetUB(0.0);
        }

        // Heater minimum operation requirement
        // qeh[t] >= Qehl * yeh[t]
        for (int t = 0; t < m_nstep_opt; ++t) {
            std::string name = "heater_minimum_limit_" + std::to_string(t);
            c = solver->MakeRowConstraint(name);
            c->SetCoefficient(cont_vars.qeh[t], 1.0);
            c->SetCoefficient(bin_vars.yeh[t], -P["Qehl"]);
            c->SetLB(0.0);
        }

        // Heaters and cycle cannot coincide
        // yeh[t] + y[t] <= 1
        for (int t = 0; t < m_nstep_opt; ++t) {
            std::string name = "heater_cycle_coincide_" + std::to_string(t);
            c = solver->MakeRowConstraint(name);
            c->SetCoefficient(bin_vars.yeh[t], 1.0);
            c->SetCoefficient(bin_vars.y[t], 1.0);
            c->SetUB(1.0);
        }

        // Heaters must be off before field defocus
        // xr[t] + xrsu[t] >= Qin[t] * yreh[t]
        for (int t = 0; t < m_nstep_opt; ++t) {
            std::string name = "heater_off_before_defocus_" + std::to_string(t);
            c = solver->MakeRowConstraint(name);
            c->SetCoefficient(cont_vars.xr[t], 1.0);
            c->SetCoefficient(cont_vars.xrsu[t], 1.0);
            c->SetCoefficient(bin_vars.yreh[t], -ts_params.q_sfavail_expected.at(t));
            c->SetLB(0.0);
            constraints.heater_off_before_defocus[t] = c;
        }

        //******* linearization of yreh[t] = yr[t] * yeh[t] ******

        // Upper bound with yr
        // yreh[t] <= yr[t]
        for (int t = 0; t < m_nstep_opt; ++t) {
            std::string name = "heater_receiver_binary_ub_yr_" + std::to_string(t);
            c = solver->MakeRowConstraint(name);
            c->SetCoefficient(bin_vars.yreh[t], 1.0);
            c->SetCoefficient(bin_vars.yr[t], -1.0);
            c->SetUB(0.0);
        }

        // Upper bound with yeh
        // yreh[t] <= yeh[t]
        for (int t = 0; t < m_nstep_opt; ++t) {
            std::string name = "heater_receiver_binary_ub_yeh_" + std::to_string(t);
            c = solver->MakeRowConstraint(name);
            c->SetCoefficient(bin_vars.yreh[t], 1.0);
            c->SetCoefficient(bin_vars.yeh[t], -1.0);
            c->SetUB(0.0);
        }

        // Lower bound
        // yreh[t] >= yr[t] + yeh[t] - 1
        for (int t = 0; t < m_nstep_opt; ++t) {
            std::string name = "heater_receiver_binary_lb_" + std::to_string(t);
            c = solver->MakeRowConstraint(name);
            c->SetCoefficient(bin_vars.yreh[t], 1.0);
            c->SetCoefficient(bin_vars.yr[t], -1.0);
            c->SetCoefficient(bin_vars.yeh[t], -1.0);
            c->SetLB(-1.0);
        }

        // Heater startup penalty
        // yhsup[t] >= yeh[t] - yeh[t-1]
        for (int t = 0; t < m_nstep_opt; ++t) {
            std::string name = "heater_startup_penalty_" + std::to_string(t);
            c = solver->MakeRowConstraint(name);
            c->SetCoefficient(bin_vars.yhsup[t], 1.0);
            c->SetCoefficient(bin_vars.yeh[t], -1.0);
            if (t > 0) {
                c->SetCoefficient(bin_vars.yeh[t - 1], 1.0);
                c->SetLB(0.0);
            }
            else {
                c->SetLB((init_conditions.is_heater_operating0 ? -1. : 0.));
                constraints.heater_startup_penalty0 = c;
            }
        }
    }

    // ******************** Power cycle constraints *******************

    // Startup inventory balance
    // ucsu[t] <= ucsu[t-1] + delta * Qc * ycsu[t]
    for (int t = 0; t < m_nstep_opt; ++t) {
        std::string name = "cycle_startup_inventory_" + std::to_string(t);
        c = solver->MakeRowConstraint(name);
        c->SetCoefficient(cont_vars.ucsu[t], 1.0);
        c->SetCoefficient(bin_vars.ycsu[t], -P["delta"] * P["Qc"]);
        if (t > 0) {
            c->SetCoefficient(cont_vars.ucsu[t - 1], -1.0);
            c->SetUB(0.0);
        }
        else {
            c->SetUB(init_conditions.pb_startup_energy0);
            constraints.cycle_startup_inventory0 = c;
        }
    }

    // Inventory nonzero
    // ucsu[t] <= Ec * ycsu[t]
    for (int t = 0; t < m_nstep_opt; ++t) {
        std::string name = "cycle_startup_nonzero_" + std::to_string(t);
        c = solver->MakeRowConstraint(name);
        c->SetCoefficient(cont_vars.ucsu[t], 1.0);
        c->SetCoefficient(bin_vars.ycsu[t], -P["Ec"] * 1.00001);  // tighter formulation
        c->SetUB(0.0);
    }

    // Cycle operation allowed when startup is complete or already operating or in standby
    // hourly:    y[t] <=  ucsu[t  ] / Ec + y[t-1] + ycsb[t-1]
    // subhourly: y[t] <=  ucsu[t-1] / Ec + y[t-1] + ycsb[t-1]   (startup and production cannot coincide)
    // NOTE: tighter formulation when Ec is distributed
    for (int t = 0; t < m_nstep_opt; ++t) {
        std::string name = "cycle_operation_allowed_" + std::to_string(t);
        c = solver->MakeRowConstraint(name);
        c->SetCoefficient(bin_vars.y[t], P["Ec"]);

        // for hourly model (delta = 1)
        if (P["delta"] >= 1.) c->SetCoefficient(cont_vars.ucsu[t], -1.0);

        if (t > 0) {
            c->SetCoefficient(bin_vars.y[t - 1], -P["Ec"]);
            if (params.can_cycle_use_standby) c->SetCoefficient(bin_vars.ycsb[t - 1], -P["Ec"]);
            if (P["delta"] < 1.) c->SetCoefficient(cont_vars.ucsu[t - 1], -1.0);        // For sub-hourly model
            c->SetUB(0.0);
        }
        else {
            double rhs = (init_conditions.is_pb_operating0 ? P["Ec"] : 0);
            if (params.can_cycle_use_standby) rhs += (init_conditions.is_pb_standby0 ? P["Ec"] : 0);
            if (P["delta"] < 1.) rhs += init_conditions.pb_startup_energy0;             // For sub-hourly model
            c->SetUB(rhs);
            constraints.cycle_operation_allowed0 = c;
        }
    }

    // Cycle consumption limit (valid only for hourly model -> Delta == 1)
    // x[t] + Qc * ycsu[t] <= Qu * y[t]
    if (P["delta"] >= 1.0) {
        for (int t = 0; t < m_nstep_opt; ++t) {
            std::string name = "cycle_consumption_limit_" + std::to_string(t);
            c = solver->MakeRowConstraint(name);
            c->SetCoefficient(cont_vars.x[t], 1.0);
            c->SetCoefficient(bin_vars.ycsu[t], P["Qc"]);
            c->SetCoefficient(bin_vars.y[t], -P["Qu"]);
            c->SetUB(0.0);
        }
    }

    // Cycle maximum operation limit
    // x[t] <= Qu * y[t]
    for (int t = 0; t < m_nstep_opt; ++t) {
        std::string name = "cycle_maximum_limit_" + std::to_string(t);
        c = solver->MakeRowConstraint(name);
        c->SetCoefficient(cont_vars.x[t], 1.0);
        c->SetCoefficient(bin_vars.y[t], -P["Qu"]);
        c->SetUB(0.0);
    }

    // Cycle minimum operation limit
    // x[t] >= Ql * y[t]
    for (int t = 0; t < m_nstep_opt; ++t) {
        std::string name = "cycle_minimum_limit_" + std::to_string(t);
        c = solver->MakeRowConstraint(name);
        c->SetCoefficient(cont_vars.x[t], 1.0);
        c->SetCoefficient(bin_vars.y[t], -P["Ql"]);
        c->SetLB(0.0);
    }

    // Power production linearization (power as function of heat input)
    // wdot[t] = eta_amb[t]/eta_des * ( etap * x[t] + ( Wdotu - etap * Qu ) * y[t] )
    for (int t = 0; t < m_nstep_opt; ++t) {
        std::string name = "cycle_production_linearization_" + std::to_string(t);
        c = solver->MakeRowConstraint(name);
        c->SetCoefficient(cont_vars.wdot[t], 1.0);
        c->SetCoefficient(cont_vars.x[t], -P["etap"] * ts_params.eta_pb_expected.at(t) / P["eta_cycle"]);
        c->SetCoefficient(bin_vars.y[t], -(P["Wdotu"] - P["etap"] * P["Qu"]) * ts_params.eta_pb_expected.at(t) / P["eta_cycle"]);
        c->SetBounds(0.0, 0.0);
        constraints.cycle_production_linearization[t] = c;
    }

    // Cycle positive production change (i.e., ramping) We might want to penalize thermal ramping instead to remove the ambient correction
    // delta_w[t] >= wdot[t] - wdot[t-1]
    for (int t = 0; t < m_nstep_opt; ++t) {
        std::string name = "cycle_ramp_up_" + std::to_string(t);
        c = solver->MakeRowConstraint(name);
        c->SetCoefficient(cont_vars.delta_w[t], 1.0);
        c->SetCoefficient(cont_vars.wdot[t], -1.0);
        double rhs = 0.0;
        if (t > 0) {
            c->SetCoefficient(cont_vars.wdot[t - 1], 1.0);
        }
        else {
            rhs += -init_conditions.wdot_pb0;
            constraints.cycle_ramp_up0 = c;
        }
        c->SetLB(rhs);
    }

    // Cycle sub-hourly (Delta < 1) ramping limit that doesn't hold when starting up, (or going into standby), (or shutdown)
    // delta_t[t] <= Wdlim + temp_coef * ( y[t] - y[t-1] + 2 * ycsb[t] + 2 * yoff[t] )
    if (P["delta"] < 1.) {
        for (int t = 0; t < m_nstep_opt; ++t) {
            std::string name = "cycle_sub_hourly_ramp_up_" + std::to_string(t);
            c = solver->MakeRowConstraint(name);

            double temp_coef = (ts_params.eta_pb_expected.at(t) / P["eta_cycle"]) * P["Wdotl"] - P["Wdlim"];
            double rhs = P["Wdlim"];

            c->SetCoefficient(cont_vars.delta_w[t], 1.0);
            c->SetCoefficient(bin_vars.y[t], -temp_coef);
            if (t > 0) {
                c->SetCoefficient(bin_vars.y[t - 1], temp_coef);
            }
            else {
                rhs += (init_conditions.is_pb_operating0 ? -temp_coef : 0);
            }

            if (params.can_cycle_use_standby) {
                c->SetCoefficient(bin_vars.ycsb[t], -temp_coef * 2.);
                c->SetCoefficient(bin_vars.yoff[t], -temp_coef * 2.);
            }

#ifdef MOD_CYCLE_SHUTDOWN
            c->SetCoefficient(bin_vars.ycsd[t], -temp_coef * 2.);
#endif
            c->SetUB(rhs);
            constraints.cycle_sub_hourly_ramp_up[t] = c;
        }
    }

    // Cycle startup and operation cannot coincide (valid for sub-hourly model Delta < 1)
    // ycsu[t] + y[t] <= 1
    if (P["delta"] < 1) {
        for (int t = 0; t < m_nstep_opt; ++t) {
            std::string name = "cycle_startup_operation_coincide_" + std::to_string(t);
            c = solver->MakeRowConstraint(name);
            c->SetCoefficient(bin_vars.ycsu[t], 1.0);
            c->SetCoefficient(bin_vars.y[t], 1.0);
            c->SetUB(1.0);
        }
    }

    // Cycle startup can't be enabled after a time step where the cycle was operating
    // ycsu[t] + y[t-1] <= 1
    for (int t = 0; t < m_nstep_opt; ++t) {
        std::string name = "cycle_startup_wait_" + std::to_string(t);
        c = solver->MakeRowConstraint(name);
        c->SetCoefficient(bin_vars.ycsu[t], 1.0);
        double rhs = 1.0;
        if (t > 0) {
            c->SetCoefficient(bin_vars.y[t - 1], 1.0);
        }
        else {
            rhs += (init_conditions.is_pb_operating0 ? -1 : 0);
            constraints.cycle_startup_wait0 = c;
        }
        c->SetUB(rhs);
    }

    // Cycle start penalty
    // ycsup[t] >= ycsu[t] - ycsu[t-1]
    for (int t = 0; t < m_nstep_opt; ++t) {
        std::string name = "cycle_startup_penalty_" + std::to_string(t);
        c = solver->MakeRowConstraint(name);
        c->SetCoefficient(bin_vars.ycsup[t], 1.0);
        c->SetCoefficient(bin_vars.ycsu[t], -1.0);
        double rhs = 0.0;
        if (t > 0) {
            c->SetCoefficient(bin_vars.ycsu[t - 1], 1.0);
        }
        else {
            rhs += (init_conditions.is_pb_starting0 ? -1. : 0.);
            constraints.cycle_startup_penalty0 = c;
        }
        c->SetLB(rhs);
    }

    // Maximum gross electricity production constraint
    // wdot[t] <= f_limit * W_dot_cycle
    for (int t = 0; t < m_nstep_opt; ++t) {
        std::string name = "cycle_maximum_production_" + std::to_string(t);
        c = solver->MakeRowConstraint(name);
        c->SetCoefficient(cont_vars.wdot[t], 1.0);
        c->SetUB(ts_params.f_pb_op_limit.at(t) * P["W_dot_cycle"]);
        constraints.cycle_maximum_production[t] = c;
    }

    // Maximum net electricity production constraint -> w_lim is not set at this point...
    for (int t = 0; t < m_nstep_opt; ++t) {     
        std::string name = "cycle_maximum_net_power_" + std::to_string(t);
        constraints.cycle_maximum_net_power[t] = solver->MakeRowConstraint(name);
    }

    // Cycle standby mode entry -> Not Tested
    if (params.can_cycle_use_standby) {
        // Cycle standby mode persistence
        // ycsb[t] <= y[t-1] + ycsb[t-1]
        for (int t = 0; t < m_nstep_opt; ++t) {
            std::string name = "cycle_standby_persistence_" + std::to_string(t);
            c = solver->MakeRowConstraint(name);
            c->SetCoefficient(bin_vars.ycsb[t], 1.0);

            double rhs = 0.0;
            if (t > 0) {
                c->SetCoefficient(bin_vars.y[t - 1], -1.0);
                c->SetCoefficient(bin_vars.ycsb[t - 1], -1.0);
            }
            else {
                rhs += (init_conditions.is_pb_operating0 ? 1 : 0) + (init_conditions.is_pb_standby0 ? 1 : 0);
                constraints.cycle_standby_persistence0 = c;
            }
            c->SetUB(rhs);
        }

        // Cycle operation and standby cannot coincide
        // y[t] + ycsb[t] <= 1
        for (int t = 0; t < m_nstep_opt; ++t) {
            std::string name = "cycle_operation_standby_coincide_" + std::to_string(t);
            c = solver->MakeRowConstraint(name);
            c->SetCoefficient(bin_vars.y[t], 1.0);
            c->SetCoefficient(bin_vars.ycsb[t], 1.0);
            c->SetUB(1.0);
        }

        // Cycle start-up and standby can't coincide (hourly model only)
        // ycsu[t] + ycsb[t] <= 1
        if (P["delta"] >= 1.) {
            for (int t = 0; t < m_nstep_opt; ++t) {
                std::string name = "cycle_startup_standby_coincide_" + std::to_string(t);
                c = solver->MakeRowConstraint(name);
                c->SetCoefficient(bin_vars.ycsu[t], 1.0);
                c->SetCoefficient(bin_vars.ycsb[t], 1.0);
                c->SetUB(1.0);
            }
        }

        // Set partitioning constraint (with cycle off state)
        // hourly:               y[t] + ycsb[t] + yoff[t] = 1
        // sub-hourly: ycsu[t] + y[t] + ycsb[t] + yoff[t] = 1
        for (int t = 0; t < m_nstep_opt; ++t) {
            std::string name = "cycle_set_partitioning_" + std::to_string(t);
            c = solver->MakeRowConstraint(name);
            if (P["delta"] < 1) c->SetCoefficient(bin_vars.ycsu[t], 1.0); // sub-hourly model
            c->SetCoefficient(bin_vars.y[t], 1.0);
            c->SetCoefficient(bin_vars.ycsb[t], 1.0);
            c->SetCoefficient(bin_vars.yoff[t], 1.0);
            c->SetBounds(1.0, 1.0);
        }

        // Cycle standby cut
        // ycsb[t] + x[t] / Qu <= 1
        for (int t = 0; t < m_nstep_opt; ++t) {
            std::string name = "cycle_standby_cut_" + std::to_string(t);
            c = solver->MakeRowConstraint(name);
            c->SetCoefficient(bin_vars.ycsb[t], 1.0);
            c->SetCoefficient(cont_vars.x[t], 1.0 / P["Qu"]);
            c->SetUB(1.0);
        }

        // Cycle standby startup penalty
        // ychsp[t] >= y[t] - (1 - ycsb[t-1])
        for (int t = 0; t < m_nstep_opt; ++t) {
            std::string name = "cycle_standby_startup_penalty_" + std::to_string(t);
            c = solver->MakeRowConstraint(name);
            c->SetCoefficient(bin_vars.ychsp[t], 1.0);
            c->SetCoefficient(bin_vars.y[t], -1.0);
            double rhs = -1.0;
            if (t > 0) {
                c->SetCoefficient(bin_vars.ycsb[t - 1], -1.0);
            }
            else {
                rhs += (init_conditions.is_pb_standby0 ? 1 : 0);
                constraints.cycle_standby_startup_penalty0 = c;
            }
            c->SetLB(rhs);
        }
    }

#ifdef MOD_CYCLE_SHUTDOWN // Figure out what we want to do with this...
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


    // ******************** TES Balance constraints *******************
    // Energy in, out, and stored in the TES system must balance.
    // delta * ( xr[t] + qeh[t] - x[t] - Qc * ycsu[t] - Qb * ycsb[t] ) = s[t] - s[t-1]
    for (int t = 0; t < m_nstep_opt; ++t) {
        std::string name = "storage_energy_balance_" + std::to_string(t);
        c = solver->MakeRowConstraint(name);
        c->SetCoefficient(cont_vars.xr[t], P["delta"]);

        if (params.is_parallel_heater) c->SetCoefficient(cont_vars.qeh[t], P["delta"]);

        c->SetCoefficient(cont_vars.x[t], -P["delta"]);
        c->SetCoefficient(bin_vars.ycsu[t], -P["delta"] * P["Qc"]);

        if (params.can_cycle_use_standby) c->SetCoefficient(bin_vars.ycsb[t], -P["delta"] * P["Qb"]);

        c->SetCoefficient(cont_vars.s[t], -1.0);

#ifdef MOD_REC_STANDBY
        c->SetCoefficient(bin_vars.yrsb[t], -P["delta"] * Qrsb);
#endif
        double rhs = 0.;
        if (t > 0) {
            c->SetCoefficient(cont_vars.s[t - 1], 1.0);
        }
        else {
            rhs += -init_conditions.e_tes0;  //initial storage state (kWh)
        }

        c->SetBounds(rhs, rhs);
        constraints.storage_energy_balance[t] = c;
    }

    // Max cycle thermal input is required in time periods where cycle operates and receiver is starting up
    // x[t+1] + Qb * ycsb[t+1] <= s[t] / delta_rs[t+1] - M * ( -3 + yrsu[t+1] + y[t] + y[t+1] + ycsb[t] + ycsb[t+1] )
    for (int t = 0; t < m_nstep_opt - 1; ++t) {
        std::string name = "storage_minimum_cycle_operating_" + std::to_string(t);
        c = solver->MakeRowConstraint(name);

        double t_rec_startup = ts_params.delta_rs.at(t) * P["delta"];
        double large = (std::max)(P["Qu"], P["Qb"]); //tighter formulation

        c->SetCoefficient(cont_vars.x[t + 1], 1.0);
        c->SetCoefficient(cont_vars.s[t], -1.0 / t_rec_startup);
        c->SetCoefficient(bin_vars.yrsu[t + 1], large);
        c->SetCoefficient(bin_vars.y[t], large);
        c->SetCoefficient(bin_vars.y[t + 1], large);

        if (params.can_cycle_use_standby) {
            c->SetCoefficient(bin_vars.ycsb[t], large);
            c->SetCoefficient(bin_vars.ycsb[t + 1], P["Qb"] + large);
        }
        c->SetUB(3.0 * large);
        constraints.storage_minimum_cycle_operating[t] = c;
    }

    if (params.is_pv_included) {
        // PV generation limit (a take it or leave it policy)
        // w_pv[t] < W_pv_avail[t]
        for (int t = 0; t < m_nstep_opt; ++t) {
            std::string name = "pv_generation_limit_" + std::to_string(t);
            c = solver->MakeRowConstraint(name);
            constraints.pv_generation_limit[t] = c;
        }
    }

    if (params.is_battery_included) {

        // TODO: Do we need lower bounds on battery charge and discharge power?
        // Battery charging power limit
        // pchar[t] <= b_pow_max * ybchar[t]
        for (int t = 0; t < m_nstep_opt; ++t) {
            std::string name = "battery_charge_limit_" + std::to_string(t);
            c = solver->MakeRowConstraint(name);
            c->SetCoefficient(cont_vars.pchar[t], 1.0);
            c->SetCoefficient(bin_vars.ybchar[t], -P["b_charge_pow_max"]);
            c->SetUB(0.0);
        }

        // Battery discharging power limit
        // pdis[t] <= b_pow_max * ybdis[t]
        for (int t = 0; t < m_nstep_opt; ++t) {
            std::string name = "battery_discharge_limit_" + std::to_string(t);
            c = solver->MakeRowConstraint(name);
            c->SetCoefficient(cont_vars.pdis[t], 1.0);
            c->SetCoefficient(bin_vars.ybdis[t], -P["b_discharge_pow_max"]);
            c->SetUB(0.0);
        }
        // Battery charge and discharge mutually exclusive
        // ybchar[t] + ybdis[t] <= 1
        for (int t = 0; t < m_nstep_opt; ++t) {
            std::string name = "battery_charge_discharge_coincide_" + std::to_string(t);
            c = solver->MakeRowConstraint(name);
            c->SetCoefficient(bin_vars.ybchar[t], 1.0);
            c->SetCoefficient(bin_vars.ybdis[t], 1.0);
            c->SetUB(1);
        }
        // Energy in, out, and stored in the Battery must balance.
        // delta * ( charge_eff * pchar[t] - (1 / discharge_eff) * pdis[t] ) / capacity = soc[t] - soc[t-1]
        for (int t = 0; t < m_nstep_opt; ++t) {
            std::string name = "battery_energy_balance_" + std::to_string(t);
            c = solver->MakeRowConstraint(name);
            c->SetCoefficient(cont_vars.pchar[t], P["delta"] * P["b_charge_eff"]);
            c->SetCoefficient(cont_vars.pdis[t], -(P["delta"] / P["b_discharge_eff"]));
            c->SetCoefficient(cont_vars.soc[t],  -P["C_bat"]);

            double rhs = 0.;
            if (t > 0) {
                c->SetCoefficient(cont_vars.soc[t - 1], P["C_bat"]);
            }
            else {
                rhs += -init_conditions.batt_soc0 * P["C_bat"];  //initial battery charge (MWh)
                constraints.batt_soc_balance0 = c;
            }
            c->SetBounds(rhs, rhs);
        }

        // Battery cycle count
        // batt_cycle_count >= delta / capacity * sum ( pdis[t] ) over t
        {
            std::string name = "battery_cycle_count";
            c = solver->MakeRowConstraint(name);
            c->SetCoefficient(cont_vars.batt_cycle_count, -1.0);
            for (int t = 0; t < m_nstep_opt; ++t) {
                c->SetCoefficient(cont_vars.pdis[t], P["delta"] / P["C_bat"]);
            }
            c->SetUB(0.0);
        }

        // Created here because it depends on w_lim, which is a time series input
        // Battery Charge generation limit -> Assumes no grid charging...
        for (int t = 0; t < m_nstep_opt; ++t) {
            std::string name = "battery_charge_generation_limit_" + std::to_string(t);
            c = solver->MakeRowConstraint(name);
            c->SetCoefficient(cont_vars.wdot[t], 1.0 - ts_params.w_condf_expected.at(t));
            c->SetCoefficient(cont_vars.xr[t], -P["Lr"]);
            c->SetCoefficient(cont_vars.xrsu[t], -P["Lr"]);
            c->SetCoefficient(bin_vars.yrsu[t], -(P["Wrsb"] / P["delta"]) - (P["Ehs"] / P["delta"]));   //MWe
            c->SetCoefficient(bin_vars.yr[t], -P["Wh"]);

            if (params.can_cycle_use_standby) c->SetCoefficient(bin_vars.ycsb[t], -P["Wb"]);
            c->SetCoefficient(cont_vars.x[t], -P["Lc"]);

            if (params.is_parallel_heater) c->SetCoefficient(cont_vars.qeh[t], -(1 / P["eta_eh"]));
            if (params.is_pv_included) c->SetCoefficient(cont_vars.w_pv[t], 1.0);

            if (params.is_battery_included) c->SetCoefficient(cont_vars.pchar[t], -1.0);
            c->SetLB(0.0);
            constraints.battery_charge_generation_limit[t] = c;
        }
    }
}

bool csp_dispatch_ortools::check_setup(int nstep)
{
    //check parameters and inputs to make sure everything has been set up correctly
    if( (int)ts_params.sell_price.size() < nstep )   return false;
    if ((int)ts_params.w_lim.size() < nstep)   return false;

    if ((int)ts_params.q_sfavail_expected.size() < nstep)   return false;
    if ((int)ts_params.eta_pb_expected.size() < nstep)   return false;
    if ((int)ts_params.f_pb_op_limit.size() < nstep)   return false;
    if ((int)ts_params.w_condf_expected.size() < nstep)   return false;

    if ((int)ts_params.wnet_lim_min.size() < nstep)   return false;
    if ((int)ts_params.delta_rs.size() < nstep)   return false;

    if ((int)ts_params.pv_generation.size() < nstep) return false;
    
    // TODO: add other checks

    return base_dispatch_opt::check_setup();
}

bool csp_dispatch_ortools::update_horizon_parameters(C_csp_tou& mc_tou)
{
    //get price signal and electricity generation limits
    int num_steps = solver_params.optimize_horizon * solver_params.steps_per_hour;
    double sec_per_step = 3600. / (double)solver_params.steps_per_hour;
    double W_dot_max = params.q_pb_max * params.eta_pb_des;                 //[MWe]

    ts_params.sell_price.clear();
    ts_params.sell_price.resize(num_steps, 1.);
    ts_params.w_lim.clear();
    ts_params.w_lim.resize(num_steps, W_dot_max * 2.0); // assuming double the design point is a hard limit

    for (int t = 0; t < num_steps; t++) {
        C_csp_tou::S_csp_tou_outputs tou_outputs;
        mc_tou.call(pointers.siminfo->ms_ts.m_time + t * sec_per_step, tou_outputs);
        ts_params.sell_price.at(t) = tou_outputs.m_elec_price * 1000.0; // $/kWhe -> $/Mhe

        // Set the maximum net power cycle output
        double w_lim_temp = tou_outputs.m_wlim_dispatch * W_dot_max; // MWe
        if (w_lim_temp < ts_params.w_lim.at(t)) ts_params.w_lim.at(t) = w_lim_temp;       // update if lower than default value

        ts_params.pv_generation.at(t) = tou_outputs.m_pv_gen / 1.e3; // kWe -> MWe
    }
    return true;
}

void csp_dispatch_ortools::update_initial_conditions(double q_dot_to_pb, double T_htf_cold_des, double pc_state_persist)
{
    //note the states of the power cycle and receiver
    init_conditions.is_rec_operating0 = pointers.col_rec->get_operating_state() == C_csp_collector_receiver::ON;
    init_conditions.is_rec_starting0 = pointers.col_rec->get_operating_state() == C_csp_collector_receiver::STARTUP;
    init_conditions.rec_startup_energy0 = 0.;       // TODO: pull this from receiver class?

    init_conditions.is_pb_operating0 = pointers.mpc_pc->get_operating_state() == C_csp_power_cycle::ON;
    init_conditions.is_pb_standby0 = pointers.mpc_pc->get_operating_state() == C_csp_power_cycle::STANDBY;
    init_conditions.pb_startup_energy0 = 0.;        // TODO: pull this from receiver class?

    // Power cycle initial heat input and power output
    init_conditions.q_pb0 = q_dot_to_pb;
    init_conditions.wdot_pb0 = 0.0;

    double slope, intercept;
    params.eff_table_load.get_slope_intercept_cycle_linear_performance(&slope, &intercept);

    if (init_conditions.q_pb0 >= params.q_pb_min)
        init_conditions.wdot_pb0 = (slope * q_dot_to_pb + intercept) * ts_params.eta_pb_expected.at(0) / params.eta_pb_des;

    //Note the state of the thermal energy storage system
    double q_disch, m_dot_disch, T_tes_return;
    pointers.tes->discharge_avail_est(T_htf_cold_des, pointers.siminfo->ms_ts.m_step, q_disch, m_dot_disch, T_tes_return);
    init_conditions.e_tes0 = q_disch * pointers.siminfo->ms_ts.m_step / 3600. + params.e_tes_min;        //MWh
    if (init_conditions.e_tes0 < params.e_tes_min)
        init_conditions.e_tes0 = params.e_tes_min;
    if (init_conditions.e_tes0 > params.e_tes_max)
        init_conditions.e_tes0 = params.e_tes_max;

    if (params.is_battery_included) {
        // TODO: should we update battery capacity over time as well?
        init_conditions.batt_soc0 = pointers.battery->battery->SOC() / 100.0;
    }
}

bool csp_dispatch_ortools::predict_performance(int step_start, int ntimeints, int divs_per_int)
{
    //Step number - 1-based index for first hour of the year.

    //save step count
    m_nstep_opt = ntimeints;

    //Predict performance out nstep values.
    ts_params.eta_sf_expected.clear();         //thermal efficiency
    ts_params.q_sfavail_expected.clear();      //predicted field energy output
    ts_params.eta_pb_expected.clear();         //power cycle efficiency
    ts_params.f_pb_op_limit.clear();           // Maximum power cycle output (normalized)
    ts_params.w_condf_expected.clear();        // condenser power
    ts_params.wnet_lim_min.clear();            // Minimum net power cycle output
    ts_params.delta_rs.clear();                // expected proportion of time step used for receiver start up

    //create the sim info
    C_csp_solver_sim_info simloc;
	simloc.ms_ts.m_step = pointers.siminfo->ms_ts.m_step;

    double pb_slope, pb_intercept;
    params.eff_table_load.get_slope_intercept_cycle_linear_performance(&pb_slope, &pb_intercept);

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

        for(int j=0; j<divs_per_int; j++) {    //take averages over hour if needed
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

        double wmin = (params.q_pb_min * pb_slope + pb_intercept) * cycle_eff_ave / params.eta_pb_des; // Electricity generation at minimum pb thermal input
        double max_parasitic = params.w_rec_pump * q_inc_ave
            + params.w_rec_ht / params.dt
            + params.w_stow / params.dt
            + params.w_track
            + params.w_cycle_standby
            + params.w_cycle_pump * params.q_pb_des
            + wcond_ave * params.q_pb_des * params.eta_pb_des;  // Largest possible parasitic load at time t

        //-----report hourly averages
        //thermal efficiency
        ts_params.eta_sf_expected.push_back( therm_eff_ave );
        //predicted field energy output
        ts_params.q_sfavail_expected.push_back( q_inc_ave );
        //power cycle efficiency
        ts_params.eta_pb_expected.push_back( cycle_eff_ave );
		// Maximum power cycle output (normalized)
        ts_params.f_pb_op_limit.push_back( f_pb_op_lim_ave );
        //condenser power
        ts_params.w_condf_expected.push_back( wcond_ave );
        // Minimum net power cycle output
        ts_params.wnet_lim_min.push_back( wmin - max_parasitic );
    }

    ts_params.delta_rs.resize(m_nstep_opt);
    for (int t = 0; t < m_nstep_opt; t++) {
        if (t < m_nstep_opt - 1) {
            double delta_rec_startup = (std::min)(1., (std::max)(params.e_rec_startup / (std::max)(ts_params.q_sfavail_expected.at(t + 1) * params.dt, 1.), params.dt_rec_startup / params.dt));
            ts_params.delta_rs.at(t) = delta_rec_startup;
        }
    }

    if(! check_setup(m_nstep_opt) )
        throw(C_csp_exception("Dispatch optimization precheck failed."));
    
    return true;
}

void csp_dispatch_ortools::s_params::create_parameter_map(unordered_map<std::string, double>& param_map, int nt)
{
    /*
     A central location for making sure the parameters from the model are accurately calculated for use in the dispatch optimization model.
    */
    param_map.clear();
    param_map["T"] = nt;
    param_map["delta"] = dt;
    param_map["Eu"] = e_tes_max;
    param_map["Er"] = e_rec_startup;
    param_map["Ec"] = e_pb_startup_cold;
    param_map["Qu"] = q_pb_des;
    param_map["Ql"] = q_pb_min;
    param_map["Qru"] = e_rec_startup / dt_rec_startup;
    param_map["Qrl"] = q_rec_min;
    param_map["Qc"] = e_pb_startup_cold / ceil(dt_pb_startup_cold / param_map["delta"]) / param_map["delta"];
    param_map["Qb"] = q_pb_standby;
    param_map["Lr"] = w_rec_pump;
    param_map["Lc"] = w_cycle_pump;
    param_map["Wh"] = w_track;
    param_map["Wb"] = w_cycle_standby;
    param_map["Ehs"] = w_stow;
    param_map["Wrsb"] = w_rec_ht;
    param_map["eta_cycle"] = eta_pb_des;
    param_map["Qrsd"] = 0.;      //<< not yet modeled, passing temporarily as zero

    if (is_parallel_heater) {
        param_map["Qehu"] = q_eh_max;
        param_map["Qehl"] = q_eh_min;
        param_map["eta_eh"] = eta_eh;
        param_map["hsu_cost"] = hsu_cost;
    }

    if (is_battery_included) {
        param_map["C_bat"] = batt_capacity;
        param_map["b_soc_min"] = batt_soc_min;
        param_map["b_soc_max"] = batt_soc_max;
        param_map["b_charge_pow_max"] = batt_charge_power_max;
        param_map["b_discharge_pow_max"] = batt_discharge_power_max;
        param_map["b_charge_eff"] = batt_charge_efficiency;
        param_map["b_discharge_eff"] = batt_discharge_efficiency;
        param_map["charge_cost"] = batt_charge_cost;
        param_map["discharge_cost"] = batt_discharge_cost;
        param_map["batt_lifecycle_cost"] = batt_lifecycle_cost;
    }

    param_map["Qrsb"] = q_rec_standby;
    param_map["W_dot_cycle"] = q_pb_des * eta_pb_des;

    // power cycle linear performance curve
    double intercept;
    eff_table_load.get_slope_intercept_cycle_linear_performance(&param_map["etap"], &intercept);

    // maximum power based on linear fit
    param_map["Wdotu"] = (param_map["etap"] * param_map["Qu"] + intercept);
    // minimum power based on linear fit
    param_map["Wdotl"] = (param_map["etap"] * param_map["Ql"] + intercept);

    //TODO: Ramp rate -> Pass as a User input
    param_map["Wdlim"] = param_map["W_dot_cycle"] * 0.03 * 60. * param_map["delta"];      //Cycle Power Ramping Limit = Rated cycle power * 3%/min (ramp limit "User Input") * 60 mins/hr * hr

    //Set by user
    param_map["disp_time_weighting"] = time_weighting;
    param_map["rsu_cost"] = rsu_cost;
    param_map["csu_cost"] = csu_cost;
    param_map["pen_delta_w"] = pen_delta_w;

    if (is_pv_included) {
        param_map["w_pv_max"] = w_pv_max;
        param_map["pv_op_cost"] = pv_op_cost;
    }
}

void csp_dispatch_ortools::update_constraints(unordered_map<std::string, double>& P) {
    // ******************** Receiver constraints *******************
    // Receiver startup energy inventory
    // ursu[t] <= ursu[t-1] + Delta * xrsu[t]       for all t in Tau
    // Update initial condition
    constraints.receiver_startup_inventory0->SetUB(init_conditions.rec_startup_energy0);

    // Receiver inventory bound when starting
    // ursu[t] <= Er * yrsu[t]                      for all t in Tau
    // No update required

    // Receiver operation allowed when start-up is complete or if receiver was operating
    // NOTE: tighter formulation when Er is distributed
    // yr[t] <= ursu[t] / Er + yr[t-1]
    // Update initial condition
    constraints.receiver_operation_allowed0->SetUB((init_conditions.is_rec_operating0 ? P["Er"] : 0.));

    // Receiver startup can't be enabled after a time step where the Receiver was operating
    // yrsu[t] + yr[t-1] <= 1
    // Update initial condition
    constraints.receiver_startup_wait0->SetUB((init_conditions.is_rec_operating0 ? 0. : 1.));

    // Receiver startup energy limit
    // xrsu[t] <= Qru * yrsu[t]
    // No update required

    // Enforces receiver startup time requirement
    // xrsu[t] >= Delta^su * Qin[t] * (yrsu[t] - yrsu[t-1])
    // Update solar resource
    for (int t = 0; t < m_nstep_opt; ++t) {
        constraints.receiver_startup_time_req[t]->SetCoefficient(bin_vars.yrsu[t], -params.dt_rec_startup * ts_params.q_sfavail_expected.at(t));
        if (t > 0) {
            constraints.receiver_startup_time_req[t]->SetCoefficient(bin_vars.yrsu[t - 1], params.dt_rec_startup * ts_params.q_sfavail_expected.at(t));
            constraints.receiver_startup_time_req[t]->SetLB(0.0);
        }
        else
            constraints.receiver_startup_time_req[t]->SetLB(-(init_conditions.is_rec_starting0 ? 1. : 0.) * params.dt_rec_startup * ts_params.q_sfavail_expected.at(t));
    }

    // Removes receiver start-ups requiring more than 2 time periods
    // yrsu[t-1] + yrsu[t] + yrsu[t+1] <= 2
    // Update initial condition
    constraints.receiver_SU_duration_limit0->SetUB(2.0 - (init_conditions.is_rec_starting0 ? 1. : 0.));

    // Receiver startup and operation consumption limit
    // xr[t] + xrsu[t] <= Qin[t]
    // Update solar resource
    for (int t = 0; t < m_nstep_opt; ++t) {
        constraints.receiver_power_limit[t]->SetUB(ts_params.q_sfavail_expected.at(t));
    }

    // Receiver maximum operation limit
    // xr[t] <= Qin[t] * yr[t]
    // Update solar resource
    for (int t = 0; t < m_nstep_opt; ++t) {
        constraints.receiver_production_limit[t]->SetCoefficient(bin_vars.yr[t], -ts_params.q_sfavail_expected.at(t));
    }

    // Receiver minimum operation limit
    // xr[t] >= Qrl * yr[t]
    // No update required

    // Receiver startup only during solar positive periods
    // TODO: This relies on presolve to remove variables (might want to create a special set of solar hours)
    // yrsu[t] <= 0 when Qin[t] = 0
    // TODO: We could just set yrsu[t] to zero when Qin[t] = 0
    for (int t = 0; t < m_nstep_opt; ++t) {
        constraints.receiver_startup_cut[t]->SetUB((std::min)(P["Qru"] * ts_params.q_sfavail_expected.at(t), 1.0));
    }

    // Receiver can't continue operating when no energy is available
    // yr[t] <= Qin[t] / Qrl
    // Update solar resource
    for (int t = 0; t < m_nstep_opt; ++t) {
        constraints.receiver_operation_limit_cut[t]->SetUB((std::min)(floor(ts_params.q_sfavail_expected.at(t) / P["Qrl"]), 1.0));
    }

    // Receiver startup penalty
    // yrsup[t] >= yrsu[t] - yrsu[t-1]
    // Update initial condition
    constraints.receiver_startup_penalty0->SetLB((init_conditions.is_rec_starting0 ? -1. : 0.));


    // ******************** Electric Heater constraints ****************
    if (params.is_parallel_heater) {
        // Heater power limit
        // qeh[t] <= Qehu * yeh[t]
        // No update required

        // Heater minimum operation requirement
        // qeh[t] >= Qehl * yeh[t]
        // No update required

        // Heaters and cycle cannot coincide
        // yeh[t] + y[t] <= 1
        // No update required

        // Heaters must be off before field defocus
        // xr[t] + xrsu[t] >= Qin[t] * yreh[t]
        // Update solar resource
        for (int t = 0; t < m_nstep_opt; ++t) {
            constraints.heater_off_before_defocus[t]->SetCoefficient(bin_vars.yreh[t], -ts_params.q_sfavail_expected.at(t));
        }

        //******* linearization of yreh[t] = yr[t] * yeh[t] ******

        // Upper bound with yr
        // yreh[t] <= yr[t]
        // No update required

        // Upper bound with yeh
        // yreh[t] <= yeh[t]
        // No update required

        // Lower bound
        // yreh[t] >= yr[t] + yeh[t] - 1
        // No update required

        // Heater startup penalty
        // yhsup[t] >= yeh[t] - yeh[t-1]
        // Update initial condition
        constraints.heater_startup_penalty0->SetLB((init_conditions.is_heater_operating0 ? -1. : 0.));
    }

    // ******************** Power cycle constraints *******************

    // Startup inventory balance
    // ucsu[t] <= ucsu[t-1] + delta * Qc * ycsu[t]
    // Update initial condition
    constraints.cycle_startup_inventory0->SetUB(init_conditions.pb_startup_energy0);

    // Inventory nonzero
    // ucsu[t] <= Ec * ycsu[t]
    // No update required

    // Cycle operation allowed when startup is complete or already operating or in standby
    // hourly:    y[t] <=  ucsu[t  ] / Ec + y[t-1] + ycsb[t-1]
    // subhourly: y[t] <=  ucsu[t-1] / Ec + y[t-1] + ycsb[t-1]   (startup and production cannot coincide)
    // NOTE: tighter formulation when Ec is distributed
    // Update initial condition
    double rhs = 0.0;
    if (P["delta"] < 1.) rhs += init_conditions.pb_startup_energy0;
    rhs += P["Ec"] * (init_conditions.is_pb_operating0 ? 1 : 0);
    if (params.can_cycle_use_standby) {
        rhs += P["Ec"] * (init_conditions.is_pb_standby0 ? 1 : 0);
    }
    constraints.cycle_operation_allowed0->SetUB(rhs);

    // Cycle consumption limit (valid only for hourly model -> Delta == 1)
    // x[t] + Qc * ycsu[t] <= Qu * y[t]
    // No update required

    // Cycle maximum operation limit
    // x[t] <= Qu * y[t]
    // No update required

    // Cycle minimum operation limit
    // x[t] >= Ql * y[t]
    // No update required

    // Power production linearization (power as function of heat input)
    // wdot[t] = eta_amb[t]/eta_des * ( etap * x[t] + ( Wdotu - etap * Qu ) * y[t] )
    // Update ambient correction
    for (int t = 0; t < m_nstep_opt; ++t) {
        constraints.cycle_production_linearization[t]->SetCoefficient(cont_vars.x[t], -P["etap"] * ts_params.eta_pb_expected.at(t) / P["eta_cycle"]);
        constraints.cycle_production_linearization[t]->SetCoefficient(bin_vars.y[t], -(P["Wdotu"] - P["etap"] * P["Qu"]) * ts_params.eta_pb_expected.at(t) / P["eta_cycle"]);
    }

    // Cycle positive production change (i.e., ramping) We might want to penalize thermal ramping instead to remove the ambient correction
    // delta_w[t] >= wdot[t] - wdot[t-1]
    // Update initial condition
    constraints.cycle_ramp_up0->SetLB(-init_conditions.wdot_pb0);

    // Cycle sub-hourly (Delta < 1) ramping limit that doesn't hold when starting up, (or going into standby), (or shutdown)
    // delta_t[t] <= Wdlim + temp_coef * ( y[t] - y[t-1] + 2 * ycsb[t] + 2 * yoff[t] )
    if (P["delta"] < 1.) {
        for (int t = 0; t < m_nstep_opt; ++t) {
            double temp_coef = (ts_params.eta_pb_expected.at(t) / P["eta_cycle"]) * P["Wdotl"] - P["Wdlim"];
            double rhs = P["Wdlim"];

            constraints.cycle_sub_hourly_ramp_up[t]->SetCoefficient(bin_vars.y[t], -temp_coef);
            if (t > 0) {
                constraints.cycle_sub_hourly_ramp_up[t]->SetCoefficient(bin_vars.y[t - 1], temp_coef);
            }
            else {
                rhs += -temp_coef * (init_conditions.is_pb_operating0 ? 1 : 0);
            }

            if (params.can_cycle_use_standby) {
                constraints.cycle_sub_hourly_ramp_up[t]->SetCoefficient(bin_vars.ycsb[t], -temp_coef * 2.);
                constraints.cycle_sub_hourly_ramp_up[t]->SetCoefficient(bin_vars.yoff[t], -temp_coef * 2.);
            }

#ifdef MOD_CYCLE_SHUTDOWN
            constraints.cycle_sub_hourly_ramp_up[t]->SetCoefficient(bin_vars.ycsd[t], -temp_coef * 2.);
#endif
            constraints.cycle_sub_hourly_ramp_up[t]->SetUB(rhs);
        }
    }

    // Cycle startup and operation cannot coincide (valid for sub-hourly model Delta < 1)
    // ycsu[t] + y[t] <= 1
    // No update required

    // Cycle startup can't be enabled after a time step where the cycle was operating
    // ycsu[t] + y[t-1] <= 1
    // Update initial condition
    constraints.cycle_startup_wait0->SetUB(1 - (init_conditions.is_pb_operating0 ? 1 : 0));

    // Cycle start penalty
    // ycsup[t] >= ycsu[t] - ycsu[t-1]
    // Update initial condition
    constraints.cycle_startup_penalty0->SetLB((init_conditions.is_pb_starting0 ? -1. : 0.));

    // Maximum gross electricity production constraint
    // wdot[t] <= f_limit * W_dot_cycle
    for (int t = 0; t < m_nstep_opt; ++t) {
        constraints.cycle_maximum_production[t]->SetUB(ts_params.f_pb_op_limit.at(t) * P["W_dot_cycle"]);
    }

    // Created here because it depends on w_lim, which is a time series input
    // Maximum net electricity production constraint
    for (int t = 0; t < m_nstep_opt; ++t) {
        if (ts_params.wnet_lim_min.at(t) > ts_params.w_lim.at(t)) {      // power cycle operation is impossible at t
            if (ts_params.w_lim.at(t) > 0)
                pointers.messages->add_message(C_csp_messages::NOTICE, "Power cycle operation not possible at time " + util::to_string(t + 1) + ": power limit below minimum operation");
            ts_params.w_lim.at(t) = 0.;
        }
        MPConstraint* c = constraints.cycle_maximum_net_power[t];

        // TODO: This might not be true any more because the battery can charge...
        if (ts_params.w_lim.at(t) < 0.) { // Power cycle operation is impossible at current constrained w_lim
            c->SetCoefficient(cont_vars.wdot[t], 1.0);
            c->SetBounds(0.0, 0.0);
        }
        else if (ts_params.w_lim.at(t) > ts_params.f_pb_op_limit.at(t) * P["W_dot_cycle"]) { // Net limit is greater than gross
            continue;
        }
        else { // Power cycle operation is possible
            c->SetCoefficient(cont_vars.wdot[t], 1.0 - ts_params.w_condf_expected.at(t));
            c->SetCoefficient(cont_vars.xr[t], -P["Lr"]);
            c->SetCoefficient(cont_vars.xrsu[t], -P["Lr"]);
            c->SetCoefficient(bin_vars.yrsu[t], -(P["Wrsb"] / P["delta"]) - (P["Ehs"] / P["delta"]));   //MWe
            c->SetCoefficient(bin_vars.yr[t], -P["Wh"]);

            if (params.can_cycle_use_standby) c->SetCoefficient(bin_vars.ycsb[t], -P["Wb"]);

            c->SetCoefficient(cont_vars.x[t], -P["Lc"]);

            if (params.is_parallel_heater) c->SetCoefficient(cont_vars.qeh[t], - (1 / P["eta_eh"]));

            if (params.is_pv_included) c->SetCoefficient(cont_vars.w_pv[t], 1.0);

            if (params.is_battery_included) {
                c->SetCoefficient(cont_vars.pdis[t], 1.0);
                c->SetCoefficient(cont_vars.pchar[t], -1.0);
            }

            c->SetUB(ts_params.w_lim.at(t) + params.sys_par_fixed);     // Add fixed system parasitic power to limit

        }
    }

    if (params.can_cycle_use_standby) {
        // Cycle standby mode persistence
        // ycsb[t] <= y[t-1] + ycsb[t-1]
        // Update initial condition
        double rhs = (init_conditions.is_pb_operating0 ? 1. : 0.) + (init_conditions.is_pb_standby0 ? 1. : 0.);
        constraints.cycle_standby_persistence0->SetUB(rhs);

        // Cycle standby startup penalty
        // ychsp[t] >= y[t] - (1 - ycsb[t-1])
        // Update initial condition
        rhs = -1.0; // Reset value
        rhs += (init_conditions.is_pb_standby0 ? 1 : 0);
        constraints.cycle_standby_startup_penalty0->SetLB(rhs);
    }

    // ******************** TES Balance constraints *******************
    // Energy in, out, and stored in the TES system must balance.
    // delta * ( xr[t] + qeh[t] - x[t] - Qc * ycsu[t] - Qb * ycsb[t] ) = s[t] - s[t-1]
    // Update initial condition
    constraints.storage_energy_balance[0]->SetBounds(-init_conditions.e_tes0, -init_conditions.e_tes0);

    // Max cycle thermal input is required in time periods where cycle operates and receiver is starting up
    // x[t+1] + Qb * ycsb[t+1] <= s[t] / delta_rs[t+1] - M * ( -3 + yrsu[t+1] + y[t] + y[t+1] + ycsb[t] + ycsb[t+1] )
    // update receiver startup
    for (int t = 0; t < m_nstep_opt - 1; ++t) {
        constraints.storage_minimum_cycle_operating[t]->SetCoefficient(cont_vars.s[t], -1.0 / (ts_params.delta_rs.at(t) * P["delta"]));
    }

    if (params.is_pv_included) {
        // ******************* PV Generation *******************
        // PV generation limit (a take it or leave it policy)
        // w_pv[t] < W_pv_avail[t]
        for (int t = 0; t < m_nstep_opt; ++t) {
            constraints.pv_generation_limit[t]->SetCoefficient(cont_vars.w_pv[t], 1.);
            constraints.pv_generation_limit[t]->SetUB(ts_params.pv_generation.at(t));
        }
    }

    if (params.is_battery_included) {
        // ******************* Battery constraints *******************
        // Battery state of charge balance
        // delta * ( charge_eff * pchar[t] - (1 / discharge_eff) * pdis[t] ) / capacity = soc[t] - soc[t-1]
        // Update initial condition
        constraints.batt_soc_balance0->SetBounds(-init_conditions.batt_soc0 * P["C_bat"], -init_conditions.batt_soc0 * P["C_bat"]);
        // Battery charge and discharge limits
        // b_charge[t] <= b_pow_max * y_b_charge[t]
        // b_discharge[t] <= b_pow_max * y_b_discharge[t]
        // No update required
        // Battery can't charge and discharge at the same time
        // y_b_charge[t] + y_b_discharge[t] <= 1
        // No update required

        // Battery Charge generation limit
        // Update cycle condenser losses
        for (int t = 0; t < m_nstep_opt; ++t) {
            constraints.battery_charge_generation_limit[t]->SetCoefficient(cont_vars.wdot[t], 1.0 - ts_params.w_condf_expected.at(t));
        }
    }
}

bool csp_dispatch_ortools::optimize()
{
    /*
    1. Update optimization model
    2. Solve optimization model
    3. Get results from optimization model
    */

    // Re-generate parameters map
    unordered_map<std::string, double> P;
    params.create_parameter_map(P, m_nstep_opt);

    // Update Objective function coefficients
    update_objective_function(P);

    // Update constraints
    update_constraints(P);

    // Solver the model and time the solve
    std::clock_t c_start = std::clock();
    MPSolver::ResultStatus result_status = solver->Solve(MPsolver_params);
    std::clock_t c_end = std::clock();
    double time_elapsed_sec = (c_end - c_start) / (double)CLOCKS_PER_SEC;

    solver_outputs.solve_state = (int)result_status;
    set_solver_outputs(time_elapsed_sec);

    // For DEBUGGING Model
    //solver->Write("csp_ortools_dispatch_problem");  // mps format -> works with Xpress
    //printResultsFile("csp_ortools_solution.csv", false);
    //throw C_csp_exception("Setup and ran first dispatch optimization problem for Debugging");

    set_outputs_from_solution(P);
    //print_dispatch_update();

    if (result_status == MPSolver::OPTIMAL || result_status == MPSolver::FEASIBLE)
        return true; // Successful solve
    else
        return false;   
}

void csp_dispatch_ortools::set_outputs_from_solution(unordered_map<std::string, double>& P) {
    // sets output structure from optimization solution
    // TODO: This layer was required for lp_solve because the lp object was destroyed and rebuilt each solve.
    //      With or-tools, this is no longer necessary, but it is retained to minimize changes to the rest of the dispatch code.
    // In the future (once all dispatch models using or-tools), this could be merged with set_dispatch_outputs to eliminate redundant code.

    int nt = (int)m_nstep_opt;

    outputs.clear();
    outputs.resize(nt);

    for (int t = 0; t < nt; t++) {

        bool ycsu = bin_vars.ycsu.at(t)->solution_value();
        bool y = bin_vars.y.at(t)->solution_value();
        //Cycle start up
        outputs.q_pb_startup.at(t) = ycsu ? P["Qc"] : 0.;
        // is cycle operating
        outputs.pb_operation.at(t) = y || ycsu;
        // cycle thermal energy consumption
        outputs.q_pb_target.at(t) = cont_vars.x.at(t)->solution_value();
        // is receiver starting or operating
        outputs.rec_operation.at(t) = bin_vars.yrsu.at(t)->solution_value() || bin_vars.yr.at(t)->solution_value();
        // receiver startup energy
        outputs.q_rec_startup.at(t) = cont_vars.xrsu.at(t)->solution_value();
        // thermal storage charge state
        outputs.tes_charge_expected.at(t) = cont_vars.s.at(t)->solution_value();
        // receiver production
        outputs.q_sf_expected.at(t) = cont_vars.xr.at(t)->solution_value();
        // net electricity production
        //outputs.w_pb_target.at(t) = cont_vars.wdot.at(t)->solution_value();


        // TODO: save parasitic losses and pass through cmod
        outputs.w_pb_target.at(t) = cont_vars.wdot.at(t)->solution_value() * (1.0 - ts_params.w_condf_expected.at(t));
        if (bin_vars.ycsu.at(t)->solution_value()) {
            outputs.w_pb_target.at(t) /= (1.0 - params.dt_pb_startup_cold);     // Start-up adjustment
        }
        // Parasitic losses
        outputs.sys_parasitic.at(t) = (cont_vars.xr.at(t)->solution_value() * P["Lr"]
            + cont_vars.xrsu.at(t)->solution_value() * P["Lr"]
            + bin_vars.yrsu.at(t)->solution_value() * ((P["Wrsb"] / P["delta"]) + (P["Ehs"] / P["delta"]))
            + bin_vars.yr.at(t)->solution_value() * P["Wh"]
            + cont_vars.x.at(t)->solution_value() * P["Lc"])
            + params.sys_par_fixed;

        outputs.w_pb_target.at(t) -= outputs.sys_parasitic.at(t);

        //if (params.can_cycle_use_standby) outputs.w_pb_target.at(t) -= bin_vars.ycsb.at(t)->solution_value() * P["Wb"];
        if (params.is_parallel_heater) {
            double heater_power = cont_vars.qeh.at(t)->solution_value() * (1 / P["eta_eh"]);
            if (bin_vars.yhsup.at(t)->solution_value()) {
                heater_power /= (1.0 - params.dt_eh_startup);
            }
            outputs.w_pb_target.at(t) -= heater_power;
            //outputs.w_pb_target.at(t) -= bin_vars.yhsup.at(t)->solution_value() * params.e_eh_su * (1 / P["eta_eh"]);
        }

        // cycle standby
        if (params.can_cycle_use_standby)
            outputs.pb_standby.at(t) = bin_vars.ycsb.at(t)->solution_value();
        else
            outputs.pb_standby.at(t) = false;  // Cycle standby is not allowed

        if (params.is_parallel_heater) {
            // is parallel heater on
            outputs.htr_operation.at(t) = bin_vars.yeh.at(t)->solution_value();
            // heater target power
            outputs.q_eh_target.at(t) = cont_vars.qeh.at(t)->solution_value();
        }
        else {// Heater is not allowed
            outputs.htr_operation.at(t) = false;
            outputs.q_eh_target.at(t) = 0.;
        }

        // PV target power - that is being sent to the grid
        if (params.is_pv_included)
            outputs.w_pv_target.at(t) = cont_vars.w_pv.at(t)->solution_value();

        // Battery charge and discharge power
        if (params.is_battery_included) {
            outputs.batt_power_target.at(t) = cont_vars.pdis.at(t)->solution_value() - cont_vars.pchar.at(t)->solution_value();
            outputs.batt_soc_expected.at(t) = cont_vars.soc[t]->solution_value() * 100.0;
        }
    }
}


bool csp_dispatch_ortools::set_dispatch_outputs()
{
    if (solver_outputs.last_opt_successful && m_current_read_step < (int)outputs.q_pb_target.size())
    {
        //calculate the current read step, account for number of dispatch steps per hour and the simulation time step
        m_current_read_step = (int)(pointers.siminfo->ms_ts.m_time * solver_params.steps_per_hour / 3600. - .001)
            % (solver_params.optimize_frequency * solver_params.steps_per_hour);

        if (m_current_read_step > (int)outputs.q_pb_target.size())
            throw C_csp_exception("Current read step is greater than solution horizon.", "csp_dispatch");

        dispatch_outputs.is_rec_su_allowed = outputs.rec_operation.at(m_current_read_step);
        dispatch_outputs.is_pc_sb_allowed = outputs.pb_standby.at(m_current_read_step);
        dispatch_outputs.is_pc_su_allowed = outputs.pb_operation.at(m_current_read_step) || dispatch_outputs.is_pc_sb_allowed;

        dispatch_outputs.q_pc_target = outputs.q_pb_target.at(m_current_read_step) + outputs.q_pb_startup.at(m_current_read_step);

        dispatch_outputs.q_dot_elec_to_CR_heat = outputs.q_sf_expected.at(m_current_read_step);

        dispatch_outputs.q_eh_target = outputs.q_eh_target.at(m_current_read_step);
        dispatch_outputs.is_eh_su_allowed = outputs.htr_operation.at(m_current_read_step);

        dispatch_outputs.w_dot_target = outputs.w_pb_target.at(m_current_read_step);

        //quality checks
        /*
        if(!is_pc_sb_allowed && (q_pc_target + 1.e-5 < q_pc_min))
            is_pc_su_allowed = false;
        if(is_pc_sb_allowed)
            q_pc_target = dispatch.params.q_pb_standby*1.e-3;
        */

        if (dispatch_outputs.q_pc_target + 1.e-5 < params.q_pb_min) {
            dispatch_outputs.is_pc_su_allowed = false;
            dispatch_outputs.q_pc_target = 0.0;
        }

        // Calculate approximate upper limit for power cycle thermal input at current electricity generation limit
        if (ts_params.w_lim.at(m_current_read_step) < 1.e-6) {
            dispatch_outputs.q_dot_pc_max = 0.0;
        }
        else if (ts_params.w_lim.at(m_current_read_step) / params.eta_pb_des > params.q_pb_max) { // Output limit is greater than max
            dispatch_outputs.q_dot_pc_max = fmax(params.q_pb_max, dispatch_outputs.q_pc_target);
        }
        else {
            double wcond;
            double eta_corr = pointers.mpc_pc->get_efficiency_at_TPH(pointers.m_weather.ms_outputs.m_tdry, 1., 30., &wcond) / params.eta_pb_des;
            double eta_calc = params.eta_pb_des * eta_corr;
            double eta_diff = 1.;
            int i = 0;
            while (eta_diff > 0.001 && i < 20) {
                double q_pc_est = ts_params.w_lim.at(m_current_read_step) / eta_calc;			// Estimated power cycle thermal input at w_lim
                double eta_new = pointers.mpc_pc->get_efficiency_at_load(q_pc_est / params.q_pb_des) * eta_corr;		// Calculated power cycle efficiency
                eta_diff = std::abs(eta_calc - eta_new);
                eta_calc = eta_new;
                i++;
            }
            dispatch_outputs.q_dot_pc_max = fmin(dispatch_outputs.q_dot_pc_max, ts_params.w_lim.at(m_current_read_step) / eta_calc); // Restrict max pc thermal input to *approximate* current allowable value (doesn't yet account for parasitic)
            dispatch_outputs.q_dot_pc_max = fmax(dispatch_outputs.q_dot_pc_max, dispatch_outputs.q_pc_target);				         // calculated q_pc_target accounts for parasitic --> can be higher than approximate limit 
        }

        dispatch_outputs.etasf_expect = ts_params.eta_sf_expected.at(m_current_read_step);
        dispatch_outputs.qsf_expect = ts_params.q_sfavail_expected.at(m_current_read_step);
        dispatch_outputs.qsfprod_expect = outputs.q_sf_expected.at(m_current_read_step);
        dispatch_outputs.qsfsu_expect = outputs.q_rec_startup.at(m_current_read_step);
        dispatch_outputs.tes_expect = outputs.tes_charge_expected.at(m_current_read_step);
        dispatch_outputs.qpbsu_expect = outputs.q_pb_startup.at(m_current_read_step);
        dispatch_outputs.wpb_expect = outputs.w_pb_target.at(m_current_read_step);
        dispatch_outputs.sys_parasitic = outputs.sys_parasitic.at(m_current_read_step);
        dispatch_outputs.rev_expect = dispatch_outputs.wpb_expect * ts_params.sell_price.at(m_current_read_step);
        dispatch_outputs.etapb_expect = dispatch_outputs.wpb_expect / (std::max)(1.e-6, outputs.q_pb_target.at(m_current_read_step))
            * (outputs.pb_operation.at(m_current_read_step) ? 1. : 0.);

        dispatch_outputs.pv_expect = outputs.w_pv_target.at(m_current_read_step);

        dispatch_outputs.batt_power_target = outputs.batt_power_target.at(m_current_read_step);
        dispatch_outputs.batt_soc_expected = outputs.batt_soc_expected.at(m_current_read_step);
        dispatch_outputs.sys_power_limit = ts_params.w_lim.at(m_current_read_step);

        if (m_current_read_step > solver_params.optimize_frequency* solver_params.steps_per_hour)
            throw C_csp_exception("Counter synchronization error in dispatch optimization routine.", "csp_dispatch");
    }
    dispatch_outputs.time_last = pointers.siminfo->ms_ts.m_time;

    return true;
}


void csp_dispatch_ortools::printResultsFile(std::string filepath, bool append) {
    std::ofstream outputfile;
    if (append)
        outputfile.open(filepath, std::ios_base::app);
    else {
        // Only print header when not appending
        outputfile.open(filepath);
        std::string var_header = "Time, Objective, Objective_wo_weight, Price, Qin, ";
        var_header += "y_rsu, y_rsup, yr, u_rsu, x_rsu, x_r, ";
        var_header += "y_csu, y_csup, y, u_csu, x, w_dot, delta_w, s, w_lim, wnet_lim_min, sys_parasitic";

        if (params.is_parallel_heater) {
            var_header += ", y_eh, y_reh, y_hsup, q_eh";
        }
        if (params.is_pv_included) {
            var_header += ", w_pv, w_pv_avail";
        }
        if (params.is_battery_included) {
            var_header += ", batt_power_target, batt_soc, y_batt_charge, y_batt_discharge, battery_objective";
        }
        outputfile << var_header << std::endl;
    }

    double tol = 1.e-2;
    double sys_parasitic;
    double objective_value;
    double common_coeff;

    double obj_wo_weight;  // Objective function value without time weighting
    double coeff_wo_weight;
    double battery_objective;
    for (int t = 0; t < solver_params.optimize_horizon; ++t) {
        // Calculate objective value for time t
        objective_value = 0.0;
        obj_wo_weight = 0.0;
        battery_objective = 0.0;

        common_coeff = params.dt * std::pow(params.time_weighting, t);
        coeff_wo_weight = params.dt;

        objective_value += common_coeff * ts_params.sell_price.at(t) * (1. - ts_params.w_condf_expected.at(t)) * cont_vars.wdot[t]->solution_value();
        obj_wo_weight += coeff_wo_weight * ts_params.sell_price.at(t) * (1. - ts_params.w_condf_expected.at(t)) * cont_vars.wdot[t]->solution_value();

        // TODO: Update this
        if (params.is_pv_included) {
            objective_value += common_coeff * ts_params.sell_price.at(t) * cont_vars.w_pv[t]->solution_value();
        }

        if (params.is_battery_included) {
            battery_objective += common_coeff * ts_params.sell_price.at(t) * cont_vars.pdis[t]->solution_value();
            objective_value += common_coeff * ts_params.sell_price.at(t) * cont_vars.pdis[t]->solution_value();
            obj_wo_weight += coeff_wo_weight * ts_params.sell_price.at(t) * cont_vars.pdis[t]->solution_value();
        }

        // Cost terms
        common_coeff = - params.dt * ts_params.sell_price.at(t) * (1. / std::pow(params.time_weighting, t));
        coeff_wo_weight = - params.dt * ts_params.sell_price.at(t);

        objective_value += common_coeff * params.w_rec_pump * (cont_vars.xr[t]->solution_value() + cont_vars.xrsu[t]->solution_value());
        obj_wo_weight += coeff_wo_weight * params.w_rec_pump * (cont_vars.xr[t]->solution_value() + cont_vars.xrsu[t]->solution_value());

        objective_value += common_coeff * params.w_track * bin_vars.yr[t]->solution_value();
        obj_wo_weight += coeff_wo_weight * params.w_track * bin_vars.yr[t]->solution_value();

        objective_value += common_coeff * params.w_cycle_pump * cont_vars.x[t]->solution_value();
        obj_wo_weight += coeff_wo_weight * params.w_cycle_pump * cont_vars.x[t]->solution_value();

        common_coeff /= params.dt;
        coeff_wo_weight /= params.dt;

        objective_value += common_coeff * (params.w_stow + params.w_rec_ht) * bin_vars.yrsu[t]->solution_value();
        obj_wo_weight += coeff_wo_weight * (params.w_stow + params.w_rec_ht) * bin_vars.yrsu[t]->solution_value();

        common_coeff = - (1. / std::pow(params.time_weighting, t));
        coeff_wo_weight = -1.;

        objective_value += common_coeff * params.pen_delta_w * cont_vars.delta_w[t]->solution_value();
        obj_wo_weight += coeff_wo_weight * params.pen_delta_w * cont_vars.delta_w[t]->solution_value();

        objective_value += common_coeff * params.rsu_cost * bin_vars.yrsup[t]->solution_value();
        obj_wo_weight += coeff_wo_weight * params.rsu_cost * bin_vars.yrsup[t]->solution_value();

        objective_value += common_coeff * params.csu_cost * bin_vars.ycsup[t]->solution_value();
        obj_wo_weight += coeff_wo_weight * params.csu_cost * bin_vars.ycsup[t]->solution_value();

        if (params.can_cycle_use_standby) {
            objective_value += common_coeff * params.dt * ts_params.sell_price.at(t) * params.w_cycle_standby * bin_vars.ycsb[t]->solution_value();
            obj_wo_weight += coeff_wo_weight * params.dt * ts_params.sell_price.at(t) * params.w_cycle_standby * bin_vars.ycsb[t]->solution_value();

            objective_value += common_coeff * params.csu_cost * 0.1 * bin_vars.ychsp[t]->solution_value();
            obj_wo_weight += coeff_wo_weight * params.csu_cost * 0.1 * bin_vars.ychsp[t]->solution_value();
        }

        if (params.is_parallel_heater) {
            objective_value += common_coeff * params.dt * ts_params.sell_price.at(t) * (1. / params.eta_eh) * cont_vars.qeh[t]->solution_value();
            obj_wo_weight += coeff_wo_weight * params.dt * ts_params.sell_price.at(t) * (1. / params.eta_eh) * cont_vars.qeh[t]->solution_value();

            objective_value += common_coeff * params.hsu_cost * bin_vars.yhsup[t]->solution_value();
            obj_wo_weight += coeff_wo_weight * params.hsu_cost * bin_vars.yhsup[t]->solution_value();
        }

        if (params.is_battery_included) {
            battery_objective += common_coeff * params.dt * params.batt_charge_cost * cont_vars.pchar[t]->solution_value();
            objective_value += common_coeff * params.dt * params.batt_charge_cost * cont_vars.pchar[t]->solution_value();
            obj_wo_weight += coeff_wo_weight * params.dt * params.batt_charge_cost * cont_vars.pchar[t]->solution_value();

            battery_objective += common_coeff * params.dt * params.batt_discharge_cost * cont_vars.pdis[t]->solution_value();
            objective_value += common_coeff * params.dt * params.batt_discharge_cost * cont_vars.pdis[t]->solution_value();
            obj_wo_weight += coeff_wo_weight * params.dt * params.batt_discharge_cost * cont_vars.pdis[t]->solution_value();
        }

        // Add the final term to value storage at end of optimization horizon
        if (t == m_nstep_opt - 1) {
            double pmean = 0;
            for (int t = 0; t < (int)ts_params.sell_price.size(); t++)
                pmean += ts_params.sell_price.at(t);
            pmean /= (double)ts_params.sell_price.size();

            objective_value += params.dt * std::pow(params.time_weighting, t) * pmean * params.eta_pb_des * params.inventory_incentive * cont_vars.s[t]->solution_value();
        }

        sys_parasitic = (cont_vars.wdot.at(t)->solution_value() * ts_params.w_condf_expected.at(t)
            + cont_vars.xr.at(t)->solution_value() * params.w_rec_pump
            + cont_vars.xrsu.at(t)->solution_value() * params.w_rec_pump
            + bin_vars.yrsu.at(t)->solution_value() * ((params.w_rec_ht / params.dt) + (params.w_stow / params.dt))
            + bin_vars.yr.at(t)->solution_value() * params.w_track
            + cont_vars.x.at(t)->solution_value() * params.w_cycle_pump
            + params.sys_par_fixed);

        outputfile << t
            << ", " << objective_value
            << ", " << obj_wo_weight
            << ", " << ts_params.sell_price[t]
            << ", " << ts_params.q_sfavail_expected[t]
            << ", " << bin_vars.yrsu[t]->solution_value()
            << ", " << bin_vars.yrsup[t]->solution_value()
            << ", " << bin_vars.yr[t]->solution_value()
            << ", " << ((std::abs(cont_vars.ursu[t]->solution_value()) < tol) ? 0.0 : cont_vars.ursu[t]->solution_value())
            << ", " << ((std::abs(cont_vars.xrsu[t]->solution_value()) < tol) ? 0.0 : cont_vars.xrsu[t]->solution_value())
            << ", " << ((std::abs(cont_vars.xr[t]->solution_value()) < tol) ? 0.0 : cont_vars.xr[t]->solution_value())
            << ", " << bin_vars.ycsu[t]->solution_value()
            << ", " << bin_vars.ycsup[t]->solution_value()
            << ", " << bin_vars.y[t]->solution_value()
            << ", " << ((std::abs(cont_vars.ucsu[t]->solution_value()) < tol) ? 0.0 : cont_vars.ucsu[t]->solution_value())
            << ", " << ((std::abs(cont_vars.x[t]->solution_value()) < tol) ? 0.0 : cont_vars.x[t]->solution_value())
            << ", " << ((std::abs(cont_vars.wdot[t]->solution_value()) < tol) ? 0.0 : cont_vars.wdot[t]->solution_value())
            << ", " << ((std::abs(cont_vars.delta_w[t]->solution_value()) < tol) ? 0.0 : cont_vars.delta_w[t]->solution_value())
            << ", " << ((std::abs(cont_vars.s[t]->solution_value()) < tol) ? 0.0 : cont_vars.s[t]->solution_value())
            << ", " << ts_params.w_lim.at(t)
            << ", " << ts_params.wnet_lim_min.at(t)
            << ", " << sys_parasitic;
        if (params.is_parallel_heater) {
            outputfile << ", " << bin_vars.yeh[t]->solution_value()
                << ", " << bin_vars.yreh[t]->solution_value()
                << ", " << bin_vars.yhsup[t]->solution_value()
                << ", " << ((std::abs(cont_vars.qeh[t]->solution_value()) < tol) ? 0.0 : cont_vars.qeh[t]->solution_value());
        }
        if (params.is_pv_included) {
            outputfile << ", " << ((std::abs(cont_vars.w_pv[t]->solution_value()) < tol) ? 0.0 : cont_vars.w_pv[t]->solution_value())
                << ", " << ts_params.pv_generation.at(t);
        }
        if (params.is_battery_included) {
            double batt_power = cont_vars.pdis[t]->solution_value() - cont_vars.pchar[t]->solution_value();
            outputfile << ", " << ((std::abs(batt_power) < tol) ? 0.0 : batt_power)
                << ", " << ((std::abs(cont_vars.soc[t]->solution_value()) < tol) ? 0.0 : cont_vars.soc[t]->solution_value())
                << ", " << bin_vars.ybchar[t]->solution_value()
                << ", " << bin_vars.ybdis[t]->solution_value()
                << ", " << battery_objective;
        }
        outputfile << std::endl;
    }
    outputfile.close();
}
