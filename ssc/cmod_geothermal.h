/*
BSD 3-Clause License

Copyright (c) Alliance for Energy Innovation, LLC. See also https://github.com/NatLabRockies/ssc/blob/develop/LICENSE
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

#ifndef _CMOD_GEOTHERMAL_H_
#define _CMOD_GEOTHERMAL_H_

#include "sscapi.h"
#include "core.h"

static var_info _cm_vtab_geothermal_fin_in[] = {

    // If financial model is lcoe of none, then need to reset this to 1 in cmod for downstream grid cmod
    { SSC_INOUT,      SSC_NUMBER,        "analysis_period",                    "Analyis period",                               "years",          "",                         "Financial Parameters",                      "",        "INTEGER,MIN=0,MAX=100", ""},

var_info_invalid };

static var_info _cm_vtab_geothermal[] = {
    //   VARTYPE           DATATYPE         NAME                                   LABEL                                           UNITS             META                        GROUP           REQUIRED_IF                  CONSTRAINTS        UI_HINTS

    // control input                                                                                                                                               
    //{ SSC_INPUT,        SSC_NUMBER,      "ui_calculations_only",               "If = 1, only run UI calculations",             "",               "",                         "GeoHourly",     "*",                         "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "sim_type",                           "1 (default): timeseries, 2: design only",      "",               "",                         "System Control",  "?=1",                    "",      "SIMULATION_PARAMETER"},

    { SSC_INPUT,      SSC_NUMBER,        "geo_financial_model",                "",                                             "1-8",            "",                         "Financial Model",                           "?=1",     "INTEGER,MIN=0",         ""},


    
    { SSC_INPUT,        SSC_NUMBER,      "geotherm.cost.inj_prod_well_ratio",  "Ratio of injection wells to production wells", "",               "",                         "GeoHourly",     "",                          "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "drilling_success_rate",              "Drilling success rate",                        "%",              "",                         "GeoHourly",     "",                          "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "stim_success_rate",                  "Stimulation success rate",                     "%",              "",                         "GeoHourly",     "",                          "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "failed_prod_flow_ratio",             "Failed production well flow ratio",            "",               "",                         "GeoHourly",     "",                          "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "stimulation_type",                   "Which wells are stimulated",                   "0/1/2/3",        "0=Injection,1=Production,2=Both,3=Neither", "GeoHourly", "?=3",                      "",                "" },

    //{ SSC_INOUT,        SSC_NUMBER,      "baseline_cost",          "Baseline cost",              "$/kW",     "",     "GeoHourly",             "?=0",           "",                        "" },

    // climate and resource inputs                                                                                                                                  
    { SSC_INPUT,        SSC_STRING,      "file_name",                          "local weather file path",                      "",               "",                         "GeoHourly",     "sim_type=1",                "LOCAL_FILE",      "" },
    { SSC_INPUT,        SSC_NUMBER,      "resource_potential",                 "Resource Potential",                           "MW",             "",                         "GeoHourly",     "sim_type=1",                "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "resource_type",                      "Type of Resource (O = hydrothermal; 1 = EGS)", "",               "",                         "GeoHourly",     "*",                         "INTEGER",         "" },
    { SSC_INPUT,        SSC_NUMBER,      "resource_temp",                      "Resource Temperature",                         "C",              "",                         "GeoHourly",     "*",                         "MAX=373",         "" },
    { SSC_INPUT,        SSC_NUMBER,      "resource_depth",                     "Resource Depth",                               "m",              "",                         "GeoHourly",     "*",                         "",                "" },

    // Other inputs                                                                                                                                                 
    { SSC_INPUT,        SSC_NUMBER,      "model_choice",                       "Which model to run (0,1,2)",                   "",               "",                         "GeoHourly",     "*",                         "INTEGER",         "" },
    { SSC_INPUT,        SSC_MATRIX,      "reservoir_model_inputs",             "Reservoir temperatures over time",             "",               "",                         "GeoHourly",     "reservoir_pressure_change_type=3", "",            "" },

    // geothermal plant and equipment                                                                                                                              
    { SSC_INPUT,        SSC_NUMBER,      "specified_pump_work_amount",         "Pump work specified by user",                  "MW",             "",                         "GeoHourly",     "*",                         "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "nameplate",                          "Desired plant output",                         "kW",             "",                         "GeoHourly",     "*",                         "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "analysis_type",                      "Analysis Type",                                "",               "",                         "GeoHourly",     "*",                         "INTEGER",         "" },
    { SSC_INPUT,        SSC_NUMBER,      "num_wells",                          "Number of Wells",                              "",               "",                         "GeoHourly",     "*",                         "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "conversion_type",                    "Conversion Type (0: binary; 1: flash)",        "",               "",                         "GeoHourly",     "*",                         "INTEGER",         "" },
    { SSC_INPUT,        SSC_NUMBER,      "plant_efficiency_input",             "Plant efficiency",                             "",               "",                         "GeoHourly",     "*",                         "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "conversion_subtype",                 "Conversion Subtype",                           "",               "",                         "GeoHourly",     "*",                         "INTEGER",         "" },
    { SSC_INPUT,        SSC_NUMBER,      "decline_type",                       "Temp decline Type (0: enter rate; 1 calc rate)","",              "",                         "GeoHourly",     "*",                         "INTEGER",         "" },
    { SSC_INPUT,        SSC_NUMBER,      "temp_decline_rate",                  "Temperature decline rate",                     "%/yr",           "",                         "GeoHourly",     "*",                         "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "temp_decline_max",                   "Maximum temperature decline",                  "C",              "",                         "GeoHourly",     "*",                         "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "dt_prod_well",                       "Temperature loss in production well",          "C",              "",                         "GeoHourly",     "*",                         "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "prod_well_choice",                   "Temperature loss in production well choice",   "0/1",            "",                         "GeoHourly",     "*",                         "",                "" },

    { SSC_INPUT,        SSC_NUMBER,      "wet_bulb_temp",                      "Wet Bulb Temperature",                         "C",              "",                         "GeoHourly",     "*",                         "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "use_weather_file_conditions",        "Use weather file ambient temperature",         "0/1",            "",                         "GeoHourly",     "?=0",                      "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "ambient_pressure",                   "Ambient pressure",                             "psi",            "",                         "GeoHourly",     "*",                         "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "well_flow_rate",                     "Production flow rate per well",                "kg/s",           "",                         "GeoHourly",     "*",                         "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "pump_efficiency",                    "Pump efficiency",                              "%",              "",                         "GeoHourly",     "*",                         "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "delta_pressure_equip",               "Delta pressure across surface equipment",      "psi",            "",                         "GeoHourly",     "*",                         "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "excess_pressure_pump",               "Excess pressure @ pump suction",               "psi",            "",                         "GeoHourly",     "*",                         "",                "" },
    /*
    { SSC_INPUT,        SSC_NUMBER,      "well_diameter",                      "Production well diameter",                     "in",             "",             "GeoHourly",        "*",                        "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "casing_size",                        "Production pump casing size",                  "in",             "",             "GeoHourly",        "*",                        "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "inj_casing_size",                    "Injection pump casing size",                   "in",             "",             "GeoHourly",        "*",                        "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "inj_well_diam",                      "Injection well diameter",                      "in",             "",             "GeoHourly",        "*",                        "",                "" },
    */
    { SSC_INPUT,        SSC_NUMBER,      "geotherm.cost.inj_cost_curve_welltype","Injection well type",                          "0/1",            "",                         "GeoHourly",     "*",                         "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "geotherm.cost.prod_cost_curve_welltype","Production well type",                        "0/1",            "",                         "GeoHourly",     "*",                         "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "geotherm.cost.inj_cost_curve_welldiam","Injection well diameter type",                 "0/1",            "0=LargerDiameter,1=SmallerDiameter", "GeoHourly", "*",                        "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "geotherm.cost.prod_cost_curve_welldiam","Production well diameter type",               "0/1",            "0=LargerDiameter,1=SmallerDiameter", "GeoHourly", "*",                        "",                "" },

    { SSC_INPUT,        SSC_NUMBER,      "specify_pump_work",                  "Did user specify pump work?",                  "0 or 1",         "",                         "GeoHourly",     "*",                         "INTEGER",         "" },

    // detailed geothermal inputs                                                                                                                                   
    { SSC_INPUT,        SSC_NUMBER,      "rock_thermal_conductivity",          "Rock thermal conductivity",                    "J/m-day-C",      "",                         "GeoHourly",     "*",                         "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "rock_specific_heat",                 "Rock specific heat",                           "J/kg-C",         "",                         "GeoHourly",     "*",                         "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "rock_density",                       "Rock density",                                 "kg/m^3",         "",                         "GeoHourly",     "*",                         "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "reservoir_pressure_change_type",     "Reservoir pressure change type",               "",               "",                         "GeoHourly",     "*",                         "INTEGER",         "" },
    { SSC_INPUT,        SSC_NUMBER,      "reservoir_pressure_change",          "Pressure change",                              "psi-h/1000lb",   "",                         "GeoHourly",     "*",                         "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "injectivity_index",                  "Injectivity index",                            "lb/hr-psi",      "",                         "GeoHourly",     "*",                         "",                "" },

    { SSC_INPUT,      SSC_NUMBER,     "geotherm.cost.conf_num_wells",          "Number of confirmation wells",                              "",        "",                      "GeoHourly",     "*",                         "",                              "?=2" },
    { SSC_INPUT,      SSC_NUMBER,     "geotherm.cost.confirm_wells_percent",   "% of Confirmation Wells Used for Production",               "",        "",                      "GeoHourly",     "*",                         "",                              "?=2" },

    { SSC_INPUT,        SSC_NUMBER,      "reservoir_width",                    "Reservoir width",                              "m",              "",                         "GeoHourly",     "*",                         "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "reservoir_height",                   "Reservoir height",                             "m",              "",                         "GeoHourly",     "*",                         "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "reservoir_permeability",             "Reservoir Permeability",                       "darcys",         "",                         "GeoHourly",     "*",                         "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "inj_prod_well_distance",             "Distance from injection to production wells",  "m",              "",                         "GeoHourly",     "*",                         "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "subsurface_water_loss",              "Subsurface water loss",                        "%",              "",                         "GeoHourly",     "*",                         "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "fracture_aperature",                 "Fracture aperature",                           "m",              "",                         "GeoHourly",     "*",                         "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "fracture_length",                    "Fracture length",                              "m",              "",                         "GeoHourly",     "*",                         "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "fracture_spacing",                   "Fracture spacing",                             "m",              "",                         "GeoHourly",     "*",                         "",                "" },

    { SSC_INPUT,        SSC_NUMBER,      "fracture_width",                     "Fracture width",                               "m",              "",                         "GeoHourly",     "*",                         "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "num_fractures",                      "Number of fractures",                          "",               "",                         "GeoHourly",     "*",                         "INTEGER",         "" },
    { SSC_INPUT,        SSC_NUMBER,      "fracture_angle",                     "Fracture angle",                               "deg",            "",                         "GeoHourly",     "*",                         "",                "" },

    // power block inputs (these could change on an hourly basis, but don't here)

    //   VARTYPE           DATATYPE         NAME                                   LABEL                                              UNITS      META            GROUP               REQUIRED_IF                  CONSTRAINTS      UI_HINTS

    // power block parameters needed but not on power block SAM input page                                                                                         
  //{ SSC_INPUT,        SSC_NUMBER,      "tech_type",                          "Technology type ID",                                  "(1-4)",   "",             "GeoHourly",        "*",                         "INTEGER",         "" },
    { SSC_INPUT,        SSC_NUMBER,      "T_htf_cold_ref",                     "Outlet design temp",                                  "C",       "",             "GeoHourly",        "sim_type=1",                "",                "" },
    //{ SSC_INPUT,        SSC_NUMBER,      "T_htf_hot_ref",                      "Inlet design temp",                                   "C",       "",             "GeoHourly",        "sim_type=1",                "",                "" },
    //{ SSC_INPUT,        SSC_NUMBER,      "HTF",                                "Heat trans fluid type ID",                            "(1-27)",  "",             "GeoHourly",        "sim_type=1",                "INTEGER",         "" },

    // power block input parameters                                                                                                                                   
  //{ SSC_INPUT,        SSC_NUMBER,      "P_ref",                              "Design Output",                                       "MW",      "",             "GeoHourly",        "*",                         "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "P_boil",                             "Design Boiler Pressure",                              "bar",     "",             "GeoHourly",        "sim_type=1",                "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "eta_ref",                            "Desgin conversion efficiency",                        "%",       "",             "GeoHourly",        "sim_type=1",                "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "q_sby_frac",                         "% thermal power for standby mode",                    "%",       "",             "GeoHourly",        "sim_type=1",                "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "startup_frac",                       "% thermal power for startup",                         "%",       "",             "GeoHourly",        "sim_type=1",                "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "startup_time",                       "Hours to start power block",                          "hours",   "",             "GeoHourly",        "sim_type=1",                "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "pb_bd_frac",                         "Blowdown steam fraction",                             "%",       "",             "GeoHourly",        "sim_type=1",                "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "T_amb_des",                          "Design ambient temperature",                          "C",       "",             "GeoHourly",        "sim_type=1",                "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "CT",                                 "Condenser type (Wet, Dry,Hybrid)",                    "(1-3)",   "",             "GeoHourly",        "sim_type=1",                "INTEGER",         "" },
    { SSC_INPUT,        SSC_NUMBER,      "dT_cw_ref",                          "Design condenser cooling water inlet/outlet T diff",  "C",       "",             "GeoHourly",        "sim_type=1",                "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "T_approach",                         "Approach Temperature",                                "C",       "",             "GeoHourly",        "sim_type=1",                "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "T_ITD_des",                          "Design ITD for dry system",                           "C",       "",             "GeoHourly",        "sim_type=1",                "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "P_cond_ratio",                       "Condenser pressure ratio",                            "",        "",             "GeoHourly",        "sim_type=1",                "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "P_cond_min",                         "Minimum condenser pressure",                          "in Hg",   "",             "GeoHourly",        "sim_type=1",                "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "hr_pl_nlev",                         "# part-load increments",                              "(0-9)",   "",             "GeoHourly",        "sim_type=1",                "INTEGER",         "" },
    { SSC_INPUT,        SSC_NUMBER,      "hc_ctl1",                            "HC Control 1",                                        "",        "",             "GeoHourly",        "sim_type=1",                "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "hc_ctl2",                            "HC Control 2",                                        "",        "",             "GeoHourly",        "sim_type=1",                "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "hc_ctl3",                            "HC Control 3",                                        "",        "",             "GeoHourly",        "sim_type=1",                "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "hc_ctl4",                            "HC Control 4",                                        "",        "",             "GeoHourly",        "sim_type=1",                "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "hc_ctl5",                            "HC Control 5",                                        "",        "",             "GeoHourly",        "sim_type=1",                "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "hc_ctl6",                            "HC Control 6",                                        "",        "",             "GeoHourly",        "sim_type=1",                "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "hc_ctl7",                            "HC Control 7",                                        "",        "",             "GeoHourly",        "sim_type=1",                "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "hc_ctl8",                            "HC Control 8",                                        "",        "",             "GeoHourly",        "sim_type=1",                "",                "" },
    { SSC_INPUT,        SSC_NUMBER,      "hc_ctl9",                            "HC Control 9",                                        "",        "",             "GeoHourly",        "sim_type=1",                "",                "" },
    // dispatch
    { SSC_INPUT,        SSC_STRING,      "hybrid_dispatch_schedule",           "Daily dispatch schedule",                           "",        "",             "GeoHourly",        "sim_type=1",                  "TOUSCHED",        "" },

    { SSC_INPUT,        SSC_NUMBER,      "allow_reservoir_replacements",       "Allow reservoir replacements",                      "",        "",             "GeoHourly",        "?=0",                      "",                "" },

    { SSC_INPUT,        SSC_NUMBER,      "start_day_of_year",                  "Start day of year for TOD periods",                 "0..6",    "0=Monday, 6=Sunday",      "GeoHourly",     "?=0",                      "",                "" },


    // OUTPUTS
    // VARTYPE           DATATYPE         NAME                                   LABEL                                               UNITS      META            GROUP             REQUIRED_IF                    CONSTRAINTS      UI_HINTS

    // Design outputs should always be defined
    // This first batch of outputs is for calculating UI values                                                                                                     
    { SSC_OUTPUT,       SSC_NUMBER,      "num_wells_getem",                    "Number of Wells GETEM calc'd",                      "",        "",              "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "num_wells_getem_output",             "Number of production wells required",               "",        "",              "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "num_wells_getem_inj",                "Number of required injection wells calculated by GETEM", "",    "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "num_wells_getem_prod_drilled",       "Number of production wells drilled",                 "",        "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "num_wells_getem_prod_failed",        "Number of production wells failed during drilling",  "",        "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "num_wells_getem_inj_drilled",        "Number of injection wells drilled",                  "",        "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "num_confirm_wells_to_production",    "Number of confirmation wells used for production",   "$",       "",             "GeoHourly",        "*",      "",                "" },

    { SSC_OUTPUT,       SSC_NUMBER,      "geothermal_analysis_period",         "Analysis Lifetime",                                  "years",   "",             "GeoHourly",        "*",      "INTEGER",         "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "system_use_lifetime_output",         "Geothermal lifetime simulation",               "0/1",            "0=SingleYearRepeated,1=RunEveryYear", "GeoHourly",     "?=1",                      "BOOLEAN",         "" },


    { SSC_OUTPUT,       SSC_NUMBER,      "design_temp",                        "Power block design temperature",                     "C",       "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "plant_brine_eff",                    "Plant Brine Efficiency",                             "",        "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "pump_watthr_per_lb",                 "Pump work Efficiency",                               "",        "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "pumpwork_prod",                      "Production Pump work per mass",                      "W-hr/lb", "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "pumpwork_inj",                       "Injection Pump work per mass",                       "W-hr/lb", "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "inj_pump_hp",                        "Injection Pump horsepower",                          "hp",      "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "pump_size_hp",                       "Production Pump horsepower",                         "hp",      "",             "GeoHourly",        "*",      "",                "" },

    { SSC_OUTPUT,       SSC_NUMBER,      "gross_output",                       "Gross output from GETEM",                            "MWe",     "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "gross_cost_output",                  "Gross output from GETEM for cost",                   "kWe",     "",             "GeoHourly",        "*",      "",                "" },

    { SSC_OUTPUT,       SSC_NUMBER,      "system_capacity",                    "System capacity",                                    "kWe",     "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "cp_system_nameplate",                "System capacity for capacity payments",              "MWe",     "",             "System Costs",     "*",      "",              "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "cp_battery_nameplate",               "Battery nameplate",                                  "MWe",     "",             "System Costs",     "*",      "",              "" },

    { SSC_OUTPUT,       SSC_NUMBER,      "pump_depth_ft",                      "Pump depth calculated by GETEM",                     "ft",      "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "pump_hp",                            "Pump hp calculated by GETEM",                        "hp",      "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "reservoir_pressure",                 "Reservoir pres calculated by GETEM",                 "",        "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "reservoir_avg_temp",                 "Avg reservoir temp calculated by GETEM",             "C",       "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "bottom_hole_pressure",               "Bottom hole pres calculated by GETEM",               "",        "",             "GeoHourly",        "*",      "",                "" },

    // Some outputs Used in cmod_geothermal_costs
    { SSC_OUTPUT,       SSC_NUMBER,      "eff_secondlaw",                      "Second Law Efficiency",                              "C",       "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "qRejectTotal",                       "Total Heat Rejection",                               "btu/h",   "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "qCondenser",                         "Condenser Heat Rejected",                            "btu/h",   "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "hp_flash_pressure",                  "HP Flash Pressure",                                  "psia",    "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "lp_flash_pressure",                  "LP Flash Pressure",                                  "psia",    "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "v_stage_1",                          "Vacumm Pump Stage 1",                                "kW",      "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "v_stage_2",                          "Vacumm Pump Stage 2",                                "kW",      "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "v_stage_3",                          "Vacumm Pump Stage 3",                                "kW",      "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "GF_flowrate",                        "GF Flow Rate",                                       "lb/h",    "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "qRejectByStage_1",                   "Heat Rejected by NCG Condenser Stage 1",             "BTU/h",   "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "qRejectByStage_2",                   "Heat Rejected by NCG Condenser Stage 2",             "BTU/h",   "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "qRejectByStage_3",                   "Heat Rejected by NCG Condenser Stage 3",             "BTU/h",   "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "ncg_condensate_pump",                "Condensate Pump Work",                               "kW",      "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "cw_pump_work",                       "CW Pump Work",                                       "kW",      "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "pressure_ratio_1",                   "Suction Steam Ratio 1",                              "",        "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "pressure_ratio_2",                   "Suction Steam Ratio 2",                              "",        "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "pressure_ratio_3",                   "Suction Steam Ratio 3",                              "",        "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "condensate_pump_power",              "hp",                                                 "",        "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "cwflow",                             "Cooling Water Flow",                                 "lb/h",    "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "cw_pump_head",                       "Cooling Water Pump Head",                            "lb/h",    "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "spec_vol",                           "HP Specific Volume",                                 "cft/lb",  "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "spec_vol_lp",                        "LP Specific Volume",                                 "cft/lb",  "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "x_hp",                               "HP Mass Fraction",                                   "%",       "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "x_lp",                               "LP Mass Fraction",                                   "%",       "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "flash_count",                        "Flash Count",                                        "(1 -2)",  "",             "GeoHourly",        "*",      "",                "" },

    // pump work is an output for both the UI call and the model run                                                                                                
    { SSC_OUTPUT,       SSC_NUMBER,      "pump_work",                          "Pump work calculated by GETEM",                      "MWe",      "",             "GeoHourly",        "*",      "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "net_plant_output",                   "Net plant power to grid; net sales in GETEM",        "MWe",      "",             "GeoHourly",        "*",      "",                "" },

    // Hardcoded outputs
    { SSC_OUTPUT,     SSC_ARRAY,      "degradation",                        "Annual energy degradation",                                 "",        "",     "System Output",         "", "",                      "" },
    { SSC_OUTPUT,     SSC_NUMBER,     "system_use_recapitalization",        "Apply recapitalization in financial models?",               "0/1",     "",     "GeoHourly",             "?",        "",                              "" },


    // The array outputs are only meaningful when the model is run (not UI calculations)                                                                             
    // User can specify whether the analysis should be done hourly or monthly.  With monthly analysis, there are only monthly results.                                
    // With hourly analysis, there are still monthly results, but there are hourly (over the whole lifetime of the project) results as well.                           
//  { SSC_OUTPUT, SSC_ARRAY,  "annual_replacements", "Resource replacement? (1=yes)", "kWhac", "", "GeoHourly", "ui_calculations_only=0", "", "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "gen",                                "System power generated",                             "kW",      "",             "GeoHourly",        "",                         "",                "" },
    { SSC_OUTPUT,       SSC_MATRIX,      "annual_energy_distribution_time",    "Annual energy production as function of Time",       "",        "",             "Heatmaps",         "",                         "",                "" },

    { SSC_OUTPUT,       SSC_ARRAY,       "system_lifetime_recapitalize",       "Resource replacement? (1=yes)",                      "",        "",             "GeoHourly",        "sim_type=1",               "",                "" },

    { SSC_OUTPUT,       SSC_ARRAY,       "monthly_resource_temperature",       "Monthly avg resource temperature",                   "C",       "",             "GeoHourly",        "sim_type=1",               "",                "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "monthly_power",                      "Monthly power",                                      "kW",      "",             "GeoHourly",        "sim_type=1",               "",                "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "monthly_energy",                     "Monthly AC energy in Year 1",                        "kWh/mo",  "",             "GeoHourly",        "",                         "LENGTH=12",       "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "monthly_energy_lifetime",            "Monthly energy before performance adjustments",      "kWh",     "",             "GeoHourly",        "sim_type=1",               "",                "" },

    { SSC_OUTPUT,       SSC_ARRAY,       "timestep_resource_temperature",      "Resource temperature",                               "C",       "",             "GeoHourly",        "sim_type=1",               "",                "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "timestep_test_values",               "Test output values in each time step",               "",        "",             "GeoHourly",        "sim_type=1",               "",                "" },

    { SSC_OUTPUT,       SSC_ARRAY,       "timestep_pressure",                  "Atmospheric pressure",                               "atm",     "",             "GeoHourly",        "sim_type=1",               "",                "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "timestep_dry_bulb",                  "Dry bulb temperature",                               "C",       "",             "GeoHourly",        "sim_type=1",               "",                "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "timestep_wet_bulb",                  "Wet bulb temperature",                               "C",       "",             "GeoHourly",        "sim_type=1",               "",                "" },

    { SSC_OUTPUT,       SSC_NUMBER,      "lifetime_output",                    "Lifetime output",                                    "kWh",     "",             "GeoHourly",        "sim_type=1",               "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "first_year_output",                  "First year output",                                  "kWh",     "",             "GeoHourly",        "sim_type=1",               "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "annual_energy",                      "Annual AC energy in Year 1",                         "kWh",     "",             "GeoHourly",        "sim_type=1",               "",                "" },

    { SSC_OUTPUT,       SSC_NUMBER,      "capacity_factor",                    "Capacity factor",                                    "",        "",             "",                 "*",                         "",                "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "kwh_per_kw",                          "First year kWh/kW",                                 "",        "",             "",                 "*",                         "",                "" },

var_info_invalid };



#endif
