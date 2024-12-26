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

static var_info _cm_vtab_mhk_tidal[] = {
	//   VARTYPE			DATATYPE			NAME									LABEL																		UNITS           META            GROUP              REQUIRED_IF					CONSTRAINTS				UI_HINTS	
	{ SSC_INPUT,			SSC_MATRIX,			"tidal_resource",					    "Frequency distribution of resource as a function of stream speeds",		"",				"",             "MHKTidal",			"*",						"",						"" },	
	{ SSC_INPUT,			SSC_MATRIX,			"tidal_power_curve",					"Power curve of tidal energy device as function of stream speeds",			"kW",			"",             "MHKTidal",			"*",						"",						"" },	
	//{ SSC_INPUT,			SSC_NUMBER,			"calculate_capacity",					"Calculate device rated capacity from power curve",							"0/1",			"",             "MHKTidal",         "?=1",                      "INTEGER,MIN=0,MAX=1",	"" },
	{ SSC_INPUT,			SSC_NUMBER,			"number_devices",						"Number of tidal devices in the system",									"",				"",             "MHKTidal",         "?=1",                      "INTEGER",				"" },
    { SSC_INPUT,			SSC_NUMBER,			"fixed_charge_rate",						"FCR from LCOE Cost page",									"",				"",             "MHKTidal",         "?=1",                      "",				"" },
    { SSC_INPUT,			SSC_NUMBER,			"device_costs_total",						"Device costs",									"$",				"",             "MHKTidal",         "?=1",                      "",				"" },
    { SSC_INPUT,			SSC_NUMBER,			"balance_of_system_cost_total",						"BOS costs",									"$",				"",             "MHKTidal",         "?=1",                      "",				"" },
    { SSC_INPUT,			SSC_NUMBER,			"financial_cost_total",						"Financial costs",									"$",				"",             "MHKTidal",         "?=1",                      "",				"" },
    { SSC_INPUT,			SSC_NUMBER,			"total_operating_cost",						"O&M costs",									"$",				"",             "MHKTidal",         "?=1",                      "",				"" },
    { SSC_INPUT,			SSC_NUMBER,			"system_capacity",						"System Nameplate Capacity",										"kW",			"",				"MHKTidal",			"?=0",						"",							"" },
    { SSC_INPUT,         SSC_NUMBER,      "tidal_resource_model_choice",           "Resource distribution or time series tidal resource data",                                 "0/1",             "",             "MHKTidal",          "?=0",                         "INTEGER",                  "" },
    { SSC_INPUT,        SSC_ARRAY,       "tidal_velocity",           "Tidal velocity",                                        "m/s",     "",                       "MHKTidal",      "?",                        "",                            "" },


	// losses
	{ SSC_INPUT,			SSC_NUMBER,			"loss_array_spacing",				"Array spacing loss",													"%",			"",				"MHKTidal",			"*",		"",						"" },
	{ SSC_INPUT,			SSC_NUMBER,			"loss_resource_overprediction",				"Resource overprediction loss",													"%",			"",				"MHKTidal",			"*",		"",						"" },
	{ SSC_INPUT,			SSC_NUMBER,			"loss_transmission",				"Transmission losses",													"%",			"",				"MHKTidal",			"*",		"",						"" },
	{ SSC_INPUT,			SSC_NUMBER,			"loss_downtime",				"Array/WEC downtime loss",													"%",			"",				"MHKTidal",			"*",		"",						"" },
	{ SSC_INPUT,			SSC_NUMBER,			"loss_additional",				"Additional losses",													"%",			"",				"MHKTidal",			"*",		"",						"" },


//	{ SSC_OUTPUT,			SSC_NUMBER,			"device_rated_capacity",				"Rated capacity of device",													"kW",			"",				"MHKTidal",			"calculate_capacity=0",		"",						"" },
	{ SSC_OUTPUT,			SSC_NUMBER,			"device_rated_capacity",				"Rated capacity of device",													"kW",			"",				"MHKTidal",			"",		"",						"" },
	{ SSC_OUTPUT,			SSC_NUMBER,			"device_average_power",					"Average power production of a single device",								"kW",			"",				"MHKTidal",			"*",						"",						"" },
	{ SSC_OUTPUT,			SSC_NUMBER,			"annual_energy",						"Annual AC energy in Year 1",										"kWh",			"",				"MHKTidal",			"*",						"",						"" },
    { SSC_OUTPUT,			SSC_ARRAY,			"gen",			                        "System power generated",					"kW",			"",				"MHKTidal",			"",						"",						"" },

    { SSC_OUTPUT,			SSC_NUMBER,			"capacity_factor",						"Capacity factor",													"%",			"",				"MHKTidal",			"*",						"",						"" },
	{ SSC_OUTPUT,			SSC_ARRAY,			"annual_energy_distribution",			"Annual energy production of array as function of speed",					"kWh",			"",				"MHKTidal",			"",						"",						"" },
	{ SSC_OUTPUT,			SSC_ARRAY,			"annual_cumulative_energy_distribution","Cumulative annual energy production of array as function of speed",		"kWh",			"",				"MHKTidal",			"",						"",						"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"tidal_resource_start_velocity",        "First tidal velocity where probability distribution is greater than 0 ",   "m/s",			"",				"MHKTidal",			"*",						"",						"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"tidal_resource_end_velocity",          "Last tidal velocity where probability distribution is greater than 0 ",	"m/s",			"",				"MHKTidal",			"*",						"",						"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"tidal_power_start_velocity",           "First tidal velocity where power curve is greater than 0 ",		        "m/s",			"",				"MHKTidal",			"*",						"",						"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"tidal_power_end_velocity",             "Last tidal velocity where power curve is greater than 0 ",		            "m/s",			"",				"MHKTidal",			"*",						"",						"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"total_capital_cost_kwh",               "Capital costs per unit annual energy",		"$/kWh",			"",				"MHKTidal",			"*",						"",						"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"total_device_cost_kwh",                "Device costs per unit annual energy",		"$/kWh",			"",				"MHKTidal",			"*",						"",						"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"total_bos_cost_kwh",                   "Balance of system costs per unit annual energy",		"$/kWh",			"",				"MHKTidal",			"*",						"",						"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"total_financial_cost_kwh",             "Financial costs per unit annual energy",		"$/kWh",			"",				"MHKTidal",			"*",						"",						"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"total_om_cost_kwh",                    "O&M costs per unit annual energy",		"$/kWh",			"",				"MHKTidal",			"*",						"",						"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"total_capital_cost_lcoe",              "Capital cost as percentage of overall LCOE",		"%",			"",				"MHKTidal",			"*",						"",						"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"total_device_cost_lcoe",               "Device cost",		"%",			"",				"MHKTidal",			"*",						"",						"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"total_bos_cost_lcoe",                  "BOS cost",		"%",			"",				"MHKTidal",			"*",						"",						"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"total_financial_cost_lcoe",            "Financial cost",		"%",			"",				"MHKTidal",			"*",						"",						"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"total_om_cost_lcoe",                   "O&M cost (annual)",		"%",			"",				"MHKTidal",			"*",						"",						"" },
    //Cost per KW
    { SSC_OUTPUT,			SSC_NUMBER,			"total_capital_cost_per_kw",							"Capital cost per kW",										"$/kW",			"",								"MHKCosts",			"",						"",							"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"total_device_cost_per_kw",							"Device cost per kW",										"$/kW",			"",								"MHKCosts",			"",						"",							"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"total_bos_cost_per_kw",							"Balance of Systems cost per kW",										"$/kW",			"",								"MHKCosts",			"",						"",							"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"total_financial_cost_per_kw",							"Financial cost per kW",										"$/kW",			"",								"MHKCosts",			"",						"",							"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"total_operations_cost_per_kw",							"O&M cost per kW",										"$/kW",			"",								"MHKCosts",			"",						"",							"" },

    var_info_invalid
};


