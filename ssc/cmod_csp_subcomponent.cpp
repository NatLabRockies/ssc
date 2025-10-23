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
#include "csp_solver_two_tank_tes.h"
#include "csp_solver_piston_cylinder_tes.h"
#include "csp_solver_packedbed_tes.h"

// Forward declarations
double C_to_K(double T);
double K_to_C(double T);

static var_info _cm_vtab_csp_subcomponent[] = {
    /* VARTYPE          DATATYPE         NAME                         LABEL                                                                               UNITS           META              GROUP             REQUIRED_IF                CONSTRAINTS         UI_HINTS*/
    // Inputs
    { SSC_INPUT,        SSC_NUMBER,      "t_step",                    "Timestep duration",                                                                "s",            "",               "system",         "*",                       "",                      "" },
    { SSC_INPUT,        SSC_ARRAY,       "T_amb",                     "Ambient temperature",                                                              "C",            "",               "weather",        "*",                       "",                      "" },
    { SSC_INPUT,        SSC_ARRAY,       "mdot_src",                  "Mass flow from heat source",                                                       "kg/s",         "",               "TES",            "*",                       "",                      "" },
    { SSC_INPUT,        SSC_ARRAY,       "mdot_sink",                 "Mass flow to heat sink or power block",                                            "kg/s",         "",               "TES",            "*",                       "",                      "" },
    { SSC_INPUT,        SSC_ARRAY,       "hot_tank_bypassed",         "Is mass flow from source going straight to cold tank?",                            "-",            "",               "TES",            "*",                       "",                      "" },
    { SSC_INPUT,        SSC_ARRAY,       "T_src_out",                 "Temperature from heat source",                                                     "C",            "",               "TES",            "*",                       "",                      "" },
    { SSC_INPUT,        SSC_ARRAY,       "T_sink_out",                "Temperature from heat sink or power block",                                        "C",            "",               "TES",            "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "T_tank_hot_ini",            "Temperature of fluid in hot tank at beginning of step",                            "C",            "",               "TES",            "",                        "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "T_tank_cold_ini",           "Temperature of fluid in cold tank at beginning of step",                           "C",            "",               "TES",            "",                        "",                      "" },

    // General System Parameters
    { SSC_INPUT,        SSC_NUMBER,      "P_ref",                     "Rated plant capacity",                                                             "MWe",          "",               "powerblock",     "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "eta_ref",                   "Power cycle efficiency at design",                                                 "none",         "",               "powerblock",     "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "solar_mult",                "Actual solar multiple of system",                                                  "-",            "",               "system",         "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "T_loop_in_des",             "Design loop inlet temperature",                                                    "C",            "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "T_loop_out",                "Target loop outlet temperature",                                                   "C",            "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "pb_pump_coef",              "Pumping power to move 1kg of HTF through PB loop",                                 "kW/kg",        "",               "powerblock",     "*",                       "",                      "" },

    // General TES Parameters
    { SSC_INPUT,        SSC_NUMBER,      "tes_type",                  "Standard two tank (1), Packed Bed (2), Piston Cylinder (3)",                       "-",            "",               "TES",            "?=1",                     "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "Fluid",                     "Field HTF fluid ID number",                                                        "-",            "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "field_fl_props",            "User defined field fluid property data",                                           "-",            "",               "solar_field",    "*",                       "",                      "" },
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
    { SSC_INPUT,        SSC_NUMBER,      "h_tank_min",                "Minimum allowable HTF height in storage tank",                                     "m",            "",               "TES",            "tes_type=1|tes_type=3",   "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "init_hot_htf_percent",      "Initial fraction of avail. vol that is hot",                                       "%",            "",               "TES",            "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "tes_n_tsteps",              "Number of subtimesteps (for NT and packed bed)",                                   "",             "",               "TES",            "tes_type>1",              "",                      "" },

    // TES
    { SSC_INPUT,        SSC_NUMBER,      "V_tes_des",                 "Design-point velocity to size the TES pipe diameters",                             "m/s",          "",               "controller",     "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "calc_design_pipe_vals",     "Calculate temps and pressures at design conditions for runners and headers",       "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "tes_pump_coef",             "Pumping power to move 1kg of HTF through tes loop",                                "kW/(kg/s)",    "",               "controller",     "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "eta_pump",                  "HTF pump efficiency",                                                              "none",         "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "has_hot_tank_bypass",       "Bypass valve connects field outlet to cold tank",                                  "-",            "",               "controller",     "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "T_tank_hot_inlet_min",      "Minimum hot tank htf inlet temperature",                                           "C",            "",               "controller",     "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "custom_tes_p_loss",         "TES pipe losses are based on custom lengths and coeffs",                           "-",            "",               "controller",     "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "custom_tes_pipe_sizes",     "Use custom TES pipe diams, wallthks, and lengths",                                 "-",            "",               "controller",     "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "k_tes_loss_coeffs",         "Minor loss coeffs for the coll, gen, and bypass loops",                            "-",            "",               "controller",     "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "tes_diams",                 "Custom TES diameters",                                                             "m",            "",               "controller",     "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "tes_wallthicks",            "Custom TES wall thicknesses",                                                      "m",            "",               "controller",     "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "tes_lengths",               "Custom TES lengths",                                                               "m",            "",               "controller",     "",                        "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "HDR_rough",                 "Header pipe roughness",                                                            "m",            "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "DP_SGS",                    "Pressure drop within the steam generator",                                         "bar",          "",               "controller",     "*",                       "",                      "" },

    // TES Two Tank Specific 
    { SSC_INPUT,        SSC_NUMBER,      "store_fluid",               "Material number for storage fluid",                                                "-",            "",               "TES",            "tes_type=1",              "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "store_fl_props",            "User defined storage fluid property data",                                         "-",            "",               "TES",            "tes_type=1",              "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "dt_hot",                    "Hot side HX approach temp",                                                        "C",            "",               "TES",            "tes_type=1",              "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "tanks_in_parallel",         "Tanks are in parallel, not in series, with solar field",                           "-",            "",               "controller",     "tes_type=1",              "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "packed_vol_frac",           "Packed volume fraction",                                                           "-",            "",               "TES",            "*",                       "",                      "" },

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
    { SSC_INPUT,        SSC_ARRAY,       "tes_pb_T_grad_ini",         "TES Temperature gradient at beginning of timestep",                                "C",            "",               "TES",            "?=[-274]",                "",                      "" },

    // TES Piston Cylinder
    { SSC_INPUT,        SSC_NUMBER,      "tes_cyl_tank_thick",         "Tank wall thickness (used for Piston Cylinder)",                                  "m",            "",               "TES",            "tes_type=3",              "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "tes_cyl_tank_cp",            "Tank wall cp (used for Piston Cylinder)",                                         "kJ/kg-K",      "",               "TES",            "tes_type=3",              "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "tes_cyl_tank_dens",          "Tank wall thickness (used for Piston Cylinder)",                                  "kg/m3",        "",               "TES",            "tes_type=3",              "",                      "" },
    { SSC_INPUT,        SSC_ARRAY,       "tes_cyl_piston_loss_poly",   "Polynomial coefficients describing piston heat loss function (f(kg/s)=%)",         "",             "",               "TES",            "tes_type=3",              "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "tes_cyl_tank_insul_percent", "Percent additional wall mass due to insulation (used for Piston Cylinder)",       "%",            "",               "TES",            "?=0",                     "",                      "" },

    // Outputs
    { SSC_OUTPUT,       SSC_ARRAY,       "T_src_in",                  "Temperature to heat source",                                                       "C",            "",               "TES",            "*",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "T_sink_in",                 "Temperature to heat sink or power block",                                          "C",            "",               "TES",            "*",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "T_tank_cold",               "Temperature of cold tank (end of timestep)",                                       "C",            "",               "TES",            "*",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "T_tank_hot",                "Temperature of hot tank (end of timestep)",                                        "C",            "",               "TES",            "*",                       "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "tes_diameter",              "TES Diameter",                                                                     "m",            "",               "TES",            "*",                       "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "tes_radius",                "TES Radius",                                                                       "m",            "",               "TES",            "*",                       "",                      "" },
    { SSC_OUTPUT,       SSC_NUMBER,      "tes_height",                "TES Height",                                                                       "m",            "",               "TES",            "*",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "hot_tank_vol_frac",         "Hot tank volume fraction of total",                                                "",             "",               "TES",            "*",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "q_dot_dc_to_htf",           "Thermal power to HTF from storage",                                                "MWt",          "",               "TES",            "*",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "q_dot_ch_from_htf",         "Thermal power from the HTF to storage",                                            "MWt",          "",               "TES",            "*",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "q_dc_to_htf",               "Thermal energy to HTF from storage",                                               "MJt",          "",               "TES",            "*",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "q_ch_from_htf",             "Thermal energy from the HTF to storage",                                           "MJt",          "",               "TES",            "*",                       "",                      "" },



    { SSC_OUTPUT,       SSC_ARRAY,       "tes_error",                 "TES energy balance error",                                                         "MW",           "",               "TES",            "tes_type=3",              "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "tes_error_percent",         "TES energy balance error percent",                                                 "%",            "",               "TES",            "tes_type=3",              "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "piston_loc",                "Piston Location (distance from left cold side)",                                   "m",            "",               "TES",            "tes_type=3",              "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "piston_frac",               "Piston Fraction (distance from left cold side)",                                   "",             "",               "TES",            "tes_type=3",              "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "tes_leak_error",            "TES energy balance error due to leakage assumption",                               "MWt",          "",               "TES",            "tes_type=3",              "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "tes_E_hot",                 "TES hot side internal energy",                                                     "MJ",           "",               "TES",            "tes_type=3",              "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "tes_E_cold",                "TES cold side internal energy",                                                    "MJ",           "",               "TES",            "tes_type=3",              "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "tes_wall_error",            "TES energy balance error due to wall temperature assumption",                      "MWt",          "",               "TES",            "tes_type=3",              "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "tes_error_corrected",       "TES energy balance error, accounting for wall and temperature assumption error",   "MWt",          "",               "TES",            "tes_type=3",              "",                      "" },
                                                                                                                                                                                            
    { SSC_OUTPUT,       SSC_ARRAY,       "tes_exp_wall_mass",         "TES expansion tank effective wall mass",                                           "kg",           "",               "TES",            "tes_type=3",              "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "tes_exp_length",            "TES expansion tank effective length",                                              "m",            "",               "TES",            "tes_type=3",              "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "tes_mass_cold",             "TES cold fluid mass",                                                              "kg",           "",               "TES",            "tes_type=3",              "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "tes_mass_hot",              "TES hot fluid mass",                                                               "kg",           "",               "TES",            "tes_type=3",              "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "tes_V_cold",                "TES cold fluid volume",                                                            "kg",           "",               "TES",            "tes_type=3",              "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "tes_V_hot",                 "TES hot fluid volume",                                                             "kg",           "",               "TES",            "tes_type=3",              "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "hot_tank_mass_perc",        "TES hot tank mass percent of total (end)",                                         "kg",           "",               "TES",            "*",                       "",                      "" },

    { SSC_OUTPUT,       SSC_MATRIX,      "T_grad_final",              "TES Temperature gradient at end of timestep",                                      "C",            "",               "TES",            "tes_type=2",              "",                      "" },


    var_info_invalid };

class cm_csp_subcomponent : public compute_module
{
public:
    cm_csp_subcomponent()
    {
        add_var_info(_cm_vtab_csp_subcomponent);
    }

    void exec()
    {
        int tes_type = as_integer("tes_type");

        util::matrix_t<double> tes_lengths;
        if (is_assigned("tes_lengths")) {
            tes_lengths = as_matrix("tes_lengths");               //[m]
        }
        if (!is_assigned("tes_lengths") || tes_lengths.ncells() < 11) {
            double vals1[11] = { 0., 90., 100., 120., 0., 30., 90., 80., 80., 120., 80. };
            tes_lengths.assign(vals1, 11);
        }


        C_csp_tes* storage_pointer;
        C_csp_two_tank_tes storage_two_tank;
        C_csp_piston_cylinder_tes storage_cyl;
        C_csp_packedbed_tes storage_packedbed;

        double P_ref = as_double("P_ref");
        double eta_ref = as_double("eta_ref");
        double tshours = as_double("tshours");

        // Two Tank
        if (tes_type == C_csp_tes::csp_tes_types::E_TES_TWO_TANK)
        {
            double T_tank_hot_ini = is_assigned("T_tank_hot_ini") == true ? as_double("T_tank_hot_ini") : as_double("T_loop_out");
            double T_tank_cold_ini = is_assigned("T_tank_cold_ini") == true ? as_double("T_tank_cold_ini") : as_double("T_loop_in_des");

            double h_tank_in = is_assigned("h_tank_in") == true ? as_double("h_tank_in") : std::numeric_limits<double>::quiet_NaN();
            double d_tank_in = is_assigned("d_tank_in") == true ? as_double("d_tank_in") : std::numeric_limits<double>::quiet_NaN();

            storage_two_tank = C_csp_two_tank_tes(
                as_integer("Fluid"),                                                // [-] field fluid identifier
                as_matrix("field_fl_props"),                                        // [-] field fluid properties
                as_integer("store_fluid"),                                          // [-] tes fluid identifier
                as_matrix("store_fl_props"),                                        // [-] tes fluid properties
                as_double("P_ref") / as_double("eta_ref"),                          // [MWt] Design heat rate in and out of tes
                as_double("solar_mult"),                                            // [-] the max design heat rate as a fraction of the nominal
                as_double("P_ref") / as_double("eta_ref") * as_double("tshours"),   // [MWt-hr] design storage capacity
                as_boolean("is_h_tank_fixed"),                                      // Use input height
                h_tank_in,                                                          // [m] tank height input
                d_tank_in,                                                          // [m] tank diameter input
                as_double("u_tank"),                                                // [W/m^2-K]
                as_integer("tank_pairs"),                                           // [-]
                as_double("hot_tank_Thtr"),                                         // [C] convert to K in init()
                as_double("hot_tank_max_heat"),                                     // [MW]
                as_double("cold_tank_Thtr"),                                        // [C] convert to K in init()
                as_double("cold_tank_max_heat"),                                    // [MW]
                as_double("dt_hot"),                                                // [C] Temperature difference across heat exchanger - assume hot and cold deltaTs are equal
                as_double("T_loop_in_des"),                                         // [C] convert to K in init()
                as_double("T_loop_out"),                                            // [C] convert to K in init()
                T_tank_hot_ini,                                                     // [C] Initial temperature in hot storage tank
                T_tank_cold_ini,                                                    // [C] Initial temperature in cold storage cold
                as_double("h_tank_min"),                                            // [m] Minimum allowable HTF height in storage tank
                as_double("init_hot_htf_percent"),                                  // [%] Initial fraction of available volume that is hot
                as_double("pb_pump_coef"),                                          // [kW/kg/s] Pumping power to move 1 kg/s of HTF through power cycle
                as_boolean("tanks_in_parallel"),                                    // [-] Whether the tanks are in series or parallel with the solar field. Series means field htf must go through storage tanks.
                as_double("packed_vol_frac"),                                       // [-] Packed volume fraction
                as_double("V_tes_des"),                                             // [m/s] Design-point velocity for sizing the diameters of the TES piping
                as_boolean("calc_design_pipe_vals"),                                // [-] Should the HTF state be calculated at design conditions
                as_double("tes_pump_coef"),                                         // [kW/kg/s] Pumping power to move 1 kg/s of HTF through tes loop
                as_double("eta_pump"),                                              // [-] Pump efficiency, for newer pumping calculations
                as_boolean("has_hot_tank_bypass"),                                  // [-] True if the bypass valve causes the field htf to bypass just the hot tank and enter the cold tank before flowing back to the field.
                as_double("T_tank_hot_inlet_min"),                                  // [C] Minimum field htf temperature that may enter the hot tank
                as_boolean("custom_tes_p_loss"),                                    // [-] True if the TES piping losses should be calculated using the TES pipe lengths and minor loss coeffs, false if using the pumping loss parameters
                as_boolean("custom_tes_pipe_sizes"),                                // [-] True if the TES diameters and wall thicknesses parameters should be used instead of calculating them
                as_matrix("k_tes_loss_coeffs"),                                     // [-] Combined minor loss coefficients of the fittings and valves in the collection (including bypass) and generation loops in the TES 
                as_matrix("tes_diams"),                                             // [m] Imported inner diameters for the TES piping as read from the modified output files
                as_matrix("tes_wallthicks"),                                        // [m] Imported wall thicknesses for the TES piping as read from the modified output files
                tes_lengths,                                                        // [m] Imported lengths for the TES piping as read from the modified output files
                as_double("HDR_rough"),                                             // [m] Pipe absolute roughness
                as_double("DP_SGS")                                                 // [bar] Pressure drop on the TES discharge side (e.g., within the steam generator)
            );

            storage_pointer = &storage_two_tank;
        }
        // Packed Bed
        else if (tes_type == C_csp_tes::csp_tes_types::E_TES_PACKED_BED)
        {
            if (as_double("tes_pb_cp_solid") > 5)
            {
                log("Be sure the packed bed solid media cp is in kJ/kg K", SSC_WARNING);
            }

            storage_packedbed = C_csp_packedbed_tes(
                as_integer("Fluid"),                                                // [-] field fluid identifier
                as_matrix("field_fl_props"),                                        // [-] field fluid properties
                as_double("P_ref") / as_double("eta_ref"),                          // [MWt] Design heat rate in and out of tes
                as_double("P_ref") / as_double("eta_ref") * as_double("tshours"),   // [MWt-hr] design storage capacity
                as_integer("is_h_tank_fixed"),                                      // [] Sizing Method (0) use fixed diameter, (1) use fixed height, (2) use preset inputs
                as_double("h_tank_in"),                                             // [m] Tank height
                as_double("d_tank_in"),                                             // [m] Tank diameter
                as_double("tes_pb_f_oversize"),                                     // [] Oversize factor
                as_double("T_loop_in_des"),                                         // [C] Cold design temperature
                as_double("T_loop_out"),                                            // [C] hot design temperature
                // Check initialization variables
                (is_assigned("T_tank_hot_ini")) ? as_double("T_tank_hot_ini") : as_double("T_loop_out"),
                (is_assigned("T_tank_cold_ini")) ? as_double("T_tank_cold_ini") : as_double("T_loop_in_des"),
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

            if (is_assigned("tes_pb_T_grad_ini"))
            {
                vector<double> T_grad_ini = as_vector_double("tes_pb_T_grad_ini");
                if (T_grad_ini.size() > 1 && T_grad_ini[0] != -274)
                {
                    if (T_grad_ini.size() != as_integer("tes_pb_n_xsteps") + 1)
                    {
                        throw exec_error("csp_subcomponent", "Initial temperature gradient should be length tes_pb_n_xsteps + 1");
                    }
                    else
                    {
                        storage_packedbed.set_T_grad_init(T_grad_ini);
                    }
                }
            }

            storage_pointer = &storage_packedbed;

        }
        // Piston Cylinder
        else if (tes_type == C_csp_tes::csp_tes_types::E_TES_CYL)
        {

            int nstep = as_integer("tes_n_tsteps");

            bool custom_tes_pipe_sizes = as_boolean("custom_tes_pipe_sizes");
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

            bool tanks_in_parallel = as_boolean("tanks_in_parallel");
            if (tanks_in_parallel == false)
            {
                throw exec_error("csp_subcomponent", "TES model requires tanks in parallel");
            }

            // Modify wall density to account for insulation mass
            double mass_factor = 1.0 + (0.01 * as_double("tes_cyl_tank_insul_percent"));
            double dens_orig = as_double("tes_cyl_tank_dens");
            double dens_w_insulation = dens_orig * mass_factor;

            double T_tank_hot_ini = is_assigned("T_tank_hot_ini") == true ? as_double("T_tank_hot_ini") : as_double("T_loop_out");
            double T_tank_cold_ini = is_assigned("T_tank_cold_ini") == true ? as_double("T_tank_cold_ini") : as_double("T_loop_in_des");

            double h_tank_in = is_assigned("h_tank_in") == true ? as_double("h_tank_in") : std::numeric_limits<double>::quiet_NaN();
            double d_tank_in = is_assigned("d_tank_in") == true ? as_double("d_tank_in") : std::numeric_limits<double>::quiet_NaN();

            storage_cyl = C_csp_piston_cylinder_tes(
                as_integer("Fluid"),                                                // [-] field fluid identifier
                as_matrix("field_fl_props"),                                        // [-] field fluid properties
                //as_integer("store_fluid"),                                          // [-] tes fluid identifier
                //as_matrix("store_fl_props"),                                        // [-] tes fluid properties
                as_double("P_ref") / as_double("eta_ref"),                          // [MWt] Design heat rate in and out of tes
                as_double("solar_mult"),                                            // [-] the max design heat rate as a fraction of the nominal
                as_double("P_ref") / as_double("eta_ref") * as_double("tshours"),   // [MWt-hr] design storage capacity
                as_boolean("is_h_tank_fixed"),                                      // Use input height
                h_tank_in,                                                          // [m] tank height input
                d_tank_in,                                                          // [m] tank diameter input
                as_double("u_tank"),                                                // [W/m^2-K]
                as_integer("tank_pairs"),                                           // [-]
                as_double("hot_tank_Thtr"),                                         // [C] convert to K in init()
                as_double("hot_tank_max_heat"),                                     // [MW]
                as_double("cold_tank_Thtr"),                                        // [C] convert to K in init()
                as_double("cold_tank_max_heat"),                                    // [MW]
                as_double("T_loop_in_des"),                                         // [C] convert to K in init()
                as_double("T_loop_out"),                                            // [C] convert to K in init()
                T_tank_hot_ini,                                                     // [C] Initial temperature in hot storage tank
                T_tank_cold_ini,                                                    // [C] Initial temperature in cold storage cold
                as_double("h_tank_min"),                                            // [m] Minimum allowable HTF height in storage tank
                as_double("init_hot_htf_percent"),                                  // [%] Initial fraction of available volume that is hot
                as_double("pb_pump_coef"),                                          // [kW/kg/s] Pumping power to move 1 kg/s of HTF through power cycle
                as_double("tes_cyl_tank_cp") * 1000,                                 // convert to J/kgK
                dens_w_insulation,                                                  // Tank Wall density
                as_double("tes_cyl_tank_thick"),                                     // Tank wall thickness
                nstep,                                                              // Number subtimesteps
                as_vector_double("tes_cyl_piston_loss_poly"),                        // Leakage polynomial (%)
                as_double("V_tes_des"),                                             // [m/s] Design-point velocity for sizing the diameters of the TES piping
                as_boolean("calc_design_pipe_vals"),                                // [-] Should the HTF state be calculated at design conditions
                as_double("tes_pump_coef"),                                         // [kW/kg/s] Pumping power to move 1 kg/s of HTF through tes loop
                as_double("eta_pump"),                                              // [-] Pump efficiency, for newer pumping calculations
                as_boolean("has_hot_tank_bypass"),                                  // [-] True if the bypass valve causes the field htf to bypass just the hot tank and enter the cold tank before flowing back to the field.
                as_double("T_tank_hot_inlet_min"),                                  // [C] Minimum field htf temperature that may enter the hot tank
                false,                                                              // [-] True if the TES piping losses should be calculated using the TES pipe lengths and minor loss coeffs, false if using the pumping loss parameters
                false,                                                              // [-] True if the TES diameters and wall thicknesses parameters should be used instead of calculating them
                as_matrix("k_tes_loss_coeffs"),                                     // [-] Combined minor loss coefficients of the fittings and valves in the collection (including bypass) and generation loops in the TES 
                tes_diams,                                                          // [m] Imported inner diameters for the TES piping as read from the modified output files
                tes_wallthicks,                                                     // [m] Imported wall thicknesses for the TES piping as read from the modified output files
                tes_lengths,                                                        // [m] Imported lengths for the TES piping as read from the modified output files
                as_double("HDR_rough"),                                             // [m] Pipe absolute roughness
                as_double("DP_SGS")                                                 // [bar] Pressure drop on the TES discharge side (e.g., within the steam generator)
            );

            storage_pointer = &storage_cyl;
        }
        else
        {
            throw exec_error("csp_subcomponent", "tes_type must be 1-3");
        }

        // Initialization   -> this is necessary to fully instantiate the TES
        C_csp_tes::S_csp_tes_init_inputs init_inputs;
        init_inputs.T_to_cr_at_des = C_to_K(as_double("T_loop_in_des"));        // [K]
        init_inputs.T_from_cr_at_des = C_to_K(as_double("T_loop_out"));         // [K]
        init_inputs.P_to_cr_at_des = 19.64;                                     // [bar]
        storage_pointer->init(init_inputs);

        // Get inputs
        double t_step = as_double("t_step");
        std::vector<double> T_amb = as_vector_double("T_amb");
        std::vector<double> mdot_src = as_vector_double("mdot_src");
        std::vector<double> mdot_sink = as_vector_double("mdot_sink");
        std::vector<double> hot_tank_bypassed = as_vector_double("hot_tank_bypassed");
        std::vector<double> T_src_out = as_vector_double("T_src_out");
        std::vector<double> T_sink_out = as_vector_double("T_sink_out");

        // Verify inputs
        size_t n_steps = T_amb.size();
        if (  mdot_src.size() != n_steps ||
              mdot_sink.size() != n_steps ||
              T_src_out.size() != n_steps ||
              T_sink_out.size() != n_steps) {
            throw exec_error("csp_subcomponent", "Input arrays not equal in size.");
        }

        // Get Design Point Outputs
        double V_tes_htf_avail_calc /*m3*/, V_tes_htf_total_calc /*m3*/,
            h_tank_calc /*m*/, d_tank_calc /*m*/, q_dot_loss_tes_des_calc /*MWt*/, dens_store_htf_at_T_ave_calc /*kg/m3*/,
            Q_tes_des_calc /*MWt-hr*/;

        if (tes_type == C_csp_tes::csp_tes_types::E_TES_TWO_TANK)
        {
            storage_two_tank.get_design_parameters(V_tes_htf_avail_calc, V_tes_htf_total_calc,
                h_tank_calc, d_tank_calc, q_dot_loss_tes_des_calc, dens_store_htf_at_T_ave_calc, Q_tes_des_calc);
        }
        if (tes_type == C_csp_tes::csp_tes_types::E_TES_PACKED_BED)
        {
            storage_packedbed.get_design_parameters(V_tes_htf_avail_calc, V_tes_htf_total_calc,
                h_tank_calc, d_tank_calc, q_dot_loss_tes_des_calc, dens_store_htf_at_T_ave_calc, Q_tes_des_calc);
        }
        else if (tes_type == C_csp_tes::csp_tes_types::E_TES_CYL)
        {
            storage_cyl.get_design_parameters(V_tes_htf_avail_calc, V_tes_htf_total_calc,
                h_tank_calc, d_tank_calc, q_dot_loss_tes_des_calc, dens_store_htf_at_T_ave_calc, Q_tes_des_calc);
        }

        // Allocate outputs
        double* T_src_in = allocate("T_src_in", n_steps);
        double* T_sink_in = allocate("T_sink_in", n_steps);
        double* T_tank_cold = allocate("T_tank_cold", n_steps);
        double* T_tank_hot = allocate("T_tank_hot", n_steps);
        double* hot_tank_vol_frac = allocate("hot_tank_vol_frac", n_steps);
        double* W_dot_elec_in_tot = allocate("W_dot_elec_in_tot", n_steps);
        double* hot_tank_mass_perc = allocate("hot_tank_mass_perc", n_steps);
        double* exp_wall_mass = allocate("tes_exp_wall_mass", n_steps);
        double* exp_length = allocate("tes_exp_length", n_steps);
        double* mass_hot = allocate("tes_mass_hot", n_steps);
        double* mass_cold = allocate("tes_mass_cold", n_steps);
        double* V_hot = allocate("tes_V_hot", n_steps);
        double* V_cold = allocate("tes_V_cold", n_steps);
        double* q_dot_dc_to_htf = allocate("q_dot_dc_to_htf", n_steps);
        double* q_dot_ch_from_htf = allocate("q_dot_ch_from_htf", n_steps);
        double* q_dc_to_htf = allocate("q_dc_to_htf", n_steps);
        double* q_ch_from_htf = allocate("q_ch_from_htf", n_steps);

        vector<double> piston_loc_vec;
        vector<double> piston_frac_vec;
        vector<double> T_hot_calc_vec;
        vector<double> T_cold_calc_vec;
        vector<double> tes_error_vec;
        vector<double> tes_error_percent_vec;
        vector<double> tes_error_hot_vec;
        vector<double> tes_error_cold_vec;
        vector<double> tes_error_leakage_vec;
        vector<double> tes_E_hot_vec;
        vector<double> tes_E_cold_vec;
        vector<double> tes_wall_error_vec;
        vector<double> tes_error_corrected_vec;
        util::matrix_t<ssc_number_t> tes_T_grad_mat;

        if (tes_type == C_csp_tes::csp_tes_types::E_TES_PACKED_BED)
        {
            tes_T_grad_mat.resize(n_steps, as_integer("tes_pb_n_xsteps") + 1);
        }

        // Simulate
        for (size_t i = 0; i < n_steps; i++) {
            double mdot_src_to_hot_tank = hot_tank_bypassed.at(i) ? 0. : mdot_src.at(i);
            double mdot_src_to_cold_tank = hot_tank_bypassed.at(i) ? mdot_src.at(i) : 0.;
            double T_src_in_K, T_sink_in_K;
            C_csp_tes::S_csp_tes_outputs tes_outputs;
            int result = storage_pointer->solve_tes_off_design(
                t_step,                         /*s*/
                C_to_K(T_amb.at(i)),            /*K*/
                mdot_src_to_hot_tank,           /*kg/s*/
                mdot_sink.at(i),                /*kg/s*/
                mdot_src_to_cold_tank,          /*kg/s*/
                C_to_K(T_src_out.at(i)),        /*K*/
                C_to_K(T_sink_out.at(i)),       /*K*/
                // Outputs:
                T_sink_in_K,                    /*K*/
                T_src_in_K,                     /*K*/
                tes_outputs);
            storage_pointer->converged();

            // Set outputs
            T_src_in[i] = K_to_C(T_src_in_K);
            T_sink_in[i] = K_to_C(T_sink_in_K);
            T_tank_cold[i] = K_to_C(storage_pointer->get_cold_temp());
            T_tank_hot[i] = K_to_C(storage_pointer->get_hot_temp());
            assign("tes_diameter", d_tank_calc);
            assign("tes_radius", d_tank_calc / 2.0);
            assign("tes_height", h_tank_calc);
            q_dot_dc_to_htf[i] = tes_outputs.m_q_dot_dc_to_htf; //[MWt]
            q_dot_ch_from_htf[i] = tes_outputs.m_q_dot_ch_from_htf; //[MWt]
            q_dc_to_htf[i] = tes_outputs.m_q_dot_dc_to_htf * t_step; //[MJt]
            q_ch_from_htf[i] = tes_outputs.m_q_dot_ch_from_htf * t_step; //[MJt]


            hot_tank_vol_frac[i] = storage_pointer->get_hot_tank_vol_frac();
            

            // Add NT specific outputs
            if (tes_type == C_csp_tes::csp_tes_types::E_TES_CYL)
            {
                double piston_location, piston_fraction;
                storage_cyl.calc_piston_location(piston_location, piston_fraction);

                piston_loc_vec.push_back(piston_location);
                piston_frac_vec.push_back(piston_fraction);

                double tes_error = storage_cyl.mc_reported_outputs.value(C_csp_piston_cylinder_tes::E_ERROR);
                double tes_error_percent = storage_cyl.mc_reported_outputs.value(C_csp_piston_cylinder_tes::E_ERROR_PERCENT);
                double tes_error_leak = storage_cyl.mc_reported_outputs.value(C_csp_piston_cylinder_tes::E_LEAK_ERROR);
                double tes_E_hot = storage_cyl.mc_reported_outputs.value(C_csp_piston_cylinder_tes::E_E_HOT);
                double tes_E_cold = storage_cyl.mc_reported_outputs.value(C_csp_piston_cylinder_tes::E_E_COLD);
                double tes_wall_error = storage_cyl.mc_reported_outputs.value(C_csp_piston_cylinder_tes::E_WALL_ERROR);
                double tes_error_corrected = storage_cyl.mc_reported_outputs.value(C_csp_piston_cylinder_tes::E_ERROR_CORRECTED);

                tes_error_vec.push_back(tes_error);
                tes_error_percent_vec.push_back(tes_error_percent);
                tes_error_leakage_vec.push_back(tes_error_leak);
                tes_E_hot_vec.push_back(tes_E_hot);
                tes_E_cold_vec.push_back(tes_E_cold);
                tes_wall_error_vec.push_back(tes_wall_error);
                tes_error_corrected_vec.push_back(tes_error_corrected);

                hot_tank_mass_perc[i] = storage_cyl.mc_reported_outputs.value(C_csp_piston_cylinder_tes::E_HOT_TANK_HTF_PERC_FINAL);
                exp_wall_mass[i] = storage_cyl.mc_reported_outputs.value(C_csp_piston_cylinder_tes::E_EXP_WALL_MASS);
                exp_length[i] = storage_cyl.mc_reported_outputs.value(C_csp_piston_cylinder_tes::E_EXP_LENGTH);
                mass_hot[i] = storage_cyl.mc_reported_outputs.value(C_csp_piston_cylinder_tes::E_MASS_HOT_TANK);
                mass_cold[i] = storage_cyl.mc_reported_outputs.value(C_csp_piston_cylinder_tes::E_MASS_COLD_TANK);
                V_cold[i] = storage_cyl.mc_reported_outputs.value(C_csp_piston_cylinder_tes::E_VOL_COLD);
                V_hot[i] = storage_cyl.mc_reported_outputs.value(C_csp_piston_cylinder_tes::E_VOL_HOT);
            }    

            // Add packed bed specific outputs
            if(tes_type == C_csp_tes::csp_tes_types::E_TES_PACKED_BED)
            {
                std::vector<double> T_prev_vec = storage_packedbed.get_T_prev_vec();
                for (int j = 0; j < T_prev_vec.size(); j++)
                {
                    tes_T_grad_mat.at(i, j) = T_prev_vec[j] - 273.15;   // [C] Convert from K
                }
            }

        }

        if (tes_type == C_csp_tes::csp_tes_types::E_TES_CYL)
        {
            set_vector("piston_loc", piston_loc_vec);
            set_vector("piston_frac", piston_frac_vec);
            set_vector("T_hot_calc", T_hot_calc_vec);
            set_vector("T_cold_calc", T_cold_calc_vec);
            set_vector("tes_error", tes_error_vec);
            set_vector("tes_error_percent", tes_error_percent_vec);
            set_vector("tes_leak_error", tes_error_leakage_vec);
            set_vector("tes_E_hot", tes_E_hot_vec);
            set_vector("tes_E_cold", tes_E_cold_vec);
            set_vector("tes_wall_error", tes_wall_error_vec);
            set_vector("tes_error_corrected", tes_error_corrected_vec);
        }
        

        if (tes_type == C_csp_tes::csp_tes_types::E_TES_PACKED_BED)
        {
            assign("T_grad_final", tes_T_grad_mat);
        }

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

double C_to_K(double T) {
    return T + 273.15;
}

double K_to_C(double T) {
    return T - 273.15;
}

DEFINE_MODULE_ENTRY(csp_subcomponent, "CSP subcomponents", 1)
