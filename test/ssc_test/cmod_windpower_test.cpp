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


#include <fstream>
#include <gtest/gtest.h>
#include <vector>

#include "../rapidjson/document.h"
#include "../rapidjson/istreamwrapper.h"

#include <lib_util.h>
#include "../ssc/sscapi.h"
#include "vartab.h"
#include "cmod_windpower_test.h"

/// Measurement heights are different from the turbine's hub height
TEST_F(CMWindPowerIntegration, HubHeightInterpolation_cmod_windpower) {
    // Case 1: hubheight is 200, error
    ssc_data_unassign(data, "wind_resource_filename");
    var_table* vt = static_cast<var_table*>(data);

    auto windresourcedata = create_winddata_array(1, 1);
    ssc_data_set_table(data, "wind_resource_data", windresourcedata);

    vt->assign("wind_turbine_hub_ht", 200);

    bool completed = compute(false);
    EXPECT_FALSE(completed) << "Heights difference > 35m";

    // Case 2: hubweight is 90, use shear exponent interpolation
    vt->unassign("wind_turbine_hub_ht");
    vt->assign("wind_turbine_hub_ht", 90);

    compute();
    ssc_number_t annual_energy;
    ssc_data_get_number(data, "annual_energy", &annual_energy);
    EXPECT_GT(annual_energy, 4e06) << "Annual energy should be higher than height at 90";

    free_winddata_array(windresourcedata);
}

/// Using Wind Resource File with various Wake Models
TEST_F(CMWindPowerIntegration, WakeModelsUsingFile_cmod_windpower) {
    // Simple Wake Model
    compute();

    ssc_number_t annual_energy;
    ssc_data_get_number(data, "annual_energy", &annual_energy);
    EXPECT_NEAR(annual_energy, 33224154, e) << "Simple";

    ssc_number_t monthly_energy = ssc_data_get_array(data, "monthly_energy", nullptr)[0];
    EXPECT_NEAR(monthly_energy, 2.8218e6, e) << "Simple: January";

    monthly_energy = ssc_data_get_array(data, "monthly_energy", nullptr)[11];
    EXPECT_NEAR(monthly_energy, 2.8218e6, e) << "Simple: December";

    ssc_number_t wake_loss;
    ssc_data_get_number(data, "annual_wake_loss_internal_percent", &wake_loss);
    EXPECT_NEAR(wake_loss, 1.546, 1e-3) << "Simple: Wake loss";


    // WAsp Model
    ssc_data_set_number(data, "wind_farm_wake_model", 1);
    compute();

    ssc_data_get_number(data, "annual_energy", &annual_energy);
    EXPECT_NEAR(annual_energy, 32346158, e) << "Wasp";

    monthly_energy = ssc_data_get_array(data, "monthly_energy", nullptr)[0];
    EXPECT_NEAR(monthly_energy, 2.7472e6, e) << "Wasp: Jan";

    monthly_energy = ssc_data_get_array(data, "monthly_energy", nullptr)[11];
    EXPECT_NEAR(monthly_energy, 2.7472e6, e) << "Wasp: Dec";

    ssc_data_get_number(data, "annual_wake_loss_internal_percent", &wake_loss);
    EXPECT_NEAR(wake_loss, 4.148, 1e-3) << "Wasp: Wake loss";

    // Eddy Viscosity Model
    ssc_data_set_number(data, "wind_farm_wake_model", 2);
    compute();

    ssc_data_get_number(data, "annual_energy", &annual_energy);
    EXPECT_NEAR(annual_energy, 31081848, e) << "Eddy";

    monthly_energy = ssc_data_get_array(data, "monthly_energy", nullptr)[0];
    EXPECT_NEAR(monthly_energy, 2.6398e6, e) << "Eddy: Jan";

    monthly_energy = ssc_data_get_array(data, "monthly_energy", nullptr)[11];
    EXPECT_NEAR(monthly_energy, 2.6398e6, e) << "Eddy: Dec";

    ssc_data_get_number(data, "annual_wake_loss_internal_percent", &wake_loss);
    EXPECT_NEAR(wake_loss, 7.895, 1e-3) << "Eddy: Wake loss";

    // Constant Loss Model
    ssc_data_set_number(data, "wind_farm_wake_model", 3);
    ssc_data_set_number(data, "wake_int_loss", 5);

    compute();

    ssc_number_t gross;
    ssc_data_get_number(data, "annual_energy", &annual_energy);
    ssc_data_get_number(data, "annual_gross_energy", &gross);
    EXPECT_NEAR(annual_energy, gross * 0.95, e) << "Constant";

    ssc_data_get_number(data, "annual_wake_loss_total_percent", &wake_loss); //this wake model option doesn't report internal wake loss as an output
    EXPECT_NEAR(wake_loss, 5, 1e-3) << "Constant: Wake loss";
}

