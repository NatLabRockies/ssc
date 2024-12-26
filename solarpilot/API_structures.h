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

#ifndef _API_STRUCTURES_
#define _API_STRUCTURES_ 1
#include <vector>
#include <numeric>
#include <limits>

#include "mod_base.h"

// This is duplicated within solarpilot_invoke... we should consolidated.
// It appears this struct was or planned to be used but never implemented
struct sp_optimize 
{
private:
	std::vector<std::vector<double> > _optimization_sim_points;
    std::vector<double> _optimization_objectives;
    std::vector< std::vector<double> > _optimization_fluxes;

public:

	void getOptimizationSimulationHistory(std::vector<std::vector<double> > &sim_points, std::vector<double> &obj_values, std::vector<std::vector< double > > &flux_values);
	void setOptimizationSimulationHistory(std::vector<std::vector<double> > &sim_points, std::vector<double> &obj_values, std::vector<std::vector< double > > &flux_values);
};

/*
    Layout stores heliostat positions, aimpoints, template number,
    cant vector, and focal length. */
struct sp_layout
{   // TODO: I could use this to pass heliostat fields for each receiver... or a receiver map?
	struct h_position
	{
        struct { double x, y, z; } location;    // [m] Heliostat location in global coordinates {x, y, z}
        struct { double x, y, z; } aimpoint;    // [m] Heliostat aimpoint location in global coordinates {x, y, z}
		int template_number;                    // [-] 0 based
        int which_rec;                          // [-] Which receiver is heliostat assign to
        struct {double i, j, k; } cant_vector;	// [optional] Canting aim vector of total magnitude equal to the cant radius
		double focal_length;	                // [optional] Heliostat focal length
	};
	
    std::vector<h_position> heliostat_positions;
};

/*
    Optical table stores whole-field optical efficiency as a function of
    solar azimuth and zenith angles. */
struct sp_optical_table 
{
	sp_optical_table();
	bool is_user_positions;		                    // True if user will specify azimuths and zeniths, False otherwise
	std::vector<double> zeniths;                    // [deg] Solar zeniths angles 
	std::vector<double> azimuths;                   // [deg] Solar azimuth angles 
	std::vector<std::vector<double> > eff_data;     // [-] Solar field optical efficiency matrix (zeniths x azimuths)
};

/*
    Flux map stores a name, x- and y- positions of flux points, and flux data */
struct sp_flux_map
{
	struct sp_flux_stack
	{
		std::string map_name;                       // Name of flux map (i.e., <Receiver Name> surface i) 
		std::vector<double> xpos;                   // [m] Flux point x-position
		std::vector<double> ypos;                   // [m] Flux point y-position
		block_t<double> flux_data;                  // [W/m2] Flux on this element (y-position, x-position, solar positions)
	};
	std::vector<sp_flux_stack> flux_surfaces;       // Vector of flux surfaces that scales by number of receivers and receivers surfaces

};

/*
    Flux table stores flux maps and heliostat field efficiencies
    for each receiver & receiver surface (if applicable)
    for a set of sun azimuth and zenith angles. */
struct sp_flux_table : sp_flux_map
{	
	sp_flux_table();

	bool is_user_spacing;	    // True if user will specify data using 'n_flux_days' and 'delta_flux_hours', False otherwise
	int n_flux_days;		    // How many days are used to calculate flux maps? (default = 8)
	double delta_flux_hrs;		// How much time (hrs) between each flux map? (default = 1)
	//-- data calculated by the algorithm:
	
	std::vector<double> azimuths;                       // [deg] Solar azimuth angles
	std::vector<double> zeniths;                        // [deg] Solar zenith angles
	std::vector<std::vector<double>> efficiency;        // [-] Heliostat field efficiency for each receiver
	//---
};

// Not being used currently... duplicating sp_layout with the addition of "user_optics"
struct sp_layout_table 
{
	struct h_position
	{
        struct { double x, y, z; } 
            location, 
            aimpoint;
		int template_number; //0 based
		bool user_optics;	//indicate whether the user will provide a cant/focus std::vector
        struct {double i, j, k; } cant_vector;	//[optional] canting aim std::vector of total magnitude equal to the cant radius
		double focal_length;	//[optional] heliostat focal length
	};

    std::vector<sp_layout_table::h_position> positions;
};


#endif
