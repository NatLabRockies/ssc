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
#include "csp_solver_core.h"

#include "csp_solver_pc_ptes.h"
#include "csp_solver_cr_heat_pump.h"
#include "csp_solver_two_tank_tes.h"
#include "etes_dispatch.h"

#include "csp_system_costs.h"

static var_info _cm_vtab_etes_ptes[] = {

    // Resource Data
    { SSC_INPUT,  SSC_STRING, "solar_resource_file",           "Local weather file path",                                        "",             "",                                  "Solar Resource",                           "?",                                                                "LOCAL_FILE",    ""},

    // Simulation Parameters
    { SSC_INPUT,  SSC_NUMBER, "is_dispatch",                   "Allow dispatch optimization?",                                   "",             "",                                  "System Control",                           "?=0",                                                              "",               ""},
    { SSC_INPUT,  SSC_NUMBER, "sim_type",                      "1 (default): timeseries, 2: design only",                        "",             "",                                  "System Control",                           "?=1",                                                              "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,  SSC_NUMBER, "etes_financial_model",          "",                                                               "1-8",          "",                                  "Financial Model",                          "?=1",                                                              "INTEGER,MIN=0", ""},
    { SSC_INPUT,  SSC_NUMBER, "time_start",                    "Simulation start time",                                          "s",            "",                                  "System Control",                           "?=0",                                                              "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,  SSC_NUMBER, "time_stop",                     "Simulation stop time",                                           "s",            "",                                  "System Control",                           "?=31536000",                                                       "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,  SSC_NUMBER, "time_steps_per_hour",           "Number of simulation time steps per hour",                       "",             "",                                  "System Control",                           "?=-1",                                                             "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,  SSC_NUMBER, "vacuum_arrays",                 "Allocate arrays for only the required number of steps",          "",             "",                                  "System Control",                           "?=0",                                                              "",              "SIMULATION_PARAMETER"},

    // System Parameters
    { SSC_INPUT,  SSC_NUMBER, "heater_mult",                   "Heater multiple relative to design cycle thermal power",         "-",            "",                                  "System Design",                            "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "tshours",                       "Equivalent full-load thermal storage hours",                     "hr",           "",                                  "System Design",                            "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "W_dot_pc_thermo_des",           "PC design thermodynamic power",                                  "MWe",          "",                                  "System Design",                            "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "eta_pc_thermo_des",             "PC design thermodynamic efficiency",                             "-",            "",                                  "System Design",                            "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "f_pc_parasitic_des",            "PC parasitics as fraction of design thermo power out",           "-",            "",                                  "System Design",                            "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "cop_hp_thermo_des",             "Heat pump design thermodynamic heat COP",                        "-",            "",                                  "System Design",                            "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "f_hp_parasitic_des",            "Heat pump parasitics as fraction of design thermo power in",     "-",            "",                                  "System Design",                            "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "T_HT_hot_htf_des",              "HT TES hot temperature",                                         "C",            "",                                  "System Design",                            "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "T_HT_cold_htf_des",             "HT TES cold temperature",                                        "C",            "",                                  "System Design",                            "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "T_CT_cold_htf_des",             "CT TES cold temperature",                                        "C",            "",                                  "System Design",                            "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "T_CT_hot_htf_des",              "CT TES hot temperature",                                         "C",            "",                                  "System Design",                            "*",                                                                "",              ""},

    // HTFs
    { SSC_INPUT,  SSC_NUMBER, "hot_htf_code",                  "Hot HTF code - see htf_props.h for list",                        "",             "",                                  "Thermal Storage",                          "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_MATRIX, "ud_hot_htf_props",              "User-defined Hot HTF fluid property data",                       "-",            "",                                  "Thermal Storage",                          "hot_htf_code=50",                                                  "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "cold_htf_code",                 "Cold HTF code - see htf_props.h for list",                       "",             "",                                  "Thermal Storage",                          "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_MATRIX, "ud_cold_htf_props",             "User-defined Cold HTF fluid property data",                      "-",            "",                                  "Thermal Storage",                          "cold_htf_code=50",                                                 "",              ""},


    // Heat Pump
    { SSC_INPUT,  SSC_NUMBER, "f_q_dot_des_allowable_su",      "Fraction of design power allowed during startup",                "-",            "",                                  "Heater",                                   "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "hrs_startup_at_max_rate",       "Duration of startup at max startup power",                       "hr",           "",                                  "Heater",                                   "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "f_q_dot_heater_min",            "Minimum allowable heater output as fraction of design",          "",             "",                                  "Heater",                                   "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "heat_pump_HT_HTF_pump_coef",    "High temp HX pumping power to move 1 kg/s",                      "kW/kg/s",      "",                                  "Power Cycle",                              "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "heat_pump_CT_HTF_pump_coef",    "Cold temp HX pumping power to move 1 kg/s",                      "kW/kg/s",      "",                                  "Power Cycle",                              "*",                                                                "",              ""},


    // Power Cycle
        // General
    { SSC_INPUT,  SSC_NUMBER, "pb_pump_coef",                  "COLD TES pumping power to move 1kg of HTF through PB loop",      "kW/kg/s",      "",                                  "Power Cycle",                              "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "CT_pb_pump_coef",               "COLD TES pumping power to move 1kg of HTF through PB loop",      "kW/kg/s",      "",                                  "Power Cycle",                              "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "startup_time",                  "Time needed for power block startup",                            "hr",           "",                                  "Power Cycle",                              "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "startup_frac",                  "Fraction of design thermal power needed for startup",            "none",         "",                                  "Power Cycle",                              "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "cycle_max_frac",                "Maximum turbine over design operation fraction",                 "",             "",                                  "Power Cycle",                              "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "cycle_cutoff_frac",             "Minimum turbine operation fraction before shutdown",             "",             "",                                  "Power Cycle",                              "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "q_sby_frac",                    "Fraction of thermal power required for standby",                 "",             "",                                  "Power Cycle",                              "*",                                                                "",              ""},


    // High Temp Two-Tank TES
    { SSC_INPUT,  SSC_NUMBER, "tes_init_hot_htf_percent",      "HOT TES Initial fraction of available volume that is hot",       "%",            "",                                  "Hot Thermal Storage",                      "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "h_tank",                        "HOT TES Total height of tank (height of HTF when tank is full)", "m",            "",                                  "Hot Thermal Storage",                      "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "cold_tank_max_heat",            "HOT TES Rated heater capacity for cold tank heating",            "MW",           "",                                  "Hot Thermal Storage",                      "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "u_tank",                        "HOT TES Loss coefficient from the tank",                         "W/m2-K",       "",                                  "Hot Thermal Storage",                      "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "tank_pairs",                    "HOT TES Number of equivalent tank pairs",                        "",             "",                                  "Hot Thermal Storage",                      "*",                                                                "INTEGER",       ""},
    { SSC_INPUT,  SSC_NUMBER, "cold_tank_Thtr",                "HOT TES Minimum allowable cold tank HTF temperature",            "C",            "",                                  "Hot Thermal Storage",                      "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "h_tank_min",                    "HOT TES Minimum allowable HTF height in storage tank",           "m",            "",                                  "Hot Thermal Storage",                      "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "hot_tank_Thtr",                 "HOT TES Minimum allowable hot tank HTF temperature",             "C",            "",                                  "Hot Thermal Storage",                      "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "hot_tank_max_heat",             "HOT TES Rated heater capacity for hot tank heating",             "MW",           "",                                  "Hot Thermal Storage",                      "*",                                                                "",              ""},


    // COLD Temp Two-Tank TES
    { SSC_INPUT,  SSC_NUMBER, "CT_h_tank",                     "COLD TES Total height of tank (height of HTF when tank is full)","m",            "",                                  "Cold Thermal Storage",                     "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "CT_u_tank",                     "COLD TES Loss coefficient from the tank",                        "W/m2-K",       "",                                  "Cold Thermal Storage",                     "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "CT_tank_pairs",                 "COLD TES Number of equivalent tank pairs",                       "",             "",                                  "Cold Thermal Storage",                     "*",                                                                "INTEGER",       ""},
    { SSC_INPUT,  SSC_NUMBER, "CT_h_tank_min",                 "COLD TES Minimum allowable HTF height in storage tank",          "m",            "",                                  "Cold Thermal Storage",                     "*",                                                                "",              ""},
    //{ SSC_INPUT,  SSC_NUMBER, "CT_cold_tank_max_heat",         "COLD TES Rated heater capacity for cold tank heating",           "MW",           "",                                  "Cold Thermal Storage",                     "*",                                                                "",              ""},
    //{ SSC_INPUT,  SSC_NUMBER, "CT_cold_tank_Thtr",             "COLD TES Minimum allowable cold tank HTF temperature",           "C",            "",                                  "Cold Thermal Storage",                     "*",                                                                "",              ""},
    //{ SSC_INPUT,  SSC_NUMBER, "CT_hot_tank_Thtr",              "COLD TES Minimum allowable hot tank HTF temperature",            "C",            "",                                  "Cold Thermal Storage",                     "*",                                                                "",              ""},
    //{ SSC_INPUT,  SSC_NUMBER, "CT_hot_tank_max_heat",          "COLD TES Rated heater capacity for hot tank heating",            "MW",           "",                                  "Cold Thermal Storage",                     "*",                                                                "",              ""},


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


    // Pricing schedules and multipliers
    { SSC_INPUT,  SSC_NUMBER, "ppa_multiplier_model",          "PPA multiplier model",                                          "0/1",          "0=diurnal,1=timestep",              "Time of Delivery Factors",                 "?=0",                                                              "INTEGER,MIN=0", "SIMULATION_PARAMETER"},
    { SSC_INPUT,  SSC_ARRAY,  "dispatch_factors_ts",           "Dispatch payment factor timeseries array",                      "",             "",                                  "Time of Delivery Factors",                 "ppa_multiplier_model=1&etes_financial_model<5&is_dispatch=1&sim_type=1",      "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,  SSC_MATRIX, "dispatch_sched_weekday",        "PPA pricing weekday schedule, 12x24",                           "",             "",                                  "Time of Delivery Factors",                 "ppa_multiplier_model=0&etes_financial_model<5&is_dispatch=1&sim_type=1",      "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,  SSC_MATRIX, "dispatch_sched_weekend",        "PPA pricing weekend schedule, 12x24",                           "",             "",                                  "Time of Delivery Factors",                 "ppa_multiplier_model=0&etes_financial_model<5&is_dispatch=1&sim_type=1",      "",              "SIMULATION_PARAMETER"},
    { SSC_INPUT,  SSC_ARRAY,  "dispatch_tod_factors",          "TOD factors for periods 1 through 9",                           "",
        "We added this array input after SAM 2022.12.21 to replace the functionality of former single value inputs dispatch_factor1 through dispatch_factor9",                       "Time of Delivery Factors",                 "ppa_multiplier_model=0&etes_financial_model<5&is_dispatch=1&sim_type=1",       "",              "SIMULATION_PARAMETER" },

    { SSC_INPUT,  SSC_ARRAY,  "ppa_price_input",			   "PPA prices - yearly",			                                "$/kWh",	    "",	                                 "Revenue",			                         "ppa_multiplier_model=0&etes_financial_model<5&is_dispatch=1&sim_type=1",      "",      	     "SIMULATION_PARAMETER"},


    // System performance
    { SSC_INPUT,  SSC_NUMBER, "pb_fixed_par",                  "Fixed parasitic load that don't generate heat - runs at all times","MWe/MWcap", "",                                  "System Control",                           "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "bop_par",                       "Balance of plant parasitic power fraction",                        "MWe/MWcap", "",                                  "System Control",                           "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "bop_par_f",                     "Balance of plant parasitic power fraction - mult frac",            "",          "",                                  "System Control",                           "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "bop_par_0",                     "Balance of plant parasitic power fraction - const coeff",          "",          "",                                  "System Control",                           "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "bop_par_1",                     "Balance of plant parasitic power fraction - linear coeff",         "",          "",                                  "System Control",                           "*",                                                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "bop_par_2",                     "Balance of plant parasitic power fraction - quadratic coeff",      "",          "",                                  "System Control",                           "*",                                                                "",              ""},

    // System costs
    { SSC_INPUT,  SSC_NUMBER, "cycle_spec_cost",               "Power cycle specific cost",                                     "$/kWe",        "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,  SSC_NUMBER, "tes_spec_cost",                 "Hot Temp thermal energy storage specific cost",                 "$/kWht",       "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,  SSC_NUMBER, "CT_tes_spec_cost",              "Cold Temp thermal energy storage specific cost",                "$/kWht",       "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_INPUT,  SSC_NUMBER, "heat_pump_spec_cost",           "Heater pump specific cost",                                     "$/kWht",       "",                                  "System Costs",                             "*",                                                                "",              "" },
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
        // Design Outputs here:
        // ****************************************************************************************************************************************
            // System
    { SSC_OUTPUT, SSC_NUMBER, "system_capacity",                 "System capacity (discharge)",                         "kWe",          "",                                  "System Design Calc",             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "nameplate",                       "Nameplate capacity (discharge)",                      "MWe",          "",                                  "System Design Calc",             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "cp_system_nameplate",              "System capacity for capacity payments",               "MWe",          "",                                  "System Design Calc",             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "cp_battery_nameplate",             "Battery nameplate",                                   "MWe",          "",                                  "System Design Calc",             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "rte_thermo",                      "Round-trip efficiency of working fluid cycles",       "MWe",          "",                                  "System Design Calc",             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "rte_net",                         "Net round-trip efficiency considering all parasitics","MWe",          "",                                  "System Design Calc",             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "charge_capacity",                 "Total electricity consumption at design-point charge","MWe",          "",                                  "System Design Calc",             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "tshours_heater",                  "Hours of TES relative to heater output",              "hr",           "",                                  "System Design Calc",             "*",                                                                "",              "" },

            // Heat pump                                            
    { SSC_OUTPUT, SSC_NUMBER, "W_dot_hp_in_thermo_des",          "Heat pump power into working fluid",                  "MWe",          "",                                  "Heat Pump",                      "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "q_dot_hp_hot_out_des",            "Heat pump heat output",                               "MWt",          "",                                  "Heat Pump",                      "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "q_dot_hp_cold_in_des",            "Heat pump heat input",                                "MWt",          "",                                  "Heat Pump",                      "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "W_dot_hp_elec_parasitic_des",     "Heat pump parasitic power",                           "MWe",          "",                                  "Heat Pump",                      "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "W_dot_hp_in_net_des",             "Heat pump total power consumption",                   "MWe",          "",                                  "Heat Pump",                      "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "COP_net_des",                     "Heat pump net COP",                                   "MWe",          "",                                  "Heat Pump",                      "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "m_dot_hp_HT_htf_des",             "Heat pump HT HTF mass flow rate",                     "kg/s",         "",                                  "Heat Pump",                      "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "W_dot_hp_HT_htf_pump_des",        "Heat pump HT HTF pump power",                         "MWe",          "",                                  "Heat Pump",                      "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "m_dot_hp_CT_htf_des",             "Heat pump CT HTF mass flow rate",                     "kg/s",         "",                                  "Heat Pump",                      "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "W_dot_hp_CT_htf_pump_des",        "Heat pump CT HTF pump power",                         "MWe",          "",                                  "Heat Pump",                      "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "E_hp_su_des",                     "Heat pump startup energy",                            "MWt-hr",       "",                                  "Heat Pump",                      "*",                                                                "",              "" },

            // Power cycle
    { SSC_OUTPUT, SSC_NUMBER, "q_dot_pc_hot_in_des",             "Cycle heat input",                                    "MWt",          "",                                  "Cycle",                          "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "q_dot_pc_cold_out_thermo_des",    "Cycle total heat rejection",                          "MWt",          "",                                  "Cycle",                          "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "W_dot_pc_elec_parasitic_des",     "Cycle parasitic power",                               "MWe",          "",                                  "Cycle",                          "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "W_dot_pc_net_des",                "Cycle net power generation",                          "MWe",          "",                                  "Cycle",                          "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "eta_pc_net_des",                  "Cycle net efficiency",                                "-",            "",                                  "Cycle",                          "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "q_dot_pc_cold_to_CTES_des",       "Cycle heat to cold TES",                              "MWt",          "",                                  "Cycle",                          "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "q_dot_pc_cold_to_surroundings_des", "Cycle heat to surroundings",                        "MWt",          "",                                  "Cycle",                          "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "m_dot_pc_HT_htf_des",             "Cycle HT HTF mass flow rate",                         "kg/s",         "",                                  "Cycle",                          "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "W_dot_pc_HT_htf_pump_des",        "Cycle HT HTF pump power",                             "MWe",          "",                                  "Cycle",                          "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "m_dot_pc_CT_htf_des",             "Cycle CT HTF mass flow rate",                         "kg/s",         "",                                  "Cycle",                          "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "W_dot_pc_CT_htf_pump_des",        "Cycle CT HTF pump power",                             "MWe",          "",                                  "Cycle",                          "*",                                                                "",              "" },

            // Hot TES
    { SSC_OUTPUT, SSC_NUMBER, "Q_tes_des",                       "TES design capacity",                                 "MWt-hr",       "",                                  "TES Design Calc",                "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "V_tes_htf_avail",                 "Volume of TES HTF available for heat transfer",       "m3",           "",                                  "TES Design Calc",                "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "V_tes_htf_total",                 "Total TES HTF volume",                                "m3",           "",                                  "TES Design Calc",                "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "d_tank_tes",                      "Diameter of TES tank",                                "m",            "",                                  "TES Design Calc",                "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "q_dot_loss_tes_des",              "TES thermal loss at design",                          "MWt",          "",                                  "TES Design Calc",                "*",                                                                "",              "" },

            // Cold TES
    { SSC_OUTPUT, SSC_NUMBER, "Q_CT_tes_des",                    "Cold TES design capacity",                            "MWt-hr",       "",                                  "TES Design Calc",                "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "V_CT_tes_htf_avail",              "Volume of cold TES HTF available for heat transfer",  "m3",           "",                                  "TES Design Calc",                "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "V_CT_tes_htf_total",              "Total cold TES HTF volume",                           "m3",           "",                                  "TES Design Calc",                "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "d_CT_tank_tes",                   "Diameter of cold TES tank",                           "m",            "",                                  "TES Design Calc",                "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "q_dot_loss_CT_tes_des",           "Cold TES thermal loss at design",                     "MWt",          "",                                  "TES Design Calc",                "*",                                                                "",              "" },

            // Balance of Plant
    { SSC_OUTPUT, SSC_NUMBER, "W_dot_bop_design",                "BOP parasitics at design",                            "MWe",          "",                                  "Balance of Plant",               "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "W_dot_fixed",                     "Fixed parasitic at design",                           "MWe",          "",                                  "Balance of Plant",               "*",                                                                "",              "" },

            // Costs
    { SSC_OUTPUT, SSC_NUMBER, "heater_cost_calc",                "Heater cost",                             "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "tes_cost_calc",                   "TES cost",                                "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "CT_tes_cost_calc",                "Cold TES cost",                           "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "bop_cost_calc",                   "BOP cost",                                "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "cycle_cost_calc",                 "Cycle cost",                              "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "direct_subtotal_cost_calc",       "Direct subtotal cost",                    "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "contingency_cost_calc",           "Contingency cost",                        "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "total_direct_cost_calc",          "Total direct cost",                       "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "epc_cost_calc",                   "EPC cost",                                "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "land_cost_calc",                  "Land cost",                               "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "sales_tax_cost_calc",             "Sales tax cost",                          "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "total_indirect_cost_calc",        "Total indirect cost",                     "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "installed_per_cap_cost_calc",     "Installed cost per capacity",             "$/kWe",        "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "total_installed_cost",            "Total installed cost",                    "$",            "",                                  "System Costs",                             "*",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "construction_financing_cost",     "Total construction financing cost",       "$",            "",                                  "Financial Parameters",                     "*",                                                                "",              "" },

    // ****************************************************************************************************************************************
    // Timeseries Simulation Outputs here:
    // ****************************************************************************************************************************************
            // Simulation outputs
    { SSC_OUTPUT, SSC_ARRAY,  "time_hr",                       "Time at end of timestep",                                       "hr",           "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "elec_purchase_price_mult",      "Electricity purchase price multiplier",                         "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "tou_period",                    "Time of use period",                                            "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "tdry",                          "Resource dry Bulb temperature",                                 "C",            "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "twet",                          "Resource wet Bulb temperature",                                 "C",            "",                                  "",                                         "sim_type=1",                                                                "",              "" },

            // Heat pump outputs
    { SSC_OUTPUT, SSC_ARRAY,  "T_hp_HT_htf_cold_in",           "Heat pump hot tes HTF inlet temperature",                       "C",            "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "T_hp_HT_htf_hot_out",           "Heat pump hot tes HTF outlet temperature",                      "C",            "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "T_hp_CT_htf_hot_in",            "Heat pump cold tes HTF inlet temperature",                      "C",            "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "T_hp_CT_htf_cold_out",          "Heat pump cold tes HTF outlet temperature",                     "C",            "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "m_dot_hp_HT_htf",               "Heat pump hot tes HTF mass flow rate",                          "kg/s"          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "m_dot_hp_CT_htf",               "Heat pump cold tes HTF mass flow rate",                         "kg/s",         "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "q_dot_hp_startup",              "Heat pump startup power",                                       "MWt",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "q_dot_hp_to_HT_htf",            "Heat pump thermal power to hot tes HTF",                        "MWt",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "q_dot_hp_from_CT_htf",          "Heat pump thermal power from cold tes HTF",                     "MWt",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "W_dot_hp_thermo",               "Heat pump thermodynamic power in",                              "MWe",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "W_dot_hp_parasitics",           "Heat pump thermodynamic parasitics",                            "MWe",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "W_dot_hp_HT_htf_pump",          "Heat pump hot tes HTF pump power",                              "MWe",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "W_dot_hp_CT_htf_pump",          "Heat pump cold tes HTF pump power",                             "MWe",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "W_dot_hp_net",                  "Heat pump total power in",                                      "MWe",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "cop_hot_hp_thermo",             "Heat pump thermodynamic hot COP",                               "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },

            // Power cycle outputs
    { SSC_OUTPUT, SSC_ARRAY,  "T_pc_HT_htf_hot_in",            "PC hot tes HTF inlet temperature",                              "C",            "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "T_pc_HT_htf_cold_out",          "PC hot tes HTF outlet temperature",                             "C",            "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "T_pc_CT_htf_cold_in",           "PC cold tes HTF inlet temperature",                             "C",            "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "T_pc_CT_htf_hot_out",           "PC cold tes HTF outlet temperature",                            "C",            "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "m_dot_pc_HT_htf",               "PC hot tes HTF mass flow rate",                                 "kg/s"          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "m_dot_pc_CT_htf",               "PC cold tes HTF mass flow rate",                                "kg/s",         "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "q_dot_pc_startup",              "PC startup power",                                              "MWt",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "q_dot_pc_from_HT_htf",          "PC thermal power from hot tes HTF",                             "MWt",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "q_dot_pc_thermo_out",           "PC total heat leaving cycle",                                   "MWt",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "q_dot_pc_to_CT_htf",            "PC thermal power to cold tes HTF",                              "MWt",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "q_dot_pc_rejected",             "PC thermal power rejected to surroundings",                     "MWt",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "W_dot_pc_thermo_out",           "PC thermodynamic power out",                                    "MWe",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "W_dot_pc_parasitics",           "PC parasitics including cooling power",                         "MWe",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "W_dot_pc_HT_htf_pump",          "PC hot tes HTF pump power",                                     "MWe",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "W_dot_pc_CT_htf_pump",          "PC cold tes HTF pump power",                                    "MWe",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "eta_pc_thermo",                 "PC thermodynamic efficiency",                                   "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },

            // Hot TES outputs
    { SSC_OUTPUT, SSC_ARRAY,  "q_dot_dc_tes",                  "TES discharge thermal power",                                   "MWt",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "q_dot_ch_tes",                  "TES charge thermal power",                                      "MWt",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "e_ch_tes",                      "TES charge state",                                              "MWht",         "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "q_dot_tes_losses",              "TES thermal losses",                                            "MWt",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "q_dot_tes_heater",              "TES freeze protection power",                                   "MWe",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "T_tes_hot",                     "TES hot temperature",                                           "C",            "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "T_tes_cold",                    "TES cold temperature",                                          "C",            "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "mass_tes_cold",                 "TES cold tank mass (end)",                                      "kg",           "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "mass_tes_hot",                  "TES hot tank mass (end)",                                       "kg",           "",                                  "",                                         "sim_type=1",                                                                "",              "" },

            // Cold TES outputs
    { SSC_OUTPUT, SSC_ARRAY,  "q_dot_CT_tes_losses",              "TES thermal losses",                                            "MWt",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "q_dot_CT_tes_heater",              "TES freeze protection power",                                   "MWe",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "T_CT_tes_hot",                     "TES hot temperature",                                           "C",            "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "T_CT_tes_cold",                    "TES cold temperature",                                          "C",            "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "mass_CT_tes_cold",                 "TES cold tank mass (end)",                                      "kg",           "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "mass_CT_tes_hot",                  "TES hot tank mass (end)",                                       "kg",           "",                                  "",                                         "sim_type=1",                                                                "",              "" },


            // System outputs
    { SSC_OUTPUT, SSC_ARRAY,  "W_dot_fixed_parasitics",        "Parasitic power plant fixed load",                              "MWe",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "W_dot_bop_parasitics",          "Parasitic power plant generation-dependent laod",               "MWe",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "W_dot_out_net",                 "Total electric power to grid",                                  "MWe",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "gen",                           "Total electric power to grid with available derate",            "kWe",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },

            // Controller outputs
    { SSC_OUTPUT, SSC_ARRAY,  "n_op_modes",                    "Operating modes in reporting timestep",                         "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "op_mode_1",                     "1st operating mode",                                            "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "op_mode_2",                     "2nd operating mode, if applicable",                             "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "op_mode_3",                     "3rd operating mode, if applicable",                             "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "m_dot_balance",                 "Relative mass flow balance error",                              "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "q_balance",                     "Relative energy balance error",                                 "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_ARRAY,  "q_pc_target",                   "Controller target pc heat input",                               "MWt",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },

        // Dispatch outputs
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_rel_mip_gap",           "Dispatch relative MIP gap",                                     "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_solve_state",           "Dispatch solver state",                                         "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_subopt_flag",           "Dispatch suboptimal solution flag",                             "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_solve_iter",            "Dispatch iterations count",                                     "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_objective",             "Dispatch objective function value",                             "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_obj_relax",             "Dispatch objective function - relaxed max",                     "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_qsfprod_expected",      "Dispatch expected heat pump heat generation",                   "MWt",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_qsfsu_expected",        "Dispatch expected heat pump startup enegy",                     "MWt",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_tes_expected",          "Dispatch expected TES charge level",                            "MWht",         "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_pceff_expected",        "Dispatch expected power cycle efficiency adj.",                 "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_qpbsu_expected",        "Dispatch expected power cycle startup energy",                  "MWht",         "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_wpb_expected",          "Dispatch expected power generation",                            "MWe",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_rev_expected",          "Dispatch expected revenue factor",                              "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_presolve_nconstr",      "Dispatch number of constraints in problem",                     "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_presolve_nvar",         "Dispatch number of variables in problem",                       "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "disp_solve_time",            "Dispatch solver time",                                          "sec",          "",                                  "",                                         "sim_type=1",                                                                "",              "" },

    { SSC_OUTPUT,    SSC_ARRAY,  "operating_modes_a",          "First 3 operating modes tried",                                 "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "operating_modes_b",          "Next 3 operating modes tried",                                  "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT,    SSC_ARRAY,  "operating_modes_c",          "Final 3 operating modes tried",                                 "",             "",                                  "",                                         "sim_type=1",                                                                "",              "" },

            // Annual single-value outputs
    { SSC_OUTPUT, SSC_NUMBER, "annual_energy",                 "Annual total electric power to grid",                           "kWhe",         "",                                  "",                                         "sim_type=1",                                                                "",              "" },

            // Dispatch
    { SSC_OUTPUT, SSC_NUMBER, "disp_objective_ann",            "Annual sum of dispatch objective function value",                  "$",         "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "disp_iter_ann",                 "Annual sum of dispatch solver iterations",                          "",         "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "disp_presolve_nconstr_ann",     "Annual sum of dispatch problem constraint count",                   "",         "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "disp_presolve_nvar_ann",        "Annual sum of dispatch problem variable count",                     "",         "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "disp_solve_time_ann",           "Annual sum of dispatch solver time",                             "sec",         "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "disp_solve_state_ann",          "Annual sum of dispatch solve state",                                "",         "",                                  "",                                         "sim_type=1",                                                                "",              "" },
    { SSC_OUTPUT, SSC_NUMBER, "avg_suboptimal_rel_mip_gap",    "Average suboptimal relative MIP gap",                              "%",         "",                                  "",                                         "sim_type=1",                                                                "",              "" },

            // ETES settings for financial model
    { SSC_OUTPUT, SSC_NUMBER, "ppa_soln_mode",                 "PPA solution mode",                                             "0/1",          "0 = solve ppa,1 = specify ppa",     "Revenue",                                  "sim_type=1",                                                                "INTEGER,MIN = 0,MAX = 1", "" },
    { SSC_OUTPUT, SSC_NUMBER, "flip_target_percent",		   "After-tax IRR target",		                                    "%",	        "",					                 "Revenue",                                  "sim_type=1",					                                              "MIN=0,MAX=100",     	     "" },



    var_info_invalid };

class cm_etes_ptes : public compute_module
{
public:
    cm_etes_ptes()
    {
        add_var_info(_cm_vtab_etes_ptes);
        add_var_info(vtab_adjustment_factors);
        add_var_info(vtab_technology_outputs);
    }

    void exec() override
    {
        //FILE* fp = fopen("etes_cmod_to_lk.lk", "w");
        //write_cmod_to_lk_script(fp, m_vartab);

        // First, check sim type
        int sim_type = as_integer("sim_type");
        if (sim_type != 1 && sim_type != 2) {
            std::string sim_type_msg = util::format("sim_type input was %d. It must be 1 (timeseries) or 2 (design only)", sim_type);

            throw exec_error("etes_ptes", sim_type_msg);
        }

        // System design calcs
        // Need more information than typical because cycle and heat pump design are related
        // And we want all component calcs to live in component methods/helpers
        double heater_mult = as_double("heater_mult");      //[-]
        double tshours = as_double("tshours");              //[-]

        // Define generation mechanical, electrical, and thermal power
        // Need to break out thermodynamic cycle so that net output, heat input, heat output, and efficiency are consistent
        // Important because: 1) important to capture exact heat rejection for CT storage and net efficiency includes electrical parasitics that don't apply to cycle working fluid
        double W_dot_gen_thermo = as_double("W_dot_pc_thermo_des");     //  100.0;    //[MWe]
        double f_elec_consume_vs_gen = as_double("f_pc_parasitic_des"); // 0.1; //[-] Fraction of thermo generation that cycle uses for parasitics (motors, generators, cooling)
        double eta_therm_mech = as_double("eta_pc_thermo_des");         // 0.5;        //[-]
        double W_dot_gen_net, W_dot_gen_elec_parasitic, q_dot_hot_in_gen, q_dot_cold_out_gen, eta_gen_net;
        W_dot_gen_net = W_dot_gen_elec_parasitic = q_dot_hot_in_gen = q_dot_cold_out_gen = eta_gen_net = std::numeric_limits<double>::quiet_NaN();
        pc_ptes_helpers::design_calcs__no_ctes(W_dot_gen_thermo, f_elec_consume_vs_gen,
            eta_therm_mech,
            W_dot_gen_net, W_dot_gen_elec_parasitic,
            q_dot_hot_in_gen, q_dot_cold_out_gen, eta_gen_net);

        // Define heat pump power/heat flows
            // Design parameters
        double COP_heat_charge_therm = as_double("cop_hp_thermo_des");              // 1.5;     //[-]
        double f_elec_consume_vs_W_dot_thermo = as_double("f_hp_parasitic_des");    // 0.05;   //[-]
            // Calculate heat and power 
        double q_dot_hot_out_charge = q_dot_hot_in_gen*heater_mult; //[MWt]
            // Call heat pump high-level design method
        double W_dot_in_charge_thermo, q_dot_cold_in_charge, W_dot_in_charge_elec, W_dot_charge_net, COP_heat_charge_net;
        W_dot_in_charge_thermo = q_dot_cold_in_charge = W_dot_in_charge_elec = W_dot_charge_net = COP_heat_charge_net = std::numeric_limits<double>::quiet_NaN();
        heat_pump_helpers::design_calcs(q_dot_hot_out_charge, COP_heat_charge_therm, f_elec_consume_vs_W_dot_thermo,
            W_dot_in_charge_thermo, q_dot_cold_in_charge, W_dot_in_charge_elec, W_dot_charge_net, COP_heat_charge_net);

        // Ratio to heat pump q_dot_cold to q_dot_hot
        // Power cycle model uses this to fix ratios of CT and HT TES mass flow rates
        // So that CT and HT availability moves together
        // (could also use COP but thought this value was more general)
        double fixed__q_dot_cold__to__q_dot_warm = q_dot_cold_in_charge / q_dot_hot_out_charge;     //[-]
        // Now can solve cycle split between CT TES and rejection to ambient
        double q_dot_cold_out_gen_to_CTES = std::numeric_limits<double>::quiet_NaN();       //[MWt]
        double q_dot_cold_out_gen_to_surr = std::numeric_limits<double>::quiet_NaN();       //[MWt]
        pc_ptes_helpers::design_calcs__q_dot_ctes(q_dot_hot_in_gen, fixed__q_dot_cold__to__q_dot_warm, q_dot_cold_out_gen,
                                    q_dot_cold_out_gen_to_CTES, q_dot_cold_out_gen_to_surr);

        // Calculate some RTE terms
        // Probably want some "true" net RTE that includes all parasitics
        // ... e.g. these don't include pumping power or BOP & fixed system parasitics
        double RTE_therm = eta_therm_mech * COP_heat_charge_therm;      //[-]
        double RTE_cycles_net = eta_gen_net * COP_heat_charge_net;      //[-]

        // *****************************************************
        // *****************************************************
        // --- Either ----
        // Define temperatures - define TES temps using CYCLE working fluid and approach temps
            // High Temp two-tank - charging
        //double T_HT_hot_charge = 565.0;     //[C]
        //double T_HT_cold_charge = 310.0;    //[C]
        //double dT_HX_HT_charge = 5.0;       //[C] assume counterflow CR = 1
        //    // Calculate Hot Temp TES temps
        //double T_HT_hot_TES = T_HT_hot_charge - dT_HX_HT_charge;   //[C]
        //double T_HT_cold_TES = T_HT_cold_charge - dT_HX_HT_charge; //[C]
        //
        //    // Cold Temp two-tank - charging
        //double T_CT_cold_charge = -50.0;    //[C]
        //double T_CT_hot_charge = 50.0;      //[C]
        //double dT_HX_CT_charge = 5.0;       //[C] assume counterflow CR = 1
        //    // Calculate Cold Temp TES temps
        //double T_CT_cold_TES = T_CT_cold_charge + dT_HX_CT_charge;  //[C]
        //double T_CT_hot_TES = T_CT_hot_charge + dT_HX_HT_charge;    //[C]
        //
        //    // Generation temperatures
        //double dT_HX_HT_gen = 5.0;  //[C] assume counterflow CR = 1
        //double T_HT_hot_gen = T_HT_hot_TES - dT_HX_HT_gen;      //[C]
        //double T_HT_cold_gen = T_HT_cold_TES - dT_HX_HT_gen;    //[C]
        //
        //double dT_HX_CT_gen = 5.0;  //[C] assume counterflow CR = 1
        //double T_CT_hot_gen = T_CT_hot_TES + dT_HX_CT_gen;      //[C]
        //double T_CT_cold_gen = T_CT_cold_TES + dT_HX_CT_gen;    //[C]
        // *****************************************************
        // *****************************************************
        // --- OR ----
        // Define TES temps only. Assume direct storage. Heat pump and cycle model off-design uses HTF temps
        double T_HT_hot_TES = as_double("T_HT_hot_htf_des");        // 560.0;    //[C]
        double T_HT_cold_TES = as_double("T_HT_cold_htf_des");      // 305.0;   //[C]
        double T_CT_cold_TES = as_double("T_CT_cold_htf_des");      // -45.0;   //[C]
        double T_CT_hot_TES = as_double("T_CT_hot_htf_des");        // 55.0;     //[C]
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
        if (weather_reader.has_error()) throw exec_error("etes_ptes", weather_reader.get_error());
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
                throw exec_error("etes_ptes", "The time step duration must be evenly divisible within an hour.");
        }

        size_t n_steps_fixed = (size_t)steps_per_hour * 8760;   //[-]
        if (as_boolean("vacuum_arrays"))
        {
            n_steps_fixed = steps_per_hour * (size_t)((sim_setup.m_sim_time_end - sim_setup.m_sim_time_start) / 3600.);
        }
        sim_setup.m_report_step = 3600.0 / (double)steps_per_hour;  //[s]
        // *****************************************************
        // *****************************************************

        // Get HTF inputs here
        int HT_htf_code = as_integer("hot_htf_code");
        util::matrix_t<double> ud_HT_htf_props;
        if (HT_htf_code == HTFProperties::User_defined) {
            ud_HT_htf_props = as_matrix("ud_hot_htf_props");
        }
        int CT_htf_code = as_integer("cold_htf_code");
        util::matrix_t<double> ud_CT_htf_props;
        if (CT_htf_code == HTFProperties::User_defined) {
            ud_CT_htf_props = as_matrix("ud_cold_htf_props");
        }

        // *****************************************************
        // Power cycle
        double cycle_max_frac = as_double("cycle_max_frac");        //[-]
        double cycle_cutoff_frac = as_double("cycle_cutoff_frac");  //[-]
        double q_sby_frac = as_double("q_sby_frac");                //[-]
        double HT_htf_pump_coef = as_double("pb_pump_coef");        //[kW/kg/s]
        double CT_htf_pump_coef = as_double("CT_pb_pump_coef");     //[kW/kg/s]
        double startup_time = as_double("startup_time");            //[hr]
        double startup_frac = as_double("startup_frac");            //[-]

        C_pc_ptes c_pc(W_dot_gen_thermo, eta_therm_mech,
            f_elec_consume_vs_gen, fixed__q_dot_cold__to__q_dot_warm,
            T_HT_hot_TES, T_HT_cold_TES, T_CT_cold_TES, T_CT_hot_TES,
            cycle_max_frac, cycle_cutoff_frac, q_sby_frac,
            startup_time, startup_frac,
            HT_htf_pump_coef, CT_htf_pump_coef,
            HT_htf_code, ud_HT_htf_props,
            CT_htf_code, ud_CT_htf_props);

        C_csp_power_cycle::S_solved_params pc_solved_params;

            // Set power cycle outputs
        c_pc.mc_reported_outputs.assign(C_pc_ptes::E_T_HT_HTF_HOT_IN, allocate("T_pc_HT_htf_hot_in", n_steps_fixed), n_steps_fixed);
        c_pc.mc_reported_outputs.assign(C_pc_ptes::E_T_HT_HTF_COLD_OUT, allocate("T_pc_HT_htf_cold_out", n_steps_fixed), n_steps_fixed);
        c_pc.mc_reported_outputs.assign(C_pc_ptes::E_T_CT_HTF_COLD_IN, allocate("T_pc_CT_htf_cold_in", n_steps_fixed), n_steps_fixed);
        c_pc.mc_reported_outputs.assign(C_pc_ptes::E_T_CT_HTF_HOT_OUT, allocate("T_pc_CT_htf_hot_out", n_steps_fixed), n_steps_fixed);
        c_pc.mc_reported_outputs.assign(C_pc_ptes::E_M_DOT_HT_HTF, allocate("m_dot_pc_HT_htf", n_steps_fixed), n_steps_fixed);
        c_pc.mc_reported_outputs.assign(C_pc_ptes::E_M_DOT_CT_HTF, allocate("m_dot_pc_CT_htf", n_steps_fixed), n_steps_fixed);
        c_pc.mc_reported_outputs.assign(C_pc_ptes::E_Q_DOT_STARTUP, allocate("q_dot_pc_startup", n_steps_fixed), n_steps_fixed);
        c_pc.mc_reported_outputs.assign(C_pc_ptes::E_Q_DOT_HOT_IN, allocate("q_dot_pc_from_HT_htf", n_steps_fixed), n_steps_fixed);
        c_pc.mc_reported_outputs.assign(C_pc_ptes::E_Q_DOT_THERMO_OUT_TOTAL, allocate("q_dot_pc_thermo_out", n_steps_fixed), n_steps_fixed);
        c_pc.mc_reported_outputs.assign(C_pc_ptes::E_Q_DOT_TO_COLD_TES, allocate("q_dot_pc_to_CT_htf", n_steps_fixed), n_steps_fixed);
        c_pc.mc_reported_outputs.assign(C_pc_ptes::E_Q_DOT_REJECTED, allocate("q_dot_pc_rejected", n_steps_fixed), n_steps_fixed);
        c_pc.mc_reported_outputs.assign(C_pc_ptes::E_W_DOT_THERMO, allocate("W_dot_pc_thermo_out", n_steps_fixed), n_steps_fixed);
        c_pc.mc_reported_outputs.assign(C_pc_ptes::E_W_DOT_CYCLE_PARASITICS, allocate("W_dot_pc_parasitics", n_steps_fixed), n_steps_fixed);
        c_pc.mc_reported_outputs.assign(C_pc_ptes::E_W_DOT_HT_HTF_PUMP, allocate("W_dot_pc_HT_htf_pump", n_steps_fixed), n_steps_fixed);
        c_pc.mc_reported_outputs.assign(C_pc_ptes::E_W_DOT_CT_HTF_PUMP, allocate("W_dot_pc_CT_htf_pump",n_steps_fixed), n_steps_fixed);

        c_pc.mc_reported_outputs.assign(C_pc_ptes::E_ETA_THERMAL, allocate("eta_pc_thermo", n_steps_fixed), n_steps_fixed);
        // **********************************************************
        // **********************************************************

        // **********************************************************
        // Heat pump
        double f_q_dot_des_allowable_su = as_double("f_q_dot_des_allowable_su");    //[-] fraction of design power allowed during startup
        double hrs_startup_at_max_rate = as_double("hrs_startup_at_max_rate");      //[hr] duration of startup at max startup power
        double f_heater_min = as_double("f_q_dot_heater_min");                      //[-] minimum allowable heater output as fraction of design

        double heat_pump_HT_htf_pump_coef = as_double("heat_pump_HT_HTF_pump_coef");           //[kW/kg/s]
        double heat_pump_CT_htf_pump_coef = as_double("heat_pump_CT_HTF_pump_coef");           //[kW/kg/s]

        C_csp_cr_heat_pump c_heat_pump(COP_heat_charge_therm, q_dot_hot_out_charge,
            f_elec_consume_vs_W_dot_thermo,
            T_HT_hot_TES, T_HT_cold_TES, T_CT_cold_TES, T_CT_hot_TES,
            f_heater_min, f_q_dot_des_allowable_su, hrs_startup_at_max_rate,
            heat_pump_HT_htf_pump_coef, heat_pump_CT_htf_pump_coef,
            HT_htf_code, ud_HT_htf_props,
            CT_htf_code, ud_CT_htf_props);


        c_heat_pump.mc_reported_outputs.assign(C_csp_cr_heat_pump::E_T_HT_HTF_IN, allocate("T_hp_HT_htf_cold_in", n_steps_fixed), n_steps_fixed);
        c_heat_pump.mc_reported_outputs.assign(C_csp_cr_heat_pump::E_T_HT_HTF_OUT, allocate("T_hp_HT_htf_hot_out", n_steps_fixed), n_steps_fixed);
        c_heat_pump.mc_reported_outputs.assign(C_csp_cr_heat_pump::E_T_CT_HTF_IN, allocate("T_hp_CT_htf_hot_in", n_steps_fixed), n_steps_fixed);
        c_heat_pump.mc_reported_outputs.assign(C_csp_cr_heat_pump::E_T_CT_HTF_OUT, allocate("T_hp_CT_htf_cold_out", n_steps_fixed), n_steps_fixed);
        c_heat_pump.mc_reported_outputs.assign(C_csp_cr_heat_pump::E_M_DOT_HT_HTF, allocate("m_dot_hp_HT_htf", n_steps_fixed), n_steps_fixed);
        c_heat_pump.mc_reported_outputs.assign(C_csp_cr_heat_pump::E_M_DOT_CT_HTF, allocate("m_dot_hp_CT_htf", n_steps_fixed), n_steps_fixed);
        c_heat_pump.mc_reported_outputs.assign(C_csp_cr_heat_pump::E_Q_DOT_STARTUP, allocate("q_dot_hp_startup", n_steps_fixed), n_steps_fixed);
        c_heat_pump.mc_reported_outputs.assign(C_csp_cr_heat_pump::E_Q_DOT_HOT_OUT, allocate("q_dot_hp_to_HT_htf", n_steps_fixed), n_steps_fixed);
        c_heat_pump.mc_reported_outputs.assign(C_csp_cr_heat_pump::E_Q_DOT_COLD_IN, allocate("q_dot_hp_from_CT_htf", n_steps_fixed), n_steps_fixed);
        c_heat_pump.mc_reported_outputs.assign(C_csp_cr_heat_pump::E_W_DOT_IN_THERMO, allocate("W_dot_hp_thermo", n_steps_fixed), n_steps_fixed);
        c_heat_pump.mc_reported_outputs.assign(C_csp_cr_heat_pump::E_W_DOT_CYCLE_PARASITICS, allocate("W_dot_hp_parasitics", n_steps_fixed), n_steps_fixed);
        c_heat_pump.mc_reported_outputs.assign(C_csp_cr_heat_pump::E_W_DOT_HT_HTF_PUMP, allocate("W_dot_hp_HT_htf_pump", n_steps_fixed), n_steps_fixed);
        c_heat_pump.mc_reported_outputs.assign(C_csp_cr_heat_pump::E_W_DOT_CT_HTF_PUMP, allocate("W_dot_hp_CT_htf_pump", n_steps_fixed), n_steps_fixed);
        c_heat_pump.mc_reported_outputs.assign(C_csp_cr_heat_pump::E_W_DOT_HEATER, allocate("W_dot_hp_net", n_steps_fixed), n_steps_fixed);

        c_heat_pump.mc_reported_outputs.assign(C_csp_cr_heat_pump::E_COP_HOT_THERMO, allocate("cop_hot_hp_thermo", n_steps_fixed), n_steps_fixed);
        // **********************************************************
        // **********************************************************

        // **********************************************************
        // High temp TES
        C_csp_two_tank_tes c_HT_TES(
            HT_htf_code,
            ud_HT_htf_props,
            HT_htf_code,
            ud_HT_htf_props,
            q_dot_hot_in_gen,                   //[MWt]
            heater_mult,                        //[-]
            q_dot_hot_in_gen* tshours,          //[MWht]
            as_double("h_tank"),
            as_double("u_tank"),
            as_integer("tank_pairs"),
            as_double("hot_tank_Thtr"),
            as_double("hot_tank_max_heat"),
            as_double("cold_tank_Thtr"),
            as_double("cold_tank_max_heat"),
            0.0,                               // MSPT assumes direct storage, so no user input here: hardcode = 0.0
            T_HT_cold_TES,                     //[C]
            T_HT_hot_TES,                      //[C]
            T_HT_hot_TES,                      //[C]
            T_HT_cold_TES,                     //[C]
            as_double("h_tank_min"),
            as_double("tes_init_hot_htf_percent"),
            0.0,                               //[kW/kg/s] No htf pump losses in direct TES
            false,                             //[-] False: Field HTF always goes to TES. PC HTF always comes from TES. ETES should not simultaneously operate heater and cycle
            1.0,                               //[-] packed volume fraction
            1.85,                              //[m/s]
            false                              // for now, to get 'tanks_in_parallel' to work
        );

            // Set TES cmod outputs
        c_HT_TES.mc_reported_outputs.assign(C_csp_two_tank_tes::E_Q_DOT_LOSS, allocate("q_dot_tes_losses", n_steps_fixed), n_steps_fixed);
        c_HT_TES.mc_reported_outputs.assign(C_csp_two_tank_tes::E_W_DOT_HEATER, allocate("q_dot_tes_heater", n_steps_fixed), n_steps_fixed);
        c_HT_TES.mc_reported_outputs.assign(C_csp_two_tank_tes::E_TES_T_HOT, allocate("T_tes_hot", n_steps_fixed), n_steps_fixed);
        c_HT_TES.mc_reported_outputs.assign(C_csp_two_tank_tes::E_TES_T_COLD, allocate("T_tes_cold", n_steps_fixed), n_steps_fixed);
        c_HT_TES.mc_reported_outputs.assign(C_csp_two_tank_tes::E_MASS_COLD_TANK, allocate("mass_tes_cold", n_steps_fixed), n_steps_fixed);
        c_HT_TES.mc_reported_outputs.assign(C_csp_two_tank_tes::E_MASS_HOT_TANK, allocate("mass_tes_hot", n_steps_fixed), n_steps_fixed);


        // **********************************************************
        // **********************************************************

        // **********************************************************
        // Cold temp TES
        // Power/effiency relationship doesn't hold for cold tank, so just fake it with power = heat and eta = 1
            // so do we want these inputs from cmod?
            // do we need a tank cooler? Close enough to ambient (we expect?) to have minor heat loss?
            // ctes_params.m_hot_tank_Thtr = as_double("CT_hot_tank_Thtr");
            // ctes_params.m_hot_tank_max_heat = as_double("CT_hot_tank_max_heat");
            // ctes_params.m_cold_tank_Thtr = as_double("CT_cold_tank_Thtr");
            // ctes_params.m_cold_tank_max_heat = as_double("CT_cold_tank_max_heat");
        double q_dot_CT_des__discharge_basis = q_dot_cold_in_charge / heater_mult;
        // Hardcoded (for now) parameters
        double hot_tank_Thtr = -200.0;
        double hot_tank_max_heat = 0.0;
        double cold_tank_Thtr = -200.0;
        double cold_tank_max_heat = 0.0;
        // ******************************
        std::shared_ptr<C_csp_two_tank_tes> c_CT_TES(new C_csp_two_tank_tes(
            CT_htf_code,
            ud_CT_htf_props,
            CT_htf_code,
            ud_CT_htf_props,
            q_dot_CT_des__discharge_basis,                   //[MWt]
            heater_mult,                                     //[-]
            q_dot_CT_des__discharge_basis * tshours,         //[MWt-hr]
            as_double("CT_h_tank"),
            as_double("CT_u_tank"),
            as_integer("CT_tank_pairs"),
            hot_tank_Thtr,                                   //[C]
            hot_tank_max_heat,                               //[MWt]
            cold_tank_Thtr,                                  //[C]
            cold_tank_max_heat,                              //[MWt]
            0.0,                                             // MSPT assumes direct storage, so no user input here: hardcode = 0.0
            T_CT_cold_TES,                                   //[C]
            T_CT_hot_TES,                                    //[C]
            T_CT_hot_TES,                                    //[C]
            T_CT_cold_TES,                                   //[C]
            as_double("CT_h_tank_min"),
            100.0 - as_double("tes_init_hot_htf_percent"),   //[-] Cold storage is charged when cold tank is full
            0.0,                                             //[kW/kg/s] No htf pump losses in direct TES
            false,                                           //[-] False: Field HTF always goes to TES. PC HTF always comes from TES. ETES should not simultaneously operate heater and cycle
            1.0,                                             //[-] packed volume fraction
            1.85,                                            //[m/s]
            false                                            // for now, to get 'tanks_in_parallel' to work
        ));

            // Set Cold TES cmod outputs
        c_CT_TES->mc_reported_outputs.assign(C_csp_two_tank_tes::E_Q_DOT_LOSS, allocate("q_dot_CT_tes_losses", n_steps_fixed), n_steps_fixed);
        c_CT_TES->mc_reported_outputs.assign(C_csp_two_tank_tes::E_W_DOT_HEATER, allocate("q_dot_CT_tes_heater", n_steps_fixed), n_steps_fixed);
        c_CT_TES->mc_reported_outputs.assign(C_csp_two_tank_tes::E_TES_T_HOT, allocate("T_CT_tes_hot", n_steps_fixed), n_steps_fixed);
        c_CT_TES->mc_reported_outputs.assign(C_csp_two_tank_tes::E_TES_T_COLD, allocate("T_CT_tes_cold", n_steps_fixed), n_steps_fixed);
        c_CT_TES->mc_reported_outputs.assign(C_csp_two_tank_tes::E_MASS_COLD_TANK, allocate("mass_CT_tes_cold", n_steps_fixed), n_steps_fixed);
        c_CT_TES->mc_reported_outputs.assign(C_csp_two_tank_tes::E_MASS_HOT_TANK, allocate("mass_CT_tes_hot", n_steps_fixed), n_steps_fixed);


        // **********************************************************
        // **********************************************************

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
                throw exec_error("etes_electric_resistsance", "etes_financial_model must be 1, 2, 3, or 4");
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
            dispatch.params.set_user_params(as_double("disp_time_weighting"), as_double("disp_csu_cost")*W_dot_gen_thermo, as_double("disp_pen_delta_w"),
                as_double("disp_hsu_cost") * q_dot_hot_out_charge, as_double("disp_down_time_min"), as_double("disp_up_time_min")); // , ppa_price_year1);
        }
        else {
            dispatch.solver_params.dispatch_optimize = false;
        }

        // *****************************************************
        // *****************************************************

        // *****************************************************
        // Construct System Simulation
        C_csp_solver csp_solver(weather_reader,
            c_heat_pump,
            c_pc,
            c_HT_TES,
            tou,        // TODO: can we refactor tou to a dispatch struct?
            dispatch,
            system,
            NULL,
            c_CT_TES,
            ssc_cmod_update,
            (void*)(this));

            // Set system cmod outputs
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::TIME_FINAL, allocate("time_hr", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::PRICING_MULT, allocate("elec_purchase_price_mult", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::TOU_PERIOD, allocate("tou_period", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::TDRY, allocate("tdry", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::TWET, allocate("twet", n_steps_fixed), n_steps_fixed);

            // TES
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::TES_Q_DOT_DC, allocate("q_dot_dc_tes", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::TES_Q_DOT_CH, allocate("q_dot_ch_tes", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::TES_E_CH_STATE, allocate("e_ch_tes", n_steps_fixed), n_steps_fixed);

            // Plant
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::W_DOT_NET, allocate("W_dot_out_net", n_steps_fixed), n_steps_fixed);
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
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_REL_MIP_GAP, allocate("disp_rel_mip_gap", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_SOLVE_STATE, allocate("disp_solve_state", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_SUBOPT_FLAG, allocate("disp_subopt_flag", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_SOLVE_ITER, allocate("disp_solve_iter", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_SOLVE_OBJ, allocate("disp_objective", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_SOLVE_OBJ_RELAX, allocate("disp_obj_relax", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_QSFPROD_EXPECT, allocate("disp_qsfprod_expected", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_QSFSU_EXPECT, allocate("disp_qsfsu_expected", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_TES_EXPECT, allocate("disp_tes_expected", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_PCEFF_EXPECT, allocate("disp_pceff_expected", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_QPBSU_EXPECT, allocate("disp_qpbsu_expected", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_WPB_EXPECT, allocate("disp_wpb_expected", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_REV_EXPECT, allocate("disp_rev_expected", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_PRES_NCONSTR, allocate("disp_presolve_nconstr", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_PRES_NVAR, allocate("disp_presolve_nvar", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::DISPATCH_SOLVE_TIME, allocate("disp_solve_time", n_steps_fixed), n_steps_fixed);

        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::CTRL_OP_MODE_SEQ_A, allocate("operating_modes_a", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::CTRL_OP_MODE_SEQ_B, allocate("operating_modes_b", n_steps_fixed), n_steps_fixed);
        csp_solver.mc_reported_outputs.assign(C_csp_solver::C_solver_outputs::CTRL_OP_MODE_SEQ_C, allocate("operating_modes_c", n_steps_fixed), n_steps_fixed);

        // *****************************************************
        // *****************************************************


        // *****************************************************
        // Initialize
        update("Initialize ETES PTES model...", 0.0);

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
        double W_dot_net_gen_calc;      //[MWe]
        double q_dot_hot_in_gen_calc;   //[MWt]
        double q_dot_cold_out_thermo_gen_calc;  //[MWt]
        double W_dot_elec_parasitic_gen_calc;   //[MWe]
        double eta_net_gen_calc;        //[MWe]
        double q_dot_cold_to_CTES_calc; //[MWt]
        double q_dot_cold_to_surr_calc; //[MWt]

        double m_dot_HT_htf_gen_calc;   //[kg/s]
        double cp_HT_htf_gen_calc;      //[kJ/kg-K]
        double W_dot_HT_htf_pump_gen_calc;  //[MWe]
        double m_dot_CT_htf_gen_calc;   //[kg/s] to CT TES
        double cp_CT_htf_gen_calc;      //[kJ/kg-K]
        double W_dot_CT_htf_pump_gen_calc;   //[MWe]

        c_pc.get_design_parameters(W_dot_net_gen_calc, q_dot_hot_in_gen_calc,
                        q_dot_cold_out_thermo_gen_calc, W_dot_elec_parasitic_gen_calc,
                        eta_net_gen_calc, q_dot_cold_to_CTES_calc, q_dot_cold_to_surr_calc,
                        m_dot_HT_htf_gen_calc, cp_HT_htf_gen_calc, W_dot_HT_htf_pump_gen_calc,
                        m_dot_CT_htf_gen_calc, cp_CT_htf_gen_calc, W_dot_CT_htf_pump_gen_calc);

        double CT_to_HT_m_dot_ratio_pc = m_dot_CT_htf_gen_calc / m_dot_HT_htf_gen_calc;

            // Heat Pump
        double W_dot_in_thermo_charge_calc;         //[MWe] power into cycle working fluid. does not consider electric parasitics (e.g. cooling fan, motor inefficiencies, etc.)
        double q_dot_cold_in_charge_calc;           //[MWt]
        double q_dot_hot_out_charge_calc;           //[MWt]
        double W_dot_elec_parasitic_charge_calc;    //[MWe]
        double W_dot_in_net_charge_calc;    //[MWe]
        double COP_net_calc;                //[-]

        // cp should be same as cycle?
        // but mass flow rates should be a bit different
        //  ... hot should scale w/ heater_mult and cold should also include the rte effect
        double m_dot_HT_htf_charge_calc;    //[kg/s]
        double cp_HT_htf_charge_calc;       //[kJ/kg-K]
        double W_dot_HT_htf_pump_charge_calc;   //[MWe]
        double m_dot_CT_htf_charge_calc;    //[kg/s]
        double cp_CT_htf_charge_calc;       //[kJ/kg-K]
        double W_dot_CT_htf_pump_charge_calc;   //[MWe]

        double E_su_charge_calc;            //[MWt-hr]

        c_heat_pump.get_design_parameters(W_dot_in_thermo_charge_calc,
                        q_dot_cold_in_charge_calc, q_dot_hot_out_charge_calc,
                        W_dot_elec_parasitic_charge_calc, W_dot_in_net_charge_calc,
                        COP_net_calc,
                        m_dot_HT_htf_charge_calc, cp_HT_htf_charge_calc, W_dot_HT_htf_pump_charge_calc,
                        m_dot_CT_htf_charge_calc, cp_CT_htf_charge_calc, W_dot_CT_htf_pump_charge_calc,
                        E_su_charge_calc);

        double CT_to_HT_m_dot_ratio_hp = m_dot_CT_htf_charge_calc / m_dot_HT_htf_charge_calc;

        double m_dot_ratio_ratio = CT_to_HT_m_dot_ratio_hp / CT_to_HT_m_dot_ratio_pc;

        if (std::abs(m_dot_ratio_ratio - 1.0) > 1.E-6) {
            throw exec_error("etes_electric_resistance", "CT to HT htf mass flow ratios from heat pump and power cycle don't match");
        }

            // HT TES
        double V_tes_htf_avail_calc /*m3*/, V_tes_htf_total_calc /*m3*/,
            d_tank_calc /*m*/, q_dot_loss_tes_des_calc /*MWt*/, dens_store_htf_at_T_ave_calc /*kg/m3*/,
            Q_tes_des_calc /*MWt-hr*/, HT_tes_total_mass /*kg*/;

        c_HT_TES.get_design_parameters(V_tes_htf_avail_calc, V_tes_htf_total_calc,
            d_tank_calc, q_dot_loss_tes_des_calc, dens_store_htf_at_T_ave_calc, Q_tes_des_calc, HT_tes_total_mass);

            // CT TES
        double CT_V_tes_htf_avail_calc /*m3*/, CT_V_tes_htf_total_calc /*m3*/,
            CT_d_tank_calc /*m*/, CT_q_dot_loss_tes_des_calc /*MWt*/, CT_dens_store_htf_at_T_ave_calc /*kg/m3*/,
            CT_Q_tes_des_calc /*MWt-hr*/, CT_tes_total_mass /*kg*/;

        c_CT_TES->get_design_parameters(CT_V_tes_htf_avail_calc, CT_V_tes_htf_total_calc,
            CT_d_tank_calc, CT_q_dot_loss_tes_des_calc, CT_dens_store_htf_at_T_ave_calc,
            CT_Q_tes_des_calc, CT_tes_total_mass);

            // System
        double W_dot_bop_design, W_dot_fixed_parasitic_design;    //[MWe]
        csp_solver.get_design_parameters(W_dot_bop_design, W_dot_fixed_parasitic_design);

            // Calculate net system *generation* capacity including HTF pumps and system parasitics
            //     use 'gen_thermo' instead of 'gen_net' so we can see all parasitics together
        double plant_net_capacity = W_dot_gen_thermo - W_dot_gen_elec_parasitic -
                                W_dot_HT_htf_pump_gen_calc - W_dot_CT_htf_pump_gen_calc -
                                W_dot_bop_design - W_dot_fixed_parasitic_design;    //[MWe]
        double system_capacity = plant_net_capacity*1.E3;         //[kWe], convert from MWe

            // Calculate net system *charging* metrics
            // *** Currently model does NOT calculate BOP during charging
            //       should consider adding this for PTES ***
        double plant_charging_power_in = W_dot_in_thermo_charge_calc + W_dot_elec_parasitic_charge_calc +
                                W_dot_HT_htf_pump_charge_calc + W_dot_CT_htf_pump_charge_calc +
                                W_dot_fixed_parasitic_design;   //[MWe]

            // Calculate net RTE
        double RTE_net = plant_net_capacity / (plant_charging_power_in / heater_mult);      //[-]

        // *****************************************************
        // System design is complete, so calculate final design outputs like cost, capacity, etc.
        double HT_tes_spec_cost = as_double("tes_spec_cost");           //[$/kWh]
        double CT_tes_spec_cost = as_double("CT_tes_spec_cost");        //[$/kWh]
        double power_cycle_spec_cost = as_double("cycle_spec_cost");    //[$/kWe]
        double heat_pump_spec_cost = as_double("heat_pump_spec_cost");  //[$/kWt]
        double bop_spec_cost = as_double("bop_spec_cost");              //[$/kWe]
        double contingency_rate = as_double("contingency_rate");        //[%]

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

        N_mspt::calculate_etes_costs(Q_tes_des_calc, HT_tes_spec_cost, CT_Q_tes_des_calc, CT_tes_spec_cost,
            W_dot_gen_thermo, power_cycle_spec_cost,
            q_dot_hot_out_charge, heat_pump_spec_cost, bop_spec_cost, contingency_rate,
            plant_net_capacity, EPC_perc_direct_cost, EPC_per_power_cost, EPC_fixed_cost,
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
        assign("system_capacity", (ssc_number_t)system_capacity);           //[kWe] Discharge capacity
        assign("nameplate", (ssc_number_t)plant_net_capacity);              //[MWe] Discharge capacity
        assign("cp_system_nameplate", system_capacity * 1.E-3);             //[MWe]
        assign("cp_battery_nameplate", system_capacity * 1.E-3);             //[MWe]
        assign("rte_thermo", (ssc_number_t)RTE_therm);                      //[-] Round-trip efficiency of working fluid cycles
        assign("rte_net", (ssc_number_t)RTE_net);                           //[-] Round-trip efficiency considering all parasitics
        assign("charge_capacity", (ssc_number_t)plant_charging_power_in);   //[MWe] Total electricity consumption at design-point charge
        assign("tshours_heater", (ssc_number_t)(tshours / heater_mult));    //[hr]

            // Heater
        assign("W_dot_hp_in_thermo_des", W_dot_in_thermo_charge_calc);  //[MWe]
        assign("q_dot_hp_hot_out_des", q_dot_hot_out_charge_calc);      //[MWt]
        assign("q_dot_hp_cold_in_des", q_dot_cold_in_charge_calc);      //[MWt]
        assign("W_dot_hp_elec_parasitic_des", W_dot_elec_parasitic_charge_calc);    //[MWe]
        assign("W_dot_hp_in_net_des", W_dot_in_net_charge_calc);        //[MWe]
        assign("COP_net_des", COP_net_calc);                                //[-]
        assign("m_dot_hp_HT_htf_des", m_dot_HT_htf_charge_calc);        //[kg/s]
        assign("W_dot_hp_HT_htf_pump_des", W_dot_HT_htf_pump_charge_calc);     //[MWe]
        assign("m_dot_hp_CT_htf_des", m_dot_CT_htf_charge_calc);                //[kg/s]
        assign("W_dot_hp_CT_htf_pump_des", W_dot_CT_htf_pump_charge_calc);     //[MWe]
        assign("E_hp_su_des", E_su_charge_calc);                        //[MWt-hr]

            // Cycle
        assign("q_dot_pc_hot_in_des", q_dot_hot_in_gen_calc);                      //[MWt]
        assign("q_dot_pc_cold_out_thermo_des", q_dot_cold_out_thermo_gen_calc);    //[MWt]
        assign("W_dot_pc_elec_parasitic_des", W_dot_elec_parasitic_gen_calc);      //[MWe]
        assign("W_dot_pc_net_des", W_dot_net_gen_calc);                            //[MWe]
        assign("eta_pc_net_des", eta_net_gen_calc);                                //[-]
        assign("q_dot_pc_cold_to_CTES_des", q_dot_cold_to_CTES_calc);              //[MWt]
        assign("q_dot_pc_cold_to_surroundings_des", q_dot_cold_to_surr_calc);      //[MWt]
        assign("m_dot_pc_HT_htf_des", m_dot_HT_htf_gen_calc);                      //[kg/s]
        assign("W_dot_pc_HT_htf_pump_des", W_dot_HT_htf_pump_gen_calc);            //[MWe]
        assign("m_dot_pc_CT_htf_des", m_dot_CT_htf_gen_calc);                      //[kg/s]
        assign("W_dot_pc_CT_htf_pump_des", W_dot_CT_htf_pump_gen_calc);            //[MWe]

            // Hot TES
        assign("Q_tes_des", Q_tes_des_calc);                //[MWt-hr]
        assign("V_tes_htf_avail", V_tes_htf_avail_calc);    //[m3]
        assign("V_tes_htf_total", V_tes_htf_total_calc);    //[m3]
        assign("d_tank_tes", d_tank_calc);                  //[m]
        assign("q_dot_loss_tes_des", q_dot_loss_tes_des_calc);  //[MWt]

            // Cold TES
        assign("Q_CT_tes_des", CT_Q_tes_des_calc);              //[MWt-hr]
        assign("V_CT_tes_htf_avail", CT_V_tes_htf_avail_calc);  //[m3]
        assign("V_CT_tes_htf_total", CT_V_tes_htf_total_calc);  //[m3]
        assign("d_CT_tank_tes", CT_d_tank_calc);                //[m]
        assign("q_dot_loss_CT_tes_des", CT_q_dot_loss_tes_des_calc);    //[MWt]

            // Balance of Plant
        assign("W_dot_bop_design", W_dot_bop_design);           //[MWe]
        assign("W_dot_fixed", W_dot_fixed_parasitic_design);    //[MWe]

            // Costs
        assign("heater_cost_calc", heater_cost);        //[$]
        assign("tes_cost_calc", tes_cost);              //[$]
        assign("CT_tes_cost_calc", CT_tes_cost);        //[$]
        assign("bop_cost_calc", bop_cost);              //[$]
        assign("cycle_cost_calc", power_cycle_cost);    //[$]
        assign("direct_subtotal_cost_calc", direct_capital_precontingency_cost);    //[$]
        assign("contingency_cost_calc", contingency_cost);  //[$]
        assign("total_direct_cost_calc", total_direct_cost);//[$]
        assign("epc_cost_calc", epc_and_owner_cost);        //[$]
        assign("land_cost_calc", total_land_cost);          //[$]
        assign("sales_tax_cost_calc", sales_tax_cost);      //[$]
        assign("total_indirect_cost_calc", total_indirect_cost);    //[$]
        assign("installed_per_cap_cost_calc", (ssc_number_t)(total_installed_cost / system_capacity));  //[$/kWe]
        assign("total_installed_cost", (ssc_number_t)total_installed_cost);                 //[$]
        assign("construction_financing_cost", (ssc_number_t)construction_financing_cost);   //[$]

            // Financial
        assign("ppa_soln_mode", 1);         // Only allow dispatch model to use fixed ppa mode so dispatch model knows absolute prices
        assign("flip_target_percent", 0.0); //[%] fixed ppa mode shouldn't use this input, so set it to a value that, if used, will give weird results

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
        if (!haf.setup(count))
            throw exec_error("etes_electric_resistance", "failed to setup adjustment factors: " + haf.error());

        ssc_number_t* p_gen = allocate("gen", count);
        for (size_t i = 0; i < count; i++)
        {
            size_t hour = (size_t)ceil(p_time_final_hr[i]);
            p_gen[i] = (ssc_number_t)(p_W_dot_net[i] * 1.E3 * haf(hour));           //[kWe]
        }
        // *****************************************************
        // *****************************************************

        // Report simulation metrics
        ssc_number_t* p_annual_energy_dist_time = gen_heatmap(this, steps_per_hour);

        // Annual metrics
        accumulate_annual_for_year("gen", "annual_energy", sim_setup.m_report_step / 3600.0, steps_per_hour, 1, n_steps_fixed / steps_per_hour);

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

        return;
    }
};

DEFINE_MODULE_ENTRY(etes_ptes, "Heat pump charging two two-tank TES from grid, discharge with power cycle", 1)
