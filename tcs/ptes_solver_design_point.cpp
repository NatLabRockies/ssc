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


#include "ptes_solver_design_point.h"
#include <cmath>
#include <algorithm>
using std::string;

/// <summary>
/// Construct Fluid Model based on a designated Fluid Type
/// </summary>
/// <param name="fluid">Fluid Type</param>
FluidMaterialProp::FluidMaterialProp(FluidType fluid_type)
{
    SetFluid(fluid_type);
}

/// <summary>
/// Constructor for Custom Hot or Cold Storage Fluid
/// Only uses Cp and Rho
/// </summary>
/// <param name="cp"></param>
/// <param name="rho"></param>
/// <param name="T0"></param>
/// <param name="P0"></param>
FluidMaterialProp::FluidMaterialProp(double cp, double rho, double T0, double P0)
    :
    cp_(cp), rho_(rho), T0_(T0), P0_(P0), is_compressible_(false),
    cv_(0), gam_(0), R_(0), mu_(0), mu0_(0), Smu_(0)
{
}

/// <summary>
/// Set Fluid Materials Properties based on Fluid Type
/// </summary>
/// <param name="fluid_type"></param>
void FluidMaterialProp::SetFluid(FluidType fluid_type)
{
    switch (fluid_type)
    {
        case(FluidType::kNitrogen):
            this->name_ = "Nitrogen";
            this->cp_ = 1041.3;
            this->cv_ = 743.2;
            this->rho_ = 1.123;
            this->gam_ = this->cp_ / this->cv_;
            this->R_ = this->cp_ - this->cv_;
            this->mu0_ = 1.663e-5;
            this->T0_ = 273;
            this->P0_ = 1e5;
            this->Smu_ = 107;
            this->is_compressible_ = true;
            break;
        case(FluidType::kArgon):
            this->name_ = "Argon";
            this->cp_ = 521.5;
            this->cv_ = 312.4;
            this->rho_ = 1.603;
            this->gam_ = this->cp_ / this->cv_;
            this->R_ = this->cp_ - this->cv_;
            this->mu0_ = 2.125e-5;
            this->T0_ = 273;
            this->P0_ = 1e5;
            this->Smu_ = 114;
            this->is_compressible_ = true;
            break;
        case(FluidType::kHydrogen):
            this->name_ = "Hydrogen";
            this->cp_ = 14312.8;
            this->cv_ = 10186.4;
            this->rho_ = 0.0807;
            this->gam_ = this->cp_ / this->cv_;
            this->R_ = this->cp_ - this->cv_;
            this->mu0_ = 8.411e-5;
            this->T0_ = 273;
            this->P0_ = 1e5;
            this->Smu_ = 97;
            this->is_compressible_ = true;
            break;
        case(FluidType::kHelium):
            this->name_ = "Helium";
            this->cp_ = 5193.2;
            this->cv_ = 3116.1;
            this->rho_ = 0.1603;
            this->gam_ = this->cp_ / this->cv_;
            this->R_ = this->cp_ - this->cv_;
            this->mu0_ = 8.411e-5;
            this->T0_ = 273;
            this->P0_ = 1e5;
            this->Smu_ = 97;
            this->is_compressible_ = true;
            break;
        case(FluidType::kAir):
            this->name_ = "Air";
            this->cp_ = 1131.9;
            this->cv_ = 844.5;
            this->rho_ = 1.276;
            this->gam_ = this->cp_ / this->cv_;
            this->R_ = this->cp_ - this->cv_;
            this->mu0_ = 1.716e-5;
            this->T0_ = 273;
            this->P0_ = 1e5;
            this->Smu_ = 111;
            this->is_compressible_ = true;
            break;

            // Hot Storage
        case(FluidType::kNitrateSalt):
            this->name_ = "Nitrate Salt";
            this->cp_ = 2000;
            this->cv_ = this->cp_; // assume incompressible
            this->rho_ = 1700;
            this->gam_ = 0;
            this->R_ = 0;
            this->mu_ = 1e-4;
            this->T0_ = 273;
            this->P0_ = 1e5;
            this->is_compressible_ = false;
            break;
        case(FluidType::kChlorideSalt):
            this->name_ = "Chloride Salt";
            this->cp_ = 2000;
            this->cv_ = this->cp_; // assume incompressible
            this->rho_ = 800;
            this->gam_ = 0;
            this->R_ = 0;
            this->mu_ = 1e-4;
            this->T0_ = 273;
            this->P0_ = 1e5;
            this->is_compressible_ = false;
            break;

            // Cold Storage
        case(FluidType::kGlycol):
            this->name_ = "Glycol";
            this->cp_ = 2000;
            this->cv_ = this->cp_; // assume incompressible
            this->rho_ = 800;
            this->gam_ = 0;
            this->R_ = 0;
            this->mu_ = 1e-4;
            this->T0_ = 273;
            this->P0_ = 1e5;
            this->is_compressible_ = false;
            break;
        case(FluidType::kMethanol):
            this->name_ = "Methanol";
            this->cp_ = 2000;
            this->cv_ = this->cp_; // assume incompressible
            this->rho_ = 800;
            this->gam_ = 0;
            this->R_ = 0;
            this->mu_ = 1e-4;
            this->T0_ = 273;
            this->P0_ = 1e5;
            this->is_compressible_ = false;
            break;

            // Rejection Air
        case(FluidType::kRejectionAir):
            this->name_ = "Rejection Air";
            this->cp_ = 1000;
            this->cv_ = 718;
            this->gam_ = this->cp_ / this->cv_;
            this->R_ = this->cp_ - this->cv_;
            this->rho_ = 1.225;           
            this->mu0_ = 1.716e-5;
            this->T0_ = 273;
            this->P0_ = 1e5;
            this->Smu_ = 111;
            this->is_compressible_ = true;
            break;

        default:
            break;
    }
}

