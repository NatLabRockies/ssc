#ifndef __lib_ptes_chp_dispatch_
#define __lib_ptes_chp_dispatch_

#include <vector>
#include "ortools/linear_solver/linear_solver.h"
using namespace operations_research;

struct DispatchConfig {
    bool is_offtaker_tes;               // Does the heat off-taker have TES? Adds storage constraints for off-taker heat.
    bool is_charge_heat_reject;         // Does the PTES reject heat during charge? Enables rejected heat constraint during charging operations
    bool is_discharge_heat_reject;      // Does the PTES reject heat during discharge? Enables rejected heat constraint during discharging operations
    bool is_heat_from_tes_allowed;      // Can heat off-taker demand be met with PTES hot TES? 
    bool is_heat_demand_required;       // Does the off-taker heat demand have to be met? Adds constraint that the heat demand must be met (can easily cause infeasibility)
    bool is_heat_valued;                // Is rejected heat valued? Heat to off-taker is valued in the objective function

    DispatchConfig() {
        is_offtaker_tes = false;
        is_charge_heat_reject = false;
        is_discharge_heat_reject = false;
        is_heat_from_tes_allowed = false;
        is_heat_demand_required = false;
        is_heat_valued = false;
    };
};

struct PtesDesign {
    double cycle_cap;   // [MWe]        Cycle thermodynamic power
    double cycle_eta;   // [MWe/MWt]    Cycle thermodynamic efficiency
    double hp_cop;      // [MWt/MWe]    Heat Pump Coefficient of Performance
    double tes_hrs;     // [hours]      Hours of storage
    double tes_cap;     // [MWht]       Storage thermal capacity
    double hp_cap;      // [MWe]        Heat pump thermodynamic power

    // initialize PTES design parameters
    void init(double cycle_capacity /*MWe*/, double cycle_eff /*MWe/MWt*/,
        double heatpump_cop /*MWt/MWe*/, double tes_duration /*hours*/) {
        cycle_cap = cycle_capacity;
        cycle_eta = cycle_eff;
        hp_cop = heatpump_cop;
        tes_hrs = tes_duration;
        tes_cap = cycle_cap / cycle_eta * tes_hrs; // MWht        Storage thermal capacity
        hp_cap = (cycle_cap / cycle_eta) / hp_cop; // MWe         Heat pump thermodynamic power
    };

};

struct min_up_down_params {
    std::vector<double> time_elapsed;       // [hr] Cumulative time elapsed at the end of period t
    double down_time_min;                   // [hr] Minimum required power cycle down-time
    double up_time_min;                     // [hr] Minimum required power cycle up-time
    double down_time0;                      // [hr] Time that has passed since the cycle has been down before the initial time step
    double up_time0;                        // [hr] Time that has passed since the cycle has been up before the initial time step

    min_up_down_params() {
        down_time_min = std::numeric_limits<double>::quiet_NaN();
        up_time_min = std::numeric_limits<double>::quiet_NaN();
        down_time0 = std::numeric_limits<double>::quiet_NaN();
        up_time0 = std::numeric_limits<double>::quiet_NaN();
    }

    void clear() {
        time_elapsed.clear();
    }

    void resize(int nt) {
        time_elapsed.resize(nt, 0.);
    }
};

struct PTES_CHP_Dispatch_Data {
    // Parameters
    int n_periods;                          // [-] Number of time steps within the time horizon
    double time_weighting;                  // [-] Weighting factor that discounts future decisions over more imminent ones
    double delta;                           // [hr] Duration of time period

    // Time-indexed Parameters
    std::vector<double> eta_amb;            // [-] Cycle efficiency ambient temperature adjustment factor in period t
    std::vector<double> heat_sell_price;    // [$/MWht] Heat sell price in period t
    std::vector<double> purchase_price;     // [$/MWh] Electricity purchase price in period t
    std::vector<double> sell_price;         // [$/MWh] Electricity sell price in period t
    std::vector<double> heat_demand;        // [MWht] Off-taker heat demand in period t

