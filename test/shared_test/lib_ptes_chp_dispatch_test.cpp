/*
BSD 3-Clause License

Copyright (c) Alliance for Sustainable Energy, LLC. See also https://github.com/NatLabRockies/ssc/blob/develop/LICENSE
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


#include <string>
#include <iostream>
#include <fstream>
#include <vector>

#include <gtest/gtest.h>
#include <lib_ortools.h>
#include "ortools/linear_solver/linear_solver.h"
#include "ortools/math_opt/cpp/math_opt.h"
#include <lib_ptes_chp_dispatch.h>

using namespace operations_research;

char input_data_path[512];
int data1 = sprintf(input_data_path, "%s/test/input_cases/ortools/", std::getenv("SSCDIR"));

TEST(lib_ptes_chp_dispatch_test, test)
{
	bool build_for_server = false;
    bool run_rolling_horizon_cases = false;

    std::string results_file = "heat_valued_results_no_off_design_heat.csv";
    std::string res_dir = "heat_valued_no_offdes_results_1p_gap/";
    std::string input_dir = input_data_path;

    // Problem set-up
    ptes_chp_dispatch ptes_chp;
    ptes_chp.design.init(100. /*cycle capacity*/, 0.47 /*cycle efficiency*/, 1.33 /*heat pump COP*/, 10. /*hour of tes*/); 
    PTES_CHP_Dispatch_Data* data = &ptes_chp.params;
    data->n_periods = 100;
    data->delta = 1.0;

    // PTES and heat off-taker configuration
    ptes_chp.config.is_charge_heat_reject = false;
    ptes_chp.config.is_discharge_heat_reject = true;
    ptes_chp.config.is_heat_demand_required = false;
    ptes_chp.config.is_heat_from_tes_allowed = false;
    ptes_chp.config.is_offtaker_tes = false;
    ptes_chp.config.is_heat_valued = true;

    // Setting model parameters
    data->setPrices(input_dir + "generic_low_carbon_duck_curve.csv", 60.0, 20.0);
    data->setHeatLoad(input_dir + "heat_load.csv");
    data->setDefaultAssumptions(ptes_chp.design);

    struct run_time{
        int opt_horizon;
        int roll_horizon;
        double time;
    };

    std::vector<run_time> times;
    times.clear();
    run_time run;
    run.opt_horizon = data->n_periods;
    run.roll_horizon = 0;
    run.time = ptes_chp.optimize(input_dir + res_dir + results_file);
    times.push_back(run);

    ptes_chp.solver->SuppressOutput(); // Turning off solver output for roll solves

    if (run_rolling_horizon_cases) {
        int opt_horizon = 0;
        int roll_horizon = 0;
        for (int roll_d = 1; roll_d <= 7; roll_d++) {
            roll_horizon = roll_d * 24;
            for (int opt_d = 1; opt_d <= 7; opt_d++) {
                opt_horizon = opt_d * 24;
                if (opt_horizon >= roll_horizon) {
                    LOG(INFO) << "Now solving an optimization horizon of " << opt_horizon
                        << " with a rolling horizon of " << roll_horizon
                        << std::endl;
                    std::string res_file_name = input_dir + res_dir
                        + std::to_string(opt_horizon) + "w_" + std::to_string(roll_horizon) + "r_" + results_file;
                    run.time = ptes_chp.rollingHorizonoptimize(opt_horizon, roll_horizon, res_file_name);
                    run.opt_horizon = opt_horizon;
                    run.roll_horizon = roll_horizon;
                    times.push_back(run);
                }
            }
        }
    }

    //LOG(INFO) << std::setfill(' ') << std::left << std::setw(15) << "Opt. Horizon"
    //    << std::left << std::setw(15) << "Roll Horizon"
    //    << std::left << std::setw(15) << "Time (sec)"
    //    << std::endl;
    //for (int i = 0; i < times.size(); i++) {
    //    LOG(INFO) << std::setfill(' ') << std::left << std::setw(15) << times[i].opt_horizon
    //        << std::left << std::setw(15) << times[i].roll_horizon
    //        << std::left << std::setw(15) << times[i].time
    //        << std::endl;
    //}

    //std::ofstream outputfile;
    //outputfile.open(input_dir + res_dir + "times.txt", std::ios_base::app);
    //std::string var_header = "Opt. Horizon, Roll Horizon, Time (sec)";
    //outputfile << var_header << std::endl;
    //for (int i = 0; i < times.size(); i++) {
    //    outputfile << times[i].opt_horizon
    //        << ", " << times[i].roll_horizon
    //        << ", " << times[i].time
    //        << std::endl;
    //}
    //outputfile.close();

    // return EXIT_SUCCESS;
}

TEST(math_opt_test, basic_example_test) {

    // Simple test to build a math_opt model and solve using multiple solvers

    math_opt::Model model("basic_example");
    const math_opt::Variable x = model.AddContinuousVariable(-1.0, 1.5, "x");
    const math_opt::Variable y = model.AddContinuousVariable(0.0, 1.0, "y");
    //const math_opt::Variable z = model.AddBinaryVariable("z");                  // Adding a binary to make it a MILP

    // TODO: Fails due to linking issues with absl_dll? Seems to be a problem with ABSL_DLL macro
    //model.AddLinearConstraint(x + y <= 1.5, "c1");
    // Another try same result
    //math_opt::LinearConstraint c1 = model.AddLinearConstraint(0.0, 1.5, "c1");
    //model.set_coefficient(c1, x, 1.0);
    //model.set_coefficient(c1, y, 1.0);

    //model.AddLinearConstraint(x <= 1.5 * z, "c2");


    //model.Maximize(x + 2 * y);

    // Set parameters, e.g., turn on logging
    //math_opt::SolveArguments args;
    //args.parameters.enable_output = true;

    // Solve the model using the GLOP solver
    //const absl::StatusOr<math_opt::SolveResult> result = math_opt::Solve(model, math_opt::SolverType::kGlop, args);
    //const absl::StatusOr<math_opt::SolveResult> result = math_opt::Solve(model, math_opt::SolverType::kXpress);


    //ASSERT_TRUE(result.ok()) << result.status().ToString();
    //CHECK_OK(result.status());
    //CHECK_OK(result->termination.EnsureIsOptimal());

    // Print some information from the result.
    //std::cout << "MathOpt solve succeeded" << std::endl;
    //std::cout << "Objective value: " << result->objective_value() << std::endl;
    //std::cout << "x: " << result->variable_values().at(x) << std::endl;
    //std::cout << "y: " << result->variable_values().at(y) << std::endl;
    //std::cout << "z: " << result->variable_values().at(z) << std::endl;
}

