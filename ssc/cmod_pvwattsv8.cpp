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



#include <memory>

#include "core.h"

#include "common.h"

#include "lib_weatherfile.h"
#include "lib_irradproc.h"
#include "lib_pvshade.h"
#include "lib_pvmodel.h"
#include "lib_snowmodel.h"
#include "lib_sandia.h"
#include "lib_pv_incidence_modifier.h"
#include "lib_cec6par.h"

class lossdiagram
{
    unordered_map< std::string, double > m_map;
    struct loss_item {
        loss_item(const std::string& _n, bool _b) : name(_n), baseline(_b) { }
        std::string name;
        bool baseline;
    };
    std::string m_error;
    std::vector<loss_item> m_items;
public:
    lossdiagram()
    {
    }

    std::string errormsg() { return m_error; }

    void add(const std::string& name, bool baseline)
    {
        m_items.push_back(loss_item(name, baseline));
        m_map[name] = 0.0;
    }

    bool assign(compute_module* cm, const std::string& prefix)
    {
        m_error.clear();

        // calculate percentages
        double last_baseline = 0.0;
        for (size_t i = 0; i < m_items.size(); i++)
        {

            if (m_map.find(m_items[i].name) == m_map.end())
            {
                m_error = "could not locate loss accumulation value '" + m_items[i].name + "'";
            }

            if (m_items[i].baseline)
                last_baseline = m_map[m_items[i].name];
            else
            {
                double value = m_map[m_items[i].name];
                double percent = value / last_baseline * 100.0;
                cm->assign(prefix + m_items[i].name + "_percent", (ssc_number_t)percent);
            }
        }

        for (auto it = m_map.begin(); it != m_map.end(); ++it)
            cm->assign(prefix + it->first, var_data((ssc_number_t)it->second));

        return m_error.size() == 0;
    }

    double& operator() (const std::string& name)
    {
        auto it = m_map.find(name);
        if (it != m_map.end())
        {
            return it->second;
        }
        else
        {
            m_map[name] = 0.0;
            return m_map.find(name)->second;
        }
    }

};

