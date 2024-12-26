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

#include <set>
#include <iomanip>
#include <numeric>

#include "SolarField.h"
#include "MultiRecOptimize.h"
#include <lp_lib.h>

//void __WINAPI mrec_opt_logfunction(lprec *, void *handler, char *buf)
//{
//    static_cast<multi_rec_opt_helper*>(handler)->sim_info->addSimulationNotice(buf);;
//}

int __WINAPI mrec_opt_abortfunction(lprec *, void *userhandle)
{
    multi_rec_opt_helper* par = static_cast<multi_rec_opt_helper*>(userhandle);
    return par->is_abort_flag ? TRUE : FALSE;
}

void __WINAPI mrec_opt_logfunction(lprec *lp, void *handler, char *buf)
//void __WINAPI opt_iter_function(lprec *lp, void *userhandle, int msg)
{
    multi_rec_opt_helper* par = static_cast<multi_rec_opt_helper*>(handler);
    
    if (par->solver_status == PRESOLVED+1)
        return;

    int stat = get_status(lp);
    
    double time_el = time_elapsed(lp);

    if (time_el > get_timeout(lp))
        par->is_abort_flag = true;

    if (stat != par->solver_status || stat > -1 )
    {
        std::string stattxt = get_statustext(lp, stat);
        par->solver_status = stat;
    }
    if (stat > -1)
    {
        std::stringstream logss;

        switch (stat)
        {
            /* Solver status values */
        case UNKNOWNERROR:
        case DATAIGNORED:
        case NOBFP:
        case NOMEMORY:
        case NOTRUN:
        case UNBOUNDED:
        case DEGENERATE:
        case NUMFAILURE:
            logss << "Optimization problem failed.";
            par->solver_status = PRESOLVED + 1; //flag that no more messages will be allowed
            break;
        case OPTIMAL:
            logss << "The optimal solution was identified!";
            par->solver_status = PRESOLVED + 1; //flag that no more messages will be allowed
            break;
        case SUBOPTIMAL:
            logss << "A solution was identified, but it is suboptimal.";
            par->solver_status = PRESOLVED + 1; //flag that no more messages will be allowed
            break;
        case INFEASIBLE:
            logss << "The optimization problem is infeasible. There is insufficient power from the heliostat field to meet the receiver power requirements.";
            par->solver_status = PRESOLVED + 1; //flag that no more messages will be allowed
            break;
        case TIMEOUT:
            logss << "The optimization problem timed out after " << get_timeout(lp) << " seconds without identifying a solution. Try again with a greater solution time limit.";
            par->solver_status = PRESOLVED + 1; //flag that no more messages will be allowed
            break;
        case USERABORT:
            logss << "The user aborted the program during optimization.";
            par->solver_status = PRESOLVED + 1; //flag that no more messages will be allowed
            break;
        case RUNNING:
        {
            if (time_el - par->last_report_time < par->sim_report_step)
                return;

            par->last_report_time = time_el;

            double obj = get_working_objective(lp);
            logss << "Time elapsed (sec): " << std::setfill(' ') << std::setw(8) << time_el << "\tObjective: " << std::setfill(' ') << std::setw(10) << obj;
            break;
        }
        case PRESOLVED:
            logss << "Optimization problem presolve complete.";
            break;
        default:
            break;
        }

        par->is_abort_flag = ! par->sim_info->addSimulationNotice(logss.str());
    }
    return;

}

//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

class mrec_optimization_vars
{
    int current_mem_pos;
    int alloc_mem_size;

    REAL *data;
public:
    struct opt_var
    {
        std::string name;
        int var_type;
        int var_dim;
        int var_dim_size;
        int var_dim_size2;
        int ind_start;
        int ind_end;
        REAL upper_bound;
        REAL lower_bound;
    };
private:
    std::vector<opt_var> var_objects;

    unordered_map<std::string, opt_var*> var_by_name;

public:
    struct VAR_TYPE { enum A { REAL_T, INT_T, BINARY_T }; };
    struct VAR_DIM { enum A { DIM_T, DIM_NT, DIM_T2, DIM_2T_TRI }; };

    mrec_optimization_vars();
    ~mrec_optimization_vars();

