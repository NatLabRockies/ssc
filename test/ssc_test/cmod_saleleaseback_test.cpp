/*
BSD 3-Clause License

Copyright Alliance for Sustainable Energy, LLC. See also https://github.com/NREL/ssc/blob/develop/LICENSE


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


#include "cmod_saleleaseback_test.h"

#include "gtest/gtest.h"


TEST_F(CmodSaleLeasebackTest, Biopower) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/saleleaseback/2023.10.27_om-expense-cash-flow_Biopower_Sale_Leaseback_cmod_saleleaseback.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/saleleaseback/2023.10.27_om-expense-cash-flow_Biopower_Sale_Leaseback_cmod_saleleaseback_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "tax_investor_aftertax_npv", "sponsor_aftertax_npv", "lcoe_real", "lppa_nom"};
    std::vector<std::string> compare_array_variables = {"cf_tax_investor_aftertax", "cf_sponsor_aftertax", "cf_annual_costs"};

    Test("saleleaseback", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodSaleLeasebackTest, DSLF) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/saleleaseback/2023.10.27_om-expense-cash-flow_DSLF_Sale_Leaseback_cmod_saleleaseback.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/saleleaseback/2023.10.27_om-expense-cash-flow_DSLF_Sale_Leaseback_cmod_saleleaseback_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "tax_investor_aftertax_npv", "sponsor_aftertax_npv", "lcoe_real", "lppa_nom"};
    std::vector<std::string> compare_array_variables = {"cf_tax_investor_aftertax", "cf_sponsor_aftertax", "cf_annual_costs"};

    Test("saleleaseback", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodSaleLeasebackTest, EmpiricalTrough) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/saleleaseback/2023.10.27_om-expense-cash-flow_Empirical_Trough_Sale_Leaseback_cmod_saleleaseback.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/saleleaseback/2023.10.27_om-expense-cash-flow_Empirical_Trough_Sale_Leaseback_cmod_saleleaseback_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "tax_investor_aftertax_npv", "sponsor_aftertax_npv", "lcoe_real", "lppa_nom"};
    std::vector<std::string> compare_array_variables = {"cf_tax_investor_aftertax", "cf_sponsor_aftertax", "cf_annual_costs"};

    Test("saleleaseback", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}


TEST_F(CmodSaleLeasebackTest, PV) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/saleleaseback/2023.10.27_om-expense-cash-flow_Flat_Plate_PV_Sale_Leaseback_cmod_saleleaseback.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/saleleaseback/2023.10.27_om-expense-cash-flow_Flat_Plate_PV_Sale_Leaseback_cmod_saleleaseback_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "tax_investor_aftertax_npv", "sponsor_aftertax_npv", "lcoe_real", "lppa_nom"};
    std::vector<std::string> compare_array_variables = {"cf_tax_investor_aftertax", "cf_sponsor_aftertax", "cf_annual_costs"};

    Test("saleleaseback", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}


TEST_F(CmodSaleLeasebackTest, CustomGenerationBattery) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/saleleaseback/2023.10.27_om-expense-cash-flow_Generic_Battery_Sale_Leaseback_cmod_saleleaseback.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/saleleaseback/2023.10.27_om-expense-cash-flow_Generic_Battery_Sale_Leaseback_cmod_saleleaseback_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "tax_investor_aftertax_npv", "sponsor_aftertax_npv", "lcoe_real", "lppa_nom"};
    std::vector<std::string> compare_array_variables = {"cf_tax_investor_aftertax", "cf_sponsor_aftertax", "cf_annual_costs"};

    Test("saleleaseback", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodSaleLeasebackTest, GenericCSP) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/saleleaseback/2023.10.27_om-expense-cash-flow_Generic_CSP_System_Sale_Leaseback_cmod_saleleaseback.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/saleleaseback/2023.10.27_om-expense-cash-flow_Generic_CSP_System_Sale_Leaseback_cmod_saleleaseback_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "tax_investor_aftertax_npv", "sponsor_aftertax_npv", "lcoe_real", "lppa_nom"};
    std::vector<std::string> compare_array_variables = {"cf_tax_investor_aftertax", "cf_sponsor_aftertax", "cf_annual_costs"};

    Test("saleleaseback", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodSaleLeasebackTest, CustomGeneration) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/saleleaseback/2023.10.27_om-expense-cash-flow_Generic_System_Sale_Leaseback_cmod_saleleaseback.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/saleleaseback/2023.10.27_om-expense-cash-flow_Generic_System_Sale_Leaseback_cmod_saleleaseback_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "tax_investor_aftertax_npv", "sponsor_aftertax_npv", "lcoe_real", "lppa_nom"};
    std::vector<std::string> compare_array_variables = {"cf_tax_investor_aftertax", "cf_sponsor_aftertax", "cf_annual_costs"};

    Test("saleleaseback", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}


TEST_F(CmodSaleLeasebackTest, Geotherrmal) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/saleleaseback/2023.10.27_om-expense-cash-flow_Geothermal_Power_Sale_Leaseback_cmod_saleleaseback.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/saleleaseback/2023.10.27_om-expense-cash-flow_Geothermal_Power_Sale_Leaseback_cmod_saleleaseback_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "tax_investor_aftertax_npv", "sponsor_aftertax_npv", "lcoe_real", "lppa_nom"};
    std::vector<std::string> compare_array_variables = {"cf_tax_investor_aftertax", "cf_sponsor_aftertax", "cf_annual_costs"};

    Test("saleleaseback", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodSaleLeasebackTest, CPV) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/saleleaseback/2023.10.27_om-expense-cash-flow_High-X_Concentrating_PV_Sale_Leaseback_cmod_saleleaseback.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/saleleaseback/2023.10.27_om-expense-cash-flow_High-X_Concentrating_PV_Sale_Leaseback_cmod_saleleaseback_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "tax_investor_aftertax_npv", "sponsor_aftertax_npv", "lcoe_real", "lppa_nom"};
    std::vector<std::string> compare_array_variables = {"cf_tax_investor_aftertax", "cf_sponsor_aftertax", "cf_annual_costs"};

    Test("saleleaseback", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}


TEST_F(CmodSaleLeasebackTest, MSLF) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/saleleaseback/2023.10.27_om-expense-cash-flow_MSLF_Sale_Leaseback_cmod_saleleaseback.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/saleleaseback/2023.10.27_om-expense-cash-flow_MSLF_Sale_Leaseback_cmod_saleleaseback_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "tax_investor_aftertax_npv", "sponsor_aftertax_npv", "lcoe_real", "lppa_nom"};
    std::vector<std::string> compare_array_variables = {"cf_tax_investor_aftertax", "cf_sponsor_aftertax", "cf_annual_costs"};

    Test("saleleaseback", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodSaleLeasebackTest, MSPT) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/saleleaseback/2023.10.27_om-expense-cash-flow_MSPT_Sale_Leaseback_cmod_saleleaseback.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/saleleaseback/2023.10.27_om-expense-cash-flow_MSPT_Sale_Leaseback_cmod_saleleaseback_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "tax_investor_aftertax_npv", "sponsor_aftertax_npv", "lcoe_real", "lppa_nom"};
    std::vector<std::string> compare_array_variables = {"cf_tax_investor_aftertax", "cf_sponsor_aftertax", "cf_annual_costs"};

    Test("saleleaseback", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodSaleLeasebackTest, PhysicalTrough) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/saleleaseback/2023.10.27_om-expense-cash-flow_Physical_Trough_Sale_Leaseback_cmod_saleleaseback.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/saleleaseback/2023.10.27_om-expense-cash-flow_Physical_Trough_Sale_Leaseback_cmod_saleleaseback_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "tax_investor_aftertax_npv", "sponsor_aftertax_npv", "lcoe_real", "lppa_nom"};
    std::vector<std::string> compare_array_variables = {"cf_tax_investor_aftertax", "cf_sponsor_aftertax", "cf_annual_costs"};

    Test("saleleaseback", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}


TEST_F(CmodSaleLeasebackTest, PVBattery) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/saleleaseback/2023.10.27_om-expense-cash-flow_PV_Battery_Sale_Leaseback_cmod_saleleaseback.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/saleleaseback/2023.10.27_om-expense-cash-flow_PV_Battery_Sale_Leaseback_cmod_saleleaseback_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "tax_investor_aftertax_npv", "sponsor_aftertax_npv", "lcoe_real", "lppa_nom"};
    std::vector<std::string> compare_array_variables = {"cf_tax_investor_aftertax", "cf_sponsor_aftertax", "cf_annual_costs"};

    Test("saleleaseback", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}


TEST_F(CmodSaleLeasebackTest, PVWatts) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/saleleaseback/2023.10.27_om-expense-cash-flow_PVWatts_Sale_Leaseback_cmod_saleleaseback.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/saleleaseback/2023.10.27_om-expense-cash-flow_PVWatts_Sale_Leaseback_cmod_saleleaseback_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "tax_investor_aftertax_npv", "sponsor_aftertax_npv", "lcoe_real", "lppa_nom"};
    std::vector<std::string> compare_array_variables = {"cf_tax_investor_aftertax", "cf_sponsor_aftertax", "cf_annual_costs"};

    Test("saleleaseback", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}

TEST_F(CmodSaleLeasebackTest, StandaloneBattery) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/saleleaseback/2023.10.27_om-expense-cash-flow_Standalone_Battery_Sale_Leaseback_cmod_saleleaseback.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/saleleaseback/2023.10.27_om-expense-cash-flow_Standalone_Battery_Sale_Leaseback_cmod_saleleaseback_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "tax_investor_aftertax_npv", "sponsor_aftertax_npv", "lcoe_real", "lppa_nom"};
    std::vector<std::string> compare_array_variables = {"cf_tax_investor_aftertax", "cf_sponsor_aftertax", "cf_annual_costs"};

    Test("saleleaseback", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}


TEST_F(CmodSaleLeasebackTest, Wind) {
    std::string file_inputs = SSCDIR;
    file_inputs += "/test/input_json/FinancialModels/saleleaseback/2023.10.27_om-expense-cash-flow_Wind_Power_Sale_Leaseback_cmod_saleleaseback.json";
    std::string file_outputs = SSCDIR;
    file_outputs += "/test/input_json/FinancialModels/saleleaseback/2023.10.27_om-expense-cash-flow_Wind_Power_Sale_Leaseback_cmod_saleleaseback_outputs.json";
    std::vector<std::string> compare_number_variables = { "ppa", "tax_investor_aftertax_npv", "sponsor_aftertax_npv", "lcoe_real", "lppa_nom"};
    std::vector<std::string> compare_array_variables = {"cf_tax_investor_aftertax", "cf_sponsor_aftertax", "cf_annual_costs"};

    Test("saleleaseback", file_inputs, file_outputs, compare_number_variables, compare_array_variables);
}


