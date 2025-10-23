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


#include <gtest/gtest.h>
#include "trough_physical_defaults.h"
#include "csp_common_test.h"
#include "vs_google_test_explorer_namespace.h"

namespace csp_trough {}
using namespace csp_trough;

//========Tests===================================================================================
NAMESPACE_TEST(csp_trough, PowerTroughCmod, Default_NoFinancial)
{
    ssc_data_t defaults = trough_physical_defaults();
    CmodUnderTest power_trough = CmodUnderTest("trough_physical", defaults);
    int errors = power_trough.RunModule();
    EXPECT_FALSE(errors);
    if (!errors) {

        double annual_energy = power_trough.GetOutput("annual_energy");
        double annual_thermal_consumption = power_trough.GetOutput("annual_thermal_consumption");
        double annual_tes_freeze_protection = power_trough.GetOutput("annual_tes_freeze_protection");
        double annual_field_freeze_protection = power_trough.GetOutput("annual_field_freeze_protection");
        double capacity_factor = power_trough.GetOutput("capacity_factor");
        double annual_W_cycle_gross = power_trough.GetOutput("annual_W_cycle_gross");
        double kwh_per_kw = power_trough.GetOutput("kwh_per_kw");
        double conversion_factor = power_trough.GetOutput("conversion_factor");
        double annual_total_water_use = power_trough.GetOutput("annual_total_water_use");

        double time_hr = power_trough.GetOutputSum("time_hr");
        double month = power_trough.GetOutputSum("month");
        double beam = power_trough.GetOutputSum("beam");
        double defocus = power_trough.GetOutputSum("defocus");
        double q_dc_tes = power_trough.GetOutputSum("q_dc_tes");
        double P_fixed = power_trough.GetOutputSum("P_fixed");
        double op_mode_1 = power_trough.GetOutputSum("op_mode_1");
        double n_op_modes = power_trough.GetOutputSum("n_op_modes");
        double is_rec_su_allowed = power_trough.GetOutputSum("is_rec_su_allowed");

        EXPECT_NEAR_FRAC(power_trough.GetOutput("annual_energy"), 377677449, kErrorToleranceHi);
        EXPECT_NEAR_FRAC(power_trough.GetOutput("annual_thermal_consumption"), 872331, kErrorToleranceHi);
        EXPECT_NEAR_FRAC(power_trough.GetOutput("annual_tes_freeze_protection"), 692200, kErrorToleranceHi);
        EXPECT_NEAR_FRAC(power_trough.GetOutput("annual_field_freeze_protection"), 180131, kErrorToleranceHi);
        EXPECT_NEAR_FRAC(power_trough.GetOutput("capacity_factor"), 43.16, kErrorToleranceHi);
        EXPECT_NEAR_FRAC(power_trough.GetOutput("annual_W_cycle_gross"), 430999201, kErrorToleranceHi);
        EXPECT_NEAR_FRAC(power_trough.GetOutput("kwh_per_kw"), 3781, kErrorToleranceHi);
        EXPECT_NEAR_FRAC(power_trough.GetOutput("conversion_factor"), 87.63, kErrorToleranceHi);
        EXPECT_NEAR_FRAC(power_trough.GetOutput("annual_total_water_use"), 82009, kErrorToleranceHi);

        EXPECT_NEAR_FRAC(power_trough.GetOutputSum("time_hr"), 38373180, kErrorToleranceHi);
        EXPECT_NEAR_FRAC(power_trough.GetOutputSum("month"), 57168, kErrorToleranceHi);
        EXPECT_NEAR_FRAC(power_trough.GetOutputSum("beam"), 2687889, kErrorToleranceHi);
        EXPECT_NEAR_FRAC(power_trough.GetOutputSum("defocus"), 8747, kErrorToleranceHi);
        EXPECT_NEAR_FRAC(power_trough.GetOutputSum("q_dc_tes"), 343833, kErrorToleranceHi);
        EXPECT_NEAR_FRAC(power_trough.GetOutputSum("P_fixed"), 5348, kErrorToleranceHi);
        EXPECT_NEAR_FRAC(power_trough.GetOutputSum("op_mode_1"), 54965, kErrorToleranceHi);
        EXPECT_NEAR_FRAC(power_trough.GetOutputSum("n_op_modes"), 9800, kErrorToleranceHi);
        EXPECT_NEAR_FRAC(power_trough.GetOutputSum("is_rec_su_allowed"), 8759, kErrorToleranceHi);
        //EXPECT_NEAR_FRAC(power_trough.GetOutputSum("operating_modes_a"), 35458021, kErrorToleranceHi);
    }
    //ssc_data_t defaults = singleowner_defaults();
    //CmodUnderTest singleowner = CmodUnderTest("singleowner", defaults);
    //int errors = singleowner.RunModule();
    //EXPECT_FALSE(errors);
    //if (!errors) {
    //    EXPECT_NEAR_FRAC(singleowner.GetOutput(""), , kErrorToleranceLo);
    //}
}

