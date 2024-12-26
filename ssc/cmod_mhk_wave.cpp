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


#include "core.h"
#include "common.h"
#include <vector>
#include <algorithm> 

static var_info _cm_vtab_mhk_wave[] = {
	//   VARTYPE			DATATYPE			NAME									LABEL																UNITS           META            GROUP              REQUIRED_IF					CONSTRAINTS					UI_HINTS	
	{ SSC_INPUT,            SSC_NUMBER,         "wave_resource_model_choice",           "Hourly or JPD wave resource data",                                 "0/1",             "",             "MHKWave",          "?=0",                         "INTEGER",                  "" },
    { SSC_INPUT,			SSC_MATRIX,			"wave_resource_matrix",					"Frequency distribution of wave resource as a function of Hs and Te","",			"",             "MHKWave",			"?",						"",							"" },
    { SSC_INPUT,            SSC_TABLE,             "wave_resource_data",                   "Array input of wave_resource_matrix (JPD) or time series (significant_wave_height and energy_period) data", "", "", "MHKWave", "?",             "",                         "" },
    { SSC_INPUT,            SSC_ARRAY,          "significant_wave_height",              "Significant wave height time series data",                         "m",            "",             "MHKWave",          "?", "",                 ""   },
    { SSC_INPUT,            SSC_ARRAY,          "energy_period",                   "Wave period time series data",                                     "s",            "",             "MHKWave",          "?", "",                 ""   },
    { SSC_INPUT,			SSC_MATRIX,			"wave_power_matrix",					"Wave Power Matrix",												"",				"",             "MHKWave",			"*",						"",							"" },
//	{ SSC_INPUT,			SSC_NUMBER,			"annual_energy_loss",					"Total energy losses",												"%",			"",             "MHKWave",			"?=0",						"",							"" },
	//{ SSC_INPUT,			SSC_NUMBER,			"calculate_capacity",					"Calculate capacity outside UI?",									"0/1",			"",             "MHKWave",          "?=1",                      "INTEGER,MIN=0,MAX=1",      "" },
	{ SSC_INPUT,			SSC_NUMBER,			"number_devices",						"Number of wave devices in the system",								"",				"",             "MHKWave",          "?=1",                      "INTEGER",			    	"" },
	{ SSC_INPUT,			SSC_NUMBER,			"system_capacity",						"System Nameplate Capacity",										"kW",			"",				"MHKWave",			"?=0",						"",							"" },
    { SSC_INPUT,           SSC_NUMBER,           "number_hours",                "Number of hours in wave time series",                                        "",     "",                       "MHKWave",      "?",                        "",                            "" },
    { SSC_INPUT,           SSC_NUMBER,           "number_records",                "Number of records in wave time series",                                        "",     "",                       "MHKWave",      "?",                        "",                            "" },

    { SSC_INPUT,			SSC_NUMBER,			"device_rated_power",				"Rated capacity of device",													"kW",			"",				"MHKWave",			"*",		"",						"" },
    { SSC_INPUT,			SSC_NUMBER,			"fixed_charge_rate",						"FCR from LCOE Cost page",									"",				"",             "MHKWave",         "?=1",                      "",				"" },
    { SSC_INPUT,			SSC_NUMBER,			"device_costs_total",						"Device costs",									"$",				"",             "MHKWave",         "?=1",                      "",				"" },
    { SSC_INPUT,			SSC_NUMBER,			"balance_of_system_cost_total",						"BOS costs",									"$",				"",             "MHKWave",         "?=1",                      "",				"" },
    { SSC_INPUT,			SSC_NUMBER,			"financial_cost_total",						"Financial costs",									"$",				"",             "MHKWave",         "?=1",                      "",				"" },
    { SSC_INPUT,			SSC_NUMBER,			"total_operating_cost",						"O&M costs",									"$",				"",             "MHKWave",         "?=1",                      "",				"" },

    { SSC_INPUT,        SSC_NUMBER,      "system_use_lifetime_output",                  "Generic lifetime simulation",                               "0/1",      "",                              "Lifetime",             "?=0",                        "INTEGER,MIN=0,MAX=1",          "" },
    { SSC_INPUT,        SSC_NUMBER,      "analysis_period",                             "Lifetime analysis period",                             "years",    "",                              "Lifetime",             "system_use_lifetime_output=1",   "",                             "" },
    { SSC_INPUT,        SSC_ARRAY,       "generic_degradation",                              "Annual AC degradation",                            "%/year",   "",                              "Lifetime",             "system_use_lifetime_output=1",   "",                             "" },


    // losses
	{ SSC_INPUT,			SSC_NUMBER,			"loss_array_spacing",				"Array spacing loss",													"%",			"",				"MHKWave",			"*",		"",						"" },
	{ SSC_INPUT,			SSC_NUMBER,			"loss_resource_overprediction",				"Resource overprediction loss",													"%",			"",				"MHKWave",			"*",		"",						"" },
	{ SSC_INPUT,			SSC_NUMBER,			"loss_transmission",				"Transmission losses",													"%",			"",				"MHKWave",			"*",		"",						"" },
	{ SSC_INPUT,			SSC_NUMBER,			"loss_downtime",				"Array/WEC downtime loss",													"%",			"",				"MHKWave",			"*",		"",						"" },
	{ SSC_INPUT,			SSC_NUMBER,			"loss_additional",				"Additional losses",													"%",			"",				"MHKWave",			"*",		"",						"" },

    { SSC_INPUT,        SSC_ARRAY,       "year",                    "Year",                             "yr",     "",                      "MHKWave",      "",                       "",               "" },
    { SSC_INPUT,        SSC_ARRAY,       "month",                   "Month",                            "mn",     "1-12",                  "MHKWave",      "",                       "",                          "" },
    { SSC_INPUT,        SSC_ARRAY,       "day",                     "Day",                              "dy",     "1-365",                 "MHKWave",      "",                       "",                          "" },
    { SSC_INPUT,        SSC_ARRAY,       "hour",                    "Hour",                             "hr",     "0-23",                  "MHKWave",      "",                       "",                          "" },
    { SSC_INPUT,        SSC_ARRAY,       "minute",                  "Minute",                           "min",    "0-59",                  "MHKWave",      "",                       "",                          "" },


	{ SSC_OUTPUT,			SSC_NUMBER,			"device_average_power",					"Average power production of a single device",											"kW",			"",				"MHKWave",			"*",						"",							"" },
	{ SSC_OUTPUT,			SSC_NUMBER,			"annual_energy",						"Annual AC energy in Year 1",											"kWh",			"",				"MHKWave",			"",						"",							"" },
    { SSC_OUTPUT,           SSC_ARRAY,          "energy_hourly_kWh",                        "Energy production of array",                                            "kWh",          "", "Time Series",          "wave_resource_model_choice=1",                        "",          "" },
    { SSC_OUTPUT,           SSC_ARRAY,          "energy_hourly_kW",                        "Power output of array",                                            "kW",          "", "Time Series",          "wave_resource_model_choice=1",                        "",          "" },

    { SSC_OUTPUT,           SSC_ARRAY,          "gen",                        "System power generated",                                            "kW",          "", "Time Series",          "",                        "",          "" },

    { SSC_OUTPUT,           SSC_ARRAY,          "sig_wave_height_index_mat",            "Wave height index locations for time series",                      "m",                         "", "MHKWave",          "wave_resource_model_choice=1",                        "",          "" },
    { SSC_OUTPUT,           SSC_ARRAY,          "sig_wave_height_data",            "Significant wave height time series data",                      "m",                         "", "MHKWave",          "wave_resource_model_choice=1",                        "",          "" },

    { SSC_OUTPUT,           SSC_ARRAY,          "energy_period_index_mat",            "Wave period index locations for time series",                      "s",                         "", "MHKWave",          "wave_resource_model_choice=1",                        "",          "" },
    { SSC_OUTPUT,           SSC_ARRAY,          "energy_period_data",            "Energy period time series data",                      "s",                         "", "MHKWave",          "wave_resource_model_choice=1",                        "",          "" },

    { SSC_OUTPUT,           SSC_ARRAY,          "wave_power_index_mat",            "Wave power for time series",                      "kW",                         "", "MHKWave",          "wave_resource_model_choice=1",                        "",          "" },
    { SSC_OUTPUT,			SSC_NUMBER,			"capacity_factor",						"Capacity factor",													"%",			"",				"MHKWave",			"*",						"",							"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"numberRecords",						"Number of Records",													"",			"",				"MHKWave",			"",						"",							"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"numberHours",						"Number of Hours",													"",			"",				"MHKWave",			"",						"",							"" },

    { SSC_OUTPUT,			SSC_MATRIX,			"annual_energy_distribution",			"Annual energy production as function of Hs and Te",				"kWh",				"",				"MHKWave",			"",						"",							"" },
    { SSC_OUTPUT,			SSC_MATRIX,			"annual_energy_distribution_time",			"Annual energy production as function of Time",				"kWh",				"",				"MHKWave",			"",						"",							"" },

    { SSC_OUTPUT,			SSC_NUMBER,			"wave_resource_start_height",			"Wave height at which first non-zero wave resource value occurs (m)",				"",				"",				"MHKWave",			"wave_resource_model_choice=0",						"",							"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"wave_resource_start_period",			"Wave period at which first non-zero wave resource value occurs (s)",				"",				"",				"MHKWave",			"wave_resource_model_choice=0",						"",							"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"wave_resource_end_height",			"Wave height at which last non-zero wave resource value occurs (m)",				"",				"",				"MHKWave",			"wave_resource_model_choice=0",						"",							"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"wave_resource_end_period",			"Wave period at which last non-zero wave resource value occurs (s)",				"",				"",				"MHKWave",			"wave_resource_model_choice=0",						"",							"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"wave_power_start_height",			"Wave height at which first non-zero WEC power output occurs (m)",				"",				"",				"MHKWave",			"wave_resource_model_choice=0",						"",							"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"wave_power_start_period",			"Wave period at which first non-zero WEC power output occurs (s)",				"",				"",				"MHKWave",			"wave_resource_model_choice=0",						"",							"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"wave_power_end_height",			"Wave height at which last non-zero WEC power output occurs (m)",				"",				"",				"MHKWave",			"wave_resource_model_choice=0",						"",							"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"wave_power_end_period",			"Wave period at which last non-zero WEC power output occurs (s)",				"",				"",				"MHKWave",			"wave_resource_model_choice=0",						"",							"" },

    { SSC_OUTPUT,			SSC_NUMBER,			"total_capital_cost_kwh",           "Capital costs per unit annual energy",		"$/kWh",			"",				"MHKWave",			"",						"",						"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"total_device_cost_kwh",            "Device costs per unit annual energy",		"$/kWh",			"",				"MHKWave",			"",						"",						"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"total_bos_cost_kwh",               "Balance of system costs per unit annual energy",		"$/kWh",			"",				"MHKWave",			"",						"",						"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"total_financial_cost_kwh",         "Financial costs per unit annual energy",		"$/kWh",			"",				"MHKWave",			"",						"",						"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"total_om_cost_kwh",                "O&M costs per unit annual energy",		"$/kWh",			"",				"MHKWave",			"",						"",						"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"total_capital_cost_lcoe",          "Capital cost as percentage of overall LCOE",		"%",			"",				"MHKWave",			"",						"",						"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"total_device_cost_lcoe",           "Device cost",		"%",			"",				"MHKWave",			"",						"",						"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"total_bos_cost_lcoe",              "BOS cost",		"%",			"",				"MHKWave",			"",						"",						"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"total_financial_cost_lcoe",        "Financial cost",		"%",			"",				"MHKWave",			"",						"",						"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"total_om_cost_lcoe",               "O&M cost (annual)",		"%",			"",				"MHKWave",			"",						"",						"" },
    //Cost per KW
    { SSC_OUTPUT,			SSC_NUMBER,			"total_capital_cost_per_kw",							"Capital cost per kW",										"$/kW",			"",								"MHKCosts",			"",						"",							"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"total_device_cost_per_kw",							"Device cost per kW",										"$/kW",			"",								"MHKCosts",			"",						"",							"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"total_bos_cost_per_kw",							"Balance of Systems cost per kW",										"$/kW",			"",								"MHKCosts",			"",						"",							"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"total_financial_cost_per_kw",							"Financial cost per kW",										"$/kW",			"",								"MHKCosts",			"",						"",							"" },
    { SSC_OUTPUT,			SSC_NUMBER,			"total_operations_cost_per_kw",							"O&M cost per kW",										"$/kW",			"",								"MHKCosts",			"",						"",							"" },

    var_info_invalid
};

class wave_data_provider
{
public:


    std::string name; //Name of system where wave data is pulled from
    std::string city; //City where wave resource is located
    std::string state; //State where wave resource data is located
    std::string country; //Country where wave resource data is located
    std::string bathymetry; //Description of the 
    std::string sea_bed; //Description of the conditions on the sea bed for the wave resource location
    std::string data_source; //Which buoy did the data come from
    std::string notes; //Any additional notes
    int year = 0; //Year for teh wave resource data
    double lat = 0; //Latitude for the wave resource data
    double lon = 0; //Longitude for the wave resource data
    double tz  = 0; //Time zone for the wave resource data
    std::string nearby_buoy_number; //Nearby buoy number
    double average_power_flux = 0; //Average power flux 

    std::vector<int> types() { return m_dataid; }
    std::vector<double> wave_heights() { return m_sigwaveheight; }
    std::vector<double> wave_periods() { return m_waveperiod; }
    std::vector<int> wave_year() { return m_year; }
    std::vector<int> wave_month() { return m_month; }
    std::vector<int> wave_day() { return m_day; }
    std::vector<int> wave_hour() { return m_hour; }
    std::vector<int> wave_minute() { return m_minute; }
    //std::vector<double> wave_matrix() { return m_wave_resource_matrix_data; }
    util::matrix_t<double> wave_matrix() { return m_wave_resource_matrix_data; }
    std::vector<double> relativeHumidity() { return m_relativeHumidity; }
    size_t num_records() { return m_nRecords; }



    std::string error() { return m_errorMsg; }

    bool has_message() { return m_errorMsg.size() > 0; }
    std::string message() { return m_errorMsg; }

protected:
    std::vector<int> m_dataid;
    size_t m_nRecords = 2920;
    /// measurement height corresponding to each column header; same size as m_dataid
    std::vector<double> m_sigwaveheight;
    std::vector<double> m_waveperiod;
    std::vector<int> m_year;
    std::vector<int> m_month;
    std::vector<int> m_day;
    std::vector<int> m_hour;
    std::vector<int> m_minute;
    util::matrix_t<double> m_wave_resource_matrix_data;
    std::vector<double> m_relativeHumidity;
    std::string m_errorMsg;

};

class wavedata : public wave_data_provider
{
public:
    explicit wavedata(int wave_resource_model_choice, var_data* data_table);

    
    size_t nrecords(int wave_resource_model_choice);

    ssc_number_t get_number(var_data* v, const char* name);

    ssc_number_t* get_vector(var_data* v, const char* name, size_t* len);
    util::matrix_t<double> get_matrix(var_data* v, const char* var_name, size_t* nrows, size_t* ncols);
    std::string get_string(var_data* v, const char* name);

};

wavedata::wavedata(int wave_resource_model_choice, var_data* data_table) //wavedata class for specifying wave resource inputs in a table for pysam
{

    if (!data_table  || data_table->type != SSC_TABLE) //Datatype must be table
    {
        m_errorMsg = "wave data must be an SSC table variable with fields: (string): name, city, state, country, sea_bed, data_source, notes, (number): lat, lon, nearby_buoy_number, average_power_flux, bathymetry, tz, (array): significant_wave_height, energy_period";
        return;
    }

    lat = get_number(data_table, "lat"); //Latitude of wave resource data
    lon = get_number(data_table, "lon"); //Longitude of wave resource data
    name = get_string(data_table, "name"); //Name of Wave resource Source
    city = get_string(data_table, "city"); //City where wave resource data was collected
    state = get_string(data_table, "state"); //State where wave resource data was collected
    country = get_string(data_table, "country"); //Country where wave resource data was collected
    bathymetry = get_string(data_table, "bathymetry"); //Bathymetry
    sea_bed = get_string(data_table, "sea_bed"); //Sea bed of wave resource location 
    tz = get_number(data_table, "tz"); //Time zone of wave resource data
    data_source = get_string(data_table, "data_source"); //Data source for wave resource data
    notes = get_string(data_table, "notes"); //Notes
    nearby_buoy_number = get_string(data_table, "nearby_buoy_number"); //Closest nearby buoy for wave resource data collection
    average_power_flux = get_number(data_table, "average_power_flux"); //Average power flux of waves
    //Look for wave heights or wave periods
    size_t len = 0;
    size_t len_year = 0;
    size_t len_month = 0;
    size_t len_day = 0;
    size_t len_hour = 0;
    size_t len_minute = 0;
    if (wave_resource_model_choice == 1) { //Time series wave resource option, required input of 0 or 1
        if (!data_table->table.lookup("significant_wave_height") || !data_table->table.lookup("energy_period")) { //Were wave height and wave wave period specified (need both for time series option)
            m_errorMsg = util::format("Must specify significant wave height and wave energy period inputs");
            return; //Return if data not specified
        }
        ssc_number_t* significant_wave_height = get_vector(data_table, "significant_wave_height", &len);
        for (size_t i = 0; i < len; i++)
            m_sigwaveheight.push_back((double)significant_wave_height[i]); //Fill wave height field for length of input array

        ssc_number_t* energy_period = get_vector(data_table, "energy_period", &len);
        for (size_t i = 0; i < len; i++)
            m_waveperiod.push_back((double)energy_period[i]); //Fall wave period field for length of input array
        size_t wave_size = m_waveperiod.size();
        size_t height_size = m_sigwaveheight.size();
        if (wave_size != height_size) { //Check that input arrays are the same size, return with error if not
            m_errorMsg = util::format("number of wave height entries must be same as number of wave period entries"); 
            return;
        }
        m_nRecords = wave_size;

        if (data_table->table.lookup("year")) {
            ssc_number_t* year = get_vector(data_table, "year", &len_year);
            for (size_t i = 0; i < len_year; i++)
                m_year.push_back((double)year[i]); //Fill wave height field for length of input array
        }
        if (data_table->table.lookup("month")) {
            ssc_number_t* month = get_vector(data_table, "month", &len_month);
            for (size_t i = 0; i < len_month; i++)
                m_month.push_back((double)month[i]); //Fill wave height field for length of input array
        }
        if (data_table->table.lookup("day")) {
            ssc_number_t* day = get_vector(data_table, "day", &len_day);
            for (size_t i = 0; i < len_day; i++)
                m_day.push_back((double)day[i]); //Fill wave height field for length of input array
        }
        if (data_table->table.lookup("hour")) {
            ssc_number_t* hour = get_vector(data_table, "hour", &len_hour);
            for (size_t i = 0; i < len_hour; i++)
                m_hour.push_back((double)hour[i]); //Fill wave height field for length of input array
        }
        if (data_table->table.lookup("minute")) {
            ssc_number_t* minute = get_vector(data_table, "minute", &len_minute);
            for (size_t i = 0; i < len_minute; i++)
                m_minute.push_back((double)minute[i]); //Fill wave height field for length of input array
        }
    }
    else if (wave_resource_model_choice == 0) { //PDF matrix option, required input of 0 or 1
        if (!data_table->table.lookup("wave_resource_matrix")) {
            m_errorMsg = util::format("Wave resource matrix not found");
            return;
        }
        size_t nrows = 0;
        size_t ncols = 0;
        util::matrix_t<double> wave_matrix = get_matrix(data_table, "wave_resource_matrix", &nrows, &ncols);
        m_wave_resource_matrix_data.resize(nrows, ncols);
        for (size_t r = 0; r < nrows; r++) {
            for (size_t c = 0; c < ncols; c++) {
                m_wave_resource_matrix_data.at(r, c) = wave_matrix.at(r, c);
            }
        }
        
        m_nRecords = nrows;
    }
    else { //If neither 0 or 1 is specified (variable not assigned or value >1 given)
        m_errorMsg = util::format("Wave resource model choice must be either 0 (matrix PDF data) or 1 (time series data)");
    }
}

size_t wavedata::nrecords(int wave_resource_model_choice) //Returns array size of height and period arrays for time series, number of rows for matrix PDF data (must be 21)
{
    if (wave_resource_model_choice == 1) {
        m_nRecords = m_sigwaveheight.size();
        size_t m_nRecords_period = m_waveperiod.size();
        if (m_nRecords == m_nRecords_period)
            return m_sigwaveheight.size(); 
        else
            return (size_t)std::numeric_limits<ssc_number_t>::quiet_NaN(); //Return nan is array sizes do not match
    }
    else {
        m_nRecords = m_wave_resource_matrix_data.nrows();
        if (m_nRecords == 21)
            return m_wave_resource_matrix_data.nrows();
        else
            return (size_t)std::numeric_limits<ssc_number_t>::quiet_NaN(); //return NaN if rows != 21
    }
}

ssc_number_t wavedata::get_number(var_data* v, const char* var_name) //Used to read numeric data from wave_resource_data table, returns NaN if no value specified
{
    if (var_data* value = v->table.lookup(var_name))
    {
        if (value->type == SSC_NUMBER)
            return value->num;
    }

    return std::numeric_limits<ssc_number_t>::quiet_NaN();
}

std::string wavedata::get_string(var_data* v, const char* var_name) //Used to read string data from wave_resource_data table, returns "none" if no value specified
{
    if (var_data* value = v->table.lookup(var_name))
    {
        if (value->type == SSC_STRING)
            return var_name;
    }

    return "none";
}

ssc_number_t* wavedata::get_vector(var_data* v, const char* var_name, size_t* len) //Used to read vector data from wave_resource_data table, returns "none" if no value specified
{
    ssc_number_t* p = 0;
    *len = 0;
    if (var_data* value = v->table.lookup(var_name))
    {
        if (value->type == SSC_ARRAY)
        {
            *len = value->num.length();
            p = value->num.data();
        }
    }
    else
        p = 0;
    return p;
}

util::matrix_t<double> wavedata::get_matrix(var_data* v, const char* var_name, size_t* nrows, size_t* ncols)
{

    util::matrix_t<double> p = 0;
    *nrows = 0;
    *ncols = 0;
    if (var_data* value = v->table.lookup(var_name))
    {
        if (value->type == SSC_MATRIX)
        {
            *nrows = value->num.nrows();
            *ncols = value->num.ncols();
            p = value->num;
        }
    }

    return p;

}

class cm_mhk_wave : public compute_module
{
private:
public:
	cm_mhk_wave() {
		add_var_info(_cm_vtab_mhk_wave);
        add_var_info(vtab_adjustment_factors);
	}

	void exec() {
        //Wave resource definition: 0-JPD data, 1-Time series data
        int wave_resource_model_choice = as_integer("wave_resource_model_choice");
        // total loss
        double total_loss = as_double("loss_array_spacing")
            + as_double("loss_resource_overprediction")
            + as_double("loss_transmission")
            + as_double("loss_downtime")
            + as_double("loss_additional");


        util::matrix_t<double>  wave_power_matrix = as_matrix("wave_power_matrix"); //Power matrix at various wave heights and periods
        //Get the system capacity
        double system_capacity = as_double("system_capacity");
        double annual_energy = 0, device_rated_capacity = 0, device_average_power = 0, capacity_factor = 0;
        //User either sets device_rated_capacity in the UI, or allows cmod to determine from power curve:
        device_rated_capacity = as_double("device_rated_power"); //Rated power of 1 Wave energy converter device (derived from power matrix)

        //Read number of devices
        int number_devices = as_integer("number_devices"); //How many devices in the array
        
        //Read and store wave resource and power matrix as a 2D matrix of vectors:
        util::matrix_t<double>  wave_resource_matrix;
        std::unique_ptr<wave_data_provider> wave_dp; //Wave data provider class for table definition of resource data for SDK
        if (is_assigned("wave_resource_data")) { //Check for table variable assignment
            wave_dp = std::unique_ptr<wave_data_provider>(new wavedata(wave_resource_model_choice, lookup("wave_resource_data"))); //Assign varible to data provider class
            if (!(wave_dp->error()=="")) { //Check for empty table
                throw exec_error("mhk_wave", wave_dp->error()); //Throw error if empty
            }
        }
        if (is_assigned("wave_resource_matrix")) //If wave resource matrix is assigned (may not be 
            wave_resource_matrix = as_matrix("wave_resource_matrix");
        else if (!is_assigned("wave_resource_matrix") && wave_resource_model_choice == 0) { //If wave resource matrix is specified in wave_resource_data table
            wave_resource_matrix = wave_dp->wave_matrix();
            size_t table_records = wave_dp->num_records();
            if (table_records != 21)  //21 rows for jpd matrix
                throw exec_error("mhk_wave", "Wave resource data from table must be 21x22 matrix" + wave_dp->error());
            
        }
        else if (is_assigned("wave_resource_matrix") && as_integer("wave_resource_model_choice") == 1) //If wave resource matrix is assigned but time series option is chosen
            throw exec_error("mhk_wave", "Resource model is set to use JPD and no JPD is given. Change wave_resource_model_choice to 0 to use JPD data");

        if(as_integer("wave_resource_model_choice")==0) {  //Wave resource PDF matrix option
		    
		    //Check to ensure size of wave_power_matrix == wave_resource_matrix :
		    if ( (wave_resource_matrix.ncols() !=  wave_power_matrix.ncols() ) || ( wave_resource_matrix.nrows() != wave_power_matrix.nrows() ) )
			    throw exec_error("mhk_wave", "Size of Power Matrix is not equal to Wave Resource Matrix");

		    //Checker to ensure frequency distribution adds to >= 99.5%:
		    double resource_vect_checker = 0;

		    //Allocate memory to store annual_energy_distribution:
		    ssc_number_t *p_annual_energy_dist = allocate("annual_energy_distribution", wave_resource_matrix.nrows(), wave_resource_matrix.ncols());
            
		    int k = 0; //Iterator going through rows and columns of resource matrix
			
		    for (size_t i = 0; i < (size_t)wave_power_matrix.nrows(); i++) {
			    for (size_t j = 0; j < (size_t)wave_power_matrix.ncols(); j++) {

				    //Calculate and allocate annual_energy_distribution:
				    if (j == 0 || i == 0)	//Where (i = 0) is the row header, and (j =  0) is the column header.
					    p_annual_energy_dist[k] = (ssc_number_t) wave_resource_matrix.at(i, j); //Don't do anything with top left corner of matrix as this value is always 0 and not part of the grid
				    else {
                        //Find annual probability (as a %) of wave at height i and period j, multiply by 8760/100 to find fractional hours per year that wave(i,j) is achieved;
                        //Mult. by number of devices and total loss multiplier to find energy for certain wave type in grid
                        p_annual_energy_dist[k] = (ssc_number_t)(wave_resource_matrix.at(i,j) * wave_power_matrix.at(i,j) * 8760.0 / 100.0) * (1-total_loss/100) * number_devices; 
					    annual_energy += p_annual_energy_dist[k]; //Add each wave grid contribution to annual energy variable
					    device_average_power += (p_annual_energy_dist[k] / (8760 * (1 - total_loss / 100) * number_devices)); //Average each wave grid to find average device power
					    //Checker to ensure frequency distribution adds to >= 99.5%:
					    resource_vect_checker += wave_resource_matrix.at(i,j);
				    }
				    k++; //Go to next value in wave resource grid

			    }
		    }

            if (resource_vect_checker < 99.5) //Sum of wave resource matrix must be ~= 100%
                throw exec_error("mhk_wave", "Probability vector does not add up to 100%.");



        }
        bool is_annual = true;
        if (wave_resource_model_choice==1) { //Time series wave resource option
            double number_records = 2920;//Initialize number of records to 2920 (3 hour annual dataset)
            double number_hours = 8760; //Initialize number of hours to 8760 (hours in annual dataset)
            std::vector<double> wave_height_input;
            std::vector<double> wave_period_input;
            std::vector<int> year;
            std::vector<int> month;
            std::vector<int> day;
            std::vector<int> hour;
            std::vector<int> minute;
            

            if (is_assigned("significant_wave_height") && is_assigned("energy_period")) { //Check if wave height and period variables are assigned
                //number_records = as_integer("number_records");
                //number_hours = as_integer("number_hours");
                wave_height_input = as_vector_double("significant_wave_height");
                wave_period_input = as_vector_double("energy_period");
                number_records = wave_height_input.size();
                number_hours = 8760; //always a full year of data for UI calls
                if (fmod(number_records, 2920) != 0) is_annual = false;
                year = as_vector_integer("year");
                month = as_vector_integer("month");
                day = as_vector_integer("day");
                hour = as_vector_integer("hour");
                minute = as_vector_integer("minute");

            }
            else if (!is_assigned("significant_wave_height") && !is_assigned("energy_period") && is_assigned("wave_resource_data")) { //Check if height and period variables are assigned in wave resource table data
                number_records = wave_dp->num_records();
                number_hours = 8760; //always 3 hour data from wave api calls
                if (fmod(number_records, 2920) != 0) is_annual = false;
                wave_height_input = wave_dp->wave_heights();
                if (wave_height_input.empty()) {
                    throw exec_error("mhk_wave", wave_dp->error());
                }
                wave_period_input = wave_dp->wave_periods();
                if (wave_period_input.empty()) {
                    throw exec_error("mhk_wave", wave_dp->error());
                }
                
                
                if(number_records == std::numeric_limits<ssc_number_t>::quiet_NaN()) //Check that number of records for height and period match
                    throw exec_error("mhk_wave", "Table definitions of wave height and wave period are of different array sizes" + wave_dp->error());

                year = wave_dp->wave_year();
                month = wave_dp->wave_month();
                day = wave_dp->wave_day();
                hour = wave_dp->wave_hour();
                minute = wave_dp->wave_minute();
                int hour0 = 0;
                int hour1 = 3;
                int hourdiff = 3;
                std::vector<int> timecheck;
                timecheck.resize(size_t(hour.size()));
                for (size_t r = 0; r < hour.size(); r++) {
                    if (r == 0) {
                        //value_0 = split(buf);
                        hour0 = hour[r];
                    }
                    if (r == 1) {
                        //value_1 = split(buf);
                        hour1 = hour[r];
                        hourdiff = hour1 - hour0;
                    }
                    
                    timecheck[r] = hour[r];
                    if (r > 0) {
                        if (timecheck[r] - timecheck[r - 1] != hourdiff && timecheck[r] != 0) {
                            throw exec_error("mhk_wave", "Time steps are nonuniform");
                        }
                    }
                }
            }
            else if (!is_assigned("significant_wave_height") || !is_assigned("energy_period")) //Both heights and periods must be assigned
                throw exec_error("mhk_wave", "Wave height and Energy period arrays of equal length must be assigned");

            bool system_use_lifetime_output = (as_integer("system_use_lifetime_output") == 1);
            size_t nyears = 1;
            std::vector<ssc_number_t> sys_degradation;
            size_t number_records_gen = number_records;
            if (number_records == 2920) number_records_gen *= 3.0;

            
            
            
            

            if (system_use_lifetime_output)
            {
                nyears = as_integer("analysis_period");
                number_records_gen *= nyears;
                sys_degradation.reserve(nyears);
                // setup system degradation
                size_t i, count_degrad = 0;
                ssc_number_t* degrad = 0;
                degrad = as_array("generic_degradation", &count_degrad);

                if (count_degrad == 1)
                {
                    for (i = 0; i < nyears; i++)
                        sys_degradation.push_back((ssc_number_t)pow((1.0 - (double)degrad[0] / 100.0), i));
                }
                else if (count_degrad > 0)
                {
                    for (i = 0; i < nyears && i < (int)count_degrad; i++) sys_degradation.push_back((ssc_number_t)(1.0 - (double)degrad[i] / 100.0));
                }
            }
            else {
                sys_degradation.push_back(1); // single year mode - degradation handled in financial models.
            }

            adjustment_factors haf(this->get_var_table(), "adjust");
            if (!haf.setup(number_records, nyears))
                throw exec_error("mhk_wave", "Failed to set up adjustment factors: " + haf.error());

            ssc_number_t* energy_hourly_kWh = allocate("energy_hourly_kWh", number_records * nyears);
            ssc_number_t* energy_hourly_kW = allocate("energy_hourly_kW", number_records_gen); //8760 of kW values
            ssc_number_t* energy_hourly_gen = allocate("gen", number_records * nyears );
            ssc_number_t* sig_wave_height_index_mat = allocate("sig_wave_height_index_mat", number_records );
            ssc_number_t* sig_wave_height_data = allocate("sig_wave_height_data", number_records);
            ssc_number_t* energy_period_index_mat = allocate("energy_period_index_mat", number_records);
            ssc_number_t* energy_period_data = allocate("energy_period_data", number_records);
            ssc_number_t* wave_power_index_mat = allocate("wave_power_index_mat", number_records);
            ssc_number_t* p_annual_energy_dist = allocate("annual_energy_distribution", wave_power_matrix.nrows(), wave_power_matrix.ncols());
            double ts_significant_wave_height, ts_energy_period;
            double resource_vect_checker = 0;
            size_t iday = 0;
            size_t ihour = 0;
            bool is_leap = false;
            size_t days_in_year = 366;
            std::vector<int> days_in_month = { 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };
            if (number_records == 2928) {
                is_leap = true;
                days_in_month = { 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 };
                days_in_year = 367;
            }
            double hour_step = 1.0;
            if (is_annual) {
                hour_step = number_hours / number_records;
            }
            
            ssc_number_t sig_wave_height_index = 0;
            ssc_number_t energy_period_index = 0;
            
            ssc_number_t* p_annual_energy_dist_time = allocate("annual_energy_distribution_time", 24 / std::ceil(hour_step) + 1.0, days_in_year); //Annual energy distribution 24 hr (3 hour time step) x 365 days for Summary page heatmap
            for (size_t j = 0; j < (size_t)wave_power_matrix.nrows(); j++) { //Build row and column labels of height x period energy distribution matrix
                p_annual_energy_dist[size_t(j) * 22] = (ssc_number_t)wave_power_matrix.at(j, 0);
            }
            for (size_t m = 0; m < (size_t)wave_power_matrix.ncols(); m++) {
                p_annual_energy_dist[size_t(m)] = (ssc_number_t)wave_power_matrix.at(0, m);
            }
            for (size_t m = 0; m < days_in_year; m++) { //Build row and column lables for time energy distribution matrix
                for (size_t h = 0; h < 24 / std::ceil(hour_step) + 1.0; h++) {
                    size_t h_index = h * std::ceil(hour_step); 
                    p_annual_energy_dist_time[h * days_in_year] = h_index - std::ceil(hour_step);
                    p_annual_energy_dist_time[m] = (ssc_number_t)m;
                }
            }

            for (size_t y = 0; y < nyears; y++) {
                for (size_t i = 0; i < size_t(number_records); i++) {
                    if (y == 0) {
                        ts_significant_wave_height = wave_height_input[i];
                        ts_energy_period = wave_period_input[i];
                        sig_wave_height_index = 0;
                        energy_period_index = 0;
                        //Significant Wave Height
                        if (ts_significant_wave_height < 0) {
                            sig_wave_height_index = 1;
                            sig_wave_height_index_mat[i] = 1;
                        }
                        else if (ts_significant_wave_height > 9.75) {
                            sig_wave_height_index = wave_power_matrix.nrows() - 1;
                            sig_wave_height_index_mat[i] = wave_power_matrix.nrows() - 1;
                        }
                        else {
                            for (ssc_number_t j = 1; j < (ssc_number_t)wave_power_matrix.nrows(); j++) {
                                if (std::abs(ts_significant_wave_height - wave_power_matrix.at(size_t(j), 0)) <= 0.25) { //Find which height is closest to height at current timestep
                                    sig_wave_height_index = j;
                                    sig_wave_height_index_mat[i] = sig_wave_height_index; //Store height index location in time series array
                                }
                            }
                        }
                        //Energy Period
                        if (ts_energy_period < 0) {
                            energy_period_index = 1;
                            energy_period_index_mat[i] = 1;
                        }
                        else if (ts_energy_period > 20.5) {
                            energy_period_index = wave_power_matrix.ncols() - 1;
                            energy_period_index_mat[i] = wave_power_matrix.ncols() - 1;
                        }
                        else {
                            for (ssc_number_t m = 1; m < (ssc_number_t)wave_power_matrix.ncols(); m++) {
                                if (std::abs(ts_energy_period - wave_power_matrix.at(0, size_t(m))) <= 0.50) {
                                    energy_period_index = m;
                                    energy_period_index_mat[i] = energy_period_index;
                                }
                            }
                        }

                        if (sig_wave_height_index == 0 || energy_period_index == 0) {
                            throw exec_error("mhk_wave", "The wave conditions at timestep" + to_string(i) + "were not able to be located in the power matrix. Please check your resource file");
                        }


                        //n-hour energy based on wave power matrix value at height and period best matching the time series inputs * number devices * size multiplier
                        //First check that indexed power does not exceed maximum system power
                        if (wave_power_matrix.at(size_t(sig_wave_height_index), size_t(energy_period_index)) > device_rated_capacity)
                            throw exec_error("mhk_wave", "The device power calculated from the wave height and wave period exceeds the maximum power matrix value at index" + to_string(i) + ". Please check the wave conditions.");
                    }
                
                    energy_hourly_kWh[y * size_t(number_records) + i] = (ssc_number_t)(wave_power_matrix.at(size_t(sig_wave_height_index_mat[i]), size_t(energy_period_index_mat[i]))) * hour_step * (1 - total_loss / 100) * sys_degradation[y] * number_devices * haf(y * number_records + i);
                    if (y == 0)
                        p_annual_energy_dist[size_t(sig_wave_height_index_mat[i]) * 22 + size_t(energy_period_index_mat[i])] += energy_hourly_kWh[i]; //Add energy for given time step to height x period distribution matrix at specified grid point
                    energy_hourly_gen[y * size_t(number_records) + i] = (ssc_number_t)(wave_power_matrix.at(size_t(sig_wave_height_index_mat[i]), size_t(energy_period_index_mat[i]))) * (1 - total_loss / 100) * sys_degradation[y] * number_devices * haf(y * number_records + i); //Store in gen to use in heatmap output (probably don't need two variables)
                    //energy_hourly_gen[i*3+1] = energy_hourly[i]; //Store in gen to use in heatmap output (probably don't need two variables)
                    //energy_hourly_gen[i*3+2] = energy_hourly[i]; //Store in gen to use in heatmap output (probably don't need two variables)
                    if (number_records == 2920) {
                        energy_hourly_kW[y * size_t(number_records * 3) + (i * 3)] = energy_hourly_gen[y * size_t(number_records) + i];
                        energy_hourly_kW[y * size_t(number_records * 3) + (i * 3) + 1] = energy_hourly_gen[y * size_t(number_records) + i];
                        energy_hourly_kW[y * size_t(number_records * 3) + (i * 3) + 2] = energy_hourly_gen[y * size_t(number_records) + i];
                    }
                    else {
                        energy_hourly_kW[y * size_t(number_records)+i] = energy_hourly_gen[y * size_t(number_records) + i];
                    }
                    //iday = floor(double(i * 3) / 24); //Calculate day of year
                    if (y == 0) {
                        if (month[i] == 1)
                            iday = day[i];
                        else
                            iday = days_in_month[size_t(month[i]) - 2] + day[i];
                        //ihour = fmod(i * 3, 24); //Calculate hour of day
                        ihour = hour[i];
                        for (size_t d = 0; d < days_in_year; d++) {
                            for (size_t h = 0; h < 24 / std::ceil(hour_step) + 1.0; h++) {
                                if (iday == d && ihour == std::ceil(hour_step) * h) {
                                    p_annual_energy_dist_time[(h+1) * days_in_year + d] += energy_hourly_kWh[i]; //Add energy for time step to time distribution matrix at day and hour of current timestep
                                    break; //Get out of loop once day and hour match is found
                                }
                            }
                        }
                        /*
                        sig_wave_height_index_mat[i] = (ssc_number_t)(wave_power_matrix.at(size_t(sig_wave_height_index), 0)); //Store height values closest to those in time series array
                        sig_wave_height_data[i] = ts_significant_wave_height;
                        energy_period_index_mat[i] = (ssc_number_t)(wave_power_matrix.at(0, size_t(energy_period_index))); //Store wave period values closest to those in time series input array
                        energy_period_data[i] = ts_energy_period;
                        */
                        wave_power_index_mat[i] = (ssc_number_t)(wave_power_matrix.at(size_t(sig_wave_height_index), size_t(energy_period_index))); //Store wave power used in each time step based on closest height and period from time series input arrays
                        annual_energy += energy_hourly_kWh[i]; //Sum up to annual energy
                        //device_average_power += energy_hourly[i] / 8760;
                        device_average_power += energy_hourly_kWh[i] / (number_hours * (1 - total_loss / 100) * number_devices); //Average for device average power

                    }
                }
            }
            p_annual_energy_dist[0] = 0; //set top left corner of matrix to 0
            p_annual_energy_dist_time[0] = 0; //set top left corner of matrix to 0

            for (size_t i = 1; i < (size_t)wave_power_matrix.nrows(); i++) {
                for (size_t j = 1; j < (size_t)wave_power_matrix.ncols(); j++) {
                    resource_vect_checker += p_annual_energy_dist[i * 22 + j] / annual_energy * 100;
                }
            }
            
            if (resource_vect_checker < 99.5) //Sum of wave resource matrix must be ~= 100%
                throw exec_error("mhk_wave", "Probability vector does not add up to 100%.");
            
            assign("numberRecords", var_data((ssc_number_t)number_records));
            assign("numberHours", var_data((ssc_number_t)number_hours));
            
        }


        //Can probably take all this stuff out
        //Need to change to not index outside of matrix bounds
        double wave_resource_start_period = 0;
        double wave_resource_start_height = 0;
        double wave_resource_end_period = 0;
        double wave_resource_end_height = 0;
        //Find where in wave resource matrix waves start to have a nonzero percentage, where wave distribution ends in grid
        if (wave_resource_model_choice == 0) { //Check for resource matrix to find ramp on and ramp off values from
            for (size_t l = 1; l < (size_t)wave_power_matrix.nrows(); l++) {
                for (size_t m = 1; m < (size_t)wave_power_matrix.ncols(); m++) {

                    if (m == 1) { //Treat first non-label column differently as we cannot look backwards
                        if (wave_resource_matrix.at(l, m) != 0) { //If first column has nonzero data this is where the resource starts
                            if (wave_resource_start_period == 0) { //Don't overwrite the start index once it has been written
                                wave_resource_start_period = wave_resource_matrix.at(0, m);
                                wave_resource_start_height = wave_resource_matrix.at(l, 0);
                            }
                        }
                    }
                    else {
                        //Find first instance of 0-nonzero-nonzero data for start of resource matrix
                        if ((ssc_number_t)wave_resource_matrix.at(l, m) != 0 && (ssc_number_t)wave_resource_matrix.at(l, m - 1) == 0 && (ssc_number_t)wave_resource_matrix.at(l, m + 1) != 0) {
                            if (wave_resource_start_period == 0) { //Don't overwrite the start index once it has been written
                                wave_resource_start_period = wave_resource_matrix.at(0, m);
                                wave_resource_start_height = wave_resource_matrix.at(l, 0);
                            }
                        }

                    }

                    if (m == wave_resource_matrix.ncols()-1) { //Treat last column differently as we cannot look forward
                        if (wave_resource_matrix.at(l, m) != 0) { //If last column contains nonzero data this is the end of the resource 
                            //Keep overwriting to find the further row down where nonzero data occurs
                            wave_resource_end_period = wave_resource_matrix.at(0, m);
                            wave_resource_end_height = wave_resource_matrix.at(l, 0);
                            
                        }
                    }
                    else { //Find instances of nonzero-nonzero-zero data in the row to find the end of the resource in the matrix
                        if ((ssc_number_t)wave_resource_matrix.at(l, m) != 0 && (ssc_number_t)wave_resource_matrix.at(l, m - 1) != 0  && (ssc_number_t)wave_resource_matrix.at(l, m + 1) == 0)
                        {
                            wave_resource_end_period = wave_resource_matrix.at(0, m);
                            wave_resource_end_height = wave_resource_matrix.at(l, 0);
                        }
                    }
                }

            }
            assign("wave_resource_start_height", var_data((ssc_number_t)wave_resource_start_height));
            assign("wave_resource_end_height", var_data((ssc_number_t)wave_resource_end_height));
            assign("wave_resource_start_period", var_data((ssc_number_t)wave_resource_start_period));
            assign("wave_resource_end_period", var_data((ssc_number_t)wave_resource_end_period));
        }
        double wave_power_start_period = 0;
        double wave_power_start_height = 0;
        double wave_power_end_period = 0;
        double wave_power_end_height = 0;
        //Find where in power matrix WEC ramps on, ramps off
        for (size_t n = 1; n < (size_t)wave_power_matrix.nrows(); n++) {
            for (size_t p = 1; p < (size_t)wave_power_matrix.ncols(); p++) {

                if (p == 1) { //Treat first column after row labels different as you cannot look backwards from it
                    if (wave_power_matrix.at(n, p) != 0) { //If first column contains nonzero data, then the power matrix starts there
                        if (wave_power_start_period == 0) { //Don't overwrite start indices once they are written
                            wave_power_start_height = wave_power_matrix.at(n, 0);
                            wave_power_start_period = wave_power_matrix.at(0, p);
                        }
                    }
                }
                else {
                    //Look for first instance where a nonzero value is preceded by a zero and followed by a nonzero value
                    if ((ssc_number_t)wave_power_matrix.at(n, p) != 0 && (ssc_number_t)wave_power_matrix.at(n, p - 1) == 0 && (ssc_number_t)wave_power_matrix.at(n, p + 1) != 0)
                    {
                        if (wave_power_start_period == 0) //Don't overwrite start indices once they are written
                        {
                            wave_power_start_period = wave_power_matrix.at(0, p);
                            wave_power_start_height = wave_power_matrix.at(n, 0);
                        }
                    }
                }

                if (p == wave_power_matrix.ncols() - 1) { //Treat last column in matrix differently as you cannot look forward
                    if (wave_power_matrix.at(n, p) != 0) { //If last column contains non-zero data that is where the matrix ends (keep overwriting to see which row the data ends at)
                        wave_power_end_period = wave_power_matrix.at(0, p);
                        wave_power_end_height = wave_power_matrix.at(n, 0);
                    }
                }
                else { //Look instance where nonzero data is preceded (in row) by nonzero data and followed by 0 data (keep overwriting to find furthest row down where this occurs
                    if ((ssc_number_t)wave_power_matrix.at(n, p) != 0 && (ssc_number_t)wave_power_matrix.at(n, p - 1) != 0 && (ssc_number_t)wave_power_matrix.at(n, p + 1) == 0)
                    {
                        wave_power_end_period = wave_power_matrix.at(0, p);
                        wave_power_end_height = wave_power_matrix.at(n, 0);
                        //Keep overwriting to find furthest row down where nonzero data occurs
                    }
                }


            }

        }

        assign("wave_power_start_height", var_data((ssc_number_t)wave_power_start_height));
        assign("wave_power_end_height", var_data((ssc_number_t)wave_power_end_height));
        assign("wave_power_start_period", var_data((ssc_number_t)wave_power_start_period));
        assign("wave_power_end_period", var_data((ssc_number_t)wave_power_end_period));
        //End of start height and period to potentially remove

        if (is_assigned("device_costs_total")) {
            //Cost category totals for LCOE contribution calculations
            double device_cost = as_double("device_costs_total");
            double bos_cost = as_double("balance_of_system_cost_total");
            double financial_cost = as_double("financial_cost_total");
            double om_cost = as_double("total_operating_cost");
            double fcr = as_double("fixed_charge_rate");

            //Cost per kwh Annual Energy
            double total_capital_cost_kwh = fcr * (device_cost + bos_cost + financial_cost) / annual_energy;
            double total_device_cost_kwh = fcr * device_cost / annual_energy;
            double total_bos_cost_kwh = fcr * bos_cost / annual_energy;
            double total_financial_cost_kwh = fcr * financial_cost / annual_energy;
            double total_om_cost_kwh = om_cost / annual_energy;




            //LCOE cost components
            double total_capital_cost_lcoe = (fcr * (device_cost + bos_cost + financial_cost)) / (fcr * (device_cost + bos_cost + financial_cost) + om_cost) * 100;
            double total_device_cost_lcoe = (fcr * device_cost) / (fcr * (device_cost + bos_cost + financial_cost) + om_cost) * 100;
            double total_bos_cost_lcoe = (fcr * bos_cost) / (fcr * (device_cost + bos_cost + financial_cost) + om_cost) * 100;
            double total_financial_cost_lcoe = (fcr * financial_cost) / (fcr * (device_cost + bos_cost + financial_cost) + om_cost) * 100;
            double total_om_cost_lcoe = (om_cost) / (fcr * (device_cost + bos_cost + financial_cost) + om_cost) * 100;
            assign("total_capital_cost_kwh", var_data((ssc_number_t)total_capital_cost_kwh));
            assign("total_device_cost_kwh", var_data((ssc_number_t)total_device_cost_kwh));
            assign("total_bos_cost_kwh", var_data((ssc_number_t)total_bos_cost_kwh));
            assign("total_financial_cost_kwh", var_data((ssc_number_t)total_financial_cost_kwh));
            assign("total_om_cost_kwh", var_data((ssc_number_t)total_om_cost_kwh));
            assign("total_capital_cost_lcoe", var_data((ssc_number_t)total_capital_cost_lcoe));
            assign("total_device_cost_lcoe", var_data((ssc_number_t)total_device_cost_lcoe));
            assign("total_bos_cost_lcoe", var_data((ssc_number_t)total_bos_cost_lcoe));
            assign("total_financial_cost_lcoe", var_data((ssc_number_t)total_financial_cost_lcoe));
            assign("total_om_cost_lcoe", var_data((ssc_number_t)total_om_cost_lcoe));

            //Cost per kW system capacity
            double capital_cost_kw = (device_cost + bos_cost + financial_cost) / system_capacity;
            double device_cost_kw = device_cost / system_capacity;
            double bos_cost_kw = bos_cost / system_capacity;
            double financial_cost_kw = financial_cost / system_capacity;
            double om_cost_kw = om_cost / system_capacity;
            assign("total_capital_cost_per_kw", var_data(ssc_number_t(capital_cost_kw)));
            assign("total_device_cost_per_kw", var_data(ssc_number_t(device_cost_kw)));
            assign("total_bos_cost_per_kw", var_data(ssc_number_t(bos_cost_kw)));
            assign("total_financial_cost_per_kw", var_data(ssc_number_t(financial_cost_kw)));
            assign("total_operations_cost_per_kw", var_data(ssc_number_t(om_cost_kw)));
        }

		//Calculating capacity factor:
		capacity_factor = annual_energy / (device_rated_capacity * number_devices * 8760);
        //capacity_factor = annual_energy / (device_rated_capacity * number_devices * number_hours);
		
		//Assigning values to outputs:
        if (is_annual) {
            assign("annual_energy", var_data((ssc_number_t)annual_energy));
        }
		assign("average_power", var_data((ssc_number_t)device_average_power));
		assign("capacity_factor", var_data((ssc_number_t)capacity_factor * 100));
		assign("device_average_power", var_data((ssc_number_t)device_average_power));
	}
};

DEFINE_MODULE_ENTRY(mhk_wave, "MHK Wave power calculation model using power distribution.", 3);
