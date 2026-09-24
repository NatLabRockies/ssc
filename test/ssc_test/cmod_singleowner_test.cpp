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


#include "cmod_singleowner_test.h"

#include "gtest/gtest.h"

TEST_F(CMSingleOwner, ResidentialDefault_cmod_swh) {

    int errors = run_module(data, "singleowner");
    ASSERT_EQ(errors, 0);

    ssc_number_t npv;
    ssc_data_get_number(data, "project_return_aftertax_npv", &npv);
    EXPECT_NEAR(npv, -647715977.8, 0.1);

}


TEST_F(CmodSingleOwnerTest, ssc_1047) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/singleowner/Default_SO_10_IBI_PVWatts_Single_Owner_cmod_singleowner.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/singleowner/Default_SO_10_IBI_PVWatts_Single_Owner_cmod_singleowner_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "project_return_aftertax_npv", "lcoe_real", "lppa_nom" };
    std::vector<std::string> compare_array_variables = { "cf_project_return_aftertax", "cf_annual_costs" };

    Test("singleowner", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodSingleOwnerTest, ETES) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/singleowner/2022.08.08_develop_branch_ETES_Single_Owner_cmod_singleowner.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/singleowner/2022.08.08_develop_branch_ETES_Single_Owner_cmod_singleowner_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "project_return_aftertax_npv", "lcoe_real", "lppa_nom" };
    std::vector<std::string> compare_array_variables = { "cf_project_return_aftertax", "cf_annual_costs" };

    Test("singleowner", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodSingleOwnerTest, PV) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/singleowner/2022.08.08_develop_branch_Flat_Plate_PV_Single_Owner_cmod_singleowner.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/singleowner/2022.08.08_develop_branch_Flat_Plate_PV_Single_Owner_cmod_singleowner_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "project_return_aftertax_npv", "lcoe_real", "lppa_nom" };
    std::vector<std::string> compare_array_variables = { "cf_project_return_aftertax", "cf_annual_costs" };

    Test("singleowner", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodSingleOwnerTest, FuelCell) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/singleowner/2022.08.08_develop_branch_Fuel_Cell_Single_Owner_cmod_singleowner.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/singleowner/2022.08.08_develop_branch_Fuel_Cell_Single_Owner_cmod_singleowner_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "project_return_aftertax_npv", "lcoe_real", "lppa_nom" };
    std::vector<std::string> compare_array_variables = { "cf_project_return_aftertax", "cf_annual_costs" };

    Test("singleowner", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}
TEST_F(CmodSingleOwnerTest, GenericCSP) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/singleowner/2022.08.08_develop_branch_Generic_CSP_System_Single_Owner_cmod_singleowner.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/singleowner/2022.08.08_develop_branch_Generic_CSP_System_Single_Owner_cmod_singleowner_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "project_return_aftertax_npv", "lcoe_real", "lppa_nom" };
    std::vector<std::string> compare_array_variables = { "cf_project_return_aftertax", "cf_annual_costs" };

    Test("singleowner", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodSingleOwnerTest, Geotherrmal) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/singleowner/2022.08.08_develop_branch_Geothermal_Power_Single_Owner_cmod_singleowner.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/singleowner/2022.08.08_develop_branch_Geothermal_Power_Single_Owner_cmod_singleowner_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "project_return_aftertax_npv", "lcoe_real", "lppa_nom" };
    std::vector<std::string> compare_array_variables = { "cf_project_return_aftertax", "cf_annual_costs" };

    Test("singleowner", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}
TEST_F(CmodSingleOwnerTest, PVBattery) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/singleowner/2022.08.08_develop_branch_PV_Battery_Single_Owner_cmod_singleowner.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/singleowner/2022.08.08_develop_branch_PV_Battery_Single_Owner_cmod_singleowner_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "project_return_aftertax_npv", "lcoe_real", "lppa_nom" };
    std::vector<std::string> compare_array_variables = { "cf_project_return_aftertax", "cf_annual_costs" };

    Test("singleowner", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}