/// <summary>
/// Relate FluidType to Strings 
/// </summary>
FluidMaterialProp::map FluidMaterialProp::map_ = {
    {"Nitrogen", FluidType::kNitrogen},
    {"Argon", FluidType::kArgon},
    {"Hydrogen", FluidType::kHydrogen},
    {"Helium", FluidType::kHelium},
    {"Air", FluidType::kAir},
    {"NitrateSalt", FluidType::kNitrateSalt},
    {"Glycol", FluidType::kGlycol},
    {"Methanol", FluidType::kMethanol}
};

/// <summary>
/// Constructor for State Information, using Fluid Type
/// </summary>
/// <param name="num_states_charge"></param>
/// <param name="num_states_discharge"></param>
/// <param name="fluid_type"></param>
FluidState::FluidState(int num_states_charge, int num_states_discharge, FluidType fluid_type)
    :
    FluidState(num_states_charge, num_states_discharge, FluidMaterialProp(fluid_type))
{
}

/// <summary>
/// Constructor for State Information, using FluidMaterialProp
/// </summary>
/// <param name="num_states_charge"></param>
/// <param name="num_states_discharge"></param>
/// <param name="fluid"></param>
FluidState::FluidState(int num_states_charge, int num_states_discharge, FluidMaterialProp fluid)
    :
    num_states_charge_(num_states_charge),
    num_states_discharge_(num_states_discharge),
    fluid_material_(fluid),
    mdot_charge_(0),
    mdot_discharge_(0)
{
    this->temp_charge_.resize(num_states_charge_, 0);
    this->temp_discharge_.resize(num_states_discharge_, 0);
    this->pressure_charge_.resize(num_states_charge_, 0);
    this->pressure_discharge_.resize(num_states_discharge_, 0);
    this->h_charge_.resize(num_states_charge_, 0);
    this->h_discharge_.resize(num_states_discharge_, 0);
    this->s_charge_.resize(num_states_charge_, 0);
    this->s_discharge_.resize(num_states_discharge_, 0);
    this->rho_charge_.resize(num_states_charge_, 0);
    this->rho_discharge_.resize(num_states_discharge_, 0);
    this->mu_charge_.resize(num_states_charge_, 0);
    this->mu_discharge_.resize(num_states_discharge_, 0);
}

/// <summary>
/// Constructor
/// </summary>
/// <param name="params">PTES System Parameters</param>
/// <param name="working_fluid_type">Working Fluid Type</param>
/// <param name="hot_fluid_type">Hot Fluid Type</param>
/// <param name="cold_fluid_type">Cold Fluid Type</param>
PTESDesignPoint::PTESDesignPoint(const PTESSystemParam params,
    const FluidType working_fluid_type,
    const FluidType hot_fluid_type,
    const FluidType cold_fluid_type)
    :
    PTESDesignPoint(params, FluidMaterialProp(working_fluid_type), FluidMaterialProp(hot_fluid_type), FluidMaterialProp(cold_fluid_type))
{
}

PTESDesignPoint::PTESDesignPoint(PTESSystemParam params, FluidMaterialProp working_fluid, FluidMaterialProp hot_fluid, FluidMaterialProp cold_fluid)
    :
    kSystemParams(params),
    working_fluid_state_(8, 9, working_fluid),
    hot_fluid_state_(2, 2, hot_fluid),
    cold_fluid_state_(2, 2, cold_fluid),
    rejection_air1_state_(2, 2, FluidType::kRejectionAir),
    rejection_air2_state_(2, 2, FluidType::kRejectionAir)
{
}

