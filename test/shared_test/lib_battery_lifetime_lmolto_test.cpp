/*
BSD 3-Clause License

Copyright (c) Alliance for Energy Innovation, LLC. See also https://github.com/NatLabRockies/ssc/blob/develop/LICENSE
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
#include <fstream>
#include <cmath>
#include "../rapidjson/document.h"
#include "../rapidjson/istreamwrapper.h"

#include "lib_util.h"
#include "lib_battery_lifetime_lmolto.h"

class lib_battery_lifetime_lmolto_test : public ::testing::Test{
protected:
    std::unique_ptr<lifetime_lmolto_t> model;

    double dt_hour = 1;
public:
    void SetUp() override {
        model = std::unique_ptr<lifetime_lmolto_t>(new lifetime_lmolto_t(dt_hour));
    }
};


TEST_F(lib_battery_lifetime_lmolto_test, InitTest) {
    double tol = 0.001;

    //check lifetime_lmolto_state initialization
    auto lifetime_state = model->get_state();
    EXPECT_NEAR(lifetime_state.lmo_lto->dq_relative_cal, 0, tol);
    EXPECT_NEAR(lifetime_state.lmo_lto->dq_relative_cyc, 0, tol);
    EXPECT_EQ(model->get_state().day_age_of_battery, 0);
    EXPECT_EQ(model->get_state().n_cycles, 0);
}

TEST_F(lib_battery_lifetime_lmolto_test, CopyTest) {
    // check lifetime_lmolto_state get & set
    auto state = model->get_state();
    state.cycle->cycle_counts = {{0, 1}, {1, 1}};
    state.cycle->cycle_DOD_max = {2, 3};
    model->set_state(state);

    state = model->get_state();
    EXPECT_EQ(state.cycle->cycle_counts[0][0], 0);
    EXPECT_EQ(state.cycle->cycle_counts[1][0], 1);
    EXPECT_EQ(state.cycle->cycle_DOD_max[0], 2);
    EXPECT_EQ(state.cycle->cycle_DOD_max[1], 3);

    auto new_model = lifetime_lmolto_t(*model);
    state = new_model.get_state();
    EXPECT_EQ(state.cycle->cycle_counts[0][0], 0);
    EXPECT_EQ(state.cycle->cycle_counts[1][0], 1);
    EXPECT_EQ(state.cycle->cycle_DOD_max[0], 2);
    EXPECT_EQ(state.cycle->cycle_DOD_max[1], 3);
}

TEST_F(lib_battery_lifetime_lmolto_test, CyclingCRate) {
    size_t day = 0;

    // 90 cycle_DOD cycle once per day, slower Crate than above
    std::vector<double> DODs_day = {50., 56.67, 63.33, 70., 76.67, 83.33,
                                    90., 83.33, 76.67, 70., 63.33, 56.67, 50., 43.33, 36.67, 30., 23.33, 16.67,
                                    10., 16.67, 23.33, 30., 36.67, 43.33};

    while (day < 87) {
        for (size_t i = 0; i < DODs_day.size(); i++) {
            size_t idx = day * 24 + i;
            bool charge_changed = i == 7 || i == 19;
            double prev_DOD = DODs_day[i % 24];
            double DOD = DODs_day[i];
            model->runLifetimeModels(idx, charge_changed, prev_DOD, DOD, 25);
        }
        day ++;
    }

    auto state = model->get_state();

    EXPECT_EQ(state.n_cycles, 86);
    EXPECT_NEAR(state.lmo_lto->dq_relative_cal, 0.243, 1);
    EXPECT_NEAR(state.lmo_lto->dq_relative_cyc, 0, 1);
    EXPECT_NEAR(state.q_relative, 99.757, 1);
    EXPECT_NEAR(state.day_age_of_battery, 87, 1e-3);

    while (day < 870) {
        for (size_t i = 0; i < DODs_day.size(); i++) {
            size_t idx = day * 24 + i;
            bool charge_changed = i == 7 || i == 19;
            double prev_DOD = DODs_day[i % 24];
            double DOD = DODs_day[i];
            model->runLifetimeModels(idx, charge_changed, prev_DOD, DOD, 25);
        }
        day ++;
    }

    state = model->get_state();

    EXPECT_EQ(state.n_cycles, 869);
    EXPECT_NEAR(state.lmo_lto->dq_relative_cal, 1.011, 1);
    EXPECT_NEAR(state.lmo_lto->dq_relative_cyc, 0, 1);
    EXPECT_NEAR(state.q_relative, 98.98, 1);
    EXPECT_NEAR(state.day_age_of_battery, 870, 1e-3);
}

TEST_F(lib_battery_lifetime_lmolto_test, CyclingCRateMinuteTimestep) {
    double dt_hr = 1. / 60;
    model = std::unique_ptr<lifetime_lmolto_t>(new lifetime_lmolto_t(dt_hr));

    size_t day = 0;
    size_t idx = 0;
    std::vector<double> DODs_day = {50., 56.67, 63.33, 70., 76.67, 83.33,
                                    90., 83.33, 76.67, 70., 63.33, 56.67, 50., 43.33, 36.67, 30., 23.33, 16.67,
                                    10., 16.67, 23.33, 30., 36.67, 43.33};

    while (day < 87) {
        for (size_t hr = 0; hr < DODs_day.size(); hr++) {
            double prev_DOD = DODs_day[hr % 24];
            double DOD = DODs_day[hr];
            for (size_t min = 0; min < (size_t)(1. / dt_hr); min++) {
                bool charge_changed = (hr == 7 || hr == 19) && min == 0;
                if (min != 0)
                    prev_DOD = DOD;
                model->runLifetimeModels(idx, charge_changed, prev_DOD, DOD, 25);
                idx ++;
            }
        }
        day ++;
    }

    auto state = model->get_state();

    EXPECT_EQ(state.n_cycles, 86);
    EXPECT_NEAR(state.lmo_lto->dq_relative_cal, 0.265, 1e-3);
    EXPECT_NEAR(state.lmo_lto->dq_relative_cyc, 0, 1e-3);
    EXPECT_NEAR(state.q_relative, 99.735, 1e-3);
    EXPECT_NEAR(state.day_age_of_battery, 87, 1e-3);

    while (day < 870) {
        for (size_t hr = 0; hr < DODs_day.size(); hr++) {
            double prev_DOD = DODs_day[hr % 24];
            double DOD = DODs_day[hr];
            for (size_t min = 0; min < (size_t)(1. / dt_hr); min++) {
                bool charge_changed = (hr == 7 || hr == 19) && min == 0;
                if (min != 0)
                    prev_DOD = DOD;
                model->runLifetimeModels(idx, charge_changed, prev_DOD, DOD, 25);
                idx ++;
            }
        }
        day ++;
    }

    state = model->get_state();

    EXPECT_EQ(state.n_cycles, 869);
    EXPECT_NEAR(state.lmo_lto->dq_relative_cal, 1.105, 1e-3);
    EXPECT_NEAR(state.lmo_lto->dq_relative_cyc, 0, 1e-3);
    EXPECT_NEAR(state.q_relative, 98.895, 1e-3);
    EXPECT_NEAR(state.day_age_of_battery, 870, 1e-3);
}

TEST_F(lib_battery_lifetime_lmolto_test, CyclingEveryTwoDays) {
    double T = 25.15;
    size_t day = 0;
    while (day < 87) {
        for (size_t i = 0; i < 48; i++) {
            size_t idx = day * 48 + i;
            if (i == 0)
                model->runLifetimeModels(idx, false, 90, 90, T);
            else if (i == 1)
                model->runLifetimeModels(idx, true, 90, 10, T);
            else if (i < 47)
                model->runLifetimeModels(idx, false, 10, 10, T);
            else
                model->runLifetimeModels(idx, true, 10, 90, T);
        }
        day += 2;
    }

    auto state = model->get_state();

    EXPECT_EQ(state.n_cycles, 43);
    EXPECT_NEAR(state.lmo_lto->dq_relative_cal, 0.27, 1e-3);
    EXPECT_NEAR(state.lmo_lto->dq_relative_cyc, 0.0637, 1e-3);
    EXPECT_NEAR(state.q_relative, 99.666, 1e-3);
    EXPECT_NEAR(state.day_age_of_battery, 88, 1e-3);
}

/** Test focusing on how different time steps affect the integration of a day's degradation in the LMO/LTO life model.
 * The integration of degradation is done at the end of each day when the elapsed time, `cum_dt` is exactly 1.
 * Check that if a simulation step has a timestep large enough that `cum_dt` passes from <1 to >1, that the effects
 * on lifetime are the same by breaking that timestep up and accruing the degradation and `cum_dt` correctly
 **/
