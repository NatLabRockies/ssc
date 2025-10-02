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

enum MHK_DEVICE_TYPES { GENERIC, RM3, RM5, RM6, RM1, RM2, GENERIC_TIDAL };
enum MHK_TECHNOLOGY_TYPE { WAVE, TIDAL };



static var_info _cm_vtab_mhk_costs[] = {
	/*   VARTYPE			DATATYPE			NAME									   LABEL                                                   UNITS			META            GROUP              REQUIRED_IF            CONSTRAINTS                      UI_HINTS*/

	{ SSC_INPUT,			SSC_NUMBER,			"device_rated_power",						"Rated capacity of device",								"kW",			"",								"MHKCosts",			"*",					"MIN=0",					"" },
	{ SSC_INPUT,			SSC_NUMBER,			"system_capacity",							"System Nameplate Capacity",							"kW",			"",								"MHKCosts",			"*",					"MIN=0",					"" },
	{ SSC_INPUT,			SSC_NUMBER,			"devices_per_row",							"Number of wave devices per row in array",				"",				"",								"MHKCosts",         "*",                    "INTEGER",			    	"" },
//	{ SSC_INPUT,			SSC_NUMBER,			"device_type",								"Device Type",											"0/1/2/3/4",		"0=Generic,1=RM3,2=RM5,3=RM6,4=RM1",	"MHKCosts",			"?=0",					"MIN=0,MAX=4",				"" },
	{ SSC_INPUT,			SSC_NUMBER,			"marine_energy_tech",						"Marine energy technology",								"0/1",			"0=Wave,1=Tidal",				"MHKCosts",			"*",					"MIN=0,MAX=1",				"" },
	{ SSC_INPUT,			SSC_NUMBER,			"library_or_input_wec",						"Wave library or user input",								"",			"0=Library,1=User",				"MHKCosts",			"marine_energy_tech=0",					"",				"" },
	{ SSC_INPUT,			SSC_STRING,			"lib_wave_device",							"Wave library name",								"",			"",				"MHKCosts",			"marine_energy_tech=0",					"",				"" },
    { SSC_INPUT,			SSC_STRING,			"lib_tidal_device",							"Tidal library name",								"",			"",				"MHKCosts",			"marine_energy_tech=1",					"",				"" },

    { SSC_INPUT,			SSC_NUMBER,			"inter_array_cable_length",					"Inter-array cable length",								"m",			"",								"MHKCosts",			"*",					"MIN=0",					"" },
	{ SSC_INPUT,			SSC_NUMBER,			"riser_cable_length",						"Riser cable length",									"m",			"",								"MHKCosts",			"*",					"MIN=0",					"" },
	{ SSC_INPUT,			SSC_NUMBER,			"export_cable_length",						"Export cable length",									"m",			"",								"MHKCosts",			"*",					"MIN=0",					"" },

	// User input for CapEx dependent costs
		{ SSC_INPUT,			SSC_NUMBER,			"structural_assembly_cost_method",								"Structural assembly cost method",											"0/1/2",		"0=Enter in $/kW,1=Enter in $,2=Use modeled value,3=Use itemized costs in $",	"MHKCosts",			"*",					"MIN=0,MAX=4",				"" },
		{ SSC_INPUT,			SSC_NUMBER,			"structural_assembly_cost_input",								"Structural assembly cost",											"$",		"",	"MHKCosts",			"structural_assembly_cost_method<2",					"",				"" },
		{ SSC_INPUT,			SSC_NUMBER,			"structural_assembly_cost_total",								"Structural assembly itemized cost total",											"$",		"",	"MHKCosts",			"structural_assembly_cost_method=3",					"",				"" },
        { SSC_INPUT,			SSC_NUMBER,			"structural_assembly_cost_rvalue",								"Structural assembly R-value",											"",		"",	"MHKCosts",			"structural_assembly_cost_method=4",					"MIN=0,MAX=1",				"" },


        { SSC_INPUT,			SSC_NUMBER,			"power_takeoff_system_cost_method",								"Power take-off system cost method",											"0/1/2",		"0=Enter in $/kW,1=Enter in $,2=Use modeled value,3=Use itemized costs in $",	"MHKCosts",			"*",					"MIN=0,MAX=4",				"" },
		{ SSC_INPUT,			SSC_NUMBER,			"power_takeoff_system_cost_input",								"Power take-off system cost",											"$",		"",	"MHKCosts",			"power_takeoff_system_cost_method<2",					"",				"" },
		{ SSC_INPUT,			SSC_NUMBER,			"power_takeoff_system_cost_total",								"Power take-off system cost itemized cost total",											"$",		"",	"MHKCosts",			"power_takeoff_system_cost_method=3",					"",				"" },
        { SSC_INPUT,			SSC_NUMBER,			"structural_assembly_cost_rvalue",								"Power take-off system R-value",											"",		"",	"MHKCosts",			"power_takeoff_system_cost_method=4",					"MIN=0,MAX=1",				"" },


        { SSC_INPUT,			SSC_NUMBER,			"mooring_found_substruc_cost_method",								"Mooring, foundation, and substructure cost method",											"0/1/2",		"0=Enter in $/kW,1=Enter in $,2=Use modeled value,3=Use itemized costs in $",	"MHKCosts",			"*",					"MIN=0,MAX=4",				"" },
		{ SSC_INPUT,			SSC_NUMBER,			"mooring_found_substruc_cost_input",								"Mooring, foundation, and substructure cost",											"$",		"",	"MHKCosts",			"mooring_found_substruc_cost_method<2",					"",				"" },
		{ SSC_INPUT,			SSC_NUMBER,			"mooring_found_substruc_cost_total",								"Mooring, foundation, and substructure itemized cost total",											"$",		"",	"MHKCosts",			"mooring_found_substruc_cost_method=3",					"",				"" },
        { SSC_INPUT,			SSC_NUMBER,			"mooring_found_substruc_cost_rvalue",								"Mooring, foundation, and substructure R-value",											"",		"",	"MHKCosts",			"mooring_found_substruc_cost_method=4",					"MIN=0,MAX=1",				"" },

// User input BOS values
		{ SSC_INPUT,			SSC_NUMBER,			"development_cost_method",								"Development cost method",											"0/1/2",		"0=Enter in $/kW,1=Enter in $,2=Use modeled value,3=Enter in itemized costs",	"MHKCosts",			"*",					"MIN=0,MAX=4",				"" },
		{ SSC_INPUT,			SSC_NUMBER,			"development_cost_input",								"Development cost",											"$",		"",	"MHKCosts",			"development_cost_method<2",					"",				"" },
        { SSC_INPUT,			SSC_NUMBER,			"development_cost_total",								"Development itemized cost total",											"$",		"",	"MHKCosts",			"development_cost_method=3",					"",				"" },
        { SSC_INPUT,			SSC_NUMBER,			"development_cost_rvalue",								"Development R-value",											"",		"",	"MHKCosts",			"development_cost_method=4",					"MIN=0,MAX=1",				"" },

        { SSC_INPUT,			SSC_NUMBER,			"eng_and_mgmt_cost_method",								"Engineering and management cost method",											"0/1/2",		"0=Enter in $/kW,1=Enter in $,2=Use modeled value,3=Enter in itemized costs",	"MHKCosts",			"*",					"MIN=0,MAX=4",				"" },
		{ SSC_INPUT,			SSC_NUMBER,			"eng_and_mgmt_cost_input",								"Engineering and management cost",											"$",		"",	"MHKCosts",			"eng_and_mgmt_cost_method<2",					"",				"" },
        { SSC_INPUT,			SSC_NUMBER,			"eng_and_mgmt_cost_total",								"Engineering and management itemized cost total",											"$",		"",	"MHKCosts",			"eng_and_mgmt_cost_method=3",					"",				"" },
        { SSC_INPUT,			SSC_NUMBER,			"eng_and_mgmt_cost_rvalue",								"Engineering and management R-value",											"",		"",	"MHKCosts",			"eng_and_mgmt_cost_method=4",					"MIN=0,MAX=1",				"" },

        { SSC_INPUT,			SSC_NUMBER,			"assembly_and_install_cost_method",								"Assembly and installation cost method",											"0/1/2",		"0=Enter in $/kW,1=Enter in $,2=Use modeled value",	"MHKCosts",			"*",					"MIN=0,MAX=4",				"" },
		{ SSC_INPUT,			SSC_NUMBER,			"assembly_and_install_cost_input",								"Assembly and installation cost",											"$",		"",	"MHKCosts",			"assembly_and_install_cost_method<2",					"",				"" },
        { SSC_INPUT,			SSC_NUMBER,			"assembly_and_install_cost_total",								"Assembly and installation itemized cost total",											"$",		"",	"MHKCosts",			"assembly_and_install_cost_method=3",					"",				"" },
        { SSC_INPUT,			SSC_NUMBER,			"assembly_and_install_cost_rvalue",								"Assembly and installation R-value",											"",		"",	"MHKCosts",			"assembly_and_install_cost_method=4",					"MIN=0,MAX=1",				"" },

        { SSC_INPUT,			SSC_NUMBER,			"other_infrastructure_cost_method",								"Other infrastructure cost method",											"0/1/2",		"0=Enter in $/kW,1=Enter in $,2=Use modeled value",	"MHKCosts",			"*",					"MIN=0,MAX=4",				"" },
		{ SSC_INPUT,			SSC_NUMBER,			"other_infrastructure_cost_input",								"Other infrastructure cost",											"$",		"",	"MHKCosts",			"other_infrastructure_cost_method<2",					"",				"" },
        { SSC_INPUT,			SSC_NUMBER,			"other_infrastructure_cost_total",								"Other infrastructure itemized cost total",											"$",		"",	"MHKCosts",			"other_infrastructure_cost_method=3",					"",				"" },
        { SSC_INPUT,			SSC_NUMBER,			"other_infrastructure_cost_rvalue",								"Other infrastructure R-value",											"",		"",	"MHKCosts",			"other_infrastructure_cost_method=4",					"MIN=0,MAX=1",				"" },

        { SSC_INPUT,			SSC_NUMBER,			"elec_infras_cost_method",								"Electrical infrastructure cost method",											"0/1/2",		"0=Enter in $/kW,1=Enter in $,2=Use modeled value",	"MHKCosts",			"*",					"MIN=0,MAX=4",				"" },
        { SSC_INPUT,			SSC_NUMBER,			"elec_infras_cost_input",								"Electrical infrastructure cost",											"$",		"",	"MHKCosts",			"elec_infras_cost_method<2",					"",				"" },
        { SSC_INPUT,			SSC_NUMBER,			"elec_infras_cost_modeled",								"Electrical infrastructure cost modeled",											"$",		"",	"MHKCosts",			"elec_infras_cost_method=2",					"",				"" },
        { SSC_INPUT,			SSC_NUMBER,			"elec_infras_cost_total",								"Electrical infrastructure itemized cost total",											"$",		"",	"MHKCosts",			"elec_infras_cost_method=3",					"",				"" },
        { SSC_INPUT,			SSC_NUMBER,			"elec_infras_cost_rvalue",								"Electrical infrastructure R-value",											"",		"",	"MHKCosts",			"elec_infras_cost_method=4",					"MIN=0,MAX=1",				"" },

        //Plant Commissioning
        { SSC_INPUT,			SSC_NUMBER,			"plant_commissioning_cost_method",								"Plant commissioning cost method",											"0/1/2",		"0=Enter in $/kW,1=Enter in $,2=Use modeled value",	"MHKCosts",			"*",					"MIN=0,MAX=4",				"" },
        { SSC_INPUT,			SSC_NUMBER,			"plant_commissioning_cost_input",								"Plant commissioning cost",											"$",		"",	"MHKCosts",			"plant_commissioning_cost_method<2",					"",				"" },
        { SSC_INPUT,			SSC_NUMBER,			"plant_commissioning_cost_total",								"Plant commissioning itemized cost total",											"$",		"",	"MHKCosts",			"plant_commissioning_cost_method=3",					"",				"" },
        { SSC_INPUT,			SSC_NUMBER,			"plant_commissioning_cost_rvalue",								"Plant commissioning R-value",											"",		"",	"MHKCosts",			"plant_commissioning_cost_method=4",					"MIN=0,MAX=1",				"" },

        //Site Access
        { SSC_INPUT,			SSC_NUMBER,			"site_access_port_staging_cost_method",								"Site access and port staging cost method",											"0/1/2",		"0=Enter in $/kW,1=Enter in $,2=Use modeled value",	"MHKCosts",			"*",					"MIN=0,MAX=4",				"" },
        { SSC_INPUT,			SSC_NUMBER,			"site_access_port_staging_cost_input",								"Site access and port staging cost",											"$",		"",	"MHKCosts",			"site_access_port_staging_cost_method<2",					"",				"" },
        { SSC_INPUT,			SSC_NUMBER,			"site_access_port_staging_cost_total",								"Site access and port staging itemized cost total",											"$",		"",	"MHKCosts",			"site_access_port_staging_cost_method=3",					"",				"" },
        { SSC_INPUT,			SSC_NUMBER,			"site_access_port_staging_cost_rvalue",								"Site access and port staging R-value",											"",		"",	"MHKCosts",			"site_access_port_staging_cost_method=4",					"MIN=0,MAX=1",				"" },


        //Project Contingency
        { SSC_INPUT,			SSC_NUMBER,			"project_contingency_cost_method",								"Project contingency cost method",											"0/1/2",		"0=Enter in $/kW,1=Enter in $,2=Use modeled value",	"MHKCosts",			"*",					"MIN=0,MAX=4",				"" },
        { SSC_INPUT,			SSC_NUMBER,			"project_contingency_cost_input",								"Project contingency cost",											"$",		"",	"MHKCosts",			"project_contingency_cost_method<2",					"",				"" },
        { SSC_INPUT,			SSC_NUMBER,			"project_contingency_cost_rvalue",								"Project contingency R-value",											"",		"",	"MHKCosts",			"project_contingency_cost_method=3",					"MIN=0,MAX=1",				"" },

        //Insurance during construction
        { SSC_INPUT,			SSC_NUMBER,			"insurance_during_construction_cost_method",								"Insurance during construction cost method",											"0/1/2",		"0=Enter in $/kW,1=Enter in $,2=Use modeled value",	"MHKCosts",			"*",					"MIN=0,MAX=4",				"" },
        { SSC_INPUT,			SSC_NUMBER,			"insurance_during_construction_cost_input",								"Insurance during construction cost",											"$",		"",	"MHKCosts",			"insurance_during_construction_cost_method<2",					"",				"" },
        { SSC_INPUT,			SSC_NUMBER,			"insurance_during_construction_cost_rvalue",								"Insurance during construction R-value",											"",		"",	"MHKCosts",			"insurance_during_construction_cost_method=3",					"MIN=0,MAX=1",				"" },

        //Reserve Accounts
       { SSC_INPUT,			SSC_NUMBER,			"reserve_accounts_cost_method",								"Reserve accounts cost method",											"0/1/2",		"0=Enter in $/kW,1=Enter in $,2=Use modeled value",	"MHKCosts",			"*",					"MIN=0,MAX=4",				"" },
       { SSC_INPUT,			SSC_NUMBER,			"reserve_accounts_cost_input",								"Reserve accounts cost",											"$",		"",	"MHKCosts",			"reserve_accounts_cost_method<2",					"",				"" },
       { SSC_INPUT,			SSC_NUMBER,			"reserve_accounts_cost_total",								"Reserve accounts itemized cost total",											"$",		"",	"MHKCosts",			"reserve_accounts_cost_method=3",					"",				"" },
       { SSC_INPUT,			SSC_NUMBER,			"reserve_accounts_cost_rvalue",								"Reserve accounts R-value",											"",		"",	"MHKCosts",			"reserve_accounts_cost_method=4",					"MIN=0,MAX=1",				"" },

       //Other financial costs
        { SSC_INPUT,			SSC_NUMBER,			"other_financial_cost_method",								"Other financial cost method",											"0/1/2",		"0=Enter in $/kW,1=Enter in $,2=Use modeled value",	"MHKCosts",			"*",					"MIN=0,MAX=4",				"" },
        { SSC_INPUT,			SSC_NUMBER,			"other_financial_cost_input",								"Other financial cost",											"$,$/kW",		"",	"MHKCosts",			"other_financial_cost_method<2",					"",				"" },
        { SSC_INPUT,			SSC_NUMBER,			"other_financial_cost_rvalue",								"Other financial R-value",											"",		"",	"MHKCosts",			"other_financial_cost_method=3",					"MIN=0,MAX=1",				"" },

		/*{ SSC_INPUT,			SSC_NUMBER,			"array_cable_system_cost_method",								"Array cable system cost method",											"0/1/2",		"0=Enter in $/kW,1=Enter in $,2=Use modeled value",	"MHKCosts",			"*",					"MIN=0,MAX=4",				"" },
		{ SSC_INPUT,			SSC_NUMBER,			"array_cable_system_cost_input",								"Array cable system cost",											"$",		"",	"MHKCosts",			"*",					"",				"" },
		{ SSC_INPUT,			SSC_NUMBER,			"export_cable_system_cost_method",								"Export cable system cost method",											"0/1/2",		"0=Enter in $/kW,1=Enter in $,2=Use modeled value",	"MHKCosts",			"*",					"MIN=0,MAX=4",				"" },
		{ SSC_INPUT,			SSC_NUMBER,			"export_cable_system_cost_input",								"Export cable system cost",											"$",		"",	"MHKCosts",			"*",					"",				"" },
		{ SSC_INPUT,			SSC_NUMBER,			"onshore_substation_cost_method",								"Onshore substation cost method",											"0/1/2",		"0=Enter in $/kW,1=Enter in $,2=Use modeled value",	"MHKCosts",			"*",					"MIN=0,MAX=4",				"" },
		{ SSC_INPUT,			SSC_NUMBER,			"onshore_substation_cost_input",								"Onshore substation cost",											"$",		"",	"MHKCosts",			"*",					"",				"" },
		{ SSC_INPUT,			SSC_NUMBER,			"offshore_substation_cost_method",								"Offshore substation cost method",											"0/1/2",		"0=Enter in $/kW,1=Enter in $,2=Use modeled value",	"MHKCosts",			"*",					"MIN=0,MAX=4",				"" },
		{ SSC_INPUT,			SSC_NUMBER,			"offshore_substation_cost_input",								"Offshore substation cost",											"$",		"",	"MHKCosts",			"*",					"",				"" },
		{ SSC_INPUT,			SSC_NUMBER,			"other_elec_infra_cost_method",								"Other electrical infrastructure cost method",											"0/1/2",		"0=Enter in $/kW,1=Enter in $,2=Use modeled value",	"MHKCosts",			"*",					"MIN=0,MAX=4",				"" },
		{ SSC_INPUT,			SSC_NUMBER,			"other_elec_infra_cost_input",								"Other electrical infrastructure cost",											"$",		"",	"MHKCosts",			"*",					"",				"" },*/

	//CapEx costs
	{ SSC_OUTPUT,			SSC_NUMBER,			"structural_assembly_cost_modeled",			"Modeled structural assembly cost",						"$",			"",								"MHKCosts",			"",						"",							"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"structural_assembly_cost",			"Structural assembly cost",						"$",			"",								"MHKCosts",			"",						"",							"" },

    { SSC_OUTPUT,			SSC_NUMBER,			"power_takeoff_system_cost_modeled",		"Modeled power take-off cost",							"$",			"",								"MHKCosts",			"",						"",							"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"power_takeoff_system_cost",		"Power take-off cost",							"$",			"",								"MHKCosts",			"",						"",							"" },

    { SSC_OUTPUT,			SSC_NUMBER,			"mooring_found_substruc_cost_modeled",		"Modeled mooring, foundation, and substructure cost",	"$",			"",								"MHKCosts",			"",						"",							"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"mooring_found_substruc_cost",		"Mooring, foundation, and substructure cost",	"$",			"",								"MHKCosts",			"",						"",							"" },

	//Balance of system costs
	{ SSC_OUTPUT,			SSC_NUMBER,			"development_cost_modeled",					"Modeled development cost",								"$",			"",								"MHKCosts",			"",						"",							"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"development_cost",					"Development cost",								"$",			"",								"MHKCosts",			"",						"",							"" },

    { SSC_OUTPUT,			SSC_NUMBER,			"eng_and_mgmt_cost_modeled",				"Modeled engineering and management cost",				"$",			"",								"MHKCosts",			"",						"",							"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"eng_and_mgmt_cost",				"Engineering and management cost",				"$",			"",								"MHKCosts",			"",						"",							"" },

    { SSC_OUTPUT,			SSC_NUMBER,			"plant_commissioning_cost_modeled",			"Modeled plant commissioning cost",						"$",			"",								"MHKCosts",			"",						"",							"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"plant_commissioning_cost",			"Plant commissioning cost",						"$",			"",								"MHKCosts",			"",						"",							"" },

    { SSC_OUTPUT,			SSC_NUMBER,			"site_access_port_staging_cost_modeled",	"Modeled site access, port, and staging cost",			"$",			"",								"MHKCosts",			"",						"",							"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"site_access_port_staging_cost",	"Site access, port, and staging cost",			"$",			"",								"MHKCosts",			"",						"",							"" },

    { SSC_OUTPUT,			SSC_NUMBER,			"assembly_and_install_cost_modeled",		"Modeled assembly and installation cost",				"$",			"",								"MHKCosts",			"",						"",							"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"assembly_and_install_cost",		"Assembly and installation cost",				"$",			"",								"MHKCosts",			"",						"",							"" },

    { SSC_OUTPUT,			SSC_NUMBER,			"other_infrastructure_cost_modeled",		"Modeled other infrastructure cost",					"$",			"",								"MHKCosts",			"",						"",							"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"other_infrastructure_cost",		"Other infrastructure cost",					"$",			"",								"MHKCosts",			"",						"",							"" },

	//Electrical infrastructure costs
	/*{ SSC_OUTPUT,			SSC_NUMBER,			"array_cable_system_cost_modeled",			"Modeled array cable system cost",						"$",			"",								"MHKCosts",			"",						"",							"" },
	{ SSC_OUTPUT,			SSC_NUMBER,			"export_cable_system_cost_modeled",			"Modeled export cable system cost",						"$",			"",								"MHKCosts",			"",						"",							"" },
	{ SSC_OUTPUT,			SSC_NUMBER,			"onshore_substation_cost_modeled",			"Modeled onshore substation cost",						"$",			"",								"MHKCosts",			"",						"",							"" },
	{ SSC_OUTPUT,			SSC_NUMBER,			"offshore_substation_cost_modeled",			"Modeled offshore substation cost",						"$",			"",								"MHKCosts",			"",						"",							"" },
	{ SSC_OUTPUT,			SSC_NUMBER,			"other_elec_infra_cost_modeled",			"Modeled other electrical infrastructure cost",			"$",			"",								"MHKCosts",			"",						"",							"" },*/

	//Financial costs
	{ SSC_OUTPUT,			SSC_NUMBER,			"project_contingency",						"Modeled project contingency cost",						"$",			"",								"MHKCosts",			"",						"",							"" },
	{ SSC_OUTPUT,			SSC_NUMBER,			"insurance_during_construction",			"Modeled cost of insurance during construction",		"$",			"",								"MHKCosts",			"",						"",							"" },
	{ SSC_OUTPUT,			SSC_NUMBER,			"reserve_accounts",							"Modeled reserve account costs",						"$",			"",								"MHKCosts",			"",						"",							"" },

	//O and M costs
	{ SSC_OUTPUT,			SSC_NUMBER,			"operations_cost",							"Operations cost",										"$",			"",								"MHKCosts",			"",						"",							"" },
	{ SSC_OUTPUT,			SSC_NUMBER,			"maintenance_cost",							"Maintenance cost",										"$",			"",								"MHKCosts",			"",						"",							"" },
    var_info_invalid };



class cm_mhk_costs : public compute_module
{
public:
	cm_mhk_costs()
	{
		add_var_info(_cm_vtab_mhk_costs);
	}

	void exec()
	{
		//get inputs to compute module
		double device_rating = as_double("device_rated_power"); // kW
		double system_capacity_kW = as_double("system_capacity"); // kW
		double system_capacity_MW = system_capacity_kW / 1000.0; // MW
//		int device_type = as_integer("device_type");
		int technology = as_integer("marine_energy_tech");
		int devices_per_row = as_integer("devices_per_row");
		double interarray_length = as_double("inter_array_cable_length");
		double riser_length = as_double("riser_cable_length");
		double export_length = as_double("export_cable_length");


		int device_type = 4;
		if (technology == WAVE)
		{
			if (as_integer("library_or_input_wec") == 1)
				device_type = 0;
			else
			{
				std::string wave_device = as_string("lib_wave_device");
				if (wave_device == "RM3")
					device_type = 1;
				else if (wave_device == "RM5")
					device_type = 2;
				else if (wave_device == "RM6")
					device_type = 3;
				else
					device_type = 0;
			}
		}
        
        else
        {
            std::string tidal_device = as_string("lib_tidal_device");
            if (tidal_device == "RM1")
                device_type = RM1; //RM1
            else if (tidal_device == "RM2")
                device_type = RM2;
            else
                device_type = GENERIC_TIDAL;
        }

		//define intermediate variables to store calculated outputs
		double structural_assembly, power_takeoff, mooring_found_substruc;
		double development, eng_and_mgmt, plant_commissioning, site_access_port_staging, assembly_and_install, other_infrastructure;
		double array_cable_system, export_cable_system, onshore_substation, offshore_substation, other_elec_infra;
		double project_contingency, insurance_during_construction, reserve_accounts;
		double operations_cost, maintenance_cost;
		
		//Most CapEx costs depend on technology
		if (technology == TIDAL)
		{ // device = RM1
            if (device_type == RM1) {
                structural_assembly = 437807 * system_capacity_MW + 1136551;
                power_takeoff = 1911085.0 * system_capacity_MW + 641328.0;
                mooring_found_substruc = 441591.0 * system_capacity_MW + 769351.0;
                //BOS costs SAM Cost Model v8.xlsx
                development = 2882733 * pow(system_capacity_MW, 0.51);
                eng_and_mgmt = 1049336 * pow(system_capacity_MW, 0.56);
            }
            else if (device_type == RM2) {
                //REPLACE WITH ACTUAL RM2 COST CURVES
                structural_assembly = 2650166 * system_capacity_MW + 352232;
                power_takeoff = 5207150 * pow(system_capacity_MW, 0.64);
                mooring_found_substruc = 330163 * system_capacity_MW;
                //BOS costs SAM Cost Model v8.xlsx
                development = 2882733 * pow(system_capacity_MW, 0.51);
                eng_and_mgmt = 1049336 * pow(system_capacity_MW, 0.56);
            }
            else { //Generic Tidal
                structural_assembly = 1573876 * system_capacity_MW + 161960;
                power_takeoff = 3397389 * system_capacity_MW;
                mooring_found_substruc = 551697 * system_capacity_MW;
                //BOS costs SAM Cost Model v8.xlsx
                development = 2957847 * pow(system_capacity_MW, 0.51);
                eng_and_mgmt = 78127 * system_capacity_MW + 2325517;
            }
		}
		else // wave
		{
			if (device_type == RM3)
			{
				structural_assembly = 8435164.0 * system_capacity_MW + 3235294.0;
				power_takeoff = 2643426.0 * pow(system_capacity_MW, 0.91);
				mooring_found_substruc = 2247873.0 * system_capacity_MW;
				//BOS costs SAM Cost Model v8.xlsx
                development = 2882733 * pow(system_capacity_MW, 0.51);
                eng_and_mgmt = 1049336 * pow(system_capacity_MW, 0.56);
			}

			else if (device_type == RM5)
			{
				structural_assembly = 8437834.0 * system_capacity_MW + 4084789.0;
				power_takeoff = 1972483.0 * pow(system_capacity_MW, 0.91);
				mooring_found_substruc = 2689321.0 * system_capacity_MW + 439357.0;
				//BOS costs SAM Cost Model v8.xlsx
                development = 2882733 * pow(system_capacity_MW, 0.51);
                eng_and_mgmt = 1049336 * pow(system_capacity_MW, 0.56);
			}

			else if (device_type == RM6)
			{
				structural_assembly = 17442939.0 * system_capacity_MW + 8221362.0;
				power_takeoff = 4903325.0 * pow(system_capacity_MW, 0.78);
				mooring_found_substruc = 2659397.0 * system_capacity_MW + 588685.0;
				//BOS costs SAM Cost Model v8.xlsx
                development = 2882733 * pow(system_capacity_MW, 0.51);
                eng_and_mgmt = 1049336 * pow(system_capacity_MW, 0.56);
			}

			else //generic model applies to everything else
			{
				structural_assembly = 7994094.0 * system_capacity_MW + 2502878;
				power_takeoff = 1970813.0 * system_capacity_MW + 2456226.0;
				mooring_found_substruc = 430255.0 * system_capacity_MW;
				//BOS costs SAM Cost Model v8.xlsx
                development = 2882733.0 * pow(system_capacity_MW, 0.51);
                eng_and_mgmt = 1049336 * pow(system_capacity_MW, 0.56);
			}
		}

		// REmaining BOS costs that are not CapEx dependent and not technology dependent
		assembly_and_install = 2973817 * pow(system_capacity_MW, 0.67);
		other_infrastructure = 0;

		//electrical infrastructure costs
		/*array_cable_system = (4.40 * (device_rating * devices_per_row / 1000.0) + 162.81) * interarray_length 
			+ (4.40 * (device_rating / 1000.0) + 162.81) * riser_length;
		export_cable_system = (4.40 * system_capacity_MW + 162.81) * export_length;
		onshore_substation = 75000.0 * system_capacity_MW;
		offshore_substation = 100000.0 * system_capacity_MW;
		other_elec_infra = 47966.16 * system_capacity_MW + 665841.0;*/
        double elec_infras = as_double("elec_infras_cost_modeled");

		// operations cost
		operations_cost = 46157.0 * system_capacity_MW + 1078155.0;

		// maintenance cost
		maintenance_cost = 79926.0 * system_capacity_MW + 2206357.0;

		//at this point, we need to assign the "independent" modeled outputs- 
		//i.e., we want the modeled value to be reported prior to overwriting with a user input
		//for all variables that are NOT dependent on CapEx
		assign("structural_assembly_cost_modeled", var_data(static_cast<ssc_number_t>(structural_assembly)));
		assign("power_takeoff_system_cost_modeled", var_data(static_cast<ssc_number_t>(power_takeoff)));
		assign("mooring_found_substruc_cost_modeled", var_data(static_cast<ssc_number_t>(mooring_found_substruc)));
		assign("development_cost_modeled", var_data(static_cast<ssc_number_t>(development)));
		assign("eng_and_mgmt_cost_modeled", var_data(static_cast<ssc_number_t>(eng_and_mgmt)));
		assign("assembly_and_install_cost_modeled", var_data(static_cast<ssc_number_t>(assembly_and_install)));
		assign("other_infrastructure_cost_modeled", var_data(static_cast<ssc_number_t>(other_infrastructure)));
		/*assign("array_cable_system_cost_modeled", var_data(static_cast<ssc_number_t>(array_cable_system)));
		assign("export_cable_system_cost_modeled", var_data(static_cast<ssc_number_t>(export_cable_system)));
		assign("onshore_substation_cost_modeled", var_data(static_cast<ssc_number_t>(onshore_substation)));
		assign("offshore_substation_cost_modeled", var_data(static_cast<ssc_number_t>(offshore_substation)));
		assign("other_elec_infra_cost_modeled", var_data(static_cast<ssc_number_t>(other_elec_infra)));*/
		assign("operations_cost", var_data(static_cast<ssc_number_t>(operations_cost)));
		assign("maintenance_cost", var_data(static_cast<ssc_number_t>(maintenance_cost)));

		// there are five cost values that are a percentage of total CapEx
		// we want those modeled values to reflect user-input values that are overwriting the modeled values
		// therefore, here, we replace modeled values calculated above with those input by the user, if that's selected in the UI
		int structural_assembly_cost_method = as_integer("structural_assembly_cost_method");
		int power_takeoff_system_cost_method = as_integer("power_takeoff_system_cost_method");
		int mooring_found_substruc_cost_method = as_integer("mooring_found_substruc_cost_method");

		int development_cost_method = as_integer("development_cost_method");
		int eng_and_mgmt_cost_method = as_integer("eng_and_mgmt_cost_method");
		int assembly_and_install_cost_method = as_integer("assembly_and_install_cost_method");
		int other_infrastructure_cost_method = as_integer("other_infrastructure_cost_method");
        int elec_infras_cost_method = as_integer("elec_infras_cost_method");
        int plant_commissioning_cost_method = as_integer("plant_commissioning_cost_method");
        int site_access_port_staging_cost_method = as_integer("site_access_port_staging_cost_method");

        int project_contingency_cost_method = as_integer("project_contingency_cost_method");
        int insurance_during_construction_cost_method = as_integer("insurance_during_construction_cost_method");
        int reserve_accounts_cost_method = as_integer("reserve_accounts_cost_method");
        int other_financial_cost_method = as_integer("other_financial_cost_method");

		/*int array_cable_system_cost_method = as_integer("array_cable_system_cost_method");
		int export_cable_system_cost_method = as_integer("export_cable_system_cost_method");
		int onshore_substation_cost_method = as_integer("onshore_substation_cost_method");
		int offshore_substation_cost_method = as_integer("offshore_substation_cost_method");
		int other_elec_infra_cost_method = as_integer("other_elec_infra_cost_method");*/

		// check for user entered values
		if (structural_assembly_cost_method == 0)
			structural_assembly = as_double("structural_assembly_cost_input") * system_capacity_kW;
		else if (structural_assembly_cost_method == 1)
			structural_assembly = as_double("structural_assembly_cost_input");
		else if (structural_assembly_cost_method == 3)
			structural_assembly = as_double("structural_assembly_cost_total");
        else if (structural_assembly_cost_method == 4)
            structural_assembly = as_double("structural_assembly_cost_input") * -1 * std::log(1.0 - as_double("structural_assembly_cost_rvalue")) / std::log(2.0);
        assign("structural_assembly_cost", var_data(static_cast<ssc_number_t>(structural_assembly)));

        if (power_takeoff_system_cost_method == 0)
			power_takeoff = as_double("power_takeoff_system_cost_input") * system_capacity_kW;
		else if (power_takeoff_system_cost_method == 1)
			power_takeoff = as_double("power_takeoff_system_cost_input");
		else if (power_takeoff_system_cost_method == 3)
			power_takeoff = as_double("power_takeoff_system_cost_total");
        else if (power_takeoff_system_cost_method == 4)
            power_takeoff = as_double("power_takeoff_system_cost_input") * -1 * std::log(1.0 - as_double("power_takeoff_system_cost_rvalue")) / std::log(2.0);
        assign("power_takeoff_system_cost", var_data(static_cast<ssc_number_t>(power_takeoff)));


		if (mooring_found_substruc_cost_method == 0)
			mooring_found_substruc = as_double("mooring_found_substruc_cost_input") * system_capacity_kW;
		else if (mooring_found_substruc_cost_method == 1)
			mooring_found_substruc = as_double("mooring_found_substruc_cost_input");
		else if (mooring_found_substruc_cost_method == 3)
			mooring_found_substruc = as_double("mooring_found_substruc_cost_total");
        else if (mooring_found_substruc_cost_method == 4)
            mooring_found_substruc = as_double("mooring_found_substruc_cost_input") * -1 * std::log(1.0 - as_double("mooring_found_substruc_cost_rvalue")) / std::log(2.0);
        assign("mooring_found_substruc_cost", var_data(static_cast<ssc_number_t>(mooring_found_substruc)));

		if (development_cost_method == 0)
			development = as_double("development_cost_input") * system_capacity_kW;
		else if (development_cost_method == 1)
			development = as_double("development_cost_input");
        else if (development_cost_method == 3)
            development = as_double("development_cost_total");
        else if (development_cost_method == 4)
            development = as_double("development_cost_input") * -1 * std::log(1.0 - as_double("development_cost_rvalue")) / std::log(2.0);
        assign("development_cost", var_data(static_cast<ssc_number_t>(development)));


		if (eng_and_mgmt_cost_method == 0)
			eng_and_mgmt = as_double("eng_and_mgmt_cost_input") * system_capacity_kW;
		else if (eng_and_mgmt_cost_method == 1)
			eng_and_mgmt = as_double("eng_and_mgmt_cost_input");
        else if (eng_and_mgmt_cost_method == 3)
            eng_and_mgmt = as_double("eng_and_mgmt_cost_total");
        else if (eng_and_mgmt_cost_method == 4)
            eng_and_mgmt = as_double("eng_and_mgmt_cost_input") * -1 * std::log(1.0 - as_double("eng_and_mgmt_cost_rvalue")) / std::log(2.0);
        assign("eng_and_mgmt_cost", var_data(static_cast<ssc_number_t>(eng_and_mgmt)));


		if (assembly_and_install_cost_method == 0)
			assembly_and_install = as_double("assembly_and_install_cost_input") * system_capacity_kW;
		else if (assembly_and_install_cost_method == 1)
			assembly_and_install = as_double("assembly_and_install_cost_input");
        else if (assembly_and_install_cost_method == 3)
            assembly_and_install = as_double("assembly_and_install_cost_total");
        else if (assembly_and_install_cost_method == 4)
            assembly_and_install = as_double("assembly_and_install_cost_input") * -1 * std::log(1.0 - as_double("assembly_and_install_cost_rvalue")) / std::log(2.0);
        assign("assembly_and_install_cost", var_data(static_cast<ssc_number_t>(assembly_and_install)));

		if (other_infrastructure_cost_method == 0)
			other_infrastructure = as_double("other_infrastructure_cost_input") * system_capacity_kW;
		else if (other_infrastructure_cost_method == 1)
			other_infrastructure = as_double("other_infrastructure_cost_input");
        else if (other_infrastructure_cost_method == 3)
            other_infrastructure = as_double("other_infrastructure_cost_total");
        else if (other_infrastructure_cost_method == 4)
            other_infrastructure = as_double("other_infrastructure_cost_input") * -1 * std::log(1.0 - as_double("other_infrastructure_cost_rvalue")) / std::log(2.0);
        assign("other_infrastructure_cost", var_data(static_cast<ssc_number_t>(other_infrastructure)));

        if (elec_infras_cost_method == 0)
            elec_infras = as_double("elec_infras_cost_input") * system_capacity_kW;
        else if (elec_infras_cost_method == 1)
            elec_infras = as_double("elec_infras_cost_input");
        else if (elec_infras_cost_method == 3)
            elec_infras = as_double("elec_infras_cost_total");
        else if (other_infrastructure_cost_method == 4)
            elec_infras = as_double("elec_infras_cost_input") * -1 * std::log(1.0 - as_double("elec_infras_cost_rvalue")) / std::log(2.0);
        assign("elec_infras_cost", var_data(static_cast<ssc_number_t>(elec_infras)));


		/*if (array_cable_system_cost_method == 0)
			array_cable_system = as_double("array_cable_system_cost_input") * system_capacity_kW;
		else if (array_cable_system_cost_method == 1)
			array_cable_system = as_double("array_cable_system_cost_input");
		if (export_cable_system_cost_method == 0)
			export_cable_system = as_double("export_cable_system_cost_input") * system_capacity_kW;
		else if (export_cable_system_cost_method == 1)
			export_cable_system = as_double("export_cable_system_cost_input");
		if (onshore_substation_cost_method == 0)
			onshore_substation = as_double("onshore_substation_cost_input") * system_capacity_kW;
		else if (onshore_substation_cost_method == 1)
			onshore_substation = as_double("onshore_substation_cost_input");
		if (offshore_substation_cost_method == 0)
			offshore_substation = as_double("offshore_substation_cost_input") * system_capacity_kW;
		else if (offshore_substation_cost_method == 1)
			offshore_substation = as_double("offshore_substation_cost_input");
		if (other_elec_infra_cost_method == 0)
			other_elec_infra = as_double("other_elec_infra_cost_input") * system_capacity_kW;
		else if (other_elec_infra_cost_method == 1)
			other_elec_infra = as_double("other_elec_infra_cost_input");*/


        /*plant_commissioning = 56103 * system_capacity_MW;
        site_access_port_staging = 75462 * system_capacity_MW;*/


		// Now, we calculated the CapEx using whatever combination of modeled values and user-entered values
		// that we have at this point.
		// CapEx is defined to include all device costs and BOS costs that are not CapEx dependent
		double capex = structural_assembly + power_takeoff + mooring_found_substruc
			+ development + eng_and_mgmt + assembly_and_install + other_infrastructure
			+ elec_infras;

        plant_commissioning = 0.016 * capex;
        assign("plant_commissioning_cost_modeled", var_data(static_cast<ssc_number_t>(plant_commissioning)));
        if (plant_commissioning_cost_method == 0)
            plant_commissioning = as_double("plant_commissioning_cost_input") * system_capacity_kW;
        else if (plant_commissioning_cost_method == 1)
            plant_commissioning = as_double("plant_commissioning_cost_input");
        else if (plant_commissioning_cost_method == 3)
            plant_commissioning = as_double("plant_commissioning_cost_input") * -1 * std::log(1.0 - as_double("plant_commissioning_cost_rvalue")) / std::log(2.0);

        site_access_port_staging = 0.011 * capex;
        assign("site_access_port_staging_cost_modeled", var_data(static_cast<ssc_number_t>(site_access_port_staging)));
        if (site_access_port_staging_cost_method == 0)
            site_access_port_staging = as_double("site_access_port_staging_cost_input") * system_capacity_kW;
        else if (site_access_port_staging_cost_method == 1)
            site_access_port_staging = as_double("site_access_port_staging_cost_input");
        else if (site_access_port_staging_cost_method == 3)
            site_access_port_staging = as_double("site_access_port_staging_cost_total");
        else if (site_access_port_staging_cost_method == 4)
            site_access_port_staging = as_double("site_access_port_staging_cost_input") * -1 * std::log(1.0 - as_double("site_access_port_staging_cost_rvalue")) / std::log(2.0);

        // Calculate the CapEx-dependent financial costs
		project_contingency = 0.081 * capex;
        assign("project_contingency_cost_modeled", var_data(static_cast<ssc_number_t>(project_contingency)));
        if (project_contingency_cost_method == 0)
            project_contingency = as_double("project_contingency_cost_input") * system_capacity_kW;
        else if (project_contingency_cost_method == 1)
            project_contingency = as_double("project_contingency_cost_input");
        else if (project_contingency_cost_method == 3)
            project_contingency = as_double("project_contingency_cost_input") * -1 * std::log(1.0 - as_double("project_contingency_cost_rvalue")) / std::log(2.0);

        insurance_during_construction = 0.013 * capex;
        assign("insurance_during_construction_cost_modeled", var_data(static_cast<ssc_number_t>(insurance_during_construction)));
        if (insurance_during_construction_cost_method == 0)
            insurance_during_construction = as_double("insurance_during_construction_cost_input") * system_capacity_kW;
        else if (insurance_during_construction_cost_method == 1)
            insurance_during_construction = as_double("insurance_during_construction_cost_input");
        else if (insurance_during_construction_cost_method == 3)
            insurance_during_construction = as_double("insurance_during_construction_cost_input") * -1 * std::log(1.0 - as_double("insurance_during_construction_cost_rvalue")) / std::log(2.0);


        reserve_accounts = 0.031 * capex;
        assign("reserve_accounts_cost_modeled", var_data(static_cast<ssc_number_t>(reserve_accounts)));
        if (reserve_accounts_cost_method == 0)
            reserve_accounts = as_double("reserve_accounts_cost_input") * system_capacity_kW;
        else if (reserve_accounts_cost_method == 1)
            reserve_accounts = as_double("reserve_accounts_cost_input");
        else if (reserve_accounts_cost_method == 3)
            reserve_accounts = as_double("reserve_accounts_cost_total");
        else if (reserve_accounts_cost_method == 4)
            reserve_accounts = as_double("reserve_accounts_cost_input") * -1 * std::log(1.0 - as_double("reserve_accounts_cost_rvalue")) / std::log(2.0);
        

		// Assign the CapEx-dependent outputs
		assign("plant_commissioning_cost", var_data(static_cast<ssc_number_t>(plant_commissioning)));
		assign("site_access_port_staging_cost", var_data(static_cast<ssc_number_t>(site_access_port_staging)));
		assign("project_contingency_cost", var_data(static_cast<ssc_number_t>(project_contingency)));
		assign("insurance_during_construction_cost", var_data(static_cast<ssc_number_t>(insurance_during_construction)));
		assign("reserve_accounts_cost", var_data(static_cast<ssc_number_t>(reserve_accounts)));

		

		

		

	}

};

DEFINE_MODULE_ENTRY(mhk_costs, "Calculates various cost categories for Marine Energy arrays for different device types.", 3);