TEST_F(CMWindPowerIntegration, WakeLossMultiplier_cmod_windpower)
{
    ssc_number_t withoutMultiplier, withMultiplier;
    ssc_number_t multiplier = 1.2;

    //Simple Wake Model
    ssc_data_set_number(data, "wake_loss_multiplier", 1.0);
    compute();
    ssc_data_get_number(data, "annual_wake_loss_internal_percent", &withoutMultiplier);
    ssc_data_set_number(data, "wake_loss_multiplier", multiplier);
    compute();
    ssc_data_get_number(data, "annual_wake_loss_internal_percent", &withMultiplier);
    if (withoutMultiplier != 0.)
        EXPECT_NEAR((withMultiplier / withoutMultiplier), multiplier, 0.01) << "Simple Wake Model Multiplier";

    //WASP model (Park)
    ssc_data_set_number(data, "wind_farm_wake_model", 1);
    ssc_data_set_number(data, "wake_loss_multiplier", 1.0);
    compute();
    ssc_data_get_number(data, "annual_wake_loss_internal_percent", &withoutMultiplier);
    ssc_data_set_number(data, "wake_loss_multiplier", multiplier);
    compute();
    ssc_data_get_number(data, "annual_wake_loss_internal_percent", &withMultiplier);
    if (withoutMultiplier != 0.)
        EXPECT_NEAR((withMultiplier / withoutMultiplier), multiplier, 0.01) << "WASP Wake Model Multiplier";

    //Eddy Viscosity model
    ssc_data_set_number(data, "wind_farm_wake_model", 2);
    ssc_data_set_number(data, "wake_loss_multiplier", 1.0);
    compute();
    ssc_data_get_number(data, "annual_wake_loss_internal_percent", &withoutMultiplier);
    ssc_data_set_number(data, "wake_loss_multiplier", multiplier);
    compute();
    ssc_data_get_number(data, "annual_wake_loss_internal_percent", &withMultiplier);
    if (withoutMultiplier != 0.)
        EXPECT_NEAR((withMultiplier / withoutMultiplier), multiplier, 0.01) << "EV Wake Model Multiplier";

    //Constant Loss model
    ssc_data_set_number(data, "wind_farm_wake_model", 3);
    ssc_data_set_number(data, "wake_int_loss", 5);
    ssc_data_set_number(data, "wake_loss_multiplier", 1.0);
    EXPECT_FALSE( compute() ); //setting wake loss multiplier with constant loss value should return an error


}

/// Test using the optional coefficient of thrust curve input for the park model
TEST_F(CMWindPowerIntegration, CtCurve_cmod_windpower)
{
    //set the park wake model and get original wake loss
    ssc_data_set_number(data, "wind_farm_wake_model", 1); 
    compute();
    ssc_number_t wakeLoss1, wakeLoss2;
    ssc_data_get_number(data, "annual_wake_loss_internal_percent", &wakeLoss1); //get the wake loss without setting Ct curve

    // use a bogus ct curve just to test that it does something
    ssc_number_t ctCurve[161] = { 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5,
        0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5,
        0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5,
        0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5,
        0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5,
        0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5 };
    ssc_data_set_array(data, "wind_turbine_ct_curve", ctCurve, 161);
        
    // get new wake loss
    EXPECT_TRUE(compute());
    ssc_data_get_number(data, "annual_wake_loss_internal_percent", &wakeLoss2); //get the wake loss with Ct curve

    ssc_number_t difference = wakeLoss2 - wakeLoss1;
    EXPECT_NEAR(wakeLoss1, 4.148, 0.01) << "Wake Loss 1";
    EXPECT_NEAR(wakeLoss2, 5.562, 0.5) << "Wake Loss 2";
    EXPECT_NEAR(difference, 1.413, 0.5) << "Difference";
}

