#include "csp_solver_fresnel_collector_receiver.h"
#include "Toolbox.h"

using namespace std;

static C_csp_reported_outputs::S_output_info S_output_info[] =
{
    {C_csp_fresnel_collector_receiver::E_EQUIV_OPT_ETA_TOT, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_csp_fresnel_collector_receiver::E_DEFOCUS, C_csp_reported_outputs::TS_WEIGHTED_AVE},

    {C_csp_fresnel_collector_receiver::E_Q_DOT_INC_SF_TOT, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_csp_fresnel_collector_receiver::E_Q_DOT_INC_SF_COSTH, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_csp_fresnel_collector_receiver::E_Q_DOT_REC_INC, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_csp_fresnel_collector_receiver::E_Q_DOT_REC_THERMAL_LOSS, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_csp_fresnel_collector_receiver::E_REC_THERMAL_EFF, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_csp_fresnel_collector_receiver::E_Q_DOT_REC_ABS, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_csp_fresnel_collector_receiver::E_Q_DOT_PIPING_LOSS, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_csp_fresnel_collector_receiver::E_E_DOT_INTERNAL_ENERGY, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_csp_fresnel_collector_receiver::E_Q_DOT_HTF_OUT, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_csp_fresnel_collector_receiver::E_Q_DOT_FREEZE_PROT, C_csp_reported_outputs::TS_WEIGHTED_AVE},

    {C_csp_fresnel_collector_receiver::E_TIME_IN_STARTUP, C_csp_reported_outputs::SUMMED},

    {C_csp_fresnel_collector_receiver::E_M_DOT_LOOP, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_csp_fresnel_collector_receiver::E_IS_RECIRCULATING, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_csp_fresnel_collector_receiver::E_M_DOT_FIELD_RECIRC, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_csp_fresnel_collector_receiver::E_M_DOT_FIELD_DELIVERED, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_csp_fresnel_collector_receiver::E_T_FIELD_COLD_IN, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_csp_fresnel_collector_receiver::E_T_REC_COLD_IN, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_csp_fresnel_collector_receiver::E_T_REC_HOT_OUT, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_csp_fresnel_collector_receiver::E_T_FIELD_HOT_OUT, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_csp_fresnel_collector_receiver::E_PRESSURE_DROP, C_csp_reported_outputs::TS_WEIGHTED_AVE},

    {C_csp_fresnel_collector_receiver::E_W_DOT_SCA_TRACK, C_csp_reported_outputs::TS_WEIGHTED_AVE},
    {C_csp_fresnel_collector_receiver::E_W_DOT_PUMP, C_csp_reported_outputs::TS_WEIGHTED_AVE},

    csp_info_invalid
};


// ---------------------------------------------------------------------------- PRIVATE

int C_csp_fresnel_collector_receiver::freeze_protection(const C_csp_weatherreader::S_outputs& weather,
    double& T_cold_in /*K*/, double m_dot_loop /*kg/s*/,
    const C_csp_solver_sim_info& sim_info, double& Q_fp /*MJ*/)
{
    C_mono_eq_freeze_prot_E_bal c_freeze_protection_eq(this, weather, m_dot_loop, sim_info);
    C_monotonic_eq_solver c_fp_solver(c_freeze_protection_eq);

    // Set upper and lower bounds on T_htf_cold_in
    double T_htf_cold_in_lower = T_cold_in;		//[K]
    double T_htf_cold_in_upper = std::numeric_limits<double>::quiet_NaN();	//[K]

    // Set two initial guess values
    double T_htf_guess_lower = (m_Q_field_losses_total_subts / sim_info.ms_ts.m_step) * 1.E6 /
        (m_c_htf_ave_ts_ave_temp * m_m_dot_htf_tot) + T_cold_in;	//[K]

    double T_htf_guess_upper = T_htf_guess_lower + 10.0;		//[K]

    // Set solver settings - relative error on E_balance
    c_fp_solver.settings(0.01, 30, T_htf_cold_in_lower, T_htf_cold_in_upper, false);

    int iter_solved = -1;
    double tol_solved = std::numeric_limits<double>::quiet_NaN();

    int fp_code = 0;
    double T_cold_in_solved = std::numeric_limits<double>::quiet_NaN();

    try
    {
        fp_code = c_fp_solver.solve(T_htf_guess_lower, T_htf_guess_upper, 0.0, T_cold_in_solved, tol_solved, iter_solved);
    }
    catch (C_csp_exception)
    {
        throw(C_csp_exception("C_csp_fresnel_collector::off - freeze protection failed"));
    }

    if (fp_code != C_monotonic_eq_solver::CONVERGED)
    {
        throw(C_csp_exception("C_csp_fresnel_collector::off - freeze protection failed to converge"));
    }

    T_cold_in = T_cold_in_solved;				//[K]
    Q_fp = c_freeze_protection_eq.m_Q_htf_fp;	//[MJ]

    return fp_code;
}

double C_csp_fresnel_collector_receiver::field_pressure_drop(double T_db, double m_dot_field, double P_field_in,
    const std::vector<double>& T_in_SCA, const std::vector<double>& T_out_SCA)
{
    std::vector<double> DP_tube(m_nMod, 0.);

    double DP_IOCOP = 0;
    double DP_loop_tot = 0;
    double DP_toField = 0;
    double DP_fromField = 0;
    double DP_hdr_cold = 0;
    double DP_hdr_hot = 0;
    double m_dot_hdr_in, m_dot_hdr, m_dot_temp;
    double rho_hdr_cold;

    double m_dot_htf = m_dot_field / (double)m_nLoops;
    double T_loop_in = T_in_SCA[0];
    double T_loop_out = T_out_SCA[m_nMod - 1];

    //handle loop pressure drop based on the heat loss model selection
    switch (m_rec_model)
    {

        //Polynomial heat loss model
        case 1:
        {
            /*
            This option doesn't require specific knowledge about absorber geometry, so we cannot calculate
            pressure drop from first principles. Instead use coefficients and scaling polynomials provided
            by the user.
            */

            DP_loop_tot = (float)m_nMod * m_DP_nominal * CSP::poly_eval(m_dot_htf / m_m_dot_design, &m_DP_coefs[0], m_DP_coefs.size());

            break;
        }

        //Evacuated tube receiver model
        case 2:
        {
            //------Inlet, Outlet, and COP

            DP_IOCOP = PressureDrop(m_dot_htf, (T_loop_in + T_loop_out) / 2.0, 1.0, m_D_h.at(0),
                m_HDR_rough, (40. + m_L_crossover), 0.0, 0.0, 2.0, 0.0, 0.0, 2.0, 0.0, 0.0, 2.0, 1.0, 0.0);
            
            //-------HCE's
            DP_tube.resize(m_nMod, 0);
            for (int j = 0; j < m_nRecVar; j++) {
                for (int i = 0; i < m_nMod; i++) {

                    //Account for extra fittings on the first HCE
                    double x1, x2;
                    if (i == 0) {
                        x1 = 10.0;
                        x2 = 3.0;
                    }
                    else {
                        x1 = 0.0;
                        x2 = 1.0;
                    }

                    double T_htf_ave = (T_in_SCA[i] + T_out_SCA[i]) / 2;
                    DP_tube[i] += PressureDrop(m_dot_htf, T_htf_ave, 1.0, m_D_h.at(j), (m_Rough[j] * m_D_h.at(j)),
                        (m_L_mod + m_L_mod_spacing), 0.0, 0.0, x1, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, x2) * m_HCE_FieldFrac[j];
                    //if(ErrorFound()) return 1
                }
            }
            //The pressure drop only across the loop
            double DP_loop = 0.;
            for (int j = 0; j < m_nMod; j++)
                DP_loop += DP_tube[j];

            DP_loop_tot = DP_IOCOP + DP_loop;

            break;
        }
            

        default:
            //error
            //message(TCS_ERROR, "No such heat loss model. Error in loop pressure drop calculations.");
            string msg = "No such heat loss model. Error in loop pressure drop calculations.";
            m_error_msg = util::format(msg.c_str());
            mc_csp_messages.add_message(C_csp_messages::NOTICE, m_error_msg);
            return -1;
    }


    //-------SGS to field section
    m_m_dot_htf_tot = m_dot_htf * float(m_nLoops);
    double m_dot_run_in = std::numeric_limits<double>::quiet_NaN();
    if (m_nfsec > 2) {  //mjw 5.4.11 Correct the mass flow for situations where nfsec/2==odd
        m_dot_run_in = m_m_dot_htf_tot / 2.0 * (1. - float(m_nfsec % 4) / float(m_nfsec));
    }
    else {
        m_dot_run_in = m_m_dot_htf_tot / 2.0;
    }
    double x3 = float(m_nrunsec) - 1.0;  //Number of contractions/expansions
    m_dot_temp = m_dot_run_in;
    DP_toField = 0.0;
    DP_fromField = 0.0;
    for (int i = 0; i < m_nrunsec; i++) {
        double rnr_toField = PressureDrop(m_dot_temp, T_loop_in, 1.0, m_D_runner[i], m_HDR_rough, m_L_runner[i], 0.0, x3, 0.0, 0.0,
            max(float(CSP::nint(m_L_runner[i] / 70.)) * 4., 8.), 1.0, 0.0, 1.0, 0.0, 0.0, 0.0);    //Correct for less than all mass flow passing through each section
        //if(ErrorFound()) return 1                  
        //-------SGS from field section
        double rnr_fromField = PressureDrop(m_dot_temp, T_loop_out, 1.0, m_D_runner[i], m_HDR_rough, m_L_runner[i], x3, 0.0, 0.0, 0.0,
            max(float(CSP::nint(m_L_runner[i] / 70.)) * 4., 8.), 1.0, 0.0, 0.0, 0.0, 0.0, 0.0);   //Correct for less than all mass flow passing through each section
        //if(ErrorFound()) return 1

        DP_toField += rnr_toField;
        DP_fromField += rnr_fromField;

        m_DP_rnr[i] = rnr_toField;
        m_DP_rnr[2 * m_nrunsec - i - 1] = rnr_fromField;

        if (i > 1) m_dot_temp = max(m_dot_temp - 2. * m_m_dot_htf_tot / float(m_nfsec), 0.0);
    }



    double m_dot_header_in = m_m_dot_htf_tot / float(m_nfsec);
    double m_dot_header = m_dot_header_in;
    DP_hdr_cold = 0.0;
    DP_hdr_hot = 0.0;
    for (int i = 0; i < m_nhdrsec; i++) {
        //Determine whether the particular section has an expansion valve
        double x2 = 0.0;
        if (i > 0) {
            if (m_D_hdr[i] != m_D_hdr[i - 1]) x2 = 1.;
        }

        //Calculate pressure drop in cold header and hot header sections.. both use similar information

        // COLD header
        double dp_hdr_cold = PressureDrop(m_dot_header, T_loop_in, 1.0, m_D_hdr[i], m_HDR_rough,
            (m_L_crossover + 4.275) * 2., 0.0, x2, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0); //*m_dot_header/m_dot_header_in  //mjw/tn 1.25.12 already account for m_dot_header in function call //mjw 5.11.11 scale by mass flow passing though
        m_DP_hdr[i] = dp_hdr_cold;
        DP_hdr_cold += dp_hdr_cold;

        // HOT header
        double dp_hdr_hot = PressureDrop(m_dot_header, T_loop_out, 1.0, m_D_hdr[i], m_HDR_rough,
            (m_L_crossover + 4.275) * 2., x2, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0); //*m_dot_header/m_dot_header_in  //mjw 5.11.11
        m_DP_hdr[2 * m_nhdrsec - i - 1] = dp_hdr_hot;
        DP_hdr_hot += dp_hdr_hot;
        
        //Siphon off header mass flow rate at each loop.  Multiply by 2 because there are 2 loops per hdr section
        m_dot_header = max(m_dot_header - 2. * m_dot_htf, 0.0);

    }




    // The total pressure drop in all of the piping
    m_dP_total = (DP_loop_tot + DP_hdr_cold + DP_hdr_hot + DP_fromField + DP_toField);

    // Convert pressure drops to gauge pressures
    m_P_rnr[0] = m_dP_total;
    for (int i = 1; i < 2 * m_nrunsec; i++) {
        m_P_rnr[i] = m_P_rnr[i - 1] - m_DP_rnr[i - 1];
        if (i == m_nrunsec) { m_P_rnr[i] -= (DP_hdr_cold + DP_loop_tot + DP_IOCOP + DP_hdr_hot); }
    }
    
    m_P_hdr[0] = m_P_rnr[m_nrunsec - 1] - m_DP_rnr[m_nrunsec - 1];    // report pressures for farthest subfield
    for (int i = 1; i < 2 * m_nhdrsec; i++) {
        m_P_hdr[i] = m_P_hdr[i - 1] - m_DP_hdr[i - 1];
        if (i == m_nhdrsec) { m_P_hdr[i] -= (DP_loop_tot + DP_IOCOP); }
    }

    // Polynomial does not have this information
    //m_P_loop[0] = m_P_hdr[m_nhdrsec - 1] - m_DP_hdr[m_nhdrsec - 1];   // report pressures for farthest loop
    //for (int i = 1; i < m_nMod; i++) {
    //    m_P_loop[i] = m_P_loop[i - 1] - m_DP_loop[i - 1];
    //}

    // The total pumping power consumption
    rho_hdr_cold = m_htfProps.dens((T_in_SCA[0] + T_out_SCA[m_nMod - 1]) / 2, P_field_in);
    m_W_dot_pump = m_dP_total * m_dot_field / (rho_hdr_cold * m_eta_pump) / 1.e6;  //[MW]


    m_dP_total *= 1.E-5;		//[bar], convert from Pa

    return m_dP_total;
}

void C_csp_fresnel_collector_receiver::set_output_value()
{
    mc_reported_outputs.value(E_EQUIV_OPT_ETA_TOT, m_EqOpteff * m_ftrack);		//[-]
    mc_reported_outputs.value(E_DEFOCUS, m_control_defocus * m_component_defocus);	//[-]

    mc_reported_outputs.value(E_Q_DOT_INC_SF_TOT, m_q_dot_inc_sf_tot);			//[MWt]

    mc_reported_outputs.value(E_Q_DOT_REC_INC, m_q_dot_sca_abs_summed_fullts + m_q_dot_sca_loss_summed_fullts
        + m_q_dot_sca_refl_summed_fullts);	//[MWt] 09.08.2023 tmb: add reflective losses (due to absorber absorptance) to receiver incident power
    mc_reported_outputs.value(E_Q_DOT_REC_THERMAL_LOSS, m_q_dot_sca_loss_summed_fullts);			//[MWt]
    mc_reported_outputs.value(E_Q_DOT_REC_ABS, m_q_dot_sca_abs_summed_fullts);						//[MWt]

    double rec_Q_inc = m_q_dot_sca_abs_summed_fullts + m_q_dot_sca_loss_summed_fullts;
    double rec_Q_abs = m_q_dot_sca_abs_summed_fullts;
    double rec_thermal_eff = rec_Q_inc == 0 ? 0 : rec_Q_abs / rec_Q_inc;
    mc_reported_outputs.value(E_REC_THERMAL_EFF, rec_thermal_eff);

    mc_reported_outputs.value(E_Q_DOT_PIPING_LOSS, m_q_dot_xover_loss_summed_fullts +
        m_q_dot_HR_cold_loss_fullts +
        m_q_dot_HR_hot_loss_fullts);		//[MWt]
    mc_reported_outputs.value(E_E_DOT_INTERNAL_ENERGY, m_E_dot_sca_summed_fullts +
        m_E_dot_xover_summed_fullts +
        m_E_dot_HR_cold_fullts +
        m_E_dot_HR_hot_fullts);			//[MWt]
    mc_reported_outputs.value(E_Q_DOT_HTF_OUT, m_q_dot_thermal_reported);				//[MWt]
    mc_reported_outputs.value(E_Q_DOT_FREEZE_PROT, m_q_dot_freeze_protection);			//[MWt]

    mc_reported_outputs.value(E_M_DOT_LOOP, m_m_dot_htf_tot / (double)m_nLoops);		//[kg/s]
    mc_reported_outputs.value(E_IS_RECIRCULATING, m_is_m_dot_recirc);		    //[-]
    if (m_is_m_dot_recirc)
    {
        mc_reported_outputs.value(E_M_DOT_FIELD_RECIRC, m_m_dot_htf_tot);		//[kg/s]
        mc_reported_outputs.value(E_M_DOT_FIELD_DELIVERED, 0.0);				//[kg/s]
    }
    else
    {
        mc_reported_outputs.value(E_M_DOT_FIELD_RECIRC, 0.0);					//[kg/s]
        mc_reported_outputs.value(E_M_DOT_FIELD_DELIVERED, m_m_dot_htf_tot);	//[kg/s]
    }

    mc_reported_outputs.value(E_T_FIELD_COLD_IN, m_T_sys_c_t_int_fullts - 273.15);			//[C]
    mc_reported_outputs.value(E_T_REC_COLD_IN, m_T_htf_c_rec_in_t_int_fullts - 273.15);		//[C]
    mc_reported_outputs.value(E_T_REC_HOT_OUT, m_T_htf_h_rec_out_t_int_fullts - 273.15);	//[C]
    mc_reported_outputs.value(E_T_FIELD_HOT_OUT, m_T_sys_h_t_int_fullts - 273.15);			//[C]
    mc_reported_outputs.value(E_PRESSURE_DROP, m_dP_total);		//[bar]

    mc_reported_outputs.value(E_W_DOT_SCA_TRACK, m_W_dot_sca_tracking);		//[MWe]
    mc_reported_outputs.value(E_W_DOT_PUMP, m_W_dot_pump);					//[MWe]

    mc_reported_outputs.value(E_TIME_IN_STARTUP, m_time_at_startup / 60.0);         //[min] convert from s

    return;
}

// ------------------------------------------------------------------- PRIVATE SUPPLEMENTAL

