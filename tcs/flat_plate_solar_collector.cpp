/*
BSD 3-Clause License

Copyright (c) Alliance for Energy Innovation, LLC. See also https://github.com/NREL/ssc/blob/develop/LICENSE
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


#include <stdexcept>
#include "flat_plate_solar_collector.h"
#include "lib_irradproc.h"

const double kPi = 3.1415926535897;


FlatPlateCollector::FlatPlateCollector()
{
    FRta_ = FRUL_ = iam_ = area_coll_ = heat_capacity_rate_test_ = m_dot_test_ = std::numeric_limits<double>::quiet_NaN();
}

FlatPlateCollector::FlatPlateCollector(const CollectorTestSpecifications &collector_test_specifications)
    :
    FRta_(collector_test_specifications.FRta),
    FRUL_(collector_test_specifications.FRUL),
    iam_(collector_test_specifications.iam),
    area_coll_(collector_test_specifications.area_coll),
    heat_capacity_rate_test_(collector_test_specifications.heat_capacity * collector_test_specifications.m_dot),
    m_dot_test_(collector_test_specifications.m_dot)
{

}

const double FlatPlateCollector::kMDotRated_ = 0.0821;

const double FlatPlateCollector::RatedHeatGain()   // [kWt]
{
    // Calculation taken from UI equation

    double G_T = 1.e3;                  // normal incident irradiance [W/m2]
    double T_inlet_minus_T_amb = 30.;   // [K]

    return area_coll_ * (FRta_*G_T - FRUL_*T_inlet_minus_T_amb) * 1.e-3;
}

const double FlatPlateCollector::RatedMassFlow()
{
    return kMDotRated_;
}

const double FlatPlateCollector::MaxAllowedTemp()   // [C]
{
    return 163.;        // based on rated continuous temperature of high-temp DYN-O-FLO HD glycol
}

const double FlatPlateCollector::MaxMassFlow()   // [kg/s]
{
    // assuming 50/50 propylene glycol, for a specific gravity of 1.041, or 1041 kg/m3
    // Heliodyne states a recommended design flow rate (with 50/50 PG) of 1.25 gal/min (7.886e-5 m3/s) for the GOBI 410
    // this translates to 0.0821 kg/s, compared to the test flow rate of 0.0498 kg/s
    return 3. * kMDotRated_;    // based on published recommendation from Heliodyne to not exceed recommended flow rates by more than (3) times
}

const double FlatPlateCollector::EstimateHeatGain(double POA /*W/m2*/, double T_in /*C*/, double T_amb /*C*/)   // [kWt]
{
    return area_coll_ * (FRta_ * POA - FRUL_ * (T_in - T_amb)) * 1.e-3;
}

const HeatAndTempInOut FlatPlateCollector::HeatGainAndLoss(const TimeAndPosition &time_and_position, const ExternalConditions &external_conditions)  // [W]
{
    Weather weather(external_conditions.weather);
    double ambient_temp(external_conditions.weather.ambient_temp);
    FluidFlow inlet_fluid_flow(external_conditions.inlet_fluid_flow);
    double albedo(external_conditions.albedo);

    PoaIrradianceComponents poa_irradiance_components = IncidentIrradiances(time_and_position, weather, albedo);
    double absorbed_irradiance_over_taualpha_n = AbsorbedIrradianceOverTauAlphaN(time_and_position.collector_orientation, poa_irradiance_components);
    double absorbed_radiant_heat = AbsorbedRadiantHeat(absorbed_irradiance_over_taualpha_n, inlet_fluid_flow, ambient_temp);
    double thermal_heat_loss = ThermalHeatLoss(inlet_fluid_flow, ambient_temp);
    double useful_heat_gain = absorbed_radiant_heat - thermal_heat_loss;

    HeatAndTempInOut heat_and_temp_in_out;
    heat_and_temp_in_out.Q_gain = absorbed_radiant_heat;
    heat_and_temp_in_out.Q_loss = thermal_heat_loss;
    heat_and_temp_in_out.T_in = external_conditions.inlet_fluid_flow.temp;
    heat_and_temp_in_out.T_out = std::numeric_limits<double>::quiet_NaN();
    return heat_and_temp_in_out;
}

const HeatAndTempInOut FlatPlateCollector::HeatFlowsAndOutletTemp(const TimeAndPosition &time_and_position, const ExternalConditions &external_conditions)     // [C]
{
    HeatAndTempInOut heat_and_temp_in_out = HeatGainAndLoss(time_and_position, external_conditions);
    double useful_heat_gain = heat_and_temp_in_out.Q_gain - heat_and_temp_in_out.Q_loss;

    double m_dot = external_conditions.inlet_fluid_flow.m_dot;
    double specific_heat = external_conditions.inlet_fluid_flow.specific_heat;
    double mdotCp_use = m_dot * specific_heat;              // [kg/s * kJ/kg-K]
    double dT_collector = useful_heat_gain / mdotCp_use;

    heat_and_temp_in_out.T_out = external_conditions.inlet_fluid_flow.temp + dT_collector;
    return heat_and_temp_in_out;
}

const double FlatPlateCollector::area_coll()    // [m2]
{
    return area_coll_;
}

void FlatPlateCollector::area_coll(double collector_area /*m2*/)
{
    area_coll_ = collector_area;
}

