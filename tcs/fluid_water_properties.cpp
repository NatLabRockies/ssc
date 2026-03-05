#include "fluid_water_properties.h"
#include "fluid_properties.h"
#include "water_properties.h"

int C_water_properties::TD(double T, double D, fluid_state* f_state)
{
    water_state w_state;
    int flag = water_TD(T, D, &w_state);
    this->fluid_state_from_water_state(&w_state, f_state);

    return flag;
}

int C_water_properties::TP(double T, double P, fluid_state* state)
{
    water_state w_state;
    int flag = water_TP(T, P, &w_state);
    this->fluid_state_from_water_state(&w_state, state);

    return flag;
}

int C_water_properties::PH(double P, double H, fluid_state* state)
{
    water_state w_state;
    int flag = water_PH(P, H, &w_state);
    this->fluid_state_from_water_state(&w_state, state);

    return flag;
}

int C_water_properties::PS(double P, double S, fluid_state* state)
{
    water_state w_state;
    int flag = water_PS(P, S, &w_state);
    this->fluid_state_from_water_state(&w_state, state);
    return flag;
}

int C_water_properties::HS(double H, double S, fluid_state* state)
{
    water_state w_state;
    int flag = water_HS(H, S, &w_state);
    this->fluid_state_from_water_state(&w_state, state);
    return flag;
}

int C_water_properties::TQ(double T, double Q, fluid_state* state)
{
    water_state w_state;
    int flag = water_TQ(T, Q, &w_state);
    this->fluid_state_from_water_state(&w_state, state);
    return flag;
}

int C_water_properties::PQ(double P, double Q, fluid_state* state)
{
    water_state w_state;
    int flag = water_PQ(P, Q, &w_state);
    this->fluid_state_from_water_state(&w_state, state);
    return flag;
}

double C_water_properties::visc(double D, double T)
{
    return water_visc(D, T);
}

double C_water_properties::cond(double D, double T)
{
    return water_cond(D, T);
}

void C_water_properties::get_info(fluid_info* info)
{
    water_info w_info;
    get_water_info(&w_info);

    info->molar_mass = w_info.molar_mass;
    info->T_critical = w_info.T_critical;
    info->D_critical = w_info.D_critical;
    info->P_critical = w_info.P_critical;
    info->temp_lower_limit = w_info.temp_lower_limit;
    info->temp_upper_limit = w_info.temp_upper_limit;
    info->pres_lower_limit = w_info.pres_lower_limit;
    info->pres_upper_limit = w_info.pres_upper_limit;
    info->sat_temp_min = w_info.sat_temp_min;
    info->sat_pres_min = w_info.sat_pres_min;
}