/// Using Interpolated Subhourly Wind Data
TEST_F(CMWindPowerIntegration, UsingInterpolatedSubhourly_cmod_windpower) {
    // Using AR Northwestern-Flat Lands

    const char *SSCDIR = std::getenv("SSCDIR");
    char file[256];
    sprintf(file, "%s/test/input_docs/AR Northwestern-Flat Lands.srw", SSCDIR);

    ssc_data_set_string(data, "wind_resource_filename", file);
    bool success = compute();

    EXPECT_TRUE(success) << "Computation 1 should succeed";

    ssc_number_t hourly_annual_energy;
    ssc_data_get_number(data, "annual_energy", &hourly_annual_energy);

    ssc_number_t hourly_january_energy = ssc_data_get_array(data, "monthly_energy", nullptr)[0];


    // Using 15 min File
    sprintf(file, "%s/test/input_docs/AR Northwestern-Flat Lands-15min.srw", SSCDIR);

    ssc_data_set_string(data, "wind_resource_filename", file);
    success = compute();

    EXPECT_TRUE(success) << "Computation 2 should succeed";

    ssc_number_t check_annual_energy;
    ssc_data_get_number(data, "annual_energy", &check_annual_energy);
    EXPECT_NEAR(check_annual_energy, hourly_annual_energy, 0.005 * check_annual_energy);

    ssc_number_t check_january_energy = ssc_data_get_array(data, "monthly_energy", nullptr)[0];
    EXPECT_NEAR(check_january_energy, hourly_january_energy, 0.005 * check_january_energy);

    size_t nEntries = static_cast<var_table *>(data)->lookup("gen")->num.ncols();
    EXPECT_EQ(nEntries, 8760 * 4);

    // Using 5 min File
    sprintf(file, "%s/test/input_docs/AR Northwestern-Flat Lands-5min.srw", SSCDIR);

    ssc_data_set_string(data, "wind_resource_filename", file);
    success = compute();

    EXPECT_TRUE(success) << "Computation 3 should succeed";

    ssc_data_get_number(data, "annual_energy", &check_annual_energy);
    EXPECT_NEAR(check_annual_energy, hourly_annual_energy, 0.005 * check_annual_energy);

    check_january_energy = ssc_data_get_array(data, "monthly_energy", nullptr)[0];
    EXPECT_NEAR(check_january_energy, hourly_january_energy, 0.005 * check_january_energy);

    nEntries = static_cast<var_table *>(data)->lookup("gen")->num.ncols();
    EXPECT_EQ(nEntries, 8760 * 12);
}

/// Using Wind Resource Data
TEST_F(CMWindPowerIntegration, UsingDataArray_cmod_windpower) {
    // using hourly data
    ssc_data_unassign(data, "wind_resource_filename");
    var_table *vt = static_cast<var_table *>(data);
    auto windresourcedata = create_winddata_array(1, 1);
    ssc_data_set_table(data, "wind_resource_data", windresourcedata);


    compute();
    double expectedAnnualEnergy = 4219481;
    double relErr = expectedAnnualEnergy * .001;


    ssc_number_t annual_energy;
    ssc_data_get_number(data, "annual_energy", &annual_energy);
    EXPECT_NEAR(annual_energy, expectedAnnualEnergy, relErr);

    ssc_number_t monthly_energy = ssc_data_get_array(data, "monthly_energy", nullptr)[0];
    EXPECT_NEAR(monthly_energy, 0, relErr / 10.);

    monthly_energy = ssc_data_get_array(data, "monthly_energy", nullptr)[11];
    EXPECT_NEAR(monthly_energy, 1972735, relErr / 10.);

    free_winddata_array(windresourcedata);

    // 15 min data
    ssc_data_unassign(data, "wind_resource_data");
    windresourcedata = create_winddata_array(4, 1);
    vt->assign("wind_resource_data", *windresourcedata);

    compute();

    ssc_data_get_number(data, "annual_energy", &annual_energy);
    EXPECT_NEAR(annual_energy, expectedAnnualEnergy, relErr);

    monthly_energy = ssc_data_get_array(data, "monthly_energy", nullptr)[0];
    EXPECT_NEAR(monthly_energy, 0, relErr / 10.);

    monthly_energy = ssc_data_get_array(data, "monthly_energy", nullptr)[11];
    EXPECT_NEAR(monthly_energy, 1972735, relErr / 10.);

    int gen_length = 0;
    ssc_data_get_array(data, "gen", &gen_length);
    EXPECT_EQ(gen_length, 8760 * 4);

    free_winddata_array(windresourcedata);
}