    // Cost Parameters
    double cycle_ramping_cost;              // [$/MWt-change] Cost to change cycle thermal input level (ramping up)
    double cycle_start_cost;                // [$/start] Cost to start-up the power cycle
    double heat_pump_ramp_cost;             // [$/MWe-change] Cost to change heat pump electrical input level (ramping up)
    double heat_pump_start_cost;            // [$/start] Cost to start-up the heat pump

    // Power block (cycle) parameters
    double e_pb_startup;                    // [MWht] Required energy expended to start the power cycle
    double q_pb_max;                        // [MWt] Maximum operational thermal power input to the power cycle
    double q_pb_min;                        // [MWt] Minimum operational thermal power input to the power cycle
    double q_pb_startup;                    // [MWt] Thermal power input during power cycle start-up
    double w_pb_max;                        // [MWe] Power cycle electric power rated capacity
    double eta_p;                           // [MWe/MWt] Slope of linear approximation of power cycle performance curve
    double eta_d;                           // [MWe/MWt] Cycle nominal efficiency
    double q_rc;                            // [MWt] Thermal power rejected during cycle operations

    // initial conditions - Cycle
    bool   is_pb_starting0;                 // [-] Power block is starting at the initial time step
    bool   is_pb_operating0;                // [-] Power block is operating at the initial time step
    double q_pb0;                           // [MWt] Thermal power consumption in the cycle entering the initial time step
    double e_pb_start0;                     // [-] Power block start energy consumed before initial time step 

    min_up_down_params min_up_down_p;      // Parameters required for minimum up- and down-time contraints

    // Heat pump parameters
    double e_hp_startup;                    // [MWht] Required energy expended to start the heat pump
    double w_hp_max;                        // [MWe] Maximum heat pump electric load
    double w_hp_min;                        // [MWe] Minimum heat pump electric load
    double w_hp_startup;                    // [MWe] Thermal power input during heat pump start-up
    double q_hp_max;                        // [MWt] Heat pump maximimum operational thermal power output
    double kappa_p;                         // [MWt/MWe] Slope of linear approximation of heat pump performance curve
    double q_rh;                            // [MWt] Thermal power rejected during heat pump operations

    // initial conditions - Heat pump
    bool   is_hp_starting0;                 // [-] Heat pump is starting at the initial time step 
    bool   is_hp_operating0;                // [-] Heat pump is operating at the initial time step
    double w_hp0;                           // [MWe] Electrical power consumption in the heat pump entering the initial time step
    double e_hp_start0;                     // [-] Heat pump start energy consumed before initial time step 

    // Thermal energy storage parameters
    double e_tes_max;                       // [MWht] Thermal energy stroage capacity
    double e_tes0;                          // [MWht] Current stored energy capacity
    double e_loss;                          // [MWht] Heat loss from stroage

    // Heat off-taker parameters
    double e_ot_max;                        // [MWht] Heat off-taker thermal energy storage capacity
    double e_ot0;                           // [MWht] Current heat off-taker stored energy capacity 
    double e_ot_loss;                       // [MWht] Heat loss from stroage
    double q_tes_max;                       // [MWt] Maximum heat rate that can be pulled from PTES storage

