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


#include <algorithm>
#include <functional>

#include "6par_newton.h"
#include "lib_shared_inverter.h"
#include "lib_util.h"

SharedInverter::SharedInverter(int inverterType, size_t numberOfInverters,
    sandia_inverter_t* sandiaInverter, partload_inverter_t* partloadInverter, ond_inverter* ondInverter, double numberOfInvertersClipping)
{
    m_inverterType = inverterType;
    m_numInverters = numberOfInverters;
    m_numInvertersClipping = numberOfInvertersClipping;
    m_sandiaInverter = sandiaInverter;
    m_partloadInverter = partloadInverter;
    m_ondInverter = ondInverter;
    m_tempEnabled = false;
    m_subhourlyClippingEnabled = false;

    if (m_inverterType == SANDIA_INVERTER || m_inverterType == DATASHEET_INVERTER || m_inverterType == COEFFICIENT_GENERATOR)
        m_nameplateAC_kW = m_numInverters * m_sandiaInverter->Paco * util::watt_to_kilowatt;
    else if (m_inverterType == PARTLOAD_INVERTER)
        m_nameplateAC_kW = m_numInverters * m_partloadInverter->Paco * util::watt_to_kilowatt;
    else if (m_inverterType == OND_INVERTER)
        m_nameplateAC_kW = m_numInverters * m_ondInverter->PMaxOUT * util::watt_to_kilowatt;

    powerDC_kW = 0.;
    powerAC_kW = 0.;
    powerAC_kW_clipping = 0.;
    efficiencyAC = 96.;
    powerClipLoss_kW = 0.;
    powerConsumptionLoss_kW = 0.;
    powerNightLoss_kW = 0.;
    powerTempLoss_kW = 0.;
    powerLossTotal_kW = 0.;
    dcWiringLoss_ond_kW = 0.;
    acWiringLoss_ond_kW = 0.;
}

SharedInverter::SharedInverter(const SharedInverter& orig) {
    m_inverterType = orig.m_inverterType;
    m_numInverters = orig.m_numInverters;
    m_nameplateAC_kW = orig.m_nameplateAC_kW;
    m_tempEnabled = orig.m_tempEnabled;
    m_thermalDerateCurves = orig.m_thermalDerateCurves;
    m_sandiaInverter = orig.m_sandiaInverter;
    m_partloadInverter = orig.m_partloadInverter;
    m_ondInverter = orig.m_ondInverter;
    efficiencyAC = orig.efficiencyAC;

    m_subhourlyClippingEnabled = orig.m_subhourlyClippingEnabled;
    m_numInvertersClipping = orig.m_numInvertersClipping;

    powerDC_kW = orig.powerDC_kW;
    powerAC_kW = orig.powerAC_kW;
    powerAC_kW_clipping = orig.powerAC_kW_clipping;
    powerClipLoss_kW = orig.powerClipLoss_kW;
    powerConsumptionLoss_kW = orig.powerConsumptionLoss_kW;
    powerNightLoss_kW = orig.powerNightLoss_kW;
    powerTempLoss_kW = orig.powerTempLoss_kW;
    powerLossTotal_kW = orig.powerLossTotal_kW;
    dcWiringLoss_ond_kW = orig.dcWiringLoss_ond_kW;
    acWiringLoss_ond_kW = orig.acWiringLoss_ond_kW;
}

bool sortByVoltage(std::vector<double> i, std::vector<double> j)
{
    return (i[0] < j[0]);
}

