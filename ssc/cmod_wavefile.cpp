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
#include <string>
#include <fstream>
#include "core.h"
#include "lib_util.h"
#include "lib_weatherfile.cpp"

static var_info _cm_wave_file_reader[] = {
/*   VARTYPE           DATATYPE         NAME                           LABEL                                        UNITS     META                      GROUP                 REQUIRED_IF                CONSTRAINTS        UI_HINTS*/
    { SSC_INPUT,         SSC_NUMBER,      "wave_resource_model_choice",           "Joint PDF or 3-hour wave resource data",                                 "0/1",             "",             "Weather Reader",          "?=1",                         "INTEGER",                  "" },
    { SSC_INPUT,         SSC_STRING,      "wave_resource_filename",               "File path with Wave Height x Period Distribution as 2-D PDF",                     "",       "",                      "Weather Reader",      "wave_resource_model_choice=0",                       "LOCAL_FILE",      "" },
    { SSC_INPUT,         SSC_STRING,      "wave_resource_filename_ts",               "File path with 3-hour Wave Height and Period data as Time Series array",                     "",       "",                      "Weather Reader",      "wave_resource_model_choice=1",                       "LOCAL_FILE",      "" },

	
// header data
	{ SSC_OUTPUT,        SSC_STRING,      "name",                    "Name",                                        "",       "",                      "Weather Reader",      "wave_resource_model_choice=0",                        "",               "" },
	{ SSC_OUTPUT,        SSC_STRING,      "city",                    "City",                                        "",       "",                      "Weather Reader",      "wave_resource_model_choice=0",                        "",               "" },
	{ SSC_OUTPUT,        SSC_STRING,      "state",                   "State",                                       "",       "",                      "Weather Reader",      "wave_resource_model_choice=0",                        "",               "" },
	{ SSC_OUTPUT,        SSC_STRING,      "country",                 "Country",                                     "",       "",                      "Weather Reader",      "wave_resource_model_choice=0",                        "",               "" },
	{ SSC_OUTPUT,        SSC_NUMBER,      "lat",                     "Latitude",                                    "deg",    "",                      "Weather Reader",      "",                        "",               "" },
	{ SSC_OUTPUT,        SSC_NUMBER,      "lon",                     "Longitude",                                   "deg",    "",                      "Weather Reader",      "",                        "",               "" },
	{ SSC_OUTPUT,        SSC_STRING,      "nearby_buoy_number",      "Nearby buoy number",                          "",       "",                      "Weather Reader",      "wave_resource_model_choice=0",                        "",               "" },
	{ SSC_OUTPUT,        SSC_NUMBER,      "average_power_flux",      "Average power flux",                           "kW/m",   "",                      "Weather Reader",      "wave_resource_model_choice=0",                        "",               "" },
	{ SSC_OUTPUT,        SSC_STRING,      "bathymetry",              "Bathymetry",                                  "",       "",                      "Weather Reader",      "wave_resource_model_choice=0",                        "",               "" },
	{ SSC_OUTPUT,        SSC_STRING,      "sea_bed",                 "Sea bed",                                     "",       "",                      "Weather Reader",      "wave_resource_model_choice=0",                        "",               "" },
	{ SSC_OUTPUT,        SSC_NUMBER,      "tz",                      "Time zone",                                   "",       "",                      "Weather Reader",      "",                        "",               "" },
	{ SSC_OUTPUT,        SSC_STRING,      "data_source",             "Data source",                                 "",       "",                      "Weather Reader",      "",                        "",               "" },
	{ SSC_OUTPUT,        SSC_STRING,      "notes",                   "Notes",                                       "",       "",                      "Weather Reader",      "",                        "",               "" },
    { SSC_OUTPUT,        SSC_NUMBER,      "location_id",             "Location ID",               "",       "",                      "Weather Reader",     "",                        "",               "" },
    { SSC_OUTPUT,        SSC_STRING,      "location_name",             "Location",               "",       "",                      "Weather Reader",     "",                        "",               "" },

    { SSC_OUTPUT,        SSC_NUMBER,      "distance_to_shore_file",       "Distance to shore",               "m",       "",                      "Weather Reader",     "?",                        "",               "" },
    { SSC_OUTPUT,        SSC_NUMBER,      "water_depth_file",             "Water depth",               "m",       "",                      "Weather Reader",     "?",                        "",               "" },

    //timestamps
    
    { SSC_OUTPUT,        SSC_ARRAY,       "year",                    "Year",                             "yr",     "",                      "Weather Reader",      "wave_resource_model_choice=1",                       "",               "" },
    { SSC_OUTPUT,        SSC_ARRAY,       "month",                   "Month",                            "mn",     "1-12",                  "Weather Reader",      "wave_resource_model_choice=1",                       "",                          "" },
    { SSC_OUTPUT,        SSC_ARRAY,       "day",                     "Day",                              "dy",     "1-365",                 "Weather Reader",      "wave_resource_model_choice=1",                       "",                          "" },
    { SSC_OUTPUT,        SSC_ARRAY,       "hour",                    "Hour",                             "hr",     "0-23",                  "Weather Reader",      "wave_resource_model_choice=1",                       "",                          "" },
    { SSC_OUTPUT,        SSC_ARRAY,       "minute",                  "Minute",                           "min",    "0-59",                  "Weather Reader",      "wave_resource_model_choice=1",                       "",                          "" },
    
// weather data records																					            
	{ SSC_OUTPUT,        SSC_MATRIX,      "wave_resource_matrix",              "Frequency distribution of resource",                                  "m/s",   "",                       "Weather Reader",      "*",                        "",                            "" },
   // { SSC_OUTPUT,        SSC_ARRAY,       "time_check",                        "Time check",                                                          "",      "",                       "Weather Reader",      "?",                        "",                            "" },
   // { SSC_OUTPUT,        SSC_ARRAY,       "month",                        "Month",                                                          "",      "",                       "Weather Reader",      "?",                        "",                            "" },

    { SSC_OUTPUT,        SSC_ARRAY,       "significant_wave_height",           "Wave height time series data",                                        "m",     "",                       "Weather Reader",      "?",                        "",                            "" },
    { SSC_OUTPUT,        SSC_NUMBER,       "number_records",                "Number of records in wave time series",                                        "",     "",                       "Weather Reader",      "?",                        "",                            "" },
    { SSC_OUTPUT,        SSC_NUMBER,       "number_hours",                "Number of hours in wave time series",                                        "",     "",                       "Weather Reader",      "?",                        "",                            "" },

    { SSC_OUTPUT,        SSC_ARRAY,       "energy_period",                "Wave period time series data",                                        "s",     "",                       "Weather Reader",      "?",                        "",                            "" },
var_info_invalid };


