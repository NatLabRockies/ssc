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
#include <lib_shared_inverter.h>

/**
* Shared Inverter Class test
*/

class sharedInverterTest_lib_shared_inverter : public ::testing::Test {
protected:
    SharedInverter* inv;
    sandia_inverter_t sandia;
    double pDC = 61130.8;
    double ratio = 0.;
    double loss = 0.;
    double e = 0.01;
public:
    void reset() {
        pDC = 61130.8;
        ratio = 1.;
        loss = 0.;
    }
    void SetUp() {
        sandia.C0 = -2.06147e-07;
        sandia.C1 = 2.7e-05;
        sandia.C2 = 0.002606;
        sandia.C3 = 0.000501;
        sandia.Paco = 59860;
        sandia.Pdco = 61130.8;
        sandia.Vdco = 630;
        sandia.Pso = 97.21398;
        sandia.Pntare = 17.958;
        inv = new SharedInverter(0, 1, &sandia, nullptr, nullptr);
    }
    void TearDown() {
        if (inv) delete inv;
    }
};


TEST_F(sharedInverterTest_lib_shared_inverter, tempDerateTest_lib_shared_inverter) {
    /// First set of curves
    std::vector<double> c1 = { 200., 20., -0.2, 40., -0.4 };
    std::vector<double> c2 = { 300., 30., -0.3, 60., -0.6 };
    std::vector<std::vector<double>> curves = { c1, c2 };
    EXPECT_FALSE(inv->setTempDerateCurves(curves)) << "set up temp derate set 1";

    // zero efficiency error case
    double V = 200.;
    double T = 5.;
    inv->calculateTempDerate(V, T, pDC, ratio, loss);
    EXPECT_NEAR(pDC, 61130.8, e) << "zero efficiency error case";

    // zero power error case
    pDC = 0.;
    ratio = 1.;
    inv->calculateTempDerate(V, T, pDC, ratio, loss);
    EXPECT_NEAR(pDC, 0, e) << "zero power error case";

    // no derate cases
    pDC = 61130.8;
    ratio = 1.;
    inv->calculateTempDerate(V, T, pDC, ratio, loss);
    EXPECT_NEAR(pDC, 61130.8, e) << "no derate";

    // V less than that of first curve, extrapolated startT is 10. and slope is -0.1
    V = 100.;
    T = 11.;
    inv->calculateTempDerate(V, T, pDC, ratio, loss);
    EXPECT_NEAR(pDC, 55017.72, e) << "case 1";

    // V more than that of first curve, interpolated startT is 25. and slope is -0.25
    V = 250.;
    T = 26.;
    reset();
    inv->calculateTempDerate(V, T, pDC, ratio, loss);
    EXPECT_NEAR(pDC, 45848.1, e) << "case 2";

    // V more than that of second curve, interpolated startT is 40. and slope is -0.4
    V = 400.;
    T = 41.;
    reset();
    inv->calculateTempDerate(V, T, pDC, ratio, loss);
    EXPECT_NEAR(pDC, 36678.48, e) << "case 3";

    /// Second set of curves, different # of pairs
    std::vector<double> c3 = { 200., 20., -0.2 };
    std::vector<double> c4 = { 300., 30., -0.3, 60., -0.6 };
    std::vector<std::vector<double>> curves2 = { c3, c4 };
    EXPECT_FALSE(inv->setTempDerateCurves(curves2)) << "set up temp derate set 2";

    // V<200 and T < 10: extrapolated startT is 10. and slope is -0.1
    V = 100.;
    T = 9.;
    reset();
    inv->calculateTempDerate(V, T, pDC, ratio, loss);
    EXPECT_NEAR(pDC, 61130.8, e) << "case 7";

    // V<200 and 10 < T: extrapolated startT is 10. and slope is -0.1
    V = 100.;
    T = 11.;
    reset();
    inv->calculateTempDerate(V, T, pDC, ratio, loss);
    EXPECT_NEAR(pDC, 55017.72, e) << "case 7";

    // 200<V<300 and T < 25, interpolated startT is 25. and slope is -0.25
    V = 250.;
    T = 24.;
    reset();
    inv->calculateTempDerate(V, T, pDC, ratio, loss);
    EXPECT_NEAR(pDC, 61130.8, e) << "case 9";

    // 200<V<300 and 25 < T < 40, interpolated startT is 25. and slope is -0.25
    V = 250.;
    T = 26.;
    reset();
    inv->calculateTempDerate(V, T, pDC, ratio, loss);
    EXPECT_NEAR(pDC, 45848.1, e) << "case 9";

    // 200<V<300 and 40 < T, interpolated startT is 40. and slope is -0.4
    V = 250.;
    T = 41.;
    reset();
    inv->calculateTempDerate(V, T, pDC, ratio, loss);
    EXPECT_NEAR(pDC, 36678.48, e) << "case 9";

    // 300<V and T < 40, extrapolated startT is 40. and slope is -0.4
    V = 400.;
    T = 9.;
    reset();
    inv->calculateTempDerate(V, T, pDC, ratio, loss);
    EXPECT_NEAR(pDC, 61130.8, e) << "case 9";

    // 300<V and 40 < T < 100, interpolated startT is 40. and slope is -0.4
    V = 400.;
    T = 41.;
    reset();
    inv->calculateTempDerate(V, T, pDC, ratio, loss);
    EXPECT_NEAR(pDC, 36678.48, e) << "case 9";

    /// Third set of curves, different # of pairs
    std::vector<double> c5 = { 200., 20., -0.2, 60, -0.6 };
    std::vector<double> c6 = { 300., 30., -0.3 };
    std::vector<std::vector<double>> curves3 = { c5, c6 };
    EXPECT_FALSE(inv->setTempDerateCurves(curves3)) << "set up temp derate set 3";

    // V<200 and T < 10: extrapolated startT is 10. and slope is -0.1
    V = 100.;
    T = 9.;
    reset();
    inv->calculateTempDerate(V, T, pDC, ratio, loss);
    EXPECT_NEAR(pDC, 61130.8, e) << "case 7";

    // V<200 and 10 < T : extrapolated startT is 10. and slope is -0.1
    V = 100.;
    T = 11.;
    reset();
    inv->calculateTempDerate(V, T, pDC, ratio, loss);
    EXPECT_NEAR(pDC, 55017.72, e) << "case 7";

    // 200<V<300 and T < 25, interpolated startT is 25. and slope is -0.25
    V = 250.;
    T = 24.;
    reset();
    inv->calculateTempDerate(V, T, pDC, ratio, loss);
    EXPECT_NEAR(pDC, 61130.8, e) << "case 9";

    // 200<V<300 and 25 < T < 40, interpolated startT is 25. and slope is -0.25
    V = 250.;
    T = 26.;
    reset();
    inv->calculateTempDerate(V, T, pDC, ratio, loss);
    EXPECT_NEAR(pDC, 45848.1, e) << "case 9";

    // 200<V<300 and 40 < T, interpolated startT is 45. and slope is -0.45
    V = 250.;
    T = 46.;
    reset();
    inv->calculateTempDerate(V, T, pDC, ratio, loss);
    EXPECT_NEAR(pDC, 33621.94, e) << "case 9";

    // 300<V and T < 40, extrapolated startT is 40. and slope is -0.4
    V = 400.;
    T = 9.;
    reset();
    inv->calculateTempDerate(V, T, pDC, ratio, loss);
    EXPECT_NEAR(pDC, 61130.8, e) << "case 9";

    // 300<V and 40 < T, interpolated startT is 40. and slope is -0.4
    V = 400.;
    T = 41.;
    reset();
    inv->calculateTempDerate(V, T, pDC, ratio, loss);
    EXPECT_NEAR(pDC, 36678.48, e) << "case 9";
}