int SharedInverter::setTempDerateCurves(std::vector<std::vector<double>> derateCurves)
{
    m_thermalDerateCurves.clear();

    // Check derate curves have V > 0, and that for each pair T > -273, slope < 0
    for (size_t r = 0; r < derateCurves.size(); r++) {
        if (derateCurves[r][0] <= 0.) return (int)r + 1;
        size_t tempSlopeEntries = derateCurves[r].size() - 1;
        if ((tempSlopeEntries % 2) != 0) return (int)r + 1;
        for (size_t p = 0; p < tempSlopeEntries / 2; p++) {
            if (derateCurves[r][2 * p + 1] <= -273. || derateCurves[r][2 * p + 2] > 0.) return (int)r + 1;
        }
        m_thermalDerateCurves.push_back(derateCurves[r]);
    }

    // Sort by DC voltage
    std::sort(m_thermalDerateCurves.begin(), m_thermalDerateCurves.end(), sortByVoltage);

    if (!m_thermalDerateCurves.empty())
        m_tempEnabled = true;
    return 0;
}

std::vector<std::vector<double>> SharedInverter::getTempDerateCurves() {
    return m_thermalDerateCurves;
}

void SharedInverter::findPointOnCurve(size_t idx, double T, double& startT, double& slope) {
    size_t p = 0;
    while (2 * p + 2 < m_thermalDerateCurves[idx].size() && T >= m_thermalDerateCurves[idx][2 * p + 1]) {
        p++;
    }
    if (2 * p + 2 >= m_thermalDerateCurves[idx].size()) {
        p--;
    }
    startT = m_thermalDerateCurves[idx][2 * p + 1];
    slope = m_thermalDerateCurves[idx][2 * p + 2];
}

void SharedInverter::calculateTempDerate(double V, double tempC, double& p_dc_rated, double& ratio, double& loss)
{
    if (ratio == 0. || p_dc_rated == 0.) return;

    double slope = 0.0;
    double startT = 0.0;
    double Vdc = 0.0;
    double slope2 = 0.0;
    double startT2 = 0.0;
    double Vdc2 = 0.0;

    double p_dc_max = getInverterDCMaxPower(p_dc_rated);

    // Find the appropriate derate curve depending on DC voltage
    size_t idx = 0;
    double deltaT = 0.0;
    double slopeInterpolated = 0.0;
    double startTInterpolated = 0.0;

    while (idx < m_thermalDerateCurves.size() && V > m_thermalDerateCurves[idx][0]) {
        idx++;
    }
    if (m_thermalDerateCurves.size() == 1) {
        Vdc2 = m_thermalDerateCurves[0][0];
        startTInterpolated = m_thermalDerateCurves[0][1];
        slopeInterpolated = m_thermalDerateCurves[0][2];
    }
    // Use temp and slope of lower and upper curves for interpolation if they both exist
    else if (idx > 0 && idx < m_thermalDerateCurves.size()) {
        Vdc2 = m_thermalDerateCurves[idx][0];
        Vdc = m_thermalDerateCurves[idx - 1][0];
        double startTGuess = 0.0;
        double slopeGuess = 0.0;
        size_t n = std::max(m_thermalDerateCurves[idx].size() / 2, m_thermalDerateCurves[idx - 1].size() / 2);
        size_t count = 0;
        while (tempC > startTGuess && count < n) {
            findPointOnCurve(idx, startT2, startT2, slope2);
            findPointOnCurve(idx - 1, startT, startT, slope);
            startTGuess = (startT2 - startT) / (Vdc2 - Vdc) * (V - Vdc2) + startT2;
            slopeGuess = (slope2 - slope) / (Vdc2 - Vdc) * (V - Vdc2) + slope2;
            if (tempC > startTGuess) {
                startTInterpolated = startTGuess;
                slopeInterpolated = slopeGuess;
                count++;
            }
        }
    }
    // otherwise extrapolate using first start temps of each curve in order to avoid inconsistent start temps
    else {
        if (idx == 0) {
            Vdc2 = m_thermalDerateCurves[idx][0];
            findPointOnCurve(idx, -273, startT2, slope2);
            Vdc = m_thermalDerateCurves[idx + 1][0];
            findPointOnCurve(idx + 1, -273, startT, slope);
            startTInterpolated = (startT2 - startT) / (Vdc2 - Vdc) * (V - Vdc2) + startT2;
            slopeInterpolated = (slope2 - slope) / (Vdc2 - Vdc) * (V - Vdc2) + slope2;
        }
        else {
            Vdc2 = m_thermalDerateCurves[idx - 1][0];
            findPointOnCurve(idx - 1, -273, startT2, slope2);
            Vdc = m_thermalDerateCurves[idx - 2][0];
            findPointOnCurve(idx - 2, -273, startT, slope);
            startTInterpolated = (startT2 - startT) / (Vdc2 - Vdc) * (V - Vdc2) + startT2;
            slopeInterpolated = (slope2 - slope) / (Vdc2 - Vdc) * (V - Vdc2) + slope2;
        }
    }
    deltaT = tempC - startTInterpolated;

    // If less than start temp, no derating
    if (deltaT <= 0) return;

    // If slope is positive, set to zero with no derating
    if (slopeInterpolated >= 0) return;
    if (slopeInterpolated < -1) slopeInterpolated = -1;

    // Power in units of W, ratio = max output / rated output
    ratio += deltaT * slopeInterpolated;
    if (ratio < 0) ratio = 0.;
    double p_dc_limit = p_dc_max * ratio;
    if (p_dc_rated > p_dc_limit) {
        loss = p_dc_rated - (p_dc_limit);
        p_dc_rated = p_dc_limit;
    }
    else {
        loss = 0;
    }
}

