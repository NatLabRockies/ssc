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

#ifndef _CMOD_GEOTHERMAL_COSTS_H_
#define _CMOD_GEOTHERMAL_COSTS_H_

#include "sscapi.h"


static var_info _cm_vtab_geothermal_costs_unique[] = {
    /*   VARTYPE        DATATYPE       NAME                                  LABEL                                                       UNITS     META    GROUP        REQUIRED_IF                 CONSTRAINTS                     UI_HINTS */

    { SSC_INPUT,      SSC_NUMBER,     "conversion_type",                    "Conversion Type",                                           "",        "",     "GeoHourly", "*",                        "INTEGER",                       ""   },
    { SSC_INPUT,      SSC_NUMBER,     "ppi_base_year",                      "PPI Base Year",                                             "",        "",     "GeoHourly", "?=19",                     "",                              ""   },

    // Binary Plant Type Inputs:
    //{ SSC_INPUT,      SSC_NUMBER,     "gross_output",                       "Gross output from GETEM",                                   "MW",      "",     "GeoHourly", "*",                        "",                              ""   },
    //{ SSC_INPUT,      SSC_NUMBER,     "gross_cost_output",                  "Gross output from GETEM for cost calculations",             "kW",      "",     "GeoHourly", "*",                        "",                              ""   },

    { SSC_INPUT,      SSC_NUMBER,     "dt_prod_well",                       "Temperature loss in production well",                       "C",       "",     "GeoHourly", "*",                        "",                              ""   },
    //{ SSC_INPUT,      SSC_NUMBER,     "eff_secondlaw",                      "Second Law Efficiency",                                      "%",       "",     "GeoHourly", "*",                        "",                              ""   },

    { SSC_INPUT,      SSC_NUMBER,     "calc_drill_costs",                   "Calculate drill costs",                                      "0/1",     "0=LargerDiameter,1=SmallerDiameter", "GeoHourly", "?=1",         "",                              ""   },
    { SSC_INPUT,      SSC_NUMBER,     "geotherm.cost.plant_auto_estimate",  "0: use user input cost; 1: use getem calcs",                 "0/1",     "",                                   "GeoHourly", "",         "",                              ""   },
    { SSC_INPUT,      SSC_NUMBER,     "geotherm.cost.plant_per_kW_input",   "user input for relative plant cost",                         "$/kWe",   "",                                   "GeoHourly", "geotherm.cost.plant_auto_estimate=0",         "",                              ""   },

    { SSC_INPUT,      SSC_NUMBER,     "geotherm.cost.inj_cost_curve_welltype","Injection well type",                                       "0/1",     "",     "GeoHourly", "calc_drill_costs=1",      "",                              ""   },
    { SSC_INPUT,      SSC_NUMBER,     "geotherm.cost.prod_cost_curve_welltype","Production well type",                                     "0/1",     "",     "GeoHourly", "calc_drill_costs=1",      "",                              ""   },
    { SSC_INPUT,      SSC_NUMBER,     "geotherm.cost.inj_cost_curve_welldiam","Injection well diameter type",                             "0/1",     "0=LargerDiameter,1=SmallerDiameter", "GeoHourly", "calc_drill_costs=1", "",                              ""   },
    { SSC_INPUT,      SSC_NUMBER,     "geotherm.cost.prod_cost_curve_welldiam","Production well diameter type",                           "0/1",     "0=LargerDiameter,1=SmallerDiameter", "GeoHourly", "calc_drill_costs=1", "",                              ""   },
    { SSC_INPUT,      SSC_NUMBER,     "geotherm.cost.inj_cost_curve",       "Injection well diameter type",                              "0/1",     "0=LargerDiameter,1=SmallerDiameter", "GeoHourly", "calc_drill_costs=1", "",                              ""   },
    { SSC_INPUT,      SSC_NUMBER,     "geotherm.cost.prod_cost_curve",      "Production well diameter type",                             "0/1",     "0=LargerDiameter,1=SmallerDiameter", "GeoHourly", "calc_drill_costs=1", "",                              ""   },

    { SSC_INPUT,      SSC_NUMBER,     "geotherm.cost.prod_inj_non_drill",   "Non drilling cost for prod and inj well",                   "$",      "0=LargerDiameter,1=SmallerDiameter", "GeoHourly", "calc_drill_costs=1", "",                              ""   },

    { SSC_INPUT,      SSC_NUMBER,     "resource_depth",                      "Resource Depth",                                           "m",       "",     "GeoHourly", "calc_drill_costs=1",      "",                              ""   },
    
    { SSC_INPUT,      SSC_NUMBER,     "geotherm.cost.stim_non_drill",       "Stimulation non drilling costs",                            "$",       "",     "GeoHourly", "calc_drill_costs=1",      "",                              "?=0" },
    { SSC_INPUT,      SSC_NUMBER,     "geotherm.cost.expl_non_drill",       "Exploration non drilling costs",                            "$",       "",     "GeoHourly", "calc_drill_costs=1",      "",                              "?=750000" },
    { SSC_INPUT,      SSC_NUMBER,     "geotherm.cost.conf_non_drill",       "Confirmation non drilling costs",                           "$",       "",     "GeoHourly", "calc_drill_costs=1",      "",                              "?=250000" },
    { SSC_INPUT,      SSC_NUMBER,     "geotherm.cost.expl_multiplier",      "Exploration cost multiplier",                               "",        "",     "GeoHourly", "calc_drill_costs=1",      "",                              "?=0.5" },
    { SSC_INPUT,      SSC_NUMBER,     "geotherm.cost.conf_multiplier",      "Confirmation cost multiplier",                              "",        "",     "GeoHourly", "calc_drill_costs=1",      "",                              "?=1.2" },
    { SSC_INPUT,      SSC_NUMBER,     "geotherm.cost.expl_num_wells",       "Number of exploration wells",                               "",        "",     "GeoHourly", "calc_drill_costs=1",      "",                              "?=2" },
    { SSC_INPUT,      SSC_NUMBER,     "geotherm.cost.conf_num_wells",       "Number of confirmation wells",                              "",        "",     "GeoHourly", "calc_drill_costs=1",      "",                              "?=2" },
    // need defaults?
    { SSC_INPUT,      SSC_NUMBER,     "geotherm.cost.pump_fixed",           "Fixed pump workover and casing cost",                       "$",       "",     "GeoHourly", "",                        "",                              ""   },
    { SSC_INPUT,      SSC_NUMBER,     "geotherm.cost.pump_per_foot",        "Pump cost per foot",                                        "$/ft",    "",     "GeoHourly", "",                        "",                              ""   },
    { SSC_INPUT,      SSC_NUMBER,     "geotherm.cost.pump_casing_cost",     "Pump casing cost per foot",                                 "$/ft",    "",     "GeoHourly", "",                        "",                              ""   },

    // name change to match assign statement
 // { SSC_INPUT,      SSC_NUMBER,     "geotherm.cost.pump_geotherm.cost.pump_depth","Pump depth",                 "ft",      "",     "GeoHourly", "",                        "",                              ""   },
    { SSC_INPUT,      SSC_NUMBER,     "stimulation_type",                   "Which wells are stimulated",                                "0/1/2/3", "",     "GeoHourly", "",                        "",                              "?=0" },

    // Outputs
    { SSC_OUTPUT,     SSC_NUMBER,     "baseline_cost",                      "Baseline cost",                                             "$/kW",    "",     "GeoHourly", "?",                        "",                              ""   },
    { SSC_OUTPUT,     SSC_NUMBER,     "total_drilling_cost",                "Total drilling cost",                                       "$",       "",     "GeoHourly", "calc_drill_costs=1",       "",                              ""   },
    { SSC_OUTPUT,     SSC_NUMBER,     "total_pump_cost",                    "Total pumping cost",                                        "$",       "",     "GeoHourly", "?",                        "",                              ""   },
    { SSC_OUTPUT,     SSC_NUMBER,     "total_gathering_cost",               "Total gathering well cost",                                 "$",       "",     "GeoHourly", "?",                        "",                              ""   },
    { SSC_OUTPUT,     SSC_NUMBER,     "indirect_pump_gathering_cost",       "Indirect pump and field gathering cost",                    "$",       "",     "GeoHourly", "?",                        "",                              ""   },
    { SSC_OUTPUT,     SSC_NUMBER,     "total_pump_gathering_cost",          "Total pump and field gathering system cost",                "$",       "",     "GeoHourly", "?",                        "",                              ""   },
    { SSC_OUTPUT,     SSC_NUMBER,     "pump_only_cost",                     "Production pump cost per well",                             "$/well",  "",     "GeoHourly", "?",                        "",                              ""   },
    { SSC_OUTPUT,     SSC_NUMBER,     "pump_cost_install",                  "Production pump installation cost",                         "$/well",  "",     "GeoHourly", "?",                        "",                              ""   },
    { SSC_OUTPUT,     SSC_NUMBER,     "total_surface_equipment_cost",       "Total surface equipment cost",                              "$",       "",     "GeoHourly", "?",                        "",                              ""   },
    { SSC_OUTPUT,     SSC_NUMBER,     "prod_pump_cost_per_well",            "Production pump cost per well",                             "$/well",  "",     "GeoHourly", "?",                        "",                              ""   },
    { SSC_OUTPUT,     SSC_NUMBER,     "inj_pump_cost_per_pump",             "Injection pump cost per pump",                              "$/pump",  "",     "GeoHourly", "?",                        "",                              ""   },
    { SSC_OUTPUT,     SSC_NUMBER,     "inj_num_pumps",                      "Number of injection pumps",                                 "",        "",     "GeoHourly", "?",                        "",                              ""   },
    { SSC_OUTPUT,     SSC_NUMBER,     "indirect_pump_cost",                 "Number of injection pumps",                                 "",        "",     "GeoHourly", "?",                        "",                              ""   },
    { SSC_OUTPUT,     SSC_NUMBER,     "prod_pump_cost",                     "Production pump system cost",                               "$",       "",     "GeoHourly", "?",                        "",                              ""   },
    { SSC_OUTPUT,     SSC_NUMBER,     "inj_pump_cost",                      "Injection pump system cost",                                "$",       "",     "GeoHourly", "?",                        "",                              ""   },
    { SSC_OUTPUT,     SSC_NUMBER,     "piping_cost_per_well",               "Surface piping cost per well",                              "$/well",  "",     "GeoHourly", "?",                        "",                              ""   },
    { SSC_OUTPUT,     SSC_NUMBER,     "field_gathering_num_wells",          "Field gathering system number of wells",                    "wells",   "",     "GeoHourly", "?",                        "",                              ""   },

    // Plant costs
    { SSC_OUTPUT,     SSC_NUMBER,     "total_plant_cost",                   "Power plant cost",                                          "$",       "",     "GeoHourly", "?",                        "",                              ""   },

    // Stimulation costs
    { SSC_OUTPUT,     SSC_NUMBER,     "stim_cost_per_well",                 "Stimulation cost per well",                                 "$/well",  "",     "GeoHourly", "?",                        "",                              ""   },
    { SSC_OUTPUT,     SSC_NUMBER,     "stim_cost_non_drill",                "Non-drilling stimulation costs",                            "$",       "",     "GeoHourly", "?",                        "",                              ""   },
    { SSC_OUTPUT,     SSC_NUMBER,     "stim_total_cost",                    "Stimulation Total costs",                                   "$",       "",     "GeoHourly", "?",                        "",                              ""   },   

    // Expl and Confirmation drilling costs
    { SSC_OUTPUT,     SSC_NUMBER,     "expl_total_cost",                    "Exploration total costs",                                   "$",       "",     "GeoHourly", "?",                        "",                              ""   },
    { SSC_OUTPUT,     SSC_NUMBER,     "expl_per_well_cost",                 "Exploration cost per production well",                      "$/well",  "",     "GeoHourly", "?",                        "",                              ""   },
    { SSC_OUTPUT,     SSC_NUMBER,     "expl_drilling_cost",                 "Exploration drilling costs",                                "$",       "",     "GeoHourly", "?",                        "",                              ""   },
    { SSC_OUTPUT,     SSC_NUMBER,     "conf_total_cost",                    "Confirmation total costs",                                  "$",       "",     "GeoHourly", "?",                        "",                              ""   },
    { SSC_OUTPUT,     SSC_NUMBER,     "conf_drilling_cost",                 "Confirmation drilling costs",                               "$",       "",     "GeoHourly", "?",                        "",                              ""   },

    // Drilling costs
    { SSC_OUTPUT,     SSC_NUMBER,     "prod_well_cost",                     "Production cost per well",                                  "$/well",  "",     "GeoHourly", "?",                        "",                              ""   },
    { SSC_OUTPUT,     SSC_NUMBER,     "prod_total_cost",                    "Total production well system cost",                         "$",       "",     "GeoHourly", "?",                        "",                              ""   },
    { SSC_OUTPUT,     SSC_NUMBER,     "inj_well_cost",                      "Injection cost per well",                                   "$/well",  "",     "GeoHourly", "?",                        "",                              ""   },
    { SSC_OUTPUT,     SSC_NUMBER,     "inj_total_cost",                     "Total injection well system cost",                          "$",       "",     "GeoHourly", "?",                        "",                              ""   },

    { SSC_OUTPUT,     SSC_NUMBER,     "sum_prod_inj_total_cost",            "Sum of total production and injection well system cost",    "$",       "",     "GeoHourly", "?",                        "",                              ""   },
    { SSC_OUTPUT,     SSC_NUMBER,     "prod_inj_total_cost",                "Sum drilling cost plus non-drilling well costs",            "$",       "",     "GeoHourly", "?",                        "",                              ""   },


    var_info_invalid
};