/// <summary>
/// Simulate PTES Charge Cycle
/// </summary>
void PTESDesignPoint::Charge()
{
    // Assign Ref Variables to Temp for ease of use
    double& T1 = working_fluid_state_.temp_charge_[0];
    double& T2 = working_fluid_state_.temp_charge_[1];
    double& T3 = working_fluid_state_.temp_charge_[2];
    double& T4 = working_fluid_state_.temp_charge_[3];
    double& T5 = working_fluid_state_.temp_charge_[4];
    double& T6 = working_fluid_state_.temp_charge_[5];
    double& T7 = working_fluid_state_.temp_charge_[6];
    double& T8 = working_fluid_state_.temp_charge_[7];

    double& TH1 = hot_fluid_state_.temp_charge_[0];
    double& TH2 = hot_fluid_state_.temp_charge_[1];
    double& PH1 = hot_fluid_state_.pressure_charge_[0];
    double& PH2 = hot_fluid_state_.pressure_charge_[1];

    double& TC1 = cold_fluid_state_.temp_charge_[0];
    double& TC2 = cold_fluid_state_.temp_charge_[1];
    double& PC1 = cold_fluid_state_.pressure_charge_[0];
    double& PC2 = cold_fluid_state_.pressure_charge_[1];

    double& TA1 = rejection_air1_state_.temp_charge_[0];
    double& TA2 = rejection_air1_state_.temp_charge_[1];
    double& PA1 = rejection_air1_state_.pressure_charge_[0];
    double& PA2 = rejection_air1_state_.pressure_charge_[1];

    double& mdot_WF = working_fluid_state_.mdot_charge_;
    double& mdot_HF = hot_fluid_state_.mdot_charge_;
    double& mdot_CF = cold_fluid_state_.mdot_charge_;

    double& P1 = working_fluid_state_.pressure_charge_[0];
    double& P2 = working_fluid_state_.pressure_charge_[1];
    double& P3 = working_fluid_state_.pressure_charge_[2];
    double& P4 = working_fluid_state_.pressure_charge_[3];
    double& P5 = working_fluid_state_.pressure_charge_[4];
    double& P6 = working_fluid_state_.pressure_charge_[5];
    double& P7 = working_fluid_state_.pressure_charge_[6];
    double& P8 = working_fluid_state_.pressure_charge_[7];

    // Collect Relevant System Parameters
    double hx_eff = this->kSystemParams.hx_eff;  // hx effectiveness
    double eta = this->kSystemParams.eta; // polytropic efficiency of compressors and expanders
    double ploss_working = this->kSystemParams.ploss_working; // Fractional pressure loss in each heat exchanger
    double ploss_liquid = this->kSystemParams.ploss_liquid; // Fractional pressure loss (liquid)
    double ploss_air = this->kSystemParams.ploss_air; // Fractional pressure loss (air)
    double T0 = this->kSystemParams.T0; // Ambient Temperature, K
    double P0 = this->kSystemParams.P0; // Ambient Pressure, Pa
    P1 = this->kSystemParams.P1; // Lowest Pressure in cycle, Pa
    T1 = this->kSystemParams.T_compressor_inlet; // Charging compressor inlet temperature, K
    T2 = this->kSystemParams.T_compressor_outlet; // Charging compressor outlet temperature, K
    mdot_WF = 1; // Working Fluid Mass Flow Rate
        
    // Calculate Compressor Outlet Pressure
    double wf_gam = working_fluid_state_.fluid_material_.GetGam();
    double p2p1_pressure_ratio = pow(T2 / T1, ((eta * wf_gam) / (wf_gam - 1)));
    P2 = P1 * p2p1_pressure_ratio;

    // Solve Recuperator Temps using HX Effectiveness (T3) and Energy Balance (T4)
    T7 = T0;
    T3 = (1.0 / hx_eff) * (T1 - T7) + T7;
    T4 = T7 + T3 - T1;

    // Solve Hot Tank Temps using HX Effectiveness (TH2)
    TH1 = T1;
    TH2 = TH1 + hx_eff * (T2 - TH1);

    // Solve Hot Mass Flow Rate with Energy Balance
    double cp_WF = working_fluid_state_.fluid_material_.GetCp();
    double cp_HF = hot_fluid_state_.fluid_material_.GetCp();
    mdot_HF = (mdot_WF * cp_WF) * (T2 - T3) / (cp_HF * (TH2 - TH1));

    // Solve Hot HX Pressure Loss
    PH1 = P0;
    PH2 = PH1 * (1.0 - ploss_liquid);

    // Solve Working Fluid Presure Losses in Hot HX and Recuperator
    P3 = P2 * (1.0 - ploss_working);
    P4 = P3 * (1.0 - ploss_working);

    // Heat Rejection Pressure Loss
    P5 = P4 * (1.0 - ploss_working);

    // Heat Rejection Temperatures
    TA1 = T0;
    T5 = T4 - hx_eff * (T4 - TA1);

    TA2 = TA1 + (1.0 / this->kSystemParams.alpha) * (T4 - T5);

    // Heat Rejection Air Mass Flow Rate
    double cp_A = rejection_air1_state_.fluid_material_.GetCp();
    rejection_air1_state_.mdot_charge_ = (mdot_WF * cp_WF * (T4 - T5)) / (cp_A * (TA2 - TA1));

    // Heat Rejection Air Pressure Loss
    PA1 = P0;
    PA2 = PA1 * (1.0 - ploss_air);

    // Expansion Inlet and Outlet Pressure
    double p5p6_pressure_ratio = (P2 / P1) * pow((1.0 - ploss_working), 5.0);
    P6 = P5 / p5p6_pressure_ratio;

    // Expansion Outlet Temperature
    double T5T6_temp_ratio = pow(P5 / P6, (((wf_gam - 1.0) * eta) / wf_gam));
    T6 = T5 / T5T6_temp_ratio;

    // Cold Heat Exchanger
    P7 = P6 * (1.0 - ploss_working);
    TC1 = T6 - (1.0 / hx_eff) * (T6 - T7);
    TC2 = TC1 + T6 - T7;

    // Cold Heat Exchanger Mass Flow Rate
    double cp_CF = cold_fluid_state_.fluid_material_.GetCp();
    mdot_CF = ((mdot_WF * cp_WF) / cp_CF) * ((T6 - T7) / (TC2 - TC1));

    // Cold Heat Exchanger Pressures
    PC1 = P0;
    PC2 = PC1 * (1.0 - ploss_liquid);

    // Close Cycle
    T8 = T1;
    P8 = P7 * (1.0 - ploss_working);

    // Calculate Enthalpy and Entropys
    
    for (auto state : { working_fluid_state_, hot_fluid_state_, cold_fluid_state_, rejection_air1_state_, rejection_air2_state_ })
    {
        state.h_charge_ = CalculateEnthalpy(state.temp_charge_, state.fluid_material_);
        state.s_charge_ = CalculateEntropy(state.temp_charge_, state.pressure_charge_, state.fluid_material_);
        state.rho_charge_ = CalculateDensity(state.temp_charge_, state.pressure_charge_, state.fluid_material_);
        state.mu_charge_ = CalculateViscosity(state.temp_charge_, state.fluid_material_);
    }
    
    has_charged_ = true;
    
}

