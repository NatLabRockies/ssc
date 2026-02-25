#include <gtest/gtest.h>

extern "C" {
#include "CoolPropLib.h"
}

TEST(CoolProp, SmokeTest)
{
    double h = PropsSI("H", "T", 25+273, "P", 300000, "IsoButane");
    h = PropsSI("H", "T", 251, "P", 101325, "Water");
    h = PropsSI("H", "T", 302, "P", 101325, "Water");
    h = PropsSI("H", "T", 303, "P", 101325, "Water");
    h = PropsSI("H", "T", 301, "P", 101325, "Water");
    h = PropsSI("H", "T", 305, "P", 101325, "Water");
    h = PropsSI("H", "T", 306, "P", 101325, "Water");
    h = PropsSI("H", "T", 300, "P", 101325, "Water");
    EXPECT_NEAR(h, 112654.89965, 0.001);
}