double SharedInverter::getInverterDCMaxPower(double p_dc_rated)
{
    double inv_dc_max_power = p_dc_rated * util::kilowatt_to_watt; //if the inverter type isn't one of the following, assume that max power is equal to rated power"
    if (m_inverterType == SANDIA_INVERTER || m_inverterType == DATASHEET_INVERTER || m_inverterType == COEFFICIENT_GENERATOR)
        //m_sandiaInverter->acpower(std::fabs(powerDC_Watts) / m_numInverters, DCStringVoltage, &powerAC_Watts, &P_par, &P_lr, &efficiencyAC, &powerClipLoss_kW, &powerConsumptionLoss_kW, &powerNightLoss_kW);
        inv_dc_max_power = m_sandiaInverter->Pdco;
    else if (m_inverterType == PARTLOAD_INVERTER)
        //m_partloadInverter->acpower(std::fabs(powerDC_Watts) / m_numInverters, &powerAC_Watts, &P_lr, &P_par, &efficiencyAC, &powerClipLoss_kW, &powerNightLoss_kW);
        inv_dc_max_power = m_partloadInverter->Pdco;
    else if (m_inverterType == OND_INVERTER)
        //m_ondInverter->acpower(std::fabs(powerDC_Watts) / m_numInverters, DCStringVoltage, tempC, &powerAC_Watts, &P_par, &P_lr, &efficiencyAC, &powerClipLoss_kW, &powerConsumptionLoss_kW, &powerNightLoss_kW, &dcWiringLoss_ond_kW, &acWiringLoss_ond_kW);
        inv_dc_max_power = m_ondInverter->PMaxDC;

    return inv_dc_max_power;
}

