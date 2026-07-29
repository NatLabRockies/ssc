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

#ifndef _CMOD_GRID_H_
#define _CMOD_GRID_H_

#include <map>
#include <memory>

#include "core.h"
#include "lib_time.h"

struct gridVariables
{
public:

//	gridVariables() {/* nothing to do */ };
	gridVariables(compute_module & cm) : 
		enable_interconnection_limit(cm.as_boolean("enable_interconnection_limit")),
		grid_interconnection_limit_kW(cm.as_double("grid_interconnection_limit_kwac"))	
	{	}

	// curtailment MW input
	std::vector<double> gridCurtailmentLifetime_MW;

	// generation input with interconnection limit
	std::vector<double> systemGenerationLifetime_kW;

	// pre-interconnected limited generation output
	std::vector<double> systemGenerationPreInterconnect_kW;

	// electric load input
	std::vector<double> loadLifetime_kW;

	// grid power
	std::vector<double> grid_kW;

	// enable interconnection limit
	bool enable_interconnection_limit;

	// interconnection limit
	double grid_interconnection_limit_kW;

	// Number of records
	size_t numberOfLifetimeRecords;
	size_t numberOfSingleYearRecords;
	size_t numberOfYears;
	double dt_hour_gen;
};

extern var_info vtab_grid_input[];
extern var_info vtab_grid_output[];

class cm_grid : public compute_module
{
public:

	/// Default constructor
	cm_grid();

	/// Default destructor
	~cm_grid() { /* nothing to do */ };

	/// construct since compute_module framework is fundamentally broken
	void construct();

	/// Main execution
	void exec();

	/// Allocate Outputs
	void allocateOutputs();

protected:

	// internally allocated
	std::unique_ptr<gridVariables> gridVars;

	// outputs
	ssc_number_t * p_gen_kW;
	ssc_number_t * p_genPreCurtailment_kW;
	ssc_number_t * p_genPreInterconnect_kW;

	
};

#endif