const CollectorTestSpecifications FlatPlateCollector::TestSpecifications()
{
    CollectorTestSpecifications collector_test_specifications;
    collector_test_specifications.FRta = FRta_;
    collector_test_specifications.FRUL = FRUL_;
    collector_test_specifications.iam = iam_;
    collector_test_specifications.area_coll = area_coll_;
    collector_test_specifications.m_dot = m_dot_test_;
    collector_test_specifications.heat_capacity = heat_capacity_rate_test_ / m_dot_test_;

    return collector_test_specifications;
}

const PoaIrradianceComponents FlatPlateCollector::IncidentIrradiances(const TimeAndPosition &time_and_position,
    const Weather &weather,
    double albedo  /*-*/)   // [W/m2]
{
    double dni = weather.dni;
    double dhi = weather.dhi;
    double ghi = weather.ghi;

    int irrad_mode;
    irrad tt;
    if (std::isfinite(dni) && std::isfinite(dhi)) {
        irrad_mode = 0;     // 0 = beam & diffuse
        tt.set_beam_diffuse(dni, dhi);
    }
    else if (std::isfinite(ghi) && std::isfinite(dni)) {
        irrad_mode = 1;     // 1 = total & beam
        tt.set_global_beam(ghi, dni);
    }
    else if (std::isfinite(ghi) && std::isfinite(dhi)) {
        irrad_mode = 2;     // 2 = total & diffuse
        tt.set_global_diffuse(ghi, dhi);
    }
    else {
        throw std::invalid_argument("FlatPlateCollector: Two of the three irradiance components must be specified.");
    }

    tt.set_location(time_and_position.collector_location.latitude,
        time_and_position.collector_location.longitude,
        time_and_position.collector_location.timezone);

    //double ts_hour = 1.0 / step_per_hour;
    //double delt = instantaneous ? IRRADPROC_NO_INTERPOLATE_SUNRISE_SUNSET : ts_hour;
    double irradproc_no_interpolate_sunrise_sunset = -1.0;      // IRRADPROC_NO_INTERPOLATE_SUNRISE_SUNSET = -1.0;
    double delt = irradproc_no_interpolate_sunrise_sunset;      // 
    tt.set_time(time_and_position.timestamp.tm_year + 1900,     // years since 1900
        time_and_position.timestamp.tm_mon + 1,                 // Jan. = 0
        time_and_position.timestamp.tm_mday,
        time_and_position.timestamp.tm_hour,
        time_and_position.timestamp.tm_min,
        delt);
    int sky_model = 2;      // isotropic=0, hdkr=1, perez=2
    tt.set_sky_model(sky_model, albedo);
    double tilt = time_and_position.collector_orientation.tilt;
    double azimuth = time_and_position.collector_orientation.azimuth;
    tt.set_surface(0, tilt, azimuth, 0, 0, 0, 0, 0, false, 0.0);
    tt.calc();

    double poa_beam, poa_sky_diffuse, poa_ground_reflected_diffuse;
    tt.get_poa(&poa_beam, &poa_sky_diffuse, &poa_ground_reflected_diffuse, 0, 0, 0);
    //double I_incident = (ssc_number_t)(poa_beam + poa_sky_diffuse + poa_ground_reflected_diffuse); // total PoA on surface

    //double solalt, solazi;
    //tt.get_sun(&solazi, 0, &solalt, 0, 0, 0, 0, 0, 0, 0);

    double poa_beam_aoi = 0;
    tt.get_angles(&poa_beam_aoi, 0, 0, 0, 0); // note: angles returned in degrees

    PoaIrradianceComponents poa_irradiance_components;
    poa_irradiance_components.beam_with_aoi.at(0) = poa_beam;
    poa_irradiance_components.beam_with_aoi.at(1) = poa_beam_aoi;
    poa_irradiance_components.sky_diffuse_with_aoi.at(0) = poa_sky_diffuse;
    poa_irradiance_components.sky_diffuse_with_aoi.at(1) = std::numeric_limits<double>::quiet_NaN();
    poa_irradiance_components.ground_reflected_diffuse_with_aoi.at(0) = poa_ground_reflected_diffuse;
    poa_irradiance_components.ground_reflected_diffuse_with_aoi.at(1) = std::numeric_limits<double>::quiet_NaN();

    return poa_irradiance_components;
};

const double FlatPlateCollector::IncidentIrradiance(const TimeAndPosition& time_and_position,
    const Weather& weather,
    double albedo  /*-*/)   // [W/m2]
{
    PoaIrradianceComponents poa_irradiance_components = IncidentIrradiances(time_and_position, weather, albedo);
    double Dni_on_tilted = poa_irradiance_components.beam_with_aoi.at(0);
    double Dhi_on_tilted = poa_irradiance_components.sky_diffuse_with_aoi.at(0);
    double ground_reflected_on_tilted = poa_irradiance_components.ground_reflected_diffuse_with_aoi.at(0);
    
    return Dni_on_tilted + Dhi_on_tilted + ground_reflected_on_tilted;  // POA
}

