#include <gtest/gtest.h>

#include "fluid_properties.h"

extern "C" {
#include "CoolPropLib.h"
}

TEST(CoolProp, SmokeTest)
{
    double h = PropsSI("H", "T", 25+273, "P", 300000, "IsoButane");
    EXPECT_NEAR(h, 589857.11183, 0.001);
}

static bool compare_vals(const double val_1, const double val_2, const double tol)
{
    double err = (val_1 - val_2) / (val_2);
    if (std::abs(err) < tol)
        return true;
    else
        return false;
}

static void compare_fluid_states(const fluid_state& fs_1, const fluid_state& fs_2)
{
    double tol = 1e-3;

    EXPECT_TRUE(compare_vals(fs_1.temp, fs_2.temp, tol));
    EXPECT_TRUE(compare_vals(fs_1.pres, fs_2.pres, tol));
    EXPECT_TRUE(compare_vals(fs_1.dens, fs_2.dens, tol));
    EXPECT_TRUE(compare_vals(fs_1.inte, fs_2.inte, tol));
    EXPECT_TRUE(compare_vals(fs_1.enth, fs_2.enth, tol));
    EXPECT_TRUE(compare_vals(fs_1.entr, fs_2.entr, tol));

    // Specific properties for within the dome
    if ((fs_1.qual > 0 && fs_1.qual < 1)
        || (fs_2.qual > 0 && fs_2.qual < 1))
    {
        EXPECT_TRUE(compare_vals(fs_1.qual, fs_2.qual, tol));
        EXPECT_TRUE(compare_vals(fs_1.sat_vap_dens, fs_2.sat_vap_dens, tol));
        EXPECT_TRUE(compare_vals(fs_1.sat_liq_dens, fs_2.sat_liq_dens, tol));
    }
    else
    {
        EXPECT_TRUE(compare_vals(fs_1.cv, fs_2.cv, tol));
        EXPECT_TRUE(compare_vals(fs_1.cp, fs_2.cp, tol));
        EXPECT_TRUE(compare_vals(fs_1.ssnd, fs_2.ssnd, tol));
    }  
}

TEST(CoolProp, CO2Properties)
{
    auto co2_ptr = C_fluid_properties::create_fluid_properties(E_fluid_type::CO2);
    auto coolprop_ptr = C_fluid_properties::create_fluid_properties(E_fluid_type::COOLPROP, "CarbonDioxide");

    double T1 = 700 + 273.15;   // [K]
    double P1 = 25e3;           // [kPa]

    // Get fluid info
    fluid_info info_co2, info_coolprop;
    co2_ptr->get_info(&info_co2);
    coolprop_ptr->get_info(&info_coolprop);

    fluid_state fs_co2, fs_coolprop;
    int err = co2_ptr->TP(T1, P1, &fs_co2);
    ASSERT_EQ(err, 0);
    err = coolprop_ptr->TP(T1, P1, &fs_coolprop);
    ASSERT_EQ(err, 0);

    // Compare fluid states
    compare_fluid_states(fs_co2, fs_coolprop);
}

TEST(CoolProp, WaterProperties)
{
    auto water_ptr = C_fluid_properties::create_fluid_properties(E_fluid_type::WATER);
    auto coolprop_ptr = C_fluid_properties::create_fluid_properties(E_fluid_type::COOLPROP, "Water");

    // Choose a T,P in the dome for water
    double T1 = 373.15;    // [K]
    double Q1 = 0.5;   // [kPa]

    // Get fluid info
    fluid_info info_water, info_coolprop;
    water_ptr->get_info(&info_water);
    coolprop_ptr->get_info(&info_coolprop);

    fluid_state fs_water, fs_coolprop;
    int err = water_ptr->TQ(T1, Q1, &fs_water);
    ASSERT_EQ(err, 0);
    err = coolprop_ptr->TQ(T1, Q1, &fs_coolprop);
    ASSERT_EQ(err, 0);

    // Compare fluid states
    compare_fluid_states(fs_water, fs_coolprop);
}
