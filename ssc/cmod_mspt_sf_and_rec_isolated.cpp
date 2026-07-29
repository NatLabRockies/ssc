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


#include "core.h"

#include "csp_solver_mspt_collector_receiver.h"
#include "csp_solver_mspt_receiver.h"
#include "csp_solver_mspt_receiver_222.h"

#include <algorithm>

static var_info _cm_vtab_mspt_sf_and_rec_isolated[] = {

    // Simulation options
    { SSC_INPUT,  SSC_NUMBER, "sim_type",                           "1 (default): timeseries, 2: design only",                                                               "",             "",              "Simulation",                               "?=1",                                "",              "SIMULATION_PARAMETER"},

    // Receiver design parameters
    { SSC_INPUT,  SSC_NUMBER, "q_dot_rec_des",                      "Receiver thermal power to HTF at design",                                                               "MWt",          "",              "Tower and Receiver",                       "*",                                  "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "T_htf_cold_des",                     "Cold HTF inlet temperature at design conditions",                                                       "C",            "",              "Tower and Receiver",                       "*",                                  "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "T_htf_hot_des",                      "Hot HTF outlet temperature at design conditions",                                                       "C",            "",              "Tower and Receiver",                       "*",                                  "",              ""},
    { SSC_INOUT,  SSC_NUMBER, "h_tower",                            "Tower height",                                                                                          "m",            "",              "Tower and Receiver",                       "*",                                  "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "rec_height",                         "Receiver height",                                                                                       "m",            "",              "Tower and Receiver",                       "*",                                  "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "D_rec",                              "The overall outer diameter of the receiver",                                                            "m",            "",              "Tower and Receiver",                       "*",                                  "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "N_panels",                           "Number of individual panels on the receiver",                                                           "",             "",              "Tower and Receiver",                       "*",                                  "INTEGER",       ""},
    { SSC_INPUT,  SSC_NUMBER, "d_tube_out",                         "The outer diameter of an individual receiver tube",                                                     "mm",           "",              "Tower and Receiver",                       "*",                                  "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "th_tube",                            "The wall thickness of a single receiver tube",                                                          "mm",           "",              "Tower and Receiver",                       "*",                                  "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "mat_tube",                           "Receiver tube material, 2=Stainless AISI316",                                                           "",             "",              "Tower and Receiver",                       "*",                                  "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "rec_htf",                            "Receiver HTF, 17=Salt (60% NaNO3, 40% KNO3) 10=Salt (46.5% LiF 11.5% NaF 42% KF) 50=Lookup tables",     "",             "",              "Tower and Receiver",                       "*",                                  "",              ""},
    { SSC_INPUT,  SSC_MATRIX, "field_fl_props",                     "User defined field fluid property data",                                                                "-",            "",              "Tower and Receiver",                       "*",                                  "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "Flow_type",                          "Receiver flow pattern: see figure on SAM Receiver page",                                                "",             "",              "Tower and Receiver",                       "*",                                  "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "crossover_shift",                    "Number of panels shift in receiver crossover position",                                                 "",             "",              "Tower and Receiver",                       "?=0",                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "epsilon",                            "The emissivity of the receiver surface coating",                                                        "",             "",              "Tower and Receiver",                       "*",                                  "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "hl_ffact",                           "The heat loss factor (thermal loss fudge factor)",                                                      "",             "",              "Tower and Receiver",                       "*",                                  "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "f_rec_min",                          "Minimum receiver mass flow rate turn down fraction",                                                    "",             "",              "Tower and Receiver",                       "*",                                  "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "rec_su_delay",                       "Fixed startup delay time for the receiver",                                                             "hr",           "",              "Tower and Receiver",                       "*",                                  "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "rec_qf_delay",                       "Energy-based receiver startup delay (fraction of rated thermal power)",                                 "",             "",              "Tower and Receiver",                       "*",                                  "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "csp.pt.rec.max_oper_frac",           "Maximum receiver mass flow rate fraction",                                                              "",             "",              "Tower and Receiver",                       "*",                                  "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "eta_pump",                           "Receiver HTF pump efficiency",                                                                          "",             "",              "Tower and Receiver",                       "*",                                  "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "piping_length_mult",                 "Piping length multiplier",                                                                              "",             "",              "Tower and Receiver",                       "*",                                  "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "piping_length_const",                "Piping constant length",                                                                                "m",            "",              "Tower and Receiver",                       "*",                                  "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "piping_loss_coefficient",            "Thermal loss per meter of piping",                                                                      "Wt/m2-K",      "",              "Tower and Receiver",                       "",                                   "",              ""},

    // Transient receiver parameters
    { SSC_INPUT,  SSC_NUMBER, "is_rec_model_trans",                 "Formulate receiver model as transient?",                                                                "",             "",              "Tower and Receiver",                       "?=0",                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "rec_tm_mult",                        "Receiver thermal mass multiplier",                                                                      "",             "",              "Tower and Receiver",                       "is_rec_model_trans=1",               "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "riser_tm_mult",                      "Riser thermal mass multiplier",                                                                         "",             "",              "Tower and Receiver",                       "is_rec_model_trans=1",               "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "downc_tm_mult",                      "Downcomer thermal mass multiplier",                                                                     "",             "",              "Tower and Receiver",                       "is_rec_model_trans=1",               "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "u_riser",                            "Design point HTF velocity in riser",                                                                    "m/s",          "",              "Tower and Receiver",                       "is_rec_model_trans=1",               "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "th_riser",                           "Riser or downcomer tube wall thickness",                                                                "mm",           "",              "Tower and Receiver",                       "is_rec_model_trans=1",               "",              ""},


    // Solution modes/configurations
    { SSC_INPUT,  SSC_NUMBER, "is_rec_clearsky_control",            "0: use measured dni, 1: use clear-sky control w/ rec_clearsky_frac input",                              "",             "",              "Tower and Receiver",                       "?=0",                                "",              ""},
    { SSC_INPUT,  SSC_NUMBER, "rec_clearsky_fraction",              "Weighting fraction on clear-sky DNI for receiver flow control",                                         "",             "",              "Receiver control",                         "is_rec_clearsky_control=1",          "",            ""},


    // ***********************************
    // Timeseries inputs
        // Time
    { SSC_INPUT, SSC_ARRAY,   "timestep_od",                        "Timestep",                                                                                              "s",            "",              "Timeseries",                               "sim_type=1",                         "",              ""},
        // Weather
    { SSC_INPUT, SSC_ARRAY,   "P_amb_od",                           "Ambient pressure",                                                                                      "mbar",         "",              "weather",                                  "sim_type=1",                         "",              ""},
    { SSC_INPUT, SSC_ARRAY,   "T_amb_od",                           "Ambient temperature",                                                                                   "C",            "",              "weather",                                  "sim_type=1",                         "",              ""},
    { SSC_INPUT, SSC_ARRAY,   "deltaT_sky_od",                      "Difference between ambient and sky temps",                                                              "C",            "",              "weather",                                  "sim_type=1",                         "",              ""},
    { SSC_INPUT, SSC_ARRAY,   "v_wind_10_od",                       "Wind speed at 10 meters",                                                                               "m/s",          "",              "weather",                                  "sim_type=1",                         "",              ""},
    { SSC_INPUT, SSC_ARRAY,   "clearsky_to_measured_dni_od",        "Ratio of clearsky to measured DNI",                                                                     "",             "",              "weather",                                  "sim_type=1&is_rec_clearsky_control=1", "",            ""},
        // Flux
    { SSC_INPUT, SSC_MATRIX,  "flux_map_od",                        "rows: timestep, columns: panels. Flux *after* rec reflectance losses",                                  "W/m2",         "",              "Flux",                                     "sim_type=1",                         "",              ""},
        // Receiver control
    { SSC_INPUT, SSC_ARRAY,   "T_htf_cold_in_od",                   "HTF inlet temperature",                                                                                 "C",            "",              "Receiver control",                         "sim_type=1",                         "",              ""},
    { SSC_INPUT, SSC_ARRAY,   "plant_defocus_od",                   "Plant defocus",                                                                                         "",             "",              "Receiver control",                         "sim_type=1",                         "",              ""},


    // **********************************************

    // OUTPUTS

    // **********************************************

    
    // **********************************************
    // Receiver design
    { SSC_OUTPUT, SSC_NUMBER, "m_dot_rec_des",                      "Receiver design mass flow rate",                                                                        "kg/s",         "",              "Tower and Receiver",                       "*",                                  "",              ""},


    // **********************************************
    // Timeseries outputs
    { SSC_OUTPUT, SSC_ARRAY,  "m_dot_rec_od",                       "Receiver mass flow rate",                                                                               "kg/s",         "",              "Tower and Receiver",                       "sim_type=1",                         "",              ""},
    { SSC_OUTPUT, SSC_ARRAY,  "T_htf_rec_out_od",                   "Receiver outlet temperature after piping losses",                                                       "C",            "",              "Tower and Receiver",                       "sim_type=1",                         "",              ""},
    { SSC_OUTPUT, SSC_ARRAY,  "q_dot_htf_od",                       "Receiver thermal power to HTF after piping losses",                                                     "MWt",          "",              "Tower and Receiver",                       "sim_type=1",                         "",              ""},
    { SSC_OUTPUT, SSC_ARRAY,  "eta_rec_od",                         "Receiver thermal efficiency",                                                                           "kg/s",         "",              "Tower and Receiver",                       "sim_type=1",                         "",              ""},
    { SSC_OUTPUT, SSC_ARRAY,  "W_dot_pump_od",                      "Receiver pumping power",                                                                                "MWe",          "",              "Tower and Receiver",                       "sim_type=1",                         "",              ""},
    { SSC_OUTPUT, SSC_ARRAY,  "rec_component_defocus_od",           "Receiver component defocus",                                                                            "",             "",              "Tower and Receiver",                       "sim_type=1",                         "",              ""},


    { SSC_OUTPUT, SSC_ARRAY,  "q_dot_rec_inc_pre_defocus",          "Receiver incident flux, pre-defocus, post-reflection",                                                  "kg/s",         "",              "Tower and Receiver",                       "sim_type=1",                         "",              ""},
    { SSC_OUTPUT, SSC_ARRAY,  "q_dot_rec_inc",                      "Receiver incident flux, post defocus and reflection",                                                   "kg/s",         "",              "Tower and Receiver",                       "sim_type=1",                         "",              ""},
    { SSC_OUTPUT, SSC_ARRAY,  "q_dot_rec_rad_loss",                 "Receiver radiative losses",                                                                             "kg/s",         "",              "Tower and Receiver",                       "sim_type=1",                         "",              ""},
    { SSC_OUTPUT, SSC_ARRAY,  "q_dot_rec_conv_loss",                "Receiver convective losses",                                                                            "kg/s",         "",              "Tower and Receiver",                       "sim_type=1",                         "",              ""},
    { SSC_OUTPUT, SSC_ARRAY,  "q_dot_rec_piping_loss",              "Receiver piping thermal losses",                                                                        "kg/s",         "",              "Tower and Receiver",                       "sim_type=1",                         "",              ""},




    var_info_invalid };

class cm_mspt_sf_and_rec_isolated : public compute_module
{
public:

    cm_mspt_sf_and_rec_isolated()
    {
        add_var_info(_cm_vtab_mspt_sf_and_rec_isolated);
    }

    void exec() override
    {
        bool is_rec_model_trans = as_boolean("is_rec_model_trans");
        bool is_rec_startup_trans = false;

        std::shared_ptr<C_pt_receiver> cr_receiver;
        std::shared_ptr<C_mspt_receiver_222> mspt_base;

        double H_rec = as_double("rec_height");
        double D_rec = as_double("D_rec");

        double q_dot_rec_des = as_double("q_dot_rec_des");  //[MWt]

        int rec_night_recirc = 0;

        bool is_rec_clearsky_control = as_boolean("is_rec_clearsky_control");
        double rec_clearsky_fraction = 0.0;
        if (is_rec_clearsky_control) {
            rec_clearsky_fraction = as_double("rec_clearsky_fraction");
        }

        bool is__clearsky_to_measured_dni_od__required = false;
        if (is_rec_clearsky_control) {
            is__clearsky_to_measured_dni_od__required = true;
        }

        bool is_calc_od_tube = false;
        double W_dot_rec_target = std::numeric_limits<double>::quiet_NaN();
        int n_panels = as_integer("N_panels");

        // Transient model
        if (is_rec_model_trans || is_rec_startup_trans) {

            // This compute module currently does not support receiver startup, so hardcode garbage startup parameters
            bool is_enforce_min_startup = true;
            double heat_trace_power = std::numeric_limits<double>::quiet_NaN();
            double preheat_flux = std::numeric_limits<double>::quiet_NaN();
            double min_preheat_time = std::numeric_limits<double>::quiet_NaN();
            double min_fill_time = std::numeric_limits<double>::quiet_NaN();
            double startup_ramp_time = std::numeric_limits<double>::quiet_NaN();
            double startup_target_Tdiff = std::numeric_limits<double>::quiet_NaN();
            bool is_rec_startup_from_T_soln = false;

            // Want to call transient model from "on", so set initial temperature to something warm
            double T_initial = as_double("T_htf_cold_des");         //[C]

            if (is_rec_startup_trans && is_rec_startup_from_T_soln)
                throw exec_error("tcsmolten_salt", "Receiver startup from solved temperature profiles is only available when receiver transient startup model is enabled");

            if (is_rec_startup_trans && !is_rec_startup_from_T_soln && !is_enforce_min_startup)
            {
                log("Both 'is_rec_enforce_min_startup' and 'is_rec_startup_from_T_soln' were set to 'false'. Minimum startup time will always be enforced unless 'is_rec_startup_from_T_soln' is set to 'true'", SSC_WARNING);
                is_enforce_min_startup = true;
            }

            std::shared_ptr<C_mspt_receiver> trans_receiver = std::shared_ptr<C_mspt_receiver>(new C_mspt_receiver(
                as_double("h_tower"), as_double("epsilon"),
                as_double("T_htf_hot_des"), as_double("T_htf_cold_des"),
                as_double("f_rec_min"), q_dot_rec_des,
                as_double("rec_su_delay"), as_double("rec_qf_delay"),
                as_double("csp.pt.rec.max_oper_frac"), as_double("eta_pump"),
                as_double("d_tube_out"), as_double("th_tube"),
                as_double("piping_loss_coefficient"), as_double("piping_length_const"),
                as_double("piping_length_mult"),
                as_integer("rec_htf"), as_matrix("field_fl_props"),
                as_integer("mat_tube"),
                rec_night_recirc,
                n_panels, D_rec, H_rec,
                as_integer("Flow_type"), as_integer("crossover_shift"), as_double("hl_ffact"),
                as_double("T_htf_hot_des"), rec_clearsky_fraction,
                is_calc_od_tube, W_dot_rec_target,
                is_rec_model_trans, is_rec_startup_trans,
                as_double("rec_tm_mult"), as_double("u_riser"),
                as_double("th_riser"), as_double("riser_tm_mult"),
                as_double("downc_tm_mult"), heat_trace_power,
                preheat_flux, min_preheat_time,
                min_fill_time, startup_ramp_time,
                as_double("T_htf_cold_des"), std::min(0.0, startup_target_Tdiff),
                T_initial,
                is_rec_startup_from_T_soln, is_enforce_min_startup
                ));    // transient receiver

            cr_receiver = trans_receiver;
            mspt_base = trans_receiver;
        }
        else { // Steady state model

            std::shared_ptr<C_mspt_receiver_222> ss_receiver = std::shared_ptr<C_mspt_receiver_222>(new C_mspt_receiver_222(
                as_double("h_tower"), as_double("epsilon"),
                as_double("T_htf_hot_des"), as_double("T_htf_cold_des"),
                as_double("f_rec_min"), q_dot_rec_des,
                as_double("rec_su_delay"), as_double("rec_qf_delay"),
                as_double("csp.pt.rec.max_oper_frac"), as_double("eta_pump"),
                as_double("d_tube_out"), as_double("th_tube"),
                as_double("piping_loss_coefficient"), as_double("piping_length_const"),
                as_double("piping_length_mult"),
                as_integer("rec_htf"), as_matrix("field_fl_props"),
                as_integer("mat_tube"),
                rec_night_recirc,
                n_panels, D_rec, H_rec,
                as_integer("Flow_type"), as_integer("crossover_shift"), as_double("hl_ffact"),
                as_double("T_htf_hot_des"), rec_clearsky_fraction,
                is_calc_od_tube, W_dot_rec_target
                ));   // steady-state receiver

            cr_receiver = ss_receiver; // std::copy(ss_receiver);
            mspt_base = ss_receiver;
        }

        mspt_base->init();

        double eta_thermal_des /*-*/, W_dot_rec_pump_des /*MWe*/, W_dot_pumping_tower_share_des /*MWe*/, W_dot_pumping_rec_share_des /*MWe*/,
            rec_pump_coef_des /*MWe/MWt*/, rec_vel_htf_des_des /*m/s*/, m_dot_htf_rec_des /*kg/s*/, m_dot_htf_max_des /*kg/s*/,
            q_dot_piping_loss_des_des /*MWt*/;
        mspt_base->get_design_performance(eta_thermal_des /*-*/,
            W_dot_rec_pump_des /*MWe*/, W_dot_pumping_tower_share_des /*MWe*/, W_dot_pumping_rec_share_des /*MWe*/,
            rec_pump_coef_des /*MWe/MWt*/, rec_vel_htf_des_des /*m/s*/,
            m_dot_htf_rec_des /*kg/s*/, m_dot_htf_max_des /*kg/s*/,
            q_dot_piping_loss_des_des /*MWt*/);

        assign("m_dot_rec_des", m_dot_htf_rec_des);

        // If only simulating receiver design then get out here
        int sim_type = as_integer("sim_type");
        if (sim_type == 2) {
            return;
        }
        // ******************************************************

        size_t n_runs = 0;

        // Get timeseries inputs
        size_t n_timestep_od;
        ssc_number_t* p_timestep_od = as_array("timestep_od", &n_timestep_od);
        n_runs = std::max(n_runs, n_timestep_od);

        util::matrix_t<double> flux_map_matrix = as_matrix("flux_map_od");
        size_t n_flux_map_od = flux_map_matrix.nrows();
        size_t n_flux_map_panels = flux_map_matrix.ncols();
        n_runs = std::max(n_runs, n_flux_map_od);

        size_t n_P_amb_od;
        ssc_number_t* p_P_amb_od = as_array("P_amb_od", &n_P_amb_od);
        n_runs = std::max(n_runs, n_P_amb_od);

        size_t n_T_amb_od;
        ssc_number_t* p_T_amb_od = as_array("T_amb_od", &n_T_amb_od);
        n_runs = std::max(n_runs, n_T_amb_od);

        size_t n_deltaT_sky_od;
        ssc_number_t* p_deltaT_sky_od = as_array("deltaT_sky_od", &n_deltaT_sky_od);
        n_runs = std::max(n_runs, n_deltaT_sky_od);

        size_t n_v_wind_10_od;
        ssc_number_t* p_v_wind_10_od = as_array("v_wind_10_od", &n_v_wind_10_od);
        n_runs = std::max(n_runs, n_v_wind_10_od);

        size_t n_clearsky_to_measured_dni_od = 0;
        ssc_number_t* p_clearsky_to_measured_dni_od;
        if (is__clearsky_to_measured_dni_od__required) {
            p_clearsky_to_measured_dni_od = as_array("clearsky_to_measured_dni_od", &n_clearsky_to_measured_dni_od);
            n_runs = std::max(n_runs, n_clearsky_to_measured_dni_od);
        }


        size_t n_plant_defocus_od;
        ssc_number_t* p_plant_defocus_od = as_array("plant_defocus_od", &n_plant_defocus_od);
        n_runs = std::max(n_runs, n_plant_defocus_od);

        size_t n_T_htf_cold_in_od;
        ssc_number_t* p_T_htf_cold_in_od = as_array("T_htf_cold_in_od", &n_T_htf_cold_in_od);
        n_runs = std::max(n_runs, n_T_htf_cold_in_od);


        // Check length of timeseries input arrays
        if (n_runs % n_timestep_od != 0) {
            std::string err_msg = util::format("The longest input array contains %d elements. It must be a multiple of the"
                " timestep_od input that contains %d elements.", n_runs, n_timestep_od);
            throw exec_error("standalone_mspt", err_msg);
        }

        if (n_runs % n_flux_map_od != 0) {
            std::string err_msg = util::format("The longest input array contains %d elements. It must be a multiple of the"
                " flux_map_od input that contains %d elements.", n_runs, n_flux_map_od);
            throw exec_error("standalone_mspt", err_msg);
        }

        if (n_runs % n_P_amb_od != 0) {
            std::string err_msg = util::format("The longest input array contains %d elements. It must be a multiple of the"
                " P_amb_od input that contains %d elements.", n_runs, n_P_amb_od);
            throw exec_error("standalone_mspt", err_msg);
        }

        if (n_runs % n_T_amb_od != 0) {
            std::string err_msg = util::format("The longest input array contains %d elements. It must be a multiple of the"
                " T_amb_od input that contains %d elements.", n_runs, n_T_amb_od);
            throw exec_error("standalone_mspt", err_msg);
        }

        if (n_runs % n_deltaT_sky_od != 0) {
            std::string err_msg = util::format("The longest input array contains %d elements. It must be a multiple of the"
                " deltaT_sky_od input that contains %d elements.", n_runs, n_deltaT_sky_od);
            throw exec_error("standalone_mspt", err_msg);
        }

        if (n_runs % n_v_wind_10_od != 0) {
            std::string err_msg = util::format("The longest input array contains %d elements. It must be a multiple of the"
                " v_wind_10_od input that contains %d elements.", n_runs, n_v_wind_10_od);
            throw exec_error("standalone_mspt", err_msg);
        }

        if (is__clearsky_to_measured_dni_od__required && n_runs % n_clearsky_to_measured_dni_od != 0) {
            std::string err_msg = util::format("The longest input array contains %d elements. It must be a multiple of the"
                " clearsky_to_measured_dni_od input that contains %d elements.", n_runs, n_clearsky_to_measured_dni_od);
            throw exec_error("standalone_mspt", err_msg);
        }

        if (n_runs % n_plant_defocus_od != 0) {
            std::string err_msg = util::format("The longest input array contains %d elements. It must be a multiple of the"
                " plant_defocus_od input that contains %d elements.", n_runs, n_plant_defocus_od);
            throw exec_error("standalone_mspt", err_msg);
        }

        if (n_runs % n_T_htf_cold_in_od != 0) {
            std::string err_msg = util::format("The longest input array contains %d elements. It must be a multiple of the"
                " T_htf_cold_in_od input that contains %d elements.", n_runs, n_T_htf_cold_in_od);
            throw exec_error("standalone_mspt", err_msg);
        }

        // Allocate timeseries outputs
        ssc_number_t* p_m_dot_rec_od = allocate("m_dot_rec_od", n_runs);
        ssc_number_t* p_T_htf_rec_out_od = allocate("T_htf_rec_out_od", n_runs);
        ssc_number_t* p_q_dot_htf_od = allocate("q_dot_htf_od", n_runs);
        ssc_number_t* p_eta_rec_od = allocate("eta_rec_od", n_runs);
        ssc_number_t* p_W_dot_pump_od = allocate("W_dot_pump_od", n_runs);
        ssc_number_t* p_rec_component_defocus_od = allocate("rec_component_defocus_od", n_runs);

        ssc_number_t* p_q_dot_rec_inc_pre_defocus = allocate("q_dot_rec_inc_pre_defocus", n_runs);
        ssc_number_t* p_q_dot_rec_inc = allocate("q_dot_rec_inc", n_runs);
        ssc_number_t* p_q_dot_rec_rad_loss = allocate("q_dot_rec_rad_loss", n_runs);
        ssc_number_t* p_q_dot_rec_conv_loss = allocate("q_dot_rec_conv_loss", n_runs);
        ssc_number_t* p_q_dot_rec_piping_loss = allocate("q_dot_rec_piping_loss", n_runs);


        for (int n_run = 0; n_run < n_runs; n_run++) {

            size_t i_timestep_od = floor(n_timestep_od / (double)n_runs * n_run);
            size_t i_flux_map_od = floor(n_flux_map_od / (double)n_runs * n_run);
            size_t i_P_amb_od = floor(n_P_amb_od / (double)n_runs * n_run);
            size_t i_T_amb_od = floor(n_T_amb_od / (double)n_runs * n_run);
            size_t i_deltaT_sky_od = floor(n_deltaT_sky_od / (double)n_runs * n_run);
            size_t i_v_wind_10_od = floor(n_v_wind_10_od / (double)n_runs * n_run);
            size_t i_clearsky_to_measured_dni_od = floor(n_clearsky_to_measured_dni_od / (double)n_runs * n_run);
            size_t i_plant_defocus_od = floor(n_plant_defocus_od / (double)n_runs * n_run);
            size_t i_T_htf_cold_in_od = floor(n_T_htf_cold_in_od / (double)n_runs * n_run);

            double step = p_timestep_od[i_timestep_od];         //[s]

            double P_amb = p_P_amb_od[i_P_amb_od] * 100.0;      //[Pa] convert from mbar
            double T_amb = p_T_amb_od[i_T_amb_od] + 273.15;     //[K] convert from C
            double T_sky = T_amb - p_deltaT_sky_od[i_deltaT_sky_od];        //[K]
            double v_wind_10 = p_v_wind_10_od[i_v_wind_10_od];              //[m/s]
            double plant_defocus = p_plant_defocus_od[i_plant_defocus_od];  //[-]
            double T_salt_cold_in = p_T_htf_cold_in_od[i_T_htf_cold_in_od] + 273.15; //[K]

            // Only needed if 1) using flux_map_input and 2) not using clearsky control
            double clearsky_to_measured_dni = std::numeric_limits<double>::quiet_NaN();
            if (is__clearsky_to_measured_dni_od__required) {
                clearsky_to_measured_dni = p_clearsky_to_measured_dni_od[i_clearsky_to_measured_dni_od];     //[-]
            }


            util::matrix_t<double> flux_map_input;
            flux_map_input.resize(1, n_flux_map_panels);
            double dni_od = 550.0;
            for (int i = 0; i < n_panels; i++) {
                flux_map_input(0, i) = flux_map_matrix(i_flux_map_od, i);       //[W/m2]
            }

            C_csp_collector_receiver::E_csp_cr_modes input_operation_mode = C_csp_collector_receiver::E_csp_cr_modes::ON;

            // For now... Set receiver to "on" through some rec method that sets E_su and t_su to 0
            mspt_base->overwrite_startup_requirements_to_on();

            int out_type = -1;
            std::string out_msg = "";
            try
            {
                mspt_base->call(step, P_amb, T_amb, T_sky,
                    clearsky_to_measured_dni,
                    v_wind_10, plant_defocus,
                    &flux_map_input, input_operation_mode,
                    T_salt_cold_in);
            }
            catch (C_csp_exception& csp_exception)
            {
                // Report warning before exiting with error
                while (mspt_base->csp_messages.get_message(&out_type, &out_msg))
                {
                    log(out_msg, out_type);
                }

                throw exec_error("etes_electric_resistance", csp_exception.m_error_message);
            }

            mspt_base->converged();

            // Essential outputs
            double m_dot_rec_od = mspt_base->ms_outputs.m_m_dot_salt_tot / 3600.0;  //[kg/s]
            double T_htf_rec_out_od = mspt_base->ms_outputs.m_T_salt_hot;              //[C] temperature after downcomer losses
            double q_dot_htf_od = mspt_base->ms_outputs.m_Q_thermal;                   //[MWt] total 'component' heat, subtracts downcomer losses: m_dot_salt_tot*c_p_coolant*(T_salt_hot - T_salt_cold_in)
            double eta_rec_od = mspt_base->ms_outputs.m_eta_therm;                  //[-]
            double W_dot_pump_od = mspt_base->ms_outputs.m_W_dot_pump;              //[MWe]
            double rec_component_defocus_od = mspt_base->ms_outputs.m_component_defocus;    //[-]

            p_m_dot_rec_od[n_run] = (ssc_number_t)m_dot_rec_od;         //[kg/s]
            p_T_htf_rec_out_od[n_run] = (ssc_number_t)T_htf_rec_out_od; //[C]
            p_q_dot_htf_od[n_run] = (ssc_number_t)q_dot_htf_od;         //[MWt]
            p_eta_rec_od[n_run] = (ssc_number_t)eta_rec_od;             //[kg/s]
            p_W_dot_pump_od[n_run] = (ssc_number_t)W_dot_pump_od;       //[MWe]
            p_rec_component_defocus_od[n_run] = (ssc_number_t)rec_component_defocus_od; //[-]

            // Energy balance outputs
            double q_dot_rec_inc_pre_defocus = mspt_base->ms_outputs.m_q_dot_rec_inc_pre_defocus;   //[MWt]
            double q_dot_rec_inc = mspt_base->ms_outputs.m_q_dot_rec_inc;               //[MWt] Absorbed by panel surface. Does not include flux lost from reflectance or defocus
            double q_dot_rec_rad_loss = mspt_base->ms_outputs.m_q_rad_sum;              //[MWt]
            double q_dot_rec_conv_loss = mspt_base->ms_outputs.m_q_conv_sum;            //[MWt]
            double q_dot_rec_piping_loss = mspt_base->ms_outputs.m_q_dot_piping_loss;   //[MWt]

            p_q_dot_rec_inc_pre_defocus[n_run] = q_dot_rec_inc_pre_defocus;     //[MWt]
            p_q_dot_rec_inc[n_run] = q_dot_rec_inc;                             //[MWt]
            p_q_dot_rec_rad_loss[n_run] = q_dot_rec_rad_loss;                   //[MWt]
            p_q_dot_rec_conv_loss[n_run] = q_dot_rec_conv_loss;                 //[MWt]
            p_q_dot_rec_piping_loss[n_run] = q_dot_rec_piping_loss;             //[MWt]

            double afaf = 1.23;




            // Receiver/tower design options
            // 1) import through cmod
            // 2) generate through solarpilot?

            // Initialization options
            // 1) Default
            // 2) Import through cmod
            // 3) Steady state (implies transient simulation?)
        }



        return;
    }
};

DEFINE_MODULE_ENTRY(mspt_sf_and_rec_isolated, "MSPT solar field and tower/receiver model", 1)