// Run the data array in lifetime mode
TEST_F(CMWindPowerIntegration, UsingDataArray_cmod_windpower_lifetime) {
    // using hourly data
    ssc_data_unassign(data, "wind_resource_filename");
    var_table* vt = static_cast<var_table*>(data);
    auto windresourcedata = create_winddata_array(1, 1);
    ssc_data_set_table(data, "wind_resource_data", windresourcedata);
    ssc_data_set_number(data, "system_use_lifetime_output", 1);
    ssc_data_set_number(data, "analysis_period", 2);
    ssc_number_t degradation[1] = { 0 };
    ssc_data_set_array(data, "generic_degradation", degradation, 1);

    compute();
    double expectedAnnualEnergy = 4219481;
    double relErr = expectedAnnualEnergy * .001;


    ssc_number_t annual_energy;
    ssc_data_get_number(data, "annual_energy", &annual_energy);
    EXPECT_NEAR(annual_energy, expectedAnnualEnergy, relErr);

    ssc_number_t monthly_energy = ssc_data_get_array(data, "monthly_energy", nullptr)[0];
    EXPECT_NEAR(monthly_energy, 0, relErr / 10.);

    monthly_energy = ssc_data_get_array(data, "monthly_energy", nullptr)[11];
    EXPECT_NEAR(monthly_energy, 1972735, relErr / 10.);

    int gen_length = 0;
    ssc_data_get_array(data, "gen", &gen_length);
    EXPECT_EQ(gen_length, 8760 * 2);

    free_winddata_array(windresourcedata);

    // 15 min data
    ssc_data_unassign(data, "wind_resource_data");
    windresourcedata = create_winddata_array(4, 1);
    vt->assign("wind_resource_data", *windresourcedata);

    compute();

    ssc_data_get_number(data, "annual_energy", &annual_energy);
    EXPECT_NEAR(annual_energy, expectedAnnualEnergy, relErr);

    monthly_energy = ssc_data_get_array(data, "monthly_energy", nullptr)[0];
    EXPECT_NEAR(monthly_energy, 0, relErr / 10.);

    monthly_energy = ssc_data_get_array(data, "monthly_energy", nullptr)[11];
    EXPECT_NEAR(monthly_energy, 1972735, relErr / 10.);

    gen_length = 0;
    ssc_data_get_array(data, "gen", &gen_length);
    EXPECT_EQ(gen_length, 8760 * 4 * 2);

    free_winddata_array(windresourcedata);
}

/// Using Weibull Distribution
TEST_F(CMWindPowerIntegration, Weibull_cmod_windpower) {
    ssc_data_set_number(data, "wind_resource_model_choice", 1);
    compute();

    ssc_number_t annual_energy;
    ssc_data_get_number(data, "annual_energy", &annual_energy);
    EXPECT_NEAR(annual_energy, 180453760, e);

    ssc_number_t monthly_energy = ssc_data_get_array(data, "monthly_energy", nullptr)[0];
    EXPECT_NEAR(monthly_energy, 15326247, e);

    monthly_energy = ssc_data_get_array(data, "monthly_energy", nullptr)[11];
    EXPECT_NEAR(monthly_energy, 15326247, e);
}