TEST_F(sharedInverterTest_lib_shared_inverter, calculateEffForACPower) {
    inv->calculateACPower(sandia.Pdco / 1000., sandia.Vdco, 25);

    // really small ac output
    double p_kwac = .05;
    double p_kwdc = inv->calculateRequiredDCPower(p_kwac, sandia.Vdco, 25);
    inv->calculateACPower(p_kwdc, sandia.Vdco, 25);
    EXPECT_LT(inv->powerAC_kW, p_kwac) << "inverter efficiency too low to produce required ac";

    // range of ac values
    for (auto p : { 0.05, 0.95, 1. }) {
        p_kwac = sandia.Paco * p / 1000.;
        p_kwdc = inv->calculateRequiredDCPower(p_kwac, sandia.Vdco, 25);
        inv->calculateACPower(p_kwdc, sandia.Vdco, 25);
        EXPECT_NEAR(inv->powerAC_kW, p_kwac, 1e-3) << "inverter should produce required ac of Paco * " << p;
        // test negative ac values
        p_kwac *= -1;
        p_kwdc = inv->calculateRequiredDCPower(p_kwac, sandia.Vdco, 25);
        inv->calculateACPower(p_kwdc, sandia.Vdco, 25);
        EXPECT_NEAR(inv->powerAC_kW, p_kwac, 1e-3) << "inverter should produce required (negative) ac of Paco * " << p;
    }

    // over max ac values
    for (auto p : { 1.05, 1.1 }) {
        p_kwac = sandia.Paco * p / 1000.;
        p_kwdc = inv->calculateRequiredDCPower(p_kwac, sandia.Vdco, 25);
        inv->calculateACPower(p_kwdc, sandia.Vdco, 25);
        EXPECT_NEAR(inv->powerAC_kW, sandia.Paco / 1000., 1e-3) << "inverter cannot produce more than max Paco";
        // test negative ac values
        p_kwac *= -1;
        p_kwdc = inv->calculateRequiredDCPower(p_kwac, sandia.Vdco, 25);
        inv->calculateACPower(p_kwdc, sandia.Vdco, 25);
        EXPECT_NEAR(inv->powerAC_kW, -sandia.Paco / 1000., 1e-3) << "inverter cannot produce more than max (negative) Paco";
    }
}