C_csp_fresnel_collector_receiver::E_loop_energy_balance_exit C_csp_fresnel_collector_receiver::loop_energy_balance_T_t_int(const C_csp_weatherreader::S_outputs& weather,
    double T_htf_cold_in /*K*/, double m_dot_htf_loop /*kg/s*/,
    const C_csp_solver_sim_info& sim_info)
{
    // Calculate total field mass flow rate
    m_m_dot_htf_tot = m_dot_htf_loop * float(m_nLoops);

    // Helpful Variables
    double dt = sim_info.ms_ts.m_step;
    double T_db = weather.m_tdry + 273.15;		//[K] Dry bulb temperature, convert from C
    double T_dp = weather.m_twet + 273.15;		//[K] Dew point temperature, convert from C

    // Calculate effective sky temperature
    double hour = fmod(sim_info.ms_ts.m_time / 3600.0, 24.0);		//[hr] Hour of day
    double T_sky;	//[K] Effective sky temperature
    {
        if (T_dp > -300.0)
            T_sky = CSP::skytemp(T_db, T_dp, hour);				//[K] Effective sky temperature 
        else
            T_sky = T_db - 20.0;
    }
    

    double q_dot_loss_HR_cold = 0.0;		//[W]
    double E_HR_cold = 0.0;					//[MJ] 
    double E_HR_cold_htf = 0.0;				//[MJ]
    double E_HR_cold_losses = 0.0;			//[MJ]
    double E_HR_cold_bal = 0.0;				//[MJ]

    // Header Properties
    double rho_hdr_cold = m_htfProps.dens(m_T_sys_c_t_end_last, 1.);	    //[kg/m^3]
    double rho_hdr_hot = m_htfProps.dens(m_T_sys_h_t_end_last, 1.);		    //[kg/m^3]
    double c_hdr_cold_last = m_htfProps.Cp(m_T_sys_c_t_end_last) * 1000.0;	//[J/kg-K] mjw 1.6.2011 Adding mc_bal to the cold header inertia

    // BULK Temperature calculations
    {
        // This value is the Bulk Temperature at the *end* of the timestep (type 262 line 1551; trough line 1187)
        m_T_sys_c_t_end = (m_T_sys_c_t_end_last - T_htf_cold_in) * exp(-(m_dot_htf_loop * float(m_nLoops))
            / (m_v_cold * rho_hdr_cold + m_mc_bal_cold / c_hdr_cold_last) * dt) + T_htf_cold_in;

        // Try calculating a timestep-average Bulk Temperature (and assume it is the outlet)
        // This is from trough (line 1189)
        m_T_sys_c_t_int = T_htf_cold_in + ((m_v_cold * rho_hdr_cold + m_mc_bal_cold / c_hdr_cold_last) / (-m_dot_htf_loop * float(m_nLoops))) *
            (m_T_sys_c_t_end_last - T_htf_cold_in) * (exp(-(m_dot_htf_loop * float(m_nLoops)) / (m_v_cold * rho_hdr_cold + m_mc_bal_cold / c_hdr_cold_last) * sim_info.ms_ts.m_step) - 1.0)
            / sim_info.ms_ts.m_step;
    }

    // Current Header Properties
    double m_cp_sys_c_t_int = m_htfProps.Cp(m_T_sys_c_t_int) * 1000.0;    //[kg/m^3]   (c_hdr_cold)

    //Consider heat loss from cold piping
    //Runner
    {
        m_Runner_hl_cold = 0.0;
        m_Runner_hl_cold_tot = 0.0;
        m_T_rnr[0] = m_T_sys_c_t_int;
        for (int i = 0; i < m_nrunsec; i++)
        {
            if (i != 0) {
                m_T_rnr[i] = m_T_rnr[i - 1] - m_Runner_hl_cold / (m_dot_runner(m_m_dot_htf_tot, m_nfsec, i - 1) * m_cp_sys_c_t_int);
            }
            m_Runner_hl_cold = m_L_runner[i] * CSP::pi * m_D_runner[i] * m_Pipe_hl_coef * (m_T_sys_c_t_end - T_db);  //[W]
            m_Runner_hl_cold_tot += m_Runner_hl_cold;
        }
    }
    //Header
    {
        m_Header_hl_cold = 0.0;
        m_Header_hl_cold_tot = 0.0;

        m_T_hdr[0] = m_T_rnr[m_nrunsec - 1] - m_Runner_hl_cold / (m_dot_runner(m_m_dot_htf_tot, m_nfsec, m_nrunsec - 1) * m_cp_sys_c_t_int);  // T's for farthest headers
        for (int i = 0; i < m_nhdrsec; i++)
        {
            if (i != 0) {
                m_T_hdr[i] = m_T_hdr[i - 1] - m_Header_hl_cold / (m_dot_header(m_m_dot_htf_tot, m_nfsec, m_nLoops, i - 1) * m_cp_sys_c_t_int);
            }
            m_Header_hl_cold = m_L_crossover * m_D_hdr[i] * CSP::pi * m_Pipe_hl_coef * (m_T_sys_c_t_end - T_db);  //[W]
            m_Header_hl_cold_tot += m_Header_hl_cold;
        }
    }
    double Pipe_hl_cold = m_Header_hl_cold_tot + m_Runner_hl_cold_tot;
    
    q_dot_loss_HR_cold = m_Header_hl_cold + m_Runner_hl_cold;	//[W]
    E_HR_cold_losses = q_dot_loss_HR_cold * sim_info.ms_ts.m_step / 1.E6;	//[MJ]

    // Internal energy change in cold runners/headers. Positive means it has gained energy (temperature)
    E_HR_cold = (m_v_cold * rho_hdr_cold * m_cp_sys_c_t_int + m_mc_bal_cold) * (m_T_sys_c_t_end - m_T_sys_c_t_end_last) * 1.E-6;		//[MJ]
    E_HR_cold_htf = m_dot_htf_loop * float(m_nLoops) * m_cp_sys_c_t_int * (m_T_htf_in_t_int[0] - T_htf_cold_in) * sim_info.ms_ts.m_step / 1.E6;	//[MJ]
    E_HR_cold_bal = -E_HR_cold_losses - E_HR_cold_htf - E_HR_cold;		//[MJ]

    // Calculate Loop Inlet temperature (follows original fresnel calculation)
    double trough_m_T_loop_in = m_T_hdr[m_nhdrsec - 1] - m_Header_hl_cold / (m_dot_header(m_m_dot_htf_tot, m_nfsec, m_nLoops, m_nhdrsec - 1) * m_cp_sys_c_t_int); // trough calculation
    m_T_loop_in = m_T_sys_c_t_end - Pipe_hl_cold / (m_dot_htf_loop * m_nLoops * m_cp_sys_c_t_int); // original fresnel calculation
    m_T_loop[0] = m_T_loop_in;
    m_T_htf_in_t_int[0] = m_T_loop_in;

    // Reset vectors that are populated in following for(i..nSCA) loop
    {
        m_q_abs_SCAtot.assign(m_q_abs_SCAtot.size(), 0.0);
        m_q_loss_SCAtot.assign(m_q_loss_SCAtot.size(), 0.0);
        m_q_1abs_tot.assign(m_q_1abs_tot.size(), 0.0);
        m_q_reflect_tot.assign(m_q_reflect_tot.size(), 0.0);
        m_E_avail.assign(m_E_avail.size(), 0.0);
        m_E_accum.assign(m_E_accum.size(), 0.0);
        m_E_int_loop.assign(m_E_int_loop.size(), 0.0);
        // And single values...
        m_EqOpteff = 0.0;
    }

    // Vectors storing information for the energy balance
    std::vector<double> E_sca, E_sca_htf, E_sca_abs, E_sca_bal;		//[MJ]
    std::vector<double> E_xover, E_xover_htf, E_xover_abs, E_xover_bal;
    {
        E_sca.resize(m_nMod);
        E_sca_htf.resize(m_nMod);
        E_sca_abs.resize(m_nMod);
        E_sca_bal.resize(m_nMod);

        E_xover.resize(m_nMod - 1);
        E_xover_htf.resize(m_nMod - 1);
        E_xover_abs.resize(m_nMod - 1);
        E_xover_bal.resize(m_nMod - 1);
    }

    std::vector<double> q_dot_loss_xover;		//[W]
    q_dot_loss_xover.resize(m_nMod - 1);
    
    double q_inc_total = 0.;
    double q_abs_abs_total = 0.;
    double q_abs_htf_total = 0.;
    std::vector<double> EqOpteffs(m_nMod, 0.);

    // MAIN SCA Temperature Solve
    for (int i = 0; i < m_nMod; i++)
    {
        m_q_loss.assign(m_q_loss.size(), 0.0);		//[W/m]
        m_q_abs.assign(m_q_abs.size(), 0.0);		//[W/m]
        m_q_1abs.assign(m_q_1abs.size(), 0.0);		//[W/m]

        double c_htf_i = 0.0;		                //[J/kg-K]
        double rho_htf_i = 0.0;		                //[kg/m^3]

        double dT_loc, m_node, T_node_ave, errhl;
        switch (m_rec_model)
        {
            // Evacuated Receiver Model
            case 2:
            {

                for (int j = 0; j < m_nRecVar; j++)
                {
                    //Check to see if the field fraction for this HCE is zero.  if so, don't bother calculating for this variation
                    if (m_HCE_FieldFrac[j] == 0.0) continue;

                    double c_htf_j, rho_htf_j;

                    m_evac_receiver->Calculate_Energy_Balance(m_T_htf_in_t_int[i], m_dot_htf_loop, T_db, T_sky,
                        weather.m_wspd, weather.m_pres * 100.0, m_q_SCA[i], j, i, false, sim_info.ms_ts.m_time / 3600.0, m_ColOptEff,
                        //outputs
                        m_q_loss[j], m_q_abs[j], m_q_1abs[j], c_htf_j, rho_htf_j, mv_HCEguessargs, m_q_reflect[j]);

                    // Check for NaN
                    if (m_q_abs[j] != m_q_abs[j])
                    {
                        return E_loop_energy_balance_exit::NaN;
                    }

                    //Keep a running sum of all of the absorbed/lost heat for each SCA in the loop
                    m_q_abs_SCAtot[i] += m_q_abs[j] * m_L_mod * m_HCE_FieldFrac[j];	//[W] Heat absorbed by HTF, weighted, for SCA
                    m_q_loss_SCAtot[i] += m_q_loss[j] * m_L_mod * m_HCE_FieldFrac[j];
                    m_q_1abs_tot[i] += m_q_1abs[j] * m_HCE_FieldFrac[j];  //losses in W/m from the absorber surface
                    m_q_reflect_tot[i] += m_q_reflect[j] * m_L_mod * m_HCE_FieldFrac[j]; //[W] Total reflective loss

                    c_htf_i += c_htf_j * m_HCE_FieldFrac[j];
                    rho_htf_i += rho_htf_j * m_HCE_FieldFrac[j];

                    //keep track of the total equivalent optical efficiency
                    EqOpteffs[i] += m_ColOptEff[i] * m_Shadowing[j] * m_dirt_env[j] * m_alpha_abs[j] * m_Tau_envelope[j] * m_HCE_FieldFrac[j];
                    m_EqOpteff += m_ColOptEff[i] * m_Shadowing[j] * m_dirt_env[j] * m_alpha_abs[j] * m_Tau_envelope[j] * (m_L_mod / m_L_tot) * m_HCE_FieldFrac[j];

                }

                q_inc_total += m_q_SCA[i] * m_L_mod;                                    // [W]
                q_abs_abs_total += m_q_SCA[i] * m_L_mod * EqOpteffs[i];                 // [W] absorbed by absorber
                q_abs_htf_total += m_q_abs_SCAtot[i];

                //Calculate the specific heat for the node
                c_htf_i *= 1000.0;	//[J/kg-K]

                //Calculate the mass of HTF associated with this node
                m_node = rho_htf_i * m_A_cs[0] * m_L_mod;


                // 7.8.2016 twn: reformulate the energy balance calculations similar to the runner/headers:
                //                    the outlet HTF temperature is equal to the bulk temperature
                // THis is from physical trough (line 1330) NOT type 262 (line 1620)
                m_T_htf_out_t_end[i] = m_q_abs_SCAtot[i] / (m_dot_htf_loop * c_htf_i) + m_T_htf_in_t_int[i] +
                    (m_T_htf_out_t_end_last[i] - m_T_htf_in_t_int[i] - m_q_abs_SCAtot[i] / (m_dot_htf_loop * c_htf_i)) *
                    exp(-m_dot_htf_loop * c_htf_i * sim_info.ms_ts.m_step / (m_node * c_htf_i + m_mc_bal_sca * m_L_mod));

                m_T_htf_out_t_int[i] = m_q_abs_SCAtot[i] / (m_dot_htf_loop * c_htf_i) + m_T_htf_in_t_int[i] +
                    ((m_node * c_htf_i + m_mc_bal_sca * m_L_mod) / (-m_dot_htf_loop * c_htf_i) *
                        (m_T_htf_out_t_end_last[i] - m_T_htf_in_t_int[i] - m_q_abs_SCAtot[i] / (m_dot_htf_loop * c_htf_i)) *
                        (exp(-m_dot_htf_loop * c_htf_i * sim_info.ms_ts.m_step / (m_node * c_htf_i + m_mc_bal_sca * m_L_mod)) - 1.0)) / sim_info.ms_ts.m_step;

                //Calculate the mass of HTF associated with this node
                m_node = rho_htf_i * m_A_cs[0] * m_L_mod;

                break;
            }

            // Polynomial Model
            case 1:
            {
                double V_wind = weather.m_wspd;
                double dt = sim_info.ms_ts.m_step;
                T_node_ave = m_T_htf_out_t_int[i];

                //iterate to improve heat loss estimate
                double errhl = 999.;
                while (std::abs(errhl) > .1) {
                    dT_loc = T_node_ave - T_db;
                    m_q_loss_SCAtot[i] = m_L_mod * CSP::poly_eval(dT_loc, &m_HL_T_coefs[0], m_HL_T_coefs.size()) * CSP::poly_eval(V_wind, &m_HL_w_coefs[0], m_HL_w_coefs.size());	//W loss
                    m_q_abs_SCAtot[i] = m_q_SCA[i] * m_L_mod * m_ColOptEff.at(i) - m_q_loss_SCAtot[i];
                    c_htf_i = m_htfProps.Cp(T_node_ave) * 1000.;	//specific heat [J/kg-K]
                    //Calculate the mass of HTF associated with this node
                    rho_htf_i = m_htfProps.dens(T_node_ave, 1.0);
                    m_node = m_rec_htf_vol / 1000. * m_A_aperture * rho_htf_i;	// [L/m2-ap]*[1 m3/1000 L]*[m2-ap]*[kg/m3] --> kg
                    // 7.8.2016 twn: reformulate the energy balance calculations similar to the runner/headers:
                    //                    the outlet HTF temperature is equal to the bulk temperature
                    // THis is from physical trough (line 1330) NOT type 262 (line 1620)

                    m_T_htf_out_t_end[i] = m_q_abs_SCAtot[i] / (m_dot_htf_loop * c_htf_i) + m_T_htf_in_t_int[i] +
                        (m_T_htf_out_t_end_last[i] - m_T_htf_in_t_int[i] - m_q_abs_SCAtot[i] / (m_dot_htf_loop * c_htf_i)) *
                        exp(-m_dot_htf_loop * c_htf_i * sim_info.ms_ts.m_step / (m_node * c_htf_i + m_mc_bal_sca * m_L_mod));

                    m_T_htf_out_t_int[i] = m_q_abs_SCAtot[i] / (m_dot_htf_loop * c_htf_i) + m_T_htf_in_t_int[i] +
                        ((m_node * c_htf_i + m_mc_bal_sca * m_L_mod) / (-m_dot_htf_loop * c_htf_i) *
                            (m_T_htf_out_t_end_last[i] - m_T_htf_in_t_int[i] - m_q_abs_SCAtot[i] / (m_dot_htf_loop * c_htf_i)) *
                            (exp(-m_dot_htf_loop * c_htf_i * sim_info.ms_ts.m_step / (m_node * c_htf_i + m_mc_bal_sca * m_L_mod)) - 1.0)) / sim_info.ms_ts.m_step;

                    errhl = T_node_ave - m_T_htf_out_t_int[i];	//iterate until the node temperature does not vary significantly

                    T_node_ave = m_T_htf_out_t_int[i];
                }

                m_q_1abs_tot[i] = m_q_loss_SCAtot[i] / m_L_mod;
                m_EqOpteff = m_eta_optical;	//Use the optical efficiency as it is for this option

                break;
            }
        }

        //Calculate the actual amount of energy absorbed by the field that doesn't go into changing the SCA's average temperature
                //Include the thermal inertia term
        if (m_q_abs_SCAtot[i] > 0.0)
        {
            double x1 = (m_node * c_htf_i + m_L_mod * m_mc_bal_sca);
            //x1 = (mass * cp of fluid in section) + (mass * cp of SCA)
            m_E_accum[i] = x1 * (m_T_htf_out_t_end[i] - m_T_htf_out_t_end_last[i]); // GOOD

            m_E_int_loop[i] = x1 * (m_T_htf_out_t_end[i] - 298.15);  //mjw 1.18.2011 energy relative to ambient

            m_E_avail[i] = max(m_q_abs_SCAtot[i] * sim_info.ms_ts.m_step - m_E_accum[i], 0.0);      //[J/s]*[hr]*[s/hr]: [J]
        }

        //Set the inlet temperature of the next SCA equal to the outlet temperature of the current SCA minus the heat losses in intermediate piping
        if (i < m_nMod - 1)
        {
            //Determine the length between SCA's to use.  if halfway down the loop, use the row distance.
            double L_int;
            if (i == m_nMod / 2 - 1) {
                L_int = 2. + m_L_crossover;
            }
            else {
                L_int = m_L_mod_spacing;
            }

            //Calculate inlet temperature of the next SCA
            m_T_htf_in_t_int[i + 1] = m_T_htf_out_t_int[i] - m_Pipe_hl_coef * m_D_abs_out[0] * CSP::pi * L_int * (m_T_htf_out_t_int[i] - T_db)
                                      / (m_dot_htf_loop * c_htf_i);

            q_dot_loss_xover[i] = m_Pipe_hl_coef * m_D_abs_out[0] * CSP::pi * L_int * (m_T_htf_out_t_int[i] - T_db);

            //Add the internal energy of the crossover piping
            //TB Seems to be += (V + m) * dTemp?
            //m_E_int_loop[i] = m_E_int_loop[i] + L_int * (pow(m_D_abs_out[0], 2) / 4. * pi + m_mc_bal_sca / c_htf_i) * (m_T_htf_out_t_int[i] - 298.150); 
            double V_xover = L_int * pow(m_D_abs_out[0], 2) / 4. * CSP::pi;
            double mc_xover_fluid =  V_xover * rho_htf_i * c_htf_i;
            double mc_xover_structure = L_int * m_mc_bal_sca;
            m_E_int_loop[i] += (mc_xover_fluid + mc_xover_structure) * (m_T_htf_out_t_int[i] - 298.150);

            E_xover[i] = 0.0;		//[MJ]
            E_xover_abs[i] = -q_dot_loss_xover[i] * sim_info.ms_ts.m_step / 1.E6;		//[MJ]
            E_xover_htf[i] = m_dot_htf_loop * c_htf_i * (m_T_htf_in_t_int[i + 1] - m_T_htf_out_t_int[i]) * sim_info.ms_ts.m_step / 1.E6;	//[MJ]
            E_xover_bal[i] = E_xover_abs[i] - E_xover_htf[i] - E_xover[i];			//[MJ]

        }

    }

    //Set the loop outlet temperature
    double T_loop_outX = m_T_htf_out_t_end[m_nMod - 1];

    // Initialize
    double q_dot_loss_HR_hot = 0.0;		//[W] 
    double E_HR_hot = 0.0;				//[MJ]
    double E_HR_hot_htf = 0.0;			//[MJ]
    double E_HR_hot_losses = 0.0;		//[MJ]
    double E_HR_hot_bal = 0.0;			//[MJ]

    //Calculation for heat losses from hot piping
    //Header
    {
        m_Header_hl_hot = 0.0;              // per piping section in one field subsection
        m_Header_hl_hot_tot = 0.0;          // total in entire field
        m_T_hdr[m_nhdrsec] = T_loop_outX;    // loop outlet temp.
        m_c_hdr_hot = m_htfProps.Cp(T_loop_outX) * 1000.;		//[kJ/kg-K]
        int D_index = 0;
        for (int i = m_nhdrsec; i < 2 * m_nhdrsec; i++)
        {
            if (i != m_nhdrsec) {
                m_T_hdr[i] = m_T_hdr[i - 1] - m_Header_hl_hot / (m_dot_header(m_m_dot_htf_tot, m_nfsec, m_nLoops, i - 1) * m_c_hdr_hot);
            }

            m_Header_hl_hot = m_L_crossover * m_D_hdr[D_index] * CSP::pi * m_Pipe_hl_coef * (T_loop_outX - T_db);
            m_Header_hl_hot_tot += m_Header_hl_hot;

            D_index++;
        }
    }
    //Runner
    {
        m_Runner_hl_hot = 0.0;              // per piping section in half the field
        m_Runner_hl_hot_tot = 0.0;          // total in entire field
        m_T_rnr[m_nrunsec] = m_T_hdr[2 * m_nhdrsec - 1] - m_Header_hl_hot / (m_dot_header(m_m_dot_htf_tot, m_nfsec, m_nLoops, 2 * m_nhdrsec - 1) * m_c_hdr_hot);
        int D_index = 0;
        for (int i = m_nrunsec; i < 2 * m_nrunsec; i++)
        {
            if (i != m_nrunsec) {
                m_T_rnr[i] = m_T_rnr[i - 1] - m_Runner_hl_hot / (m_dot_runner(m_m_dot_htf_tot, m_nfsec, i - 1) * m_c_hdr_hot);
            }
            m_Runner_hl_hot = m_L_runner[D_index] * CSP::pi * m_D_runner[D_index] * m_Pipe_hl_coef * (m_T_rnr[i] - T_db);  //Wt
            m_Runner_hl_hot_tot += m_Runner_hl_hot;
            D_index++;
        }
    }

    q_dot_loss_HR_hot = m_Header_hl_hot + m_Runner_hl_hot;	//[W]

    m_T_field_out = m_T_rnr[2 * m_nrunsec - 1] - m_Runner_hl_hot / (m_dot_runner(m_m_dot_htf_tot, m_nfsec, 2 * m_nrunsec - 1) * m_c_hdr_hot);

    // Adjust the loop outlet temperature to account for thermal losses incurred in the hot header and the runner pipe
    double T_sys_h_in = T_loop_outX - q_dot_loss_HR_hot / (m_dot_htf_loop * float(m_nLoops) * m_c_hdr_hot);	//[C]

    // MAIN End Timestep Calculations
    {
        // Calculate the hot field/system/runner/header outlet temperature at the end of the timestep
        m_T_sys_h_t_end = (m_T_sys_h_t_end_last - T_sys_h_in) * exp(-m_dot_htf_loop * float(m_nLoops) / (m_v_hot * rho_hdr_hot + m_mc_bal_hot / m_c_hdr_hot) * sim_info.ms_ts.m_step) + T_sys_h_in;

        // Calculate the hot field/system/runner/header timestep-integrated-average temperature
            // Try calculating a timestep-average Bulk Temperature (and assume it is the outlet)
        m_T_sys_h_t_int = T_sys_h_in + ((m_v_hot * rho_hdr_hot + m_mc_bal_hot / m_c_hdr_hot) / (-m_dot_htf_loop * float(m_nLoops))) *
            (m_T_sys_h_t_end_last - T_sys_h_in) * (exp(-(m_dot_htf_loop * float(m_nLoops)) / (m_v_hot * rho_hdr_hot + m_mc_bal_hot / m_c_hdr_hot) * sim_info.ms_ts.m_step) - 1.0)
            / sim_info.ms_ts.m_step;
    }

    // Calculate outputs
    {
        double E_bal_T_h_t_ave = -(m_dot_htf_loop * float(m_nLoops) * m_c_hdr_hot * (m_T_sys_h_t_int - T_sys_h_in) * sim_info.ms_ts.m_step +
            (m_v_hot * rho_hdr_hot * m_c_hdr_hot + m_mc_bal_hot) * (m_T_sys_h_t_end - m_T_sys_h_t_end_last));	//[J]

        E_HR_hot_htf = m_dot_htf_loop * float(m_nLoops) * m_c_hdr_hot * (m_T_sys_h_t_int - T_loop_outX) * sim_info.ms_ts.m_step / 1.E6;	//[MJ]

        E_HR_hot = (m_v_hot * rho_hdr_hot * m_c_hdr_hot + m_mc_bal_hot) * (m_T_sys_h_t_end - m_T_sys_h_t_end_last) * 1.E-6;		//[MJ]

        E_HR_hot_bal = -E_HR_hot_losses - E_HR_hot_htf - E_HR_hot;		//[MJ]

        // Calculate sub-timestep reporting energy (rate) balance metrics
            // Loop metrics
        m_q_dot_sca_loss_summed_subts = 0.0;	//[MWt]
        m_q_dot_sca_abs_summed_subts = 0.0;		//[MWt]
        m_q_dot_sca_refl_summed_subts = 0.0;    //[MWt]
        m_q_dot_xover_loss_summed_subts = 0.0;	//[MWt]
        m_E_dot_sca_summed_subts = 0.0;			//[MWt]
        m_E_dot_xover_summed_subts = 0.0;		//[MWt]

        for (int i = 0; i < m_nMod; i++)
        {
            if (i < m_nMod - 1)
            {
                m_q_dot_xover_loss_summed_subts += q_dot_loss_xover[i];		//[W] -> convert to MWt and multiply by nLoops below
                m_E_dot_xover_summed_subts += E_xover[i];					//[MJ] -> convert to MWt and multiply by nLoops below
            }
            m_q_dot_sca_loss_summed_subts += m_q_loss_SCAtot[i];			//[W] -> convert to MWT and multiply by nLoops below
            m_q_dot_sca_abs_summed_subts += m_q_abs_SCAtot[i];				//[W] -> convert to MWT and multiply by nLoops below
            m_q_dot_sca_refl_summed_subts += m_q_reflect_tot[i];            //[W] -> convert to MWT and multiply by nLoops below
            m_E_dot_sca_summed_subts += E_sca[i];							//[MJ] -> convert to MWt and multiply by nLoops below
        }
        m_q_dot_xover_loss_summed_subts *= 1.E-6 * m_nLoops;				//[MWt] 
        m_E_dot_xover_summed_subts *= (m_nLoops / sim_info.ms_ts.m_step);	//[MWt]
        m_q_dot_sca_loss_summed_subts *= 1.E-6 * m_nLoops;					//[MWt]
        m_q_dot_sca_abs_summed_subts *= 1.E-6 * m_nLoops;					//[MWt]
        m_q_dot_sca_refl_summed_subts *= 1.E-6 * m_nLoops;                  //[MWt]
        m_E_dot_sca_summed_subts *= (m_nLoops / sim_info.ms_ts.m_step);		//[MWt]

        // Header-runner metrics
        m_q_dot_HR_cold_loss_subts = q_dot_loss_HR_cold * 1.E-6;			//[MWt]
        m_q_dot_HR_hot_loss_subts = q_dot_loss_HR_hot * 1.E-6;			    //[MWt]
        m_E_dot_HR_cold_subts = E_HR_cold / sim_info.ms_ts.m_step;		    //[MWt]
        m_E_dot_HR_hot_subts = E_HR_hot / sim_info.ms_ts.m_step;		    //[MWt]

        // HTF out of system
        m_c_htf_ave_ts_ave_temp = m_htfProps.Cp_ave(T_htf_cold_in, m_T_sys_h_t_int) * 1000.0;	//[J/kg-K]
        m_q_dot_htf_to_sink_subts = m_m_dot_htf_tot * m_c_htf_ave_ts_ave_temp * (m_T_sys_h_t_int - T_htf_cold_in) * 1.E-6;

        double Q_dot_balance_subts = m_q_dot_sca_abs_summed_subts - m_q_dot_xover_loss_summed_subts -
            m_q_dot_HR_cold_loss_subts - m_q_dot_HR_hot_loss_subts -
            m_E_dot_sca_summed_subts - m_E_dot_xover_summed_subts -
            m_E_dot_HR_cold_subts - m_E_dot_HR_hot_subts - m_q_dot_htf_to_sink_subts;	//[MWt]

        // Calculate total field energy balance:
        double Q_abs_scas_summed = 0.0;		                                //[MJ]
        double Q_loss_xover = 0.0;			                                //[MJ]
        double E_scas_summed = 0.0;			                                //[MJ]
        double E_xovers_summed = 0.0;		                                //[MJ]

        double E_scas_htf_summed = 0.0;		                                //[MJ]
        double E_xovers_htf_summed = 0.0;	                                //[MJ]

        for (int i = 0; i < m_nMod; i++)
        {
            if (i < m_nMod - 1)
            {
                Q_loss_xover += q_dot_loss_xover[i];		//[W] -> convert to MJ and multiply nLoops below
                E_xovers_summed += E_xover[i];				//[MJ] -> multiply nLoops below
                E_xovers_htf_summed += E_xover_htf[i];		//[MJ] -> multiply by nLoops below
            }
            Q_abs_scas_summed += m_q_abs_SCAtot[i];		                    //[W] -> convert to MJ and multiply nLoops below
            E_scas_summed += E_sca[i];					                    //[MJ] -> multiply nLoops below
            E_scas_htf_summed += E_sca_htf[i];			                    //[MJ] -> multiply by nLoops below
        }
        Q_loss_xover *= sim_info.ms_ts.m_step * 1.E-6 * m_nLoops;		    //[MJ] = [W*s*MW/W*#loops]
        Q_abs_scas_summed *= sim_info.ms_ts.m_step * 1.E-6 * m_nLoops;	    //[MJ] = [W*s*MW/W*#loops]
        E_xovers_summed *= m_nLoops;			                            //[MJ] multiply nLoops below
        E_scas_summed *= m_nLoops;				                            //[MJ] multiply nLoops below

        E_scas_htf_summed *= m_nLoops;			                            //[MJ] 
        E_xovers_htf_summed *= m_nLoops;		                            //[MJ]


        double Q_htf = m_m_dot_htf_tot * m_c_htf_ave_ts_ave_temp * (m_T_sys_h_t_int - T_htf_cold_in) * sim_info.ms_ts.m_step * 1.E-6;		//[MJ]
        double E_htf_bal = E_HR_cold_htf + E_scas_htf_summed + E_xovers_htf_summed + E_HR_hot_htf - Q_htf;		//[MJ]

        double Q_loss_HR_cold = q_dot_loss_HR_cold * sim_info.ms_ts.m_step * 1.E-6;		                        //[MJ]
        double Q_loss_HR_hot = q_dot_loss_HR_hot * sim_info.ms_ts.m_step * 1.E-6;		                        //[MJ]

        m_Q_field_losses_total_subts = Q_loss_xover + Q_loss_HR_cold + Q_loss_HR_hot - Q_abs_scas_summed;		//[MJ]
    }


    return E_loop_energy_balance_exit::SOLVED;
}

/**************************************************************************************************
    ---------------------------------------------------------------------------------
    --Inputs
       * nhsec - [-] number of header sections
       * nfsec - [-] number of field section
       * nrunsec- [-] number of unique runner diameter sections
       * rho   - [kg/m3] Fluid density
       * V_max - [m/s] Maximum fluid velocity at design
       * V_min - [m/s] Minimum fluid velocity at design
       * m_dot - [kg/s] Mass flow rate at design
    --Outputs
       * D_hdr - [m] An ARRAY containing the header diameter for each loop section
       * D_runner - [m] An ARRAY containing the diameter of the runner pipe sections
       * summary - Address of string variable on which summary contents will be written.
    ---------------------------------------------------------------------------------			*/
void C_csp_fresnel_collector_receiver::header_design(int nhsec, int nfsec, int nrunsec, double rho, double V_max, double V_min, double m_dot,
    vector<double>& D_hdr, vector<double>& D_runner, std::string* summary = NULL) {

    //resize the header matrices if they are incorrect
        //real(8),intent(out):: D_hdr(nhsec), D_runner(nrunsec)
    if ((int)D_hdr.size() != nhsec) D_hdr.resize(nhsec);
    if ((int)D_runner.size() != nrunsec) D_runner.resize(nrunsec);

    //----
    int nst, nend, nd;
    double m_dot_max, m_dot_min, m_dot_ts, m_dot_hdr, m_dot_2loops, m_dot_temp;

    for (int i = 0; i < nhsec; i++) { D_hdr[i] = 0.; }

    //mass flow to section is always half of total
    m_dot_ts = m_dot / 2.;
    //Mass flow into 1 header
    m_dot_hdr = 2. * m_dot_ts / (float(nfsec));
    //Mass flow into the 2 loops attached to a single header section
    m_dot_2loops = m_dot_hdr / float(nhsec);

    //Runner diameters
    //runner pipe needs some length to go from the power block to the headers
    D_runner.at(0) = CSP::pipe_sched(sqrt(4. * m_dot_ts / (rho * V_max * CSP::pi)));
    //other runner diameters
    m_dot_temp = m_dot_ts * (1. - float(nfsec % 4) / float(nfsec));  //mjw 5.4.11 Fix mass flow rate for nfsec/2==odd 
    if (nrunsec > 1) {
        for (int i = 1; i < nrunsec; i++) {
            D_runner[i] = CSP::pipe_sched(sqrt(4. * m_dot_temp / (rho * V_max * CSP::pi)));
            m_dot_temp = max(m_dot_temp - m_dot_hdr * 2, 0.0);
        }
    }

    //Calculate each section in the header
    nst = 0; nend = 0; nd = 0;
    m_dot_max = m_dot_hdr;

    for (int i = 0; i < nhsec; i++) {
        if ((i == nst) && (nd <= 10)) {
            //If we've reached the point where a diameter adjustment must be made...
            //Also, limit the number of diameter reductions to 10

            nd++; //keep track of the total number of diameter sections
            //Calculate header diameter based on max velocity
            D_hdr[i] = CSP::pipe_sched(sqrt(4. * m_dot_max / (rho * V_max * CSP::pi)));
            //Determine the mass flow corresponding to the minimum velocity at design
            m_dot_min = rho * V_min * CSP::pi * D_hdr[i] * D_hdr[i] / 4.;
            //Determine the loop after which the current diameter calculation will no longer apply
            nend = (int)floor((m_dot_hdr - m_dot_min) / (m_dot_2loops));  //tn 4.12.11 ceiling->floor
            //The starting loop for the next diameter section starts after the calculated ending loop
            nst = nend;
            //Adjust the maximum required flow rate for the next diameter section based on the previous 
            //section's outlet conditions
            m_dot_max = max(m_dot_hdr - m_dot_2loops * float(nend), 0.0);
        }
        else {
            //If we haven't yet reached the point where the minimum flow condition is acheived, just
            //set the header diameter for this loop to be equal to the last diameter
            D_hdr[i] = D_hdr.at(i - 1);
        }
    }

    //Print the results to a string
    if (summary != NULL) {
        summary->clear();
        char tstr[200];
        //Write runner diam
        sprintf(tstr, "Piping geometry file\n\nMaximum fluid velocity: %.2f\nMinimum fluid velocity: %.2f\n\n", V_max, V_min);
        summary->append(tstr);

        for (int i = 0; i < nrunsec; i++) {
            sprintf(tstr, "To section %d header pipe diameter: %.4f m (%.2f in)\n", i + 1, D_runner[i], D_runner[i] * m_mtoinch);
            summary->append(tstr);
        }
        //Write header diams
        sprintf(tstr, "Loop No. | Diameter [m] | Diameter [in] | Diam. ID\n--------------------------------------------------\n");
        summary->append(tstr);

        nd = 1;
        for (int i = 0; i < nhsec; i++) {
            if (i > 1) {
                if (D_hdr[i] != D_hdr.at(i - 1)) nd = nd + 1;
            }
            sprintf(tstr, "  %4d   |    %6.4f    |    %6.4f     | %3d\n", i + 1, D_hdr[i], D_hdr[i] * m_mtoinch, nd);
            summary->append(tstr);
        }
        //110 format(2X,I4,3X,"|",4X,F6.4,4X,"|",4X,F6.3,5X,"|",1X,I3)
    }

}

void C_csp_fresnel_collector_receiver::loop_optical_eta(const C_csp_weatherreader::S_outputs& weather,
    const C_csp_solver_sim_info& sim_info)
{
    //if (weather.m_wspd >= m_wind_stow_speed)    // no wind stow speed user input
    if (false)
    {
        loop_optical_wind_stow();
    }
    else
    {
        // First, clear all the values calculated below
        loop_optical_eta_off();

        //calculate the m_hour of the day
        double time_hr = sim_info.ms_ts.m_time / 3600.;		//[hr]
        double dt_hr = sim_info.ms_ts.m_step / 3600.;			//[hr]
        double hour = fmod(time_hr, 24.);				//[hr]

        //Time calculations
        int day_of_year = (int)ceil(time_hr / 24.);  //Day of the year
        // Duffie & Beckman 1.5.3b
        double B = (day_of_year - 1) * 360.0 / 365.0 * CSP::pi / 180.0;
        // Eqn of time in minutes
        double EOT = 229.2 * (0.000075 + 0.001868 * cos(B) - 0.032077 * sin(B) - 0.014615 * cos(B * 2.0) - 0.04089 * sin(B * 2.0));
        // Declination in radians (Duffie & Beckman 1.6.1)
        double Dec = 23.45 * sin(360.0 * (284.0 + day_of_year) / 365.0 * CSP::pi / 180.0) * CSP::pi / 180.0;
        // Solar Noon and time in hours
        double SolarNoon = 12. - ((m_shift) * 180.0 / CSP::pi) / 15.0 - EOT / 60.0;

        // Deploy & stow times in hours
        // Calculations modified by MJW 11/13/2009 to correct bug
        m_theta_dep = max(m_theta_dep, 1.e-6);
        double DepHr1 = cos(m_latitude) / tan(m_theta_dep);
        double DepHr2 = -tan(Dec) * sin(m_latitude) / tan(m_theta_dep);
        double DepHr3 = CSP::sign(tan(CSP::pi - m_theta_dep)) * acos((DepHr1 * DepHr2 + sqrt(DepHr1 * DepHr1 - DepHr2 * DepHr2 + 1.0)) / (DepHr1 * DepHr1 + 1.0)) * 180.0 / CSP::pi / 15.0;
        double DepTime = SolarNoon + DepHr3;

        m_theta_stow = max(m_theta_stow, 1.e-6);
        double StwHr1 = cos(m_latitude) / tan(m_theta_stow);
        double StwHr2 = -tan(Dec) * sin(m_latitude) / tan(m_theta_stow);
        double StwHr3 = CSP::sign(tan(CSP::pi - m_theta_stow)) * acos((StwHr1 * StwHr2 + sqrt(StwHr1 * StwHr1 - StwHr2 * StwHr2 + 1.0)) / (StwHr1 * StwHr1 + 1.0)) * 180.0 / CSP::pi / 15.0;
        double StwTime = SolarNoon + StwHr3;

        // m_ftrack is the fraction of the time period that the field is tracking. MidTrack is time at midpoint of operation
        double HrA = hour - dt_hr;
        double HrB = hour;

        double  MidTrack;
        m_ftrack = std::numeric_limits<double>::quiet_NaN();
        // Solar field operates
        if ((HrB > DepTime) && (HrA < StwTime))
        {
            // solar field deploys during time period
            if (HrA < DepTime)
            {
                m_ftrack = (HrB - DepTime) / dt_hr;
                MidTrack = HrB - m_ftrack * 0.5 * dt_hr;

                // Solar field stows during time period
            }
            else if (HrB > StwTime)
            {
                m_ftrack = (StwTime - HrA) / dt_hr;
                MidTrack = HrA + m_ftrack * 0.5 * dt_hr;
                // solar field operates during entire period
            }
            else
            {
                m_ftrack = 1.0;
                MidTrack = HrA + 0.5 * dt_hr;
            }
            // solar field doesn't operate
        }
        else
        {
            m_ftrack = 0.0;
            MidTrack = HrA + 0.5 * dt_hr;
        }

        //// Maximum wind speed value   NO max wind speed
        //if (V_wind >= m_V_wind_max)
        //    m_ftrack = 0.0;

        double StdTime = MidTrack;
        double SolarTime = StdTime + ((m_shift) * 180.0 / CSP::pi) / 15.0 + EOT / 60.0;
        // m_hour angle (arc of sun) in radians
        double omega = (SolarTime - 12.0) * 15.0 * CSP::pi / 180.0;

        // Convert other input data as necessary
        double SolarAz = weather.m_solazi;		//[deg] Solar azimuth angle
        SolarAz = (SolarAz - 180.0) * m_d2r;	//[rad] convert from [deg]
        double SolarAlt;
        // B. Stine equation for Solar Altitude angle in radians
        SolarAlt = asin(sin(Dec) * sin(m_latitude) + cos(m_latitude) * cos(Dec) * cos(omega));

        double SolarZenRad = weather.m_solzen * m_d2r; // Convert from degree to radian

        if (SolarZenRad < CSP::pi / 2.) {
            //Convert the solar angles to collector incidence angles
            CSP::theta_trans(SolarAz, SolarZenRad, m_ColAz, m_phi_t, m_theta_L);

            switch (m_opt_model)
            {
                case 1:		//sun position
                    //user provides an optical table as a function of solar position
                    m_eta_optical = eta_opt_fixed * max(optical_table.interpolate(SolarAz, min(SolarZenRad, CSP::pi / 2.)), 0.0);
                    break;
                case 2:		//incidence angle table
                    //user provides an optical table as a function of collector incidence angles
                    m_eta_optical = eta_opt_fixed * max(optical_table.interpolate(m_phi_t, max(m_theta_L, 0.0)), 0.0);
                    break;
                case 3:		//incidence angle modifier polys
                    //Otherwise, calculate the collector incidence angles for the IAM equations
                    m_eta_optical = eta_opt_fixed *
                        CSP::poly_eval(m_phi_t, &m_IAM_T_coefs[0], m_IAM_T_coefs.size()) *
                        CSP::poly_eval(m_theta_L, &m_IAM_L_coefs[0], m_IAM_L_coefs.size());
                    break;
                default:
                    //error
                    //message(TCS_ERROR, "No corresponding optical model. Error in solar angle calculation.");
                    string msg = "No corresponding optical model. Error in solar angle calculation.";
                    m_error_msg = util::format(msg.c_str());
                    mc_csp_messages.add_message(C_csp_messages::NOTICE, m_error_msg);
                    return;
            }

            m_eta_optical *= m_ftrack;
        }
        else {
            m_eta_optical = 0.0;
            m_phi_t = CSP::pi / 2.;
            m_theta_L = 0.0;
        }

        double I_b = weather.m_beam;
        m_q_i = I_b * m_A_aperture / m_L_mod; //[W/m] The incoming solar irradiation per aperture length

        //Optical efficiency and incident power values for each SCA
        for (int j = 0; j < m_nMod; j++) {
            m_ColOptEff.at(j) = m_eta_optical;
            m_q_SCA[j] = m_q_i;        //[W/m] The flux on the collector
        }

        // Assume that whenever fresnel is in STARTUP OR ON, we're using the nominal tracking load
        // This is because it takes power to move into and out of defocus, and we'd probably
        //    just add complexity without any accuracy by trying to capture that
        m_W_dot_sca_tracking = m_W_dot_sca_tracking_nom * m_ftrack;	//[MWe]

        m_control_defocus = m_component_defocus = 1.0;	//[-]

        m_q_dot_inc_sf_tot = m_Ap_tot * weather.m_beam / 1.E6;	//[MWt]

    }
}

void C_csp_fresnel_collector_receiver::loop_optical_eta_off()
{
    // If fresnel is not absorbing any sunlight (night or 100% defocus), then set member data as necessary

    m_q_i = 0;		                        //[W/m] DNI * A_aper / L_sca
    m_ColOptEff.fill(0.0);				    //[-] tracking * geom * rho * dirt * error * IAM * row shadow * end loss * ftrack
    m_EqOpteff = 0.;
    m_q_SCA.assign(m_q_SCA.size(), 0.0);	//[W/m] Total incident irradiation on the receiver (q"*A_aper/L_sca*cos(theta))

    m_W_dot_sca_tracking = 0.0;		        //[MWe]

    m_control_defocus = 1.0;
    m_component_defocus = 1.0;

    m_q_dot_inc_sf_tot = 0.0;		        //[MWt]

    m_eta_optical = 0;

    return;
}

