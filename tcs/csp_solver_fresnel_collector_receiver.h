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

#ifndef __csp_solver_fresnel_collector_receiver_
#define __csp_solver_fresnel_collector_receiver_

#include "csp_solver_core.h"
#include "htf_props.h"
#include "sam_csp_util.h"

class EvacReceiverModel
{
    // Private Fields
private:

    // Constants
    const double m_T_htf_prop_min = 275;
    const double pi = acos(-1);
    const double g = 9.81;

    // Fields
    const vector<double> m_D_abs_in;		                    // [m] The inner absorber tube diameter (m_D_2)
    const vector<double> m_D_abs_out;		                    // [m] The outer absorber tube diameter (m_D_3)

    const vector<double> m_D_glass_in;		                    // [m] The inner glass envelope diameter (m_D_4)
    const vector<double> m_D_glass_out;		                    // [m] The outer glass envelope diameter (m_D_5)
    const vector<double> m_D_plug;		                        // [m] The diameter of the absorber flow plug (optional)  (m_D_p)

    const double m_L_mod;		                                // The length of the collector module (L_SCA)

    const vector<bool> m_GlazingIntact;		                    // [-] Glazing intact (broken glass) flag {1=true, else=false}
    const vector<double> m_Shadowing; 			                // [-] Receiver bellows shadowing loss factor
    const vector<double> m_dirt_env;		                    // Loss due to dirt on the receiver envelope (m_Dirt_HCE)

    const vector<double> m_P_a; 					            // [torr] Annulus gas pressure	

    const vector<double> m_alpha_abs;                           // [-] Absorber absorptance
    const vector<double> m_epsilon_glass;		                // Glass envelope emissivity
    const vector<double> m_Tau_envelope; 		                // [-] Envelope transmittance

    const vector<double> m_alpha_env; 			                // [-] Envelope absorptance
    emit_table* m_epsilon_abs;                                  // [-] Absorber emittance

    HTFProperties m_htfProps, m_airProps;                       // htf and air material properties
    
    const util::matrix_t<HTFProperties*> m_AnnulusGasMat;		// HTF Property class for each variant of each receiver type
    const util::matrix_t<AbsorberProps*> m_AbsorberPropMat;	    // Absorber Property class for each variant of each receiver type
    
    const vector<double> m_Flow_type; 			                // [-] Flow type through the absorber

    const vector<double> m_A_cs;	                            //[m^2] Cross-sectional area for HTF flow for each receiver
    const vector<double> m_D_h;	                                //[m^2] Hydraulic diameters for HTF flow for each receiver and variant (why variant?)	

    // Private Methods
private:

    double fT_2_v2(double q_12conv, double T_1, double T_2g, double v_1, int hv);

    void FQ_34CONV_v2(double T_3, double T_4, double P_6, double v_6, double T_6, int hv, double& q_34conv, double& h_34);

    void FQ_34RAD_v2(double T_3, double T_4, double T_7, double epsilon_abs_v, int hv, double& q_34rad, double& h_34);

    void FQ_56CONV_v2(double T_5, double T_6, double P_6, double v_6, int hv, double& q_56conv, double& h_6);

    double FQ_COND_BRACKET_v2(double T_3, double T_6, double P_6, double v_6);

    double FK_23_v2(double T_2, double T_3, int hv);

public:

    EvacReceiverModel(vector<double> D_abs_in, vector<double> D_abs_out, vector<double> D_glass_in, vector<double> D_glass_out, vector<double> D_plug,
        double L_mod, vector<bool> GlazingIntact, vector<double> Shadowing, vector<double> dirt_env, vector<double> P_a, vector<double> alpha_abs,
        vector<double> epsilon_glass, vector<double> Tau_envelope, vector<double> alpha_env, emit_table* epsilon_abs, HTFProperties htfProps, HTFProperties airProps,
        util::matrix_t<HTFProperties*> AnnulusGasMat, util::matrix_t<AbsorberProps*> AbsorberPropMat, vector<double> Flow_type, vector<double> A_cs, vector<double> D_h);


    void Calculate_Energy_Balance(double T_1_in, double m_dot, double T_amb, double T_sky, double v_6, double P_6, double q_i,
        int hv /* HCE variant [0..3] */, int sca_num, bool single_point, double time, util::matrix_t<double> ColOptEff,
        //outputs
        double& q_heatloss, double& q_12conv, double& q_34tot, double& c_1ave, double& rho_1ave, std::vector<double>& v_reguess_args, double& q_3reflect);

};


class C_csp_fresnel_collector_receiver : public C_csp_collector_receiver
{

public:
    
    enum
    {
        E_EQUIV_OPT_ETA_TOT,	//[-]
        E_DEFOCUS,				//[-]

        E_Q_DOT_INC_SF_TOT,        //[MWt]
        E_Q_DOT_INC_SF_COSTH,	   //[MWt]
        E_Q_DOT_REC_INC,		   //[MWt]
        E_Q_DOT_REC_THERMAL_LOSS,  //[MWt]
        E_REC_THERMAL_EFF,
        E_Q_DOT_REC_ABS,		   //[MWt]
        E_Q_DOT_PIPING_LOSS,	   //[MWt]
        E_E_DOT_INTERNAL_ENERGY,   //[MWt]
        E_Q_DOT_HTF_OUT,		   //[MWt]
        E_Q_DOT_FREEZE_PROT,       //[MWt]

