/*
BSD 3-Clause License

Copyright Alliance for Energy Innovation, LLC. See also https://github.com/NatLabRockies/ssc/blob/develop/LICENSE


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


#include "cmod_mhk_costs_test.h"
#include "core.h"

TEST_F(CM_MHKCost, ComputeModuleTest_cmod_mhk_costs) {
   
    int mhk_wave_errors = run_module(data, "mhk_costs");
	ASSERT_EQ(mhk_wave_errors, 0);
	
	ssc_number_t structural, pto, moor;

	ssc_data_get_number(data, "structural_assembly_cost", &structural);
	EXPECT_NEAR(structural, 244480984, 1.0); //$
	
	ssc_data_get_number(data, "power_takeoff_system_cost", &pto);
	EXPECT_NEAR(pto, 55906235, 1.0); //$
	
	ssc_data_get_number(data, "mooring_found_substruc_cost", &moor);
	EXPECT_NEAR(moor, 64289168, 1.0); //$

}

TEST_F(CM_MHKCost, ModeledValuesTest) {
    
    

    std::string devices[3] = { "RM1", "RM3", "RM5" };
    ssc_number_t device_rated_power[3] = { 1115, 286, 360 };
    ssc_number_t system_capacity[3] = { 111500, 28600, 36000 };
    ssc_number_t elec_infras_cost[3] = { 17322061, 7451369, 8594027 };
    ssc_number_t struc_cost[3] = { 49952032, 244480984, 307846813 };
    ssc_number_t pto_cost[3] = { 213727306, 55906235, 51433792 };
    ssc_number_t moor_cost[3] = { 50006748, 64289168, 97254913 };
    ssc_number_t dev_cost[3] = { 31909103, 15942304, 17927457 };
    ssc_number_t eng_cost[3] = { 14702409, 6862456, 7806277 };
    ssc_number_t assem_cost[3] = { 69982331, 28124155, 32812318 };
    ssc_number_t elec_cost[3] = { 17322061, 7451369, 8594027 };
    ssc_number_t site_cost[3] = { 4923622, 4653623, 5760432 };
    ssc_number_t plant_cost[3] = { 7161632, 6768907, 8378810 };
    ssc_number_t ins_cost[3] = { 5818826, 5499737, 6807783 };
    ssc_number_t cont_cost[3] = { 36255761, 34267590, 42417723 };
    ssc_number_t res_cost[3] = { 13875662, 13114757, 16233944 };
    ssc_number_t oper_cost[3] = { 6224661, 2398245, 2739807 };
    ssc_number_t maint_cost[3] = { 11118106, 4492241, 5083693 };
    ssc_number_t struc, pto, moor, dev, eng, assem, elec, site, plant, ins, cont, res, oper, maint;
    for (int i = 0; i < 3; i++) {
        ssc_data_set_string(data, "lib_wave_device", devices[i].c_str());
        if (devices[i] == "RM1")
            ssc_data_set_number(data, "marine_energy_tech", 1);
        else
            ssc_data_set_number(data, "marine_energy_tech", 0);
        ssc_data_set_number(data, "device_rated_power", device_rated_power[i]);
        ssc_data_set_number(data, "system_capacity", system_capacity[i]);
        ssc_data_set_number(data, "elec_infras_cost_modeled", elec_infras_cost[i]);
        int mhk_wave_errors = run_module(data, "mhk_costs");
        ASSERT_EQ(mhk_wave_errors, 0);
        ssc_data_get_number(data, "structural_assembly_cost", &struc);
        EXPECT_NEAR(struc, struc_cost[i], 1.0); //$
        ssc_data_get_number(data, "power_takeoff_system_cost", &pto);
        EXPECT_NEAR(pto, pto_cost[i], 1.0); //$
        ssc_data_get_number(data, "mooring_found_substruc_cost", &moor);
        EXPECT_NEAR(moor, moor_cost[i], 1.0); //$
        ssc_data_get_number(data, "development_cost", &dev);
        EXPECT_NEAR(dev, dev_cost[i], 1.0); //$
        ssc_data_get_number(data, "eng_and_mgmt_cost", &eng);
        EXPECT_NEAR(eng, eng_cost[i], 1.0); //$
        ssc_data_get_number(data, "assembly_and_install_cost", &assem);
        EXPECT_NEAR(assem, assem_cost[i], 1.0); //$
        ssc_data_get_number(data, "elec_infras_cost", &elec);
        EXPECT_NEAR(elec, elec_cost[i], 1.0); //$
        ssc_data_get_number(data, "site_access_port_staging_cost", &site);
        EXPECT_NEAR(site, site_cost[i], 1.0); //$
        ssc_data_get_number(data, "plant_commissioning_cost", &plant);
        EXPECT_NEAR(plant, plant_cost[i], 1.0); //$
        ssc_data_get_number(data, "insurance_during_construction_cost", &ins);
        EXPECT_NEAR(ins, ins_cost[i], 1.0); //$
        ssc_data_get_number(data, "project_contingency_cost", &cont);
        EXPECT_NEAR(cont, cont_cost[i], 1.0); //$
        ssc_data_get_number(data, "reserve_accounts_cost", &res);
        EXPECT_NEAR(res, res_cost[i], 1.0); //$
        ssc_data_get_number(data, "operations_cost", &oper);
        EXPECT_NEAR(oper, oper_cost[i], 1000.0); //$
        ssc_data_get_number(data, "maintenance_cost", &maint);
        EXPECT_NEAR(maint, maint_cost[i], 1000.0); //$

    }
}

TEST_F(CM_MHKCost, CostModelOptionsTest) {
    ssc_data_set_number(data, "structural_assembly_cost_input", 100); //$ or $/kW
    ssc_data_set_number(data, "structural_assembly_cost_total", 1000000);
    ssc_data_set_number(data, "structural_assembly_cost_rvalue", 0.1);

    ssc_number_t struc_cost[5] = { 2860000, 100, 244480984, 1000000, 15.20 };
    
    ssc_number_t struc, pto, moor, dev, eng, assem, elec, site, plant, ins, cont, res, oper, maint;
    for (int i = 0; i < 5; i++) {
        ssc_data_set_number(data, "structural_assembly_cost_method", i);
        ssc_data_set_number(data, "power_takeoff_system_cost_method", i);
        ssc_data_set_number(data, "mooring_found_substruct_cost_method", i);
        ssc_data_set_number(data, "development_cost_method", i);
        ssc_data_set_number(data, "eng_and_mgmt_cost_method", i);
        ssc_data_set_number(data, "assembly_and_install_cost_method", i);
        ssc_data_set_number(data, "other_infrastructure_cost_method", i);
        ssc_data_set_number(data, "elec_infras_cost_method", i);
        ssc_data_set_number(data, "plant_commissioning_cost_method", i);
        ssc_data_set_number(data, "site_access_port_staging_cost_method", i);
        ssc_data_set_number(data, "project_conteingency_cost_method", i);
        ssc_data_set_number(data, "insurance_during_construction_cost_method", i);
        ssc_data_set_number(data, "reserve_accounts_cost_method", i);
        ssc_data_set_number(data, "other_financial_cost_method", i);
        ssc_data_set_number(data, "operations_cost_method", i);
        ssc_data_set_number(data, "maintenance_cost_method", i);
        int mhk_wave_errors = run_module(data, "mhk_costs");
        ASSERT_EQ(mhk_wave_errors, 0);
        ssc_data_get_number(data, "structural_assembly_cost", &struc);
        EXPECT_NEAR(struc, struc_cost[i], 1.0); //$
        

    }
}





