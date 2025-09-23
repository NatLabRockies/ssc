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


#ifndef __csp_solver_pt_receiver_
#define __csp_solver_pt_receiver_

#include "csp_solver_util.h"

#include "htf_props.h"
#include "csp_solver_core.h"

class C_pt_receiver
{
// The abstract parent class for all receivers, including the original steady-state and the transient receiver

public:
    virtual ~C_pt_receiver() {};

    C_csp_messages csp_messages;        // Class to save messages for upstream classes

    struct S_inputs
    {
        double m_plant_defocus;     //[-] plant defocus signal
        C_csp_collector_receiver::E_csp_cr_modes m_input_operation_mode;			                //[-] operating mode of collector receiver, corresponding to enum C_csp_collector_receiver::E_csp_cr_modes
        const util::matrix_t<double> *m_flux_map_input;		//[-] flux values for each receiver surface node, as fraction of an evenly distributed irradiance
        double m_clearsky_dni;      //[W/m2]

        S_inputs()
        {
            m_plant_defocus = std::numeric_limits<double>::quiet_NaN();
            m_input_operation_mode = C_csp_collector_receiver::E_csp_cr_modes::OFF;
            m_clearsky_dni = std::numeric_limits<double>::quiet_NaN();
        }
    };

    struct S_outputs
    {
        double m_m_dot_salt_tot;		//[kg/hr] HTF mass flow through receiver
        double m_eta_therm;				//[-] receiver thermal efficiency
        double m_W_dot_pump;			//[MWe] HTF pumping power
        double m_q_conv_sum;			//[MWt] total receiver convection losses
        double m_q_rad_sum;				//[MWt] total receiver radiation losses
        double m_Q_thermal;				//[MWt] thermal power delivered to TES/PC: subtracts piping losses (q_dot_rec - q_dot_piping_losses)
        double m_T_salt_hot;			//[C] HTF outlet temperature, includes downcomer piping losses
        double m_component_defocus;		//[-] defocus applied by receiver to stay within mass flow or other constraints

        //[MWt] receiver incident thermal power before defocus
        // -- *cavity* receiver currently reports this as nan
        double m_q_dot_rec_inc_pre_defocus;

        //[MWt] receiver incident thermal power after all defocus
        // -- external receiver applies reflection to flux maps, so m_q_dot_rec_inc is after reflection losses
        // -- cavity receiver applies reflection in receiver thermal model, so m_q_dot_rec_inc is before reflection losses
        double m_q_dot_rec_inc;

        //[MWt] receiver reflection losses
        // -- external receiver applies reflection to flux maps, so reports this value as 0
        // -- cavity receiver applies reflection in receiver thermal model, so reports this value
        double m_q_dot_refl_loss;       //[MWt] Reflection losses (0 for external receiver - instead included in opt efficiency)

        double m_q_startup;				//[MWt-hr] thermal energy used to start receiver
        double m_dP_receiver;			//[bar] receiver pressure drop
        double m_dP_total;				//[bar] total pressure drop
        double m_ratio_dP_tower_to_rec; //[-] ratio of total pressure drop that is caused by tower height
        double m_vel_htf;				//[m/s] HTF flow velocity through receiver tubes
        double m_T_salt_cold;			//[C] HTF inlet temperature
        double m_time_required_su;		//[s] time it took receiver to startup
        double m_q_dot_piping_loss;		//[MWt] thermal power lost from piping to surroundings
        double m_q_heattrace;			//[MWt-hr] Power required for heat tracing
        double m_inst_T_salt_hot;		//[C] Instantaneous HTF outlet T at the end of the time step
        double m_max_T_salt_hot;		//[C] Maximum HTF outlet T during the time step
        double m_min_T_salt_hot;		//[C] Minimum HTF outlet T during the time step
        double m_max_rec_tout;			//[C] Maximum HTF T (at the receiver outlet before downcomer piping loss) during the time step
        double m_Twall_inlet;			//[C] Average receiver wall temperature at inlet
        double m_Twall_outlet;			//[C] Average receiver wall temperature at receiver outlet
        double m_Triser;				//[C] Average riser wall temperature at inlet
        double m_Tdownc;				//[C] Average downcomer wall temperature at outlet

