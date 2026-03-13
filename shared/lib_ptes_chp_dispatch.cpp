#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip> // for table output

#include "ortools/base/timer.h"
#include "lib_ptes_chp_dispatch.h"
using namespace operations_research;

void PTES_CHP_Dispatch_Data::setPrices(std::string sell_price_file, double average_price_multipler /*$/MWhe*/,
    double heat_price /*$/MWht*/) {
    set_data_by_file(sell_price, sell_price_file);
    for (int t = 0; t < sell_price.size(); t++) {
        sell_price[t] *= average_price_multipler;
    }
    purchase_price = sell_price;                        // sell and purchase price equal
    heat_sell_price.resize(n_periods, heat_price);      // Set to a constant of $20/MWht
};

void PTES_CHP_Dispatch_Data::setHeatLoad(std::string heat_load_file, double multipler /*-*/) {
    set_data_by_file(heat_demand, heat_load_file);
    for (int t = 0; t < heat_demand.size(); t++) {
        heat_demand[t] *= multipler;
    }
}

void PTES_CHP_Dispatch_Data::set_data_by_file(std::vector<double>& data, std::string filepath) {

    std::vector< std::string> data_str;
    std::ifstream datafile(filepath);

    char line[250];
    if (datafile.is_open()) {
        while (datafile) {
            datafile.getline(line, 250);
            data_str.push_back(line);
        }
    }

    data.resize(data_str.size() - 1, 0.);
    for (int i = 0; i < data_str.size() - 1; i++) {
        data.at(i) = std::stod(data_str.at(i));
    }
}

void PTES_CHP_Dispatch_Data::setDefaultAssumptions(PtesDesign design) {
    eta_amb.resize(n_periods, 1.);       // assuming no ambient temperature impact

    // Cost parameters
    cycle_ramping_cost = 0.3;                         // $/MWt
    cycle_start_cost = 20.0 * design.cycle_cap;       // $/start
    heat_pump_ramp_cost = 0.6;                        // $/MWe 
    heat_pump_start_cost = 5.0 * design.hp_cap;      // $/start

    // Power cycle parameters
    double cycle_des_heat = design.cycle_cap / design.cycle_eta;
    e_pb_startup = cycle_des_heat * 0.25; // 15-minutes at design
    q_pb_max = cycle_des_heat;            // Cycle heat input (max)
    q_pb_min = cycle_des_heat * 0.3;      // 30% turn down
    q_pb_startup = cycle_des_heat * 0.25;
    w_pb_max = design.cycle_cap;
    eta_p = (w_pb_max - w_pb_max * 0.15) / (q_pb_max - q_pb_min); // Assuming 15% power at min turndown
    eta_d = 1.0; // this is used to un-normalize the ambient correction -> ignoring at the moment
    q_rc = ((cycle_des_heat) - design.cycle_cap) * 0.5320755; // Assume the 60 MWt default value

    // Cycle initial conditions
    is_pb_starting0 = false;            // Assume cycle is off and not starting up
    is_pb_operating0 = false;
    q_pb0 = 0.0;
    e_pb_start0 = 0.0;

    // minimum up and down time parameters
    min_up_down_p.down_time_min = 2.0;
    min_up_down_p.up_time_min = 2.0;
    min_up_down_p.down_time0 = 10.0;     // Enable cycle to start up in period 1
    min_up_down_p.up_time0 = 0.0;
    min_up_down_p.time_elapsed.clear();
    for (int t = 0; t < n_periods; t++) {
        min_up_down_p.time_elapsed.push_back(delta * (t + 1));
    }

    // Heat pump parameters
    e_hp_startup = design.hp_cap * 0.25;       // MWhe -> 15-minutes at design
    w_hp_max = design.hp_cap;                  // MWe
    w_hp_min = design.hp_cap * 0.3;            // MWe -> Assuming a 30% turndown
    w_hp_startup = design.hp_cap * 0.25;       // MWe
    q_hp_max = design.hp_cap * design.hp_cop;
    kappa_p = (q_hp_max - q_hp_max * 0.15) / (w_hp_max - w_hp_min);   // Assuming 15% heat at min turndown
    q_rh = 0.0;                           // Assuming zero heat rejected during charging

    // Heat pump initial conditions
    is_hp_starting0 = false;            // Assume heat is off and not starting up
    is_hp_operating0 = false;
    w_hp0 = 0.0;
    e_hp_start0 = 0.0;

    // TES parameters
    e_tes_max = design.tes_cap;
    e_tes0 = design.tes_cap * 0.05;     // Assuming 5% full
    e_loss = design.tes_cap * 0.0004;   // Determined from SAM's default PTES case

    // Heat offtaker TES parameters
    e_ot_max = design.tes_cap * 0.1;    // Assuming off-taker storage is 10%
    e_ot0 = e_ot_max * 0.05;
    e_ot_loss = e_ot_max * 0.0004;      // Determined from SAM's default PTES case
    q_tes_max = 60.;                    // TODO: May need to update
}

ptes_chp_dispatch::ptes_chp_dispatch() {

    initializeSolver();
}

void ptes_chp_dispatch::initializeSolver() {
    solver = CreateSolver("SCIP");
    //solver = CreateSolver("SCIP");
    //solver = std::unique_ptr<MPSolver>(MPSolver::CreateSolver("SCIP"));
    if (!solver) {
        LOG(WARNING) << "Solver unavailable.";
        return;
    }
    //LOG(INFO) << solver->SolverVersion();
    //solver->SetNumThreads(4);
    solver->EnableOutput();
    solver->set_time_limit(180*60*1000); //micro-seconds  120*60

    //solver_params.SetIntegerParam(MPSolverParameters::PRESOLVE, MPSolverParameters::PresolveValues::PRESOLVE_OFF);

    solver_params.SetDoubleParam(MPSolverParameters::DoubleParam::RELATIVE_MIP_GAP, 1.e-2); // 1.e-2
    
    //solver_params.SetDoubleParam(MPSolverParameters::DoubleParam::DUAL_TOLERANCE, 1.e-3);
    //solver_params.SetDoubleParam(MPSolverParameters::DoubleParam::PRIMAL_TOLERANCE, 1.e-3);
}

std::unique_ptr<MPSolver> ptes_chp_dispatch::CreateSolver(const std::string& solver_id) {
    return std::unique_ptr<MPSolver>(MPSolver::CreateSolver(solver_id));
}