/// <summary>
/// Simulate PTES Discharge Cycle
/// </summary>
void PTESDesignPoint::Discharge()
{
    // Check if already charged
    if (this->has_charged_ == false)
        return;

    // Assign Ref Variables to Temp for ease of use
    double& T5_C = working_fluid_state_.temp_charge_[4];

    double& T1_D = working_fluid_state_.temp_discharge_[0];
    double& T2_D = working_fluid_state_.temp_discharge_[1];
    double& T3_D = working_fluid_state_.temp_discharge_[2];
    double& T4_D = working_fluid_state_.temp_discharge_[3];
    double& T5_D = working_fluid_state_.temp_discharge_[4];
    double& T6_D = working_fluid_state_.temp_discharge_[5];
    double& T7_D = working_fluid_state_.temp_discharge_[6];
    double& T8_D = working_fluid_state_.temp_discharge_[7];
    double& T9_D = working_fluid_state_.temp_discharge_[8];

    double& P1_D = working_fluid_state_.pressure_discharge_[0];
    double& P2_D = working_fluid_state_.pressure_discharge_[1];
    double& P3_D = working_fluid_state_.pressure_discharge_[2];
    double& P4_D = working_fluid_state_.pressure_discharge_[3];
    double& P5_D = working_fluid_state_.pressure_discharge_[4];
    double& P6_D = working_fluid_state_.pressure_discharge_[5];
    double& P7_D = working_fluid_state_.pressure_discharge_[6];
    double& P8_D = working_fluid_state_.pressure_discharge_[7];
    double& P9_D = working_fluid_state_.pressure_discharge_[8];

    double& TH1_C = hot_fluid_state_.temp_charge_[0];
    double& TH2_C = hot_fluid_state_.temp_charge_[1];
    double& TC1_C = cold_fluid_state_.temp_charge_[0];
    double& TC2_C = cold_fluid_state_.temp_charge_[1];

    double& TH1_D = hot_fluid_state_.temp_discharge_[0];
    double& TH2_D = hot_fluid_state_.temp_discharge_[1];
    double& TC1_D = cold_fluid_state_.temp_discharge_[0];
    double& TC2_D = cold_fluid_state_.temp_discharge_[1];
    double& TA11_D = rejection_air1_state_.temp_discharge_[0];
    double& TA12_D = rejection_air1_state_.temp_discharge_[1];
    double& TA21_D = rejection_air2_state_.temp_discharge_[0];
    double& TA22_D = rejection_air2_state_.temp_discharge_[1];

    double& PA11_D = rejection_air1_state_.pressure_discharge_[0];
    double& PA12_D = rejection_air1_state_.pressure_discharge_[1];
    double& PA21_D = rejection_air2_state_.pressure_discharge_[0];
    double& PA22_D = rejection_air2_state_.pressure_discharge_[1];

    double& PH1_D = hot_fluid_state_.pressure_discharge_[0];
    double& PH2_D = hot_fluid_state_.pressure_discharge_[1];
    double& PC1_D = cold_fluid_state_.pressure_discharge_[0];
    double& PC2_D = cold_fluid_state_.pressure_discharge_[1];

    double& mdot_HF_D = hot_fluid_state_.mdot_discharge_;
    double& mdot_CF_D = cold_fluid_state_.mdot_discharge_;

    double& mdot_WF_D = working_fluid_state_.mdot_discharge_;

    // Collect Relevant System Variables
    double hx_eff = this->kSystemParams.hx_eff; // hx effectiveness
    double eta = this->kSystemParams.eta; // polytropic efficiency of compressors and expanders    
    double ploss_working = this->kSystemParams.ploss_working; // Fractional pressure loss in each heat exchanger
    double ploss_air = this->kSystemParams.ploss_air; // Fractional pressure loss (air)
    double ploss_liquid = this->kSystemParams.ploss_liquid; // Fractional pressure loss (liquid)
    double T0 = this->kSystemParams.T0; // Ambient Temperature, K
    double P0 = this->kSystemParams.P0; // Ambient Pressure, Pa
    P1_D = this->kSystemParams.P1; // Lowest Pressure in cycle, Pa
    mdot_WF_D = 1; // Mass Flow Rate of Working Fluid during Discharge

    // Hot Heat Exchanger (returns to original temperature)
    TH1_D = TH2_C;
    TH2_D = TH1_C;
    mdot_HF_D = hot_fluid_state_.mdot_charge_; // Same flow rate as during charge
    double cp_WF = working_fluid_state_.fluid_material_.GetCp();
    double cp_HF = hot_fluid_state_.fluid_material_.GetCp();
    double mdotcp_min = std::min(mdot_WF_D * cp_WF, mdot_HF_D * cp_HF);
    T4_D = TH1_D - (mdot_HF_D * cp_HF) * ((TH1_D - TH2_D) / (hx_eff * mdotcp_min));
    T5_D = T4_D + (mdot_HF_D * cp_HF) * ((TH1_D - TH2_D) / (mdot_WF_D * cp_WF));
    PH1_D = P0;
    PH2_D = PH1_D * (1.0 - ploss_liquid);

    // Recuperator
    T3_D = T5_C;
    T6_D = T3_D + (1.0 / hx_eff) * (T4_D - T3_D);
    T7_D = T6_D - T4_D + T3_D;

    // Expander
    double gam_wf = working_fluid_state_.fluid_material_.GetGam();
    double p5p6_ratio = pow(T5_D / T6_D, gam_wf / (eta * (gam_wf - 1)));
    P6_D = P1_D / pow(1.0 - ploss_working, 3.0);
    P5_D = p5p6_ratio * P6_D;

    // Cold Heat Exchanger
    double cp_CF = cold_fluid_state_.fluid_material_.GetCp();
    TC1_D = TC2_C; // Return to original temps
    TC2_D = TC1_C;
    T8_D = TC1_D + (1.0 / hx_eff) * (TC2_D - TC1_D); // ASSUME [mdot cp] WF == [mdot cp] CF
    T1_D = T8_D - TC2_D + TC1_D;
    mdot_CF_D = ((mdot_WF_D * cp_WF) * (T8_D - T1_D)) / (cp_CF * (TC2_D - TC1_D));
    PC1_D = P0;
    PC2_D = PC1_D * (1.0 - ploss_liquid);

    // Compressor
    double p2p1_ratio = p5p6_ratio / pow(1.0 - ploss_working, 6.0);
    T2_D = T1_D * pow(p2p1_ratio, (gam_wf - 1.0) / (gam_wf * eta));
    P2_D = P1_D * p2p1_ratio;

    // Heat Rejection 1
    TA11_D = T0;
    TA12_D = TA11_D + (1.0 / this->kSystemParams.alpha) * hx_eff * (T2_D - TA11_D);

    double cp_air = rejection_air1_state_.fluid_material_.GetCp();
    rejection_air1_state_.mdot_discharge_ = (mdot_WF_D * cp_WF) * (T2_D - T3_D) / (cp_air * (TA12_D - TA11_D));
    PA11_D = P0;
    PA12_D = PA11_D * (1.0 - ploss_air);

    // Heat Rejection 2
    TA21_D = T0;
    TA22_D = TA21_D + (hx_eff / this->kSystemParams.alpha) * (T7_D - TA21_D);

    double cp_air2 = rejection_air2_state_.fluid_material_.GetCp();
    rejection_air2_state_.mdot_discharge_ = (mdot_WF_D * cp_WF) * (T7_D - T8_D) / (cp_air2 * (TA22_D - TA21_D));
    PA21_D = P0;
    PA22_D = PA21_D * (1.0 - ploss_air);

    // Remaining Pressures
    P3_D = P2_D * (1.0 - ploss_working);
    P4_D = P3_D * (1.0 - ploss_working);
    P7_D = P6_D * (1.0 - ploss_working);
    P8_D = P7_D * (1.0 - ploss_working);
    P9_D = P8_D * (1.0 - ploss_working);

    // Complete Circuit
    T9_D = T1_D;

    // Calculate Enthalpy and Entropys
    for (auto state : { working_fluid_state_, hot_fluid_state_, cold_fluid_state_, rejection_air1_state_, rejection_air2_state_ })
    {
        state.h_discharge_ = CalculateEnthalpy(state.temp_discharge_, state.fluid_material_);
        state.s_discharge_ = CalculateEntropy(state.temp_discharge_, state.pressure_discharge_, state.fluid_material_);
        state.rho_discharge_ = CalculateDensity(state.temp_discharge_, state.pressure_discharge_, state.fluid_material_);
        state.mu_discharge_ = CalculateViscosity(state.temp_discharge_, state.fluid_material_);
    }

    has_discharged_ = true;
}