    PTES_CHP_Dispatch_Data() {
        n_periods = 8760;
        time_weighting = 0.99999;
        delta = 1.0;

        cycle_ramping_cost = 0.63;          // $/MW cap. form Kumar Gas - Aero Derivative CT (Typical Load Follows)
        cycle_start_cost = 24.;             // $/MW cap. from Kumar Gas - Aero Derivative CT (Warm Start)
        heat_pump_ramp_cost = 0.63;         //Assuming the same numbers for heat pump
        heat_pump_start_cost = 24.;

        e_pb_startup = std::numeric_limits<double>::quiet_NaN();
        q_pb_max = std::numeric_limits<double>::quiet_NaN();
        q_pb_min = std::numeric_limits<double>::quiet_NaN();
        q_pb_startup = std::numeric_limits<double>::quiet_NaN();
        w_pb_max = std::numeric_limits<double>::quiet_NaN();
        eta_p = std::numeric_limits<double>::quiet_NaN();
        eta_d = std::numeric_limits<double>::quiet_NaN();
        q_rc = std::numeric_limits<double>::quiet_NaN();

        is_pb_starting0 = false;
        is_pb_operating0 = false;
        q_pb0 = std::numeric_limits<double>::quiet_NaN();
        e_pb_start0 = std::numeric_limits<double>::quiet_NaN();

        e_hp_startup = std::numeric_limits<double>::quiet_NaN();
        w_hp_max = std::numeric_limits<double>::quiet_NaN();
        w_hp_min = std::numeric_limits<double>::quiet_NaN();
        w_hp_startup = std::numeric_limits<double>::quiet_NaN();
        q_hp_max = std::numeric_limits<double>::quiet_NaN();
        kappa_p = std::numeric_limits<double>::quiet_NaN();
        q_rh = std::numeric_limits<double>::quiet_NaN();

        is_hp_starting0 = false;
        is_hp_operating0 = false;
        w_hp0 = std::numeric_limits<double>::quiet_NaN();
        e_hp_start0 = std::numeric_limits<double>::quiet_NaN();

        e_tes_max = std::numeric_limits<double>::quiet_NaN();
        e_tes0 = std::numeric_limits<double>::quiet_NaN();
        e_loss = std::numeric_limits<double>::quiet_NaN();

        e_ot_max = std::numeric_limits<double>::quiet_NaN();
        e_ot0 = std::numeric_limits<double>::quiet_NaN();
        e_ot_loss = std::numeric_limits<double>::quiet_NaN();
        q_tes_max = std::numeric_limits<double>::quiet_NaN();
    }

    // Sets electricity sell and purchase prices as well as heat price
    void setPrices(std::string sell_price_file, double average_price /*$/MWhe*/, double heat_price /*$/MWht*/);

    // Sets heat load based on file and multipler
    void setHeatLoad(std::string heat_load_file, double multipler = 1.0 /*-*/);

    // Used to set a data vector via a csv file
    static void set_data_by_file(std::vector<double>& data, std::string filepath);

    void setDefaultAssumptions(PtesDesign design);

    void clear()
    {
        min_up_down_p.clear();
        eta_amb.clear();
        heat_sell_price.clear();
        purchase_price.clear();
        sell_price.clear();
        heat_demand.clear();
    }

    void resize(int nt)
    {
        min_up_down_p.resize(nt);
        eta_amb.resize(nt, 0.);
        heat_sell_price.resize(nt, 0.);
        purchase_price.resize(nt, 0.);
        sell_price.resize(nt, 0.);
        heat_demand.resize(nt, 0.);
    }

};

struct PTES_CHP_Dispatch_Output {
    std::vector<double> q_c;                // [MWt] Power cycle thermal power utilization in period t
    std::vector<double> q_delta_c;          // [MWt] Change in power cycle thermal input at time t
    std::vector<double> q_h;                // [MWt] Heat pump thermal power output in period t
    std::vector<double> q_r;                // [MWt] Thermal power rejected in period t
    std::vector<double> q_s;                // [MWt] Thermal power sold to heat off-taker in period t
    std::vector<double> s;                  // [MWht] Expected thermal energy storage charge state
    std::vector<double> s_ot;               // [MWht] Expected off-taker thermal energy storage charge state
    std::vector<double> u_csu;              // [MWht] Power cycle start-up energy inventory in period t
    std::vector<double> u_hsu;              // [MWht] Heat pump start-up energy inventory in period t
    std::vector<double> w_c;                // [MWe] Power cycle electricity generation in period t
    std::vector<double> w_delta_h;          // [MWe] Change in heat pump electrical input in period t
    std::vector<double> w_h;                // [MWe] Power cycle electricity generation in period t

