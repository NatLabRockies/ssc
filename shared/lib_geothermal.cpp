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

// TFF notes on 25-Jan-2012
// Differences from last version (SAM 2011-21-2) of code that will cause different answers for the same inputs
// 1 - "CalculateNewTemperature" is called with current elapsed time, instead of one month ahead (similar to Jun 2011 version & GETEM spreadsheet)
// 2 - DAYS_PER_YEAR changed to 365 from 365.25
// 3 - "CGeothermalAnalyzer::EGSFractureLength" uses the input "mo_geo_in.md_DistanceBetweenProductionInjectionWellsM" instead of
//     having the distance between wells be hardwired to 1000m regardless of what user enters.
// 4 - Because this code stores values in constants rather than in member variables (that were never changed), the results changed slightly (at about the 4th significant digit)
//     E.G., geothermal::EXCESS_PRESSURE_BAR returns 3.50000000000, but mpGBI->mdExcessPressureBar returned 3.4997786965077915, although it was set to 3.5 and never changed.
// March 2013
// 5 - pump efficiencies were used incorrectly in the old version (fix committed to svn March 1, 2012)
// 6 - pump power calculations in the old version did not match GETEM

#include "lib_physics.h"
#include "lib_geothermal.h"

#ifdef _MSC_VER
#pragma warning(disable: 4127)  // ignore warning: 'warning C4127: conditional expression is constant'
#endif


#ifndef MAX
#define MAX(a,b)  ((a>b)?a:b)
#endif

namespace geothermal
{

	const double MAX_TEMP_RATIO = 1.134324;  // max valid value for (resource temp)/(plant design temp) where both are measured in Kelvin
	const double DEFAULT_AMBIENT_TEMPC_BINARY = 10.0;			// degrees C
	//const double WET_BULB_TEMPERATURE_FOR_FLASH_CALCS = 15.0;	// degrees C, used in Flash calcs brine effectiveness calcs an flash injection temperature calcs
	const bool ADDITIONAL_PRESSURE_REQUIRED = true;
	//const double EFFICIENCY_PUMP_GF = 0.6;
	const double EGS_THERMAL_CONDUCTIVITY = 3 * 3600 * 24;				// J/m-day-C
	//const double PRESSURE_CHANGE_ACROSS_SURFACE_EQUIPMENT_PSI = 25;	// 25 psi [2B.Resource&Well Input].D146, H146
	const double TEMPERATURE_EGS_INJECTIONC = 76.1;					// degrees C, [7C.EGS Subsrfce HX].D11 [should be a function of plant design temperature]
	const double TEMPERATURE_EGS_AMBIENT_C = 15.0;					// Note in GETEM spreadsheet says that this is only used in calculating resource temp or depth.  However, if EGS calculations are based on depth, then resource temp is based on this number, so all power calcs are based on it as well
	const double CONST_CT = 0.000544;									// these are both inputs that are shaded out in GETEM
	const double CONST_CP = 0.000000000464;							//	"		"			"			"			"
	//const double EXCESS_PRESSURE_BAR = 3.5;						// default 3.5 bar, [2B.Resource&Well Input].D205
	//const double PRESSURE_AMBIENT_PSI = 14.7; // default
	const double WATER_LOSS_PERCENT = 0.02;							// 2%
	const double EGS_TIME_INPUT = 3.076;							// years, not really explained - user is supposed to vary input until a calculated value equals plant design temp [7C.EGS Subsrfce HX].D42 (fTimeStar)
	const double FRACTURE_LENGTH_ADJUSTMENT = 2;					// used for one instance of where the EGS fracture length is used.  All others use the original fracture length
	const double DELTA_PRESSURE_HP_FLASH_PSI = 1.0;					//Was 2.2 -> now changed to 1.0 (as seen in GETEM on 10/09/18)
	const double DELTA_PRESSURE_LP_FLASH_PSI = 1.0;
	const double DELTA_TEMPERATURE_CWF = 25.0;						// (degrees F) Was 30.0 -> now changed to 25.0 (as seen in GETEM on 10/09/18)
	const double TEMPERATURE_PINCH_PT_CONDENSER_F = 7.5;			//Was 10.0 -> now changed to 7.5 (as seen in GETEM on 10/09/18)
	const double TEMPERATURE_PINCH_PT_COOLING_TOWER_F = 5;			//Was 15 -> now changed to 5.0 (as seen in GETEM on 10/09/18)
	const double PRESSURE_CONDENSER_NCG_PARTIAL_INHG = 0.32;			// (inches of Mercury) was 0.5 -> now changed to 0.32 (as seen in GETEM on 10/09/18)
	const double GEOTHERMAL_FLUID_FOR_FLASH = 1000;					// D67 in "5C.Flash-Steam Plant Perf" [was an integer, not sure why]
	const double EFFICIENCY_TURBINE = 0.80;							//Was 0.825 -> now changed to 0.80 (as seen in GETEM on 10/09/18)
	const double EFFICIENCY_GENERATOR = 0.98;
	const double EFFICIENCY_PUMP_FLASH = 0.7;
	const ncgRemovalTypes NCG_REMOVAL_TYPE = HYBRID;					//Always type JET??
	const int NUMBER_OF_COOLING_STAGES = 3;							// 1,2, or 3
	const double NCG_LEVEL_PPM = 2000;								//Was 100 -> now changed to 2000 (as seen in GETEM on 10/09/18)
	const double MOLE_WEIGHT_NCG = 44.0;
	const double MOLE_WEIGHT_H2O = 18.0;
	const double BASE_CW_PUMP_HEAD_FT = 65.0;						//Was 60.0 -> now changed to 65.0 (as seen in GETEM on 10/09/18)		
	const condenserTypes CONDENSER_TYPE = SURFACE;
	const double INJECTION_PUMPING_CYCLES = 5.0;					//Was  6.0 -> now changed to 5.0 (as seen in GETEM on 10/09/18) - cell #D133
	const double ADDITIONAL_CW_PUMP_HEAD_SURFACE = 10 * 144 / physics::WATER_DENSITY;
	//const double MAX_TEMPERATURE_DECLINE_C = 30;
	const double FINAL_YEARS_WITH_NO_REPLACEMENT = 5;


	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GETEM Physics and general equations
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	const bool IMITATE_GETEM = true;
	const double GETEM_FT_IN_METER = (IMITATE_GETEM) ? 3.28083 : physics::FT_PER_METER; // feet per meter - largest source of discrepancy
	//const double GETEM_PSI_PER_BAR = (IMITATE_GETEM) ? 14.50377 : physics::PSI_PER_BAR; // psi per bar
    const double GETEM_PSI_PER_BAR = 14.50377; // psi per bar
    const double GETEM_PSI_PER_INHG = (IMITATE_GETEM) ? 0.49115 : physics::PSI_PER_INHG; // psi per inch of mercury
	const double GETEM_KGM3_PER_LBF3 = (IMITATE_GETEM) ? (35.3146 / 2.20462) : physics::KGM3_PER_LBF3; // lbs/ft^3 per kg/m^3 
	const double GETEM_LB_PER_KG = (IMITATE_GETEM) ? 2.20462 : physics::LB_PER_KG; // pounds per kilogram
	const double GETEM_KW_PER_HP = (IMITATE_GETEM) ? 0.7457 : physics::KW_PER_HP; // kilowatts per unit of horsepower
	//const double GRAVITY_MS2 = (IMITATE_GETEM) ? 9.807 : physics::GRAVITY_MS2; // meters per second^2; this varies between 9.78 and 9.82 depending on latitude
    const double GRAVITY_MS2 = 9.807;
	const double DAYS_PER_YEAR = 365.25;

	double MetersToFeet(const double &m) { return m * GETEM_FT_IN_METER; }
	double FeetToMeters(const double &ft) { return ft / GETEM_FT_IN_METER; }
	double M2ToFeet2(const double &mSquared) { return (IMITATE_GETEM) ? mSquared * 10.76391 : mSquared * pow(GETEM_FT_IN_METER, 2); }

	double InHgToPsi(const double &inHg) { return inHg * GETEM_PSI_PER_INHG; }
	double PsiToInHg(const double &psi) { return psi / GETEM_PSI_PER_INHG; }
	double BarToPsi(const double &bar) { return bar * GETEM_PSI_PER_BAR; }

	double KgPerM3ToLbPerCf(const double &kgPerM3) { return kgPerM3 / GETEM_KGM3_PER_LBF3; }
	double LbPerCfToKgPerM3(const double &lbPerCf) { return lbPerCf * GETEM_KGM3_PER_LBF3; }
	double LbPerCfToKgPerM3_B(const double &lbPerCf) { return (IMITATE_GETEM) ? lbPerCf * 16.01846 : lbPerCf * GETEM_KGM3_PER_LBF3; }

	double KgToLb(const double &kg) { return kg * GETEM_LB_PER_KG; }
	double LbToKg(const double &lb) { return lb / GETEM_LB_PER_KG; }

	double HPtoKW(const double &hp) { return hp * GETEM_KW_PER_HP; }
	double KWtoHP(const double &kw) { return kw / GETEM_KW_PER_HP; }

	double PSItoFTB(const double &psi) { return (IMITATE_GETEM) ? psi * 144 / 62 : physics::PSItoFT(psi); }  // convert PSI to pump 'head' in feet.  assumes water density ~ 62 lb/ft^3 if imitating GETEM

	double pumpSizeInHP(const double &flow_LbPerHr, const double &head_Ft, const double &eff, std::string sErr)
	{
		if (eff <= 0) {
			sErr = ("Pump efficiency <= 0 in 'pumpSizeInHP'.");
			return 0;
		}
		return (flow_LbPerHr * head_Ft) / (60 * 33000 * eff);
	}

	double pumpWorkInWattHr(const double &flow_LbPerHr, const double &head_Ft, const double &eff, std::string sErr)
	{
		return HPtoKW(1000 * pumpSizeInHP(flow_LbPerHr, head_Ft, eff, sErr));
	}

	double calcEGSTemperatureConstant(double tempC, double maxSecondLawEff)
	{	// not explained.  a constant used to calculate the 'average water temp' for EGS resource
		// it's used in [7C.EGS Subsrfce HX].D127, and to do the makeup calculations in [6Bb.Makeup-EGS HX]
		double c1 = (-0.0006 * tempC) - 0.0681;
		double c2 = (-0.0004 * tempC) + 1.0166;
		double c3 = (maxSecondLawEff * c1) + c2;
		double c4 = (-0.0002 * tempC) + 0.9117;
		double c5 = (-0.001 * tempC) + 0.55;
		return (tempC < 150) ? c3 : ((maxSecondLawEff < c5) ? c3 : c4);
	}

	double calcEGSAverageWaterTemperatureC(double temp1C, double temp2C, double maxEff)
	{
		return physics::KelvinToCelcius(physics::CelciusToKelvin(temp1C) * calcEGSTemperatureConstant(temp2C, maxEff));
	}


	double gauss_error_function(const double &x)
	{
		// Based VBA code in Xnumbers.xla v 5.6
		// by Foxes Team, 2007
		// E -mail: leovlp@libero.it
		// Web:    http://digilander.libero.it/foxes
		// 10.11.2006


		//if (is) wxMessageBox(wxString::Format("Reached max loops in ERFC calculation (u <= 2). x = %2.10f\n", x));


		// returns the integral of Gauss' standard error function and complimentary error function
		// first 6 digits match MS Excel erfc function, but they are different after that
		int i;
		double u, a0, a1, a2, b0, B1, b2, g, t, p, s, f1, f2 = 0, d;
		double y, yc; // y = err function, yc = complimentary error function
		const int maxloop = 2000;
		const double tiny = 10e-15;
		u = std::abs(x);   //10.11.06 fix bug for x<<0. Thanks to Michael Hautus
		if (u <= 2)
		{
			t = 2 * u * u; p = 1; s = 1;
			for (i = 3; i <= maxloop; i = i + 2)
			{
				p = p * t / i;
				s = s + p;
				if (p < tiny) break;
			}
			//if (i >= maxloop - 1) wxMessageBox(("Reached max loops in ERFC calculation (u<=2)"));
			//if (i >= maxloop - 1)
			//	wxMessageBox(wxString::Format("Reached max loops in ERFC calculation (u <= 2). x = %2.10f\n", x));
			y = 2 * s * u * exp(-u * u) / sqrt(physics::PI);
			if (x < 0) y = -y;
			yc = 1 - y;
		}
		else
		{
			a0 = 1; b0 = 0; a1 = 0; B1 = 1; f1 = 0;
			for (i = 1; i <= maxloop; i++)
			{
				g = 2 - fmod(i, 2.0);
				a2 = g * u * a1 + i * a0;
				b2 = g * u * B1 + i * b0;
				f2 = a2 / b2;
				d = std::abs(f2 - f1);
				if (d < tiny) break;
				a0 = a1 / b2;
				b0 = B1 / b2;
				a1 = a2 / b2;
				B1 = 1;
				f1 = f2;
			}
			//if (i >= maxloop - 1) wxMessageBox(("Reached max loops in ERFC calculation (u > 2)"));
	//		if (i >= maxloop - 1)
	//			wxMessageBox(wxString::Format("Reached max loops in ERFC calculation (u > 2). x = %2.10f\n", x));
			yc = 2 * exp(-u * u) / (2 * u + f2) / sqrt(physics::PI);
			y = 1 - yc;
			if (x < 0) { y = -y; yc = 2 - yc; }
		}
		return yc;  // y = err function, yc = complimentary error function
	}

	double evaluatePolynomial(const double &x, const double &c0, const double &c1, const double &c2, const double &c3, const double &c4, const double &c5, const double &c6)
	{
		return (c0 + (c1 * x) + (c2 * pow(x, 2)) + (c3 * pow(x, 3)) + (c4 * pow(x, 4)) + (c5 * pow(x, 5)) + (c6 * pow(x, 6)));
	}

	// Convert foot-lbs per hour to watt-hr/lb and include pump efficiency
	double FrictionFactor(double Re) { return pow((0.79 * log(Re) - 1.640), -2); }

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Create CPolynomial class and objects to use throughout code
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class CPolynomial
	{
	public:
		CPolynomial(void) { md1 = 0.0; md2 = 0.0; md3 = 0.0; md4 = 0.0; md5 = 0.0; md6 = 0.0; md7 = 0.0; }
		CPolynomial(const double &c1) { md1 = c1; md2 = 0.0; md3 = 0.0; md4 = 0.0; md5 = 0.0; md6 = 0.0; md7 = 0.0; }
		CPolynomial(const double &c1, const double &c2) { md1 = c1; md2 = c2; md3 = 0.0; md4 = 0.0; md5 = 0.0; md6 = 0.0; md7 = 0.0; }
		CPolynomial(const double &c1, const double &c2, const double &c3) { md1 = c1; md2 = c2; md3 = c3; md4 = 0.0; md5 = 0.0; md6 = 0.0; md7 = 0.0; }
		CPolynomial(const double &c1, const double &c2, const double &c3, const double &c4) { md1 = c1; md2 = c2; md3 = c3; md4 = c4; md5 = 0.0; md6 = 0.0; md7 = 0.0; }
		CPolynomial(const double &c1, const double &c2, const double &c3, const double &c4, const double &c5) { md1 = c1; md2 = c2; md3 = c3; md4 = c4; md5 = c5; md6 = 0.0; md7 = 0.0; }
		CPolynomial(const double &c1, const double &c2, const double &c3, const double &c4, const double &c5, const double &c6) { md1 = c1; md2 = c2; md3 = c3; md4 = c4; md5 = c5; md6 = c6; md7 = 0.0; }
		CPolynomial(const double &c1, const double &c2, const double &c3, const double &c4, const double &c5, const double &c6, const double &c7) { md1 = c1; md2 = c2; md3 = c3; md4 = c4; md5 = c5; md6 = c6; md7 = c7; }
		virtual ~CPolynomial(void) {}

		//void init(const double &c1, const double &c2, const double &c3, const double &c4, const double &c5, const double &c6, const double &c7) { md1=c1; md2=c2; md3=c3; md4=c4; md5=c5; md6=c6; md7=c7; }
		double evaluate(double val) { return evaluatePolynomial(val, md1, md2, md3, md4, md5, md6, md7); }

	private:
		double md1, md2, md3, md4, md5, md6, md7;

	};

	// Enthalpy and Entropy Constants
	CPolynomial oAmbientEnthalpyConstants(-31.76958886, 0.997066497, 0.00001087);
	CPolynomial oAmbientEntropyConstants(-0.067875028480951, 0.002201824618666, -0.000002665154152, 0.000000004390426, -0.000000000004355);
	CPolynomial oBinaryEnthalpyConstants(-24.113934502, 0.83827719984, 0.0013462856545, -5.9760546933E-6, 1.4924845946E-8, -1.8805783302E-11, 1.0122595469E-14);
	CPolynomial oBinaryEntropyConstants(-0.060089552413, 0.0020324314656, -1.2026247967E-6, -1.8419111147E-09, 8.8430105661E-12, -1.2945213491E-14, 7.3991541798E-18);
	CPolynomial oFlashEnthalpyConstants(-32.232886, 1.0112508, -0.00013079803, 0.00000050269721, -0.00000000050170088, 1.5041709E-13, 7.0459062E-16);
	CPolynomial oFlashEntropyConstants(-0.067756238, 0.0021979159, -0.0000026352004, 0.0000000045293969, -6.5394475E-12, 6.2185729E-15, -2.2525163E-18);

	// specific volume calculation constants
	CPolynomial oSVC(0.017070951786, -0.000023968043944, 0.00000022418007508, -9.1528222658E-10, 2.1775771856E-12, -2.6995711458E-15, 1.4068205291E-18);

	// pressure calculation constants
	CPolynomial oPC(8.0894106754, -0.19788525656, 0.0019695373372, -0.0000091909636468, 0.000000024121846658, -2.5517506351E-12);
	CPolynomial oPressureAmbientConstants(0.320593729630411, -0.0156410175570826, 0.0003545452343917, -0.0000027120923771, 0.0000000136666056);
	CPolynomial oDensityConstants(62.329, 0.0072343, -0.00012456, 0.00000020215, -0.00000000017845);
	CPolynomial oFlashTempConstants(113.186, -2.48032, 0.0209139, -0.0000557641, 0.0000000542893);

	// used in calculating flash brine effectiveness
	CPolynomial oFlashConstants1(-1.306483, 0.2198881, -0.003125628, 0.0000173028, -0.00000003258986);
	CPolynomial oFlashConstants2(0.01897203, -0.0002054368, 0.000002824477, -0.00000001427949, 0.00000000002405238);
	CPolynomial oPSatConstants(0.0588213, -0.0018299913, 0.00010459209, -0.00000084085735, 0.0000000086940123);

	// EGS
	CPolynomial oEGSDensity(0.001003773308, -0.00000043857183, 0.00000001365689, -0.00000000006419, 0.00000000000013);
	CPolynomial oEGSSpecificHeat(4.301651536642, -0.011554722573, 0.00020328187235, -0.0000011433197, 0.00000000217642);

	// Min geothermal fluid outlet temperatures to prevent Si precipitation
	// If fluid temp >= 356 degrees F (180 C), use quartz curve
	CPolynomial oMinimumTemperatureQuartz(-159.597976, 0.69792956, 0.00035129);
	// If fluid temp < 356 degrees F (180 C), use chalcedony curve
	CPolynomial oMinimumTemperatureChalcedony(-127.71, 0.8229);

	// The constants for the following 19 objects were only used within the CGETEMGlobals class
	CPolynomial oDHaUnder150(60.251233, -0.28682223, 0.0049745244, -0.000050841601, 0.00000026431087, -0.00000000054076309);
	CPolynomial oDHa150To1500(53.67656, -0.02861559, 0.0000469389, -0.000000047788062, 0.000000000024733176, -5.0493347E-15);
	CPolynomial oDHaOver1500(123.86562, -0.18362579, 0.00016780015, -0.000000077555328, 0.000000000017815452, -1.6323827E-15);
	CPolynomial oDHbUnder150(-2.1991099, 1.4133748, -0.019163136, 0.0001766481, -0.00000087079731, 0.0000000017257066);
	CPolynomial oDHb150To1500(33.304544, 0.27192791, -0.00045591346, 0.000000443209, -0.00000000022501399, 4.5323448E-14);
	CPolynomial oDHbOver1500(740.43412, -1.5040745, 0.0014334909, -0.00000067364263, 0.00000000015600207, -1.4371477E-14);

	// Getting enthalpy from temperature
	CPolynomial oFlashEnthalpyFUnder125(-32.479184, 1.0234315, -0.00034115062, 0.0000020320904, -0.000000004480902);
	CPolynomial oFlashEnthalpyF125To325(-31.760088, 0.9998551, -0.000027703224, 0.000000073480055, 0.00000000025563678);
	CPolynomial oFlashEnthalpyF325To675(-1137.0718729, 13.426933583, -0.055373746094, 0.00012227602697, -0.00000013378773724, 5.8634263518E-11);
	CPolynomial oFlashEnthalpyFOver675(-5658291651.7, 41194401.715, -119960.00955, 174.6587566, -0.12714518982, 0.000037021613128);

	CPolynomial oFlashEnthalpyGUnder125(1061.0996074, 0.44148580795, -0.000030268712038, -0.00000015844186585, -7.2150559138E-10);
	CPolynomial oFlashEnthalpyG125To325(1061.9537518, 0.42367961566, 0.000099006018886, -0.00000051596852593, -0.0000000005035389718);
	CPolynomial oFlashEnthalpyG325To675(-3413.791688, 60.38391862, -0.33157805684, 0.00096963380389, -0.0000015842735401, 0.0000000013698021251, -4.9118123157E-13);
	CPolynomial oFlashEnthalpyGOver675(7355226428.1, -53551582.984, 155953.29919, -227.07686319, 0.16531315908, -0.000048138033984);

    CPolynomial oFlashEntropyFUnder125(-0.06778459, 0.002195168, -2.499642E-06, 2.964634E-09);
    CPolynomial oFlashEntropyF125To325(-0.06615222, 0.00215356, -2.16312E-06, 2.223552E-09, -9.593628E-13);
    CPolynomial oFlashEntropyF325To675(4.729245, -0.06296176, 3.612208E-04, -1.064881E-06, 1.739088E-09, -1.495185E-12, 5.297508E-16);
    CPolynomial oFlashEntropyFOver675(10317090, -91866.44, 340.7939, -0.674174, 7.501037E-04, -4.450574E-07, 1.100136E-10);

    CPolynomial oFlashEntropyGUnder125(2.312154, -0.004205795, 0.00001113945, -2.213415E-08, 2.38219E-11);
    CPolynomial oFlashEntropyG125To325(2.305898, -0.004032959, 9.334758E-06, -1.361597E-08, 8.392618E-12);
    CPolynomial oFlashEntropyG325To675(-5.19791, 0.09784205, -5.589582E-04, 1.65401E-06, -2.708314E-09, 2.333203E-12, -8.283605E-16);
    CPolynomial oFlashEntropyGOver675(-13250460, 118002.2, -437.8104, 0.8662219, -9.639254E-04, 5.720121E-07, -1.41418E-10);

	// Getting temperature from pressure
	CPolynomial oFlashTemperatureUnder2(14.788238833, 255.85632577, -403.56297354, 400.57269432, -222.30982965, 63.304761377, -7.1864066799);
	CPolynomial oFlashTemperature2To20(78.871966537, 31.491049082, -4.8016701723, 0.49468791547, -0.029734376328, 0.00094358038872, -0.000012178121702);
	CPolynomial oFlashTemperature20To200(161.40853789, 4.3688747745, -0.062604066919, 0.00061292292067, -0.0000034988475881, 0.00000001053096688, -1.2878309875E-11);
	CPolynomial oFlashTemperature200To1000(256.29706201, 0.93056131917, -0.0020724712921, 0.0000034048164769, -0.0000000034275245432, 1.8867165569E-12, -4.3371351471E-16);
	CPolynomial oFlashTemperatureOver1000(342.90613285, 0.33345911089, -0.00020256473758, 0.000000094407417758, -2.7823504188E-11, 4.589696886E-15, -3.2288675486E-19);

	// Second law equations, used in FractionOfMaxEfficiency
	CPolynomial oSecondLawConstantsBinary(-10.466, 22.422, -10.956, 0); // ("6Ab. Makeup-Annl%").Range("R24:R27")
	CPolynomial oSecondLawConstantsSingleFlashAbove240(-10.467, 22.89446, -11.42747, 0);	// ("6Ef.Flash Makeup").Range("R20:V20")
    CPolynomial oSecondLawConstantsSingleFlashBelow240(-9.07044, 20.13903, -10.06859, 0);	// ("6Ef.Flash Makeup").Range("R20:V20")
	CPolynomial oSecondLawConstantsDualFlashAbove210(-8.8276, 19.388, -9.5604, 0); // ("6Ef.Flash Makeup").Range("R22:V22")
	CPolynomial oSecondLawConstantsDualFlashBelow210(-10.124, 21.683, -10.599, 0);	// ("6Ef.Flash Makeup").Range("R21:V21")


	//Specific Volume Coefficients (Used in Flass Vessels Cost Calculation):
	CPolynomial specVolUnder125(11678.605, -464.41472, 8.9931223, -0.1033793, 0.00071596466, -0.0000027557218, 0.0000000045215227);
	CPolynomial specVol125to325(3890.919, -83.834081, 0.78482148, -0.0040132715, 0.000011692082, -0.000000018270648, 0.000000000011909478);
	CPolynomial specVol325to675(268.32894, -2.7389634, 0.011958041, -0.000028277928, 0.000000037948334, -0.000000000027284644, 8.187709e-15);
	CPolynomial specVolOver675(1786.8983, 10.645163, -0.023769687, 0.000023582903, -0.0000000087731388);

    //Pressure from temperature for flash plants
    CPolynomial PressureUnder125(0.021248, 1.11e-03, 1.84e-05, 3.42e-07, 1.14e-09, 1.68e-11, 9.97e-15);
    CPolynomial Pressure125to325(-0.0607916, 6.00322e-03, -9.09892e-05, 1.58988e-06, -6.75074e-09, 4.34771e-11, -2.80188e-14);
    CPolynomial Pressure325to675(1934.47, -26.49, 0.150475, -4.54e-04, 7.68e-07, -6.63e-10, 2.43e-13);
    CPolynomial PressureOver675(10153.58, -125.9218, 0.6471745, -1.77e-03, 2.70e-06, -2.17e-09, 7.26e-13);

	double EGSWaterDensity(double tempC) { return 1 / oEGSDensity.evaluate(tempC); }			// kg/m^3
	double EGSSpecificHeat(double tempC) { return oEGSSpecificHeat.evaluate(tempC) * 1000; }	// J/kg-C




	double GetDHa(double pressurePSI)
	{
		if (pressurePSI > 1500)
			return oDHaOver1500.evaluate(pressurePSI);
		else if (pressurePSI > 150)
			return oDHa150To1500.evaluate(pressurePSI);
		else
			return oDHaUnder150.evaluate(pressurePSI);
	}

	double GetDHb(double pressurePSI)
	{
		if (pressurePSI > 1500)
			return oDHbOver1500.evaluate(pressurePSI);
		else if (pressurePSI > 150)
			return oDHb150To1500.evaluate(pressurePSI);
		else
			return oDHbUnder150.evaluate(pressurePSI);
	}

	double GetFlashEnthalpyF(double temperatureF)
	{
		if (temperatureF > 675)
			return  oFlashEnthalpyFOver675.evaluate(temperatureF);
		else if (temperatureF > 325)
			return  oFlashEnthalpyF325To675.evaluate(temperatureF);
		else if (temperatureF > 125)
			return  oFlashEnthalpyF125To325.evaluate(temperatureF);
		else
			return  oFlashEnthalpyFUnder125.evaluate(temperatureF);
	}

	double GetFlashEnthalpyG(double temperatureF)
	{
		if (temperatureF > 675)
			return  oFlashEnthalpyGOver675.evaluate(temperatureF);
		else if (temperatureF > 325)
			return  oFlashEnthalpyG325To675.evaluate(temperatureF);
		else if (temperatureF > 125)
			return  oFlashEnthalpyG125To325.evaluate(temperatureF);
		else
			return  oFlashEnthalpyGUnder125.evaluate(temperatureF);
	}