/// <summary>
/// Calculate Cycle Performance
/// </summary>
void PTESDesignPoint::Performance()
{
    // Check if charged and discharged
    if (this->has_charged_ == false || this->has_discharged_ == false)
        return;

    // Collect Simplified Names
    FluidState& wf = this->working_fluid_state_;
    FluidState& hf = this->hot_fluid_state_;
    FluidState& cf = this->cold_fluid_state_;
    FluidState& a1 = this->rejection_air1_state_;
    FluidState& a2 = this->rejection_air2_state_;

    // Charge

    // Work by compressor, turbine
    double cp_WF = wf.fluid_material_.GetCp();
    double w_WF_C = cp_WF * ((wf.temp_charge_[1] - wf.temp_charge_[0]) - (wf.temp_charge_[4] - wf.temp_charge_[5])); // W/kg/s

    // Collect Parasitic Pump Variables
    double gam_a1 = this->rejection_air1_state_.fluid_material_.GetGam();
    double rho_hf = hf.fluid_material_.GetRho();
    double rho_cf = cf.fluid_material_.GetRho();
    double cp_a1 = a1.fluid_material_.GetCp();
    double cp_a2 = a2.fluid_material_.GetCp();
    double gam_a2 = a2.fluid_material_.GetGam();
    double etaP = this->kSystemParams.eta_pump;

    // Pressure Ratios
    comp_ratio_C_ = wf.pressure_charge_[1] / wf.pressure_charge_[0];
    exp_ratio_D_ = wf.pressure_discharge_[4] / wf.pressure_discharge_[5];

    // Parasitic Work
    double w_HF_C = (hf.mdot_charge_ / (etaP * rho_hf)) * (hf.pressure_charge_[0] - hf.pressure_charge_[1]);
    double w_CF_C = (cf.mdot_charge_ / (etaP * rho_cf)) * (cf.pressure_charge_[0] - cf.pressure_charge_[1]);
    double w_A1_C = ((a1.mdot_charge_ * cp_a1 * a1.temp_charge_[0]) / etaP) *
                    (pow(a1.pressure_charge_[0] / a1.pressure_charge_[1], (gam_a1 - 1.0) / gam_a1) - 1.0);

    // Net Work In
    double w_in_net = (w_WF_C + w_HF_C + w_CF_C + w_A1_C) / kSystemParams.motor_eff;
    W_out_ = kSystemParams.power_output;

    // Discharge

    // Work done by compressor and turbine
    double w_WF_D = cp_WF * ((wf.temp_discharge_[4] - wf.temp_discharge_[5]) - (wf.temp_discharge_[1] - wf.temp_discharge_[0]));

    // Parasitic Work Discharge
    double w_HF_D = (hf.mdot_discharge_ / (etaP * rho_hf)) * (hf.pressure_discharge_[0] - hf.pressure_discharge_[1]);
    double w_CF_D = (cf.mdot_discharge_ / (etaP * rho_cf)) * (cf.pressure_discharge_[0] - cf.pressure_discharge_[1]);
    double w_A1_D = ((a1.mdot_discharge_ * cp_a1 * a1.temp_discharge_[0]) / etaP) *
        (pow(a1.pressure_discharge_[0] / a1.pressure_discharge_[1], (gam_a1 - 1.0) / gam_a1) - 1.0);
    double w_A2_D = ((a2.mdot_discharge_ * cp_a2 * a2.temp_discharge_[0]) / etaP) *
        (pow(a2.pressure_discharge_[0] / a2.pressure_discharge_[1], (gam_a2 - 1.0) / gam_a2) - 1.0);

    // Net Work Out (unit)
    double w_out_net = (w_WF_D - w_HF_D - w_CF_D - w_A1_D - w_A2_D) * kSystemParams.gen_eff;

    // Heat Calculations
    double q_in = cp_WF * (wf.temp_charge_[1] - wf.temp_charge_[2]); // amount of heat being transferred to hot tanks (charge)
    double q_out = cp_WF * (wf.temp_discharge_[4] - wf.temp_discharge_[3]); // amount of heat extracted from hot tank (discharge)

    // Calculate Efficiencies
    rt_eff_ = w_out_net / w_in_net;     // round trip efficiency
    hp_COP_ = q_in / w_in_net;          // coefficient of performance
    cycle_eff_ = w_out_net / q_out;        // heat engine efficiency

    // Work and Heat Ratios
    W_ratio_ = (wf.temp_charge_[1] - wf.temp_charge_[0]) / (wf.temp_charge_[4] - wf.temp_charge_[5]);
    Q_ratio_ = ((wf.temp_charge_[1] - wf.temp_charge_[4]) + (wf.temp_charge_[0] - wf.temp_charge_[5])) /
                    ((wf.temp_charge_[1] - wf.temp_charge_[0]) - (wf.temp_charge_[4] - wf.temp_charge_[5]));

    // Calculate Energy
    E_out_ = kSystemParams.power_output * kSystemParams.discharge_time_hr * 3600.0; // convert hrs to seconds
    E_in_ = E_out_ / rt_eff_;
    W_in_ = E_in_ / (kSystemParams.charge_time_hr * 3600.0); // Generator efficiency is considered in E_in_ (via rt_eff_)

    // Mass Flow Rate
    double MDot_D = kSystemParams.power_output / w_out_net;
    double MDot_C = W_in_ / w_in_net;
    double unit_factor = MDot_D / wf.mdot_discharge_;    // Fraction of Real mass flow to unit

    // Thermodynamic Power Output
    W_WF_D_ = w_WF_D * MDot_D;

    // Hot and Cold Fluid Volumes
    vol_hf_ = hf.mdot_charge_ * MDot_C * kSystemParams.charge_time_hr * 3600.0 / rho_hf;
    vol_cf_ = cf.mdot_charge_ * MDot_C * kSystemParams.charge_time_hr * 3600.0 / rho_cf;

    // Tank Volumes
    vol_total_ = 2.0 * 1.1 * (vol_hf_ + vol_cf_); // each has two tanks. Volume of tank slightly larger than fluid vol
    E_density_ = E_out_ / vol_total_; // energy density

    // Output Parameters
    // hp_COP_;                                                 // Heat Pump Coefficient of Performance
    // W_WF_D                                                   // Cycle Thermodynamic Power
    // cycle_eff_;                                              // Cycle Thermo Efficiency
    Th_hot_ = this->hot_fluid_state_.temp_charge_[1] - 273;     // Hot reservoir's hot tank
    Th_cold_ = this->hot_fluid_state_.temp_charge_[0] - 273;    // Hot reservoir's cold tank
    Tc_hot_ = this->cold_fluid_state_.temp_charge_[0] - 273;    // Cold reservoir's hot tank
    Tc_cold_ = this->cold_fluid_state_.temp_charge_[1] - 273;   // Cold reservoir's cold tank


    // Heat Pump Performance
    {
        // Parasitic Work Charge
        double MDot_A1_C = a1.mdot_charge_ * unit_factor;                   // kg/s
        double power_in = kSystemParams.power_output / rt_eff_;             // W

        double W_A1_C = w_A1_C * unit_factor;                               // W
        double W_in_chg = w_WF_C * unit_factor;                             // W
        hp_parasitic_fraction_ = (power_in - W_in_chg + W_A1_C) / power_in; // - (W/W)

        // Pumping Power through Hot HX
        double MDot_HF = hf.mdot_discharge_ * unit_factor;                  // kg/s
        double W_HF_C = w_HF_C * unit_factor;                               // W
        hp_hot_pump_power_ = W_HF_C / 1e3 / MDot_HF;                        // kW/kg/s

        // Pumping Power through Cold HX
        double MDot_CF = cf.mdot_discharge_ * unit_factor;                  // kg/s
        double W_CF_C = w_CF_C * unit_factor;                               // W
        hp_cold_pump_power_ = W_CF_C / (1e3 * MDot_CF);                     // kW/kg/s
    }

    // Power Cycle Performance
    {
        // Parasitic Work Discharge
        double W_out_D = W_out_;                                            // W

        double MDot_A1_D = a1.mdot_discharge_ * unit_factor;                // kg/s
        double W_A1_D = w_A1_D * unit_factor;                               // W
        double MDot_A2_D = a1.mdot_discharge_ * unit_factor;                // kg/s
        double W_A2_D = w_A2_D * unit_factor;                               // W

        pc_parasitic_fraction_ = (W_out_D - kSystemParams.power_output + W_A1_D + W_A2_D) / W_out_D;    // - (W/W)

        // Pumping Power through Hot HX
        double MDot_HF_D = hf.mdot_discharge_ * unit_factor;                // kg/s
        double W_HF_D = w_HF_D * unit_factor;                               // W
        pc_hot_pump_power_ = W_HF_D / (1e3 * MDot_HF_D);                    // kW/kg/s

        // Pumping Power Through Cold HX
        double MDot_CF_D = cf.mdot_discharge_ * unit_factor;                // kg/s
        double W_CF_D = w_CF_D * unit_factor;                               // W
        pc_cold_pump_power_ = W_CF_D / (1e3 * MDot_CF_D);                   // kW/kg/s
    }

    has_performed_ = true;

}