// Now in lifetime mode!
TEST_F(CMWindPowerIntegration, Weibull_cmod_windpower_lifetime) {
    ssc_data_set_number(data, "wind_resource_model_choice", 1);
    ssc_data_set_number(data, "system_use_lifetime_output", 1);
    ssc_data_set_number(data, "analysis_period", 2);
    ssc_number_t degradation[1] = { 0 };
    ssc_data_set_array(data, "generic_degradation", degradation, 1);
    compute();

    ssc_number_t annual_energy;
    ssc_data_get_number(data, "annual_energy", &annual_energy);
    EXPECT_NEAR(annual_energy, 180453760, e);

    ssc_number_t monthly_energy = ssc_data_get_array(data, "monthly_energy", nullptr)[0];
    EXPECT_NEAR(monthly_energy, 15326247, e);

    monthly_energy = ssc_data_get_array(data, "monthly_energy", nullptr)[11];
    EXPECT_NEAR(monthly_energy, 15326247, e);

    int gen_length = 0;
    ssc_data_get_array(data, "gen", &gen_length);
    EXPECT_EQ(gen_length, 8760 * 2);
}

/// Using Wind Resource 2-D Distribution
TEST_F(CMWindPowerIntegration, WindDist_cmod_windpower) {
    ssc_data_set_number(data, "wind_resource_model_choice", 2);
    double dist[18] = {1.5, 180, .12583,
                       5, 180, .3933,
                       8, 180, .18276,
                       10, 180, .1341,
                       13.5, 180, .14217,
                       19, 180, .0211};

    ssc_data_set_matrix(data, "wind_resource_distribution", dist, 6, 3);
    compute();

    ssc_number_t annual_energy;
    ssc_data_get_number(data, "annual_energy", &annual_energy);
    EXPECT_NEAR(annual_energy, 159807000, e);

    ssc_number_t monthly_energy = ssc_data_get_array(data, "monthly_energy", nullptr)[0];
    EXPECT_NEAR(monthly_energy, 13573000, e);

    monthly_energy = ssc_data_get_array(data, "monthly_energy", nullptr)[11];
    EXPECT_NEAR(monthly_energy, 13573000, e);
}

// Same as above, in lifetime mode 
TEST_F(CMWindPowerIntegration, WindDist_cmod_windpower_lifetime) {
    ssc_data_set_number(data, "wind_resource_model_choice", 2);
    double dist[18] = { 1.5, 180, .12583,
                       5, 180, .3933,
                       8, 180, .18276,
                       10, 180, .1341,
                       13.5, 180, .14217,
                       19, 180, .0211 };

    ssc_data_set_matrix(data, "wind_resource_distribution", dist, 6, 3);
    ssc_data_set_number(data, "system_use_lifetime_output", 1);
    ssc_data_set_number(data, "analysis_period", 2);
    ssc_number_t degradation[1] = { 0 };
    ssc_data_set_array(data, "generic_degradation", degradation, 1);
    compute();

    ssc_number_t annual_energy;
    ssc_data_get_number(data, "annual_energy", &annual_energy);
    EXPECT_NEAR(annual_energy, 159807000, e);

    ssc_number_t monthly_energy = ssc_data_get_array(data, "monthly_energy", nullptr)[0];
    EXPECT_NEAR(monthly_energy, 13573000, e);

    monthly_energy = ssc_data_get_array(data, "monthly_energy", nullptr)[11];
    EXPECT_NEAR(monthly_energy, 13573000, e);

    int gen_length = 0;
    ssc_data_get_array(data, "gen", &gen_length);
    EXPECT_EQ(gen_length, 8760 * 2);
}