void ptes_chp_dispatch::createVariables() {
    // Continous variables
    solver->MakeNumVarArray(params.n_periods, 0.0, params.q_pb_max, "q_c", &vars.q_c);
    solver->MakeNumVarArray(params.n_periods, 0.0, params.q_pb_max, "q_delta_c", &vars.q_delta_c);
    solver->MakeNumVarArray(params.n_periods, 0.0, params.q_hp_max, "q_h", &vars.q_h);
    solver->MakeNumVarArray(params.n_periods, 0.0, std::max(params.q_rc, params.q_rh), "q_r", &vars.q_r);
    solver->MakeNumVarArray(params.n_periods, 0.0, params.q_tes_max, "q_tes", &vars.q_tes);
    solver->MakeNumVarArray(params.n_periods, 0.0, MPSolver::infinity(), "q_s", &vars.q_s);
    solver->MakeNumVarArray(params.n_periods, 0.0, params.e_tes_max, "s", &vars.s);
    solver->MakeNumVarArray(params.n_periods, 0.0, config.is_offtaker_tes ? params.e_ot_max : 0.0, "s_ot", &vars.s_ot);
    solver->MakeNumVarArray(params.n_periods, 0.0, MPSolver::infinity(), "u_csu", &vars.u_csu);
    solver->MakeNumVarArray(params.n_periods, 0.0, MPSolver::infinity(), "u_hsu", &vars.u_hsu);
    solver->MakeNumVarArray(params.n_periods, 0.0, params.w_pb_max * 1.2, "w_c", &vars.w_c);
    solver->MakeNumVarArray(params.n_periods, 0.0, params.w_hp_max, "w_delta_h", &vars.w_delta_h);
    solver->MakeNumVarArray(params.n_periods, 0.0, params.w_hp_max, "w_h", &vars.w_h);
    // Binary variables
    solver->MakeBoolVarArray(params.n_periods, "y_c", &bin_vars.y_c);
    solver->MakeBoolVarArray(params.n_periods, "y_cgb", &bin_vars.y_cgb);
    solver->MakeBoolVarArray(params.n_periods, "y_cge", &bin_vars.y_cge);
    solver->MakeBoolVarArray(params.n_periods, "y_csu", &bin_vars.y_csu);
    solver->MakeBoolVarArray(params.n_periods, "y_csup", &bin_vars.y_csup);
    solver->MakeBoolVarArray(params.n_periods, "y_h", &bin_vars.y_h);
    solver->MakeBoolVarArray(params.n_periods, "y_hsu", &bin_vars.y_hsu);
    solver->MakeBoolVarArray(params.n_periods, "y_hsup", &bin_vars.y_hsup);

    LOG(INFO) << "Number of variables = " << solver->NumVariables();
}

