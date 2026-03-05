#ifndef FLUID_WATER_PROPERTIES_H
#define FLUID_WATER_PROPERTIES_H

#include <limits>

#include "fluid_properties.h"
#include "water_properties.h"

class C_water_properties : public C_fluid_properties
{
public:

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

    static void fluid_state_from_water_state(const water_state* ws, fluid_state* fs)
    {
        fs->temp = ws->temp;
        fs->pres = ws->pres;
        fs->dens = ws->dens;
        fs->qual = ws->qual;
        fs->inte = ws->inte;
        fs->enth = ws->enth;
        fs->entr = ws->entr;
        fs->cv = ws->cv;
        fs->cp = ws->cp;
        fs->ssnd = ws->ssnd;
        fs->sat_vap_dens = ws->sat_vap_dens;
        fs->sat_liq_dens = ws->sat_liq_dens;
    }

};

#endif