NAMESPACE_TEST(csp_trough, PowerTroughCmod, Start_Stop_Initialize_Default_NoFinancial)
{
    ssc_data_t defaults = trough_physical_defaults();
    CmodUnderTest power_trough = CmodUnderTest("trough_physical", defaults);
    int errors = power_trough.RunModule();
    EXPECT_FALSE(errors);
    std::vector<double> original_gen = power_trough.GetOutputVector("gen");
    // first short run
    power_trough.SetInput("time_start", 0);
    power_trough.SetInput("time_stop", 3600 * 10);   // first 10 hours
    power_trough.SetInput("vacuum_arrays", 1);
    errors = power_trough.RunModule();
    EXPECT_FALSE(errors);
    std::vector<double> step1_gen = power_trough.GetOutputVector("gen");
    double step1_gen_sum = power_trough.GetOutputSum("gen");

    // second short run with restart
    power_trough.SetInput("time_start", 3600 * 10);
    power_trough.SetInput("time_stop", 3600 * 20);   // next 10 hours
    power_trough.SetInput("vacuum_arrays", 1);
    // Reset plant state - solar field
    power_trough.SetInput("defocus_initial", power_trough.GetOutputVector("defocus_final").back());
    power_trough.SetInput("rec_op_mode_initial", power_trough.GetOutputVector("rec_op_mode_final").back());
    power_trough.SetInput("T_in_loop_initial", power_trough.GetOutputVector("T_in_loop_final").back());
    power_trough.SetInput("T_out_loop_initial", power_trough.GetOutputVector("T_out_loop_final").back());
    std::vector<ssc_number_t> T_out_scas_initial = power_trough.GetOutputVector("T_out_scas_last_final");
    power_trough.SetInput("T_out_scas_initial", &T_out_scas_initial[0], T_out_scas_initial.size()); // pass the whole array
    // Power cycle
    power_trough.SetInput("pc_op_mode_initial", power_trough.GetOutputVector("pc_op_mode_final").back());
    power_trough.SetInput("pc_startup_time_remain_init", power_trough.GetOutputVector("pc_startup_time_remain_final").back());
    power_trough.SetInput("pc_startup_energy_remain_initial", power_trough.GetOutputVector("pc_startup_energy_remain_final").back());
    // TES
    power_trough.SetInput("T_tank_cold_init", power_trough.GetOutputVector("T_tes_cold").back());
    power_trough.SetInput("T_tank_hot_init", power_trough.GetOutputVector("T_tes_hot").back());
    power_trough.SetInput("init_hot_htf_percent", power_trough.GetOutputVector("hot_tank_htf_percent_final").back());
    errors = power_trough.RunModule();
    EXPECT_FALSE(errors);
    std::vector<double> step2_gen = power_trough.GetOutputVector("gen");
    double step2_gen_sum = power_trough.GetOutputSum("gen");

    if (!errors) {
        EXPECT_GT(step1_gen_sum, 0.0);  // positive generation
        EXPECT_GT(step2_gen_sum, 0.0);
        // checking generation array
        for (size_t i = 0; i < step1_gen.size() + step2_gen.size(); i++) {
            if (i < step1_gen.size()) {
                EXPECT_NEAR_FRAC(std::abs(original_gen[i]), std::abs(step1_gen[i]), kErrorToleranceHi);
            }
            else {
                EXPECT_NEAR_FRAC(std::abs(original_gen[i]), std::abs(step2_gen[i - step1_gen.size()]), kErrorToleranceHi);
            }
        }
    }
}