void ptes_chp_dispatch::createConstraints() {
    /*==== Charging - Heat Pump ====*/
    // Heat pump startup inventory
    // u_hsu[t] <= u_hsu[t-1] + Delta * W^hsu * y_hsu[t]
    create_startup_inventory_constraint(vars.u_hsu, bin_vars.y_hsu, params.delta * params.w_hp_startup, params.e_hp_start0, "hp_start_inv");
    // Heat pump inventory nonzero
    // uhsu[t] <= Eh * yhsu[t]
    create_inventory_nonzero_constraint(vars.u_hsu, bin_vars.y_hsu, params.e_hp_startup * 1.00001, "hp_inv_nonzero");
    // Heat pump operation allowed when:
    // yh[t] <= uhsu[t] / Eh + yh[t-1]
    create_operation_allowed_constraint(bin_vars.y_h, vars.u_hsu, params.e_hp_startup, params.is_hp_operating0, "hp_op_allowed");
    // Heat pump startup can't be enabled after a time step where it was operating
    // yhsu[t] + yh[t-1] <= 1
    create_startup_wait_constraint(bin_vars.y_hsu, bin_vars.y_h, params.is_hp_operating0, "hp_su_wait");
    // Heat pump minimum power input requirement
    // wh[t] >= Wh{min} * yh[t]
    create_minimum_operation_constraint(vars.w_h, bin_vars.y_h, params.w_hp_min, "hp_min_input_power");
    // Limits the electrical power to the heat pump during periods of startup
    // wh[t] + Whsu * yhsu[t] <= Wh{max} * yh[t]
    create_maximum_operation_wstartup_constraint(vars.w_h, bin_vars.y_hsu, bin_vars.y_h, params.w_hp_startup, params.w_hp_max, "hp_max_input_power");
    // Heat production linearization (heat output as function of power input)
    // qh[t] = Kp * wh[t] + ( Qh{max} - Kp * Wh{max}) * yh[t]
    double intercept = params.q_hp_max - params.kappa_p * params.w_hp_max;
    create_linear_production_constraint(vars.q_h, vars.w_h, bin_vars.y_h, params.kappa_p, intercept, params.eta_amb, "hp_heat_production");
    // Heat pump ramping (positive change)
    // wdeltah[t] >= wh[t] - wh[t-1]
    create_positive_ramping_constraint(vars.w_delta_h, vars.w_h, params.w_hp0, "hp_ramping");
    // Heat pump startup penalty
    // yhsup[t] >= yhsu[t] - yhsu[t-1]
    create_startup_penalty_constraint(bin_vars.y_hsup, bin_vars.y_hsu, params.is_hp_starting0, "hp_startup_penalty");

    /*==== Discharging - Closed Brayton Cycle ====*/
    // Cycle startup inventory
    // ucsu[t] <= ucsu[t-1] + Delta * Q^csu * ycsu[t]
    create_startup_inventory_constraint(vars.u_csu, bin_vars.y_csu, params.delta * params.q_pb_startup, params.e_pb_start0, "pc_start_inv");
    // Cycle inventory nonzero
    // ucsu[t] <= Ec * ycsu[t]
    create_inventory_nonzero_constraint(vars.u_csu, bin_vars.y_csu, params.e_pb_startup * 1.00001, "pc_inv_nonzero");
    // Cycle operation allowed when:
    // yc[t] <= ucsu[t] / Ec + yc[t-1]
    create_operation_allowed_constraint(bin_vars.y_c, vars.u_csu, params.e_pb_startup, params.is_pb_operating0, "pc_op_allowed");
    // Cycle startup can't be enabled after a time step where it was operating
    // ycsu[t] + yc[t-1] <= 1
    create_startup_wait_constraint(bin_vars.y_csu, bin_vars.y_c, params.is_pb_operating0, "pc_su_wait");
    // Cycle minimum heat input requirement
    // qc[t] >= Qc{min} * yc[t]
    create_minimum_operation_constraint(vars.q_c, bin_vars.y_c, params.q_pb_min, "pc_min_input_heat");
    // Limits the thermal power to the cycle during periods of startup
    // qc[t] + Qcsu * ycsu[t] <= Qc{max} * yc[t]
    create_maximum_operation_wstartup_constraint(vars.q_c, bin_vars.y_csu, bin_vars.y_c, params.q_pb_startup, params.q_pb_max, "pc_max_input_heat");
    // Power production linearization (power output as function of heat input)
    // wc[t] = eta^amb[t] * (etap * qc[t] + ( Wc{max} - etap * Qc{max}) * yc[t])
    intercept = params.w_pb_max - params.eta_p * params.q_pb_max;
    create_linear_production_constraint(vars.w_c, vars.q_c, bin_vars.y_c, params.eta_p, intercept, params.eta_amb, "hp_heat_production");
    // Cycle ramping (positive change)
    // qdeltac[t] >= qc[t] - qc[t-1]
    create_positive_ramping_constraint(vars.q_delta_c, vars.q_c, params.q_pb0, "pc_ramping");
    // Cycle startup penalty
    // ycsup[t] >= ycsu[t] - ycsu[t-1]
    create_startup_penalty_constraint(bin_vars.y_csup, bin_vars.y_csu, params.is_pb_starting0, "pc_startup_penalty");
    // Charging and discharging cannot coincide
    // yh[t] + yc[t] <= 1
    create_set_packing_constraint(bin_vars.y_h, bin_vars.y_c, "char_dischar_pack");
    // Minimum up- and down- constraints
    // ycgb[t] - ycge[t] = y_c[t] - y_c[t-1]
    // sum{tp in Tau : 0 <= deltaE[t] - deltaE[tp] < Yu} ycgb[tp] <= y[t] forall t in Tau : deltaE[t] > (Yu-Yu0)*y0
    // sum{tp in Tau : 0 <= deltaE[t] - deltaE[tp] < Yd} ycge[tp] <= 1 - y[t] forall t in Tau : deltaE[t] > (Yd-Yd0)*(1-y0)
    // y_c[t] = y0 forall t in Tau : deltaE[t] <= max{ (Yu - Yu0) * y0 , (Yd - Yd0) * (1 - y0) }
    create_min_up_down_constraints(bin_vars.y_c, bin_vars.y_cgb, bin_vars.y_cge, params.is_pb_operating0, params.min_up_down_p, "");

    /*==== Thermal Energy Storage ====*/
    // Storage energy balance
    // s[t] - s[t-1] = Delta * ( qh[t] - qc[t] - Qcsu * ycsu[t]) - Qloss
    // if heat demand can be met with PTES hot TES
    // s[t] - s[t-1] = Delta * ( qh[t] - qc[t] - Qcsu * ycsu[t] - q_tes[t]) - E_loss
    for (int t = 0; t < params.n_periods; ++t) { // For all time periods
        MPConstraint* constraint;
        std::string name = "tes_balance[" + std::to_string(t) + "]";
        if (t == 0) {
            constraint = solver->MakeRowConstraint(params.e_tes0 - params.e_loss, params.e_tes0 - params.e_loss, name);
        }
        else {
            constraint = solver->MakeRowConstraint(-params.e_loss, -params.e_loss, name);
            constraint->SetCoefficient(vars.s[t - 1], -1.0);
        }
        constraint->SetCoefficient(vars.s[t], 1.0);
        constraint->SetCoefficient(vars.q_h[t], -params.delta);
        constraint->SetCoefficient(vars.q_c[t], params.delta);
        constraint->SetCoefficient(bin_vars.y_csu[t], params.delta * params.q_pb_startup);
        if (config.is_heat_from_tes_allowed) constraint->SetCoefficient(vars.q_tes[t], params.delta);
    }

    /*==== Heat Off-taker ====*/
    // Rejected heat occurs during charging and/or discharging
    // q_r[t] <= Qrh * y_h[t] + Qrc * y_c[t]
    // TODO: Partload...
    // q_r[t] <= Qrh / Qh{max} * q_h[t] + Qrc / Qc{max} * q_c[t]
    for (int t = 0; t < params.n_periods; ++t) { // For all time periods
        MPConstraint* constraint;
        std::string name = "heat_reject[" + std::to_string(t) + "]";
        constraint = solver->MakeRowConstraint(-MPSolver::infinity(), 0.0, name);
        constraint->SetCoefficient(vars.q_r[t], 1.0);
        if (config.is_charge_heat_reject) constraint->SetCoefficient(bin_vars.y_h[t], -params.q_rh);
        if (config.is_discharge_heat_reject) constraint->SetCoefficient(bin_vars.y_c[t], -params.q_rc);
        //if (is_charge_heat_reject) constraint->SetCoefficient(vars.q_h[t], -params.q_rh / params.q_hp_max);
        //if (is_discharge_heat_reject) constraint->SetCoefficient(vars.q_c[t], -params.q_rc / params.q_pb_max);
    }

    // Heat off-taker demand must be met
    // q_s[t] == Q_d[t]
    //     *** OR ***
    // Sold heat must be less than heat off-taker demand
    // q_s[t] <= Q_d[t]
    for (int t = 0; t < params.n_periods; ++t) { // For all time periods
        MPConstraint* constraint;
        std::string name = "heat_demand[" + std::to_string(t) + "]";
        constraint = solver->MakeRowConstraint(config.is_heat_demand_required ? params.heat_demand[t] : -MPSolver::infinity(), params.heat_demand[t], name);
        constraint->SetCoefficient(vars.q_s[t], 1.0);
    }

    if (!config.is_offtaker_tes) { 
        params.e_ot0 = 0.0; // Must be set to zero when not available
        params.e_ot_loss = 0.0;
    }

    // Heat off-taker storage energy balance
    // s_ot[t] - s_ot[t-1] = Delta * ( q_r[t] - q_s[t]) - Eloss
    // if heat demand can be met with PTES hot TES
    // s_ot[t] - s_ot[t-1] = Delta * ( q_r[t] + q_tes[t] - q_s[t]) - E_ot_loss
    for (int t = 0; t < params.n_periods; ++t) { // For all time periods
        MPConstraint* constraint;
        std::string name = "off_taker_tes_balance[" + std::to_string(t) + "]";
        if (t == 0) {
            constraint = solver->MakeRowConstraint(params.e_ot0 - params.e_ot_loss, params.e_ot0 - params.e_ot_loss, name);
        }
        else {
            constraint = solver->MakeRowConstraint(-params.e_ot_loss, -params.e_ot_loss, name);
            constraint->SetCoefficient(vars.s_ot[t - 1], -1.0);
        }
        constraint->SetCoefficient(vars.s_ot[t], 1.0);
        constraint->SetCoefficient(vars.q_r[t], -params.delta);
        if (config.is_heat_from_tes_allowed) constraint->SetCoefficient(vars.q_tes[t], -params.delta);
        constraint->SetCoefficient(vars.q_s[t], params.delta);
    }

    LOG(INFO) << "Number of constraints = " << solver->NumConstraints();
}

void ptes_chp_dispatch::createObjectiveFunction() {
    /*==== Objective Function ====*/
    objective = solver->MutableObjective();
    double common_coeff;
    for (int t = 0; t < params.n_periods; ++t) { // For all time periods
        common_coeff = params.delta * std::pow(params.time_weighting, t);
        objective->SetCoefficient(vars.w_c[t], common_coeff * params.sell_price[t]);
        if (config.is_heat_valued) {
            objective->SetCoefficient(vars.q_s[t], common_coeff * params.heat_sell_price[t]);
        }
        common_coeff = -params.delta * params.purchase_price[t] * (1. / std::pow(params.time_weighting, t));
        objective->SetCoefficient(vars.w_h[t], common_coeff);
        common_coeff *= params.w_hp_startup;
        objective->SetCoefficient(bin_vars.y_hsu[t], common_coeff);
        // Cost terms
        common_coeff = -(1. / std::pow(params.time_weighting, t));
        objective->SetCoefficient(bin_vars.y_csup[t], common_coeff * params.cycle_start_cost);
        objective->SetCoefficient(vars.q_delta_c[t], common_coeff * params.cycle_ramping_cost);
        objective->SetCoefficient(bin_vars.y_hsup[t], common_coeff * params.heat_pump_start_cost);
        objective->SetCoefficient(vars.w_delta_h[t], common_coeff * params.heat_pump_ramp_cost);
    }
    objective->SetMaximization();
}