    std::vector<bool> y_c;                  // [-] 1 if cycle is generating electric power in period t; 0 otherwise
    std::vector<bool> y_cgb;                // [-] 1 if cycle begins electric power generation in period t; 0 otherwise
    std::vector<bool> y_cge;                // [-] 1 if cycle stops electric power generation in period t; 0 otherwise
    std::vector<bool> y_csu;                // [-] 1 if cycle is starting up in period t; 0 otherwise
    std::vector<bool> y_csup;               // [-] 1 if cycle start-up cost is incurred in period t; 0 otherwise
    std::vector<bool> y_h;                  // [-] 1 if heat pump is operating in period t; 0 otherwise
    std::vector<bool> y_hsu;                // [-] 1 if heat pump is starting up in period t; 0 otherwise
    std::vector<bool> y_hsup;               // [-] 1 if heat pump start-up cost is incurred in period t; 0 otherwise

    void clear() {
        q_c.clear();
        q_delta_c.clear();
        q_h.clear();
        q_r.clear();
        q_s.clear();
        s.clear();
        s_ot.clear();
        u_csu.clear();
        u_hsu.clear();
        w_c.clear();
        w_delta_h.clear();
        w_h.clear();

        y_c.clear();
        y_cgb.clear();
        y_cge.clear();
        y_csu.clear();
        y_csup.clear();
        y_h.clear();
        y_hsu.clear();
        y_hsup.clear();
    }

    void resize(int nt) {
        q_c.resize(nt, 0.);
        q_delta_c.resize(nt, 0.);
        q_h.resize(nt, 0.);
        q_r.resize(nt, 0.);
        q_s.resize(nt, 0.);
        s.resize(nt, 0.);
        s_ot.resize(nt, 0.);
        u_csu.resize(nt, 0.);
        u_hsu.resize(nt, 0.);
        w_c.resize(nt, 0.);
        w_delta_h.resize(nt, 0.);
        w_h.resize(nt, 0.);

        y_c.resize(nt, false);
        y_cgb.resize(nt, false);
        y_cge.resize(nt, false);
        y_csu.resize(nt, false);
        y_csup.resize(nt, false);
        y_h.resize(nt, false);
        y_hsu.resize(nt, false);
        y_hsup.resize(nt, false);
    }
};

class ptes_chp_dispatch
{
    struct ContinuousVariables {
        std::vector<MPVariable*> q_c;            // [units] description...
        std::vector<MPVariable*> q_delta_c;
        std::vector<MPVariable*> q_h;
        std::vector<MPVariable*> q_r;
        std::vector<MPVariable*> q_tes;
        std::vector<MPVariable*> q_s;
        std::vector<MPVariable*> s;
        std::vector<MPVariable*> s_ot;
        std::vector<MPVariable*> u_csu;
        std::vector<MPVariable*> u_hsu;
        std::vector<MPVariable*> w_c;
        std::vector<MPVariable*> w_delta_h;
        std::vector<MPVariable*> w_h;
    
        void Clear() {
            q_c.clear();
            q_delta_c.clear();
            q_h.clear();
            q_r.clear();
            q_tes.clear();
            q_s.clear();
            s.clear();
            s_ot.clear();
            u_csu.clear();
            u_hsu.clear();
            w_c.clear();
            w_delta_h.clear();
            w_h.clear();
        }
    } vars;

    struct BinaryVariables {
        std::vector<MPVariable*> y_c;
        std::vector<MPVariable*> y_cgb;
        std::vector<MPVariable*> y_cge;
        std::vector<MPVariable*> y_csu;
        std::vector<MPVariable*> y_csup;
        std::vector<MPVariable*> y_h;
        std::vector<MPVariable*> y_hsu;
        std::vector<MPVariable*> y_hsup;

        void Clear() {
            y_c.clear();
            y_cgb.clear();
            y_cge.clear();
            y_csu.clear();
            y_csup.clear();
            y_h.clear();
            y_hsu.clear();
            y_hsup.clear();
        }
    } bin_vars;