void C_csp_fresnel_collector_receiver::loop_optical_wind_stow()
{
    // Want to completely defocus fresnel because wind speed is faster than stow speed
    // Can use 'loop_optical_eta_off' but then need to reset:
    //	* tracking power
    //  * defocus values

    loop_optical_eta_off();

    m_W_dot_sca_tracking = m_W_dot_sca_tracking_nom;		//[MWe]

    m_component_defocus = 0.0;
}

void C_csp_fresnel_collector_receiver::update_last_temps()
{
    // Update "_last" temperatures
    m_T_sys_c_t_end_last = m_T_sys_c_t_end;		//[K]
    m_T_sys_h_t_end_last = m_T_sys_h_t_end;		//[K]
    for (int i = 0; i < m_nMod; i++)
    {
        m_T_htf_out_t_end_last[i] = m_T_htf_out_t_end[i];	//[K]
    }

    return;
}

void C_csp_fresnel_collector_receiver::reset_last_temps()
{
    // Update "_last" temperatures
    m_T_sys_c_t_end_last = m_T_sys_c_t_end_converged;		//[K]
    m_T_sys_h_t_end_last = m_T_sys_h_t_end_converged;		//[K]
    for (int i = 0; i < m_nMod; i++)
    {
        m_T_htf_out_t_end_last[i] = m_T_htf_out_t_end_converged[i];	//[K]
    }
}

void C_csp_fresnel_collector_receiver::apply_control_defocus(double defocus)
{
    // Uses m_q_i, and input defocus to calculate m_q_SCA_control_df

    // Store control defocus
    m_control_defocus = defocus;

    for (int i = 0; i < m_nMod; i++)
    {
        m_q_SCA_control_df[i] = defocus * m_q_i;
    }

}

void C_csp_fresnel_collector_receiver::apply_component_defocus(double defocus /*-*/)
{
    // Uses m_q_SCA_control_df and input defocus to calculate m_q_SCA

    // Store component defocus
    m_component_defocus = defocus;

    for (int i = 0; i < m_nMod; i++)
    {
        m_q_SCA[i] = defocus * m_q_SCA_control_df[i];
    }
}

double C_csp_fresnel_collector_receiver::Pump_SGS(double rho, double m_dotsf, double sm) {

    int nl = 8;
    double v_dotpb, v_dotsf, m_dotpb, vel_max;
    double
        * V_dot = new double[nl],
        * D = new double[nl],
        * V = new double[nl];

    //Line no.	
    //1	Expansion vessel or thermal storage tank to pump suction header
    //2	Individual pump suction line, from suction header to pump inlet
    //3	Individual pump discharge line, from pump discharge to discharge header
    //4	Pump discharge header
    //5	Collector field outlet header to expansion vessel or thermal storage tank
    //6	Steam generator supply header
    //7	Inter steam generator piping
    //8	Steam generator exit header to expansion vessel or thermal storage
    //Assume standard lengths for each line [m] (Kelly & Kearney)
    //Assume 3 pumps at 50% each. #3) 3*30. 
    double L_line[] = { 0.0, 0.0, 90.0, 100.0, 120.0, 80.0, 120.0, 80.0 };

    //Assume a maximum HTF velocity of 1.85 m/s (based on average from Kelly & Kearney model
    vel_max = 1.85;

    //design-point vol. flow rate m3/s
    m_dotpb = m_dotsf / sm;
    v_dotpb = m_dotpb / rho;
    v_dotsf = m_dotsf / rho;

    //Set the volumetric flow rate for each line.
    V_dot[0] = v_dotsf;
    V_dot[1] = v_dotsf / 2.0;
    V_dot[2] = V_dot[1];
    V_dot[3] = v_dotsf;
    V_dot[4] = V_dot[3];
    V_dot[5] = v_dotpb;
    V_dot[6] = V_dot[5];
    V_dot[7] = V_dot[5];

    //for each line..
    double psum = 0.;
    for (int i = 0; i < nl; i++) {
        //Calculate the pipe diameter
        D[i] = CSP::pipe_sched(sqrt(4.0 * V_dot[i] / (vel_max * CSP::pi)));
        //Calculate the total volume
        V[i] = pow(D[i], 2) / 4. * CSP::pi * L_line[i];
        psum += V[i];
    }

    delete[] V_dot;
    delete[] D;
    delete[] V;

    return psum;

}

/*
   ***************************************************************************************************
    Trough system piping loss model
   ***************************************************************************************************

    This piping loss model is derived from the pressure drop calculations presented in the
    following document:

      Parabolic Trough Solar System Piping Model

      B. Kelly
      Nexant, Inc. San Francisco, California

      D. Kearney
      Kearney & Associates
      Vashon, Washington

      Subcontract Report
      NREL/SR-550-40165
      July 2006

    ----------------------------
    Note on use of this function
    ----------------------------
    The function returns the pressure drop across a given length of pipe, and also accounts for
    a variety of possible pressure-loss components. This function should be called multiple times -
    once for each section under consideration.  For example, separate calls should be made for the
    HCE pressure drop, the pressure drop in each section of the header in which flow/geometrical
    conditions vary, the section of pipe leading to the header, and so on.

    ----------------------------
    Inputs
    ----------------------------
    No | Name         | Description                           | Units     |  Type
   ===================================================================================
     1 | Fluid        | Number associated with fluid type     | none      | float
     2 | m_dot        | Mass flow rate of the fluid           | kg/s      | float
     3 | T            | Fluid temperature                     | K         | float
     4 | P            | Fluid pressure                        | Pa        | float
     5 | D            | Diameter of the contact surface       | m         | float
     6 | Rough        | Pipe roughness                        | m         | float
     7 | L_pipe       | Length of pipe for pressure drop      | m         | float
     8 | Nexp         | Number of expansions                  | none      | float
     9 | Ncon         | Number of contractions                | none      | float
    10 | Nels         | Number of standard elbows             | none      | float
    11 | Nelm         | Number of medium elbows               | none      | float
    12 | Nell         | Number of long elbows                 | none      | float
    13 | Ngav         | Number of gate valves                 | none      | float
    14 | Nglv         | Number of globe valves                | none      | float
    15 | Nchv         | Number of check valves                | none      | float
    16 | Nlw          | Number of loop weldolets              | none      | float
    17 | Nlcv         | Number of loop control valves         | none      | float
    18 | Nbja         | Number of ball joint assemblies       | none      | float
   ===================================================================================
    ----------------------------
    Outputs
    ----------------------------
    1. PressureDrop  (Pa)
   */
double C_csp_fresnel_collector_receiver::PressureDrop(double m_dot, double T, double P, double D, double Rough, double L_pipe,
    double Nexp, double Ncon, double Nels, double Nelm, double Nell, double Ngav, double Nglv,
    double Nchv, double Nlw, double Nlcv, double Nbja) {

    double rho, v_dot, mu, nu, u_fluid, Re, f, DP_pipe, DP_exp, DP_con, DP_els, DP_elm, DP_ell, DP_gav,
        DP_glv, DP_chv, DP_lw, DP_lcv, DP_bja, HL_pm;

    //Calculate fluid properties and characteristics
    rho = m_htfProps.dens(T, P);
    mu = m_htfProps.visc(T);
    nu = mu / rho;
    v_dot = m_dot / rho;   //fluid volumetric flow rate
    u_fluid = v_dot / (CSP::pi * (D / 2.) * (D / 2.));  //Fluid mean velocity

    //Dimensionless numbers
    Re = u_fluid * D / nu;
    //if(Re<2300.) then
    //    f = 64./max(Re,1.0)
    //else
    f = FricFactor(Rough / D, Re);
    if (f == 0) return std::numeric_limits<double>::quiet_NaN();
    //}

    //Calculation of pressure loss from pipe length
    HL_pm = f * u_fluid * u_fluid / (2. * D * CSP::grav);
    DP_pipe = HL_pm * rho * CSP::grav * L_pipe;

    //Calculation of pressure loss from Fittings
    DP_exp = 0.25 * rho * u_fluid * u_fluid * Nexp;
    DP_con = 0.25 * rho * u_fluid * u_fluid * Ncon;
    DP_els = 0.9 * D / f * HL_pm * rho * CSP::grav * Nels;
    DP_elm = 0.75 * D / f * HL_pm * rho * CSP::grav * Nelm;
    DP_ell = 0.6 * D / f * HL_pm * rho * CSP::grav * Nell;
    DP_gav = 0.19 * D / f * HL_pm * rho * CSP::grav * Ngav;
    DP_glv = 10.0 * D / f * HL_pm * rho * CSP::grav * Nglv;
    DP_chv = 2.5 * D / f * HL_pm * rho * CSP::grav * Nchv;
    DP_lw = 1.8 * D / f * HL_pm * rho * CSP::grav * Nlw;
    DP_lcv = 10.0 * D / f * HL_pm * rho * CSP::grav * Nlcv;
    DP_bja = 8.69 * D / f * HL_pm * rho * CSP::grav * Nbja;

    return DP_pipe + DP_exp + DP_con + DP_els + DP_elm + DP_ell + DP_gav + DP_glv + DP_chv + DP_lw + DP_lcv + DP_bja;

}

/**************************************************************************************************
 Friction factor (taken from Piping loss model)
***************************************************************************************************
 Uses an iterative method to solve the implicit friction factor function.
 For more on this method, refer to Fox, et al., 2006 Introduction to Fluid Mechanics.			 */
double C_csp_fresnel_collector_receiver::FricFactor(double Rough, double Reynold) {

    double Test, TestOld, X, Xold, Slope;
    double Acc = .01; //0.0001
    int NumTries;

    if (Reynold < 2750.) {
        return 64. / max(Reynold, 1.0);
    }

    X = 33.33333;  //1. / 0.03
    TestOld = X + 2. * log10(Rough / 3.7 + 2.51 * X / Reynold);
    Xold = X;
    X = 28.5714;  //1. / (0.03 + 0.005)
    NumTries = 0;

    while (NumTries < 21) {
        NumTries++;
        Test = X + 2 * log10(Rough / 3.7 + 2.51 * X / Reynold);
        if (std::abs(Test - TestOld) <= Acc) {
            return 1. / (X * X);
        }

        Slope = (Test - TestOld) / (X - Xold);
        Xold = X;
        TestOld = Test;
        X = max((Slope * X - Test) / Slope, 1.e-5);
    }

    //call Messages(-1," Could not find friction factor solution",'Warning',0,250) 
    return 0;
}

// ******************************************************************** Private STATIC Methods

// Returns runner mass flow for a given runner index
double C_csp_fresnel_collector_receiver::m_dot_runner(double m_dot_field, int nfieldsec, int irnr)
{
    int nrnrsec = (int)floor(float(nfieldsec) / 4.0) + 1;

    if (irnr < 0 || irnr > 2 * nrnrsec - 1) { throw std::invalid_argument("Invalid runner index"); }

    int irnr_onedir;
    double m_dot_rnr;
    double m_dot_rnr_0;
    double m_dot_rnr_1;

    // convert index to a mass flow equivalent cold runner index
    if (irnr > nrnrsec - 1) {
        irnr_onedir = 2 * nrnrsec - irnr - 1;
    }
    else {
        irnr_onedir = irnr;
    }

    m_dot_rnr_0 = m_dot_field / 2.;
    m_dot_rnr_1 = m_dot_rnr_0 * (1. - float(nfieldsec % 4) / float(nfieldsec));

    switch (irnr_onedir) {
        case 0:
            m_dot_rnr = m_dot_rnr_0;
        case 1:
            m_dot_rnr = m_dot_rnr_1;
        default:
            m_dot_rnr = m_dot_rnr_1 - (irnr_onedir - 1) * m_dot_field / float(nfieldsec) * 2;
    }

    return max(m_dot_rnr, 0.0);
}

// Returns header mass flow for a given header index
double C_csp_fresnel_collector_receiver::m_dot_header(double m_dot_field, int nfieldsec, int nLoopsField, int ihdr)
{
    int nhdrsec = (int)ceil(float(nLoopsField) / float(nfieldsec * 2));  // in the cold or hot headers

    if (ihdr < 0 || ihdr > 2 * nhdrsec - 1) { throw std::invalid_argument("Invalid header index"); }

    int ihdr_onedir;

    // convert index to a mass flow equivalent cold header index
    if (ihdr > nhdrsec - 1) {
        ihdr_onedir = 2 * nhdrsec - ihdr - 1;
    }
    else {
        ihdr_onedir = ihdr;
    }

    double m_dot_oneloop = m_dot_field / float(nLoopsField);
    return m_dot_field / float(nfieldsec) - ihdr_onedir * 2 * m_dot_oneloop;
}

// ------------------------------------------------------------------------------ PUBLIC

C_csp_fresnel_collector_receiver::C_csp_fresnel_collector_receiver()
{
    mc_reported_outputs.construct(S_output_info);

    // Set maximum timestep from parent class member data
    m_max_step = 60.0 * 60.0;			//[s]: [m] * [s/m]
    m_step_recirc = 10.0 * 60.0;		//[s]

    m_W_dot_sca_tracking_nom = std::numeric_limits<double>::quiet_NaN();

    // set initial values for all parameters to prevent possible misuse
    m_nMod = -1;
    m_nRecVar = -1;
    m_nLoops = -1;
    m_FieldConfig = -1;
    m_eta_pump = std::numeric_limits<double>::quiet_NaN();
    m_HDR_rough = std::numeric_limits<double>::quiet_NaN();
    m_theta_stow = std::numeric_limits<double>::quiet_NaN();
    m_theta_dep = std::numeric_limits<double>::quiet_NaN();
    m_T_startup = std::numeric_limits<double>::quiet_NaN();
    m_m_dot_htfmin = std::numeric_limits<double>::quiet_NaN();
    m_m_dot_htfmax = std::numeric_limits<double>::quiet_NaN();
    m_T_loop_in_des = std::numeric_limits<double>::quiet_NaN();
    m_T_loop_out_des = std::numeric_limits<double>::quiet_NaN();
    m_Fluid = -1;

    m_m_dot_design = std::numeric_limits<double>::quiet_NaN();
    m_m_dot_loop_des = std::numeric_limits<double>::quiet_NaN();

    m_T_fp = std::numeric_limits<double>::quiet_NaN();
    m_I_bn_des = std::numeric_limits<double>::quiet_NaN();
    m_V_hdr_max = std::numeric_limits<double>::quiet_NaN();
    m_V_hdr_min = std::numeric_limits<double>::quiet_NaN();
    m_Pipe_hl_coef = std::numeric_limits<double>::quiet_NaN();
    m_SCA_drives_elec = std::numeric_limits<double>::quiet_NaN();
    m_ColAz = std::numeric_limits<double>::quiet_NaN();

    m_solar_mult = std::numeric_limits<double>::quiet_NaN();
    m_mc_bal_hot = std::numeric_limits<double>::quiet_NaN();
    m_mc_bal_cold = std::numeric_limits<double>::quiet_NaN();
    m_mc_bal_sca = std::numeric_limits<double>::quiet_NaN();

    m_latitude = std::numeric_limits<double>::quiet_NaN();
    m_longitude = std::numeric_limits<double>::quiet_NaN();

    // ************************************************************************
    // CSP Solver Temperature Tracking
        // Temperatures from the most recent converged() operation
    m_T_sys_c_t_end_converged = std::numeric_limits<double>::quiet_NaN();	//[K]
    m_T_sys_h_t_end_converged = std::numeric_limits<double>::quiet_NaN();	//[K]

    // Temperatures from the most recent timstep (in the event that a method solves multiple, shorter timesteps
    m_T_sys_c_t_end_last = std::numeric_limits<double>::quiet_NaN();	//[K] Temperature (bulk) of cold runners & headers at end of previous timestep
    m_T_sys_h_t_end_last = std::numeric_limits<double>::quiet_NaN();	//[K]

    // Latest temperature solved during present call to this class
    m_T_sys_c_t_end = std::numeric_limits<double>::quiet_NaN();			//[K]
    m_T_sys_c_t_int = std::numeric_limits<double>::quiet_NaN();			//[K]
    m_T_sys_h_t_end = std::numeric_limits<double>::quiet_NaN();			//[K]
    m_T_sys_h_t_int = std::numeric_limits<double>::quiet_NaN();			//[K]

    m_Q_field_losses_total_subts = std::numeric_limits<double>::quiet_NaN();	//[MJ]
    m_c_htf_ave_ts_ave_temp = std::numeric_limits<double>::quiet_NaN();	//[J/kg-K]

    m_q_dot_sca_loss_summed_subts = std::numeric_limits<double>::quiet_NaN();	//[MWt]
    m_q_dot_sca_abs_summed_subts = std::numeric_limits<double>::quiet_NaN();	//[MWt]
    m_q_dot_sca_refl_summed_subts = std::numeric_limits<double>::quiet_NaN();   //[MWt]
    m_q_dot_xover_loss_summed_subts = std::numeric_limits<double>::quiet_NaN();	//[MWt]
    m_q_dot_HR_cold_loss_subts = std::numeric_limits<double>::quiet_NaN();		//[MWt]
    m_q_dot_HR_hot_loss_subts = std::numeric_limits<double>::quiet_NaN();		//[MWt]
    m_E_dot_sca_summed_subts = std::numeric_limits<double>::quiet_NaN();	//[MWt]
    m_E_dot_xover_summed_subts = std::numeric_limits<double>::quiet_NaN();	//[MWt]
    m_E_dot_HR_cold_subts = std::numeric_limits<double>::quiet_NaN();		//[MWt]
    m_E_dot_HR_hot_subts = std::numeric_limits<double>::quiet_NaN();		//[MWt]
    m_q_dot_htf_to_sink_subts = std::numeric_limits<double>::quiet_NaN();	//[MWt]

    // ************************************************************************
    // ************************************************************************
        // Full Timestep Outputs
    m_T_sys_c_t_int_fullts = std::numeric_limits<double>::quiet_NaN();			//[K]
    m_T_htf_c_rec_in_t_int_fullts = std::numeric_limits<double>::quiet_NaN();	//[K]
    m_T_htf_h_rec_out_t_int_fullts = std::numeric_limits<double>::quiet_NaN();	//[K]
    m_T_sys_h_t_int_fullts = std::numeric_limits<double>::quiet_NaN();			//[K]

    m_q_dot_sca_loss_summed_fullts = std::numeric_limits<double>::quiet_NaN();	//[MWt]
    m_q_dot_sca_abs_summed_fullts = std::numeric_limits<double>::quiet_NaN();	//[MWt]
    m_q_dot_sca_refl_summed_fullts = std::numeric_limits<double>::quiet_NaN();  //[MWt]
    m_q_dot_xover_loss_summed_fullts = std::numeric_limits<double>::quiet_NaN();	//[MWt]
    m_q_dot_HR_cold_loss_fullts = std::numeric_limits<double>::quiet_NaN();		//[MWt]
    m_q_dot_HR_hot_loss_fullts = std::numeric_limits<double>::quiet_NaN();		//[MWt]
    m_E_dot_sca_summed_fullts = std::numeric_limits<double>::quiet_NaN();	//[MWt]
    m_E_dot_xover_summed_fullts = std::numeric_limits<double>::quiet_NaN();	//[MWt]
    m_E_dot_HR_cold_fullts = std::numeric_limits<double>::quiet_NaN();		//[MWt]
    m_E_dot_HR_hot_fullts = std::numeric_limits<double>::quiet_NaN();		//[MWt]
    m_q_dot_htf_to_sink_fullts = std::numeric_limits<double>::quiet_NaN();	//[MWt]
    m_q_dot_freeze_protection = std::numeric_limits<double>::quiet_NaN();	//[MWt]

    m_q_dot_thermal_reported = std::numeric_limits<double>::quiet_NaN();    //[MWt]

    m_dP_total = std::numeric_limits<double>::quiet_NaN();		//[bar]
    m_W_dot_pump = std::numeric_limits<double>::quiet_NaN();	//[MWe]

    m_time_at_off = std::numeric_limits<double>::quiet_NaN();       //[s]
    m_time_at_startup = std::numeric_limits<double>::quiet_NaN();   //[s]
    m_time_at_on = std::numeric_limits<double>::quiet_NaN();        //[s]

    m_is_m_dot_recirc = false;

    m_W_dot_sca_tracking = std::numeric_limits<double>::quiet_NaN();	//[MWe]

    m_EqOpteff = std::numeric_limits<double>::quiet_NaN();
    m_m_dot_htf_tot = std::numeric_limits<double>::quiet_NaN();
    m_c_htf_ave = std::numeric_limits<double>::quiet_NaN();

    m_control_defocus = std::numeric_limits<double>::quiet_NaN();
    m_component_defocus = std::numeric_limits<double>::quiet_NaN();

    m_q_dot_inc_sf_tot = std::numeric_limits<double>::quiet_NaN();

    m_AnnulusGasMat.fill(NULL);
    m_AbsorberPropMat.fill(NULL);

    mv_HCEguessargs.resize(3);
    std::fill(mv_HCEguessargs.begin(), mv_HCEguessargs.end(), std::numeric_limits<double>::quiet_NaN());
}

C_csp_fresnel_collector_receiver::~C_csp_fresnel_collector_receiver()
{
    for (int i = 0; i < m_AbsorberPropMat.nrows(); i++) {
        for (int j = 0; j < m_AbsorberPropMat.ncols(); j++) {
            delete m_AbsorberPropMat(i, j);
            delete m_AnnulusGasMat(i, j);
        }
    }
}

void C_csp_fresnel_collector_receiver::init(const C_csp_collector_receiver::S_csp_cr_init_inputs init_inputs,
    C_csp_collector_receiver::S_csp_cr_solved_params& solved_params)
{   
    /*
        --Initialization call--

        Do any setup required here.
        Get the values of the inputs and parameters
    */

    //Initialize air properties -- used in reeiver calcs
    m_airProps.SetFluid(HTFProperties::Air);

    // Save init_inputs to member data
    {
        m_latitude = init_inputs.m_latitude;	//[deg]
        m_longitude = init_inputs.m_longitude;	//[deg]
        m_shift = init_inputs.m_shift;			//[deg]
        m_latitude *= m_d2r;		            //[rad] convert from [deg]
        m_longitude *= m_d2r;		            //[rad] convert from [deg]
        m_shift *= m_d2r;			            //[rad] convert from [deg]

        m_P_field_in = 17 / 1.e-5;              //Assumed inlet htf pressure for property lookups (DP_tot_max = 16 bar + 1 atm) [Pa]
    }

    // Set HTF properties
    {
        if (m_Fluid != HTFProperties::User_defined)
        {
            if (!m_htfProps.SetFluid(m_Fluid))
            {
                //message(TCS_ERROR, "Field HTF code is not recognized");
                string msg = "Field HTF code is not recognized";
                m_error_msg = util::format(msg.c_str());
                mc_csp_messages.add_message(C_csp_messages::NOTICE, m_error_msg);
                return;
            }
        }
        else if (m_Fluid == HTFProperties::User_defined)
        {
            int nrows = (int)m_field_fl_props.nrows();
            int ncols = (int)m_field_fl_props.ncols();
            if (nrows > 2 && ncols == 7)
            {

                if (!m_htfProps.SetUserDefinedFluid(m_field_fl_props))
                {
                    //message(TCS_ERROR, m_htfProps.UserFluidErrMessage(), nrows, ncols);
                    string msg = m_htfProps.UserFluidErrMessage();
                    m_error_msg = util::format(msg.c_str());
                    mc_csp_messages.add_message(C_csp_messages::NOTICE, m_error_msg);
                    return;
                }
            }
            else
            {
                //message(TCS_ERROR, "The user defined field HTF table must contain at least 3 rows and exactly 7 columns. The current table contains %d row(s) and %d column(s)", nrows, ncols);
                string msg = "The user defined field HTF table must contain at least 3 rows and exactly 7 columns. The current table contains %d row(s) and %d column(s)";
                m_error_msg = util::format(msg.c_str(), nrows, ncols);
                mc_csp_messages.add_message(C_csp_messages::NOTICE, m_error_msg);
                return;
            }
        }
        else
        {
            //message(TCS_ERROR, "Field HTF code is not recognized");
            string msg = "Field HTF code is not recognized";
            m_error_msg = util::format(msg.c_str());
            mc_csp_messages.add_message(C_csp_messages::NOTICE, m_error_msg);
            return;
        }
    }
    
    // Set up the optical table object..
    {
        /*
        The input should be defined as follows:
        - Data of size nx, ny
        - OpticalTable of size (nx+1)*(ny+1)
        - First nx+1 values (row 1) are x-axis values, not data, starting at index 1
        - First value of remaining ny rows are y-axis values, not data
        - Data is contained in cells i,j : where i>1, j>1
        */
        int ncol_OpticalTable = m_OpticalTable.ncols();
        int nrow_OpticalTable = m_OpticalTable.nrows();

        double* xax = new double[ncol_OpticalTable - 1];
        double* yax = new double[nrow_OpticalTable - 1];
        double* data = new double[(ncol_OpticalTable - 1) * (nrow_OpticalTable - 1)];

        //get the xaxis data values
        for (int i = 1; i < ncol_OpticalTable; i++) {
            xax[i - 1] = m_OpticalTable.at(0, i) * m_d2r;
        }
        //get the yaxis data values
        for (int j = 1; j < nrow_OpticalTable; j++) {
            yax[j - 1] = m_OpticalTable.at(j, 0) * m_d2r;
        }
        //Get the data values
        for (int j = 1; j < nrow_OpticalTable; j++) {
            for (int i = 1; i < ncol_OpticalTable; i++) {
                data[i - 1 + (ncol_OpticalTable - 1) * (j - 1)] = m_OpticalTable.at(j, i);
            }
        }

        optical_table.AddXAxis(xax, ncol_OpticalTable - 1);
        optical_table.AddYAxis(yax, nrow_OpticalTable - 1);
        optical_table.AddData(data);
        delete[] xax;
        delete[] yax;
        delete[] data;

    }

    // Adjust parameters
    m_ColAz = m_ColAz * m_d2r;		//[rad] Collector azimuth angle, convert from [deg]

    // Organize the emittance tables
    {
        m_epsilon_abs.init(4);
        m_epsilon_abs.addTable(&m_epsilon_abs_1);	//HCE #1
        m_epsilon_abs.addTable(&m_epsilon_abs_2);
        m_epsilon_abs.addTable(&m_epsilon_abs_3);
        m_epsilon_abs.addTable(&m_epsilon_abs_4);
    }

    // Unit Conversions
    {
        m_theta_stow *= m_d2r;
        m_theta_stow = max(m_theta_stow, 1.e-6);
        m_theta_dep *= m_d2r;
        m_theta_dep = max(m_theta_dep, 1.e-6);
        m_T_startup += 273.15;			//[K] convert from C
        m_T_loop_in_des += 273.15;		//[K] convert from C
        m_T_loop_out_des += 273.15;			//[K] convert from C
        m_T_fp += 273.15;				//[K] convert from C
        m_mc_bal_sca *= 3.6e3;			//[Wht/K-m] -> [J/K-m]
    }
    
    /*--- Do any initialization calculations here ---- */
    //Allocate space for the loop simulation objects
    {

        // Old Fresnel
        m_q_loss.resize(m_nRecVar);
        m_q_abs.resize(m_nRecVar);
        m_DP_tube.resize(m_nMod);
        m_E_int_loop.resize(m_nMod);
        m_E_accum.resize(m_nMod);
        m_E_avail.resize(m_nMod);
        m_q_loss_SCAtot.resize(m_nMod);
        m_q_abs_SCAtot.resize(m_nMod);
        m_q_SCA.resize(m_nMod);
        m_q_1abs_tot.resize(m_nMod);
        m_q_1abs.resize(m_nRecVar);
        m_q_reflect_tot.resize(m_nMod);
        m_q_reflect.resize(m_nRecVar);
        m_ColOptEff.resize(m_nMod);

        // Trough
        m_q_SCA_control_df.resize(m_nMod);
    }
    
    // Resize CSP Solver Temp Tracking Vectors
    {
        m_T_htf_out_t_end_converged.resize(m_nMod);
        m_T_htf_out_t_end_last.resize(m_nMod);
        m_T_htf_in_t_int.resize(m_nMod);
        m_T_htf_out_t_end.resize(m_nMod);
        m_T_htf_out_t_int.resize(m_nMod);
    }
    
    //Set up annulus gas and absorber property matrices
    {
        // Old Fresnel Version
        //Set up annulus gas and absorber property matrices
        m_AnnulusGasMat.resize(m_nRecVar);
        m_AbsorberPropMat.resize(m_nRecVar);
        for (int j = 0; j < m_nRecVar; j++) {
            //Set up a matrix of annulus gas properties
            m_AnnulusGasMat.at(j) = new HTFProperties();
            m_AnnulusGasMat.at(j)->SetFluid((int)m_AnnulusGas[j]);
            //Set up a matrix of absorber prop materials
            m_AbsorberPropMat.at(j) = new AbsorberProps();
            m_AbsorberPropMat.at(j)->setMaterial((int)m_AbsorberMaterial[j]);
        }
    }

    //Initialize values

    // for test start
    init_fieldgeom();
    // for test end

    // Set solved parameters
    solved_params.m_T_htf_cold_des = m_T_loop_in_des;	//[K]
    solved_params.m_q_dot_rec_des = m_q_design_actual / 1.E6;	//[MWt]
    solved_params.m_A_aper_total = m_Ap_tot;			//[m^2]

    // Set previous operating mode
    m_operating_mode_converged = C_csp_collector_receiver::OFF;					//[-] 0 = requires startup, 1 = starting up, 2 = running

    // Create Evacuated Receiver Model (if necessary)
    if (m_rec_model == 2)
    {
        m_evac_receiver = std::unique_ptr<EvacReceiverModel>(new EvacReceiverModel(m_D_abs_in, m_D_abs_out, m_D_glass_in, m_D_glass_out, m_D_plug, m_L_mod, m_GlazingIntact,
            m_Shadowing, m_dirt_env, m_P_a, m_alpha_abs, m_epsilon_glass, m_Tau_envelope, m_alpha_env, &m_epsilon_abs,
            m_htfProps, m_airProps, m_AnnulusGasMat, m_AbsorberPropMat, m_Flow_type, m_A_cs, m_D_h));
    }

    // Run steady state 
    {
        C_csp_weatherreader::S_outputs weatherValues;
        weatherValues.m_lat = init_inputs.m_latitude;
        weatherValues.m_lon = init_inputs.m_longitude;
        weatherValues.m_tz = init_inputs.m_tz;
        weatherValues.m_shift = init_inputs.m_shift;
        weatherValues.m_elev = init_inputs.m_elev;
        weatherValues.m_year = 2009;
        weatherValues.m_month = 6;
        weatherValues.m_day = 21;
        weatherValues.m_hour = 12;
        weatherValues.m_minute = 0;
        weatherValues.m_beam = m_I_bn_des;
        weatherValues.m_tdry = 30;
        weatherValues.m_tdew = 30 - 10;
        weatherValues.m_wspd = 5;
        weatherValues.m_pres = 1013;
        weatherValues.m_solazi = m_ColAz;
        weatherValues.m_solzen = 0;

        C_csp_solver_htf_1state htfInletState;
        //htfInletState.m_m_dot = m_m_dot_design;
        //htfInletState.m_pres = 101.3;
        //htfInletState.m_qual = 0;
        htfInletState.m_temp = m_T_loop_in_des - 273.15;
        double defocus = 1;
        C_csp_solver_sim_info fresnelInfo;
        fresnelInfo.ms_ts.m_time_start = 14817600.;
        fresnelInfo.ms_ts.m_step = 15. * 60.;               // 5-minute timesteps
        fresnelInfo.ms_ts.m_time = fresnelInfo.ms_ts.m_time_start + fresnelInfo.ms_ts.m_step;
        fresnelInfo.m_tou = 1.;
        C_csp_collector_receiver::S_csp_cr_out_solver fresnelOutputs;

        steady_state(weatherValues, htfInletState, std::numeric_limits<double>::quiet_NaN(), defocus, fresnelOutputs, fresnelInfo);
        solved_params.m_T_htf_hot_des = m_T_field_out;
        solved_params.m_dP_sf = fresnelOutputs.m_dP_sf;




        // Calculate Steady State Results
        {
            // Field Results
            double temp_initial = htfInletState.m_temp + 273.15;  // K
            double temp_final = m_T_field_out;  // K
            double mdot = fresnelOutputs.m_m_dot_salt_tot / 3600.0; // convert from kg/hr to kg/s
            double c_htf_ave = m_htfProps.Cp((temp_initial + temp_final) / 2.0);  //[kJ/kg-K]
            m_Q_field_des_SS = mdot * c_htf_ave * (temp_final - temp_initial) * 1000.0; // convert kW to W
            m_T_field_out_des_SS = fresnelOutputs.m_T_salt_hot;  // C
            m_m_dot_des_SS = mdot;  // kg/s field
            m_m_dot_loop_des_SS = mdot / float(m_nLoops);

            // Steady State velocities
            {
                double D_hdr_min = *std::min_element(m_D_hdr.begin(), m_D_hdr.end());
                double D_hdr_max = *std::max_element(m_D_hdr.begin(), m_D_hdr.end());
                double mdot_hdr = mdot / m_nfsec;
                double rho_ave = m_htfProps.dens((temp_initial + temp_final) / 2.0, 0.0); //kg/m3

                double V_min_calc = -1;
                double V_max_calc = -1;
                std::vector<double> mdot_vec;
                for (int i = 0; i < m_nhdrsec; i++)
                {
                    double mdot_hdr_section = this->m_dot_header(mdot, m_nfsec, this->m_nLoops, i);
                    double D_hdr_section = m_D_hdr[i];

                    double V = (4.0 * mdot_hdr_section) / (rho_ave * CSP::pi * pow(D_hdr_section, 2.0));

                    if (i == 0)
                    {
                        V_min_calc = V;
                        V_max_calc = V;
                    }
                    else if (V < V_min_calc)
                        V_min_calc = V;
                    else if (V > V_max_calc)
                        V_max_calc = V;


                    mdot_vec.push_back(mdot_hdr_section);
                }

                m_V_hdr_min_des_SS = V_min_calc;
                m_V_hdr_max_des_SS = V_max_calc;


                double max_field_mdot = m_m_dot_htfmax * float(m_nLoops);
                double max_hdr_mdot = max_field_mdot / m_nfsec;
                double max_velocity_based_on_max_htf_mdot = (4.0 * max_hdr_mdot) / (rho_ave * CSP::pi * pow(0.48895, 2.0));

                double min_field_mdot = m_m_dot_htfmin * float(m_nLoops);
                double min_hdr_mdot = min_field_mdot / m_nfsec;
                double min_velocity_based_on_min_htf_mdot = (4.0 * min_hdr_mdot) / (rho_ave * CSP::pi * pow(0.48895, 2.0));
            }

            // SS optical efficiency is collector optical efficiency * receiver OPTICAL efficiency (does not consider heat loss)
            m_eta_optical_des_SS = this->m_eta_optical * m_opt_derate;

            // Field Efficiency
            double Q_available = m_Ap_tot * weatherValues.m_beam * m_eta_optical_des_SS;  // W
            m_therm_eff_des_SS = m_Q_field_des_SS / Q_available;
            m_eff_des_SS = m_eta_optical_des_SS * m_therm_eff_des_SS;

            // Loop Results
            double loop_in = m_T_loop_in;                               // K
            double loop_out = m_T_htf_out_t_int[m_nMod - 1];            // K
            double c_htf_loop_ave = m_htfProps.Cp((loop_in + loop_out) / 2.0);  //[kJ/kg-K]
            m_Q_loop_des_SS = m_m_dot_loop_des_SS * c_htf_loop_ave * (loop_out - loop_in) * 1000.0;  // convert kW to W
            m_T_loop_out_des_SS = loop_out - 273.15;                    // Convert from K to C
            double Q_loop_available = m_A_loop * weatherValues.m_beam * m_eta_optical_des_SS; // W
            m_therm_eff_loop_des_SS = m_Q_loop_des_SS / Q_loop_available;
            m_eff_loop_des_SS = m_therm_eff_loop_des_SS * m_eta_optical_des_SS;

            // Pumping Power
            m_W_dot_pump_des_SS = m_W_dot_pump;

            // Field Pressure Drop
            m_dP_des_SS = fresnelOutputs.m_dP_sf;

            // Thermal Losses
            m_Q_loss_receiver_des_SS = m_q_dot_sca_loss_summed_fullts; // MWt
            m_Q_loss_hdr_rnr_des_SS = m_q_dot_HR_cold_loss_fullts + m_q_dot_HR_hot_loss_fullts;  // MWt

        }

        
    }

    return;
}