    double GetFlashEntropyF(double temperatureF)
    {
        if (temperatureF > 675)
            return  oFlashEntropyFOver675.evaluate(temperatureF);
        else if (temperatureF > 325)
            return  oFlashEntropyF325To675.evaluate(temperatureF);
        else if (temperatureF > 125)
            return  oFlashEntropyF125To325.evaluate(temperatureF);
        else
            return  oFlashEntropyFUnder125.evaluate(temperatureF);
    }

    double GetFlashEntropyG(double temperatureF)
    {
        if (temperatureF > 675)
            return  oFlashEntropyGOver675.evaluate(temperatureF);
        else if (temperatureF > 325)
            return  oFlashEntropyG325To675.evaluate(temperatureF);
        else if (temperatureF > 125)
            return  oFlashEntropyG125To325.evaluate(temperatureF);
        else
            return  oFlashEntropyGUnder125.evaluate(temperatureF);
    }

	double GetFlashTemperature(double pressurePSI)
	{
		if (pressurePSI > 1000)
			return  oFlashTemperatureOver1000.evaluate(pressurePSI);
		else if (pressurePSI > 200)
			return  oFlashTemperature200To1000.evaluate(pressurePSI);
		else if (pressurePSI > 20)
			return  oFlashTemperature20To200.evaluate(pressurePSI);
		else if (pressurePSI > 2)
			return  oFlashTemperature2To20.evaluate(pressurePSI);
		else
			return  oFlashTemperatureUnder2.evaluate(pressurePSI);
	}

	double getSpecVol(double flashTempF)
	{
		if (flashTempF > 675)
			return specVolOver675.evaluate(flashTempF);
		else if (flashTempF > 325)
			return specVol325to675.evaluate(flashTempF);
		else if (flashTempF > 125)
			return specVol125to325.evaluate(flashTempF);
		else
			return specVolUnder125.evaluate(flashTempF);

	}

	double GetSiPrecipitationTemperatureF(double geoFluidTempF)
	{
		return (geoFluidTempF >= 356) ? oMinimumTemperatureQuartz.evaluate(geoFluidTempF) : oMinimumTemperatureChalcedony.evaluate(geoFluidTempF);
	}



	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Declaration of CGeoFluidContainer2 
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class CGeoFluidContainer2
	{
	public:
		double GetAEForBinaryWattHr(double tempF, double ambientTempF) { return physics::toWattHr(GetAEForBinaryBTU(tempF, ambientTempF)); }
		double GetAEForFlashWattHr(double tempF, double ambientTempF) { return physics::toWattHr(GetAEForFlashBTU(tempF, ambientTempF)); }

		double GetAEForBinaryWattHrUsingC(double tempC, double ambientTempC) { return GetAEForBinaryWattHr(physics::CelciusToFarenheit(tempC), physics::CelciusToFarenheit(ambientTempC)); }
		double GetAEForFlashWattHrUsingC(double tempC, double ambientTempC) { return GetAEForFlashWattHr(physics::CelciusToFarenheit(tempC), physics::CelciusToFarenheit(ambientTempC)); }

	private:
		double GetAEForBinaryBTU(double tempHighF, double tempLowF)
		{
			return (oBinaryEnthalpyConstants.evaluate(tempHighF) - oAmbientEnthalpyConstants.evaluate(tempLowF)) - ((tempLowF + 460) * (oBinaryEntropyConstants.evaluate(tempHighF) - oAmbientEntropyConstants.evaluate(tempLowF)));
		}

		double GetAEForFlashBTU(double tempHighF, double tempLowF)
		{
			return (oBinaryEnthalpyConstants.evaluate(tempHighF) - oAmbientEnthalpyConstants.evaluate(tempLowF)) - ((tempLowF + 460) * (oBinaryEntropyConstants.evaluate(tempHighF) - oAmbientEntropyConstants.evaluate(tempLowF)));
		}

	};
	CGeoFluidContainer2 oGFC;

};// namespace geotheraml




//******************************************************************************************************************************************************************************
//******************************************************************************************************************************************************************************
// Implementation of CGeoHourlyAnalysis
//******************************************************************************************************************************************************************************
//******************************************************************************************************************************************************************************
CGeothermalAnalyzer::CGeothermalAnalyzer(const SGeothermal_Inputs& gti, SGeothermal_Outputs& gto)
	: mp_geo_out(&gto), mo_geo_in(gti)
{
	init();
}

CGeothermalAnalyzer::CGeothermalAnalyzer(const SPowerBlockParameters& pbp, SPowerBlockInputs& pbi, const SGeothermal_Inputs& gti, SGeothermal_Outputs& gto)
	: mp_geo_out(&gto), mo_geo_in(gti), mo_pb_p(pbp), mo_pb_in(pbi)
{
	init();
}

CGeothermalAnalyzer::~CGeothermalAnalyzer(void)
{
	// delete anything?
}

void CGeothermalAnalyzer::init()
{	// code common to constructors
	ms_ErrorString = "";
	mf_LastIntervalDone = 0.0f;
	mb_WeatherFileOpen = false;
	ml_ReadCount = 0;
	ml_HourCount = 0;
	me_makeup = NO_MAKEUP_ALGORITHM;
	mi_ReservoirReplacements = 0;
	md_WorkingTemperatureC = 0.0;
	md_LastProductionTemperatureC = 0.0;
	md_TimeOfLastReservoirReplacement = 0.0;
}

bool CGeothermalAnalyzer::IsHourly() { return (mo_geo_in.mi_MakeupCalculationsPerYear == 8760) ? true : false; }

double CGeothermalAnalyzer::PlantGrossPowerkW(void)
{
	double dPlantBrineEfficiency = 0;  // plant Brine Efficiency as a function of temperature
    double dGrossOutput = 0;
    double dGrossPower = 0;
	switch (me_makeup)
	{
    case MA_EGS_BINARY:
	case MA_BINARY:
		//dPlantBrineEfficiency = MaxSecondLawEfficiency() * FractionOfMaxEfficiency() * ((geothermal::IMITATE_GETEM) ? GetAEBinary() : GetAE());				//MaxSecondLawEfficiency() * FractionOfMaxEfficiency() * GetAEBinaryAtTemp(md_WorkingTemperatureC);
        dPlantBrineEfficiency = MaxSecondLawEfficiency() * mo_geo_in.md_PlantEfficiency * FractionOfMaxEfficiency() * GetAEBinaryAtTemp(md_WorkingTemperatureC - DT_prod_well(mo_geo_in.md_dtProdWellChoice));	
		break;
    case MA_EGS_FLASH:
	case MA_FLASH:
		dPlantBrineEfficiency = MaxSecondLawEfficiency() * FractionOfMaxEfficiency() * GetAEFlashAtTemp(md_WorkingTemperatureC- DT_prod_well(mo_geo_in.md_dtProdWellChoice));
        /*calculateFlashPressures();
        dGrossOutput = turbine1OutputKWh();
        if (FlashCount() == 2) dGrossOutput += turbine2OutputKWh();
        dGrossPower = dGrossOutput * geothermal::EFFICIENCY_GENERATOR;
        dPlantBrineEfficiency = dGrossPower;*/
        break;
	default: ms_ErrorString = ("Invalid make up technology in CGeothermalAnalyzer::PlantGrossPowerkW"); return 0;
	}

	return dPlantBrineEfficiency * flowRateTotal() / 1000.0;
}

double CGeothermalAnalyzer::GrossPowerMW()
{
    calculateFlashPressures();
    double dGrossOutput = turbine1OutputKWh();
    if (FlashCount() == 2) dGrossOutput += turbine2OutputKWh();
    double dGrossPower = dGrossOutput * geothermal::EFFICIENCY_GENERATOR;
    return dGrossPower * flowRateTotal() / 1000;
}

double CGeothermalAnalyzer::MaxSecondLawEfficiency()
{
	// the available energy, in GETEM (dGetemAEForSecondLaw), is actually based on plant design temp, although resource temp is being used to calculate the output
	// this only matters for EGS resources, where resource temp and plant design temp are different
	// this leads to Plant brine effectiveness higher than input values
	// which leads to actual plant output(after pumping losses) > design output (before pump losses) ??
	// which leads to relative revenue > 1 ??
	double dGetemAEForSecondLaw = (me_makeup == MA_BINARY || me_makeup == MA_EGS_BINARY) ? GetAEBinary() : GetAE(); // GETEM uses the correct ambient temperature, but it always uses Binary constants, even if flash is chosen as the conversion technology
    //double dGetemAEForSecondLaw = GetAEBinary();
    mp_geo_out->eff_secondlaw = GetPlantBrineEffectiveness() / dGetemAEForSecondLaw;	//2nd law efficiency used in direct plant cost calculations. This is NOT the same as the MAX 2nd law efficiency.
	if (me_makeup == MA_BINARY || me_makeup == MA_EGS_BINARY)
		return (mp_geo_out->max_secondlaw);
	else
		return (GetPlantBrineEffectiveness() / dGetemAEForSecondLaw);
}


double CGeothermalAnalyzer::FractionOfMaxEfficiency()
{
	double dTemperatureRatio = 0.0;
	dTemperatureRatio = physics::CelciusToKelvin(physics::FarenheitToCelcius(TemperatureWetBulbF())) / physics::CelciusToKelvin(md_WorkingTemperatureC);
    double carnot_eff_initial = 1 - physics::CelciusToKelvin(physics::FarenheitToCelcius(TemperatureWetBulbF())) / physics::CelciusToKelvin(GetTemperaturePlantDesignC());
    double carnot_eff = 1 - dTemperatureRatio;
    double carnot_ratio = carnot_eff / carnot_eff_initial;
	if (me_makeup == MA_FLASH || me_makeup == MA_EGS_FLASH)
	{
		switch (mo_geo_in.me_ft)
		{
		case SINGLE_FLASH_NO_TEMP_CONSTRAINT:
		case SINGLE_FLASH_WITH_TEMP_CONSTRAINT:
            
            if (GetResourceTemperatureC() <= 240.0)
                return -10.06859 * pow(carnot_ratio, 2) + 20.13903 * carnot_ratio - 9.07044;
            else
                return -11.42747 * pow(carnot_ratio, 2) + 22.89466 * carnot_ratio - 10.467;

		case DUAL_FLASH_NO_TEMP_CONSTRAINT:
		case DUAL_FLASH_WITH_TEMP_CONSTRAINT:
            if (GetResourceTemperatureC() <= 210.0)
			    return -10.559 * pow(carnot_ratio, 2) + 21.683 * carnot_ratio - 10.124;
            else
                return -9.5604 * pow(carnot_ratio, 2) + 19.388 * carnot_ratio - 8.8276;

		default: ms_ErrorString = ("Invalid flash technology in CGeothermalAnalyzer::FractionOfMaxEfficiency"); return 0;
		}

	}
	else // Binary and EGS
		return -10.956 * pow(carnot_ratio, 2) + 22.422 * carnot_ratio - 10.466;
}

bool CGeothermalAnalyzer::CanReplaceReservoir(double dTimePassedInYears)
{
	return ((mi_ReservoirReplacements < NumberOfReservoirs()) && (dTimePassedInYears + geothermal::FINAL_YEARS_WITH_NO_REPLACEMENT <= mo_geo_in.mi_ProjectLifeYears)) ? true : false;
}

void CGeothermalAnalyzer::CalculateNewTemperature(double dElapsedTimeInYears)
{
	if (me_makeup != MA_EGS_FLASH || me_makeup != MA_EGS_BINARY)
		md_WorkingTemperatureC = md_WorkingTemperatureC * (1 - (mo_geo_in.md_TemperatureDeclineRate / 12));
	else
	{
		// The EGS temperature drop depends on the amount of fluid being produced (makes intuitive sense).
		md_LastProductionTemperatureC = md_WorkingTemperatureC;

		double dAverageReservoirTempC = geothermal::calcEGSAverageWaterTemperatureC(md_LastProductionTemperatureC, md_LastProductionTemperatureC, MaxSecondLawEfficiency());
		//double dDaysSinceLastReDrill = (md_YearsAtNextTimeStep - md_TimeOfLastReservoirReplacement) * geothermal::DAYS_PER_YEAR;
		double dDaysSinceLastReDrill = (dElapsedTimeInYears - md_TimeOfLastReservoirReplacement) * geothermal::DAYS_PER_YEAR;
		double dFunctionOfRockProperties = EGSReservoirConstant(dAverageReservoirTempC, dDaysSinceLastReDrill); //[6Bb.Makeup-EGS HX] column AG

		double tempBrineEfficiencyC = physics::KelvinToCelcius(exp((-0.42 * log(md_LastProductionTemperatureC) + 1.4745) * MaxSecondLawEfficiency() * FractionOfMaxEfficiency()) * physics::CelciusToKelvin(md_LastProductionTemperatureC));
		double tempSILimitC = physics::FarenheitToCelcius(geothermal::GetSiPrecipitationTemperatureF(physics::CelciusToFarenheit(md_LastProductionTemperatureC)));
		double dNewInjectionTemperatureC = MAX(tempBrineEfficiencyC, tempSILimitC);
		double dNewEGSProductionTemperatureC = GetResourceTemperatureC() + ((dNewInjectionTemperatureC - GetResourceTemperatureC()) * dFunctionOfRockProperties);

		md_WorkingTemperatureC = dNewEGSProductionTemperatureC;
	}
}


double CGeothermalAnalyzer::GetPumpWorkKW(void)
{
	return (mo_geo_in.mb_CalculatePumpWork) ? GetPumpWorkWattHrPerLb() * flowRateTotal() / 1000.0 : mo_geo_in.md_UserSpecifiedPumpWorkKW;
}

double CGeothermalAnalyzer::NumberOfReservoirs(void)
{
	double d1 = GetAEBinary();
	if ((d1 == 0) && (geothermal::IMITATE_GETEM))
	{
		ms_ErrorString = ("GetAEBinary returned zero in CGeothermalAnalyzer::NumberOfReservoirs. Could not calculate the number of reservoirs.");
		return 0;
	}

	double dFactor = (geothermal::IMITATE_GETEM) ? GetAE() / d1 : 1;
	double dPlantOutputKW = dFactor * flowRateTotal() * GetPlantBrineEffectiveness() / 1000.0; // KW = (watt-hr/lb)*(lbs/hr) / 1000
	if (dPlantOutputKW == 0)
	{
		ms_ErrorString = ("The Plant Output was zero in CGeothermalAnalyzer::NumberOfReservoirs. Could not calculate the number of reservoirs.");
		return 0;
	}
	return floor(mo_geo_in.md_PotentialResourceMW * 1000 / dPlantOutputKW);
}


double CGeothermalAnalyzer::CalculatePumpWorkInKW(double dFlowLbPerHr, double dPumpHeadFt)
{
	//	double test = geothermal::pumpWorkInWattHr(dFlowLbPerHr, dPumpHeadFt, geothermal::EFFICIENCY_PUMP_FLASH, ms_ErrorString);

	return geothermal::HPtoKW((dFlowLbPerHr * dPumpHeadFt) / (60 * 33000 * geothermal::EFFICIENCY_PUMP_FLASH));
}

double CGeothermalAnalyzer::GetPumpWorkWattHrPerLb(void)
{	// Enter 1 for flow to Get power per lb of flow
	//double dProductionPumpPower = geothermal::pumpWorkInWattHr(1, pumpHeadFt(), geothermal::EFFICIENCY_PUMP_GF, ms_ErrorString);
	
    //double dProductionPumpPower = geothermal::pumpWorkInWattHr(1, GetProductionPumpWorkft(), mo_geo_in.md_GFPumpEfficiency, ms_ErrorString);
	if (!ms_ErrorString.empty()) return 0;

	double dInjectionPumpPower = 0;
    double dFractionOfInletGFInjected = 1.0;
	if (geothermal::ADDITIONAL_PRESSURE_REQUIRED)
	{
		double dWaterLoss = (1 / (1 - mo_geo_in.md_WaterLossPercent)); // G130 - lb/hr

		
		if (mo_geo_in.me_rt == EGS)
			dFractionOfInletGFInjected =  1.0 / (1 - mo_geo_in.md_WaterLossPercent);
		else if (mo_geo_in.me_ct == FLASH)
		{
			calculateFlashPressures();
			double dWaterLossFractionOfGF = waterLoss() / geothermal::GEOTHERMAL_FLUID_FOR_FLASH;
            dFractionOfInletGFInjected = (1 - dWaterLossFractionOfGF);
			//return (1 - dWaterLossFractionOfGF);
		}

		// Calculate injection pump items, on [7A.GF Pumps] unless otherwise noted
		double dInjectionPressure = mo_geo_in.md_AdditionalPressure + geothermal::BarToPsi(mo_geo_in.md_ExcessPressureBar) + GetPressureChangeAcrossReservoir();
		if (mo_geo_in.md_AdditionalPressure < 0)
		{
			//			double injectionPressurePSI = 150 - (pressureInjectionWellBottomHolePSI() - pressureHydrostaticPSI() );
			//			double dInjectionPressure = (injectionPressurePSI < 0) ? 0 : injectionPressurePSI; // G40,  If it's less than zero, use zero.
		}
		double dInjectionPumpHeadFt = dInjectionPressure * 144 / InjectionDensity(); // G129

		//dInjectionPumpPower = geothermal::pumpWorkInWattHr(dWaterLoss, dInjectionPumpHeadFt, geothermal::EFFICIENCY_PUMP_GF, ms_ErrorString) * dFractionOfInletGFInjected; // ft-lbs/hr
		//dInjectionPumpPower = geothermal::pumpWorkInWattHr(dWaterLoss, dInjectionPumpHeadFt, mo_geo_in.md_GFPumpEfficiency, ms_ErrorString) * dFractionOfInletGFInjected; // ft-lbs/hr
        dInjectionPumpPower = geothermal::pumpWorkInWattHr(1, GetInjectionPumpWorkft(), mo_geo_in.md_GFPumpEfficiency, ms_ErrorString) * dFractionOfInletGFInjected; // ft-lbs/hr
	}

    double dProductionPumpPower = geothermal::pumpWorkInWattHr(1, GetProductionPumpWorkft(), mo_geo_in.md_GFPumpEfficiency, ms_ErrorString);
    //double dProductionPumpPower = geothermal::pumpWorkInWattHr(1,GetProductionPumpWorkft(GetInjectionPumpWorkft()), mo_geo_in.md_GFPumpEfficiency, ms_ErrorString);
    mp_geo_out->md_pumpwork_prod = dProductionPumpPower;
    mp_geo_out->md_pumpwork_inj = dInjectionPumpPower;
    mp_geo_out->md_FractionGFInjected = dFractionOfInletGFInjected;
    double check = GetProductionPumpWorkft();
    double retVal = 0;
    if (mo_geo_in.me_ct == FLASH)
        retVal = dInjectionPumpPower; // watt-hr per lb of flow
    else if (mo_geo_in.me_ct == BINARY)
        retVal = dProductionPumpPower + dInjectionPumpPower;

	if (retVal < 0)
	{
		ms_ErrorString = ("CGeothermalAnalyzer::GetPumpWorkWattHrPerLb calculated a value < 0");
		return 0;
	}

	return retVal;
}

double CGeothermalAnalyzer::GetInjectionPumpWorkft(void)
{
    double flow = mo_geo_in.md_ProductionFlowRateKgPerS / mo_geo_in.md_RatioInjectionToProduction; //kg/s
    double flow_lbh = flow * 2.20462 * 3600;
    //Upper interval
    double D_well = 0;
    if (mo_geo_in.md_InjectionWellDiam == 0) {
        D_well = 12.5; //inches (larger diameter)
        mo_geo_in.md_DiameterInjPumpCasingInches = 12.25;
        mo_geo_in.md_DiameterInjectionWellInches = 12.5;
    }
    else {
        D_well = 8.75; //inches (smaller diameter)
        mo_geo_in.md_DiameterInjPumpCasingInches = 8.5;
        mo_geo_in.md_DiameterInjectionWellInches = 8.75;
    }
    //double D_well = mo_geo_in.md_DiameterInjPumpCasingInches;
    double D_well_ft = D_well / 12;
    double A = 3.1415 * pow(D_well_ft, 2) / 4;
    double L_int = 0.8 * mo_geo_in.md_ResourceDepthM; //Length interval (m), how is this calculated?
    double surf_rough_casing = 0.00015; //different for open hole vs. slotted liner
    double dT_dL = (DT_prod_well(mo_geo_in.md_dtProdWellChoice)) / (mo_geo_in.md_ResourceDepthM);
    double T_star = InjectionTemperatureC() + dT_dL * mo_geo_in.md_RatioInjectionToProduction * (L_int) * 0.5;
    double P_sat = geothermal::oPC.evaluate(T_star*1.8 + 32);
    double rho_sat = 1 / geothermal::oSVC.evaluate(T_star * 1.8 + 32);
    double viscosity = 407.22 * pow((T_star * 1.8 + 32), -1.194) / 3600;
    double rho_head = L_int * 3.28084 * rho_sat / 144;
    double P_inject_wellhead = (mo_geo_in.me_ct == FLASH) ? mp_geo_out->md_PressureLPFlashPSI : geothermal::oPC.evaluate((GetTemperaturePlantDesignC() - DT_prod_well(mo_geo_in.md_dtProdWellChoice)) * 1.8 + 32) + physics::PSI_PER_BAR * mo_geo_in.md_ExcessPressureBar - mo_geo_in.md_PressureChangeAcrossSurfaceEquipmentPSI;
    double P_ratio = 0.5 * (rho_head + P_inject_wellhead) / P_sat;
    double rho_correction = 1 + (7.15037e-19 * pow(physics::CelciusToFarenheit(T_star), 5.91303)) * (P_ratio - 1);
    double mu_correction = 1 + (4.02401e-18 * pow(physics::CelciusToFarenheit(T_star), 5.736882)) * (P_ratio - 1);
    double flow_cubic = flow_lbh / (3600 * rho_sat * rho_correction);
    double vel_well = flow_cubic / A;
    double Re_well = vel_well * D_well_ft * rho_sat * rho_correction / (viscosity * mu_correction);
    double friction_factor_smooth = pow(0.79 * log(Re_well) - 1.64, -2);
    //Serghide
    double a = -2 * log10((surf_rough_casing / D_well_ft) / 3.7 + 12 / Re_well);
    double v = -2 * log10((surf_rough_casing / D_well_ft) / 3.7 + 2.51 * a / Re_well);
    double c = -2 * log10((surf_rough_casing / D_well_ft) / 3.7 + 2.51 * v / Re_well);
    double f = pow((a - pow((v - a), 2) / (c - 2 * v + a)),-2);
    double friction_head_loss = (f / D_well_ft) * pow(vel_well, 2) / (2 * 32.174);
    double friction_head_loss_ft = friction_head_loss * L_int * physics::FT_PER_METER;
    double friction_head_psid1 = friction_head_loss_ft * rho_sat * rho_correction / 144;
    double P_start = 0;
    if (mo_geo_in.me_ct == FLASH) P_start = mp_geo_out->md_PressureLPFlashPSI;
    else P_start = geothermal::oPC.evaluate((GetResourceTemperatureC() - dT_dL * GetResourceDepthM()) * 1.8 + 32) + physics::mBarToPSI(mo_geo_in.md_ExcessPressureBar*1000) - mo_geo_in.md_PressureChangeAcrossSurfaceEquipmentPSI;
    double P_upper_bottom_interval = P_start + rho_sat * rho_correction * physics::FT_PER_METER * L_int / 144 - friction_head_psid1;

    //Injection interval
    flow = mo_geo_in.md_ProductionFlowRateKgPerS / mo_geo_in.md_RatioInjectionToProduction; //kg/s
    flow_lbh = flow * 2.20462 * 3600;
    //Upper interval
    D_well = 0;
    if (mo_geo_in.md_InjectionWellDiam == 0) D_well = 12.25; //inches (larger diameter)
    else D_well = 8.50; //inches (smaller diameter)
    //D_well = mo_geo_in.md_DiameterInjectionWellInches;
    D_well_ft = D_well / 12;
    A = 3.1415 * pow(D_well_ft, 2) / 4;
    L_int = 0.2 * mo_geo_in.md_ResourceDepthM; //Length interval (m), how is this calculated?
    surf_rough_casing = 0.02; //different for open hole vs. slotted liner
    if (mo_geo_in.md_InjectionWellType == 1) surf_rough_casing = 0.001;
    //dT_dL = (mo_geo_in.md_dtProdWell * 1.8) / (physics::FT_PER_METER * mo_geo_in.md_ResourceDepthM);
    T_star = InjectionTemperatureC() + dT_dL * mo_geo_in.md_RatioInjectionToProduction * (0.5 * L_int + (mo_geo_in.md_ResourceDepthM - L_int));
    P_sat = geothermal::oPC.evaluate(T_star * 1.8 + 32);
    rho_sat = 1 / geothermal::oSVC.evaluate(T_star * 1.8 + 32);
    viscosity = 407.22 * pow((T_star * 1.8 + 32), -1.194) / 3600;
    rho_head = L_int * 3.28084 * rho_sat / 144;
    P_ratio = (0.5 * rho_head + P_upper_bottom_interval) / P_sat;
    rho_correction = 1 + (7.15037e-19 * pow(physics::CelciusToFarenheit(T_star), 5.91303)) * (P_ratio - 1);
    mu_correction = 1 + (4.02401e-18 * pow(physics::CelciusToFarenheit(T_star), 5.736882)) * (P_ratio - 1);
    flow_cubic = flow_lbh / (3600 * rho_sat * rho_correction);
    vel_well = flow_cubic / A;
    Re_well = vel_well * D_well_ft * rho_sat * rho_correction / (viscosity * mu_correction);
    friction_factor_smooth = pow(0.79 * log(Re_well) - 1.64, -2);
    //Serghide
    a = -2 * log10((surf_rough_casing / D_well_ft) / 3.7 + 12 / Re_well);
    v = -2 * log10((surf_rough_casing / D_well_ft) / 3.7 + 2.51 * a / Re_well);
    c = -2 * log10((surf_rough_casing / D_well_ft) / 3.7 + 2.51 * v / Re_well);
    f = pow((a - pow((v - a), 2) / (c - 2 * v + a)), -2);
    friction_head_loss = (f / D_well_ft) * pow(vel_well, 2) / (2 * 32.174);
    //if (mo_geo_in.me_ct == BINARY) friction_head_loss = friction_head_loss * 1.0 / 3.0;
    if (mo_geo_in.me_rt == EGS) friction_head_loss_ft *= 1.0 / 3.0;
    friction_head_loss_ft = friction_head_loss * L_int * physics::FT_PER_METER;
    double friction_head_psid2 = friction_head_loss_ft * rho_sat * rho_correction / 144;
    double P_bottomhole = P_upper_bottom_interval + rho_sat * rho_correction * physics::FT_PER_METER * L_int / 144 - friction_head_psid2;
    double bottom_hole_pressure = pressureInjectionWellBottomHolePSI();
    mo_geo_in.md_InjWellFriction = friction_head_psid1 + friction_head_psid2;
    //double reservoir_buildup = GetPressureChangeAcrossReservoir() / mo_geo_in.md_RatioInjectionToProduction; //Injectivity index
    double reservoir_buildup = flowRatePerWell() / mo_geo_in.md_RatioInjectionToProduction / mo_geo_in.md_InjectivityIndex;
    //double excess_pressure = bottom_hole_pressure - pressureHydrostaticPSI();
    double excess_pressure = P_bottomhole - pressureHydrostaticPSI();
    //double reservoir_buildup = GetPressureChangeAcrossReservoir();
    double injection_pump_head_psi = -excess_pressure + reservoir_buildup + mo_geo_in.md_AdditionalPressure;
    mo_geo_in.md_InjWellPressurePSI = injection_pump_head_psi; 
    double injection_pump_head_ft = injection_pump_head_psi * 144 / InjectionDensity();
    if (injection_pump_head_ft < 0.0) injection_pump_head_ft = 0;
    double P_inject_bottomhole_used = injection_pump_head_psi + bottom_hole_pressure;
    //double pump_inj_hp = (injection_pump_head_ft * (flowRateTotal() / mo_geo_in.md_RatioInjectionToProduction / 2500) / (60 * 33000)) / mo_geo_in.md_GFPumpEfficiency;
    //mp_geo_out->md_InjPump_hp = pump_inj_hp;
    //double pump_inj_kW = pump_inj_hp * 0.7457;
    return injection_pump_head_ft;
}