		double m_Q_thermal_csky_ss;		//[MWt]  Steady-state thermal power delivered to TES/PC if DNI is equal to clear-sky DNI 
		double m_Q_thermal_ss;			//[MWt] Steady-state thermal power delivered to TES/PC 

        S_outputs()
        {
            clear();
        }

        void clear()
        {
            m_m_dot_salt_tot = m_eta_therm = m_W_dot_pump = m_q_conv_sum = m_q_rad_sum = m_Q_thermal =
                m_T_salt_hot = m_component_defocus =
                m_q_dot_rec_inc_pre_defocus = m_q_dot_rec_inc = m_q_startup =
                m_dP_receiver = m_dP_total = m_ratio_dP_tower_to_rec = m_vel_htf = m_T_salt_cold =
                m_time_required_su = m_q_dot_piping_loss = m_q_heattrace = std::numeric_limits<double>::quiet_NaN();

			m_inst_T_salt_hot = m_max_T_salt_hot = m_min_T_salt_hot = m_max_rec_tout = m_Twall_inlet = m_Twall_outlet = 
				m_Triser = m_Tdownc = m_Q_thermal_csky_ss = m_Q_thermal_ss = std::numeric_limits<double>::quiet_NaN();
        }
    };

    S_outputs outputs;
    S_outputs ms_outputs;

    virtual void init();

    C_csp_collector_receiver::E_csp_cr_modes get_operating_state();

    virtual void call(const C_csp_weatherreader::S_outputs &weather,
        const C_csp_solver_htf_1state &htf_state_in,
        const C_pt_receiver::S_inputs &inputs,
        const C_csp_solver_sim_info &sim_info) = 0;

    virtual void off(const C_csp_weatherreader::S_outputs &weather,
        const C_csp_solver_htf_1state &htf_state_in,
        const C_csp_solver_sim_info &sim_info) = 0;

    virtual void converged() = 0;

    void get_converged_values(C_csp_collector_receiver::E_csp_cr_modes& m_mode_final,
        double& E_su_final, double& t_su_final);

    virtual double get_pumping_parasitic_coef() = 0;

    HTFProperties *get_htf_property_object();

    virtual double get_startup_time();   //[s]

    virtual double get_startup_energy(); //[MWh]

    virtual double area_proj() = 0; //[m^2]

    double estimate_thermal_efficiency(const C_csp_weatherreader::S_outputs& weather, double q_inc);

    double get_min_power_delivery(); //[MWt]

    double get_max_power_delivery(); //[MWt]

    double get_T_htf_cold_des();    //[K]

    double get_q_dot_rec_des();     //[MWt]

    double get_design_rec_pumping_power();  //[MWe]

    void get_design_geometry(double& L_tower_piping /*m*/, double& od_tube_calc /*m*/);

    void get_design_performance(double& eta_thermal /*-*/,
        double& W_dot_rec_pump /*MWe*/, double& W_dot_pumping_tower_share /*MWe*/, double& W_dot_pumping_rec_share /*MWe*/,
        double& rec_pump_coef /*MWe/MWt*/, double& rec_vel_htf_des /*m/s*/,
        double& m_dot_htf_rec /*kg/s*/, double& m_dot_htf_max /*kg/s*/,
        double& q_dot_piping_loss_des /*MWt*/);

protected:

