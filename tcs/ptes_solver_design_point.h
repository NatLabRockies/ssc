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

#ifndef __PTES_SOLVER_DESIGN_POINT__
#define __PTES_SOLVER_DESIGN_POINT__

#include <string>
#include <functional>
#include <vector>
#include <unordered_map>
#include <map>
using std::vector;
using std::string;

enum struct FluidType
{
    kNitrogen = 0,
    kArgon = 1,
    kHydrogen = 2,
    kHelium = 3,
    kAir = 4,
    kNitrateSalt = 5,
    kChlorideSalt = 6,
    kGlycol = 7,
    kMethanol = 8,
    kRejectionAir = 9
};

class FluidMaterialProp
{
public:
    FluidMaterialProp(FluidType fluid_type);
    FluidMaterialProp(double cp, double rho, double T0, double P0);

    double GetCp() { return cp_; }
    double GetRho() { return rho_; }
    double GetGam() { return gam_; }
    double GetR() { return R_; }
    double GetMu() { return mu_; }
    double GetMu0() { return mu0_; }
    double GetT0() { return T0_; }
    double GetP0() { return P0_; }
    double GetSmu() { return Smu_; }
    bool GetIsCompressible() { return is_compressible_; }

    typedef std::map<std::string, FluidType> map;
    static map map_;

private:
    std::string name_;                  // Material Name
    double cp_;                         // Specific Heat, constant pressure (J/kg K)
    double cv_;                         // Specific Heat, constant volume (J/kg K)
    double rho_;                        // Density (kg/m3)
    double gam_;                        // Ratio of Specific Heats
    double R_;                          // Specific Heat Constant
    double mu_;                         // Viscosity (Ns/m2)
    double mu0_;                        // Viscosity at T0, P0 (Ns/m2)
    double T0_;                         // Reference Temp (K)
    double P0_;                         // Reference Pressure (Pa)
    double Smu_;                        // Sutherland's Constant

    bool is_compressible_;

    void SetFluid(FluidType fluid_type);

};

class FluidState
{
public:

    FluidState(int num_states_charge, int num_states_discharge, FluidType fluid_type);
    FluidState(int num_states_charge, int num_states_discharge, FluidMaterialProp fluid);

    const int num_states_charge_;       // Number of states in charge cycle
    const int num_states_discharge_;    // Number of states in discharge cycle

    vector<double> temp_charge_;        // Temperature Values in charge cycle (T1 = T[0])
    vector<double> temp_discharge_;     // Temperature Values in discharge cycle
    vector<double> pressure_charge_;    // Pressure Values in charge cycle
    vector<double> pressure_discharge_; // Pressure Values in discharge cycle
    vector<double> h_charge_;           // Enthalpy in charge cycle
    vector<double> h_discharge_;        // Enthalpy in discharge cycle
    vector<double> s_charge_;           // Entropy in charge cycle
    vector<double> s_discharge_;        // Entropy in discharge cycle
    vector<double> rho_charge_;         // Density in charge cycle
    vector<double> rho_discharge_;      // Density in discharge cycle
    vector<double> mu_charge_;          // Viscosity in charge cycle
    vector<double> mu_discharge_;       // Viscosity in discharge cycle

    double mdot_charge_;                // Mass Flow Rate in charge cycle       *unit mass flow rate (relative to WF=1)
    double mdot_discharge_;             // Mass Flow Rate in discharge cycle    *unit mass flow rate (relative to WF=1)

    FluidMaterialProp fluid_material_;  // Fluid Material Properties

};

class PTESSystemParam
{
public:

    PTESSystemParam() {};

    // Component Properties
    double hx_eff = 0;                  // hx effectiveness
    double eta = 0;                     // polytropic efficiency of compressors and expanders    
    double eta_pump = 0;                // polytropic efficiency of air pump
    double ploss_working = 0;           // Fractional pressure loss in each heat exchanger
    double ploss_air = 0;               // Fractional pressure loss (air)
    double ploss_liquid = 0;            // Fractional pressure loss (liquid)
    double motor_eff = 0;               // Motor Efficiency
    double gen_eff = 0;                 // Generator Efficiency

    // Ambient Properties
    double T0 = 0;                      // Ambient Temperature, K
    double P0 = 0;                      // Ambient Pressure, Pa