//function that calculates AC power and inverter losses for a single inverter with one MPPT input
void SharedInverter::calculateACPower(const double powerDC_kW_in, const double DCStringVoltage, double tempC)
{
    double P_par, P_lr;
    double P_par_clipping, P_lr_clipping;
    bool negativePower = powerDC_kW_in < 0 ? true : false;


    dcWiringLoss_ond_kW = 0.0;
    acWiringLoss_ond_kW = 0.0;

    // Power quantities go in and come out in units of W
    double powerDC_Watts = powerDC_kW_in * util::kilowatt_to_watt;
    double powerAC_Watts = 0.0;
    double powerAC_Watts_clipping = 0.0;
    Tdry_C = tempC;
    StringV = DCStringVoltage;
    double tempLoss = 0.0;
    double power_ratio = 1.0;
    if (m_tempEnabled) {
        calculateTempDerate(DCStringVoltage, tempC, powerDC_Watts, power_ratio, tempLoss);
    }

    if (m_inverterType == SANDIA_INVERTER || m_inverterType == DATASHEET_INVERTER || m_inverterType == COEFFICIENT_GENERATOR)
        m_sandiaInverter->acpower(std::abs(powerDC_Watts) / m_numInverters, DCStringVoltage, &powerAC_Watts, &P_par, &P_lr, &efficiencyAC, &powerClipLoss_kW, &powerConsumptionLoss_kW, &powerNightLoss_kW);
    else if (m_inverterType == PARTLOAD_INVERTER)
        m_partloadInverter->acpower(std::abs(powerDC_Watts) / m_numInverters, &powerAC_Watts, &P_lr, &P_par, &efficiencyAC, &powerClipLoss_kW, &powerNightLoss_kW);
    else if (m_inverterType == OND_INVERTER)
        m_ondInverter->acpower(std::abs(powerDC_Watts) / m_numInverters, DCStringVoltage, tempC, &powerAC_Watts, &P_par, &P_lr, &efficiencyAC, &powerClipLoss_kW, &powerConsumptionLoss_kW, &powerNightLoss_kW, &dcWiringLoss_ond_kW, &acWiringLoss_ond_kW);
    else if (m_inverterType == NONE) {
        powerClipLoss_kW = 0.;
        powerConsumptionLoss_kW = 0.;
        powerNightLoss_kW = 0.;
        efficiencyAC = NONE_INVERTER_EFF;
        powerAC_Watts = powerDC_Watts * efficiencyAC;
    }

    // Convert units to kW- no need to scale to system size because passed in as power to total number of inverters
    powerDC_kW = powerDC_Watts * util::watt_to_kilowatt;
    convertOutputsToKWandScale(tempLoss, powerAC_Watts);

    // In event shared inverter is charging a battery only, need to re-convert to negative power
    if (negativePower) {
        powerAC_kW = -1.0 * std::abs(powerAC_kW);
    }
}

void SharedInverter::calculateACPower(const double powerDC_kW_in, const double DCStringVoltage, double tempC, bool clippingEnabled)
{
    double P_par, P_lr;
    double P_par_clipping, P_lr_clipping;
    double efficiencyAC_clipping, powerClipLoss_kW_clipping, powerConsumptionLoss_kW_clipping, powerNightLoss_kW_clipping = 0;
    double dcWiringLoss_ond_kW_clipping, acWiringLoss_ond_kW_clipping = 0;
    bool negativePower = powerDC_kW_in < 0 ? true : false;


    dcWiringLoss_ond_kW = 0.0;
    acWiringLoss_ond_kW = 0.0;

    // Power quantities go in and come out in units of W
    double powerDC_Watts = powerDC_kW_in * util::kilowatt_to_watt;
    double powerAC_Watts = 0.0;
    double powerAC_Watts_clipping = 0.0;
    Tdry_C = tempC;
    StringV = DCStringVoltage;
    double tempLoss = 0.0;
    double power_ratio = 1.0;
    if (m_tempEnabled) {
        calculateTempDerate(DCStringVoltage, tempC, powerDC_Watts, power_ratio, tempLoss);
    }

    if (m_inverterType == SANDIA_INVERTER || m_inverterType == DATASHEET_INVERTER || m_inverterType == COEFFICIENT_GENERATOR)
        m_sandiaInverter->acpower(std::abs(powerDC_Watts) / m_numInvertersClipping, DCStringVoltage, &powerAC_Watts_clipping, &P_par_clipping, &P_lr, &efficiencyAC_clipping, &powerClipLoss_kW_clipping, &powerConsumptionLoss_kW_clipping, &powerNightLoss_kW_clipping);
    else if (m_inverterType == PARTLOAD_INVERTER)
        m_partloadInverter->acpower(std::abs(powerDC_Watts) / m_numInvertersClipping, &powerAC_Watts_clipping, &P_lr, &P_par_clipping, &efficiencyAC_clipping, &powerClipLoss_kW_clipping, &powerConsumptionLoss_kW_clipping);
    else if (m_inverterType == OND_INVERTER)
        m_ondInverter->acpower(std::abs(powerDC_Watts) / m_numInvertersClipping, DCStringVoltage, tempC, &powerAC_Watts_clipping, &P_par_clipping, &P_lr, &efficiencyAC_clipping, &powerClipLoss_kW_clipping, &powerConsumptionLoss_kW_clipping, &powerNightLoss_kW_clipping, &dcWiringLoss_ond_kW_clipping, &acWiringLoss_ond_kW_clipping);
    else if (m_inverterType == NONE) {
        powerClipLoss_kW = 0.;
        powerConsumptionLoss_kW = 0.;
        powerNightLoss_kW = 0.;
        efficiencyAC = NONE_INVERTER_EFF;
        powerAC_Watts_clipping = powerDC_Watts * efficiencyAC;
    }
    

    if (clippingEnabled) {
        m_subhourlyClippingEnabled = true;
        powerAC_kW_clipping = powerAC_Watts_clipping * m_numInvertersClipping * util::watt_to_kilowatt;
        return;
    }
}

