#include "fluid_co2_properties.h"
#include "fluid_properties.h"


int C_co2_properties::TD(double T, double D, fluid_state* state)
{
    CO2_state w_state;
    int flag = CO2_TD(T, D, &w_state);
    this->fluid_state_from_CO2_state(&w_state, state);

    return flag;
}

int C_co2_properties::TP(double T, double P, fluid_state* state)
{
    CO2_state w_state;
    int flag = CO2_TP(T, P, &w_state);
    this->fluid_state_from_CO2_state(&w_state, state);

    return flag;
}

int C_co2_properties::PH(double P, double H, fluid_state* state)
{
    CO2_state w_state;
    int flag = CO2_PH(P, H, &w_state);
    this->fluid_state_from_CO2_state(&w_state, state);

    return flag;
}

int C_co2_properties::PS(double P, double S, fluid_state* state)
{
    CO2_state w_state;
    int flag = CO2_PS(P, S, &w_state);
    this->fluid_state_from_CO2_state(&w_state, state);
    return flag;
}

int C_co2_properties::HS(double H, double S, fluid_state* state)
{
    CO2_state w_state;
    int flag = CO2_HS(H, S, &w_state);
    this->fluid_state_from_CO2_state(&w_state, state);
    return flag;
}

int C_co2_properties::TQ(double T, double Q, fluid_state* state)
{
    CO2_state w_state;
    int flag = CO2_TQ(T, Q, &w_state);
    this->fluid_state_from_CO2_state(&w_state, state);
    return flag;
}

int C_co2_properties::PQ(double P, double Q, fluid_state* state)
{
    // Not supported
    return -1;
}

double C_co2_properties::visc(double D, double T)
{
    return CO2_visc(D, T);
}

double C_co2_properties::cond(double D, double T)
{
    return CO2_cond(D, T);
}

void C_co2_properties::get_info(fluid_info* info)
{
    CO2_info c_info;
    get_CO2_info(&c_info);

    info->molar_mass = c_info.molar_mass;
    info->T_critical = c_info.T_critical;
    info->D_critical = c_info.D_critical;
    info->P_critical = c_info.P_critical;
    info->temp_lower_limit = c_info.temp_lower_limit;
    info->temp_upper_limit = c_info.temp_upper_limit;
    info->pres_lower_limit = c_info.pres_lower_limit;
    info->pres_upper_limit = c_info.pres_upper_limit;
    info->sat_temp_min = c_info.sat_temp_min;
    info->sat_pres_min = c_info.sat_pres_min;
}