TEST_F(lib_battery_lifetime_lmolto_test, IrregularTimeStep) {
    double T = 35.15;
    auto state = model->get_state();

    auto b_params = std::make_shared<lifetime_params>(model->get_params());
    b_params->dt_hr = 0.5;
    auto b_state = std::make_shared<lifetime_state>(model->get_state());
    auto subhourly_model = std::unique_ptr<lifetime_lmolto_t>(new lifetime_lmolto_t(b_params, b_state));

    // run hourly
    size_t day = 0;
    while (day < 870) {
        size_t idx = 0;
        while (idx < 48) {
            size_t i = idx % 24;
            if (i == 0)
                model->runLifetimeModels(idx, false, 50, 70, T);
            else if (i == 1)
                model->runLifetimeModels(idx, true, 70, 30, T);
            else if (i == 2)
                model->runLifetimeModels(idx, true, 30, 50, T);
            else
                model->runLifetimeModels(idx, false, 50, 50, T);
            idx += 1;
        }
        day += 2;
    }
    state = model->get_state();

    EXPECT_EQ(state.n_cycles, 869);
    EXPECT_NEAR(state.lmo_lto->dq_relative_cal, 2.408, 1e-3);
    EXPECT_NEAR(state.lmo_lto->dq_relative_cyc, 0.141, 1e-3);
    EXPECT_NEAR(state.day_age_of_battery, 870, 1e-3);

    // Now compare with: run 30min timesteps for 23.5 hours then hourly for 24, then 1 0.5 hr time idx
    day = 0;
    while (day < 870) {
        size_t idx = 0;
        while (idx < (size_t) (23.5 * 2)) {
            size_t i = idx;
            if (i == 0)
                subhourly_model->runLifetimeModels(idx, false, 50, 70, T);
            else if (i == 1)
                subhourly_model->runLifetimeModels(idx, true, 70, 30, T);
            else if (i == 2)
                subhourly_model->runLifetimeModels(idx, true, 30, 50, T);
            else
                subhourly_model->runLifetimeModels(idx, false, 50, 50, T);
            idx += 1;
        }

        b_params->dt_hr = 1;
        b_state = std::make_shared<lifetime_state>(subhourly_model->get_state());
        subhourly_model = std::unique_ptr<lifetime_lmolto_t>(new lifetime_lmolto_t(b_params, b_state));
        idx = 0;
        while (idx < 24) {
            size_t i = idx % 24;
            if (i == 0)
                subhourly_model->runLifetimeModels(idx, false, 50, 70, T);
            else if (i == 1)
                subhourly_model->runLifetimeModels(idx, true, 70, 30, T);
            else if (i == 2)
                subhourly_model->runLifetimeModels(idx, true, 30, 50, T);
            else
                subhourly_model->runLifetimeModels(idx, false, 50, 50, T);
            idx += 1;
        }
        b_params->dt_hr = 0.5;
        b_state = std::make_shared<lifetime_state>(subhourly_model->get_state());
        subhourly_model = std::unique_ptr<lifetime_lmolto_t>(new lifetime_lmolto_t(b_params, b_state));
        subhourly_model->runLifetimeModels(idx, false, 50, 50, T);
        day += 2;
    }
    state = subhourly_model->get_state();

    EXPECT_EQ(state.n_cycles, 869);
    EXPECT_NEAR(state.lmo_lto->dq_relative_cal, 2.408, 1e-3);
    EXPECT_NEAR(state.lmo_lto->dq_relative_cyc, 0.145, 1e-3);
    EXPECT_NEAR(state.day_age_of_battery, 870, 1e-3);
}

