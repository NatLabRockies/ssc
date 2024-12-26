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


#include <gtest/gtest.h>

#include "cmod_cashloan_test.h"


// Discounted Payback update to address ssc issue 616
TEST_F(CmodCashLoanTest, DiscountedPayback) {

    char file_path[256];
    int nfc1 = sprintf(file_path, "%s/test/input_cases/general_data/cashloan_discounted_payback.json", SSCDIR);
    std::ifstream file(file_path);
    std::ostringstream tmp;
    tmp << file.rdbuf();
    file.close();
    ssc_data_t dat = json_to_ssc_data(tmp.str().c_str());
    tmp.str("");
    // setup path to weather file
    nfc1 = sprintf(file_path, "%s/test/input_cases/general_data/phoenix_az_33.450495_-111.983688_psmv3_60_tmy.csv", SSCDIR);
    ssc_data_set_string(dat, "solar_resource_file", file_path);

    // Run with fixed output
    int errors = run_module(dat, "pvwattsv7");
    errors += run_module(dat, "belpe");
    errors += run_module(dat, "grid");
    errors += run_module(dat, "utilityrate5");
    errors += run_module(dat, "cashloan");

    EXPECT_FALSE(errors);
    if (!errors)
    {
        ssc_number_t payback, discounted_payback, npv;
        ssc_data_get_number(dat, "payback", &payback);
        ssc_data_get_number(dat, "discounted_payback", &discounted_payback);
        ssc_data_get_number(dat, "npv", &npv);

        // values from cashloan_discounted_payback.xlsx
        EXPECT_NEAR(payback, 6.7, 6.7*0.01);
        EXPECT_NEAR(discounted_payback, 10.5, 10.5*0.01);
        EXPECT_NEAR(npv, 1207.0, 1207.0*0.01);
    }
    ssc_data_free(dat);
    dat = nullptr;
}



TEST_F(CmodCashLoanTest, PVWattsResidential) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_PVWatts_Residential_cmod_cashloan.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_PVWatts_Residential_cmod_cashloan_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_nom", "npv", "payback" };
    std::vector<std::string> compare_array_variables = { "cf_after_tax_cash_flow", "cf_value_added" };

    Test("cashloan", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodCashLoanTest, PVWattsCommercial) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_PVWatts_Commercial_cmod_cashloan.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_PVWatts_Commercial_cmod_cashloan_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_nom", "npv", "payback" };
    std::vector<std::string> compare_array_variables = { "cf_after_tax_cash_flow", "cf_value_added" };

    Test("cashloan", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodCashLoanTest, PVWattsBatteryResidential) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_PVWatts_Battery_Residential_cmod_cashloan.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_PVWatts_Battery_Residential_cmod_cashloan_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_nom", "npv", "payback" };
    std::vector<std::string> compare_array_variables = { "cf_after_tax_cash_flow", "cf_value_added", "cf_after_tax_net_equity_cost_flow", "cf_parasitic_cost", "cf_util_escal_rate" };

    Test("cashloan", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodCashLoanTest, PVWattsBatteryCommercial) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_PVWatts_Battery_Commercial_cmod_cashloan.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_PVWatts_Battery_Commercial_cmod_cashloan_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_nom", "npv", "payback" };
    std::vector<std::string> compare_array_variables = { "cf_after_tax_cash_flow", "cf_value_added" };

    Test("cashloan", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodCashLoanTest, PVResidential) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_Flat_Plate_PV_Residential_cmod_cashloan.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_Flat_Plate_PV_Residential_cmod_cashloan_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_nom", "npv", "payback" };
    std::vector<std::string> compare_array_variables = { "cf_after_tax_cash_flow", "cf_value_added" };

    Test("cashloan", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodCashLoanTest, PVCommercial) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_Flat_Plate_PV_Commercial_cmod_cashloan.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_Flat_Plate_PV_Commercial_cmod_cashloan_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_nom", "npv", "payback" };
    std::vector<std::string> compare_array_variables = { "cf_after_tax_cash_flow", "cf_value_added" , "cf_energy_sales",  "cf_energy_net" };

    Test("cashloan", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}