        E_TIME_IN_STARTUP,          //[min]

        E_M_DOT_LOOP,				//[kg/s]
        E_IS_RECIRCULATING,         //[-]
        E_M_DOT_FIELD_RECIRC,		//[kg/s]
        E_M_DOT_FIELD_DELIVERED,	//[kg/s]
        E_T_FIELD_COLD_IN,	//[C]
        E_T_REC_COLD_IN,	//[C]
        E_T_REC_HOT_OUT,	//[C]
        E_T_FIELD_HOT_OUT,	//[C]
        E_PRESSURE_DROP,	//[bar]

        E_W_DOT_SCA_TRACK,	//[MWe]
        E_W_DOT_PUMP,		//[MWe]

    };

    enum struct E_loop_energy_balance_exit
    {
        SOLVED,
        NaN
    };

    // Private Fields
private:

    // Hardcoded constants
    const double m_d2r = CSP::pi / 180.0;
    const double m_r2d = 180.0 / CSP::pi;
    const double m_mtoinch = 39.3700787;                // [m] to [in]
    const double m_T_htf_prop_min = 275.0;              // K Minimum temperature allowed in props call to minimize errors
    const double fp_offset = 10;                        // freeze protection offset
    double m_P_field_in;                                // Assumed inlet htf pressure for property lookups (defined in init())

    // Evac Receiver Model Class and Parameters
    std::unique_ptr<EvacReceiverModel> m_evac_receiver;
    std::vector<double> mv_HCEguessargs;

    // Geometry calculated in init()
    double m_v_hot;			                            // [m^3] Hot piping volume
    double m_v_cold;		                            // [m^3] Cold piping volume
    int	m_nfsec;			                            // [-] Number of field sections
    int	m_nhdrsec;			                            // [-] Number of header sections
    int	m_nrunsec;			                            // [-] Number of unique runner diameters
    double m_L_tot;			                            // [m] Total length of collectors in a loop

    // Design Point properties calculated in init()
    bool m_is_solar_mult_designed = false;              // Flag for whether solar multiple has been calculated
    double m_opteff_des;	                            // [-] Design-point optical efficiency (theta = 0) from the solar field
    vector<double> m_A_cs;	                            // [m^2] Cross-sectional area for HTF flow for each receiver and variant (why variant?)
    vector<double> m_D_h;	                            // [m^2] Hydraulic diameters for HTF flow for each receiver and variant (why variant?)	
    string m_piping_summary;
    double eta_opt_fixed;

    // Cycle properties
    emit_table m_epsilon_3;			                    // Table of emissivity vs temperature for each variant of each receiver type
    util::matrix_t<HTFProperties*> m_AnnulusGasMat;		// HTF Property class for each variant of each receiver type
    util::matrix_t<AbsorberProps*> m_AbsorberPropMat;	// Absorber Property class for each variant of each receiver type
    OpticalDataTable optical_table;
    emit_table m_epsilon_abs;

    // Init() inputs
    double m_latitude;		                            // [deg] convert to [rad] in init()
    double m_longitude;		                            // [deg] convert to [rad] in init()
    double m_shift;			                            // [deg] convert to [rad] in init()

    // Process Parameters
    C_csp_collector_receiver::E_csp_cr_modes m_operating_mode_converged;    // Converged Operating Mode
    C_csp_collector_receiver::E_csp_cr_modes m_operating_mode;              // Current Operating Mode
    bool m_is_m_dot_recirc;		                                            // [-] True: fresnel is recirculationg HTF with interacting with other CSP components
    double m_step_recirc;
    std::string m_error_msg;                                                // member string for exception messages

    // Optical Parameters (calculated with loop_optical_eta)
    double m_eta_optical;		                        // Collector total optical efficiency
    double m_phi_t = 0;		                            // Solar incidence angle in the collector transversal plane
    double m_theta_L = 0;		                        // Solar incidence angle in the collector longitudinal plane
    double m_ftrack = 0;                                   
    double m_q_i;			                            // [W/m] DNI * A_aper / L_sca      NOT a vector because all collector lengths are equal
    std::vector<double> m_q_SCA;		                // [W/m] Total incident irradiation on the receiver (q"*A_aper/L_sca*cos(theta)*all_defocus) // This value (m_q_SCA) is passed to the Evacuated Tube model, where the other optical losses are applied
    std::vector<double> m_q_SCA_control_df;	            // [W/m] Total incident irradiation less CONTROL defocus (m_q_sca * control_defocus)
    util::matrix_t<double> m_ColOptEff;	                // [-] tracking * geom * rho * dirt * error * IAM * row shadow * end loss * ftrack
    double m_W_dot_sca_tracking;	                    // [MWe] SCA tracking power
    double m_q_dot_inc_sf_tot;	                        // [MWt] Total incident radiation on solar field
                                                           
