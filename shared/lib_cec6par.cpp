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

#include <stdio.h>
#include <math.h>
#include <cmath>
#include <limits>
#include <iostream>

#include "lib_cec6par.h"
#include "lib_pv_incidence_modifier.h"
#include "lib_util.h"

static const double KB = 8.618e-5; // Boltzmann constant [eV/K] note units
static const double k_air=0.02676, mu_air=1.927E-5, Pr_air=0.724;  // !Viscosity in units of N-s/m^2
static const double EmisC = 0.84, EmisB = 0.7;  // Emissivities of glass cover, backside material
static const double sigma = 5.66961E-8, cp_air = 1005.5;
static double amavec[5] = { 0.918093, 0.086257, -0.024459, 0.002816, -0.000126 };	// !Air mass modifier coefficients as indicated for polycrystalline modules in Table A1 of DeSoto paper published in Solar Energy  Vol 80 Issue 1 January 2006

static const double Tc_ref = (25+273.15); // 25 'C
static const double I_ref = 1000; // 1000 W/m2
static const double Tamb_noct = 20;  // 20 Ambient NOCT temp ('C)
static const double I_noct = 800; // 800 NOCT Irradiance W/m2
static const double TauAlpha = 0.9; // 0.9
static const double eg0 = 1.12; // 1.12

const double air_table[41][8] = //Temp (degC), Density (kg/m^3), spec. heat (J/kgK), thermal conductivity (W/mK), Thermal diffusivity (m^2/s), Dynamic Viscosity (kg/m*s), Prandtl Number
{
    {-150,2.866,983,0.01171,0.000004158,0.000008636,0.000003013,0.7246},
    {-100,2.038,966,0.01582,0.000008036,0.000001189,0.000005837,0.7263},
    {-50,1.582,999,0.01979,0.00001252,0.00001474,0.000009319,0.744},
    {-40,1.514,1002,0.02057,0.00001356,0.00001527,0.00001008,0.7436},
    {-30,1.451,1004,0.02134,0.00001465,0.00001579,0.00001087,0.7425},
    {-20,1.394,1005,0.02211,0.00001578,0.0000163,0.00001169,0.7408},
    {-10,1.341,1006,0.02288,0.00001696,0.0000168,0.00001252,0.7387},
    {0,1.292,1006,0.02364,0.00001818,0.00001729,0.00001338,0.7362},
    {5,1.269,1006,0.02401,0.0000188,0.00001754,0.00001382,0.735},
    {10,1.246,1006,0.02439,0.00001944,0.00001778,0.00001426,0.7336},
    {15,1.225,1007,0.02476,0.00002009,0.00001802,0.0000147,0.7323},
    {20,1.204,1007,0.02514,0.00002074,0.00001825,0.00001516,0.7309},
    {25,1.184,1007,0.02551,0.00002141,0.00001849,0.00001562,0.7296},
    {30,1.164,1007,0.02588,0.00002208,0.00001872,0.00001608,0.7282},
    {35,1.145,1007,0.02625,0.00002277,0.00001895,0.00001655,0.7268},
    {40,1.127,1007,0.02662,0.00002346,0.00001918,0.00001702,0.7255},
    {45,1.109,1007,0.02699,0.00002416,0.00001941,0.0000175,0.7241},
    {50,1.092,1007,0.02735,0.00002487,0.00001963,0.00001798,0.7228},
    {60,1.059,1007,0.02808,0.00002632,0.00002008,0.00001896,0.7202},
    {70,1.028,1007,0.02881,0.0000278,0.00002052,0.00001995,0.7177},
    {80,0.9994,1008,0.02953,0.00002931,0.00002096,0.00002097,0.7154},
    {90,0.9718,1008,0.03024,0.00003086,0.00002139,0.00002201,0.7132},
    {100,0.9458,1009,0.03095,0.00003243,0.00002181,0.00002306,0.7111},
    {120,0.8977,1011,0.03235,0.00003565,0.00002264,0.00002522,0.7073},
    {140,0.8542,1013,0.03374,0.00003898,0.00002345,0.00002745,0.7041},
    {160,0.8148,1016,0.03511,0.00004241,0.0000242,0.00002975,0.7014},
    {180,0.7788,1019,0.03646,0.00004593,0.00002504,0.00003212,0.6992},
    {200,0.7459,1023,0.03779,0.00004954,0.00002577,0.00003455,0.6974},
    {250,0.6746,1033,0.04104,0.0000589,0.0000276,0.00004091,0.6946},
    {300,0.6158,1044,0.04418,0.00006871,0.00002934,0.00004765,0.6935},
    {350,0.5664,1056,0.04721,0.00007892,0.00003101,0.00005475,0.6937},
    {400,0.5243,1069,0.05015,0.00008951,0.00003261,0.00006219,0.6948},
    {450,0.488,1081,0.05298,0.0001004,0.00003415,0.00006997,0.6965},
    {500,0.4565,1093,0.05572,0.0001117,0.00003563,0.00007806,0.6986},
    {600,0.4042,1115,0.06093,0.0001352,0.00003846,0.00009515,0.7037},
    {700,0.3627,1135,0.06581,0.0001598,0.00004111,0.0001133,0.7092},
    {800,0.3289,1153,0.07037,0.0001855,0.00004362,0.0001326,0.7149},
    {900,0.3008,1169,0.07465,0.0002122,0.000046,0.0001529,0.7206},
    {1000,0.2772,1184,0.07868,0.0002398,0.00004826,0.0001741,0.726},
    {1500,0.199,1234,0.09599,0.0003908,0.00005817,0.0002922,0.7478},
    {2000,0.1553,1264,0.11113,0.0005664,0.0000663,0.000427,0.7539}
};