// Returns the absorbed irradiance divided by transmittance-absorptance product at normal incidence, or: S/(tau-alpha_n)
const double FlatPlateCollector::AbsorbedIrradianceOverTauAlphaN(const CollectorOrientation &collector_orientation,
    const PoaIrradianceComponents &poa_irradiance_components)   // [W/m2]
{
    // calculate transmittance through cover
    double Kta_d = 0.0;
    double Kta_b = 0.0;
    double Kta_g = 0.0;

    // incidence angle modifier (IAM) for beam (D&B eqn 6.17.10 pp 297)
    double aoi_beam = poa_irradiance_components.beam_with_aoi.at(1);
    if (aoi_beam <= 60.0) {
        Kta_b = 1 - iam_ * (1 / cos(aoi_beam*kPi / 180) - 1);
    }
    else if (aoi_beam > 60.0 && aoi_beam <= 90.0) {
        Kta_b = (1 - iam_)*(aoi_beam - 90.0)*kPi / 180;
    }
    if (Kta_b < 0) Kta_b = 0;

    double tilt = collector_orientation.tilt;
    // effective incidence angle for sky diffuse radiation (D&B eqn 5.4.2 pp 215)
    double theta_eff_diffuse = 59.7*kPi / 180 - 0.1388*tilt*kPi / 180 + 0.001497*tilt*kPi / 180 * tilt*kPi / 180;
    double cos_theta_eff_diffuse = cos(theta_eff_diffuse);

    // incidence angle modifier (IAM) for diffuse (D&B eqn 6.17.10 pp 297)
    if (theta_eff_diffuse <= kPi / 3.) {
        Kta_d = 1 - iam_ * (1 / cos_theta_eff_diffuse - 1);
    }
    else if (theta_eff_diffuse > kPi / 3. && theta_eff_diffuse <= kPi / .2) {
        Kta_d = (1 - iam_)*(theta_eff_diffuse - kPi / 2.);
    }
    if (Kta_d < 0) {
        Kta_d = 0;
    }

    // effective incidence angle modifier for ground reflected radiation (D&B eqn 5.4.1 pp 215)
    double theta_eff_ground = 90 * kPi / 180 - 0.5788*tilt*kPi / 180 + 0.002693*tilt*kPi / 180 * tilt*kPi / 180;
    double cos_theta_eff_ground = cos(theta_eff_ground);

    // incidence angle modifier (IAM) for ground reflected radiation (D&B eqn 6.17.10 pp 297)
    if (theta_eff_ground <= kPi / 3) {
        Kta_g = 1 - iam_ * (1 / cos_theta_eff_ground - 1);
    }
    else if (theta_eff_ground > kPi / 3 && theta_eff_ground <= kPi / 2) {
        Kta_g = (1 - iam_)*(theta_eff_ground - kPi / 2.);
    }
    if (Kta_g < 0) {
        Kta_g = 0;
    }

    double beam_shading_factor = 1.0;
    double diffuse_shading_factor = 1.0;
    // TODO - How are shading losses calculated? Why does their setup require a cmod argument? Shading currently ignored.
    //if (shad.fbeam(hour, solalt, solazi, jj, step_per_hour)) {
    //    beam_loss_factor = shad.beam_shade_factor();
    //}
    //diffuse_shading_factor = shad.fdiff();

    // TODO - Why are shading loss factors applied here and not at the incidence irradiance calculation?
    double poa_beam = poa_irradiance_components.beam_with_aoi.at(0);
    double poa_sky_diffuse = poa_irradiance_components.sky_diffuse_with_aoi.at(0);
    double poa_ground_reflected_diffuse = poa_irradiance_components.ground_reflected_diffuse_with_aoi.at(0);
    double s_over_taualpha_n =
        Kta_b * poa_beam * beam_shading_factor +
        Kta_d * poa_sky_diffuse * diffuse_shading_factor +
        Kta_g * poa_ground_reflected_diffuse;

    return s_over_taualpha_n;
}

const double FlatPlateCollector::AbsorbedRadiantHeat(double absorbed_irradiance_over_taualpha_n /*W/m2*/, const  FluidFlow &inlet_fluid_flow, double T_amb /*C*/)    // [kWt]
{
    double m_dot = inlet_fluid_flow.m_dot;
    double specific_heat = inlet_fluid_flow.specific_heat;
    double mdotCp_use = m_dot * specific_heat * 1.e3;       // mass flow rate (kg/s) * Cp_fluid (kJ/kg.K) * 1000 J/kJ

    /* Flow rate corrections to FRta, FRUL (D&B pp 307) */
    double FprimeUL = -heat_capacity_rate_test_ * 1.e3 / area_coll_ * ::log(1 - FRUL_ * area_coll_ / (heat_capacity_rate_test_ * 1.e3) ); // D&B eqn 6.20.4
    double r = (mdotCp_use / area_coll_ * (1 - exp(-area_coll_ * FprimeUL / mdotCp_use))) / FRUL_; // D&B eqn 6.20.3
    double FRta_use = FRta_ * r; // FRta_use = value for this time step 

    double Q_dot_absorbed = area_coll_ * FRta_use* absorbed_irradiance_over_taualpha_n * 1.e-3; // [kWt]    from D&B eqn 6.8.1
    return Q_dot_absorbed;
}

const double FlatPlateCollector::ThermalHeatLoss(const FluidFlow &inlet_fluid_flow, double T_amb /*C*/)  // [kWt]
{
    double T_in = inlet_fluid_flow.temp;
    double m_dot = inlet_fluid_flow.m_dot;
    double specific_heat = inlet_fluid_flow.specific_heat;
    double mdotCp_use = m_dot * specific_heat * 1.e3;       // mass flow rate (kg/s) * Cp_fluid (J/kg.K) * 1000 J/kJ

    double FprimeUL = -heat_capacity_rate_test_ * 1.e3 / area_coll_ * ::log(1 - FRUL_ * area_coll_ / (heat_capacity_rate_test_ * 1.e3) ); // D&B eqn 6.20.4
    double r = (mdotCp_use / area_coll_ * (1 - exp(-area_coll_ * FprimeUL / mdotCp_use))) / FRUL_; // D&B eqn 6.20.3

    double FRUL_use = FRUL_ * r;                                            // FRUL_use = value for this time step
    double Q_dot_losses = area_coll_ * FRUL_use * (T_in - T_amb) * 1.e-3;   // [kWt] from D&B eqn 6.8.1
    return Q_dot_losses;
}



