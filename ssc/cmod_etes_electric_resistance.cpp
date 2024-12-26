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
#include "csp_common.h"
#include "csp_solver_core.h"
#include "csp_solver_cr_electric_resistance.h"
#include "csp_solver_pc_Rankine_indirect_224.h"
#include "etes_dispatch.h"

#include "csp_system_costs.h"

#include <ctime>

static var_info _cm_vtab_etes_electric_resistance[] = {

    // Resource Data
    { SSC_INPUT,  SSC_STRING, "solar_resource_file",           "Local weather file path",                                        "",             "",                                  "Solar Resource",                    "?",                                                                "LOCAL_FILE",    ""},

    // Simulation Parameters
    { SSC_INPUT,  SSC_NUMBER, "is_dispatch",                   "Allow dispatch optimization?",                                  "",             "",                                  "System Control",                           "?=0",                                                              "",               ""},
    { SSC_INPUT,  SSC_NUMBER, "sim_type",                      "1 (default): timeseries, 2: design only",                        "",             "",                                  "System Control",                           "?=1",                                                              "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,  SSC_NUMBER, "etes_financial_model",          "",                                                               "1-8",          "",                                  "Financial Model",                          "?=1",                                                              "INTEGER,MIN=0", ""},
    { SSC_INPUT,  SSC_NUMBER, "time_start",                    "Simulation start time",                                          "s",            "",                                  "System Control",                           "?=0",                                                              "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,  SSC_NUMBER, "time_stop",                     "Simulation stop time",                                           "s",            "",                                  "System Control",                           "?=31536000",                                                       "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,  SSC_NUMBER, "time_steps_per_hour",           "Number of simulation time steps per hour",                       "",             "",                                  "System Control",                           "?=-1",                                                             "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,  SSC_NUMBER, "vacuum_arrays",                 "Allocate arrays for only the required number of steps",          "",             "",                                  "System Control",                           "?=0",                                                              "",              "SIMULATION_PARAMETER"},


    // System
    { SSC_INPUT,  SSC_NUMBER, "T_htf_cold_des",                "Cold HTF inlet temperature at design conditions",                "C",            "",                                  "System Design",                            "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "T_htf_hot_des",                 "Hot HTF outlet temperature at design conditions",                "C",            "",                                  "System Design",                            "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "P_ref",                         "Reference output electric power at design condition",            "MW",           "",                                  "System Design",                            "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "design_eff",                    "Power cycle efficiency at design",                               "none",         "",                                  "System Design",                            "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "tshours",                       "Equivalent full-load thermal storage hours",                     "hr",           "",                                  "System Design",                            "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "heater_mult",                   "Heater multiple relative to design cycle thermal power",         "-",            "",                                  "System Design",                            "*",                                                                "",              ""},

    // Power Cycle
        // General
    { SSC_INPUT,  SSC_NUMBER, "pc_config",                     "PC configuration 0=Steam Rankine, 1=user defined",               "",             "",                                  "Power Cycle",                              "?=0",                                                              "INTEGER",       ""},
    { SSC_INPUT,  SSC_NUMBER, "pb_pump_coef",                  "Pumping power to move 1kg of HTF through PB loop",               "kW/kg",        "",                                  "Power Cycle",                              "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "startup_time",                  "Time needed for power block startup",                            "hr",           "",                                  "Power Cycle",                              "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "startup_frac",                  "Fraction of design thermal power needed for startup",            "none",         "",                                  "Power Cycle",                              "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "cycle_max_frac",                "Maximum turbine over design operation fraction",                 "",             "",                                  "Power Cycle",                              "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "cycle_cutoff_frac",             "Minimum turbine operation fraction before shutdown",             "",             "",                                  "Power Cycle",                              "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "q_sby_frac",                    "Fraction of thermal power required for standby",                 "",             "",                                  "Power Cycle",                              "*",                                                                "",              ""},
        // Steam Rankine
    { SSC_INPUT,  SSC_NUMBER, "dT_cw_ref",                     "Reference condenser cooling water inlet/outlet temperature difference",  "C",    "",                                  "Rankine Cycle",                            "pc_config=0",                                                      "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "T_amb_des",                     "Reference ambient temperature at design point",                          "C",    "",                                  "Rankine Cycle",                            "pc_config=0",                                                      "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "CT",                            "Condensor type: 1=evaporative, 2=air",                                   "",     "",                                  "Rankine Cycle",                            "pc_config=0",                                                      "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "T_approach",                    "Cooling tower approach temperature",                                     "C",    "",                                  "Rankine Cycle",                            "pc_config=0",                                                      "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "T_ITD_des",                     "ITD at design for dry system",                                           "C",    "",                                  "Rankine Cycle",                            "pc_config=0",                                                      "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "P_cond_ratio",                  "Condenser pressure ratio",                                               "",     "",                                  "Rankine Cycle",                            "pc_config=0",                                                      "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "pb_bd_frac",                    "Power block blowdown steam fraction",                                    "",     "",                                  "Rankine Cycle",                            "pc_config=0",                                                      "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "P_cond_min",                    "Minimum condenser pressure",                                             "inHg", "",                                  "Rankine Cycle",                            "pc_config=0",                                                      "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "n_pl_inc",                      "Number of part-load increments for the heat rejection system",           "none", "",                                  "Rankine Cycle",                            "pc_config=0",                                                      "INTEGER",       ""},
    { SSC_INPUT,  SSC_NUMBER, "tech_type",                     "Turbine inlet pressure control 1=Fixed, 3=Sliding",                      "",     "",                                  "Rankine Cycle",                            "pc_config=0",                                                      "",              ""},
        // User Defined cycle
    { SSC_INPUT,  SSC_NUMBER, "ud_f_W_dot_cool_des",           "Percent of user-defined power cycle design gross output consumed by cooling",                     "%",      "",       "User Defined Power Cycle",                 "pc_config=1",                                                      "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "ud_m_dot_water_cool_des",       "Mass flow rate of water required at user-defined power cycle design point",                       "kg/s",   "",       "User Defined Power Cycle",                 "pc_config=1",                                                      "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "ud_is_sco2_regr",               "False: default, base udpc interpolation, True: use sco2 heuristic regression",                    "",       "",       "User Defined Power Cycle",                 "?=0",                                                              "",              ""},
    { SSC_INPUT,  SSC_MATRIX, "ud_ind_od",                     "Off design user-defined power cycle performance as function of T_htf, m_dot_htf [ND], and T_amb", "",       "",       "User Defined Power Cycle",                 "pc_config=1",                                                      "",              ""},


    // TES
        // Performance
    { SSC_INPUT,  SSC_NUMBER, "hot_htf_code",                  "Receiver HTF, 17=Salt (60% NaNO3, 40% KNO3) 10=Salt (46.5% LiF 11.5% NaF 42% KF) 50=Lookup tables", "", "",          "Thermal Storage",                          "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_MATRIX, "ud_hot_htf_props",              "User-defined TES fluid property data",                          "-",            "",                                  "Thermal Storage",                          "hot_htf_code=50",                                                  "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "tes_init_hot_htf_percent",      "Initial fraction of available volume that is hot",              "%",            "",                                  "Thermal Storage",                          "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "h_tank",                        "Total height of tank (height of HTF when tank is full)",        "m",            "",                                  "Thermal Storage",                          "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "cold_tank_max_heat",            "Rated heater capacity for cold tank heating",                   "MW",           "",                                  "Thermal Storage",                          "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "u_tank",                        "Loss coefficient from the tank",                                "W/m2-K",       "",                                  "Thermal Storage",                          "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "tank_pairs",                    "Number of equivalent tank pairs",                               "",             "",                                  "Thermal Storage",                          "*",                                                                "INTEGER",       ""},
    { SSC_INPUT,  SSC_NUMBER, "cold_tank_Thtr",                "Minimum allowable cold tank HTF temperature",                   "C",            "",                                  "Thermal Storage",                          "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "h_tank_min",                    "Minimum allowable HTF height in storage tank",                  "m",            "",                                  "Thermal Storage",                          "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "hot_tank_Thtr",                 "Minimum allowable hot tank HTF temperature",                    "C",            "",                                  "Thermal Storage",                          "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "hot_tank_max_heat",             "Rated heater capacity for hot tank heating",                    "MW",           "",                                  "Thermal Storage",                          "*",                                                                "",              ""},


    // Heater
    { SSC_INPUT,  SSC_NUMBER, "heater_efficiency",             "Heater electric to thermal efficiency",                         "%",            "",                                  "Heater",                                   "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "f_q_dot_des_allowable_su",      "Fraction of design power allowed during startup",               "-",            "",                                  "Heater",                                   "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "hrs_startup_at_max_rate",       "Duration of startup at max startup power",                      "hr",           "",                                  "Heater",                                   "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "f_q_dot_heater_min",            "Minimum allowable heater output as fraction of design",         "",             "",                                  "Heater",                                   "*",                                                                "",              ""},


    // System control
    { SSC_INPUT,  SSC_NUMBER, "disp_horizon",                  "Time horizon for dispatch optimization",                        "hour",         "",                                  "System Control",                           "is_dispatch=1",                                                    "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "disp_frequency",                "Frequency for dispatch optimization calculations",              "hour",         "",                                  "System Control",                           "is_dispatch=1",                                                    "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "disp_max_iter",                 "Max number of dispatch optimization iterations",                "",             "",                                  "System Control",                           "is_dispatch=1",                                                    "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "disp_timeout",                  "Max dispatch optimization solve duration",                      "s",            "",                                  "System Control",                           "is_dispatch=1",                                                    "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "disp_mip_gap",                  "Dispatch optimization solution tolerance",                      "",             "",                                  "System Control",                           "is_dispatch=1",                                                    "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "disp_spec_bb",                  "Dispatch optimization B&B heuristic",                           "",             "",                                  "System Control",                           "?=-1",                                                             "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,  SSC_NUMBER, "disp_reporting",                "Dispatch optimization reporting level",                         "",             "",                                  "System Control",                           "?=-1",                                                             "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,  SSC_NUMBER, "disp_spec_presolve",            "Dispatch optimization presolve heuristic",                      "",             "",                                  "System Control",                           "?=-1",                                                             "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,  SSC_NUMBER, "disp_spec_scaling",             "Dispatch optimization scaling heuristic",                       "",             "",                                  "System Control",                           "?=-1",                                                             "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,  SSC_NUMBER, "disp_pen_delta_w",              "Dispatch cycle production change penalty",                      "$/MWe-change", "",                                  "System Control",                           "is_dispatch=1",                                                    "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "disp_csu_cost",                 "Cycle startup cost",                                            "$/MWe-cycle/start", "",                             "System Control",                           "is_dispatch=1",                                                    "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "disp_hsu_cost",                 "Heater startup cost",                                           "$/MWe-cycle/start", "",                             "System Control",                           "is_dispatch=1",                                                    "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "disp_time_weighting",           "Dispatch optimization future time discounting factor",          "",             "",                                  "System Control",                           "is_dispatch=1",                                                    "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "disp_down_time_min",            "Minimum time requirement for cycle to not generate power",      "hr",           "",                                  "System Control",                           "is_dispatch=1",                                                    "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "disp_up_time_min",              "Minimum time requirement for cycle to generate power",          "hr",           "",                                  "System Control",                           "is_dispatch=1",                                                    "",              ""},


    // System performance
    { SSC_INPUT,  SSC_NUMBER, "pb_fixed_par",                  "Fixed parasitic load that don't generate heat - runs at all times","MWe/MWcap", "",                                  "System Control",                           "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "bop_par",                       "Balance of plant parasitic power fraction",                        "MWe/MWcap", "",                                  "System Control",                           "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "bop_par_f",                     "Balance of plant parasitic power fraction - mult frac",            "",          "",                                  "System Control",                           "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "bop_par_0",                     "Balance of plant parasitic power fraction - const coeff",          "",          "",                                  "System Control",                           "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "bop_par_1",                     "Balance of plant parasitic power fraction - linear coeff",         "",          "",                                  "System Control",                           "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "bop_par_2",                     "Balance of plant parasitic power fraction - quadratic coeff",      "",          "",                                  "System Control",                           "*",                                                                "",              ""},


    // Pricing schedules and multipliers
    { SSC_INPUT,  SSC_NUMBER, "ppa_multiplier_model",          "PPA multiplier model",                                          "0/1",          "0=diurnal,1=timestep",              "Time of Delivery Factors",                 "?=0",                                                              "INTEGER,MIN=0", "SIMULATION_PARAMETER"},
    { SSC_INPUT,  SSC_ARRAY,  "dispatch_factors_ts",           "Dispatch payment factor timeseries array",                      "",             "",                                  "Time of Delivery Factors",                 "ppa_multiplier_model=1&etes_financial_model<5&is_dispatch=1&sim_type=1",      "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,  SSC_MATRIX, "dispatch_sched_weekday",        "PPA pricing weekday schedule, 12x24",                           "",             "",                                  "Time of Delivery Factors",                 "ppa_multiplier_model=0&etes_financial_model<5&is_dispatch=1&sim_type=1",      "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,  SSC_MATRIX, "dispatch_sched_weekend",        "PPA pricing weekend schedule, 12x24",                           "",             "",                                  "Time of Delivery Factors",                 "ppa_multiplier_model=0&etes_financial_model<5&is_dispatch=1&sim_type=1",      "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,  SSC_ARRAY,  "dispatch_tod_factors",          "TOD factors for periods 1 through 9",                           "",
        "We added this array input after SAM 2022.12.21 to replace the functionality of former single value inputs dispatch_factor1 through dispatch_factor9",                       "Time of Delivery Factors",                 "ppa_multiplier_model=0&csp_financial_model<5&is_dispatch=1&sim_type=1",       "",              "SIMULATION_PARAMETER" },

    { SSC_INPUT,  SSC_ARRAY,  "ppa_price_input",			   "PPA prices - yearly",			                                "$/kWh",	    "",	                                 "Revenue",			                         "ppa_multiplier_model=0&etes_financial_model<5&is_dispatch=1&sim_type=1",      "",      	     "SIMULATION_PARAMETER"},
    { SSC_INPUT,  SSC_MATRIX, "mp_energy_market_revenue",      "Energy market revenue input",                                   "",             "Lifetime x 2[Cleared Capacity(MW),Price($/MWh)]", "Revenue",                    "etes_financial_model=6&is_dispatch=1&sim_type=1",                             "",              "SIMULATION_PARAMETER"},

    // System Costs
    { SSC_INPUT,  SSC_NUMBER, "cycle_spec_cost",               "Power cycle specific cost",                                     "$/kWe",        "",                                  "System Cost",                              "*",                                                                "",              "" },
    { SSC_INPUT,  SSC_NUMBER, "tes_spec_cost",                 "Thermal energy storage specific cost",                          "$/kWht",       "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,  SSC_NUMBER, "heater_spec_cost",              "Heater specific cost",                                          "$/kWht",       "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,  SSC_NUMBER, "bop_spec_cost",                 "Balance of plant specific cost",                                "$/kWe",        "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,  SSC_NUMBER, "contingency_rate",              "Contingency for cost overrun",                                  "%",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,  SSC_NUMBER, "sales_tax_frac",                "Percent of cost to which sales tax applies",                    "%",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,  SSC_NUMBER, "epc_cost_perc_of_direct",       "EPC cost percent of direct",                                    "%",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,  SSC_NUMBER, "epc_cost_per_watt",             "EPC cost per watt",                                             "$/W",          "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,  SSC_NUMBER, "epc_cost_fixed",                "EPC fixed",                                                     "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,  SSC_NUMBER, "land_cost_perc_of_direct",      "Land cost percent of direct",                                   "%",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,  SSC_NUMBER, "land_cost_per_watt",            "Land cost per watt",                                            "$/W",          "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,  SSC_NUMBER, "land_cost_fixed",               "Land fixed",                                                    "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },


    // Financial Parameters
    { SSC_INPUT,  SSC_NUMBER, "sales_tax_rate",                "Sales tax rate",                                                "%",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },


        // Construction financing inputs/outputs (SSC variable table from cmod_cb_construction_financing)
    { SSC_INPUT,  SSC_NUMBER, "const_per_interest_rate1",      "Interest rate, loan 1",                                         "%",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
    { SSC_INPUT,  SSC_NUMBER, "const_per_interest_rate2",      "Interest rate, loan 2",                                         "%",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
    { SSC_INPUT,  SSC_NUMBER, "const_per_interest_rate3",      "Interest rate, loan 3",                                         "%",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
    { SSC_INPUT,  SSC_NUMBER, "const_per_interest_rate4",      "Interest rate, loan 4",                                         "%",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
    { SSC_INPUT,  SSC_NUMBER, "const_per_interest_rate5",      "Interest rate, loan 5",                                         "%",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
    { SSC_INPUT,  SSC_NUMBER, "const_per_months1",             "Months prior to operation, loan 1",                             "",             "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
    { SSC_INPUT,  SSC_NUMBER, "const_per_months2",             "Months prior to operation, loan 2",                             "",             "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
    { SSC_INPUT,  SSC_NUMBER, "const_per_months3",             "Months prior to operation, loan 3",                             "",             "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
    { SSC_INPUT,  SSC_NUMBER, "const_per_months4",             "Months prior to operation, loan 4",                             "",             "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
    { SSC_INPUT,  SSC_NUMBER, "const_per_months5",             "Months prior to operation, loan 5",                             "",             "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
    { SSC_INPUT,  SSC_NUMBER, "const_per_percent1",            "Percent of total installed cost, loan 1",                       "%",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
    { SSC_INPUT,  SSC_NUMBER, "const_per_percent2",            "Percent of total installed cost, loan 2",                       "%",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
    { SSC_INPUT,  SSC_NUMBER, "const_per_percent3",            "Percent of total installed cost, loan 3",                       "%",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
    { SSC_INPUT,  SSC_NUMBER, "const_per_percent4",            "Percent of total installed cost, loan 4",                       "%",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
    { SSC_INPUT,  SSC_NUMBER, "const_per_percent5",            "Percent of total installed cost, loan 5",                       "%",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
    { SSC_INPUT,  SSC_NUMBER, "const_per_upfront_rate1",       "Upfront fee on principal, loan 1",                              "%",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
    { SSC_INPUT,  SSC_NUMBER, "const_per_upfront_rate2",       "Upfront fee on principal, loan 2",                              "%",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
    { SSC_INPUT,  SSC_NUMBER, "const_per_upfront_rate3",       "Upfront fee on principal, loan 3",                              "%",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
    { SSC_INPUT,  SSC_NUMBER, "const_per_upfront_rate4",       "Upfront fee on principal, loan 4",                              "%",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },
    { SSC_INPUT,  SSC_NUMBER, "const_per_upfront_rate5",       "Upfront fee on principal, loan 5",                              "%",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },

    // ****************************************************************************************************************************************
    //     DEPRECATED INPUTS -- exec() checks if a) variable is assigned and b) if replacement variable is assigned. throws exception if a=true and b=false
    // ****************************************************************************************************************************************
    { SSC_INPUT,  SSC_NUMBER, "P_boil",                        "Boiler operating pressure",                                     "bar",          "",                                  "Rankine Cycle",                             "",                                                                "",              "SIMULATION_PARAMETER" },


    // ****************************************************************************************************************************************
    // Design Outputs here:
    // ****************************************************************************************************************************************
        // System
    { SSC_OUTPUT, SSC_NUMBER, "system_capacity",             "System capacity",                         "kWe",          "",                                  "System Design Calc",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "nameplate",                   "Nameplate capacity",                      "MWe",          "",                                  "System Design Calc",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "cp_system_nameplate",          "System capacity for capacity payments",   "MWe",          "",                                  "System Design Calc",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "cp_battery_nameplate",         "Battery nameplate",                       "MWe",          "",                                  "System Design Calc",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "q_pb_design",                 "Cycle thermal input at design"            "MWt",          "",                                  "System Design Calc",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "q_dot_heater_design",         "Heater thermal output at design",         "MWt",          "",                                  "System Design Calc",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "tshours_heater",              "Hours of TES relative to heater output",  "hr",           "",                                  "System Design Calc",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "Q_tes_des",                   "TES design capacity",                     "MWt-hr",       "",                                  "System Design Calc",                             "*",                                                                "",              "" },

        // Cycle
    { SSC_OUTPUT, SSC_NUMBER, "m_dot_htf_cycle_des",         "Cycle htf mass flow rate at design",      "kg/s",         "",                                  "Cycle Design Calc",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "cp_htf_cycle_des",            "Cycle htf cp at T ave at design",         "kJ/kg-K",      "",                                  "Cycle Design Calc",                             "*",                                                                "",              "" },
        // UDPC
    { SSC_OUTPUT, SSC_NUMBER, "n_T_htf_pars_calc",           "UDPC number of HTF parametric values",               "",  "",                                  "UDPC Design Calc",                              "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "n_T_amb_pars_calc",           "UDPC number of ambient temp parametric values",      "",  "",                                  "UDPC Design Calc",                              "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "n_m_dot_pars_calc",           "UDPC number of mass flow parametric values",         "",  "",                                  "UDPC Design Calc",                              "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "T_htf_ref_calc",              "UDPC reference HTF temperature",                     "C", "",                                  "UDPC Design Calc",                              "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "T_htf_low_calc",              "UDPC low level HTF temperature",                     "C", "",                                  "UDPC Design Calc",                              "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "T_htf_high_calc",             "UDPC high level HTF temperature",                    "C", "",                                  "UDPC Design Calc",                              "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "T_amb_ref_calc",              "UDPC reference ambient temperature",                 "C", "",                                  "UDPC Design Calc",                              "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "T_amb_low_calc",              "UDPC low level ambient temperature",                 "C", "",                                  "UDPC Design Calc",                              "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "T_amb_high_calc",             "UDPC high level ambient temperature",                "C", "",                                  "UDPC Design Calc",                              "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "m_dot_htf_ND_ref_calc",       "UDPC reference normalized mass flow rate",           "",  "",                                  "UDPC Design Calc",                              "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "m_dot_htf_ND_low_calc",       "UDPC low level normalized mass flow rate",           "",  "",                                  "UDPC Design Calc",                              "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "m_dot_htf_ND_high_calc",      "UDPC high level normalized mass flow rate",          "",  "",                                  "UDPC Design Calc",                              "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "W_dot_gross_ND_des_calc",     "UDPC calculated normalized gross power at design",   "",  "",                                  "UDPC Design Calc",                              "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "Q_dot_HTF_ND_des_calc",       "UDPC calculated normalized heat input at design",    "",  "",                                  "UDPC Design Calc",                              "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "W_dot_cooling_ND_des_calc",   "UPPC calculated normalized cooling power at design", "",  "",                                  "UDPC Design Calc",                              "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "m_dot_water_ND_des_calc",     "UDPC calculated water use at design",                "",  "",                                  "UDPC Design Calc",                              "*",                                                                "",              "" },

        // Heater
    { SSC_OUTPUT, SSC_NUMBER, "W_dot_heater_des",            "Heater electricity consumption at design","MWe",          "",                                  "Cycle Design Calc",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "E_heater_su_des",             "Heater startup energy",                   "MWt-hr",       "",                                  "Cycle Design Calc",                             "*",                                                                "",              "" },

        // TES
    { SSC_OUTPUT, SSC_NUMBER, "V_tes_htf_avail",             "Volume of TES HTF available for heat transfer", "m3",     "",                                  "TES Design Calc",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "V_tes_htf_total",             "Total TES HTF volume",                    "m3",     "",                                        "TES Design Calc",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "d_tank_tes",                  "Diameter of TES tank",                    "m",      "",                                        "TES Design Calc",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "q_dot_loss_tes_des",          "TES thermal loss at design",              "MWt",    "",                                        "TES Design Calc",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "dens_store_htf_at_T_ave",     "Density of TES HTF at avg temps",         "kg/m3",  "",                                        "TES Design Calc",                             "*",                                                                "",              "" },

        // System
    { SSC_OUTPUT, SSC_NUMBER, "W_dot_bop_design",            "BOP parasitics at design",                "MWe",          "",                                  "System Design Calc",                             "*",                                                                "",              "" },

    
        // Costs
    { SSC_OUTPUT, SSC_NUMBER, "heater_cost_calc",            "Heater cost",                             "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "tes_cost_calc",               "TES cost",                                "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "bop_cost_calc",               "BOP cost",                                "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "cycle_cost_calc",             "Cycle cost",                              "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "direct_subtotal_cost_calc",   "Direct subtotal cost",                    "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "contingency_cost_calc",       "Contingency cost",                        "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "total_direct_cost_calc",      "Total direct cost",                       "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "epc_cost_calc",               "EPC cost",                                "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "land_cost_calc",              "Land cost",                               "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "sales_tax_cost_calc",         "Sales tax cost",                          "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "total_indirect_cost_calc",    "Total indirect cost",                     "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "installed_per_cap_cost_calc", "Installed cost per capacity",             "$/kWe",        "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "total_installed_cost",          "Total installed cost",                  "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "construction_financing_cost",   "Total construction financing cost",     "$",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },


    // ****************************************************************************************************************************************
    // Timeseries Simulation Outputs here:
    // ****************************************************************************************************************************************
        // Simulation outputs
    { SSC_OUTPUT, SSC_ARRAY,  "time_hr",                       "Time at end of timestep",                                       "hr",           "",                                  "",                                         "sim_type=1",                                                                "",              ""},
    { SSC_OUTPUT, SSC_ARRAY,  "elec_purchase_price_mult",      "Electricity purchase price multiplier",                         "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "tou_period",                    "Time of use period",                                            "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "tdry",                          "Resource dry Bulb temperature",                                 "C",            "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "twet",                          "Resource wet Bulb temperature",                                 "C",            "",                                  "",                                         "sim_type=1",                                                                "",              "" },

        // Heater outputs
    { SSC_OUTPUT, SSC_ARRAY,  "W_dot_heater",                  "Heater electricity consumption",                                "MWe",          "",                                  "",                                         "sim_type=1",                                                                "",              ""},
    { SSC_OUTPUT, SSC_ARRAY,  "q_dot_heater_to_htf",           "Heater thermal power to HTF",                                   "MWt",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "q_dot_heater_startup",          "Heater thermal power consumed during startup",                  "MWt",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "m_dot_htf_heater",              "Heater HTF mass flow rate",                                     "kg/s",         "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "T_htf_heater_in",               "Heater HTF inlet temperature",                                  "C",            "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "T_htf_heater_out",              "Heater HTF outlet temperature",                                 "C",            "",                                  "",                                         "sim_type=1",                                                                "",              "" },

        // TES outputs
    { SSC_OUTPUT, SSC_ARRAY,  "q_dot_dc_tes",                  "TES discharge thermal power",                                   "MWt",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "q_dot_ch_tes",                  "TES charge thermal power",                                      "MWt",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "e_ch_tes",                      "TES charge state",                                              "MWht",         "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "q_dot_tes_losses",              "TES thermal losses",                                            "MWt",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "q_dot_tes_heater",              "TES freeze protection power",                                   "MWe",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "T_tes_hot",                     "TES hot temperature",                                           "C",            "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "T_tes_cold",                    "TES cold temperature",                                          "C",            "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "mass_tes_cold",                 "TES cold tank mass (end)",                                      "kg",           "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "mass_tes_hot",                  "TES hot tank mass (end)",                                       "kg",           "",                                  "",                                         "sim_type=1",                                                                "",              "" },

        // Cycle outputs
    { SSC_OUTPUT, SSC_ARRAY,  "eta_cycle_gross",               "PC efficiency gross (no cooling parasitics)",                   "",             "",                                  "powerblock",                               "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "q_dot_cycle",                   "PC thermal power",                                              "MWt",          "",                                  "powerblock",                               "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "W_dot_cycle_gross",             "PC electrical power gross (no cooling parasitics)",             "MWe",          "",                                  "powerblock",                               "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "q_dot_cycle_startup",           "PC startup thermal power",                                      "MWt",          "",                                  "powerblock",                               "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "W_dot_cycle_cooling",           "PC cooling parasitics",                                         "MWe",          "",                                  "powerblock",                               "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "W_dot_cycle_net",               "PC electrical power net (with cooling parasitics)",             "MWe",          "",                                  "powerblock",                               "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "eta_cycle_net",                 "PC efficiency net (with cooling parasitics)",                   "",             "",                                  "powerblock",                               "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "m_dot_htf_cycle",               "PC HTF mass flow rate",                                         "kg/s",         "",                                  "powerblock",                               "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "T_htf_cycle_in",                "PC HTF inlet temperature",                                      "C",            "",                                  "powerblock",                               "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "T_htf_cycle_out",               "PC HTF outlet temperature",                                     "C",            "",                                  "powerblock",                               "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "m_dot_water_cycle",             "PC water consumption, makeup + cooling",                        "kg/s",         "",                                  "powerblock",                               "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "W_dot_cycle_htf_pump",          "PC HTF pumping power",                                          "MWe",          "",                                  "powerblock",                               "sim_type=1",                                                                "",              "" },

        // System outputs
    { SSC_OUTPUT, SSC_ARRAY,  "W_dot_fixed_parasitics",        "Parasitic power plant fixed load",                              "MWe",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "W_dot_bop_parasitics",          "Parasitic power plant generation-dependent laod",               "MWe",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "W_dot_out_net",                 "Total electric power to grid",                                  "MWe",          "",                                  "",                                         "sim_type=1",                                                                "",              ""},
    { SSC_OUTPUT, SSC_ARRAY,  "gen",                           "Total electric power to grid with available derate",            "kWe",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },

        // Controller outputs
    { SSC_OUTPUT, SSC_ARRAY,  "n_op_modes",                    "Operating modes in reporting timestep",                         "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "op_mode_1",                     "1st operating mode",                                            "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "op_mode_2",                     "2nd operating mode, if applicable",                             "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "op_mode_3",                     "3rd operating mode, if applicable",                             "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "m_dot_balance",                 "Relative mass flow balance error",                              "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "q_balance",                     "Relative energy balance error",                                 "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },

        // Dispatch outputs
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_rel_mip_gap",           "Dispatch relative MIP gap",                                     "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_solve_state",           "Dispatch solver state",                                         "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_subopt_flag",           "Dispatch suboptimal solution flag",                             "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_solve_iter",            "Dispatch iterations count",                                     "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_objective",             "Dispatch objective function value",                             "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_obj_relax",             "Dispatch objective function - relaxed max",                     "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_qsfprod_expected",      "Dispatch expected electric heater heat generation",             "MWt",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_qsfsu_expected",        "Dispatch expected electric heater startup enegy",               "MWt",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_tes_expected",          "Dispatch expected TES charge level",                            "MWht",         "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_pceff_expected",        "Dispatch expected power cycle efficiency adj.",                 "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_qpbsu_expected",        "Dispatch expected power cycle startup energy",                  "MWht",         "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_wpb_expected",          "Dispatch expected power generation",                            "MWe",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_rev_expected",          "Dispatch expected revenue factor",                              "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_presolve_nconstr",      "Dispatch number of constraints in problem",                     "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_presolve_nvar",         "Dispatch number of variables in problem",                       "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_solve_time",            "Dispatch solver time",                                          "sec",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },

        // These outputs correspond to the first csp-solver timestep in the reporting timestep.
        //     Subsequent csp-solver timesteps within the same reporting timestep are not tracked
    { SSC_OUTPUT,    SSC_ARRAY,  "operating_modes_a",          "First 3 operating modes tried",                                 "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "operating_modes_b",          "Next 3 operating modes tried",                                  "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "operating_modes_c",          "Final 3 operating modes tried",                                 "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },


        // Annual single-value outputs
    { SSC_OUTPUT, SSC_NUMBER, "annual_energy",                 "Annual total electric power to grid",                           "kWhe",         "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "annual_energy_full_availability","Annual total electric power to grid w/ full availability",     "MWhe",         "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "annual_E_heater",               "Annual heater electric energy consumption",                     "MWhe",         "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "annual_Q_heater_to_htf",        "Annual heater thermal power to HTF",                            "MWhe",         "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "annual_Q_heater_startup",       "Annual heater thermal energy consumed by startup",              "MWht",         "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "annual_E_tes_heater",           "Annual TES freeze heater electric energy consumption",          "MWhe",         "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "annual_Q_tes_losses",           "Annual TES thermal energy lost to ambient",                     "MWht",         "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "annual_E_cycle_gross",          "Annual cycle gross electric energy generation",                 "MWhe",         "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "annual_Q_cycle_thermal_in",     "Annual cycle thermal energy input",                             "MWht",         "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "annual_Q_cycle_thermal_startup","Annual cycle thermal energy consumed by startup",               "MWht",         "",                                  "",                                         "sim_type=1",                                                                "",              "" },

    { SSC_OUTPUT, SSC_NUMBER, "disp_objective_ann",            "Annual sum of dispatch objective function value",                  "$",         "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "disp_iter_ann",                 "Annual sum of dispatch solver iterations",                          "",         "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "disp_presolve_nconstr_ann",     "Annual sum of dispatch problem constraint count",                   "",         "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "disp_presolve_nvar_ann",        "Annual sum of dispatch problem variable count",                     "",         "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "disp_solve_time_ann",           "Annual sum of dispatch solver time",                             "sec",         "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "disp_solve_state_ann",          "Annual sum of dispatch solve state",                                "",         "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "avg_suboptimal_rel_mip_gap",    "Average suboptimal relative MIP gap",                              "%",         "",                                  "",                                         "sim_type=1",                                                                "",              "" },


    { SSC_OUTPUT, SSC_NUMBER, "sim_cpu_run_time",              "Simulation duration clock time",                                   "s",         "",                                  "",                                         "sim_type=1",                                                                "",              "" },

        // ETES settings for financial model
    { SSC_OUTPUT, SSC_NUMBER, "ppa_soln_mode",                 "PPA solution mode",                                             "0/1",          "0 = solve ppa,1 = specify ppa",     "Revenue",                                  "sim_type=1",                                                                "INTEGER,MIN = 0,MAX = 1", "" },
    { SSC_OUTPUT, SSC_NUMBER, "flip_target_percent",		   "After-tax IRR target",		                                    "%",	        "",					                 "Revenue",                                  "sim_type=1",					                                              "MIN=0,MAX=100",     	     "" },
    { SSC_OUTPUT, SSC_NUMBER, "total_land_area",               "Total land area",                                               "acre",         "",                                  "System Costs",                             "*",                                                                         "",              "" },

    var_info_invalid };

class cm_etes_electric_resistance : public compute_module
{
public:

    cm_etes_electric_resistance()
    {
        add_var_info(_cm_vtab_etes_electric_resistance);
        add_var_info(vtab_adjustment_factors);
        add_var_info(vtab_technology_outputs);
    }

    void exec() override
    {
        //FILE* fp = fopen("etes_cmod_to_lk.lk", "w");
        //write_cmod_to_lk_script(fp, m_vartab);

        std::clock_t clock_start = std::clock();

        // First, check sim type
        int sim_type = as_integer("sim_type");
        if (sim_type != 1 && sim_type != 2) {
            std::string sim_type_msg = util::format("sim_type input was %d. It must be 1 (timeseries) or 2 (design only)", sim_type);

            throw exec_error("etes_electric_resistsance", sim_type_msg);
        }

        // *****************************************************
        // Check deprecated variables

        if (is_assigned("P_boil")) {
            log("We removed boiler pressure (P_boil) as a user input to the Rankine Cycle model. Because the cycle efficiency"
                " is provided by the user, the boiler pressure input does not modify the efficiency as one might expect. Instead the model"
                " uses boiler pressure in second order calculations to 1) define a boiling temperature to normalize off-design HTF temperature and"
                " 2) estimate steam mass flow for cycle make-up water calculations. Because boiler pressure only has influences"
                " results in these minor non-intuitive ways, we decided to hardcode the valu to 100 bar.");
        }
        // *****************************************************
        // *****************************************************

        // *****************************************************
        // System Design Parameters
        double T_htf_cold_des = as_double("T_htf_cold_des");    //[C]
        double T_htf_hot_des = as_double("T_htf_hot_des");      //[C]
        double W_dot_cycle_des = as_double("P_ref");            //[MWe]
        double eta_cycle = as_double("design_eff");             //[-]
        double tshours = as_double("tshours");                  //[-]
        double heater_mult = as_double("heater_mult");          //[-]

        // TES parameters
        int hot_htf_code = as_integer("hot_htf_code");
        util::matrix_t<double> ud_hot_htf_props;
        if (hot_htf_code == 50) {
            ud_hot_htf_props = as_matrix("ud_hot_htf_props");
        }

        // System Design Calcs
        double q_dot_pc_des = W_dot_cycle_des / eta_cycle;      //[MWt]
        double q_dot_heater_des = q_dot_pc_des * heater_mult;   //[MWt]
        double Q_tes_des = q_dot_pc_des * tshours;              //[MWt-hr] TES thermal capacity at design
        // *****************************************************
        // *****************************************************


        // *****************************************************
        // Weather reader
        C_csp_weatherreader weather_reader;
        weather_reader.m_weather_data_provider = make_shared<weatherfile>(as_string("solar_resource_file"));
        if (weather_reader.m_weather_data_provider->has_message()) log(weather_reader.m_weather_data_provider->message(), SSC_WARNING);

        weather_reader.m_trackmode = 0;
        weather_reader.m_tilt = 0.0;
        weather_reader.m_azimuth = 0.0;
        // Initialize to get weather file info
        weather_reader.init();
        if (weather_reader.has_error()) throw exec_error("etes_electric_resistance", weather_reader.get_error());
        // *****************************************************
        // *****************************************************


        // *****************************************************
        // Simulation setup
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
        sim_setup.m_report_step = 3600.0 / (double)steps_per_hour;  //[s]
        // *****************************************************
        // *****************************************************


        // *****************************************************
        // Power cycle
        // Steam Rankine and User Defined power cycle classes
        C_pc_Rankine_indirect_224 rankine_pc;

        int pb_tech_type = as_integer("pc_config");
        if (pb_tech_type == 0 || pb_tech_type == 1)
        {
            C_pc_Rankine_indirect_224::S_params* pc = &rankine_pc.ms_params;
            pc->m_P_ref = W_dot_cycle_des;              //[MWe]
            pc->m_eta_ref = eta_cycle;                  //[-]
            pc->m_T_htf_hot_ref = T_htf_hot_des;        //[C]
            pc->m_T_htf_cold_ref = T_htf_cold_des;      //[C]
            pc->m_cycle_max_frac = as_double("cycle_max_frac");
            pc->m_cycle_cutoff_frac = as_double("cycle_cutoff_frac");
            pc->m_q_sby_frac = as_double("q_sby_frac");
            pc->m_startup_time = as_double("startup_time");
            pc->m_startup_frac = as_double("startup_frac");
            pc->m_htf_pump_coef = as_double("pb_pump_coef");
            pc->m_pc_fl = hot_htf_code;      
            pc->m_pc_fl_props = ud_hot_htf_props;

            if (pb_tech_type == 0)
            {
                pc->m_dT_cw_ref = as_double("dT_cw_ref");
                pc->m_T_amb_des = as_double("T_amb_des");
                pc->m_P_boil_des = 100.0;       //[bar]
                pc->m_CT = as_integer("CT");                    // cooling tech type: 1=evaporative, 2=air, 3=hybrid
                if (pc->m_CT > 2) {
                    std::string err_msg = util::format("The specified power cycle cooling tech type, %d, does not exist"
                        " for the ETES electric resistance heating model. Choose from 1) evaporative or 2) air\n", pc->m_CT);
                    log(err_msg, SSC_WARNING);
                    return;
                }
                pc->m_tech_type = as_integer("tech_type");      // 1: Fixed, 3: Sliding
                if (pc->m_tech_type == 2) { pc->m_tech_type = 1; }; // changing fixed pressure for the trough to fixed pressure for the tower

                if (!(pc->m_tech_type == 1 || pc->m_tech_type == 3 || pc->m_tech_type == 5 || pc->m_tech_type == 6 || pc->m_tech_type == 7 || pc->m_tech_type == 8))
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
                pc->m_W_dot_cooling_des = as_double("ud_f_W_dot_cool_des") / 100.0 * as_double("P_ref");  //[MWe]
                pc->m_m_dot_water_des = as_double("ud_m_dot_water_cool_des");       //[kg/s]
                pc->m_is_udpc_sco2_regr = as_boolean("ud_is_sco2_regr");            //[-]

                // User-Defined Cycle Off-Design Tables 
                pc->mc_combined_ind = as_matrix("ud_ind_od");
            }
        }
        else
        {
            std::string err_msg = util::format("The specified power cycle configuration, %d, does not exist. See SSC Input Table for help.\n", pb_tech_type);
            log(err_msg, SSC_WARNING);
            return;
        }

        // Set cycle cmod outputs
        rankine_pc.assign(C_pc_Rankine_indirect_224::E_W_DOT, allocate("W_dot_cycle_gross", n_steps_fixed), n_steps_fixed);
        rankine_pc.assign(C_pc_Rankine_indirect_224::E_Q_DOT_HTF, allocate("q_dot_cycle", n_steps_fixed), n_steps_fixed);
        rankine_pc.assign(C_pc_Rankine_indirect_224::E_Q_DOT_STARTUP, allocate("q_dot_cycle_startup", n_steps_fixed), n_steps_fixed);
        rankine_pc.assign(C_pc_Rankine_indirect_224::E_M_DOT_HTF, allocate("m_dot_htf_cycle", n_steps_fixed), n_steps_fixed);
        rankine_pc.assign(C_pc_Rankine_indirect_224::E_T_HTF_IN, allocate("T_htf_cycle_in", n_steps_fixed), n_steps_fixed);
        rankine_pc.assign(C_pc_Rankine_indirect_224::E_T_HTF_OUT, allocate("T_htf_cycle_out", n_steps_fixed), n_steps_fixed);
        rankine_pc.assign(C_pc_Rankine_indirect_224::E_M_DOT_WATER, allocate("m_dot_water_cycle", n_steps_fixed), n_steps_fixed);
        rankine_pc.assign(C_pc_Rankine_indirect_224::E_W_DOT_HTF_PUMP, allocate("W_dot_cycle_htf_pump", n_steps_fixed), n_steps_fixed);
        rankine_pc.assign(C_pc_Rankine_indirect_224::E_W_DOT_COOLER, allocate("W_dot_cycle_cooling", n_steps_fixed), n_steps_fixed);

        rankine_pc.assign(C_pc_Rankine_indirect_224::E_ETA_THERMAL, allocate("eta_cycle_gross", n_steps_fixed), n_steps_fixed);
        // *****************************************************
        // *****************************************************


        // *****************************************************
        // Electric resistance heater
        // Construct electric resistance heater class
        double heater_efficiency = as_double("heater_efficiency") / 100.0;          //[-] convert from % input
        double f_q_dot_des_allowable_su = as_double("f_q_dot_des_allowable_su");    //[-] fraction of design power allowed during startup
        double hrs_startup_at_max_rate = as_double("hrs_startup_at_max_rate");      //[hr] duration of startup at max startup power
        double f_heater_min = as_double("f_q_dot_heater_min");                      //[-] minimum allowable heater output as fraction of design
        C_csp_cr_electric_resistance c_electric_resistance(T_htf_cold_des, T_htf_hot_des,
            q_dot_heater_des, heater_efficiency, f_heater_min,
            f_q_dot_des_allowable_su, hrs_startup_at_max_rate,
            hot_htf_code, ud_hot_htf_props, C_csp_cr_electric_resistance::E_elec_resist_startup_mode::SEQUENCED);

        // Set heater cmod outputs
        c_electric_resistance.mc_reported_outputs.assign(C_csp_cr_electric_resistance::E_W_DOT_HEATER, allocate("W_dot_heater", n_steps_fixed), n_steps_fixed);
        c_electric_resistance.mc_reported_outputs.assign(C_csp_cr_electric_resistance::E_Q_DOT_HTF, allocate("q_dot_heater_to_htf", n_steps_fixed), n_steps_fixed);
        c_electric_resistance.mc_reported_outputs.assign(C_csp_cr_electric_resistance::E_Q_DOT_STARTUP, allocate("q_dot_heater_startup", n_steps_fixed), n_steps_fixed);
        c_electric_resistance.mc_reported_outputs.assign(C_csp_cr_electric_resistance::E_M_DOT_HTF, allocate("m_dot_htf_heater", n_steps_fixed), n_steps_fixed);
        c_electric_resistance.mc_reported_outputs.assign(C_csp_cr_electric_resistance::E_T_HTF_IN, allocate("T_htf_heater_in", n_steps_fixed), n_steps_fixed);
        c_electric_resistance.mc_reported_outputs.assign(C_csp_cr_electric_resistance::E_T_HTF_OUT, allocate("T_htf_heater_out", n_steps_fixed), n_steps_fixed);
        // *****************************************************
        // *****************************************************


        // *****************************************************
        // TES
        C_csp_two_tank_tes storage(
            hot_htf_code,
            ud_hot_htf_props,
            hot_htf_code,
            ud_hot_htf_props,
            W_dot_cycle_des / eta_cycle,  //[MWt]
            heater_mult,                  //[-]
            W_dot_cycle_des / eta_cycle * tshours,  //[MWht]
            as_double("h_tank"),
            as_double("u_tank"),
            as_integer("tank_pairs"),
            as_double("hot_tank_Thtr"),
            as_double("hot_tank_max_heat"),
            as_double("cold_tank_Thtr"),
            as_double("cold_tank_max_heat"),
            0.0,                          // MSPT assumes direct storage, so no user input here: hardcode = 0.0
            T_htf_cold_des,               //[C]
            T_htf_hot_des,                //[C]
            T_htf_hot_des,                //[C]
            T_htf_cold_des,               //[C]
            as_double("h_tank_min"),
            as_double("tes_init_hot_htf_percent"),
            as_double("pb_pump_coef"),
            false,                        //[-] False: Field HTF always goes to TES. PC HTF always comes from TES. ETES should not simultaneously operate heater and cycle
            1.0,                          // Packed volume fraction
            1.85,                         //[m/s]
            false                         // for now, to get 'tanks_in_parallel' to work
        );

        // Set TES cmod outputs
        storage.mc_reported_outputs.assign(C_csp_two_tank_tes::E_Q_DOT_LOSS, allocate("q_dot_tes_losses", n_steps_fixed), n_steps_fixed);
        storage.mc_reported_outputs.assign(C_csp_two_tank_tes::E_W_DOT_HEATER, allocate("q_dot_tes_heater", n_steps_fixed), n_steps_fixed);
        storage.mc_reported_outputs.assign(C_csp_two_tank_tes::E_TES_T_HOT, allocate("T_tes_hot", n_steps_fixed), n_steps_fixed);
        storage.mc_reported_outputs.assign(C_csp_two_tank_tes::E_TES_T_COLD, allocate("T_tes_cold", n_steps_fixed), n_steps_fixed);
        storage.mc_reported_outputs.assign(C_csp_two_tank_tes::E_MASS_COLD_TANK, allocate("mass_tes_cold", n_steps_fixed), n_steps_fixed);
        storage.mc_reported_outputs.assign(C_csp_two_tank_tes::E_MASS_HOT_TANK, allocate("mass_tes_hot", n_steps_fixed), n_steps_fixed);

        // *****************************************************
        // *****************************************************

        // Off-taker schedule
        C_timeseries_schedule_inputs offtaker_schedule = C_timeseries_schedule_inputs(1.0, std::numeric_limits<double>::quiet_NaN());

        // Electricity pricing schedule
        C_timeseries_schedule_inputs elec_pricing_schedule;

        int etes_financial_model = as_integer("etes_financial_model");
        bool is_dispatch = as_boolean("is_dispatch");

        if (sim_type == 1) {    // if sim_type = 2, skip this until ui call back is ironed out
            if (etes_financial_model > 0 && etes_financial_model < 5) { // Single Owner financial models

                double ppa_price_year1 = std::numeric_limits<double>::quiet_NaN();

                // Get first year base ppa price
                size_t count_ppa_price_input;
                ssc_number_t* ppa_price_input_array = as_array("ppa_price_input", &count_ppa_price_input);
                ppa_price_year1 = (double)ppa_price_input_array[0];  // [$/kWh]

                // Time-of-Delivery factors by time step:
                int ppa_mult_model = as_integer("ppa_multiplier_model");
                if (ppa_mult_model == 1) {   // use dispatch_ts input

                    if (is_assigned("dispatch_factors_ts") || is_dispatch) {
                        auto vec = as_vector_double("dispatch_factors_ts");
                        elec_pricing_schedule = C_timeseries_schedule_inputs(vec, ppa_price_year1);
                    }
                    else { // if no dispatch optimization, don't need an input pricing schedule
                        elec_pricing_schedule = C_timeseries_schedule_inputs(1.0, std::numeric_limits<double>::quiet_NaN());
                    }
                }
                else if (ppa_mult_model == 0) { // standard diuranal input

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
            else {
                throw exec_error("etes_electric_resistsance", "etes_financial_model must be 1, 2, 3, 4, or 6");
            }
        }
        else if (sim_type == 2) { // if sim_type = 2, skip this until ui call back is ironed out
            elec_pricing_schedule = C_timeseries_schedule_inputs(1.0, std::numeric_limits<double>::quiet_NaN());
        }

        // Set dispatch model type
        C_csp_tou::C_dispatch_model_type::E_dispatch_model_type dispatch_model_type = C_csp_tou::C_dispatch_model_type::E_dispatch_model_type::UNDEFINED;
        if (is_dispatch) {
            dispatch_model_type = C_csp_tou::C_dispatch_model_type::E_dispatch_model_type::DISPATCH_OPTIMIZATION;
        }
        else {
            dispatch_model_type = C_csp_tou::C_dispatch_model_type::E_dispatch_model_type::ARBITRAGE_CUTOFF;
        }

        bool is_offtaker_frac_also_max = true;

        C_csp_tou tou(offtaker_schedule, elec_pricing_schedule, dispatch_model_type, is_offtaker_frac_also_max);

        //tou.mc_dispatch_params.m_is_tod_pc_target_also_pc_max = true;
        //tou.mc_dispatch_params.m_is_block_dispatch = false;
        //tou.mc_dispatch_params.m_is_arbitrage_policy = !as_boolean("is_dispatch");
        
        // *****************************************************
        // *****************************************************


        // *****************************************************
        // System performance
        C_csp_solver::S_csp_system_params system;
        system.m_pb_fixed_par = as_double("pb_fixed_par");
        system.m_bop_par = as_double("bop_par");
        system.m_bop_par_f = as_double("bop_par_f");
        system.m_bop_par_0 = as_double("bop_par_0");
        system.m_bop_par_1 = as_double("bop_par_1");
        system.m_bop_par_2 = as_double("bop_par_2");

        // *****************************************************
        // *****************************************************


        // *****************************************************
        // System dispatch
        etes_dispatch_opt dispatch;

        if (as_boolean("is_dispatch")) {
            dispatch.solver_params.set_user_inputs(as_boolean("is_dispatch"), steps_per_hour, as_integer("disp_frequency"), as_integer("disp_horizon"),
                as_integer("disp_max_iter"), as_double("disp_mip_gap"), as_double("disp_timeout"),
                as_integer("disp_spec_presolve"), as_integer("disp_spec_bb"), as_integer("disp_spec_scaling"), as_integer("disp_reporting"),
                false, false, "", "");
            dispatch.params.set_user_params(as_double("disp_time_weighting"), as_double("disp_csu_cost")*W_dot_cycle_des, as_double("disp_pen_delta_w"),
                as_double("disp_hsu_cost") * q_dot_heater_des, as_double("disp_down_time_min"), as_double("disp_up_time_min")); // , ppa_price_year1);
        }
        else {
            dispatch.solver_params.dispatch_optimize = false;
        }

        // *****************************************************
        // *****************************************************

        // *****************************************************
        // Construct System Simulation
        C_csp_solver csp_solver(weather_reader,
            c_electric_resistance,
            rankine_pc,
            storage,
            tou,        // TODO: can we refactor tou to a dispatch struct?
            dispatch,
            system,
            NULL,
            nullptr,
            ssc_cmod_update,
            (void*)(this));

        // Set system cmod outputs
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::TIME_FINAL, allocate("time_hr", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::PRICING_MULT, allocate("elec_purchase_price_mult", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::TOU_PERIOD, allocate("tou_period", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::TDRY, allocate("tdry", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::TWET, allocate("twet", n_steps_fixed), n_steps_fixed);

            // Cycle
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::W_DOT_NET, allocate("W_dot_out_net", n_steps_fixed), n_steps_fixed);

            // TES
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::TES_Q_DOT_DC, allocate("q_dot_dc_tes", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::TES_Q_DOT_CH, allocate("q_dot_ch_tes", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::TES_E_CH_STATE, allocate("e_ch_tes", n_steps_fixed), n_steps_fixed);

            // Parasitics
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::SYS_W_DOT_FIXED, allocate("W_dot_fixed_parasitics", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::SYS_W_DOT_BOP, allocate("W_dot_bop_parasitics", n_steps_fixed), n_steps_fixed);

            // Controller
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::ERR_M_DOT, allocate("m_dot_balance", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::ERR_Q_DOT, allocate("q_balance", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::N_OP_MODES, allocate("n_op_modes", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::OP_MODE_1, allocate("op_mode_1", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::OP_MODE_2, allocate("op_mode_2", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::OP_MODE_3, allocate("op_mode_3", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::PC_Q_DOT_TARGET, allocate("q_pc_target", n_steps_fixed), n_steps_fixed);

        // DISPATCH
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_REL_MIP_GAP,  allocate("disp_rel_mip_gap", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_SOLVE_STATE,  allocate("disp_solve_state", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_SUBOPT_FLAG, allocate("disp_subopt_flag", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_SOLVE_ITER,   allocate("disp_solve_iter", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_SOLVE_OBJ,    allocate("disp_objective", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_SOLVE_OBJ_RELAX, allocate("disp_obj_relax", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_QSFPROD_EXPECT, allocate("disp_qsfprod_expected", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_QSFSU_EXPECT, allocate("disp_qsfsu_expected", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_TES_EXPECT, allocate("disp_tes_expected", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_PCEFF_EXPECT, allocate("disp_pceff_expected", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_QPBSU_EXPECT, allocate("disp_qpbsu_expected", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_WPB_EXPECT, allocate("disp_wpb_expected", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_REV_EXPECT,   allocate("disp_rev_expected", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_PRES_NCONSTR, allocate("disp_presolve_nconstr", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_PRES_NVAR,    allocate("disp_presolve_nvar", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_SOLVE_TIME,   allocate("disp_solve_time", n_steps_fixed), n_steps_fixed);

        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::CTRL_OP_MODE_SEQ_A, allocate("operating_modes_a", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::CTRL_OP_MODE_SEQ_B, allocate("operating_modes_b", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::CTRL_OP_MODE_SEQ_C, allocate("operating_modes_c", n_steps_fixed), n_steps_fixed);


        // *****************************************************
        // *****************************************************


        // *****************************************************
        // Initialize
        update("Initialize ETES electric resistance heating model...", 0.0);

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

            throw exec_error("etes_electric_resistance", csp_exception.m_error_message);
        }

        // If no exception, then report messages
        while (csp_solver.mc_csp_messages.get_message(&out_type, &out_msg))
        {
            log(out_msg, out_type);
        }
        // *****************************************************
        // *****************************************************


        // *****************************************************
        // System design is complete, get design parameters from component models as necessary

            // Cycle
        double m_dot_htf_pc_des;    //[kg/s]
        double cp_htf_pc_des;       //[kJ/kg-K]
        double W_dot_pc_pump_des;   //[MWe]
        double W_dot_cooling_des;   //[MWe]
        int n_T_htf_pars, n_T_amb_pars, n_m_dot_pars;
        n_T_htf_pars = n_T_amb_pars = n_m_dot_pars = -1;
        double T_htf_ref_calc, T_htf_low_calc, T_htf_high_calc, T_amb_ref_calc, T_amb_low_calc, T_amb_high_calc,
            m_dot_htf_ND_ref_calc, m_dot_htf_ND_low_calc, m_dot_htf_ND_high_calc, W_dot_gross_ND_des, Q_dot_HTF_ND_des,
            W_dot_cooling_ND_des, m_dot_water_ND_des;
        T_htf_ref_calc = T_htf_low_calc = T_htf_high_calc =
            T_amb_ref_calc = T_amb_low_calc = T_amb_high_calc =
            m_dot_htf_ND_ref_calc = m_dot_htf_ND_low_calc = m_dot_htf_ND_high_calc =
            W_dot_gross_ND_des = Q_dot_HTF_ND_des = W_dot_cooling_ND_des = m_dot_water_ND_des = std::numeric_limits<double>::quiet_NaN();

        rankine_pc.get_design_parameters(m_dot_htf_pc_des, cp_htf_pc_des, W_dot_pc_pump_des, W_dot_cooling_des,
            n_T_htf_pars, n_T_amb_pars, n_m_dot_pars,
            T_htf_ref_calc /*C*/, T_htf_low_calc /*C*/, T_htf_high_calc /*C*/,
            T_amb_ref_calc /*C*/, T_amb_low_calc /*C*/, T_amb_high_calc /*C*/,
            m_dot_htf_ND_ref_calc, m_dot_htf_ND_low_calc /*-*/, m_dot_htf_ND_high_calc /*-*/,
            W_dot_gross_ND_des, Q_dot_HTF_ND_des, W_dot_cooling_ND_des, m_dot_water_ND_des);
        m_dot_htf_pc_des /= 3600.0;     // convert from kg/hr to kg/s

            // Heater
        double E_heater_su_des;         //[MWt-hr]
        double W_dot_heater_des_calc;   //[MWe]
        c_electric_resistance.get_design_parameters(E_heater_su_des, W_dot_heater_des_calc);

            // TES
        double V_tes_htf_avail /*m3*/, V_tes_htf_total /*m3*/, d_tank /*m*/, q_dot_loss_tes_des /*MWt*/, dens_store_htf_at_T_ave /*kg/m3*/, Q_tes_des_tes_class, tes_total_mass /*kh*/;
        storage.get_design_parameters(V_tes_htf_avail, V_tes_htf_total, d_tank,
            q_dot_loss_tes_des, dens_store_htf_at_T_ave, Q_tes_des_tes_class, tes_total_mass);

            // System
        double W_dot_bop_design;    //[MWe]
        double W_dot_fixed_parasitic_design;    //[MWe]
        csp_solver.get_design_parameters(W_dot_bop_design, W_dot_fixed_parasitic_design);

        double plant_net_capacity_des_calc = W_dot_cycle_des - W_dot_pc_pump_des - W_dot_cooling_des -
                                W_dot_bop_design - W_dot_fixed_parasitic_design;    //[MWe]

        double plant_net_conv_calc = plant_net_capacity_des_calc / W_dot_cycle_des; //[-]
        double system_capacity = plant_net_capacity_des_calc * 1.E3;        //[kWe]

        // *****************************************************
        // System design is complete, so calculate final design outputs like cost, capacity, etc.
        double tes_spec_cost = as_double("tes_spec_cost");
        double power_cycle_spec_cost = as_double("cycle_spec_cost");
        double heater_spec_cost = as_double("heater_spec_cost");
        double bop_spec_cost = as_double("bop_spec_cost");
        double contingency_rate = as_double("contingency_rate");

        // no Cold Temp TES, so set those cost model inputs to 0
        double Q_CT_tes = 0.0;
        double CT_tes_spec_cost = 0.0;

        double EPC_perc_direct_cost = as_double("epc_cost_perc_of_direct");
        double EPC_per_power_cost = as_double("epc_cost_per_watt");
        double EPC_fixed_cost = as_double("epc_cost_fixed");
        double total_land_perc_direct_cost = as_double("land_cost_perc_of_direct");
        double total_land_per_power_cost = as_double("land_cost_per_watt");
        double total_land_fixed_cost = as_double("land_cost_fixed");
        double sales_tax_basis = as_double("sales_tax_frac");
        double sales_tax_rate = as_double("sales_tax_rate");

        // Cost model outputs
        double tes_cost, CT_tes_cost, power_cycle_cost, heater_cost, bop_cost, fossil_backup_cost,
            direct_capital_precontingency_cost, contingency_cost, total_direct_cost, total_land_cost,
            epc_and_owner_cost, sales_tax_cost, total_indirect_cost, total_installed_cost, estimated_installed_cost_per_cap;
        tes_cost = CT_tes_cost = power_cycle_cost = heater_cost = bop_cost = fossil_backup_cost =
            direct_capital_precontingency_cost = contingency_cost = total_direct_cost = total_land_cost =
            epc_and_owner_cost = sales_tax_cost = total_indirect_cost = total_installed_cost = estimated_installed_cost_per_cap = std::numeric_limits<double>::quiet_NaN();

        N_mspt::calculate_etes_costs(Q_tes_des, tes_spec_cost, Q_CT_tes, CT_tes_spec_cost,
            W_dot_cycle_des, power_cycle_spec_cost,
            q_dot_heater_des, heater_spec_cost, bop_spec_cost, contingency_rate,
            plant_net_capacity_des_calc, EPC_perc_direct_cost, EPC_per_power_cost, EPC_fixed_cost,
            total_land_perc_direct_cost, total_land_per_power_cost, total_land_fixed_cost,
            sales_tax_basis, sales_tax_rate,
            tes_cost, CT_tes_cost, power_cycle_cost, heater_cost, bop_cost, direct_capital_precontingency_cost,
            contingency_cost, total_direct_cost, total_land_cost, epc_and_owner_cost,
            sales_tax_cost, total_indirect_cost, total_installed_cost, estimated_installed_cost_per_cap);

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

        // Assign cmod variables that are required by downstream models or represent design-point
            // System
        assign("system_capacity", (ssc_number_t)system_capacity);           //[kWe]
        assign("nameplate", (ssc_number_t)(system_capacity * 1.E-3));       //[MWe]
        assign("cp_system_nameplate", system_capacity * 1.E-3);             //[MWe]
        assign("cp_battery_nameplate", system_capacity * 1.E-3);             //[MWe]
        assign("q_pb_design", (ssc_number_t)q_dot_pc_des);                  //[MWt]
        assign("q_dot_heater_design", (ssc_number_t)q_dot_heater_des);      //[MWt]
        assign("tshours_heater", (ssc_number_t)(tshours / heater_mult));    //[hr]
        assign("Q_tes_des", (ssc_number_t)Q_tes_des);                       //[MWt-hr]

            // Cycle
        assign("m_dot_htf_cycle_des", (ssc_number_t)m_dot_htf_pc_des);            //[kg/s]
        assign("cp_htf_cycle_des", (ssc_number_t)cp_htf_pc_des);                  //[kJ/kg-K]
            // UDPC
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

            // Heater
        assign("W_dot_heater_des", (ssc_number_t)W_dot_heater_des_calc);    //[MWe]
        assign("E_heater_su_des", (ssc_number_t)E_heater_su_des);           //[MWt-hr]

            // TES
        assign("V_tes_htf_avail", V_tes_htf_avail);         //[m3]
        assign("V_tes_htf_total", V_tes_htf_total);         //[m3]
        assign("d_tank_tes", d_tank);                       //[m]
        assign("q_dot_loss_tes_des", q_dot_loss_tes_des);   //[kg/m3]
        assign("dens_store_htf_at_T_ave", dens_store_htf_at_T_ave); //[kg/m3]

            // System
        assign("W_dot_bop_design", W_dot_bop_design);       //[MWe]

            // Costs
        assign("heater_cost_calc", (ssc_number_t)heater_cost);
        assign("tes_cost_calc", (ssc_number_t)tes_cost);
        assign("bop_cost_calc", (ssc_number_t)bop_cost);
        assign("cycle_cost_calc", (ssc_number_t)power_cycle_cost);
        assign("direct_subtotal_cost_calc", (ssc_number_t)direct_capital_precontingency_cost);
        assign("contingency_cost_calc", (ssc_number_t)contingency_cost);
        assign("total_direct_cost_calc", (ssc_number_t)total_direct_cost);
        assign("epc_cost_calc", (ssc_number_t)epc_and_owner_cost);
        assign("land_cost_calc", (ssc_number_t)total_land_cost);
        assign("sales_tax_cost_calc", (ssc_number_t)sales_tax_cost);
        assign("total_indirect_cost_calc", (ssc_number_t)total_indirect_cost);
        assign("installed_per_cap_cost_calc", (ssc_number_t)(total_installed_cost / system_capacity));
        assign("total_installed_cost", (ssc_number_t)total_installed_cost);                 //[$]
        assign("construction_financing_cost", (ssc_number_t)construction_financing_cost);   //[$]

            // Financial
        assign("ppa_soln_mode", 1);     // Only allow dispatch model to use fixed ppa mode so dispatch model knows absolute prices
        assign("flip_target_percent", 0.0); //[%] fixed ppa mode shouldn't use this input, so set it to a value that, if used, will give weird results
            // ETES model assumes no land area. Can calculate land costs based on % or absolute values
        assign("total_land_area", 0);   //[acre]

        // *****************************************************
        // *****************************************************

        // *****************************************************
        // If calling cmod to run design only, return here
        if (as_double("sim_type") != 1) {
            return;
        }
        // *****************************************************
        // *****************************************************


        // *****************************************************
        // Run timeseries simulation
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

            throw exec_error("etes_electric_resistance", csp_exception.m_error_message);
        }

        // If no exception, then report messages
        while (csp_solver.mc_csp_messages.get_message(&out_type, &out_msg))
        {
            log(out_msg, out_type);
        }
        // *****************************************************
        // *****************************************************


        // *****************************************************
        // Post-process
        size_t count;
        ssc_number_t* p_W_dot_net = as_array("W_dot_out_net", &count);
        ssc_number_t* p_time_final_hr = as_array("time_hr", &count);

        // 'adjustment_factors' class stores factors in hourly array, so need to index as such
        adjustment_factors haf(this->get_var_table(), "adjust");
        if (!haf.setup(n_steps_fixed))
            throw exec_error("etes_electric_resistance", "failed to setup adjustment factors: " + haf.error());

        ssc_number_t* p_gen = allocate("gen", count);
        for (size_t i = 0; i < count; i++)
        {
            size_t hour = (size_t)ceil(p_time_final_hr[i]);
            p_gen[i] = (ssc_number_t)(p_W_dot_net[i] * 1.E3 * haf(hour));           //[kWe]
        }
        // *****************************************************
        // *****************************************************


        // *****************************************************
        // Report simulation metrics
        ssc_number_t* p_annual_energy_dist_time = gen_heatmap(this, steps_per_hour);

        // Post-process hourly values
        ssc_number_t* p_eta_cycle_net = allocate("eta_cycle_net", n_steps_fixed);
        ssc_number_t* p_W_dot_cycle_net = allocate("W_dot_cycle_net", n_steps_fixed);
        size_t count_q_dot_cycle, count_W_dot_cycle, count_W_dot_cycle_cooling, count_m_dot_htf_cycle, count_m_dot_water_cycle;
        ssc_number_t* p_q_dot_cycle = as_array("q_dot_cycle", &count_q_dot_cycle);
        ssc_number_t* p_W_dot_cycle_gross = as_array("W_dot_cycle_gross", &count_W_dot_cycle);
        ssc_number_t* p_W_dot_cycle_cooling = as_array("W_dot_cycle_cooling", &count_W_dot_cycle_cooling);
        ssc_number_t* p_m_dot_htf_cycle = as_array("m_dot_htf_cycle", &count_m_dot_htf_cycle);
        ssc_number_t* p_m_dot_water_cycle = as_array("m_dot_water_cycle", &count_m_dot_water_cycle);
        if (count_q_dot_cycle != n_steps_fixed || count_W_dot_cycle != n_steps_fixed || count_W_dot_cycle_cooling != n_steps_fixed
            || count_m_dot_htf_cycle != n_steps_fixed || count_m_dot_water_cycle != n_steps_fixed ) {
            log("cycle output arrays are different lengths than 'n_steps_fixed'.", SSC_WARNING);
            return;
        }
        for (size_t i = 0; i < n_steps_fixed; i++) {
            p_W_dot_cycle_net[i] = p_W_dot_cycle_gross[i] - p_W_dot_cycle_cooling[i];     //[MWe]
            if (p_q_dot_cycle[i] > 0.0) {
                p_eta_cycle_net[i] = p_W_dot_cycle_net[i] / p_q_dot_cycle[i];
            }
            else {
                p_eta_cycle_net[i] = 0.0;
            }

            // Scale mass flows from kg/hr to kg/s
            p_m_dot_htf_cycle[i] /= 3600.0;
            p_m_dot_water_cycle[i] /= 3600.0;
        }

        // Annual metrics
        accumulate_annual_for_year("gen", "annual_energy", sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour);
        accumulate_annual_for_year("W_dot_out_net", "annual_energy_full_availability", sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour);
        accumulate_annual_for_year("W_dot_heater", "annual_E_heater", sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour); //[MWhe]
        accumulate_annual_for_year("q_dot_heater_to_htf", "annual_Q_heater_to_htf", sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour); //[MWhe]
        accumulate_annual_for_year("q_dot_heater_startup", "annual_Q_heater_startup", sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour); //[MWht]
        accumulate_annual_for_year("q_dot_tes_heater", "annual_E_tes_heater", sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour); //[MWht]
        accumulate_annual_for_year("q_dot_tes_losses", "annual_Q_tes_losses", sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour); //[MWht]
        accumulate_annual_for_year("W_dot_cycle_gross", "annual_E_cycle_gross", sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour); //[MWhe]
        accumulate_annual_for_year("q_dot_cycle", "annual_Q_cycle_thermal_in", sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour); //[MWht]
        accumulate_annual_for_year("q_dot_cycle_startup", "annual_Q_cycle_thermal_startup", sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour); //[MWht]

        accumulate_annual_for_year("disp_objective", "disp_objective_ann", sim_setup.m_report_step / 3600.0 / as_double("disp_frequency"), steps_per_hour, 1, n_steps_fixed / steps_per_hour);
        accumulate_annual_for_year("disp_solve_iter", "disp_iter_ann", sim_setup.m_report_step / 3600.0 / as_double("disp_frequency"), steps_per_hour, 1, n_steps_fixed / steps_per_hour);
        accumulate_annual_for_year("disp_presolve_nconstr", "disp_presolve_nconstr_ann", sim_setup.m_report_step / 3600.0 / as_double("disp_frequency"), steps_per_hour, 1, n_steps_fixed / steps_per_hour);
        accumulate_annual_for_year("disp_presolve_nvar", "disp_presolve_nvar_ann", sim_setup.m_report_step / 3600.0 / as_double("disp_frequency"), steps_per_hour, 1, n_steps_fixed / steps_per_hour);
        accumulate_annual_for_year("disp_solve_time", "disp_solve_time_ann", sim_setup.m_report_step / 3600. / as_double("disp_frequency"), steps_per_hour, 1, n_steps_fixed / steps_per_hour);
        accumulate_annual_for_year("disp_solve_state", "disp_solve_state_ann", sim_setup.m_report_step / 3600. / as_double("disp_frequency"), steps_per_hour, 1, n_steps_fixed / steps_per_hour);

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

        // *****************************************************
        // Check annual metrics
        double E_heater = as_double("annual_E_heater");
        double Q_heater_to_htf = as_double("annual_Q_heater_to_htf");
        double Q_tes_heater = as_double("annual_E_tes_heater");
        double Q_tes_losses = as_double("annual_Q_tes_losses");
        double Q_cycle_thermal_in = as_double("annual_Q_cycle_thermal_in");

        double q_balance_rel = (Q_heater_to_htf + Q_tes_heater - Q_tes_losses - Q_cycle_thermal_in) / Q_cycle_thermal_in;

        std::clock_t clock_end = std::clock();
        double sim_cpu_run_time = (clock_end - clock_start) / (double)CLOCKS_PER_SEC;		//[s]
        assign("sim_cpu_run_time", sim_cpu_run_time);   //[s]
    }
};

DEFINE_MODULE_ENTRY(etes_electric_resistance, "Electric resistance heater charging TES from grid, discharge with power cycle", 1)