/// <summary>
/// Returns TS Data for Working Fluid during Charge
/// </summary>
void PTESDesignPoint::GetTSDataCharge(vector<double>& temp_vec, vector<double>& entropy_vec)
{
    // Check if simulation has been run
    if (has_charged_ == false || has_discharged_ == false)
        return;

    this->GenerateTSData(temp_vec, entropy_vec, true);

}

/// <summary>
/// Returns TS Data for Working Fluid during Charge
/// </summary>
void PTESDesignPoint::GetTSDataDischarge(vector<double>& temp_vec, vector<double>& entropy_vec)
{
    // Check if simulation has been run
    if (has_charged_ == false || has_discharged_ == false)
        return;

    this->GenerateTSData(temp_vec, entropy_vec, false);

}

/// <summary>
/// Private function for generating TS Data
/// </summary>
void PTESDesignPoint::GenerateTSData(vector<double>& temp_vec, vector<double>& entropy_vec, bool is_charge)
{
    // Check if simulation has been run
    if (has_charged_ == false || has_discharged_ == false)
        return;

    // Collect necessary variables
    FluidState& wf = this->working_fluid_state_;
    double cp = wf.fluid_material_.GetCp();
    double R = wf.fluid_material_.GetR();
    double T0 = wf.fluid_material_.GetT0();
    double P0 = wf.fluid_material_.GetP0();

    // Initialize
    int num_states;
    vector<double> og_t_vec;
    vector<double> og_s_vec;
    vector<double> og_p_vec;

    // Charge Cycle
    if (is_charge == true)
    {
        num_states = wf.temp_charge_.size();
        og_t_vec = wf.temp_charge_;
        og_s_vec = wf.s_charge_;
        og_p_vec = wf.pressure_charge_;
    }

    // Discharge Cycle
    else
    {
        num_states = wf.temp_discharge_.size();
        og_t_vec = wf.temp_discharge_;
        og_s_vec = wf.s_discharge_;
        og_p_vec = wf.pressure_discharge_;
    }

    // Get Maximum Pressure Loss Factor
    vector<double> ploss_vec = { kSystemParams.ploss_air, kSystemParams.ploss_liquid, kSystemParams.ploss_working };
    double max_ploss = *std::max_element(ploss_vec.begin(), ploss_vec.end());

    // Initialize
    int Np = 100; // Number of data points along HX
    vector<double> T_vec;
    vector<double> P_vec;
    vector<double> s_vec;

    // Loop through states
    for (int j = 0; j < num_states - 1; j++)
    {
        // Values at boundaries
        double s_start = og_s_vec[j];
        double s_end = og_s_vec[j + 1];
        double T_start = og_t_vec[j];
        double T_end = og_t_vec[j + 1];
        double P_start = og_p_vec[j];
        double P_end = og_p_vec[j + 1];

        vector<double> T_seg_vec;
        vector<double> P_seg_vec;
        vector<double> s_seg_vec;

        // Check if pressure decreases a small amount (then it is heat exchanger segment)
        if (P_end >= (1.0 - max_ploss) * P_start && P_end < P_start)
        {
            T_seg_vec.resize(Np);
            P_seg_vec.resize(Np);
            s_seg_vec.resize(Np);

            double T_step = (T_end - T_start) / (Np - 1.0);
            double P_step = (P_end - P_start) / (Np - 1.0);

            // Calculate Values along HX
            for (int i = 0; i < Np; i++)
            {
                // T and P vary linearly along HX
                T_seg_vec[i] = T_start + i * T_step;
                P_seg_vec[i] = P_start + i * P_step;

                // Calculate S at each point
                s_seg_vec[i] = (cp * std::log(T_seg_vec[i] / T0) - R * std::log(P_seg_vec[i] / P0)) / 1.0e3; // convert to kJ/kg K

                // Convert T to C
                T_seg_vec[i] += -273.0;
            }
        }

        else
        {
            T_seg_vec = { T_start - 273.0, T_end - 273.0 };
            s_seg_vec = { s_start / 1.0e3, s_end / 1.0e3 };
        }

        // Add Segment to Overall Vector
        T_vec.insert(T_vec.end(), T_seg_vec.begin(), T_seg_vec.end());
        s_vec.insert(s_vec.end(), s_seg_vec.begin(), s_seg_vec.end());

    }

    temp_vec = T_vec;
    entropy_vec = s_vec;

    return;
}