    // Field Pressure Drop                                 
    std::vector<double> m_DP_tube;	                    // [Pa] Pressure drops in each receiver
    double m_dP_total;						            // [bar] FIELD pressure drop
    double m_W_dot_pump;					            // [MWe] FIELD pumping power
                                                           
                                                           
    // Defocus Parameters                                  
    double m_control_defocus;	                        // [-] Defocus signal from control model
    double m_component_defocus;	                        // [-] Defocus signal from this component (max mass flow rate reached and still over target...)
                                                           
    // Loop Energy Balance T Int                           
    double m_EqOpteff;			                        // [-] 
    double m_Header_hl_cold;	                        // [W] Total heat loss from the cold headers *in one field section*
    double m_Header_hl_cold_tot;                           
    double m_Runner_hl_cold;	                        // [W] Total heat loss from the cold runners *in one field section*
    double m_Runner_hl_cold_tot;                           
                                                           
    double m_Header_hl_hot;		                        // [W] Total heat loss from the hot headers *in one field section*
    double m_Header_hl_hot_tot;                            
    double m_Runner_hl_hot;		                        // [W] Total heat loss from the hot runners *in one field section*
    double m_Runner_hl_hot_tot;                            
                                                           
    double m_c_hdr_hot;			                        // [J/kg-K] Specific heat of fluid at outlet temperature of last SCA (not necessarily return temperature if modeling runners and headers)
                                                           
    double m_T_loop_in;                                    
                                                           
    double m_m_dot_htf_tot;	                            // [kg/s] The total flow rate through the entire field (m_dot_loop * N_loops)
    double m_c_htf_ave;		                            // [J/kg-K] Average solar field specific heat
                                                           
    std::vector<double> m_E_int_loop;	                // [J] Energy relative to ambient for each receiver
    std::vector<double> m_E_accum;		                // [J] Internal energy change in timestep for each receiver
    std::vector<double> m_E_avail;		                // [J] Energy absorbed less internal energy change for each receiver
    std::vector<double> m_q_abs_SCAtot;	                // [W] Heat absorption into HTF in each SCA, weighted variants
    std::vector<double> m_q_loss_SCAtot;                // [W] Total heat losses from each SCA, weighted variants
    std::vector<double> m_q_1abs_tot;	                // [W/m] Thermal losses from each SCA, weighted variants
    std::vector<double> m_q_reflect_tot;                // [W] Reflective Losses on each SCA, weighted variants 09.08.2023 tmb

    std::vector<double> m_q_loss;		                // [W/m] Total thermal losses per length in each SCA, one variant
    std::vector<double> m_q_abs;		                // [W/m] Total heat absorption per length into HTF in each SCA, one variant
    std::vector<double> m_q_1abs;		                // [W/m] Total *thermal* losses per length in each SCA, one variant
    std::vector<double> m_q_reflect;                    //[W/m] Total receiver reflected energy due to absorber absorptance 08.29.2023 tmb

    double m_Q_field_losses_total_subts;	            // [MJ] SYSTEM scas + xover + hot_HR + cold_HR
    double m_c_htf_ave_ts_ave_temp;			            // [J/kg-K] integrated-averaged cp over T_htf_cold_in, m_T_sys_h_t_in


    // *********************************************
    // CSP Solver Temperature Tracking
        // Temperatures from the most recent converged() operation
    double m_T_sys_c_t_end_converged;
    std::vector<double> m_T_htf_out_t_end_converged;
    double m_T_sys_h_t_end_converged;
    // ** Check for these in other methods developed for CSP Solver **

    // Temperatures from the most recent timstep (in the event that a method solves multiple, shorter timesteps
    double m_T_sys_c_t_end_last;			            // [K] Temperature (bulk) of cold runners & headers at end of previous timestep
    std::vector<double> m_T_htf_out_t_end_last;			// [K] Temperature of HTF temperature & material at end of previous timestep
    double m_T_sys_h_t_end_last;			            // [K] Temperature (bulk) of hot runners & headers at end of previous timestep

    // Latest temperature solved during present call to this class
        // SUB TIMESTEP outputs
    double m_T_sys_c_t_end;				                // [K] Temperature (bulk) of cold runners & headers at end of current timestep
    double m_T_sys_c_t_int;				                // [K] Temperature (bulk) of cold runners & headers at time-INTegrated-average	
    std::vector<double> m_T_htf_in_t_int;	            // [K] time-integrated-average inlet HTF temperature to each SCA
    std::vector<double> m_T_htf_out_t_end;	            // [K] end-of-timestep outlet HTF temperature of each SCA
    std::vector<double> m_T_htf_out_t_int;	            // [K] time-integrated-average outlet HTF temp of each SCA
    double m_T_sys_h_t_end;				                // [K] Temperature (bulk) of hot runners & headers at end of current timestep
    double m_T_sys_h_t_int;				                // [K] Temperature (bulk) of hot runners & headers at timestep-integrated-average
                                                           