NAMESPACE_TEST(csp_trough, PowerTroughCmod, Dispatch_Targets_Default_NoFinancial) {

    bool print_outputs = false; // True will make test fail to print output!
    int len_window = 24;
    ssc_data_t defaults = trough_physical_defaults();
    CmodUnderTest power_tower = CmodUnderTest("trough_physical", defaults);

    // Calculating max cycle thermal and start-up energy
    double W_dot_cycle_des = power_tower.GetOutput("P_ref");            //[MWe]
    double eta_cycle = power_tower.GetOutput("eta_ref");                //[-]
    double q_dot_pc_des = W_dot_cycle_des / eta_cycle;                  //[MWt]
    double startup_frac = power_tower.GetOutput("startup_frac");        //[-]
    double pc_startup_energy = q_dot_pc_des * startup_frac;             //[MWt]

    // Set targets
    std::vector<ssc_number_t> q_pc_target_su_in(len_window);
    std::vector<ssc_number_t> q_pc_target_on_in(len_window);
    std::vector<ssc_number_t> q_pc_max_in(len_window);
    std::vector<ssc_number_t> is_rec_su_allowed_in(len_window);
    std::vector<ssc_number_t> is_pc_su_allowed_in(len_window);
    std::vector<ssc_number_t> is_pc_sb_allowed_in(len_window);

    std::fill(q_pc_target_su_in.begin(), q_pc_target_su_in.end(), 0.0);
    std::fill(q_pc_target_on_in.begin(), q_pc_target_on_in.end(), 0.0);
    std::fill(q_pc_max_in.begin(), q_pc_max_in.end(), q_dot_pc_des);
    std::fill(is_rec_su_allowed_in.begin(), is_rec_su_allowed_in.end(), 1);
    std::fill(is_pc_su_allowed_in.begin(), is_pc_su_allowed_in.end(), 0);
    std::fill(is_pc_sb_allowed_in.begin(), is_pc_sb_allowed_in.end(), 0);

    // Modify schedules
    // Cycle start and operation
    int pc_start_time = 14;
    int pc_sd_time = 21;
    int pc_partload_start = 16;
    int pc_pl_duration = 2;
    double pc_pl_fraction = 0.5;
    q_pc_target_su_in[pc_start_time] = pc_startup_energy;
    for (int i = pc_start_time + 1; i < pc_sd_time; i++) {
        if (i >= pc_partload_start && i < pc_partload_start + pc_pl_duration) {
            q_pc_target_on_in[i] = q_dot_pc_des * pc_pl_fraction;
        }
        else {
            q_pc_target_on_in[i] = q_dot_pc_des;
        }
        is_pc_su_allowed_in[i] = 1.0;
    }

    // Receiver mid-day shutdown
    int rec_shutdown_start = 17;
    int rec_sd_duration = 2;
    for (int i = rec_shutdown_start; i < rec_shutdown_start + rec_sd_duration; i++) {
        is_rec_su_allowed_in[i] = 0.0;
    }

    // Set dispatch targets
    power_tower.SetInput("is_dispatch_targets", true);
    power_tower.SetInput("q_pc_target_su_in", &q_pc_target_su_in[0], q_pc_target_su_in.size());
    power_tower.SetInput("q_pc_target_on_in", &q_pc_target_on_in[0], q_pc_target_on_in.size());
    power_tower.SetInput("q_pc_max_in", &q_pc_max_in[0], q_pc_max_in.size());
    power_tower.SetInput("is_rec_su_allowed_in", &is_rec_su_allowed_in[0], is_rec_su_allowed_in.size());
    power_tower.SetInput("is_pc_su_allowed_in", &is_pc_su_allowed_in[0], is_pc_su_allowed_in.size());
    power_tower.SetInput("is_pc_sb_allowed_in", &is_pc_sb_allowed_in[0], is_pc_sb_allowed_in.size());

    // Run the model
    power_tower.SetInput("init_hot_htf_percent", 50.0);
    power_tower.SetInput("time_start", 0);
    power_tower.SetInput("time_stop", 3600 * len_window);   // only run the window length
    power_tower.SetInput("vacuum_arrays", 1);
    int errors = power_tower.RunModule();
    EXPECT_FALSE(errors);

    double gen_sum = power_tower.GetOutputSum("gen");
    std::vector<double> gen = power_tower.GetOutputVector("gen");
    std::vector<double> rec_heat_output = power_tower.GetOutputVector("q_dot_htf_sf_out");
    std::vector<double> cycle_heat = power_tower.GetOutputVector("q_pb");
    std::vector<double> cycle_su_heat = power_tower.GetOutputVector("q_pc_startup");

    for (int i = 0; i < q_pc_target_on_in.size(); i++) {
        // Check Power cycle operations
        if (q_pc_target_on_in[i] > 0.0) {      // heat is sent to cycle
            EXPECT_GT(gen[i], 0.0);            // cycle should be generating
            //EXPECT_NEAR_FRAC(cycle_heat[i], q_pc_target_on_in[i], kErrorToleranceHi);
        }
        else {
            EXPECT_LT(gen[i], 0.0);
        }
        // Check Receiver operations
        if (is_rec_su_allowed_in[i] == 0.0) {
            EXPECT_LT(rec_heat_output[i], 0.0001); // receiver shutdown
        }
    }

    // For Debugging
    if (print_outputs) {
        std::cout << left << setw(6) << "hour: "
            << setw(10) << "gen" << "\t"
            << setw(10) << "pc heat" << "\t"
            << setw(10) << "pc su ht" << "\t"
            << setw(10) << "pc targets" << "\t"
            << setw(10) << "rec output" << "\t"
            << setw(10) << "rec sign" << std::endl;
        for (int i = 0; i < gen.size(); i++) {
            std::cout << right << setw(2) << i << ": \t"
                << setw(10) << gen[i] << "\t"
                << setw(10) << cycle_heat[i] << "\t"
                << setw(10) << cycle_su_heat[i] << "\t"
                << setw(10) << q_pc_target_on_in[i] << "\t"
                << setw(10) << rec_heat_output[i] << "\t"
                << setw(10) << is_rec_su_allowed_in[i] << std::endl;
        }
        EXPECT_FALSE(true); //needed to make the test fail for output
    }
}
