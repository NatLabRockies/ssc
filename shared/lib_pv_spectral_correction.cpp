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


#include <math.h>

#include "lib_pv_incidence_modifier.h"
#include "lib_util.h"
#include "../ssc/vartab.h"
#include "../ssc/core.h"

//def spectral_factor_firstsolar(precipitable_water, airmass_absolute,
//    module_type = None, coefficients = None,
//    min_precipitable_water = 0.1,
//    max_precipitable_water = 8,
//    min_airmass_absolute = 0.58,
//    max_airmass_absolute = 10) :
//    r"""
//    Spectral mismatch modifier based on precipitable water and absolute
//    (pressure - adjusted) air mass.
//
//    Estimates the spectral mismatch modifier, : math : `M`, representing the
//    effect of variation in the spectral irradiance on the module short circuit
//    current : math:`M`  is estimated from absolute(pressure - corrected) air
//    mass, :math:`AM_a`, and precipitable water, :math:`Pw`.
//
//    Default coefficients are determined for several cell types with
//    known quantum efficiency curves, by using the Simple Model of the
//    Atmospheric Radiative Transfer of Sunshine(SMARTS)[1]_.Using
//    SMARTS, spectrums are simulated with all combinations of AMa and
//    Pw where :
//
//*: math : `0.5 \textrm{ cm } <= Pw <= 5 \textrm{ cm }`
//* :math:`1.0 <= AM_a <= 5.0`
//* Spectral range is limited to that of CMP11(280 nm to 2800 nm)
//* Spectrum simulated on an equatorial facing surface with 37° tilt
//* All other parameters fixed at G173 standard
//
//From these simulated spectra, :math:`M` is calculated using the known
//quantum efficiency curves.Multiple linear regression is then
//applied to fit Eq. 1 to determine the coefficients for each module.More
//details on the model can be found in[2]_.
//
//Parameters
//----------
//precipitable_water : numeric
//atmospheric precipitable water.[cm]
//
//airmass_absolute : numeric
//absolute(pressure - adjusted) air mass.[unitless]
//
//module_type : str, optional
//a string specifying a cell type.Values of 'cdte', 'monosi', 'xsi',
//'multisi', and 'polysi' (can be lower or upper case).If provided,
//module_type selects default coefficients for the following modules :
//
//*``'cdte'`` - First Solar Series 4 - 2 CdTe module.
//* ``'monosi'``, ``'xsi'`` - First Solar TetraSun module.
//* ``'multisi'``, ``'polysi'`` - anonymous multi - crystalline silicon
//module.
//* ``'cigs'`` - anonymous copper indium gallium selenide module.
//* ``'asi'`` - anonymous amorphous silicon module.
//
//The module used to calculate the spectral correction
//coefficients corresponds to the Multi - crystalline silicon
//Manufacturer 2 Model C from[3]_.The spectral response(SR) of CIGS
//and a - Si modules used to derive coefficients can be found in[4]_
//
//coefficients : array - like, optional
//Allows for entry of user - defined spectral correction
//coefficients.Coefficients must be of length 6. Derivation of
//coefficients requires use of SMARTS and PV module quantum
//efficiency curve.Useful for modeling PV module types which are
//not included as defaults, or to fine tune the spectral
//correction to a particular PV module.Note that the parameters for
//modules with very similar quantum efficiency should be similar,
//in most cases limiting the need for module specific coefficients.
//
//min_precipitable_water : float, default 0.1
//minimum atmospheric precipitable water.Any ``precipitable_water``
//value lower than ``min_precipitable_water``
//is set to ``min_precipitable_water``.[cm]
//
//max_precipitable_water : float, default 8
//maximum atmospheric precipitable water.Any ``precipitable_water``
//value greater than ``max_precipitable_water``
//is set to ``np.nan``.[cm]
//
//min_airmass_absolute : float, default 0.58
//minimum absolute airmass.Any ``airmass_absolute`` value lower than
//``min_airmass_absolute`` is set to ``min_airmass_absolute``.[unitless]
//
//max_airmass_absolute : float, default 10
//minimum absolute airmass.Any ``airmass_absolute`` value greater than
//``max_airmass_absolute`` is set to ``max_airmass_absolute``.[unitless]
//
//Returns
//------ -
//modifier: array - like
//spectral mismatch factor(unitless) which can be multiplied
//with broadband irradiance reaching a module's cells to estimate
//effective irradiance, i.e., the irradiance that is converted to
//electrical current.
//
//Notes
//----
//The ``spectral_factor_firstsolar`` model takes the following form :
//
//..math::
//
//M = c_1 + c_2 AM_a + c_3 Pw + c_4 AM_a^ { 0.5 }
//+ c_5 Pw^ { 0.5 } + c_6 \frac{ AM_a } {Pw^ { 0.5 }}.
//
//The default values for the limits applied to : math:`AM_a` and :math:`Pw`
//via the ``min_precipitable_water``, ``max_precipitable_water``,
//``min_airmass_absolute``, and ``max_airmass_absolute`` are set to prevent
//divergence of the model presented above.These default values were
//determined by the publication authors in the original pvlib - python
//implementation(:pull:`208`).
//
//    References
//    ----------
//    ..[1] Gueymard, Christian.SMARTS2: a simple model of the atmospheric
//    radiative transfer of sunshine : algorithms and performance
//    assessment.Cocoa, FL : Florida Solar Energy Center, 1995.
//    ..[2] Lee, Mitchell, and Panchula, Alex. "Spectral Correction for
//    Photovoltaic Module Performance Based on Air Mass and Precipitable
//    Water." IEEE Photovoltaic Specialists Conference, Portland, 2016
//    ..[3] Marion, William F., et al.User's Manual for Data for Validating
//    Models for PV Module Performance.National Renewable Energy
//    Laboratory, 2014. http://www.nrel.gov/docs/fy14osti/61610.pdf
//..[4] Schweiger, M. and Hermann, W, Influence of Spectral Effects
//on Energy Yield of Different PV Modules : Comparison of Pwat and
//MMF Approach, TUV Rheinland Energy GmbH report 21237296.003,
//January 2017
//"""
//pw = np.atleast_1d(precipitable_water)
//pw = pw.astype('float64')
//if np.min(pw) < min_precipitable_water:
//pw = np.maximum(pw, min_precipitable_water)
//warn('Low precipitable water values replaced with '
//    f'{min_precipitable_water} cm in the calculation of spectral '
//    'mismatch.')
//
//    if np.max(pw) > max_precipitable_water:
//pw[pw > max_precipitable_water] = np.nan
//warn('High precipitable water values replaced with np.nan in '
//    'the calculation of spectral mismatch.')
//
//    airmass_absolute = np.minimum(airmass_absolute, max_airmass_absolute)
//
//    if np.min(airmass_absolute) < min_airmass_absolute:
//airmass_absolute = np.maximum(airmass_absolute, min_airmass_absolute)
//warn('Low airmass values replaced with 'f'{min_airmass_absolute} in '
//    'the calculation of spectral mismatch.')
//    # pvlib.atmosphere.get_absolute_airmass(1,
//        # pvlib.atmosphere.alt2pres(4340)) = 0.58 Elevation of
//    # Mina Pirquita, Argentian = 4340 m.Highest elevation city with
//    # population over 50, 000.
//
//    _coefficients = {}
//    _coefficients['cdte'] = (
//        0.86273, -0.038948, -0.012506, 0.098871, 0.084658, -0.0042948)
//    _coefficients['monosi'] = (
//        0.85914, -0.020880, -0.0058853, 0.12029, 0.026814, -0.0017810)
//    _coefficients['xsi'] = _coefficients['monosi']
//    _coefficients['polysi'] = (
//        0.84090, -0.027539, -0.0079224, 0.13570, 0.038024, -0.0021218)
//    _coefficients['multisi'] = _coefficients['polysi']
//    _coefficients['cigs'] = (
//        0.85252, -0.022314, -0.0047216, 0.13666, 0.013342, -0.0008945)
//    _coefficients['asi'] = (
//        1.12094, -0.047620, -0.0083627, -0.10443, 0.098382, -0.0033818)
//
//    if module_type is not None and coefficients is None :
//coefficients = _coefficients[module_type.lower()]
//elif module_type is None and coefficients is not None :
//    pass
//    elif module_type is None and coefficients is None :
//raise TypeError('No valid input provided, both module_type and ' +
//    'coefficients are None')
//    else:
//raise TypeError('Cannot resolve input, must supply only one of ' +
//    'module_type and coefficients')
//
//    coeff = coefficients
//    ama = airmass_absolute
//    modifier = (
//        coeff[0] + coeff[1] * ama + coeff[2] * pw + coeff[3] * np.sqrt(ama) +
//        coeff[4] * np.sqrt(pw) + coeff[5] * ama / np.sqrt(pw))
//
//    return modifier
enum { monoSi, multiSi, CdTe, CIS, CIGS, Amorphous };
static double coeffs[6] = { 0.86273, -0.038948, -0.012506, 0.098871, 0.084658, -0.0042948 };
//std::vector<double> amavec[5] = { 0.918093, 0.086257, -0.024459, 0.002816, -0.000126 };	// !Air mass modifier coefficients as indicated for polycrystalline modules in Table A1 of DeSoto paper published in Solar Energy  Vol 80 Issue 1 January 2006