// Static Functions

/// <summary>
/// Retrieve FluidType Enum based on string
/// </summary>
/// <param name="type_string"></param>
/// <param name="flag"></param>
/// <returns></returns>
FluidType PTESDesignPoint::GetFluidTypeFromString(std::string type_string, bool& flag)
{
    FluidType fluid_enum;

    try
    {
        fluid_enum = FluidMaterialProp::map_.at(type_string);
        flag = true;
    }
    catch (std::exception e)
    {
        flag = false;
        return FluidType::kAir;
    }




    return fluid_enum;
}

/// <summary>
/// Calculate Enthalpy in the system
/// </summary>
/// <param name="temp_vec">Temperature Vector</param>
/// <param name="fluid">Fluid Material Properties</param>
vector<double> PTESDesignPoint::CalculateEnthalpy(const vector<double>& temp_vec, FluidMaterialProp& fluid)
{
    // Create Enthalpy Vector
    int count = temp_vec.size();
    std::vector<double> enthalpy_vec(count, 0);

    // Loop through temp, calculating enthalpy
    double T0 = fluid.GetT0();
    double cp = fluid.GetCp();
    for (int i = 0; i < count; i++)
    {
        enthalpy_vec[i] = cp * (temp_vec[i] - T0);
    }

    return enthalpy_vec;
}

