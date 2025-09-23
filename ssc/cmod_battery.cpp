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


#include <math.h>

#include "cmod_battery.h"
#include "common.h"
#include "core.h"
#include "lib_battery.h"
#include "lib_battery_dispatch_automatic_btm.h"
#include "lib_battery_dispatch_automatic_fom.h"
#include "lib_battery_dispatch_pvsmoothing_fom.h"
#include "lib_battery_dispatch_manual.h"
#include "lib_battery_powerflow.h"
#include "lib_power_electronics.h"
#include "lib_resilience.h"
#include "lib_shared_inverter.h"
#include "lib_time.h"
#include "lib_util.h"
#include "lib_utility_rate.h"

var_info vtab_battery_inputs[] = {
    /*   VARTYPE           DATATYPE         NAME                                            LABEL                                                   UNITS      META                   GROUP           REQUIRED_IF                 CONSTRAINTS                      UI_HINTS*/

    // simulation inputs - required only if lifetime analysis
    { SSC_INPUT,        SSC_NUMBER,      "system_use_lifetime_output",                 "Enable lifetime simulation",                                  "0/1",     "0=SingleYearRepeated,1=RunEveryYear",                     "Lifetime",             "?=0",                        "BOOLEAN",                        "" },
    { SSC_INPUT,        SSC_NUMBER,      "analysis_period",                            "Lifetime analysis period",                                "years",   "The number of years in the simulation",                   "Lifetime",             "system_use_lifetime_output =1",   "",                               "" },

    // configuration inputs
    { SSC_INPUT,        SSC_NUMBER,      "batt_chem",                                  "Battery chemistry",                                       "",        "0=LeadAcid,1=LiIon",   "BatteryCell",       "",                           "",                              "" },
    { SSC_INPUT,        SSC_NUMBER,      "inverter_model",                             "Inverter model specifier",                                "",        "0=cec,1=datasheet,2=partload,3=coefficientgenerator,4=generic","Inverter","?=4", "INTEGER,MIN=0,MAX=4",           "" },
    { SSC_INPUT,        SSC_NUMBER,      "inverter_count",                             "Number of inverters",                                     "",        "",                     "Inverter"        "",                            "",                              "" },
    { SSC_INPUT,        SSC_NUMBER,      "inv_snl_eff_cec",                            "Inverter Sandia CEC Efficiency",                          "%",       "",                     "Inverter",       "",                           "",                              "" },
    { SSC_INPUT,        SSC_NUMBER,      "inv_snl_paco",                               "Inverter Sandia Maximum AC Power",                        "Wac",     "",                     "Inverter",       "",                           "",                              "" },
    { SSC_INPUT,        SSC_NUMBER,      "inv_ds_eff",                                 "Inverter Datasheet Efficiency",                           "%",       "",                     "Inverter",       "",                           "",                              "" },
    { SSC_INPUT,        SSC_NUMBER,      "inv_ds_paco",                                "Inverter Datasheet Maximum AC Power",                     "Wac",     "",                     "Inverter",       "",                           "",                              "" },
    { SSC_INPUT,        SSC_NUMBER,      "inv_pd_eff",                                 "Inverter Partload Efficiency",                            "%",       "",                     "Inverter",       "",                           "",                              "" },
    { SSC_INPUT,        SSC_NUMBER,      "inv_pd_paco",                                "Inverter Partload Maximum AC Power",                      "Wac",     "",                     "Inverter",       "",                           "",                              "" },
    { SSC_INPUT,        SSC_NUMBER,      "inv_cec_cg_eff_cec",                         "Inverter Coefficient Generator CEC Efficiency",           "%",       "",                     "Inverter",       "",                           "",                              "" },
    { SSC_INPUT,        SSC_NUMBER,      "inv_cec_cg_paco",                            "Inverter Coefficient Generator Max AC Power",             "Wac",       "",                   "Inverter",       "",                           "",                              "" },
    { SSC_INPUT,        SSC_NUMBER,      "batt_ac_or_dc",                              "Battery interconnection (AC or DC)",                      "",        "0=DC_Connected,1=AC_Connected",                  "BatterySystem",       "",                           "",                              "" },
    { SSC_INPUT,        SSC_NUMBER,      "batt_dc_dc_efficiency",                      "System DC to battery DC efficiency",                          "",        "",                     "BatterySystem",       "",                           "",                              "" },
    { SSC_INPUT,        SSC_NUMBER,      "dcoptimizer_loss",                           "DC optimizer loss",                      "",        "",                     "Losses",       "",                           "",                              "" },
    { SSC_INPUT,        SSC_NUMBER,      "batt_dc_ac_efficiency",                      "Battery DC to AC efficiency",                             "",        "",                     "BatterySystem",       "",                           "",                              "" },
    { SSC_INPUT,        SSC_NUMBER,      "batt_ac_dc_efficiency",                      "Inverter AC to battery DC efficiency",                    "",        "",                     "BatterySystem",       "",                           "",                              "" },
    { SSC_INPUT,        SSC_NUMBER,      "batt_meter_position",                        "Position of battery relative to electric meter",          "",        "0=BehindTheMeter,1=FrontOfMeter",                     "BatterySystem",       "",                           "",                              "" },
    { SSC_INPUT,        SSC_NUMBER,      "batt_inverter_efficiency_cutoff",            "Inverter efficiency at which to cut battery charge or discharge off",          "%",        "","BatterySystem",       "",                           "",                              "" },
    { SSC_INPUT,        SSC_ARRAY,       "batt_losses",                                "Battery system losses at each timestep (kW DC for DC connected, AC for AC connected)",                  "kW",       "",                     "BatterySystem",       "?=0",                        "",                             "" },
    { SSC_INPUT,        SSC_ARRAY,       "batt_losses_charging",                       "Battery system losses when charging (kW DC for DC connected, AC for AC connected)",                     "kW",       "",                     "BatterySystem",       "?=0",                        "",                             "" },
    { SSC_INPUT,        SSC_ARRAY,       "batt_losses_discharging",                    "Battery system losses when discharging (kW DC for DC connected, AC for AC connected)",                  "kW",       "",                     "BatterySystem",       "?=0",                        "",                             "" },
    { SSC_INPUT,        SSC_ARRAY,       "batt_losses_idle",                           "Battery system losses when idle (kW DC for DC connected, AC for AC connected)",                         "kW",       "",                     "BatterySystem",       "?=0",                        "",                             "" },
    { SSC_INPUT,        SSC_NUMBER,      "batt_loss_choice",                           "Loss power input option",                                 "0/1",      "0=Monthly,1=TimeSeries",                     "BatterySystem",       "?=0",                        "",                             "" },

    // Current and capacity battery inputs
    { SSC_INPUT,        SSC_NUMBER,      "batt_current_choice",                        "Limit cells by current or power",                         "",        "",                     "BatterySystem",       "",                           "",                              "" },
    { SSC_INOUT,        SSC_NUMBER,      "batt_computed_strings",                      "Battery number of strings of cells",                              "",        "",                     "BatterySystem",       "",                           "",                              "" },
    { SSC_INOUT,        SSC_NUMBER,      "batt_computed_series",                       "Battery number of cells in series",                               "",        "",                     "BatterySystem",       "",                           "",                              "" },
    { SSC_INOUT,        SSC_NUMBER,      "batt_computed_bank_capacity",                "Battery computed bank capacity",                                  "kWh",     "",                     "BatterySystem",       "",                           "",                              "" },
    { SSC_INOUT,        SSC_NUMBER,      "batt_current_charge_max",                    "Battery maximum charge current",                                  "A",       "",                     "BatterySystem",       "",                           "",                              "" },
    { SSC_INOUT,        SSC_NUMBER,      "batt_current_discharge_max",                 "Battery maximum discharge current",                               "A",       "",                     "BatterySystem",       "",                           "",                              "" },
    { SSC_INOUT,        SSC_NUMBER,      "batt_power_charge_max_kwdc",                 "Battery maximum charge power (DC)",                               "kWdc",    "",                    "BatterySystem",       "",                           "",                              "" },
    { SSC_INOUT,        SSC_NUMBER,      "batt_power_discharge_max_kwdc",              "Battery maximum discharge power (DC)",                            "kWdc",    "",                    "BatterySystem",       "",                           "",                              "" },
    { SSC_INOUT,        SSC_NUMBER,      "batt_power_charge_max_kwac",                 "Battery maximum charge power (AC)",                               "kWac",    "",                    "BatterySystem",       "",                           "",                              "" },
    { SSC_INOUT,        SSC_NUMBER,      "batt_power_discharge_max_kwac",              "Battery maximum discharge power (AC)",                            "kWac",    "",                    "BatterySystem",       "",                           "",                              "" },


    // Voltage discharge curve
    { SSC_INPUT,        SSC_NUMBER,      "batt_voltage_choice",                        "Battery voltage input option",                            "0/1",      "0=UseVoltageModel,1=InputVoltageTable",                    "BatteryCell",       "?=0",                        "",                             "" },
    { SSC_INPUT,        SSC_NUMBER,      "batt_Vfull",                                 "Fully charged cell voltage",                              "V",       "",                     "BatteryCell",       "",                           "",                              "" },
    { SSC_INPUT,        SSC_NUMBER,      "batt_Vexp",                                  "Cell voltage at end of exponential zone",                 "V",       "",                     "BatteryCell",       "",                           "",                              "" },
    { SSC_INPUT,        SSC_NUMBER,      "batt_Vnom",                                  "Cell voltage at end of nominal zone",                     "V",       "",                     "BatteryCell",       "",                           "",                              "" },
    { SSC_INPUT,        SSC_NUMBER,      "batt_Vnom_default",                          "Default nominal cell voltage",                            "V",       "",                     "BatteryCell",       "",                           "",                              "" },
    { SSC_INPUT,        SSC_NUMBER,      "batt_Qfull",                                 "Fully charged cell capacity",                             "Ah",      "",                     "BatteryCell",       "",                           "",                              "" },
    { SSC_INPUT,        SSC_NUMBER,      "batt_Qfull_flow",                            "Fully charged flow battery capacity",                     "Ah",      "",                     "BatteryCell",       "",                           "",                              "" },
    { SSC_INPUT,        SSC_NUMBER,      "batt_Qexp",                                  "Cell capacity at end of exponential zone",                "Ah",      "",                     "BatteryCell",       "",                           "",                              "" },
    { SSC_INPUT,        SSC_NUMBER,      "batt_Qnom",                                  "Cell capacity at end of nominal zone",                    "Ah",      "",                     "BatteryCell",       "",                           "",                              "" },
    { SSC_INPUT,        SSC_NUMBER,      "batt_Vcut",                                  "Cutoff voltage for battery rated capacity",               "V",       "",                     "BatteryCell",       "?=0",                        "",                              "" },
    { SSC_INPUT,        SSC_NUMBER,      "batt_C_rate",                                "Rate at which voltage vs. capacity curve input",          "",        "",                     "BatteryCell",       "",                           "",                              "" },
    { SSC_INPUT,        SSC_NUMBER,      "batt_resistance",                            "Internal resistance",                                     "Ohm",     "",                     "BatteryCell",       "",                           "",                              "" },
    { SSC_INPUT,		SSC_MATRIX,      "batt_voltage_matrix",                        "Battery voltage vs. depth-of-discharge",                 "",         "",                     "BatteryCell",       "",                           "",                             "" },

    // lead-acid inputs
    { SSC_INPUT,		SSC_NUMBER,		"LeadAcid_q20_computed",	                   "Capacity at 20-hour discharge rate",                     "Ah",       "",                     "BatteryCell",       "",                           "",                             "" },
    { SSC_INPUT,		SSC_NUMBER,		"LeadAcid_q10_computed",	                   "Capacity at 10-hour discharge rate",                     "Ah",       "",                     "BatteryCell",       "",                           "",                             "" },
    { SSC_INPUT,		SSC_NUMBER,		"LeadAcid_qn_computed",	                       "Capacity at discharge rate for n-hour rate",             "Ah",       "",                     "BatteryCell",       "",                           "",                             "" },
    { SSC_INPUT,		SSC_NUMBER,		"LeadAcid_tn",	                               "Time to discharge",                                      "h",        "",                     "BatteryCell",       "",                           "",                             "" },

    // charge limits and priority inputs
    { SSC_INPUT,        SSC_NUMBER,      "batt_initial_SOC",		                   "Initial state-of-charge",                                 "%",       "",                     "BatteryCell",       "",                           "",                              "" },
    { SSC_INPUT,        SSC_NUMBER,      "batt_minimum_SOC",		                   "Minimum allowed state-of-charge during nominal operation","%",       "",                     "BatteryCell",       "",                           "",                              "" },
    { SSC_INPUT,        SSC_NUMBER,      "batt_minimum_outage_SOC",		               "Minimum allowed state-of-charge during an outage",        "%",       "",                     "BatteryCell",       "",                           "",                              "" },
    { SSC_INPUT,        SSC_NUMBER,      "batt_maximum_SOC",                           "Maximum allowed state-of-charge",                         "%",       "",                     "BatteryCell",       "",                           "",                              "" },
    { SSC_INPUT,        SSC_NUMBER,      "batt_minimum_modetime",                      "Minimum time at charge state",                            "min",     "",                     "BatteryCell",       "",                           "",                              "" },

    // lifetime inputs
    { SSC_INPUT,		SSC_NUMBER,     "batt_life_model",                             "Battery life model specifier",                           "0/1/2",    "0=calendar/cycle,1=NMC,2=LMO/LTO", "BatteryCell",       "?=0",                           "",                             "" },
    { SSC_INPUT,		SSC_MATRIX,     "batt_lifetime_matrix",                        "Cycles vs capacity at different depths-of-discharge",    "",         "",                     "BatteryCell",       "en_batt=1&batt_life_model=0",                           "",                             "" },
    { SSC_INPUT,        SSC_NUMBER,     "batt_calendar_choice",                        "Calendar life degradation input option",                 "0/1/2",    "0=NoCalendarDegradation,1=LithiomIonModel,2=InputLossTable", "BatteryCell",       "en_batt=1&batt_life_model=0",                           "",                             "" },
    { SSC_INPUT,        SSC_MATRIX,     "batt_calendar_lifetime_matrix",               "Days vs capacity",                                       "",         "",                     "BatteryCell",       "en_batt=1&batt_life_model=0&batt_calendar_choice=2", "",                             "" },
    { SSC_INPUT,        SSC_NUMBER,     "batt_calendar_q0",                            "Calendar life model initial capacity cofficient",        "",         "",                     "BatteryCell",       "en_batt=1&batt_life_model=0&batt_calendar_choice=1",  "",                             "" },
    { SSC_INPUT,        SSC_NUMBER,     "batt_calendar_a",                             "Calendar life model coefficient",                        "1/sqrt(day)","",                   "BatteryCell",       "en_batt=1&batt_life_model=0&batt_calendar_choice=1",  "",                             "" },
    { SSC_INPUT,        SSC_NUMBER,     "batt_calendar_b",                             "Calendar life model coefficient",                        "K",        "",                     "BatteryCell",       "en_batt=1&batt_life_model=0&batt_calendar_choice=1",  "",                             "" },
    { SSC_INPUT,        SSC_NUMBER,     "batt_calendar_c",                             "Calendar life model coefficient",                        "K",        "",                     "BatteryCell",       "en_batt=1&batt_life_model=0&batt_calendar_choice=1",  "",                             "" },

    // replacement inputs
    { SSC_INPUT,        SSC_NUMBER,     "batt_replacement_capacity",                   "Capacity degradation at which to replace battery",       "%",        "",                     "BatterySystem",       "",                           "",                             "" },
    { SSC_INPUT,        SSC_NUMBER,     "batt_replacement_option",                     "Enable battery replacement?",                            "0=none,1=capacity based,2=user schedule", "", "BatterySystem", "?=0",                  "INTEGER,MIN=0,MAX=2",          "" },
    { SSC_INPUT,        SSC_ARRAY,      "batt_replacement_schedule_percent",           "Percentage of battery capacity to replace in each year", "%","length <= analysis_period",                  "BatterySystem",      "batt_replacement_option=2",   "",                             "" },
    { SSC_INPUT,        SSC_ARRAY,      "om_replacement_cost1",                        "Cost to replace battery per kWh",                        "$/kWh",    "",                     "BatterySystem",       "",                           "",                             "" },

    // thermal inputs
    { SSC_INPUT,        SSC_NUMBER,     "batt_mass",                                   "Battery mass",                                           "kg",       "",                     "BatterySystem",       "",                           "",                             "" },
    { SSC_INPUT,        SSC_NUMBER,     "batt_surface_area",                            "Battery surface area",                                   "m^2",      "",                     "BatterySystem",       "",                           "",                             "" },
    { SSC_INPUT,        SSC_NUMBER,     "batt_Cp",                                     "Battery specific heat capacity",                         "J/KgK",    "",                     "BatteryCell",       "",                           "",                             "" },
    { SSC_INPUT,        SSC_NUMBER,     "batt_h_to_ambient",                           "Heat transfer between battery and environment",          "W/m2K",    "",                     "BatteryCell",       "",                           "",                             "" },
    { SSC_INPUT,        SSC_ARRAY,      "batt_room_temperature_celsius",               "Temperature of storage room",                            "C", "length=1 for fixed, # of weatherfile records otherwise", "BatteryCell",        "",                           "",                             "" },
    { SSC_INPUT,        SSC_MATRIX,     "cap_vs_temp",                                 "Effective capacity as function of temperature",          "C,%",      "",                     "BatteryCell",       "",                           "",                             "" },

    // storage dispatch
    { SSC_INPUT,        SSC_ARRAY,      "dispatch_manual_charge",                      "Periods 1-6 charging from system allowed?",              "",         "",                     "BatteryDispatch",       "en_batt=1&en_standalone_batt=0&batt_dispatch_choice=3",                           "",                             "" },
    { SSC_INPUT,        SSC_ARRAY,      "dispatch_manual_fuelcellcharge",			  "Periods 1-6 charging from fuel cell allowed?",           "",         "",                      "BatteryDispatch",     "",                        "",                              "" },
    { SSC_INPUT,        SSC_ARRAY,      "dispatch_manual_discharge",                   "Periods 1-6 discharging allowed?",                       "",         "",                     "BatteryDispatch",       "en_batt=1&batt_dispatch_choice=3",                           "",                             "" },
    { SSC_INPUT,        SSC_ARRAY,      "dispatch_manual_btm_discharge_to_grid",        "Periods 1-6 behind the meter discharging to grid allowed?", "",      "",                     "BatteryDispatch",       "en_batt=1&batt_dispatch_choice=3&batt_meter_position=0",                           "",                             "" },
    { SSC_INPUT,        SSC_ARRAY,      "dispatch_manual_gridcharge",                  "Periods 1-6 grid charging allowed?",                     "",         "",                     "BatteryDispatch",       "en_batt=1&batt_dispatch_choice=3",                           "",                             "" },
    { SSC_INPUT,        SSC_ARRAY,      "dispatch_manual_percent_discharge",           "Periods 1-6 discharge percent",                          "%",        "",                     "BatteryDispatch",       "en_batt=1&batt_dispatch_choice=3",                           "",                             "" },
    { SSC_INPUT,        SSC_ARRAY,      "dispatch_manual_percent_gridcharge",          "Periods 1-6 gridcharge percent",                         "%",        "",                     "BatteryDispatch",       "en_batt=1&batt_dispatch_choice=3",                           "",                             "" },
    { SSC_INPUT,        SSC_MATRIX,     "dispatch_manual_sched",                       "Battery dispatch schedule for weekday",                  "",         "",                     "BatteryDispatch",       "en_batt=1&batt_dispatch_choice=3",                           "",                             "" },
    { SSC_INPUT,        SSC_MATRIX,     "dispatch_manual_sched_weekend",               "Battery dispatch schedule for weekend",                  "",         "",                     "BatteryDispatch",       "en_batt=1&batt_dispatch_choice=3",                           "",                             "" },
    { SSC_INPUT,        SSC_NUMBER,     "dispatch_manual_system_charge_first",         "System charges battery before meeting load",                 "0/1",    "0=LoadFirst,1=ChargeFirst","BatteryDispatch",      "en_batt=1&en_standalone_batt=0&batt_meter_position=0&batt_dispatch_choice=3&batt_dispatch_charge_only_system_exceeds_load=0",                           "",                             "" },
    { SSC_INPUT,        SSC_ARRAY,      "batt_target_power",                           "Grid target power for every time step",                  "kW",       "",                     "BatteryDispatch",       "en_batt=1&batt_meter_position=0&batt_dispatch_choice=1",                        "",                             "" },
    { SSC_INPUT,        SSC_ARRAY,      "batt_target_power_monthly",                   "Grid target power on monthly basis",                     "kW",       "",                     "BatteryDispatch",       "en_batt=1&batt_meter_position=0&batt_dispatch_choice=1",                        "",                             "" },
    { SSC_INPUT,        SSC_NUMBER,     "batt_target_choice",                          "Target power input option",                              "0/1",      "0=InputMonthlyTarget,1=InputFullTimeSeries", "BatteryDispatch", "en_batt=1&en_standalone_batt=0&batt_meter_position=0&batt_dispatch_choice=1",                        "",                             "" },
    { SSC_INPUT,        SSC_ARRAY,      "batt_custom_dispatch",                        "Custom battery power for every time step",               "kW",       "kWAC if AC-connected, else kWDC", "BatteryDispatch",       "en_batt=1&en_standalone_batt=0&batt_dispatch_choice=2","",                         "" },
    { SSC_INPUT,        SSC_NUMBER,     "batt_dispatch_choice",                        "Battery dispatch algorithm",                             "0/1/2/3/4/5", "If behind the meter: 0=PeakShaving,1=InputGridTarget,2=InputBatteryPower,3=ManualDispatch,4=RetailRateDispatch,5=SelfConsumption if front of meter: 0=AutomatedEconomic,1=PV_Smoothing,2=InputBatteryPower,3=ManualDispatch",                    "BatteryDispatch",       "en_batt=1",                        "",                             "" },
    { SSC_INPUT,        SSC_NUMBER,     "batt_dispatch_auto_can_fuelcellcharge",       "Charging from fuel cell allowed for automated dispatch?", "0/1",       "",                   "BatteryDispatch",       "",                           "",                             "" },
    { SSC_INPUT,        SSC_NUMBER,     "batt_dispatch_auto_can_gridcharge",           "Grid charging allowed for automated dispatch?",          "0/1",       "",                    "BatteryDispatch",       "",                           "",                             "" },
    { SSC_INPUT,        SSC_NUMBER,     "batt_dispatch_auto_can_charge",               "System charging allowed for automated dispatch?",            "0/1",       "",                "BatteryDispatch",       "",                           "",                             "" },
    { SSC_INPUT,        SSC_NUMBER,     "batt_dispatch_auto_can_clipcharge",           "Battery can charge from clipped power?",                 "0/1",   "",                        "BatteryDispatch",       "",                           "",                             "" },
    { SSC_INPUT,        SSC_NUMBER,     "batt_dispatch_auto_can_curtailcharge",        "Battery can charge from grid-limited system power?",                "0/1",   "",                       "BatteryDispatch",       "",                           "",                             "" },
    { SSC_INPUT,        SSC_NUMBER,     "batt_dispatch_auto_btm_can_discharge_to_grid", "Behind the meter battery can discharge to grid?",        "0/1",   "",                       "BatteryDispatch",       "",                           "",                             "" },
    { SSC_INPUT,        SSC_NUMBER,     "batt_dispatch_charge_only_system_exceeds_load",  "Battery can charge from system only when system output exceeds load", "0/1",   "",        "BatteryDispatch",       "en_batt=1&en_standalone_batt=0&batt_meter_position=0",                           "",                             "" },
    { SSC_INPUT,        SSC_NUMBER,     "batt_dispatch_discharge_only_load_exceeds_system","Battery can discharge battery only when load exceeds system output", "0/1",   "",        "BatteryDispatch",       "en_batt=1&en_standalone_batt=0&batt_meter_position=0",                           "",                             "" },
    { SSC_INPUT,        SSC_NUMBER,     "batt_look_ahead_hours",                       "Hours to look ahead in automated dispatch",              "hours",    "",                     "BatteryDispatch",       "",                           "MIN=1",                             "" },
    { SSC_INPUT,        SSC_NUMBER,     "batt_dispatch_update_frequency_hours",        "Frequency to update the look-ahead dispatch",            "hours",    "",                     "BatteryDispatch",       "",                           "",                             "" },

    // PV smoothing specific inputs
    { SSC_INPUT,        SSC_NUMBER,      "batt_dispatch_pvs_nameplate_ac",             "Nameplate for pv smoothing",                            "kWac",         "",   "BatteryDispatch",       "en_batt=1&en_standalone_batt=0&batt_meter_position=1&batt_dispatch_choice=1",                           "",                             "" },
    { SSC_INPUT,        SSC_NUMBER,      "batt_dispatch_pvs_ac_lb_enable",             "Enable AC lower bound",                                 "0/1",         "",   "BatteryDispatch",       "en_batt=1&en_standalone_batt=0&batt_meter_position=1&batt_dispatch_choice=1",                           "",                             "" },
    { SSC_INPUT,        SSC_NUMBER,      "batt_dispatch_pvs_ac_lb",                    "AC lower bound",                                        "fraction of nameplate",         "",                     "BatteryDispatch",       "en_batt=1&en_standalone_batt=0&batt_meter_position=1&batt_dispatch_choice=1",                           "",                             "" },
    { SSC_INPUT,        SSC_NUMBER,      "batt_dispatch_pvs_ac_ub_enable",             "Enable AC upper bound",                                 "0/1",         "",   "BatteryDispatch",       "en_batt=1&en_standalone_batt=0&batt_meter_position=1&batt_dispatch_choice=1",                           "",                             "" },
    { SSC_INPUT,        SSC_NUMBER,      "batt_dispatch_pvs_ac_ub",                    "AC upper bound",                                        "fraction of nameplate",         "",                     "BatteryDispatch",       "en_batt=1&en_standalone_batt=0&batt_meter_position=1&batt_dispatch_choice=1",                           "",                             "" },
    { SSC_INPUT,        SSC_NUMBER,      "batt_dispatch_pvs_curtail_as_control",       "Correct up-ramp violations",                            "0/1",         "",   "BatteryDispatch",       "en_batt=1&en_standalone_batt=0&batt_meter_position=1&batt_dispatch_choice=1",                           "",                             "" },
    { SSC_INPUT,        SSC_NUMBER,      "batt_dispatch_pvs_curtail_if_violation",     "Curtail violations",                                    "0/1",         "",   "BatteryDispatch",       "en_batt=1&en_standalone_batt=0&batt_meter_position=1&batt_dispatch_choice=1",                           "",                             "" },
    { SSC_INPUT,        SSC_NUMBER,      "batt_dispatch_pvs_short_forecast_enable",    "Enable short term power forecast",                      "0/1",         "",   "BatteryDispatch",       "en_batt=1&en_standalone_batt=0&batt_meter_position=1&batt_dispatch_choice=1",                           "",                             "" },
    { SSC_INPUT,        SSC_NUMBER,      "batt_dispatch_pvs_forecast_shift_periods",   "Forecasting window",                                    "periods of ramp intervals",         "",                     "BatteryDispatch",       "en_batt=1&en_standalone_batt=0&batt_meter_position=1&batt_dispatch_choice=1",                           "",                             "" },
    { SSC_INPUT,        SSC_NUMBER,      "batt_dispatch_pvs_timestep_multiplier",      "Ramp timestep multiplier",                              "",         "",                     "BatteryDispatch",       "en_batt=1&en_standalone_batt=0&batt_meter_position=1&batt_dispatch_choice=1",                           "",                             "" },
    { SSC_INPUT,        SSC_NUMBER,      "batt_dispatch_pvs_max_ramp",                 "Maximum ramp rate",                                     "% of nameplate per ramp interval",         "",                     "BatteryDispatch",       "en_batt=1&en_standalone_batt=0&batt_meter_position=1&batt_dispatch_choice=1",                           "",                             "" },
    { SSC_INPUT,        SSC_NUMBER,      "batt_dispatch_pvs_soc_rest",                 "Battery resting SOC",                                   "%",         "",                     "BatteryDispatch",       "en_batt=1&en_standalone_batt=0&batt_meter_position=1&batt_dispatch_choice=1",                           "",                             "" },
    { SSC_INPUT,        SSC_NUMBER,      "batt_dispatch_pvs_kp",                       "Track PV power multiplier (kp)",                        "",         "",                     "BatteryDispatch",       "en_batt=1&en_standalone_batt=0&batt_meter_position=1&batt_dispatch_choice=1",                           "",                             "" },
    { SSC_INPUT,        SSC_NUMBER,      "batt_dispatch_pvs_ki",                       "Return to rest SOC multiplier (ki)",                    "",         "",                     "BatteryDispatch",       "en_batt=1&en_standalone_batt=0&batt_meter_position=1&batt_dispatch_choice=1",                           "",                             "" },
    { SSC_INPUT,        SSC_NUMBER,      "batt_dispatch_pvs_kf",                       "Forecast accumulation error multiplier (kf)",           "",         "",                     "BatteryDispatch",       "en_batt=1&en_standalone_batt=0&batt_meter_position=1&batt_dispatch_choice=1",                           "",                             "" },



    // Dispatch forecast - optional parameters used in cmod_pvsamv1
    { SSC_INPUT,        SSC_NUMBER,     "batt_dispatch_wf_forecast_choice",            "Weather forecast choice for automatic dispatch",                 "0/1/2",   "0=LookAhead,1=LookBehind,2=InputForecast", "BatteryDispatch",       "?=0",                        "",                             "" },
    { SSC_INPUT,        SSC_NUMBER,     "batt_dispatch_load_forecast_choice",          "Load forecast choice for automatic dispatch",                 "0/1/2",   "0=LookAhead,1=LookBehind,2=InputForecast", "BatteryDispatch",       "?=0",                        "",                             "" },
    { SSC_INPUT,        SSC_ARRAY,      "batt_pv_clipping_forecast",                   "PV clipping forecast",                                    "kW",       "Length either 8760 * steps per hour (values repeat each year) or 8760 * steps per hour * analysis period",                     "BatteryDispatch",       "",  "",          "" },
    { SSC_INPUT,        SSC_ARRAY,      "batt_pv_ac_forecast",                         "PV ac power forecast",                                    "kW",       "Length either 8760 * steps per hour (values repeat each year) or 8760 * steps per hour * analysis period",                     "BatteryDispatch",       "",  "",          "" },
    { SSC_INPUT,        SSC_ARRAY,      "batt_load_ac_forecast",                       "Load ac power forecast",                                  "kW",       "Length either 8760 or 8760 * steps per hour",                     "BatteryDispatch",       "",  "",          "" },
    { SSC_INPUT,        SSC_ARRAY,      "batt_load_ac_forecast_escalation",            "Annual load escalation for ac power forecast",            "kW",       "length <= analysis_period",                     "BatteryDispatch",       "",  "",          "" },

    //  cycle cost inputs
    { SSC_INPUT,        SSC_NUMBER,     "batt_cycle_cost_choice",                      "Use SAM cost model for degradaton penalty or input custom via batt_cycle_cost", "0/1",     "0=UseCostModel,1=InputCost", "BatteryDispatch", "?=0",                           "",                             "" },
    { SSC_INPUT,        SSC_ARRAY,      "batt_cycle_cost",                             "Input battery cycle degradaton penalty per year",                      "$/cycle-kWh","length 1 or analysis_period, length 1 will be extended using inflation", "BatteryDispatch",       "batt_cycle_cost_choice=1",                           "",                             "" },

    { SSC_INPUT,        SSC_NUMBER,     "inflation_rate",                              "Inflation rate",                                          "%",      "", "Lifetime", "?=0", "MIN=-99", "" },
    { SSC_INPUT,        SSC_ARRAY,      "load_escalation",                             "Annual load escalation",                                  "%/year", "", "Load",                                               "?=0",                                "",                    "" },
    { SSC_INPUT,        SSC_ARRAY,      "om_batt_replacement_cost",                    "Replacement cost 1",                                      "$/kWh",  "", "System Costs"         , "?=0.0"          , ""                      , "" },
    { SSC_INPUT,        SSC_NUMBER,     "om_replacement_cost_escal",                   "Replacement cost escalation",                             "%/year", "", "System Costs"         , "?=0.0"          , ""                      , "" },

    { SSC_INPUT,        SSC_ARRAY,      "om_batt_variable_cost",                       "Battery production-based System Costs amount",            "$/MWh",  "", "System Costs"         , "?=0.0"          , ""                      , "" },
    { SSC_INPUT,        SSC_NUMBER,     "om_production_escal",                         "Production-based O&M escalation",                         "%/year", "", "System Costs",            "?=0.0",                 "",                                         "" },

    // Powerflow calculation inputs
    { SSC_INPUT,       SSC_ARRAY,       "fuelcell_power",                              "Electricity from fuel cell AC",                           "kW",     "", "FuelCell",              "",                           "",                         "" },


    var_info_invalid
};