    C_pt_receiver(double h_tower /*m*/, double m_epsilon /*-*/,
                double T_htf_hot_des /*C*/, double T_htf_cold_des /*C*/,
                double f_rec_min /*-*/, double q_dot_rec_des /*MWt*/,
                double rec_su_delay /*hr*/, double rec_qf_delay /*-*/,
                double m_dot_htf_max_frac /*-*/, double eta_pump /*-*/,
                double od_tube /*mm*/, double th_tube /*mm*/,
                double piping_loss_coef /*Wt/m2-K*/, double pipe_length_add /*m*/,
                double pipe_length_mult /*-*/,
                int field_fl /*-*/, util::matrix_t<double> field_fl_props,
                int tube_mat_code /*-*/,
                int night_recirc /*-*/);

    // *******************************************
    // Base class design parameters
    double m_h_tower;				    //[m] height of the tower
    double m_epsilon;				    //[-] emissivity of the receiver panels
    double m_T_htf_hot_des;			    //[K] hot outlet HTF temperature at design, converted from C in constructor
    double m_T_htf_cold_des;		    //[K] cold inlet HTF temperature at design, converted from C in constructor
    double m_f_rec_min;				    //[-] minimum receiver thermal output as fraction of design
    double m_q_rec_des;				    //[MW] design recever thermal output, converted to [W] in init()
    double m_rec_su_delay;			    //[hr] required startup time
    double m_rec_qf_delay;			    //[-] required startup energy as fraction of design thermal output
    double m_m_dot_htf_max_frac;	    //[-] maximum receiver HTF mass flow as fraction of design mass flow
    double m_eta_pump;					//[-] HTF pump efficiency
    double m_od_tube;				    //[m], converted from mm in constructor
    double m_th_tube;				    //[m], converted from mm in constructor

    double m_piping_loss_coefficient;   //[Wt/m2-K]
    double m_pipe_length_add;		//[m]
    double m_pipe_length_mult;		//[-]

    int m_field_fl;
    util::matrix_t<double> m_field_fl_props;

    int m_tube_mat_code;                //[-]

    int m_night_recirc;					//[-] 1=receiver is circulating HTF at night, otherwise not

    // Design ambient conditions
    double m_T_amb_des;       //[K]
    double m_T_sky_des;       //[K]
    double m_v_wind_10_des;   //[m/s]
    double m_P_amb_des;       //[Pa]

    // Calculated design geometry/dimensions
    double m_L_piping;          //[m]
    double m_Q_dot_piping_loss;	//[Wt] = Constant thermal losses from piping to env. = (THT*length_mult + length_add) * piping_loss_coef

    // Calculated design point performance
    double m_q_dot_inc_min;             //[Wt] minimum receiver thermal power
    double m_eta_thermal_des_calc;      //[-]
    double m_W_dot_rec_pump_des_calc;   //[MWe]
    double m_W_dot_pumping_tower_share; //[MWe]
    double m_W_dot_pumping_rec_share;   //[MWe]
    double m_rec_pump_coef;             //[MWe/MWt]
    double m_vel_htf_des;		        //[m/s] HTF flow velocity through receiver tubes
    double m_m_dot_htf_des;             //[kg/s] receiver HTF mass flow at design
    double m_m_dot_htf_max;             //[kg/s] receiver HTF max mass flow rate

    // *******************************************
    // *******************************************

    HTFProperties field_htfProps;       // heat transfer fluid properties
    HTFProperties tube_material;		// receiver tube material
    HTFProperties ambient_air;			// ambient air properties

    C_csp_collector_receiver::E_csp_cr_modes m_mode;                         //[-] current operating mode of receiver
    C_csp_collector_receiver::E_csp_cr_modes m_mode_prev;                    //[-] operating mode of receiver at end of last converged timestep

    // ************************************
    // State variables
    double m_E_su_prev;         //[W-hr] Startup energy required at end of previous timestep
    double m_E_su;              //[W-hr] Startup energy required calculated at end of current timestep
    double m_t_su;              //[hr] Startup time requirement at end of previous timestep
    double m_t_su_prev;         //[hr] Startup time requirement calculated at end of current timestep

    std::string error_msg;              // member string for exception messages

};

#endif  // __csp_solver_pt_receiver_