class cm_wave_file_reader : public compute_module
{
public:

	cm_wave_file_reader()
	{
		add_var_info(_cm_wave_file_reader);
	}
	
    void exec()
    {
      
        std::string file;
        if (is_assigned("wave_resource_filename") && as_integer("wave_resource_model_choice")==0)
        {
            file = as_string("wave_resource_filename");
        }
        else if (is_assigned("wave_resource_filename_ts") && as_integer("wave_resource_model_choice")==1)
        {
            file = as_string("wave_resource_filename_ts");
        }
        else
        {
            throw exec_error("wave_file_reader", "Model choice and wave file do not match");
        }
        if (file.empty())
        {
            throw exec_error("wave_file_reader", "File name missing.");
        }


        std::string buf, buf1;
        std::ifstream ifs(file);

        if (!ifs.is_open())
        {
            throw exec_error("wave_file_reader", "could not open file for reading: " + file);
        }

        std::vector<std::string> values;
        std::vector<std::string> value_0;
        std::vector<std::string> value_1;
        // header if not use_specific_wf_file
        
        getline(ifs, buf);
        getline(ifs, buf1);

        // header name value pairs
        std::vector<std::string> keys = split(buf);
        values = split(buf1);
        int ncols = (int)keys.size();
        int ncols1 = (int)values.size();
        //Do we need to require all 
        if (ncols != ncols1)
        {
            throw exec_error("wave_file_reader", "Number of header column labels does not match number of values. There are " + std::to_string(ncols) + "keys and " + std::to_string(ncols1) + "values.");
        }
        if (as_integer("wave_resource_model_choice") == 0) {
            if (values.size() < 13) throw exec_error("wave_file_reader", "The JPD file is missing header information and cannot be read.");
            assign("name", var_data(values[0]));
            assign("city", var_data(values[1]));
            assign("state", var_data(values[2]));
            assign("country", var_data(values[3]));
            // lat with S is negative
            ssc_number_t dlat = std::numeric_limits<double>::quiet_NaN();
            std::vector<std::string> slat = split(values[4], ' ');
            if (slat.size() > 0)
            {
                dlat = std::stod(slat[0]);
                if (slat.size() > 1)
                {
                    if (slat[1] == "S") dlat = 0.0 - dlat;
                }
            }
            assign("lat", var_data(dlat));
            // lon with W is negative
            ssc_number_t dlon = std::numeric_limits<double>::quiet_NaN();
            std::vector<std::string> slon = split(values[5], ' ');
            if (slon.size() > 0)
            {
                dlon = std::stod(slon[0]);
                if (slon.size() > 1)
                {
                    if (slon[1] == "W") dlon = 0.0 - dlon;
                }
            }
            assign("lon", var_data(dlon));
            assign("nearby_buoy_number", var_data(values[6]));
            assign("average_power_flux", var_data(std::stod(values[7])));
            assign("bathymetry", var_data(values[8]));
            assign("sea_bed", var_data(values[9]));
            assign("tz", var_data(std::stod(values[10])));
            assign("data_source", var_data(values[11]));
            assign("notes", var_data(values[12]));
        }
        else {
            /*
            assign("location_id", var_data(std::stod(values[1])));
            assign("location_name", var_data(values[2]));
            assign("distance_to_shore_file", var_data(std::stod(values[7])));
            assign("water_depth_file", var_data(std::stod(values[18])));
            assign("lat", var_data(std::stod(values[3])));
            assign("lon", var_data(std::stod(values[4])));
            assign("tz", var_data(std::stod(values[6])));
            assign("data_source", var_data(values[0]));
            assign("notes", var_data(values[19]));
            */

            // allow metadata rows to have different lengths as long as required data is included
            ncols = std::min(ncols, ncols1);

            std::string name, value;

            for (size_t i = 0; (int)i < ncols; i++)
            {

                name = "";
                if (!keys[i].empty())
                    name = util::lower_case(trimboth(keys[i]));

                value = "";
                if (!values[i].empty())
                    value = trimboth(values[i]);

                // required metadata (see checks below)
                if (name == "lat" || name == "latitude")
                {
                    assign("lat", var_data(std::stod(value)));
                }
                else if (name == "lon" || name == "long" || name == "longitude" || name == "lng")
                {
                    assign("lon", var_data(std::stod(value)));
                }
                else if (name == "tz" || name == "timezone" || name == "time zone") // require "time zone" and "local time zone" columns in NSRDB files are the same
                {
                    assign("tz", var_data(std::stod(value)));
                }
                else if (name == "distance to shore" || name == "shore distance" || name == "distance") // require "time zone" and "local time zone" columns in NSRDB files are the same
                {
                    assign("distance_to_shore_file", var_data(std::stod(value)));
                }
                else if (name == "water depth" || name == "depth") // require "time zone" and "local time zone" columns in NSRDB files are the same
                {
                    assign("water_depth_file", var_data(std::stod(value)));
                }
                else if (name == "id" || name == "jurisdiction" || name == "station id" || name == "wban" || name == "wban#" || name == "site")
                {
                    assign("location_name", var_data(value));
                }
                else if (name == "location" || name == "location id")
                {
                    assign("location_id", var_data(std::stod(value)));
                }

                else if (name == "source" || name == "src" || name == "data source")
                {
                    assign("data_source", var_data(value));
                }
                else if (name == "notes" || name == "source notes")
                {
                    assign("notes", var_data(value));
                }
            }

        }
        
        
        if (as_integer("wave_resource_model_choice") == 1)
        {
            size_t numberRecords = 0;
            double numberRecords_mat = 0;
            int year_index = -1, month_index = -1, day_index = -1, hour_index = -1, minute_index = -1, period_index = -1, height_index = -1;
            getline(ifs, buf); //Skip past column labels for record counting
            while (getline(ifs, buf)) {
                numberRecords++;
                numberRecords_mat++;
            }
            //if (numberRecords < 2920) throw exec_error("wave_file_reader", "Number of records in the wave file must = 2920 (8760 h / 3 h interval)");
            if (numberRecords == 0) throw exec_error("wave_file_reader", "No data found in file: " + file);
            
            if ((numberRecords % 2920 != 0)) {
                if ((numberRecords % 8760 != 0)) //check for subhourly
                    throw exec_error("wave_file_reader", "Invalid number of entries in wave resource file: " + file);
            }
            
            assign("number_records", (int)numberRecords);
            // rewind the file and reposition right after the header information
            ifs.clear();
            ifs.seekg(0);
            for (size_t i = 0; i < 3; i++)
                getline(ifs, buf);
            if (ifs.eof())
            {
                throw exec_error("wave_file_reader", "Could not read column names");
            }

            auto cols = split(buf);
            int num_cols = (int)cols.size();
            for (int i = 0; i < num_cols; i++)
            {
                const std::string col_name = trimboth(cols[i]);
                if (name.length() > 0)
                {
                    std::string lowname = util::lower_case(col_name);

                    if (lowname == "yr" || lowname == "year") year_index = i;
                    else if (lowname == "mo" || lowname == "month") month_index = i;
                    else if (lowname == "day") day_index = i;
                    else if (lowname == "hour" || lowname == "hr") hour_index = i;
                    else if (lowname == "min" || lowname == "minute") minute_index = i;
                    else if (lowname == "wave height" || lowname == "wave heights" || lowname == "heights" || lowname == "height" || lowname == "hs" || lowname == "significant wave height") height_index = i;
                    else if (lowname == "wave period" || lowname == "wave periods" || lowname == "energy period" || lowname == "energy periods" || lowname == "tp") period_index = i;
                    
                }
            }
            if (year_index == -1 || month_index == -1 || day_index == -1 || hour_index == -1 || minute_index == -1 ||
                period_index == -1 || height_index == -1)
                throw exec_error("wave_file_reader", "Data values could not be identified from column headings. Please check for year, month, day, hour, minute, wave period, and wave height columns headings");
            ssc_number_t hour0 = 0, hour1 = 0, hourdiff = 0;
            ssc_number_t ts_significant_wave_height;
            ssc_number_t ts_energy_period;
            size_t ncols = 22;
            size_t nrows = 21;
            size_t sig_wave_height_index = 0;
            size_t energy_period_index = 0;
            ssc_number_t* p_year = allocate("year", numberRecords);
            ssc_number_t* p_day = allocate("day", numberRecords);
            ssc_number_t* p_hour = allocate("hour", numberRecords);
            ssc_number_t* p_minute = allocate("minute", numberRecords);
            ssc_number_t* mat = allocate("wave_resource_matrix", nrows, ncols);
            
            for (size_t j = 0; j < 21; j++) {
                mat[j * ncols] = (0.25 + (j-1) * 0.5);
            }
            for (size_t m = 0; m < 22; m++) {
                mat[m] = m - 0.5;
            }
            ssc_number_t* p_month = allocate("month", numberRecords);
            std::vector<ssc_number_t> timecheck(numberRecords);
            timecheck[0] = 0;
            ssc_number_t* wave_heights = allocate("significant_wave_height", numberRecords);
            ssc_number_t* wave_periods = allocate("energy_period", numberRecords);
            for (size_t r = 0; r < numberRecords; r++) {
                getline(ifs, buf);
                values.clear();
                values = split(buf);
                /*
                if (r == 0) {
                    //value_0 = split(buf);
                    hour0 = (ssc_number_t)std::stod(values[hour_index]);
                }
                if (r == 1) {
                    //value_1 = split(buf);
                    hour1 = (ssc_number_t)std::stod(values[hour_index]);
                    hourdiff = hour1 - hour0;
                }
                month[r] = (ssc_number_t)std::stod(values[month_index]);
                timecheck[r] = (ssc_number_t)std::stod(values[hour_index]);
                if (r > 0) {
                    if (timecheck[r] - timecheck[r - 1] != hourdiff && timecheck[r] != 0) {
                        throw exec_error("wave_file_reader", "Time steps are nonuniform");
                    }
                }*/
                p_year[r] = (ssc_number_t)std::stod(values[year_index]);
                p_hour[r] = (ssc_number_t)std::stod(values[hour_index]);
                p_day[r] = (ssc_number_t)std::stod(values[day_index]);
                p_minute[r] = (ssc_number_t)std::stod(values[minute_index]);
                p_month[r] = (ssc_number_t)std::stod(values[month_index]);
                wave_heights[r] = (ssc_number_t)std::stod(values[height_index]);
                wave_periods[r] = (ssc_number_t)std::stod(values[period_index]);

                //Make JPD from time series data

                ts_significant_wave_height = wave_heights[r];
                ts_energy_period = wave_periods[r];
                for (size_t j = 0; j < 21; j++) {
                    if (std::abs(ts_significant_wave_height - (0.25 + (j - 1) * 0.5)) <= 0.25 && j != 0) {
                        sig_wave_height_index = j;
                        break;
                    }
                }
                for (size_t m = 0; m < 22; m++) {
                    if (std::abs(ts_energy_period - (0.5 + (m - 1))) <= 0.5 && m != 0) {
                        energy_period_index = m;
                        break;
                    }
                }
                //Add percentage point to resource matrix for matcing wave height and energy period bins
                double mat_incr = 100 / numberRecords_mat;
                mat[sig_wave_height_index * ncols + energy_period_index] = mat[sig_wave_height_index * ncols + energy_period_index] + mat_incr; //1/numberRecords * 100 to make a percentage at each time step
                                
            }
            //Set decimals to 2 places in resource matrix for easier reading in output
            
            for (size_t r2 = 0; r2 < 21; r2++) {
                for (size_t c2 = 0; c2 < 22; c2++) {
                    if (r2 != 0 && c2 != 0) mat[r2 * 22 + c2] = round(mat[r2 * 22 + c2] * 100) / 100;
                }
            }
            double test_value = mat[28];
            double test_value2 = mat[187];
            mat[0] = 0;
            assign("number_hours", int(numberRecords * hourdiff));
        }
        else if (as_integer("wave_resource_model_choice") == 0) {
            ssc_number_t* mat = allocate("wave_resource_matrix", 21, 22);
            for (size_t r = 0; r < 21; r++)
            {
                getline(ifs, buf);
                values.clear();
                values = split(buf);
                if (values.size() != 22)
                {
                    throw exec_error("wave_file_reader", "Wave period columns must span 0.5s to 20.5s with increments of 1s. Incorrect number of wave period (s) columns: " + std::to_string(values.size()));
                }
                for (size_t c = 0; c < 22; c++)
                {
                    if (r == 0 && c == 0)
                        mat[r * 22 + c] = 0.0;
                    else
                        mat[r * 22 + c] = std::stod(values[c]);
                }
            }
            
        }
        else {
            throw exec_error("wave_file_reader", "Resource data type needs to be defined ");
        }

        return;
    }
};

DEFINE_MODULE_ENTRY(wave_file_reader, "SAM Wave Resource File Reader", 1)
