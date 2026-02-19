/*
BSD 3-Clause License

Copyright (c) Alliance for Energy Innovation, LLC. See also https://github.com/NREL/ssc/blob/develop/LICENSE
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


#include "core.h"


// hybrids with "steps" instructions of how to combine - superceeded by cmod_pseudocode.txt from 5/2/2023 meeting in SAM-documentation\Performance Models\Hybrids (PVWindBatteryetc) folder
static var_info _cm_vtab_hybrid_steps[] = {
/*   VARTYPE           DATATYPE         NAME                           LABEL                                UNITS     META                      GROUP                      REQUIRED_IF                 CONSTRAINTS                      UI_HINTS*/
	{ SSC_INPUT,         SSC_TABLE,      "input",               "input_table input for one technology",             "","","",      "*",        "",      "" },
    { SSC_OUTPUT,        SSC_TABLE,      "output",               "input_table output for one technology",           "","","",      "*",        "",      "" },


var_info_invalid };

class cm_hybrid_steps : public compute_module
{
public:

	cm_hybrid_steps()
	{
		add_var_info( _cm_vtab_hybrid_steps );
	}

    void exec()
    {
        auto input_table = lookup("input");
        if (input_table->type != SSC_TABLE)
            throw exec_error("hybrid_steps", "No input input_table found.");

        if (input_table->table.is_assigned("steps")) {

            auto& vec_steps = input_table->table.lookup("steps")->vec;

            auto outputs = ssc_data_create();

            for (size_t i = 0; i < vec_steps.size(); i++) {

                auto& current_step = input_table->table.lookup(vec_steps[i].str)->table;

                if (current_step.is_assigned("run")) {
                    // e.g.  "run": ["pvwattsv8", "windpower"]

                    // TODO:remove after combine working testing aggregates - vectors or single values, etc.
                    double cumulative_annual_energy = 0, annual_energy;

                    auto& vec_cms = current_step.lookup("run")->vec;
                    // loop based on input_table of input_table inputs
                    // loop for multiple hybrid compute modules starts here

                    for (size_t i = 0; i < vec_cms.size(); i++) {

                        auto& compute_module = vec_cms[i].str;
                        auto compute_module_inputs = input_table->table.lookup(compute_module);
                        if (compute_module_inputs->type != SSC_TABLE)
                            throw exec_error("hybrid_steps", "No input input_table found for ." + compute_module);

                        auto module = ssc_module_create(compute_module.c_str());

                        auto& input = compute_module_inputs->table;
                        ssc_module_exec(module, static_cast<ssc_data_t>(&input));

                        auto compute_module_outputs = ssc_data_create();

                        int pidx = 0;
                        while (const ssc_info_t p_inf = ssc_module_var_info(module, pidx++))
                        {
                            int var_type = ssc_info_var_type(p_inf);   // SSC_INPUT, SSC_OUTPUT, SSC_INOUT
                            if (var_type == SSC_OUTPUT) { // maybe add INOUT
                                auto var_name = ssc_info_name(p_inf);
                                auto type = ssc_info_data_type(p_inf);
                                auto var_value = input.lookup(var_name);
                                ssc_data_set_var(compute_module_outputs, var_name, var_value);
                            }
                        }

                        ssc_data_set_table(outputs, compute_module.c_str(), compute_module_outputs);

                        ssc_data_get_number(compute_module_outputs, "annual_energy", &annual_energy);
                        cumulative_annual_energy += annual_energy;

                        ssc_module_free(module);
                        ssc_data_free(compute_module_outputs);
                    }
                    // need to agregate some outputs potenitally here
                    ssc_data_set_number(outputs, "cumulative_annual_energy", cumulative_annual_energy);

                }
                else if (current_step.is_assigned("add")) {
                    // e.g.    "combine": ["pvwattsv8:annual_energy", "windpower:annual_energy"]
                    auto& vec_ccombines = current_step.lookup("add")->vec;

                    // determine type to combine
                    // here implement single number
                    // TODO - check for valid output table and types
                    std::string out_var_name = "output";
                    ssc_number_t sum = 0;
                    for (size_t i = 0; i < vec_ccombines.size(); i++) {
                        auto cm_var = util::split(vec_ccombines[i].str, ":"); // TODO check size == 2
                        out_var_name = cm_var[1];
                        auto cm_output_table = ssc_data_get_table(outputs, cm_var[0].c_str());
                        ssc_number_t output_value;
                        if (ssc_data_get_number(cm_output_table, cm_var[1].c_str(), &output_value))
                            sum += output_value;
                    }
                    ssc_data_set_number(outputs, out_var_name.c_str(), sum);


                }
                else {
                    throw exec_error("hybrid_steps", "No valid instruction found for step " + vec_steps[i].str);
                }

            } // for vec_stpes

            assign("output", var_data(*(static_cast<var_table*>(outputs))));
            ssc_data_free(outputs);

        }
        else {
            throw exec_error("hybrid_steps", "No steps found.");
        }
	}
};

DEFINE_MODULE_ENTRY( hybrid_steps, "Hybrid processing with 'steps' passed in on how to combine", 1 )
