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

#include "csp_system_costs.h"
#include "csp_solver_util.h"


static var_info _cm_vtab_cb_mspt_system_costs[] = {

	
	
	{ SSC_INPUT,        SSC_NUMBER,      "A_sf",								"Total reflective solar field area",					"m2",           "",            "heliostat",      "*",        "",  "" },
	{ SSC_INPUT,        SSC_NUMBER,      "site_spec_cost",						"Site improvement cost",								"$/m2",         "",            "system_costs",   "*",        "",  "" },
	{ SSC_INPUT,        SSC_NUMBER,      "heliostat_spec_cost",					"Heliostat field cost",									"$/m2",         "",            "system_costs",   "*",        "",  "" },
	{ SSC_INPUT,        SSC_NUMBER,      "csp.pt.cost.fixed_sf",				"Heliostat field cost fixed",							"$",            "",            "system_costs",   "*",        "",  "" },
	
	{ SSC_INPUT,        SSC_NUMBER,      "h_tower",								"Tower height",											"m",            "",            "receiver",       "*",        "",  "" },
	{ SSC_INPUT,        SSC_NUMBER,      "H_rec",								"The height of the receiver",							"m",            "",            "receiver",       "*",        "",  "" },
    { SSC_INPUT,        SSC_NUMBER,      "helio_height",						"Heliostat height",										"m",            "",            "receiver",       "*",        "",  "" },
	{ SSC_INPUT,        SSC_NUMBER,      "tower_fixed_cost",					"Tower fixed cost",										"$",            "",            "system_costs",   "*",        "",  "" },
	{ SSC_INPUT,        SSC_NUMBER,      "tower_exp",							"Tower cost scaling exponent",							"",             "",            "system_costs",   "*",        "",  "" },
	
	{ SSC_INPUT,        SSC_NUMBER,      "csp.pt.cost.receiver.area",			"Receiver area",										"m2",           "",            "receiver",       "*",        "",  "" },
	{ SSC_INPUT,        SSC_NUMBER,      "rec_ref_cost",						"Receiver reference cost",								"$",            "",            "system_costs",   "*",        "",  "" },
	{ SSC_INPUT,        SSC_NUMBER,      "rec_ref_area",						"Receiver reference area for cost scale",				"",             "",            "system_costs",   "*",        "",  "" },
	{ SSC_INPUT,        SSC_NUMBER,      "rec_cost_exp",						"Receiver cost scaling exponent",						"",             "",            "system_costs",   "*",        "",  "" },

	{ SSC_INPUT,        SSC_NUMBER,      "csp.pt.cost.storage_mwht",			"Storage capacity",										"MWt-hr",       "",            "TES",            "*",        "",  "" },
	{ SSC_INPUT,        SSC_NUMBER,      "tes_spec_cost",						"Thermal energy storage cost",							"$/kWht",       "",            "system_costs",   "*",        "",  "" },
	
  	{ SSC_INPUT,        SSC_NUMBER,      "P_ref",								"Reference output electric power at design condition",  "MWe",          "",            "system_design",  "*",        "",  "" },	
	{ SSC_INPUT,        SSC_NUMBER,      "csp.pt.cost.power_block_per_kwe",		"Power cycle specific cost",							"$/kWe",        "",            "system_costs",   "*",        "",  "" },

	{ SSC_INPUT,        SSC_NUMBER,      "bop_spec_cost",						"BOP specific cost",									"$/kWe",        "",            "system_costs",   "*",        "",  "" },
	
	{ SSC_INPUT,        SSC_NUMBER,      "fossil_spec_cost",					"Fossil system specific cost",							"$/kWe",        "",            "system_costs",   "*",        "",  "" },

	{ SSC_INPUT,        SSC_NUMBER,      "contingency_rate",					"Contingency for cost overrun",							"%",            "",            "system_costs",   "*",        "",  "" },

	{ SSC_INPUT,        SSC_NUMBER,      "csp.pt.cost.total_land_area",			"Total land area",										"acre",         "",            "system_costs",   "*",        "",  "" },
	{ SSC_INPUT,        SSC_NUMBER,      "system_capacity",						"Nameplate capacity",									"MWe",          "",            "system_design",  "*",        "",  "" },    
	{ SSC_INPUT,        SSC_NUMBER,      "csp.pt.cost.epc.per_acre",			"EPC cost per acre",									"$/acre",       "",            "system_costs",   "*",        "",  "" },
	{ SSC_INPUT,        SSC_NUMBER,      "csp.pt.cost.epc.percent",				"EPC cost percent of direct",							"%",            "",            "system_costs",   "*",        "",  "" },
	{ SSC_INPUT,        SSC_NUMBER,      "csp.pt.cost.epc.per_watt",			"EPC cost per watt",									"$/W",          "",            "system_costs",   "*",        "",  "" },
	{ SSC_INPUT,        SSC_NUMBER,      "csp.pt.cost.epc.fixed",				"EPC fixed",											"$",            "",            "system_costs",   "*",        "",  "" },
	{ SSC_INPUT,        SSC_NUMBER,      "csp.pt.cost.plm.per_acre",			"PLM cost per acre",									"$/acre",       "",            "system_costs",   "*",        "",  "" },
	{ SSC_INPUT,        SSC_NUMBER,      "csp.pt.cost.plm.percent",				"PLM cost percent of direct",							"%",            "",            "system_costs",   "*",        "",  "" },
	{ SSC_INPUT,        SSC_NUMBER,      "csp.pt.cost.plm.per_watt",			"PLM cost per watt",									"$/W",          "",            "system_costs",   "*",        "",  "" },
	{ SSC_INPUT,        SSC_NUMBER,      "csp.pt.cost.plm.fixed",				"PLM fixed",											"$",            "",            "system_costs",   "*",        "",  "" },
	{ SSC_INPUT,        SSC_NUMBER,      "sales_tax_frac",						"Percent of cost to which sales tax applies",			"%",            "",            "system_costs",   "*",        "",  "" },
	{ SSC_INPUT,        SSC_NUMBER,      "sales_tax_rate",						"Sales tax rate",										"%",            "",            "system_costs",   "*",        "",  "" },

	{ SSC_OUTPUT,       SSC_NUMBER,      "csp.pt.cost.site_improvements",	    "Site improvement cost",								"$",            "",            "system_costs",   "*",        "",  "" },
	{ SSC_OUTPUT,       SSC_NUMBER,      "csp.pt.cost.heliostats",	            "Heliostat cost",										"$",            "",            "system_costs",   "*",        "",  "" },
	{ SSC_OUTPUT,       SSC_NUMBER,      "csp.pt.cost.tower",	                "Tower cost",											"$",            "",            "system_costs",   "*",        "",  "" },
	{ SSC_OUTPUT,       SSC_NUMBER,      "csp.pt.cost.receiver",	            "Receiver cost",										"$",            "",            "system_costs",   "*",        "",  "" },
	{ SSC_OUTPUT,       SSC_NUMBER,      "csp.pt.cost.storage",	                "TES cost",												"$",            "",            "system_costs",   "*",        "",  "" },
	{ SSC_OUTPUT,       SSC_NUMBER,      "csp.pt.cost.power_block",	            "Power cycle cost",										"$",            "",            "system_costs",   "*",        "",  "" },
	{ SSC_OUTPUT,       SSC_NUMBER,      "csp.pt.cost.bop",	                    "BOP cost",												"$",            "",            "system_costs",   "*",        "",  "" },
	{ SSC_OUTPUT,       SSC_NUMBER,      "csp.pt.cost.fossil",	                "Fossil backup cost",									"$",            "",            "system_costs",   "*",        "",  "" },
	{ SSC_OUTPUT,       SSC_NUMBER,      "ui_direct_subtotal",	                "Direct capital precontingency cost",					"$",            "",            "system_costs",   "*",        "",  "" },
	{ SSC_OUTPUT,       SSC_NUMBER,      "csp.pt.cost.contingency",	            "Contingency cost",										"$",            "",            "system_costs",   "*",        "",  "" },
	{ SSC_OUTPUT,       SSC_NUMBER,      "total_direct_cost",	                "Total direct cost",									"$",            "",            "system_costs",   "*",        "",  "" },
	{ SSC_OUTPUT,       SSC_NUMBER,      "csp.pt.cost.epc.total",	            "EPC and owner cost",									"$",            "",            "system_costs",   "*",        "",  "" },
	{ SSC_OUTPUT,       SSC_NUMBER,      "csp.pt.cost.plm.total",	            "Total land cost",										"$",            "",            "system_costs",   "*",        "",  "" },
	{ SSC_OUTPUT,       SSC_NUMBER,      "csp.pt.cost.sales_tax.total",	        "Sales tax cost",										"$",            "",            "system_costs",   "*",        "",  "" },
	{ SSC_OUTPUT,       SSC_NUMBER,      "total_indirect_cost",	                "Total indirect cost",									"$",            "",            "system_costs",   "*",        "",  "" },
	{ SSC_OUTPUT,       SSC_NUMBER,      "total_installed_cost",	            "Total installed cost",									"$",            "",            "system_costs",   "*",        "",  "" },
	{ SSC_OUTPUT,       SSC_NUMBER,      "csp.pt.cost.installed_per_capacity",  "Estimated installed cost per cap",						"$",            "",            "system_costs",   "*",        "",  "" },
	


	var_info_invalid };

