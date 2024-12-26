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

// for adjustment factors
#include "common.h"

// solarpilot header files
#include "AutoPilot_API.h"
#include "SolarField.h"
#include "IOUtil.h"
#include "csp_common.h"

// Can probably delete these headers later...
#include "csp_solver_util.h"
#include "csp_solver_core.h"
#include "csp_solver_pt_sf_perf_interp.h"
#include "csp_solver_falling_particle_receiver.h"
#include "csp_solver_falling_particle_collector_receiver.h"
#include "csp_solver_pc_Rankine_indirect_224.h"
#include "csp_solver_two_tank_tes.h"

#include "csp_dispatch.h"

#include "csp_solver_cr_electric_resistance.h"
#include "csp_solver_cavity_receiver.h"

#include "csp_system_costs.h"

#include <ctime>

//#define TESTING

static var_info _cm_vtab_csp_tower_particle[] = {

    // VARTYPE       DATATYPE    NAME                                  LABEL                                                                                                                                      UNITS           META                                 GROUP                                       REQUIRED_IF                                                         CONSTRAINTS      UI_HINTS
    { SSC_INPUT,     SSC_STRING, "solar_resource_file",                "Local weather file path",                                                                                                                 "",             "",                                  "Solar Resource",                           "?",                                                                "LOCAL_FILE",    ""},
    { SSC_INPUT,     SSC_TABLE,  "solar_resource_data",                "Weather resource data in memory",                                                                                                         "",             "",                                  "Solar Resource",                           "?",                                                                "",              "SIMULATION_PARAMETER"},

    // Simulation parameters
    { SSC_INPUT,     SSC_NUMBER, "is_dispatch",                        "Allow dispatch optimization?",                                                                                                            "",             "",                                  "System Control",                           "?=0",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "sim_type",                           "1 (default): time-series, 2: design only",                                                                                                "",             "",                                  "System Control",                           "?=1",                                                              "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_NUMBER, "csp_financial_model",                "",                                                                                                                                        "1-8",          "",                                  "Financial Model",                          "?=1",                                                              "INTEGER,MIN=0", ""},
    { SSC_INPUT,     SSC_NUMBER, "time_start",                         "Simulation start time",                                                                                                                   "s",            "",                                  "System Control",                           "?=0",                                                              "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_NUMBER, "time_stop",                          "Simulation stop time",                                                                                                                    "s",            "",                                  "System Control",                           "?=31536000",                                                       "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_NUMBER, "time_steps_per_hour",                "Number of simulation time steps per hour",                                                                                                "",             "",                                  "System Control",                           "?=-1",                                                             "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_NUMBER, "vacuum_arrays",                      "Allocate arrays for only the required number of steps",                                                                                   "",             "",                                  "System Control",                           "?=0",                                                              "",              "SIMULATION_PARAMETER"},

    // System Design
    { SSC_INPUT,     SSC_NUMBER, "is_parallel_htr",                    "Does plant include a HTF heater parallel to solar field?",                                                                                "",             "",                                  "System Control",                           "?=0",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "T_htf_cold_des",                     "Cold HTF inlet temperature at design conditions",                                                                                         "C",            "",                                  "System Design",                            "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "T_htf_hot_des",                      "Hot HTF outlet temperature at design conditions",                                                                                         "C",            "",                                  "System Design",                            "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "P_ref",                              "Reference output electric power at design condition",                                                                                     "MW",           "",                                  "System Design",                            "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "design_eff",                         "Power cycle efficiency at design",                                                                                                        "none",         "",                                  "System Design",                            "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "tshours",                            "Equivalent full-load thermal storage hours",                                                                                              "hr",           "",                                  "System Design",                            "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "solarm",                             "Solar multiple",                                                                                                                          "-",            "",                                  "System Design",                            "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "dni_des",                            "Design-point DNI",                                                                                                                        "W/m2",         "",                                  "System Design",                            "*",                                                                "",              ""},

    // Solar field
    { SSC_INPUT,     SSC_NUMBER, "field_model_type",                   "0=optimize field and tower/receiver geometry, 1=design field, 2=user specified field, 3=user flux and eta map, pass heliostat_positions to SolarPILOT for layout, 4=user flux and eta maps, no SolarPILOT, input A_sf_in, total_land_area_in, and N_hel", "", "", "Heliostat Field", "*",                      "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "is_design_field_for_sim_type_2",     "0 = no (default), design-only call to cmod will use heliostat_positions, 1 = use SolarPILOT to layout field during design call to cmod (takes time)", "", "",                                  "Heliostat Field",                          "?=0",                                                              "",              ""},    
    { SSC_INPUT,     SSC_NUMBER, "helio_width",                        "Heliostat width",                                                                                                                         "m",            "",                                  "Heliostat Field",                          "field_model_type<4",                                               "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "helio_height",                       "Heliostat height",                                                                                                                        "m",            "",                                  "Heliostat Field",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "helio_optical_error_mrad",           "Heliostat optical error",                                                                                                                 "mrad",         "",                                  "Heliostat Field",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "helio_active_fraction",              "Heliostat active fraction",                                                                                                               "",             "",                                  "Heliostat Field",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "dens_mirror",                        "Ratio of heliostat reflective area to profile",                                                                                           "",             "",                                  "Heliostat Field",                          "field_model_type<4",                                               "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "helio_reflectance",                  "Heliostat reflectance",                                                                                                                   "",             "",                                  "Heliostat Field",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "land_max",                           "Land max boundary",                                                                                                                       "-",            "",                                  "Heliostat Field",                          "?=7.5",                                                            "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "land_min",                           "Land min boundary",                                                                                                                       "-",            "",                                  "Heliostat Field",                          "?=0.75",                                                           "",              ""},
    { SSC_INPUT,     SSC_MATRIX, "land_bound_table",                   "Land boundary table",                                                                                                                     "m",            "",                                  "Heliostat Field",                          "?",                                                                "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_ARRAY,  "land_bound_list",                    "Land boundary table listing",                                                                                                             "",             "",                                  "Heliostat Field",                          "?",                                                                "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_NUMBER, "p_start",                            "Heliostat startup energy",                                                                                                                "kWe-hr",       "",                                  "Heliostat Field",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "p_track",                            "Heliostat tracking power",                                                                                                                "kWe",          "",                                  "Heliostat Field",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "hel_stow_deploy",                    "Stow/deploy elevation angle",                                                                                                             "deg",          "",                                  "Heliostat Field",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "v_wind_max",                         "Heliostat max wind velocity",                                                                                                             "m/s",          "",                                  "Heliostat Field",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_MATRIX, "eta_map",                            "Field efficiency array",                                                                                                                  "",             "",                                  "Heliostat Field",                          "field_model_type>2",                                               "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_NUMBER, "eta_map_aod_format",                 "Use 3D AOD format field efficiency array",                                                                                                "",             "heliostat",                         "Heliostat Field",                          "field_model_type>2",                                               "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_MATRIX, "flux_maps",                          "Flux map intensities",                                                                                                                    "",             "",                                  "Heliostat Field",                          "field_model_type>2",                                               "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_NUMBER, "c_atm_0",                            "Attenuation coefficient 0",                                                                                                               "",             "",                                  "Heliostat Field",                          "?=0.006789",                                                       "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "c_atm_1",                            "Attenuation coefficient 1",                                                                                                               "",             "",                                  "Heliostat Field",                          "?=0.1046",                                                         "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "c_atm_2",                            "Attenuation coefficient 2",                                                                                                               "",             "",                                  "Heliostat Field",                          "?=-0.0107",                                                        "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "c_atm_3",                            "Attenuation coefficient 3",                                                                                                               "",             "",                                  "Heliostat Field",                          "?=0.002845",                                                       "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "n_facet_x",                          "Number of heliostat facets - X",                                                                                                          "",             "",                                  "Heliostat Field",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "n_facet_y",                          "Number of heliostat facets - Y",                                                                                                          "",             "",                                  "Heliostat Field",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "focus_type",                         "Heliostat focus method",                                                                                                                  "",             "",                                  "Heliostat Field",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "cant_type",                          "Heliostat canting method",                                                                                                                "",             "",                                  "Heliostat Field",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "water_usage_per_wash",               "Water usage per wash",                                                                                                                    "L/m2_aper",    "",                                  "Heliostat Field",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "washing_frequency",                  "Mirror washing frequency",                                                                                                                "none",         "",                                  "Heliostat Field",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "check_max_flux",                     "Check max flux at design point",                                                                                                          "",             "",                                  "Heliostat Field",                          "?=0",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "sun_loc_des",                        "Sun location at design point (0 = Summer solstice, 1 = Equinox, 2 = Winter solstice)",                                                    "",             "",                                  "Heliostat Field",                          "?=0",                                                              "",              ""},

    // Inputs required for user-defined SF performance when field_model_type = 4
    // Values can be defined by mapping to equivalent _calc output for simulation results with field_model_type < 3
    { SSC_INPUT,     SSC_ARRAY,  "A_sf_in",                            "Solar field area",                                                                                                                        "m^2",          "",                                  "Heliostat Field",                          "field_model_type>3",                                               "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_NUMBER, "total_land_area_in",                 "Total land area - in",                                                                                                                    "acre",         "",                                  "Heliostat Field",                          "field_model_type>3",                                               "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_ARRAY,  "N_hel",                              "Number of heliostats - in",                                                                                                               "",             "",                                  "Heliostat Field",                          "field_model_type>3",                                               "",              "SIMULATION_PARAMETER"},

    { SSC_INPUT,     SSC_NUMBER, "flux_max",                           "Maximum allowable flux",                                                                                                                  "",             "",                                  "Tower and Receiver",                       "?=1000",                                                           "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "opt_init_step",                      "Optimization initial step size",                                                                                                          "",             "",                                  "Heliostat Field",                          "?=0.05",                                                           "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "opt_max_iter",                       "Max number iteration steps",                                                                                                              "",             "",                                  "Heliostat Field",                          "?=200",                                                            "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "opt_conv_tol",                       "Optimization convergence tolerance",                                                                                                      "",             "",                                  "Heliostat Field",                          "?=0.001",                                                          "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "opt_flux_penalty",                   "Optimization flux overage penalty",                                                                                                       "",             "",                                  "Heliostat Field",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "opt_algorithm",                      "Optimization algorithm",                                                                                                                  "",             "",                                  "Heliostat Field",                          "?=1",                                                              "",              ""},

    // Receiver parameters - general
    { SSC_INPUT,     SSC_NUMBER, "num_recs",                           "Number of receivers",                                                                                                                     "",             "",                                  "Tower and Receiver",                       "?=1",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "is_sep_rec_modes",                   "Are separate operating modes per receiver allowed?",                                                                                      "",             "",                                  "Tower and Receiver",                       "?=1",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "is_recs_duplicate",                  "1 = All receivers have the same power and geometry; 0 = otherwise",                                                                       "",             "",                                  "Tower and Receiver",                       "?=1",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "rec_htf",                            "Receiver HTF, 36=Bauxite particles 50=Lookup tables",                                                                                     "",             "",                                  "Tower and Receiver",                       "?=36",                                                             "",              ""},
    { SSC_INPUT,     SSC_MATRIX, "field_fl_props",                     "User-defined field fluid property data",                                                                                                  "-",            "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "f_rec_min",                          "Minimum receiver mass flow rate turn down fraction",                                                                                      "",             "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "rec_su_delay",                       "Fixed startup delay time for the receiver",                                                                                               "hr",           "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "rec_qf_delay",                       "Energy-based receiver startup delay (fraction of rated thermal power)",                                                                   "",             "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "csp.pt.rec.max_oper_frac",           "Maximum receiver mass flow rate fraction",                                                                                                "",             "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "eta_lift",                           "Receiver particle lift efficiency",                                                                                                       "",             "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},

    { SSC_INPUT,     SSC_NUMBER, "rec_clearsky_model",				   "Clearsky model: None = -1, User-defined data = 0, Meinel = 1; Hottel = 2; Allen = 3; Moon = 4",											  "",             "",                                  "Tower and Receiver",                       "?=-1",															   "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_ARRAY,  "rec_clearsky_dni",				   "User-defined clear-sky DNI",																											  "W/m2",         "",                                  "Tower and Receiver",                       "rec_clearsky_model=0",											   "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_NUMBER, "rec_clearsky_fraction",              "Weighting fraction on clear-sky DNI for receiver flow control",                                                                           "",             "",                                  "Tower and Receiver",                       "?=0.0",                                                            "",              "SIMULATION_PARAMETER"},

    // New variables replacing deprecated variable "piping_loss". Variable currently not required so exec() can check if assigned and throw a more detailed error
    { SSC_INPUT,     SSC_NUMBER, "transport_deltaT_hot",               "Temperature loss for hot particle transport",                                                                                             "K",            "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "transport_deltaT_cold",              "Temperature loss for cold particle transport",                                                                                            "K",            "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},

    // Falling particle receiver inputs for SolarPILOT that should *not* be reset during call to this cmod
    { SSC_INPUT,     SSC_MATRIX, "rec_tower_offset",                   "Distance from tower optical height to receiver aperture center, format [[x1,y1],[x2,y2],...] or [[x1,y1,z1],[x2,y2,z2],...]",             "m",            "",                                  "Tower and Receiver",                       "?",                                                                "",              ""},
    { SSC_INPUT,     SSC_ARRAY,  "rec_azimuth",                        "Receiver azimuth orientation: 0 deg is north, positive clockwise",                                                                        "deg",          "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_ARRAY,  "power_fraction",                     "Target fraction of absorbed energy delivered by the heliostat field in configurations with multiple receivers",                           "",             "",                                  "Tower and Receiver",                       "?",                                                                "",              ""},
    { SSC_INPUT,     SSC_ARRAY,  "norm_curtain_height",                "Normalized particle curtain height",                                                                                                      "",             "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_ARRAY,  "norm_curtain_width",                 "Normalized particle curtain width",                                                                                                       "",             "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_ARRAY,  "max_curtain_depth",                  "Particle curtain entrance depth",                                                                                                         "m",            "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_MATRIX, "norm_heights_depths",                "Normalized troughs heights and depths, pass [[0,0]] for no curtain troughs",                                                              "",             "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "curtain_type",                       "Flat=0;Curved=1",                                                                                                                         "",             "",                                  "Tower and Receiver",                       "?=0",                                                              "",              ""},
    //{ SSC_INPUT,     SSC_NUMBER, "curtain_radius",                     "Particle curtain radius",                                                                                                                 "m",            "",                                  "Tower and Receiver",                       "curtain_type=1",                                                   "",              "" },
    { SSC_INPUT,     SSC_ARRAY,  "is_snout",                           "Is SNOUT enabled?",                                                                                                                       "",             "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_ARRAY,  "snout_depth",                        "Distance from aperture window to SNOUT front plane",                                                                                      "m",            "",                                  "Tower and Receiver",                       "?",                                                                "",              ""},
    { SSC_INPUT,     SSC_ARRAY,  "snout_horiz_angle",                  "SNOUT spanning angle defined in the aperture vertical mid-plane",                                                                         "deg",          "",                                  "Tower and Receiver",                       "?",                                                                "",              ""},
    { SSC_INPUT,     SSC_ARRAY,  "snout_vert_bot_angle",               "SNOUT bottom surface angle from aperture normal",                                                                                         "deg",          "",                                  "Tower and Receiver",                       "?",                                                                "",              ""},
    { SSC_INPUT,     SSC_ARRAY,  "snout_vert_top_angle",               "SNOUT top surface angle from aperture normal",                                                                                            "deg",          "",                                  "Tower and Receiver",                       "?",                                                                "",              ""},

    { SSC_INPUT,     SSC_NUMBER, "n_flux_days",                        "Number of days in flux map lookup",                                                                                                       "",             "",                                  "Tower and Receiver",                       "?=8",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "delta_flux_hrs",                     "Hourly frequency in flux map lookup",                                                                                                     "",             "",                                  "Tower and Receiver",                       "?=1",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "n_flux_x",                           "Flux map X resolution",                                                                                                                   "",             "",                                  "Tower and Receiver",                       "?=3",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "n_flux_y",                           "Flux map Y resolution",                                                                                                                   "",             "",                                  "Tower and Receiver",                       "?=12",                                                             "",              ""},

    // Falling particle receiver thermal model parameters 
    { SSC_INPUT,     SSC_NUMBER, "rec_model_type",                     "Receiver model type (0 = fixed efficiency, 1 = Sandia FFPR 0D correlation, 2 = Sandia MFPR 0D correlation, 3 = Quasi 2D model)",          "",             "",                                  "Tower and Receiver",                       "?=3",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "rec_rad_model_type",                 "Receiver radiation model type (0 = Simplified formulation from Sandia, 1 = Full formulation)",                                            "",             "",                                  "Tower and Receiver",                       "?=1",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "rec_adv_model_type",                 "Receiver advection model type (0 = Fixed advective loss coefficient, 1 = Correlations from Sandia)",                                      "",             "",                                  "Tower and Receiver",                       "?=1",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "rec_eta_fixed",                      "User-provided fixed receiver efficiency (fraction)",                                                                                      "",             "",                                  "Tower and Receiver",                       "rec_model_type=0",                                                 "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "rec_hadv_fixed",                     "User-provided fixed receiver advective loss coefficient",                                                                                 "W/m2/K",       "",                                  "Tower and Receiver",                       "rec_adv_model_type=0",                                             "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "rec_ny",                             "Number of curtain elements in the vertical dimension",                                                                                    "",             "",                                  "Tower and Receiver",                       "?=13",                                                             "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "rec_nx",                             "Number of curtain elements in the width dimension",                                                                                       "",             "",                                  "Tower and Receiver",                       "?=3",                                                              "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "rec_rad_ny",                         "Number of curtain element groups in vertical dimension for radiative exchange",                                                           "",             "",                                  "Tower and Receiver",                       "?=5",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "rec_rad_nx",                         "Number of curtain element groups in width dimension for radiative exchange",                                                              "",             "",                                  "Tower and Receiver",                       "?=3",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "particle_dp",                        "Particle diameter",                                                                                                                       "m",            "",                                  "Tower and Receiver",                       "?=350e-6",                                                         "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "particle_abs",                       "Particle absorptivity",                                                                                                                   "",             "",                                  "Tower and Receiver",                       "?=0.9",                                                            "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "curtain_emis",                       "Curtain emissivity",                                                                                                                      "",             "",                                  "Tower and Receiver",                       "?=0.9",                                                            "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "curtain_dthdy",                      "Rate of curtain thickness increase with respect to fall distance",                                                                        "",             "",                                  "Tower and Receiver",                       "?=0.0087",                                                         "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "cav_abs",                            "Cavity wall solar absorptivity",                                                                                                          "",             "",                                  "Tower and Receiver",                       "?=0.2",                                                            "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "cav_emis",                           "Cavity wall IR emissivity",                                                                                                               "",             "",                                  "Tower and Receiver",                       "?=0.8",                                                            "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "cav_twall",                          "Cavity wall thickness",                                                                                                                   "m",            "",                                  "Tower and Receiver",                       "?=0.05",                                                           "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "cav_kwall",                          "Cavity wall thermal conductivity",                                                                                                        "W/m/K",        "",                                  "Tower and Receiver",                       "?=0.2",                                                            "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "cav_hext",                           "Cavity wall external convective loss coefficient",                                                                                        "W/m2/K",       "",                                  "Tower and Receiver",                       "?=10.0",                                                           "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "rec_tauc_mult",                      "User-provided multiplier for calculated curtain transmissivity",                                                                          "",             "",                                  "Tower and Receiver",                       "?=1.0",                                                            "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "rec_hadv_mult",                      "User-provided multiplier for calculated advective loss coefficient",                                                                      "",             "",                                  "Tower and Receiver",                       "?=1.0",                                                            "",              ""},

        // Field layout and tower/receiver dimensions
        // If field_model_type = 1, tower/receiver dimensions are used as guess values
        //        and optimized values are reported as _calc outputs
    { SSC_INPUT,     SSC_MATRIX, "helio_positions",                    "Heliostat position table - in",                                                                                                           "",             "",                                  "Heliostat Field",                          "field_model_type=2|field_model_type=3",                            "", "COL_LABEL=XY_POSITION" },
    { SSC_INPUT,     SSC_ARRAY,  "fp_rec_height",                      "Receiver height - in",                                                                                                                    "m",            "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_ARRAY,  "rec_width",                          "Receiver width - in",                                                                                                                     "m",            "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "h_tower",                            "Tower height - in",                                                                                                                       "m",            "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},

    // Parallel heater parameters
    { SSC_INPUT,     SSC_NUMBER, "heater_mult",                        "Heater multiple relative to design cycle thermal power",                                                                                  "-",            "",                                  "Parallel Heater",                          "is_parallel_htr=1",                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "heater_efficiency",                  "Heater electric to thermal efficiency",                                                                                                   "%",            "",                                  "Parallel Heater",                          "is_parallel_htr=1",                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "f_q_dot_des_allowable_su",           "Fraction of design power allowed during startup",                                                                                         "-",            "",                                  "Parallel Heater",                          "is_parallel_htr=1",                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "hrs_startup_at_max_rate",            "Duration of startup at max startup power",                                                                                                "hr",           "",                                  "Parallel Heater",                          "is_parallel_htr=1",                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "f_q_dot_heater_min",                 "Minimum allowable heater output as fraction of design",                                                                                   "",             "",                                  "Parallel Heater",                          "is_parallel_htr=1",                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "disp_hsu_cost_rel",                  "Heater startup cost",                                                                                                                     "$/MWt/start",  "",                                  "System Control",                           "is_dispatch=1&is_parallel_htr=1",                                  "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "heater_spec_cost",                   "Heater specific cost",                                                                                                                    "$/kWht",       "",                                  "System Costs",                             "is_parallel_htr=1",                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "allow_heater_no_dispatch_opt",       "Allow heater with no dispatch optimization? SAM UI relies on cmod default",                                                               "",             "",                                  "System Costs",                             "?=0",                                                              "",              "SIMULATION_PARAMETER" },


    // TES parameters - general
    { SSC_INPUT,     SSC_NUMBER, "tes_init_hot_htf_percent",           "Initial fraction of available volume that is hot",                                                                                        "%",            "",                                  "Thermal Storage",                          "*", /*not required because replacing deprecated var and checked in cmod*/ "",        ""},
    { SSC_INPUT,     SSC_NUMBER, "h_tank",                             "Total height of bin (height of HTF when bin is full)",                                                                                  "m",            "",                                  "Thermal Storage",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "cold_tank_max_heat",                 "Rated heater capacity for cold bin heating",                                                                                             "MW",           "",                                  "Thermal Storage",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "u_tank",                             "Loss coefficient from the bin",                                                                                                          "W/m2-K",       "",                                  "Thermal Storage",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "tank_pairs",                         "Number of equivalent bin pairs",                                                                                                         "",             "",                                  "Thermal Storage",                          "*",                                                                "INTEGER",       ""},
    { SSC_INPUT,     SSC_NUMBER, "cold_tank_Thtr",                     "Minimum allowable cold bin HTF temperature",                                                                                             "C",            "",                                  "Thermal Storage",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "packed_vol_frac",                    "Packed volume fraction",                                                                                                                  "-",            "",                                  "Thermal Storage",                          "*",                                                                "",              ""},
    // TES parameters - 2 tank
    { SSC_INPUT,     SSC_NUMBER, "h_tank_min",                         "Minimum allowable HTF height in storage bin",                                                                                            "m",            "",                                  "Thermal Storage",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "hot_tank_Thtr",                      "Minimum allowable hot bin HTF temperature",                                                                                              "C",            "",                                  "Thermal Storage",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "hot_tank_max_heat",                  "Rated heater capacity for hot bin heating",                                                                                              "MW",           "",                                  "Thermal Storage",                          "*",                                                                "",              ""},

    // Power Cycle Inputs
    { SSC_INPUT,     SSC_NUMBER, "pc_config",                          "PC configuration 0=Steam Rankine (224), 1=user defined",                                                                                  "",             "",                                  "Power Cycle",                              "?=0",                                                              "INTEGER",       ""},
    { SSC_INPUT,     SSC_NUMBER, "phx_height",                         "Height particles travel from Hot TES outlet to the top of the PHX",                                                                       "m",            "",                                  "Power Cycle",                              "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "startup_time",                       "Time needed for power block startup",                                                                                                     "hr",           "",                                  "Power Cycle",                              "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "startup_frac",                       "Fraction of design thermal power needed for startup",                                                                                     "none",         "",                                  "Power Cycle",                              "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "cycle_max_frac",                     "Maximum turbine over design operation fraction",                                                                                          "",             "",                                  "Power Cycle",                              "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "cycle_cutoff_frac",                  "Minimum turbine operation fraction before shutdown",                                                                                      "",             "",                                  "Power Cycle",                              "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "q_sby_frac",                         "Fraction of thermal power required for standby",                                                                                          "",             "",                                  "Power Cycle",                              "*",                                                                "",              ""},

    // Steam Rankine cycle
    { SSC_INPUT,     SSC_NUMBER, "dT_cw_ref",                          "Reference condenser cooling water inlet/outlet temperature difference",                                                                   "C",            "",                                  "Rankine Cycle",                            "pc_config=0",                                                      "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "T_amb_des",                          "Reference ambient temperature at design point",                                                                                           "C",            "",                                  "Rankine Cycle",                            "pc_config=0",                                                      "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "CT",                                 "Condenser type: 1=evaporative, 2=air",                                                                                                    "",             "",                                  "Rankine Cycle",                            "pc_config=0",                                                      "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "T_approach",                         "Cooling tower approach temperature",                                                                                                      "C",            "",                                  "Rankine Cycle",                            "pc_config=0",                                                      "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "T_ITD_des",                          "ITD at design for dry system",                                                                                                            "C",            "",                                  "Rankine Cycle",                            "pc_config=0",                                                      "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "P_cond_ratio",                       "Condenser pressure ratio",                                                                                                                "",             "",                                  "Rankine Cycle",                            "pc_config=0",                                                      "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "pb_bd_frac",                         "Power block blow-down steam fraction",                                                                                                    "",             "",                                  "Rankine Cycle",                            "pc_config=0",                                                      "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "P_cond_min",                         "Minimum condenser pressure",                                                                                                              "inHg",         "",                                  "Rankine Cycle",                            "pc_config=0",                                                      "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "n_pl_inc",                           "Number of part-load increments for the heat rejection system",                                                                            "none",         "",                                  "Rankine Cycle",                            "pc_config=0",                                                      "INTEGER",       ""},
    { SSC_INPUT,     SSC_NUMBER, "tech_type",                          "Turbine inlet pressure control 1=Fixed, 3=Sliding",                                                                                       "",             "",                                  "Rankine Cycle",                            "pc_config=0",                                                      "",              ""},

    // User Defined cycle
    { SSC_INPUT,     SSC_NUMBER, "ud_f_W_dot_cool_des",                "Percent of user-defined power cycle design gross output consumed by cooling",                                                             "%",            "",                                  "User Defined Power Cycle",                 "pc_config=1",                                                      "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "ud_m_dot_water_cool_des",            "Mass flow rate of water required at user-defined power cycle design point",                                                               "kg/s",         "",                                  "User Defined Power Cycle",                 "pc_config=1",                                                      "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "ud_is_sco2_regr",                    "False: default, base udpc interpolation, True: use sco2 heuristic regression",                                                            "",             "",                                  "User Defined Power Cycle",                 "?=0",                                                              "",              ""},
    { SSC_INPUT,     SSC_MATRIX, "ud_ind_od",                          "Off design user-defined power cycle performance as function of T_htf, m_dot_htf [ND], and T_amb",                                         "",             "",                                  "User Defined Power Cycle",                 "pc_config=1",                                                      "",              ""},

    // Aux and Balance of Plant
    { SSC_INPUT,     SSC_NUMBER, "pb_fixed_par",                       "Fixed parasitic load - runs at all times",                                                                                                "MWe/MWcap",    "",                                  "System Control",                           "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "aux_par",                            "Aux heater, boiler parasitic",                                                                                                            "MWe/MWcap",    "",                                  "System Control",                           "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "aux_par_f",                          "Aux heater, boiler parasitic - multiplying fraction",                                                                                     "",             "",                                  "System Control",                           "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "aux_par_0",                          "Aux heater, boiler parasitic - constant coefficient",                                                                                     "",             "",                                  "System Control",                           "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "aux_par_1",                          "Aux heater, boiler parasitic - linear coefficient",                                                                                       "",             "",                                  "System Control",                           "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "aux_par_2",                          "Aux heater, boiler parasitic - quadratic coefficient",                                                                                    "",             "",                                  "System Control",                           "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "bop_par",                            "Balance of plant parasitic power fraction",                                                                                               "MWe/MWcap",    "",                                  "System Control",                           "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "bop_par_f",                          "Balance of plant parasitic power fraction - mult frac",                                                                                   "",             "",                                  "System Control",                           "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "bop_par_0",                          "Balance of plant parasitic power fraction - const coeff",                                                                                 "",             "",                                  "System Control",                           "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "bop_par_1",                          "Balance of plant parasitic power fraction - linear coeff",                                                                                "",             "",                                  "System Control",                           "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "bop_par_2",                          "Balance of plant parasitic power fraction - quadratic coeff",                                                                             "",             "",                                  "System Control",                           "*",                                                                "",              ""},

    // System Control
    { SSC_INPUT,     SSC_ARRAY,  "timestep_load_fractions",            "Turbine load fraction for each timestep, alternative to block dispatch",                                                                  "",             "",                                  "System Control",                           "?",                                                                "",              "SIMULATION_PARAMETER" },
    { SSC_INPUT,     SSC_ARRAY,  "f_turb_tou_periods",                 "Dispatch logic for turbine load fraction",                                                                                                "",             "",                                  "System Control",                           "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_MATRIX, "weekday_schedule",                   "12x24 CSP operation Time-of-Use Weekday schedule",                                                                                        "",             "",                                  "System Control",                           "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_MATRIX, "weekend_schedule",                   "12x24 CSP operation Time-of-Use Weekend schedule",                                                                                        "",             "",                                  "System Control",                           "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "is_tod_pc_target_also_pc_max",       "Is the TOD target cycle heat input also the max cycle heat input?",                                                                       "",             "",                                  "System Control",                           "?=0",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "can_cycle_use_standby",              "Can the cycle use standby operation?",                                                                                                    "",             "",                                  "System Control",                           "?=0",                                                              "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_NUMBER, "disp_horizon",                       "Time horizon for dispatch optimization",                                                                                                  "hour",         "",                                  "System Control",                           "is_dispatch=1",                                                    "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "disp_frequency",                     "Frequency for dispatch optimization calculations",                                                                                        "hour",         "",                                  "System Control",                           "is_dispatch=1",                                                    "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "disp_max_iter",                      "Max number of dispatch optimization iterations",                                                                                          "",             "",                                  "System Control",                           "is_dispatch=1",                                                    "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "disp_timeout",                       "Max dispatch optimization solve duration",                                                                                                "s",            "",                                  "System Control",                           "is_dispatch=1",                                                    "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "disp_mip_gap",                       "Dispatch optimization solution tolerance",                                                                                                "",             "",                                  "System Control",                           "is_dispatch=1",                                                    "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "disp_spec_bb",                       "Dispatch optimization B&B heuristic",                                                                                                     "",             "",                                  "System Control",                           "?=-1",                                                             "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_NUMBER, "disp_reporting",                     "Dispatch optimization reporting level",                                                                                                   "",             "",                                  "System Control",                           "?=-1",                                                             "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_NUMBER, "disp_spec_presolve",                 "Dispatch optimization presolve heuristic",                                                                                                "",             "",                                  "System Control",                           "?=-1",                                                             "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_NUMBER, "disp_spec_scaling",                  "Dispatch optimization scaling heuristic",                                                                                                 "",             "",                                  "System Control",                           "?=-1",                                                             "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_NUMBER, "disp_time_weighting",                "Dispatch optimization future time discounting factor",                                                                                    "",             "",                                  "System Control",                           "?=0.99",                                                           "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "is_write_ampl_dat",                  "Write AMPL data files for dispatch run",                                                                                                  "",             "",                                  "System Control",                           "?=0",                                                              "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_STRING, "ampl_data_dir",                      "AMPL data file directory",                                                                                                                "",             "",                                  "System Control",                           "?=''",                                                             "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_NUMBER, "is_ampl_engine",                     "Run dispatch optimization with external AMPL engine",                                                                                     "",             "",                                  "System Control",                           "?=0",                                                              "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_STRING, "ampl_exec_call",                     "System command to run AMPL code",                                                                                                         "",             "",                                  "System Control",                           "?='ampl sdk_solution.run'",                                        "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_NUMBER, "disp_rsu_cost_rel",                  "Receiver startup cost",                                                                                                                   "$/MWt/start",  "",                                  "System Control",                           "",                                                                 "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "disp_csu_cost_rel",                  "Cycle startup cost",                                                                                                                      "$/MWe-cycle/start", "",                             "System Control",                           "",                                                                 "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "disp_pen_ramping",                   "Dispatch cycle production change penalty",                                                                                                "$/MWe-change", "",                                  "System Control",                           "",                                                                 "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "disp_inventory_incentive",           "Dispatch storage terminal inventory incentive multiplier",                                                                                "",             "",                                  "System Control",                           "?=0.0",                                                            "",              "SIMULATION_PARAMETER"},

    // Pricing schedules and multipliers
        // Ideally this would work with sim_type = 2, but UI inputs availability depends on financial mode
    { SSC_INPUT,     SSC_NUMBER, "ppa_multiplier_model",               "PPA multiplier model 0: dispatch factors dispatch_factorX, 1: hourly multipliers dispatch_factors_ts",                                    "0/1",          "0=diurnal,1=timestep",              "Time of Delivery Factors",                 "?=0", /*need a default so this var works in required_if*/                     "INTEGER,MIN=0", "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_NUMBER, "ppa_soln_mode",                      "PPA solution mode (0=Specify IRR target, 1=Specify PPA price)",                                                                           "",             "",                                  "Financial Solution Mode",                  "sim_type=1&ppa_multiplier_model=0&csp_financial_model<5&is_dispatch=1&sim_type=1",       "",   "SIMULATION_PARAMETER" },
    { SSC_INPUT,     SSC_ARRAY,  "dispatch_factors_ts",                "Dispatch payment factor array",                                                                                                           "",             "",                                  "Time of Delivery Factors",                 "ppa_multiplier_model=1&csp_financial_model<5&is_dispatch=1&sim_type=1",       "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_NUMBER, "en_electricity_rates",               "Enable electricity rates for grid purchase",                                                                                              "0/1",          "",                                  "Electricity Rates",                        "?=0",                                                                         "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_MATRIX, "dispatch_sched_weekday",             "PPA pricing weekday schedule, 12x24",                                                                                                     "",             "",                                  "Time of Delivery Factors",                 "ppa_multiplier_model=0&csp_financial_model<5&is_dispatch=1&sim_type=1",       "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_MATRIX, "dispatch_sched_weekend",             "PPA pricing weekend schedule, 12x24",                                                                                                     "",             "",                                  "Time of Delivery Factors",                 "ppa_multiplier_model=0&csp_financial_model<5&is_dispatch=1&sim_type=1",       "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_ARRAY,  "dispatch_tod_factors",               "TOD factors for periods 1 through 9",                                                                                                     "",
        "We added this array input after SAM 2022.12.21 to replace the functionality of former single value inputs dispatch_factor1 through dispatch_factor9",                                                                                                         "Time of Delivery Factors",                 "ppa_multiplier_model=0&csp_financial_model<5&is_dispatch=1&sim_type=1",       "",              "SIMULATION_PARAMETER" },

    { SSC_INPUT,     SSC_NUMBER, "is_dispatch_series",                 "Use time-series dispatch factors",                                                                                                        "",             "",                                  "System Control",                           "?=0",                                                                         "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_ARRAY,  "dispatch_series",                    "Time series dispatch factors",                                                                                                            "",             "",                                  "System Control",                           "",                                                                            "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_ARRAY,  "ppa_price_input",			           "PPA prices - yearly",			                                                                                                          "$/kWh",	      "",	                               "Revenue",			                       "ppa_multiplier_model=0&csp_financial_model<5&is_dispatch=1&sim_type=1",       "",      	       "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_MATRIX, "mp_energy_market_revenue",           "Energy market revenue input",                                                                                                             "",             "Lifetime x 2[Cleared Capacity(MW),Price($/MWh)]", "Revenue",                    "csp_financial_model=6&is_dispatch=1&sim_type=1",                              "",              "SIMULATION_PARAMETER"},

    // Costs
    { SSC_INPUT,     SSC_NUMBER, "tower_fixed_cost",                   "Tower fixed cost",                                                                                                                        "$",            "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "tower_exp",                          "Tower cost scaling exponent",                                                                                                             "",             "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "rec_ref_cost",                       "Receiver reference cost",                                                                                                                 "$",            "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "rec_ref_area",                       "Receiver reference area for cost scale",                                                                                                  "",             "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "rec_cost_exp",                       "Receiver cost scaling exponent",                                                                                                          "",             "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "rec_lift_spec_cost",                 "Receiver lift specific cost",                                                                                                             "$-s/m-kg",     "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "site_spec_cost",                     "Site improvement cost",                                                                                                                   "$/m2",         "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "heliostat_spec_cost",                "Heliostat field cost",                                                                                                                    "$/m2",         "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "plant_spec_cost",                    "Power cycle specific cost",                                                                                                               "$/kWe",        "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "bop_spec_cost",                      "BOS specific cost",                                                                                                                       "$/kWe",        "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "tes_cost_model",                     "TES cost model type (0 = Energy based, 1 = Detailed)",                                                                                    "",             "",                                  "System Costs",                             "?=1",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "tes_spec_cost",                      "Thermal energy storage cost",                                                                                                             "$/kWht",       "",                                  "System Costs",                             "tes_cost_model=0",                                                 "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "tes_cost_per_mass",                  "Thermal energy storage cost per mass",                                                                                                    "$/kg",         "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "bin_cost_per_mass",                  "Thermal energy storage bin cost per mass",                                                                                                "$/kg",         "",                                  "System Costs",                             "tes_cost_model=1",                                                 "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "bin_cost_per_mass_exp",              "Thermal energy storage bin cost per mass scaling exponent",                                                                               "-",            "",                                  "System Costs",                             "tes_cost_model=1",                                                 "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "tes_phx_lift_spec_cost",             "Lift specific cost (TES and PHX)",                                                                                                        "$-s/m-kg",     "",                                  "System Costs",                             "tes_cost_model=1",                                                 "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "land_spec_cost",                     "Total land area cost",                                                                                                                    "$/acre",       "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "contingency_rate",                   "Contingency for cost overrun",                                                                                                            "%",            "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "sales_tax_rate",                     "Sales tax rate",                                                                                                                          "%",            "",                                  "Financial Parameters",                     "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "sales_tax_frac",                     "Percent of cost to which sales tax applies",                                                                                              "%",            "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "cost_sf_fixed",                      "Solar field fixed cost",                                                                                                                  "$",            "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "fossil_spec_cost",                   "Fossil system specific cost",                                                                                                             "$/kWe",        "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "csp.pt.cost.epc.per_acre",           "EPC cost per acre",                                                                                                                       "$/acre",       "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "csp.pt.cost.epc.percent",            "EPC cost percent of direct",                                                                                                              "%",            "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "csp.pt.cost.epc.per_watt",           "EPC cost per watt",                                                                                                                       "$/W",          "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "csp.pt.cost.epc.fixed",              "EPC fixed",                                                                                                                               "$",            "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "csp.pt.cost.plm.percent",            "PLM cost percent of direct",                                                                                                              "%",            "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "csp.pt.cost.plm.per_watt",           "PLM cost per watt",                                                                                                                       "$/W",          "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "csp.pt.cost.plm.fixed",              "PLM fixed",                                                                                                                               "$",            "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "csp.pt.sf.fixed_land_area",          "Fixed land area",                                                                                                                         "acre",         "",                                  "Heliostat Field",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "csp.pt.sf.land_overhead_factor",     "Land overhead factor",                                                                                                                    "",             "",                                  "Heliostat Field",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "frac_rec_flow_lost",                 "Fraction of receiver flow lost",                                                                                                          "kg/kg",        "",                                  "System Costs",                             "*",                                                                "",              ""},

    // O&M - use "INOUT" so SAM shows in outputs
        // Array (years) input for Single Owner
    { SSC_INOUT,     SSC_ARRAY,  "om_fixed",                           "Fixed O&M annual amount",                                                                                                                 "$/year",       "",                                  "System Costs",                             "?=[0]",                                                            "",              "SIMULATION_PARAMETER" },
        // LCOE
    { SSC_INOUT,     SSC_NUMBER, "fixed_operating_cost",               "Annual fixed operating cost",                                                                                                              "$",           "",                                  "Simple LCOE",                              "sim_type=1&csp_financial_model=7",                                 "",              "SIMULATION_PARAMETER" },


        // Construction financing inputs/outputs (SSC variable table from cmod_cb_construction_financing)
    { SSC_INPUT,     SSC_NUMBER, "const_per_interest_rate1",           "Interest rate, loan 1",                                                                                                                   "%",            "",                                  "Financial Parameters",                     "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "const_per_interest_rate2",           "Interest rate, loan 2",                                                                                                                   "%",            "",                                  "Financial Parameters",                     "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "const_per_interest_rate3",           "Interest rate, loan 3",                                                                                                                   "%",            "",                                  "Financial Parameters",                     "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "const_per_interest_rate4",           "Interest rate, loan 4",                                                                                                                   "%",            "",                                  "Financial Parameters",                     "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "const_per_interest_rate5",           "Interest rate, loan 5",                                                                                                                   "%",            "",                                  "Financial Parameters",                     "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "const_per_months1",                  "Months prior to operation, loan 1",                                                                                                       "",             "",                                  "Financial Parameters",                     "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "const_per_months2",                  "Months prior to operation, loan 2",                                                                                                       "",             "",                                  "Financial Parameters",                     "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "const_per_months3",                  "Months prior to operation, loan 3",                                                                                                       "",             "",                                  "Financial Parameters",                     "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "const_per_months4",                  "Months prior to operation, loan 4",                                                                                                       "",             "",                                  "Financial Parameters",                     "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "const_per_months5",                  "Months prior to operation, loan 5",                                                                                                       "",             "",                                  "Financial Parameters",                     "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "const_per_percent1",                 "Percent of total installed cost, loan 1",                                                                                                 "%",            "",                                  "Financial Parameters",                     "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "const_per_percent2",                 "Percent of total installed cost, loan 2",                                                                                                 "%",            "",                                  "Financial Parameters",                     "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "const_per_percent3",                 "Percent of total installed cost, loan 3",                                                                                                 "%",            "",                                  "Financial Parameters",                     "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "const_per_percent4",                 "Percent of total installed cost, loan 4",                                                                                                 "%",            "",                                  "Financial Parameters",                     "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "const_per_percent5",                 "Percent of total installed cost, loan 5",                                                                                                 "%",            "",                                  "Financial Parameters",                     "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "const_per_upfront_rate1",            "Upfront fee on principal, loan 1",                                                                                                        "%",            "",                                  "Financial Parameters",                     "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "const_per_upfront_rate2",            "Upfront fee on principal, loan 2",                                                                                                        "%",            "",                                  "Financial Parameters",                     "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "const_per_upfront_rate3",            "Upfront fee on principal, loan 3",                                                                                                        "%",            "",                                  "Financial Parameters",                     "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "const_per_upfront_rate4",            "Upfront fee on principal, loan 4",                                                                                                        "%",            "",                                  "Financial Parameters",                     "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "const_per_upfront_rate5",            "Upfront fee on principal, loan 5",                                                                                                        "%",            "",                                  "Financial Parameters",                     "*",                                                                "",              ""},
    
    // ****************************************************************************************************************************************
    //     DEPRECATED INPUTS -- exec() checks if a) variable is assigned and b) if replacement variable is assigned. throws exception if a=true and b=false
    // ****************************************************************************************************************************************

    // ****************************************************************************************************************************************
    // Design Outputs here:
    // ****************************************************************************************************************************************

        // land area with variable name required by downstream financial model
    { SSC_OUTPUT,    SSC_NUMBER, "total_land_area",                    "Total land area",                                                                                                                         "acre",         "",                                  "System Costs",                             "*",                                                                "",              ""},
        // System capacity required by downstream financial model
    { SSC_OUTPUT,    SSC_NUMBER, "system_capacity",                    "System capacity",                                                                                                                         "kWe",          "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "cp_system_nameplate",                "System capacity for capacity payments",                                                                                                   "MWe",          "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "cp_battery_nameplate",               "Battery nameplate",                                                                                                                       "MWe",          "",                                  "System Costs",                             "*",                                                                "",              ""},

        // Solar Field
    { SSC_OUTPUT,    SSC_NUMBER, "N_hel_tot_calc",                     "Total number of heliostats - out",                                                                                                               "",             "",                                  "Heliostat Field",                          "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "N_hel_calc",                         "Number of heliostats per receiver - out",                                                                                                               "",             "",                                  "Heliostat Field",                          "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "refl_image_error",                   "Reflected image error",                                                                                                                    "mrad",         "",                                  "Heliostat Field",                          "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "heliostat_area",                     "Active area of heliostat",                                                                                                                 "m^2",          "",                                  "Heliostat Field",                          "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "average_attenuation",                "Average solar field attenuation",                                                                                                          "%",            "",                                  "Heliostat Field",                          "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_MATRIX, "helio_positions_calc",               "Heliostat position table - out",                                                                                                           "",             "",                                  "Heliostat Field",                          "*",                                                                "",              "COL_LABEL=XY_POSITION" },
    { SSC_OUTPUT,    SSC_NUMBER, "A_sf_tot",                           "Total solar field area",                                                                                                                   "m^2",          "",                                  "Heliostat Field",                          "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "A_sf",                               "Solar field area per receiver",                                                                                                            "m^2",          "",                                  "Heliostat Field",                          "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "land_min_abs",                       "Min distance from tower to heliostat",                                                                                                     "m",            "",                                  "Heliostat Field",                          "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "land_max_abs",                       "Max distance from tower to heliostat",                                                                                                     "m",            "",                                  "Heliostat Field",                          "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "land_area_base_calc",                "Land area occupied by heliostats",                                                                                                         "acre",         "",                                  "Heliostat Field",                          "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "total_land_area_calc",               "Total land area - out",                                                                                                                    "acre",         "",                                  "Heliostat Field",                          "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "W_dot_col_tracking_des",             "Collector tracking power at design",                                                                                                       "MWe",          "",                                  "Heliostat Field",                          "*",                                                                "",              ""},

        // Receiver Geometry
    { SSC_OUTPUT,    SSC_ARRAY,  "rec_height_calc",                    "Receiver height - out",                                                                                                                    "m",            "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "rec_width_calc",                     "Receiver width - out",                                                                                                                     "m",            "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "rec_azimuth_calc",                   "Receiver azimuth - out",                                                                                                                   "m",            "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "h_tower_calc",                       "Tower height - out",                                                                                                                       "m",            "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},

    { SSC_OUTPUT,    SSC_NUMBER, "A_rec_total",                        "Total receiver aperture area",                                                                                                             "m2",           "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "A_rec",                              "Receiver(s) aperture area",                                                                                                                "m2",           "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "A_rec_curtain_total",                "Total receiver particle curtain area",                                                                                                           "m2",           "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "A_rec_curtain",                      "Receiver(s) particle curtain area",                                                                                                           "m2",           "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "L_tower_piping_calc",                "Tower piping length",                                                                                                                      "m",            "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},

        // Receiver Performance
    { SSC_OUTPUT,    SSC_NUMBER, "q_dot_rec_des_total",                "Total receiver thermal output at design",                                                                                                       "MWt",         "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "eta_rec_thermal_des",                "Receiver estimated thermal efficiency at design",                                                                                         "",            "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "P_tower_lift_des",                   "Receiver and tower estimated particle lift power at design",                                                                              "MWe",         "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "Q_transport_loss_des",               "Receiver estimated particle transport losses at design",                                                                                  "MWt",         "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "m_dot_htf_rec_des",                  "Receiver HTF mass flow rate at design",                                                                                                   "kg/s",        "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "m_dot_htf_rec_max",                  "Receiver max HTF mass flow rate",                                                                                                         "kg/s",        "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },

    { SSC_OUTPUT,    SSC_ARRAY,  "rec_design_hl",                      "Receiver(s) estimated thermal loss per aperture area used in SolarPILOT of field sizing",                                                 "kWt/m2",      "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "q_dot_des_per_rec",                  "Receiver(s) thermal output at design",                                                                                                    "MWt",         "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "eta_rec_thermal_des_per_rec",        "Receiver(s) estimated thermal efficiency at design",                                                                                      "",            "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "W_dot_rec_lift_des_per_rec",         "Receiver(s) and tower estimated particle lift power at design",                                                                           "MWe",         "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "q_dot_piping_loss_des_per_rec",      "Receiver(s) estimated particle transport losses at design",                                                                               "MWt",         "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "m_dot_htf_rec_des_per_rec",          "Receiver(s) HTF mass flow rate at design",                                                                                                "kg/s",        "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "m_dot_htf_rec_max_per_rec",          "Receiver(s) max HTF mass flow rate",                                                                                                      "kg/s",        "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "q_dot_inc_per_rec",                  "Receiver(s) incident power per receiver",                                                                                                 "kg/s",        "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },

        // Heater
    { SSC_OUTPUT,    SSC_NUMBER, "q_dot_heater_des",                   "Heater design thermal power",                                                                                                             "MWt",         "",                                  "Heater",                                   "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "W_dot_heater_des",                   "Heater electricity consumption at design",                                                                                                "MWe",         "",                                  "Heater",                                   "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "E_heater_su_des",                    "Heater startup energy",                                                                                                                   "MWt-hr",      "",                                  "Heater",                                   "*",                                                                "",              ""},

        // Power Cycle
    { SSC_OUTPUT,    SSC_NUMBER, "m_dot_htf_cycle_des",                "PC HTF mass flow rate at design",                                                                                                         "kg/s",        "",                                  "Power Cycle",                              "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "q_dot_cycle_des",                    "PC thermal input at design",                                                                                                              "MWt",         "",                                  "Power Cycle",                              "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "W_dot_cycle_lift_des",               "PC HTF lift power at design",                                                                                                             "MWe",         "",                                  "Power Cycle",                              "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "W_dot_cycle_cooling_des",            "PC cooling power at design",                                                                                                              "MWe",         "",                                  "Power Cycle",                              "*",                                                                "",              ""},
        // UDPC
    { SSC_OUTPUT,    SSC_NUMBER, "n_T_htf_pars_calc",                  "UDPC number of HTF parametric values",                                                                                                     "",            "",                                  "UDPC Design Calc",                        "*",                                                                "",              ""},      
    { SSC_OUTPUT,    SSC_NUMBER, "n_T_amb_pars_calc",                  "UDPC number of ambient temp parametric values",                                                                                            "",            "",                                  "UDPC Design Calc",                        "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "n_m_dot_pars_calc",                  "UDPC number of mass flow parametric values",                                                                                               "",            "",                                  "UDPC Design Calc",                        "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "T_htf_ref_calc",                     "UDPC reference HTF temperature",                                                                                                           "C",           "",                                  "UDPC Design Calc",                        "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "T_htf_low_calc",                     "UDPC low level HTF temperature",                                                                                                           "C",           "",                                  "UDPC Design Calc",                        "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "T_htf_high_calc",                    "UDPC high level HTF temperature",                                                                                                          "C",           "",                                  "UDPC Design Calc",                        "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "T_amb_ref_calc",                     "UDPC reference ambient temperature",                                                                                                       "C",           "",                                  "UDPC Design Calc",                        "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "T_amb_low_calc",                     "UDPC low level ambient temperature",                                                                                                       "C",           "",                                  "UDPC Design Calc",                        "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "T_amb_high_calc",                    "UDPC high level ambient temperature",                                                                                                      "C",           "",                                  "UDPC Design Calc",                        "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "m_dot_htf_ND_ref_calc",              "UDPC reference normalized mass flow rate",                                                                                                 "",            "",                                  "UDPC Design Calc",                        "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "m_dot_htf_ND_low_calc",              "UDPC low level normalized mass flow rate",                                                                                                 "",            "",                                  "UDPC Design Calc",                        "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "m_dot_htf_ND_high_calc",             "UDPC high level normalized mass flow rate",                                                                                                "",            "",                                  "UDPC Design Calc",                        "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "W_dot_gross_ND_des_calc",            "UDPC calculated normalized gross power at design",                                                                                         "",            "",                                  "UDPC Design Calc",                        "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "Q_dot_HTF_ND_des_calc",              "UDPC calculated normalized heat input at design",                                                                                          "",            "",                                  "UDPC Design Calc",                        "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "W_dot_cooling_ND_des_calc",          "UPPC calculated normalized cooling power at design",                                                                                       "",            "",                                  "UDPC Design Calc",                        "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "m_dot_water_ND_des_calc",            "UDPC calculated water use at design",                                                                                                      "",            "",                                  "UDPC Design Calc",                        "*",                                                                "",              ""},

        // TES
    { SSC_OUTPUT,    SSC_NUMBER, "Q_tes_des",                          "TES design capacity",                                                                                                                     "MWt-hr",       "",                                 "TES Design Calc",                          "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "V_tes_htf_avail_des",                "TES volume of HTF available for heat transfer",                                                                                           "m3",           "",                                 "TES Design Calc",                          "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "V_tes_htf_total_des",                "TES total HTF volume",                                                                                                                    "m3",           "",                                 "TES Design Calc",                          "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "d_tank_tes",                         "TES bin diameter",                                                                                                                       "m",            "",                                 "TES Design Calc",                          "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "q_dot_loss_tes_des",                 "TES thermal loss at design",                                                                                                              "MWt",          "",                                 "TES Design Calc",                          "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "tshours_rec",                        "TES duration at receiver design output",                                                                                                  "hr",           "",                                 "TES Design Calc",                          "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "tshours_heater",                     "TES duration at heater design output",                                                                                                    "hr",           "",                                 "TES Design Calc",                          "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "dens_store_htf_at_T_ave",            "TES density of HTF at avg temps",                                                                                                         "kg/m3",        "",                                 "TES Design Calc",                          "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "m_tes_total_des",                    "TES medium total mass",                                                                                                                   "kg",           "",                                 "TES Design Calc",                          "*",                                                                "",              ""},

        // Balance of Plant
    { SSC_OUTPUT,    SSC_NUMBER, "nameplate",                          "Nameplate capacity",                                                                                                                      "MWe",          "",                                 "System Design Calc",                       "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "W_dot_bop_design",                   "BOP parasitics at design",                                                                                                                "MWe",          "",                                 "Balance of Plant",                         "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "W_dot_fixed",                        "Fixed parasitic at design",                                                                                                               "MWe",          "",                                 "Balance of Plant",                         "*",                                                                "",              ""},

        // Costs
    { SSC_OUTPUT,    SSC_NUMBER, "h_rec_input_to_cost_model",          "Receiver height for cost model selected from receiver type",                                                                              "m",            "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "tower_total_height",                 "Total tower height used in cost model calculations",                                                                                      "m",            "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "csp.pt.cost.site_improvements",      "Site improvement cost",                                                                                                                   "$",            "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "csp.pt.cost.heliostats",             "Heliostat cost",                                                                                                                          "$",            "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "csp.pt.cost.tower",                  "Tower cost",                                                                                                                              "$",            "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "csp.pt.cost.receiver",               "Receiver cost",                                                                                                                           "$",            "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "receiver_lift_cost",                 "Receiver lift cost",                                                                                                                      "$",            "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "csp.pt.cost.storage",                "TES cost",                                                                                                                                "$",            "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "tes_medium_cost",                    "TES medium cost",                                                                                                                         "$",            "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "tes_bin_cost",                       "TES bin cost",                                                                                                                            "$",            "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "tes_lift_cost",                      "TES lift cost",                                                                                                                           "$",            "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "phx_lift_cost",                      "PHX lift cost",                                                                                                                           "$",            "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "csp.pt.cost.power_block",            "Power cycle cost",                                                                                                                        "$",            "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "heater_cost",                        "Heater cost",                                                                                                                             "$",            "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "csp.pt.cost.bop",                    "BOP cost",                                                                                                                                "$",            "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "csp.pt.cost.fossil",                 "Fossil backup cost",                                                                                                                      "$",            "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "ui_direct_subtotal",                 "Direct capital pre-contingency cost",                                                                                                     "$",            "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "csp.pt.cost.contingency",            "Contingency cost",                                                                                                                        "$",            "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "total_direct_cost",                  "Total direct cost",                                                                                                                       "$",            "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "csp.pt.cost.epc.total",              "EPC and owner cost",                                                                                                                      "$",            "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "csp.pt.cost.plm.total",              "Total land cost",                                                                                                                         "$",            "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "csp.pt.cost.sales_tax.total",        "Sales tax cost",                                                                                                                          "$",            "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "total_indirect_cost",                "Total indirect cost",                                                                                                                     "$",            "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "total_installed_cost",               "Total installed cost",                                                                                                                    "$",            "",                                  "System Costs",                             "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "csp.pt.cost.installed_per_capacity", "Estimated installed cost per cap",                                                                                                        "$",            "",                                  "System Costs",                             "*",                                                                "",              ""},

        // Financing
    { SSC_OUTPUT,    SSC_NUMBER, "const_per_principal1",               "Principal, loan 1",                                                                                                                       "$",            "",                                  "Financial Parameters",                     "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "const_per_principal2",               "Principal, loan 2",                                                                                                                       "$",            "",                                  "Financial Parameters",                     "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "const_per_principal3",               "Principal, loan 3",                                                                                                                       "$",            "",                                  "Financial Parameters",                     "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "const_per_principal4",               "Principal, loan 4",                                                                                                                       "$",            "",                                  "Financial Parameters",                     "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "const_per_principal5",               "Principal, loan 5",                                                                                                                       "$",            "",                                  "Financial Parameters",                     "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "const_per_interest1",                "Interest cost, loan 1",                                                                                                                   "$",            "",                                  "Financial Parameters",                     "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "const_per_interest2",                "Interest cost, loan 2",                                                                                                                   "$",            "",                                  "Financial Parameters",                     "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "const_per_interest3",                "Interest cost, loan 3",                                                                                                                   "$",            "",                                  "Financial Parameters",                     "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "const_per_interest4",                "Interest cost, loan 4",                                                                                                                   "$",            "",                                  "Financial Parameters",                     "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "const_per_interest5",                "Interest cost, loan 5",                                                                                                                   "$",            "",                                  "Financial Parameters",                     "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "const_per_total1",                   "Total financing cost, loan 1",                                                                                                            "$",            "",                                  "Financial Parameters",                     "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "const_per_total2",                   "Total financing cost, loan 2",                                                                                                            "$",            "",                                  "Financial Parameters",                     "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "const_per_total3",                   "Total financing cost, loan 3",                                                                                                            "$",            "",                                  "Financial Parameters",                     "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "const_per_total4",                   "Total financing cost, loan 4",                                                                                                            "$",            "",                                  "Financial Parameters",                     "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "const_per_total5",                   "Total financing cost, loan 5",                                                                                                            "$",            "",                                  "Financial Parameters",                     "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "const_per_percent_total",            "Total percent of installed costs, all loans",                                                                                             "%",            "",                                  "Financial Parameters",                     "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "const_per_principal_total",          "Total principal, all loans",                                                                                                              "$",            "",                                  "Financial Parameters",                     "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "const_per_interest_total",           "Total interest costs, all loans",                                                                                                         "$",            "",                                  "Financial Parameters",                     "*",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "construction_financing_cost",        "Total construction financing cost",                                                                                                       "$",            "",                                  "Financial Parameters",                     "*",                                                                "",              ""},

    // ****************************************************************************************************************************************
    // Timeseries Simulation Outputs here:
    // ****************************************************************************************************************************************
        // Simulation outputs
    { SSC_OUTPUT,    SSC_ARRAY,  "time_hr",                            "Time at end of timestep",                                                                                                                 "hr",           "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "solzen",                             "Resource solar zenith",                                                                                                                   "deg",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "solaz",                              "Resource solar azimuth",                                                                                                                  "deg",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "beam",                               "Resource beam normal irradiance",                                                                                                         "W/m2",         "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "tdry",                               "Resource dry Bulb temperature",                                                                                                           "C",            "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "twet",                               "Resource wet Bulb temperature",                                                                                                           "C",            "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "rh",                                 "Resource relative humidity",                                                                                                              "%",            "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "wspd",                               "Resource wind velocity",                                                                                                                  "m/s",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "wdir",                               "Resource wind direction",                                                                                                                 "deg",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},

        // Collector-receiver outputs
    { SSC_OUTPUT,    SSC_MATRIX, "eta_map_out",                        "Solar field optical efficiencies",                                                                                                        "",             "",                                  "",                                         "sim_type=1",                                                       "",              "COL_LABEL=OPTICAL_EFFICIENCY,ROW_LABEL=NO_ROW_LABEL"},
    { SSC_OUTPUT,    SSC_MATRIX, "flux_maps_for_import",               "Flux map for import",                                                                                                                     "",             "",                                  "",                                         "sim_type=1",                                                       "",              "COL_LABEL=FLUX_MAPS_MULT_REC,ROW_LABEL=NO_ROW_LABEL" },
    { SSC_OUTPUT,    SSC_MATRIX, "flux_maps_out",                      "Flux map intensities",                                                                                                                    "",             "",                                  "",                                         "sim_type=1",                                                       "",              "COL_LABEL=FLUX_MAPS_MULT_REC,ROW_LABEL=NO_ROW_LABEL"},

    { SSC_OUTPUT,    SSC_ARRAY,  "q_sf_inc",                           "Field incident thermal power",                                                                                                            "MWt",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "eta_field",                          "Field optical efficiency",                                                                                                                "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "defocus",                            "Field optical focus fraction",                                                                                                            "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "sf_adjust_out",                      "Field availability adjustment factor",                                                                                                    "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "rec_defocus",                        "Receiver component defocus",                                                                                                              "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "q_dot_rec_inc",                      "Receiver incident thermal power on particle curtain",                                                                                     "MWt",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "eta_therm",                          "Receiver efficiency",                                                                                                                     "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "Q_thermal",                          "Receiver thermal power to HTF less particle transport loss",                                                                              "MWt",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "pparasi",                            "Field tracking power",                                                                                                                    "MWe",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},

    { SSC_OUTPUT,    SSC_ARRAY,  "m_dot_rec",                          "Receiver mass flow rate",                                                                                                                 "kg/s",         "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "q_startup",                          "Receiver startup thermal power consumed",                                                                                                 "MWt",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "T_rec_in",                           "Receiver HTF inlet temperature",                                                                                                          "C",            "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "T_rec_out",                          "Receiver HTF outlet temperature",                                                                                                         "C",            "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "T_cav_wall_max",                     "Maximum back cavity wall temperature",                                                                                                    "C",            "",                                  "",                                         "sim_type=1",                                                       "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "T_cav_wall_avg",                     "Average back cavity wall temperature",                                                                                                    "C",            "",                                  "",                                         "sim_type=1",                                                       "",              "" },

    { SSC_OUTPUT,    SSC_ARRAY,  "q_transport_loss",                   "Receiver particle transport losses",                                                                                                      "MWt",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "q_thermal_loss",                     "Receiver convection and emission losses",                                                                                                 "MWt",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "q_reflection_loss",                  "Receiver reflection losses",                                                                                                              "MWt",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "P_tower_lift",                       "Receiver and tower particle lift power",                                                                                                  "MWe",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
                                                                                                                                                                                                                                                                                                         
	{ SSC_OUTPUT,    SSC_ARRAY,  "clearsky",						   "Predicted clear-sky beam normal irradiance",																							  "W/m2",         "",                                  "CR",                                       "sim_type=1&rec_clearsky_fraction>0",                               "",              ""},

        // Collector-receiver outputs from individual receivers (only reported if > 1 receiver)
    { SSC_OUTPUT,    SSC_ARRAY,  "q_dot_rec_inc_1",                    "Receiver 1 incident thermal power on particle curtain",                                                                                   "MWt",          "",                                  "",                                         "sim_type=1&num_recs>1",                                            "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "q_dot_rec_inc_2",                    "Receiver 2 incident thermal power on particle curtain",                                                                                   "MWt",          "",                                  "",                                         "sim_type=1&num_recs>1",                                            "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "q_dot_rec_inc_3",                    "Receiver 3 incident thermal power on particle curtain",                                                                                   "MWt",          "",                                  "",                                         "sim_type=1&num_recs>2",                                            "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "q_dot_rec_inc_4",                    "Receiver 4 incident thermal power on particle curtain",                                                                                   "MWt",          "",                                  "",                                         "sim_type=1&num_recs>3",                                            "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "Q_thermal_1",                        "Receiver 1 thermal power to HTF less particle transport loss",                                                                            "MWt",          "",                                  "",                                         "sim_type=1&num_recs>1",                                            "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "Q_thermal_2",                        "Receiver 2 thermal power to HTF less particle transport loss",                                                                            "MWt",          "",                                  "",                                         "sim_type=1&num_recs>1",                                            "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "Q_thermal_3",                        "Receiver 3 thermal power to HTF less particle transport loss",                                                                            "MWt",          "",                                  "",                                         "sim_type=1&num_recs>2",                                            "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "Q_thermal_4",                        "Receiver 4 thermal power to HTF less particle transport loss",                                                                            "MWt",          "",                                  "",                                         "sim_type=1&num_recs>3",                                            "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "q_startup_1",                        "Receiver 1 startup thermal power consumed",                                                                                               "MWt",          "",                                  "",                                         "sim_type=1&num_recs>1",                                            "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "q_startup_2",                        "Receiver 2 startup thermal power consumed",                                                                                               "MWt",          "",                                  "",                                         "sim_type=1&num_recs>1",                                            "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "q_startup_3",                        "Receiver 3 startup thermal power consumed",                                                                                               "MWt",          "",                                  "",                                         "sim_type=1&num_recs>2",                                            "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "q_startup_4",                        "Receiver 4 startup thermal power consumed",                                                                                               "MWt",          "",                                  "",                                         "sim_type=1&num_recs>3",                                            "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "eta_therm_1",                        "Receiver 1 efficiency",                                                                                                                   "",             "",                                  "",                                         "sim_type=1&num_recs>1",                                            "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "eta_therm_2",                        "Receiver 2 efficiency",                                                                                                                   "",             "",                                  "",                                         "sim_type=1&num_recs>1",                                            "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "eta_therm_3",                        "Receiver 3 efficiency",                                                                                                                   "",             "",                                  "",                                         "sim_type=1&num_recs>2",                                            "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "eta_therm_4",                        "Receiver 4 efficiency",                                                                                                                   "",             "",                                  "",                                         "sim_type=1&num_recs>3",                                            "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "eta_field_1",                        "Field 1 optical efficiency",                                                                                                              "",             "",                                  "",                                         "sim_type=1&num_recs>1",                                            "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "eta_field_2",                        "Field 2 optical efficiency",                                                                                                              "",             "",                                  "",                                         "sim_type=1&num_recs>1",                                            "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "eta_field_3",                        "Field 3 optical efficiency",                                                                                                              "",             "",                                  "",                                         "sim_type=1&num_recs>2",                                            "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "eta_field_4",                        "Field 4 optical efficiency",                                                                                                              "",             "",                                  "",                                         "sim_type=1&num_recs>3",                                            "",              "" },

        // Heater outputs is_parallel_htr
    { SSC_OUTPUT,    SSC_ARRAY,  "W_dot_heater",                       "Parallel heater electricity consumption",                                                                                                 "MWe",          "",                                  "Parallel Heater",                          "sim_type=1&is_parallel_htr=1",                                     "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "q_dot_heater_to_htf",                "Parallel heater thermal power to HTF",                                                                                                    "MWt",          "",                                  "Parallel Heater",                          "sim_type=1&is_parallel_htr=1",                                     "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "q_dot_heater_startup",               "Parallel heater thermal power consumed during startup",                                                                                   "MWt",          "",                                  "Parallel Heater",                          "sim_type=1&is_parallel_htr=1",                                     "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "m_dot_htf_heater",                   "Parallel heater HTF mass flow rate",                                                                                                      "kg/s",         "",                                  "Parallel Heater",                          "sim_type=1&is_parallel_htr=1",                                     "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "T_htf_heater_in",                    "Parallel heater HTF inlet temperature",                                                                                                   "C",            "",                                  "Parallel Heater",                          "sim_type=1&is_parallel_htr=1",                                     "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "T_htf_heater_out",                   "Parallel heater HTF outlet temperature",                                                                                                  "C",            "",                                  "Parallel Heater",                          "sim_type=1&is_parallel_htr=1",                                     "",              ""},

        // Power cycle outputs
    { SSC_OUTPUT,    SSC_ARRAY,  "eta",                                "PC efficiency, gross",                                                                                                                    "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "q_pb",                               "PC input energy",                                                                                                                         "MWt",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "m_dot_pc",                           "PC HTF mass flow rate",                                                                                                                   "kg/s",         "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "q_pc_startup",                       "PC startup thermal energy",                                                                                                               "MWht",         "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "q_dot_pc_startup",                   "PC startup thermal power",                                                                                                                "MWt",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "P_cycle",                            "PC electrical power output, gross",                                                                                                       "MWe",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "T_pc_in",                            "PC HTF inlet temperature",                                                                                                                "C",            "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "T_pc_out",                           "PC HTF outlet temperature",                                                                                                               "C",            "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "m_dot_water_pc",                     "PC water consumption, makeup + cooling",                                                                                                  "kg/s",         "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "T_cond_out",                         "PC condenser water outlet temperature",                                                                                                   "C",            "",                                  "PC",                                       "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "P_cond",                             "PC condensing pressure",                                                                                                                  "Pa",           "",                                  "PC",                                       "?",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "P_cond_iter_err",                    "PC condenser pressure iteration error",                                                                                                   "",             "",                                  "PC",                                       "?",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "cycle_htf_lift_power",               "Cycle HTF particle lift power",                                                                                                                    "MWe",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "P_cooling_tower_tot",                "Parasitic power condenser operation",                                                                                                     "MWe",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},


        // Thermal energy storage outputs
    { SSC_OUTPUT,    SSC_ARRAY,  "tank_losses",                        "TES thermal losses",                                                                                                                      "MWt",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "q_heater",                           "TES bin heater power",                                                                                                             "MWe",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "T_tes_hot",                          "TES hot temperature",                                                                                                                     "C",            "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "T_tes_cold",                         "TES cold temperature",                                                                                                                    "C",            "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "mass_tes_cold",                      "TES cold mass (end)",                                                                                                                "kg",           "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "mass_tes_hot",                       "TES hot mass (end)",                                                                                                                 "kg",           "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "q_dc_tes",                           "TES discharge thermal power",                                                                                                             "MWt",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "q_ch_tes",                           "TES charge thermal power",                                                                                                                "MWt",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "e_ch_tes",                           "TES charge state",                                                                                                                        "MWht",         "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "m_dot_cr_to_tes_hot",                "Mass flow: field to hot TES",                                                                                                             "kg/s",         "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "m_dot_tes_hot_out",                  "Mass flow: TES hot out",                                                                                                                  "kg/s",         "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "m_dot_pc_to_tes_cold",               "Mass flow: cycle to cold TES",                                                                                                            "kg/s",         "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "m_dot_tes_cold_out",                 "Mass flow: TES cold out",                                                                                                                 "kg/s",         "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "m_dot_field_to_cycle",               "Mass flow: field to cycle",                                                                                                               "kg/s",         "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "m_dot_cycle_to_field",               "Mass flow: cycle to field",                                                                                                               "kg/s",         "",                                  "",                                         "sim_type=1",                                                       "",              ""},


        // Parasitics outputs
    { SSC_OUTPUT,    SSC_ARRAY,  "P_fixed",                            "Parasitic power fixed load",                                                                                                              "MWe",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "P_plant_balance_tot",                "Parasitic power generation-dependent load",                                                                                               "MWe",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},

        // System outputs
    { SSC_OUTPUT,    SSC_ARRAY,  "P_out_net",                          "Total electric power to grid",                                                                                                            "MWe",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},

        // Controller outputs
    { SSC_OUTPUT,    SSC_ARRAY,  "tou_value",                          "CSP operating time-of-use value",                                                                                                         "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "pricing_mult",                       "PPA price multiplier",                                                                                                                    "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "n_op_modes",                         "Operating modes in reporting timestep",                                                                                                   "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "op_mode_1",                          "1st operating mode",                                                                                                                      "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "op_mode_2",                          "2nd operating mode, if applicable",                                                                                                       "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "op_mode_3",                          "3rd operating mode, if applicable",                                                                                                       "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "m_dot_balance",                      "Relative mass flow balance error",                                                                                                        "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "q_balance",                          "Relative energy balance error",                                                                                                           "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},

        // Dispatch outputs
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_rel_mip_gap",                   "Dispatch relative MIP gap",                                                                                                               "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_solve_state",                   "Dispatch solver state",                                                                                                                   "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_subopt_flag",                   "Dispatch suboptimal solution flag",                                                                                                       "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_solve_iter",                    "Dispatch iterations count",                                                                                                               "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_objective",                     "Dispatch objective function value",                                                                                                       "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_obj_relax",                     "Dispatch objective function - relaxed max",                                                                                               "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_qsf_expected",                  "Dispatch expected solar field available energy",                                                                                          "MWt",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_qsfprod_expected",              "Dispatch expected solar field generation",                                                                                                "MWt",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_qsfsu_expected",                "Dispatch expected solar field startup enegy",                                                                                             "MWt",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_tes_expected",                  "Dispatch expected TES charge level",                                                                                                      "MWht",         "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_pceff_expected",                "Dispatch expected power cycle efficiency adj.",                                                                                           "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_thermeff_expected",             "Dispatch expected SF thermal efficiency adj.",                                                                                            "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_qpbsu_expected",                "Dispatch expected power cycle startup energy",                                                                                            "MWht",         "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_wpb_expected",                  "Dispatch expected power generation",                                                                                                      "MWe",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_rev_expected",                  "Dispatch expected revenue factor",                                                                                                        "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_presolve_nconstr",              "Dispatch number of constraints in problem",                                                                                               "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_presolve_nvar",                 "Dispatch number of variables in problem",                                                                                                 "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_solve_time",                    "Dispatch solver time",                                                                                                                    "sec",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},


        // These outputs correspond to the first csp-solver timestep in the reporting timestep.
        //     Subsequent csp-solver timesteps within the same reporting timestep are not tracked
    { SSC_OUTPUT,    SSC_ARRAY,  "q_dot_pc_sb",                        "Thermal power for PC standby",                                                                                                            "MWt",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "q_dot_pc_min",                       "Thermal power for PC min operation",                                                                                                      "MWt",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "q_dot_pc_max",                       "Max thermal power to PC",                                                                                                                 "MWt",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "q_dot_pc_target",                    "Target thermal power to PC",                                                                                                              "MWt",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "is_rec_su_allowed",                  "Is receiver startup allowed",                                                                                                             "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "is_pc_su_allowed",                   "Is power cycle startup allowed",                                                                                                          "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "is_pc_sb_allowed",                   "Is power cycle standby allowed",                                                                                                          "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},

    { SSC_OUTPUT,    SSC_ARRAY,  "is_PAR_HTR_allowed",                 "Is parallel electric heater operation allowed",                                                                                           "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "q_dot_elec_to_PAR_HTR",              "Electric heater thermal power target",                                                                                                    "MWt",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},

    { SSC_OUTPUT,    SSC_ARRAY,  "q_dot_est_cr_su",                    "Estimated receiver startup thermal power",                                                                                                "MWt",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "q_dot_est_cr_on",                    "Estimated receiver thermal power TO HTF",                                                                                                 "MWt",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "q_dot_est_tes_dc",                   "Estimated max TES discharge thermal power",                                                                                               "MWt",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "q_dot_est_tes_ch",                   "Estimated max TES charge thermal power",                                                                                                  "MWt",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "operating_modes_a",                  "First 3 operating modes tried",                                                                                                           "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "operating_modes_b",                  "Next 3 operating modes tried",                                                                                                            "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "operating_modes_c",                  "Final 3 operating modes tried",                                                                                                           "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},

    { SSC_OUTPUT,    SSC_ARRAY,  "gen",                                "Total electric power to grid with available derate",                                                                                      "kWe",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},

    // Annual single-value outputs
    { SSC_OUTPUT,    SSC_NUMBER, "cost_particle_loss_year1",           "Cost of replacement particles in year 1",                                                                                                 "$",            "",                                  "",                                         "sim_type=1",                                                       "",              ""},

    { SSC_OUTPUT,    SSC_NUMBER, "annual_energy",                      "Annual total electric power to grid",                                                                                                     "kWhe",         "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "annual_W_cycle_gross",               "Electrical source - power cycle gross output",                                                                                            "kWhe",         "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "annual_W_cooling_tower",             "Total of condenser operation parasitics",                                                                                                 "kWhe",         "",                                  "PC",                                       "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "annual_q_rec_inc",                   "Annual receiver incident thermal power",                                                                                                  "MWt-hr",       "",                                  "Tower and Receiver",                       "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "annual_q_rec_reflection_loss",       "Annual receiver reflection losses",                                                                                                       "MWt-hr",       "",                                  "Tower and Receiver",                       "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "annual_q_rec_thermal_loss",          "Annual receiver advective and radiative losses",                                                                                          "MWt-hr",       "",                                  "Tower and Receiver",                       "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "annual_rec_mass_throughput",         "Annual receiver mass throughput",                                                                                                         "kg",           "",                                  "Tower and Receiver",                       "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "particles_lost_per_year",            "Particles lost per year",                                                                                                                 "kg",           "",                                  "Tower and Receiver",                       "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "annual_eta_rec",                     "Annual receiver efficiency including reflective, advective, radiative loss",                                                              "",             "",                                  "Tower and Receiver",                       "sim_type=1",                                                       "",              ""},

    { SSC_OUTPUT,    SSC_NUMBER, "conversion_factor",                  "Gross to net conversion factor",                                                                                                          "%",            "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "capacity_factor",                    "Capacity factor",                                                                                                                         "%",            "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "sales_energy_capacity_factor",       "Capacity factor considering only positive net generation periods",                                                                        "%",            "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "kwh_per_kw",                         "First year kWh/kW",                                                                                                                       "kWh/kW",       "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "annual_total_water_use",             "Total annual water usage, cycle + mirror washing",                                                                                        "m3",           "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "capacity_factor_highest_1000_ppas",  "Capacity factor at 1000 highest ppa timesteps",                                                                                           "-",            "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "capacity_factor_highest_2000_ppas",  "Capacity factor at 2000 highest ppa timesteps",                                                                                           "-",            "",                                  "",                                         "sim_type=1",                                                       "",              ""},
                                                                                                                                                                                                                                                                                                                    
    { SSC_OUTPUT,    SSC_NUMBER, "disp_objective_ann",                 "Annual sum of dispatch objective function value",                                                                                         "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "disp_iter_ann",                      "Annual sum of dispatch solver iterations",                                                                                                "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "disp_presolve_nconstr_ann",          "Annual sum of dispatch problem constraint count",                                                                                         "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "disp_presolve_nvar_ann",             "Annual sum of dispatch problem variable count",                                                                                           "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "disp_solve_time_ann",                "Annual sum of dispatch solver time",                                                                                                      "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "disp_solve_state_ann",               "Annual sum of dispatch solve state",                                                                                                      "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "avg_suboptimal_rel_mip_gap",         "Average suboptimal relative MIP gap",                                                                                                     "%",            "",                                  "",                                         "sim_type=1",                                                       "",              ""},

    { SSC_OUTPUT,    SSC_NUMBER, "sim_cpu_run_time",                   "Simulation duration clock time",                                                                                                         "s",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},

    // 12.13.23 twn: for now, need these to defined here to pass downstream to LCOE model
    { SSC_OUTPUT,    SSC_NUMBER, "annual_electricity_consumption",     "Annual electricity consumption w/ avail derate",                                                                                         "kWe-hr",        "",                                 "Post-process",                              "sim_type=1",                                                       "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "electricity_rate",                   "Cost of electricity used to operate lifts and trackers",                                                                                 "$/kWe-hr",      "",                                 "Post-process",                              "sim_type=1",                                                       "",              "" },


    var_info_invalid };

class cm_csp_tower_particle : public compute_module
{
public:

    cm_csp_tower_particle()
    {
        add_var_info(_cm_vtab_csp_tower_particle);
        add_var_info(vtab_adjustment_factors);
        add_var_info(vtab_sf_adjustment_factors);
        add_var_info(vtab_technology_outputs);
    } 

	void exec() override
	{
        //FILE* fp = fopen("particle_cmod_to_lk.lk", "w");
        //write_cmod_to_lk_script(fp, m_vartab);

        std::clock_t clock_start = std::clock();
        int sim_type = as_integer("sim_type");      // 1 (default): time-series, 2: design only
        bool is_dispatch = as_boolean("is_dispatch");

        // *****************************************************
        // System Design Parameters
        // *****************************************************
        double W_dot_cycle_des = as_double("P_ref");                // [MWe]
        double eta_cycle = as_double("design_eff");                 // [-]

        // System Design calculations
        double q_dot_pc_des = W_dot_cycle_des / eta_cycle;          // [MWt]
        double q_dot_rec_des = q_dot_pc_des * as_number("solarm");  // [MWt]

        size_t num_recs = as_integer("num_recs");
        if (num_recs > 4)
            throw exec_error("csp_tower_particle", "Invalid number of receivers. Maximum number of receivers supported is 4.");
        bool duplicate_recs = as_boolean("is_recs_duplicate");

        // *****************************************************
        // Weather reader
        // *****************************************************
		C_csp_weatherreader weather_reader;
		if (is_assigned("solar_resource_file")){
			weather_reader.m_weather_data_provider = make_shared<weatherfile>(as_string("solar_resource_file"));
			if (weather_reader.m_weather_data_provider->has_message()) log(weather_reader.m_weather_data_provider->message(), SSC_WARNING);
		}
		if (is_assigned("solar_resource_data")){
			weather_reader.m_weather_data_provider = make_shared<weatherdata>(lookup("solar_resource_data"));
			if (weather_reader.m_weather_data_provider->has_message()) log(weather_reader.m_weather_data_provider->message(), SSC_WARNING);
		}

        weather_reader.m_trackmode = 0;
        weather_reader.m_tilt = 0.0;
        weather_reader.m_azimuth = 0.0;
        // Initialize to get weather file info
        weather_reader.init();
        if (weather_reader.has_error()) throw exec_error("csp_tower_particle", weather_reader.get_error());

        // *********************************************************
        //      Receiver model
        // *********************************************************

        // Initializing receivers with cmod inputs - these are initial guesses when SolarPILOT optimization is used
        double THT = as_double("h_tower");                                      // [m] tower height
        std::vector<double> rec_height = as_vector_double("fp_rec_height");     // [m] receiver height
        std::vector<double> rec_width = as_vector_double("rec_width");          // [m] receiver width
        if (!duplicate_recs && (rec_height.size() < num_recs || rec_width.size() < num_recs))
            throw exec_error("csp_tower_particle", "Invalid receiver height and/or width input. For non-duplicate receivers, input arrays must have a length of " + util::to_string((int)num_recs));

        bool is_rec_model_clearsky = as_double("rec_clearsky_fraction") > 0.0;
        int rec_clearsky_model = as_integer("rec_clearsky_model");

        if (rec_clearsky_model > 4)
            throw exec_error("csp_tower_particle", "Invalid specification for 'rec_clearsky_model'");
        if (rec_clearsky_model == -1 && as_double("rec_clearsky_fraction") >= 0.0001)
            throw exec_error("csp_tower_particle", "'rec_clearsky_model' must be specified when 'rec_clearsky_fraction' > 0.0.");

        // Reading in multi-receiver geometry inputs
        std::vector<double> rec_azimuth = as_vector_double("rec_azimuth");
        std::vector<double> power_fraction = as_vector_double("power_fraction");
        std::vector<double> norm_curtain_height = as_vector_double("norm_curtain_height");
        std::vector<double> norm_curtain_width = as_vector_double("norm_curtain_width");
        std::vector<double> max_curtain_depth = as_vector_double("max_curtain_depth");

        // Checking array lengths 
        if (!duplicate_recs
            && (rec_azimuth.size() < num_recs
                || power_fraction.size() < num_recs
                || norm_curtain_height.size() < num_recs
                || norm_curtain_width.size() < num_recs
                || max_curtain_depth.size() < num_recs))
                throw exec_error("csp_tower_particle", "Invalid receiver input. For nonduplicate receivers input array must have a length of " + util::to_string((int)num_recs));

        // setting receiver parameters
        double user_efficiency = as_integer("rec_model_type") == 0 ? as_double("rec_eta_fixed") : 0.0;
        double user_hadv = as_integer("rec_adv_model_type") == 0 ? as_double("rec_hadv_fixed") : 0.0;
        int n_x = as_integer("rec_nx");
        int n_y = as_integer("rec_ny");
        if (as_integer("curtain_type") == 1) {
            std::string err_msg = "Curved particle curtain is not currently implemented in the falling particle receiver performance model. Simulated performance will assume a flat curtain.\n";
            log(err_msg, SSC_WARNING);
        }

        size_t input_idx;
        double ap_height, ap_width, curtain_height, curtain_width, ap_curtain_depth_ratio, rec_orientation;
        double A_rec_curtain_total = 0.0, A_rec_aperture_total = 0.0, power_fraction_sum = 0.0;
        std::vector<double> A_rec_aperture(num_recs);
        std::vector<double> A_rec_curtain(num_recs);
        std::vector<double> q_dot_des_per_rec(num_recs);
        std::vector<double> design_heat_loss(num_recs);
        std::vector<std::shared_ptr<C_pt_receiver>> receivers(num_recs);
        for (size_t i = 0; i < num_recs; i++) {
            input_idx = duplicate_recs ? 0 : i;
            power_fraction_sum += power_fraction[input_idx];
        }

        for (size_t i = 0; i < num_recs; i++) {  // loop through receivers
            input_idx = duplicate_recs ? 0 : i; // Use the first element if duplicate receivers
            ap_height = rec_height[input_idx];
            ap_width = rec_width[input_idx];
            curtain_height = ap_height * norm_curtain_height[input_idx];
            curtain_width = ap_width * norm_curtain_width[input_idx];     // TODO: Update curtain width when curved curtains are allowed
            A_rec_curtain.at(i) = curtain_height * curtain_width;  // This receiver area is used to define the flux distribution.  Particle receiver model assumes that the flux distribution is defined based on the curtain area.
            A_rec_aperture.at(i) = ap_height * ap_width;           // The aperture area should be used in cost calculations
            A_rec_aperture_total += A_rec_aperture.at(i);
            A_rec_curtain_total += A_rec_curtain.at(i);
            ap_curtain_depth_ratio = max_curtain_depth[input_idx] / ap_height;
            q_dot_des_per_rec.at(i) = q_dot_rec_des * (power_fraction[input_idx] / power_fraction_sum);  // Receiver design point power
            rec_orientation = rec_azimuth[i];

            receivers.at(i) = std::shared_ptr<C_falling_particle_receiver>(new C_falling_particle_receiver(
                THT, as_double("T_htf_hot_des"), as_double("T_htf_cold_des"),
                as_double("f_rec_min"), q_dot_des_per_rec.at(i),
                as_double("rec_su_delay"), as_double("rec_qf_delay"),
                as_double("csp.pt.rec.max_oper_frac"), as_double("eta_lift"),
                as_integer("rec_htf"), as_matrix("field_fl_props"),
                as_integer("rec_model_type"), user_efficiency, as_integer("rec_rad_model_type"), as_integer("rec_adv_model_type"), user_hadv,
                ap_height, ap_width, norm_curtain_height[input_idx], norm_curtain_width[input_idx], ap_curtain_depth_ratio, rec_orientation,
                as_double("particle_dp"), as_double("particle_abs"), as_double("curtain_emis"), as_double("curtain_dthdy"),
                as_double("cav_abs"), as_double("cav_emis"), as_double("cav_twall"), as_double("cav_kwall"), as_double("cav_hext"),
                as_double("transport_deltaT_cold"), as_double("transport_deltaT_hot"),
                as_double("rec_tauc_mult"), as_double("rec_hadv_mult"),
                n_x, n_y, as_integer("rec_rad_nx"), as_integer("rec_rad_ny"),
                as_double("T_htf_hot_des"), as_double("rec_clearsky_fraction")));

            receivers.at(i)->init();
            design_heat_loss.at(i) = receivers.at(i)->getHeatLossPerApertureArea() / 1000.0;    // [W/m^2] -> [kW/m^2]
            if (design_heat_loss.at(i) != design_heat_loss.at(i)) {// Receiver model failed to solve -> NAN result
                if (as_integer("field_model_type") == 0) { // Heliostat field optimization to follow
                    design_heat_loss.at(i) = 0.0;
                    std::string msg;
                    msg = util::format("Receiver %d failed to converge at design condition. Setting receiver heat loss to 0.0 for SolarPILOT. \n"
                        "Resulting heliostat field could be undersized.", i+1);
                    log(msg, SSC_WARNING);
                }
                else { // Throw error if not running optimization
                    if (sim_type == 1) {
                        std::string msg;
                        msg = util::format("Receiver %d failed to converge at design condition. "
                            "Aperture size is most likely too large or receiver parameters are unrealistic.", i+1);
                        throw exec_error("csp_tower_particle", msg);
                    }
                    else {
                        design_heat_loss.at(i) = std::numeric_limits<double>::quiet_NaN();
                    }
                }
            }
        }

        // Assign design receiver heat loss for SolarPILOT
        std::string hl_msg;
        ssc_number_t* p_rec_hl = allocate("rec_design_hl", design_heat_loss.size());
        for (size_t i = 0; i < design_heat_loss.size(); i++) {
            p_rec_hl[i] = (ssc_number_t)design_heat_loss.at(i);
        }

        // *****************************************************
        // Heliostat Field Model
        // *****************************************************

        // 'field_model_type'
        // 0 = optimize design field and tower/receiver geometry
        // 1 = design field
        // 2 = user specified heliostat positions, SolarPILOT calculates performance
        // 3 = user flux and eta maps, pass heliostat_positions to SolarPILOT for layout
        // 4 = user flux and eta maps, no SolarPILOT, input A_sf_in, total_land_area_in, and N_hel
        int field_model_type = as_integer("field_model_type");

        bool is_design_field_at_sim_type_2 = as_boolean("is_design_field_for_sim_type_2");

        if (!is_design_field_at_sim_type_2) {
            if (sim_type == 2 && field_model_type < 2) {
                field_model_type = 2;  //skip heliostat design, only users maps are not provided
            }
        }

        // Run SolarPILOT right away to update values as needed
        assign("receiver_type", var_receiver::REC_TYPE::FALLING_PARTICLE);
        // SolarPILOT outputs
        util::matrix_t<double> mt_eta_map;         // [deg, deg, -] Azimuth, zenith, solar field efficiency (per receiver)
        util::matrix_t<double> mt_flux_maps;       // Receiver flux maps
        util::matrix_t<double> helio_pos;          // [m, m] X, Y positions of heliostats
        std::vector<int> N_hel;                                                 // [-] Number of heliostats
        std::vector<double> A_sf;                                               // [m^2] Solar field area 
        double land_area_base = std::numeric_limits<double>::quiet_NaN();       // [acres] base land area
        double total_land_area = std::numeric_limits<double>::quiet_NaN();      // [acres] Total land area
        double heliostat_area = std::numeric_limits<double>::quiet_NaN();       // [m^2] heliostat reflective area
        double average_attenuation = std::numeric_limits<double>::quiet_NaN();  // [%] average attenuation loss
        double refl_image_error = std::numeric_limits<double>::quiet_NaN();     // [mrad] Reflected image conical error 
        double land_max_abs = std::numeric_limits<double>::quiet_NaN();         // [m] maximum distance from tower
        double land_min_abs = std::numeric_limits<double>::quiet_NaN();         // [m] minimum distance from tower
        
        double helio_optical_error_mrad = as_number("helio_optical_error_mrad");        //[mrad]
        refl_image_error = std::sqrt(2. * helio_optical_error_mrad * 2. * helio_optical_error_mrad * 2.);   //[mrad]
        assign("refl_image_error", refl_image_error);   //[mrad]
        assign("helio_optical_error", (ssc_number_t)(helio_optical_error_mrad * 1.E-3));

        if (field_model_type < 4) {
            // Field types 0-3 Requires solarPILOT
            if (field_model_type == 0 && sim_type == 1) {
                // Run heliostat field / receiver optimization first
                solarpilot_invoke spi_opt(this);
                assign("q_design", q_dot_rec_des);       //[MWt]

                // Optimize design field and tower/receiver geometry without flux_map calculations
                assign("is_optimize", 1); 
                assign("calc_fluxmaps", 0);

                // Check azimuth input for optimization:
                if (num_recs > 1 && rec_azimuth.front() <= 0.0) {
                    throw exec_error("csp_tower_particle", "The first receiver azimuth must be greater than zero"
                        "when using the heliostat field optimization method for a multi-receiver system.");
                }

                spi_opt.run(weather_reader.m_weather_data_provider);    // Runs SolarPILOT
                weather_reader.m_weather_data_provider->rewind();       // Reset for next SolarPILOT run

                // Get optimized sizing
                size_t sp_num_recs = spi_opt.recs.size();
                if (sp_num_recs != num_recs)
                    throw exec_error("csp_tower_particle", "Unexpected error: SolarPILOT's receivers count is inconsistent with user input.");

                // Allocate optimization variables so next solarPilot run can use the values
                THT = spi_opt.sf.tht.val;
                assign("h_tower", THT);
                rec_height.resize(num_recs);
                rec_width.resize(num_recs);
                rec_azimuth.resize(num_recs);
                ssc_number_t* p_rec_height_opt = allocate("fp_rec_height", num_recs);
                ssc_number_t* p_rec_width_opt = allocate("rec_width", num_recs);
                ssc_number_t* p_rec_azimuth_opt = allocate("rec_azimuth", num_recs);
                for (size_t i = 0; i < spi_opt.recs.size(); i++) {
                    p_rec_height_opt[i] = rec_height[i] = spi_opt.recs.at(i).rec_height.val;
                    p_rec_width_opt[i] = rec_width[i] = spi_opt.recs.at(i).rec_width.val;
                    p_rec_azimuth_opt[i] = rec_azimuth[i] = spi_opt.recs.at(i).rec_azimuth.val;
                }

                // Re-initialize receivers 
                A_rec_aperture_total = A_rec_curtain_total = 0.0;
                for (size_t i = 0; i < num_recs; i++) {  // loop through receivers
                    input_idx = duplicate_recs ? 0 : i; // Use the first element if duplicate receivers
                    curtain_height = rec_height[input_idx] * norm_curtain_height[input_idx];
                    curtain_width = rec_width[input_idx] * norm_curtain_width[input_idx];     // TODO: Update curtain width when curved curtains are allowed
                    A_rec_curtain.at(i) = curtain_height * curtain_width;  // This receiver area is used to define the flux distribution.  Particle receiver model assumes that the flux distribution is defined based on the curtain area.
                    A_rec_aperture.at(i) = rec_height[input_idx] * rec_width[input_idx];           // The aperture area should be used in cost calculations
                    A_rec_aperture_total += A_rec_aperture.at(i);
                    A_rec_curtain_total += A_rec_curtain.at(i);
                    receivers.at(i)->update_sizing(rec_height[input_idx], rec_width[input_idx], rec_azimuth[i], THT);
                    design_heat_loss.at(i) = receivers.at(i)->getHeatLossPerApertureArea() / 1000.0;    // [W/m^2] -> [kW/m^2]
                }

                // Re-assign heat loss for heliostat field final design
                for (size_t i = 0; i < design_heat_loss.size(); i++) {
                    p_rec_hl[i] = (ssc_number_t)design_heat_loss.at(i);
                }
            }

            solarpilot_invoke spi(this);
            assign("q_design", q_dot_rec_des);       //[MWt]

            // TODO: Check 'n_flux_x' and 'n_flux_y'?
            //      - n_flux_y must be greater than the number of troughs in the curtain
            //      - Add a limit to these values <= 50?
            //if (as_integer("n_flux_x") < 1 || as_integer("n_flux_y") < 5)

            // Default configuration specific case will overwrite these values
            assign("is_optimize", 0);            // Turn-off heliostat layout and tower optimization
            assign("calc_fluxmaps", 1);          // Include flux map calculations

            if (field_model_type == 3 || sim_type == 2) {
                assign("calc_fluxmaps", 0); // efficiency and flux maps are provide by the user
            }

            // Process user provided solar field
            if (field_model_type == 2 || field_model_type == 3) { 
                // Only calculates a flux map, so need to "assign" 'helio_positions_in' for SolarPILOT cmod
                util::matrix_t<double> helio_pos_temp = as_matrix("helio_positions");
                size_t n_h_rows = helio_pos_temp.nrows();
                ssc_number_t* p_helio_positions_in = allocate("helio_positions_in", n_h_rows, 2);

                for (size_t i = 0; i < n_h_rows; i++) {
                    p_helio_positions_in[i * 2] = (ssc_number_t)helio_pos_temp(i, 0);       //[m] x
                    p_helio_positions_in[i * 2 + 1] = (ssc_number_t)helio_pos_temp(i, 1);   //[m] y
                }
            }

            if (sim_type == 2) {
                // For design-point a single receiver is adequate for the UI outputs
                // Multi-receivers requires receiver-heliostat assignment problem to be solved
                //      each UI callback -> results in UI delay.
                assign("num_recs", 1);
            }

            // SolarPILOT will check "heliostat_positions_in" to determine if it needs to layout the field
            spi.run(weather_reader.m_weather_data_provider);    // Runs SolarPILOT

            // Collect the optical efficiency and flux map data
            if (field_model_type <= 2) {
                if (sim_type == 1) {
                    spi.getHeliostatFieldEfficiency(mt_eta_map);
                    spi.getReceiverFluxMaps(mt_flux_maps);
                }
                else { // sim_type == 2
                    // Filling maps with dummy values
                    mt_eta_map.resize_fill(1, (size_t)(2 + num_recs), std::numeric_limits<double>::quiet_NaN());
                    mt_flux_maps.resize_fill(as_integer("n_flux_y"), as_integer("n_flux_x") * num_recs, std::numeric_limits<double>::quiet_NaN());
                }
            }
            else if (field_model_type == 3) { // Read in user efficiency and flux maps
                mt_eta_map = as_matrix("eta_map");

                // Remove the solar position from flux_maps
                util::matrix_t<double> raw_flux_maps = as_matrix("flux_maps");
                importFluxMaps(raw_flux_maps, &mt_flux_maps);
            }
            else {
                string msg = util::format("Invalid combination of field_model_type and sim_type");
                throw exec_error("CSP Solver", msg);
            }

            // Count heliostats per receiver and get positions
            N_hel.clear();
            N_hel.resize(num_recs, 0);
            int tot_N_hel = (int)spi.layout.heliostat_positions.size();
            helio_pos.resize(tot_N_hel, 3);
            for (int i = 0; i < tot_N_hel; i++) {
                helio_pos(i, 0) = (ssc_number_t)spi.layout.heliostat_positions.at(i).location.x;
                helio_pos(i, 1) = (ssc_number_t)spi.layout.heliostat_positions.at(i).location.y;
                //helio_pos(i, 2) = (ssc_number_t)spi.layout.heliostat_positions.at(i).which_rec;
                N_hel[spi.layout.heliostat_positions.at(i).which_rec]++;
            }

            THT = spi.sf.tht.val;

            if (sim_type == 1) {    // Only check this when annual simulation is taking place.
                size_t sp_num_recs = spi.recs.size();
                if (sp_num_recs != num_recs)
                    throw exec_error("csp_tower_particle", "Unexpected error: SolarPILOT's receivers count is inconsistent with user input.");
            }

            rec_height.resize(num_recs);
            rec_width.resize(num_recs);
            rec_azimuth.resize(num_recs);
            for (size_t i = 0; i < spi.recs.size(); i++) {
                rec_height[i] = spi.recs.at(i).rec_height.val;
                rec_width[i] = spi.recs.at(i).rec_width.val;
                rec_azimuth[i] = spi.recs.at(i).rec_azimuth.val;
            }

            A_sf.clear();
            A_sf.resize(num_recs);
            for (size_t i = 0; i < spi.recs.size(); i++) {
                A_sf[i] = spi.CalcSolarFieldArea(N_hel[i]);
            }
            heliostat_area = spi.CalcHeliostatArea();
            average_attenuation = spi.CalcAveAttenuation();

            land_min_abs = as_double("land_min") * THT;     //[m]
            land_max_abs = as_double("land_max") * THT;     //[m]

            total_land_area = spi.GetTotalLandArea();       // [acres] Total land area
            land_area_base = spi.GetBaseLandArea();         // [acres] Land area occupied by heliostats
        }
        else if (field_model_type == 4) {
            // User input flux and efficiency maps, no SolarPILOT needed
            mt_eta_map = as_matrix("eta_map");
            // Remove the solar position from flux_maps
            util::matrix_t<double> raw_flux_maps = as_matrix("flux_maps");
            importFluxMaps(raw_flux_maps, &mt_flux_maps);

            // Need to specify:
            // 1) reflective area (scale flux map)
            // 2) number heliostats for heliostats (tracking parasitic)
            // 3) total land area
            // 4) tower and receiver dimensions
            total_land_area = as_double("total_land_area_in");  //[acres]
            N_hel = as_vector_integer("N_hel");
            A_sf = as_vector_double("A_sf_in");
            if (N_hel.size() != num_recs
                || A_sf.size() != num_recs)
                throw exec_error("csp_tower_particle",
                    "Invalid field input. Either N_hel or A_sf_in length is not equal to the number of receivers: " + util::to_string((int)num_recs));

            helio_pos.resize_fill(1, 2, std::numeric_limits<double>::quiet_NaN());

            // Don't define land_area_base because we're not requiring it as an input in this field model type
            land_area_base = std::numeric_limits<double>::quiet_NaN();
        }
        else
        {
            string msg = util::format("One field performance modeling option must be set to True");
            throw exec_error("CSP Solver", msg);
        }

        // *****************************************************
        // simulation setup
        // *****************************************************
        // Set steps per hour
        C_csp_solver::S_sim_setup sim_setup;
        sim_setup.m_sim_time_start = as_double("time_start");       //[s] time at beginning of first time step
        sim_setup.m_sim_time_end = as_double("time_stop");          //[s] time at end of last time step
        
        int steps_per_hour = (int)as_double("time_steps_per_hour");     //[-]

        //if the number of steps per hour is not provided (=-1), then assign it based on the weather file step
        if( steps_per_hour < 0 )
        {
            double sph_d = 3600. / weather_reader.m_weather_data_provider->step_sec();
            steps_per_hour = (int)( sph_d + 1.e-5 );
            if ((double)steps_per_hour != sph_d)
                throw exec_error("CSP Solver", "The time step duration must be evenly divisible within an hour.");
        }

        size_t n_steps_fixed = (size_t)steps_per_hour * 8760;   //[-]
        if( as_boolean("vacuum_arrays") )
            n_steps_fixed = steps_per_hour * (size_t)( (sim_setup.m_sim_time_end - sim_setup.m_sim_time_start)/3600. );
        sim_setup.m_report_step = 3600.0 / (double)steps_per_hour;  //[s] 

        // ***********************************************
        // Power cycle
        // ***********************************************
        C_csp_power_cycle * p_csp_power_cycle;
        // Steam Rankine and User Defined power cycle classes
        C_pc_Rankine_indirect_224 rankine_pc;

        // Check power block type
        int pb_tech_type = as_integer("pc_config");
        if (pb_tech_type == 0 || pb_tech_type == 1)     // Rankine or User Defined
        {
            C_pc_Rankine_indirect_224::S_params *pc = &rankine_pc.ms_params;
            pc->m_P_ref = as_double("P_ref");
            pc->m_eta_ref = as_double("design_eff");
            pc->m_T_htf_hot_ref = as_double("T_htf_hot_des");
            pc->m_T_htf_cold_ref = as_double("T_htf_cold_des");
            pc->m_cycle_max_frac = as_double("cycle_max_frac");
            pc->m_cycle_cutoff_frac = as_double("cycle_cutoff_frac");
            pc->m_q_sby_frac = as_double("q_sby_frac");
            pc->m_startup_time = as_double("startup_time");
            pc->m_startup_frac = as_double("startup_frac");

            // Calculate power block pumping coefficient based on TES and PHX heights -> used calculating lift power
            pc->m_htf_pump_coef = ((as_double("h_tank") + as_double("phx_height")) * 9.8067 / as_double("eta_lift")) / 1.e3; // Convert from W/kg/s to kW/kg/s
            pc->m_pc_fl = as_integer("rec_htf");                            // power cycle HTF is same as receiver HTF
            pc->m_pc_fl_props = as_matrix("field_fl_props");

            if (pb_tech_type == 0)
            {
                pc->m_P_boil_des = 100;     //[bar]
                pc->m_dT_cw_ref = as_double("dT_cw_ref");
                pc->m_T_amb_des = as_double("T_amb_des");
                pc->m_CT = as_integer("CT");                    // cooling tech type: 1=evaporative, 2=air
                if (pc->m_CT > 2) {
                    std::string err_msg = util::format("The specified power cycle cooling tech type, %d, does not exist"
                        " for the Particle Tower heating model. Choose from 1) evaporative or 2) air\n", pc->m_CT);
                    log(err_msg, SSC_WARNING);
                    return;
                }
                pc->m_tech_type = as_integer("tech_type");      // 1: Fixed, 3: Sliding
                if (pc->m_tech_type == 2) { pc->m_tech_type = 1; }; // changing fixed pressure for the trough to fixed pressure for the tower
                //if (pc->m_tech_type == 8) { pc->m_tech_type = 3; }; // changing sliding pressure for the trough to sliding pressure for the tower  ->  don't, this disallows the use of the old tower sliding curves
                
                if (!(pc->m_tech_type == 1 || pc->m_tech_type == 3 || pc->m_tech_type ==5 || pc->m_tech_type==6 || pc->m_tech_type == 7 || pc->m_tech_type == 8))
                {
                    std::string tech_msg = util::format("tech_type must be either 1 (fixed pressure) or 3 (sliding). Input was %d."
                        " Simulation proceeded with fixed pressure", pc->m_tech_type);
                    pc->m_tech_type = 1;
                }
                pc->m_T_approach = as_double("T_approach");
                pc->m_T_ITD_des = as_double("T_ITD_des");
                pc->m_P_cond_ratio = as_double("P_cond_ratio");
                pc->m_pb_bd_frac = as_double("pb_bd_frac");
                pc->m_P_cond_min = as_double("P_cond_min");
                pc->m_n_pl_inc = as_integer("n_pl_inc");

                // Set User Defined cycle parameters to appropriate values
                pc->m_is_user_defined_pc = false;
                pc->m_W_dot_cooling_des = std::numeric_limits<double>::quiet_NaN();
            }
            else if (pb_tech_type == 1)
            {
                pc->m_is_user_defined_pc = true;

                // User-Defined Cycle Parameters
                pc->m_W_dot_cooling_des = as_double("ud_f_W_dot_cool_des") / 100.0*as_double("P_ref");  //[MWe]
                pc->m_m_dot_water_des = as_double("ud_m_dot_water_cool_des");       //[kg/s]
                pc->m_is_udpc_sco2_regr = as_boolean("ud_is_sco2_regr");            //[-]

                // User-Defined Cycle Off-Design Tables 
                pc->mc_combined_ind = as_matrix("ud_ind_od");
            }

            // Set pointer to parent class
            p_csp_power_cycle = &rankine_pc;
        }
        else
        {
            std::string err_msg = util::format("The specified power cycle configuration, %d, does not exist. See SSC Input Table for help.\n", pb_tech_type);
            log(err_msg, SSC_WARNING);
            return;
        }

        // Set power cycle outputs common to all power cycle technologies
        p_csp_power_cycle->assign(C_pc_Rankine_indirect_224::E_Q_DOT_HTF, allocate("q_pb", n_steps_fixed), n_steps_fixed);
        p_csp_power_cycle->assign(C_pc_Rankine_indirect_224::E_M_DOT_HTF, allocate("m_dot_pc", n_steps_fixed), n_steps_fixed);
        p_csp_power_cycle->assign(C_pc_Rankine_indirect_224::E_Q_DOT_STARTUP, allocate("q_dot_pc_startup", n_steps_fixed), n_steps_fixed);
        p_csp_power_cycle->assign(C_pc_Rankine_indirect_224::E_W_DOT, allocate("P_cycle", n_steps_fixed), n_steps_fixed);
        p_csp_power_cycle->assign(C_pc_Rankine_indirect_224::E_T_HTF_IN, allocate("T_pc_in", n_steps_fixed), n_steps_fixed);
        p_csp_power_cycle->assign(C_pc_Rankine_indirect_224::E_T_HTF_OUT, allocate("T_pc_out", n_steps_fixed), n_steps_fixed);
        p_csp_power_cycle->assign(C_pc_Rankine_indirect_224::E_M_DOT_WATER, allocate("m_dot_water_pc", n_steps_fixed), n_steps_fixed);
        p_csp_power_cycle->assign(C_pc_Rankine_indirect_224::E_T_COND_OUT, allocate("T_cond_out", n_steps_fixed), n_steps_fixed);
        p_csp_power_cycle->assign(C_pc_Rankine_indirect_224::E_W_DOT_HTF_PUMP, allocate("cycle_htf_lift_power", n_steps_fixed), n_steps_fixed);
        p_csp_power_cycle->assign(C_pc_Rankine_indirect_224::E_W_DOT_COOLER, allocate("P_cooling_tower_tot", n_steps_fixed), n_steps_fixed);
        p_csp_power_cycle->assign(C_pc_Rankine_indirect_224::E_P_COND, allocate("P_cond", n_steps_fixed), n_steps_fixed);
        p_csp_power_cycle->assign(C_pc_Rankine_indirect_224::E_P_COND_ITER_ERR, allocate("P_cond_iter_err", n_steps_fixed), n_steps_fixed);
        p_csp_power_cycle->assign(C_pc_Rankine_indirect_224::E_ETA_THERMAL, allocate("eta", n_steps_fixed), n_steps_fixed);


        // *********************************************************
        //      Heliostat field class
        // *******************************************************
        // Formatting efficiency data and flux maps for heliostat field class
        std::vector<util::matrix_t<double>> eta_maps;
        {// scope for temp_eta_map
            util::matrix_t<double> temp_eta_map;
            temp_eta_map.resize(mt_eta_map.nrows(), 3);
            for (size_t r = 0; r < mt_eta_map.nrows(); r++) {
                temp_eta_map.at(r, 0) = mt_eta_map.at(r, 0);
                temp_eta_map.at(r, 1) = mt_eta_map.at(r, 1);
            }
            for (size_t rec = 0; rec < num_recs; rec++) {
                for (size_t r = 0; r < mt_eta_map.nrows(); r++) {
                    temp_eta_map.at(r, 2) = mt_eta_map.at(r, 2 + rec);
                }
                eta_maps.push_back(temp_eta_map);
            }
        }

        std::vector<util::matrix_t<double>> rec_flux_maps;
        {// scope for temp_flux_map
            size_t nx = as_integer("n_flux_x");
            util::matrix_t<double> temp_flux_map;
            temp_flux_map.resize(mt_flux_maps.nrows(), nx );
            for (size_t rec = 0; rec < num_recs; rec++) {
                for (size_t r = 0; r < mt_flux_maps.nrows(); r++) {
                    for (size_t c = 0; c < nx; c++) {
                        temp_flux_map.at(r, c) = mt_flux_maps.at(r, rec * nx + c);
                    }
                }
                rec_flux_maps.push_back(temp_flux_map);
            }
        }

        // *******************************************************
        // Construct heliostat field class after receiver
        //    so it can use the active receiver area

        // Calculate clear-sky DNI (if relevant)
        size_t n_steps_full = weather_reader.m_weather_data_provider->nrecords();
        std::vector<double> clearsky_data;
        if (as_integer("rec_clearsky_model") == 0)
        {
            size_t n_csky = 0;
            ssc_number_t* csky = as_array("rec_clearsky_dni", &n_csky);
            if (n_csky != n_steps_full)
                throw exec_error("csp_tower_particle", "Invalid clear-sky DNI data. Array must have " + util::to_string((int)n_steps_full) + " rows.");

            clearsky_data.resize(n_steps_full);
            for (size_t i = 0; i < n_steps_full; i++)
                clearsky_data.at(i) = (double)csky[i];
        }

        //Load the solar field adjustment factors.  Assume the same adjustment factors apply to each heliostat field
        adjustment_factors sf_haf(this->get_var_table(), "sf_adjust");
        if (!sf_haf.setup(n_steps_full))
            throw exec_error("csp_tower_particle", "failed to setup sf adjustment factors: " + sf_haf.error());



        std::vector<C_pt_sf_perf_interp*> heliostatfields(num_recs);
        for (size_t i = 0; i < num_recs; i++)   // loop through receivers
        {
            heliostatfields.at(i) = new C_pt_sf_perf_interp(A_rec_curtain.at(i));
            heliostatfields.at(i)->ms_params.m_p_start = as_double("p_start");      //[kWe-hr] Heliostat startup energy
            heliostatfields.at(i)->ms_params.m_p_track = as_double("p_track");      //[kWe] Heliostat tracking power
            heliostatfields.at(i)->ms_params.m_hel_stow_deploy = as_double("hel_stow_deploy");  // N/A
            heliostatfields.at(i)->ms_params.m_v_wind_max = as_double("v_wind_max");            // N/A
            heliostatfields.at(i)->ms_params.m_eta_map = eta_maps[i];
            heliostatfields.at(i)->ms_params.m_flux_maps = rec_flux_maps[i];
            heliostatfields.at(i)->ms_params.m_n_flux_x = (int)rec_flux_maps[i].ncols();
            heliostatfields.at(i)->ms_params.m_n_flux_y = (int)(rec_flux_maps[i].nrows() / eta_maps[i].nrows());
            heliostatfields.at(i)->ms_params.m_N_hel = N_hel[i];
            heliostatfields.at(i)->ms_params.m_A_sf = A_sf[i];        //[m2]
            if (field_model_type < 3)
            {
                heliostatfields.at(i)->ms_params.m_eta_map_aod_format = false;
            }
            else
            {
                heliostatfields.at(i)->ms_params.m_eta_map_aod_format = as_boolean("eta_map_aod_format");
            }

            //Clearsky DNI
            heliostatfields.at(i)->ms_params.m_clearsky_model = as_integer("rec_clearsky_model");
            heliostatfields.at(i)->ms_params.mv_clearsky_data = clearsky_data;

            //Solar field adjustment factors
            heliostatfields.at(i)->ms_params.m_sf_adjust.resize(sf_haf.size());
            for (size_t j = 0; j < sf_haf.size(); j++)  // TODO: Could do this just one for all receivers
                heliostatfields.at(i)->ms_params.m_sf_adjust.at(j) = sf_haf(j);

            // TODO: do we need this?
            //heliostatfields.at(i)->mf_callback = ssc_cmod_solarpilot_callback;
            //heliostatfields.at(i)->m_cdata = (void*)this;
 
        }

        // Now try to instantiate mspt_collector_receiver
        bool is_sep_modes = as_boolean("is_sep_rec_modes");
        C_csp_falling_particle_collector_receiver collector_receiver(heliostatfields, receivers, is_sep_modes);

        // Then try init() call here, which should call inits from both classes
        //collector_receiver.init();

        // *******************************************************
        // Set receiver outputs
        //float *p_q_thermal_copy = allocate("Q_thermal_123", n_steps_fixed);
        collector_receiver.mc_reported_outputs.assign(C_csp_falling_particle_collector_receiver::E_FIELD_Q_DOT_INC, allocate("q_sf_inc", n_steps_fixed), n_steps_fixed);
        collector_receiver.mc_reported_outputs.assign(C_csp_falling_particle_collector_receiver::E_FIELD_ETA_OPT, allocate("eta_field", n_steps_fixed), n_steps_fixed);
        collector_receiver.mc_reported_outputs.assign(C_csp_falling_particle_collector_receiver::E_FIELD_ADJUST, allocate("sf_adjust_out", n_steps_fixed), n_steps_fixed);
        collector_receiver.mc_reported_outputs.assign(C_csp_falling_particle_collector_receiver::E_REC_DEFOCUS, allocate("rec_defocus", n_steps_fixed), n_steps_fixed);
        collector_receiver.mc_reported_outputs.assign(C_csp_falling_particle_collector_receiver::E_Q_DOT_INC, allocate("q_dot_rec_inc", n_steps_fixed), n_steps_fixed);
        collector_receiver.mc_reported_outputs.assign(C_csp_falling_particle_collector_receiver::E_ETA_THERMAL, allocate("eta_therm", n_steps_fixed), n_steps_fixed);
        collector_receiver.mc_reported_outputs.assign(C_csp_falling_particle_collector_receiver::E_Q_DOT_THERMAL, allocate("Q_thermal", n_steps_fixed), n_steps_fixed);
        collector_receiver.mc_reported_outputs.assign(C_csp_falling_particle_collector_receiver::E_M_DOT_HTF, allocate("m_dot_rec", n_steps_fixed), n_steps_fixed);
        collector_receiver.mc_reported_outputs.assign(C_csp_falling_particle_collector_receiver::E_Q_DOT_STARTUP, allocate("q_startup", n_steps_fixed), n_steps_fixed);
        collector_receiver.mc_reported_outputs.assign(C_csp_falling_particle_collector_receiver::E_T_HTF_IN, allocate("T_rec_in", n_steps_fixed), n_steps_fixed);
        collector_receiver.mc_reported_outputs.assign(C_csp_falling_particle_collector_receiver::E_T_HTF_OUT, allocate("T_rec_out", n_steps_fixed), n_steps_fixed);
        collector_receiver.mc_reported_outputs.assign(C_csp_falling_particle_collector_receiver::E_T_CAV_WALL_MAX, allocate("T_cav_wall_max", n_steps_fixed), n_steps_fixed);
        collector_receiver.mc_reported_outputs.assign(C_csp_falling_particle_collector_receiver::E_T_CAV_WALL_AVG, allocate("T_cav_wall_avg", n_steps_fixed), n_steps_fixed);
        collector_receiver.mc_reported_outputs.assign(C_csp_falling_particle_collector_receiver::E_Q_DOT_PIPE_LOSS, allocate("q_transport_loss", n_steps_fixed), n_steps_fixed);
        collector_receiver.mc_reported_outputs.assign(C_csp_falling_particle_collector_receiver::E_Q_DOT_LOSS, allocate("q_thermal_loss", n_steps_fixed), n_steps_fixed);
        collector_receiver.mc_reported_outputs.assign(C_csp_falling_particle_collector_receiver::E_Q_DOT_REFL_LOSS, allocate("q_reflection_loss", n_steps_fixed), n_steps_fixed);
        collector_receiver.mc_reported_outputs.assign(C_csp_falling_particle_collector_receiver::E_W_DOT_TRACKING, allocate("pparasi", n_steps_fixed), n_steps_fixed);
        collector_receiver.mc_reported_outputs.assign(C_csp_falling_particle_collector_receiver::E_W_DOT_PUMP, allocate("P_tower_lift", n_steps_fixed), n_steps_fixed);

        if (is_rec_model_clearsky) {
            collector_receiver.mc_reported_outputs.assign(C_csp_falling_particle_collector_receiver::E_CLEARSKY, allocate("clearsky", n_steps_fixed), n_steps_fixed);
        }

        if (num_recs > 1)
        {
            collector_receiver.mc_reported_outputs.assign(C_csp_falling_particle_collector_receiver::E_Q_DOT_INC_1, allocate("q_dot_rec_inc_1", n_steps_fixed), n_steps_fixed);
            collector_receiver.mc_reported_outputs.assign(C_csp_falling_particle_collector_receiver::E_Q_DOT_THERMAL_1, allocate("Q_thermal_1", n_steps_fixed), n_steps_fixed);
            collector_receiver.mc_reported_outputs.assign(C_csp_falling_particle_collector_receiver::E_Q_DOT_STARTUP_1, allocate("q_startup_1", n_steps_fixed), n_steps_fixed);
            collector_receiver.mc_reported_outputs.assign(C_csp_falling_particle_collector_receiver::E_ETA_THERMAL_1, allocate("eta_therm_1", n_steps_fixed), n_steps_fixed);
            collector_receiver.mc_reported_outputs.assign(C_csp_falling_particle_collector_receiver::E_FIELD_ETA_OPT_1, allocate("eta_field_1", n_steps_fixed), n_steps_fixed);

            collector_receiver.mc_reported_outputs.assign(C_csp_falling_particle_collector_receiver::E_Q_DOT_INC_2, allocate("q_dot_rec_inc_2", n_steps_fixed), n_steps_fixed);
            collector_receiver.mc_reported_outputs.assign(C_csp_falling_particle_collector_receiver::E_Q_DOT_THERMAL_2, allocate("Q_thermal_2", n_steps_fixed), n_steps_fixed);
            collector_receiver.mc_reported_outputs.assign(C_csp_falling_particle_collector_receiver::E_Q_DOT_STARTUP_2, allocate("q_startup_2", n_steps_fixed), n_steps_fixed);
            collector_receiver.mc_reported_outputs.assign(C_csp_falling_particle_collector_receiver::E_ETA_THERMAL_2, allocate("eta_therm_2", n_steps_fixed), n_steps_fixed);
            collector_receiver.mc_reported_outputs.assign(C_csp_falling_particle_collector_receiver::E_FIELD_ETA_OPT_2, allocate("eta_field_2", n_steps_fixed), n_steps_fixed);
        }
        if (num_recs > 2)
        {
            collector_receiver.mc_reported_outputs.assign(C_csp_falling_particle_collector_receiver::E_Q_DOT_INC_3, allocate("q_dot_rec_inc_3", n_steps_fixed), n_steps_fixed);
            collector_receiver.mc_reported_outputs.assign(C_csp_falling_particle_collector_receiver::E_Q_DOT_THERMAL_3, allocate("Q_thermal_3", n_steps_fixed), n_steps_fixed);
            collector_receiver.mc_reported_outputs.assign(C_csp_falling_particle_collector_receiver::E_Q_DOT_STARTUP_3, allocate("q_startup_3", n_steps_fixed), n_steps_fixed);
            collector_receiver.mc_reported_outputs.assign(C_csp_falling_particle_collector_receiver::E_ETA_THERMAL_3, allocate("eta_therm_3", n_steps_fixed), n_steps_fixed);
            collector_receiver.mc_reported_outputs.assign(C_csp_falling_particle_collector_receiver::E_FIELD_ETA_OPT_3, allocate("eta_field_3", n_steps_fixed), n_steps_fixed);
        }
        if (num_recs > 3)
        {
            collector_receiver.mc_reported_outputs.assign(C_csp_falling_particle_collector_receiver::E_Q_DOT_INC_4, allocate("q_dot_rec_inc_4", n_steps_fixed), n_steps_fixed);
            collector_receiver.mc_reported_outputs.assign(C_csp_falling_particle_collector_receiver::E_Q_DOT_THERMAL_4, allocate("Q_thermal_4", n_steps_fixed), n_steps_fixed);
            collector_receiver.mc_reported_outputs.assign(C_csp_falling_particle_collector_receiver::E_Q_DOT_STARTUP_4, allocate("q_startup_4", n_steps_fixed), n_steps_fixed);
            collector_receiver.mc_reported_outputs.assign(C_csp_falling_particle_collector_receiver::E_ETA_THERMAL_4, allocate("eta_therm_4", n_steps_fixed), n_steps_fixed);
            collector_receiver.mc_reported_outputs.assign(C_csp_falling_particle_collector_receiver::E_FIELD_ETA_OPT_4, allocate("eta_field_4", n_steps_fixed), n_steps_fixed);
        }


        // *****************************************************
        // Electric Heater
        // Check if system configuration includes a heater parallel to primary collector receiver
        C_csp_collector_receiver* p_heater;
        C_csp_cr_electric_resistance* p_electric_resistance = NULL;
        bool is_parallel_heater = as_boolean("is_parallel_htr");    // defaults to false
        double q_dot_heater_des = 0.0;  //[MWt]
        double heater_spec_cost = 0.0;
        if (is_parallel_heater) {

            if (!is_dispatch && sim_type == 1) {
                if (!as_boolean("allow_heater_no_dispatch_opt")) {
                    throw exec_error("csp_tower_particle", "When the particle power tower case has an electric HTF charger, dispatch optimization must be selected");
                }
            }

            double heater_mult = as_double("heater_mult");      //[-]
            heater_spec_cost = as_double("heater_spec_cost");   //[$/kWt]

            q_dot_heater_des = q_dot_pc_des*heater_mult;     //[MWt]

            double heater_efficiency = as_double("heater_efficiency") / 100.0;          //[-] convert from % input
            double f_q_dot_des_allowable_su = as_double("f_q_dot_des_allowable_su");    //[-] fraction of design power allowed during startup
            double hrs_startup_at_max_rate = as_double("hrs_startup_at_max_rate");      //[hr] duration of startup at max startup power
            double f_heater_min = as_double("f_q_dot_heater_min");                      //[-] minimum allowable heater output as fraction of design

            p_electric_resistance = new C_csp_cr_electric_resistance(as_double("T_htf_cold_des"), as_double("T_htf_hot_des"),
                q_dot_heater_des, heater_efficiency, f_heater_min,
                f_q_dot_des_allowable_su, hrs_startup_at_max_rate,
                as_integer("rec_htf"), as_matrix("field_fl_props"), C_csp_cr_electric_resistance::E_elec_resist_startup_mode::INSTANTANEOUS_NO_MAX_ELEC_IN);

            p_electric_resistance->mc_reported_outputs.assign(C_csp_cr_electric_resistance::E_W_DOT_HEATER, allocate("W_dot_heater", n_steps_fixed), n_steps_fixed);
            p_electric_resistance->mc_reported_outputs.assign(C_csp_cr_electric_resistance::E_Q_DOT_HTF, allocate("q_dot_heater_to_htf", n_steps_fixed), n_steps_fixed);
            p_electric_resistance->mc_reported_outputs.assign(C_csp_cr_electric_resistance::E_Q_DOT_STARTUP, allocate("q_dot_heater_startup", n_steps_fixed), n_steps_fixed);
            p_electric_resistance->mc_reported_outputs.assign(C_csp_cr_electric_resistance::E_M_DOT_HTF, allocate("m_dot_htf_heater", n_steps_fixed), n_steps_fixed);
            p_electric_resistance->mc_reported_outputs.assign(C_csp_cr_electric_resistance::E_T_HTF_IN, allocate("T_htf_heater_in", n_steps_fixed), n_steps_fixed);
            p_electric_resistance->mc_reported_outputs.assign(C_csp_cr_electric_resistance::E_T_HTF_OUT, allocate("T_htf_heater_out", n_steps_fixed), n_steps_fixed);
        }
        p_heater = p_electric_resistance;

        // *****************************************************
        // Thermal energy storage
        C_csp_two_tank_tes storage(
            as_integer("rec_htf"),
            as_matrix("field_fl_props"),
            as_integer("rec_htf"),
            as_matrix("field_fl_props"),
            as_double("P_ref") / as_double("design_eff"),   //[MWt]
            as_double("solarm"),                            //[-]
            as_double("P_ref") / as_double("design_eff") * as_double("tshours"),
            as_double("h_tank"),
            as_double("u_tank"),
            as_integer("tank_pairs"),
            as_double("hot_tank_Thtr"),
            as_double("hot_tank_max_heat"),
            as_double("cold_tank_Thtr"),
            as_double("cold_tank_max_heat"),
            0.0,                                    // MSPT assumes direct storage, so no user input here: hard-code = 0.0
            as_double("T_htf_cold_des"),
            as_double("T_htf_hot_des"),
            as_double("T_htf_hot_des"),
            as_double("T_htf_cold_des"),
            as_double("h_tank_min"),
            as_double("tes_init_hot_htf_percent"),
            0.0,
            false,      //[-]
            as_double("packed_vol_frac"),
            1.85,                                   //[m/s]
            false                                   // for now, to get 'tanks_in_parallel' to work
        );
        
        // Set storage outputs
        storage.mc_reported_outputs.assign(C_csp_two_tank_tes::E_Q_DOT_LOSS, allocate("tank_losses", n_steps_fixed), n_steps_fixed);
        storage.mc_reported_outputs.assign(C_csp_two_tank_tes::E_W_DOT_HEATER, allocate("q_heater", n_steps_fixed), n_steps_fixed);
        storage.mc_reported_outputs.assign(C_csp_two_tank_tes::E_TES_T_HOT, allocate("T_tes_hot", n_steps_fixed), n_steps_fixed);
        storage.mc_reported_outputs.assign(C_csp_two_tank_tes::E_TES_T_COLD, allocate("T_tes_cold", n_steps_fixed), n_steps_fixed);
        storage.mc_reported_outputs.assign(C_csp_two_tank_tes::E_MASS_COLD_TANK, allocate("mass_tes_cold", n_steps_fixed), n_steps_fixed);
        storage.mc_reported_outputs.assign(C_csp_two_tank_tes::E_MASS_HOT_TANK, allocate("mass_tes_hot", n_steps_fixed), n_steps_fixed);

        // *****************************************************
        // Schedules

        // Off-taker schedule
        C_timeseries_schedule_inputs offtaker_schedule;
        bool is_timestep_load_fractions = false;
        if (is_assigned("is_timestep_load_fractions")) {
            is_timestep_load_fractions = as_boolean("is_timestep_load_fractions");
        }
        if (is_timestep_load_fractions) {
            auto vec = as_vector_double("timestep_load_fractions");
            C_timeseries_schedule_inputs offtaker_series = C_timeseries_schedule_inputs(vec, std::numeric_limits<double>::quiet_NaN());
            offtaker_schedule = offtaker_series;
        }
        else {      // Block schedules
            C_timeseries_schedule_inputs offtaker_block = C_timeseries_schedule_inputs(as_matrix("weekday_schedule"), as_matrix("weekend_schedule"),
                as_vector_double("f_turb_tou_periods"), std::numeric_limits<double>::quiet_NaN());
            offtaker_schedule = offtaker_block;
        }

        // Electricity pricing schedule
        C_timeseries_schedule_inputs elec_pricing_schedule;

        int csp_financial_model = as_integer("csp_financial_model");
        if (sim_type == 1) {
            if (csp_financial_model > 0 && csp_financial_model < 5) {   // Single Owner financial models

                double ppa_price_year1 = std::numeric_limits<double>::quiet_NaN();

                // Get first year base ppa price
                bool is_ppa_price_input_assigned = is_assigned("ppa_price_input");
                if (is_dispatch && !is_ppa_price_input_assigned) {
                    throw exec_error("csp_tower_particle", "\n\nYou selected dispatch optimization which requires that the array input ppa_price_input is defined\n");
                }

                if (is_ppa_price_input_assigned) {
                    size_t count_ppa_price_input;
                    ssc_number_t* ppa_price_input_array = as_array("ppa_price_input", &count_ppa_price_input);
                    ppa_price_year1 = (double)ppa_price_input_array[0];  // [$/kWh]
                }
                else {
                    ppa_price_year1 = 1.0;      //[-] don't need ppa multiplier if not optimizing
                }

                int ppa_soln_mode = as_integer("ppa_soln_mode");    // PPA solution mode (0=Specify IRR target, 1=Specify PPA price)
                if (ppa_soln_mode == 0 && is_dispatch) {
                    throw exec_error("csp_tower_particle", "\n\nYou selected dispatch optimization and the Specify IRR Target financial solution mode, "
                        "but dispatch optimization requires known absolute electricity prices. Dispatch optimization requires "
                        "the Specify PPA Price financial solution mode. You can continue using dispatch optimization and iteratively "
                        "solve for the PPA that results in a target IRR by running a SAM Parametric analysis or script.\n");
                }

                int en_electricity_rates = as_integer("en_electricity_rates");  // 0 = Use PPA, 1 = Use Retail
                if (en_electricity_rates == 1 && is_dispatch) {
                    throw exec_error("csp_tower_particle", "\n\nYou selected dispatch optimization and the option to Use Retail Electricity Rates on the Electricity Purchases page, "
                        "but the dispatch optimization model currently does not accept separate buy and sell prices. Please use the Use PPA or Market Prices option "
                        "on the Electricity Purchases page.\n");
                }

                // Time-of-Delivery factors by time step:
                int ppa_mult_model = as_integer("ppa_multiplier_model");
                if (ppa_mult_model == 1)        // use dispatch_ts input
                {
                    if (is_assigned("dispatch_factors_ts") || is_dispatch) {
                        auto vec = as_vector_double("dispatch_factors_ts");
                        elec_pricing_schedule = C_timeseries_schedule_inputs(vec, ppa_price_year1);
                    }
                    else { // if no dispatch optimization, don't need an input pricing schedule
                        elec_pricing_schedule = C_timeseries_schedule_inputs(1.0, std::numeric_limits<double>::quiet_NaN());
                    }
                }
                else if (ppa_mult_model == 0) // standard diurnal input
                {
                    // Most likely use case is to use schedules and TOD. So assume if at least one is provided, then user intended to use this approach
                    // the 'else' option applies non-feasible electricity prices, so we want to guard against selecting that it appears users
                    // are trying to use the schedules. 
                    bool is_one_assigned = is_assigned("dispatch_sched_weekday") || is_assigned("dispatch_sched_weekend") || is_assigned("dispatch_tod_factors");

                    if (is_one_assigned || is_dispatch) {

                        elec_pricing_schedule = C_timeseries_schedule_inputs(as_matrix("dispatch_sched_weekday"), as_matrix("dispatch_sched_weekend"),
                            as_vector_double("dispatch_tod_factors"), ppa_price_year1);
                    }
                    else {
                        // If electricity pricing data is not available, then dispatch to a uniform schedule
                        elec_pricing_schedule = C_timeseries_schedule_inputs(1.0, std::numeric_limits<double>::quiet_NaN());
                    }
                }
            }
            else if (csp_financial_model == 6) {     // use 'mp_energy_market_revenue' -> from Merchant Plant model

                if (is_dispatch) {
                    util::matrix_t<double> mp_energy_market_revenue = as_matrix("mp_energy_market_revenue"); // col 0 = cleared capacity, col 1 = $/MWh
                    size_t n_rows = mp_energy_market_revenue.nrows();
                    if (n_rows < n_steps_fixed) {
                        string ppa_msg = util::format("mp_energy_market_revenue input has %d rows but there are %d number of timesteps", n_rows, n_steps_fixed);
                        throw exec_error("csp_tower_particle", ppa_msg);
                    }

                    double conv_dolmwh_to_centkwh = 0.1;

                    std::vector<double> prices;
                    prices.resize(n_steps_fixed);
                    for (size_t ii = 0; ii < n_steps_fixed; ii++) {
                        prices[ii] = mp_energy_market_revenue(ii, 1) * conv_dolmwh_to_centkwh;
                    }

                    // prices is already dimensional for mp_energy_market_revenue, so use multiplier of 1
                    elec_pricing_schedule = C_timeseries_schedule_inputs(prices, 1.0);
                }
                else { // if no dispatch optimization, don't need an input pricing schedule
                    elec_pricing_schedule = C_timeseries_schedule_inputs(1.0, std::numeric_limits<double>::quiet_NaN());
                }
            }
            else if (csp_financial_model == 7 || csp_financial_model == 8) {    // LCOE (7) and None (8)
                if (is_dispatch) {
                    throw exec_error("csp_tower_particle", "Can't select dispatch optimization if No Financial model");
                }
                else { // if no dispatch optimization, don't need an input pricing schedule
                    // If electricity pricing data is not available, then dispatch to a uniform schedule
                    elec_pricing_schedule = C_timeseries_schedule_inputs(1.0, std::numeric_limits<double>::quiet_NaN());
                }
            }
            else {
                throw exec_error("csp_tower_particle", "csp_financial_model must be 1, 2, 3, 4, or 6");
            }
        }
        else if (sim_type == 2) {

            elec_pricing_schedule = C_timeseries_schedule_inputs(1.0, std::numeric_limits<double>::quiet_NaN());
        }
        // *****************************************************
        //

        C_csp_tou::C_dispatch_model_type::E_dispatch_model_type dispatch_model_type = C_csp_tou::C_dispatch_model_type::E_dispatch_model_type::UNDEFINED;
        if (is_dispatch) {
            dispatch_model_type = C_csp_tou::C_dispatch_model_type::E_dispatch_model_type::DISPATCH_OPTIMIZATION;
        }
        else {
            dispatch_model_type = C_csp_tou::C_dispatch_model_type::E_dispatch_model_type::HEURISTIC;
        }

        bool is_offtaker_frac_also_max = as_boolean("is_tod_pc_target_also_pc_max");

        C_csp_tou tou(offtaker_schedule, elec_pricing_schedule, dispatch_model_type, is_offtaker_frac_also_max);

        // *****************************************************
        // System parameters
        C_csp_solver::S_csp_system_params system;
        system.m_pb_fixed_par = as_double("pb_fixed_par");
        system.m_bop_par = as_double("bop_par");
        system.m_bop_par_f = as_double("bop_par_f");
        system.m_bop_par_0 = as_double("bop_par_0");
        system.m_bop_par_1 = as_double("bop_par_1");
        system.m_bop_par_2 = as_double("bop_par_2");

        // *****************************************************
        // System dispatch
        csp_dispatch_opt dispatch;

        if (is_dispatch && sim_type == 1){

            double heater_startup_cost = 0.0;
            if (is_parallel_heater) {
                heater_startup_cost = as_double("disp_hsu_cost_rel") * q_dot_heater_des;    //[$/start]
            }

            dispatch.solver_params.set_user_inputs(as_boolean("is_dispatch"), steps_per_hour, as_integer("disp_frequency"), as_integer("disp_horizon"),
                as_integer("disp_max_iter"), as_double("disp_mip_gap"), as_double("disp_timeout"),
                as_integer("disp_spec_presolve"), as_integer("disp_spec_bb"), as_integer("disp_spec_scaling"), as_integer("disp_reporting"),
                as_boolean("is_write_ampl_dat"), as_boolean("is_ampl_engine"), as_string("ampl_data_dir"), as_string("ampl_exec_call"));

            double disp_csu_cost_calc = as_double("disp_csu_cost_rel")*W_dot_cycle_des; //[$/start]
            double disp_rsu_cost_calc = as_double("disp_rsu_cost_rel")*q_dot_rec_des;   //[$/start]
            dispatch.params.set_user_params(as_boolean("can_cycle_use_standby"), as_double("disp_time_weighting"),
                disp_rsu_cost_calc, heater_startup_cost, disp_csu_cost_calc, as_double("disp_pen_ramping"),
                as_double("disp_inventory_incentive"), 0.0, 0.0);
        }
        else {
            dispatch.solver_params.dispatch_optimize = false;
        }

        // *****************************************************
        // Instantiate Solver       
        C_csp_solver csp_solver(weather_reader, 
                        collector_receiver, 
                        *p_csp_power_cycle, 
                        storage, 
                        tou,
                        dispatch,
                        system,
                        p_heater,
                        nullptr,
                        ssc_cmod_update,
                        (void*)(this));


        // Set solver reporting outputs
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::TIME_FINAL, allocate("time_hr", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::ERR_M_DOT, allocate("m_dot_balance", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::ERR_Q_DOT, allocate("q_balance", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::N_OP_MODES, allocate("n_op_modes", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::OP_MODE_1, allocate("op_mode_1", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::OP_MODE_2, allocate("op_mode_2", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::OP_MODE_3, allocate("op_mode_3", n_steps_fixed), n_steps_fixed);


        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::TOU_PERIOD, allocate("tou_value", n_steps_fixed), n_steps_fixed);            
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::PRICING_MULT, allocate("pricing_mult", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::PC_Q_DOT_SB, allocate("q_dot_pc_sb", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::PC_Q_DOT_MIN, allocate("q_dot_pc_min", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::PC_Q_DOT_TARGET, allocate("q_dot_pc_target", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::PC_Q_DOT_MAX, allocate("q_dot_pc_max", n_steps_fixed), n_steps_fixed);
        
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::CTRL_IS_REC_SU, allocate("is_rec_su_allowed", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::CTRL_IS_PC_SU, allocate("is_pc_su_allowed", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::CTRL_IS_PC_SB, allocate("is_pc_sb_allowed", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::EST_Q_DOT_CR_SU, allocate("q_dot_est_cr_su", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::EST_Q_DOT_CR_ON, allocate("q_dot_est_cr_on", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::EST_Q_DOT_DC, allocate("q_dot_est_tes_dc", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::EST_Q_DOT_CH, allocate("q_dot_est_tes_ch", n_steps_fixed), n_steps_fixed);

        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::CTRL_IS_PAR_HTR_SU, allocate("is_PAR_HTR_allowed", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::PAR_HTR_Q_DOT_TARGET, allocate("q_dot_elec_to_PAR_HTR", n_steps_fixed), n_steps_fixed);
        
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::CTRL_OP_MODE_SEQ_A, allocate("operating_modes_a", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::CTRL_OP_MODE_SEQ_B, allocate("operating_modes_b", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::CTRL_OP_MODE_SEQ_C, allocate("operating_modes_c", n_steps_fixed), n_steps_fixed);

        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_REL_MIP_GAP, allocate("disp_rel_mip_gap", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_SOLVE_STATE, allocate("disp_solve_state", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_SUBOPT_FLAG, allocate("disp_subopt_flag", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_SOLVE_ITER, allocate("disp_solve_iter", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_SOLVE_OBJ, allocate("disp_objective", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_SOLVE_OBJ_RELAX, allocate("disp_obj_relax", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_QSF_EXPECT, allocate("disp_qsf_expected", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_QSFPROD_EXPECT, allocate("disp_qsfprod_expected", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_QSFSU_EXPECT, allocate("disp_qsfsu_expected", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_TES_EXPECT, allocate("disp_tes_expected", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_PCEFF_EXPECT, allocate("disp_pceff_expected", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_SFEFF_EXPECT, allocate("disp_thermeff_expected", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_QPBSU_EXPECT, allocate("disp_qpbsu_expected", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_WPB_EXPECT, allocate("disp_wpb_expected", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_REV_EXPECT, allocate("disp_rev_expected", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_PRES_NCONSTR, allocate("disp_presolve_nconstr", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_PRES_NVAR, allocate("disp_presolve_nvar", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_SOLVE_TIME, allocate("disp_solve_time", n_steps_fixed), n_steps_fixed);

        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::SOLZEN, allocate("solzen", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::SOLAZ, allocate("solaz", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::BEAM, allocate("beam", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::TDRY, allocate("tdry", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::TWET, allocate("twet", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::RH, allocate("RH", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::WSPD, allocate("wspd", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::WDIR, allocate("wdir", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::CR_DEFOCUS, allocate("defocus", n_steps_fixed), n_steps_fixed);

        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::TES_Q_DOT_DC, allocate("q_dc_tes", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::TES_Q_DOT_CH, allocate("q_ch_tes", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::TES_E_CH_STATE, allocate("e_ch_tes", n_steps_fixed), n_steps_fixed);
       
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::M_DOT_CR_TO_TES_HOT, allocate("m_dot_cr_to_tes_hot", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::M_DOT_TES_HOT_OUT, allocate("m_dot_tes_hot_out", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::M_DOT_PC_TO_TES_COLD, allocate("m_dot_pc_to_tes_cold", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::M_DOT_TES_COLD_OUT, allocate("m_dot_tes_cold_out", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::M_DOT_FIELD_TO_CYCLE, allocate("m_dot_field_to_cycle", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::M_DOT_CYCLE_TO_FIELD, allocate("m_dot_cycle_to_field", n_steps_fixed), n_steps_fixed);

        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::SYS_W_DOT_FIXED, allocate("P_fixed", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::SYS_W_DOT_BOP, allocate("P_plant_balance_tot", n_steps_fixed), n_steps_fixed);

        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::W_DOT_NET, allocate("P_out_net", n_steps_fixed), n_steps_fixed);

        update("Initialize MSPT model...", 0.0);

        int out_type = -1;
        std::string out_msg = "";
        try {
            // Initialize Solver
            csp_solver.init();
        }
        catch( C_csp_exception &csp_exception ) {
            // Report warning before exiting with error
            while( csp_solver.mc_csp_messages.get_message(&out_type, &out_msg) ) {
                log(out_msg, out_type);
            }
            throw exec_error("csp_tower_particle", csp_exception.m_error_message);
        }

        // If no exception, then report messages
        while (csp_solver.mc_csp_messages.get_message(&out_type, &out_msg)) {
            log(out_msg, out_type);
        }

        // *****************************************************
        // System design is complete, get design parameters from component models as necessary

            // *************************
            // Solar field
        assign("N_hel_tot_calc", std::accumulate(N_hel.begin(), N_hel.end(), 0.0));     // [-] Total number of Heliostats
        assign("refl_image_error", refl_image_error);           //[mrad]
        assign("heliostat_area", heliostat_area);               //[m2]
        assign("average_attenuation", average_attenuation);     //[%]
        assign("land_min_abs", (ssc_number_t)land_min_abs);     //[m]
        assign("land_max_abs", (ssc_number_t)land_max_abs);     //[m]
        assign("land_area_base_calc", (ssc_number_t)land_area_base);          //[acre]
        assign("total_land_area_calc", (ssc_number_t)total_land_area);        //[acre]

        ssc_number_t* p_N_hel_calc = allocate("N_hel_calc", num_recs);  //[-]
        ssc_number_t* p_A_sf = allocate("A_sf", num_recs);              //[m2]
        for (size_t i = 0; i < num_recs; i++) {
            p_N_hel_calc[i] = (ssc_number_t)N_hel[i];
            p_A_sf[i] = (ssc_number_t)A_sf[i];
        }

        size_t n_helio_pos_rows = helio_pos.nrows();
        ssc_number_t* p_helio_positions_calc = allocate("helio_positions_calc", n_helio_pos_rows, 2);
        for (size_t i = 0; i < n_helio_pos_rows; i++) {
            p_helio_positions_calc[i * 2] = (ssc_number_t)helio_pos(i, 0);       //[m] x
            p_helio_positions_calc[i * 2 + 1] = (ssc_number_t)helio_pos(i, 1);   //[m] y
            //p_helio_positions_calc[i * 3 + 2] = (ssc_number_t)helio_pos(i, 2);   //[-] receiver assignment -> requires update to COL_LABEL=XY_POSITION
        }

        double W_dot_col_tracking_des = collector_receiver.get_tracking_power();    //[MWe]
        assign("W_dot_col_tracking_des", W_dot_col_tracking_des);       //[MWe]

            // *************************
            // Tower and receiver
        assign("h_tower_calc", (ssc_number_t)THT);   //[m]
        ssc_number_t* rec_height_calc = allocate("rec_height_calc", num_recs);
        ssc_number_t* rec_width_calc = allocate("rec_width_calc", num_recs);
        ssc_number_t* rec_aspect = allocate("rec_aspect", num_recs);
        ssc_number_t* rec_azimuth_calc = allocate("rec_azimuth_calc", num_recs);
        for (size_t i = 0; i < num_recs; i++) {
            input_idx = duplicate_recs ? 0 : i;
            rec_height_calc[i] = rec_height[input_idx];                     //[m]
            rec_width_calc[i] = rec_width[input_idx];                       //[m]
            rec_aspect[i] = rec_height[input_idx] / rec_width[input_idx];   //[-]
            rec_azimuth_calc[i] = rec_azimuth[i];                           //[deg]
        }

        assign("A_rec_total", A_rec_aperture_total);            //[m2]
        assign("A_rec_curtain_total", A_rec_curtain_total);     //[m2]
        assign("q_dot_rec_des_total", q_dot_rec_des);           //[MWt]
        ssc_number_t* A_rec = allocate("A_rec", num_recs);
        ssc_number_t* A_rec_curtain_out = allocate("A_rec_curtain", num_recs);
        ssc_number_t* q_dot_per_rec = allocate("q_dot_des_per_rec", num_recs);         
        for (size_t i = 0; i < num_recs; i++) {
            A_rec[i] = A_rec_aperture.at(i);                 //[m2]
            A_rec_curtain_out[i] = A_rec_curtain.at(i);      //[m2]
            q_dot_per_rec[i] = q_dot_des_per_rec.at(i);      //[MWt]
        }

        double L_tower_piping = std::numeric_limits<double>::quiet_NaN();
        double od_tube_NA = std::numeric_limits<double>::quiet_NaN();
        receivers.at(0)->get_design_geometry(L_tower_piping, od_tube_NA); 
        assign("L_tower_piping_calc", L_tower_piping);      //[m]

        double eta_rec_thermal_des;     //[-]
        double W_dot_rec_lift_des;      //[MWe]
        double m_dot_htf_rec_des;       //[kg/s]
        double m_dot_htf_rec_max;       //[kg/s]
        double W_dot_rec_pump_tower_share_des; // Undefined for particle receiver
        double W_dot_rec_pump_rec_share_des;   // Undefined for particle receiver
        double rec_pump_coef_des;              // Undefined for particle receiver
        double rec_vel_htf_des;                // Undefined for particle receiver
        double q_dot_piping_loss_des;

        double q_dot_inc;
        eta_rec_thermal_des = W_dot_rec_lift_des = q_dot_piping_loss_des = m_dot_htf_rec_des = m_dot_htf_rec_max = q_dot_inc = 0.0;

        std::vector<double> eta_rec_thermal_des_per_rec(num_recs);
        std::vector<double> W_dot_rec_lift_des_per_rec(num_recs);
        std::vector<double> q_dot_piping_loss_des_per_rec(num_recs);
        std::vector<double> m_dot_htf_rec_des_per_rec(num_recs);
        std::vector<double> m_dot_htf_rec_max_per_rec(num_recs);
        std::vector<double> q_dot_inc_per_rec(num_recs);

        for (size_t i = 0; i < num_recs; i++)   // loop through receivers
        {
            receivers.at(i)->get_design_performance(eta_rec_thermal_des_per_rec.at(i), W_dot_rec_lift_des_per_rec.at(i),
                W_dot_rec_pump_tower_share_des, W_dot_rec_pump_rec_share_des, rec_pump_coef_des, rec_vel_htf_des,
                m_dot_htf_rec_des_per_rec.at(i), m_dot_htf_rec_max_per_rec.at(i), q_dot_piping_loss_des_per_rec.at(i));

            q_dot_inc_per_rec.at(i) = receivers.at(i)->get_q_dot_rec_des() / eta_rec_thermal_des_per_rec.at(i);
            q_dot_inc += q_dot_inc_per_rec.at(i);

            eta_rec_thermal_des += eta_rec_thermal_des_per_rec.at(i) * q_dot_inc_per_rec.at(i);
            W_dot_rec_lift_des += W_dot_rec_lift_des_per_rec.at(i);
            q_dot_piping_loss_des += q_dot_piping_loss_des_per_rec.at(i);
            m_dot_htf_rec_des += m_dot_htf_rec_des_per_rec.at(i);
            m_dot_htf_rec_max += m_dot_htf_rec_max_per_rec.at(i);
        }
        eta_rec_thermal_des /= q_dot_inc;

        assign("eta_rec_thermal_des", eta_rec_thermal_des);     //[-]
        assign("P_tower_lift_des", W_dot_rec_lift_des);         //[MWe]
        assign("Q_transport_loss_des", q_dot_piping_loss_des);  //[MWt]
        assign("m_dot_htf_rec_des", m_dot_htf_rec_des);         //[kg/s]
        assign("m_dot_htf_rec_max", m_dot_htf_rec_max);         //[kg/s]

        ssc_number_t* eta_rec_thermal_des_per_rec_out = allocate("eta_rec_thermal_des_per_rec", num_recs);
        ssc_number_t* W_dot_rec_lift_des_per_rec_out = allocate("W_dot_rec_lift_des_per_rec", num_recs);
        ssc_number_t* q_dot_piping_loss_des_per_rec_out = allocate("q_dot_piping_loss_des_per_rec", num_recs);
        ssc_number_t* m_dot_htf_rec_des_per_rec_out = allocate("m_dot_htf_rec_des_per_rec", num_recs);
        ssc_number_t* m_dot_htf_rec_max_per_rec_out = allocate("m_dot_htf_rec_max_per_rec", num_recs);
        ssc_number_t* q_dot_inc_per_rec_out = allocate("q_dot_inc_per_rec", num_recs);
        for (size_t i = 0; i < num_recs; i++) {
            eta_rec_thermal_des_per_rec_out[i] = eta_rec_thermal_des_per_rec.at(i);      //[-]
            W_dot_rec_lift_des_per_rec_out[i] = W_dot_rec_lift_des_per_rec.at(i);        //[MWe]
            q_dot_piping_loss_des_per_rec_out[i] = q_dot_piping_loss_des_per_rec.at(i);  //[MWt]
            m_dot_htf_rec_des_per_rec_out[i] = m_dot_htf_rec_des_per_rec.at(i);          //[kg/s]
            m_dot_htf_rec_max_per_rec_out[i] = m_dot_htf_rec_max_per_rec.at(i);          //[kg/s]
            q_dot_inc_per_rec_out[i] = q_dot_inc_per_rec.at(i);                          //[MWt]
        }

            // *************************
            // Heater
        assign("q_dot_heater_des", q_dot_heater_des);       //[MWt]
        double W_dot_heater_des_calc = 0.0;                 //[MWe]
        double E_heater_su_des = 0.0;                       //[MWt-hr]
        if (is_parallel_heater) {
            p_electric_resistance->get_design_parameters(E_heater_su_des, W_dot_heater_des_calc);
        }
        assign("W_dot_heater_des", (ssc_number_t)W_dot_heater_des_calc);    //[MWe]
        assign("E_heater_su_des", (ssc_number_t)E_heater_su_des);           //[MWt-hr]

            // *************************
            // Thermal Energy Storage
        double V_tes_htf_avail_calc /*m3*/, V_tes_htf_total_calc /*m3*/,
            d_tank_calc /*m*/, q_dot_loss_tes_des_calc /*MWt*/, dens_store_htf_at_T_ave_calc /*kg/m3*/,
            Q_tes_des_calc /*MWt-hr*/, tes_total_mass /*kg*/;

        storage.get_design_parameters(V_tes_htf_avail_calc, V_tes_htf_total_calc,
            d_tank_calc, q_dot_loss_tes_des_calc, dens_store_htf_at_T_ave_calc, Q_tes_des_calc, tes_total_mass);

        assign("Q_tes_des", Q_tes_des_calc);                //[MWt-hr]
        assign("V_tes_htf_avail_des", V_tes_htf_avail_calc);    //[m3]
        assign("V_tes_htf_total_des", V_tes_htf_total_calc);    //[m3]
        assign("d_tank_tes", d_tank_calc);                      //[m]
        assign("q_dot_loss_tes_des", q_dot_loss_tes_des_calc);  //[MWt]
        assign("tshours_rec", Q_tes_des_calc / q_dot_rec_des);  //[hr]
        assign("dens_store_htf_at_T_ave", dens_store_htf_at_T_ave_calc); //[kg/m3]
        assign("m_tes_total_des", tes_total_mass);

        double tshours_heater = 0.0;
        if (q_dot_heater_des > 0.0) {
            tshours_heater = Q_tes_des_calc / q_dot_heater_des;     //[hr]
        }

        assign("tshours_heater", tshours_heater);

            // *************************
            // Power Cycle
        double m_dot_htf_pc_des;    //[kg/s]
        double cp_htf_pc_des;       //[kJ/kg-K]
        double W_dot_pc_lift_des;   //[MWe]
        double W_dot_pc_cooling_des;   //[MWe]
        int n_T_htf_pars, n_T_amb_pars, n_m_dot_pars;
        n_T_htf_pars = n_T_amb_pars = n_m_dot_pars = -1;
        double T_htf_ref_calc, T_htf_low_calc, T_htf_high_calc, T_amb_ref_calc, T_amb_low_calc, T_amb_high_calc,
            m_dot_htf_ND_ref_calc, m_dot_htf_ND_low_calc, m_dot_htf_ND_high_calc, W_dot_gross_ND_des, Q_dot_HTF_ND_des,
            W_dot_cooling_ND_des, m_dot_water_ND_des;
        T_htf_ref_calc = T_htf_low_calc = T_htf_high_calc =
            T_amb_ref_calc = T_amb_low_calc = T_amb_high_calc =
            m_dot_htf_ND_ref_calc = m_dot_htf_ND_low_calc = m_dot_htf_ND_high_calc =
            W_dot_gross_ND_des = Q_dot_HTF_ND_des = W_dot_cooling_ND_des = m_dot_water_ND_des = std::numeric_limits<double>::quiet_NaN();

        rankine_pc.get_design_parameters(m_dot_htf_pc_des, cp_htf_pc_des, W_dot_pc_lift_des, W_dot_pc_cooling_des,
                        n_T_htf_pars, n_T_amb_pars, n_m_dot_pars,
                        T_htf_ref_calc /*C*/, T_htf_low_calc /*C*/, T_htf_high_calc /*C*/,
                        T_amb_ref_calc /*C*/, T_amb_low_calc /*C*/, T_amb_high_calc /*C*/,
                        m_dot_htf_ND_ref_calc, m_dot_htf_ND_low_calc /*-*/, m_dot_htf_ND_high_calc /*-*/,
                        W_dot_gross_ND_des, Q_dot_HTF_ND_des, W_dot_cooling_ND_des, m_dot_water_ND_des);
        m_dot_htf_pc_des /= 3600.0;     // convert from kg/hr to kg/s
        assign("m_dot_htf_cycle_des", m_dot_htf_pc_des);
        assign("q_dot_cycle_des", q_dot_pc_des);
        assign("W_dot_cycle_lift_des", W_dot_pc_lift_des);          // Includes both PHX lift & cold tank lift
        assign("W_dot_cycle_cooling_des", W_dot_pc_cooling_des);
        assign("n_T_htf_pars_calc", n_T_htf_pars);
        assign("n_T_amb_pars_calc", n_T_amb_pars);
        assign("n_m_dot_pars_calc", n_m_dot_pars);
        assign("T_htf_ref_calc", T_htf_ref_calc);
        assign("T_htf_low_calc", T_htf_low_calc);
        assign("T_htf_high_calc", T_htf_high_calc);
        assign("T_amb_ref_calc", T_amb_ref_calc);
        assign("T_amb_low_calc", T_amb_low_calc);
        assign("T_amb_high_calc", T_amb_high_calc);
        assign("m_dot_htf_ND_ref_calc", m_dot_htf_ND_ref_calc);
        assign("m_dot_htf_ND_low_calc", m_dot_htf_ND_low_calc);
        assign("m_dot_htf_ND_high_calc", m_dot_htf_ND_high_calc);
        assign("W_dot_gross_ND_des_calc", W_dot_gross_ND_des);
        assign("Q_dot_HTF_ND_des_calc", Q_dot_HTF_ND_des);
        assign("W_dot_cooling_ND_des_calc", W_dot_cooling_ND_des);
        assign("m_dot_water_ND_des_calc", m_dot_water_ND_des);

            // *************************
            // System
        double W_dot_bop_design, W_dot_fixed_parasitic_design;    //[MWe]
        csp_solver.get_design_parameters(W_dot_bop_design, W_dot_fixed_parasitic_design);

                // Calculate net system *generation* capacity including HTF lifts and system parasitics
        double plant_net_capacity_calc = W_dot_cycle_des - W_dot_col_tracking_des - W_dot_rec_lift_des -
                                       W_dot_pc_lift_des - W_dot_pc_cooling_des - W_dot_bop_design - W_dot_fixed_parasitic_design;    //[MWe]

        double system_capacity = plant_net_capacity_calc * 1.E3;         //[kWe], convert from MWe

        assign("W_dot_bop_design", W_dot_bop_design);           //[MWe]
        assign("W_dot_fixed", W_dot_fixed_parasitic_design);    //[MWe]
        // Calculate system capacity instead of pass in
        assign("system_capacity", system_capacity);     //[kWe]
        assign("nameplate", system_capacity * 1.E-3);   //[MWe]
        assign("cp_system_nameplate", system_capacity * 1.E-3); //[MWe]
        assign("cp_battery_nameplate", 0.0);             //[MWe]

            // *****************************
            // ******* System Costs ********
        double tot_A_sf = std::accumulate(A_sf.begin(), A_sf.end(), 0.0);   // Total solar field area
        assign("A_sf_tot", tot_A_sf);

        double site_improvement_cost =
            N_mspt::site_improvement_cost(tot_A_sf, as_double("site_spec_cost"));
        assign("csp.pt.cost.site_improvements", (ssc_number_t)site_improvement_cost);

        double heliostat_cost =
            N_mspt::heliostat_cost(tot_A_sf, as_double("heliostat_spec_cost"), as_double("cost_sf_fixed"));
        assign("csp.pt.cost.heliostats", (ssc_number_t)heliostat_cost);

        // Handle curtain height and receiver z offset (if applicable)
        util::matrix_t<double> rec_tower_offset = as_matrix("rec_tower_offset");  // [-]
        double max_rec_height_offset = 0.0;
        for (size_t i = 0; i < num_recs; i++) {
            input_idx = duplicate_recs ? 0 : i;
            double half_ap_height = (norm_curtain_height[input_idx] - 0.5) * rec_height[input_idx];   // height of curtain above half aperture
            double rec_offset = 0.0;
            if (rec_tower_offset.ncols() == 3)
                rec_offset = rec_tower_offset.at(i, 2);
            max_rec_height_offset = max(max_rec_height_offset, half_ap_height + rec_offset);
        }

        // Calculating tower height based on MAX height of tower (i.e., top of curtain to ground)
        double h_helio = as_double("helio_height");     //[m] Heliostat height - for system costing
        double tower_cost =
            N_mspt::tower_cost(THT + max_rec_height_offset, 0.0, h_helio, as_double("tower_fixed_cost"), as_double("tower_exp"));
        //Repeated calc for UI
        double tower_total_height = THT + max_rec_height_offset + h_helio / 2.0;
        assign("h_rec_input_to_cost_model", (ssc_number_t)max_rec_height_offset);       //[m]
        assign("tower_total_height", (ssc_number_t)tower_total_height);                 //[m]
        assign("csp.pt.cost.tower", (ssc_number_t)tower_cost);

        double receiver_cost = 0.0;
        for (size_t i = 0; i<num_recs; i++)
            receiver_cost += N_mspt::receiver_cost(A_rec_aperture.at(i), as_double("rec_ref_cost"), as_double("rec_ref_area"), as_double("rec_cost_exp"));
        assign("csp.pt.cost.receiver", (ssc_number_t)receiver_cost);

        double rec_lift_height = THT + max_rec_height_offset + h_helio / 2.;
        double rec_lift_cost =
            N_mspt::lift_cost(m_dot_htf_rec_des, rec_lift_height, as_double("rec_lift_spec_cost"), 1.0);
        assign("receiver_lift_cost", (ssc_number_t)rec_lift_cost);

        // Two types of TES costs models
        double tes_cost, tes_medium_cost, tes_bin_cost, tes_lift_cost, phx_lift_cost;
        tes_cost = tes_medium_cost = tes_bin_cost = tes_lift_cost = phx_lift_cost = std::numeric_limits<double>::quiet_NaN();
        if (as_integer("tes_cost_model") == 0) {
            double Q_storage = as_double("P_ref") / as_double("design_eff") * as_double("tshours");
            tes_cost = N_mspt::tes_cost(Q_storage, as_double("tes_spec_cost")); //energy based method
            tes_medium_cost = tes_bin_cost = tes_lift_cost = phx_lift_cost = 0.0;
        }
        else if (as_integer("tes_cost_model") == 1) {
            tes_medium_cost =
                N_mspt::tes_medium_cost(tes_total_mass, as_double("tes_cost_per_mass"), 0.0); //inactive fraction is already accounted
            tes_bin_cost =
                N_mspt::tes_tank_cost(tes_total_mass, as_double("bin_cost_per_mass"), as_double("bin_cost_per_mass_exp"));
            tes_lift_cost =
                N_mspt::lift_cost(m_dot_htf_pc_des, as_double("h_tank"), as_double("tes_phx_lift_spec_cost"), 1.0);
            phx_lift_cost =
                N_mspt::lift_cost(m_dot_htf_pc_des, as_double("phx_height"), as_double("tes_phx_lift_spec_cost"), 1.0);
            tes_cost = tes_medium_cost + tes_bin_cost + tes_lift_cost + phx_lift_cost;
        }
        else {
            string msg = util::format("'tes_cost_model' must be either 0 (energy-based) or 1 (detailed).");
            throw exec_error("csp_tower_particle", msg);
        }

        assign("tes_medium_cost", (ssc_number_t)tes_medium_cost);
        assign("tes_bin_cost", (ssc_number_t)tes_bin_cost);
        assign("tes_lift_cost", (ssc_number_t)tes_lift_cost);
        assign("phx_lift_cost", (ssc_number_t)phx_lift_cost);
        assign("csp.pt.cost.storage", (ssc_number_t)tes_cost);

        double W_dot_design = as_double("P_ref");
        double power_cycle_cost =
            N_mspt::power_cycle_cost(W_dot_design, as_double("plant_spec_cost"));
        assign("csp.pt.cost.power_block", (ssc_number_t)power_cycle_cost);

        double bop_cost =
            N_mspt::bop_cost(W_dot_design, as_double("bop_spec_cost"));
        assign("csp.pt.cost.bop", (ssc_number_t)bop_cost);

        double fossil_backup_cost =
            N_mspt::fossil_backup_cost(W_dot_design, as_double("fossil_spec_cost"));
        assign("csp.pt.cost.fossil", (ssc_number_t)fossil_backup_cost);

        double heater_cost =
            N_mspt::heater_cost(q_dot_heater_des, heater_spec_cost);
        assign("heater_cost", (ssc_number_t)heater_cost);

        double direct_capital_precontingency_cost =
            site_improvement_cost + heliostat_cost + tower_cost + receiver_cost + rec_lift_cost +
            tes_cost + power_cycle_cost + heater_cost + bop_cost + fossil_backup_cost;
        assign("ui_direct_subtotal", (ssc_number_t)direct_capital_precontingency_cost);

        double contingency_cost =
            N_mspt::contingency_cost(as_double("contingency_rate"), direct_capital_precontingency_cost);
        assign("csp.pt.cost.contingency", (ssc_number_t)contingency_cost);

        double total_direct_cost =
            N_mspt::total_direct_cost(direct_capital_precontingency_cost, contingency_cost);
        assign("total_direct_cost", (ssc_number_t)total_direct_cost);

        double total_land_cost =
            N_mspt::total_land_cost(total_land_area, total_direct_cost, plant_net_capacity_calc,
                as_double("land_spec_cost"), as_double("csp.pt.cost.plm.percent"),
                as_double("csp.pt.cost.plm.per_watt"), as_double("csp.pt.cost.plm.fixed"));
        assign("csp.pt.cost.plm.total", (ssc_number_t)total_land_cost);
        assign("total_land_area", (ssc_number_t)total_land_area);

        double epc_and_owner_cost =
            N_mspt::epc_and_owner_cost(total_land_area, total_direct_cost, plant_net_capacity_calc,
                as_double("csp.pt.cost.epc.per_acre"), as_double("csp.pt.cost.epc.percent"),
                as_double("csp.pt.cost.epc.per_watt"), as_double("csp.pt.cost.epc.fixed"));
        assign("csp.pt.cost.epc.total", (ssc_number_t)epc_and_owner_cost);

        double sales_tax_cost =
            N_mspt::sales_tax_cost(total_direct_cost, as_double("sales_tax_frac"), as_double("sales_tax_rate"));
        assign("csp.pt.cost.sales_tax.total", (ssc_number_t)sales_tax_cost);

        double total_indirect_cost =
            N_mspt::total_indirect_cost(total_land_cost, epc_and_owner_cost, sales_tax_cost);
        assign("total_indirect_cost", (ssc_number_t)total_indirect_cost);

        // Financial model needs an updated total_installed_cost, remaining are for reporting only
        double total_installed_cost =
            N_mspt::total_installed_cost(total_direct_cost, total_indirect_cost);
        assign("total_installed_cost", (ssc_number_t)total_installed_cost);

        double estimated_installed_cost_per_cap =
            N_mspt::estimated_installed_cost_per_cap(total_installed_cost, plant_net_capacity_calc);
        assign("csp.pt.cost.installed_per_capacity", (ssc_number_t)estimated_installed_cost_per_cap);

        // Update construction financing costs, specifically, update: "construction_financing_cost"
        double const_per_interest_rate1 = as_double("const_per_interest_rate1");
        double const_per_interest_rate2 = as_double("const_per_interest_rate2");
        double const_per_interest_rate3 = as_double("const_per_interest_rate3");
        double const_per_interest_rate4 = as_double("const_per_interest_rate4");
        double const_per_interest_rate5 = as_double("const_per_interest_rate5");
        double const_per_months1 = as_double("const_per_months1");
        double const_per_months2 = as_double("const_per_months2");
        double const_per_months3 = as_double("const_per_months3");
        double const_per_months4 = as_double("const_per_months4");
        double const_per_months5 = as_double("const_per_months5");
        double const_per_percent1 = as_double("const_per_percent1");
        double const_per_percent2 = as_double("const_per_percent2");
        double const_per_percent3 = as_double("const_per_percent3");
        double const_per_percent4 = as_double("const_per_percent4");
        double const_per_percent5 = as_double("const_per_percent5");
        double const_per_upfront_rate1 = as_double("const_per_upfront_rate1");
        double const_per_upfront_rate2 = as_double("const_per_upfront_rate2");
        double const_per_upfront_rate3 = as_double("const_per_upfront_rate3");
        double const_per_upfront_rate4 = as_double("const_per_upfront_rate4");
        double const_per_upfront_rate5 = as_double("const_per_upfront_rate5");

        double const_per_principal1, const_per_principal2, const_per_principal3, const_per_principal4, const_per_principal5;
        double const_per_interest1, const_per_interest2, const_per_interest3, const_per_interest4, const_per_interest5;
        double const_per_total1, const_per_total2, const_per_total3, const_per_total4, const_per_total5;
        double const_per_percent_total, const_per_principal_total, const_per_interest_total, construction_financing_cost;

        const_per_principal1 = const_per_principal2 = const_per_principal3 = const_per_principal4 = const_per_principal5 =
            const_per_interest1 = const_per_interest2 = const_per_interest3 = const_per_interest4 = const_per_interest5 =
            const_per_total1 = const_per_total2 = const_per_total3 = const_per_total4 = const_per_total5 =
            const_per_percent_total = const_per_principal_total = const_per_interest_total = construction_financing_cost =
            std::numeric_limits<double>::quiet_NaN();

        N_financial_parameters::construction_financing_total_cost(total_installed_cost,
            const_per_interest_rate1, const_per_interest_rate2, const_per_interest_rate3, const_per_interest_rate4, const_per_interest_rate5,
            const_per_months1, const_per_months2, const_per_months3, const_per_months4, const_per_months5,
            const_per_percent1, const_per_percent2, const_per_percent3, const_per_percent4, const_per_percent5,
            const_per_upfront_rate1, const_per_upfront_rate2, const_per_upfront_rate3, const_per_upfront_rate4, const_per_upfront_rate5,
            const_per_principal1, const_per_principal2, const_per_principal3, const_per_principal4, const_per_principal5,
            const_per_interest1, const_per_interest2, const_per_interest3, const_per_interest4, const_per_interest5,
            const_per_total1, const_per_total2, const_per_total3, const_per_total4, const_per_total5,
            const_per_percent_total, const_per_principal_total, const_per_interest_total, construction_financing_cost);

        assign("const_per_principal1", (ssc_number_t)const_per_principal1);
        assign("const_per_principal2", (ssc_number_t)const_per_principal2);
        assign("const_per_principal3", (ssc_number_t)const_per_principal3);
        assign("const_per_principal4", (ssc_number_t)const_per_principal4);
        assign("const_per_principal5", (ssc_number_t)const_per_principal5);
        assign("const_per_interest1", (ssc_number_t)const_per_interest1);
        assign("const_per_interest2", (ssc_number_t)const_per_interest2);
        assign("const_per_interest3", (ssc_number_t)const_per_interest3);
        assign("const_per_interest4", (ssc_number_t)const_per_interest4);
        assign("const_per_interest5", (ssc_number_t)const_per_interest5);
        assign("const_per_total1", (ssc_number_t)const_per_total1);
        assign("const_per_total2", (ssc_number_t)const_per_total2);
        assign("const_per_total3", (ssc_number_t)const_per_total3);
        assign("const_per_total4", (ssc_number_t)const_per_total4);
        assign("const_per_total5", (ssc_number_t)const_per_total5);
        assign("const_per_percent_total", (ssc_number_t)const_per_percent_total);
        assign("const_per_principal_total", (ssc_number_t)const_per_principal_total);
        assign("const_per_interest_total", (ssc_number_t)const_per_interest_total);
        assign("construction_financing_cost", (ssc_number_t)construction_financing_cost);


        // *****************************************************
        // If calling cmod to run design only, return here
        if (as_integer("sim_type") != 1) {
            return;
        }
        // *****************************************************
        // *****************************************************

        update("Begin timeseries simulation...", 0.0);

        try
        {
            // Simulate !
            csp_solver.Ssimulate(sim_setup);
        }
        catch(C_csp_exception &csp_exception)
        {
            // Report warning before exiting with error
            while( csp_solver.mc_csp_messages.get_message(&out_type, &out_msg) )
            {
                log(out_msg);
            }

            throw exec_error("csp_tower_particle", csp_exception.m_error_message);
        }

        // If no exception, then report messages
        while (csp_solver.mc_csp_messages.get_message(&out_type, &out_msg))
        {
            log(out_msg, out_type);
        }

        // Do unit post-processing here
        double *p_q_pc_startup = allocate("q_pc_startup", n_steps_fixed);
        size_t count_pc_su = 0;
        ssc_number_t *p_q_dot_pc_startup = as_array("q_dot_pc_startup", &count_pc_su);
        if( count_pc_su != n_steps_fixed )
        {
            log("q_dot_pc_startup array is a different length than 'n_steps_fixed'.", SSC_WARNING);
            return;
        }
        for( size_t i = 0; i < n_steps_fixed; i++ )
        {
            p_q_pc_startup[i] = (float)(p_q_dot_pc_startup[i] * (sim_setup.m_report_step / 3600.0));    //[MWh]
        }

        // Convert mass flow rates from [kg/hr] to [kg/s]
        size_t count_m_dot_pc, count_m_dot_rec, count_m_dot_water_pc; 
        count_m_dot_pc = count_m_dot_rec = count_m_dot_water_pc = 0;  
        ssc_number_t *p_m_dot_rec = as_array("m_dot_rec", &count_m_dot_rec);
        ssc_number_t *p_m_dot_pc = as_array("m_dot_pc", &count_m_dot_pc);
        ssc_number_t *p_m_dot_water_pc = as_array("m_dot_water_pc", &count_m_dot_water_pc);
        if (count_m_dot_rec != n_steps_fixed || count_m_dot_pc != n_steps_fixed || count_m_dot_water_pc != n_steps_fixed)
        {
            log("At least one m_dot array is a different length than 'n_steps_fixed'.", SSC_WARNING);
            return;
        }
        for (size_t i = 0; i < n_steps_fixed; i++) {
            p_m_dot_rec[i] = (ssc_number_t)(p_m_dot_rec[i] / 3600.0);   //[kg/s] convert from kg/hr
            p_m_dot_pc[i] = (ssc_number_t)(p_m_dot_pc[i] / 3600.0);     //[kg/s] convert from kg/hr
            p_m_dot_water_pc[i] = (ssc_number_t)(p_m_dot_water_pc[i] / 3600.0); //[kg/s] convert from kg/hr
        }


        // Set output data from heliostat class  
        size_t n_rows_eta_map = heliostatfields.at(0)->ms_params.m_eta_map.nrows();                     // Number of solar positions (same for all receivers)
        size_t n_rows_flux_maps = heliostatfields.at(0)->ms_params.m_flux_maps.nrows();                 // solar positions * number of y flux points
        size_t n_cols_flux_maps = heliostatfields.at(0)->ms_params.m_flux_maps.ncols() * num_recs + 2;  // 2 + number of x flux points * number of receivers
        size_t rec_n_flux_x = heliostatfields.at(0)->ms_params.m_n_flux_x;
        size_t rec_n_flux_y = heliostatfields.at(0)->ms_params.m_n_flux_y;

        ssc_number_t* eta_map_out = allocate("eta_map_out", n_rows_eta_map, 2+num_recs);
        ssc_number_t* flux_maps_out = allocate("flux_maps_out", n_rows_flux_maps, n_cols_flux_maps);
        ssc_number_t* flux_maps_for_import = allocate("flux_maps_for_import", n_rows_flux_maps, n_cols_flux_maps);

        if (n_rows_eta_map * rec_n_flux_y != n_rows_flux_maps) {
            log("The number of rows in the field efficiency and receiver flux map matrices are not equal. This is unexpected, and the flux maps may be inaccurate.");
        }

        double flux_scaling_mult;
        size_t idx;
        size_t ncol = 2 + num_recs;
        for (size_t i = 0; i < n_rows_eta_map; i++)  // for each solar position
        {
            for (size_t rec = 0; rec < num_recs; rec++)
            {
                eta_map_out[ncol * i] = (ssc_number_t)heliostatfields.at(rec)->ms_params.m_eta_map(i, 0);               //[deg] Solar azimuth angle
                eta_map_out[ncol * i + 1] = (ssc_number_t)heliostatfields.at(rec)->ms_params.m_eta_map(i, 1);           //[deg] Solar zenith angle
                eta_map_out[ncol * i + 2 + rec] = (ssc_number_t)heliostatfields.at(rec)->ms_params.m_eta_map(i, 2);     //[deg] Solar field optical efficiency
            }

            for (size_t j = 0; j < rec_n_flux_y; j++)  // for all y flux positions
            {
                idx = rec_n_flux_y * n_cols_flux_maps * i + n_cols_flux_maps * j;
                flux_maps_for_import[idx] = flux_maps_out[idx] = eta_map_out[ncol * i];             //[deg] Solar azimuth angle
                idx++;
                flux_maps_for_import[idx] = flux_maps_out[idx] = eta_map_out[ncol * i + 1];         //[deg] Solar zenith angle
                idx++;
                for (size_t rec = 0; rec < num_recs; rec++)
                {
                    flux_scaling_mult = as_double("dni_des") * heliostatfields.at(rec)->ms_params.m_A_sf / 1000.0 / (A_rec_curtain.at(rec) / double(rec_n_flux_x * rec_n_flux_y));
                    for (size_t k = 0; k < rec_n_flux_x; k++)
                    {
                        flux_maps_out[idx + rec * rec_n_flux_x + k] = (ssc_number_t)(heliostatfields.at(rec)->ms_params.m_flux_maps(rec_n_flux_y * i + j, k) * heliostatfields.at(rec)->ms_params.m_eta_map(i, 2) * flux_scaling_mult);      //[kW/m^2]
                        flux_maps_for_import[idx + rec * rec_n_flux_x + k] = (ssc_number_t)heliostatfields.at(rec)->ms_params.m_flux_maps(rec_n_flux_y * i + j, k);
                    }
                }
            }
        }


        size_t count;
        ssc_number_t *p_W_dot_net = as_array("P_out_net", &count);
        ssc_number_t *p_time_final_hr = as_array("time_hr", &count);

        // 'adjustment_factors' class stores factors in hourly array, so need to index as such
        adjustment_factors haf(this->get_var_table(), "adjust");
        if (!haf.setup(n_steps_full))
            throw exec_error("csp_tower_particle", "failed to setup adjustment factors: " + haf.error());

        ssc_number_t *p_gen = allocate("gen", count);
        ssc_number_t* p_gensales_after_avail = allocate("gensales_after_avail", count);
        for( size_t i = 0; i < count; i++ )
        {
            size_t hour = (size_t)ceil(p_time_final_hr[i]);
            p_gen[i] = (ssc_number_t)(p_W_dot_net[i] * 1.E3 * haf(hour));           //[kWe]
            p_gensales_after_avail[i] = max(0.0, p_gen[i]);                         //[kWe]
        }
        ssc_number_t* p_annual_energy_dist_time = gen_heatmap(this, steps_per_hour);
        accumulate_annual_for_year("gen", "annual_energy", sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed/steps_per_hour);
        accumulate_annual_for_year("gensales_after_avail", "annual_sales_energy", sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour);
        
        accumulate_annual_for_year("P_cycle", "annual_W_cycle_gross", 1000.0*sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed/steps_per_hour);        //[kWe-hr]
        accumulate_annual_for_year("P_cooling_tower_tot", "annual_W_cooling_tower", 1000.0*sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour);        //[kWe-hr]

        accumulate_annual_for_year("q_dot_rec_inc", "annual_q_rec_inc", sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour);           //[MWt-hr]  // Incident power on particle curtain not including reflection (note that this is different than the definition of "incident" in the MSPT model)
        accumulate_annual_for_year("q_reflection_loss", "annual_q_rec_reflection_loss", sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour);
        accumulate_annual_for_year("q_thermal_loss", "annual_q_rec_thermal_loss", sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour);
        accumulate_annual_for_year("m_dot_rec", "annual_rec_mass_throughput", sim_setup.m_report_step, steps_per_hour, 1, n_steps_fixed / steps_per_hour);

        assign("annual_eta_rec", (ssc_number_t)(1.0 - (as_number("annual_q_rec_thermal_loss") + as_number("annual_q_rec_reflection_loss"))/ as_number("annual_q_rec_inc")));

        // Update fixed O&M based on receiver throughput
        double particles_lost_per_year = as_double("frac_rec_flow_lost") * as_number("annual_rec_mass_throughput");
        assign("particles_lost_per_year", (ssc_number_t)particles_lost_per_year);
        double om_particle_cost = as_double("tes_cost_per_mass") * particles_lost_per_year;
        assign("cost_particle_loss_year1", om_particle_cost);

        if (csp_financial_model < 6) {
            ssc_number_t* om_fixed = as_array("om_fixed", &count);

            for (size_t i = 0; i < count; i++) {
                om_fixed[i] += om_particle_cost;
            }
        }
        else if (csp_financial_model == 7) {
            double fixed_operating_cost_in = as_double("fixed_operating_cost");
            assign("fixed_operating_cost", fixed_operating_cost_in + om_particle_cost);
        }

        // 12.13.23 twn: Hardcode values that LCOE model want. Can remove these when/if LCOE model is straightened out
        assign("annual_electricity_consumption", 0.0);
        if (!is_assigned("electricity_rate")) {
            assign("electricity_rate", 0.0);
        }

        //Annual dispatch outputs
        accumulate_annual_for_year("disp_objective", "disp_objective_ann", sim_setup.m_report_step / 3600.0 / as_double("disp_frequency"), steps_per_hour, 1, n_steps_fixed/steps_per_hour);
        accumulate_annual_for_year("disp_solve_iter", "disp_iter_ann", sim_setup.m_report_step / 3600.0 / as_double("disp_frequency"), steps_per_hour, 1, n_steps_fixed/steps_per_hour);
        accumulate_annual_for_year("disp_presolve_nconstr", "disp_presolve_nconstr_ann", sim_setup.m_report_step / 3600.0/ as_double("disp_frequency"), steps_per_hour, 1, n_steps_fixed/steps_per_hour);
        accumulate_annual_for_year("disp_presolve_nvar", "disp_presolve_nvar_ann", sim_setup.m_report_step / 3600.0/ as_double("disp_frequency"), steps_per_hour, 1, n_steps_fixed/steps_per_hour);
        accumulate_annual_for_year("disp_solve_time", "disp_solve_time_ann", sim_setup.m_report_step / 3600.0 / as_double("disp_frequency"), steps_per_hour, 1, n_steps_fixed/steps_per_hour );
        accumulate_annual_for_year("disp_solve_state", "disp_solve_state_ann", sim_setup.m_report_step / 3600.0 / as_double("disp_frequency"), steps_per_hour, 1, n_steps_fixed / steps_per_hour);

        // Reporting dispatch solution counts
        size_t n_flag, n_gap = 0;
        ssc_number_t* subopt_flag = as_array("disp_subopt_flag", &n_flag);
        ssc_number_t* rel_mip_gap = as_array("disp_rel_mip_gap", &n_gap);

        std::vector<int> flag;
        std::vector<double> gap;
        flag.resize(n_flag);
        gap.resize(n_flag);
        for (size_t i = 0; i < n_flag; i++) {
            flag[i] = (int)subopt_flag[i];
            gap[i] = (double)rel_mip_gap[i];
        }

        double avg_gap = 0;
        if (as_boolean("is_dispatch")) {
            std::string disp_sum_msg;
            dispatch.count_solutions_by_type(flag, (int)as_double("disp_frequency"), disp_sum_msg);
            log(disp_sum_msg, SSC_NOTICE);
            avg_gap = dispatch.calc_avg_subopt_gap(gap, flag, (int)as_double("disp_frequency"));
        }
        assign("avg_suboptimal_rel_mip_gap", (ssc_number_t)avg_gap);

        // Calculated Outputs
            // First, sum power cycle water consumption timeseries outputs
        accumulate_annual_for_year("m_dot_water_pc", "annual_total_water_use", sim_setup.m_report_step / 1000.0, steps_per_hour, 1, n_steps_fixed/steps_per_hour); //[m^3], convert from kg
            // Then, add water usage from mirror cleaning
        ssc_number_t V_water_cycle = as_number("annual_total_water_use");
        double V_water_mirrors = as_double("water_usage_per_wash") / 1000.0 * tot_A_sf * as_double("washing_frequency");
        assign("annual_total_water_use", (ssc_number_t)(V_water_cycle + V_water_mirrors));

        ssc_number_t ae = as_number("annual_energy");           //[kWe-hr]
        ssc_number_t pg = as_number("annual_W_cycle_gross");    //[kWe-hr]
        ssc_number_t annual_sales_energy = as_number("annual_sales_energy");        //[kWe-hr]
        ssc_number_t convfactor = (pg != 0) ? 100 * ae / pg : (ssc_number_t)0.0;
        assign("conversion_factor", convfactor);

        double kWh_per_kW = 0.0;
        double kWh_sales_energy_per_kW_nameplate = 0.0;
        double nameplate = system_capacity;     //[kWe]
        if (nameplate > 0.0) {
            kWh_per_kW = ae / nameplate;
            kWh_sales_energy_per_kW_nameplate = annual_sales_energy / nameplate;
        }

        assign("capacity_factor", (ssc_number_t)(kWh_per_kW / ((double)n_steps_fixed / (double)steps_per_hour)*100.));
        assign("sales_energy_capacity_factor", (ssc_number_t)(kWh_sales_energy_per_kW_nameplate / ((double)n_steps_fixed / (double)steps_per_hour) * 100.));
        assign("kwh_per_kw", (ssc_number_t)kWh_per_kW);
         
        ssc_number_t* p_pricing_mult = as_array("pricing_mult", &count);

        std::vector<pair<size_t, double>> ppa_pairs;
        ppa_pairs.resize(count);
        for (size_t i = 0; i < count; i++) {
            ppa_pairs[i].first = i;
            ppa_pairs[i].second = p_pricing_mult[i];
        }

        //std::sort(ppa_pairs.begin(), ppa_pairs.end(), SortByPPAPrice);
        size_t n_ppa_steps = 1000;

        double total_energy_in_sub_period = 0.0;
        for (size_t i = 0; i < n_ppa_steps; i++) {
            size_t j = ppa_pairs[i].first;
            total_energy_in_sub_period += p_gen[j] * sim_setup.m_report_step / 3600.0;     //[kWe-hr]
        }

        double total_energy_nameplate = nameplate * n_ppa_steps * sim_setup.m_report_step / 3600.0;     //[kWe-hr]

        double cap_fac_highest_1000_ppas = 0.0;
        if (nameplate > 0.0) {
            cap_fac_highest_1000_ppas = total_energy_in_sub_period / total_energy_nameplate * 100.0;    //[%]        
        }

        assign("capacity_factor_highest_1000_ppas", cap_fac_highest_1000_ppas);

        n_ppa_steps = 2000;

        total_energy_in_sub_period = 0.0;
        for (size_t i = 0; i < n_ppa_steps; i++) {
            size_t j = ppa_pairs[i].first;
            total_energy_in_sub_period += p_gen[j] * sim_setup.m_report_step / 3600.0;     //[kWe-hr]
        }

        total_energy_nameplate = nameplate * n_ppa_steps * sim_setup.m_report_step / 3600.0;     //[kWe-hr]

        double cap_fac_highest_2000_ppas = 0.0;
        if (nameplate > 0.0) {
            cap_fac_highest_2000_ppas = total_energy_in_sub_period / total_energy_nameplate * 100.0;    //[%]
        }

        assign("capacity_factor_highest_2000_ppas", cap_fac_highest_2000_ppas);

        if (p_electric_resistance != NULL) {
            delete p_electric_resistance;
        }

        std::clock_t clock_end = std::clock();
        double sim_cpu_run_time = (clock_end - clock_start) / (double)CLOCKS_PER_SEC;		//[s]
        assign("sim_cpu_run_time", sim_cpu_run_time);   //[s]

    }
};

//bool SortByPPAPrice(const pair<int, double>& lhs,
//    const pair<int, double>& rhs)
//{
//    return lhs.second > rhs.second;
//}

DEFINE_MODULE_ENTRY(csp_tower_particle, "CSP free-falling particle tower with hierarchical controller and dispatch optimization", 1)