bool C_csp_fresnel_collector_receiver::init_fieldgeom()
{
    /*
        Call this method once when call() is first invoked. The calculations require location information that
        is provided by the weatherreader class and not set until after init() and before the first call().
        */

    // If solar multiple is not yet calculated
    if(m_is_solar_mult_designed == false)
        this->design_solar_mult(m_latitude / m_d2r);

    if (m_rec_model == 2)
    {
        //Evacuated tube receiver model
        //Calculate the cross-sectional flow area of the receiver piping
        m_D_h.resize(m_nRecVar);
        m_A_cs.resize(m_nRecVar);
        for (int i = 0; i < m_nRecVar; i++) {

            if ((int)m_Flow_type[i] == 2) {
                m_D_h.at(i) = m_D_abs_in[i] - m_D_plug[i];
            }
            else {
                m_D_h.at(i) = m_D_abs_in[i];
                m_D_plug[i] = 0.;
            }
            m_A_cs.at(i) = CSP::pi * (m_D_abs_in[i] * m_D_abs_in[i] - m_D_plug[i] * m_D_plug[i]) / 4.;  //[m2] The cross-sectional flow area
        }
    }

    //Calculate header diameters here based on min/max velocities
        //output file with calculated header diameter "header_diam.out"
    m_nfsec = m_FieldConfig;
    if (m_nfsec % 2 != 0) {
        //message(TCS_ERROR, "Number of field subsections must equal an even number");
        string msg = "Number of field subsections must equal an even number";
        m_error_msg = util::format(msg.c_str());
        mc_csp_messages.add_message(C_csp_messages::NOTICE, m_error_msg);
        return false;
    }

    /*
    The number of header sections per field section is equal to the total number of loops divided
    by the number of distinct headers. Since two loops are connected to the same header section,
    the total number of header sections is then divided by 2.
    */
    m_nhdrsec = (int)ceil(float(m_nLoops) / float(m_nfsec * 2));

    //We need to determine design information about the field for purposes of header sizing ONLY
    m_c_htf_ave = m_htfProps.Cp((m_T_loop_out_des + m_T_loop_in_des) / 2.0) * 1000.;    //Specific heat

    //Start by initializing sensitive variables
    double x1 = 0.0, loss_tot = 0.0;
    m_opteff_des = 0.0;
    m_m_dot_design = 0.0;
    m_L_tot = (float)m_nMod * m_L_mod;

    //Determine the optical efficiency at design
    eta_opt_fixed = m_TrackingError * m_GeomEffects * m_reflectivity * m_Dirt_mirror * m_Error;
    //design point solar elevation
    double elev_des = asin(sin(0.4092793) * sin(m_latitude) + cos(m_latitude) * cos(0.4092793));
    //translate the solar angles into incidence angles
    double phi_t, theta_L, iam_t, iam_l;
    CSP::theta_trans(0., CSP::pi / 2. - elev_des, m_ColAz, phi_t, theta_L);	//phi_t and theta_L are the translated angles (transverse and longitudinal)

    switch (m_opt_model)
    {
        case 1:		//Solar position table
            m_opteff_des = eta_opt_fixed * optical_table.interpolate(0., CSP::pi / 2. - elev_des);
            break;
        case 2:		//Collector incidence table
            m_opteff_des = eta_opt_fixed * optical_table.interpolate(0., theta_L);
            //optical_IAM = optical_table.interpolate(0., theta_L);
            //m_opteff_des = eta_opt_fixed;
            break;
        case 3:		//IAM polynomials
        {
            iam_t = 0.;
            iam_l = 0.;
            int n_IAM_L_coefs = m_IAM_L_coefs.size();
            int n_IAM_T_coefs = m_IAM_T_coefs.size();
            for (int i = 0; i < n_IAM_L_coefs; i++)
                iam_l += m_IAM_L_coefs[i] * pow(theta_L, i);
            for (int i = 0; i < n_IAM_T_coefs; i++)
                iam_t += m_IAM_T_coefs[i] * pow(phi_t, i);
            m_opteff_des = eta_opt_fixed * iam_t * iam_l;
            break;
        }
        default:
            //message(TCS_ERROR, "The selected optical model (%d) does not exist. Options are 1=Solar position table : 2=Collector incidence table : 3= IAM polynomials", opt_model);
            string msg = "The selected optical model (%d) does not exist. Options are 1=Solar position table : 2=Collector incidence table : 3= IAM polynomials";
            m_error_msg = util::format(msg.c_str(), m_opt_model);
            mc_csp_messages.add_message(C_csp_messages::NOTICE, m_error_msg);
            return false;
    }

    //Determine the heat loss efficiency at design
    double dT_loc, c_hl, dTSCA, c_hl_w, hceopt;
    switch (m_rec_model)
    {
        //Polynomial model
        case 1:
            //evaluate the wind speed polynomial
        {
            c_hl_w = 0.;
            int n_HL_w_coefs = m_HL_w_coefs.size();
            int n_HL_T_coefs = m_HL_T_coefs.size();

            for (int j = 0; j < n_HL_w_coefs; j++) {
                c_hl_w += m_HL_w_coefs[j] * pow(m_V_wind_des, j);
            }

            //Assume a linear temperature rise across the field
            c_hl = 0.;
            dTSCA = (m_T_loop_out_des - m_T_loop_in_des) / (float)(m_nMod + 1);
            for (int j = 0; j < m_nMod; j++) {
                dT_loc = m_T_loop_in_des + dTSCA * (0.5 + (float)j) - m_T_amb_sf_des;
                //evaluate the temperature polynomial
                for (int k = 0; k < n_HL_T_coefs; k++) {
                    c_hl += m_HL_T_coefs[k] * pow(dT_loc, k) * m_L_mod;		//Total receiver thermal loss [W/m] for a single loop
                }
            }
            //Calculate the total thermal loss, including temperature and wind loss effects, for the entire loop
            loss_tot = c_hl_w * c_hl;

            break;
        }

        //Evacuated tube receiver model
        case 2:
            loss_tot = 0.;
            for (int j = 0; j < m_nRecVar; j++)
                loss_tot += (float)m_nMod * m_L_mod * m_HCE_FieldFrac[j] * m_Design_loss[j];
            //correct for receiver optical losses
            hceopt = 0.;
            for (int i = 0; i < m_nRecVar; i++) {
                // TMB 12-06-2023 Add shadowing and envelope dirt losses to optical loss
                //hceopt += m_alpha_abs[i] * m_Tau_envelope[i] * m_HCE_FieldFrac[i];
                hceopt += m_alpha_abs[i] * m_Tau_envelope[i] * m_HCE_FieldFrac[i] * m_Shadowing[i] * m_dirt_env[i];
            }
            m_opteff_des *= hceopt;
            break;

        default:
            //message(TCS_ERROR, "The selected thermal model (%d) does not exist. Options are 1=Regression model : 2=Evacuated tube receiver model", rec_model);
            string msg = "The selected thermal model (%d) does not exist. Options are 1=Regression model : 2=Evacuated tube receiver model";
            m_error_msg = util::format(msg.c_str(), m_rec_model);
            mc_csp_messages.add_message(C_csp_messages::NOTICE, m_error_msg);
            return false;
    }

    //the estimated mass flow rate at design (in solar field)
    m_m_dot_design = (m_Ap_tot * m_I_bn_des * m_opteff_des - loss_tot * float(m_nLoops)) / (m_c_htf_ave * (m_T_loop_out_des - m_T_loop_in_des));  //tn 4.25.11 using Ap_tot instead of A_loop. Change location of opteff_des

    // 'ideal' m_q_design before mass flow limits
    m_q_design_ideal = m_m_dot_design * m_c_htf_ave * (m_T_loop_out_des - m_T_loop_in_des); //[Wt]


    double thermal_field = m_Ap_tot * m_I_bn_des * m_opteff_des - loss_tot * float(m_nLoops);

    // Calculate min and max mass flow
    m_m_dot_loop_des = m_m_dot_design / (double)m_nLoops;	//[kg/s]

    double m_dot_field_min = 0.0;
    double m_dot_field_max = 0.0;

    // Use absolute mdot limit
    if (m_use_abs_or_rel_mdot_limit == 0)
    {
        m_dot_field_min = m_m_dot_htfmin_in * (double)m_nLoops; //[kg/s]
        m_dot_field_max = m_m_dot_htfmax_in * (double)m_nLoops; //[kg/s]
        m_m_dot_htfmin = m_m_dot_htfmin_in;
        m_m_dot_htfmax = m_m_dot_htfmax_in;
        m_f_htfmin = m_m_dot_htfmin_in / m_m_dot_loop_des;
        m_f_htfmax = m_m_dot_htfmax_in / m_m_dot_loop_des;
    }
    // Use relative mdot limit
    else
    {
        m_dot_field_min = m_f_htfmin_in * m_m_dot_design;   //[kg/s]
        m_dot_field_max = m_f_htfmax_in * m_m_dot_design;   //[kg/s]
        m_m_dot_htfmin = m_dot_field_min / (double)m_nLoops;
        m_m_dot_htfmax = m_dot_field_max / (double)m_nLoops;
        m_f_htfmin = m_f_htfmin_in;
        m_f_htfmax = m_f_htfmax_in;
    }

    if (m_m_dot_design > m_dot_field_max) {
        const char* msg = "The calculated field design mass flow rate of %.2f kg/s is greater than the maximum defined by the max single loop flow rate and number of loops (%.2f kg/s). "
            "The design mass flow rate is reset to the latter.";
        m_error_msg = util::format(msg, m_m_dot_design, m_dot_field_max);
        mc_csp_messages.add_message(C_csp_messages::NOTICE, m_error_msg);
        m_m_dot_design = m_dot_field_max;
    }
    else if (m_m_dot_design < m_dot_field_min) {
        const char* msg = "The calculated field design mass flow rate of %.2f kg/s is less than the minimum defined by the min single loop flow rate and number of loops (%.2f kg/s). "
            "The design mass flow rate is reset to the latter.";
        m_error_msg = util::format(msg, m_m_dot_design, m_dot_field_min);
        mc_csp_messages.add_message(C_csp_messages::NOTICE, m_error_msg);
        m_m_dot_design = m_dot_field_min;
    }

    // Recalculate design loop mass flow with bounded field mass flow
    m_m_dot_loop_des = m_m_dot_design / (double)m_nLoops; // [kg/s]

    // Already defined in design_solar_mult (TB)
    //mjw 1.16.2011 Design field thermal power
    m_q_design_actual = m_m_dot_design * m_c_htf_ave * (m_T_loop_out_des - m_T_loop_in_des); //[Wt]

    double q_design_old = m_I_bn_des * m_Ap_tot * m_loop_eff;

    //mjw 1.16.2011 Convert the thermal inertia terms here
    m_mc_bal_hot = m_mc_bal_hot * 3.6 * m_q_design_actual;    //[J/K]
    m_mc_bal_cold = m_mc_bal_cold * 3.6 * m_q_design_actual;  //[J/K]



    //need to provide fluid density
    double rho_ave = m_htfProps.dens((m_T_loop_out_des + m_T_loop_in_des) / 2.0, 0.0); //kg/m3
    //Calculate the header design
    m_nrunsec = (int)floor(float(m_nfsec) / 4.0) + 1;  //The number of unique runner diameters
    m_T_loop.resize(2 * m_nMod + 3);
    m_T_rnr.resize(2 * m_nrunsec);
    m_T_hdr.resize(2 * m_nhdrsec);
    m_D_runner.resize(m_nrunsec);
    m_L_runner.resize(m_nrunsec);
    m_D_hdr.resize(m_nhdrsec);
    m_P_rnr.resize(2 * m_nrunsec);
    m_P_hdr.resize(2 * m_nhdrsec);
    m_P_rnr_dsn = m_P_rnr;
    m_DP_rnr.resize(2 * m_nrunsec);
    m_T_rnr_dsn = m_T_rnr;
    m_T_hdr_dsn = m_T_hdr;
    m_DP_hdr.resize(2 * m_nhdrsec);
    m_T_loop_dsn = m_T_loop;

    // Removed (polynomial model does not have pressure across individual receivers)
    //m_DP_loop.resize(2 * m_nMod + 3);
    //m_P_loop.resize(2 * m_nMod + 3);

    header_design(m_nhdrsec, m_nfsec, m_nrunsec, rho_ave, m_V_hdr_max, m_V_hdr_min, m_m_dot_design, m_D_hdr, m_D_runner, &m_piping_summary);
    mc_csp_messages.add_message(C_csp_messages::NOTICE, m_piping_summary);

    /* ----- Set initial storage values ------ */
    double T_field_ini = 0.5 * (m_T_fp + m_T_loop_in_des);	//[K]

    /*
    Do one-time calculations for system geometry. Calculate all HTF volume, set runner piping length
    Assume there are two field subsections per span, then if there's an even number of spans in the field,
    we count the first header section as half-length. I.e., if a field looks like this:
       (1)        (2)
     |||||||   |||||||
     -----------------
     ||||||| : |||||||
             :
            [P]
             :
     ||||||| : |||||||
     -----------------
     |||||||   |||||||
       (3)        (4)
    Then the field has 4 subfields and two spans. The runner pipe (:) is half the distance between the two spans.
    If the number of subfields were 6 (3 spans), the two runner pipe segments would both be equal to the full
    distance between spans.
    */
    if (m_nfsec / 2 % 2 == 1) {
        x1 = 2.;     //the first runners are normal
    }
    else {
        x1 = 1.;     //the first runners are short
    }
    m_L_runner[0] = m_L_rnr_pb;
    if (m_nrunsec > 1) {
        for (int i = 1; i < m_nrunsec; i++) {
            m_L_runner[i] = x1 * (2 * m_L_crossover + (m_L_mod + m_L_mod_spacing) * float(m_nMod) / 2.);
            x1 = 2.;   //tn 4.25.11 Default to 2 for subsequent runners
        }
    }
    double v_tofrom_sgs = 0.0;
    for (int i = 0; i < m_nrunsec; i++) {
        v_tofrom_sgs = v_tofrom_sgs + 2. * m_L_runner[i] * CSP::pi * pow(m_D_runner[i], 2) / 4.;  //This is the volume of the runner in 1 direction.
    }

    //6/14/12, TN: Multiplier for runner heat loss. In main section of code, are only calculating loss for one path.
    //Since there will be two symmetric paths (when nrunsec > 1), need to calculate multiplier for heat loss, considering
    //that the first 50 meters of runner is assumed shared.
    double lsum = 0.;
    for (int i = 0; i < m_nrunsec; i++) { lsum += m_L_runner[i]; }

    //Calculate the total HTF volume per loop based on user input. Select method based on heat loss model
    double v_loop_tot = 0.;
    switch (m_rec_model)
    {
        case 1:		//Polynomial model
            v_loop_tot = m_A_loop * m_rec_htf_vol / 1000. * (float)m_nLoops; //[m3]
            break;
        case 2:
            //-------piping from header into and out of the HCE's
            for (int j = 0; j < m_nRecVar; j++) {
                for (int i = 0; i < m_nMod; i++) {
                    v_loop_tot += (m_L_mod + m_L_mod_spacing) * m_A_cs.at(j) * m_HCE_FieldFrac[j] * (float)m_nLoops;
                }
            }
            //mjw 1.13.2011 Add on volume for the crossover piping 
            //v_loop_tot = v_loop_tot + L_crossover*A_cs(SCAInfoArray(nMod/2,1),1)*float(nLoops)
            v_loop_tot += m_L_crossover * m_A_cs.at(0) * (float)m_nLoops;      //TN 6/20: need to solve for nMod = 1
            break;
        default:
            //message(TCS_ERROR, "The selected thermal model (%d) does not exist. Options are 1=Regression model : 2=Evacuated tube receiver model", rec_model);
            string msg = "The selected thermal model (%d) does not exist. Options are 1=Regression model : 2=Evacuated tube receiver model";
            m_error_msg = util::format(msg.c_str(), m_rec_model);
            mc_csp_messages.add_message(C_csp_messages::NOTICE, m_error_msg);
            return false;
    }

    //-------field header loop
    double v_header = 0.0;
    for (int i = 0; i < m_nhdrsec; i++) {
        //Also calculate the hot and cold header volume for later use. 4.25 is for header expansion bends
        v_header += m_D_hdr[i] * m_D_hdr[i] / 4. * CSP::pi * (m_L_crossover + 4.275) * float(m_nfsec) * 2.0;  //tn 4.25.11 The header distance should be multiplied by 2 row spacings
    }
    //Add on inlet/outlet from the header to the loop. Assume header to loop inlet ~= 10 [m] (Kelley/Kearney)
    if (m_rec_model == 2) v_header = v_header + 20. * m_A_cs.at(0) * float(m_nLoops);

    //Calculate the HTF volume associated with pumps and the SGS
    double v_sgs = Pump_SGS(rho_ave, m_m_dot_design, m_solar_mult);

    //Calculate the hot and cold balance-of-plant volumes
    m_v_hot = v_header + v_tofrom_sgs;
    m_v_cold = m_v_hot;

    //Write the volume totals to the piping diameter file
    m_piping_summary.append(
        "\n----------------------------------------------\n"
        "Plant HTF volume information:\n"
        "----------------------------------------------\n");
    char tstr[500];
    string fmt = "Cold header pipe volume:   %10.4e m3\n"
        "Hot header pipe volume:    %10.4e m3\n"
        "Volume per loop:           %10.4e m3\n"
        "Total volume in all loops: %10.4e m3\n"
        "Total solar field volume:  %10.4e m3\n"
        "Pump / SGS system volume:  %10.4e m3\n"
        "---------------------------\n"
        "Total plant HTF volume:    %10.4e m3\n";
    sprintf(tstr, fmt.c_str(), m_v_cold, m_v_hot, v_loop_tot / float(m_nLoops), v_loop_tot, (m_v_hot * 2. + v_loop_tot), v_sgs, (m_v_hot * 2. + v_loop_tot + v_sgs));
    //piping_summary.append(tstr);

    //Include the pump/SGS volume with the header
    m_v_hot = m_v_hot + v_sgs / 2.;
    m_v_cold = m_v_cold + v_sgs / 2.;

    // *********************************************
         // CSP Solver Temperature Tracking
    m_T_sys_c_t_end_converged = m_T_sys_c_t_end_last = T_field_ini;	//[K]
    m_T_sys_h_t_end_converged = m_T_sys_h_t_end_last = T_field_ini; //[K]
    for (int i = 0; i < m_nMod; i++)
    {
        m_T_htf_out_t_end_converged[i] = m_T_htf_out_t_end_last[i] = T_field_ini;	//[K]
    }
    // *********************************************

    // Calculate tracking parasitics for when fresnel is on sun
    m_W_dot_sca_tracking_nom = m_SCA_drives_elec * (double)(m_nMod * m_nLoops) / 1.E6;	//[MWe]

    return true;
}

C_csp_collector_receiver::E_csp_cr_modes C_csp_fresnel_collector_receiver::get_operating_state()
{
    return m_operating_mode_converged;
}

double C_csp_fresnel_collector_receiver::get_startup_time()
{
    // Note: C_csp_fresnel_collector_receiver::startup() is called after this function
    return m_rec_su_delay * 3600.;                    // sec
}

double C_csp_fresnel_collector_receiver::get_startup_energy()
{
    // Note: C_csp_fresnel_collector_receiver::startup() is called after this function
    return m_rec_qf_delay * m_q_design_actual * 1.e-6;       // MWh
}

double C_csp_fresnel_collector_receiver::get_pumping_parasitic_coef()
{
    double T_amb_des = 42. + 273.15;
    double T_avg = (m_T_loop_in_des + m_T_loop_out_des) / 2.;
    double P_field_in = m_P_rnr_dsn[1]; // hard code?
    double dT_avg_SCA = (m_T_loop_out_des - m_T_loop_in_des) / m_nMod;
    std::vector<double> T_in_SCA, T_out_SCA;

    for (size_t i = 0; i < m_nMod; i++) {
        T_in_SCA.push_back(m_T_loop_in_des + dT_avg_SCA * i);
        T_out_SCA.push_back(m_T_loop_in_des + dT_avg_SCA * (i + 1));
    }

    double dP_field = field_pressure_drop(T_amb_des, m_m_dot_design, P_field_in, T_in_SCA, T_out_SCA);

    return m_W_dot_pump / (m_q_design_actual * 1.e-6);
}

double C_csp_fresnel_collector_receiver::get_min_power_delivery()
{
    double c_htf_ave = m_htfProps.Cp((m_T_startup + m_T_loop_in_des) / 2.0) * 1000.;    //[J/kg-K] Specific heat
    return m_m_dot_htfmin * m_nLoops * c_htf_ave * (m_T_startup - m_T_loop_in_des) * 1.e-6;     // [MWt]
}

double C_csp_fresnel_collector_receiver::get_max_power_delivery(double T_cold_in /*C*/)
{
    double T_in = T_cold_in + 273.15;                                          // [K]
    double T_out = m_T_loop_out_des;                                           // [K]
    double c_htf_ave = m_htfProps.Cp((T_out + T_in) / 2.0) * 1000.;            // [J/kg-K]
    return m_m_dot_htfmax * m_nLoops * c_htf_ave * (T_out - T_in) * 1.e-6;     // [MWt]
}

double C_csp_fresnel_collector_receiver::get_tracking_power()
{
    return m_SCA_drives_elec * 1.e-6 * m_nMod * m_nLoops;     //MWe
}

double C_csp_fresnel_collector_receiver::get_col_startup_power()
{
    return m_p_start * 1.e-3 * m_nMod * m_nLoops;             //MWe-hr
}

void C_csp_fresnel_collector_receiver::get_design_parameters(C_csp_collector_receiver::S_csp_cr_solved_params& solved_params)
{
    return;
}