double ptes_chp_dispatch::optimize(std::string resultsfilepath) {
    // Create variables
    createVariables(); // This could be moved to the constructor class

    // Create the constraints
    createConstraints();

    // Create the objective function
    createObjectiveFunction();

    //solver->EnableOutput();
    std::clock_t c_start = std::clock();
    MPSolver::ResultStatus result_status = solver->Solve(solver_params);
    absl::Duration wall_time = solver->DurationSinceConstruction();
    std::clock_t c_end = std::clock();
    double time_elapsed_sec = (c_end - c_start) / CLOCKS_PER_SEC;

    // Check that the problem has an optimal solution.
    if (result_status != MPSolver::OPTIMAL) {  // How to handle infeasibilities 
        //LOG(FATAL) << "The problem does not have an optimal solution.";
        solver->Clear();
        vars.Clear();
        bin_vars.Clear();
        return 0.0;
    }

    printVariableValuesToConsole(wall_time);
    printResultsFile(resultsfilepath);

    // Must clear solver and variables before next solve
    solver->Clear();
    vars.Clear();
    bin_vars.Clear();

    return time_elapsed_sec;
}

double ptes_chp_dispatch::rollingHorizonoptimize(int opt_horizon, int roll_horizon, std::string resultsfilepath) {
    int original_horizon = params.n_periods;

    // Save full vectors
    std::vector<double> f_eta_amb = params.eta_amb;
    std::vector<double> f_heat_sell_price = params.heat_sell_price;
    std::vector<double> f_purchase_price = params.purchase_price;
    std::vector<double> f_sell_price = params.sell_price;
    std::vector<double> f_heat_demand = params.heat_demand;

    // for loop to set up model, solve, and save results
    double n_solves = (int)std::ceil(params.n_periods / roll_horizon);

    params.min_up_down_p.time_elapsed.clear();
    for (int t = 0; t < opt_horizon; t++) {
        params.min_up_down_p.time_elapsed.push_back(params.delta * (t + 1));
    }

    std::clock_t c_start = std::clock();
    for (int s = 0; s < n_solves; ++s) {
        int sidx = (int)(s * roll_horizon);     // window start
        //int eidx = (int)(sidx + opt_horizon);   // window end
        params.n_periods = opt_horizon;
        // update parameter vectors
        params.eta_amb.resize(params.n_periods);
        params.heat_sell_price.resize(params.n_periods);
        params.purchase_price.resize(params.n_periods);
        params.sell_price.resize(params.n_periods);
        params.heat_demand.resize(params.n_periods);
        for (int i = 0; i < params.n_periods; ++i) {
            int original_idx = sidx + i;
            if (original_idx >= original_horizon) original_idx -= original_horizon; // start back at the begining
            params.eta_amb[i] = f_eta_amb[original_idx];
            params.heat_sell_price[i] = f_heat_sell_price[original_idx];
            params.purchase_price[i] = f_purchase_price[original_idx];
            params.sell_price[i] = f_sell_price[original_idx];
            params.heat_demand[i] = f_heat_demand[original_idx];
        }

        // create problem and solve
        LOG(INFO) << " Solving Optimization Horizon: " << s << std::endl;
        createVariables();
        createConstraints();
        createObjectiveFunction();
        MPSolver::ResultStatus result_status = solver->Solve(solver_params);
        updateInitialConditions(roll_horizon - 1);
        //change n_periods so result file loop works
        params.n_periods = roll_horizon;
        if (s == 0)
            printResultsFile(resultsfilepath); // create file with header
        else
            printResultsFile(resultsfilepath, true); // append results

        // Must clear solver and variables before next solve
        solver->Clear();
        vars.Clear();
        bin_vars.Clear();
    }

    std::clock_t c_end = std::clock();
    double time_elapsed_sec = (c_end - c_start) / CLOCKS_PER_SEC;
    LOG(INFO) << "CPU time used: " << time_elapsed_sec << " sec" << std::endl;

    // Return class back to original state
    params.n_periods = original_horizon;
    params.eta_amb = f_eta_amb;
    params.heat_sell_price = f_heat_sell_price;
    params.purchase_price = f_purchase_price;
    params.sell_price = f_sell_price;
    params.heat_demand = f_heat_demand;

    return time_elapsed_sec;
}

void ptes_chp_dispatch::updateInitialConditions(int init_t) {
    // Cycle
    params.is_pb_starting0 = bin_vars.y_csu[init_t]->solution_value() ? true : false;
    params.is_pb_operating0 = bin_vars.y_c[init_t]->solution_value() ? true : false;
    params.q_pb0 = params.is_pb_operating0 ? vars.q_c[init_t]->solution_value() : 0.0;
    params.e_pb_start0 = params.is_pb_starting0 ? vars.u_csu[init_t]->solution_value() : 0.0; 
    // Up time and down time
    params.min_up_down_p.time_elapsed.clear();
    for (int t = 0; t < params.n_periods; t++) {
        params.min_up_down_p.time_elapsed.push_back(params.delta * (t + 1));
    }
    double init_up_time = 0.0;
    double init_down_time = 0.0;
    if (params.is_pb_operating0) { // cycle is up
        for (int t = init_t; t > 0; t--) {
            if (bin_vars.y_c[t]->solution_value()) {
                init_up_time += params.delta;
            }
            else break;
        }
    }
    else {
        for (int t = init_t; t > 0; t--) {
            if (!bin_vars.y_c[t]->solution_value()) {
                init_down_time += params.delta;
            }
            else break;
        }
    }
    params.min_up_down_p.up_time0 = init_up_time;
    params.min_up_down_p.down_time0 = init_down_time;
    // Heat pump
    params.is_hp_starting0 = bin_vars.y_hsu[init_t]->solution_value() ? true : false;
    params.is_hp_operating0 = bin_vars.y_h[init_t]->solution_value() ? true : false;
    params.w_hp0 = params.is_hp_operating0 ? vars.w_h[init_t]->solution_value() : 0.0;
    params.e_hp_start0 = params.is_hp_starting0 ? vars.u_hsu[init_t]->solution_value() : 0.0;
    // Storage 
    params.e_tes0 = std::max(vars.s[init_t]->solution_value(), 0.0);
    // Heat off-taker Storage
    params.e_ot0 = std::max(vars.s_ot[init_t]->solution_value(), 0.0);
}

