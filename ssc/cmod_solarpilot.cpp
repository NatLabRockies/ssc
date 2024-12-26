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

#include "core.h"
#include "lib_weatherfile.h"
#include "lib_util.h"

// solarpilot header files
//#include "AutoPilot_API.h"
//#include "SolarField.h"
//#include "IOUtil.h"
#include "csp_common.h"

#include "AutoPilot_API.h"  //solarpilot
#include "definitions.h"    //solarpilot

#include "csp_solver_cavity_receiver.h"

#define pi 3.141592654


static var_info _cm_vtab_solarpilot[] = {
/*   VARTYPE           DATATYPE         NAME                         LABEL                                                                                                                                      UNITS     META        GROUP          REQUIRED_IF         CONSTRAINTS         UI_HINTS*/

	/*
	{ SSC_INPUT,        SSC_NUMBER,      "optimize",                  "Enable constrained optimization",                                                                                                        "0/1",    "",         "SolarPILOT",   "*",                "",                "" },
	{ SSC_INPUT,        SSC_NUMBER,      "range_tht_min",             "Tower height, minimum",                                                                                                                  "m",      "",         "SolarPILOT",   "*",                "",                "" },
	{ SSC_INPUT,        SSC_NUMBER,      "range_tht_max",             "Tower height, maximum",                                                                                                                  "m",      "",         "SolarPILOT",   "*",                "",                "" },
	{ SSC_INPUT,        SSC_NUMBER,      "range_rec_aspect_min",      "Receiver aspect ratio, minimum",                                                                                                         "",       "",         "SolarPILOT",   "*",                "",                "" },
	{ SSC_INPUT,        SSC_NUMBER,      "range_rec_aspect_max",      "Receiver aspect ratio, maximum",                                                                                                         "",       "",         "SolarPILOT",   "*",                "",                "" },
	{ SSC_INPUT,        SSC_NUMBER,      "range_rec_height_min",      "Receiver height, minimum",                                                                                                               "m",      "",         "SolarPILOT",   "*",                "",                "" },
	{ SSC_INPUT,        SSC_NUMBER,      "range_rec_height_max",      "Receiver height, maximum",                                                                                                               "m",      "",         "SolarPILOT",   "*",                "",                "" },
	{ SSC_INPUT,        SSC_NUMBER,      "flux_max",                  "Maximum flux",                                                                                                                           "kW/m2",  "",         "SolarPILOT",   "*",                "",                "" },
	*/

	{ SSC_INPUT,        SSC_STRING,      "solar_resource_file",       "Solar weather data file",                                                                                                                "",       "",         "SolarPILOT",   "?",                "LOCAL_FILE",      "" },

    { SSC_INPUT,        SSC_NUMBER,      "sun_loc_des",               "Sun location at design point (0 = Summer solstice, 1 = Equinox, 2 = Winter solstice)",                                                   "",       "",         "SolarPILOT",   "?=0",              "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "receiver_type",             "0: external (default), 1; cavity",                                                                                                       "",       "",         "SolarPILOT",   "?=0",              "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "rec_height",                "External receiver height",                                                                                                               "m",      "",         "SolarPILOT",   "receiver_type=0",  "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "rec_aspect",                "Receiver aspect ratio (H/W)",                                                                                                            "frac",   "",         "SolarPILOT",   "receiver_type=0",  "",                "" },

    // Cavity receiver
    { SSC_INPUT,        SSC_NUMBER,      "cav_rec_height",            "Cavity receiver height",                                                                                                                 "m",      "",         "SolarPILOT",   "receiver_type=1",  "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "cav_rec_width",             "Cavity receiver width",                                                                                                                  "m",      "",         "SolarPILOT",   "receiver_type=1",  "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "n_cav_rec_panels",          "Cavity receiver number of panels",                                                                                                       "",       "",         "SolarPILOT",   "receiver_type=1",  "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "cav_rec_span",              "Cavity receiver span angle",                                                                                                             "deg",    "",         "SolarPILOT",   "receiver_type=1",  "",                "" },

    // Free-falling particle receiver
    { SSC_INPUT,        SSC_NUMBER,      "num_recs",                  "Number of receivers",                                                                                                                    "",       "",         "SolarPILOT",   "?=1",              "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "is_recs_duplicate",         "1 = All receivers have the same power and geometry; 0 = otherwise",                                                                      "",       "",         "SolarPILOT",   "?=1",              "",                "" },
    { SSC_INPUT,        SSC_MATRIX,      "rec_tower_offset",          "Distance from tower optical height to receiver aperture center, format [[x1,y1],[x2,y2],...] or [[x1,y1,z1],[x2,y2,z2],...]",            "m",      "",         "SolarPILOT",   "?",                "",                "" },
    { SSC_INPUT,        SSC_ARRAY,       "rec_azimuth",               "Receiver azimuth orientation: 0 deg is north, positive clockwise",                                                                       "deg",    "",         "SolarPILOT",   "?",                "",                "" },
    { SSC_INPUT,        SSC_ARRAY,       "power_fraction",            "Target fraction of absorbed energy delivered by the heliostat field in configurations with multiple receivers",                          "",       "",         "SolarPILOT",   "?",                "",                "" },
    { SSC_INPUT,        SSC_ARRAY,       "fp_rec_height",             "Aperture height - in",                                                                                                                   "m",      "",         "SolarPILOT",   "receiver_type=3",  "",                "" },
    { SSC_INPUT,        SSC_ARRAY,       "rec_width",                 "Aperture width - in",                                                                                                                    "m",      "",         "SolarPILOT",   "receiver_type=3",  "",                "" },
    { SSC_INPUT,        SSC_ARRAY,       "norm_curtain_height",       "Normalized particle curtain height",                                                                                                     "",       "",         "SolarPILOT",   "receiver_type=3",  "",                "" },
    { SSC_INPUT,        SSC_ARRAY,       "norm_curtain_width",        "Normalized particle curtain width",                                                                                                      "",       "",         "SolarPILOT",   "receiver_type=3",  "",                "" },
    { SSC_INPUT,        SSC_ARRAY,       "max_curtain_depth",         "Particle curtain entrance depth",                                                                                                        "m",      "",         "SolarPILOT",   "receiver_type=3",  "",                "" },
    { SSC_INPUT,        SSC_MATRIX,      "norm_heights_depths",       "Normalized troughs heights and depths, pass [[0,0]] for no curtain troughs",                                                             "",       "",         "SolarPILOT",   "receiver_type=3",  "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "curtain_type",              "Flat=0;Curved=1",                                                                                                                        "",       "",         "SolarPILOT",   "receiver_type=3",  "",                "" },
    //{ SSC_INPUT,        SSC_NUMBER,      "curtain_radius",            "Particle curtain radius",                                                                                                                "m",      "",         "SolarPILOT",   "receiver_type=3&curtain_type=1",   "",        ""},
    { SSC_INPUT,        SSC_ARRAY,       "is_snout",                  "Is SNOUT enabled?",                                                                                                                      "",       "",         "SolarPILOT",   "receiver_type=3",  "",                "" },
    { SSC_INPUT,        SSC_ARRAY,       "snout_depth",               "Distance from aperture window to SNOUT front plane",                                                                                     "m",      "",         "SolarPILOT",   "receiver_type=3",  "",                "" },
    { SSC_INPUT,        SSC_ARRAY,       "snout_horiz_angle",         "SNOUT spanning angle defined in the aperture vertical mid-plane",                                                                        "deg",    "",         "SolarPILOT",   "receiver_type=3",  "",                "" },
    { SSC_INPUT,        SSC_ARRAY,       "snout_vert_bot_angle",      "SNOUT bottom surface angle from aperture normal",                                                                                        "deg",    "",         "SolarPILOT",   "receiver_type=3",  "",                "" },
    { SSC_INPUT,        SSC_ARRAY,       "snout_vert_top_angle",      "SNOUT top surface angle from aperture normal",                                                                                           "deg",    "",         "SolarPILOT",   "receiver_type=3",  "",                "" },
    { SSC_INPUT,        SSC_ARRAY,       "rec_design_hl",             "Receiver(s) estimated thermal loss per aperture area used in SolarPILOT of field sizing",                                                "kWt/m2", "",         "SolarPILOT",   "?",  "",                "" },

    // Heliostat parameters
    { SSC_INPUT,        SSC_NUMBER,      "helio_width",               "Heliostat width",                                                                                                                        "m",      "",         "SolarPILOT",   "*",                "",                "" },
	{ SSC_INPUT,        SSC_NUMBER,      "helio_height",              "Heliostat height",                                                                                                                       "m",      "",         "SolarPILOT",   "*",                "",                "" },
	{ SSC_INPUT,        SSC_NUMBER,      "helio_optical_error",       "Optical error",                                                                                                                          "rad",    "",         "SolarPILOT",   "*",                "",                "" },
	{ SSC_INPUT,        SSC_NUMBER,      "helio_active_fraction",     "Active fraction of reflective area",                                                                                                     "frac",   "",         "SolarPILOT",   "*",                "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "dens_mirror",               "Ratio of reflective area to profile",                                                                                                    "frac",   "",         "SolarPILOT",   "*",                "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "helio_reflectance",         "Mirror reflectance",                                                                                                                     "frac",   "",         "SolarPILOT",   "*",                "",                "" },

    // General receiver parameters
    { SSC_INPUT,        SSC_NUMBER,      "rec_absorptance",           "Absorptance",                                                                                                                            "frac",   "",         "SolarPILOT",   "*",                "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "rec_hl_perm2",              "Receiver design heat loss",                                                                                                              "kW/m2",  "",         "SolarPILOT",   "",                 "",                "" },
	{ SSC_INPUT,        SSC_NUMBER,      "q_design",                  "Receiver thermal design power",                                                                                                          "MW",     "",         "SolarPILOT",   "*",                "",                "" },
	{ SSC_INPUT,        SSC_NUMBER,      "dni_des",                   "Design-point DNI",                                                                                                                       "W/m2",   "",         "SolarPILOT",   "*",                "",                "" },

    { SSC_INPUT,        SSC_NUMBER,      "land_max",                  "Max heliostat-dist-to-tower-height ratio",                                                                                               "",       "",         "SolarPILOT",   "*",                "",                "" },
	{ SSC_INPUT,        SSC_NUMBER,      "land_min",                  "Min heliostat-dist-to-tower-height ratio",                                                                                               "",       "",         "SolarPILOT",   "*",                "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "csp.pt.sf.fixed_land_area",          "Fixed land area",                                                                                                               "acre",   "",         "SolarPILOT",   "*",                "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "csp.pt.sf.land_overhead_factor",     "Land overhead factor",                                                                                                          "",       "",         "SolarPILOT",   "*",                "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "h_tower",                   "Tower height",                                                                                                                           "m",      "",         "SolarPILOT",   "*",                "",                "" },

    { SSC_INPUT,        SSC_NUMBER,      "c_atm_0",					  "Attenuation coefficient 0",                                                                                                              "",       "",         "SolarPILOT",   "?=0.006789",       "",                "" },
	{ SSC_INPUT,        SSC_NUMBER,      "c_atm_1",					  "Attenuation coefficient 1",                                                                                                              "",       "",         "SolarPILOT",   "?=0.1046",         "",                "" },
	{ SSC_INPUT,        SSC_NUMBER,      "c_atm_2",					  "Attenuation coefficient 2",                                                                                                              "",       "",         "SolarPILOT",   "?=-0.0107",        "",                "" },
	{ SSC_INPUT,        SSC_NUMBER,      "c_atm_3",					  "Attenuation coefficient 3",                                                                                                              "",       "",         "SolarPILOT",   "?=0.002845",       "",                "" },

    { SSC_INPUT,        SSC_NUMBER,      "n_facet_x",                 "Number of heliostat facets - X",                                                                                                         "",       "",         "SolarPILOT",   "*",                "",                "" },
	{ SSC_INPUT,        SSC_NUMBER,      "n_facet_y",                 "Number of heliostat facets - Y",                                                                                                         "",       "",         "SolarPILOT",   "*",                "",                "" },
	{ SSC_INPUT,        SSC_NUMBER,      "focus_type",                "Heliostat focus method",                                                                                                                 "",       "",         "SolarPILOT",   "*",                "",                "" },
	{ SSC_INPUT,        SSC_NUMBER,      "cant_type",                 "Heliostat cant method",                                                                                                                  "",       "",         "SolarPILOT",   "*",                "",                "" },
	{ SSC_INPUT,        SSC_NUMBER,      "n_flux_days",               "No. days in flux map lookup",                                                                                                            "",       "",         "SolarPILOT",   "?=8",              "",                "" },
	{ SSC_INPUT,        SSC_NUMBER,      "delta_flux_hrs",            "Hourly frequency in flux map lookup",                                                                                                    "",       "",         "SolarPILOT",   "?=1",              "",                "" },

    { SSC_INPUT,        SSC_NUMBER,      "calc_fluxmaps",             "Include fluxmap calculations",                                                                                                           "",       "",         "SolarPILOT",   "?=0",              "",                "" },
	{ SSC_INPUT,        SSC_NUMBER,      "n_flux_x",                  "Flux map X resolution",                                                                                                                  "",       "",         "SolarPILOT",   "?=12",             "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "n_flux_y",                  "Flux map Y resolution",                                                                                                                  "",       "",         "SolarPILOT",   "?=1",              "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "check_max_flux",            "Check max flux at design point",                                                                                                         "",       "",         "SolarPILOT",   "?=0",              "",                "" },

    { SSC_INPUT,        SSC_NUMBER,      "tower_fixed_cost",          "Tower fixed cost",                                                                                                                       "$",      "",         "SolarPILOT",   "*",                "",                "" },
	{ SSC_INPUT,        SSC_NUMBER,      "tower_exp",                 "Tower cost scaling exponent",                                                                                                            "",       "",         "SolarPILOT",   "*",                "",                "" },
	{ SSC_INPUT,        SSC_NUMBER,      "rec_ref_cost",              "Receiver reference cost",                                                                                                                "$",      "",         "SolarPILOT",   "*",                "",                "" },
	{ SSC_INPUT,        SSC_NUMBER,      "rec_ref_area",              "Receiver reference area for cost scale",                                                                                                 "",       "",         "SolarPILOT",   "*",                "",                "" },
	{ SSC_INPUT,        SSC_NUMBER,      "rec_cost_exp",              "Receiver cost scaling exponent",                                                                                                         "",       "",         "SolarPILOT",   "*",                "",                "" },
	{ SSC_INPUT,        SSC_NUMBER,      "site_spec_cost",            "Site improvement cost",                                                                                                                  "$/m2",   "",         "SolarPILOT",   "*",                "",                "" },
	{ SSC_INPUT,        SSC_NUMBER,      "heliostat_spec_cost",       "Heliostat field cost",                                                                                                                   "$/m2",   "",         "SolarPILOT",   "*",                "",                "" },
	//{ SSC_INPUT,        SSC_NUMBER,      "plant_spec_cost",           "Power cycle specific cost",                                                                                                              "$/kWe",  "",         "SolarPILOT",   "*",                "",                "" },
	//{ SSC_INPUT,        SSC_NUMBER,      "bop_spec_cost",             "BOS specific cost",                                                                                                                      "$/kWe",  "",         "SolarPILOT",   "*",                "",                "" },
	//{ SSC_INPUT,        SSC_NUMBER,      "tes_spec_cost",             "Thermal energy storage cost",                                                                                                            "$/kWht", "",         "SolarPILOT",   "*",                "",                "" },
	{ SSC_INPUT,        SSC_NUMBER,      "land_spec_cost",            "Total land area cost",                                                                                                                   "$/acre", "",         "SolarPILOT",   "*",                "",                "" },
	{ SSC_INPUT,        SSC_NUMBER,      "contingency_rate",          "Contingency for cost overrun",                                                                                                           "%",      "",         "SolarPILOT",   "*",                "",                "" },
	{ SSC_INPUT,        SSC_NUMBER,      "sales_tax_rate",            "Sales tax rate",                                                                                                                         "%",      "",         "SolarPILOT",   "*",                "",                "" },
	{ SSC_INPUT,        SSC_NUMBER,      "sales_tax_frac",            "Percent of cost to which sales tax applies",                                                                                             "%",      "",         "SolarPILOT",   "*",                "",                "" },
	{ SSC_INPUT,        SSC_NUMBER,      "cost_sf_fixed",             "Solar field fixed cost",                                                                                                                 "$",      "",         "SolarPILOT",   "*",                "",                "" },
    
    { SSC_INPUT,        SSC_NUMBER,      "is_optimize",               "Do SolarPILOT optimization",                                                                                                             "",       "",         "SolarPILOT",   "?=0",              "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "flux_max",                  "Maximum allowable flux",                                                                                                                 "",       "",         "SolarPILOT",   "?=1000",           "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "opt_init_step",             "Optimization initial step size",                                                                                                         "",       "",         "SolarPILOT",   "?=0.05",           "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "opt_max_iter",              "Max. number iteration steps",                                                                                                            "",       "",         "SolarPILOT",   "?=200",            "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "opt_conv_tol",              "Optimization convergence tolerance",                                                                                                     "",       "",         "SolarPILOT",   "?=0.001",          "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "opt_algorithm",             "Optimization algorithm",                                                                                                                 "",       "",         "SolarPILOT",   "?=1",              "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "opt_flux_penalty",          "Optimization flux overage penalty",                                                                                                      "",       "",         "SolarPILOT",   "*",                "",                "" },
	{ SSC_INPUT,        SSC_MATRIX,      "helio_positions_in",        "Heliostat position table",                                                                                                               "",       "",         "SolarPILOT",   "",                 "",                "" },

	/* outputs */
	{ SSC_OUTPUT,       SSC_MATRIX,      "opteff_table",              "Optical efficiency (azi, zen, eff x nsim)",                                                                                              "",       "",         "SolarPILOT",   "*",                "",                "" },
	{ SSC_OUTPUT,       SSC_MATRIX,      "flux_table",                "Flux intensity table (flux(X) x (flux(y) x position)",                                                                                   "frac",   "",         "SolarPILOT",   "*",                "",                "" },
	{ SSC_OUTPUT,       SSC_MATRIX,      "heliostat_positions",       "Heliostat positions (x,y)",                                                                                                              "m",      "",         "SolarPILOT",   "*",                "",                "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "rec_assignments",           "Heliostat-receiver assignments for UI plotting",                                                                                         "",       "",         "SolarPILOT",   "*",                "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "number_heliostats",         "Number of heliostats",                                                                                                                   "",       "",         "SolarPILOT",   "*",                "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "area_sf",                   "Total reflective heliostat area",                                                                                                        "m^2",    "",         "SolarPILOT",   "*",                "",                "" },
	{ SSC_OUTPUT,       SSC_NUMBER,      "land_area_base_calc",       "Land area occupied by heliostats",                                                                                                       "acre",   "",         "SolarPILOT",   "*",                "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "total_land_area_calc",      "Total land area",                                                                                                                        "acre",   "",         "SolarPILOT",   "*",                "",                "" }, 
    { SSC_OUTPUT,       SSC_NUMBER,      "average_attenuation",       "Average solar field attenuation",                                                                                                        "%",      "",         "SolarPILOT",   "*",                "",                "" },


    { SSC_OUTPUT,       SSC_NUMBER,      "h_tower_opt",               "Optimized tower height",                                                                                                                 "m",      "",         "SolarPILOT",   "*",                "",                "" },
	{ SSC_OUTPUT,       SSC_NUMBER,      "rec_height_opt",            "Optimized receiver height",                                                                                                              "m",      "",         "SolarPILOT",   "*",                "",                "" },
	{ SSC_OUTPUT,       SSC_NUMBER,      "rec_aspect_opt",            "Optimized receiver aspect ratio",                                                                                                        "-",      "",         "SolarPILOT",   "receiver_type=0",  "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "cav_rec_aper_width_opt",    "Optimized cavity receiver aperture width",                                                                                               "-",      "",         "SolarPILOT",   "receiver_type=1",  "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "rec_width_opt",             "Optimized receiver width",                                                                                                               "m",      "",         "SolarPILOT",   "receiver_type=3",  "",                "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "rec_azimuth_opt",           "Optimized receiver azimuth",                                                                                                             "deg",    "",         "SolarPILOT",   "receiver_type=3",  "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "flux_max_observed",         "Maximum observed flux at design",                                                                                                        "kW/m2",  "",         "SolarPILOT",   "check_max_flux=1", "",                "" },

    { SSC_OUTPUT,       SSC_NUMBER,      "cost_rec_tot",              "Total receiver cost",                                                                                                                    "$",      "",         "SolarPILOT",   "*",                "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "cost_sf_tot",               "Total heliostat field cost",                                                                                                             "$",      "",         "SolarPILOT",   "*",                "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "cost_tower_tot",            "Total tower cost",                                                                                                                       "$",      "",         "SolarPILOT",   "*",                "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "cost_land_tot",             "Total land cost",                                                                                                                        "$",      "",         "SolarPILOT",   "*",                "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "cost_site_tot",             "Total site cost",                                                                                                                        "$",      "",         "SolarPILOT",   "*",                "",                "" },

	var_info_invalid };

//static bool solarpilot_callback( simulation_info *siminfo, void *data );
//static bool optimize_callback( simulation_info *siminfo, void *data );

#ifdef _MSC_VER
#define mysnprintf _snprintf
#else
#define mysnprintf snprintf
#endif

class cm_solarpilot : public compute_module
{
public:
	
	cm_solarpilot()
	{
		add_var_info( _cm_vtab_solarpilot );
	}

	void exec( )
	{
		std::shared_ptr<weather_data_provider> wdata = std::make_shared<weatherfile>(as_string("solar_resource_file"));
		solarpilot_invoke spi( this );
		spi.run(wdata);

        int rec_type = as_integer("receiver_type");

        // Post-process solarpilot outputs for receiver configs
        // Set outputs unique to receiver configs
        if (rec_type == 0) { // external
            assign("rec_aspect_opt", (ssc_number_t)spi.recs.front().rec_aspect.Val());
        }
        else if (rec_type == 1) {   // cavity

            double cav_rec_height_spout, cav_radius_spout, f_offset_spout;
            cav_rec_height_spout = cav_radius_spout = f_offset_spout = std::numeric_limits<double>::quiet_NaN();
            int n_panels_spout = -1;

            cav_rec_height_spout = spi.recs.front().rec_height.val;   //[m]
            cav_radius_spout = spi.recs.front().rec_cav_rad.val;      //[m]
            f_offset_spout = spi.recs.front().rec_cav_cdepth.val;     //[-]
            n_panels_spout = spi.recs.front().n_panels.val;           //[-]

            double theta0_calc, panelSpan_calc, panel_width_calc, rec_area_calc, rec_width_calc,
                rec_span_calc, offset_calc;

            cavity_receiver_helpers::calc_receiver_macro_geometry_sp_inputs(cav_rec_height_spout, cav_radius_spout,
                        f_offset_spout, n_panels_spout,
                        theta0_calc, panelSpan_calc, panel_width_calc,
                        rec_area_calc, rec_width_calc, rec_span_calc, offset_calc);

            // compare input rec_span w/ calculated rec_span
            // should be same even after optimization, because optimization fixes offset/radius
            //double rec_span_in = as_double("cav_rec_span")*PI/180.0;     //[rad] convert from deg

            assign("cav_rec_aper_width_opt", (ssc_number_t)rec_width_calc);
        }
        else if (rec_type == 3) { // free-falling receiver
            assign("rec_width_opt", (ssc_number_t)spi.recs.front().rec_width.val);

            ssc_number_t* rec_azi_opt = allocate("rec_azimuth_opt", spi.recs.size());
            for (size_t i = 0; i < spi.recs.size(); i++) {
                rec_azi_opt[i] = (ssc_number_t)spi.recs.at(i).rec_azimuth.val;
            }
        }

		assign("h_tower_opt", (ssc_number_t)spi.sf.tht.val);
        assign("rec_height_opt", (ssc_number_t)spi.recs.front().rec_height.val);
		assign("cost_rec_tot", (ssc_number_t)spi.fin.rec_cost.Val());
		assign("cost_sf_tot", (ssc_number_t)spi.fin.heliostat_cost.Val());
		assign("cost_tower_tot", (ssc_number_t)spi.fin.tower_cost.Val());
		assign("cost_land_tot", (ssc_number_t)spi.fin.land_cost.Val());
		assign("cost_site_tot", (ssc_number_t)spi.fin.site_cost.Val());
		assign("area_sf", (ssc_number_t)spi.sf.sf_area.Val());

        //return the land area
        assign("land_area_base_calc", (ssc_number_t)spi.GetBaseLandArea());         //[acre]
        assign("total_land_area_calc", (ssc_number_t)spi.GetTotalLandArea());       //[acre]
        assign("average_attenuation", (ssc_number_t)spi.CalcAveAttenuation());     //[%]

        if( is_assigned("helio_positions_in") )
        {
            util::matrix_t<double> hposin = as_matrix("helio_positions_in");
			ssc_number_t *hpos = allocate( "heliostat_positions",  hposin.nrows(), 2 );
			for(size_t i=0; i< hposin.nrows(); i++){
				hpos[i * 2] = (ssc_number_t)hposin.at(i, 0);
				hpos[i * 2 + 1] = (ssc_number_t)hposin.at(i, 1);
			}
            //return the number of heliostats
			assign("number_heliostats", (ssc_number_t)hposin.nrows());
        }
        else
        {
		    //Collect the heliostat position data
		    if(  spi.layout.heliostat_positions.size() > 0 ) {
			    ssc_number_t *hpos = allocate( "heliostat_positions",  spi.layout.heliostat_positions.size(), 2 );
                ssc_number_t* rec_assign = allocate("rec_assignments", spi.layout.heliostat_positions.size());
			    for(size_t i=0; i< spi.layout.heliostat_positions.size(); i++){
				    hpos[i * 2    ] = (float) spi.layout.heliostat_positions.at(i).location.x;
				    hpos[i * 2 + 1] = (float) spi.layout.heliostat_positions.at(i).location.y;
                    rec_assign[i] = (int)spi.layout.heliostat_positions.at(i).which_rec;
			    }            
		    }
		    else
			    throw exec_error("solarpilot", "failed to generate a heliostat field layout");
 		    
            //return the number of heliostats
			assign("number_heliostats", (ssc_number_t)spi.layout.heliostat_positions.size());
        }

		//check if flux map calculations are desired
		if( as_boolean("calc_fluxmaps") ){
            // [deg, deg, - (per receiver)] Azimuth, zenith, solar field efficiency map
            util::matrix_t<ssc_number_t>& eta_map = allocate_matrix("opteff_table", spi.fluxtab.efficiency.size(), 2 + spi.fluxtab.efficiency.front().size());
            spi.getHeliostatFieldEfficiency(eta_map);

			//collect the flux map data
            block_t<double>* flux_data = &spi.fluxtab.flux_surfaces.front().flux_data;  //there should be only one flux stack for SAM
            int nflux_rows = (int)flux_data->nrows();
            int nflux_cols = (int)flux_data->ncols();

            if (rec_type == 1) {
                nflux_cols = spi.fluxtab.flux_surfaces.size() - 1;
            }
            else if(rec_type == 3) {
                nflux_cols *= spi.recs.size();
            }

            util::matrix_t<ssc_number_t>& flux_maps = allocate_matrix("flux_table", nflux_rows * flux_data->nlayers(), nflux_cols);
            spi.getReceiverFluxMaps(flux_maps);
		}
		else 
		{
			//fluxmaps not required, so declare required variables and fill with zeros
			allocate("opteff_table", 1, 3);
			allocate("flux_table", 1, 1);
		}

	}
};


DEFINE_MODULE_ENTRY( solarpilot, "SolarPILOT - CSP tower solar field layout tool.", 0 )