/// Using Wind Resource 2-D Distribution
TEST_F(CMWindPowerIntegration, WindDist2_cmod_windpower) {
    ssc_data_set_number(data, "wind_resource_model_choice", 2);
    // mimic a weibull with k factor 2 and avg speed 7.25 for comparison -> scale param : 8.181
    double dst[18] = {1.5, 180, .12583,
                      5, 180, .3933,
                      8, 180, .18276,
                      10, 180, .1341,
                      13.5, 180, .14217,
                      19, 180, .0211};

    var_data dist = var_data(dst, 6, 3);
    auto *vt = static_cast<var_table *>(data);
    vt->assign("wind_resource_distribution", dist);
    compute();

    ssc_number_t annual_energy;
    ssc_data_get_number(data, "annual_energy", &annual_energy);
    EXPECT_NEAR(annual_energy, 159806945, e);

    ssc_number_t monthly_energy = ssc_data_get_array(data, "monthly_energy", nullptr)[0];
    EXPECT_NEAR(monthly_energy, 13572644, e);

    monthly_energy = ssc_data_get_array(data, "monthly_energy", nullptr)[11];
    EXPECT_NEAR(monthly_energy, 13572644, e);
}

/// Using Wind Resource 2-D Distribution with Constant Loss Wake Model
TEST_F(CMWindPowerIntegration, WindDist3_cmod_windpower) {
    ssc_data_set_number(data, "wind_resource_model_choice", 2);
    // mimic a weibull with k factor 2 and avg speed 7.25 for comparison -> scale param : 8.181
    double dst[18] = {1.5, 180, .12583,
                      5, 180, .3933,
                      8, 180, .18276,
                      10, 180, .1341,
                      13.5, 180, .14217,
                      19, 180, .0211};

    var_data dist = var_data(dst, 6, 3);
    auto *vt = static_cast<var_table *>(data);
    vt->assign("wind_resource_distribution", dist);
    ssc_data_set_number(data, "wind_farm_wake_model", 3);
    ssc_data_set_number(data, "wake_int_loss", 5);
    ssc_data_set_number(data, "avail_turb_loss", 5);

    compute();

    ssc_number_t annual_energy, gross;
    ssc_data_get_number(data, "annual_energy", &annual_energy);
    ssc_data_get_number(data, "annual_gross_energy", &gross);
    EXPECT_NEAR(gross, 160804000, e);
    EXPECT_NEAR(annual_energy, gross * 0.95 * 0.95, e);

    ssc_number_t monthly_energy = ssc_data_get_array(data, "monthly_energy", nullptr)[0];
    EXPECT_NEAR(monthly_energy, 12326000, e);

}

/// Icing and Low Temp Cutoff, with Wind Resource Data
TEST_F(CMWindPowerIntegration, IcingAndLowTempCutoff_cmod_windpower) {
    //modify test inputs
    ssc_data_unassign(data, "wind_resource_filename");
//    var_data *windresourcedata = create_winddata_array(1, 1);
    auto windresourcedata = create_winddata_array(1, 1);

    double rh[8760];
    for (unsigned int i = 0; i < 8760; i++) {
        if (i % 2 == 0) rh[i] = 0.75f;
        else rh[i] = 0.0f;
    }
    var_data rh_vd = var_data(rh, 8760);
    windresourcedata->assign("rh", rh_vd);
    auto *vt = static_cast<var_table *>(data);
    ssc_data_set_table(data, "wind_resource_data", windresourcedata);
    vt->assign("en_low_temp_cutoff", 1);
    vt->assign("en_icing_cutoff", 1);
    vt->assign("low_temp_cutoff", 40.f);
    vt->assign("icing_cutoff_temp", 55.f);
    vt->assign("icing_cutoff_rh", 0.70f);

    compute();

    ssc_number_t annual_energy;
    ssc_data_get_number(data, "annual_energy", &annual_energy);
    EXPECT_NEAR(annual_energy, 2110545, e) << "Reduced annual energy";

    ssc_number_t monthly_energy = ssc_data_get_array(data, "monthly_energy", nullptr)[0];
    EXPECT_NEAR(monthly_energy, 0, e);

    monthly_energy = ssc_data_get_array(data, "monthly_energy", nullptr)[11];
    EXPECT_NEAR(monthly_energy, 986114, e);

    ssc_number_t losses_percent;
    ssc_data_get_number(data, "cutoff_losses", &losses_percent);
    EXPECT_NEAR(losses_percent, 0.5, 0.01);

    //now test the persistence feature
    vt->assign("icing_persistence_timesteps", 100);
    compute();
    ssc_data_get_number(data, "cutoff_losses", &losses_percent);
    EXPECT_NEAR(losses_percent, 1, 0.01);

    free_winddata_array(windresourcedata);
}