    double m_q_dot_sca_loss_summed_subts;	            // [MWt] SYSTEM SCA heat loss
    double m_q_dot_sca_abs_summed_subts;	            // [MWt] SYSTEM SCA absorbed thermal power (into HTF stream & material)
    double m_q_dot_sca_refl_summed_subts;               // [MWt] SYSTEM SCA reflected heat loss (due to absorber absorptance) 09.08.2023 tmb
    double m_q_dot_xover_loss_summed_subts;	            // [MWt] SYSTEM Cross-over/connecting piping heat loss
    double m_q_dot_HR_cold_loss_subts;		            // [MWt] SYSTEM Cold header heat loss
    double m_q_dot_HR_hot_loss_subts;		            // [MWt] SYSTEM Hot header heat loss
    double m_E_dot_sca_summed_subts;		            // [MWt] SYSTEM SCA internal energy change over time
    double m_E_dot_xover_summed_subts;		            // [MWt] SYSTEM Cross-over/connecting piping internal energy change over time
    double m_E_dot_HR_cold_subts;			            // [MWt] SYSTEM Cold header internal energy change
    double m_E_dot_HR_hot_subts;			            // [MWt] SYSTEM hot header internal energy change
    double m_q_dot_htf_to_sink_subts;		            // [MWt] SYSTEM thermal power to sink (or artificially added to system in recirculation...)
    // *********************************************       
    // *********************************************       
            // Full Timestep outputs                       
    double m_T_sys_c_t_int_fullts;			            // [K] Temperature (bulk) of cold runners & headers at end of current timestep
    double m_T_htf_c_rec_in_t_int_fullts;	            // [K] Time-integrated-average inlet HTF temperature to FIRST sca
    double m_T_htf_h_rec_out_t_int_fullts;	            // [K] Time-integrated-average outlet HTF temperature from LAST sca
    double m_T_sys_h_t_int_fullts;			            // [K] Temperature (bulk) of hot runners & headers at timestep-integrated-average
                                                           
    double m_q_dot_sca_loss_summed_fullts;	            // [MWt] SYSTEM SCA heat loss
    double m_q_dot_sca_abs_summed_fullts;	            // [MWt] SYSTEM SCA absorbed thermal power (into HTF stream & material)
    double m_q_dot_sca_refl_summed_fullts;              // [MWt] SYSTEM SCA reflected heat loss (due to absorber absorptance) 09.08.2023 tmb
    double m_q_dot_xover_loss_summed_fullts;            // [MWt] SYSTEM Cross-over/connecting piping heat loss
    double m_q_dot_HR_cold_loss_fullts;		            // [MWt] SYSTEM Cold header heat loss
    double m_q_dot_HR_hot_loss_fullts;		            // [MWt] SYSTEM Hot header heat loss
    double m_E_dot_sca_summed_fullts;		            // [MWt] SYSTEM SCA internal energy change over time
    double m_E_dot_xover_summed_fullts;		            // [MWt] SYSTEM Cross-over/connecting piping internal energy change over time
    double m_E_dot_HR_cold_fullts;			            // [MWt] SYSTEM Cold header internal energy change
    double m_E_dot_HR_hot_fullts;			            // [MWt] SYSTEM hot header internal energy change
    double m_q_dot_htf_to_sink_fullts;		            // [MWt] SYSTEM thermal power to sink (or artificially added to system in recirculation...)
    double m_q_dot_freeze_protection;		            // [MWt] SYSTEM thermal freeze protection

    double m_q_dot_thermal_reported;        //[MWt] HTF thermal power reported back to controller - should be 0 for recirculating startup and off

    double m_time_at_off;                   //[s]
    double m_time_at_startup;               //[s]
    double m_time_at_on;                    //[s]

    // Private Methods
private:

    int freeze_protection(const C_csp_weatherreader::S_outputs& weather,
        double& T_cold_in /*K*/, double m_dot_loop /*kg/s*/,
        const C_csp_solver_sim_info& sim_info, double& Q_fp /*MJ*/);

    double field_pressure_drop(double T_db, double m_dot_field, double P_in_field,
        const std::vector<double>& T_in_SCA, const std::vector<double>& T_out_SCA);

    void set_output_value();

    // This method is designed to pass the timestep integrated HTF temperature to successive energy balance nodes
    E_loop_energy_balance_exit loop_energy_balance_T_t_int(const C_csp_weatherreader::S_outputs& weather,
        double T_htf_cold_in /*K*/, double m_dot_htf_loop /*kg/s*/,
        const C_csp_solver_sim_info& sim_info);

    void header_design(int nhsec, int nfsec, int nrunsec, double rho, double V_max, double V_min, double m_dot,
        vector<double>& D_hdr, vector<double>& D_runner, std::string*);


    void loop_optical_eta(const C_csp_weatherreader::S_outputs& weather,
        const C_csp_solver_sim_info& sim_info);

    void loop_optical_eta_off();

    void loop_optical_wind_stow();

    void update_last_temps();

    void reset_last_temps();

    void apply_control_defocus(double defocus /*-*/);

    void apply_component_defocus(double defocus /*-*/);

    // From sam_mw_lf_type262_salt

    double Pump_SGS(double rho, double m_dotsf, double sm);

    double PressureDrop(double m_dot, double T, double P, double D, double Rough, double L_pipe,
        double Nexp, double Ncon, double Nels, double Nelm, double Nell, double Ngav, double Nglv,
        double Nchv, double Nlw, double Nlcv, double Nbja);

    double FricFactor(double Rough, double Reynold);

    static double m_dot_runner(double m_dot_field, int nfieldsec, int irnr);

    static double m_dot_header(double m_dot_field, int nfieldsec, int nLoopsField, int ihdr);


    // Public Fields
public:

    // INPUTS

    int m_solar_mult_or_Ap;                         // Design using specified solar mult or field aperture
    double m_solar_mult_in;                         // Solar multiple input
    double m_total_Ap_in;                           // Field aperture input

    int m_nMod;                                     // Number of collector modules in a loop   (m_nSCA)
    int m_nRecVar;                                  // Number of receiver variations (m_nHCEt)

    double m_eta_pump;		                        // [-] HTF pump efficiency
    double m_HDR_rough;		                        // [m] Header pipe roughness
    double m_theta_stow;	                        // [deg] stow angle
    double m_theta_dep;		                        // [deg] deploy angle
    int m_FieldConfig;		                        // [-] Number of subfield headers
    double m_T_startup;		                        // [C] The required temperature (converted to K in init) of the system before the power block can be switched on
    double m_P_ref;                                 // Design Turbine Net Output (W)
    double m_eta_ref;                               // Design cycle thermal efficiency

    double m_T_loop_in_des;	                        // [C] Design loop inlet temperature, converted to K in init
    double m_T_loop_out_des;                        // [C] Target loop outlet temperature, converted to K in init
    int m_Fluid;			                        // [-] Field HTF fluid number

    int m_use_abs_or_rel_mdot_limit = 0;            // Use mass flow abs (0) or relative (1) limits
    double m_m_dot_htfmin_in = std::numeric_limits<double>::quiet_NaN();	    //[kg/s] Minimum loop HTF flow rate INPUT
    double m_m_dot_htfmax_in = std::numeric_limits<double>::quiet_NaN();	    //[kg/s] Maximum loop HTF flow rate INPUT
    double m_f_htfmin_in = std::numeric_limits<double>::quiet_NaN();	        // Minimum loop HTF fraction of mdot design INPUT
    double m_f_htfmax_in = std::numeric_limits<double>::quiet_NaN();	        // Maximum loop HTF fraction of mdot design INPUT


    util::matrix_t<double> m_field_fl_props;	    // [-] User-defined field HTF properties
    double m_T_fp;			                        // [C] Freeze protection temperature (heat trace activation temperature), convert to K in init
    double m_I_bn_des;		                        // [W/m^2] Solar irradiation at design
    double m_V_hdr_max;		                        // [m/s] Maximum HTF velocity in the header at design
    double m_V_hdr_min;		                        // [m/s] Minimum HTF velocity in the header at design
    double m_Pipe_hl_coef;	                        // [W/m2-K] Loss coefficient from the header, runner pipe, and non-HCE piping
    double m_SCA_drives_elec;	                    // [W/SCA] Tracking power, in Watts per SCA drive
    double m_ColAz;			                        // [deg] Collector azimuth angle

    double m_mc_bal_hot;		                    // [J/K] The heat capacity of the balance of plant on the hot side
    double m_mc_bal_cold;		                    // [J/K] The heat capacity of the balance of plant on the cold side
    double m_mc_bal_sca;		                    // [Wht/K-m] Non-HTF heat capacity associated with each SCA - per meter basis

    int m_opt_model;		                        // The optical model (1=Solar position ; 2=Collector incidence table ; 3 = IAM polys)

    // Mirror Properties
    double m_A_aperture;	                        // [m^2] Reflective aperture area of the collector
    double m_reflectivity;                          // Solar-weighted mirror reflectivity value
    double m_TrackingError;                         // [-] Tracking error derate
    double m_GeomEffects;	                        // [-] Geometry effects derate
    double m_Dirt_mirror;	                        // [-] Dirt on mirror derate
    double m_Error;		                            // [-] General optical error derate
    double m_L_mod;		                            // The length of the collector module (L_SCA)

    vector<double> m_IAM_T_coefs;                   // Incidence angle modifier coefficients - transversal plane
    vector<double> m_IAM_L_coefs;		            // Incidence angle modifier coefficients - longitudinal plane
    util::matrix_t<double> m_OpticalTable;          // Values of the optical efficiency table
    int m_rec_model;		                        // Receiver model type (1=Polynomial ; 2=Evac tube)

    vector<double> m_HCE_FieldFrac;                 // [-] Fraction of the field occupied by this HCE type
    vector<double> m_D_abs_in;		                // [m] The inner absorber tube diameter (m_D_2)
    vector<double> m_D_abs_out;		                // [m] The outer absorber tube diameter (m_D_3)

    vector<double> m_D_glass_in;		            // [m] The inner glass envelope diameter (m_D_4)
    vector<double> m_D_glass_out;		            // [m] The outer glass envelope diameter (m_D_5)
    vector<double> m_D_plug;		                // [m] The diameter of the absorber flow plug (optional)  (m_D_p)
    vector<double> m_Flow_type; 			        // [-] Flow type through the absorber
    vector<double> m_Rough; 				        // [m] Roughness of the internal surface
    vector<double> m_alpha_env; 			        // [-] Envelope absorptance

    util::matrix_t<double> m_epsilon_abs_1;         // Absorber emittance - HCE variation 1
    util::matrix_t<double> m_epsilon_abs_2;         // Absorber emittance - HCE variation 2
    util::matrix_t<double> m_epsilon_abs_3;         // Absorber emittance - HCE variation 3
    util::matrix_t<double> m_epsilon_abs_4;         // Absorber emittance - HCE variation 4