    void add_var(const std::string &vname, int var_type /* VAR_TYPE enum */, int var_dim /* VAR_DIM enum */, int var_dim_size, REAL lowbo = -DEF_INFINITE, REAL upbo = DEF_INFINITE);
    void add_var(const std::string &vname, int var_type /* VAR_TYPE enum */, int var_dim /* VAR_DIM enum */, int var_dim_size, int var_dim_size2, REAL lowbo = -DEF_INFINITE, REAL upbo = DEF_INFINITE);

    bool construct();

    int get_num_varobjs();
    int get_total_var_count();

    REAL &operator()(char *varname, int ind);    //Access for 1D var
    REAL &operator()(char *varname, int ind1, int ind2);     //Access for 2D var
    REAL &operator()(int varindex, int ind);
    REAL &operator()(int varindex, int ind1, int ind2);

    int column(const std::string &varname, int ind);
    int column(const std::string &varname, int ind1, int ind2);
    int column(int varindex, int ind);
    int column(int varindex, int ind1, int ind2);

    REAL *get_variable_array();

    opt_var *get_var(const std::string &varname);
    opt_var *get_var(int varindex);
};



//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

int multi_rec_opt_helper::run(SolarField *SF)
{
    /*
    During field operation, we must assign each heliostat to aim at one of several receivers at any given time. 
    The amount of power delivered to each receiver is determined by the receiver operation protocol and the 
    heliostats must collectively meet minimum power constraints for each receiver. The performance of each 
    heliostat differs depending on the receiver to which it is assigned.

    Two problems must be addressed: (1) the design problem must choose the optimal set of heliostats to include 
    in the final field layout given knowledge of receiver constraints and performance of each potential 
    heliostat with respect to each receiver assignment, (2) once the set of heliostats is determined, each must 
    be assigned to a receiver in a way that maximizes total field power output while maintaining receiver power 
    constraints.

    --- 

    This method poses and solves the linear optimization problem of assigning heliostats to receivers in such a 
    way that the optimal set of heliostats are chosen for design, and the maximum power output from the field is 
    achieved during optimization.

    */
    Hvector *helio_candidates = SF->getHeliostats();

    Hvector helios;
    for (int i = 0; i < helio_candidates->size(); i++)
    {
        unordered_map<Receiver*, double> rmap = helio_candidates->at(i)->getReceiverPowerAlloc();
        bool any = false;
        for (unordered_map<Receiver*, double>::iterator p = rmap.begin(); p != rmap.end(); p++)
            if (p->second > 0.)
                any = true;

        if (any)
            helios.push_back(helio_candidates->at(i));
    }

    Rvector *recs = SF->getReceivers();
    int Nrec = (int)recs->size();

    //calculate receiver design point power values
    std::vector< double > rec_design_power;
    for (int i = 0; i < Nrec; i++)
        rec_design_power.push_back( 1.e6 * recs->at(i)->getVarMap()->q_rec_des.Val() / recs->at(i)->getVarMap()->therm_eff.Val() );

    double sum_rec_des_pow = std::accumulate(rec_design_power.begin(), rec_design_power.end(), 0);

    int Nh = (int)helios.size();

    //heliostat to receiver power fractions
    unordered_map<int, std::vector<double> > power_allocs;
    unordered_map<int, std::vector<double> > costs;
    //The heliostat "cost of energy" will be scaled by the maximum production from a heliostat "max_rank"
    double max_rank = 0.;

    for (int i = 0; i < Nh; i++)
    {
        int id = helios.at(i)->getId();
        
        //estimate for energy value produced by the heliostat
        double rank_metric = helios.at(i)->getRankingMetricValue();
        //set the power from the heliostat to each receiver in the power allocation structure
        unordered_map<Receiver*, double> rpa = helios.at(i)->getReceiverPowerAlloc();

        power_allocs[id] = std::vector<double>();
        costs[id] = std::vector<double>();

        //do the power allocation calculation and save
        for (Rvector::iterator r = recs->begin(); r != recs->end(); r++)
        {
            double powalloc = rpa[*r];
            power_allocs[id].push_back(powalloc);
            costs[id].push_back(powalloc * rank_metric);
            //track max ranking metric value
            max_rank = std::fmax(powalloc*rank_metric, max_rank);
        }
    }

    //go back and scale all of the heliostat "cost of energy" values by the maximum observed ranking value
    for (unordered_map<int, std::vector<double> >::iterator pair = costs.begin(); pair != costs.end(); pair++)
        for (std::vector<double>::iterator dit = pair->second.begin(); dit != pair->second.end(); dit++)
            *dit = max_rank / (*dit > 0. ? *dit : max_rank * 1e3);

    //--------------------------------------------------------------------------
    //     Formulate the linear optimization problem
    //--------------------------------------------------------------------------

    mrec_optimization_vars O;    //helper class. largely duplicates ssc/tcs/csp_dispatch.* structure
    
    //add a new variable 'x' of dimension Nh x Nrec. 
    O.add_var("x", mrec_optimization_vars::VAR_TYPE::REAL_T, mrec_optimization_vars::VAR_DIM::DIM_NT, Nh, Nrec, 0., 1.);
    O.construct();
    
    //initialize the lp context
    lprec* lp = make_lp(0, Nh*Nrec);

    //reserve space for input arrays
    int *col = new int[Nh*Nrec];
    REAL *row = new REAL[Nh*Nrec];

    double rfact = SF->getVarMap()->flux.multi_rec_aim_rand.val;
    rfact = rfact > 1. ? 1. : rfact < 0. ? 0. : rfact;

    //set up objective
    if (is_performance)
    {   //performance problem objective function
        for (int j = 0; j < Nrec; j++)
        {
            for (int i = 0; i < Nh; i++)
            {
                //summation of all power values delivered from heliostat 'i' to receiver 'j'
                col[j*Nh + i] = O.column("x", i, j);
                row[j*Nh + i] = power_allocs[helios.at(i)->getId()].at(j) * (1. - rfact/2. + rfact*(double)rand()/(double)RAND_MAX) /sum_rec_des_pow; // normalize for scaling
            }
        }
    }
    else
    {   //design problem objective function
        for (int j = 0; j < Nrec; j++)
        {
            for (int i = 0; i < Nh; i++)
            {
                //summation of all heliostat "cost of energy" values delivered from heliostat 'i' to receiver 'j'
                col[j*Nh + i] = O.column("x", i, j);
                row[j*Nh + i] = costs[helios.at(i)->getId()][j];  //cost of energy proxy for each heliostat
            }
        }
    }
    
    //assign objective. Do this before starting to add constraints
    set_obj_fnex(lp, Nh*Nrec, row, col);
    //switch to constraint-adding mode
    set_add_rowmode(lp, TRUE);

    /*
    --------------------------------------------------------------------------------
    set up the variable properties
    --------------------------------------------------------------------------------
    */
    for (int j = 0; j < Nrec; j++)
    {
        for (int i = 0; i < Nh; i++)
        {
            //simple variable bounds. Upper bound is set by packing constraint
            set_lowbo(lp, j*Nh + i + 1, 0.);

            //set column name
            char s[40];
            sprintf(s, "%d:%d", helios.at(i)->getId(), j);
            set_col_name(lp, O.column("x", i, j), s);
        }
    }

    /*
    --------------------------------------------------------------------------------
    set up the constraints
    --------------------------------------------------------------------------------
    */
    //each receiver must meet a minimum power requirement
    if (is_performance)
    {
        /* 
        performance constraint - maintain desired power fractions by setting fractional power
        for all receivers equal to the first receiver

        Only enforce this constraint if the checkbox is enabled
        */
        if (SF->getVarMap()->sf.is_multirec_powfrac.val)
        {
            for (int j = 1; j < Nrec; j++)
            {
                //gamma_r is the fraction of power expected to be delivered by receiver 'r'
                double gamma_0 = SF->getVarMap()->recs.front().q_rec_des.Val() / SF->getVarMap()->sf.q_des.val;
                //sum all power delivered by receiver 0. do this each time, since row/col values are disturbed when adding constraints
                for (int i = 0; i < Nh; i++)
                {
                    int id = helios.at(i)->getId();
                    col[i] = O.column("x", i, 0);
                    row[i] = power_allocs.at(id).at(0) / sum_rec_des_pow / gamma_0;
                }

                double gamma_r = SF->getVarMap()->recs.at(j).q_rec_des.Val() / SF->getVarMap()->sf.q_des.val;
                //sum all power delivered by receiver r (r>=1).
                for (int i = 0; i < Nh; i++)
                {
                    int id = helios.at(i)->getId();
                    col[Nh + i] = O.column("x", i, j);
                    row[Nh + i] = -power_allocs.at(id).at(j) / sum_rec_des_pow / gamma_r;
                }
                //the constraint means sum of power from receiver 0 minus sum of power from receiver 'r' equals zero when scaled by their power fractions.
                add_constraintex(lp, Nh*2, row, col, EQ, 0.);
            }
        }
    }
    else
    {   //design constraint - achieve receiver design powers
        for (int j = 0; j < Nrec; j++)
        {
            //sum all power delivered to each receiver by all heliostats
            for (int i = 0; i < Nh; i++)
            {
                int id = helios.at(i)->getId();
                col[i] = O.column("x", i, j);
                row[i] = power_allocs[id].at(j) / rec_design_power.at(j);
            }
            //minimum power must be met. If insufficient power is available for any receiver, the problem fails as infeasible
            add_constraintex(lp, Nh, row, col, GE, 1.0);

        }
    }

    //total allocation of power from each can't exceed 1 times the available power from that heliostat
    for (int i = 0; i < Nh; i++)
    {
        for (int j = 0; j < Nrec; j++)
        {
            col[j] = O.column("x", i, j);
            row[j] = 1.;
        }
        add_constraintex(lp, Nrec, row, col, LE, 1.);
    }

    delete[] row;
    delete[] col;

    if (is_performance)
        //Set problem to maximize power output
        set_maxim(lp);
    else
        //(design) set problem to minimize heliostat cost of energy
        set_minim(lp);

    //reset the row mode
    set_add_rowmode(lp, FALSE);

    //optimization control parameters
    is_abort_flag = false;  //initialize abort flag

    if (sim_info)   //if a simulation info object has been assigned, request messages and logs be directed there
    {
        //put_msgfunc(lp, opt_iter_function, (void*)(this), MSG_ITERATION | MSG_LPBETTER | MSG_LPFEASIBLE | MSG_LPOPTIMAL);
        put_logfunc(lp, mrec_opt_logfunction, (void*)(this));
    }
    put_abortfunc(lp, mrec_opt_abortfunction, (void*)(this));

#ifdef _DEBUG
    set_outputfile(lp, "aimpoint_optimization_log.txt");
    set_verbose(lp, FULL);

    //if (is_field_assigned) {
    //    set_outputfile(lp, "field_assign_setup.txt");
    //    print_lp(lp);
    //    set_outputfile(lp, "field_assign_solution.txt");
    //    print_solution(lp, 1);
    //}
    //else if (is_performance) {
    //    set_outputfile(lp, "performance_setup.txt");
    //    print_lp(lp);
    //    set_outputfile(lp, "performance_solution.txt");
    //    print_solution(lp, 1);
    //}
    //else {
    //    set_outputfile(lp, "setup.txt");
    //    print_lp(lp);
    //    set_outputfile(lp, "solution.txt");
    //    print_solution(lp, 1);
    //}
#else
    set_verbose(lp, IMPORTANT); //DETAILED //http://web.mit.edu/lpsolve/doc/set_verbose.htm
#endif
    set_timeout(lp, timeout_sec);  //max solution time
    set_presolve(lp, PRESOLVE_NONE, get_presolveloops(lp));
    set_scaling(lp, SCALE_EXTREME | SCALE_FUTURE2);

    //run the solver
    if( sim_info )
    {
        std::stringstream tmpss;
        tmpss << "Optimizing " << problem_name << " to " << (is_performance ? "maximize heliostat power output." : "minimize heliostat field cost.");
        sim_info->addSimulationNotice(tmpss.str());
    }
    int ret = solve(lp);

    //Collect the dispatch profile and startup flags
    bool return_ok = ret == OPTIMAL || ret == SUBOPTIMAL;

    //translate result to non-LPSOLVE enum values
    switch (ret)
    {
    case OPTIMAL:
        result_status = RESULT_STATUS::RS_OPTIMAL;
        break;
    case SUBOPTIMAL:
        result_status = RESULT_STATUS::RS_SUBOPTIMAL;
        break;
    case UNBOUNDED:
        result_status = RESULT_STATUS::RS_UNKNOWN_ERROR;
        break;
    case NUMFAILURE:
        result_status = RESULT_STATUS::RS_UNKNOWN_ERROR;
        break;
    case INFEASIBLE:
        result_status = RESULT_STATUS::RS_INFEASIBLE;
        break;
    case TIMEOUT:
        result_status = RESULT_STATUS::RS_TIMED_OUT;
        break;
    }
    double objective = get_objective(lp);
    int ncols = get_Ncolumns(lp);

    REAL *vars = new REAL[ncols];
    get_variables(lp, vars);
    
    //process the results
    unordered_map<int, unordered_map<int, double> > results;

    for (int i = 0; i < ncols; i++)
    {
        //which variable is this?
        char *colname = get_col_name(lp, i + 1);
        if (!colname) continue;
        int id, rec;
        try
        {
            std::vector<std::string> h_r = split(colname, ":");

            id = std::stoi(h_r.front());
            rec = std::stoi(h_r.back());
        }
        catch (...)
        {
            continue;
        }
        //save the result in a sparse map. Not all combinations of id & rec will be filled due to presove reductions
        results[id][rec] = vars[i];
    }

    delete[] vars;
    if (lp != NULL) {
        /* clean up such that all used memory by lpsolve is freed */
        delete_lp(lp);
    }

    //create a pointer map between heliostat ID and heliostat address
    unordered_map<int, Heliostat*> hmap;
    std::vector<Heliostat>* hobjs = SF->getHeliostatObjects();
    for (int i = 0; i < hobjs->size(); i++)
        hmap[hobjs->at(i).getId()] = &hobjs->at(i);

    //set of unique heliostats
    std::set<Heliostat*> heliostat_set;

    for (unordered_map<int, unordered_map<int, double> >::iterator hres = results.begin(); hres != results.end(); hres++)
    {
        //find heliostat address from ID map
        Heliostat* h = hmap.at(hres->first);
        h->setPowerToReceiver(0.);

        int best_rec = 0;
        double most_rec_power = -1.;

        //determine which receiver offers the most power opportunity for this heliostat
        for (int j = 0; j < Nrec; j++)
        {
            if (hres->second.find(j) != hres->second.end())
            {
                double prec = hres->second[j] * power_allocs.at(hres->first).at(j);

                if (prec > 0.)
                {
                    h->setPowerToReceiver(h->getPowerToReceiver() + prec);
                    heliostat_set.insert(h);        // If heliostat is all zeros, than is not included here

                    if (prec > most_rec_power)
                    {
                        most_rec_power = prec;
                        best_rec = j;
                    }
                }
            }
        }
        //assign this heliostat to the best receiver
        h->setWhichReceiver( recs->at(best_rec) );
    }

    if (!is_performance || is_field_assigned)
    {
        included_heliostats.clear();
        included_heliostats.reserve(heliostat_set.size());
        for (std::set<Heliostat*>::iterator h = heliostat_set.begin(); h != heliostat_set.end(); h++)
            included_heliostats.push_back(*h);
    }

    return result_status;
}


