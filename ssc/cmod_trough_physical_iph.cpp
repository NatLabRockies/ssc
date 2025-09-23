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

// Trough CSP - physical model
#include "core.h"
//#include "tckernel.h"

// for adjustment factors
#include "common.h"

//#include "lib_weatherfile.h
//#include "csp_solver_util.h"
#include "csp_solver_core.h"
#include "csp_solver_trough_collector_receiver.h"
#include "csp_solver_pc_heat_sink.h"
#include "csp_solver_pc_heat_sink_physical.h"
#include "csp_solver_two_tank_tes.h"

#include "csp_solver_piston_cylinder_tes.h"
#include "csp_solver_packedbed_tes.h"
#include "csp_system_costs.h"

//#include "cst_iph_dispatch.h"
#include "csp_dispatch.h"

#include "csp_solver_cr_electric_resistance.h"

#include <ctime>
#include <algorithm>
#include <iterator>

// for utility rates
#include "cmod_utilityrate5.h"

// signed/unsigned mismatch
#pragma warning (disable : 4388)

static var_info _cm_vtab_trough_physical_iph[] = {

    /* VARTYPE          DATATYPE         NAME                         LABEL                                                                               UNITS           META              GROUP             REQUIRED_IF                CONSTRAINTS         UI_HINTS*/

    { SSC_INPUT,        SSC_NUMBER,      "is_dispatch",               "Allow dispatch optimization?",                                                     "",             "",               "System Control", "?=0",                    "",              ""},
    { SSC_INPUT,        SSC_NUMBER,      "sim_type",                  "1 (default): timeseries, 2: design only",                                          "",             "",               "System Control", "?=1",                    "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,        SSC_NUMBER,      "is_parallel_htr",           "Does plant include a HTF heater parallel to solar field?",                         "",             "",               "System Control", "?=0",                     "",                      ""},

    // Weather Reader
    { SSC_INPUT,        SSC_STRING,      "file_name",                 "Local weather file with path",                                                     "none",         "",               "weather",        "?",                       "LOCAL_FILE",            "" },
    { SSC_INPUT,        SSC_TABLE,       "solar_resource_data",       "Weather resource data in memory",                                                  "",             "",               "weather",        "?",                       "",                      "SIMULATION_PARAMETER" },

    // Solar Field, Trough

    { SSC_INPUT,        SSC_NUMBER,      "q_pb_design",               "Design heat input to power block",                                                 "MWt",          "",               "System_Design",  "*",                       "",                      "" },

    { SSC_INPUT,        SSC_NUMBER,      "nHCEt",                     "Number of HCE types",                                                              "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "nColt",                     "Number of collector types",                                                        "none",         "constant=4",     "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "nHCEVar",                   "Number of HCE variants per type",                                                  "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "FieldConfig",               "Number of subfield headers",                                                       "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "eta_pump",                  "HTF pump efficiency",                                                              "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "Fluid",                     "Field HTF fluid ID number",                                                        "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "accept_loc",                "In acceptance testing mode - temperature sensor location",                         "1/2",          "hx/loop",        "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "HDR_rough",                 "Header pipe roughness",                                                            "m",            "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "theta_stow",                "Stow angle",                                                                       "deg",          "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "theta_dep",                 "Deploy angle",                                                                     "deg",          "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "Row_Distance",              "Spacing between rows (centerline to centerline)",                                  "m",            "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "T_loop_in_des",             "Design loop inlet temperature",                                                    "C",            "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "T_loop_out",                "Target loop outlet temperature",                                                   "C",            "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "T_startup",                 "Required temperature of the system before the power block can be switched on",     "C",            "",               "solar_field",    "?",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "T_shutdown",                "Temperature when solar field begins recirculating",                                "C",            "",               "solar_field",    "?",                       "",                      "" },


    { SSC_INPUT,        SSC_NUMBER,      "use_abs_or_rel_mdot_limit", "Use mass flow abs (0) or relative (1) limits",                                     "",             "",               "solar_field",    "?=0",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "m_dot_htfmin",              "Minimum loop HTF flow rate",                                                       "kg/s",         "",               "solar_field",    "use_abs_or_rel_mdot_limit=0",  "",                 "" },
    { SSC_INPUT,        SSC_NUMBER,      "m_dot_htfmax",              "Maximum loop HTF flow rate",                                                       "kg/s",         "",               "solar_field",    "use_abs_or_rel_mdot_limit=0",  "",                 "" },
    { SSC_INPUT,        SSC_NUMBER,      "f_htfmin",                  "Minimum loop mass flow rate fraction of design",                                   "",             "",               "solar_field",    "use_abs_or_rel_mdot_limit=1",  "",                 "" },
    { SSC_INPUT,        SSC_NUMBER,      "f_htfmax",                  "Maximum loop mass flow rate fraction of design",                                   "",             "",               "solar_field",    "use_abs_or_rel_mdot_limit=1",  "",                 "" },

    { SSC_INPUT,        SSC_MATRIX,      "field_fl_props",            "User defined field fluid property data",                                           "-",            "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "T_fp",                      "Freeze protection temperature (heat trace activation temperature)",                "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "I_bn_des",                  "Solar irradiation at design",                                                      "C",            "",               "solar_field",    "*",                       "",                      "" },
    //{ SSC_INPUT,        SSC_NUMBER,      "V_hdr_max",                 "Maximum HTF velocity in the header at design",                                     "W/m2",         "",               "solar_field",    "*",                       "",                      "" },
    //{ SSC_INPUT,        SSC_NUMBER,      "V_hdr_min",                 "Minimum HTF velocity in the header at design",                                     "m/s",          "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "Pipe_hl_coef",              "Loss coefficient from the header, runner pipe, and non-HCE piping",                "m/s",          "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "SCA_drives_elec",           "Tracking power, in Watts per SCA drive",                                           "W/m2-K",       "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "tilt",                      "Tilt angle of surface/axis",                                                       "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "azimuth",                   "Azimuth angle of surface/axis",                                                    "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "wind_stow_speed",           "Trough wind stow speed",                                                           "m/s",          "",               "solar_field",    "?=50",                    "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "accept_mode",               "Acceptance testing mode?",                                                         "0/1",          "no/yes",         "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "accept_init",               "In acceptance testing mode - require steady-state startup",                        "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "mc_bal_hot",                "Heat capacity of the balance of plant on the hot side",                            "kWht/K-MWt",   "none",           "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "mc_bal_cold",               "Heat capacity of the balance of plant on the cold side",                           "kWht/K-MWt",   "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "mc_bal_sca",                "Non-HTF heat capacity associated with each SCA - per meter basis",                 "Wht/K-m",      "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_ARRAY,       "W_aperture",                "The collector aperture width (Total structural area used for shadowing)",          "m",            "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_ARRAY,       "A_aperture",                "Reflective aperture area of the collector",                                        "m2",           "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_ARRAY,       "TrackingError",             "User-defined tracking error derate",                                               "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_ARRAY,       "GeomEffects",               "User-defined geometry effects derate",                                             "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_ARRAY,       "Rho_mirror_clean",          "User-defined clean mirror reflectivity",                                           "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_ARRAY,       "Dirt_mirror",               "User-defined dirt on mirror derate",                                               "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_ARRAY,       "Error",                     "User-defined general optical error derate ",                                       "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_ARRAY,       "Ave_Focal_Length",          "Average focal length of the collector ",                                           "m",            "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_ARRAY,       "L_SCA",                     "Length of the SCA ",                                                               "m",            "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_ARRAY,       "L_aperture",                "Length of a single mirror/HCE unit",                                               "m",            "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_ARRAY,       "ColperSCA",                 "Number of individual collector sections in an SCA ",                               "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_ARRAY,       "Distance_SCA",              "Piping distance between SCA's in the field",                                       "m",            "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "IAM_matrix",                "IAM coefficients, matrix for 4 collectors",                                        "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "HCE_FieldFrac",             "Fraction of the field occupied by this HCE type ",                                 "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "D_2",                       "Inner absorber tube diameter",                                                     "m",            "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "D_3",                       "Outer absorber tube diameter",                                                     "m",            "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "D_4",                       "Inner glass envelope diameter ",                                                   "m",            "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "D_5",                       "Outer glass envelope diameter ",                                                   "m",            "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "D_p",                       "Diameter of the absorber flow plug (optional) ",                                   "m",            "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "Flow_type",                 "Flow type through the absorber",                                                   "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "Rough",                     "Relative roughness of the internal HCE surface ",                                  "-",            "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "alpha_env",                 "Envelope absorptance ",                                                            "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "epsilon_3_11",              "Absorber emittance for receiver type 1 variation 1",                               "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "epsilon_3_12",              "Absorber emittance for receiver type 1 variation 2",                               "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "epsilon_3_13",              "Absorber emittance for receiver type 1 variation 3",                               "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "epsilon_3_14",              "Absorber emittance for receiver type 1 variation 4",                               "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "epsilon_3_21",              "Absorber emittance for receiver type 2 variation 1",                               "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "epsilon_3_22",              "Absorber emittance for receiver type 2 variation 2",                               "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "epsilon_3_23",              "Absorber emittance for receiver type 2 variation 3",                               "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "epsilon_3_24",              "Absorber emittance for receiver type 2 variation 4",                               "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "epsilon_3_31",              "Absorber emittance for receiver type 3 variation 1",                               "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "epsilon_3_32",              "Absorber emittance for receiver type 3 variation 2",                               "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "epsilon_3_33",              "Absorber emittance for receiver type 3 variation 3",                               "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "epsilon_3_34",              "Absorber emittance for receiver type 3 variation 4",                               "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "epsilon_3_41",              "Absorber emittance for receiver type 4 variation 1",                               "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "epsilon_3_42",              "Absorber emittance for receiver type 4 variation 2",                               "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "epsilon_3_43",              "Absorber emittance for receiver type 4 variation 3",                               "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "epsilon_3_44",              "Absorber emittance for receiver type 4 variation 4",                               "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "alpha_abs",                 "Absorber absorptance ",                                                            "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "Tau_envelope",              "Envelope transmittance",                                                           "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "EPSILON_4",                 "Inner glass envelope emissivities (Pyrex) ",                                       "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "EPSILON_5",                 "Outer glass envelope emissivities (Pyrex) ",                                       "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "GlazingIntactIn",           "Glazing intact (broken glass) flag {1=true, else=false}",                          "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "P_a",                       "Annulus gas pressure",                                                             "torr",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "AnnulusGas",                "Annulus gas type (1=air, 26=Ar, 27=H2)",                                           "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "AbsorberMaterial",          "Absorber material type",                                                           "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "Shadowing",                 "Receiver bellows shadowing loss factor",                                           "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "Dirt_HCE",                  "Loss due to dirt on the receiver envelope",                                        "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "Design_loss",               "Receiver heat loss at design",                                                     "W/m",          "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "rec_su_delay",              "Fixed startup delay time for the receiver",                                        "hr",           "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "rec_qf_delay",              "Energy-based receiver startup delay (fraction of rated thermal power)",            "-",            "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "p_start",                   "Collector startup energy, per SCA",                                                "kWhe",       "",                 "solar_field",    "*",                       "",                      "" },

    // Heat Sink
    { SSC_INPUT,     SSC_NUMBER,         "pb_pump_coef",              "Pumping power to move 1kg of HTF through PB loop",                                 "kW/kg",        "",               "Heat Sink",      "*",                       "",                      "" },

    { SSC_INPUT,     SSC_NUMBER,         "hs_type",                   "0: ideal model, 1: physical steam model",                                          "",             "",               "Heat Sink",      "?=0",                     "",                      "" },
    { SSC_INPUT,     SSC_NUMBER,         "hs_htf_mdot_max_frac",      "Maximum HTF mass flow to heat sink relative to design point",                      "",             "",               "Heat Sink",      "*",                       "",                      "" },

        
    //{ SSC_INPUT,     SSC_NUMBER,         "hs_phys_N_sub",             "Number physical heat sink HX nodes",                                               "",             "",               "Heat Sink",      "hs_type=1",               "",                      "" },
    //{ SSC_INPUT,     SSC_NUMBER,         "hs_phys_tol",               "Physical heat sink solve tolerance",                                               "",             "",               "Heat Sink",      "hs_type=1",               "",                      "" },
    //{ SSC_INPUT,     SSC_NUMBER,         "hs_phys_f_mdot_steam_min",  "Min steam mdot fraction for physical heat sink",                                   "",             "",               "Heat Sink",      "hs_type=1",               "",                      "" },
    //{ SSC_INPUT,     SSC_NUMBER,         "hs_phys_f_mdot_steam_max",  "Max steam mdot fraction for physical heat sink",                                   "",             "",               "Heat Sink",      "hs_type=1",               "",                      "" },
    { SSC_INPUT,     SSC_NUMBER,         "hs_phys_T_steam_cold_des",  "Steam inlet temperature for physical heat sink",                                   "C",            "",               "Heat Sink",      "hs_type=1",               "",                      "" },
    { SSC_INPUT,     SSC_NUMBER,         "hs_phys_P_steam_hot_des",   "Steam outlet (and inlet) pressure for physical heat sink",                         "bar",          "",               "Heat Sink",      "hs_type=1",               "",                      "" },
    { SSC_INPUT,     SSC_NUMBER,         "hs_phys_Q_steam_hot_des",   "Steam outlet quality for physical heat sink",                                      "",             "",               "Heat Sink",      "hs_type=1",               "",                      "" },

    // Parallel heater parameters
    { SSC_INPUT,        SSC_NUMBER,      "heater_mult",                 "Heater multiple relative to design cycle thermal power",                         "-",            "",           "Parallel Heater",    "is_parallel_htr=1",              "",               "" },
    { SSC_INPUT,        SSC_NUMBER,      "heater_efficiency",           "Heater electric to thermal efficiency",                                          "%",            "",           "Parallel Heater",    "is_parallel_htr=1",              "",               "" },
    { SSC_INPUT,        SSC_NUMBER,      "f_q_dot_des_allowable_su",    "Fraction of design power allowed during startup",                                "-",            "",           "Parallel Heater",    "is_parallel_htr=1",              "",               "" },
    { SSC_INPUT,        SSC_NUMBER,      "hrs_startup_at_max_rate",     "Duration of startup at max startup power",                                       "hr",           "",           "Parallel Heater",    "is_parallel_htr=1",              "",               "" },
    { SSC_INPUT,        SSC_NUMBER,      "f_q_dot_heater_min",          "Minimum allowable heater output as fraction of design",                          "",             "",           "Parallel Heater",    "is_parallel_htr=1",              "",               "" },
    { SSC_INPUT,        SSC_NUMBER,      "heater_spec_cost",            "Heater specific cost",                                                           "$/kWht",       "",           "System Costs",       "is_parallel_htr=1",              "",               "" },
    { SSC_INPUT,        SSC_NUMBER,      "allow_heater_no_dispatch_opt","Allow heater with no dispatch optimization? SAM UI relies on cmod default",      "",             "",           "System Costs",       "?=0",                            "",               "SIMULATION_PARAMETER" },

    // General TES Parameters
    { SSC_INPUT,        SSC_NUMBER,      "tes_type",                  "Standard two tank (1), Packed Bed (2), Piston Cylinder (3)",                       "-",            "",               "TES",            "?=1",                     "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "tshours",                   "Equivalent full-load thermal storage hours",                                       "hr",           "",               "TES",            "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "is_h_tank_fixed",           "[1] Use fixed height (calculate diameter) [0] Use fixed diameter [2] Use fixed d and h (for packed bed)",  "-",  "", "TES",            "?=1",                     "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "h_tank_in",                 "Total height of tank input (height of HTF when tank is full",                      "m",            "",               "TES",            "is_h_tank_fixed=1",       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "d_tank_in",                 "Tank diameter input",                                                              "m",            "",               "TES",            "is_h_tank_fixed=0|is_h_tank_fixed=2",       "",    "" },
    { SSC_INPUT,        SSC_NUMBER,      "u_tank",                    "Loss coefficient from the tank",                                                   "W/m2-K",       "",               "TES",            "tes_type=1|tes_type=3",   "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "tank_pairs",                "Number of equivalent tank pairs",                                                  "-",            "",               "TES",            "*",                       "INTEGER",               "" },
    { SSC_INPUT,        SSC_NUMBER,      "hot_tank_Thtr",             "Minimum allowable hot tank HTF temp",                                              "C",            "",               "TES",            "tes_type=1|tes_type=3",   "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "hot_tank_max_heat",         "Rated heater capacity for hot tank heating",                                       "MWe",          "",               "TES",            "tes_type=1|tes_type=3",   "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "cold_tank_Thtr",            "Minimum allowable cold tank HTF temp",                                             "C",            "",               "TES",            "tes_type=1|tes_type=3",   "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "cold_tank_max_heat",        "Rated heater capacity for cold tank heating",                                      "MWe",          "",               "TES",            "tes_type=1|tes_type=3",   "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "h_tank_min",                "Minimum allowable HTF height in storage tank",                                     "m",            "",               "TES",            "tes_type=1|tes_type=3",   "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "init_hot_htf_percent",      "Initial fraction of avail. vol that is hot",                                       "%",            "",               "TES",            "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "tes_n_tsteps",              "Number of subtimesteps (for NT and packed bed)",                                   "",             "",               "TES",            "tes_type>1",              "",                      "" },

    // TES Two Tank Specific 
    { SSC_INPUT,        SSC_NUMBER,      "store_fluid",               "Material number for storage fluid",                                                "-",            "",               "TES",            "tes_type=1",              "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "store_fl_props",            "User defined storage fluid property data",                                         "-",            "",               "TES",            "tes_type=1",              "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "dt_hot",                    "Hot side HX approach temp",                                                        "C",            "",               "TES",            "tes_type=1",              "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "tanks_in_parallel",         "Tanks are in parallel, not in series, with solar field",                           "-",            "",               "controller",     "tes_type=1",              "",                      "" },

    // TES Piston Cylinder
    { SSC_INPUT,        SSC_NUMBER,      "tes_cyl_tank_thick",         "Tank wall thickness (used for Piston Cylinder)",                                  "m",            "",               "TES",            "tes_type=3",              "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "tes_cyl_tank_cp",            "Tank wall cp (used for Piston Cylinder)",                                         "kJ/kg-K",      "",               "TES",            "tes_type=3",              "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "tes_cyl_tank_dens",          "Tank wall thickness (used for Piston Cylinder)",                                  "kg/m3",        "",               "TES",            "tes_type=3",              "",                      "" },
    { SSC_INPUT,        SSC_ARRAY,       "tes_cyl_piston_loss_poly",   "Polynomial coefficients describing piston heat loss function (f(kg/s)=%)",        "",             "",               "TES",            "tes_type=3",              "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "tes_cyl_tank_insul_percent", "Percent additional wall mass due to insulation (used for Piston Cylinder)",       "%",            "",               "TES",            "?=0",                     "",                      "" },

    // TES Packed Bed
    { SSC_INPUT,        SSC_NUMBER,      "tes_pb_n_xsteps",           "Number of spatial segments",                                                       "",             "",               "TES",            "tes_type=2",              "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "tes_pb_k_eff",              "TES packed bed effective conductivity",                                            "W/m K",        "",               "TES",            "tes_type=2",              "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "tes_pb_void_frac",          "TES packed bed void fraction",                                                     "",             "",               "TES",            "tes_type=2",              "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "tes_pb_dens_solid",         "TES packed bed media density",                                                     "kg/m3",        "",               "TES",            "tes_type=2",              "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "tes_pb_cp_solid",           "TES particle specific heat",                                                       "kJ/kg K",      "",               "TES",            "tes_type=2",              "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "tes_pb_T_hot_delta",        "Max allowable decrease in hot discharge temp",                                     "C",            "",               "TES",            "tes_type=2",              "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "tes_pb_T_cold_delta",       "Max allowable increase in cold discharge temp",                                    "C",            "",               "TES",            "tes_type=2",              "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "tes_pb_T_charge_min",       "Min charge temp",                                                                  "C",            "",               "TES",            "tes_type=2",              "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "tes_pb_f_oversize",         "Packed bed oversize factor",                                                       "",             "",               "TES",            "tes_type=2",              "",                      "" },

    
        // Dispatch optimization
    { SSC_INPUT,        SSC_NUMBER,      "disp_horizon",              "Time horizon for dispatch optimization",                                           "hour",         "",               "Sys_Control",    "",                        "",                      "SIMULATION_PARAMETER" },
    { SSC_INPUT,        SSC_NUMBER,      "disp_frequency",            "Frequency for dispatch optimization calculations",                                 "hour",         "",               "Sys_Control",    "",                        "",                      "SIMULATION_PARAMETER" },
    { SSC_INPUT,        SSC_NUMBER,      "disp_max_iter",             "Max. no. dispatch optimization iterations",                                        "-",            "",               "Sys_Control",    "",                        "",                      "SIMULATION_PARAMETER" },
    { SSC_INPUT,        SSC_NUMBER,      "disp_timeout",              "Max. dispatch optimization solve duration",                                        "s",            "",               "Sys_Control",    "",                        "",                      "SIMULATION_PARAMETER" },
    { SSC_INPUT,        SSC_NUMBER,      "disp_mip_gap",              "Dispatch optimization solution tolerance",                                         "-",            "",               "Sys_Control",    "",                        "",                      "SIMULATION_PARAMETER" },
    { SSC_INPUT,        SSC_NUMBER,      "disp_time_weighting",       "Dispatch optimization future time discounting factor",                             "-",            "",               "Sys_Control",    "?=0.999",                  "",                      "" },

    { SSC_INPUT,        SSC_NUMBER,      "disp_steps_per_hour",       "Time steps per hour for dispatch optimization calculations",                            "-",                   "",                             "tou",                                      "?=1",                     "",                      "SIMULATION_PARAMETER" },
    { SSC_INPUT,        SSC_NUMBER,      "disp_spec_presolve",        "Dispatch optimization presolve heuristic",                                              "-",                   "",                             "tou",                                      "?=-1",                    "",                      "SIMULATION_PARAMETER" },
    { SSC_INPUT,        SSC_NUMBER,      "disp_spec_bb",              "Dispatch optimization B&B heuristic",                                                   "-",                   "",                             "tou",                                      "?=-1",                    "",                      "SIMULATION_PARAMETER" },
    { SSC_INPUT,        SSC_NUMBER,      "disp_reporting",            "Dispatch optimization reporting level",                                                 "-",                   "",                             "tou",                                      "?=-1",                    "",                      "SIMULATION_PARAMETER" },
    { SSC_INPUT,        SSC_NUMBER,      "disp_spec_scaling",         "Dispatch optimization scaling heuristic",                                               "-",                   "",                             "tou",                                      "?=-1",                    "",                      "SIMULATION_PARAMETER" },

    { SSC_INPUT,        SSC_NUMBER,      "disp_inventory_incentive",  "Dispatch storage terminal inventory incentive multiplier",                         "",             "",               "System Control", "?=0.0",                   "",                      "SIMULATION_PARAMETER" },

    { SSC_INPUT,        SSC_NUMBER,      "csp_financial_model",       "",                                                                                 "1-8",          "",               "Financial Model",        "?=1",                                                      "INTEGER,MIN=0",  "" },

    // Prices for *electricity* purchases
    { SSC_INPUT,        SSC_NUMBER,      "ppa_multiplier_model",      "PPA multiplier model 0: dispatch factors dispatch_factorX, 1: hourly multipliers dispatch_factors_ts", "0/1", "0=diurnal,1=timestep", "tou",    "?=0",  /*need a default so this var works in required_if*/  "INTEGER,MIN=0", "" },
    { SSC_INPUT,        SSC_NUMBER,      "ppa_soln_mode",             "PPA solution mode (0=Specify IRR target, 1=Specify PPA price)",                    "",             "",               "Financial Solution Mode", "ppa_multiplier_model=0&csp_financial_model<5&is_dispatch=1","",              "" },
    { SSC_INPUT,        SSC_ARRAY,       "ppa_price_input_heat_btu",  "PPA prices - yearly",			                                                  "$/MMBtu",	  "",	            "Revenue",			       "ppa_multiplier_model=0&csp_financial_model<5&is_dispatch=1","",      	     "" },
        // *Electricity* hourly price multipliers from Block Schedule                                                                                                                                                                                                                                
    { SSC_INPUT,        SSC_MATRIX,      "dispatch_sched_weekday",    "12x24 PPA pricing Weekday schedule",                                               "",             "",               "tou",                     "ppa_multiplier_model=0&csp_financial_model<5&is_dispatch=1","",              "" },
    { SSC_INPUT,        SSC_MATRIX,      "dispatch_sched_weekend",    "12x24 PPA pricing Weekend schedule",                                               "",             "",               "tou",                     "ppa_multiplier_model=0&csp_financial_model<5&is_dispatch=1","",              "" },
    { SSC_INPUT,        SSC_ARRAY,       "dispatch_tod_factors",      "TOD factors for periods 1 through 9",                                              "",                                                                                                                                        
        "We added this array input after SAM 2022.12.21 to replace the functionality of former single value inputs dispatch_factor1 through dispatch_factor9",                              "Time of Delivery Factors","ppa_multiplier_model=0&csp_financial_model<5&is_dispatch=1","",              "" },
        // *Electricity* hourly price multipliers from time series input
    { SSC_INPUT,        SSC_ARRAY,       "dispatch_factors_ts",       "Time series electricity price multipliers",                                        "",             "",               "tou",                     "ppa_multiplier_model=1&csp_financial_model<5&is_dispatch=1","",              "" },
                                                                                                                                                                                                                       
    // Control for *heat* output                                                                                                                                                                                       
    { SSC_INPUT,        SSC_NUMBER,      "is_timestep_load_fractions","0: block dispatch, 1: hourly load fraction, 2: absolute load",                     "",             "",               "tou",                     "?=0",                                                       "",              "" },
    { SSC_INPUT,        SSC_ARRAY,       "timestep_load_fractions",   "Heat sink load fraction for each timestep, alternative to block dispatch",         "",             "",               "tou",                     "is_timestep_load_fractions=1",                              "",              "" },
    { SSC_INPUT,        SSC_MATRIX,      "weekday_schedule",          "12x24 CSP operation Time-of-Use Weekday schedule",                                 "",             "",               "tou",                     "is_timestep_load_fractions=0",                              "",              "" },
    { SSC_INPUT,        SSC_MATRIX,      "weekend_schedule",          "12x24 CSP operation Time-of-Use Weekend schedule",                                 "",             "",               "tou",                     "is_timestep_load_fractions=0",                              "",              "" },
    { SSC_INPUT,        SSC_ARRAY,       "f_turb_tou_periods",        "Time series heat sink load fractions",                                             "",             "",               "tou",                     "is_timestep_load_fractions=0",                              "",              "" },
    { SSC_INPUT,        SSC_ARRAY,       "timestep_load_abs",         "Heat sink hourly load (not normalized)",                                           "kWt",          "",               "tou",                     "is_timestep_load_fractions=2",                              "",              "" },
    { SSC_INPUT,        SSC_NUMBER,      "timestep_load_abs_factor",  "Heat sink hourly load scale factor",                                               "",             "",               "tou",                     "?=1",                                                       "",              "" },

    //                                                                                                                                                                                                                                                                          
    { SSC_INPUT,        SSC_NUMBER,      "is_tod_pc_target_also_pc_max", "Is the TOD target cycle heat input also the max cycle heat input?",             "",             "",               "tou",                     "?=0",                                                       "",              "" },

    // System
    { SSC_INPUT,        SSC_NUMBER,      "pb_fixed_par",              "Fraction of rated gross power constantly consumed",                                "MWe/MWcap",    "",               "system",         "*",                       "",                      "" },
    { SSC_INPUT,        SSC_ARRAY,       "bop_array",                 "Balance of plant parasitic power fraction, mult frac and const, linear and quad coeff", "",        "",               "system",         "*",                       "",                      "" },
    { SSC_INPUT,        SSC_ARRAY,       "aux_array",                 "Auxiliary heater, mult frac and const, linear and quad coeff",                     "",             "",               "system",         "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "water_usage_per_wash",      "Water usage per wash",                                                             "L/m2_aper",    "",               "system",         "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "washing_frequency",         "Mirror washing frequency",                                                         "-/year",       "",               "system",         "*",                       "",                      "" },

    // Newly added
    { SSC_INPUT,        SSC_NUMBER,      "calc_design_pipe_vals",     "Calculate temps and pressures at design conditions for runners and headers",       "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "V_hdr_cold_max",            "Maximum HTF velocity in the cold headers at design",                               "m/s",          "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "V_hdr_cold_min",            "Minimum HTF velocity in the cold headers at design",                               "m/s",          "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "V_hdr_hot_max",             "Maximum HTF velocity in the hot headers at design",                                "m/s",          "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "V_hdr_hot_min",             "Minimum HTF velocity in the hot headers at design",                                "m/s",          "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "N_max_hdr_diams",           "Maximum number of diameters in each of the hot and cold headers",                  "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "L_rnr_pb",                  "Length of runner pipe in power block",                                             "m",            "",               "powerblock",     "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "L_rnr_per_xpan",            "Threshold length of straight runner pipe without an expansion loop",               "m",            "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "L_xpan_hdr",                "Compined perpendicular lengths of each header expansion loop",                     "m",            "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "L_xpan_rnr",                "Compined perpendicular lengths of each runner expansion loop",                     "m",            "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "Min_rnr_xpans",             "Minimum number of expansion loops per single-diameter runner section",             "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "northsouth_field_sep",      "North/south separation between subfields. 0 = SCAs are touching",                  "m",            "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "N_hdr_per_xpan",            "Number of collector loops per expansion loop",                                     "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "offset_xpan_hdr",           "Location of first header expansion loop. 1 = after first collector loop",          "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "custom_sf_pipe_sizes",      "Use custom solar field pipe diams, wallthks, and lengths",                         "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "sf_rnr_diams",              "Custom runner diameters",                                                          "m",            "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "sf_rnr_wallthicks",         "Custom runner wall thicknesses",                                                   "m",            "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "sf_rnr_lengths",            "Custom runner lengths",                                                            "m",            "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "sf_hdr_diams",              "Custom header diameters",                                                          "m",            "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "sf_hdr_wallthicks",         "Custom header wall thicknesses",                                                   "m",            "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "sf_hdr_lengths",            "Custom header lengths",                                                            "m",            "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "tanks_in_parallel",         "Tanks are in parallel, not in series, with solar field",                           "-",            "",               "controller",     "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "has_hot_tank_bypass",       "Bypass valve connects field outlet to cold tank",                                  "-",            "",               "controller",     "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "T_tank_hot_inlet_min",      "Minimum hot tank htf inlet temperature",                                           "C",            "",               "controller",     "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "tes_pump_coef",             "Pumping power to move 1kg of HTF through tes loop",                                "kW/(kg/s)",    "",               "controller",     "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "V_tes_des",                 "Design-point velocity to size the TES pipe diameters",                             "m/s",          "",               "controller",     "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "custom_tes_p_loss",         "TES pipe losses are based on custom lengths and coeffs",                           "-",            "",               "controller",     "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "k_tes_loss_coeffs",         "Minor loss coeffs for the coll, gen, and bypass loops",                            "-",            "",               "controller",     "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "custom_tes_pipe_sizes",     "Use custom TES pipe diams, wallthks, and lengths",                                 "-",            "",               "controller",     "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "tes_diams",                 "Custom TES diameters",                                                             "m",            "",               "controller",     "custom_tes_pipe_sizes=1",                      "",                      "SIMULATION_PARAMETER" },
    { SSC_INPUT,        SSC_MATRIX,      "tes_wallthicks",            "Custom TES wall thicknesses",                                                      "m",            "",               "controller",     "custom_tes_pipe_sizes=1", "",                      "SIMULATION_PARAMETER" },
    { SSC_INPUT,        SSC_MATRIX,      "tes_lengths",               "Custom TES lengths",                                                               "m",            "",               "controller",     "custom_tes_pipe_sizes=1", "",                      "SIMULATION_PARAMETER" },

    // Needed for auto-updating dependent inputs
    { SSC_INPUT,        SSC_NUMBER,      "use_solar_mult_or_aperture_area",     "Use solar multiple or total field aperture area",                        "-",            "",               "controller",     "?=0",                     "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "specified_solar_multiple",            "specified_solar_multiple",                                               "-",            "",               "controller",     "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "specified_total_aperture",            "specified_total_aperture",                                               "-",            "",               "controller",     "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "non_solar_field_land_area_multiplier", "non_solar_field_land_area_multiplier",                                  "-",            "",               "controller",     "*",                       "",                      "" },
    { SSC_INPUT,        SSC_ARRAY,       "trough_loop_control",                 "trough_loop_control",                                                    "-",            "",               "controller",     "*",                       "",                      "" },

    // Direct Capital Costs
    { SSC_INPUT,    SSC_NUMBER,         "csp.dtr.cost.site_improvements.cost_per_m2", "Site Improvement Cost per m2",                                     "$/m2",         "",               "Capital_Costs",      "?=0",        "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "csp.dtr.cost.solar_field.cost_per_m2",       "Solar Field Cost per m2",                                          "$/m2",         "",               "Capital_Costs",      "?=0",        "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "csp.dtr.cost.htf_system.cost_per_m2",        "HTF System Cost Per m2",                                           "$/m2",         "",               "Capital_Costs",      "?=0",        "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "csp.dtr.cost.storage.cost_per_kwht",         "Storage cost per kWht",                                            "$/kWht",       "",               "Capital_Costs",      "?=0",        "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "csp.dtr.cost.heat_sink.cost_per_kwe",        "Heat Sink Cost per kWe",                                         "$/kWe",        "",               "Capital_Costs",      "?=0",        "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "csp.dtr.cost.bop_per_kwe",                   "Balance of Plant Cost per kWe",                                    "$/kWe",        "",               "Capital_Costs",      "?=0",        "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "csp.dtr.cost.contingency_percent",           "Contingency Percent",                                              "%",            "",               "Capital_Costs",      "?=0",        "",              "" },
                                                                                                                                                                                                                                
        // Indirect Capital Costs                                                                                                                                                                                               
    { SSC_INPUT,    SSC_NUMBER,         "csp.dtr.cost.epc.per_acre",                  "EPC Costs per acre",                                               "$/acre",       "",               "Capital_Costs",      "?=0",        "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "csp.dtr.cost.epc.percent",                   "EPC Costs % direct",                                               "%",            "",               "Capital_Costs",      "?=0",        "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "csp.dtr.cost.epc.per_watt",                  "EPC Cost Wac",                                                     "$/Wac",        "",               "Capital_Costs",      "?=0",        "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "csp.dtr.cost.epc.fixed",                     "Fixed EPC Cost",                                                   "$",            "",               "Capital_Costs",      "?=0",        "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "csp.dtr.cost.plm.per_acre",                  "Land Cost per acre",                                               "$/acre",       "",               "Capital_Costs",      "?=0",        "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "csp.dtr.cost.plm.percent",                   "Land Cost % direct",                                               "%",            "",               "Capital_Costs",      "?=0",        "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "csp.dtr.cost.plm.per_watt",                  "Land Cost Wac",                                                    "$/Wac",        "",               "Capital_Costs",      "?=0",        "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "csp.dtr.cost.plm.fixed",                     "Fixed Land Cost",                                                  "$",            "",               "Capital_Costs",      "?=0",        "",              "" },
    
        // Sales Tax
    { SSC_INPUT,    SSC_NUMBER,         "csp.dtr.cost.sales_tax.percent",            "Sales Tax Percentage of Direct Cost",                               "%",            "",               "Capital_Costs",      "?=0",        "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "sales_tax_rate",                            "Sales Tax Rate",                                                    "%",            "",               "Capital_Costs",      "?=0",        "",              "" },


    // Construction financing inputs/outputs (SSC variable table from cmod_cb_construction_financing)
    { SSC_INPUT,    SSC_NUMBER,         "const_per_interest_rate1",                  "Interest rate, loan 1",                                             "%",            "",               "Financial Parameters",  "csp_financial_model=1",       "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "const_per_interest_rate2",                  "Interest rate, loan 2",                                             "%",            "",               "Financial Parameters",  "csp_financial_model=1",       "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "const_per_interest_rate3",                  "Interest rate, loan 3",                                             "%",            "",               "Financial Parameters",  "csp_financial_model=1",       "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "const_per_interest_rate4",                  "Interest rate, loan 4",                                             "%",            "",               "Financial Parameters",  "csp_financial_model=1",       "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "const_per_interest_rate5",                  "Interest rate, loan 5",                                             "%",            "",               "Financial Parameters",  "csp_financial_model=1",       "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "const_per_months1",                         "Months prior to operation, loan 1",                                 "",             "",               "Financial Parameters",  "csp_financial_model=1",       "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "const_per_months2",                         "Months prior to operation, loan 2",                                 "",             "",               "Financial Parameters",  "csp_financial_model=1",       "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "const_per_months3",                         "Months prior to operation, loan 3",                                 "",             "",               "Financial Parameters",  "csp_financial_model=1",       "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "const_per_months4",                         "Months prior to operation, loan 4",                                 "",             "",               "Financial Parameters",  "csp_financial_model=1",       "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "const_per_months5",                         "Months prior to operation, loan 5",                                 "",             "",               "Financial Parameters",  "csp_financial_model=1",       "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "const_per_percent1",                        "Percent of total installed cost, loan 1",                           "%",            "",               "Financial Parameters",  "csp_financial_model=1",       "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "const_per_percent2",                        "Percent of total installed cost, loan 2",                           "%",            "",               "Financial Parameters",  "csp_financial_model=1",       "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "const_per_percent3",                        "Percent of total installed cost, loan 3",                           "%",            "",               "Financial Parameters",  "csp_financial_model=1",       "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "const_per_percent4",                        "Percent of total installed cost, loan 4",                           "%",            "",               "Financial Parameters",  "csp_financial_model=1",       "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "const_per_percent5",                        "Percent of total installed cost, loan 5",                           "%",            "",               "Financial Parameters",  "csp_financial_model=1",       "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "const_per_upfront_rate1",                   "Upfront fee on principal, loan 1",                                  "%",            "",               "Financial Parameters",  "csp_financial_model=1",       "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "const_per_upfront_rate2",                   "Upfront fee on principal, loan 2",                                  "%",            "",               "Financial Parameters",  "csp_financial_model=1",       "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "const_per_upfront_rate3",                   "Upfront fee on principal, loan 3",                                  "%",            "",               "Financial Parameters",  "csp_financial_model=1",       "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "const_per_upfront_rate4",                   "Upfront fee on principal, loan 4",                                  "%",            "",               "Financial Parameters",  "csp_financial_model=1",       "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "const_per_upfront_rate5",                   "Upfront fee on principal, loan 5",                                  "%",            "",               "Financial Parameters",  "csp_financial_model=1",       "",              "" },

    // *************************************************************************************************
    //    OUTPUTS
    // *************************************************************************************************

    // Design Point Outputs
    { SSC_OUTPUT,       SSC_NUMBER,      "solar_mult",                       "Actual solar multiple",                                                    "",              "",               "System Design Calc","*",                             "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "nameplate",                        "Nameplate capacity",                                                       "MWt",           "",               "System Design Calc","*",                             "",                      "" },

    // System capacity required by downstream financial model
    { SSC_OUTPUT,       SSC_NUMBER,      "system_capacity",                  "System capacity",                                                          "kWt",          "",                "System Design",  "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "cp_system_nameplate",              "System capacity for capacity payments",                                    "MWt",          "",                "System Design",  "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "cp_battery_nameplate",             "Battery nameplate",                                                        "MWt",          "",                "System Design",  "*",                                "",                      "" },

    // Solar Field
    { SSC_OUTPUT,       SSC_NUMBER,      "nSCA",                             "Number of SCAs in a loop",                                                 "none",          "",               "solar_field",     "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "field_htf_min_temp",               "Minimum field htf temp",                                                   "C",             "",               "Power Cycle",    "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "field_htf_max_temp",               "Maximum field htf temp",                                                   "C",             "",               "Power Cycle",    "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "field_htf_cp_avg_des",             "Field average htf cp at design",                                           "kJ/kgK",        "",               "Solar Field",    "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "single_loop_aperture",             "Single loop aperture",                                                     "m2",            "",               "Solar Field",    "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "min_inner_diameter",               "Minimum absorber inner diameter in loop",                                  "m",             "",               "Solar Field",    "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "csp_dtr_hce_design_heat_losses",   "Heat loss at design",                                                      "W/m",           "",               "Solar Field",    "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "csp_dtr_loop_hce_heat_loss",       "Loop Heat Loss from HCE at Design",                                        "W/m",           "",               "Solar Field",    "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "csp_dtr_sca_calc_sca_effs",        "SCA optical efficiencies at design",                                       "",              "",               "Solar Field",    "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "loop_optical_efficiency",          "Loop total optical effiency at design",                                    "",              "",               "Solar Field",    "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "csp_dtr_hce_optical_effs",         "HCE optical efficiencies at design",                                       "",              "",               "Solar Field",    "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_MATRIX,      "SCAInfoArray",                     "Receiver (,1) and collector (,2) type for each assembly in loop",          "",              "",               "Solar Field",    "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "SCADefocusArray",                  "Order in which the SCA's should be defocused",                             "",              "",               "Solar Field",    "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "max_field_flow_velocity",          "Maximum field flow velocity",                                              "m/s",           "",               "Solar Field",    "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "min_field_flow_velocity",          "Minimum field flow velocity",                                              "m/s",           "",               "Solar Field",    "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "m_dot_htfmin_actual",              "Actual minimum loop HTF flow rate",                                         "kg/s",         "",                "solar_field",    "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "m_dot_htfmax_actual",              "Actual maximum loop HTF flow rate",                                         "kg/s",         "",                "solar_field",    "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "f_htfmin_actual",                  "Actual minimum loop mass flow rate fraction of design",                     "",             "",                "solar_field",    "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "f_htfmax_actual",                  "Actual maximum loop mass flow rate fraction of design",                     "",             "",                "solar_field",    "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "max_loop_flow_vel_des",            "Maximum loop flow velocity at design",                                     "m/s",           "",               "Solar Field",    "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "min_loop_flow_vel_des",            "Minimum loop flow velocity at design",                                     "m/s",           "",               "Solar Field",    "*",                                "",                      "" },


    { SSC_OUTPUT,       SSC_NUMBER,      "total_loop_conversion_efficiency", "Total Loop Conversion Efficiency at Design",                               "",              "",               "Solar Field",    "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "total_required_aperture_for_SM1",  "Aperture required for solar mult = 1",                                     "m2",            "",               "Solar Field",    "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "required_number_of_loops_for_SM1", "Heat loss at design",                                                      "",              "",               "Solar Field",    "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "nLoops",                           "Number of loops in the field",                                             "",              "",               "Solar Field",    "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "total_aperture",                   "Total field aperture area",                                                "m2",            "",               "Solar Field",    "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "fixed_land_area",                  "Fixed Land Area",                                                          "acre",          "",               "Solar Field",    "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "total_land_area",                  "Total Land Area",                                                          "acre",          "",               "Solar Field",    "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "total_tracking_power",             "Total Tracking Power",                                                     "MWe",           "",               "Solar Field",    "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_MATRIX,      "K_cpnt",                           "Minor loss coefficients of the components in each loop interconnect",      "",              "",               "Solar Field",    "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_MATRIX,      "D_cpnt",                           "Inner diameters of the components in each loop interconnect",              "m",             "",               "Solar Field",    "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_MATRIX,      "L_cpnt",                           "Lengths of the components in each loop interconnect",                      "m",             "",               "Solar Field",    "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_MATRIX,      "Type_cpnt",                        "Type of component in each loop interconnect [0=fitting | 1=pipe | 2=flex_hose]",  "Wm",     "",               "Solar Field",    "*",                                "",                      "" },

    { SSC_OUTPUT,       SSC_NUMBER,      "field_thermal_output_actual",      "Design-point thermal power from the solar field limited by mass flow",     "MW",            "",               "Solar Field",    "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "field_thermal_output_ideal",       "Design-point thermal power from the solar field with no limit",            "MW",            "",               "Solar Field",    "*",                                "",                      "" },

        // Solar Field STEADY STATE
    { SSC_OUTPUT,       SSC_NUMBER,      "dP_sf_SS",                         "Steady State field pressure drop",                                         "bar",           "",               "Solar Field",    "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "W_dot_pump_SS",                    "Steady State pumping power",                                               "MWe",           "",               "Solar Field",    "*",                                "",                      "" },

    // Heater
    { SSC_OUTPUT,       SSC_NUMBER,      "q_dot_heater_des",                 "Heater design thermal power",                                              "MWt",           "",               "Heater",         "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "W_dot_heater_des",                 "Heater electricity consumption at design",                                 "MWe",           "",               "Heater",         "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "E_heater_su_des",                  "Heater startup energy",                                                    "MWht",          "",               "Heater",         "*",                                "",                      "" },

    // Thermal Storage
    { SSC_OUTPUT,       SSC_NUMBER,      "vol_tank",                         "Total tank volume",                                                        "m3",            "",               "Thermal Storage","*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "q_tes",                            "TES design capacity",                                                      "MWht",          "",               "Thermal Storage","*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "csp_pt_tes_tank_diameter",         "Tank diameter",                                                            "m",             "",               "Thermal Storage","*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "csp_pt_tes_tank_height",           "Tank height",                                                              "m",             "",               "Thermal Storage","*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "q_dot_tes_est",                    "Estimated TES Heat Loss",                                                  "MW",            "",               "Thermal Storage","*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "csp_pt_tes_htf_density",           "Storage htf density",                                                      "kg/m3",         "",               "Thermal Storage","*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "tes_avail_vol",                    "Available HTF volume",                                                     "m3",            "",               "Thermal Storage","*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "is_hx",                            "System has heat exchanger no/yes (0/1)",                                   "",              "",               "Thermal Storage","*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "vol_min",                          "Minimum Fluid Volume",                                                     "m3",            "",               "Thermal Storage","*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "V_tank_hot_ini",                   "Initial hot tank volume",                                                  "m3",            "",               "Thermal Storage","*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "tes_htf_avg_temp",                 "HTF Average Temperature at Design",                                        "C",             "",               "Thermal Storage","*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "tes_htf_min_temp",                 "Minimum storage htf temp",                                                 "C",             "",               "Power Cycle",    "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "tes_htf_max_temp",                 "Maximum storage htf temp",                                                 "C",             "",               "Power Cycle",    "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "tshours_field",                    "TES duration at field design output",                                      "hr",            "",               "TES Design Calc","*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "tshours_heater",                   "TES duration at heater design output",                                     "hr",            "",               "TES Design Calc","*",                                "",                      "" },

    // Collector
    { SSC_OUTPUT,       SSC_MATRIX,      "csp_dtr_sca_ap_lengths",           "Length of single module",                                                  "m",             "",               "Collector",      "?=0",                              "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "csp_dtr_sca_calc_zenith",          "Calculated zenith",                                                        "degree",        "",               "Collector",      "?=0",                              "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "csp_dtr_sca_calc_costh",           "Calculated costheta",                                                      "",              "",               "Collector",      "?=0",                              "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "csp_dtr_sca_calc_theta",           "Calculated theta",                                                         "degree",        "",               "Collector",      "?=0",                              "",                      "" },
    { SSC_OUTPUT,       SSC_MATRIX,      "csp_dtr_sca_calc_end_gains",       "End gain factor",                                                          "",              "",               "Collector",      "?=0",                              "",                      "" },
    { SSC_OUTPUT,       SSC_MATRIX,      "csp_dtr_sca_calc_end_losses",      "Use time-series net electricity generation limits",                        "",              "",               "Collector",      "?=0",                              "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "csp_dtr_sca_calc_latitude",        "Latitude",                                                                 "degree",        "",               "Collector",      "?=0",                              "",                      "" },
    { SSC_OUTPUT,       SSC_MATRIX,      "csp_dtr_sca_calc_iams",            "IAM at summer solstice",                                                   "",              "",               "Collector",      "?=0",                              "",                      "" },

    { SSC_OUTPUT,       SSC_NUMBER,      "m_dot_hs_ext_des",                 "Heat sink fluid mass flow rate",                                           "kg/s",          "",               "System Control",  "*",                               "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "T_hs_ext_out_des",                 "Heat sink fluid outlet temperature",                                       "C",             "",               "System Control",  "*",                               "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "hx_min_dT_des",                    "Heat sink hx min temp difference",                                         "C",             "",               "System Control",  "*",                               "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "hx_UA_des",                        "Heat sink hx conductance",                                                 "MW/K",          "",               "System Control",  "*",                               "",                      "" },


    // System Control
    { SSC_OUTPUT,       SSC_NUMBER,      "bop_design",                       "BOP parasitics at design",                                                 "MWe",           "",               "System Control", "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "aux_design",                       "Aux parasitics at design",                                                 "MWe",           "",               "System Control", "*",                                "",                      "" },

    { SSC_OUTPUT,       SSC_ARRAY,       "timestep_load_fractions_calc",     "Calculated timestep load fractions",                                       "",              "",               "System Control", "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "timestep_load_abs_calc",           "Calculated timestep load data",                                            "kWt",           "",               "System Control", "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "thermal_load_heat_btu",            "Thermal load (year 1)",                                                    "MMBtu/hr",      "",               "Thermal Rate",   "csp_financial_model=5",            "",                      "" },


        // Capital Costs

           // Direct Capital Costs
    { SSC_OUTPUT,       SSC_NUMBER,      "csp.dtr.cost.site_improvements",   "Site improvements cost",                                                   "$",             "",               "Capital Costs",  "",                                 "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "csp.dtr.cost.solar_field",         "Solar field cost",                                                         "$",             "",               "Capital Costs",  "",                                 "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "csp.dtr.cost.htf_system",          "HTF system cost",                                                          "$",             "",               "Capital Costs",  "",                                 "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "csp.dtr.cost.storage",             "Thermal storage cost",                                                     "$",             "",               "Capital Costs",  "",                                 "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "csp.dtr.cost.heat_sink",           "Heat sink cost",                                                         "$",             "",               "Capital Costs",  "",                                 "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "csp.dtr.cost.bop",                 "Balance of plant cost",                                                    "$",             "",               "Capital Costs",  "",                                 "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "csp.dtr.cost.contingency",         "Contingency cost",                                                         "$",             "",               "Capital Costs",  "",                                 "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "total_direct_cost",                "Total direct cost",                                                        "$",             "",               "Capital Costs",  "",                                 "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "direct_subtotal",                  "Direct subtotal",                                                          "$",             "",               "Capital Costs",  "",                                 "",                      "" },


        // Indirect Capital Costs        
    { SSC_OUTPUT,       SSC_NUMBER,      "csp.dtr.cost.epc.total",           "EPC total cost",                                                           "$",             "",               "Capital Costs",  "",                                 "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "csp.dtr.cost.plm.total",           "Total land cost",                                                          "$",             "",               "Capital Costs",  "",                                 "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "total_indirect_cost",              "Total direct cost",                                                        "$",             "",               "Capital Costs",  "",                                 "",                      "" },


        // Sales Tax
    { SSC_OUTPUT,       SSC_NUMBER,      "csp.dtr.cost.sales_tax.total",    "Sales tax total",                                                           "$",             "",               "Capital Costs",  "",                                 "",                      "" },

        // Total Installed Costs
    { SSC_OUTPUT,       SSC_NUMBER,      "total_installed_cost",      "Total installed cost",                                                            "$",             "",               "Capital Costs",  "",                                 "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "csp.dtr.cost.installed_per_capacity", "Estimated total installed cost per net capacity ($/kW)",                "$/kW",          "",               "Capital Costs",  "",                                 "",                      "" },

        // Financing
    { SSC_OUTPUT,       SSC_NUMBER,      "const_per_principal1",             "Principal, loan 1",                                                        "$",             "",               "Financial Parameters",   "csp_financial_model=1",    "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "const_per_principal2",             "Principal, loan 2",                                                        "$",             "",               "Financial Parameters",   "csp_financial_model=1",    "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "const_per_principal3",             "Principal, loan 3",                                                        "$",             "",               "Financial Parameters",   "csp_financial_model=1",    "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "const_per_principal4",             "Principal, loan 4",                                                        "$",             "",               "Financial Parameters",   "csp_financial_model=1",    "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "const_per_principal5",             "Principal, loan 5",                                                        "$",             "",               "Financial Parameters",   "csp_financial_model=1",    "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "const_per_interest1",              "Interest cost, loan 1",                                                    "$",             "",               "Financial Parameters",   "csp_financial_model=1",    "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "const_per_interest2",              "Interest cost, loan 2",                                                    "$",             "",               "Financial Parameters",   "csp_financial_model=1",    "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "const_per_interest3",              "Interest cost, loan 3",                                                    "$",             "",               "Financial Parameters",   "csp_financial_model=1",    "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "const_per_interest4",              "Interest cost, loan 4",                                                    "$",             "",               "Financial Parameters",   "csp_financial_model=1",    "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "const_per_interest5",              "Interest cost, loan 5",                                                    "$",             "",               "Financial Parameters",   "csp_financial_model=1",    "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "const_per_total1",                 "Total financing cost, loan 1",                                             "$",             "",               "Financial Parameters",   "csp_financial_model=1",    "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "const_per_total2",                 "Total financing cost, loan 2",                                             "$",             "",               "Financial Parameters",   "csp_financial_model=1",    "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "const_per_total3",                 "Total financing cost, loan 3",                                             "$",             "",               "Financial Parameters",   "csp_financial_model=1",    "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "const_per_total4",                 "Total financing cost, loan 4",                                             "$",             "",               "Financial Parameters",   "csp_financial_model=1",    "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "const_per_total5",                 "Total financing cost, loan 5",                                             "$",             "",               "Financial Parameters",   "csp_financial_model=1",    "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "const_per_percent_total",          "Total percent of installed costs, all loans",                              "%",             "",               "Financial Parameters",   "csp_financial_model=1",    "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "const_per_principal_total",        "Total principal, all loans",                                               "$",             "",               "Financial Parameters",   "csp_financial_model=1",    "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "const_per_interest_total",         "Total interest costs, all loans",                                          "$",             "",               "Financial Parameters",   "csp_financial_model=1",    "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "construction_financing_cost",      "Total construction financing cost",                                        "$",             "",               "Financial Parameters",   "csp_financial_model=1",    "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "ppa_price_input",			         "PPA prices - yearly",			                                             "$/kWh",	      "",	            "Revenue",			      "ppa_multiplier_model=0&csp_financial_model<5&is_dispatch=1","",      	     "" },


    // Simulation Kernel
    { SSC_OUTPUT,       SSC_ARRAY,       "time_hr",                   "Time at end of timestep",                                                          "hr",           "",               "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "timestep_sim_duration",     "Simulation duration of timestep",                                                  "s",            "",               "solver",         "sim_type=1",                       "",                      "" },
       
    // Weather Reader
    { SSC_OUTPUT,       SSC_ARRAY,       "month",                     "Resource Month",                                                                   "",             "",               "weather",        "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "hour_day",                  "Resource Hour of Day",                                                             "",             "",               "weather",        "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "solazi",                    "Resource Solar Azimuth",                                                           "deg",          "",               "weather",        "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "solzen",                    "Resource Solar Zenith",                                                            "deg",          "",               "weather",        "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "beam",                      "Resource Beam normal irradiance",                                                  "W/m2",         "",               "weather",        "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "tdry",                      "Resource Dry bulb temperature",                                                    "C",            "",               "weather",        "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "twet",                      "Resource Wet bulb temperature",                                                    "C",            "",               "weather",        "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "rh",                        "Resource Relative Humidity",                                                       "%",            "",               "weather",        "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "wspd",                      "Resource Wind Speed",                                                              "m/s",          "",               "weather",        "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "pres",                      "Resource Pressure",                                                                "mbar",         "",               "weather",        "sim_type=1",                       "",                      "" },
   
    { SSC_OUTPUT,       SSC_ARRAY,       "defocus",                   "Field optical focus fraction",                                                     "",             "",               "weather",        "sim_type=1",                       "",                      "" },

    // Solar Field                                                                                                                                                                                                                   
    { SSC_OUTPUT,       SSC_ARRAY,       "Theta_ave",                 "Field collector solar incidence angle",                                            "deg",          "",               "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "CosTh_ave",                 "Field collector cosine efficiency",                                                "",             "",               "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "IAM_ave",                   "Field collector incidence angle modifier",                                         "",             "",               "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "RowShadow_ave",             "Field collector row shadowing loss",                                               "",             "",               "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "EndLoss_ave",               "Field collector optical end loss",                                                 "",             "",               "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "dni_costh",                 "Field collector DNI-cosine product",                                               "W/m2",         "",               "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "EqOpteff",                  "Field optical efficiency before defocus",                                          "",             "",               "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "SCAs_def",                  "Field fraction of focused SCAs",                                                   "",             "",               "solar_field",    "sim_type=1",                       "",                      "" },
                                                                                                                                                                                                                                                               
    { SSC_OUTPUT,       SSC_ARRAY,       "q_inc_sf_tot",              "Field thermal power incident",                                                     "MWt",          "",               "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "qinc_costh",                "Field thermal power incident after cosine",                                        "MWt",          "",               "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "q_dot_rec_inc",             "Receiver thermal power incident",                                                  "MWt",          "",               "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "q_dot_rec_thermal_loss",    "Receiver thermal losses",                                                          "MWt",          "",               "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "q_dot_rec_abs",             "Receiver thermal power absorbed",                                                  "MWt",          "",               "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "q_dot_piping_loss",         "Field piping thermal losses",                                                      "MWt",          "",               "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "e_dot_field_int_energy",    "Field change in material/htf internal energy",                                     "MWt",          "",               "solar_field",    "sim_type=1",                       "",                      "" }, 
    { SSC_OUTPUT,       SSC_ARRAY,       "q_dot_htf_sf_out",          "Field thermal power leaving in HTF",                                               "MWt",          "",               "solar_field",    "sim_type=1",                       "",                      "" },  
    { SSC_OUTPUT,       SSC_ARRAY,       "q_dot_freeze_prot",         "Field freeze protection required",                                                 "MWt",          "",               "solar_field",    "sim_type=1",                       "",                      "" },

    { SSC_OUTPUT,       SSC_ARRAY,       "rec_time_in_startup",       "Field time at startup",                                                            "min",          "",               "solar_field",    "sim_type=1",                       "",                      "" },

    { SSC_OUTPUT,       SSC_ARRAY,       "m_dot_loop",                "Receiver mass flow rate",                                                          "kg/s",         "",               "solar_field",    "sim_type=1",                       "",                      "" },  
    { SSC_OUTPUT,       SSC_ARRAY,       "m_dot_field_recirc",        "Field total mass flow recirculated",                                               "kg/s",         "",               "solar_field",    "sim_type=1",                       "",                      "" },  
    { SSC_OUTPUT,       SSC_ARRAY,       "m_dot_field_delivered",     "Field total mass flow delivered",                                                  "kg/s",         "",               "solar_field",    "sim_type=1",                       "",                      "" },  
    { SSC_OUTPUT,       SSC_ARRAY,       "T_field_cold_in",           "Field timestep-averaged inlet temperature",                                        "C",            "",               "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "T_rec_cold_in",             "Loop timestep-averaged inlet temperature",                                         "C",            "",               "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "T_rec_hot_out",             "Loop timestep-averaged outlet temperature",                                        "C",            "",               "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "T_field_hot_out",           "Field timestep-averaged outlet temperature",                                       "C",            "",               "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "deltaP_field",              "Field pressure drop",                                                              "bar",          "",               "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "vel_loop_min",              "Receiver min HTF velocity in loop",                                                "m/s",          "",               "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "vel_loop_max",              "Receiver max HTF velocity in loop",                                                "m/s",          "",               "solar_field",    "sim_type=1",                       "",                      "" },


    { SSC_OUTPUT,       SSC_ARRAY,       "W_dot_sca_track",           "Field collector tracking power",                                                   "MWe",          "",               "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "W_dot_field_pump",          "Field htf pumping power",                                                          "MWe",          "",               "solar_field",    "sim_type=1",                       "",                      "" },

    { SSC_OUTPUT,       SSC_ARRAY,       "pipe_header_diams",         "Field piping header diameters",                                                    "m",            "",               "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "pipe_header_wallthk",       "Field piping header wall thicknesses",                                             "m",            "",               "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "pipe_header_lengths",       "Field piping header lengths",                                                      "m",            "",               "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "pipe_header_expansions",    "Number of field piping header expansions",                                         "-",            "",               "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "pipe_header_mdot_dsn",      "Field piping header mass flow at design",                                          "kg/s",         "",               "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "pipe_header_vel_dsn",       "Field piping header velocity at design",                                           "m/s",          "",               "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "pipe_header_T_dsn",         "Field piping header temperature at design",                                        "C",            "",               "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "pipe_header_P_dsn",         "Field piping header pressure at design",                                           "bar",          "",               "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "pipe_runner_diams",         "Field piping runner diameters",                                                    "m",            "",               "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "pipe_runner_wallthk",       "Field piping runner wall thicknesses",                                             "m",            "",               "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "pipe_runner_lengths",       "Field piping runner lengths",                                                      "m",            "",               "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "pipe_runner_expansions",    "Number of field piping runner expansions",                                         "-",            "",               "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "pipe_runner_mdot_dsn",      "Field piping runner mass flow at design",                                          "kg/s",         "",               "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "pipe_runner_vel_dsn",       "Field piping runner velocity at design",                                           "m/s",          "",               "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "pipe_runner_T_dsn",         "Field piping runner temperature at design",                                        "C",            "",               "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "pipe_runner_P_dsn",         "Field piping runner pressure at design",                                           "bar",          "",               "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "pipe_loop_T_dsn",           "Field piping loop temperature at design",                                          "C",            "",               "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "pipe_loop_P_dsn",           "Field piping loop pressure at design",                                             "bar",          "",               "solar_field",    "sim_type=1",                       "",                      "" },

    // Heat Sink
    { SSC_OUTPUT,       SSC_ARRAY,      "q_dot_to_heat_sink",         "Heat sink thermal power",                                                          "MWt",          "",               "Heat_Sink",      "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "W_dot_pc_pump",              "Heat sink pumping power",                                                          "MWe",          "",               "Heat_Sink",      "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "m_dot_htf_heat_sink",        "Heat sink HTF mass flow",                                                          "kg/s",         "",               "Heat_Sink",      "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "T_heat_sink_in",             "Heat sink HTF inlet temp",                                                         "C",            "",               "Heat_Sink",      "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "T_heat_sink_out",            "Heat sink HTF outlet temp",                                                        "C",            "",               "Heat_Sink",      "sim_type=1",                       "",                      "" },
                                                                                                                                                                                            
    { SSC_OUTPUT,       SSC_ARRAY,      "m_dot_wf_heat_sink",         "Heat sink steam mass flow rate",                                                   "kg/s",         "",               "Heat_Sink",      "sim_type=1&hs_type=1",             "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "x_out_wf_heat_sink",         "Heat sink steam outlet quality",                                                   "-",            "",               "Heat_Sink",      "sim_type=1&hs_type=1",             "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "T_out_wf_heat_sink",         "Heat sink steam outlet temp",                                                      "C",            "",               "Heat_Sink",      "sim_type=1&hs_type=1",             "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "hx_min_dT_heat_sink",        "Heat sink HX min temp difference",                                                 "C",            "",               "Heat_Sink",      "sim_type=1&hs_type=1",             "",                      "" },

    // TES
    { SSC_OUTPUT,       SSC_ARRAY,       "tank_losses",               "TES thermal losses",                                                               "MWt",          "",               "TES",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "q_tes_heater",              "TES freeze protection power",                                                      "MWe",          "",               "TES",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "T_tes_hot",                 "TES hot temperature",                                                              "C",            "",               "TES",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "T_tes_cold",                "TES cold temperature",                                                             "C",            "",               "TES",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "mass_tes_cold",             "TES cold tank mass (end)",                                                         "kg",           "",               "TES",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "mass_tes_hot",              "TES hot tank mass (end)",                                                          "kg",           "",               "TES",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "q_dc_tes",                  "TES discharge thermal power",                                                      "MWt",          "",               "TES",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "q_ch_tes",                  "TES charge thermal power",                                                         "MWt",          "",               "TES",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "e_ch_tes",                  "TES charge state",                                                                 "MWht",         "",               "TES",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "m_dot_cr_to_tes_hot",       "Mass flow: field to hot TES",                                                      "kg/s",         "",               "TES",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "m_dot_tes_hot_out",         "Mass flow: TES hot out",                                                           "kg/s",         "",               "TES",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "m_dot_pc_to_tes_cold",      "Mass flow: cycle to cold TES",                                                     "kg/s",         "",               "TES",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "m_dot_tes_cold_out",        "Mass flow: TES cold out",                                                          "kg/s",         "",               "TES",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "m_dot_field_to_cycle",      "Mass flow: field to cycle",                                                        "kg/s",         "",               "TES",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "m_dot_cycle_to_field",      "Mass flow: cycle to field",                                                        "kg/s",         "",               "TES",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "m_dot_cold_tank_to_hot_tank", "Mass flow: cold tank to hot tank",                                               "kg/s",         "",               "TES",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "tes_htf_pump_power",         "TES HTF pump power",                                                              "MWe",          "",               "TES",            "sim_type=1",                       "",                      "" },

    // NT TES
    { SSC_OUTPUT,       SSC_ARRAY,       "vol_tes_cold",              "TES cold fluid volume",                                                            "m3",           "",               "TES",            "sim_type=1&tes_type=3",            "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "vol_tes_hot",               "TES hot fluid volume",                                                             "m3" ,          "",               "TES",            "sim_type=1&tes_type=3",            "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "vol_tes_tot",               "TES total fluid volume",                                                           "m3",           "",               "TES",            "sim_type=1",            "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "tes_piston_loc",            "TES piston distance from left (cold) side",                                        "m",            "",               "TES",            "sim_type=1&tes_type=3",            "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "tes_piston_frac",           "TES piston fraction of cold distance over total",                                  "",             "",               "TES",            "sim_type=1&tes_type=3",            "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "tes_cold_vol_frac",         "TES volume fraction of cold over total",                                           "",             "",               "TES",            "sim_type=1&tes_type=3",            "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "tes_mass_tot",              "TES total fluid mass",                                                             "kg",           "",               "TES",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "tes_SA_cold",               "TES cold side surface area",                                                       "m2",           "",               "TES",            "sim_type=1&tes_type=3",            "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "tes_SA_hot",                "TES hot side surface area",                                                        "m2",           "",               "TES",            "sim_type=1&tes_type=3",            "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "tes_SA_tot",                "TES total surface area",                                                           "m2",           "",               "TES",            "sim_type=1&tes_type=3",            "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "tes_error",                 "TES energy balance error",                                                         "MWt",          "",               "TES",            "sim_type=1&tes_type=3",            "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "tes_error_percent",         "TES energy balance error percent",                                                 "%",            "",               "TES",            "sim_type=1&tes_type=3",            "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "tes_leak_error",            "TES energy balance error due to leakage assumption",                               "MWt",          "",               "TES",            "sim_type=1&tes_type=3",              "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "tes_wall_error",            "TES energy balance error due to wall temperature assumption",                      "MWt",           "",               "TES",           "sim_type=1&tes_type=3",              "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "tes_error_corrected",       "TES energy balance error, accounting for wall and temperature assumption error",   "MWt",           "",              "TES",            "sim_type=1&tes_type=3",              "",                      "" },

    // Packed Bed TES
    { SSC_OUTPUT,       SSC_ARRAY,       "T_grad_0",                  "TES Temperature gradient 0 indice",                                                "C",            "",               "TES",            "sim_type=1&tes_type=2",                                 "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "T_grad_1",                  "TES Temperature gradient 1 indice",                                                "C",            "",               "TES",            "sim_type=1&tes_type=2",                                 "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "T_grad_2",                  "TES Temperature gradient 2 indice",                                                "C",            "",               "TES",            "sim_type=1&tes_type=2",                                 "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "T_grad_3",                  "TES Temperature gradient 3 indice",                                                "C",            "",               "TES",            "sim_type=1&tes_type=2",                                 "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "T_grad_4",                  "TES Temperature gradient 4 indice",                                                "C",            "",               "TES",            "sim_type=1&tes_type=2",                                 "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "T_grad_5",                  "TES Temperature gradient 5 indice",                                                "C",            "",               "TES",            "sim_type=1&tes_type=2",                                 "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "T_grad_6",                  "TES Temperature gradient 6 indice",                                                "C",            "",               "TES",            "sim_type=1&tes_type=2",                                 "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "T_grad_7",                  "TES Temperature gradient 7 indice",                                                "C",            "",               "TES",            "sim_type=1&tes_type=2",                                 "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "T_grad_8",                  "TES Temperature gradient 8 indice",                                                "C",            "",               "TES",            "sim_type=1&tes_type=2",                                 "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "T_grad_9",                  "TES Temperature gradient 9 indice",                                                "C",            "",               "TES",            "sim_type=1&tes_type=2",                                 "",                      "" },

    //{ SSC_OUTPUT,       SSC_ARRAY,       "m_dot_tes_dc",              "TES discharge mass flow rate",                                                     "kg/s",         "",               "TES",            "*",                       "",                      "" },
    //{ SSC_OUTPUT,       SSC_ARRAY,       "m_dot_tes_ch",              "TES charge mass flow rate",                                                        "kg/s",         "",               "TES",            "*",                       "",                      "" },
    
    // SYSTEM
    { SSC_OUTPUT,       SSC_ARRAY,       "W_dot_parasitic_tot",       "System total electrical parasitic",                                                "MWe",          "",               "system",         "sim_type=1",                       "",                      "" },
                                                                                                                                                                                                            
    // Controller                                                                                                                                                                                           
    { SSC_OUTPUT,       SSC_ARRAY,       "op_mode_1",                 "1st operating mode",                                                               "",             "",               "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "op_mode_2",                 "2nd op. mode, if applicable",                                                      "",             "",               "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "op_mode_3",                 "3rd op. mode, if applicable",                                                      "",             "",               "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "m_dot_balance",             "Relative mass flow balance error",                                                 "",             "",               "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "q_balance",                 "Relative energy balance error",                                                    "",             "",               "solver",         "sim_type=1",                       "",                      "" },

    // Monthly Outputs
    { SSC_OUTPUT,       SSC_ARRAY,       "monthly_energy",            "Monthly Energy Gross",                                                             "kWht",          "",               "Post-process",   "sim_type=1",              "LENGTH=12",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "monthly_energy_heat_btu",   "Monthly Energy Gross in MMBtu",                                                    "MMBtu",          "",               "Post-process",   "sim_type=1",              "LENGTH=12",                      "" },


    // Annual Outputs
    { SSC_OUTPUT,       SSC_NUMBER,      "annual_energy",                   "Annual net thermal energy w/ avail. derate",                                 "kWht",       "",               "Post-process",   "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "annual_energy_heat_btu",          "Annual net thermal energy w/ avail. derate",                                 "MMBtu",       "",               "Post-process",   "sim_type=1",                       "",                      "" },

    //{ SSC_OUTPUT,       SSC_NUMBER,      "annual_gross_energy",             "Annual Gross Electrical Energy Production w/ avail derate",                  "kWhe",       "",               "Post-process",   "*",                       "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "annual_thermal_consumption",      "Annual thermal freeze protection required",                                  "kWht",       "",               "Post-process",   "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "annual_electricity_consumption",  "Annual electricity consumption w/ avail derate",                             "kWhe",       "",               "Post-process",   "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "annual_total_water_use",          "Total Annual Water Usage",                                                   "m^3",          "",               "Post-process",   "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "annual_field_freeze_protection",  "Annual thermal power for field freeze protection",                           "kWht",       "",               "Post-process",   "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "annual_tes_freeze_protection",    "Annual thermal power for TES freeze protection",                             "kWht",       "",               "Post-process",   "sim_type=1",                       "",                      "" },

    // Newly added
    { SSC_OUTPUT,       SSC_ARRAY,       "n_op_modes",                "Operating modes in reporting timestep",                                            "",             "",               "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "tou_value",                 "CSP operating Time-of-use value",                                                  "",             "",               "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "pricing_mult",              "PPA price multiplier",                                                             "",             "",               "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "elec_price_out",            "Electricity price at timestep",                                                    "",             "",               "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "q_dot_pc_sb",               "Thermal power for PC standby",                                                     "MWt",          "",               "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "q_dot_pc_min",              "Thermal power for PC min operation",                                               "MWt",          "",               "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "q_dot_pc_target",           "Target thermal power to PC",                                                       "MWt",          "",               "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "q_dot_pc_max",              "Max thermal power to PC",                                                          "MWt",          "",               "solver",         "sim_type=1",                       "",                      "" },
                                                                                                                                                                                                                                                                  
    { SSC_OUTPUT,       SSC_ARRAY,       "is_rec_su_allowed",         "is receiver startup allowed",                                                      "",             "",               "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "is_pc_su_allowed",          "is power cycle startup allowed",                                                   "",             "",               "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "is_pc_sb_allowed",          "is power cycle standby allowed",                                                   "",             "",               "solver",         "sim_type=1",                       "",                      "" },

    { SSC_OUTPUT,       SSC_ARRAY,       "is_PAR_HTR_allowed",        "Is parallel electric heater operation allowed",                                    "",             "",               "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "q_dot_elec_to_PAR_HTR",     "Electric heater thermal power target",                                             "MWt",          "",               "solver",         "sim_type=1",                       "",                      "" },

    { SSC_OUTPUT,       SSC_ARRAY,       "q_dot_est_cr_su",           "Estimate rec. startup thermal power",                                              "MWt",           "",              "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "q_dot_est_cr_on",           "Estimate rec. thermal power TO HTF",                                               "MWt",          "",               "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "q_dot_est_tes_dc",          "Estimate max TES discharge thermal power",                                         "MWt",          "",               "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "q_dot_est_tes_ch",          "Estimate max TES charge thermal power",                                            "MWt",          "",               "solver",         "sim_type=1",                       "",                      "" },
                                                                                                                                                                                                                                                                  
    { SSC_OUTPUT,       SSC_ARRAY,       "operating_modes_a",         "First 3 operating modes tried",                                                    "",             "",               "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "operating_modes_b",         "Next 3 operating modes tried",                                                     "",             "",               "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "operating_modes_c",         "Final 3 operating modes tried",                                                    "",             "",               "solver",         "sim_type=1",                       "",                      "" },

    // Heater outputs is_parallel_htr
    { SSC_OUTPUT,       SSC_ARRAY,      "W_dot_heater",               "Parallel heater electricity consumption",                                          "MWe",          "",               "Parallel Heater","sim_type=1&is_parallel_htr=1",     "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "q_dot_heater_to_htf",        "Parallel heater thermal power to HTF",                                             "MWt",          "",               "Parallel Heater","sim_type=1&is_parallel_htr=1",     "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "q_dot_heater_startup",       "Parallel heater thermal power consumed during startup",                            "MWt",          "",               "Parallel Heater","sim_type=1&is_parallel_htr=1",     "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "m_dot_htf_heater",           "Parallel heater HTF mass flow rate",                                               "kg/s",         "",               "Parallel Heater","sim_type=1&is_parallel_htr=1",     "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "T_htf_heater_in",            "Parallel heater HTF inlet temperature",                                            "C",            "",               "Parallel Heater","sim_type=1&is_parallel_htr=1",     "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "T_htf_heater_out",           "Parallel heater HTF outlet temperature",                                           "C",            "",               "Parallel Heater","sim_type=1&is_parallel_htr=1",     "",                      "" },

    // Dispatch
    { SSC_OUTPUT,       SSC_ARRAY,       "disp_rel_mip_gap",          "Dispatch relative MIP gap",                                                        "",             "",               "tou",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "disp_solve_state",          "Dispatch solver state",                                                            "",             "",               "tou",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "disp_subopt_flag",          "Dispatch suboptimal solution flag",                                                "",             "",               "tou",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "disp_solve_iter",           "Dispatch iterations count",                                                        "",             "",               "tou",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "disp_objective",            "Dispatch objective function value",                                                "",             "",               "tou",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "disp_obj_relax",            "Dispatch objective function - relaxed max",                                        "",             "",               "tou",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "disp_qsf_expected",         "Dispatch expected solar field available energy",                                   "MWt",          "",               "tou",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "disp_qsfprod_expected",     "Dispatch expected solar field generation",                                         "MWt",          "",               "tou",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "disp_qsfsu_expected",       "Dispatch expected solar field startup energy",                                     "MWt",          "",               "tou",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "disp_tes_expected",         "Dispatch expected TES charge level",                                               "MWht",         "",               "tou",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "disp_thermeff_expected",    "Dispatch expected SF thermal efficiency adj.",                                     "",             "",               "tou",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "disp_presolve_nconstr",     "Dispatch number of constraints in problem",                                        "",             "",               "tou",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "disp_presolve_nvar",        "Dispatch number of variables in problem",                                          "",             "",               "tou",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "disp_solve_time",           "Dispatch solver time",                                                             "sec",          "",               "tou",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "avg_suboptimal_rel_mip_gap","Average suboptimal relative MIP gap",                                              "%",            "",               "tou",            "sim_type=1",                       "",                      "" },

    { SSC_OUTPUT,       SSC_ARRAY,       "P_fixed",                   "Parasitic power fixed load",                                                       "MWe",          "",               "system",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "P_plant_balance_tot",       "Parasitic power generation-dependent load",                                        "MWe",          "",               "system",         "sim_type=1",                       "",                      "" },
                                                                                                                                                                                                                                                                  
    { SSC_OUTPUT,       SSC_ARRAY,       "gen_heat",                  "System net thermal power w/ avail. derate",                                        "kWt",          "",               "system",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "gen",                       "System net electrical power w/ avail. derate",                                     "kWe",          "",               "system",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "gen_heat_btu",              "System net thermal power w/ avail. derate",                                        "MMBtu/hr",     "",               "system",         "sim_type=1",                       "",                      "" },

    //{ SSC_OUTPUT,       SSC_NUMBER,      "conversion_factor",         "Gross to Net Conversion Factor",                                                   "%",            "",               "system",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "capacity_factor",           "Capacity factor",                                                                  "%",            "",               "system",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "kwh_per_kw",                "First year kWh/kW",                                                                "kWh/kW",       "",               "system",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "sim_duration",              "Computational time of timeseries simulation",                                      "s",            "",               "system",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "W_dot_par_tot_haf",         "Adjusted parasitic power",                                                         "kWe",          "",               "system",         "sim_type=1",                       "",                      "" },
    //{ SSC_OUTPUT,       SSC_NUMBER,      "q_dot_defocus_est",         "Thermal energy intentionally lost by defocusing",                                  "MWt",          "",               "system",         "*",                       "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "heat_load_capacity_factor", "Percentage of heat load met",                                                      "%",            "",               "system",         "sim_type=1",                       "",                      "" },

    { SSC_OUTPUT,       SSC_ARRAY,       "recirculating",             "Field recirculating (bypass valve open)",                                          "-",            "",               "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "pipe_tes_diams",            "Pipe diameters in TES",                                                            "m",            "",               "TES",            "sim_type=1&tes_type=1",            "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "pipe_tes_wallthk",          "Pipe wall thickness in TES",                                                       "m",            "",               "TES",            "sim_type=1&tes_type=1",            "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "pipe_tes_lengths",          "Pipe lengths in TES",                                                              "m",            "",               "TES",            "sim_type=1&tes_type=1",            "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "pipe_tes_mdot_dsn",         "Mass flow TES pipes at design conditions",                                         "kg/s",         "",               "TES",            "sim_type=1&tes_type=1",            "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "pipe_tes_vel_dsn",          "Velocity in TES pipes at design conditions",                                       "m/s",          "",               "TES",            "sim_type=1&tes_type=1",            "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "pipe_tes_T_dsn",            "Temperature in TES pipes at design conditions",                                    "C",            "",               "TES",            "sim_type=1&tes_type=1",            "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "pipe_tes_P_dsn",            "Pressure in TES pipes at design conditions",                                       "bar",          "",               "TES",            "sim_type=1&tes_type=1",            "",                      "" },


    //{ SSC_OUTPUT,       SSC_ARRAY,       "defocus",                   "Field optical focus fraction",                                                     "",             "",               "solver",         "*",                       "",                      "" },

    var_info_invalid };
    
    
    
class cm_trough_physical_iph : public compute_module
{
public:

    cm_trough_physical_iph()
    {
        add_var_info( _cm_vtab_trough_physical_iph );
        add_var_info( vtab_adjustment_factors );
        add_var_info(vtab_technology_outputs);
        add_var_info(vtab_utility_rate_common); // Required for dispatch w/ utility rates

    }

    void exec( )
    {
        // Uncomment following 2 lines to write cmod inputs to LK script
        //FILE* fp = fopen("trough_iph_cmod_to_lk.lk", "w");
        //write_cmod_to_lk_script(fp, m_vartab);

        // Common Parameters
        int sim_type = as_number("sim_type");
        int csp_financial_model = as_integer("csp_financial_model");
        double T_htf_cold_des = as_double("T_loop_in_des");    //[C]
        double T_htf_hot_des = as_double("T_loop_out");      //[C]
        double tshours = as_double("tshours");                  //[-]
        double q_dot_hs_des = as_double("q_pb_design");         //[MWt] HEAT SINK design thermal power
        double Q_tes = q_dot_hs_des * tshours;                  //[MWht]
        int is_dispatch = as_boolean("is_dispatch");
        
        int tes_type = as_integer("tes_type");
        const double MMBTU_TO_KWh = 293.07107; // 1 MMBtu = 293.07107 kWh

        // Convert IPH Input Units
        {
            if (is_assigned("ppa_price_input_heat_btu"))
            {
                size_t count_ppa_price_MMBTU_input;
                ssc_number_t* ppa_price_MMBTU_input_array = as_array("ppa_price_input_heat_btu", &count_ppa_price_MMBTU_input);
                std::vector<ssc_number_t> ppa_price_input_vec;
                for (int i = 0; i < count_ppa_price_MMBTU_input; i++)
                {
                    ppa_price_input_vec.push_back(ppa_price_MMBTU_input_array[i] / MMBTU_TO_KWh);
                }
                set_vector("ppa_price_input", ppa_price_input_vec);
            }
        }

        // *****************************************************
        // System Design Parameters


        // ********************************
        // ********************************
        // Weather reader
        // ********************************
        // ********************************
        C_csp_weatherreader weather_reader;
        C_csp_solver::S_sim_setup sim_setup;
        int n_steps_fixed;
        double lat = std::numeric_limits<double>::quiet_NaN();
        int steps_per_hour;
        {
            if (is_assigned("file_name")) {
                weather_reader.m_weather_data_provider = make_shared<weatherfile>(as_string("file_name"));
                if (weather_reader.m_weather_data_provider->has_message()) log(weather_reader.m_weather_data_provider->message(), SSC_WARNING);
            }
            if (is_assigned("solar_resource_data")) {
                weather_reader.m_weather_data_provider = make_shared<weatherdata>(lookup("solar_resource_data"));
                if (weather_reader.m_weather_data_provider->has_message()) log(weather_reader.m_weather_data_provider->message(), SSC_WARNING);
            }

            weather_reader.m_filename = as_string("file_name");
            weather_reader.m_trackmode = 0;
            weather_reader.m_tilt = 0.0;
            weather_reader.m_azimuth = 0.0;
            // Initialize to get weather file info
            weather_reader.init();
            if (weather_reader.has_error()) throw exec_error("trough_physical", weather_reader.get_error());

            // Set up ssc output arrays
            // Set steps per hour
            double nhourssim = 8760.0;                                  //[hr] Number of hours to simulate
            
            sim_setup.m_sim_time_start = 0.0;                           //[s] starting first hour of year
            sim_setup.m_sim_time_end = nhourssim * 3600.;                 //[s] full year simulation

            steps_per_hour = 1;                                     //[-]

            int n_wf_records = (int)weather_reader.m_weather_data_provider->nrecords();
            steps_per_hour = n_wf_records / 8760;                       //[-]

            lat = weather_reader.ms_solved_params.m_lat;     //[deg]

            n_steps_fixed = steps_per_hour * 8760;                    //[-]
            sim_setup.m_report_step = 3600.0 / (double)steps_per_hour;  //[s]
        }
        
        // ********************************
        // ********************************
        // Solar field, trough
        // ********************************
        // ********************************
        double q_dot_rec_des_ideal_pre_init = std::numeric_limits<double>::quiet_NaN();
        C_csp_trough_collector_receiver c_trough;
        {
            // Collect Inputs
            {
                // ADDED Trough Inputs (TMB 10/06/2023) for design point calculations
                //std::vector<double> trough_loop_vec = as_vector_double("trough_loop_control");
                //c_trough.m_trough_loop_control.assign(&trough_loop_vec[0], trough_loop_vec.size());

                //int actual_nSCA = trough_loop_vec[0];

                c_trough.m_use_solar_mult_or_aperture_area = as_number("use_solar_mult_or_aperture_area"); // Use specified solar mult (0) or total aperture (1)
                c_trough.m_specified_solar_mult = as_number("specified_solar_multiple");            // User specified solar mult
                c_trough.m_specified_total_aperture = as_number("specified_total_aperture");    //[m2] User specified total aperture
                //c_trough.m_nSCA = actual_nSCA;   CALCULATED INTERNAL                           //[-] Number of SCA's in a loop
                c_trough.m_nHCEt = as_integer("nHCEt");                     //[-] Number of HCE types
                c_trough.m_nColt = as_integer("nColt");                     //[-] Number of collector types
                c_trough.m_nHCEVar = as_integer("nHCEVar");                 //[-] Number of HCE variants per t
                //c_trough.m_nLoops = as_integer("nLoops");                   //[-] Number of loops in the field
                c_trough.m_FieldConfig = as_integer("FieldConfig");         //[-] Number of subfield headers
                //c_trough.m_L_power_block_piping = as_double("L_power_block_piping");                           //[m] Length of piping (full mass flow) through power block (if applicable)
                //c_trough.m_include_fixed_power_block_runner = as_boolean("include_fixed_power_block_runner");  //[-] Should model consider piping through power block?
                c_trough.m_eta_pump = as_double("eta_pump");                //[-] HTF pump efficiency
                c_trough.m_Fluid = as_integer("Fluid");                     //[-] Field HTF fluid number
                //c_trough.m_fthrok = as_integer("fthrok");                 //[-] Flag to allow partial defocusing of the collectors
                c_trough.m_fthrctrl = 2;                                    //[-] Defocusing strategy; hardcode = 2 for now
                c_trough.m_accept_loc = as_integer("accept_loc");           //[-] In acceptance testing mode - temperature sensor location (1=hx,2=loop)
                c_trough.m_HDR_rough = as_double("HDR_rough");              //[-] Header pipe relative roughness
                c_trough.m_theta_stow = as_double("theta_stow");            //[deg] stow angle
                c_trough.m_theta_dep = as_double("theta_dep");              //[deg] deploy angle
                c_trough.m_Row_Distance = as_double("Row_Distance");        //[m] Spacing between rows (centerline to centerline)

                double T_loop_in_des = as_double("T_loop_in_des");          //[C] Design loop inlet temperature, converted to K in init
                c_trough.m_T_loop_in_des = T_loop_in_des;                   //[C] Design loop inlet temperature, converted to K in init
                double T_loop_out_des = as_double("T_loop_out");            //[C] Target loop outlet temperature, converted to K in init
                c_trough.m_T_loop_out_des = T_loop_out_des;                 //[C] Target loop outlet temperature, converted to K in init

                // Startup and Shutdown temperatures
                double T_startup_min = T_loop_in_des;
                if (T_loop_out_des > 600.0)
                {
                    T_startup_min = T_loop_out_des - 70.0;
                }
                double T_startup_old = max(T_startup_min, 0.67 * T_loop_in_des + 0.33 * T_loop_out_des); //[C]

                double T_startup = T_startup_old;
                if (is_assigned("T_startup"))
                {
                    T_startup = as_double("T_startup");
                }

                double T_shutdown = T_startup;
                if (is_assigned("T_shutdown"))
                {
                    T_shutdown = as_double("T_shutdown");
                }

                c_trough.m_T_startup = T_startup;            //[C] The required temperature (converted to K in init) of the system before the power block can be switched on
                c_trough.m_T_shutdown = T_shutdown;            //[C]

                c_trough.m_use_abs_or_rel_mdot_limit = as_integer("use_abs_or_rel_mdot_limit"); // Use mass flow abs (0) or relative (1) limits
                c_trough.m_m_dot_htfmin_in = as_double("m_dot_htfmin");        //[kg/s] Minimum loop HTF flow rate
                c_trough.m_m_dot_htfmax_in = as_double("m_dot_htfmax");        //[kg/s] Maximum loop HTF flow rate
                c_trough.m_f_htfmin_in = as_double("f_htfmin");                //[] Minimum loop htf flow rate fraction
                c_trough.m_f_htfmax_in = as_double("f_htfmax");                //[] Maximum loop htf flow rate fraction


                c_trough.m_field_fl_props = as_matrix("field_fl_props");    //[-] User-defined field HTF properties
                c_trough.m_T_fp = as_double("T_fp");                        //[C] Freeze protection temperature (heat trace activation temperature), convert to K in init
                c_trough.m_I_bn_des = as_double("I_bn_des");                //[W/m^2] Solar irradiation at design
                c_trough.m_V_hdr_cold_max = as_double("V_hdr_cold_max");    //[m/s] Maximum HTF velocity in the cold header at design
                c_trough.m_V_hdr_cold_min = as_double("V_hdr_cold_min");    //[m/s] Minimum HTF velocity in the cold header at design
                c_trough.m_V_hdr_hot_max = as_double("V_hdr_hot_max");      //[m/s] Maximum HTF velocity in the hot header at design
                c_trough.m_V_hdr_hot_min = as_double("V_hdr_hot_min");      //[m/s] Minimum HTF velocity in the hot header at design
                c_trough.m_Pipe_hl_coef = as_double("Pipe_hl_coef");        //[W/m2-K] Loss coefficient from the header, runner pipe, and non-HCE piping
                c_trough.m_SCA_drives_elec = as_double("SCA_drives_elec");  //[W/SCA] Tracking power, in Watts per SCA drive
                c_trough.m_ColTilt = as_double("tilt");                     //[deg] Collector tilt angle (0 is horizontal, 90deg is vertical)
                c_trough.m_ColAz = as_double("azimuth");                    //[deg] Collector azimuth angle
                c_trough.m_wind_stow_speed = as_double("wind_stow_speed");  //[m/s] Wind speed at and above which the collectors will be stowed
                c_trough.m_accept_mode = as_integer("accept_mode");         //[-] Acceptance testing mode? (1=yes, 0=no)
                c_trough.m_accept_init = as_boolean("accept_init");         //[-] In acceptance testing mode - require steady-state startup
                c_trough.m_mc_bal_hot_per_MW = as_double("mc_bal_hot");     //[kWht/K-MWt] The heat capacity of the balance of plant on the hot side
                c_trough.m_mc_bal_cold_per_MW = as_double("mc_bal_cold");   //[kWht/K-MWt] The heat capacity of the balance of plant on the cold side
                c_trough.m_mc_bal_sca = as_double("mc_bal_sca");            //[Wht/K-m] Non-HTF heat capacity associated with each SCA - per meter basis

                c_trough.m_P_ref = q_dot_hs_des * 1e6;                      //[W] Design Turbine Net Output
                c_trough.m_eta_ref = 1;                                     //[] Design cycle thermal efficiency
                c_trough.m_non_solar_field_land_area_multiplier = as_double("non_solar_field_land_area_multiplier");    //[]

                //[m] The collector aperture width (Total structural area.. used for shadowing)
                size_t nval_W_aperture = 0;
                ssc_number_t* W_aperture = as_array("W_aperture", &nval_W_aperture);
                c_trough.m_W_aperture.resize(nval_W_aperture);
                for (size_t i = 0; i < nval_W_aperture; i++)
                    c_trough.m_W_aperture[i] = (double)W_aperture[i];

                //[m^2] Reflective aperture area of the collector
                size_t nval_A_aperture = 0;
                ssc_number_t* A_aperture = as_array("A_aperture", &nval_A_aperture);
                c_trough.m_A_aperture.resize(nval_A_aperture);
                for (size_t i = 0; i < nval_A_aperture; i++)
                    c_trough.m_A_aperture[i] = (double)A_aperture[i];

                //[-] Tracking error derate
                size_t nval_TrackingError = 0;
                ssc_number_t* TrackingError = as_array("TrackingError", &nval_TrackingError);
                c_trough.m_TrackingError.resize(nval_TrackingError);
                for (size_t i = 0; i < nval_TrackingError; i++)
                    c_trough.m_TrackingError[i] = (double)TrackingError[i];

                //[-] Geometry effects derate
                size_t nval_GeomEffects = 0;
                ssc_number_t* GeomEffects = as_array("GeomEffects", &nval_GeomEffects);
                c_trough.m_GeomEffects.resize(nval_GeomEffects);
                for (size_t i = 0; i < nval_GeomEffects; i++)
                    c_trough.m_GeomEffects[i] = (double)GeomEffects[i];

                //[-] Clean mirror reflectivity
                size_t nval_Rho_mirror_clean = 0;
                ssc_number_t* Rho_mirror_clean = as_array("Rho_mirror_clean", &nval_Rho_mirror_clean);
                c_trough.m_Rho_mirror_clean.resize(nval_Rho_mirror_clean);
                for (size_t i = 0; i < nval_Rho_mirror_clean; i++)
                    c_trough.m_Rho_mirror_clean[i] = (double)Rho_mirror_clean[i];

                //[-] Dirt on mirror derate
                size_t nval_Dirt_mirror = 0;
                ssc_number_t* Dirt_mirror = as_array("Dirt_mirror", &nval_Dirt_mirror);
                c_trough.m_Dirt_mirror.resize(nval_Dirt_mirror);
                for (size_t i = 0; i < nval_Dirt_mirror; i++)
                    c_trough.m_Dirt_mirror[i] = (double)Dirt_mirror[i];

                //[-] General optical error derate
                size_t nval_Error = 0;
                ssc_number_t* Error = as_array("Error", &nval_Error);
                c_trough.m_Error.resize(nval_Error);
                for (size_t i = 0; i < nval_Error; i++)
                    c_trough.m_Error[i] = (double)Error[i];

                //[m] The average focal length of the collector 
                size_t nval_Ave_Focal_Length = 0;
                ssc_number_t* Ave_Focal_Length = as_array("Ave_Focal_Length", &nval_Ave_Focal_Length);
                c_trough.m_Ave_Focal_Length.resize(nval_Ave_Focal_Length);
                for (size_t i = 0; i < nval_Ave_Focal_Length; i++)
                    c_trough.m_Ave_Focal_Length[i] = (double)Ave_Focal_Length[i];

                //[m] The length of the SCA 
                size_t nval_L_SCA = 0;
                ssc_number_t* L_SCA = as_array("L_SCA", &nval_L_SCA);
                c_trough.m_L_SCA.resize(nval_L_SCA);
                for (size_t i = 0; i < nval_L_SCA; i++)
                    c_trough.m_L_SCA[i] = (double)L_SCA[i];

                //[m] The length of a single mirror/HCE unit
                size_t nval_L_aperture = 0;
                ssc_number_t* L_aperture = as_array("L_aperture", &nval_L_aperture);
                c_trough.m_L_aperture.resize(nval_L_aperture);
                for (size_t i = 0; i < nval_L_aperture; i++)
                    c_trough.m_L_aperture[i] = (double)L_aperture[i];

                //[-] The number of individual collector sections in an SCA
                size_t nval_ColperSCA = 0;
                ssc_number_t* ColperSCA = as_array("ColperSCA", &nval_ColperSCA);
                c_trough.m_ColperSCA.resize(nval_ColperSCA);
                for (size_t i = 0; i < nval_ColperSCA; i++)
                    c_trough.m_ColperSCA[i] = (double)ColperSCA[i];

                //[m] Piping distance between SCA's in the field
                size_t nval_Distance_SCA = 0;
                ssc_number_t* Distance_SCA = as_array("Distance_SCA", &nval_Distance_SCA);
                c_trough.m_Distance_SCA.resize(nval_Distance_SCA);
                for (size_t i = 0; i < nval_Distance_SCA; i++)
                    c_trough.m_Distance_SCA[i] = (double)Distance_SCA[i];

                c_trough.m_IAM_matrix = as_matrix("IAM_matrix");                //[-] IAM coefficients, matrix for 4 collectors

                // Why are these matrices - can't they be arrays?               
                c_trough.m_HCE_FieldFrac = as_matrix("HCE_FieldFrac");          //[-] Fraction of the field occupied by this HCE type
                c_trough.m_D_2 = as_matrix("D_2");                              //[m] Inner absorber tube diameter
                c_trough.m_D_3 = as_matrix("D_3");                              //[m] Outer absorber tube diameter
                c_trough.m_D_4 = as_matrix("D_4");                              //[m] Inner glass envelope diameter
                c_trough.m_D_5 = as_matrix("D_5");                              //[m] Outer glass envelope diameter
                c_trough.m_D_p = as_matrix("D_p");                              //[m] Diameter of the absorber flow plug (optional)
                c_trough.m_Flow_type = as_matrix("Flow_type");                  //[-] Flow type through the absorber
                c_trough.m_Rough = as_matrix("Rough");                          //[-] Relative roughness of the internal HCE surface
                c_trough.m_alpha_env = as_matrix("alpha_env");                  //[-] Envelope absorptance
                // **********************************************************

                // Emittance vs. temperature profile for each receiver type and variation
                c_trough.m_epsilon_3_11 = as_matrix_transpose("epsilon_3_11");  //[-] Absorber emittance for receiver type 1 variation 1
                c_trough.m_epsilon_3_12 = as_matrix_transpose("epsilon_3_12");  //[-] Absorber emittance for receiver type 1 variation 2
                c_trough.m_epsilon_3_13 = as_matrix_transpose("epsilon_3_13");  //[-] Absorber emittance for receiver type 1 variation 3
                c_trough.m_epsilon_3_14 = as_matrix_transpose("epsilon_3_14");  //[-] Absorber emittance for receiver type 1 variation 4
                c_trough.m_epsilon_3_21 = as_matrix_transpose("epsilon_3_21");  //[-] Absorber emittance for receiver type 2 variation 1
                c_trough.m_epsilon_3_22 = as_matrix_transpose("epsilon_3_22");  //[-] Absorber emittance for receiver type 2 variation 2
                c_trough.m_epsilon_3_23 = as_matrix_transpose("epsilon_3_23");  //[-] Absorber emittance for receiver type 2 variation 3
                c_trough.m_epsilon_3_24 = as_matrix_transpose("epsilon_3_24");  //[-] Absorber emittance for receiver type 2 variation 4
                c_trough.m_epsilon_3_31 = as_matrix_transpose("epsilon_3_31");  //[-] Absorber emittance for receiver type 3 variation 1
                c_trough.m_epsilon_3_32 = as_matrix_transpose("epsilon_3_32");  //[-] Absorber emittance for receiver type 3 variation 2
                c_trough.m_epsilon_3_33 = as_matrix_transpose("epsilon_3_33");  //[-] Absorber emittance for receiver type 3 variation 3
                c_trough.m_epsilon_3_34 = as_matrix_transpose("epsilon_3_34");  //[-] Absorber emittance for receiver type 3 variation 4
                c_trough.m_epsilon_3_41 = as_matrix_transpose("epsilon_3_41");  //[-] Absorber emittance for receiver type 4 variation 1
                c_trough.m_epsilon_3_42 = as_matrix_transpose("epsilon_3_42");  //[-] Absorber emittance for receiver type 4 variation 2
                c_trough.m_epsilon_3_43 = as_matrix_transpose("epsilon_3_43");  //[-] Absorber emittance for receiver type 4 variation 3
                c_trough.m_epsilon_3_44 = as_matrix_transpose("epsilon_3_44");  //[-] Absorber emittance for receiver type 4 variation 4

                c_trough.m_alpha_abs = as_matrix("alpha_abs");                  //[-] Absorber absorptance
                c_trough.m_Tau_envelope = as_matrix("Tau_envelope");            //[-] Envelope transmittance
                c_trough.m_EPSILON_4 = as_matrix("EPSILON_4");                  //[-] Inner glass envelope emissivities
                c_trough.m_EPSILON_5 = as_matrix("EPSILON_5");                  //[-] Outer glass envelope emissivities

                //c_trough.m_GlazingIntact = (as_matrix("GlazingIntactIn") > 0);  //[-] Glazing intact (broken glass) flag {1=true, else=false}
                util::matrix_t<double> glazing_intact_double = as_matrix("GlazingIntactIn"); //[-] Is the glazing intact?
                int n_gl_row = (int)glazing_intact_double.nrows();
                int n_gl_col = (int)glazing_intact_double.ncols();
                c_trough.m_GlazingIntact.resize(n_gl_row, n_gl_col);
                for (int i = 0; i < n_gl_row; i++)
                {
                    for (int j = 0; j < n_gl_col; j++)
                    {
                        c_trough.m_GlazingIntact(i, j) = (glazing_intact_double(i, j) > 0);
                    }
                }

                c_trough.m_P_a = as_matrix("P_a");                              //[torr] Annulus gas pressure
                c_trough.m_AnnulusGas = as_matrix("AnnulusGas");                //[-] Annulus gas type (1=air, 26=Ar, 27=H2)
                c_trough.m_AbsorberMaterial = as_matrix("AbsorberMaterial");    //[-] Absorber material type
                c_trough.m_Shadowing = as_matrix("Shadowing");                  //[-] Receiver bellows shadowing loss factor
                c_trough.m_Dirt_HCE = as_matrix("Dirt_HCE");                    //[-] Loss due to dirt on the receiver envelope
                c_trough.m_Design_loss = as_matrix("Design_loss");              //[-] Receiver heat loss at design
                c_trough.m_rec_su_delay = as_double("rec_su_delay");            //[hr] Fixed startup delay time for the receiver
                c_trough.m_rec_qf_delay = as_double("rec_qf_delay");            //[-] Energy-based receiver startup delay (fraction of rated thermal power)
                c_trough.m_p_start = as_double("p_start");                      //[kWhe] Collector startup energy, per SCA
                c_trough.m_calc_design_pipe_vals = as_boolean("calc_design_pipe_vals"); //[-] Should the HTF state be calculated at design conditions
                c_trough.m_L_rnr_pb = as_double("L_rnr_pb");                      //[m] Length of hot or cold runner pipe around the power block
                c_trough.m_N_max_hdr_diams = as_double("N_max_hdr_diams");        //[-] Maximum number of allowed diameters in each of the hot and cold headers
                c_trough.m_L_rnr_per_xpan = as_double("L_rnr_per_xpan");          //[m] Threshold length of straight runner pipe without an expansion loop
                c_trough.m_L_xpan_hdr = as_double("L_xpan_hdr");                  //[m] Combined length in meters of the two perpendicular segments of a header expansion loop
                c_trough.m_L_xpan_rnr = as_double("L_xpan_rnr");                  //[m] Combined length in meters of the two perpendicular segments of a runner expansion loop
                c_trough.m_Min_rnr_xpans = as_double("Min_rnr_xpans");            //[-] Minimum number of expansion loops per single-diameter runner section
                c_trough.m_northsouth_field_sep = as_double("northsouth_field_sep"); //[m] Shortest north/south distance between SCAs in different subfields
                c_trough.m_N_hdr_per_xpan = as_double("N_hdr_per_xpan");          //[-] Number of collector loops per header expansion loops. 1 = expansion loop between every collector loop
                c_trough.m_offset_xpan_hdr = as_double("offset_xpan_hdr");        //[-] Location of first header expansion loop. 1 = after first collector loop
                //c_trough.m_K_cpnt = as_matrix("K_cpnt");                          //[-] Minor loss coefficients of the components in each loop interconnect
                //c_trough.m_D_cpnt = as_matrix("D_cpnt");                          //[m] Inner diameters of the components in each loop interconnect
                //c_trough.m_L_cpnt = as_matrix("L_cpnt");                          //[m] Lengths of the components in each loop interconnect
                //c_trough.m_Type_cpnt = as_matrix("Type_cpnt");                    //[-] Type of component in each loop interconnect [0=fitting | 1=pipe | 2=flex_hose]
                c_trough.m_custom_sf_pipe_sizes = as_boolean("custom_sf_pipe_sizes"); //[-] Should the field pipe diameters, wall thickness and lengths be imported instead of calculated
                c_trough.m_sf_rnr_diams = as_matrix("sf_rnr_diams");              //[m] Imported runner diameters, used if custom_sf_pipe_sizes is true
                c_trough.m_sf_rnr_wallthicks = as_matrix("sf_rnr_wallthicks");    //[m] Imported runner wall thicknesses, used if custom_sf_pipe_sizes is true
                c_trough.m_sf_rnr_lengths = as_matrix("sf_rnr_lengths");          //[m] Imported runner lengths, used if custom_sf_pipe_sizes is true
                c_trough.m_sf_hdr_diams = as_matrix("sf_hdr_diams");              //[m] Imported header diameters, used if custom_sf_pipe_sizes is true
                c_trough.m_sf_hdr_wallthicks = as_matrix("sf_hdr_wallthicks");    //[m] Imported header wall thicknesses, used if custom_sf_pipe_sizes is true
                c_trough.m_sf_hdr_lengths = as_matrix("sf_hdr_lengths");          //[m] Imported header lengths, used if custom_sf_pipe_sizes is true

            }

            // Calculate solar multiple (needed for other component constructors)
            bool success = c_trough.design_solar_mult(as_vector_double("trough_loop_control"));
            if (success == false)
                throw exec_error("trough_physical_iph", "Negative solar mult or total aperture.");

            q_dot_rec_des_ideal_pre_init = c_trough.m_q_design_ideal / 1e6; // [MWt]


            // Allocate trough outputs
            {
                c_trough.mc_reported_outputs.assign(C_csp_trough_collector_receiver::E_THETA_AVE, allocate("Theta_ave", n_steps_fixed), n_steps_fixed);
                c_trough.mc_reported_outputs.assign(C_csp_trough_collector_receiver::E_COSTH_AVE, allocate("CosTh_ave", n_steps_fixed), n_steps_fixed);
                c_trough.mc_reported_outputs.assign(C_csp_trough_collector_receiver::E_IAM_AVE, allocate("IAM_ave", n_steps_fixed), n_steps_fixed);
                c_trough.mc_reported_outputs.assign(C_csp_trough_collector_receiver::E_ROWSHADOW_AVE, allocate("RowShadow_ave", n_steps_fixed), n_steps_fixed);
                c_trough.mc_reported_outputs.assign(C_csp_trough_collector_receiver::E_ENDLOSS_AVE, allocate("EndLoss_ave", n_steps_fixed), n_steps_fixed);
                c_trough.mc_reported_outputs.assign(C_csp_trough_collector_receiver::E_DNI_COSTH, allocate("dni_costh", n_steps_fixed), n_steps_fixed);
                c_trough.mc_reported_outputs.assign(C_csp_trough_collector_receiver::E_EQUIV_OPT_ETA_TOT, allocate("EqOpteff", n_steps_fixed), n_steps_fixed);
                c_trough.mc_reported_outputs.assign(C_csp_trough_collector_receiver::E_DEFOCUS, allocate("SCAs_def", n_steps_fixed), n_steps_fixed);

                c_trough.mc_reported_outputs.assign(C_csp_trough_collector_receiver::E_Q_DOT_INC_SF_TOT, allocate("q_inc_sf_tot", n_steps_fixed), n_steps_fixed);
                c_trough.mc_reported_outputs.assign(C_csp_trough_collector_receiver::E_Q_DOT_INC_SF_COSTH, allocate("qinc_costh", n_steps_fixed), n_steps_fixed);
                c_trough.mc_reported_outputs.assign(C_csp_trough_collector_receiver::E_Q_DOT_REC_INC, allocate("q_dot_rec_inc", n_steps_fixed), n_steps_fixed);
                c_trough.mc_reported_outputs.assign(C_csp_trough_collector_receiver::E_Q_DOT_REC_THERMAL_LOSS, allocate("q_dot_rec_thermal_loss", n_steps_fixed), n_steps_fixed);
                c_trough.mc_reported_outputs.assign(C_csp_trough_collector_receiver::E_Q_DOT_REC_ABS, allocate("q_dot_rec_abs", n_steps_fixed), n_steps_fixed);
                c_trough.mc_reported_outputs.assign(C_csp_trough_collector_receiver::E_Q_DOT_PIPING_LOSS, allocate("q_dot_piping_loss", n_steps_fixed), n_steps_fixed);
                c_trough.mc_reported_outputs.assign(C_csp_trough_collector_receiver::E_E_DOT_INTERNAL_ENERGY, allocate("e_dot_field_int_energy", n_steps_fixed), n_steps_fixed);
                c_trough.mc_reported_outputs.assign(C_csp_trough_collector_receiver::E_Q_DOT_HTF_OUT, allocate("q_dot_htf_sf_out", n_steps_fixed), n_steps_fixed);
                c_trough.mc_reported_outputs.assign(C_csp_trough_collector_receiver::E_Q_DOT_FREEZE_PROT, allocate("q_dot_freeze_prot", n_steps_fixed), n_steps_fixed);

                c_trough.mc_reported_outputs.assign(C_csp_trough_collector_receiver::E_TIME_IN_STARTUP, allocate("rec_time_in_startup", n_steps_fixed), n_steps_fixed);

                c_trough.mc_reported_outputs.assign(C_csp_trough_collector_receiver::E_M_DOT_LOOP, allocate("m_dot_loop", n_steps_fixed), n_steps_fixed);
                c_trough.mc_reported_outputs.assign(C_csp_trough_collector_receiver::E_IS_RECIRCULATING, allocate("recirculating", n_steps_fixed), n_steps_fixed);
                c_trough.mc_reported_outputs.assign(C_csp_trough_collector_receiver::E_M_DOT_FIELD_RECIRC, allocate("m_dot_field_recirc", n_steps_fixed), n_steps_fixed);
                c_trough.mc_reported_outputs.assign(C_csp_trough_collector_receiver::E_M_DOT_FIELD_DELIVERED, allocate("m_dot_field_delivered", n_steps_fixed), n_steps_fixed);
                c_trough.mc_reported_outputs.assign(C_csp_trough_collector_receiver::E_T_FIELD_COLD_IN, allocate("T_field_cold_in", n_steps_fixed), n_steps_fixed);
                c_trough.mc_reported_outputs.assign(C_csp_trough_collector_receiver::E_T_REC_COLD_IN, allocate("T_rec_cold_in", n_steps_fixed), n_steps_fixed);
                c_trough.mc_reported_outputs.assign(C_csp_trough_collector_receiver::E_T_REC_HOT_OUT, allocate("T_rec_hot_out", n_steps_fixed), n_steps_fixed);
                c_trough.mc_reported_outputs.assign(C_csp_trough_collector_receiver::E_T_FIELD_HOT_OUT, allocate("T_field_hot_out", n_steps_fixed), n_steps_fixed);
                c_trough.mc_reported_outputs.assign(C_csp_trough_collector_receiver::E_PRESSURE_DROP, allocate("deltaP_field", n_steps_fixed), n_steps_fixed);          //[bar]

                c_trough.mc_reported_outputs.assign(C_csp_trough_collector_receiver::E_W_DOT_SCA_TRACK, allocate("W_dot_sca_track", n_steps_fixed), n_steps_fixed);     //[MWe]
                c_trough.mc_reported_outputs.assign(C_csp_trough_collector_receiver::E_W_DOT_PUMP, allocate("W_dot_field_pump", n_steps_fixed), n_steps_fixed);         //[MWe]

                c_trough.mc_reported_outputs.assign(C_csp_trough_collector_receiver::E_VEL_LOOP_MIN, allocate("vel_loop_min", n_steps_fixed), n_steps_fixed);               //[C]
                c_trough.mc_reported_outputs.assign(C_csp_trough_collector_receiver::E_VEL_LOOP_MAX, allocate("vel_loop_max", n_steps_fixed), n_steps_fixed);               //[C]

            }
            
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
                    throw exec_error("trough_physical", "When the IPH physical trough case has an electric HTF charger, dispatch optimization must be selected");
                }
            }

            double heater_mult = as_double("heater_mult");      //[-]
            heater_spec_cost = as_double("heater_spec_cost");   //[$/kWt]

            q_dot_heater_des = q_dot_hs_des * heater_mult;     //[MWt]

            double heater_efficiency = as_double("heater_efficiency") / 100.0;          //[-] convert from % input
            double f_q_dot_des_allowable_su = as_double("f_q_dot_des_allowable_su");    //[-] fraction of design power allowed during startup
            double hrs_startup_at_max_rate = as_double("hrs_startup_at_max_rate");      //[hr] duration of startup at max startup power
            double f_heater_min = as_double("f_q_dot_heater_min");                      //[-] minimum allowable heater output as fraction of design

            
        
       
            p_electric_resistance = new C_csp_cr_electric_resistance(T_htf_cold_des, T_htf_hot_des,
                q_dot_heater_des, heater_efficiency, f_heater_min,
                f_q_dot_des_allowable_su, hrs_startup_at_max_rate,
                as_integer("Fluid"), as_matrix("field_fl_props"), C_csp_cr_electric_resistance::E_elec_resist_startup_mode::INSTANTANEOUS_NO_MAX_ELEC_IN);

            p_electric_resistance->mc_reported_outputs.assign(C_csp_cr_electric_resistance::E_W_DOT_HEATER, allocate("W_dot_heater", n_steps_fixed), n_steps_fixed);
            p_electric_resistance->mc_reported_outputs.assign(C_csp_cr_electric_resistance::E_Q_DOT_HTF, allocate("q_dot_heater_to_htf", n_steps_fixed), n_steps_fixed);
            p_electric_resistance->mc_reported_outputs.assign(C_csp_cr_electric_resistance::E_Q_DOT_STARTUP, allocate("q_dot_heater_startup", n_steps_fixed), n_steps_fixed);
            p_electric_resistance->mc_reported_outputs.assign(C_csp_cr_electric_resistance::E_M_DOT_HTF, allocate("m_dot_htf_heater", n_steps_fixed), n_steps_fixed);
            p_electric_resistance->mc_reported_outputs.assign(C_csp_cr_electric_resistance::E_T_HTF_IN, allocate("T_htf_heater_in", n_steps_fixed), n_steps_fixed);
            p_electric_resistance->mc_reported_outputs.assign(C_csp_cr_electric_resistance::E_T_HTF_OUT, allocate("T_htf_heater_out", n_steps_fixed), n_steps_fixed);
        }
        p_heater = p_electric_resistance;

        // ********************************
        // ********************************
        // TES
        // ********************************
        // ********************************
        C_csp_tes* storage_pointer;
        C_csp_two_tank_tes storage_two_tank;
        C_csp_piston_cylinder_tes storage_cyl;
        C_csp_packedbed_tes storage_packedbed;
        // Two Tank
        if (tes_type == C_csp_tes::csp_tes_types::E_TES_TWO_TANK)
        {

            bool custom_tes_pipe_sizes = as_boolean("custom_tes_pipe_sizes");
            util::matrix_t<double> tes_lengths;
            if (is_assigned("tes_lengths")) {
                tes_lengths = as_matrix("tes_lengths");               //[m]
            }
            if (!is_assigned("tes_lengths") || tes_lengths.ncells() < 11) {
                double vals1[11] = { 0., 90., 100., 120., 0., 30., 90., 80., 80., 120., 80. };
                tes_lengths.assign(vals1, 11);
            }
            util::matrix_t<double> tes_wallthicks;
            if (!is_assigned("tes_wallthicks"))
            {
                double tes_wallthicks_val[1] = { -1 };
                tes_wallthicks.assign(tes_wallthicks_val, 1);
            }
            util::matrix_t<double> tes_diams;
            if (!is_assigned("tes_diams"))
            {
                double tes_diams_val[1] = { -1 };
                tes_diams.assign(tes_diams_val, 1);
            }

            double h_tank_in = is_assigned("h_tank_in") == true ? as_double("h_tank_in") : std::numeric_limits<double>::quiet_NaN();
            double d_tank_in = is_assigned("d_tank_in") == true ? as_double("d_tank_in") : std::numeric_limits<double>::quiet_NaN();

            storage_two_tank = C_csp_two_tank_tes(
                c_trough.m_Fluid,
                c_trough.m_field_fl_props,
                as_integer("store_fluid"),
                as_matrix("store_fl_props"),
                q_dot_hs_des,
                c_trough.m_solar_mult,
                Q_tes,
                as_boolean("is_h_tank_fixed"),
                h_tank_in,
                d_tank_in,
                as_double("u_tank"),
                as_integer("tank_pairs"),
                as_double("hot_tank_Thtr"),
                as_double("hot_tank_max_heat"),
                as_double("cold_tank_Thtr"),
                as_double("cold_tank_max_heat"),
                as_double("dt_hot"),
                as_double("T_loop_in_des"),
                as_double("T_loop_out"),
                // Check initialization variables
                (is_assigned("T_tank_hot_init")) ? as_double("T_tank_hot_init") : as_double("T_loop_out"),
                (is_assigned("T_tank_cold_init")) ? as_double("T_tank_cold_init") : as_double("T_loop_in_des"),
                as_double("h_tank_min"),
                as_double("init_hot_htf_percent"),
                as_double("pb_pump_coef"),
                as_boolean("tanks_in_parallel"),
                as_double("V_tes_des"),
                as_boolean("calc_design_pipe_vals"),
                as_double("tes_pump_coef"),
                as_double("eta_pump"),
                as_boolean("has_hot_tank_bypass"),
                as_double("T_tank_hot_inlet_min"),
                as_boolean("custom_tes_p_loss"),
                as_boolean("custom_tes_pipe_sizes"),
                as_matrix("k_tes_loss_coeffs"),
                tes_diams,
                tes_wallthicks,
                tes_lengths,
                as_double("HDR_rough")
            );

            storage_pointer = &storage_two_tank;

            // Set storage outputs
            storage_two_tank.mc_reported_outputs.assign(C_csp_two_tank_tes::E_Q_DOT_LOSS, allocate("tank_losses", n_steps_fixed), n_steps_fixed);
            storage_two_tank.mc_reported_outputs.assign(C_csp_two_tank_tes::E_W_DOT_HEATER, allocate("q_tes_heater", n_steps_fixed), n_steps_fixed);
            storage_two_tank.mc_reported_outputs.assign(C_csp_two_tank_tes::E_TES_T_HOT, allocate("T_tes_hot", n_steps_fixed), n_steps_fixed);
            storage_two_tank.mc_reported_outputs.assign(C_csp_two_tank_tes::E_TES_T_COLD, allocate("T_tes_cold", n_steps_fixed), n_steps_fixed);
            storage_two_tank.mc_reported_outputs.assign(C_csp_two_tank_tes::E_M_DOT_TANK_TO_TANK, allocate("m_dot_cold_tank_to_hot_tank", n_steps_fixed), n_steps_fixed);
            storage_two_tank.mc_reported_outputs.assign(C_csp_two_tank_tes::E_MASS_COLD_TANK, allocate("mass_tes_cold", n_steps_fixed), n_steps_fixed);
            storage_two_tank.mc_reported_outputs.assign(C_csp_two_tank_tes::E_MASS_HOT_TANK, allocate("mass_tes_hot", n_steps_fixed), n_steps_fixed);
            storage_two_tank.mc_reported_outputs.assign(C_csp_two_tank_tes::E_W_DOT_HTF_PUMP, allocate("tes_htf_pump_power", n_steps_fixed), n_steps_fixed);
            storage_two_tank.mc_reported_outputs.assign(C_csp_two_tank_tes::E_VOL_TOT, allocate("vol_tes_tot", n_steps_fixed), n_steps_fixed);
            storage_two_tank.mc_reported_outputs.assign(C_csp_two_tank_tes::E_MASS_TOT, allocate("tes_mass_tot", n_steps_fixed), n_steps_fixed);
            storage_two_tank.mc_reported_outputs.assign(C_csp_two_tank_tes::E_HOT_TANK_HTF_PERC_FINAL, allocate("hot_tank_htf_percent_final", n_steps_fixed), n_steps_fixed);
        }
        // Packed Bed
        else if (tes_type == C_csp_tes::csp_tes_types::E_TES_PACKED_BED)
        {
            storage_packedbed = C_csp_packedbed_tes(
                as_integer("Fluid"),                                                // [-] field fluid identifier
                as_matrix("field_fl_props"),                                        // [-] field fluid properties
                q_dot_hs_des,                                                       // [MWt] Design heat rate in and out of tes
                Q_tes,                                                              // [MWht] design storage capacity
                as_integer("is_h_tank_fixed"),                                      // [] Sizing Method (0) use fixed diameter, (1) use fixed height, (2) use preset inputs
                as_double("h_tank_in"),                                             // [m] Tank height
                as_double("d_tank_in"),                                             // [m] Tank diameter
                as_double("tes_pb_f_oversize"),                                     // [] Oversize factor
                as_double("T_loop_in_des"),                                         // [C] Cold design temperature
                as_double("T_loop_out"),                                            // [C] hot design temperature
                // Check initialization variables
                (is_assigned("T_tank_hot_init")) ? as_double("T_tank_hot_init") : as_double("T_loop_out"),
                (is_assigned("T_tank_cold_init")) ? as_double("T_tank_cold_init") : as_double("T_loop_in_des"),
                as_double("init_hot_htf_percent"),                                  // [%] Initial fraction of available volume that is hot
                as_integer("tes_pb_n_xsteps"),                                      // number spatial sub steps
                as_integer("tes_n_tsteps"),                                         // number subtimesteps
                as_double("tes_pump_coef"),                                         // [kW/kg/s] Pumping power to move 1 kg/s of HTF through tes loop 
                as_double("tes_pb_k_eff"),                                          // [W/m K] Effective thermal conductivity
                as_double("tes_pb_void_frac"),                                      // [] Packed bed void fraction
                as_double("tes_pb_dens_solid"),                                     // [kg/m3] solid specific heat 
                as_double("tes_pb_cp_solid"),                                       // [kJ/kg K] solid specific heat
                as_double("tes_pb_T_hot_delta"),                                    // [C] Max allowable decrease in hot discharge temp
                as_double("tes_pb_T_cold_delta"),                                   // [C] Max allowable increase in cold discharge temp
                as_double("tes_pb_T_charge_min")                                    // [C] Min allowable charge temperature
            );

            storage_pointer = &storage_packedbed;

            // Set storage outputs
            storage_packedbed.mc_reported_outputs.assign(C_csp_packedbed_tes::E_Q_DOT_LOSS, allocate("tank_losses", n_steps_fixed), n_steps_fixed);
            storage_packedbed.mc_reported_outputs.assign(C_csp_packedbed_tes::E_W_DOT_HEATER, allocate("q_tes_heater", n_steps_fixed), n_steps_fixed);
            storage_packedbed.mc_reported_outputs.assign(C_csp_packedbed_tes::E_TES_T_HOT, allocate("T_tes_hot", n_steps_fixed), n_steps_fixed);
            storage_packedbed.mc_reported_outputs.assign(C_csp_packedbed_tes::E_TES_T_COLD, allocate("T_tes_cold", n_steps_fixed), n_steps_fixed);
            storage_packedbed.mc_reported_outputs.assign(C_csp_packedbed_tes::E_M_DOT_TANK_TO_TANK, allocate("m_dot_cold_tank_to_hot_tank", n_steps_fixed), n_steps_fixed);
            storage_packedbed.mc_reported_outputs.assign(C_csp_packedbed_tes::E_MASS_COLD_TANK, allocate("mass_tes_cold", n_steps_fixed), n_steps_fixed);
            storage_packedbed.mc_reported_outputs.assign(C_csp_packedbed_tes::E_MASS_HOT_TANK, allocate("mass_tes_hot", n_steps_fixed), n_steps_fixed);
            storage_packedbed.mc_reported_outputs.assign(C_csp_packedbed_tes::E_W_DOT_HTF_PUMP, allocate("tes_htf_pump_power", n_steps_fixed), n_steps_fixed);
            storage_packedbed.mc_reported_outputs.assign(C_csp_packedbed_tes::E_VOL_TOT, allocate("vol_tes_tot", n_steps_fixed), n_steps_fixed);
            storage_packedbed.mc_reported_outputs.assign(C_csp_packedbed_tes::E_MASS_TOT, allocate("tes_mass_tot", n_steps_fixed), n_steps_fixed);
            storage_packedbed.mc_reported_outputs.assign(C_csp_packedbed_tes::E_T_GRAD_0, allocate("T_grad_0", n_steps_fixed), n_steps_fixed);
            storage_packedbed.mc_reported_outputs.assign(C_csp_packedbed_tes::E_T_GRAD_1, allocate("T_grad_1", n_steps_fixed), n_steps_fixed);
            storage_packedbed.mc_reported_outputs.assign(C_csp_packedbed_tes::E_T_GRAD_2, allocate("T_grad_2", n_steps_fixed), n_steps_fixed);
            storage_packedbed.mc_reported_outputs.assign(C_csp_packedbed_tes::E_T_GRAD_3, allocate("T_grad_3", n_steps_fixed), n_steps_fixed);
            storage_packedbed.mc_reported_outputs.assign(C_csp_packedbed_tes::E_T_GRAD_4, allocate("T_grad_4", n_steps_fixed), n_steps_fixed);
            storage_packedbed.mc_reported_outputs.assign(C_csp_packedbed_tes::E_T_GRAD_5, allocate("T_grad_5", n_steps_fixed), n_steps_fixed);
            storage_packedbed.mc_reported_outputs.assign(C_csp_packedbed_tes::E_T_GRAD_6, allocate("T_grad_6", n_steps_fixed), n_steps_fixed);
            storage_packedbed.mc_reported_outputs.assign(C_csp_packedbed_tes::E_T_GRAD_7, allocate("T_grad_7", n_steps_fixed), n_steps_fixed);
            storage_packedbed.mc_reported_outputs.assign(C_csp_packedbed_tes::E_T_GRAD_8, allocate("T_grad_8", n_steps_fixed), n_steps_fixed);
            storage_packedbed.mc_reported_outputs.assign(C_csp_packedbed_tes::E_T_GRAD_9, allocate("T_grad_9", n_steps_fixed), n_steps_fixed);
        }
        // Piston Cylinder
        else if (tes_type == C_csp_tes::csp_tes_types::E_TES_CYL)
        {
            // Get number of sub time steps
            int nstep = as_integer("tes_n_tsteps");

            bool custom_tes_pipe_sizes = as_boolean("custom_tes_pipe_sizes");
            util::matrix_t<double> tes_lengths;
            if (is_assigned("tes_lengths")) {
                tes_lengths = as_matrix("tes_lengths");               //[m]
            }
            if (!is_assigned("tes_lengths") || tes_lengths.ncells() < 11) {
                double vals1[11] = { 0., 90., 100., 120., 0., 30., 90., 80., 80., 120., 80. };
                tes_lengths.assign(vals1, 11);
            }
            util::matrix_t<double> tes_wallthicks;
            if (!is_assigned("tes_wallthicks"))
            {
                double tes_wallthicks_val[1] = { -1 };
                tes_wallthicks.assign(tes_wallthicks_val, 1);
            }
            util::matrix_t<double> tes_diams;
            if (!is_assigned("tes_diams"))
            {
                double tes_diams_val[1] = { -1 };
                tes_diams.assign(tes_diams_val, 1);
            }

            // Modify wall density to account for insulation mass
            double mass_factor = 1.0 + (0.01 * as_double("tes_cyl_tank_insul_percent"));
            double dens_orig = as_double("tes_cyl_tank_dens");
            double dens_w_insulation = dens_orig * mass_factor;

            double h_tank_in = is_assigned("h_tank_in") == true ? as_double("h_tank_in") : std::numeric_limits<double>::quiet_NaN();
            double d_tank_in = is_assigned("d_tank_in") == true ? as_double("d_tank_in") : std::numeric_limits<double>::quiet_NaN();

            storage_cyl = C_csp_piston_cylinder_tes(
                c_trough.m_Fluid,
                c_trough.m_field_fl_props,
                //as_integer("store_fluid"),
                //as_matrix("store_fl_props"),
                q_dot_hs_des,
                c_trough.m_solar_mult,
                Q_tes,
                as_boolean("is_h_tank_fixed"),
                h_tank_in,
                d_tank_in,
                as_double("u_tank"),
                as_integer("tank_pairs"),
                as_double("hot_tank_Thtr"),
                as_double("hot_tank_max_heat"),
                as_double("cold_tank_Thtr"),
                as_double("cold_tank_max_heat"),
                as_double("T_loop_in_des"),
                as_double("T_loop_out"),
                // Check initialization variables
                (is_assigned("T_tank_hot_init")) ? as_double("T_tank_hot_init") : as_double("T_loop_out"),
                (is_assigned("T_tank_cold_init")) ? as_double("T_tank_cold_init") : as_double("T_loop_in_des"),
                as_double("h_tank_min"),
                as_double("init_hot_htf_percent"),
                as_double("pb_pump_coef"),
                as_double("tes_cyl_tank_cp") * 1000, // convert to J/kgK
                dens_w_insulation,
                as_double("tes_cyl_tank_thick"),
                nstep,
                as_vector_double("tes_cyl_piston_loss_poly"),
                as_double("V_tes_des"),
                as_boolean("calc_design_pipe_vals"),
                as_double("tes_pump_coef"),
                as_double("eta_pump"),
                as_boolean("has_hot_tank_bypass"),
                as_double("T_tank_hot_inlet_min"),
                false,
                false,
                as_matrix("k_tes_loss_coeffs"),
                tes_diams,
                tes_wallthicks,
                tes_lengths,
                as_double("HDR_rough")
            );

            storage_pointer = &storage_cyl;

            // Set storage outputs
            storage_cyl.mc_reported_outputs.assign(C_csp_piston_cylinder_tes::E_Q_DOT_LOSS, allocate("tank_losses", n_steps_fixed), n_steps_fixed);
            storage_cyl.mc_reported_outputs.assign(C_csp_piston_cylinder_tes::E_W_DOT_HEATER, allocate("q_tes_heater", n_steps_fixed), n_steps_fixed);
            storage_cyl.mc_reported_outputs.assign(C_csp_piston_cylinder_tes::E_TES_T_HOT, allocate("T_tes_hot", n_steps_fixed), n_steps_fixed);
            storage_cyl.mc_reported_outputs.assign(C_csp_piston_cylinder_tes::E_TES_T_COLD, allocate("T_tes_cold", n_steps_fixed), n_steps_fixed);
            storage_cyl.mc_reported_outputs.assign(C_csp_piston_cylinder_tes::E_M_DOT_TANK_TO_TANK, allocate("m_dot_cold_tank_to_hot_tank", n_steps_fixed), n_steps_fixed);
            storage_cyl.mc_reported_outputs.assign(C_csp_piston_cylinder_tes::E_MASS_COLD_TANK, allocate("mass_tes_cold", n_steps_fixed), n_steps_fixed);
            storage_cyl.mc_reported_outputs.assign(C_csp_piston_cylinder_tes::E_MASS_HOT_TANK, allocate("mass_tes_hot", n_steps_fixed), n_steps_fixed);
            storage_cyl.mc_reported_outputs.assign(C_csp_piston_cylinder_tes::E_W_DOT_HTF_PUMP, allocate("tes_htf_pump_power", n_steps_fixed), n_steps_fixed);
            storage_cyl.mc_reported_outputs.assign(C_csp_piston_cylinder_tes::E_HOT_TANK_HTF_PERC_FINAL, allocate("hot_tank_htf_percent_final", n_steps_fixed), n_steps_fixed);

            storage_cyl.mc_reported_outputs.assign(C_csp_piston_cylinder_tes::E_VOL_COLD, allocate("vol_tes_cold", n_steps_fixed), n_steps_fixed);
            storage_cyl.mc_reported_outputs.assign(C_csp_piston_cylinder_tes::E_VOL_HOT, allocate("vol_tes_hot", n_steps_fixed), n_steps_fixed);
            storage_cyl.mc_reported_outputs.assign(C_csp_piston_cylinder_tes::E_VOL_TOT, allocate("vol_tes_tot", n_steps_fixed), n_steps_fixed);
            storage_cyl.mc_reported_outputs.assign(C_csp_piston_cylinder_tes::E_PIST_LOC, allocate("tes_piston_loc", n_steps_fixed), n_steps_fixed);
            storage_cyl.mc_reported_outputs.assign(C_csp_piston_cylinder_tes::E_PIST_FRAC, allocate("tes_piston_frac", n_steps_fixed), n_steps_fixed);
            storage_cyl.mc_reported_outputs.assign(C_csp_piston_cylinder_tes::E_COLD_FRAC, allocate("tes_cold_vol_frac", n_steps_fixed), n_steps_fixed);
            storage_cyl.mc_reported_outputs.assign(C_csp_piston_cylinder_tes::E_MASS_TOT, allocate("tes_mass_tot", n_steps_fixed), n_steps_fixed);
            storage_cyl.mc_reported_outputs.assign(C_csp_piston_cylinder_tes::E_SA_COLD, allocate("tes_SA_cold", n_steps_fixed), n_steps_fixed);
            storage_cyl.mc_reported_outputs.assign(C_csp_piston_cylinder_tes::E_SA_HOT, allocate("tes_SA_hot", n_steps_fixed), n_steps_fixed);
            storage_cyl.mc_reported_outputs.assign(C_csp_piston_cylinder_tes::E_SA_TOT, allocate("tes_SA_tot", n_steps_fixed), n_steps_fixed);
            storage_cyl.mc_reported_outputs.assign(C_csp_piston_cylinder_tes::E_ERROR, allocate("tes_error", n_steps_fixed), n_steps_fixed);
            storage_cyl.mc_reported_outputs.assign(C_csp_piston_cylinder_tes::E_ERROR_PERCENT, allocate("tes_error_percent", n_steps_fixed), n_steps_fixed);
            storage_cyl.mc_reported_outputs.assign(C_csp_piston_cylinder_tes::E_LEAK_ERROR, allocate("tes_leak_error", n_steps_fixed), n_steps_fixed);
            storage_cyl.mc_reported_outputs.assign(C_csp_piston_cylinder_tes::E_WALL_ERROR, allocate("tes_wall_error", n_steps_fixed), n_steps_fixed);
            storage_cyl.mc_reported_outputs.assign(C_csp_piston_cylinder_tes::E_ERROR_CORRECTED, allocate("tes_error_corrected", n_steps_fixed), n_steps_fixed);
        }
        else
        {
            throw exec_error("trough_physical", "tes_type must be 1-3");
        }



        // ********************************
        // ********************************
        // Schedules
        // ********************************
        // ********************************

        // Off-taker schedule
        C_timeseries_schedule_inputs offtaker_schedule;
        int is_timestep_load_fractions = as_integer("is_timestep_load_fractions");
        std::vector<double> timestep_load_fractions_calc;
        std::vector<double> timestep_load_abs_calc;

        // Block schedules
        if(is_timestep_load_fractions == 0) {      // Block schedules
            C_timeseries_schedule_inputs offtaker_block = C_timeseries_schedule_inputs(as_matrix("weekday_schedule"),
                as_matrix("weekend_schedule"), as_vector_double("f_turb_tou_periods"), std::numeric_limits<double>::quiet_NaN());
            offtaker_schedule = offtaker_block;
        }
        // Hourly load fractions
        else if (is_timestep_load_fractions == 1) {
            auto vec = as_vector_double("timestep_load_fractions");
            C_timeseries_schedule_inputs offtaker_series = C_timeseries_schedule_inputs(vec, std::numeric_limits<double>::quiet_NaN());
            offtaker_schedule = offtaker_series;
        }
        // Absolute load values
        else if (is_timestep_load_fractions == 2) {
            std::vector<double> vec_abs = as_vector_double("timestep_load_abs");    //[kWt]
            double scale_factor = as_double("timestep_load_abs_factor");
            std::vector<double> vec_abs_scaled;
            for (double abs_val : vec_abs)
                vec_abs_scaled.push_back(abs_val * scale_factor);                   //[kWt]
            std::vector<double> vec_norm;
            double q_pb_design_kW = q_dot_hs_des * 1.e3;                //[kWt]
            for (double abs_val_scaled : vec_abs_scaled)
                vec_norm.push_back(abs_val_scaled / q_pb_design_kW);
            C_timeseries_schedule_inputs offtaker_series = C_timeseries_schedule_inputs(vec_norm, std::numeric_limits<double>::quiet_NaN());
            offtaker_schedule = offtaker_series;
        }
        else
        {
            throw exec_error("trough_physical_iph", "Variable is_timestep_load_fractions must be 0-2");
        }

        // Heat Sink
        int hs_type = as_integer("hs_type");
        C_csp_power_cycle* c_heat_sink_pointer = nullptr;
        C_pc_heat_sink c_heat_sink;
        C_pc_heat_sink_physical c_heat_sink_phys;

            // Ideal heat sink
        if (hs_type == 0)
        {
            //size_t n_f_turbine1 = 0;
            //ssc_number_t* p_f_turbine1 = as_array("f_turb_tou_periods", &n_f_turbine1);   // heat sink, not turbine
            //double f_turbine_max1 = 1.0;
            //for (size_t i = 0; i < n_f_turbine1; i++) {
            //    f_turbine_max1 = max(f_turbine_max1, p_f_turbine1[i]);
            //}
            //double f_turbine_max1 = 1.0;
            //for (S_timeseries_schedule_data data : offtaker_schedule.mv_timeseries_schedule_data)
            //    f_turbine_max1 = max(f_turbine_max1, data.nondim_value);


            c_heat_sink.ms_params.m_T_htf_hot_des = T_htf_hot_des;		//[C] FIELD design outlet temperature
            c_heat_sink.ms_params.m_T_htf_cold_des = T_htf_cold_des;	//[C] FIELD design inlet temperature
            c_heat_sink.ms_params.m_q_dot_des = q_dot_hs_des;			//[MWt] HEAT SINK design thermal power (could have field solar multiple...)
            // 9.18.2016 twn: assume for now there's no pressure drop though heat sink
            c_heat_sink.ms_params.m_htf_pump_coef = as_double("pb_pump_coef");		//[kWe/kg/s]
            c_heat_sink.ms_params.m_max_frac = as_double("hs_htf_mdot_max_frac");  // f_turbine_max1;

            c_heat_sink.ms_params.m_pc_fl = as_integer("Fluid");
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
            c_heat_sink_phys.ms_params.m_q_dot_des = q_dot_hs_des;			//[MWt] HEAT SINK design thermal power (could have field solar multiple...)
            // 9.18.2016 twn: assume for now there's no pressure drop though heat sink
            c_heat_sink_phys.ms_params.m_htf_pump_coef = as_double("pb_pump_coef");		//[kWe/kg/s]


            c_heat_sink_phys.ms_params.m_max_frac = as_double("hs_htf_mdot_max_frac");  // f_turbine_max1;


            c_heat_sink_phys.ms_params.m_pc_fl = as_integer("Fluid");
            c_heat_sink_phys.ms_params.m_pc_fl_props = as_matrix("field_fl_props");


            c_heat_sink_phys.ms_params.m_T_ext_cold_des = as_double("hs_phys_T_steam_cold_des");   //[C] Steam fluid inlet temp
            c_heat_sink_phys.ms_params.m_Q_ext_hot_des = as_double("hs_phys_Q_steam_hot_des");  //[C] Steam quality target
            c_heat_sink_phys.ms_params.m_P_ext_cold_des = as_double("hs_phys_P_steam_hot_des") * 100.0;  //[kPa] Steam inlet pressure
            c_heat_sink_phys.ms_params.m_P_ext_hot_des = as_double("hs_phys_P_steam_hot_des") * 100.0;   //[kPa] Steam outlet pressure

            // 25.01.30 twn: hardcode these for now so users don't have to set them
            c_heat_sink_phys.ms_params.m_f_m_dot_ext_min = 0.01;    // as_double("hs_phys_f_mdot_steam_min"); //[] Min fraction Steam mass flow rate
            c_heat_sink_phys.ms_params.m_f_m_dot_ext_max = 1.5;     // as_double("hs_phys_f_mdot_steam_max"); //[] Max fraction Steam mass flow rate
            c_heat_sink_phys.ms_params.m_N_sub_hx = 15;             // as_double("hs_phys_N_sub");      //[] Number HX nodes
            c_heat_sink_phys.ms_params.m_od_tol = 0.001;            // as_double("hs_phys_tol");         //[] HX off design tolerance

            // Allocate heat sink outputs
            c_heat_sink_phys.mc_reported_outputs.assign(C_pc_heat_sink_physical::E_Q_DOT_HEAT_SINK, allocate("q_dot_to_heat_sink", n_steps_fixed), n_steps_fixed);
            c_heat_sink_phys.mc_reported_outputs.assign(C_pc_heat_sink_physical::E_W_DOT_PUMPING, allocate("W_dot_pc_pump", n_steps_fixed), n_steps_fixed);
            c_heat_sink_phys.mc_reported_outputs.assign(C_pc_heat_sink_physical::E_M_DOT_HTF, allocate("m_dot_htf_heat_sink", n_steps_fixed), n_steps_fixed);
            c_heat_sink_phys.mc_reported_outputs.assign(C_pc_heat_sink_physical::E_T_HTF_IN, allocate("T_heat_sink_in", n_steps_fixed), n_steps_fixed);
            c_heat_sink_phys.mc_reported_outputs.assign(C_pc_heat_sink_physical::E_T_HTF_OUT, allocate("T_heat_sink_out", n_steps_fixed), n_steps_fixed);            

            c_heat_sink_phys.mc_reported_outputs.assign(C_pc_heat_sink_physical::E_M_DOT_EXT, allocate("m_dot_wf_heat_sink", n_steps_fixed), n_steps_fixed);
            c_heat_sink_phys.mc_reported_outputs.assign(C_pc_heat_sink_physical::E_X_OUT_EXT, allocate("x_out_wf_heat_sink", n_steps_fixed), n_steps_fixed);
            c_heat_sink_phys.mc_reported_outputs.assign(C_pc_heat_sink_physical::E_T_OUT_EXT, allocate("T_out_wf_heat_sink", n_steps_fixed), n_steps_fixed);
            c_heat_sink_phys.mc_reported_outputs.assign(C_pc_heat_sink_physical::E_HX_MIN_DT, allocate("hx_min_dT_heat_sink", n_steps_fixed), n_steps_fixed);


            c_heat_sink_pointer = &c_heat_sink_phys;
        }
        else
        {
            throw exec_error("trough_physical_iph", "hs_type must be 0-1");
        }

        if (c_heat_sink_pointer == nullptr)
        {
            throw exec_error("trough_physical_iph", "Heat sink pointer not assigned");
        }

        // Electricity pricing schedule
        C_timeseries_schedule_inputs elec_pricing_schedule;

        if (sim_type == 1)
        {
            if (csp_financial_model == 8 || csp_financial_model == 7) {        // No Financial Model or LCOH
                if (is_dispatch) {
                    throw exec_error("trough_physical_iph", "Can't select dispatch optimization if No Financial or LCOH model");
                }
                else { // if no dispatch optimization, don't need an input pricing schedule
                    // If electricity pricing data is not available, then dispatch to a uniform schedule
                    elec_pricing_schedule = C_timeseries_schedule_inputs(-1.0, std::numeric_limits<double>::quiet_NaN());
                }
            }
            else if (csp_financial_model == 1) {    // Single Owner

                double ppa_price_year1 = std::numeric_limits<double>::quiet_NaN();

                // Get first year base ppa price
                bool is_ppa_price_input_assigned = is_assigned("ppa_price_input");
                if (is_dispatch && !is_ppa_price_input_assigned) {
                    throw exec_error("trough_physical_iph", "\n\nYou selected dispatch optimization which requires that the array input ppa_price_input is defined\n");
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
                    throw exec_error("trough_physical_iph", "\n\nYou selected dispatch optimization and the Specify IRR Target financial solution mode, "
                        "but dispatch optimization requires known absolute electricity prices. Dispatch optimization requires "
                        "the Specify PPA Price financial solution mode. You can continue using dispatch optimization and iteratively "
                        "solve for the PPA that results in a target IRR by running a SAM Parametric analysis or script.\n");
                }

                // Time-of-Delivery factors by time step:
                int ppa_mult_model = as_integer("ppa_multiplier_model");
                if (ppa_mult_model == 1)    // use dispatch_ts input
                {
                    if (is_assigned("dispatch_factors_ts") || is_dispatch) {
                        auto vec = as_vector_double("dispatch_factors_ts");
                        elec_pricing_schedule = C_timeseries_schedule_inputs(vec, ppa_price_year1);
                    }
                    else { // if no dispatch optimization, don't need an input pricing schedule
                        elec_pricing_schedule = C_timeseries_schedule_inputs(-1.0, numeric_limits<double>::quiet_NaN());
                    }
                }
                else if (ppa_mult_model == 0) // standard diuranal input
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
            else if (csp_financial_model == 5) {    // Commercial

                bool is_ur_assigned = is_assigned("ur_en_ts_sell_rate");

                // rate data setup from ~ line 1336 in cmod_battery.cpp
                rate_data* util_rate_data = new rate_data();
                rate_setup::setup(m_vartab, 8760, 1, *util_rate_data, "cmod_trough_physical_iph");

                // Need to figure out dispatch, but for now, just use something so that annual simulation solves
                elec_pricing_schedule = C_timeseries_schedule_inputs(-1.0, std::numeric_limits<double>::quiet_NaN());
            }
            else {
                throw exec_error("trough_physical_iph", "csp_financial_model must be 1, 5, 7, or 8");
            }
        }
        else if (sim_type == 2)
        {
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

        C_csp_tou tou(offtaker_schedule, elec_pricing_schedule, dispatch_model_type, is_offtaker_frac_also_max);

        // Placeholder for heat price schedule
        //double heat_price = 0.02;   //[$/kWht]
        //C_timeseries_schedule_inputs heat_pricing_schedule = C_timeseries_schedule_inputs(1.0, heat_price);
        //tou.mc_heat_pricing_schedule = heat_pricing_schedule;

        // System parameters
        C_csp_solver::S_csp_system_params system;
        {
            system.m_pb_fixed_par = as_double("pb_fixed_par");
            size_t nval_bop_array = 0;
            ssc_number_t* bop_array = as_array("bop_array", &nval_bop_array);
            if (nval_bop_array != 5) throw exec_error("trough_physical", "Should be 5 elements in bop_array, has " + util::to_string((int)nval_bop_array) + ".");
            system.m_bop_par = bop_array[0];
            system.m_bop_par_f = bop_array[1];
            system.m_bop_par_0 = bop_array[2];
            system.m_bop_par_1 = bop_array[3];
            system.m_bop_par_2 = bop_array[4];
        }
        

        // *****************************************************
        // System dispatch
        //cst_iph_dispatch_opt dispatch;
        csp_dispatch_opt dispatch;

        if (is_dispatch && sim_type == 1) {

            double heater_startup_cost = 0.0;

            dispatch.solver_params.set_user_inputs(as_integer("disp_steps_per_hour"), as_integer("disp_frequency"), as_integer("disp_horizon"),
                as_integer("disp_max_iter"), as_double("disp_mip_gap"), as_double("disp_timeout"),
                as_integer("disp_spec_presolve"), as_integer("disp_spec_bb"), as_integer("disp_spec_scaling"), as_integer("disp_reporting"));

            bool can_cycle_use_standby = false;
            double disp_csu_cost_calc = 0.0;
            double disp_pen_ramping = 0.0;
            double q_rec_standby = 0.0;
            double q_rec_heattrace = 0.0;

            double disp_rsu_cost_calc = 0.0;

            dispatch.params.set_user_params(can_cycle_use_standby, as_double("disp_time_weighting"),
                disp_rsu_cost_calc, heater_startup_cost, disp_csu_cost_calc, disp_pen_ramping,
                as_double("disp_inventory_incentive"), q_rec_standby, q_rec_heattrace);

        }

        // Instantiate Solver
        C_csp_solver csp_solver(weather_reader, 
                                c_trough,
                                *c_heat_sink_pointer,
                                *storage_pointer, 
                                tou,
                                dispatch,
                                system,
                                p_heater,
                                nullptr,
                                ssc_cmod_update,
                                (void*)(this));

        // Set solver reporting outputs
        {
            // Simulation Kernel
            csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::TIME_FINAL, allocate("time_hr", n_steps_fixed), n_steps_fixed);
            csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::SIM_DURATION, allocate("timestep_sim_duration", n_steps_fixed), n_steps_fixed);
            // Weather reader
            csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::MONTH, allocate("month", n_steps_fixed), n_steps_fixed);
            csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::HOUR_DAY, allocate("hour_day", n_steps_fixed), n_steps_fixed);
            csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::SOLAZ, allocate("solazi", n_steps_fixed), n_steps_fixed);
            csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::SOLZEN, allocate("solzen", n_steps_fixed), n_steps_fixed);
            csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::BEAM, allocate("beam", n_steps_fixed), n_steps_fixed);
            csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::TDRY, allocate("tdry", n_steps_fixed), n_steps_fixed);
            csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::TWET, allocate("twet", n_steps_fixed), n_steps_fixed);
            csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::RH, allocate("RH", n_steps_fixed), n_steps_fixed);
            csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::WSPD, allocate("wspd", n_steps_fixed), n_steps_fixed);
            csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::PRES, allocate("pres", n_steps_fixed), n_steps_fixed);

            csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::CR_DEFOCUS, allocate("defocus", n_steps_fixed), n_steps_fixed);
            // TES
            csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::TES_Q_DOT_DC, allocate("q_dc_tes", n_steps_fixed), n_steps_fixed);
            csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::TES_Q_DOT_CH, allocate("q_ch_tes", n_steps_fixed), n_steps_fixed);
            csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::TES_E_CH_STATE, allocate("e_ch_tes", n_steps_fixed), n_steps_fixed);
            csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::M_DOT_CR_TO_TES_HOT, allocate("m_dot_cr_to_tes_hot", n_steps_fixed), n_steps_fixed);
            csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::M_DOT_TES_HOT_OUT, allocate("m_dot_tes_hot_out", n_steps_fixed), n_steps_fixed);
            csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::M_DOT_PC_TO_TES_COLD, allocate("m_dot_pc_to_tes_cold", n_steps_fixed), n_steps_fixed);
            csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::M_DOT_TES_COLD_OUT, allocate("m_dot_tes_cold_out", n_steps_fixed), n_steps_fixed);
            csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::M_DOT_FIELD_TO_CYCLE, allocate("m_dot_field_to_cycle", n_steps_fixed), n_steps_fixed);
            csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::M_DOT_CYCLE_TO_FIELD, allocate("m_dot_cycle_to_field", n_steps_fixed), n_steps_fixed);
            // System
            csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::SYS_W_DOT_FIXED, allocate("P_fixed", n_steps_fixed), n_steps_fixed);
            csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::SYS_W_DOT_BOP, allocate("P_plant_balance_tot", n_steps_fixed), n_steps_fixed);
            csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::W_DOT_NET, allocate("W_dot_parasitic_tot", n_steps_fixed), n_steps_fixed);
            // Controller
            csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::OP_MODE_1, allocate("op_mode_1", n_steps_fixed), n_steps_fixed);
            csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::OP_MODE_2, allocate("op_mode_2", n_steps_fixed), n_steps_fixed);
            csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::OP_MODE_3, allocate("op_mode_3", n_steps_fixed), n_steps_fixed);
            csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::ERR_M_DOT, allocate("m_dot_balance", n_steps_fixed), n_steps_fixed);
            csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::ERR_Q_DOT, allocate("q_balance", n_steps_fixed), n_steps_fixed);

            csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::N_OP_MODES, allocate("n_op_modes", n_steps_fixed), n_steps_fixed);
            csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::TOU_PERIOD, allocate("tou_value", n_steps_fixed), n_steps_fixed);
            csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::PRICING_MULT, allocate("pricing_mult", n_steps_fixed), n_steps_fixed);
            csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::ELEC_PRICE, allocate("elec_price_out", n_steps_fixed), n_steps_fixed);
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
            csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_SFEFF_EXPECT, allocate("disp_thermeff_expected", n_steps_fixed), n_steps_fixed);
            csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_PRES_NCONSTR, allocate("disp_presolve_nconstr", n_steps_fixed), n_steps_fixed);
            csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_PRES_NVAR, allocate("disp_presolve_nvar", n_steps_fixed), n_steps_fixed);
            csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_SOLVE_TIME, allocate("disp_solve_time", n_steps_fixed), n_steps_fixed);

        }
        

        update("Initialize physical trough model...", 0.0);

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

            throw exec_error("trough_physical", csp_exception.m_error_message);
        }

        // If no exception, then report messages
        while (csp_solver.mc_csp_messages.get_message(&out_type, &out_msg))
        {
            log(out_msg, out_type);
        }

        update("Begin timeseries simulation...", 0.0);

        // ********************************
        // ********************************
        // Report Design Point Calculations to UI
        // ********************************
        // ********************************
        double nameplate = std::numeric_limits<double>::quiet_NaN();
        {
            // System Design
            nameplate = q_dot_hs_des;
            {
                assign("nameplate", nameplate); // [MWt]
                assign("system_capacity", nameplate * 1.E3);    //[kWt]
                assign("cp_system_nameplate", nameplate);       //[MWt]
                assign("cp_battery_nameplate", 0.0);            //[MWt]
            }

            // Solar Field
            {
                assign("nSCA", c_trough.m_nSCA);
                assign("field_htf_min_temp", c_trough.m_htfProps.min_temp() - 273.15);    // [C]
                assign("field_htf_max_temp", c_trough.m_htfProps.max_temp() - 273.15);    // [C]
                assign("field_htf_cp_avg_des", c_trough.m_field_htf_cp_avg_des);          // [kJ/kg-K]
                assign("single_loop_aperture", c_trough.m_single_loop_aperture);  // [m2]
                assign("min_inner_diameter", c_trough.m_min_inner_diameter);      // [m]
                ssc_number_t* hce_design = allocate("csp_dtr_hce_design_heat_losses", c_trough.m_HCE_heat_loss_des.size());
                for (int i = 0; i < c_trough.m_HCE_heat_loss_des.size(); i++)
                    hce_design[i] = c_trough.m_HCE_heat_loss_des[i];    // [W/m]
                assign("csp_dtr_loop_hce_heat_loss", c_trough.m_HCE_heat_loss_loop_des); //[W/m]
                ssc_number_t* sca_effs = allocate("csp_dtr_sca_calc_sca_effs", c_trough.m_csp_dtr_sca_calc_sca_effs.size());
                for (int i = 0; i < c_trough.m_csp_dtr_sca_calc_sca_effs.size(); i++)
                    sca_effs[i] = c_trough.m_csp_dtr_sca_calc_sca_effs[i];    // []
                assign("loop_optical_efficiency", c_trough.m_opteff_des);  //[]
                ssc_number_t* hce_effs = allocate("csp_dtr_hce_optical_effs", c_trough.m_csp_dtr_hce_optical_effs.size());
                for (int i = 0; i < c_trough.m_csp_dtr_hce_optical_effs.size(); i++)
                    hce_effs[i] = c_trough.m_csp_dtr_hce_optical_effs[i];    // []
                assign("SCAInfoArray", c_trough.m_SCAInfoArray);    // []
                set_vector("SCADefocusArray", c_trough.m_SCADefocusArray);
                assign("max_field_flow_velocity", c_trough.m_max_field_flow_velocity);  //[m/s]
                assign("min_field_flow_velocity", c_trough.m_min_field_flow_velocity);  //[m/s]
                assign("max_loop_flow_vel_des", c_trough.m_max_loop_flow_vel_des);      //[m/s]
                assign("min_loop_flow_vel_des", c_trough.m_min_loop_flow_vel_des);      //[m/s]
                assign("total_loop_conversion_efficiency", c_trough.m_total_loop_conversion_efficiency_des);
                assign("total_required_aperture_for_SM1", c_trough.m_total_required_aperture_for_SM1);
                assign("required_number_of_loops_for_SM1", c_trough.m_required_number_of_loops_for_SM1);
                assign("nLoops", c_trough.m_nLoops);
                assign("total_aperture", c_trough.m_Ap_tot);    //[m2]
                assign("field_thermal_output_actual", c_trough.m_q_design_actual / 1e6); // [MWt]
                assign("field_thermal_output_ideal", c_trough.m_q_design_ideal / 1e6); // [MWt]
                assign("solar_mult", c_trough.m_solar_mult);
                assign("fixed_land_area", c_trough.m_fixed_land_area);  //[acre]
                assign("total_land_area", c_trough.m_total_land_area);  //[acre]
                double total_tracking_power = c_trough.get_tracking_power();
                assign("total_tracking_power", total_tracking_power);  //[MWe]
                assign("K_cpnt", c_trough.m_K_cpnt);
                assign("D_cpnt", c_trough.m_D_cpnt);    //[m]
                assign("L_cpnt", c_trough.m_L_cpnt);    //[m]
                assign("Type_cpnt", c_trough.m_Type_cpnt);   //[]

                assign("m_dot_htfmin_actual", c_trough.m_m_dot_htfmin); //[kg/s]
                assign("m_dot_htfmax_actual", c_trough.m_m_dot_htfmax); //[kg/s]
                assign("f_htfmin_actual", c_trough.m_f_htfmin);         //[]
                assign("f_htfmax_actual", c_trough.m_f_htfmax);         //[]

                assign("dP_sf_SS", c_trough.m_dP_sf_SS);                //[bar]
                assign("W_dot_pump_SS", c_trough.m_W_dot_pump_SS);      //[MWe]
            }

            // *************************
            // Heater
            {
                assign("q_dot_heater_des", q_dot_heater_des);       //[MWt]
                double W_dot_heater_des_calc = 0.0;                 //[MWe]
                double E_heater_su_des = 0.0;                       //[MWhr]
                if (is_parallel_heater) {
                    p_electric_resistance->get_design_parameters(E_heater_su_des, W_dot_heater_des_calc);
                }
                assign("W_dot_heater_des", (ssc_number_t)W_dot_heater_des_calc);    //[MWe]
                assign("E_heater_su_des", (ssc_number_t)E_heater_su_des);           //[MWhr]
            }


            // Thermal Storage
            {
                double V_tes_htf_avail_calc /*m3*/, V_tes_htf_total_calc /*m3*/,
                    h_tank_calc /*m*/, d_tank_calc /*m*/,
                    q_dot_loss_tes_des_calc /*MWt*/, dens_store_htf_at_T_ave_calc /*kg/m3*/,
                    Q_tes_des_calc /*MWht*/;

                double tes_htf_min_temp = 0;
                double tes_htf_max_temp = 0;
                double vol_min = 0;
                int is_hx = 0;
                double hot_vol_frac = 0;

                if (tes_type == C_csp_tes::csp_tes_types::E_TES_TWO_TANK)
                {
                    storage_two_tank.get_design_parameters(V_tes_htf_avail_calc, V_tes_htf_total_calc,
                        h_tank_calc, d_tank_calc, q_dot_loss_tes_des_calc, dens_store_htf_at_T_ave_calc, Q_tes_des_calc);

                    tes_htf_min_temp = storage_two_tank.get_min_storage_htf_temp() - 273.15;
                    tes_htf_max_temp = storage_two_tank.get_max_storage_htf_temp() - 273.15;
                    vol_min = V_tes_htf_total_calc * (storage_two_tank.m_h_tank_min / h_tank_calc);
                    hot_vol_frac = storage_two_tank.get_hot_tank_vol_frac();
                    is_hx = storage_two_tank.get_is_hx();
                }
                else if (tes_type == C_csp_tes::csp_tes_types::E_TES_PACKED_BED)
                {
                    storage_packedbed.get_design_parameters(V_tes_htf_avail_calc, V_tes_htf_total_calc,
                        h_tank_calc, d_tank_calc, q_dot_loss_tes_des_calc, dens_store_htf_at_T_ave_calc, Q_tes_des_calc);
                    hot_vol_frac = storage_packedbed.get_hot_tank_vol_frac();
                }
                else if (tes_type == C_csp_tes::csp_tes_types::E_TES_CYL)
                {
                    storage_cyl.get_design_parameters(V_tes_htf_avail_calc, V_tes_htf_total_calc,
                        h_tank_calc, d_tank_calc, q_dot_loss_tes_des_calc, dens_store_htf_at_T_ave_calc, Q_tes_des_calc);
                    hot_vol_frac = storage_cyl.get_hot_tank_vol_frac();
                }

                Q_tes = Q_tes_des_calc;

                double V_tank_hot_ini = hot_vol_frac * V_tes_htf_total_calc; // m3
                double T_avg = (as_double("T_loop_in_des") + as_double("T_loop_out")) / 2.0;    // C

                assign("q_tes", Q_tes_des_calc); // MWht
                assign("tes_avail_vol", V_tes_htf_avail_calc); // m3
                assign("vol_tank", V_tes_htf_total_calc);   // m3
                assign("csp_pt_tes_tank_height", h_tank_calc);    // m
                assign("csp_pt_tes_tank_diameter", d_tank_calc);    // m
                assign("q_dot_tes_est", q_dot_loss_tes_des_calc);   // MWt
                assign("csp_pt_tes_htf_density", dens_store_htf_at_T_ave_calc); // kg/m3
                assign("is_hx", is_hx);
                assign("vol_min", vol_min); // m3
                assign("V_tank_hot_ini", V_tank_hot_ini);   // m3
                assign("tes_htf_avg_temp", T_avg);  // C
                assign("tes_htf_min_temp", tes_htf_min_temp);
                assign("tes_htf_max_temp", tes_htf_max_temp);
                double q_dot_field_des = c_trough.m_solar_mult * q_dot_hs_des;
                assign("tshours_field", Q_tes_des_calc / q_dot_field_des);  //[hr]

                double tshours_heater = 0.0;
                if (q_dot_heater_des > 0.0) {
                    tshours_heater = Q_tes_des_calc / q_dot_heater_des;     //[hr]
                }
                assign("tshours_heater", tshours_heater);
            }

            // Collector
            {
                vector<double> L_SCA = as_vector_double("L_SCA");
                vector<double> ColperSCA = as_vector_double("ColperSCA");
                vector<double> Ave_Focal_Length = as_vector_double("Ave_Focal_Length");
                
                util::matrix_t<ssc_number_t> csp_dtr_sca_ap_lengths;
                {
                    size_t n = L_SCA.size();

                    csp_dtr_sca_ap_lengths.resize(n);           // NOTE!: You must do a separate 'fill', probably with how this is eventually set to an array instead of a matrix. This fails:  result(n, 1, std::numeric_limits<double>::quiet_NaN())
                    csp_dtr_sca_ap_lengths.fill(std::numeric_limits<double>::quiet_NaN());
                    for (size_t i = 0; i < n; i++) {
                        csp_dtr_sca_ap_lengths.at(i) = L_SCA.at(i) / ColperSCA.at(i);
                    }


                }

                double csp_dtr_sca_calc_zenith = std::numeric_limits<double>::quiet_NaN();
                {
                    csp_dtr_sca_calc_zenith = M_PI / 180. * (90. - (90. - (lat - 23.5)));
                }

                double csp_dtr_sca_calc_costh = std::numeric_limits<double>::quiet_NaN();
                {
                    double tilt = as_double("tilt");
                    double azimuth = as_double("azimuth");

                    csp_dtr_sca_calc_costh = sqrt(1 - pow(cos(1.57 - csp_dtr_sca_calc_zenith - tilt)
                        - cos(tilt)
                        * cos(1.57 - csp_dtr_sca_calc_zenith)
                        * (1. - cos(0. - azimuth)), 2));
                }

                double csp_dtr_sca_calc_theta = std::numeric_limits<double>::quiet_NaN();
                {
                    csp_dtr_sca_calc_theta = acos(csp_dtr_sca_calc_costh);
                }

                util::matrix_t<ssc_number_t> csp_dtr_sca_calc_end_gains(1, 1, std::numeric_limits<double>::quiet_NaN());
                {
                    vector<double> Distance_SCA = as_vector_double("Distance_SCA");

                    size_t n = Ave_Focal_Length.size();

                    csp_dtr_sca_calc_end_gains.resize(n);            // NOTE!: You must do a separate 'fill', probably with how this is eventually set to an array instead of a matrix. This fails:  result(n, 1, std::numeric_limits<double>::quiet_NaN())
                    csp_dtr_sca_calc_end_gains.fill(std::numeric_limits<double>::quiet_NaN());
                    for (size_t i = 0; i < n; i++)
                    {
                        double end_gain_calc = Ave_Focal_Length.at(i) * tan(csp_dtr_sca_calc_theta) - Distance_SCA.at(i);
                        csp_dtr_sca_calc_end_gains.at(i) = end_gain_calc > 0 ? end_gain_calc : 0;
                    }
                }

                util::matrix_t<ssc_number_t> csp_dtr_sca_calc_end_losses(1, 1, std::numeric_limits<double>::quiet_NaN());
                {
                    int nSCA = c_trough.m_nSCA;

                    size_t n = Ave_Focal_Length.size();

                    csp_dtr_sca_calc_end_losses.resize(n);
                    csp_dtr_sca_calc_end_losses.fill(std::numeric_limits<double>::quiet_NaN());
                    for (size_t i = 0; i < n; i++) {
                        csp_dtr_sca_calc_end_losses.at(i) = 1 - (Ave_Focal_Length.at(i) * tan(csp_dtr_sca_calc_theta)
                            - (nSCA - 1) / nSCA * csp_dtr_sca_calc_end_gains.at(i))
                            / (L_SCA.at(i) * ColperSCA.at(i));
                    }
                }

                double csp_dtr_sca_calc_latitude = lat;

                util::matrix_t<ssc_number_t> csp_dtr_sca_calc_iams(1, 1, std::numeric_limits<double>::quiet_NaN());
                {
                    util::matrix_t<double> IAM_matrix = as_matrix("IAM_matrix");

                    csp_dtr_sca_calc_iams.resize(IAM_matrix.nrows());
                    csp_dtr_sca_calc_iams.fill(std::numeric_limits<double>::quiet_NaN());
                    for (size_t i = 0; i < IAM_matrix.nrows(); i++) {
                        if (IAM_matrix.ncols() < 2) {                            // not sure this actually captures varying lengths of the different 1-D arrays in this matrix
                            csp_dtr_sca_calc_iams.at(i) = IAM_matrix.at(i, 0);
                        }
                        else {
                            double IAM = IAM_matrix.at(i, 0);
                            for (size_t j = 1; j < IAM_matrix.ncols(); j++) {
                                IAM = IAM + IAM_matrix.at(i, j) * pow(csp_dtr_sca_calc_theta, j) / csp_dtr_sca_calc_costh;
                            }
                            csp_dtr_sca_calc_iams.at(i) = IAM;
                        }
                    }

                }

                assign("csp_dtr_sca_ap_lengths", csp_dtr_sca_ap_lengths);
                assign("csp_dtr_sca_calc_zenith", csp_dtr_sca_calc_zenith);
                assign("csp_dtr_sca_calc_costh", csp_dtr_sca_calc_costh);
                assign("csp_dtr_sca_calc_theta", csp_dtr_sca_calc_theta);
                assign("csp_dtr_sca_calc_end_gains", csp_dtr_sca_calc_end_gains);
                assign("csp_dtr_sca_calc_end_losses", csp_dtr_sca_calc_end_losses);
                assign("csp_dtr_sca_calc_latitude", csp_dtr_sca_calc_latitude);
                assign("csp_dtr_sca_calc_iams", csp_dtr_sca_calc_iams);
            }

            // System Control
            {
                vector<double> bop_vec = as_vector_double("bop_array");
                double bop_design = bop_vec[0] * bop_vec[1] * (bop_vec[2] + bop_vec[3] + bop_vec[4]) * q_dot_hs_des;
                vector<double> aux_vec = as_vector_double("aux_array");
                double aux_design = aux_vec[0] * aux_vec[1] * (aux_vec[2] + aux_vec[3] + aux_vec[4]) * q_dot_hs_des;

                assign("bop_design", bop_design);       // MWe
                assign("aux_design", aux_design);       // MWe

                std::vector<double> timestep_load_fractions_calc;
                std::vector<double> timestep_load_abs_calc;
                for (S_timeseries_schedule_data data : offtaker_schedule.mv_timeseries_schedule_data)
                {
                    double frac_val = data.nondim_value;
                    double abs_val = q_dot_hs_des * frac_val * 1.e3;    //[kWt]
                    timestep_load_fractions_calc.push_back(frac_val);
                    timestep_load_abs_calc.push_back(abs_val);
                }

                set_vector("timestep_load_fractions_calc", timestep_load_fractions_calc);
                set_vector("timestep_load_abs_calc", timestep_load_abs_calc);


                // Need to assign thermal load in Btu for thermalrate_iph cmod if commercial
                if (csp_financial_model == 5)
                {
                    std::vector<double> load_abs_MMBtu;
                    for (double val_kW : timestep_load_abs_calc)
                    {
                        load_abs_MMBtu.push_back(val_kW / MMBTU_TO_KWh);
                    }
                    set_vector("thermal_load_heat_btu", load_abs_MMBtu);
                }

            }

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
        }

        // Calculate Costs and assign outputs
        if (true)
        {

            // Collect dedicated cost inputs
            double site_improvements_spec_cost = as_double("csp.dtr.cost.site_improvements.cost_per_m2");
            double solar_field_spec_cost = as_double("csp.dtr.cost.solar_field.cost_per_m2");
            double htf_system_spec_cost = as_double("csp.dtr.cost.htf_system.cost_per_m2");
            double storage_spec_cost = as_double("csp.dtr.cost.storage.cost_per_kwht");
            double heat_sink_spec_cost = as_double("csp.dtr.cost.heat_sink.cost_per_kwe");
            double bop_spec_cost = as_double("csp.dtr.cost.bop_per_kwe");
            double contingency_percent = as_double("csp.dtr.cost.contingency_percent");

            double epc_cost_per_acre = as_double("csp.dtr.cost.epc.per_acre");
            double epc_cost_percent_direct = as_double("csp.dtr.cost.epc.percent");
            double epc_cost_per_watt = as_double("csp.dtr.cost.epc.per_watt");
            double epc_cost_fixed = as_double("csp.dtr.cost.epc.fixed");
            double plm_cost_per_acre = as_double("csp.dtr.cost.plm.per_acre");
            double plm_cost_percent_direct = as_double("csp.dtr.cost.plm.percent");
            double plm_cost_per_watt = as_double("csp.dtr.cost.plm.per_watt");
            double plm_cost_fixed = as_double("csp.dtr.cost.plm.fixed");

            double sales_tax_percent = as_double("csp.dtr.cost.sales_tax.percent");

            // Collect necessary variables defined in other tabs
            double site_improvements_area = c_trough.m_Ap_tot;
            double solar_field_area = c_trough.m_Ap_tot;
            double htf_system_area = c_trough.m_Ap_tot;
            //Q_tes
            double heat_sink_mwe = q_dot_hs_des;         // MWe
            double bop_mwe = q_dot_hs_des;                 // MWe
            // total_land_area                      // m2
            double sales_tax_rate = as_double("sales_tax_rate");



            // Define outputs
            double heat_sink_cost_out, ts_cost_out, site_improvements_cost_out, bop_cost_out, solar_field_cost_out, heater_cost_out, htf_system_cost_out, dummy_cost_out, contingency_cost_out,
                total_direct_cost_out, epc_total_cost_out, plm_total_cost_out, total_indirect_cost_out, sales_tax_total_out, total_installed_cost_out, installed_per_capacity_out;

            // Calculate Costs
            N_mspt::calculate_mslf_costs(site_improvements_area, site_improvements_spec_cost, solar_field_area, solar_field_spec_cost, q_dot_heater_des /*[MWt]*/, heater_spec_cost, htf_system_area, htf_system_spec_cost, Q_tes, storage_spec_cost, 0,
                0, heat_sink_mwe, heat_sink_spec_cost, bop_mwe, bop_spec_cost, contingency_percent, c_trough.m_total_land_area, nameplate, epc_cost_per_acre, epc_cost_percent_direct, epc_cost_per_watt,
                epc_cost_fixed, plm_cost_per_acre, plm_cost_percent_direct, plm_cost_per_watt, plm_cost_fixed, sales_tax_rate, sales_tax_percent,

                heat_sink_cost_out, ts_cost_out, site_improvements_cost_out, bop_cost_out, solar_field_cost_out, heater_cost_out, htf_system_cost_out, dummy_cost_out, contingency_cost_out,
                total_direct_cost_out, epc_total_cost_out, plm_total_cost_out, total_indirect_cost_out, sales_tax_total_out, total_installed_cost_out, installed_per_capacity_out);

            double direct_subtotal = site_improvements_cost_out + solar_field_cost_out + heater_cost_out + htf_system_cost_out + ts_cost_out + heat_sink_cost_out + bop_cost_out;

            // Assign Outputs
            {
                assign("csp.dtr.cost.site_improvements", site_improvements_cost_out);
                assign("csp.dtr.cost.solar_field", solar_field_cost_out);
                assign("heater_cost", heater_cost_out);
                assign("csp.dtr.cost.htf_system", htf_system_cost_out);
                assign("csp.dtr.cost.storage", ts_cost_out);
                assign("csp.dtr.cost.heat_sink", heat_sink_cost_out);
                assign("csp.dtr.cost.bop", bop_cost_out);
                assign("csp.dtr.cost.contingency", contingency_cost_out);
                assign("total_direct_cost", total_direct_cost_out);
                assign("direct_subtotal", direct_subtotal);

                assign("csp.dtr.cost.epc.total", epc_total_cost_out);
                assign("csp.dtr.cost.plm.total", plm_total_cost_out);
                assign("total_indirect_cost", total_indirect_cost_out);

                assign("csp.dtr.cost.sales_tax.total", sales_tax_total_out);
                assign("total_installed_cost", total_installed_cost_out);
                assign("csp.dtr.cost.installed_per_capacity", installed_per_capacity_out);
            }


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

                N_financial_parameters::construction_financing_total_cost(total_installed_cost_out,
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
            
        }

        // Return if only called for design point
        if (sim_type != 1)
            return;

        std::clock_t clock_start = std::clock();

        try
        {
            // Simulate !
            csp_solver.Ssimulate(sim_setup);
        }
        catch( C_csp_exception &csp_exception )
        {
            // Report warning before exiting with error
            while( csp_solver.mc_csp_messages.get_message(&out_type, &out_msg) )
            {
                log(out_msg);
            }

            throw exec_error("trough_physical", csp_exception.m_error_message);
        }

        // If no exception, then report messages
        while( csp_solver.mc_csp_messages.get_message(&out_type, &out_msg) )
        {
            log(out_msg, out_type);
        }

        std::clock_t clock_end = std::clock();
        double sim_duration = (clock_end - clock_start) / (double)CLOCKS_PER_SEC;		//[s]
        assign("sim_duration", (ssc_number_t)sim_duration);

        // Do unit post-processing here


        



        // Convert mass flow rates from [kg/hr] to [kg/s]


        size_t count;
        ssc_number_t* p_q_dot_heat_sink = as_array("q_dot_to_heat_sink", &count);
        ssc_number_t *p_time_final_hr = as_array("time_hr", &count);
        if((int)count != n_steps_fixed)
            throw exec_error("trough_physical", "The number of fixed steps does not match the length of output data arrays");

        // 'adjustment_factors' class stores factors in hourly array, so need to index as such
        adjustment_factors haf(this->get_var_table(), "adjust");
        if( !haf.setup(n_steps_fixed) )
            throw exec_error("trough_physical", "failed to setup adjustment factors: " + haf.error());

        ssc_number_t *p_gen_heat = allocate("gen_heat", n_steps_fixed);
        ssc_number_t* p_gen = allocate("gen", n_steps_fixed);
        ssc_number_t* p_gen_heat_btu = allocate("gen_heat_btu", n_steps_fixed);
        ssc_number_t *p_W_dot_par_tot_haf = allocate("W_dot_par_tot_haf", n_steps_fixed);
        ssc_number_t *p_q_dot_defocus_est = allocate("q_dot_defocus_est", n_steps_fixed);
        ssc_number_t* p_load = allocate("load", n_steps_fixed); // testing using cmod_utilityrate5 for electricity rates p_load = p_W_dot_par_tot_haf


        ssc_number_t *p_W_dot_parasitic_tot = as_array("W_dot_parasitic_tot", &count);
        if (count != n_steps_fixed)
            throw exec_error("trough_physical", "The number of fixed steps does not match the length of output data arrays1");
        
        ssc_number_t *p_SCAs_def = as_array("SCAs_def", &count);
        if ((int)count != n_steps_fixed)
            throw exec_error("trough_physical", "The number of fixed steps does not match the length of output data arrays2");

        ssc_number_t *p_q_dot_htf_sf_out = as_array("q_dot_htf_sf_out", &count);
        if ((int)count != n_steps_fixed)
            throw exec_error("trough_physical", "The number of fixed steps does not match the length of output data arrays3");

        ssc_number_t* p_elec_price_out = as_array("elec_price_out", &count);
        if((int)count != n_steps_fixed)
            throw exec_error("trough_physical", "The number of fixed steps does not match the length of output data arrays4");

        //ssc_number_t *p_m_dot_tes_dc = as_array("m_dot_tes_dc", &count);
        //if ((int)count != n_steps_fixed)
        //    throw exec_error("trough_physical", "The number of fixed steps for 'm_dot_tes_dc' does not match the length of output data arrays");
        //
        //ssc_number_t *p_m_dot_tes_ch = as_array("m_dot_tes_ch", &count);
        //if ((int)count != n_steps_fixed)
        //    throw exec_error("trough_physical", "The number of fixed steps for 'm_dot_tes_ch' does not match the length of output data arrays");

        ssc_number_t* p_elec_purchase_cost = allocate("elec_purchase_cost", n_steps_fixed);

        double annual_elec_cost = 0.0;      //[$]
        double i_elec_cost = std::numeric_limits<double>::quiet_NaN();
        for(int i = 0; i < n_steps_fixed; i++)
        {
            size_t hour = (size_t)ceil(p_time_final_hr[i]);
            p_gen_heat[i] = (ssc_number_t)(p_q_dot_heat_sink[i] * haf(hour) * 1.E3);     //[kWt]
            p_gen[i] = (ssc_number_t)0.0;   //[kWt] (no electrical generation for IPH trough)
            p_gen_heat_btu[i] = p_gen_heat[i] / MMBTU_TO_KWh;   //[MMBtu/hr]
            p_W_dot_parasitic_tot[i] *= -1.0;           //[kWe] Label is total parasitics, so change to a positive value
            p_W_dot_par_tot_haf[i] = (ssc_number_t)(p_W_dot_parasitic_tot[i] * haf(hour) * 1.E3);       //[kWe]
            p_q_dot_defocus_est[i] = (ssc_number_t)(1.0 - p_SCAs_def[i])*p_q_dot_htf_sf_out[i]; //[MWt]
            //p_m_dot_tes_dc[i] = (ssc_number_t)(p_m_dot_tes_dc[i] / 3600.0);     //[kg/s] convert from kg/hr
            //p_m_dot_tes_ch[i] = (ssc_number_t)(p_m_dot_tes_ch[i] / 3600.0);     //[kg/s] convert from kg/hr
            p_load[i] = p_W_dot_par_tot_haf[i];

            i_elec_cost = p_elec_price_out[i] * p_W_dot_par_tot_haf[i];     //[$]
            p_elec_purchase_cost[i] = i_elec_cost;                          //[$]
            annual_elec_cost += i_elec_cost;                                //[$]
        }

        ssc_number_t* p_annual_energy_dist_time = gen_heatmap(this, steps_per_hour, true);
        // Non-timeseries array outputs
        double P_adj = 0; // slightly adjust all field design pressures to account for pressure drop in TES before hot tank
        if (tes_type == C_csp_tes::csp_tes_types::E_TES_TWO_TANK)
            P_adj = storage_two_tank.P_in_des;
        else if (tes_type == C_csp_tes::csp_tes_types::E_TES_CYL)
            P_adj = storage_cyl.P_in_des;

        transform(c_trough.m_P_rnr_dsn.begin(), c_trough.m_P_rnr_dsn.end(), c_trough.m_P_rnr_dsn.begin(), [P_adj](double x) {return x + P_adj; });
        transform(c_trough.m_P_hdr_dsn.begin(), c_trough.m_P_hdr_dsn.end(), c_trough.m_P_hdr_dsn.begin(), [P_adj](double x) {return x + P_adj; });
        transform(c_trough.m_P_loop_dsn.begin(), c_trough.m_P_loop_dsn.end(), c_trough.m_P_loop_dsn.begin(), [P_adj](double x) {return x + P_adj; });

        ssc_number_t *p_pipe_runner_diams = allocate("pipe_runner_diams", c_trough.m_D_runner.size());
        std::copy(c_trough.m_D_runner.begin(), c_trough.m_D_runner.end(), p_pipe_runner_diams);
        ssc_number_t *p_pipe_runner_wallthk = allocate("pipe_runner_wallthk", c_trough.m_WallThk_runner.size());
        std::copy(c_trough.m_WallThk_runner.begin(), c_trough.m_WallThk_runner.end(), p_pipe_runner_wallthk);
        ssc_number_t *p_pipe_runner_lengths = allocate("pipe_runner_lengths", c_trough.m_L_runner.size());
        std::copy(c_trough.m_L_runner.begin(), c_trough.m_L_runner.end(), p_pipe_runner_lengths);
        ssc_number_t *p_pipe_runner_expansions = allocate("pipe_runner_expansions", c_trough.m_N_rnr_xpans.size());
        std::copy(c_trough.m_N_rnr_xpans.begin(), c_trough.m_N_rnr_xpans.end(), p_pipe_runner_expansions);
        ssc_number_t *p_pipe_runner_mdot_dsn = allocate("pipe_runner_mdot_dsn", c_trough.m_m_dot_rnr_dsn.size());
        std::copy(c_trough.m_m_dot_rnr_dsn.begin(), c_trough.m_m_dot_rnr_dsn.end(), p_pipe_runner_mdot_dsn);
        ssc_number_t *p_pipe_runner_vel_dsn = allocate("pipe_runner_vel_dsn", c_trough.m_V_rnr_dsn.size());
        std::copy(c_trough.m_V_rnr_dsn.begin(), c_trough.m_V_rnr_dsn.end(), p_pipe_runner_vel_dsn);
        ssc_number_t *p_pipe_runner_T_dsn = allocate("pipe_runner_T_dsn", c_trough.m_T_rnr_dsn.size());
        std::copy(c_trough.m_T_rnr_dsn.begin(), c_trough.m_T_rnr_dsn.end(), p_pipe_runner_T_dsn);
        ssc_number_t *p_pipe_runner_P_dsn = allocate("pipe_runner_P_dsn", c_trough.m_P_rnr_dsn.size());
        std::copy(c_trough.m_P_rnr_dsn.begin(), c_trough.m_P_rnr_dsn.end(), p_pipe_runner_P_dsn);

        ssc_number_t *p_pipe_header_diams = allocate("pipe_header_diams", c_trough.m_D_hdr.size());
        std::copy(c_trough.m_D_hdr.begin(), c_trough.m_D_hdr.end(), p_pipe_header_diams);
        ssc_number_t *p_pipe_header_wallthk = allocate("pipe_header_wallthk", c_trough.m_WallThk_hdr.size());
        std::copy(c_trough.m_WallThk_hdr.begin(), c_trough.m_WallThk_hdr.end(), p_pipe_header_wallthk);
        ssc_number_t *p_pipe_header_lengths = allocate("pipe_header_lengths", c_trough.m_L_hdr.size());
        std::copy(c_trough.m_L_hdr.begin(), c_trough.m_L_hdr.end(), p_pipe_header_lengths);
        ssc_number_t *p_pipe_header_expansions = allocate("pipe_header_expansions", c_trough.m_N_hdr_xpans.size());
        std::copy(c_trough.m_N_hdr_xpans.begin(), c_trough.m_N_hdr_xpans.end(), p_pipe_header_expansions);
        ssc_number_t *p_pipe_header_mdot_dsn = allocate("pipe_header_mdot_dsn", c_trough.m_m_dot_hdr_dsn.size());
        std::copy(c_trough.m_m_dot_hdr_dsn.begin(), c_trough.m_m_dot_hdr_dsn.end(), p_pipe_header_mdot_dsn);
        ssc_number_t *p_pipe_header_vel_dsn = allocate("pipe_header_vel_dsn", c_trough.m_V_hdr_dsn.size());
        std::copy(c_trough.m_V_hdr_dsn.begin(), c_trough.m_V_hdr_dsn.end(), p_pipe_header_vel_dsn);
        ssc_number_t *p_pipe_header_T_dsn = allocate("pipe_header_T_dsn", c_trough.m_T_hdr_dsn.size());
        std::copy(c_trough.m_T_hdr_dsn.begin(), c_trough.m_T_hdr_dsn.end(), p_pipe_header_T_dsn);
        ssc_number_t *p_pipe_header_P_dsn = allocate("pipe_header_P_dsn", c_trough.m_P_hdr_dsn.size());
        std::copy(c_trough.m_P_hdr_dsn.begin(), c_trough.m_P_hdr_dsn.end(), p_pipe_header_P_dsn);

        ssc_number_t *p_pipe_loop_T_dsn = allocate("pipe_loop_T_dsn", c_trough.m_T_loop_dsn.size());
        std::copy(c_trough.m_T_loop_dsn.begin(), c_trough.m_T_loop_dsn.end(), p_pipe_loop_T_dsn);
        ssc_number_t *p_pipe_loop_P_dsn = allocate("pipe_loop_P_dsn", c_trough.m_P_loop_dsn.size());
        std::copy(c_trough.m_P_loop_dsn.begin(), c_trough.m_P_loop_dsn.end(), p_pipe_loop_P_dsn);

        // Two Tank specific outputs
        if (tes_type == C_csp_tes::csp_tes_types::E_TES_TWO_TANK)
        {
            ssc_number_t* p_pipe_tes_diams = allocate("pipe_tes_diams", storage_two_tank.pipe_diams.ncells());
            std::copy(storage_two_tank.pipe_diams.data(), storage_two_tank.pipe_diams.data() + storage_two_tank.pipe_diams.ncells(), p_pipe_tes_diams);
            ssc_number_t* p_pipe_tes_wallthk = allocate("pipe_tes_wallthk", storage_two_tank.pipe_wall_thk.ncells());
            std::copy(storage_two_tank.pipe_wall_thk.data(), storage_two_tank.pipe_wall_thk.data() + storage_two_tank.pipe_wall_thk.ncells(), p_pipe_tes_wallthk);
            ssc_number_t* p_pipe_tes_lengths = allocate("pipe_tes_lengths", storage_two_tank.pipe_lengths.ncells());
            std::copy(storage_two_tank.pipe_lengths.data(), storage_two_tank.pipe_lengths.data() + storage_two_tank.pipe_lengths.ncells(), p_pipe_tes_lengths);
            ssc_number_t* p_pipe_tes_mdot_dsn = allocate("pipe_tes_mdot_dsn", storage_two_tank.pipe_m_dot_des.ncells());
            std::copy(storage_two_tank.pipe_m_dot_des.data(), storage_two_tank.pipe_m_dot_des.data() + storage_two_tank.pipe_m_dot_des.ncells(), p_pipe_tes_mdot_dsn);
            ssc_number_t* p_pipe_tes_vel_dsn = allocate("pipe_tes_vel_dsn", storage_two_tank.pipe_vel_des.ncells());
            std::copy(storage_two_tank.pipe_vel_des.data(), storage_two_tank.pipe_vel_des.data() + storage_two_tank.pipe_vel_des.ncells(), p_pipe_tes_vel_dsn);
            ssc_number_t* p_pipe_tes_T_dsn = allocate("pipe_tes_T_dsn", storage_two_tank.pipe_T_des.ncells());
            std::copy(storage_two_tank.pipe_T_des.data(), storage_two_tank.pipe_T_des.data() + storage_two_tank.pipe_T_des.ncells(), p_pipe_tes_T_dsn);
            ssc_number_t* p_pipe_tes_P_dsn = allocate("pipe_tes_P_dsn", storage_two_tank.pipe_P_des.ncells());
            std::copy(storage_two_tank.pipe_P_des.data(), storage_two_tank.pipe_P_des.data() + storage_two_tank.pipe_P_des.ncells(), p_pipe_tes_P_dsn);
        }

        // Monthly outputs
        accumulate_monthly_for_year("gen_heat", "monthly_energy", sim_setup.m_report_step / 3600.0, steps_per_hour, 1);
        accumulate_monthly_for_year("gen_heat_btu", "monthly_energy_heat_btu", sim_setup.m_report_step / 3600.0, steps_per_hour, 1);

        // Annual outputs
        accumulate_annual_for_year("gen_heat", "annual_energy", sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour);
        accumulate_annual_for_year("gen_heat_btu", "annual_energy_heat_btu", sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour);
        //accumulate_annual_for_year("disp_objective", "disp_objective_ann", 1000.0*sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour);
        //accumulate_annual_for_year("disp_solve_iter", "disp_iter_ann", 1000.0*sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour);
        //accumulate_annual_for_year("disp_presolve_nconstr", "disp_presolve_nconstr_ann", sim_setup.m_report_step / 3600.0 / as_double("disp_frequency"), steps_per_hour, 1, n_steps_fixed / steps_per_hour);
        //accumulate_annual_for_year("disp_presolve_nvar", "disp_presolve_nvar_ann", sim_setup.m_report_step / 3600.0 / as_double("disp_frequency"), steps_per_hour, 1, n_steps_fixed / steps_per_hour);
        //accumulate_annual_for_year("disp_solve_time", "disp_solve_time_ann", sim_setup.m_report_step / 3600. / as_double("disp_frequency"), steps_per_hour, 1, n_steps_fixed / steps_per_hour);
        accumulate_annual_for_year("q_dc_tes", "annual_q_dc_tes", sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour);
        accumulate_annual_for_year("q_ch_tes", "annual_q_ch_tes", sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour);

        // This term currently includes TES freeze protection
        accumulate_annual_for_year("W_dot_par_tot_haf", "annual_electricity_consumption", sim_setup.m_report_step / 3600.0, steps_per_hour);	//[kWhe]

        double annual_electricity_consumption = as_double("annual_electricity_consumption");    //[kWhe]

        ssc_number_t annual_field_fp = accumulate_annual_for_year("q_dot_freeze_prot", "annual_field_freeze_protection", sim_setup.m_report_step / 3600.0*1.E3, steps_per_hour);    //[kWht]
        ssc_number_t annual_tes_fp = accumulate_annual_for_year("q_tes_heater", "annual_tes_freeze_protection", sim_setup.m_report_step / 3600.0*1.E3, steps_per_hour); //[kWht]

        ssc_number_t annual_thermal_consumption = annual_field_fp + annual_tes_fp;  //[kWht]
        assign("annual_thermal_consumption", annual_thermal_consumption);

        // Reporting dispatch solution counts
        std::vector<int> flag = as_vector_integer("disp_subopt_flag");
        std::vector<double> gap = as_vector_double("disp_rel_mip_gap");

        double avg_gap = 0;
        if (is_dispatch) {
            std::string disp_sum_msg;
            dispatch.count_solutions_by_type(flag, (int)as_double("disp_frequency"), disp_sum_msg);
            log(disp_sum_msg, SSC_NOTICE);
            avg_gap = dispatch.calc_avg_subopt_gap(gap, flag, (int)as_double("disp_frequency"));
        }
        assign("avg_suboptimal_rel_mip_gap", (ssc_number_t)avg_gap);

        // Calculate water use
        double V_water_mirrors = as_double("water_usage_per_wash")/1000.0*c_trough.m_Ap_tot*as_double("washing_frequency");
        assign("annual_total_water_use", (ssc_number_t)(V_water_mirrors));        //[m3]

        ssc_number_t ae = as_number("annual_energy");
        //ssc_number_t convfactor = (pg != 0) ? 100 * ae / pg : (ssc_number_t)0.0;
        //assign("conversion_factor", convfactor);

        double kWh_per_kW = 0.0;
        if (nameplate > 0.0)
            kWh_per_kW = ae / (nameplate * 1e3);

        assign("capacity_factor", (ssc_number_t)(kWh_per_kW / ((double)n_steps_fixed / (double)steps_per_hour)*100.));
        assign("kwh_per_kw", (ssc_number_t)kWh_per_kW);

        // Calculate percentage of heat load met
        //std::vector<double> gen = as_vector_double("gen");
        std::vector<double> heat_sink_q = as_vector_double("q_dot_to_heat_sink");
        double tot_heat_load = 0.0, heat_load = 0.0, load_met = 0.0;
        double step_s, hl_nondim_val, temp;
        int temp_int;
        double sec_per_ts = 3600. / steps_per_hour;
        for (int i = 0; i < heat_sink_q.size(); i++) {
            step_s = (i+1) * sec_per_ts;
            offtaker_schedule.get_timestep_data(step_s, hl_nondim_val, temp, temp_int);
            heat_load = hl_nondim_val * nameplate;
            tot_heat_load += heat_load;
            load_met += heat_sink_q[i] > heat_load ? heat_load : heat_sink_q[i];    // Only get credit for the load itself
        }
        assign("heat_load_capacity_factor", (ssc_number_t)load_met * 100. / tot_heat_load);        
    }

    template <typename T>
    void set_vector(const std::string& name, const vector<T> vec)
    {
        int size = vec.size();
        ssc_number_t* alloc_vals = allocate(name, size);
        for (int i = 0; i < size; i++)
            alloc_vals[i] = vec[i];    // []
    }

};

DEFINE_MODULE_ENTRY(trough_physical_iph, "Physical trough iph applications", 1)