/* This function takes input inverter DC power (kW) per MPPT input for a SINGLE multi-mppt inverter, DC voltage (V) per input, and ambient temperature (deg C), and calculates output for the total number of inverters in the system */
void SharedInverter::calculateACPower(const std::vector<double> powerDC_kW_in, const std::vector<double> DCStringVoltage, double tempC)
{
    double P_par, P_lr;

    //need to convert to watts and divide power by m_num_inverters
    std::vector<double> powerDC_Watts_one_inv;
    std::vector<double> powerDC_Watts_one_inv_iter;
    for (size_t i = 0; i < powerDC_kW_in.size(); i++) {
        powerDC_Watts_one_inv.push_back(powerDC_kW_in[i] * util::kilowatt_to_watt / m_numInverters);
    }
    Tdry_C = tempC;
    StringV = DCStringVoltage[0];
    int size = DCStringVoltage.size();
    std::vector<double> tempLoss(size, 0);

    double power_ratio = 1.0;
    if (m_tempEnabled) {
        //use average of the DC voltages to pick which temp curve to use- a weighted average might be better but we don't have that information here
        double avgDCVoltage = 0;
        double avgDCPower_Watts = 0;
        for (size_t i = 0; i < powerDC_Watts_one_inv.size(); i++) {
            power_ratio = 1.0;
            calculateTempDerate(DCStringVoltage[i], tempC, powerDC_Watts_one_inv[i], power_ratio, tempLoss[i]);
        }
    }
    // Power quantities go in and come out in units of W
    double powerAC_Watts = 0;
    if (m_inverterType == SANDIA_INVERTER || m_inverterType == DATASHEET_INVERTER || m_inverterType == COEFFICIENT_GENERATOR)
        m_sandiaInverter->acpower(powerDC_Watts_one_inv, DCStringVoltage, &powerAC_Watts, &P_par, &P_lr, &efficiencyAC, &powerClipLoss_kW, &powerConsumptionLoss_kW, &powerNightLoss_kW);
    else if (m_inverterType == PARTLOAD_INVERTER)
        m_partloadInverter->acpower(powerDC_Watts_one_inv, &powerAC_Watts, &P_lr, &P_par, &efficiencyAC, &powerClipLoss_kW, &powerNightLoss_kW);



    // Scale to total system size
    // Do not need to scale back up by m_numInverters because scaling them down was a separate vector, powerDC_Watts_one_inv
    powerDC_kW = 0;
    double tempLoss_avg = 0;
    for (size_t i = 0; i < powerDC_Watts_one_inv.size(); i++) {
        powerDC_kW += powerDC_Watts_one_inv[i] * util::watt_to_kilowatt * m_numInverters;
        tempLoss_avg += tempLoss[i];
    }
    tempLoss_avg /= tempLoss.size();
    //Convert units to kW and scale to total array for all other outputs
    convertOutputsToKWandScale(tempLoss_avg, powerAC_Watts);
}