static var_info _cm_vtab_pvwattsv8[] = {

    /*   VARTYPE           DATATYPE          NAME                              LABEL                                          UNITS        META                                            GROUP          REQUIRED_IF                 CONSTRAINTS                      UI_HINTS*/
        { SSC_INPUT,        SSC_STRING,      "solar_resource_file",            "Weather file path",                          "",           "",                                             "Solar Resource",      "",                       "",                              "" },
        { SSC_INPUT,        SSC_TABLE,       "solar_resource_data",            "Weather data",                               "",           "dn,df,tdry,wspd,lat,lon,tz,elev",              "Solar Resource",      "",                       "",                              "" },
        { SSC_INPUT,        SSC_ARRAY,       "albedo",                         "Albedo",                                     "0..1",       "albedo input array of 1 constant value or 12 monthly values","Solar Resource",    "",                        "",                              "" },
        { SSC_INPUT,        SSC_NUMBER,      "albedo_default",                 "Albedo default",                             "0..1",       "default when albedo invalid","Solar Resource",    "?=0.2",                        "",                              "" },
        { SSC_INPUT,        SSC_NUMBER,      "albedo_default_snow",            "Albedo default for snow",                    "0..1",       "default when albedo invalid and snow model enabled","Solar Resource",    "?=0.6",                        "",                              "" },
        { SSC_INPUT,        SSC_NUMBER,      "use_wf_albedo",                  "Use albedo from weather file",               "0/1",        "0=albedo input, 1=albedo from weather file (use albedo default if invalid)","Solar Resource","?=1",                    "BOOLEAN",          "" },

        { SSC_INOUT,        SSC_NUMBER,      "system_use_lifetime_output",     "Run lifetime simulation",                    "0/1",        "",                                             "Lifetime",            "?=0",                        "",                              "" },
        { SSC_INPUT,        SSC_NUMBER,      "analysis_period",                "Analysis period",                            "years",      "",                                             "Lifetime",            "system_use_lifetime_output=1", "",                          "" },
        { SSC_INPUT,        SSC_ARRAY,       "dc_degradation",                 "Annual DC degradation for lifetime simulations","%/year",  "",                                             "Lifetime",            "system_use_lifetime_output=1", "",                          "" },

        { SSC_INPUT,        SSC_NUMBER,      "system_capacity",                "System size (DC nameplate)",                  "kW",        "",											   "System Design",      "*",                       "",                      "" },
        { SSC_INPUT,        SSC_NUMBER,      "module_type",                    "Module type",                                 "0/1/2",     "standard,premium,thin film",                   "System Design",      "?=0",                     "MIN=0,MAX=2,INTEGER",           "" },
        { SSC_INPUT,        SSC_NUMBER,      "dc_ac_ratio",                    "DC to AC ratio",                              "ratio",     "",                                             "System Design",      "?=1.1",                   "POSITIVE",                      "" },

        { SSC_INPUT,        SSC_NUMBER,      "bifaciality",                    "Module bifaciality factor",                   "0 or ~0.65","",                                             "System Design",      "?=0",                       "",                              "" },

        { SSC_INPUT,        SSC_NUMBER,      "array_type",                     "Array type",                                  "0/1/2/3/4", "fixed open rack,fixed roof mount,1-axis tracking,1-axis backtracking,2-axis tracking","System Design",      "*",                       "MIN=0,MAX=4,INTEGER",           "" },
        { SSC_INPUT,        SSC_NUMBER,      "tilt",                           "Tilt angle",                                  "degrees",       "H=0,V=90",                                     "System Design",      "array_type<4",            "MIN=0,MAX=90",                  "" },
        { SSC_INPUT,        SSC_NUMBER,      "azimuth",                        "Azimuth angle",                               "degrees",       "E=90,S=180,W=270",                             "System Design",      "array_type<4",            "MIN=0,MAX=360",                 "" },
        { SSC_INPUT,        SSC_NUMBER,      "gcr",                            "Ground coverage ratio",                       "0..1",      "",                                             "System Design",      "?=0.3",                   "MIN=0.01,MAX=0.99",             "" },
        { SSC_INPUT,        SSC_NUMBER,      "rotlim",                         "Tracker rotation angle limit",                "degrees",       "",                                             "System Design",      "?=45.0",                  "",                              "" },

        { SSC_INPUT,        SSC_ARRAY,       "soiling",                        "Soiling loss",                                "%",         "",                                             "System Design",      "?",                       "",                              "" },
        { SSC_INPUT,        SSC_NUMBER,      "losses",						   "DC system losses",                             "%",         "total system losses",                          "System Design",      "*",                       "MIN=-5,MAX=99",                 "" },

        { SSC_INPUT,        SSC_NUMBER,      "enable_wind_stow",               "Enable tracker stow at high wind speeds",     "0/1",       "",                                             "System Design",      "?=0",                     "BOOLEAN",                              "" },
        { SSC_INPUT,        SSC_NUMBER,      "stow_wspd",                      "Tracker stow wind speed threshold",           "m/s",       "",                                             "System Design",      "?=10",                    "",                              "" },
        { SSC_INPUT,        SSC_NUMBER,      "gust_factor",                    "Wind gust estimation factor",                 "",          "",                                             "System Design",      "?",                       "",                              "" },
        { SSC_INPUT,        SSC_NUMBER,      "wind_stow_angle",                "Tracker angle for wind stow",                 "degrees",       "",                                             "System Design",      "?=30.0",                  "",                              "" },

        { SSC_INPUT,        SSC_NUMBER,      "en_snowloss",                    "Enable snow loss model",                      "0/1",       "",                                             "System Design",      "?=0",                     "BOOLEAN",                       "" },

        { SSC_INPUT,        SSC_NUMBER,      "inv_eff",                        "Inverter efficiency at rated power",          "%",         "",                                             "System Design",      "?=96",                    "MIN=90,MAX=99.5",               "" },

        { SSC_INPUT,        SSC_NUMBER,      "xfmr_nll",                       "GSU transformer no load loss (iron core)",    "%(ac)",     "",                                             "System Design",      "?=0.0",                   "",                              "" },
        { SSC_INPUT,        SSC_NUMBER,      "xfmr_ll",                        "GSU transformer load loss (resistive)",       "%(ac)",     "",                                             "System Design",      "?=0.0",                   "",                              "" },

//         { SSC_INPUT,        SSC_TABLE,      "shading",               "Shading loss table",                 "",         "",                                             "System Design",      "?",                        "",                             "" },
        {SSC_INPUT, SSC_NUMBER,   "shading_en_string_option",           "Enable shading string option",             "0/1",    "0=false,1=true",                    "Shading",                                               "?=0",                                  "BOOLEAN",                    "" },
        {SSC_INPUT, SSC_NUMBER,   "shading_string_option",      "Shading string option",                   "",       "0=shadingdb,1=average,2=maximum,3=minimum",  "Shading",                                               "?=-1",                               "INTEGER,MIN=-1,MAX=4","" },
        {SSC_INPUT, SSC_NUMBER,   "shading_en_timestep",         "Enable timestep beam shading losses",          "0/1",    "0=false,1=true",                       "Shading",                                               "?=0",                                  "BOOLEAN",                    "" },
        {SSC_INPUT, SSC_MATRIX,   "shading_timestep",           "Timestep beam shading losses",            "%",      "",                                           "Shading",                                               "?",                                  "",                    "" },
        {SSC_INPUT, SSC_NUMBER,   "shading_en_mxh",               "Enable month x Hour beam shading losses",  "0/1",    "0=false,1=true",                          "Shading",                                               "?=0",                                  "BOOLEAN",                    "" },
        {SSC_INPUT, SSC_MATRIX,   "shading_mxh",                "Month x Hour beam shading losses",        "%",      "",                                           "Shading",                                               "?",                                  "",                    "" },
        {SSC_INPUT, SSC_NUMBER,   "shading_en_azal",               "Enable azimuth x altitude beam shading losses",          "0/1",    "0=false,1=true",           "Shading",                                               "?=0",                                  "BOOLEAN",                    "" },
        {SSC_INPUT, SSC_MATRIX,   "shading_azal",               "Azimuth x altitude beam shading losses",  "%",      "",                                           "Shading",                                               "?",                                  "",                    "" },
        {SSC_INPUT, SSC_NUMBER,   "shading_en_diff",               "Enable diffuse shading loss",          "0/1",    "0=false,1=true",                             "Shading",                                               "?=0",                                  "BOOLEAN",                    "" },
        {SSC_INPUT, SSC_NUMBER,   "shading_diff",               "Diffuse shading loss",                    "%",      "",                                           "Shading",                                               "?",                                  "",                    "" },


        { SSC_INPUT,        SSC_NUMBER,      "batt_simple_enable",             "Enable Battery",                              "0/1",       "",                                             "System Design",     "?=0",                     "BOOLEAN",                        "" },
       
        /* outputs */
        { SSC_OUTPUT,       SSC_ARRAY,       "gh",                             "Weather file global horizontal irradiance",                "W/m2",      "",                                             "Time Series",      "*",                       "",                          "" },
        { SSC_OUTPUT,       SSC_ARRAY,       "dn",                             "Weather file beam irradiance",                             "W/m2",      "",											   "Time Series",      "*",                       "",                          "" },
        { SSC_OUTPUT,       SSC_ARRAY,       "df",                             "Weather file diffuse irradiance",                          "W/m2",      "",											   "Time Series",      "*",                       "",                          "" },
        { SSC_OUTPUT,       SSC_ARRAY,       "tamb",                           "Weather file ambient temperature",                         "C",         "",										       "Time Series",      "*",                       "",                          "" },
        { SSC_OUTPUT,       SSC_ARRAY,       "wspd",                           "Weather file wind speed",                                  "m/s",       "",											   "Time Series",      "*",                       "",                          "" },
        { SSC_OUTPUT,       SSC_ARRAY,       "snow",                           "Weather file snow depth",                                  "cm",        "",										       "Time Series",      "",                        "",                          "" },

        { SSC_OUTPUT,       SSC_ARRAY,       "alb",                            "Albedo",                                  "",        "",										       "Time Series",      "",                        "",                          "" },
        { SSC_OUTPUT,       SSC_ARRAY,       "soiling_f",                 "Soiling factor",                                  "",        "",										       "Time Series",      "",                        "",                          "" },

        { SSC_OUTPUT,       SSC_ARRAY,       "sunup",                          "Sun up over horizon",                         "0/1",       "",                                             "Time Series",      "*",                       "",                          "" },
        { SSC_OUTPUT,       SSC_ARRAY,       "shad_beam_factor",               "External shading factor for beam radiation",           "",          "",                                             "Time Series",      "*",                       "",                                     "" },
        { SSC_OUTPUT,       SSC_ARRAY,       "ss_beam_factor",                 "Calculated self-shading factor for beam radiation",           "",          "1=no shading",                                             "Time Series",      "*",                       "",                                     "" },
        { SSC_OUTPUT,       SSC_ARRAY,       "ss_sky_diffuse_factor",          "Calculated self-shading factor for sky diffuse radiation",           "",          "1=no shading",                                             "Time Series",      "*",                       "",                                     "" },
        { SSC_OUTPUT,       SSC_ARRAY,       "ss_gnd_diffuse_factor",          "Calculated self-shading factor for ground-reflected diffuse radiation",           "",          "1=no shading",                                             "Time Series",      "*",                       "",                                     "" },
        { SSC_OUTPUT,       SSC_ARRAY,       "aoi",                            "Angle of incidence",                          "degrees",       "",                                             "Time Series",      "*",                       "",                          "" },
        { SSC_OUTPUT,       SSC_ARRAY,       "poa",                            "Plane of array irradiance",                   "W/m2",      "",                                             "Time Series",      "*",                       "",                          "" },
        { SSC_OUTPUT,       SSC_ARRAY,       "tpoa",                           "Transmitted plane of array irradiance",       "W/m2",      "",                                             "Time Series",      "*",                       "",                          "" },
        { SSC_OUTPUT,       SSC_ARRAY,       "tcell",                          "Module temperature",                          "C",         "",                                             "Time Series",      "*",                       "",                          "" },
        { SSC_OUTPUT,       SSC_ARRAY,       "dcsnowderate",                   "DC power loss due to snow",            "%",         "",                                             "Time Series",      "*",                       "",                          "" },
        { SSC_OUTPUT,       SSC_ARRAY,       "snow_cover",                     "Fraction of row covered by snow",            "0..1",         "",                                             "Time Series",      "*",                       "",                          "" },

        { SSC_OUTPUT,       SSC_ARRAY,       "dc",                             "DC inverter input power",                              "W",         "",                                             "Time Series",      "*",                       "",                          "" },
        { SSC_OUTPUT,       SSC_ARRAY,       "ac",                             "AC inverter output power",                           "W",         "",                                             "Time Series",      "*",                       "",                          "" },
        { SSC_OUTPUT,       SSC_ARRAY,       "ac_pre_adjust",                  "AC inverter output power before system availability",                           "W",         "",                                             "Time Series",      "*",                       "",                          "" },

        { SSC_OUTPUT,       SSC_ARRAY,       "inv_eff_output",                        "Inverter efficiency",                           "%",         "",                                             "Time Series",      "*",                       "",                          "" },

        { SSC_OUTPUT,       SSC_ARRAY,       "poa_monthly",                    "Plane of array irradiance",                   "kWh/m2",    "",                                             "Monthly",          "",                       "LENGTH=12",                          "" },
        { SSC_OUTPUT,       SSC_ARRAY,       "solrad_monthly",                 "Daily average solar irradiance",              "kWh/m2/day","",                                             "Monthly",          "",                       "LENGTH=12",                          "" },
        { SSC_OUTPUT,       SSC_ARRAY,       "dc_monthly",                     "Monthly DC energy",                             "kWh",       "",                                             "Monthly",          "",                       "LENGTH=12",                          "" },
        { SSC_OUTPUT,       SSC_ARRAY,       "ac_monthly",                     "Monthly AC energy",                            "kWh",       "",                                             "Monthly",          "",                       "LENGTH=12",                          "" },
        { SSC_OUTPUT,       SSC_ARRAY,       "monthly_energy",                 "Monthly AC energy in Year 1",                              "kWh",       "",                                             "Monthly",          "",                       "LENGTH=12",                          "" },
        { SSC_OUTPUT,	    SSC_MATRIX,		 "annual_energy_distribution_time","Annual energy production as function of time",				"",				"",				"Heatmaps",			"",						"",							"" },

        { SSC_OUTPUT,       SSC_NUMBER,      "solrad_annual",                  "Daily average solar irradiance",              "kWh/m2/day","",                                              "Annual",      "",                       "",                          "" },
        { SSC_OUTPUT,       SSC_NUMBER,      "ac_annual",                      "Annual AC output",                     "kWh",       "",                                                     "Annual",      "",                       "",                          "" },
        { SSC_OUTPUT,       SSC_NUMBER,      "ac_annual_pre_adjust",                      "Annual AC output before system availability",                     "kWh",       "",                                                     "Annual",      "",                       "",                          "" },

        { SSC_OUTPUT,       SSC_NUMBER,      "annual_energy",                  "Annual AC energy in Year 1",                               "kWh",       "",                                              "Annual",      "",                       "",                          "" },
        { SSC_OUTPUT,       SSC_NUMBER,      "capacity_factor",                "Capacity factor based on nameplate DC capacity",    "%",         "",                                           "Annual",        "",                       "",                          "" },
        { SSC_OUTPUT,       SSC_NUMBER,      "capacity_factor_ac",             "Capacity factor based on total AC capacity",    "%",         "",                                           "Annual",        "",                       "",                          "" },
        { SSC_OUTPUT,       SSC_NUMBER,      "kwh_per_kw",                     "Energy yield",                           "kWh/kW",          "",                                             "Annual",        "",                       "",                          "" },

        { SSC_OUTPUT,       SSC_STRING,      "location",                       "Location ID",                                 "",          "",                                             "Location",      "*",                       "",                          "" },
        { SSC_OUTPUT,       SSC_STRING,      "city",                           "City",                                        "",          "",                                             "Location",      "*",                       "",                          "" },
        { SSC_OUTPUT,       SSC_STRING,      "state",                          "State",                                       "",          "",                                             "Location",      "*",                       "",                          "" },
        { SSC_OUTPUT,       SSC_NUMBER,      "lat",                            "Latitude",                                    "degrees",       "",                                             "Location",      "*",                       "",                          "" },
        { SSC_OUTPUT,       SSC_NUMBER,      "lon",                            "Longitude",                                   "degrees",       "",                                             "Location",      "*",                       "",                          "" },
        { SSC_OUTPUT,       SSC_NUMBER,      "tz",                             "Time zone",                                   "UTC offset",        "",                                    "Location",      "*",                       "",                          "" },
        { SSC_OUTPUT,       SSC_NUMBER,      "elev",                           "Site elevation",                              "m",         "",                                             "Location",      "*",                       "",                          "" },

        { SSC_OUTPUT,       SSC_NUMBER,      "inverter_efficiency",            "Inverter efficiency at rated power",          "%",         "",                                             "PVWatts",      "",                        "",                              "" },

        { SSC_OUTPUT,       SSC_NUMBER,      "ts_shift_hours",                 "Time offset for interpreting time series outputs", "hours","",                                             "Miscellaneous", "*",                       "",                          "" },
        { SSC_OUTPUT,       SSC_NUMBER,      "percent_complete",               "Estimated percent of total completed simulation", "%",     "",                                             "Miscellaneous", "",                        "",                          "" },
        { SSC_OUTPUT,       SSC_NUMBER,      "system_capacity_ac",             "System nameplate AC rating", "kWac",     "",                                             "Miscellaneous", "",                        "",                          "" },

        var_info_invalid };

class cm_pvwattsv8 : public compute_module
{
protected:

    enum module_type { STANDARD, PREMIUM, THINFILM };
    enum module_orientation { PORTRAIT, LANDSCAPE };
    enum array_type { FIXED_RACK, FIXED_ROOF, ONE_AXIS, ONE_AXIS_BACKTRACKING, TWO_AXIS, AZIMUTH_AXIS }; //azimuth axis not enabled in inputs?

    static const constexpr double bifacialTransmissionFactor = 0.013;

