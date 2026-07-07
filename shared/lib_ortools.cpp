#include "ortools/linear_solver/linear_solver.h"
#include "lib_ptes_chp_dispatch.h"
#include "lib_ortools.h"

using namespace operations_research;


void example::contraint_from_function(MPVariable* var1, MPVariable* var2, double bound, std::string name_pre) {
    std::string name = name_pre + "ct" + std::to_string(2) + "end";
    MPConstraint* const ct = solver->MakeRowConstraint(0.0, bound, name);
    ct->SetCoefficient(var1, 1);
    ct->SetCoefficient(var2, 1);
}

void example::BasicExample() {
    // Create the linear solver.
    //std::unique_ptr<MPSolver> solver(MPSolver::CreateSolver("XPRESS"));
    //std::unique_ptr<MPSolver> solver(MPSolver::CreateSolver("SCIP"));

    if (!solver) {
        LOG(WARNING) << "Solver unavailable.";
        return;
    }

    // Creating an array of variables x (another way)
    std::vector<MPVariable*> x, y;
    solver->MakeNumVarArray(2, 0.0, 2.0, "x", &x);
    //solver->MakeIntVarArray(2, 0.0, 1.0, "y", &y); // binary variable
    solver->MakeBoolVarArray(2, "y", &y); // binary variable

    LOG(INFO) << "Number of variables = " << solver->NumVariables();

    // Create a linear constraint, 0 <= x + y <= 2.
    contraint_from_function(x[0], x[1], data, "continous");
    contraint_from_function(y[0], y[1], 1., "binary");

    LOG(INFO) << "Number of constraints = " << solver->NumConstraints();

    // Create the objective function
    MPObjective* const objective = solver->MutableObjective();
    for (int i = 0; i < 2; ++i) {
        objective->SetCoefficient(x[i], obj_coeff[i]);
        objective->SetCoefficient(y[i], obj_coeff[i]/4.0);
    }
    objective->SetMaximization();

    solver->Solve();

    LOG(INFO) << "Solution:" << std::endl;
    LOG(INFO) << "Objective value = " << objective->Value();
    for (int i = 0; i < 2; ++i) {
        LOG(INFO) << "x[" << i << "] = " << x[i]->solution_value();
    }
    for (int i = 0; i < 2; ++i) {
        LOG(INFO) << "y[" << i << "] = " << y[i]->solution_value();
    }
    LOG(INFO) << "Constraint name = " << solver->constraint(0)->name();
    LOG(INFO) << "Constraint name = " << solver->constraint(1)->name();
}