Pipe::Pipe()
{
    pipe_diam_ = pipe_k_ = pipe_insul_ = pipe_length_ = std::numeric_limits<double>::quiet_NaN();
}

Pipe::Pipe(double pipe_diam /*m*/, double pipe_k /*W/m-K*/, double pipe_insul /*m*/, double pipe_length /*m*/)
    : pipe_diam_(pipe_diam),
    pipe_k_(pipe_k),
    pipe_insul_(pipe_insul),
    pipe_length_(pipe_length)
{}

const double Pipe::pipe_od()    // [m]
{
    return pipe_diam_ + pipe_insul_ * 2;
}

const double Pipe::UA_pipe()    // [W/K]
{
    double U_pipe = 2 * pipe_k_ / (pipe_od() * ::log(pipe_od() / pipe_diam_)); //  **TODO** CHECK whether should be pipe_diam*log(pipe_od/pipe_diam) in denominator
    double UA_pipe = U_pipe * kPi * pipe_od() * pipe_length_; // W/'C
    return UA_pipe;
}

const double Pipe::ThermalHeatLoss(double T_in /*C*/, double T_amb /*C*/)   // [kWt]
{
    return UA_pipe()*(T_in - T_amb) * 1.e-3;
}

const double Pipe::T_out(double T_in /*C*/, double T_amb /*C*/, double heat_capacity_rate /*kW/K*/)     // [C]
{
    double thermal_heat_loss = ThermalHeatLoss(T_in, T_amb);
    double T_out = -thermal_heat_loss / heat_capacity_rate + T_in;
    return T_out;
}

HeatExchanger::HeatExchanger(const HxDesignProps& hx_design_props)
    :
    hx_design_props_(hx_design_props),
    C_hx_two_tank_tes()
{
    HTFProperties external_fluid, subsystem_fluid;
    external_fluid.SetFluid(hx_design_props.external_fluid_id);
    subsystem_fluid.SetFluid(hx_design_props.subsystem_fluid_id);
    this->init(
        external_fluid,
        subsystem_fluid,
        hx_design_props.duty * 1.e3 /*W*/,
        hx_design_props.dT_approach,
        hx_design_props.T_in_hot + 273.15 /*K*/,
        hx_design_props.T_out_hot + 273.15 /*K*/);
}

HeatExchanger::HeatExchanger() {
    hx_design_props_ = HxDesignProps();
};

void HeatExchanger::SetHxDesignProps(const HxDesignProps& hx_design_props)
{
    hx_design_props_ = hx_design_props;
    HTFProperties fluid_subsystem = HTFProperties();
    fluid_subsystem.SetFluid(hx_design_props.subsystem_fluid_id);
    HTFProperties fluid_external = HTFProperties();
    fluid_external.SetFluid(hx_design_props.external_fluid_id);
    this->init(
        fluid_subsystem,
        fluid_external,
        hx_design_props.duty * 1.e3,            // [W]
        hx_design_props.dT_approach,
        hx_design_props.T_in_hot + 273.15,      // [K]
        hx_design_props.T_out_hot + 273.15      // [K]
    );    
}

const HxDesignProps* HeatExchanger::GetHxDesignProps() const
{
    return &hx_design_props_;
}

FlatPlateArray::FlatPlateArray()
{
    flat_plate_collector_ = FlatPlateCollector();
    collector_location_ = CollectorLocation();
    collector_orientation_ = CollectorOrientation();
    array_dimensions_ = ArrayDimensions();
    inlet_pipe_ = outlet_pipe_ = Pipe();
}

FlatPlateArray::FlatPlateArray(const FlatPlateCollector &flat_plate_collector, const CollectorLocation &collector_location,
    const CollectorOrientation &collector_orientation, const ArrayDimensions &array_dimensions,
    const Pipe &inlet_pipe, const Pipe &outlet_pipe)
    :
    flat_plate_collector_(flat_plate_collector),
    collector_location_(collector_location),
    collector_orientation_(collector_orientation),
    array_dimensions_(array_dimensions),
    inlet_pipe_(inlet_pipe),
    outlet_pipe_(outlet_pipe)
{

}

FlatPlateArray::FlatPlateArray(const CollectorTestSpecifications &collector_test_specifications, const CollectorLocation &collector_location,
    const CollectorOrientation &collector_orientation, const ArrayDimensions &array_dimensions,
    const Pipe &inlet_pipe, const Pipe &outlet_pipe)
    :
    flat_plate_collector_(collector_test_specifications),
    collector_location_(collector_location),
    collector_orientation_(collector_orientation),
    array_dimensions_(array_dimensions),
    inlet_pipe_(inlet_pipe),
    outlet_pipe_(outlet_pipe)
{

}

void FlatPlateArray::SetHxDesignProps(const HxDesignProps& hx_design_props) {
    heat_exchanger_.SetHxDesignProps(hx_design_props);
}