/// Override for number of wind turbines with wake model
TEST_F(CMWindPowerIntegration, WakeModelMaxTurbineOverride) {
    // set up 310 turbines
    ssc_number_t xcoord[310];
    ssc_number_t ycoord[310];
    for (int i = 0; i < 310; i++) {
        double x = i % 20;
        xcoord[i] = (ssc_number_t)x;
        double y = floor(i / 30);
        ycoord[i] = (ssc_number_t)y;
    }
    ssc_data_set_array(data, "wind_farm_xCoordinates", xcoord, 310);
    ssc_data_set_array(data, "wind_farm_yCoordinates", ycoord, 310);

    // should fail with an error for 310 turbines
    ssc_module_t module = ssc_module_create("windpower");
    ASSERT_FALSE(ssc_module_exec(module, data));

    // set new override input and test that it works
    ssc_data_set_number(data, "max_turbine_override", 315);
    ssc_module_exec(module, data);
    ssc_number_t annual_energy;
    ssc_data_get_number(data, "annual_energy", &annual_energy);
    EXPECT_NEAR(annual_energy, 31636868.6, 1) << "Turbine override";

    ssc_module_free(module);
}


/// Testing Turbine powercurve calculation
TEST(Turbine_powercurve_cmod_windpower_eqns, NoData) {
    ASSERT_FALSE(Turbine_calculate_powercurve(nullptr));
}

TEST(Turbine_powercurve_cmod_windpower_eqns, MissingVariables) {
    var_table *vd = new var_table;
    ASSERT_FALSE(Turbine_calculate_powercurve(vd));
    delete vd;
}

TEST(Turbine_powercurve_cmod_windpower_eqns, Case1) {
    var_table *vd = new var_table;
    vd->assign("turbine_size", 1500);
    vd->assign("wind_turbine_rotor_diameter", 75);
    vd->assign("elevation", 0);
    vd->assign("wind_turbine_max_cp", 0.45);
    vd->assign("max_tip_speed", 80);
    vd->assign("max_tip_sp_ratio", 8);
    vd->assign("cut_in", 4);
    vd->assign("cut_out", 25);
    vd->assign("drive_train", 0);

    Turbine_calculate_powercurve(vd);

    util::matrix_t<ssc_number_t> ws = vd->lookup("wind_turbine_powercurve_windspeeds")->num;
    util::matrix_t<ssc_number_t> power = vd->lookup("wind_turbine_powercurve_powerout")->num;
    util::matrix_t<ssc_number_t> eff = vd->lookup("hub_efficiency")->num;
    double rated_wx = vd->lookup("rated_wind_speed")->num;
    ASSERT_NEAR(power[17], 64.050, 1e-2);
    ASSERT_NEAR(power[18], 80.0420, 1e-2);
    ASSERT_NEAR(power[43], 1346.764, 1e-2);
    ASSERT_NEAR(power[44], 1431.227, 1e-2);
    ASSERT_NEAR(power[45], 1500., 1e-2);
    ASSERT_NEAR(power[100], 0., 1e-2);
    ASSERT_NEAR(ws[100], 25., 1e-2);
    ASSERT_NEAR(rated_wx, 11.204, 1e-2);

    delete vd;
}