static var_info _cm_vtab_geothermal_costs_upstream[] = {
    /*   VARTYPE        DATATYPE       NAME                                  LABEL                                                       UNITS     META    GROUP        REQUIRED_IF                 CONSTRAINTS                     UI_HINTS */

    { SSC_INPUT,      SSC_NUMBER,     "design_temp",                        "Power block design temperature",                            "C",       "",     "GeoHourly", "*",                        "",                              ""   },

    // Binary Plant Type Inputs:
    { SSC_INPUT,      SSC_NUMBER,     "gross_output",                       "Gross output from GETEM",                                   "MWe",      "",     "GeoHourly", "*",                        "",                              ""   },
    { SSC_INPUT,      SSC_NUMBER,     "gross_cost_output",                  "Gross output from GETEM for cost calculations",             "kWe",      "",     "GeoHourly", "*",                        "",                              ""   },
    { SSC_INPUT,      SSC_NUMBER,     "eff_secondlaw",                      "Second Law Efficiency",                                      "%",       "",     "GeoHourly", "*",                        "",                              ""   },

    // Flash Plant Type Inputs:
    { SSC_INPUT,      SSC_NUMBER,     "qRejectTotal",                       "Total Rejected Heat",                                       "btu/h",   "",     "GeoHourly", "conversion_type=1",       "",                              ""   },
    { SSC_INPUT,      SSC_NUMBER,     "qCondenser",                         "Condenser Heat Rejected",                                   "btu/h",   "",     "GeoHourly", "conversion_type=1",       "",                              ""   },
    { SSC_INPUT,      SSC_NUMBER,     "v_stage_1",                          "Vacumm Pump Stage 1",                                       "kW",      "",     "GeoHourly", "conversion_type=1",       "",                              ""   },
    { SSC_INPUT,      SSC_NUMBER,     "v_stage_2",                          "Vacumm Pump Stage 2",                                       "kW",      "",     "GeoHourly", "conversion_type=1",       "",                              ""   },
    { SSC_INPUT,      SSC_NUMBER,     "v_stage_3",                          "Vacumm Pump Stage 3",                                       "kW",      "",     "GeoHourly", "conversion_type=1",       "",                              ""   },
    { SSC_INPUT,      SSC_NUMBER,     "GF_flowrate",                        "GF Flow Rate",                                               "lb/h",    "",     "GeoHourly", "conversion_type=1",       "",                              ""   },
    { SSC_INPUT,      SSC_NUMBER,     "qRejectByStage_1",                   "Heat Rejected by NCG Condenser Stage 1",                    "BTU/hr",  "",     "GeoHourly", "conversion_type=1",       "",                              ""   },
    { SSC_INPUT,      SSC_NUMBER,     "qRejectByStage_2",                   "Heat Rejected by NCG Condenser Stage 2",                    "BTU/hr",  "",     "GeoHourly", "conversion_type=1",       "",                              ""   },
    { SSC_INPUT,      SSC_NUMBER,     "qRejectByStage_3",                   "Heat Rejected by NCG Condenser Stage 3",                    "BTU/hr",  "",     "GeoHourly", "conversion_type=1",       "",                              ""   },
    { SSC_INPUT,      SSC_NUMBER,     "ncg_condensate_pump",                "Condensate Pump Work",                                      "kW",      "",     "GeoHourly", "conversion_type=1",       "",                              ""   },
    { SSC_INPUT,      SSC_NUMBER,     "cw_pump_work",                       "CW Pump Work",                                               "kW",      "",     "GeoHourly", "conversion_type=1",       "",                              ""   },
    { SSC_INPUT,      SSC_NUMBER,     "pressure_ratio_1",                   "Suction Steam Ratio 1",                                      "",        "",     "GeoHourly", "conversion_type=1",       "",                              ""   },
    { SSC_INPUT,      SSC_NUMBER,     "pressure_ratio_2",                   "Suction Steam Ratio 2",                                      "",        "",     "GeoHourly", "conversion_type=1",       "",                              ""   },
    { SSC_INPUT,      SSC_NUMBER,     "pressure_ratio_3",                   "Suction Steam Ratio 3",                                      "",        "",     "GeoHourly", "conversion_type=1",       "",                              ""   },
    { SSC_INPUT,      SSC_NUMBER,     "condensate_pump_power",              "hp",                                                         "",        "",     "GeoHourly", "conversion_type=1",       "",                              ""   },
    { SSC_INPUT,      SSC_NUMBER,     "cwflow",                             "Cooling Water Flow",                                         "lb/h",    "",     "GeoHourly", "conversion_type=1",       "",                              ""   },
    { SSC_INPUT,      SSC_NUMBER,     "cw_pump_head",                       "Cooling Water Pump Head",                                    "lb/h",    "",     "GeoHourly", "conversion_type=1",       "",                              ""   },
    { SSC_INPUT,      SSC_NUMBER,     "spec_vol",                           "Specific Volume",                                            "cft/lb",  "",     "GeoHourly", "conversion_type=1",       "",                              ""   },
    { SSC_INPUT,      SSC_NUMBER,     "spec_vol_lp",                        "LP Specific Volume",                                         "cft/lb",  "",     "GeoHourly", "conversion_type=1",       "",                              ""   },
    { SSC_INPUT,      SSC_NUMBER,     "x_hp",                               "HP Mass Fraction",                                           "%",       "",     "GeoHourly", "conversion_type=1",       "",                              ""   },
    { SSC_INPUT,      SSC_NUMBER,     "x_lp",                               "LP Mass Fraction",                                           "%",       "",     "GeoHourly", "conversion_type=1",       "",                              ""   },
    { SSC_INPUT,      SSC_NUMBER,     "hp_flash_pressure",                  "HP Flash Pressure",                                          "psia",    "",     "GeoHourly", "conversion_type=1",       "",                              ""   },
    { SSC_INPUT,      SSC_NUMBER,     "lp_flash_pressure",                  "LP Flash Pressure",                                          "psia",    "",     "GeoHourly", "conversion_type=1",       "",                              ""   },
    { SSC_INPUT,      SSC_NUMBER,     "flash_count",                        "Flash Count",                                                "(1 -2)",  "",     "GeoHourly", "conversion_type=1",       "",                              ""   },

    // Drilling
    { SSC_INPUT,      SSC_NUMBER,     "num_wells_getem",                    "Number of production wells required",                       "",        "",     "GeoHourly", "",                        "",                              "?=3.667" },
    { SSC_INPUT,      SSC_NUMBER,     "num_wells_getem_prod_drilled",       "Number of drilled production wells",                        "",        "",     "GeoHourly", "",                        "",                              ""   },
    { SSC_INPUT,      SSC_NUMBER,     "num_wells_getem_prod_failed",        "Number of failed production wells",                         "",        "",     "GeoHourly", "",                        "",                              ""   },    { SSC_INPUT,      SSC_NUMBER,     "num_wells_getem_inj_drilled",        "Number of drilled injection wells",                         "0/1",     "0=LargerDiameter,1=SmallerDiameter", "GeoHourly", "",                        "",                              ""   },
    { SSC_INPUT,      SSC_NUMBER,     "num_wells_getem_inj_drilled",        "Number of drilled injection wells",                         "",        "",     "GeoHourly", "",                        "",                              ""   },
    { SSC_INPUT,      SSC_NUMBER,     "num_wells_getem_prod_inj_sum",       "Sum of production and injection drilled wells",             "",        "",     "GeoHourly", "",                        "",                              ""   },

    { SSC_INPUT,      SSC_NUMBER,     "pump_depth_ft",                      "Pump depth",                                                "ft",      "",     "GeoHourly", "",                        "",                              "?=1123120" },
    { SSC_INPUT,      SSC_NUMBER,     "inj_pump_hp",                        "Injection pump power",                                      "hp",      "",     "GeoHourly", "",                        "",                              ""   },
    { SSC_INPUT,      SSC_NUMBER,     "pump_size_hp",                       "Production pump power",                                     "hp",      "",     "GeoHourly", "",                        "",                              "?733.646" },


    var_info_invalid
};

#endif