FluidFlowsAndSystemHeats FlatPlateArray::RunWithHx(tm& timestamp, ExternalConditions& external_conditions, double T_out_target /*C*/)
{
    double T_in_hx_f = external_conditions.inlet_fluid_flow.temp;
    double mdot_external = external_conditions.inlet_fluid_flow.m_dot;
    double T_min_rise = 0.;		// doing away with min temp rise so fpc array is more predictable to controller
    bool fp_array_is_on = false;

    double T_approach_hx = heat_exchanger_.GetHxDesignProps()->dT_approach;
    HTFProperties fluid_external = external_conditions.inlet_fluid_flow.fluid;
    double T_out_fp_target = T_out_target + T_approach_hx;
    double T_in_fp_expected = T_in_hx_f + T_approach_hx;

    double POA = IncidentIrradiance(timestamp, external_conditions);
    double Q_fp_est = EstimateHeatGain(POA, T_in_fp_expected, external_conditions.weather.ambient_temp);		// [kWt]
    double T_f_hx_out, mdot_fp, T_out_fp, Q_gain_fp, Q_loss_fp;

    if (T_out_target - T_in_hx_f < T_min_rise || Q_fp_est <= 0.) {
        fp_array_is_on = false;
        mdot_fp = 0.;
        T_f_hx_out = T_in_hx_f;
        T_out_fp = external_conditions.weather.ambient_temp;       // just a placeholder, not accurate
        Q_gain_fp = 0.;
        Q_loss_fp = 0.;
    }
    else {
        double T_f_hx_avg_est = 0.5 * (T_in_hx_f + T_out_target);
        double T_fp_avg_est = T_f_hx_avg_est + T_approach_hx;
        mdot_fp = mdot_external * fluid_external.Cp(T_f_hx_avg_est + 273.15) / fluid_.Cp(T_fp_avg_est + 273.15);		// choosing mdot_fp so C_R = 1

        C_MEQ__T_in_fp c_eq(T_approach_hx, mdot_fp, T_in_hx_f, T_out_target,
            mdot_external, this, &heat_exchanger_, &fluid_external,
            GetFluid(), &timestamp, &external_conditions);
        C_monotonic_eq_solver c_solver(c_eq);
        c_solver.settings(1.e-3, 75, std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN(), false);

        double T_in_fp_guess_lower = T_in_hx_f;
        double T_in_fp_guess_higher = T_in_hx_f + 2. * T_approach_hx;

        double T_in_fp_solved, tol_solved;
        T_in_fp_solved = tol_solved = std::numeric_limits<double>::quiet_NaN();
        int iter_solved = -1;
        int solver_code = 0;

        try {
            solver_code = c_solver.solve(T_in_fp_guess_lower, T_in_fp_guess_higher, 0., T_in_fp_solved, tol_solved, iter_solved);
        }
        catch (C_csp_exception) {
            throw(C_csp_exception("C_MEQ__mdot_fp -> C_MEQ__T_in_fp received exception from mono equation solver"));
        }

        if (solver_code == C_monotonic_eq_solver::CONVERGED) {
            T_f_hx_out = c_eq.T_f_hx_out_;      // out of HX on the system side, opposite flat plate array
            T_out_fp = c_eq.T_out_fp_;          // out of flat plate array into the HX on the subsystem side
            Q_gain_fp = c_eq.Q_gain_fp_;
            Q_loss_fp = c_eq.Q_loss_fp_;
        }
        else {
            fp_array_is_on = false;
            mdot_fp = 0.;
            T_f_hx_out = T_in_hx_f;
            T_out_fp = external_conditions.weather.ambient_temp;       // just a placeholder, not accurate
            Q_gain_fp = 0.;
            Q_loss_fp = 0.;

            if (solver_code > C_monotonic_eq_solver::CONVERGED && std::abs(tol_solved) <= 0.1) {
                //mpc_csp_solver->error_msg = util::format("At time = %lg the C_MEQ__mdot_fp -> C_MEQ__T_in_fp iteration "
                //    "to find the flat plate cold inlet HTF temperature only reached a convergence = %lg. "
                //    "Check that results at this timestep are not unreasonably biasing total simulation results",
                //    mpc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time / 3600.0, tol_solved);
                //mpc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, mpc_csp_solver->error_msg);
            }
            else {
                //
            }
        }

        // Is there a max PTC inlet temperature?? Currently assuming no.

        if (T_out_fp > MaxAllowedTemp()) {
            //fp_array_is_on = false;
            fp_array_is_on = true;
        }
        else if (T_f_hx_out - T_in_hx_f < T_min_rise) {	// flat plate array isn't heating the system temp. much
            fp_array_is_on = false;
        }
        else {
            fp_array_is_on = true;
        }
    }

    if (!fp_array_is_on) {
        T_f_hx_out = T_in_hx_f;         // bypassing HX and flat plate array
        mdot_fp = 0.;
        Q_gain_fp = 0.;
        Q_loss_fp = 0.;
    }

    FluidFlowsAndSystemHeats fluid_flows_and_system_heats;

    fluid_flows_and_system_heats.fluid_flows.subsystem_side.fluid = *GetFluid();
    fluid_flows_and_system_heats.fluid_flows.subsystem_side.temp = T_out_fp;
    fluid_flows_and_system_heats.fluid_flows.subsystem_side.m_dot = mdot_fp;
    fluid_flows_and_system_heats.fluid_flows.system_side.fluid = external_conditions.inlet_fluid_flow.fluid;
    fluid_flows_and_system_heats.fluid_flows.system_side.temp = T_f_hx_out;
    fluid_flows_and_system_heats.fluid_flows.system_side.m_dot = external_conditions.inlet_fluid_flow.m_dot;
    fluid_flows_and_system_heats.Q_gain_subsystem = Q_gain_fp;
    fluid_flows_and_system_heats.Q_loss_subsystem = Q_loss_fp;

    return fluid_flows_and_system_heats;
}