double spectral_correction_lee(double prec_water, double abs_airmass, int celltech, std::vector<double> coeff_inputs,
    double min_prec_water, double max_prec_water, double min_abs_airmass, double max_abs_airmass) {
    //Check for bounds of precipitable water
    double pw = prec_water;
    if (pw < min_prec_water) {
        pw = min_prec_water;
        //log("Precipatble water below minimum threshold. Replacing with minimum threshold");
    }
    else if (pw > max_prec_water) {
        pw = max_prec_water;
    }

    double am = abs_airmass;
    if (am < min_abs_airmass) {
        am = min_abs_airmass;
    }
    else if (am > max_abs_airmass) {
        am = max_abs_airmass;
    }
    int type = celltech;
    std::vector<double> coeff;
    coeff.resize(6);
    if (coeff_inputs.size() != 6) {
        switch (type)
        {
        case 0: //monoSi
            coeff = { 0.85914, -0.020880, -0.0058853, 0.12029, 0.026814, -0.0017810 };
        case 1: //multiSi
            coeff = { 0.84090, -0.027539, -0.0079224, 0.13570, 0.038024, -0.0021218 };
        case 2: // CdTe
            coeff = { 0.86273, -0.038948, -0.012506, 0.098871, 0.084658, -0.0042948 };
        case 3: //CIS??? (Don't see published coefficients)
            coeff = { 0.85914, -0.020880, -0.0058853, 0.12029, 0.026814, -0.0017810 };
        case 4: // CIGS
            coeff = { 0.85252, -0.022314, -0.0047216, 0.13666, 0.013342, -0.0008945 };
        case 5: // Amorphous
            coeff = { 1.12094, -0.047620, -0.0083627, -0.10443, 0.098382, -0.0033818 };
        default:
            //m_err = "Invalid cell technology type provided.";
            coeff = { 0.85914, -0.020880, -0.0058853, 0.12029, 0.026814, -0.0017810 };

        }

    }
    else {
        coeff = coeff_inputs;
    }
    double scf = coeff[0] + coeff[1] * am + coeff[2] * pw + coeff[3] * sqrt(am) + coeff[4] * sqrt(pw) + coeff[5] * am / sqrt(pw);
    return scf;

}

