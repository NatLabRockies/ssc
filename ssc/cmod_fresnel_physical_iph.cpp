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

#include "common.h"

#include "csp_solver_fresnel_collector_receiver.h"
#include "csp_solver_pc_heat_sink.h"
#include "csp_dispatch.h"
#include "csp_system_costs.h"
#include "csp_solver_two_tank_tes.h"

#include <ctime>
#include <algorithm>
#include <iterator>

static var_info _cm_vtab_fresnel_physical_iph[] = {

    { SSC_INPUT,        SSC_NUMBER,      "sim_type",                    "1 (default): timeseries, 2: design only",                                          "",             "",               "System Control", "?=1",                    "",                       "SIMULATION_PARAMETER"},


    // Weather Reader

    { SSC_INPUT,        SSC_STRING,      "file_name",                 "Local weather file with path",                                                     "none",         "",               "weather",        "*",                       "LOCAL_FILE",            "" },


    // System Design

    { SSC_INPUT,    SSC_NUMBER,         "solar_mult_in",               "Solar multiple Input",                                                                  "",                    "",                             "System_Design",        "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "total_Ap_in",                 "Field aperture Input",                                                                  "m3",                  "",                             "System_Design",        "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "solar_mult_or_Ap",            "Design using specified solar mult or field aperture",                                   "m3",                  "",                             "System_Design",        "?=0",              "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "T_loop_in_des",               "Design loop inlet temperature",                                                         "C",                   "",                             "System_Design",        "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "T_loop_out",                  "Target loop outlet temperature",                                                        "C",                   "",                             "System_Design",        "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "I_bn_des",                    "Solar irradiation at design",                                                           "W/m2",                "",                             "System_Design",        "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "tshours",                     "Equivalent full-load thermal storage hours",                                            "hr",                  "",                             "System_Design",        "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "q_pb_design",                 "Design heat input to power block",                                                      "MWt",                 "",                             "System_Design",        "*",                "",                 "" },


    // Solar Field

    { SSC_INPUT,    SSC_NUMBER,         "nMod",                        "Number of collector modules in a loop",                                                 "",                    "",                             "Solar_Field",          "*",                "INTEGER",          "" },
    { SSC_INPUT,    SSC_NUMBER,         "eta_pump",                    "HTF pump efficiency",                                                                   "",                    "",                             "Solar_Field",          "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "HDR_rough",                   "Header pipe roughness",                                                                 "m",                   "",                             "Solar_Field",          "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "theta_stow",                  "stow angle",                                                                            "deg",                 "",                             "Solar_Field",          "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "theta_dep",                   "deploy angle",                                                                          "deg",                 "",                             "Solar_Field",          "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "FieldConfig",                 "Number of subfield headers",                                                            "",                    "",                             "Solar_Field",          "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "Fluid",                       "Field HTF fluid number",                                                                "",                    "",                             "Solar_Field",          "*",                "INTEGER",          "" },
    { SSC_INPUT,    SSC_NUMBER,         "T_fp",                        "Freeze protection temperature (heat trace activation temperature)",                     "C",                   "",                             "Solar_Field",          "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "V_hdr_max",                   "Maximum HTF velocity in the header at design",                                          "m/s",                 "",                             "Solar_Field",          "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "V_hdr_min",                   "Minimum HTF velocity in the header at design",                                          "m/s",                 "",                             "Solar_Field",          "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "Pipe_hl_coef",                "Loss coefficient from the header - runner pipe - and non-HCE piping",                   "W/m2-K",              "",                             "Solar_Field",          "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "mc_bal_hot",                  "The heat capacity of the balance of plant on the hot side",                             "kWht/K-MWt",          "",                             "Solar_Field",          "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "mc_bal_cold",                 "The heat capacity of the balance of plant on the cold side",                            "kWht/K-MWt",          "",                             "Solar_Field",          "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "mc_bal_sca",                  "Non-HTF heat capacity associated with each SCA - per meter basis",                      "Wht/K-m",             "",                             "Solar_Field",          "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "water_per_wash",              "Water usage per wash",                                                                  "L/m2_aper",           "",                             "Solar_Field",          "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "washes_per_year",             "Mirror washing frequency",                                                              "none",                "",                             "Solar_Field",          "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "rec_htf_vol",                 "Volume of HTF in a single collector unit per unit aperture area",                       "L/m2-ap",             "",                             "Solar_Field",          "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "T_amb_sf_des",                "Ambient design-point temperature for the solar field",                                  "C",                   "",                             "Solar_Field",          "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "V_wind_des",                  "Design-point wind velocity",                                                            "m/s",                 "",                             "Solar_Field",          "*",                "",                 "" },
    { SSC_INPUT,    SSC_MATRIX,         "field_fl_props",              "Fluid property data",                                                                   "",                    "",                             "Solar_Field",          "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "SCA_drives_elec",             "Tracking power in Watts per SCA drive",                                                 "W/module",            "",                             "Solar_Field",          "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "land_mult",                   "Non-solar field land area multiplier",                                                  "-",                   "",                             "Solar_Field",          "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "T_startup",                   "Power block startup temperature",                                                       "C",                   "",                             "Solar_Field",          "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "rec_su_delay",                "Fixed startup delay time for the receiver",                                             "hr",                  "",                             "Solar_Field",          "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "rec_qf_delay",                "Energy-based receiver startup delay (fraction of rated thermal power)",                 "-",                   "",                             "Solar_Field",          "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "p_start",                     "Collector startup energy, per SCA",                                                     "kWe-hr",              "",                             "Solar_Field",          "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "L_rnr_pb",                    "Length of runner pipe in power block",                                                  "m",                   "",                             "Solar_Field",          "*",                "",                 "" },

    { SSC_INPUT,    SSC_NUMBER,         "use_abs_or_rel_mdot_limit",   "Use mass flow abs (0) or relative (1) limits",                                          "",                    "",                             "solar_field",          "?=0",                          "",     "" },
    { SSC_INPUT,    SSC_NUMBER,         "m_dot_htfmin",                "Minimum loop HTF flow rate",                                                            "kg/s",                "",                             "solar_field",          "use_abs_or_rel_mdot_limit=0",  "",     "" },
    { SSC_INPUT,    SSC_NUMBER,         "m_dot_htfmax",                "Maximum loop HTF flow rate",                                                            "kg/s",                "",                             "solar_field",          "use_abs_or_rel_mdot_limit=0",  "",     "" },
    { SSC_INPUT,    SSC_NUMBER,         "f_htfmin",                    "Minimum loop mass flow rate fraction of design",                                        "",                    "",                             "solar_field",          "use_abs_or_rel_mdot_limit=1",  "",     "" },
    { SSC_INPUT,    SSC_NUMBER,         "f_htfmax",                    "Maximum loop mass flow rate fraction of design",                                        "",                    "",                             "solar_field",          "use_abs_or_rel_mdot_limit=1",  "",     "" },


    // Collector and Receiver

    { SSC_INPUT,    SSC_NUMBER,         "ColAz",                       "Collector azimuth angle",                                                               "deg",                 "",                             "Col_Rec",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "opt_model",                   "The optical model",                                                                     "",                    "",                             "Col_Rec",              "*",                "INTEGER",          "" },
    { SSC_INPUT,    SSC_NUMBER,         "A_aperture",                  "Reflective aperture area of the collector",                                             "m2",                  "",                             "Col_Rec",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "reflectivity",                "Solar-weighted mirror reflectivity value",                                              "",                    "",                             "Col_Rec",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "TrackingError",               "Tracking error derate",                                                                 "",                    "",                             "Col_Rec",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "GeomEffects",                 "Geometry effects derate",                                                               "",                    "",                             "Col_Rec",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "Dirt_mirror",                 "User-defined dirt on mirror derate",                                                    "",                    "",                             "Col_Rec",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "Error",                       "User-defined general optical error derate",                                             "",                    "",                             "Col_Rec",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "L_mod",                       "The length of the collector module",                                                    "m",                   "",                             "Col_Rec",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_ARRAY,          "IAM_T_coefs",                 "Incidence angle modifier coefficients - transversal plane",                             "",                    "",                             "Col_Rec",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_ARRAY,          "IAM_L_coefs",                 "Incidence angle modifier coefficients - longitudinal plane",                            "",                    "",                             "Col_Rec",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_MATRIX,         "OpticalTable",                "Values of the optical efficiency table",                                                "",                    "",                             "Col_Rec",              "*",                "",                 "" },

    { SSC_INPUT,    SSC_NUMBER,         "rec_model",                   "Receiver model type (1=Polynomial ; 2=Evac tube)",                                      "",                    "",                             "Col_Rec",              "*",                "INTEGER",          "" },
    { SSC_INPUT,    SSC_ARRAY,          "HCE_FieldFrac",               "The fraction of the field occupied by this HCE type",                                   "",                    "",                             "Col_Rec",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_ARRAY,          "D_abs_in",                    "The inner absorber tube diameter",                                                      "m",                   "",                             "Col_Rec",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_ARRAY,          "D_abs_out",                   "The outer absorber tube diameter",                                                      "m",                   "",                             "Col_Rec",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_ARRAY,          "D_glass_in",                  "The inner glass envelope diameter",                                                     "m",                   "",                             "Col_Rec",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_ARRAY,          "D_glass_out",                 "The outer glass envelope diameter",                                                     "m",                   "",                             "Col_Rec",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_ARRAY,          "D_plug",                      "The diameter of the absorber flow plug (optional)",                                     "m",                   "",                             "Col_Rec",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_ARRAY,          "Flow_type",                   "The flow type through the absorber",                                                    "",                    "",                             "Col_Rec",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_ARRAY,          "Rough",                       "Roughness of the internal surface",                                                     "m",                   "",                             "Col_Rec",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_ARRAY,          "alpha_env",                   "Envelope absorptance",                                                                  "",                    "",                             "Col_Rec",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_MATRIX,         "epsilon_abs_1",               "Absorber emittance - HCE variation 1",                                                  "",                    "",                             "Col_Rec",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_MATRIX,         "epsilon_abs_2",               "Absorber emittance - HCE variation 2",                                                  "",                    "",                             "Col_Rec",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_MATRIX,         "epsilon_abs_3",               "Absorber emittance - HCE variation 3",                                                  "",                    "",                             "Col_Rec",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_MATRIX,         "epsilon_abs_4",               "Absorber emittance - HCE variation 4",                                                  "",                    "",                             "Col_Rec",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_ARRAY,          "alpha_abs",                   "Absorber absorptance",                                                                  "",                    "",                             "Col_Rec",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_ARRAY,          "Tau_envelope",                "Envelope transmittance",                                                                "",                    "",                             "Col_Rec",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_ARRAY,          "epsilon_glass",               "Glass envelope emissivity",                                                             "",                    "",                             "Col_Rec",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_ARRAY,          "GlazingIntactIn",             "The glazing intact flag",                                                               "",                    "",                             "Col_Rec",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_ARRAY,          "P_a",                         "Annulus gas pressure",                                                                  "torr",                "",                             "Col_Rec",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_ARRAY,          "AnnulusGas",                  "Annulus gas type (1=air; 26=Ar; 27=H2)",                                                "",                    "",                             "Col_Rec",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_ARRAY,          "AbsorberMaterial",            "Absorber material type",                                                                "",                    "",                             "Col_Rec",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_ARRAY,          "Shadowing",                   "Receiver bellows shadowing loss factor",                                                "",                    "",                             "Col_Rec",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_ARRAY,          "dirt_env",                    "Loss due to dirt on the receiver envelope",                                             "",                    "",                             "Col_Rec",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_ARRAY,          "Design_loss",                 "Receiver heat loss at design",                                                          "W/m",                 "",                             "Col_Rec",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "L_mod_spacing",               "Piping distance between sequential modules in a loop",                                  "m",                   "",                             "Col_Rec",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "L_crossover",                 "Length of crossover piping in a loop",                                                  "m",                   "",                             "Col_Rec",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_ARRAY,          "HL_T_coefs",                  "HTF temperature-dependent heat loss coefficients",                                      "W/m-K",               "",                             "Col_Rec",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_ARRAY,          "HL_w_coefs",                  "Wind-speed-dependent heat loss coefficients",                                           "W/m-(m/s)",           "",                             "Col_Rec",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "DP_nominal",                  "Pressure drop across a single collector assembly at design",                            "bar",                 "",                             "Col_Rec",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_ARRAY,          "DP_coefs",                    "Pressure drop mass flow based part-load curve",                                         "",                    "",                             "Col_Rec",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "nRecVar",                     "Number of receiver variations",                                                         "",                    "",                             "Col_Rec",              "?=4",              "INTEGER",          "" },


    // Heat Sink

    {SSC_INPUT,     SSC_NUMBER,         "pb_pump_coef",                "Pumping power to move 1kg of HTF through PB loop",                                      "kW/kg",               "",                              "Heat Sink",           "*",                "",                 ""},

    // TES   

    { SSC_INPUT,    SSC_NUMBER,         "store_fluid",                 "Storage HTF ID",                                                                        "",                    "",                             "Storage",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_MATRIX,         "store_fl_props",              "Storage user-defined HTF Properties",                                                   "",                    "",                             "Storage",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "h_tank",                      "Height of HTF when tank is full",                                                       "",                    "",                             "Storage",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "u_tank",                      "Loss coefficient from tank",                                                            "",                    "",                             "Storage",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "tank_pairs",                  "Number of equivalent tank pairs",                                                       "",                    "",                             "Storage",              "*",                "INTEGER",          "" },
    { SSC_INPUT,    SSC_NUMBER,         "hot_tank_Thtr",               "Hot tank heater set point",                                                             "",                    "",                             "Storage",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "hot_tank_max_heat",           "Rated heater capacity for hot tank heating",                                            "MWe",                 "",                             "Storage",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "cold_tank_Thtr",              "Cold tank heater set point",                                                            "",                    "",                             "Storage",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "cold_tank_max_heat",          "Rated heater capacity for cold tank heating",                                           "MWe",                 "",                             "Storage",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "dt_hot",                      "Hot side HX approach temp",                                                             "",                    "",                             "Storage",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "h_tank_min",                  "Minimum tank fluid height",                                                             "",                    "",                             "Storage",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "dt_cold",                     "Cold side HX approach temp",                                                            "",                    "",                             "Storage",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "init_hot_htf_percent",        "Initial fraction of avail. vol that is hot",                                            "%",                   "",                             "Storage",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "tes_pump_coef",               "Pumping power to move 1kg of HTF through tes loop",                                     "kW/(kg/s)",           "",                             "Storage",              "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "tanks_in_parallel",           "Tanks are in parallel, not in series, with solar field",                                "-",                   "",                             "Storage",              "*",                "",                 "" },

    /*Storage NOT in UI*/{ SSC_INPUT,    SSC_NUMBER,         "V_tes_des",                   "Design-point velocity to size the TES pipe diameters",                                  "m/s",                 "",                             "Storage",              "?=1.85",           "",                 "SIMULATION_PARAMETER" },


    // System Control
    { SSC_INPUT,    SSC_NUMBER,         "is_timestep_load_fractions",  "Use turbine load fraction for each timestep instead of block dispatch?",                "",                    "",                             "tou",                  "?=0",              "",             "SIMULATION_PARAMETER" },
    { SSC_INPUT,    SSC_ARRAY,          "timestep_load_fractions",     "Turbine load fraction for each timestep, alternative to block dispatch",                "",                    "",                             "tou",                  "?",                "",             "SIMULATION_PARAMETER" },

    { SSC_INPUT,    SSC_NUMBER,         "pb_fixed_par",                "Fixed parasitic load - runs at all times",                                              "",                    "",                             "Sys_Control",          "*",                "",                 "" },
    { SSC_INPUT,    SSC_ARRAY,          "bop_array",                   "Balance of plant parasitic power fraction",                                             "",                    "",                             "Sys_Control",          "*",                "",                 "" },
    { SSC_INPUT,    SSC_ARRAY,          "aux_array",                   "Aux heater, boiler parasitic",                                                          "",                    "",                             "Sys_Control",          "*",                "",                 "" },

    { SSC_INPUT,    SSC_NUMBER,         "is_dispatch",                 "Allow dispatch optimization?",  /*TRUE=1*/                                              "-",                   "",                             "Sys_Control",          "?=0",              "",                 "" },

    { SSC_INPUT,    SSC_NUMBER,         "disp_rsu_cost_rel",           "Receiver startup cost",                                                                 "$/MWt/start",         "",                             "Sys_Control",          "is_dispatch=1",    "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "disp_horizon",                "Time horizon for dispatch optimization",                                                "hour",                "",                             "Sys_Control",          "is_dispatch=1",    "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "disp_frequency",              "Frequency for dispatch optimization calculations",                                      "hour",                "",                             "Sys_Control",          "is_dispatch=1",    "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "disp_max_iter",               "Max. no. dispatch optimization iterations",                                             "-",                   "",                             "Sys_Control",          "is_dispatch=1",    "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "disp_timeout",                "Max. dispatch optimization solve duration",                                             "s",                   "",                             "Sys_Control",          "is_dispatch=1",    "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "disp_mip_gap",                "Dispatch optimization solution tolerance",                                              "-",                   "",                             "Sys_Control",          "is_dispatch=1",    "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "disp_time_weighting",         "Dispatch optimization future time discounting factor",                                  "-",                   "",                             "Sys_Control",          "?=0.99",           "",                 "" },

    /*LK Only*/{ SSC_INPUT,    SSC_NUMBER,         "is_write_ampl_dat",           "Write AMPL data files for dispatch run",                                                "-",                   "",                             "tou",                                      "?=0",                     "",                      "SIMULATION_PARAMETER" },
    /*LK Only*/{ SSC_INPUT,    SSC_NUMBER,         "is_ampl_engine",              "Run dispatch optimization with external AMPL engine",                                   "-",                   "",                             "tou",                                      "?=0",                     "",                      "SIMULATION_PARAMETER" },
    /*LK Only*/{ SSC_INPUT,    SSC_STRING,         "ampl_data_dir",               "AMPL data file directory",                                                              "-",                   "",                             "tou",                                      "?=''",                    "",                      "SIMULATION_PARAMETER" },
    /*LK Only*/{ SSC_INPUT,    SSC_STRING,         "ampl_exec_call",              "System command to run AMPL code",                                                       "-",                   "",                             "tou",                                      "?='ampl sdk_solution.run'", "",                    "SIMULATION_PARAMETER" },
    /*LK Only*/{ SSC_INPUT,    SSC_NUMBER,         "disp_spec_presolve",          "Dispatch optimization presolve heuristic",                                              "-",                   "",                             "tou",                                      "?=-1",                    "",                      "SIMULATION_PARAMETER" },
    /*LK Only*/{ SSC_INPUT,    SSC_NUMBER,         "disp_spec_bb",                "Dispatch optimization B&B heuristic",                                                   "-",                   "",                             "tou",                                      "?=-1",                    "",                      "SIMULATION_PARAMETER" },
    /*LK Only*/{ SSC_INPUT,    SSC_NUMBER,         "disp_reporting",              "Dispatch optimization reporting level",                                                 "-",                   "",                             "tou",                                      "?=-1",                    "",                      "SIMULATION_PARAMETER" },
    /*LK Only*/{ SSC_INPUT,    SSC_NUMBER,         "disp_spec_scaling",           "Dispatch optimization scaling heuristic",                                               "-",                   "",                             "tou",                                      "?=-1",                    "",                      "SIMULATION_PARAMETER" },
    /*LK Only*/{ SSC_INPUT,    SSC_NUMBER,         "disp_inventory_incentive",    "Dispatch storage terminal inventory incentive multiplier",                              "",                    "",                             "System Control",                           "?=0.0",                   "",                      "SIMULATION_PARAMETER" },

    // Receiver control
    /*LK Only*/{ SSC_INPUT,    SSC_NUMBER,         "q_rec_heattrace",             "Receiver heat trace energy consumption during startup",                                 "kWe-hr",              "",                             "tou",                                      "?=0.0",                   "",                      "SIMULATION_PARAMETER" },
    /*LK Only*/{ SSC_INPUT,    SSC_NUMBER,         "q_rec_standby",               "Receiver standby energy consumption",                                                   "kWt",                 "",                             "tou",                                      "?=9e99",                  "",                      "SIMULATION_PARAMETER" },



    // Financials  and Pricing schedules (copied from electricity - eventually need to resolve electricity vs. heat)
    {SSC_INPUT,    SSC_NUMBER,          "csp_financial_model",         "",                                                                                      "1-8",                 "",                             "Financial Model",      "*",              "INTEGER,MIN=0",    ""},

    { SSC_INPUT,    SSC_NUMBER,         "ppa_multiplier_model",        "PPA multiplier model 0: dispatch factors dispatch_factorX, 1: hourly multipliers dispatch_factors_ts", "0/1",  "",                             "tou",                  "?=0",  /*need a default so this var works in required_if*/ "INTEGER,MIN=0",  "SIMULATION_PARAMETER" },
    { SSC_INPUT,    SSC_NUMBER,         "ppa_soln_mode",               "PPA solution mode (0=Specify IRR target, 1=Specify PPA price)",                         "",                    "",                             "Financial Solution Mode",                  "ppa_multiplier_model=0&csp_financial_model<5&is_dispatch=1&sim_type=1",       "",              "SIMULATION_PARAMETER" },
    { SSC_INPUT,    SSC_ARRAY,          "dispatch_factors_ts",         "Dispatch payment factor array",                                                         "",                    "",                             "tou",                                      "ppa_multiplier_model=1&csp_financial_model<5&is_dispatch=1&sim_type=1",       "",              "SIMULATION_PARAMETER" },
    { SSC_INPUT,    SSC_MATRIX,         "dispatch_sched_weekday",      "PPA pricing weekday schedule, 12x24",                                                   "",                    "",                             "Time of Delivery Factors",                 "ppa_multiplier_model=0&csp_financial_model<5&is_dispatch=1&sim_type=1",       "",              "SIMULATION_PARAMETER" },
    { SSC_INPUT,    SSC_MATRIX,         "dispatch_sched_weekend",      "PPA pricing weekend schedule, 12x24",                                                   "",                    "",                             "Time of Delivery Factors",                 "ppa_multiplier_model=0&csp_financial_model<5&is_dispatch=1&sim_type=1",       "",              "SIMULATION_PARAMETER" },
    { SSC_INPUT,    SSC_ARRAY,          "dispatch_tod_factors",        "TOD factors for periods 1 through 9",                                                   "",                    "",                             "Time of Delivery Factors",                 "ppa_multiplier_model=0&csp_financial_model<5&is_dispatch=1&sim_type=1",       "",  "SIMULATION_PARAMETER" },
    { SSC_INPUT,    SSC_ARRAY,          "ppa_price_input",             "PPA solution mode (0=Specify IRR target, 1=Specify PPA price)",                         "",                    "",                             "Financial Solution Mode",                  "ppa_multiplier_model=0&csp_financial_model<5&is_dispatch=1&sim_type=1",       "",              "SIMULATION_PARAMETER" },


    // System Control
    { SSC_INPUT,    SSC_MATRIX,         "weekday_schedule",            "12x24 Time of Use Values for week days",                                                "",                    "",                             "Sys_Control",          "*",                "",                 "" },
    { SSC_INPUT,    SSC_MATRIX,         "weekend_schedule",            "12x24 Time of Use Values for week end days",                                            "",                    "",                             "Sys_Control",          "*",                "",                 "" },
    { SSC_INPUT,    SSC_NUMBER,         "is_tod_pc_target_also_pc_max","Is the TOD target cycle heat input also the max cycle heat input?",                     "",                    "",                             "tou",                  "?=0",              "",                 "" },
    { SSC_INPUT,    SSC_ARRAY,          "f_turb_tou_periods",          "Dispatch logic for turbine load fraction",                                              "-",                   "",                             "tou",                  "*",                "",                 "" },


    // Capital Costs
                // Direct Capital Costs
    { SSC_INPUT,    SSC_NUMBER,         "site_improvements_spec_cost", "Site Improvement Cost per m2",                                                          "$/m2",                "",                             "Capital_Costs",                 "?=0",       "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "solar_field_spec_cost",       "Solar Field Cost per m2",                                                               "$/m2",                "",                             "Capital_Costs",                 "?=0",       "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "htf_system_spec_cost",        "HTF System Cost Per m2",                                                                "$/m2",                "",                             "Capital_Costs",                 "?=0",       "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "storage_spec_cost",           "Storage cost per kWht",                                                                 "$/kWht",              "",                             "Capital_Costs",                 "?=0",       "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "heat_sink_spec_cost",         "Heat Sink Cost per kWt",                                                                "$/kWt",               "",                             "Capital_Costs",                 "?=0",       "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "bop_spec_cost",               "Balance of Plant Cost per kWe",                                                         "$/kWe",               "",                             "Capital_Costs",                 "?=0",       "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "contingency_percent",         "Contingency Percent",                                                                   "%",                   "",                             "Capital_Costs",                 "?=0",       "",              "" },

    // Indirect Capital Costs
    { SSC_INPUT,    SSC_NUMBER,         "epc_cost_per_acre",           "EPC Costs per acre",                                                                    "$/acre",              "",                             "Capital_Costs",                 "?=0",       "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "epc_cost_percent_direct",     "EPC Costs % direct",                                                                    "%",                   "",                             "Capital_Costs",                 "?=0",       "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "epc_cost_per_watt",           "EPC Cost Wac",                                                                          "$/Wac",               "",                             "Capital_Costs",                 "?=0",       "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "epc_cost_fixed",              "Fixed EPC Cost",                                                                        "$",                   "",                             "Capital_Costs",                 "?=0",       "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "plm_cost_per_acre",           "Land Cost per acre",                                                                    "$/acre",              "",                             "Capital_Costs",                 "?=0",       "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "plm_cost_percent_direct",     "Land Cost % direct",                                                                    "%",                   "",                             "Capital_Costs",                 "?=0",       "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "plm_cost_per_watt",           "Land Cost Wac",                                                                         "$/Wac",               "",                             "Capital_Costs",                 "?=0",       "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "plm_cost_fixed",              "Fixed Land Cost",                                                                       "$",                   "",                             "Capital_Costs",                 "?=0",       "",              "" },


    // Sales Tax
    { SSC_INPUT,    SSC_NUMBER,         "sales_tax_percent",           "Sales Tax Percentage of Direct Cost",                                                   "%",                   "",                             "Capital_Costs",                 "?=0",       "",              "" },
    { SSC_INPUT,    SSC_NUMBER,         "sales_tax_rate",              "Sales Tax Rate",                                                                        "%",                   "",                             "Capital_Costs",                 "?=0",       "",              "" },

        // Construction financing inputs/outputs (SSC variable table from cmod_cb_construction_financing)
    { SSC_INPUT,     SSC_NUMBER,        "const_per_interest_rate1",    "Interest rate, loan 1",                                                                 "%",                   "",                             "Financial Parameters",          "*",         "",              "" },
    { SSC_INPUT,     SSC_NUMBER,        "const_per_interest_rate2",    "Interest rate, loan 2",                                                                 "%",                   "",                             "Financial Parameters",          "*",         "",              "" },
    { SSC_INPUT,     SSC_NUMBER,        "const_per_interest_rate3",    "Interest rate, loan 3",                                                                 "%",                   "",                             "Financial Parameters",          "*",         "",              "" },
    { SSC_INPUT,     SSC_NUMBER,        "const_per_interest_rate4",    "Interest rate, loan 4",                                                                 "%",                   "",                             "Financial Parameters",          "*",         "",              "" },
    { SSC_INPUT,     SSC_NUMBER,        "const_per_interest_rate5",    "Interest rate, loan 5",                                                                 "%",                   "",                             "Financial Parameters",          "*",         "",              "" },
    { SSC_INPUT,     SSC_NUMBER,        "const_per_months1",           "Months prior to operation, loan 1",                                                     "",                    "",                             "Financial Parameters",          "*",         "",              "" },
    { SSC_INPUT,     SSC_NUMBER,        "const_per_months2",           "Months prior to operation, loan 2",                                                     "",                    "",                             "Financial Parameters",          "*",         "",              "" },
    { SSC_INPUT,     SSC_NUMBER,        "const_per_months3",           "Months prior to operation, loan 3",                                                     "",                    "",                             "Financial Parameters",          "*",         "",              "" },
    { SSC_INPUT,     SSC_NUMBER,        "const_per_months4",           "Months prior to operation, loan 4",                                                     "",                    "",                             "Financial Parameters",          "*",         "",              "" },
    { SSC_INPUT,     SSC_NUMBER,        "const_per_months5",           "Months prior to operation, loan 5",                                                     "",                    "",                             "Financial Parameters",          "*",         "",              "" },
    { SSC_INPUT,     SSC_NUMBER,        "const_per_percent1",          "Percent of total installed cost, loan 1",                                               "%",                   "",                             "Financial Parameters",          "*",         "",              "" },
    { SSC_INPUT,     SSC_NUMBER,        "const_per_percent2",          "Percent of total installed cost, loan 2",                                               "%",                   "",                             "Financial Parameters",          "*",         "",              "" },
    { SSC_INPUT,     SSC_NUMBER,        "const_per_percent3",          "Percent of total installed cost, loan 3",                                               "%",                   "",                             "Financial Parameters",          "*",         "",              "" },
    { SSC_INPUT,     SSC_NUMBER,        "const_per_percent4",          "Percent of total installed cost, loan 4",                                               "%",                   "",                             "Financial Parameters",          "*",         "",              "" },
    { SSC_INPUT,     SSC_NUMBER,        "const_per_percent5",          "Percent of total installed cost, loan 5",                                               "%",                   "",                             "Financial Parameters",          "*",         "",              "" },
    { SSC_INPUT,     SSC_NUMBER,        "const_per_upfront_rate1",     "Upfront fee on principal, loan 1",                                                      "%",                   "",                             "Financial Parameters",          "*",         "",              "" },
    { SSC_INPUT,     SSC_NUMBER,        "const_per_upfront_rate2",     "Upfront fee on principal, loan 2",                                                      "%",                   "",                             "Financial Parameters",          "*",         "",              "" },
    { SSC_INPUT,     SSC_NUMBER,        "const_per_upfront_rate3",     "Upfront fee on principal, loan 3",                                                      "%",                   "",                             "Financial Parameters",          "*",         "",              "" },
    { SSC_INPUT,     SSC_NUMBER,        "const_per_upfront_rate4",     "Upfront fee on principal, loan 4",                                                      "%",                   "",                             "Financial Parameters",          "*",         "",              "" },
    { SSC_INPUT,     SSC_NUMBER,        "const_per_upfront_rate5",     "Upfront fee on principal, loan 5",                                                      "%",                   "",                             "Financial Parameters",          "*",         "",              "" },


    // OUTPUTS
        // Design Point Outputs

    // System capacity required by downstream financial model
    { SSC_OUTPUT,    SSC_NUMBER, "system_capacity",                    "System capacity",                                                           "kWt",          "",                                  "System Design",                             "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "cp_system_nameplate",                 "System capacity for capacity payments",                                    "MWt",          "",                                  "System Design",                             "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "cp_battery_nameplate",                "Battery nameplate",                                                        "MWt",          "",                                  "System Design",                             "*",                                                                "",              "" },


        // System Design
    { SSC_OUTPUT,       SSC_NUMBER,     "solar_mult",                       "Actual solar multiple",                                                "",          "",         "System Design Calc",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "total_Ap",                         "Actual field aperture",                                                "m2",          "",         "System Design Calc",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "nLoops",                           "Number of loops in the field",                                         "",             "",         "controller",                              "*",        "",              "" },

    // Solar Field
    { SSC_OUTPUT,       SSC_NUMBER,     "A_loop",                           "Aperture of a single loop",                                            "m2",           "",         "Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "loop_opt_eff",                     "Loop optical efficiency at design",                                    "",             "",         "Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "loop_therm_eff",                   "Loop thermal efficiency at design",                                    "",             "",         "Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "loop_eff",                         "Total loop conversion efficiency at design",                           "",             "",         "Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "sm1_aperture",                     "Total required aperture, SM=1",                                        "m2",           "",         "Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "sm1_nLoops",                       "Required number of loops, SM=1",                                       "",             "",         "Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "total_tracking_power",             "Design tracking power",                                                "MW",           "",         "Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "A_field",                          "Total field aperture",                                                 "m2",           "",         "Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "q_field_des_actual",               "Design-point thermal power from the solar field limited by mass flow", "MW",           "",         "Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "q_field_des_ideal",                "Design-point thermal power from the solar field with no limit",        "MW",           "",         "Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "field_area",                       "Solar field area",                                                     "acres",        "",         "Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "total_land_area",                  "Total land area",                                                      "acres",        "",         "Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "field_htf_min_temp",               "Minimum field htf temp",                                               "C",            "",         "Power Cycle",                    "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "field_htf_max_temp",               "Maximum field htf temp",                                               "C",            "",         "Power Cycle",                    "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "mdot_field_des",                   "Field design HTF mass flow rate",                                      "kg/s",         "",         "Receiver",                       "*",                                                                "",              "" },
    
    { SSC_OUTPUT,       SSC_NUMBER,     "dP_field_des_SS",                  "Steady State Field design total pressure drop",                        "bar",          "",          "Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "Q_field_des_SS",                   "Steady State Field design thermal power",                              "MWt",          "",          "Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "T_field_out_des_SS",               "Steady State Field design outlet temperature",                         "C",            "",          "Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "m_dot_des_SS",                     "Steady State Field mass flow rate",                                    "kg/s",         "",          "Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "m_dot_loop_des_SS",                "Steady State Loop mass flow rate",                                     "kg/s",         "",          "Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "V_hdr_min_des_SS",                 "Steady State min header velocity",                                     "m/s",          "",          "Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "V_hdr_max_des_SS",                 "Steady State max header velocity",                                     "m/s",          "",          "Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "eta_optical_des_SS",               "Steady State optical efficiency",                                      "",             "",          "Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "therm_eff_des_SS",                 "Steady State field optical efficiency",                                "",             "",          "Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "eff_des_SS",                       "Steady State field total efficiency",                                  "",             "",          "Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "W_dot_pump_des_SS",                "Steady State field pumping power",                                     "MWe",          "",          "Receiver",                       "*",                                                                "",              "" },

    { SSC_OUTPUT,       SSC_NUMBER,     "m_dot_htfmin_actual",              "Actual minimum loop HTF flow rate",                                    "kg/s",         "",          "solar_field",                    "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "m_dot_htfmax_actual",              "Actual maximum loop HTF flow rate",                                    "kg/s",         "",          "solar_field",                    "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "f_htfmin_actual",                  "Actual minimum loop mass flow rate fraction of design",                "",             "",          "solar_field",                    "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "f_htfmax_actual",                  "Actual maximum loop mass flow rate fraction of design",                "",             "",          "solar_field",                    "*",                                                                "",              "" },

    
    { SSC_OUTPUT,       SSC_NUMBER,     "T_loop_out_des_SS",                "Steady State loop design outlet temperature",                          "C",            "",          "Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "Q_loop_des_SS",                    "Steady State loop design thermal power",                               "MWt",          "",          "Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "therm_eff_loop_des_SS",            "Steady State loop optical efficiency",                                 "",             "",          "Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "eff_loop_des_SS",                  "Steady State loop total efficiency",                                   "",             "",          "Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "W_dot_pump_des_SS",                "Steady State field pumping power",                                     "MWe",          "",          "Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "Q_loss_receiver_des_SS",           "Steady State field heat loss from receiver",                           "MWt",          "",          "Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "Q_loss_hdr_rnr_des_SS",            "Steady State field heat loss from headers and runners",                "MWt",          "",          "Receiver",                       "*",                                                                "",              "" },
    
    
    // Collector and Receiver
    { SSC_OUTPUT,       SSC_NUMBER,     "DP_pressure_loss",                 "Total loop pressure loss at design",                                   "bar",          "",         "Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "avg_dt_des",                       "Average field temp difference at design",                              "C",            "",         "Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "hl_des",                           "Heat loss at design",                                                  "W/m",          "",         "Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "opt_derate",                       "Receiver optical derate",                                              "",             "",         "Receiver",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "opt_normal",                       "Collector optical loss at normal incidence",                           "",             "",         "Receiver",                       "*",                                                                "",              "" },
    
    // Power Cycle
    //{ SSC_OUTPUT,       SSC_NUMBER,     "q_dot_cycle_des",                  "PC thermal input at design",                                           "MWt",          "",         "Power Cycle",                              "*",                                                                "",              "" },
    //{ SSC_OUTPUT,       SSC_NUMBER,     "mdot_cycle_des",                   "PC thermal input at design",                                           "MWt",          "",         "Power Cycle",                              "*",                                                                "",              "" },
    
    // Thermal Storage
    { SSC_OUTPUT,       SSC_NUMBER,     "vol_tank",                         "Total tank volume",                                                    "m3",           "",         "Power Cycle",                              "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "Q_tes_des",                        "TES design capacity",                                                  "MWt-hr",       "",         "Power Cycle",                              "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "d_tank",                           "Tank diameter",                                                        "m",            "",         "Power Cycle",                              "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "vol_min",                          "Minimum Fluid Volume",                                                 "m3",           "",         "Power Cycle",                              "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "q_dot_loss_tes_des",               "Estimated TES Heat Loss",                                              "MW",           "",         "Power Cycle",                              "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "tes_htf_min_temp",                 "Minimum storage htf temp",                                             "C",            "",         "Power Cycle",                              "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "tes_htf_max_temp",                 "Maximum storage htf temp",                                             "C",            "",         "Power Cycle",                              "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "tes_htf_dens",                     "Storage htf density",                                                  "kg/m3",        "",         "Power Cycle",                              "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "tes_htf_cp",                       "Storage htf specific heat",                                            "kJ/kg-K",      "",         "Power Cycle",                              "*",                                                                "",              "" },
    
    // System Control
    { SSC_OUTPUT,       SSC_NUMBER,     "W_dot_bop_design",                 "BOP parasitics at design",                                             "MWe",          "",         "Power Cycle",                              "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "W_dot_fixed",                      "Fixed parasitic at design",                                            "MWe",          "",         "Power Cycle",                              "*",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "aux_design",                       "Aux parasitics at design",                                             "MWe",          "",         "System Control",                              "*",                                                                "",              "" },
    
    // Capital Costs
    
        // Direct Capital Costs
    { SSC_OUTPUT,       SSC_NUMBER,     "site_improvements_cost",           "Site improvements cost",                                               "$",          "",         "Capital Costs",                              "",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "solar_field_cost",                 "Solar field cost",                                                     "$",          "",         "Capital Costs",                              "",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "htf_system_cost",                  "HTF system cost",                                                      "$",          "",         "Capital Costs",                              "",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "ts_cost",                          "Thermal storage cost",                                                 "$",          "",         "Capital Costs",                              "",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "heat_sink_cost",                   "Heat sink cost",                                                       "$",          "",         "Capital Costs",                              "",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "bop_cost",                         "Balance of plant cost",                                                "$",          "",         "Capital Costs",                              "",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "contingency_cost",                 "Contingency cost",                                                     "$",          "",         "Capital Costs",                              "",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "total_direct_cost",                "Total direct cost",                                                    "$",          "",         "Capital Costs",                              "",                                                                "",              "" },
    
    // Indirect Capital Costs
    { SSC_OUTPUT,       SSC_NUMBER,     "epc_total_cost",                   "EPC total cost",                                                       "$",          "",         "Capital Costs",                              "",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "plm_total_cost",                   "Total land cost",                                                      "$",          "",         "Capital Costs",                              "",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "total_indirect_cost",              "Total direct cost",                                                    "$",          "",         "Capital Costs",                              "",                                                                "",              "" },
    
    // Sales Tax
    { SSC_OUTPUT,       SSC_NUMBER,     "sales_tax_total",                  "Sales tax total",                                                      "$",          "",         "Capital Costs",                              "",                                                                "",              "" },
    
    // Total Installed Costs
    { SSC_OUTPUT,       SSC_NUMBER,     "total_installed_cost",             "Total installed cost",                                                 "$",          "",         "Capital Costs",                              "",                                                                "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "installed_per_capacity",           "Estimated total installed cost per net capacity ($/kW)",               "$/kW",       "",         "Capital Costs",                              "",                                                                "",              "" },

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
    // Timeseries Simulation Outputs here (sim_type = 1):
    // ****************************************************************************************************************************************

    // Simulation Kernel
    { SSC_OUTPUT,       SSC_ARRAY,      "time_hr",                          "Time at end of timestep",                                              "hr",           "",         "solver",         "sim_type=1",                       "",                      "" },
    
    
    // Weather Reader
    { SSC_OUTPUT,       SSC_ARRAY,      "month",                            "Resource Month",                                                       "",             "",         "weather",        "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "hour_day",                         "Resource Hour of Day",                                                 "",             "",         "weather",        "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "solazi",                           "Resource Solar Azimuth",                                               "deg",          "",         "weather",        "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "solzen",                           "Resource Solar Zenith",                                                "deg",          "",         "weather",        "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "beam",                             "Resource Beam normal irradiance",                                      "W/m2",         "",         "weather",        "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "tdry",                             "Resource Dry bulb temperature",                                        "C",            "",         "weather",        "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "twet",                             "Resource Wet bulb temperature",                                        "C",            "",         "weather",        "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "rh",                               "Resource Relative Humidity",                                           "%",            "",         "weather",        "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "wspd",                             "Resource Wind Speed",                                                  "m/s",          "",         "weather",        "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "pres",                             "Resource Pressure",                                                    "mbar",         "",         "weather",        "sim_type=1",                       "",                      "" },
    
    { SSC_OUTPUT,       SSC_ARRAY,      "defocus",                          "Field optical focus fraction",                                         "",             "",         "weather",        "sim_type=1",                       "",                      "" },
    
    
    
    // Solar Field
    { SSC_OUTPUT,       SSC_ARRAY,      "EqOpteff",                         "Field optical efficiency before defocus",                              "",             "",         "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "SCAs_def",                         "Field fraction of focused SCAs",                                       "",             "",         "solar_field",    "sim_type=1",                       "",                      "" },
    
    { SSC_OUTPUT,       SSC_ARRAY,      "q_inc_sf_tot",                     "Field thermal power incident",                                         "MWt",          "",         "solar_field",    "*",                                "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "q_dot_rec_inc",                    "Receiver thermal power incident",                                      "MWt",          "",         "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "q_dot_rec_thermal_loss",           "Receiver thermal losses",                                              "MWt",          "",         "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "q_dot_rec_abs",                    "Receiver thermal power absorbed",                                      "MWt",          "",         "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "rec_thermal_eff",                  "Receiver thermal efficiency",                                          "",             "",         "solar_field",    "sim_type=1",                       "",                      "" },
    
    { SSC_OUTPUT,       SSC_ARRAY,      "q_dot_piping_loss",                "Field piping thermal losses",                                          "MWt",          "",         "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "e_dot_field_int_energy",           "Field change in material/htf internal energy",                         "MWt",          "",         "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "q_dot_htf_sf_out",                 "Field thermal power leaving in HTF",                                   "MWt",          "",         "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "q_dot_freeze_prot",                "Field freeze protection required",                                     "MWt",          "",         "solar_field",    "sim_type=1",                       "",                      "" },
    
    { SSC_OUTPUT,       SSC_ARRAY,      "m_dot_loop",                       "Receiver mass flow rate",                                              "kg/s",         "",         "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "m_dot_field_recirc",               "Field total mass flow recirculated",                                   "kg/s",         "",         "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "m_dot_field_delivered",            "Field total mass flow delivered",                                      "kg/s",         "",         "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "T_field_cold_in",                  "Field timestep-averaged inlet temperature",                            "C",            "",         "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "T_rec_cold_in",                    "Loop timestep-averaged inlet temperature",                             "C",            "",         "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "T_rec_hot_out",                    "Loop timestep-averaged outlet temperature",                            "C",            "",         "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "T_field_hot_out",                  "Field timestep-averaged outlet temperature",                           "C",            "",         "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "deltaP_field",                     "Field pressure drop",                                                  "bar",          "",         "solar_field",    "sim_type=1",                       "",                      "" },
    
    { SSC_OUTPUT,       SSC_ARRAY,      "W_dot_sca_track",                  "Field collector tracking power",                                       "MWe",          "",         "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "W_dot_field_pump",                 "Field htf pumping power",                                              "MWe",          "",         "solar_field",    "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "recirculating",                    "Field recirculating (bypass valve open)",                              "-",            "",         "solar_field",    "sim_type=1",                       "",                      "" },
    
    // power block
    //{ SSC_OUTPUT,       SSC_ARRAY,      "eta",                              "PC efficiency: gross",                                                 "",             "",         "powerblock",     "sim_type=1",                       "",                      "" },
    //{ SSC_OUTPUT,       SSC_ARRAY,      "q_pb",                             "PC input energy",                                                      "MWt",          "",         "powerblock",     "sim_type=1",                       "",                      "" },
    //{ SSC_OUTPUT,       SSC_ARRAY,      "m_dot_pc",                         "PC HTF mass flow rate",                                                "kg/s",         "",         "powerblock",     "sim_type=1",                       "",                      "" },
    //{ SSC_OUTPUT,       SSC_ARRAY,      "q_dot_pc_startup",                 "PC startup thermal power",                                             "MWt",          "",         "powerblock",     "sim_type=1",                       "",                      "" },
    //{ SSC_OUTPUT,       SSC_ARRAY,      "P_cycle",                          "PC electrical power output: gross",                                    "MWe",          "",         "powerblock",     "sim_type=1",                       "",                      "" },
    //{ SSC_OUTPUT,       SSC_ARRAY,      "T_pc_in",                          "PC HTF inlet temperature",                                             "C",            "",         "powerblock",     "sim_type=1",                       "",                      "" },
    //{ SSC_OUTPUT,       SSC_ARRAY,      "T_pc_out",                         "PC HTF outlet temperature",                                            "C",            "",         "powerblock",     "sim_type=1",                       "",                      "" },
    //{ SSC_OUTPUT,       SSC_ARRAY,      "m_dot_water_pc",                   "PC water consumption: makeup + cooling",                               "kg/s",         "",         "powerblock",     "sim_type=1",                       "",                      "" },
    //{ SSC_OUTPUT,       SSC_ARRAY,      "q_pc_startup",                     "PC startup thermal energy",                                            "MWht",         "",         "powerblock",     "sim_type=1",                       "",                      "" },
    //{ SSC_OUTPUT,       SSC_ARRAY,      "cycle_htf_pump_power",             "PC HTF pump power",                                                    "MWe",          "",         "powerblock",     "sim_type=1",                       "",                      "" },
    //{ SSC_OUTPUT,       SSC_ARRAY,      "P_cooling_tower_tot",              "Parasitic power condenser operation",                                  "MWe",          "",         "powerblock",     "sim_type=1",                       "",                      "" },

    // Heat Sink
    { SSC_OUTPUT,       SSC_ARRAY,      "q_dot_to_heat_sink",               "Heat sink thermal power",                                              "MWt",          "",         "Heat_Sink",      "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "W_dot_pc_pump",                    "Heat sink pumping power",                                              "MWe",          "",         "Heat_Sink",      "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "m_dot_htf_heat_sink",              "Heat sink HTF mass flow",                                              "kg/s",         "",         "Heat_Sink",      "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "T_heat_sink_in",                   "Heat sink HTF inlet temp",                                             "C",            "",         "Heat_Sink",      "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "T_heat_sink_out",                  "Heat sink HTF outlet temp",                                            "C",            "",         "Heat_Sink",      "sim_type=1",                       "",                      "" },
                                                                                                                                                                                                                                      
                                                                                                                                                                                                                                      
    // TES                                                                                                                                                                                                                            
    { SSC_OUTPUT,       SSC_ARRAY,      "tank_losses",                      "TES thermal losses",                                                   "MWt",          "",         "TES",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "q_tes_heater",                     "TES freeze protection power",                                          "MWt",          "",         "TES",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "T_tes_hot",                        "TES hot temperature",                                                  "C",            "",         "TES",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "T_tes_cold",                       "TES cold temperature",                                                 "C",            "",         "TES",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "mass_tes_cold",                    "TES cold tank mass (end)",                                             "kg",           "",         "TES",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "mass_tes_hot",                     "TES hot tank mass (end)",                                              "kg",           "",         "TES",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "q_dc_tes",                         "TES discharge thermal power",                                          "MWt",          "",         "TES",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "q_ch_tes",                         "TES charge thermal power",                                             "MWt",          "",         "TES",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "e_ch_tes",                         "TES charge state",                                                     "MWht",         "",         "TES",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "m_dot_cr_to_tes_hot",              "Mass flow: field to hot TES",                                          "kg/s",         "",         "TES",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "m_dot_tes_hot_out",                "Mass flow: TES hot out",                                               "kg/s",         "",         "TES",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "m_dot_pc_to_tes_cold",             "Mass flow: cycle to cold TES",                                         "kg/s",         "",         "TES",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "m_dot_tes_cold_out",               "Mass flow: TES cold out",                                              "kg/s",         "",         "TES",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "m_dot_field_to_cycle",             "Mass flow: field to cycle",                                            "kg/s",         "",         "TES",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "m_dot_cycle_to_field",             "Mass flow: cycle to field",                                            "kg/s",         "",         "TES",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "m_dot_cold_tank_to_hot_tank",      "Mass flow: cold tank to hot tank",                                     "kg/s",         "",         "TES",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "tes_htf_pump_power",               "TES HTF pump power",                                                   "MWe",          "",         "TES",            "sim_type=1",                       "",                      "" },

    { SSC_OUTPUT,       SSC_ARRAY,      "pipe_tes_diams",                   "Pipe diameters in TES",                                                "m",            "",         "TES",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "pipe_tes_wallthk",                 "Pipe wall thickness in TES",                                           "m",            "",         "TES",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "pipe_tes_lengths",                 "Pipe lengths in TES",                                                  "m",            "",         "TES",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "pipe_tes_mdot_dsn",                "Mass flow TES pipes at design conditions",                             "kg/s",         "",         "TES",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "pipe_tes_vel_dsn",                 "Velocity in TES pipes at design conditions",                           "m/s",          "",         "TES",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "pipe_tes_T_dsn",                   "Temperature in TES pipes at design conditions",                        "C",            "",         "TES",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "pipe_tes_P_dsn",                   "Pressure in TES pipes at design conditions",                           "bar",          "",         "TES",            "sim_type=1",                       "",                      "" },


    // Parasitics outputs
    { SSC_OUTPUT,       SSC_ARRAY,      "P_fixed",                          "Parasitic power fixed load",                                           "MWe",          "",         "system",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "P_plant_balance_tot",              "Parasitic power generation-dependent load",                            "MWe",          "",         "system",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "W_dot_parasitic_tot",              "System total electrical parasitic",                                    "MWe",          "",         "system",         "sim_type=1",                       "",                      "" },


    // Controller
    { SSC_OUTPUT,       SSC_ARRAY,      "tou_value",                        "CSP operating Time-of-use value",                                      "",             "",         "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "pricing_mult",                     "PPA price multiplier",                                                 "",             "",         "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "n_op_modes",                       "Operating modes in reporting timestep",                                "",             "",         "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "op_mode_1",                        "1st operating mode",                                                   "",             "",         "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "op_mode_2",                        "2nd op. mode, if applicable",                                          "",             "",         "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "op_mode_3",                        "3rd op. mode, if applicable",                                          "",             "",         "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "m_dot_balance",                    "Relative mass flow balance error",                                     "",             "",         "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "q_balance",                        "Relative energy balance error",                                        "",             "",         "solver",         "sim_type=1",                       "",                      "" },

    // Dispatch outputs
    { SSC_OUTPUT,       SSC_ARRAY,      "disp_rel_mip_gap",                 "Dispatch relative MIP gap",                                            "",             "",         "tou",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "disp_solve_state",                 "Dispatch solver state",                                                "",             "",         "tou",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "disp_subopt_flag",                 "Dispatch suboptimal solution flag",                                    "",             "",         "tou",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "disp_solve_iter",                  "Dispatch iterations count",                                            "",             "",         "tou",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "disp_objective",                   "Dispatch objective function value",                                    "",             "",         "tou",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "disp_obj_relax",                   "Dispatch objective function - relaxed max",                            "",             "",         "tou",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "disp_qsf_expected",                "Dispatch expected solar field available energy",                       "MWt",          "",         "tou",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "disp_qsfprod_expected",            "Dispatch expected solar field generation",                             "MWt",          "",         "tou",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "disp_qsfsu_expected",              "Dispatch expected solar field startup enegy",                          "MWt",          "",         "tou",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "disp_tes_expected",                "Dispatch expected TES charge level",                                   "MWht",         "",         "tou",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "disp_pceff_expected",              "Dispatch expected power cycle efficiency adj.",                        "",             "",         "tou",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "disp_thermeff_expected",           "Dispatch expected SF thermal efficiency adj.",                         "",             "",         "tou",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "disp_qpbsu_expected",              "Dispatch expected power cycle startup energy",                         "MWht",         "",         "tou",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "disp_wpb_expected",                "Dispatch expected power generation",                                   "MWe",          "",         "tou",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "disp_rev_expected",                "Dispatch expected revenue factor",                                     "",             "",         "tou",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "disp_presolve_nconstr",            "Dispatch number of constraints in problem",                            "",             "",         "tou",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "disp_presolve_nvar",               "Dispatch number of variables in problem",                              "",             "",         "tou",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "disp_solve_time",                  "Dispatch solver time",                                                 "sec",          "",         "tou",            "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "avg_suboptimal_rel_mip_gap",       "Average suboptimal relative MIP gap",                                  "%",            "",         "tou",            "sim_type=1",                       "",                      "" },


    // These outputs correspond to the first csp-solver timestep in the reporting timestep.
    //     Subsequent csp-solver timesteps within the same reporting timestep are not tracked
    { SSC_OUTPUT,       SSC_ARRAY,      "q_dot_pc_sb",                      "Thermal power for PC standby",                                         "MWt",          "",         "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "q_dot_pc_min",                     "Thermal power for PC min operation",                                   "MWt",          "",         "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "q_dot_pc_max",                     "Max thermal power to PC",                                              "MWt",          "",         "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "q_dot_pc_target",                  "Target thermal power to PC",                                           "MWt",          "",         "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "is_rec_su_allowed",                "is receiver startup allowed",                                          "",             "",         "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "is_pc_su_allowed",                 "is power cycle startup allowed",                                       "",             "",         "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "is_pc_sb_allowed",                 "is power cycle standby allowed",                                       "",             "",         "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "q_dot_est_cr_su",                  "Estimate rec. startup thermal power",                                  "MWt",          "",         "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "q_dot_est_cr_on",                  "Estimate rec. thermal power TO HTF",                                   "MWt",          "",         "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "q_dot_est_tes_dc",                 "Estimate max TES discharge thermal power",                             "MWt",          "",         "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "q_dot_est_tes_ch",                 "Estimate max TES charge thermal power",                                "MWt",          "",         "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "operating_modes_a",                "First 3 operating modes tried",                                        "",             "",         "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "operating_modes_b",                "Next 3 operating modes tried",                                         "",             "",         "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "operating_modes_c",                "Final 3 operating modes tried",                                        "",             "",         "solver",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,      "gen",                              "Total thermal power to heat sink with available derate",               "kWe",          "",         "system",         "sim_type=1",                       "",                      "" },

    // Monthly Outputs
    { SSC_OUTPUT,       SSC_ARRAY,      "monthly_energy",                   "Monthly AC energy in Year 1",                                                       "kWh",          "",         "Post-process",   "sim_type=1",              "LENGTH=12",                      "" },

    // Annual Outputs
    { SSC_OUTPUT,       SSC_NUMBER,     "annual_energy",                    "Annual net electricity production with availability derate",              "kWe-hr",       "",         "Post-process",   "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "annual_thermal_consumption",       "Annual thermal freeze protection required",                            "kWt-hr",       "",         "Post-process",   "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "annual_electricity_consumption",   "Annual electricity consumption with availability derate",                       "kWe-hr",       "",         "Post-process",   "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "annual_total_water_use",           "Total Annual Water Usage",                                             "m^3",          "",         "Post-process",   "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "annual_field_freeze_protection",   "Annual thermal power for field freeze protection",                     "kWt-hr",       "",         "Post-process",   "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "annual_tes_freeze_protection",     "Annual thermal power for TES freeze protection",                       "kWt-hr",       "",         "Post-process",   "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "capacity_factor",                  "Capacity factor",                                                      "%",            "",         "system",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "kwh_per_kw",                       "First year kWh/kW",                                                    "kWh/kW",       "",         "system",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "sim_duration",                     "Computational time of timeseries simulation",                          "s",            "",         "system",         "sim_type=1",                       "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "W_dot_par_tot_haf",                "Adjusted parasitic power",                                             "kWe",          "",         "system",         "",                       "",                      "" },
    
    
    
    

    var_info_invalid };

class cm_fresnel_physical_iph : public compute_module
{
public:

    cm_fresnel_physical_iph()
    {
        add_var_info(_cm_vtab_fresnel_physical_iph);
        add_var_info(vtab_adjustment_factors);
        add_var_info(vtab_technology_outputs);
    }

    void exec()
    {
        // Uncomment following 2 lines to write cmod inputs to LK script
        //FILE* fp = fopen("fresnel_iph_cmod_to_lk.lk", "w");
        //write_cmod_to_lk_script(fp, m_vartab);

        // Common Parameters
        bool is_dispatch = as_boolean("is_dispatch");
        int sim_type = as_number("sim_type");
        double T_htf_cold_des = as_double("T_loop_in_des");    //[C]
        double T_htf_hot_des = as_double("T_loop_out");      //[C]
        double tshours = as_double("tshours");                  //[-]
        double q_dot_pc_des = as_double("q_pb_design");         //[MWt] HEAT SINK design thermal power
        double Q_tes = q_dot_pc_des * tshours;                  //[MWt-hr]

        // Weather reader
        C_csp_weatherreader weather_reader;
        C_csp_solver::S_sim_setup sim_setup;
        int n_steps_fixed;
        int steps_per_hour;
        {
            weather_reader.m_weather_data_provider = std::make_shared<weatherfile>(as_string("file_name"));
            weather_reader.m_filename = as_string("file_name");
            weather_reader.m_trackmode = 0;
            weather_reader.m_tilt = 0.0;
            weather_reader.m_azimuth = 0.0;
            // Initialize to get weather file info
            weather_reader.init();
            if (weather_reader.has_error()) throw exec_error("fresnel_physical", weather_reader.get_error());

            // Set up ssc output arrays
            // Set steps per hour
            double nhourssim = 8760.0;                                  //[hr] Number of hours to simulate

            sim_setup.m_sim_time_start = 0.0;                           //[s] starting first hour of year
            sim_setup.m_sim_time_end = nhourssim * 3600.;                 //[s] full year simulation

            steps_per_hour = 1;                                     //[-]

            int n_wf_records = (int)weather_reader.m_weather_data_provider->nrecords();
            steps_per_hour = n_wf_records / 8760;                       //[-]

            n_steps_fixed = steps_per_hour * 8760;                    //[-]
            sim_setup.m_report_step = 3600.0 / (double)steps_per_hour;  //[s]
        }

        // Solar field
        C_csp_fresnel_collector_receiver c_fresnel;
        {
            // Inputs
            {
                c_fresnel.m_solar_mult_or_Ap = as_integer("solar_mult_or_Ap");
                c_fresnel.m_solar_mult_in = as_double("solar_mult_in");
                c_fresnel.m_total_Ap_in = as_double("total_Ap_in");

                c_fresnel.m_nMod = as_integer("nMod");
                c_fresnel.m_nRecVar = as_integer("nRecVar");

                c_fresnel.m_eta_pump = as_number("eta_pump");
                c_fresnel.m_HDR_rough = as_number("HDR_rough");
                c_fresnel.m_theta_stow = as_number("theta_stow");
                c_fresnel.m_theta_dep = as_number("theta_dep");
                c_fresnel.m_FieldConfig = as_integer("FieldConfig");
                c_fresnel.m_T_startup = as_number("T_startup");

                // Set P_ref = q_pb_design and eta_ref = 1 
                c_fresnel.m_P_ref = as_double("q_pb_design") * 1e6;
                c_fresnel.m_eta_ref = 1;

                c_fresnel.m_use_abs_or_rel_mdot_limit = as_integer("use_abs_or_rel_mdot_limit"); // Use mass flow abs (0) or relative (1) limits
                c_fresnel.m_m_dot_htfmin_in = as_double("m_dot_htfmin");        //[kg/s] Minimum loop HTF flow rate
                c_fresnel.m_m_dot_htfmax_in = as_double("m_dot_htfmax");        //[kg/s] Maximum loop HTF flow rate
                c_fresnel.m_f_htfmin_in = as_double("f_htfmin");                //[] Minimum loop htf flow rate fraction
                c_fresnel.m_f_htfmax_in = as_double("f_htfmax");                //[] Maximum loop htf flow rate fraction


                c_fresnel.m_T_loop_in_des = as_number("T_loop_in_des");
                c_fresnel.m_T_loop_out_des = as_number("T_loop_out");
                c_fresnel.m_Fluid = as_integer("Fluid");

                c_fresnel.m_field_fl_props = as_matrix("field_fl_props");
                c_fresnel.m_T_fp = as_number("T_fp");
                c_fresnel.m_I_bn_des = as_number("I_bn_des");
                c_fresnel.m_V_hdr_max = as_number("V_hdr_max");
                c_fresnel.m_V_hdr_min = as_number("V_hdr_min");
                c_fresnel.m_Pipe_hl_coef = as_number("Pipe_hl_coef");
                c_fresnel.m_SCA_drives_elec = as_number("SCA_drives_elec");
                c_fresnel.m_ColAz = as_number("ColAz");

                c_fresnel.m_mc_bal_hot = as_number("mc_bal_hot");
                c_fresnel.m_mc_bal_cold = as_number("mc_bal_cold");
                c_fresnel.m_mc_bal_sca = as_number("mc_bal_sca");

                c_fresnel.m_opt_model = as_integer("opt_model");

                c_fresnel.m_A_aperture = as_number("A_aperture");
                c_fresnel.m_reflectivity = as_number("reflectivity");
                c_fresnel.m_TrackingError = as_number("TrackingError");
                c_fresnel.m_GeomEffects = as_number("GeomEffects");
                c_fresnel.m_Dirt_mirror = as_number("Dirt_mirror");
                c_fresnel.m_Error = as_number("Error");
                c_fresnel.m_L_mod = as_number("L_mod");

                c_fresnel.m_IAM_T_coefs = as_vector_double("IAM_T_coefs");
                c_fresnel.m_IAM_L_coefs = as_vector_double("IAM_L_coefs");
                c_fresnel.m_OpticalTable = as_matrix("OpticalTable");
                c_fresnel.m_rec_model = as_integer("rec_model");

                c_fresnel.m_HCE_FieldFrac = as_vector_double("HCE_FieldFrac");
                c_fresnel.m_D_abs_in = as_vector_double("D_abs_in");
                c_fresnel.m_D_abs_out = as_vector_double("D_abs_out");
                c_fresnel.m_D_glass_in = as_vector_double("D_glass_in");
                c_fresnel.m_D_glass_out = as_vector_double("D_glass_out");
                c_fresnel.m_D_plug = as_vector_double("D_plug");
                c_fresnel.m_Flow_type = as_vector_double("Flow_type");
                c_fresnel.m_Rough = as_vector_double("Rough");
                c_fresnel.m_alpha_env = as_vector_double("alpha_env");

                c_fresnel.m_epsilon_abs_1 = as_matrix_transpose("epsilon_abs_1");
                c_fresnel.m_epsilon_abs_2 = as_matrix_transpose("epsilon_abs_2");
                c_fresnel.m_epsilon_abs_3 = as_matrix_transpose("epsilon_abs_3");
                c_fresnel.m_epsilon_abs_4 = as_matrix_transpose("epsilon_abs_4");

                c_fresnel.m_alpha_abs = as_vector_double("alpha_abs");
                c_fresnel.m_Tau_envelope = as_vector_double("Tau_envelope");
                c_fresnel.m_epsilon_glass = as_vector_double("epsilon_glass");
                c_fresnel.m_GlazingIntact = as_vector_bool("GlazingIntactIn");

                c_fresnel.m_P_a = as_vector_double("P_a");

                c_fresnel.m_AnnulusGas = as_vector_double("AnnulusGas");
                c_fresnel.m_AbsorberMaterial = as_vector_double("AbsorberMaterial");
                c_fresnel.m_Shadowing = as_vector_double("Shadowing");
                c_fresnel.m_dirt_env = as_vector_double("dirt_env");
                c_fresnel.m_Design_loss = as_vector_double("Design_loss");

                c_fresnel.m_L_mod_spacing = as_number("L_mod_spacing");
                c_fresnel.m_L_crossover = as_number("L_crossover");
                c_fresnel.m_HL_T_coefs = as_vector_double("HL_T_coefs");
                c_fresnel.m_HL_w_coefs = as_vector_double("HL_w_coefs");

                c_fresnel.m_DP_nominal = as_number("DP_nominal");
                c_fresnel.m_DP_coefs = as_vector_double("DP_coefs");
                c_fresnel.m_rec_htf_vol = as_number("rec_htf_vol");

                c_fresnel.m_L_rnr_pb = as_number("L_rnr_pb"); // No power block line length
                c_fresnel.m_rec_su_delay = as_number("rec_su_delay");
                c_fresnel.m_rec_qf_delay = as_number("rec_qf_delay");
                c_fresnel.m_p_start = as_number("p_start");

                c_fresnel.m_V_wind_des = as_number("V_wind_des");
                c_fresnel.m_T_amb_sf_des = as_number("T_amb_sf_des");
            }

            // Calculate solar multiple (needed for other component constructors)
            // Need latitude from weather reader
            weather_reader.init();
            c_fresnel.design_solar_mult(weather_reader.ms_solved_params.m_lat);

            // Allocate Outputs
            {
                c_fresnel.mc_reported_outputs.assign(C_csp_fresnel_collector_receiver::E_EQUIV_OPT_ETA_TOT, allocate("EqOpteff", n_steps_fixed), n_steps_fixed);
                c_fresnel.mc_reported_outputs.assign(C_csp_fresnel_collector_receiver::E_DEFOCUS, allocate("SCAs_def", n_steps_fixed), n_steps_fixed);

                c_fresnel.mc_reported_outputs.assign(C_csp_fresnel_collector_receiver::E_Q_DOT_INC_SF_TOT, allocate("q_inc_sf_tot", n_steps_fixed), n_steps_fixed);
                c_fresnel.mc_reported_outputs.assign(C_csp_fresnel_collector_receiver::E_Q_DOT_INC_SF_COSTH, allocate("qinc_costh", n_steps_fixed), n_steps_fixed);
                c_fresnel.mc_reported_outputs.assign(C_csp_fresnel_collector_receiver::E_Q_DOT_REC_INC, allocate("q_dot_rec_inc", n_steps_fixed), n_steps_fixed);
                c_fresnel.mc_reported_outputs.assign(C_csp_fresnel_collector_receiver::E_Q_DOT_REC_THERMAL_LOSS, allocate("q_dot_rec_thermal_loss", n_steps_fixed), n_steps_fixed);
                c_fresnel.mc_reported_outputs.assign(C_csp_fresnel_collector_receiver::E_Q_DOT_REC_ABS, allocate("q_dot_rec_abs", n_steps_fixed), n_steps_fixed);
                c_fresnel.mc_reported_outputs.assign(C_csp_fresnel_collector_receiver::E_REC_THERMAL_EFF, allocate("rec_thermal_eff", n_steps_fixed), n_steps_fixed);
                c_fresnel.mc_reported_outputs.assign(C_csp_fresnel_collector_receiver::E_Q_DOT_PIPING_LOSS, allocate("q_dot_piping_loss", n_steps_fixed), n_steps_fixed);
                c_fresnel.mc_reported_outputs.assign(C_csp_fresnel_collector_receiver::E_E_DOT_INTERNAL_ENERGY, allocate("e_dot_field_int_energy", n_steps_fixed), n_steps_fixed);
                c_fresnel.mc_reported_outputs.assign(C_csp_fresnel_collector_receiver::E_Q_DOT_HTF_OUT, allocate("q_dot_htf_sf_out", n_steps_fixed), n_steps_fixed);
                c_fresnel.mc_reported_outputs.assign(C_csp_fresnel_collector_receiver::E_Q_DOT_FREEZE_PROT, allocate("q_dot_freeze_prot", n_steps_fixed), n_steps_fixed);

                c_fresnel.mc_reported_outputs.assign(C_csp_fresnel_collector_receiver::E_M_DOT_LOOP, allocate("m_dot_loop", n_steps_fixed), n_steps_fixed);
                c_fresnel.mc_reported_outputs.assign(C_csp_fresnel_collector_receiver::E_IS_RECIRCULATING, allocate("recirculating", n_steps_fixed), n_steps_fixed);
                c_fresnel.mc_reported_outputs.assign(C_csp_fresnel_collector_receiver::E_M_DOT_FIELD_RECIRC, allocate("m_dot_field_recirc", n_steps_fixed), n_steps_fixed);
                c_fresnel.mc_reported_outputs.assign(C_csp_fresnel_collector_receiver::E_M_DOT_FIELD_DELIVERED, allocate("m_dot_field_delivered", n_steps_fixed), n_steps_fixed);
                c_fresnel.mc_reported_outputs.assign(C_csp_fresnel_collector_receiver::E_T_FIELD_COLD_IN, allocate("T_field_cold_in", n_steps_fixed), n_steps_fixed);
                c_fresnel.mc_reported_outputs.assign(C_csp_fresnel_collector_receiver::E_T_REC_COLD_IN, allocate("T_rec_cold_in", n_steps_fixed), n_steps_fixed);
                c_fresnel.mc_reported_outputs.assign(C_csp_fresnel_collector_receiver::E_T_REC_HOT_OUT, allocate("T_rec_hot_out", n_steps_fixed), n_steps_fixed);
                c_fresnel.mc_reported_outputs.assign(C_csp_fresnel_collector_receiver::E_T_FIELD_HOT_OUT, allocate("T_field_hot_out", n_steps_fixed), n_steps_fixed);
                c_fresnel.mc_reported_outputs.assign(C_csp_fresnel_collector_receiver::E_PRESSURE_DROP, allocate("deltaP_field", n_steps_fixed), n_steps_fixed);          //[bar]

                c_fresnel.mc_reported_outputs.assign(C_csp_fresnel_collector_receiver::E_W_DOT_SCA_TRACK, allocate("W_dot_sca_track", n_steps_fixed), n_steps_fixed);     //[MWe]
                c_fresnel.mc_reported_outputs.assign(C_csp_fresnel_collector_receiver::E_W_DOT_PUMP, allocate("W_dot_field_pump", n_steps_fixed), n_steps_fixed);         //[MWe]
            }

        }

        // Heat Sink
        C_pc_heat_sink c_heat_sink;
        {
            size_t n_f_turbine1 = 0;
            ssc_number_t* p_f_turbine1 = as_array("f_turb_tou_periods", &n_f_turbine1);   // heat sink, not turbine
            double f_turbine_max1 = 1.0;
            for (size_t i = 0; i < n_f_turbine1; i++) {
                f_turbine_max1 = max(f_turbine_max1, p_f_turbine1[i]);
            }

            c_heat_sink.ms_params.m_T_htf_hot_des = T_htf_hot_des;		//[C] FIELD design outlet temperature
            c_heat_sink.ms_params.m_T_htf_cold_des = T_htf_cold_des;	//[C] FIELD design inlet temperature
            c_heat_sink.ms_params.m_q_dot_des = q_dot_pc_des;			//[MWt] HEAT SINK design thermal power (could have field solar multiple...)
            // 9.18.2016 twn: assume for now there's no pressure drop though heat sink
            c_heat_sink.ms_params.m_htf_pump_coef = as_double("pb_pump_coef");		//[kWe/kg/s]
            c_heat_sink.ms_params.m_max_frac = f_turbine_max1;

            c_heat_sink.ms_params.m_pc_fl = as_integer("Fluid");
            c_heat_sink.ms_params.m_pc_fl_props = as_matrix("field_fl_props");


            // Allocate heat sink outputs
            c_heat_sink.mc_reported_outputs.assign(C_pc_heat_sink::E_Q_DOT_HEAT_SINK, allocate("q_dot_to_heat_sink", n_steps_fixed), n_steps_fixed);
            c_heat_sink.mc_reported_outputs.assign(C_pc_heat_sink::E_W_DOT_PUMPING, allocate("W_dot_pc_pump", n_steps_fixed), n_steps_fixed);
            c_heat_sink.mc_reported_outputs.assign(C_pc_heat_sink::E_M_DOT_HTF, allocate("m_dot_htf_heat_sink", n_steps_fixed), n_steps_fixed);
            c_heat_sink.mc_reported_outputs.assign(C_pc_heat_sink::E_T_HTF_IN, allocate("T_heat_sink_in", n_steps_fixed), n_steps_fixed);
            c_heat_sink.mc_reported_outputs.assign(C_pc_heat_sink::E_T_HTF_OUT, allocate("T_heat_sink_out", n_steps_fixed), n_steps_fixed);
        }
        
        // TES
        C_csp_two_tank_tes storage;
        {
            double V_tes_des = as_double("V_tes_des");

            storage = C_csp_two_tank_tes(
                as_integer("Fluid"),
                as_matrix("field_fl_props"),
                as_integer("store_fluid"),
                as_matrix("store_fl_props"),
                q_dot_pc_des,
                c_fresnel.m_solar_mult,
                Q_tes,
                as_double("h_tank"),
                as_double("u_tank"),
                as_integer("tank_pairs"),
                as_double("hot_tank_Thtr"),
                as_double("hot_tank_max_heat"),
                as_double("cold_tank_Thtr"),
                as_double("cold_tank_max_heat"),
                as_double("dt_hot"),
                as_double("T_loop_in_des"),
                as_double("T_loop_out"),
                as_double("T_loop_out"),
                as_double("T_loop_in_des"),
                as_double("h_tank_min"),
                as_double("init_hot_htf_percent"),
                as_double("pb_pump_coef"),
                as_boolean("tanks_in_parallel"),
                1.0,                    // packed volume fraction
                V_tes_des,
                false,
                as_double("tes_pump_coef")
            );


            // Set storage outputs
            int n_wf_records = (int)weather_reader.m_weather_data_provider->nrecords();
            double steps_per_hour = n_wf_records / 8760;
            int n_steps_fixed = steps_per_hour * 8760;
            storage.mc_reported_outputs.assign(C_csp_two_tank_tes::E_Q_DOT_LOSS, allocate("tank_losses", n_steps_fixed), n_steps_fixed);
            storage.mc_reported_outputs.assign(C_csp_two_tank_tes::E_W_DOT_HEATER, allocate("q_tes_heater", n_steps_fixed), n_steps_fixed);
            storage.mc_reported_outputs.assign(C_csp_two_tank_tes::E_TES_T_HOT, allocate("T_tes_hot", n_steps_fixed), n_steps_fixed);
            storage.mc_reported_outputs.assign(C_csp_two_tank_tes::E_TES_T_COLD, allocate("T_tes_cold", n_steps_fixed), n_steps_fixed);
            storage.mc_reported_outputs.assign(C_csp_two_tank_tes::E_M_DOT_TANK_TO_TANK, allocate("m_dot_cold_tank_to_hot_tank", n_steps_fixed), n_steps_fixed);
            storage.mc_reported_outputs.assign(C_csp_two_tank_tes::E_MASS_COLD_TANK, allocate("mass_tes_cold", n_steps_fixed), n_steps_fixed);
            storage.mc_reported_outputs.assign(C_csp_two_tank_tes::E_MASS_HOT_TANK, allocate("mass_tes_hot", n_steps_fixed), n_steps_fixed);
            storage.mc_reported_outputs.assign(C_csp_two_tank_tes::E_W_DOT_HTF_PUMP, allocate("tes_htf_pump_power", n_steps_fixed), n_steps_fixed);

        }

        // *******************************************
        // Schedules

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
        int csp_financial_model = as_integer("csp_financial_model");
        C_timeseries_schedule_inputs elec_pricing_schedule;

        if (sim_type == 1) {
            if (csp_financial_model == 8 || csp_financial_model == 7) {        // No Financial Model or LCOH; Single Owner in progress 9/2023
                if (is_dispatch) {
                    throw exec_error("fresnel_physical_iph", "Can't select dispatch optimization if No Financial model");
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
                    throw exec_error("fresnel_physical_iph", "\n\nYou selected dispatch optimization which requires that the array input ppa_price_input is defined\n");
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
                    throw exec_error("fresnel_physical_iph", "\n\nYou selected dispatch optimization and the Specify IRR Target financial solution mode, "
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
                        elec_pricing_schedule = C_timeseries_schedule_inputs(-1.0, std::numeric_limits<double>::quiet_NaN());
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
            else {
                throw exec_error("fresnel_physical_iph", "csp_financial_model must be 1, 7, or 8");
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

        // TOU
        C_csp_tou tou(offtaker_schedule, elec_pricing_schedule, dispatch_model_type, is_offtaker_frac_also_max);

        //tou.mc_dispatch_params.m_is_tod_pc_target_also_pc_max = as_boolean("is_tod_pc_target_also_pc_max");
        //tou.mc_dispatch_params.m_is_block_dispatch = !is_dispatch;      //mw

        // System Parameters
        C_csp_solver::S_csp_system_params system;
        {
            system.m_pb_fixed_par = as_double("pb_fixed_par");
            size_t nval_bop_array = 0;
            ssc_number_t* bop_array = as_array("bop_array", &nval_bop_array);
            if (nval_bop_array != 5) throw exec_error("fresnel_physical", "Should be 5 elements in bop_array, has " + util::to_string((int)nval_bop_array) + ".");
            system.m_bop_par = bop_array[0];        //as_double("bop_par");
            system.m_bop_par_f = bop_array[1];      //as_double("bop_par_f");
            system.m_bop_par_0 = bop_array[2];      //as_double("bop_par_0");
            system.m_bop_par_1 = bop_array[3];      //as_double("bop_par_1");
            system.m_bop_par_2 = bop_array[4];      //as_double("bop_par_2");
        }

        // System Dispatch
        csp_dispatch_opt dispatch;

        if (is_dispatch) {

            double heater_startup_cost = 0.0;

            double q_dot_rec_des = q_dot_pc_des * c_fresnel.m_solar_mult; //[MWt]

            dispatch.solver_params.set_user_inputs(is_dispatch, steps_per_hour, as_integer("disp_frequency"), as_integer("disp_horizon"),
                as_integer("disp_max_iter"), as_double("disp_mip_gap"), as_double("disp_timeout"),
                as_integer("disp_spec_presolve"), as_integer("disp_spec_bb"), as_integer("disp_spec_scaling"), as_integer("disp_reporting"),
                as_boolean("is_write_ampl_dat"), as_boolean("is_ampl_engine"), as_string("ampl_data_dir"), as_string("ampl_exec_call"));

            bool can_cycle_use_standby = false;
            double disp_csu_cost_calc = 0.0;
            double disp_pen_ramping = 0.0;

            double disp_rsu_cost_calc = as_double("disp_rsu_cost_rel") * q_dot_rec_des;   //[$/start]
            dispatch.params.set_user_params(can_cycle_use_standby, as_double("disp_time_weighting"),
                disp_rsu_cost_calc, heater_startup_cost, disp_csu_cost_calc, disp_pen_ramping,
                as_double("disp_inventory_incentive"), as_double("q_rec_standby"), as_double("q_rec_heattrace")); // , ppa_price_year1);
        }
        else {
            dispatch.solver_params.dispatch_optimize = false;
        }

        // Instantiate Solver
        C_csp_solver csp_solver(weather_reader,
            c_fresnel,
            c_heat_sink,
            storage,
            tou,
            dispatch,
            system,
            NULL,
            nullptr,
            ssc_cmod_update,
            (void*)(this));

        // Set solver reporting outputs
        {
            // Simulation Kernel
            csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::TIME_FINAL, allocate("time_hr", n_steps_fixed), n_steps_fixed);
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
        }

        update("Initialize physical fresnel model...", 0.0);

        // Initialize Solver
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

            throw exec_error("fresnel_physical", csp_exception.m_error_message);
        }

        // If no exception, then report messages
        while (csp_solver.mc_csp_messages.get_message(&out_type, &out_msg))
        {
            log(out_msg, out_type);
        }

        // Design point is complete, assign technology design outputs
        double total_land_area = std::numeric_limits<double>::quiet_NaN();
        double nameplate = std::numeric_limits<double>::quiet_NaN();
        {
            // System Design Calcs
            //double eta_ref = as_double("eta_ref");                          //[-]
            //double W_dot_cycle_des = as_double("P_ref");                    //[MWe]
            double tshours = as_double("tshours");                          //[-]
            double solar_mult_des = c_fresnel.m_solar_mult;
            double q_pb_design = as_double("q_pb_design");

            double q_dot_pc_des = q_pb_design;               //[MWt]
            Q_tes = q_dot_pc_des * tshours;                          //[MWt-hr]

            double mdot_field_des = c_fresnel.m_m_dot_design;          // [kg/s]

            double avg_T_des = (c_fresnel.m_T_loop_in_des + c_fresnel.m_T_loop_out_des) / 2.0;

            // Solar Field

            double W_dot_col_tracking_des = c_fresnel.get_tracking_power();                 // [MWe]
            double A_loop = c_fresnel.m_A_loop;                                             // [m2]
            double loop_opt_eff = c_fresnel.m_loop_opt_eff;
            double loop_therm_eff = c_fresnel.m_loop_therm_eff;
            double loop_eff = c_fresnel.m_loop_eff;
            double sm1_aperture = c_fresnel.m_Ap_sm1;                                       // [m2]
            double sm1_nLoops = c_fresnel.m_nLoops_sm1;
            double total_tracking_power = c_fresnel.m_W_dot_sca_tracking_nom;               // [MW]
            double A_field = c_fresnel.m_Ap_tot;                                            // [m2]
            double q_field_des = c_fresnel.m_q_design_actual / 1e6;                         // [MW]
            double q_field_des_ideal = c_fresnel.m_q_design_ideal / 1e6;                    // [MW]

            double field_area = A_field / 4046.85642;                                       // [acres] (convert m2 to acre)
            double land_mult = as_double("land_mult");
            total_land_area = field_area * land_mult;                                // [acres]

            double field_htf_min_temp = c_fresnel.m_htfProps.min_temp() - 273.15;           // [C]
            double field_htf_max_temp = c_fresnel.m_htfProps.max_temp() - 273.15;           // [C]

            // steady state results
            double dP_field_des_SS = c_fresnel.m_dP_des_SS;                                 // [bar]
            double Q_field_des_SS = c_fresnel.m_Q_field_des_SS / 1e6;                       // [MW]
            double T_field_out_des_SS = c_fresnel.m_T_field_out_des_SS;                     // [C]
            double m_dot_des_SS = c_fresnel.m_m_dot_des_SS;                                 // [kg/s]
            double m_dot_loop_des_SS = c_fresnel.m_m_dot_loop_des_SS;                       // [kg/s]
            double V_hdr_min_des_SS = c_fresnel.m_V_hdr_min_des_SS;                         // [m/s]
            double V_hdr_max_des_SS = c_fresnel.m_V_hdr_max_des_SS;                         // [m/s]
            double eta_optical_des_SS = c_fresnel.m_eta_optical_des_SS;
            double therm_eff_des_SS = c_fresnel.m_therm_eff_des_SS;
            double eff_des_SS = c_fresnel.m_eff_des_SS;
            double W_dot_pump_des_SS = c_fresnel.m_W_dot_pump_des_SS;                       // [MWe]

            double T_loop_out_des_SS = c_fresnel.m_T_loop_out_des_SS;                       // [C]
            double Q_loop_des_SS = c_fresnel.m_Q_loop_des_SS / 1e6;                         // [MW]
            double therm_eff_loop_des_SS = c_fresnel.m_therm_eff_loop_des_SS;
            double eff_loop_des_SS = c_fresnel.m_eff_loop_des_SS;
            double Q_loss_receiver_des_SS = c_fresnel.m_Q_loss_receiver_des_SS;             // [MWt]
            double Q_loss_hdr_rnr_des_SS = c_fresnel.m_Q_loss_hdr_rnr_des_SS;               // [MWt]

            // Assign
            {
                assign("A_loop", A_loop);
                assign("loop_opt_eff", loop_opt_eff);
                assign("loop_therm_eff", loop_therm_eff);
                assign("loop_eff", loop_eff);
                assign("sm1_aperture", sm1_aperture);
                assign("sm1_nLoops", sm1_nLoops);
                assign("total_tracking_power", total_tracking_power);
                assign("A_field", A_field);
                assign("q_field_des_actual", q_field_des);
                assign("q_field_des_ideal", q_field_des_ideal);
                assign("field_area", field_area);
                assign("total_land_area", total_land_area);
                assign("field_htf_min_temp", field_htf_min_temp);
                assign("field_htf_max_temp", field_htf_max_temp);
                assign("mdot_field_des", mdot_field_des);

                assign("m_dot_htfmin_actual", c_fresnel.m_m_dot_htfmin);    //[kg/s]
                assign("m_dot_htfmax_actual", c_fresnel.m_m_dot_htfmax);    //[kg/s]
                assign("f_htfmin_actual", c_fresnel.m_f_htfmin);
                assign("f_htfmax_actual", c_fresnel.m_f_htfmax);

                assign("dP_field_des_SS", dP_field_des_SS);
                assign("Q_field_des_SS", Q_field_des_SS);
                assign("T_field_out_des_SS", T_field_out_des_SS);
                assign("m_dot_des_SS", m_dot_des_SS);
                assign("m_dot_loop_des_SS", m_dot_loop_des_SS);
                assign("V_hdr_min_des_SS", V_hdr_min_des_SS);
                assign("V_hdr_max_des_SS", V_hdr_max_des_SS);
                assign("eta_optical_des_SS", eta_optical_des_SS);
                assign("therm_eff_des_SS", therm_eff_des_SS);
                assign("eff_des_SS", eff_des_SS);
                assign("W_dot_pump_des_SS", W_dot_pump_des_SS);

                assign("T_loop_out_des_SS", T_loop_out_des_SS);
                assign("Q_loop_des_SS", Q_loop_des_SS);
                assign("therm_eff_loop_des_SS", therm_eff_loop_des_SS);
                assign("eff_loop_des_SS", eff_loop_des_SS);

                assign("Q_loss_receiver_des_SS", Q_loss_receiver_des_SS);
                assign("Q_loss_hdr_rnr_des_SS", Q_loss_hdr_rnr_des_SS);
            }

            // Collector and Receiver
            double DP_pressure_loss = c_fresnel.m_nMod * c_fresnel.m_DP_nominal;            // [bar]
            double avg_dt_des = c_fresnel.m_dT_des;                                         // [C]
            double hl_des = c_fresnel.m_hl_des;                                             // [W/m]
            double opt_derate = c_fresnel.m_opt_derate;
            double opt_normal = c_fresnel.m_opt_normal;

            // Assign
            {
                assign("DP_pressure_loss", DP_pressure_loss);
                assign("avg_dt_des", avg_dt_des);
                assign("hl_des", hl_des);
                assign("opt_derate", opt_derate);
                assign("opt_normal", opt_normal);
            }

            // Storage
            double V_tes_htf_avail_calc /*m3*/, V_tes_htf_total_calc /*m3*/,
                d_tank_calc /*m*/, q_dot_loss_tes_des_calc /*MWt*/, dens_store_htf_at_T_ave_calc /*kg/m3*/,
                Q_tes_des_calc /*MWt-hr*/, tes_total_mass/*kg*/;

            storage.get_design_parameters(V_tes_htf_avail_calc, V_tes_htf_total_calc,
                d_tank_calc, q_dot_loss_tes_des_calc, dens_store_htf_at_T_ave_calc, Q_tes_des_calc, tes_total_mass);

            double vol_min = V_tes_htf_total_calc * (storage.m_h_tank_min / storage.m_h_tank);
            double tes_htf_min_temp = storage.get_min_storage_htf_temp() - 273.15;
            double tes_htf_max_temp = storage.get_max_storage_htf_temp() - 273.15;
            double tes_htf_dens = storage.get_storage_htf_density();
            double tes_htf_cp = storage.get_storage_htf_cp();

            // Assign
            {
                assign("vol_tank", V_tes_htf_total_calc);
                assign("Q_tes_des", Q_tes_des_calc);
                assign("d_tank", d_tank_calc);
                assign("vol_min", vol_min);
                assign("q_dot_loss_tes_des", q_dot_loss_tes_des_calc);
                assign("tes_htf_min_temp", tes_htf_min_temp);
                assign("tes_htf_max_temp", tes_htf_max_temp);
                assign("tes_htf_dens", tes_htf_dens);
                assign("tes_htf_cp", tes_htf_cp);
            }

            // Power Cycle

            //double m_dot_htf_pc_des_perhr;    //[kg/hr]
            //double cp_htf_pc_des;       //[kJ/kg-K]
            //double W_dot_pc_pump_des;   //[MWe]
            //double W_dot_pc_cooling_des;   //[MWe]
            //int n_T_htf_pars, n_T_amb_pars, n_m_dot_pars;
            //n_T_htf_pars = n_T_amb_pars = n_m_dot_pars = -1;
            //double T_htf_ref_calc, T_htf_low_calc, T_htf_high_calc, T_amb_ref_calc, T_amb_low_calc, T_amb_high_calc,
            //    m_dot_htf_ND_ref_calc, m_dot_htf_ND_low_calc, m_dot_htf_ND_high_calc, W_dot_gross_ND_des, Q_dot_HTF_ND_des,
            //    W_dot_cooling_ND_des, m_dot_water_ND_des;
            //T_htf_ref_calc = T_htf_low_calc = T_htf_high_calc =
            //    T_amb_ref_calc = T_amb_low_calc = T_amb_high_calc =
            //    m_dot_htf_ND_ref_calc = m_dot_htf_ND_low_calc = m_dot_htf_ND_high_calc =
            //    W_dot_gross_ND_des = Q_dot_HTF_ND_des = W_dot_cooling_ND_des = m_dot_water_ND_des = std::numeric_limits<double>::quiet_NaN();

            //rankine_pc.get_design_parameters(m_dot_htf_pc_des_perhr, cp_htf_pc_des, W_dot_pc_pump_des, W_dot_pc_cooling_des,
            //    n_T_htf_pars, n_T_amb_pars, n_m_dot_pars,
            //    T_htf_ref_calc /*C*/, T_htf_low_calc /*C*/, T_htf_high_calc /*C*/,
            //    T_amb_ref_calc /*C*/, T_amb_low_calc /*C*/, T_amb_high_calc /*C*/,
            //    m_dot_htf_ND_ref_calc, m_dot_htf_ND_low_calc /*-*/, m_dot_htf_ND_high_calc /*-*/,
            //    W_dot_gross_ND_des, Q_dot_HTF_ND_des, W_dot_cooling_ND_des, m_dot_water_ND_des);

            //// Assign
            //{
            //    assign("q_dot_cycle_des", q_dot_pc_des);
            //    assign("mdot_cycle_des", m_dot_htf_pc_des_perhr / 3600.0); //   [kg/s]
            //}

            // System Design
            double W_dot_bop_design, W_dot_fixed_parasitic_design;    //[MWe]
            csp_solver.get_design_parameters(W_dot_bop_design, W_dot_fixed_parasitic_design);

            nameplate = q_dot_pc_des;    // [MWt]

            // Assign
            {
                assign("nameplate", nameplate * 1.E-3); // [MWt]
                assign("W_dot_bop_design", W_dot_bop_design);
                assign("W_dot_fixed", W_dot_fixed_parasitic_design);

                assign("solar_mult", c_fresnel.m_solar_mult);
                assign("nLoops", c_fresnel.m_nLoops);
                assign("total_Ap", c_fresnel.m_Ap_tot);

                assign("system_capacity", nameplate * 1.E3);    //[kWt]
                assign("cp_system_nameplate", nameplate);       //[MWt]
                assign("cp_battery_nameplate", 0.0);            //[MWt]
            }

            // System Control
            // temporary fix
            vector<double> aux_vec = as_vector_double("aux_array");
            double W_dot_cycle_des = 0;
            double aux_design = aux_vec[0] * aux_vec[1] * (aux_vec[2] + aux_vec[3] + aux_vec[4]) * W_dot_cycle_des;

            // Assign
            {
                assign("aux_design", aux_design);
            }
        }

        // Calculate Costs and assign outputs
        if (true)
        {

            // Collect dedicated cost inputs
            double site_improvements_spec_cost = as_double("site_improvements_spec_cost");
            double solar_field_spec_cost = as_double("solar_field_spec_cost");
            double htf_system_spec_cost = as_double("htf_system_spec_cost");
            double storage_spec_cost = as_double("storage_spec_cost");
            double heat_sink_spec_cost = as_double("heat_sink_spec_cost");
            double bop_spec_cost = as_double("bop_spec_cost");
            double contingency_percent = as_double("contingency_percent");

            double epc_cost_per_acre = as_double("epc_cost_per_acre");
            double epc_cost_percent_direct = as_double("epc_cost_percent_direct");
            double epc_cost_per_watt = as_double("epc_cost_per_watt");
            double epc_cost_fixed = as_double("epc_cost_fixed");
            double plm_cost_per_acre = as_double("plm_cost_per_acre");
            double plm_cost_percent_direct = as_double("plm_cost_percent_direct");
            double plm_cost_per_watt = as_double("plm_cost_per_watt");
            double plm_cost_fixed = as_double("plm_cost_fixed");

            double sales_tax_percent = as_double("sales_tax_percent");

            // Collect necessary variables defined in other tabs
            double site_improvements_area = c_fresnel.m_Ap_tot;
            double solar_field_area = c_fresnel.m_Ap_tot;
            double htf_system_area = c_fresnel.m_Ap_tot;
            // Q_tes
            double heat_sink_mwt = as_double("q_pb_design");
            double power_plant_mwt = heat_sink_mwt;         // MWe
            double bop_mwt = heat_sink_mwt;                 // MWe
            // total_land_area                      // m2
            // nameplate_des                        // MWe
            double sales_tax_rate = as_double("sales_tax_rate");

            // Define outputs
            double heat_sink_cost_out, ts_cost_out, site_improvements_cost_out, bop_cost_out, solar_field_cost_out, htf_system_cost_out, contingency_cost_out,
                total_direct_cost_out, epc_total_cost_out, plm_total_cost_out, total_indirect_cost_out, sales_tax_total_out, total_installed_cost_out, installed_per_capacity_out;
            double dummy;

            // Calculate Costs
            N_mspt::calculate_mslf_costs(site_improvements_area, site_improvements_spec_cost, solar_field_area, solar_field_spec_cost, htf_system_area, htf_system_spec_cost, Q_tes, storage_spec_cost,0,0,
                heat_sink_mwt, heat_sink_spec_cost, bop_mwt, bop_spec_cost, contingency_percent, total_land_area, nameplate, epc_cost_per_acre, epc_cost_percent_direct, epc_cost_per_watt,
                epc_cost_fixed, plm_cost_per_acre, plm_cost_percent_direct, plm_cost_per_watt, plm_cost_fixed, sales_tax_rate, sales_tax_percent,

                heat_sink_cost_out, ts_cost_out, site_improvements_cost_out, bop_cost_out, solar_field_cost_out, htf_system_cost_out, dummy, contingency_cost_out,
                total_direct_cost_out, epc_total_cost_out, plm_total_cost_out, total_indirect_cost_out, sales_tax_total_out, total_installed_cost_out, installed_per_capacity_out);

            // Assign Outputs
            {
                assign("site_improvements_cost", site_improvements_cost_out);
                assign("solar_field_cost", solar_field_cost_out);
                assign("htf_system_cost", htf_system_cost_out);
                assign("ts_cost", ts_cost_out);
                assign("heat_sink_cost", heat_sink_cost_out);
                assign("bop_cost", bop_cost_out);
                assign("contingency_cost", contingency_cost_out);
                assign("total_direct_cost", total_direct_cost_out);

                assign("epc_total_cost", epc_total_cost_out);
                assign("plm_total_cost", plm_total_cost_out);
                assign("total_indirect_cost", total_indirect_cost_out);

                assign("sales_tax_total", sales_tax_total_out);
                assign("total_installed_cost", total_installed_cost_out);
                assign("installed_per_capacity", installed_per_capacity_out);
            }

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


        // Return if only called for design point
        if (sim_type != 1)
            return;

        update("Begin timeseries simulation...", 0.0);
        std::clock_t clock_start = std::clock();

        // Run Simulation
        try
        {
            // Simulate
            csp_solver.Ssimulate(sim_setup);
        }
        catch (C_csp_exception& csp_exception)
        {
            // Report warning before exiting with error
            while (csp_solver.mc_csp_messages.get_message(&out_type, &out_msg))
            {
                log(out_msg);
            }

            throw exec_error("fresnel_physical", csp_exception.m_error_message);
        }

        // If no exception, then report messages
        while (csp_solver.mc_csp_messages.get_message(&out_type, &out_msg))
        {
            log(out_msg, out_type);
        }


        std::clock_t clock_end = std::clock();
        double sim_duration = (clock_end - clock_start) / (double)CLOCKS_PER_SEC;		//[s]
        assign("sim_duration", (ssc_number_t)sim_duration);

        // Outputs


        size_t count;
        ssc_number_t* p_q_dot_heat_sink = as_array("q_dot_to_heat_sink", &count);
        //ssc_number_t* p_W_dot_net = as_array("P_out_net", &count);
        ssc_number_t* p_time_final_hr = as_array("time_hr", &count);
        if ((int)count != n_steps_fixed)
            throw exec_error("fresnel_physical", "The number of fixed steps does not match the length of output data arrays");

        // 'adjustment_factors' class stores factors in hourly array, so need to index as such
        adjustment_factors haf(this->get_var_table(), "adjust");
        if (!haf.setup(n_steps_fixed))
            throw exec_error("fresnel_physical", "failed to setup adjustment factors: " + haf.error());

        ssc_number_t* p_gen = allocate("gen", n_steps_fixed);
        ssc_number_t* p_q_dot_defocus_est = allocate("q_dot_defocus_est", n_steps_fixed);
        ssc_number_t* p_SCAs_def = as_array("SCAs_def", &count);
        if ((int)count != n_steps_fixed)
            throw exec_error("fresnel_physical", "The number of fixed steps does not match the length of output data arrays2");

        ssc_number_t* p_W_dot_parasitic_tot = as_array("W_dot_parasitic_tot", &count);
        ssc_number_t* p_W_dot_par_tot_haf = allocate("W_dot_par_tot_haf", n_steps_fixed);

        ssc_number_t* p_q_dot_htf_sf_out = as_array("q_dot_htf_sf_out", &count);
        if ((int)count != n_steps_fixed)
            throw exec_error("fresnel_physical", "The number of fixed steps does not match the length of output data arrays3");
        for (int i = 0; i < n_steps_fixed; i++)
        {
            size_t hour = (size_t)ceil(p_time_final_hr[i]);
            p_gen[i] = (ssc_number_t)(p_q_dot_heat_sink[i] * haf(hour) * 1.E3);     //[kWe]
            p_q_dot_defocus_est[i] = (ssc_number_t)(1.0 - p_SCAs_def[i]) * p_q_dot_htf_sf_out[i]; //[MWt]
            p_W_dot_parasitic_tot[i] *= -1.0;			//[MWe] Label is total parasitics, so change to a positive value
            p_W_dot_par_tot_haf[i] = (ssc_number_t)(p_W_dot_parasitic_tot[i] * haf(hour) * 1.E3);		//[kWe] apply availability derate and convert from MWe 
        }

        // Monthly outputs
        accumulate_monthly_for_year("gen", "monthly_energy", sim_setup.m_report_step / 3600.0, steps_per_hour, 1);

        // Annual outputs
        accumulate_annual_for_year("gen", "annual_energy", sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour);

        // This term currently includes TES freeze protection
        accumulate_annual_for_year("W_dot_par_tot_haf", "annual_electricity_consumption", sim_setup.m_report_step / 3600.0, steps_per_hour);	//[kWe-hr]

        double V_water_mirrors = as_double("water_per_wash") / 1000.0 * c_fresnel.m_Ap_tot * as_double("washes_per_year");
        assign("annual_total_water_use", (ssc_number_t)V_water_mirrors);

        ssc_number_t annual_field_fp = accumulate_annual_for_year("q_dot_freeze_prot", "annual_field_freeze_protection", sim_setup.m_report_step / 3600.0 * 1.E3, steps_per_hour);    //[kWt-hr]
        ssc_number_t annual_tes_fp = accumulate_annual_for_year("q_tes_heater", "annual_tes_freeze_protection", sim_setup.m_report_step / 3600.0 * 1.E3, steps_per_hour); //[kWt-hr]

        ssc_number_t annual_thermal_consumption = annual_field_fp + annual_tes_fp;  //[kWt-hr]
        assign("annual_thermal_consumption", annual_thermal_consumption);

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
        if (is_dispatch) {
            std::string disp_sum_msg;
            dispatch.count_solutions_by_type(flag, (int)as_double("disp_frequency"), disp_sum_msg);
            log(disp_sum_msg, SSC_NOTICE);
            avg_gap = dispatch.calc_avg_subopt_gap(gap, flag, (int)as_double("disp_frequency"));
        }
        assign("avg_suboptimal_rel_mip_gap", (ssc_number_t)avg_gap);


        ssc_number_t ae = as_number("annual_energy");			//[kWt-hr]
        double kWh_per_kW = ae / (nameplate*1.E3);              // convert nameplate to kW
        assign("capacity_factor", (ssc_number_t)(kWh_per_kW / 8760. * 100.));
        assign("kwh_per_kw", (ssc_number_t)kWh_per_kW);







        // Do unit post-processing here

        ssc_number_t* p_annual_energy_dist_time = gen_heatmap(this, steps_per_hour);
        // Non-timeseries array outputs
        double P_adj = storage.P_in_des; // slightly adjust all field design pressures to account for pressure drop in TES before hot tank
        transform(c_fresnel.m_P_rnr_dsn.begin(), c_fresnel.m_P_rnr_dsn.end(), c_fresnel.m_P_rnr_dsn.begin(), [P_adj](double x) {return x + P_adj; });
        transform(c_fresnel.m_P_hdr_dsn.begin(), c_fresnel.m_P_hdr_dsn.end(), c_fresnel.m_P_hdr_dsn.begin(), [P_adj](double x) {return x + P_adj; });
        transform(c_fresnel.m_P_loop_dsn.begin(), c_fresnel.m_P_loop_dsn.end(), c_fresnel.m_P_loop_dsn.begin(), [P_adj](double x) {return x + P_adj; });

        ssc_number_t* p_pipe_runner_diams = allocate("pipe_runner_diams", c_fresnel.m_D_runner.size());
        std::copy(c_fresnel.m_D_runner.begin(), c_fresnel.m_D_runner.end(), p_pipe_runner_diams);
        ssc_number_t* p_pipe_runner_wallthk = allocate("pipe_runner_wallthk", c_fresnel.m_WallThk_runner.size());
        std::copy(c_fresnel.m_WallThk_runner.begin(), c_fresnel.m_WallThk_runner.end(), p_pipe_runner_wallthk);
        ssc_number_t* p_pipe_runner_lengths = allocate("pipe_runner_lengths", c_fresnel.m_L_runner.size());
        std::copy(c_fresnel.m_L_runner.begin(), c_fresnel.m_L_runner.end(), p_pipe_runner_lengths);
        ssc_number_t* p_pipe_runner_expansions = allocate("pipe_runner_expansions", c_fresnel.m_N_rnr_xpans.size());
        std::copy(c_fresnel.m_N_rnr_xpans.begin(), c_fresnel.m_N_rnr_xpans.end(), p_pipe_runner_expansions);
        ssc_number_t* p_pipe_runner_mdot_dsn = allocate("pipe_runner_mdot_dsn", c_fresnel.m_m_dot_rnr_dsn.size());
        std::copy(c_fresnel.m_m_dot_rnr_dsn.begin(), c_fresnel.m_m_dot_rnr_dsn.end(), p_pipe_runner_mdot_dsn);
        ssc_number_t* p_pipe_runner_vel_dsn = allocate("pipe_runner_vel_dsn", c_fresnel.m_V_rnr_dsn.size());
        std::copy(c_fresnel.m_V_rnr_dsn.begin(), c_fresnel.m_V_rnr_dsn.end(), p_pipe_runner_vel_dsn);
        ssc_number_t* p_pipe_runner_T_dsn = allocate("pipe_runner_T_dsn", c_fresnel.m_T_rnr_dsn.size());
        std::copy(c_fresnel.m_T_rnr_dsn.begin(), c_fresnel.m_T_rnr_dsn.end(), p_pipe_runner_T_dsn);
        ssc_number_t* p_pipe_runner_P_dsn = allocate("pipe_runner_P_dsn", c_fresnel.m_P_rnr_dsn.size());
        std::copy(c_fresnel.m_P_rnr_dsn.begin(), c_fresnel.m_P_rnr_dsn.end(), p_pipe_runner_P_dsn);

        ssc_number_t* p_pipe_header_diams = allocate("pipe_header_diams", c_fresnel.m_D_hdr.size());
        std::copy(c_fresnel.m_D_hdr.begin(), c_fresnel.m_D_hdr.end(), p_pipe_header_diams);
        ssc_number_t* p_pipe_header_wallthk = allocate("pipe_header_wallthk", c_fresnel.m_WallThk_hdr.size());
        std::copy(c_fresnel.m_WallThk_hdr.begin(), c_fresnel.m_WallThk_hdr.end(), p_pipe_header_wallthk);
        //ssc_number_t* p_pipe_header_lengths = allocate("pipe_header_lengths", c_fresnel.m_L_hdr.size());
        //std::copy(c_fresnel.m_L_hdr.begin(), c_fresnel.m_L_hdr.end(), p_pipe_header_lengths);
        ssc_number_t* p_pipe_header_expansions = allocate("pipe_header_expansions", c_fresnel.m_N_hdr_xpans.size());
        std::copy(c_fresnel.m_N_hdr_xpans.begin(), c_fresnel.m_N_hdr_xpans.end(), p_pipe_header_expansions);
        ssc_number_t* p_pipe_header_mdot_dsn = allocate("pipe_header_mdot_dsn", c_fresnel.m_m_dot_hdr_dsn.size());
        std::copy(c_fresnel.m_m_dot_hdr_dsn.begin(), c_fresnel.m_m_dot_hdr_dsn.end(), p_pipe_header_mdot_dsn);
        ssc_number_t* p_pipe_header_vel_dsn = allocate("pipe_header_vel_dsn", c_fresnel.m_V_hdr_dsn.size());
        std::copy(c_fresnel.m_V_hdr_dsn.begin(), c_fresnel.m_V_hdr_dsn.end(), p_pipe_header_vel_dsn);
        ssc_number_t* p_pipe_header_T_dsn = allocate("pipe_header_T_dsn", c_fresnel.m_T_hdr_dsn.size());
        std::copy(c_fresnel.m_T_hdr_dsn.begin(), c_fresnel.m_T_hdr_dsn.end(), p_pipe_header_T_dsn);
        ssc_number_t* p_pipe_header_P_dsn = allocate("pipe_header_P_dsn", c_fresnel.m_P_hdr_dsn.size());
        std::copy(c_fresnel.m_P_hdr_dsn.begin(), c_fresnel.m_P_hdr_dsn.end(), p_pipe_header_P_dsn);

        ssc_number_t* p_pipe_loop_T_dsn = allocate("pipe_loop_T_dsn", c_fresnel.m_T_loop_dsn.size());
        std::copy(c_fresnel.m_T_loop_dsn.begin(), c_fresnel.m_T_loop_dsn.end(), p_pipe_loop_T_dsn);
        ssc_number_t* p_pipe_loop_P_dsn = allocate("pipe_loop_P_dsn", c_fresnel.m_P_loop_dsn.size());
        std::copy(c_fresnel.m_P_loop_dsn.begin(), c_fresnel.m_P_loop_dsn.end(), p_pipe_loop_P_dsn);

        ssc_number_t* p_pipe_tes_diams = allocate("pipe_tes_diams", storage.pipe_diams.ncells());
        std::copy(storage.pipe_diams.data(), storage.pipe_diams.data() + storage.pipe_diams.ncells(), p_pipe_tes_diams);
        ssc_number_t* p_pipe_tes_wallthk = allocate("pipe_tes_wallthk", storage.pipe_wall_thk.ncells());
        std::copy(storage.pipe_wall_thk.data(), storage.pipe_wall_thk.data() + storage.pipe_wall_thk.ncells(), p_pipe_tes_wallthk);
        ssc_number_t* p_pipe_tes_lengths = allocate("pipe_tes_lengths", storage.pipe_lengths.ncells());
        std::copy(storage.pipe_lengths.data(), storage.pipe_lengths.data() + storage.pipe_lengths.ncells(), p_pipe_tes_lengths);
        ssc_number_t* p_pipe_tes_mdot_dsn = allocate("pipe_tes_mdot_dsn", storage.pipe_m_dot_des.ncells());
        std::copy(storage.pipe_m_dot_des.data(), storage.pipe_m_dot_des.data() + storage.pipe_m_dot_des.ncells(), p_pipe_tes_mdot_dsn);
        ssc_number_t* p_pipe_tes_vel_dsn = allocate("pipe_tes_vel_dsn", storage.pipe_vel_des.ncells());
        std::copy(storage.pipe_vel_des.data(), storage.pipe_vel_des.data() + storage.pipe_vel_des.ncells(), p_pipe_tes_vel_dsn);
        ssc_number_t* p_pipe_tes_T_dsn = allocate("pipe_tes_T_dsn", storage.pipe_T_des.ncells());
        std::copy(storage.pipe_T_des.data(), storage.pipe_T_des.data() + storage.pipe_T_des.ncells(), p_pipe_tes_T_dsn);
        ssc_number_t* p_pipe_tes_P_dsn = allocate("pipe_tes_P_dsn", storage.pipe_P_des.ncells());
        std::copy(storage.pipe_P_des.data(), storage.pipe_P_des.data() + storage.pipe_P_des.ncells(), p_pipe_tes_P_dsn);


        




    }
};

DEFINE_MODULE_ENTRY(fresnel_physical_iph, "Physical Fresnel IPH applications", 1)
