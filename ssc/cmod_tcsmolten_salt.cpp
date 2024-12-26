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
#include "csp_solver_mspt_receiver_222.h"
#include "csp_solver_mspt_receiver.h"
#include "csp_solver_mspt_collector_receiver.h"
#include "csp_solver_pc_Rankine_indirect_224.h"
#include "csp_solver_two_tank_tes.h"

#include "csp_dispatch.h"

#include "csp_solver_cr_electric_resistance.h"
#include "csp_solver_cavity_receiver.h"

#include "csp_system_costs.h"

#include <ctime>
#include <cmath>
#include <limits>

static var_info _cm_vtab_tcsmolten_salt[] = {

    // VARTYPE       DATATYPE    NAME                                  LABEL                                                                                                                                      UNITS           META                                 GROUP                                       REQUIRED_IF                                                         CONSTRAINTS      UI_HINTS
    { SSC_INPUT,     SSC_STRING, "solar_resource_file",                "Local weather file path",                                                                                                                 "",             "",                                  "Solar Resource",                    "?",                                                                "LOCAL_FILE",    ""},
    { SSC_INPUT,     SSC_TABLE,  "solar_resource_data",                "Weather resource data in memory",                                                                                                         "",             "",                                  "Solar Resource",                    "?",                                                                "",              "SIMULATION_PARAMETER"},

    // Simulation parameters
    { SSC_INPUT,     SSC_NUMBER, "is_dispatch",                        "Allow dispatch optimization?",                                                                                                            "",             "",                                  "System Control",                           "?=0",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "sim_type",                           "1 (default): timeseries, 2: design only",                                                                                                 "",             "",                                  "System Control",                           "?=1",                                                              "",              "SIMULATION_PARAMETER"},
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

    // Optional system design parameters potentially useful for analysis
    { SSC_INPUT,     SSC_NUMBER, "is_calc_sm",                         "False (default): use input solarm, True: calc solar multiple to achieve q_dot_rec_des_target",                                            "",             "",                                  "Tower and Receiver",                       "?=0",                                                              "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_NUMBER, "q_dot_rec_des_target",               "Target design receiver thermal power",                                                                                                    "MWe",          "",                                  "Tower and Receiver",                       "is_calc_sm=1",                                                     "",              "SIMULATION_PARAMETER" },

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
    { SSC_INPUT,     SSC_NUMBER, "field_model_type",                   "0=design field and tower/receiver geometry, 1=design field, 2=user specified field, 3=user flux and eta map, pass heliostat_positions to SolarPILOT for layout, 4=user flux and eta maps, no SolarPILOT, input A_sf_in, total_land_area_before_rad_cooling_in, and N_hel", "", "", "Heliostat Field", "*",     "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "helio_width",                        "Heliostat width",                                                                                                                         "m",            "",                                  "Heliostat Field",                          "field_model_type<4",                                               "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "helio_height",                       "Heliostat height",                                                                                                                        "m",            "",                                  "Heliostat Field",                          "field_model_type<4",                                               "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "helio_optical_error_mrad",           "Heliostat optical error",                                                                                                                 "mrad",         "",                                  "Heliostat Field",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "helio_active_fraction",              "Heliostat active fraction",                                                                                                               "",             "",                                  "Heliostat Field",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "dens_mirror",                        "Ratio of heliostat reflective area to profile",                                                                                           "",             "",                                  "Heliostat Field",                          "field_model_type<4",                                               "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "helio_reflectance",                  "Heliostat reflectance",                                                                                                                   "",             "",                                  "Heliostat Field",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "rec_absorptance",                    "Receiver absorptance",                                                                                                                    "",             "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "rec_hl_perm2",                       "Receiver design heatloss",                                                                                                                "kW/m2",        "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "land_max",                           "Land max boundary",                                                                                                                       "-ORm",         "",                                  "Heliostat Field",                          "?=7.5",                                                            "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "land_min",                           "Land min boundary",                                                                                                                       "-ORm",         "",                                  "Heliostat Field",                          "?=0.75",                                                           "",              ""},
    { SSC_INPUT,     SSC_MATRIX, "land_bound_table",                   "Land boundary table",                                                                                                                     "m",            "",                                  "Heliostat Field",                          "?",                                                                "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_ARRAY,  "land_bound_list",                    "Land boundary table listing",                                                                                                             "",             "",                                  "Heliostat Field",                          "?",                                                                "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_NUMBER, "p_start",                            "Heliostat startup energy",                                                                                                                "kWe-hr",       "",                                  "Heliostat Field",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "p_track",                            "Heliostat tracking power",                                                                                                               "kWe",          "",                                  "Heliostat Field",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "hel_stow_deploy",                    "Stow/deploy elevation angle",                                                                                                             "deg",          "",                                  "Heliostat Field",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "v_wind_max",                         "Heliostat max wind velocity",                                                                                                             "m/s",          "",                                  "Heliostat Field",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "interp_nug",                         "Interpolation nugget",                                                                                                                    "-",            "",                                  "Heliostat Field",                          "?=0",                                                              "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_NUMBER, "interp_beta",                        "Interpolation beta coef.",                                                                                                                "-",            "",                                  "Heliostat Field",                          "?=1.99",                                                           "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_MATRIX, "helio_aim_points",                   "Heliostat aim point table",                                                                                                               "m",            "",                                  "Heliostat Field",                          "?",                                                                "",              "SIMULATION_PARAMETER"},
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
    { SSC_INPUT,     SSC_NUMBER, "n_flux_days",                        "Number of days in flux map lookup",                                                                                                       "",             "",                                  "Tower and Receiver",                       "?=8",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "delta_flux_hrs",                     "Hourly frequency in flux map lookup",                                                                                                     "",             "",                                  "Tower and Receiver",                       "?=1",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "water_usage_per_wash",               "Water usage per wash",                                                                                                                    "L/m2_aper",    "",                                  "Heliostat Field",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "washing_frequency",                  "Mirror washing frequency",                                                                                                                "none",         "",                                  "Heliostat Field",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "check_max_flux",                     "Check max flux at design point",                                                                                                          "",             "",                                  "Heliostat Field",                          "?=0",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "sun_loc_des",                        "Sun location at design point (0 = Summer solstice, 1 = Equinox, 2 = Winter solstice)",                                                    "",             "",                                  "Heliostat Field",                          "?=0",                                                              "",              ""},

    // Inputs required for user defined SF performance when field_model_type = 4
    // Values can be defined by mapping to equivalent _calc output for simulation results with field_model_type < 3
    { SSC_INPUT,     SSC_NUMBER, "A_sf_in",                            "Solar field area",                                                                                                                        "m^2",          "",                                  "Heliostat Field",                          "field_model_type>3",                                               "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_NUMBER, "total_land_area_before_rad_cooling_in", "Total land area not including radiative cooling - in",                                                                                 "acre",         "",                                  "Heliostat Field",                          "field_model_type>3",                                               "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_NUMBER, "N_hel",                              "Number of heliostats - in",                                                                                                               "",             "",                                  "Heliostat Field",                          "field_model_type>3",                                               "",              "SIMULATION_PARAMETER"},

    { SSC_INPUT,     SSC_NUMBER, "flux_max",                           "Maximum allowable flux",                                                                                                                  "",             "",                                  "Tower and Receiver",                       "?=1000",                                                           "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "opt_init_step",                      "Optimization initial step size",                                                                                                          "",             "",                                  "Heliostat Field",                          "?=0.05",                                                           "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "opt_max_iter",                       "Max number iteration steps",                                                                                                              "",             "",                                  "Heliostat Field",                          "?=200",                                                            "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "opt_conv_tol",                       "Optimization convergence tolerance",                                                                                                      "",             "",                                  "Heliostat Field",                          "?=0.001",                                                          "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "opt_flux_penalty",                   "Optimization flux overage penalty",                                                                                                       "",             "",                                  "Heliostat Field",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "opt_algorithm",                      "Optimization algorithm",                                                                                                                  "",             "",                                  "Heliostat Field",                          "?=1",                                                              "",              ""},

    
    // Receiver (type 222) parameters
    { SSC_INPUT,     SSC_NUMBER, "receiver_type",                      "0: external (default), 1; cavity",                                                                                                        "",             "",                                  "Heliostat Field",                          "?=0",                                                              "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "N_panels",                           "Number of individual panels on the receiver",                                                                                             "",             "",                                  "Tower and Receiver",                       "*",                                                                "INTEGER",       ""},
    { SSC_INPUT,     SSC_NUMBER, "d_tube_out",                         "The outer diameter of an individual receiver tube",                                                                                       "mm",           "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "th_tube",                            "The wall thickness of a single receiver tube",                                                                                            "mm",           "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "mat_tube",                           "Receiver tube material, 2=Stainless AISI316",                                                                                             "",             "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "rec_htf",                            "Receiver HTF, 17=Salt (60% NaNO3, 40% KNO3) 10=Salt (46.5% LiF 11.5% NaF 42% KF) 50=Lookup tables",                                       "",             "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_MATRIX, "field_fl_props",                     "User defined field fluid property data",                                                                                                  "-",            "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "Flow_type",                          "Receiver flow pattern: see figure on SAM Receiver page",                                                                                  "",             "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "crossover_shift",                    "Number of panels shift in receiver crossover position",                                                                                   "",             "",                                  "Tower and Receiver",                       "?=0",                                                              "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_NUMBER, "epsilon",                            "The emissivity of the receiver surface coating",                                                                                          "",             "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "hl_ffact",                           "The heat loss factor (thermal loss fudge factor)",                                                                                        "",             "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "f_rec_min",                          "Minimum receiver mass flow rate turn down fraction",                                                                                      "",             "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "rec_su_delay",                       "Fixed startup delay time for the receiver",                                                                                               "hr",           "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "rec_qf_delay",                       "Energy-based receiver startup delay (fraction of rated thermal power)",                                                                   "",             "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "csp.pt.rec.max_oper_frac",           "Maximum receiver mass flow rate fraction",                                                                                                "",             "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "eta_pump",                           "Receiver HTF pump efficiency",                                                                                                            "",             "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "piping_length_mult",                 "Piping length multiplier",                                                                                                                "",             "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "piping_length_const",                "Piping constant length",                                                                                                                  "m",            "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},

        // Cavity inputs that should *not* be reset during call to this cmod
    { SSC_INPUT,     SSC_NUMBER, "n_cav_rec_panels",                   "Cavity receiver number of panels",                                                                                                        "",             "",                                  "Tower and Receiver",                       "receiver_type=1",                                                  "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "cav_rec_span",                       "Cavity receiver span angle",                                                                                                              "deg",          "",                                  "Tower and Receiver",                       "receiver_type=1",                                                  "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "cav_rec_passive_abs",                "Cavity receiver passive surface solar absorptance",                                                                                       "",             "",                                  "Tower and Receiver",                       "receiver_type=1",                                                  "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "cav_rec_passive_eps",                "Cavity receiver passive surface thermal emissivity",                                                                                      "",             "",                                  "Tower and Receiver",                       "receiver_type=1",                                                  "",              "" },


    // New variables replacing deprecated variable "piping_loss". Variable currently not required so exec() can check if assigned and throw a more detailed error
    { SSC_INPUT,     SSC_NUMBER, "piping_loss_coefficient",            "Thermal loss per meter of piping",                                                                                                        "Wt/m2-K",      "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},

	{ SSC_INPUT,     SSC_NUMBER, "rec_clearsky_model",				   "Clearsky model: None = -1, User-defined data = 0, Meinel = 1; Hottel = 2; Allen = 3; Moon = 4",											  "",             "",                                  "Tower and Receiver",                       "?=-1",															   "",              "SIMULATION_PARAMETER"},
	{ SSC_INPUT,     SSC_ARRAY,  "rec_clearsky_dni",					"User-defined clear-sky DNI",																											  "W/m2",         "",                                  "Tower and Receiver",                       "rec_clearsky_model=0",											   "",              "SIMULATION_PARAMETER"},
	{ SSC_INPUT,     SSC_NUMBER, "rec_clearsky_fraction",               "Weighting fraction on clear-sky DNI for receiver flow control",                                                                          "",             "",                                  "Tower and Receiver",                       "?=0.0",                                                            "",              "SIMULATION_PARAMETER"},

    // Optional receiver design parameters potentially useful for analysis
    { SSC_INPUT,     SSC_NUMBER, "is_calc_od_tube",                     "False (default): use input d_tube_output, True: calc OD tube to achieve W_dot_rec_target",                                               "",             "",                                  "Tower and Receiver",                       "?=0",                                                              "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_NUMBER, "W_dot_rec_target",                    "Target pumping power loss through receiver (not including riser/downcomer)",                                                             "MWe",          "",                                  "Tower and Receiver",                       "is_calc_od_tube=1",                                                "",              "SIMULATION_PARAMETER" },

    // Transient receiver parameters
	{ SSC_INPUT,     SSC_NUMBER, "is_rec_model_trans",                 "Formulate receiver model as transient?",                                                                                                  "",             "",                                  "Tower and Receiver",                       "?=0",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "is_rec_startup_trans",               "Formulate receiver startup model as transient?",                                                                                          "",             "",                                  "Tower and Receiver",                       "?=0",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "rec_tm_mult",                        "Receiver thermal mass multiplier",                                                                                                        "",             "",                                  "Tower and Receiver",                       "?=1.0",                                                            "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "riser_tm_mult",                      "Riser thermal mass multiplier",                                                                                                           "",             "",                                  "Tower and Receiver",                       "?=1.0",                                                            "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "downc_tm_mult",                      "Downcomer thermal mass multiplier",                                                                                                       "",             "",                                  "Tower and Receiver",                       "?=1.0",                                                            "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "u_riser",                            "Design point HTF velocity in riser",                                                                                                      "m/s",          "",                                  "Tower and Receiver",                       "?=4.0",                                                            "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "th_riser",                           "Riser or downcomer tube wall thickness",                                                                                                  "mm",           "",                                  "Tower and Receiver",                       "?=15.0",                                                           "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "heat_trace_power",                   "Riser/downcomer heat trace power during startup",                                                                                         "kW/m",         "",                                  "Tower and Receiver",                       "?=500.0",                                                          "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "preheat_flux",                       "Tube absorbed solar flux during preheat",                                                                                                 "kW/m2",        "",                                  "Tower and Receiver",                       "?=50.0",                                                           "",              ""},
	{ SSC_INPUT,     SSC_NUMBER, "min_preheat_time",                   "Minimum time required in preheat startup stage",                                                                                          "hr",			  "",                                  "Tower and Receiver",                       "?=0.0",                                                            "",              "SIMULATION_PARAMETER"},
	{ SSC_INPUT,     SSC_NUMBER, "min_fill_time",                      "Startup time delay for filling the receiver/piping",                                                                                      "hr",			  "",                                  "Tower and Receiver",                       "?=0.1333",                                                         "",              "SIMULATION_PARAMETER"},	
	{ SSC_INPUT,     SSC_NUMBER, "startup_ramp_time",                  "Time required to reach full flux during receiver startup",                                                                                "hr",           "",                                  "Tower and Receiver",                       "?=0.1333",                                                         "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "startup_target_Tdiff",               "Target HTF T at end of startup - steady state hot HTF temperature",                                                                       "C",            "",                                  "Tower and Receiver",                       "?=-5.0",                                                           "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "is_rec_startup_from_T_soln",         "Begin receiver startup from solved temperature profiles?",                                                                                "",             "",                                  "Tower and Receiver",                       "?=0",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "is_rec_enforce_min_startup",         "Always enforce minimum startup time",                                                                                                     "",             "",                                  "Tower and Receiver",                       "?=1",                                                              "",              ""},

        
        // Field layout and tower/receiver dimensions
        // If field_model_type = 1, tower/receiver dimensions are used as guess values
        //        and optimized values are reported as _calc outputs
    { SSC_INPUT,     SSC_MATRIX, "helio_positions",                    "Heliostat position table - in",                                                                                                           "",             "",                                  "Heliostat Field",                          "field_model_type=2|field_model_type=3",                           "", "COL_LABEL=XY_POSITION" },
    { SSC_INPUT,     SSC_NUMBER, "rec_height",                         "Receiver height - in",                                                                                                                    "m",            "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "D_rec",                              "The overall outer diameter of the receiver - in",                                                                                         "m",            "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "h_tower",                            "Tower height - in",                                                                                                                       "m",            "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "cav_rec_height",                     "Cavity receiver height - in",                                                                                                             "m",            "",                                  "Tower and Receiver",                       "receiver_type=1",                                                  "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "cav_rec_width",                      "Cavity receiver aperture width - in",                                                                                                     "m",            "",                                  "Tower and Receiver",                       "receiver_type=1",                                                  "",              "" },

    // Parallel heater parameters
    { SSC_INPUT,     SSC_NUMBER, "heater_mult",                        "Heater multiple relative to design cycle thermal power",                                                                                  "-",            "",                                  "Parallel Heater",                          "is_parallel_htr=1",                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "heater_efficiency",                  "Heater electric to thermal efficiency",                                                                                                   "%",            "",                                  "Parallel Heater",                          "is_parallel_htr=1",                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "f_q_dot_des_allowable_su",           "Fraction of design power allowed during startup",                                                                                         "-",            "",                                  "Parallel Heater",                          "is_parallel_htr=1",                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "hrs_startup_at_max_rate",            "Duration of startup at max startup power",                                                                                                "hr",           "",                                  "Parallel Heater",                          "is_parallel_htr=1",                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "f_q_dot_heater_min",                 "Minimum allowable heater output as fraction of design",                                                                                   "",             "",                                  "Parallel Heater",                          "is_parallel_htr=1",                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "disp_hsu_cost_rel",                  "Heater startup cost",                                                                                                                     "$/MWt/start",  "",                                  "System Control",                           "is_dispatch=1&is_parallel_htr=1",                                  "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "heater_spec_cost",                   "Heater specific cost",                                                                                                                    "$/kWht",       "",                                  "System Costs",                             "is_parallel_htr=1",                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "allow_heater_no_dispatch_opt",       "Allow heater with no dispatch optimization? SAM UI relies on cmod default",                                                               "",             "",                                  "System Costs",                             "?=0",                                                              "",              "SIMULATION_PARAMETER" },


    // TES parameters - general
    { SSC_INPUT,     SSC_NUMBER, "tes_init_hot_htf_percent",           "Initial fraction of available volume that is hot",                                                                                        "%",            "",                                  "Thermal Storage",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "h_tank",                             "Total height of tank (height of HTF when tank is full)",                                                                                  "m",            "",                                  "Thermal Storage",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "cold_tank_max_heat",                 "Rated heater capacity for cold tank heating",                                                                                             "MW",           "",                                  "Thermal Storage",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "u_tank",                             "Loss coefficient from the tank",                                                                                                          "W/m2-K",       "",                                  "Thermal Storage",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "tank_pairs",                         "Number of equivalent tank pairs",                                                                                                         "",             "",                                  "Thermal Storage",                          "*",                                                                "INTEGER",       ""},
    { SSC_INPUT,     SSC_NUMBER, "cold_tank_Thtr",                     "Minimum allowable cold tank HTF temperature",                                                                                             "C",            "",                                  "Thermal Storage",                          "*",                                                                "",              ""},
    // TES parameters - 2 tank
    { SSC_INPUT,     SSC_NUMBER, "h_tank_min",                         "Minimum allowable HTF height in storage tank",                                                                                            "m",            "",                                  "Thermal Storage",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "hot_tank_Thtr",                      "Minimum allowable hot tank HTF temperature",                                                                                              "C",            "",                                  "Thermal Storage",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "hot_tank_max_heat",                  "Rated heater capacity for hot tank heating",                                                                                              "MW",           "",                                  "Thermal Storage",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "tanks_in_parallel",                  "Tanks are in parallel, not in series, with solar field",                                                                                  "-",            "",                                  "Thermal Storage",                          "*",                                                                "",              "" },


        //RADIATIVE COOLING WITH COLD STORAGE
        //   Includes radiative cooling cost parameters
    { SSC_INPUT,     SSC_NUMBER, "h_ctes_tank_min",                    "Minimum allowable water height in storage tank",                                                                                          "m",            "",                                  "RADCOOL",                                  "?=0",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "ctes_tshours",                       "Equivalent full load storage hours",                                                                                                      "hr",           "",                                  "RADCOOL",                                  "?=0",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "ctes_field_fl",                      "Fluid in radiator field. 3=liquid water. Other = Glycol.",                                                                                "-",            "",                                  "RADCOOL",                                  "?=3",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "h_ctes_tank",                        "Total height of cold storage tank when full",                                                                                             "m",            "",                                  "RADCOOL",                                  "?=0",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "u_ctes_tank",                        "Loss coefficient from cold storage tank",                                                                                                 "W/m2-K",       "",                                  "RADCOOL",                                  "?=0",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "ctes_tankpairs",                     "Number of equivalent tank pairs",                                                                                                         "-",            "",                                  "RADCOOL",                                  "?=0",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "T_ctes_cold_design",                 "Design value of cooled water to power block",                                                                                             "C",            "",                                  "RADCOOL",                                  "?=0",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "T_ctes_warm_design",                 "Design value of warm water returning from power block",                                                                                   "C",            "",                                  "RADCOOL",                                  "?=0",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "T_ctes_warm_ini",                    "Initial value of warm tank",                                                                                                              "C",            "",                                  "RADCOOL",                                  "?=0",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "T_ctes_cold_ini",                    "Initial value of cold tank",                                                                                                              "C",            "",                                  "RADCOOL",                                  "?=0",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "f_ctes_warm_ini",                    "Initial fraction of avail. volume that is warm",                                                                                          "-",            "",                                  "RADCOOL",                                  "?=0",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "rad_multiplier",                     "Ratio of radiator field area to solar aperture area",                                                                                    "-",            "",                                  "RADCOOL",                                  "?=0",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "m_dot_radpanel",                     "Mass flow rate through single radiator panel",                                                                                            "kg/sec",       "",                                  "RADCOOL",                                  "?=0",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "n_rad_tubes",                        "Number of parallel tubes in single radiator panel",                                                                                       "-",            "",                                  "RADCOOL",                                  "?=0",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "W_rad_tubes",                        "Center-to-center distance between tubes in radiator panel",                                                                               "m",            "",                                  "RADCOOL",                                  "?=0",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "L_rad",                              "Length of radiator panel row",                                                                                                            "m",            "",                                  "RADCOOL",                                  "?=0",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "th_rad_panel",                       "Thickness of radiator panel",                                                                                                             "m",            "",                                  "RADCOOL",                                  "?=0",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "D_rad_tubes",                        "Inner diameter of tubes in radiator panel",                                                                                               "m",            "",                                  "RADCOOL",                                  "?=0",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "k_panel",                            "Thermal conductivity of radiator panel material",                                                                                         "W/m-K",        "",                                  "RADCOOL",                                  "?=235",                                                            "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "epsilon_radtop",                     "Emissivity of top of radiator panel",                                                                                                    "-",            "",                                  "RADCOOL",                                  "?=.95",                                                            "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "epsilon_radbot",                     "Emissivity of top of radiator panel bottom (facing ground)",                                                                             "-",            "",                                  "RADCOOL",                                  "?=.07",                                                            "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "epsilon_radgrnd",                    "Emissivity of ground underneath radiator panel",                                                                                         "-",            "",                                  "RADCOOL",                                  "?=.90",                                                            "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "L_rad_sections",                     "Length of individual radiator panel",                                                                                                     "m",            "",                                  "RADCOOL",                                  "?=0",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "epsilon_radHX",                      "Effectiveness of HX between radiative field and cold storage",                                                                            "-",            "",                                  "RADCOOL",                                  "?=.8",                                                             "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "ctes_type",                          "Type of cold storage (2=two tank, 3= three node)",                                                                                        "-",            "",                                  "RADCOOL",                                  "?=0",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "radiator_unitcost",                  "Cost of radiative panels",                                                                                                                "$/m^2",        "",                                  "RADCOOL",                                  "?=0",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "radiator_installcost",               "Installation cost of radiative panels",                                                                                                   "$/m^2",        "",                                  "RADCOOL",                                  "?=0",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "radiator_fluidcost",                 "Cost of circulating fluid in radiative panels",                                                                                           "$/L",          "",                                  "RADCOOL",                                  "?=0",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "radfluid_vol_ratio",                 "Ratio of fluid in distribution to fluid in panels",                                                                                       "-",            "",                                  "RADCOOL",                                  "?=0",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "ctes_cost",                          "Cost of cold storage construction",                                                                                                       "$/L",          "",                                  "RADCOOL",                                  "?=0",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "rad_pressuredrop",                   "Average pressure drop through a radiative panel & distribution",                                                                          "kPa",          "",                                  "RADCOOL",                                  "?=0",                                                              "",              ""},

    // Power Cycle Inputs
    { SSC_INPUT,     SSC_NUMBER, "pc_config",                          "PC configuration 0=Steam Rankine (224), 1=user defined",                                                                                  "",             "",                                  "Power Cycle",                              "?=0",                                                              "INTEGER",       ""},
    { SSC_INPUT,     SSC_NUMBER, "pb_pump_coef",                       "Pumping power to move 1kg of HTF through PB loop",                                                                                        "kW/kg",        "",                                  "Power Cycle",                              "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "startup_time",                       "Time needed for power block startup",                                                                                                     "hr",           "",                                  "Power Cycle",                              "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "startup_frac",                       "Fraction of design thermal power needed for startup",                                                                                     "none",         "",                                  "Power Cycle",                              "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "cycle_max_frac",                     "Maximum turbine over design operation fraction",                                                                                          "",             "",                                  "Power Cycle",                              "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "cycle_cutoff_frac",                  "Minimum turbine operation fraction before shutdown",                                                                                      "",             "",                                  "Power Cycle",                              "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "q_sby_frac",                         "Fraction of thermal power required for standby",                                                                                          "",             "",                                  "Power Cycle",                              "*",                                                                "",              ""},

        // Optional cycle design parameters potentially useful for analysis
    { SSC_INPUT,     SSC_NUMBER, "is_calc_pb_pump_coef",               "False (default): use input pb_pump_coef, True: calc pb_pump_coef to achieve W_dot_pb_pump_target",                                        "",             "",                                  "Tower and Receiver",                       "?=0",                                                              "",              "SIMULATION_PARAMETER" },
    { SSC_INPUT,     SSC_NUMBER, "W_dot_pb_pump_target",               "Target HTF pumping power loss through cycle primary heat exchanger",                                                                      "MWe",          "",                                  "Tower and Receiver",                       "is_calc_pb_pump_coef=1",                                           "",              "SIMULATION_PARAMETER" },

    // Steam Rankine cycle
    { SSC_INPUT,     SSC_NUMBER, "dT_cw_ref",                          "Reference condenser cooling water inlet/outlet temperature difference",                                                                   "C",            "",                                  "Rankine Cycle",                            "pc_config=0",                                                      "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "T_amb_des",                          "Reference ambient temperature at design point",                                                                                           "C",            "",                                  "Rankine Cycle",                            "pc_config=0",                                                      "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "CT",                                 "Condenser type: 1=evaporative, 2=air, 3=hybrid",                                                                                          "",             "",                                  "Rankine Cycle",                            "pc_config=0",                                                      "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "T_approach",                         "Cooling tower approach temperature",                                                                                                      "C",            "",                                  "Rankine Cycle",                            "pc_config=0",                                                      "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "T_ITD_des",                          "ITD at design for dry system",                                                                                                            "C",            "",                                  "Rankine Cycle",                            "pc_config=0",                                                      "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "P_cond_ratio",                       "Condenser pressure ratio",                                                                                                                "",             "",                                  "Rankine Cycle",                            "pc_config=0",                                                      "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "pb_bd_frac",                         "Power block blowdown steam fraction",                                                                                                     "",             "",                                  "Rankine Cycle",                            "pc_config=0",                                                      "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "P_cond_min",                         "Minimum condenser pressure",                                                                                                              "inHg",         "",                                  "Rankine Cycle",                            "pc_config=0",                                                      "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "n_pl_inc",                           "Number of part-load increments for the heat rejection system",                                                                            "none",         "",                                  "Rankine Cycle",                            "pc_config=0",                                                      "INTEGER",       ""},
    { SSC_INPUT,     SSC_ARRAY,  "F_wc",                               "TOU array of fractions indicating wet cooling share for hybrid cooling",                                                                  "",             "",                                  "System Control",                           "pc_config=0",                                                      "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "tech_type",                          "Turbine inlet pressure control 1=Fixed, 3=Sliding",                                                                                       "",             "",                                  "Rankine Cycle",                            "pc_config=0",                                                      "",              ""},

    // User Defined cycle
    { SSC_INPUT,     SSC_NUMBER, "ud_f_W_dot_cool_des",                "Percent of user-defined power cycle design gross output consumed by cooling",                                                             "%",            "",                                  "User Defined Power Cycle",                 "pc_config=1",                                                      "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "ud_m_dot_water_cool_des",            "Mass flow rate of water required at user-defined power cycle design point",                                                               "kg/s",         "",                                  "User Defined Power Cycle",                 "pc_config=1",                                                      "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "ud_is_sco2_regr",                    "False: default, base udpc interpolation, True: use sco2 heuristic regression",                                                            "",             "",                                  "User Defined Power Cycle",                 "?=0",                                                              "",              ""},
    { SSC_INPUT,     SSC_MATRIX, "ud_ind_od",                          "Off design user-defined power cycle performance as function of T_htf, m_dot_htf [ND], and T_amb",                                         "",             "",                                  "User Defined Power Cycle",                 "pc_config=1",                                                      "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "use_net_cycle_output_as_capacity",   "False: default, use net calculation including system parasitic, True: for UDPC only, set as cycle output less cooling power",            "",             "",                                  "User Defined Power Cycle",                 "?=0",                                                              "",              "SIMULATION_PARAMETER" },

    // Aux and Balance of Plant
    { SSC_INPUT,     SSC_NUMBER, "pb_fixed_par",                       "Fixed parasitic load - runs at all times",                                                                                                "MWe/MWcap",    "",                                  "System Control",                           "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "aux_par",                            "Aux heater, boiler parasitic",                                                                                                            "MWe/MWcap",    "",                                  "System Control",                           "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "aux_par_f",                          "Aux heater, boiler parasitic - multiplying fraction",                                                                                     "",             "",                                  "System Control",                           "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "aux_par_0",                          "Aux heater, boiler parasitic - constant coefficient",                                                                                     "",             "",                                  "System Control",                           "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "aux_par_1",                          "Aux heater, boiler parasitic - linear coefficient",                                                                                       "",             "",                                  "System Control",                           "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "aux_par_2",                          "Aux heater, boiler parasitic - quadratic coefficient",                                                                                    "",             "",                                  "System Control",                           "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "bop_par",                            "Balance of plant parasitic power fraction",                                                                                               "MWe/MWcap",    "",                                  "System Control",                           "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "bop_par_f",                          "Balance of plant parasitic power fraction - mult frac",                                                                                   "",             "",                                  "System Control",                           "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "bop_par_0",                          "Balance of plant parasitic power fraction - const coeff",                                                                                 "",             "",                                  "System Control",                           "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "bop_par_1",                          "Balance of plant parasitic power fraction - linear coeff",                                                                                "",             "",                                  "System Control",                           "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "bop_par_2",                          "Balance of plant parasitic power fraction - quadratic coeff",                                                                             "",             "",                                  "System Control",                           "*",                                                                "",              "" },

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
    { SSC_INPUT,     SSC_NUMBER, "disp_spec_presolve",                 "Dispatch optimization pre-solve heuristic",                                                                                                "",             "",                                  "System Control",                           "?=-1",                                                             "",              "SIMULATION_PARAMETER"},
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
    { SSC_INPUT,     SSC_NUMBER, "q_rec_standby",                      "Receiver standby energy consumption",                                                                                                     "kWt",          "",                                  "System Control",                           "?=9e99",                                                           "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_NUMBER, "q_rec_heattrace",                    "Receiver heat trace energy consumption during startup",                                                                                   "kWe-hr",       "",                                  "System Control",                           "?=0.0",                                                            "",              "SIMULATION_PARAMETER"},

    // Pricing schedules and multipliers
        // Ideally this would work with sim_type = 2, but UI inputs availability depends on financial mode
    { SSC_INPUT,     SSC_NUMBER, "ppa_multiplier_model",               "PPA multiplier model 0: dispatch factors dispatch_factorX, 1: hourly multipliers dispatch_factors_ts",                                    "0/1",          "0=diurnal,1=timestep",              "Time of Delivery Factors",                 "?=0", /*need a default so this var works in required_if*/          "INTEGER,MIN=0", "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_NUMBER, "ppa_soln_mode",                      "PPA solution mode (0=Specify IRR target, 1=Specify PPA price)",                                                                           "",             "",                                  "Financial Solution Mode",                  "sim_type=1&ppa_multiplier_model=0&csp_financial_model<5&is_dispatch=1&sim_type=1",       "",   "SIMULATION_PARAMETER" },
    { SSC_INPUT,     SSC_ARRAY,  "dispatch_factors_ts",                "Dispatch payment factor array",                                                                                                           "",             "",                                  "Time of Delivery Factors",                 "ppa_multiplier_model=1&csp_financial_model<5&is_dispatch=1&sim_type=1",       "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_NUMBER, "en_electricity_rates",               "Enable electricity rates for grid purchase",                                                                                              "0/1",          "",                                  "Electricity Rates",                        "?=0",                                                                         "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_MATRIX, "dispatch_sched_weekday",             "PPA pricing weekday schedule, 12x24",                                                                                                     "",             "",                                  "Time of Delivery Factors",                 "ppa_multiplier_model=0&csp_financial_model<5&is_dispatch=1&sim_type=1",       "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_MATRIX, "dispatch_sched_weekend",             "PPA pricing weekend schedule, 12x24",                                                                                                     "",             "",                                  "Time of Delivery Factors",                 "ppa_multiplier_model=0&csp_financial_model<5&is_dispatch=1&sim_type=1",       "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_ARRAY,  "dispatch_tod_factors",               "TOD factors for periods 1 through 9",                                                                                                     "",
        "We added this array input after SAM 2022.12.21 to replace the functionality of former single value inputs dispatch_factor1 through dispatch_factor9",                                                                                                         "Time of Delivery Factors",                 "ppa_multiplier_model=0&csp_financial_model<5&is_dispatch=1&sim_type=1",       "",              "SIMULATION_PARAMETER" },

    { SSC_INPUT,     SSC_ARRAY,  "ppa_price_input",			           "PPA prices - yearly",			                                                                                                          "$/kWh",	      "",	                               "Revenue",			                       "ppa_multiplier_model=0&csp_financial_model<5&is_dispatch=1&sim_type=1",       "",      	       "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_MATRIX, "mp_energy_market_revenue",           "Energy market revenue input",                                                                                                             "",             "Lifetime x 2[Cleared Capacity(MW),Price($/MWh)]", "Revenue",                    "csp_financial_model=6&is_dispatch=1&sim_type=1",                              "",              "SIMULATION_PARAMETER"},

    // Optional Component Initialization (state at start of first timestep)
        // Heliostat field and Receiver
    { SSC_INPUT,     SSC_NUMBER, "is_field_tracking_init",             "Is heliostat field tracking? (1 = true)",                                                                                                 "-",            "",                                  "System Control",                           "",                                                                 "",              "SIMULATION_PARAMETER" },
    { SSC_INPUT,     SSC_NUMBER, "rec_op_mode_initial",                "Initial receiver operating mode 0: off, 1: startup, 2: on",                                                                               "-",            "",                                  "System Control",                           "",                                                                 "",              "SIMULATION_PARAMETER" },
    { SSC_INPUT,     SSC_NUMBER, "rec_startup_time_remain_init",       "Initial receiver startup time remaining",                                                                                                 "hr",           "",                                  "System Control",                           "",                                                                 "",              "SIMULATION_PARAMETER" },
    { SSC_INPUT,     SSC_NUMBER, "rec_startup_energy_remain_init",     "Initial receiver startup energy remaining",                                                                                               "W-hr",         "",                                  "System Control",                           "",                                                                 "",              "SIMULATION_PARAMETER" },
        // Power cycle
    { SSC_INPUT,     SSC_NUMBER, "pc_op_mode_initial",                 "Initial cycle operation mode 0:startup, 1:on, 2:standby, 3:off, 4:startup_controlled",                                                    "-",            "",                                  "System Control",                           "",                                                                 "",              "SIMULATION_PARAMETER" },
    { SSC_INPUT,     SSC_NUMBER, "pc_startup_time_remain_init",        "Initial cycle startup time remaining",                                                                                                    "hr",           "",                                  "System Control",                           "",                                                                 "",              "SIMULATION_PARAMETER" },
    { SSC_INPUT,     SSC_NUMBER, "pc_startup_energy_remain_initial",   "Initial cycle startup energy remaining",                                                                                                  "kwh",          "",                                  "System Control",                           "",                                                                 "",              "SIMULATION_PARAMETER" },
        // Thermal energy storage
    { SSC_INPUT,     SSC_NUMBER, "T_tank_cold_init",                   "Initial cold tank temp",                                                                                                                  "C",            "",                                  "System Control",                           "",                                                                 "",              "SIMULATION_PARAMETER" },
    { SSC_INPUT,     SSC_NUMBER, "T_tank_hot_init",                    "Initial hot tank temp",                                                                                                                   "C",            "",                                  "System Control",                           "",                                                                 "",              "SIMULATION_PARAMETER" },
        // Input Dispatch Targets
    { SSC_INPUT,     SSC_NUMBER, "is_dispatch_targets",                "Run solution from user-specified dispatch targets?",                                                                                      "-",            "",                                  "System Control",                           "?=0",                                                              "",              "SIMULATION_PARAMETER" },
    { SSC_INPUT,     SSC_ARRAY,  "q_pc_target_su_in",                  "User-provided target thermal power to PC",                                                                                                "MWt",          "",                                  "System Control",                           "is_dispatch_targets=1",                                            "",              "SIMULATION_PARAMETER" },
    { SSC_INPUT,     SSC_ARRAY,  "q_pc_target_on_in",                  "User-provided target thermal power to PC",                                                                                                "MWt",          "",                                  "System Control",                           "is_dispatch_targets=1",                                            "",              "SIMULATION_PARAMETER" },
    { SSC_INPUT,     SSC_ARRAY,  "q_pc_max_in",                        "User-provided max thermal power to PC",                                                                                                   "MWt",          "",                                  "System Control",                           "is_dispatch_targets=1",                                            "",              "SIMULATION_PARAMETER" },
    { SSC_INPUT,     SSC_ARRAY,  "is_rec_su_allowed_in",               "User-provided is receiver startup allowed?",                                                                                              "-",            "",                                  "System Control",                           "is_dispatch_targets=1",                                            "",              "SIMULATION_PARAMETER" },
    { SSC_INPUT,     SSC_ARRAY,  "is_pc_su_allowed_in",                "User-provided is power cycle startup allowed?",                                                                                           "-",            "",                                  "System Control",                           "is_dispatch_targets=1",                                            "",              "SIMULATION_PARAMETER" },
    { SSC_INPUT,     SSC_ARRAY,  "is_pc_sb_allowed_in",                "User-provided is power cycle standby allowed?",                                                                                           "-",            "",                                  "System Control",                           "is_dispatch_targets=1",                                            "",              "SIMULATION_PARAMETER" },
    { SSC_INPUT,     SSC_ARRAY,  "q_dot_elec_to_PAR_HTR_in",           "User-provided electrical power to parallel heater",                                                                                       "-",            "",                                  "System Control",                           "is_dispatch_targets=1&is_parallel_htr=1",                                            "",              "SIMULATION_PARAMETER" },
    { SSC_INPUT,     SSC_ARRAY,  "is_PAR_HTR_allowed_in",              "User-provided is electrical heater operation allowed?",                                                                                   "-",            "",                                  "System Control",                           "is_dispatch_targets=1&is_parallel_htr=1",                                            "",              "SIMULATION_PARAMETER" },

    // Costs
    { SSC_INPUT,     SSC_NUMBER, "tower_fixed_cost",                   "Tower fixed cost",                                                                                                                        "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "tower_exp",                          "Tower cost scaling exponent",                                                                                                             "",             "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "rec_ref_cost",                       "Receiver reference cost",                                                                                                                 "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "rec_ref_area",                       "Receiver reference area for cost scale",                                                                                                  "",             "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "rec_cost_exp",                       "Receiver cost scaling exponent",                                                                                                          "",             "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "site_spec_cost",                     "Site improvement cost",                                                                                                                   "$/m2",         "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "heliostat_spec_cost",                "Heliostat field cost",                                                                                                                    "$/m2",         "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "plant_spec_cost",                    "Power cycle specific cost",                                                                                                               "$/kWe",        "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "bop_spec_cost",                      "BOS specific cost",                                                                                                                       "$/kWe",        "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "tes_spec_cost",                      "Thermal energy storage cost",                                                                                                             "$/kWht",       "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "land_spec_cost",                     "Total land area cost",                                                                                                                    "$/acre",       "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "contingency_rate",                   "Contingency for cost overrun",                                                                                                            "%",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "sales_tax_rate",                     "Sales tax rate",                                                                                                                          "%",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "sales_tax_frac",                     "Percent of cost to which sales tax applies",                                                                                              "%",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "cost_sf_fixed",                      "Solar field fixed cost",                                                                                                                  "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "fossil_spec_cost",                   "Fossil system specific cost",                                                                                                             "$/kWe",        "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "csp.pt.cost.epc.per_acre",           "EPC cost per acre",                                                                                                                       "$/acre",       "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "csp.pt.cost.epc.percent",            "EPC cost percent of direct",                                                                                                              "%",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "csp.pt.cost.epc.per_watt",           "EPC cost per watt",                                                                                                                       "$/W",          "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "csp.pt.cost.epc.fixed",              "EPC fixed",                                                                                                                               "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "csp.pt.cost.plm.percent",            "PLM cost percent of direct",                                                                                                              "%",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "csp.pt.cost.plm.per_watt",           "PLM cost per watt",                                                                                                                       "$/W",          "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "csp.pt.cost.plm.fixed",              "PLM fixed",                                                                                                                               "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "csp.pt.sf.fixed_land_area",          "Fixed land area",                                                                                                                         "acre",         "",                                  "Heliostat Field",                          "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "csp.pt.sf.land_overhead_factor",     "Land overhead factor",                                                                                                                    "",             "",                                  "Heliostat Field",                          "*",                                                                "",              "" },

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
    { SSC_INPUT,     SSC_NUMBER, "piping_loss",                        "Thermal loss per meter of piping",                                                                                                        "Wt/m",         "",                                  "Deprecated",                           "",                                                                 "",              "SIMULATION_PARAMETER" },
    { SSC_INPUT,     SSC_NUMBER, "disp_csu_cost",                      "Cycle startup cost",                                                                                                                      "$",            "",                                  "Deprecated",                           "",                                                                 "",              "SIMULATION_PARAMETER" },
    { SSC_INPUT,     SSC_NUMBER, "disp_rsu_cost",                      "Receiver startup cost",                                                                                                                   "$",            "",                                  "Deprecated",                           "",                                                                 "",              "SIMULATION_PARAMETER" },
    { SSC_INPUT,     SSC_NUMBER, "disp_pen_delta_w",                   "Dispatch cycle production change penalty",                                                                                                "$/kWe-change", "",                                  "Deprecated",                           "",                                                                 "",              "SIMULATION_PARAMETER" },
    { SSC_INPUT,     SSC_NUMBER, "P_boil",                             "Boiler operating pressure",                                                                                                               "bar",          "",                                  "Deprecated",                           "",                                                                 "",              "SIMULATION_PARAMETER" },
    { SSC_INPUT,     SSC_NUMBER, "csp.pt.tes.init_hot_htf_percent",    "Initial fraction of available volume that is hot",                                                                                        "%",            "",                                  "Deprecated",                           "",                                                                 "",              "SIMULATION_PARAMETER" },

    // ****************************************************************************************************************************************
    // Design Outputs here:
    // ****************************************************************************************************************************************

        // land area with variable name required by downstream financial model
    { SSC_OUTPUT,    SSC_NUMBER, "total_land_area",                    "Total land area",                                                                                                                         "acre",         "",                                  "System Costs",                             "*",                                                                "",              "" },
        // System capacity required by downstream financial model
    { SSC_OUTPUT,    SSC_NUMBER, "system_capacity",                    "System capacity",                                                                                                                         "kWe",          "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "cp_system_nameplate",                 "System capacity for capacity payments",                                                                                                   "MWe",          "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "cp_battery_nameplate",                "Battery nameplate",                                                                                                                       "MWe",          "",                                  "System Costs",                             "*",                                                                "",              "" },

        // Solar Field
    { SSC_OUTPUT,    SSC_NUMBER, "N_hel_calc",                         "Number of heliostats - out",                                                                                                               "",             "",                                  "Heliostat Field",                          "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "refl_image_error",                   "Reflected image error",                                                                                                                    "mrad",         "",                                  "Heliostat Field",                          "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "heliostat_area",                     "Active area of heliostat",                                                                                                                 "m^2",          "",                                  "Heliostat Field",                          "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "average_attenuation",                "Average solar field attenuation",                                                                                                          "%",            "",                                  "Heliostat Field",                          "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_MATRIX, "helio_positions_calc",               "Heliostat position table - out",                                                                                                           "",             "",                                  "Heliostat Field",                          "*",                                                                "",              "COL_LABEL=XY_POSITION" },
    { SSC_OUTPUT,    SSC_NUMBER, "A_sf",                               "Solar field area",                                                                                                                         "m^2",          "",                                  "Heliostat Field",                          "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "land_min_abs",                       "Min distance from tower to heliostat",                                                                                                     "m",            "",                                  "Heliostat Field",                          "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "land_max_abs",                       "Max distance from tower to heliostat",                                                                                                     "m",            "",                                  "Heliostat Field",                          "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "land_area_base_calc",                "Land area occupied by heliostats",                                                                                                         "acre",         "",                                  "Heliostat Field",                          "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "total_land_area_before_rad_cooling_calc", "Total land area not including radiative cooling - out",                                                                               "acre",         "",                                  "Heliostat Field",                          "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "W_dot_col_tracking_des",             "Collector tracking power at design",                                                                                                       "MWe",          "",                                  "Heliostat Field",                          "*",                                                                "",              "" },

        // Receiver Geometry
    { SSC_OUTPUT,    SSC_NUMBER, "rec_height_calc",                    "Receiver height - out",                                                                                                                    "m",            "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "D_rec_calc",                         "The overall outer diameter of the receiver - out",                                                                                         "m",            "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "h_tower_calc",                       "Tower height - out",                                                                                                                       "m",            "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "ext_rec_area",                       "External receiver area - out",                                                                                                             "m2",           "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "ext_rec_aspect",                     "External receiver aspect ratio - out",                                                                                                     "",             "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "cav_rec_height_calc",                "Cavity receiver height - out",                                                                                                             "m",            "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "cav_rec_width_calc",                 "Cavity receiver aperture width - out",                                                                                                     "m",            "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "cav_rec_area",                       "Cavity receiver area",                                                                                                                     "m2",           "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "cav_panel_width",                    "Cavity panel width",                                                                                                                       "m",            "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },        
    { SSC_OUTPUT,    SSC_NUMBER, "cav_radius",                         "Cavity radius",                                                                                                                            "m",            "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },        
    { SSC_OUTPUT,    SSC_NUMBER, "A_rec",                              "Receiver area - planar",                                                                                                                   "m2",           "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "L_tower_piping_calc",                "Tower piping length",                                                                                                                      "m",            "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "od_tube_calc",                       "Receiver tube outer diameter - out",                                                                                                       "mm",           "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },

        // Receiver Performance
    { SSC_OUTPUT,    SSC_NUMBER, "q_dot_rec_des",                      "Receiver thermal output at design",                                                                                                       "MWt",         "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "solar_mult_calc",                    "Receiver solar multiple - out",                                                                                                           "",            "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "eta_rec_thermal_des",                "Receiver estimated thermal efficiency at design",                                                                                         "",            "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "W_dot_rec_pump_des",                 "Receiver estimated pump power at design",                                                                                                 "MWe",         "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "W_dot_rec_pump_tower_share_des",     "Receiver estimated pump power due to tower height at design",                                                                             "MWe",         "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "W_dot_rec_pump_rec_share_des",       "Receiver estimated pump power due to rec tubes at design",                                                                                "MWe",         "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "vel_rec_htf_des",                    "Receiver estimated tube HTF velocity at design",                                                                                          "m/s",         "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "m_dot_htf_rec_des",                  "Receiver HTF mass flow rate at design",                                                                                                   "kg/s",        "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "m_dot_htf_rec_max",                  "Receiver max HTF mass flow rate",                                                                                                         "kg/s",        "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "q_dot_piping_loss_des",              "Receiver estimated piping loss at design",                                                                                                "MWt",         "",                                  "Tower and Receiver",                       "*",                                                                "",              "" },

        // Heater
    { SSC_OUTPUT,    SSC_NUMBER, "q_dot_heater_des",                   "Heater design thermal power",                                                                                                             "MWt",         "",                                  "Heater",                                   "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "W_dot_heater_des",                   "Heater electricity consumption at design",                                                                                                "MWe",         "",                                  "Heater",                                   "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "E_heater_su_des",                    "Heater startup energy",                                                                                                                   "MWt-hr",      "",                                  "Heater",                                   "*",                                                                "",              "" },

        // Power Cycle
    { SSC_OUTPUT,    SSC_NUMBER, "m_dot_htf_cycle_des",                "PC HTF mass flow rate at design",                                                                                                         "kg/s",        "",                                  "Power Cycle",                              "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "q_dot_cycle_des",                    "PC thermal input at design",                                                                                                              "MWt",         "",                                  "Power Cycle",                              "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "W_dot_cycle_pump_des",               "PC HTF pump power at design",                                                                                                             "MWe",         "",                                  "Power Cycle",                              "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "W_dot_cycle_cooling_des",            "PC cooling power at design",                                                                                                              "MWe",         "",                                  "Power Cycle",                              "*",                                                                "",              "" },
        // UDPC
    { SSC_OUTPUT,    SSC_NUMBER, "n_T_htf_pars_calc",                  "UDPC number of HTF parametric values",                                                                                                     "",            "",                                  "UDPC Design Calc",                        "*",                                                                "",              "" },      
    { SSC_OUTPUT,    SSC_NUMBER, "n_T_amb_pars_calc",                  "UDPC number of ambient temp parametric values",                                                                                            "",            "",                                  "UDPC Design Calc",                        "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "n_m_dot_pars_calc",                  "UDPC number of mass flow parametric values",                                                                                               "",            "",                                  "UDPC Design Calc",                        "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "T_htf_ref_calc",                     "UDPC reference HTF temperature",                                                                                                           "C",           "",                                  "UDPC Design Calc",                        "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "T_htf_low_calc",                     "UDPC low level HTF temperature",                                                                                                           "C",           "",                                  "UDPC Design Calc",                        "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "T_htf_high_calc",                    "UDPC high level HTF temperature",                                                                                                          "C",           "",                                  "UDPC Design Calc",                        "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "T_amb_ref_calc",                     "UDPC reference ambient temperature",                                                                                                       "C",           "",                                  "UDPC Design Calc",                        "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "T_amb_low_calc",                     "UDPC low level ambient temperature",                                                                                                       "C",           "",                                  "UDPC Design Calc",                        "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "T_amb_high_calc",                    "UDPC high level ambient temperature",                                                                                                      "C",           "",                                  "UDPC Design Calc",                        "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "m_dot_htf_ND_ref_calc",              "UDPC reference normalized mass flow rate",                                                                                                 "",            "",                                  "UDPC Design Calc",                        "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "m_dot_htf_ND_low_calc",              "UDPC low level normalized mass flow rate",                                                                                                 "",            "",                                  "UDPC Design Calc",                        "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "m_dot_htf_ND_high_calc",             "UDPC high level normalized mass flow rate",                                                                                                "",            "",                                  "UDPC Design Calc",                        "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "W_dot_gross_ND_des_calc",            "UDPC calculated normalized gross power at design",                                                                                         "",            "",                                  "UDPC Design Calc",                        "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "Q_dot_HTF_ND_des_calc",              "UDPC calculated normalized heat input at design",                                                                                          "",            "",                                  "UDPC Design Calc",                        "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "W_dot_cooling_ND_des_calc",          "UPPC calculated normalized cooling power at design",                                                                                       "",            "",                                  "UDPC Design Calc",                        "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "m_dot_water_ND_des_calc",            "UDPC calculated water use at design",                                                                                                      "",            "",                                  "UDPC Design Calc",                        "*",                                                                "",              "" },

        // TES
    { SSC_OUTPUT,    SSC_NUMBER, "Q_tes_des",                          "TES design capacity",                                                                                                                     "MWt-hr",       "",                                 "TES Design Calc",                          "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "V_tes_htf_avail_des",                "TES volume of HTF available for heat transfer",                                                                                           "m3",           "",                                 "TES Design Calc",                          "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "V_tes_htf_total_des",                "TES total HTF volume",                                                                                                                    "m3",           "",                                 "TES Design Calc",                          "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "d_tank_tes",                         "TES tank diameter",                                                                                                                       "m",            "",                                 "TES Design Calc",                          "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "q_dot_loss_tes_des",                 "TES thermal loss at design",                                                                                                              "MWt",          "",                                 "TES Design Calc",                          "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "tshours_rec",                        "TES duration at receiver design output",                                                                                                  "hr",           "",                                 "TES Design Calc",                          "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "tshours_heater",                     "TES duration at heater design output",                                                                                                    "hr",           "",                                 "TES Design Calc",                          "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "dens_store_htf_at_T_ave",            "TES density of HTF at avg temps",                                                                                                         "kg/m3",        "",                                 "TES Design Calc",                          "*",                                                                "",              "" },

        // Balance of Plant
    { SSC_OUTPUT,    SSC_NUMBER, "nameplate",                          "Nameplate capacity",                                                                                                                      "MWe",          "",                                 "System Design Calc",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "W_dot_bop_design",                   "BOP parasitic at design",                                                                                                                "MWe",          "",                                 "Balance of Plant",                         "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "W_dot_fixed",                        "Fixed parasitic at design",                                                                                                               "MWe",          "",                                 "Balance of Plant",                         "*",                                                                "",              "" },

        // Costs
    { SSC_OUTPUT,    SSC_NUMBER, "h_rec_input_to_cost_model",          "Receiver height for cost model selected from receiver type",                                                                              "m",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "csp.pt.cost.site_improvements",      "Site improvement cost",                                                                                                                   "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "csp.pt.cost.heliostats",             "Heliostat cost",                                                                                                                          "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "csp.pt.cost.tower",                  "Tower cost",                                                                                                                              "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "csp.pt.cost.receiver",               "Receiver cost",                                                                                                                           "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "csp.pt.cost.storage",                "TES cost",                                                                                                                                "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "csp.pt.cost.power_block",            "Power cycle cost",                                                                                                                        "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "heater_cost",                        "Heater cost",                                                                                                                             "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "csp.pt.cost.rad_field",              "Radiative field cost"                                                                                                                     "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "csp.pt.cost.rad_fluid",              "Radiative fluid cost"                                                                                                                     "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "csp.pt.cost.rad_storage",            "Cold storage cost"                                                                                                                        "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "csp.pt.cost.bop",                    "BOP cost",                                                                                                                                "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "csp.pt.cost.fossil",                 "Fossil backup cost",                                                                                                                      "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "ui_direct_subtotal",                 "Direct capital pre-contingency cost",                                                                                                      "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "csp.pt.cost.contingency",            "Contingency cost",                                                                                                                        "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "total_direct_cost",                  "Total direct cost",                                                                                                                       "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "csp.pt.cost.epc.total",              "EPC and owner cost",                                                                                                                      "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "csp.pt.cost.plm.total",              "Total land cost",                                                                                                                         "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "csp.pt.cost.sales_tax.total",        "Sales tax cost",                                                                                                                          "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "total_indirect_cost",                "Total indirect cost",                                                                                                                     "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "total_installed_cost",               "Total installed cost",                                                                                                                    "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "csp.pt.cost.installed_per_capacity", "Estimated installed cost per cap",                                                                                                        "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },

        // Financing
    { SSC_OUTPUT,    SSC_NUMBER, "const_per_principal1",               "Principal, loan 1",                                                                                                                       "$",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "const_per_principal2",               "Principal, loan 2",                                                                                                                       "$",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "const_per_principal3",               "Principal, loan 3",                                                                                                                       "$",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "const_per_principal4",               "Principal, loan 4",                                                                                                                       "$",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "const_per_principal5",               "Principal, loan 5",                                                                                                                       "$",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "const_per_interest1",                "Interest cost, loan 1",                                                                                                                   "$",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "const_per_interest2",                "Interest cost, loan 2",                                                                                                                   "$",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "const_per_interest3",                "Interest cost, loan 3",                                                                                                                   "$",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "const_per_interest4",                "Interest cost, loan 4",                                                                                                                   "$",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "const_per_interest5",                "Interest cost, loan 5",                                                                                                                   "$",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "const_per_total1",                   "Total financing cost, loan 1",                                                                                                            "$",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "const_per_total2",                   "Total financing cost, loan 2",                                                                                                            "$",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "const_per_total3",                   "Total financing cost, loan 3",                                                                                                            "$",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "const_per_total4",                   "Total financing cost, loan 4",                                                                                                            "$",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "const_per_total5",                   "Total financing cost, loan 5",                                                                                                            "$",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "const_per_percent_total",            "Total percent of installed costs, all loans",                                                                                             "%",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "const_per_principal_total",          "Total principal, all loans",                                                                                                              "$",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "const_per_interest_total",           "Total interest costs, all loans",                                                                                                         "$",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "construction_financing_cost",        "Total construction financing cost",                                                                                                       "$",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },



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

        // Collector-receiver outputs
    { SSC_OUTPUT,    SSC_MATRIX, "eta_map_out",                        "Solar field optical efficiencies",                                                                                                        "",             "",                                  "",                                         "sim_type=1",                                                       "",              "COL_LABEL=OPTICAL_EFFICIENCY,ROW_LABEL=NO_ROW_LABEL"},
    { SSC_OUTPUT,    SSC_MATRIX, "flux_maps_for_import",               "Flux map for import",                                                                                                                     "",             "",                                  "",                                         "sim_type=1",                                                       "",              "COL_LABEL=FLUX_MAPS,ROW_LABEL=NO_ROW_LABEL" },
    { SSC_OUTPUT,    SSC_MATRIX, "flux_maps_out",                      "Flux map intensities",                                                                                                                    "",             "",                                  "",                                         "sim_type=1",                                                       "",              "COL_LABEL=FLUX_MAPS,ROW_LABEL=NO_ROW_LABEL"},

    { SSC_OUTPUT,    SSC_ARRAY,  "q_sf_inc",                           "Field incident thermal power",                                                                                                            "MWt",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "eta_field",                          "Field optical efficiency",                                                                                                                "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "defocus",                            "Field optical focus fraction",                                                                                                            "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "sf_adjust_out",                      "Field availability adjustment factor",                                                                                                    "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "rec_defocus",                        "Receiver component defocus",                                                                                                              "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "q_dot_rec_inc",                      "Receiver incident power after refl and defocus",                                                                                          "MWt",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "eta_therm",                          "Receiver thermal efficiency",                                                                                                             "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "Q_thermal",                          "Receiver thermal power to HTF less piping loss",                                                                                          "MWt",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "pparasi",                            "Field tracking power",                                                                                                                    "MWe",          "",                                  "",                                         "sim_type=1",                                                       "",              "" },

    { SSC_OUTPUT,    SSC_ARRAY,  "m_dot_rec",                          "Receiver mass flow rate",                                                                                                                 "kg/s",         "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "q_startup",                          "Receiver startup thermal energy consumed",                                                                                                "MWt",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "T_rec_in",                           "Receiver HTF inlet temperature",                                                                                                          "C",            "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "T_rec_out",                          "Receiver HTF outlet temperature",                                                                                                         "C",            "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "q_piping_losses",                    "Receiver header/tower piping losses",                                                                                                     "MWt",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "q_thermal_loss",                     "Receiver convection and emission losses",                                                                                                 "MWt",          "",                                  "",                                         "sim_type=1",                                                       "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "q_dot_reflection_loss",              "Receiver reflection losses",                                                                                                              "MWt",          "",                                  "",                                         "sim_type=1&receiver_type=1",                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "P_tower_pump",                       "Receiver and tower HTF pumping power",                                                                                                    "MWe",          "",                                  "",                                         "sim_type=1",                                                       "",              "" },

    { SSC_OUTPUT,    SSC_ARRAY,  "T_rec_out_end",                      "Receiver HTF outlet temperature at end of timestep",                                                                                      "C",            "",								   "CR",                                       "sim_type=1&is_rec_model_trans=1",                                  "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "T_rec_out_max",                      "Receiver maximum HTF outlet temperature during timestep",                                                                                 "C",            "",                                  "CR",                                       "sim_type=1&is_rec_model_trans=1",                                  "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "T_panel_out_max",                    "Receiver panel maximum HTF outlet temperature during timestep",                                                                           "C",            "",                                  "CR",                                       "sim_type=1&is_rec_model_trans=1",                                  "",              ""},    
    { SSC_OUTPUT,    SSC_ARRAY,  "T_wall_rec_inlet",                   "Receiver inlet panel wall temperature at end of timestep",                                                                                "C",            "",                                  "CR",                                       "sim_type=1&is_rec_model_trans=1",                                  "",              ""},    
    { SSC_OUTPUT,    SSC_ARRAY,  "T_wall_rec_outlet",                  "Receiver outlet panel wall temperature at end of timestep",                                                                               "C",            "",                                  "CR",                                       "sim_type=1&is_rec_model_trans=1",                                  "",              ""},    
    { SSC_OUTPUT,    SSC_ARRAY,  "T_wall_riser",                       "Receiver riser wall temperature at end of timestep",                                                                                      "C",            "",                                  "CR",                                       "sim_type=1&is_rec_model_trans=1",                                  "",              ""},    
    { SSC_OUTPUT,    SSC_ARRAY,  "T_wall_downcomer",                   "Receiver downcomer wall temperature at end of timestep",                                                                                  "C",            "",                                  "CR",                                       "sim_type=1&is_rec_model_trans=1",                                  "",              ""},    
                                                                                                                                                                                                                                                                                                                    
	{ SSC_OUTPUT,    SSC_ARRAY,  "clearsky",						   "Predicted clear-sky beam normal irradiance",																							  "W/m2",         "",                                  "CR",                                       "sim_type=1&rec_clearsky_fraction>0",                               "",              "" },
	{ SSC_OUTPUT,    SSC_ARRAY,  "Q_thermal_ss",					   "Receiver thermal power to HTF less piping loss (steady state)",																			  "MWt",          "",                                  "CR",                                       "sim_type=1&is_rec_model_trans=1",                                  "",              "" },
	{ SSC_OUTPUT,    SSC_ARRAY,  "Q_thermal_ss_csky",				   "Receiver thermal power to HTF less piping loss under clear-sky conditions (steady state)",												  "MWt",          "",                                  "CR",                                       "sim_type=1&rec_clearsky_fraction>0",                               "",              "" },

        // Heater outputs is_parallel_htr
    { SSC_OUTPUT,    SSC_ARRAY,  "W_dot_heater",                       "Parallel heater electricity consumption",                                                                                                 "MWe",          "",                                  "Parallel Heater",                          "sim_type=1&is_parallel_htr=1",                                     "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "q_dot_heater_to_htf",                "Parallel heater thermal power to HTF",                                                                                                    "MWt",          "",                                  "Parallel Heater",                          "sim_type=1&is_parallel_htr=1",                                     "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "q_dot_heater_startup",               "Parallel heater thermal power consumed during startup",                                                                                   "MWt",          "",                                  "Parallel Heater",                          "sim_type=1&is_parallel_htr=1",                                     "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "m_dot_htf_heater",                   "Parallel heater HTF mass flow rate",                                                                                                      "kg/s",         "",                                  "Parallel Heater",                          "sim_type=1&is_parallel_htr=1",                                     "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "T_htf_heater_in",                    "Parallel heater HTF inlet temperature",                                                                                                   "C",            "",                                  "Parallel Heater",                          "sim_type=1&is_parallel_htr=1",                                     "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "T_htf_heater_out",                   "Parallel heater HTF outlet temperature",                                                                                                  "C",            "",                                  "Parallel Heater",                          "sim_type=1&is_parallel_htr=1",                                     "",              "" },

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
    { SSC_OUTPUT,    SSC_ARRAY,  "T_cold",                             "Cold storage cold temperature",                                                                                                           "C",            "",                                  "PC",                                       "?",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "m_cold",                             "Cold storage cold tank mass",                                                                                                             "kg",           "",                                  "PC",                                       "?",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "m_warm",                             "Cold storage warm tank mass",                                                                                                             "kg",           "",                                  "PC",                                       "?",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "T_warm",                             "Cold storage warm tank temperature",                                                                                                      "C",            "",                                  "PC",                                       "?",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "T_rad_out",                          "Radiator outlet temperature",                                                                                                             "C",            "",                                  "PC",                                       "?",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "A_radfield",                         "Radiator field surface area",                                                                                                             "m^2",          "",                                  "PC",                                       "?",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "P_cond",                             "PC condensing pressure",                                                                                                                 "Pa",           "",                                  "PC",                                       "?",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "P_cond_iter_err",                    "PC condenser pressure iteration error",                                                                                                    "",             "",                                  "PC",                                       "?",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "radcool_control",                    "Radiative cooling status code",                                                                                                           "-",            "",                                  "PC",                                       "?",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "cycle_htf_pump_power",               "Cycle HTF pump power",                                                                                                                    "MWe",          "",                                  "",                                         "sim_type=1",                                                       "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "P_cooling_tower_tot",                "Parasitic power condenser operation",                                                                                                     "MWe",          "",                                  "",                                         "sim_type=1",                                                       "",              "" },

        // Thermal energy storage outputs
    { SSC_OUTPUT,    SSC_ARRAY,  "tank_losses",                        "TES thermal losses",                                                                                                                      "MWt",          "",                                  "",                                         "sim_type=1",                                                       "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "q_heater",                           "TES freeze protection power",                                                                                                             "MWe",          "",                                  "",                                         "sim_type=1",                                                       "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "T_tes_hot",                          "TES hot temperature",                                                                                                                     "C",            "",                                  "",                                         "sim_type=1",                                                       "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "T_tes_cold",                         "TES cold temperature",                                                                                                                    "C",            "",                                  "",                                         "sim_type=1",                                                       "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "mass_tes_cold",                      "TES cold tank mass (end)",                                                                                                                "kg",           "",                                  "",                                         "sim_type=1",                                                       "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "mass_tes_hot",                       "TES hot tank mass (end)",                                                                                                                 "kg",           "",                                  "",                                         "sim_type=1",                                                       "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "q_dc_tes",                           "TES discharge thermal power",                                                                                                             "MWt",          "",                                  "",                                         "sim_type=1",                                                       "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "q_ch_tes",                           "TES charge thermal power",                                                                                                                "MWt",          "",                                  "",                                         "sim_type=1",                                                       "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "e_ch_tes",                           "TES charge state",                                                                                                                        "MWht",         "",                                  "",                                         "sim_type=1",                                                       "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "m_dot_cr_to_tes_hot",                "Mass flow: field to hot TES",                                                                                                             "kg/s",         "",                                  "",                                         "sim_type=1",                                                       "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "m_dot_tes_hot_out",                  "Mass flow: TES hot out",                                                                                                                  "kg/s",         "",                                  "",                                         "sim_type=1",                                                       "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "m_dot_pc_to_tes_cold",               "Mass flow: cycle to cold TES",                                                                                                            "kg/s",         "",                                  "",                                         "sim_type=1",                                                       "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "m_dot_tes_cold_out",                 "Mass flow: TES cold out",                                                                                                                 "kg/s",         "",                                  "",                                         "sim_type=1",                                                       "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "m_dot_field_to_cycle",               "Mass flow: field to cycle",                                                                                                               "kg/s",         "",                                  "",                                         "sim_type=1",                                                       "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "m_dot_cycle_to_field",               "Mass flow: cycle to field",                                                                                                               "kg/s",         "",                                  "",                                         "sim_type=1",                                                       "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "tes_htf_pump_power",                 "TES HTF pump power",                                                                                                                      "MWe",          "",                                  "",                                         "sim_type=1",                                                       "",              "" },

        // Parasitics outputs
    { SSC_OUTPUT,    SSC_ARRAY,  "P_fixed",                            "Parasitic power fixed load",                                                                                                              "MWe",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "P_plant_balance_tot",                "Parasitic power generation-dependent load",                                                                                               "MWe",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "P_rec_heattrace",                    "Receiver heat trace parasitic load",                                                                                                      "MWe",          "",                                  "System",                                   "sim_type=1&is_rec_model_trans=1",                                  "",              ""},

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
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_qsfsu_expected",                "Dispatch expected solar field startup energy",                                                                                             "MWt",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
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
    { SSC_OUTPUT,    SSC_NUMBER, "annual_energy",                      "Annual total electric power to grid",                                                                                                     "kWhe",         "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "annual_W_cycle_gross",               "Electrical source - power cycle gross output",                                                                                            "kWhe",         "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "annual_W_cooling_tower",             "Total of condenser operation parasitic",                                                                                                 "kWhe",         "",                                  "PC",                                       "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "annual_q_rec_htf",                   "Annual receiver power delivered to HTF",                                                                                                  "MWt-hr",       "",                                  "Tower and Receiver",                       "sim_type=1",                                                       "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "annual_q_rec_inc",                   "Annual receiver incident thermal power after reflective and defocus losses",                                                              "MWt-hr",       "",                                  "Tower and Receiver",                       "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "annual_q_rec_loss",                  "Annual receiver convective and radiative losses",                                                                                         "MWt-hr",       "",                                  "Tower and Receiver",                       "sim_type=1",                                                       "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "annual_q_piping_loss",               "Annual tower piping losses",                                                                                                              "MWt-hr",       "",                                  "Tower and Receiver",                       "sim_type=1",                                                       "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "annual_q_rec_startup",               "Annual receiver startup energy",                                                                                                          "MWt-hr",       "",                                  "Tower and Receiver",                       "sim_type=1",                                                       "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "annual_E_tower_pump",                "Annual tower pumping power",                                                                                                              "MWe-hr",       "",                                  "Tower and Receiver",                       "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "annual_eta_rec_th",                  "Annual receiver thermal efficiency ignoring rec reflective loss",                                                                         "",             "",                                  "Tower and Receiver",                       "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "annual_eta_rec_th_incl_refl",        "Annual receiver thermal efficiency including reflective loss",                                                                            "",             "",                                  "Tower and Receiver",                       "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "annual_q_defocus_est",               "Annual defocus loss estimate",                                                                                                            "MWt-hr",       "",                                  "Tower and Receiver",                       "sim_type=1",                                                       "",              "" },

    { SSC_OUTPUT,    SSC_NUMBER, "conversion_factor",                  "Gross to net conversion factor",                                                                                                          "%",            "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "capacity_factor",                    "Capacity factor",                                                                                                                         "%",            "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "sales_energy_capacity_factor",       "Capacity factor considering only positive net generation periods",                                                                        "%",            "",                                  "",                                         "sim_type=1",                                                       "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "kwh_per_kw",                         "First year kWh/kW",                                                                                                                       "kWh/kW",       "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "annual_total_water_use",             "Total annual water usage, cycle + mirror washing",                                                                                        "m3",           "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "capacity_factor_highest_1000_ppas",  "Capacity factor at 1000 highest ppa timesteps",                                                                                           "-",            "",                                  "",                                         "sim_type=1",                                                       "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "capacity_factor_highest_2000_ppas",  "Capacity factor at 2000 highest ppa timesteps",                                                                                           "-",            "",                                  "",                                         "sim_type=1",                                                       "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "capacity_factor_warmest_100_Tambs",  "Capacity factor at 100 warmest ambient temperatures",                                                                                     "-",            "",                                  "",                                         "sim_type=1",                                                       "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "hot_hours_revenue_fraction",         "Fraction of potential revenue (based on system capacity) earned during hours hotter than 33 C",                                           "-",            "",                                  "",                                         "sim_type=1&csp_financial_model<5",                                 "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "all_hours_revenue_fraction",         "Fraction of potential annual revenue (based on system capacity)",                                                                         "-",            "",                                  "",                                         "sim_type=1&csp_financial_model<5",                                 "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "hot_hours_electricity_sales",        "Electricity sales during hours hotter than 33 C",                                                                                         "$",            "",                                  "",                                         "sim_type=1&csp_financial_model<5",                                 "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "all_hours_electricity_sales",        "Electricity sales",                                                                                                                       "$",            "",                                  "",                                         "sim_type=1&csp_financial_model<5",                                 "",              "" },

    { SSC_OUTPUT,    SSC_NUMBER, "disp_objective_ann",                 "Annual sum of dispatch objective function value",                                                                                         "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "disp_iter_ann",                      "Annual sum of dispatch solver iterations",                                                                                                "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "disp_presolve_nconstr_ann",          "Annual sum of dispatch problem constraint count",                                                                                         "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "disp_presolve_nvar_ann",             "Annual sum of dispatch problem variable count",                                                                                           "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "disp_solve_time_ann",                "Annual sum of dispatch solver time",                                                                                                      "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "disp_solve_state_ann",               "Annual sum of dispatch solve state",                                                                                                      "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "avg_suboptimal_rel_mip_gap",         "Average suboptimal relative MIP gap",                                                                                                     "%",            "",                                  "",                                         "sim_type=1",                                                       "",              ""},

    { SSC_OUTPUT,    SSC_NUMBER, "sim_cpu_run_time",                   "Simulation duration clock time",                                                                                                          "s",            "",                                  "",                                         "sim_type=1",                                                       "",              ""},

    // Final component states (for use in subsequent calls to this cmod as values for "Optional Component Initialization" inputs above
        // Heliostat field and Receiver
    { SSC_OUTPUT,    SSC_ARRAY,  "is_field_tracking_final",            "Final heliostat field operation is tracking? (1 = true)",                                                                                 "-",            "",                                  "System Control",                           "",                                                                 "",              "" },          
    { SSC_OUTPUT,    SSC_ARRAY,  "rec_op_mode_final",                  "Final receiver operating mode 0: off, 1: startup, 2: on",                                                                                 "-",            "",                                  "System Control",                           "",                                                                 "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "rec_startup_time_remain_final",      "Final receiver startup time remaining",                                                                                                   "hr",           "",                                  "System Control",                           "",                                                                 "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "rec_startup_energy_remain_final",    "Final receiver startup energy remaining",                                                                                                 "W-hr",         "",                                  "System Control",                           "",                                                                 "",              "" },
        // Power cycle
    { SSC_OUTPUT,    SSC_ARRAY,  "pc_op_mode_final",                   "Final cycle operation mode 0:startup, 1:on, 2:standby, 3:off, 4:startup_controlled",                                                      "-",            "",                                  "System Control",                           "",                                                                 "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "pc_startup_time_remain_final",       "Final cycle startup time remaining",                                                                                                      "hr",           "",                                  "System Control",                           "",                                                                 "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "pc_startup_energy_remain_final",     "Final cycle startup energy remaining",                                                                                                    "kwh",          "",                                  "System Control",                           "",                                                                 "",              "" },
        // Thermal energy storage
    { SSC_OUTPUT,    SSC_ARRAY,  "hot_tank_htf_percent_final",         "Final percent fill of available hot tank mass",                                                                                           "%",            "",                                  "System Control",                           "",                                                                 "",              "" },
        // Cycle off-design perfomance tables - for external controls
    { SSC_OUTPUT,    SSC_MATRIX, "cycle_eff_load_table",               "Cycle efficiency vs. thermal load",                                                                                                       "",             "",                                  "",                                         "sim_type=1",                                                       "",              "COL_LABEL=THERMLOAD_EFFICIENCY,ROW_LABEL=NO_ROW_LABEL" },
    { SSC_OUTPUT,    SSC_MATRIX, "cycle_Tdb_table",                    "Normalized cycle efficiency and condenser power vs. ambient temperature",                                                                 "",             "",                                  "",                                         "sim_type=1",                                                       "",              "COL_LABEL=AMBTEMP_EFF_CONDPOW,ROW_LABEL=NO_ROW_LABEL" },

    var_info_invalid };


bool SortByDouble(const pair<int, double>& lhs,
    const pair<int, double>& rhs);

class cm_tcsmolten_salt : public compute_module
{
public:

    cm_tcsmolten_salt()
    {
        add_var_info(_cm_vtab_tcsmolten_salt);
        add_var_info(vtab_adjustment_factors);
        add_var_info(vtab_sf_adjustment_factors);
        add_var_info(vtab_technology_outputs);
    } 

    bool relay_message(string &msg, double percent)
    {
        log(msg);
        return update(msg, (float)percent);
    }

	void exec() override
	{
        std::clock_t clock_start = std::clock();

        int sim_type = as_integer("sim_type");      // 1 (default): timeseries, 2: design only

        bool is_dispatch = as_boolean("is_dispatch");

        // *****************************************************
        // Check deprecated variables
            // Piping loss coefficient
        bool is_piping_loss_assigned = is_assigned("piping_loss");
        bool is_piping_loss_coefficient_assigned = is_assigned("piping_loss_coefficient");

        if (is_piping_loss_assigned && is_piping_loss_coefficient_assigned) {
            log("We replaced the functionality of input variable piping_loss with new input variable piping_loss_coefficient,"
                " so the model does not use your piping_loss input.");
        }
        else if (is_piping_loss_assigned) {
            throw exec_error("tcsmolten_salt", "We replaced the functionality of input variable piping_loss [Wt/m] with new input variable piping_loss_coefficient [Wt/m2-K]."
                " The new input scales piping thermal losses as a function of receiver thermal power and design-point temperatures."
                " Please define piping_loss_coefficient in your script.");
        }

        bool is_csp_pt_tes_init_hot_htf_percent_assigned = is_assigned("csp.pt.tes.init_hot_htf_percent");
        bool is_tes_hot_htf_percent_assigned = is_assigned("tes_hot_htf_percent_assigned");
        if (is_csp_pt_tes_init_hot_htf_percent_assigned && is_tes_hot_htf_percent_assigned) {
            log("We renamed input variable csp.pt.tes.init_hot_htf_percent to tes_init_hot_htf_percent,"
                " you provided both inputs, so the model does not use your csp.pt.tes.init_hot_htf_percent input.");
        }
        else if (is_csp_pt_tes_init_hot_htf_percent_assigned) {
            throw exec_error("tcsmolten_salt", "We renamed input variable csp.pt.tes.init_hot_htf_percent to tes_init_hot_htf_percent,"
                " please define tes_init_hot_htf_percent in your script.");
        }

        if (is_assigned("P_boil")) {
            log("We removed boiler pressure (P_boil) as a user input to the Rankine Cycle model. Because the cycle efficiency"
                " is provided by the user, the boiler pressure input does not modify the efficiency as one might expect. Instead the model"
                " uses boiler pressure in second order calculations to 1) define a boiling temperature to normalize off-design HTF temperature and"
                " 2) estimate steam mass flow for cycle make-up water calculations. Because boiler pressure only has influences"
                " results in these minor non-intuitive ways, we decided to hardcode the valu to 100 bar.");
        }

        if (is_dispatch && sim_type == 1) {
            // Cycle startup cost disp_csu_cost
            bool is_disp_csu_cost_assigned = is_assigned("disp_csu_cost");
            bool is_disp_csu_cost_rel_assigned = is_assigned("disp_csu_cost_rel");

            if (is_disp_csu_cost_assigned && is_disp_csu_cost_rel_assigned) {
                log("We replaced the functionality of input variable disp_csu_cost with new input variable disp_csu_cost_rel,"
                    " so the model does not use your disp_csu_cost input.");
            }
            else if (is_disp_csu_cost_assigned) {
                throw exec_error("tcsmolten_salt", "We replaced the functionality of input variable disp_csu_cost [$/start] with new input variable disp_csu_cost_rel [$/MWe-cycle/start]."
                    " The new input represents cycle startup costs normalized by the cycle design capacity."
                    " Please define disp_csu_cost_rel in your script.");
            }

            // Receiver startup cost disp_rsu_cost
            bool is_disp_rsu_cost_assigned = is_assigned("disp_rsu_cost");
            bool is_disp_rsu_cost_rel_assigned = is_assigned("disp_rsu_cost_rel");

            if (is_disp_rsu_cost_assigned && is_disp_rsu_cost_rel_assigned) {
                log("We replaced the funcationality of input variable disp_rsu_cost with new input variable disp_rsu_cost_rel,"
                    " so the model does not use your disp_rsu_cost input.");
            }
            else if (is_disp_rsu_cost_assigned) {
                throw exec_error("tcsmolten_salt", "We replaced the functionality of input variable disp_rsu_cost [$/start] with new input variable disp_rsu_cost_rel [$/MWe-cycle/start]."
                    " The new input represents receiver startup costs normalized by the receiver design thermal power."
                    " Please define disp_rsu_cost_rel in your script.");
            }

            // Cycle ramping
            bool is_disp_pen_delta_w_assigned = is_assigned("disp_pen_delta_w");
            bool is_disp_pen_ramping_assigned = is_assigned("disp_pen_ramping");
            if (is_disp_pen_delta_w_assigned && is_disp_pen_ramping_assigned) {
                log("We replaced the functionality of input variable disp_pen_delta_w with new input variable disp_pen_ramping,"
                    " so the model does not use your disp_pen_delta_w input");
            }
            else if (is_disp_pen_delta_w_assigned) {
                throw exec_error("tcsmolten_salt", "We replaced the functionality of input variable disp_pen_delta_w [$/kWe-change] with new input variable disp_pen_ramping [$/MWe-change]."
                    " The new input represents the receiver startup costs in an optimization model that uses absolute grid prices."
                    " Please define disp_pen_ramping in your script. SAM's default value in the molten salt power tower model is 1.0");
            }
        }

        // *****************************************************
        // *****************************************************


        // *****************************************************
        // System Design Parameters
        double W_dot_cycle_des = as_double("P_ref");            //[MWe]
        double eta_cycle = as_double("design_eff");             //[-]

        // System Design Calcs
        double q_dot_pc_des = W_dot_cycle_des / eta_cycle;      //[MWt]
        double solar_mult = as_number("solarm");                //[-]
        double q_dot_rec_des = q_dot_pc_des * solar_mult;       //[MWt]

        bool is_calc_sm = as_boolean("is_calc_sm");
        if (is_calc_sm) {
            q_dot_rec_des = as_double("q_dot_rec_des_target");  //[MWt]
            solar_mult = q_dot_rec_des / q_dot_pc_des;          //[-]
        }

        // Weather reader
		C_csp_weatherreader weather_reader;
		if (is_assigned("solar_resource_file")){
			weather_reader.m_weather_data_provider = make_shared<weatherfile>(as_string("solar_resource_file"));
			if (weather_reader.m_weather_data_provider->has_message()) log(weather_reader.m_weather_data_provider->message(), SSC_WARNING);
		}
		if (is_assigned("solar_resource_data")){
			weather_reader.m_weather_data_provider = make_shared<weatherdata>(lookup("solar_resource_data"));
			if (weather_reader.m_weather_data_provider->has_message()) log(weather_reader.m_weather_data_provider->message(), SSC_WARNING);
		}

        size_t n_steps_full = weather_reader.m_weather_data_provider->nrecords(); //steps_per_hour * 8760;
        weather_reader.m_trackmode = 0;
        weather_reader.m_tilt = 0.0;
        weather_reader.m_azimuth = 0.0;
        // Initialize to get weather file info
        weather_reader.init();
        if (weather_reader.has_error()) throw exec_error("tcsmolten_salt", weather_reader.get_error());

        int tes_type = 1;
        
        // 'sf_model_type'
        // 0 = design field and tower/receiver geometry
        // 1 = design field
        // 2 = user field, calculate performance
        // 3 = user performance maps vs solar position
        int field_model_type = as_integer("field_model_type");

        if (sim_type == 2 && field_model_type < 2) {
            field_model_type = 2;
        }

        int rec_type = as_integer("receiver_type");

        // Run solarpilot right away to update values as needed
        util::matrix_t<double> mt_eta_map;
        util::matrix_t<double> mt_flux_maps;

        int N_hel = -999;
        double A_sf = std::numeric_limits<double>::quiet_NaN();
        double THT = std::numeric_limits<double>::quiet_NaN();
        double rec_height = std::numeric_limits<double>::quiet_NaN();
        double cav_rec_height = std::numeric_limits<double>::quiet_NaN();
        double rec_aspect = std::numeric_limits<double>::quiet_NaN();
        double D_rec = std::numeric_limits<double>::quiet_NaN();
        double cav_rec_width = std::numeric_limits<double>::quiet_NaN();
        double land_area_base = std::numeric_limits<double>::quiet_NaN();
        double total_land_area_before_rad_cooling = std::numeric_limits<double>::quiet_NaN();
        double heliostat_area = std::numeric_limits<double>::quiet_NaN();
        double h_helio = std::numeric_limits<double>::quiet_NaN();     //[m]
        double average_attenuation = std::numeric_limits<double>::quiet_NaN();
        double refl_image_error = std::numeric_limits<double>::quiet_NaN();
        double land_max_abs = std::numeric_limits<double>::quiet_NaN();
        double land_min_abs = std::numeric_limits<double>::quiet_NaN();
        
        util::matrix_t<double> helio_pos;

        assign("is_optimize", 0);
        bool is_optimize = false;
        int n_flux_y = 1;

        if (field_model_type < 4) {

            h_helio = as_double("helio_height");     //[m]
            
            // Field types 0-3 require solar pilot
            solarpilot_invoke spi(this);

            assign("q_design", q_dot_rec_des);  //[MWt]

            // Set up "cmod_solarpilot.cpp" conversions as necessary
            double helio_optical_error_mrad = as_number("helio_optical_error_mrad");        //[mrad]
            refl_image_error = std::sqrt(2. * helio_optical_error_mrad * 2. * helio_optical_error_mrad * 2.);   //[mrad]
            assign("helio_optical_error", (ssc_number_t)(helio_optical_error_mrad * 1.E-3));

            // Set 'n_flux_x' and 'n_flux_y' here, for now
            assign("n_flux_y", n_flux_y);

            if (rec_type == 0) {
                assign("rec_aspect", as_number("rec_height") / as_number("D_rec"));
                int n_ext_rec_panels = as_integer("N_panels");
                assign("n_flux_x", (ssc_number_t)max(12, n_ext_rec_panels));
            }
            else if (rec_type == 1) {
                assign("n_flux_x", 2);  // n_flux_x represents *per panel* the number subsurfaces in x direction 
            }
            else {
                throw exec_error("tcsmolten_salt", "receiver_type must be 0 (external) or 1 (cavity)");
            }

            if ((field_model_type == 0 || field_model_type == 1) && sim_type == 1) // Auto-design. Generate a new system (is_optimize = true) or field layout
            {
                // What if sim_type = 2?
                // If from the UI, then probably never want to actually layout the field and geometry
                //   because there are macros for that
                // If called from a script.... then maybe?
                // So.. maybe add optional input like "is_layout_field_in_design_only" and default to False?

                if (field_model_type == 0) {
                    assign("is_optimize", 1);
                    is_optimize = true;
                }

                assign("calc_fluxmaps", 1);
                spi.run(weather_reader.m_weather_data_provider);

                if (is_optimize) {
                    //Optimization iteration history
                    // TODO (Bill): who owns "opt_history"? GUI? It doesn't appear anywhere...
                    vector<vector<double> > steps;
                    vector<double> obj, flux;
                    spi.getOptimizationSimulationHistory(steps, obj, flux);
                    size_t nr = steps.size();
                    if (nr > 0) {
                        size_t nc = steps.front().size() + 2;
                        ssc_number_t* ssc_hist = allocate("opt_history", nr, nc);   
                        for (size_t i = 0; i < nr; i++) {

                            for (size_t j = 0; j < steps.front().size(); j++)
                                ssc_hist[i * nc + j] = (ssc_number_t)steps.at(i).at(j);
                            ssc_hist[i * nc + nc - 2] = (ssc_number_t)obj.at(i);
                            ssc_hist[i * nc + nc - 1] = (ssc_number_t)flux.at(i);
                        }
                    }
                }
                //collect the optical efficiency data and sun positions
                spi.getHeliostatFieldEfficiency(mt_eta_map);

                //collect the flux map data
                spi.getReceiverFluxMaps(mt_flux_maps);
            }
            else if (field_model_type == 2 || field_model_type == 3)
            {
                // only calculates a flux map, so need to "assign" 'helio_positions_in' for SolarPILOT cmod
                util::matrix_t<double> helio_pos_temp = as_matrix("helio_positions");
                size_t n_h_rows = helio_pos_temp.nrows();
                ssc_number_t* p_helio_positions_in = allocate("helio_positions_in", n_h_rows, 2);

                // Try to determine whether heliostat positions represent surround or cavity field
                double y_h_min = 1.E5;
                double y_h_max = -1.E5;
                for (size_t i = 0; i < n_h_rows; i++)
                {
                    p_helio_positions_in[i * 2] = (ssc_number_t)helio_pos_temp(i, 0);       //[m] x
                    p_helio_positions_in[i * 2 + 1] = (ssc_number_t)helio_pos_temp(i, 1);   //[m] y

                    y_h_min = min(y_h_min, p_helio_positions_in[i * 2 + 1]);
                    y_h_max = max(y_h_max, p_helio_positions_in[i * 2 + 1]);
                }

                bool is_cavity_field = false;
                if ((y_h_max - y_h_min) / max(std::abs(y_h_max), std::abs(y_h_min)) < 1.25) {
                    is_cavity_field = true;
                }

                // Check determined field type against user-specified receiver type
                if (sim_type == 1) {
                    if (is_cavity_field && rec_type == 0) {
                        throw exec_error("mspt compute module", "\nExternal receiver specified, but cavity field detected. Try one of the following options:\n"
                            "1) Run field layout macro on Heliostat Field page\n"
                            "2) Select option for simulation to layout field and tower/receiver design\n"
                            "3) Enter new heliostat positions\n");
                    }

                    if (!is_cavity_field && rec_type == 1) {
                        throw exec_error("mspt compute module", "\nCavity receiver specified, but surround field detected. Try one of the following options:\n"
                            "1) Run field layout macro on Heliostat Field page\n"
                            "2) Select option for simulation to layout field and tower/receiver design\n"
                            "3) Enter new heliostat positions\n");
                    }
                }

                // 'calc_fluxmaps' defaults to false in solarpilot cmod, so overwrite here if we want flux maps
                if (sim_type == 1 && field_model_type == 2) {
                    assign("calc_fluxmaps", 1);
                }
                else if(sim_type == 2 || field_model_type == 3) {
                    assign("calc_fluxmaps", 0);
                }

                spi.run(weather_reader.m_weather_data_provider);

                if (sim_type == 1 && field_model_type == 2) {
                    //collect the optical efficiency data and sun positions
                    spi.getHeliostatFieldEfficiency(mt_eta_map);

                    //collect the flux map data
                    spi.getReceiverFluxMaps(mt_flux_maps);
                }
                else if (field_model_type == 3) {

                    mt_eta_map = as_matrix("eta_map");
                    util::matrix_t<double> raw_flux_maps = as_matrix("flux_maps");
                    importFluxMaps(raw_flux_maps, &mt_flux_maps);
                }
                else if (field_model_type == 2 && sim_type == 2) {

                    mt_eta_map.resize_fill(1, 3, std::numeric_limits<double>::quiet_NaN());
                    mt_flux_maps.resize_fill(1, 12, std::numeric_limits<double>::quiet_NaN());

                }
                else{
                    string msg = util::format("Invalid combination of field_model_type and sim_type");

                    throw exec_error("MSPT CSP Solver", msg);
                }
            }

            N_hel = (int)spi.layout.heliostat_positions.size();

            helio_pos.resize(N_hel, 2);
            for (int i = 0; i < N_hel; i++) {
                helio_pos(i, 0) = (ssc_number_t)spi.layout.heliostat_positions.at(i).location.x;
                helio_pos(i, 1) = (ssc_number_t)spi.layout.heliostat_positions.at(i).location.y;
            }

            THT = spi.sf.tht.val;

            if (rec_type == 0) {
                rec_height = spi.recs.front().rec_height.val;
                rec_aspect = spi.recs.front().rec_aspect.Val();
                D_rec = rec_height / rec_aspect;        //[-]
            }
            else if (rec_type == 1) {
                cav_rec_height = spi.recs.front().rec_height.val;

                // copied from cmod_solarpilot, would be nice to consolidate there
                double cav_rec_height_spout, cav_radius_spout, f_offset_spout;
                cav_rec_height_spout = cav_radius_spout = f_offset_spout = std::numeric_limits<double>::quiet_NaN();
                int n_panels_spout = -1;

                cav_rec_height_spout = spi.recs.front().rec_height.val;   //[m]
                cav_radius_spout = spi.recs.front().rec_cav_rad.val;      //[m]
                f_offset_spout = spi.recs.front().rec_cav_cdepth.val;       //[-]
                n_panels_spout = spi.recs.front().n_panels.val;           //[-]

                double theta0_calc, panelSpan_calc, panel_width_calc, rec_area_calc,
                    rec_span_calc, offset_calc;

                cavity_receiver_helpers::calc_receiver_macro_geometry_sp_inputs(cav_rec_height_spout, cav_radius_spout,
                    f_offset_spout, n_panels_spout,
                    theta0_calc, panelSpan_calc, panel_width_calc,
                    rec_area_calc, cav_rec_width, rec_span_calc, offset_calc);

            }

            A_sf = spi.CalcSolarFieldArea(N_hel);
            heliostat_area = spi.CalcHeliostatArea();
            average_attenuation = spi.CalcAveAttenuation();

            land_min_abs = as_double("land_min") * THT;     //[m]
            land_max_abs = as_double("land_max") * THT;     //[m]

            total_land_area_before_rad_cooling = spi.GetTotalLandArea();    // [acres] Total land area
            land_area_base = spi.GetBaseLandArea();             // [acres] Land area occupied by heliostats

        }
        else if (field_model_type == 4)
        {
            // Use input flux and efficiency maps
            mt_eta_map = as_matrix("eta_map");
            util::matrix_t<double> raw_flux_maps = as_matrix("flux_maps");
            importFluxMaps(raw_flux_maps, &mt_flux_maps);

            // Need to specify:
            // 1) reflective area (scale flux map)
            // 2) number heliostats for heliostats (tracking parasitics)
            // 3) total land area before radiative cooling
            // 4) tower and receiver dimensions
            N_hel = as_number("N_hel");
            A_sf = as_number("A_sf_in");        //[m2]
            total_land_area_before_rad_cooling = as_double("total_land_area_before_rad_cooling_in");

            // Get tower/receiver dimensions through cmod
            THT = as_double("h_tower");             //[m]
            h_helio = 0.0;                          //[m] Need a finite value for cost model

            if (rec_type == 0) {
                rec_height = as_double("rec_height");   //[m]
                D_rec = as_double("D_rec");             //[m]
                rec_aspect = rec_height / D_rec;        //[-]
            }
            else if (rec_type == 1) {
                cav_rec_height = as_double("cav_rec_height");
                cav_rec_width = as_double("cav_rec_width");
            }

            helio_pos.resize_fill(1, 2, std::numeric_limits<double>::quiet_NaN());

            // Don't define land_area_base because we're not requiring it as an input in this field model type
            land_area_base = std::numeric_limits<double>::quiet_NaN();
        }
        else
        {
            string msg = util::format("One field performance modeling option must be set to True");

            throw exec_error("MSPT CSP Solver", msg);
        }
        

        if( tes_type != 1 )
        {
            throw exec_error("MSPT CSP Solver", "Thermocline thermal energy storage is not yet supported by the new CSP Solver and Dispatch Optimization models.\n");
        }

        
        // Set steps per hour
        C_csp_solver::S_sim_setup sim_setup;
        sim_setup.m_sim_time_start = as_double("time_start");       //[s] time at beginning of first time step
        sim_setup.m_sim_time_end = as_double("time_stop");          //[s] time at end of last time step
        
        int steps_per_hour = (int)as_double("time_steps_per_hour");     //[-]

        //if the number of steps per hour is not provided (=-1), then assign it based on the weather file step
        if( steps_per_hour < 0 ) {
            double sph_d = 3600. / weather_reader.m_weather_data_provider->step_sec();
            steps_per_hour = (int)( sph_d + 1.e-5 );
            if( (double)steps_per_hour != sph_d )
                throw spexception("The time step duration must be evenly divisible within an hour.");
        }

        size_t n_steps_fixed = (size_t)steps_per_hour * 8760;   //[-]
        if( as_boolean("vacuum_arrays") ) {
            n_steps_fixed = steps_per_hour * (size_t)( (sim_setup.m_sim_time_end - sim_setup.m_sim_time_start)/3600. );
        }
        sim_setup.m_report_step = 3600.0 / (double)steps_per_hour;  //[s]

        // ***********************************************
        // ***********************************************
        // Power cycle
        // ***********************************************
        // ***********************************************
        C_csp_power_cycle * p_csp_power_cycle;
        // Steam Rankine and User Defined power cycle classes
        C_pc_Rankine_indirect_224 rankine_pc;


        // Set pb_pump_coef logic for cycle
        bool is_calc_pb_pump_coef = as_boolean("is_calc_pb_pump_coef");
        double W_dot_htf_pump_target = std::numeric_limits<double>::quiet_NaN();
        if (is_calc_pb_pump_coef) {
            W_dot_htf_pump_target = as_double("W_dot_pb_pump_target");    //[MWe]
        }


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
            pc->m_htf_pump_coef = as_double("pb_pump_coef");
            pc->m_pc_fl = as_integer("rec_htf");                            // power cycle HTF is same as receiver HTF
            pc->m_pc_fl_props = as_matrix("field_fl_props");

            pc->m_is_calc_htf_pump_coef = is_calc_pb_pump_coef;
            pc->m_W_dot_htf_pump_target = W_dot_htf_pump_target;    //[MWe]

            // Check initialization variables
            pc->m_operating_mode_initial = C_csp_power_cycle::OFF;
            if (is_assigned("pc_op_mode_initial")) {
                pc->m_operating_mode_initial = (C_csp_power_cycle::E_csp_power_cycle_modes)as_integer("pc_op_mode_initial");
                if (is_assigned("pc_startup_time_remain_init")) {
                    pc->m_startup_time_remain_init = as_double("pc_startup_time_remain_init");
                }
                if (is_assigned("pc_startup_energy_remain_initial")) {
                    pc->m_startup_energy_remain_init = as_double("pc_startup_energy_remain_initial");
                }
            }

            if (pb_tech_type == 0)
            {
                pc->m_P_boil_des = 100;     //[bar]
                pc->m_dT_cw_ref = as_double("dT_cw_ref");
                pc->m_T_amb_des = as_double("T_amb_des");
                //pc->m_P_boil = as_double("P_boil");
                pc->m_CT = as_integer("CT");                    // cooling tech type: 1=evaporative, 2=air, 3=hybrid    , 5= custom for rad cool, 6= custom for rad cool
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

                //parameters for radiative cooling with cold storage
                C_csp_cold_tes *two_tank = &rankine_pc.mc_two_tank_ctes;    //pointer for two tank
                C_csp_stratified_tes *stratified = &rankine_pc.mc_stratified_ctes; //pointer for stratified
            
                two_tank->ms_params.m_ctes_type = as_integer("ctes_type");
                stratified->ms_params.m_ctes_type = as_integer("ctes_type");

                if (rankine_pc.ms_params.m_CT == 4)
                {
                    if (two_tank->ms_params.m_ctes_type == 2)
                    {
                        two_tank->ms_params.m_h_tank_min = as_double("h_ctes_tank_min");
                        two_tank->ms_params.m_ts_hours = as_double("ctes_tshours");
                        two_tank->ms_params.m_h_tank = as_double("h_ctes_tank");
                        two_tank->ms_params.m_u_tank = as_double("u_ctes_tank");
                        two_tank->ms_params.m_tank_pairs = as_integer("ctes_tankpairs");
                        two_tank->ms_params.m_T_cold_des = as_double("T_ctes_cold_design");
                        two_tank->ms_params.m_T_hot_des = as_double("T_ctes_warm_design");
                        two_tank->ms_params.m_T_tank_hot_ini = as_double("T_ctes_warm_ini");
                        two_tank->ms_params.m_T_tank_cold_ini = as_double("T_ctes_cold_ini");
                        two_tank->ms_params.m_f_V_hot_ini = as_double("f_ctes_warm_ini");
                        two_tank->ms_params.m_lat = weather_reader.ms_solved_params.m_lat;
                    }
                    if (two_tank->ms_params.m_ctes_type > 2)
                    {
                        stratified->ms_params.m_h_tank_min = 0;                             //hardcode zero minimum height for stratified tanks.
                        stratified->ms_params.m_ts_hours = as_double("ctes_tshours");
                        stratified->ms_params.m_h_tank = as_double("h_ctes_tank");
                        stratified->ms_params.m_u_tank = as_double("u_ctes_tank");
                        stratified->ms_params.m_tank_pairs = as_integer("ctes_tankpairs");
                        stratified->ms_params.m_T_cold_des = as_double("T_ctes_cold_design");
                        stratified->ms_params.m_T_hot_des = as_double("T_ctes_warm_design");
                        stratified->ms_params.m_T_tank_hot_ini = as_double("T_ctes_warm_ini");
                        stratified->ms_params.m_T_tank_cold_ini = as_double("T_ctes_cold_ini");
                        stratified->ms_params.m_f_V_hot_ini = as_double("f_ctes_warm_ini");
                        stratified->ms_params.m_lat = weather_reader.ms_solved_params.m_lat;

                    }
                    rankine_pc.mc_radiator.ms_params.m_field_fl = as_integer("ctes_field_fl");
                    rankine_pc.mc_radiator.ms_params.RM = as_double("rad_multiplier");
                    rankine_pc.mc_radiator.ms_params.Asolar_refl = A_sf;        //[m2]
                    rankine_pc.mc_radiator.ms_params.m_dot_panel = as_double("m_dot_radpanel");
                    rankine_pc.mc_radiator.ms_params.n = as_integer("n_rad_tubes");
                    rankine_pc.mc_radiator.ms_params.W = as_double("W_rad_tubes");
                    rankine_pc.mc_radiator.ms_params.L = as_double("L_rad");
                    rankine_pc.mc_radiator.ms_params.th = as_double("th_rad_panel");
                    rankine_pc.mc_radiator.ms_params.D = as_double("D_rad_tubes");
                    rankine_pc.mc_radiator.ms_params.k_panel = as_double("k_panel");
                    rankine_pc.mc_radiator.ms_params.epsilon = as_double("epsilon_radtop");
                    rankine_pc.mc_radiator.ms_params.epsilonb = as_double("epsilon_radbot");
                    rankine_pc.mc_radiator.ms_params.epsilong = as_double("epsilon_radgrnd");
                    rankine_pc.mc_radiator.ms_params.Lsec = as_double("L_rad_sections");
                    rankine_pc.mc_radiator.ms_params.epsilon_HX = as_double("epsilon_radHX");
                    rankine_pc.mc_radiator.ms_params.radfield_dp = as_double("rad_pressuredrop");
                }

                size_t n_F_wc = 0;
                ssc_number_t *p_F_wc = as_array("F_wc", &n_F_wc);
                pc->m_F_wc.resize(n_F_wc, 0.0);
                for (size_t i = 0; i < n_F_wc; i++)
                    pc->m_F_wc[i] = (double)p_F_wc[i];

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
        p_csp_power_cycle->assign(C_pc_Rankine_indirect_224::E_W_DOT_HTF_PUMP, allocate("cycle_htf_pump_power", n_steps_fixed), n_steps_fixed);
        p_csp_power_cycle->assign(C_pc_Rankine_indirect_224::E_W_DOT_COOLER, allocate("P_cooling_tower_tot", n_steps_fixed), n_steps_fixed);
        p_csp_power_cycle->assign(C_pc_Rankine_indirect_224::E_P_COND, allocate("P_cond", n_steps_fixed), n_steps_fixed);
        p_csp_power_cycle->assign(C_pc_Rankine_indirect_224::E_P_COND_ITER_ERR, allocate("P_cond_iter_err", n_steps_fixed), n_steps_fixed);
        p_csp_power_cycle->assign(C_pc_Rankine_indirect_224::E_ETA_THERMAL, allocate("eta", n_steps_fixed), n_steps_fixed);
        p_csp_power_cycle->assign(C_pc_Rankine_indirect_224::E_PC_OP_MODE_FINAL, allocate("pc_op_mode_final", n_steps_fixed), n_steps_fixed);
        p_csp_power_cycle->assign(C_pc_Rankine_indirect_224::E_PC_STARTUP_TIME_REMAIN_FINAL, allocate("pc_startup_time_remain_final", n_steps_fixed), n_steps_fixed);
        p_csp_power_cycle->assign(C_pc_Rankine_indirect_224::E_PC_STARTUP_ENERGY_REMAIN_FINAL, allocate("pc_startup_energy_remain_final", n_steps_fixed), n_steps_fixed);

        if (pb_tech_type == 0) {
            if (rankine_pc.ms_params.m_CT == 4) {
                p_csp_power_cycle->assign(C_pc_Rankine_indirect_224::E_T_COLD, allocate("T_cold", n_steps_fixed), n_steps_fixed);
                p_csp_power_cycle->assign(C_pc_Rankine_indirect_224::E_M_COLD, allocate("m_cold", n_steps_fixed), n_steps_fixed);
                p_csp_power_cycle->assign(C_pc_Rankine_indirect_224::E_M_WARM, allocate("m_warm", n_steps_fixed), n_steps_fixed);
                p_csp_power_cycle->assign(C_pc_Rankine_indirect_224::E_T_WARM, allocate("T_warm", n_steps_fixed), n_steps_fixed);
                p_csp_power_cycle->assign(C_pc_Rankine_indirect_224::E_T_RADOUT, allocate("T_rad_out", n_steps_fixed), n_steps_fixed);
                p_csp_power_cycle->assign(C_pc_Rankine_indirect_224::E_RADCOOL_CNTRL, allocate("radcool_control", n_steps_fixed), n_steps_fixed);
            }
        }

        //// *********************************************************
        //// *********************************************************
        //// *********************************************************
        ////      Now set Type 222 parameters
        //// *********************************************************
        //// *********************************************************
        //// *********************************************************

        double A_rec = std::numeric_limits<double>::quiet_NaN();
        double ext_rec_area = std::numeric_limits<double>::quiet_NaN();
        double ext_rec_aspect = std::numeric_limits<double>::quiet_NaN();
        double cav_rec_area = std::numeric_limits<double>::quiet_NaN();
        double cav_panel_width = std::numeric_limits<double>::quiet_NaN();
        double cav_radius = std::numeric_limits<double>::quiet_NaN();

        // Calculate external receiver area, height, diameter here
        // Calculate cavity receiver area and height below. Don't set diameter or aspect ratio for cavity receiver
        if(rec_type == 0){
            A_rec = rec_height * D_rec * 3.1415926;         //[m2]
            ext_rec_area = A_rec;                           //[m2]
            ext_rec_aspect = rec_height / D_rec;            //[-]
        }

        std::unique_ptr<C_pt_receiver> receiver;
        bool is_rec_model_trans = as_boolean("is_rec_model_trans");
        bool is_rec_model_clearsky = as_double("rec_clearsky_fraction") > 0.0;

        if (rec_type == 1) {    // Cavity receiver

            // No transient model available for cavity receiver
            is_rec_model_trans = false;

            // No clear sky model available for cavity receiver
            is_rec_model_clearsky = false;

            double od_rec_tube = as_double("d_tube_out");           //[mm]
            double th_rec_tube = as_double("th_tube");              //[mm]
            double rec_span = as_double("cav_rec_span")*CSP::pi/180.0;  //[rad] convert from cmod units of deg

            // lips must be 0 for now due to solarpilot limitations
            size_t nPanels = as_integer("n_cav_rec_panels");    //[-]
            double topLipHeight = 0;        //[m] Height of top lip in meters
            double botLipHeight = 0;        //[m] Height of bottom lip in meters

            // surface radiative properties
            double e_act_sol = as_double("rec_absorptance");        //[-] Absorptivity in short wave - active surfaces
            double e_act_therm = as_double("epsilon");              //[-] Emissivity in long wave - active surfaces
            double e_pass_sol = as_double("cav_rec_passive_abs");   //[-] Absorptivity in short wave (solar) - passive surfaces
            double e_pass_therm = as_double("cav_rec_passive_eps"); //[-] Emissivity in long wave - passive surfaces

            C_cavity_receiver::E_mesh_types active_surface_mesh_type = C_cavity_receiver::E_mesh_types::no_mesh;    // quad;   // no_mesh;    // quad;
            C_cavity_receiver::E_mesh_types floor_and_cover_mesh_type = C_cavity_receiver::E_mesh_types::no_mesh;
            C_cavity_receiver::E_mesh_types lips_mesh_type = C_cavity_receiver::E_mesh_types::no_mesh;  // quad;

            // Default values to test against matlab code
            // *************************************************************************************
            if (false) {
                // Method sets up, initializes, and runs the steady state model w/ inputs corresponding to matlab code
                cavity_receiver_helpers::test_cavity_case();
            }
            // ***************************************************************************************
            // ***************************************************************************************

            std::unique_ptr<C_cavity_receiver> c_cav_rec = std::unique_ptr<C_cavity_receiver>(new C_cavity_receiver(as_double("dni_des"),
                as_integer("rec_htf"), as_matrix("field_fl_props"),
                od_rec_tube, th_rec_tube, as_integer("mat_tube"),
                nPanels, cav_rec_height, cav_rec_width,
                rec_span, topLipHeight, botLipHeight,
                e_act_sol, e_pass_sol, e_act_therm, e_pass_therm,
                active_surface_mesh_type, floor_and_cover_mesh_type, lips_mesh_type,
                as_double("piping_loss_coefficient"), as_double("piping_length_const"), as_double("piping_length_mult"),
                THT, as_double("T_htf_hot_des"),
                as_double("T_htf_cold_des"), as_double("f_rec_min"), q_dot_rec_des,
                as_double("rec_su_delay"), as_double("rec_qf_delay"), as_double("csp.pt.rec.max_oper_frac"),
                as_double("eta_pump") ));

            receiver = std::move(c_cav_rec);

            // Calculate receiver area
            double theta0, panelspan, panelwidth, radius, offset;
            theta0 = panelspan = panelwidth = radius = offset = A_rec = std::numeric_limits<double>::quiet_NaN();
            cavity_receiver_helpers::calc_receiver_macro_geometry(cav_rec_height, cav_rec_width,
                                            rec_span, nPanels,
                                            theta0, panelspan, panelwidth, A_rec, radius, offset );

            cav_rec_area = A_rec;           //[m2]
            cav_panel_width = panelwidth;   //[m]
            cav_radius = radius;            //[m]
        }
        else if (rec_type == 0){

            int rec_night_recirc = 0;
            int rec_clearsky_model = as_integer("rec_clearsky_model");

            bool is_calc_od_tube = as_boolean("is_calc_od_tube");       //[-]
            double W_dot_rec_target = std::numeric_limits<double>::quiet_NaN();
            if (is_calc_od_tube) {
                W_dot_rec_target = as_double("W_dot_rec_target");    //[MWe]
            }

            if (rec_clearsky_model > 4)
                throw exec_error("tcsmolten_salt", "Invalid specification for 'rec_clearsky_model'");
            if (rec_clearsky_model == -1 && as_double("rec_clearsky_fraction") >= 0.0001)
                throw exec_error("tcsmolten_salt", "'rec_clearsky_model' must be specified when 'rec_clearsky_fraction' > 0.0.");

            if (!as_boolean("is_rec_model_trans") && !as_boolean("is_rec_startup_trans")) {
                //std::unique_ptr<C_mspt_receiver_222> ss_receiver = std::make_unique<C_mspt_receiver_222>();   // new to C++14
                std::unique_ptr<C_mspt_receiver_222> ss_receiver = std::unique_ptr<C_mspt_receiver_222>(new C_mspt_receiver_222(
                    THT, as_double("epsilon"),
                    as_double("T_htf_hot_des"), as_double("T_htf_cold_des"),
                    as_double("f_rec_min"), q_dot_rec_des,
                    as_double("rec_su_delay"), as_double("rec_qf_delay"),
                    as_double("csp.pt.rec.max_oper_frac"), as_double("eta_pump"),
                    as_double("d_tube_out"), as_double("th_tube"),
                    as_double("piping_loss_coefficient"), as_double("piping_length_const"),
                    as_double("piping_length_mult"),
                    as_integer("rec_htf"), as_matrix("field_fl_props"),
                    as_integer("mat_tube"),
                    rec_night_recirc,
                    as_integer("N_panels"), D_rec, rec_height,
                    as_integer("Flow_type"), as_integer("crossover_shift"), as_double("hl_ffact"),
                    as_double("T_htf_hot_des"), as_double("rec_clearsky_fraction"),
                    is_calc_od_tube, W_dot_rec_target
                    ));   // steady-state receiver

                // Set initial state
                if (is_assigned("rec_op_mode_initial")
                    && is_assigned("rec_startup_energy_remain_init")
                    && is_assigned("rec_startup_time_remain_init")) {
                    ss_receiver->set_inital_state(
                        (C_csp_collector_receiver::E_csp_cr_modes)as_integer("rec_op_mode_initial"),
                        as_double("rec_startup_energy_remain_init"),
                        as_double("rec_startup_time_remain_init"));
                }
                else if (is_assigned("rec_op_mode_initial")
                    && (!is_assigned("rec_startup_energy_remain_init")
                        || !is_assigned("rec_startup_time_remain_init"))) {
                    throw exec_error("tcsmolten_salt", "Receiver inital mode was given but not startup time and/or energy.");
                }
                
                receiver = std::move(ss_receiver);
            }
            else {

                bool is_enforce_min_startup = as_boolean("is_rec_enforce_min_startup");

                //trans_receiver->m_is_startup_from_solved_profile = as_boolean("is_rec_startup_from_T_soln");
                if (as_boolean("is_rec_startup_trans") && as_boolean("is_rec_startup_from_T_soln"))
                    throw exec_error("tcsmolten_salt", "Receiver startup from solved temperature profiles is only available when receiver transient startup model is enabled");

                //trans_receiver->m_is_enforce_min_startup = as_boolean("is_rec_enforce_min_startup");
                if (as_boolean("is_rec_startup_trans") && !as_boolean("is_rec_startup_from_T_soln") && !is_enforce_min_startup)
                {
                    log("Both 'is_rec_enforce_min_startup' and 'is_rec_startup_from_T_soln' were set to 'false'. Minimum startup time will always be enforced unless 'is_rec_startup_from_T_soln' is set to 'true'", SSC_WARNING);
                    is_enforce_min_startup = true;
                }

                std::unique_ptr<C_mspt_receiver> trans_receiver = std::unique_ptr<C_mspt_receiver>(new C_mspt_receiver(
                    THT, as_double("epsilon"),
                    as_double("T_htf_hot_des"), as_double("T_htf_cold_des"),
                    as_double("f_rec_min"), q_dot_rec_des,
                    as_double("rec_su_delay"), as_double("rec_qf_delay"),
                    as_double("csp.pt.rec.max_oper_frac"), as_double("eta_pump"),
                    as_double("d_tube_out"), as_double("th_tube"),
                    as_double("piping_loss_coefficient"), as_double("piping_length_const"),
                    as_double("piping_length_mult"),
                    as_integer("rec_htf"), as_matrix("field_fl_props"),
                    as_integer("mat_tube"),
                    rec_night_recirc,
                    as_integer("N_panels"), D_rec, rec_height,
                    as_integer("Flow_type"), as_integer("crossover_shift"), as_double("hl_ffact"),
                    as_double("T_htf_hot_des"), as_double("rec_clearsky_fraction"),
                    is_calc_od_tube, W_dot_rec_target,
                    as_boolean("is_rec_model_trans"), as_boolean("is_rec_startup_trans"),
                    as_double("rec_tm_mult"), as_double("u_riser"),
                    as_double("th_riser"), as_double("riser_tm_mult"),
                    as_double("downc_tm_mult"), as_double("heat_trace_power"),
                    as_double("preheat_flux"), as_double("min_preheat_time"),
                    as_double("min_fill_time"), as_double("startup_ramp_time"),
                    as_double("T_htf_cold_des"), min(0.0, as_double("startup_target_Tdiff")),
                    5.0,
                    as_boolean("is_rec_startup_from_T_soln"), is_enforce_min_startup
                    ));    // transient receiver

                // Set initial state
                if (is_assigned("rec_op_mode_initial")
                    && is_assigned("rec_startup_energy_remain_init")
                    && is_assigned("rec_startup_time_remain_init")) {
                    trans_receiver->set_inital_state(
                        (C_csp_collector_receiver::E_csp_cr_modes)as_integer("rec_op_mode_initial"),
                        as_double("rec_startup_energy_remain_init"),
                        as_double("rec_startup_time_remain_init"));
                }
                else if (is_assigned("rec_op_mode_initial")
                    && (!is_assigned("rec_startup_energy_remain_init")
                        || !is_assigned("rec_startup_time_remain_init"))) {
                    throw exec_error("tcsmolten_salt", "Receiver inital mode was given but not startup time and/or energy.");
                }

                receiver = std::move(trans_receiver);
            }
        }

        // Test mspt_receiver initialization
        //receiver.init();

        // *******************************
        // *******************************
        // Construct heliostat field class after receiver
        //    so it can use the active receiver area
        C_pt_sf_perf_interp heliostatfield(A_rec);

        heliostatfield.ms_params.m_p_start = as_double("p_start");      //[kWe-hr] Heliostat startup energy
        heliostatfield.ms_params.m_p_track = as_double("p_track");      //[kWe] Heliostat tracking power
        heliostatfield.ms_params.m_hel_stow_deploy = as_double("hel_stow_deploy");  // N/A
        heliostatfield.ms_params.m_v_wind_max = as_double("v_wind_max");            // N/A
        heliostatfield.ms_params.m_eta_map = mt_eta_map;
        heliostatfield.ms_params.m_flux_maps = mt_flux_maps;
        heliostatfield.ms_params.m_n_flux_x = mt_flux_maps.ncols();     // for multi-surface cav receiver, these values need to match
        heliostatfield.ms_params.m_n_flux_y = n_flux_y;
        heliostatfield.ms_params.m_N_hel = N_hel;
        heliostatfield.ms_params.m_A_sf = A_sf;        //[m2]
        if (field_model_type < 3)
        {
            heliostatfield.ms_params.m_eta_map_aod_format = false;
        }
        else
        {
            heliostatfield.ms_params.m_eta_map_aod_format = as_boolean("eta_map_aod_format");
        }
        heliostatfield.ms_params.m_clearsky_model = as_integer("rec_clearsky_model");

        std::vector<double> clearsky_data;
        if (heliostatfield.ms_params.m_clearsky_model == 0)
        {
            size_t n_csky = 0;
            ssc_number_t* csky = as_array("rec_clearsky_dni", &n_csky);
            if (n_csky != n_steps_full)
                throw exec_error("tcsmolten_salt", "Invalid clear-sky DNI data. Array must have " + util::to_string((int)n_steps_full) + " rows.");

            clearsky_data.resize(n_steps_full);
            for (size_t i = 0; i < n_steps_full; i++)
                clearsky_data.at(i) = (double)csky[i];
        }
        heliostatfield.ms_params.mv_clearsky_data = clearsky_data;

        //Load the solar field adjustment factors
        adjustment_factors sf_haf(this->get_var_table(), "sf_adjust");
        if (!sf_haf.setup((int)n_steps_full))
            throw exec_error("tcsmolten_salt", "failed to setup sf adjustment factors: " + sf_haf.error());
        //allocate array to pass to tcs
        heliostatfield.ms_params.m_sf_adjust.resize(sf_haf.size());
        for (size_t i = 0; i < sf_haf.size(); i++)
            heliostatfield.ms_params.m_sf_adjust.at(i) = sf_haf(i);

        // Set callback information
        heliostatfield.mf_callback = ssc_cmod_solarpilot_callback;
        heliostatfield.m_cdata = (void*)this;

        // Check initialization variable
        if (is_assigned("is_field_tracking_init")) {
            heliostatfield.ms_params.m_is_field_tracking_init = as_boolean("is_field_tracking_init");
        }
        else {
            heliostatfield.ms_params.m_is_field_tracking_init = false;
        }

        // Try running pt heliostat init() call just for funsies
            // What happens when no callback to reference?
        //heliostatfield.init();


        // Now try to instantiate mspt_collector_receiver
        C_csp_mspt_collector_receiver collector_receiver(heliostatfield, *receiver);
        // Then try init() call here, which should call inits from both classes
        //collector_receiver.init();

        // *******************************************************
        // *******************************************************
        // Set receiver outputs
        collector_receiver.mc_reported_outputs.assign(C_csp_mspt_collector_receiver::E_FIELD_Q_DOT_INC, allocate("q_sf_inc", n_steps_fixed), n_steps_fixed);
        collector_receiver.mc_reported_outputs.assign(C_csp_mspt_collector_receiver::E_FIELD_ETA_OPT, allocate("eta_field", n_steps_fixed), n_steps_fixed);
        collector_receiver.mc_reported_outputs.assign(C_csp_mspt_collector_receiver::E_FIELD_ADJUST, allocate("sf_adjust_out", n_steps_fixed), n_steps_fixed);
        collector_receiver.mc_reported_outputs.assign(C_csp_mspt_collector_receiver::E_IS_FIELD_TRACKING_FINAL, allocate("is_field_tracking_final", n_steps_fixed), n_steps_fixed);
        collector_receiver.mc_reported_outputs.assign(C_csp_mspt_collector_receiver::E_REC_OP_MODE_FINAL, allocate("rec_op_mode_final", n_steps_fixed), n_steps_fixed);
        collector_receiver.mc_reported_outputs.assign(C_csp_mspt_collector_receiver::E_REC_STARTUP_TIME_REMAIN_FINAL, allocate("rec_startup_time_remain_final", n_steps_fixed), n_steps_fixed);
        collector_receiver.mc_reported_outputs.assign(C_csp_mspt_collector_receiver::E_REC_STARTUP_ENERGY_REMAIN_FINAL, allocate("rec_startup_energy_remain_final", n_steps_fixed), n_steps_fixed);

        collector_receiver.mc_reported_outputs.assign(C_csp_mspt_collector_receiver::E_REC_DEFOCUS, allocate("rec_defocus", n_steps_fixed), n_steps_fixed);
        collector_receiver.mc_reported_outputs.assign(C_csp_mspt_collector_receiver::E_Q_DOT_INC, allocate("q_dot_rec_inc", n_steps_fixed), n_steps_fixed);
        collector_receiver.mc_reported_outputs.assign(C_csp_mspt_collector_receiver::E_ETA_THERMAL, allocate("eta_therm", n_steps_fixed), n_steps_fixed);
        collector_receiver.mc_reported_outputs.assign(C_csp_mspt_collector_receiver::E_Q_DOT_THERMAL, allocate("Q_thermal", n_steps_fixed), n_steps_fixed);
        collector_receiver.mc_reported_outputs.assign(C_csp_mspt_collector_receiver::E_M_DOT_HTF, allocate("m_dot_rec", n_steps_fixed), n_steps_fixed);
        collector_receiver.mc_reported_outputs.assign(C_csp_mspt_collector_receiver::E_Q_DOT_STARTUP, allocate("q_startup", n_steps_fixed), n_steps_fixed);
        collector_receiver.mc_reported_outputs.assign(C_csp_mspt_collector_receiver::E_T_HTF_IN, allocate("T_rec_in", n_steps_fixed), n_steps_fixed);
        collector_receiver.mc_reported_outputs.assign(C_csp_mspt_collector_receiver::E_T_HTF_OUT, allocate("T_rec_out", n_steps_fixed), n_steps_fixed);
        collector_receiver.mc_reported_outputs.assign(C_csp_mspt_collector_receiver::E_Q_DOT_PIPE_LOSS, allocate("q_piping_losses", n_steps_fixed), n_steps_fixed);
        collector_receiver.mc_reported_outputs.assign(C_csp_mspt_collector_receiver::E_Q_DOT_LOSS, allocate("q_thermal_loss", n_steps_fixed), n_steps_fixed);
            // Cavity-specific outputs
        if (rec_type == 1) {
            collector_receiver.mc_reported_outputs.assign(C_csp_mspt_collector_receiver::E_Q_DOT_REFL_LOSS, allocate("q_dot_reflection_loss", n_steps_fixed), n_steps_fixed);
        }
        collector_receiver.mc_reported_outputs.assign(C_csp_mspt_collector_receiver::E_W_DOT_TRACKING, allocate("pparasi", n_steps_fixed), n_steps_fixed);
        collector_receiver.mc_reported_outputs.assign(C_csp_mspt_collector_receiver::E_W_DOT_PUMP, allocate("P_tower_pump", n_steps_fixed), n_steps_fixed);

            // Transient model specific outputs
        if (is_rec_model_trans) {
            collector_receiver.mc_reported_outputs.assign(C_csp_mspt_collector_receiver::E_P_HEATTRACE, allocate("P_rec_heattrace", n_steps_fixed), n_steps_fixed);
            collector_receiver.mc_reported_outputs.assign(C_csp_mspt_collector_receiver::E_T_HTF_OUT_END, allocate("T_rec_out_end", n_steps_fixed), n_steps_fixed);
            collector_receiver.mc_reported_outputs.assign(C_csp_mspt_collector_receiver::E_T_HTF_OUT_MAX, allocate("T_rec_out_max", n_steps_fixed), n_steps_fixed);
            collector_receiver.mc_reported_outputs.assign(C_csp_mspt_collector_receiver::E_T_HTF_PANEL_OUT_MAX, allocate("T_panel_out_max", n_steps_fixed), n_steps_fixed);

            collector_receiver.mc_reported_outputs.assign(C_csp_mspt_collector_receiver::E_T_WALL_INLET, allocate("T_wall_rec_inlet", n_steps_fixed), n_steps_fixed);
            collector_receiver.mc_reported_outputs.assign(C_csp_mspt_collector_receiver::E_T_WALL_OUTLET, allocate("T_wall_rec_outlet", n_steps_fixed), n_steps_fixed);
            collector_receiver.mc_reported_outputs.assign(C_csp_mspt_collector_receiver::E_T_RISER, allocate("T_wall_riser", n_steps_fixed), n_steps_fixed);
            collector_receiver.mc_reported_outputs.assign(C_csp_mspt_collector_receiver::E_T_DOWNC, allocate("T_wall_downcomer", n_steps_fixed), n_steps_fixed);

            collector_receiver.mc_reported_outputs.assign(C_csp_mspt_collector_receiver::E_Q_DOT_THERMAL_SS, allocate("Q_thermal_ss", n_steps_fixed), n_steps_fixed);
        }
        if (is_rec_model_clearsky) {
            collector_receiver.mc_reported_outputs.assign(C_csp_mspt_collector_receiver::E_CLEARSKY, allocate("clearsky", n_steps_fixed), n_steps_fixed);
            collector_receiver.mc_reported_outputs.assign(C_csp_mspt_collector_receiver::E_Q_DOT_THERMAL_CSKY_SS, allocate("Q_thermal_ss_csky", n_steps_fixed), n_steps_fixed);
        }

        // Check if system configuration includes a heater parallel to primary collector receiver
        C_csp_collector_receiver* p_heater;
        C_csp_cr_electric_resistance* p_electric_resistance = NULL;
        bool is_parallel_heater = as_boolean("is_parallel_htr");    // defaults to false
        double q_dot_heater_des = 0.0;  //[MWt]
        double heater_spec_cost = 0.0;
        if (is_parallel_heater) {

            if (!is_dispatch && sim_type == 1) {
                if (!as_boolean("allow_heater_no_dispatch_opt")) {
                    throw exec_error("tcsmolten_salt", "When the molten salt power tower case has an electric HTF charger, dispatch optimization must be selected");
                }
            }

            double heater_mult = as_double("heater_mult");      //[-]
            heater_spec_cost = as_double("heater_spec_cost");   //[$/kWt]

            q_dot_heater_des = q_dot_pc_des*heater_mult;     //[MWt]
            //double q_dot_heater_des = receiver->m_q_rec_des * 2.0;  // / 4.0;      //[MWt]

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
            0.0,                                    // MSPT assumes direct storage, so no user input here: hardcode = 0.0
            as_double("T_htf_cold_des"),
            as_double("T_htf_hot_des"),
            // Check initialization variables
            (is_assigned("T_tank_hot_init")) ? as_double("T_tank_hot_init") : as_double("T_htf_hot_des"),
            (is_assigned("T_tank_cold_init")) ? as_double("T_tank_cold_init") : as_double("T_htf_cold_des"),
            as_double("h_tank_min"),
            as_double("tes_init_hot_htf_percent"),
            as_double("pb_pump_coef"),
            as_boolean("tanks_in_parallel"),        //[-]
            1.0,                                    //[-] packed volume fraction
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
        storage.mc_reported_outputs.assign(C_csp_two_tank_tes::E_W_DOT_HTF_PUMP, allocate("tes_htf_pump_power", n_steps_fixed), n_steps_fixed);
        storage.mc_reported_outputs.assign(C_csp_two_tank_tes::E_HOT_TANK_HTF_PERC_FINAL, allocate("hot_tank_htf_percent_final", n_steps_fixed), n_steps_fixed);

        // *************************************************************************
        // Schedules

        // Off-taker schedule
        C_timeseries_schedule_inputs offtaker_schedule;
        bool assigned_is_timestep_fractions = is_assigned("is_timestep_load_fractions");
        bool is_timestep_load_fractions = false;
        if (assigned_is_timestep_fractions) {
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
                    throw exec_error("tcsmolten_salt", "\n\nYou selected dispatch optimization which requires that the array input ppa_price_input is defined\n");
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
                    throw exec_error("tcsmolten_salt", "\n\nYou selected dispatch optimization and the Specify IRR Target financial solution mode, "
                        "but dispatch optimization requires known absolute electricity prices. Dispatch optimization requires "
                        "the Specify PPA Price financial solution mode. You can continue using dispatch optimization and iteratively "
                        "solve for the PPA that results in a target IRR by running a SAM Parametric analysis or script.\n");
                }

                int en_electricity_rates = as_integer("en_electricity_rates");  // 0 = Use PPA, 1 = Use Retail
                if (en_electricity_rates == 1 && is_dispatch) {
                    throw exec_error("tcsmolten_salt", "\n\nYou selected dispatch optimization and the option to Use Retail Electricity Rates on the Electricity Purchases page, "
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
                        throw exec_error("tcsmolten_salt", ppa_msg);
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
            else if (csp_financial_model == 8) {        // No Financial Model
                if (is_dispatch) {
                    throw exec_error("tcsmolten_salt", "Can't select dispatch optimization if No Financial model");
                }
                else { // if no dispatch optimization, don't need an input pricing schedule
                    // If electricity pricing data is not available, then dispatch to a uniform schedule
                    elec_pricing_schedule = C_timeseries_schedule_inputs(1.0, std::numeric_limits<double>::quiet_NaN());
                }
            }
            else {
                throw exec_error("tcsmolten_salt", "csp_financial_model must be 1, 2, 3, 4, or 6");
            }
        }
        else if (sim_type == 2) {

            elec_pricing_schedule = C_timeseries_schedule_inputs(1.0, std::numeric_limits<double>::quiet_NaN());
        }
        // *****************************************************
        //

        // Figure out dispatch model type
        // User-specified dispatch targets (specified at weather-file resolution)
        bool is_dispatch_targets = as_boolean("is_dispatch_targets");
        if (is_dispatch_targets && is_dispatch) {
            log("Both 'is_dispatch' and 'is_dispatch_targets' were set to true. Plant dispatch will be optimized and all user-specified dispatch target arrays will be ignored", SSC_WARNING);
            is_dispatch_targets = false;
        }

        C_csp_tou::C_dispatch_model_type::E_dispatch_model_type dispatch_model_type = C_csp_tou::C_dispatch_model_type::E_dispatch_model_type::UNDEFINED;
        if (is_dispatch) {
            dispatch_model_type = C_csp_tou::C_dispatch_model_type::E_dispatch_model_type::DISPATCH_OPTIMIZATION;
        }
        else if (is_dispatch_targets) {
            dispatch_model_type = C_csp_tou::C_dispatch_model_type::E_dispatch_model_type::IMPORT_DISPATCH_TARGETS;
        }
        else {
            dispatch_model_type = C_csp_tou::C_dispatch_model_type::E_dispatch_model_type::HEURISTIC;
        }

        bool is_offtaker_frac_also_max = as_boolean("is_tod_pc_target_also_pc_max");

        C_csp_tou tou(offtaker_schedule, elec_pricing_schedule, dispatch_model_type, is_offtaker_frac_also_max);

        if (is_dispatch_targets) {
            size_t n_expect = (size_t)ceil((sim_setup.m_sim_time_end - sim_setup.m_sim_time_start) / 3600. * steps_per_hour);

            size_t inputs_len = 0;
            ssc_number_t* q_pc_target_su_in = as_array("q_pc_target_su_in", &inputs_len);
            if (inputs_len != n_expect) throw exec_error("tcsmolten_salt", "The length of dispatch target q_pc_target_su_in array does not match the value expected from the simulation start time, end time, and time steps per hour");

            ssc_number_t* q_pc_target_on_in = as_array("q_pc_target_on_in", &inputs_len);
            if (inputs_len != n_expect) throw exec_error("tcsmolten_salt", "The length of dispatch target q_pc_target_on_in array does not match the value expected from the simulation start time, end time, and time steps per hour");

            ssc_number_t* q_pb_max = as_array("q_pc_max_in", &inputs_len);
            if (inputs_len != n_expect) throw exec_error("tcsmolten_salt", "The length of dispatch target q_pc_max_in array does not match the value expected from the simulation start time, end time, and time steps per hour");

            ssc_number_t* is_rec_su_allowed_in = as_array("is_rec_su_allowed_in", &inputs_len);
            if (inputs_len != n_expect) throw exec_error("tcsmolten_salt", "The length of dispatch target is_rec_su_allowed_in array does not match the value expected from the simulation start time, end time, and time steps per hour");

            ssc_number_t* is_pc_su_allowed_in = as_array("is_pc_su_allowed_in", &inputs_len);
            if (inputs_len != n_expect) throw exec_error("tcsmolten_salt", "The length of dispatch target is_pc_su_allowed_in array does not match the value expected from the simulation start time, end time, and time steps per hour");

            ssc_number_t* is_pc_sb_allowed_in = as_array("is_pc_sb_allowed_in", &inputs_len);
            if (inputs_len != n_expect) throw exec_error("tcsmolten_salt", "The length of dispatch target is_pc_sb_allowed_in array does not match the value expected from the simulation start time, end time, and time steps per hour");

            tou.mc_dispatch_params.resize(inputs_len);
            for (size_t i = 0; i < inputs_len; i++) {
                tou.mc_dispatch_params.m_q_pc_target_su_in.at(i) = q_pc_target_su_in[i];
                tou.mc_dispatch_params.m_q_pc_target_on_in.at(i) = q_pc_target_on_in[i];
                tou.mc_dispatch_params.m_q_pc_max_in.at(i) = q_pb_max[i];
                tou.mc_dispatch_params.m_is_rec_su_allowed_in.at(i) = (bool)is_rec_su_allowed_in[i];
                tou.mc_dispatch_params.m_is_pc_su_allowed_in.at(i) = (bool)is_pc_su_allowed_in[i];
                tou.mc_dispatch_params.m_is_pc_sb_allowed_in.at(i) = (bool)is_pc_sb_allowed_in[i];
                tou.mc_dispatch_params.m_q_dot_elec_to_PAR_HTR_in.at(i) = 0.0;      // Defaults values for heater
                tou.mc_dispatch_params.m_is_PAR_HTR_allowed_in.at(i) = false;
            }

            if (is_parallel_heater) {
                ssc_number_t* q_dot_elec_to_PAR_HTR_in, * is_PAR_HTR_allowed_in;

                q_dot_elec_to_PAR_HTR_in = as_array("q_dot_elec_to_PAR_HTR_in", &inputs_len);
                if (inputs_len != n_expect) throw exec_error("tcsmolten_salt", "The length of dispatch target q_dot_elec_to_PAR_HTR_in array does not match the value expected from the simulation start time, end time, and time steps per hour");

                is_PAR_HTR_allowed_in = as_array("is_PAR_HTR_allowed_in", &inputs_len);
                if (inputs_len != n_expect) throw exec_error("tcsmolten_salt", "The length of dispatch target is_PAR_HTR_allowed_in array does not match the value expected from the simulation start time, end time, and time steps per hour");

                for (size_t i = 0; i < inputs_len; i++) {
                    tou.mc_dispatch_params.m_q_dot_elec_to_PAR_HTR_in.at(i) = q_dot_elec_to_PAR_HTR_in[i];
                    tou.mc_dispatch_params.m_is_PAR_HTR_allowed_in.at(i) = (bool)is_PAR_HTR_allowed_in[i];
                }
            }
        }

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

        dispatch.solver_params.dispatch_optimize = is_dispatch;

        if (is_dispatch && sim_type == 1){

            double heater_startup_cost = 0.0;
            if (is_parallel_heater) {
                heater_startup_cost = as_double("disp_hsu_cost_rel") * q_dot_heater_des;    //[$/start]
            }

            dispatch.solver_params.set_user_inputs(is_dispatch, steps_per_hour, as_integer("disp_frequency"), as_integer("disp_horizon"),
                as_integer("disp_max_iter"), as_double("disp_mip_gap"), as_double("disp_timeout"),
                as_integer("disp_spec_presolve"), as_integer("disp_spec_bb"), as_integer("disp_spec_scaling"), as_integer("disp_reporting"),
                as_boolean("is_write_ampl_dat"), as_boolean("is_ampl_engine"), as_string("ampl_data_dir"), as_string("ampl_exec_call"));

            double disp_csu_cost_calc = as_double("disp_csu_cost_rel")*W_dot_cycle_des; //[$/start]
            double disp_rsu_cost_calc = as_double("disp_rsu_cost_rel")*q_dot_rec_des;   //[$/start]
            dispatch.params.set_user_params(as_boolean("can_cycle_use_standby"), as_double("disp_time_weighting"),
                disp_rsu_cost_calc, heater_startup_cost, disp_csu_cost_calc, as_double("disp_pen_ramping"),
                as_double("disp_inventory_incentive"), as_double("q_rec_standby"), as_double("q_rec_heattrace"));
        }

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
        try
        {
            // Initialize Solver
            csp_solver.init();
        }
        catch( C_csp_exception &csp_exception )
        {
            // Report warning before exiting with error
            while( csp_solver.mc_csp_messages.get_message(&out_type, &out_msg) )
            {
                log(out_msg, out_type);
            }

            throw exec_error("tcsmolten_salt", csp_exception.m_error_message);
        }

        // If no exception, then report messages
        while (csp_solver.mc_csp_messages.get_message(&out_type, &out_msg))
        {
            log(out_msg, out_type);
        }

        // *****************************************************
        // System design is complete, get design parameters from component models as necessary

            // *************************
            // Solar field
        assign("N_hel_calc", N_hel);                    //[-]
        assign("refl_image_error", refl_image_error);   //[mrad]
        assign("heliostat_area", heliostat_area);   //[m2]
        assign("average_attenuation", average_attenuation); //[%]
        assign("A_sf", (ssc_number_t)A_sf);         //[m2]
        assign("land_min_abs", (ssc_number_t)land_min_abs);     //[m]
        assign("land_max_abs", (ssc_number_t)land_max_abs);     //[m]
        assign("land_area_base_calc", (ssc_number_t)land_area_base);     //[acre]
        assign("total_land_area_before_rad_cooling_calc", (ssc_number_t)total_land_area_before_rad_cooling);        //[acre]
        
        size_t n_helio_pos_rows = helio_pos.nrows();
        ssc_number_t* p_helio_positions_calc = allocate("helio_positions_calc", n_helio_pos_rows, 2);
        // Try to determine whether heliostat positions represent surround or cavity field
        for (size_t i = 0; i < n_helio_pos_rows; i++)
        {
            p_helio_positions_calc[i * 2] = (ssc_number_t)helio_pos(i, 0);       //[m] x
            p_helio_positions_calc[i * 2 + 1] = (ssc_number_t)helio_pos(i, 1);   //[m] y
        }

        double W_dot_col_tracking_des = collector_receiver.get_tracking_power();    //[MWe]
        assign("W_dot_col_tracking_des", W_dot_col_tracking_des);       //[MWe]

            // *************************
            // Tower and receiver
        assign("h_tower_calc", (ssc_number_t)THT);   //[m]
                // External receiver
        assign("rec_height_calc", (ssc_number_t)rec_height);    //[m]
        assign("D_rec_calc", (ssc_number_t)D_rec);              //[m]
        assign("ext_rec_area", (ssc_number_t)ext_rec_area);     //[m2]
        assign("ext_rec_aspect", (ssc_number_t)ext_rec_aspect); //[-]
                // Cavity receiver
        assign("cav_rec_height_calc", (ssc_number_t)cav_rec_height);
        assign("cav_rec_width_calc", (ssc_number_t)cav_rec_width);
        assign("cav_rec_area", (ssc_number_t)cav_rec_area);
        assign("cav_panel_width", (ssc_number_t)cav_panel_width);
        assign("cav_radius", (ssc_number_t)cav_radius);
                // Both
        assign("A_rec", A_rec);     //[m2]

        double L_tower_piping = std::numeric_limits<double>::quiet_NaN();
        double od_tube_calc = std::numeric_limits<double>::quiet_NaN();
        receiver->get_design_geometry(L_tower_piping, od_tube_calc);
        assign("L_tower_piping_calc", L_tower_piping);      //[m]
        assign("od_tube_calc", od_tube_calc*1.E3);          //[mm] convert from m

        double eta_rec_thermal_des;     //[-]
        double W_dot_rec_pump_des;      //[MWe]
        double W_dot_rec_pump_tower_share_des;  //[MWe]
        double W_dot_rec_pump_rec_share_des;    //[MWe]
        double rec_pump_coef_des;       //[MWe/MWt]
        double rec_vel_htf_des;         //[m/s]
        double m_dot_htf_rec_des;       //[kg/s]
        double q_dot_piping_loss_des;   //[MWt]
        double m_dot_htf_rec_max;       //[kg/s]
        receiver->get_design_performance(eta_rec_thermal_des,
            W_dot_rec_pump_des, W_dot_rec_pump_tower_share_des, W_dot_rec_pump_rec_share_des,
            rec_pump_coef_des, rec_vel_htf_des, m_dot_htf_rec_des, m_dot_htf_rec_max, q_dot_piping_loss_des);
        assign("q_dot_rec_des", q_dot_rec_des);                 //[MWt]
        assign("solar_mult_calc", solar_mult);                  //[-]
        assign("eta_rec_thermal_des", eta_rec_thermal_des);     //[-]
        assign("W_dot_rec_pump_des", W_dot_rec_pump_des);       //[MWe]
        assign("W_dot_rec_pump_tower_share_des", W_dot_rec_pump_tower_share_des);     //[MWe]
        assign("W_dot_rec_pump_rec_share_des", W_dot_rec_pump_rec_share_des);       //[MWe]
        assign("vel_rec_htf_des", rec_vel_htf_des);             //[m/s]
        assign("m_dot_htf_rec_des", m_dot_htf_rec_des);         //[kg/s]
        assign("q_dot_piping_loss_des", q_dot_piping_loss_des); //[MWt]
        assign("m_dot_htf_rec_max", m_dot_htf_rec_max);         //[kg/s]

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

        double tshours_heater = 0.0;
        if (q_dot_heater_des > 0.0) {
            tshours_heater = Q_tes_des_calc / q_dot_heater_des;     //[hr]
        }

        assign("tshours_heater", tshours_heater);

            // *************************
            // Power Cycle
        double m_dot_htf_pc_des;    //[kg/s]
        double cp_htf_pc_des;       //[kJ/kg-K]
        double W_dot_pc_pump_des;   //[MWe]
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

        rankine_pc.get_design_parameters(m_dot_htf_pc_des, cp_htf_pc_des, W_dot_pc_pump_des, W_dot_pc_cooling_des,
                        n_T_htf_pars, n_T_amb_pars, n_m_dot_pars,
                        T_htf_ref_calc /*C*/, T_htf_low_calc /*C*/, T_htf_high_calc /*C*/,
                        T_amb_ref_calc /*C*/, T_amb_low_calc /*C*/, T_amb_high_calc /*C*/,
                        m_dot_htf_ND_ref_calc, m_dot_htf_ND_low_calc /*-*/, m_dot_htf_ND_high_calc /*-*/,
                        W_dot_gross_ND_des, Q_dot_HTF_ND_des, W_dot_cooling_ND_des, m_dot_water_ND_des);
        m_dot_htf_pc_des /= 3600.0;     // convert from kg/hr to kg/s
        assign("m_dot_htf_cycle_des", m_dot_htf_pc_des);
        assign("q_dot_cycle_des", q_dot_pc_des);
        assign("W_dot_cycle_pump_des", W_dot_pc_pump_des);
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

        double W_dot_rec_pump_des_for_cap_calc = W_dot_rec_pump_des;
        if (!std::isfinite(W_dot_rec_pump_des_for_cap_calc)) {
            W_dot_rec_pump_des_for_cap_calc = 0.02 * W_dot_cycle_des;
        }

        double plant_net_capacity_calc = W_dot_cycle_des - W_dot_col_tracking_des - W_dot_rec_pump_des_for_cap_calc -
                                        W_dot_pc_pump_des - W_dot_pc_cooling_des - W_dot_bop_design - W_dot_fixed_parasitic_design;    //[MWe]

        
        bool use_net_cycle_output_as_capacity = as_boolean("use_net_cycle_output_as_capacity");
        if (pb_tech_type == 1 && use_net_cycle_output_as_capacity) {
            // Calculate system capacity using only net cycle output
            double W_dot_pc_cooling_des_for_cap_calcs = rankine_pc.get_design_input_cooling_power();
            plant_net_capacity_calc = W_dot_cycle_des - W_dot_pc_cooling_des_for_cap_calcs;
        }

        double system_capacity = plant_net_capacity_calc * 1.E3;         //[kWe], convert from MWe

        assign("W_dot_bop_design", W_dot_bop_design);           //[MWe]
        assign("W_dot_fixed", W_dot_fixed_parasitic_design);    //[MWe]
        // Calculate system capacity instead of pass in
        assign("system_capacity", system_capacity);     //[kWe]
        assign("nameplate", system_capacity * 1.E-3);   //[MWe]
        assign("cp_system_nameplate", system_capacity * 1.E-3); //[MWe]
        assign("cp_battery_nameplate", 0.0);             //[MWe]

            // ******* Costs ************
        double A_sf_refl = A_sf;
        double site_improv_spec_cost = as_double("site_spec_cost");
        double heliostat_spec_cost = as_double("heliostat_spec_cost");
        double heliostat_fixed_cost = as_double("cost_sf_fixed");

        double h_rec_cost_in = std::numeric_limits<double>::quiet_NaN();
        if (rec_type == 0) {
            h_rec_cost_in = rec_height; //[m]
        }
        else if (rec_type == 1) {
            h_rec_cost_in = cav_rec_height; //[m]
        }
        double tower_fixed_cost = as_double("tower_fixed_cost");
        double tower_cost_scaling_exp = as_double("tower_exp");

        double rec_ref_cost = as_double("rec_ref_cost");
        double A_rec_ref = as_double("rec_ref_area");
        double rec_cost_scaling_exp = as_double("rec_cost_exp");

        double Q_storage = as_double("P_ref") / as_double("design_eff") * as_double("tshours");
        double tes_spec_cost = as_double("tes_spec_cost");

        // no Cold Temp TES, so set those cost model inputs to 0
        double Q_CT_tes = 0.0;
        double CT_tes_spec_cost = 0.0;

        double W_dot_design = as_double("P_ref");
        double power_cycle_spec_cost = as_double("plant_spec_cost");

        // Set heater thermal power and cost above, because they're dependent on is_heater boolean

        double rad_fluidcost = 0.0;
        double rad_installcost = 0.0;
        double rad_unitcost = 0.0;
        double rad_volmulti = 0.0;
        double coldstorage_unitcost = 0.0;
        double radfield_area = 0.0;
        double coldstorage_vol = 0.0;
        double radfield_vol = 0.0;

        if (rankine_pc.ms_params.m_CT == 4) {
            radfield_area = rankine_pc.mc_radiator.ms_params.Afield;
            radfield_vol = rankine_pc.mc_radiator.ms_params.D * rankine_pc.mc_radiator.ms_params.D / 4 * PI * rankine_pc.mc_radiator.ms_params.n * rankine_pc.mc_radiator.ms_params.Np * rankine_pc.mc_radiator.ms_params.L; //Calculate volume in radiator panel tubes = pi/4*d^2*L*n*Np
            if (rankine_pc.mc_two_tank_ctes.ms_params.m_ctes_type == 2) //If two tank
            {
                coldstorage_vol = rankine_pc.mc_two_tank_ctes.get_physical_volume();
            }
            if (rankine_pc.mc_two_tank_ctes.ms_params.m_ctes_type > 2) //If stratified 
            {
                coldstorage_vol = rankine_pc.mc_stratified_ctes.get_physical_volume();

            }

            rad_unitcost = as_double("radiator_unitcost");
            rad_installcost = as_double("radiator_installcost");
            rad_fluidcost = as_double("radiator_fluidcost");
            rad_volmulti = as_double("radfluid_vol_ratio");
            coldstorage_unitcost = as_double("ctes_cost");
        }

        double bop_spec_cost = as_double("bop_spec_cost");

        double fossil_backup_spec_cost = as_double("fossil_spec_cost");

        double contingency_rate = as_double("contingency_rate");

        //land area
        double total_land_area = total_land_area_before_rad_cooling + radfield_area / 4046.86 /*acres/m^2*/;
        assign("total_land_area", (ssc_number_t)total_land_area);

        double plant_net_capacity = system_capacity / 1000.0;         //[MWe], convert from kWe
        double EPC_land_spec_cost = as_double("csp.pt.cost.epc.per_acre");
        double EPC_land_perc_direct_cost = as_double("csp.pt.cost.epc.percent");
        double EPC_land_per_power_cost = as_double("csp.pt.cost.epc.per_watt");
        double EPC_land_fixed_cost = as_double("csp.pt.cost.epc.fixed");
        double total_land_spec_cost = as_double("land_spec_cost");
        double total_land_perc_direct_cost = as_double("csp.pt.cost.plm.percent");
        double total_land_per_power_cost = as_double("csp.pt.cost.plm.per_watt");
        double total_land_fixed_cost = as_double("csp.pt.cost.plm.fixed");
        double sales_tax_basis = as_double("sales_tax_frac");
        double sales_tax_rate = as_double("sales_tax_rate");

        double site_improvement_cost, heliostat_cost, tower_cost, receiver_cost, tes_cost, CT_tes_cost, power_cycle_cost,
            heater_cost, rad_field_totcost, rad_fluid_totcost, rad_storage_totcost, bop_cost, fossil_backup_cost,
            direct_capital_precontingency_cost, contingency_cost, total_direct_cost, epc_and_owner_cost, total_land_cost,
            sales_tax_cost, total_indirect_cost, total_installed_cost, estimated_installed_cost_per_cap;

        site_improvement_cost = heliostat_cost = tower_cost = receiver_cost = tes_cost = CT_tes_cost = power_cycle_cost =
            heater_cost = rad_field_totcost = rad_fluid_totcost = rad_storage_totcost = bop_cost = fossil_backup_cost =
            direct_capital_precontingency_cost = contingency_cost = total_direct_cost = epc_and_owner_cost = total_land_cost =
            sales_tax_cost = total_indirect_cost = total_installed_cost = estimated_installed_cost_per_cap = std::numeric_limits<double>::quiet_NaN();

        N_mspt::calculate_mspt_etes_costs(
            A_sf_refl,
            site_improv_spec_cost,
            heliostat_spec_cost,
            heliostat_fixed_cost,

            THT,
            h_rec_cost_in,
            h_helio,
            tower_fixed_cost,
            tower_cost_scaling_exp,

            A_rec,
            rec_ref_cost,
            A_rec_ref,
            rec_cost_scaling_exp,

            Q_storage,
            tes_spec_cost,

            Q_CT_tes,
            CT_tes_spec_cost,

            W_dot_design,
            power_cycle_spec_cost,

            q_dot_heater_des,       //[MWt]
            heater_spec_cost,

            radfield_area,
            coldstorage_vol,
            radfield_vol,
            rad_unitcost,
            rad_installcost,
            rad_volmulti,
            rad_fluidcost,
            coldstorage_unitcost,

            bop_spec_cost,

            fossil_backup_spec_cost,

            contingency_rate,

            total_land_area,
            plant_net_capacity,
            EPC_land_spec_cost,
            EPC_land_perc_direct_cost,
            EPC_land_per_power_cost,
            EPC_land_fixed_cost,
            total_land_spec_cost,
            total_land_perc_direct_cost,
            total_land_per_power_cost,
            total_land_fixed_cost,
            sales_tax_basis,
            sales_tax_rate,

            site_improvement_cost,
            heliostat_cost,
            tower_cost,
            receiver_cost,
            tes_cost,
            CT_tes_cost,
            power_cycle_cost,
            heater_cost,
            rad_field_totcost,
            rad_fluid_totcost,
            rad_storage_totcost,
            bop_cost,
            fossil_backup_cost,
            direct_capital_precontingency_cost,
            contingency_cost,
            total_direct_cost,
            total_land_cost,
            epc_and_owner_cost,
            sales_tax_cost,
            total_indirect_cost,
            total_installed_cost,
            estimated_installed_cost_per_cap
        );

        // 1.5.2016 twn: financial model needs an updated total_installed_cost, remaining are for reporting only
        assign("total_installed_cost", (ssc_number_t)total_installed_cost);

        assign("h_rec_input_to_cost_model", (ssc_number_t)h_rec_cost_in);       //[m]
        assign("csp.pt.cost.site_improvements", (ssc_number_t)site_improvement_cost);
        assign("csp.pt.cost.heliostats", (ssc_number_t)heliostat_cost);
        assign("csp.pt.cost.tower", (ssc_number_t)tower_cost);
        assign("csp.pt.cost.receiver", (ssc_number_t)receiver_cost);
        assign("csp.pt.cost.storage", (ssc_number_t)tes_cost);
        assign("csp.pt.cost.power_block", (ssc_number_t)power_cycle_cost);
        assign("heater_cost", (ssc_number_t)heater_cost);

        if (pb_tech_type == 0) {
            if (rankine_pc.ms_params.m_CT == 4) {
                assign("csp.pt.cost.rad_field", (ssc_number_t)rad_field_totcost);
                assign("csp.pt.cost.rad_fluid", (ssc_number_t)rad_fluid_totcost);
                assign("csp.pt.cost.rad_storage", (ssc_number_t)rad_storage_totcost);
            }
        }
        assign("csp.pt.cost.bop", (ssc_number_t)bop_cost);
        assign("csp.pt.cost.fossil", (ssc_number_t)fossil_backup_cost);
        assign("ui_direct_subtotal", (ssc_number_t)direct_capital_precontingency_cost);
        assign("csp.pt.cost.contingency", (ssc_number_t)contingency_cost);
        assign("total_direct_cost", (ssc_number_t)total_direct_cost);
        assign("csp.pt.cost.epc.total", (ssc_number_t)epc_and_owner_cost);
        assign("csp.pt.cost.plm.total", (ssc_number_t)total_land_cost);
        assign("csp.pt.cost.sales_tax.total", (ssc_number_t)sales_tax_cost);
        assign("total_indirect_cost", (ssc_number_t)total_indirect_cost);
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

            throw exec_error("tcsmolten_salt", csp_exception.m_error_message);
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
        for (size_t i = 0; i < n_steps_fixed; i++)
        {
            p_m_dot_rec[i] = (ssc_number_t)(p_m_dot_rec[i] / 3600.0);   //[kg/s] convert from kg/hr
            p_m_dot_pc[i] = (ssc_number_t)(p_m_dot_pc[i] / 3600.0);     //[kg/s] convert from kg/hr
            p_m_dot_water_pc[i] = (ssc_number_t)(p_m_dot_water_pc[i] / 3600.0); //[kg/s] convert from kg/hr
        }       

        // Set output data from heliostat class
        size_t n_rows_eta_map = heliostatfield.ms_params.m_eta_map.nrows();
        ssc_number_t *eta_map_out = allocate("eta_map_out", n_rows_eta_map, 3);
        size_t n_rows_flux_maps = heliostatfield.ms_params.m_flux_maps.nrows();
        size_t n_cols_flux_maps = heliostatfield.ms_params.m_flux_maps.ncols() + 2;
        ssc_number_t *flux_maps_out = allocate("flux_maps_out", n_rows_eta_map, n_cols_flux_maps);
        ssc_number_t *flux_maps_for_import = allocate("flux_maps_for_import", n_rows_eta_map, n_cols_flux_maps);

        if(n_rows_eta_map != n_rows_flux_maps)
        {
            log("The number of rows in the field efficiency and receiver flux map matrices are not equal. This is unexpected, and the flux maps may be inaccurate.");
        }

        // [W/m2 * m2 / (m2_per_panel?)]
        double flux_scaling_mult = as_double("dni_des")*heliostatfield.ms_params.m_A_sf / 1000.0 /
            (A_rec / double(heliostatfield.ms_params.m_n_flux_x));

        for( size_t i = 0; i < n_rows_eta_map; i++ ) {
            flux_maps_for_import[n_cols_flux_maps * i] =
                flux_maps_out[n_cols_flux_maps*i] =
                eta_map_out[3 * i] = (ssc_number_t)heliostatfield.ms_params.m_eta_map(i, 0);        //[deg] Solar azimuth angle
            flux_maps_for_import[n_cols_flux_maps * i + 1] =
                flux_maps_out[n_cols_flux_maps*i + 1] =
                eta_map_out[3 * i + 1] = (ssc_number_t)heliostatfield.ms_params.m_eta_map(i, 1);    //[deg] Solar zenith angle
            eta_map_out[3 * i + 2] = (ssc_number_t)heliostatfield.ms_params.m_eta_map(i, 2);                            //[deg] Solar field optical efficiency
            for( size_t j = 2; j < n_cols_flux_maps; j++ )
            {
                flux_maps_out[n_cols_flux_maps*i + j] = (ssc_number_t)(heliostatfield.ms_params.m_flux_maps(i, j - 2)*heliostatfield.ms_params.m_eta_map(i, 2)*flux_scaling_mult);      //[kW/m^2]
                flux_maps_for_import[n_cols_flux_maps*i + j] = (ssc_number_t)heliostatfield.ms_params.m_flux_maps(i, j - 2);
            }
        }

        size_t count;
        ssc_number_t *p_W_dot_net = as_array("P_out_net", &count);
        ssc_number_t *p_time_final_hr = as_array("time_hr", &count);

        // 'adjustment_factors' class stores factors in hourly array, so need to index as such
        adjustment_factors haf(this->get_var_table(), "adjust");
        if( !haf.setup(n_steps_full) )
            throw exec_error("tcsmolten_salt", "failed to setup adjustment factors: " + haf.error());

        ssc_number_t *p_gen = allocate("gen", count);
        ssc_number_t* p_gensales_after_avail = allocate("gensales_after_avail", count);
        for( size_t i = 0; i < count; i++ )
        {
            size_t hour = (size_t)ceil(p_time_final_hr[i]);
            p_gen[i] = (ssc_number_t)(p_W_dot_net[i] * 1.E3 * haf(hour));           //[kWe]
            p_gensales_after_avail[i] = max(0.0, p_gen[i]);                         //[kWe]
        }

        if (!as_boolean("vacuum_arrays")) {
            ssc_number_t* p_annual_energy_dist_time = gen_heatmap(this, steps_per_hour);
        }
        accumulate_annual_for_year("gen", "annual_energy", sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed/steps_per_hour);
        accumulate_annual_for_year("gensales_after_avail", "annual_sales_energy", sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour);
        
        accumulate_annual_for_year("P_cycle", "annual_W_cycle_gross", 1000.0*sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed/steps_per_hour);        //[kWe-hr]
        accumulate_annual_for_year("P_cooling_tower_tot", "annual_W_cooling_tower", 1000.0*sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour);        //[kWe-hr]

        accumulate_annual_for_year("Q_thermal", "annual_q_rec_htf", sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour);           //[MWt-hr]
        accumulate_annual_for_year("q_dot_rec_inc", "annual_q_rec_inc", sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour);           //[MWt-hr]
        accumulate_annual_for_year("q_thermal_loss", "annual_q_rec_loss", sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour);
        accumulate_annual_for_year("q_piping_losses", "annual_q_piping_loss", sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour);
        accumulate_annual_for_year("q_startup", "annual_q_rec_startup", sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour);
        accumulate_annual_for_year("P_tower_pump", "annual_E_tower_pump", sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour);

        assign("annual_eta_rec_th", (ssc_number_t)(1.0 - as_number("annual_q_rec_loss") / as_number("annual_q_rec_inc")));
        assign("annual_eta_rec_th_incl_refl", (ssc_number_t)(as_number("rec_absorptance")*as_number("annual_eta_rec_th")));

        size_t count_df;
        ssc_number_t* p_defocus = as_array("defocus", &count_df);
        size_t count_q_rec_in;
        ssc_number_t* p_q_rec_in = as_array("q_dot_rec_inc", &count_q_rec_in);

        double q_defocus_sum = 0.0;
        double i_defocus;
        for (size_t i = 0; i < count_df; i++) {
            i_defocus = min(1.0, max(0.0, p_defocus[i]));
            q_defocus_sum += p_q_rec_in[i]*(1.0 - i_defocus);   //[MWt]
        }
        q_defocus_sum *= sim_setup.m_report_step/3600.0;    //[MWt-hr]
        assign("annual_q_defocus_est", q_defocus_sum);      //[MWt-hr]

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
        double V_water_mirrors = as_double("water_usage_per_wash") / 1000.0*A_sf*as_double("washing_frequency");
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
         
        if (pb_tech_type == 0) {
            if (rankine_pc.ms_params.m_CT == 4) {
                double A_radfield = rankine_pc.mc_radiator.ms_params.Afield;
                assign("A_radfield", (ssc_number_t)A_radfield);
            }
        }

        // Cycle off-design performance tables for use with dispatch optimization models solved outside of ssc
        // Mimics calculations in dispatch.params.eff_table_load and dispatch.params.eff_table_Tdb in csp_solver_core,
        // but repeating here to allow for more load points that could be needed for nonlinear dispatch model formulation
        // TODO: This should be moved out of the cmod and into the C_csp_power_cycle class
        int neff = 10;
        ssc_number_t* table_load_efficiency = allocate("cycle_eff_load_table", neff, 2);
        double q_min = p_csp_power_cycle->get_min_thermal_power();
        double q_max = p_csp_power_cycle->get_max_thermal_power();
        double q_des = as_double("P_ref") / as_double("design_eff");
        for (int i = 0; i < neff; i++)
        {
            double x = q_min + (q_max - q_min) / (double)(neff - 1) * i;
            double xf = x / q_des;
            double eta = p_csp_power_cycle->get_efficiency_at_load(xf);
            table_load_efficiency[2 * i] = x;  //MWt
            table_load_efficiency[2 * i + 1] = eta;
        }

        int neffT = 40;
        ssc_number_t* table_Tdb_eff_wcond = allocate("cycle_Tdb_table", neffT, 3);
        for (int i = 0; i < neffT; i++)
        {
            double T = -10. + 60. / (double)(neffT - 1) * i; // C
            double wcond;
            double norm_eta = p_csp_power_cycle->get_efficiency_at_TPH(T, 1., 30., &wcond) / as_double("design_eff");
            table_Tdb_eff_wcond[3 * i] = T;
            table_Tdb_eff_wcond[3 * i + 1] = norm_eta;
            table_Tdb_eff_wcond[3 * i + 2] = wcond / as_double("P_ref");  //fraction of rated gross gen
        }

        // Calculating the Capacity factor of the highest priced 1000 and 2000 hours
        ssc_number_t* p_pricing_mult = as_array("pricing_mult", &count);
        ssc_number_t* p_tdry = as_array("tdry", &count);
        if (!as_boolean("vacuum_arrays")) {
            // Capacity factors based on highest pricing hours
            std::vector<pair<size_t, double>> ppa_pairs;
            ppa_pairs.resize(count);
            for (size_t i = 0; i < count; i++) {
                ppa_pairs[i].first = i;
                ppa_pairs[i].second = p_pricing_mult[i];
            }

            std::sort(ppa_pairs.begin(), ppa_pairs.end(), SortByDouble);
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

            // **********************************************************
            // **********************************************************

            // Capacity factors based on warmest ambient temperatures
            std::vector<pair<size_t, double>> tdry_pairs;
            tdry_pairs.resize(count);
            for (size_t i = 0; i < count; i++) {
                tdry_pairs[i].first = i;
                tdry_pairs[i].second = p_tdry[i];
            }

            std::sort(tdry_pairs.begin(), tdry_pairs.end(), SortByDouble);
            size_t n_tdry_steps = 100;

            double total_energy_in_sub_period_tdry = 0.0;
            for (size_t i = 0; i < n_tdry_steps; i++) {
                size_t j = tdry_pairs[i].first;
                total_energy_in_sub_period_tdry += p_gen[j] * sim_setup.m_report_step / 3600.0;     //[kWe-hr]
            }

            double total_energy_nameplate_tdry = nameplate * n_tdry_steps * sim_setup.m_report_step / 3600.0;     //[kWe-hr]

            double cap_fac_warmest_100_tdrys = 0.0;
            if (nameplate > 0.0) {
                cap_fac_warmest_100_tdrys = total_energy_in_sub_period_tdry / total_energy_nameplate_tdry * 100.0;    //[%]        
            }

            assign("capacity_factor_warmest_100_Tambs", cap_fac_warmest_100_tdrys);
        }
        else {
            double nan_output = std::numeric_limits<double>::quiet_NaN();
            assign("capacity_factor_highest_1000_ppas", nan_output);
            assign("capacity_factor_highest_2000_ppas", nan_output);
            assign("capacity_factor_warmest_100_Tambs", nan_output);
        }
        // **********************************************************
        // **********************************************************

        if (csp_financial_model > 0 && csp_financial_model < 5) {   // Single Owner financial models

            if (is_assigned("ppa_price_input")) {

                // Get first year base ppa price
                size_t count_ppa_price_input;
                ssc_number_t* ppa_price_input_array = as_array("ppa_price_input", &count_ppa_price_input);
                double ppa_price_year1 = (double)ppa_price_input_array[0];  // [$/kWh]

                double T_amb_hot = 30.0;    //[C]
                double rev_full_cap_T_amb_hot = 0.0;    //[$]
                double rev_actual_T_amb_hot = 0.0;      //[$]

                for (size_t i = 0; i < count; i++) {
                    if (p_tdry[i] > T_amb_hot) {
                        double tod = p_pricing_mult[i] * ppa_price_year1;
                        rev_full_cap_T_amb_hot += system_capacity * tod;        //[kWe]*1[hr]*[$/kWh] = $
                        rev_actual_T_amb_hot += p_gen[i] * tod;                 //[$]
                    }
                }
                double hot_hours_revenue_fraction = rev_actual_T_amb_hot / rev_full_cap_T_amb_hot;    //[-]
                assign("hot_hours_revenue_fraction", hot_hours_revenue_fraction);
                assign("hot_hours_electricity_sales", rev_actual_T_amb_hot);  //[$]

                double rev_full_cap = 0.0;
                double rev_actual = 0.0;

                for (size_t i = 0; i < count; i++) {
                    double tod = p_pricing_mult[i] * ppa_price_year1;
                    rev_full_cap += system_capacity * tod;        //[kWe]*1[hr]*[$/kWh] = $
                    rev_actual += p_gen[i] * tod;                 //[$]
                }

                double all_hours_revenue_fraction = rev_actual / rev_full_cap;
                assign("all_hours_revenue_fraction", all_hours_revenue_fraction);
                assign("all_hours_electricity_sales", rev_actual);            //[$]
            }
            else {
                double nan_output = std::numeric_limits<double>::quiet_NaN();

                assign("hot_hours_revenue_fraction", nan_output);
                assign("hot_hours_electricity_sales", nan_output);  //[$]

                assign("all_hours_revenue_fraction", nan_output);
                assign("all_hours_electricity_sales", nan_output);            //[$]
            }
        }

        if (p_electric_resistance != NULL) {
            delete p_electric_resistance;
        }

        std::clock_t clock_end = std::clock();
        double sim_cpu_run_time = (clock_end - clock_start) / (double)CLOCKS_PER_SEC;		//[s]
        assign("sim_cpu_run_time", sim_cpu_run_time);   //[s]

    }
};

bool SortByDouble(const pair<int, double>& lhs,
    const pair<int, double>& rhs)
{
    return lhs.second > rhs.second;
}

DEFINE_MODULE_ENTRY(tcsmolten_salt, "CSP molten salt power tower with hierarchical controller and dispatch optimization", 1)