double spectral_correction_king(double abs_airmass, double Zenith_deg, double Elev_m, std::vector<double> a)
{
    // !Calculation of Air Mass Modifier
    double air_mass = abs_airmass;
    std::vector<double> amavec;
    amavec.resize(5);
    amavec = { 0.918093, 0.086257, -0.024459, 0.002816, -0.000126 };

    if (a.size() != 5) {
        a = amavec; // Default coefficients from DeSoto paper
            
    }
    //air_mass *= exp(-0.0001184 * Elev_m); // 'optional' correction for elevation (m), as applied in Sandia PV model
    double f1 = a[0] + a[1] * air_mass + a[2] * pow(air_mass, 2) + a[3] * pow(air_mass, 3) + a[4] * pow(air_mass, 4);
    return f1 > 0.0 ? f1 : 0.0;
}

double spectral_correction_pelland(double abs_airmass, double csky_index, int celltech, std::vector<double> coeff_inputs = { 0.0 }) {

    std::vector<double> coeff;
    if (coeff_inputs.size() != 3) {
        switch (celltech)
        {
        case 0: //monoSi
            coeff = { 0.9845, -0.05169, 0.03034 };
        case 1: //multiSi
            coeff = { 0.9847, -0.05237, 0.03034 };
        case 2: // CdTe (First Solar 4-2)
            coeff = { 1.002, -0.07108, 0.02465 };
            //First Solar 4-1 (0.9981, -0.05776, 0.02336)
        case 3: //CIS??? (Don't see published coefficients)
            coeff = { 0.9845, -0.05169, 0.03034 };
        case 4: // CIGS
            coeff = { 0.9791, -0.03904, 0.03096 };
        case 5: // Amorphous
            coeff = { 1.051, -0.1033, 0.009838 };
        default:
            coeff = { 0.9845, -0.05169, 0.03034 };
            //m_err = "Invalid cell technology type provided.";
        }

    }
    else {
        coeff = coeff_inputs;
    }
    double scf = coeff[0] * pow(csky_index, coeff[1]) * pow(abs_airmass, coeff[2]);
    return scf;
}