//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------


mrec_optimization_vars::mrec_optimization_vars()
{
    current_mem_pos = 0;
    alloc_mem_size = 0;
}

mrec_optimization_vars::~mrec_optimization_vars()
{
    delete[] data;
}

void mrec_optimization_vars::add_var(const std::string &vname, int var_type /* VAR_TYPE enum */, int var_dim /* VAR_DIM enum */, int var_dim_size, REAL lobo, REAL upbo)
{
    if (var_dim == VAR_DIM::DIM_T2)
        add_var(vname, var_type, VAR_DIM::DIM_NT, var_dim_size, var_dim_size, lobo, upbo);
    else
        add_var(vname, var_type, var_dim, var_dim_size, 1, lobo, upbo);

}

void mrec_optimization_vars::add_var(const std::string &vname, int var_type /* VAR_TYPE enum */, int var_dim /* VAR_DIM enum */, int var_dim_size, int var_dim_size2, REAL lobo, REAL upbo)
{
    var_objects.push_back(mrec_optimization_vars::opt_var());
    mrec_optimization_vars::opt_var *v = &var_objects.back();
    v->name = vname;
    v->ind_start = current_mem_pos;
    v->var_type = var_type;
    v->var_dim = var_dim;
    v->var_dim_size = var_dim_size;
    v->var_dim_size2 = var_dim_size2;
    if (v->var_type == mrec_optimization_vars::VAR_TYPE::BINARY_T)
    {
        v->upper_bound = 1.;
        v->lower_bound = 0.;
    }
    else
    {
        v->upper_bound = upbo;
        v->lower_bound = lobo;
    }

    //calculate the required memory space for this type of variable
    int mem_size;
    switch (var_dim)
    {
    case mrec_optimization_vars::VAR_DIM::DIM_T:
        mem_size = var_dim_size;
        break;
    case mrec_optimization_vars::VAR_DIM::DIM_NT:
        mem_size = var_dim_size * var_dim_size2;
        break;
    case mrec_optimization_vars::VAR_DIM::DIM_T2:
        throw std::runtime_error("invalid var dimension in add_var");
    case mrec_optimization_vars::VAR_DIM::DIM_2T_TRI:
        mem_size = (var_dim_size + 1) * var_dim_size / 2;
        break;
    }

    v->ind_end = v->ind_start + mem_size;

    current_mem_pos += mem_size;


}