double CGeothermalAnalyzer::GetProductionPumpWorkft(void)
{
    double P_res = pressureHydrostaticPSI();
    double Prod_well_minus_bottomhole = P_res - GetPressureChangeAcrossReservoir();
    double flow = mo_geo_in.md_ProductionFlowRateKgPerS; //kg/s
    double flow_lbh = flow * 2.20462 * 3600;
    //Upper interval
    double D_well = 0;
    if (mo_geo_in.md_ProductionWellDiam == 0) {
        D_well = 12.25; //inches (larger diameter)
        mo_geo_in.md_DiameterPumpCasingInches = 12.25;
        mo_geo_in.md_DiameterProductionWellInches = 12.5;
    }
    else {
        D_well = 8.50; //inches (smaller diameter)
        mo_geo_in.md_DiameterPumpCasingInches = 8.5;
        mo_geo_in.md_DiameterProductionWellInches = 8.75;
    }
    //double D_well = mo_geo_in.md_DiameterPumpCasingInches - 2 * 0.4;
    double D_well_ft = D_well / 12;
    double A = 3.1415 * pow(D_well_ft, 2) / 4;
    double L_int = 0.2 * mo_geo_in.md_ResourceDepthM; //Length interval (m), how is this calculated?
    double surf_rough_casing = 0.02; //different for open hole vs. slotted liner
    if (mo_geo_in.md_ProductionWellType == 1) surf_rough_casing = 0.001;
    double dT_dL = (DT_prod_well(mo_geo_in.md_dtProdWellChoice)) / (mo_geo_in.md_ResourceDepthM);
    double T_star = GetResourceTemperatureC() - dT_dL * (L_int) * 0.5;
    double P_sat = geothermal::oPC.evaluate(T_star * 1.8 + 32);
    double rho_sat = 1 / geothermal::oSVC.evaluate(T_star * 1.8 + 32);
    double viscosity = 407.22 * pow((T_star * 1.8 + 32), -1.194) / 3600;
    double rho_head = L_int * 3.28084 * rho_sat / 144;
    //double P_ratio = 0.5 * (rho_head + mp_geo_out->md_PressureLPFlashPSI) / P_sat;
    double P_ratio = (Prod_well_minus_bottomhole - 0.5 * rho_sat * L_int * physics::FT_PER_METER / 144) / P_sat;
    double rho_correction = 1 + (7.15037e-19 * pow(physics::CelciusToFarenheit(T_star), 5.91303)) * (P_ratio - 1);
    double mu_correction = 1 + (4.02401e-18 * pow(physics::CelciusToFarenheit(T_star), 5.736882)) * (P_ratio - 1);
    double flow_cubic = flow_lbh / (3600 * rho_sat * rho_correction);
    double vel_well = flow_cubic / A;
    double Re_well = vel_well * D_well_ft * rho_sat * rho_correction / (viscosity * mu_correction);
    double friction_factor_smooth = pow(0.79 * log(Re_well) - 1.64, -2);
    //Serghide
    double a = -2 * log10((surf_rough_casing / D_well_ft) / 3.7 + 12 / Re_well);
    double v = -2 * log10((surf_rough_casing / D_well_ft) / 3.7 + 2.51 * a / Re_well);
    double c = -2 * log10((surf_rough_casing / D_well_ft) / 3.7 + 2.51 * v / Re_well);
    double f = pow((a - pow((v - a), 2) / (c - 2 * v + a)), -2);
    double friction_head_loss = (f / D_well_ft) * pow(vel_well, 2) / (2 * 32.174);
    double friction_head_loss_ft = friction_head_loss * L_int * physics::FT_PER_METER;
    if (mo_geo_in.me_rt == EGS) friction_head_loss_ft *= 1.0 / 3.0;
    double friction_head_psid1 = friction_head_loss_ft * rho_sat * rho_correction / 144;
    double P_upper_bottom_interval = Prod_well_minus_bottomhole - rho_sat * rho_correction * physics::FT_PER_METER * L_int / 144 - friction_head_psid1;

    //Injection interval
    flow = mo_geo_in.md_ProductionFlowRateKgPerS; //kg/s
    flow_lbh = flow * 2.20462 * 3600;
    //Upper interval
    D_well = 0;
    if (mo_geo_in.md_ProductionWellDiam == 0) D_well = 12.5; //inches (larger diameter)
    else D_well = 8.75; //inches (smaller diameter)
    //D_well = mo_geo_in.md_DiameterProductionWellInches;
    D_well_ft = D_well / 12;
    A = 3.1415 * pow(D_well_ft, 2) / 4;
    L_int = 0.8 * mo_geo_in.md_ResourceDepthM; //Length interval (m), how is this calculated?
    surf_rough_casing = 0.00015; //different for open hole vs. slotted liner
    //dT_dL = (mo_geo_in.md_dtProdWell * 1.8) / (physics::FT_PER_METER * mo_geo_in.md_ResourceDepthM);
    //T_star = T_star - dT_dL * (0.5 * L_int);
    P_sat = geothermal::oPC.evaluate(T_star * 1.8 + 32);
    rho_sat = 1 / geothermal::oSVC.evaluate(T_star * 1.8 + 32);
    viscosity = 407.22 * pow((T_star * 1.8 + 32), -1.194) / 3600;
    rho_head = L_int * 3.28084 * rho_sat / 144;
    //P_ratio = (0.5 * rho_head + P_upper_bottom_interval) / P_sat;
    P_ratio = 0.5 * (P_upper_bottom_interval + P_sat) / P_sat;
    rho_correction = 1 + (7.15037e-19 * pow(physics::CelciusToFarenheit(T_star), 5.91303)) * (P_ratio - 1);
    mu_correction = 1 + (4.02401e-18 * pow(physics::CelciusToFarenheit(T_star), 5.736882)) * (P_ratio - 1);
    flow_cubic = flow_lbh / (3600 * rho_sat * rho_correction);
    vel_well = flow_cubic / A;
    double vel_well_flash = vel_well;
    Re_well = vel_well * D_well_ft * rho_sat * rho_correction / (viscosity * mu_correction);
    friction_factor_smooth = pow(0.79 * log(Re_well) - 1.64, -2);
    //Serghide
    a = -2 * log10((surf_rough_casing / D_well_ft) / 3.7 + 12 / Re_well);
    v = -2 * log10((surf_rough_casing / D_well_ft) / 3.7 + 2.51 * a / Re_well);
    c = -2 * log10((surf_rough_casing / D_well_ft) / 3.7 + 2.51 * v / Re_well);
    f = pow((a - pow((v - a), 2) / (c - 2 * v + a)), -2);
    friction_head_loss = (f / D_well_ft) * pow(vel_well, 2) / (2 * 32.174);
    friction_head_loss_ft = friction_head_loss * L_int * physics::FT_PER_METER;
    double friction_head_psid2 = friction_head_loss_ft * rho_sat * rho_correction / 144;
    double P_pump_suction = geothermal::oPC.evaluate(physics::CelciusToFarenheit(GetResourceTemperatureC() - dT_dL * GetResourceDepthM())) +  physics::mBarToPSI(mo_geo_in.md_ExcessPressureBar*1000);
    double P_available = P_upper_bottom_interval - P_pump_suction;
    double P_available_psf = P_available * 144;
    double P_available_head = P_available_psf / (rho_sat*rho_correction) / (1 + friction_head_loss);

    //Flashing in well bore
    double Pres_avail_before_flashing = P_upper_bottom_interval - P_sat;
    double head_avail = ((Pres_avail_before_flashing * 144) / (rho_sat*rho_correction)) / (1 + friction_head_loss);
    if (mo_geo_in.me_ct == FLASH) {
        friction_head_psid2 = friction_head_loss * rho_sat / 144.0 * head_avail;
    }
    else {
        friction_head_psid2 = friction_head_loss * rho_sat / 144.0 * P_available_head;
    }
    double depth_to_liner = (L_int) * physics::FT_PER_METER;
    double pump_setting = depth_to_liner - P_available_head;
    flow = mo_geo_in.md_ProductionFlowRateKgPerS; //kg/s
    flow_lbh = flow * 2.20462 * 3600;
    //Upper interval
    D_well = 0;
    if (mo_geo_in.md_ProductionWellDiam == 0) D_well = 9.625 - 0.944; //inches (larger diameter)
    else D_well = 7.00 - 0.944; //inches (smaller diameter)
    //D_well = mo_geo_in.md_DiameterPumpCasingInches - 0.944;
    D_well_ft = D_well / 12;
    A = 3.1415 * pow(D_well_ft, 2) / 4;
    L_int = 0.2 * mo_geo_in.md_ResourceDepthM; //Length interval (m), how is this calculated?
    surf_rough_casing = 0.00015; //different for open hole vs. slotted liner
    //dT_dL = (mo_geo_in.md_dtProdWell * 1.8) / (physics::FT_PER_METER * mo_geo_in.md_ResourceDepthM);
    T_star = T_star;
    P_sat = geothermal::oPC.evaluate(T_star * 1.8 + 32);
    rho_sat = 1 / geothermal::oSVC.evaluate(T_star * 1.8 + 32);
    viscosity = 407.22 * pow((T_star * 1.8 + 32), -1.194) / 3600;
    rho_head = L_int * 3.28084 * rho_sat / 144;
    P_ratio = (0.5 * rho_head + P_upper_bottom_interval) / P_sat;
    rho_correction = 1 + (7.15037e-19 * pow(physics::CelciusToFarenheit(T_star), 5.91303)) * (P_ratio - 1);
    mu_correction = 1 + (4.02401e-18 * pow(physics::CelciusToFarenheit(T_star), 5.736882)) * (P_ratio - 1);
    flow_cubic = flow_lbh / (3600 * rho_sat * rho_correction);
    vel_well = flow_cubic / A;
    Re_well = vel_well * D_well_ft * rho_sat * rho_correction / (viscosity * mu_correction);
    friction_factor_smooth = pow(0.79 * log(Re_well) - 1.64, -2);
    //Serghide
    a = -2 * log10((surf_rough_casing / D_well_ft) / 3.7 + 12 / Re_well);
    v = -2 * log10((surf_rough_casing / D_well_ft) / 3.7 + 2.51 * a / Re_well);
    c = -2 * log10((surf_rough_casing / D_well_ft) / 3.7 + 2.51 * v / Re_well);
    f = pow((a - pow((v - a), 2) / (c - 2 * v + a)), -2);
    friction_head_loss = (f / D_well_ft) * pow(vel_well, 2) / (2 * 32.174);
    friction_head_loss_ft = friction_head_loss * pump_setting;
    double friction_head_psid3 = friction_head_loss_ft * rho_sat * rho_correction / 144;
    double depth_to_flashing = depth_to_liner - head_avail;
    double density_change_a = (33.623 * exp(-0.035468 * (GetResourceTemperatureC() - DT_prod_well(mo_geo_in.md_dtProdWellChoice))));
    double density_change_b = 0.42512 * exp(0.002486 * (GetResourceTemperatureC() - DT_prod_well(mo_geo_in.md_dtProdWellChoice)));
    double density_change_c = 1.0;
    double eff_density_change = density_change_a * pow(depth_to_flashing, density_change_b) + density_change_c;
    double est_wellhead_pressure = P_sat - (depth_to_flashing * rho_sat / (eff_density_change * 144.0));
    double vel_eff = vel_well_flash * eff_density_change;
    double friction_head_loss_eff = friction_head_loss * pow(vel_eff / vel_well_flash, 2);
    double friction_head_loss_eff_ft = friction_head_loss_eff * depth_to_flashing;
    if (mo_geo_in.me_ct == FLASH) {
        friction_head_psid3 = friction_head_loss_eff_ft * rho_sat / (eff_density_change * 144.0);
    }
    /*
    else {
        friction_head_loss_ft = friction_head_loss * pump_setting;
        friction_head_psid3 = friction_head_loss_ft * rho_sat * rho_correction / 144;
    }*/
    mo_geo_in.md_ProdWellFriction = friction_head_psid1 + friction_head_psid2 + friction_head_psid3;
    double pump_lift = pump_setting + friction_head_loss_ft;
    if (pump_lift < 0.0) pump_lift = 0.0;
    double ideal_pumping_power = flow * pump_lift * 1 / physics::FT_PER_METER; //ft-lb/h
    double ideal_pumping_power_permin = ideal_pumping_power / 60; //ft-lb/min
    double ideal_pumping_power_hp = ideal_pumping_power_permin / 33000; //hp
    double pumping_power_hp = ideal_pumping_power_hp / mo_geo_in.md_GFPumpEfficiency;
    double pumping_power_ft = pumping_power_hp * (60 * 33000); //ft
    return pump_lift;
}


double CGeothermalAnalyzer::GetCalculatedPumpDepthInFeet(void)
{	// Calculate the pumpSetDepth

	// mp_geo_out->md_BottomHolePressure; // [7B.Reservoir Hydraulics].G75
	double dInectionPumpHeadUsed = 0; // [2B.Resource&Well Input].D162

	if (mo_geo_in.me_rt == EGS)
		mp_geo_out->md_BottomHolePressure = (pressureInjectionWellBottomHolePSI() + dInectionPumpHeadUsed) - GetPressureChangeAcrossReservoir();
	else
		mp_geo_out->md_BottomHolePressure = pressureHydrostaticPSI() - GetPressureChangeAcrossReservoir();

	double pressureDiff = mp_geo_out->md_BottomHolePressure - pressureWellHeadPSI();
	double dDiameterProductionWellFt = mo_geo_in.md_DiameterProductionWellInches / 12;

	double areaWell = physics::areaCircle(dDiameterProductionWellFt / 2); // ft^2
	double velocityWell = productionFlowRate() / areaWell; // [7A.GF Pumps].G70al
	double ReWell = dDiameterProductionWellFt * velocityWell * productionDensity() / productionViscosity();
	double frictionHeadLossWell = (geothermal::FrictionFactor(ReWell) / dDiameterProductionWellFt)* pow(velocityWell, 2) / (2 * physics::GRAVITY_FTS2);

	double pumpSetting = ((pressureDiff * 144) / productionDensity())*(1 - frictionHeadLossWell);   // [7A.GF Pumps].D89
	return (geothermal::MetersToFeet(GetResourceDepthM()) - pumpSetting < 0) ? 0 : geothermal::MetersToFeet(GetResourceDepthM()) - pumpSetting; // feet - [7A.GF Pumps].D90
}

double CGeothermalAnalyzer::pumpHeadFt() // ft
{	// calculate the friction head loss of the casing
	double dDiameterPumpCasingFt = (mo_geo_in.md_DiameterPumpCasingInches - 0.944) / 12;
	double areaCasing = physics::areaCircle(dDiameterPumpCasingFt / 2); // ft^2
	double velocityCasing = productionFlowRate() / areaCasing;

	double dReCasing = dDiameterPumpCasingFt * velocityCasing * productionDensity() / productionViscosity();
	double frictionHeadLossCasing = (geothermal::FrictionFactor(dReCasing) * GetCalculatedPumpDepthInFeet() / dDiameterPumpCasingFt)* pow(velocityCasing, 2) / (2 * physics::GRAVITY_FTS2); //feet

	// Add (friction head loss) and (pump Set depth) to Get total pump head.

	return frictionHeadLossCasing + GetCalculatedPumpDepthInFeet();
}