static double sandia_absolute_air_mass(double SolZen, double Altitude)
{
    /*
   C Returns absolute air mass
   C SolZen = solar zenith (deg)
   C Altitude = site altitude (m)
   */
    if (SolZen < 89.9)
    {
        double AM = 1 / (cos(SolZen * 0.01745329) + 0.5057 * pow((96.08 - SolZen), -1.634));
        return AM * exp(-0.0001184 * Altitude);
    }
    else
        return 999;
}

double spectral_correction_factor(compute_module* cm, double pwater, double solzen = 0.0, double alt = 0.0, double csky_index = 0.0) {

    if (!cm)
        return 0;
    //Spectral correction functions
    int model_type = cm->as_integer("spectral_correction_model_choice");
    int celltech = cm->as_integer("celltech");
    double prec_water = pwater;
    double min_prec_water = cm->as_double("min_prec_water");
    double max_prec_water = cm->as_double("max_prec_water");
    double min_abs_airmass = cm->as_double("min_abs_airmass");
    double max_abs_airmass = cm->as_double("max_abs_airmass");
    std::vector<double> coeff_inputs;
    size_t* coeff_size;
    //Check if precipitable water exists
    if(isnan(pwater)) model_type = 1; //King, no precipitation data needed
    if(model_type == 0 && cm->is_assigned("coeff_inputs_lee")) {
        coeff_inputs = cm->as_vector_double("coeff_inputs_lee");
    }
    else if (model_type == 1 && cm->is_assigned("coeff_inputs_king")) {
        coeff_inputs = cm->as_vector_double("coeff_inputs_king");
    }
    else if (model_type == 2 && cm->is_assigned("coeff_inputs_pelland")) {
        coeff_inputs = cm->as_vector_double("coeff_inputs_pelland");
    }
    else {
        coeff_inputs = { 0.0 }; //How to handle errors
    }

    double abs_airmass = sandia_absolute_air_mass(solzen, alt);
    double scf = 0;
    if (model_type == 0) { //Default First Solar (TODO: rename with author of paper)
        scf = spectral_correction_lee(prec_water, abs_airmass, celltech, coeff_inputs,
            min_prec_water, max_prec_water, min_abs_airmass, max_abs_airmass);
    }
    else if (model_type == 1) { //King
        scf = spectral_correction_king(abs_airmass, solzen, alt, coeff_inputs);
    }
    else if (model_type == 2) { //Pelland
        scf = spectral_correction_pelland(abs_airmass, csky_index, celltech, coeff_inputs);
    }
    else {
        scf = 1; //How to handle errors
    }
    return scf;
}

var_info vtab_spectral_correction[] = {
    // instantaneous power at each timestep - consistent with sun position
{ SSC_INPUT, SSC_NUMBER , "spectral_correction_model_choice",                 "Spectral correction model choice",                        "0/1/2",         "", "Spectral Correction",      "?=0",     "",    ""},
{ SSC_INPUT, SSC_NUMBER , "celltech",                 "Cell technology",                        "",         "", "Spectral Correction",      "?=0",     "",    ""},
{ SSC_INPUT, SSC_NUMBER , "min_prec_water",                 "Minimum precipitable water",                        "",         "", "Spectral Correction",      "?=0.1",     "",    ""},
{ SSC_INPUT, SSC_NUMBER , "max_prec_water",                 "Maximum precipitable water",                        "",         "", "Spectral Correction",      "?=8",     "",    ""},
{ SSC_INPUT, SSC_NUMBER , "min_abs_airmass",                 "Cell technology",                        "",         "", "Spectral Correction",      "?=0.58",     "",    ""},
{ SSC_INPUT, SSC_NUMBER , "max_abs_airmass",                 "Cell technology",                        "",         "", "Spectral Correction",      "?=10",     "",    ""},
{ SSC_INPUT, SSC_ARRAY , "coeff_inputs_lee",                 "Cell technology",                        "",         "", "Spectral Correction",      "?",     "LENGTH=6",    ""},
{ SSC_INPUT, SSC_ARRAY , "coeff_inputs_king",                 "Cell technology",                        "",         "", "Spectral Correction",      "?",     "LENGTH=5",    ""},
{ SSC_INPUT, SSC_ARRAY , "coeff_inputs_pelland",                 "Cell technology",                        "",         "", "Spectral Correction",      "?",     "LENGTH=3",    ""},
{ SSC_OUTPUT, SSC_MATRIX, "annual_energy_distribution_time",	   "Annual energy production as function of time",	"kW",		  "", "Heatmaps",		  "",	   "",	  ""},

    var_info_invalid };