TEST(Turbine_powercurve_cmod_windpower_eqns, Case2) {
    var_table *vd = new var_table;
    vd->assign("turbine_size", 1500);
    vd->assign("wind_turbine_rotor_diameter", 75);
    vd->assign("elevation", 0);
    vd->assign("wind_turbine_max_cp", 0.45);
    vd->assign("max_tip_speed", 80);
    vd->assign("max_tip_sp_ratio", 8);
    vd->assign("cut_in", 4);
    vd->assign("cut_out", 25);
    vd->assign("drive_train", 1);

    Turbine_calculate_powercurve(vd);

    util::matrix_t<ssc_number_t> ws = vd->lookup("wind_turbine_powercurve_windspeeds")->num;
    util::matrix_t<ssc_number_t> power = vd->lookup("wind_turbine_powercurve_powerout")->num;
    util::matrix_t<ssc_number_t> eff = vd->lookup("hub_efficiency")->num;
    double rated_wx = vd->lookup("rated_wind_speed")->num;
    ASSERT_NEAR(power[17], 67.26, 1e-2);
    ASSERT_NEAR(power[18], 83.971, 1e-2);
    ASSERT_NEAR(power[44], 1416.36, 1e-2);
    ASSERT_NEAR(power[45], 1494.44, 1e-2);
    ASSERT_NEAR(power[46], 1500., 1e-2);
    ASSERT_NEAR(power[100], 0., 1e-2);
    ASSERT_NEAR(ws[100], 25., 1e-2);
    ASSERT_NEAR(rated_wx, 11.27, 1e-2);

    delete vd;
}

TEST(Turbine_powercurve_cmod_windpower_eqns, Case3) {
    var_table *vd = new var_table;
    vd->assign("turbine_size", 1500);
    vd->assign("wind_turbine_rotor_diameter", 75);
    vd->assign("elevation", 0);
    vd->assign("wind_turbine_max_cp", 0.45);
    vd->assign("max_tip_speed", 80);
    vd->assign("max_tip_sp_ratio", 8);
    vd->assign("cut_in", 4);
    vd->assign("cut_out", 25);
    vd->assign("drive_train", 2);

    Turbine_calculate_powercurve(vd);

    util::matrix_t<ssc_number_t> ws = vd->lookup("wind_turbine_powercurve_windspeeds")->num;
    util::matrix_t<ssc_number_t> power = vd->lookup("wind_turbine_powercurve_powerout")->num;
    util::matrix_t<ssc_number_t> eff = vd->lookup("hub_efficiency")->num;
    double rated_wx = vd->lookup("rated_wind_speed")->num;
    ASSERT_NEAR(power[17], 62.66, 1e-2);
    ASSERT_NEAR(power[18], 79.24, 1e-2);
    ASSERT_NEAR(power[44], 1405.26, 1e-2);
    ASSERT_NEAR(power[45], 1483.27, 1e-2);
    ASSERT_NEAR(power[46], 1500., 1e-2);
    ASSERT_NEAR(power[100], 0., 1e-2);
    ASSERT_NEAR(ws[100], 25., 1e-2);
    ASSERT_NEAR(rated_wx, 11.30, 1e-2);

    delete vd;
}

TEST(Turbine_powercurve_cmod_windpower_eqns, Case4) {
    var_table *vd = new var_table;
    vd->assign("turbine_size", 1500);
    vd->assign("wind_turbine_rotor_diameter", 75);
    vd->assign("elevation", 0);
    vd->assign("wind_turbine_max_cp", 0.45);
    vd->assign("max_tip_speed", 80);
    vd->assign("max_tip_sp_ratio", 8);
    vd->assign("cut_in", 4);
    vd->assign("cut_out", 25);
    vd->assign("drive_train", 3);

    Turbine_calculate_powercurve(vd);

    util::matrix_t<ssc_number_t> ws = vd->lookup("wind_turbine_powercurve_windspeeds")->num;
    util::matrix_t<ssc_number_t> power = vd->lookup("wind_turbine_powercurve_powerout")->num;
    util::matrix_t<ssc_number_t> eff = vd->lookup("hub_efficiency")->num;
    double rated_wx = vd->lookup("rated_wind_speed")->num;
    ASSERT_NEAR(power[17], 74.44, 1e-2);
    ASSERT_NEAR(power[18], 91.43, 1e-2);
    ASSERT_NEAR(power[43], 1356.14, 1e-2);
    ASSERT_NEAR(power[44], 1434.82, 1e-2);
    ASSERT_NEAR(power[45], 1500., 1e-2);
    ASSERT_NEAR(power[100], 0., 1e-2);
    ASSERT_NEAR(ws[100], 25., 1e-2);
    ASSERT_NEAR(rated_wx, 11.21, 1e-2);

    delete vd;

}
