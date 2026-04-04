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


#include <exception>
#include <algorithm>
#include <math.h>

#include "vartab.h"
#include "../shared/lib_util.h"
#include "../shared/lib_time.h"

#include "cmod_merchantplant_eqns.h"
//#pragma warning(disable: 4297)  // ignore warning: 'function assumed not to throw an exception but does'

bool mp_ancillary_services(ssc_data_t data)
{
	std::string error = "";
    std::string warning = "";
    bool ancillary_services_success = false;
	bool calculate_revenue = false;
	auto vt = static_cast<var_table*>(data);
    if (!vt) {
        return false;
    }
	try {
		bool gen_is_assigned = false;

		int mp_enable_energy_market_revenue, mp_enable_ancserv1, mp_enable_ancserv2, mp_enable_ancserv3, mp_enable_ancserv4,
            system_use_lifetime_output, mp_enable_market_percent_gen, mp_enable_ancserv1_percent_gen, mp_enable_ancserv2_percent_gen,
            mp_enable_ancserv3_percent_gen, mp_enable_ancserv4_percent_gen;
        double mp_market_percent_gen, mp_ancserv1_percent_gen, mp_ancserv2_percent_gen, mp_ancserv3_percent_gen, mp_ancserv4_percent_gen;
		int mp_calculate_revenue;
		ssc_number_t analysis_period, system_capacity;
        util::matrix_t<ssc_number_t> mp_energy_market_revenue, mp_ancserv1_revenue, mp_ancserv2_revenue, mp_ancserv3_revenue, mp_ancserv4_revenue, system_gen, degradation;
         /* also add single matrices in SAM_994 and header file for price only data
		{ SSC_INPUT,        SSC_NUMBER,     "mp_enable_energy_market_revenue",		      "Enable energy market revenue",   "0/1",   "",    "",  "*",	"INTEGER,MIN=0,MAX=1",      "" },
		{ SSC_INPUT, SSC_MATRIX, "mp_energy_market_revenue", "Energy market revenue input", "", "","*", "", ""},
		{ SSC_INPUT,        SSC_NUMBER,     "mp_enable_ancserv1",		      "Enable ancillary services 1 revenue",   "0/1",   "",    "",  "*",	"INTEGER,MIN=0,MAX=1",      "" },
		{ SSC_INPUT, SSC_MATRIX, "mp_ancserv1_revenue", "Ancillary services 1 revenue input", "", "","*", "", "" },
		{ SSC_INPUT,        SSC_NUMBER,     "mp_enable_ancserv2",		      "Enable ancillary services 2 revenue",   "0/1",   "",    "",  "*",	"INTEGER,MIN=0,MAX=1",      "" },
		{ SSC_INPUT, SSC_MATRIX, "mp_ancserv2_revenue", "Ancillary services 2 revenue input", "", "","*", "", "" },
		{ SSC_INPUT,        SSC_NUMBER,     "mp_enable_ancserv3",		      "Enable ancillary services 3 revenue",   "0/1",   "",    "",  "*",	"INTEGER,MIN=0,MAX=1",      "" },
		{ SSC_INPUT, SSC_MATRIX, "mp_ancserv3_revenue", "Ancillary services 3 revenue input", "", "","*", "", "" },
		{ SSC_INPUT,        SSC_NUMBER,     "mp_enable_ancserv4",		      "Enable ancillary services 4 revenue",   "0/1",   "",    "",  "*",	"INTEGER,MIN=0,MAX=1",      "" },
		{ SSC_INPUT, SSC_MATRIX, "mp_ancserv4_revenue", "Ancillary services 4 revenue input", "", "","*", "", "" },
		*/
		vt_get_int(vt, "system_use_lifetime_output", &system_use_lifetime_output);
        vt_get_number(vt, "analysis_period", &analysis_period);

        // percent generation ssc variable retrieval
        vt_get_int(vt, "mp_enable_market_percent_gen", &mp_enable_market_percent_gen);
        vt_get_int(vt, "mp_enable_ancserv1_percent_gen", &mp_enable_ancserv1_percent_gen);
        vt_get_int(vt, "mp_enable_ancserv2_percent_gen", &mp_enable_ancserv2_percent_gen);
        vt_get_int(vt, "mp_enable_ancserv3_percent_gen", &mp_enable_ancserv3_percent_gen);
        vt_get_int(vt, "mp_enable_ancserv4_percent_gen", &mp_enable_ancserv4_percent_gen);
        vt_get_number(vt, "mp_market_percent_gen", &mp_market_percent_gen);
        vt_get_number(vt, "mp_ancserv1_percent_gen", &mp_ancserv1_percent_gen);
        vt_get_number(vt, "mp_ancserv2_percent_gen", &mp_ancserv2_percent_gen);
        vt_get_number(vt, "mp_ancserv3_percent_gen", &mp_ancserv3_percent_gen);
        vt_get_number(vt, "mp_ancserv4_percent_gen", &mp_ancserv4_percent_gen);

		vt_get_int(vt, "mp_enable_energy_market_revenue", &mp_enable_energy_market_revenue);
		vt_get_int(vt, "mp_enable_ancserv1", &mp_enable_ancserv1);
		vt_get_int(vt, "mp_enable_ancserv2", &mp_enable_ancserv2);
		vt_get_int(vt, "mp_enable_ancserv3", &mp_enable_ancserv3);
		vt_get_int(vt, "mp_enable_ancserv4", &mp_enable_ancserv4);

        
        // load relevant matrices
        vt_get_matrix(vt, "mp_energy_market_revenue" + std::string((mp_enable_market_percent_gen > 0.5) ? "_single" : ""), mp_energy_market_revenue);
        vt_get_matrix(vt, "mp_ancserv1_revenue" + std::string((mp_enable_ancserv1_percent_gen > 0.5) ? "_single" : ""), mp_ancserv1_revenue);
        vt_get_matrix(vt, "mp_ancserv2_revenue" + std::string((mp_enable_ancserv2_percent_gen > 0.5) ? "_single" : ""), mp_ancserv2_revenue);
        vt_get_matrix(vt, "mp_ancserv3_revenue" + std::string((mp_enable_ancserv3_percent_gen > 0.5) ? "_single" : ""), mp_ancserv3_revenue);
        vt_get_matrix(vt, "mp_ancserv4_revenue" + std::string((mp_enable_ancserv4_percent_gen > 0.5) ? "_single" : ""), mp_ancserv4_revenue);
        


        gen_is_assigned = (vt->lookup("gen") != NULL);
		if (gen_is_assigned)
		{
			system_capacity = 0.0;
			// these are arrays so VT_GET_INPUT replaced by vt_get_matrix fails for all in commit c461b9
			vt_get_matrix(vt, "gen", system_gen);
			vt_get_matrix(vt, "degradation", degradation);
		}
		else
		{
			vt_get_number(vt, "system_capacity", &system_capacity);
		}
		calculate_revenue = (vt->lookup("mp_calculate_revenue") != NULL);
		if (calculate_revenue)
		{
			vt_get_int(vt, "mp_calculate_revenue", &mp_calculate_revenue);
			calculate_revenue = (bool)mp_calculate_revenue;
		}

		// kW to MW for comparison
		system_capacity /= 1000.0;
		// sum up all power values from all revenue inputs and find smallest timestep and compare sum to system capacity
		bool en_mp_energy_market = (mp_enable_energy_market_revenue > 0.5);
		bool en_mp_ancserv1 = (mp_enable_ancserv1 > 0.5);
		bool en_mp_ancserv2 = (mp_enable_ancserv2 > 0.5);
		bool en_mp_ancserv3 = (mp_enable_ancserv3 > 0.5);
		bool en_mp_ancserv4 = (mp_enable_ancserv4 > 0.5);
        bool en_mp_market_percent_gen = (mp_enable_market_percent_gen > 0.5);
        bool en_mp_ancserv1_percent_gen = (mp_enable_ancserv1_percent_gen > 0.5);
        bool en_mp_ancserv2_percent_gen = (mp_enable_ancserv2_percent_gen > 0.5);
        bool en_mp_ancserv3_percent_gen = (mp_enable_ancserv3_percent_gen > 0.5);
        bool en_mp_ancserv4_percent_gen = (mp_enable_ancserv4_percent_gen > 0.5);

        // convert number percentages to factors
        double mp_market_gen_factor = mp_market_percent_gen / 100.0;
        double mp_ancserv1_gen_factor = mp_ancserv1_percent_gen / 100.0;
        double mp_ancserv2_gen_factor = mp_ancserv2_percent_gen / 100.0;
        double mp_ancserv3_gen_factor = mp_ancserv3_percent_gen / 100.0;
        double mp_ancserv4_gen_factor = mp_ancserv4_percent_gen / 100.0;

		// if none enabled then check passes
		ancillary_services_success = (!mp_enable_energy_market_revenue && !mp_enable_ancserv1 && !mp_enable_ancserv2 && !mp_enable_ancserv3 && !mp_enable_ancserv4);

		size_t nsteps = 0, nsteps_per_year = 8760;
		if (en_mp_energy_market)
			nsteps = std::max(nsteps, mp_energy_market_revenue.nrows());
		if (en_mp_ancserv1)
			nsteps = std::max(nsteps, mp_ancserv1_revenue.nrows());
		if (en_mp_ancserv2)
			nsteps = std::max(nsteps, mp_ancserv2_revenue.nrows());
		if (en_mp_ancserv3)
			nsteps = std::max(nsteps, mp_ancserv3_revenue.nrows());
		if (en_mp_ancserv4)
			nsteps = std::max(nsteps, mp_ancserv4_revenue.nrows());

		if (nsteps < (8760 * (size_t)analysis_period)) nsteps = 8760 * (size_t)analysis_period; // extrapolated timeseries has minimum of hourly values for use in all forecasting

		std::vector<ssc_number_t> energy_market_revenue(nsteps, 0.0);
		std::vector<ssc_number_t> ancillary_services1_revenue(nsteps, 0.0);
		std::vector<ssc_number_t> ancillary_services2_revenue(nsteps, 0.0);
		std::vector<ssc_number_t> ancillary_services3_revenue(nsteps, 0.0);
		std::vector<ssc_number_t> ancillary_services4_revenue(nsteps, 0.0);
        std::vector<ssc_number_t> energy_market_purchases(nsteps, 0.0);
        std::vector<ssc_number_t> ancillary_services1_purchases(nsteps, 0.0);
        std::vector<ssc_number_t> ancillary_services2_purchases(nsteps, 0.0);
        std::vector<ssc_number_t> ancillary_services3_purchases(nsteps, 0.0);
        std::vector<ssc_number_t> ancillary_services4_purchases(nsteps, 0.0);

        if (ancillary_services_success) {
            warning = "No source of revenue enabled. The Merchant Plant financial model requires at least one source of revenue. To fix the problem, enable energy market revenue and/or one or more ancillary services.";
        }
		// cleared capacity and price columns
		if (analysis_period > 0)
		{
			if (en_mp_energy_market)
				nsteps = std::max(nsteps, mp_energy_market_revenue.nrows());
			if (en_mp_ancserv1)
				nsteps = std::max(nsteps, mp_ancserv1_revenue.nrows());
			if (en_mp_ancserv2)
				nsteps = std::max(nsteps, mp_ancserv2_revenue.nrows());
			if (en_mp_ancserv3)
				nsteps = std::max(nsteps, mp_ancserv3_revenue.nrows());
			if (en_mp_ancserv4)
				nsteps = std::max(nsteps, mp_ancserv4_revenue.nrows());

			if (nsteps > 0)
			{
				if (nsteps < (8760 * (size_t)analysis_period)) nsteps = 8760 * (size_t)analysis_period; // extrapolated timeseries has minimum of hourly values for use in all forecasting
				std::vector<ssc_number_t> cleared_capacity_sum(nsteps, 0.0);
				std::vector<ssc_number_t> system_generation(nsteps, 0.0);
				std::vector<ssc_number_t> energy_market_capacity(nsteps, 0.0);
				std::vector<ssc_number_t> ancillary_services1_capacity(nsteps, 0.0);
				std::vector<ssc_number_t> ancillary_services2_capacity(nsteps, 0.0);
				std::vector<ssc_number_t> ancillary_services3_capacity(nsteps, 0.0);
				std::vector<ssc_number_t> ancillary_services4_capacity(nsteps, 0.0);
				std::vector<ssc_number_t> current_year_capacity;
				std::vector<ssc_number_t> extrapolated_current_year_capacity;
				std::vector<ssc_number_t> current_year_revenue;
				std::vector<ssc_number_t> extrapolated_current_year_revenue;
				nsteps_per_year = nsteps / (size_t)analysis_period;
				if (nsteps_per_year < 8760) nsteps_per_year = 8760; // for use of extrapolated_timeseries

				size_t steps_per_hour = nsteps_per_year / 8760;
				size_t current_num_per_year;
				for (size_t iyear = 0; iyear < (size_t)analysis_period; iyear++)
				{
					if (gen_is_assigned)
					{
						current_year_capacity.clear();

						// note that arrays inputs retrieved as matrices are single rows.
						if (system_use_lifetime_output > 0.5) // lifetime "gen" = "system_gen"
							current_num_per_year = system_gen.ncols() / (size_t)analysis_period;
						else // adjust single year for lifetime system_generation
							current_num_per_year = system_gen.ncols();
						current_year_capacity.reserve(current_num_per_year);
						for (size_t ic = 0; (ic < current_num_per_year); ic++)
						{
							if (system_use_lifetime_output > 0.5) // lifetime "gen" = "system_gen"
							{
								if ((ic + iyear * current_num_per_year) < system_gen.ncols())
									current_year_capacity.push_back(system_gen(0, ic + iyear * current_num_per_year) / 1000.0); // kW to MW
							}
							else // single year - adjust with degradation
							{
								// degradation
								// degradation starts in year 2 for single value degradation - no degradation in year 1 - degradation =1.0
								// lifetime degradation applied in technology compute modules
								if (system_use_lifetime_output < 1) // adjust single year for lifetime system_generation
								{
									ssc_number_t degrade_factor;
									if (degradation.nrows() == 1)
										degrade_factor = pow((1.0 - degradation(0, 0) / 100.0), iyear);
									else
										degrade_factor = (1.0 - degradation(0, ic) / 100.0);
									current_year_capacity.push_back((system_gen(0, ic) * degrade_factor) / 1000.0); // kW to MW
								}

							}
						}
						extrapolated_current_year_capacity = extrapolate_timeseries(current_year_capacity, steps_per_hour);
						for (size_t ic = 0; (ic < extrapolated_current_year_capacity.size()) && ((ic + iyear * current_num_per_year) < cleared_capacity_sum.size()); ic++)
							system_generation[ic + iyear * nsteps_per_year] = extrapolated_current_year_capacity[ic];
					}
					else
					{
						for (size_t ic = 0; (ic < nsteps_per_year); ic++)
							system_generation[ic + iyear * nsteps_per_year] = system_capacity;
					}
					if (en_mp_energy_market)
					{
                        current_num_per_year = mp_energy_market_revenue.nrows() / (size_t)analysis_period;
                        if (mp_energy_market_revenue.nrows() < 2) current_num_per_year = 1;

                        if (mp_energy_market_revenue.ncols() > 1) {
                            current_year_capacity.clear();
                            if (mp_energy_market_revenue.nrows() < 2) {// single value - addresses SAM issue 1004
                                if (mp_energy_market_revenue.nrows() == 1)
                                    current_year_capacity.push_back(mp_energy_market_revenue(0, 0));
                                else
                                    current_year_capacity.push_back(0.0);
                            }
                            else {
                                current_year_capacity.reserve(current_num_per_year);
                                for (size_t ic = 0; (ic < current_num_per_year) && ((ic + iyear * current_num_per_year) < mp_energy_market_revenue.nrows()); ic++)
                                    current_year_capacity.push_back(mp_energy_market_revenue(ic + iyear * current_num_per_year, 0));
                            }
                            extrapolated_current_year_capacity = extrapolate_timeseries(current_year_capacity, steps_per_hour);
                        }
                        if (en_mp_market_percent_gen)
                        {
                            // copy capacity as % of system generation as specified from user input 
                            for (size_t ic = 0; (ic + iyear * nsteps_per_year) < cleared_capacity_sum.size(); ic++)
                            {
                                energy_market_capacity[ic + iyear * nsteps_per_year] = mp_market_gen_factor * system_generation[ic + iyear * nsteps_per_year];
                                cleared_capacity_sum[ic + iyear * nsteps_per_year] += mp_market_gen_factor * system_generation[ic + iyear * nsteps_per_year];
                            }
                        }
                        else
                        {
                            for (size_t ic = 0; (ic < extrapolated_current_year_capacity.size()) && ((ic + iyear * nsteps_per_year) < cleared_capacity_sum.size()); ic++)
                            {
                                energy_market_capacity[ic + iyear * nsteps_per_year] = extrapolated_current_year_capacity[ic];
                                cleared_capacity_sum[ic + iyear * nsteps_per_year] += extrapolated_current_year_capacity[ic];
                            }
                        }

                        int price_col = en_mp_market_percent_gen ? 0 : 1;

						if (calculate_revenue)
						{
							current_year_revenue.clear();
                            if (mp_energy_market_revenue.nrows() < 2) {// single value - addresses SAM issue 1004
                                if (mp_energy_market_revenue.nrows() == 1)
                                    current_year_revenue.push_back(mp_energy_market_revenue(0, price_col));
                                else
                                    current_year_revenue.push_back(0.0);
                            }
                            else {
                                current_year_revenue.reserve(current_num_per_year);
                                for (size_t ic = 0; (ic < current_num_per_year) && ((ic + iyear * current_num_per_year) < mp_energy_market_revenue.nrows()); ic++)
                                    current_year_revenue.push_back(mp_energy_market_revenue(ic + iyear * current_num_per_year, price_col));
                            }
                            extrapolated_current_year_revenue = extrapolate_timeseries(current_year_revenue, steps_per_hour);
                            for (size_t ic = 0; (ic < extrapolated_current_year_revenue.size()) && ((ic + iyear * nsteps_per_year) < energy_market_revenue.size()); ic++)
                                energy_market_revenue[ic + iyear * nsteps_per_year] = extrapolated_current_year_revenue[ic]; // $/MWh
                        }

					}
                    if (en_mp_ancserv1)
                    {
                        current_num_per_year = mp_ancserv1_revenue.nrows() / (size_t)analysis_period;
                        if (mp_ancserv1_revenue.nrows() < 2) current_num_per_year = 1;

                        if (mp_ancserv1_revenue.ncols() > 1) {
                            current_year_capacity.clear();
                            if (mp_ancserv1_revenue.nrows() < 2) {// single value - addresses SAM issue 1004
                                if (mp_ancserv1_revenue.nrows() == 1)
                                    current_year_capacity.push_back(mp_ancserv1_revenue(0, 0));
                                else
                                    current_year_capacity.push_back(0.0);
                            }
                            else {
                                current_year_capacity.reserve(current_num_per_year);
                                for (size_t ic = 0; (ic < current_num_per_year) && ((ic + iyear * current_num_per_year) < mp_ancserv1_revenue.nrows()); ic++)
                                    current_year_capacity.push_back(mp_ancserv1_revenue(ic + iyear * current_num_per_year, 0));
                            }
                            extrapolated_current_year_capacity = extrapolate_timeseries(current_year_capacity, steps_per_hour);
                        }
                        if (en_mp_ancserv1_percent_gen)
                        {
                            // copy capacity as % of system generation as specified from user input
                            for (size_t ic = 0; (ic + iyear * nsteps_per_year) < cleared_capacity_sum.size(); ic++)
                            {
                                ancillary_services1_capacity[ic + iyear * nsteps_per_year] = mp_ancserv1_gen_factor * system_generation[ic + iyear * nsteps_per_year];
                                cleared_capacity_sum[ic + iyear * nsteps_per_year] += mp_ancserv1_gen_factor * system_generation[ic + iyear * nsteps_per_year];
                            }
                        }
                        else
                        {
                            for (size_t ic = 0; (ic < extrapolated_current_year_capacity.size()) && ((ic + iyear * nsteps_per_year) < cleared_capacity_sum.size()); ic++)
                            {
                                ancillary_services1_capacity[ic + iyear * nsteps_per_year] = extrapolated_current_year_capacity[ic];
                                cleared_capacity_sum[ic + iyear * nsteps_per_year] += extrapolated_current_year_capacity[ic];
                            }
                        }

                        int price_col = en_mp_ancserv1_percent_gen ? 0 : 1;

						if (calculate_revenue)
						{
							current_year_revenue.clear();
                            if (mp_ancserv1_revenue.nrows() < 2) {// single value - addresses SAM issue 1004
                                if (mp_ancserv1_revenue.nrows() == 1)
                                    current_year_revenue.push_back(mp_ancserv1_revenue(0, price_col));
                                else
                                    current_year_revenue.push_back(0.0);
                            }
                            else {
                                current_year_revenue.reserve(current_num_per_year);
                                for (size_t ic = 0; (ic < current_num_per_year) && ((ic + iyear * current_num_per_year) < mp_ancserv1_revenue.nrows()); ic++)
                                    current_year_revenue.push_back(mp_ancserv1_revenue(ic + iyear * current_num_per_year, price_col));
                            }
							extrapolated_current_year_revenue = extrapolate_timeseries(current_year_revenue, steps_per_hour);
							for (size_t ic = 0; (ic < extrapolated_current_year_revenue.size()) && ((ic + iyear * nsteps_per_year) < ancillary_services1_revenue.size()); ic++)
								ancillary_services1_revenue[ic + iyear * nsteps_per_year] = extrapolated_current_year_revenue[ic]; // $/MWh
						}

					}
					if (en_mp_ancserv2)
					{
                        current_num_per_year = mp_ancserv2_revenue.nrows() / (size_t)analysis_period;
                        if (mp_ancserv2_revenue.nrows() < 2) current_num_per_year = 1;

                        if (mp_ancserv2_revenue.ncols() > 1) {
                            current_year_capacity.clear();
                            if (mp_ancserv2_revenue.nrows() < 2) {// single value - addresses SAM issue 1004
                                if (mp_ancserv2_revenue.nrows() == 1)
                                    current_year_capacity.push_back(mp_ancserv2_revenue(0, 0));
                                else
                                    current_year_capacity.push_back(0.0);
                            }
                            else {
                                current_year_capacity.reserve(current_num_per_year);
                                for (size_t ic = 0; (ic < current_num_per_year) && ((ic + iyear * current_num_per_year) < mp_ancserv2_revenue.nrows()); ic++)
                                    current_year_capacity.push_back(mp_ancserv2_revenue(ic + iyear * current_num_per_year, 0));
                            }
                            extrapolated_current_year_capacity = extrapolate_timeseries(current_year_capacity, steps_per_hour);
                        }
                        if (en_mp_ancserv2_percent_gen)
                        {
                            // copy capacity as % of system generation as specified from user input
                            for (size_t ic = 0;(ic + iyear * nsteps_per_year) < cleared_capacity_sum.size(); ic++)
                            {
                                ancillary_services2_capacity[ic + iyear * nsteps_per_year] = mp_ancserv2_gen_factor * system_generation[ic + iyear * nsteps_per_year];
                                cleared_capacity_sum[ic + iyear * nsteps_per_year] += mp_ancserv2_gen_factor * system_generation[ic + iyear * nsteps_per_year];
                            }
                        }
                        else
                        {
                            for (size_t ic = 0; (ic < extrapolated_current_year_capacity.size()) && ((ic + iyear * nsteps_per_year) < cleared_capacity_sum.size()); ic++)
                            {
                                ancillary_services2_capacity[ic + iyear * nsteps_per_year] = extrapolated_current_year_capacity[ic];
                                cleared_capacity_sum[ic + iyear * nsteps_per_year] += extrapolated_current_year_capacity[ic];
                            }
                        }

                        int price_col = en_mp_ancserv2_percent_gen ? 0 : 1;

						if (calculate_revenue)
						{
							current_year_revenue.clear();
                            if (mp_ancserv2_revenue.nrows() < 2) {// single value - addresses SAM issue 1004
                                if (mp_ancserv2_revenue.nrows() == 1)
                                    current_year_revenue.push_back(mp_ancserv2_revenue(0, price_col));
                                else
                                    current_year_revenue.push_back(0.0);
                            }
                            else {
                                current_year_revenue.reserve(current_num_per_year);
                                for (size_t ic = 0; (ic < current_num_per_year) && ((ic + iyear * current_num_per_year) < mp_ancserv2_revenue.nrows()); ic++)
                                    current_year_revenue.push_back(mp_ancserv2_revenue(ic + iyear * current_num_per_year, price_col));
                            }
                            extrapolated_current_year_revenue = extrapolate_timeseries(current_year_revenue, steps_per_hour);
							for (size_t ic = 0; (ic < extrapolated_current_year_revenue.size()) && ((ic + iyear * nsteps_per_year) < ancillary_services2_revenue.size()); ic++)
								ancillary_services2_revenue[ic + iyear * nsteps_per_year] = extrapolated_current_year_revenue[ic]; // $/MWh
						}

					}
					if (en_mp_ancserv3)
					{
                        current_num_per_year = mp_ancserv3_revenue.nrows() / (size_t)analysis_period;
                        if (mp_ancserv3_revenue.nrows() < 2) current_num_per_year = 1;

                        if (mp_ancserv3_revenue.ncols() > 1) {
                            current_year_capacity.clear();
                            if (mp_ancserv3_revenue.nrows() < 2) {// single value - addresses SAM issue 1004
                                if (mp_ancserv3_revenue.nrows() == 1)
                                    current_year_capacity.push_back(mp_ancserv3_revenue(0, 0));
                                else
                                    current_year_capacity.push_back(0.0);
                            }
                            else {
                                current_year_capacity.reserve(current_num_per_year);
                                for (size_t ic = 0; (ic < current_num_per_year) && ((ic + iyear * current_num_per_year) < mp_ancserv3_revenue.nrows()); ic++)
                                    current_year_capacity.push_back(mp_ancserv3_revenue(ic + iyear * current_num_per_year, 0));
                            }
                            extrapolated_current_year_capacity = extrapolate_timeseries(current_year_capacity, steps_per_hour);
                        }
                        if (en_mp_ancserv3_percent_gen)
                        {
                            // copy capacity as % of system generation as specified from user input
                            for (size_t ic = 0;(ic + iyear * nsteps_per_year) < cleared_capacity_sum.size(); ic++)
                            {
                                ancillary_services3_capacity[ic + iyear * nsteps_per_year] = mp_ancserv3_gen_factor * system_generation[ic + iyear * nsteps_per_year];
                                cleared_capacity_sum[ic + iyear * nsteps_per_year] += mp_ancserv3_gen_factor * system_generation[ic + iyear * nsteps_per_year];
                            }
                        }
                        else
                        {
                            for (size_t ic = 0; (ic < extrapolated_current_year_capacity.size()) && ((ic + iyear * nsteps_per_year) < cleared_capacity_sum.size()); ic++)
                            {
                                ancillary_services3_capacity[ic + iyear * nsteps_per_year] = extrapolated_current_year_capacity[ic];
                                cleared_capacity_sum[ic + iyear * nsteps_per_year] += extrapolated_current_year_capacity[ic];
                            }
                        }

                        int price_col = en_mp_ancserv3_percent_gen ? 0 : 1;

						if (calculate_revenue)
						{
							current_year_revenue.clear();
                            if (mp_ancserv3_revenue.nrows() < 2) {// single value - addresses SAM issue 1004
                                if (mp_ancserv3_revenue.nrows() == 1)
                                    current_year_revenue.push_back(mp_ancserv3_revenue(0, price_col));
                                else
                                    current_year_revenue.push_back(0.0);
                            }
                            else {
                                current_year_revenue.reserve(current_num_per_year);
                                for (size_t ic = 0; (ic < current_num_per_year) && ((ic + iyear * current_num_per_year) < mp_ancserv3_revenue.nrows()); ic++)
                                    current_year_revenue.push_back(mp_ancserv3_revenue(ic + iyear * current_num_per_year, price_col));
                            }
                            extrapolated_current_year_revenue = extrapolate_timeseries(current_year_revenue, steps_per_hour);
							for (size_t ic = 0; (ic < extrapolated_current_year_revenue.size()) && ((ic + iyear * nsteps_per_year) < ancillary_services3_revenue.size()); ic++)
								ancillary_services3_revenue[ic + iyear * nsteps_per_year] = extrapolated_current_year_revenue[ic]; // $/MWh
						}

					}
					if (en_mp_ancserv4)
					{
                        current_num_per_year = mp_ancserv4_revenue.nrows() / (size_t)analysis_period;
                        if (mp_ancserv4_revenue.nrows() < 2) current_num_per_year = 1;

                        if (mp_ancserv4_revenue.ncols() > 1) {
                            current_year_capacity.clear();
                            if (mp_ancserv4_revenue.nrows() < 2) {// single value - addresses SAM issue 1004
                                if (mp_ancserv4_revenue.nrows() == 1)
                                    current_year_capacity.push_back(mp_ancserv4_revenue(0, 0));
                                else
                                    current_year_capacity.push_back(0.0);
                            }
                            else {
                                current_year_capacity.reserve(current_num_per_year);
                                for (size_t ic = 0; (ic < current_num_per_year) && ((ic + iyear * current_num_per_year) < mp_ancserv4_revenue.nrows()); ic++)
                                    current_year_capacity.push_back(mp_ancserv4_revenue(ic + iyear * current_num_per_year, 0));
                            }
                            extrapolated_current_year_capacity = extrapolate_timeseries(current_year_capacity, steps_per_hour);
                        }
                        if (en_mp_ancserv4_percent_gen)
                        {
                            // copy capacity as % of system generation as specified from user input
                            for (size_t ic = 0; (ic + iyear * nsteps_per_year) < cleared_capacity_sum.size(); ic++)
                            {
                                ancillary_services4_capacity[ic + iyear * nsteps_per_year] = mp_ancserv4_gen_factor * system_generation[ic + iyear * nsteps_per_year];
                                cleared_capacity_sum[ic + iyear * nsteps_per_year] += mp_ancserv4_gen_factor * system_generation[ic + iyear * nsteps_per_year];
                            }
                        }
                        else
                        {
                            for (size_t ic = 0; (ic < extrapolated_current_year_capacity.size()) && ((ic + iyear * nsteps_per_year) < cleared_capacity_sum.size()); ic++)
                            {
                                ancillary_services4_capacity[ic + iyear * nsteps_per_year] = extrapolated_current_year_capacity[ic];
                                cleared_capacity_sum[ic + iyear * nsteps_per_year] += extrapolated_current_year_capacity[ic];
                            }
                        }

                        int price_col = en_mp_ancserv4_percent_gen ? 0 : 1;

						if (calculate_revenue)
						{
							current_year_revenue.clear();
                            if (mp_ancserv4_revenue.nrows() < 2) {// single value - addresses SAM issue 1004
                                if (mp_ancserv4_revenue.nrows() == 1)
                                    current_year_revenue.push_back(mp_ancserv4_revenue(0, price_col));
                                else
                                    current_year_revenue.push_back(0.0);
                            }
                            else {
                                current_year_revenue.reserve(current_num_per_year);
                                for (size_t ic = 0; (ic < current_num_per_year) && ((ic + iyear * current_num_per_year) < mp_ancserv4_revenue.nrows()); ic++)
                                    current_year_revenue.push_back(mp_ancserv4_revenue(ic + iyear * current_num_per_year, price_col));
                            }
                            extrapolated_current_year_revenue = extrapolate_timeseries(current_year_revenue, steps_per_hour);
							for (size_t ic = 0; (ic < extrapolated_current_year_revenue.size()) && ((ic + iyear * nsteps_per_year) < ancillary_services4_revenue.size()); ic++)
								ancillary_services4_revenue[ic + iyear * nsteps_per_year] = extrapolated_current_year_revenue[ic]; // $/MWh
						}

					}
				}
				// check each timestep against system capacity
				// TODO: check that each enabled market cleared capacity is greater than or equal to zero.
				if (cleared_capacity_sum.size() != system_generation.size())
					error = util::format("cleared capacity size %d and capacity check size %d do not match", int(cleared_capacity_sum.size()), int(system_generation.size()));
				else
				{
					for (size_t i = 0; (i < cleared_capacity_sum.size()) && (i < system_generation.size()); i++)
					{
						if ((cleared_capacity_sum[i] > 0) && ((cleared_capacity_sum[i] - system_generation[i]) > 1e-5 * std::abs(system_generation[i]) ))
						{
							error = util::format("sum of cleared capacity %g MW does not match system capacity %g MW at timestep %d", cleared_capacity_sum[i], system_generation[i], int(i));
							break;
						}

                        if ((cleared_capacity_sum[i] < 0) && ((cleared_capacity_sum[i] - system_generation[i]) > 1e-5 * std::abs(system_generation[i])))
                        {
                            warning = util::format("sum of cleared capacity %g MW does not match system capacity %g MW at timestep %d", cleared_capacity_sum[i], system_generation[i], int(i));
                            // Don't break in case there's an error at a later step. Note that the current behavior reports the last step at which there was a cleared capacity issue
                        }
					}
				}

				if (calculate_revenue)
				{ // all user specified capacities are greater than zero and sum of all less than system generation at timestep i
					vt->assign("mp_energy_market_cleared_capacity", var_data(energy_market_capacity.data(), energy_market_capacity.size()));
					vt->assign("mp_ancillary_services1_cleared_capacity", var_data(ancillary_services1_capacity.data(), ancillary_services1_capacity.size()));
					vt->assign("mp_ancillary_services2_cleared_capacity", var_data(ancillary_services2_capacity.data(), ancillary_services2_capacity.size()));
					vt->assign("mp_ancillary_services3_cleared_capacity", var_data(ancillary_services3_capacity.data(), ancillary_services3_capacity.size()));
					vt->assign("mp_ancillary_services4_cleared_capacity", var_data(ancillary_services4_capacity.data(), ancillary_services4_capacity.size()));
					// price (modified below from price to revenue)
					vt->assign("mp_energy_market_price", var_data(energy_market_revenue.data(), energy_market_revenue.size()));
					vt->assign("mp_ancillary_services1_price", var_data(ancillary_services1_revenue.data(), ancillary_services1_revenue.size()));
					vt->assign("mp_ancillary_services2_price", var_data(ancillary_services2_revenue.data(), ancillary_services2_revenue.size()));
					vt->assign("mp_ancillary_services3_price", var_data(ancillary_services3_revenue.data(), ancillary_services3_revenue.size()));
					vt->assign("mp_ancillary_services4_price", var_data(ancillary_services4_revenue.data(), ancillary_services4_revenue.size()));
					// total cleared capacity
					vt->assign("mp_total_cleared_capacity", var_data(cleared_capacity_sum.data(), cleared_capacity_sum.size()));

                    ssc_number_t revenue = 0.0;
                    for (size_t i = 0; (i < system_generation.size()) && (i < energy_market_capacity.size()) && (i < energy_market_revenue.size()); i++)
					{
						
                        // Only count positive revenue here. Negative numbers are an operating expense for LCOE
                        if (en_mp_energy_market) {
                            revenue = energy_market_revenue[i] * energy_market_capacity[i] / steps_per_hour;// [MW] * [$/MWh] / fraction per hour [1/h]
                            energy_market_revenue[i] = revenue > 0 ? revenue : 0.0;
                            energy_market_purchases[i] = revenue < 0 ? revenue : 0.0;
                        }
                        else {
                            energy_market_revenue[i] = 0.0;
                            energy_market_purchases[i] = 0.0;
                        }

                        if (en_mp_ancserv1) {
                            revenue = ancillary_services1_revenue[i] * ancillary_services1_capacity[i] / steps_per_hour; // [MW] * [$/MWh] / fraction per hour [1/h]
                            ancillary_services1_revenue[i] = revenue > 0 ? revenue : 0.0;
                            ancillary_services1_purchases[i] = revenue < 0 ? revenue : 0.0;
                        }
                        else {
                            ancillary_services1_revenue[i] = 0.0;
                            ancillary_services1_purchases[i] = 0.0;
                        }

                        if (en_mp_ancserv2) {
                            revenue = ancillary_services2_revenue[i] * ancillary_services2_capacity[i] / steps_per_hour; // [MW] * [$/MWh] / fraction per hour [1/h]
                            ancillary_services2_revenue[i] = revenue > 0 ? revenue : 0.0;
                            ancillary_services2_purchases[i] = revenue < 0 ? revenue : 0.0;
                        }
                        else {
                            ancillary_services2_revenue[i] = 0.0;
                            ancillary_services2_purchases[i] = 0.0;
                        }

                        if (en_mp_ancserv3) {
                            revenue = ancillary_services3_revenue[i] * ancillary_services3_capacity[i] / steps_per_hour; // [MW] * [$/MWh] / fraction per hour [1/h]
                            ancillary_services3_revenue[i] = revenue > 0 ? revenue : 0.0;
                            ancillary_services3_purchases[i] = revenue < 0 ? revenue : 0.0;
                        }
                        else {
                            ancillary_services3_revenue[i] = 0.0;
                            ancillary_services3_purchases[i] = 0.0;
                        }

                        if (en_mp_ancserv4) {
                            revenue = ancillary_services4_revenue[i] * ancillary_services4_capacity[i] / steps_per_hour; // [MW] * [$/MWh] / fraction per hour [1/h]
                            ancillary_services4_revenue[i] = revenue > 0 ? revenue : 0.0;
                            ancillary_services4_purchases[i] = revenue < 0 ? revenue : 0.0;
                        }
                        else {
                            ancillary_services4_revenue[i] = 0.0;
                            ancillary_services4_purchases[i] = 0.0;
                        }
						
					}
				}

			}
			else
				error = util::format("Invalid number of timesteps requested %d", int(analysis_period));
		}
		else
			error = util::format("Invalid analysis period %d", int(analysis_period));
	    // expected outputs regardless of which markets enabled - does not work when passing in and casting m_vartab
	    vt->assign("mp_energy_market_generated_revenue", var_data(energy_market_revenue.data(), energy_market_revenue.size()));
	    vt->assign("mp_ancillary_services1_generated_revenue", var_data(ancillary_services1_revenue.data(), ancillary_services1_revenue.size()));
	    vt->assign("mp_ancillary_services2_generated_revenue", var_data(ancillary_services2_revenue.data(), ancillary_services2_revenue.size()));
	    vt->assign("mp_ancillary_services3_generated_revenue", var_data(ancillary_services3_revenue.data(), ancillary_services3_revenue.size()));
	    vt->assign("mp_ancillary_services4_generated_revenue", var_data(ancillary_services4_revenue.data(), ancillary_services4_revenue.size()));

        vt->assign("mp_energy_market_consumed_cost", var_data(energy_market_purchases.data(), energy_market_purchases.size()));
        vt->assign("mp_ancillary_services1_consumed_cost", var_data(ancillary_services1_purchases.data(), ancillary_services1_purchases.size()));
        vt->assign("mp_ancillary_services2_consumed_cost", var_data(ancillary_services2_purchases.data(), ancillary_services2_purchases.size()));
        vt->assign("mp_ancillary_services3_consumed_cost", var_data(ancillary_services3_purchases.data(), ancillary_services3_purchases.size()));
        vt->assign("mp_ancillary_services4_consumed_cost", var_data(ancillary_services4_purchases.data(), ancillary_services4_purchases.size()));
    }
    catch (std::exception& e)
    {
	    error = std::string(e.what());
	    return false;
    }
    ancillary_services_success = (error == "");
    vt->assign("mp_ancillary_services", var_data(ancillary_services_success));
    vt->assign("mp_ancillary_services_error", var_data(error));
    vt->assign("mp_ancillary_services_warning", var_data(warning));
    return true;
}