void C_csp_fresnel_collector_receiver::off(const C_csp_weatherreader::S_outputs& weather,
    const C_csp_solver_htf_1state& htf_state_in,
    C_csp_collector_receiver::S_csp_cr_out_solver& cr_out_solver,
    const C_csp_solver_sim_info& sim_info)
{
    // Always reset last temps
    reset_last_temps();

    m_is_m_dot_recirc = true;

    // Get optical properties
        // Should reflect that the collector is not tracking and probably (but not necessarily) DNI = 0
    loop_optical_eta_off();

    // Set mass flow rate to minimum allowable
    double m_dot_htf_loop = m_m_dot_htfmin;		//[kg/s]

    // Set duration for recirculation timestep
    if (m_step_recirc != m_step_recirc)
        m_step_recirc = 10.0 * 60.0;	//[s]

    // Calculate number of steps required given timestep from solver and recirculation step
    int n_steps_recirc = (int)std::ceil(sim_info.ms_ts.m_step / m_step_recirc);	//[-] Number of recirculation steps required

    // Define a copy of the sim_info structure
    double time_start = sim_info.ms_ts.m_time - sim_info.ms_ts.m_step;	//[s]
    double step_local = sim_info.ms_ts.m_step / (double)n_steps_recirc;	//[s]
    C_csp_solver_sim_info sim_info_temp = sim_info;
    sim_info_temp.ms_ts.m_step = step_local;		//[s]

    double Q_fp_sum = 0.0;				//[MJ]

    // Zero full timestep outputs
    m_T_sys_c_t_int_fullts = m_T_htf_c_rec_in_t_int_fullts =
        m_T_htf_h_rec_out_t_int_fullts = m_T_sys_h_t_int_fullts = 0.0;	//[K]

    m_q_dot_sca_loss_summed_fullts = m_q_dot_sca_abs_summed_fullts =
        m_q_dot_sca_loss_summed_fullts = m_q_dot_xover_loss_summed_fullts =
        m_q_dot_HR_cold_loss_fullts = m_q_dot_HR_hot_loss_fullts =
        m_E_dot_sca_summed_fullts = m_E_dot_xover_summed_fullts =
        m_E_dot_HR_cold_fullts = m_E_dot_HR_hot_fullts =
        m_q_dot_htf_to_sink_fullts = m_q_dot_thermal_reported = 0.0;

    // Simulate through time steps
    for (int i = 0; i < n_steps_recirc; i++)
    {
        sim_info_temp.ms_ts.m_time = time_start + step_local * (i + 1); //[s]

        // Set inlet temperature to previous timestep outlet temperature
        double T_cold_in = m_T_sys_h_t_end_last;			//[K]

        // Call energy balance with updated info
        loop_energy_balance_T_t_int(weather, T_cold_in, m_dot_htf_loop, sim_info_temp);

        // Check freeze protection
        if (m_T_htf_out_t_end[m_nMod - 1] < m_T_fp + fp_offset)
        {
            if (m_Q_field_losses_total_subts > 0.0)
            {
                double Q_fp_i = std::numeric_limits<double>::quiet_NaN();
                double T_cold_in_i = T_cold_in;
                int fp_code = freeze_protection(weather, T_cold_in_i, m_dot_htf_loop, sim_info_temp, Q_fp_i);

                T_cold_in = T_cold_in_i;	//[K]
                Q_fp_sum += Q_fp_i;			//[MJ]
            }
        }

        // Add current temperature to summation
        m_T_sys_c_t_int_fullts += T_cold_in;					//[K]
        m_T_htf_c_rec_in_t_int_fullts += m_T_htf_in_t_int[0];	//[K]
        m_T_htf_h_rec_out_t_int_fullts += m_T_htf_out_t_int[m_nMod - 1];	//[K]
        m_T_sys_h_t_int_fullts += m_T_sys_h_t_int;				//[K]

        // Add subtimestep calcs
        m_q_dot_sca_loss_summed_fullts += m_q_dot_sca_loss_summed_subts;		//[MWt]
        m_q_dot_sca_abs_summed_fullts += m_q_dot_sca_abs_summed_subts;			//[MWt]
        m_q_dot_sca_refl_summed_fullts += m_q_dot_sca_refl_summed_subts;        //[MWt]
        m_q_dot_xover_loss_summed_fullts += m_q_dot_xover_loss_summed_subts;	//[MWt]
        m_q_dot_HR_cold_loss_fullts += m_q_dot_HR_cold_loss_subts;				//[MWt]
        m_q_dot_HR_hot_loss_fullts += m_q_dot_HR_hot_loss_subts;				//[MWt]
        m_E_dot_sca_summed_fullts += m_E_dot_sca_summed_subts;					//[MWt]
        m_E_dot_xover_summed_fullts += m_E_dot_xover_summed_subts;				//[MWt]
        m_E_dot_HR_cold_fullts += m_E_dot_HR_cold_subts;						//[MWt]
        m_E_dot_HR_hot_fullts += m_E_dot_HR_hot_subts;							//[MWt]
        m_q_dot_htf_to_sink_fullts += m_q_dot_htf_to_sink_subts;				//[MWt]

        update_last_temps();
    }

    // Now, calculate average value over all subtimesteps
    double nd_steps_recirc = (double)n_steps_recirc;
    m_T_sys_c_t_int_fullts /= nd_steps_recirc;			//[K]
    m_T_htf_c_rec_in_t_int_fullts /= nd_steps_recirc;	//[K]
    m_T_htf_h_rec_out_t_int_fullts /= nd_steps_recirc;	//[K]
    m_T_sys_h_t_int_fullts /= nd_steps_recirc;			//[K]

    m_q_dot_sca_loss_summed_fullts /= nd_steps_recirc;			//[MWt]
    m_q_dot_sca_abs_summed_fullts /= nd_steps_recirc;			//[MWt]
    m_q_dot_sca_refl_summed_fullts /= nd_steps_recirc;          //[MWt]
    m_q_dot_xover_loss_summed_fullts /= nd_steps_recirc;		//[MWt]
    m_q_dot_HR_cold_loss_fullts /= nd_steps_recirc;				//[MWt]
    m_q_dot_HR_hot_loss_fullts /= nd_steps_recirc;				//[MWt]
    m_E_dot_sca_summed_fullts /= nd_steps_recirc;				//[MWt]
    m_E_dot_xover_summed_fullts /= nd_steps_recirc;				//[MWt]
    m_E_dot_HR_cold_fullts /= nd_steps_recirc;					//[MWt]
    m_E_dot_HR_hot_fullts /= nd_steps_recirc;					//[MWt]
    m_q_dot_htf_to_sink_fullts /= nd_steps_recirc;				//[MWt]

    m_q_dot_freeze_protection = Q_fp_sum / sim_info.ms_ts.m_step;	//[MWt]

    // Solve for pressure drop and pumping power
    m_dP_total = field_pressure_drop(weather.m_tdry, this->m_m_dot_htf_tot, this->m_P_field_in, this->m_T_htf_in_t_int, this->m_T_htf_out_t_int);

    // Are any of these required by the solver for system-level iteration?
    cr_out_solver.m_q_startup = 0.0;						//[MWt-hr] Receiver thermal output used to warm up the receiver
    cr_out_solver.m_time_required_su = sim_info.ms_ts.m_step;					//[s] Time required for receiver to startup - at least the entire timestep because it's off

    // 5.8.17, twn: Don't report a component *delivered* mass flow rate if fresnel is recirculating...
    //              .... and not passing HTF to other components
    //cr_out_solver.m_m_dot_salt_tot = m_dot_htf_loop*3600.0*(double)m_nLoops;	//[kg/hr] Total HTF mass flow rate
    cr_out_solver.m_m_dot_salt_tot = 0.0;	//[kg/hr] Total HTF mass flow rate

    m_q_dot_thermal_reported = 0.0;
    cr_out_solver.m_q_thermal = m_q_dot_thermal_reported;				//[MWt] No available receiver thermal output
    // 7.12.16: Return timestep-end or timestep-integrated-average?
    // If multiple recirculation steps, then need to calculate average of timestep-integrated-average
    cr_out_solver.m_T_salt_hot = m_T_sys_h_t_int_fullts - 273.15;		//[C]
    cr_out_solver.m_component_defocus = 1.0;
    cr_out_solver.m_is_recirculating = m_is_m_dot_recirc;

    cr_out_solver.m_W_dot_elec_in_tot = m_W_dot_sca_tracking + m_W_dot_pump;    //[MWe]
    cr_out_solver.m_q_dot_heater = m_q_dot_freeze_protection;   //[MWt]

    m_operating_mode = C_csp_collector_receiver::OFF;

    m_time_at_off = sim_info.ms_ts.m_step;
    m_time_at_startup = 0.0;
    m_time_at_on = 0.0;

    set_output_value();

    return;
}

void C_csp_fresnel_collector_receiver::startup(const C_csp_weatherreader::S_outputs& weather,
    const C_csp_solver_htf_1state& htf_state_in,
    C_csp_collector_receiver::S_csp_cr_out_solver& cr_out_solver,
    const C_csp_solver_sim_info& sim_info)
{

    // Always reset last temps
    reset_last_temps();

    m_is_m_dot_recirc = true;

    // Get optical performance
    loop_optical_eta(weather, sim_info);

    // Set mass flow rate to what I imagine might be an appropriate value
    double m_dot_htf_loop = m_m_dot_htfmin;
    if (weather.m_beam > 50.0 && m_T_htf_out_t_end_converged[m_nMod - 1] > (0.5 * m_T_fp + 0.5 * m_T_startup))
    {
        double m_dot_ss = (weather.m_beam / m_I_bn_des) * m_m_dot_loop_des;		//[kg/s]
        m_dot_htf_loop = min(m_m_dot_htfmax, max(m_m_dot_htfmin, 0.8 * m_dot_ss + 0.2 * m_m_dot_htfmin));		//[kg/s]
    }

    // Set duration for recirculation timestep
    if (m_step_recirc != m_step_recirc)
        m_step_recirc = 10.0 * 60.0;	//[s]

    // Calculate number of steps required given timestep from solver and recirculation step
    int n_steps_recirc = (int)std::ceil(sim_info.ms_ts.m_step / m_step_recirc);	//[-] Number of recirculation steps required

    // Define a copy of the sim_info structure
    double time_start = sim_info.ms_ts.m_time - sim_info.ms_ts.m_step;	//[s]
    double time_end = sim_info.ms_ts.m_time;            //[s]
    C_csp_solver_sim_info sim_info_temp = sim_info;

    bool is_T_startup_achieved = false;

    // This code finds the first "Recirculation Step" when the outlet temperature is greater than the Startup Temperature
    double time_required_su = sim_info.ms_ts.m_step;		//[s]

    double Q_fp_sum = 0.0;				//[MJ]

    // Zero full timestep outputs
    m_T_sys_c_t_int_fullts = m_T_htf_c_rec_in_t_int_fullts =
        m_T_htf_h_rec_out_t_int_fullts = m_T_sys_h_t_int_fullts = 0.0;	//[K]

    // Zero full timestep outputs
    m_q_dot_sca_loss_summed_fullts = m_q_dot_sca_abs_summed_fullts =
        m_q_dot_sca_refl_summed_fullts = m_q_dot_xover_loss_summed_fullts =
        m_q_dot_HR_cold_loss_fullts = m_q_dot_HR_hot_loss_fullts =
        m_E_dot_sca_summed_fullts = m_E_dot_xover_summed_fullts =
        m_E_dot_HR_cold_fullts = m_E_dot_HR_hot_fullts =
        m_q_dot_htf_to_sink_fullts = m_q_dot_thermal_reported = 0.0;

    sim_info_temp.ms_ts.m_time = time_start;

    // Loop through time steps
    while (sim_info_temp.ms_ts.m_time < time_end)
    {
        sim_info_temp.ms_ts.m_time_start = sim_info_temp.ms_ts.m_time;      //[s]
        sim_info_temp.ms_ts.m_time = std::min(sim_info_temp.ms_ts.m_time_start + m_step_recirc, time_end);  //[s]
        sim_info_temp.ms_ts.m_step = sim_info_temp.ms_ts.m_time - sim_info_temp.ms_ts.m_time_start;     //[s]

        // Set inlet temperature to previous timestep outlet temperature
        double T_cold_in = m_T_sys_h_t_end_last;			//[K]

        // Call energy balance with updated info
        loop_energy_balance_T_t_int(weather, T_cold_in, m_dot_htf_loop, sim_info_temp);

        // Check freeze protection
        if (m_T_htf_out_t_end[m_nMod - 1] < m_T_fp + fp_offset)
        {
            if (m_Q_field_losses_total_subts > 0.0)
            {
                double Q_fp_i = std::numeric_limits<double>::quiet_NaN();
                double T_cold_in_i = T_cold_in;
                int fp_code = freeze_protection(weather, T_cold_in_i, m_dot_htf_loop, sim_info_temp, Q_fp_i);

                T_cold_in = T_cold_in_i;	//[K]
                Q_fp_sum += Q_fp_i;			//[MJ]
            }
        }

        // Add current temperatures
        m_T_sys_c_t_int_fullts += T_cold_in * sim_info_temp.ms_ts.m_step;					//[K]
        m_T_htf_c_rec_in_t_int_fullts += m_T_htf_in_t_int[0] * sim_info_temp.ms_ts.m_step;	//[K]
        m_T_htf_h_rec_out_t_int_fullts += m_T_htf_out_t_int[m_nMod - 1] * sim_info_temp.ms_ts.m_step;	//[K]
        m_T_sys_h_t_int_fullts += m_T_sys_h_t_int * sim_info_temp.ms_ts.m_step;				//[K]

        // Add subtimestep calcs
        m_q_dot_sca_loss_summed_fullts += m_q_dot_sca_loss_summed_subts * sim_info_temp.ms_ts.m_step;		//[MWt]
        m_q_dot_sca_abs_summed_fullts += m_q_dot_sca_abs_summed_subts * sim_info_temp.ms_ts.m_step;			//[MWt]
        m_q_dot_sca_refl_summed_fullts += m_q_dot_sca_refl_summed_subts * sim_info_temp.ms_ts.m_step;       //[MWt]
        m_q_dot_xover_loss_summed_fullts += m_q_dot_xover_loss_summed_subts * sim_info_temp.ms_ts.m_step;	//[MWt]
        m_q_dot_HR_cold_loss_fullts += m_q_dot_HR_cold_loss_subts * sim_info_temp.ms_ts.m_step;				//[MWt]
        m_q_dot_HR_hot_loss_fullts += m_q_dot_HR_hot_loss_subts * sim_info_temp.ms_ts.m_step;				//[MWt]
        m_E_dot_sca_summed_fullts += m_E_dot_sca_summed_subts * sim_info_temp.ms_ts.m_step;					//[MWt]
        m_E_dot_xover_summed_fullts += m_E_dot_xover_summed_subts * sim_info_temp.ms_ts.m_step;				//[MWt]
        m_E_dot_HR_cold_fullts += m_E_dot_HR_cold_subts * sim_info_temp.ms_ts.m_step;						//[MWt]
        m_E_dot_HR_hot_fullts += m_E_dot_HR_hot_subts * sim_info_temp.ms_ts.m_step;							//[MWt]
        m_q_dot_htf_to_sink_fullts += m_q_dot_htf_to_sink_subts * sim_info_temp.ms_ts.m_step;

        // If the *outlet temperature at the end of the timestep* is greater than startup temperature,
        if (m_T_sys_h_t_end > m_T_startup)
        {
            time_required_su = sim_info_temp.ms_ts.m_time - time_start;		//[s]
            m_operating_mode = C_csp_collector_receiver::ON;				//[-]
            is_T_startup_achieved = true;
            break;
        }

        update_last_temps();

    }

    // Check if startup is achieved in current controller/kernel timestep
    if (!is_T_startup_achieved)
    {
        time_required_su = sim_info.ms_ts.m_step;		//[s]
        m_operating_mode = C_csp_collector_receiver::STARTUP;	//[-]
    }

    // Account for time required
    {
        m_T_sys_c_t_int_fullts /= time_required_su;			//[K]
        m_T_htf_c_rec_in_t_int_fullts /= time_required_su;	//[K]
        m_T_htf_h_rec_out_t_int_fullts /= time_required_su;	//[K]
        m_T_sys_h_t_int_fullts /= time_required_su;			//[K]

        m_q_dot_sca_loss_summed_fullts /= time_required_su;			//[MWt]
        m_q_dot_sca_abs_summed_fullts /= time_required_su;			//[MWt]
        m_q_dot_sca_refl_summed_fullts /= time_required_su;         //[MWt]
        m_q_dot_xover_loss_summed_fullts /= time_required_su;		//[MWt]
        m_q_dot_HR_cold_loss_fullts /= time_required_su;			//[MWt]
        m_q_dot_HR_hot_loss_fullts /= time_required_su;				//[MWt]
        m_E_dot_sca_summed_fullts /= time_required_su;				//[MWt]
        m_E_dot_xover_summed_fullts /= time_required_su;			//[MWt]
        m_E_dot_HR_cold_fullts /= time_required_su;					//[MWt]
        m_E_dot_HR_hot_fullts /= time_required_su;					//[MWt]
        m_q_dot_htf_to_sink_fullts /= time_required_su;				//[MWt]

        m_q_dot_freeze_protection = Q_fp_sum / time_required_su;	//[MWt]
    }
    
    double Q_dot_balance_subts = m_q_dot_sca_abs_summed_fullts - m_q_dot_xover_loss_summed_fullts -
        m_q_dot_HR_cold_loss_fullts - m_q_dot_HR_hot_loss_fullts -
        m_E_dot_sca_summed_fullts - m_E_dot_xover_summed_fullts -
        m_E_dot_HR_cold_fullts - m_E_dot_HR_hot_fullts - m_q_dot_htf_to_sink_fullts;	//[MWt]

    // Solve for pressure drop and pumping power
    m_dP_total = field_pressure_drop(weather.m_tdry, this->m_m_dot_htf_tot, this->m_P_field_in, this->m_T_htf_in_t_int, this->m_T_htf_out_t_int);

    // These outputs need some more thought
        // For now, just set this > 0.0 so that the controller knows that startup was successful
    cr_out_solver.m_q_startup = 1.0;						//[MWt-hr] Receiver thermal output used to warm up the receiver
    // Startup time is calculated here
    cr_out_solver.m_time_required_su = time_required_su;	//[s]
    // 	Need to be sure this value is correct..., but controller doesn't use it in CR_SU (confirmed)

    // 5.8.17, twn: Don't report a component *delivered* mass flow rate if fresnel is recirculating...
    //              .... and not passing HTF to other components
    //cr_out_solver.m_m_dot_salt_tot = m_m_dot_htf_tot*3600.0;	//[kg/hr] Total HTF mass flow rate
    cr_out_solver.m_m_dot_salt_tot = 0.0;	//[kg/hr]

    // Should not be available thermal output if receiver is in start up, but controller doesn't use it in CR_SU (confirmed)
    m_q_dot_thermal_reported = 0.0;
    cr_out_solver.m_q_thermal = m_q_dot_thermal_reported;				//[MWt] No available receiver thermal output
    // 7.12.16: Return timestep-end or timestep-integrated-average?
    // If multiple recirculation steps, then need to calculate average of timestep-integrated-average
    cr_out_solver.m_T_salt_hot = m_T_sys_h_t_int_fullts - 273.15;		//[C]

    cr_out_solver.m_component_defocus = 1.0;	//[-]
    cr_out_solver.m_is_recirculating = m_is_m_dot_recirc;

    cr_out_solver.m_W_dot_elec_in_tot = m_W_dot_sca_tracking + m_W_dot_pump;    //[MWe]
    // Shouldn't need freeze protection if in startup, but may want a check on this
    cr_out_solver.m_q_dot_heater = m_q_dot_freeze_protection;    //[MWt]

    m_time_at_off = 0.0;
    m_time_at_startup = sim_info.ms_ts.m_step;
    m_time_at_on = 0.0;

    set_output_value();

    return;
}

void C_csp_fresnel_collector_receiver::on(const C_csp_weatherreader::S_outputs& weather,
    const C_csp_solver_htf_1state& htf_state_in,
    double q_dot_elec_to_CR_heat /*MWt*/, double field_control,
    C_csp_collector_receiver::S_csp_cr_out_solver& cr_out_solver,
    //C_csp_collector_receiver::S_csp_cr_out_report &cr_out_report,
    const C_csp_solver_sim_info& sim_info)
{

    // Always reset last temps
    reset_last_temps();

    m_is_m_dot_recirc = false;

    // Get optical performance (no defocus applied in this method)
        // This returns m_q_SCA with NO defocus
    loop_optical_eta(weather, sim_info);

    // If Control Defocus: field_control < 1, then apply it here
    if (field_control < 1.0)
    {
        // 1) Calculate m_q_sca_control_df
        apply_control_defocus(field_control);

        // 2) Set m_q_sca = m_q_sca_control_df
            // m_q_sca_control_df will be baseline for component defocusing downstream in this method
            // While loop_energy_balance uses m_q_sca
        m_q_SCA = m_q_SCA_control_df;
    }
    else if (field_control == 1.0)
    {
        // If no CONTROL defocus, then baseline against the vector returned by 'loop_optical_eta'
        m_q_SCA_control_df = m_q_SCA;
    }
    else
    {
        throw(C_csp_exception("C_csp_fresnel_collector::on(...) received a CONTROL defocus > 1.0, "
            "and that is not ok!"));
    }

    // Solve the loop energy balance at the minimum mass flow rate
        // Set mass flow rate to minimum allowable
    double m_dot_htf_loop = m_m_dot_htfmin;		//[kg/s]
    // Get inlet condition from input argument
    double T_cold_in = htf_state_in.m_temp + 273.15;	//[K]
    // Call energy balance with updated info
    E_loop_energy_balance_exit balance_code = loop_energy_balance_T_t_int(weather, T_cold_in, m_dot_htf_loop, sim_info);

    bool on_success = true;

    if (balance_code != E_loop_energy_balance_exit::SOLVED)
    {
        on_success = false;
    }

    // If the outlet temperature (of last SCA!) is greater than the target (considering some convergence tolerance)
        // then adjust mass flow rate and see what happens
    if ((m_T_htf_out_t_end[m_nMod - 1] - m_T_loop_out_des) / m_T_loop_out_des > 0.001 && on_success)
    {
        // Try the maximum mass flow rate
        m_dot_htf_loop = m_m_dot_htfmax;		//[kg/s]

        // We set T_cold_in above, so call loop energy balance
        loop_energy_balance_T_t_int(weather, T_cold_in, m_dot_htf_loop, sim_info);

        // Is the outlet temperature (of the last SCA!) still greater than the target (considering some convergence tolerance)
            // then need to defocus
        if ((m_T_htf_out_t_end[m_nMod - 1] - m_T_loop_out_des) / m_T_loop_out_des > 0.001)
        {
            // The Monotonic Solver will iterate on defocus that achieves the target outlet temperature
            //     at the maximum HTF mass flow rate
            C_mono_eq_defocus c_defocus_function(this, weather, T_cold_in, m_dot_htf_loop, sim_info);
            C_monotonic_eq_solver c_defocus_solver(c_defocus_function);

            // Set upper and lower bounds
            double defocus_upper = 1.0;		//[-]
            double defocus_lower = 0.0;		//[-]

            // Set guess values... can be smarter about this...
            double defocus_guess_upper = min(1.0, (m_T_loop_out_des - m_T_loop_in_des) / (m_T_htf_out_t_end[m_nMod - 1] - m_T_loop_in_des));
            double defocus_guess_lower = 0.9 * defocus_guess_upper;	//[-]

            // Set solver settings - relative error on T_htf_out
            c_defocus_solver.settings(0.001, 30, defocus_lower, defocus_upper, true);

            int iter_solved = -1;
            double tol_solved = std::numeric_limits<double>::quiet_NaN();

            int defocus_code = 0;
            double defocus_solved = 1.0;
            try
            {
                defocus_code = c_defocus_solver.solve(defocus_guess_lower, defocus_guess_upper, m_T_loop_out_des,
                    defocus_solved, tol_solved, iter_solved);
            }
            catch (C_csp_exception)
            {
                throw(C_csp_exception("C_csp_fresnel_collector::on(...) COMPONENT defocus failed."));
                on_success = false;
            }

            if (defocus_code != C_monotonic_eq_solver::CONVERGED)
            {
                throw(C_csp_exception("C_csp_fresnel_collector::on(...) COMPONENT defocus failed."));
                on_success = false;
            }

        }
        else
        {
            // Apply 1 var solver to find the mass flow rate that achieves the target outlet temperature
            C_mono_eq_T_htf_loop_out c_T_htf_out_calc(this, weather, T_cold_in, sim_info);
            C_monotonic_eq_solver c_htf_m_dot_solver(c_T_htf_out_calc);

            // Set upper and lower bounds
            double m_dot_upper = m_m_dot_htfmax;	//[kg/s]
            double m_dot_lower = m_m_dot_htfmin;	//[kg/s]

            // Set guess values... can be smarter about this...
            double m_dot_guess_upper = 0.75 * m_m_dot_htfmax + 0.25 * m_m_dot_htfmin;	//[kg/s]
            double m_dot_guess_lower = 0.25 * m_m_dot_htfmax + 0.75 * m_m_dot_htfmin;	//[kg/s]

            // Set solver settings
            // Relative error
            c_htf_m_dot_solver.settings(0.001, 30, m_dot_lower, m_dot_upper, true);

            int iter_solved = -1;
            double tol_solved = std::numeric_limits<double>::quiet_NaN();

            int m_dot_htf_code = 0;
            try
            {
                m_dot_htf_code = c_htf_m_dot_solver.solve(m_dot_guess_lower, m_dot_guess_upper, m_T_loop_out_des,
                    m_dot_htf_loop, tol_solved, iter_solved);
            }
            catch (C_csp_exception)
            {
                throw(C_csp_exception("C_csp_fresnel_collector::on(...) HTF mass flow rate iteration failed."));
                on_success = false;
            }

            if (m_dot_htf_code != C_monotonic_eq_solver::CONVERGED)
            {
                throw(C_csp_exception("C_csp_fresnel_collector::on(...) HTF mass flow rate iteration failed."));
                on_success = false;
            }
        }

    }

    if (on_success)
    {
        m_T_sys_c_t_int_fullts = T_cold_in;								//[K]
        m_T_htf_c_rec_in_t_int_fullts = m_T_htf_in_t_int[0];			//[K]
        m_T_htf_h_rec_out_t_int_fullts = m_T_htf_out_t_int[m_nMod - 1];	//[K]
        m_T_sys_h_t_int_fullts = m_T_sys_h_t_int;						//[K]

        m_q_dot_sca_loss_summed_fullts = m_q_dot_sca_loss_summed_subts;		//[MWt]
        m_q_dot_sca_abs_summed_fullts = m_q_dot_sca_abs_summed_subts;		//[MWt]
        m_q_dot_sca_refl_summed_fullts = m_q_dot_sca_refl_summed_subts;     //[MWt]
        m_q_dot_xover_loss_summed_fullts = m_q_dot_xover_loss_summed_subts;	//[MWt]
        m_q_dot_HR_cold_loss_fullts = m_q_dot_HR_cold_loss_subts;			//[MWt]
        m_q_dot_HR_hot_loss_fullts = m_q_dot_HR_hot_loss_subts;				//[MWt]
        m_E_dot_sca_summed_fullts = m_E_dot_sca_summed_subts;				//[MWt]
        m_E_dot_xover_summed_fullts = m_E_dot_xover_summed_subts;			//[MWt]
        m_E_dot_HR_cold_fullts = m_E_dot_HR_cold_subts;						//[MWt]
        m_E_dot_HR_hot_fullts = m_E_dot_HR_hot_subts;						//[MWt]
        m_q_dot_htf_to_sink_fullts = m_q_dot_htf_to_sink_subts;				//[MWt]
        m_q_dot_freeze_protection = 0.0;									//[MWt]

        double Q_dot_balance_subts = m_q_dot_sca_abs_summed_fullts - m_q_dot_xover_loss_summed_fullts -
            m_q_dot_HR_cold_loss_fullts - m_q_dot_HR_hot_loss_fullts -
            m_E_dot_sca_summed_fullts - m_E_dot_xover_summed_fullts -
            m_E_dot_HR_cold_fullts - m_E_dot_HR_hot_fullts - m_q_dot_htf_to_sink_fullts;	//[MWt]

        // Solve for pressure drop and pumping power
        m_dP_total = field_pressure_drop(weather.m_tdry, this->m_m_dot_htf_tot, this->m_P_field_in, this->m_T_htf_in_t_int, this->m_T_htf_out_t_int);

        // Set solver outputs & return
        // Receiver is already on, so the controller is not looking for this value
        cr_out_solver.m_q_startup = 0.0;		//[MWt-hr] 
        // Receiver is already on, so the controller is not looking for the required startup time
        cr_out_solver.m_time_required_su = 0.0;	//[s]
        // The controller requires the total mass flow rate from the collector-receiver
            // This value is set in the most recent call to the loop energy balance
        cr_out_solver.m_m_dot_salt_tot = m_m_dot_htf_tot * 3600.0;	//[kg/hr]

        // The controller also requires the receiver thermal output
        // 7.12.16 Now using the timestep-integrated-average temperature
        double c_htf_ave = m_htfProps.Cp((m_T_sys_h_t_int + T_cold_in) / 2.0);  //[kJ/kg-K]
        m_q_dot_thermal_reported = (cr_out_solver.m_m_dot_salt_tot / 3600.0) * c_htf_ave * (m_T_sys_h_t_int - T_cold_in) / 1.E3;	//[MWt]
        cr_out_solver.m_q_thermal = m_q_dot_thermal_reported;

        // Finally, the controller need the HTF outlet temperature from the field
        cr_out_solver.m_T_salt_hot = m_T_sys_h_t_int - 273.15;		//[C]

        cr_out_solver.m_component_defocus = m_component_defocus;	//[-]
        cr_out_solver.m_is_recirculating = m_is_m_dot_recirc;
        // ***********************************************************
        // ***********************************************************

        // For now, set parasitic outputs to 0
        cr_out_solver.m_W_dot_elec_in_tot = m_W_dot_sca_tracking + m_W_dot_pump;    //[MWe]
        cr_out_solver.m_dP_sf = m_dP_total;         //[bar]
        cr_out_solver.m_q_dot_heater = m_q_dot_freeze_protection;    //[MWt]
    }
    else
    {	// Solution failed, so tell controller/solver

        m_T_sys_c_t_int_fullts = 0.0;			//[K]
        m_T_htf_c_rec_in_t_int_fullts = 0.0;	//[K]
        m_T_htf_h_rec_out_t_int_fullts = 0.0;	//[K]
        m_T_sys_h_t_int_fullts = 0.0;			//[K]

        m_q_dot_sca_loss_summed_fullts = m_q_dot_sca_abs_summed_fullts =
            m_q_dot_sca_loss_summed_fullts = m_q_dot_xover_loss_summed_fullts =
            m_q_dot_HR_cold_loss_fullts = m_q_dot_HR_hot_loss_fullts =
            m_E_dot_sca_summed_fullts = m_E_dot_xover_summed_fullts =
            m_E_dot_HR_cold_fullts = m_E_dot_HR_hot_fullts =
            m_q_dot_htf_to_sink_fullts = m_q_dot_freeze_protection = m_q_dot_thermal_reported = 0.0;

        cr_out_solver.m_q_startup = 0.0;			//[MWt-hr]
        cr_out_solver.m_time_required_su = 0.0;		//[s]
        cr_out_solver.m_m_dot_salt_tot = 0.0;		//[kg/hr]
        cr_out_solver.m_q_thermal = m_q_dot_thermal_reported;			//[MWt]
        cr_out_solver.m_T_salt_hot = 0.0;			//[C]
        cr_out_solver.m_component_defocus = 1.0;	//[-]
        cr_out_solver.m_is_recirculating = false;
        m_W_dot_sca_tracking = 0.0;
        m_W_dot_pump = 0.0;
        cr_out_solver.m_W_dot_elec_in_tot = m_W_dot_sca_tracking + m_W_dot_pump;    //[MWe]
        cr_out_solver.m_dP_sf = 0.0;                //[bar]

        cr_out_solver.m_q_dot_heater = m_q_dot_freeze_protection;    //[MWt]
    }

    m_time_at_off = 0.0;
    m_time_at_startup = 0.0;
    m_time_at_on = sim_info.ms_ts.m_step;

    set_output_value();

    return;
}

void C_csp_fresnel_collector_receiver::steady_state(const C_csp_weatherreader::S_outputs& weather,
    const C_csp_solver_htf_1state& htf_state_in,
    double W_dot_elec_to_CR_heat /*MWe*/, double field_control,
    C_csp_collector_receiver::S_csp_cr_out_solver& cr_out_solver,
    const C_csp_solver_sim_info& sim_info)
{
    // Original converged values to reset back to
    double T_sys_c_t_end_converged_orig = m_T_sys_c_t_end_converged;
    double T_sys_h_t_end_converged_orig = m_T_sys_h_t_end_converged;
    std::vector<double> T_htf_out_t_end_converged_orig = m_T_htf_out_t_end_converged;

    m_T_sys_c_t_end_converged = htf_state_in.m_temp + 273.15;       // this sets m_T_sys_c_t_end_last
    m_T_sys_h_t_end_converged = htf_state_in.m_temp + 273.15;       // this sets m_T_sys_h_t_end_last
    m_T_htf_out_t_end_converged.assign(m_nMod, htf_state_in.m_temp + 273.15);

    // Values for checking whether steady-state
    double ss_diff = std::numeric_limits<double>::quiet_NaN();
    const double tol = 0.05;
    std::vector<double> T_htf_in_t_int_last = m_T_htf_in_t_int;
    std::vector<double> T_htf_out_t_int_last = m_T_htf_out_t_int;
    double minutes2SS = 0.;
    int count = 0;
    int max_iterations = 50;

    do
    {
        this->on(weather, htf_state_in, W_dot_elec_to_CR_heat, field_control, cr_out_solver, sim_info);

        // Calculate metric for deciding whether steady-state is reached
        ss_diff = 0.;
        for (int i = 0; i < m_nMod; i++) {
            ss_diff += std::abs(m_T_htf_in_t_int[i] - T_htf_in_t_int_last[i]) +
                std::abs(m_T_htf_out_t_int[i] - T_htf_out_t_int_last[i]);
        }

        // Set converged values so reset_last_temps() propagates the temps in time
        m_T_sys_c_t_end_converged = m_T_sys_c_t_end;
        m_T_sys_h_t_end_converged = m_T_sys_h_t_end;
        m_T_htf_out_t_end_converged = m_T_htf_out_t_end;

        // Update 'last' values
        T_htf_in_t_int_last = m_T_htf_in_t_int;
        T_htf_out_t_int_last = m_T_htf_out_t_int;

        minutes2SS += sim_info.ms_ts.m_step / 60.;

        
        count++;
    } while (ss_diff / 200. > tol && count < max_iterations);

    if (count == max_iterations)
    {
        int x = 0;
    }

    // Re-run runner and header pipe sizing using the same diameters to get the actual mass flows and velocities at steady state
    double m_dot_ss = cr_out_solver.m_m_dot_salt_tot / 3600.;                   // [kg/s]
    double rho_cold = m_htfProps.dens(T_htf_in_t_int_last[0], 10.e5);           // [kg/m3]
    double rho_hot = m_htfProps.dens(T_htf_out_t_int_last[m_nMod - 1], 10.e5);  // [kg/m3]
    std::string summary;

    // Set steady-state outputs
    transform(m_T_rnr.begin(), m_T_rnr.end(), m_T_rnr_dsn.begin(), [](double x) {return x - 273.15; });        // K to C
    transform(m_P_rnr.begin(), m_P_rnr.end(), m_P_rnr_dsn.begin(), [](double x) {return x / 1.e5; });          // Pa to bar
    transform(m_T_hdr.begin(), m_T_hdr.end(), m_T_hdr_dsn.begin(), [](double x) {return x - 273.15; });        // K to C
    //transform(m_P_hdr.begin(), m_P_hdr.end(), m_P_hdr_dsn.begin(), [](double x) {return x / 1.e5; });          // Pa to bar
    transform(m_T_loop.begin(), m_T_loop.end(), m_T_loop_dsn.begin(), [](double x) {return x - 273.15; });     // K to C
    //transform(m_P_loop.begin(), m_P_loop.end(), m_P_loop_dsn.begin(), [](double x) {return x / 1.e5; });       // Pa to bar

    // After steady-state is calculated, reset back to original converged values
    m_T_sys_c_t_end_converged = T_sys_c_t_end_converged_orig;
    m_T_sys_h_t_end_converged = T_sys_h_t_end_converged_orig;
    m_T_htf_out_t_end_converged = T_htf_out_t_end_converged_orig;

    return;
}