class cm_cb_mspt_system_costs : public compute_module
{
public:

	cm_cb_mspt_system_costs()
	{
		add_var_info(_cm_vtab_cb_mspt_system_costs);
	}

	void exec() override
	{
		double A_sf_refl = as_double("A_sf");
		double site_improv_spec_cost = as_double("site_spec_cost");
		double heliostat_spec_cost = as_double("heliostat_spec_cost");
		double heliostat_fixed_cost = as_double("csp.pt.cost.fixed_sf");
        
		double h_tower = as_double("h_tower");
		double h_rec = as_double("H_rec");
		double h_helio = as_double("helio_height");
		double tower_fixed_cost = as_double("tower_fixed_cost");
		double tower_cost_scaling_exp = as_double("tower_exp");
        
		double A_rec = as_double("csp.pt.cost.receiver.area");
		double rec_ref_cost = as_double("rec_ref_cost");
		double A_rec_ref = as_double("rec_ref_area");
		double rec_cost_scaling_exp = as_double("rec_cost_exp");
        
		double Q_storage = as_double("csp.pt.cost.storage_mwht");
		double tes_spec_cost = as_double("tes_spec_cost");
		
		double W_dot_design = as_double("P_ref");
		double power_cycle_spec_cost = as_double("csp.pt.cost.power_block_per_kwe");
        
		double bop_spec_cost = as_double("bop_spec_cost");
        
		double fossil_backup_spec_cost = as_double("fossil_spec_cost");
        
		double contingency_rate = as_double("contingency_rate");
        
		double total_land_area = as_double("csp.pt.cost.total_land_area");
		double plant_net_capacity = as_double("system_capacity");
		double EPC_land_spec_cost = as_double("csp.pt.cost.epc.per_acre");
		double EPC_land_perc_direct_cost = as_double("csp.pt.cost.epc.percent");
		double EPC_land_per_power_cost = as_double("csp.pt.cost.epc.per_watt");
		double EPC_land_fixed_cost = as_double("csp.pt.cost.epc.fixed");
		double total_land_spec_cost = as_double("csp.pt.cost.plm.per_acre");
		double total_land_perc_direct_cost = as_double("csp.pt.cost.plm.percent");
		double total_land_per_power_cost = as_double("csp.pt.cost.plm.per_watt");
		double total_land_fixed_cost = as_double("csp.pt.cost.plm.fixed");
		double sales_tax_basis = as_double("sales_tax_frac");
		double sales_tax_rate = as_double("sales_tax_rate");

        double q_dot_heater_design = 0.0;
        double heater_spec_cost = 0.0;

        double rad_fluidcost = 0.0;
        double rad_installcost = 0.0;
        double rad_unitcost = 0.0;
        double rad_volmulti = 0.0;
        double coldstorage_unitcost = 0.0;
        double radfield_area = 0.0;
        double coldstorage_vol = 0.0;
        double radfield_vol = 0.0;

        double site_improvement_cost, heliostat_cost, tower_cost, receiver_cost, tes_cost, power_cycle_cost,
            bop_cost, fossil_backup_cost,
            direct_capital_precontingency_cost, contingency_cost, total_direct_cost, epc_and_owner_cost, total_land_cost,
            sales_tax_cost, total_indirect_cost, total_installed_cost, estimated_installed_cost_per_cap;

        site_improvement_cost = heliostat_cost = tower_cost = receiver_cost = tes_cost = power_cycle_cost =
            bop_cost = fossil_backup_cost =
            direct_capital_precontingency_cost = contingency_cost = total_direct_cost = epc_and_owner_cost = total_land_cost =
            sales_tax_cost = total_indirect_cost = total_installed_cost = estimated_installed_cost_per_cap = std::numeric_limits<double>::quiet_NaN();

        
        N_mspt::calculate_mspt__no_rad_cool__costs(
            A_sf_refl,
            site_improv_spec_cost,
            heliostat_spec_cost,
            heliostat_fixed_cost,

            h_tower,
            h_rec,
            h_helio,
            tower_fixed_cost,
            tower_cost_scaling_exp,

            A_rec,
            rec_ref_cost,
            A_rec_ref,
            rec_cost_scaling_exp,

            Q_storage,
            tes_spec_cost,

            W_dot_design,
            power_cycle_spec_cost,

            bop_spec_cost,

            fossil_backup_spec_cost,

            contingency_rate,

            total_land_area,
            plant_net_capacity,
            EPC_land_spec_cost,
            EPC_land_perc_direct_cost,
            EPC_land_per_power_cost,
            EPC_land_fixed_cost,
            total_land_spec_cost,
            total_land_perc_direct_cost,
            total_land_per_power_cost,
            total_land_fixed_cost,
            sales_tax_basis,
            sales_tax_rate,

            site_improvement_cost,
            heliostat_cost,
            tower_cost,
            receiver_cost,
            tes_cost,
            power_cycle_cost,
            bop_cost,
            fossil_backup_cost,
            direct_capital_precontingency_cost,
            contingency_cost,
            total_direct_cost,
            total_land_cost,
            epc_and_owner_cost,
            sales_tax_cost,
            total_indirect_cost,
            total_installed_cost,
            estimated_installed_cost_per_cap
        );

		assign("csp.pt.cost.site_improvements", (ssc_number_t)site_improvement_cost);
		assign("csp.pt.cost.heliostats", (ssc_number_t)heliostat_cost);
		assign("csp.pt.cost.tower", (ssc_number_t)tower_cost);
		assign("csp.pt.cost.receiver", (ssc_number_t)receiver_cost);
		assign("csp.pt.cost.storage", (ssc_number_t)tes_cost);
		assign("csp.pt.cost.power_block", (ssc_number_t)power_cycle_cost);
		assign("csp.pt.cost.bop", (ssc_number_t)bop_cost);
		assign("csp.pt.cost.fossil", (ssc_number_t)fossil_backup_cost);
		assign("ui_direct_subtotal", (ssc_number_t)direct_capital_precontingency_cost);
		assign("csp.pt.cost.contingency", (ssc_number_t)contingency_cost);
		assign("total_direct_cost", (ssc_number_t)total_direct_cost);
		assign("csp.pt.cost.epc.total", (ssc_number_t)epc_and_owner_cost);
		assign("csp.pt.cost.plm.total", (ssc_number_t)total_land_cost);
		assign("csp.pt.cost.sales_tax.total", (ssc_number_t)sales_tax_cost);
		assign("total_indirect_cost", (ssc_number_t)total_indirect_cost);
		assign("total_installed_cost", (ssc_number_t)total_installed_cost);
		assign("csp.pt.cost.installed_per_capacity", (ssc_number_t)estimated_installed_cost_per_cap);

	}

};

DEFINE_MODULE_ENTRY(cb_mspt_system_costs, "CSP molten salt power tower system costs", 0)