void ptes_chp_dispatch::printVariableValuesToConsole(absl::Duration wall_time) {
    LOG(INFO) << "Solution:" << std::endl;
    LOG(INFO) << "Objective value = " << objective->Value() << std::endl;
    LOG(INFO) << "Interations = " << solver->iterations() << std::endl;
    LOG(INFO) << "Wall Time = " << wall_time << std::endl;
    LOG(INFO) << "==================================================================" << std::endl;
    LOG(INFO) << "                       Cycle Variable Values                      " << std::endl;
    LOG(INFO) << "==================================================================" << std::endl;
    int col_width = 10;
    double tol = 1.e-3;
    LOG(INFO) << std::setfill(' ') << std::left << std::setw(6) << "Time"
        << std::left << std::setw(col_width) << "price"
        << std::left << std::setw(col_width) << "y_csup"
        << std::left << std::setw(col_width) << "y_csu"
        << std::left << std::setw(col_width) << "u_csu"
        << std::left << std::setw(col_width) << "y_c"
        << std::left << std::setw(col_width) << "w_c"
        << std::left << std::setw(col_width) << "q_c"
        << std::left << std::setw(col_width) << "q_delta_c"
        << std::left << std::setw(col_width) << "y_cgb"
        << std::left << std::setw(col_width) << "y_cge"
        << std::endl;
    for (int t = 0; t < std::min(params.n_periods, 48); ++t) {
        LOG(INFO) << std::setfill(' ') << std::left << std::setw(6) << t
            << std::left << std::setw(col_width) << params.sell_price[t]
            << std::left << std::setw(col_width) << (bin_vars.y_csup[t]->solution_value() ? 1 : 0)
            << std::left << std::setw(col_width) << (bin_vars.y_csu[t]->solution_value() ? 1 : 0)
            << std::left << std::setw(col_width) << ((std::abs(vars.u_csu[t]->solution_value()) < tol) ? 0.0 : vars.u_csu[t]->solution_value())
            << std::left << std::setw(col_width) << (bin_vars.y_c[t]->solution_value() ? 1 : 0)
            << std::left << std::setw(col_width) << ((std::abs(vars.w_c[t]->solution_value()) < tol) ? 0.0 : vars.w_c[t]->solution_value())
            << std::left << std::setw(col_width) << ((std::abs(vars.q_c[t]->solution_value()) < tol) ? 0.0 : vars.q_c[t]->solution_value())
            << std::left << std::setw(col_width) << ((std::abs(vars.q_delta_c[t]->solution_value()) < tol) ? 0.0 : vars.q_delta_c[t]->solution_value())
            << std::left << std::setw(col_width) << (bin_vars.y_cgb[t]->solution_value() ? 1 : 0)
            << std::left << std::setw(col_width) << (bin_vars.y_cge[t]->solution_value() ? 1 : 0)
            << std::endl;
    }
    LOG(INFO) << "==================================================================" << std::endl;
    LOG(INFO) << "                     Heat Pump Variable Values                    " << std::endl;
    LOG(INFO) << "==================================================================" << std::endl;
    LOG(INFO) << std::setfill(' ') << std::left << std::setw(6) << "Time"
        << std::left << std::setw(col_width) << "s"
        << std::left << std::setw(col_width) << "y_hsup"
        << std::left << std::setw(col_width) << "y_hsu"
        << std::left << std::setw(col_width) << "u_hsu"
        << std::left << std::setw(col_width) << "y_h"
        << std::left << std::setw(col_width) << "w_h"
        << std::left << std::setw(col_width) << "q_h"
        << std::left << std::setw(col_width) << "w_delta_h"
        << std::endl;
    for (int t = 0; t < std::min(params.n_periods, 48); ++t) {
        LOG(INFO) << std::setfill(' ') << std::left << std::setw(6) << t
            << std::left << std::setw(col_width) << ((std::abs(vars.s[t]->solution_value()) < tol) ? 0.0 : vars.s[t]->solution_value())
            << std::left << std::setw(col_width) << (bin_vars.y_hsup[t]->solution_value() ? 1 : 0)
            << std::left << std::setw(col_width) << (bin_vars.y_hsu[t]->solution_value() ? 1 : 0)
            << std::left << std::setw(col_width) << ((std::abs(vars.u_hsu[t]->solution_value()) < tol) ? 0.0 : vars.u_hsu[t]->solution_value())
            << std::left << std::setw(col_width) << (bin_vars.y_h[t]->solution_value() ? 1 : 0)
            << std::left << std::setw(col_width) << ((std::abs(vars.w_h[t]->solution_value()) < tol) ? 0.0 : vars.w_h[t]->solution_value())
            << std::left << std::setw(col_width) << ((std::abs(vars.q_h[t]->solution_value()) < tol) ? 0.0 : vars.q_h[t]->solution_value())
            << std::left << std::setw(col_width) << ((std::abs(vars.w_delta_h[t]->solution_value()) < tol) ? 0.0 : vars.w_delta_h[t]->solution_value())
            << std::endl;
    }
    LOG(INFO) << "==================================================================" << std::endl;
    LOG(INFO) << "                  Heat Off-Taker Variable Values                  " << std::endl;
    LOG(INFO) << "==================================================================" << std::endl;
    LOG(INFO) << std::setfill(' ') << std::left << std::setw(6) << "Time"
        << std::left << std::setw(col_width) << "heat_P"
        << std::left << std::setw(col_width) << "ht_dem"
        << std::left << std::setw(col_width) << "s_ot"
        << std::left << std::setw(col_width) << "q_s"
        << std::left << std::setw(col_width) << "q_r"
        << std::left << std::setw(col_width) << "q_tes"
        << std::endl;
    for (int t = 0; t < std::min(params.n_periods, 48); ++t) {
        LOG(INFO) << std::setfill(' ') << std::left << std::setw(6) << t
            << std::left << std::setw(col_width) << params.heat_sell_price[t]
            << std::left << std::setw(col_width) << params.heat_demand[t]
            << std::left << std::setw(col_width) << ((std::abs(vars.s_ot[t]->solution_value()) < tol) ? 0.0 : vars.s_ot[t]->solution_value())
            << std::left << std::setw(col_width) << ((std::abs(vars.q_s[t]->solution_value()) < tol) ? 0.0 : vars.q_s[t]->solution_value())
            << std::left << std::setw(col_width) << ((std::abs(vars.q_r[t]->solution_value()) < tol) ? 0.0 : vars.q_r[t]->solution_value())
            << std::left << std::setw(col_width) << ((std::abs(vars.q_tes[t]->solution_value()) < tol) ? 0.0 : vars.q_tes[t]->solution_value())
            << std::endl;
    }
}

