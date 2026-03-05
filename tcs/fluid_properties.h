#ifndef FLUID_PROPERTIES_H
#define FLUID_PROPERTIES_H

#include <limits>

struct fluid_info
{
    double molar_mass = std::numeric_limits<double>::quiet_NaN();       // molar mass of fluid (kg/kmol)
    double T_critical = std::numeric_limits<double>::quiet_NaN();       // critical temperature (K)
    double D_critical = std::numeric_limits<double>::quiet_NaN();       // critical density (kg/m3)
    double P_critical = std::numeric_limits<double>::quiet_NaN();       // critical pressure (kPa)
    double temp_lower_limit = std::numeric_limits<double>::quiet_NaN(); // lowest available temperature (K)
    double temp_upper_limit = std::numeric_limits<double>::quiet_NaN(); // highest available temperature (K)
    double pres_lower_limit = std::numeric_limits<double>::quiet_NaN(); // lowest available pressure (kPa)
    double pres_upper_limit = std::numeric_limits<double>::quiet_NaN(); // highest available pressure (kPa)
    double sat_temp_min = std::numeric_limits<double>::quiet_NaN();     // minimum saturation temperature (K)
    double sat_pres_min = std::numeric_limits<double>::quiet_NaN();     // minimum saturation pressure (kPa)
};

struct fluid_state
{
    double temp = std::numeric_limits<double>::quiet_NaN();             // temperature (K)
    double pres = std::numeric_limits<double>::quiet_NaN();             // pressure (kPa)
    double dens = std::numeric_limits<double>::quiet_NaN();             // density (kg/m3)
    double qual = std::numeric_limits<double>::quiet_NaN();             // quality (-)
    double inte = std::numeric_limits<double>::quiet_NaN();             // internal energy (kJ/kg)
    double enth = std::numeric_limits<double>::quiet_NaN();             // enthalpy (kJ/kg)
    double entr = std::numeric_limits<double>::quiet_NaN();             // entropy (kJ/kg-K)
    double cv = std::numeric_limits<double>::quiet_NaN();               // specific heat at const. volume (kJ/kg-K)
    double cp = std::numeric_limits<double>::quiet_NaN();               // specific heat at const. pressure (kJ/kg-K)
    double ssnd = std::numeric_limits<double>::quiet_NaN();             // speed of sound in fluid (m/s)
    double sat_vap_dens = std::numeric_limits<double>::quiet_NaN();     // saturated vapor density (kg/m3)
    double sat_liq_dens = std::numeric_limits<double>::quiet_NaN();     // saturated liquid density (kg/m3)
};

class C_fluid_properties
{
public:

    // Thermodynamic property functions
    virtual int TD(double T, double D, fluid_state* state) = 0;
    virtual int TP(double T, double P, fluid_state* state) = 0;
    virtual int PH(double P, double H, fluid_state* state) = 0;
    virtual int PS(double P, double S, fluid_state* state) = 0;
    virtual int HS(double H, double S, fluid_state* state) = 0;
    virtual int TQ(double T, double Q, fluid_state* state) = 0;
    virtual int PQ(double P, double Q, fluid_state* state) = 0;

    // Misc functions
    virtual double visc(double D, double T) = 0;    // [uPa-s]
    virtual double cond(double D, double T) = 0;    // [W/m-K]

    // Fluid info
    virtual void get_info(fluid_info* info) = 0;
    //virtual double T_crit() const = 0;
    //virtual double P_crit() const = 0;
    //virtual double D_crit() const = 0;
    //virtual double T_upper_limit() const = 0;
    //virtual double P_upper_limit() const = 0;
};

#endif