FluidFlow FlatPlateArray::RunSimplifiedWithHx(tm& timestamp, ExternalConditions& external_conditions)
{
    double T_min_rise = 0.;		// doing away with min temp rise so fpc array is more predictable to controller
    bool fp_array_is_on = false;

    // Get starting temperatures
    const HxDesignProps *hx_design_props = heat_exchanger_.GetHxDesignProps();
    double T_approach_hx = hx_design_props->dT_approach;
    double T_in_hx_external = external_conditions.inlet_fluid_flow.temp;
    double T_in_fp_expected = T_in_hx_external + T_approach_hx;

    // Get solar heat gain
    double POA = IncidentIrradiance(timestamp, external_conditions);
    double Q_fp_est = EstimateHeatGain(POA, T_in_fp_expected, external_conditions.weather.ambient_temp);		// [kWt]

    // Calculate outlet temperature on external side
    double T_avg_external = 0.5 * (hx_design_props->T_in_hot + hx_design_props->T_out_hot) - T_approach_hx;
    double mdot_external = external_conditions.inlet_fluid_flow.m_dot;
    HTFProperties fluid_external = external_conditions.inlet_fluid_flow.fluid;
    double Cp = fluid_external.Cp(T_avg_external + 273.15);
    double T_out_hx_external = Q_fp_est / (mdot_external * fluid_external.Cp(T_avg_external + 273.15)) + T_in_hx_external;

    // Populate outputs
    FluidFlow outlet_fluid_flow;
    outlet_fluid_flow.fluid = external_conditions.inlet_fluid_flow.fluid;
    outlet_fluid_flow.m_dot = external_conditions.inlet_fluid_flow.m_dot;
    outlet_fluid_flow.temp = T_out_hx_external;

    return outlet_fluid_flow;
}

const int FlatPlateArray::ncoll()
{
    return array_dimensions_.num_in_series * array_dimensions_.num_in_parallel;
}

const double FlatPlateArray::area_total()
{
    return flat_plate_collector_.area_coll() * ncoll();
}

void FlatPlateArray::resize_array(ArrayDimensions array_dimensions)
{
    if (array_dimensions.num_in_series <= 0 || array_dimensions.num_in_parallel <= 0) return;

    array_dimensions_ = array_dimensions;
}

void FlatPlateArray::resize_array(double T_in_des_external /*C*/, double dT_design_external /*K*/, double mdot_design_external /*kg/s*/, double dT_approach /*K*/, HTFProperties& fluid_external)
{
    resize_num_in_series(T_in_des_external, dT_design_external, dT_approach);
    resize_num_in_parallel(T_in_des_external, dT_design_external, mdot_design_external, fluid_external);
}

void FlatPlateArray::resize_num_in_series(double T_in_des_external /*C*/, double dT_design_external /*K*/, double dT_approach = 0. /*K*/)
{
    if (!std::isnormal(dT_design_external)) return;

    double Q_des_collector = flat_plate_collector_.RatedHeatGain();                         // [kWt]
    double mdot_des_collector = flat_plate_collector_.RatedMassFlow();                      // [kg/s]
    double T_avg_fp_array = T_in_des_external + 0.5 * dT_design_external + dT_approach;     // [C]
    double Cp_collector = fluid_.Cp(T_avg_fp_array + 273.15);                               // TODO: add dT_approach, but should be close enough
    double dT_collector = Q_des_collector / (mdot_des_collector * Cp_collector);            // [K]
    double exact_fractional_collectors_in_series = dT_design_external / dT_collector;
    if (exact_fractional_collectors_in_series < 1.) {
        array_dimensions_.num_in_series = 1;
    }
    else {
        array_dimensions_.num_in_series = static_cast<int>(std::round(exact_fractional_collectors_in_series));      // std::round() rounds up at halfway point
    }

}

void FlatPlateArray::resize_num_in_parallel(double T_in_des_external /*C*/, double dT_design_external /*K*/, double mdot_design_external /*kg/s*/,
    HTFProperties &fluid_external)
{
    if (!std::isnormal(mdot_design_external) || !std::isnormal(dT_design_external)) return;
    if (mdot_design_external <= 0.) return;

    double T_avg_external = T_in_des_external + 0.5 * dT_design_external;
    double Cp_external = fluid_external.Cp(T_avg_external + 273.15);
    double Q_des_fp_array = mdot_design_external * Cp_external * dT_design_external;                                    // [kWt]
    double Q_des_series_string = array_dimensions_.num_in_series * flat_plate_collector_.RatedHeatGain();               // [kWt]

    double exact_fractional_collectors_in_parallel = Q_des_fp_array / Q_des_series_string;
    if (exact_fractional_collectors_in_parallel < 1.) {
        array_dimensions_.num_in_parallel = 1;
    }
    else {
        array_dimensions_.num_in_parallel = static_cast<int>(std::round(exact_fractional_collectors_in_parallel));      // std::round() rounds up at halfway point
    }
}

ArrayDimensions FlatPlateArray::array_size() const
{
    return array_dimensions_;
}