    vector<double> m_alpha_abs;                     // [-] Absorber absorptance
    vector<double> m_Tau_envelope; 		            // [-] Envelope transmittance
    vector<double> m_epsilon_glass;		            // Glass envelope emissivity
    vector<bool> m_GlazingIntact;		            // [-] Glazing intact (broken glass) flag {1=true, else=false}

    vector<double> m_P_a; 					        // [torr] Annulus gas pressure	

    vector<double> m_AnnulusGas; 			        // [-] Annulus gas type (1=air, 26=Ar, 27=H2)
    vector<double> m_AbsorberMaterial; 	            // [-] Absorber material type
    vector<double> m_Shadowing; 			        // [-] Receiver bellows shadowing loss factor
    vector<double> m_dirt_env;		                // Loss due to dirt on the receiver envelope (m_Dirt_HCE)
    vector<double> m_Design_loss; 			        // [-] Receiver heat loss at design

    double m_L_mod_spacing;                         // Piping distance between sequential modules in a loop
    double m_L_crossover;                           // Length of crossover piping in a loop
    vector<double> m_HL_T_coefs;		            // HTF temperature-dependent heat loss coefficients
    vector<double> m_HL_w_coefs;		            // Wind-speed-dependent heat loss coefficients

    double m_DP_nominal;		                    // Pressure drop across a single collector assembly at design
    vector<double> m_DP_coefs;		                // Pressure drop mass flow based part-load curve
    double m_rec_htf_vol;		                    // Volume of HTF in a single collector unit per unit aperture area

    double m_V_wind_des;                            // Design-point wind velocity
    double m_T_amb_sf_des;                          // Ambient design-point temperature for the solar field

    double m_L_rnr_pb;                              //[m] Length of hot or cold runner pipe around the power block

    double m_rec_su_delay;                          //[hr] Fixed startup delay time for the receiver
    double m_rec_qf_delay;                          //[-] Energy-based receiver startup delay (fraction of rated thermal power)
    double m_p_start;                               //[kWe-hr] Collector startup energy, per SCA
    

    // Fields accessible as outputs
public:

    std::vector<double> m_D_runner;	                // [m]    Diameters of runner sections
    std::vector<double> m_WallThk_runner;	        // [m]    Pipe wall thicknesses of runner sections
    std::vector<double> m_m_dot_rnr_dsn;            // [kg/s] Design mass flow through runner sections
    std::vector<double> m_V_rnr_dsn;                // [m/s]  Design velocity through runner sections
    std::vector<double> m_L_runner;	                // [m]    Lengths of runner sections
    std::vector<int> m_N_rnr_xpans;                 // [-]    Number of expansions in runner sections
    std::vector<double> m_DP_rnr;                   // [bar]  Pressure drop in runner sections
    std::vector<double> m_T_rnr_dsn;                // [C]    Temperature entering runner sections at design
    std::vector<double> m_P_rnr_dsn;                // [bar]  Gauge pessure in runner sections at design
    std::vector<double> m_T_rnr;                    // [K]    Temperature entering runner sections
    double m_T_field_out;                           // [K]    Temperature exiting last runner, and thus exiting field
    std::vector<double> m_P_rnr;                    // [Pa]  Gauge pessure in runner sections                                                   
                                                       
    std::vector<double> m_D_hdr;	                // [m]    Diameters of header sections
    std::vector<double> m_WallThk_hdr;   	        // [m]    Pipe wall thicknesses of header sections
    std::vector<double> m_m_dot_hdr_dsn;            // [kg/s] Design mass flow through header sections
    std::vector<double> m_V_hdr_dsn;                // [m/s]  Design velocity through header sections
                                                       
    std::vector<int> m_N_hdr_xpans;                 // [-]    Number of expansions in header sections
    std::vector<double> m_DP_hdr;                   // [bar]  Pressure drop in header sections
    std::vector<double> m_T_hdr_dsn;                // [C]    Temperature entering header sections at design
    std::vector<double> m_P_hdr_dsn;                // [bar]  Gauge pessure in header sections at design
    std::vector<double> m_T_hdr;                    // [K]    Temperature entering header sections
    std::vector<double> m_P_hdr;                    //[Pa]   Gauge pessure in header sections                 
                                                       
    std::vector<double> m_T_loop_dsn;               // [C]    Temperature entering loop sections at design
    std::vector<double> m_P_loop_dsn;               // [bar]  Gauge pressure in loop sections at design
    std::vector<double> m_T_loop;                   // [K]    Temperature entering loop sections

    // Removed (polynomial model does not have pressure across individual receivers)
    //std::vector<double> m_DP_loop;                  // [bar]  Pressure drop in loop sections
    //std::vector<double> m_P_loop;                   //[Pa]   Gauge pessure in loop sections

    C_csp_reported_outputs mc_reported_outputs;

    HTFProperties m_htfProps, m_airProps;