void C_csp_fresnel_collector_receiver::estimates(const C_csp_weatherreader::S_outputs& weather,
    const C_csp_solver_htf_1state& htf_state_in,
    C_csp_collector_receiver::S_csp_cr_est_out& est_out,
    const C_csp_solver_sim_info& sim_info)
{
    if (m_operating_mode == C_csp_collector_receiver::ON)
    {
        C_csp_collector_receiver::S_csp_cr_out_solver cr_out_solver;

        on(weather, htf_state_in, std::numeric_limits<double>::quiet_NaN(), 1.0, cr_out_solver, sim_info);

        est_out.m_q_dot_avail = cr_out_solver.m_q_thermal;		//[MWt]
        est_out.m_m_dot_avail = cr_out_solver.m_m_dot_salt_tot;	//[kg/hr]
        est_out.m_T_htf_hot = cr_out_solver.m_T_salt_hot;		//[C]
        est_out.m_q_startup_avail = 0.0;	//[MWt]
    }
    else
    {
        if (weather.m_beam > 1.0)
        {
            est_out.m_q_startup_avail = 1.0;	//[MWt] fresnel is recirculating, so going into startup isn't significantly different than OFF
        }
        else
        {
            est_out.m_q_startup_avail = 0.0;
        }
        est_out.m_q_dot_avail = 0.0;
        est_out.m_m_dot_avail = 0.0;
        est_out.m_T_htf_hot = 0.0;
    }

    return;
}

void C_csp_fresnel_collector_receiver::converged()
{
    /*
    -- Post-convergence call --

    Update values that should be transferred to the next time step
    */

    // Check that, if fresnel is ON, if outlet temperature at the end of the timestep is colder than the Startup Temperature
    if (m_operating_mode == ON && m_T_sys_h_t_end < m_T_startup)
    {
        m_operating_mode = OFF;
    }

    // CSP Solver Temperature Tracking
    m_T_sys_c_t_end_converged = m_T_sys_c_t_end_last = m_T_sys_c_t_end;		//[K]
    m_T_sys_h_t_end_converged = m_T_sys_h_t_end_last = m_T_sys_h_t_end;		//[K]
    for (int i = 0; i < m_nMod; i++)
    {
        m_T_htf_out_t_end_converged[i] = m_T_htf_out_t_end_last[i] = m_T_htf_out_t_end[i];	//[K]
    }

    if (m_operating_mode == C_csp_collector_receiver::STEADY_STATE)
    {
        throw(C_csp_exception("Receiver should only be run at STEADY STATE mode for estimating output. It must be run at a different mode before exiting a timestep",
            "fresnel converged method"));
    }

    m_operating_mode_converged = m_operating_mode;	//[-]

    // Always reset the m_defocus control at the first call of a timestep
    //m_defocus_new = 1.0;	//[-]
    //m_defocus_old = 1.0;	//[-]
    //m_defocus = 1.0;		//[-]

    m_W_dot_sca_tracking = 0.0;		//[MWe]

    // Reset the optical efficiency member data
    loop_optical_eta_off();

    mc_reported_outputs.set_timestep_outputs();

    return;
}

void C_csp_fresnel_collector_receiver::write_output_intervals(double report_time_start,
    const std::vector<double>& v_temp_ts_time_end, double report_time_end)
{
    mc_reported_outputs.send_to_reporting_ts_array(report_time_start,
        v_temp_ts_time_end, report_time_end);
}

double C_csp_fresnel_collector_receiver::calculate_optical_efficiency(const C_csp_weatherreader::S_outputs& weather, const C_csp_solver_sim_info& sim)
{
    // loop_optical_eta() has side-effects. Store affected member variable values for restore after call.
    double m_q_i_ini(m_q_i);
    util::matrix_t<double> m_ColOptEff_ini(m_ColOptEff);
    double m_EqOpteff_ini(m_EqOpteff);
    std::vector<double> m_q_SCA_ini(m_q_SCA);
    double m_W_dot_sca_tracking_ini(m_W_dot_sca_tracking);
    double m_control_defocus_ini(m_control_defocus);
    double m_component_defocus_ini(m_component_defocus);
    double m_q_dot_inc_sf_tot_ini(m_q_dot_inc_sf_tot);

    loop_optical_eta(weather, sim);
    double eta_optical = m_eta_optical;     // m_EqOpteff;

    // Restore member variable values
    m_q_i = m_q_i_ini;
    m_ColOptEff = m_ColOptEff_ini;
    m_EqOpteff = m_EqOpteff_ini;
    m_q_SCA = m_q_SCA_ini;
    m_W_dot_sca_tracking = m_W_dot_sca_tracking_ini;
    m_control_defocus = m_control_defocus_ini;
    m_component_defocus = m_component_defocus_ini;
    m_q_dot_inc_sf_tot = m_q_dot_inc_sf_tot_ini;

    return eta_optical;
}

double C_csp_fresnel_collector_receiver::calculate_thermal_efficiency_approx(const C_csp_weatherreader::S_outputs& weather, double q_incident /*MW*/, const C_csp_solver_sim_info& sim)
{
    // q_incident is the power incident (absorbed by the absorber) on all the HCE receivers, calculated using the DNI and optical efficiency
    if (q_incident <= 0) return 0.;

    // New Estimate (using steady state)
    double q_eff = 0;
    {
        C_csp_solver_htf_1state htfInletState;
        htfInletState.m_temp = m_T_loop_in_des - 273.15;
        double defocus = 1;
        //C_csp_solver_sim_info fresnelInfo;
        //fresnelInfo.ms_ts.m_time_start = 14817600.;
        //fresnelInfo.ms_ts.m_step = 15. * 60.;               // 5-minute timesteps
        //fresnelInfo.ms_ts.m_time = fresnelInfo.ms_ts.m_time_start + fresnelInfo.ms_ts.m_step;
        //fresnelInfo.m_tou = 1.;

        C_csp_solver_sim_info fresnelInfo;
        fresnelInfo.ms_ts.m_time_start = sim.ms_ts.m_time_start;
        fresnelInfo.ms_ts.m_step = 15. * 60.;               // 5-minute timesteps
        fresnelInfo.ms_ts.m_time = sim.ms_ts.m_time;
        fresnelInfo.m_tou = 1.;

        C_csp_collector_receiver::S_csp_cr_out_solver fresnelOutputs;

        steady_state(weather, htfInletState, std::numeric_limits<double>::quiet_NaN(), defocus, fresnelOutputs, fresnelInfo);
        double q_thermal = fresnelOutputs.m_q_thermal * 1e6;  // [Wt]

        // Optical Efficiency
        double optical_eff = m_eta_optical;
        double given_optical_eff = (q_incident * 1e6) / (m_Ap_tot * weather.m_beam);
        double eff_now = this->calculate_optical_efficiency(weather, sim);

        // Thermal Efficiency
        double Q_available = m_Ap_tot * weather.m_beam * optical_eff;  // W
        q_eff = q_thermal / Q_available;

        if (q_incident == 0)
            q_eff = 0;

        else if (q_eff < 0)
            q_eff = 0;

        else if (q_eff > 1)
            q_eff = 1;
        

    }

    return q_eff;
}

double C_csp_fresnel_collector_receiver::get_collector_area()
{
    return m_Ap_tot;
}

// ------------------------------------------------------------------- PUBLIC SUPPLEMENTAL

bool C_csp_fresnel_collector_receiver::design_solar_mult(double latitude_deg)
{
    if (m_is_solar_mult_designed == true)
        return false;

    // Set up the optical table object..
    {
        /*
        The input should be defined as follows:
        - Data of size nx, ny
        - OpticalTable of size (nx+1)*(ny+1)
        - First nx+1 values (row 1) are x-axis values, not data, starting at index 1
        - First value of remaining ny rows are y-axis values, not data
        - Data is contained in cells i,j : where i>1, j>1
        */
        int ncol_OpticalTable = m_OpticalTable.ncols();
        int nrow_OpticalTable = m_OpticalTable.nrows();

        double* xax = new double[ncol_OpticalTable - 1];
        double* yax = new double[nrow_OpticalTable - 1];
        double* data = new double[(ncol_OpticalTable - 1) * (nrow_OpticalTable - 1)];

        //get the xaxis data values
        for (int i = 1; i < ncol_OpticalTable; i++) {
            xax[i - 1] = m_OpticalTable.at(0, i) * m_d2r;
        }
        //get the yaxis data values
        for (int j = 1; j < nrow_OpticalTable; j++) {
            yax[j - 1] = m_OpticalTable.at(j, 0) * m_d2r;
        }
        //Get the data values
        for (int j = 1; j < nrow_OpticalTable; j++) {
            for (int i = 1; i < ncol_OpticalTable; i++) {
                data[i - 1 + (ncol_OpticalTable - 1) * (j - 1)] = m_OpticalTable.at(j, i);
            }
        }

        optical_table.AddXAxis(xax, ncol_OpticalTable - 1);
        optical_table.AddYAxis(yax, nrow_OpticalTable - 1);
        optical_table.AddData(data);
        delete[] xax;
        delete[] yax;
        delete[] data;

    }

    // Calculate nLoops, depending on designing for solar mult or total field aperture
    {
        // Optical Derate (Receiver)
        m_opt_derate = 0;
        for (int i = 0; i < m_nRecVar; i++)
            // TMB 12-06-2023 Add receiver absorptivity and envelope transmissivity to optical derate
            //m_opt_derate += m_HCE_FieldFrac[i] * m_Shadowing[i] * m_dirt_env[i];
            m_opt_derate += m_HCE_FieldFrac[i] * m_Shadowing[i] * m_dirt_env[i] * m_alpha_abs[i] * m_Tau_envelope[i];

        // Account for solar position
        double position_derate = 1;
        if (latitude_deg != -377)
        {
            //design point solar elevation
            double lat_rad = latitude_deg * m_d2r;
            double elev_des = asin(sin(0.4092793) * sin(lat_rad) + cos(lat_rad) * cos(0.4092793));
            //translate the solar angles into incidence angles
            double phi_t, theta_L, iam_t, iam_l;
            double ColAz = m_ColAz * m_d2r; // TMB 12.7.2023 temporary dirty fix because unit conversions are in init (and that hasn't been called yet)
            CSP::theta_trans(0., CSP::pi / 2. - elev_des, ColAz, phi_t, theta_L);	//phi_t and theta_L are the translated angles (transverse and longitudinal)

            
            switch (m_opt_model)
            {
                case 1:		//Solar position table
                    position_derate = optical_table.interpolate(0., CSP::pi / 2. - elev_des);
                    break;
                case 2:		//Collector incidence table
                    position_derate = optical_table.interpolate(0., theta_L);
                    //optical_IAM = optical_table.interpolate(0., theta_L);
                    //m_opteff_des = eta_opt_fixed;
                    break;
                case 3:		//IAM polynomials
                {
                    iam_t = 0.;
                    iam_l = 0.;
                    int n_IAM_L_coefs = m_IAM_L_coefs.size();
                    int n_IAM_T_coefs = m_IAM_T_coefs.size();
                    for (int i = 0; i < n_IAM_L_coefs; i++)
                        iam_l += m_IAM_L_coefs[i] * pow(theta_L, i);
                    for (int i = 0; i < n_IAM_T_coefs; i++)
                        iam_t += m_IAM_T_coefs[i] * pow(phi_t, i);
                    position_derate = iam_t * iam_l;
                    break;
                }
                default:
                    //message(TCS_ERROR, "The selected optical model (%d) does not exist. Options are 1=Solar position table : 2=Collector incidence table : 3= IAM polynomials", opt_model);
                    string msg = "The selected optical model (%d) does not exist. Options are 1=Solar position table : 2=Collector incidence table : 3= IAM polynomials";
                    m_error_msg = util::format(msg.c_str(), m_opt_model);
                    mc_csp_messages.add_message(C_csp_messages::NOTICE, m_error_msg);
                    return false;
            }
        }
        

        // Optical Normal (Mirror/Collector)
        m_opt_normal = 0;
        // TMB 12.7.2023 Add position derate
        m_opt_normal = m_TrackingError * m_GeomEffects * m_reflectivity * m_Dirt_mirror * m_Error * position_derate;

        // Loop Optical Efficiency
        m_loop_opt_eff = m_opt_derate * m_opt_normal;

        // Loop Aperture
        m_A_loop = (float)m_nMod * m_A_aperture;

        // Heat Loss at Design
        m_hl_des = 0;
        m_dT_des = ((m_T_loop_in_des + m_T_loop_out_des) / 2.0) - (m_T_amb_sf_des + 273.15);            // Average temperature difference at design
        switch (m_rec_model)
        {
            // Polynomial
            case (1):
            {
                m_hl_des = CSP::poly_eval(m_dT_des, &m_HL_T_coefs[0], m_HL_T_coefs.size());
                break;
            }
            // Evacuated Receiver
            case (2):
            {
                for (int i = 0; i < m_nRecVar; i++)
                    m_hl_des += m_HCE_FieldFrac[i] * m_Design_loss[i];
                break;
            }
            default:
            {
                string msg = "The selected thermal model (%d) does not exist. Options are 1=Regression model : 2=Evacuated tube receiver model";
                m_error_msg = util::format(msg.c_str(), m_rec_model);
                mc_csp_messages.add_message(C_csp_messages::NOTICE, m_error_msg);
                return false;
            }
        }

        // Loop Thermal Efficiency
        m_loop_therm_eff = 1.0 - ((m_hl_des * m_L_mod * m_nMod) / (m_A_loop * m_I_bn_des * m_loop_opt_eff));

        // Loop Efficiency
        m_loop_eff = m_loop_opt_eff * m_loop_therm_eff;

        // Thermal Power at Design
        m_q_pb_design = m_P_ref / m_eta_ref; // 279

        // Required Aperture for solar multiple = 1
        m_Ap_sm1 = m_q_pb_design / (m_I_bn_des * m_loop_eff);

        // Calculate actual solar mult, total field aperture, and nLoops
        switch (m_solar_mult_or_Ap)
        {
            // Use Solar Multiple
            case 0:
            {
                m_solar_mult = m_solar_mult_in;
                m_Ap_tot = m_solar_mult * m_Ap_sm1;
                m_nLoops = std::ceil(m_Ap_tot / m_A_loop);

                // Get 'Actual' Ap_tot
                m_Ap_tot = m_nLoops * m_A_loop;
                m_solar_mult = m_Ap_tot / m_Ap_sm1;
                break;
            }
            case 1:
            {
                m_Ap_tot = m_total_Ap_in;
                m_nLoops = std::ceil(m_Ap_tot / m_A_loop);

                // Get 'Actual' total aperture
                m_Ap_tot = m_nLoops * m_A_loop;
                m_solar_mult = m_Ap_tot / m_Ap_sm1;
                break;
            }
            default:
            {
                string msg = "use_solar_mult_or_total_Ap integer should be 0 (solar mult) or 1 (field aperture)";
                mc_csp_messages.add_message(C_csp_messages::NOTICE, msg);
                return false;
            }
        }

        // Update m_q_design with actual aperture
        //m_q_design_actual = m_I_bn_des * m_Ap_tot * m_loop_eff;

        // Number of Loops necessary for solar mult = 1
        m_nLoops_sm1 = std::ceil(m_Ap_sm1 / m_A_loop);

        m_is_solar_mult_designed = true;

    }
    return true;
}

// ******************************************************************** Internal Class Methods

int C_csp_fresnel_collector_receiver::C_mono_eq_defocus::operator()(double defocus /*-*/, double* T_htf_loop_out /*K*/)
{
    // Apply the defocus to calculate a new m_q_SCA
    mpc_fresnel->apply_component_defocus(defocus);

    // Solve the loop energy balance at the input mass flow rate
    E_loop_energy_balance_exit exit_code = mpc_fresnel->loop_energy_balance_T_t_int(ms_weather, m_T_cold_in, m_m_dot_loop, ms_sim_info);

    if (exit_code != E_loop_energy_balance_exit::SOLVED)
    {
        *T_htf_loop_out = std::numeric_limits<double>::quiet_NaN();
        return -1;
    }

    // Set the outlet temperature at end of timestep
    *T_htf_loop_out = mpc_fresnel->m_T_htf_out_t_end[mpc_fresnel->m_nMod - 1];

    return 0;
}

int C_csp_fresnel_collector_receiver::C_mono_eq_T_htf_loop_out::operator()(double m_dot_htf_loop /*kg/s*/, double* T_htf_loop_out /*K*/)
{
    // Solve the loop energy balance at the input mass flow rate
    E_loop_energy_balance_exit exit_code = mpc_fresnel->loop_energy_balance_T_t_int(ms_weather, m_T_cold_in, m_dot_htf_loop, ms_sim_info);

    if (exit_code != E_loop_energy_balance_exit::SOLVED)
    {
        *T_htf_loop_out = std::numeric_limits<double>::quiet_NaN();
        return -1;
    }

    // Set the outlet temperature at end of timestep
    *T_htf_loop_out = mpc_fresnel->m_T_htf_out_t_end[mpc_fresnel->m_nMod - 1];

    return 0;
}

int C_csp_fresnel_collector_receiver::C_mono_eq_freeze_prot_E_bal::operator()(double T_htf_cold_in /*K*/, double* E_loss_balance /*-*/)
{
    // Solve the loop energy balance at the input HTF inlet temperature
    if (mpc_fresnel->loop_energy_balance_T_t_int(ms_weather, T_htf_cold_in, m_m_dot_loop, ms_sim_info) != E_loop_energy_balance_exit::SOLVED)
    {
        *E_loss_balance = std::numeric_limits<double>::quiet_NaN();
        return -1;
    }

    // Get energy added to the HTF
    m_Q_htf_fp = mpc_fresnel->m_m_dot_htf_tot * mpc_fresnel->m_c_htf_ave_ts_ave_temp *
        (T_htf_cold_in - mpc_fresnel->m_T_sys_h_t_end_last) / 1.E6 * (ms_sim_info.ms_ts.m_step);	//[MJ]

    // Set the normalized difference between the Field Energy Loss and Freeze Protection Energy
    *E_loss_balance = (m_Q_htf_fp - mpc_fresnel->m_Q_field_losses_total_subts) / mpc_fresnel->m_Q_field_losses_total_subts;		//[-]

    return 0;
}




// ******************************************************************** Evacuated Receiver Model

//    This subroutine contains the trough detailed plant model.  The collector field is modeled
//    using an iterative solver.
//    This code was written for the National Renewable Energy Laboratory
//    Copyright 2009-2010
//    Author: Mike Wagner
//
//    Subroutine Inputs (and parameters)
//     ----------------------------------------------------------------------------------------------------------------------
//     Nb | Variable             | Description                                             | Input  Units   | Internal Units
//     ---|----------------------|---------------------------------------------------------|----------------|----------------
//     1  | T_1_in               | Receiver inlet temperature                              |                |
//     2  | m_dot                | Heat transfer fluid mass flow rate                      |                |
//     3  | T_amb                | Ambient dry-bulb temperature                            |                |
//     4  | T_sky                | Sky temperature                                         |                |
//     5  | v_6                  | Ambient wind velocity                                   |                |
//     6  | P_6                  | Ambient atmospheric pressure                            |                |
//     7  | q_i                  | Total incident irradiation on the receiver              |                |
//     8  | A_cs                 | Internal absorber tube cross-sectional area             |                |
//     9  | m_D_abs_in                  | Internal absorber tube diameter                         |                |
//     10 | m_D_abs_out                  | External absorber tube diameter                         |                |
//     11 | m_D_glass_in                  | Internal glass envelope diameter                        |                |
//     12 | m_D_glass_out                  | External glass envelope diameter                        |                |
//     13 | m_D_plug                  | (optional) Plug diameter                                |                |
//     14 | m_D_h                  | Absorber tube hydraulic diameter                        |                |
//     15 | eps_mode             | Interpolation mode for the emissivity (1=table,2=fixed) |                |
//     16 | xx                   | Array of temperature values for emissivity table        |                |
//     17 | yy                   | Array of emissivity values for table                    |                |
//     18 | nea                  | Number of entries in the emissivity table               |                |
//     19 | m_L_mod             | Length of the active receiver surface                   |                |
//     20 | single_point         | Logical flag - is the calculation for a single point?   |                |
//     21 | Epsilon_32           | Constant value for emissivity if table isn't used       |                |
//     22 | Epsilon_4            | Envelope inner surface emissivity                       |                |
//     23 | epsilon_glass            | Envelope outer surface emissivity                       |                |
//     24 | m_alpha_abs            | Absorber tube absorptance                               |                |
//     25 | m_alpha_env            | Envelope absorptance                                    |                |
//     26 | m_ColOptEff            | Collector optical efficiency                            |                |
//     27 | m_Tau_envelope         | Total envelope transmittance                            |                |
//     28 | m_P_a                  | Annulus gas pressure                                    | torr           |
//     29 | Flow_type            | Flag indicating the presence of an internal plug        |                |
//     30 | AnnulusGas           | Annulus gas type                                        |                |
//     31 | Fluid                | Heat transfer fluid type                                |                |
//     32 | AbsorberMaterial     | Absorber material type                                  |                |
//     33 | time                 | Simulation time                                         |                |
//
//    Subroutine outputs
//     ----------------------------------------------------------------------------------------------------------------------
//     Nb | Variable             | Description                                             | Input  Units   | Internal Units
//     ---|----------------------|---------------------------------------------------------|----------------|----------------
//     1  | q_heatloss           | Total heat loss from the receiver                       | W/m            |
//     2  | q_12conv             | Total heat absorption into the HTF                      | W/m            |
//     3  | q_34tot              | Convective and radiative heat loss                      |                |
//     4  | c_1ave               | Specific heat of the HTF across the receiver            | kJ/kg-K        |
//     5  | rho_1ave             | Density of the HTF across the receiver                  |                |
//
//     ----------------------------------------------------------------------------------------------------------------------
//    Forristall Temperature distribution diagram
//    *****************************************************
//        Fluid (1) ----------->(2)<--Absorber-->(3)<-- Annulus -->(4)<--- Glass  --->(5)<-- Air (6)/Sky (7)
//
//
//        T_1 = Bulk heat transfer fluid (HTF) temperature
//        T_2 = Absorber Inside surface temperature
//        T_3 = Absorber outside surface temperature
//        T_4 = Glass envelope inside surface temperature
//        T_5 = Glass envelope outside surface temperature
//        T_6 = Ambient temperature
//        T_7 = Effective Sky Temperature
//
//        q_12conv = Convection heat transfer rate per unit length between the HTF and the inside of the receiver tube
//        q_23cond = Conduction heat transfer rate per unit length through the absorber
//        q_34conv = Convection heat transfer rate per unit length between the absorber outer surface and the glazing inner surface
//        q_34rad = Radiation heat transfer rate per unit length between the absorber outer surface and the glazing inner surface
//        q_45cond = Conduction heat transfer rate per unit length through the glazing
//        q_56conv = Convection heat transfer rate per unit length between the glazing outer surface and the ambient air
//        q_57rad = Radiation heat transfer rate per unit length between the glazing outer surface and the sky
//    ----------------------------------------------------------------------------------------------------------------------
//    */

/// <summary>
///    This subroutine contains the fresnel detailed plant model.  The collector field is modeled
///    using an iterative solver.
///    This code was written for the National Renewable Energy Laboratory
///    Copyright 2009-2010
///    Author: Mike Wagner
/// </summary>
/// <param name="D_abs_in">Internal absorber tube diameter</param>
/// <param name="D_abs_out">External absorber tube diameter</param>
/// <param name="D_glass_in">Internal glass envelope diameter</param>
/// <param name="D_glass_out">External glass envelope diameter</param>
/// <param name="D_plug">(optional) Plug diameter</param>
/// <param name="L_mod">Length of the active receiver surface</param>
/// <param name="GlazingIntact">Is glazing intact</param>
/// <param name="Shadowing"></param>
/// <param name="dirt_env"></param>
/// <param name="P_a">Annulus gas pressure</param>
/// <param name="alpha_abs">Absorber tube absorptance</param>
/// <param name="epsilon_glass">Envelope outer surface emissivity</param>
/// <param name="Tau_envelope">Total envelope transmittance</param>
/// <param name="alpha_env">Envelope absorptance</param>
/// <param name="epsilon_abs"></param>
/// <param name="htfProps">HTF Material Properties</param>
/// <param name="airProps">Air Material Properties</param>
/// <param name="AnnulusGasMat">Annulus gas type</param>
/// <param name="AbsorberPropMat">Annulus gas properties</param>
/// <param name="Flow_type">Fluid type</param>
/// <param name="A_cs">Cross sectional area</param>
/// <param name="D_h">Hydraulic diameter</param>
EvacReceiverModel::EvacReceiverModel(vector<double> D_abs_in, vector<double> D_abs_out, vector<double> D_glass_in, vector<double> D_glass_out, vector<double> D_plug,
    double L_mod, vector<bool> GlazingIntact, vector<double> Shadowing, vector<double> dirt_env, vector<double> P_a, vector<double> alpha_abs,
    vector<double> epsilon_glass, vector<double> Tau_envelope, vector<double> alpha_env, emit_table* epsilon_abs, HTFProperties htfProps, HTFProperties airProps,
    util::matrix_t<HTFProperties*> AnnulusGasMat, util::matrix_t<AbsorberProps*> AbsorberPropMat, vector<double> Flow_type, vector<double> A_cs, vector<double> D_h)
    :
    m_D_abs_in(D_abs_in), m_D_abs_out(D_abs_out), m_D_glass_in(D_glass_in), m_D_glass_out(D_glass_out), m_D_plug(D_plug),
    m_L_mod(L_mod), m_GlazingIntact(GlazingIntact), m_Shadowing(Shadowing), m_dirt_env(dirt_env), m_P_a(P_a), m_alpha_abs(alpha_abs),
    m_epsilon_glass(epsilon_glass), m_Tau_envelope(Tau_envelope), m_alpha_env(alpha_env), m_epsilon_abs(epsilon_abs), m_htfProps(htfProps), m_airProps(airProps),
    m_AnnulusGasMat(AnnulusGasMat), m_AbsorberPropMat(AbsorberPropMat), m_Flow_type(Flow_type), m_A_cs(A_cs), m_D_h(D_h)
{
}