void ptes_chp_dispatch::printResultsFile(std::string filepath, bool append) {
    std::ofstream outputfile;
    if (append)
        outputfile.open(filepath, std::ios_base::app);
    else {
        // Only print header when not appending
        outputfile.open(filepath);
        std::string var_header = "Time, Objective, Objective_wo_weight, Price, y_csup, y_csu, u_csu, y_c, w_c, q_c, q_delta_c, y_cgb, y_cge, "
            "s, y_hsup, y_hsu, u_hsu, y_h, w_h, q_h, w_delta_h, "
            "heat_price, heat_demand, s_ot, q_s, q_r, q_tes";
        outputfile << var_header << std::endl;
    }

    double tol = 1.e3;
    double objective_value;
    double common_coeff;

    double obj_wo_weight;  // Objective function value without time weighting
    double coeff_wo_weight;
    for (int t = 0; t < params.n_periods; ++t) {
        // Calculate objective value for time t
        objective_value = 0.0;
        obj_wo_weight = 0.0;

        common_coeff = params.delta * std::pow(params.time_weighting, t);
        coeff_wo_weight = params.delta;
        objective_value += common_coeff * params.sell_price[t] * vars.w_c[t]->solution_value();
        obj_wo_weight += coeff_wo_weight * params.sell_price[t] * vars.w_c[t]->solution_value();

        if (config.is_heat_valued) {
            objective_value += common_coeff * params.heat_sell_price[t] * vars.q_s[t]->solution_value();
            obj_wo_weight += coeff_wo_weight * params.heat_sell_price[t] * vars.q_s[t]->solution_value();
        }

        common_coeff = -params.delta * params.purchase_price[t] * (1. / std::pow(params.time_weighting, t));
        coeff_wo_weight = -params.delta * params.purchase_price[t];
        objective_value += common_coeff * vars.w_h[t]->solution_value();
        obj_wo_weight += coeff_wo_weight * vars.w_h[t]->solution_value();

        common_coeff *= params.w_hp_startup;
        coeff_wo_weight *= params.w_hp_startup;
        objective_value += common_coeff * bin_vars.y_hsu[t]->solution_value();
        obj_wo_weight += coeff_wo_weight * bin_vars.y_hsu[t]->solution_value();

        // Cost terms
        common_coeff = -(1. / std::pow(params.time_weighting, t));
        coeff_wo_weight = -1.;

        objective_value += common_coeff * params.cycle_start_cost * bin_vars.y_csup[t]->solution_value();
        objective_value += common_coeff * params.cycle_ramping_cost * vars.q_delta_c[t]->solution_value();
        objective_value += common_coeff * params.heat_pump_start_cost * bin_vars.y_hsup[t]->solution_value();
        objective_value += common_coeff * params.heat_pump_ramp_cost * vars.w_delta_h[t]->solution_value();

        obj_wo_weight += coeff_wo_weight * params.cycle_start_cost * bin_vars.y_csup[t]->solution_value();
        obj_wo_weight += coeff_wo_weight * params.cycle_ramping_cost * vars.q_delta_c[t]->solution_value();
        obj_wo_weight += coeff_wo_weight * params.heat_pump_start_cost * bin_vars.y_hsup[t]->solution_value();
        obj_wo_weight += coeff_wo_weight * params.heat_pump_ramp_cost * vars.w_delta_h[t]->solution_value();

        outputfile << t
            << ", " << objective_value
            << ", " << obj_wo_weight
            << ", " << params.sell_price[t]
            << ", " << bin_vars.y_csup[t]->solution_value()
            << ", " << bin_vars.y_csu[t]->solution_value()
            << ", " << ((std::abs(vars.u_csu[t]->solution_value()) < tol) ? 0.0 : vars.u_csu[t]->solution_value())
            << ", " << bin_vars.y_c[t]->solution_value()
            << ", " << ((std::abs(vars.w_c[t]->solution_value()) < tol) ? 0.0 : vars.w_c[t]->solution_value())
            << ", " << ((std::abs(vars.q_c[t]->solution_value()) < tol) ? 0.0 : vars.q_c[t]->solution_value())
            << ", " << ((std::abs(vars.q_delta_c[t]->solution_value()) < tol) ? 0.0 : vars.q_delta_c[t]->solution_value())
            << ", " << bin_vars.y_cgb[t]->solution_value()
            << ", " << bin_vars.y_cge[t]->solution_value()
            << ", " << ((std::abs(vars.s[t]->solution_value()) < tol) ? 0.0 : vars.s[t]->solution_value())
            << ", " << bin_vars.y_hsup[t]->solution_value()
            << ", " << bin_vars.y_hsu[t]->solution_value()
            << ", " << ((std::abs(vars.u_hsu[t]->solution_value()) < tol) ? 0.0 : vars.u_hsu[t]->solution_value())
            << ", " << bin_vars.y_h[t]->solution_value()
            << ", " << ((std::abs(vars.w_h[t]->solution_value()) < tol) ? 0.0 : vars.w_h[t]->solution_value())
            << ", " << ((std::abs(vars.q_h[t]->solution_value()) < tol) ? 0.0 : vars.q_h[t]->solution_value())
            << ", " << ((std::abs(vars.w_delta_h[t]->solution_value()) < tol) ? 0.0 : vars.w_delta_h[t]->solution_value())
            << ", " << params.heat_sell_price[t]
            << ", " << params.heat_demand[t]
            << ", " << ((std::abs(vars.s_ot[t]->solution_value()) < tol) ? 0.0 : vars.s_ot[t]->solution_value())
            << ", " << ((std::abs(vars.q_s[t]->solution_value()) < tol) ? 0.0 : vars.q_s[t]->solution_value())
            << ", " << ((std::abs(vars.q_r[t]->solution_value()) < tol) ? 0.0 : vars.q_r[t]->solution_value())
            << ", " << ((std::abs(vars.q_tes[t]->solution_value()) < tol) ? 0.0 : vars.q_tes[t]->solution_value())
            << std::endl;
    }
    outputfile.close();
}

void ptes_chp_dispatch::create_startup_inventory_constraint(std::vector<MPVariable*> su_inv, std::vector<MPVariable*> su_binary, 
    double inv_roc, double init_su_inv, std::string name_pre) {
    // Start-up inventory constraint for all time periods (0 to n_periods-1)
    // General Form: su_inv[t] <= su_inv[t-1] + inv_roc * su_binary[t]
    // if (t==0): su_inv[t] <= init_su_inv + inv_roc * su_binary[t]
    MPConstraint* constraint;
    for (int t = 0; t < params.n_periods; ++t) { // For all time periods
        std::string name = name_pre + "[" + std::to_string(t) + "]";
        if (t == 0) {
            constraint = solver->MakeRowConstraint(-MPSolver::infinity(), init_su_inv, name);
        }
        else {
            constraint = solver->MakeRowConstraint(-MPSolver::infinity(), 0.0, name);
            constraint->SetCoefficient(su_inv[t - 1], -1.0);
        }
        constraint->SetCoefficient(su_inv[t], 1.0);
        constraint->SetCoefficient(su_binary[t], -inv_roc);
    }
}

void ptes_chp_dispatch::create_inventory_nonzero_constraint(std::vector<MPVariable*> su_inv, std::vector<MPVariable*> su_binary,
    double su_inv_req, std::string name_pre) {
    // Start-up inventory nonzero constraint for all time periods (0 to n_periods-1)
    // General Form: su_inv[t] <= su_inv_req * su_binary[t]
    MPConstraint* constraint;
    for (int t = 0; t < params.n_periods; ++t) { // For all time periods
        std::string name = name_pre + "[" + std::to_string(t) + "]";
        constraint = solver->MakeRowConstraint(-MPSolver::infinity(), 0.0, name);
        constraint->SetCoefficient(su_inv[t], 1.0);
        constraint->SetCoefficient(su_binary[t], -su_inv_req);
    }
}

