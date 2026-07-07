#pragma once
#include "ortools/linear_solver/linear_solver.h"

using namespace operations_research;

class example {
    std::unique_ptr<MPSolver> solver;

public:
    example() {
        solver = std::unique_ptr<MPSolver>(MPSolver::CreateSolver("XPRESS"));
    }

    double data;
    std::vector<double> obj_coeff;

    void contraint_from_function(MPVariable* var1, MPVariable* var2, double bound, std::string name_pre);

    void BasicExample();
};