    // Public Design Point Outputs
    int m_nLoops;                                   // [-] Number of loops in the field
    double m_solar_mult;		                    // [-] Solar multiple
    double m_Ap_tot;                                // Total field aperture [m2]
    double m_q_design_actual;		                // [Wt] Design-point thermal power from the solar field limited by mass flow
    double m_q_design_ideal;                        // [Wt] Design-point thermal power from the solar field not limited by mass flow
    double m_Ap_sm1;                                // Total required aperture, SM=1 [m2]
    double m_nLoops_sm1;                            // Required number of loops, SM=1
    double m_A_loop;                                // Aperture of a loop [m2]
    double m_dT_des;                                // Average field temp difference at design [delta C (or K)]
    double m_hl_des;                                // Heat loss at design [W/m]
    double m_loop_opt_eff;                          // Loop optical efficiency
    double m_loop_therm_eff;                        // Loop thermal Efficiency
    double m_loop_eff;                              // Loop total efficiency
    double m_W_dot_sca_tracking_nom;	            // [MWe] Tracking parasitics when fresnel is on sun
    double m_opt_derate;                            // Optical derate
    double m_opt_normal;                            // Collector optical loss at normal incidence
    double m_m_dot_design;	                        // [kg/s] Total solar field mass flow rate at design
    double m_m_dot_loop_des;                        // [kg/s] LOOP design mass flow rate
    double m_q_pb_design;                           // [Wt] Power block design input power

    double m_m_dot_htfmin = std::numeric_limits<double>::quiet_NaN();	                        //[kg/s] Minimum loop HTF flow rate
    double m_m_dot_htfmax = std::numeric_limits<double>::quiet_NaN();	                        //[kg/s] Maximum loop HTF flow rate
    double m_f_htfmin = std::numeric_limits<double>::quiet_NaN();	                            // Minimum loop HTF fraction of mdot design
    double m_f_htfmax = std::numeric_limits<double>::quiet_NaN();	                            // Maximum loop HTF fraction of mdot design


        // Steady State Design Point Outputs
    double m_dP_des_SS;                             // [bar] FIELD pressure drop at design (calculated in init (via steady_state -> On))
    double m_T_field_out_des_SS;                    // [C] FIELD outlet temperature at design (calculated in init (via steady_state))
    double m_Q_field_des_SS;                        // [Wt] Field thermal power at design (calculated in init (via steady_state))
    double m_m_dot_des_SS;                          // [kg/s] Field mass flow rate at design (calculated in init (via steady_state))
    double m_m_dot_loop_des_SS;                     // [kg/s] Loop mass flow rate at design (calculated in init (via steady_state))
    double m_V_hdr_min_des_SS;                      // [m/s] Header min HTF velocity at design (calculated in init (via steady_state))
    double m_V_hdr_max_des_SS;                      // [m/s] Header max HTF velocity at design (calculated in init (via steady_state))
    double m_eta_optical_des_SS;                    // Optical Efficiency at Steady State design (calculated in init (via steady_state))
    double m_therm_eff_des_SS;                      // Field Thermal efficiency at steady state design (calculated in init (via steady_state))
    double m_eff_des_SS;                            // Field Total efficiency at steady state design (calculated in init (via steady_state))
    double m_Q_loop_des_SS;                         // [Wt] Loop thermal power at design (calculated in init (via steady_state))
    double m_T_loop_out_des_SS;                     // [C] Loop outlet temperature at design (calculated in init (via steady_state))
    double m_therm_eff_loop_des_SS;                 // Loop Thermal efficiency at steady state design (calculated in init (via steady_state))
    double m_eff_loop_des_SS;                       // Loop Total efficiency at steady state design (calculated in init (via steady_state))
    double m_W_dot_pump_des_SS;                     // [MWe] Loop Total efficiency at steady state design (calculated in init (via steady_state))
    double m_Q_loss_receiver_des_SS;                // [MWt] Total Field Receiver thermal loss
    double m_Q_loss_hdr_rnr_des_SS;                 // [MWt] Total field thermal loss from headers and runners

    // Methods
public:

    C_csp_fresnel_collector_receiver();

    ~C_csp_fresnel_collector_receiver();

    // Overloaded Public Methods

    virtual void init(const C_csp_collector_receiver::S_csp_cr_init_inputs init_inputs,
        C_csp_collector_receiver::S_csp_cr_solved_params& solved_params);

    virtual bool init_fieldgeom();

    virtual double get_startup_time();

    virtual double get_startup_energy(); //MWh

    virtual double get_pumping_parasitic_coef();  //MWe/MWt

    virtual double get_min_power_delivery();    //MWt

    virtual double get_max_power_delivery(double T_htf_cold_in /*C*/);    //MWt

    virtual double get_tracking_power();		//MWe

    virtual double get_col_startup_power();		//MWe-hr

    virtual C_csp_collector_receiver::E_csp_cr_modes get_operating_state();

    virtual void get_design_parameters(C_csp_collector_receiver::S_csp_cr_solved_params& solved_params);

    virtual void off(const C_csp_weatherreader::S_outputs& weather,
        const C_csp_solver_htf_1state& htf_state_in,
        C_csp_collector_receiver::S_csp_cr_out_solver& cr_out_solver,
        const C_csp_solver_sim_info& sim_info);

    virtual void startup(const C_csp_weatherreader::S_outputs& weather,
        const C_csp_solver_htf_1state& htf_state_in,
        C_csp_collector_receiver::S_csp_cr_out_solver& cr_out_solver,
        const C_csp_solver_sim_info& sim_info);