    struct {
        module_type type;		//standard, premium, thinfilm
        double stc_watts;		//rated power at STC in Watts
        double stc_eff;			//rated efficiency at STC (unitless)
        double ff;				//fill factor (unitless)
        double aspect_ratio;	//module length / width (unitless)
        double width;			//module width in meters
        double length;			//module length in meters
        double area;			//module area in square meters
        double vmp;				//maximum power voltage in volts
        int ndiode;				//number of diodes in module (unitless)
        double gamma;			//temperature coefficient of maximum power- units are 1 / degree Celsius
        bool ar_glass;			//whether or not module has anti-reflective glass
        double bifaciality;		//bifaciality factor for bifacial modules (unitless)
    } module;

    struct {
        array_type type;

        double dc_nameplate;    //nameplate rated capacity of the DC side of the system units of this variable are W, while input is in kW
        double dc_ac_ratio;     //ratio of DC nameplate capacity to AC nameplate capacity (unitless)
        double ac_nameplate;    //nameplate rated capacity of the AC side of the system (W)
        double xfmr_rating;     //rating of the transformer, hardcoded to be equal to ac_nameplate (W)
        double inv_eff_percent; //inverter efficiency at rated power (percent)
        double dc_loss_percent; //DC system losses (percent)
        double tilt, azimuth;   //tilt and azimuth of the system (degrees)
        double rotlim;          //tracker rotation limit (degrees)

        double xfmr_nll_f;      //transformer no-load-loss (percent of AC power)
        double xfmr_ll_f;       //transformer load loss (percent of AC power)

        double nmodules;        //number of modules (unitless)
        double nmodperstr;      //number of modules per string (unitless)
        int nmodx, nmody, nrows;//number of modules along the bottom of a row, number of modules along the upward direction of a row, number of rows (unitless)
        double row_spacing;     //row spacing, calculated from other inputs (meters)
        double gcr;             //ground coverage ratio (unitless)

    } pv;

    struct sdmml { //single diode model mermoud lejeune (alternative to pvwatts model)
        double Area;
        double Vmp;
        double Imp;
        double Voc;
        double Isc;

        double n_0;
        double mu_n;
        double N_series;
        double alpha_isc;
        double E_g;
        double R_shexp;
        double R_sh0;
        double R_shref;
        double R_s;
        double D2MuTau;
    };

    sdmml sdm;

    lossdiagram ld;

public:
    cm_pvwattsv8()
    {
        add_var_info(vtab_technology_outputs);
        add_var_info(_cm_vtab_pvwattsv8);
        add_var_info(vtab_adjustment_factors);
        add_var_info(vtab_technology_outputs);
        add_var_info(vtab_hybrid_tech_om_outputs);


        ld.add("poa_nominal", true);
        ld.add("poa_loss_tracker_stow", false);
        ld.add("poa_loss_ext_beam_shade", false);
        ld.add("poa_loss_ext_diff_shade", false);
        ld.add("poa_loss_self_beam_shade", false);
        ld.add("poa_loss_self_diff_shade", false);
        ld.add("poa_loss_soiling", false);
        ld.add("poa_loss_bifacial", false);

        ld.add("dc_nominal", true);
        ld.add("dc_loss_cover", false);
        ld.add("dc_loss_spectral", false);
        ld.add("dc_loss_thermal", false);
        ld.add("dc_loss_nonlinear", false);
        ld.add("dc_loss_snow", false);
        ld.add("dc_loss_other", false);

        ld.add("ac_nominal", true);
        ld.add("ac_loss_efficiency", false);
        ld.add("ac_loss_inverter_clipping", false);
        ld.add("ac_loss_adjustments", false);
        ld.add("ac_loss_plant_clipping", false);
        ld.add("ac_loss_transformer", false);

        ld.add("ac_delivered", true);
    }

    virtual ~cm_pvwattsv8()
    {
        // nothing to do
    }

    double sdmml_power(sdmml& m, double S, double T_cell) //mermoud lejeune single diode model structure, S=irradiance, T_cell is cell temperature
    {
        static const double S_ref = 1000;
        static const double T_ref = 25;
        static const double k = 1.38064852e-23; // Boltzmann constant [J/K]
        static const double q = 1.60217662e-19; // Elemenatry charge [C]
        static const double T_0 = 273.15; // 0 degrees Celsius in Kelvin [K]

        if (S > 1)
        {
            double R_sh_STC = m.R_shref + (m.R_sh0 - m.R_shref) * exp(-m.R_shexp * (S_ref / S_ref));
            double nVT = m.N_series * m.n_0 * k * (T_ref + T_0) / q;
            double I_0ref = (m.Isc + (m.Isc * m.R_s - m.Voc) / R_sh_STC) / ((exp(m.Voc / nVT) - 1) - (exp((m.Isc * m.R_s) / nVT) - 1));
            double I_Lref = I_0ref * (exp(m.Voc / nVT) - 1) + m.Voc / R_sh_STC;
            double Vbi = 0.9 * m.N_series;
            double n = m.n_0 + m.mu_n * (T_cell - T_ref);
            double a = m.N_series * k * (T_cell + T_0) * n / q;
            double I_L = (S / S_ref) * (I_Lref + m.alpha_isc * (T_cell - T_ref));
            double I_0 = I_0ref * pow(((T_cell + T_0) / (T_ref + T_0)), 3) * exp((q * m.E_g) / (n * k) * (1 / (T_ref + T_0) - 1 / (T_cell + T_0)));
            double R_sh = m.R_shref + (m.R_sh0 - m.R_shref) * exp(-m.R_shexp * (S / S_ref));
            double V_oc = openvoltage_5par_rec(m.Voc, a, I_L, I_0, R_sh, m.D2MuTau, Vbi);

            double V, I;
            return maxpower_5par_rec(V_oc, a, I_L, I_0, m.R_s, R_sh, m.D2MuTau, Vbi, &V, &I);
        }
        else
            return 0.0;
    }

