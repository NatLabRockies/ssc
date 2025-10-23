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


#include "core.h"
#include "common.h"


static var_info _cm_vtab_hybrid[] = {
    /*   VARTYPE           DATATYPE         NAME                           LABEL                                UNITS     META                      GROUP                      REQUIRED_IF                 CONSTRAINTS                      UI_HINTS*/
        { SSC_INPUT,         SSC_TABLE,      "input",               "input_table for multiple technologies and one financial market",             "","","",      "*",        "",      "" },
        { SSC_OUTPUT,        SSC_TABLE,      "output",               "output_table for multiple technologies and one financial market",           "","","",      "*",        "",      "" },

    var_info_invalid };


class cm_hybrid : public compute_module
{
public:

    cm_hybrid()
    {
        add_var_info(_cm_vtab_hybrid);
    }

    void ssc_module_exec_with_error(ssc_module_t module, var_table& input, std::string compute_module) {
        if (!ssc_module_exec(module, static_cast<ssc_data_t>(&input))) {
            std::string str = std::string(compute_module) + " execution error. ";
            int idx = 0;
            int type = -1;
            while (const char* msg = ssc_module_log(module, idx++, &type, nullptr))
            {
                if (/*/(type == SSC_NOTICE) || */(type == SSC_WARNING) || (type == SSC_ERROR)) {
                    str += std::string(msg);
                    str += "\t";
                }
            }
            ssc_module_free(module);
            throw std::runtime_error(str);
        }
    }

