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


#ifndef _LIB_PV_INCIDENCE_MODIFIER_H_
#define _LIB_PV_INCIDENCE_MODIFIER_H_

#include "lib_util.h"

#define AOI_MIN 0.5
#define AOI_MAX 89.5


// reference: De Soto, W., S. A. Klein and W. A. Beckman (2006). “Improvement and validation of a model for photovoltaic array performance.” Solar Energy 80(1): 78-88.
static const double n_glass = 1.526;   // !refractive index of glass
static const double l_glass = 0.002;   // !thickness of glass cover
static const double k_glass = 4; // proportionality constant for glass

static const double n_arc = 1.3; // refractive index of the anti-reflective coating
static const double l_arc = l_glass * 0.01; // thickness of anti-reflective coating- assume 1/100th of thickness of glass
static const double k_arc = 4; // proportionality constant for the anti-reflective coating

static const double n_air = 1.0; //refractive index of air is 1

double transmittance(double theta1_deg, /* incidence angle of incoming radiation (deg) */
	double n_cover,  /* refractive index of cover material, n_glass = 1.586 */
	double n_incoming, /* refractive index of incoming material, typically n_air = 1.0 */
	double k,        /* proportionality constant assumed to be 4 (1/m) for derivation of Bouguer's law (set to zero to skip bougeur's law */
	double l_thick,  /* thickness of cover material (m), usually 2 mm for typical module (set to zero to skip Bouguer's law) */
	double *_theta2_deg = 0); /* returns angle of refraction in degrees */

double iam(double theta_deg, bool ar_glass); // incidence angle modifier factor relative to normal incidence

double iam_nonorm(double theta_deg, bool ar_glass);  // non-normalized cover loss (typically use one above!)

/**
* function: iamSjerpsKoomen
*
* This method calculates the correction factor for angle of incidence, a factor from
* zero to one (no correction) using the air-glass model of Sjerps-Koomen et al in "A Simple
* Model for PV Module Reflection Losses under Field Conditons in Solar Energy
* 1997; 57:421-432. 5/27/2014
*
* Variables passed to the method:
*       n2 = index of refraction for glazing (1.526 for glass, 1.4 for Tefzel, 1.3 for AR glass)
*      inc = incident angle in radians
*
*
*/
double iamSjerpsKoomen(double n, double incidenceAngleRadians);

/// Calculate Irradiance through the cover using the DeSoto model
double calculateIrradianceThroughCoverDeSoto(double incidenceAngleDegrees, double tiltDegrees,
		double poaBeam, double poaSkyDiffuse, double poaGroundReflected, bool antiReflectiveGlass);

static const double MarionAOICorrectionFactorsGlass[] = 
	{0.057563, 0.128570, 0.199651, 0.265024, 0.324661, 0.378968, 0.428391, 0.473670, 0.514788, 0.552454,
	0.586857, 0.618484, 0.647076, 0.673762, 0.698029, 0.720118, 0.740726, 0.759671, 0.776946, 0.792833,
	0.807374, 0.821010, 0.833534, 0.845241, 0.855524, 0.865562, 0.874567, 0.882831, 0.890769, 0.897939,
	0.904373, 0.910646, 0.916297, 0.921589, 0.926512, 0.930906, 0.935179, 0.939074, 0.942627, 0.946009,
	0.949096, 0.952030, 0.954555, 0.957157, 0.959669, 0.961500, 0.963481, 0.965353, 0.967387, 0.968580,
	0.970311, 0.971567, 0.972948, 0.974114, 0.975264, 0.976287, 0.977213, 0.978142, 0.979057, 0.979662,
	0.980460, 0.981100, 0.981771, 0.982459, 0.982837, 0.983199, 0.983956, 0.984156, 0.984682, 0.985026,
	0.985364, 0.985645, 0.985954, 0.986241, 0.986484, 0.986686, 0.986895, 0.987043, 0.987287, 0.987388,
	0.987541, 0.987669, 0.987755, 0.987877, 0.987903, 0.987996, 0.988022, 0.988091, 0.988104, 0.988114,
	0.988114, 0.988104, 0.988091, 0.988022, 0.987996, 0.987903, 0.987877, 0.987755, 0.987669, 0.987541,
	0.987388, 0.987287, 0.987043, 0.986895, 0.986686, 0.986484, 0.986240, 0.985954, 0.985645, 0.985364,
	0.985020, 0.984676, 0.984156, 0.983956, 0.983199, 0.982837, 0.982459, 0.981771, 0.981100, 0.980460,
	0.979662, 0.979057, 0.978142, 0.977213, 0.976287, 0.975264, 0.974114, 0.972947, 0.971567, 0.970311,
	0.968580, 0.967387, 0.965353, 0.963481, 0.961501, 0.959671, 0.957157, 0.954555, 0.952030, 0.949096,
	0.946009, 0.942627, 0.939074, 0.935179, 0.930906, 0.926512, 0.921589, 0.916297, 0.910646, 0.904373,
	0.897939, 0.890769, 0.882831, 0.874567, 0.865562, 0.855524, 0.845241, 0.833534, 0.821010, 0.807374,
	0.792833, 0.776946, 0.759671, 0.740726, 0.720118, 0.698029, 0.673762, 0.647076, 0.618484, 0.586857,
	0.552454, 0.514788, 0.473670, 0.428391, 0.378968, 0.324661, 0.265024, 0.199651, 0.128570, 0.057563};

#endif