void SharedInverter::solve_kwdc_for_kwac(const double* x, double* f) {
    calculateACPower(x[0], StringV, Tdry_C);
    f[0] = powerAC_kW - solver_AC;
}

using namespace std::placeholders;
double SharedInverter::calculateRequiredDCPower(const double kwAC, const double DCStringV, double tempC) {

    SharedInverter clone = SharedInverter(*this);

    // set up solver values
    clone.StringV = DCStringV;
    clone.Tdry_C = tempC;
    clone.solver_AC = kwAC;

    std::function<void(const double*, double*)> f;
    f = std::bind(&SharedInverter::solve_kwdc_for_kwac, &clone, _1, _2);

    double x[1], resid[1];
    x[0] = kwAC * 1.04;
    bool check = false;

    newton<double, std::function<void(const double*, double*)>, 1>(x, resid, check, f,
        100, 1e-6, 1e-6, 0.7);

    // if efficiency is too low, the required DC power becomes infinite
    if (!isfinite(x[0]))
        x[0] = kwAC;

    return x[0];
}

double SharedInverter::getInverterDCNominalVoltage()
{
    if (m_inverterType == SANDIA_INVERTER || m_inverterType == DATASHEET_INVERTER || m_inverterType == COEFFICIENT_GENERATOR)
        return m_sandiaInverter->Vdco;
    else if (m_inverterType == PARTLOAD_INVERTER)
        return m_partloadInverter->Vdco;
    else if (m_inverterType == OND_INVERTER)
        return m_ondInverter->VNomEff[1];
    else
        return 0.;
}

void SharedInverter::convertOutputsToKWandScale(double tempLoss, double powerAC_watts)
{
    powerAC_kW = powerAC_watts * m_numInverters * util::watt_to_kilowatt;
    powerClipLoss_kW *= m_numInverters * util::watt_to_kilowatt;
    powerConsumptionLoss_kW *= m_numInverters * util::watt_to_kilowatt;
    powerNightLoss_kW *= m_numInverters * util::watt_to_kilowatt;
    powerTempLoss_kW = tempLoss * m_numInverters * util::watt_to_kilowatt;
    if (powerDC_kW < 0.0)
        powerLossTotal_kW = std::abs(powerDC_kW) - std::abs(powerAC_kW); // DC connected grid charging
    else
        powerLossTotal_kW = powerDC_kW - powerAC_kW; // Normal operation and night time losses
    efficiencyAC *= 100;
    dcWiringLoss_ond_kW *= m_numInverters * util::watt_to_kilowatt;
    acWiringLoss_ond_kW *= m_numInverters * util::watt_to_kilowatt;
}

double SharedInverter::getMaxPowerEfficiency()
{
    if (m_inverterType == SANDIA_INVERTER || m_inverterType == DATASHEET_INVERTER || m_inverterType == COEFFICIENT_GENERATOR)
        calculateACPower(m_sandiaInverter->Paco * util::watt_to_kilowatt * m_numInverters, m_sandiaInverter->Vdco, 0.0);
    else if (m_inverterType == PARTLOAD_INVERTER)
        calculateACPower(m_partloadInverter->Paco * util::watt_to_kilowatt * m_numInverters, m_partloadInverter->Vdco, 0.0);
    else if (m_inverterType == OND_INVERTER)
        calculateACPower(m_ondInverter->PMaxOUT * util::watt_to_kilowatt * m_numInverters, m_ondInverter->VAbsMax, 0.0);

    return efficiencyAC;
}

double SharedInverter::getACNameplateCapacitykW()
{
    return m_nameplateAC_kW;
}

