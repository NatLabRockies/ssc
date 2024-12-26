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

#include "csp_system_costs.h"
#include "csp_solver_util.h"

#include <math.h>

void N_mspt::calculate_mspt_etes_costs(
    // Heliostat Field
    double A_sf_refl,				//[m^2] Total solar field reflective area
    double site_improv_spec_cost,	//[$/m^2_reflect] Site improvement specific cost
    double heliostat_spec_cost,		//[$/m^2_reflect] Heliostat specific cost
    double heliostat_fixed_cost,	//[$] Heliostat fixed cost

    // Tower
    double h_tower,					//[m] Tower height
    double h_rec,					//[m] Receiver height
    double h_helio,					//[m] Heliostat height
    double tower_fixed_cost,		//[$] Tower fixed cost
    double tower_cost_scaling_exp,	//[-] Tower cost scaling exponent

    // Receiver
    double A_rec,					//[m^2] Receiver area
    double rec_ref_cost,			//[$] Receiver reference cost
    double A_rec_ref,				//[m^2] Receiver reference area
    double rec_cost_scaling_exp,	//[-] Receiver cost scaling exponent

    // TES
    double Q_storage,				//[MWt-hr] Storage capacity
    double tes_spec_cost,			//[$/kWt-hr] TES specific cost

    // Cold Temp TES
    double Q_CT_tes,                //[MWt-hr] Cold Temp Storage capacity
    double CT_tes_spec_cost,        //[$/kWt-hr] CT TES specific cost

    // Power Cycle
    double W_dot_design,			//[MWe] Power cycle design output (w/o subtracting plant parasitics)
    double power_cycle_spec_cost,	//[$/kWe] Power cycle specific cost

    // Heater
    double q_dot_heater_design,     //[MWt] Heater design thermal power
    double heater_spec_cost,        //[$/kWe] Heater specific cost

    //Radiative cooling & cold storage
    double radfield_area,			//[m^2] Radiator field area if any 
    double coldstorage_vol,			//[m^3] Cold storage tank volume (total of all tanks)
    double radfield_vol,			//[m^3] Volume of fluid in radiator panels
    double rad_unitcost,			//[$/m^2] Radiator area
    double rad_installcost,			//[$/m^2] Radiator installation cost per square meter
    double rad_volmulti,			//[-]	Multiplier for the volume of fluid in delivery versus in radiators
    double rad_fluidcost,			//[$/L]	Cooling fluid unit cost
    double coldstorage_unitcost,	//[$/L] Cold storage construction cost

    // Balance Of Plant
    double bop_spec_cost,			//[$/kWe] BOP specific cost

    // Fossil Backup Cost
    double fossil_backup_spec_cost,	//[$/kWe] Fossil backup specific cost

    // Contingency Cost
    double contingency_rate,		//[%] Of pre-contingency direct capital costs

    // Indirect Capital Costs
    double total_land_area,			    //[acres]
    double plant_net_capacity,		    //[MWe] Nameplate plant capacity (Net cycle output less estimated parasitics)
    double EPC_land_spec_cost,		    //[$/acre]
    double EPC_land_perc_direct_cost,	//[%] Of calculated direct cost
    double EPC_land_per_power_cost,		//[$/We] Of plant net capacity
    double EPC_land_fixed_cost,		    //[$]
    double total_land_spec_cost,	    //[$/acre]
    double total_land_perc_direct_cost,	//[%] Of calculated direct cost
    double total_land_per_power_cost,	//[$/We] Of plant net capacity
    double total_land_fixed_cost,	    //[$]
    double sales_tax_basis,			    //[%] Of total direct cost
    double sales_tax_rate,			    //[%]

    // Calculated Outputs
    double& site_improvement_cost,
    double& heliostat_cost,
    double& tower_cost,
    double& receiver_cost,
    double& tes_cost,
    double& CT_tes_cost,
    double& power_cycle_cost,
    double& heater_cost,
    double& rad_field_totcost,
    double& rad_fluid_totcost,
    double& rad_storage_totcost,
    double& bop_cost,
    double& fossil_backup_cost,
    double& direct_capital_precontingency_cost,
    double& contingency_cost,
    double& total_direct_cost,
    double& total_land_cost,
    double& epc_and_owner_cost,
    double& sales_tax_cost,
    double& total_indirect_cost,
    double& total_installed_cost,
    double& estimated_installed_cost_per_cap)
{
    site_improvement_cost =
        N_mspt::site_improvement_cost(A_sf_refl, site_improv_spec_cost);

    heliostat_cost =
        N_mspt::heliostat_cost(A_sf_refl, heliostat_spec_cost, heliostat_fixed_cost);

    tower_cost =
        N_mspt::tower_cost(h_tower, h_rec, h_helio, tower_fixed_cost, tower_cost_scaling_exp);

    receiver_cost =
        N_mspt::receiver_cost(A_rec, rec_ref_cost, A_rec_ref, rec_cost_scaling_exp);

    tes_cost =
        N_mspt::tes_cost(Q_storage, tes_spec_cost);

    CT_tes_cost =
        N_mspt::tes_cost(Q_CT_tes, CT_tes_spec_cost);

    power_cycle_cost =
        N_mspt::power_cycle_cost(W_dot_design, power_cycle_spec_cost);

    heater_cost =
        N_mspt::heater_cost(q_dot_heater_design, heater_spec_cost);

    rad_field_totcost =
        N_mspt::rad_field_totcost(radfield_area, rad_unitcost, rad_installcost);

    rad_fluid_totcost =
        N_mspt::rad_fluid_totcost(radfield_vol, rad_fluidcost, rad_volmulti);

    rad_storage_totcost =
        N_mspt::rad_storage_totcost(coldstorage_vol, coldstorage_unitcost);

    bop_cost =
        N_mspt::bop_cost(W_dot_design, bop_spec_cost);

    fossil_backup_cost =
        N_mspt::fossil_backup_cost(W_dot_design, fossil_backup_spec_cost);

    direct_capital_precontingency_cost =
        N_mspt::direct_capital_precontingency_cost(
            site_improvement_cost,
            heliostat_cost,
            tower_cost,
            receiver_cost,
            tes_cost,
            CT_tes_cost,
            power_cycle_cost,
            heater_cost,
            rad_field_totcost,
            rad_fluid_totcost,
            rad_storage_totcost,
            bop_cost,
            fossil_backup_cost);

    contingency_cost =
        N_mspt::contingency_cost(contingency_rate, direct_capital_precontingency_cost);

    total_direct_cost =
        N_mspt::total_direct_cost(direct_capital_precontingency_cost, contingency_cost);

    total_land_cost =
        N_mspt::total_land_cost(total_land_area, total_direct_cost, plant_net_capacity,
            total_land_spec_cost, total_land_perc_direct_cost, total_land_per_power_cost, total_land_fixed_cost);

    epc_and_owner_cost =
        N_mspt::epc_and_owner_cost(total_land_area, total_direct_cost, plant_net_capacity,
            EPC_land_spec_cost, EPC_land_perc_direct_cost, EPC_land_per_power_cost, EPC_land_fixed_cost);

    sales_tax_cost =
        N_mspt::sales_tax_cost(total_direct_cost, sales_tax_basis, sales_tax_rate);

    total_indirect_cost =
        N_mspt::total_indirect_cost(total_land_cost, epc_and_owner_cost, sales_tax_cost);

    total_installed_cost =
        N_mspt::total_installed_cost(total_direct_cost, total_indirect_cost);

    estimated_installed_cost_per_cap =
        N_mspt::estimated_installed_cost_per_cap(total_installed_cost, plant_net_capacity);

    return;
}
void N_mspt::calculate_mspt_etes__no_rad_cool__costs(
    // Heliostat Field
    double A_sf_refl,				//[m^2] Total solar field reflective area
    double site_improv_spec_cost,	//[$/m^2_reflect] Site improvement specific cost
    double heliostat_spec_cost,		//[$/m^2_reflect] Heliostat specific cost
    double heliostat_fixed_cost,	//[$] Heliostat fixed cost

    // Tower
    double h_tower,					//[m] Tower height
    double h_rec,					//[m] Receiver height
    double h_helio,					//[m] Heliostat height
    double tower_fixed_cost,		//[$] Tower fixed cost
    double tower_cost_scaling_exp,	//[-] Tower cost scaling exponent

    // Receiver
    double A_rec,					//[m^2] Receiver area
    double rec_ref_cost,			//[$] Receiver reference cost
    double A_rec_ref,				//[m^2] Receiver reference area
    double rec_cost_scaling_exp,	//[-] Receiver cost scaling exponent

    // TES
    double Q_storage,				//[MWt-hr] Storage capacity
    double tes_spec_cost,			//[$/kWt-hr] TES specific cost

    // Cold Temp TES
    double Q_CT_tes,                //[MWt-hr] Cold Temp Storage capacity
    double CT_tes_spec_cost,        //[$/kWt-hr] CT TES specific cost

    // Power Cycle
    double W_dot_design,			//[MWe] Power cycle design output (w/o subtracting plant parasitics)
    double power_cycle_spec_cost,	//[$/kWe] Power cycle specific cost

    // Heater
    double q_dot_heater_design,     //[MWt] Heater design thermal power
    double heater_spec_cost,        //[$/kWe] Heater specific cost

    // Balance Of Plant
    double bop_spec_cost,			//[$/kWe] BOP specific cost

    // Fossil Backup Cost
    double fossil_backup_spec_cost,	//[$/kWe] Fossil backup specific cost

    // Contingency Cost
    double contingency_rate,		//[%] Of pre-contingency direct capital costs

    // Indirect Capital Costs
    double total_land_area,			    //[acres]
    double plant_net_capacity,		    //[MWe] Nameplate plant capacity (Net cycle output less estimated parasitics)
    double EPC_land_spec_cost,		    //[$/acre]
    double EPC_land_perc_direct_cost,	//[%] Of calculated direct cost
    double EPC_land_per_power_cost,		//[$/We] Of plant net capacity
    double EPC_land_fixed_cost,		    //[$]
    double total_land_spec_cost,	    //[$/acre]
    double total_land_perc_direct_cost,	//[%] Of calculated direct cost
    double total_land_per_power_cost,	//[$/We] Of plant net capacity
    double total_land_fixed_cost,	    //[$]
    double sales_tax_basis,			    //[%] Of total direct cost
    double sales_tax_rate,			    //[%]

    // Calculated Outputs
    double& site_improvement_cost,                  //[$]
    double& heliostat_cost,                         //[$]
    double& tower_cost,                             //[$]
    double& receiver_cost,                          //[$]
    double& tes_cost,                               //[$]
    double& CT_tes_cost,                            //[$]
    double& power_cycle_cost,                       //[$]
    double& heater_cost,                            //[$]
    double& bop_cost,                               //[$]
    double& fossil_backup_cost,                     //[$]
    double& direct_capital_precontingency_cost,     //[$]
    double& contingency_cost,                       //[$]
    double& total_direct_cost,                      //[$]
    double& total_land_cost,                        //[$]
    double& epc_and_owner_cost,                     //[$]
    double& sales_tax_cost,                         //[$]
    double& total_indirect_cost,                    //[$]
    double& total_installed_cost,                   //[$]
    double& estimated_installed_cost_per_cap        //[$/kWe]
)
{
    double rad_fluidcost = 0.0;
    double rad_installcost = 0.0;
    double rad_unitcost = 0.0;
    double rad_volmulti = 0.0;
    double coldstorage_unitcost = 0.0;
    double radfield_area = 0.0;
    double coldstorage_vol = 0.0;
    double radfield_vol = 0.0;

    // Unused outputs
    double rad_field_totcost, rad_fluid_totcost, rad_storage_totcost;

    N_mspt::calculate_mspt_etes_costs(
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

        Q_CT_tes,
        CT_tes_spec_cost,

        W_dot_design,
        power_cycle_spec_cost,

        q_dot_heater_design,
        heater_spec_cost,

        radfield_area,
        coldstorage_vol,
        radfield_vol,
        rad_unitcost,
        rad_installcost,
        rad_volmulti,
        rad_fluidcost,
        coldstorage_unitcost,

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
        CT_tes_cost,
        power_cycle_cost,
        heater_cost,
        rad_field_totcost,
        rad_fluid_totcost,
        rad_storage_totcost,
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
}

void N_mspt::calculate_mspt__no_rad_cool__costs(
    // Heliostat Field
    double A_sf_refl,				//[m^2] Total solar field reflective area
    double site_improv_spec_cost,	//[$/m^2_reflect] Site improvement specific cost
    double heliostat_spec_cost,		//[$/m^2_reflect] Heliostat specific cost
    double heliostat_fixed_cost,	//[$] Heliostat fixed cost

    // Tower
    double h_tower,					//[m] Tower height
    double h_rec,					//[m] Receiver height
    double h_helio,					//[m] Heliostat height
    double tower_fixed_cost,		//[$] Tower fixed cost
    double tower_cost_scaling_exp,	//[-] Tower cost scaling exponent

    // Receiver
    double A_rec,					//[m^2] Receiver area
    double rec_ref_cost,			//[$] Receiver reference cost
    double A_rec_ref,				//[m^2] Receiver reference area
    double rec_cost_scaling_exp,	//[-] Receiver cost scaling exponent

    // TES
    double Q_storage,				//[MWt-hr] Storage capacity
    double tes_spec_cost,			//[$/kWt-hr] TES specific cost

    // Power Cycle
    double W_dot_design,			//[MWe] Power cycle design output (w/o subtracting plant parasitics)
    double power_cycle_spec_cost,	//[$/kWe] Power cycle specific cost

    // Balance Of Plant
    double bop_spec_cost,			//[$/kWe] BOP specific cost

    // Fossil Backup Cost
    double fossil_backup_spec_cost,	//[$/kWe] Fossil backup specific cost

    // Contingency Cost
    double contingency_rate,		//[%] Of pre-contingency direct capital costs

    // Indirect Capital Costs
    double total_land_area,			    //[acres]
    double plant_net_capacity,		    //[MWe] Nameplate plant capacity (Net cycle output less estimated parasitics)
    double EPC_land_spec_cost,		    //[$/acre]
    double EPC_land_perc_direct_cost,	//[%] Of calculated direct cost
    double EPC_land_per_power_cost,		//[$/We] Of plant net capacity
    double EPC_land_fixed_cost,		    //[$]
    double total_land_spec_cost,	    //[$/acre]
    double total_land_perc_direct_cost,	//[%] Of calculated direct cost
    double total_land_per_power_cost,	//[$/We] Of plant net capacity
    double total_land_fixed_cost,	    //[$]
    double sales_tax_basis,			    //[%] Of total direct cost
    double sales_tax_rate,			    //[%]

    // Calculated Outputs
    double& site_improvement_cost,                  //[$]
    double& heliostat_cost,                         //[$]
    double& tower_cost,                             //[$]
    double& receiver_cost,                          //[$]
    double& tes_cost,                               //[$]
    double& power_cycle_cost,                       //[$]
    double& bop_cost,                               //[$]
    double& fossil_backup_cost,                     //[$]
    double& direct_capital_precontingency_cost,     //[$]
    double& contingency_cost,                       //[$]
    double& total_direct_cost,                      //[$]
    double& total_land_cost,                        //[$]
    double& epc_and_owner_cost,                     //[$]
    double& sales_tax_cost,                         //[$]
    double& total_indirect_cost,                    //[$]
    double& total_installed_cost,                   //[$]
    double& estimated_installed_cost_per_cap        //[$/kWe]
)
{
    double q_dot_heater_design = 0.0;
    double heater_spec_cost = 0.0;

    // no cold temp TES
    double Q_CT_tes = 0.0;
    double CT_tes_spec_cost = 0.0;

    double rad_fluidcost = 0.0;
    double rad_installcost = 0.0;
    double rad_unitcost = 0.0;
    double rad_volmulti = 0.0;
    double coldstorage_unitcost = 0.0;
    double radfield_area = 0.0;
    double coldstorage_vol = 0.0;
    double radfield_vol = 0.0;

    // Unused outputs
    double heater_cost, rad_field_totcost, rad_fluid_totcost, rad_storage_totcost;
    double CT_tes_cost;

    N_mspt::calculate_mspt_etes_costs(
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

        Q_CT_tes,
        CT_tes_spec_cost,

        W_dot_design,
        power_cycle_spec_cost,

        q_dot_heater_design,
        heater_spec_cost,

        radfield_area,
        coldstorage_vol,
        radfield_vol,
        rad_unitcost,
        rad_installcost,
        rad_volmulti,
        rad_fluidcost,
        coldstorage_unitcost,

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
        CT_tes_cost,
        power_cycle_cost,
        heater_cost,
        rad_field_totcost,
        rad_fluid_totcost,
        rad_storage_totcost,
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
}


void N_mspt::calculate_etes_costs(

    // TES
    double Q_storage,				//[MWt-hr] Storage capacity
    double tes_spec_cost,			//[$/kWt-hr] TES specific cost

    // Cold Temp TES
    double Q_CT_tes,                //[MWt-hr] Cold Temp Storage capacity
    double CT_tes_spec_cost,        //[$/kWt-hr] CT TES specific cost

    // Power Cycle
    double W_dot_design,			//[MWe] Power cycle design output (w/o subtracting plant parasitics)
    double power_cycle_spec_cost,	//[$/kWe] Power cycle specific cost

    // Heater
    double q_dot_heater_design,     //[MWt] Heater design thermal power
    double heater_spec_cost,        //[$/kWe] Heater specific cost

    // Balance Of Plant
    double bop_spec_cost,			//[$/kWe] BOP specific cost

    // Contingency Cost
    double contingency_rate,		//[%] Of pre-contingency direct capital costs

    // Indirect Capital Costs
    double plant_net_capacity,		    //[MWe] Nameplate plant capacity (Net cycle output less estimated parasitics)
    double EPC_land_perc_direct_cost,	//[%] Of calculated direct cost
    double EPC_land_per_power_cost,		//[$/We] Of plant net capacity
    double EPC_land_fixed_cost,		    //[$]
    double total_land_perc_direct_cost,	//[%] Of calculated direct cost
    double total_land_per_power_cost,	//[$/We] Of plant net capacity
    double total_land_fixed_cost,	    //[$]
    double sales_tax_basis,			    //[%] Of total direct cost
    double sales_tax_rate,			    //[%]

    // Calculated Outputs
    double& tes_cost,                               //[$]
    double& CT_tes_cost,                            //[$]
    double& power_cycle_cost,                       //[$]
    double& heater_cost,                            //[$]
    double& bop_cost,                               //[$]
    double& direct_capital_precontingency_cost,     //[$]
    double& contingency_cost,                       //[$]
    double& total_direct_cost,                      //[$]
    double& total_land_cost,                        //[$]
    double& epc_and_owner_cost,                     //[$]
    double& sales_tax_cost,                         //[$]
    double& total_indirect_cost,                    //[$]
    double& total_installed_cost,                   //[$]
    double& estimated_installed_cost_per_cap        //[$/kWe]
)
{
    // Cost model inputs not relevant to ETES - set to 0
    double A_sf_refl, site_improv_spec_cost, heliostat_spec_cost, heliostat_fixed_cost,
        h_tower, h_rec, h_helio, tower_fixed_cost, tower_cost_scaling_exp,
        A_rec, rec_ref_cost, A_rec_ref, rec_cost_scaling_exp,
        radfield_area, coldstorage_vol, radfield_vol, rad_unitcost, rad_installcost,
        rad_volmulti, rad_fluidcost, coldstorage_unitcost,
        fossil_backup_spec_cost, total_land_area,
        EPC_land_spec_cost, total_land_spec_cost;

    A_sf_refl = site_improv_spec_cost = heliostat_spec_cost = heliostat_fixed_cost =
        h_tower = h_rec = h_helio = tower_fixed_cost = tower_cost_scaling_exp =
        A_rec = rec_ref_cost = A_rec_ref = rec_cost_scaling_exp =
        radfield_area = coldstorage_vol = radfield_vol = rad_unitcost = rad_installcost =
        rad_volmulti = rad_fluidcost = coldstorage_unitcost =
        fossil_backup_spec_cost = total_land_area = 
        EPC_land_spec_cost = total_land_spec_cost = 0.0;

    // Cost model outputs not relevant to ETES - don't pass upstream
    double site_improvement_cost, heliostat_cost, tower_cost, receiver_cost,
        rad_field_totcost, rad_fluid_totcost, rad_storage_totcost, fossil_backup_cost;
    site_improvement_cost = heliostat_cost = tower_cost = receiver_cost =
        rad_field_totcost = rad_fluid_totcost = rad_storage_totcost = fossil_backup_cost = std::numeric_limits<double>::quiet_NaN();


    N_mspt::calculate_mspt_etes_costs(
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

        Q_CT_tes,
        CT_tes_spec_cost,

        W_dot_design,
        power_cycle_spec_cost,

        q_dot_heater_design,
        heater_spec_cost,

        radfield_area,
        coldstorage_vol,
        radfield_vol,
        rad_unitcost,
        rad_installcost,
        rad_volmulti,
        rad_fluidcost,
        coldstorage_unitcost,

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
        CT_tes_cost,
        power_cycle_cost,
        heater_cost,
        rad_field_totcost,
        rad_fluid_totcost,
        rad_storage_totcost,
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
}

void N_mspt::calculate_mslf_costs(

    // Inputs
    double site_improvement_area,           // csp.mslf.cost.site_improvements.area
    double site_improvement_cost_per_m2,    // csp.mslf.cost.site_improvements.cost_per_m2
    double sf_area,                         // csp.mslf.cost.solar_field.area
    double sf_cost_per_m2,                  // csp.mslf.cost.solar_field.cost_per_m2
    double htf_area,                        // csp.mslf.cost.htf_system.area
    double htf_cost_per_m2,                 // csp.mslf.cost.htf_system.cost_per_m2
    double ts_mwht,                         // csp.mslf.cost.ts_mwht
    double ts_per_kwht,                     // csp.mslf.cost.ts_per_kwht
    double fossil_mwe,                      // csp.mslf.cost.fossil_backup.mwe
    double fossil_cost_per_kwe,             // csp.mslf.cost.fossil_backup.cost_per_kwe
    double power_plant_mwe,                 // csp.mslf.cost.power_plant.mwe
    double power_plant_cost_per_kwe,        // csp.mslf.cost.power_plant.cost_per_kwe
    double bop_mwe,                         // csp.mslf.cost.bop_mwe
    double bop_per_kwe,                     // csp.mslf.cost.bop_per_kwe
    double contigency_percent,              // csp.mslf.cost.contingency_percent

    double total_land_area,                 // csp.mslf.cost.total_land_area
    double nameplate_MWe,                       // csp.mslf.cost.nameplate

    double epc_per_acre,                    // csp.mslf.cost.epc.per_acre
    double epc_percent,                     // csp.mslf.cost.epc.percent
    double epc_per_watt,                    // csp.mslf.cost.epc.per_watt
    double epc_fixed,                       // csp.mslf.cost.epc.fixed

    double plm_per_acre,                    // csp.mslf.cost.plm.per_acre
    double plm_percent,                     // csp.mslf.cost.plm.percent
    double plm_per_watt,                    // csp.mslf.cost.plm.per_watt
    double plm_fixed,                       // csp.mslf.cost.plm.fixed

    double sales_tax_value,                 // csp.mslf.cost.sales_tax.value
    double sales_tax_percent,                // csp.mslf.cost.sales_tax.percent

    // Outputs
    double& power_plant_cost_out,           // csp.mslf.cost.power_plant
    double& ts_out,                         // csp.mslf.cost.ts
    double& site_improvements_cost_out,     // csp.mslf.cost.site_improvements
    double& bop_out,                        // csp.mslf.cost.bop
    double& solar_field_cost_out,           // csp.mslf.cost.solar_field
    double& htf_system_cost_out,            // csp.mslf.cost.htf_system
    double& fossil_backup_cost_out,         // csp.mslf.cost.fossil_backup
    double& contingency_cost_out,           // csp.mslf.cost.contingency
    double& total_direct_cost_out,          // csp.mslf.cost.total_direct
    double& epc_total_cost_out,             // csp.mslf.cost.epc.total
    double& plm_total_cost_out,             // csp.mslf.cost.plm.total
    double& total_indirect_cost_out,        // csp.mslf.cost.total_indirect
    double& sales_tax_total_out,            // csp.mslf.cost.sales_tax.total
    double& total_installed_cost_out,       // csp.mslf.cost.total_installed
    double& installed_per_capacity_out      // csp.mslf.cost.installed_per_capacity



)
{
    double power_plant = power_cycle_cost(power_plant_mwe, power_plant_cost_per_kwe);

    double ts = tes_cost(ts_mwht, ts_per_kwht);

    double site_improvements = site_improvement_cost(site_improvement_area, site_improvement_cost_per_m2);

    double bop = bop_cost(bop_mwe, bop_per_kwe);

    double solar_field = sf_area * sf_cost_per_m2;

    double htf_system = htf_area * htf_cost_per_m2;

    double fossil_backup = fossil_backup_cost(fossil_mwe, fossil_cost_per_kwe);

    double direct_capital_precontingency_cost = site_improvements + solar_field + htf_system + fossil_backup
        + power_plant + bop + ts;

    double contingency = contingency_cost(contigency_percent, direct_capital_precontingency_cost);

    double total_direct = total_direct_cost(direct_capital_precontingency_cost, contingency);

    double epc_total = epc_and_owner_cost(total_land_area, total_direct, nameplate_MWe, epc_per_acre,
        epc_percent, epc_per_watt, epc_fixed);

    double plm_total = total_land_cost(total_land_area, total_direct, nameplate_MWe, plm_per_acre, plm_percent,
        plm_per_watt, plm_fixed);

    double total_indirect = epc_total + plm_total;

    double sales_tax_total = sales_tax_cost(total_direct, sales_tax_value, sales_tax_percent);

    double total_installed = total_direct + total_indirect + sales_tax_total;

    double installed_per_cap = estimated_installed_cost_per_cap(total_installed, nameplate_MWe);

    // Set Outputs
    {
        power_plant_cost_out = power_plant;
        ts_out = ts;
        site_improvements_cost_out = site_improvements;
        bop_out = bop;
        solar_field_cost_out = solar_field;
        htf_system_cost_out = htf_system;
        fossil_backup_cost_out = fossil_backup;
        contingency_cost_out = contingency;
        total_direct_cost_out = total_direct;
        epc_total_cost_out = epc_total;
        plm_total_cost_out = plm_total;
        total_indirect_cost_out = total_indirect;
        sales_tax_total_out = sales_tax_total;
        total_installed_cost_out = total_installed;
        installed_per_capacity_out = installed_per_cap;
    }
}


double N_mspt::site_improvement_cost(double A_refl /*m^2*/, double site_improv_spec_cost /*$/m^2_reflect*/)
{
	return A_refl*site_improv_spec_cost;		//[$]
}

double N_mspt::heliostat_cost(double A_refl /*m^2*/, double heliostat_spec_cost /*$/m^2*/, double heliostat_fixed_cost /*$*/)
{
	return A_refl*heliostat_spec_cost + heliostat_fixed_cost;	//[$]
}

double N_mspt::tower_cost(double h_tower /*m*/, double h_rec /*m*/, double h_helio /*m*/, double tower_fixed_cost /*$*/, double tower_cost_scaling_exp /*-*/)
{
	return tower_fixed_cost * exp(tower_cost_scaling_exp * (h_tower - h_rec / 2.0 + h_helio / 2.0));	//[$]
}

double N_mspt::receiver_cost(double A_rec /*m^2*/, double rec_ref_cost /*$*/, double rec_ref_area /*m^2*/, double rec_cost_scaling_exp /*-*/)
{
	return rec_ref_cost*pow(A_rec/rec_ref_area, rec_cost_scaling_exp);	//[$]
}

double N_mspt::lift_cost(double lift_mdot /*kg/s*/, double lift_height /*m*/, double lift_spec_cost /*$-s/m-kg*/, double n_lifts /*-*/)
{
    return lift_spec_cost * lift_mdot * lift_height * n_lifts;
}

double N_mspt::tes_cost(double Q_storage /*MWt-hr*/, double tes_spec_cost /*$/kWt-hr*/)
{
	return Q_storage*1.E3*tes_spec_cost;		//[$]
}

double N_mspt::tes_tank_cost(double tes_mass /*kg*/,double cost_per_mass /*$/kg*/, double tes_cost_exp /*-*/)
{
    return cost_per_mass * pow(tes_mass, tes_cost_exp);
}

double N_mspt::tes_medium_cost(double tes_mass /*kg*/, double tes_cost_per_mass /*$/kg*/, double nonactive_tes_mass_frac /*-*/)
{
    return (1.0 + nonactive_tes_mass_frac) * tes_cost_per_mass * tes_mass;
}

double N_mspt::power_cycle_cost(double W_dot_design /*MWe*/, double power_cycle_spec_cost /*$/kWe*/)
{
	return W_dot_design*1.E3*power_cycle_spec_cost;		//[$]
}

double N_mspt::heater_cost(double q_dot_heater_design /*MWt*/, double heater_spec_cost /*$/kWe*/)
{
    return q_dot_heater_design*1.E3*heater_spec_cost;   //[$]
}

double N_mspt::rad_field_totcost(double rad_area /*m^2*/, double panelcost /*$/m^2*/, double panelinstallcost /*$/m^2*/)
{
	return rad_area * (panelcost + panelinstallcost) ;		//[$]
}

double N_mspt::rad_fluid_totcost(double rad_vol /*m^3*/, double fluidcost /*$/L*/, double muliplier_volume /*-*/)
{
	return rad_vol * 1000 /*1000 L/1 m^3*/ * muliplier_volume /*ratio*/ *fluidcost /*$/L*/;		//[$]
}

double N_mspt::rad_storage_totcost(double cold_volume /*m^3*/, double storagecost /*$/L*/)
{
	return cold_volume * 1000 /*1000 L/1 m^3*/ * storagecost /*$/L*/;		//[$]
}

double N_mspt::bop_cost(double W_dot_design /*MWe*/, double bop_spec_cost /*$/kWe*/)
{
	return W_dot_design*1.E3*bop_spec_cost;				//[$]
}

double N_mspt::fossil_backup_cost(double W_dot_design /*MWe*/, double fossil_backup_spec_cost /*$/kWe*/)
{
	return W_dot_design*1.E3*fossil_backup_spec_cost;	//[$]
}

double N_mspt::direct_capital_precontingency_cost(double site_improvement_cost /*$*/,
	double heliostat_cost /*$*/,
	double tower_cost /*$*/,
	double receiver_cost /*$*/,
	double tes_cost /*$*/,
    double CT_tes_cost /*$*/,
	double power_cycle_cost /*$*/,
    double heater_cost /*$*/,
	double rad_field_totcost /*$*/,
	double rad_fluid_totcost /*$*/,
	double rad_storage_totcost /*$*/,
	double bop_cost /*$*/,
	double fossil_backup_cost /*$*/)
{
	return site_improvement_cost +
		heliostat_cost +
		tower_cost +
		receiver_cost +
		tes_cost +
        CT_tes_cost +
		power_cycle_cost +
        heater_cost +
		rad_field_totcost +
		rad_fluid_totcost +
		rad_storage_totcost +
		bop_cost +
		fossil_backup_cost;	//[$]
}

double N_mspt::contingency_cost(double contingency_rate /*%*/, double direct_capital_precontingency_cost /*$*/)
{
	return contingency_rate/100.0*direct_capital_precontingency_cost;	//[$]
}

double N_mspt::total_direct_cost(double direct_capital_precontingency_cost /*$*/, double contingency_cost /*$*/)
{
	return direct_capital_precontingency_cost + contingency_cost;		//[$]
}

double N_mspt::total_land_cost(double total_land_area /*acres*/, double total_direct_cost /*$*/, double plant_net_capacity /*MWe*/,
	double land_spec_cost /*$/acre*/, double land_perc_direct_cost /*%*/, double land_spec_per_power_cost /*$/We*/, double land_fixed_cost /*$*/)
{
	return total_land_area*land_spec_cost +
		total_direct_cost*land_perc_direct_cost/100.0 +
		plant_net_capacity*1.E6*land_spec_per_power_cost +
		land_fixed_cost;	//[$]
}

double N_mspt::epc_and_owner_cost(double total_land_area /*acres*/, double total_direct_cost /*$*/, double plant_net_capacity /*MWe*/,
	double land_spec_cost /*$/acre*/, double land_perc_direct_cost /*%*/, double land_spec_per_power_cost /*$/We*/, double land_fixed_cost /*$*/)
{
	return total_land_area*land_spec_cost +
		total_direct_cost*land_perc_direct_cost / 100.0 +
		plant_net_capacity*1.E6*land_spec_per_power_cost +
		land_fixed_cost;	//[$]
}

double N_mspt::sales_tax_cost(double total_direct_cost /*$*/, double sales_tax_basis /*% of tot. direct cost*/, double sales_tax_rate /*%*/)
{
	return total_direct_cost*(sales_tax_basis/100.0)*(sales_tax_rate/100.0);
}

double N_mspt::total_indirect_cost(double total_land_cost /*$*/, double epc_and_owner_cost /*$*/, double sales_tax_cost /*$*/)
{
	return total_land_cost + epc_and_owner_cost + sales_tax_cost;	//[$]
}

double N_mspt::total_installed_cost(double total_direct_cost /*$*/, double total_indirect_cost /*$*/)
{
	return total_direct_cost + total_indirect_cost;		//[$]
}

double N_mspt::estimated_installed_cost_per_cap(double total_installed_cost /*$*/, double plant_net_capacity /*MWe*/)
{
	return total_installed_cost/(plant_net_capacity*1.E3);
}

void N_financial_parameters::construction_financing_total_cost(double total_installed_cost /*$*/,
	double const_per_interest_rate1 /*%*/, double const_per_interest_rate2 /*%*/, double const_per_interest_rate3 /*%*/, double const_per_interest_rate4 /*%*/, double const_per_interest_rate5 /*%*/,
	double const_per_months1 /*-*/, double const_per_months2 /*-*/, double const_per_months3 /*-*/, double const_per_months4 /*-*/, double const_per_months5 /*-*/,
	double const_per_percent1 /*%*/, double const_per_percent2 /*%*/, double const_per_percent3 /*%*/, double const_per_percent4 /*%*/, double const_per_percent5 /*%*/,
	double const_per_upfront_rate1 /*%*/, double const_per_upfront_rate2 /*%*/, double const_per_upfront_rate3 /*%*/, double const_per_upfront_rate4 /*%*/, double const_per_upfront_rate5 /*%*/,
	double & const_per_principal1 /*$*/, double & const_per_principal2 /*$*/, double & const_per_principal3 /*$*/, double & const_per_principal4 /*$*/, double & const_per_principal5 /*$*/,
	double & const_per_interest1 /*$*/, double & const_per_interest2 /*$*/, double & const_per_interest3 /*$*/, double & const_per_interest4 /*$*/, double & const_per_interest5 /*$*/,
	double & const_per_total1 /*$*/, double & const_per_total2 /*$*/, double & const_per_total3 /*$*/, double & const_per_total4 /*$*/, double & const_per_total5 /*$*/,
	double & const_per_percent_total /*%*/, double & const_per_principal_total /*$*/, double & const_per_interest_total /*$*/, double & construction_financing_cost /*$*/)
{
	// Loan 1
	const_per_principal1 = const_per_percent1*total_installed_cost/100.0;
	construction_financing_loan_cost(const_per_principal1, const_per_interest_rate1, const_per_months1, const_per_upfront_rate1, 
		const_per_interest1, const_per_total1);

	const_per_principal2 = const_per_percent2*total_installed_cost/100.0;
	construction_financing_loan_cost(const_per_principal2, const_per_interest_rate2, const_per_months2, const_per_upfront_rate2,
		const_per_interest2, const_per_total2);

	const_per_principal3 = const_per_percent3*total_installed_cost/100.0;
	construction_financing_loan_cost(const_per_principal3, const_per_interest_rate3, const_per_months3, const_per_upfront_rate3,
		const_per_interest3, const_per_total3);

	const_per_principal4 = const_per_percent4*total_installed_cost/100.0;
	construction_financing_loan_cost(const_per_principal4, const_per_interest_rate4, const_per_months4, const_per_upfront_rate4,
		const_per_interest4, const_per_total4);

	const_per_principal5 = const_per_percent5*total_installed_cost/100.0;
	construction_financing_loan_cost(const_per_principal5, const_per_interest_rate5, const_per_months5, const_per_upfront_rate5,
		const_per_interest5, const_per_total5);

	const_per_percent_total = const_per_percent1 + const_per_percent2 + const_per_percent3 + const_per_percent4 + const_per_percent5;
	const_per_principal_total = const_per_principal1 + const_per_principal2 + const_per_principal3 + const_per_principal4 + const_per_principal5;
	const_per_interest_total = const_per_interest1 + const_per_interest2 + const_per_interest3 + const_per_interest4 + const_per_interest5;
	construction_financing_cost = const_per_total1 + const_per_total2 + const_per_total3 + const_per_total4 + const_per_total5;

}

void N_financial_parameters::construction_financing_loan_cost(double principal /*$*/, double interest_rate /*%*/, double term_months /*-*/, double upfront_rate /*%*/,
	double & interest /*$*/, double & total_cost /*$*/)
{
	double r = interest_rate/100.0;		//[-] annual loan rate

	interest = principal * r / 12.0 * term_months / 2.0;	//[$] loan interest

	double u = upfront_rate/100.0;		//[-] up-front rate

	total_cost = principal*u + interest;
}