    // Cycle Properties
    double P1 = 0;                      // Lowest Pressure in cycle, Pa
    double T_compressor_inlet = 0;      // Charging compressor inlet temperature, K
    double T_compressor_outlet = 0;     // Charging compressor outlet temperature, K
    double power_output = 0;            // Power Output, W
    double charge_time_hr = 0;          // charging time, hr
    double discharge_time_hr = 0;       // discharge time, hr
    double alpha = 0;                   // Ratio of mdot cp     AIR/WF

};

class PTESDesignPoint
{
public:

    // Functions
    PTESDesignPoint(PTESSystemParam params, FluidType working_fluid_type, FluidType hot_fluid_type, FluidType cold_fluid_type);
    PTESDesignPoint(PTESSystemParam params, FluidMaterialProp working_fluid, FluidMaterialProp hot_fluid, FluidMaterialProp cold_fluid);

    static FluidType GetFluidTypeFromString(std::string type_string, bool& flag);

    void Charge();                      // Simulate Charge Cycle
    void Discharge();                   // Simulate Discharge Cycle
    void Performance();                 // Calculate Cycle Performance

    void GetTSDataCharge(vector<double>& temp_vec, vector<double>& entropy_vec);
    void GetTSDataDischarge(vector<double>& temp_vec, vector<double>& entropy_vec);

    // Cycle Results
    double W_in_ = 0;                       // Design Power Input, W-e
    double W_out_ = 0;                      // Design Power Output, W-e
    double E_in_ = 0;                       // Design Energy Input, J
    double E_out_ = 0;                      // Design Energy Output, J
    double E_density_ = 0;                  // Energy Density, J/m3
    double rt_eff_ = 0;                     // Round Trip Efficiency, %
    double W_ratio_ = 0;                    // Work Ratio
    double Q_ratio_ = 0;                    // Heat to Work Ratio
    double comp_ratio_C_ = 0;               // Charge Compressor Pressure Ratio
    double exp_ratio_D_ = 0;                // Discharge Compressor Pressure Ratio
    double vol_hf_ = 0;                     // Hot Fluid Volume, m3
    double vol_cf_ = 0;                     // Cold Fluid Volume, m3
    double vol_total_ = 0;                  // Total Volume, m3

    // SAM Output Results
    double hp_COP_ = 0;                     // Heat Pump Coefficient of Performance, %
    double W_WF_D_ = 0;
    double cycle_eff_ = 0;                  // Cycle Efficiency, %
    double Th_hot_ = 0;                     // Temperature of the Hot Tanks hotter tank, C
    double Th_cold_ = 0;                    // Temperature of the Hot Tanks colder tank, C
    double Tc_hot_ = 0;                     // Temperature of the Cold Tanks hotter tank, C
    double Tc_cold_ = 0;                    // Temperature of the Cold Tanks colder tank, C

    double hp_parasitic_fraction_ = 0;      // Heat Pump Non Pumping Parasitic Fraction
    double hp_hot_pump_power_ = 0;          // Heat Pump Pumping Power Through Hot HX, kW/kg/s
    double hp_cold_pump_power_ = 0;         // Heat Pump Pumping Power Through Cold HX, kW/kg/s
    double pc_parasitic_fraction_ = 0;      // Power Cycle Non Pumping Parasitic Function
    double pc_hot_pump_power_ = 0;          // Power Cycle Pumping Power Through Hot HX, kW/kg/s
    double pc_cold_pump_power_ = 0;         // Power Cycle Pumping Power Through Cold HX, kW/kg/s

private:

    // Internal Parameters
    FluidState working_fluid_state_;
    FluidState hot_fluid_state_;
    FluidState cold_fluid_state_;
    FluidState rejection_air1_state_;
    FluidState rejection_air2_state_;
    bool has_charged_ = false;
    bool has_discharged_ = false;
    bool has_performed_ = false;
    const PTESSystemParam kSystemParams; // constant system parameters

    // Private Functions
    void GenerateTSData(vector<double>& temp_vec, vector<double>& entropy_vec, bool is_charge);

    // Static Functions
    static vector<double> CalculateEnthalpy(const vector<double>& temp_vec, FluidMaterialProp& fluid);
    static vector<double> CalculateEntropy(const vector<double>& temp_vec, const vector<double>& pressure_vec, FluidMaterialProp& fluid);
    static vector<double> CalculateDensity(const vector<double>& temp_vec, const vector<double>& pressure_vec, FluidMaterialProp& fluid);
    static vector<double> CalculateViscosity(const vector<double>& temp_vec, FluidMaterialProp& fluid);

};

#endif // !__PTES_SOLVER_DESIGN_POINT_
