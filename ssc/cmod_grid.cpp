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

#include <cmath>
#include <memory>
#include <numeric>

#include "common.h"
#include "core.h"

#include "cmod_grid.h"

var_info vtab_grid_input[] = {
	/*   VARTYPE           DATATYPE         NAME                               LABEL                                    UNITS      META                   GROUP                  REQUIRED_IF                 CONSTRAINTS                      UI_HINTS*/
	// simulation inputs
	{ SSC_INPUT,        SSC_NUMBER,      "system_use_lifetime_output",        "Lifetime simulation",                   "0/1",     "0=SingleYearRepeated,1=RunEveryYear",   "Lifetime",        "?=0",                   "BOOLEAN",                          "" },
	{ SSC_INPUT,        SSC_NUMBER,      "analysis_period",                   "Lifetime analysis period",              "years",   "The number of years in the simulation", "Lifetime",        "system_use_lifetime_output=1","",                           "" },
    { SSC_INOUT,        SSC_ARRAY,       "energy_hourly_kW",								  "Power output of array",                "kW",        "Lifetime system generation",          "System Output",                  "",                        "",                              "" },

	// external compute module inputs
	{ SSC_INOUT,        SSC_ARRAY,       "gen",								  "System power generated",                "kW",        "Lifetime system generation",          "System Output",                  "",                        "",                              "" },
	{ SSC_INOUT,		SSC_ARRAY,	     "load",			                  "Electricity load (year 1)",             "kW",	    "",                                    "Load",	                       "",	                      "",	                           "" },
    { SSC_INPUT,		SSC_ARRAY,	     "crit_load",			              "Critical electricity load (year 1)",    "kW",	        "",				        "Load",                             "",	                      "",	                            "" },
    { SSC_INOUT,        SSC_ARRAY,       "grid_outage",                       "Grid outage in this time step",            "0/1",        "0=GridAvailable,1=GridUnavailable,Length=load", "Load",    "",                       "",                               "" },
    { SSC_INPUT,        SSC_ARRAY,       "load_escalation",                   "Annual load escalation",                "%/year",    "",                                    "Load",                        "?=0",                      "",                            "" },
    { SSC_INOUT,        SSC_ARRAY,       "monthly_energy",                    "Monthly AC energy in Year 1",              "kWh/mo",    "",                 "Monthly",       "",                    "LENGTH=12",                              "" },


var_info_invalid };

var_info vtab_grid_output[] = {

    { SSC_OUTPUT,        SSC_ARRAY,       "full_load",                       "Electricity load prior to grid outage (year 1)",  "kW",       "Load" "",                 "",                        "",                              "" },

	{ SSC_OUTPUT,        SSC_ARRAY,       "system_pre_interconnect_kwac",     "System power before grid interconnect",  "kW",       "Lifetime system generation" "",                 "",                        "",                              "" },
	{ SSC_OUTPUT,        SSC_NUMBER,      "capacity_factor_interconnect_ac",  "Capacity factor based on AC interconnection limit",  "%",          "",                "",                           "",                     "",                              "" },
	{ SSC_OUTPUT,        SSC_NUMBER,      "annual_energy_pre_interconnect_ac", "Annual Energy AC pre-interconnection (year 1)",   "kWh",        "",                "",                           "",                     "",                              "" },
	{ SSC_INOUT,        SSC_NUMBER,      "annual_energy",                    "Annual AC energy in Year 1",                        "kWh",        "",                "System Output",                           "",                     "",                              "" },
	{ SSC_OUTPUT,        SSC_NUMBER,      "annual_ac_interconnect_loss_percent","Annual Energy loss from interconnection limit (year 1)", "%", "",                "",                           "",                     "",                              "" },
	{ SSC_OUTPUT,        SSC_NUMBER,      "annual_ac_interconnect_loss_kwh",   "Annual Energy loss from interconnection limit (year 1)", "kWh", "",                "",                           "",                     "",                              "" },

	{ SSC_OUTPUT,        SSC_ARRAY,       "system_pre_curtailment_kwac",     "System power before grid curtailment",  "kW",       "Lifetime system generation" "",                 "",                        "",                              "" },
	
// outputs to be assigned
{ SSC_OUTPUT,        SSC_NUMBER,      "capacity_factor_curtailment_ac",  "Capacity factor based on AC electricity after curtailment and AC interconnection limit",  "%",          "",                "",                           "",                     "",                              "" },
	{ SSC_OUTPUT,        SSC_NUMBER,      "annual_energy_pre_curtailment_ac", "Annual Energy AC pre-curtailment (year 1)",   "kWh",        "",                "",                           "",                     "",                              "" },
	{ SSC_OUTPUT,        SSC_NUMBER,      "annual_ac_curtailment_loss_percent","Annual Energy loss from curtailment (year 1)", "%", "",                "",                           "",                     "",                              "" },
	{ SSC_OUTPUT,        SSC_NUMBER,      "annual_ac_curtailment_loss_kwh",   "Annual Energy loss from curtailment (year 1)", "kWh", "",                "",                           "",                     "",
			  "" },

var_info_invalid };