    void exec()
    {
        float percent = 0;
        var_data* input_table = lookup("input");
        if (input_table->type != SSC_TABLE)
            throw exec_error("hybrid", "No input input_table found.");

        // container for output tables
        ssc_data_t outputs = ssc_data_create();

        // setup generators and battery
        if (input_table->table.is_assigned("compute_modules")) { // list of compute modules from configuration

            std::vector<var_data>& vec_cms = input_table->table.lookup("compute_modules")->vec;

            std::vector<std::string> generators;
            std::vector<std::string> batteries;
            std::vector<std::string> fuelcells;
            std::vector<std::string> financials;  // remainder of compute modules e.g.  'grid', 'utilityrate5', 'singleowner' in above example "Hybrid" VarTable from SAM

            for (size_t i = 0; i < vec_cms.size(); i++) {
                std::string computemodulename = vec_cms[i].str;
                if ((computemodulename == "pvsamv1") || (computemodulename == "pvwattsv8") || (computemodulename == "windpower") || (computemodulename == "custom_generation"))
                    generators.push_back(computemodulename);
                else if (computemodulename == "battery")
                    batteries.push_back(computemodulename);
                else if (computemodulename == "fuelcell")
                    fuelcells.push_back(computemodulename);
                else {
                    financials.push_back(computemodulename);
                    computemodulename = "hybrid";
                }
                var_data* compute_module_inputs = input_table->table.lookup(computemodulename);
                if (compute_module_inputs->type != SSC_TABLE)
                    throw exec_error("hybrid", "No input input_table found for " + computemodulename);
            }

            // Hybrid system precheck
            if (generators.size() < 1)
                throw exec_error("hybrid", "Less than one generator specified.");
            if (batteries.size() > 1)
                throw exec_error("hybrid", "Only one battery bank allowed at this time.");
            if (fuelcells.size() > 1)
                throw exec_error("hybrid", "Only one fuel cell allowed at this time.");

            // run all generators and collect outputs and compute outputs
            size_t maximumTimeStepsPerHour = 1, currentTimeStepsPerHour;
            double hybridSystemCapacity = 0, hybridTotalInstalledCost = 0;
            int len = 0;
            std::vector<size_t> genTimestepsPerHour;
            bool ts_adj = false; // keep track of whether time step is adjusted for log messages

            // get financial inputs common to all technologies and copy into each tech's input tables
            var_data* financial_compute_modules = input_table->table.lookup("hybrid");
            int analysisPeriod = (int)financial_compute_modules->table.lookup("analysis_period")->num;
            ssc_number_t inflation_rate = financial_compute_modules->table.lookup("inflation_rate")->num * 0.01;
            ssc_number_t sales_tax_rate = 0;
            if (financial_compute_modules->table.is_assigned("sales_tax_rate")){
                sales_tax_rate = financial_compute_modules->table.lookup("sales_tax_rate")->num * 0.01;
            }

            for (size_t i = 0; i < vec_cms.size(); i++) {
                std::string computemodulename = vec_cms[i].str;
                var_data* compute_module_inputs = input_table->table.lookup(computemodulename);
                if (compute_module_inputs) {
                    compute_module_inputs->table.assign("analysis_period", analysisPeriod);
                    compute_module_inputs->table.assign("inflation_rate", inflation_rate * 1e2);
                    compute_module_inputs->table.assign("sales_tax_rate", sales_tax_rate * 1e2);
                }
            }

            for (size_t igen = 0; igen < generators.size(); igen++) {

                percent = 100.0f * ((float)igen / (float)(generators.size() + fuelcells.size() + batteries.size() + financials.size()));
                update("", percent);

                std::string& compute_module = generators[igen];
                var_data* compute_module_inputs = input_table->table.lookup(compute_module);
                ssc_module_t module = ssc_module_create(compute_module.c_str());
                ssc_module_hybridize(module);

                var_table& input = compute_module_inputs->table;
                ssc_data_set_number(static_cast<ssc_data_t>(&input), "en_batt", 0);

                ssc_module_exec_with_error(module, input, compute_module);

                ssc_data_t compute_module_outputs = ssc_data_create();

                int pidx = 0;
                while (const ssc_info_t p_inf = ssc_module_var_info(module, pidx++)) {
                    int var_type = ssc_info_var_type(p_inf);   // SSC_INPUT, SSC_OUTPUT, SSC_INOUT
                    if ((var_type == SSC_OUTPUT) || (var_type == SSC_INOUT)) { // maybe remove INOUT
                        auto var_name = ssc_info_name(p_inf);
                        auto var_value = input.lookup(var_name);
                        ssc_data_set_var(compute_module_outputs, var_name, var_value);
                    } 
                }
                bool system_use_lifetime_output = false;
                if (compute_module_inputs->table.lookup("system_use_lifetime_output"))
                    system_use_lifetime_output = compute_module_inputs->table.lookup("system_use_lifetime_output")->num;

                ssc_number_t system_capacity = compute_module_inputs->table.lookup("system_capacity")->num;
                if ((compute_module == "pvsamv1") || (compute_module == "pvwattsv8")) {
                    ssc_data_get_number(compute_module_outputs, "system_capacity_ac", &system_capacity);
                }
                hybridSystemCapacity += system_capacity;
                hybridTotalInstalledCost += compute_module_inputs->table.lookup("total_installed_cost")->num;

                // get minimum timestep from gen vector
                ssc_number_t* curGen = ssc_data_get_array(compute_module_outputs, "gen", &len);
                currentTimeStepsPerHour = len / 8760;
                log(util::format("Simulation time step is %d minutes for %s.", 60 / int(maximumTimeStepsPerHour), compute_module.c_str()), SSC_NOTICE);
                if (system_use_lifetime_output > 0) // below - assuming single year only
                    currentTimeStepsPerHour /= analysisPeriod;
                if (currentTimeStepsPerHour > maximumTimeStepsPerHour)
                {
                    maximumTimeStepsPerHour = currentTimeStepsPerHour;
                    ts_adj = true;
                }
                genTimestepsPerHour.push_back(currentTimeStepsPerHour);

                // add production O and M calculations - done below before financial calculations, production, capacity, annual and land lease...
                ssc_number_t* pOMProduction = ((var_table*)compute_module_outputs)->allocate("cf_om_production", analysisPeriod + 1);
                ssc_number_t* pOMCapacity = ((var_table*)compute_module_outputs)->allocate("cf_om_capacity", analysisPeriod + 1);
                ssc_number_t* pOMFixed = ((var_table*)compute_module_outputs)->allocate("cf_om_fixed", analysisPeriod + 1);

                escal_or_annual(input, pOMFixed, analysisPeriod, "om_fixed", inflation_rate, 1.0, false, input.as_double("om_fixed_escal") * 0.01); // $ 
                escal_or_annual(input, pOMProduction, analysisPeriod, "om_production", inflation_rate, 0.001, false, input.as_double("om_production_escal") * 0.01); // $/kWh after conversion
                escal_or_annual(input, pOMCapacity, analysisPeriod, "om_capacity", inflation_rate, system_capacity, false, input.as_double("om_capacity_escal") * 0.01); // $ after multiplying by system capacity

                // production - multiply by yearly gen (initially assume single year) - use degradation - specific to each generator
                // pvwattsv8 - "degradation" applied in financial model - assuming single year analysis like standalone pvwatts/single owner configuration
                // wind - "degradation" applied in financial model - assumes system availability already applied to "gen" output
                // pvsamv1 - "degradation" applied in performance model
                ssc_number_t* pEnergyNet = ((var_table*)compute_module_outputs)->allocate("cf_energy_net", analysisPeriod + 1);
                ssc_number_t* pDegradation = ((var_table*)compute_module_outputs)->allocate("cf_degradation", analysisPeriod + 1);

                if (system_use_lifetime_output > 0) { // e.g. pvsamv1
                    size_t timestepsPerYear = len / analysisPeriod;
                    for (int i = 0; i < analysisPeriod; i++) {
                        pDegradation[i + 1] = 1.0;
                        pEnergyNet[i + 1] = 0;
                        for (size_t j = 0; j < timestepsPerYear; j++) { // steps per year
                            pEnergyNet[i + 1] += curGen[i * timestepsPerYear + j] * currentTimeStepsPerHour; // power to energy
                        }
                    }
                }
                else {
                    size_t count_degrad = 0;
                    ssc_number_t* degrad = input.as_array("degradation", &count_degrad);
                    if (compute_module == "custom_generation")
                        input.assign("generic_degradation", *input.lookup("degradation"));
                    if (count_degrad == 1) {
                        for (int i = 1; i <= analysisPeriod; i++)
                            pDegradation[i] = pow((1.0 - degrad[0] / 100.0), i - 1);
                    }
                    else if (count_degrad > 0) {
                        for (int i = 0; i < analysisPeriod && i < (int)count_degrad; i++)
                            pDegradation[i + 1] = (1.0 - degrad[i] / 100.0);
                    }
                    ssc_number_t first_year_energy = ((var_table*)compute_module_outputs)->as_double("annual_energy"); // first year energy value
                    for (int i = 1; i <= analysisPeriod; i++) {
                        pEnergyNet[i] = first_year_energy * pDegradation[i];
                    }
                }
                for (int i = 1; i <= analysisPeriod; i++) {
                    pOMProduction[i] *= pEnergyNet[i];
                }

                // optional land lease o and m costs if present - set to zero by default
                if (compute_module_inputs->table.lookup("om_land_lease")) {
                    ssc_number_t* pOMLandLease = ((var_table*)compute_module_outputs)->allocate("cf_om_land_lease", analysisPeriod + 1);
                    ssc_number_t total_land_area = compute_module_inputs->table.lookup("land_area")->num;
                    escal_or_annual(input, pOMLandLease, analysisPeriod, "om_land_lease", inflation_rate, total_land_area, false, input.as_double("om_land_lease_escal") * 0.01);
                }
                // optional fossil fuel costs
                if (compute_module_inputs->table.lookup("om_fuel_cost") && compute_module_inputs->table.lookup("system_heat_rate") && compute_module_inputs->table.lookup("annual_fuel_usage")) {
                    ssc_number_t* pOMFuelCost = ((var_table*)compute_module_outputs)->allocate("cf_om_fuel_cost", analysisPeriod + 1);
                    ssc_number_t system_heat_rate = compute_module_inputs->table.lookup("system_heat_rate")->num;
                    ssc_number_t year1_fuel_use = ((var_table*)compute_module_outputs)->as_double("annual_fuel_usage"); // kWht
                    escal_or_annual(input, pOMFuelCost, analysisPeriod, "om_fuel_cost", inflation_rate, year1_fuel_use * system_heat_rate * 0.001, false, input.as_double("om_fuel_cost_escal") * 0.01);
                }

                // add calculations to compute module outputs - done above for regular compute module outputs - done above with allocate to compute_module_outputs

                ssc_data_set_table(outputs, compute_module.c_str(), compute_module_outputs);
                ssc_module_free(module);
                ssc_data_free(compute_module_outputs);

            } // end of generators

            if (ts_adj) log(util::format("Simulation time step for hybrid system is %d minutes.", 60 / int(maximumTimeStepsPerHour), SSC_NOTICE));
            size_t genLength = 8760 * maximumTimeStepsPerHour * analysisPeriod;
            ssc_number_t* pGen = ((var_table*)outputs)->allocate("gen", genLength);

            size_t idx = 0;
            for (size_t i = 0; i < genLength; i++)
                pGen[i] = 0.0;
            for (size_t g = 0; g < generators.size(); g++) {
                var_table generator_outputs = ((var_table*)outputs)->lookup(generators[g])->table;
                // retrieve each generator "gen" and "cf_degradation"
                size_t count_gen;
                ssc_number_t* gen = generator_outputs.as_array("gen", &count_gen);
                size_t count_degrade;
                ssc_number_t* cf_degradation = generator_outputs.as_array("cf_degradation", &count_degrade);
                idx = 0;
                for (int y = 1; y <= analysisPeriod; y++) {
                    for (size_t h = 0; h < 8760; h++) {
                        for (size_t sph = 0; sph < maximumTimeStepsPerHour; sph++) {
                            size_t offset = sph / maximumTimeStepsPerHour / genTimestepsPerHour[g];
                            if (offset > genTimestepsPerHour[g]) offset = genTimestepsPerHour[g];
                            if (count_gen == genLength)
                                pGen[idx] += gen[idx]; // lifetime output with degradation and availability
                            else
                                pGen[idx] += gen[h + offset] * cf_degradation[y];
                            idx++;
                        }
                    }
                }
            }

            if (fuelcells.size() > 0) { // run single fuel cell if present 

                percent = 100.0f * ((float)(generators.size() + fuelcells.size()) / (float)(generators.size() + fuelcells.size() + batteries.size() + financials.size()));
                update("", percent);

                std::string& compute_module = fuelcells[0];
                var_data* compute_module_inputs = input_table->table.lookup(compute_module);

                ssc_module_t module = ssc_module_create(compute_module.c_str());
                class compute_module* cmod = static_cast<class compute_module*>(module);
                ssc_module_hybridize(module);

                ssc_number_t system_capacity = compute_module_inputs->table.lookup("fuelcell_unit_max_power")->num;
                system_capacity *= compute_module_inputs->table.lookup("fuelcell_number_of_units")->num;
                hybridSystemCapacity += system_capacity;
                hybridTotalInstalledCost += compute_module_inputs->table.lookup("total_installed_cost")->num;

                var_table& input = compute_module_inputs->table;
                ssc_data_set_array(static_cast<ssc_data_t>(&input), "gen", pGen, (int)genLength);
                ssc_data_set_number(static_cast<ssc_data_t>(&input), "system_use_lifetime_output", 1); // for fuelcell_annual_energy_discharged

                // merge in hybrid vartable for configurations where battery and fuel cell dispatch are combined and not in the technology bin
                std::string hybridVarTable("Hybrid");
                var_data* hybrid_inputs = input_table->table.lookup(hybridVarTable);
                var_table& hybridinput = hybrid_inputs->table;
                input.merge(hybridinput, false);
                ssc_module_exec_with_error(module, input, compute_module);

                ssc_data_t compute_module_outputs = ssc_data_create();

                int pidx = 0;
                while (const ssc_info_t p_inf = ssc_module_var_info(module, pidx++)) {
                    int var_type = ssc_info_var_type(p_inf);   // SSC_INPUT, SSC_OUTPUT, SSC_INOUT
                    if ((var_type == SSC_OUTPUT) || (var_type == SSC_INOUT)) { // maybe remove INOUT
                        auto var_name = ssc_info_name(p_inf);
                        auto var_value = input.lookup(var_name);
                        ssc_data_set_var(compute_module_outputs, var_name, var_value);
                    }
                }

                // add production O and M calculations - done below before financial calculations
                ssc_number_t nameplate = 0;
                std::vector<double> fuelcell_discharged(analysisPeriod + 1, 0);
                ssc_number_t* pOMProduction = ((var_table*)compute_module_outputs)->allocate("cf_om_production", analysisPeriod + 1);
                ssc_number_t* pOMCapacity = ((var_table*)compute_module_outputs)->allocate("cf_om_capacity", analysisPeriod + 1);
                ssc_number_t* pOMFixed = ((var_table*)compute_module_outputs)->allocate("cf_om_fixed", analysisPeriod + 1);
                ssc_number_t* pFuelCellReplacement = ((var_table*)compute_module_outputs)->allocate("cf_fuelcell_replacement_cost_schedule", analysisPeriod + 1);
                escal_or_annual(input, pOMFixed, analysisPeriod, "om_fuelcell_fixed_cost", inflation_rate, 1.0, false, input.as_double("om_fixed_escal") * 0.01); // $
                escal_or_annual(input, pOMProduction, analysisPeriod, "om_fuelcell_variable_cost", inflation_rate, 0.001, false, input.as_double("om_production_escal") * 0.01); // $/kW
                escal_or_annual(input, pOMCapacity, analysisPeriod, "om_fuelcell_capacity_cost", inflation_rate, system_capacity, false, input.as_double("om_capacity_escal") * 0.01); // $

                ssc_number_t* pOMFuelCost = ((var_table*)compute_module_outputs)->allocate("cf_om_fuel_cost", analysisPeriod + 1);
                ssc_number_t system_heat_rate = compute_module_inputs->table.lookup("system_heat_rate")->num;
                ssc_number_t year1_fuel_use = ((var_table*)compute_module_outputs)->as_double("annual_fuel_usage"); // kWht
                escal_or_annual(input, pOMFuelCost, analysisPeriod, "om_fuel_cost", inflation_rate, year1_fuel_use * system_heat_rate * 0.001, false, input.as_double("om_fuel_cost_escal") * 0.01);

                nameplate = system_capacity;
                fuelcell_discharged = ((var_table*)compute_module_outputs)->as_vector_double("fuelcell_annual_energy_discharged");
                if (fuelcell_discharged.size() == 1) { // ssc #992
                    double first_val = fuelcell_discharged[0]; // first value differs here!
                    fuelcell_discharged.resize(analysisPeriod , first_val);
                }
                if (fuelcell_discharged.size() != (size_t)analysisPeriod )
                    throw exec_error("hybrid", util::format("fuelcell_discharged size (%d) incorrect", (int)fuelcell_discharged.size()));
                // fuelcell cost - replacement from lifetime analysis
                if (input.is_assigned("fuelcell_replacement_option") && (input.as_integer("fuelcell_replacement_option") > 0))
                {
                    size_t count;
                    ssc_number_t* fuelcell_rep = 0;
                    if (input.as_integer("fuelcell_replacement_option") == 1)
                        fuelcell_rep = ((var_table*)compute_module_outputs)->as_array("fuelcell_replacement", &count); // replacements per year calculated
                    else // user specified
                        fuelcell_rep = input.as_array("fuelcell_replacement_schedule", &count); // replacements per year user-defined

                    escal_or_annual(input, pFuelCellReplacement, analysisPeriod, "om_fuelcell_replacement_cost", inflation_rate, nameplate, false, input.as_double("om_replacement_cost_escal") * 0.01);

                    for (size_t i = 0; i < (size_t)analysisPeriod && i < count; i++) {
                        pFuelCellReplacement[i + 1] = fuelcell_rep[i] * pFuelCellReplacement[i + 1];
                    }
                }
                else {
                    for (size_t i = 0; i < (size_t)analysisPeriod; i++) {
                        pFuelCellReplacement[i + 1] = 0.0;
                    }
                }
                // production O and M conversion to $
                for (size_t i = 0; i < (size_t)analysisPeriod; i++)
                    pOMProduction[i + 1] *= fuelcell_discharged[i];

                // add to gen "fuelcell_power" * timestep (set for pGen above)
                // cash flow line item is fuelcell_annual_energy_discharged from cmod_fuelcell
                std::vector<double> gen(genLength, 0);
                gen = ((var_table*)compute_module_outputs)->as_vector_double("fuelcell_power");
                if (gen.size() != genLength)
                    throw exec_error("hybrid", util::format("fuelcell_power size (%d) incorrect", (int)gen.size()));
                for (size_t g = 0; g < genLength; g++) {
                    pGen[g] += gen[g] * maximumTimeStepsPerHour;
                }


                // resize annual outputs
                size_t arr_length = analysisPeriod + 1;
                ssc_number_t yr_0_value = 0.0;
                prepend_to_output((var_table*)compute_module_outputs, "fuelcell_replacement", arr_length, yr_0_value);
                prepend_to_output((var_table*)compute_module_outputs, "annual_fuel_usage_lifetime", arr_length, yr_0_value);
                prepend_to_output((var_table*)compute_module_outputs, "fuelcell_annual_energy_discharged", arr_length, yr_0_value);

                ssc_data_set_table(outputs, compute_module.c_str(), compute_module_outputs);
                ssc_module_free(module);
                ssc_data_free(compute_module_outputs);
            }
 
            if (batteries.size() > 0) { // run single battery (refator running code below)

                percent = 100.0f * ((float)(generators.size() + fuelcells.size() + batteries.size()) / (float)(generators.size() + fuelcells.size() + batteries.size() + financials.size()));
                update("", percent);

                std::string& compute_module = batteries[0];
                var_data* compute_module_inputs = input_table->table.lookup(compute_module);

                ssc_number_t system_capacity = compute_module_inputs->table.lookup("batt_power_discharge_max_kwac")->num;
                hybridSystemCapacity += system_capacity; // TODO: check capacity definitions for batteries and hybrid systems
                hybridTotalInstalledCost += compute_module_inputs->table.lookup("total_installed_cost")->num;

                // copy over required dispatch variables from hybrid
                if (financial_compute_modules->table.is_assigned("dispatch_sched_weekday"))
                compute_module_inputs->table.assign("dispatch_sched_weekday", *financial_compute_modules->table.lookup("dispatch_sched_weekday"));
                if (financial_compute_modules->table.is_assigned("dispatch_sched_weekday"))
                compute_module_inputs->table.assign("dispatch_sched_weekend", *financial_compute_modules->table.lookup("dispatch_sched_weekend"));
                if (financial_compute_modules->table.is_assigned("dispatch_tod_factors"))
                compute_module_inputs->table.assign("dispatch_tod_factors", *financial_compute_modules->table.lookup("dispatch_tod_factors"));
                if (financial_compute_modules->table.is_assigned("grid_interconnection_limit_kwac"))
                compute_module_inputs->table.assign("grid_interconnection_limit_kwac", *financial_compute_modules->table.lookup("grid_interconnection_limit_kwac"));
                if (financial_compute_modules->table.is_assigned("ppa_escalation"))
                compute_module_inputs->table.assign("ppa_escalation", *financial_compute_modules->table.lookup("ppa_escalation"));
                if (financial_compute_modules->table.is_assigned("ppa_multiplier_model"))
                compute_module_inputs->table.assign("ppa_multiplier_model", *financial_compute_modules->table.lookup("ppa_multiplier_model"));
                if (financial_compute_modules->table.is_assigned("ppa_price_input"))
                compute_module_inputs->table.assign("ppa_price_input", *financial_compute_modules->table.lookup("ppa_price_input"));

                ssc_module_t module = ssc_module_create(compute_module.c_str());
                ssc_module_hybridize(module);

                var_table& input = compute_module_inputs->table;

                // merge in hybrid vartable for configurations where battery dispatch variables are combined and not in the technology bin
                std::string hybridVarTable("Hybrid");
                var_data* hybrid_inputs = input_table->table.lookup(hybridVarTable);
                if (compute_module_inputs->type != SSC_TABLE)
                    throw exec_error("hybrid", "No input input_table found for ." + hybridVarTable);
                var_table& hybridinput = hybrid_inputs->table;
                input.merge(hybridinput, false);

                ssc_data_set_array(static_cast<ssc_data_t>(&input), "gen", pGen, (int)genLength);  // check if issue with lookahead dispatch with hourly PV and subhourly wind
                ssc_data_set_number(static_cast<ssc_data_t>(&input), "system_use_lifetime_output", 1);
                ssc_data_set_number(static_cast<ssc_data_t>(&input), "en_batt", 1); // should be done at UI level

                ssc_module_exec_with_error(module, input, compute_module);

                ssc_data_t compute_module_outputs = ssc_data_create();

                int pidx = 0;
                while (const ssc_info_t p_inf = ssc_module_var_info(module, pidx++)) {
                    int var_type = ssc_info_var_type(p_inf);   // SSC_INPUT, SSC_OUTPUT, SSC_INOUT
                    if ((var_type == SSC_OUTPUT) || (var_type == SSC_INOUT)) { // maybe remove INOUT
                        auto var_name = ssc_info_name(p_inf);
                        auto var_value = input.lookup(var_name);
                        ssc_data_set_var(compute_module_outputs, var_name, var_value);
                    }
                }
                // get latest output
                // add production O and M calculations - done below before financial calculations - note that these values do not have ssc_module_var_info values - need to be INOUT to subsequent compute modules
                ssc_number_t nameplate = 0;
                ssc_number_t* pOMProduction = ((var_table*)compute_module_outputs)->allocate("cf_om_production", analysisPeriod + 1);
                ssc_number_t* pOMCapacity = ((var_table*)compute_module_outputs)->allocate("cf_om_capacity", analysisPeriod + 1);
                ssc_number_t* pOMFixed = ((var_table*)compute_module_outputs)->allocate("cf_om_fixed", analysisPeriod + 1);
                escal_or_annual(input, pOMFixed, analysisPeriod, "om_batt_fixed_cost", inflation_rate, 1.0, false, input.as_double("om_fixed_escal") * 0.01);
                escal_or_annual(input, pOMProduction, analysisPeriod, "om_batt_variable_cost", inflation_rate, 0.001, false, input.as_double("om_production_escal") * 0.01);
                std::vector<double> battery_discharged(analysisPeriod, 0);
                nameplate = compute_module_inputs->table.lookup("om_batt_nameplate")->num;
                ssc_number_t* battery_discharged_array = ssc_data_get_array(compute_module_outputs, "batt_annual_discharge_energy", &len);
                if (len == 1) { // ssc #992
                    double first_val = battery_discharged[0];
                    battery_discharged.resize(analysisPeriod, first_val);
                }
                else if (len != analysisPeriod) {
                    throw exec_error("hybrid", util::format("battery_discharged size (%d) incorrect", (int)len));
                }
                else {
                    for (int i = 0; i < len; i++)
                        battery_discharged[i] = battery_discharged_array[i];
                }

                // battery cost - replacement from lifetime analysis
                double batt_cap = 0.0;
                if (input.as_integer("batt_replacement_option") > 0) {
                    ssc_number_t* batt_rep = 0;
                    std::vector<ssc_number_t> replacement_percent;
                    size_t count;
                    batt_rep = ((var_table*)compute_module_outputs)->as_array("batt_bank_replacement", &count); // replacements per year calculated

                    // replace at capacity percent
                    if (input.as_integer("batt_replacement_option") == 1) {

                        for (int i = 0; i < (int)count; i++) {
                            replacement_percent.push_back(100);
                        }
                    }
                    else {// user specified
                        replacement_percent = input.as_vector_ssc_number_t("batt_replacement_schedule_percent");
                    }
                    batt_cap = ((var_table*)compute_module_outputs)->as_double("batt_computed_bank_capacity");
                    // updated 10/17/15 per 10/14/15 meeting
                    ssc_number_t* pOMBattReplacementCost = ((var_table*)compute_module_outputs)->allocate("cf_battery_replacement_cost_schedule", analysisPeriod + 1);
                    escal_or_annual(input, pOMBattReplacementCost, analysisPeriod, "om_batt_replacement_cost", inflation_rate, batt_cap, false, input.as_double("om_replacement_cost_escal") * 0.01);

                    for (int i = 0; i < analysisPeriod && i < (int)count; i++) {
                        // the cash flow sheets are 1 indexed, batt_rep and replacement_percent is zero indexed
                        pOMBattReplacementCost[i + 1] = batt_rep[i] * replacement_percent[i] * 0.01 * pOMBattReplacementCost[i + 1];
                    }
                }
                else
                {
                    batt_cap = ((var_table*)compute_module_outputs)->as_double("batt_computed_bank_capacity");
                    // updated 10/17/15 per 10/14/15 meeting
                    ssc_number_t* pOMBattReplacementCost = ((var_table*)compute_module_outputs)->allocate("cf_battery_replacement_cost_schedule", analysisPeriod + 1);
                    escal_or_annual(input, pOMBattReplacementCost, analysisPeriod, "om_batt_replacement_cost", inflation_rate, batt_cap, false, input.as_double("om_replacement_cost_escal") * 0.01);
                }
                escal_or_annual(input, pOMCapacity, analysisPeriod, "om_batt_capacity_cost", inflation_rate, batt_cap, false, input.as_double("om_capacity_escal") * 0.01);

                // production O and M conversion to $
                for (size_t i = 0; i < (size_t)analysisPeriod; i++)
                    pOMProduction[i+1] *= battery_discharged[i];

                // resize annual outputs
                size_t arr_length = analysisPeriod + 1;
                ssc_number_t yr_0_value = 0.0;
                prepend_to_output((var_table*)compute_module_outputs, "batt_bank_replacement", arr_length, yr_0_value);
                prepend_to_output((var_table*)compute_module_outputs, "batt_annual_charge_energy", arr_length, yr_0_value);
                prepend_to_output((var_table*)compute_module_outputs, "batt_annual_discharge_energy", arr_length, yr_0_value);
                prepend_to_output((var_table*)compute_module_outputs, "batt_annual_charge_from_system", arr_length, yr_0_value);


                // add calculations to compute module outputs - done above for regular compute module outputs

                ssc_data_set_table(outputs, compute_module.c_str(), compute_module_outputs);
                ssc_module_free(module);
                ssc_data_free(compute_module_outputs);
            }

            bool use_batt_output = false;
            ssc_number_t* pBattGen = 0;
            size_t battGenLen = 0;
            // update gen to battery output
            if (batteries.size() > 0) {
                use_batt_output = true;
                pBattGen = ((var_table*)outputs)->lookup(batteries[0])->table.as_array("gen", &battGenLen);
                if (battGenLen == genLength) {
                    for (size_t g = 0; g < genLength; g++) {
                        // Batt's gen is an inout that includes other system components
                        pGen[g] = pBattGen[g];
                    }
                }
                else {
                    throw exec_error("hybrid", util::format("Battery gen length incorrect, battery timeseries contains %d entries, generators contain %d", battGenLen, genLength));
                }
            }

            // monthly energy generated
            size_t step_per_hour = maximumTimeStepsPerHour;
            ssc_number_t* pGenMonthly = ((var_table*)outputs)->allocate("monthly_energy", 12);
            size_t c = 0;
            for (int m = 0; m < 12; m++) // each month
            {
                pGenMonthly[m] = 0;
                for (size_t d = 0; d < util::nday[m]; d++) // for each day in each month
                    for (int h = 0; h < 24; h++) // for each hour in each day
                        for (size_t j = 0; j < step_per_hour; j++)
                            pGenMonthly[m] += pGen[c++];
            }

            ssc_number_t pGenAnnual = 0;
            for (size_t i = 0; i < genLength; i++)
                pGenAnnual += pGen[i];
            ((var_table*)outputs)->assign("annual_energy", var_data(pGenAnnual));

            ssc_number_t* pHybridOMSum = ((var_table*)outputs)->allocate("cf_hybrid_om_sum", analysisPeriod + 1); // add to top level "output" - assumes analysis period the same for all generators

            for (int i = 0; i <= analysisPeriod; i++)
                pHybridOMSum[i] = 0.0;
            for (size_t g = 0; g < generators.size(); g++) {
                var_table generator_outputs = ((var_table*)outputs)->lookup(generators[g])->table;
                size_t count_gen;
                ssc_number_t* om_production = generator_outputs.as_array("cf_om_production", &count_gen);
                ssc_number_t* om_fixed = generator_outputs.as_array("cf_om_fixed", &count_gen);
                ssc_number_t* om_capacity = generator_outputs.as_array("cf_om_capacity", &count_gen);
                ssc_number_t* om_fuel_cost = NULL;
                if (generator_outputs.lookup("cf_om_fuel_cost"))
                    om_fuel_cost = generator_outputs.as_array("cf_om_fuel_cost", &count_gen);
                ssc_number_t* om_landlease = NULL;
                if (generator_outputs.lookup("cf_om_land_lease"))
                    om_landlease = generator_outputs.as_array("cf_om_land_lease", &count_gen);
                for (int y = 1; y <= analysisPeriod; y++) {
                    pHybridOMSum[y] += om_production[y] + om_fixed[y] + om_capacity[y];
                    if (generator_outputs.lookup("cf_om_fuel_cost"))
                        pHybridOMSum[y] += om_fuel_cost[y];
                    if (generator_outputs.lookup("cf_om_land_lease"))
                        pHybridOMSum[y] += om_landlease[y];
                }
            }

            ssc_number_t* pThermalPower = ((var_table*)outputs)->allocate("fuelcell_power_thermal", genLength);
            size_t thermalPowerLen = 0;


            for (size_t f = 0; f < fuelcells.size(); f++) {
                var_table fuelcell_outputs = ((var_table*)outputs)->lookup(fuelcells[f])->table;
                size_t count_fc;
                ssc_number_t* fc_thermalpower = fuelcell_outputs.as_array("fuelcell_power_thermal", &thermalPowerLen);
                if(thermalPowerLen != genLength) {
                    throw exec_error("hybrid", util::format("fuel cell power thermal size (%d) incorrect", (int)thermalPowerLen));
                }
                else {
                    for (size_t i = 0; i < genLength; i++)
                        pThermalPower[i] = fc_thermalpower[i];
                }

                ssc_number_t* om_production = fuelcell_outputs.as_array("cf_om_production", &count_fc);
                ssc_number_t* om_fixed = fuelcell_outputs.as_array("cf_om_fixed", &count_fc);
                ssc_number_t* om_capacity = fuelcell_outputs.as_array("cf_om_capacity", &count_fc);
                ssc_number_t* om_replacement = fuelcell_outputs.as_array("cf_fuelcell_replacement_cost_schedule", &count_fc);// optional
                ssc_number_t* om_fuel_cost = fuelcell_outputs.as_array("cf_om_fuel_cost", &count_fc);
                for (int y = 1; y <= analysisPeriod; y++) {
                    pHybridOMSum[y] += om_production[y] + om_fixed[y] + om_capacity[y] + om_replacement[y] + om_fuel_cost[y];
                }
            }

            for (size_t b = 0; b < batteries.size(); b++) {
                var_table batteries_outputs = ((var_table*)outputs)->lookup(batteries[b])->table;
                size_t count_b;
                ssc_number_t* om_production = batteries_outputs.as_array("cf_om_production", &count_b);
                ssc_number_t* om_fixed = batteries_outputs.as_array("cf_om_fixed", &count_b);
                ssc_number_t* om_capacity = batteries_outputs.as_array("cf_om_capacity", &count_b);
                ssc_number_t* om_replacement = batteries_outputs.as_array("cf_battery_replacement_cost_schedule", &count_b);
                for (int y = 1; y <= analysisPeriod; y++) {
                    pHybridOMSum[y] += om_production[y] + om_fixed[y] + om_capacity[y] + om_replacement[y];
                }
            }

            if (financials.size() > 0) { // run remaining compute modules with necessary inputs
                // note that single vartable is used to run multiple compute modules
                // battery outputs passed in if present
                std::string hybridVarTable("Hybrid");
                var_data* compute_module_inputs = input_table->table.lookup(hybridVarTable);
                var_table& input = compute_module_inputs->table;

                ssc_data_set_array(static_cast<ssc_data_t>(&input), "gen", pGen, (int)genLength);

                if (batteries.size() > 0)
                    ssc_data_set_number(static_cast<ssc_data_t>(&input), "is_hybrid", 1); // for updating battery outputs to annual length in update_battery_outputs in common_financial.cpp

                if (fuelcells.size() > 0)
                    ssc_data_set_array(static_cast<ssc_data_t>(&input), "fuelcell_power_thermal", pThermalPower, (int)thermalPowerLen); // for running cmod_thermalrate

                ssc_data_set_number(static_cast<ssc_data_t>(&input), "system_use_lifetime_output", 1);

                // set additional inputs from previous results - note - remove these from UI?
                ssc_data_set_number(static_cast<ssc_data_t>(&input), "total_installed_cost", hybridTotalInstalledCost);
                ssc_data_set_number(static_cast<ssc_data_t>(&input), "system_capacity", hybridSystemCapacity);

                ssc_data_set_array(&(compute_module_inputs->table), "cf_hybrid_om_sum", pHybridOMSum, (int)(analysisPeriod + 1));

                // set monthly and annual energy generated above and for outputs from financial models
                ssc_data_set_array(&(compute_module_inputs->table), "monthly_energy", pGenMonthly, 12);
                ssc_data_set_number(static_cast<ssc_data_t>(&input), "annual_energy", pGenAnnual);

                // run remaining compute modules in sequence and add results to "Hybrid" VarTable
                ssc_data_t hybridFinancialOutputs = ssc_data_create();

                for (size_t i = 0; i < financials.size(); i++) {

                    percent = 100.0f * ((float)(i + generators.size() + fuelcells.size() + batteries.size()) / (float)(generators.size() + fuelcells.size() + batteries.size() + financials.size()));
                    update("", percent);

                    std::string compute_module = financials[i];

                    ssc_module_t module = ssc_module_create(compute_module.c_str());
                    ssc_module_exec_with_error(module, input, compute_module);

                    int pidx = 0;
                    while (const ssc_info_t p_inf = ssc_module_var_info(module, pidx++)) {
                        int var_type = ssc_info_var_type(p_inf);   // SSC_INPUT, SSC_OUTPUT, SSC_INOUT
                        if ((var_type == SSC_OUTPUT) || (var_type == SSC_INOUT)) { // maybe remove INOUT
                            auto var_name = ssc_info_name(p_inf);
                            auto var_value = input.lookup(var_name);
                            ssc_data_set_var(hybridFinancialOutputs, var_name, var_value);
                        }
                    }
                    ssc_module_free(module);
                }

                ssc_data_set_table(outputs, hybridVarTable.c_str(), hybridFinancialOutputs);
                ssc_data_free(hybridFinancialOutputs);
            }

            assign("output", var_data(*(static_cast<var_table*>(outputs))));
            ssc_data_free(outputs);
        }
        else {
            throw exec_error("hybrid", "No compute modules specified.");
        }
    }