/// <summary>
/// Calculate Entropy
/// </summary>
/// <param name="temp_vec">Temperature Vector</param>
/// <param name="pressure_vec">Pressure Vector</param>
/// <param name="fluid">Fluid Material Properties</param>
/// <returns></returns>
vector<double> PTESDesignPoint::CalculateEntropy(const vector<double>& temp_vec, const vector<double>& pressure_vec, FluidMaterialProp& fluid)
{
    // Create Entropy Vector
    int count = temp_vec.size();
    std::vector<double> entropy_vec(count, 0);

    double cp = fluid.GetCp();
    double T0 = fluid.GetT0();
    
    // Compressible
    if (fluid.GetIsCompressible() == true)
    {
        double P0 = fluid.GetP0();
        double R = fluid.GetR();

        for (int i = 0; i < count; i++)
        {
            entropy_vec[i] = cp * std::log(temp_vec[i] / T0) - R * std::log(pressure_vec[i] / P0);
        }
    }
    // Incompressible
    else
    {
        for (int i = 0; i < count; i++)
        {
            entropy_vec[i] = cp * std::log(temp_vec[i] / T0);
        }
    }

    return entropy_vec;
}

/// <summary>
/// Calculate Density
/// </summary>
/// <param name="temp_vec">Temperature Vector</param>
/// <param name="pressure_vec">Pressure Vector</param>
/// <param name="fluid">Fluid Material Properties</param>
/// <returns></returns>
vector<double> PTESDesignPoint::CalculateDensity(const vector<double>& temp_vec, const vector<double>& pressure_vec, FluidMaterialProp& fluid)
{
    // Create Density Vector
    int count = temp_vec.size();
    std::vector<double> density_vec(count, 0);

    // Compressible
    if (fluid.GetIsCompressible() == true)
    {
        double R = fluid.GetR();

        for (int i = 0; i < count; i++)
        {
            density_vec[i] = pressure_vec[i] / (R * temp_vec[i]);
        }
    }

    // Incompressible
    else
    {
        for (int i = 0; i < count; i++)
        {
            density_vec[i] = fluid.GetRho();
        }
    }

    return density_vec;
}

/// <summary>
/// Calculate Viscosity
/// </summary>
/// <param name="temp_vec">Temperature Vector</param>
/// <param name="fluid">Fluid Material Properties</param>
/// <returns></returns>
vector<double> PTESDesignPoint::CalculateViscosity(const vector<double>& temp_vec, FluidMaterialProp& fluid)
{
    // Create Viscosity Vector
    int count = temp_vec.size();
    vector<double> viscosity_vec(count, 0);

    // Compressible
    if (fluid.GetIsCompressible() == true)
    {
        double mu0 = fluid.GetMu0();
        double Smu = fluid.GetSmu();
        double T0 = fluid.GetT0();

        for (int i = 0; i < count; i++)
        {
            viscosity_vec[i] = mu0 * ((T0 + Smu) / (temp_vec[i] + Smu)) * pow(temp_vec[i] / T0, 1.5);
        }
    }

    // Incompressible
    else
    {
        double mu = fluid.GetMu();

        for (int i = 0; i < count; i++)
        {
            viscosity_vec[i] = mu;
        }
    }

    return viscosity_vec;
}

