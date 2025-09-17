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


#include <chrono>

#include "cmod_battery_eqns_test.h"

#include "cmod_battery_eqns.h"
#include "pvsamv1_common_data.h"

TEST_F(CMBatteryEqns_cmod_battery_eqns, TestStatefulSizeModifications) {
    CreateModel(1.);

    ssc_data_set_number(data, "desired_voltage", 600.0); // Increase voltage from 500 V
    ssc_data_set_number(data, "desired_capacity", 20.0); // Increase capacity from 10 kWh

    Size_batterystateful(data);

    EXPECT_TRUE(ssc_stateful_module_setup(mod, data));
    EXPECT_TRUE(ssc_module_exec(mod, data));

    ssc_number_t new_voltage, new_capacity, new_mass, new_surface_area;
    ssc_data_get_number(data, "nominal_voltage", &new_voltage);
    ssc_data_get_number(data, "nominal_energy", &new_capacity);
    ssc_data_get_number(data, "mass", &new_mass);
    ssc_data_get_number(data, "surface_area", &new_surface_area);

    EXPECT_NEAR(600.0, new_voltage, m_error_tolerance_lo);
    EXPECT_NEAR(20.0, new_capacity, m_error_tolerance_lo);
    EXPECT_NEAR(1014.0, new_mass, m_error_tolerance_lo);
    EXPECT_NEAR(3.2033, new_surface_area, m_error_tolerance_lo);
}

TEST_F(CMBatteryEqns_cmod_battery_eqns, reopt_sizing) {
    data = ssc_data_create();
    battery_data_default(data);
    utility_rate5_default(data);
    cashloan_default(data);

    ssc_number_t gen[8760] = { 0 };
    ssc_data_set_array(data, "gen", gen, 8760);
    ssc_data_set_number(data, "analysis_period", 2); 

    ssc_data_set_number(data, "system_capacity", 5);
    set_array(data, "load", load_profile_path, 8760); // Load is required for peak shaving controllers


    Reopt_size_standalone_battery_params(data);

    auto vd = static_cast<var_table*>(data);
    ASSERT_TRUE(vd->is_assigned("reopt_scenario"));
    auto site = vd->lookup("reopt_scenario");

    auto settings = site->table.lookup("Settings");
    assert(settings->table.lookup("time_steps_per_hour")->num == 1);

    std::vector<std::string> sections = { "ElectricTariff", "ElectricLoad", "Financial", "ElectricStorage"};
    for (const auto& s : sections)
        ASSERT_TRUE(site->table.is_assigned(s));
}