cec6par_module_t::cec6par_module_t( )
{
	Area = Vmp = Imp = Voc = Isc = alpha_isc = beta_voc 
		= a = Il = Io = Rs = Rsh = Adj = std::numeric_limits<double>::quiet_NaN();
}
double air_mass_modifier( double Zenith_deg, double Elev_m, double a[5] )
{
	// !Calculation of Air Mass Modifier
	double air_mass = 1/(cos( Zenith_deg*M_PI/180 )+0.5057*pow(96.080-Zenith_deg, -1.634));
	air_mass *= exp(-0.0001184 * Elev_m); // 'optional' correction for elevation (m), as applied in Sandia PV model
	double f1 = a[0] + a[1]*air_mass + a[2]*pow(air_mass,2) + a[3]*pow(air_mass,3) + a[4]*pow(air_mass,4);
	return f1 > 0.0 ? f1 : 0.0;
}

bool cec6par_module_t::operator() ( pvinput_t &input, double TcellC, double opvoltage, pvoutput_t &out )
{
	double muIsc = alpha_isc * (1-Adj/100);
	//double muVoc = beta_voc * (1+Adj/100);
	
	/* initialize output first */
	out.Power = out.Voltage = out.Current = out.Efficiency = out.Voc_oper = out.Isc_oper= out.AOIModifier = 0.0;
	
	double G_front, G_total, Geff_front_total, Geff_total;

	if( input.radmode != 3){ // Determine if the model needs to skip the cover effects (will only be skipped if the user is using POA reference cell data) 
		G_front = input.Ibeam + input.Idiff + input.Ignd;
		G_total = G_front + input.Irear; // total incident irradiance on tilted surface, W/m2
			
		// Rear side already accounts for these losses
		Geff_front_total = calculateIrradianceThroughCoverDeSoto(
			input.IncAng,
			input.Tilt,
			input.Ibeam,
			input.Idiff,
			input.Ignd, true);

		Geff_total = Geff_front_total + input.Irear;

		double aoi_modifier = 0.0;
		if (G_front > 0.) {
			aoi_modifier = Geff_front_total / G_front;
		}
		out.AOIModifier = aoi_modifier;

	
		double theta_z = input.Zenith;
		if (theta_z > 86.0) theta_z = 86.0; // !Zenith angle must be < 90 (?? why 86?)
		if (theta_z < 0) theta_z = 0; // Zenith angle must be >= 0

		Geff_total *= air_mass_modifier( theta_z, input.Elev, amavec );
	
	}
    else { // Even though we're using POA ref. data, we may still need to use the decomposed poa
		if( input.usePOAFromWF)
			G_total = Geff_total = input.poaIrr;
		else{
			G_total = input.poaIrr;
			Geff_total = input.Ibeam + input.Idiff + input.Ignd + input.Irear;
		}
        out.AOIModifier = 1.0; // there are no additional reflection losses to apply if we're using POA from a reference cell

	}

	double T_cell = input.Tdry + 273.15;
	if ( Geff_total >= 1.0 ) 
	{
		T_cell = TcellC + 273.15; // want cell temp in kelvin

		// calculation of IL and IO at operating conditions
		double IL_oper = Geff_total/I_ref *( Il + muIsc*(T_cell-Tc_ref) );
		if (IL_oper < 0.0) IL_oper = 0.0;
		
		double EG = eg0 * (1-0.0002677*(T_cell-Tc_ref));
		double IO_oper = Io * pow(T_cell/Tc_ref, 3) * exp( 1/KB*(eg0/Tc_ref - EG/T_cell) );
		double A_oper = a * T_cell / Tc_ref;
		double Rsh_oper = Rsh*(I_ref/Geff_total);
			
		double V_oc = openvoltage_5par( Voc, A_oper, IL_oper, IO_oper, Rsh_oper );
		double I_sc = IL_oper/(1+Rs/Rsh_oper);
		
		double P, V, I;
		
		if ( opvoltage < 0 )
		{
			P = maxpower_5par( V_oc, A_oper, IL_oper, IO_oper, Rs, Rsh_oper, &V, &I );			
		}
		else
		{ // calculate power at specified operating voltage
			V = opvoltage;
			if (V >= V_oc) I = 0;
			else I = current_5par( V, 0.9*IL_oper, A_oper, IL_oper, IO_oper, Rs, Rsh_oper );

			P = V*I;
		}
		
		out.Power = P;
		out.Voltage  = V;
		out.Current = I;
		out.Efficiency = P/(Area*G_total);
		out.Voc_oper = V_oc;
		out.Isc_oper = I_sc;
		out.CellTemp = T_cell - 273.15;
	}

	return out.Power >= 0;
}



/**********************************************************************************************
 *********************************************************************************************
    CEC/Wisconsin pv cell temperature models
 *********************************************************************************************
 *********************************************************************************************/