    virtual void on(const C_csp_weatherreader::S_outputs& weather,
        const C_csp_solver_htf_1state& htf_state_in,
        double q_dot_elec_to_CR_heat /*MWt*/, double field_control,
        C_csp_collector_receiver::S_csp_cr_out_solver& cr_out_solver,
        const C_csp_solver_sim_info& sim_info);

    virtual void steady_state(const C_csp_weatherreader::S_outputs& weather,
        const C_csp_solver_htf_1state& htf_state_in,
        double W_dot_elec_to_CR_heat /*MWe*/, double field_control,
        C_csp_collector_receiver::S_csp_cr_out_solver& cr_out_solver,
        const C_csp_solver_sim_info& sim_info);


    virtual void estimates(const C_csp_weatherreader::S_outputs& weather,
        const C_csp_solver_htf_1state& htf_state_in,
        C_csp_collector_receiver::S_csp_cr_est_out& est_out,
        const C_csp_solver_sim_info& sim_info);

    virtual void converged();

    virtual void write_output_intervals(double report_time_start,
        const std::vector<double>& v_temp_ts_time_end, double report_time_end);

    virtual double calculate_optical_efficiency(const C_csp_weatherreader::S_outputs& weather, const C_csp_solver_sim_info& sim);

    virtual double calculate_thermal_efficiency_approx(const C_csp_weatherreader::S_outputs& weather, double q_incident /*MW*/, const C_csp_solver_sim_info& sim);

    virtual double get_collector_area();

    // ------------------------------------------ supplemental methods -----------------------------------------------------------

    bool design_solar_mult(double latitude = -377);


    // Classes
public:
    class C_mono_eq_defocus : public C_monotonic_equation
    {	// The solver chooses a defocus and sends it to the operator. The operator 
        //    calculates a new m_q_SCA and then solves the loop_energy_balance *at max HTF mass flow rate* 
        //    and returns T_htf_SCA_out. The solver finds the defocus resulting in the target HTF outlet temp
    private:
        C_csp_fresnel_collector_receiver* mpc_fresnel;
        C_csp_weatherreader::S_outputs ms_weather;
        double m_T_cold_in;				//[K]
        double m_m_dot_loop;			//[kg/s]
        C_csp_solver_sim_info ms_sim_info;

    public:
        C_mono_eq_defocus(C_csp_fresnel_collector_receiver* pc_fresnel, const C_csp_weatherreader::S_outputs& weather,
            double T_htf_cold_in /*K*/, double m_dot_loop /*kg/s*/, const C_csp_solver_sim_info& sim_info)
        {
            mpc_fresnel = pc_fresnel;
            ms_weather = weather;
            m_T_cold_in = T_htf_cold_in;	//[K]
            m_m_dot_loop = m_dot_loop;		//[kg/s]
            ms_sim_info = sim_info;
        }

        virtual int operator()(double defocus /*-*/, double* T_htf_loop_out /*K*/);
    };

    class C_mono_eq_T_htf_loop_out : public C_monotonic_equation
    {
    private:
        C_csp_fresnel_collector_receiver* mpc_fresnel;
        C_csp_weatherreader::S_outputs ms_weather;
        double m_T_cold_in;				//[K]
        C_csp_solver_sim_info ms_sim_info;

    public:
        C_mono_eq_T_htf_loop_out(C_csp_fresnel_collector_receiver* pc_fresnel, const C_csp_weatherreader::S_outputs& weather,
            double T_htf_cold_in /*K*/, const C_csp_solver_sim_info& sim_info)
        {
            mpc_fresnel = pc_fresnel;
            ms_weather = weather;
            m_T_cold_in = T_htf_cold_in;	//[K]
            ms_sim_info = sim_info;
        }

        virtual int operator()(double m_dot_htf_loop /*kg/s*/, double* T_htf_loop_out /*K*/);
    };

    class C_mono_eq_freeze_prot_E_bal : public C_monotonic_equation
    {	// The solver chooses a cold inlet temperature and sends it to the operator. The operator
        //		call the loop energy balance at the recirculation mass flow rate
        //		and returns the total field heat loss. The solver finds the T_cold_in such that E_fp_htf = E_losses
    private:
        C_csp_fresnel_collector_receiver* mpc_fresnel;
        C_csp_weatherreader::S_outputs ms_weather;
        double m_m_dot_loop;					//[kg/s]
        C_csp_solver_sim_info ms_sim_info;

    public:

        double m_Q_htf_fp;	//[MJ]

        C_mono_eq_freeze_prot_E_bal(C_csp_fresnel_collector_receiver* pc_fresnel, const C_csp_weatherreader::S_outputs& weather,
            double m_dot_loop /*kg/s*/, const C_csp_solver_sim_info& sim_info)
        {
            mpc_fresnel = pc_fresnel;
            ms_weather = weather;
            m_m_dot_loop = m_dot_loop;	//[kg/s]
            ms_sim_info = sim_info;

            m_Q_htf_fp = std::numeric_limits<double>::quiet_NaN();
        }

        virtual int operator()(double T_htf_cold_in /*K*/, double* E_loss_balance /*-*/);
    };

    

};



#endif