util::matrix_t<double> SharedInverter::SubhourlyClippingMatrix()
{
    const double Subhourly_Clipping_Matrix[21][21] =
    {
        {0, -2, -0.4, -0.3, -0.2, -0.1, 0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1, 1.1, 1.2, 1.3, 1.4},
        {-0.001, 1.65e-08, 2.17e-08, 2.48e-08, 3.09e-08, 4.49e-08, 6.7e-08, 1.06e-07, 1.74e-07, 2.67e-07, 3.97e-07, 5.71e-07, 8.09e-07, 1.11e-06, 1.55e-06, 2.03e-06, 2.38e-06, 2.84e-06, 2.85e-06, 4.86e-06, 3.12e-08},
        {0.05, 6.14e-08, 1.07e-07, 1.39e-07, 2.05e-07, 3.72e-07, 7.5e-07, 1.49e-06, 2.57e-06, 4.02e-06, 5.87e-06, 8.09e-06, 1.06e-05, 1.33e-05, 1.64e-05, 1.91e-05, 2.15e-05, 2.45e-05, 2.65e-05, 2.75e-05, 0},
        {0.1, 7.63e-08, 1.45e-07, 1.89e-07, 2.84e-07, 5.77e-07, 1.28e-06, 2.72e-06, 4.89e-06, 7.72e-06, 1.12e-05, 1.5e-05, 1.9e-05, 2.28e-05, 2.66e-05, 2.96e-05, 3.04e-05, 3.09e-05, 3e-05, 2.9e-05, 0},
        {0.15, 8.58e-08, 1.67e-07, 2.16e-07, 3.22e-07, 6.78e-07, 1.67e-06, 3.81e-06, 7.09e-06, 1.12e-05, 1.59e-05, 2.1e-05, 2.54e-05, 2.92e-05, 3.24e-05, 3.44e-05, 3.31e-05, 3.36e-05, 3.13e-05, 7.77e-05, 0},
        {0.2, 9.56e-08, 1.76e-07, 2.34e-07, 3.51e-07, 7.77e-07, 2.05e-06, 4.87e-06, 9.19e-06, 1.48e-05, 2.07e-05, 2.68e-05, 3.13e-05, 3.43e-05, 3.62e-05, 3.67e-05, 3.39e-05, 2.86e-05, 1.8e-05, 1.22e-05, 0},
        {0.25, 8.82e-08, 1.83e-07, 2.52e-07, 3.9e-07, 7.96e-07, 2.18e-06, 5.35e-06, 1.06e-05, 1.71e-05, 2.41e-05, 3.01e-05, 3.42e-05, 3.66e-05, 3.76e-05, 3.66e-05, 3.36e-05, 2.87e-05, 3.02e-05, 2.5e-06, 0},
        {0.3, 8.79e-08, 1.77e-07, 2.35e-07, 3.56e-07, 7.78e-07, 2.28e-06, 6.03e-06, 1.22e-05, 1.98e-05, 2.69e-05, 3.25e-05, 3.56e-05, 3.64e-05, 3.59e-05, 3.33e-05, 2.89e-05, 2.49e-05, 1.66e-05, 7.82e-06, 0},
        {0.35, 8.77e-08, 1.74e-07, 2.35e-07, 3.6e-07, 7.71e-07, 2.3e-06, 6.38e-06, 1.35e-05, 2.19e-05, 2.94e-05, 3.38e-05, 3.54e-05, 3.49e-05, 3.3e-05, 2.96e-05, 2.48e-05, 2e-05, 1.24e-05, 5.89e-06, 0},
        {0.4, 7.67e-08, 1.5e-07, 2.04e-07, 3.19e-07, 6.85e-07, 2.3e-06, 6.85e-06, 1.46e-05, 2.35e-05, 3e-05, 3.31e-05, 3.29e-05, 3.07e-05, 2.81e-05, 2.49e-05, 2.13e-05, 1.66e-05, 1.55e-05, 8.9e-06, 0},
        {0.45, 6.85e-08, 1.26e-07, 1.74e-07, 2.54e-07, 5.75e-07, 2.05e-06, 6.49e-06, 1.43e-05, 2.28e-05, 2.81e-05, 2.93e-05, 2.77e-05, 2.47e-05, 2.22e-05, 1.95e-05, 1.72e-05, 1.48e-05, 1.13e-05, 9.97e-06, 0},
        {0.5, 6e-08, 1.07e-07, 1.46e-07, 2.14e-07, 4.76e-07, 1.87e-06, 6.42e-06, 1.44e-05, 2.19e-05, 2.5e-05, 2.44e-05, 2.15e-05, 1.85e-05, 1.67e-05, 1.51e-05, 1.23e-05, 1.08e-05, 6.83e-06, 3.94e-06, 0},
        {0.55, 5.45e-08, 8.43e-08, 1.1e-07, 1.62e-07, 3.57e-07, 1.44e-06, 5.31e-06, 1.24e-05, 1.8e-05, 1.92e-05, 1.74e-05, 1.45e-05, 1.24e-05, 1.11e-05, 1.02e-05, 9.23e-06, 8.33e-06, 7.41e-06, 1.03e-05, 0},
        {0.6, 5.04e-08, 6.61e-08, 7.87e-08, 1.04e-07, 2.43e-07, 1.08e-06, 4.34e-06, 1e-05, 1.35e-05, 1.28e-05, 1.05e-05, 8.57e-06, 7.34e-06, 6.72e-06, 6.32e-06, 5.98e-06, 5.48e-06, 5.91e-06, 7.29e-06, 1.29e-05},
        {0.65, 4.65e-08, 5.31e-08, 5.61e-08, 6.96e-08, 1.64e-07, 7.59e-07, 3.32e-06, 7.41e-06, 9e-06, 7.31e-06, 5.61e-06, 4.63e-06, 4.02e-06, 3.77e-06, 3.62e-06, 3.75e-06, 4.05e-06, 4.68e-06, 1.28e-06, 0},
        {0.7, 4.32e-08, 4.14e-08, 3.93e-08, 4.27e-08, 8.67e-08, 4.76e-07, 2.34e-06, 5.12e-06, 4.83e-06, 3.1e-06, 2.29e-06, 1.92e-06, 1.7e-06, 1.6e-06, 1.51e-06, 1.76e-06, 1.86e-06, 2.76e-06, 2.81e-06, 1.47e-05},
        {0.75, 3.67e-08, 2.98e-08, 2.64e-08, 2.72e-08, 5.66e-08, 3.51e-07, 1.83e-06, 3.31e-06, 1.93e-06, 9.42e-07, 6.97e-07, 6.06e-07, 5.35e-07, 5.02e-07, 4.92e-07, 5.28e-07, 6.97e-07, 1.11e-06, 1.85e-06, 0},
        {0.8, 2.4e-08, 1.66e-08, 1.46e-08, 1.32e-08, 2.8e-08, 2.74e-07, 1.42e-06, 1.67e-06, 5.05e-07, 1.83e-07, 1.31e-07, 1.1e-07, 9.81e-08, 9.13e-08, 9.04e-08, 8.69e-08, 9.37e-08, 3.13e-08, -6.82e-21, 0},
        {0.85, 1.02e-08, 4.91e-09, 5.46e-09, 6.62e-09, 1.35e-08, 2.19e-07, 8.5e-07, 4.94e-07, 7e-08, 1.92e-08, 1.37e-08, 1.12e-08, 1e-08, 8.96e-09, 9.5e-09, 8.62e-09, 1.65e-08, 3.26e-08, 1.5e-07, 0},
        {0.9, 9.96e-10, 3.15e-09, 2.89e-09, 3.44e-09, 3.53e-09, 1.49e-07, 4.09e-07, 9.15e-08, 1.53e-09, -2.09e-20, -6.38e-21, -4.12e-21, -1.64e-20, -1.97e-20, -1.74e-20, -1.63e-20, -6.11e-21, -1.17e-20, 0, 0},
        {0.95, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}

    };
    util::matrix_t<double> sub_clipping_matrix(21, 21);
    for (int i = 0; i < 21; i++) {
        for (int j = 0; j < 21; j++) {
            sub_clipping_matrix.at(i, j) = Subhourly_Clipping_Matrix[i][j];
        }
    }

    return sub_clipping_matrix;

}