void ptes_chp_dispatch::create_operation_allowed_constraint(std::vector<MPVariable*> op_binary, std::vector<MPVariable*> su_inv,
    double su_inv_req, int prev_op_state, std::string name_pre) {
    // Operation allowing when start-up inventory is fulfilled or previously operating for all time periods (0 to n_periods-1)
    // General Form: op_binary[t] <= su_inv[t] / su_inv_req + op_binary[t-1]
    // if (t==0): op_binary[t] <= su_inv[t] / su_inv_req  + prev_op_state
    MPConstraint* constraint;
    for (int t = 0; t < params.n_periods; ++t) { // For all time periods
        std::string name = name_pre + "[" + std::to_string(t) + "]";
        if (t == 0) {
            constraint = solver->MakeRowConstraint(-MPSolver::infinity(), su_inv_req * prev_op_state, name);
        }
        else {
            constraint = solver->MakeRowConstraint(-MPSolver::infinity(), 0.0, name);
            constraint->SetCoefficient(op_binary[t - 1], -su_inv_req);
        }
        constraint->SetCoefficient(op_binary[t], su_inv_req);
        constraint->SetCoefficient(su_inv[t], -1.0);
    }
}

void ptes_chp_dispatch::create_startup_wait_constraint(std::vector<MPVariable*> su_binary, std::vector<MPVariable*> op_binary, 
    int prev_op_state, std::string name_pre) {
    // Startup cannot be enabled after a time period where the device was operating for all time periods (0 to n_periods-1)
    // General Form: su_binary[t] + op_binary[t-1] <= 1
    // if (t==0): su_binary[t] + prev_op_state <= 1
    MPConstraint* constraint;
    for (int t = 0; t < params.n_periods; ++t) { // For all time periods
        std::string name = name_pre + "[" + std::to_string(t) + "]";
        if (t == 0) {
            constraint = solver->MakeRowConstraint(-MPSolver::infinity(), 1.0 - prev_op_state, name);
            // NOTE: for some reason range constraints are not working for Xpress... need to check the basic example
        }
        else {
            constraint = solver->MakeRowConstraint(-MPSolver::infinity(), 1.0, name);
            constraint->SetCoefficient(op_binary[t - 1], 1.0);
        }
        constraint->SetCoefficient(su_binary[t], 1.0);
    }
}

void ptes_chp_dispatch::create_minimum_operation_constraint(std::vector<MPVariable*> op_lvl, std::vector<MPVariable*> op_binary,
    double min_limit, std::string name_pre) {
    // Enforce minimum operation limit when system is operating for all time periods (0 to n_periods-1)
    // General Form: op_lvl[t] >= min_limit * op_binary[t]
    MPConstraint* constraint;
    for (int t = 0; t < params.n_periods; ++t) { // For all time periods
        std::string name = name_pre + "[" + std::to_string(t) + "]";
        constraint = solver->MakeRowConstraint(0.0, MPSolver::infinity(), name);
        constraint->SetCoefficient(op_lvl[t], 1.0);
        constraint->SetCoefficient(op_binary[t], -min_limit);
    }
}

void ptes_chp_dispatch::create_maximum_operation_wstartup_constraint(std::vector<MPVariable*> op_lvl, std::vector<MPVariable*> su_binary, std::vector<MPVariable*> op_binary,
    double su_rate, double max_limit, std::string name_pre) {
    // Enforce maximum operation limit when system is operating and if starting up for all time periods (0 to n_periods-1)
    // General Form: op_lvl[t] + su_rate * su_binary[t] <= max_limit * op_binary[t]
    MPConstraint* constraint;
    for (int t = 0; t < params.n_periods; ++t) { // For all time periods
        std::string name = name_pre + "[" + std::to_string(t) + "]";
        constraint = solver->MakeRowConstraint(-MPSolver::infinity(), 0.0, name);
        constraint->SetCoefficient(op_lvl[t], 1.0);
        constraint->SetCoefficient(su_binary[t], su_rate);
        constraint->SetCoefficient(op_binary[t], -max_limit);
    }
}

// Currently not being used
void ptes_chp_dispatch::create_maximum_operation_constraint(std::vector<MPVariable*> op_lvl, std::vector<MPVariable*> op_binary,
    double max_limit, std::string name_pre) {
    // Enforce maximum operation limit when system is operating for all time periods (0 to n_periods-1)
    // General Form: op_lvl[t] <= max_limit * op_binary[t]
    MPConstraint* constraint;
    for (int t = 0; t < params.n_periods; ++t) { // For all time periods
        std::string name = name_pre + "[" + std::to_string(t) + "]";
        constraint = solver->MakeRowConstraint(-MPSolver::infinity(), 0.0, name);
        constraint->SetCoefficient(op_lvl[t], 1.0);
        constraint->SetCoefficient(op_binary[t], -max_limit);
    }
}

void ptes_chp_dispatch::create_linear_production_constraint(std::vector<MPVariable*> output, std::vector<MPVariable*> input, std::vector<MPVariable*> op_binary,
    double slope, double intercept, std::vector<double> amb_corr, std::string name_pre) {
    // Model output production as a linear relationship of input for all time periods (0 to n_periods-1)
    // General Form: output[t] = amb_corr[t] * ( slope * input[t] + intercept * op_binary[t])
    MPConstraint* constraint;
    for (int t = 0; t < params.n_periods; ++t) { // For all time periods
        std::string name = name_pre + "[" + std::to_string(t) + "]";
        constraint = solver->MakeRowConstraint(0.0, 0.0, name);
        constraint->SetCoefficient(output[t], 1.0);
        constraint->SetCoefficient(input[t], -amb_corr[t] * slope);
        constraint->SetCoefficient(op_binary[t], -amb_corr[t] * intercept);
    }
}

void ptes_chp_dispatch::create_positive_ramping_constraint(std::vector<MPVariable*> ramping, std::vector<MPVariable*> op_lvl,
    double prev_op_rate, std::string name_pre) {
    // Calculates positive ramping for all time periods (0 to n_periods-1)
    // General Form: ramping[t] >= op_lvl[t] - op_lvl[t-1]
    // if (t==0): ramping[t] >= op_lvl[t] - prev_op_rate
    MPConstraint* constraint;
    for (int t = 0; t < params.n_periods; ++t) { // For all time periods
        std::string name = name_pre + "[" + std::to_string(t) + "]";
        if (t == 0) {
            constraint = solver->MakeRowConstraint(-prev_op_rate, MPSolver::infinity(), name);
        }
        else {
            constraint = solver->MakeRowConstraint(0.0, MPSolver::infinity(), name);
            constraint->SetCoefficient(op_lvl[t - 1], 1.0);
        }
        constraint->SetCoefficient(ramping[t], 1.0);
        constraint->SetCoefficient(op_lvl[t], -1.0);
    }
}

void ptes_chp_dispatch::create_startup_penalty_constraint(std::vector<MPVariable*> penalty_binary, std::vector<MPVariable*> su_binary,
    double prev_su_state, std::string name_pre) {
    // Enforces startup penalty for all time periods (0 to n_periods-1)
    // General Form: penalty[t] >= su_binary[t] - su_binary[t-1]
    // if (t==0): penalty_binary[t] >= su_binary[t] - prev_su_state
    MPConstraint* constraint;
    for (int t = 0; t < params.n_periods; ++t) { // For all time periods
        std::string name = name_pre + "[" + std::to_string(t) + "]";
        if (t == 0) {
            constraint = solver->MakeRowConstraint(-prev_su_state, MPSolver::infinity(), name);
        }
        else {
            constraint = solver->MakeRowConstraint(0.0, MPSolver::infinity(), name);
            constraint->SetCoefficient(su_binary[t - 1], 1.0);
        }
        constraint->SetCoefficient(penalty_binary[t], 1.0);
        constraint->SetCoefficient(su_binary[t], -1.0);
    }
}

