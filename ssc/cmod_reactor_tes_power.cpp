/*
BSD 3-Clause License

Copyright (c) Alliance for Energy Innovation, LLC. See also https://github.com/NREL/ssc/blob/develop/LICENSE
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

#include "csp_common.h"

#include "csp_solver_util.h"
#include "csp_solver_core.h"
#include "csp_solver_pc_Rankine_indirect_224.h"
#include "csp_solver_two_tank_tes.h"
#include "csp_solver_cr_reactor.h"

#include "csp_dispatch.h"

#include "csp_solver_cr_electric_resistance.h"

#include "csp_system_costs.h"

#include <ctime>
#include <cmath>
#include <limits>

static var_info _cm_vtab_reactor_tes_power[] = {

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
    { SSC_INPUT,     SSC_NUMBER, "T_htf_cold_des",                     "Cold HTF inlet temperature at design conditions",                                                                                         "C",            "",                                  "System Design",                            "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "T_htf_hot_des",                      "Hot HTF outlet temperature at design conditions",                                                                                         "C",            "",                                  "System Design",                            "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "P_ref",                              "Reference output electric power at design condition",                                                                                     "MW",           "",                                  "System Design",                            "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "design_eff",                         "Power cycle efficiency at design",                                                                                                        "none",         "",                                  "System Design",                            "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "tshours",                            "Equivalent full-load thermal storage hours",                                                                                              "hr",           "",                                  "System Design",                            "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "reactor_mult",                       "Solar multiple",                                                                                                                          "-",            "",                                  "System Design",                            "*",                                                                "",              ""},

    // Reactor
    { SSC_INPUT,     SSC_NUMBER, "hot_htf_code",                       "Reactor HTF, 17=Salt (60% NaNO3, 40% KNO3) 10=Salt (46.5% LiF 11.5% NaF 42% KF) 50=Lookup tables",                                       "",             "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_MATRIX, "ud_hot_htf_props",                   "User defined htf property data",                                                                                                  "-",            "",                                  "Tower and Receiver",                       "*",                                                                "",              ""},

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

    // Power Cycle Inputs
    { SSC_INPUT,     SSC_NUMBER, "pc_config",                          "PC configuration 0=Steam Rankine (224), 1=user defined",                                                                                  "",             "",                                  "Power Cycle",                              "?=0",                                                              "INTEGER",       ""},
    { SSC_INPUT,     SSC_NUMBER, "pb_pump_coef",                       "Pumping power to move 1kg of HTF through PB loop",                                                                                        "kW/kg",        "",                                  "Power Cycle",                              "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "startup_time",                       "Time needed for power block startup",                                                                                                     "hr",           "",                                  "Power Cycle",                              "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "startup_frac",                       "Fraction of design thermal power needed for startup",                                                                                     "none",         "",                                  "Power Cycle",                              "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "cycle_max_frac",                     "Maximum turbine over design operation fraction",                                                                                          "",             "",                                  "Power Cycle",                              "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "cycle_cutoff_frac",                  "Minimum turbine operation fraction before shutdown",                                                                                      "",             "",                                  "Power Cycle",                              "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "q_sby_frac",                         "Fraction of thermal power required for standby",                                                                                          "",             "",                                  "Power Cycle",                              "*",                                                                "",              ""},

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
    { SSC_INPUT,     SSC_NUMBER, "ud_is_sco2_regr",                    "0: (default) simple max htf mass flow correction; 1: sco2 heuristic regression; 2: no correction",                                        "",             "",                                  "User Defined Power Cycle",                 "?=0",                                                              "",              ""},
    { SSC_INPUT,     SSC_MATRIX, "ud_ind_od",                          "Off design user-defined power cycle performance as function of T_htf, m_dot_htf [ND], and T_amb",                                         "",             "",                                  "User Defined Power Cycle",                 "pc_config=1",                                                      "",              ""},

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
    { SSC_INPUT,     SSC_NUMBER, "is_timestep_load_fractions",         "Use turbine load fraction for each timestep instead of block dispatch?",                                                                  "",             "",                                  "Time of Delivery Factors",                 "?=0",                                                              "",              "SIMULATION_PARAMETER" },
    { SSC_INPUT,     SSC_ARRAY,  "timestep_load_fractions",            "Turbine load fraction for each timestep, alternative to block dispatch",                                                                  "",             "",                                  "System Control",                           "",                                                                 "",              ""},
    { SSC_INPUT,     SSC_ARRAY,  "f_turb_tou_periods",                 "Dispatch logic for turbine load fraction",                                                                                                "",             "",                                  "System Control",                           "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_MATRIX, "weekday_schedule",                   "12x24 CSP operation Time-of-Use Weekday schedule",                                                                                        "",             "",                                  "System Control",                           "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_MATRIX, "weekend_schedule",                   "12x24 CSP operation Time-of-Use Weekend schedule",                                                                                        "",             "",                                  "System Control",                           "*",                                                                "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "is_tod_pc_target_also_pc_max",       "Is the TOD target cycle heat input also the max cycle heat input?",                                                                       "",             "",                                  "System Control",                           "?=0",                                                              "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "can_cycle_use_standby",              "Can the cycle use standby operation?",                                                                                                    "",             "",                                  "System Control",                           "?=0",                                                              "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_NUMBER, "disp_horizon",                       "Time horizon for dispatch optimization",                                                                                                  "hour",         "",                                  "System Control",                           "is_dispatch=1",                                                    "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "disp_frequency",                     "Frequency for dispatch optimization calculations",                                                                                        "hour",         "",                                  "System Control",                           "is_dispatch=1",                                                    "",              ""},
    { SSC_INPUT,     SSC_NUMBER, "disp_steps_per_hour",                "Time steps per hour for dispatch optimization calculations",                                                                              "",             "",                                  "System Control",                           "?=1",                                                              "",              "SIMULATION_PARAMETER"},
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
    { SSC_INPUT,     SSC_NUMBER, "q_rec_heattrace",                    "Receiver heat trace energy consumption during startup",                                                                                   "kWhe",         "",                                  "System Control",                           "?=0.0",                                                            "",              "SIMULATION_PARAMETER"},

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

    // Day of week for weekday/weekend schedules
    { SSC_INPUT,        SSC_NUMBER,     "start_day_of_year",           "Start day of year for TOD periods",                                                                                                       "0..6", "0=Monday, 6=Sunday",    "Time of Delivery Factors", "?=0", "", "" },

    { SSC_INPUT,     SSC_ARRAY,  "ppa_price_input",			           "PPA prices - yearly",			                                                                                                          "$/kWh",	      "",	                               "Revenue",			                       "ppa_multiplier_model=0&csp_financial_model<5&is_dispatch=1&sim_type=1",       "",      	       "SIMULATION_PARAMETER"},
    { SSC_INPUT,     SSC_MATRIX, "mp_energy_market_revenue",           "Energy market revenue input",                                                                                                             "",             "Lifetime x 2[Cleared Capacity(MW),Price($/MWh)]", "Revenue",                    "csp_financial_model=6&is_dispatch=1&sim_type=1",                              "",              "SIMULATION_PARAMETER"},

    // Optional Component Initialization (state at start of first timestep)
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

    // Costs
    { SSC_INPUT,     SSC_NUMBER, "reactor_spec_cost",                  "Reactor specific cost",                                                                                                                   "$/kWt",        "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "cycle_spec_cost",                    "Power cycle specific cost",                                                                                                               "$/kWe",        "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "tes_spec_cost",                      "Thermal energy storage cost",                                                                                                             "$/kWht",       "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "bop_spec_cost",                      "BOS specific cost",                                                                                                                       "$/kWe",        "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "contingency_rate",                   "Contingency for cost overrun",                                                                                                            "%",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "sales_tax_rate",                     "Sales tax rate",                                                                                                                          "%",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "sales_tax_frac",                     "Percent of cost to which sales tax applies",                                                                                              "%",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "epc_cost_perc_of_direct",            "EPC cost percent of direct",                                    "%",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "epc_cost_per_watt",                  "EPC cost per watt",                                             "$/W",          "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "epc_cost_fixed",                     "EPC fixed",                                                     "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "land_cost_perc_of_direct",           "Land cost percent of direct",                                   "%",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "land_cost_per_watt",                 "Land cost per watt",                                            "$/W",          "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,     SSC_NUMBER, "land_cost_fixed",                    "Land fixed",                                                    "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },

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
    // Design Outputs here:
    // ****************************************************************************************************************************************

        // System capacity required by downstream financial model
    { SSC_OUTPUT,    SSC_NUMBER, "system_capacity",                    "System capacity",                                                                                                                         "kWe",          "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "cp_system_nameplate",                 "System capacity for capacity payments",                                                                                                   "MWe",          "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "cp_battery_nameplate",                "Battery nameplate",                                                                                                                       "MWe",          "",                                  "System Costs",                             "*",                                                                "",              "" },

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
    { SSC_OUTPUT,    SSC_NUMBER, "Q_tes_des",                          "TES design capacity",                                                                                                                     "MWht",       "",                                 "TES Design Calc",                          "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "V_tes_htf_avail_des",                "TES volume of HTF available for heat transfer",                                                                                           "m3",           "",                                 "TES Design Calc",                          "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "V_tes_htf_total_des",                "TES total HTF volume",                                                                                                                    "m3",           "",                                 "TES Design Calc",                          "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "d_tank_tes",                         "TES tank diameter",                                                                                                                       "m",            "",                                 "TES Design Calc",                          "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "q_dot_loss_tes_des",                 "TES thermal loss at design",                                                                                                              "MWt",          "",                                 "TES Design Calc",                          "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "tshours_reactor",                    "TES duration at the reactor design output",                                                                                               "hr",           "",                                 "TES Design Calc",                          "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "dens_store_htf_at_T_ave",            "TES density of HTF at avg temps",                                                                                                         "kg/m3",        "",                                 "TES Design Calc",                          "*",                                                                "",              "" },

        // Balance of Plant
    { SSC_OUTPUT,    SSC_NUMBER, "nameplate",                          "Nameplate capacity",                                                                                                                      "MWe",          "",                                 "System Design Calc",                       "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "W_dot_bop_design",                   "BOP parasitic at design",                                                                                                                "MWe",          "",                                 "Balance of Plant",                         "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "W_dot_fixed",                        "Fixed parasitic at design",                                                                                                               "MWe",          "",                                 "Balance of Plant",                         "*",                                                                "",              "" },

        // Costs
    { SSC_OUTPUT,    SSC_NUMBER, "tes_cost_calc",                      "TES cost",                                                                                                                                "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "cycle_cost_calc",                    "Power cycle cost",                                                                                                                        "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "bop_cost_calc",                      "BOP cost",                                                                                                                                "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "direct_subtotal_cost_calc",          "Direct capital pre-contingency cost",                                                                                                      "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "contingency_cost_calc",              "Contingency cost",                                                                                                                        "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "total_direct_cost_calc",             "Total direct cost",                                                                                                                       "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "epc_cost_calc",                      "EPC and owner cost",                                                                                                                      "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "land_cost_calc",                     "Total land cost",                                                                                                                         "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "sales_tax_cost_calc",                "Sales tax cost",                                                                                                                          "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "total_indirect_cost_calc",           "Total indirect cost",                                                                                                                     "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "total_installed_cost",               "Total installed cost",                                                                                                                    "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_NUMBER, "installed_per_cap_cost_calc",        "Estimated installed cost per cap",                                                                                                        "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },

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
    { SSC_OUTPUT,    SSC_ARRAY,  "P_cond",                             "PC condensing pressure",                                                                                                                 "Pa",           "",                                  "PC",                                       "?",                                                                "",              ""},
    { SSC_OUTPUT,    SSC_ARRAY,  "P_cond_iter_err",                    "PC condenser pressure iteration error",                                                                                                    "",             "",                                  "PC",                                       "?",                                                                "",              "" },
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

        // System outputs
    { SSC_OUTPUT,    SSC_ARRAY,  "P_out_net",                          "System net electrical power",                                                                                                             "MWe",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},

        // Controller outputs
    { SSC_OUTPUT,    SSC_ARRAY,  "tou_value",                          "CSP operating time-of-use period",                                                                                                         "",             "",                                  "",                                         "sim_type=1",                                                       "",              ""},
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
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_qsfsu_expected",                "Dispatch expected solar field startup energy",                                                                                            "MWt",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},
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

    { SSC_OUTPUT,    SSC_ARRAY,  "gen",                                "System net electrical power w/ avail. derate",                                                                                            "kWe",          "",                                  "",                                         "sim_type=1",                                                       "",              ""},

    // Annual single-value outputs
    { SSC_OUTPUT,    SSC_NUMBER, "annual_energy",                      "Annual net electrical energy w/ avail. derate",                                                                                           "kWhe",         "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "annual_W_cycle_gross",               "Electrical source - power cycle gross output",                                                                                            "kWhe",         "",                                  "",                                         "sim_type=1",                                                       "",              ""},
    { SSC_OUTPUT,    SSC_NUMBER, "annual_W_cooling_tower",             "Total of condenser operation parasitic",                                                                                                  "kWhe",         "",                                  "PC",                                       "sim_type=1",                                                       "",              ""},

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

class cm_reactor_tes_power : public compute_module
{
public:

    cm_reactor_tes_power()
    {
        add_var_info(_cm_vtab_reactor_tes_power);
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
        // System Design Parameters
        double T_htf_cold_des = as_double("T_htf_cold_des");    //[C]
        double T_htf_hot_des = as_double("T_htf_hot_des");      //[C]
        double W_dot_cycle_des = as_double("P_ref");            //[MWe]
        double eta_cycle = as_double("design_eff");             //[-]
        double tshours = as_double("tshours");                  //[-]

        // System Design Calcs
        double q_dot_pc_des = W_dot_cycle_des / eta_cycle;      //[MWt]
        double Q_tes = q_dot_pc_des * tshours;                  //[MWht]
        double reactor_mult = as_number("reactor_mult");                //[-]
        double q_dot_reactor_des = q_dot_pc_des * reactor_mult;     //[MWt]

        // Weather reader
		C_csp_weatherreader weather_reader;
		if (is_assigned("solar_resource_file")){
			weather_reader.m_weather_data_provider = make_shared<weatherfile>(as_string("solar_resource_file"));
			if (weather_reader.m_weather_data_provider->has_message()) log(weather_reader.m_weather_data_provider->message(), SSC_WARNING);
		}

        size_t n_steps_full = weather_reader.m_weather_data_provider->nrecords(); //steps_per_hour * 8760;
        weather_reader.m_trackmode = 0;
        weather_reader.m_tilt = 0.0;
        weather_reader.m_azimuth = 0.0;
        // Initialize to get weather file info
        weather_reader.init();
        if (weather_reader.has_error()) throw exec_error("reactor_tes_power", weather_reader.get_error());

        // Get info from the weather reader initialization
        double site_elevation = weather_reader.ms_solved_params.m_elev;     //[m]

        int tes_type = 1;       
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
            pc->m_pc_fl = as_integer("hot_htf_code");                            // power cycle HTF is same as receiver HTF
            pc->m_pc_fl_props = as_matrix("ud_hot_htf_props");

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
                pc->m_is_udpc_sco2_regr = as_integer("ud_is_sco2_regr");            //[-]

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

        // Check if system configuration includes a heater parallel to primary collector receiver
        C_csp_collector_receiver* p_heater;
        C_csp_cr_electric_resistance* p_electric_resistance = NULL;
        bool is_parallel_heater = false;
        p_heater = p_electric_resistance;        

        // Reactor
        double reactor_efficiency = 1.0;
        double reactor_min_frac = 0.01;
        int reactor_htf_code = as_integer("hot_htf_code");
        util::matrix_t<double> reactor_ud_htf_props = as_matrix("ud_hot_htf_props");
        C_csp_cr_reactor c_reactor = C_csp_cr_reactor(T_htf_cold_des, T_htf_hot_des,
            q_dot_pc_des, reactor_efficiency, reactor_min_frac,
            reactor_htf_code, reactor_ud_htf_props);


        // Thermal energy storage
        C_csp_two_tank_tes storage(
            as_integer("hot_htf_code"),
            as_matrix("ud_hot_htf_props"),
            as_integer("hot_htf_code"),
            as_matrix("ud_hot_htf_props"),
            as_double("P_ref") / as_double("design_eff"),   //[MWt]
            as_double("reactor_mult"),                            //[-]
            as_double("P_ref") / as_double("design_eff") * as_double("tshours"),
            true,                                              // Fixed height
            as_double("h_tank"),
            0.0,                                            //[m] No input diameter (use height instead)
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
                as_vector_double("f_turb_tou_periods"), std::numeric_limits<double>::quiet_NaN(), as_number("start_day_of_year"));
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
                    throw exec_error("reactor_tes_power", "\n\nYou selected dispatch optimization which requires that the array input ppa_price_input is defined\n");
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
                    throw exec_error("reactor_tes_power", "\n\nYou selected dispatch optimization and the Specify IRR Target financial solution mode, "
                        "but dispatch optimization requires known absolute electricity prices. Dispatch optimization requires "
                        "the Specify PPA Price financial solution mode. You can continue using dispatch optimization and iteratively "
                        "solve for the PPA that results in a target IRR by running a SAM Parametric analysis or script.\n");
                }

                int en_electricity_rates = as_integer("en_electricity_rates");  // 0 = Use PPA, 1 = Use Retail
                if (en_electricity_rates == 1 && is_dispatch) {
                    throw exec_error("reactor_tes_power", "\n\nYou selected dispatch optimization and the option to Use Retail Electricity Rates on the Electricity Purchases page, "
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
                            as_vector_double("dispatch_tod_factors"), ppa_price_year1, as_number("start_day_of_year"));
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
                        throw exec_error("reactor_tes_power", ppa_msg);
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
                    throw exec_error("reactor_tes_power", "Can't select dispatch optimization if No Financial model");
                }
                else { // if no dispatch optimization, don't need an input pricing schedule
                    // If electricity pricing data is not available, then dispatch to a uniform schedule
                    elec_pricing_schedule = C_timeseries_schedule_inputs(1.0, std::numeric_limits<double>::quiet_NaN());
                }
            }
            else {
                throw exec_error("reactor_tes_power", "csp_financial_model must be 1, 2, 3, 4, or 6");
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
            int n_expect = (int)ceil((sim_setup.m_sim_time_end - sim_setup.m_sim_time_start) / 3600. * steps_per_hour);

            size_t inputs_len = 0;
            ssc_number_t* q_pc_target_su_in = as_array("q_pc_target_su_in", &inputs_len);
            if (inputs_len != n_expect) throw exec_error("reactor_tes_power", "The length of dispatch target q_pc_target_su_in array does not match the value expected from the simulation start time, end time, and time steps per hour");

            ssc_number_t* q_pc_target_on_in = as_array("q_pc_target_on_in", &inputs_len);
            if (inputs_len != n_expect) throw exec_error("reactor_tes_power", "The length of dispatch target q_pc_target_on_in array does not match the value expected from the simulation start time, end time, and time steps per hour");

            ssc_number_t* q_pb_max = as_array("q_pc_max_in", &inputs_len);
            if (inputs_len != n_expect) throw exec_error("reactor_tes_power", "The length of dispatch target q_pc_max_in array does not match the value expected from the simulation start time, end time, and time steps per hour");

            ssc_number_t* is_rec_su_allowed_in = as_array("is_rec_su_allowed_in", &inputs_len);
            if (inputs_len != n_expect) throw exec_error("reactor_tes_power", "The length of dispatch target is_rec_su_allowed_in array does not match the value expected from the simulation start time, end time, and time steps per hour");

            ssc_number_t* is_pc_su_allowed_in = as_array("is_pc_su_allowed_in", &inputs_len);
            if (inputs_len != n_expect) throw exec_error("reactor_tes_power", "The length of dispatch target is_pc_su_allowed_in array does not match the value expected from the simulation start time, end time, and time steps per hour");

            ssc_number_t* is_pc_sb_allowed_in = as_array("is_pc_sb_allowed_in", &inputs_len);
            if (inputs_len != n_expect) throw exec_error("reactor_tes_power", "The length of dispatch target is_pc_sb_allowed_in array does not match the value expected from the simulation start time, end time, and time steps per hour");

            ssc_number_t *q_dot_elec_to_PAR_HTR_in, *is_PAR_HTR_allowed_in;
            if (is_parallel_heater) {
                q_dot_elec_to_PAR_HTR_in = as_array("q_dot_elec_to_PAR_HTR_in", &inputs_len);
                if (inputs_len != n_expect) throw exec_error("reactor_tes_power", "The length of dispatch target q_dot_elec_to_PAR_HTR_in array does not match the value expected from the simulation start time, end time, and time steps per hour");

                is_PAR_HTR_allowed_in = as_array("is_PAR_HTR_allowed_in", &inputs_len);
                if (inputs_len != n_expect) throw exec_error("reactor_tes_power", "The length of dispatch target is_PAR_HTR_allowed_in array does not match the value expected from the simulation start time, end time, and time steps per hour");
            }

            tou.mc_dispatch_params.m_q_pc_target_su_in.resize(inputs_len);
            tou.mc_dispatch_params.m_q_pc_target_on_in.resize(inputs_len);
            tou.mc_dispatch_params.m_q_pc_max_in.resize(inputs_len);
            tou.mc_dispatch_params.m_is_rec_su_allowed_in.resize(inputs_len);
            tou.mc_dispatch_params.m_is_pc_su_allowed_in.resize(inputs_len);
            tou.mc_dispatch_params.m_is_pc_sb_allowed_in.resize(inputs_len);

            tou.mc_dispatch_params.m_q_dot_elec_to_PAR_HTR_in.resize(inputs_len);
            tou.mc_dispatch_params.m_is_PAR_HTR_allowed_in.resize(inputs_len);

            for (int i = 0; i < inputs_len; i++) {
                tou.mc_dispatch_params.m_q_pc_target_su_in.at(i) = q_pc_target_su_in[i];
                tou.mc_dispatch_params.m_q_pc_target_on_in.at(i) = q_pc_target_on_in[i];
                tou.mc_dispatch_params.m_q_pc_max_in.at(i) = q_pb_max[i];
                tou.mc_dispatch_params.m_is_rec_su_allowed_in.at(i) = (bool)is_rec_su_allowed_in[i];
                tou.mc_dispatch_params.m_is_pc_su_allowed_in.at(i) = (bool)is_pc_su_allowed_in[i];
                tou.mc_dispatch_params.m_is_pc_sb_allowed_in.at(i) = (bool)is_pc_sb_allowed_in[i];

                if (is_parallel_heater) {
                    tou.mc_dispatch_params.m_q_dot_elec_to_PAR_HTR_in.at(i) = q_dot_elec_to_PAR_HTR_in[i];
                    tou.mc_dispatch_params.m_is_PAR_HTR_allowed_in.at(i) = (bool)is_PAR_HTR_allowed_in[i];
                }
                else {
                    tou.mc_dispatch_params.m_q_dot_elec_to_PAR_HTR_in.at(i) = 0.0;
                    tou.mc_dispatch_params.m_is_PAR_HTR_allowed_in.at(i) = false;
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

        if (is_dispatch && sim_type == 1){

            double reactor_su_cost = 0.0;
        }

        // Instantiate Solver       
        C_csp_solver csp_solver(weather_reader, 
                        c_reactor, // collector_receiver, 
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

            throw exec_error("reactor_tes_power", csp_exception.m_error_message);
        }

        // If no exception, then report messages
        while (csp_solver.mc_csp_messages.get_message(&out_type, &out_msg))
        {
            log(out_msg, out_type);
        }

        // *****************************************************
        // System design is complete, get design parameters from component models as necessary

        double W_dot_col_tracking_des = 0.0;    // collector_receiver.get_tracking_power();    //[MWe]

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
        assign("tshours_reactor", Q_tes_des_calc / q_dot_reactor_des);  //[hr]
        assign("dens_store_htf_at_T_ave", dens_store_htf_at_T_ave_calc); //[kg/m3]

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

        double plant_net_capacity = W_dot_cycle_des - W_dot_col_tracking_des - 
                                        W_dot_pc_pump_des - W_dot_pc_cooling_des - W_dot_bop_design - W_dot_fixed_parasitic_design;    //[MWe]

        double plant_net_conv_calc = plant_net_capacity / W_dot_cycle_des; //[-]

        double system_capacity = plant_net_capacity * 1.E3;         //[kWe], convert from MWe

        assign("W_dot_bop_design", W_dot_bop_design);           //[MWe]
        assign("W_dot_fixed", W_dot_fixed_parasitic_design);    //[MWe]
        // Calculate system capacity instead of pass in
        assign("system_capacity", system_capacity);     //[kWe]
        assign("nameplate", system_capacity * 1.E-3);   //[MWe]
        assign("cp_system_nameplate", system_capacity * 1.E-3); //[MWe]
        assign("cp_battery_nameplate", 0.0);             //[MWe]

            // ******* Costs ************
        double reactor_spec_cost = as_double("reactor_spec_cost");

        double Q_storage = as_double("P_ref") / as_double("design_eff") * as_double("tshours");
        double tes_spec_cost = as_double("tes_spec_cost");

        double power_cycle_spec_cost = as_double("cycle_spec_cost");
        double bop_spec_cost = as_double("bop_spec_cost");

        double contingency_rate = as_double("contingency_rate");

        double EPC_land_perc_direct_cost = as_double("epc_cost_perc_of_direct");
        double EPC_land_per_power_cost = as_double("epc_cost_per_watt");
        double EPC_land_fixed_cost = as_double("epc_cost_fixed");
        double total_land_perc_direct_cost = as_double("land_cost_perc_of_direct");
        double total_land_per_power_cost = as_double("land_cost_per_watt");
        double total_land_fixed_cost = as_double("land_cost_fixed");
        double sales_tax_basis = as_double("sales_tax_frac");
        double sales_tax_rate = as_double("sales_tax_rate");

        double reactor_cost, tes_cost, power_cycle_cost,bop_cost, 
            direct_capital_precontingency_cost, contingency_cost, total_direct_cost, epc_and_owner_cost, total_land_cost,
            sales_tax_cost, total_indirect_cost, total_installed_cost, estimated_installed_cost_per_cap;

        reactor_cost = tes_cost = power_cycle_cost = bop_cost = 
            direct_capital_precontingency_cost = contingency_cost = total_direct_cost = epc_and_owner_cost = total_land_cost =
            sales_tax_cost = total_indirect_cost = total_installed_cost = estimated_installed_cost_per_cap = std::numeric_limits<double>::quiet_NaN();

        reactor_cost = q_dot_reactor_des * 1.E3 * reactor_spec_cost;    //[$]
        tes_cost = N_mspt::tes_cost(Q_storage, tes_spec_cost);
        power_cycle_cost = N_mspt::power_cycle_cost(W_dot_cycle_des, power_cycle_spec_cost);
        bop_cost = N_mspt::bop_cost(W_dot_cycle_des, bop_spec_cost);

        direct_capital_precontingency_cost = reactor_cost + tes_cost + power_cycle_cost + bop_cost;

        contingency_cost = N_mspt::contingency_cost(contingency_rate, direct_capital_precontingency_cost);

        total_direct_cost = N_mspt::total_direct_cost(direct_capital_precontingency_cost, contingency_cost);

        double total_land_area = 0.0;       // Don't include land area
        double total_land_spec_cost = 0.0;  // Don't calc land cost as a function of land area
        total_land_cost = N_mspt::total_land_cost(total_land_area, total_direct_cost, plant_net_capacity,
                total_land_spec_cost, total_land_perc_direct_cost, total_land_per_power_cost, total_land_fixed_cost);

        double EPC_land_spec_cost = 0.0;    // Don't calc cost as a function of land area
        epc_and_owner_cost = N_mspt::epc_and_owner_cost(total_land_area, total_direct_cost, plant_net_capacity,
                EPC_land_spec_cost, EPC_land_perc_direct_cost, EPC_land_per_power_cost, EPC_land_fixed_cost);

        sales_tax_cost = N_mspt::sales_tax_cost(total_direct_cost, sales_tax_basis, sales_tax_rate);

        total_indirect_cost = N_mspt::total_indirect_cost(total_land_cost, epc_and_owner_cost, sales_tax_cost);

        total_installed_cost = N_mspt::total_installed_cost(total_direct_cost, total_indirect_cost);

        estimated_installed_cost_per_cap = N_mspt::estimated_installed_cost_per_cap(total_installed_cost, plant_net_capacity);

        assign("total_installed_cost", (ssc_number_t)total_installed_cost);

        assign("tes_cost_calc", (ssc_number_t)tes_cost);
        assign("cycle_cost_calc", (ssc_number_t)power_cycle_cost);
        assign("bop_cost_calc", (ssc_number_t)bop_cost);
        assign("direct_subtotal_cost_calc", (ssc_number_t)direct_capital_precontingency_cost);
        assign("contingency_cost_calc", (ssc_number_t)contingency_cost);
        assign("total_direct_cost_calc", (ssc_number_t)total_direct_cost);
        assign("epc_cost_calc", (ssc_number_t)epc_and_owner_cost);
        assign("land_cost_calc", (ssc_number_t)total_land_cost);
        assign("sales_tax_cost_calc", (ssc_number_t)sales_tax_cost);
        assign("total_indirect_cost_calc", (ssc_number_t)total_indirect_cost);
        assign("installed_per_cap_cost_calc", (ssc_number_t)estimated_installed_cost_per_cap);

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

            throw exec_error("reactor_tes_power", csp_exception.m_error_message);
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
        size_t count_m_dot_pc, count_m_dot_water_pc; 
        count_m_dot_pc = count_m_dot_water_pc = 0;  
        //ssc_number_t *p_m_dot_rec = as_array("m_dot_rec", &count_m_dot_rec);
        ssc_number_t *p_m_dot_pc = as_array("m_dot_pc", &count_m_dot_pc);
        ssc_number_t *p_m_dot_water_pc = as_array("m_dot_water_pc", &count_m_dot_water_pc);
        if (count_m_dot_pc != n_steps_fixed || count_m_dot_water_pc != n_steps_fixed)
        {
            log("At least one m_dot array is a different length than 'n_steps_fixed'.", SSC_WARNING);
            return;
        }
        for (size_t i = 0; i < n_steps_fixed; i++)
        {
            //p_m_dot_rec[i] = (ssc_number_t)(p_m_dot_rec[i] / 3600.0);   //[kg/s] convert from kg/hr
            p_m_dot_pc[i] = (ssc_number_t)(p_m_dot_pc[i] / 3600.0);     //[kg/s] convert from kg/hr
            p_m_dot_water_pc[i] = (ssc_number_t)(p_m_dot_water_pc[i] / 3600.0); //[kg/s] convert from kg/hr
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
        
        accumulate_annual_for_year("P_cycle", "annual_W_cycle_gross", 1000.0*sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed/steps_per_hour);        //[kWhe]
        accumulate_annual_for_year("P_cooling_tower_tot", "annual_W_cooling_tower", 1000.0*sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour);        //[kWhe]

        accumulate_annual_for_year("disp_objective", "disp_objective_ann", sim_setup.m_report_step / 3600.0 / as_double("disp_frequency"), steps_per_hour, 1, n_steps_fixed/steps_per_hour);
        accumulate_annual_for_year("disp_solve_iter", "disp_iter_ann", sim_setup.m_report_step / 3600.0 / as_double("disp_frequency"), steps_per_hour, 1, n_steps_fixed/steps_per_hour);
        accumulate_annual_for_year("disp_presolve_nconstr", "disp_presolve_nconstr_ann", sim_setup.m_report_step / 3600.0/ as_double("disp_frequency"), steps_per_hour, 1, n_steps_fixed/steps_per_hour);
        accumulate_annual_for_year("disp_presolve_nvar", "disp_presolve_nvar_ann", sim_setup.m_report_step / 3600.0/ as_double("disp_frequency"), steps_per_hour, 1, n_steps_fixed/steps_per_hour);
        accumulate_annual_for_year("disp_solve_time", "disp_solve_time_ann", sim_setup.m_report_step / 3600.0 / as_double("disp_frequency"), steps_per_hour, 1, n_steps_fixed/steps_per_hour );
        accumulate_annual_for_year("disp_solve_state", "disp_solve_state_ann", sim_setup.m_report_step / 3600.0 / as_double("disp_frequency"), steps_per_hour, 1, n_steps_fixed / steps_per_hour);

        // Reporting dispatch solution counts
        std::vector<int> flag = as_vector_integer("disp_subopt_flag");
        std::vector<double> gap = as_vector_double("disp_rel_mip_gap");
        
        double avg_gap = 0;
        if (as_boolean("is_dispatch")) {
            std::string disp_sum_msg;
            dispatch.count_solutions_by_type(flag, (int)as_double("disp_frequency"), disp_sum_msg);
            log(disp_sum_msg, SSC_NOTICE);
            avg_gap = dispatch.calc_avg_subopt_gap(gap, flag, (int)as_double("disp_frequency"));
        }
        assign("avg_suboptimal_rel_mip_gap", (ssc_number_t)avg_gap);

        // Calculated Outputs
        accumulate_annual_for_year("m_dot_water_pc", "annual_total_water_use", sim_setup.m_report_step / 1000.0, steps_per_hour, 1, n_steps_fixed/steps_per_hour); //[m^3], convert from kg
        
        ssc_number_t ae = as_number("annual_energy");           //[kWhe]
        ssc_number_t pg = as_number("annual_W_cycle_gross");    //[kWhe]
        ssc_number_t annual_sales_energy = as_number("annual_sales_energy");        //[kWhe]
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
            std::vector<pair<int, double>> ppa_pairs;
            ppa_pairs.resize(count);
            for (size_t i = 0; i < count; i++) {
                ppa_pairs[i].first = i;
                ppa_pairs[i].second = p_pricing_mult[i];
            }

            std::sort(ppa_pairs.begin(), ppa_pairs.end(), SortByDouble);
            int n_ppa_steps = 1000;

            double total_energy_in_sub_period = 0.0;
            for (size_t i = 0; i < n_ppa_steps; i++) {
                size_t j = ppa_pairs[i].first;
                total_energy_in_sub_period += p_gen[j] * sim_setup.m_report_step / 3600.0;     //[kWhe]
            }

            double total_energy_nameplate = nameplate * n_ppa_steps * sim_setup.m_report_step / 3600.0;     //[kWhe]

            double cap_fac_highest_1000_ppas = 0.0;
            if (nameplate > 0.0) {
                cap_fac_highest_1000_ppas = total_energy_in_sub_period / total_energy_nameplate * 100.0;    //[%]        
            }

            assign("capacity_factor_highest_1000_ppas", cap_fac_highest_1000_ppas);

            n_ppa_steps = 2000;

            total_energy_in_sub_period = 0.0;
            for (size_t i = 0; i < n_ppa_steps; i++) {
                size_t j = ppa_pairs[i].first;
                total_energy_in_sub_period += p_gen[j] * sim_setup.m_report_step / 3600.0;     //[kWhe]
            }

            total_energy_nameplate = nameplate * n_ppa_steps * sim_setup.m_report_step / 3600.0;     //[kWhe]

            double cap_fac_highest_2000_ppas = 0.0;
            if (nameplate > 0.0) {
                cap_fac_highest_2000_ppas = total_energy_in_sub_period / total_energy_nameplate * 100.0;    //[%]
            }

            assign("capacity_factor_highest_2000_ppas", cap_fac_highest_2000_ppas);

            // **********************************************************
            // **********************************************************

            // Capacity factors based on warmest ambient temperatures
            ssc_number_t* p_tdry = as_array("tdry", &count);

            std::vector<pair<int, double>> tdry_pairs;
            tdry_pairs.resize(count);
            for (size_t i = 0; i < count; i++) {
                tdry_pairs[i].first = i;
                tdry_pairs[i].second = p_tdry[i];
            }

            std::sort(tdry_pairs.begin(), tdry_pairs.end(), SortByDouble);
            int n_tdry_steps = 100;

            double total_energy_in_sub_period_tdry = 0.0;
            for (size_t i = 0; i < n_tdry_steps; i++) {
                size_t j = tdry_pairs[i].first;
                total_energy_in_sub_period_tdry += p_gen[j] * sim_setup.m_report_step / 3600.0;     //[kWhe]
            }

            double total_energy_nameplate_tdry = nameplate * n_tdry_steps * sim_setup.m_report_step / 3600.0;     //[kWhe]

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

DEFINE_MODULE_ENTRY(reactor_tes_power, "Reactor - test - powre cyle model with hierarchical controller and dispatch optimization", 1)