    void exec()
    {
        std::unique_ptr<weather_data_provider> wdprov;

        if (is_assigned("solar_resource_file"))
        {
            const char* file = as_string("solar_resource_file");
            wdprov = std::unique_ptr<weather_data_provider>(new weatherfile(file));

            weatherfile* wfile = dynamic_cast<weatherfile*>(wdprov.get());
            if (!wfile->ok()) throw exec_error("pvwattsv8", wfile->message());
            if (wfile->has_message()) log(wfile->message(), SSC_WARNING);
        }
        else if (is_assigned("solar_resource_data"))
        {
            wdprov = std::unique_ptr<weather_data_provider>(new weatherdata(lookup("solar_resource_data")));
            if (!wdprov->ok()) {
                throw exec_error("pvwattsv8", wdprov->message());
            }
            if (wdprov->has_message()) log(wdprov->message(), SSC_WARNING);
        }
        else
            throw exec_error("pvwattsv8", "No weather data supplied.");

        pv.dc_nameplate = as_double("system_capacity") * 1000; //units of this variable are W, while input is in kW
        pv.dc_ac_ratio = as_double("dc_ac_ratio");
        pv.ac_nameplate = pv.dc_nameplate / pv.dc_ac_ratio;

        pv.xfmr_rating = pv.ac_nameplate;

        pv.inv_eff_percent = as_double("inv_eff");

        size_t soiling_len = 0;
        ssc_number_t* soiling = nullptr;
        if (is_assigned("soiling"))
        {
            soiling = as_array("soiling", &soiling_len);
        }

        size_t albedo_len = 0;
        ssc_number_t* albedo = 0;
        if (is_assigned("albedo"))
        {
            albedo = as_array("albedo", &albedo_len);
        }
        bool use_wf_albedo = as_boolean("use_wf_albedo");

        pv.dc_loss_percent = as_double("losses");
        pv.tilt = pv.azimuth = std::numeric_limits<double>::quiet_NaN();
        pv.rotlim = 45.0;
        if (is_assigned("tilt")) pv.tilt = as_double("tilt");
        if (is_assigned("azimuth")) pv.azimuth = as_double("azimuth");
        if (is_assigned("rotlim")) pv.rotlim = as_double("rotlim");

        pv.xfmr_ll_f = as_double("xfmr_ll") * 0.01; //transformer inputs always present, but default to 0
        pv.xfmr_nll_f = as_double("xfmr_nll") * 0.01;

        bool enable_wind_stow = as_boolean("enable_wind_stow");
        if (enable_wind_stow && !wdprov->annualSimulation())
            log("Using the wind stow model with weather data that is not continuous over one year may result in over-estimation of stow losses.", SSC_WARNING);
        double wstow = std::numeric_limits<double>::quiet_NaN();
        if (is_assigned("stow_wspd")) wstow = as_double("stow_wspd"); // wind stow speed, m/s.
        double wind_stow_angle_deg = std::numeric_limits<double>::quiet_NaN(); // default is to assume stowing at 30 degrees (set in var_table) for better dynamic torsional stability, despite higher static loading on piles
        if (is_assigned("wind_stow_angle")) wind_stow_angle_deg = as_double("wind_stow_angle");
        // gust factor defined later because it depends on timestep

        //hidden input variable (not in var_table): whether or not to use the mermoud lejeune single diode model as defined above (0 = don't use model, 1 = use model)
        int en_mlm = is_assigned("en_mlm") ? as_integer("en_mlm") : 0;

        cec6par_module_t mod; // structure for the CEC single diode model calculations
        noct_celltemp_t modTempCalc; // structure for the module temperature calculations
        modTempCalc.standoff_tnoct_adj = 0; // do not use the adjustment for cell temperature, but rather set noct as commented below
        modTempCalc.ffv_wind = 0.51; // assume that all mounting configurations are < 22 ft. This may not be perfect for roof-mounted systems, but the differences are likely minimal.

        module.type = (module_type)as_integer("module_type");
        switch (module.type)
        {
        case STANDARD:
            module.gamma = -0.0037;
            module.ar_glass = true;
            module.ff = 0.778; //fill factors required for self-shading calculations
            module.stc_eff = 0.19;

            // for full CEC single diode model, Canadian Solar CS1H-320MS, parameters calculated by SAM version 2020.2.29 r3
            mod.Area = 1.6864;
            mod.Vmp = 35.8;
            mod.Imp = 9.01;
            mod.Voc = 43.3;
            mod.Isc = 9.51;
            mod.alpha_isc = 0.004755;
            mod.beta_voc = -0.12557;
            mod.a = 1.65916;
            mod.Il = 9.51393;
            mod.Io = 4.37813e-11;
            mod.Rs = 0.269953;
            mod.Rsh = 654.059;
            mod.Adj = 5.79586;

            // for optional SDM module model:
            // selected module from PVsyst PAN database: TSM-330DD14A(II)
            // note that this is a DIFFERENT module than the factors listed above
            sdm.Area = 1.940;
            sdm.Vmp = 37.8;
            sdm.Imp = 8.73;
            sdm.Voc = 46.2;
            sdm.Isc = 9.27;
            sdm.n_0 = 0.92;
            sdm.mu_n = 0;
            sdm.N_series = 72;
            sdm.alpha_isc = 0.0046;
            sdm.E_g = 1.12;
            sdm.R_shexp = 20;
            sdm.R_sh0 = 2000;
            sdm.R_shref = 550;
            sdm.R_s = 0.382;
            sdm.D2MuTau = 0.0;
            break;

        case PREMIUM:
            module.gamma = -0.0035;
            module.ar_glass = true;
            module.ff = 0.780;
            module.stc_eff = 0.21; //efficiency assumption updated per Sunpower

            // for full CEC single diode model, Sunpower SPR-E20-327
            mod.Area = 1.6297;
            mod.Vmp = 54.7;
            mod.Imp = 5.98;
            mod.Voc = 64.9;
            mod.Isc = 6.46;
            mod.alpha_isc = 0.0026;
            mod.beta_voc = -0.1766;
            mod.a = 2.45326;
            mod.Il = 6.4704;
            mod.Io = 2.01784e-11;
            mod.Rs = 0.421231;
            mod.Rsh = 261.723;
            mod.Adj = 9.56482;

            // for optional SDM module model:
            // selected module from PVsyst PAN database: SPR-X20-327-COM
            // note that this is a DIFFERENT module than the factors listed above
            sdm.Area = 1.630;
            sdm.Vmp = 59.5;
            sdm.Imp = 5.49;
            sdm.Voc = 70.0;
            sdm.Isc = 5.84;
            sdm.n_0 = 1.17;
            sdm.mu_n = 0;
            sdm.N_series = 96;
            sdm.alpha_isc = 0.0025;
            sdm.E_g = 1.12;
            sdm.R_shexp = 5.5;
            sdm.R_sh0 = 14000;
            sdm.R_shref = 3444;
            sdm.R_s = 0.4;
            sdm.D2MuTau = 0.0;
            break;

        case THINFILM:
            module.gamma = -0.0032;
            module.ar_glass = true;
            module.ff = 0.777;
            module.stc_eff = 0.18;

            // for full CEC single diode model, First Solar FS-6435A
            mod.Area = 2.4751;
            mod.Vmp = 183.6;
            mod.Imp = 2.37;
            mod.Voc = 219.6;
            mod.Isc = 2.55;
            mod.alpha_isc = 0.00102;
            mod.beta_voc = -0.61488;
            mod.a = 7.97293;
            mod.Il = 2.55475;
            mod.Io = 2.69243e-12;
            mod.Rs = 4.60991;
            mod.Rsh = 2476.95;
            mod.Adj = -2.49865;

            // for optional SDM module model:
            // selected module from PVsyst PAN database: FS-4112-3
            // note that this is a DIFFERENT module than the factors listed above
            sdm.Area = 0.72;
            sdm.Vmp = 68.5;
            sdm.Imp = 1.64;
            sdm.Voc = 87.0;
            sdm.Isc = 1.83;
            sdm.n_0 = 1.5;
            sdm.mu_n = 0.002;
            sdm.N_series = 108;
            sdm.alpha_isc = 0.0007;
            sdm.E_g = 1.12;
            sdm.R_shexp = 6;
            sdm.R_sh0 = 12000;
            sdm.R_shref = 3500;
            sdm.R_s = 4.36;
            sdm.D2MuTau = 0.95;
            break;
        }

        // common module parameters
        module.aspect_ratio = 1.7; // typical geometry, length / width
        module.stc_watts = 300; // assume a typical mid-size module nameplate (e.g. 300 Watts)
        module.area = module.stc_watts / module.stc_eff / 1000.0; // module area in m2
        module.width = sqrt((module.area / module.aspect_ratio));
        module.length = module.width * module.aspect_ratio;
        module.vmp = 60.0;
        module.ndiode = 3;

        module.bifaciality = 0.0;
        if (is_assigned("bifaciality"))
            module.bifaciality = as_double("bifaciality");


        static double AMdesoto[5] = { 0.918093, 0.086257, -0.024459, 0.002816, -0.000126 };	// !Air mass modifier coefficients as indicated in DeSoto paper

        double module_m2 = pv.dc_nameplate / module.stc_eff / 1000;

        pv.type = (array_type)as_integer("array_type");
        switch (pv.type)
        {
        case FIXED_ROOF:
            modTempCalc.Tnoct = 49; // rather than using the standoff_tnoct_adj factor from the noct_celltemp_t structure, we just make an assumption here for increased Tnoct due to roof-mounting
            break;
        default: // all other types
            modTempCalc.Tnoct = 45;
            break;
        }

        // warning if tilt is > 0 for a tracking system becaues this is a very uncommon configuration but an easy mistake to make
        if ((pv.type == ONE_AXIS || pv.type == ONE_AXIS_BACKTRACKING) && pv.tilt > 0)
            log(util::format("The tilt angle is %f degrees with one-axis tracking. Large one-axis tracking arrays typically have a tilt angle of zero.", pv.tilt), SSC_WARNING);

        if (pv.type == FIXED_ROOF && module.bifaciality > 0.0)
            log("The Fixed Roof Mount array type is not appropriate for bifacial modules because it assumes there is no space between the back of the array and the roof surface.", SSC_WARNING);

        pv.gcr = as_double("gcr");
        if (pv.gcr < 0.01 || pv.gcr >= 1.0)
            throw exec_error("pvwattsv8", "invalid gcr");

        bool en_self_shading = (pv.type == FIXED_RACK || pv.type == ONE_AXIS || pv.type == ONE_AXIS_BACKTRACKING);

        // Electrical Layout
        // modules per string: 7 modules of about 60 Vmp each
        // gives a nominal DC voltage of about 420 V DC which seems reasonable
        pv.nmodperstr = 7;
        pv.nmodules = ceil(pv.dc_nameplate / module.stc_watts); // estimate of # of modules in system
        // but make sure there's at least one module, in the case where dc_nameplate < stc_watts
        if (pv.nmodules < 1) pv.nmodules = 1;

        // Physical Layout
        // reasonable estimates of system geometry, used for self-shading calculations ONLY.
        // assume one module high for trackers, 2 modules high for fixed or two-axis
        if (pv.type == ONE_AXIS || pv.type == ONE_AXIS_BACKTRACKING)
            pv.nmody = 1; // e.g. Nextracker or ArrayTechnologies single portrait
        else
            pv.nmody = 2; // typical fixed 2 up portrait
         if (pv.type == FIXED_ROOF)
            pv.nrows = 1; // fixed roof systems are all in one "row", so we'll assign that here for clarity even though these numbers only affect self-shading and that's disabled for rooftop systems
        else
            pv.nrows = (int)ceil(sqrt(pv.nmodules/pv.nmody)); // assume a ''square'' system layout- same number of modules across a row (nmodx) as number of rows (nrows), so need to divide by nmody to ensure that's true
        pv.nmodx = ceil(pv.nmodules / ( pv.nrows * pv.nmody)); // would rather that the estimated physical layout be larger than the actual number of modules for estimating self-shading impacts
        if (pv.nmodx < 1) pv.nmodx = 1; // shading calculation fails for pv.nmodx < 1
        pv.row_spacing = module.length * pv.nmody / pv.gcr;

        // see note farther down in code about self-shading for small systems
        // assume at least some reasonable number of rows.
        // otherwise self shading model may not really apply very well.
        // in fact, should have some minimum system size
        /*if (pv.nrows < 10)
            log(util::format("system size is too small to accurately estimate regular row-row self shading impacts. (estimates: #modules=%d, #rows=%d).  disabling self-shading calculations.",
            (int)pv.nmodules, (int)pv.nrows), SSC_WARNING);*/

        pvsnowmodel snowmodel;
        bool en_snowloss = as_boolean("en_snowloss");
        if (en_snowloss)
        {
            // check for snow model with non-annual simulations: because snow model coefficients need to know the timestep, and we don't know timestep if non-annual
            if (!wdprov->annualSimulation())
                log("Using the snow model with weather data that is not continuous over one year may result in over-estimation of snow losses.", SSC_WARNING);
            // if tracking mode is 1-axis tracking,
            // don't need to limit tilt angles
            if (snowmodel.setup(pv.nmody,
                (float)pv.tilt, 1.97,
                pv.type == FIXED_RACK || pv.type == FIXED_ROOF)) {

                if (!snowmodel.good) {
                    log(snowmodel.msg, SSC_ERROR);
                }
            }
        }

        

        // read all the shading input data and calculate the hourly factors for use subsequently
        // timeseries beam shading factors cannot be used with non-annual data
        if (is_assigned("shading_en_timestep") && as_boolean("shading_en_timestep") && !wdprov->annualSimulation())
            //        if (is_assigned("shading:timestep") && !wdprov->annualSimulation())
                throw exec_error("pvwattsv8", "Timeseries beam shading inputs cannot be used for a simulation period that is not continuous over one or more years.");
        shading_factor_calculator shad;
        if (!shad.setup(this, ""))
            throw exec_error("pvwattsv8", shad.get_error());
        // self-shading initialization
        sssky_diffuse_table ssSkyDiffuseTable;
        if (en_self_shading)
            ssSkyDiffuseTable.init(pv.tilt, pv.gcr);

        weather_header hdr;
        wdprov->header(&hdr);

        // assumes instantaneous values, unless hourly file with no minute column specified- same code as lib_pv_io_manager.cpp. can we make this one set of code somehow?
        double ts_shift_hours = 0.0;
        bool instantaneous = true;
        if (wdprov->has_data_column(weather_data_provider::MINUTE))
        {
            // if we have an file with a minute column, then
            // the starting time offset equals the time
            // of the first record (for correct plotting)
            // this holds true even for hourly data with a minute column
            weather_record rec;
            if (wdprov->read(&rec))
                ts_shift_hours = rec.minute / 60.0;

            wdprov->rewind();
        }
        else if (wdprov->nrecords() == 8760)
        {
            // hourly file with no minute data column.  assume
            // integrated/averaged values and use mid point convention for interpreting results
            instantaneous = false;
            ts_shift_hours = 0.5;
        }
        else
            throw exec_error("pvwattsv8", "Minute column required in weather data for subhourly data or data that is not continuous over one year.");

        assign("ts_shift_hours", var_data((ssc_number_t)ts_shift_hours));

        weather_record wf;

        size_t nyears = 1;
        std::vector<double> degradationFactor;
        if (as_boolean("system_use_lifetime_output")) {
            if (!wdprov->annualSimulation())
                throw exec_error("pvwattsv8", "Simulation cannot be run over analysis period for weather data that is not continuous over one year. Set system_use_lifetime_output to 0 to resolve this issue.");
            nyears = as_unsigned_long("analysis_period");
            std::vector<double> dc_degradation = as_vector_double("dc_degradation");
            if (dc_degradation.size() == 1) {
                degradationFactor.push_back(1.0); // assume zero degradation in year 1
                for (size_t y = 1; y < nyears; y++) {
                    degradationFactor.push_back(1.0 - (dc_degradation[0] * y) / 100.0);
                }
            }
            else {
                if (dc_degradation.size() != nyears)
                    throw exec_error("pvwattsv8", "Length of degradation array must be equal to analysis period.");
                for (size_t y = 0; y < nyears; y++) {
                    degradationFactor.push_back(1.0 - dc_degradation[y] / 100.0);
                }
            }
        }
        else {
            degradationFactor.push_back(1.0);
        }

        size_t nrec = wdprov->nrecords();
        size_t nlifetime = nrec * nyears;

        adjustment_factors haf(this->get_var_table(), "adjust");
        if (!haf.setup(nrec, nyears))
            throw exec_error("pvwattsv8", "Failed to set up adjustment factors: " + haf.error());

        size_t step_per_hour = 1; //default to 1 step per hour for non-annual simulations
        if (wdprov->annualSimulation())
            step_per_hour = nrec / 8760; //overwrite with real value for annual simulations
        if (wdprov->annualSimulation() && (step_per_hour < 1 || step_per_hour > 60 || step_per_hour * 8760 != nrec))
            throw exec_error("pvwattsv8", util::format("Invalid number of data records (%d): must be an integer multiple of 8760.", (int)nrec));
        double ts_hour = 1.0 / step_per_hour; //timestep in fraction of hours (decimal)

        double wm2_to_wh = module_m2 * ts_hour; //conversion from watts per meter squared to watt hours- need to convert with ts_hour for subhourly data

        double gustf = std::numeric_limits<double>::quiet_NaN(); // gust factor
        if (is_assigned("gust_factor")) gustf = as_double("gust_factor");
        double gf = gustf;
        if (!std::isfinite(gf)) //if gust factor isn't defined by user, determine it for them
        {
            // determine the sustained 1 minute gust wind speed
            // based on the current time step
            // this translation is for the 'in-land' category in
            // table 1.1 of the World Metereological Organization report
            //  'Guidelines for converting between various wind averaging periods
            //   in tropical cyclone conditions', October 2008

            double ts_sec = ts_hour * 3600.0;
            if (ts_sec >= 600)
                gf = 1.28;
            else if (ts_sec >= 180)
                gf = 1.21;
            else if (ts_sec >= 120)
                gf = 1.15;
            else if (ts_sec >= 60)
                gf = 1.13;
            else
                gf = 1.0;
        }

        /* allocate output arrays */
        ssc_number_t* p_gh = allocate("gh", nrec);
        ssc_number_t* p_dn = allocate("dn", nrec);
        ssc_number_t* p_df = allocate("df", nrec);
        ssc_number_t* p_tamb = allocate("tamb", nrec);
        ssc_number_t* p_wspd = allocate("wspd", nrec);
        ssc_number_t* p_snow = allocate("snow", nrec);

        ssc_number_t* p_sunup = allocate("sunup", nrec);
        ssc_number_t* p_aoi = allocate("aoi", nrec);
        ssc_number_t* p_shad_beam = allocate("shad_beam_factor", nrec); // just for reporting output
        ssc_number_t* p_ss_beam = allocate("ss_beam_factor", nrec);
        ssc_number_t* p_ss_sky_diffuse = allocate("ss_sky_diffuse_factor", nrec);
        ssc_number_t* p_ss_gnd_diffuse = allocate("ss_gnd_diffuse_factor", nrec);
        ssc_number_t* p_stow = allocate("tracker_stowing", nrec); // just for reporting output

        ssc_number_t* p_tmod = allocate("tcell", nrec);
        ssc_number_t* p_alb = allocate("alb", nrec);
        ssc_number_t* p_soiling_f = allocate("soiling_f", nrec);
        ssc_number_t* p_dcshadederate = allocate("dcshadederate", nrec);
        ssc_number_t* p_dcsnowderate = allocate("dcsnowderate", nrec);
        ssc_number_t* p_snowcover = allocate("snow_cover", nrec);
        ssc_number_t* p_poa = allocate("poa", nrec);
        ssc_number_t* p_tpoa = allocate("tpoa", nrec);
        ssc_number_t* p_dc = allocate("dc", nrec);
        ssc_number_t* p_ac = allocate("ac", nrec);
        ssc_number_t* p_ac_pre_adjust = allocate("ac_pre_adjust", nrec);
        ssc_number_t* p_inv_eff = allocate("inv_eff_output", nrec);
        ssc_number_t* p_gen = allocate("gen", nlifetime);

        double annual_kwh = 0;

        size_t idx_life = 0;
        float percent = 0;
        int n_alb_errs = 0;
        double elev, pres, t_amb;
        irrad irr;
        if (nyears > 1)
            irr.setup_solarpos_outputs_for_lifetime(nrec);
        for (size_t y = 0; y < nyears; y++)
        {
            for (size_t idx = 0; idx < nrec; idx++)
            {
                if (!wdprov->read(&wf))
                    throw exec_error("pvwattsv8", util::format("could not read data line %d of %d in weather file", (int)(idx + 1), (int)nrec));
                size_t hour_of_year = util::hour_of_year(wf.month, wf.day, wf.hour);

#define NSTATUS_UPDATES 50  // set this to the number of times a progress update should be issued for the simulation
                if (nrec > 50) //avoid divide by zero problems in the following if statement- probably don't need a lot of updates otherwise
                {
                    if (idx % (nrec / NSTATUS_UPDATES) == 0)
                    {
                        percent = 100.0f * ((float)idx_life + 1) / ((float)nlifetime); //3 is the number of technologies we're assuming for this output (pvwatts + fuel cell + battery)
                        // check percentage
                        if (percent > 100.0f) percent = 99.0f;
                        if (!update("", percent, (float)hour_of_year))
                            throw exec_error("pvwattsv8", "Simulation stopped at hour " + util::to_string(hour_of_year + 1.0));
                    }
                }

                bool tracker_stowing = false;

                p_gh[idx] = (ssc_number_t)wf.gh;
                p_dn[idx] = (ssc_number_t)wf.dn;
                p_df[idx] = (ssc_number_t)wf.df;
                p_tamb[idx] = (ssc_number_t)wf.tdry;
                p_wspd[idx] = (ssc_number_t)wf.wspd;
                p_snow[idx] = (ssc_number_t)wf.snow; // if there is no snow data in the weather file, this will be NaN- consistent with pvsamv1
                p_tmod[idx] = (ssc_number_t)wf.tdry;

                // use wf albedo if use_wf_albedo is true and wf albedo is valid
                // use user albedo if use_wf_albedo is false and user albedo is valid
                // otherwise use default albedo (snow or no snow)
                double alb = 0;
                if (use_wf_albedo)
                {
                    if ((std::isfinite(wf.alb) && (wf.alb > 0 && wf.alb < 1)))
                        alb = wf.alb;
                    else
                        n_alb_errs++;
                }
                else
                {
                    if (albedo_len == 1)
                        alb = albedo[0];
                    else if (albedo_len == 12)
                        alb = albedo[wf.month - 1];
                    else if (albedo_len == nrec)
                        alb = albedo[idx];
                    else if (is_assigned("albedo") && (idx == 0)) // display warning once
                        log(util::format("Albedo input array is not the correct length (1, 12, or %d entries). "
                            "Using default albedo value of %f (snow) or %f (no snow). "
                            "[year:%d month:%d day:%d hour:%d minute:%lg]. ",
                            nrec, as_double("albedo_default_snow"), as_double("albedo_default"),
                            wf.year, wf.month, wf.day, wf.hour, wf.minute), SSC_NOTICE);
                }
                if (alb <= 0 || alb >= 1)
                {
                    if (std::isfinite(wf.snow) && wf.snow > 0.5 && wf.snow < 999 && en_snowloss)
                        alb = as_double("albedo_default_snow");
                    else
                        alb = as_double("albedo_default");
                    if (!use_wf_albedo && n_alb_errs < 5) // display warning up to 5 times
                    {
                        log(util::format("Albedo input value is not valid for time step %d. Using default albedo value of %f (snow) or %f (no snow). This warning only appears for the first five instances of this error.", idx, as_double("albedo_default_snow"), as_double("albedo_default")), SSC_NOTICE);
                        n_alb_errs++;
                    }
                }

                // report albedo value as output
                p_alb[idx] = (ssc_number_t)alb;

                irr.set_time(wf.year, wf.month, wf.day, wf.hour, wf.minute,
                    instantaneous ? IRRADPROC_NO_INTERPOLATE_SUNRISE_SUNSET : ts_hour);
                irr.set_location(hdr.lat, hdr.lon, hdr.tz);
                irr.set_optional(hdr.elev, wf.pres, wf.tdry);
                irr.set_sky_model(irrad::PEREZ, alb);
                irr.set_beam_diffuse(wf.dn, wf.df);

                int track_mode = 0;
                switch (pv.type)
                {
                case FIXED_RACK:
                case FIXED_ROOF:
                    track_mode = 0;
                    break;
                case ONE_AXIS:
                case ONE_AXIS_BACKTRACKING:
                    track_mode = 1;
                    break;
                case TWO_AXIS:
                    track_mode = 2;
                    break;
                case AZIMUTH_AXIS:
                    track_mode = 3;
                    break;
                }

                irr.set_surface(track_mode, pv.tilt, pv.azimuth, pv.rotlim,
                    pv.type == ONE_AXIS_BACKTRACKING, // backtracking mode
                    pv.gcr, 0.0, 0.0, false, 0.0);

                int code = irr.calc();

                //create variables to store outputs
                double solazi, solzen, solalt, aoi, stilt, sazi, rot, btd;
                int sunup;
                double ibeam = 0.0, iskydiff = 0.0, ignddiff = 0.0, irear = 0.0;
                double poa = 0.0, tpoa = 0.0, tmod = 0.0, dc = 0.0, ac = 0.0;

                irr.get_sun(&solazi, &solzen, &solalt, nullptr, nullptr, nullptr, &sunup, nullptr, nullptr, nullptr); //nullptr used when you don't need to retrieve the output
                irr.get_angles(&aoi, &stilt, &sazi, &rot, &btd);
                irr.get_poa(&ibeam, &iskydiff, &ignddiff, nullptr, nullptr, nullptr); //nullptr used when you don't need to retrieve the output
                irr.get_optional(&elev, &pres, &t_amb);

                if (module.bifaciality > 0)
                {
                    irr.calc_rear_side(bifacialTransmissionFactor, 1, module.length * pv.nmody);
                    irear = irr.get_poa_rear() * module.bifaciality; //total rear irradiance is returned, so must multiply module bifaciality
                }

                if (-1 == code)
                {
                    log(util::format("Beam irradiance exceeded extraterrestrial value at record [year:%d month:%d day:%d hour:%d minute:%lg].",
                        wf.year, wf.month, wf.day, wf.hour, wf.minute));
                }
                else if (0 != code)
                    throw exec_error("pvwattsv8",
                        util::format("Failed to process irradiation on surface (message: %s) [year:%d month:%d day:%d hour:%d minute:%lg].",
                            irr.getErrorMessage().c_str(), wf.year, wf.month, wf.day, wf.hour, wf.minute));

                p_sunup[idx] = (ssc_number_t)sunup;
                p_aoi[idx] = (ssc_number_t)aoi;

                double shad_beam = 1.0;
                if (shad.fbeam(hour_of_year, wf.minute, solalt, solazi))
                    shad_beam = shad.beam_shade_factor();

                p_shad_beam[idx] = (ssc_number_t)shad_beam;
                p_ss_beam[idx] = (ssc_number_t)1.0;
                p_ss_sky_diffuse[idx] = (ssc_number_t)1.0;
                p_ss_gnd_diffuse[idx] = (ssc_number_t)1.0;

                if (sunup > 0)
                {
                    // save the total available POA for the loss diagram
                    if (y == 0 && wdprov->annualSimulation()) ld("poa_nominal") += (ibeam + iskydiff + ignddiff) * wm2_to_wh;
                    if (y == 0 && wdprov->annualSimulation()) ld("poa_loss_bifacial") += (-irear) * wm2_to_wh;


                    // check for wind stowing on trackers
                    if ((pv.type == ONE_AXIS
                        || pv.type == ONE_AXIS_BACKTRACKING
                        || pv.type == TWO_AXIS)
                        && std::isfinite(wf.wspd) && wf.wspd > 0
                        && std::isfinite(wstow)
                        && enable_wind_stow)
                    {
                        double gust = gf * wf.wspd;

                        if (gust > wstow)
                        {
                            // save poa before going into stow position
                            double poa_no_stow = ibeam + iskydiff + ignddiff;

                            if (pv.type == TWO_AXIS)
                            {
                                // two axis tracker stows at the horizontal position
                                // easiest way to do this in two dimensions is to set it as a flat fixed tilt system
                                // because the force to stow flag only fixes one rotation angle, not both
                                irr.set_surface(irrad::FIXED_TILT, // tracking 0=fixed
                                    0, 180, // tilt, azimuth
                                    0, 0, 0.4, 0.0, 0.0, false, 0.0); // rotlim, bt, gcr, force to stow, stow angle
                            }
                            else
                            {
                                // one axis tracker stows at a prescribed rotation angle,
                                // but still need to consider the rotation axis tilt and azimuth
                                double stow_angle = std::abs(wind_stow_angle_deg);
                                if (rot < 0) stow_angle = -stow_angle;  // go to stow in the same direction of current tracker position

                                irr.set_surface(irrad::SINGLE_AXIS, pv.tilt, pv.azimuth,
                                    stow_angle, // rotation angle limit, the forced stow position
                                    false, // backtracking mode
                                    pv.gcr, 0.0, 0.0,
                                    true, stow_angle  // force tracker to the rotation limit (stow_angle here)
                                );
                            }

                            irr.calc(); // recalculate POA and aoi, and rear side irradiance if bifacial, in the new stow position

                            double irear_stow = 0.0;
                            if (module.bifaciality > 0)
                            {
                                irr.calc_rear_side(bifacialTransmissionFactor, 1, module.length * pv.nmody);
                                irear_stow = irr.get_poa_rear() * module.bifaciality; //total rear irradiance is returned, so must multiply module bifaciality
                            }

                            irr.get_angles(&aoi, &stilt, &sazi, &rot, &btd);
                            irr.get_poa(&ibeam, &iskydiff, &ignddiff, nullptr, nullptr, nullptr); //nullptr used when you don't need to retrieve the output
                            double poa_stow = ibeam + iskydiff + ignddiff;

                            double stow_loss = (poa_no_stow - poa_stow) + (irear - irear_stow);
                            if (y == 0 && wdprov->annualSimulation()) ld("poa_loss_tracker_stow") += stow_loss * wm2_to_wh;
                            irear = irear_stow;
                            tracker_stowing = true;
                        }
                    }

                    // apply hourly external shading factors to beam (if none enabled, factors are 1.0)
                    if (y == 0 && wdprov->annualSimulation()) ld("poa_loss_ext_beam_shade") += ibeam * (1.0 - shad_beam) * wm2_to_wh;
                    ibeam *= shad_beam;

                    // apply hourly external sky diffuse shading factor (specified as constant, nominally 1.0 if disabled in UI)
                    if (y == 0 && wdprov->annualSimulation()) ld("poa_loss_ext_diff_shade") += (iskydiff + ignddiff) * (1.0 - shad.fdiff()) * wm2_to_wh;
                    iskydiff *= shad.fdiff();

                    // also applies to back irradiance if sky is blocked
                    irear *= shad.fdiff();

                    // save the unselfshaded beam irradiance if nonlinear losses are calculated
                    // to avoid double counting the beam irradiance loss when calculating module power output
                    double ibeam_unselfshaded = ibeam;

                    // calculate any self-shading effects in fixed or tracking regular row systems
                    double f_nonlinear = 1.0; //nonlinear shading factor, 1 for no shading
                    double Fskydiff = 1.0; //shading factor for sky diffuse, 1 for no shading
                    double Fgnddiff = 1.0; //shading factor for ground-reflected diffuse, 1 for no shading


                    if (en_self_shading) //shading applies in each of these three cases- see reference implementation in pvsamv1
                        //&& (pv.nrows >= 10) // note that enabling self-shading for small systems might be suspicious
                        // because the intent of the self-shading algorithms used here are to apply to large systems
                        // however, some testing of the self-shading algorithms for smaller systems doesn't reveal any wildly wrong behavior,
                        // so enabling it for all systems sizes to prevent confusion to users
                    {
                        // first calculate linear shading for one-axis trackers for use in self-shading algorithms
                        double shad1xf = 0.0; // default: zero shade fraction
                        if (pv.type == ONE_AXIS)
                        {
                            shad1xf = shadeFraction1x(solazi, solzen, pv.tilt, pv.azimuth, pv.gcr, rot, 0.0, 0.0);
                        }

                        // run self-shading calculations for both FIXED_RACK and ONE_AXIS because the non-linear derate applies in both cases (below)
                        ssinputs ssin;
                        ssin.nstrx = (int)(((double)pv.nmodx) / pv.nmodperstr);
                        ssin.nmodx = pv.nmodx;
                        ssin.nmody = pv.nmody;
                        ssin.nrows = pv.nrows;
                        ssin.length = module.length;
                        ssin.width = module.width;
                        ssin.mod_orient = 0; // portrait module orientation
                        ssin.str_orient = 1; // horizontal stringing
                        ssin.row_space = pv.row_spacing;
                        ssin.ndiode = module.ndiode;
                        ssin.Vmp = module.vmp;
                        ssin.mask_angle_calc_method = 0; // worst case mask angle assumption
                        ssin.FF0 = module.ff;
                        ssoutputs ssout;

                        if (!ss_exec(ssin,
                            stilt, sazi, //surface tilt and azimuth
                            solzen, solazi, //solar zenith and azimuth
                            wf.dn, // Gb_nor (e.g. DNI)
                            wf.df, //Gdh (e.g. DHI)
                            ibeam * (1.0 - shad1xf), // Gb_poa
                            iskydiff, //poa_sky
                            ignddiff, // poa_gnd
                            alb,
                            pv.type == ONE_AXIS, // is tracking system?
                            module.type == THINFILM,  // is linear shading? (only with long cell thin films)
                            shad1xf,
                            ssSkyDiffuseTable,
                            ssout))
                        {
                            throw exec_error("pvwattsv8", util::format("Self-shading calculation failed at %d.", (int)idx_life));
                        }

                        // fixed tilt system with linear self-shading: beam is derated by fixed shade fraction
                        // fixed tilt non-linear self-shading, beam would NOT usually be derated, because non-linear dc derate accounts for it
                        // however, to be able to distinguish between irradiance and non-linear shading in loss diagram:
                        // we will apply it here, BUT, we will use an un-self-shaded irradiance in the power calculations later
                        // so that the loss isn't double counted.
                        if (pv.type == FIXED_RACK)
                        {
                            if (y == 0 && wdprov->annualSimulation()) ld("poa_loss_self_beam_shade") += ibeam * ssout.m_shade_frac_fixed * wm2_to_wh;
                            ibeam *= (1 - ssout.m_shade_frac_fixed);
                            p_ss_beam[idx] = (ssc_number_t)(1 - ssout.m_shade_frac_fixed);
                        }

                        // one-axis true tracking system with linear self-shading: beam is derated by linear shade fraction for 1-axis trackers
                        // one-axis non-linear self-shading, beam would NOT usually be derated, because non-linear dc derate accounts for it
                        // however, to be able to distinguish between irradiance and non-linear shading in loss diagram:
                        // we will apply it here, BUT, we will use an un-self-shaded irradiance in the power calculations later
                        // so that the loss isn't double counted.
                        else if (pv.type == ONE_AXIS)
                        {
                            if (y == 0 && wdprov->annualSimulation()) ld("poa_loss_self_beam_shade") += ibeam * shad1xf * wm2_to_wh;
                            ibeam *= (1 - shad1xf);
                            p_ss_beam[idx] = (ssc_number_t)(1 - shad1xf);
                        }

                        // for non-linear self-shading (fixed and one-axis, but not backtracking)
                        // the non-linear dc derate is calculated and we need to save it for later
                        /*if ((pv.type == FIXED_RACK || pv.type == ONE_AXIS) && module.type != THINFILM)
                        {
                            f_nonlinear = ssout.m_dc_derate;
                        }*/ //disconnecting non-linear shading for now due to possible bug in non-linear shading algorithm resulting in 9% loss in annual energy compared to linear case for large systems

                        // for backtracked systems, there is no beam irradiance reduction or non-linear DC derate
                        // however, sky and ground-reflected diffuse are still blocked, so apply those to everything below

                        // always derate diffuse for any self-shaded system,
                        // due to inter-row blocking of sky and ground view factors.
                        // the derates are calculated by the lib_pvshade.cpp:diffuse_reduce() function and are
                        // purely geometric - apply independent of whether DC derate loss is linear or nonlinear
                        Fskydiff = ssout.m_diffuse_derate;
                        Fgnddiff = ssout.m_reflected_derate;

                    }

                    // apply derate factors to diffuse
                    if (Fskydiff >= -0.00001 && Fskydiff <= 1.00001) //include tolerances due to double representation
                    {
                        if (y == 0 && wdprov->annualSimulation()) ld("poa_loss_self_diff_shade") += (1.0 - Fskydiff) * (iskydiff + irear) * wm2_to_wh; //irear is zero if not bifacial
                        iskydiff *= Fskydiff;
                        irear *= Fskydiff;
                        p_ss_sky_diffuse[idx] = (ssc_number_t)Fskydiff;
                    }
                    else log(util::format("Sky diffuse reduction factor invalid at time %lg: fskydiff=%lg, stilt=%lg.", idx, Fskydiff, stilt), SSC_NOTICE, (float)idx);

                    if (Fgnddiff >= -0.00001 && Fgnddiff <= 1.00001) //include tolerances due to double representation
                    {
                        if (y == 0 && wdprov->annualSimulation()) ld("poa_loss_self_diff_shade") += (1.0 - Fgnddiff) * ignddiff * wm2_to_wh;
                        ignddiff *= Fgnddiff;
                        p_ss_gnd_diffuse[idx] = (ssc_number_t)Fgnddiff;
                    }
                    else log(util::format("Ground diffuse reduction factor invalid at time %lg: fgnddiff=%lg, stilt=%lg.", idx, Fgnddiff, stilt), SSC_NOTICE, (float)idx);


                    // apply soiling loss to the total effective POA
                    if (is_assigned("soiling"))
                    {
                        double soiling_f = 0.0;
                        if (soiling_len == 1)
                            soiling_f = soiling[0] * 0.01; //convert from percentage to decimal
                        else if (soiling_len == 12)
                            soiling_f = soiling[wf.month - 1] * 0.01; //convert from percentage to decimal
                        else if (soiling_len == nrec)
                            soiling_f = soiling[idx] * 0.01; //convert from percentage to decimal
                        else
                            throw exec_error("pvwattsv8", "Soiling input array must have 1, 12, or nrecords values.");

                        // report soiling factor output
                        p_soiling_f[idx] = (ssc_number_t)soiling_f;

                        if (y == 0 && wdprov->annualSimulation()) ld("poa_loss_soiling") += (ibeam + iskydiff + ignddiff) * soiling_f * wm2_to_wh;

                        ibeam *= (1.0 - soiling_f);
                        iskydiff *= (1.0 - soiling_f);
                        ignddiff *= (1.0 - soiling_f);
                        // note: assume no soiling on rear side?
                    }
                    else
                        if (y == 0 && wdprov->annualSimulation()) ld("poa_loss_soiling") = 0;

                    // now add up total effective POA, accounting for external and self shading
                    double poa_front = ibeam + iskydiff + ignddiff;
                    poa = poa_front + irear; //irear is zero if not bifacial

                    // dc power nominal before any losses
                    double dc_nom = pv.dc_nameplate * poa / 1000; // Watts_DC * (POA W/m2 / 1000 W/m2 STC value );
                    if (y == 0 && wdprov->annualSimulation()) ld("dc_nominal") += dc_nom * ts_hour; //ts_hour required to correctly convert to Wh for subhourly data

                    // nonlinear dc loss from shading
                    if (y == 0 && wdprov->annualSimulation()) ld("dc_loss_nonlinear") += dc_nom * (1.0 - f_nonlinear) * ts_hour; //ts_hour required to correctly convert to Wh for subhourly data

                    // dc losses
                    if (y == 0 && wdprov->annualSimulation()) ld("dc_loss_other") += dc_nom * pv.dc_loss_percent * 0.01 * ts_hour; //ts_hour required to correctly convert to Wh for subhourly data
                    double f_losses = (1 - pv.dc_loss_percent * 0.01);

                    // run the snow loss model
                    double f_snow = 1.0;
                    double snow_cover = 0;
                    if (en_snowloss)
                    {
                        float smLoss = 0.0f;
                        if (!snowmodel.getLoss(
                            (float)(poa_front + irr.get_poa_rear()), (float)stilt,
                            (float)wf.wspd, (float)wf.tdry, (float)wf.snow,
                            sunup, (float)ts_hour,
                            smLoss))
                        {
                            if (!snowmodel.good)
                                throw exec_error("pvwattsv8", snowmodel.msg);
                        }
                        if (poa != 0) smLoss *= poa_front / poa;
                        f_snow = (1.0 - smLoss);
                        snow_cover = snowmodel.coverage;
                    }

                    // dc snow loss
                    if (y == 0 && wdprov->annualSimulation()) ld("dc_loss_snow") += dc_nom * (1.0 - f_snow) * ts_hour; //ts_hour required to correctly convert to Wh for subhourly data

                    // calculate actual DC power now with all the derates/losses
                    // remember, that for non-linear self-shading, we don't use derated ibeam for module power
                    // calculations because total loss is encapsulated in the DC derate
                    // but, we derated it earlier so we could break out the beam shading from the non-linear component.
                    // SO: if dcshadedderate < 1.0, then use ibeam_noselfshade
                    double poa_for_power =
                        (f_nonlinear < 1.0 && poa > 0.0) // if there is a nonlinear self-shading derate
                        ? (ibeam_unselfshaded + iskydiff + ignddiff) // then use the unshaded beam to calculate eff POA for power calc but adjust for IAM and spectral
                        : (ibeam + iskydiff + ignddiff); // otherwise, use the 'linearly' derated beam irradiance

                    // set up inputs to module model for both temperature and subsequent CEC module model calculations
                    // bifaciality is applied to irear on line 885 above for fixed arrays and line 962 for trackers - so, module.bifaciality should not be applied again here - SAM issue 1151
                    //pvinput_t in((f_nonlinear < 1.0 && poa > 0.0) ? ibeam_unselfshaded : ibeam, iskydiff, ignddiff, irear* module.bifaciality, poa_for_power,
                    pvinput_t in((f_nonlinear < 1.0 && poa > 0.0) ? ibeam_unselfshaded : ibeam, iskydiff, ignddiff, irear, poa_for_power,
                        wf.tdry, wf.tdew, wf.wspd, wf.wdir, wf.pres,
                        solzen, aoi, elev,
                        stilt, sazi,
                        ((double)wf.hour) + wf.minute / 60.0,
                        irrad::DN_DF, false);

                    // module temperature calculations
                    if (!modTempCalc(in, mod, -1.0, tmod)) throw exec_error("pvwattsv8", util::format("Module temperature calculation failed at index %d.", (int)idx_life)); //calculate temperature at MPP (-1.0 flag determines this)

                    // calculate transmitted POA (tpoa) to report as an output
                    if (aoi > AOI_MIN && aoi < AOI_MAX && poa_front > 0)
                    {
                        tpoa = calculateIrradianceThroughCoverDeSoto(
                            aoi, stilt, ibeam, iskydiff, ignddiff, en_mlm == 0 && module.ar_glass);
                        if (tpoa < 0.0) tpoa = 0.0;
                        if (tpoa > poa) tpoa = poa_front;
                    }

                    // DC power conversion calculations
                    if (en_mlm) // hidden feature for pvwatts SDK users contributed by Aron Dobos
                    {
                        // adjustments to irradiance that are covered as part of the CEC model, but not by the mlm model
                        // module cover effects
                        double f_cover = 1.0;
                        f_cover = tpoa / poa_front;
                        // spectral correction via air mass modifier
                        double f_AM = air_mass_modifier(solzen, elev, AMdesoto);

                        // derate poa irradiance and record losses for loss diagram
                        poa_for_power *= f_cover * f_AM; //derate irradiance for module cover and spectral effects
                        poa_for_power += irear * f_AM; // backside irradiance model already includes back cover effects
                        if (y == 0 && wdprov->annualSimulation()) ld("dc_loss_cover") += (1 - f_cover) * dc_nom * ts_hour; //ts_hour required to correctly convert to Wh for subhourly data
                        if (y == 0 && wdprov->annualSimulation()) ld("dc_loss_spectral") += (1 - f_AM) * dc_nom * ts_hour; //ts_hour required to correctly convert to Wh for subhourly data*/

                        // single diode model per PVsyst using representative module parameters for each module type
                        double P_single_module_sdm = sdmml_power(sdm, poa_for_power, tmod);
                        dc = P_single_module_sdm * pv.dc_nameplate / (sdm.Vmp * sdm.Imp);
                    }
                    else // normal PVWattsV8 DC conversion module model
                    {

                        // set up output structure for module model
                        pvoutput_t out(0, 0, 0, 0, 0, 0, 0, 0);
                        // call the module model
                        if (!mod(in, tmod, -1.0, out)) throw exec_error("pvwattsv8", util::format("Module power calculation failed at index %d.", (int)idx_life));
                        // scale the power output for a single module (out.Power) to the actual system size-
                        // divide the DC nameplate input by the "single module" nameplate (Vmp * Imp) to get a fractional number of modules in the system, and multiply by that fraction
                        dc = out.Power * pv.dc_nameplate / (mod.Vmp * mod.Imp);
                    }

                    // apply common DC losses here (independent of module model)
                    dc *= f_nonlinear * f_snow * f_losses;
                    p_dcshadederate[idx] = (ssc_number_t)f_nonlinear;
                    //p_dcsnowderate[idx] = (ssc_number_t)f_snow; // output is percentage - calculated value is derate
                    p_dcsnowderate[idx] =  (1.0 - f_snow) * 100.0;
                    p_snowcover[idx] = snow_cover;

                    // apply DC degradation
                    dc *= degradationFactor[y];

                    // inverter calculations
                    sandia_inverter_t inverter;
                    inverter.Paco = pv.ac_nameplate;
                    inverter.Pdco = inverter.Paco / (pv.inv_eff_percent * 0.01); // get inverter DC rating by dividing AC rating by efficiency
                    // set both Vdco and Vdc to zero. the assumption we make for voltages are irrelevant as long as C1, C2, and C3 are zero
                    inverter.Vdco = 0.0;
                    inverter.Pntare = 0.0; // simplifying assumption that inverter has no nighttime losses
                    // default values for C1, C2, C3 are zero per Sandia documentation: https://pvpmc.sandia.gov/modeling-steps/dc-to-ac-conversion/sandia-inverter-model/
                    // setting these to 0 results in similar inverter output to pvwattsv5
                    inverter.C1 = 0.0;
                    inverter.C2 = 0.0;
                    inverter.C3 = 0.0;

                    // Set based on market per ssc issue 870
                    inverter.Pso = 0.0; // simplifying assumption that the inverter can always operate - needed for knee
                    inverter.C0 = 0.0; // needed for curvature - voltage needed for this parameter to be used

                    if (pv.dc_nameplate < 10) { // Residential
                        inverter.Pso = 0.002246 * inverter.Paco;
                    }
                    else if (pv.dc_nameplate < 1000) { // Commercial >= 10kW
                        inverter.Pso = 0.002478 * inverter.Paco;
                    }
                    else { // Utility >= 1MW
                        inverter.Pso = 0.004931 * inverter.Paco;
                    }

                    // call inverter function
                    // set operating voltage (second parameter) to zero. the assumption we make for voltages are irrelevant as long as C0, C1, C2, and C3 are zero (set above)
                    // use null pointers for results that we don't care about
                    if (!inverter.acpower(dc, 0.0, &ac, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr)) throw exec_error("pvwattsv8", util::format("Inverter power calculation failed at index %d.", (int)idx_life));

                    // track inverter efficiency
                    p_inv_eff[idx] = (dc > 0) ? ac/dc * 100 : 0.0;

                    // record AC results
                    if (y == 0 && wdprov->annualSimulation()) ld("ac_nominal") += dc * ts_hour; //ts_hour required to correctly convert to Wh for subhourly data
                    if (y == 0 && wdprov->annualSimulation()) ld("ac_loss_efficiency") += (dc - ac) * ts_hour; //ts_hour required to correctly convert to Wh for subhourly data

                    // power clipping
                    double cliploss = ac > pv.ac_nameplate ? ac - pv.ac_nameplate : 0.0;
                    if (y == 0 && wdprov->annualSimulation()) ld("ac_loss_inverter_clipping") += cliploss * ts_hour; //ts_hour required to correctly convert to Wh for subhourly data
                    ac -= cliploss;

                    // make sure no negative AC values (no parasitic nighttime losses calculated for PVWatts)
                    if (ac < 0) ac = 0;
                }
                else
                {
                    poa = 0;
                    tpoa = 0;
                    tmod = wf.tdry;
                    dc = 0;
                    ac = 0;
                }

                // transformer loss (night and day)
                double iron_loss = pv.xfmr_nll_f * pv.xfmr_rating;
                double winding_loss = pv.xfmr_ll_f * ac * (ac / pv.xfmr_rating);
                double xfmr_loss = iron_loss + winding_loss;
                if (y == 0 && wdprov->annualSimulation()) ld("ac_loss_transformer") += xfmr_loss * ts_hour; //ts_hour required to correctly convert to Wh for subhourly data
                ac -= xfmr_loss;

                p_stow[idx] = (tracker_stowing ? 1.0 : 0.0);
                p_shad_beam[idx] = (ssc_number_t)shad_beam; // might be updated by 1 axis self shading so report updated value

                p_poa[idx] = (ssc_number_t)poa; // W/m2
                p_tpoa[idx] = (ssc_number_t)tpoa;  // W/m2
                p_tmod[idx] = (ssc_number_t)tmod;
                p_dc[idx] = (ssc_number_t)dc; // power, Watts
                p_ac_pre_adjust[idx] = (ssc_number_t)ac; //power, Watts
                p_ac[idx] = (ssc_number_t)(ac * haf(wdprov->annualSimulation() ? hour_of_year : idx)); // power, Watts

                // accumulate hourly energy (kWh) (was initialized to zero when allocated)
                p_gen[idx_life] = (ssc_number_t)(p_ac[idx] * util::watt_to_kilowatt);


                if (y == 0 && wdprov->annualSimulation()) { //report first year annual energy
                    annual_kwh += p_gen[idx] / step_per_hour;
                }

                if (y == 0 && wdprov->annualSimulation()) ld("ac_loss_adjustments") += ac * (1.0 - haf(wdprov->annualSimulation() ? hour_of_year : idx)) * ts_hour; //ts_hour required to correctly convert to Wh for subhourly data
                if (y == 0 && wdprov->annualSimulation()) ld("ac_delivered") += ac * haf(wdprov->annualSimulation() ? hour_of_year : idx) * ts_hour; //ts_hour required to correctly convert to Wh for subhourly data

                idx_life++;
            }

            if (n_alb_errs > 0)
                log(util::format("Weather file albedo has %d invalid values, using monthly value", (int)n_alb_errs), SSC_WARNING);

            wdprov->rewind();
        }

        if (wdprov->annualSimulation())
        {
            ssc_number_t* p_annual_energy_dist_time = gen_heatmap(this, (double)step_per_hour);

            accumulate_monthly_for_year("gen", "monthly_energy", ts_hour, step_per_hour);
            accumulate_annual_for_year("gen", "annual_energy", ts_hour, step_per_hour);

            accumulate_monthly("dc", "dc_monthly", 0.001 * ts_hour);
            accumulate_monthly("ac", "ac_monthly", 0.001 * ts_hour);

            ssc_number_t* poam = accumulate_monthly("poa", "poa_monthly", 0.001 * ts_hour); // convert to energy
            ssc_number_t* solrad = allocate("solrad_monthly", 12);
            ssc_number_t solrad_ann = 0;
            for (int m = 0; m < 12; m++)
            {
                solrad[m] = poam[m] / util::nday[m];
                solrad_ann += solrad[m];
            }
            assign("solrad_annual", var_data(solrad_ann / 12));

            accumulate_annual("ac", "ac_annual", 0.001 * ts_hour);
            accumulate_annual("ac_pre_adjust", "ac_annual_pre_adjust", 0.001 * ts_hour);

            // metric outputs
            double kWhperkW = util::kilowatt_to_watt * annual_kwh / pv.dc_nameplate;
            assign("kwh_per_kw", var_data((ssc_number_t)kWhperkW));
            assign("capacity_factor", var_data((ssc_number_t)(kWhperkW / 87.6))); //convert from kWh/kW to percent, so divide by 8760 hours and multiply by 100 percent
            assign("capacity_factor_ac", var_data((ssc_number_t)util::kilowatt_to_watt* annual_kwh / pv.ac_nameplate / 87.6)); //same conversion as above
        }

        // location outputs
        assign("location", var_data(hdr.location));
        assign("city", var_data(hdr.city));
        assign("state", var_data(hdr.state));
        assign("lat", var_data((ssc_number_t)hdr.lat));
        assign("lon", var_data((ssc_number_t)hdr.lon));
        assign("tz", var_data((ssc_number_t)hdr.tz));
        assign("elev", var_data((ssc_number_t)elev));
        assign("percent_complete", var_data((ssc_number_t)percent));

        double gcr_for_land = pv.gcr;
        if (gcr_for_land < 0.01) gcr_for_land = 1.0;
        double landf = is_assigned("landf") ? as_number("landf") : 1.0f;
        assign("land_acres", var_data((ssc_number_t)(landf * module_m2 / gcr_for_land * 0.0002471)));

        // for battery model, specify a number of inverters
        assign("inverter_efficiency", var_data((ssc_number_t)(as_double("inv_eff"))));
        assign("system_capacity_ac", var_data((ssc_number_t)pv.ac_nameplate / 1000.0));

        if (en_snowloss && snowmodel.badValues > 0)
            log(util::format("The snow model has detected %d bad snow depth values (less than 0 or greater than 610 cm). These values have been set to zero.", snowmodel.badValues), SSC_WARNING);

        // assign loss factors to outputs (kwh)
        if (wdprov->annualSimulation())
        {
            if (!ld.assign(this, "lossd_"))
                log(ld.errormsg(), SSC_WARNING);
        }
    }
};

DEFINE_MODULE_ENTRY(pvwattsv8, "PVWatts V8 - integrated hourly weather reader and PV system simulator.", 3)