class cm_mhk_tidal : public compute_module
{
private:
public: 
	cm_mhk_tidal() {
		add_var_info(_cm_vtab_mhk_tidal);
        add_var_info(vtab_adjustment_factors);

	}
	
	void exec() {

	//Read and store tidal resource and power curve:
		util::matrix_t<double>  tidal_resource_matrix = as_matrix("tidal_resource");
		util::matrix_t<double>  tidal_power_curve = as_matrix("tidal_power_curve");
		
		//Check to ensure size of _power_vect == _speed_vect : 
		if ( tidal_power_curve.nrows() != tidal_resource_matrix.nrows() )
			throw exec_error("mhk_tidal", "Size of Power Curve is not equal to Tidal Resource");

		//Store the number of rows- this will have to change if resource and power curve can have different stream speeds
		int number_rows = (int)tidal_resource_matrix.nrows();

		//Check that the power matrix only has two columns
		if (tidal_power_curve.ncols() != (size_t)2)
			throw exec_error("mhk_tidal", "Power curve must contain two columns");

		//Check that the resource matrix has at least two columns
		if (tidal_power_curve.ncols() < (size_t)2)
			throw exec_error("mhk_tidal", "Resource matrix must have at least two columns");


		//Create vectors to store individual columns from the user input matrix "tidal_resource"
		//Size the vectors based on the resource matrix so that the length can change in the future
		std::vector<double> _speed_vect(number_rows);	// Stream speed (u [m/s])
		std::vector<double> _probability_vect(number_rows);	// Probability in decimals (i.e. 0.5, not 50%) at different depths***************************************************Need to program in the possibility of probabilities at multiple depths
		
		//Vector to store power curve of tidal energy conversion system from "tidal_power_curve":
		//Size the vector based on the tidal power matrix so that the length can be different from the resource in the future
		std::vector<double> _power_vect(number_rows);	//Tidal power curve (P [kW])

		
	//Initialize variables to store calculated values and outputs:
		ssc_number_t *p_annual_energy_dist = allocate("annual_energy_distribution", number_rows);
		ssc_number_t *p_annual_cumulative_energy_dist = allocate("annual_cumulative_energy_distribution", number_rows);
		double annual_energy = 0, device_average_power = 0, _probability_vect_checker = 0, capacity_factor = 0, device_rated_capacity = 0;
		
		//User either sets device_rated_capacity in the UI, or allows cmod to determine from power curve:
		if (is_assigned("device_rated_capacity"))
			device_rated_capacity = as_double("device_rated_capacity");
		else
			device_rated_capacity = 0.0;

		//Read number of devices
		int number_devices = as_integer("number_devices");

		// total loss
		double total_loss = as_double("loss_array_spacing")
			+ as_double("loss_resource_overprediction")
			+ as_double("loss_transmission")
			+ as_double("loss_downtime")
			+ as_double("loss_additional");

        int tidal_resource_model_choice = as_integer("tidal_resource_model_choice");
        double tidal_resource_start_velocity = 0;
        double tidal_power_start_velocity = 0;
        double tidal_resource_end_velocity = 0;
        double tidal_power_end_velocity = 0;
        if (tidal_resource_model_choice == 0) {
            //Storing each column of the tidal_resource_matrix and tidal_power_curve as vectors:
            double min_velocity, max_velocity = 0;
            min_velocity = tidal_resource_matrix.at(0, 0);
            max_velocity = tidal_resource_matrix.at(size_t(number_rows) - 1, 0);

            for (int i = 0; i < number_rows; i++) {
                size_t n = i;
                if (i != 0) {
                    if (tidal_resource_matrix.at(n, 1) != 0 && tidal_resource_matrix.at(n - 1, 1) == 0)
                    {
                        tidal_resource_start_velocity = tidal_resource_matrix.at(n, 0);
                    }
                    if (tidal_power_curve.at(n, 1) != 0 && tidal_power_curve.at(n - 1, 1) == 0 && n != 0)
                    {
                        tidal_power_start_velocity = tidal_power_curve.at(n, 0);
                    }
                }
                else {
                    if (tidal_resource_matrix.at(n, 1) != 0)
                    {
                        tidal_resource_start_velocity = tidal_resource_matrix.at(n, 0);
                    }
                    if (tidal_power_curve.at(n, 1) != 0)
                    {
                        tidal_power_start_velocity = tidal_power_curve.at(n, 0);
                    }
                }
                if (i != number_rows - 1) {
                    if (tidal_resource_matrix.at(n, 1) != 0 && tidal_resource_matrix.at(n + 1, 1) == 0 && n != 0)
                    {
                        tidal_resource_end_velocity = tidal_resource_matrix.at(n, 0);
                    }
                    if (tidal_power_curve.at(n, 1) != 0 && tidal_power_curve.at(n + 1, 1) == 0 && n != 0)
                    {
                        tidal_power_end_velocity = tidal_power_curve.at(n, 0);
                    }
                }
                else {
                    if (i == number_rows - 1 && tidal_resource_end_velocity == 0)
                    {
                        tidal_resource_end_velocity = tidal_resource_matrix.at(n, 0);
                    }
                    if (i == number_rows - 1 && tidal_power_end_velocity == 0)
                    {
                        tidal_power_end_velocity = tidal_power_curve.at(n, 0);
                    }
                }

                _speed_vect[i] = tidal_resource_matrix.at(i, 0);
                _probability_vect[i] = tidal_resource_matrix.at(i, 1); //*******************again need to modify to handle different depths
                _power_vect[i] = tidal_power_curve.at(i, 1);

                //Store max power if not set in UI:
                /*if (as_boolean("calculate_capacity")) */
                if (_power_vect[i] > device_rated_capacity)
                    device_rated_capacity = _power_vect[i];

                //Checker to ensure probability distribution adds to >= 99.5%:
                _probability_vect_checker += _probability_vect[i];

                //Calculate annual energy production at each stream speed bin:
                p_annual_energy_dist[i] = _power_vect[i] * _probability_vect[i] * number_devices * 8760 * (1 - total_loss/100.0);

                //Add current annual energy bin to total annual energy
                annual_energy += p_annual_energy_dist[i];


                p_annual_cumulative_energy_dist[i] = p_annual_energy_dist[i] + p_annual_cumulative_energy_dist[i - 1];

                //Contribution to Average Power from this speed bin 
                device_average_power += _power_vect[i] * _probability_vect[i];
            }
        }
        else {
            size_t number_records;
            ssc_number_t* tidal_velocity = as_array("tidal_velocity", &number_records);
            ssc_number_t* p_gen = allocate("gen", number_records);
            int power_bin = 0;

            adjustment_factors haf(this->get_var_table(), "adjust");
            if (!haf.setup(number_records, 1))
                throw exec_error("mhk_tidal", "Failed to set up adjustment factors: " + haf.error());

            //Find velocity bin of power array
            for (int i = 0; i < number_records; i++) {
                if (tidal_velocity[i] >= tidal_power_curve.at(tidal_power_curve.nrows() - 1, 0)) {
                        power_bin = tidal_power_curve.nrows() - 1;
                        continue;
                }
                else if (tidal_velocity[i] <= tidal_power_curve.at(0, 0)) {
                        power_bin = 0;
                        continue;
                }
                else {
                    for (int j = 1; j < tidal_power_curve.nrows(); j++) {
                        if (tidal_velocity[i] - tidal_power_curve.at(j - 1, 0) > 0 && tidal_velocity[i] - tidal_power_curve.at(j, 0) <= 0) {
                            power_bin = j;
                        }
        
                    }
                }
                p_gen[i] = tidal_power_curve.at(power_bin, 1) * (1 - total_loss/100.0) * number_devices * haf(i); //kW
                //p_annual_energy_dist[i] = p_gen[i] * 8760.0 / number_records;
                annual_energy += p_gen[i] * 8760.0 / number_records;
                p_annual_energy_dist[power_bin] += p_gen[i] * 8760.0 / number_records;
                
                //p_annual_cumulative_energy_dist[i] = p_annual_energy_dist[i] + p_annual_cumulative_energy_dist[i - 1];
                device_average_power += p_gen[i] / number_devices / number_records;
                for (int k = 0; k < tidal_power_curve.nrows(); k++) {
                    if (tidal_power_curve.at(k, 1) > device_rated_capacity)
                        device_rated_capacity = tidal_power_curve.at(k, 1);
                }
                
            }
            p_annual_cumulative_energy_dist[0] = p_annual_energy_dist[0];
            for (int i = 1; i < tidal_power_curve.nrows() - 1; i++) { 
                p_annual_cumulative_energy_dist[i] = p_annual_energy_dist[i] + p_annual_cumulative_energy_dist[i - 1];
            }
        }


				
		//Throw exception if frequency distribution vector sums to < 99.5%
        if (tidal_resource_model_choice == 0) {
            double probability_tolerance = 0.005;
            if (std::abs(1.0 - _probability_vect_checker) > probability_tolerance)
                throw exec_error("mhk_tidal", "Probability distribution vector does not add up to 100%.");
        }

		//Factoring in losses in total annual energy production:
		//annual_energy *= (1 - (total_loss / 100 ));
		// leave device power without losses
        if (is_assigned("device_costs_total")) {
            //TEST cost metrics in tidal page rather than cost page
            double device_cost = as_double("device_costs_total");
            double bos_cost = as_double("balance_of_system_cost_total");
            double financial_cost = as_double("financial_cost_total");
            double om_cost = as_double("total_operating_cost");
            double fcr = as_double("fixed_charge_rate");
            double total_capital_cost_kwh = fcr * (device_cost + bos_cost + financial_cost) / annual_energy;
            double total_device_cost_kwh = fcr * device_cost / annual_energy;
            double total_bos_cost_kwh = fcr * bos_cost / annual_energy;
            double total_financial_cost_kwh = fcr * financial_cost / annual_energy;
            double total_om_cost_kwh = om_cost / annual_energy;
            double total_capital_cost_lcoe = (fcr * (device_cost + bos_cost + financial_cost)) / (fcr * (device_cost + bos_cost + financial_cost) + om_cost) * 100;
            double total_device_cost_lcoe = (fcr * device_cost) / (fcr * (device_cost + bos_cost + financial_cost) + om_cost) * 100;
            double total_bos_cost_lcoe = (fcr * bos_cost) / (fcr * (device_cost + bos_cost + financial_cost) + om_cost) * 100;
            double total_financial_cost_lcoe = (fcr * financial_cost) / (fcr * (device_cost + bos_cost + financial_cost) + om_cost) * 100;
            double total_om_cost_lcoe = (om_cost) / (fcr * (device_cost + bos_cost + financial_cost) + om_cost) * 100;
            assign("total_capital_cost_kwh", var_data((ssc_number_t)total_capital_cost_kwh));
            assign("total_device_cost_kwh", var_data((ssc_number_t)total_device_cost_kwh));
            assign("total_bos_cost_kwh", var_data((ssc_number_t)total_bos_cost_kwh));
            assign("total_financial_cost_kwh", var_data((ssc_number_t)total_financial_cost_kwh));
            assign("total_om_cost_kwh", var_data((ssc_number_t)total_om_cost_kwh));
            assign("total_capital_cost_lcoe", var_data((ssc_number_t)total_capital_cost_lcoe));
            assign("total_device_cost_lcoe", var_data((ssc_number_t)total_device_cost_lcoe));
            assign("total_bos_cost_lcoe", var_data((ssc_number_t)total_bos_cost_lcoe));
            assign("total_financial_cost_lcoe", var_data((ssc_number_t)total_financial_cost_lcoe));
            assign("total_om_cost_lcoe", var_data((ssc_number_t)total_om_cost_lcoe));

            //Cost per kW system capacity
            double system_capacity = as_double("system_capacity");
            double capital_cost_kw = (device_cost + bos_cost + financial_cost) / system_capacity;
            double device_cost_kw = device_cost / system_capacity;
            double bos_cost_kw = bos_cost / system_capacity;
            double financial_cost_kw = financial_cost / system_capacity;
            double om_cost_kw = om_cost / system_capacity;
            assign("total_capital_cost_per_kw", var_data(ssc_number_t(capital_cost_kw)));
            assign("total_device_cost_per_kw", var_data(ssc_number_t(device_cost_kw)));
            assign("total_bos_cost_per_kw", var_data(ssc_number_t(bos_cost_kw)));
            assign("total_financial_cost_per_kw", var_data(ssc_number_t(financial_cost_kw)));
            assign("total_operations_cost_per_kw", var_data(ssc_number_t(om_cost_kw)));
        }

		//Calculating capacity factor:
		capacity_factor = annual_energy / (device_rated_capacity * number_devices * 8760);

		//Assigning values to outputs:
		assign("annual_energy", var_data((ssc_number_t)annual_energy));
		assign("device_average_power", var_data((ssc_number_t)device_average_power));
		assign("device_rated_capacity", var_data((ssc_number_t)device_rated_capacity));
		assign("capacity_factor", var_data((ssc_number_t)capacity_factor * 100));
        assign("tidal_resource_start_velocity", var_data((ssc_number_t)tidal_resource_start_velocity));
        assign("tidal_resource_end_velocity", var_data((ssc_number_t)tidal_resource_end_velocity));
        assign("tidal_power_start_velocity", var_data((ssc_number_t)tidal_power_start_velocity));
        assign("tidal_power_end_velocity", var_data((ssc_number_t)tidal_power_end_velocity));
        //assign("tidal_min_velocity", var_data((ssc_number_t)min_velocity));
        //assign("tidal_max_velocity", var_data((ssc_number_t)max_velocity));
	}
}; 

DEFINE_MODULE_ENTRY( mhk_tidal , "MHK Tidal power calculation model using power distribution.", 3);