/// <summary>
/// Calculate Energy Balance for evacuated receiver
/// </summary>
/// <param name="T_1_in">Receiver inlet temperature</param>
/// <param name="m_dot">Heat transfer fluid mass flow rate</param>
/// <param name="T_amb">Ambient dry-bulb temperature</param>
/// <param name="T_sky">Sky temperature</param>
/// <param name="v_6">Ambient wind velocity</param>
/// <param name="P_6">Ambient atmospheric pressure</param>
/// <param name="q_i">Total incident irradiation on the receiver</param>
/// <param name="hv">HCE variant [0..3]</param>
/// <param name="sca_num">Module index</param>
/// <param name="single_point"></param>
/// <param name="ncall"></param>
/// <param name="time"></param>
/// <param name="ColOptEff">Collector optical efficiency</param>
/// <param name="q_heatloss">Total heat loss from the receiver</param>
/// <param name="q_12conv">Total heat absorption into the HTF</param>
/// <param name="q_34tot">Convective and radiative heat loss</param>
/// <param name="c_1ave">Specific heat of the HTF across the receiver</param>
/// <param name="rho_1ave">Density of the HTF across the receiver</param>
/// <param name="v_reguess_args"></param>
/// <param name="q_3reflect">Absorber reflective losses</param>
void EvacReceiverModel::Calculate_Energy_Balance(double T_1_in, double m_dot, double T_amb, double T_sky, double v_6, double P_6, double q_i,
    int hv /* HCE variant [0..3] */, int sca_num, bool single_point, double time, util::matrix_t<double> ColOptEff,
    //outputs
    double& q_heatloss, double& q_12conv, double& q_34tot, double& c_1ave, double& rho_1ave, std::vector<double>& v_reguess_args, double& q_3reflect)
{

    //---Variable declarations------
    bool reguess;
    double T_2, T_3, T_4, T_5, T_6, T_7, v_1, k_23, q_34conv, q_34rad, h_34conv, h_34rad, q_23cond,
        k_45, q_45cond, q_56conv, h_56conv, q_57rad, q_3SolAbs, q_5solabs, q_cond_bracket, R_45cond,
        T_save[5], T_2g, cp_1, T3_tol, q5_tol, T1_tol, T2_tol, Diff_T3, diff_q5, T_lower, T_upper,
        q_5out, T_1_out, diff_T1, T_1_ave, T_1_out1, diff_T2, eps_3, q_in_W, T_upper_max, y_upper,
        y_lower, upmult, q5_tol_1, T3_upper, T3_lower, y_T3_upper, y_T3_lower, abs_diffT3;

    bool UPFLAG, LOWFLAG, T3upflag, T3lowflag, is_e_table;
    int qq, q5_iter, T1_iter, q_conv_iter;

    double T_save_tot, colopteff_tot;
    //cc--> note that xx and yy have size 'nea'

    //---Re-guess criteria:---
    if (time <= 2) goto lab_reguess;

    if (((int)v_reguess_args[0] == 1) != m_GlazingIntact.at(hv)) goto lab_reguess;	//m_GlazingIntact state has changed

    if (m_P_a[hv] != v_reguess_args[1]) goto lab_reguess;                   //Reguess for different annulus pressure

    if (std::abs(v_reguess_args[2] - T_1_in) > 50.) goto lab_reguess;

    for (int i = 0; i < 5; i++) { if (T_save[i] < T_sky - 1.) goto lab_reguess; }

    T_save_tot = 0.;
    for (int i = 0; i < 5; i++) { T_save_tot += T_save[i]; }
    if (T_save_tot != T_save_tot) goto lab_reguess;	//NaN check.. a value is only not equal to itself if it is NaN

    reguess = false;
    goto lab_keep_guess;
lab_reguess:
    reguess = true;
lab_keep_guess:


    //------------------------

    if (reguess) {
        if (m_GlazingIntact.at(hv)) {
            T_save[0] = T_1_in;
            T_save[1] = T_1_in + 2.;
            T_save[2] = T_save[1] + 5.;
            if (m_P_a[hv] > 1.0) {          //Set guess values for different annulus pressures
                T_save[3] = T_save[2] - 0.5 * (T_save[2] - T_amb);       //If higher pressure, guess higher T4   
                T_upper_max = T_save[2] - 0.2 * (T_save[2] - T_amb);     //Also, high upper limit for T4
            }
            else {
                T_save[3] = T_save[2] - 0.9 * (T_save[2] - T_amb);       //If lower pressure, guess lower T4
                T_upper_max = T_save[2] - 0.5 * (T_save[2] - T_amb);     //Also, low upper limit for T4
            }
            T_save[4] = T_save[3] - 2.;

            v_reguess_args[1] = m_P_a[hv];               //Reset previous pressure
            v_reguess_args[0] = m_GlazingIntact.at(hv) ? 1. : 0.;   //Reset previous glazing logic
            v_reguess_args[2] = T_1_in;            //Reset previous T_1_in

        }
        else {
            T_save[0] = T_1_in;
            T_save[1] = T_1_in + 2.;
            T_save[2] = T_save[1] + 5.;
            T_save[3] = T_amb;
            T_save[4] = T_amb;

            v_reguess_args[0] = m_GlazingIntact.at(hv) ? 1. : 0.;   //Reset previous glazing logic
            v_reguess_args[1] = T_1_in;            //Reset previous T_1_in

        }
    }

    //Set intial guess values
    T_2 = T_save[1];
    T_3 = T_save[2];
    T_4 = T_save[3];
    T_5 = T_save[4];
    //Set constant temps
    T_6 = T_amb;
    T_7 = T_sky;

    qq = 0;                  //Set iteration counter for T3 loop

    T_2g = T_2;              //Initial guess value for T_2 (only used in property lookup)        
    cp_1 = 1950.;            //Initial guess value for cp of WF

    //Tolerances for iteration
    T3_tol = 1.5e-3;
    q5_tol = 1.0e-3;         //Since iterations are nested inside T3, make tolerances a bit tighter        
    T1_tol = 1.0e-3;
    T2_tol = 1.0e-3;

    ////Decreasing the tolerance helps get out of repeating defocus iterations
    //if (ncall > 8) {
    //    T3_tol = 1.5e-4;        //1.0   
    //    q5_tol = 1.0e-4;        //max(1.0, 0.001*q_i)
    //    T1_tol = 1.0e-4;        //1.0
    //    T2_tol = 1.0e-4;        //1.0
    //}

    Diff_T3 = 10.0 + T3_tol;    //Set difference > tolerance

    //Constants
    k_45 = 1.04;                             //[W/m-K]  Conductivity of glass
    R_45cond = log(m_D_glass_out[hv] / m_D_glass_in[hv]) / (2. * pi * k_45);    //[K-m/W]Equation of thermal resistance for conduction through a cylinder

    colopteff_tot = ColOptEff.at(sca_num) * m_dirt_env[hv] * m_Shadowing[hv];	//The total optical efficiency

    if (m_GlazingIntact.at(hv)) {   //These calculations (q_3SolAbs,q_5solAbs) are not dependent on temperature, so only need to be computed once per call to subroutine

        q_3SolAbs = q_i * colopteff_tot * m_Tau_envelope[hv] * m_alpha_abs[hv];  //[W/m]  
        //We must account for the radiation absorbed as it passes through the envelope
        q_5solabs = q_i * colopteff_tot * m_alpha_env[hv];   //[W/m]

        q_3reflect = q_i * colopteff_tot * m_Tau_envelope[hv] * (1.0 - m_alpha_abs[hv]); // [W/m]
    }
    else {
        //Calculate the absorbed energy 
        q_3SolAbs = q_i * colopteff_tot * m_alpha_abs[hv];  //[W/m]  
        //No envelope
        q_5solabs = 0.0;                            //[W/m]


        q_3reflect = q_i * colopteff_tot * (1.0 - m_alpha_abs[hv]); // [W/m]
    }

    is_e_table = false;
    if (m_epsilon_abs->getTableSize(hv) < 2) {
        eps_3 = m_epsilon_abs->getSingleValue(hv);
    }
    else {
        eps_3 = m_epsilon_abs->interpolate(hv, T_3 - 273.15);          //Set epsilon value for case that eps_mode = 1.  Will reset inside temp loop if eps_mode > 1.
        is_e_table = true;	//The emissivity is in tabular form
    }

    T3upflag = false;
    T3lowflag = false;

    double T3_adjust = 0.0;
    double T3_prev_qq = 0.0;

    while (((std::abs(Diff_T3) > T3_tol) && (qq < 100)) || (qq < 2)) {    //Outer loop: Find T_3 such than energy balance is satisfied
        qq = qq + 1; //loop counter

        T3_prev_qq = T_3;

        if (qq > 1) {
            if ((T3upflag) && (T3lowflag)) {
                if (Diff_T3 > 0.) {
                    T3_upper = T_3;
                    y_T3_upper = Diff_T3;
                }
                else {
                    T3_lower = T_3;
                    y_T3_lower = Diff_T3;
                }
                T_3 = (y_T3_upper) / (y_T3_upper - y_T3_lower) * (T3_lower - T3_upper) + T3_upper;

            }
            else {
                if (Diff_T3 > 0.) {
                    T3_upper = T_3;
                    y_T3_upper = Diff_T3;
                    T3upflag = true;
                }
                else {
                    T3_lower = T_3;
                    y_T3_lower = Diff_T3;
                    T3lowflag = true;
                }

                if ((T3upflag) && (T3lowflag)) {
                    T_3 = (y_T3_upper) / (y_T3_upper - y_T3_lower) * (T3_lower - T3_upper) + T3_upper;
                }
                else
                {
                    if (Diff_T3 > 0.)
                        T_3 = T_3 - 50.0;
                    else
                        T_3 = T_3 + 50.0;
                    //T_3 = max(T_7, T_3 - abs_diffT3);         //Note that recalculating T_3 using this exact equation, rather than T_3 = T_3 - frac*diff_T3 was found to solve in fewer iterations
                }
            }
        }

        T3_adjust = T_3 - T3_prev_qq;

        //Calculate temperature sensitive emissivity using T_3, if required
        if (is_e_table) eps_3 = m_epsilon_abs->interpolate(hv, (T_3 - 273.15)); //call interp((T_3-273.15),eps_mode,xx,yy,eps3old,eps_3)

        //Separate m_GlazingIntact = true and m_GlazingIntact = false  If true, T4 must be solved, if false then T4 is explicitly known (or doesn't exist, depending on how you want to look at it)
        //Solving for correct T4 as it relates to current T3 value
        if (m_GlazingIntact.at(hv)) {

            //**********************************************
            //************* SET UP T_4 ITERATION **********************
            //**********************************************

            // if(qq==1){               //If first iteration, set T_4 bounds to phyiscal limits defined by T_3 and T_sky
            // 	T_lower = T_sky;         //Lowest possible temperature of T_4 is sky temp        
            // 	T_upper = max(T_upper_max,T_amb);    //Highest possible temperature is the highest temperature on either side of T_4: either T_3 or ambient
            // 	q5_tol_1= 0.001;           //Just get T4 in the ball park.  '20' may not be the optimum value.....
            // } 
            // else {                                            //For additional iterations:
            // 	T_lower = T_lower - max(abs_diffT3,0.0);       //If diff_T3 is + then new T3 < old T3 so adjust lower limit
            // 	T_upper = T_upper + fabs(min(abs_diffT3,0.0));  //If diff_T3 is (-) then new T3 > old T3 so adjust upper limit
            // 	q5_tol_1= q5_tol;        //For remaining T3 iterations, use specified tolerance (note that 2 iterations for T3 are gauranteed)                   
            // }
            if (qq == 1)
            {
                T_lower = T_sky;
                T_upper = max(T_3, T_amb);
            }
            else
            {
                if (T3_adjust > 0.0)	// new T3 > old T3 so adjust upper limit
                {
                    T_upper = min(T_3, T_upper + 1.25 * T3_adjust);
                    T_lower = T_4;
                    T_4 = T_4 + 0.5 * T3_adjust;
                }
                else	// T3_adjust negative
                {
                    T_lower = max(T_sky, T_lower + 1.25 * T3_adjust);
                    T_upper = T_4;
                    T_4 = T_4 + 0.5 * T3_adjust;
                }
            }
            q5_tol_1 = q5_tol;

            //if( T_4 > T_upper || T_4 < T_lower )
            //	T_4 = 0.5*(T_upper + T_lower);

            diff_q5 = q5_tol_1 + 1.0;       //Set diff > tolerance
            q5_iter = 0;                     //Set iteration counter

            UPFLAG = false;           //Set logic to switch from bisection to false position mode
            LOWFLAG = false;           //Set logic to switch from bisection to false position mode   
            //***********************************************************************************
            //************* Begin Bisection/False Position Iteration method *********************
            //***********************************************************************************
            while ((std::abs(diff_q5) > q5_tol_1) && (q5_iter < 100)) {       //Determine T_4 such that energy balance from T_3 to surroundings is satisfied

                q5_iter = q5_iter + 1;                       //Increase iteration counter

                //The convective heat exchange between the absorber and the envelope
                //      UNITS   ( K , K, torr, Pa , m/s,  K , -, -, W/m, W/m2-K)
                FQ_34CONV_v2(T_3, T_4, P_6, v_6, T_6, hv, q_34conv, h_34conv);

                //The radiative heat exchange between the absorber and the envelope
                //    Units         ( K ,  K ,  m ,  m , K  ,    -     ,    -     ,   logical    ,  W/m   , W/m2-K)
                FQ_34RAD_v2(T_3, T_4, T_7, eps_3, hv, q_34rad, h_34rad);
                //The total heat exchange between absorber and envelope
                q_34tot = q_34conv + q_34rad;            //[W/m]

                //**********************************************
                //************* Calculate T_5 *************
                //**********************************************
                //The thermal energy flow across 45 is equal to the energy from the absorber plus
                //the thermal energy that is generated by direct heating of the glass envelope
                q_45cond = q_34tot + q_5solabs;          //[W/m]

                //Knowing heat flow and properties, T_5 can be calculated
                T_5 = T_4 - q_45cond * R_45cond;           //[K]

                //*************************************************************************
                //************* Calculate HT from exterior surface to ambient *************
                //*************************************************************************
                //With T_5 and T_6 (amb T) calculate convective and radiative loss from the glass envelope
                //           units   ( K ,  K ,  torr, m/s, -, -, W/m, W/m2-K)
                FQ_56CONV_v2(T_5, T_6, P_6, v_6, hv, q_56conv, h_56conv); //[W/m]
                q_57rad = m_epsilon_glass[hv] * 5.67e-8 * (pow(T_5, 4) - pow(T_7, 4));
                q_5out = q_57rad + q_56conv;     //[W/m]

                //***************************************************************************
                //********** Compare q_5out with q_45 cond***********************************
                //***************************************************************************
                diff_q5 = (q_5out - q_45cond) / q_45cond;     //[W/m]

                //Determine next guess for T_4.  Want to use false position method, but it requires that the *results* at both ends of the bracket are known.  We have
                //defined a bracket but not the results.  Use the guess T_4 to get the results at one end of a new bracket.  Then calculate a new T_4 that is highly weighted 
                //towards the side of the original bracket that the 1st T_4 did not replace.  In most cases, this new T_4 will result in the opposite diff_q5, which 
                //defines both sides of the bracket.  If results for both sides are then defined, "LOWFLAG" and "UPFLAG" will be true, and false position method will be applied.

                if (LOWFLAG && UPFLAG) {          //False position method     
                    if (diff_q5 > 0.0) {
                        T_upper = T_4;       //If energy leaving T_5 is greater than energy entering T_5, then T_4 guess is too high
                        y_upper = diff_q5;   //so set new upper limit to T_4
                    }
                    else {                    //If energy leaving T_5 is less than energy entering T_5, then T_4 guess is too low
                        T_lower = T_4;       //so set new lower limit to T_4
                        y_lower = diff_q5;   //also, set result to go along with lower limit
                    }
                    T_4 = (y_upper) / (y_upper - y_lower) * (T_lower - T_upper) + T_upper;

                }
                else {                        //For bisection method...

                    if (diff_q5 > 0.0) {    //If energy leaving T_5 is greater than energy entering T_5, then T_4 guess is too high
                        T_upper = T_4;       //so set new upper limit to T_4
                        y_upper = diff_q5;   //also, set result to go along with upper limit
                        UPFLAG = true;    //Upper result is now known
                        if (qq == 1) {
                            upmult = 0.1;       //Just want to get in ballpark for first iteration of receiver
                        }
                        else {
                            upmult = 0.1;       //Weight such that next calculated T_4 (if using bisection method) results in negative diff_q5
                        }

                    }
                    else {                    //If energy leaving T_5 is less than energy entering T_5, then T_4 guess is too low          
                        T_lower = T_4;       //so set new lower limit to T_4
                        y_lower = diff_q5;   //also, set result to go along with lower limit
                        LOWFLAG = true;    //Lower result is now known
                        if (qq == 1) {
                            upmult = 0.1;       //Just want to get in ballpark for first iteration of receiver
                        }
                        else {
                            upmult = 0.9;       //Weight such that next calculated T_4 (if using bisection method) results in positive diff_q5
                        }

                    }

                    if (LOWFLAG && UPFLAG) {  //If results of bracket are defined, use false position
                        T_4 = (y_upper) / (y_upper - y_lower) * (T_lower - T_upper) + T_upper;
                    }
                    else {                            //If not, keep bisection
                        T_4 = (1. - upmult) * T_lower + upmult * T_upper;
                    }

                }

                //*********************************************************************************************
                //********** END Bisection/False Position Iteration Loop on T_4 *******************************
                //*********************************************************************************************       
            }

        }
        else {      //Glazing is not intact

            //Know convection and radiation forcing temps
            //----Having guessed the system temperatures, calculate the thermal losses starting from
            //----the absorber surface (3)
            //The convective heat exchange between the absorber and the envelope
            FQ_34CONV_v2(T_3, T_4, P_6, v_6, T_6, hv, q_34conv, h_34conv);
            //The radiative heat exchange between the absorber and the envelope
            FQ_34RAD_v2(T_3, T_4, T_7, eps_3, hv, q_34rad, h_34rad);
            //The total heat exchange between absorber and envelope
            q_34tot = q_34conv + q_34rad;    //[W/m]

        }      //Know heat transfer from outer surface of receiver tube to ambient

        //Bracket Losses
        //Bracket conduction losses apply 
        q_cond_bracket = FQ_COND_BRACKET_v2(T_3, T_6, P_6, v_6); //[W/m] 

        q_12conv = q_3SolAbs - (q_34tot + q_cond_bracket);         //[W/m] Energy transfer to/from fluid based on energy balance at T_3

        q_in_W = q_12conv * m_L_mod;                           //Convert [W/m] to [W] for some calculations

        if (!single_point) {
            T_1_out = max(T_sky, q_in_W / (m_dot * cp_1) + T_1_in);    //Estimate outlet temperature with previous cp

            diff_T1 = T1_tol + 1.0;                                 //Set diff > tolerance
            T1_iter = 0;                                             //Set iteration counter    

            while ((std::abs(diff_T1) > T1_tol) && (T1_iter < 100)) {       //Find correct cp& rho and solve for T_1_ave

                T1_iter++;                   //Increase iteration counter
                T_1_ave = (T_1_out + T_1_in) / 2.0;     //Average fluid temperature
                cp_1 = m_htfProps.Cp(T_1_ave) * 1000.;
                T_1_out1 = max(T_sky, q_in_W / (m_dot * cp_1) + T_1_in);  //Estimate outlet temperature with previous cp 
                diff_T1 = (T_1_out - T_1_out1) / T_1_out;  //Difference between T_1_out used to calc T_ave, and T_1_out calculated with new cp
                T_1_out = T_1_out1;                      //Calculate new T_1_out

            }
        }
        else {
            //If we're only calculating performance for a single point, set the receiver ave/outlet temperature to the inlet.
            T_1_out = T_1_in;
            T_1_ave = T_1_in;
        }

        rho_1ave = m_htfProps.dens(T_1_ave, 0.0);       //[kg/m^3] Density
        v_1 = m_dot / (rho_1ave * m_A_cs.at(hv));             //HTF bulk velocity

        q_conv_iter = 0;                 //Set iteration counter
        diff_T2 = 1.0 + T2_tol;         //Set diff > tolerance

        bool T2upflag = false;
        bool T2lowflag = false;

        double y_T2_low = std::numeric_limits<double>::quiet_NaN();
        double y_T2_up = std::numeric_limits<double>::quiet_NaN();

        double T2_low = min(T_1_ave, T_3);
        double T2_up = max(T_1_ave, T_3);

        //Ensure convective calculations are correct (converge on T_2)
        while ((std::abs(diff_T2) > T2_tol) && (q_conv_iter < 100)) {

            q_conv_iter++;       //Increase iteration counter

            T_2 = fT_2_v2(q_12conv, T_1_ave, T_2g, v_1, hv);	//Calculate T_2 (with previous T_2 as input)
            diff_T2 = (T_2 - T_2g) / T_2;          //T_2 difference

            if (diff_T2 > 0.0)		// Calculated > Guessed, set lower limit and increase guessed
            {
                T2_low = T_2g;
                T2lowflag = true;
                y_T2_low = diff_T2;
                if (T2upflag)
                    T_2g = y_T2_up / (y_T2_up - y_T2_low) * (T2_low - T2_up) + T2_up;
                else
                    T_2g = T2_up;
            }
            else					// Calculated < Guessed, set upper limit and decrease guessed
            {
                T2_up = T_2g;
                T2upflag = true;
                y_T2_up = diff_T2;
                if (T2lowflag)
                    T_2g = y_T2_up / (y_T2_up - y_T2_low) * (T2_low - T2_up) + T2_up;
                else
                    T_2g = T2_low;
            }

            if ((T2_up - T2_low) / T2_low < T2_tol / 10.0)
                break;

            //T_2g = T_2 - 0.5*(T_2-T_2g);         //Reset T_2

            // if(qq<2){        //For first T3 iteration, do not iterate on T_2 (again, this control is based on observation of solve time and may not be optimal for all simulations)
            // 	break;
            // }

        }

        //The conductive heat transfer equals the convective heat transfer (energy balance)
        q_23cond = q_12conv;         //[W/m]

        //Calculate tube conductivity 
        k_23 = FK_23_v2(T_2, T_3, hv);       //[W/m-K]  

        //Update the absorber surface temperature (T_3) according to new heat transfer rate
        abs_diffT3 = T_3 - (T_2 + q_23cond * log(m_D_abs_out[hv] / m_D_abs_in[hv]) / (2. * pi * k_23));
        Diff_T3 = abs_diffT3 / T_3;


    }

    //Warning of convergence failure
    //if(qq>99) {                           //End simulation if loop does not converge
    //    call messages(-1,"Trough Energy Balance Convergence Error 1",'WARNING',INFO(1),INFO(2))
    //    return
    //}
    //
    //if(T1_iter>99) {
    //    call messages(-1,"Trough Energy Balance Convergence Error 2",'WARNING',INFO(1),INFO(2))
    //    return
    //}
    //
    //if(q_conv_iter>99) {
    //    call messages(-1,"Trough Energy Balance Convergence Error 3",'WARNING',INFO(1),INFO(2))
    //    return
    //}
    //
    //if(q5_iter>99) {
    //    call messages(-1,"Trough Energy Balance Convergence Error 4",'WARNING',INFO(1),INFO(2))
    //    return
    //}

    //Calculate specific heat in kJ/kg
    c_1ave = cp_1 / 1000.;

    q_heatloss = q_34tot + q_cond_bracket + q_5solabs;   //[W/m]

    //Save temperatures
    T_save[1] = T_2;
    T_save[2] = T_3;
    T_save[3] = T_4;
    T_save[4] = T_5;

};

//    #################################################################################################################
//    #################################################################################################################
//    #################################################################################################################
//
//
//    "******************************************************************************************************************************
//        FUNCTION Fq_12conv :  Convective heat transfer rate from the HTF to the inside of the receiver tube
//    ******************************************************************************************************************************"
//      Author: R.E. Forristall (2003, EES)
//      Implemented and revised:  M.J. Wagner (10/2009)
//                    Copyright:  National Renewable Energy Lab (Golden, CO) 2009
//                         note:  This function was programmed and tested against the EES original.
//                                Small variations in output are due to slightly different fluid
//                                properties used in the two models.
//
//        Newton's Law of Cooling.
//
//            q' = h * D_i *  PI * (T_m - T_s)
//
//            h = Nu_Di * k / D_i
//
//        Where
//
//            q' = convection heat transfer rate per unit length [W/m]
//            h = convection heat transfer coefficient [W/m^2-k]
//            D_i = inside diameter of absorber pipe [m]
//            T_m = mean (bulk) temperature of HTF [C]
//            T_s = inside surface temperature of absorber pipe [C]
//            Nu_Di = Nusselt number based on inside diameter
//            k = conduction heat transfer coefficient of HTF [W/m-K]
//
//        The Nusselt number is estimated with the correlation developed by Gnielinski.
//
//            Nu# = (f / 8) * (Re_Di - 1000) * Pr / (1 + 12.7 * (f / 8)^(1/2) * (Pr^(2/3) -1))  * (Pr / Pr_w)^0.11
//            f = (1.82 * log10(Re_Di) - 1.64)^(-2)
//            Re_Di = Rho * v_m * Di / u
//            Pr  = Cp * u / k
//
//        Where
//
//            Nu# = Nusselt number
//            Re_Di = Reynolds number for internal pipe flow
//            Pr = Prandtl number
//            Pr_w = Prandtl number evaluated at the wall temperature
//            u = fluid absolute viscosity [kg/m-s]
//            Di = inside diameter [m]
//            Cp = fluid specific heat [J/kg-K]
//            k = fluid thermal conductivity [W/m-K]
//            Rho = fluid density [kg/m^3]
//            v_m = mean fluid velocity [m/s]
//
//    The above correlation is valid for 0.5 < Pr < 2000 and 2300< Re_Di < 5 * 10^6 and can be used for both uniform heat flux and uniform wall temperature cases. With the exception of Pr_w, all properties are evaluated at the mean fluid temperature.
//
//    If Re_D <= 2300 and the choice was made from the diagram window  to use the laminar flow model, one of  the following correlations is used.
//
//            for inner tube flow (uniform flux condition)
//            Nu# = 4.36
//
//            for inner annulus flow (uniform flux condition -- estimated from table for Nu# with heat fluxes at both surfaces)
//            m_D_plug/m_D_abs_in	Nu#
//            0		4.364
//            0.05		4.792
//            0.10		4.834
//            0.20		4.833
//            0.40		4.979
//            0.60		5.099
//            0.80		5.24
//            1.00		5.385
//
//
//    For the "SNL test platform" case the inside diameter in the above correlations is replaced with the following hydraulic diameter definition.
//
//            m_D_h = 4 * A_c / P = D_ao - D_ai
//
//        Where
//
//            m_D_h = hydraulic diameter [m]
//            A_c = flow cross sectional area [m^2]
//            P = wetted perimeter [m]
//            D_ai = inner annulus diameter [m]
//            D_ao = outer annulus diameter [m]
//
//    (Sources: Incropera, F., DeWitt, D., Fundamentals of Heat and Mass Transfer, Third Edition; John Wiley and Sons, New York, 1981, pp. 489-491, 502-503. Gnielinski, V., "New Equations for Heat and Mass Transfer in Turbulent Pipe and Channel Flow," International Chemical Engineering, Vol. 16, No. 2, April 1976.)
//    */
double EvacReceiverModel::fT_2_v2(double q_12conv, double T_1, double T_2g, double v_1, int hv)
{
    //		convection 1->2,  HTF temp, guess T2,  fluid velocity, HCE #, HCE variant

    // missing variables
    //HTFProperties m_htfProps;
    //vector<double> m_D_h;
    //vector<double> m_Flow_type;
    //vector<double> m_D_plug;
    //vector<double> m_D_abs_in;
    //double pi = 3.14159;

    double Cp_1, Cp_2, f, h_1, k_1, k_2, mu_1, mu_2, Nu_D2, Pr_1, Pr_2, Re_D2, rho_1, DRatio;
    bool includelaminar = true;	//cc -- this is always set to TRUE in TRNSYS

    T_2g = max(T_2g, m_T_htf_prop_min);

    // Thermophysical properties for HTF 
    mu_1 = m_htfProps.visc(T_1);  //[kg/m-s]
    mu_2 = m_htfProps.visc(T_2g);  //[kg/m-s]
    Cp_1 = m_htfProps.Cp(T_1) * 1000.;  //[J/kg-K]
    Cp_2 = m_htfProps.Cp(T_2g) * 1000.;  //[J/kg-K]
    k_1 = max(m_htfProps.cond(T_1), 1.e-4);  //[W/m-K]
    k_2 = max(m_htfProps.cond(T_2g), 1.e-4);  //[W/m-K]
    rho_1 = m_htfProps.dens(T_1, 0.0);  //[kg/m^3]

    Pr_2 = (Cp_2 * mu_2) / k_2;
    Pr_1 = (Cp_1 * mu_1) / k_1;

    if (v_1 > 0.1) {

        Re_D2 = (rho_1 * m_D_h.at(hv) * v_1) / (mu_1);

        // Nusselt Number for laminar flow case if option to include laminar flow model is chosen 
        if ((includelaminar == true) && (Re_D2 <= 2300.)) {
            if (m_Flow_type[hv] == 2.0) {
                DRatio = m_D_plug[hv] / m_D_abs_in[hv];
                //Estimate for uniform heat flux case (poly. regression based on lookup table in Forristall EES model)
                //---Note that this regression is based on an 8-point table, and is highly non-practical outside of DRatio bounds
                //---0 and 1
                if (DRatio > 1.) {
                    Nu_D2 = 5.385;
                }
                else if (DRatio < 0.) {
                    Nu_D2 = 4.364;
                }
                else {
                    Nu_D2 = 41.402 * pow(DRatio, 5) - 109.702 * pow(DRatio, 4) + 104.570 * pow(DRatio, 3) - 42.979 * pow(DRatio, 2) + 7.686 * DRatio + 4.411;
                }
            }
            else {
                Nu_D2 = 4.36;				//uniform heat flux
            }
        }
        else {
            // Warning statements if turbulent/transitional flow Nusselt Number correlation is used out of recommended range 
    //		if (Pr_1 <= 0.5) or (2000 <= Pr_1) { CALL WARNING('The result may not be accurate, since 0.5 < Pr_1 < 2000 does not hold. See PROCEDURE Pq_12conv. Pr_1 = XXXA1', Pr_1)
    //		if (Pr_2 <= 0.5) or (2000 <= Pr_2) { CALL WARNING('The result may not be accurate, since 0.5 < Pr_2 < 2000 does not hold. See PROCEDURE Pq_12conv. Pr_2 = XXXA1', Pr_2)
    //		If ( Re_D2 <= (2300) ) or (5*10**6 <= Re_D2 ) Then CALL WARNING('The result may not be accurate, since 2300 < Re_D2 < (5 * 10**6) does not hold. See PROCEDURE Pq_12conv. Re_D2 = XXXA1', Re_D2)

            // Turbulent/transitional flow Nusselt Number correlation (modified Gnielinski correlation) 	
            f = pow(1.82 * log10(Re_D2) - 1.64, -2);
            Nu_D2 = (f / 8.) * (Re_D2 - 1000.) * Pr_1 / (1. + 12.7 * sqrt(f / 8.) * (pow(Pr_1, 0.6667) - 1.)) * pow(Pr_1 / Pr_2, 0.11);
        }

        h_1 = Nu_D2 * k_1 / m_D_h.at(hv);  //[W/m**2-K]
        return T_1 + q_12conv / (h_1 * m_D_abs_in[hv] * pi);
        //q_12conv = h_1 * m_D_abs_in * PI  * (T_2 - T_1ave)  //[W/m]
    }
    else {
        h_1 = 0.0001;
        return T_1;
    }

};

