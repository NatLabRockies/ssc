#ifndef FLUID_COOLPROP_PROPERTIES_H
#define FLUID_COOLPROP_PROPERTIES_H

#include <string>

#include "fluid_properties.h"

class C_coolprop_properties : public C_fluid_properties
{
public:

    C_coolprop_properties(const std::string& fluid);
    C_coolprop_properties(const std::string& fluid, const std::string& backend);
    ~C_coolprop_properties();

    C_coolprop_properties(const C_coolprop_properties&) = delete;
    C_coolprop_properties& operator=(const C_coolprop_properties&) = delete;

    // Thermodynamic property functions
    int TD(double T, double D, fluid_state* state) override;
    int TP(double T, double P, fluid_state* state) override;
    int PH(double P, double H, fluid_state* state) override;
    int PS(double P, double S, fluid_state* state) override;
    int HS(double H, double S, fluid_state* state) override;
    int TQ(double T, double Q, fluid_state* state) override;
    int PQ(double P, double Q, fluid_state* state) override;

    // Misc functions
    double visc(double D, double T) override;
    double cond(double D, double T) override;

    // Fluid info
    void get_info(fluid_info* info) override;


private:
    void init_handle();
    void free_handle();

    int update_handle_state(long input_pair, double val1, double val2);
    int fluid_state_from_handle(fluid_state* fs);

    long m_handle;
    std::string m_backend;
    std::string m_fluid;

    // CoolProp indices
    long m_iTD = -1;
    long m_iPT = -1;
    long m_iHP = -1;
    long m_iPS = -1;
    long m_iHS = -1;
    long m_iQT = -1;
    long m_iPQ = -1;

    long m_iT = -1;
    long m_iP = -1;
    long m_iDmass = -1;
    long m_iUmass = -1;
    long m_iHmass = -1;
    long m_iSmass = -1;
    long m_iCvmass = -1;
    long m_iCpmass = -1;
    long m_iQ = -1;
    long m_iSpeedSound = -1;
};

#endif