TEST_F(lib_battery_lifetime_lmolto_test, TestAgainstINLData) {
    const char * SSCDIR = std::getenv("SSCDIR");

    std::string validation_path = std::string(SSCDIR) + "/test/input_cases/battery_lmolto_life/";

    std::vector<int> cells_to_test = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
//    cells_to_test = {4};

    for (auto cell : cells_to_test) {
        // Get Cell input data
        std::string file_path = validation_path + "lifetime_validation_cell_" + std::to_string(cell) + ".json";
        std::ifstream file(file_path);


        rapidjson::Document root;
        rapidjson::IStreamWrapper iswc(file);
        root.ParseStream(iswc);


        std::vector<double> rpt_cycles;
        for (const auto& i : root["rpt_EFCs_cum"].GetArray())
            rpt_cycles.push_back(i.GetDouble());

        std::vector<int> days_to_test;
        for (const auto& i : root["rpt_days_cum"].GetArray())
            days_to_test.push_back((int)round(i.GetDouble()));

        std::vector<double> full_soc_profile;
        for (const auto& i : root["15min_profile"].GetArray())
            full_soc_profile.push_back(i.GetDouble());
        dt_hour = 0.25;

        double cell_temp_K = root["temp"].GetDouble();

        // Run Life model with the profile, which starts with charging and ends with discharging
        model = std::unique_ptr<lifetime_lmolto_t>(new lifetime_lmolto_t(dt_hour));

        // record capacity and cycles elapsed at RPT days, days_to_test
        std::vector<double> life_model_caps;
        std::vector<double> life_model_dqCal;
        std::vector<double> life_model_dqCyc;
        std::vector<double> EFCs;

        // charging = -1; discharging = 1
        int charge_mode = -1;
        int prev_charge_mode = 1;

        for (int i = 0; i < full_soc_profile.size(); i++) {
            int j = i + 1;
            if (j > full_soc_profile.size() - 1)
                j = 0;
            double prev_SOC = full_soc_profile[i];
            double SOC = full_soc_profile[j];

            if (SOC - prev_SOC > 1e-5)
                charge_mode = -1;
            else if (prev_SOC - SOC > 1e-5)
                charge_mode = 1;

            bool charge_changed = false;

            if (charge_mode != prev_charge_mode) {
                charge_changed = true;
            }
//            printf("%d, ", i);
            model->runLifetimeModels(i, charge_changed, (1. - prev_SOC) * 100, (1. - SOC) * 100., cell_temp_K - 273.15);
            prev_charge_mode = charge_mode;

            if (model->day_age_of_battery() - days_to_test[0] > -1e-7) {
                auto s = model->get_state();
                life_model_caps.push_back(s.q_relative * 0.01);
                life_model_dqCal.push_back(s.lmo_lto->dq_relative_cal * 0.01);
                life_model_dqCyc.push_back(s.lmo_lto->dq_relative_cyc * 0.01);
                EFCs.push_back(s.lmo_lto->EFC);
                days_to_test.erase(days_to_test.begin());
            }
            if (days_to_test.empty())
                break;
        }

        // Get Expected Cycle Count & Model Prediction
        std::vector<double> sam_cap_rel;
        for (const auto& i : root["sam_cap_rel"].GetArray())
            sam_cap_rel.push_back(i.GetDouble());

        std::vector<double> sam_cap_qcyc;
        for (const auto& i : root["sam_cap_qcyc"].GetArray())
            sam_cap_qcyc.push_back(i.GetDouble());

        std::vector<double> sam_cap_qcal;
        for (const auto& i : root["sam_cap_qcal"].GetArray())
            sam_cap_qcal.push_back(i.GetDouble());

        for (size_t n = 0; n < life_model_caps.size(); n++) {
            EXPECT_NEAR(sam_cap_rel[n], life_model_caps[n], 1e-3) << "cell" << cell;
            EXPECT_NEAR(sam_cap_qcyc[n], life_model_dqCyc[n], 1e-3) << "cell" << cell;
            EXPECT_NEAR(sam_cap_qcal[n], life_model_dqCal[n], 1e-3) << "cell" << cell;
        }
    }
}