TEST_F(CmodCashLoanTest, PVBatteryResidential) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_PV_Battery_Residential_cmod_cashloan.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_PV_Battery_Residential_cmod_cashloan_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_nom", "npv", "payback" };
    std::vector<std::string> compare_array_variables = { "cf_after_tax_cash_flow", "cf_value_added" };

    Test("cashloan", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodCashLoanTest, PVBatteryCommercial) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_PV_Battery_Commercial_cmod_cashloan.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_PV_Battery_Commercial_cmod_cashloan_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_nom", "npv", "payback" };
    std::vector<std::string> compare_array_variables = { "cf_after_tax_cash_flow", "cf_value_added" };

    Test("cashloan", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodCashLoanTest, CustomGenerationResidential) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_Generic_System_Residential_cmod_cashloan.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_Generic_System_Residential_cmod_cashloan_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_nom", "npv", "payback" };
    std::vector<std::string> compare_array_variables = { "cf_after_tax_cash_flow", "cf_value_added" };

    Test("cashloan", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodCashLoanTest, CustomGenerationCommercial) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_Generic_System_Commercial_cmod_cashloan.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_Generic_System_Commercial_cmod_cashloan_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_nom", "npv", "payback" };
    std::vector<std::string> compare_array_variables = { "cf_after_tax_cash_flow", "cf_value_added" };

    Test("cashloan", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodCashLoanTest, CustomGenerationBatteryResidential) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_Generic_Battery_Residential_cmod_cashloan.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_Generic_Battery_Residential_cmod_cashloan_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_nom", "npv", "payback" };
    std::vector<std::string> compare_array_variables = { "cf_after_tax_cash_flow", "cf_value_added" };

    Test("cashloan", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodCashLoanTest, CustomGenerationBatteryCommercial) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_Generic_Battery_Commercial_cmod_cashloan.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_Generic_Battery_Commercial_cmod_cashloan_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_nom", "npv", "payback" };
    std::vector<std::string> compare_array_variables = { "cf_after_tax_cash_flow", "cf_value_added" };

    Test("cashloan", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodCashLoanTest, DSLFCommercial) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_DSLF_Commercial_cmod_cashloan.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_DSLF_Commercial_cmod_cashloan_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_nom", "npv", "payback" };
    std::vector<std::string> compare_array_variables = { "cf_after_tax_cash_flow", "cf_value_added", "cf_after_tax_net_equity_cost_flow", "cf_parasitic_cost", "cf_util_escal_rate" };

    Test("cashloan", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodCashLoanTest, EmpiricalTroughCommercial) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_Empirical_Trough_Commercial_cmod_cashloan.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_Empirical_Trough_Commercial_cmod_cashloan_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_nom", "npv", "payback" };
    std::vector<std::string> compare_array_variables = { "cf_after_tax_cash_flow", "cf_value_added", "cf_after_tax_net_equity_cost_flow", "cf_parasitic_cost", "cf_util_escal_rate" };

    Test("cashloan", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodCashLoanTest, FuelCellCommercial) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_Fuel_Cell_Commercial_cmod_cashloan.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_Fuel_Cell_Commercial_cmod_cashloan_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_nom", "npv", "payback" };
    std::vector<std::string> compare_array_variables = { "cf_after_tax_cash_flow", "cf_value_added", "cf_after_tax_net_equity_cost_flow", "cf_parasitic_cost", "cf_util_escal_rate", "cf_energy_value", "cf_thermal_value" };

    Test("cashloan", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodCashLoanTest, GenericCSPCommercial) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_Generic_CSP_System_Commercial_cmod_cashloan.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_Generic_CSP_System_Commercial_cmod_cashloan_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_nom", "npv", "payback" };
    std::vector<std::string> compare_array_variables = { "cf_after_tax_cash_flow", "cf_value_added", "cf_after_tax_net_equity_cost_flow", "cf_parasitic_cost", "cf_util_escal_rate" };

    Test("cashloan", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodCashLoanTest, MSLFCommercial) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_MSLF_Commercial_cmod_cashloan.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_MSLF_Commercial_cmod_cashloan_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_nom", "npv", "payback" };
    std::vector<std::string> compare_array_variables = { "cf_after_tax_cash_flow", "cf_value_added", "cf_after_tax_net_equity_cost_flow", "cf_parasitic_cost", "cf_util_escal_rate" };

    Test("cashloan", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodCashLoanTest, SWHResidential) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_Solar_Water_Heating_Residential_cmod_cashloan.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_Solar_Water_Heating_Residential_cmod_cashloan_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_nom", "npv", "payback" };
    std::vector<std::string> compare_array_variables = { "cf_after_tax_cash_flow", "cf_value_added", "cf_after_tax_net_equity_cost_flow", "cf_parasitic_cost", "cf_util_escal_rate" };

    Test("cashloan", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodCashLoanTest, SWHCommercial) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_Solar_Water_Heating_Commercial_cmod_cashloan.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_Solar_Water_Heating_Commercial_cmod_cashloan_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_nom", "npv", "payback" };
    std::vector<std::string> compare_array_variables = { "cf_after_tax_cash_flow", "cf_value_added", "cf_after_tax_net_equity_cost_flow", "cf_parasitic_cost", "cf_util_escal_rate" };

    Test("cashloan", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodCashLoanTest, StandaloneBatteryResidential) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_Standalone_Battery_Residential_cmod_cashloan.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_Standalone_Battery_Residential_cmod_cashloan_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_nom", "npv", "payback" };
    std::vector<std::string> compare_array_variables = { "cf_after_tax_cash_flow", "cf_value_added", "cf_after_tax_net_equity_cost_flow", "cf_parasitic_cost", "cf_util_escal_rate" };

    Test("cashloan", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodCashLoanTest, StandaloneBatteryCommercial) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_Standalone_Battery_Commercial_cmod_cashloan.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_Standalone_Battery_Commercial_cmod_cashloan_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_nom", "npv", "payback" };
    std::vector<std::string> compare_array_variables = { "cf_after_tax_cash_flow", "cf_value_added", "cf_after_tax_net_equity_cost_flow", "cf_parasitic_cost", "cf_util_escal_rate" };

    Test("cashloan", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodCashLoanTest, WindResidential) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_Wind_Power_Residential_cmod_cashloan.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_Wind_Power_Residential_cmod_cashloan_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_nom", "npv", "payback" };
    std::vector<std::string> compare_array_variables = { "cf_after_tax_cash_flow", "cf_value_added", "cf_after_tax_net_equity_cost_flow", "cf_parasitic_cost", "cf_util_escal_rate" };

    Test("cashloan", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodCashLoanTest, WindCommercial) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_Wind_Power_Commercial_cmod_cashloan.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/cashloan/2022.08.08_develop_branch_Wind_Power_Commercial_cmod_cashloan_outputs.json";
    std::vector<std::string> compare_number_variables = { "lcoe_nom", "npv", "payback" };
    std::vector<std::string> compare_array_variables = { "cf_after_tax_cash_flow", "cf_value_added", "cf_after_tax_net_equity_cost_flow", "cf_parasitic_cost", "cf_util_escal_rate" };

    Test("cashloan", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