cm_grid::cm_grid()
{
	add_var_info(vtab_grid_input);
	add_var_info(vtab_grid_output);
	add_var_info(vtab_technology_outputs);
	add_var_info(vtab_grid_curtailment);
}

// Have to add this since compute module isn't actually fully constructed until compute is called with
// a vartable.
void cm_grid::construct()
{
	std::unique_ptr<gridVariables> tmp(new gridVariables(*this));
	gridVars = std::move(tmp);
}

void cm_grid::exec()
{
    construct();
    // System generation output, which is lifetime (if system_lifetime_output == true);
    if (is_assigned("energy_hourly_kW"))
        gridVars->systemGenerationLifetime_kW = as_vector_double("energy_hourly_kW");
    else
        gridVars->systemGenerationLifetime_kW = as_vector_double("gen");

    size_t n_rec_lifetime = gridVars->systemGenerationLifetime_kW.size();
    size_t n_rec_single_year;

    size_t analysis_period = 1;
    if (is_assigned("analysis_period")) {
        analysis_period = (size_t) as_integer("analysis_period");
    }
    bool system_use_lifetime_output = false;
    if (is_assigned("system_use_lifetime_output")) {
        system_use_lifetime_output = (bool) as_integer("system_use_lifetime_output");
    }

    std::vector<double> scaleFactors(analysis_period, 1.0); // No scaling factors for curtailment

    std::vector<double> curtailment_year_one;
    if (is_assigned("grid_curtailment")) {
        curtailment_year_one = as_vector_double("grid_curtailment");
    }
    double interpolation_factor = 1.0;
    single_year_to_lifetime_interpolated<double>(
        system_use_lifetime_output,
        (size_t)analysis_period,
        n_rec_lifetime,
        curtailment_year_one,
        scaleFactors,
        interpolation_factor,
        gridVars->gridCurtailmentLifetime_MW,
        n_rec_single_year,
        gridVars->dt_hour_gen);

    allocateOutputs();
    gridVars->numberOfLifetimeRecords = n_rec_lifetime;
    gridVars->numberOfSingleYearRecords = n_rec_single_year;
    gridVars->numberOfYears = n_rec_lifetime / n_rec_single_year;

    gridVars->grid_kW.reserve(gridVars->numberOfLifetimeRecords);
    gridVars->grid_kW = gridVars->systemGenerationLifetime_kW;

    std::vector<double> load_year_one;

    if (is_assigned("load")) {
        load_year_one = as_vector_double("load");
    }

    if (is_assigned("grid_outage")) {
        std::vector<bool> grid_outage_steps = as_vector_bool("grid_outage");
        bool analyze_outage = std::any_of(grid_outage_steps.begin(), grid_outage_steps.end(), [](bool x) {return x; });
        if (analyze_outage) {
            ssc_number_t* full_load = allocate("full_load", load_year_one.size());
            ssc_number_t* load_kw_out = allocate("load", load_year_one.size());
            for (size_t i = 0; i < load_year_one.size(); i++) {
                full_load[i] = load_year_one[i];
                load_year_one[i] = grid_outage_steps[i] ? 0.0 : load_year_one[i];
                load_kw_out[i] = static_cast<ssc_number_t>(load_year_one[i]);
            }
        }
        // No conversion from std::vector<bool> to var_data - worth adding?
        assign("grid_outage", as_vector_integer("grid_outage"));
    }

    scalefactors scale_calculator(m_vartab);

    // compute load (electric demand) annual escalation multipliers
    std::vector<ssc_number_t> load_scale(analysis_period, 1.0);
    if (is_assigned("load_escalation")) {
        load_scale = scale_calculator.get_factors("load_escalation");
    }

    interpolation_factor = 1.0;
    single_year_to_lifetime_interpolated<double>(
        system_use_lifetime_output,
        analysis_period,
        n_rec_lifetime,
        load_year_one,
        load_scale,
        interpolation_factor,
        gridVars->loadLifetime_kW,
        n_rec_single_year,
        gridVars->dt_hour_gen);

	// interconnection  calculations
	double capacity_factor_interconnect, annual_energy_pre_curtailment, annual_energy_pre_interconnect, annual_energy, capacity_factor_curtailment;
	capacity_factor_interconnect = annual_energy_pre_curtailment = annual_energy_pre_interconnect = annual_energy = capacity_factor_curtailment = 0;

//	annual_energy_pre_interconnect = std::accumulate(gridVars->systemGenerationLifetime_kW.begin(), gridVars->systemGenerationLifetime_kW.begin() + gridVars->numberOfSingleYearRecords, (double)0.0)*gridVars->dt_hour_gen;

	size_t hour = 0;
	size_t num_steps_per_hour = size_t(1.0 / gridVars->dt_hour_gen);
//	double annual_energy_pre_interconnect = as_double("annual_energy");
	// compute grid export, apply limit
	for (size_t i = 0; i < gridVars->numberOfLifetimeRecords; i++) 
	{
	    double gen = gridVars->systemGenerationLifetime_kW[i];
		double gridNet = gen - gridVars->loadLifetime_kW[i];

        // TODO - curtail if grid outage here

		if (gridVars->enable_interconnection_limit){
		    p_genPreInterconnect_kW[i] = static_cast<ssc_number_t>(gen);
            double interconnectionLimited = fmax(0., gridNet - gridVars->grid_interconnection_limit_kW);
		    gen -= interconnectionLimited;
		    gridNet -= interconnectionLimited;
		}

		// compute curtailment MW
		p_genPreCurtailment_kW[i] = static_cast<ssc_number_t>(gen);
		double curtailed = fmax(0., gridNet - gridVars->gridCurtailmentLifetime_MW[i]*1000.0);
        gen -= curtailed;

        p_gen_kW[i] = static_cast<ssc_number_t>(gen);

		if (i < gridVars->numberOfSingleYearRecords)
		{
			annual_energy_pre_interconnect += p_genPreInterconnect_kW[i];
			annual_energy_pre_curtailment += p_genPreCurtailment_kW[i];
			annual_energy += p_gen_kW[i];
		}


		if (((i + 1) % gridVars->numberOfSingleYearRecords) == 0)
			hour = 0;
		else if (((i + 1) % num_steps_per_hour) == 0)
			hour++;

	}

	annual_energy_pre_curtailment *= gridVars->dt_hour_gen;
	annual_energy_pre_interconnect *= gridVars->dt_hour_gen;
	annual_energy *= gridVars->dt_hour_gen;

    accumulate_monthly_for_year("gen", "monthly_energy", gridVars->dt_hour_gen, num_steps_per_hour);

//	annual_energy_interconnect = std::accumulate(gridVars->systemGenerationLifetime_kW.begin(), gridVars->systemGenerationLifetime_kW.begin() + gridVars->numberOfSingleYearRecords, (double)0.0)*gridVars->dt_hour_gen;
    if (gridVars->enable_interconnection_limit)
	    capacity_factor_interconnect = annual_energy_pre_curtailment * util::fraction_to_percent / (gridVars->grid_interconnection_limit_kW * 8760.);
	//	annual_energy_interconnect = std::accumulate(gridVars->systemGenerationLifetime_kW.begin(), gridVars->systemGenerationLifetime_kW.begin() + gridVars->numberOfSingleYearRecords, (double)0.0)*gridVars->dt_hour_gen;
	capacity_factor_curtailment = annual_energy * util::fraction_to_percent / (gridVars->grid_interconnection_limit_kW * 8760.);



    if (gridVars->enable_interconnection_limit){
	    assign("capacity_factor_interconnect_ac", var_data(capacity_factor_interconnect));
	    assign("annual_energy_pre_interconnect_ac", var_data(annual_energy_pre_interconnect));
	    assign("annual_ac_interconnect_loss_kwh", var_data(std::round(annual_energy_pre_interconnect - annual_energy_pre_curtailment)));
	    assign("annual_ac_interconnect_loss_percent", var_data(100.0*(annual_energy_pre_interconnect - annual_energy_pre_curtailment) / annual_energy_pre_interconnect));
	    assign("capacity_factor_interconnect_ac", var_data(capacity_factor_curtailment));
    }
	assign("annual_energy_pre_curtailment_ac", var_data(annual_energy_pre_curtailment));
	assign("annual_energy", var_data(annual_energy));
	assign("annual_ac_curtailment_loss_kwh", var_data(std::round(annual_energy_pre_curtailment - annual_energy)));
	assign("annual_ac_curtailment_loss_percent", var_data(100.0*(annual_energy_pre_curtailment - annual_energy) / annual_energy_pre_curtailment));


}

void cm_grid::allocateOutputs()
{
    p_gen_kW = allocate("gen", gridVars->systemGenerationLifetime_kW.size());
	p_genPreCurtailment_kW = allocate("system_pre_curtailment_kwac", gridVars->systemGenerationLifetime_kW.size());
	p_genPreInterconnect_kW = allocate("system_pre_interconnect_kwac", gridVars->systemGenerationLifetime_kW.size());
}

DEFINE_MODULE_ENTRY(grid, "Grid model", 1)