TEST_F(lib_battery_lifetime_lmolto_test, replaceBatteryTest) {
    double tol = 0.01;
    double T = 25.15;
    size_t day = 0;
    while (day < 2000) {
        for (size_t i = 0; i < 24; i++) {
            size_t idx = day * 24 + i;
            if (i == 0)
                model->runLifetimeModels(idx, false, 50, 90, T);
            else if (i == 1)
                model->runLifetimeModels(idx, true, 90, 10, T);
            else if (i == 3)
                model->runLifetimeModels(idx, true, 10, 50, T);
            else
                model->runLifetimeModels(idx, false, 50, 50, T);
        }
        day ++;
    }

    auto s = model->get_state();

    EXPECT_NEAR(s.lmo_lto->dq_relative_cal, 1.878, tol);
    EXPECT_NEAR(s.lmo_lto->dq_relative_cyc, 0.521, tol);

    model->replaceBattery(1);
    s = model->get_state();
    EXPECT_NEAR(s.lmo_lto->dq_relative_cal, 0.878, tol);
    EXPECT_NEAR(s.lmo_lto->dq_relative_cyc, 0, tol);
    EXPECT_NEAR(s.q_relative, 99.122, tol);
    EXPECT_NEAR(s.cycle->rainflow_Xlt, 0, tol);
    EXPECT_NEAR(s.cycle->rainflow_Ylt, 0, tol);
    EXPECT_NEAR(s.cycle->rainflow_jlt, 0, tol);
    EXPECT_NEAR(s.cycle_range, 80, tol);
    EXPECT_NEAR(s.average_range, 80, tol);
    EXPECT_NEAR(s.n_cycles, 1999, tol);

    model->replaceBattery(100);
    s = model->get_state();
    EXPECT_NEAR(s.lmo_lto->dq_relative_cal, 0, tol);
    EXPECT_NEAR(s.lmo_lto->dq_relative_cyc, 0, tol);
    EXPECT_NEAR(s.q_relative, 100, tol);
    EXPECT_NEAR(s.cycle->rainflow_Xlt, 0, tol);
    EXPECT_NEAR(s.cycle->rainflow_Ylt, 0, tol);
    EXPECT_NEAR(s.cycle->rainflow_jlt, 0, tol);
    EXPECT_NEAR(s.cycle_range, 0, tol);
    EXPECT_NEAR(s.average_range, 0, tol);
    EXPECT_NEAR(s.n_cycles, 0, tol);
}