//    FUNCTION fq_34conv :	Convective heat transfer rate between the absorber outer surface and the glazing inner surface
//******************************************************************************************************************************"
//  NOTE: Temperatures input in terms of degrees K
//
//  Author: R.E. Forristall (2003, EES)
//  Implemented and revised:  M.J. Wagner (10/2009)
//                Copyright:  National Renewable Energy Lab (Golden, CO) 2009
//
//{ Four cases:
//
//    1. Vacuum in annulus: free-molecular heat transfer model for an annulus.
//    2. Low or lost vacuum: natural convection heat transfer model for an annulus.
//    3. No glazing, no wind: natural convection heat transfer model for a horizontal cylinder.
//    4. No glazing, with wind: forced convection heat transfer model for a horizontal cylinder.
//
//
//Case 1:
//
//    Free-molecular heat transfer for an annular space between horizontal cylinders.
//
//        q' = D_i * PI * h * (T_i - T_o)
//        h = k_gas / (D_i / 2 * ln(D_o / D_i) + b * Lambda * (D_i / D_o + 1))
//        b = (2 - a) / a * (9 * Gamma - 5) / (2 * (Gamma + 1))
//        Lambda = 2.331 * 10^(-20) * T_avg / (P * Delta^2)
//
//    Where
//
//        q' = convection heat transfer rate per unit length [W/m]
//        D_i = outer absorber diameter [m]
//        D_o = inner glazing diameter [m]
//        h = convection heat transfer coefficient for annulus gas [W/m^2-K]
//        T_i = outer absorber surface temperature [C]
//        T_o = inner glazing surface temperature [C]
//        k_gas = thermal conductivity of the annulus fluid at standard temperature and pressure [W/m^2-K]
//        b = interaction coefficient [dimensionless]
//        Lambda = mean-free-path between collisions of a molecule [cm]
//        a = accommodation coefficient [dimensionless]
//        Gamma = ratio of specific heats for the annulus fluid [dimensionless]
//        T_avg = average temperature of the annulus fluid [K]
//        P = pressure of the annulus gas [mm of Hg]
//        Delta = molecular diameter of the annulus gas [cm]
//
//    The above correlation is valid for Ra_Do < (D_o / (D_o -D_i))^4, but may over estimate q' slightly for large vacuums.
//
//(Source: Ratzel, A., Hickox, C., Gartling, D., "Techniques for Reducing Thermal Conduction and Natural Convection Heat Losses
// in Annular Receiver Geometries," Journal of Heat Transfer, Vol. 101, No. 1, February 1979; pp. 108-113)
//
//
//Case 2:
//
//    Modified Raithby and Hollands correlation for natural convection in an annular space between horizontal cylinders.
//
//        q' = 2.425 * k * (T_i - T_o) / (1 + (D_i / D_o)^(3/5))^(5/4) * (Pr * Ra_Di / (0.861 + Pr))^(1/4)
//        Pr = NU / Alpha
//        Ra_Di = g * Beta * (T_i - T_o) * (D_i)^3 / (Alpha * NU)
//        Beta = 1 / T_avg		"Ideal Gas"
//
//    Where
//
//        k = conduction heat transfer coefficient for the annulus gas [W/m-K]
//        Pr = Prandtl number
//        NU = kinematic viscosity [m^2/s]
//        Alpha = thermal diffusivity [m^2/s]
//        Ra_Di = Rayleigh number based on the annulus inner diameter
//        g = local acceleration due to gravity [m/s^2]
//        Beta = volumetric thermal expansion coefficient [1/K]
//        Rho_o = annulus gas density at the outer surface [kg/m^3]
//        Rho_i = annulus gas density at the inner surface [kg/m^3]
//        T_avg = average temperature, (T_i + T_o) / 2 [K]
//
//    Above correlation is valid for Ra_Do > (D_o / (D_o -D_i))^4. All physical properties are evaluated at the average temperature, (T_i + T_o)/2.
//
//(Source: Bejan, A., Convection Heat Transfer, Second Edition; John Wiley & Son's, New York, 1995, pp. 257-259.)
//
//
//Case 3:
//
//    Churchill and Chu correlation for natural convection from a long isothermal horizontal cylinder.
//
//        Nu_bar = (0.60 + (0.387 * Ra_D^(1/6)) / (1 + (0.559 / Pr)^(9/16))^(8/27) )^2
//        Ra_D = g * Beta * (T_s - T_inf) * D^3 / (Alpha * NU)
//        Beta =  1 / T_f	"Ideal Gas"
//        Alpha = k / (Cp * Rho)
//        Pr = NU / Alpha
//
//        h = Nu_bar * k / D
//
//        q' = h * PI * D * (T_s - T_inf)
//
//    Where
//
//        Nu_bar = average Nusselt number
//        Ra_D = Rayleigh number based on diameter
//        Rho = fluid density  [kg/m^3]
//        Cp = specific heat at constant pressure [kJ / kg-K]
//        T_inf = fluid temperature in the free stream [C]
//        T_s = surface temperature [C]
//        T_f = film temperature, (T_s + T_inf) / 2 [K]
//        T_inf = ambient air temperature [C]
//
//    Above correlation is valid for  10^(-5) < Ra_D < 10^12. All physical properties are evaluated at the film temperature, (T_s + T_inf) / 2.
//
//(Source: Incropera, F., DeWitt, D., Fundamentals of Heat and Mass Transfer, Third Edition; John Wiley and Sons, New York, 1981, pp. 550-552.)
//
//
//Case 4:
//
//    Zhukauskas's correlation for external forced convection flow normal to an isothermal cylinder.
//
//        Nu_bar = C * Re_D^m * Pr^n * (Pr / Pr_s)^(1/4)
//
//        Re_D		C			m
//        1-40		0.75			0.4
//        40-1000		0.51			0.5
//        1e3- 2e5	0.26			0.6
//        2e5-1e6	0.076			0.7
//
//        n = 0.37, Pr <=10
//        n = 0.36, Pr >10
//
//        Re_D =  U_inf * D / NU
//        Pr  = NU / Alpha
//        Alpha = k / (Cp * Rho)
//
//        Q =  h * D * PI * (T_s - T_inf) * L
//
//    Where,
//
//        Re_D = Reynolds number evaluated at the diameter
//        Cp = specific heat at constant pressure of air [W/m-K]
//        Rho = density of air [kg/m^3]
//        C, m, n = constants
//
//    Above correlation is valid for  0.7 < Pr < 500, and 1 < Re_D < 10^6. All physical properties evaluated
//   at the free stream temperature, T_inf,  except Pr_s.
//
//(Source: Incropera, F., DeWitt, D., Fundamentals of Heat and Mass Transfer, Third Edition; John Wiley and
// Sons, New York, 1981, p. 413.)
//}*/
void EvacReceiverModel::FQ_34CONV_v2(double T_3, double T_4, double P_6, double v_6, double T_6, int hv, double& q_34conv, double& h_34)
{
    // Missing Variables
    //vector<double> m_P_a;
    //vector<bool> m_GlazingIntact;
    //HTFProperties m_airProps;
    //vector<double> m_D_abs_out;
    //double pi = 3.14159;
    //util::matrix_t<HTFProperties*> m_AnnulusGasMat;
    //vector<double> m_D_glass_in;



    double a, Alpha_34, b, Beta_34, C, C1, Cp_34, Cv_34, Delta, Gamma, k_34, Lambda,
        m, mu_34, n, nu_34, P, Pr_34, P_A1, Ra_D3, Ra_D4, rho_34, T_34, T_36,
        grav, Nu_bar, rho_3, rho_6, mu_36, rho_36, cp_36,
        k_36, nu_36, alpha_36, beta_36, Pr_36, h_36, mu_3, mu_6, k_3, k_6, cp_3, Cp_6, nu_6, nu_3,
        Alpha_3, alpha_6, Re_D3, Pr_3, Pr_6, Natq_34conv, Kineticq_34conv;

    grav = 9.81; //m/s2  gravitation constant

    P_A1 = m_P_a[hv] * 133.322368;  //convert("torr", "Pa")  //[Pa]

    T_34 = (T_3 + T_4) / 2.;  //[C]
    T_36 = (T_3 + T_6) / 2.;  //[C]

    if (!m_GlazingIntact.at(hv)) {

        // Thermophysical Properties for air 
        rho_3 = m_airProps.dens(T_3, P_6);  //[kg/m**3], air is fluid 1.
        rho_6 = m_airProps.dens(T_6, P_6);  //[kg/m**3], air is fluid 1.

        if (v_6 <= 0.1) {
            mu_36 = m_airProps.visc(T_36);  //[N-s/m**2], AIR
            rho_36 = m_airProps.dens(T_36, P_6);  //[kg/m**3], AIR
            cp_36 = m_airProps.Cp(T_36) * 1000.;  //[J/kg-K], AIR
            k_36 = m_airProps.cond(T_36);  //[W/m-K], AIR
            nu_36 = mu_36 / rho_36;  //[m**2/s] kinematic viscosity, AIR
            alpha_36 = k_36 / (cp_36 * rho_36);  //[m**2/s], thermal diffusivity, AIR
            beta_36 = 1.0 / T_36;  //[1/K]
            Ra_D3 = grav * beta_36 * std::abs(T_3 - T_6) * pow(m_D_abs_out[hv], 3) / (alpha_36 * nu_36);

            // Warning Statement if following Nusselt Number correlation is used out of recommended range //
            //If ((Ra_D3 <= 1.e-5) || (Ra_D3 >= 1.e12)) continue
                //CALL WARNING('The result may not be accurate, since 10**(-5) < Ra_D3 < 10**12 does not hold. See Function fq_34conv. Ra_D3 = XXXA1', Ra_D3)

            // Churchill and Chu correlation for natural convection from a long isothermal horizontal cylinder //
            Pr_36 = nu_36 / alpha_36;
            Nu_bar = pow(0.60 + (0.387 * pow(Ra_D3, 0.1667)) / pow(1. + pow(0.559 / Pr_36, 0.5625), 0.2963), 2);
            h_36 = Nu_bar * k_36 / m_D_abs_out[hv];  //[W/m**2-K]//
            q_34conv = h_36 * pi * m_D_abs_out[hv] * (T_3 - T_6);  //[W/m]//
            h_34 = h_36;  //Set output coefficient
        }
        else {

            // Thermophysical Properties for air 
            mu_3 = m_airProps.visc(T_3);  //[N-s/m**2]
            mu_6 = m_airProps.visc(T_6);  //[N-s/m**2]
            k_3 = m_airProps.cond(T_3);  //[W/m-K]
            k_6 = m_airProps.cond(T_6);  //[W/m-K]
            cp_3 = m_airProps.Cp(T_3) * 1000.;  //[J/kg-K]
            Cp_6 = m_airProps.Cp(T_6) * 1000.;  //[J/kg-K]
            nu_6 = mu_6 / rho_6;  //[m**2/s]
            nu_3 = mu_3 / rho_3;  //[m**2/s]
            Alpha_3 = k_3 / (cp_3 * rho_3);  //[m**2/s]
            alpha_6 = k_6 / (Cp_6 * rho_6);  //[m**2/s]
            Re_D3 = v_6 * m_D_abs_out[hv] / nu_6;
            Pr_3 = nu_3 / Alpha_3;
            Pr_6 = nu_6 / alpha_6;

            // Warning Statements if following Nusselt Number correlation is used out of range //
            //if (Re_D3 <= 1) or (Re_D3 >= 10**6) { CALL WARNING('The result may not be accurate, since 1 < Re_D3 < 10**6 does not hold. See Function fq_34conv. Re_D3 = XXXA1', Re_D3)
            //If (Pr_6 <= 0.7) or (Pr_6 >= 500) Then CALL WARNING('The result may not be accurate, since 0.7 < Pr_6 < 500 does not hold. See Function fq_34conv. Pr_6 = XXXA1', Pr_6)

            // Coefficients for external forced convection Nusselt Number correlation (Zhukauskas's correlation) //
            if (Pr_6 <= 10) {
                n = 0.37;
            }
            else {
                n = 0.36;
            }

            if (Re_D3 < 40) {
                C = 0.75;
                m = 0.4;
            }
            else {

                if ((40 <= Re_D3) && (Re_D3 < 1000.)) {
                    C = 0.51;
                    m = 0.5;
                }
                else {
                    if ((1.e3 <= Re_D3) && (Re_D3 < 2.e5)) {
                        C = 0.26;
                        m = 0.6;
                    }
                    else {
                        if ((2.e5 <= Re_D3) && (Re_D3 < 1.e6)) {
                            C = 0.076;
                            m = 0.7;
                        }
                    }
                }
            }

            // Zhukauskas's correlation for external forced convection flow normal to an isothermal cylinder 
            Nu_bar = C * pow(Re_D3, m) * pow(Pr_6, n) * pow(Pr_6 / Pr_3, 0.25);
            h_36 = Nu_bar * k_6 / m_D_abs_out[hv];  //[W/m**2-K]
            q_34conv = h_36 * m_D_abs_out[hv] * pi * (T_3 - T_6);  //[W/m]	
            h_34 = h_36;  //set output coefficient
        }
    }
    else {

        // Thermophysical Properties for gas in annulus space 
        mu_34 = m_AnnulusGasMat.at(hv)->visc(T_34);  //[kg/m-s] 
        Cp_34 = m_AnnulusGasMat.at(hv)->Cp(T_34) * 1000.;  //[J/kg-K]
        Cv_34 = m_AnnulusGasMat.at(hv)->Cv(T_34) * 1000.;  //[J/kg-K]
        rho_34 = m_AnnulusGasMat.at(hv)->dens(T_34, P_A1);  //[kg/m**3]
        k_34 = m_AnnulusGasMat.at(hv)->cond(T_34);  //[W/m-K]

        // Modified Raithby and Hollands correlation for natural convection in an annular space between horizontal cylinders 
        Alpha_34 = k_34 / (Cp_34 * rho_34);  //[m**2/s]//
        nu_34 = mu_34 / rho_34;  //[m**2/s]//
        Beta_34 = 1. / max(T_34, 1.0);  //[1/K]//
        Ra_D3 = grav * Beta_34 * std::abs(T_3 - T_4) * pow(m_D_abs_out[hv], 3) / (Alpha_34 * nu_34);
        Ra_D4 = grav * Beta_34 * std::abs(T_3 - T_4) * pow(m_D_glass_in[hv], 3) / (Alpha_34 * nu_34);
        Pr_34 = nu_34 / Alpha_34;
        Natq_34conv = 2.425 * k_34 * (T_3 - T_4) / pow(1 + pow(m_D_abs_out[hv] / m_D_glass_in[hv], 0.6), 1.25) * pow(Pr_34 * Ra_D3 / (0.861 + Pr_34), 0.25);  //[W/m]//	
        P = m_P_a[hv];  //[mmHg] (note that 1 torr = 1 mmHg by definition)
        C1 = 2.331e-20;  //[mmHg-cm**3/K]//

        // Free-molecular heat transfer for an annular space between horizontal cylinders 
        if (m_AnnulusGasMat.at(hv)->GetFluid() == HTFProperties::Air) { //AIR
            Delta = 3.53e-8;  //[cm]
        }

        if (m_AnnulusGasMat.at(hv)->GetFluid() == HTFProperties::Hydrogen_ideal) { //H2
            Delta = 2.4e-8;  //[cm]
        }

        if (m_AnnulusGasMat.at(hv)->GetFluid() == HTFProperties::Argon_ideal) {  //Argon
            Delta = 3.8e-8;  //[cm]
        }

        Lambda = C1 * T_34 / (P * Delta * Delta);  //[cm]
        Gamma = Cp_34 / Cv_34;
        a = 1.;
        b = (2. - a) / a * (9. * Gamma - 5.) / (2. * (Gamma + 1.));
        h_34 = k_34 / (m_D_abs_out[hv] / 2. * log(m_D_glass_in[hv] / m_D_abs_out[hv]) + b * Lambda / 100. * (m_D_abs_out[hv] / m_D_glass_in[hv] + 1.));  //[W/m**2-K]
        Kineticq_34conv = m_D_abs_out[hv] * pi * h_34 * (T_3 - T_4);  //[W/m]

        // Following compares free-molecular heat transfer with natural convection heat transfer and uses the largest value for heat transfer in annulus 
        if (Kineticq_34conv > Natq_34conv) {
            q_34conv = Kineticq_34conv;  //[W/m]
        }
        else {
            q_34conv = Natq_34conv;  //[W/m]
            h_34 = q_34conv / (m_D_abs_out[hv] * pi * (T_3 - T_4));  //Recalculate the convection coefficient for natural convection
        }
    }

}

//    FUNCTION fq_34rad :	Radiation heat transfer rate between the absorber surface and glazing inner surface
//******************************************************************************************************************************"
//  NOTE: Temperatures input in terms of degrees K
//
//  Author: R.E. Forristall (2003, EES)
//  Implemented and revised:  M.J. Wagner (10/2009)
//                Copyright:  National Renewable Energy Lab (Golden, CO) 2009
//                   note  :  Tested against original EES version
//
//{ 	Radiation heat transfer for a two-surface enclosure.
//
//        Two cases, one if the glazing envelope is intact and one if the glazing is missing or damaged.
//
//        Case 1: Long (infinite) concentric cylinders.
//
//            q' = sigma * PI * D_1 * (T_1^4 - T_2^4) / (1 / EPSILON_1 + (1 - EPSILON_2) / EPSILON_2 * (D_1 / m_D_abs_in))
//
//            Where,
//
//                q' = radiation heat transfer per unit length [W/m]
//                sigma = Stephan-Boltzmann constant [W/m^2-K^4]
//                T_1 = absorber outer surface temperature [K]
//                T_2 = glazing inner surface temperature [K]
//                D_1 = outer absorber diameter [m]
//                m_D_abs_in = inner glazing diameter [m]
//                EPSILON_1 = emissivity of inner surface
//                EPSILON_2 = emissivity of outer surface
//
//        Case 2: Small convex object in a large cavity.
//
//            q' = sigma * PI * D_1 * EPSILON_1 * (T_1^4 - T_2^4)
//}*/
void EvacReceiverModel::FQ_34RAD_v2(double T_3, double T_4, double T_7, double epsilon_abs_v, int hv, double& q_34rad, double& h_34)
{
    // Missing Variables
    //vector<bool> m_GlazingIntact;
    //double pi = 3.14159;
    //vector<double> m_D_abs_out;
    //vector<double> m_D_glass_in;
    //vector<double> m_epsilon_glass;

    double sigma = 5.67e-8, T_ave;
    T_ave = (T_3 + T_4) / 2.;
    if (!m_GlazingIntact.at(hv)) {
        q_34rad = epsilon_abs_v * pi * m_D_abs_out[hv] * sigma * (pow(T_3, 4) - pow(T_7, 4));  //[W/m]
        h_34 = q_34rad / (pi * m_D_abs_out[hv] * (T_3 - T_7));
    }
    else {
        h_34 = sigma * (T_3 * T_3 + T_4 * T_4) * (T_3 + T_4) / (1.0 / epsilon_abs_v + m_D_abs_out[hv] / m_D_glass_in[hv] * (1.0 / m_epsilon_glass[hv] - 1.0));
        q_34rad = pi * m_D_abs_out[hv] * h_34 * (T_3 - T_4);
    }

}

//    FUNCTION fq_56conv :	Convective heat transfer rate between the glazing outer surface and the ambient air
//******************************************************************************************************************************"
//  Author: R.E. Forristall (2003, EES)
//  Implemented and revised:  M.J. Wagner (10/2009)
//                Copyright:  National Renewable Energy Lab (Golden, CO) 2009
//                   note  :  Tested against original EES version
//
//{ 	h6	Heat Transfer Coefficient
//
//    If no wind, then the Churchill and Chu correlation is used. If wind, then the Zhukauskas's correlation is used. These correlations are described above for q_34conv.
//}*/
void EvacReceiverModel::FQ_56CONV_v2(double T_5, double T_6, double P_6, double v_6, int hv, double& q_56conv, double& h_6)
{
    // Missing Variables
    //HTFProperties m_airProps;
    //vector<bool> m_GlazingIntact;
    //vector<double> m_D_glass_out;
    //double g = 9.8;
    //double pi = 3.14159;

    double alpha_5, alpha_6, C, Cp_5, Cp_56, Cp_6, k_5, k_56, k_6, m, mu_5, mu_56, mu_6, n, Nus_6,
        nu_5, nu_6, Pr_5, Pr_6, Re_D5, rho_5, rho_56, rho_6, T_56, Nu_bar,
        nu_56, alpha_56, beta_56, Ra_D5, Pr_56;

    T_56 = (T_5 + T_6) / 2.0;  //[K]

    // Thermophysical Properties for air 
    mu_5 = m_airProps.visc(T_5);  //[kg/m-s]
    mu_6 = m_airProps.visc(T_6);  //[kg/m-s]
    mu_56 = m_airProps.visc(T_56);  //[kg/m-s]
    k_5 = m_airProps.cond(T_5);  //[W/m-K]
    k_6 = m_airProps.cond(T_6);  //[W/m-K]
    k_56 = m_airProps.cond(T_56);  //[W/m-K]
    Cp_5 = m_airProps.Cp(T_5) * 1000.;  //[J/kg-K]
    Cp_6 = m_airProps.Cp(T_6) * 1000.;  //[J/kg-K]
    Cp_56 = m_airProps.Cp(T_56) * 1000.;  //[J/kg-K]
    rho_5 = m_airProps.dens(T_5, P_6);  //[kg/m^3]
    rho_6 = m_airProps.dens(T_6, P_6);  //[kg/m^3]
    rho_56 = m_airProps.dens(T_56, P_6);  //[kg/m^3]

    // if the glass envelope is missing then the convection heat transfer from the glass 
    //envelope is forced to zero by T_5 = T_6 
    if (!m_GlazingIntact.at(hv)) {
        q_56conv = (T_5 - T_6);  //[W/m]
    }
    else {
        if (v_6 <= 0.1) {

            // Coefficients for Churchill and Chu natural convection correlation //
            nu_56 = mu_56 / rho_56;  //[m^2/s]
            alpha_56 = k_56 / (Cp_56 * rho_56);  //[m^2/s]
            beta_56 = 1.0 / T_56;  //[1/K]
            Ra_D5 = g * beta_56 * std::abs(T_5 - T_6) * pow(m_D_glass_out[hv], 3) / (alpha_56 * nu_56);

            // Warning Statement if following Nusselt Number correlation is used out of range //
            //If (Ra_D5 <= 10**(-5)) or (Ra_D5 >= 10**12) Then CALL WARNING('The result may not be accurate, 
            //since 10**(-5) < Ra_D5 < 10**12 does not hold. See Function fq_56conv. Ra_D5 = XXXA1', Ra_D5)

            // Churchill and Chu correlation for natural convection for a horizontal cylinder //
            Pr_56 = nu_56 / alpha_56;
            Nu_bar = pow(0.60 + (0.387 * pow(Ra_D5, 0.1667)) / pow(1.0 + pow(0.559 / Pr_56, 0.5625), 0.2963), 2);
            h_6 = Nu_bar * k_56 / m_D_glass_out[hv];  //[W/m**2-K]
            q_56conv = h_6 * pi * m_D_glass_out[hv] * (T_5 - T_6);  //[W/m]
        }
        else {

            // Coefficients for Zhukauskas's correlation //
            alpha_5 = k_5 / (Cp_5 * rho_5);  //[m**2/s]
            alpha_6 = k_6 / (Cp_6 * rho_6);  //[m**2/s]
            nu_5 = mu_5 / rho_5;  //[m**2/s]
            nu_6 = mu_6 / rho_6;  //[m**2/s]
            Pr_5 = nu_5 / alpha_5;
            Pr_6 = nu_6 / alpha_6;
            Re_D5 = v_6 * m_D_glass_out[hv] * rho_6 / mu_6;

            // Warning Statement if following Nusselt Number correlation is used out of range //
//			if (Pr_6 <= 0.7) or (Pr_6 >= 500) { CALL WARNING('The result may not be accurate, since 0.7 < Pr_6 < 500 does not hold. See Function fq_56conv. Pr_6 = XXXA1', Pr_6)
//			If (Re_D5 <= 1) or (Re_D5 >= 10**6) Then CALL WARNING('The result may not be accurate, since 1 < Re_D5 < 10**6 does not hold. See Function fq_56conv. Re_D5 = XXXA1 ', Re_D5)

            // Zhukauskas's correlation for forced convection over a long horizontal cylinder //
            if (Pr_6 <= 10) {
                n = 0.37;
            }
            else {
                n = 0.36;
            }

            // TB 2023 initialize C and m to be nan
            m = std::numeric_limits<double>::quiet_NaN();
            C = std::numeric_limits<double>::quiet_NaN();

            if (Re_D5 < 40.0) {
                C = 0.75;
                m = 0.4;
            }
            else {
                if ((40.0 <= Re_D5) && (Re_D5 < 1.e3)) {
                    C = 0.51;
                    m = 0.5;
                }
                else {
                    if ((1.e3 <= Re_D5) && (Re_D5 < 2.e5)) {
                        C = 0.26;
                        m = 0.6;
                    }
                    else {
                        if ((2.e5 <= Re_D5) && (Re_D5 < 1.e6)) {
                            C = 0.076;
                            m = 0.7;
                        }
                    }
                }
            }

            Nus_6 = C * pow(Re_D5, m) * pow(Pr_6, n) * pow(Pr_6 / Pr_5, 0.25);
            h_6 = Nus_6 * k_6 / m_D_glass_out[hv];  //[W/m**2-K]
            q_56conv = h_6 * pi * m_D_glass_out[hv] * (T_5 - T_6);  //[W/m]
        }
    }
}

//    FUNCTION fq_cond_bracket:	Heat loss estimate through HCE support bracket
// ******************************************************************************************************************************"
//  Author: R.E. Forristall (2003, EES)
//  Implemented and revised:  M.J. Wagner (10/2009)
//                Copyright:  National Renewable Energy Lab (Golden, CO) 2009
//                   note  :  Tested against original EES version
//*/
double EvacReceiverModel::FQ_COND_BRACKET_v2(double T_3, double T_6, double P_6, double v_6)
{
    // Missing Variables
    //HTFProperties m_airProps;
    //double g = 9.8;

    double P_brac, D_brac, A_CS_brac, T_base, T_brac, T_brac6, mu_brac6, rho_brac6,
        Cp_brac6, nu_brac6, Alpha_brac6, Beta_brac6, Ra_Dbrac, Pr_brac6, Nu_bar, h_brac6,
        mu_brac, mu_6, rho_6, rho_brac, k_6, Cp_brac, nu_6, Cp_6, Nu_brac, Alpha_brac,
        Re_Dbrac, Pr_brac, Pr_6, n, C, m, L_HCE, alpha_6;


    // effective bracket perimeter for convection heat transfer
    P_brac = 0.2032;  //[m]

    // effective bracket diameter (2 x 1in) 
    D_brac = 0.0508;  //[m]

    // minimum bracket cross-sectional area for conduction heat transfer
    A_CS_brac = 0.00016129;  //[m**2]

    // conduction coefficient for carbon steel at 600 K
    double k_brac_steel = 48.0;  //[W/m-K]

    // effective bracket base temperature
    T_base = T_3 - 10.0;  //[C]

    // estimate average bracket temperature 
    T_brac = (T_base + T_6) / 2.0;  //[C]  //NOTE: MJW modified from /3 to /2.. believed to be an error

    // estimate film temperature for support bracket 
    T_brac6 = (T_brac + T_6) / 2.0;  //[C]

    // convection coefficient with and without wind
    if (v_6 <= 0.1) {

        mu_brac6 = m_airProps.visc(T_brac6);  //[N-s/m**2]
        rho_brac6 = m_airProps.dens(T_brac6, P_6);  //[kg/m**3]
        Cp_brac6 = m_airProps.Cp(T_brac6) * 1000.;  //[J/kg-K]
        double k_brac6 = m_airProps.cond(T_brac6);  //[W/m-K]
        nu_brac6 = mu_brac6 / rho_brac6;  //[m**2/s]
        Alpha_brac6 = k_brac6 / (Cp_brac6 * rho_brac6);  //[m**2/s]
        Beta_brac6 = 1.0 / T_brac6;  //[1/K]
        Ra_Dbrac = g * Beta_brac6 * std::abs(T_brac - T_6) * D_brac * D_brac * D_brac / (Alpha_brac6 * nu_brac6);

        // Warning Statement if following Nusselt Number correlation is used out of recommended range 
        //If ((Ra_Dbrac <= 1.e-5)) || (Ra_Dbrac >= 1.e12) Then CALL WARNING('The result may not be accurate, 
        //since 10**(-5) < Ra_Dbrac < 10**12 does not hold. See Function fq_cond_bracket. Ra_Dbrac = XXXA1', Ra_Dbrac)

        // Churchill and Chu correlation for natural convection from a long isothermal horizontal cylinder 
        Pr_brac6 = nu_brac6 / Alpha_brac6;
        Nu_bar = pow(0.60 + (0.387 * pow(Ra_Dbrac, 0.1667)) / pow(1.0 + pow(0.559 / Pr_brac6, 0.5625), 0.2963), 2);
        h_brac6 = Nu_bar * k_brac6 / D_brac;  //[W/m**2-K]
    }
    else {

        // Thermophysical Properties for air 
        mu_brac = m_airProps.visc(T_brac);  //[N-s/m**2]
        mu_6 = m_airProps.visc(T_6);  //[N-s/m**2]
        rho_6 = m_airProps.dens(T_6, P_6);  //[kg/m**3]
        rho_brac = m_airProps.dens(T_brac, P_6);  //[kg/m**3]
        double k_brac = m_airProps.cond(T_brac);  //[W/m-K]
        k_6 = m_airProps.cond(T_6);  //[W/m-K]
        double k_brac6 = m_airProps.cond(T_brac6);  //[W/m-K]
        Cp_brac = m_airProps.Cp(T_brac) * 1000.;  //[J/kg-K]
        Cp_6 = m_airProps.Cp(T_6) * 1000.;  //[J/kg-K]
        nu_6 = mu_6 / rho_6;  //[m**2/s]
        Nu_brac = mu_brac / rho_brac;  //[m**2/s]

        Alpha_brac = k_brac / (Cp_brac * rho_brac * 1000.0);  //[m**2/s]
        alpha_6 = k_6 / (Cp_6 * rho_6 * 1000.0);  //[m**2/s]
        Re_Dbrac = v_6 * D_brac / nu_6;
        Pr_brac = Nu_brac / Alpha_brac;
        Pr_6 = nu_6 / alpha_6;

        // Warning Statements if following Nusselt Correlation is used out of range 
//		if (Re_Dbrac <= 1) or (Re_Dbrac >= 10**6) { CALL WARNING('The result may not be accurate, since 1 < Re_Dbrac < 10**6 does not hold. See Function fq_cond_bracket. Re_Dbrac = XXXA1', Re_Dbrac)
//		If (Pr_6 <= 0.7) or (Pr_6 >= 500) Then CALL WARNING('The result may not be accurate, since 0.7 < Pr_6 < 500 does not hold. See Function fq_cond_bracket. Pr_6 = XXXA1', Pr_6)

        // Coefficients for external forced convection Nusselt Number correlation (Zhukauskas's correlation) 
        if (Pr_6 <= 10.) {
            n = 0.37;
        }
        else {
            n = 0.36;
        }

        if (Re_Dbrac < 40.) {
            C = 0.75;
            m = 0.4;
        }
        else {

            if ((40. <= Re_Dbrac) && (Re_Dbrac < 1.e3)) {
                C = 0.51;
                m = 0.5;
            }
            else {
                if ((1.e3 <= Re_Dbrac) && (Re_Dbrac < 2.e5)) {
                    C = 0.26;
                    m = 0.6;
                }
                else {
                    if ((2.e5 <= Re_Dbrac) && (Re_Dbrac < 1.e6)) {
                        C = 0.076;
                        m = 0.7;
                    }
                }
            }
        }

        // Zhukauskas's correlation for external forced convection flow normal to an isothermal cylinder 
        Nu_bar = C * pow(Re_Dbrac, m) * pow(Pr_6, n) * pow(Pr_6 / Pr_brac, 0.25);
        h_brac6 = Nu_bar * k_brac6 / D_brac;  //[W/m**2-K]

    }

    // estimated conduction heat loss through HCE support brackets / HCE length 
    L_HCE = 4.06;  //[m]
    return sqrt(h_brac6 * P_brac * k_brac_steel * A_CS_brac) * (T_base - T_6) / L_HCE;  //[W/m]

}

//    FUNCTION fk_23:	Absorber conductance
//******************************************************************************************************************************"
//{ Based on linear fit of data from "Alloy Digest, Sourcebook, Stainless Steels"; ASM International, 2000.}
//*/
double EvacReceiverModel::FK_23_v2(double T_2, double T_3, int hv)
{
    double T_23;

    //Absorber materials:
    // (1)   304L
    // (2)   216L
    // (3)   321H
    // (4)   B42 Copper Pipe

    T_23 = (T_2 + T_3) / 2. - 273.15;  //[C]
    return m_AbsorberPropMat.at(hv)->cond(T_23);

}
