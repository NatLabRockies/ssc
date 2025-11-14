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

#ifndef MHK_COST_INPUTS_H_
#define MHK_COST_INPUTS_H_

#include <stdio.h>
#include "../test/input_cases/code_generator_utilities.h"

void mhk_cost_inputs(ssc_data_t &data) {
	
    ssc_data_set_number(data, "device_rated_power", 286);
    ssc_data_set_number(data, "system_capacity", 28600);
    ssc_data_set_number(data, "devices_per_row", 10);
    ssc_data_set_number(data, "marine_energy_tech", 0);
    ssc_data_set_number(data, "library_or_input_wec", 0);
    ssc_data_set_string(data, "lib_wave_device", "RM3");
    ssc_data_set_string(data, "lib_tidal_device", "RM1");
    ssc_data_set_number(data, "structural_assembly_cost_method", 2);
    ssc_data_set_number(data, "structural_assembly_cost_input", 100); //$ or $/kW
    ssc_data_set_number(data, "structural_assembly_cost_total", 1000000);
    ssc_data_set_number(data, "structural_assembly_cost_rvalue", 0.1);
    ssc_data_set_number(data, "power_takeoff_system_cost_method", 2);
    ssc_data_set_number(data, "power_takeoff_system_cost_input", 100); //$ or $/kW
    ssc_data_set_number(data, "power_takeoff_system_cost_total", 1000000);
    ssc_data_set_number(data, "power_takeoff_system_cost_rvalue", 0.1);
    ssc_data_set_number(data, "mooring_found_substruc_cost_method", 2);
    ssc_data_set_number(data, "mooring_found_substruc_cost_input", 100); //$ or $/kW
    ssc_data_set_number(data, "mooring_found_substruc_cost_total", 1000000);
    ssc_data_set_number(data, "mooring_found_substruc_cost_rvalue", 0.1);
    ssc_data_set_number(data, "development_cost_method", 2);
    ssc_data_set_number(data, "development_cost_input", 100); //$ or $/kW
    ssc_data_set_number(data, "development_cost_total", 1000000);
    ssc_data_set_number(data, "development_cost_rvalue", 0.1);
    ssc_data_set_number(data, "eng_and_mgmt_cost_method", 2);
    ssc_data_set_number(data, "eng_and_mgmt_cost_input", 100); //$ or $/kW
    ssc_data_set_number(data, "eng_and_mgmt_cost_total", 1000000);
    ssc_data_set_number(data, "eng_and_mgmt_cost_rvalue", 0.1);
    ssc_data_set_number(data, "assembly_and_install_cost_method", 2);
    ssc_data_set_number(data, "assembly_and_install_cost_input", 100); //$ or $/kW
    ssc_data_set_number(data, "assembly_and_install_cost_total", 1000000);
    ssc_data_set_number(data, "assembly_and_install_cost_rvalue", 0.1);
    ssc_data_set_number(data, "other_infrastructure_cost_method", 2);
    ssc_data_set_number(data, "other_infrastructure_cost_input", 100); //$ or $/kW
    ssc_data_set_number(data, "other_infrastructure_cost_total", 1000000);
    ssc_data_set_number(data, "other_infrastructure_cost_rvalue", 0.1);
    ssc_data_set_number(data, "elec_infras_cost_method", 2);
    ssc_data_set_number(data, "elec_infras_cost_modeled", 0);
    ssc_data_set_number(data, "elec_infras_cost_input", 100); //$ or $/kW
    ssc_data_set_number(data, "elec_infras_cost_total", 1000000);
    ssc_data_set_number(data, "elec_infras_cost_rvalue", 0.1);
    ssc_data_set_number(data, "plant_commissioning_cost_method", 2);
    ssc_data_set_number(data, "plant_commissioning_cost_input", 100); //$ or $/kW
    ssc_data_set_number(data, "plant_commissioning_cost_total", 1000000);
    ssc_data_set_number(data, "plant_commissioning_cost_rvalue", 0.1);
    ssc_data_set_number(data, "site_access_port_staging_cost_method", 2);
    ssc_data_set_number(data, "site_access_port_staging_cost_input", 100); //$ or $/kW
    ssc_data_set_number(data, "site_access_port_staging_cost_total", 1000000);
    ssc_data_set_number(data, "site_access_port_staging_cost_rvalue", 0.1);
    ssc_data_set_number(data, "project_contingency_cost_method", 2);
    ssc_data_set_number(data, "project_contingency_cost_input", 100); //$ or $/kW
    ssc_data_set_number(data, "project_contingency_cost_total", 1000000);
    ssc_data_set_number(data, "project_contingency_cost_rvalue", 0.1);
    ssc_data_set_number(data, "insurance_during_construction_cost_method", 2);
    ssc_data_set_number(data, "insurance_during_construction_cost_input", 100); //$ or $/kW
    ssc_data_set_number(data, "insurance_during_construction_cost_total", 1000000);
    ssc_data_set_number(data, "insurance_during_construction_cost_rvalue", 0.1);
    ssc_data_set_number(data, "reserve_accounts_cost_method", 2);
    ssc_data_set_number(data, "reserve_accounts_cost_input", 100); //$ or $/kW
    ssc_data_set_number(data, "reserve_accounts_cost_total", 1000000);
    ssc_data_set_number(data, "reserve_accounts_cost_rvalue", 0.1);
    ssc_data_set_number(data, "other_financial_cost_method", 2);
    ssc_data_set_number(data, "other_financial_cost_input", 100); //$ or $/kW
    ssc_data_set_number(data, "other_financial_cost_total", 1000000);
    ssc_data_set_number(data, "other_financial_cost_rvalue", 0.1);
    ssc_data_set_number(data, "operations_cost_method", 2);
    ssc_data_set_number(data, "operations_cost_input", 100); //$ or $/kW
    ssc_data_set_number(data, "operations_cost_total", 1000000);
    ssc_data_set_number(data, "operations_cost_rvalue", 0.1);
    ssc_data_set_number(data, "maintenance_cost_method", 2);
    ssc_data_set_number(data, "maintenance_cost_input", 100); //$ or $/kW
    ssc_data_set_number(data, "maintenance_cost_total", 1000000);
    ssc_data_set_number(data, "maintenance_cost_rvalue", 0.1);
	

}

#endif