const double FlatPlateArray::IncidentIrradiance(const tm &timestamp, const ExternalConditions& external_conditions)     // [W/m2] POA
{
    TimeAndPosition time_and_position;
    time_and_position.collector_location = collector_location_;
    time_and_position.collector_orientation = collector_orientation_;
    time_and_position.timestamp = timestamp;
    return flat_plate_collector_.IncidentIrradiance(time_and_position, external_conditions.weather, external_conditions.albedo);
}

const double FlatPlateArray::RatedHeatGain()
{
    return this->ncoll() * flat_plate_collector_.RatedHeatGain();
}

const double FlatPlateArray::RatedMassFlow()
{
    return array_dimensions_.num_in_parallel * flat_plate_collector_.RatedMassFlow();
}

const double FlatPlateArray::MaxAllowedTemp()
{
    return flat_plate_collector_.MaxAllowedTemp();
}

const double FlatPlateArray::MaxMassFlow()
{
    return array_dimensions_.num_in_parallel * flat_plate_collector_.MaxMassFlow();
}

const double FlatPlateArray::EstimateHeatGain(double POA, double T_in, double T_amb)
{
    return this->ncoll() * flat_plate_collector_.EstimateHeatGain(POA, T_in, T_amb);
}


const HeatAndTempInOut FlatPlateArray::HeatFlowsAndOutletTemp(const tm &timestamp, const ExternalConditions &external_conditions)     // [C]
{
    TimeAndPosition time_and_position;
    time_and_position.collector_location = collector_location_;
    time_and_position.collector_orientation = collector_orientation_;
    time_and_position.timestamp = timestamp;
    double T_in = external_conditions.inlet_fluid_flow.temp;
    double T_amb = external_conditions.weather.ambient_temp;
    double m_dot = external_conditions.inlet_fluid_flow.m_dot;
    double specific_heat = external_conditions.inlet_fluid_flow.specific_heat;
    double specific_heat_capacity = m_dot * specific_heat;

    // Inlet pipe
    double T_out_inlet_pipe = inlet_pipe_.T_out(T_in, T_amb, specific_heat_capacity);
    double Q_loss_inlet_pipe = inlet_pipe_.ThermalHeatLoss(T_in, T_amb);                    // [kWt]
    double T_array_in = T_out_inlet_pipe;

    // Collectors
    ExternalConditions external_conditions_to_collector(external_conditions);
    external_conditions_to_collector.inlet_fluid_flow.temp = T_array_in;
    external_conditions_to_collector.inlet_fluid_flow.m_dot = m_dot / array_dimensions_.num_in_parallel;
    double T_array_out = T_array_in;
    double Q_gain_array = 0.;
    double Q_loss_array = 0.;
    for (std::size_t i = 0; i < array_dimensions_.num_in_series; i++) {
        HeatAndTempInOut heat_and_temp_in_out = flat_plate_collector_.HeatFlowsAndOutletTemp(time_and_position, external_conditions_to_collector);
        external_conditions_to_collector.inlet_fluid_flow.temp = heat_and_temp_in_out.T_out;   // to next collector in series
        T_array_out = heat_and_temp_in_out.T_out;                                              // continually overwritten except for last collector
        Q_gain_array += heat_and_temp_in_out.Q_gain * array_dimensions_.num_in_parallel;
        Q_loss_array += heat_and_temp_in_out.Q_loss * array_dimensions_.num_in_parallel;
    }

    // Outlet pipe
    double T_out_outlet_pipe = outlet_pipe_.T_out(T_array_out, T_amb, specific_heat_capacity);
    double Q_loss_outlet_pipe = outlet_pipe_.ThermalHeatLoss(T_array_out, T_amb);           // [kWt]

    HeatAndTempInOut heat_and_temp_in_out;
    heat_and_temp_in_out.Q_gain = Q_gain_array;                                             // [kWt]
    heat_and_temp_in_out.Q_loss = Q_loss_inlet_pipe + Q_loss_array + Q_loss_outlet_pipe;    // [kWt]
    heat_and_temp_in_out.T_in = T_in;
    heat_and_temp_in_out.T_out = T_out_outlet_pipe;

    return heat_and_temp_in_out;
}

void FlatPlateArray::SetFluid(int fluid_id)
{
    fluid_.SetFluid(fluid_id);
}

HTFProperties* FlatPlateArray::GetFluid()
{
    return &fluid_;
}

