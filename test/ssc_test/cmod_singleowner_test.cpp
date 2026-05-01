/*
BSD 3-Clause License

Copyright Alliance for Energy Innovation, LLC. See also https://github.com/NREL/ssc/blob/develop/LICENSE


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
}