bool mrec_optimization_vars::construct()
{
    if (current_mem_pos < 0 || current_mem_pos > 1000000)
        throw std::runtime_error("Bad memory allocation when constructing variable table for dispatch optimization.");

    data = new REAL[current_mem_pos];  //where is this deallocated?

    alloc_mem_size = current_mem_pos;

    for (int i = 0; i < (int)var_objects.size(); i++)
        var_by_name[var_objects.at(i).name] = &var_objects.at(i);

    return true;
}

REAL &mrec_optimization_vars::operator()(char *varname, int ind)    //Access for 1D var
{
    return data[var_by_name[varname]->ind_start + ind];

}

REAL &mrec_optimization_vars::operator()(char *varname, int ind1, int)     //Access for 2D var
{
    return data[column(varname, ind1, ind1) - 1];
}

REAL &mrec_optimization_vars::operator()(int varind, int ind)    //Access for 1D var
{
    return data[var_objects.at(varind).ind_start + ind];

}

REAL &mrec_optimization_vars::operator()(int varind, int ind1, int ind2)     //Access for 2D var
{
    return data[column(varind, ind1, ind2) - 1];
}


int mrec_optimization_vars::column(const std::string &varname, int ind)
{
    return var_by_name[varname]->ind_start + ind + 1;
}