TEST_F(CmodSingleOwnerTest, PVWatts) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/singleowner/2022.08.08_develop_branch_PVWatts_Single_Owner_cmod_singleowner.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/singleowner/2022.08.08_develop_branch_PVWatts_Single_Owner_cmod_singleowner_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "project_return_aftertax_npv", "lcoe_real", "lppa_nom" };
    std::vector<std::string> compare_array_variables = { "cf_project_return_aftertax", "cf_annual_costs" };

    Test("singleowner", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodSingleOwnerTest, IrrTarget) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/irr_target/so_irr_dscr_targets_PVWatts_Single_Owner_cmod_singleowner.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/irr_target/so_irr_dscr_targets_PVWatts_Single_Owner_cmod_singleowner_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "project_return_aftertax_npv", "lcoe_real", "lppa_nom", "min_dscr", "project_return_aftertax_irr" };
    std::vector<std::string> compare_array_variables = { "cf_project_return_aftertax", "cf_annual_costs", "cf_pretax_dscr", "cf_debt_balance" };

    Test("singleowner", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodSingleOwnerTest, FixedDebtPPAInput) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/singleowner/so-fixed-debt-fixed-price_PVWatts_Single_Owner_cmod_singleowner.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/singleowner/so-fixed-debt-fixed-price_PVWatts_Single_Owner_cmod_singleowner_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "project_return_aftertax_npv", "lcoe_real", "lppa_nom", "min_dscr", "project_return_aftertax_irr" };
    std::vector<std::string> compare_array_variables = { "cf_project_return_aftertax", "cf_annual_costs", "cf_pretax_dscr", "cf_debt_balance" };

    Test("singleowner", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodSingleOwnerTest, FixedDebtIrrTarget) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/singleowner/so-fixed-debt-irr-target_PVWatts_Single_Owner_cmod_singleowner.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/singleowner/so-fixed-debt-irr-target_PVWatts_Single_Owner_cmod_singleowner_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "project_return_aftertax_npv", "lcoe_real", "lppa_nom", "min_dscr", "project_return_aftertax_irr" };
    std::vector<std::string> compare_array_variables = { "cf_project_return_aftertax", "cf_annual_costs", "cf_pretax_dscr", "cf_debt_balance" };

    Test("singleowner", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodSingleOwnerTest, BonusDepreciation) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/singleowner/so_bonus_depr_PVWatts_Single_Owner_cmod_singleowner.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/singleowner/so_bonus_depr_PVWatts_Single_Owner_cmod_singleowner_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "project_return_aftertax_npv", "lcoe_real", "lppa_nom", "min_dscr", "project_return_aftertax_irr", "pre_depr_alloc_basis", "depr_stabas_first_year_bonus_macrs_5", "depr_fedbas_first_year_bonus_macrs_5"};
    std::vector<std::string> compare_array_variables = { "cf_project_return_aftertax", "cf_annual_costs", "cf_pretax_dscr", "cf_debt_balance" };

    Test("singleowner", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodSingleOwnerTest, PBIforDebtService) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/singleowner/so_pbi_dscr_targets_PVWatts_Single_Owner_cmod_singleowner.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/singleowner/so_pbi_dscr_targets_PVWatts_Single_Owner_cmod_singleowner_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "project_return_aftertax_npv", "lcoe_real", "lppa_nom", "min_dscr", "project_return_aftertax_irr" };
    std::vector<std::string> compare_array_variables = { "cf_project_return_aftertax", "cf_annual_costs", "cf_pretax_dscr", "cf_debt_balance" };

    Test("singleowner", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodSingleOwnerTest, CustomDepr) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/singleowner/so-custom_dep_PVWatts_Single_Owner_cmod_singleowner.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/singleowner/so-custom_dep_PVWatts_Single_Owner_cmod_singleowner_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "project_return_aftertax_npv", "lcoe_real", "lppa_nom", "min_dscr", "project_return_aftertax_irr" };
    std::vector<std::string> compare_array_variables = { "cf_project_return_aftertax", "cf_annual_costs", "cf_pretax_dscr", "cf_debt_balance" };

    Test("singleowner", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodSingleOwnerTest, Moratorium) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/singleowner/so-moratorium_PVWatts_Single_Owner_cmod_singleowner.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/singleowner/so-moratorium_PVWatts_Single_Owner_cmod_singleowner_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "project_return_aftertax_npv", "lcoe_real", "lppa_nom", "min_dscr", "project_return_aftertax_irr" };
    std::vector<std::string> compare_array_variables = { "cf_project_return_aftertax", "cf_annual_costs", "cf_pretax_dscr", "cf_debt_balance" };

    Test("singleowner", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodSingleOwnerTest, RecievablesReserve) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/singleowner/so-receivables_PVWatts_Single_Owner_cmod_singleowner.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/singleowner/so-receivables_PVWatts_Single_Owner_cmod_singleowner_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "project_return_aftertax_npv", "lcoe_real", "lppa_nom", "min_dscr", "project_return_aftertax_irr" };
    std::vector<std::string> compare_array_variables = { "cf_project_return_aftertax", "cf_annual_costs", "cf_pretax_dscr", "cf_debt_balance" };

    Test("singleowner", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodSingleOwnerTest, EquipReserves) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/singleowner/so-major-equip-depr_PVWatts_Single_Owner_cmod_singleowner.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/singleowner/so-major-equip-depr_PVWatts_Single_Owner_cmod_singleowner_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "project_return_aftertax_npv", "lcoe_real", "lppa_nom", "min_dscr", "project_return_aftertax_irr" };
    std::vector<std::string> compare_array_variables = { "cf_project_return_aftertax", "cf_annual_costs", "cf_pretax_dscr", "cf_debt_balance" };

    Test("singleowner", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodSingleOwnerTest, IBICBI) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/singleowner/so-cbi-ibi_PVWatts_Single_Owner_cmod_singleowner.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/singleowner/so-cbi-ibi_PVWatts_Single_Owner_cmod_singleowner_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "project_return_aftertax_npv", "lcoe_real", "lppa_nom", "min_dscr", "project_return_aftertax_irr" };
    std::vector<std::string> compare_array_variables = { "cf_project_return_aftertax", "cf_annual_costs", "cf_pretax_dscr", "cf_debt_balance" };

    Test("singleowner", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodSingleOwnerTest, NonEnergyRevenueFixedDebt) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/singleowner/so-fixed-debt-fixed-price_PVWatts_Single_Owner_cmod_singleowner.json";

    std::ifstream file(file_inputs);
    std::ostringstream tmp;
    tmp << file.rdbuf();
    file.close();
    ssc_data_t dat_inputs = json_to_ssc_data(tmp.str().c_str());

    tmp.str("");
    int errors = run_module(dat_inputs, "singleowner");

    EXPECT_FALSE(errors);

    std::vector<double> default_revenue = { 0, 8478885.956083823, 8521031.0160498116, 8563109.5073487349, 8605113.4751514699, 8647064.8985940199, 8688959.7209229749, 8730788.5669338983, 8772523.4115527496, 8814185.7912173141, 8855769.8138282895, 8897248.7138918284, 8938630.7883684468, 8979896.6277180221, 9021075.6455845423, 9062180.3135051988, 9103205.0441374388, 9144144.1064228881, 9184991.6294496525, 9225741.688526297, 9266388.2748306263, 9306925.3121825177, 9347346.5308990683, 9387645.4814586416, 9427815.7014087141, 9467850.651133962 };
    std::vector<double> default_costs = { 0, 1900000, 1947499.9999999998, 1996187.5, 2046092.1874999993, 2097244.4921874991, 2149675.6044921861, 2203417.4946044912, 2258502.9319696035, 2314965.5052688429, 2372839.6429005638, 2432160.6339730779, 2492964.6498224046, 2555288.7660679645, 2619170.985219663, 2684650.2598501546, 2751766.5163464085, 2820560.6792550683, 2891074.6962364446, 2963351.5636423551, 3037435.3527334142, 3113371.2365517491, 3191205.517465543, 3270985.6554021812, 3352760.2967872354, 3436579.3042069157 };
    std::vector<double> default_ebitda = { 0, 6578885.956083823, 6573531.0160498116, 6566922.0073487349, 6559021.2876514709, 6549820.4064065209, 6539284.1164307892, 6527371.0723294076, 6514020.4795831461, 6499220.2859484712, 6482930.1709277257, 6465088.0799187506, 6445666.1385460421, 6424607.8616500571, 6401904.6603648793, 6377530.0536550442, 6351438.5277910307, 6323583.4271678198, 6293916.9332132079, 6262390.1248839423, 6228952.9220972117, 6193554.0756307691, 6156141.0134335253, 6116659.8260564599, 6075055.4046214782, 6031271.3469270468 };

    // Confirm arrays match default run with 0 non-energy revenue.
    int arr_length;
    auto pRevArray = ssc_data_get_array(dat_inputs, "cf_total_revenue", &arr_length);
    for (size_t i = 0; i < default_revenue.size(); i++) {
        EXPECT_NEAR(pRevArray[i], default_revenue[i], 0.1) << " revenue issue at index i=" << i;
    }

    auto pCostArray = ssc_data_get_array(dat_inputs, "cf_operating_expenses", &arr_length);
    for(size_t i = 0; i < default_costs.size(); i++) {
        EXPECT_NEAR(pCostArray[i], default_costs[i], 0.1) << " cost issue at index i=" << i;
    }

    auto pEbitdaArray = ssc_data_get_array(dat_inputs, "cf_ebitda", &arr_length);
    for(size_t i = 0; i < default_ebitda.size(); i++) {
        EXPECT_NEAR(pEbitdaArray[i], default_ebitda[i], 0.1) << " ebitda issue at index i=" << i;
    }

    // Test 50% revenue retained
    std::vector<double> post_share_revenue;
    std::vector<double> post_share_costs;
    std::vector<double> post_share_ebitda;

    for (size_t i = 0; i < default_revenue.size(); i++) {
        post_share_revenue.push_back(pRevArray[i] * 0.5);
        post_share_costs.push_back(pCostArray[i] * 0.5);
        post_share_ebitda.push_back(post_share_revenue[i] - post_share_costs[i]);
    }

    std::vector<double> shared_energy_revenue = { 50 };
    std::vector<double> shared_energy_expenses = { 50 };

    ssc_data_set_array(dat_inputs, "energy_revenue_ret", shared_energy_revenue.data(), (int)shared_energy_revenue.size());
    ssc_data_set_array(dat_inputs, "energy_expenses_ret", shared_energy_expenses.data(), (int)shared_energy_expenses.size());

    errors = run_module(dat_inputs, "singleowner");
    EXPECT_FALSE(errors);

    // Expect 50% reductions in values
    pRevArray = ssc_data_get_array(dat_inputs, "cf_total_revenue", &arr_length);
    for (size_t i = 0; i < post_share_revenue.size(); i++) {
        EXPECT_NEAR(pRevArray[i], post_share_revenue[i], 0.1) << " revenue issue at index i=" << i;
    }

    pCostArray = ssc_data_get_array(dat_inputs, "cf_operating_expenses", &arr_length);
    for (size_t i = 0; i < post_share_costs.size(); i++) {
        EXPECT_NEAR(pCostArray[i], post_share_costs[i], 0.1) << " cost issue at index i=" << i;
    }

    pEbitdaArray = ssc_data_get_array(dat_inputs, "cf_ebitda", &arr_length);
    for (size_t i = 0; i < post_share_ebitda.size(); i++) {
        EXPECT_NEAR(pEbitdaArray[i], post_share_ebitda[i], 0.1) << " ebitda issue at index i=" << i;
    }


    // Test 50% revenue retained but with non-energy revenue added
    post_share_revenue.clear();
    post_share_costs.clear();
    post_share_ebitda.clear();

    post_share_revenue.push_back(0.0);
    post_share_costs.push_back(0.0);
    post_share_ebitda.push_back(0.0);
    for (size_t i = 1; i < default_revenue.size(); i++) {
        post_share_revenue.push_back(default_revenue[i] * 0.5 + 100000);
        post_share_costs.push_back(default_costs[i] * 0.5 + 50000);
        post_share_ebitda.push_back(post_share_revenue[i] - post_share_costs[i]);
    }

    std::vector<double> non_energy_revenue(25, 100000);
    std::vector<double> non_energy_expenses(25, 50000);
    std::vector<double> non_energy_revenue_ret = { 100 };
    std::vector<double> non_energy_expenses_ret = { 100 };

    ssc_data_set_array(dat_inputs, "non_energy_revenue", non_energy_revenue.data(), (int)non_energy_revenue.size());
    ssc_data_set_array(dat_inputs, "non_energy_expenses", non_energy_expenses.data(), (int)non_energy_expenses.size());
    ssc_data_set_array(dat_inputs, "non_energy_revenue_ret", non_energy_revenue_ret.data(), (int)non_energy_revenue_ret.size());
    ssc_data_set_array(dat_inputs, "non_energy_expenses_ret", non_energy_expenses_ret.data(), (int)non_energy_expenses_ret.size());

    ssc_data_set_number(dat_inputs, "non_energy_revenue_escal", -2.5);
    ssc_data_set_number(dat_inputs, "non_energy_expenses_escal", -2.5);

    errors = run_module(dat_inputs, "singleowner");
    EXPECT_FALSE(errors);

    // Expect 50% reductions in values
    pRevArray = ssc_data_get_array(dat_inputs, "cf_total_revenue", &arr_length);
    for (size_t i = 0; i < post_share_revenue.size(); i++) {
        EXPECT_NEAR(pRevArray[i], post_share_revenue[i], 0.1) << " revenue issue at index i=" << i;
    }

    pCostArray = ssc_data_get_array(dat_inputs, "cf_operating_expenses", &arr_length);
    for (size_t i = 0; i < post_share_costs.size(); i++) {
        EXPECT_NEAR(pCostArray[i], post_share_costs[i], 0.1) << " cost issue at index i=" << i;
    }

    pEbitdaArray = ssc_data_get_array(dat_inputs, "cf_ebitda", &arr_length);
    for (size_t i = 0; i < post_share_ebitda.size(); i++) {
        EXPECT_NEAR(pEbitdaArray[i], post_share_ebitda[i], 0.1) << " ebitda issue at index i=" << i;
    }

}