bool noct_celltemp_t::operator() ( pvinput_t &input, pvmodule_t &module, double , double &Tcell )
{
	double G_total, Geff_total;
	double tau_al = std::abs(TauAlpha);

	double theta_z = input.Zenith;
	if (theta_z > 86.0) theta_z = 86.0; // !Zenith angle must be < 90 (?? why 86?)
	if (theta_z < 0) theta_z = 0; // Zenith angle must be >= 0
	
	double W_spd = input.Wspd;
	if (W_spd < 0.001) W_spd = 0.001;

	if( input.radmode != 3){ // Determine if the model needs to skip the cover effects (will only be skipped if the user is using POA reference cell data) 
		G_total = input.Ibeam + input.Idiff + input.Ignd; // total incident irradiance on tilted surface, W/m2
			
		Geff_total = G_total;
		Geff_total = calculateIrradianceThroughCoverDeSoto(
			input.IncAng,
			input.Tilt,
			input.Ibeam,
			input.Idiff,
			input.Ignd, true );

		if (G_total > 0)
			tau_al *= Geff_total/G_total;
			
		// !Calculation of Air Mass Modifier
		Geff_total *= air_mass_modifier( theta_z, input.Elev, amavec );	

	} else { // Even though we're using POA ref. data, we may still need to use the decomposed poa
		if( input.usePOAFromWF )
			G_total = Geff_total = input.poaIrr;
		else{
			G_total = Geff_total = input.Ibeam + input.Idiff + input.Ignd;
		}

	}

	// TODO - shouldn't tau_al above include AM correction below?
	
	if (Geff_total > 0)
	{
		double Imp = module.ImpRef();
		double Vmp = module.VmpRef();
		double Area = module.AreaRef();

		// calculate cell temperature, kelvin
		//double G_total = input.Ibeam + input.Idiff + input.Ignd;		
		double eff_ref = Imp *Vmp / ( I_ref*Area );
		tau_al = std::abs(TauAlpha);  // Sev: What's the point of recalculating this??

		W_spd = input.Wspd * ffv_wind; //added 1/11/12 to account for FFV_wind correction factor internally
		if (W_spd < 0.001) W_spd = 0.001;				

		double Tnoct_adj = Tnoct + standoff_tnoct_adj; // added 1/11/12 for adjustment to NOCT as in the CECPV calculator based on standoff height, used in eqn below.
		Tcell = (input.Tdry+273.15) + (Geff_total/I_noct * (Tnoct_adj - Tamb_noct) * (1.0-eff_ref/tau_al))*9.5/(5.7 + 3.8*W_spd);
		Tcell = Tcell-273.15;
	}

	return true;
}


// !*****************************************************************
static double free_convection_194( double TC, double TA, double SLOPE, double rho_air, 
	double Area, double Length, double Width)
{      
// !Function added by TN (2010)
// !Solution for free convection coefficienet as presented in Nellis and Klein (2008) and EES
	 
	double L_ch_f,nu,Beta,g_spec,Gr,Ra,C_lam,Nu_lam,C_turb,Nu_turb,Nu_bar,h_up,h_vert,h_down;
	static const double grav = 9.81;

	L_ch_f    = Area/(2.*(Length+Width));//  !Eq. 6-54 (Nellis&Klein)

	if (TA > TC) SLOPE = 180.0 - SLOPE;

	// !Properties Constant for Each Plate Scenario
	nu        = mu_air / rho_air; //          !Kinematic Viscosity
	Beta      = 1. / ((TA+TC)/2.); //         !volumetric coefficient of thermal expansion

	// !Horizontal Heated Upward Facing Plate (L_ch_f)
	// !OR Cooled Downward Facing Plate
	g_spec    = grav*MAX(0.,cosd(SLOPE));//  !Adjustment of gravity vector;
	Gr        = g_spec*Beta* std::abs(TC-TA)*pow(L_ch_f,3)/pow(nu,2); //    !Grashof Number
	Ra        = MAX(0.0001,Gr*Pr_air); //                 !Rayleigh Number

	C_lam     = 0.671/pow(1. + pow(0.492/Pr_air, 9./16.) , 4./9.); //  !Eq. 6-49 (Nellis&Klein)
	Nu_lam    = 1.4/log(1.+(1.4/(0.835*C_lam*pow(Ra,0.25)))); // !Eq. 6-56 (Nellis&Klein)
	C_turb    = 0.14*((1.+0.0107*Pr_air)/(1.+0.01*Pr_air)); // !Eq. 6-58 (Nellis&Klein)
	Nu_turb   = C_turb*pow(Ra,1./3.); //        !Eq. 6-57  (Nellis&Klein)
	Nu_bar    = pow(pow(Nu_lam,10) + pow(Nu_turb,10.), (1./10.)); // !Eq. 6-55  (Nellis&Klein)
	h_up      = Nu_bar*k_air/L_ch_f;

	// !Vertical Plate (Length)
	g_spec    = grav*sind(SLOPE);  //          !Adjustment of gravity vector
	Gr        = g_spec*Beta* std::abs(TC-TA)*pow(Length,3)/pow(nu,2);
	Ra        = MAX(0.0001,Gr*Pr_air);
	
	Nu_bar    = pow(0.825+(0.387*pow(Ra,(1./6.)))/pow(1+ pow(0.492/Pr_air, 9./16.), (8./27.)),2)  ;  // !(Incropera et al.,2006)
	h_vert    = Nu_bar*k_air/Length;  

	// !Horizontal Heated Downward Facing Plate
	// !OR Cooled Upward Facing Plate
	g_spec    = grav*MAX(0.,-cosd(SLOPE));
	Gr        = g_spec*Beta* std::abs(TC-TA)*pow(L_ch_f,3)/pow(nu,2);
	Ra        = MAX(0.0001,Gr*Pr_air);

	Nu_bar    = 2.5/log(1.+(2.5/(0.527*pow(Ra,0.2)))*pow(1.+pow(1.9/Pr_air,0.9),2./9.));//    !Eq. 6-59  (Nellis&Klein)
	h_down    = Nu_bar*k_air/L_ch_f;

	// !Take Maximum of 3 Calculated Heat Transfer Coefficients

	return MAX(MAX(h_down,h_vert),h_up); //  !Fig. 6-12  (Nellis&Klein)
}

static double ffd_194( double D_h, double Re_dh )
{
	// !Function added by TN (2010)
	// !Solution for friction factor of channel flow as presented in Nellis and Klein (2008) and EES.

	static const double e = 0.005;
	return pow( (-2.*log10(MAX(1.e-6,((2.*e/(7.54*D_h)-5.02/Re_dh*log10(2.*e/(7.54*D_h)+13./Re_dh)))))), -2.0 );
}