    MPObjective* objective; //const

    // Initialize solver
    void initializeSolver();

    // Create dispatch variables
    void createVariables();

    // Create dispatch constraints
    void createConstraints();

    // Create objective function
    void createObjectiveFunction();

    void printVariableValuesToConsole(absl::Duration wall_time);

    // Print model results to file
    void printResultsFile(std::string filepath, bool append = false);

    // Update initial condtions
    void updateInitialConditions(int init_t);


public:
    std::unique_ptr<MPSolver> solver; // TODO:: move back to private?

    DispatchConfig config;

    MPSolverParameters solver_params;

    PTES_CHP_Dispatch_Data params;
    PTES_CHP_Dispatch_Output outputs;

    PtesDesign design;

    ptes_chp_dispatch();

    // Create Solver 
    std::unique_ptr<MPSolver> CreateSolver(const std::string& solver_id);

    // Declare dispatch formulation
    double optimize(std::string resultsfilepath);

    // Do optimization with rolling horizon
    double rollingHorizonoptimize(int opt_horizon, int roll_horizon, std::string resultsfilepath);

    // TODO:These functions should be inherited

    /*Start-up inventory constraint for all time periods(0 to nt - 1)
        General Form: su_inv[t] <= su_inv[t-1] + inv_roc * su_binary[t]
        if (t==0): su_inv[t] <= init_su_inv + inv_roc * su_binary[t]*/
    void create_startup_inventory_constraint(std::vector<MPVariable*> su_inv, std::vector<MPVariable*> su_binary,
        double inv_roc, double init_su_inv = 0.0, std::string name_pre = "");

    /* Start-up inventory nonzero constraint for all time periods(0 to nt - 1)
        General Form: su_inv[t] <= su_inv_req * su_binary[t]*/
    void create_inventory_nonzero_constraint(std::vector<MPVariable*> su_inv, std::vector<MPVariable*> su_binary,
        double su_inv_req, std::string name_pre = "");

    /*Operation allowing when start - up inventory is fulfilled or previously operating for all time periods(0 to nt - 1)
        General Form: op_binary[t] <= su_inv[t] / su_inv_req + op_binary[t-1]
        if (t==0): op_binary[t] <= su_inv[t] / su_inv_req  + prev_op_state */
    void create_operation_allowed_constraint(std::vector<MPVariable*> op_binary, std::vector<MPVariable*> su_inv,
        double su_inv_req, int prev_op_state = 0.0, std::string name_pre = "");

    /* Startup cannot be enabled after a time period where the device was operating for all time periods(0 to nt - 1)
        General Form: su_binary[t] + op_binary[t-1] <= 1
        if (t==0): su_binary[t] + prev_op_state <= 1 */
    void create_startup_wait_constraint(std::vector<MPVariable*> su_binary, std::vector<MPVariable*> op_binary,
        int prev_op_state = 0.0, std::string name_pre = "");
    
    /* Enforce minimum operation limit when system is operating for all time periods(0 to nt - 1)
        General Form: op_lvl[t] >= min_limit * op_binary[t] */
    void create_minimum_operation_constraint(std::vector<MPVariable*> op_lvl, std::vector<MPVariable*> op_binary,
        double min_limit, std::string name_pre = "");

    /* Enforce maximum operation limit when system is operatingand if starting up for all time periods(0 to nt - 1)
        General Form: op_lvl[t] + su_rate * su_binary[t] <= max_limit * op_binary[t]*/
    void create_maximum_operation_wstartup_constraint(std::vector<MPVariable*> op_lvl, std::vector<MPVariable*> su_binary, std::vector<MPVariable*> op_binary,
        double su_rate, double max_limit, std::string name_pre = "");

    /* Enforce maximum operation limit when system is operating for all time periods (0 to n_periods-1)
        General Form: op_lvl[t] <= max_limit * op_binary[t]*/
    void create_maximum_operation_constraint(std::vector<MPVariable*> op_lvl, std::vector<MPVariable*> op_binary,
        double max_limit, std::string name_pre = "");