var_info vtab_battery_outputs[] = {
    // Capacity, Voltage, Charge outputs
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_q0",                                    "Battery total charge",                                   "Ah",       "",                     "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_q1",                                    "Battery available charge",                               "Ah",       "",                     "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_q2",                                    "Battery bound charge",                                   "Ah",       "",                     "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_SOC",                                   "Battery state of charge",                                "%",        "",                     "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_DOD",                                   "Battery cycle depth of discharge",                       "%",        "",                     "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_qmaxI",                                 "Battery maximum capacity at current",                    "Ah",       "",                     "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_qmax",                                  "Battery maximum charge with degradation",                "Ah",       "",                     "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_qmax_thermal",                          "Battery maximum charge at temperature",                  "Ah",       "",                     "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_I",                                     "Battery current",                                        "A",        "",                     "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_voltage_cell",                          "Battery cell voltage",                                   "V",        "",                     "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_voltage",                               "Battery voltage",	                                     "V",        "",                     "Battery",       "",                           "",                              "" },

    // Lifecycle related outputs
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_DOD_cycle_average",                     "Battery average cycle DOD",                              "",         "",                     "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_cycles",                                "Battery number of cycles",                               "",         "",                     "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_temperature",                           "Battery temperature",                                    "C",        "",                     "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_capacity_percent",                      "Battery relative capacity to nameplate",                 "%",        "",                     "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_capacity_percent_cycle",                "Battery relative capacity to nameplate (cycling)",       "%",        "",                     "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_capacity_percent_calendar",             "Battery relative capacity to nameplate (calendar)",      "%",        "",                     "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_capacity_thermal_percent",              "Battery capacity percent for temperature",               "%",        "",                     "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_bank_replacement",                      "Battery bank replacements per year",                     "number/year", "",                  "Battery",       "",                           "",                              "" },

    // Power outputs at native timestep
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_power",                                 "Electricity to/from battery AC",                           "kW",      "",                       "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_power_dc",                              "Electricity to/from battery DC",                           "kW",      "",                       "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "grid_power",                                 "Electricity to/from grid AC",                              "kW",      "",                       "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "system_to_load",                             "Electricity to load from system AC",                       "kW",      "",                       "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_to_load",                               "Electricity to load from battery AC",                      "kW",      "",                       "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "grid_to_load",                               "Electricity to load from grid AC",                         "kW",      "",                       "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "system_to_batt",                             "Electricity to battery from system AC",                    "kW",      "",                       "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "system_to_batt_dc",                          "Electricity to battery from system DC",                    "kW",      "",                       "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "fuelcell_to_batt",                           "Electricity to battery from fuel cell AC",                 "kW",      "",                       "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "grid_to_batt",                               "Electricity to battery from grid AC",                      "kW",      "",                       "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "system_to_grid",                             "Electricity to grid from system AC",                       "kW",      "",                       "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_to_grid",                               "Electricity to grid from battery AC",                      "kW",      "",                       "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_to_system_load",                        "Electricity to system loads from battery AC",              "kW",      "",                       "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "interconnection_loss",                       "Electricity loss due to curtailment interconnection outage", "kW",      "",             "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_conversion_loss",                       "Battery loss from power electronics",         "kW",      "",                       "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_system_loss",                           "Battery loss from ancillary equipment",     "kW",      "",                       "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_to_inverter_dc",                        "Electricity to inverter from battery DC",     "kW",      "",                       "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "grid_power_target",                          "Electricity grid power target for automated dispatch","kW","",                               "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_power_target",                          "Electricity battery power target for automated dispatch","kW","",                            "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_cost_to_cycle",                         "Battery computed cycle degradation penalty",            "$/cycle-kWh", "",                       "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "market_sell_rate_series_yr1",                "Power price for battery dispatch",                             "$/MWh", "",                         "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_revenue_gridcharge",					   "Revenue to charge from grid",                           "$/kWh", "",                         "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_revenue_charge",                        "Revenue to charge from system",                         "$/kWh", "",                         "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_revenue_clipcharge",                    "Revenue to charge from clipped",                        "$/kWh", "",                         "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_revenue_discharge",                     "Revenue to discharge",                                  "$/kWh", "",                         "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "gen_without_battery",                        "Power produced without the battery or curtailment",    "kW","",                      "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "crit_load_unmet",                            "Critical load unmet in this timestep",                  "kW","",                      "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "crit_load",                                  "Critical load in this timestep",                        "kW","",                      "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "outage_losses_unmet",                        "Battery and system losses unmet in this timestep",     "kW","",                      "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_dispatch_period",                       "Battery manual dispatch period in this timestep",              "","",                      "Battery",       "",                           "",                              "" },


    // PV Smoothing
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_pvs_PV_ramp_interval",                  "PV smoothing PV power sampled", "kW", "", "Battery", "", "", "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_pvs_forecast_pv_energy",                "PV smoothing PV power forecast",              "kW",      "",                       "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_pvs_P_pv_ac",                           "PV smoothing PV power before smoothing",              "kW",      "",                       "Battery",       "",                           "",                              "" },

    { SSC_OUTPUT,        SSC_ARRAY,      "batt_pvs_outpower",                          "PV smoothing outpower",                                "kW",      "",                       "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_pvs_battpower",                         "PV smoothing battpower",                                "kW",      "",                       "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_pvs_battsoc",                           "PV smoothing battery SOC",                             "%",      "",                       "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_pvs_curtail",                           "PV smoothing curtailed power",                         "kW",      "",                       "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_pvs_violation_list",                    "PV smoothing violation",                               "",      "",                       "Battery",       "",                           "",                              "" },

    { SSC_OUTPUT,        SSC_NUMBER,      "batt_pvs_violation_count",                  "PV smoothing violation count",                               "",      "",                       "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_NUMBER,      "batt_pvs_violation_percent",                "PV smoothing violation percent (of all intervals-including nighttime)",                               "%",      "",                       "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_NUMBER,      "batt_pvs_energy_to_grid_percent",           "PV smoothing energy to grid percent (loss due to curtail and battery loss)",                               "%",      "",                       "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_NUMBER,      "batt_pvs_energy_to_grid_percent_sam",       "PV smoothing energy to grid percent actual (loss due to curtail and battery loss)",                               "%",      "",                       "Battery",       "",                           "",                              "" },

    // Self-consumption outputs
    { SSC_OUTPUT,        SSC_NUMBER,      "num_ts_load_met_by_system_yr1",                  "Number of timesteps electric load met by system (year 1)",     "",      "",                       "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_NUMBER,      "percent_ts_load_met_by_system_yr1",              "Percent of timesteps electric load met by system (year 1)",     "",      "",                       "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_NUMBER,      "num_ts_load_met_by_system_lifetime",             "Number of timesteps electric load met by system (lifetime)",     "",      "",                       "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_NUMBER,      "percent_ts_load_met_by_system_lifetime",         "Percent of timesteps electric load met by system (lifetime)",     "",      "",                       "Battery",       "",                           "",                              "" },

    // validation outputs at ramp interval - use for debugging and remove for release
/*
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_pvs_outpower_vec",                          "PV smoothing outpower at ramp interval",                                "kW",      "",                       "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_pvs_battpower_vec",                         "PV smoothing battpower at ramp interval",                                "kW",      "",                       "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_pvs_battsoc_vec",                           "PV smoothing battery SOC at ramp interval",                             "%",      "",                       "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_pvs_curtail_vec",                           "PV smoothing curtailed power at ramp interval",                         "kW",      "",                       "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_pvs_violation_list_vec",                    "PV smoothing violation at ramp interval",                               "",      "",                       "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_pvs_PV_ramp_interval_vec",                  "PV smoothing sampled power at ramp interval",                         "kW",      "",                       "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_pvs_forecast_pv_energy_vec",                "PV smoothing forecast energy at ramp interval",                               "",      "",                       "Battery",       "",                           "",                              "" },
*/

    // monthly outputs
    { SSC_OUTPUT,        SSC_ARRAY,      "monthly_system_to_load",                     "Energy to load from system",                            "kWh",      "",                      "Battery",       "",                          "LENGTH=12",                     "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "monthly_batt_to_load",                       "Energy to load from battery",                           "kWh",      "",                      "Battery",       "",                          "LENGTH=12",                     "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "monthly_grid_to_load",                       "Energy to load from grid",                              "kWh",      "",                      "Battery",       "",                          "LENGTH=12",                     "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "monthly_system_to_grid",                     "Energy to grid from system",                            "kWh",      "",                      "Battery",       "",                          "LENGTH=12",                     "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "monthly_batt_to_grid",                       "Energy to grid from battery",                           "kWh",      "",                      "Battery",       "",                          "LENGTH=12",                     "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "monthly_system_to_batt",                     "Energy to battery from system",                         "kWh",      "",                      "Battery",       "",                          "LENGTH=12",                     "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "monthly_grid_to_batt",                       "Energy to battery from grid",                           "kWh",      "",                      "Battery",       "",                          "LENGTH=12",                     "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "monthly_interconnection_loss",               "Energy loss due to curtailment, interconnection, or outage", "kWh", "",                      "Battery",       "",                          "LENGTH = 12",                     "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "monthly_crit_load_unmet",                    "Critical load energy unmet",                                 "kWh",      "",                      "Battery",       "",                           "LENGTH=12",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "monthly_crit_load_unmet_percentage",         "Critical load unmet percentage",                         "%",      "",                      "Battery",       "",                           "LENGTH=12",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "monthly_crit_load",                          "Critical load energy",                                  "kWh",      "",                      "Battery",       "",                           "LENGTH=12",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "monthly_outage_losses_unmet",                "Battery and system losses unmet energy",                      "kWh",      "",                      "Battery",       "",                           "LENGTH=12",                              "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "monthly_batt_to_system_load",                "Energy to system loads from battery",               "kWh",      "",                      "Battery",       "",                          "LENGTH=12",                     "" },


    // annual metrics
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_annual_charge_from_system",             "Battery annual energy charged from system",                 "kWh",      "",                      "Battery",       "",                           "",                               "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_annual_charge_from_grid",               "Battery annual energy charged from grid",               "kWh",      "",                      "Battery",       "",                           "",                               "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_annual_charge_energy",                  "Battery annual energy charged",                         "kWh",      "",                      "Battery",       "",                           "",                               "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_annual_discharge_energy",               "Battery annual energy discharged",                      "kWh",      "",                      "Battery",       "",                           "",                               "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_annual_energy_loss",                    "Battery annual energy loss",                            "kWh",      "",                      "Battery",       "",                           "",                               "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "batt_annual_energy_system_loss",             "Battery annual system energy loss",                     "kWh",      "",                      "Battery",       "",                           "",                               "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "annual_export_to_grid_energy",               "Annual energy exported to grid",                        "kWh",      "",                      "Battery",       "",                           "",                               "" },
    { SSC_OUTPUT,        SSC_ARRAY,      "annual_import_to_grid_energy",               "Annual energy imported from grid",                      "kWh",      "",                      "Battery",       "",                           "",                               "" },

    // single value metrics
    { SSC_OUTPUT,        SSC_NUMBER,     "average_battery_conversion_efficiency",      "Battery average cycle conversion efficiency",           "%",        "",                      "Annual",        "",                           "",                               "" },
    { SSC_OUTPUT,        SSC_NUMBER,     "average_battery_roundtrip_efficiency",       "Battery average roundtrip efficiency",                  "%",        "",                      "Annual",        "",                           "",                               "" },
    { SSC_OUTPUT,        SSC_NUMBER,     "batt_system_charge_percent",                 "Battery charge energy charged from system",             "%",        "",                      "Annual",        "",                           "",                               "" },
    { SSC_OUTPUT,        SSC_NUMBER,     "batt_grid_charge_percent",                 "Battery charge energy charged from grid",             "%",        "",                      "Annual",        "",                           "",                               "" },
    { SSC_OUTPUT,        SSC_NUMBER,     "batt_bank_installed_capacity",               "Battery bank installed capacity",                       "kWh",      "",                      "Annual",        "",                           "",                               "" },
    { SSC_OUTPUT,        SSC_NUMBER,     "annual_crit_load",                           "Critical load energy (year 1)",                    "kWh",      "",                           "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_NUMBER,     "annual_crit_load_unmet",                     "Critical load energy unmet (year 1)",                    "kWh",      "",                      "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_NUMBER,     "annual_crit_load_unmet_percentage",          "Critical load unmet percentage (year 1)",                "%",        "",                      "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_NUMBER,     "annual_outage_losses_unmet",                 "Battery and system losses unmet energy (year 1)",        "kWh",      "",                      "Battery",       "",                           "",                              "" },
    { SSC_OUTPUT,        SSC_NUMBER,      "batt_year1_charge_from_system",             "Battery annual energy charged from system (year 1)",                 "kWh",      "",                      "Battery",       "",                           "",                               "" },
    { SSC_OUTPUT,        SSC_NUMBER,      "batt_year1_charge_from_grid",               "Battery annual energy charged from grid (year 1)",               "kWh",      "",                      "Battery",       "",                           "",                               "" },

    // test matrix output
    { SSC_OUTPUT,        SSC_MATRIX,     "batt_dispatch_sched",                        "Battery dispatch schedule",                              "",        "",                     "Battery",       "",                           "",                               "ROW_LABEL=MONTHS,COL_LABEL=HOURS_OF_DAY"  },


    var_info_invalid };

battstor::battstor(var_table& vt, bool setup_model, size_t nrec, double dt_hr, const std::shared_ptr<batt_variables>& batt_vars_in)
{
    make_vars = false;
    utilityRate = NULL;
    util_rate_data = NULL;
    // time quantities
    _dt_hour = dt_hr;
    step_per_hour = static_cast<size_t>(1. / _dt_hour);
    initialize_time(0, 0, 0);

    bool has_fuelcell = false;
    if (auto vd = vt.lookup("fuelcell_power")) {
        fuelcellPower = vd->arr_vector();
        has_fuelcell = true;
    }

    // battery variables
    if (batt_vars_in == 0)
    {
        make_vars = true;
        batt_vars = std::make_shared<batt_variables>();


        // fuel cell variables
        batt_vars->en_fuelcell = false;
        if (has_fuelcell) {
            batt_vars->en_fuelcell = true;
            batt_vars->batt_can_fuelcellcharge = vt.as_vector_bool("dispatch_manual_fuelcellcharge");
        }

        batt_vars->en_batt = vt.as_boolean("en_batt") || vt.as_boolean("en_standalone_batt") || vt.as_boolean("en_wave_batt");
        if (batt_vars->en_batt)
        {
            // Financial Parameters
            batt_vars->analysis_period = vt.as_integer("analysis_period");

            // Lifetime simulation
            batt_vars->system_use_lifetime_output = vt.as_boolean("system_use_lifetime_output");

            nyears = 1;
            if (batt_vars->system_use_lifetime_output) {
                nyears = batt_vars->analysis_period;
            }
            total_steps = nyears * 8760 * step_per_hour;

            // Chemistry
            batt_vars->batt_chem = vt.as_integer("batt_chem");

            // Lead acid settings
            if (batt_vars->batt_chem == battery_params::LEAD_ACID)
            {
                batt_vars->LeadAcid_q10_computed = vt.as_double("LeadAcid_q10_computed");
                batt_vars->LeadAcid_q20_computed = vt.as_double("LeadAcid_q20_computed");
                batt_vars->LeadAcid_qn_computed = vt.as_double("LeadAcid_qn_computed");
                batt_vars->LeadAcid_tn = vt.as_double("LeadAcid_tn");
            }

            // Battery bank sizing
            batt_vars->batt_computed_series = vt.as_integer("batt_computed_series");
            batt_vars->batt_computed_strings = vt.as_integer("batt_computed_strings");
            batt_vars->batt_kwh = vt.as_double("batt_computed_bank_capacity");
            batt_vars->batt_kw = vt.as_double("batt_power_discharge_max_kwdc");

            // Voltage properties
            batt_vars->batt_voltage_choice = vt.as_integer("batt_voltage_choice");
            batt_vars->batt_Vnom_default = vt.as_double("batt_Vnom_default");
            batt_vars->batt_voltage_matrix = vt.as_matrix("batt_voltage_matrix");
            batt_vars->batt_Vfull = vt.as_double("batt_Vfull");
            batt_vars->batt_Vexp = vt.as_double("batt_Vexp");
            batt_vars->batt_Vnom = vt.as_double("batt_Vnom");
            batt_vars->batt_Vcut = vt.as_double("batt_Vcut");
            batt_vars->batt_Qfull_flow = vt.as_double("batt_Qfull_flow");
            batt_vars->batt_Qfull = vt.as_double("batt_Qfull");
            batt_vars->batt_Qexp = vt.as_double("batt_Qexp");
            batt_vars->batt_Qnom = vt.as_double("batt_Qnom");
            batt_vars->batt_C_rate = vt.as_double("batt_C_rate");
            batt_vars->batt_resistance = vt.as_double("batt_resistance");


            // Current and capacity
            batt_vars->batt_current_choice = vt.as_integer("batt_current_choice");
            batt_vars->batt_current_charge_max = vt.as_double("batt_current_charge_max");
            batt_vars->batt_current_discharge_max = vt.as_double("batt_current_discharge_max");
            batt_vars->batt_power_charge_max_kwdc = vt.as_double("batt_power_charge_max_kwdc");
            batt_vars->batt_power_discharge_max_kwdc = vt.as_double("batt_power_discharge_max_kwdc");
            batt_vars->batt_power_charge_max_kwac = vt.as_double("batt_power_charge_max_kwac");
            batt_vars->batt_power_discharge_max_kwac = vt.as_double("batt_power_discharge_max_kwac");

            // Power converters and topology
            batt_vars->batt_topology = vt.as_integer("batt_ac_or_dc");
            batt_vars->batt_ac_dc_efficiency = vt.as_double("batt_ac_dc_efficiency");
            batt_vars->batt_dc_ac_efficiency = vt.as_double("batt_dc_ac_efficiency");
            batt_vars->batt_dc_dc_bms_efficiency = vt.as_double("batt_dc_dc_efficiency");

            if (vt.is_assigned("dcoptimizer_loss")) {
                batt_vars->pv_dc_dc_mppt_efficiency = 100. - vt.as_double("dcoptimizer_loss");
            }
            else {
                batt_vars->pv_dc_dc_mppt_efficiency = 100;
            }

            // Ancillary equipment losses
            batt_vars->batt_loss_choice = vt.as_integer("batt_loss_choice");
            batt_vars->batt_losses_charging = vt.as_vector_double("batt_losses_charging");
            batt_vars->batt_losses_discharging = vt.as_vector_double("batt_losses_discharging");
            batt_vars->batt_losses_idle = vt.as_vector_double("batt_losses_idle");
            batt_vars->batt_losses = vt.as_vector_double("batt_losses");

            // Charge limits and priority
            batt_vars->batt_initial_SOC = vt.as_double("batt_initial_SOC");
            batt_vars->batt_maximum_SOC = vt.as_double("batt_maximum_SOC");
            batt_vars->batt_minimum_SOC = vt.as_double("batt_minimum_SOC");
            if (vt.is_assigned("batt_minimum_outage_SOC")) {
                batt_vars->batt_minimum_outage_SOC = vt.as_double("batt_minimum_outage_SOC");
            }
            else {
                batt_vars->batt_minimum_outage_SOC = 0.0; // This is likely to happen for front of meter, which is not able to set up outages
            }
            batt_vars->batt_minimum_modetime = vt.as_double("batt_minimum_modetime");

            // Storage dispatch controllers
            batt_vars->batt_dispatch = vt.as_integer("batt_dispatch_choice");
            batt_vars->batt_dispatch_wf_forecast = vt.as_integer("batt_dispatch_wf_forecast_choice");
            batt_vars->batt_dispatch_load_forecast = vt.as_integer("batt_dispatch_load_forecast_choice");
            batt_vars->batt_meter_position = vt.as_integer("batt_meter_position");

            // Cycle cost calculations
            batt_vars->batt_cycle_cost_choice = vt.as_integer("batt_cycle_cost_choice");

            size_t cnt = 0, i = 0;
            double inflation_rate = vt.as_double("inflation_rate") * 0.01;

            // compute utility rate out-years escalation multipliers
            std::vector<ssc_number_t> cycle_cost(nyears);
            if (batt_vars->batt_cycle_cost_choice == 1)
            {
                ssc_number_t* parr = vt.as_array("batt_cycle_cost", &cnt);
                if (cnt == 1)
                {
                    for (i = 0; i < nyears; i++)
                        cycle_cost[i] = parr[0] * (ssc_number_t)pow((double)(inflation_rate + 1), (double)i);
                }
                else if (cnt < nyears)
                {
                    throw exec_error("battery", "Invalid number for batt_cycle_cost, must be 1 or equal to analysis_period.");
                }
                else
                {
                    for (i = 0; i < nyears; i++)
                        cycle_cost[i] = parr[i];
                }
            }
            batt_vars->batt_cycle_cost = cycle_cost;


            // Battery bank replacement
            if (vt.is_assigned("om_batt_replacement_cost"))
            {
                std::vector<ssc_number_t> replacement_cost(nyears);
                ssc_number_t*  parr = vt.as_array("om_batt_replacement_cost", &cnt);
                if (cnt == 1)
                {
                    double escal = 0.0;
                    if (vt.is_assigned("om_replacement_cost_escal")) {
                        escal = vt.as_double("om_replacement_cost_escal");
                    }

                    for (i = 0; i < nyears; i++)
                        replacement_cost[i] = parr[0] * (ssc_number_t)pow((double)(inflation_rate + escal + 1), (double)i);
                }
                else if (cnt < nyears)
                {
                    throw exec_error("battery", "Invalid number for om_batt_replacement_cost, must be 1 or equal to analysis_period.");
                }
                else {
                    for (i = 0; i < nyears; i++)
                        replacement_cost[i] = parr[i];
                }
                batt_vars->batt_cost_per_kwh = replacement_cost;
            }
            else
                batt_vars->batt_cost_per_kwh = std::vector<double>(nyears, 0.0);


            // battery o and m cost
            if (vt.is_assigned("om_batt_variable_cost"))
            {
                std::vector<ssc_number_t> om_cost(nyears);
                ssc_number_t* parr = vt.as_array("om_batt_variable_cost", &cnt);
                if (cnt == 1) {
                    double escal = 0.0;
                    if (vt.is_assigned("om_production_escal")) {
                        escal = vt.as_double("om_production_escal");
                    }
                    for (i = 0; i < nyears; i++)
                        om_cost[i] = 0.001 * parr[0] * (ssc_number_t)pow((double)(inflation_rate + escal + 1), (double)i); // $/MWh to $/kWh
                }
                else if (cnt < nyears)  {
                    throw exec_error("battery", "Invalid number for om_batt_variable_cost, must be 1 or equal to analysis_period.");
                }
                else {
                    for (i = 0; i < nyears; i++)
                        om_cost[i] = 0.001* parr[i]; // $/MWh to $/kWh
                }
                batt_vars->om_batt_variable_cost_per_kwh = om_cost;
            }
            else
                batt_vars->om_batt_variable_cost_per_kwh = std::vector<double>(nyears, 0.0);




            // Interconnection and curtailment
            std::vector<double> scaleFactors(batt_vars->analysis_period, 1.0); // No scaling factors for curtailment

            std::vector<double> curtailment_year_one;
            if (vt.is_assigned("grid_curtailment")) {
                curtailment_year_one = vt.as_vector_double("grid_curtailment");
                double interpolation_factor = 1.0;
                single_year_to_lifetime_interpolated<double>(
                    batt_vars->system_use_lifetime_output,
                    (size_t)batt_vars->analysis_period,
                    total_steps,
                    curtailment_year_one,
                    scaleFactors,
                    interpolation_factor,
                    batt_vars->gridCurtailmentLifetime_MW,
                    step_per_year,
                    _dt_hour);
            }

            if (vt.is_assigned("enable_interconnection_limit")) {
                batt_vars->enable_interconnection_limit = vt.as_boolean("enable_interconnection_limit");
                if (batt_vars->enable_interconnection_limit && vt.is_assigned("grid_interconnection_limit_kwac")) {
                    batt_vars->grid_interconnection_limit_kW = vt.as_double("grid_interconnection_limit_kwac");
                }
                else {
                    batt_vars->enable_interconnection_limit = false;
                }
            }
            else {
                batt_vars->enable_interconnection_limit = false;
            }

            if (!batt_vars->enable_interconnection_limit) {
                batt_vars->grid_interconnection_limit_kW = 1e+38;
            }

            // Front of meter
            if (batt_vars->batt_meter_position == dispatch_t::FRONT)
            {
                if (batt_vars->batt_dispatch == dispatch_t::FOM_PV_SMOOTHING) {
                    batt_vars->batt_dispatch_pvs_nameplate_ac = vt.as_double("batt_dispatch_pvs_nameplate_ac");
                    batt_vars->batt_dispatch_pvs_ac_lb_enable = vt.as_boolean("batt_dispatch_pvs_ac_lb_enable");
                    batt_vars->batt_dispatch_pvs_ac_lb = vt.as_double("batt_dispatch_pvs_ac_lb");
                    batt_vars->batt_dispatch_pvs_ac_ub_enable = vt.as_boolean("batt_dispatch_pvs_ac_ub_enable");
                    batt_vars->batt_dispatch_pvs_ac_ub = vt.as_double("batt_dispatch_pvs_ac_ub");
                    batt_vars->batt_dispatch_pvs_curtail_as_control = vt.as_boolean("batt_dispatch_pvs_curtail_as_control");
                    batt_vars->batt_dispatch_pvs_curtail_if_violation = vt.as_boolean("batt_dispatch_pvs_curtail_if_violation");
                    batt_vars->batt_dispatch_pvs_short_forecast_enable = vt.as_boolean("batt_dispatch_pvs_short_forecast_enable");
                    batt_vars->batt_dispatch_pvs_forecast_shift_periods = vt.as_double("batt_dispatch_pvs_forecast_shift_periods");
                    batt_vars->batt_dispatch_pvs_timestep_multiplier = vt.as_double("batt_dispatch_pvs_timestep_multiplier");
                    batt_vars->batt_dispatch_pvs_max_ramp = vt.as_double("batt_dispatch_pvs_max_ramp")/100.0;
                    batt_vars->batt_dispatch_pvs_soc_rest = vt.as_double("batt_dispatch_pvs_soc_rest")/100.0;
                    batt_vars->batt_dispatch_pvs_kp = vt.as_double("batt_dispatch_pvs_kp");
                    batt_vars->batt_dispatch_pvs_ki = vt.as_double("batt_dispatch_pvs_ki");
                    batt_vars->batt_dispatch_pvs_kf = vt.as_double("batt_dispatch_pvs_kf");
                }
                else {
                    forecast_price_signal fps(&vt);
                    bool success = fps.setup(step_per_hour);
                    if (!success) {
                        throw exec_error("battery", fps.error());
                    }
                    batt_vars->forecast_price_series_dollar_per_kwh = fps.forecast_price();
                    batt_vars->forecast_cleared_capacities_kw = fps.cleared_capacity();
                    batt_vars->capacity_forecast_type = fps.forecast_type;
                    batt_vars->cleared_capacity_percent = fps.cleared_capacity_percent;
                    outMarketPrice = vt.allocate("market_sell_rate_series_yr1", batt_vars->forecast_price_series_dollar_per_kwh.size());
                    for (i = 0; i < batt_vars->forecast_price_series_dollar_per_kwh.size(); i++) {
                        outMarketPrice[i] = (ssc_number_t)(batt_vars->forecast_price_series_dollar_per_kwh[i] * 1000.0);
                    }

                    // For automated front of meter with electricity rates
                    batt_vars->ec_rate_defined = false;
                    if (vt.is_assigned("en_electricity_rates")) { // Only defined for singleowner
                        if (vt.as_integer("en_electricity_rates"))
                        {
                            batt_vars->ec_use_realtime = vt.as_boolean("ur_en_ts_sell_rate");
                            if (!batt_vars->ec_use_realtime) {
                                batt_vars->ec_weekday_schedule = vt.as_matrix_unsigned_long("ur_ec_sched_weekday");
                                batt_vars->ec_weekend_schedule = vt.as_matrix_unsigned_long("ur_ec_sched_weekend");
                                batt_vars->ec_tou_matrix = vt.as_matrix("ur_ec_tou_mat");
                            }
                            else {
                                batt_vars->ec_realtime_buy = vt.as_vector_double("ur_ts_buy_rate");
                            }
                            batt_vars->ec_rate_defined = true;
                        }
                        else {
                            batt_vars->ec_use_realtime = true;
                            batt_vars->ec_realtime_buy = batt_vars->forecast_price_series_dollar_per_kwh;
                        }
                    }

                    if (batt_vars->batt_dispatch == dispatch_t::FOM_AUTOMATED_ECONOMIC)
                    {
                        batt_vars->batt_look_ahead_hours = vt.as_unsigned_long("batt_look_ahead_hours");
                        batt_vars->batt_dispatch_update_frequency_hours = vt.as_double("batt_dispatch_update_frequency_hours");
                    }
                    else if (batt_vars->batt_dispatch == dispatch_t::FOM_CUSTOM_DISPATCH)
                    {
                        batt_vars->batt_custom_dispatch = vt.as_vector_double("batt_custom_dispatch");
                    }
                }
            }
            // Automated behind-the-meter
            else
            {
                 // For automated behind the meter with electricity rates
                batt_vars->ec_rate_defined = false;
                if (vt.is_assigned("ur_ec_tou_mat")) { // Some tests don't have this assigned, ensure it is before setting up forecast rate
                    batt_vars->ec_rate_defined = true;
                }


                if (batt_vars->batt_dispatch == dispatch_t::MAINTAIN_TARGET)
                {
                    batt_vars->batt_target_choice = vt.as_integer("batt_target_choice");
                    batt_vars->target_power_monthly = vt.as_vector_double("batt_target_power_monthly");
                    batt_vars->target_power = vt.as_vector_double("batt_target_power");

                    if (batt_vars->batt_target_choice == dispatch_automatic_behind_the_meter_t::TARGET_SINGLE_MONTHLY)
                    {
                        target_power_monthly = batt_vars->target_power_monthly;

                        if (target_power_monthly.size() != 12) {
                            throw exec_error("battery", "batt_target_power_monthly must have 12 entries if batt_target_choice is target_single_monthly (0).");
                        }

                        target_power.clear();
                        target_power.reserve(8760 * step_per_hour);
                        for (size_t month = 0; month != 12; month++)
                        {
                            double target = target_power_monthly[month];
                            for (size_t h = 0; h != util::hours_in_month(month + 1); h++)
                            {
                                for (size_t s = 0; s != step_per_hour; s++)
                                    target_power.push_back(target);
                            }
                        }

                    }
                    else
                        target_power = batt_vars->target_power;

                    if (target_power.size() != nrec)
                        throw exec_error("battery", "Length of target_power must equal number of records in weather file.");

                    // extend target power to lifetime internally
                    for (size_t y = 1; y < nyears; y++) {
                        for (i = 0; i < nrec; i++) {
                            target_power.push_back(target_power[i]);
                        }
                    }
                    batt_vars->target_power = target_power;

                }
                else if (batt_vars->batt_dispatch == dispatch_t::SELF_CONSUMPTION)
                {
                    //for self-consumption (aka 24/7 carbon-free energy) dispatch option, we're going to use the grid target power algorithm with an array of zeros
                    std::vector<double> target_power_temp(nyears* nrec, 0.0);
                    target_power = target_power_temp;
                }
                else if (batt_vars->batt_dispatch == dispatch_t::CUSTOM_DISPATCH)
                {
                    batt_vars->batt_custom_dispatch = vt.as_vector_double("batt_custom_dispatch");
                }
            }

            // Manual dispatch
            if ((batt_vars->batt_meter_position == dispatch_t::FRONT && batt_vars->batt_dispatch == dispatch_t::FOM_MANUAL) ||
                (batt_vars->batt_meter_position == dispatch_t::BEHIND && batt_vars->batt_dispatch == dispatch_t::MANUAL))
            {
                batt_vars->batt_can_charge = vt.as_vector_bool("dispatch_manual_charge");
                batt_vars->batt_can_discharge = vt.as_vector_bool("dispatch_manual_discharge");
                batt_vars->batt_can_gridcharge = vt.as_vector_bool("dispatch_manual_gridcharge");
                batt_vars->batt_discharge_percent = vt.as_vector_double("dispatch_manual_percent_discharge");
                batt_vars->batt_gridcharge_percent = vt.as_vector_double("dispatch_manual_percent_gridcharge");
                batt_vars->batt_discharge_schedule_weekday = vt.as_matrix_unsigned_long("dispatch_manual_sched");
                batt_vars->batt_discharge_schedule_weekend = vt.as_matrix_unsigned_long("dispatch_manual_sched_weekend");

                if (batt_vars->batt_meter_position == dispatch_t::BEHIND) {
                    batt_vars->batt_btm_can_discharge_to_grid = vt.as_vector_bool("dispatch_manual_btm_discharge_to_grid");
                }
            }

            // Common to automated methods
            batt_vars->batt_dispatch_auto_can_charge = true;
            batt_vars->batt_dispatch_auto_can_clipcharge = false;
            batt_vars->batt_dispatch_auto_can_curtailcharge = false;
            batt_vars->batt_dispatch_auto_can_gridcharge = false;
            batt_vars->batt_dispatch_auto_can_fuelcellcharge = true;
            batt_vars->batt_dispatch_auto_btm_can_discharge_to_grid = false;

            if (vt.is_assigned("batt_dispatch_auto_can_gridcharge")) {
                batt_vars->batt_dispatch_auto_can_gridcharge = vt.as_boolean("batt_dispatch_auto_can_gridcharge");
            }
            if (vt.is_assigned("batt_dispatch_auto_can_charge")) {
                batt_vars->batt_dispatch_auto_can_charge = vt.as_boolean("batt_dispatch_auto_can_charge");
            }
            if (vt.is_assigned("batt_dispatch_auto_can_clipcharge")) {
                batt_vars->batt_dispatch_auto_can_clipcharge = vt.as_boolean("batt_dispatch_auto_can_clipcharge");
            }
            if (vt.is_assigned("batt_dispatch_auto_can_curtailcharge")) {
                batt_vars->batt_dispatch_auto_can_curtailcharge = vt.as_boolean("batt_dispatch_auto_can_curtailcharge");
            }
            if (vt.is_assigned("batt_dispatch_auto_can_fuelcellcharge")) {
                batt_vars->batt_dispatch_auto_can_fuelcellcharge = vt.as_boolean("batt_dispatch_auto_can_fuelcellcharge");
            }
            if (vt.is_assigned("batt_dispatch_auto_btm_can_discharge_to_grid")) {
                batt_vars->batt_dispatch_auto_btm_can_discharge_to_grid = vt.as_boolean("batt_dispatch_auto_btm_can_discharge_to_grid");
            }

            // Control powerflow for all BTM methods
            batt_vars->batt_dispatch_charge_only_system_exceeds_load = true;
            batt_vars->batt_dispatch_discharge_only_load_exceeds_system = true;
            batt_vars->batt_dispatch_batt_system_charge_first = false;

            if (vt.is_assigned("batt_dispatch_charge_only_system_exceeds_load")) {
                batt_vars->batt_dispatch_charge_only_system_exceeds_load = vt.as_boolean("batt_dispatch_charge_only_system_exceeds_load");
            }
            if (vt.is_assigned("batt_dispatch_discharge_only_load_exceeds_system")) {
                batt_vars->batt_dispatch_discharge_only_load_exceeds_system = vt.as_boolean("batt_dispatch_discharge_only_load_exceeds_system");
            }
            if (vt.is_assigned("dispatch_manual_system_charge_first")) {
                batt_vars->batt_dispatch_batt_system_charge_first = vt.as_boolean("dispatch_manual_system_charge_first");
            }

            batt_vars->batt_replacement_option = vt.as_integer("batt_replacement_option");
            batt_vars->batt_replacement_capacity = vt.as_double("batt_replacement_capacity");

            if (batt_vars->batt_replacement_option == replacement_params::SCHEDULE) {
                batt_vars->batt_replacement_schedule_percent = vt.as_vector_double("batt_replacement_schedule_percent");
            }

            // Battery lifetime
            batt_vars->batt_life_model = vt.as_integer("batt_life_model");

            if (batt_vars->batt_life_model == lifetime_params::NMC && batt_vars->batt_chem != battery_params::LITHIUM_ION)
                throw exec_error("battery", "NMC life model (batt_life_model=1) can only be used with Li-Ion chemistries (batt_chem=1).");

            if (batt_vars->batt_life_model == lifetime_params::LMOLTO && batt_vars->batt_chem != battery_params::LITHIUM_ION)
                throw exec_error("battery", "LMO/LTO life model (batt_life_model=2) can only be used with Li-Ion chemistries (batt_chem=1).");

            if (batt_vars->batt_life_model == lifetime_params::MODEL_CHOICE::CALCYC) {
                batt_vars->batt_calendar_choice = vt.as_integer("batt_calendar_choice");
                batt_vars->batt_lifetime_matrix = vt.as_matrix("batt_lifetime_matrix");
                batt_vars->batt_calendar_lifetime_matrix = vt.as_matrix("batt_calendar_lifetime_matrix");
                batt_vars->batt_calendar_q0 = vt.as_double("batt_calendar_q0");
                batt_vars->batt_calendar_a = vt.as_double("batt_calendar_a");
                batt_vars->batt_calendar_b = vt.as_double("batt_calendar_b");
                batt_vars->batt_calendar_c = vt.as_double("batt_calendar_c");
            }

            // Thermal behavior
            batt_vars->batt_surface_area = vt.as_double("batt_surface_area");
            batt_vars->cap_vs_temp = vt.as_matrix("cap_vs_temp");
            batt_vars->batt_mass = vt.as_double("batt_mass");
            batt_vars->batt_Cp = vt.as_double("batt_Cp");
            batt_vars->batt_h_to_ambient = vt.as_double("batt_h_to_ambient");
            batt_vars->T_room = vt.as_vector_double("batt_room_temperature_celsius");

            // If only one variable was specified, use a fixed ambient temperature
            if (batt_vars->T_room.size() == 1) {
                double T_ambient = batt_vars->T_room[0];
                batt_vars->T_room = std::vector<double>(nrec, T_ambient);
            }

            // Inverter settings
            batt_vars->inverter_model = vt.as_integer("inverter_model");
            if (batt_vars->inverter_model < 4) //user has assigned an actual inverter model
            {
                batt_vars->inverter_count = vt.as_integer("inverter_count");
                batt_vars->batt_inverter_efficiency_cutoff = vt.as_double("batt_inverter_efficiency_cutoff");

                if (batt_vars->inverter_model == SharedInverter::SANDIA_INVERTER)
                {
                    batt_vars->inverter_efficiency = vt.as_double("inv_snl_eff_cec");
                    batt_vars->inverter_paco = batt_vars->inverter_count * vt.as_double("inv_snl_paco") * util::watt_to_kilowatt;
                }
                else if (batt_vars->inverter_model == SharedInverter::DATASHEET_INVERTER)
                {
                    batt_vars->inverter_efficiency = vt.as_double("inv_ds_eff");
                    batt_vars->inverter_paco = batt_vars->inverter_count * vt.as_double("inv_ds_paco") * util::watt_to_kilowatt;

                }
                else if (batt_vars->inverter_model == SharedInverter::PARTLOAD_INVERTER)
                {
                    batt_vars->inverter_efficiency = vt.as_double("inv_pd_eff");
                    batt_vars->inverter_paco = batt_vars->inverter_count * vt.as_double("inv_pd_paco") * util::watt_to_kilowatt;
                }
                else if (batt_vars->inverter_model == SharedInverter::COEFFICIENT_GENERATOR)
                {
                    batt_vars->inverter_efficiency = vt.as_double("inv_cec_cg_eff_cec");
                    batt_vars->inverter_paco = batt_vars->inverter_count * vt.as_double("inv_cec_cg_paco") * util::watt_to_kilowatt;
                }
            }
            else //use the default "none" inverter model, which is the default behavior of this input
            {
                batt_vars->inverter_model = SharedInverter::NONE;
                batt_vars->inverter_count = 1;
                batt_vars->inverter_efficiency = batt_vars->batt_ac_dc_efficiency;
                batt_vars->inverter_paco = batt_vars->batt_kw;
            }
        }
    }
    else {
        nyears = (batt_vars_in->system_use_lifetime_output) ? batt_vars_in->analysis_period : 1;
        total_steps = nyears * 8760 * step_per_hour;
        batt_vars = batt_vars_in;
    }

    // Check to see if the outage variables need to be set up
    analyze_outage = false;
    if (vt.is_assigned("grid_outage")) {
        batt_vars->grid_outage_steps = vt.as_vector_bool("grid_outage"); // All lines that check for this check for length and default to false, so no exception should be ok here.
        // If not all false, we need the outage vars
        analyze_outage = std::any_of(batt_vars->grid_outage_steps.begin(), batt_vars->grid_outage_steps.end(), [](bool x) {return x; });

        // If outage is all zeroes we don't care about the length, make sure if an outage is specified that it matches the weather file
        if (analyze_outage && batt_vars->grid_outage_steps.size() < step_per_hour * 8760) {
            throw exec_error("Battery", "Length of grid_outage must equal number of steps in one year of simulation.");
        }
    }

    // component models
    battery_model = 0;
    dispatch_model = 0;
    charge_control = 0;
    battery_metrics = 0;

    // outputs
    outTotalCharge = 0;
    outAvailableCharge = 0;
    outBoundCharge = 0;
    outMaxChargeAtCurrent = 0;
    outMaxChargeThermal = 0;
    outMaxCharge = 0;
    outSOC = 0;
    outDOD = 0;
    outDODCycleAverage = 0;
    outCurrent = 0;
    outCellVoltage = 0;
    outBatteryVoltage = 0;
    outCapacityPercent = 0;
    outCycles = 0;
    outBatteryBankReplacement = 0;
    outBatteryTemperature = 0;
    outCapacityThermalPercent = 0;
    outBatteryPowerAC = 0;
    outBatteryPowerDC = 0;
    outGridPower = 0;
    outSystemToLoad = 0;
    outBatteryToLoad = 0;
    outGridToLoad = 0;
    outFuelCellToLoad = 0;
    outGridPowerTarget = 0;
    outSystemToBattAC = 0;
    outSystemToBattDC = 0;
    outGridToBatt = 0;
    outFuelCellToBatt = 0;
    outSystemToGrid = 0;
    outBatteryToGrid = 0;
    outBatteryToSystemLoad = 0;
    outBatteryToInverterDC = 0;
    outFuelCellToGrid = 0;
    outBatteryConversionPowerLoss = 0;
    outBatterySystemLoss = 0;
    outInterconnectionLoss = 0;
    outCritLoad = 0;
    outCritLoadUnmet = 0;
    outUnmetLosses = 0;
    outAverageCycleEfficiency = 0;
    outSystemChargePercent = 0;
    outGridChargePercent = 0;
    outAnnualSystemChargeEnergy = 0;
    outAnnualGridChargeEnergy = 0;
    outAnnualChargeEnergy = 0;
    outAnnualDischargeEnergy = 0;
    outAnnualGridImportEnergy = 0;
    outAnnualGridExportEnergy = 0;
    outCostToCycle = 0;
    outBenefitCharge = 0;
    outBenefitGridcharge = 0;
    outBenefitClipcharge = 0;
    outBenefitDischarge = 0;
    outPVS_outpower = 0;
    outPVS_battpower = 0;
    outPVS_battsoc = 0;
    outPVS_curtail = 0;
    outPVS_violation_list = 0;
    outPVS_P_pv_ac = 0;
    outPVS_PV_ramp_interval = 0;
    outPVS_forecast_pv_energy = 0;


    en = setup_model;
    if (!en) return;

    if (!batt_vars->system_use_lifetime_output) {
        if (batt_vars->batt_replacement_option > 0)
            throw exec_error("battery", "Battery replacements are enabled with single year simulation. You must enable lifetime simulations (system_use_lifetime_output = 1) to model battery replacements.");
    }
    chem = batt_vars->batt_chem;


    /* **********************************************************************
    Initialize outputs
    ********************************************************************** */

    // only allocate if lead-acid
    if (chem == 0)
    {
        outAvailableCharge = vt.allocate("batt_q1", nrec * nyears);
        outBoundCharge = vt.allocate("batt_q2", nrec * nyears);
    }
    outCellVoltage = vt.allocate("batt_voltage_cell", nrec * nyears);
    outMaxCharge = vt.allocate("batt_qmax", nrec * nyears);
    outMaxChargeThermal = vt.allocate("batt_qmax_thermal", nrec * nyears);
    outBatteryTemperature = vt.allocate("batt_temperature", nrec * nyears);
    outCapacityThermalPercent = vt.allocate("batt_capacity_thermal_percent", nrec * nyears);

    outCurrent = vt.allocate("batt_I", nrec * nyears);
    outBatteryVoltage = vt.allocate("batt_voltage", nrec * nyears);
    outTotalCharge = vt.allocate("batt_q0", nrec * nyears);
    outCycles = vt.allocate("batt_cycles", nrec * nyears);
    outSOC = vt.allocate("batt_SOC", nrec * nyears);
    outDOD = vt.allocate("batt_DOD", nrec * nyears);
    outDODCycleAverage = vt.allocate("batt_DOD_cycle_average", nrec * nyears);
    outCapacityPercent = vt.allocate("batt_capacity_percent", nrec * nyears);
    if (batt_vars->batt_life_model == lifetime_params::CALCYC || batt_vars->batt_life_model == lifetime_params::LMOLTO) {
        outCapacityPercentCycle = vt.allocate("batt_capacity_percent_cycle", nrec * nyears);
        outCapacityPercentCalendar = vt.allocate("batt_capacity_percent_calendar", nrec * nyears);
    }
    outBatteryPowerAC = vt.allocate("batt_power", nrec * nyears);
    outBatteryPowerDC = vt.allocate("batt_power_dc", nrec * nyears);
    outGridPower = vt.allocate("grid_power", nrec * nyears); // Net grid energy required.  Positive indicates putting energy on grid.  Negative indicates pulling off grid
    outGenPower = vt.allocate("pv_batt_gen", nrec * nyears);
    outGenWithoutBattery = vt.allocate("gen_without_battery", nrec * nyears);
    outSystemToGrid = vt.allocate("system_to_grid", nrec * nyears);
    outBatteryToSystemLoad = vt.allocate("batt_to_system_load", nrec * nyears);
    outBatteryToGrid = vt.allocate("batt_to_grid", nrec * nyears);
    outBatteryToInverterDC = vt.allocate("batt_to_inverter_dc", nrec * nyears);
    outAdjustLosses = vt.allocate("batt_availability_loss", nrec * nyears);

    if (batt_vars->batt_meter_position == dispatch_t::BEHIND)
    {
        outSystemToLoad = vt.allocate("system_to_load", nrec * nyears);
        outBatteryToLoad = vt.allocate("batt_to_load", nrec * nyears);
        outGridToLoad = vt.allocate("grid_to_load", nrec * nyears);

        if (batt_vars->batt_dispatch != dispatch_t::MANUAL)
        {
            outGridPowerTarget = vt.allocate("grid_power_target", nrec * nyears);
            outBattPowerTarget = vt.allocate("batt_power_target", nrec * nyears);
        }
        else {
            outDispatchPeriod = vt.allocate("batt_dispatch_period", nrec * nyears);
        }
    }
    else if (batt_vars->batt_meter_position == dispatch_t::FRONT)
    {
        if (batt_vars->batt_dispatch == dispatch_t::FOM_PV_SMOOTHING) {
            outPVS_outpower = vt.allocate("batt_pvs_outpower", nrec * nyears);
            outPVS_battpower = vt.allocate("batt_pvs_battpower", nrec * nyears);
            outPVS_battsoc = vt.allocate("batt_pvs_battsoc", nrec * nyears);
            outPVS_curtail = vt.allocate("batt_pvs_curtail", nrec * nyears);
            outPVS_violation_list = vt.allocate("batt_pvs_violation_list", nrec * nyears);
            outPVS_P_pv_ac = vt.allocate("batt_pvs_P_pv_ac", nrec * nyears);
            outPVS_PV_ramp_interval = vt.allocate("batt_pvs_PV_ramp_interval", nrec * nyears);
            outPVS_forecast_pv_energy = vt.allocate("batt_pvs_forecast_pv_energy", nrec * nyears);
        }
        else  if (batt_vars->batt_dispatch != dispatch_t::FOM_MANUAL) {
            outBattPowerTarget = vt.allocate("batt_power_target", nrec * nyears);
            outBenefitCharge = vt.allocate("batt_revenue_charge", nrec * nyears);
            outBenefitGridcharge = vt.allocate("batt_revenue_gridcharge", nrec * nyears);
            outBenefitClipcharge = vt.allocate("batt_revenue_clipcharge", nrec * nyears);
            outBenefitDischarge = vt.allocate("batt_revenue_discharge", nrec * nyears);
        }
        else {
            outDispatchPeriod = vt.allocate("batt_dispatch_period", nrec * nyears);
        }
    }
    outSystemToBattAC = vt.allocate("system_to_batt", nrec * nyears);
    outSystemToBattDC = vt.allocate("system_to_batt_dc", nrec * nyears);
    outGridToBatt = vt.allocate("grid_to_batt", nrec * nyears);

    if (batt_vars->en_fuelcell) {
        outFuelCellToBatt = vt.allocate("fuelcell_to_batt", nrec * nyears);
        outFuelCellToGrid = vt.allocate("fuelcell_to_grid", nrec * nyears);
        outFuelCellToLoad = vt.allocate("fuelcell_to_load", nrec * nyears);

    }

    bool cycleCostRelevant = (batt_vars->batt_meter_position == dispatch_t::BEHIND && batt_vars->batt_dispatch == dispatch_t::RETAIL_RATE) ||
        (batt_vars->batt_meter_position == dispatch_t::FRONT && (batt_vars->batt_dispatch != dispatch_t::FOM_MANUAL && batt_vars->batt_dispatch != dispatch_t::FOM_CUSTOM_DISPATCH));
    if (cycleCostRelevant && batt_vars->batt_cycle_cost_choice == dispatch_t::MODEL_CYCLE_COST) {
        outCostToCycle = vt.allocate("batt_cost_to_cycle", nrec * nyears);
    }

    outBatteryConversionPowerLoss = vt.allocate("batt_conversion_loss", nrec * nyears);
    outBatterySystemLoss = vt.allocate("batt_system_loss", nrec * nyears);
    outInterconnectionLoss = vt.allocate("interconnection_loss", nrec * nyears);

    if (analyze_outage) {
        outCritLoadUnmet = vt.allocate("crit_load_unmet", nrec * nyears);
        outCritLoad = vt.allocate("crit_load", nrec * nyears);
        outUnmetLosses = vt.allocate("outage_losses_unmet", nrec * nyears);
    }

    // annual outputs
    outBatteryBankReplacement = vt.allocate("batt_bank_replacement", nyears);
    outAnnualChargeEnergy = vt.allocate("batt_annual_charge_energy", nyears);
    outAnnualDischargeEnergy = vt.allocate("batt_annual_discharge_energy", nyears);
    outAnnualGridImportEnergy = vt.allocate("annual_import_to_grid_energy", nyears);
    outAnnualGridExportEnergy = vt.allocate("annual_export_to_grid_energy", nyears);
    outAnnualEnergySystemLoss = vt.allocate("batt_annual_energy_system_loss", nyears);
    outAnnualEnergyLoss = vt.allocate("batt_annual_energy_loss", nyears);
    outAnnualSystemChargeEnergy = vt.allocate("batt_annual_charge_from_system", nyears);
    outAnnualGridChargeEnergy = vt.allocate("batt_annual_charge_from_grid", nyears);

    // model initialization
    voltage_t* voltage_model = 0;
    lifetime_t* lifetime_model = 0;
    thermal_t* thermal_model = 0;
    capacity_t* capacity_model = 0;
    losses_t* losses_model = 0;

    if ((chem == battery_params::LEAD_ACID || chem == battery_params::LITHIUM_ION) && batt_vars->batt_voltage_choice == voltage_params::MODEL) {
        voltage_model = new voltage_dynamic_t(batt_vars->batt_computed_series, batt_vars->batt_computed_strings,
            batt_vars->batt_Vnom_default, batt_vars->batt_Vfull, batt_vars->batt_Vexp,
            batt_vars->batt_Vnom, batt_vars->batt_Qfull, batt_vars->batt_Qexp,
            batt_vars->batt_Qnom, batt_vars->batt_Vcut, batt_vars->batt_C_rate, batt_vars->batt_resistance,
            dt_hr);
    }
    else if ((chem == battery_params::VANADIUM_REDOX) && batt_vars->batt_voltage_choice == voltage_params::MODEL)
        voltage_model = new voltage_vanadium_redox_t(batt_vars->batt_computed_series, batt_vars->batt_computed_strings,
            batt_vars->batt_Vnom_default, batt_vars->batt_resistance,
            dt_hr);
    else
        voltage_model = new voltage_table_t(batt_vars->batt_computed_series, batt_vars->batt_computed_strings,
            batt_vars->batt_Vnom_default,
            batt_vars->batt_voltage_matrix, batt_vars->batt_resistance,
            dt_hr);

    if (batt_vars->batt_life_model == lifetime_params::CALCYC) {
        if (batt_vars->batt_calendar_choice == calendar_cycle_params::CALENDAR_CHOICE::MODEL) {
            lifetime_model = new lifetime_calendar_cycle_t(batt_vars->batt_lifetime_matrix, dt_hr,
                                                           batt_vars->batt_calendar_q0, batt_vars->batt_calendar_a, batt_vars->batt_calendar_b, batt_vars->batt_calendar_c);
        }
        else if (batt_vars->batt_calendar_choice == calendar_cycle_params::CALENDAR_CHOICE::TABLE) {
            lifetime_model = new lifetime_calendar_cycle_t(batt_vars->batt_lifetime_matrix, dt_hr, batt_vars->batt_calendar_lifetime_matrix);
        }
        else {
            lifetime_model = new lifetime_calendar_cycle_t(batt_vars->batt_lifetime_matrix, dt_hr);
        }
    }
    else if (batt_vars->batt_life_model == lifetime_params::NMC) {
        lifetime_model = new lifetime_nmc_t(dt_hr);
    }
    else if (batt_vars->batt_life_model == lifetime_params::LMOLTO) {
        lifetime_model = new lifetime_lmolto_t(dt_hr);
    }
    else {
        throw exec_error("battery", "Unrecognized batt_life_model option. Valid options are 0 for separate calendar and cycle models, "
                                    "1 for NMC (Smith 2017) life model, 2 for LMO/LTO life model.");
    }

    if (batt_vars->T_room.size() != nrec) {
        throw exec_error("battery", util::format("Length of battery environment temperature batt_room_temperature_celsius must equal number of weather file and/or electric load data records: %d records or %d-minute time steps.", nrec, 60/(nrec/8760)));
    }

    if (batt_vars->batt_life_model == lifetime_params::NMC) {
        thermal_model = new thermal_t(
            dt_hr,
            batt_vars->batt_mass, // [kg]
            batt_vars->batt_surface_area, // [m]
            batt_vars->batt_resistance, // [J/kgK]
            batt_vars->batt_Cp,
            batt_vars->batt_h_to_ambient,
            batt_vars->T_room
        );
    }
    else {
        thermal_model = new thermal_t(
            dt_hr,
            batt_vars->batt_mass, // [kg]
            batt_vars->batt_surface_area, // [m]
            batt_vars->batt_resistance, // [J/kgK]
            batt_vars->batt_Cp,
            batt_vars->batt_h_to_ambient,
            batt_vars->cap_vs_temp,
            batt_vars->T_room
        );
    }


    if (chem == battery_params::LEAD_ACID)
    {
        capacity_model = new capacity_kibam_t(
            batt_vars->LeadAcid_q20_computed,
            batt_vars->LeadAcid_tn,
            batt_vars->LeadAcid_qn_computed,
            batt_vars->LeadAcid_q10_computed,
            batt_vars->batt_initial_SOC,
            batt_vars->batt_maximum_SOC,
            batt_vars->batt_minimum_SOC,
            dt_hr);
    }
    else if (chem == battery_params::LITHIUM_ION)
    {
        capacity_model = new capacity_lithium_ion_t(
            batt_vars->batt_Qfull * batt_vars->batt_computed_strings, batt_vars->batt_initial_SOC, batt_vars->batt_maximum_SOC, batt_vars->batt_minimum_SOC, dt_hr);
    }
    // for now assume Flow Batteries responds quickly, like Lithium-ion, but with an independent capacity/power
    else if (chem == battery_params::VANADIUM_REDOX || chem == battery_params::IRON_FLOW)
    {
        capacity_model = new capacity_lithium_ion_t(
            batt_vars->batt_Qfull_flow, batt_vars->batt_initial_SOC, batt_vars->batt_maximum_SOC, batt_vars->batt_minimum_SOC, dt_hr);
    }

    std::vector<double> adj_losses(1, 0.0);
    if (vt.is_assigned("batt_adjust_constant") || vt.is_assigned("batt_adjust_periods") || vt.is_assigned("batt_adjust_timeindex")) {
        adj_losses.clear();
        adjustment_factors haf(&vt, "batt_adjust");
        if (!haf.setup(nrec, nyears)) {
            throw exec_error("battery", "failed to setup battery adjustment factors: " + haf.error());
        }
        for (size_t i = 0; i < haf.size(); i++) {
            adj_losses.push_back(1.0 - haf(i)); // Convert to convention within powerflow and capacity code
        }
    }

    if (batt_vars->batt_loss_choice == losses_params::MONTHLY) {
        if (*std::min_element(batt_vars->batt_losses_charging.begin(), batt_vars->batt_losses_charging.end()) < 0
            || *std::min_element(batt_vars->batt_losses_discharging.begin(), batt_vars->batt_losses_discharging.end()) < 0
            || *std::min_element(batt_vars->batt_losses_idle.begin(), batt_vars->batt_losses_idle.end()) < 0) {
            throw exec_error("battery", "Battery loss inputs batt_losses_charging, batt_losses_discharging, and batt_losses_idle cannot include negative numbers.");
        }
        losses_model = new losses_t(batt_vars->batt_losses_charging, batt_vars->batt_losses_discharging, batt_vars->batt_losses_idle, adj_losses);
    }
    else if (batt_vars->batt_loss_choice == losses_params::SCHEDULE) {
        if (!(batt_vars->batt_losses.size() == 1 || batt_vars->batt_losses.size() == nrec)) {
            throw exec_error("battery", "Length of battery loss input batt_losses must be 1 or equal to weather file length for time series input mode.");
        }
        if (*std::min_element(batt_vars->batt_losses.begin(), batt_vars->batt_losses.end()) < 0) {
            throw exec_error("battery", "Battery loss input batt_losses cannot include negative numbers.");
        }
        losses_model = new losses_t(batt_vars->batt_losses, adj_losses);
    }
    else {
        losses_model = new losses_t();
    }

    battery_model = new battery_t(dt_hr, chem, capacity_model, voltage_model, lifetime_model, thermal_model, losses_model);

    if (batt_vars->batt_replacement_option == replacement_params::SCHEDULE) {
        battery_model->setupReplacements(batt_vars->batt_replacement_schedule_percent);
    }
    else if (batt_vars->batt_replacement_option == replacement_params::CAPACITY_PERCENT) {
        battery_model->setupReplacements(batt_vars->batt_replacement_capacity);
    }

    battery_metrics = new battery_metrics_t(dt_hr);

    /*! Process the dispatch options and create the appropriate model */
    if ((batt_vars->batt_meter_position == dispatch_t::BEHIND && batt_vars->batt_dispatch == dispatch_t::MANUAL) ||
        (batt_vars->batt_meter_position == dispatch_t::FRONT && batt_vars->batt_dispatch == dispatch_t::FOM_MANUAL))
    {
        /*! Generic manual dispatch model inputs */
        if (batt_vars->batt_can_charge.size() != 6 || batt_vars->batt_can_discharge.size() != 6 || batt_vars->batt_can_gridcharge.size() != 6)
            throw exec_error("battery", "Invalid manual dispatch control vector length, must be length 6.");

        if (batt_vars->batt_discharge_schedule_weekday.nrows() != 12 || batt_vars->batt_discharge_schedule_weekday.ncols() != 24)
            throw exec_error("battery", "Invalid manual dispatch schedule matrix batt_discharge_schedule dimensions, must be 12 x 24.");

        size_t max_period = 6;
        size_t* discharge_schedule_vec = batt_vars->batt_discharge_schedule_weekday.data();
        size_t* period_num = std::find_if(discharge_schedule_vec, discharge_schedule_vec + batt_vars->batt_discharge_schedule_weekday.ncells() - 1, [max_period](size_t element) { return (max_period < element); });
        if (*period_num > max_period)
            throw exec_error("battery", "Invalid manual dispatch period in weekday schedule. Period numbers must be less than or equal to 6.");

        if (batt_vars->batt_discharge_schedule_weekend.nrows() != 12 || batt_vars->batt_discharge_schedule_weekend.ncols() != 24)
            throw exec_error("battery", "Invalid weekend manual dispatch schedule matrix dimensions, must be 12 x 24.");

        discharge_schedule_vec = batt_vars->batt_discharge_schedule_weekend.data();
        period_num = std::find_if(discharge_schedule_vec, discharge_schedule_vec + batt_vars->batt_discharge_schedule_weekend.ncells() - 1, [max_period](size_t element) { return (max_period < element); });
        if (*period_num > max_period)
            throw exec_error("battery", "Invalid manual dispatch period in weekend schedule. Period numbers must be less than or equal to 6.");

        if (batt_vars->en_fuelcell) {
            if (batt_vars->batt_can_fuelcellcharge.size() != 6)
                throw exec_error("fuelcell", "Invalid manual dispatch control vector lengths, must be length 6.");
        }

        if (batt_vars->batt_meter_position == dispatch_t::BEHIND) {
            if (batt_vars->batt_btm_can_discharge_to_grid.size() != 6) {
                throw exec_error("Battery", "Invalid manual dispatch control vector lenghts, must be length 6.");
            }

            if (batt_vars->batt_dispatch_batt_system_charge_first && batt_vars->batt_dispatch_charge_only_system_exceeds_load) {
                throw exec_error("Battery", "Invalid charging priority choices. Only enable dispatch_manual_system_charge_first if batt_dispatch_charge_only_system_exceeds_load is false.");
            }
        }


        size_t discharge_index = 0;
        size_t gridcharge_index = 0;
        for (size_t i = 0; i < batt_vars->batt_can_discharge.size(); i++)
        {
            if (batt_vars->batt_can_discharge[i])
                dm_percent_discharge[i + 1] = batt_vars->batt_discharge_percent[discharge_index++];

            if (batt_vars->batt_can_gridcharge[i])
                dm_percent_gridcharge[i + 1] = batt_vars->batt_gridcharge_percent[gridcharge_index++];
        }
        // Manual Dispatch Model
        if ((batt_vars->batt_meter_position == dispatch_t::BEHIND && batt_vars->batt_dispatch == dispatch_t::MANUAL) ||
            (batt_vars->batt_meter_position == dispatch_t::FRONT && batt_vars->batt_dispatch == dispatch_t::FOM_MANUAL))
        {
            dispatch_model = new dispatch_manual_t(battery_model, dt_hr, batt_vars->batt_minimum_SOC, batt_vars->batt_maximum_SOC,
                batt_vars->batt_current_choice,
                batt_vars->batt_current_charge_max, batt_vars->batt_current_discharge_max,
                batt_vars->batt_power_charge_max_kwdc, batt_vars->batt_power_discharge_max_kwdc,
                batt_vars->batt_power_charge_max_kwac, batt_vars->batt_power_discharge_max_kwac,
                batt_vars->batt_minimum_modetime,
                batt_vars->batt_dispatch, batt_vars->batt_meter_position,
                batt_vars->batt_discharge_schedule_weekday, batt_vars->batt_discharge_schedule_weekend,
                batt_vars->batt_can_charge, batt_vars->batt_can_discharge, batt_vars->batt_can_gridcharge, batt_vars->batt_can_fuelcellcharge, batt_vars->batt_btm_can_discharge_to_grid,
                dm_percent_discharge, dm_percent_gridcharge, batt_vars->batt_dispatch_auto_can_clipcharge, batt_vars->batt_dispatch_auto_can_curtailcharge, batt_vars->grid_interconnection_limit_kW,
                batt_vars->batt_dispatch_charge_only_system_exceeds_load,
                batt_vars->batt_dispatch_discharge_only_load_exceeds_system,
                batt_vars->batt_minimum_outage_SOC,
                batt_vars->batt_dispatch_batt_system_charge_first);
        }
    }
    /*! Front of meter automated DC-connected dispatch */
    else if (batt_vars->batt_meter_position == dispatch_t::FRONT)
    {
        double eta_discharge = batt_vars->batt_dc_dc_bms_efficiency * 0.01 * batt_vars->inverter_efficiency;
        double eta_pvcharge = batt_vars->batt_dc_dc_bms_efficiency;
        double eta_gridcharge = batt_vars->batt_dc_dc_bms_efficiency * 0.01 * batt_vars->inverter_efficiency;
        if (batt_vars->batt_topology == ChargeController::AC_CONNECTED) {
            eta_discharge = batt_vars->batt_dc_ac_efficiency;
            eta_pvcharge = batt_vars->batt_ac_dc_efficiency;
            eta_gridcharge = batt_vars->batt_ac_dc_efficiency;
        }

        // Create UtilityRate object only if utility rate is defined
        if (batt_vars->ec_rate_defined) {
            utilityRate = new UtilityRate(batt_vars->ec_use_realtime, batt_vars->ec_weekday_schedule, batt_vars->ec_weekend_schedule, batt_vars->ec_tou_matrix, batt_vars->ec_realtime_buy);
        }

        // PV Smoothing dispatch model
        if (batt_vars->batt_dispatch == dispatch_t::FOM_PV_SMOOTHING) {
            dispatch_model = new dispatch_pvsmoothing_front_of_meter_t(battery_model, dt_hr, batt_vars->batt_minimum_SOC, batt_vars->batt_maximum_SOC,
                batt_vars->batt_current_choice, batt_vars->batt_current_charge_max, batt_vars->batt_current_discharge_max,
                batt_vars->batt_power_charge_max_kwdc, batt_vars->batt_power_discharge_max_kwdc,
                batt_vars->batt_power_charge_max_kwac, batt_vars->batt_power_discharge_max_kwac,
                batt_vars->batt_minimum_modetime,
                batt_vars->batt_dispatch, batt_vars->batt_dispatch_wf_forecast, batt_vars->batt_meter_position,
                nyears, batt_vars->batt_look_ahead_hours, batt_vars->batt_dispatch_update_frequency_hours,
                batt_vars->batt_dispatch_auto_can_charge, batt_vars->batt_dispatch_auto_can_clipcharge, batt_vars->batt_dispatch_auto_can_gridcharge,
                batt_vars->batt_dispatch_auto_can_fuelcellcharge, batt_vars->batt_dispatch_auto_can_curtailcharge,
                batt_vars->inverter_paco, batt_vars->batt_cost_per_kwh,
                batt_vars->batt_cycle_cost_choice, batt_vars->batt_cycle_cost, batt_vars->om_batt_variable_cost_per_kwh,
                eta_pvcharge, eta_gridcharge, eta_discharge, batt_vars->batt_dispatch_pvs_nameplate_ac,
                batt_vars->batt_dispatch_pvs_ac_lb, batt_vars->batt_dispatch_pvs_ac_lb_enable, batt_vars->batt_dispatch_pvs_ac_ub,
                batt_vars->batt_dispatch_pvs_ac_ub_enable, batt_vars->batt_dispatch_pvs_curtail_as_control, batt_vars->batt_dispatch_pvs_curtail_if_violation,
                batt_vars->batt_dispatch_pvs_forecast_shift_periods, batt_vars->batt_dispatch_pvs_kf, batt_vars->batt_dispatch_pvs_ki, batt_vars->batt_dispatch_pvs_kp,
                batt_vars->batt_dispatch_pvs_max_ramp, batt_vars->batt_dispatch_pvs_short_forecast_enable,
                batt_vars->batt_dispatch_pvs_soc_rest, batt_vars->batt_dispatch_pvs_timestep_multiplier, batt_vars->grid_interconnection_limit_kW);

        }
        else {
            dispatch_model = new dispatch_automatic_front_of_meter_t(battery_model, dt_hr, batt_vars->batt_minimum_SOC, batt_vars->batt_maximum_SOC,
                batt_vars->batt_current_choice, batt_vars->batt_current_charge_max, batt_vars->batt_current_discharge_max,
                batt_vars->batt_power_charge_max_kwdc, batt_vars->batt_power_discharge_max_kwdc,
                batt_vars->batt_power_charge_max_kwac, batt_vars->batt_power_discharge_max_kwac,
                batt_vars->batt_minimum_modetime,
                batt_vars->batt_dispatch, batt_vars->batt_dispatch_wf_forecast, batt_vars->batt_meter_position,
                nyears, batt_vars->batt_look_ahead_hours, batt_vars->batt_dispatch_update_frequency_hours,
                batt_vars->batt_dispatch_auto_can_charge, batt_vars->batt_dispatch_auto_can_clipcharge, batt_vars->batt_dispatch_auto_can_gridcharge,
                batt_vars->batt_dispatch_auto_can_fuelcellcharge, batt_vars->batt_dispatch_auto_can_curtailcharge,
                batt_vars->inverter_paco, batt_vars->batt_cost_per_kwh,
                batt_vars->batt_cycle_cost_choice, batt_vars->batt_cycle_cost, batt_vars->om_batt_variable_cost_per_kwh,
                batt_vars->forecast_price_series_dollar_per_kwh, utilityRate,
                eta_pvcharge, eta_gridcharge, eta_discharge,  batt_vars->grid_interconnection_limit_kW,
                batt_vars->forecast_cleared_capacities_kw, batt_vars->capacity_forecast_type, batt_vars->cleared_capacity_percent);

            if (batt_vars->batt_dispatch == dispatch_t::FOM_CUSTOM_DISPATCH)
            {
                if (dispatch_automatic_front_of_meter_t* dispatch_fom = dynamic_cast<dispatch_automatic_front_of_meter_t*>(dispatch_model))
                {
                    if (batt_vars->batt_custom_dispatch.size() != 8760 * step_per_hour) {
                        throw exec_error("battery", util::format("Battery custom dispatch time step (%d minutes) does not match simulation time step (%d minutes). Length of batt_custom_dispatch (%d) must be steps_per_hour (%lg) * 8760.", batt_vars->batt_custom_dispatch.size()/8760*60, (int)(60/step_per_hour), batt_vars->batt_custom_dispatch.size(), (double)step_per_hour));
                    }
                    dispatch_fom->set_custom_dispatch(batt_vars->batt_custom_dispatch);
                }
            }
        }
    }
    /*! Behind-the-meter automated dispatch for peak shaving */
    else
    {
         if (batt_vars->ec_rate_defined) {
            util_rate_data = new rate_data();
            rate_setup::setup(&vt, (int)step_per_year, batt_vars->analysis_period, *util_rate_data, "cmod_battery");
         }
         else if (batt_vars->batt_dispatch == dispatch_t::RETAIL_RATE) {
             throw exec_error("Battery", "Cannot select retail rate dispatch without providing utility rate data.");
         }
        dispatch_model = new dispatch_automatic_behind_the_meter_t(battery_model, dt_hr, batt_vars->batt_minimum_SOC, batt_vars->batt_maximum_SOC,
            batt_vars->batt_current_choice, batt_vars->batt_current_charge_max, batt_vars->batt_current_discharge_max,
            batt_vars->batt_power_charge_max_kwdc, batt_vars->batt_power_discharge_max_kwdc,
            batt_vars->batt_power_charge_max_kwac, batt_vars->batt_power_discharge_max_kwac,
            batt_vars->batt_minimum_modetime,
            batt_vars->batt_dispatch, batt_vars->batt_dispatch_wf_forecast, batt_vars->batt_meter_position, nyears,
            batt_vars->batt_look_ahead_hours, batt_vars->batt_dispatch_update_frequency_hours,
            batt_vars->batt_dispatch_auto_can_charge, batt_vars->batt_dispatch_auto_can_clipcharge, batt_vars->batt_dispatch_auto_can_gridcharge,
            batt_vars->batt_dispatch_auto_can_fuelcellcharge, batt_vars->batt_dispatch_auto_can_curtailcharge,
            util_rate_data, batt_vars->batt_cost_per_kwh, batt_vars->batt_cycle_cost_choice, batt_vars->batt_cycle_cost, batt_vars->om_batt_variable_cost_per_kwh, batt_vars->grid_interconnection_limit_kW,
            batt_vars->batt_dispatch_charge_only_system_exceeds_load, batt_vars->batt_dispatch_discharge_only_load_exceeds_system,
            batt_vars->batt_dispatch_auto_btm_can_discharge_to_grid, batt_vars->batt_minimum_outage_SOC, batt_vars->batt_dispatch_load_forecast
        );
        if (batt_vars->batt_dispatch == dispatch_t::CUSTOM_DISPATCH)
        {
            if (dispatch_automatic_behind_the_meter_t* dispatch_btm = dynamic_cast<dispatch_automatic_behind_the_meter_t*>(dispatch_model))
            {
                if (batt_vars->batt_custom_dispatch.size() != 8760 * step_per_hour) {
                    throw exec_error("battery", util::format("Input battery power targets time step (%d minutes) does not match simulation time step (%d minutes). Length of batt_custom_dispatch (%d) must be steps_per_hour (%lg) * 8760.", batt_vars->batt_custom_dispatch.size() / 8760 * 60, (int)(60 / step_per_hour), batt_vars->batt_custom_dispatch.size(), (double)step_per_hour));
                }
                dispatch_btm->set_custom_dispatch(batt_vars->batt_custom_dispatch);
            }
        }
    }

    if (batt_vars->batt_topology == ChargeController::AC_CONNECTED) {
        charge_control = new ACBatteryController(dispatch_model, battery_metrics, batt_vars->batt_ac_dc_efficiency, batt_vars->batt_dc_ac_efficiency);
    }
    else if (batt_vars->batt_topology == ChargeController::DC_CONNECTED) {
        charge_control = new DCBatteryController(dispatch_model, battery_metrics, batt_vars->batt_dc_dc_bms_efficiency, batt_vars->batt_inverter_efficiency_cutoff);
    }

    parse_configuration();
}

void battstor::parse_configuration()
{
    int batt_dispatch = batt_vars->batt_dispatch;
    int batt_weather_forecast = batt_vars->batt_dispatch_wf_forecast;
    int batt_load_forecast = batt_vars->batt_dispatch_load_forecast;
    int batt_meter_position = batt_vars->batt_meter_position;

    // parse configuration
    if (dynamic_cast<dispatch_automatic_t*>(dispatch_model))
    {
        prediction_index = 0;
        if (batt_meter_position == dispatch_t::BEHIND)
        {
            if (batt_dispatch == dispatch_t::PEAK_SHAVING || batt_dispatch == dispatch_t::MAINTAIN_TARGET || batt_dispatch == dispatch_t::RETAIL_RATE ||
                batt_dispatch == dispatch_t::SELF_CONSUMPTION)
            {
                switch (batt_weather_forecast) {
                    case dispatch_t::WEATHER_FORECAST_CHOICE::WF_LOOK_AHEAD:
                        wf_look_ahead = true;
                        break;
                    case dispatch_t::WEATHER_FORECAST_CHOICE::WF_LOOK_BEHIND:
                        wf_look_behind = true;
                        break;
                    case dispatch_t::WEATHER_FORECAST_CHOICE::WF_CUSTOM:
                        wf_input_forecast = true;
                        break;
                }

                switch (batt_load_forecast) {
                    case dispatch_t::LOAD_LOOK_AHEAD:
                        load_look_ahead = true;
                        break;
                    case dispatch_t::LOAD_LOOK_BEHIND:
                        load_look_behind = true;
                        break;
                    case dispatch_t::LOAD_CUSTOM:
                        load_input_forecast = true;
                        break;
                }

                if (batt_dispatch == dispatch_t::MAINTAIN_TARGET || batt_dispatch == dispatch_t::SELF_CONSUMPTION)
                    input_target = true;
            }
            else if (batt_dispatch == dispatch_t::CUSTOM_DISPATCH)
            {
                input_custom_dispatch = true;
            }
        }
        else if (batt_meter_position == dispatch_t::FRONT)
        {
            if (batt_dispatch == dispatch_t::FOM_AUTOMATED_ECONOMIC || batt_dispatch == dispatch_t::FOM_PV_SMOOTHING) {
                switch (batt_weather_forecast) {
                case dispatch_t::WEATHER_FORECAST_CHOICE::WF_LOOK_AHEAD:
                    wf_look_ahead = true;
                    break;
                case dispatch_t::WEATHER_FORECAST_CHOICE::WF_LOOK_BEHIND:
                    wf_look_behind = true;
                    break;
                case dispatch_t::WEATHER_FORECAST_CHOICE::WF_CUSTOM:
                    wf_input_forecast = true;
                    break;
                }
            }
            else if (batt_dispatch == dispatch_t::FOM_CUSTOM_DISPATCH) {
                input_custom_dispatch = true;
            }
        }
    }
    else
        manual_dispatch = true;
}

void battstor::initialize_automated_dispatch(std::vector<ssc_number_t> pv, std::vector<ssc_number_t> load, std::vector<ssc_number_t> cliploss)
{
    if (dynamic_cast<dispatch_automatic_t*>(dispatch_model))
    {
        // automatic look ahead or behind
        size_t nrec = nyears * 8760 * step_per_hour;
        if (!input_custom_dispatch)
        {
            // look ahead
            if (wf_look_ahead)
            {
                if (pv.size() != 0)
                {
                    for (size_t idx = 0; idx != nrec; idx++) {
                        pv_prediction.push_back(pv[idx]);
                    }

                }
                if (cliploss.size() != 0)
                {
                    for (size_t idx = 0; idx != nrec; idx++) {
                        cliploss_prediction.push_back(cliploss[idx]);
                    }
                }
            }
            else if (wf_look_behind)
            {
                // day one Dec 31st
                for (size_t idx = ((8760 * step_per_hour) - (24 * step_per_hour)); idx != 8760 * step_per_hour; idx++)
                {
                    if (pv.size() > idx)
                    {
                        pv_prediction.push_back(pv[idx]);
                    }
                    if (cliploss.size() > idx)
                    {
                        cliploss_prediction.push_back(cliploss[idx]);
                    }
                }

                if (pv.size() != 0)
                {
                    for (size_t idx = 0; idx != nrec - 24 * step_per_hour; idx++) {
                        pv_prediction.push_back(pv[idx]);
                    }
                }
                if (cliploss.size() != 0)
                {
                    for (size_t idx = 0; idx != nrec - 24 * step_per_hour; idx++) {
                        cliploss_prediction.push_back(cliploss[idx]);
                    }
                }
            }
            else if (wf_input_forecast)
            {
                if (pv.size() != 0)
                {
                    for (size_t idx = 0; idx != nrec; idx++) {
                        pv_prediction.push_back(pv[idx]);
                    }
                }
                if (cliploss.size() != 0)
                {
                    for (size_t idx = 0; idx != nrec; idx++) {
                        cliploss_prediction.push_back(cliploss[idx]);
                    }
                }
            }

            // All of these will be false for FOM, so load will not be populated
            if (load_look_ahead) {
                if (load.size() != 0)
                {
                    for (size_t idx = 0; idx != nrec; idx++) {
                        load_prediction.push_back(load[idx]);
                    }
                }
            }
            else if (load_look_behind) {
                // day one uses Dec 31st
                for (size_t idx = ((8760 * step_per_hour) - (24 * step_per_hour)); idx != 8760 * step_per_hour; idx++)
                {
                    if (load.size() > idx)
                    {
                        load_prediction.push_back(load[idx]);
                    }
                }
                if (load.size() != 0)
                {
                    for (size_t idx = 0; idx != nrec - 24 * step_per_hour; idx++) {
                        load_prediction.push_back(load[idx]);
                    }
                }

            }
            else if (load_input_forecast) {
                if (load.size() != 0)
                {
                    for (size_t idx = 0; idx != nrec; idx++) {
                        load_prediction.push_back(load[idx]);
                    }
                }
            }

            // Input checking
            if (pv.size() == 0)
            {

                for (size_t idx = 0; idx != nrec; idx++) {
                    pv_prediction.push_back(0.);
                }
            }
            if (load.size() == 0)
            {
                for (size_t idx = 0; idx != nrec; idx++) {
                    load_prediction.push_back(0.);
                }
            }
            if (cliploss.size() == 0)
            {
                for (size_t idx = 0; idx != nrec; idx++) {
                    cliploss_prediction.push_back(0.);
                }
            }

            if (dispatch_automatic_behind_the_meter_t* automatic_dispatch_btm = dynamic_cast<dispatch_automatic_behind_the_meter_t*>(dispatch_model))
            {
                automatic_dispatch_btm->update_pv_data(pv_prediction);
                automatic_dispatch_btm->update_load_data(load_prediction);
                automatic_dispatch_btm->update_cliploss_data(cliploss_prediction);

                if (input_target)
                    automatic_dispatch_btm->set_target_power(target_power);

                if (batt_vars->ec_rate_defined) {
                    automatic_dispatch_btm->setup_rate_forecast();
                }
            }
            else if (dispatch_automatic_front_of_meter_t* automatic_dispatch_fom = dynamic_cast<dispatch_automatic_front_of_meter_t*>(dispatch_model))
            {
                automatic_dispatch_fom->update_pv_data(pv_prediction);
                automatic_dispatch_fom->update_cliploss_data(cliploss_prediction);
            }
            else if (dispatch_pvsmoothing_front_of_meter_t* pvsmoothing_dispatch_fom = dynamic_cast<dispatch_pvsmoothing_front_of_meter_t*>(dispatch_model))
            {
                pvsmoothing_dispatch_fom->update_pv_data(pv_prediction);
                pvsmoothing_dispatch_fom->update_cliploss_data(cliploss_prediction);
            }
        }
    }

}
battstor::~battstor()
{
    delete battery_model;
    delete battery_metrics;
    delete dispatch_model;
    delete charge_control;
    if (util_rate_data != NULL) {
        delete util_rate_data;
    }
    if (utilityRate != NULL) {
        delete utilityRate;
    }
}

battstor::battstor(const battstor& orig) {
    // copy values
    manual_dispatch = orig.manual_dispatch;
    wf_look_ahead = orig.wf_look_ahead;
    wf_look_behind = orig.wf_look_behind;
    wf_input_forecast = orig.wf_input_forecast;
    load_look_ahead = orig.load_look_ahead;
    load_look_behind = orig.load_look_behind;
    load_input_forecast = orig.load_input_forecast;
    input_target = orig.input_target;
    input_custom_dispatch = orig.input_custom_dispatch;
    step_per_hour = orig.step_per_hour;
    step_per_year = orig.step_per_year;
    nyears = orig.nyears;
    total_steps = orig.total_steps;
    _dt_hour = orig._dt_hour;

    year = orig.year;
    hour = orig.hour;
    step = orig.step;
    index = orig.index;
    year_index = orig.year_index;
    en = orig.en;
    chem = orig.chem;

    dm_percent_discharge = orig.dm_percent_discharge;

    dm_percent_gridcharge = orig.dm_percent_gridcharge;

    target_power = orig.target_power;
    target_power_monthly = orig.target_power_monthly;

    e_charge = orig.e_charge;
    e_discharge = orig.e_discharge;

    /*! Variables to store forecast data */
    pv_prediction = orig.pv_prediction;
    load_prediction = orig.load_prediction;
    cliploss_prediction = orig.cliploss_prediction;
    prediction_index = orig.prediction_index;

    fuelcellPower = orig.fuelcellPower;

    // outputs
    outTotalCharge = orig.outTotalCharge;
    outAvailableCharge = orig.outAvailableCharge;
    outBoundCharge = orig.outBoundCharge;
    outMaxChargeAtCurrent = orig.outMaxChargeAtCurrent;
    outMaxCharge = orig.outMaxCharge;
    outMaxChargeThermal = orig.outMaxChargeThermal;
    outSOC = orig.outSOC;
    outDOD = orig.outDOD;
    outCurrent = orig.outCurrent;
    outCellVoltage = orig.outCellVoltage;
    outBatteryVoltage = orig.outBatteryVoltage;
    outCapacityPercent = orig.outCapacityPercent;
    outCapacityPercentCycle = orig.outCapacityPercentCycle;
    outCapacityPercentCalendar = orig.outCapacityPercentCalendar;
    outCycles = orig.outCycles;
    outDODCycleAverage = orig.outDODCycleAverage;
    outBatteryBankReplacement = orig.outBatteryBankReplacement;
    outBatteryTemperature = orig.outBatteryTemperature;
    outCapacityThermalPercent = orig.outCapacityThermalPercent;
    outDispatchMode = orig.outDispatchMode;
    outBatteryPowerAC = orig.outBatteryPowerAC;
    outBatteryPowerDC = orig.outBatteryPowerDC;
    outGenPower = orig.outGenPower;
    outGenWithoutBattery = orig.outGenWithoutBattery;
    outGridPower = orig.outGridPower;
    outSystemToLoad = orig.outSystemToLoad;
    outBatteryToLoad = orig.outBatteryToLoad;
    outGridToLoad = orig.outGridToLoad;
    outFuelCellToLoad = orig.outFuelCellToLoad;
    outGridPowerTarget = orig.outGridPowerTarget;
    outBattPowerTarget = orig.outBattPowerTarget;
    outSystemToBattAC = orig.outSystemToBattAC;
    outSystemToBattDC = orig.outSystemToBattDC;
    outGridToBatt = orig.outGridToBatt;
    outFuelCellToBatt = orig.outFuelCellToBatt;
    outSystemToGrid = orig.outSystemToGrid;
    outBatteryToGrid = orig.outBatteryToGrid;
    outBatteryToSystemLoad = orig.outBatteryToSystemLoad;
    outFuelCellToGrid = orig.outFuelCellToGrid;
    outBatteryConversionPowerLoss = orig.outBatteryConversionPowerLoss;
    outBatterySystemLoss =  orig.outBatterySystemLoss;
    outBatteryToInverterDC = orig.outBatteryToInverterDC;
    outInterconnectionLoss = orig.outInterconnectionLoss;
    outCritLoadUnmet = orig.outCritLoadUnmet;
    outCritLoad = orig.outCritLoad;
    outUnmetLosses = orig.outUnmetLosses;
    outAnnualSystemChargeEnergy = orig.outAnnualSystemChargeEnergy;
    outAnnualGridChargeEnergy = orig.outAnnualGridChargeEnergy;
    outAnnualChargeEnergy = orig.outAnnualChargeEnergy;
    outAnnualDischargeEnergy = orig.outAnnualDischargeEnergy;
    outAnnualGridImportEnergy = orig.outAnnualGridImportEnergy;
    outAnnualGridExportEnergy = orig.outAnnualGridExportEnergy;
    outAnnualEnergySystemLoss = orig.outAnnualEnergySystemLoss;
    outAnnualEnergyLoss = orig.outAnnualEnergyLoss;
    outMarketPrice = orig.outMarketPrice;
    outCostToCycle = orig.outCostToCycle;
    outBenefitCharge = orig.outBenefitCharge;
    outBenefitGridcharge = orig.outBenefitGridcharge;
    outBenefitClipcharge = orig.outBenefitClipcharge;
    outBenefitDischarge = orig.outBenefitDischarge;

    outPVS_outpower = orig.outPVS_outpower;
    outPVS_battpower = orig.outPVS_battpower;
    outPVS_battsoc = orig.outPVS_battsoc;
    outPVS_curtail = orig.outPVS_curtail;
    outPVS_violation_list = orig.outPVS_violation_list;
    outPVS_P_pv_ac = orig.outPVS_P_pv_ac;
    outPVS_PV_ramp_interval = orig.outPVS_PV_ramp_interval;
    outPVS_forecast_pv_energy = orig.outPVS_forecast_pv_energy;


    outAverageCycleEfficiency = orig.outAverageCycleEfficiency;
    outAverageRoundtripEfficiency = orig.outAverageRoundtripEfficiency;
    outSystemChargePercent = orig.outSystemChargePercent;
    outGridChargePercent = orig.outGridChargePercent;

    outAdjustLosses = orig.outAdjustLosses;

    outDispatchPeriod = orig.outDispatchPeriod;

    // copy models
    if (orig.batt_vars) batt_vars = orig.batt_vars;
    battery_metrics = new battery_metrics_t(orig._dt_hour);


    if (orig.dispatch_model) {
        if (auto disp_man = dynamic_cast<dispatch_manual_t*>(orig.dispatch_model))
            dispatch_model = new dispatch_manual_t(*disp_man);
        else if (auto disp_man_BTM = dynamic_cast<dispatch_automatic_behind_the_meter_t*>(orig.dispatch_model))
            dispatch_model = new dispatch_automatic_behind_the_meter_t(*disp_man_BTM);
        else if (auto disp_auto = dynamic_cast<dispatch_automatic_front_of_meter_t*>(orig.dispatch_model))
            dispatch_model = new dispatch_automatic_front_of_meter_t(*disp_auto);
        else if (auto disp_pvs = dynamic_cast<dispatch_pvsmoothing_front_of_meter_t*>(orig.dispatch_model))
            dispatch_model = new dispatch_pvsmoothing_front_of_meter_t(*disp_pvs);
        else
            throw general_error("dispatch_model in battstor is not of recognized type.");

        battery_model = dispatch_model->battery_model();
    }
    else {
        dispatch_model = nullptr;
    }
    if (orig.charge_control) {
        if (dynamic_cast<ACBatteryController*>(orig.charge_control))
            charge_control = new ACBatteryController(dispatch_model, battery_metrics, batt_vars->batt_ac_dc_efficiency,
                batt_vars->batt_dc_ac_efficiency);
        else if (dynamic_cast<DCBatteryController*>(orig.charge_control))
            charge_control = new DCBatteryController(dispatch_model, battery_metrics, batt_vars->batt_dc_dc_bms_efficiency, batt_vars->batt_inverter_efficiency_cutoff);
        else
            throw general_error("charge_control in battstor is not of recognized type.");
    }
    else {
        charge_control = nullptr;
        if (orig.battery_model) {
            battery_model = new battery_t(*orig.battery_model);
        }
        else {
            battery_model = nullptr;
        }
    }
    utilityRate = NULL;
    if (orig.utilityRate)
        utilityRate = new UtilityRate(*orig.utilityRate);
    util_rate_data = NULL;
    if (orig.util_rate_data)
        util_rate_data = new rate_data(*orig.util_rate_data);

}

bool battstor::uses_forecast() {
    if (batt_vars->batt_meter_position == dispatch_t::FRONT) {
        return batt_vars->batt_dispatch == dispatch_t::FOM_AUTOMATED_ECONOMIC || batt_vars->batt_dispatch == dispatch_t::FOM_PV_SMOOTHING;
    }
    else {
        return batt_vars->batt_dispatch == dispatch_t::RETAIL_RATE || dispatch_t::PEAK_SHAVING;
    }
}

void battstor::check_replacement_schedule()
{
    battery_model->runReplacement(year, hour, step);
}


void battstor::initialize_time(size_t year_in, size_t hour_of_year, size_t step_of_hour)
{
    step = step_of_hour;
    hour = hour_of_year;
    year = year_in;
    index = (year * 8760 + hour) * step_per_hour + step;
    year_index = (hour * step_per_hour) + step;
    step_per_year = 8760 * step_per_hour;
}
void battstor::advance(var_table*, double P_gen, double V_gen, double P_load, double P_crit_load, double ac_wiring_loss, double ac_loss_post_battery, double P_gen_clipped, double xfmr_ll, double xfmr_nll)
{
    BatteryPower* powerflow = dispatch_model->getBatteryPower();
    powerflow->reset();

    if (index < fuelcellPower.size()) {
        powerflow->powerFuelCell = fuelcellPower[index];
    }
    if (index < batt_vars->gridCurtailmentLifetime_MW.size()) {
        powerflow->powerCurtailmentLimit = batt_vars->gridCurtailmentLifetime_MW[index] * 1000.0;
    }
    // Set to false in reset() above
    powerflow->isOutageStep = is_outage_step(index % step_per_year);

    powerflow->powerGeneratedBySystem = P_gen;
    powerflow->powerSystem = P_gen - powerflow->powerFuelCell;
    powerflow->powerLoad = P_load;
    powerflow->powerCritLoad = P_crit_load;
    powerflow->voltageSystem = V_gen;
    powerflow->acLossWiring = ac_wiring_loss;
    powerflow->acLossSystemAvailability = ac_loss_post_battery;
    powerflow->acXfmrLoadLoss = xfmr_ll;
    powerflow->acXfmrNoLoadLoss = xfmr_nll;
    powerflow->powerSystemClipped = P_gen_clipped;

    size_t lifetime_index = util::lifetimeIndex(year, hour, step, _dt_hour);
    powerflow->adjustLosses = battery_model->getAvailabilityLoss(lifetime_index);

    charge_control->run(year, hour, step, year_index);
    outputs_fixed();
    outputs_topology_dependent();
    metrics();
}
void battstor::setSharedInverter(SharedInverter* sharedInverter)
{
    if (DCBatteryController* tmp = dynamic_cast<DCBatteryController*>(charge_control))
        tmp->setSharedInverter(sharedInverter);
    dispatch_model->getBatteryPower()->setSharedInverter(sharedInverter);
}

void battstor::setXfmrRating(double xfmrRating)
{
    dispatch_model->getBatteryPower()->acXfmrRating = xfmrRating;
}

void battstor::outputs_fixed()
{
    auto state = battery_model->get_state();

    // Capacity Output with Losses Applied
    if (chem == battery_params::LEAD_ACID)
    {
        outAvailableCharge[index] = (ssc_number_t)(state.capacity->leadacid.q1);
        outBoundCharge[index] = (ssc_number_t)(state.capacity->leadacid.q2);
    }
    outCellVoltage[index] = (ssc_number_t)(state.voltage->cell_voltage);
    outMaxCharge[index] = (ssc_number_t)(state.capacity->qmax_lifetime);
    outMaxChargeThermal[index] = (ssc_number_t)(state.capacity->qmax_thermal);

    outBatteryTemperature[index] = (ssc_number_t)state.thermal->T_batt;
    outCapacityThermalPercent[index] = (ssc_number_t)(state.thermal->q_relative_thermal);
    
    outTotalCharge[index] = (ssc_number_t)(state.capacity->q0);
    outCurrent[index] = (ssc_number_t)(state.capacity->cell_current);
    outBatteryVoltage[index] = (ssc_number_t)(battery_model->V());

    outCycles[index] = (ssc_number_t)(state.lifetime->n_cycles);
    outSOC[index] = (ssc_number_t)(state.capacity->SOC);
    outDOD[index] = (ssc_number_t)(state.lifetime->cycle_range);
    outDODCycleAverage[index] = (ssc_number_t)(state.lifetime->average_range);
    outCapacityPercent[index] = (ssc_number_t)(state.lifetime->q_relative);
    outAdjustLosses[index] = (ssc_number_t)(state.losses->adjust_loss_percent * 100.0);
    if (batt_vars->batt_life_model == lifetime_params::CALCYC) {
        outCapacityPercentCycle[index] = (ssc_number_t)(state.lifetime->cycle->q_relative_cycle);
        outCapacityPercentCalendar[index] = (ssc_number_t)(state.lifetime->calendar->q_relative_calendar);
    }
    else if (batt_vars->batt_life_model == lifetime_params::LMOLTO) {
        outCapacityPercentCycle[index] = (ssc_number_t)(100. - state.lifetime->lmo_lto->dq_relative_cyc);
        outCapacityPercentCalendar[index] = (ssc_number_t)(100. - state.lifetime->lmo_lto->dq_relative_cal);
    }
}

void battstor::outputs_topology_dependent()
{
    // Power output (all Powers in kWac)
    outBatteryPowerAC[index] = (ssc_number_t)(dispatch_model->power_tofrom_battery_ac());
    outBatteryPowerDC[index] = (ssc_number_t)(dispatch_model->power_tofrom_battery_dc());
    outGridPower[index] = (ssc_number_t)(dispatch_model->power_tofrom_grid());
    outGenPower[index] = (ssc_number_t)(dispatch_model->power_gen());
    outSystemToBattAC[index] = (ssc_number_t)(dispatch_model->power_pv_to_batt_ac());
    outSystemToBattDC[index] = (ssc_number_t)(dispatch_model->power_pv_to_batt_dc());
    outGridToBatt[index] = (ssc_number_t)(dispatch_model->power_grid_to_batt());
    outBatteryToGrid[index] = (ssc_number_t)(dispatch_model->power_battery_to_grid());

    // Fuel cell updates
    if (batt_vars->en_fuelcell) {
        outFuelCellToLoad[index] = (ssc_number_t)(dispatch_model->power_fuelcell_to_load());
        outFuelCellToBatt[index] = (ssc_number_t)(dispatch_model->power_fuelcell_to_batt());
        outFuelCellToGrid[index] = (ssc_number_t)(dispatch_model->power_fuelcell_to_grid());
    }
    outBatteryConversionPowerLoss[index] = (ssc_number_t)(dispatch_model->power_conversion_loss());
    outBatterySystemLoss[index] = (ssc_number_t)(dispatch_model->power_system_loss());
    outSystemToGrid[index] = (ssc_number_t)(dispatch_model->power_pv_to_grid());
    outBatteryToSystemLoad[index] = (ssc_number_t)(dispatch_model->power_battery_to_system_load());
    outBatteryToInverterDC[index] = (ssc_number_t)(dispatch_model->power_battery_to_inverter_dc());
    outInterconnectionLoss[index] = (ssc_number_t)(dispatch_model->power_interconnection_loss());

    if (batt_vars->batt_meter_position == dispatch_t::BEHIND)
    {
        outSystemToLoad[index] = (ssc_number_t)(dispatch_model->power_pv_to_load());
        outBatteryToLoad[index] = (ssc_number_t)(dispatch_model->power_battery_to_load());
        outGridToLoad[index] = (ssc_number_t)(dispatch_model->power_grid_to_load());

        if (batt_vars->batt_dispatch != dispatch_t::MANUAL)
        {
            outGridPowerTarget[index] = (ssc_number_t)(dispatch_model->power_grid_target());
            outBattPowerTarget[index] = (ssc_number_t)(dispatch_model->power_batt_target());
        }
        else {
            outDispatchPeriod[index] = (ssc_number_t)(dispatch_model->get_dispatch_period());
        }

        if (analyze_outage) {
            outCritLoadUnmet[index] = (ssc_number_t)(dispatch_model->power_crit_load_unmet());
            outCritLoad[index] = (ssc_number_t)(dispatch_model->power_crit_load());
            outUnmetLosses[index] = (ssc_number_t)(dispatch_model->power_losses_unmet());
        }
    }
    else if (batt_vars->batt_meter_position == dispatch_t::FRONT)
    {
        if (batt_vars->batt_dispatch == dispatch_t::FOM_PV_SMOOTHING) {
            dispatch_pvsmoothing_front_of_meter_t* dispatch_fom = dynamic_cast<dispatch_pvsmoothing_front_of_meter_t*>(dispatch_model);
            outPVS_battpower[index] = dispatch_fom->batt_dispatch_pvs_battpower();
            outPVS_battsoc[index] = dispatch_fom->batt_dispatch_pvs_battsoc();
            outPVS_curtail[index] = dispatch_fom->batt_dispatch_pvs_curtail();
            outPVS_outpower[index] = dispatch_fom->batt_dispatch_pvs_outpower();
            outPVS_violation_list[index] = dispatch_fom->batt_dispatch_pvs_violation_list();
            outPVS_P_pv_ac[index] = dispatch_fom->batt_dispatch_pvs_P_pv_ac();
            outPVS_PV_ramp_interval[index] = dispatch_fom->batt_dispatch_pvs_PV_ramp_interval();
            outPVS_forecast_pv_energy[index] = dispatch_fom->batt_dispatch_pvs_forecast_pv_energy();
            // remove pv smoothing hour limited (curtailed) power - lost (before system output in pvsamv1)
            outGenPower[index] -= outPVS_curtail[index];
        }
        else if (batt_vars->batt_dispatch != dispatch_t::FOM_MANUAL) {
            dispatch_automatic_front_of_meter_t* dispatch_fom = dynamic_cast<dispatch_automatic_front_of_meter_t*>(dispatch_model);
            outBattPowerTarget[index] = (ssc_number_t)(dispatch_model->power_batt_target());
            outBenefitCharge[index] = (ssc_number_t)(dispatch_fom->benefit_charge());
            outBenefitDischarge[index] = (ssc_number_t)(dispatch_fom->benefit_discharge());
            outBenefitClipcharge[index] = (ssc_number_t)(dispatch_fom->benefit_clipcharge());
            outBenefitGridcharge[index] = (ssc_number_t)(dispatch_fom->benefit_gridcharge());
        }
        else {
            outDispatchPeriod[index] = (ssc_number_t)(dispatch_model->get_dispatch_period());
        }
    }

    bool cycleCostRelevant = (batt_vars->batt_meter_position == dispatch_t::BEHIND && batt_vars->batt_dispatch == dispatch_t::RETAIL_RATE) ||
        (batt_vars->batt_meter_position == dispatch_t::FRONT && (batt_vars->batt_dispatch != dispatch_t::FOM_MANUAL && batt_vars->batt_dispatch != dispatch_t::FOM_CUSTOM_DISPATCH));
    if (cycleCostRelevant && batt_vars->batt_cycle_cost_choice == dispatch_t::MODEL_CYCLE_COST) {
        outCostToCycle[index] = (ssc_number_t)(dispatch_model->cost_to_cycle_per_kwh());
    }
}

void battstor::metrics()
{
    size_t annual_index = year;
    outBatteryBankReplacement[annual_index] = (ssc_number_t)battery_model->getNumReplacementYear();

    if ((hour == 8759) && (step == step_per_hour - 1))
    {
        battery_model->resetReplacement();
        outAnnualGridImportEnergy[annual_index] = (ssc_number_t)(battery_metrics->energy_grid_import_annual());
        outAnnualGridExportEnergy[annual_index] = (ssc_number_t)(battery_metrics->energy_grid_export_annual());
        outAnnualSystemChargeEnergy[annual_index] = (ssc_number_t)(battery_metrics->energy_pv_charge_annual());
        outAnnualGridChargeEnergy[annual_index] = (ssc_number_t)(battery_metrics->energy_grid_charge_annual());
        outAnnualChargeEnergy[annual_index] = (ssc_number_t)(battery_metrics->energy_charge_annual());
        outAnnualDischargeEnergy[annual_index] = (ssc_number_t)(battery_metrics->energy_discharge_annual());
        outAnnualEnergyLoss[annual_index] = (ssc_number_t)(battery_metrics->energy_loss_annual());
        outAnnualEnergySystemLoss[annual_index] = (ssc_number_t)(battery_metrics->energy_system_loss_annual());
        battery_metrics->new_year();
    }

    // Average battery conversion efficiency
    outAverageCycleEfficiency = (ssc_number_t)battery_metrics->average_battery_conversion_efficiency();
    if (outAverageCycleEfficiency > 100)
        outAverageCycleEfficiency = 100;
    else if (outAverageCycleEfficiency < 0)
        outAverageCycleEfficiency = 0;

    // Average battery roundtrip efficiency
    outAverageRoundtripEfficiency = (ssc_number_t)battery_metrics->average_battery_roundtrip_efficiency();
    if (outAverageRoundtripEfficiency > 100)
        outAverageRoundtripEfficiency = 100;
    else if (outAverageRoundtripEfficiency < 0)
        outAverageRoundtripEfficiency = 0;

    // PV charge ratio
    outSystemChargePercent = (ssc_number_t)battery_metrics->pv_charge_percent();
    if (outSystemChargePercent > 100)
        outSystemChargePercent = 100;
    else if (outSystemChargePercent < 0)
        outSystemChargePercent = 0;

    // Grid charge ratio
    outGridChargePercent = (ssc_number_t)battery_metrics->grid_charge_percent();
    if (outGridChargePercent > 100)
        outGridChargePercent = 100;
    else if (outGridChargePercent < 0)
        outGridChargePercent = 0;

}

// function needed to correctly calculate P_grid due to additional losses in P_gen post battery like wiring, curtailment, availablity
void battstor::update_grid_power(compute_module&, double P_gen_ac, double P_load_ac, size_t index_replace)
{
    // Need to get this tolerance instead of the one defined in the header of battery capacity
    BatteryPower* powerflow = dispatch_model->getBatteryPower();
    double tolerance = powerflow->tolerance;

    double P_interconnection_loss = outInterconnectionLoss[index_replace];
    double P_grid_old = outGridPower[index_replace] + P_interconnection_loss;
    double P_grid = P_gen_ac - P_load_ac;
    double P_crit_load_unmet = 0;
    double P_unmet_losses = 0;
    if (analyze_outage) {
        P_crit_load_unmet = outCritLoadUnmet[index_replace];
        if (P_gen_ac < 0.0) {
            if (std::abs(P_gen_ac) < tolerance) {
                outUnmetLosses[index_replace] = 0.0;
            }
            else {
                // Update post-AC losses
                outUnmetLosses[index_replace] = std::abs(P_gen_ac);
            }
        }
        P_unmet_losses = outUnmetLosses[index_replace];
        if (std::abs(P_unmet_losses) < tolerance) {
            P_unmet_losses = 0.0;
            outUnmetLosses[index_replace] = 0.0;
        }
    }
    
    if (P_grid >= 0) {
        P_interconnection_loss = std::fmax(P_interconnection_loss - (P_grid_old - P_grid), 0.0);
    }
    else {
        // Interconnection losses should be 0 if drawing power from the grid
        P_interconnection_loss = 0;
    }
    outInterconnectionLoss[index_replace] = P_interconnection_loss;
    P_grid = P_gen_ac - P_load_ac - P_interconnection_loss + P_crit_load_unmet + P_unmet_losses;
    if (std::abs(P_grid) < tolerance) {
        P_grid = 0;
    }

    outGridPower[index_replace] = (ssc_number_t)(P_grid);
}

bool battstor::is_outage_step(size_t idx) {
    if (idx < batt_vars->grid_outage_steps.size()) {
        return batt_vars->grid_outage_steps[idx];
    }
    return false;
}

bool battstor::is_offline(size_t index) {
    double soc = (double) outSOC[index];
    double prev_soc = soc;
    if (index > 0) {
        prev_soc = (double) outSOC[index - 1];
    }
    
    double min_soc = dispatch_model->getBatteryPower()->stateOfChargeMin;

    return ((soc - min_soc) < low_tolerance) && ((prev_soc - min_soc) < low_tolerance);
}

void battstor::calculate_monthly_and_annual_outputs(compute_module& cm)
{
    // single value metrics
    cm.assign("average_battery_conversion_efficiency", var_data((ssc_number_t)outAverageCycleEfficiency));
    cm.assign("average_battery_roundtrip_efficiency", var_data((ssc_number_t)outAverageRoundtripEfficiency));
    cm.assign("batt_system_charge_percent", var_data((ssc_number_t)outSystemChargePercent));
    cm.assign("batt_grid_charge_percent", var_data((ssc_number_t)outGridChargePercent));
    cm.assign("batt_bank_installed_capacity", (ssc_number_t)batt_vars->batt_kwh);

    cm.assign("batt_year1_charge_from_grid", var_data((ssc_number_t)outAnnualGridChargeEnergy[0]));
    cm.assign("batt_year1_charge_from_system", var_data((ssc_number_t)outAnnualSystemChargeEnergy[0]));

    // monthly outputs
    cm.accumulate_monthly_for_year("system_to_batt", "monthly_system_to_batt", _dt_hour, step_per_hour);
    cm.accumulate_monthly_for_year("grid_to_batt", "monthly_grid_to_batt", _dt_hour, step_per_hour);
    cm.accumulate_monthly_for_year("system_to_grid", "monthly_system_to_grid", _dt_hour, step_per_hour);
    cm.accumulate_monthly_for_year("interconnection_loss", "monthly_interconnection_loss", _dt_hour, step_per_hour);
    cm.accumulate_monthly_for_year("batt_to_grid", "monthly_batt_to_grid", _dt_hour, step_per_hour);

    // critical load unmet values
    if (cm.is_assigned("crit_load_unmet")) {
        auto annual_unmet_load = cm.accumulate_annual_for_year("crit_load_unmet", "annual_crit_load_unmet", _dt_hour, step_per_hour);
        auto pmonthly_unmet_load = cm.accumulate_monthly_for_year("crit_load_unmet", "monthly_crit_load_unmet", _dt_hour, step_per_hour);
        if (cm.is_assigned("crit_load")) {
            auto annual_load = cm.accumulate_annual_for_year("crit_load", "annual_crit_load", _dt_hour, step_per_hour);
            auto pmonthly_load = cm.accumulate_monthly_for_year("crit_load", "monthly_crit_load", _dt_hour, step_per_hour);
            ssc_number_t* monthly_unmet_percentage = cm.allocate("monthly_crit_load_unmet_percentage", 12);
            for (size_t i = 0; i < 12; i++) {
                monthly_unmet_percentage[i] = 100.0 * (pmonthly_load[i] > 0 ? pmonthly_unmet_load[i] / pmonthly_load[i] : 0.0);
            }
            cm.assign("annual_crit_load_unmet_percentage", (var_data)((ssc_number_t)(100.0 * (annual_load > 0 ? annual_unmet_load / annual_load : 0.0))));
        }
        if (cm.is_assigned("outage_losses_unmet")) {
            auto annual_unmet_losses = cm.accumulate_annual_for_year("outage_losses_unmet", "annual_outage_losses_unmet", _dt_hour, step_per_hour);
            auto pmonthly_unmet_losses = cm.accumulate_monthly_for_year("outage_losses_unmet", "monthly_outage_losses_unmet", _dt_hour, step_per_hour);
        }
    }


    if (batt_vars->batt_meter_position == dispatch_t::BEHIND)
    {
        cm.accumulate_monthly_for_year("system_to_load", "monthly_system_to_load", _dt_hour, step_per_hour);
        cm.accumulate_monthly_for_year("batt_to_load", "monthly_batt_to_load", _dt_hour, step_per_hour);
        cm.accumulate_monthly_for_year("grid_to_load", "monthly_grid_to_load", _dt_hour, step_per_hour);

        if (batt_vars->batt_dispatch == dispatch_t::SELF_CONSUMPTION)
        {
            std::vector<double> crit_load_unmet;
            if (cm.is_assigned("crit_load_unmet")) {
                crit_load_unmet = cm.as_vector_double("crit_load_unmet");
            }

            //calculate all outputs for number of timesteps the load is met by the system, using grid_to_load == 0 as a qualification
            //better to parse the grid_to_load timeseries once here for all outputs, than to create a new timeseries variable for whether load is met by system
            outTimestepsLoadMetBySystemYear1 = 0.0;
            outTimestepsLoadMetBySystemLifetime = 0.0;
            for (size_t i = 0; i < total_steps; i++)
            {
                double crit_load_unmet_i = 0.0;
                if (i < crit_load_unmet.size()) {
                    crit_load_unmet_i = crit_load_unmet[i];
                }
                
                if (outGridToLoad[i] == 0.0 && crit_load_unmet_i == 0.0)
                {
                    outTimestepsLoadMetBySystemLifetime++;
                    if (i < step_per_year) outTimestepsLoadMetBySystemYear1++;
                }
            }
            cm.assign("num_ts_load_met_by_system_yr1", outTimestepsLoadMetBySystemYear1);
            cm.assign("num_ts_load_met_by_system_lifetime", outTimestepsLoadMetBySystemLifetime);
            outPercentTimestepsLoadMetBySystemYear1 = (outTimestepsLoadMetBySystemYear1 / step_per_year) * 100.0;
            cm.assign("percent_ts_load_met_by_system_yr1", (ssc_number_t)outPercentTimestepsLoadMetBySystemYear1);
            outPercentTimestepsLoadMetBySystemLifetime = (outTimestepsLoadMetBySystemLifetime / total_steps) * 100.0;
            cm.assign("percent_ts_load_met_by_system_lifetime", (ssc_number_t)outPercentTimestepsLoadMetBySystemLifetime);
         }
    }
    else if (batt_vars->batt_meter_position == dispatch_t::FRONT)
    {
        if (batt_vars->batt_dispatch == dispatch_t::FOM_PV_SMOOTHING) {
            // total number of violations
            size_t violation_count = 0;
            // violation percent
            ssc_number_t violation_percent = 0;
            // energy to grid percent - algorithm and actual
            ssc_number_t energy_to_grid_pvs = 0; // sum of pvs outpower
            ssc_number_t energy_to_grid_sam = 0; // sum of outGenPower
            ssc_number_t energy_to_grid_pv = 0; // pv system only energy no battery or curtaiment
            for (size_t i = 0; i < total_steps; i++) {
                violation_count += (size_t)outPVS_violation_list[i];
                energy_to_grid_pv += outPVS_P_pv_ac[i] * _dt_hour;
                energy_to_grid_pvs += outPVS_outpower[i] * _dt_hour;
                energy_to_grid_sam += outGenPower[i] * _dt_hour;
            }

            cm.assign("batt_pvs_violation_count", (ssc_number_t)violation_count);

            violation_percent = (ssc_number_t)total_steps;
            if (batt_vars->batt_dispatch_pvs_timestep_multiplier > 0)
                violation_percent /= ssc_number_t(batt_vars->batt_dispatch_pvs_timestep_multiplier);
            if (violation_percent > 0)
                violation_percent = violation_count / violation_percent;
            cm.assign("batt_pvs_violation_percent", violation_percent * 100.0);

            if (energy_to_grid_pv <= 0) { // throw if this happens
                energy_to_grid_pv = 1.0;
            }
            // unscale pvs outpower if scaled
            //energy_to_grid_pvs *= batt_vars->batt_dispatch_pvs_nameplate_ac;
            cm.assign("batt_pvs_energy_to_grid_percent", energy_to_grid_pvs / energy_to_grid_pv * 100.0);
            cm.assign("batt_pvs_energy_to_grid_percent_sam", energy_to_grid_sam / energy_to_grid_pv * 100.0);




            /*
                       // validation work - all ramp_interval size TODO - remove after validation
                dispatch_pvsmoothing_front_of_meter_t* dispatch_fom = dynamic_cast<dispatch_pvsmoothing_front_of_meter_t*>(dispatch_model);
                auto bp_vec = dispatch_fom->batt_dispatch_pvs_battpower_vec();
                ssc_number_t* bp_vec_ssc = cm.allocate("batt_pvs_battpower_vec", bp_vec.size());
                auto op_vec = dispatch_fom->batt_dispatch_pvs_outpower_vec();
                ssc_number_t* op_vec_ssc = cm.allocate("batt_pvs_outpower_vec", op_vec.size());
                auto cp_vec = dispatch_fom->batt_dispatch_pvs_curtail_vec();
                ssc_number_t* cp_vec_ssc = cm.allocate("batt_pvs_curtail_vec", cp_vec.size());
                auto soc_vec = dispatch_fom->batt_dispatch_pvs_battsoc_vec();
                ssc_number_t* soc_vec_ssc = cm.allocate("batt_pvs_battsoc_vec", soc_vec.size());
                auto vl_vec = dispatch_fom->batt_dispatch_pvs_violation_list_vec();
                ssc_number_t* vl_vec_ssc = cm.allocate("batt_pvs_violation_list_vec", vl_vec.size());

                auto pv_vec = dispatch_fom->batt_dispatch_pvs_PV_ramp_interval_vec();
                ssc_number_t* pv_vec_ssc = cm.allocate("batt_pvs_PV_ramp_interval_vec", pv_vec.size());
                auto ef_vec = dispatch_fom->batt_dispatch_pvs_forecast_pv_energy_vec();
                ssc_number_t* ef_vec_ssc = cm.allocate("batt_pvs_forecast_pv_energy_vec", ef_vec.size());

                //  check for same size
                for (size_t i = 0; i < bp_vec.size(); i++) {
                    bp_vec_ssc[i] = bp_vec[i];
                    op_vec_ssc[i] = op_vec[i];
                    cp_vec_ssc[i] = cp_vec[i];
                    soc_vec_ssc[i] = soc_vec[i];
                    vl_vec_ssc[i] = vl_vec[i];
                    pv_vec_ssc[i] = pv_vec[i];
                    ef_vec_ssc[i] = ef_vec[i];
                }
             */
        }


    }

    if (batt_vars->system_use_lifetime_output) {
        /* Add yr 0 to annual outputs */
        size_t arr_length = nyears + 1;
        ssc_number_t yr_0_value = 0.0;
        prepend_to_output(&cm, "annual_import_to_grid_energy", arr_length, yr_0_value);
        prepend_to_output(&cm, "annual_export_to_grid_energy", arr_length, yr_0_value);
        prepend_to_output(&cm, "batt_annual_energy_system_loss", arr_length, yr_0_value);
        prepend_to_output(&cm, "batt_annual_energy_loss", arr_length, yr_0_value);
        prepend_to_output(&cm, "batt_annual_charge_from_grid", arr_length, yr_0_value);
    }
}

///////////////////////////////////////////////////
static var_info _cm_vtab_battery[] = {
    /*   VARTYPE           DATATYPE         NAME                                             LABEL                                                   UNITS      META                           GROUP                  REQUIRED_IF                 CONSTRAINTS                      UI_HINTS*/
    { SSC_INOUT,        SSC_NUMBER,      "percent_complete",                           "Estimated simulation status",                             "%",          "",                     "Simulation",                        "",                            "",                               "" },
    { SSC_INPUT,        SSC_NUMBER,      "system_use_lifetime_output",                 "Lifetime simulation",                                     "0/1",        "0=SingleYearRepeated,1=RunEveryYear",   "Lifetime",        "?=0",                   "BOOLEAN",                              "" },
    { SSC_INPUT,        SSC_NUMBER,      "analysis_period",                            "Lifetime analysis period",                                "years",      "The number of years in the simulation", "Lifetime",        "system_use_lifetime_output=1","",                               "" },
    { SSC_INPUT,        SSC_NUMBER,      "timestep_minutes",                           "Simulation timestep",                                    "minutes",         "The number of minutes in each timestep", "Simulation",        "en_standalone_batt=1","",                               "" },
    { SSC_INPUT,        SSC_NUMBER,      "en_batt",                                    "Enable battery storage model",                            "0/1",        "",                     "BatterySystem",                      "?=0",                    "",                               "" },
    { SSC_INPUT,        SSC_NUMBER,      "en_standalone_batt",                         "Enable standalone battery storage model",                 "0/1",        "",                     "BatterySystem",                      "?=0",                    "",                               "" },
    { SSC_INPUT,        SSC_NUMBER,      "en_wave_batt",                         "Enable wave battery storage model",                 "0/1",        "",                     "BatterySystem",                      "?=0",                    "",                               "" },
    { SSC_INOUT,        SSC_ARRAY,       "energy_hourly_kW",								  "Power output of array",                "kW",        "Lifetime system generation",          "System Output",                  "en_wave_batt=1",                        "",                              "" },

    { SSC_INOUT,        SSC_ARRAY,       "gen",										   "System power generated",                                  "kW",         "",                     "System Output",                    "",                       "",                               "" },
    { SSC_INPUT,		SSC_ARRAY,	     "load",			                           "Electricity load (year 1)",                               "kW",	        "",				        "Load",                             "",	                      "",	                            "" },
    { SSC_INPUT,		SSC_ARRAY,	     "crit_load",			                       "Critical electricity load (year 1)",                      "kW",	        "",				        "Load",                             "",	                      "",	                            "" },
    { SSC_INPUT,        SSC_ARRAY,       "load_escalation",                            "Annual load escalation",                                  "%/year",     "",                     "Load",                             "?=0",                    "",                               "" },
    { SSC_INPUT,        SSC_ARRAY,       "crit_load_escalation",                       "Annual critical load escalation",                         "%/year",     "",                     "Load",                             "?=0",                    "",                    "" },
    { SSC_INPUT,        SSC_ARRAY,       "grid_outage",                                "Grid outage in this time step",                              "0/1",        "0=GridAvailable,1=GridUnavailable,Length=load", "Load",    "",                       "",                               "" },
    { SSC_INPUT,        SSC_NUMBER,      "run_resiliency_calcs",                       "Enable resilence calculations for every timestep",        "0/1",        "0=DisableCalcs,1=EnableCalcs",                  "Load",    "?=0",                    "",                               "" },
    { SSC_INOUT,        SSC_NUMBER,      "capacity_factor",                            "Capacity factor",                                         "%",          "",                     "System Output",                             "",                    "",                               "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "capacity_factor_sales",                      "Capacity factor based on AC electricity to grid",                                         "%",          "",                     "System Output",                             "",                    "",                               "" },

    { SSC_INOUT,        SSC_NUMBER,      "annual_energy",                              "Annual AC energy in Year 1",                                           "kWh",        "",                     "System Output",                      "?=0",                    "",                               "" },

    // other variables come from battstor common table
    var_info_invalid };

extern var_info vtab_fuelcell_output[];

class cm_battery : public compute_module
{
public:

    cm_battery()
    {
        add_var_info(_cm_vtab_battery);
        add_var_info(vtab_battery_inputs);
        add_var_info(vtab_forecast_price_signal);
        add_var_info(vtab_batt_adjustment_factors);
        add_var_info(vtab_battery_outputs);
        add_var_info(vtab_resilience_outputs);
        add_var_info(vtab_utility_rate_common);
        add_var_info(vtab_grid_curtailment);
        add_var_info(vtab_hybrid_tech_om_outputs);

    }

    void exec() override
    {
        if (as_boolean("en_batt") || as_boolean("en_standalone_batt") || as_boolean("en_wave_batt"))
        {
            std::vector<ssc_number_t> power_input_lifetime;
            std::vector<ssc_number_t> load_lifetime, load_year_one;
            std::vector<ssc_number_t> grid_curtailment;
            size_t nload;
            std::string timestep_source = "weather file";
            bool use_lifetime = as_boolean("system_use_lifetime_output");
            // System generation output, which is lifetime (if system_lifetime_output == true);
            if (as_boolean("en_standalone_batt")) {
                int timestep_minutes = as_number("timestep_minutes");
                size_t n_rec_year_1 = 8760 * 60 / timestep_minutes;

                if (use_lifetime)
                    power_input_lifetime.resize(n_rec_year_1 * as_integer("analysis_period"), 0.0);
                else
                    power_input_lifetime.resize(n_rec_year_1, 0.0);

                ssc_number_t* p_gen = allocate("gen", power_input_lifetime.size());
                for (size_t i = 0; i < power_input_lifetime.size(); i++)
                    p_gen[i] = power_input_lifetime[i];

                timestep_source = "simulation timestep";
            }
            else if (as_boolean("en_wave_batt")) {
                power_input_lifetime = as_vector_ssc_number_t("energy_hourly_kW");
            }
            else
                power_input_lifetime = as_vector_ssc_number_t("gen");

            size_t n_rec_lifetime = power_input_lifetime.size();
            size_t analysis_period = (size_t)as_integer("analysis_period");

            if (use_lifetime && (double)(util::hours_per_year * analysis_period) / n_rec_lifetime > 1)
                throw exec_error("battery", "Input gen must be from lifetime simulation when system_use_lifetime_output=1.");

            size_t n_rec_single_year = use_lifetime ? n_rec_lifetime / analysis_period : n_rec_lifetime;
            double dt_hour_gen = (double)(util::hours_per_year) / n_rec_single_year;
            if (is_assigned("load")) {
                load_year_one = as_vector_ssc_number_t("load");
                nload = load_year_one.size();
                // Array length for non-lifetime mode, lifetime mode, and hourly load
                if (nload != n_rec_lifetime && nload != n_rec_lifetime / analysis_period && nload != 8760)
                    throw exec_error("battery", "Electric load must have either the same time step as the " + timestep_source + ", or 8760 time steps.");
            }

            // resilience metrics for battery
            std::unique_ptr<resilience_runner> resilience = nullptr;
            std::vector<ssc_number_t> p_crit_load;
            std::vector<ssc_number_t> p_crit_load_full; p_crit_load_full.reserve(n_rec_lifetime);
            bool run_resilience = as_boolean("run_resiliency_calcs");
            // Need to grab the crit load before battstor assigns it as an output
            if (is_assigned("crit_load")) {
                p_crit_load = as_vector_ssc_number_t("crit_load");
                size_t n_crit_load = p_crit_load.size();
                if (n_crit_load != n_rec_single_year && n_crit_load != 8760)
                    throw exec_error("battery", "Critical load crit_load must have same number of values as " + timestep_source + ", or 8760.");
                if (n_crit_load != nload)
                    throw exec_error("battery", "Critical load crit_load must have same number of values as load.");

            }

            auto batt = std::make_shared<battstor>(*m_vartab, true, n_rec_single_year, dt_hour_gen);

            if (is_assigned("crit_load")) {
                bool crit_load_specified = !p_crit_load.empty() && *std::max_element(p_crit_load.begin(), p_crit_load.end()) > 0;
                if (run_resilience) {
                    if (crit_load_specified) {
                        resilience = std::unique_ptr<resilience_runner>(new resilience_runner(batt));
                        auto logs = resilience->get_logs();
                        if (!logs.empty()) {
                            log(logs[0], SSC_WARNING);
                        }
                    }
                    else {
                        throw exec_error("battery", "If run_resiliency_calcs is 1, crit_load must have length > 0 and values > 0.");
                    }
                }
                if (!crit_load_specified && batt->analyze_outage) {
                    throw exec_error("battery", "If grid_outage is specified in any time step, crit_load must have length > 0 and values > 0.");
                }
            }

            scalefactors scale_calculator(m_vartab);
            // compute load (electric demand) annual escalation multipliers
            std::vector<ssc_number_t> load_scale = scale_calculator.get_factors("load_escalation");

            double interpolation_factor = 1.0;
            single_year_to_lifetime_interpolated<ssc_number_t>(
                use_lifetime,
                analysis_period,
                n_rec_lifetime,
                load_year_one,
                load_scale,
                interpolation_factor,
                load_lifetime,
                n_rec_single_year,
                dt_hour_gen);

            // Setup custom forecasts
            std::vector<ssc_number_t> p_pv_ac_forecast;
            int batt_forecast_choice = as_integer("batt_dispatch_wf_forecast_choice");
            if (is_assigned("batt_pv_ac_forecast")) {
                p_pv_ac_forecast = as_vector_ssc_number_t("batt_pv_ac_forecast");
                // Annual simulation is enforced above
                if (p_pv_ac_forecast.size() < dt_hour_gen * 8760) {
                    if (batt_forecast_choice == dispatch_t::WEATHER_FORECAST_CHOICE::WF_CUSTOM && batt->uses_forecast()) {
                        throw exec_error("battery", "Length of batt_pv_ac_forecast is " + std::to_string(p_pv_ac_forecast.size()) + " when custom weather file forecast is selected. Change batt_dispatch_wf_forecast_choice or provide a forecast of at least length " + std::to_string(dt_hour_gen * 8760) + ".");
                    }
                    else {
                        // Using look ahead or look behind, and need to clear inputs from lk
                        p_pv_ac_forecast = power_input_lifetime;
                    }
                }
            }
            else {
                p_pv_ac_forecast = power_input_lifetime;
            }

            std::vector<ssc_number_t> p_load_forecast_in;
            std::vector<ssc_number_t> p_load_forecast_full;
            p_load_forecast_full.reserve(n_rec_lifetime);
            if (is_assigned("batt_load_ac_forecast"))
            {
                p_load_forecast_in = as_vector_ssc_number_t("batt_load_ac_forecast");
                size_t nload = p_load_forecast_in.size();
                if (nload == 1 || !batt->uses_forecast()) {
                    // Length 1 is "empty" to UI lk
                    p_load_forecast_in.clear();
                }
                // Array length for non-lifetime mode, lifetime mode, and hourly load
                else if (nload != n_rec_lifetime && nload != n_rec_lifetime / analysis_period && nload != 8760) {
                        throw exec_error("battery", "Electric load forecast must have either the same time step as the " + timestep_source + ", or 8760 time steps.");
                }
            }
            if (p_load_forecast_in.size() > 0) {
                std::vector<ssc_number_t> load_forecast_scale = scale_calculator.get_factors("batt_load_ac_forecast_escalation");
                interpolation_factor = 1.0;
                single_year_to_lifetime_interpolated<ssc_number_t>(
                    use_lifetime,
                    analysis_period,
                    n_rec_lifetime,
                    p_load_forecast_in,
                    load_forecast_scale,
                    interpolation_factor,
                    p_load_forecast_full,
                    n_rec_single_year,
                    dt_hour_gen);
            }
            else {
                p_load_forecast_full = load_lifetime;
            }

            // Create battery structure and initialize

            if (is_assigned("fuelcell_power"))
                add_var_info(vtab_fuelcell_output);
            batt->initialize_automated_dispatch(p_pv_ac_forecast, p_load_forecast_full);

            if (load_lifetime.size() != n_rec_lifetime) {
                throw exec_error("battery", "Load length does not match system generation length.");
            }
            if (batt->batt_vars->batt_topology == ChargeController::DC_CONNECTED) {
                throw exec_error("battery", "Generic system requires AC-connected battery.");
            }

            // compute critical load (electric demand) annual escalation multipliers
            std::vector<ssc_number_t> crit_load_scale = scale_calculator.get_factors("crit_load_escalation");

            interpolation_factor = 1.0;
            single_year_to_lifetime_interpolated<ssc_number_t>(
                use_lifetime,
                analysis_period,
                n_rec_lifetime,
                p_crit_load,
                crit_load_scale,
                interpolation_factor,
                p_crit_load_full,
                n_rec_single_year,
                dt_hour_gen);

            // Prepare outputs
            ssc_number_t* p_gen;
            if (as_boolean("en_wave_batt"))
                p_gen = allocate("energy_hourly_kW", n_rec_lifetime);
            else
                p_gen = allocate("gen", n_rec_lifetime);
            double capacity_factor_in, annual_energy_in, nameplate_in;
            capacity_factor_in = annual_energy_in = nameplate_in = 0;

            if (is_assigned("capacity_factor") && is_assigned("annual_energy")) {
                capacity_factor_in = as_double("capacity_factor");
                annual_energy_in = as_double("annual_energy");
                nameplate_in = (annual_energy_in / (capacity_factor_in * 0.01)) / util::hours_per_year;
            }


            /* *********************************************************************************************
            Run Simulation
            *********************************************************************************************** */
            double annual_energy = 0;
            double annual_energy_sales = 0;
            float percent_complete = 0.0;
            float percent = 0.0;
            size_t nStatusUpdates = 50;
            // assume that anyone using this module is chaining with two techs
            float techs = 1;

            if (is_assigned("percent_complete")) {
                percent_complete = as_float("percent_complete");
            }

            size_t lifetime_idx = 0;
            for (size_t year = 0; year != batt->nyears; year++)
            {
                for (size_t hour = 0; hour < 8760; hour++)
                {
                    // status bar
                    if (hour % (8760 / nStatusUpdates) == 0)
                    {
                        percent = percent_complete + 100.0f * ((float)lifetime_idx + 1) / ((float)n_rec_lifetime) / techs;
                        if (!update("", percent, (float)hour)) {
                            throw exec_error("battery", "Simulation canceled at hour " + util::to_string(hour + 1.0) + ".");
                        }
                    }

                    for (size_t jj = 0; jj < batt->step_per_hour; jj++)
                    {

                        batt->initialize_time(year, hour, jj);
                        batt->check_replacement_schedule();

                        if (resilience) {
                            resilience->add_battery_at_outage_timestep(*batt->dispatch_model, lifetime_idx);
                            resilience->run_surviving_batteries(p_crit_load_full[lifetime_idx], power_input_lifetime[lifetime_idx]);
                        }

                        batt->outGenWithoutBattery[lifetime_idx] = power_input_lifetime[lifetime_idx];
                        batt->advance(m_vartab, power_input_lifetime[lifetime_idx], 0, load_lifetime[lifetime_idx], p_crit_load_full[lifetime_idx], 0);
                        p_gen[lifetime_idx] = batt->outGenPower[lifetime_idx];
                        if (year == 0) {
                            annual_energy += p_gen[lifetime_idx] * batt->_dt_hour;
                            if (p_gen[lifetime_idx] > 0) annual_energy_sales += p_gen[lifetime_idx] * batt->_dt_hour;
                        }
                        lifetime_idx++;
                    }
                }
            }
            batt->calculate_monthly_and_annual_outputs(*this);
            gen_heatmap(this, double(n_rec_single_year / 8760));

            // update capacity factor and annual energy
            assign("capacity_factor", var_data(static_cast<ssc_number_t>(annual_energy * 100.0 / (nameplate_in * util::hours_per_year))));
            assign("capacity_factor_sales", var_data(static_cast<ssc_number_t>(annual_energy_sales * 100.0 / (nameplate_in * util::hours_per_year))));
            assign("annual_energy", var_data(static_cast<ssc_number_t>(annual_energy)));
            assign("percent_complete", var_data((ssc_number_t)percent));

            // resiliency metrics
            if (resilience) {
                resilience->run_surviving_batteries_by_looping(&p_crit_load_full[0], &power_input_lifetime[0]);
                calculate_resilience_outputs(this, resilience);
            }
         }
        else
            assign("average_battery_roundtrip_efficiency", var_data((ssc_number_t)0.));
    }
};

DEFINE_MODULE_ENTRY(battery, "Battery storage standalone model .", 10)