int C_MEQ__mdot_fp::operator()(double mdot_fp /*kg/s*/, double* diff_T_out_f /*C*/)
{
    C_MEQ__T_in_fp c_eq(T_approach_, mdot_fp, m_T_loop_in_, T_f_hx_out_target_,
        m_m_dot_process_heat_, flat_plate_array_, heat_exchanger_, m_htfProps_,
        flat_plate_htf_, timestamp_, external_conditions_);
    C_monotonic_eq_solver c_solver(c_eq);
    c_solver.settings(1.e-3, 75, std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN(), false);

    double T_in_fp_guess_lower = m_T_loop_in_;
    double T_in_fp_guess_higher = m_T_loop_in_ + 2. * T_approach_;

    double T_in_fp_solved, tol_solved;
    T_in_fp_solved = tol_solved = std::numeric_limits<double>::quiet_NaN();
    int iter_solved = -1;
    int solver_code = 0;

    try {
        solver_code = c_solver.solve(T_in_fp_guess_lower, T_in_fp_guess_higher, 0., T_in_fp_solved, tol_solved, iter_solved);
    }
    catch (C_csp_exception) {
        throw(C_csp_exception("C_MEQ__mdot_fp -> C_MEQ__T_in_fp received exception from mono equation solver"));
    }

    if (solver_code != C_monotonic_eq_solver::CONVERGED) {
        if (solver_code > C_monotonic_eq_solver::CONVERGED && std::abs(tol_solved) <= 0.1) {
            //mpc_csp_solver->error_msg = util::format("At time = %lg the C_MEQ__mdot_fp -> C_MEQ__T_in_fp iteration "
            //    "to find the flat plate cold inlet HTF temperature only reached a convergence = %lg. "
            //    "Check that results at this timestep are not unreasonably biasing total simulation results",
            //    mpc_csp_solver->mc_kernel.mc_sim_info.ms_ts.m_time / 3600.0, tol_solved);
            //mpc_csp_solver->mc_csp_messages.add_message(C_csp_messages::NOTICE, mpc_csp_solver->error_msg);
        }
        else {
            *diff_T_out_f = std::numeric_limits<double>::quiet_NaN();
            return -1;
        }
    }

    T_f_hx_out_ = c_eq.T_f_hx_out_;
    T_out_fp_ = c_eq.T_out_fp_;
    eff_hx_ = c_eq.eff_hx_;
    q_dot_hx_ = c_eq.q_dot_hx_;
    dT_hot_ = c_eq.dT_hot_;
    dT_cold_ = c_eq.dT_cold_;

    if (std::abs(T_f_hx_out_ - T_f_hx_out_target_) < std::abs(T_closest_f_hx_out_iter_ - T_f_hx_out_target_) &&
        //T_f_hx_out_ <= T_f_hx_out_target_ &&
        T_out_fp_ <= flat_plate_array_->MaxAllowedTemp())
    {
        T_closest_f_hx_out_iter_ = T_f_hx_out_;
        T_out_fp_at_T_closest_iter_ = T_out_fp_;
        mdot_fp_at_T_closest_iter_ = mdot_fp;
    }
    *diff_T_out_f = T_f_hx_out_ - T_f_hx_out_target_;            //[C]

    return 0;
}

int C_MEQ__T_in_fp::operator()(double T_in_fp /*C*/, double* diff_T_in_fp /*C*/)
{
    // The outer MEQ that calls this one provides mdot_fp_
    // This MEQ converges on T_in_fp for that given mdot_fp_

    double T_out_fp_expected = T_f_hx_out_target_ + T_approach_;
    if (T_in_fp > T_out_fp_expected) {
        *diff_T_in_fp = std::numeric_limits<double>::quiet_NaN();
        return -1;
    }

    double POA = flat_plate_array_->IncidentIrradiance(*timestamp_, *external_conditions_);
    double Q_fp_est = flat_plate_array_->EstimateHeatGain(POA, T_in_fp, external_conditions_->weather.ambient_temp);	// [kWt]

    // This should be verified before calling the MEQs; this is a 'just-in-case'
    if (Q_fp_est <= 0.) {
        *diff_T_in_fp = std::numeric_limits<double>::quiet_NaN();
        return -2;
    }

    double T_avg_guess = 0.5 * (m_T_loop_in_ + T_f_hx_out_target_) + T_approach_;
    double T_out_fp_guess = Q_fp_est / (mdot_fp_ * flat_plate_htf_->Cp(T_avg_guess + 273.15)) + T_in_fp;		// Cp is in [kJ/kg-K]
    external_conditions_->inlet_fluid_flow.m_dot = mdot_fp_;
    external_conditions_->inlet_fluid_flow.specific_heat = flat_plate_htf_->Cp(0.5 * (T_in_fp + T_out_fp_guess) + 273.15);
    external_conditions_->inlet_fluid_flow.temp = T_in_fp;
    HeatAndTempInOut heat_and_temp_in_out = flat_plate_array_->HeatFlowsAndOutletTemp(*timestamp_, *external_conditions_);
    T_out_fp_ = heat_and_temp_in_out.T_out;
    Q_gain_fp_ = heat_and_temp_in_out.Q_gain;
    Q_loss_fp_ = heat_and_temp_in_out.Q_loss;

    // If flat plates are cooling the htf
    if (T_out_fp_ < T_in_fp) {
        *diff_T_in_fp = std::numeric_limits<double>::quiet_NaN();
        return -3;
    }

    double T_in_fp_calcd, eff, q_dot_hx;
    heat_exchanger_->solve(
        m_T_loop_in_ + 273.15,			 // [K]    heat exchanger in from 'field' (process heat)
        m_m_dot_process_heat_,			 // [kg/s] mass flow from process heat
        T_out_fp_ + 273.15,				 // [K]	   outlet temperature of flat plates
        mdot_fp_,						 // [kg/s] mass flow through entire flat plate array (all loops)
        // Outputs:
        T_f_hx_out_,				     // [K]    hot temperature out of HX on 'field' side (to PTCs)
        T_in_fp_calcd,  				 // [K]    cold temperature out of HX on 'storage' side (back to FPCs)
        eff,							 // [-]    heat exchanger effectiveness
        q_dot_hx						 // [MWt]  heat flow across heat exchanger from one fluid to the other
    );

    T_f_hx_out_ -= 273.15;
    T_in_fp_calcd -= 273.15;
    q_dot_hx *= 1.e3;

    eff_hx_ = eff;
    q_dot_hx_ = q_dot_hx;
    dT_hot_ = T_out_fp_ - T_f_hx_out_;
    dT_cold_ = T_in_fp_calcd - m_T_loop_in_;

    *diff_T_in_fp = T_in_fp - T_in_fp_calcd;

    return 0;
}
