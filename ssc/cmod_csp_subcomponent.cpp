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

    // TES
    { SSC_INPUT,        SSC_NUMBER,      "Fluid",                     "Field HTF fluid ID number",                                                        "-",            "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "field_fl_props",            "User defined field fluid property data",                                           "-",            "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "store_fluid",               "Material number for storage fluid",                                                "-",            "",               "TES",            "*",                       "",                      "" },
    { SSC_INPUT,        SSC_MATRIX,      "store_fl_props",            "User defined storage fluid property data",                                         "-",            "",               "TES",            "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "P_ref",                     "Rated plant capacity",                                                             "MWe",          "",               "powerblock",     "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "eta_ref",                   "Power cycle efficiency at design",                                                 "none",         "",               "powerblock",     "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "solar_mult",                "Actual solar multiple of system",                                                  "-",            "",               "system",         "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "tshours",                   "Equivalent full-load thermal storage hours",                                       "hr",           "",               "TES",            "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "h_tank",                    "Total height of tank (height of HTF when tank is full",                            "m",            "",               "TES",            "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "u_tank",                    "Loss coefficient from the tank",                                                   "W/m2-K",       "",               "TES",            "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "tank_pairs",                "Number of equivalent tank pairs",                                                  "-",            "",               "TES",            "*",                       "INTEGER",               "" },
    { SSC_INPUT,        SSC_NUMBER,      "hot_tank_Thtr",             "Minimum allowable hot tank HTF temp",                                              "C",            "",               "TES",            "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "hot_tank_max_heat",         "Rated heater capacity for hot tank heating",                                       "MWe",          "",               "TES",            "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "cold_tank_Thtr",            "Minimum allowable cold tank HTF temp",                                             "C",            "",               "TES",            "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "packed_vol_frac",           "Packed volume fraction",                                                           "-",            "",               "TES",            "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "cold_tank_max_heat",        "Rated heater capacity for cold tank heating",                                      "MWe",          "",               "TES",            "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "dt_hot",                    "Hot side HX approach temp",                                                        "C",            "",               "TES",            "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "T_loop_in_des",             "Design loop inlet temperature",                                                    "C",            "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "T_loop_out",                "Target loop outlet temperature",                                                   "C",            "",               "solar_field",    "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "h_tank_min",                "Minimum allowable HTF height in storage tank",                                     "m",            "",               "TES",            "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "init_hot_htf_percent",      "Initial fraction of avail. vol that is hot",                                       "%",            "",               "TES",            "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "pb_pump_coef",              "Pumping power to move 1kg of HTF through PB loop",                                 "kW/kg",        "",               "powerblock",     "*",                       "",                      "" },
    { SSC_INPUT,        SSC_NUMBER,      "tanks_in_parallel",         "Tanks are in parallel, not in series, with solar field",                           "-",            "",               "controller",     "*",                       "",                      "" },
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

    // Outputs
    { SSC_OUTPUT,       SSC_ARRAY,       "T_src_in",                  "Temperature to heat source",                                                       "C",            "",               "TES",            "*",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "T_sink_in",                 "Temperature to heat sink or power block",                                          "C",            "",               "TES",            "*",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "T_tank_cold",               "Temperature of cold tank (average)",                                               "C",            "",               "TES",            "*",                       "",                      "" },
    { SSC_OUTPUT,       SSC_ARRAY,       "T_tank_hot",                "Temperature of hot tank (average)",                                                "C",            "",               "TES",            "*",                       "",                      "" },

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
        util::matrix_t<double> tes_lengths;
        if (is_assigned("tes_lengths")) {
            tes_lengths = as_matrix("tes_lengths");               //[m]
        }
        if (!is_assigned("tes_lengths") || tes_lengths.ncells() < 11) {
            double vals1[11] = { 0., 90., 100., 120., 0., 30., 90., 80., 80., 120., 80. };
            tes_lengths.assign(vals1, 11);
        }
        C_csp_two_tank_tes tes(
            as_integer("Fluid"),                                                // [-] field fluid identifier
            as_matrix("field_fl_props"),                                        // [-] field fluid properties
            as_integer("store_fluid"),                                          // [-] tes fluid identifier
            as_matrix("store_fl_props"),                                        // [-] tes fluid properties
            as_double("P_ref") / as_double("eta_ref"),                          // [MWt] Design heat rate in and out of tes
            as_double("solar_mult"),                                            // [-] the max design heat rate as a fraction of the nominal
            as_double("P_ref") / as_double("eta_ref") * as_double("tshours"),   // [MWt-hr] design storage capacity
            as_double("h_tank"),                                                // [m] tank height
            as_double("u_tank"),                                                // [W/m^2-K]
            as_integer("tank_pairs"),                                           // [-]
            as_double("hot_tank_Thtr"),                                         // [C] convert to K in init()
            as_double("hot_tank_max_heat"),                                     // [MW]
            as_double("cold_tank_Thtr"),                                        // [C] convert to K in init()
            as_double("cold_tank_max_heat"),                                    // [MW]
            as_double("dt_hot"),                                                // [C] Temperature difference across heat exchanger - assume hot and cold deltaTs are equal
            as_double("T_loop_in_des"),                                         // [C] convert to K in init()
            as_double("T_loop_out"),                                            // [C] convert to K in init()
            as_double("T_loop_out"),                                            // [C] Initial temperature in hot storage tank
            as_double("T_loop_in_des"),                                         // [C] Initial temperature in cold storage cold
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

        // Initialization   -> this is necessary to fully instantiate the TES
        C_csp_tes::S_csp_tes_init_inputs init_inputs;
        init_inputs.T_to_cr_at_des = C_to_K(as_double("T_loop_in_des"));        // [K]
        init_inputs.T_from_cr_at_des = C_to_K(as_double("T_loop_out"));         // [K]
        init_inputs.P_to_cr_at_des = 19.64;                                     // [bar]
        tes.init(init_inputs);

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

        // Allocate outputs
        double* T_src_in = allocate("T_src_in", n_steps);
        double* T_sink_in = allocate("T_sink_in", n_steps);
        double* T_tank_cold = allocate("T_tank_cold", n_steps);
        double* T_tank_hot = allocate("T_tank_hot", n_steps);

        // Simulate
        for (size_t i = 0; i < n_steps; i++) {
            double mdot_src_to_hot_tank = hot_tank_bypassed.at(i) ? 0. : mdot_src.at(i);
            double mdot_src_to_cold_tank = hot_tank_bypassed.at(i) ? mdot_src.at(i) : 0.;
            double T_src_in_K, T_sink_in_K;
            C_csp_tes::S_csp_tes_outputs tes_outputs;
            int result = tes.solve_tes_off_design(
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
            tes.converged();

            // Set outputs
            T_src_in[i] = K_to_C(T_src_in_K);
            T_sink_in[i] = K_to_C(T_sink_in_K);
            T_tank_cold[i] = K_to_C(tes.get_cold_temp());
            T_tank_hot[i] = K_to_C(tes.get_hot_temp());
        }
    }
};

double C_to_K(double T) {
    return T + 273.15;
}

double K_to_C(double T) {
    return T - 273.15;
}

DEFINE_MODULE_ENTRY(csp_subcomponent, "CSP subcomponents", 1)