void ptes_chp_dispatch::create_set_packing_constraint(std::vector<MPVariable*> binary1, std::vector<MPVariable*> binary2, std::string name_pre) {
    // Enforces set packing constraint between two binaries variables for all time periods (0 to n_periods-1)
    // General Form: binary1[t] + binary2[t] <= 1
    MPConstraint* constraint;
    for (int t = 0; t < params.n_periods; ++t) { // For all time periods
        std::string name = name_pre + "[" + std::to_string(t) + "]";
        constraint = solver->MakeRowConstraint(-MPSolver::infinity(), 1.0, name);
        constraint->SetCoefficient(binary1[t], 1.0);
        constraint->SetCoefficient(binary2[t], 1.0);
    }
}

void ptes_chp_dispatch::create_min_up_down_logic_constraint(std::vector<MPVariable*> op_binary, std::vector<MPVariable*> up_binary, std::vector<MPVariable*> down_binary,
    int prev_op_state, std::string name_pre) {
    // Binary logic when switching state for tracking up- and down-time for all time periods (0 to n_periods-1)
    // General Form: up_binary[t] - down_binary[t] = op_binary[t] - op_binary[t-1]
    // if (t==0): up_binary[t] - down_binary[t] = op_binary[t] - prev_op_state
    for (int t = 0; t < params.n_periods; t++) { // For all time periods
        MPConstraint* constraint;
        std::string name = name_pre + "[" + std::to_string(t) + "]";
        if (t == 0) {
            constraint = solver->MakeRowConstraint(-prev_op_state, -prev_op_state, name);
        }
        else {
            constraint = solver->MakeRowConstraint(0.0, 0.0, name);
            constraint->SetCoefficient(op_binary[t - 1], 1.0);
        }
        constraint->SetCoefficient(up_binary[t], 1.0);
        constraint->SetCoefficient(down_binary[t], -1.0);
        constraint->SetCoefficient(op_binary[t], -1.0);
    }
}

void ptes_chp_dispatch::create_min_up_time_constraint(std::vector<MPVariable*> op_binary, std::vector<MPVariable*> up_binary,
    int prev_op_state, min_up_down_params up_down_params, std::string name_pre) {
    // Minimum up-time constraint
    // General Form: sum{tp in Tau : 0 <= time_elapsed[t] - time_elapsed[tp] < min_up_time} up_binary[tp] <= op_binary[t] 
    //                      forall t in Tau : time_elapsed[t] > (min_up_time - init_up_time) * prev_op_state
    for (int t = 0; t < params.n_periods; t++) { // For all time periods
        if (up_down_params.time_elapsed.at(t) > (up_down_params.up_time_min - up_down_params.up_time0) * prev_op_state) {
            MPConstraint* constraint;
            std::string name = name_pre + "[" + std::to_string(t) + "]";
            constraint = solver->MakeRowConstraint(-MPSolver::infinity(), 0.0, name);
            for (int tp = 0; tp < params.n_periods; tp++) {
                double delta_time = up_down_params.time_elapsed.at(t) - up_down_params.time_elapsed.at(tp);
                if ((delta_time >= 0) && (delta_time < up_down_params.up_time_min)) {
                    constraint->SetCoefficient(up_binary[tp], 1.0);
                }
            }
            constraint->SetCoefficient(op_binary[t], -1.0);
        }
    }
}

void ptes_chp_dispatch::create_min_down_time_constraint(std::vector<MPVariable*> op_binary, std::vector<MPVariable*> down_binary,
    int prev_op_state, min_up_down_params up_down_params, std::string name_pre) {
    // Minimum down-time constraint
    // General Form: sum{tp in Tau : 0 <= time_elapsed[t] - time_elapsed[tp] < min_down_time} down_binary[tp] <= 1 - op_binary[t] 
    //                      forall t in Tau : time_elapsed[t] > (min_down_time - init_down_time) * (1 - prev_op_state)
    for (int t = 0; t < params.n_periods; t++) { // For all time periods
        if (up_down_params.time_elapsed.at(t) > (up_down_params.down_time_min - up_down_params.down_time0) * (1 - prev_op_state)) {
            MPConstraint* constraint;
            std::string name = name_pre + "[" + std::to_string(t) + "]";
            constraint = solver->MakeRowConstraint(-MPSolver::infinity(), 1.0, name);
            for (int tp = 0; tp < params.n_periods; tp++) {
                double delta_time = up_down_params.time_elapsed.at(t) - up_down_params.time_elapsed.at(tp);
                if ((delta_time >= 0) && (delta_time < up_down_params.down_time_min)) {
                    constraint->SetCoefficient(down_binary[tp], 1.0);
                }
            }
            constraint->SetCoefficient(op_binary[t], 1.0);
        }
    }
}

void ptes_chp_dispatch::create_init_up_down_time_constraint(std::vector<MPVariable*> op_binary, int prev_op_state, 
    min_up_down_params up_down_params, std::string name_pre) {
    // Initial minimum up- and down- time enforcement
    // General Form: op_binary[t] = prev_op_state 
    //                  forall t in Tau : time_elapsed[t] <= max{ (min_up_time - init_up_time) * prev_op_state, 
    //                                                            (min_down_time - init_down_time) * (1 - prev_op_state)}
    for (int t = 0; t < params.n_periods; t++) {
        if (up_down_params.time_elapsed.at(t) <= std::max((up_down_params.up_time_min - up_down_params.up_time0) * prev_op_state,
            (up_down_params.down_time_min - up_down_params.down_time0) * (1 - prev_op_state))) {
            MPConstraint* constraint;
            std::string name = name_pre + "[" + std::to_string(t) + "]";
            constraint = solver->MakeRowConstraint(prev_op_state, prev_op_state, name);
            constraint->SetCoefficient(op_binary[t], 1.0);
        }
        else break;
    }
}

void ptes_chp_dispatch::create_min_up_down_constraints(std::vector<MPVariable*> op_binary, std::vector<MPVariable*> up_binary, std::vector<MPVariable*> down_binary,
    int prev_op_state, min_up_down_params up_down_params, std::string name_pre) {
    // Binary logic when switching state
    ptes_chp_dispatch::create_min_up_down_logic_constraint(op_binary, up_binary, down_binary, prev_op_state, name_pre + "binary_switch_logic");
    // minimum up-time constraint
    ptes_chp_dispatch::create_min_up_time_constraint(op_binary, up_binary, prev_op_state, up_down_params, name_pre + "min_up_time");
    // minimum down-time constraint
    ptes_chp_dispatch::create_min_down_time_constraint(op_binary, down_binary, prev_op_state, up_down_params, name_pre + "min_down_time");
    // initial minimum up- and down-time enforcement
    ptes_chp_dispatch::create_init_up_down_time_constraint(op_binary, prev_op_state, up_down_params, name_pre + "min_up_ititial");
}