int mrec_optimization_vars::column(const std::string &varname, int ind1, int ind2)
{
    opt_var *v = var_by_name[std::string(varname)];
    switch (v->var_dim)
    {
    case VAR_DIM::DIM_T:
        throw std::runtime_error("Attempting to access optimization variable memory via 2D call when referenced variable is 1D.");
    case VAR_DIM::DIM_NT:
        return v->ind_start + v->var_dim_size2 * ind1 + ind2 + 1;
    default:
    {
        int ind = v->var_dim_size * ind1 + ind2 - ((ind1 - 1)*ind1 / 2);
        return v->ind_start + ind + 1;
    }
    break;
    }
}

int mrec_optimization_vars::column(int varindex, int ind)
{
    return var_objects[varindex].ind_start + ind + 1;
}

int mrec_optimization_vars::column(int varindex, int ind1, int ind2)
{
    opt_var *v = &var_objects[varindex];
    switch (v->var_dim)
    {
    case VAR_DIM::DIM_T:
        throw std::runtime_error("Attempting to access optimization variable memory via 2D call when referenced variable is 1D.");
    case VAR_DIM::DIM_NT:
        return v->ind_start + v->var_dim_size2 * ind1 + ind2 + 1;
    default:
    {
        int ind = v->var_dim_size * ind1 + ind2 - ((ind1 - 1)*ind1 / 2);
        return v->ind_start + ind + 1;
    } break;
    }
}

int mrec_optimization_vars::get_num_varobjs()
{
    return (int)var_objects.size();
}

int mrec_optimization_vars::get_total_var_count()
{
    return alloc_mem_size;
}

REAL *mrec_optimization_vars::get_variable_array()
{
    return data;
}

mrec_optimization_vars::opt_var *mrec_optimization_vars::get_var(const std::string &varname)
{
    return var_by_name[varname];
}

mrec_optimization_vars::opt_var *mrec_optimization_vars::get_var(int varindex)
{
    return &var_objects[varindex];
}