double CGeothermalAnalyzer::Gringarten()
{
    //gringarten lookup table columns
    std::vector<double> t_d_vec{0, 0.009429319, 0.018858638, 0.028287956, 0.037717275, 0.047146594, 0.056575913, 0.066005231, 0.07543455, 0.084863869, 0.094293188, 0.103722506, 0.113151825, 0.122581144, 0.132010463, 0.141439781, 0.1508691, 0.160298419, 0.169727738, 0.179157056, 0.188586375, 0.198015694, 0.207445013, 0.216874331, 0.22630365, 0.235732969, 0.245162288, 0.254591606, 0.264020925, 0.273450244, 0.282879563, 0.292308881, 0.3017382, 0.311167519, 0.320596838, 0.330026156, 0.339455475, 0.348884794, 0.358314113, 0.367743431, 0.37717275, 0.386602069, 0.396031388, 0.405460706, 0.414890025, 0.424319344, 0.433748663, 0.443177981, 0.4526073, 0.462036619, 0.471465938, 0.480895256, 0.490324575, 0.499753894, 0.509183213, 0.518612531, 0.52804185, 0.537471169, 0.546900488, 0.556329806, 0.565759125, 0.575188444, 0.584617763, 0.594047081, 0.6034764, 0.612905719, 0.622335038, 0.631764356, 0.641193675, 0.650622994, 0.660052313, 0.669481631, 0.67891095, 0.688340269, 0.697769588, 0.707198906, 0.716628225, 0.726057544, 0.735486863, 0.744916181, 0.7543455, 0.763774819, 0.773204138, 0.782633456, 0.792062775, 0.801492094, 0.810921413, 0.820350731, 0.82978005, 0.839209369, 0.848638688, 0.858068006, 0.867497325, 0.876926644, 0.886355963, 0.895785281, 0.9052146, 0.914643919, 0.924073238, 0.933502556, 0.942931875, 0.952361194, 0.961790513, 0.971219831, 0.98064915, 0.990078469, 0.999507788, 1.008937106, 1.018366425, 1.027795744, 1.037225063, 1.046654381, 1.0560837, 1.065513019, 1.074942338, 1.084371656, 1.093800975, 1.103230294, 1.112659613, 1.122088931, 1.13151825, 1.140947569, 1.150376888, 1.159806206, 1.169235525, 1.178664844, 1.188094163, 1.197523481, 1.2069528, 1.216382119, 1.225811438, 1.235240756, 1.244670075, 1.254099394, 1.263528713, 1.272958031, 1.28238735, 1.291816669, 1.301245988, 1.310675306, 1.320104625, 1.329533944, 1.338963263, 1.348392581, 1.3578219, 1.367251219, 1.376680538, 1.386109856, 1.395539175, 1.404968494, 1.414397813, 1.423827131, 1.43325645, 1.442685769, 1.452115088, 1.461544406, 1.470973725, 1.480403044, 1.489832363, 1.499261681, 1.508691, 1.518120319, 1.527549638, 1.536978956, 1.546408275, 1.555837594, 1.565266913, 1.574696231, 1.58412555, 1.593554869, 1.602984188, 1.612413506, 1.621842825, 1.631272144, 1.640701463, 1.650130781, 1.6595601, 1.668989419, 1.678418738, 1.687848056, 1.697277375, 1.706706694, 1.716136013, 1.725565331, 1.73499465, 1.744423969, 1.753853288, 1.763282606, 1.772711925, 1.782141244, 1.791570563, 1.800999881, 1.8104292, 1.819858519, 1.829287838, 1.838717156, 1.848146475, 1.857575794, 1.867005113, 1.876434431, 1.88586375, 1.895293069, 1.904722388, 1.914151706, 1.923581025, 1.933010344, 1.942439663, 1.951868981, 1.9612983, 1.970727619, 1.980156938, 1.989586256, 1.999015575, 2.008444894, 2.017874213, 2.027303531, 2.03673285, 2.046162169, 2.055591488, 2.065020806, 2.074450125, 2.083879444, 2.093308763, 2.102738081, 2.1121674, 2.121596719, 2.131026038, 2.140455356, 2.149884675, 2.159313994, 2.168743313, 2.178172631, 2.18760195, 2.197031269, 2.206460588, 2.215889906, 2.225319225, 2.234748544, 2.244177863, 2.253607181, 2.2630365, 2.272465819, 2.281895138, 2.291324456, 2.300753775, 2.310183094, 2.319612413, 2.329041731, 2.33847105, 2.347900369, 2.357329688, 2.366759006, 2.376188325, 2.385617644, 2.395046963, 2.404476281, 2.4139056, 2.423334919, 2.432764238, 2.442193556, 2.451622875, 2.461052194, 2.470481513, 2.479910831, 2.48934015, 2.498769469, 2.508198788, 2.517628106, 2.527057425, 2.536486744, 2.545916063, 2.555345381, 2.5647747, 2.574204019, 2.583633338, 2.593062656, 2.602491975, 2.611921294, 2.621350613, 2.630779931, 2.64020925, 2.649638569, 2.659067888, 2.668497206, 2.677926525, 2.687355844, 2.696785163, 2.706214481, 2.7156438, 2.725073119, 2.734502438, 2.743931756, 2.753361075, 2.762790394, 2.772219713, 2.781649031, 2.79107835, 2.800507669, 2.809936988, 2.819366306, 2.828795625, 2.838224944, 2.847654263, 2.857083581, 2.8665129, 2.875942219, 2.885371538, 2.894800856, 2.904230175, 2.913659494, 2.923088813, 2.932518131, 2.94194745, 2.951376769, 2.960806088, 2.970235406, 2.979664725, 2.989094044, 2.998523363, 3.007952681, 3.017382, 3.026811319, 3.036240638, 3.045669956, 3.055099275, 3.064528594, 3.073957913, 3.083387231, 3.09281655, 3.102245869, 3.111675188, 3.121104506, 3.130533825, 3.139963144, 3.149392463, 3.158821781, 3.1682511, 3.177680419, 3.187109738, 3.196539056, 3.205968375, 3.215397694, 3.224827013, 3.234256331, 3.24368565, 3.253114969, 3.262544288, 3.271973606, 3.281402925, 3.290832244, 3.300261563, 3.309690881, 3.3191202, 3.328549519, 3.337978838, 3.347408156, 3.356837475, 3.366266794, 3.375696113, 3.385125431, 3.39455475, 3.403984069, 3.413413388, 3.422842706, 3.432272025, 3.441701344, 3.451130663, 3.460559981, 3.4699893, 3.479418619, 3.488847938, 3.498277256, 3.507706575, 3.517135894, 3.526565213, 3.535994531, 3.54542385, 3.554853169, 3.564282488, 3.573711806, 3.583141125, 3.592570444, 3.601999763, 3.611429081, 3.6208584, 3.630287719, 3.639717038, 3.649146356, 3.658575675, 3.668004994, 3.677434313, 3.686863631, 3.69629295, 3.705722269, 3.715151588, 3.724580906, 3.734010225, 3.743439544, 3.752868863, 3.762298181, 3.7717275};
    std::vector<double> Td_xed0{ 0, 3.29E-13, 3.05E-07, 5.14E-05, 0.000991479, 0.007155967, 0.029079151, 0.080756519, 0.171091244, 0.296648635, 0.442021361, 0.586909148, 0.714249376, 0.814766974, 0.887055397, 0.934962039, 0.964490727, 0.981548887, 0.990843584, 0.995646471, 0.998010987, 0.999124482, 0.999627808, 0.999846858, 0.999938892, 0.999976309, 0.999991062, 0.999996714, 0.999998821, 0.999999586, 0.999999858, 0.999999952, 0.999999984, 0.999999995, 0.999999998, 0.999999999, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
    std::vector<double> Td_xed1{0, 3.29E-13, 2.62E-07, 2.64E-05, 0.000284942, 0.001290191, 0.003815834, 0.008806692, 0.01726484, 0.030124211, 0.04813833, 0.071796971, 0.10127921, 0.136443518, 0.176850282, 0.221809024, 0.270441246, 0.321750055, 0.374688951, 0.42822395, 0.481385187, 0.533306013, 0.58324915, 0.630620707, 0.674973616, 0.716002534, 0.753532395, 0.787502717, 0.817949543, 0.844986614, 0.868786974, 0.889565911, 0.90756581, 0.923043219, 0.936258243, 0.947466196, 0.956911349, 0.964822532, 0.971410324, 0.976865543, 0.981358777, 0.985040692, 0.988042924, 0.990479345, 0.99244757, 0.99403058, 0.99529837, 0.996309552, 0.997112874, 0.997748624, 0.998249894, 0.998643715, 0.998952044, 0.999192627, 0.999379736, 0.999524795, 0.999636907, 0.999723296, 0.999789671, 0.999840522, 0.999879374, 0.999908979, 0.999931477, 0.999948532, 0.999961428, 0.999971155, 0.999978476, 0.999983972, 0.999988089, 0.999991166, 0.999993461, 0.999995169, 0.999996438, 0.999997378, 0.999998074, 0.999998587, 0.999998966, 0.999999244, 0.999999448, 0.999999598, 0.999999708, 0.999999788, 0.999999846, 0.999999889, 0.999999919, 0.999999942, 0.999999958, 0.99999997, 0.999999978, 0.999999984, 0.999999989, 0.999999992, 0.999999994, 0.999999996, 0.999999997, 0.999999998, 0.999999999, 0.999999999, 0.999999999, 0.999999999, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    std::vector<double> Td_xed2{0, 3.29E-13, 2.62E-07, 2.62E-05, 0.000271633, 0.001127692, 0.002950835, 0.005918978, 0.010044498, 0.01524119, 0.021384578, 0.028350053, 0.036031183, 0.044344784, 0.053228885, 0.062637841, 0.072537101, 0.082898803, 0.093698548, 0.104913296, 0.116520143, 0.12849573, 0.14081604, 0.153456439, 0.166391815, 0.179596779, 0.193045859, 0.206713678, 0.220575115, 0.234605433, 0.248780384, 0.263076301, 0.277470164, 0.291939653, 0.306463198, 0.32102001, 0.335590107, 0.350154334, 0.364694381, 0.379192787, 0.393632947, 0.407999114, 0.422276394, 0.436450743, 0.450508955, 0.464438655, 0.478228282, 0.491867077, 0.505345062, 0.518653027, 0.531782505, 0.544725753, 0.557475731, 0.570026079, 0.582371091, 0.594505696, 0.606425428, 0.618126406, 0.629605309, 0.640859349, 0.651886247, 0.662684212, 0.673251911, 0.683588451, 0.69369335, 0.703566519, 0.713208236, 0.722619124, 0.731800132, 0.740752509, 0.749477789, 0.757977768, 0.766254485, 0.774310206, 0.782147401, 0.789768732, 0.797177032, 0.804375294, 0.811366651, 0.818154365, 0.824741809, 0.83113246, 0.837329879, 0.843337706, 0.849159643, 0.854799447, 0.860260918, 0.86554789, 0.870664222, 0.875613789, 0.880400474, 0.885028163, 0.889500735, 0.893822056, 0.897995974, 0.902026315, 0.905916873, 0.90967141, 0.913293651, 0.916787277, 0.920155923, 0.923403178, 0.926532577, 0.929547601, 0.932451675, 0.935248163, 0.93794037, 0.940531539, 0.943024849, 0.945423415, 0.947730283, 0.949948438, 0.952080794, 0.954130199, 0.956099433, 0.957991211, 0.959808178, 0.961552911, 0.963227922, 0.964835656, 0.966378491, 0.967858741, 0.969278653, 0.970640413, 0.97194614, 0.973197893, 0.974397671, 0.975547408, 0.976648983, 0.977704215, 0.978714866, 0.97968264, 0.980609188, 0.981496108, 0.982344943, 0.983157186, 0.983934279, 0.984677616, 0.985388542, 0.986068355, 0.98671831, 0.987339615, 0.987933437, 0.988500899, 0.989043085, 0.98956104, 0.990055769, 0.99052824, 0.990979385, 0.991410103, 0.991821255, 0.992213673, 0.992588155, 0.992945469, 0.993286352, 0.993611514, 0.993921636, 0.994217372, 0.99449935, 0.994768173, 0.99502442, 0.995268647, 0.995501385, 0.995723147, 0.995934422, 0.996135679, 0.996327369, 0.996509924, 0.996683756, 0.996849262, 0.997006821, 0.997156796, 0.997299535, 0.997435369, 0.997564618, 0.997687587, 0.997804566, 0.997915834, 0.998021658, 0.998122292, 0.99821798, 0.998308956, 0.99839544, 0.998477646, 0.998555777, 0.998630027, 0.99870058, 0.998767614, 0.998831297, 0.99889179, 0.998949247, 0.999003815, 0.999055633, 0.999104836, 0.99915155, 0.999195896, 0.999237991, 0.999277944, 0.999315862, 0.999351843, 0.999385985, 0.999418376, 0.999449105, 0.999478254, 0.999505902, 0.999532122, 0.999556988, 0.999580565, 0.99960292, 0.999624114, 0.999644204, 0.999663248, 0.999681297, 0.999698402, 0.999714611, 0.99972997, 0.999744522, 0.999758308, 0.999771368, 0.999783738, 0.999795454, 0.99980655, 0.999817057, 0.999827006, 0.999836426, 0.999845345, 0.999853787, 0.999861779, 0.999869343, 0.999876502, 0.999883277, 0.999889688, 0.999895754, 0.999901494, 0.999906924, 0.99991206, 0.999916919, 0.999921514, 0.999925861, 0.999929971, 0.999933858, 0.999937533, 0.999941008, 0.999944293, 0.999947399, 0.999950335, 0.99995311, 0.999955733, 0.999958211, 0.999960554, 0.999962767, 0.999964859, 0.999966835, 0.999968701, 0.999970465, 0.999972131, 0.999973705, 0.999975191, 0.999976594, 0.99997792, 0.999979172, 0.999980354, 0.99998147, 0.999982523, 0.999983518, 0.999984457, 0.999985343, 0.99998618, 0.999986969, 0.999987714, 0.999988418, 0.999989081, 0.999989707, 0.999990298, 0.999990855, 0.999991381, 0.999991877, 0.999992345, 0.999992786, 0.999993203, 0.999993595, 0.999993966, 0.999994315, 0.999994644, 0.999994954, 0.999995247, 0.999995523, 0.999995783, 0.999996028, 0.99999626, 0.999996477, 0.999996683, 0.999996876, 0.999997059, 0.999997231, 0.999997393, 0.999997545, 0.999997689, 0.999997825, 0.999997952, 0.999998073, 0.999998186, 0.999998293, 0.999998393, 0.999998488, 0.999998577, 0.999998661, 0.99999874, 0.999998814, 0.999998884, 0.99999895, 0.999999013, 0.999999071, 0.999999126, 0.999999178, 0.999999227, 0.999999273, 0.999999316, 0.999999357, 0.999999395, 0.999999431, 0.999999465, 0.999999497, 0.999999527, 0.999999555, 0.999999582, 0.999999607, 0.999999631, 0.999999653, 0.999999673, 0.999999693, 0.999999712, 0.999999729, 0.999999745, 0.999999761, 0.999999775, 0.999999788, 0.999999801, 0.999999813, 0.999999825, 0.999999835, 0.999999845, 0.999999854, 0.999999863, 0.999999872, 0.999999879, 0.999999887, 0.999999894, 0.9999999, 0.999999906, 0.999999912, 0.999999917, 0.999999922, 0.999999927, 0.999999931, 0.999999936, 0.999999939, 0.999999943, 0.999999947, 0.99999995, 0.999999953, 0.999999956, 0.999999959, 0.999999961, 0.999999963, 0.999999966, 0.999999968, 0.99999997, 0.999999972, 0.999999973, 0.999999975, 0.999999977, 0.999999978, 0.999999979, 0.999999981, 0.999999982, 0.999999983, 0.999999984, 0.999999985, 0.999999986, 0.999999987, 0.999999988, 0.999999988, 0.999999989, 0.99999999, 0.99999999, 0.999999991, 0.999999992, 0.999999992, 0.999999993, 0.999999993, 0.999999993, 0.999999994, 0.999999994, 0.999999995, 0.999999995, 0.999999995, 0.999999996, 0.999999996, 0.999999996, 0.999999996};
    std::vector<double> Td_xed3{0, 3.29E-13, 2.62E-07, 2.62E-05, 0.000271633, 0.001127688, 0.002950735, 0.005917822, 0.010037312, 0.01521164, 0.021293495, 0.028122297, 0.035544165, 0.043421162, 0.051634417, 0.060084047, 0.068687584, 0.07737786, 0.086100814, 0.094813451, 0.103482034, 0.112080526, 0.120589268, 0.128993862, 0.137284232, 0.145453833, 0.153498981, 0.161418295, 0.169212222, 0.176882635, 0.184432502, 0.191865604, 0.199186299, 0.206399333, 0.213509679, 0.220522412, 0.227442601, 0.234275234, 0.241025151, 0.247697, 0.254295201, 0.260823924, 0.267287076, 0.273688289, 0.280030927, 0.286318082, 0.292552583, 0.298737008, 0.304873692, 0.310964743, 0.317012054, 0.323017316, 0.328982036, 0.334907547, 0.340795025, 0.3466455, 0.352459871, 0.358238917, 0.363983309, 0.36969362, 0.375370336, 0.381013865, 0.386624544, 0.392202651, 0.397748406, 0.403261984, 0.408743517, 0.414193101, 0.4196108, 0.424996652, 0.43035067, 0.435672849, 0.440963169, 0.446221593, 0.451448076, 0.456642563, 0.461804994, 0.466935302, 0.472033419, 0.477099272, 0.48213279, 0.487133901, 0.492102535, 0.497038623, 0.501942099, 0.506812901, 0.511650969, 0.516456249, 0.521228689, 0.525968245, 0.530674876, 0.535348545, 0.539989222, 0.544596882, 0.549171506, 0.55371308, 0.558221595, 0.562697047, 0.56713944, 0.571548781, 0.575925084, 0.580268367, 0.584578655, 0.588855976, 0.593100366, 0.597311864, 0.601490513, 0.605636365, 0.609749472, 0.613829895, 0.617877695, 0.621892941, 0.625875706, 0.629826066, 0.633744102, 0.637629898, 0.641483543, 0.64530513, 0.649094754, 0.652852517, 0.656578521, 0.660272873, 0.663935684, 0.667567066, 0.671167136, 0.674736015, 0.678273823, 0.681780687, 0.685256735, 0.688702097, 0.692116906, 0.695501299, 0.698855413, 0.702179389, 0.705473369, 0.708737498, 0.711971923, 0.715176793, 0.718352258, 0.721498471, 0.724615586, 0.727703759, 0.730763147, 0.733793909, 0.736796207, 0.7397702, 0.742716054, 0.745633932, 0.748523999, 0.751386423, 0.754221372, 0.757029013, 0.759809518, 0.762563056, 0.765289799, 0.767989919, 0.770663589, 0.773310984, 0.775932276, 0.778527642, 0.781097257, 0.783641295, 0.786159935, 0.788653351, 0.791121722, 0.793565225, 0.795984037, 0.798378336, 0.800748301, 0.803094108, 0.805415936, 0.807713964, 0.80998837, 0.812239332, 0.814467029, 0.816671637, 0.818853336, 0.821012303, 0.823148716, 0.825262753, 0.82735459, 0.829424404, 0.831472373, 0.833498672, 0.835503477, 0.837486965, 0.839449309, 0.841390686, 0.843311269, 0.845211232, 0.847090748, 0.848949991, 0.850789132, 0.852608344, 0.854407797, 0.856187662, 0.857948109, 0.859689307, 0.861411425, 0.863114631, 0.864799092, 0.866464975, 0.868112445, 0.869741669, 0.871352809, 0.872946031, 0.874521496, 0.876079367, 0.877619805, 0.879142971, 0.880649024, 0.882138124, 0.883610428, 0.885066093, 0.886505277, 0.887928133, 0.889334818, 0.890725484, 0.892100284, 0.893459371, 0.894802895, 0.896131007, 0.897443856, 0.898741589, 0.900024355, 0.901292299, 0.902545568, 0.903784305, 0.905008655, 0.90621876, 0.907414761, 0.9085968, 0.909765017, 0.910919549, 0.912060535, 0.913188112, 0.914302415, 0.91540358, 0.916491741, 0.91756703, 0.91862958, 0.919679522, 0.920716985, 0.921742099, 0.922754991, 0.92375579, 0.924744621, 0.92572161, 0.926686879, 0.927640554, 0.928582756, 0.929513606, 0.930433225, 0.931341732, 0.932239245, 0.933125882, 0.934001759, 0.934866992, 0.935721695, 0.936565982, 0.937399966, 0.938223757, 0.939037467, 0.939841206, 0.940635083, 0.941419204, 0.942193678, 0.94295861, 0.943714105, 0.944460267, 0.945197201, 0.945925007, 0.946643787, 0.947353643, 0.948054672, 0.948746975, 0.949430649, 0.95010579, 0.950772495, 0.951430859, 0.952080975, 0.952722938, 0.95335684, 0.953982772, 0.954600825, 0.955211089, 0.955813653, 0.956408606, 0.956996033, 0.957576023, 0.95814866, 0.958714029, 0.959272215, 0.9598233, 0.960367368, 0.960904498, 0.961434773, 0.961958272, 0.962475074, 0.962985258, 0.9634889, 0.963986079, 0.96447687, 0.964961348, 0.965439588, 0.965911663, 0.966377646, 0.966837611, 0.967291627, 0.967739767, 0.968182099, 0.968618694, 0.969049619, 0.969474943, 0.969894733, 0.970309056, 0.970717977, 0.97112156, 0.971519872, 0.971912974, 0.972300931, 0.972683805, 0.973061657, 0.973434548, 0.97380254, 0.97416569, 0.97452406, 0.974877706, 0.975226687, 0.97557106, 0.975910882, 0.976246209, 0.976577095, 0.976903596, 0.977225765, 0.977543657, 0.977857324, 0.978166819, 0.978472193, 0.978773498, 0.979070784, 0.979364101, 0.979653499, 0.979939027, 0.980220732, 0.980498664, 0.980772869, 0.981043394, 0.981310286, 0.981573589, 0.98183335, 0.982089613, 0.982342422, 0.982591821, 0.982837852, 0.98308056, 0.983319985, 0.98355617, 0.983789156, 0.984018983, 0.984245692, 0.984469323, 0.984689915, 0.984907506, 0.985122136, 0.985333842, 0.985542662, 0.985748633, 0.985951791, 0.986152174, 0.986349815, 0.986544752, 0.986737019, 0.98692665, 0.98711368, 0.987298142, 0.987480069, 0.987659495, 0.987836453, 0.988010973, 0.988183089, 0.988352831, 0.98852023, 0.988685318, 0.988848123, 0.989008677, 0.989167009, 0.989323147, 0.989477121, 0.989628959, 0.98977869, 0.98992634, 0.990071938, 0.99021551, 0.990357084, 0.990496685, 0.990634341, 0.990770076, 0.990903916, 0.991035886, 0.991166012};
    std::vector<double> Td_xed4{0, 3.29E-13, 2.62E-07, 2.62E-05, 0.000271633, 0.001127688, 0.002950735, 0.005917822, 0.010037312, 0.01521164, 0.021293494, 0.028122296, 0.035544157, 0.043421127, 0.051634294, 0.060083677, 0.068686615, 0.0773756, 0.086096024, 0.094804084, 0.103464926, 0.112051059, 0.120541014, 0.128918233, 0.13717016, 0.145287478, 0.153263503, 0.161093674, 0.168775146, 0.176306452, 0.18368723, 0.190918001, 0.197999985, 0.204934951, 0.211725096, 0.218372949, 0.224881283, 0.231253055, 0.23749135, 0.243599337, 0.249580234, 0.255437279, 0.261173709, 0.266792738, 0.272297546, 0.277691267, 0.282976979, 0.288157697, 0.293236372, 0.298215881, 0.303099033, 0.307888558, 0.312587116, 0.317197289, 0.321721589, 0.32616245, 0.330522239, 0.33480325, 0.339007709, 0.343137774, 0.347195538, 0.351183033, 0.355102226, 0.358955027, 0.362743286, 0.3664688, 0.370133309, 0.373738503, 0.37728602, 0.380777449, 0.384214332, 0.387598167, 0.390930405, 0.394212456, 0.397445687, 0.400631427, 0.403770964, 0.406865551, 0.409916402, 0.412924696, 0.41589158, 0.418818167, 0.421705535, 0.424554735, 0.427366785, 0.430142675, 0.432883365, 0.435589789, 0.438262854, 0.440903438, 0.443512398, 0.446090563, 0.448638739, 0.451157709, 0.453648233, 0.45611105, 0.458546875, 0.460956405, 0.463340315, 0.465699261, 0.46803388, 0.47034479, 0.472632589, 0.474897862, 0.477141172, 0.479363068, 0.481564083, 0.483744732, 0.485905517, 0.488046924, 0.490169425, 0.492273476, 0.494359522, 0.496427994, 0.498479309, 0.500513872, 0.502532076, 0.504534301, 0.506520918, 0.508492284, 0.510448746, 0.512390642, 0.514318298, 0.516232029, 0.518132144, 0.520018939, 0.521892702, 0.523753714, 0.525602244, 0.527438555, 0.529262902, 0.531075531, 0.532876682, 0.534666584, 0.536445464, 0.538213537, 0.539971016, 0.541718103, 0.543454998, 0.545181891, 0.546898968, 0.548606409, 0.550304388, 0.551993075, 0.553672633, 0.555343221, 0.557004992, 0.558658096, 0.560302676, 0.561938873, 0.563566821, 0.565186653, 0.566798495, 0.568402471, 0.569998701, 0.571587299, 0.57316838, 0.57474205, 0.576308417, 0.577867581, 0.579419643, 0.580964698, 0.58250284, 0.584034159, 0.585558742, 0.587076675, 0.58858804, 0.590092918, 0.591591385, 0.593083517, 0.594569387, 0.596049066, 0.597522623, 0.598990125, 0.600451636, 0.60190722, 0.603356936, 0.604800846, 0.606239007, 0.607671474, 0.609098302, 0.610519545, 0.611935253, 0.613345476, 0.614750263, 0.616149662, 0.617543718, 0.618932477, 0.620315981, 0.621694273, 0.623067395, 0.624435386, 0.625798286, 0.627156133, 0.628508964, 0.629856815, 0.631199722, 0.632537719, 0.633870839, 0.635199116, 0.636522581, 0.637841265, 0.6391552, 0.640464414, 0.641768937, 0.643068797, 0.644364022, 0.64565464, 0.646940676, 0.648222157, 0.649499108, 0.650771554, 0.65203952, 0.653303029, 0.654562104, 0.655816769, 0.657067047, 0.658312958, 0.659554525, 0.660791769, 0.662024712, 0.663253372, 0.664477771, 0.665697928, 0.666913862, 0.668125593, 0.669333139, 0.67053652, 0.671735752, 0.672930854, 0.674121843, 0.675308738, 0.676491554, 0.677670309, 0.678845019, 0.6800157, 0.68118237, 0.682345043, 0.683503736, 0.684658463, 0.685809241, 0.686956084, 0.688099007, 0.689238025, 0.690373153, 0.691504405, 0.692631795, 0.693755338, 0.694875047, 0.695990937, 0.69710302, 0.698211311, 0.699315822, 0.700416568, 0.701513561, 0.702606815, 0.703696342, 0.704782155, 0.705864267, 0.70694269, 0.708017437, 0.709088521, 0.710155953, 0.711219746, 0.712279913, 0.713336464, 0.714389412, 0.71543877, 0.716484548, 0.717526758, 0.718565412, 0.719600522, 0.7206321, 0.721660155, 0.722684701, 0.723705748, 0.724723307, 0.72573739, 0.726748007, 0.727755171, 0.728758891, 0.729759178, 0.730756045, 0.7317495, 0.732739556, 0.733726223, 0.734709511, 0.735689431, 0.736665994, 0.73763921, 0.73860909, 0.739575644, 0.740538882, 0.741498816, 0.742455454, 0.743408808, 0.744358888, 0.745305704, 0.746249265, 0.747189583, 0.748126667, 0.749060527, 0.749991173, 0.750918616, 0.751842865, 0.752763929, 0.75368182, 0.754596546, 0.755508117, 0.756416544, 0.757321835, 0.758224002, 0.759123052, 0.760018997, 0.760911845, 0.761801606, 0.76268829, 0.763571906, 0.764452463, 0.765329972, 0.766204441, 0.76707588, 0.767944298, 0.768809705, 0.76967211, 0.770531522, 0.77138795, 0.772241405, 0.773091894, 0.773939427, 0.774784014, 0.775625663, 0.776464383, 0.777300184, 0.778133075, 0.778963064, 0.779790161, 0.780614375, 0.781435715, 0.782254189, 0.783069806, 0.783882576, 0.784692507, 0.785499608, 0.786303888, 0.787105356, 0.78790402, 0.788699889, 0.789492972, 0.790283278, 0.791070815, 0.791855592, 0.792637618, 0.793416901, 0.794193449, 0.794967272, 0.795738378, 0.796506775, 0.797272473, 0.798035478, 0.7987958, 0.799553448, 0.800308429, 0.801060752, 0.801810426, 0.802557459, 0.803301858, 0.804043633, 0.804782792, 0.805519343, 0.806253294, 0.806984653, 0.807713429, 0.808439629, 0.809163263, 0.809884337, 0.810602861, 0.811318842, 0.812032289, 0.812743208, 0.81345161, 0.8141575, 0.814860888, 0.815561782, 0.816260188, 0.816956116, 0.817649574, 0.818340568, 0.819029107, 0.819715199, 0.820398851, 0.821080072, 0.821758869, 0.82243525, 0.823109222, 0.823780794, 0.824449973, 0.825116767, 0.825781183, 0.826443229, 0.827102913};
    std::vector<double> Td_xed5{0, 3.29E-13, 2.62E-07, 2.62E-05, 0.000271633, 0.001127688, 0.002950735, 0.005917822, 0.010037312, 0.01521164, 0.021293494, 0.028122296, 0.035544157, 0.043421127, 0.051634294, 0.060083677, 0.068686615, 0.0773756, 0.086096024, 0.094804084, 0.103464926, 0.112051059, 0.120541014, 0.128918233, 0.13717016, 0.145287478, 0.153263503, 0.161093674, 0.168775146, 0.176306451, 0.18368723, 0.190918, 0.197999983, 0.204934947, 0.211725089, 0.218372937, 0.224881263, 0.231253023, 0.2374913, 0.24359926, 0.249580117, 0.255437106, 0.261173458, 0.26679238, 0.272297044, 0.277690573, 0.282976033, 0.288156425, 0.293234683, 0.298213666, 0.303096159, 0.307884868, 0.312582423, 0.317191378, 0.321714205, 0.326153305, 0.330511, 0.334789539, 0.338991099, 0.343117785, 0.347171636, 0.351154621, 0.355068646, 0.358915553, 0.362697124, 0.366415083, 0.370071094, 0.373666768, 0.377203663, 0.380683286, 0.384107093, 0.387476494, 0.390792853, 0.394057488, 0.397271677, 0.400436655, 0.403553618, 0.406623724, 0.409648095, 0.412627816, 0.41556394, 0.418457486, 0.421309441, 0.424120763, 0.42689238, 0.429625192, 0.43232007, 0.434977863, 0.43759939, 0.440185449, 0.442736813, 0.445254232, 0.447738436, 0.450190132, 0.452610007, 0.454998731, 0.457356951, 0.459685298, 0.461984386, 0.46425481, 0.46649715, 0.46871197, 0.470899818, 0.473061228, 0.475196719, 0.477306796, 0.479391952, 0.481452665, 0.483489402, 0.485502616, 0.487492752, 0.48946024, 0.491405499, 0.493328941, 0.495230964, 0.497111957, 0.498972301, 0.500812365, 0.50263251, 0.50443309, 0.506214447, 0.507976918, 0.50972083, 0.511446502, 0.513154247, 0.514844371, 0.516517169, 0.518172934, 0.51981195, 0.521434494, 0.523040838, 0.524631246, 0.526205978, 0.527765287, 0.529309422, 0.530838624, 0.53235313, 0.533853173, 0.535338978, 0.536810768, 0.538268761, 0.539713167, 0.541144196, 0.542562052, 0.543966932, 0.545359033, 0.546738545, 0.548105657, 0.54946055, 0.550803405, 0.552134397, 0.5534537, 0.554761481, 0.556057907, 0.55734314, 0.558617338, 0.559880659, 0.561133254, 0.562375274, 0.563606865, 0.564828173, 0.566039338, 0.567240499, 0.568431792, 0.569613351, 0.570785307, 0.571947789, 0.573100923, 0.574244832, 0.575379639, 0.576505463, 0.577622422, 0.578730629, 0.579830199, 0.580921242, 0.582003868, 0.583078184, 0.584144294, 0.585202303, 0.586252312, 0.587294421, 0.588328728, 0.589355329, 0.590374319, 0.591385792, 0.592389839, 0.593386549, 0.594376012, 0.595358315, 0.596333543, 0.59730178, 0.598263109, 0.599217612, 0.600165368, 0.601106457, 0.602040956, 0.602968941, 0.603890488, 0.604805669, 0.605714559, 0.606617228, 0.607513747, 0.608404185, 0.60928861, 0.61016709, 0.61103969, 0.611906477, 0.612767513, 0.613622862, 0.614472586, 0.615316746, 0.616155403, 0.616988616, 0.617816444, 0.618638944, 0.619456173, 0.620268187, 0.621075041, 0.62187679, 0.622673486, 0.623465183, 0.624251933, 0.625033786, 0.625810794, 0.626583007, 0.627350472, 0.62811324, 0.628871356, 0.629624869, 0.630373825, 0.63111827, 0.631858248, 0.632593804, 0.633324982, 0.634051826, 0.634774377, 0.635492679, 0.636206772, 0.636916698, 0.637622497, 0.638324208, 0.639021872, 0.639715527, 0.640405211, 0.641090962, 0.641772818, 0.642450815, 0.64312499, 0.643795379, 0.644462016, 0.645124937, 0.645784177, 0.64643977, 0.647091748, 0.647740146, 0.648384997, 0.649026332, 0.649664183, 0.650298583, 0.650929562, 0.651557151, 0.652181381, 0.652802282, 0.653419883, 0.654034214, 0.654645304, 0.655253181, 0.655857873, 0.656459409, 0.657057817, 0.657653122, 0.658245354, 0.658834537, 0.659420699, 0.660003865, 0.660584061, 0.661161313, 0.661735645, 0.662307083, 0.662875651, 0.663441374, 0.664004274, 0.664564377, 0.665121705, 0.665676282, 0.666228131, 0.666777274, 0.667323733, 0.667867531, 0.66840869, 0.668947232, 0.669483177, 0.670016548, 0.670547364, 0.671075647, 0.671601418, 0.672124696, 0.672645502, 0.673163855, 0.673679776, 0.674193283, 0.674704396, 0.675213133, 0.675719515, 0.676223559, 0.676725283, 0.677224707, 0.677721848, 0.678216723, 0.678709351, 0.67919975, 0.679687935, 0.680173926, 0.680657737, 0.681139387, 0.681618892, 0.682096269, 0.682571533, 0.683044701, 0.683515788, 0.683984811, 0.684451786, 0.684916727, 0.68537965, 0.68584057, 0.686299503, 0.686756462, 0.687211464, 0.687664522, 0.688115651, 0.688564866, 0.689012179, 0.689457607, 0.689901162, 0.690342857, 0.690782708, 0.691220728, 0.691656929, 0.692091325, 0.692523929, 0.692954754, 0.693383814, 0.69381112, 0.694236686, 0.694660524, 0.695082646, 0.695503065, 0.695921794, 0.696338843, 0.696754225, 0.697167952, 0.697580036, 0.697990489, 0.698399321, 0.698806544, 0.69921217, 0.69961621, 0.700018675, 0.700419576, 0.700818924, 0.701216729, 0.701613004, 0.702007757, 0.702401001, 0.702792745, 0.703182999, 0.703571774, 0.70395908, 0.704344928, 0.704729327, 0.705112287, 0.705493818, 0.705873929, 0.706252632, 0.706629934, 0.707005846, 0.707380376, 0.707753535, 0.708125332, 0.708495776, 0.708864875, 0.70923264, 0.709599078, 0.709964199, 0.710328012, 0.710690525, 0.711051748, 0.711411688, 0.711770354, 0.712127754, 0.712483898, 0.712838793, 0.713192448, 0.71354487, 0.713896068, 0.714246051, 0.714594825, 0.714942399, 0.71528878, 0.715633977, 0.715977998};
    std::vector<double> Td_xed6{0, 3.29E-13, 2.62E-07, 2.62E-05, 0.000271633, 0.001127688, 0.002950735, 0.005917822, 0.010037312, 0.01521164, 0.021293494, 0.028122296, 0.035544157, 0.043421127, 0.051634294, 0.060083677, 0.068686615, 0.0773756, 0.086096024, 0.094804084, 0.103464926, 0.112051059, 0.120541014, 0.128918233, 0.13717016, 0.145287478, 0.153263503, 0.161093674, 0.168775146, 0.176306451, 0.18368723, 0.190918, 0.197999983, 0.204934947, 0.211725089, 0.218372937, 0.224881263, 0.231253023, 0.2374913, 0.24359926, 0.249580117, 0.255437106, 0.261173458, 0.26679238, 0.272297044, 0.277690573, 0.282976033, 0.288156425, 0.293234683, 0.298213666, 0.303096159, 0.307884868, 0.312582423, 0.317191378, 0.321714205, 0.326153305, 0.330511, 0.334789539, 0.338991099, 0.343117785, 0.347171636, 0.351154621, 0.355068646, 0.358915553, 0.362697124, 0.366415083, 0.370071094, 0.373666768, 0.377203663, 0.380683286, 0.384107093, 0.387476494, 0.390792853, 0.394057488, 0.397271677, 0.400436655, 0.403553618, 0.406623724, 0.409648095, 0.412627816, 0.41556394, 0.418457486, 0.421309441, 0.424120763, 0.42689238, 0.429625192, 0.43232007, 0.434977863, 0.43759939, 0.440185449, 0.442736813, 0.445254232, 0.447738436, 0.450190132, 0.452610007, 0.454998731, 0.457356951, 0.459685298, 0.461984386, 0.46425481, 0.46649715, 0.46871197, 0.470899818, 0.473061228, 0.475196719, 0.477306796, 0.479391952, 0.481452665, 0.483489402, 0.485502616, 0.487492752, 0.48946024, 0.491405499, 0.493328941, 0.495230964, 0.497111957, 0.498972301, 0.500812365, 0.50263251, 0.50443309, 0.506214447, 0.507976918, 0.50972083, 0.511446502, 0.513154247, 0.514844371, 0.516517169, 0.518172934, 0.51981195, 0.521434494, 0.523040838, 0.524631246, 0.526205978, 0.527765287, 0.529309422, 0.530838624, 0.53235313, 0.533853172, 0.535338978, 0.536810768, 0.538268761, 0.539713167, 0.541144196, 0.542562051, 0.543966932, 0.545359033, 0.546738545, 0.548105656, 0.549460549, 0.550803404, 0.552134397, 0.553453699, 0.554761481, 0.556057906, 0.557343139, 0.558617337, 0.559880658, 0.561133253, 0.562375272, 0.563606864, 0.564828171, 0.566039335, 0.567240496, 0.568431789, 0.569613348, 0.570785304, 0.571947786, 0.573100919, 0.574244828, 0.575379634, 0.576505458, 0.577622415, 0.578730622, 0.579830191, 0.580921234, 0.582003859, 0.583078173, 0.584144283, 0.58520229, 0.586252298, 0.587294405, 0.588328711, 0.58935531, 0.590374299, 0.591385769, 0.592389814, 0.593386522, 0.594375982, 0.595358282, 0.596333507, 0.597301741, 0.598263066, 0.599217565, 0.600165318, 0.601106402, 0.602040896, 0.602968877, 0.603890418, 0.604805594, 0.605714477, 0.606617139, 0.607513651, 0.608404081, 0.609288498, 0.610166969, 0.611039561, 0.611906337, 0.612767362, 0.6136227, 0.614472412, 0.61531656, 0.616155204, 0.616988403, 0.617816215, 0.618638699, 0.619455911, 0.620267907, 0.621074742, 0.62187647, 0.622673145, 0.623464819, 0.624251545, 0.625033373, 0.625810355, 0.626582539, 0.627349975, 0.628112711, 0.628870794, 0.629624273, 0.630373192, 0.631117599, 0.631857537, 0.632593051, 0.633324185, 0.634050982, 0.634773484, 0.635491735, 0.636205774, 0.636915644, 0.637621383, 0.638323033, 0.639020633, 0.63971422, 0.640403834, 0.641089512, 0.641771291, 0.642449208, 0.643123299, 0.6437936, 0.644460147, 0.645122973, 0.645782114, 0.646437603, 0.647089475, 0.647737761, 0.648382495, 0.649023709, 0.649661435, 0.650295704, 0.650926548, 0.651553996, 0.652178079, 0.652798828, 0.653416272, 0.654030439, 0.654641359, 0.65524906, 0.65585357, 0.656454917, 0.657053129, 0.657648232, 0.658240253, 0.65882922, 0.659415157, 0.659998091, 0.660578047, 0.66115505, 0.661729126, 0.662300299, 0.662868593, 0.663434033, 0.663996642, 0.664556443, 0.665113461, 0.665667717, 0.666219235, 0.666768037, 0.667314145, 0.667857581, 0.668398367, 0.668936524, 0.669472074, 0.670005037, 0.670535434, 0.671063285, 0.671588612, 0.672111433, 0.672631769, 0.673149639, 0.673665063, 0.67417806, 0.674688649, 0.675196848, 0.675702676, 0.676206152, 0.676707293, 0.677206118, 0.677702645, 0.67819689, 0.678688871, 0.679178606, 0.679666112, 0.680151405, 0.680634501, 0.681115419, 0.681594173, 0.68207078, 0.682545256, 0.683017617, 0.683487878, 0.683956055, 0.684422163, 0.684886218, 0.685348234, 0.685808227, 0.68626621, 0.686722199, 0.687176208, 0.687628251, 0.688078342, 0.688526496, 0.688972726, 0.689417046, 0.689859469, 0.690300009, 0.69073868, 0.691175494, 0.691610464, 0.692043603, 0.692474925, 0.692904441, 0.693332165, 0.693758109, 0.694182284, 0.694604704, 0.69502538, 0.695444325, 0.695861549, 0.696277065, 0.696690885, 0.69710302, 0.697513481, 0.697922279, 0.698329426, 0.698734934, 0.699138812, 0.699541071, 0.699941723, 0.700340779, 0.700738247, 0.701134141, 0.701528468, 0.701921241, 0.702312468, 0.702702161, 0.703090328, 0.703476981, 0.703862128, 0.70424578, 0.704627946, 0.705008636, 0.705387859, 0.705765625, 0.706141943, 0.706516822, 0.706890271, 0.707262299, 0.707632916, 0.70800213, 0.70836995, 0.708736384, 0.709101442, 0.709465132, 0.709827463, 0.710188442, 0.710548079, 0.710906382, 0.711263358, 0.711619016, 0.711973365, 0.712326411, 0.712678164, 0.71302863, 0.713377819, 0.713725736, 0.714072391, 0.714417791, 0.714761943, 0.715104854, 0.715446533, 0.715786987};


    double p_w = geothermal::EGSWaterDensity(EGSAverageWaterTemperatureC2()); //density of water kg/m3
    double c_w = geothermal::EGSSpecificHeat(EGSAverageWaterTemperatureC2()); //water specific heat capacity (J/kg-K)
    double k_r = mo_geo_in.md_EGSThermalConductivity; //rock thermal conductivity
    double c_r = mo_geo_in.md_EGSSpecificHeatConstant; //rock specific heat capacity;
    double p_r = mo_geo_in.md_EGSRockDensity; //rock density
    double Q = EGSFlowPerFracture(EGSAverageWaterTemperatureC2()); // volumetric flow per fracture
    double h_f = mo_geo_in.md_EGSFractureLength; // fracture height (m)
    double w_f = mo_geo_in.md_EGSFractureWidthM; //fracture width (m)
    double x_e = mo_geo_in.md_EGSFractureSpacing; //fracture spacing (m)
    double t = mp_geo_out->ElapsedHours * 3600; //elapsed time (s)

    //dimensionless fracture spacing
    double x_ED = ((p_w * c_w) / (2 * k_r)) * (Q / (h_f * w_f)) * x_e;
    double T_D = 0; //dimensionless temperature
    double t_it = 0;
    double td_x1y1 = 0;
    double td_x2y1 = 0;
    double td_x1y2 = 0;
    double td_x2y2 = 0;
    double x1 = 0;
    double x2 = 0;
    double y1 = 0;
    double y2 = 0;
    double y = x_ED;
    int i = 0;
    //dimensionless time
    double t_D = (pow(p_w * c_w, 2) / (4 * k_r * p_r * c_r)) * pow(Q / (h_f * w_f), 2) * t;
    double x = t_D;
    if (t_D < t_d_vec[0]) {
        //T_D = 1.0;
        T_D = 0.0;
    }
    else if (t_D > t_d_vec[t_d_vec.size() - 1]) {
        //T_D = 1.0;
        T_D = 0.0;
    }
    else {
        while (t_it <= 0) {
            t_it = t_d_vec[i] - t_D;
            i++;
        }
        x2 = t_d_vec[i];
        x1 = t_d_vec[i - 1];
        if (x_ED <= 0.1) {
            td_x1y1 = Td_xed0[i - 1];
            td_x2y1 = Td_xed0[i];
            td_x1y1 = Td_xed0[i - 1];
            td_x2y2 = Td_xed0[i];
            y1 = 0.1;
            y2 = 0.1;
        }
        else if (x_ED > 0.1 && x_ED <= 0.2) {
            td_x1y1 = Td_xed0[i - 1];
            td_x2y1 = Td_xed0[i];
            td_x1y1 = Td_xed1[i - 1];
            td_x2y2 = Td_xed1[i];
            y1 = 0.1;
            y2 = 0.2;
        }
        else if (x_ED > 0.2 && x_ED <= 0.5) {
            td_x1y1 = Td_xed1[i - 1];
            td_x2y1 = Td_xed1[i];
            td_x1y1 = Td_xed2[i - 1];
            td_x2y2 = Td_xed2[i];
            y1 = 0.2;
            y2 = 0.5;
        }
        else if (x_ED > 0.5 && x_ED <= 1) {
            td_x1y1 = Td_xed2[i - 1];
            td_x2y1 = Td_xed2[i];
            td_x1y1 = Td_xed3[i - 1];
            td_x2y2 = Td_xed3[i];
            y1 = 0.5;
            y2 = 1.0;
        }
        else if (x_ED > 1 && x_ED <= 2) {
            td_x1y1 = Td_xed3[i - 1];
            td_x2y1 = Td_xed3[i];
            td_x1y1 = Td_xed4[i - 1];
            td_x2y2 = Td_xed4[i];
            y1 = 1.0;
            y2 = 2.0;
        }
        else if (x_ED > 2 && x_ED <= 5) {
            td_x1y1 = Td_xed4[i - 1];
            td_x2y1 = Td_xed4[i];
            td_x1y1 = Td_xed5[i - 1];
            td_x2y2 = Td_xed5[i];
            y1 = 2.0;
            y2 = 5.0;
        }
        else {
            td_x1y1 = Td_xed5[i - 1];
            td_x2y1 = Td_xed5[i];
            td_x1y1 = Td_xed5[i - 1];
            td_x2y2 = Td_xed5[i];
            y1 = 5.0;
            y2 = 5.0;
        }
        if (y2 != y1 && x2 != x1) {
            T_D = ((y2 - y) / (y2 - y1)) * ((x2 - x) / (x2 - x1) * td_x1y1 + (x - x1) / (x2 - x1) * td_x2y1) + ((y - y1) / (y2 - y1)) * ((x2 - x) / (x2 - x1) * td_x1y2 + (x - x1) / (x2 - x1) * td_x2y2);
        }
        else {
            //T_D = 1.0;
            T_D = 0.0;
        }
    }

    double working_temp = GetResourceTemperatureC() - T_D * (GetResourceTemperatureC() - geothermal::TEMPERATURE_EGS_INJECTIONC);
    return working_temp;
    

}

double CGeothermalAnalyzer::RameyWellbore()
{
    double alpharock = (mo_geo_in.md_EGSThermalConductivity) / (mo_geo_in.md_EGSRockDensity * mo_geo_in.md_EGSSpecificHeatConstant);
    double time = 0;
    double working_temp = md_WorkingTemperatureC;
    if (mp_geo_out->ElapsedHours < 0.1) {
        time = 8760 * 3600;
        working_temp = GetResourceTemperatureC();
    }
    else {
        time = mp_geo_out->ElapsedHours * 3600; //elapsed time (s)
        working_temp = md_WorkingTemperatureC;
    }
    double utilfactor = 1.0; //capacity factor?
    double avg_gradient = (GetResourceTemperatureC() - 11.6) / GetResourceDepthM(); //local average geothermal gradient
    double framey = -1.0 * log(1.1 * (0.3048 * (mo_geo_in.md_DiameterPumpCasingInches / (2 * 12)) / sqrt(4.0 * alpharock * time * utilfactor))) - 0.29;
    double c_w = geothermal::EGSSpecificHeat(working_temp);
    double rameyA = mo_geo_in.md_ProductionFlowRateKgPerS * c_w * framey / (2 * physics::PI * mo_geo_in.md_EGSThermalConductivity);
    double ProdTempDrop = -1.0 * ((GetResourceTemperatureC() - working_temp) - avg_gradient * (GetResourceDepthM() - rameyA) + (working_temp - avg_gradient * rameyA - GetResourceTemperatureC()) * exp(-GetResourceDepthM() / rameyA));
    return (isfinite(ProdTempDrop)) ? ProdTempDrop : 0.0;
}

double CGeothermalAnalyzer::DT_prod_well(double prod_well_choice)
{
    if (prod_well_choice == 1)
        return RameyWellbore();
    else
        return mo_geo_in.md_dtProdWell;
}



void CGeothermalAnalyzer::ReplaceReservoir(double dElapsedTimeInYears)
{
	mi_ReservoirReplacements++;
	md_WorkingTemperatureC = GetResourceTemperatureC();

	if (me_makeup == MA_EGS_FLASH || me_makeup == MA_EGS_BINARY)
	{	// have to keep track of the last temperature of the working fluid, and the last time the reservoir was "replaced" (re-drilled)
		md_LastProductionTemperatureC = md_WorkingTemperatureC;
		double dYearsAtNextTimeStep = dElapsedTimeInYears + (1.0 / 12.0);
		if (dElapsedTimeInYears > 0) md_TimeOfLastReservoirReplacement = dYearsAtNextTimeStep - (EGSTimeStar(EGSAverageWaterTemperatureC2()) / geothermal::DAYS_PER_YEAR);
	}
}

double CGeothermalAnalyzer::GetTemperatureGradient(void) // degrees C per km
{	// Conversation with Chad on August 30th 2010, 10am MT - just use the average gradient, even if it's changing at that point according to the depth/temp graph.
	if (mo_geo_in.me_rt == HYDROTHERMAL) { return ((mo_geo_in.md_TemperatureResourceC - GetAmbientTemperatureC(BINARY)) / mo_geo_in.md_ResourceDepthM) * 1000; }
	return ((mo_geo_in.md_TemperatureResourceC - GetAmbientTemperatureC(BINARY)) / mo_geo_in.md_ResourceDepthM) * 1000;
}

double CGeothermalAnalyzer::GetResourceTemperatureC(void) // degrees C
{
	if ((mo_geo_in.me_rt == EGS) && (mo_geo_in.me_dc == DEPTH)) return ((mo_geo_in.md_ResourceDepthM / 1000) * GetTemperatureGradient()) + GetAmbientTemperatureC(BINARY);
	return mo_geo_in.md_TemperatureResourceC;
}

double CGeothermalAnalyzer::GetTemperaturePlantDesignC(void) { return (mo_geo_in.me_rt == EGS) ? mo_geo_in.md_TemperaturePlantDesignC : (GetResourceTemperatureC()); }

double CGeothermalAnalyzer::GetResourceDepthM(void) // meters
{
	if ((mo_geo_in.me_rt == EGS) && (mo_geo_in.me_dc == TEMPERATURE)) return 1000 * (mo_geo_in.md_TemperatureResourceC - GetAmbientTemperatureC(BINARY)) / GetTemperatureGradient();
	return mo_geo_in.md_ResourceDepthM;
}

double CGeothermalAnalyzer::GetAmbientTemperatureC(conversionTypes ct)
{
	if (ct == NO_CONVERSION_TYPE) ct = mo_geo_in.me_ct;
	//return (ct == BINARY) ? geothermal::DEFAULT_AMBIENT_TEMPC_BINARY : (1.3842 * physics::FarenheitToCelcius(TemperatureWetBulbF())) + 5.1772 ;
	//return (ct == BINARY) ? geothermal::DEFAULT_AMBIENT_TEMPC_BINARY : physics::FarenheitToCelcius(TemperatureWetBulbF());
    return physics::FarenheitToCelcius(TemperatureWetBulbF());
}

double CGeothermalAnalyzer::InjectionTemperatureC() // calculate injection temperature in degrees C
{	// Plant design temp AND resource temp have to be Set correctly!!!
	// These are the calculations done at the bottom of [10B.GeoFluid] with the result in D89

	// this is used in pump Power calculations, and in EGS energy produciton calculations
	if ((GetTemperaturePlantDesignC() != GetResourceTemperatureC()) && (mo_geo_in.me_rt != EGS))
	{
		ms_ErrorString = ("Resource temperature was not equal to plant design temp in non-EGS analysis in CGeoHourlyBaseInputs::InjectionTemperatureC");
		return 0;
	}
    if (me_makeup == MA_BINARY) {
        //double a = (-0.000655 * GetTemperaturePlantDesignC()) + 1.01964;
        double a = 1;
        //double b = (-0.00244 * GetTemperaturePlantDesignC()) - 0.0567;
        double b = (-0.002954 * GetTemperaturePlantDesignC() - 0.121503);
        double dPlantBrineEffectiveness = (geothermal::IMITATE_GETEM) ? 10.35 : GetPlantBrineEffectiveness();
        double eff = dPlantBrineEffectiveness / GetAEBinary(); //available energy based on resource temp


        //double tr = a * exp(b*eff);
        double tr = 1 + (b * eff);
        double t1 = physics::KelvinToCelcius(physics::CelciusToKelvin(GetTemperaturePlantDesignC()) * tr);
        double t2 = GetAmbientTemperatureC() + 27;

        double x = geothermal::evaluatePolynomial(GetTemperaturePlantDesignC(), 4.205944351495, 0.3672417729236, -0.0036294799613, 0.0000706584462, -0.0000001334837, 0, 0);
        double x1 = geothermal::evaluatePolynomial(x, -0.294394, 0.307616, -0.000119669, -0.00000000425191, 0.0000000000249634, 0, 0);

        double t3 = physics::FarenheitToCelcius(physics::CelciusToFarenheit(x1) + 1);
        double y = (t1 > t2) ? t1 : t2;

        return ((t3 > y) ? t3 : y);
    }
    else { //Flash
        double p_flash_out = mp_geo_out->md_PressureLPFlashPSI;
        double T_inj = physics::FarenheitToCelcius(geothermal::evaluatePolynomial(p_flash_out, 134.575, 7.7497, -0.226287, 0.00456759, -5.4475e-5, 3.4638e-7, -9.0287e-10));
        double flash_steam_frac = (turbine1Steam() + turbine2Steam()) / geothermal::GEOTHERMAL_FLUID_FOR_FLASH;
        double x = geothermal::evaluatePolynomial(GetTemperaturePlantDesignC(), 4.205944351495, 0.3672417729236, -0.0036294799613, 0.0000706584462, -0.0000001334837, 0, 0);
        double sio2 = x / (1 - flash_steam_frac);
        double x1 = geothermal::evaluatePolynomial(sio2, -0.294394, 0.307616, -0.000119669, -0.00000000425191, 0.0000000000249634, 0, 0);
        double P = geothermal::oPC.evaluate(x1 * 1.8 + 32);
        double flash_T_limit = 0.897 * (GetTemperaturePlantDesignC() + 273) - 57 - 273;
        return (T_inj > x1) ? T_inj : ((x1 > flash_T_limit) ? x1 : flash_T_limit);
    }
}





double CGeothermalAnalyzer::InjectionTemperatureF(void)
{
	double dInjectionTempForResource = InjectionTemperatureC();	// D15 - degrees C

	return physics::CelciusToFarenheit(dInjectionTempForResource);	// G15 - degrees F
}

double CGeothermalAnalyzer::InjectionDensity(void) { return (1 / geothermal::oSVC.evaluate(InjectionTemperatureF())); }											// G19,G44, G128 - lb/ft^3


double CGeothermalAnalyzer::GetAEAtTemp(double tempC) { return (mo_geo_in.me_ct == BINARY) ? GetAEBinaryAtTemp(tempC) : GetAEFlashAtTemp(tempC); }
double CGeothermalAnalyzer::GetAEBinaryAtTemp(double tempC) { return geothermal::oGFC.GetAEForBinaryWattHrUsingC(tempC, GetAmbientTemperatureC()); }	// watt-hr/lb - Calculate available energy using binary constants and plant design temp (short cut)
double CGeothermalAnalyzer::GetAEFlashAtTemp(double tempC) { return geothermal::oGFC.GetAEForFlashWattHrUsingC(tempC, GetAmbientTemperatureC()); }	// watt-hr/lb - Calculate available energy using flash constants and plant design temp (short cut)
double CGeothermalAnalyzer::GetAE(void) { return GetAEAtTemp(GetTemperaturePlantDesignC()-DT_prod_well(mo_geo_in.md_dtProdWellChoice)); }
double CGeothermalAnalyzer::GetAEBinary(void) { return GetAEBinaryAtTemp(GetTemperaturePlantDesignC()-DT_prod_well(mo_geo_in.md_dtProdWellChoice)); }// watt-hr/lb - Calculate available energy using binary constants and plant design temp (short cut)
double CGeothermalAnalyzer::GetAEFlash(void) { return GetAEFlashAtTemp(GetTemperaturePlantDesignC()); }

double CGeothermalAnalyzer::EGSTimeStar(double tempC)
{
	double dEGSFractureSurfaceArea = mo_geo_in.md_EGSFractureWidthM * mo_geo_in.md_EGSFractureLength;
	double d2 = 27 * geothermal::EGSWaterDensity(tempC) * geothermal::EGSSpecificHeat(tempC) * EGSFlowPerFracture(tempC);
	return (pow(EGSThermalConductivity() * dEGSFractureSurfaceArea / (d2), 2) / EGSAlpha()) + EGSLengthOverVelocity(tempC);
}

double CGeothermalAnalyzer::EGSAverageWaterTemperatureC2()
{	// degrees C (used in [6Bb.Makeup-EGS HX ].X35 to calc time*
	return (InjectionTemperatureC() + GetResourceTemperatureC()) / 2;
}

double CGeothermalAnalyzer::EGSThermalConductivity()
{	// convert to J/m-hr-C for hourly analysis
	//return (IsHourly()) ? geothermal::EGS_THERMAL_CONDUCTIVITY/24 : geothermal::EGS_THERMAL_CONDUCTIVITY;
	return geothermal::EGS_THERMAL_CONDUCTIVITY;
}

double CGeothermalAnalyzer::EGSFlowPerFracture(double tempC)
{	// m^3 per day
	//double dFlowInTimePeriod = (IsHourly()) ? 60*60  : 60*60*24 ; // hourly analysis uses hourly flow, monthly analysis uses daily flow
	//double dFlowInTimePeriod = 60 * 60 * 24; // hourly analysis and monthly analyses use daily flow
    //m^3 / s
	return ((mo_geo_in.md_ProductionFlowRateKgPerS / geothermal::EGSWaterDensity(tempC)) / mo_geo_in.md_EGSNumberOfFractures);
}

double CGeothermalAnalyzer::EGSAlpha(void)
{	// fAlpha (m^2 per day) or (m^2 per hr)
	return mo_geo_in.md_EGSThermalConductivity / (mo_geo_in.md_EGSSpecificHeatConstant * mo_geo_in.md_EGSRockDensity);
}

double CGeothermalAnalyzer::EGSLengthOverVelocity(double tempC)
{
	double dEGSFractureCrossSectionArea = mo_geo_in.md_EGSFractureWidthM * mo_geo_in.md_EGSFractureAperature; // Cross Sectional Area, m^2
	double dEGSVelocity = EGSFlowPerFracture(tempC) / dEGSFractureCrossSectionArea;		// m^3 per day / m^2 = m/day (or hour)
	return mo_geo_in.md_EGSFractureLength / dEGSVelocity; 	// (m / m per day) = days (or hours)
}

double CGeothermalAnalyzer::EGSAvailableEnergy()
{	// watt-hr/lb - not sure why the flash constants are used to calc EGS available energy
	return geothermal::oGFC.GetAEForFlashWattHrUsingC(mo_geo_in.md_TemperaturePlantDesignC, GetAmbientTemperatureC(BINARY));
}

double CGeothermalAnalyzer::EGSReservoirConstant(double avgWaterTempC, double dDays)
{	// all this is from [7C.EGS Subsrfce HX], also from the calculations over time on 6Bb.Makeup-EGS HX, AF62-AF422
	double lv = EGSLengthOverVelocity(avgWaterTempC);	// days (or hours)
	if (dDays <= lv) return 0;

	double cp = geothermal::EGSSpecificHeat(avgWaterTempC);	// J/kg-C
	double rho = geothermal::EGSWaterDensity(avgWaterTempC);	// kg/m^3
	double flow = EGSFlowPerFracture(avgWaterTempC);	// m^3 per day (or per hour)
	double dEGSFractureSurfaceArea = mo_geo_in.md_EGSFractureWidthM * mo_geo_in.md_EGSFractureLength;//fFractureSurfaceArea, m^2

	double x = (EGSThermalConductivity() * dEGSFractureSurfaceArea) / (cp * rho * flow * sqrt(EGSAlpha()*(dDays - lv)));
	return geothermal::gauss_error_function(x);
}

double CGeothermalAnalyzer::GetPressureChangeAcrossReservoir()
{	//  Only used in GetCalculatedPumpDepthInFeet

	// [7B.Reservoir Hydraulics].G70
	if (mo_geo_in.me_pc == ENTER_PC) return flowRatePerWell() / mo_geo_in.md_ReservoirDeltaPressure;
	double md_PressureChangeAcrossReservoir = 0.0;

    

	// if user didn't input the pressure change, we have to calculate it.  start with these

	// Why does GETEM calculate the average water temperature for EGS two different ways?  Is one better?  Method 2 is certainly simpler.
	double dEGSAverageWaterTemperatureC1 = geothermal::calcEGSAverageWaterTemperatureC(GetResourceTemperatureC(), mo_geo_in.md_TemperaturePlantDesignC, GetPlantBrineEffectiveness() / EGSAvailableEnergy()); // degrees C (used in EGS makeup, and on [7C.EGS Subsrfce HX]
	//double EGSAverageWaterTemperatureC1 = KelvinToCelcius(CelciusToKelvin(GetResourceTemperatureC()) * calcEGSTemperatureConstant( (GetPlantBrineEffectiveness() / EGSAvailableEnergy()), md_TemperaturePlantDesignC)); // other equation used in GETEM	

	// all this is from [7C.EGS Subsrfce HX]
	double waterTempC = (geothermal::IMITATE_GETEM) ? dEGSAverageWaterTemperatureC1 : EGSAverageWaterTemperatureC2(); // degrees C
	double days = geothermal::EGS_TIME_INPUT * geothermal::DAYS_PER_YEAR;
	double tempEGSProductionC_old = GetResourceTemperatureC() + (geothermal::TEMPERATURE_EGS_INJECTIONC - GetResourceTemperatureC()) * EGSReservoirConstant(waterTempC, days);
    double tempEGSProductionCtest = Gringarten();
    double tempEGSProductionC = Gringarten();
    double dEGSAverageReservoirTemperatureF_old = physics::CelciusToFarenheit((geothermal::TEMPERATURE_EGS_INJECTIONC + tempEGSProductionC_old) / 2);

    double dEGSAverageReservoirTemperatureF = physics::CelciusToFarenheit((geothermal::TEMPERATURE_EGS_INJECTIONC + tempEGSProductionC) / 2);
    double pres_out = 0;
    double t = mp_geo_out->ElapsedHours * 3600; //elapsed time (s)
    if (mo_geo_in.me_pc == USER_TEMP) {
        int i = 0;
        while (mo_geo_in.md_ReservoirInputs.at(i, 0) < t) {
            i++;
        }
        if (i == 0) {
            tempEGSProductionC = mo_geo_in.md_ReservoirInputs.at(i, 1);
            pres_out = mo_geo_in.md_ReservoirInputs.at(i, 2) * 14.5038; //psi
        }
        else if (i > mo_geo_in.md_ReservoirInputs.nrows()) {
            tempEGSProductionC = mo_geo_in.md_ReservoirInputs.at(mo_geo_in.md_ReservoirInputs.nrows() - 1, 1);
            pres_out = mo_geo_in.md_ReservoirInputs.at(mo_geo_in.md_ReservoirInputs.nrows() - 1, 2) * 14.5038; //psi
        }
        else {
            tempEGSProductionC = (mo_geo_in.md_ReservoirInputs.at(i, 0) - t) / (mo_geo_in.md_ReservoirInputs.at(i, 0) - mo_geo_in.md_ReservoirInputs.at(i - 1, 0)) * (mo_geo_in.md_ReservoirInputs.at(i, 1) - mo_geo_in.md_ReservoirInputs.at(i - 1, 1)) + mo_geo_in.md_ReservoirInputs.at(i - 1, 1);
            pres_out = ((mo_geo_in.md_ReservoirInputs.at(i, 0) - t) / (mo_geo_in.md_ReservoirInputs.at(i, 0) - mo_geo_in.md_ReservoirInputs.at(i - 1, 0)) * (mo_geo_in.md_ReservoirInputs.at(i, 2) - mo_geo_in.md_ReservoirInputs.at(i - 1, 2)) + mo_geo_in.md_ReservoirInputs.at(i - 1, 2)) * 14.5038; //psi
        }
        dEGSAverageReservoirTemperatureF = physics::CelciusToFarenheit((geothermal::TEMPERATURE_EGS_INJECTIONC + tempEGSProductionC) / 2);
        mp_geo_out->md_AverageReservoirTemperatureF = dEGSAverageReservoirTemperatureF;
        return pres_out;
    }
    

	mp_geo_out->md_AverageReservoirTemperatureF = (mo_geo_in.me_rt == EGS) ? dEGSAverageReservoirTemperatureF : physics::CelciusToFarenheit(GetResourceTemperatureC());	// G54 on [7B.Reservoir Hydraulics]

	double density = geothermal::oDensityConstants.evaluate(mp_geo_out->md_AverageReservoirTemperatureF); // lbs per ft^3
	double volumetricFlow = (flowRatePerWell() / density) / 3600; // ft^3 per second
	double viscosity = 0.115631 * pow(mp_geo_out->md_AverageReservoirTemperatureF, -1.199532); // lb per ft-second

	if ((mo_geo_in.me_rt == EGS) && (mo_geo_in.me_pc == SIMPLE_FRACTURE || mo_geo_in.me_pc == USER_TEMP))
	{	// only a valid option for EGS resources
		// calculate the pressure change across the reservoir using simple fracture flow
		double dEGSFractureLengthUserAdjusted = mo_geo_in.md_EGSFractureLength ;
		double effectiveLengthFt = geothermal::MetersToFeet(dEGSFractureLengthUserAdjusted);
        double fracperm = pow(mo_geo_in.md_EGSFractureAperature, 2) / 12; //m^2
        double flowPerFracture = (volumetricFlow * 1 / 35.3147) / mo_geo_in.md_EGSNumberOfFractures; //m^3/s
		double fractureFlowArea = mo_geo_in.md_EGSFractureAperature * mo_geo_in.md_EGSFractureWidthM;  // m^2
        md_PressureChangeAcrossReservoir = flowPerFracture / (fractureFlowArea * fracperm) * (viscosity * 1.48816) * dEGSFractureLengthUserAdjusted * 0.000145038; //psi
        double test = md_PressureChangeAcrossReservoir;
		//double hydraulicDiameter = (2 * fractureFlowArea) / (geothermal::MetersToFeet(mo_geo_in.md_EGSFractureAperature) + geothermal::MetersToFeet(mo_geo_in.md_EGSFractureWidthM));  // ft
		//double flowPerFracture = volumetricFlow / mo_geo_in.md_EGSNumberOfFractures; // ft^3 per second
		//double velocity = flowPerFracture / fractureFlowArea; // ft per second
		//double Re = density * velocity * hydraulicDiameter / viscosity;
		//double frictionFactor = 64 / Re;
		//double headLoss = frictionFactor * (effectiveLengthFt / hydraulicDiameter) * pow(velocity, 2) / (2 * physics::GRAVITY_FTS2); // ft
		//md_PressureChangeAcrossReservoir = headLoss * density / 144; // psi
	}
	else {
		// calculate the change in pressure across reservoir using K*A (from [7B.Reservoir Hydraulics].G70)
		double dReservoirAreaSqFt = geothermal::MetersToFeet(mo_geo_in.md_ReservoirHeightM) * geothermal::MetersToFeet(mo_geo_in.md_ReservoirWidthM);
		double G53 = geothermal::M2ToFeet2(mo_geo_in.md_ReservoirPermeability * dReservoirAreaSqFt *  0.000000000000986923); //ft^4
		double G61 = volumetricFlow * viscosity * geothermal::MetersToFeet(mo_geo_in.md_DistanceBetweenProductionInjectionWellsM) / G53; // lbs per second^2-ft
		md_PressureChangeAcrossReservoir = G61 / physics::GRAVITY_FTS2 / 144; // change in pressure (psi)
	}
	return md_PressureChangeAcrossReservoir;
}

double CGeothermalAnalyzer::pressureInjectionWellBottomHolePSI() // [7B.Reservoir Hydraulics].G72, [7A.GF Pumps].G50
{
	//double injectionWellSurfacePressurePSI = (mo_geo_in.me_ct == FLASH) ? 0 : (pressureWellHeadPSI() - geothermal::PRESSURE_CHANGE_ACROSS_SURFACE_EQUIPMENT_PSI); // [2B.Resource&Well Input].D149
	double injectionWellSurfacePressurePSI = (mo_geo_in.me_ct == FLASH) ? 0 : (pressureWellHeadPSI() - mo_geo_in.md_PressureChangeAcrossSurfaceEquipmentPSI); // [2B.Resource&Well Input].D149
	// this used to incorrectly convert PSI to bar - does this still work for Binary?????????????????????
	double pMax = (pZero() > injectionWellSurfacePressurePSI) ? pZero() : injectionWellSurfacePressurePSI; //G18
	double depthFt = geothermal::MetersToFeet(GetResourceDepthM()); //G22
	double G23 = pMax + InjectionDensity() * depthFt / 144; // psi

	double flowRate = mo_geo_in.md_ProductionFlowRateKgPerS / mo_geo_in.md_RatioInjectionToProduction / (1 - mo_geo_in.md_WaterLossPercent); // kg per second
    //double flowRate = mo_geo_in.md_ProductionFlowRateKgPerS / mo_geo_in.md_RatioInjectionToProduction; // kg per second
	flowRate = geothermal::KgToLb(flowRate) / InjectionDensity();  // cf per second
	double dDiameterInjectionWellFt = mo_geo_in.md_DiameterInjectionWellInches / 12;
	double areaInjectionWell = physics::areaCircle(dDiameterInjectionWellFt / 2); // ft^2
	double velocityInjectionWell = flowRate / areaInjectionWell;

	double viscosity = 0.0925 * pow(InjectionTemperatureF(), -1.159);
	double ReInjectionWell = dDiameterInjectionWellFt * velocityInjectionWell * InjectionDensity() / viscosity;

	double frictionHeadLossInjectionWell = (geothermal::FrictionFactor(ReInjectionWell) * depthFt / dDiameterInjectionWellFt)* pow(velocityInjectionWell, 2) / (2 * physics::GRAVITY_FTS2); //feet
	double G36 = frictionHeadLossInjectionWell * InjectionDensity() / 144; // conversion to psi

	return G23 - G36; // pressureBHInjection, psi
}

double CGeothermalAnalyzer::pressureWellHeadPSI()
{
	double tempF = physics::CelciusToFarenheit(GetTemperaturePlantDesignC() - DT_prod_well(mo_geo_in.md_dtProdWellChoice));
	double pressureSaturation = geothermal::oPC.evaluate(tempF); // valid above boiling, I guess.
	//double pressureExcessPSI = geothermal::BarToPsi(geothermal::EXCESS_PRESSURE_BAR); // bar to psi
	double pressureExcessPSI = geothermal::BarToPsi(mo_geo_in.md_ExcessPressureBar); // bar to psi
	//return (GetTemperaturePlantDesignC() > 100) ? pressureSaturation + pressureExcessPSI : geothermal::PRESSURE_AMBIENT_PSI + pressureExcessPSI;
	return (GetTemperaturePlantDesignC() > 100) ? pressureSaturation + pressureExcessPSI : mo_geo_in.md_PressureAmbientPSI + pressureExcessPSI;
}

double CGeothermalAnalyzer::pressureHydrostaticPSI()
{	// calculate the hydrostatic pressure (at the bottom of the well)
	double tempAmbientF = physics::CelciusToFarenheit(11.6);
	double pressureAmbientBar = physics::PsiToBar(geothermal::oPressureAmbientConstants.evaluate(tempAmbientF));
    if (tempAmbientF <= 212) pressureAmbientBar = 1.014;
	double tempF = physics::CelciusToFarenheit(11.6);
	double densityAmbient = geothermal::LbPerCfToKgPerM3_B(geothermal::oDensityConstants.evaluate(tempF));

	double tempAmbientC = 11.6; // GETEM assumes 10 deg C ambient temperature here. Above, the assumption is 15 deg C ambient.
	double tempGradient = (mo_geo_in.me_rt == EGS) ? GetTemperatureGradient() / 1000 : (GetResourceTemperatureC() - tempAmbientC) / GetResourceDepthM();

	// hydrostatic pressure at production well depth (GetResourceDepthFt) in bar
	double d1 = densityAmbient * geothermal::GRAVITY_MS2 * geothermal::CONST_CP;
	//double d2 = (exp(d1 * (GetResourceDepthM() - (0.5 * geothermal::CONST_CT * tempGradient * pow(GetResourceDepthM(), 2)))/100000) - 1);
    double ct = 0.0009 / (30.796 * pow(GetResourceTemperatureC(), -0.552));
    double d2 = (exp(d1 * (GetResourceDepthM() - (0.5 * ct * tempGradient * pow(GetResourceDepthM(), 2))) / 100000) - 1);

	double pressureHydrostaticBar = pressureAmbientBar + (1 / geothermal::CONST_CP) * (d2);

	return geothermal::BarToPsi(pressureHydrostaticBar);
}

double CGeothermalAnalyzer::pZero(void) { return (InjectionTemperatureF() < 100 ) ? 14.7 : geothermal::oPC.evaluate(InjectionTemperatureF()); }	// G16 - psi


// wells
double CGeothermalAnalyzer::productionTempF(void) { return physics::CelciusToFarenheit(GetTemperaturePlantDesignC()); }
double CGeothermalAnalyzer::productionDensity(void) { return 1 / geothermal::oSVC.evaluate(productionTempF()); } // [7A.GF Pumps].G81; specific volume, f^3 per lb
double CGeothermalAnalyzer::productionFlowRate(void) { return (flowRatePerWell() / productionDensity()) / 3600; } // [7A.GF Pumps].G69 // lbs per hr / lbs per cf = cf/hr divided by 3600 = cfs
double CGeothermalAnalyzer::productionViscosity(void) { return 0.115631 * pow(productionTempF(), -1.199532); } // seems like this is resource temp in spreadsheet!
double CGeothermalAnalyzer::flowRatePerWell(void) { return (60 * 60 * geothermal::KgToLb(mo_geo_in.md_ProductionFlowRateKgPerS)); } // lbs per hour, one well
double CGeothermalAnalyzer::flowRateTotal(void) {
	mp_geo_out->GF_flowrate = (flowRatePerWell() * GetNumberOfWells());
    if (me_makeup == MA_BINARY && mo_geo_in.me_cb != NUMBER_OF_WELLS) {
        mp_geo_out->GF_flowrate = mo_geo_in.md_DesiredSalesCapacityKW * 1000 / (GetPlantBrineEffectiveness() - GetPumpWorkWattHrPerLb());
        return (mo_geo_in.md_DesiredSalesCapacityKW * 1000 / (GetPlantBrineEffectiveness() - GetPumpWorkWattHrPerLb()));
    }
    else {
        mp_geo_out->GF_flowrate = (flowRatePerWell() * GetNumberOfWells());
        return (flowRatePerWell() * GetNumberOfWells());
    }
}								// lbs per hour, all wells

void CGeothermalAnalyzer::WellCountDecisionTable(void)
{
    int well_stim = mo_geo_in.md_WellsStimulated; //0 - injection only, 1 - production only, 2 - both
    double production_stim_well = 0; //Column A
    double injection_stim_well = 0; //Column B
    double stim_well = 0; //Column C
    double SWDDE = mo_geo_in.md_ExplorationWellsProd;
    switch (well_stim) {
    case 0:
        if (mo_geo_in.me_rt == HYDROTHERMAL) {
            production_stim_well = SWDDE;
            injection_stim_well = 0;
        }
        else if (SWDDE >= 1.0) {
            production_stim_well = SWDDE - 1;
            injection_stim_well = 1;
        }
        else if (SWDDE > 0.0) {
            production_stim_well = 0;
            injection_stim_well = SWDDE;
        }
        else {
            production_stim_well = 0;
            injection_stim_well = 0;
        }
        break;
    case 1:
        if (mo_geo_in.me_rt == HYDROTHERMAL) {
            production_stim_well = SWDDE;
            injection_stim_well = 0;
        }
        else if (SWDDE >= 1.0) {
            production_stim_well = 1;
            injection_stim_well = SWDDE - 1;
        }
        else if (SWDDE > 0.0) {
            production_stim_well = 0;
            injection_stim_well = SWDDE;
        }
        else {
            production_stim_well = 0;
            injection_stim_well = 0;
        }
        break;
    case 2:
        if (mo_geo_in.me_rt == HYDROTHERMAL) {
            production_stim_well = SWDDE;
            injection_stim_well = 0;
        }
        else if (SWDDE > 0.0) {
            production_stim_well = SWDDE/2;
            injection_stim_well = SWDDE/2;
        }
        else {
            production_stim_well = 0;
            injection_stim_well = 0;
        }
        break;
    case 3:
        if (mo_geo_in.me_rt == HYDROTHERMAL) {
            production_stim_well = SWDDE;
            injection_stim_well = 0;
        }
        else {
            production_stim_well = 0;
            injection_stim_well = 0;
        }
    }
    mp_geo_out->ProdWellsExploration = production_stim_well;
    mp_geo_out->InjWellsExploration = injection_stim_well;
}

double CGeothermalAnalyzer::GetNumberOfWells(void)
{
    WellCountDecisionTable();
    bool inj_wells_stimulated = (mo_geo_in.md_WellsStimulated == 0 || mo_geo_in.md_WellsStimulated == 2);
    bool prod_wells_stimulated = (mo_geo_in.md_WellsStimulated == 1 || mo_geo_in.md_WellsStimulated == 2);
    if (mo_geo_in.me_cb == NUMBER_OF_WELLS) {

        mp_geo_out->md_NumberOfWells = mo_geo_in.md_NumberOfWells;
        mp_geo_out->md_NumberOfWellsInj = mo_geo_in.md_NumberOfWells / mo_geo_in.md_RatioInjectionToProduction;
    }
	else
	{
		double netBrineEffectiveness = GetPlantBrineEffectiveness() - GetPumpWorkWattHrPerLb();
		double netCapacityPerWell = flowRatePerWell() * netBrineEffectiveness / 1000.0;			// after pumping losses
        double flowPerWellInj = flowRatePerWell() / mo_geo_in.md_RatioInjectionToProduction;
		if (netCapacityPerWell == 0)
		{
			ms_ErrorString = "The well capacity was calculated to be zero.  Could not continue analysis.";
			mp_geo_out->md_NumberOfWells = 0;
		}
        mp_geo_out->md_BrineEff = GetPlantBrineEffectiveness();
        mp_geo_out->md_PumpWorkWattHrPerLb = GetPumpWorkWattHrPerLb();
		mp_geo_out->md_NumberOfWells = mo_geo_in.md_DesiredSalesCapacityKW / netCapacityPerWell;
        if (mp_geo_out->md_NumberOfWells < 0) mp_geo_out->md_NumberOfWells = 0;
        mp_geo_out->md_NumberOfWellsProdExp = (mp_geo_out->md_NumberOfWells > mp_geo_out->ProdWellsExploration) ? mp_geo_out->md_NumberOfWells - mp_geo_out->ProdWellsExploration : 0.0;
        double prod_well_stim_success_rate = (prod_wells_stimulated) ? mo_geo_in.md_StimSuccessRate : 0.0;
        mp_geo_out->md_NumberOfWellsProdDrilled = mp_geo_out->md_NumberOfWellsProdExp / (1 - (1 - prod_well_stim_success_rate) * (1 - mo_geo_in.md_DrillSuccessRate));
        double num_prod_wells_successful = mp_geo_out->md_NumberOfWellsProdDrilled * mo_geo_in.md_DrillSuccessRate;
        double num_prod_wells_failed = mp_geo_out->md_NumberOfWellsProdDrilled * (1 - mo_geo_in.md_DrillSuccessRate);
        mp_geo_out->md_NumberOfWellsProdFailed = num_prod_wells_failed;
        //2.	# of Injection Wells Required = # successful injection wells required in drilling phase + # successful injection wells drilled in exploration phase

        double inj_flow = flowRatePerWell() * mp_geo_out->md_NumberOfWells;
        if (mo_geo_in.me_ct == FLASH) inj_flow -= inj_flow * (waterLoss() / 1000.0);
        double failed_prod_wells_inj = mp_geo_out->md_NumberOfWellsProdDrilled - num_prod_wells_successful;
        double friction = 25.0; //psi
        double pressure_well_head = 0;
        if (mo_geo_in.me_ct == FLASH && FlashCount() == 1) pressure_well_head = mp_geo_out->md_PressureHPFlashPSI;
        else if (mo_geo_in.me_ct == FLASH && FlashCount() == 2) pressure_well_head = mp_geo_out->md_PressureLPFlashPSI;
        else pressure_well_head = pressureWellHeadPSI() - mo_geo_in.md_PressureChangeAcrossSurfaceEquipmentPSI;
        double prod_failed_inj_rate = (mo_geo_in.md_FailedProdFlowRatio * mo_geo_in.md_ReservoirDeltaPressure) *
            (mo_geo_in.md_InjWellPressurePSI + geothermal::MetersToFeet(GetResourceDepthM()) * InjectionDensity() / 144.0 + (pressure_well_head) -
            mo_geo_in.md_ProdWellFriction * pow(mo_geo_in.md_FailedProdFlowRatio, 2) - pressureHydrostaticPSI());
        double inj_failed_inj_rate = (mo_geo_in.md_FailedProdFlowRatio > 0) ? (mo_geo_in.md_FailedProdFlowRatio * mo_geo_in.md_ReservoirDeltaPressure) *
            (mo_geo_in.md_InjWellPressurePSI + geothermal::MetersToFeet(GetResourceDepthM()) * InjectionDensity() / 144.0 + (pressure_well_head) -
            mo_geo_in.md_InjWellFriction * pow(mo_geo_in.md_FailedProdFlowRatio, 2) - pressureHydrostaticPSI()) : 0.0;
        double inj_rate_failed_prod_wells = MIN(prod_failed_inj_rate, flowRatePerWell()); //Injectivity of failed production well?
        double inj_rate_failed_inj_wells = MIN(inj_failed_inj_rate, flowRatePerWell());
        double inj_rate_successful_inj_wells = flowRatePerWell() / mo_geo_in.md_RatioInjectionToProduction;
        double num_failed_prod_wells_inj = (mo_geo_in.md_FailedProdFlowRatio > 0) ? mp_geo_out->md_NumberOfWellsProdDrilled - num_prod_wells_successful : 0.0;
        double successful_inj_wells_required_drilling = (inj_flow - (num_failed_prod_wells_inj * prod_failed_inj_rate)) / (inj_rate_successful_inj_wells + (inj_rate_failed_inj_wells * (1 / (mo_geo_in.md_DrillSuccessRate) - 1)));
        double successful_inj_wells_exploration = mp_geo_out->InjWellsExploration;
        double num_injection_wells_required = successful_inj_wells_required_drilling + successful_inj_wells_exploration;
        mp_geo_out->md_NumberOfWellsInj = (inj_flow - (failed_prod_wells_inj * inj_rate_failed_prod_wells)) / (flowPerWellInj + inj_rate_failed_inj_wells * (1 / (mo_geo_in.md_DrillSuccessRate) - 1));
        mp_geo_out->md_NumberOfWellsInjDrilled = (1 / mo_geo_in.md_DrillSuccessRate) * successful_inj_wells_required_drilling + successful_inj_wells_exploration;
        double num_inj_wells_successful = mp_geo_out->md_NumberOfWellsInjDrilled * mo_geo_in.md_DrillSuccessRate;
        double num_inj_wells_failed = mp_geo_out->md_NumberOfWellsInjDrilled * (1 - mo_geo_in.md_DrillSuccessRate);
        //mp_geo_out->md_NumberOfWellsInj = (mo_geo_in.md_DesiredSalesCapacityKW / (netBrineEffectiveness / 1000)) * (mp_geo_out->md_FractionGFInjected) / flowPerWellInj;
        double pump_inj_hp = (GetInjectionPumpWorkft() * (inj_flow) / (60 * 33000)) / mo_geo_in.md_GFPumpEfficiency;
        mp_geo_out->md_InjPump_hp = pump_inj_hp;
        if (mo_geo_in.me_rt == EGS) {
            //mp_geo_out->md_NumberOfWellsProdExp = mp_geo_out->md_NumberOfWells - 8;
            if (mo_geo_in.md_WellsStimulated != 1) mp_geo_out->md_NumberOfWellsProdDrilled = mp_geo_out->md_NumberOfWellsProdExp / (mo_geo_in.md_DrillSuccessRate);
            else {
                double failed_prod_well_stimulations = 0;
                double successful_prod_well_stimulations = 0;
                if (mo_geo_in.md_WellsStimulated == 3) {
                    failed_prod_well_stimulations = 0;
                    successful_prod_well_stimulations = 0;
                }
                else {
                    failed_prod_well_stimulations = mp_geo_out->md_NumberOfWellsProdExp * (1 / mo_geo_in.md_StimSuccessRate - 1);
                    successful_prod_well_stimulations = mp_geo_out->md_NumberOfWellsProdExp;
                }
                mp_geo_out->md_NumberOfWellsProdDrilled = (failed_prod_well_stimulations + successful_prod_well_stimulations) / mo_geo_in.md_DrillSuccessRate;

            }
            double inj_wells_drilled = 0;
            if (inj_wells_stimulated) {
                inj_wells_drilled = mp_geo_out->md_NumberOfWellsInj / (mo_geo_in.md_DrillSuccessRate * mo_geo_in.md_StimSuccessRate);
            }
            else {
                inj_wells_drilled = mp_geo_out->md_NumberOfWellsInj / (mo_geo_in.md_DrillSuccessRate);
            }
            mp_geo_out->md_NumberOfWellsInj = inj_flow / inj_rate_successful_inj_wells - successful_inj_wells_exploration;
            mp_geo_out->md_NumberOfWellsInjDrilled = inj_wells_drilled;
            //mp_geo_out->md_NumberOfWellsProdDrilled = mp_geo_out->md_NumberOfWellsProdExp / mo_geo_in.md_DrillSuccessRate;
            /*
            num_prod_wells_failed = mp_geo_out->md_NumberOfWellsProdExp / mo_geo_in.md_DrillSuccessRate * (1 - mo_geo_in.md_DrillSuccessRate);
            num_prod_wells_successful = mp_geo_out->md_NumberOfWellsProdExp;
            inj_flow = flowRatePerWell() * mp_geo_out->md_NumberOfWells * (1 + (1 / (1 - 0.05) - 1));
            double successful_inj_wells_expl = inj_flow / (flowPerWellInj) - 1;
            mp_geo_out->md_NumberOfWellsInj = successful_inj_wells_expl;
            mp_geo_out->md_NumberOfWellsInjDrilled = successful_inj_wells_expl / (mo_geo_in.md_DrillSuccessRate * mo_geo_in.md_StimSuccessRate);
            num_inj_wells_failed = mp_geo_out->md_NumberOfWellsInjDrilled * mo_geo_in.md_DrillSuccessRate;
            double inj_wells_stim = successful_inj_wells_expl / mo_geo_in.md_StimSuccessRate;
            double failed_stim_wells = inj_wells_stim - successful_inj_wells_expl;
            */
        }

        if (mp_geo_out->md_NumberOfWellsInj < 0) mp_geo_out->md_NumberOfWellsInj = 0;
        if (mp_geo_out->md_NumberOfWells < 0) mp_geo_out->md_NumberOfWells = 0;
	}

	return mp_geo_out->md_NumberOfWells;
}

double CGeothermalAnalyzer::GetPlantBrineEffectiveness(void)
{
	/*
	double dTemperaturePlantDesignF = physics::CelciusToFarenheit(GetTemperaturePlantDesignC());
	double exitTempLowF = (0.8229 * dTemperaturePlantDesignF ) - 127.71;
	double exitTempHighF = (0.00035129 * pow(dTemperaturePlantDesignF,2)) + (0.69792956 * dTemperaturePlantDesignF) - 159.598;
	double dTemperatureGFExitF = 109.31;// (GetTemperaturePlantDesignC() < 180) ? exitTempLowF : exitTempHighF;  // degrees farenheit - exit temperature for geothermal fluid
	double dTemperatureGFExitC = physics::FarenheitToCelcius(dTemperatureGFExitF);	//physics::FarenheitToCelcius();
	double dAE_At_Exit = GetAEAtTemp(dTemperatureGFExitC); // watt-hr/lb - Calculate available energy using binary constants and plant design temp (short cut)
	*/
	double TSiO2 = -(0.0000001334837*pow(GetTemperaturePlantDesignC(), 4)) + (0.0000706584462*pow(GetTemperaturePlantDesignC(), 3)) - (0.0036294799613*pow(GetTemperaturePlantDesignC(), 2)) + (0.3672417729236*GetTemperaturePlantDesignC()) + 4.205944351495;
	double TamphSiO2 = (0.0000000000249634* pow(TSiO2, 4)) - (0.00000000425191 * pow(TSiO2, 3)) - (0.000119669*pow(TSiO2, 2)) + (0.307616*TSiO2) - 0.294394;



	//	double dTemperatureGFExitF = physics::CelciusToFarenheit(TamphSiO2); //109.31

	double dAE_At_Exit = GetAEAtTemp(TamphSiO2);


	// GETEM's "optimizer" seems to pick the max possible brine effectiveness for the default binary plant, so use this as a proxy for now



//	double dAEMaxPossible = (geothermal::IMITATE_GETEM) ? GetAEBinary() -  GetAEBinaryAtTemp(TamphSiO2) : GetAE() - dAE_At_Exit; // watt-hr/lb - [10B.GeoFluid].H54 "maximum possible available energy accounting for the available energy lost due to a silica constraint on outlet temperature"

	mp_geo_out->max_secondlaw = (1 - ((geothermal::IMITATE_GETEM) ? GetAEBinaryAtTemp(TamphSiO2) / GetAEBinary() : dAE_At_Exit / GetAE()) - 0.375);
	//double dMaxBinaryBrineEffectiveness = ((geothermal::IMITATE_GETEM) ? GetAEBinary() : GetAE()) * ((GetTemperaturePlantDesignC() < 150) ? 0.14425 * exp(0.008806 * GetTemperaturePlantDesignC()) : mp_geo_out->max_secondlaw);
    double dMaxBinaryBrineEffectiveness = ((geothermal::IMITATE_GETEM) ? GetAEBinary() : GetAE()) * (mp_geo_out->max_secondlaw);

	return (mo_geo_in.me_ct == FLASH) ? FlashBrineEffectiveness() : dMaxBinaryBrineEffectiveness * mo_geo_in.md_PlantEfficiency;
}

double CGeothermalAnalyzer::calculateX(double enthalpyIn, double temperatureF)
{
	double enthalpyF = geothermal::GetFlashEnthalpyF(temperatureF);
	double enthalpyG = geothermal::GetFlashEnthalpyG(temperatureF);
	//mp_geo_out -> getX = (enthalpyIn - enthalpyF) / (enthalpyG - enthalpyF);
	return (enthalpyIn - enthalpyF) / (enthalpyG - enthalpyF);
}

double CGeothermalAnalyzer::enthalpyChangeTurbine(double dEnthalpyDeltaInitial, double dEnthalpyTurbineG, double dCondF, double dCondG)
{	// I65-I80, I87-I102
	double xPrime, effTurb, dEnthalpyDelta, hEx;


    double A = 0.5 * geothermal::EFFICIENCY_TURBINE * dEnthalpyDeltaInitial;
    hEx = (dEnthalpyTurbineG - A * (1 - dCondF / (dCondG - dCondF))) / (1 + A / (dCondG - dCondF));
    xPrime = (hEx - dCondF) / (dCondG - dCondF);
    dEnthalpyDelta = dEnthalpyTurbineG - hEx;
	return dEnthalpyDelta;
}


// Turbine 1 - high pressure
double CGeothermalAnalyzer::turbine1dHInitial() { return calculateDH1(mp_geo_out->md_PressureHPFlashPSI - geothermal::DELTA_PRESSURE_HP_FLASH_PSI); } // I65
double CGeothermalAnalyzer::turbine1TemperatureF() {
	mp_geo_out->flash_temperature = geothermal::GetFlashTemperature(mp_geo_out->md_PressureHPFlashPSI - geothermal::DELTA_PRESSURE_HP_FLASH_PSI);		//Storing HP Flash Temperature (for cmod_geothermal_costs)
	return geothermal::GetFlashTemperature(mp_geo_out->md_PressureHPFlashPSI - geothermal::DELTA_PRESSURE_HP_FLASH_PSI);
} // D80

//Getting HP and LP specific volume values
double CGeothermalAnalyzer::GetSpecVol(double flash_temp) { return geothermal::getSpecVol(flash_temp); }


double CGeothermalAnalyzer::turbine1EnthalpyF() { return  geothermal::GetFlashEnthalpyF(turbine1TemperatureF()); }	// D81
double CGeothermalAnalyzer::turbine1EnthalpyG() { return  geothermal::GetFlashEnthalpyG(turbine1TemperatureF()); }	// D82
double CGeothermalAnalyzer::turbine1DH() { return enthalpyChangeTurbine(turbine1dHInitial(), turbine1EnthalpyG(), geothermal::GetFlashEnthalpyF(temperatureCondF()), geothermal::GetFlashEnthalpyG(temperatureCondF())); } // I80 - btu/lb
double CGeothermalAnalyzer::turbine1HEx() { return turbine1EnthalpyG() - turbine1DH(); } // I81 - btu/lb
double CGeothermalAnalyzer::turbine1X()
{	// D83 - %
	mp_geo_out->spec_vol = GetSpecVol(mp_geo_out->flash_temperature);
	double enthalpyPlantDesignTemp = geothermal::GetFlashEnthalpyF(physics::CelciusToFarenheit(GetTemperaturePlantDesignC()-DT_prod_well(mo_geo_in.md_dtProdWellChoice)));// D69
	mp_geo_out->getX_hp = calculateX(enthalpyPlantDesignTemp, turbine1TemperatureF());
	return calculateX(enthalpyPlantDesignTemp, geothermal::GetFlashTemperature(mp_geo_out->md_PressureHPFlashPSI));
}
double CGeothermalAnalyzer::turbine1Steam() { return geothermal::GEOTHERMAL_FLUID_FOR_FLASH * turbine1X(); }																										// D85 - lb/hr
double CGeothermalAnalyzer::turbine1NetSteam()
{	// I82 lb/hr
	double dForNCGRemoval = 0.0;
	if (geothermal::NCG_REMOVAL_TYPE != VAC_PUMP)
	{
		double dSteamFlow = steamFlow(1);
		if (geothermal::NUMBER_OF_COOLING_STAGES > 1) { dSteamFlow += steamFlow(2); }
		if (geothermal::NUMBER_OF_COOLING_STAGES > 2) { dSteamFlow += steamFlow(3); }
		dForNCGRemoval = dSteamFlow;
	}
	return turbine1Steam() - dForNCGRemoval;
}
double CGeothermalAnalyzer::turbine1OutputKWh() { return turbine1DH() * turbine1NetSteam() / 3413; }																				// I83 - kW/hr = (btu/lb) * (lb/hr) / (btu/kW)

// Flash Turbine 2 - low pressure
double CGeothermalAnalyzer::turbine2dHInitial(void) { return calculateDH2(mp_geo_out->md_PressureLPFlashPSI - geothermal::DELTA_PRESSURE_LP_FLASH_PSI); }														// I87
double CGeothermalAnalyzer::turbine2TemperatureF(void) {
	mp_geo_out->flash_temperature_lp = geothermal::GetFlashTemperature(mp_geo_out->md_PressureLPFlashPSI - geothermal::DELTA_PRESSURE_LP_FLASH_PSI);	//Getting LP Flash Temperature value for calculations in cmod_geothermal_costs
	mp_geo_out->spec_vol_lp = GetSpecVol(mp_geo_out->flash_temperature_lp);
	return geothermal::GetFlashTemperature(mp_geo_out->md_PressureLPFlashPSI - geothermal::DELTA_PRESSURE_LP_FLASH_PSI);
}														// D88
double CGeothermalAnalyzer::turbine2EnthalpyF(void) { return geothermal::GetFlashEnthalpyF(turbine2TemperatureF()); }															// D89
double CGeothermalAnalyzer::turbine2EnthalpyG(void) { return geothermal::GetFlashEnthalpyG(turbine2TemperatureF()); }															// D90
double CGeothermalAnalyzer::turbine2DH(void) { return enthalpyChangeTurbine(turbine2dHInitial(), turbine2EnthalpyG(), geothermal:: GetFlashEnthalpyF(temperatureCondF()), geothermal::GetFlashEnthalpyG(temperatureCondF())); }																// I102 - btu/lb
double CGeothermalAnalyzer::turbine2HEx(void) { return turbine2EnthalpyG() - turbine2DH(); }																							// I103 - btu/lb
double CGeothermalAnalyzer::turbine2X(void) {
	mp_geo_out->getX_lp = calculateX(turbine1EnthalpyF(), geothermal::GetFlashTemperature(mp_geo_out->md_PressureLPFlashPSI));
	return calculateX(geothermal::GetFlashEnthalpyF(geothermal::GetFlashTemperature(mp_geo_out->md_PressureHPFlashPSI)), geothermal::GetFlashTemperature(mp_geo_out->md_PressureLPFlashPSI));
}																		// D91 %
double CGeothermalAnalyzer::turbine2Steam(void) { return (FlashCount() == 2) ? geothermal::GEOTHERMAL_FLUID_FOR_FLASH * turbine2X() * (1 - turbine1X()) : 0; }																						// I104, D93 - lb/hr
double CGeothermalAnalyzer::turbine2OutputKWh(void) { return turbine2DH() * turbine2Steam() / 3413; }																				// I105 - kW/hr


// NCG Removal
double CGeothermalAnalyzer::pInter(int stage)
{	// D156, D205, D253 - psi
	switch (stage)
	{
	case 0: return pTotal();
	case 1: return pTotal() * pRatio();
		//case 2: return (geothermal::NUMBER_OF_COOLING_STAGES > 2) ? pTotal() * pRatio() * pRatio()  : geothermal::PRESSURE_AMBIENT_PSI;
	case 2: return (geothermal::NUMBER_OF_COOLING_STAGES > 2) ? pTotal() * pRatio() * pRatio() : mo_geo_in.md_PressureAmbientPSI;
		//case 3: return geothermal::PRESSURE_AMBIENT_PSI;
	case 3: return mo_geo_in.md_PressureAmbientPSI;
	default: { ms_ErrorString = ("Invalid stage in CGeothermalAnalyzer::pInter"); return 0; }
	}
}
double CGeothermalAnalyzer::pTotal(void) { return (geothermal::IMITATE_GETEM) ? pressureSaturation() + (geothermal::PRESSURE_CONDENSER_NCG_PARTIAL_INHG * 0.49) : pressureCondenser(); } // calculated separately in spreadsheet, but mathematically equivalent to pressureCondenser					   D150,D74 - psi
//double CGeothermalAnalyzer::pRatio(void) { return exp(log(geothermal::PRESSURE_AMBIENT_PSI / (pTotal()))/geothermal::NUMBER_OF_COOLING_STAGES); }
double CGeothermalAnalyzer::pRatio(void) { return exp(log(mo_geo_in.md_PressureAmbientPSI / (pTotal())) / geothermal::NUMBER_OF_COOLING_STAGES); }
double CGeothermalAnalyzer::ncgFlowLbsPerHour(void) { return geothermal::GEOTHERMAL_FLUID_FOR_FLASH * geothermal::NCG_LEVEL_PPM / 1000000; }
double CGeothermalAnalyzer::ncgFlowMolesPerHour(void) { return ncgFlowLbsPerHour() / geothermal::MOLE_WEIGHT_NCG; }
double CGeothermalAnalyzer::pSuction(int stage) { return pTotal() * pow(pRatio(), stage - 1); }
double CGeothermalAnalyzer::prJet(int stage) { return pInter(stage) / pInter(stage - 1); }
double CGeothermalAnalyzer::h2oMolesPerHour(int st) { return ncgFlowMolesPerHour() / ((pSuction(st) / pressureSaturation()) - 1); }
double CGeothermalAnalyzer::totalVentFlow(int st) { return ncgFlowLbsPerHour() + (h2oMolesPerHour(st) * geothermal::MOLE_WEIGHT_H2O); }
double CGeothermalAnalyzer::moleWeightVent(int st) { return totalVentFlow(st) / (ncgFlowMolesPerHour() + h2oMolesPerHour(st)); }
double CGeothermalAnalyzer::suctionSteamRatio(int st) {
	mp_geo_out->pressure_ratio_1 = pSuction(1) / mp_geo_out->md_PressureHPFlashPSI;
	//mp_geo_out-> pressure_ratio_2 = pSuction(2) / mp_geo_out->md_PressureHPFlashPSI;
	//mp_geo_out-> pressure_ratio_3 = pSuction(3) / mp_geo_out->md_PressureHPFlashPSI;
	return pSuction(st) / mp_geo_out->md_PressureHPFlashPSI;
}
double CGeothermalAnalyzer::AR(int stage) { return ((3.5879 * pow(prJet(stage), -2.1168)) + 0.1) * pow(suctionSteamRatio(stage), (-1.155 * pow(prJet(stage), -0.0453))); }
double CGeothermalAnalyzer::ERd(int stage) { return (1.0035 * AR(stage) + 8.9374)* pow(suctionSteamRatio(stage), (2.9594* pow(AR(stage), -0.8458) + 0.99)); }
double CGeothermalAnalyzer::ER(int st) { return ERd(st) * pow((((460 + geothermal::GetFlashTemperature(mp_geo_out->md_PressureHPFlashPSI)) * moleWeightVent(st)) / ((temperatureCondF() + 460) * geothermal::MOLE_WEIGHT_H2O)), 0.5); }
double CGeothermalAnalyzer::steamFlow(int st) { return (st >= 3 && (geothermal::NCG_REMOVAL_TYPE != JET || geothermal::NUMBER_OF_COOLING_STAGES < 3)) ? 0 : totalVentFlow(st) / ER(st); }

int CGeothermalAnalyzer::FlashCount(void) {
	mp_geo_out->flash_count = (mo_geo_in.me_ft >= DUAL_FLASH_NO_TEMP_CONSTRAINT) ? 2 : 1;
	return (mo_geo_in.me_ft >= DUAL_FLASH_NO_TEMP_CONSTRAINT) ? 2 : 1;
}

double CGeothermalAnalyzer::calculateDH1(double pressureIn)
{
	//double a = geothermal::GetDHa(pressureIn);
	//double b = geothermal::GetDHb(pressureIn);
	//double x = pressureIn / (pressureCondenser());
    //return a * log(x) + b;
    double h_ex_f = geothermal::GetFlashEnthalpyF(temperatureCondF());
    double h_ex_g = geothermal::GetFlashEnthalpyG(temperatureCondF());
    double s_cond_g = geothermal::GetFlashEntropyG(temperatureCondF());
    double s_cond_f = geothermal::GetFlashEntropyF(temperatureCondF());
    double s_in_g = geothermal::GetFlashEntropyG(turbine1TemperatureF());
    double h_ex_isent = h_ex_f + (h_ex_g - h_ex_f) * (s_in_g - s_cond_f) / (s_cond_g - s_cond_f);
    return turbine1EnthalpyG() - h_ex_isent;
}

double CGeothermalAnalyzer::calculateDH2(double pressureIn)
{
    //double a = geothermal::GetDHa(pressureIn);
    //double b = geothermal::GetDHb(pressureIn);
    //double x = pressureIn / (pressureCondenser());
    //return a * log(x) + b;
    double h_ex_f = geothermal::GetFlashEnthalpyF(temperatureCondF());
    double h_ex_g = geothermal::GetFlashEnthalpyG(temperatureCondF());
    double s_cond_g = geothermal::GetFlashEntropyG(temperatureCondF());
    double s_cond_f = geothermal::GetFlashEntropyF(temperatureCondF());
    double s_in_g = geothermal::GetFlashEntropyG(turbine2TemperatureF());
    double h_ex_isent = h_ex_f + (h_ex_g - h_ex_f) * (s_in_g - s_cond_f) / (s_cond_g - s_cond_f);
    return turbine2EnthalpyG() - h_ex_isent;
}

double CGeothermalAnalyzer::TemperatureWetBulbF(void) {
    double ElapsedTime = 0;
    
    if (mo_geo_in.md_UseWeatherFileConditions == 0 || isnan(m_wf.tdry) )
        return physics::CelciusToFarenheit(mo_geo_in.md_TemperatureWetBulbC);
    else {
        /*
        if (IsHourly()) ElapsedTime = mp_geo_out->ElapsedHours;
        else ElapsedTime = mp_geo_out->ElapsedMonths;
        OpenWeatherFile(mo_geo_in.mc_WeatherFileName);
        ReadWeatherForTimeStep(IsHourly(), ElapsedTime);
        */
        if (isnan(m_wf.twet)) {
            if (isnan(m_wf.rhum) || isnan(m_wf.pres)) {
                double dp_depression = (physics::CelciusToFarenheit(m_wf.tdry) - physics::CelciusToFarenheit(m_wf.tdew)) / 3.0;
                return physics::CelciusToFarenheit(m_wf.tdry) - dp_depression;
            }
            else {
                return physics::CelciusToFarenheit(calc_twet(m_wf.tdry, m_wf.rhum, m_wf.pres));
            }
        }
        else 
            return physics::CelciusToFarenheit(m_wf.twet);
    }
}

double CGeothermalAnalyzer::calc_twet(double T, double RH, double P)
{
    //	function [Twet] = calctwet(T, RH, P)
    //% calculate wet bulb temperature from T (dry bulb, 'C), RH (%), Pressure
    //% (mbar)
    //% see http://www.ejournal.unam.mx/atm/Vol07-3/ATM07304.pdf for eqns.

    /*
    Mike Wagner:
    There is a units error here! The original reference specifies that pressure should be provided in
    hPa (hectoPascals), which is equivalent with millibar. However, the units SHOULD BE in kPa, or mbar/10.
    Correct for the units issue here.

    IMPACT:
    This subroutine has been returning wet bulb temperatures much too high. This could adversely affect any
    model that calls the method and whose performance is sensitive to the wet bulb temperature.
    */
    if (T == -999. || RH == -999. || P == -999.) return -999.;

    volatile double Pkpa = P / 10.;	//Correct for units problem

    //volatile double Twet = T*0.7;// initial guess
    volatile double Twet = T - 5.;	//Initial guess [mjw -- negative values of T were causing problems here]

    //[mjw] Use a bisection method to solve for Twet. The previous iteration method is unstable.
    bool
        hiflag = false,
        lowflag = false;
    double
        hival = 0, lowval = 0, err;
    const double tol = 0.05;

    int i = 0;
    while (i++ < 250)
    {
        err = exp((21.3 * Twet + 494.41) / (Twet + 273.15)) - RH / 100 * exp((21.3 * T + 494.41) / (T + 273.15)) - (6.53 * 10e-4) * Pkpa * (T - Twet);
        //double G = exp( (21.3 * Twet + 494.41) / (Twet+273.15) ) * ( (21.4 * (Twet+273.15) - (21.4 * Twet+494.41)) / pow(Twet+273.15, 2) ) + 6.53*10e-4 * Pkpa * Twet;
        if (err < 0.) {
            lowval = Twet;
            lowflag = true;
        }
        else if (err > 0.) {
            hival = Twet;
            hiflag = true;
        }

        if (std::abs(err) < tol) break;

        //If the error is still too high, guess new values
        if (hiflag && lowflag) {
            //Bisect
            Twet = (hival + lowval) / 2.;
        }
        else if (hiflag) {
            //A lower bound hasn't yet been found. Try decreasing by 5 C
            Twet += -5;
        }
        else if (lowflag) {
            //An upper bound hasn't yet been found. Bisect the current Twet and the Tdry
            Twet = (Twet + T) / 2.;
        }
        else {
            //Neither flags have been set. Guess a lower temp.
            Twet += -5.;
        }

    }

    if (Twet != Twet) // check for NaN
    {
        /*
        from biopower, Jennie Jorgenson:
        For estimating the dew point (first line of code), I used this very simple relation from wikipedia: http://en.wikipedia.org/wiki/Dew_point#Simple_approximation
        The second line is from a slightly sketchier source (http://www.theweatherprediction.com/habyhints/170/), meteorologist Jeff Haby. His procedure is for temperatures in F.
        */

        double dp_est = T - ((1 - RH / 100) / 0.05);
        Twet = T - ((T - dp_est) / 3.0);
    }

    return Twet;
}

double CGeothermalAnalyzer::temperatureCondF(void)
{	// D71 - deg F
	return (TemperatureWetBulbF() + geothermal::DELTA_TEMPERATURE_CWF + geothermal::TEMPERATURE_PINCH_PT_CONDENSER_F + geothermal::TEMPERATURE_PINCH_PT_COOLING_TOWER_F);
}
double CGeothermalAnalyzer::pressureSaturation(void) { return geothermal::oPSatConstants.evaluate(temperatureCondF()); }// D72 - psi
double CGeothermalAnalyzer::pressureCondenser(void)
{// D74 - psi
	return pressureSaturation() + geothermal::InHgToPsi(geothermal::PRESSURE_CONDENSER_NCG_PARTIAL_INHG);
}

double CGeothermalAnalyzer::FlashBrineEffectiveness(void)
{
	if (!mp_geo_out->mb_BrineEffectivenessCalculated) {
		calculateFlashPressures();

		double dGrossOutput = turbine1OutputKWh();
		if (FlashCount() == 2) dGrossOutput += turbine2OutputKWh();
		double dGrossPower = dGrossOutput * geothermal::EFFICIENCY_GENERATOR;
		//double deltaPressureCondenserFt = (geothermal::CONDENSER_TYPE == SURFACE) ? geothermal::ADDITIONAL_CW_PUMP_HEAD_SURFACE : physics::PSItoFT(geothermal::PRESSURE_AMBIENT_PSI + 1 - (pressureCondenser())); // O102
		double deltaPressureCondenserFt = (geothermal::CONDENSER_TYPE == SURFACE) ? geothermal::ADDITIONAL_CW_PUMP_HEAD_SURFACE : physics::PSItoFT(mo_geo_in.md_PressureAmbientPSI + 1 - (pressureCondenser())); // O102
		double cwPumpHead = geothermal::BASE_CW_PUMP_HEAD_FT + deltaPressureCondenserFt; // D110 - ft
		mp_geo_out->cw_pump_head = cwPumpHead;


		double mainCWPumpPowerKW = CalculatePumpWorkInKW(cwFlow(), cwPumpHead);	// part of I105
		double dTotalPumpingKW = mainCWPumpPowerKW + cwPumpWorkKW();


		double dParasiticPower = dTotalPumpingKW + condensatePumpingKW() + fanPowerKW() + vacuumPumpingKW() + condenserInjectionPumpingKW();
		mp_geo_out->md_FlashBrineEffectiveness = dGrossPower - dParasiticPower;
		mp_geo_out->mb_BrineEffectivenessCalculated = true;
	}
	return mp_geo_out->md_FlashBrineEffectiveness;
}

void CGeothermalAnalyzer::calculateFlashPressures(void)
{	// This function Sets some values that will be used throughout the calculations of the flash brine effectiveness
	// These cannot be Set during initialization since some of the public properties may have been changed.  These
	// need to be calculated right when the brine effectiveness is calculated.

	if (mp_geo_out->mb_FlashPressuresCalculated) return;

	// if single flash - add flash pressure to delta pressure and quit
	if (FlashCount() == 1)
	{
		//mp_geo_out->md_PressureHPFlashPSI = pressureSingle() + geothermal::DELTA_PRESSURE_HP_FLASH_PSI;
        mp_geo_out->md_PressureHPFlashPSI = pressureSingleFlash() + geothermal::DELTA_PRESSURE_HP_FLASH_PSI;
        //mp_geo_out->md_PressureHPFlashPSI = pressureSingleFlash();
		return;
	}

	// dual flash, have to calculate both
	// high pressure flash
//i think this might be using the wrong temperature - resource instead of plant design - for EGS
	//mp_geo_out->md_PressureHPFlashPSI = pressureDualHigh() + geothermal::DELTA_PRESSURE_HP_FLASH_PSI;
    mp_geo_out->md_PressureHPFlashPSI = pressureDualFlashTempHigh() + geothermal::DELTA_PRESSURE_HP_FLASH_PSI;
    //mp_geo_out->md_PressureHPFlashPSI = pressureDualFlashTempHigh();
	// low pressure flash
	//mp_geo_out->md_PressureLPFlashPSI = pressureDualLow() + geothermal::DELTA_PRESSURE_LP_FLASH_PSI;
    mp_geo_out->md_PressureLPFlashPSI = pressureDualFlashTempLow() + geothermal::DELTA_PRESSURE_LP_FLASH_PSI;
    //mp_geo_out->md_PressureLPFlashPSI = pressureDualFlashTempLow();
	mp_geo_out->mb_FlashPressuresCalculated = true;
}

// Pump Power
double CGeothermalAnalyzer::overAllSteam(void) { return (this->FlashCount() == 2) ? turbine1NetSteam() + turbine2Steam() : turbine1NetSteam(); } // D96
double CGeothermalAnalyzer::qCondenser(void) {
	mp_geo_out->condenser_q = overAllSteam() * (overAllHEx() - geothermal::GetFlashEnthalpyF(temperatureCondF()));
	return overAllSteam() * (overAllHEx() - geothermal::GetFlashEnthalpyF(temperatureCondF()));
} // D99
double CGeothermalAnalyzer::cwFlow(void) {
	mp_geo_out->cwflow = qCondenser() / geothermal::DELTA_TEMPERATURE_CWF;
	//return qCondenser() / geothermal::DELTA_TEMPERATURE_CWF;
    return qRejectedTower() / geothermal::DELTA_TEMPERATURE_CWF;
} // D115
double CGeothermalAnalyzer::overAllHEx() //I107
{
	return (FlashCount() == 2) ? ((turbine2HEx() * turbine2Steam()) + (turbine1HEx() * turbine1NetSteam())) / (turbine1NetSteam() + turbine2Steam()) : turbine1HEx();
}

// CW Pump Power
double CGeothermalAnalyzer::h2oVentFlow(int stage) { return h2oMolesPerHour(stage) * geothermal::MOLE_WEIGHT_H2O; } // D160 - lb/hr
double CGeothermalAnalyzer::moleRatio(int st) { return (pInter(st) / pressureSaturation()) - 1; } // D184,
double CGeothermalAnalyzer::flowSteamMolesPerHr(int st) { return ncgFlowMolesPerHour() / moleRatio(st); } // D186,
double CGeothermalAnalyzer::flowSteamLbPerHr(int st) { return flowSteamMolesPerHr(st) * geothermal::MOLE_WEIGHT_H2O; } // D187,  - lb/hr
double CGeothermalAnalyzer::condensedSteamLbPerHour(int stage) { return steamFlow(stage) + h2oVentFlow(stage) - flowSteamLbPerHr(stage); } // D188 = D171+D160-D187 = stage1CondensedSteam (lb/hr)
double CGeothermalAnalyzer::pumpWorkFromSteamFlow(double flow)
{// D189-D198,
	double enthalpyCondF = geothermal::GetFlashEnthalpyF(temperatureCondF());
	double enthalpyCondG = geothermal::GetFlashEnthalpyG(temperatureCondF());

	double qReject = flow * (enthalpyCondG - enthalpyCondF);
	double cwFlow = qReject / geothermal::DELTA_TEMPERATURE_CWF;
	double pumpHead = geothermal::BASE_CW_PUMP_HEAD_FT + geothermal::ADDITIONAL_CW_PUMP_HEAD_SURFACE;
	return CalculatePumpWorkInKW(cwFlow, pumpHead);
}
double CGeothermalAnalyzer::cwPumpWorkKWByStage(int st) { return pumpWorkFromSteamFlow(condensedSteamLbPerHour(st)); } // D203- kW
double CGeothermalAnalyzer::cwPumpWorkKW(void) {
	mp_geo_out->cw_pump_work = cwPumpWorkKWByStage(1) + cwPumpWorkKWByStage(2) + cwPumpWorkKWByStage(3);
	return cwPumpWorkKWByStage(1) + cwPumpWorkKWByStage(2) + cwPumpWorkKWByStage(3);
} // D305 - kW, part of L105


// Condensate Pump Power
//double CGeothermalAnalyzer::condensatePumpHead(void) { return geothermal::PSItoFTB(geothermal::PRESSURE_AMBIENT_PSI + 1 - pressureCondenser()) + geothermal::BASE_CW_PUMP_HEAD_FT; } // D121
double CGeothermalAnalyzer::condensatePumpHead(void) { return geothermal::PSItoFTB(mo_geo_in.md_PressureAmbientPSI + 1 - pressureCondenser()) + geothermal::BASE_CW_PUMP_HEAD_FT; } // D121
double CGeothermalAnalyzer::condensatePumpPowerKW(void) {
	mp_geo_out->condensate_pump_power = CalculatePumpWorkInKW(overAllSteam(), condensatePumpHead());
	return CalculatePumpWorkInKW(overAllSteam(), condensatePumpHead());
} // D126->kw, part of I106
//double CGeothermalAnalyzer::condensatePumpHeadByStage(int st) { return geothermal::PSItoFTB(geothermal::PRESSURE_AMBIENT_PSI + 1 - pInter(st)); } // D201, D249, D297
double CGeothermalAnalyzer::condensatePumpHeadByStage(int st) { return geothermal::PSItoFTB(mo_geo_in.md_PressureAmbientPSI + 1 - pInter(st)); } // D201, D249, D297
double CGeothermalAnalyzer::condensatePumpWorkByStage(int st) { return CalculatePumpWorkInKW(condensedSteamLbPerHour(st), condensatePumpHeadByStage(st)); } // D207, ... kW
double CGeothermalAnalyzer::totalCondensatePumpWorkKW(void) {
	mp_geo_out->ncg_condensate_pump = condensatePumpWorkByStage(1) + condensatePumpWorkByStage(2) + condensatePumpWorkByStage(3); //D310
	return condensatePumpWorkByStage(1) + condensatePumpWorkByStage(2) + condensatePumpWorkByStage(3);
} // D310 - kW
double CGeothermalAnalyzer::condensatePumpingKW(void) { return condensatePumpPowerKW() + totalCondensatePumpWorkKW(); } // I106 - kW

// Fan Power
double CGeothermalAnalyzer::qRejectByStage(int stage) { return condensedSteamLbPerHour(stage) * (geothermal::GetFlashEnthalpyG(temperatureCondF()) - geothermal::GetFlashEnthalpyF(temperatureCondF())); } // D190
double CGeothermalAnalyzer::qRejectTotal(void) {
	mp_geo_out->qRejectByStage_1 = qRejectByStage(1);
	mp_geo_out->qRejectByStage_2 = qRejectByStage(2);
	mp_geo_out->qRejectByStage_3 = qRejectByStage(3);

	//mp_geo_out->qRejectedTotal = qRejectByStage(1) + qRejectByStage(2) + qRejectByStage(3);
	return qRejectByStage(1) + qRejectByStage(2) + qRejectByStage(3);
} // D303
double CGeothermalAnalyzer::qRejectedTower(void) {
    mp_geo_out->qRejectedTotal = qCondenser() + qRejectTotal();
	return qCondenser() + qRejectTotal();
} // D102
double CGeothermalAnalyzer::fanPowerCoeffA(void) { return -2.0814 * log(geothermal::DELTA_TEMPERATURE_CWF) + 10.6013; } // O95
double CGeothermalAnalyzer::fanPowerCoeffB(void) { return -0.0188 * pow(geothermal::DELTA_TEMPERATURE_CWF, 0.0232); } // P95
double CGeothermalAnalyzer::fanPower(void) { return fanPowerCoeffA() * exp(this->TemperatureWetBulbF() * fanPowerCoeffB()); } // D103 - hp per MMBtu/hr
double CGeothermalAnalyzer::fanPowerKW(void) { return geothermal::HPtoKW(fanPower() * qRejectedTower() / 1000000); } // D105, I118


double CGeothermalAnalyzer::deltaPressureByStage(int st) { return pInter(st) - pSuction(st); } // D173, D222, D270 - psi
double CGeothermalAnalyzer::densityForVacuumPump(int st) { return pSuction(st) * moleWeightVent(st) / ((temperatureCondF() + 460)*10.7316); }// D166, D215, D263 - lb/ft^3
double CGeothermalAnalyzer::vaccumPumpHead(int st) { return deltaPressureByStage(st) * 144 / densityForVacuumPump(st); }	// D175, D224, D272 - ft
double CGeothermalAnalyzer::vacuumPumpWorkByStage(int st)
{ // D182, D231, D279 - kW
	return (geothermal::NCG_REMOVAL_TYPE == VAC_PUMP || (st == 3 && geothermal::NCG_REMOVAL_TYPE == HYBRID)) ? CalculatePumpWorkInKW(totalVentFlow(st), vaccumPumpHead(st)) : 0;
}
double CGeothermalAnalyzer::vacuumPumpingKW(void) {
	mp_geo_out->v_stage_1 = vacuumPumpWorkByStage(1);
	mp_geo_out->v_stage_2 = vacuumPumpWorkByStage(2);
	mp_geo_out->v_stage_3 = vacuumPumpWorkByStage(3);
	mp_geo_out->pressure_ratio_2 = pInter(1) / mp_geo_out->md_PressureHPFlashPSI;				//pressure ratios used in ncg ejector cost calculation
	mp_geo_out->pressure_ratio_3 = pInter(2) / mp_geo_out->md_PressureHPFlashPSI;
	return vacuumPumpWorkByStage(1) + vacuumPumpWorkByStage(2) + vacuumPumpWorkByStage(3);
}	// D311, I108

// Condenser Injection Pump Power
double CGeothermalAnalyzer::injectionDeltaP(void)
{	// D127 - psi (condensate injection delta pressure)
	//return (FlashCount() == 1) ? mp_geo_out->md_PressureHPFlashPSI - geothermal::PRESSURE_AMBIENT_PSI : mp_geo_out->md_PressureLPFlashPSI - geothermal::PRESSURE_AMBIENT_PSI; 
	return (FlashCount() == 1) ? mp_geo_out->md_PressureHPFlashPSI - mo_geo_in.md_PressureAmbientPSI : mp_geo_out->md_PressureLPFlashPSI - mo_geo_in.md_PressureAmbientPSI;
}
double CGeothermalAnalyzer::injectionPumpHead(void) { return physics::PSItoFT(injectionDeltaP()); } // D128 - ft
double CGeothermalAnalyzer::injCoeffA(void) { return -0.0001769 * log(geothermal::DELTA_TEMPERATURE_CWF) + 0.0011083; }// R95
double CGeothermalAnalyzer::injCoeffB(void) { return  0.0657628 * log(geothermal::DELTA_TEMPERATURE_CWF) - 0.4091309; }	// S95
double CGeothermalAnalyzer::injCoeffC(void) { return -6.7041142 * log(geothermal::DELTA_TEMPERATURE_CWF) + 44.3438937; }	// T95
double CGeothermalAnalyzer::injCoeffD(void) { return -0.0325112 * pow(geothermal::DELTA_TEMPERATURE_CWF, 2) + (6.831236 * geothermal::DELTA_TEMPERATURE_CWF) - 64.6250943; }	// U95

double CGeothermalAnalyzer::evaporativeWaterLoss(void) { return ((injCoeffA() * pow(TemperatureWetBulbF(), 3)) + (injCoeffB() * pow(TemperatureWetBulbF(), 2)) + (injCoeffC() * TemperatureWetBulbF()) + injCoeffD()) * qRejectedTower() / 1000000; } // D129 - lb/hr (evaporative water loss)
double CGeothermalAnalyzer::drift(void) { return 0.001 * cwFlow(); }																												// D130
double CGeothermalAnalyzer::blowDown(void) { return evaporativeWaterLoss() / (geothermal::INJECTION_PUMPING_CYCLES - 1) - drift(); }																	// D132
double CGeothermalAnalyzer::waterLoss(void) { return evaporativeWaterLoss() + drift(); }																				// D133
double CGeothermalAnalyzer::steamCondensate(void) { return (turbine1Steam() + turbine2Steam()) - waterLoss(); }																		// D135
double CGeothermalAnalyzer::steamCondensateInjected(void) { return (steamCondensate() < 0) ? 0 : steamCondensate(); }																// D136 - lb/hr
double CGeothermalAnalyzer::condenserInjectionPumpingKW() { return CalculatePumpWorkInKW(steamCondensateInjected(), injectionPumpHead()); }													// D138, I120 - kW


// Flash Pressures 
bool CGeothermalAnalyzer::TempConstraint(void)
{
	return ((mo_geo_in.me_ft == DUAL_FLASH_WITH_TEMP_CONSTRAINT) || (mo_geo_in.me_ft == SINGLE_FLASH_WITH_TEMP_CONSTRAINT));
}
double CGeothermalAnalyzer::tempFlashLimitF(void)
{	// D26 - deg F
	return physics::CelciusToFarenheit(geothermal::oFlashTempConstants.evaluate(GetResourceTemperatureC()));
}
double CGeothermalAnalyzer::pressureFlashAmorphousSilica(void) { return geothermal::oPC.evaluate(tempFlashLimitF()); } // D27 - psi
double CGeothermalAnalyzer::pressureSingleNoConstraint()
{ // Q64
	return (0.0207 * temperatureCondF() - 0.8416) * exp(0.0334*pow(temperatureCondF(), -0.1732) * GetTemperaturePlantDesignC());
}
double CGeothermalAnalyzer::pressureSingleWithConstraint()
{ // S64
	return (pressureSingleNoConstraint() < pressureFlashAmorphousSilica()) ? pressureFlashAmorphousSilica() : pressureSingleNoConstraint();
}
double CGeothermalAnalyzer::pressureSingleToTest(void)
{	// Q64 or S64
	return (TempConstraint()) ? pressureSingleWithConstraint() : pressureSingleNoConstraint();
}
double CGeothermalAnalyzer::pressureSingle(void)
{	// O64
	//return (pressureSingleToTest() < geothermal::PRESSURE_AMBIENT_PSI) ? geothermal::PRESSURE_AMBIENT_PSI : pressureSingleToTest(); 
	return (pressureSingleToTest() < mo_geo_in.md_PressureAmbientPSI) ? mo_geo_in.md_PressureAmbientPSI : pressureSingleToTest();
}
double CGeothermalAnalyzer::pressureDualHighNoConstraint()
{	// R64
	return geothermal::oFlashConstants1.evaluate(temperatureCondF()) * exp(geothermal::oFlashConstants2.evaluate(temperatureCondF()) * GetTemperaturePlantDesignC());
}
double CGeothermalAnalyzer::pressureDualHighWithConstraint()
{
	double a = (temperatureCondF() > 125) ? 1.59 + (0.0015547 * exp(0.0354727*temperatureCondF())) : 1.59 + (0.098693 * exp(0.0025283*temperatureCondF()));
	double b = (temperatureCondF() > 125) ? 0.01916 - (0.000005307 * exp(0.031279921*temperatureCondF())) : 0.01916 - (0.000167123 * exp(0.00400728*temperatureCondF()));
	return a * exp(b * (GetTemperaturePlantDesignC()-DT_prod_well(mo_geo_in.md_dtProdWellChoice)));
}
double CGeothermalAnalyzer::pressureDualHigh(void)
{	// P64
	return (TempConstraint()) ? pressureDualHighWithConstraint() : pressureDualHighNoConstraint();
}
double CGeothermalAnalyzer::pressureDualLowUnconstrained()
{ // R65
	return (0.12632*exp(0.01918 * temperatureCondF())) * exp((0.0146 * exp(-0.00205*temperatureCondF()) * GetTemperaturePlantDesignC()));
}
double CGeothermalAnalyzer::pressureDualLowConstrained()
{ // T65
	return (pressureDualLowUnconstrained() < pressureFlashAmorphousSilica()) ? pressureFlashAmorphousSilica() : pressureDualLowUnconstrained();
}
double CGeothermalAnalyzer::pressureDualLowToTest(void)
{	// R65 or T65
	return (TempConstraint()) ? pressureDualLowConstrained() : pressureDualLowUnconstrained();
}
double CGeothermalAnalyzer::pressureDualLow(void)
{	// P65
	//return  (pressureDualLowToTest() < geothermal::PRESSURE_AMBIENT_PSI) ? geothermal::PRESSURE_AMBIENT_PSI : pressureDualLowToTest(); 
	return  (pressureDualLowToTest() < mo_geo_in.md_PressureAmbientPSI) ? mo_geo_in.md_PressureAmbientPSI : pressureDualLowToTest();
}
double CGeothermalAnalyzer::pressureDualFlashTempHigh(void)
{
    double temp_lpdualflash = physics::CelciusToFarenheit(GetTemperaturePlantDesignC()) - 2 * (physics::CelciusToFarenheit(GetTemperaturePlantDesignC()) - temperatureCondF()) / 3;
    double si_limit = physics::CelciusToFarenheit(0.000161869 * pow(GetTemperaturePlantDesignC(), 2) + 0.83889 * GetTemperaturePlantDesignC() - 79.496);
    if (temp_lpdualflash < si_limit) temp_lpdualflash = si_limit;
    double temp_hpdualflash = physics::CelciusToFarenheit(GetTemperaturePlantDesignC()) - (physics::CelciusToFarenheit(GetTemperaturePlantDesignC()) - temp_lpdualflash) / 2;
    return geothermal::Pressure125to325.evaluate(temp_hpdualflash);
}

double CGeothermalAnalyzer::pressureDualFlashTempLow(void)
{
    double temp_lpdualflash = physics::CelciusToFarenheit(GetTemperaturePlantDesignC()) - 2*(physics::CelciusToFarenheit(GetTemperaturePlantDesignC()) - temperatureCondF()) / 3;
    double si_limit = physics::CelciusToFarenheit(0.000161869 * pow(GetTemperaturePlantDesignC(), 2) + 0.83889 * GetTemperaturePlantDesignC() - 79.496);
    if (temp_lpdualflash < si_limit) temp_lpdualflash = si_limit;
    return geothermal::Pressure125to325.evaluate(temp_lpdualflash);
}

double CGeothermalAnalyzer::pressureSingleFlash(void)
{
    double temp_hpdualflash = physics::CelciusToFarenheit(GetTemperaturePlantDesignC()) - (physics::CelciusToFarenheit(GetTemperaturePlantDesignC()) - temperatureCondF()) / 2;
    double si_limit = physics::CelciusToFarenheit(0.000161869 * pow(GetTemperaturePlantDesignC(), 2) + 0.83889 * GetTemperaturePlantDesignC() - 79.496);
    double temp_celcius = physics::FarenheitToCelcius(temp_hpdualflash);
    if (temp_celcius < si_limit) temp_celcius = si_limit;
    if (temp_celcius < 125) return geothermal::PressureUnder125.evaluate(temp_celcius);
    else if (temp_celcius >= 125 && temp_celcius < 325) return geothermal::Pressure125to325.evaluate(temp_celcius);
    else if (temp_celcius >= 325 && temp_celcius < 675) return geothermal::Pressure325to675.evaluate(temp_celcius);
    else return geothermal::PressureOver675.evaluate(temp_celcius);
}



//---------------------------------------------------------------------------------------------------------------------------------------------------
bool CGeothermalAnalyzer::OpenWeatherFile(const char * fn)
{
	mb_WeatherFileOpen = false;
	ml_ReadCount = 0;
    if (!m_wFile.open(fn))
        ms_ErrorString = "Could not open the weather file: ";// +std::string(fn);
	else
		mb_WeatherFileOpen = true;

	return mb_WeatherFileOpen;
}

bool CGeothermalAnalyzer::ReadWeatherForTimeStep(bool bHourly, unsigned int timeStep)
// Read one line in weather file for hourly analysis, or calculate the average values for a month for monthly analysis
{
	// if this is an hourly analysis, just ignore the time step and get the data from the next line in the weather file
	if (bHourly) return ReadNextLineInWeatherFile();

	// Not an hourly analysis, so calculate the monthly weather info
	int month = (timeStep % 12) + 1;
	size_t hours = util::hours_in_month(month);
	if (hours == 0)
	{
        ms_ErrorString = "util::hours_in_month returned zero for month =  ";// +util::to_string(month) + ".";
		return false;
	}

	double pressure = 0, wetbulb = 0, drybulb = 0, rel_humidity = 0;
	for (size_t i = 0; i < hours; i++)
	{
		ReadNextLineInWeatherFile();
		pressure += m_wf.pres;
		wetbulb += m_wf.twet;
		drybulb += m_wf.tdry;
		rel_humidity += m_wf.rhum;
	}
	m_wf.pres = pressure / hours;
	m_wf.twet = wetbulb / hours;
	m_wf.tdry = drybulb / hours;
	m_wf.rhum = rel_humidity / hours;
	return true;
}


bool CGeothermalAnalyzer::ReadNextLineInWeatherFile(void)
// Private function, called from ReadWeatherForTimeStep(timeStep)
// Read the next line from weather file; rewind file if passed the end; assumes 8760 hour weather file;
{
	if (ml_ReadCount >= 8760)
	{
		m_wFile.rewind();
		ml_ReadCount = 0;
	}

	if (!m_wFile.read(&m_wf))
	{
		ms_ErrorString = "Could not read  line " + util::to_string((int)ml_ReadCount + 1) + " in the weather file.";
		return false;
	}
	ml_ReadCount++;
	ml_HourCount++;

	return true;
}


bool CGeothermalAnalyzer::determineMakeupAlgorithm()
{   // This is the logic to determine which makeup algorithm GETEM uses: Binary, Flash, or EGS
	// Just because the user chooses "EGS" from the drop-down box on the "2A.Scenario Input" sheet,
	// does NOT mean that the model will use the results from the EGS makeup sheet.
	me_makeup = NO_MAKEUP_ALGORITHM;

	if ((mo_geo_in.me_rt != HYDROTHERMAL) && (mo_geo_in.me_rt != EGS)) ms_ErrorString = "Reource type not recognized in CGeoHourlyBaseInputs::determineMakeupAlgorithm.";
	if ((mo_geo_in.me_ct != BINARY) && (mo_geo_in.me_ct != FLASH))   ms_ErrorString = "Conversion system not recognized in CGeoHourlyBaseInputs::determineMakeupAlgorithm.";
	if (ms_ErrorString != "") return false;

	if (mo_geo_in.me_tdm == ENTER_RATE)
	{ // if user has chosen to enter the temperature decline rate, then the makeup is calculated either with the binary or flash method.
        if (mo_geo_in.me_ct == BINARY)
            me_makeup = MA_BINARY;
        else if (mo_geo_in.me_rt == EGS)
            me_makeup = MA_FLASH;
		else
		{
			if ((mo_geo_in.me_ft > NO_FLASH_SUBTYPE) && (mo_geo_in.me_ft <= DUAL_FLASH_WITH_TEMP_CONSTRAINT))
				me_makeup = MA_FLASH;
			else
				ms_ErrorString = ("Conversion system Set to 'flash', but the type of flash system was not recognized in CGeoHourlyBaseInputs::determineMakeupAlgorithm");
		}
	}
	else if (mo_geo_in.me_tdm == CALCULATE_RATE)
	{	// this temperature decline can only be calculated for Binary conversion systems with EGS resources
        if ((mo_geo_in.me_rt == EGS) && (mo_geo_in.me_ct == BINARY))
            me_makeup = MA_EGS_BINARY;
        else if ((mo_geo_in.me_rt == EGS) && (mo_geo_in.me_ct == FLASH))
            me_makeup = MA_EGS_FLASH;
	}
	else
		ms_ErrorString = ("Fluid temperature decline method not recognized in CGeoHourlyBaseInputs::determineMakeupAlgorithm.");

	return (me_makeup != NO_MAKEUP_ALGORITHM);
}

bool CGeothermalAnalyzer::inputErrorsForUICalculations(void)
{
	if (!ms_ErrorString.empty()) return true;
	if (GetTemperaturePlantDesignC() > GetResourceTemperatureC()) { ms_ErrorString = ("Plant design temperature cannot be greater than the resource temperature."); return true; }

	if ((mo_geo_in.me_rt != EGS) && (mo_geo_in.me_pc == SIMPLE_FRACTURE)) { ms_ErrorString = ("Reservoir pressure change based on simple fracture flow can only be calculated for EGS resources."); return true; }

	if ((mo_geo_in.me_rt != EGS) && (mo_geo_in.me_tdm == CALCULATE_RATE)) { ms_ErrorString = ("Temperature decline can only be calculated for EGS resources."); return true; }

	if ((mo_geo_in.me_tdm == ENTER_RATE) && (mo_geo_in.md_TemperatureDeclineRate < 0))
	{
		ms_ErrorString = ("Fluid temperature decline method chosen was 'enter rate', but the rate is < 0"); return true;
	}

	double dTemperatureRatio = physics::CelciusToKelvin(GetResourceTemperatureC()) / physics::CelciusToKelvin(GetTemperaturePlantDesignC()); // max valid value is MAX_TEMP_RATIO
	if ((dTemperatureRatio > geothermal::MAX_TEMP_RATIO) && (mo_geo_in.mi_ModelChoice == 0))
	{
		ms_ErrorString = ("Plant design temperature is too low for resource temperature.  GETEM equations will return invalid results."); return true;
	}

	//if ( this->netBrineEffectiveness() == 0 ) // this will cause a division by zero error
	//	{ ms_ErrorString = ("Inputs led to a divide by zero error.  Pump work = Plant output, so the net efficiency is zero."); return true; }

	//if ( this->netBrineEffectiveness() < 0 )
	//	{ ms_ErrorString = ("Inputs lead to required pump energy being greater than the plant output."); return true; }


	if (GetAEBinary() == 0)
	{
		ms_ErrorString = ("Inputs lead to available energy = zero, which will cause a division by zero error."); return true;
	}

	if (!determineMakeupAlgorithm()) return true; // determineMakeupAlgorithm sets member enum "me_makeup"

	return false;
}

bool CGeothermalAnalyzer::inputErrorsForAnalysis(void)
{	// check for errors in mo_geo_in
	if (inputErrorsForUICalculations()) return true;

	if (mo_geo_in.mi_ProjectLifeYears == 0) { ms_ErrorString = ("Project life was zero."); return true; }
	if (mo_geo_in.mi_ModelChoice < 0) { ms_ErrorString = ("The model choice was not set."); return true; }

	if (!(NumberOfReservoirs() > 0)) { ms_ErrorString = ("Resource potential must be greater than the gross plant output."); return true; }
	if (mo_pb_p.P_ref == 0) { ms_ErrorString = ("The power block parameters were not initialized."); return true; }

	if (!ms_ErrorString.empty()) return true;
	return false;
}


bool CGeothermalAnalyzer::ReadyToAnalyze()
{
	if (inputErrorsForAnalysis()) return false;

	if (!OpenWeatherFile(mo_geo_in.mc_WeatherFileName)) return false;

	if (!mp_geo_out->maf_ReplacementsByYear || !mp_geo_out->maf_monthly_resource_temp || !mp_geo_out->maf_monthly_power || !mp_geo_out->maf_monthly_energy || !mp_geo_out->maf_timestep_resource_temp ||
		!mp_geo_out->maf_timestep_power || !mp_geo_out->maf_timestep_test_values || !mp_geo_out->maf_timestep_pressure || !mp_geo_out->maf_timestep_dry_bulb || !mp_geo_out->maf_timestep_wet_bulb)

	{
		ms_ErrorString = "One of the output arrays was not initialized in the geothermal hourly model.";
		return false;
	}

	return true;
}

bool CGeothermalAnalyzer::RunAnalysis(bool(*update_function)(float, void*), void *user_data)
{
	if (!ReadyToAnalyze()) return false;   // open weather file m_wf

	if ((mo_geo_in.mi_ModelChoice != 0) && (!mo_PowerBlock.InitializeForParameters(mo_pb_p)))
	{
		ms_ErrorString = "There was an error initializing the power block with the input parameters: " + mo_PowerBlock.GetLastError();
		return false;
	}

	// ReSet all calculated values to zero
	float fPercentDone = 0.0;
	bool bWantToReplaceReservoir = false;
	double dElapsedTimeInYears = 0.0;

	// Initialize
	ReplaceReservoir(dElapsedTimeInYears);
	mp_geo_out->md_PumpWorkKW = GetPumpWorkKW();

	// Go through time step (hours or months) one by one
//    bool bReDrill = false;
	unsigned int iElapsedMonths = 0, iElapsedTimeSteps = 0, iEvaluationsInMonth = 0, iElapsedHours = 0;
	float fMonthlyPowerTotal;
	for (unsigned int year = 0; year < mo_geo_in.mi_ProjectLifeYears; year++)
	{
		mp_geo_out->maf_ReplacementsByYear[year] = 0;
		for (unsigned int month = 1; month < 13; month++)
		{
			fPercentDone = (float)iElapsedMonths / (float)(12 * mo_geo_in.mi_ProjectLifeYears) * 100.0f;

			if ((update_function != 0) && (TimeToUpdateInterface(fPercentDone, 2.0f)))
			{
				if (!(*update_function)(fPercentDone, user_data))
				{
					ms_ErrorString = "Aborted by user.";
					return false;
				}
			}

			fMonthlyPowerTotal = 0;
			for (unsigned int hour = 0; hour < (unsigned int)util::hours_in_month(month); hour++)
			{
				if (IsHourly() || (hour == 0))
				{
					// Error check
					if (iElapsedTimeSteps >= mo_geo_in.mi_TotalMakeupCalculations)
					{
						ms_ErrorString = "Time step exceded the array size in CGeoHourlyAnalysis::RunAnalysis().";
						return false;
					}

					// Read weather file info (function is smart enough to average for month if tis is a monthly analysis)
					// The call to ReadWeatherForTimeStep increments the hour counter (over whole life), and file read counter [0 to 8760(=# lines in weather file]
					if (!ReadWeatherForTimeStep(IsHourly(), iElapsedTimeSteps)) return false;

					// Set inputs that change for each timestep, weather data into power block inputs
					mo_pb_in.T_htf_hot = md_WorkingTemperatureC;
					mo_pb_in.T_wb = m_wf.twet;
					mo_pb_in.T_db = m_wf.tdry;
					mo_pb_in.P_amb = physics::mBarToAtm(m_wf.pres);
					mo_pb_in.TOU = mo_geo_in.mia_tou[ml_ReadCount - 1];

					// record current temperature (temperature changes monthly, but this is an hourly record of it)
					mp_geo_out->maf_timestep_resource_temp[iElapsedTimeSteps] = (float)md_WorkingTemperatureC; // NOTE: If EGS temp drop is being calculated, then PlantGrossPowerkW must be called.  No production = no temp change
					mp_geo_out->maf_timestep_pressure[iElapsedTimeSteps] = (float)mo_pb_in.P_amb;
					mp_geo_out->maf_timestep_dry_bulb[iElapsedTimeSteps] = (float)mo_pb_in.T_db;
					mp_geo_out->maf_timestep_wet_bulb[iElapsedTimeSteps] = (float)mo_pb_in.T_wb;

					// record outputs based on current inputs
					if (mo_geo_in.mi_ModelChoice == 0) // model choice 0 = GETEM
						mp_geo_out->maf_timestep_power[iElapsedTimeSteps] = (float)MAX(PlantGrossPowerkW() - mp_geo_out->md_PumpWorkKW, 0) * mo_geo_in.haf[iElapsedHours];
					else
					{	// run power block model
						if (!mo_PowerBlock.Execute((ml_HourCount - 1) * 3600, mo_pb_in))
							ms_ErrorString = "There was an error running the power block model: " + mo_PowerBlock.GetLastError();
                        mp_geo_out->maf_timestep_power[iElapsedTimeSteps] = (float)MAX(mo_PowerBlock.GetOutputkW() - mp_geo_out->md_PumpWorkKW, 0) * mo_geo_in.haf[iElapsedHours];
						//fJunk = (float)moMA->PlantGrossPowerkW(); // kinda works, but not quite the same
					}

					mp_geo_out->maf_timestep_test_values[iElapsedTimeSteps] = (float)(year + 1) * 1000 + (month);//+(hour); // puts number formatted "year,month,hour_of_month" number into test value

					fMonthlyPowerTotal += mp_geo_out->maf_timestep_power[iElapsedTimeSteps];

					// record hourly power which = hourly energy
					mp_geo_out->maf_hourly_power[iElapsedHours] = mp_geo_out->maf_timestep_power[iElapsedTimeSteps];
                    

					//md_ElapsedTimeInYears = year + util::percent_of_year(month,hour);
					if (!ms_ErrorString.empty()) { return false; }
					iElapsedTimeSteps++;
					dElapsedTimeInYears = iElapsedTimeSteps * (1.0 / mo_geo_in.mi_MakeupCalculationsPerYear);  //moved to be after iElapsedTimeSteps++;
				}
				else
					mp_geo_out->maf_hourly_power[iElapsedHours] = fMonthlyPowerTotal;

				iElapsedHours++;
                mp_geo_out->ElapsedHours = iElapsedHours;
			}//hours

			mp_geo_out->maf_monthly_resource_temp[iElapsedMonths] = (float)md_WorkingTemperatureC;	// resource temperature for this month
			iEvaluationsInMonth = (IsHourly()) ? (unsigned int)util::hours_in_month(month) : 1;
			mp_geo_out->maf_monthly_power[iElapsedMonths] = fMonthlyPowerTotal / iEvaluationsInMonth;		// avg monthly power
			mp_geo_out->maf_monthly_energy[iElapsedMonths] = fMonthlyPowerTotal * util::hours_in_month(month) / iEvaluationsInMonth;		// energy output in month (kWh)

			// Is it possible and do we want to replace the reservoir in the next time step?
			//bWantToReplaceReservoir = ( md_WorkingTemperatureC < (GetResourceTemperatureC() - geothermal::MAX_TEMPERATURE_DECLINE_C) ) ? true : false;
			bWantToReplaceReservoir = (md_WorkingTemperatureC < (GetResourceTemperatureC() - mo_geo_in.md_MaxTempDeclineC) && mo_geo_in.md_AllowReservoirReplacements) ? true : false;
			if (bWantToReplaceReservoir && CanReplaceReservoir(dElapsedTimeInYears + (1.0 / 12)))
			{
				ReplaceReservoir(dElapsedTimeInYears); // this will 'reset' temperature back to original resource temp
				mp_geo_out->maf_ReplacementsByYear[year] = mp_geo_out->maf_ReplacementsByYear[year] + 1;
			}
			else
				//CalculateNewTemperature( dElapsedTimeInYears ); // once per month -> reduce temperature from last temp
				CalculateNewTemperature(dElapsedTimeInYears + (1.0 / 12)); // once per month -> reduce temperature from last temp

			iElapsedMonths++;  //for recording values into arrays, not used in calculations
            mp_geo_out->ElapsedMonths = iElapsedMonths;
		}//months
	}//years

	if (!ms_ErrorString.empty()) return false;
	return true;
}


bool CGeothermalAnalyzer::TimeToUpdateInterface(float fPercentDone, float fNotificationIntervalInPercent)
{	// Needs to be called with fPercentDone = zero at beginning of each run to reset the static var

	if (fPercentDone == 0)
	{
		mf_LastIntervalDone = 0;
		return true;
	}

	if (fPercentDone >= (mf_LastIntervalDone + fNotificationIntervalInPercent))
	{
		mf_LastIntervalDone += fNotificationIntervalInPercent;
		return true;
	}

	return false;
}

bool CGeothermalAnalyzer::InterfaceOutputsFilled(void)
{
	if (inputErrorsForUICalculations()) return false;

	// This is not very efficient because it will call much of the code several times un-necessarily. Probably
	// doesn't matter, since this is called from the user interface and not repetatively.
	GetNumberOfWells();
	mp_geo_out->md_PlantBrineEffectiveness = GetPlantBrineEffectiveness();
	ReplaceReservoir(0.0); // set the working temp so the further calculations are correct
	mp_geo_out->md_GrossPlantOutputMW = PlantGrossPowerkW() / 1000;
    mp_geo_out->md_GrossPowerkW = GrossPowerMW();
	mp_geo_out->md_PumpWorkKW = GetPumpWorkKW();
    mp_geo_out->md_PumpDepthFt = GetProductionPumpWorkft();
	// mp_geo_out->md_BottomHolePressure  is calculated in GetCalculatedPumpDepthInFeet()
	//mp_geo_out->md_PumpHorsePower = (flowRatePerWell() * pumpHeadFt())/(60 * 33000 * geothermal::EFFICIENCY_PUMP_GF);
	mp_geo_out->md_PumpHorsePower = (flowRatePerWell() * pumpHeadFt()) / (60 * 33000 * mo_geo_in.md_GFPumpEfficiency);

	mp_geo_out->md_AverageReservoirTemperatureF = physics::CelciusToFarenheit(GetResourceTemperatureC());	// Set a default value, it might be recalculated in "GetPressureChangeAcrossReservoir()" if necessary
	mp_geo_out->md_PressureChangeAcrossReservoir = GetPressureChangeAcrossReservoir();

	if ((mp_geo_out->md_NumberOfWells > 0) && (error().empty()))
		return true;
	else
		return false;
}




/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RunGeothermalAnalysis
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int RunGeothermalAnalysis(bool(*update_function)(float, void*), void*user_data, std::string &err_msg,
	const SPowerBlockParameters &pbp, SPowerBlockInputs &pbInputs,
	const SGeothermal_Inputs &geo_inputs, SGeothermal_Outputs &geo_outputs)
{
	// return value 0 = clean run; 1 = error with message; 2 = unknown error, no message
	CGeothermalAnalyzer geo_analyzer(pbp, pbInputs, geo_inputs, geo_outputs);
	if (geo_analyzer.RunAnalysis(update_function, user_data))  // 
		return 0;
	else
		if (geo_analyzer.error() != "")
		{
			err_msg = geo_analyzer.error();
			return 1; // error that was flagged
		}
		else
		{
			err_msg = "Unknown error during run"; return 2;
		}
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get interum values for user interface
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int FillOutputsForUI(std::string &err_msg, const SGeothermal_Inputs &geo_inputs, SGeothermal_Outputs &geo_outputs)
{
	// return value 0 = clean run; 1 = error with message; 2 = unknown error, no message
	CGeothermalAnalyzer geo_analyzer(geo_inputs, geo_outputs);
	if (geo_analyzer.InterfaceOutputsFilled())
		return 0;
	else
		if (geo_analyzer.error() != "")
		{
			err_msg = geo_analyzer.error();
			return 1; // error that was flagged
		}
		else
		{
			err_msg = "Unknown error during run"; return 2;
		}
}