TEST_F(CmodSingleOwnerTest, NonEnergyRevenueDSCR) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/singleowner/2022.08.08_develop_branch_PVWatts_Single_Owner_cmod_singleowner.json";

    std::ifstream file(file_inputs);
    std::ostringstream tmp;
    tmp << file.rdbuf();
    file.close();
    ssc_data_t dat_inputs = json_to_ssc_data(tmp.str().c_str());

    tmp.str("");
    int errors = run_module(dat_inputs, "singleowner");

    EXPECT_FALSE(errors);

    std::vector<double> default_revenue = { 0, 9112020.5977755021, 9157125.0997344907, 9202452.8689781744, 9248005.0106796212, 9293782.6354824826, 9339786.8595281206, 9386018.804482786, 9432479.5975649767, 9479170.3715729229, 9526092.2649122048, 9573246.4216235224, 9620633.9914105572, 9668256.1296680402, 9716113.9975098986, 9764208.7617975734, 9812541.5951684713, 9861113.6760645546, 9909926.1887610722, 9958980.3233954422, 10008277.275996249, 10057818.24851243, 10107604.448842568, 10157637.090864338, 10207917.394464117, 10258446.585566713 };
    std::vector<double> default_costs = { 0, 1500000, 1537499.9999999998, 1575937.4999999998, 1615335.9374999993, 1655719.3359374991, 1697112.3193359368, 1739540.127319335, 1783028.6305023183, 1827604.3462648757, 1873294.4549214977, 1920126.8162945351, 1968129.9867018983, 2017333.2363694457, 2067766.5672786813, 2119460.7314606486, 2172447.2497471645, 2226758.4309908431, 2282427.3917656145, 2339488.0765597541, 2397975.2784737479, 2457924.6604355914, 2519372.7769464813, 2582357.0963701429, 2646916.0237793964, 2713088.9243738805 };
    std::vector<double> default_ebitda = { 0, 7612020.5977755021, 7619625.0997344907, 7626515.3689781744, 7632669.0731796222, 7638063.2995449835, 7642674.540192184, 7646478.677163451, 7649450.9670626586, 7651566.0253080474, 7652797.8099907069, 7653119.6053289874, 7652504.0047086589, 7650922.8932985943, 7648347.4302312173, 7644748.0303369248, 7640094.3454213068, 7634355.2450737115, 7627498.7969954573, 7619492.2468356881, 7610301.9975225013, 7599893.5880768392, 7588231.671896087, 7575279.9944941951, 7561001.3706847206, 7545357.6611928325 };

    // Confirm arrays match default run with 0 non-energy revenue.
    int arr_length;
    auto pRevArray = ssc_data_get_array(dat_inputs, "cf_total_revenue", &arr_length);
    for (size_t i = 0; i < default_revenue.size(); i++) {
        EXPECT_NEAR(pRevArray[i], default_revenue[i], 0.1) << " revenue issue at index i=" << i;
    }

    auto pCostArray = ssc_data_get_array(dat_inputs, "cf_operating_expenses", &arr_length);
    for (size_t i = 0; i < default_costs.size(); i++) {
        EXPECT_NEAR(pCostArray[i], default_costs[i], 0.1) << " cost issue at index i=" << i;
    }

    auto pEbitdaArray = ssc_data_get_array(dat_inputs, "cf_ebitda", &arr_length);
    for (size_t i = 0; i < default_ebitda.size(); i++) {
        EXPECT_NEAR(pEbitdaArray[i], default_ebitda[i], 0.1) << " ebitda issue at index i=" << i;
    }

    // Get default debt level
    ssc_number_t debt_frac;
    ssc_data_get_number(dat_inputs, "debt_fraction", &debt_frac);
    EXPECT_NEAR(42.26, debt_frac, 0.1);

    std::vector<double> post_share_revenue;
    std::vector<double> post_share_costs;
    std::vector<double> post_share_ebitda;

    std::vector<double> non_energy_revenue(25, 100000);
    std::vector<double> non_energy_expenses(25, 50000);
    std::vector<double> non_energy_revenue_ret = { 100 };
    std::vector<double> non_energy_expenses_ret = { 100 };

    post_share_revenue.push_back(0.0);
    post_share_costs.push_back(0.0);
    post_share_ebitda.push_back(0.0);
    for (size_t i = 1; i < default_revenue.size(); i++) {
        post_share_revenue.push_back(default_revenue[i] + 100000);
        post_share_costs.push_back(default_costs[i] + 50000);
        post_share_ebitda.push_back(post_share_revenue[i] - post_share_costs[i]);
    }

    ssc_data_set_array(dat_inputs, "non_energy_revenue", non_energy_revenue.data(), (int)non_energy_revenue.size());
    ssc_data_set_array(dat_inputs, "non_energy_expenses", non_energy_expenses.data(), (int)non_energy_expenses.size());
    ssc_data_set_array(dat_inputs, "non_energy_revenue_ret", non_energy_revenue_ret.data(), (int)non_energy_revenue_ret.size());
    ssc_data_set_array(dat_inputs, "non_energy_expenses_ret", non_energy_expenses_ret.data(), (int)non_energy_expenses_ret.size());

    ssc_data_set_number(dat_inputs, "non_energy_revenue_escal", -2.5);
    ssc_data_set_number(dat_inputs, "non_energy_expenses_escal", -2.5);
    ssc_data_set_number(dat_inputs, "non_energy_revenue_ds", 1);
    ssc_data_set_number(dat_inputs, "non_energy_expenses_ds", 1);

    errors = run_module(dat_inputs, "singleowner");
    EXPECT_FALSE(errors);

    // Expect 50% reductions in values
    pRevArray = ssc_data_get_array(dat_inputs, "cf_total_revenue", &arr_length);
    for (size_t i = 0; i < post_share_revenue.size(); i++) {
        EXPECT_NEAR(pRevArray[i], post_share_revenue[i], 0.1) << " revenue issue at index i=" << i;
    }

    pCostArray = ssc_data_get_array(dat_inputs, "cf_operating_expenses", &arr_length);
    for (size_t i = 0; i < post_share_costs.size(); i++) {
        EXPECT_NEAR(pCostArray[i], post_share_costs[i], 0.1) << " cost issue at index i=" << i;
    }

    pEbitdaArray = ssc_data_get_array(dat_inputs, "cf_ebitda", &arr_length);
    for (size_t i = 0; i < post_share_ebitda.size(); i++) {
        EXPECT_NEAR(pEbitdaArray[i], post_share_ebitda[i], 0.1) << " ebitda issue at index i=" << i;
    }

    ssc_data_get_number(dat_inputs, "debt_fraction", &debt_frac);
    EXPECT_NEAR(42.56, debt_frac, 0.1); // Slight increase due to increased revenue

    ssc_data_set_number(dat_inputs, "non_energy_revenue_ds", 0);
    ssc_data_set_number(dat_inputs, "non_energy_expenses_ds", 0);

    errors = run_module(dat_inputs, "singleowner");
    EXPECT_FALSE(errors);

    ssc_data_get_number(dat_inputs, "debt_fraction", &debt_frac);
    EXPECT_NEAR(42.26, debt_frac, 0.1);
}
