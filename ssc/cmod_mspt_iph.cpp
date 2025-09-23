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
#include "csp_solver_pc_heat_sink.h"
#include "csp_solver_pc_heat_sink_physical.h"
#include "csp_solver_two_tank_tes.h"

#include "csp_dispatch.h"

#include "csp_solver_cr_electric_resistance.h"
#include "csp_solver_cavity_receiver.h"

#include "csp_system_costs.h"

#include <ctime>

static var_info _cm_vtab_mspt_iph[] = {

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
    { SSC_INPUT,     SSC_NUMBER, "q_pb_design",                        "Design heat input to power block",                                                                                                        "MWt",          "",                                  "System Design",                            "*",                                                                "",              "" },
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
    { SSC_INPUT,     SSC_NUMBER, "p_start",                            "Heliostat startup energy",                                                                                                                "kWhe",       "",                                  "Heliostat Field",                          "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "p_track",                            "Heliostat tracking energy",                                                                                                               "kWe",          "",                                  "Heliostat Field",                          "*",                                                                "",              ""},
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
    { SSC_INPUT,     SSC_NUMBER, "sf_excess",                          "Heliostat field multiple",                                                                                                                "",             "",                                  "System Design",                            "?=1.0",                                                            "",              ""},

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


{ SSC_INPUT,     SSC_NUMBER, "piping_loss_coefficient",            "Thermal loss per meter of piping",                                                                                                        "Wt/m2-K",      "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},

{ SSC_INPUT,     SSC_NUMBER, "rec_clearsky_model",				   "Clearsky model: None = -1, User-defined data = 0, Meinel = 1; Hottel = 2; Allen = 3; Moon = 4",											  "",             "",                                  "Tower and Receiver",                       "?=-1",															   "",              "SIMULATION_PARAMETER"},
{ SSC_INPUT,     SSC_ARRAY,  "rec_clearsky_dni",					"User-defined clear-sky DNI",																											  "W/m2",         "",                                  "Tower and Receiver",                       "rec_clearsky_model=0",											   "",              "SIMULATION_PARAMETER"},
{ SSC_INPUT,     SSC_NUMBER, "rec_clearsky_fraction",               "Weighting fraction on clear-sky DNI for receiver flow control",                                                                          "",             "",                                  "Tower and Receiver",                       "?=0.0",                                                            "",              "SIMULATION_PARAMETER"},

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
{ SSC_INPUT,     SSC_NUMBER, "heater_mult",                        "Heater multiple relative to design cycle thermal power",                                                                                  "-",            "",                                  "Parallel Heater",                          "is_parallel_htr=1",                                                "",              "SIMULATION_PARAMETER" },
{ SSC_INPUT,     SSC_NUMBER, "heater_efficiency",                  "Heater electric to thermal efficiency",                                                                                                   "%",            "",                                  "Parallel Heater",                          "is_parallel_htr=1",                                                "",              "SIMULATION_PARAMETER" },
{ SSC_INPUT,     SSC_NUMBER, "f_q_dot_des_allowable_su",           "Fraction of design power allowed during startup",                                                                                         "-",            "",                                  "Parallel Heater",                          "is_parallel_htr=1",                                                "",              "SIMULATION_PARAMETER" },
{ SSC_INPUT,     SSC_NUMBER, "hrs_startup_at_max_rate",            "Duration of startup at max startup power",                                                                                                "hr",           "",                                  "Parallel Heater",                          "is_parallel_htr=1",                                                "",              "SIMULATION_PARAMETER" },
{ SSC_INPUT,     SSC_NUMBER, "f_q_dot_heater_min",                 "Minimum allowable heater output as fraction of design",                                                                                   "",             "",                                  "Parallel Heater",                          "is_parallel_htr=1",                                                "",              "SIMULATION_PARAMETER" },
{ SSC_INPUT,     SSC_NUMBER, "disp_hsu_cost_rel",                  "Heater startup cost",                                                                                                                     "$/MWt/start",  "",                                  "System Control",                           "is_dispatch=1&is_parallel_htr=1",                                  "",              "SIMULATION_PARAMETER" },
{ SSC_INPUT,     SSC_NUMBER, "heater_spec_cost",                   "Heater specific cost",                                                                                                                    "$/kWht",       "",                                  "System Costs",                             "is_parallel_htr=1",                                                "",              "SIMULATION_PARAMETER" },
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

// Heat Sink
{ SSC_INPUT,     SSC_NUMBER, "pb_pump_coef",                       "Pumping power to move 1kg of HTF through PB loop",                                                                                        "kW/kg",        "",                                  "Heat Sink",                                "*",                                                                "",              "" },

{ SSC_INPUT,     SSC_NUMBER, "hs_type",                            "0: ideal model, 1: physical steam model",                                                                                                 "",             "",                                  "Heat Sink",                                "?=0",                                                              "",              "" },
{ SSC_INPUT,     SSC_NUMBER, "hs_htf_mdot_max_frac",               "Maximum HTF mass flow to heat sink relative to design point",                                                                             "",             "",                                  "Heat Sink",                                "*",                                                                "",              "" },

//{ SSC_INPUT,     SSC_NUMBER, "hs_phys_N_sub",                      "Number physical heat sink HX nodes",                                                                                                      "",             "",                                  "Heat Sink",                                "hs_type=1",                                                        "",              "" },
//{ SSC_INPUT,     SSC_NUMBER, "hs_phys_tol",                        "Physical heat sink solve tolerance",                                                                                                      "",             "",                                  "Heat Sink",                                "hs_type=1",                                                        "",              "" },
//{ SSC_INPUT,     SSC_NUMBER, "hs_phys_f_mdot_steam_min",           "Min steam mdot fraction for physical heat sink",                                                                                          "",             "",                                  "Heat Sink",                                "hs_type=1",                                                        "",              "" },
//{ SSC_INPUT,     SSC_NUMBER, "hs_phys_f_mdot_steam_max",           "Max steam mdot fraction for physical heat sink",                                                                                          "",             "",                                  "Heat Sink",                                "hs_type=1",                                                        "",              "" },
{ SSC_INPUT,     SSC_NUMBER, "hs_phys_T_steam_cold_des",           "Steam inlet temperature for physical heat sink",                                                                                          "C",            "",                                  "Heat Sink",                                "hs_type=1",                                                        "",              "" },
{ SSC_INPUT,     SSC_NUMBER, "hs_phys_P_steam_hot_des",            "Steam outlet (and inlet) pressure for physical heat sink",                                                                                "bar",          "",                                  "Heat Sink",                                "hs_type=1",                                                        "",              "" },
{ SSC_INPUT,     SSC_NUMBER, "hs_phys_Q_steam_hot_des",            "Steam outlet quality for physical heat sink",                                                                                             "",             "",                                  "Heat Sink",                                "hs_type=1",                                                        "",              "" },


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
{ SSC_INPUT,     SSC_NUMBER, "is_timestep_load_fractions",         "Use turbine load fraction for each timestep instead of block dispatch?",                                                                  "",             "",                                  "System Control",                           "?=0",                                                              "",              "SIMULATION_PARAMETER" },
{ SSC_INPUT,     SSC_ARRAY,  "timestep_load_fractions",            "Turbine load fraction for each timestep, alternative to block dispatch",                                                                  "",             "",                                  "System Control",                           "is_timestep_load_fractions=1",                                     "",              "SIMULATION_PARAMETER" },
{ SSC_INPUT,     SSC_ARRAY,  "f_turb_tou_periods",                 "Dispatch logic for turbine load fraction",                                                                                                "",             "",                                  "System Control",                           "*",                                                                "",              ""},
{ SSC_INPUT,     SSC_MATRIX, "weekday_schedule",                   "12x24 CSP operation Time-of-Use Weekday schedule",                                                                                        "",             "",                                  "System Control",                           "*",                                                                "",              ""},
{ SSC_INPUT,     SSC_MATRIX, "weekend_schedule",                   "12x24 CSP operation Time-of-Use Weekend schedule",                                                                                        "",             "",                                  "System Control",                           "*",                                                                "",              ""},
{ SSC_INPUT,     SSC_NUMBER, "is_tod_pc_target_also_pc_max",       "Is the TOD target cycle heat input also the max cycle heat input?",                                                                       "",             "",                                  "System Control",                           "?=0",                                                              "",              ""},

// Receiver control
{ SSC_INPUT,     SSC_NUMBER, "q_rec_standby",                      "Receiver standby energy consumption",                                                                                                     "kWt",          "",                                  "System Control",                           "?=9e99",                                                           "",              "SIMULATION_PARAMETER"},
{ SSC_INPUT,     SSC_NUMBER, "q_rec_heattrace",                    "Receiver heat trace energy consumption during startup",                                                                                   "kWhe",       "",                                  "System Control",                           "?=0.0",                                                            "",              "SIMULATION_PARAMETER"},

// System Control
    // Required if dispatch
{ SSC_INPUT,     SSC_NUMBER, "disp_rsu_cost_rel",                  "Receiver startup cost",                                                                                                                   "$/MWt/start",  "",                                  "System Control",                           "",                                                                 "",              "" },
{ SSC_INPUT,     SSC_NUMBER, "disp_horizon",                       "Time horizon for dispatch optimization",                                                                                                  "hour",         "",                                  "System Control",                           "is_dispatch=1",                                                    "",              "" },
{ SSC_INPUT,     SSC_NUMBER, "disp_frequency",                     "Frequency for dispatch optimization calculations",                                                                                        "hour",         "",                                  "System Control",                           "is_dispatch=1",                                                    "",              "" },
{ SSC_INPUT,     SSC_NUMBER, "disp_max_iter",                      "Max number of dispatch optimization iterations",                                                                                          "",             "",                                  "System Control",                           "is_dispatch=1",                                                    "",              "" },
{ SSC_INPUT,     SSC_NUMBER, "disp_timeout",                       "Max dispatch optimization solve duration",                                                                                                "s",            "",                                  "System Control",                           "is_dispatch=1",                                                    "",              "" },
{ SSC_INPUT,     SSC_NUMBER, "disp_mip_gap",                       "Dispatch optimization solution tolerance",                                                                                                "",             "",                                  "System Control",                           "is_dispatch=1",                                                    "",              "" },
{ SSC_INPUT,     SSC_NUMBER, "disp_time_weighting",                "Dispatch optimization future time discounting factor",                                                                                    "",             "",                                  "System Control",                           "is_dispatch=1",                                                    "",              "" },
    // Optional for custom scripting
{ SSC_INPUT,     SSC_NUMBER, "disp_steps_per_hour",                "Time steps per hour for dispatch optimization calculations",                                                                              "",             "",                                  "System Control",                           "?=1",                                                              "",              "SIMULATION_PARAMETER" },
{ SSC_INPUT,     SSC_NUMBER, "disp_spec_presolve",                 "Dispatch optimization presolve heuristic",                                                                                                "",             "",                                  "System Control",                           "?=-1",                                                             "",              "SIMULATION_PARAMETER" },
{ SSC_INPUT,     SSC_NUMBER, "disp_spec_bb",                       "Dispatch optimization B&B heuristic",                                                                                                     "",             "",                                  "System Control",                           "?=-1",                                                             "",              "SIMULATION_PARAMETER" },
{ SSC_INPUT,     SSC_NUMBER, "disp_spec_scaling",                  "Dispatch optimization scaling heuristic",                                                                                                 "",             "",                                  "System Control",                           "?=-1",                                                             "",              "SIMULATION_PARAMETER" },
{ SSC_INPUT,     SSC_NUMBER, "disp_reporting",                     "Dispatch optimization reporting level",                                                                                                   "",             "",                                  "System Control",                           "?=-1",                                                             "",              "SIMULATION_PARAMETER" },
{ SSC_INPUT,     SSC_NUMBER, "disp_inventory_incentive",           "Dispatch storage terminal inventory incentive multiplier",                                                                                "",             "",                                  "System Control",                           "?=0.0",                                                            "",              "SIMULATION_PARAMETER" },


// Pricing schedules (copied from electricity - eventually need to resolve electricity vs. heat)
{ SSC_INPUT,     SSC_NUMBER, "ppa_multiplier_model",               "PPA multiplier model 0: dispatch factors dispatch_factorX, 1: hourly multipliers dispatch_factors_ts",                                    "0/1",          "0=diurnal,1=timestep",              "Time of Delivery Factors",                 "?=0", /*need a default so this var works in required_if*/          "INTEGER,MIN=0", "SIMULATION_PARAMETER" },
{ SSC_INPUT,     SSC_NUMBER, "ppa_soln_mode",                      "PPA solution mode (0=Specify IRR target, 1=Specify PPA price)",                                                                           "",             "",                                  "Financial Solution Mode",                  "ppa_multiplier_model=0&csp_financial_model<5&is_dispatch=1&sim_type=1",       "",   "SIMULATION_PARAMETER" },
{ SSC_INPUT,     SSC_ARRAY,  "dispatch_factors_ts",                "Dispatch payment factor array",                                                                                                           "",             "",                                  "Time of Delivery Factors",                 "ppa_multiplier_model=1&csp_financial_model<5&is_dispatch=1&sim_type=1",       "",   "SIMULATION_PARAMETER" },
{ SSC_INPUT,     SSC_MATRIX, "dispatch_sched_weekday",             "PPA pricing weekday schedule, 12x24",                                                                                                     "",             "",                                  "Time of Delivery Factors",                 "ppa_multiplier_model=0&csp_financial_model<5&is_dispatch=1&sim_type=1",       "",   "SIMULATION_PARAMETER" },
{ SSC_INPUT,     SSC_MATRIX, "dispatch_sched_weekend",             "PPA pricing weekend schedule, 12x24",                                                                                                     "",             "",                                  "Time of Delivery Factors",                 "ppa_multiplier_model=0&csp_financial_model<5&is_dispatch=1&sim_type=1",       "",   "SIMULATION_PARAMETER" },
{ SSC_INPUT,     SSC_ARRAY,  "dispatch_tod_factors",               "TOD factors for periods 1 through 9",                                                                                                     "",             "",                                  "Time of Delivery Factors",                 "ppa_multiplier_model=0&csp_financial_model<5&is_dispatch=1&sim_type=1",       "",   "SIMULATION_PARAMETER" },
{ SSC_INPUT,     SSC_ARRAY,  "ppa_price_input_heat_btu",           "PPA prices - yearly",			                                                                                                          "$/MMBtu",	  "",	                               "Revenue",			                       "ppa_multiplier_model=0&csp_financial_model<5&is_dispatch=1",                  "",   "" },


// Costs
{ SSC_INPUT,     SSC_NUMBER, "tower_fixed_cost",                   "Tower fixed cost",                                                                                                                        "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
{ SSC_INPUT,     SSC_NUMBER, "tower_exp",                          "Tower cost scaling exponent",                                                                                                             "",             "",                                  "System Costs",                             "*",                                                                "",              "" },
{ SSC_INPUT,     SSC_NUMBER, "rec_ref_cost",                       "Receiver reference cost",                                                                                                                 "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
{ SSC_INPUT,     SSC_NUMBER, "rec_ref_area",                       "Receiver reference area for cost scale",                                                                                                  "",             "",                                  "System Costs",                             "*",                                                                "",              "" },
{ SSC_INPUT,     SSC_NUMBER, "rec_cost_exp",                       "Receiver cost scaling exponent",                                                                                                          "",             "",                                  "System Costs",                             "*",                                                                "",              "" },
{ SSC_INPUT,     SSC_NUMBER, "site_spec_cost",                     "Site improvement cost",                                                                                                                   "$/m2",         "",                                  "System Costs",                             "*",                                                                "",              "" },
{ SSC_INPUT,     SSC_NUMBER, "heliostat_spec_cost",                "Heliostat field cost",                                                                                                                    "$/m2",         "",                                  "System Costs",                             "*",                                                                "",              "" },
//{ SSC_INPUT,     SSC_NUMBER, "plant_spec_cost",                    "Power cycle specific cost",                                                                                                               "$/kWe",        "",                                  "System Costs",                             "*",                                                                "",              "" },
{ SSC_INPUT,     SSC_NUMBER, "bop_spec_cost",                      "BOS specific cost",                                                                                                                       "$/kWe",        "",                                  "System Costs",                             "*",                                                                "",              "" },
{ SSC_INPUT,     SSC_NUMBER, "tes_spec_cost",                      "Thermal energy storage cost",                                                                                                             "$/kWht",       "",                                  "System Costs",                             "*",                                                                "",              "" },
{ SSC_INPUT,     SSC_NUMBER, "land_spec_cost",                     "Total land area cost",                                                                                                                    "$/acre",       "",                                  "System Costs",                             "*",                                                                "",              "" },
{ SSC_INPUT,     SSC_NUMBER, "contingency_rate",                   "Contingency for cost overrun",                                                                                                            "%",            "",                                  "System Costs",                             "*",                                                                "",              "" },
{ SSC_INPUT,     SSC_NUMBER, "sales_tax_rate",                     "Sales tax rate",                                                                                                                          "%",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
{ SSC_INPUT,     SSC_NUMBER, "sales_tax_frac",                     "Percent of cost to which sales tax applies",                                                                                              "%",            "",                                  "System Costs",                             "*",                                                                "",              "" },
{ SSC_INPUT,     SSC_NUMBER, "cost_sf_fixed",                      "Solar field fixed cost",                                                                                                                  "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
//{ SSC_INPUT,     SSC_NUMBER, "fossil_spec_cost",                   "Fossil system specific cost",                                                                                                             "$/kWe",        "",                                  "System Costs",                             "*",                                                                "",              "" },
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
{ SSC_INPUT,     SSC_NUMBER, "const_per_interest_rate1",           "Interest rate, loan 1",                                                                                                                   "%",            "",                                  "Financial Parameters",                     "csp_financial_model=1",                                            "",              ""},
{ SSC_INPUT,     SSC_NUMBER, "const_per_interest_rate2",           "Interest rate, loan 2",                                                                                                                   "%",            "",                                  "Financial Parameters",                     "csp_financial_model=1",                                            "",              ""},
{ SSC_INPUT,     SSC_NUMBER, "const_per_interest_rate3",           "Interest rate, loan 3",                                                                                                                   "%",            "",                                  "Financial Parameters",                     "csp_financial_model=1",                                            "",              ""},
{ SSC_INPUT,     SSC_NUMBER, "const_per_interest_rate4",           "Interest rate, loan 4",                                                                                                                   "%",            "",                                  "Financial Parameters",                     "csp_financial_model=1",                                            "",              ""},
{ SSC_INPUT,     SSC_NUMBER, "const_per_interest_rate5",           "Interest rate, loan 5",                                                                                                                   "%",            "",                                  "Financial Parameters",                     "csp_financial_model=1",                                            "",              ""},
{ SSC_INPUT,     SSC_NUMBER, "const_per_months1",                  "Months prior to operation, loan 1",                                                                                                       "",             "",                                  "Financial Parameters",                     "csp_financial_model=1",                                            "",              ""},
{ SSC_INPUT,     SSC_NUMBER, "const_per_months2",                  "Months prior to operation, loan 2",                                                                                                       "",             "",                                  "Financial Parameters",                     "csp_financial_model=1",                                            "",              ""},
{ SSC_INPUT,     SSC_NUMBER, "const_per_months3",                  "Months prior to operation, loan 3",                                                                                                       "",             "",                                  "Financial Parameters",                     "csp_financial_model=1",                                            "",              ""},
{ SSC_INPUT,     SSC_NUMBER, "const_per_months4",                  "Months prior to operation, loan 4",                                                                                                       "",             "",                                  "Financial Parameters",                     "csp_financial_model=1",                                            "",              ""},
{ SSC_INPUT,     SSC_NUMBER, "const_per_months5",                  "Months prior to operation, loan 5",                                                                                                       "",             "",                                  "Financial Parameters",                     "csp_financial_model=1",                                            "",              ""},
{ SSC_INPUT,     SSC_NUMBER, "const_per_percent1",                 "Percent of total installed cost, loan 1",                                                                                                 "%",            "",                                  "Financial Parameters",                     "csp_financial_model=1",                                            "",              ""},
{ SSC_INPUT,     SSC_NUMBER, "const_per_percent2",                 "Percent of total installed cost, loan 2",                                                                                                 "%",            "",                                  "Financial Parameters",                     "csp_financial_model=1",                                            "",              ""},
{ SSC_INPUT,     SSC_NUMBER, "const_per_percent3",                 "Percent of total installed cost, loan 3",                                                                                                 "%",            "",                                  "Financial Parameters",                     "csp_financial_model=1",                                            "",              ""},
{ SSC_INPUT,     SSC_NUMBER, "const_per_percent4",                 "Percent of total installed cost, loan 4",                                                                                                 "%",            "",                                  "Financial Parameters",                     "csp_financial_model=1",                                            "",              ""},
{ SSC_INPUT,     SSC_NUMBER, "const_per_percent5",                 "Percent of total installed cost, loan 5",                                                                                                 "%",            "",                                  "Financial Parameters",                     "csp_financial_model=1",                                            "",              ""},
{ SSC_INPUT,     SSC_NUMBER, "const_per_upfront_rate1",            "Upfront fee on principal, loan 1",                                                                                                        "%",            "",                                  "Financial Parameters",                     "csp_financial_model=1",                                            "",              ""},
{ SSC_INPUT,     SSC_NUMBER, "const_per_upfront_rate2",            "Upfront fee on principal, loan 2",                                                                                                        "%",            "",                                  "Financial Parameters",                     "csp_financial_model=1",                                            "",              ""},
{ SSC_INPUT,     SSC_NUMBER, "const_per_upfront_rate3",            "Upfront fee on principal, loan 3",                                                                                                        "%",            "",                                  "Financial Parameters",                     "csp_financial_model=1",                                            "",              ""},
{ SSC_INPUT,     SSC_NUMBER, "const_per_upfront_rate4",            "Upfront fee on principal, loan 4",                                                                                                        "%",            "",                                  "Financial Parameters",                     "csp_financial_model=1",                                            "",              ""},
{ SSC_INPUT,     SSC_NUMBER, "const_per_upfront_rate5",            "Upfront fee on principal, loan 5",                                                                                                        "%",            "",                                  "Financial Parameters",                     "csp_financial_model=1",                                            "",              ""},

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
{ SSC_OUTPUT,    SSC_NUMBER, "system_capacity",                    "System capacity",                                                                                                                         "kWt",          "",                                  "System Costs",                             "*",                                                                "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "cp_system_nameplate",                 "System capacity for capacity payments",                                                                                                   "MWt",          "",                                  "System Costs",                             "*",                                                                "",              "" },
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
{ SSC_OUTPUT,    SSC_NUMBER, "E_heater_su_des",                    "Heater startup energy",                                                                                                                   "MWht",        "",                                  "Heater",                                   "*",                                                                "",              "" },

// Heat sink
{ SSC_OUTPUT,    SSC_NUMBER, "m_dot_hs_ext_des",                   "Heat sink fluid mass flow rate",                                                                                                          "kg/s",        "",                                  "System Control",                           "*",                                                                "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "T_hs_ext_out_des",                   "Heat sink fluid outlet temperature",                                                                                                      "C",           "",                                  "System Control",                           "*",                                                                "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "hx_min_dT_des",                      "Heat sink hx min temp difference",                                                                                                        "C",           "",                                  "System Control",                           "*",                                                                "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "hx_UA_des",                          "Heat sink hx conductance",                                                                                                                "MW/K",        "",                                  "System Control",                           "*",                                                                "",              "" },

// TES
{ SSC_OUTPUT,    SSC_NUMBER, "Q_tes_des",                          "TES design capacity",                                                                                                                     "MWht",         "",                                 "TES Design Calc",                          "*",                                                                "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "V_tes_htf_avail_des",                "TES volume of HTF available for heat transfer",                                                                                           "m3",           "",                                 "TES Design Calc",                          "*",                                                                "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "V_tes_htf_total_des",                "TES total HTF volume",                                                                                                                    "m3",           "",                                 "TES Design Calc",                          "*",                                                                "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "d_tank_tes",                         "TES tank diameter",                                                                                                                       "m",            "",                                 "TES Design Calc",                          "*",                                                                "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "q_dot_loss_tes_des",                 "TES thermal loss at design",                                                                                                              "MWt",          "",                                 "TES Design Calc",                          "*",                                                                "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "tshours_rec",                        "TES duration at receiver design output",                                                                                                  "hr",           "",                                 "TES Design Calc",                          "*",                                                                "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "tshours_heater",                     "TES duration at heater design output",                                                                                                    "hr",           "",                                 "TES Design Calc",                          "*",                                                                "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "dens_store_htf_at_T_ave",            "TES density of HTF at avg temps",                                                                                                         "kg/m3",        "",                                 "TES Design Calc",                          "*",                                                                "",              "" },

// Balance of Plant
{ SSC_OUTPUT,    SSC_NUMBER, "nameplate",                          "Nameplate capacity",                                                                                                                      "MWt",          "",                                 "System Design Calc",                       "*",                                                                "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "W_dot_bop_design",                   "BOP parasitics at design",                                                                                                                "MWe",          "",                                 "Balance of Plant",                         "*",                                                                "",              "" },
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
{ SSC_OUTPUT,    SSC_NUMBER, "csp.pt.cost.bop",                    "BOP cost",                                                                                                                                "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "csp.pt.cost.fossil",                 "Fossil backup cost",                                                                                                                      "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "ui_direct_subtotal",                 "Direct capital precontingency cost",                                                                                                      "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "csp.pt.cost.contingency",            "Contingency cost",                                                                                                                        "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "total_direct_cost",                  "Total direct cost",                                                                                                                       "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "csp.pt.cost.epc.total",              "EPC and owner cost",                                                                                                                      "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "csp.pt.cost.plm.total",              "Total land cost",                                                                                                                         "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "csp.pt.cost.sales_tax.total",        "Sales tax cost",                                                                                                                          "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "total_indirect_cost",                "Total indirect cost",                                                                                                                     "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "total_installed_cost",               "Total installed cost",                                                                                                                    "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "csp.pt.cost.installed_per_capacity", "Estimated installed cost per cap",                                                                                                        "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },

// Financing
{ SSC_OUTPUT,    SSC_NUMBER, "const_per_principal1",               "Principal, loan 1",                                                                                                                       "$",            "",                                  "Financial Parameters",                     "csp_financial_model=1",                                            "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "const_per_principal2",               "Principal, loan 2",                                                                                                                       "$",            "",                                  "Financial Parameters",                     "csp_financial_model=1",                                            "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "const_per_principal3",               "Principal, loan 3",                                                                                                                       "$",            "",                                  "Financial Parameters",                     "csp_financial_model=1",                                            "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "const_per_principal4",               "Principal, loan 4",                                                                                                                       "$",            "",                                  "Financial Parameters",                     "csp_financial_model=1",                                            "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "const_per_principal5",               "Principal, loan 5",                                                                                                                       "$",            "",                                  "Financial Parameters",                     "csp_financial_model=1",                                            "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "const_per_interest1",                "Interest cost, loan 1",                                                                                                                   "$",            "",                                  "Financial Parameters",                     "csp_financial_model=1",                                            "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "const_per_interest2",                "Interest cost, loan 2",                                                                                                                   "$",            "",                                  "Financial Parameters",                     "csp_financial_model=1",                                            "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "const_per_interest3",                "Interest cost, loan 3",                                                                                                                   "$",            "",                                  "Financial Parameters",                     "csp_financial_model=1",                                            "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "const_per_interest4",                "Interest cost, loan 4",                                                                                                                   "$",            "",                                  "Financial Parameters",                     "csp_financial_model=1",                                            "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "const_per_interest5",                "Interest cost, loan 5",                                                                                                                   "$",            "",                                  "Financial Parameters",                     "csp_financial_model=1",                                            "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "const_per_total1",                   "Total financing cost, loan 1",                                                                                                            "$",            "",                                  "Financial Parameters",                     "csp_financial_model=1",                                            "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "const_per_total2",                   "Total financing cost, loan 2",                                                                                                            "$",            "",                                  "Financial Parameters",                     "csp_financial_model=1",                                            "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "const_per_total3",                   "Total financing cost, loan 3",                                                                                                            "$",            "",                                  "Financial Parameters",                     "csp_financial_model=1",                                            "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "const_per_total4",                   "Total financing cost, loan 4",                                                                                                            "$",            "",                                  "Financial Parameters",                     "csp_financial_model=1",                                            "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "const_per_total5",                   "Total financing cost, loan 5",                                                                                                            "$",            "",                                  "Financial Parameters",                     "csp_financial_model=1",                                            "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "const_per_percent_total",            "Total percent of installed costs, all loans",                                                                                             "%",            "",                                  "Financial Parameters",                     "csp_financial_model=1",                                            "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "const_per_principal_total",          "Total principal, all loans",                                                                                                              "$",            "",                                  "Financial Parameters",                     "csp_financial_model=1",                                            "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "const_per_interest_total",           "Total interest costs, all loans",                                                                                                         "$",            "",                                  "Financial Parameters",                     "csp_financial_model=1",                                            "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "construction_financing_cost",        "Total construction financing cost",                                                                                                       "$",            "",                                  "Financial Parameters",                     "csp_financial_model=1",                                            "",              "" },
{ SSC_OUTPUT,    SSC_ARRAY,  "ppa_price_input",			           "PPA prices - yearly",			                                                                                                          "$/kWh",	      "",	                               "Revenue",			                       "ppa_multiplier_model=0&csp_financial_model<5&is_dispatch=1&sim_type=1",       "",   "SIMULATION_PARAMETER" },



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
{ SSC_OUTPUT,    SSC_ARRAY,  "q_dot_rec_inc",                      "Receiver incident thermal power",                                                                                                         "MWt",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
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

// Heat Sink
{ SSC_OUTPUT,   SSC_ARRAY,   "q_dot_to_heat_sink", "Heat sink thermal power",             "MWt",     "",  "Heat_Sink",      "sim_type=1",                       "",  "" },
{ SSC_OUTPUT,   SSC_ARRAY,   "W_dot_pc_pump",      "Heat sink pumping power",             "MWe",     "",  "Heat_Sink",      "sim_type=1",                       "",  "" },
{ SSC_OUTPUT,   SSC_ARRAY,   "m_dot_htf_heat_sink","Heat sink HTF mass flow",             "kg/s",    "",  "Heat_Sink",      "sim_type=1",                       "",  "" },
{ SSC_OUTPUT,   SSC_ARRAY,   "T_heat_sink_in",     "Heat sink HTF inlet temp",            "C",       "",  "Heat_Sink",      "sim_type=1",                       "",  "" },
{ SSC_OUTPUT,   SSC_ARRAY,   "T_heat_sink_out",    "Heat sink HTF outlet temp",           "C",       "",  "Heat_Sink",      "sim_type=1",                       "",  "" },

{ SSC_OUTPUT,   SSC_ARRAY,   "m_dot_wf_heat_sink", "Heat sink steam mass flow rate",      "kg/s",    "",  "Heat_Sink",      "sim_type=1&hs_type=1",             "",  "" },
{ SSC_OUTPUT,   SSC_ARRAY,   "x_out_wf_heat_sink", "Heat sink steam outlet quality",      "-",       "",  "Heat_Sink",      "sim_type=1&hs_type=1",             "",  "" },
{ SSC_OUTPUT,   SSC_ARRAY,   "T_out_wf_heat_sink", "Heat sink steam outlet temp",         "C",       "",  "Heat_Sink",      "sim_type=1&hs_type=1",             "",  "" },
{ SSC_OUTPUT,   SSC_ARRAY,   "hx_min_dT_heat_sink","Heat sink HX min temp difference",    "C",       "",  "Heat_Sink",      "sim_type=1&hs_type=1",             "",  "" },

// Thermal energy storage outputs
{ SSC_OUTPUT,    SSC_ARRAY,  "tank_losses",                        "TES thermal losses",                                                                                                                      "MWt",          "",                                  "",                                         "sim_type=1",                                                       "",              "" },
{ SSC_OUTPUT,    SSC_ARRAY,  "q_dot_tes_heater",                   "TES freeze protection power",                                                                                                             "MWt",          "",                                  "",                                         "sim_type=1",                                                       "",              "" },
{ SSC_OUTPUT,    SSC_ARRAY,  "T_tes_hot",                          "TES hot temperature",                                                                                                                     "C",            "",                                  "",                                         "sim_type=1",                                                       "",              "" },
{ SSC_OUTPUT,    SSC_ARRAY,  "T_tes_cold",                         "TES cold temperature",                                                                                                                    "C",            "",                                  "",                                         "sim_type=1",                                                       "",              "" },
{ SSC_OUTPUT,   SSC_ARRAY,   "mass_tes_cold",                      "TES cold tank mass (end)",                                                                                                                "kg",           "",                                  "",                                         "sim_type=1",                                                       "",              "" },
{ SSC_OUTPUT,   SSC_ARRAY,   "mass_tes_hot",                       "TES hot tank mass (end)",                                                                                                                 "kg",           "",                                  "",                                         "sim_type=1",                                                       "",              "" },
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
{ SSC_OUTPUT,    SSC_ARRAY,  "W_dot_parasitic_tot",                "System total electrical parasitic",                                                                                                       "MWe",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},

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

{ SSC_OUTPUT,    SSC_ARRAY,  "gen_heat",                           "System net thermal power w/ avail. derate",                                                                                               "kWt",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
{ SSC_OUTPUT,    SSC_ARRAY,  "gen",                                "System net electrical power w/ avail. derate",                                                                                            "kWe",          "",                                  "system",                                   "sim_type=1",                                                       "",              "" },
{ SSC_OUTPUT,    SSC_ARRAY,  "gen_heat_btu",                       "System net thermal power w/ avail. derate",                                                                                               "MMBtu/hr",     "",                                  "system",                                   "sim_type=1",                                                       "",              "" },


// Annual single-value outputs
{ SSC_OUTPUT,    SSC_NUMBER, "annual_energy",                      "Annual net thermal energy w/ avail. derate",                                                                                              "kWht",         "",                                  "Post-process",                             "sim_type=1",                                                       "",              ""},
{ SSC_OUTPUT,    SSC_NUMBER, "annual_q_rec_htf",                   "Annual receiver power delivered to HTF",                                                                                                  "MWht",         "",                                  "Tower and Receiver",                       "sim_type=1",                                                       "",              ""},
{ SSC_OUTPUT,    SSC_NUMBER, "annual_energy_heat_btu",             "Annual net thermal energy w/ avail. derate",                                                                                              "MMBtu",        "",                                  "Post-process",                             "sim_type=1",                                                       "",              ""},


{ SSC_OUTPUT,    SSC_NUMBER, "annual_electricity_consumption",     "Annual electricity consumption w/ avail derate",                                      "kWhe",       "",                    "Post-process",          "sim_type=1",                "",              ""},

{ SSC_OUTPUT,    SSC_NUMBER, "capacity_factor",                    "Capacity factor",                                                                     "%",            "",                    "Post-process",          "sim_type=1",                "",              ""},
{ SSC_OUTPUT,    SSC_NUMBER, "kwh_per_kw",                         "First year kWh/kW",                                                                   "kWth/kWt",     "",                    "Post-process",          "sim_type=1",                "",              ""},

{ SSC_OUTPUT,    SSC_NUMBER, "annual_total_water_use",             "Total annual water usage from mirror washing",                                        "m3",           "",                    "Post-process",          "sim_type=1",                "",              ""},

{ SSC_OUTPUT,    SSC_NUMBER, "annual_q_rec_inc",                   "Annual receiver incident thermal power after reflective losses",                                                                          "MWht",         "",                                  "Tower and Receiver",                       "sim_type=1",                                                       "",              ""},
{ SSC_OUTPUT,    SSC_NUMBER, "annual_q_rec_loss",                  "Annual receiver convective and radiative losses",                                                                                         "MWht",         "",                                  "Tower and Receiver",                       "sim_type=1",                                                       "",              ""},
{ SSC_OUTPUT,    SSC_NUMBER, "annual_q_piping_loss",               "Annual tower piping losses",                                                                                                              "MWht",         "",                                  "Tower and Receiver",                       "sim_type=1",                                                       "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "annual_q_rec_startup",               "Annual receiver startup energy",                                                                                                          "MWht",         "",                                  "Tower and Receiver",                       "sim_type=1",                                                       "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "annual_E_tower_pump",                "Annual tower pumping power",                                                                                                              "MWhe",         "",                                  "Tower and Receiver",                       "sim_type=1",                                                       "",              "" },
{ SSC_OUTPUT,    SSC_NUMBER, "annual_eta_rec_th",                  "Annual receiver thermal efficiency ignoring rec reflective loss",                                                                         "",             "",                                  "Tower and Receiver",                       "sim_type=1",                                                       "",              ""},
{ SSC_OUTPUT,    SSC_NUMBER, "annual_eta_rec_th_incl_refl",        "Annual receiver thermal efficiency including reflective loss",                                                                            "",             "",                                  "Tower and Receiver",                       "sim_type=1",                                                       "",              ""},
{ SSC_OUTPUT,    SSC_NUMBER, "annual_q_defocus_est",               "Annual defocus loss estimate",                                                                                                            "MWht",         "",                                  "Tower and Receiver",                       "sim_type=1",                                                       "",              "" },

{ SSC_OUTPUT,    SSC_NUMBER, "sim_cpu_run_time",                   "Simulation duration clock time",                                                                                                         "s",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},

var_info_invalid };

class cm_mspt_iph : public compute_module
{
public:

    cm_mspt_iph()
    {
        add_var_info(_cm_vtab_mspt_iph);
        add_var_info(vtab_adjustment_factors);
        add_var_info(vtab_sf_adjustment_factors);
        add_var_info(vtab_technology_outputs);
    }

    bool relay_message(string& msg, double percent)
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
        // *****************************************************

        // Convert IPH Input Units
        const double MMBTU_TO_KWh = 293.07107; // 1
        if (is_assigned("ppa_price_input_heat_btu"))
        {
            size_t count_ppa_price_MMBTU_input;
            ssc_number_t* ppa_price_MMBTU_input_array = as_array("ppa_price_input_heat_btu", &count_ppa_price_MMBTU_input);
            std::vector<ssc_number_t> ppa_price_input_vec;
            for (int i = 0; i < count_ppa_price_MMBTU_input; i++)
            {
                ppa_price_input_vec.push_back(ppa_price_MMBTU_input_array[i] / MMBTU_TO_KWh);
            }

            int size = ppa_price_input_vec.size();
            ssc_number_t* alloc_vals = allocate("ppa_price_input", size);
            for (int i = 0; i < size; i++)
                alloc_vals[i] = ppa_price_input_vec[i];    // []
        }
        

        // *****************************************************
        // System Design Parameters
        double T_htf_cold_des = as_double("T_htf_cold_des");    //[C]
        double T_htf_hot_des = as_double("T_htf_hot_des");      //[C]
        double tshours = as_double("tshours");                  //[-]

        // System Design Calcs
        double q_dot_pc_des = as_double("q_pb_design");         //[MWt] HEAT SINK design thermal power
        double Q_tes = q_dot_pc_des * tshours;                  //[MWht]
        double q_dot_rec_des = q_dot_pc_des * as_number("solarm");  //[MWt]

        // Weather reader
        C_csp_weatherreader weather_reader;
        if (is_assigned("solar_resource_file")) {
            weather_reader.m_weather_data_provider = make_shared<weatherfile>(as_string("solar_resource_file"));
            if (weather_reader.m_weather_data_provider->has_message()) log(weather_reader.m_weather_data_provider->message(), SSC_WARNING);
        }
        if (is_assigned("solar_resource_data")) {
            weather_reader.m_weather_data_provider = make_shared<weatherdata>(lookup("solar_resource_data"));
            if (weather_reader.m_weather_data_provider->has_message()) log(weather_reader.m_weather_data_provider->message(), SSC_WARNING);
        }

        size_t n_steps_full = weather_reader.m_weather_data_provider->nrecords(); //steps_per_hour * 8760;
        weather_reader.m_trackmode = 0;
        weather_reader.m_tilt = 0.0;
        weather_reader.m_azimuth = 0.0;
        // Initialize to get weather file info
        weather_reader.init();
        if (weather_reader.has_error()) throw exec_error("mspt_iph", weather_reader.get_error());

        // Get info from the weather reader initialization
        double site_elevation = weather_reader.ms_solved_params.m_elev;     //[m]

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
                throw exec_error("mspt_iph", "receiver_type must be 1 (external) or 0 (cavity)");
            }

            if ((field_model_type == 0 || field_model_type == 1) && sim_type == 1) // Auto-design. Generate a new system (is_optimize = true) or field layout
            {
                // What if sim_type = 2?
                // If from the UI, then probably never want to actually layout the field and geometry
                //   because there are macros for that
                // If called from a script.... then maybe?
                // So.. maybe add optional input like "is_layout_field_in_design_only" and default to False?

                if (field_model_type == 0)
                {
                    assign("is_optimize", 1);
                    is_optimize = true;
                }

                assign("calc_fluxmaps", 1);

                spi.run(weather_reader.m_weather_data_provider);

                if (is_optimize)
                {
                    //Optimization iteration history
                    vector<vector<double> > steps;
                    vector<double> obj, flux;
                    spi.getOptimizationSimulationHistory(steps, obj, flux);
                    size_t nr = steps.size();
                    if (nr > 0)
                    {
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

                // receiver calculations
                double H_rec = spi.recs.front().rec_height.val;

                //collect the optical efficiency data and sun positions
                if (spi.fluxtab.zeniths.size() > 0 && spi.fluxtab.azimuths.size() > 0
                    && spi.fluxtab.efficiency.size() > 0)
                {
                    size_t nvals = spi.fluxtab.efficiency.size();
                    mt_eta_map.resize(nvals, 3);

                    for (size_t i = 0; i < nvals; i++)
                    {
                        mt_eta_map(i, 0) = spi.fluxtab.azimuths[i] * 180. / CSP::pi;      //Convention is usually S=0, E<0, W>0 
                        mt_eta_map(i, 1) = spi.fluxtab.zeniths[i] * 180. / CSP::pi;     //Provide zenith angle
                        mt_eta_map(i, 2) = spi.fluxtab.efficiency[i];
                    }
                }
                else
                    throw exec_error("solarpilot", "failed to calculate a correct optical efficiency table");

                //collect the flux map data
                block_t<double>* flux_data = &spi.fluxtab.flux_surfaces.front().flux_data;  //there should be only one flux stack for SAM
                if (flux_data->ncols() > 0 && flux_data->nlayers() > 0)
                {
                    if (rec_type == 0) {

                        int nflux_y = (int)flux_data->nrows();
                        int nflux_x = (int)flux_data->ncols();

                        mt_flux_maps.resize(nflux_y * flux_data->nlayers(), nflux_x);

                        int cur_row = 0;

                        for (size_t i = 0; i < flux_data->nlayers(); i++)
                        {
                            for (int j = 0; j < nflux_y; j++)
                            {
                                for (int k = 0; k < nflux_x; k++)
                                {
                                    mt_flux_maps(cur_row, k) = flux_data->at(j, k, i);
                                    //fluxdata[cur_row * nflux_x + k] = (float)flux_data->at(j, k, i);
                                }
                                cur_row++;
                            }
                        }
                    }
                    else if (rec_type == 1) {

                        int nflux_y = (int)flux_data->nrows();
                        int nflux_x = (int)flux_data->ncols();

                        int n_panels_cav = as_integer("n_cav_rec_panels");  //[-]
                        int n_sp_surfaces = spi.fluxtab.flux_surfaces.size();
                        int n_panels_cav_sp = n_sp_surfaces - 1;

                        if (nflux_y > 1) {
                            throw exec_error("solarpilot", "cavity flux maps currently only work for nflux_y = 1");
                        }

                        mt_flux_maps.resize(nflux_y * flux_data->nlayers(), n_panels_cav_sp);

                        int cur_row = 0;

                        // nlayers is number of solar positions (i.e. flux maps)
                        for (size_t i = 0; i < flux_data->nlayers(); i++) {

                            int j = 0;

                            double flux_receiver = 0.0;

                            // Start at k=1 because the first surface in flux_surfaces is the aperture, which we don't want
                            for (int k = 1; k <= n_panels_cav_sp; k++) {

                                block_t<double>* flux_data = &spi.fluxtab.flux_surfaces[k].flux_data; //.front().flux_data;  //there should be only one flux stack for SAM

                                double flux_local = 0.0;
                                for (int l = 0; l < nflux_x; l++) {
                                    //double flux_local0 = flux_data->at(j, 0, i);
                                    //double flux_local1 = flux_data->at(j, 1, i);
                                    //double flux_local2 = flux_data->at(j, 2, i);
                                    //double flux_local3 = flux_data->at(j, 3, i);

                                    flux_local += flux_data->at(j, l, i);
                                }

                                // Adjust k to start flux maps with first receiver surface
                                mt_flux_maps(cur_row, k - 1) = flux_local;
                                flux_receiver += flux_local;
                                double abc = 1.23;
                            }

                            cur_row++;
                        }
                    }
                }
                else
                    throw exec_error("solarpilot", "failed to calculate a correct flux map table");
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
                else if (sim_type == 2 || field_model_type == 3) {
                    assign("calc_fluxmaps", 0);
                }

                spi.run(weather_reader.m_weather_data_provider);

                if (sim_type == 1 && field_model_type == 2) {
                    //collect the optical efficiency data and sun positions
                    if (spi.fluxtab.zeniths.size() > 0 && spi.fluxtab.azimuths.size() > 0
                        && spi.fluxtab.efficiency.size() > 0)
                    {
                        size_t nvals = spi.fluxtab.efficiency.size();
                        mt_eta_map.resize(nvals, 3);

                        for (size_t i = 0; i < nvals; i++)
                        {
                            mt_eta_map(i, 0) = spi.fluxtab.azimuths[i] * 180. / CSP::pi;    //Convention is usually S=0, E<0, W>0 
                            mt_eta_map(i, 1) = spi.fluxtab.zeniths[i] * 180. / CSP::pi;     //Provide zenith angle
                            mt_eta_map(i, 2) = spi.fluxtab.efficiency[i];
                        }
                    }
                    else
                        throw exec_error("solarpilot", "failed to calculate a correct optical efficiency table");

                    //collect the flux map data
                    block_t<double>* flux_data = &spi.fluxtab.flux_surfaces.front().flux_data;  //there should be only one flux stack for SAM
                    if (flux_data->ncols() > 0 && flux_data->nlayers() > 0)
                    {
                        if (rec_type == 0) {

                            int nflux_y = (int)flux_data->nrows();
                            int nflux_x = (int)flux_data->ncols();

                            mt_flux_maps.resize(nflux_y * flux_data->nlayers(), nflux_x);

                            int cur_row = 0;

                            for (size_t i = 0; i < flux_data->nlayers(); i++)
                            {
                                for (int j = 0; j < nflux_y; j++)
                                {
                                    for (int k = 0; k < nflux_x; k++)
                                    {
                                        mt_flux_maps(cur_row, k) = flux_data->at(j, k, i);
                                        //fluxdata[cur_row * nflux_x + k] = (float)flux_data->at(j, k, i);
                                    }
                                    cur_row++;
                                }
                            }
                        }
                        else if (rec_type == 1) {

                            int nflux_y = (int)flux_data->nrows();
                            int nflux_x = (int)flux_data->ncols();

                            int n_panels_cav = as_integer("n_cav_rec_panels"); //[-]
                            int n_sp_surfaces = spi.fluxtab.flux_surfaces.size();
                            int n_panels_cav_sp = n_sp_surfaces - 1;

                            if (nflux_y > 1) {
                                throw exec_error("solarpilot", "cavity flux maps currently only work for nflux_y = 1");
                            }

                            mt_flux_maps.resize(nflux_y * flux_data->nlayers(), n_panels_cav_sp);

                            int cur_row = 0;

                            // nlayers is number of solar positions (i.e. flux maps)
                            for (size_t i = 0; i < flux_data->nlayers(); i++) {

                                int j = 0;

                                double flux_receiver = 0.0;

                                // Start at k=1 because the first surface in flux_surfaces is the aperture, which we don't want
                                for (int k = 1; k <= n_panels_cav_sp; k++) {

                                    block_t<double>* flux_data = &spi.fluxtab.flux_surfaces[k].flux_data; //.front().flux_data;  //there should be only one flux stack for SAM

                                    double flux_local = 0.0;
                                    for (int l = 0; l < nflux_x; l++) {
                                        flux_local += flux_data->at(j, l, i);
                                    }

                                    // Adjust k to start flux maps with first receiver surface
                                    mt_flux_maps(cur_row, k - 1) = flux_local;
                                    flux_receiver += flux_local;
                                }
                                // flux_receiver should equal 1 after each panel is added

                                cur_row++;
                            }
                        }
                    }
                    else
                        throw exec_error("solarpilot", "failed to calculate a correct flux map table");

                }
                else if (field_model_type == 3) {

                    mt_eta_map = as_matrix("eta_map");
                    mt_flux_maps = as_matrix("flux_maps");
                }
                else if (field_model_type == 2 && sim_type == 2) {

                    mt_eta_map.resize_fill(1, 3, std::numeric_limits<double>::quiet_NaN());
                    mt_flux_maps.resize_fill(1, 12, std::numeric_limits<double>::quiet_NaN());

                }
                else {
                    string msg = util::format("Invalid combination of field_model_type and sim_type");

                    throw exec_error("MSPT CSP Solver", msg);
                }
            }

            N_hel = (int)spi.layout.heliostat_positions.size();

            helio_pos.resize(N_hel, 2);
            for (int i = 0; i < N_hel; i++) {
                helio_pos(i, 0) = (ssc_number_t)spi.layout.heliostat_positions.at(i).location.x;
                helio_pos(i, 0) = (ssc_number_t)spi.layout.heliostat_positions.at(i).location.y;
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
            mt_flux_maps = as_matrix("flux_maps");

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


        if (tes_type != 1)
        {
            throw exec_error("MSPT CSP Solver", "Thermocline thermal energy storage is not yet supported by the new CSP Solver and Dispatch Optimization models.\n");
        }


        // Set steps per hour
        C_csp_solver::S_sim_setup sim_setup;
        sim_setup.m_sim_time_start = as_double("time_start");       //[s] time at beginning of first time step
        sim_setup.m_sim_time_end = as_double("time_stop");          //[s] time at end of last time step

        int steps_per_hour = (int)as_double("time_steps_per_hour");     //[-]

        //if the number of steps per hour is not provided (=-1), then assign it based on the weather file step
        if (steps_per_hour < 0)
        {
            double sph_d = 3600. / weather_reader.m_weather_data_provider->step_sec();
            steps_per_hour = (int)(sph_d + 1.e-5);
            if ((double)steps_per_hour != sph_d)
                throw spexception("The time step duration must be evenly divisible within an hour.");
        }

        size_t n_steps_fixed = (size_t)steps_per_hour * 8760;   //[-]
        if (as_boolean("vacuum_arrays"))
        {
            n_steps_fixed = steps_per_hour * (size_t)((sim_setup.m_sim_time_end - sim_setup.m_sim_time_start) / 3600.);
        }
        //int n_steps_fixed = (int)( (sim_setup.m_sim_time_end - sim_setup.m_sim_time_start) * steps_per_hour / 3600. ) ; 
        sim_setup.m_report_step = 3600.0 / (double)steps_per_hour;  //[s]

        // ***********************************************
        // ***********************************************
        // Heat Sink
        // ***********************************************
        // ***********************************************

        int hs_type = as_integer("hs_type");
        C_csp_power_cycle* c_heat_sink_pointer = nullptr;
        C_pc_heat_sink c_heat_sink;
        C_pc_heat_sink_physical c_heat_sink_phys;

        //size_t n_f_turbine1 = 0;
        //ssc_number_t* p_f_turbine1 = as_array("f_turb_tou_periods", &n_f_turbine1);   // heat sink, not turbine
        //double f_turbine_max1 = 1.0;
        //for (size_t i = 0; i < n_f_turbine1; i++) {
        //    f_turbine_max1 = max(f_turbine_max1, p_f_turbine1[i]);
        //}

            // Ideal heat sink
        if (hs_type == 0)
        {
            c_heat_sink.ms_params.m_T_htf_hot_des = T_htf_hot_des;		//[C] FIELD design outlet temperature
            c_heat_sink.ms_params.m_T_htf_cold_des = T_htf_cold_des;	//[C] FIELD design inlet temperature
            c_heat_sink.ms_params.m_q_dot_des = q_dot_pc_des;			//[MWt] HEAT SINK design thermal power (could have field solar multiple...)
            // 9.18.2016 twn: assume for now there's no pressure drop though heat sink
            c_heat_sink.ms_params.m_htf_pump_coef = as_double("pb_pump_coef");		//[kWe/kg/s]
            c_heat_sink.ms_params.m_max_frac = as_double("hs_htf_mdot_max_frac");   // f_turbine_max1;

            c_heat_sink.ms_params.m_pc_fl = as_integer("rec_htf");
            c_heat_sink.ms_params.m_pc_fl_props = as_matrix("field_fl_props");


            // Allocate heat sink outputs
            c_heat_sink.mc_reported_outputs.assign(C_pc_heat_sink::E_Q_DOT_HEAT_SINK, allocate("q_dot_to_heat_sink", n_steps_fixed), n_steps_fixed);
            c_heat_sink.mc_reported_outputs.assign(C_pc_heat_sink::E_W_DOT_PUMPING, allocate("W_dot_pc_pump", n_steps_fixed), n_steps_fixed);
            c_heat_sink.mc_reported_outputs.assign(C_pc_heat_sink::E_M_DOT_HTF, allocate("m_dot_htf_heat_sink", n_steps_fixed), n_steps_fixed);
            c_heat_sink.mc_reported_outputs.assign(C_pc_heat_sink::E_T_HTF_IN, allocate("T_heat_sink_in", n_steps_fixed), n_steps_fixed);
            c_heat_sink.mc_reported_outputs.assign(C_pc_heat_sink::E_T_HTF_OUT, allocate("T_heat_sink_out", n_steps_fixed), n_steps_fixed);

            c_heat_sink_pointer = &c_heat_sink;
        }
        else if (hs_type == 1)
        {
            //size_t n_f_turbine1 = 0;
            //ssc_number_t* p_f_turbine1 = as_array("f_turb_tou_periods", &n_f_turbine1);   // heat sink, not turbine
            //double f_turbine_max1 = 1.0;
            //for (size_t i = 0; i < n_f_turbine1; i++) {
            //    f_turbine_max1 = max(f_turbine_max1, p_f_turbine1[i]);
            //}

            c_heat_sink_phys.ms_params.m_T_htf_hot_des = T_htf_hot_des;		//[C] FIELD design outlet temperature
            c_heat_sink_phys.ms_params.m_T_htf_cold_des = T_htf_cold_des;	//[C] FIELD design inlet temperature
            c_heat_sink_phys.ms_params.m_q_dot_des = q_dot_pc_des;			//[MWt] HEAT SINK design thermal power (could have field solar multiple...)
            // 9.18.2016 twn: assume for now there's no pressure drop though heat sink
            c_heat_sink_phys.ms_params.m_htf_pump_coef = as_double("pb_pump_coef");		//[kWe/kg/s]
            c_heat_sink_phys.ms_params.m_max_frac = as_double("hs_htf_mdot_max_frac");  // f_turbine_max1;

            c_heat_sink_phys.ms_params.m_pc_fl = as_integer("rec_htf");
            c_heat_sink_phys.ms_params.m_pc_fl_props = as_matrix("field_fl_props");

            c_heat_sink_phys.ms_params.m_T_ext_cold_des = as_double("hs_phys_T_steam_cold_des");   //[C] Steam fluid inlet temp
            c_heat_sink_phys.ms_params.m_Q_ext_hot_des = as_double("hs_phys_Q_steam_hot_des");  //[C] Steam quality target
            c_heat_sink_phys.ms_params.m_P_ext_cold_des = as_double("hs_phys_P_steam_hot_des") * 100.0;  //[kPa] Steam inlet pressure
            c_heat_sink_phys.ms_params.m_P_ext_hot_des = as_double("hs_phys_P_steam_hot_des") * 100.0;   //[kPa] Steam outlet pressure

            // 25.02.03 twn: hardcode these for now so users don't have to set them
            c_heat_sink_phys.ms_params.m_f_m_dot_ext_min = 0.01;    // as_double("hs_phys_f_mdot_steam_min"); //[] Min fraction Steam mass flow rate
            c_heat_sink_phys.ms_params.m_f_m_dot_ext_max = 1.5;     // as_double("hs_phys_f_mdot_steam_max"); //[] Max fraction Steam mass flow rate
            c_heat_sink_phys.ms_params.m_N_sub_hx = 15;             // as_double("hs_phys_N_sub");      //[] Number HX nodes
            c_heat_sink_phys.ms_params.m_od_tol = 0.001;            // as_double("hs_phys_tol");         //[] HX off design tolerance

            // Allocate heat sink outputs
            c_heat_sink_phys.mc_reported_outputs.assign(C_pc_heat_sink::E_Q_DOT_HEAT_SINK, allocate("q_dot_to_heat_sink", n_steps_fixed), n_steps_fixed);
            c_heat_sink_phys.mc_reported_outputs.assign(C_pc_heat_sink::E_W_DOT_PUMPING, allocate("W_dot_pc_pump", n_steps_fixed), n_steps_fixed);
            c_heat_sink_phys.mc_reported_outputs.assign(C_pc_heat_sink::E_M_DOT_HTF, allocate("m_dot_htf_heat_sink", n_steps_fixed), n_steps_fixed);
            c_heat_sink_phys.mc_reported_outputs.assign(C_pc_heat_sink::E_T_HTF_IN, allocate("T_heat_sink_in", n_steps_fixed), n_steps_fixed);
            c_heat_sink_phys.mc_reported_outputs.assign(C_pc_heat_sink::E_T_HTF_OUT, allocate("T_heat_sink_out", n_steps_fixed), n_steps_fixed);

            c_heat_sink_phys.mc_reported_outputs.assign(C_pc_heat_sink_physical::E_M_DOT_EXT, allocate("m_dot_wf_heat_sink", n_steps_fixed), n_steps_fixed);
            c_heat_sink_phys.mc_reported_outputs.assign(C_pc_heat_sink_physical::E_X_OUT_EXT, allocate("x_out_wf_heat_sink", n_steps_fixed), n_steps_fixed);
            c_heat_sink_phys.mc_reported_outputs.assign(C_pc_heat_sink_physical::E_T_OUT_EXT, allocate("T_out_wf_heat_sink", n_steps_fixed), n_steps_fixed);
            c_heat_sink_phys.mc_reported_outputs.assign(C_pc_heat_sink_physical::E_HX_MIN_DT, allocate("hx_min_dT_heat_sink", n_steps_fixed), n_steps_fixed);

            c_heat_sink_pointer = &c_heat_sink_phys;
        }
        else
        {
            throw exec_error("mspt_iph", "hs_type must be 0-1");
        }
        if (c_heat_sink_pointer == nullptr)
        {
            throw exec_error("mspt_iph", "Heat sink pointer not assigned");
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
        if (rec_type == 0) {
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
            double rec_span = as_double("cav_rec_span") * CSP::pi / 180.0;  //[rad] convert from cmod units of deg

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
                as_double("eta_pump")));

            receiver = std::move(c_cav_rec);

            // Calculate receiver area
            double theta0, panelspan, panelwidth, radius, offset;
            theta0 = panelspan = panelwidth = radius = offset = A_rec = std::numeric_limits<double>::quiet_NaN();
            cavity_receiver_helpers::calc_receiver_macro_geometry(cav_rec_height, cav_rec_width,
                rec_span, nPanels,
                theta0, panelspan, panelwidth, A_rec, radius, offset);

            cav_rec_area = A_rec;           //[m2]
            cav_panel_width = panelwidth;   //[m]
            cav_radius = radius;            //[m]
        }
        else if (rec_type == 0) {

            int rec_night_recirc = 0;
            int rec_clearsky_model = as_integer("rec_clearsky_model");
            bool is_calc_od_tube = false;
            double W_dot_rec_target = std::numeric_limits<double>::quiet_NaN();

            if (rec_clearsky_model > 4)
                throw exec_error("mspt_iph", "Invalid specification for 'rec_clearsky_model'");
            if (rec_clearsky_model == -1 && as_double("rec_clearsky_fraction") >= 0.0001)
                throw exec_error("mspt_iph", "'rec_clearsky_model' must be specified when 'rec_clearsky_fraction' > 0.0.");

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

                receiver = std::move(ss_receiver);
            }
            else {

                bool is_enforce_min_startup = as_boolean("is_rec_enforce_min_startup");

                //trans_receiver->m_is_startup_from_solved_profile = as_boolean("is_rec_startup_from_T_soln");
                if (as_boolean("is_rec_startup_trans") && as_boolean("is_rec_startup_from_T_soln"))
                    throw exec_error("mspt_iph", "Receiver startup from solved temperature profiles is only available when receiver transient startup model is enabled");

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

        heliostatfield.ms_params.m_p_start = as_double("p_start");      //[kWhe] Heliostat startup energy
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
                throw exec_error("mspt_iph", "Invalid clear-sky DNI data. Array must have " + util::to_string((int)n_steps_full) + " rows.");

            clearsky_data.resize(n_steps_full);
            for (size_t i = 0; i < n_steps_full; i++)
                clearsky_data.at(i) = (double)csky[i];
        }
        heliostatfield.ms_params.mv_clearsky_data = clearsky_data;

        //Load the solar field adjustment factors
        adjustment_factors sf_haf(this->get_var_table(), "sf_adjust");
        if (!sf_haf.setup((int)n_steps_full))
            throw exec_error("mspt_iph", "failed to setup sf adjustment factors: " + sf_haf.error());
        //allocate array to pass to tcs
        heliostatfield.ms_params.m_sf_adjust.resize(sf_haf.size());
        for (int i = 0; i < sf_haf.size(); i++)
            heliostatfield.ms_params.m_sf_adjust.at(i) = sf_haf(i);

        // Set callback information
        heliostatfield.mf_callback = ssc_cmod_solarpilot_callback;
        heliostatfield.m_cdata = (void*)this;

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
        //float *p_q_thermal_copy = allocate("Q_thermal_123", n_steps_fixed);
        collector_receiver.mc_reported_outputs.assign(C_csp_mspt_collector_receiver::E_FIELD_Q_DOT_INC, allocate("q_sf_inc", n_steps_fixed), n_steps_fixed);
        collector_receiver.mc_reported_outputs.assign(C_csp_mspt_collector_receiver::E_FIELD_ETA_OPT, allocate("eta_field", n_steps_fixed), n_steps_fixed);
        collector_receiver.mc_reported_outputs.assign(C_csp_mspt_collector_receiver::E_FIELD_ADJUST, allocate("sf_adjust_out", n_steps_fixed), n_steps_fixed);

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
                    throw exec_error("mspt_iph", "When the molten salt power tower case has an electric HTF charger, dispatch optimization must be selected");
                }
            }

            double heater_mult = as_double("heater_mult");      //[-]
            heater_spec_cost = as_double("heater_spec_cost");   //[$/kWt]

            q_dot_heater_des = q_dot_pc_des * heater_mult;     //[MWt]
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
            q_dot_pc_des,   //[MWt]
            as_double("solarm"),                            //[-]
            Q_tes,
            true,
            as_double("h_tank"),
            0.0,
            as_double("u_tank"),
            as_integer("tank_pairs"),
            as_double("hot_tank_Thtr"),
            as_double("hot_tank_max_heat"),
            as_double("cold_tank_Thtr"),
            as_double("cold_tank_max_heat"),
            0.0,                                    // MSPT assumes direct storage, so no user input here: hardcode = 0.0
            as_double("T_htf_cold_des"),
            as_double("T_htf_hot_des"),
            as_double("T_htf_hot_des"),
            as_double("T_htf_cold_des"),
            as_double("h_tank_min"),
            as_double("tes_init_hot_htf_percent"),
            as_double("pb_pump_coef"),
            as_boolean("tanks_in_parallel"),        //[-]       
            1.85,                                   //[m/s]
            false                                   // for now, to get 'tanks_in_parallel' to work
        );

        // Set storage outputs
        storage.mc_reported_outputs.assign(C_csp_two_tank_tes::E_Q_DOT_LOSS, allocate("tank_losses", n_steps_fixed), n_steps_fixed);
        storage.mc_reported_outputs.assign(C_csp_two_tank_tes::E_W_DOT_HEATER, allocate("q_dot_tes_heater", n_steps_fixed), n_steps_fixed);
        storage.mc_reported_outputs.assign(C_csp_two_tank_tes::E_TES_T_HOT, allocate("T_tes_hot", n_steps_fixed), n_steps_fixed);
        storage.mc_reported_outputs.assign(C_csp_two_tank_tes::E_TES_T_COLD, allocate("T_tes_cold", n_steps_fixed), n_steps_fixed);
        storage.mc_reported_outputs.assign(C_csp_two_tank_tes::E_MASS_COLD_TANK, allocate("mass_tes_cold", n_steps_fixed), n_steps_fixed);
        storage.mc_reported_outputs.assign(C_csp_two_tank_tes::E_MASS_HOT_TANK, allocate("mass_tes_hot", n_steps_fixed), n_steps_fixed);
        storage.mc_reported_outputs.assign(C_csp_two_tank_tes::E_W_DOT_HTF_PUMP, allocate("tes_htf_pump_power", n_steps_fixed), n_steps_fixed);

        // ********************************
        // Schedules
        // ********************************

        // Off-taker schedule
        C_timeseries_schedule_inputs offtaker_schedule;
        bool is_timestep_load_fractions = as_boolean("is_timestep_load_fractions");
        if (is_timestep_load_fractions) {
            auto vec = as_vector_double("timestep_load_fractions");
            C_timeseries_schedule_inputs offtaker_series = C_timeseries_schedule_inputs(vec, std::numeric_limits<double>::quiet_NaN());
            offtaker_schedule = offtaker_series;
        }
        else {      // Block schedules
            C_timeseries_schedule_inputs offtaker_block = C_timeseries_schedule_inputs(as_matrix("weekday_schedule"),
                as_matrix("weekend_schedule"), as_vector_double("f_turb_tou_periods"), std::numeric_limits<double>::quiet_NaN());
            offtaker_schedule = offtaker_block;
        }

        // Electricity pricing schedule
        C_timeseries_schedule_inputs elec_pricing_schedule;
        int csp_financial_model = as_integer("csp_financial_model");

        if (sim_type == 1) {
            if (csp_financial_model == 8 || csp_financial_model == 7) {        // No Financial Model or LCOH
                if (is_dispatch) {
                    throw exec_error("mspt_iph", "Can't select dispatch optimization if No Financial or LCOH model");
                }
                else { // if no dispatch optimization, don't need an input pricing schedule
                    // If electricity pricing data is not available, then dispatch to a uniform schedule
                    elec_pricing_schedule = C_timeseries_schedule_inputs(-1.0, std::numeric_limits<double>::quiet_NaN());
                }
            }
            else if (csp_financial_model == 1) {   // Single owner

                double ppa_price_year1 = std::numeric_limits<double>::quiet_NaN();

                // Get first year base ppa price
                bool is_ppa_price_input_assigned = is_assigned("ppa_price_input");
                if (is_dispatch && !is_ppa_price_input_assigned) {
                    throw exec_error("mspt_iph", "\n\nYou selected dispatch optimization which requires that the array input ppa_price_input is defined\n");
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
                    throw exec_error("mspt_iph", "\n\nYou selected dispatch optimization and the Specify IRR Target financial solution mode, "
                        "but dispatch optimization requires known absolute electricity prices. Dispatch optimization requires "
                        "the Specify PPA Price financial solution mode. You can continue using dispatch optimization and iteratively "
                        "solve for the PPA that results in a target IRR by running a SAM Parametric analysis or script.\n");
                }

                // Time-of-Delivery multipliers by time step:
                int ppa_mult_model = as_integer("ppa_multiplier_model");
                if (ppa_mult_model == 1)        // use dispatch_ts input
                {
                    if (is_assigned("dispatch_factors_ts") || is_dispatch) {
                        auto vec = as_vector_double("dispatch_factors_ts");
                        elec_pricing_schedule = C_timeseries_schedule_inputs(vec, ppa_price_year1);
                    }
                    else { // if no dispatch optimization, don't need an input pricing schedule
                        elec_pricing_schedule = C_timeseries_schedule_inputs(-1.0, std::numeric_limits<double>::quiet_NaN());
                    }
                }
                else if (ppa_mult_model == 0) // standard diurnal input
                {
                    // Most likely use case is to use schedules and TOD. So assume if at least one is provided, then user intended to use this approach
                    // the 'else' option applies non-feasible electricity prices, so we want to guard against selecting that it appears users
                    // are trying to use the schedules. 
                    bool is_one_assigned = is_assigned("dispatch_sched_weekday") || is_assigned("dispatch_sched_weekend") || is_assigned("dispatch_tod_factors");

                    if (is_one_assigned || is_dispatch) {

                        elec_pricing_schedule = C_timeseries_schedule_inputs(as_matrix("dispatch_sched_weekday"),
                            as_matrix("dispatch_sched_weekend"), as_vector_double("dispatch_tod_factors"), ppa_price_year1);
                    }
                    else {
                        // If electricity pricing data is not available, then dispatch to a uniform schedule
                        elec_pricing_schedule = C_timeseries_schedule_inputs(-1.0, std::numeric_limits<double>::quiet_NaN());
                    }
                }
            }
            else {
                throw exec_error("mspt_iph", "csp_financial_model must be 1, 7, or 8");
            }
        }
        else if (sim_type == 2) {
            elec_pricing_schedule = C_timeseries_schedule_inputs(-1.0, std::numeric_limits<double>::quiet_NaN());

        }

        // Set dispatch model type
        C_csp_tou::C_dispatch_model_type::E_dispatch_model_type dispatch_model_type = C_csp_tou::C_dispatch_model_type::E_dispatch_model_type::UNDEFINED;
        if (is_dispatch) {
            dispatch_model_type = C_csp_tou::C_dispatch_model_type::E_dispatch_model_type::DISPATCH_OPTIMIZATION;
        }
        else {
            dispatch_model_type = C_csp_tou::C_dispatch_model_type::E_dispatch_model_type::HEURISTIC;
        }

        bool is_offtaker_frac_also_max = as_boolean("is_tod_pc_target_also_pc_max");

        // TOU parameters
        C_csp_tou tou(offtaker_schedule, elec_pricing_schedule, dispatch_model_type, is_offtaker_frac_also_max);

        // *****************************************************
        //

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

        if (is_dispatch) {

            double heater_startup_cost = 0.0;

            dispatch.solver_params.set_user_inputs(as_integer("disp_steps_per_hour"), as_integer("disp_frequency"), as_integer("disp_horizon"),
                as_integer("disp_max_iter"), as_double("disp_mip_gap"), as_double("disp_timeout"),
                as_integer("disp_spec_presolve"), as_integer("disp_spec_bb"), as_integer("disp_spec_scaling"), as_integer("disp_reporting"));

            bool can_cycle_use_standby = false;
            double disp_csu_cost_calc = 0.0;
            double disp_pen_ramping = 0.0;

            double disp_rsu_cost_calc = as_double("disp_rsu_cost_rel") * q_dot_rec_des;   //[$/start]
            dispatch.params.set_user_params(can_cycle_use_standby, as_double("disp_time_weighting"),
                disp_rsu_cost_calc, heater_startup_cost, disp_csu_cost_calc, disp_pen_ramping,
                as_double("disp_inventory_incentive"), as_double("q_rec_standby"), as_double("q_rec_heattrace"));
        }

        // Instantiate Solver       
        C_csp_solver csp_solver(weather_reader,
            collector_receiver,
            *c_heat_sink_pointer,
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

        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::W_DOT_NET, allocate("W_dot_parasitic_tot", n_steps_fixed), n_steps_fixed);


        update("Initialize MSPT model...", 0.0);

        int out_type = -1;
        std::string out_msg = "";
        try
        {
            // Initialize Solver
            csp_solver.init();
        }
        catch (C_csp_exception& csp_exception)
        {
            // Report warning before exiting with error
            while (csp_solver.mc_csp_messages.get_message(&out_type, &out_msg))
            {
                log(out_msg, out_type);
            }

            throw exec_error("mspt_iph", csp_exception.m_error_message);
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
        assign("od_tube_calc", od_tube_calc * 1.E3); //[mm] convert from m

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
        double E_heater_su_des = 0.0;                       //[MWht]
        if (is_parallel_heater) {
            p_electric_resistance->get_design_parameters(E_heater_su_des, W_dot_heater_des_calc);
        }
        assign("W_dot_heater_des", (ssc_number_t)W_dot_heater_des_calc);    //[MWe]
        assign("E_heater_su_des", (ssc_number_t)E_heater_su_des);           //[MWht]

        // *************************
        // Thermal Energy Storage
        double V_tes_htf_avail_calc /*m3*/, V_tes_htf_total_calc /*m3*/,
            h_tank_calc /*m*/, d_tank_calc /*m*/, q_dot_loss_tes_des_calc /*MWt*/, dens_store_htf_at_T_ave_calc /*kg/m3*/,
            Q_tes_des_calc /*MWht*/;

        storage.get_design_parameters(V_tes_htf_avail_calc, V_tes_htf_total_calc,
            h_tank_calc, d_tank_calc, q_dot_loss_tes_des_calc, dens_store_htf_at_T_ave_calc, Q_tes_des_calc);

        assign("Q_tes_des", Q_tes_des_calc);                //[MWht]
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

        // Heat sink
        double m_dot_hs_ext_des, T_hs_ext_out_des, hx_min_dT_des, hx_UA_des;
        m_dot_hs_ext_des = T_hs_ext_out_des = hx_min_dT_des = hx_UA_des = std::numeric_limits<double>::quiet_NaN();
        if (hs_type == 1) {
            m_dot_hs_ext_des = c_heat_sink_phys.get_m_dot_ext_des(); //[kg/s]
            T_hs_ext_out_des = c_heat_sink_phys.get_T_ext_out_des(); //[C]
            hx_min_dT_des = c_heat_sink_phys.get_hx_min_dT_des();    //[C]
            hx_UA_des = c_heat_sink_phys.get_hx_UA_des();        //[kW/K]
        }

        assign("m_dot_hs_ext_des", m_dot_hs_ext_des);
        assign("T_hs_ext_out_des", T_hs_ext_out_des);
        assign("hx_min_dT_des", hx_min_dT_des);
        assign("hx_UA_des", hx_UA_des);

        // *************************
        // System
        double W_dot_bop_design, W_dot_fixed_parasitic_design;    //[MWe]
        csp_solver.get_design_parameters(W_dot_bop_design, W_dot_fixed_parasitic_design);

        // Calculate net system *generation* capacity including HTF pumps and system parasitics
        //double plant_net_capacity_calc = W_dot_cycle_des - W_dot_col_tracking_des - W_dot_rec_pump_des -
        //    W_dot_pc_pump_des - W_dot_pc_cooling_des - W_dot_bop_design - W_dot_fixed_parasitic_design;    //[MWe]

        //double plant_net_conv_calc = plant_net_capacity_calc / W_dot_cycle_des; //[-]

        //double system_capacity = plant_net_capacity_calc * 1.E3;         //[kWe], convert from MWe
        double system_capacity = q_dot_pc_des * 1.E3;       //[kWt]

        assign("W_dot_bop_design", W_dot_bop_design);           //[MWe]
        assign("W_dot_fixed", W_dot_fixed_parasitic_design);    //[MWe]
        // Calculate system capacity instead of pass in
        assign("system_capacity", system_capacity);     //[kWt]
        assign("nameplate", system_capacity * 1.E-3);   //[MWt]
        assign("cp_system_nameplate", system_capacity * 1.E-3); //[MWt]
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

        double tes_spec_cost = as_double("tes_spec_cost");

        // no Cold Temp TES, so set those cost model inputs to 0
        double Q_CT_tes = 0.0;
        double CT_tes_spec_cost = 0.0;

        //double W_dot_design = as_double("P_ref");
        double power_cycle_spec_cost = 0.0; // as_double("plant_spec_cost");

        // Set heater thermal power and cost above, because they're dependent on is_heater boolean

        double bop_spec_cost = as_double("bop_spec_cost");

        double fossil_backup_spec_cost = 0.0;   // as_double("fossil_spec_cost");

        double contingency_rate = as_double("contingency_rate");

        // land area
        double total_land_area = total_land_area_before_rad_cooling;
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

        // Still need to set radiative cooling inputs to 0 until/if we remove radiative cooling from cost model
        double rad_fluidcost = 0.0;
        double rad_installcost = 0.0;
        double rad_unitcost = 0.0;
        double rad_volmulti = 0.0;
        double coldstorage_unitcost = 0.0;
        double radfield_area = 0.0;
        double coldstorage_vol = 0.0;
        double radfield_vol = 0.0;

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

            Q_tes,
            tes_spec_cost,

            Q_CT_tes,
            CT_tes_spec_cost,

            q_dot_pc_des,
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
        if (csp_financial_model == 1)
        {
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
        }

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
        catch (C_csp_exception& csp_exception)
        {
            // Report warning before exiting with error
            while (csp_solver.mc_csp_messages.get_message(&out_type, &out_msg))
            {
                log(out_msg);
            }

            throw exec_error("mspt_iph", csp_exception.m_error_message);
        }

        // If no exception, then report messages
        while (csp_solver.mc_csp_messages.get_message(&out_type, &out_msg))
        {
            log(out_msg, out_type);
        }

        size_t count;
        ssc_number_t* p_q_dot_heat_sink = as_array("q_dot_to_heat_sink", &count);

        // 'adjustment_factors' class stores factors in hourly array, so need to index as such
        adjustment_factors haf(this->get_var_table(), "adjust");
        if (!haf.setup(count))
            throw exec_error("mspt_iph", "failed to setup adjustment factors: " + haf.error());

        ssc_number_t* p_gen_heat = allocate("gen_heat", count);
        ssc_number_t* p_gen = allocate("gen", n_steps_fixed);
        ssc_number_t* p_gen_heat_btu = allocate("gen_heat_btu", n_steps_fixed);
        ssc_number_t* p_W_dot_parasitic_tot = as_array("W_dot_parasitic_tot", &count);
        ssc_number_t* p_W_dot_par_tot_haf = allocate("W_dot_par_tot_haf", n_steps_fixed);
        ssc_number_t* p_load = allocate("load", n_steps_fixed); // testing using cmod_utilityrate5 for electricity rates p_load = p_W_dot_par_tot_haf


        ssc_number_t* p_time_final_hr = as_array("time_hr", &count);

        for (size_t i = 0; i < count; i++)
        {
            size_t hour = (size_t)ceil(p_time_final_hr[i]);
            p_gen_heat[i] = (ssc_number_t)(p_q_dot_heat_sink[i] * 1.E3 * haf(hour));           //[kWt]
            p_gen[i] = (ssc_number_t)0.0;   //[kWt] (no electrical generation for IPH tower)
            p_gen_heat_btu[i] = p_gen_heat[i] / MMBTU_TO_KWh;   //[MMBtu/hr]
            p_W_dot_parasitic_tot[i] *= -1.0;			//[MWe] Label is total parasitics, so change to a positive value
            p_W_dot_par_tot_haf[i] = (ssc_number_t)(p_W_dot_parasitic_tot[i] * haf(hour) * 1.E3);		//[kWe] apply availability derate and convert from MWe 
            p_load[i] = p_W_dot_par_tot_haf[i]; 
        }

        ssc_number_t* p_annual_energy_dist_time = gen_heatmap(this, steps_per_hour, true);

        accumulate_annual_for_year("gen_heat", "annual_energy", sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour);  //[kWht]
        accumulate_annual_for_year("gen_heat_btu", "annual_energy_heat_btu", sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour);  //[MMBtu]


            // This term currently includes TES freeze protection
        accumulate_annual_for_year("W_dot_par_tot_haf", "annual_electricity_consumption", sim_setup.m_report_step / 3600.0, steps_per_hour);	//[kWhe]

        double V_water_mirrors = as_double("water_usage_per_wash") / 1000.0 * A_sf * as_double("washing_frequency");
        assign("annual_total_water_use", (ssc_number_t) V_water_mirrors);

        ssc_number_t ae = as_number("annual_energy");			//[kWht]
        double nameplate = q_dot_pc_des * 1.E3;                 //[kWt]
        double kWh_per_kW = ae / nameplate;
        assign("capacity_factor", (ssc_number_t)(kWh_per_kW / 8760. * 100.));
        assign("kwh_per_kw", (ssc_number_t)kWh_per_kW);


        // Do unit post-processing here

        // Convert mass flow rates from [kg/hr] to [kg/s]
        size_t count_m_dot_rec = 0;
        ssc_number_t* p_m_dot_rec = as_array("m_dot_rec", &count_m_dot_rec);
        if (count_m_dot_rec != n_steps_fixed)
        {
            log("At least one m_dot array is a different length than 'n_steps_fixed'.", SSC_WARNING);
            return;
        }
        for (size_t i = 0; i < n_steps_fixed; i++)
        {
            p_m_dot_rec[i] = (ssc_number_t)(p_m_dot_rec[i] / 3600.0);   //[kg/s] convert from kg/hr
        }

        // Set output data from heliostat class
        size_t n_rows_eta_map = heliostatfield.ms_params.m_eta_map.nrows();
        ssc_number_t* eta_map_out = allocate("eta_map_out", n_rows_eta_map, 3);
        size_t n_rows_flux_maps = heliostatfield.ms_params.m_flux_maps.nrows();
        size_t n_cols_flux_maps = heliostatfield.ms_params.m_flux_maps.ncols() + 2;
        ssc_number_t* flux_maps_out = allocate("flux_maps_out", n_rows_eta_map, n_cols_flux_maps);
        ssc_number_t* flux_maps_for_import = allocate("flux_maps_for_import", n_rows_eta_map, n_cols_flux_maps);

        if (n_rows_eta_map != n_rows_flux_maps)
        {
            log("The number of rows in the field efficiency and receiver flux map matrices are not equal. This is unexpected, and the flux maps may be inaccurate.");
        }

        // [W/m2 * m2 / (m2_per_panel?)]
        double flux_scaling_mult = as_double("dni_des") * heliostatfield.ms_params.m_A_sf / 1000.0 /
            (A_rec / double(heliostatfield.ms_params.m_n_flux_x));

        for (size_t i = 0; i < n_rows_eta_map; i++)
        {
            flux_maps_out[n_cols_flux_maps * i] = eta_map_out[3 * i] = (ssc_number_t)heliostatfield.ms_params.m_eta_map(i, 0);        //[deg] Solar azimuth angle
            flux_maps_out[n_cols_flux_maps * i + 1] = eta_map_out[3 * i + 1] = (ssc_number_t)heliostatfield.ms_params.m_eta_map(i, 1);    //[deg] Solar zenith angle
            flux_maps_for_import[n_cols_flux_maps * i] = eta_map_out[3 * i] = (ssc_number_t)heliostatfield.ms_params.m_eta_map(i, 0);        //[deg] Solar azimuth angle
            flux_maps_for_import[n_cols_flux_maps * i + 1] = eta_map_out[3 * i + 1] = (ssc_number_t)heliostatfield.ms_params.m_eta_map(i, 1);    //[deg] Solar zenith angle
            eta_map_out[3 * i + 2] = (ssc_number_t)heliostatfield.ms_params.m_eta_map(i, 2);                            //[deg] Solar field optical efficiency
            for (size_t j = 2; j < n_cols_flux_maps; j++)
            {
                flux_maps_out[n_cols_flux_maps * i + j] = (ssc_number_t)(heliostatfield.ms_params.m_flux_maps(i, j - 2) * heliostatfield.ms_params.m_eta_map(i, 2) * flux_scaling_mult);      //[kW/m^2]
                flux_maps_for_import[n_cols_flux_maps * i + j] = (ssc_number_t)heliostatfield.ms_params.m_flux_maps(i, j - 2);
            }
        }

        accumulate_annual_for_year("Q_thermal", "annual_q_rec_htf", sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour); //[MWht]
        accumulate_annual_for_year("q_dot_rec_inc", "annual_q_rec_inc", sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour);           //[MWht]
        accumulate_annual_for_year("q_thermal_loss", "annual_q_rec_loss", sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour);
        accumulate_annual_for_year("q_piping_losses", "annual_q_piping_loss", sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour);
        accumulate_annual_for_year("q_startup", "annual_q_rec_startup", sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour);
        accumulate_annual_for_year("P_tower_pump", "annual_E_tower_pump", sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour);

        assign("annual_eta_rec_th", (ssc_number_t)(1.0 - as_number("annual_q_rec_loss") / as_number("annual_q_rec_inc")));
        assign("annual_eta_rec_th_incl_refl", (ssc_number_t)(as_number("rec_absorptance") * as_number("annual_eta_rec_th")));

        size_t count_df;
        ssc_number_t* p_defocus = as_array("defocus", &count_df);
        size_t count_q_rec_in;
        ssc_number_t* p_q_rec_in = as_array("q_dot_rec_inc", &count_q_rec_in);

        double q_defocus_sum = 0.0;
        double i_defocus;
        for (size_t i = 0; i < count_df; i++) {
            i_defocus = min(1.0, max(0.0, p_defocus[i]));
            q_defocus_sum += p_q_rec_in[i] * (1.0 - i_defocus);   //[MWt]
        }
        q_defocus_sum *= sim_setup.m_report_step / 3600.0;    //[MWht]
        assign("annual_q_defocus_est", q_defocus_sum);      //[MWht]

        std::clock_t clock_end = std::clock();
        double sim_cpu_run_time = (clock_end - clock_start) / (double)CLOCKS_PER_SEC;		//[s]
        assign("sim_cpu_run_time", sim_cpu_run_time);   //[s]

    }
};

DEFINE_MODULE_ENTRY(mspt_iph, "CSP molten salt power tower with hierarchical controller and dispatch optimization", 1)