static double channel_free_194( double W_gap, double SLOPE, double TA, double T_cr,
	double rho_air, double Length )
{
	// !Function added by TN (2010)
	// !Solution for internal forced convection as presented in Nellis and Klein (2008) an d EES

	double g_spec,Beta,alpha,nu_air,Ra,Nu;

	static const double grav = 9.81;

	nu_air    = mu_air / rho_air; //          !Kinematic Viscosity 

	g_spec    = MAX(0.1, sind(SLOPE)*grav);
	Beta   	= 1./((T_cr+TA)/2.);
	alpha 	= k_air / (rho_air * cp_air);
	Ra	    = MAX(0.001,g_spec*pow(W_gap,3)*Beta*(T_cr-TA)/(nu_air*alpha));
	Nu     	= Ra/24.*W_gap/Length*pow(1.0-exp(-35./Ra*Length/W_gap),0.75);

	return  Nu*k_air/W_gap;
}

bool mcsp_celltemp_t::operator() ( pvinput_t &input, pvmodule_t &module, double opvoltage, double &Tcell )
{	

	if ( input.Ibeam + input.Idiff + input.Ignd < 1 )
	{
		Tcell = input.Tdry;
		return true;
	}

	double THETAZ = input.Zenith;

	if (THETAZ > 86.0) THETAZ = 86.0; // !Zenith angle must be < 90 degrees	  

	// !INCIDENCE ANGLE MODIFIER CALCULATIONS
	double THETA = input.IncAng;
	if (THETA < 0) THETA=1;

	double n2         = 1.526; //   !refractive index of glass

	double RefrAng1   = asind(sind(THETA)/n2);
	double TransSurf1 = 1-0.5*( pow(sind(RefrAng1-THETA),2)/pow(sind(RefrAng1+THETA),2)
			+ pow(tand(RefrAng1-THETA),2)/pow(tand(RefrAng1+THETA),2) );
	double TransCoverAbs1 = exp(-k_glass*l_glass/cosd(RefrAng1));
	double tau1       = TransCoverAbs1*TransSurf1;
        
	//!Evaluating transmittance at angle Normal to surface (0), use 1 to avoid probs.
	double THETA2     = 1;
	double RefrAng2   = asind(sind(THETA2)/n2);
	double TransSurf2 = 1-0.5*( pow(sind(RefrAng2-1),2)/pow(sind(RefrAng2+1),2)
		+ pow(tand(RefrAng2-1),2)/pow(tand(RefrAng2+1),2) );
	double TransCoverAbs2 = exp(-k_glass*l_glass/cosd(RefrAng2));
	double tau2       = TransCoverAbs2*TransSurf2;

	//!Evaluating transmittance at equivalent angle for diffuse 
	double THETA3 = 59.7 - 0.1388*input.Tilt  + 0.001497*pow(input.Tilt,2);
	double RefrAng3   = asind(sind(THETA3)/n2);
	double TransSurf3 = 1-.5*( pow(sind(RefrAng3-THETA3),2)/pow(sind(RefrAng3+THETA3),2)
		+ pow(tand(RefrAng3-THETA3),2)/pow(tand(RefrAng3+THETA3),2) );
	double TransCoverAbs3 = exp(-k_glass*l_glass/cosd(RefrAng3));
	double TransCoverDiff = TransCoverAbs3;
	double tau3       = TransCoverAbs3*TransSurf3;
	double TADIR      = tau1/tau2;
	double TADIFF     = tau3/tau2;
//	double THETA3X	   = THETA3;

	// !Evaluating transmittance at equivalent angle for ground reflected radiation 
	THETA3 = 90.0 - 0.5788*input.Tilt  + 0.002693*pow(input.Tilt,2);
	RefrAng3   = asind(sind(THETA3)/n2);
	TransSurf3 = 1-.5*( pow(sind(RefrAng3-THETA3),2)/pow(sind(RefrAng3+THETA3),2)
		+ pow(tand(RefrAng3-THETA3),2)/pow(tand(RefrAng3+THETA3),2) );
	TransCoverAbs3 = exp(-k_glass*l_glass/cosd(RefrAng3));
	tau3       = TransCoverAbs3*TransSurf3;
	double TAGND     = tau3/tau2;
      
	// !Calculate HDKR COVER absorbed radiation for energy balance
	double QDIFF = input.Idiff*(1.-TransCoverDiff);
	double QGND = input.Ignd*(1.-TransCoverAbs3);
	double QDIR = input.Ibeam*(1.-TransCoverAbs1);
	double QHDKR = QDIFF+QGND+QDIR;

	// !Calculate HDKR TOTAL (cover + cell) absorbed radiaion for energy balance
	double SHDKR = input.Idiff*TADIFF*tau2+input.Ignd*TAGND*tau2+input.Ibeam*TADIR*tau2 + QHDKR;

	// !Calculation of Effective irradiance      
	double SUNDIFF=input.Idiff*TADIFF;
	double SUNGND=input.Ignd*TAGND;
	double SUNDIR=input.Ibeam*TADIR;
	double SUNEFF = SUNDIFF+SUNGND+SUNDIR;

//	double TAU_AL     = TauAlpha;
	//IF (SUNTILT.GT.0) TAU_AL = TAMAX*SUNEFF/SUNTILT  !DAA: = TAMAX is used for CEC study
	if (SUNEFF < 0)  SUNEFF=0;

	//!Calculation of Air Mass Modifier
	if  (THETAZ < 0) THETAZ=0;
	//double AMASS      = 1/(cosd(THETAZ)+0.5057*pow(96.080-THETAZ,-1.634));
	double MAM        = //a0+a1*AMASS+a2*pow(AMASS,2)+a3*pow(AMASS,3)+a4*pow(AMASS,4);
		air_mass_modifier( THETAZ, input.Elev, amavec );
	SUNEFF     = SUNEFF*MAM;

	if (SUNEFF < 1)
	{
		Tcell = input.Tdry;
		return true;
	}

	if (HTD == 1)
	{
		Nrows = Ncols = 1;
	}
	else if (HTD == 2)
	{
		Length = Nrows*Length;
		Width = Ncols*Width;
	}
	
	double Imp = module.ImpRef();
	double Vmp = module.VmpRef();
	
	double Area_base = module.AreaRef();	 // !Use provided area for Duffie and Beckman model to maintain consistency w/ previous model
	double Area = Area_base * Length * Width; // !Surface area of module
    // !Define characteristic length
    double L_char     = 4.0 * Length * Width / (2.0 * (Width + Length));
       
    // !If gap is less than 1 mm, use flush mounting configuration
    if (Wgap < 0.001 && MC == 4) MC = 2;
    
	double R_gap = Wgap / Length;
	if ( MC == 4  && R_gap > 1 && MSO == 1) MC = 1;	

	double v_ch = 1.0, Fcg, Fcs, Fbs, Fbg, T_sky, T_ground, T_rw;
	
	
	// !Set cover wind speed to wind input; backside wind speed may change based on mounting configuration
	double V_WIND = MAX(0.001,input.Wspd);
	double V_cover = V_WIND;
	double P_guess = 0;

	// convert to kelvin
	double TA = input.Tdry+273.15;
    if (std::isnan(input.Patm)) {
        m_err = "Atmospheric pressure (millibar) is required for this temperature model.";
        Tcell = std::numeric_limits<double>::quiet_NaN();
        return false;
    }
	double Patm = input.Patm*100; // convert millibar into Pascal

	double EFFREF = 1e-3;
		
	// !Guess power based on SRC efficiency and irradiance
	if (MC == 5)
		EFFREF = Imp*Vmp/(I_ref*Area_base);   // !Efficiency of module at SRC conditions
	else
		EFFREF = Imp*Vmp/(I_ref*Area);   // !Efficiency of module at SRC conditions

	P_guess = EFFREF * (SUNEFF*Area); // !Estimate performance based on SRC efficiency
	if (HTD == 2) P_guess = P_guess * Nrows * Ncols;


	// !Adjust backside wind speed based on mounting structure orientation for "gap" mounting configuration
	if (MC == 4) {
		if (MSO==2) V_WIND  = MAX(0.001, std::abs(cosd(input.Wdir-input.Azimuth))*V_WIND);
		if (MSO==3) V_WIND  = MAX(0.001, std::abs(cosd(input.Wdir+90.-input.Azimuth))*V_WIND);
		v_ch = V_WIND * 0.3;   // !Give realistic starting value to channel air velocity
	}

	Fcg        = (1. - cosd(input.Tilt))/2;  // !view factor between top of tilted plate and horizontal plane adjacent to bottom edge of plate
	Fcs        = 1. - Fcg;              // !view factor between top of tilted plate and everything else (sky)
	Fbs        = Fcg;                   // !view factor bewteen top and ground = bottom and sky
	Fbg        = Fcs;                   // !view factor bewteen bottom and ground = top and sky
    if (std::isnan(input.Tdew)) {
        m_err = "Dew point temperature (degC) is required for this temperature model.";
        Tcell = std::numeric_limits<double>::quiet_NaN();
        return false;
    }
	T_sky      = TA*pow(0.711+0.0056*input.Tdew+0.000073*pow(input.Tdew,2)+0.013*cosd(input.HourOfDay), 0.25);   // !Sky Temperature: Berdahl and Martin  
	T_ground   = TA;                    // !Set ground temp equal to ambient temp
	T_rw       = TA;                    // !Initial guess for roof or wall temp
	
	double err_P      = 100.;  // !Set initial performance error. Must be > tolerance for power error in do loop
	double err_P1     = 100.;  // !Set initial performance error for updated power guess
	double err_P2     = 0.;    // !Set initial previous error.  Should be zero so approach factor doesn't reset after 1 iteration
	int p_iter     = 0;     // !Set iteration counter (performance)
	double app_fac_P  = 1.;
	
	double TC = input.Tdry+273.15;
    double rho_air_test = 0;
    double mu_air_test = 0;
    double k_air_test = 0;
    double Pr_air_test = 0;
    double temp_interp = 0;
	while( p_iter <= 300 && std::abs(err_P) > 0.1 )
	{		  
		double err_TC   = 100.; //  !Set initial temperature error. Must be > tolerance for temp error in do loop
		double err_TC_p = 0.; //    !Set initial previous error. Should be zero so approach factor doesn't reset after 1 iteration
		int h_iter   = 0 ; //    !Set iteration counter (temperature)
		double app_fac  = 0.5  ; //  !Set approach factor for updating cell temp guess value
		double app_fac_v = 0.5 ; // !Set approach factor for updating channel velocity guess value

        //Lacunarity
        double res = 0.025; //resolution
        int oA_out = 0;
        int n_p = 2;
        const int n_p_const = n_p;
        std::vector<double> L(n_p);
        std::vector<double> L_n(n_p);
        util::matrix_t<double> Z;
        util::matrix_t<double> Z_n;
        Z.resize(n_p, 4);
        Z_n.resize(n_p, 4);
        std::vector<double> R(n_p);
        std::vector<double> R_n(n_p);

        //util::matrix_t<int> ArrayLog = SolArrayLog(res, 5 * ground_clearance_height, GCR, Length, 0.025, input.Tilt, Width, 9, false, oA_out);
        //SuperLac(ArrayLog, oA_out, 2, L, L_n, R, R_n, Z, Z_n);
        //Do something with finding asymptote of L or L_n to get final Lacunarity value here?


		switch( MC )
		{
		case 1 : // !Rack Mounting Configuration 
			while(std::abs(err_TC) > 0.001 )
			{
                //L_char = Lsc; //Change characteristic length to Lacunarity length scale
				double rho_air    = Patm*28.967/8314.34*(1./((TA+TC)/2.)) ; // !density of air as a function of pressure and ambient temp
                int iter = 0;
                
                                                                            //double Re_forced  = MAX(0.1,rho_air*V_cover*L_char/mu_air) ; //  !Reynolds number of wind moving across module
                //double Re_forced = MAX(0.1, rho_air * V_cover * Lsc / mu_air); //  !Reynolds number of wind moving across module
                
                
                double Re_forced = MAX(0.1, rho_air * V_cover * L_char / mu_air);
                double Nu_forced  = 0.037 * pow(Re_forced,4./5.) * pow(Pr_air, 1./3.) ; //  !Nusselt Number (Incropera et al., 2006)
                double h_forced = 0;
                if (lacunarity_enable == 0)
                    h_forced   = Nu_forced * k_air / L_char;
                else {
                    double Re_forced_Lsc = MAX(0.1, rho_air * V_cover * Lsc / mu_air);
                    h_forced = (k_air / (ground_clearance_height + 2.0 * Length * sind(input.Tilt))) * pow(10, (0.090125 * pow(Re_forced_Lsc, 1. / 5.) * pow(Pr_air, 1. / 12.) + 1.8617));
                }
                double h_sky      = (TC*TC+T_sky*T_sky)*(TC+T_sky);
				double h_ground   = (TC*TC+T_ground*T_ground)*(TC+T_ground);
				double h_free_c   = free_convection_194(TC,TA,input.Tilt,rho_air,Area,Length,Width) ; //   !Call function to calculate free convection on tilted surface (top)           
				double h_free_b   = free_convection_194(TC,TA,180.0-input.Tilt,rho_air,Area,Length,Width); // !Call function to calculate free convection on tilted surface (bottom)              
				double h_conv_c   = pow( pow(h_forced,3.) + pow(h_free_c,3.) , 1./3.) ; // !Combine free and forced heat transfer coefficients (top)
				double h_conv_b   = pow( pow(h_forced,3.) + pow(h_free_b,3.) , 1./3.) ; // !Combine free and forced heat transfer coefficients (bottom)
                //h_conv_c = h_forced;
                //h_conv_b = h_forced;
				// !Energy balance to calculate TC
				double TC1 = ( (h_conv_c+h_conv_b)*TA 
						+ (Fcs*EmisC+Fbs*EmisB)*sigma*h_sky*T_sky 
						+ (Fcg*EmisC+Fbg*EmisB)*sigma*h_ground*T_ground
						-(P_guess/Area)+SHDKR )
						/ ( h_conv_c 
						+ h_conv_b 
						+ (Fcs*EmisC +Fbs*EmisB)*sigma*h_sky 
						+ (Fcg*EmisC + Fbg*EmisB)*sigma*h_ground );

                

				// !Since some variables in TC1 calc are function of TC, iterative solving is required        
				err_TC     = TC1 - TC; // !Error between n-1 and n temp calculations
				TC         = TC1; //      !Set cell temp to most recent calculation
					
				h_iter++;			
				if ( h_iter > 150 ) return false;
			}
			break;
				
		case 2: // !Flush Mounting Configuration
			while(std::abs(err_TC) > 0.001)
			{
				double rho_air    = Patm*28.967/8314.34*(1 / ((TA+TC)/2.)); // !density of air a function of pressure and ambient temp
				double Re_forced  = MAX(0.1,rho_air*V_cover*L_char/mu_air); //  !Reynolds number of wind moving across panel: function of L_char: array depen?
				double Nu_forced  = 0.037 * pow(Re_forced, 4./5.) * pow(Pr_air, 1./3.);
				double h_forced   = Nu_forced * k_air / L_char;
				double h_sky      = (TC*TC+T_sky*T_sky)*(TC+T_sky);
				double h_ground   = (TC*TC+T_ground*T_ground)*(TC+T_ground);
				double h_free_c   = free_convection_194(TC,TA,input.Tilt,rho_air,Area,Length,Width);
				double h_conv_c   = pow((pow(h_forced,3.) + pow(h_free_c,3.)), (1./3.));
					
				double TC1 = ((h_conv_c)*TA + (Fcs*EmisC)*sigma*h_sky*T_sky + (Fcg*EmisC)*sigma*h_ground*T_ground
						- (P_guess/Area)+SHDKR)/(h_conv_c+(Fcs*EmisC)*sigma*h_sky+(Fcg*EmisC)*sigma*h_ground);
					
				err_TC     = TC1 - TC;
				TC         = TC1;
					
				h_iter++;
				if (h_iter > 150) return false;
			}
			break;
			
		case 3: // !Integrated Mounting Configuration
			while(std::abs(err_TC) > 0.001)
			{
				double TbackK = TbackInteg + 273.15;
				double rho_air    = Patm*28.967/8314.34*(1 / ((TA+TC)/2.)); // !density of air a function of pressure and film temp
				double rho_bk     = Patm*28.967/8314.34*(1 / ((TbackK+TC)/2.));
				double Re_forced  = MAX(0.1,rho_air*V_cover*L_char/mu_air); //  !Reynolds number of wind moving across panel: function of L_char: array depen?
				double Nu_forced  = 0.037 * pow(Re_forced, 4./5.) * pow(Pr_air, 1./3.);
				double h_forced   = Nu_forced * k_air / L_char;					
				double h_sky      = (TC*TC+T_sky*T_sky)*(TC+T_sky);
				double h_ground   = (TC*TC+T_ground*T_ground)*(TC+T_ground);				   
				double h_radbk    = (TC*TC+TbackK*TbackK)*(TC+TbackK); // !Using TbackK now instead of TA					
				double h_free_c   = free_convection_194(TC,TA,input.Tilt,rho_air,Area,Length,Width);				 
				double h_free_b   = free_convection_194(TC,TbackK,180.-input.Tilt,rho_bk,Area,Length,Width);				 
				double h_conv_c   = pow( pow(h_forced,3.) + pow(h_free_c,3.), (1./3.));
				double h_conv_b   = h_free_b;// !No forced convection on backside
					
				double TC1 = (h_conv_c*TA+h_conv_b*TbackK+Fcs*EmisC*sigma*h_sky*T_sky+Fcg*EmisC*sigma*h_ground*T_ground
								+EmisB*sigma*h_radbk*TbackK-(P_guess/Area)+SHDKR)
							/ (h_conv_c+h_conv_b+Fcs*EmisC*sigma*h_sky+Fcg*EmisC*sigma*h_ground+EmisB*sigma*h_radbk);
					
				err_TC     = TC1 - TC;
				TC         = TC1;
					
				h_iter++;
				if (h_iter > 150) return false;
			}
			break;
				
		case 4: // !Gap (channel) Mounting Configuration
			{
				double A_c, L_charB, L_str, Per_cw, D_h;
				
				if ( MSO == 1)
				{
					// !Define channel length and width for gap mounting configuration that does not block air flow in any direction
					// !Use minimum dimension for length so that MSO 1 will have lower temp than MSO 2 or 3
					L_charB = MIN(Width, Length);
					L_str   = MAX(Width, Length);
						
					// !These values are dependent on MSO
					A_c        = Wgap * L_str;     // !Cross Sectional area of channel
					Per_cw 	   = 2.*L_str;          // !Perimeter minus open sides
					D_h 	   = (4.*A_c)/Per_cw;   // !Hydraulic diameter
				}
				else if (MSO == 2) //  !Vertical supports
				{
					L_charB = Length;
					L_str   = Width / Ncols;
					A_c        = Wgap * L_str ; //         !Cross Sectional area of channel
					Per_cw 	   = 2.*L_str + 2.*Wgap ; //   !Perimeter ACCOUNTING for supports: different than MSO 1
					D_h 	   = (4.*A_c)/Per_cw ; //       !Hydraulic diameter
				}
				else if (MSO == 3) // ! Horizontal supports
				{
					// !Flow is restricted to one direction.  Wind speed has already been adjusted using a cosine projection
					L_charB = Width;
					// !Width of channel is function of number of columns of modules.  Assuming that support structures are exactly the length of a module
					L_str   = Length / Nrows;
					A_c        = Wgap * L_str ; //         !Cross Sectional area of channel
					Per_cw 	   = 2.*L_str + 2.*Wgap ; //   !Perimeter ACCOUNTING for supports: different than MSO 1
					D_h 	   = (4.*A_c)/Per_cw ; //       !Hydraulic diameter
				}
				else
					return false; // invalid parameter specified
					
				// !Begin iteration to find cell temperature
				while (std::abs(err_TC) > 0.001 )
				{      
					double rho_air    = Patm*28.967/8314.34*(1 / ((TA+TC)/2.)); // !density of air a function of pressure and film temp
					double err_v      = 100; //                                     !set error for channel velocity iteration
					double err_v_p    = 100;
					double P_in       = 0.5*V_WIND*V_WIND * rho_air; //  !Dynamic pressure at inlet
					int v_iter     = 0 ; //                       !set iteration counter for channel velocity iteration
						
					// !Calculate air velocity through channel by assuming roughness and estimating pressure drop
					while ( v_iter < 80 && std::abs(err_v) > 0.001)
					{
						double Re_dh_ch   = rho_air*v_ch*(D_h / mu_air) ; //   !Reynolds number for channel flow
						double f_fd       = ffd_194(D_h,Re_dh_ch)  ; //         !Friction factor of channel flow
						double tau_s      = f_fd*rho_air*pow(v_ch,2/8.); //        !Shear stress on air
						double P_out      = P_in - tau_s*Per_cw*L_charB/A_c; // !Dynamic pressure at outlet
						double v_ch1      = sqrt(MAX(0.0005,(2*P_out/rho_air))); //  !velocity
						err_v      = v_ch1 - v_ch; //                   !Current Error
						double err_v_sign = err_v * err_v_p; //                !Did error switch signs between previous calc?
						err_v_p    = err_v; //                          !Previous Error     
							
						if(err_v_sign < 0.) app_fac_v=app_fac_v*0.5; //    !If error switched signs, reduced "approach factor"
							
						v_ch       = v_ch + app_fac_v * err_v; //         !New velocity estimate equals old + a portion of the last error
						v_iter++ ; //                    !Add 1 to iteration counter
					}

					
					// !Heat transfer on cover of module: using un-adjusted wind speed input
					double Re_forced  = MAX(0.1,rho_air*V_cover*L_char/mu_air); //  !Reynolds number of wind moving across panel
					double Nu_forced  = 0.037*pow(Re_forced,4./5.)*pow(Pr_air,1./3.);
					double h_forced   = Nu_forced * k_air / L_char;
					double h_sky      = (TC*TC+T_sky*T_sky)*(TC+T_sky);
					double h_ground   = (TC*TC+T_ground*T_ground)*(TC+T_ground);
					double h_free_c   = free_convection_194(TC,TA,input.Tilt,rho_air,Area,Length,Width);
					double h_conv_c   = pow(pow(h_forced,3.) + pow(h_free_c,3.), 1./3.);
					
					// !Reynolds number for channel flow
					double Re_fp 	   = rho_air*v_ch*L_charB / mu_air;
					// !Use calculated channel velocity in flat plate correlation to find heat transfer coefficient
					// !This approach (rather than channel flow correlations) allows channel equations to approach open rack as gap increases
					double Nus_ch     = 0.037*pow(Re_fp,4./5.)*pow(Pr_air,1./3.);
					double h_ch       = Nus_ch * k_air / L_charB;
					h_ch       = MIN(h_ch, h_forced); //           !Make sure gap mounted doesn't calc lower temps than open rack
						
					// !Set iteration  counter and initial error for roof/wall temperature iteration
					int iter_T_rw  = 0;
					double err_T_rw   = 2;
					
					double h_radbk = 0;
					double Q_conv_c = 0, Q_conv_r = 0;
					// !Calculate roof temperature based on current cell temperature guess
					while ( iter_T_rw < 121 && std::abs(err_T_rw) > 0.001 )
					{
						double T_cr = (TC+T_rw)/2.; // !Average of cell and roof temp assumed in correlations

						double h_fr = 0;
							
						if (MSO == 3) h_fr = 0; //  !If E-W supports then assume no free convection
						else h_fr = channel_free_194(Wgap,input.Tilt,TA,T_cr,rho_air,Length); // !Call function for channel free convection        
				 
						double m_dot 	   = v_ch*rho_air*A_c ; // !mass flow rate through channel
						double h_conv_b   = pow( pow(h_ch,3) + pow(h_fr,3) , (1./3.)) ; // !total heat transfer coefficient in channel
							
						// !Calculate air temperature at the end of the channel 
						// !For MSO 2 & 3 have been calculating gap HT per channel(not necessarily entire array), so need to consider that going forward
							
						int AR = 0;
						if (MSO == 1) AR = 1;
						if (MSO == 2) AR = Ncols;
						if (MSO == 3) AR = Nrows;
					
						double T_m = T_cr-(T_cr-TA)*exp(-2*(Area/AR)*h_conv_b/(m_dot*cp_air));
						 
						// !Using air temp at end of channel, calculate heat transfer to air in the channel: Then adjust for entire array (AR)
						double Q_air = MAX(0.0001, cp_air*m_dot*(T_m - TA)) * AR;

						// !Determine the ratio of the heat transfered to channel that was from module and roof/wall by comparing temperatures
						double DELTAT_r   = T_rw - TA;
						double DELTAT_c   = TC - TA;
							
						double R_r        = MIN(1., DELTAT_r / MAX(0.1,(DELTAT_r + DELTAT_c)));
						double R_c        = MIN(1., DELTAT_c / MAX(0.1,(DELTAT_r + DELTAT_c)));
							
						// !Adjust to flux
						Q_conv_c   = R_c * Q_air / Area;
						Q_conv_r   = R_r * Q_air / Area;
					
						// !Calculate heat transfer coefficient for radiation
						h_radbk    = (TC*TC+T_rw*T_rw)*(TC+T_rw);
						// !Energy Balance to calculate roof/wall temperature     
						double T_rw1        = MAX(TA, TC - Q_conv_r/(EmisB*sigma*h_radbk));

						err_T_rw    = T_rw1 - T_rw; //   !Error
						T_rw        = T_rw + (0.5-0.495*(iter_T_rw/60))*err_T_rw; //  !Reset guess
						iter_T_rw++; //   !Increase iteration counter
					}
					
					// Once roof temperature has been solved for guess cell temperature, re-calculate cell temperature
					double TC1 = (h_conv_c*TA + Fcs*EmisC*sigma*h_sky*T_sky+Fcg*EmisC*sigma*h_ground*T_ground+sigma*EmisB*h_radbk*T_rw
							-Q_conv_c-(P_guess/Area)+SHDKR)/(h_conv_c+Fcs*EmisC*sigma*h_sky+Fcg*EmisC*sigma*h_ground+sigma*EmisB*h_radbk);
						
					err_TC      = TC1 - TC ; //                  !Current Error
					double err_sign    = err_TC * err_TC_p ; //        !Did error switch signs between previous calc?
					err_TC_p    = err_TC ; //                   !Previous Error 
						
					if(err_sign < 0.) app_fac = app_fac * 0.9;
						
					TC         = TC + app_fac * err_TC;

					h_iter++;
						
					if (h_iter > 150) return false;
				}
			}
			break;
		}

		// now calculate module power based on new Cell Temp

		pvoutput_t out;
		if (!module( input, TC-273.15, opvoltage, out ))
		{
			m_err = module.error();
			return false;
		}

		double PMAX_1 = out.Power * DcDerate;
		      
		if (HTD == 2)   PMAX_1 = PMAX_1 * Nrows * Ncols; //   !Calculate power on # modules used for heat transfer calcs
     
		err_P1     = PMAX_1 - P_guess; // !Performance error
		double err_sign_P = err_P1 * err_P2;
		err_P2     = err_P1;
      
		if( (p_iter > 5) && (err_sign_P < 0.)) app_fac_P = 0.75*app_fac_P;
      
		err_P      = (PMAX_1 - P_guess)/(Nrows * Ncols); //    !Performance error for 1 panel
		P_guess    = P_guess + app_fac_P*err_P1; //  !Set performance to most recent calc
		p_iter     = p_iter + 1; // !+1 to iteration counter
      
		if ( p_iter > 300 && std::abs(err_P) > 0.1 )
		{
			m_err = "Power Calculations Did Not Converge";
			return false;
		}
	}
	
	Tcell = TC - 273.15;
	return true;
}