// Part load inverter efficiency should be AC / DC when clipping (SAM issue 2272)
TEST(partloadInverterTest_lib_shared_inverter, clippingEfficiency) {
    partload_inverter_t partload;
    partload.Paco = 4000;
    partload.Pdco = 4210.53;
    partload.Vdco = 310;
    partload.Pntare = 0;
    partload.Partload = { 0, 10, 50, 100 };
    partload.Efficiency = { 0, 90, 95, 96 };
    SharedInverter inv(SharedInverter::PARTLOAD_INVERTER, 1, nullptr, &partload, nullptr);
    double e = 1e-6;

    // no clipping, 30% load is halfway between 90% and 95% on the curve
    inv.calculateACPower(0.3 * partload.Pdco / 1000., partload.Vdco, 25);
    EXPECT_NEAR(inv.efficiencyAC, 92.5, e) << "efficiency from part load curve";
    EXPECT_NEAR(inv.powerClipLoss_kW, 0, e) << "no clipping below Paco";
    EXPECT_NEAR(inv.powerAC_kW, inv.powerDC_kW * inv.efficiencyAC * 0.01, e) << "efficiency is ac / dc without clipping";

    // DC/AC ratio of 2, clipped to Paco
    double p_dc_kw = 2 * partload.Paco / 1000.;
    inv.calculateACPower(p_dc_kw, partload.Vdco, 25);
    EXPECT_NEAR(inv.powerAC_kW, partload.Paco / 1000., e) << "output clipped to Paco";
    EXPECT_GT(inv.powerClipLoss_kW, 0) << "clipping loss reported";
    EXPECT_NEAR(inv.efficiencyAC, 50., e) << "efficiency includes clipping";
    EXPECT_NEAR(inv.powerAC_kW, inv.powerDC_kW * inv.efficiencyAC * 0.01, e) << "efficiency is ac / dc with clipping";

    // two MPPT inputs
    inv.calculateACPower(std::vector<double>{ p_dc_kw / 2, p_dc_kw / 2 }, std::vector<double>{ partload.Vdco, partload.Vdco }, 25);
    EXPECT_NEAR(inv.powerAC_kW, partload.Paco / 1000., e) << "multiple MPPT output clipped to Paco";
    EXPECT_NEAR(inv.efficiencyAC, 50., e) << "multiple MPPT efficiency includes clipping";
}