    void escal_or_annual(var_table& vt, ssc_number_t* cf, int nyears, const std::string& variable,
        double inflation_rate, double scale, bool as_rate = true, double escal = 0.0)
    {
        size_t count;
        ssc_number_t* arrp = vt.as_array(variable, &count);

        if (as_rate)
        {
            if (count == 1)
            {
                escal = inflation_rate + scale * arrp[0];
                for (int i = 0; i < nyears; i++)
                    cf[i + 1] = pow(1 + escal, i);
            }
            else
            {
                for (int i = 0; i < nyears && i < (int)count; i++)
                    cf[i + 1] = 1 + arrp[i] * scale;
            }
        }
        else
        {
            if (count == 1)
            {
                for (int i = 0; i < nyears; i++)
                    cf[i + 1] = arrp[0] * scale * pow(1 + escal + inflation_rate, i);
            }
            else
            {
                for (int i = 0; i < nyears && i < (int)count; i++)
                    cf[i + 1] = arrp[i] * scale;
            }
        }
    }


    void prepend_to_output(var_table* vt, std::string var_name, size_t count, ssc_number_t value) {
        size_t orig_count = 0;
        if (vt->is_assigned(var_name)) {
            ssc_number_t* arr = vt->as_array(var_name, &orig_count);
            arr = vt->resize_array(var_name, count);
            if (count > orig_count) {
                size_t diff = count - orig_count;
                for (int i = (int)orig_count - 1; i >= 0; i--) {
                    arr[i + diff] = arr[i];
                }
                for (int i = 0; i < (int)diff; i++) {
                    arr[i] = value;
                }
            }
        }
    }


};

DEFINE_MODULE_ENTRY(hybrid, "Hybrid processing", 1)