    /* Model output production as a linear relationship of input for all time periods(0 to nt - 1)
        General Form: output[t] = amb_corr[t] * ( slope * input[t] + intercept * op_binary[t])*/
    void create_linear_production_constraint(std::vector<MPVariable*> output, std::vector<MPVariable*> input, std::vector<MPVariable*> op_binary,
        double slope, double intercept, std::vector<double> amb_corr, std::string name_pre = "");

    /* Calculates positive ramping for all time periods(0 to nt - 1)
        General Form: ramping[t] >= op_lvl[t] - op_lvl[t-1]
        if (t==0): ramping[t] >= op_lvl[t] - prev_op_rate*/
    void create_positive_ramping_constraint(std::vector<MPVariable*> ramping, std::vector<MPVariable*> op_lvl,
        double prev_op_rate, std::string name_pre = "");

    /* Enforces startup penalty for all time periods(0 to nt - 1)
        General Form: penalty[t] >= su_binary[t] - su_binary[t-1]
        if (t==0): penalty_binary[t] >= su_binary[t] - prev_su_state */
    void create_startup_penalty_constraint(std::vector<MPVariable*> penalty_binary, std::vector<MPVariable*> su_binary,
        double prev_su_state = 0.0, std::string name_pre = "");

    /*Enforces set packing constraint between two binaries variables for all time periods(0 to nt - 1)
        General Form: binary1[t] + binary2[t] <= 1*/
    void create_set_packing_constraint(std::vector<MPVariable*> binary1, std::vector<MPVariable*> binary2, std::string name_pre = "");

    /*Binary logic when switching state for tracking up - and down - time for all time periods(0 to nt - 1)
        General Form: up_binary[t] - down_binary[t] = op_binary[t] - op_binary[t-1]
        if (t==0): up_binary[t] - down_binary[t] = op_binary[t] - prev_op_state*/
    void create_min_up_down_logic_constraint(std::vector<MPVariable*> op_binary, std::vector<MPVariable*> up_binary, std::vector<MPVariable*> down_binary,
        int prev_op_state, std::string name_pre = "");

    /* Minimum up-time constraint
        General Form: sum{tp in Tau : 0 <= time_elapsed[t] - time_elapsed[tp] < min_up_time} up_binary[tp] <= op_binary[t] 
                              forall t in Tau : time_elapsed[t] > (min_up_time - init_up_time) * prev_op_state*/
    void create_min_up_time_constraint(std::vector<MPVariable*> op_binary, std::vector<MPVariable*> up_binary,
        int prev_op_state, min_up_down_params params, std::string name_pre = "");

    /* Minimum down-time constraint
        General Form: sum{tp in Tau : 0 <= time_elapsed[t] - time_elapsed[tp] < min_down_time} down_binary[tp] <= 1 - op_binary[t] 
                              forall t in Tau : time_elapsed[t] > (min_down_time - init_down_time) * (1 - prev_op_state)*/
    void create_min_down_time_constraint(std::vector<MPVariable*> op_binary, std::vector<MPVariable*> down_binary,
        int prev_op_state, min_up_down_params params, std::string name_pre = "");

    /* Initial minimum up- and down-time enforcement
         General Form: op_binary[t] = prev_op_state 
                          forall t in Tau : time_elapsed[t] <= max{ (min_up_time - init_up_time) * prev_op_state, 
                                                                    (min_down_time - init_down_time) * (1 - prev_op_state)}*/
    void create_init_up_down_time_constraint(std::vector<MPVariable*> op_binary, int prev_op_state, min_up_down_params params, std::string name_pre = "");

    /* Creates minimum up- and down- constraints*/
    void create_min_up_down_constraints(std::vector<MPVariable*> op_binary, std::vector<MPVariable*> up_binary, std::vector<MPVariable*> down_binary,
        int prev_op_state, min_up_down_params params, std::string name_pre = "");


};

#endif // !__lib_ptes_chp_dispatch_