TEST(linear_solver_test, basic_example_test) {
    // Create the linear solver with the GLOP backend.
    //std::unique_ptr<MPSolver> solver(MPSolver::CreateSolver("GLOP"));     // LP
    //std::unique_ptr<MPSolver> solver(MPSolver::CreateSolver("XPRESS"));   // LP or MILP
    //std::unique_ptr<MPSolver> solver(MPSolver::CreateSolver("SCIP"));
    //std::unique_ptr<MPSolver> solver(MPSolver::CreateSolver("GLPK"));
    //std::unique_ptr<MPSolver> solver(MPSolver::CreateSolver("CLP"));
    std::unique_ptr<MPSolver> solver(MPSolver::CreateSolver("CBC"));
    //std::unique_ptr<MPSolver> solver(MPSolver::CreateSolver("HIGHS"));

    // Removes HIGHS naming error
    //MPSolver solver_obj = MPSolver("testing", MPSolver::OptimizationProblemType::HIGHS_MIXED_INTEGER_PROGRAMMING);
    //MPSolver* solver = &solver_obj; // Using MPSolver object directly instead of unique_ptr

    if (!solver) {
        LOG(WARNING) << "Could not create solver";
        return;
    }

    // Create the variables x and y.
    MPVariable* const x = solver->MakeNumVar(0.0, 1, "x");
    MPVariable* const y = solver->MakeNumVar(0.0, 2, "y");

    LOG(INFO) << "Number of variables = " << solver->NumVariables();

    // Create a linear constraint, x + y <= 2.
    const double infinity = solver->infinity();
    MPConstraint* const ct = solver->MakeRowConstraint(-infinity, 2.0, "ct");
    ct->SetCoefficient(x, 1);
    ct->SetCoefficient(y, 1);

    LOG(INFO) << "Number of constraints = " << solver->NumConstraints();

    // Create the objective function, 3 * x + y.
    MPObjective* const objective = solver->MutableObjective();
    objective->SetCoefficient(x, 3);
    objective->SetCoefficient(y, 1);
    objective->SetMaximization();

    LOG(INFO) << "Solving with " << solver->SolverVersion();
    MPSolver::ResultStatus result_status = solver->Solve();
    // Check that the problem has an optimal solution.
    LOG(INFO) << "Status: " << result_status;
    if (result_status != MPSolver::OPTIMAL) {
        LOG(INFO) << "The problem does not have an optimal solution!";
        if (result_status == MPSolver::FEASIBLE) {
            LOG(INFO) << "A potentially suboptimal solution was found";
        }
        else {
            LOG(WARNING) << "The solver could not solve the problem.";
            return;
        }
    }

    LOG(INFO) << "Solution:";
    LOG(INFO) << "Objective value = " << objective->Value();
    LOG(INFO) << "x = " << x->solution_value();
    LOG(INFO) << "y = " << y->solution_value();


    LOG(INFO) << "Changing the x bounds and solving again";
    //solver->variables();
    x->SetBounds(0.5, 3); // Updating bounds
    result_status = solver->Solve();
    LOG(INFO) << "Solution:";
    LOG(INFO) << "Objective value = " << objective->Value();
    LOG(INFO) << "x = " << x->solution_value();
    LOG(INFO) << "y = " << y->solution_value();

    LOG(INFO) << "Changing the constraint and solving again";
    //solver->constraints();
    ct->SetCoefficient(x, 2);
    result_status = solver->Solve();
    LOG(INFO) << "Solution:";
    LOG(INFO) << "Objective value = " << objective->Value();
    LOG(INFO) << "x = " << x->solution_value();
    LOG(INFO) << "y = " << y->solution_value();

    LOG(INFO) << "Changing the objective and solving again";
    solver->MutableObjective()->SetCoefficient(x, 1);
    result_status = solver->Solve();
    LOG(INFO) << "Solution:";
    LOG(INFO) << "Objective value = " << objective->Value();
    LOG(INFO) << "x = " << x->solution_value();
    LOG(INFO) << "y = " << y->solution_value();


    //LOG(INFO) << "Waiting 60 seconds...";
    //std::this_thread::sleep_for(std::chrono::seconds(60));
    //LOG(INFO) << "Done...";
}


// TODO: Can't get this working due to linking issues...
//namespace operations_research::math_opt {
//
//    using ::testing::status::IsOkAndHolds;
//
//    TEST(SmallModelTest, Integer) {
//        const std::unique_ptr<const Model> model = SmallModel(/*integer=*/true);
//        Solve(*model, SolverType::kGscip);
//        //EXPECT_THAT(Solve(*model, SolverType::kGscip), IsOkAndHolds(IsOptimal(9.0)));
//    }
//}
