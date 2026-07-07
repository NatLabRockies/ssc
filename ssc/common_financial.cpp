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


#include "common_financial.h"
#include "core.h"
#include "common.h"
#include <sstream>
#include <sstream>
#include "../shared/lib_time.h"
#include "../shared/lib_financial.h"

#ifndef WIN32
#include <float.h>
#endif


// P
double Const_per_principal(double const_per_percent /*%*/, double total_installed_cost /*$*/) {		// [$]
	return (const_per_percent / 100.) * total_installed_cost;
}

// I
double Const_per_interest(double const_per_principal /*$*/, double const_per_interest_rate /*$*/,
	double const_per_months /*months*/) {		// [$]
	return const_per_principal * (const_per_interest_rate / 100.) / 12. * const_per_months / 2;
}

// F
double Const_per_total(double const_per_interest /*$*/, double const_per_principal /*$*/,
	double const_per_upfront_rate /*%*/) {		// [$]

	double up_front_fee = const_per_principal * (const_per_upfront_rate / 100.);
	return const_per_interest + up_front_fee;
}



void save_cf(compute_module *cm, util::matrix_t<double>& mat, int cf_line, int m_nyears, const std::string &name)
{
	ssc_number_t *arrp = cm->allocate(name, m_nyears + 1);
	for (size_t i = 0; i <= (size_t)m_nyears; i++)
		arrp[i] = (ssc_number_t)mat.at(cf_line, i);
}



enum {
	// Dispatch

	CF_TODJanEnergy,
	CF_TODFebEnergy,
	CF_TODMarEnergy,
	CF_TODAprEnergy,
	CF_TODMayEnergy,
	CF_TODJunEnergy,
	CF_TODJulEnergy,
	CF_TODAugEnergy,
	CF_TODSepEnergy,
	CF_TODOctEnergy,
	CF_TODNovEnergy,
	CF_TODDecEnergy,

	CF_TODJanRevenue,
	CF_TODFebRevenue,
	CF_TODMarRevenue,
	CF_TODAprRevenue,
	CF_TODMayRevenue,
	CF_TODJunRevenue,
	CF_TODJulRevenue,
	CF_TODAugRevenue,
	CF_TODSepRevenue,
	CF_TODOctRevenue,
	CF_TODNovRevenue,
	CF_TODDecRevenue,

	CF_max_timestep,


	CF_TOD1Energy,
	CF_TOD2Energy,
	CF_TOD3Energy,
	CF_TOD4Energy,
	CF_TOD5Energy,
	CF_TOD6Energy,
	CF_TOD7Energy,
	CF_TOD8Energy,
	CF_TOD9Energy,

	CF_TOD1JanEnergy,
	CF_TOD1FebEnergy,
	CF_TOD1MarEnergy,
	CF_TOD1AprEnergy,
	CF_TOD1MayEnergy,
	CF_TOD1JunEnergy,
	CF_TOD1JulEnergy,
	CF_TOD1AugEnergy,
	CF_TOD1SepEnergy,
	CF_TOD1OctEnergy,
	CF_TOD1NovEnergy,
	CF_TOD1DecEnergy,

	CF_TOD2JanEnergy,
	CF_TOD2FebEnergy,
	CF_TOD2MarEnergy,
	CF_TOD2AprEnergy,
	CF_TOD2MayEnergy,
	CF_TOD2JunEnergy,
	CF_TOD2JulEnergy,
	CF_TOD2AugEnergy,
	CF_TOD2SepEnergy,
	CF_TOD2OctEnergy,
	CF_TOD2NovEnergy,
	CF_TOD2DecEnergy,

	CF_TOD3JanEnergy,
	CF_TOD3FebEnergy,
	CF_TOD3MarEnergy,
	CF_TOD3AprEnergy,
	CF_TOD3MayEnergy,
	CF_TOD3JunEnergy,
	CF_TOD3JulEnergy,
	CF_TOD3AugEnergy,
	CF_TOD3SepEnergy,
	CF_TOD3OctEnergy,
	CF_TOD3NovEnergy,
	CF_TOD3DecEnergy,

	CF_TOD4JanEnergy,
	CF_TOD4FebEnergy,
	CF_TOD4MarEnergy,
	CF_TOD4AprEnergy,
	CF_TOD4MayEnergy,
	CF_TOD4JunEnergy,
	CF_TOD4JulEnergy,
	CF_TOD4AugEnergy,
	CF_TOD4SepEnergy,
	CF_TOD4OctEnergy,
	CF_TOD4NovEnergy,
	CF_TOD4DecEnergy,

	CF_TOD5JanEnergy,
	CF_TOD5FebEnergy,
	CF_TOD5MarEnergy,
	CF_TOD5AprEnergy,
	CF_TOD5MayEnergy,
	CF_TOD5JunEnergy,
	CF_TOD5JulEnergy,
	CF_TOD5AugEnergy,
	CF_TOD5SepEnergy,
	CF_TOD5OctEnergy,
	CF_TOD5NovEnergy,
	CF_TOD5DecEnergy,

	CF_TOD6JanEnergy,
	CF_TOD6FebEnergy,
	CF_TOD6MarEnergy,
	CF_TOD6AprEnergy,
	CF_TOD6MayEnergy,
	CF_TOD6JunEnergy,
	CF_TOD6JulEnergy,
	CF_TOD6AugEnergy,
	CF_TOD6SepEnergy,
	CF_TOD6OctEnergy,
	CF_TOD6NovEnergy,
	CF_TOD6DecEnergy,

	CF_TOD7JanEnergy,
	CF_TOD7FebEnergy,
	CF_TOD7MarEnergy,
	CF_TOD7AprEnergy,
	CF_TOD7MayEnergy,
	CF_TOD7JunEnergy,
	CF_TOD7JulEnergy,
	CF_TOD7AugEnergy,
	CF_TOD7SepEnergy,
	CF_TOD7OctEnergy,
	CF_TOD7NovEnergy,
	CF_TOD7DecEnergy,

	CF_TOD8JanEnergy,
	CF_TOD8FebEnergy,
	CF_TOD8MarEnergy,
	CF_TOD8AprEnergy,
	CF_TOD8MayEnergy,
	CF_TOD8JunEnergy,
	CF_TOD8JulEnergy,
	CF_TOD8AugEnergy,
	CF_TOD8SepEnergy,
	CF_TOD8OctEnergy,
	CF_TOD8NovEnergy,
	CF_TOD8DecEnergy,

	CF_TOD9JanEnergy,
	CF_TOD9FebEnergy,
	CF_TOD9MarEnergy,
	CF_TOD9AprEnergy,
	CF_TOD9MayEnergy,
	CF_TOD9JunEnergy,
	CF_TOD9JulEnergy,
	CF_TOD9AugEnergy,
	CF_TOD9SepEnergy,
	CF_TOD9OctEnergy,
	CF_TOD9NovEnergy,
	CF_TOD9DecEnergy,

	CF_TOD1Revenue,
	CF_TOD2Revenue,
	CF_TOD3Revenue,
	CF_TOD4Revenue,
	CF_TOD5Revenue,
	CF_TOD6Revenue,
	CF_TOD7Revenue,
	CF_TOD8Revenue,
	CF_TOD9Revenue,

	CF_revenue_monthly_firstyear_TOD1,
	cf_energy_sales_monthly_firstyear_TOD1,
	CF_revenue_monthly_firstyear_TOD2,
	cf_energy_sales_monthly_firstyear_TOD2,
	CF_revenue_monthly_firstyear_TOD3,
	cf_energy_sales_monthly_firstyear_TOD3,
	CF_revenue_monthly_firstyear_TOD4,
	cf_energy_sales_monthly_firstyear_TOD4,
	CF_revenue_monthly_firstyear_TOD5,
	cf_energy_sales_monthly_firstyear_TOD5,
	CF_revenue_monthly_firstyear_TOD6,
	cf_energy_sales_monthly_firstyear_TOD6,
	CF_revenue_monthly_firstyear_TOD7,
	cf_energy_sales_monthly_firstyear_TOD7,
	CF_revenue_monthly_firstyear_TOD8,
	cf_energy_sales_monthly_firstyear_TOD8,
	CF_revenue_monthly_firstyear_TOD9,
	cf_energy_sales_monthly_firstyear_TOD9,

	CF_max_dispatch
};



dispatch_calculations::dispatch_calculations(compute_module *cm, std::vector<double>& degradation, std::vector<double>& hourly_energy)
{
	init(cm, degradation, hourly_energy);
}

bool dispatch_calculations::init(compute_module *cm, std::vector<double>& degradation, std::vector<double>& hourly_energy)
{
	if (!cm) return false;

	m_cm = cm;
	m_degradation = degradation;
	m_hourly_energy = hourly_energy;
	m_timestep = (m_cm->as_integer("ppa_multiplier_model")==1);

	m_nyears = m_cm->as_integer("analysis_period");
	if (m_degradation.size() != (size_t)m_nyears + 1) return false;

	if (m_timestep)
	{
		setup_ts();
		if (m_cm->as_integer("system_use_lifetime_output"))
			compute_lifetime_dispatch_output_ts(); // TODO - finish and test this!!
		else
			compute_dispatch_output_ts();
	}
	else
	{
		setup();
		if (m_cm->as_integer("system_use_lifetime_output"))
			compute_lifetime_dispatch_output();
		else
			compute_dispatch_output();
	}
	return true;
}

bool dispatch_calculations::compute_outputs_ts(std::vector<double>& ppa)
{

	if (ppa.size() != (size_t)m_nyears + 1) return false;

	// outputs
	// dispatch energy
	save_cf(m_cm, m_cf, CF_TODJanEnergy, m_nyears, "cf_energy_sales_jan");
	save_cf(m_cm, m_cf, CF_TODFebEnergy, m_nyears, "cf_energy_sales_feb");
	save_cf(m_cm, m_cf, CF_TODMarEnergy, m_nyears, "cf_energy_sales_mar");
	save_cf(m_cm, m_cf, CF_TODAprEnergy, m_nyears, "cf_energy_sales_apr");
	save_cf(m_cm, m_cf, CF_TODMayEnergy, m_nyears, "cf_energy_sales_may");
	save_cf(m_cm, m_cf, CF_TODJunEnergy, m_nyears, "cf_energy_sales_jun");
	save_cf(m_cm, m_cf, CF_TODJulEnergy, m_nyears, "cf_energy_sales_jul");
	save_cf(m_cm, m_cf, CF_TODAugEnergy, m_nyears, "cf_energy_sales_aug");
	save_cf(m_cm, m_cf, CF_TODSepEnergy, m_nyears, "cf_energy_sales_sep");
	save_cf(m_cm, m_cf, CF_TODOctEnergy, m_nyears, "cf_energy_sales_oct");
	save_cf(m_cm, m_cf, CF_TODNovEnergy, m_nyears, "cf_energy_sales_nov");
	save_cf(m_cm, m_cf, CF_TODDecEnergy, m_nyears, "cf_energy_sales_dec");

	for (int y = 0; y <= m_nyears; y++)
	{
		// compute energy value
		m_cf.at(CF_TODJanRevenue, y) *= (ppa[y] / 100.0);
		m_cf.at(CF_TODFebRevenue, y) *= (ppa[y] / 100.0);
		m_cf.at(CF_TODMarRevenue, y) *= (ppa[y] / 100.0);
		m_cf.at(CF_TODAprRevenue, y) *= (ppa[y] / 100.0);
		m_cf.at(CF_TODMayRevenue, y) *= (ppa[y] / 100.0);
		m_cf.at(CF_TODJunRevenue, y) *= (ppa[y] / 100.0);
		m_cf.at(CF_TODJulRevenue, y) *= (ppa[y] / 100.0);
		m_cf.at(CF_TODAugRevenue, y) *= (ppa[y] / 100.0);
		m_cf.at(CF_TODSepRevenue, y) *= (ppa[y] / 100.0);
		m_cf.at(CF_TODOctRevenue, y) *= (ppa[y] / 100.0);
		m_cf.at(CF_TODNovRevenue, y) *= (ppa[y] / 100.0);
		m_cf.at(CF_TODDecRevenue, y) *= (ppa[y] / 100.0);
	}

	// dispatch revenue
	save_cf(m_cm, m_cf, CF_TODJanRevenue, m_nyears, "cf_revenue_jan");
	save_cf(m_cm, m_cf, CF_TODFebRevenue, m_nyears, "cf_revenue_feb");
	save_cf(m_cm, m_cf, CF_TODMarRevenue, m_nyears, "cf_revenue_mar");
	save_cf(m_cm, m_cf, CF_TODAprRevenue, m_nyears, "cf_revenue_apr");
	save_cf(m_cm, m_cf, CF_TODMayRevenue, m_nyears, "cf_revenue_may");
	save_cf(m_cm, m_cf, CF_TODJunRevenue, m_nyears, "cf_revenue_jun");
	save_cf(m_cm, m_cf, CF_TODJulRevenue, m_nyears, "cf_revenue_jul");
	save_cf(m_cm, m_cf, CF_TODAugRevenue, m_nyears, "cf_revenue_aug");
	save_cf(m_cm, m_cf, CF_TODSepRevenue, m_nyears, "cf_revenue_sep");
	save_cf(m_cm, m_cf, CF_TODOctRevenue, m_nyears, "cf_revenue_oct");
	save_cf(m_cm, m_cf, CF_TODNovRevenue, m_nyears, "cf_revenue_nov");
	save_cf(m_cm, m_cf, CF_TODDecRevenue, m_nyears, "cf_revenue_dec");

	return true;
}


bool dispatch_calculations::compute_outputs( std::vector<double>& ppa)
{
	if (ppa.size() != (size_t)m_nyears+1) return false;

	if (m_timestep)
	{
		return compute_outputs_ts(ppa);
	}

	size_t i;

    double dispatch_factor1 = m_dispatch_tod_factors[0];
	double dispatch_factor2 = m_dispatch_tod_factors[1];
	double dispatch_factor3 = m_dispatch_tod_factors[2];
	double dispatch_factor4 = m_dispatch_tod_factors[3];
	double dispatch_factor5 = m_dispatch_tod_factors[4];
	double dispatch_factor6 = m_dispatch_tod_factors[5];
	double dispatch_factor7 = m_dispatch_tod_factors[6];
	double dispatch_factor8 = m_dispatch_tod_factors[7];
	double dispatch_factor9 = m_dispatch_tod_factors[8];

	if (m_cm->as_integer("system_use_lifetime_output"))
		process_lifetime_dispatch_output();
	else
		process_dispatch_output();


// outputs
	// dispatch energy
	save_cf( m_cm, m_cf,  CF_TODJanEnergy, m_nyears, "cf_energy_sales_jan");
	save_cf( m_cm, m_cf,  CF_TODFebEnergy, m_nyears, "cf_energy_sales_feb");
	save_cf( m_cm, m_cf,  CF_TODMarEnergy, m_nyears, "cf_energy_sales_mar");
	save_cf( m_cm, m_cf,  CF_TODAprEnergy, m_nyears, "cf_energy_sales_apr");
	save_cf( m_cm, m_cf,  CF_TODMayEnergy, m_nyears, "cf_energy_sales_may");
	save_cf( m_cm, m_cf,  CF_TODJunEnergy, m_nyears, "cf_energy_sales_jun");
	save_cf( m_cm, m_cf,  CF_TODJulEnergy, m_nyears, "cf_energy_sales_jul");
	save_cf( m_cm, m_cf,  CF_TODAugEnergy, m_nyears, "cf_energy_sales_aug");
	save_cf( m_cm, m_cf,  CF_TODSepEnergy, m_nyears, "cf_energy_sales_sep");
	save_cf( m_cm, m_cf,  CF_TODOctEnergy, m_nyears, "cf_energy_sales_oct");
	save_cf( m_cm, m_cf,  CF_TODNovEnergy, m_nyears, "cf_energy_sales_nov");
	save_cf( m_cm, m_cf,  CF_TODDecEnergy, m_nyears, "cf_energy_sales_dec");

	save_cf( m_cm, m_cf,  CF_TOD1Energy, m_nyears, "cf_energy_sales_dispatch1");
	save_cf( m_cm, m_cf,  CF_TOD2Energy, m_nyears, "cf_energy_sales_dispatch2");
	save_cf( m_cm, m_cf,  CF_TOD3Energy, m_nyears, "cf_energy_sales_dispatch3");
	save_cf( m_cm, m_cf,  CF_TOD4Energy, m_nyears, "cf_energy_sales_dispatch4");
	save_cf( m_cm, m_cf,  CF_TOD5Energy, m_nyears, "cf_energy_sales_dispatch5");
	save_cf( m_cm, m_cf,  CF_TOD6Energy, m_nyears, "cf_energy_sales_dispatch6");
	save_cf( m_cm, m_cf,  CF_TOD7Energy, m_nyears, "cf_energy_sales_dispatch7");
	save_cf( m_cm, m_cf,  CF_TOD8Energy, m_nyears, "cf_energy_sales_dispatch8");
	save_cf( m_cm, m_cf,  CF_TOD9Energy, m_nyears, "cf_energy_sales_dispatch9");

	// dispatch revenue cents/kWh ppa input in cents per kWh - revenue in dollars
	for (i = 0; i <= (size_t)m_nyears; i++)
	{
		m_cf.at(CF_TOD1Revenue, i) = ppa[i] / 100.0 * dispatch_factor1 * m_cf.at(CF_TOD1Energy, i);
		m_cf.at(CF_TOD2Revenue, i) = ppa[i] / 100.0 * dispatch_factor2 * m_cf.at(CF_TOD2Energy, i);
		m_cf.at(CF_TOD3Revenue, i) = ppa[i] / 100.0 * dispatch_factor3 * m_cf.at(CF_TOD3Energy, i);
		m_cf.at(CF_TOD4Revenue, i) = ppa[i] / 100.0 * dispatch_factor4 * m_cf.at(CF_TOD4Energy, i);
		m_cf.at(CF_TOD5Revenue, i) = ppa[i] / 100.0 * dispatch_factor5 *m_cf.at(CF_TOD5Energy, i);
		m_cf.at(CF_TOD6Revenue, i) = ppa[i] / 100.0 * dispatch_factor6 * m_cf.at(CF_TOD6Energy, i);
		m_cf.at(CF_TOD7Revenue, i) = ppa[i] / 100.0 * dispatch_factor7 * m_cf.at(CF_TOD7Energy, i);
		m_cf.at(CF_TOD8Revenue, i) = ppa[i] / 100.0 * dispatch_factor8 * m_cf.at(CF_TOD8Energy, i);
		m_cf.at(CF_TOD9Revenue, i) = ppa[i] / 100.0 * dispatch_factor9 * m_cf.at(CF_TOD9Energy, i);
	}

	save_cf( m_cm, m_cf,  CF_TOD1Revenue, m_nyears, "cf_revenue_dispatch1");
	save_cf( m_cm, m_cf,  CF_TOD2Revenue, m_nyears, "cf_revenue_dispatch2");
	save_cf( m_cm, m_cf,  CF_TOD3Revenue, m_nyears, "cf_revenue_dispatch3");
	save_cf( m_cm, m_cf,  CF_TOD4Revenue, m_nyears, "cf_revenue_dispatch4");
	save_cf( m_cm, m_cf,  CF_TOD5Revenue, m_nyears, "cf_revenue_dispatch5");
	save_cf( m_cm, m_cf,  CF_TOD6Revenue, m_nyears, "cf_revenue_dispatch6");
	save_cf( m_cm, m_cf,  CF_TOD7Revenue, m_nyears, "cf_revenue_dispatch7");
	save_cf( m_cm, m_cf,  CF_TOD8Revenue, m_nyears, "cf_revenue_dispatch8");
	save_cf( m_cm, m_cf,  CF_TOD9Revenue, m_nyears, "cf_revenue_dispatch9");

	for (i = 0; i <= (size_t)m_nyears; i++)
	{
		m_cf.at(CF_TODJanRevenue, i) = ppa[i] / 100.0 * (
			dispatch_factor1 * m_cf.at(CF_TOD1JanEnergy, i) +
			dispatch_factor2 * m_cf.at(CF_TOD2JanEnergy, i) +
			dispatch_factor3 * m_cf.at(CF_TOD3JanEnergy, i) +
			dispatch_factor4 * m_cf.at(CF_TOD4JanEnergy, i) +
			dispatch_factor5 * m_cf.at(CF_TOD5JanEnergy, i) +
			dispatch_factor6 * m_cf.at(CF_TOD6JanEnergy, i) +
			dispatch_factor7 * m_cf.at(CF_TOD7JanEnergy, i) +
			dispatch_factor8 * m_cf.at(CF_TOD8JanEnergy, i) +
			dispatch_factor9 * m_cf.at(CF_TOD9JanEnergy, i));
	}
	save_cf( m_cm, m_cf,  CF_TODJanRevenue, m_nyears, "cf_revenue_jan");

	for (i = 0; i <= (size_t)m_nyears; i++)
	{
		m_cf.at(CF_TODFebRevenue, i) = ppa[i] / 100.0 * (
			dispatch_factor1 * m_cf.at(CF_TOD1FebEnergy, i) +
			dispatch_factor2 * m_cf.at(CF_TOD2FebEnergy, i) +
			dispatch_factor3 * m_cf.at(CF_TOD3FebEnergy, i) +
			dispatch_factor4 * m_cf.at(CF_TOD4FebEnergy, i) +
			dispatch_factor5 * m_cf.at(CF_TOD5FebEnergy, i) +
			dispatch_factor6 * m_cf.at(CF_TOD6FebEnergy, i) +
			dispatch_factor7 * m_cf.at(CF_TOD7FebEnergy, i) +
			dispatch_factor8 * m_cf.at(CF_TOD8FebEnergy, i) +
			dispatch_factor9 * m_cf.at(CF_TOD9FebEnergy, i));
	}
	save_cf( m_cm, m_cf,  CF_TODFebRevenue, m_nyears, "cf_revenue_feb");

	for (i = 0; i <= (size_t)m_nyears; i++)
	{
		m_cf.at(CF_TODMarRevenue, i) = ppa[i] / 100.0 * (
			dispatch_factor1 * m_cf.at(CF_TOD1MarEnergy, i) +
			dispatch_factor2 * m_cf.at(CF_TOD2MarEnergy, i) +
			dispatch_factor3 * m_cf.at(CF_TOD3MarEnergy, i) +
			dispatch_factor4 * m_cf.at(CF_TOD4MarEnergy, i) +
			dispatch_factor5 * m_cf.at(CF_TOD5MarEnergy, i) +
			dispatch_factor6 * m_cf.at(CF_TOD6MarEnergy, i) +
			dispatch_factor7 * m_cf.at(CF_TOD7MarEnergy, i) +
			dispatch_factor8 * m_cf.at(CF_TOD8MarEnergy, i) +
			dispatch_factor9 * m_cf.at(CF_TOD9MarEnergy, i));
	}
	save_cf( m_cm, m_cf,  CF_TODMarRevenue, m_nyears, "cf_revenue_mar");

	for (i = 0; i <= (size_t)m_nyears; i++)
	{
		m_cf.at(CF_TODAprRevenue, i) = ppa[i] / 100.0 * (
			dispatch_factor1 * m_cf.at(CF_TOD1AprEnergy, i) +
			dispatch_factor2 * m_cf.at(CF_TOD2AprEnergy, i) +
			dispatch_factor3 * m_cf.at(CF_TOD3AprEnergy, i) +
			dispatch_factor4 * m_cf.at(CF_TOD4AprEnergy, i) +
			dispatch_factor5 * m_cf.at(CF_TOD5AprEnergy, i) +
			dispatch_factor6 * m_cf.at(CF_TOD6AprEnergy, i) +
			dispatch_factor7 * m_cf.at(CF_TOD7AprEnergy, i) +
			dispatch_factor8 * m_cf.at(CF_TOD8AprEnergy, i) +
			dispatch_factor9 * m_cf.at(CF_TOD9AprEnergy, i));
	}
	save_cf( m_cm, m_cf,  CF_TODAprRevenue, m_nyears, "cf_revenue_apr");

	for (i = 0; i <= (size_t)m_nyears; i++)
	{
		m_cf.at(CF_TODMayRevenue, i) = ppa[i] / 100.0 * (
			dispatch_factor1 * m_cf.at(CF_TOD1MayEnergy, i) +
			dispatch_factor2 * m_cf.at(CF_TOD2MayEnergy, i) +
			dispatch_factor3 * m_cf.at(CF_TOD3MayEnergy, i) +
			dispatch_factor4 * m_cf.at(CF_TOD4MayEnergy, i) +
			dispatch_factor5 * m_cf.at(CF_TOD5MayEnergy, i) +
			dispatch_factor6 * m_cf.at(CF_TOD6MayEnergy, i) +
			dispatch_factor7 * m_cf.at(CF_TOD7MayEnergy, i) +
			dispatch_factor8 * m_cf.at(CF_TOD8MayEnergy, i) +
			dispatch_factor9 * m_cf.at(CF_TOD9MayEnergy, i));
	}
	save_cf( m_cm, m_cf,  CF_TODMayRevenue, m_nyears, "cf_revenue_may");

	for (i = 0; i <= (size_t)m_nyears; i++)
	{
		m_cf.at(CF_TODJunRevenue, i) = ppa[i] / 100.0 * (
			dispatch_factor1 * m_cf.at(CF_TOD1JunEnergy, i) +
			dispatch_factor2 * m_cf.at(CF_TOD2JunEnergy, i) +
			dispatch_factor3 * m_cf.at(CF_TOD3JunEnergy, i) +
			dispatch_factor4 * m_cf.at(CF_TOD4JunEnergy, i) +
			dispatch_factor5 * m_cf.at(CF_TOD5JunEnergy, i) +
			dispatch_factor6 * m_cf.at(CF_TOD6JunEnergy, i) +
			dispatch_factor7 * m_cf.at(CF_TOD7JunEnergy, i) +
			dispatch_factor8 * m_cf.at(CF_TOD8JunEnergy, i) +
			dispatch_factor9 * m_cf.at(CF_TOD9JunEnergy, i));
	}
	save_cf( m_cm, m_cf,  CF_TODJunRevenue, m_nyears, "cf_revenue_jun");

	for (i = 0; i <= (size_t)m_nyears; i++)
	{
		m_cf.at(CF_TODJulRevenue, i) = ppa[i] / 100.0 * (
			dispatch_factor1 * m_cf.at(CF_TOD1JulEnergy, i) +
			dispatch_factor2 * m_cf.at(CF_TOD2JulEnergy, i) +
			dispatch_factor3 * m_cf.at(CF_TOD3JulEnergy, i) +
			dispatch_factor4 * m_cf.at(CF_TOD4JulEnergy, i) +
			dispatch_factor5 * m_cf.at(CF_TOD5JulEnergy, i) +
			dispatch_factor6 * m_cf.at(CF_TOD6JulEnergy, i) +
			dispatch_factor7 * m_cf.at(CF_TOD7JulEnergy, i) +
			dispatch_factor8 * m_cf.at(CF_TOD8JulEnergy, i) +
			dispatch_factor9 * m_cf.at(CF_TOD9JulEnergy, i));
	}
	save_cf( m_cm, m_cf,  CF_TODJulRevenue, m_nyears, "cf_revenue_jul");

	for (i = 0; i <= (size_t)m_nyears; i++)
	{
		m_cf.at(CF_TODAugRevenue, i) = ppa[i] / 100.0 * (
			dispatch_factor1 * m_cf.at(CF_TOD1AugEnergy, i) +
			dispatch_factor2 * m_cf.at(CF_TOD2AugEnergy, i) +
			dispatch_factor3 * m_cf.at(CF_TOD3AugEnergy, i) +
			dispatch_factor4 * m_cf.at(CF_TOD4AugEnergy, i) +
			dispatch_factor5 * m_cf.at(CF_TOD5AugEnergy, i) +
			dispatch_factor6 * m_cf.at(CF_TOD6AugEnergy, i) +
			dispatch_factor7 * m_cf.at(CF_TOD7AugEnergy, i) +
			dispatch_factor8 * m_cf.at(CF_TOD8AugEnergy, i) +
			dispatch_factor9 * m_cf.at(CF_TOD9AugEnergy, i));
	}
	save_cf( m_cm, m_cf,  CF_TODAugRevenue, m_nyears, "cf_revenue_aug");

	for (i = 0; i <= (size_t)m_nyears; i++)
	{
		m_cf.at(CF_TODSepRevenue, i) = ppa[i] / 100.0 * (
			dispatch_factor1 * m_cf.at(CF_TOD1SepEnergy, i) +
			dispatch_factor2 * m_cf.at(CF_TOD2SepEnergy, i) +
			dispatch_factor3 * m_cf.at(CF_TOD3SepEnergy, i) +
			dispatch_factor4 * m_cf.at(CF_TOD4SepEnergy, i) +
			dispatch_factor5 * m_cf.at(CF_TOD5SepEnergy, i) +
			dispatch_factor6 * m_cf.at(CF_TOD6SepEnergy, i) +
			dispatch_factor7 * m_cf.at(CF_TOD7SepEnergy, i) +
			dispatch_factor8 * m_cf.at(CF_TOD8SepEnergy, i) +
			dispatch_factor9 * m_cf.at(CF_TOD9SepEnergy, i));
	}
	save_cf( m_cm, m_cf,  CF_TODSepRevenue, m_nyears, "cf_revenue_sep");

	for (i = 0; i <= (size_t)m_nyears; i++)
	{
		m_cf.at(CF_TODOctRevenue, i) = ppa[i] / 100.0 * (
			dispatch_factor1 * m_cf.at(CF_TOD1OctEnergy, i) +
			dispatch_factor2 * m_cf.at(CF_TOD2OctEnergy, i) +
			dispatch_factor3 * m_cf.at(CF_TOD3OctEnergy, i) +
			dispatch_factor4 * m_cf.at(CF_TOD4OctEnergy, i) +
			dispatch_factor5 * m_cf.at(CF_TOD5OctEnergy, i) +
			dispatch_factor6 * m_cf.at(CF_TOD6OctEnergy, i) +
			dispatch_factor7 * m_cf.at(CF_TOD7OctEnergy, i) +
			dispatch_factor8 * m_cf.at(CF_TOD8OctEnergy, i) +
			dispatch_factor9 * m_cf.at(CF_TOD9OctEnergy, i));
	}
	save_cf( m_cm, m_cf,  CF_TODOctRevenue, m_nyears, "cf_revenue_oct");

	for (i = 0; i <= (size_t)m_nyears; i++)
	{
		m_cf.at(CF_TODNovRevenue, i) = ppa[i] / 100.0 * (
			dispatch_factor1 * m_cf.at(CF_TOD1NovEnergy, i) +
			dispatch_factor2 * m_cf.at(CF_TOD2NovEnergy, i) +
			dispatch_factor3 * m_cf.at(CF_TOD3NovEnergy, i) +
			dispatch_factor4 * m_cf.at(CF_TOD4NovEnergy, i) +
			dispatch_factor5 * m_cf.at(CF_TOD5NovEnergy, i) +
			dispatch_factor6 * m_cf.at(CF_TOD6NovEnergy, i) +
			dispatch_factor7 * m_cf.at(CF_TOD7NovEnergy, i) +
			dispatch_factor8 * m_cf.at(CF_TOD8NovEnergy, i) +
			dispatch_factor9 * m_cf.at(CF_TOD9NovEnergy, i));
	}
	save_cf( m_cm, m_cf,  CF_TODNovRevenue, m_nyears, "cf_revenue_nov");

	for (i = 0; i <= (size_t)m_nyears; i++)
	{
		m_cf.at(CF_TODDecRevenue, i) = ppa[i] / 100.0 * (
			dispatch_factor1 * m_cf.at(CF_TOD1DecEnergy, i) +
			dispatch_factor2 * m_cf.at(CF_TOD2DecEnergy, i) +
			dispatch_factor3 * m_cf.at(CF_TOD3DecEnergy, i) +
			dispatch_factor4 * m_cf.at(CF_TOD4DecEnergy, i) +
			dispatch_factor5 * m_cf.at(CF_TOD5DecEnergy, i) +
			dispatch_factor6 * m_cf.at(CF_TOD6DecEnergy, i) +
			dispatch_factor7 * m_cf.at(CF_TOD7DecEnergy, i) +
			dispatch_factor8 * m_cf.at(CF_TOD8DecEnergy, i) +
			dispatch_factor9 * m_cf.at(CF_TOD9DecEnergy, i));
	}
	save_cf( m_cm, m_cf,  CF_TODDecRevenue, m_nyears, "cf_revenue_Dec");

	/*
	m_cf.at(CF_revenue_monthly_firstyear, 0) = m_cf.at(CF_TODJanRevenue, 1);
	m_cf.at(CF_revenue_monthly_firstyear, 1) = m_cf.at(CF_TODFebRevenue, 1);
	m_cf.at(CF_revenue_monthly_firstyear, 2) = m_cf.at(CF_TODMarRevenue, 1);
	m_cf.at(CF_revenue_monthly_firstyear, 3) = m_cf.at(CF_TODAprRevenue, 1);
	m_cf.at(CF_revenue_monthly_firstyear, 4) = m_cf.at(CF_TODMayRevenue, 1);
	m_cf.at(CF_revenue_monthly_firstyear, 5) = m_cf.at(CF_TODJunRevenue, 1);
	m_cf.at(CF_revenue_monthly_firstyear, 6) = m_cf.at(CF_TODJulRevenue, 1);
	m_cf.at(CF_revenue_monthly_firstyear, 7) = m_cf.at(CF_TODAugRevenue, 1);
	m_cf.at(CF_revenue_monthly_firstyear, 8) = m_cf.at(CF_TODSepRevenue, 1);
	m_cf.at(CF_revenue_monthly_firstyear, 9) = m_cf.at(CF_TODOctRevenue, 1);
	m_cf.at(CF_revenue_monthly_firstyear, 10) = m_cf.at(CF_TODNovRevenue, 1);
	m_cf.at(CF_revenue_monthly_firstyear, 11) = m_cf.at(CF_TODDecRevenue, 1);

	m_cf.at(cf_energy_sales_monthly_firstyear, 0) = m_cf.at(CF_TODJanEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear, 1) = m_cf.at(CF_TODFebEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear, 2) = m_cf.at(CF_TODMarEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear, 3) = m_cf.at(CF_TODAprEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear, 4) = m_cf.at(CF_TODMayEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear, 5) = m_cf.at(CF_TODJunEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear, 6) = m_cf.at(CF_TODJulEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear, 7) = m_cf.at(CF_TODAugEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear, 8) = m_cf.at(CF_TODSepEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear, 9) = m_cf.at(CF_TODOctEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear, 10) = m_cf.at(CF_TODNovEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear, 11) = m_cf.at(CF_TODDecEnergy, 1);

	save_cf( m_cm, m_cf,  CF_revenue_monthly_firstyear, 11, "cf_revenue_monthly_firstyear");
	save_cf( m_cm, m_cf,  cf_energy_sales_monthly_firstyear, 11, "cf_energy_sales_monthly_firstyear");
	*/


	m_cf.at(CF_revenue_monthly_firstyear_TOD1, 0) = ppa[1] / 100.0 *
		dispatch_factor1 * m_cf.at(CF_TOD1JanEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD1, 1) = ppa[1] / 100.0 *
		dispatch_factor1 * m_cf.at(CF_TOD1FebEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD1, 2) = ppa[1] / 100.0 *
		dispatch_factor1 * m_cf.at(CF_TOD1MarEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD1, 3) = ppa[1] / 100.0 *
		dispatch_factor1 * m_cf.at(CF_TOD1AprEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD1, 4) = ppa[1] / 100.0 *
		dispatch_factor1 * m_cf.at(CF_TOD1MayEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD1, 5) = ppa[1] / 100.0 *
		dispatch_factor1 * m_cf.at(CF_TOD1JunEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD1, 6) = ppa[1] / 100.0 *
		dispatch_factor1 * m_cf.at(CF_TOD1JulEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD1, 7) = ppa[1] / 100.0 *
		dispatch_factor1 * m_cf.at(CF_TOD1AugEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD1, 8) = ppa[1] / 100.0 *
		dispatch_factor1 * m_cf.at(CF_TOD1SepEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD1, 9) = ppa[1] / 100.0 *
		dispatch_factor1 * m_cf.at(CF_TOD1OctEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD1, 10) = ppa[1] / 100.0 *
		dispatch_factor1 * m_cf.at(CF_TOD1NovEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD1, 11) = ppa[1] / 100.0 *
		dispatch_factor1 * m_cf.at(CF_TOD1DecEnergy, 1);

	m_cf.at(cf_energy_sales_monthly_firstyear_TOD1, 0) = m_cf.at(CF_TOD1JanEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD1, 1) = m_cf.at(CF_TOD1FebEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD1, 2) = m_cf.at(CF_TOD1MarEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD1, 3) = m_cf.at(CF_TOD1AprEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD1, 4) = m_cf.at(CF_TOD1MayEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD1, 5) = m_cf.at(CF_TOD1JunEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD1, 6) = m_cf.at(CF_TOD1JulEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD1, 7) = m_cf.at(CF_TOD1AugEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD1, 8) = m_cf.at(CF_TOD1SepEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD1, 9) = m_cf.at(CF_TOD1OctEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD1, 10) = m_cf.at(CF_TOD1NovEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD1, 11) = m_cf.at(CF_TOD1DecEnergy, 1);

	save_cf( m_cm, m_cf,  CF_revenue_monthly_firstyear_TOD1, 11, "cf_revenue_monthly_firstyear_TOD1");
	save_cf( m_cm, m_cf,  cf_energy_sales_monthly_firstyear_TOD1, 11, "cf_energy_sales_monthly_firstyear_TOD1");


	m_cf.at(CF_revenue_monthly_firstyear_TOD2, 0) = ppa[1] / 100.0 *
		dispatch_factor2 * m_cf.at(CF_TOD2JanEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD2, 1) = ppa[1] / 100.0 *
		dispatch_factor2 * m_cf.at(CF_TOD2FebEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD2, 2) = ppa[1] / 100.0 *
		dispatch_factor2 * m_cf.at(CF_TOD2MarEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD2, 3) = ppa[1] / 100.0 *
		dispatch_factor2 * m_cf.at(CF_TOD2AprEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD2, 4) = ppa[1] / 100.0 *
		dispatch_factor2 * m_cf.at(CF_TOD2MayEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD2, 5) = ppa[1] / 100.0 *
		dispatch_factor2 * m_cf.at(CF_TOD2JunEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD2, 6) = ppa[1] / 100.0 *
		dispatch_factor2 * m_cf.at(CF_TOD2JulEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD2, 7) = ppa[1] / 100.0 *
		dispatch_factor2 * m_cf.at(CF_TOD2AugEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD2, 8) = ppa[1] / 100.0 *
		dispatch_factor2 * m_cf.at(CF_TOD2SepEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD2, 9) = ppa[1] / 100.0 *
		dispatch_factor2 * m_cf.at(CF_TOD2OctEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD2, 10) = ppa[1] / 100.0 *
		dispatch_factor2 * m_cf.at(CF_TOD2NovEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD2, 11) = ppa[1] / 100.0 *
		dispatch_factor2 * m_cf.at(CF_TOD2DecEnergy, 1);

	m_cf.at(cf_energy_sales_monthly_firstyear_TOD2, 0) = m_cf.at(CF_TOD2JanEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD2, 1) = m_cf.at(CF_TOD2FebEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD2, 2) = m_cf.at(CF_TOD2MarEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD2, 3) = m_cf.at(CF_TOD2AprEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD2, 4) = m_cf.at(CF_TOD2MayEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD2, 5) = m_cf.at(CF_TOD2JunEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD2, 6) = m_cf.at(CF_TOD2JulEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD2, 7) = m_cf.at(CF_TOD2AugEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD2, 8) = m_cf.at(CF_TOD2SepEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD2, 9) = m_cf.at(CF_TOD2OctEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD2, 10) = m_cf.at(CF_TOD2NovEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD2, 11) = m_cf.at(CF_TOD2DecEnergy, 1);

	save_cf( m_cm, m_cf,  CF_revenue_monthly_firstyear_TOD2, 11, "cf_revenue_monthly_firstyear_TOD2");
	save_cf( m_cm, m_cf,  cf_energy_sales_monthly_firstyear_TOD2, 11, "cf_energy_sales_monthly_firstyear_TOD2");


	m_cf.at(CF_revenue_monthly_firstyear_TOD3, 0) = ppa[1] / 100.0 *
		dispatch_factor3 * m_cf.at(CF_TOD3JanEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD3, 1) = ppa[1] / 100.0 *
		dispatch_factor3 * m_cf.at(CF_TOD3FebEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD3, 2) = ppa[1] / 100.0 *
		dispatch_factor3 * m_cf.at(CF_TOD3MarEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD3, 3) = ppa[1] / 100.0 *
		dispatch_factor3 * m_cf.at(CF_TOD3AprEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD3, 4) = ppa[1] / 100.0 *
		dispatch_factor3 * m_cf.at(CF_TOD3MayEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD3, 5) = ppa[1] / 100.0 *
		dispatch_factor3 * m_cf.at(CF_TOD3JunEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD3, 6) = ppa[1] / 100.0 *
		dispatch_factor3 * m_cf.at(CF_TOD3JulEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD3, 7) = ppa[1] / 100.0 *
		dispatch_factor3 * m_cf.at(CF_TOD3AugEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD3, 8) = ppa[1] / 100.0 *
		dispatch_factor3 * m_cf.at(CF_TOD3SepEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD3, 9) = ppa[1] / 100.0 *
		dispatch_factor3 * m_cf.at(CF_TOD3OctEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD3, 10) = ppa[1] / 100.0 *
		dispatch_factor3 * m_cf.at(CF_TOD3NovEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD3, 11) = ppa[1] / 100.0 *
		dispatch_factor3 * m_cf.at(CF_TOD3DecEnergy, 1);

	m_cf.at(cf_energy_sales_monthly_firstyear_TOD3, 0) = m_cf.at(CF_TOD3JanEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD3, 1) = m_cf.at(CF_TOD3FebEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD3, 2) = m_cf.at(CF_TOD3MarEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD3, 3) = m_cf.at(CF_TOD3AprEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD3, 4) = m_cf.at(CF_TOD3MayEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD3, 5) = m_cf.at(CF_TOD3JunEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD3, 6) = m_cf.at(CF_TOD3JulEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD3, 7) = m_cf.at(CF_TOD3AugEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD3, 8) = m_cf.at(CF_TOD3SepEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD3, 9) = m_cf.at(CF_TOD3OctEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD3, 10) = m_cf.at(CF_TOD3NovEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD3, 11) = m_cf.at(CF_TOD3DecEnergy, 1);

	save_cf( m_cm, m_cf,  CF_revenue_monthly_firstyear_TOD3, 11, "cf_revenue_monthly_firstyear_TOD3");
	save_cf( m_cm, m_cf,  cf_energy_sales_monthly_firstyear_TOD3, 11, "cf_energy_sales_monthly_firstyear_TOD3");


	m_cf.at(CF_revenue_monthly_firstyear_TOD4, 0) = ppa[1] / 100.0 *
		dispatch_factor4 * m_cf.at(CF_TOD4JanEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD4, 1) = ppa[1] / 100.0 *
		dispatch_factor4 * m_cf.at(CF_TOD4FebEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD4, 2) = ppa[1] / 100.0 *
		dispatch_factor4 * m_cf.at(CF_TOD4MarEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD4, 3) = ppa[1] / 100.0 *
		dispatch_factor4 * m_cf.at(CF_TOD4AprEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD4, 4) = ppa[1] / 100.0 *
		dispatch_factor4 * m_cf.at(CF_TOD4MayEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD4, 5) = ppa[1] / 100.0 *
		dispatch_factor4 * m_cf.at(CF_TOD4JunEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD4, 6) = ppa[1] / 100.0 *
		dispatch_factor4 * m_cf.at(CF_TOD4JulEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD4, 7) = ppa[1] / 100.0 *
		dispatch_factor4 * m_cf.at(CF_TOD4AugEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD4, 8) = ppa[1] / 100.0 *
		dispatch_factor4 * m_cf.at(CF_TOD4SepEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD4, 9) = ppa[1] / 100.0 *
		dispatch_factor4 * m_cf.at(CF_TOD4OctEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD4, 10) = ppa[1] / 100.0 *
		dispatch_factor4 * m_cf.at(CF_TOD4NovEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD4, 11) = ppa[1] / 100.0 *
		dispatch_factor4 * m_cf.at(CF_TOD4DecEnergy, 1);

	m_cf.at(cf_energy_sales_monthly_firstyear_TOD4, 0) = m_cf.at(CF_TOD4JanEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD4, 1) = m_cf.at(CF_TOD4FebEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD4, 2) = m_cf.at(CF_TOD4MarEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD4, 3) = m_cf.at(CF_TOD4AprEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD4, 4) = m_cf.at(CF_TOD4MayEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD4, 5) = m_cf.at(CF_TOD4JunEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD4, 6) = m_cf.at(CF_TOD4JulEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD4, 7) = m_cf.at(CF_TOD4AugEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD4, 8) = m_cf.at(CF_TOD4SepEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD4, 9) = m_cf.at(CF_TOD4OctEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD4, 10) = m_cf.at(CF_TOD4NovEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD4, 11) = m_cf.at(CF_TOD4DecEnergy, 1);

	save_cf( m_cm, m_cf,  CF_revenue_monthly_firstyear_TOD4, 11, "cf_revenue_monthly_firstyear_TOD4");
	save_cf( m_cm, m_cf,  cf_energy_sales_monthly_firstyear_TOD4, 11, "cf_energy_sales_monthly_firstyear_TOD4");


	m_cf.at(CF_revenue_monthly_firstyear_TOD5, 0) = ppa[1] / 100.0 *
		dispatch_factor5 * m_cf.at(CF_TOD5JanEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD5, 1) = ppa[1] / 100.0 *
		dispatch_factor5 * m_cf.at(CF_TOD5FebEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD5, 2) = ppa[1] / 100.0 *
		dispatch_factor5 * m_cf.at(CF_TOD5MarEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD5, 3) = ppa[1] / 100.0 *
		dispatch_factor5 * m_cf.at(CF_TOD5AprEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD5, 4) = ppa[1] / 100.0 *
		dispatch_factor5 * m_cf.at(CF_TOD5MayEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD5, 5) = ppa[1] / 100.0 *
		dispatch_factor5 * m_cf.at(CF_TOD5JunEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD5, 6) = ppa[1] / 100.0 *
		dispatch_factor5 * m_cf.at(CF_TOD5JulEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD5, 7) = ppa[1] / 100.0 *
		dispatch_factor5 * m_cf.at(CF_TOD5AugEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD5, 8) = ppa[1] / 100.0 *
		dispatch_factor5 * m_cf.at(CF_TOD5SepEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD5, 9) = ppa[1] / 100.0 *
		dispatch_factor5 * m_cf.at(CF_TOD5OctEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD5, 10) = ppa[1] / 100.0 *
		dispatch_factor5 * m_cf.at(CF_TOD5NovEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD5, 11) = ppa[1] / 100.0 *
		dispatch_factor5 * m_cf.at(CF_TOD5DecEnergy, 1);

	m_cf.at(cf_energy_sales_monthly_firstyear_TOD5, 0) = m_cf.at(CF_TOD5JanEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD5, 1) = m_cf.at(CF_TOD5FebEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD5, 2) = m_cf.at(CF_TOD5MarEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD5, 3) = m_cf.at(CF_TOD5AprEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD5, 4) = m_cf.at(CF_TOD5MayEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD5, 5) = m_cf.at(CF_TOD5JunEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD5, 6) = m_cf.at(CF_TOD5JulEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD5, 7) = m_cf.at(CF_TOD5AugEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD5, 8) = m_cf.at(CF_TOD5SepEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD5, 9) = m_cf.at(CF_TOD5OctEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD5, 10) = m_cf.at(CF_TOD5NovEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD5, 11) = m_cf.at(CF_TOD5DecEnergy, 1);

	save_cf( m_cm, m_cf,  CF_revenue_monthly_firstyear_TOD5, 11, "cf_revenue_monthly_firstyear_TOD5");
	save_cf( m_cm, m_cf,  cf_energy_sales_monthly_firstyear_TOD5, 11, "cf_energy_sales_monthly_firstyear_TOD5");


	m_cf.at(CF_revenue_monthly_firstyear_TOD6, 0) = ppa[1] / 100.0 *
		dispatch_factor6 * m_cf.at(CF_TOD6JanEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD6, 1) = ppa[1] / 100.0 *
		dispatch_factor6 * m_cf.at(CF_TOD6FebEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD6, 2) = ppa[1] / 100.0 *
		dispatch_factor6 * m_cf.at(CF_TOD6MarEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD6, 3) = ppa[1] / 100.0 *
		dispatch_factor6 * m_cf.at(CF_TOD6AprEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD6, 4) = ppa[1] / 100.0 *
		dispatch_factor6 * m_cf.at(CF_TOD6MayEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD6, 5) = ppa[1] / 100.0 *
		dispatch_factor6 * m_cf.at(CF_TOD6JunEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD6, 6) = ppa[1] / 100.0 *
		dispatch_factor6 * m_cf.at(CF_TOD6JulEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD6, 7) = ppa[1] / 100.0 *
		dispatch_factor6 * m_cf.at(CF_TOD6AugEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD6, 8) = ppa[1] / 100.0 *
		dispatch_factor6 * m_cf.at(CF_TOD6SepEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD6, 9) = ppa[1] / 100.0 *
		dispatch_factor6 * m_cf.at(CF_TOD6OctEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD6, 10) = ppa[1] / 100.0 *
		dispatch_factor6 * m_cf.at(CF_TOD6NovEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD6, 11) = ppa[1] / 100.0 *
		dispatch_factor6 * m_cf.at(CF_TOD6DecEnergy, 1);

	m_cf.at(cf_energy_sales_monthly_firstyear_TOD6, 0) = m_cf.at(CF_TOD6JanEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD6, 1) = m_cf.at(CF_TOD6FebEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD6, 2) = m_cf.at(CF_TOD6MarEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD6, 3) = m_cf.at(CF_TOD6AprEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD6, 4) = m_cf.at(CF_TOD6MayEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD6, 5) = m_cf.at(CF_TOD6JunEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD6, 6) = m_cf.at(CF_TOD6JulEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD6, 7) = m_cf.at(CF_TOD6AugEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD6, 8) = m_cf.at(CF_TOD6SepEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD6, 9) = m_cf.at(CF_TOD6OctEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD6, 10) = m_cf.at(CF_TOD6NovEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD6, 11) = m_cf.at(CF_TOD6DecEnergy, 1);

	save_cf( m_cm, m_cf,  CF_revenue_monthly_firstyear_TOD6, 11, "cf_revenue_monthly_firstyear_TOD6");
	save_cf( m_cm, m_cf,  cf_energy_sales_monthly_firstyear_TOD6, 11, "cf_energy_sales_monthly_firstyear_TOD6");


	m_cf.at(CF_revenue_monthly_firstyear_TOD7, 0) = ppa[1] / 100.0 *
		dispatch_factor7 * m_cf.at(CF_TOD7JanEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD7, 1) = ppa[1] / 100.0 *
		dispatch_factor7 * m_cf.at(CF_TOD7FebEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD7, 2) = ppa[1] / 100.0 *
		dispatch_factor7 * m_cf.at(CF_TOD7MarEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD7, 3) = ppa[1] / 100.0 *
		dispatch_factor7 * m_cf.at(CF_TOD7AprEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD7, 4) = ppa[1] / 100.0 *
		dispatch_factor7 * m_cf.at(CF_TOD7MayEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD7, 5) = ppa[1] / 100.0 *
		dispatch_factor7 * m_cf.at(CF_TOD7JunEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD7, 6) = ppa[1] / 100.0 *
		dispatch_factor7 * m_cf.at(CF_TOD7JulEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD7, 7) = ppa[1] / 100.0 *
		dispatch_factor7 * m_cf.at(CF_TOD7AugEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD7, 8) = ppa[1] / 100.0 *
		dispatch_factor7 * m_cf.at(CF_TOD7SepEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD7, 9) = ppa[1] / 100.0 *
		dispatch_factor7 * m_cf.at(CF_TOD7OctEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD7, 10) = ppa[1] / 100.0 *
		dispatch_factor7 * m_cf.at(CF_TOD7NovEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD7, 11) = ppa[1] / 100.0 *
		dispatch_factor7 * m_cf.at(CF_TOD7DecEnergy, 1);

	m_cf.at(cf_energy_sales_monthly_firstyear_TOD7, 0) = m_cf.at(CF_TOD7JanEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD7, 1) = m_cf.at(CF_TOD7FebEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD7, 2) = m_cf.at(CF_TOD7MarEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD7, 3) = m_cf.at(CF_TOD7AprEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD7, 4) = m_cf.at(CF_TOD7MayEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD7, 5) = m_cf.at(CF_TOD7JunEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD7, 6) = m_cf.at(CF_TOD7JulEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD7, 7) = m_cf.at(CF_TOD7AugEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD7, 8) = m_cf.at(CF_TOD7SepEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD7, 9) = m_cf.at(CF_TOD7OctEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD7, 10) = m_cf.at(CF_TOD7NovEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD7, 11) = m_cf.at(CF_TOD7DecEnergy, 1);

	save_cf( m_cm, m_cf,  CF_revenue_monthly_firstyear_TOD7, 11, "cf_revenue_monthly_firstyear_TOD7");
	save_cf( m_cm, m_cf,  cf_energy_sales_monthly_firstyear_TOD7, 11, "cf_energy_sales_monthly_firstyear_TOD7");


	m_cf.at(CF_revenue_monthly_firstyear_TOD8, 0) = ppa[1] / 100.0 *
		dispatch_factor8 * m_cf.at(CF_TOD8JanEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD8, 1) = ppa[1] / 100.0 *
		dispatch_factor8 * m_cf.at(CF_TOD8FebEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD8, 2) = ppa[1] / 100.0 *
		dispatch_factor8 * m_cf.at(CF_TOD8MarEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD8, 3) = ppa[1] / 100.0 *
		dispatch_factor8 * m_cf.at(CF_TOD8AprEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD8, 4) = ppa[1] / 100.0 *
		dispatch_factor8 * m_cf.at(CF_TOD8MayEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD8, 5) = ppa[1] / 100.0 *
		dispatch_factor8 * m_cf.at(CF_TOD8JunEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD8, 6) = ppa[1] / 100.0 *
		dispatch_factor8 * m_cf.at(CF_TOD8JulEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD8, 7) = ppa[1] / 100.0 *
		dispatch_factor8 * m_cf.at(CF_TOD8AugEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD8, 8) = ppa[1] / 100.0 *
		dispatch_factor8 * m_cf.at(CF_TOD8SepEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD8, 9) = ppa[1] / 100.0 *
		dispatch_factor8 * m_cf.at(CF_TOD8OctEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD8, 10) = ppa[1] / 100.0 *
		dispatch_factor8 * m_cf.at(CF_TOD8NovEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD8, 11) = ppa[1] / 100.0 *
		dispatch_factor8 * m_cf.at(CF_TOD8DecEnergy, 1);

	m_cf.at(cf_energy_sales_monthly_firstyear_TOD8, 0) = m_cf.at(CF_TOD8JanEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD8, 1) = m_cf.at(CF_TOD8FebEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD8, 2) = m_cf.at(CF_TOD8MarEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD8, 3) = m_cf.at(CF_TOD8AprEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD8, 4) = m_cf.at(CF_TOD8MayEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD8, 5) = m_cf.at(CF_TOD8JunEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD8, 6) = m_cf.at(CF_TOD8JulEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD8, 7) = m_cf.at(CF_TOD8AugEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD8, 8) = m_cf.at(CF_TOD8SepEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD8, 9) = m_cf.at(CF_TOD8OctEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD8, 10) = m_cf.at(CF_TOD8NovEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD8, 11) = m_cf.at(CF_TOD8DecEnergy, 1);

	save_cf( m_cm, m_cf,  CF_revenue_monthly_firstyear_TOD8, 11, "cf_revenue_monthly_firstyear_TOD8");
	save_cf( m_cm, m_cf,  cf_energy_sales_monthly_firstyear_TOD8, 11, "cf_energy_sales_monthly_firstyear_TOD8");


	m_cf.at(CF_revenue_monthly_firstyear_TOD9, 0) = ppa[1] / 100.0 *
		dispatch_factor9 * m_cf.at(CF_TOD9JanEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD9, 1) = ppa[1] / 100.0 *
		dispatch_factor9 * m_cf.at(CF_TOD9FebEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD9, 2) = ppa[1] / 100.0 *
		dispatch_factor9 * m_cf.at(CF_TOD9MarEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD9, 3) = ppa[1] / 100.0 *
		dispatch_factor9 * m_cf.at(CF_TOD9AprEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD9, 4) = ppa[1] / 100.0 *
		dispatch_factor9 * m_cf.at(CF_TOD9MayEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD9, 5) = ppa[1] / 100.0 *
		dispatch_factor9 * m_cf.at(CF_TOD9JunEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD9, 6) = ppa[1] / 100.0 *
		dispatch_factor9 * m_cf.at(CF_TOD9JulEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD9, 7) = ppa[1] / 100.0 *
		dispatch_factor9 * m_cf.at(CF_TOD9AugEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD9, 8) = ppa[1] / 100.0 *
		dispatch_factor9 * m_cf.at(CF_TOD9SepEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD9, 9) = ppa[1] / 100.0 *
		dispatch_factor9 * m_cf.at(CF_TOD9OctEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD9, 10) = ppa[1] / 100.0 *
		dispatch_factor9 * m_cf.at(CF_TOD9NovEnergy, 1);
	m_cf.at(CF_revenue_monthly_firstyear_TOD9, 11) = ppa[1] / 100.0 *
		dispatch_factor9 * m_cf.at(CF_TOD9DecEnergy, 1);

	m_cf.at(cf_energy_sales_monthly_firstyear_TOD9, 0) = m_cf.at(CF_TOD9JanEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD9, 1) = m_cf.at(CF_TOD9FebEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD9, 2) = m_cf.at(CF_TOD9MarEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD9, 3) = m_cf.at(CF_TOD9AprEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD9, 4) = m_cf.at(CF_TOD9MayEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD9, 5) = m_cf.at(CF_TOD9JunEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD9, 6) = m_cf.at(CF_TOD9JulEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD9, 7) = m_cf.at(CF_TOD9AugEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD9, 8) = m_cf.at(CF_TOD9SepEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD9, 9) = m_cf.at(CF_TOD9OctEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD9, 10) = m_cf.at(CF_TOD9NovEnergy, 1);
	m_cf.at(cf_energy_sales_monthly_firstyear_TOD9, 11) = m_cf.at(CF_TOD9DecEnergy, 1);

	save_cf( m_cm, m_cf,  CF_revenue_monthly_firstyear_TOD9, 11, "cf_revenue_monthly_firstyear_TOD9");
	save_cf( m_cm, m_cf,  cf_energy_sales_monthly_firstyear_TOD9, 11, "cf_energy_sales_monthly_firstyear_TOD9");


	m_cm->assign("firstyear_revenue_dispatch1", var_data((ssc_number_t)m_cf.at(CF_TOD1Revenue, 1)));
	m_cm->assign("firstyear_revenue_dispatch2", var_data((ssc_number_t)m_cf.at(CF_TOD2Revenue, 1)));
	m_cm->assign("firstyear_revenue_dispatch3", var_data((ssc_number_t)m_cf.at(CF_TOD3Revenue, 1)));
	m_cm->assign("firstyear_revenue_dispatch4", var_data((ssc_number_t)m_cf.at(CF_TOD4Revenue, 1)));
	m_cm->assign("firstyear_revenue_dispatch5", var_data((ssc_number_t)m_cf.at(CF_TOD5Revenue, 1)));
	m_cm->assign("firstyear_revenue_dispatch6", var_data((ssc_number_t)m_cf.at(CF_TOD6Revenue, 1)));
	m_cm->assign("firstyear_revenue_dispatch7", var_data((ssc_number_t)m_cf.at(CF_TOD7Revenue, 1)));
	m_cm->assign("firstyear_revenue_dispatch8", var_data((ssc_number_t)m_cf.at(CF_TOD8Revenue, 1)));
	m_cm->assign("firstyear_revenue_dispatch9", var_data((ssc_number_t)m_cf.at(CF_TOD9Revenue, 1)));

	m_cm->assign("firstyear_energy_dispatch1", var_data((ssc_number_t)m_cf.at(CF_TOD1Energy, 1)));
	m_cm->assign("firstyear_energy_dispatch2", var_data((ssc_number_t)m_cf.at(CF_TOD2Energy, 1)));
	m_cm->assign("firstyear_energy_dispatch3", var_data((ssc_number_t)m_cf.at(CF_TOD3Energy, 1)));
	m_cm->assign("firstyear_energy_dispatch4", var_data((ssc_number_t)m_cf.at(CF_TOD4Energy, 1)));
	m_cm->assign("firstyear_energy_dispatch5", var_data((ssc_number_t)m_cf.at(CF_TOD5Energy, 1)));
	m_cm->assign("firstyear_energy_dispatch6", var_data((ssc_number_t)m_cf.at(CF_TOD6Energy, 1)));
	m_cm->assign("firstyear_energy_dispatch7", var_data((ssc_number_t)m_cf.at(CF_TOD7Energy, 1)));
	m_cm->assign("firstyear_energy_dispatch8", var_data((ssc_number_t)m_cf.at(CF_TOD8Energy, 1)));
	m_cm->assign("firstyear_energy_dispatch9", var_data((ssc_number_t)m_cf.at(CF_TOD9Energy, 1)));
	// revenue in $
	m_cm->assign("firstyear_energy_price1", var_data((ssc_number_t)((m_cf.at(CF_TOD1Energy, 1) == 0) ? 0 : (m_cf.at(CF_TOD1Revenue, 1) / m_cf.at(CF_TOD1Energy, 1)) * 100.0)));
	m_cm->assign("firstyear_energy_price2", var_data((ssc_number_t)((m_cf.at(CF_TOD2Energy, 1) == 0) ? 0 : (m_cf.at(CF_TOD2Revenue, 1) / m_cf.at(CF_TOD2Energy, 1)) * 100.0)));
	m_cm->assign("firstyear_energy_price3", var_data((ssc_number_t)((m_cf.at(CF_TOD3Energy, 1) == 0) ? 0 : (m_cf.at(CF_TOD3Revenue, 1) / m_cf.at(CF_TOD3Energy, 1)) * 100.0)));
	m_cm->assign("firstyear_energy_price4", var_data((ssc_number_t)((m_cf.at(CF_TOD4Energy, 1) == 0) ? 0 : (m_cf.at(CF_TOD4Revenue, 1) / m_cf.at(CF_TOD4Energy, 1)) * 100.0)));
	m_cm->assign("firstyear_energy_price5", var_data((ssc_number_t)((m_cf.at(CF_TOD5Energy, 1) == 0) ? 0 : (m_cf.at(CF_TOD5Revenue, 1) / m_cf.at(CF_TOD5Energy, 1)) * 100.0)));
	m_cm->assign("firstyear_energy_price6", var_data((ssc_number_t)((m_cf.at(CF_TOD6Energy, 1) == 0) ? 0 : (m_cf.at(CF_TOD6Revenue, 1) / m_cf.at(CF_TOD6Energy, 1)) * 100.0)));
	m_cm->assign("firstyear_energy_price7", var_data((ssc_number_t)((m_cf.at(CF_TOD7Energy, 1) == 0) ? 0 : (m_cf.at(CF_TOD7Revenue, 1) / m_cf.at(CF_TOD7Energy, 1)) * 100.0)));
	m_cm->assign("firstyear_energy_price8", var_data((ssc_number_t)((m_cf.at(CF_TOD8Energy, 1) == 0) ? 0 : (m_cf.at(CF_TOD8Revenue, 1) / m_cf.at(CF_TOD8Energy, 1)) * 100.0)));
	m_cm->assign("firstyear_energy_price9", var_data((ssc_number_t)((m_cf.at(CF_TOD9Energy, 1) == 0) ? 0 : (m_cf.at(CF_TOD9Revenue, 1) / m_cf.at(CF_TOD9Energy, 1)) * 100.0)));

	return true;
}

util::matrix_t<double>& dispatch_calculations::dispatch_output()
{
	return m_cf;
}

double dispatch_calculations::tod_energy(int period, int year)
{
	double energy = 0;
	switch (period)
	{
	case 1:
		energy = m_cf.at(CF_TOD1Energy, year);
		break;
	case 2:
		energy = m_cf.at(CF_TOD2Energy, year);
		break;
	case 3:
		energy = m_cf.at(CF_TOD3Energy, year);
		break;
	case 4:
		energy = m_cf.at(CF_TOD4Energy, year);
		break;
	case 5:
		energy = m_cf.at(CF_TOD5Energy, year);
		break;
	case 6:
		energy = m_cf.at(CF_TOD6Energy, year);
		break;
	case 7:
		energy = m_cf.at(CF_TOD7Energy, year);
		break;
	case 8:
		energy = m_cf.at(CF_TOD8Energy, year);
		break;
	case 9:
		energy = m_cf.at(CF_TOD9Energy, year);
		break;
	}
	return energy;
}
//  convenience function for tod periods 1 through 9
double dispatch_calculations::tod_energy_value(int year)
{
	double energy_value = 0;
	if (m_timestep)
	{
		energy_value += m_cf.at(CF_TODJanRevenue, year);
		energy_value += m_cf.at(CF_TODFebRevenue, year);
		energy_value += m_cf.at(CF_TODMarRevenue, year);
		energy_value += m_cf.at(CF_TODAprRevenue, year);
		energy_value += m_cf.at(CF_TODMayRevenue, year);
		energy_value += m_cf.at(CF_TODJunRevenue, year);
		energy_value += m_cf.at(CF_TODJulRevenue, year);
		energy_value += m_cf.at(CF_TODAugRevenue, year);
		energy_value += m_cf.at(CF_TODSepRevenue, year);
		energy_value += m_cf.at(CF_TODOctRevenue, year);
		energy_value += m_cf.at(CF_TODNovRevenue, year);
		energy_value += m_cf.at(CF_TODDecRevenue, year);
	}
	else  // diurnal
	{
		for (int i = 1; i < 10; i++)
			energy_value += tod_energy_value(i, year);
	}
	return energy_value;
}

double dispatch_calculations::tod_energy_value(int period, int year)
{
	double energy_value = 0;


	switch (period)
	{
	case 1:
        energy_value = m_cf.at(CF_TOD1Energy, year)
            * m_dispatch_tod_factors[0];// m_cm->as_double("dispatch_factor1");
		break;
	case 2:
		energy_value = m_cf.at(CF_TOD2Energy, year)
			* m_dispatch_tod_factors[1];//m_cm->as_double("dispatch_factor2");
		break;
	case 3:
		energy_value = m_cf.at(CF_TOD3Energy, year)
			* m_dispatch_tod_factors[2];//m_cm->as_double("dispatch_factor3");
		break;
	case 4:
		energy_value = m_cf.at(CF_TOD4Energy, year)
			* m_dispatch_tod_factors[3];//m_cm->as_double("dispatch_factor4");
		break;
	case 5:
		energy_value = m_cf.at(CF_TOD5Energy, year)
			* m_dispatch_tod_factors[4];//m_cm->as_double("dispatch_factor5");
		break;
	case 6:
		energy_value = m_cf.at(CF_TOD6Energy, year)
			* m_dispatch_tod_factors[5];//m_cm->as_double("dispatch_factor6");
		break;
	case 7:
		energy_value = m_cf.at(CF_TOD7Energy, year)
			* m_dispatch_tod_factors[6];//m_cm->as_double("dispatch_factor7");
		break;
	case 8:
		energy_value = m_cf.at(CF_TOD8Energy, year)
			* m_dispatch_tod_factors[7];//m_cm->as_double("dispatch_factor8");
		break;
	case 9:
		energy_value = m_cf.at(CF_TOD9Energy, year)
			* m_dispatch_tod_factors[8];//m_cm->as_double("dispatch_factor9");
		break;
	}
	return energy_value;
}

bool dispatch_calculations::setup()
{
	// initialize cashflow matrix
	if ((m_nyears + 1) > 12)
		m_cf.resize_fill(CF_max_dispatch, m_nyears + 1, 0.0);
	else // must be able to handle TOD periods and months hard crash in releases 2016.3.14-r1 and before
		m_cf.resize_fill(CF_max_dispatch, 12, 0.0);

	size_t nrows, ncols;
	ssc_number_t *disp_weekday = m_cm->as_matrix("dispatch_sched_weekday", &nrows, &ncols);
	if (nrows != 12 || ncols != 24)
	{
		m_error = util::format("dispatch values weekday schedule must be 12x24, input is %dx%d", (int)nrows, (int)ncols);
		throw exec_error("dispatch_values", m_error);
	}
	ssc_number_t *disp_weekend = m_cm->as_matrix("dispatch_sched_weekend", &nrows, &ncols);
	if (nrows != 12 || ncols != 24)
	{
		m_error = util::format("dispatch values weekend schedule must be 12x24, input is %dx%d", (int)nrows, (int)ncols);
		throw exec_error("dispatch_values", m_error);
	}
	util::matrix_t<double> schedwkday(12, 24);
	schedwkday.assign(disp_weekday, nrows, ncols);
	util::matrix_t<double> schedwkend(12, 24);
	schedwkend.assign(disp_weekend, nrows, ncols);

	int tod[8760];
    int start_day = m_cm->as_number("start_day_of_year");
	if (!util::translate_schedule(tod, schedwkday, schedwkend, 1, 9, start_day))
	{
		m_error = "could not translate weekday and weekend schedules for dispatch values";
		throw general_error(m_error);
	}

    m_dispatch_tod_factors = m_cm->as_vector_double("dispatch_tod_factors");
    if (m_dispatch_tod_factors.size() != 9)
        return false;

    m_periods.resize(8760, 1);
	ssc_number_t *ppa_multipliers = m_cm->allocate("ppa_multipliers", 8760);

	for (int i = 0; i < 8760; i++)
	{
		m_periods[i] = tod[i];

        if ((tod[i] < 1) || (tod[i] > 9)) {
            m_error = util::format("invalid tod period %d for hour %d", tod[i], i);
            throw general_error(m_error);
        }

        ppa_multipliers[i] = m_dispatch_tod_factors[tod[i] - 1];
 	}

	return m_error.length() == 0;
}

bool dispatch_calculations::setup_ts()
{
	// initialize cashflow matrix
	if ((m_nyears + 1) > 12)
		m_cf.resize_fill(CF_max_timestep, m_nyears + 1, 0.0);
	else // must be able to handle TOD periods and months hard crash in releases 2016.3.14-r1 and before
		m_cf.resize_fill(CF_max_timestep, 12, 0.0);

	m_multipliers = m_cm->as_array("dispatch_factors_ts", &m_nmultipliers);
    
    m_gen = m_cm->as_array("revenue_gen", &m_ngen);

	// TODO - handle differences in ngen and nmultipliers - checked in compute_lifetime_dispatch_ts
	// Could interporlate for different number of records like for PV and utility rates
//	if (m_ngen != m_nmultipliers)
//	{
//		m_error = "issue with timestep dispatch multipliers";
//		throw general_error(m_error);
//	}

	ssc_number_t *ppa_multipliers = m_cm->allocate("ppa_multipliers", m_nmultipliers);

	for (size_t i = 0; i < m_nmultipliers; i++)
		ppa_multipliers[i] = m_multipliers[i];

	return m_error.length() == 0;
}


int dispatch_calculations::operator()(size_t time)
{
	if (time < m_periods.size()) return m_periods[time];
	else return 1;
}



bool dispatch_calculations::compute_dispatch_output()
{
	//Calculate energy dispatched in each dispatch period


	int h;
	size_t count = m_hourly_energy.size();

	// hourly energy

	if (count != 8760)
	{
		std::stringstream outm;
		outm << "Bad hourly gen output length (" << count << "), should be 8760 value";
		m_cm->log(outm.str());
		return false;
	}


	m_cf.at(CF_TOD1Energy, 1) = 0;
	m_cf.at(CF_TOD2Energy, 1) = 0;
	m_cf.at(CF_TOD3Energy, 1) = 0;
	m_cf.at(CF_TOD4Energy, 1) = 0;
	m_cf.at(CF_TOD5Energy, 1) = 0;
	m_cf.at(CF_TOD6Energy, 1) = 0;
	m_cf.at(CF_TOD7Energy, 1) = 0;
	m_cf.at(CF_TOD8Energy, 1) = 0;
	m_cf.at(CF_TOD9Energy, 1) = 0;


	// hourly net energy include first year curtailment, availability and degradation
	// unapply first year availability and degradation so that dispatch can be properly calculated and
	// so that availability and degradation is not applied multiple times
	// Better would be to calculate dispatch energy in cmod_annual output; however, dispatch only
	// applies to IPP and DHF markets.



	for (h = 0; h<8760; h++)
	{
		switch (m_periods[h])
		{
		case 1:
			m_cf.at(CF_TOD1Energy, 1) += m_hourly_energy[h];
			break;
		case 2:
			m_cf.at(CF_TOD2Energy, 1) += m_hourly_energy[h];
			break;
		case 3:
			m_cf.at(CF_TOD3Energy, 1) += m_hourly_energy[h];
			break;
		case 4:
			m_cf.at(CF_TOD4Energy, 1) += m_hourly_energy[h];
			break;
		case 5:
			m_cf.at(CF_TOD5Energy, 1) += m_hourly_energy[h];
			break;
		case 6:
			m_cf.at(CF_TOD6Energy, 1) += m_hourly_energy[h];
			break;
		case 7:
			m_cf.at(CF_TOD7Energy, 1) += m_hourly_energy[h];
			break;
		case 8:
			m_cf.at(CF_TOD8Energy, 1) += m_hourly_energy[h];
			break;
		case 9:
			m_cf.at(CF_TOD9Energy, 1) += m_hourly_energy[h];
			break;
		}
	}
	// remove degradation and availability from year 1 values but keep curtailment so that
	// availability and degradation yearly schedules from cmod_annualoutput can be properly applied.
	double year1_TOD1Energy = m_cf.at(CF_TOD1Energy, 1);
	double year1_TOD2Energy = m_cf.at(CF_TOD2Energy, 1);
	double year1_TOD3Energy = m_cf.at(CF_TOD3Energy, 1);
	double year1_TOD4Energy = m_cf.at(CF_TOD4Energy, 1);
	double year1_TOD5Energy = m_cf.at(CF_TOD5Energy, 1);
	double year1_TOD6Energy = m_cf.at(CF_TOD6Energy, 1);
	double year1_TOD7Energy = m_cf.at(CF_TOD7Energy, 1);
	double year1_TOD8Energy = m_cf.at(CF_TOD8Energy, 1);
	double year1_TOD9Energy = m_cf.at(CF_TOD9Energy, 1);


	for (int y = 0; y <= m_nyears; y++)
	{
		// compute energy dispatched
		m_cf.at(CF_TOD1Energy, y) = year1_TOD1Energy * m_degradation[y];
		m_cf.at(CF_TOD2Energy, y) = year1_TOD2Energy * m_degradation[y];
		m_cf.at(CF_TOD3Energy, y) = year1_TOD3Energy * m_degradation[y];
		m_cf.at(CF_TOD4Energy, y) = year1_TOD4Energy * m_degradation[y];
		m_cf.at(CF_TOD5Energy, y) = year1_TOD5Energy * m_degradation[y];
		m_cf.at(CF_TOD6Energy, y) = year1_TOD6Energy * m_degradation[y];
		m_cf.at(CF_TOD7Energy, y) = year1_TOD7Energy * m_degradation[y];
		m_cf.at(CF_TOD8Energy, y) = year1_TOD8Energy * m_degradation[y];
		m_cf.at(CF_TOD9Energy, y) = year1_TOD9Energy * m_degradation[y];
	}


	return true;
}

bool dispatch_calculations::process_dispatch_output()
{
	//Calculate energy dispatched in each dispatch period

	size_t count=m_hourly_energy.size();

	// hourly energy
	if (count != 8760)
	{
		std::stringstream outm;
		outm << "Bad hourly gen output length (" << count << "), should be 8760 value";
		m_cm->log(outm.str());
		return false;
	}



	m_cf.at(CF_TODJanEnergy, 1) = 0;
	m_cf.at(CF_TODFebEnergy, 1) = 0;
	m_cf.at(CF_TODMarEnergy, 1) = 0;
	m_cf.at(CF_TODAprEnergy, 1) = 0;
	m_cf.at(CF_TODMayEnergy, 1) = 0;
	m_cf.at(CF_TODJunEnergy, 1) = 0;
	m_cf.at(CF_TODJulEnergy, 1) = 0;
	m_cf.at(CF_TODAugEnergy, 1) = 0;
	m_cf.at(CF_TODSepEnergy, 1) = 0;
	m_cf.at(CF_TODOctEnergy, 1) = 0;
	m_cf.at(CF_TODNovEnergy, 1) = 0;
	m_cf.at(CF_TODDecEnergy, 1) = 0;

	m_cf.at(CF_TOD1JanEnergy, 1) = 0;
	m_cf.at(CF_TOD1FebEnergy, 1) = 0;
	m_cf.at(CF_TOD1MarEnergy, 1) = 0;
	m_cf.at(CF_TOD1AprEnergy, 1) = 0;
	m_cf.at(CF_TOD1MayEnergy, 1) = 0;
	m_cf.at(CF_TOD1JunEnergy, 1) = 0;
	m_cf.at(CF_TOD1JulEnergy, 1) = 0;
	m_cf.at(CF_TOD1AugEnergy, 1) = 0;
	m_cf.at(CF_TOD1SepEnergy, 1) = 0;
	m_cf.at(CF_TOD1OctEnergy, 1) = 0;
	m_cf.at(CF_TOD1NovEnergy, 1) = 0;
	m_cf.at(CF_TOD1DecEnergy, 1) = 0;

	m_cf.at(CF_TOD2JanEnergy, 1) = 0;
	m_cf.at(CF_TOD2FebEnergy, 1) = 0;
	m_cf.at(CF_TOD2MarEnergy, 1) = 0;
	m_cf.at(CF_TOD2AprEnergy, 1) = 0;
	m_cf.at(CF_TOD2MayEnergy, 1) = 0;
	m_cf.at(CF_TOD2JunEnergy, 1) = 0;
	m_cf.at(CF_TOD2JulEnergy, 1) = 0;
	m_cf.at(CF_TOD2AugEnergy, 1) = 0;
	m_cf.at(CF_TOD2SepEnergy, 1) = 0;
	m_cf.at(CF_TOD2OctEnergy, 1) = 0;
	m_cf.at(CF_TOD2NovEnergy, 1) = 0;
	m_cf.at(CF_TOD2DecEnergy, 1) = 0;

	m_cf.at(CF_TOD3JanEnergy, 1) = 0;
	m_cf.at(CF_TOD3FebEnergy, 1) = 0;
	m_cf.at(CF_TOD3MarEnergy, 1) = 0;
	m_cf.at(CF_TOD3AprEnergy, 1) = 0;
	m_cf.at(CF_TOD3MayEnergy, 1) = 0;
	m_cf.at(CF_TOD3JunEnergy, 1) = 0;
	m_cf.at(CF_TOD3JulEnergy, 1) = 0;
	m_cf.at(CF_TOD3AugEnergy, 1) = 0;
	m_cf.at(CF_TOD3SepEnergy, 1) = 0;
	m_cf.at(CF_TOD3OctEnergy, 1) = 0;
	m_cf.at(CF_TOD3NovEnergy, 1) = 0;
	m_cf.at(CF_TOD3DecEnergy, 1) = 0;

	m_cf.at(CF_TOD4JanEnergy, 1) = 0;
	m_cf.at(CF_TOD4FebEnergy, 1) = 0;
	m_cf.at(CF_TOD4MarEnergy, 1) = 0;
	m_cf.at(CF_TOD4AprEnergy, 1) = 0;
	m_cf.at(CF_TOD4MayEnergy, 1) = 0;
	m_cf.at(CF_TOD4JunEnergy, 1) = 0;
	m_cf.at(CF_TOD4JulEnergy, 1) = 0;
	m_cf.at(CF_TOD4AugEnergy, 1) = 0;
	m_cf.at(CF_TOD4SepEnergy, 1) = 0;
	m_cf.at(CF_TOD4OctEnergy, 1) = 0;
	m_cf.at(CF_TOD4NovEnergy, 1) = 0;
	m_cf.at(CF_TOD4DecEnergy, 1) = 0;

	m_cf.at(CF_TOD5JanEnergy, 1) = 0;
	m_cf.at(CF_TOD5FebEnergy, 1) = 0;
	m_cf.at(CF_TOD5MarEnergy, 1) = 0;
	m_cf.at(CF_TOD5AprEnergy, 1) = 0;
	m_cf.at(CF_TOD5MayEnergy, 1) = 0;
	m_cf.at(CF_TOD5JunEnergy, 1) = 0;
	m_cf.at(CF_TOD5JulEnergy, 1) = 0;
	m_cf.at(CF_TOD5AugEnergy, 1) = 0;
	m_cf.at(CF_TOD5SepEnergy, 1) = 0;
	m_cf.at(CF_TOD5OctEnergy, 1) = 0;
	m_cf.at(CF_TOD5NovEnergy, 1) = 0;
	m_cf.at(CF_TOD5DecEnergy, 1) = 0;

	m_cf.at(CF_TOD6JanEnergy, 1) = 0;
	m_cf.at(CF_TOD6FebEnergy, 1) = 0;
	m_cf.at(CF_TOD6MarEnergy, 1) = 0;
	m_cf.at(CF_TOD6AprEnergy, 1) = 0;
	m_cf.at(CF_TOD6MayEnergy, 1) = 0;
	m_cf.at(CF_TOD6JunEnergy, 1) = 0;
	m_cf.at(CF_TOD6JulEnergy, 1) = 0;
	m_cf.at(CF_TOD6AugEnergy, 1) = 0;
	m_cf.at(CF_TOD6SepEnergy, 1) = 0;
	m_cf.at(CF_TOD6OctEnergy, 1) = 0;
	m_cf.at(CF_TOD6NovEnergy, 1) = 0;
	m_cf.at(CF_TOD6DecEnergy, 1) = 0;

	m_cf.at(CF_TOD7JanEnergy, 1) = 0;
	m_cf.at(CF_TOD7FebEnergy, 1) = 0;
	m_cf.at(CF_TOD7MarEnergy, 1) = 0;
	m_cf.at(CF_TOD7AprEnergy, 1) = 0;
	m_cf.at(CF_TOD7MayEnergy, 1) = 0;
	m_cf.at(CF_TOD7JunEnergy, 1) = 0;
	m_cf.at(CF_TOD7JulEnergy, 1) = 0;
	m_cf.at(CF_TOD7AugEnergy, 1) = 0;
	m_cf.at(CF_TOD7SepEnergy, 1) = 0;
	m_cf.at(CF_TOD7OctEnergy, 1) = 0;
	m_cf.at(CF_TOD7NovEnergy, 1) = 0;
	m_cf.at(CF_TOD7DecEnergy, 1) = 0;

	m_cf.at(CF_TOD8JanEnergy, 1) = 0;
	m_cf.at(CF_TOD8FebEnergy, 1) = 0;
	m_cf.at(CF_TOD8MarEnergy, 1) = 0;
	m_cf.at(CF_TOD8AprEnergy, 1) = 0;
	m_cf.at(CF_TOD8MayEnergy, 1) = 0;
	m_cf.at(CF_TOD8JunEnergy, 1) = 0;
	m_cf.at(CF_TOD8JulEnergy, 1) = 0;
	m_cf.at(CF_TOD8AugEnergy, 1) = 0;
	m_cf.at(CF_TOD8SepEnergy, 1) = 0;
	m_cf.at(CF_TOD8OctEnergy, 1) = 0;
	m_cf.at(CF_TOD8NovEnergy, 1) = 0;
	m_cf.at(CF_TOD8DecEnergy, 1) = 0;

	m_cf.at(CF_TOD9JanEnergy, 1) = 0;
	m_cf.at(CF_TOD9FebEnergy, 1) = 0;
	m_cf.at(CF_TOD9MarEnergy, 1) = 0;
	m_cf.at(CF_TOD9AprEnergy, 1) = 0;
	m_cf.at(CF_TOD9MayEnergy, 1) = 0;
	m_cf.at(CF_TOD9JunEnergy, 1) = 0;
	m_cf.at(CF_TOD9JulEnergy, 1) = 0;
	m_cf.at(CF_TOD9AugEnergy, 1) = 0;
	m_cf.at(CF_TOD9SepEnergy, 1) = 0;
	m_cf.at(CF_TOD9OctEnergy, 1) = 0;
	m_cf.at(CF_TOD9NovEnergy, 1) = 0;
	m_cf.at(CF_TOD9DecEnergy, 1) = 0;

	int i = 0;
	for (int m = 0; m<12; m++)
	{
		for (size_t d = 0; d<util::nday[m]; d++)
		{
			for (int h = 0; h<24 && i<8760 && m * 24 + h<288; h++)
			{
				switch (m)
				{
				case 0:
					m_cf.at(CF_TODJanEnergy, 1) += m_hourly_energy[i];
					switch (m_periods[i])
					{
					case 1:
						m_cf.at(CF_TOD1JanEnergy, 1) += m_hourly_energy[i];
						break;
					case 2:
						m_cf.at(CF_TOD2JanEnergy, 1) += m_hourly_energy[i];
						break;
					case 3:
						m_cf.at(CF_TOD3JanEnergy, 1) += m_hourly_energy[i];
						break;
					case 4:
						m_cf.at(CF_TOD4JanEnergy, 1) += m_hourly_energy[i];
						break;
					case 5:
						m_cf.at(CF_TOD5JanEnergy, 1) += m_hourly_energy[i];
						break;
					case 6:
						m_cf.at(CF_TOD6JanEnergy, 1) += m_hourly_energy[i];
						break;
					case 7:
						m_cf.at(CF_TOD7JanEnergy, 1) += m_hourly_energy[i];
						break;
					case 8:
						m_cf.at(CF_TOD8JanEnergy, 1) += m_hourly_energy[i];
						break;
					case 9:
						m_cf.at(CF_TOD9JanEnergy, 1) += m_hourly_energy[i];
						break;
					}
					break;
				case 1:
					m_cf.at(CF_TODFebEnergy, 1) += m_hourly_energy[i];
					switch (m_periods[i])
					{
					case 1:
						m_cf.at(CF_TOD1FebEnergy, 1) += m_hourly_energy[i];
						break;
					case 2:
						m_cf.at(CF_TOD2FebEnergy, 1) += m_hourly_energy[i];
						break;
					case 3:
						m_cf.at(CF_TOD3FebEnergy, 1) += m_hourly_energy[i];
						break;
					case 4:
						m_cf.at(CF_TOD4FebEnergy, 1) += m_hourly_energy[i];
						break;
					case 5:
						m_cf.at(CF_TOD5FebEnergy, 1) += m_hourly_energy[i];
						break;
					case 6:
						m_cf.at(CF_TOD6FebEnergy, 1) += m_hourly_energy[i];
						break;
					case 7:
						m_cf.at(CF_TOD7FebEnergy, 1) += m_hourly_energy[i];
						break;
					case 8:
						m_cf.at(CF_TOD8FebEnergy, 1) += m_hourly_energy[i];
						break;
					case 9:
						m_cf.at(CF_TOD9FebEnergy, 1) += m_hourly_energy[i];
						break;
					}
					break;
				case 2:
					m_cf.at(CF_TODMarEnergy, 1) += m_hourly_energy[i];
					switch (m_periods[i])
					{
					case 1:
						m_cf.at(CF_TOD1MarEnergy, 1) += m_hourly_energy[i];
						break;
					case 2:
						m_cf.at(CF_TOD2MarEnergy, 1) += m_hourly_energy[i];
						break;
					case 3:
						m_cf.at(CF_TOD3MarEnergy, 1) += m_hourly_energy[i];
						break;
					case 4:
						m_cf.at(CF_TOD4MarEnergy, 1) += m_hourly_energy[i];
						break;
					case 5:
						m_cf.at(CF_TOD5MarEnergy, 1) += m_hourly_energy[i];
						break;
					case 6:
						m_cf.at(CF_TOD6MarEnergy, 1) += m_hourly_energy[i];
						break;
					case 7:
						m_cf.at(CF_TOD7MarEnergy, 1) += m_hourly_energy[i];
						break;
					case 8:
						m_cf.at(CF_TOD8MarEnergy, 1) += m_hourly_energy[i];
						break;
					case 9:
						m_cf.at(CF_TOD9MarEnergy, 1) += m_hourly_energy[i];
						break;
					}
					break;
				case 3:
					m_cf.at(CF_TODAprEnergy, 1) += m_hourly_energy[i];
					switch (m_periods[i])
					{
					case 1:
						m_cf.at(CF_TOD1AprEnergy, 1) += m_hourly_energy[i];
						break;
					case 2:
						m_cf.at(CF_TOD2AprEnergy, 1) += m_hourly_energy[i];
						break;
					case 3:
						m_cf.at(CF_TOD3AprEnergy, 1) += m_hourly_energy[i];
						break;
					case 4:
						m_cf.at(CF_TOD4AprEnergy, 1) += m_hourly_energy[i];
						break;
					case 5:
						m_cf.at(CF_TOD5AprEnergy, 1) += m_hourly_energy[i];
						break;
					case 6:
						m_cf.at(CF_TOD6AprEnergy, 1) += m_hourly_energy[i];
						break;
					case 7:
						m_cf.at(CF_TOD7AprEnergy, 1) += m_hourly_energy[i];
						break;
					case 8:
						m_cf.at(CF_TOD8AprEnergy, 1) += m_hourly_energy[i];
						break;
					case 9:
						m_cf.at(CF_TOD9AprEnergy, 1) += m_hourly_energy[i];
						break;
					}
					break;
				case 4:
					m_cf.at(CF_TODMayEnergy, 1) += m_hourly_energy[i];
					switch (m_periods[i])
					{
					case 1:
						m_cf.at(CF_TOD1MayEnergy, 1) += m_hourly_energy[i];
						break;
					case 2:
						m_cf.at(CF_TOD2MayEnergy, 1) += m_hourly_energy[i];
						break;
					case 3:
						m_cf.at(CF_TOD3MayEnergy, 1) += m_hourly_energy[i];
						break;
					case 4:
						m_cf.at(CF_TOD4MayEnergy, 1) += m_hourly_energy[i];
						break;
					case 5:
						m_cf.at(CF_TOD5MayEnergy, 1) += m_hourly_energy[i];
						break;
					case 6:
						m_cf.at(CF_TOD6MayEnergy, 1) += m_hourly_energy[i];
						break;
					case 7:
						m_cf.at(CF_TOD7MayEnergy, 1) += m_hourly_energy[i];
						break;
					case 8:
						m_cf.at(CF_TOD8MayEnergy, 1) += m_hourly_energy[i];
						break;
					case 9:
						m_cf.at(CF_TOD9MayEnergy, 1) += m_hourly_energy[i];
						break;
					}
					break;
				case 5:
					m_cf.at(CF_TODJunEnergy, 1) += m_hourly_energy[i];
					switch (m_periods[i])
					{
					case 1:
						m_cf.at(CF_TOD1JunEnergy, 1) += m_hourly_energy[i];
						break;
					case 2:
						m_cf.at(CF_TOD2JunEnergy, 1) += m_hourly_energy[i];
						break;
					case 3:
						m_cf.at(CF_TOD3JunEnergy, 1) += m_hourly_energy[i];
						break;
					case 4:
						m_cf.at(CF_TOD4JunEnergy, 1) += m_hourly_energy[i];
						break;
					case 5:
						m_cf.at(CF_TOD5JunEnergy, 1) += m_hourly_energy[i];
						break;
					case 6:
						m_cf.at(CF_TOD6JunEnergy, 1) += m_hourly_energy[i];
						break;
					case 7:
						m_cf.at(CF_TOD7JunEnergy, 1) += m_hourly_energy[i];
						break;
					case 8:
						m_cf.at(CF_TOD8JunEnergy, 1) += m_hourly_energy[i];
						break;
					case 9:
						m_cf.at(CF_TOD9JunEnergy, 1) += m_hourly_energy[i];
						break;
					}
					break;
				case 6:
					m_cf.at(CF_TODJulEnergy, 1) += m_hourly_energy[i];
					switch (m_periods[i])
					{
					case 1:
						m_cf.at(CF_TOD1JulEnergy, 1) += m_hourly_energy[i];
						break;
					case 2:
						m_cf.at(CF_TOD2JulEnergy, 1) += m_hourly_energy[i];
						break;
					case 3:
						m_cf.at(CF_TOD3JulEnergy, 1) += m_hourly_energy[i];
						break;
					case 4:
						m_cf.at(CF_TOD4JulEnergy, 1) += m_hourly_energy[i];
						break;
					case 5:
						m_cf.at(CF_TOD5JulEnergy, 1) += m_hourly_energy[i];
						break;
					case 6:
						m_cf.at(CF_TOD6JulEnergy, 1) += m_hourly_energy[i];
						break;
					case 7:
						m_cf.at(CF_TOD7JulEnergy, 1) += m_hourly_energy[i];
						break;
					case 8:
						m_cf.at(CF_TOD8JulEnergy, 1) += m_hourly_energy[i];
						break;
					case 9:
						m_cf.at(CF_TOD9JulEnergy, 1) += m_hourly_energy[i];
						break;
					}
					break;
				case 7:
					m_cf.at(CF_TODAugEnergy, 1) += m_hourly_energy[i];
					switch (m_periods[i])
					{
					case 1:
						m_cf.at(CF_TOD1AugEnergy, 1) += m_hourly_energy[i];
						break;
					case 2:
						m_cf.at(CF_TOD2AugEnergy, 1) += m_hourly_energy[i];
						break;
					case 3:
						m_cf.at(CF_TOD3AugEnergy, 1) += m_hourly_energy[i];
						break;
					case 4:
						m_cf.at(CF_TOD4AugEnergy, 1) += m_hourly_energy[i];
						break;
					case 5:
						m_cf.at(CF_TOD5AugEnergy, 1) += m_hourly_energy[i];
						break;
					case 6:
						m_cf.at(CF_TOD6AugEnergy, 1) += m_hourly_energy[i];
						break;
					case 7:
						m_cf.at(CF_TOD7AugEnergy, 1) += m_hourly_energy[i];
						break;
					case 8:
						m_cf.at(CF_TOD8AugEnergy, 1) += m_hourly_energy[i];
						break;
					case 9:
						m_cf.at(CF_TOD9AugEnergy, 1) += m_hourly_energy[i];
						break;
					}
					break;
				case 8:
					m_cf.at(CF_TODSepEnergy, 1) += m_hourly_energy[i];
					switch (m_periods[i])
					{
					case 1:
						m_cf.at(CF_TOD1SepEnergy, 1) += m_hourly_energy[i];
						break;
					case 2:
						m_cf.at(CF_TOD2SepEnergy, 1) += m_hourly_energy[i];
						break;
					case 3:
						m_cf.at(CF_TOD3SepEnergy, 1) += m_hourly_energy[i];
						break;
					case 4:
						m_cf.at(CF_TOD4SepEnergy, 1) += m_hourly_energy[i];
						break;
					case 5:
						m_cf.at(CF_TOD5SepEnergy, 1) += m_hourly_energy[i];
						break;
					case 6:
						m_cf.at(CF_TOD6SepEnergy, 1) += m_hourly_energy[i];
						break;
					case 7:
						m_cf.at(CF_TOD7SepEnergy, 1) += m_hourly_energy[i];
						break;
					case 8:
						m_cf.at(CF_TOD8SepEnergy, 1) += m_hourly_energy[i];
						break;
					case 9:
						m_cf.at(CF_TOD9SepEnergy, 1) += m_hourly_energy[i];
						break;
					}
					break;
				case 9:
					m_cf.at(CF_TODOctEnergy, 1) += m_hourly_energy[i];
					switch (m_periods[i])
					{
					case 1:
						m_cf.at(CF_TOD1OctEnergy, 1) += m_hourly_energy[i];
						break;
					case 2:
						m_cf.at(CF_TOD2OctEnergy, 1) += m_hourly_energy[i];
						break;
					case 3:
						m_cf.at(CF_TOD3OctEnergy, 1) += m_hourly_energy[i];
						break;
					case 4:
						m_cf.at(CF_TOD4OctEnergy, 1) += m_hourly_energy[i];
						break;
					case 5:
						m_cf.at(CF_TOD5OctEnergy, 1) += m_hourly_energy[i];
						break;
					case 6:
						m_cf.at(CF_TOD6OctEnergy, 1) += m_hourly_energy[i];
						break;
					case 7:
						m_cf.at(CF_TOD7OctEnergy, 1) += m_hourly_energy[i];
						break;
					case 8:
						m_cf.at(CF_TOD8OctEnergy, 1) += m_hourly_energy[i];
						break;
					case 9:
						m_cf.at(CF_TOD9OctEnergy, 1) += m_hourly_energy[i];
						break;
					}
					break;
				case 10:
					m_cf.at(CF_TODNovEnergy, 1) += m_hourly_energy[i];
					switch (m_periods[i])
					{
					case 1:
						m_cf.at(CF_TOD1NovEnergy, 1) += m_hourly_energy[i];
						break;
					case 2:
						m_cf.at(CF_TOD2NovEnergy, 1) += m_hourly_energy[i];
						break;
					case 3:
						m_cf.at(CF_TOD3NovEnergy, 1) += m_hourly_energy[i];
						break;
					case 4:
						m_cf.at(CF_TOD4NovEnergy, 1) += m_hourly_energy[i];
						break;
					case 5:
						m_cf.at(CF_TOD5NovEnergy, 1) += m_hourly_energy[i];
						break;
					case 6:
						m_cf.at(CF_TOD6NovEnergy, 1) += m_hourly_energy[i];
						break;
					case 7:
						m_cf.at(CF_TOD7NovEnergy, 1) += m_hourly_energy[i];
						break;
					case 8:
						m_cf.at(CF_TOD8NovEnergy, 1) += m_hourly_energy[i];
						break;
					case 9:
						m_cf.at(CF_TOD9NovEnergy, 1) += m_hourly_energy[i];
						break;
					}
					break;
				case 11:
					m_cf.at(CF_TODDecEnergy, 1) += m_hourly_energy[i];
					switch (m_periods[i])
					{
					case 1:
						m_cf.at(CF_TOD1DecEnergy, 1) += m_hourly_energy[i];
						break;
					case 2:
						m_cf.at(CF_TOD2DecEnergy, 1) += m_hourly_energy[i];
						break;
					case 3:
						m_cf.at(CF_TOD3DecEnergy, 1) += m_hourly_energy[i];
						break;
					case 4:
						m_cf.at(CF_TOD4DecEnergy, 1) += m_hourly_energy[i];
						break;
					case 5:
						m_cf.at(CF_TOD5DecEnergy, 1) += m_hourly_energy[i];
						break;
					case 6:
						m_cf.at(CF_TOD6DecEnergy, 1) += m_hourly_energy[i];
						break;
					case 7:
						m_cf.at(CF_TOD7DecEnergy, 1) += m_hourly_energy[i];
						break;
					case 8:
						m_cf.at(CF_TOD8DecEnergy, 1) += m_hourly_energy[i];
						break;
					case 9:
						m_cf.at(CF_TOD9DecEnergy, 1) += m_hourly_energy[i];
						break;
					}
					break;
				}
				i++;
			}
		}
	}


	double year1_TODJanEnergy = m_cf.at(CF_TODJanEnergy, 1);
	double year1_TODFebEnergy = m_cf.at(CF_TODFebEnergy, 1);
	double year1_TODMarEnergy = m_cf.at(CF_TODMarEnergy, 1);
	double year1_TODAprEnergy = m_cf.at(CF_TODAprEnergy, 1);
	double year1_TODMayEnergy = m_cf.at(CF_TODMayEnergy, 1);
	double year1_TODJunEnergy = m_cf.at(CF_TODJunEnergy, 1);
	double year1_TODJulEnergy = m_cf.at(CF_TODJulEnergy, 1);
	double year1_TODAugEnergy = m_cf.at(CF_TODAugEnergy, 1);
	double year1_TODSepEnergy = m_cf.at(CF_TODSepEnergy, 1);
	double year1_TODOctEnergy = m_cf.at(CF_TODOctEnergy, 1);
	double year1_TODNovEnergy = m_cf.at(CF_TODNovEnergy, 1);
	double year1_TODDecEnergy = m_cf.at(CF_TODDecEnergy, 1);

	double year1_TOD1JanEnergy = m_cf.at(CF_TOD1JanEnergy, 1);
	double year1_TOD1FebEnergy = m_cf.at(CF_TOD1FebEnergy, 1);
	double year1_TOD1MarEnergy = m_cf.at(CF_TOD1MarEnergy, 1);
	double year1_TOD1AprEnergy = m_cf.at(CF_TOD1AprEnergy, 1);
	double year1_TOD1MayEnergy = m_cf.at(CF_TOD1MayEnergy, 1);
	double year1_TOD1JunEnergy = m_cf.at(CF_TOD1JunEnergy, 1);
	double year1_TOD1JulEnergy = m_cf.at(CF_TOD1JulEnergy, 1);
	double year1_TOD1AugEnergy = m_cf.at(CF_TOD1AugEnergy, 1);
	double year1_TOD1SepEnergy = m_cf.at(CF_TOD1SepEnergy, 1);
	double year1_TOD1OctEnergy = m_cf.at(CF_TOD1OctEnergy, 1);
	double year1_TOD1NovEnergy = m_cf.at(CF_TOD1NovEnergy, 1);
	double year1_TOD1DecEnergy = m_cf.at(CF_TOD1DecEnergy, 1);

	double year1_TOD2JanEnergy = m_cf.at(CF_TOD2JanEnergy, 1);
	double year1_TOD2FebEnergy = m_cf.at(CF_TOD2FebEnergy, 1);
	double year1_TOD2MarEnergy = m_cf.at(CF_TOD2MarEnergy, 1);
	double year1_TOD2AprEnergy = m_cf.at(CF_TOD2AprEnergy, 1);
	double year1_TOD2MayEnergy = m_cf.at(CF_TOD2MayEnergy, 1);
	double year1_TOD2JunEnergy = m_cf.at(CF_TOD2JunEnergy, 1);
	double year1_TOD2JulEnergy = m_cf.at(CF_TOD2JulEnergy, 1);
	double year1_TOD2AugEnergy = m_cf.at(CF_TOD2AugEnergy, 1);
	double year1_TOD2SepEnergy = m_cf.at(CF_TOD2SepEnergy, 1);
	double year1_TOD2OctEnergy = m_cf.at(CF_TOD2OctEnergy, 1);
	double year1_TOD2NovEnergy = m_cf.at(CF_TOD2NovEnergy, 1);
	double year1_TOD2DecEnergy = m_cf.at(CF_TOD2DecEnergy, 1);

	double year1_TOD3JanEnergy = m_cf.at(CF_TOD3JanEnergy, 1);
	double year1_TOD3FebEnergy = m_cf.at(CF_TOD3FebEnergy, 1);
	double year1_TOD3MarEnergy = m_cf.at(CF_TOD3MarEnergy, 1);
	double year1_TOD3AprEnergy = m_cf.at(CF_TOD3AprEnergy, 1);
	double year1_TOD3MayEnergy = m_cf.at(CF_TOD3MayEnergy, 1);
	double year1_TOD3JunEnergy = m_cf.at(CF_TOD3JunEnergy, 1);
	double year1_TOD3JulEnergy = m_cf.at(CF_TOD3JulEnergy, 1);
	double year1_TOD3AugEnergy = m_cf.at(CF_TOD3AugEnergy, 1);
	double year1_TOD3SepEnergy = m_cf.at(CF_TOD3SepEnergy, 1);
	double year1_TOD3OctEnergy = m_cf.at(CF_TOD3OctEnergy, 1);
	double year1_TOD3NovEnergy = m_cf.at(CF_TOD3NovEnergy, 1);
	double year1_TOD3DecEnergy = m_cf.at(CF_TOD3DecEnergy, 1);

	double year1_TOD4JanEnergy = m_cf.at(CF_TOD4JanEnergy, 1);
	double year1_TOD4FebEnergy = m_cf.at(CF_TOD4FebEnergy, 1);
	double year1_TOD4MarEnergy = m_cf.at(CF_TOD4MarEnergy, 1);
	double year1_TOD4AprEnergy = m_cf.at(CF_TOD4AprEnergy, 1);
	double year1_TOD4MayEnergy = m_cf.at(CF_TOD4MayEnergy, 1);
	double year1_TOD4JunEnergy = m_cf.at(CF_TOD4JunEnergy, 1);
	double year1_TOD4JulEnergy = m_cf.at(CF_TOD4JulEnergy, 1);
	double year1_TOD4AugEnergy = m_cf.at(CF_TOD4AugEnergy, 1);
	double year1_TOD4SepEnergy = m_cf.at(CF_TOD4SepEnergy, 1);
	double year1_TOD4OctEnergy = m_cf.at(CF_TOD4OctEnergy, 1);
	double year1_TOD4NovEnergy = m_cf.at(CF_TOD4NovEnergy, 1);
	double year1_TOD4DecEnergy = m_cf.at(CF_TOD4DecEnergy, 1);

	double year1_TOD5JanEnergy = m_cf.at(CF_TOD5JanEnergy, 1);
	double year1_TOD5FebEnergy = m_cf.at(CF_TOD5FebEnergy, 1);
	double year1_TOD5MarEnergy = m_cf.at(CF_TOD5MarEnergy, 1);
	double year1_TOD5AprEnergy = m_cf.at(CF_TOD5AprEnergy, 1);
	double year1_TOD5MayEnergy = m_cf.at(CF_TOD5MayEnergy, 1);
	double year1_TOD5JunEnergy = m_cf.at(CF_TOD5JunEnergy, 1);
	double year1_TOD5JulEnergy = m_cf.at(CF_TOD5JulEnergy, 1);
	double year1_TOD5AugEnergy = m_cf.at(CF_TOD5AugEnergy, 1);
	double year1_TOD5SepEnergy = m_cf.at(CF_TOD5SepEnergy, 1);
	double year1_TOD5OctEnergy = m_cf.at(CF_TOD5OctEnergy, 1);
	double year1_TOD5NovEnergy = m_cf.at(CF_TOD5NovEnergy, 1);
	double year1_TOD5DecEnergy = m_cf.at(CF_TOD5DecEnergy, 1);

	double year1_TOD6JanEnergy = m_cf.at(CF_TOD6JanEnergy, 1);
	double year1_TOD6FebEnergy = m_cf.at(CF_TOD6FebEnergy, 1);
	double year1_TOD6MarEnergy = m_cf.at(CF_TOD6MarEnergy, 1);
	double year1_TOD6AprEnergy = m_cf.at(CF_TOD6AprEnergy, 1);
	double year1_TOD6MayEnergy = m_cf.at(CF_TOD6MayEnergy, 1);
	double year1_TOD6JunEnergy = m_cf.at(CF_TOD6JunEnergy, 1);
	double year1_TOD6JulEnergy = m_cf.at(CF_TOD6JulEnergy, 1);
	double year1_TOD6AugEnergy = m_cf.at(CF_TOD6AugEnergy, 1);
	double year1_TOD6SepEnergy = m_cf.at(CF_TOD6SepEnergy, 1);
	double year1_TOD6OctEnergy = m_cf.at(CF_TOD6OctEnergy, 1);
	double year1_TOD6NovEnergy = m_cf.at(CF_TOD6NovEnergy, 1);
	double year1_TOD6DecEnergy = m_cf.at(CF_TOD6DecEnergy, 1);

	double year1_TOD7JanEnergy = m_cf.at(CF_TOD7JanEnergy, 1);
	double year1_TOD7FebEnergy = m_cf.at(CF_TOD7FebEnergy, 1);
	double year1_TOD7MarEnergy = m_cf.at(CF_TOD7MarEnergy, 1);
	double year1_TOD7AprEnergy = m_cf.at(CF_TOD7AprEnergy, 1);
	double year1_TOD7MayEnergy = m_cf.at(CF_TOD7MayEnergy, 1);
	double year1_TOD7JunEnergy = m_cf.at(CF_TOD7JunEnergy, 1);
	double year1_TOD7JulEnergy = m_cf.at(CF_TOD7JulEnergy, 1);
	double year1_TOD7AugEnergy = m_cf.at(CF_TOD7AugEnergy, 1);
	double year1_TOD7SepEnergy = m_cf.at(CF_TOD7SepEnergy, 1);
	double year1_TOD7OctEnergy = m_cf.at(CF_TOD7OctEnergy, 1);
	double year1_TOD7NovEnergy = m_cf.at(CF_TOD7NovEnergy, 1);
	double year1_TOD7DecEnergy = m_cf.at(CF_TOD7DecEnergy, 1);

	double year1_TOD8JanEnergy = m_cf.at(CF_TOD8JanEnergy, 1);
	double year1_TOD8FebEnergy = m_cf.at(CF_TOD8FebEnergy, 1);
	double year1_TOD8MarEnergy = m_cf.at(CF_TOD8MarEnergy, 1);
	double year1_TOD8AprEnergy = m_cf.at(CF_TOD8AprEnergy, 1);
	double year1_TOD8MayEnergy = m_cf.at(CF_TOD8MayEnergy, 1);
	double year1_TOD8JunEnergy = m_cf.at(CF_TOD8JunEnergy, 1);
	double year1_TOD8JulEnergy = m_cf.at(CF_TOD8JulEnergy, 1);
	double year1_TOD8AugEnergy = m_cf.at(CF_TOD8AugEnergy, 1);
	double year1_TOD8SepEnergy = m_cf.at(CF_TOD8SepEnergy, 1);
	double year1_TOD8OctEnergy = m_cf.at(CF_TOD8OctEnergy, 1);
	double year1_TOD8NovEnergy = m_cf.at(CF_TOD8NovEnergy, 1);
	double year1_TOD8DecEnergy = m_cf.at(CF_TOD8DecEnergy, 1);

	double year1_TOD9JanEnergy = m_cf.at(CF_TOD9JanEnergy, 1);
	double year1_TOD9FebEnergy = m_cf.at(CF_TOD9FebEnergy, 1);
	double year1_TOD9MarEnergy = m_cf.at(CF_TOD9MarEnergy, 1);
	double year1_TOD9AprEnergy = m_cf.at(CF_TOD9AprEnergy, 1);
	double year1_TOD9MayEnergy = m_cf.at(CF_TOD9MayEnergy, 1);
	double year1_TOD9JunEnergy = m_cf.at(CF_TOD9JunEnergy, 1);
	double year1_TOD9JulEnergy = m_cf.at(CF_TOD9JulEnergy, 1);
	double year1_TOD9AugEnergy = m_cf.at(CF_TOD9AugEnergy, 1);
	double year1_TOD9SepEnergy = m_cf.at(CF_TOD9SepEnergy, 1);
	double year1_TOD9OctEnergy = m_cf.at(CF_TOD9OctEnergy, 1);
	double year1_TOD9NovEnergy = m_cf.at(CF_TOD9NovEnergy, 1);
	double year1_TOD9DecEnergy = m_cf.at(CF_TOD9DecEnergy, 1);

	for (int y = 0; y <= m_nyears; y++)
	{
		// compute energy dispatched
		m_cf.at(CF_TODJanEnergy, y) = year1_TODJanEnergy * m_degradation[y];
		m_cf.at(CF_TODFebEnergy, y) = year1_TODFebEnergy * m_degradation[y];
		m_cf.at(CF_TODMarEnergy, y) = year1_TODMarEnergy * m_degradation[y];
		m_cf.at(CF_TODAprEnergy, y) = year1_TODAprEnergy * m_degradation[y];
		m_cf.at(CF_TODMayEnergy, y) = year1_TODMayEnergy * m_degradation[y];
		m_cf.at(CF_TODJunEnergy, y) = year1_TODJunEnergy * m_degradation[y];
		m_cf.at(CF_TODJulEnergy, y) = year1_TODJulEnergy * m_degradation[y];
		m_cf.at(CF_TODAugEnergy, y) = year1_TODAugEnergy * m_degradation[y];
		m_cf.at(CF_TODSepEnergy, y) = year1_TODSepEnergy * m_degradation[y];
		m_cf.at(CF_TODOctEnergy, y) = year1_TODOctEnergy * m_degradation[y];
		m_cf.at(CF_TODNovEnergy, y) = year1_TODNovEnergy * m_degradation[y];
		m_cf.at(CF_TODDecEnergy, y) = year1_TODDecEnergy * m_degradation[y];

		m_cf.at(CF_TOD1JanEnergy, y) = year1_TOD1JanEnergy * m_degradation[y];
		m_cf.at(CF_TOD1FebEnergy, y) = year1_TOD1FebEnergy * m_degradation[y];
		m_cf.at(CF_TOD1MarEnergy, y) = year1_TOD1MarEnergy * m_degradation[y];
		m_cf.at(CF_TOD1AprEnergy, y) = year1_TOD1AprEnergy * m_degradation[y];
		m_cf.at(CF_TOD1MayEnergy, y) = year1_TOD1MayEnergy * m_degradation[y];
		m_cf.at(CF_TOD1JunEnergy, y) = year1_TOD1JunEnergy * m_degradation[y];
		m_cf.at(CF_TOD1JulEnergy, y) = year1_TOD1JulEnergy * m_degradation[y];
		m_cf.at(CF_TOD1AugEnergy, y) = year1_TOD1AugEnergy * m_degradation[y];
		m_cf.at(CF_TOD1SepEnergy, y) = year1_TOD1SepEnergy * m_degradation[y];
		m_cf.at(CF_TOD1OctEnergy, y) = year1_TOD1OctEnergy * m_degradation[y];
		m_cf.at(CF_TOD1NovEnergy, y) = year1_TOD1NovEnergy * m_degradation[y];
		m_cf.at(CF_TOD1DecEnergy, y) = year1_TOD1DecEnergy * m_degradation[y];

		m_cf.at(CF_TOD2JanEnergy, y) = year1_TOD2JanEnergy * m_degradation[y];
		m_cf.at(CF_TOD2FebEnergy, y) = year1_TOD2FebEnergy * m_degradation[y];
		m_cf.at(CF_TOD2MarEnergy, y) = year1_TOD2MarEnergy * m_degradation[y];
		m_cf.at(CF_TOD2AprEnergy, y) = year1_TOD2AprEnergy * m_degradation[y];
		m_cf.at(CF_TOD2MayEnergy, y) = year1_TOD2MayEnergy * m_degradation[y];
		m_cf.at(CF_TOD2JunEnergy, y) = year1_TOD2JunEnergy * m_degradation[y];
		m_cf.at(CF_TOD2JulEnergy, y) = year1_TOD2JulEnergy * m_degradation[y];
		m_cf.at(CF_TOD2AugEnergy, y) = year1_TOD2AugEnergy * m_degradation[y];
		m_cf.at(CF_TOD2SepEnergy, y) = year1_TOD2SepEnergy * m_degradation[y];
		m_cf.at(CF_TOD2OctEnergy, y) = year1_TOD2OctEnergy * m_degradation[y];
		m_cf.at(CF_TOD2NovEnergy, y) = year1_TOD2NovEnergy * m_degradation[y];
		m_cf.at(CF_TOD2DecEnergy, y) = year1_TOD2DecEnergy * m_degradation[y];

		m_cf.at(CF_TOD3JanEnergy, y) = year1_TOD3JanEnergy * m_degradation[y];
		m_cf.at(CF_TOD3FebEnergy, y) = year1_TOD3FebEnergy * m_degradation[y];
		m_cf.at(CF_TOD3MarEnergy, y) = year1_TOD3MarEnergy * m_degradation[y];
		m_cf.at(CF_TOD3AprEnergy, y) = year1_TOD3AprEnergy * m_degradation[y];
		m_cf.at(CF_TOD3MayEnergy, y) = year1_TOD3MayEnergy * m_degradation[y];
		m_cf.at(CF_TOD3JunEnergy, y) = year1_TOD3JunEnergy * m_degradation[y];
		m_cf.at(CF_TOD3JulEnergy, y) = year1_TOD3JulEnergy * m_degradation[y];
		m_cf.at(CF_TOD3AugEnergy, y) = year1_TOD3AugEnergy * m_degradation[y];
		m_cf.at(CF_TOD3SepEnergy, y) = year1_TOD3SepEnergy * m_degradation[y];
		m_cf.at(CF_TOD3OctEnergy, y) = year1_TOD3OctEnergy * m_degradation[y];
		m_cf.at(CF_TOD3NovEnergy, y) = year1_TOD3NovEnergy * m_degradation[y];
		m_cf.at(CF_TOD3DecEnergy, y) = year1_TOD3DecEnergy * m_degradation[y];

		m_cf.at(CF_TOD4JanEnergy, y) = year1_TOD4JanEnergy * m_degradation[y];
		m_cf.at(CF_TOD4FebEnergy, y) = year1_TOD4FebEnergy * m_degradation[y];
		m_cf.at(CF_TOD4MarEnergy, y) = year1_TOD4MarEnergy * m_degradation[y];
		m_cf.at(CF_TOD4AprEnergy, y) = year1_TOD4AprEnergy * m_degradation[y];
		m_cf.at(CF_TOD4MayEnergy, y) = year1_TOD4MayEnergy * m_degradation[y];
		m_cf.at(CF_TOD4JunEnergy, y) = year1_TOD4JunEnergy * m_degradation[y];
		m_cf.at(CF_TOD4JulEnergy, y) = year1_TOD4JulEnergy * m_degradation[y];
		m_cf.at(CF_TOD4AugEnergy, y) = year1_TOD4AugEnergy * m_degradation[y];
		m_cf.at(CF_TOD4SepEnergy, y) = year1_TOD4SepEnergy * m_degradation[y];
		m_cf.at(CF_TOD4OctEnergy, y) = year1_TOD4OctEnergy * m_degradation[y];
		m_cf.at(CF_TOD4NovEnergy, y) = year1_TOD4NovEnergy * m_degradation[y];
		m_cf.at(CF_TOD4DecEnergy, y) = year1_TOD4DecEnergy * m_degradation[y];

		m_cf.at(CF_TOD5JanEnergy, y) = year1_TOD5JanEnergy * m_degradation[y];
		m_cf.at(CF_TOD5FebEnergy, y) = year1_TOD5FebEnergy * m_degradation[y];
		m_cf.at(CF_TOD5MarEnergy, y) = year1_TOD5MarEnergy * m_degradation[y];
		m_cf.at(CF_TOD5AprEnergy, y) = year1_TOD5AprEnergy * m_degradation[y];
		m_cf.at(CF_TOD5MayEnergy, y) = year1_TOD5MayEnergy * m_degradation[y];
		m_cf.at(CF_TOD5JunEnergy, y) = year1_TOD5JunEnergy * m_degradation[y];
		m_cf.at(CF_TOD5JulEnergy, y) = year1_TOD5JulEnergy * m_degradation[y];
		m_cf.at(CF_TOD5AugEnergy, y) = year1_TOD5AugEnergy * m_degradation[y];
		m_cf.at(CF_TOD5SepEnergy, y) = year1_TOD5SepEnergy * m_degradation[y];
		m_cf.at(CF_TOD5OctEnergy, y) = year1_TOD5OctEnergy * m_degradation[y];
		m_cf.at(CF_TOD5NovEnergy, y) = year1_TOD5NovEnergy * m_degradation[y];
		m_cf.at(CF_TOD5DecEnergy, y) = year1_TOD5DecEnergy * m_degradation[y];

		m_cf.at(CF_TOD6JanEnergy, y) = year1_TOD6JanEnergy * m_degradation[y];
		m_cf.at(CF_TOD6FebEnergy, y) = year1_TOD6FebEnergy * m_degradation[y];
		m_cf.at(CF_TOD6MarEnergy, y) = year1_TOD6MarEnergy * m_degradation[y];
		m_cf.at(CF_TOD6AprEnergy, y) = year1_TOD6AprEnergy * m_degradation[y];
		m_cf.at(CF_TOD6MayEnergy, y) = year1_TOD6MayEnergy * m_degradation[y];
		m_cf.at(CF_TOD6JunEnergy, y) = year1_TOD6JunEnergy * m_degradation[y];
		m_cf.at(CF_TOD6JulEnergy, y) = year1_TOD6JulEnergy * m_degradation[y];
		m_cf.at(CF_TOD6AugEnergy, y) = year1_TOD6AugEnergy * m_degradation[y];
		m_cf.at(CF_TOD6SepEnergy, y) = year1_TOD6SepEnergy * m_degradation[y];
		m_cf.at(CF_TOD6OctEnergy, y) = year1_TOD6OctEnergy * m_degradation[y];
		m_cf.at(CF_TOD6NovEnergy, y) = year1_TOD6NovEnergy * m_degradation[y];
		m_cf.at(CF_TOD6DecEnergy, y) = year1_TOD6DecEnergy * m_degradation[y];

		m_cf.at(CF_TOD7JanEnergy, y) = year1_TOD7JanEnergy * m_degradation[y];
		m_cf.at(CF_TOD7FebEnergy, y) = year1_TOD7FebEnergy * m_degradation[y];
		m_cf.at(CF_TOD7MarEnergy, y) = year1_TOD7MarEnergy * m_degradation[y];
		m_cf.at(CF_TOD7AprEnergy, y) = year1_TOD7AprEnergy * m_degradation[y];
		m_cf.at(CF_TOD7MayEnergy, y) = year1_TOD7MayEnergy * m_degradation[y];
		m_cf.at(CF_TOD7JunEnergy, y) = year1_TOD7JunEnergy * m_degradation[y];
		m_cf.at(CF_TOD7JulEnergy, y) = year1_TOD7JulEnergy * m_degradation[y];
		m_cf.at(CF_TOD7AugEnergy, y) = year1_TOD7AugEnergy * m_degradation[y];
		m_cf.at(CF_TOD7SepEnergy, y) = year1_TOD7SepEnergy * m_degradation[y];
		m_cf.at(CF_TOD7OctEnergy, y) = year1_TOD7OctEnergy * m_degradation[y];
		m_cf.at(CF_TOD7NovEnergy, y) = year1_TOD7NovEnergy * m_degradation[y];
		m_cf.at(CF_TOD7DecEnergy, y) = year1_TOD7DecEnergy * m_degradation[y];

		m_cf.at(CF_TOD8JanEnergy, y) = year1_TOD8JanEnergy * m_degradation[y];
		m_cf.at(CF_TOD8FebEnergy, y) = year1_TOD8FebEnergy * m_degradation[y];
		m_cf.at(CF_TOD8MarEnergy, y) = year1_TOD8MarEnergy * m_degradation[y];
		m_cf.at(CF_TOD8AprEnergy, y) = year1_TOD8AprEnergy * m_degradation[y];
		m_cf.at(CF_TOD8MayEnergy, y) = year1_TOD8MayEnergy * m_degradation[y];
		m_cf.at(CF_TOD8JunEnergy, y) = year1_TOD8JunEnergy * m_degradation[y];
		m_cf.at(CF_TOD8JulEnergy, y) = year1_TOD8JulEnergy * m_degradation[y];
		m_cf.at(CF_TOD8AugEnergy, y) = year1_TOD8AugEnergy * m_degradation[y];
		m_cf.at(CF_TOD8SepEnergy, y) = year1_TOD8SepEnergy * m_degradation[y];
		m_cf.at(CF_TOD8OctEnergy, y) = year1_TOD8OctEnergy * m_degradation[y];
		m_cf.at(CF_TOD8NovEnergy, y) = year1_TOD8NovEnergy * m_degradation[y];
		m_cf.at(CF_TOD8DecEnergy, y) = year1_TOD8DecEnergy * m_degradation[y];

		m_cf.at(CF_TOD9JanEnergy, y) = year1_TOD9JanEnergy * m_degradation[y];
		m_cf.at(CF_TOD9FebEnergy, y) = year1_TOD9FebEnergy * m_degradation[y];
		m_cf.at(CF_TOD9MarEnergy, y) = year1_TOD9MarEnergy * m_degradation[y];
		m_cf.at(CF_TOD9AprEnergy, y) = year1_TOD9AprEnergy * m_degradation[y];
		m_cf.at(CF_TOD9MayEnergy, y) = year1_TOD9MayEnergy * m_degradation[y];
		m_cf.at(CF_TOD9JunEnergy, y) = year1_TOD9JunEnergy * m_degradation[y];
		m_cf.at(CF_TOD9JulEnergy, y) = year1_TOD9JulEnergy * m_degradation[y];
		m_cf.at(CF_TOD9AugEnergy, y) = year1_TOD9AugEnergy * m_degradation[y];
		m_cf.at(CF_TOD9SepEnergy, y) = year1_TOD9SepEnergy * m_degradation[y];
		m_cf.at(CF_TOD9OctEnergy, y) = year1_TOD9OctEnergy * m_degradation[y];
		m_cf.at(CF_TOD9NovEnergy, y) = year1_TOD9NovEnergy * m_degradation[y];
		m_cf.at(CF_TOD9DecEnergy, y) = year1_TOD9DecEnergy * m_degradation[y];
	}
	return true;
}


bool dispatch_calculations::compute_dispatch_output_ts()
{
	//Calculate energy dispatched in each month

	size_t nrec_gen_per_year = m_ngen;
//	if (m_cm->as_integer("system_use_lifetime_output") == 1)
//		nrec_gen_per_year = m_ngen / m_nyears;
	size_t step_per_hour_gen = nrec_gen_per_year / 8760;
	if (step_per_hour_gen < 1 || step_per_hour_gen > 60 || step_per_hour_gen * 8760 != nrec_gen_per_year)
	{
		m_error = util::format("invalid number of gen records (%d): must be an integer multiple of 8760", (int)nrec_gen_per_year);
		throw exec_error("dispatch_calculations", m_error);
		return false;
	}
	if (m_nmultipliers != nrec_gen_per_year)
	{
		m_error = util::format("invalid number of gen records per year (%d) must be equal to number of ppa multiplier records (%d)", (int)nrec_gen_per_year, (int)m_nmultipliers);
		throw exec_error("dispatch_calculations", m_error);
		return false;
	}
	ssc_number_t ts_hour_gen = 1.0f / step_per_hour_gen;


	m_cf.at(CF_TODJanEnergy, 1) = 0;
	m_cf.at(CF_TODFebEnergy, 1) = 0;
	m_cf.at(CF_TODMarEnergy, 1) = 0;
	m_cf.at(CF_TODAprEnergy, 1) = 0;
	m_cf.at(CF_TODMayEnergy, 1) = 0;
	m_cf.at(CF_TODJunEnergy, 1) = 0;
	m_cf.at(CF_TODJulEnergy, 1) = 0;
	m_cf.at(CF_TODAugEnergy, 1) = 0;
	m_cf.at(CF_TODSepEnergy, 1) = 0;
	m_cf.at(CF_TODOctEnergy, 1) = 0;
	m_cf.at(CF_TODNovEnergy, 1) = 0;
	m_cf.at(CF_TODDecEnergy, 1) = 0;

	m_cf.at(CF_TODJanRevenue, 1) = 0;
	m_cf.at(CF_TODFebRevenue, 1) = 0;
	m_cf.at(CF_TODMarRevenue, 1) = 0;
	m_cf.at(CF_TODAprRevenue, 1) = 0;
	m_cf.at(CF_TODMayRevenue, 1) = 0;
	m_cf.at(CF_TODJunRevenue, 1) = 0;
	m_cf.at(CF_TODJulRevenue, 1) = 0;
	m_cf.at(CF_TODAugRevenue, 1) = 0;
	m_cf.at(CF_TODSepRevenue, 1) = 0;
	m_cf.at(CF_TODOctRevenue, 1) = 0;
	m_cf.at(CF_TODNovRevenue, 1) = 0;
	m_cf.at(CF_TODDecRevenue, 1) = 0;

	int i = 0;
	for (int m = 0; m<12; m++)
	{
		for (size_t d = 0; d<util::nday[m]; d++)
		{
			for (int h = 0; h<24 && i<(int)nrec_gen_per_year; h++)
			{
				for (int k = 0; k < (int)step_per_hour_gen; k++)
				{
					switch (m)
					{
					case 0:
						m_cf.at(CF_TODJanEnergy, 1) += m_gen[i] * ts_hour_gen;
						m_cf.at(CF_TODJanRevenue, 1) += m_gen[i] * ts_hour_gen * m_multipliers[i];
						break;
					case 1:
						m_cf.at(CF_TODFebEnergy, 1) += m_gen[i] * ts_hour_gen;
						m_cf.at(CF_TODFebRevenue, 1) += m_gen[i] * ts_hour_gen* m_multipliers[i];
						break;
					case 2:
						m_cf.at(CF_TODMarEnergy, 1) += m_gen[i] * ts_hour_gen;
						m_cf.at(CF_TODMarRevenue, 1) += m_gen[i] * ts_hour_gen * m_multipliers[i];
						break;
					case 3:
						m_cf.at(CF_TODAprEnergy, 1) += m_gen[i] * ts_hour_gen;
						m_cf.at(CF_TODAprRevenue, 1) += m_gen[i] * ts_hour_gen * m_multipliers[i];
						break;
					case 4:
						m_cf.at(CF_TODMayEnergy, 1) += m_gen[i] * ts_hour_gen;
						m_cf.at(CF_TODMayRevenue, 1) += m_gen[i] * ts_hour_gen * m_multipliers[i];
						break;
					case 5:
						m_cf.at(CF_TODJunEnergy, 1) += m_gen[i] * ts_hour_gen;
						m_cf.at(CF_TODJunRevenue, 1) += m_gen[i] * ts_hour_gen* m_multipliers[i];
						break;
					case 6:
						m_cf.at(CF_TODJulEnergy, 1) += m_gen[i] * ts_hour_gen;
						m_cf.at(CF_TODJulRevenue, 1) += m_gen[i] * ts_hour_gen * m_multipliers[i];
						break;
					case 7:
						m_cf.at(CF_TODAugEnergy, 1) += m_gen[i] * ts_hour_gen;
						m_cf.at(CF_TODAugRevenue, 1) += m_gen[i] * ts_hour_gen * m_multipliers[i];
						break;
					case 8:
						m_cf.at(CF_TODSepEnergy, 1) += m_gen[i] * ts_hour_gen;
						m_cf.at(CF_TODSepRevenue, 1) += m_gen[i] * ts_hour_gen * m_multipliers[i];
						break;
					case 9:
						m_cf.at(CF_TODOctEnergy, 1) += m_gen[i] * ts_hour_gen;
						m_cf.at(CF_TODOctRevenue, 1) += m_gen[i] * ts_hour_gen * m_multipliers[i];
						break;
					case 10:
						m_cf.at(CF_TODNovEnergy, 1) += m_gen[i] * ts_hour_gen;
						m_cf.at(CF_TODNovRevenue, 1) += m_gen[i] * ts_hour_gen * m_multipliers[i];
						break;
					case 11:
						m_cf.at(CF_TODDecEnergy, 1) += m_gen[i] * ts_hour_gen;
						m_cf.at(CF_TODDecRevenue, 1) += m_gen[i] * ts_hour_gen * m_multipliers[i];
						break;
					}
					i++;
				}
			}
		}
	}

	double year1_TODJanEnergy = m_cf.at(CF_TODJanEnergy, 1);
	double year1_TODFebEnergy = m_cf.at(CF_TODFebEnergy, 1);
	double year1_TODMarEnergy = m_cf.at(CF_TODMarEnergy, 1);
	double year1_TODAprEnergy = m_cf.at(CF_TODAprEnergy, 1);
	double year1_TODMayEnergy = m_cf.at(CF_TODMayEnergy, 1);
	double year1_TODJunEnergy = m_cf.at(CF_TODJunEnergy, 1);
	double year1_TODJulEnergy = m_cf.at(CF_TODJulEnergy, 1);
	double year1_TODAugEnergy = m_cf.at(CF_TODAugEnergy, 1);
	double year1_TODSepEnergy = m_cf.at(CF_TODSepEnergy, 1);
	double year1_TODOctEnergy = m_cf.at(CF_TODOctEnergy, 1);
	double year1_TODNovEnergy = m_cf.at(CF_TODNovEnergy, 1);
	double year1_TODDecEnergy = m_cf.at(CF_TODDecEnergy, 1);

	double year1_TODJanRevenue = m_cf.at(CF_TODJanRevenue, 1);
	double year1_TODFebRevenue = m_cf.at(CF_TODFebRevenue, 1);
	double year1_TODMarRevenue = m_cf.at(CF_TODMarRevenue, 1);
	double year1_TODAprRevenue = m_cf.at(CF_TODAprRevenue, 1);
	double year1_TODMayRevenue = m_cf.at(CF_TODMayRevenue, 1);
	double year1_TODJunRevenue = m_cf.at(CF_TODJunRevenue, 1);
	double year1_TODJulRevenue = m_cf.at(CF_TODJulRevenue, 1);
	double year1_TODAugRevenue = m_cf.at(CF_TODAugRevenue, 1);
	double year1_TODSepRevenue = m_cf.at(CF_TODSepRevenue, 1);
	double year1_TODOctRevenue = m_cf.at(CF_TODOctRevenue, 1);
	double year1_TODNovRevenue = m_cf.at(CF_TODNovRevenue, 1);
	double year1_TODDecRevenue = m_cf.at(CF_TODDecRevenue, 1);

	for (int y = 0; y <= m_nyears; y++)
	{
		// compute energy dispatched
		m_cf.at(CF_TODJanEnergy, y) = year1_TODJanEnergy * m_degradation[y];
		m_cf.at(CF_TODFebEnergy, y) = year1_TODFebEnergy * m_degradation[y];
		m_cf.at(CF_TODMarEnergy, y) = year1_TODMarEnergy * m_degradation[y];
		m_cf.at(CF_TODAprEnergy, y) = year1_TODAprEnergy * m_degradation[y];
		m_cf.at(CF_TODMayEnergy, y) = year1_TODMayEnergy * m_degradation[y];
		m_cf.at(CF_TODJunEnergy, y) = year1_TODJunEnergy * m_degradation[y];
		m_cf.at(CF_TODJulEnergy, y) = year1_TODJulEnergy * m_degradation[y];
		m_cf.at(CF_TODAugEnergy, y) = year1_TODAugEnergy * m_degradation[y];
		m_cf.at(CF_TODSepEnergy, y) = year1_TODSepEnergy * m_degradation[y];
		m_cf.at(CF_TODOctEnergy, y) = year1_TODOctEnergy * m_degradation[y];
		m_cf.at(CF_TODNovEnergy, y) = year1_TODNovEnergy * m_degradation[y];
		m_cf.at(CF_TODDecEnergy, y) = year1_TODDecEnergy * m_degradation[y];
		// compute energy value
		m_cf.at(CF_TODJanRevenue, y) = year1_TODJanRevenue * m_degradation[y];
		m_cf.at(CF_TODFebRevenue, y) = year1_TODFebRevenue * m_degradation[y];
		m_cf.at(CF_TODMarRevenue, y) = year1_TODMarRevenue * m_degradation[y];
		m_cf.at(CF_TODAprRevenue, y) = year1_TODAprRevenue * m_degradation[y];
		m_cf.at(CF_TODMayRevenue, y) = year1_TODMayRevenue * m_degradation[y];
		m_cf.at(CF_TODJunRevenue, y) = year1_TODJunRevenue * m_degradation[y];
		m_cf.at(CF_TODJulRevenue, y) = year1_TODJulRevenue * m_degradation[y];
		m_cf.at(CF_TODAugRevenue, y) = year1_TODAugRevenue * m_degradation[y];
		m_cf.at(CF_TODSepRevenue, y) = year1_TODSepRevenue * m_degradation[y];
		m_cf.at(CF_TODOctRevenue, y) = year1_TODOctRevenue * m_degradation[y];
		m_cf.at(CF_TODNovRevenue, y) = year1_TODNovRevenue * m_degradation[y];
		m_cf.at(CF_TODDecRevenue, y) = year1_TODDecRevenue * m_degradation[y];
	}
	return true;
}

bool dispatch_calculations::compute_lifetime_dispatch_output_ts()
{
	//Calculate energy dispatched in each month
	// assumption is that gen contains all avaialbility and curtailment
	size_t nrec_gen_per_year = m_ngen;
	if (m_cm->as_integer("system_use_lifetime_output") == 1) // should be true always
		nrec_gen_per_year = m_ngen / m_nyears;
	size_t step_per_hour_gen = nrec_gen_per_year / 8760;
	if (step_per_hour_gen < 1 || step_per_hour_gen > 60 || step_per_hour_gen * 8760 != nrec_gen_per_year)
	{
		m_error = util::format("invalid number of gen records (%d): must be an integer multiple of 8760", (int)nrec_gen_per_year);
		throw exec_error("dispatch_calculations", m_error);
		return false;
	}
	if (m_nmultipliers != nrec_gen_per_year)
	{
		m_error = util::format("invalid number of gen records per year (%d) must be equal to number of ppa multiplier records (%d)", (int)nrec_gen_per_year, (int)m_nmultipliers);
		throw exec_error("dispatch_calculations", m_error);
		return false;
	}
	ssc_number_t ts_hour_gen = 1.0f / step_per_hour_gen;

	for (int iyear = 0; iyear < m_nyears; iyear++)
	{
		m_cf.at(CF_TODJanEnergy, iyear + 1) = 0;
		m_cf.at(CF_TODFebEnergy, iyear + 1) = 0;
		m_cf.at(CF_TODMarEnergy, iyear + 1) = 0;
		m_cf.at(CF_TODAprEnergy, iyear + 1) = 0;
		m_cf.at(CF_TODMayEnergy, iyear + 1) = 0;
		m_cf.at(CF_TODJunEnergy, iyear + 1) = 0;
		m_cf.at(CF_TODJulEnergy, iyear + 1) = 0;
		m_cf.at(CF_TODAugEnergy, iyear + 1) = 0;
		m_cf.at(CF_TODSepEnergy, iyear + 1) = 0;
		m_cf.at(CF_TODOctEnergy, iyear + 1) = 0;
		m_cf.at(CF_TODNovEnergy, iyear + 1) = 0;
		m_cf.at(CF_TODDecEnergy, iyear + 1) = 0;

		m_cf.at(CF_TODJanRevenue, iyear + 1) = 0;
		m_cf.at(CF_TODFebRevenue, iyear + 1) = 0;
		m_cf.at(CF_TODMarRevenue, iyear + 1) = 0;
		m_cf.at(CF_TODAprRevenue, iyear + 1) = 0;
		m_cf.at(CF_TODMayRevenue, iyear + 1) = 0;
		m_cf.at(CF_TODJunRevenue, iyear + 1) = 0;
		m_cf.at(CF_TODJulRevenue, iyear + 1) = 0;
		m_cf.at(CF_TODAugRevenue, iyear + 1) = 0;
		m_cf.at(CF_TODSepRevenue, iyear + 1) = 0;
		m_cf.at(CF_TODOctRevenue, iyear + 1) = 0;
		m_cf.at(CF_TODNovRevenue, iyear + 1) = 0;
		m_cf.at(CF_TODDecRevenue, iyear + 1) = 0;

		int i = 0; // iterator for current year

		for (int m = 0; m < 12; m++)
		{
			for (size_t d = 0; d < util::nday[m]; d++)
			{
				for (int h = 0; h < 24 && i < (int)nrec_gen_per_year; h++)
				{
					for (int k = 0; k < (int)step_per_hour_gen; k++)
					{
						switch (m)
						{
						case 0:
							m_cf.at(CF_TODJanEnergy, iyear + 1) += m_gen[i + (iyear * nrec_gen_per_year)] * ts_hour_gen;
							m_cf.at(CF_TODJanRevenue, iyear + 1) += m_gen[i + (iyear * nrec_gen_per_year)] * ts_hour_gen * m_multipliers[i];
							break;
						case 1:
							m_cf.at(CF_TODFebEnergy, iyear + 1) += m_gen[i + (iyear * nrec_gen_per_year)] * ts_hour_gen;
							m_cf.at(CF_TODFebRevenue, iyear + 1) += m_gen[i + (iyear * nrec_gen_per_year)] * ts_hour_gen* m_multipliers[i];
							break;
						case 2:
							m_cf.at(CF_TODMarEnergy, iyear + 1) += m_gen[i + (iyear * nrec_gen_per_year)] * ts_hour_gen;
							m_cf.at(CF_TODMarRevenue, iyear + 1) += m_gen[i + (iyear * nrec_gen_per_year)] * ts_hour_gen * m_multipliers[i];
							break;
						case 3:
							m_cf.at(CF_TODAprEnergy, iyear + 1) += m_gen[i + (iyear * nrec_gen_per_year)] * ts_hour_gen;
							m_cf.at(CF_TODAprRevenue, iyear + 1) += m_gen[i + (iyear * nrec_gen_per_year)] * ts_hour_gen * m_multipliers[i];
							break;
						case 4:
							m_cf.at(CF_TODMayEnergy, iyear + 1) += m_gen[i + (iyear * nrec_gen_per_year)] * ts_hour_gen;
							m_cf.at(CF_TODMayRevenue, iyear + 1) += m_gen[i + (iyear * nrec_gen_per_year)] * ts_hour_gen * m_multipliers[i];
							break;
						case 5:
							m_cf.at(CF_TODJunEnergy, iyear + 1) += m_gen[i + (iyear * nrec_gen_per_year)] * ts_hour_gen;
							m_cf.at(CF_TODJunRevenue, iyear + 1) += m_gen[i + (iyear * nrec_gen_per_year)] * ts_hour_gen* m_multipliers[i];
							break;
						case 6:
							m_cf.at(CF_TODJulEnergy, iyear + 1) += m_gen[i + (iyear * nrec_gen_per_year)] * ts_hour_gen;
							m_cf.at(CF_TODJulRevenue, iyear + 1) += m_gen[i + (iyear * nrec_gen_per_year)] * ts_hour_gen * m_multipliers[i];
							break;
						case 7:
							m_cf.at(CF_TODAugEnergy, iyear + 1) += m_gen[i + (iyear * nrec_gen_per_year)] * ts_hour_gen;
							m_cf.at(CF_TODAugRevenue, iyear + 1) += m_gen[i + (iyear * nrec_gen_per_year)] * ts_hour_gen * m_multipliers[i];
							break;
						case 8:
							m_cf.at(CF_TODSepEnergy, iyear + 1) += m_gen[i + (iyear * nrec_gen_per_year)] * ts_hour_gen;
							m_cf.at(CF_TODSepRevenue, iyear + 1) += m_gen[i + (iyear * nrec_gen_per_year)] * ts_hour_gen * m_multipliers[i];
							break;
						case 9:
							m_cf.at(CF_TODOctEnergy, iyear + 1) += m_gen[i + (iyear * nrec_gen_per_year)] * ts_hour_gen;
							m_cf.at(CF_TODOctRevenue, iyear + 1) += m_gen[i + (iyear * nrec_gen_per_year)] * ts_hour_gen * m_multipliers[i];
							break;
						case 10:
							m_cf.at(CF_TODNovEnergy, iyear + 1) += m_gen[i + (iyear * nrec_gen_per_year)] * ts_hour_gen;
							m_cf.at(CF_TODNovRevenue, iyear + 1) += m_gen[i + (iyear * nrec_gen_per_year)] * ts_hour_gen * m_multipliers[i];
							break;
						case 11:
							m_cf.at(CF_TODDecEnergy, iyear + 1) += m_gen[i + (iyear * nrec_gen_per_year)] * ts_hour_gen;
							m_cf.at(CF_TODDecRevenue, iyear + 1) += m_gen[i + (iyear * nrec_gen_per_year)] * ts_hour_gen * m_multipliers[i];
							break;
						}
						i++;
					}  // steps per hour
				} // hours per day
			} // days per month
		} // months per year
	} // years per analysis period
	return true;
}


bool dispatch_calculations::compute_lifetime_dispatch_output()
{
	//Calculate energy dispatched in each dispatch period


	int h;
	size_t count=m_hourly_energy.size();

	// hourly energy includes all curtailment, availability
	if ((int)count != (8760 * m_nyears))
	{
		std::stringstream outm;
		outm << "Bad hourly gen output length (" << count << "), should be (analysis period-1) * 8760 value (" << 8760 * m_nyears << ")";
		m_cm->log(outm.str());
		return false;
	}

	//// hourly dispatch
	//dispatch_calculations hourly_dispatch(this);
	//if (!hourly_dispatch.setup())
	//	throw exec_error("ippppa", "failed to setup dispatch periods: " + hourly_dispatch.error());

	for (int y = 1; y <= m_nyears; y++)
	{
		m_cf.at(CF_TOD1Energy, y) = 0;
		m_cf.at(CF_TOD2Energy, y) = 0;
		m_cf.at(CF_TOD3Energy, y) = 0;
		m_cf.at(CF_TOD4Energy, y) = 0;
		m_cf.at(CF_TOD5Energy, y) = 0;
		m_cf.at(CF_TOD6Energy, y) = 0;
		m_cf.at(CF_TOD7Energy, y) = 0;
		m_cf.at(CF_TOD8Energy, y) = 0;
		m_cf.at(CF_TOD9Energy, y) = 0;

		for (h = 0; h<8760; h++)
		{
			switch (m_periods[h])
			{
			case 1:
				m_cf.at(CF_TOD1Energy, y) += m_hourly_energy[(y - 1) * 8760 + h];
				break;
			case 2:
				m_cf.at(CF_TOD2Energy, y) += m_hourly_energy[(y - 1) * 8760 + h];
				break;
			case 3:
				m_cf.at(CF_TOD3Energy, y) += m_hourly_energy[(y - 1) * 8760 + h];
				break;
			case 4:
				m_cf.at(CF_TOD4Energy, y) += m_hourly_energy[(y - 1) * 8760 + h];
				break;
			case 5:
				m_cf.at(CF_TOD5Energy, y) += m_hourly_energy[(y - 1) * 8760 + h];
				break;
			case 6:
				m_cf.at(CF_TOD6Energy, y) += m_hourly_energy[(y - 1) * 8760 + h];
				break;
			case 7:
				m_cf.at(CF_TOD7Energy, y) += m_hourly_energy[(y - 1) * 8760 + h];
				break;
			case 8:
				m_cf.at(CF_TOD8Energy, y) += m_hourly_energy[(y - 1) * 8760 + h];
				break;
			case 9:
				m_cf.at(CF_TOD9Energy, y) += m_hourly_energy[(y - 1) * 8760 + h];
				break;
			}
		}
	}


	return true;
}

bool dispatch_calculations::process_lifetime_dispatch_output()
{
	//Calculate energy dispatched in each dispatch period

	size_t count=m_hourly_energy.size();

	// hourly energy include all curtailment, availability
	if ((int)count != (8760 * m_nyears))
	{
		std::stringstream outm;
		outm << "Bad hourly gen output length (" << count << "), should be (analysis period-1) * 8760 value (" << 8760 * m_nyears << ")";
		m_cm->log(outm.str());
		return false;
	}

	//// hourly dispatch
	//dispatch_calculations hourly_dispatch(this);
	//if (!hourly_dispatch.setup())
	//	throw exec_error("ippppa", "failed to setup dispatch periods: " + hourly_dispatch.error());


	for (int y = 1; y <= m_nyears; y++)
	{
		m_cf.at(CF_TODJanEnergy, y) = 0;
		m_cf.at(CF_TODFebEnergy, y) = 0;
		m_cf.at(CF_TODMarEnergy, y) = 0;
		m_cf.at(CF_TODAprEnergy, y) = 0;
		m_cf.at(CF_TODMayEnergy, y) = 0;
		m_cf.at(CF_TODJunEnergy, y) = 0;
		m_cf.at(CF_TODJulEnergy, y) = 0;
		m_cf.at(CF_TODAugEnergy, y) = 0;
		m_cf.at(CF_TODSepEnergy, y) = 0;
		m_cf.at(CF_TODOctEnergy, y) = 0;
		m_cf.at(CF_TODNovEnergy, y) = 0;
		m_cf.at(CF_TODDecEnergy, y) = 0;

		m_cf.at(CF_TOD1JanEnergy, y) = 0;
		m_cf.at(CF_TOD1FebEnergy, y) = 0;
		m_cf.at(CF_TOD1MarEnergy, y) = 0;
		m_cf.at(CF_TOD1AprEnergy, y) = 0;
		m_cf.at(CF_TOD1MayEnergy, y) = 0;
		m_cf.at(CF_TOD1JunEnergy, y) = 0;
		m_cf.at(CF_TOD1JulEnergy, y) = 0;
		m_cf.at(CF_TOD1AugEnergy, y) = 0;
		m_cf.at(CF_TOD1SepEnergy, y) = 0;
		m_cf.at(CF_TOD1OctEnergy, y) = 0;
		m_cf.at(CF_TOD1NovEnergy, y) = 0;
		m_cf.at(CF_TOD1DecEnergy, y) = 0;

		m_cf.at(CF_TOD2JanEnergy, y) = 0;
		m_cf.at(CF_TOD2FebEnergy, y) = 0;
		m_cf.at(CF_TOD2MarEnergy, y) = 0;
		m_cf.at(CF_TOD2AprEnergy, y) = 0;
		m_cf.at(CF_TOD2MayEnergy, y) = 0;
		m_cf.at(CF_TOD2JunEnergy, y) = 0;
		m_cf.at(CF_TOD2JulEnergy, y) = 0;
		m_cf.at(CF_TOD2AugEnergy, y) = 0;
		m_cf.at(CF_TOD2SepEnergy, y) = 0;
		m_cf.at(CF_TOD2OctEnergy, y) = 0;
		m_cf.at(CF_TOD2NovEnergy, y) = 0;
		m_cf.at(CF_TOD2DecEnergy, y) = 0;

		m_cf.at(CF_TOD3JanEnergy, y) = 0;
		m_cf.at(CF_TOD3FebEnergy, y) = 0;
		m_cf.at(CF_TOD3MarEnergy, y) = 0;
		m_cf.at(CF_TOD3AprEnergy, y) = 0;
		m_cf.at(CF_TOD3MayEnergy, y) = 0;
		m_cf.at(CF_TOD3JunEnergy, y) = 0;
		m_cf.at(CF_TOD3JulEnergy, y) = 0;
		m_cf.at(CF_TOD3AugEnergy, y) = 0;
		m_cf.at(CF_TOD3SepEnergy, y) = 0;
		m_cf.at(CF_TOD3OctEnergy, y) = 0;
		m_cf.at(CF_TOD3NovEnergy, y) = 0;
		m_cf.at(CF_TOD3DecEnergy, y) = 0;

		m_cf.at(CF_TOD4JanEnergy, y) = 0;
		m_cf.at(CF_TOD4FebEnergy, y) = 0;
		m_cf.at(CF_TOD4MarEnergy, y) = 0;
		m_cf.at(CF_TOD4AprEnergy, y) = 0;
		m_cf.at(CF_TOD4MayEnergy, y) = 0;
		m_cf.at(CF_TOD4JunEnergy, y) = 0;
		m_cf.at(CF_TOD4JulEnergy, y) = 0;
		m_cf.at(CF_TOD4AugEnergy, y) = 0;
		m_cf.at(CF_TOD4SepEnergy, y) = 0;
		m_cf.at(CF_TOD4OctEnergy, y) = 0;
		m_cf.at(CF_TOD4NovEnergy, y) = 0;
		m_cf.at(CF_TOD4DecEnergy, y) = 0;

		m_cf.at(CF_TOD5JanEnergy, y) = 0;
		m_cf.at(CF_TOD5FebEnergy, y) = 0;
		m_cf.at(CF_TOD5MarEnergy, y) = 0;
		m_cf.at(CF_TOD5AprEnergy, y) = 0;
		m_cf.at(CF_TOD5MayEnergy, y) = 0;
		m_cf.at(CF_TOD5JunEnergy, y) = 0;
		m_cf.at(CF_TOD5JulEnergy, y) = 0;
		m_cf.at(CF_TOD5AugEnergy, y) = 0;
		m_cf.at(CF_TOD5SepEnergy, y) = 0;
		m_cf.at(CF_TOD5OctEnergy, y) = 0;
		m_cf.at(CF_TOD5NovEnergy, y) = 0;
		m_cf.at(CF_TOD5DecEnergy, y) = 0;

		m_cf.at(CF_TOD6JanEnergy, y) = 0;
		m_cf.at(CF_TOD6FebEnergy, y) = 0;
		m_cf.at(CF_TOD6MarEnergy, y) = 0;
		m_cf.at(CF_TOD6AprEnergy, y) = 0;
		m_cf.at(CF_TOD6MayEnergy, y) = 0;
		m_cf.at(CF_TOD6JunEnergy, y) = 0;
		m_cf.at(CF_TOD6JulEnergy, y) = 0;
		m_cf.at(CF_TOD6AugEnergy, y) = 0;
		m_cf.at(CF_TOD6SepEnergy, y) = 0;
		m_cf.at(CF_TOD6OctEnergy, y) = 0;
		m_cf.at(CF_TOD6NovEnergy, y) = 0;
		m_cf.at(CF_TOD6DecEnergy, y) = 0;

		m_cf.at(CF_TOD7JanEnergy, y) = 0;
		m_cf.at(CF_TOD7FebEnergy, y) = 0;
		m_cf.at(CF_TOD7MarEnergy, y) = 0;
		m_cf.at(CF_TOD7AprEnergy, y) = 0;
		m_cf.at(CF_TOD7MayEnergy, y) = 0;
		m_cf.at(CF_TOD7JunEnergy, y) = 0;
		m_cf.at(CF_TOD7JulEnergy, y) = 0;
		m_cf.at(CF_TOD7AugEnergy, y) = 0;
		m_cf.at(CF_TOD7SepEnergy, y) = 0;
		m_cf.at(CF_TOD7OctEnergy, y) = 0;
		m_cf.at(CF_TOD7NovEnergy, y) = 0;
		m_cf.at(CF_TOD7DecEnergy, y) = 0;

		m_cf.at(CF_TOD8JanEnergy, y) = 0;
		m_cf.at(CF_TOD8FebEnergy, y) = 0;
		m_cf.at(CF_TOD8MarEnergy, y) = 0;
		m_cf.at(CF_TOD8AprEnergy, y) = 0;
		m_cf.at(CF_TOD8MayEnergy, y) = 0;
		m_cf.at(CF_TOD8JunEnergy, y) = 0;
		m_cf.at(CF_TOD8JulEnergy, y) = 0;
		m_cf.at(CF_TOD8AugEnergy, y) = 0;
		m_cf.at(CF_TOD8SepEnergy, y) = 0;
		m_cf.at(CF_TOD8OctEnergy, y) = 0;
		m_cf.at(CF_TOD8NovEnergy, y) = 0;
		m_cf.at(CF_TOD8DecEnergy, y) = 0;

		m_cf.at(CF_TOD9JanEnergy, y) = 0;
		m_cf.at(CF_TOD9FebEnergy, y) = 0;
		m_cf.at(CF_TOD9MarEnergy, y) = 0;
		m_cf.at(CF_TOD9AprEnergy, y) = 0;
		m_cf.at(CF_TOD9MayEnergy, y) = 0;
		m_cf.at(CF_TOD9JunEnergy, y) = 0;
		m_cf.at(CF_TOD9JulEnergy, y) = 0;
		m_cf.at(CF_TOD9AugEnergy, y) = 0;
		m_cf.at(CF_TOD9SepEnergy, y) = 0;
		m_cf.at(CF_TOD9OctEnergy, y) = 0;
		m_cf.at(CF_TOD9NovEnergy, y) = 0;
		m_cf.at(CF_TOD9DecEnergy, y) = 0;

		int i = 0;
		for (int m = 0; m<12; m++)
		{
			for (size_t d = 0; d<util::nday[m]; d++)
			{
				for (int h = 0; h<24 && i<8760 && m * 24 + h<288; h++)
				{
					switch (m)
					{
					case 0:
						m_cf.at(CF_TODJanEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
						switch (m_periods[i])
						{
						case 1:
							m_cf.at(CF_TOD1JanEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 2:
							m_cf.at(CF_TOD2JanEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 3:
							m_cf.at(CF_TOD3JanEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 4:
							m_cf.at(CF_TOD4JanEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 5:
							m_cf.at(CF_TOD5JanEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 6:
							m_cf.at(CF_TOD6JanEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 7:
							m_cf.at(CF_TOD7JanEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 8:
							m_cf.at(CF_TOD8JanEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 9:
							m_cf.at(CF_TOD9JanEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						}
						break;
					case 1:
						m_cf.at(CF_TODFebEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
						switch (m_periods[i])
						{
						case 1:
							m_cf.at(CF_TOD1FebEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 2:
							m_cf.at(CF_TOD2FebEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 3:
							m_cf.at(CF_TOD3FebEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 4:
							m_cf.at(CF_TOD4FebEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 5:
							m_cf.at(CF_TOD5FebEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 6:
							m_cf.at(CF_TOD6FebEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 7:
							m_cf.at(CF_TOD7FebEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 8:
							m_cf.at(CF_TOD8FebEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 9:
							m_cf.at(CF_TOD9FebEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						}
						break;
					case 2:
						m_cf.at(CF_TODMarEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
						switch (m_periods[i])
						{
						case 1:
							m_cf.at(CF_TOD1MarEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 2:
							m_cf.at(CF_TOD2MarEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 3:
							m_cf.at(CF_TOD3MarEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 4:
							m_cf.at(CF_TOD4MarEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 5:
							m_cf.at(CF_TOD5MarEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 6:
							m_cf.at(CF_TOD6MarEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 7:
							m_cf.at(CF_TOD7MarEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 8:
							m_cf.at(CF_TOD8MarEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 9:
							m_cf.at(CF_TOD9MarEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						}
						break;
					case 3:
						m_cf.at(CF_TODAprEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
						switch (m_periods[i])
						{
						case 1:
							m_cf.at(CF_TOD1AprEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 2:
							m_cf.at(CF_TOD2AprEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 3:
							m_cf.at(CF_TOD3AprEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 4:
							m_cf.at(CF_TOD4AprEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 5:
							m_cf.at(CF_TOD5AprEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 6:
							m_cf.at(CF_TOD6AprEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 7:
							m_cf.at(CF_TOD7AprEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 8:
							m_cf.at(CF_TOD8AprEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 9:
							m_cf.at(CF_TOD9AprEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						}
						break;
					case 4:
						m_cf.at(CF_TODMayEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
						switch (m_periods[i])
						{
						case 1:
							m_cf.at(CF_TOD1MayEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 2:
							m_cf.at(CF_TOD2MayEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 3:
							m_cf.at(CF_TOD3MayEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 4:
							m_cf.at(CF_TOD4MayEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 5:
							m_cf.at(CF_TOD5MayEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 6:
							m_cf.at(CF_TOD6MayEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 7:
							m_cf.at(CF_TOD7MayEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 8:
							m_cf.at(CF_TOD8MayEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 9:
							m_cf.at(CF_TOD9MayEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						}
						break;
					case 5:
						m_cf.at(CF_TODJunEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
						switch (m_periods[i])
						{
						case 1:
							m_cf.at(CF_TOD1JunEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 2:
							m_cf.at(CF_TOD2JunEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 3:
							m_cf.at(CF_TOD3JunEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 4:
							m_cf.at(CF_TOD4JunEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 5:
							m_cf.at(CF_TOD5JunEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 6:
							m_cf.at(CF_TOD6JunEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 7:
							m_cf.at(CF_TOD7JunEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 8:
							m_cf.at(CF_TOD8JunEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 9:
							m_cf.at(CF_TOD9JunEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						}
						break;
					case 6:
						m_cf.at(CF_TODJulEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
						switch (m_periods[i])
						{
						case 1:
							m_cf.at(CF_TOD1JulEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 2:
							m_cf.at(CF_TOD2JulEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 3:
							m_cf.at(CF_TOD3JulEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 4:
							m_cf.at(CF_TOD4JulEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 5:
							m_cf.at(CF_TOD5JulEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 6:
							m_cf.at(CF_TOD6JulEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 7:
							m_cf.at(CF_TOD7JulEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 8:
							m_cf.at(CF_TOD8JulEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 9:
							m_cf.at(CF_TOD9JulEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						}
						break;
					case 7:
						m_cf.at(CF_TODAugEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
						switch (m_periods[i])
						{
						case 1:
							m_cf.at(CF_TOD1AugEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 2:
							m_cf.at(CF_TOD2AugEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 3:
							m_cf.at(CF_TOD3AugEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 4:
							m_cf.at(CF_TOD4AugEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 5:
							m_cf.at(CF_TOD5AugEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 6:
							m_cf.at(CF_TOD6AugEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 7:
							m_cf.at(CF_TOD7AugEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 8:
							m_cf.at(CF_TOD8AugEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 9:
							m_cf.at(CF_TOD9AugEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						}
						break;
					case 8:
						m_cf.at(CF_TODSepEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
						switch (m_periods[i])
						{
						case 1:
							m_cf.at(CF_TOD1SepEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 2:
							m_cf.at(CF_TOD2SepEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 3:
							m_cf.at(CF_TOD3SepEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 4:
							m_cf.at(CF_TOD4SepEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 5:
							m_cf.at(CF_TOD5SepEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 6:
							m_cf.at(CF_TOD6SepEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 7:
							m_cf.at(CF_TOD7SepEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 8:
							m_cf.at(CF_TOD8SepEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 9:
							m_cf.at(CF_TOD9SepEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						}
						break;
					case 9:
						m_cf.at(CF_TODOctEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
						switch (m_periods[i])
						{
						case 1:
							m_cf.at(CF_TOD1OctEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 2:
							m_cf.at(CF_TOD2OctEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 3:
							m_cf.at(CF_TOD3OctEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 4:
							m_cf.at(CF_TOD4OctEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 5:
							m_cf.at(CF_TOD5OctEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 6:
							m_cf.at(CF_TOD6OctEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 7:
							m_cf.at(CF_TOD7OctEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 8:
							m_cf.at(CF_TOD8OctEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 9:
							m_cf.at(CF_TOD9OctEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						}
						break;
					case 10:
						m_cf.at(CF_TODNovEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
						switch (m_periods[i])
						{
						case 1:
							m_cf.at(CF_TOD1NovEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 2:
							m_cf.at(CF_TOD2NovEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 3:
							m_cf.at(CF_TOD3NovEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 4:
							m_cf.at(CF_TOD4NovEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 5:
							m_cf.at(CF_TOD5NovEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 6:
							m_cf.at(CF_TOD6NovEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 7:
							m_cf.at(CF_TOD7NovEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 8:
							m_cf.at(CF_TOD8NovEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 9:
							m_cf.at(CF_TOD9NovEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						}
						break;
					case 11:
						m_cf.at(CF_TODDecEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
						switch (m_periods[i])
						{
						case 1:
							m_cf.at(CF_TOD1DecEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 2:
							m_cf.at(CF_TOD2DecEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 3:
							m_cf.at(CF_TOD3DecEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 4:
							m_cf.at(CF_TOD4DecEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 5:
							m_cf.at(CF_TOD5DecEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 6:
							m_cf.at(CF_TOD6DecEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 7:
							m_cf.at(CF_TOD7DecEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 8:
							m_cf.at(CF_TOD8DecEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						case 9:
							m_cf.at(CF_TOD9DecEnergy, y) += m_hourly_energy[(y - 1) * 8760 + i];
							break;
						}
						break;
					}
					i++;
				}
			}
		}

	}

	return true;
}

/*   VARTYPE           DATATYPE         NAME                               LABEL                                       UNITS     META                                     GROUP                 REQUIRED_IF                 CONSTRAINTS                      UI_HINTS*/
/*
var_info vtab_advanced_financing_cost[] = {

{ SSC_INPUT, SSC_NUMBER, "total_direct_cost", "Total Direct Cost", "", "", "advanced_financing_costs", "*", "", "" },
{ SSC_OUTPUT, SSC_NUMBER, "direct_cost", "Total Direct Cost", "", "", "advanced_financing_cost", "*", "", "" },
{ SSC_OUTPUT, SSC_NUMBER, "indirect_cost", "Total Indirect Cost", "", "", "advanced_financing_cost", "*", "", "" },
{ SSC_OUTPUT, SSC_NUMBER, "financing_cost", "Total Financing Cost", "", "", "advanced_financing_cost", "*", "", "" },

	var_info_invalid };


advanced_financing_cost::advanced_financing_cost(compute_module *cm)
: m_cm(cm)
{
}


bool advanced_financing_cost::compute_cost(double cost_installed, double equity, double debt, double cbi, double ibi)
{
	double installed_cost = m_cm->as_double("total_installed_cost");
	double direct_cost = m_cm->as_double("total_direct_cost");
	double indirect_cost = installed_cost - direct_cost;
	double project_cost = cost_installed;

	m_cm->assign("direct_cost", var_data((ssc_number_t)direct_cost));
	m_cm->assign("indirect_cost", var_data((ssc_number_t)indirect_cost));
	m_cm->assign("financing_cost", var_data((ssc_number_t)(project_cost - indirect_cost - direct_cost)));

	// rounding on debt/equity
//	if (project_cost != (ibi + cbi + debt + equity))
//	{
		m_cm->assign("size_of_equity", var_data((ssc_number_t)(project_cost - ibi - cbi - debt)));
//	}
	return true;
}
*/

bool hourly_energy_calculation::calculate(compute_module *cm, bool heat)
{
	if (!cm) return false;

	m_cm = cm;

	m_nyears = m_cm->as_integer("analysis_period");


	ssc_number_t *pgen;
    size_t nrec_gen = 0;
    m_step_per_hour_gen = 1;
    if (heat)
        pgen = m_cm->as_array("gen_heat", &nrec_gen); // kWht
    else
        pgen = m_cm->as_array("gen", &nrec_gen);

	// in front of meter - account for charging and
	size_t i;
	ssc_number_t *revenue_gen = m_cm->allocate("revenue_gen", nrec_gen);
	ssc_number_t *gen_purchases = m_cm->allocate("gen_purchases", nrec_gen);

	// we do this so that grid energy purchased through the electricity rate is not inadvertently double counted as lost revenue
	for (i = 0; i < nrec_gen; i++) {
        gen_purchases[i] = std::min(pgen[i], 0.0);
		revenue_gen[i] = std::max(pgen[i], 0.0);
	}

	// for lifetime analysis
	size_t nrec_gen_per_year = nrec_gen;
	if (m_cm->as_integer("system_use_lifetime_output") == 1)
		nrec_gen_per_year = nrec_gen / m_nyears;
    m_step_per_hour_gen = nrec_gen_per_year / 8760;
	if (m_step_per_hour_gen < 1 || m_step_per_hour_gen > 60 || m_step_per_hour_gen * 8760 != nrec_gen_per_year)
	{
		m_error = util::format("invalid number of gen records (%d): must be an integer multiple of 8760", (int)nrec_gen_per_year);
		throw exec_error("hourly_energy_calculation", m_error);
	}
    m_ts_hour_gen = 1.0f / m_step_per_hour_gen;

	m_hourly_energy.clear();
    m_energy_sales.clear();
    m_energy_purchases.clear();
    m_energy_without_battery.clear();

    sum_ts_to_hourly(pgen, m_hourly_energy);
    sum_ts_to_hourly(revenue_gen, m_energy_sales);
    sum_ts_to_hourly(gen_purchases, m_energy_purchases);

    if (cm->is_assigned("gen_without_battery")) {
        ssc_number_t* gen_without_battery = m_cm->as_array("gen_without_battery", &nrec_gen);
        if (nrec_gen % 8760 == 0) {
            sum_ts_to_hourly(gen_without_battery, m_energy_without_battery);
        }
    }

	return true;
}

void hourly_energy_calculation::sum_ts_to_hourly(ssc_number_t* timestep_power, std::vector<double>& hourly)
{
    size_t idx = 0;
	ssc_number_t ts_power = 0;

    if (m_cm->as_integer("system_use_lifetime_output") == 1)
    {   // availability, curtailment and degradation included in lifetime output
        for (size_t y = 0; y < m_nyears; y++)
        {
            for (size_t i = 0; i < 8760; i++)
            {
                double hourly_energy = 0;
                for (size_t ii = 0; ii < m_step_per_hour_gen; ii++)
                {
                    ts_power = timestep_power[idx];
                    hourly_energy += ts_power * m_ts_hour_gen;
                    idx++;
                }
                hourly.push_back(hourly_energy);
            }
        }
        // check size
        if (hourly.size() != 8760 * m_nyears)
        {
            m_error = util::format("invalid number of hourly energy records (%d): must be %d", (int)hourly.size(), 8760 * m_nyears);
            throw exec_error("hourly_energy_calculation", m_error);
        }
    }
    else
    {
        for (size_t i = 0; i < 8760; i++)
        {
            double hourly_energy = 0;
            for (size_t ii = 0; ii < m_step_per_hour_gen; ii++)
            {
                ts_power = timestep_power[idx];
                hourly_energy += ts_power * m_ts_hour_gen;
                idx++;
            }
            hourly.push_back(hourly_energy);
        }
        // check size
        if (m_hourly_energy.size() != 8760)
        {
            m_error = util::format("invalid number of hourly energy records (%d): must be 8760", (int)hourly.size());
            throw exec_error("hourly_energy_calculation", m_error);
        }
    }
}

void check_financial_metrics::check_irr(compute_module* cm, ssc_number_t& irr)
{
    if (std::isnan(irr))
        cm->log("IRR at end of analysis period is not a number (NaN). This can indicate that revenues are too low to cover costs, or that they are excessively high compared to costs.", SSC_WARNING);
    if (irr > 50.0)
        cm->log(util::format("IRR at end of analysis period is %lg%%. A high IRR may indicate a project with unrealistically high returns.", irr), SSC_WARNING);
}

void check_financial_metrics::check_irr_flip(compute_module* cm, ssc_number_t& irr)
{
    if (std::isnan(irr))
        cm->log("IRR in target year is not a number (NaN). This can indicate that revenues are too low to cover costs, or that they are excessively high compared to costs.", SSC_WARNING);
    if (irr > 50.0)
        cm->log(util::format("IRR in target year is %lg%%. A high IRR may indicate a project with unrealistically high returns.", irr), SSC_WARNING);
}

void check_financial_metrics::check_npv(compute_module* cm, ssc_number_t& npv_metric)
{
    if (npv_metric < 0.0)
        cm->log(util::format("NPV is $%lg. A negative NPV indicates project costs are higher than revenues.", npv_metric), SSC_WARNING);
}

void check_financial_metrics::check_debt_percentage(compute_module* cm, ssc_number_t& debt_percentage)
{
    if (debt_percentage > 100.0)
        cm->log(util::format("Debt percent is %lg. A debt percent greater than 100%% may indicate that revenues are higher than necessary to cover project costs.", debt_percentage), SSC_WARNING);
    if (debt_percentage < 0.0)
        cm->log(util::format("Debt percent is %lg. A debt percent less than 0%% may indicate the mininum EBITDA cannot support any debt in at least one year.", debt_percentage), SSC_WARNING);
}



var_info vtab_tod_dispatch_periods[] = {
    // TOD factors
        { SSC_INPUT, SSC_NUMBER, "ppa_multiplier_model", "PPA multiplier model", "0/1", "0=diurnal,1=timestep", "Revenue", "?=0", "INTEGER,MIN=0", "" },
        { SSC_INPUT, SSC_ARRAY, "dispatch_factors_ts", "Dispatch payment factor array", "", "", "Revenue", "ppa_multiplier_model=1", "", "" },

        { SSC_OUTPUT, SSC_ARRAY, "ppa_multipliers", "TOD factors", "", "", "Revenue", "*", "", "" },
        { SSC_INPUT,        SSC_ARRAY,      "dispatch_tod_factors",                   "TOD factors for periods 1 through 9",                           "",   "",                          "Revenue",             "ppa_multiplier_model=0",						   "",                 "" },
        { SSC_INPUT,        SSC_MATRIX,     "dispatch_sched_weekday",                 "Diurnal weekday TOD periods",                                   "1..9", "12 x 24 matrix",    "Revenue", "ppa_multiplier_model=0", "", "" },
        { SSC_INPUT,        SSC_MATRIX,     "dispatch_sched_weekend",                 "Diurnal weekend TOD periods",                                   "1..9", "12 x 24 matrix",    "Revenue", "ppa_multiplier_model=0", "", "" },
        { SSC_INPUT,        SSC_NUMBER,     "start_day_of_year",                      "Start day of year for TOD periods",                             "0..6", "0=Monday, 6=Sunday",    "Revenue", "?=0", "", "" },
        var_info_invalid };

var_info vtab_update_tech_outputs[] = {
    { SSC_INOUT,        SSC_ARRAY,      "batt_annual_charge_from_system",                 "Battery annual energy charged from system",                 "kWh",      "",                      "LCOS",       "",                           "",                               "" },
    { SSC_INOUT,        SSC_ARRAY,      "batt_annual_discharge_energy",               "Battery annual energy discharged",                      "kWh",      "",                      "LCOS",       "",                           "",                               "" },
    { SSC_INOUT,        SSC_ARRAY,      "batt_annual_charge_energy",               "Battery annual energy charged",                      "kWh",      "",                      "LCOS",       "",                           "",                               "" },

    { SSC_INOUT,       SSC_ARRAY,      "annual_fuel_usage_lifetime",            "Annual Fuel Usage (lifetime)",               "kWht",          "",      "Fuel Cell",           "",               "",                    "" },
    { SSC_INOUT,       SSC_ARRAY,       "fuelcell_annual_energy_discharged",  "Fuel cell annual energy discharged",    "kWh",        "",                 "Fuel Cell",                  "",                        "",                              "" },
    { SSC_INOUT,       SSC_ARRAY,       "fuelcell_replacement",                "Fuel cell replacements per year",      "number/year", "",              "Fuel Cell",           "",                           "",                              "" },
    var_info_invalid
};

var_info vtab_lcos_inputs[] = {
    /*   VARTYPE           DATATYPE         NAME                             LABEL                                UNITS      META                 GROUP          REQUIRED_IF                 CONSTRAINTS                      UI_HINTS*/
    { SSC_INPUT,        SSC_NUMBER,     "batt_salvage_percentage",                     "Net pre-tax cash battery salvage value",	                               "%",	 "",					  "LCOS",             "?=0",                     "MIN=0,MAX=100",      			"" },

    { SSC_INPUT,        SSC_NUMBER,      "battery_total_cost_lcos",               "Battery total investment cost",                      "$",      "",                      "LCOS",       "",                           "",                               "" },
    { SSC_INPUT,        SSC_ARRAY,      "grid_to_batt",                               "Electricity to grid from battery",                      "kW",      "",                       "LCOS",       "",                           "",                              "" },
    { SSC_INPUT, SSC_ARRAY,             "year1_monthly_ec_charge_with_system", "Energy charge with system", "$", "", "LCOS", "", "", "" },
    { SSC_INPUT, SSC_ARRAY,             "year1_monthly_ec_charge_gross_with_system", "Energy charge with system before credits", "$/mo", "", "LCOS", "", "LENGTH=12", "" },
    { SSC_INPUT,       SSC_ARRAY,      "year1_monthly_electricity_to_grid",    "Electricity to/from grid",           "kWh/mo", "", "LCOS",          "",                         "LENGTH=12",                     "" },
    { SSC_INPUT, SSC_MATRIX,           "charge_w_sys_ec_ym", "Energy charge with system", "$", "", "LCOS", "", "", "COL_LABEL=MONTHS,FORMAT_SPEC=CURRENCY,GROUP=UR_AM" },
    { SSC_INPUT, SSC_MATRIX,           "true_up_credits_ym",     "Net annual true-up payments", "$", "", "LCOS", "", "", "COL_LABEL=MONTHS,FORMAT_SPEC=CURRENCY,GROUP=UR_AM" },
    { SSC_INPUT, SSC_MATRIX, "nm_dollars_applied_ym", "Net metering credit", "$", "", "Charges by Month", "", "", "COL_LABEL=MONTHS,FORMAT_SPEC=CURRENCY,GROUP=UR_AM" },
    { SSC_INPUT, SSC_MATRIX, "net_billing_credits_ym", "Net billing credit", "$", "", "Charges by Month", "", "", "COL_LABEL=MONTHS,FORMAT_SPEC=CURRENCY,GROUP=UR_AM" },

    // fix for running financial compute modules tests
    { SSC_INOUT,       SSC_ARRAY,       "gen_purchases",                              "Electricity from grid to system",                         "kW",      "",                       "System Output",       "",                           "",                              "" },
    { SSC_INPUT,        SSC_ARRAY,      "rate_escalation",          "Annual electricity rate escalation",   "%/year",   "",                      "Electricity Rates",       "",              "",                             "" },

    
    { SSC_INPUT,        SSC_ARRAY,      "batt_capacity_percent",                      "Battery relative capacity to nameplate",                 "%",        "",                     "LCOS",       "",                           "",                              "" },
    { SSC_INPUT,        SSC_ARRAY,      "monthly_grid_to_batt",                       "Energy to battery from grid",                           "kWh",      "",                      "LCOS",       "",                          "LENGTH=12",                     "" },
    { SSC_INPUT,        SSC_ARRAY,      "monthly_batt_to_grid",                       "Energy to grid from battery",                           "kWh",      "",                      "LCOS",       "",                          "LENGTH=12",                     "" },
    { SSC_INPUT,        SSC_ARRAY,      "monthly_grid_to_load",                       "Energy to load from grid",                              "kWh",      "",                      "LCOS",       "",                          "LENGTH=12",                     "" },
    { SSC_INPUT,        SSC_ARRAY,      "monthly_system_to_grid",                     "Energy to grid from system",                            "kWh",      "",                      "LCOS",       "",                          "LENGTH=12",                     "" },
    { SSC_OUTPUT, SSC_ARRAY, "cf_annual_cost_lcos", "Annual storage costs", "$", "", "LCOS calculations", "", "LENGTH_EQUAL=cf_length", "" },
    { SSC_OUTPUT, SSC_ARRAY, "cf_annual_discharge_lcos", "Annual storage discharge", "kWh", "", "LCOS calculations", "", "LENGTH_EQUAL=cf_length", "" },
    { SSC_OUTPUT, SSC_ARRAY, "cf_charging_cost_grid", "Annual cost to charge from grid", "$", "", "LCOE calculations", "", "LENGTH_EQUAL=cf_length", "" },
    { SSC_OUTPUT, SSC_ARRAY, "cf_charging_cost_grid_month", "Annual cost to charge from grid (monthly)", "$", "", "LCOE calculations", "", "LENGTH_EQUAL=cf_length", "" },

    { SSC_OUTPUT, SSC_ARRAY, "cf_charging_cost_pv", "Annual cost to charge from system", "$", "", "LCOE calculations", "", "LENGTH_EQUAL=cf_length", "" },
    { SSC_OUTPUT, SSC_ARRAY, "cf_om_batt_capacity_expense", "Annual cost for battery capacity based maintenance", "$", "", "LCOE calculations", "", "LENGTH_EQUAL=cf_length", "" },
    { SSC_OUTPUT, SSC_ARRAY, "cf_om_batt_fixed_expense", "Annual fixed cost for battery maintenance", "$", "", "LCOE calculations", "", "LENGTH_EQUAL=cf_length", "" },
    { SSC_OUTPUT, SSC_ARRAY, "cf_salvage_cost_lcos", "Annual battery salvage value costs", "$", "", "LCOE calculations", "", "LENGTH_EQUAL=cf_length", "" },
    { SSC_OUTPUT, SSC_ARRAY, "cf_util_escal_rate", "Utility escalation rate", "", "", "LCOE calculations", "", "LENGTH_EQUAL=cf_length", "" },

    { SSC_OUTPUT, SSC_NUMBER, "npv_annual_costs_lcos", "Present value of annual storage costs", "$", "", "LCOE calculations", "", "", "" },
    { SSC_OUTPUT, SSC_NUMBER, "npv_energy_lcos_real", "Present value of annual stored energy (real)", "kWh", "", "LCOE calculations", "", "", "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "lcos_nom",                        "LCOS Levelized cost of storage nominal",              "cents/kWh",                   "", "Metrics", "", "", "" },
    { SSC_OUTPUT,       SSC_NUMBER,     "lcos_real",                        "LCOS Levelized cost of storage real",              "cents/kWh",                   "", "Metrics", "", "", "" },
    { SSC_OUTPUT, SSC_NUMBER, "npv_energy_lcos_nom", "Present value of annual stored energy (nominal)", "kWh", "", "LCOE calculations", "", "", "" },
    var_info_invalid };

double npv(int cf_line, int nyears, double rate, util::matrix_t<double> cf)
{
    //if (rate == -1.0) throw general_error("cannot calculate NPV with discount rate equal to -1.0");
    double rr = 1.0;
    if (rate != -1.0) rr = 1.0 / (1.0 + rate);
    double result = 0;
    for (int i = nyears; i > 0; i--)
        result = rr * result + cf.at(cf_line, i);

    return result * rr;
}

void escal_or_annual(int cf_line, int nyears, const std::string& variable,
    double inflation_rate, double scale, util::matrix_t<double> cf, compute_module* cm, bool as_rate = true, double escal = 0.0)
{
    size_t count;
    ssc_number_t* arrp = cm->as_array(variable, &count);
    double check;
    if (as_rate)
    {
        if (count == 1)
        {
            escal = inflation_rate + scale * arrp[0];
            for (int i = 0; i < nyears; i++) {
                cf.at(cf_line, i + 1) = pow(1 + escal, i);
                check = cf.at(cf_line, i + 1);
            }
        }
        else
        {
            for (int i = 0; i < nyears && i < (int)count; i++)
                cf.at(cf_line, i + 1) = 1 + arrp[i] * scale;
        }
    }
    else
    {
        if (count == 1)
        {
            for (int i = 0; i < nyears; i++)
                cf.at(cf_line, i + 1) = arrp[0] * scale * pow(1 + escal + inflation_rate, i);
        }
        else
        {
            for (int i = 0; i < nyears && i < (int)count; i++)
                cf.at(cf_line, i + 1) = arrp[i] * scale;
        }
    }
}

void save_cf(int cf_line, int nyears, const std::string& name, util::matrix_t<double> cf, compute_module* cm)
{
    ssc_number_t* arrp = cm->allocate(name, nyears + 1);
    for (int i = 0; i <= nyears; i++)
        arrp[i] = (ssc_number_t)cf.at(cf_line, i);
}

void lcos_calc(compute_module* cm, util::matrix_t<double> cf, int nyears, double nom_discount_rate, double inflation_rate, double lcoe_real, double total_cost, double real_discount_rate, int grid_charging_cost_version) {
    enum {
        CF_battery_replacement_cost_lcos,
        CF_battery_replacement_cost_schedule_lcos,
        CF_ppa_price_lcos,
        CF_om_fixed_expense_lcos,
        CF_om_production_expense_lcos,
        CF_om_capacity_expense_lcos,
        CF_om_fixed1_expense_lcos,
        CF_om_production1_expense_lcos,
        CF_om_capacity1_expense_lcos,
        CF_om_fixed2_expense_lcos,
        CF_om_production2_expense_lcos,
        CF_om_capacity2_expense_lcos,
        CF_om_fuel_expense_lcos,
        CF_energy_charged_grid_lcos,
        CF_energy_charged_pv_lcos,
        CF_energy_discharged_lcos,
        CF_charging_cost_pv_lcos,
        CF_charging_cost_grid_lcos,
        CF_om_cost_lcos_lcos,
        CF_salvage_cost_lcos_lcos,
        CF_investment_cost_lcos_lcos,
        CF_annual_cost_lcos_lcos,
        CF_util_escal_rate_lcos,
        CF_degradation_lcos,

    };

    if (cm->is_assigned("battery_total_cost_lcos") && cm->as_double("battery_total_cost_lcos") != 0) { //Check that there are non-zero battery costs from the UI
        double lcos_investment_cost = cm->as_double("battery_total_cost_lcos"); //Battery capital costs ($), does not include replacement costs
        std::vector<double> charged_pv = cm->as_vector_double("batt_annual_charge_from_system"); //Annual kwh charged from PV or generic system (nyears array)
        std::vector<double> charged_total = cm->as_vector_double("batt_annual_charge_energy"); //Total annual kwh charged by battery system (nyears array)
        std::vector<double> lcos_energy_discharged = cm->as_vector_double("batt_annual_discharge_energy"); //Total annual kwh discharged by battery system (nyears array)
        size_t n_grid_to_batt, n_monthly_grid_to_batt, n_monthly_grid_to_load, n_monthly_energy_charge, n_net_annual_true_up; //Preallocate size variables
        ssc_number_t* grid_to_batt = cm->as_array("grid_to_batt", &n_grid_to_batt); //Power from grid to battery in kW hourly
        ssc_number_t* monthly_grid_to_batt = cm->as_array("monthly_grid_to_batt", &n_monthly_grid_to_batt);

        ssc_number_t* monthly_batt_to_grid;
        if (grid_charging_cost_version != 0) //For FOM systems the battery exports to grid rather than to load
            monthly_batt_to_grid = cm->as_array("monthly_batt_to_grid", &n_monthly_grid_to_load); //monthly battery exports to grid for 1st year (kWh)
        ssc_number_t* monthly_grid_to_load;
        if (grid_charging_cost_version == 0) //For BTM systems the battery exports to load
            monthly_grid_to_load = cm->as_array("monthly_grid_to_load", &n_monthly_grid_to_load);//monthly battery exports to load for first year (kWh)

        size_t n_batt_capacity_percent;
        ssc_number_t* batt_capacity_percent = cm->as_array("batt_capacity_percent", &n_batt_capacity_percent); //battery capacity relative to nameplate (%)


        util::matrix_t<double> monthly_energy_charge; //monthly energy charges at 12 month x nyears matrix ($)
        util::matrix_t<double> net_annual_true_up; //net annual true up payments as 12 month x nyears matrix ($)
        size_t n_steps_per_year = 8760; //Initialize number of timesteps per year (calculated based on lifetime choice and nstep of weather file
        //ssc_number_t* monthly_batt_to_grid = as_array("monthly_batt_to_grid", &n_monthly_grid_to_load);
        ssc_number_t* monthly_system_to_grid = cm->as_array("monthly_system_to_grid", &n_monthly_grid_to_load); //monthly grid energy bought to satisfy load for first year (kWh)
        ssc_number_t* monthly_electricity_tofrom_grid;
        bool ppa_purchases = !(cm->is_assigned("en_electricity_rates") && cm->as_number("en_electricity_rates") == 1); //Does the system use ppa purchases in revenue calculations (Single Owner only)
        if (!ppa_purchases || (grid_charging_cost_version == 0)) { //Are ppa purchases used in Single Owner or is the case using BTM financial model?
            monthly_electricity_tofrom_grid = cm->as_array("year1_monthly_electricity_to_grid", &n_monthly_grid_to_load); //Monthly electricity exported to the grid in the first year (kWh)
            monthly_energy_charge = cm->as_matrix("charge_w_sys_ec_ym"); //Use monthly energy charges from utility bill ($)
            net_annual_true_up = cm->as_matrix("true_up_credits_ym"); //Use net annual true up payments regardless of billing mode ($)
        }
        util::matrix_t<double> net_billing_credit; //net annual true up payments as 12 month x nyears matrix ($)
        util::matrix_t<double> net_metering_credit;
        size_t n_year1_hourly_ec;
        size_t n_year1_hourly_dc;
        ssc_number_t* year1_hourly_ec;
        ssc_number_t* year1_hourly_dc;
        size_t n_e_fromgrid;
        ssc_number_t* year1_hourly_e_from_grid;
        ssc_number_t* monthly_gen_purchases;
        ssc_number_t* monthly_e_fromgrid;
        int n_steps_per_hour = 0;
        if (grid_charging_cost_version == 0) {
            net_annual_true_up = cm->as_matrix("true_up_credits_ym"); //Use net annual true up payments regardless of billing mode ($)
            net_billing_credit = cm->as_matrix("net_billing_credits_ym"); //Use net annual true up payments regardless of billing mode ($)
            net_metering_credit = cm->as_matrix("nm_dollars_applied_ym");
            year1_hourly_ec = cm->as_array("year1_hourly_ec_with_system", &n_year1_hourly_ec);
            year1_hourly_dc = cm->as_array("year1_hourly_dc_with_system", &n_year1_hourly_dc);
            year1_hourly_e_from_grid = cm->as_array("year1_hourly_e_fromgrid", &n_e_fromgrid);
            monthly_gen_purchases = cm->allocate("monthly_gen_purchases", 12 * nyears);
            monthly_e_fromgrid = cm->allocate("monthly_e_from_grid", 12);
            n_steps_per_hour = n_grid_to_batt / (nyears * 8760);
        }

        size_t n_mp_market_price;
        size_t steps_per_hour;
        size_t steps_per_year;
        ssc_number_t* mp_market_price; //Market revenue for Merchant Plant model
        std::vector<ssc_number_t> grid_to_batt_mp;
        std::vector<ssc_number_t> extrapolated_grid_to_batt_mp;
        if (grid_charging_cost_version == 2) //Only if financial model is Merchant Plant
            mp_market_price = cm->as_array("mp_energy_market_price", &n_mp_market_price); //Energy market price (hourly) for lifetime array to calculate grid charging cost for Merchant Plant ($)

        size_t n_degradation;
        ssc_number_t* degradation = cm->as_array("degradation", &n_degradation);
        if (n_degradation == 1)
        {
            for (size_t i = 1; i <= nyears; i++) cf.at(CF_degradation_lcos, i) = pow((1.0 - degradation[0] / 100.0), i - 1);
        }
        else if (n_degradation > 0)
        {
            for (size_t i = 0; i < nyears && i < (int)n_degradation; i++) cf.at(CF_degradation_lcos, i + 1) = (1.0 - degradation[i] / 100.0);
        }

        if (cm->as_integer("system_use_lifetime_output") == 1)
            n_steps_per_year = n_grid_to_batt / nyears; //Steps per year = Lifetime array size / number years
        else
            n_steps_per_year = n_grid_to_batt; //If no lifetime, steps per year = first year array size
        //std::vector<double> grid_to_batt = as_vector_double("grid_to_batt");
        cf.at(CF_charging_cost_grid_lcos, 0) = 0; //Initialize year 0 charging cost to $0


        if (cm->is_assigned("rate_escalation")) { //Create rate escalation nyears array with inflation and specified rate escalation %
            size_t count;
            ssc_number_t* arrp = cm->as_array("rate_escalation", &count);
            double scale = 0.01; //%
            if (count == 1)
            {
                double escal = inflation_rate + scale * arrp[0];
                for (int i = 0; i < nyears; i++) {
                    cf.at(CF_util_escal_rate_lcos, i + 1) = pow(1 + escal, i);
                }
            }
            else
            {
                for (int i = 0; i < nyears && i < (int)count; i++)
                    cf.at(CF_util_escal_rate_lcos, i + 1) = 1 + arrp[i] * scale;
            }
            
        }
        save_cf(CF_util_escal_rate_lcos, nyears, "cf_util_escal_rate", cf, cm);

        double capex_lcoe_ratio = 1 / 0.8; //ratio of capex ratio between PV+batt / PV to LCOE ratio PV+batt/ PV (assumed based on table)
        double lcoe_real_lcos = lcoe_real * capex_lcoe_ratio * (total_cost - lcos_investment_cost) / total_cost; //cents/kWh
        //Using ration of investment cost to lcoe for system+storage and system only to approximate lcoe for system only; Used in PV charging cost calculations

        //Preallocate tod multiplier index
        size_t tod_mult_index = 0;
        ssc_number_t* tod_multipliers;
        size_t* n_tod_multipliers = 0;
        if (grid_charging_cost_version == 1) {
            if (cm->is_assigned("ppa_multiplier_model") && cm->as_integer("ppa_multiplier_model") == 0) {
                tod_multipliers = cm->as_array("ppa_multipliers", n_tod_multipliers);
            }
            else if (cm->is_assigned("ppa_multiplier_model") && cm->as_integer("ppa_multiplier_model") == 1) {
                tod_multipliers = cm->as_array("dispatch_factors_ts", n_tod_multipliers);
            }
        }

        for (int a = 0; a <= nyears; a++) { //Iterate through nyears of the project


            if (grid_charging_cost_version == 0) { //0 - Cashloan, BTM (Residential, Commercial, Third Party - Host Developer)
                if (cm->as_integer("system_use_lifetime_output") == 1) { //Lifetime 
                    for (size_t m = 1; m <= 12; m++) { //monthly iteration for each year
                        for (size_t d = 1; d <= util::days_in_month(int(m - 1)); d++) {
                            for (size_t h = 0; h < 24; h++) { //monthly iteration for each year
                                for (size_t n = 0; n < n_steps_per_hour; n++) {
                                    if (a == 0) monthly_e_fromgrid[m-1] += year1_hourly_e_from_grid[n_steps_per_hour * util::hour_of_year(m, d, h) + n];
                                    if (a != 0 && year1_hourly_e_from_grid[n_steps_per_hour * util::hour_of_year(m, d, h) + n] != 0.0) {
                                        cf.at(CF_charging_cost_grid_lcos, a) += grid_to_batt[(size_t(a) - 1) * 8760 * n_steps_per_hour + n_steps_per_hour * util::hour_of_year(m, d, h) + n] * cf.at(CF_degradation_lcos, a) /
                                            year1_hourly_e_from_grid[n_steps_per_hour * util::hour_of_year(m, d, h) + n] *
                                            (year1_hourly_ec[n_steps_per_hour * util::hour_of_year(m, d, h) + n] + year1_hourly_dc[n_steps_per_hour * util::hour_of_year(m, d, h) + n]) * cf.at(CF_util_escal_rate_lcos, a); //use the electricity rate data by year (also trueup) //* charged_grid[a] / charged_grid[1] * cf.at(CF_util_escal_rate, a);
                                    }
                                }
                            }
                        }
                        if (a != 0 && monthly_e_fromgrid[m - 1] > 0) cf.at(CF_charging_cost_grid_lcos, a) += -monthly_grid_to_batt[m - 1] / monthly_e_fromgrid[m - 1] * (net_annual_true_up.at(a, m - 1) + net_billing_credit.at(a, m - 1) + net_metering_credit.at(a, m - 1));
                    }
                }
                else {
                    for (size_t m = 1; m <= 12; m++) { //monthly iteration for each year
                        for (size_t d = 1; d <= util::days_in_month(int(m - 1)); d++) {
                            for (size_t h = 0; h < 24; h++) { //monthly iteration for each year
                                for (size_t n = 0; n < n_steps_per_hour; n++) {
                                    if (a == 1) monthly_e_fromgrid[m-1] += year1_hourly_e_from_grid[n_steps_per_hour * util::hour_of_year(m, d, h) + n];
                                    if (a != 0 && year1_hourly_e_from_grid[n_steps_per_hour * util::hour_of_year(m, d, h) + n] != 0.0) {
                                        cf.at(CF_charging_cost_grid_lcos, a) += grid_to_batt[n_steps_per_hour * util::hour_of_year(m, d, h) + n] * cf.at(CF_degradation_lcos, a) /
                                            year1_hourly_e_from_grid[n_steps_per_hour * util::hour_of_year(m, d, h) + n] *
                                            (year1_hourly_ec[n_steps_per_hour * util::hour_of_year(m, d, h) + n] + year1_hourly_dc[n_steps_per_hour * util::hour_of_year(m, d, h) + n]) * cf.at(CF_util_escal_rate_lcos, a); //use the electricity rate data by year (also trueup) //* charged_grid[a] / charged_grid[1] * cf.at(CF_util_escal_rate, a);
                                    }
                                    if (a != 0 && d == util::days_in_month(int(m - 1)) && h == 23) cf.at(CF_charging_cost_grid_lcos, a) += -monthly_grid_to_batt[m - 1] / monthly_e_fromgrid[m - 1] * (net_annual_true_up.at(a, m - 1) + net_billing_credit.at(a, m - 1) + net_metering_credit.at(a, m - 1));
                                }
                            }
                        }
                        if (a != 0 && monthly_e_fromgrid[m - 1] > 0) cf.at(CF_charging_cost_grid_lcos, a) += -monthly_grid_to_batt[m - 1] / monthly_e_fromgrid[m - 1] * (net_annual_true_up.at(a, m - 1) + net_billing_credit.at(a, m - 1) + net_metering_credit.at(a, m - 1));
                    }
                }
            }
            else if (grid_charging_cost_version == 1) { //Single Owner (FOM), other FOM systems (flip, leaseback)
                if (cm->as_integer("system_use_lifetime_output") == 1) //Lifetime
                {
                    //Lifetime calculations
                    //Calculate cost to charge battery from grid (either using ppa price or electricity rates if enabled)
                    double ppa_value = cf.at(CF_ppa_price_lcos, a); //PPA price at year a
                    if (ppa_purchases && a != 0) {
                        for (size_t h = 0; h < n_steps_per_year; h++) {
                            tod_mult_index = floor(h / (n_steps_per_year / 8760));
                            cf.at(CF_charging_cost_grid_lcos, a) += grid_to_batt[(size_t(a) - 1) * n_steps_per_year + h] * 8760 / n_steps_per_year * ppa_value / 100.0 * tod_multipliers[tod_mult_index]; //Grid charging cost from PPA price ($)
                        }
                    }
                    else if (!ppa_purchases && a != 0) {
                        for (size_t m = 0; m < 12; m++) {
                            if (((monthly_batt_to_grid[m] + monthly_system_to_grid[m]) + -monthly_electricity_tofrom_grid[m]) != 0)
                                cf.at(CF_charging_cost_grid_lcos, a) += monthly_grid_to_batt[m] / ((monthly_batt_to_grid[m] + monthly_system_to_grid[m]) + -monthly_electricity_tofrom_grid[m]) * monthly_energy_charge.at(a, m) + net_annual_true_up.at(a, m); //Grid charging cost from monthly energy charges and net annual true up ($

                        }
                    }


                }
                else //Not lifetime
                {
                    //Single year calculations (might not be needed as their is no Single Owner for PVWatts Battery
                    //Calculate cost to charge battery from grid (either using ppa price or electricity rates if enabled)
                    double ppa_value = cf.at(CF_ppa_price_lcos, a); //PPA price at year a ($)
                    if (ppa_purchases && a != 0) { //PPA purchases enabled and not in investment year
                        for (size_t h = 0; h < 8760; h++) {

                            tod_mult_index = floor(h / 8760);
                            cf.at(CF_charging_cost_grid_lcos, a) += grid_to_batt[h] * cf.at(CF_degradation_lcos, a) * ppa_value / 100.0 * tod_multipliers[h]; //Grid charging cost calculated from PPA price ($)
                        }
                    }
                    else if (!ppa_purchases && a != 0) { //No PPA purchases and not in investment year
                        for (size_t m = 0; m < 12; m++) {
                            if ((-monthly_electricity_tofrom_grid[m] + (monthly_batt_to_grid[m] + monthly_system_to_grid[m])) != 0)
                                cf.at(CF_charging_cost_grid_lcos, a) += monthly_grid_to_batt[m] / (-monthly_electricity_tofrom_grid[m] + (monthly_batt_to_grid[m] + monthly_system_to_grid[m])) * monthly_energy_charge.at(a, m) + net_annual_true_up.at(a, m); //Grid charging cost calculated from monthly energy charges at year a and net annual true ($)

                        }
                    }

                }
            }
            else if (grid_charging_cost_version == 2) { //Merchant Plant


                if (cm->as_integer("system_use_lifetime_output") == 1) //Lifetime
                {
                    if (ppa_purchases && a != 0) {
                        steps_per_hour = n_mp_market_price / (8760.0 * nyears);
                        grid_to_batt_mp.clear();
                        grid_to_batt_mp.reserve(n_steps_per_year);
                        for (size_t h = 0; h < n_steps_per_year; h++) {
                            grid_to_batt_mp.push_back(grid_to_batt[(a - 1) * n_steps_per_year + h]);
                        }
                        extrapolated_grid_to_batt_mp = extrapolate_timeseries(grid_to_batt_mp, steps_per_hour);
                        for (size_t h = 0; h < extrapolated_grid_to_batt_mp.size(); h++) {
                            if (a != 0) { //Not in investment year
                                cf.at(CF_charging_cost_grid_lcos, a) += extrapolated_grid_to_batt_mp[h] * 8760 / n_steps_per_year * mp_market_price[(a-1)*extrapolated_grid_to_batt_mp.size() + h] / (1000); //Grid charging cost from energy market price ($)
                            }
                        }
                    }
                    else if (!ppa_purchases && a != 0) {
                        for (size_t m = 0; m < 12; m++) {
                            if (((monthly_batt_to_grid[m] + monthly_system_to_grid[m]) + -monthly_electricity_tofrom_grid[m]) != 0)
                                cf.at(CF_charging_cost_grid_lcos, a) += monthly_grid_to_batt[m] / ((monthly_batt_to_grid[m] + monthly_system_to_grid[m]) + -monthly_electricity_tofrom_grid[m]) * monthly_energy_charge.at(a, m) + net_annual_true_up.at(a, m); //Grid charging cost from monthly energy charges and net annual true up ($

                        }
                    }

                }
                else //Not Lifetime
                {
                    if (ppa_purchases && a != 0) {
                        steps_per_hour = n_mp_market_price / (8760.0);
                        grid_to_batt_mp.clear();
                        grid_to_batt_mp.reserve(n_steps_per_year);
                        for (size_t h = 0; h < n_steps_per_year; h++) {
                            grid_to_batt_mp.push_back(grid_to_batt[h]);
                        }
                        extrapolated_grid_to_batt_mp = extrapolate_timeseries(grid_to_batt_mp, steps_per_hour);
                        for (size_t h = 0; h < extrapolated_grid_to_batt_mp.size(); h++) {
                            if (a != 0) { //Not in investment year
                                cf.at(CF_charging_cost_grid_lcos, a) += grid_to_batt[h] * cf.at(CF_degradation_lcos, a) * 8760.0 / n_steps_per_year * mp_market_price[(a-1)* extrapolated_grid_to_batt_mp.size() + h] / (1000); //Grid charging cost from energy market price ($)
                            }
                        }
                    }
                    else if (!ppa_purchases && a != 0) { //No PPA purchases and not in investment year
                        for (size_t m = 0; m < 12; m++) {
                            if ((-monthly_electricity_tofrom_grid[m] + (monthly_batt_to_grid[m] + monthly_system_to_grid[m])) != 0)
                                cf.at(CF_charging_cost_grid_lcos, a) += monthly_grid_to_batt[m] / (-monthly_electricity_tofrom_grid[m] + (monthly_batt_to_grid[m] + monthly_system_to_grid[m])) * monthly_energy_charge.at(a, m) + net_annual_true_up.at(a, m); //Grid charging cost calculated from monthly energy charges at year a and net annual true ($)

                        }
                    }
                }
            }

            if (cm->as_integer("system_use_lifetime_output") == 1 && a != 0) { //Lifetime
                cf.at(CF_charging_cost_pv_lcos, a) = charged_pv[a - 1] * lcoe_real_lcos / 100 * pow(1 + inflation_rate, a - 1); //Calculate system charging cost from year a system charged amount ($)
                cf.at(CF_energy_discharged_lcos, a) = lcos_energy_discharged[a - 1];

            }
            else if (cm->as_integer("system_use_lifetime_output") != 1 && a != 0) { //Not lifetime
                cf.at(CF_charging_cost_pv_lcos, a) = charged_pv[0] * cf.at(CF_degradation_lcos, a) * lcoe_real_lcos / 100 * pow(1 + inflation_rate, a - 1); //Calculate system charging cost from year 1 sytem charged amount ($) (Probably need to account for degradation)
                cf.at(CF_energy_discharged_lcos, a) = lcos_energy_discharged[0] * cf.at(CF_degradation_lcos, a); //Store energy discharged in year 1 to each year of the cash flow
            }
            cf.at(CF_annual_cost_lcos_lcos, a) = -cf.at(CF_charging_cost_grid_lcos, a) + //Grid charging cost +
                -cf.at(CF_charging_cost_pv_lcos, a) + -cf.at(CF_om_fixed1_expense_lcos, a) + //System charging cost + Fixed OM expense +
                -cf.at(CF_om_capacity1_expense_lcos, a) + -cf.at(CF_om_production1_expense_lcos, a) + //Capacity based OM expense + Production based OM expense
                -cf.at(CF_battery_replacement_cost_lcos, a); //Battery replacement expense

        } //End annual iteration
        cf.at(CF_energy_discharged_lcos, 0) = 0; //Initialize year 0 of energy discharged to 0 for investment year
        cf.at(CF_annual_cost_lcos_lcos, 0) += -lcos_investment_cost; //add initial investment to year 0
        double batt_salvage_value_frac = cm->as_double("batt_salvage_percentage") * 0.01; //Battery salvage percentage of initial battery capital cost turned to fraction
        double lcos_salvage_value = cf.at(CF_battery_replacement_cost_schedule_lcos, nyears) * batt_capacity_percent[n_batt_capacity_percent - 1] / 100 * batt_salvage_value_frac; //set as a percentage or direct salvage value //Calculate salvage value of batter only
        cf.at(CF_salvage_cost_lcos_lcos, nyears) = lcos_salvage_value; //Store salvage value in cash flow
        cf.at(CF_annual_cost_lcos_lcos, nyears) -= cf.at(CF_salvage_cost_lcos_lcos, nyears); //Add salvage value to overall LCOS cash flow
        double lcos_denominator = npv(CF_energy_discharged_lcos, nyears, nom_discount_rate, cf); //Find npv of battery energy discharged to use as denominator of LCOS equation (kWh)
        double lcos_denominator_real = npv(CF_energy_discharged_lcos, nyears, real_discount_rate, cf); //Find npv of battery energy discharged with real discount rate to use as denominator of LCOS equation (kWh)
        double lcos_numerator = -(npv(CF_annual_cost_lcos_lcos, nyears, nom_discount_rate, cf)) - cf.at(CF_annual_cost_lcos_lcos, 0); //Find npv of battery costs for numerator in LCOS equation ($)
        cm->assign("npv_annual_costs_lcos", var_data((ssc_number_t)lcos_numerator)); //Store battery cost NPV in outputs
        save_cf(CF_annual_cost_lcos_lcos, nyears, "cf_annual_cost_lcos", cf, cm); //Store batterys costs in each year in cash flow
        save_cf(CF_energy_discharged_lcos, nyears, "cf_annual_discharge_lcos", cf, cm); //Store battery energy discharge in each year in cash flow
        save_cf(CF_charging_cost_grid_lcos, nyears, "cf_charging_cost_grid", cf, cm); //Store grid charging cost in each year in cash flow ($)
        save_cf(CF_charging_cost_pv_lcos, nyears, "cf_charging_cost_pv", cf, cm); //Store system charging cost in each year in cash flow ($)
        save_cf(CF_salvage_cost_lcos_lcos, nyears, "cf_salvage_cost_lcos", cf, cm); //Store salvage value cost in each year in cash flow ($)
        double lcos_nom = lcos_numerator / lcos_denominator * 100.0; // cent/kWh Nominal LCOS
        double lcos_real = lcos_numerator / lcos_denominator_real * 100.0; // cents/kWh Real LCOS
        cm->assign("lcos_nom", var_data((ssc_number_t)lcos_nom)); //Store nominal LCOS in outputs
        cm->assign("lcos_real", var_data((ssc_number_t)lcos_real)); //Store real LCOS in outputs
        cm->assign("npv_energy_lcos_nom", var_data((ssc_number_t)lcos_denominator)); //Store Nominal LCOS denominator in outputs
        cm->assign("npv_energy_lcos_real", var_data((ssc_number_t)lcos_denominator_real)); //Store Real LCOS denominator in outputs
    }
    /////////////////////////////////////////////////////////////////////////////////////////
}

void update_battery_outputs(compute_module* cm, size_t nyears) {
    if (cm->as_integer("system_use_lifetime_output") == 1) {
        size_t arr_length = nyears + 1;
        ssc_number_t yr_0_value = 0.0;
        prepend_to_output(cm, "batt_bank_replacement", arr_length, yr_0_value);
        prepend_to_output(cm, "batt_annual_charge_energy", arr_length, yr_0_value);
        prepend_to_output(cm, "batt_annual_discharge_energy", arr_length, yr_0_value);
        prepend_to_output(cm, "batt_annual_charge_from_system", arr_length, yr_0_value);
    }
}

void update_fuelcell_outputs(compute_module* cm, size_t nyears) {
    if (cm->as_integer("system_use_lifetime_output") == 1) {
        size_t arr_length = nyears + 1;
        ssc_number_t yr_0_value = 0.0;
        prepend_to_output(cm, "fuelcell_replacement", arr_length, yr_0_value);
        prepend_to_output(cm, "annual_fuel_usage_lifetime", arr_length, yr_0_value);
        prepend_to_output(cm, "fuelcell_annual_energy_discharged", arr_length, yr_0_value);
    }
}

// CF enum for ITC and depreciation calculations
enum {
    CF_itc_fed_amount,
    CF_itc_sta_amount,
    CF_itc_fed_percent_fraction,
    CF_itc_sta_percent_fraction,
    CF_itc_sta_percent_maxvalue,
    CF_itc_fed_percent_maxvalue,

    CF_itc_sta_percent_amount,
    CF_itc_fed_percent_amount,
    CF_itc_fed,
    CF_itc_sta,
    CF_itc_total,

    CF_macrs_5_frac,
    CF_macrs_15_frac,
    CF_sl_5_frac,
    CF_sl_15_frac,
    CF_sl_20_frac,
    CF_sl_39_frac,
    CF_custom_frac,

    CF_stadepr_macrs_5,
    CF_stadepr_macrs_15,
    CF_stadepr_sl_5,
    CF_stadepr_sl_15,
    CF_stadepr_sl_20,
    CF_stadepr_sl_39,
    CF_stadepr_custom,

    CF_stadepr_me1,
    CF_stadepr_me2,
    CF_stadepr_me3,

    CF_stadepr_total,

    CF_feddepr_macrs_5,
    CF_feddepr_macrs_15,
    CF_feddepr_sl_5,
    CF_feddepr_sl_15,
    CF_feddepr_sl_20,
    CF_feddepr_sl_39,
    CF_feddepr_custom,

    CF_feddepr_me1,
    CF_feddepr_me2,
    CF_feddepr_me3,

    CF_feddepr_total,

    CF_max
};

itc_and_depreciation_calculations::itc_and_depreciation_calculations(compute_module* cm, size_t n_years) {

    nyears = n_years;

    m_cf.resize_fill(CF_max, nyears + 1, 0.0); 

    // SAM 1038
    // itc fixed
    itc_fed_amount = 0.0;
    double_vec vitc_fed_amount = cm->as_vector_double("itc_fed_amount");
    for (size_t k = 0; k < vitc_fed_amount.size() && k < nyears; k++) {
        m_cf.at(CF_itc_fed_amount, k + 1) = vitc_fed_amount[k];
        itc_fed_amount += vitc_fed_amount[k];
    }

    itc_sta_amount = 0.0;
    double_vec vitc_sta_amount = cm->as_vector_double("itc_sta_amount");
    for (size_t k = 0; k < vitc_sta_amount.size() && k < nyears; k++) {
        m_cf.at(CF_itc_sta_amount, k + 1) = vitc_sta_amount[k];
        itc_sta_amount += vitc_sta_amount[k];
    }

    // itc percent - max value used for comparison to qualifying costs
    double_vec vitc_fed_frac = cm->as_vector_double("itc_fed_percent");
    for (size_t k = 0; k < vitc_fed_frac.size(); k++)
        m_cf.at(CF_itc_fed_percent_fraction, k + 1) = vitc_fed_frac[k] * 0.01;
    
    double_vec vitc_sta_frac = cm->as_vector_double("itc_sta_percent");
    for (size_t k = 0; k < vitc_sta_frac.size(); k++)
        m_cf.at(CF_itc_sta_percent_fraction, k + 1) = vitc_sta_frac[k] * 0.01;
    

    double_vec itc_sta_percent_maxvalue = cm->as_vector_double("itc_sta_percent_maxvalue");
    if (itc_sta_percent_maxvalue.size() == 1) {
        for (size_t k = 0; k < nyears; k++)
            m_cf.at(CF_itc_sta_percent_maxvalue, k + 1) = itc_sta_percent_maxvalue[0];
    }
    else {
        for (size_t k = 0; k < itc_sta_percent_maxvalue.size() && k < nyears; k++)
            m_cf.at(CF_itc_sta_percent_maxvalue, k + 1) = itc_sta_percent_maxvalue[k];
    }

    double_vec itc_fed_percent_maxvalue = cm->as_vector_double("itc_fed_percent_maxvalue");
    if (itc_fed_percent_maxvalue.size() == 1) {
        for (size_t k = 0; k < nyears; k++)
            m_cf.at(CF_itc_fed_percent_maxvalue, k + 1) = itc_fed_percent_maxvalue[0];
    }
    else {
        for (size_t k = 0; k < itc_fed_percent_maxvalue.size() && k < nyears; k++)
            m_cf.at(CF_itc_fed_percent_maxvalue, k + 1) = itc_fed_percent_maxvalue[k];
    }

    bool init_method = cm->as_boolean("depr_en_basis_mat");

    if (init_method) {
        depr_alloc_macrs_5_frac = 0;
        depr_alloc_macrs_15_frac = 0;
        depr_alloc_sl_5_frac = 0;
        depr_alloc_sl_15_frac = 0;
        depr_alloc_sl_20_frac = 0;
        depr_alloc_sl_39_frac = 0;
        depr_alloc_custom_frac = 0;

        itc_sta_qual_macrs_5_frac = 0;
        itc_sta_qual_macrs_15_frac = 0;
        itc_sta_qual_sl_5_frac = 0;
        itc_sta_qual_sl_15_frac = 0;
        itc_sta_qual_sl_20_frac = 0;
        itc_sta_qual_sl_39_frac = 0;
        itc_sta_qual_custom_frac = 0;
        itc_sta_qual_none_frac = 0;

        itc_fed_qual_macrs_5_frac = 0;
        itc_fed_qual_macrs_15_frac = 0;
        itc_fed_qual_sl_5_frac = 0;
        itc_fed_qual_sl_15_frac = 0;
        itc_fed_qual_sl_20_frac = 0;
        itc_fed_qual_sl_39_frac = 0;
        itc_fed_qual_custom_frac = 0;
        itc_fed_qual_none_frac = 0;

        util::matrix_t<double> depr_basis_mat;
        cm->get_matrix("depr_basis_mat", depr_basis_mat);
        for (size_t r = 0; r < depr_basis_mat.nrows(); r++) {
            double alloc_percent = depr_basis_mat.at(r, 2) * 0.01;
            if (depr_basis_mat.at(r, 5) == 0) { //MACRS 5 year
                depr_alloc_macrs_5_frac += alloc_percent;
                itc_fed_qual_macrs_5_frac += (depr_basis_mat.at(r, 3) ? alloc_percent : 0);
                itc_sta_qual_macrs_5_frac += (depr_basis_mat.at(r, 4) ? alloc_percent : 0);
            }
            else if (depr_basis_mat.at(r, 5) == 1) { //MACRS 15 year
                depr_alloc_macrs_15_frac += alloc_percent;
                itc_fed_qual_macrs_15_frac += (depr_basis_mat.at(r, 3) ? alloc_percent : 0);
                itc_sta_qual_macrs_15_frac += (depr_basis_mat.at(r, 4) ? alloc_percent : 0);
            }
            else if (depr_basis_mat.at(r, 5) == 2) { //SL 5 year
                depr_alloc_sl_5_frac += alloc_percent;
                itc_fed_qual_sl_5_frac += (depr_basis_mat.at(r, 3) ? alloc_percent : 0);
                itc_sta_qual_sl_5_frac += (depr_basis_mat.at(r, 4) ? alloc_percent : 0);
            }
            else if (depr_basis_mat.at(r, 5) == 3) { //SL 15 year
                depr_alloc_sl_15_frac += alloc_percent;
                itc_fed_qual_sl_15_frac += (depr_basis_mat.at(r, 3) ? alloc_percent : 0);
                itc_sta_qual_sl_15_frac += (depr_basis_mat.at(r, 4) ? alloc_percent : 0);
            }
            else if (depr_basis_mat.at(r, 5) == 4) { //SL 20 year
                depr_alloc_sl_20_frac += alloc_percent;
                itc_fed_qual_sl_20_frac += (depr_basis_mat.at(r, 3) ? alloc_percent : 0);
                itc_sta_qual_sl_20_frac += (depr_basis_mat.at(r, 4) ? alloc_percent : 0);
            }
            else if (depr_basis_mat.at(r, 5) == 5) { //SL 39 year
                depr_alloc_sl_39_frac += alloc_percent;
                itc_fed_qual_sl_39_frac += (depr_basis_mat.at(r, 3) ? alloc_percent : 0);
                itc_sta_qual_sl_39_frac += (depr_basis_mat.at(r, 4) ? alloc_percent : 0);
            }
            else if (depr_basis_mat.at(r, 5) == 6) { //Custom
                depr_alloc_custom_frac += alloc_percent;
                itc_fed_qual_custom_frac += (depr_basis_mat.at(r, 3) ? alloc_percent : 0);
                itc_sta_qual_custom_frac += (depr_basis_mat.at(r, 4) ? alloc_percent : 0);
            }
            else if (depr_basis_mat.at(r, 5) == 7) { //None (for residential)
                itc_fed_qual_none_frac += (depr_basis_mat.at(r, 3) ? alloc_percent : 0);
                itc_sta_qual_none_frac += (depr_basis_mat.at(r, 4) ? alloc_percent : 0);
            }
        }
        // Normalize the ITC qualification fractions by the allocation fractions to get the percent of each depreciation category that qualifies for ITC
        itc_fed_qual_macrs_5_frac = (depr_alloc_macrs_5_frac > 0) ? itc_fed_qual_macrs_5_frac / depr_alloc_macrs_5_frac : 0;
        itc_fed_qual_macrs_15_frac = (depr_alloc_macrs_15_frac > 0) ? itc_fed_qual_macrs_15_frac / depr_alloc_macrs_15_frac : 0;
        itc_fed_qual_sl_5_frac = (depr_alloc_sl_5_frac > 0) ? itc_fed_qual_sl_5_frac / depr_alloc_sl_5_frac : 0;
        itc_fed_qual_sl_15_frac = (depr_alloc_sl_15_frac > 0) ? itc_fed_qual_sl_15_frac / depr_alloc_sl_15_frac : 0;
        itc_fed_qual_sl_20_frac = (depr_alloc_sl_20_frac > 0) ? itc_fed_qual_sl_20_frac / depr_alloc_sl_20_frac : 0;
        itc_fed_qual_sl_39_frac = (depr_alloc_sl_39_frac > 0) ? itc_fed_qual_sl_39_frac / depr_alloc_sl_39_frac : 0;
        itc_fed_qual_custom_frac = (depr_alloc_custom_frac > 0) ? itc_fed_qual_custom_frac / depr_alloc_custom_frac : 0;


        itc_sta_qual_macrs_5_frac = (depr_alloc_macrs_5_frac > 0) ? itc_sta_qual_macrs_5_frac / depr_alloc_macrs_5_frac : 0;
        itc_sta_qual_macrs_15_frac = (depr_alloc_macrs_15_frac > 0) ? itc_sta_qual_macrs_15_frac / depr_alloc_macrs_15_frac : 0;
        itc_sta_qual_sl_5_frac = (depr_alloc_sl_5_frac > 0) ? itc_sta_qual_sl_5_frac / depr_alloc_sl_5_frac : 0;
        itc_sta_qual_sl_15_frac = (depr_alloc_sl_15_frac > 0) ? itc_sta_qual_sl_15_frac / depr_alloc_sl_15_frac : 0;
        itc_sta_qual_sl_20_frac = (depr_alloc_sl_20_frac > 0) ? itc_sta_qual_sl_20_frac / depr_alloc_sl_20_frac : 0;
        itc_sta_qual_sl_39_frac = (depr_alloc_sl_39_frac > 0) ? itc_sta_qual_sl_39_frac / depr_alloc_sl_39_frac : 0;
        itc_sta_qual_custom_frac = (depr_alloc_custom_frac > 0) ? itc_sta_qual_custom_frac / depr_alloc_custom_frac : 0;
        itc_sta_qual_custom_frac = (depr_alloc_custom_frac > 0) ? itc_sta_qual_custom_frac / depr_alloc_custom_frac : 0;

    }
    else {

        depr_alloc_macrs_5_frac = cm->as_double("depr_alloc_macrs_5_percent") * 0.01;
        depr_alloc_macrs_15_frac = cm->as_double("depr_alloc_macrs_15_percent") * 0.01;
        depr_alloc_sl_5_frac = cm->as_double("depr_alloc_sl_5_percent") * 0.01;
        depr_alloc_sl_15_frac = cm->as_double("depr_alloc_sl_15_percent") * 0.01;
        depr_alloc_sl_20_frac = cm->as_double("depr_alloc_sl_20_percent") * 0.01;
        depr_alloc_sl_39_frac = cm->as_double("depr_alloc_sl_39_percent") * 0.01;
        depr_alloc_custom_frac = cm->as_double("depr_alloc_custom_percent") * 0.01;


        itc_sta_qual_macrs_5_frac = (cm->as_boolean("depr_itc_sta_macrs_5") ? 1 : 0);
        itc_sta_qual_macrs_15_frac = (cm->as_boolean("depr_itc_sta_macrs_15") ? 1 : 0);
        itc_sta_qual_sl_5_frac = (cm->as_boolean("depr_itc_sta_sl_5") ? 1 : 0);
        itc_sta_qual_sl_15_frac = (cm->as_boolean("depr_itc_sta_sl_15") ? 1 : 0);
        itc_sta_qual_sl_20_frac = (cm->as_boolean("depr_itc_sta_sl_20") ? 1 : 0);
        itc_sta_qual_sl_39_frac = (cm->as_boolean("depr_itc_sta_sl_39") ? 1 : 0);
        itc_sta_qual_custom_frac = (cm->as_boolean("depr_itc_sta_custom") ? 1 : 0);

        itc_fed_qual_macrs_5_frac = (cm->as_boolean("depr_itc_fed_macrs_5") ? 1 : 0);
        itc_fed_qual_macrs_15_frac = (cm->as_boolean("depr_itc_fed_macrs_15") ? 1 : 0);
        itc_fed_qual_sl_5_frac = (cm->as_boolean("depr_itc_fed_sl_5") ? 1 : 0);
        itc_fed_qual_sl_15_frac = (cm->as_boolean("depr_itc_fed_sl_15") ? 1 : 0);
        itc_fed_qual_sl_20_frac = (cm->as_boolean("depr_itc_fed_sl_20") ? 1 : 0);
        itc_fed_qual_sl_39_frac = (cm->as_boolean("depr_itc_fed_sl_39") ? 1 : 0);
        itc_fed_qual_custom_frac = (cm->as_boolean("depr_itc_fed_custom") ? 1 : 0);


    }

    depr_alloc_total_frac = depr_alloc_macrs_5_frac + depr_alloc_macrs_15_frac +
        depr_alloc_sl_5_frac + depr_alloc_sl_15_frac + depr_alloc_sl_20_frac + depr_alloc_sl_39_frac + depr_alloc_custom_frac;
    // TODO - place check that depr_alloc_total_frac <= 1 and <>0
    depr_alloc_none_frac = 1.0 - depr_alloc_total_frac;
    // TODO - place check that depr_alloc_none_frac >= 0

    if (init_method) {
        itc_fed_qual_none_frac = (depr_alloc_none_frac > 0) ? itc_fed_qual_none_frac / depr_alloc_none_frac : 0;
        itc_sta_qual_none_frac = (depr_alloc_none_frac > 0) ? itc_sta_qual_none_frac / depr_alloc_none_frac : 0;
    }
    else {
        itc_fed_qual_none_frac = 0.;
        itc_sta_qual_none_frac = 0.;
    }

    // ITC reduction
    itc_fed_percent_deprbas_sta = cm->as_boolean("itc_fed_percent_deprbas_sta") ? 1.0 : 0.0;
    itc_fed_amount_deprbas_sta = cm->as_boolean("itc_fed_amount_deprbas_sta") ? 1.0 : 0.0;
    itc_sta_percent_deprbas_sta = cm->as_boolean("itc_sta_percent_deprbas_sta") ? 1.0 : 0.0;
    itc_sta_amount_deprbas_sta = cm->as_boolean("itc_sta_amount_deprbas_sta") ? 1.0 : 0.0;

    // Bonus depreciation
    depr_stabas_macrs_5_bonus_frac = (cm->as_boolean("depr_bonus_sta_macrs_5") ? cm->as_double("depr_bonus_sta") * 0.01 : 0);
    depr_stabas_macrs_15_bonus_frac = (cm->as_boolean("depr_bonus_sta_macrs_15") ? cm->as_double("depr_bonus_sta") * 0.01 : 0);
    depr_stabas_sl_5_bonus_frac = (cm->as_boolean("depr_bonus_sta_sl_5") ? cm->as_double("depr_bonus_sta") * 0.01 : 0);
    depr_stabas_sl_15_bonus_frac = (cm->as_boolean("depr_bonus_sta_sl_15") ? cm->as_double("depr_bonus_sta") * 0.01 : 0);
    depr_stabas_sl_20_bonus_frac = (cm->as_boolean("depr_bonus_sta_sl_20") ? cm->as_double("depr_bonus_sta") * 0.01 : 0);
    depr_stabas_sl_39_bonus_frac = (cm->as_boolean("depr_bonus_sta_sl_39") ? cm->as_double("depr_bonus_sta") * 0.01 : 0);
    depr_stabas_custom_bonus_frac = (cm->as_boolean("depr_bonus_sta_custom") ? cm->as_double("depr_bonus_sta") * 0.01 : 0);

    // ITC reduction
    itc_fed_percent_deprbas_fed = cm->as_boolean("itc_fed_percent_deprbas_fed") ? 1.0 : 0.0;
    itc_fed_amount_deprbas_fed = cm->as_boolean("itc_fed_amount_deprbas_fed") ? 1.0 : 0.0;
    itc_sta_percent_deprbas_fed = cm->as_boolean("itc_sta_percent_deprbas_fed") ? 1.0 : 0.0;
    itc_sta_amount_deprbas_fed = cm->as_boolean("itc_sta_amount_deprbas_fed") ? 1.0 : 0.0;

    // Bonus depreciation
    depr_fedbas_macrs_5_bonus_frac = (cm->as_boolean("depr_bonus_fed_macrs_5") ? cm->as_double("depr_bonus_fed") * 0.01 : 0);
    depr_fedbas_macrs_15_bonus_frac = (cm->as_boolean("depr_bonus_fed_macrs_15") ? cm->as_double("depr_bonus_fed") * 0.01 : 0);
    depr_fedbas_sl_5_bonus_frac = (cm->as_boolean("depr_bonus_fed_sl_5") ? cm->as_double("depr_bonus_fed") * 0.01 : 0);
    depr_fedbas_sl_15_bonus_frac = (cm->as_boolean("depr_bonus_fed_sl_15") ? cm->as_double("depr_bonus_fed") * 0.01 : 0);
    depr_fedbas_sl_20_bonus_frac = (cm->as_boolean("depr_bonus_fed_sl_20") ? cm->as_double("depr_bonus_fed") * 0.01 : 0);
    depr_fedbas_sl_39_bonus_frac = (cm->as_boolean("depr_bonus_fed_sl_39") ? cm->as_double("depr_bonus_fed") * 0.01 : 0);
    depr_fedbas_custom_bonus_frac = (cm->as_boolean("depr_bonus_fed_custom") ? cm->as_double("depr_bonus_fed") * 0.01 : 0);

    // redistribute fractions to only depreciable allocations
    if (depr_alloc_total_frac > 0) // and <=1
    {
        depr_alloc_macrs_5_frac /= depr_alloc_total_frac;
        depr_alloc_macrs_15_frac /= depr_alloc_total_frac;
        depr_alloc_sl_5_frac /= depr_alloc_total_frac;
        depr_alloc_sl_15_frac /= depr_alloc_total_frac;
        depr_alloc_sl_20_frac /= depr_alloc_total_frac;
        depr_alloc_sl_39_frac /= depr_alloc_total_frac;
        depr_alloc_custom_frac /= depr_alloc_total_frac;
    }


    if (cm->as_integer("depr_stabas_method") == 0)
    {
        depr_stabas_macrs_5_frac = 1.0;
        depr_stabas_macrs_15_frac = 0.0;
        depr_stabas_sl_5_frac = 0.0;
        depr_stabas_sl_15_frac = 0.0;
        depr_stabas_sl_20_frac = 0.0;
        depr_stabas_sl_39_frac = 0.0;
        depr_stabas_custom_frac = 0.0;
    }
    else
    {
        depr_stabas_macrs_5_frac = depr_alloc_macrs_5_frac;
        depr_stabas_macrs_15_frac = depr_alloc_macrs_15_frac;
        depr_stabas_sl_5_frac = depr_alloc_sl_5_frac;
        depr_stabas_sl_15_frac = depr_alloc_sl_15_frac;
        depr_stabas_sl_20_frac = depr_alloc_sl_20_frac;
        depr_stabas_sl_39_frac = depr_alloc_sl_39_frac;
        depr_stabas_custom_frac = depr_alloc_custom_frac;
    }

    if (cm->as_integer("depr_fedbas_method") == 0)
    {
        depr_fedbas_macrs_5_frac = 1.0;
        depr_fedbas_macrs_15_frac = 0.0;
        depr_fedbas_sl_5_frac = 0.0;
        depr_fedbas_sl_15_frac = 0.0;
        depr_fedbas_sl_20_frac = 0.0;
        depr_fedbas_sl_39_frac = 0.0;
        depr_fedbas_custom_frac = 0.0;
    }
    else
    {
        depr_fedbas_macrs_5_frac = depr_alloc_macrs_5_frac;
        depr_fedbas_macrs_15_frac = depr_alloc_macrs_15_frac;
        depr_fedbas_sl_5_frac = depr_alloc_sl_5_frac;
        depr_fedbas_sl_15_frac = depr_alloc_sl_15_frac;
        depr_fedbas_sl_20_frac = depr_alloc_sl_20_frac;
        depr_fedbas_sl_39_frac = depr_alloc_sl_39_frac;
        depr_fedbas_custom_frac = depr_alloc_custom_frac;
    }



    itc_sta_disallow_factor = 0.5;

    itc_disallow_sta_fixed_macrs_5 = (itc_sta_disallow_factor * itc_sta_qual_macrs_5_frac * itc_sta_amount);
    itc_disallow_sta_fixed_macrs_15 = (itc_sta_disallow_factor * itc_sta_qual_macrs_15_frac * itc_sta_amount);
    itc_disallow_sta_fixed_sl_5 = (itc_sta_disallow_factor * itc_sta_qual_sl_5_frac * itc_sta_amount);
    itc_disallow_sta_fixed_sl_15 = (itc_sta_disallow_factor * itc_sta_qual_sl_15_frac * itc_sta_amount);
    itc_disallow_sta_fixed_sl_20 = (itc_sta_disallow_factor * itc_sta_qual_sl_20_frac * itc_sta_amount);
    itc_disallow_sta_fixed_sl_39 = (itc_sta_disallow_factor * itc_sta_qual_sl_39_frac * itc_sta_amount);
    itc_disallow_sta_fixed_custom = (itc_sta_disallow_factor * itc_sta_qual_custom_frac * itc_sta_amount);




    itc_fed_disallow_factor = 0.5;

    itc_disallow_fed_fixed_macrs_5 = (itc_fed_disallow_factor * itc_fed_qual_macrs_5_frac * itc_fed_amount);
    itc_disallow_fed_fixed_macrs_15 = (itc_fed_disallow_factor * itc_fed_qual_macrs_15_frac * itc_fed_amount);
    itc_disallow_fed_fixed_sl_5 = (itc_fed_disallow_factor * itc_fed_qual_sl_5_frac * itc_fed_amount);
    itc_disallow_fed_fixed_sl_15 = (itc_fed_disallow_factor * itc_fed_qual_sl_15_frac * itc_fed_amount);
    itc_disallow_fed_fixed_sl_20 = (itc_fed_disallow_factor * itc_fed_qual_sl_20_frac * itc_fed_amount);
    itc_disallow_fed_fixed_sl_39 = (itc_fed_disallow_factor * itc_fed_qual_sl_39_frac * itc_fed_amount);
    itc_disallow_fed_fixed_custom = (itc_fed_disallow_factor * itc_fed_qual_custom_frac * itc_fed_amount);

    

}

void itc_and_depreciation_calculations::calc_basis(double in_pre_depr_alloc_basis, double in_pre_itc_qual_basis) {
    pre_depr_alloc_basis = in_pre_itc_qual_basis;
    pre_itc_qual_basis = in_pre_itc_qual_basis;

    depr_alloc_total = depr_alloc_total_frac * pre_depr_alloc_basis;
    depr_alloc_macrs_5 = depr_alloc_macrs_5_frac * depr_alloc_total;
    depr_alloc_macrs_15 = depr_alloc_macrs_15_frac * depr_alloc_total;
    depr_alloc_sl_5 = depr_alloc_sl_5_frac * depr_alloc_total;
    depr_alloc_sl_15 = depr_alloc_sl_15_frac * depr_alloc_total;
    depr_alloc_sl_20 = depr_alloc_sl_20_frac * depr_alloc_total;
    depr_alloc_sl_39 = depr_alloc_sl_39_frac * depr_alloc_total;
    depr_alloc_custom = depr_alloc_custom_frac * depr_alloc_total;
    depr_alloc_none = depr_alloc_none_frac * pre_depr_alloc_basis;

    itc_sta_qual_macrs_5 = itc_sta_qual_macrs_5_frac * (depr_alloc_macrs_5 - depr_stabas_macrs_5_frac * depr_sta_reduction);
    itc_sta_qual_macrs_15 = itc_sta_qual_macrs_15_frac * (depr_alloc_macrs_15 - depr_stabas_macrs_15_frac * depr_sta_reduction);
    itc_sta_qual_sl_5 = itc_sta_qual_sl_5_frac * (depr_alloc_sl_5 - depr_stabas_sl_5_frac * depr_sta_reduction);
    itc_sta_qual_sl_15 = itc_sta_qual_sl_15_frac * (depr_alloc_sl_15 - depr_stabas_sl_15_frac * depr_sta_reduction);
    itc_sta_qual_sl_20 = itc_sta_qual_sl_20_frac * (depr_alloc_sl_20 - depr_stabas_sl_20_frac * depr_sta_reduction);
    itc_sta_qual_sl_39 = itc_sta_qual_sl_39_frac * (depr_alloc_sl_39 - depr_stabas_sl_39_frac * depr_sta_reduction);
    itc_sta_qual_custom = itc_sta_qual_custom_frac * (depr_alloc_custom - depr_stabas_custom_frac * depr_sta_reduction);
    itc_sta_qual_none = itc_sta_qual_none_frac * depr_alloc_none; // Used for residential

    itc_sta_qual_total = itc_sta_qual_macrs_5 + itc_sta_qual_macrs_15 + itc_sta_qual_sl_5 + itc_sta_qual_sl_15 + itc_sta_qual_sl_20 + itc_sta_qual_sl_39 + itc_sta_qual_custom + itc_sta_qual_none;

    // SAM 1038
    itc_sta_per = 0.0;
    for (size_t k = 0; k <= nyears; k++) {
        m_cf.at(CF_itc_sta_percent_amount, k) = libfin::min(m_cf.at(CF_itc_sta_percent_maxvalue, k), m_cf.at(CF_itc_sta_percent_fraction, k) * itc_sta_qual_total);
        itc_sta_per += m_cf.at(CF_itc_sta_percent_amount, k);
    }

    if (itc_sta_qual_total > 0)
    {
        itc_disallow_sta_percent_macrs_5 = (itc_sta_disallow_factor * itc_sta_qual_macrs_5 / itc_sta_qual_total * itc_sta_per);
        itc_disallow_sta_percent_macrs_15 = (itc_sta_disallow_factor * itc_sta_qual_macrs_15 / itc_sta_qual_total * itc_sta_per);
        itc_disallow_sta_percent_sl_5 = (itc_sta_disallow_factor * itc_sta_qual_sl_5 / itc_sta_qual_total * itc_sta_per);
        itc_disallow_sta_percent_sl_15 = (itc_sta_disallow_factor * itc_sta_qual_sl_15 / itc_sta_qual_total * itc_sta_per);
        itc_disallow_sta_percent_sl_20 = (itc_sta_disallow_factor * itc_sta_qual_sl_20 / itc_sta_qual_total * itc_sta_per);
        itc_disallow_sta_percent_sl_39 = (itc_sta_disallow_factor * itc_sta_qual_sl_39 / itc_sta_qual_total * itc_sta_per);
        itc_disallow_sta_percent_custom = (itc_sta_disallow_factor * itc_sta_qual_custom / itc_sta_qual_total * itc_sta_per);

        itc_disallow_sta_fixed_macrs_5 = (itc_sta_disallow_factor * itc_sta_qual_macrs_5 / itc_sta_qual_total * itc_sta_amount);
        itc_disallow_sta_fixed_macrs_15 = (itc_sta_disallow_factor * itc_sta_qual_macrs_15 / itc_sta_qual_total * itc_sta_amount);
        itc_disallow_sta_fixed_sl_5 = (itc_sta_disallow_factor * itc_sta_qual_sl_5 / itc_sta_qual_total * itc_sta_amount);
        itc_disallow_sta_fixed_sl_15 = (itc_sta_disallow_factor * itc_sta_qual_sl_15 / itc_sta_qual_total * itc_sta_amount);
        itc_disallow_sta_fixed_sl_20 = (itc_sta_disallow_factor * itc_sta_qual_sl_20 / itc_sta_qual_total * itc_sta_amount);
        itc_disallow_sta_fixed_sl_39 = (itc_sta_disallow_factor * itc_sta_qual_sl_39 / itc_sta_qual_total * itc_sta_amount);
        itc_disallow_sta_fixed_custom = (itc_sta_disallow_factor * itc_sta_qual_custom / itc_sta_qual_total * itc_sta_amount);
    }
    else
    {
        itc_disallow_sta_percent_macrs_5 = 0;
        itc_disallow_sta_percent_macrs_15 = 0;
        itc_disallow_sta_percent_sl_5 = 0;
        itc_disallow_sta_percent_sl_15 = 0;
        itc_disallow_sta_percent_sl_20 = 0;
        itc_disallow_sta_percent_sl_39 = 0;
        itc_disallow_sta_percent_custom = 0;

        itc_disallow_sta_fixed_macrs_5 = 0;
        itc_disallow_sta_fixed_macrs_15 = 0;
        itc_disallow_sta_fixed_sl_5 = 0;
        itc_disallow_sta_fixed_sl_15 = 0;
        itc_disallow_sta_fixed_sl_20 = 0;
        itc_disallow_sta_fixed_sl_39 = 0;
        itc_disallow_sta_fixed_custom = 0;
    }

    itc_fed_qual_macrs_5 = itc_fed_qual_macrs_5_frac * (depr_alloc_macrs_5 - depr_fedbas_macrs_5_frac * depr_fed_reduction);
    itc_fed_qual_macrs_15 = itc_fed_qual_macrs_15_frac * (depr_alloc_macrs_15 - depr_fedbas_macrs_15_frac * depr_fed_reduction);
    itc_fed_qual_sl_5 = itc_fed_qual_sl_5_frac * (depr_alloc_sl_5 - depr_fedbas_sl_5_frac * depr_fed_reduction);
    itc_fed_qual_sl_15 = itc_fed_qual_sl_15_frac * (depr_alloc_sl_15 - depr_fedbas_sl_15_frac * depr_fed_reduction);
    itc_fed_qual_sl_20 = itc_fed_qual_sl_20_frac * (depr_alloc_sl_20 - depr_fedbas_sl_20_frac * depr_fed_reduction);
    itc_fed_qual_sl_39 = itc_fed_qual_sl_39_frac * (depr_alloc_sl_39 - depr_fedbas_sl_39_frac * depr_fed_reduction);
    itc_fed_qual_custom = itc_fed_qual_custom_frac * (depr_alloc_custom - depr_fedbas_custom_frac * depr_fed_reduction);
    itc_fed_qual_none = itc_fed_qual_none_frac * depr_alloc_none; // Used for residential

    itc_fed_qual_total = itc_fed_qual_macrs_5 + itc_fed_qual_macrs_15 + itc_fed_qual_sl_5 + itc_fed_qual_sl_15 + itc_fed_qual_sl_20 + itc_fed_qual_sl_39 + itc_fed_qual_custom + itc_fed_qual_none;

    // SAM 1038
    itc_fed_per = 0.0;
    for (size_t k = 0; k <= nyears; k++) {
        m_cf.at(CF_itc_fed_percent_amount, k) = libfin::min(m_cf.at(CF_itc_fed_percent_maxvalue, k), m_cf.at(CF_itc_fed_percent_fraction, k) * itc_fed_qual_total);
        itc_fed_per += m_cf.at(CF_itc_fed_percent_amount, k);
    }

    if (itc_fed_qual_total > 0)
    {
        itc_disallow_fed_percent_macrs_5 = (itc_fed_disallow_factor * itc_fed_qual_macrs_5 / itc_fed_qual_total * itc_fed_per);
        itc_disallow_fed_percent_macrs_15 = (itc_fed_disallow_factor * itc_fed_qual_macrs_15 / itc_fed_qual_total * itc_fed_per);
        itc_disallow_fed_percent_sl_5 = (itc_fed_disallow_factor * itc_fed_qual_sl_5 / itc_fed_qual_total * itc_fed_per);
        itc_disallow_fed_percent_sl_15 = (itc_fed_disallow_factor * itc_fed_qual_sl_15 / itc_fed_qual_total * itc_fed_per);
        itc_disallow_fed_percent_sl_20 = (itc_fed_disallow_factor * itc_fed_qual_sl_20 / itc_fed_qual_total * itc_fed_per);
        itc_disallow_fed_percent_sl_39 = (itc_fed_disallow_factor * itc_fed_qual_sl_39 / itc_fed_qual_total * itc_fed_per);
        itc_disallow_fed_percent_custom = (itc_fed_disallow_factor * itc_fed_qual_custom / itc_fed_qual_total * itc_fed_per);

        itc_disallow_fed_fixed_macrs_5 = (itc_fed_disallow_factor * itc_fed_qual_macrs_5 / itc_fed_qual_total * itc_fed_amount);
        itc_disallow_fed_fixed_macrs_15 = (itc_fed_disallow_factor * itc_fed_qual_macrs_15 / itc_fed_qual_total * itc_fed_amount);
        itc_disallow_fed_fixed_sl_5 = (itc_fed_disallow_factor * itc_fed_qual_sl_5 / itc_fed_qual_total * itc_fed_amount);
        itc_disallow_fed_fixed_sl_15 = (itc_fed_disallow_factor * itc_fed_qual_sl_15 / itc_fed_qual_total * itc_fed_amount);
        itc_disallow_fed_fixed_sl_20 = (itc_fed_disallow_factor * itc_fed_qual_sl_20 / itc_fed_qual_total * itc_fed_amount);
        itc_disallow_fed_fixed_sl_39 = (itc_fed_disallow_factor * itc_fed_qual_sl_39 / itc_fed_qual_total * itc_fed_amount);
        itc_disallow_fed_fixed_custom = (itc_fed_disallow_factor * itc_fed_qual_custom / itc_fed_qual_total * itc_fed_amount);
    }
    else
    {
        itc_disallow_fed_percent_macrs_5 = 0;
        itc_disallow_fed_percent_macrs_15 = 0;
        itc_disallow_fed_percent_sl_5 = 0;
        itc_disallow_fed_percent_sl_15 = 0;
        itc_disallow_fed_percent_sl_20 = 0;
        itc_disallow_fed_percent_sl_39 = 0;
        itc_disallow_fed_percent_custom = 0;

        itc_disallow_fed_fixed_macrs_5 = 0;
        itc_disallow_fed_fixed_macrs_15 = 0;
        itc_disallow_fed_fixed_sl_5 = 0;
        itc_disallow_fed_fixed_sl_15 = 0;
        itc_disallow_fed_fixed_sl_20 = 0;
        itc_disallow_fed_fixed_sl_39 = 0;
        itc_disallow_fed_fixed_custom = 0;
    }

    // SAM 1038
    for (size_t k = 0; k <= nyears; k++) {
        m_cf.at(CF_itc_fed, k) = m_cf.at(CF_itc_fed_amount, k) + m_cf.at(CF_itc_fed_percent_amount, k);
        m_cf.at(CF_itc_sta, k) = m_cf.at(CF_itc_sta_amount, k) + m_cf.at(CF_itc_sta_percent_amount, k);
        m_cf.at(CF_itc_total, k) = m_cf.at(CF_itc_fed, k) + m_cf.at(CF_itc_sta, k);
    }
    // Depreciation
    // State depreciation
    depr_stabas_macrs_5 = depr_alloc_macrs_5 - depr_stabas_macrs_5_frac * depr_sta_reduction;
    depr_stabas_macrs_15 = depr_alloc_macrs_15 - depr_stabas_macrs_15_frac * depr_sta_reduction;
    depr_stabas_sl_5 = depr_alloc_sl_5 - depr_stabas_sl_5_frac * depr_sta_reduction;
    depr_stabas_sl_15 = depr_alloc_sl_15 - depr_stabas_sl_15_frac * depr_sta_reduction;
    depr_stabas_sl_20 = depr_alloc_sl_20 - depr_stabas_sl_20_frac * depr_sta_reduction;
    depr_stabas_sl_39 = depr_alloc_sl_39 - depr_stabas_sl_39_frac * depr_sta_reduction;
    depr_stabas_custom = depr_alloc_custom - depr_stabas_custom_frac * depr_sta_reduction;

    // ITC reduction
    depr_stabas_macrs_5 -= (itc_fed_percent_deprbas_sta * itc_disallow_fed_percent_macrs_5 +
        itc_fed_amount_deprbas_sta * itc_disallow_fed_fixed_macrs_5 +
        itc_sta_percent_deprbas_sta * itc_disallow_sta_percent_macrs_5 +
        itc_sta_amount_deprbas_sta * itc_disallow_sta_fixed_macrs_5);

    depr_stabas_macrs_15 -= (itc_fed_percent_deprbas_sta * itc_disallow_fed_percent_macrs_15 +
        itc_fed_amount_deprbas_sta * itc_disallow_fed_fixed_macrs_15 +
        itc_sta_percent_deprbas_sta * itc_disallow_sta_percent_macrs_15 +
        itc_sta_amount_deprbas_sta * itc_disallow_sta_fixed_macrs_15);

    depr_stabas_sl_5 -= (itc_fed_percent_deprbas_sta * itc_disallow_fed_percent_sl_5 +
        itc_fed_amount_deprbas_sta * itc_disallow_fed_fixed_sl_5 +
        itc_sta_percent_deprbas_sta * itc_disallow_sta_percent_sl_5 +
        itc_sta_amount_deprbas_sta * itc_disallow_sta_fixed_sl_5);

    depr_stabas_sl_15 -= (itc_fed_percent_deprbas_sta * itc_disallow_fed_percent_sl_15 +
        itc_fed_amount_deprbas_sta * itc_disallow_fed_fixed_sl_15 +
        itc_sta_percent_deprbas_sta * itc_disallow_sta_percent_sl_15 +
        itc_sta_amount_deprbas_sta * itc_disallow_sta_fixed_sl_15);

    depr_stabas_sl_20 -= (itc_fed_percent_deprbas_sta * itc_disallow_fed_percent_sl_20 +
        itc_fed_amount_deprbas_sta * itc_disallow_fed_fixed_sl_20 +
        itc_sta_percent_deprbas_sta * itc_disallow_sta_percent_sl_20 +
        itc_sta_amount_deprbas_sta * itc_disallow_sta_fixed_sl_20);

    depr_stabas_sl_39 -= (itc_fed_percent_deprbas_sta * itc_disallow_fed_percent_sl_39 +
        itc_fed_amount_deprbas_sta * itc_disallow_fed_fixed_sl_39 +
        itc_sta_percent_deprbas_sta * itc_disallow_sta_percent_sl_39 +
        itc_sta_amount_deprbas_sta * itc_disallow_sta_fixed_sl_39);

    depr_stabas_custom -= (itc_fed_percent_deprbas_sta * itc_disallow_fed_percent_custom +
        itc_fed_amount_deprbas_sta * itc_disallow_fed_fixed_custom +
        itc_sta_percent_deprbas_sta * itc_disallow_sta_percent_custom +
        itc_sta_amount_deprbas_sta * itc_disallow_sta_fixed_custom);

    // Bonus depreciation
    depr_stabas_macrs_5_bonus = depr_stabas_macrs_5_bonus_frac * depr_stabas_macrs_5;
    depr_stabas_macrs_15_bonus = depr_stabas_macrs_15_bonus_frac * depr_stabas_macrs_15;
    depr_stabas_sl_5_bonus = depr_stabas_sl_5_bonus_frac * depr_stabas_sl_5;
    depr_stabas_sl_15_bonus = depr_stabas_sl_15_bonus_frac * depr_stabas_sl_15;
    depr_stabas_sl_20_bonus = depr_stabas_sl_20_bonus_frac * depr_stabas_sl_20;
    depr_stabas_sl_39_bonus = depr_stabas_sl_39_bonus_frac * depr_stabas_sl_39;
    depr_stabas_custom_bonus = depr_stabas_custom_bonus_frac * depr_stabas_custom;

    depr_stabas_macrs_5 -= depr_stabas_macrs_5_bonus;
    depr_stabas_macrs_15 -= depr_stabas_macrs_15_bonus;
    depr_stabas_sl_5 -= depr_stabas_sl_5_bonus;
    depr_stabas_sl_15 -= depr_stabas_sl_15_bonus;
    depr_stabas_sl_20 -= depr_stabas_sl_20_bonus;
    depr_stabas_sl_39 -= depr_stabas_sl_39_bonus;
    depr_stabas_custom -= depr_stabas_custom_bonus;

    depr_stabas_total = depr_stabas_macrs_5 + depr_stabas_macrs_15 + depr_stabas_sl_5 + depr_stabas_sl_15 + depr_stabas_sl_20 + depr_stabas_sl_39 + depr_stabas_custom;

    // Federal depreciation
    depr_fedbas_macrs_5 = depr_alloc_macrs_5 - depr_fedbas_macrs_5_frac * depr_fed_reduction;
    depr_fedbas_macrs_15 = depr_alloc_macrs_15 - depr_fedbas_macrs_15_frac * depr_fed_reduction;
    depr_fedbas_sl_5 = depr_alloc_sl_5 - depr_fedbas_sl_5_frac * depr_fed_reduction;
    depr_fedbas_sl_15 = depr_alloc_sl_15 - depr_fedbas_sl_15_frac * depr_fed_reduction;
    depr_fedbas_sl_20 = depr_alloc_sl_20 - depr_fedbas_sl_20_frac * depr_fed_reduction;
    depr_fedbas_sl_39 = depr_alloc_sl_39 - depr_fedbas_sl_39_frac * depr_fed_reduction;
    depr_fedbas_custom = depr_alloc_custom - depr_fedbas_custom_frac * depr_fed_reduction;

    // ITC reduction
    depr_fedbas_macrs_5 -= (itc_fed_percent_deprbas_fed * itc_disallow_fed_percent_macrs_5 +
        itc_fed_amount_deprbas_fed * itc_disallow_fed_fixed_macrs_5 +
        itc_sta_percent_deprbas_fed * itc_disallow_sta_percent_macrs_5 +
        itc_sta_amount_deprbas_fed * itc_disallow_sta_fixed_macrs_5);

    depr_fedbas_macrs_15 -= (itc_fed_percent_deprbas_fed * itc_disallow_fed_percent_macrs_15 +
        itc_fed_amount_deprbas_fed * itc_disallow_fed_fixed_macrs_15 +
        itc_sta_percent_deprbas_fed * itc_disallow_sta_percent_macrs_15 +
        itc_sta_amount_deprbas_fed * itc_disallow_sta_fixed_macrs_15);

    depr_fedbas_sl_5 -= (itc_fed_percent_deprbas_fed * itc_disallow_fed_percent_sl_5 +
        itc_fed_amount_deprbas_fed * itc_disallow_fed_fixed_sl_5 +
        itc_sta_percent_deprbas_fed * itc_disallow_sta_percent_sl_5 +
        itc_sta_amount_deprbas_fed * itc_disallow_sta_fixed_sl_5);

    depr_fedbas_sl_15 -= (itc_fed_percent_deprbas_fed * itc_disallow_fed_percent_sl_15 +
        itc_fed_amount_deprbas_fed * itc_disallow_fed_fixed_sl_15 +
        itc_sta_percent_deprbas_fed * itc_disallow_sta_percent_sl_15 +
        itc_sta_amount_deprbas_fed * itc_disallow_sta_fixed_sl_15);

    depr_fedbas_sl_20 -= (itc_fed_percent_deprbas_fed * itc_disallow_fed_percent_sl_20 +
        itc_fed_amount_deprbas_fed * itc_disallow_fed_fixed_sl_20 +
        itc_sta_percent_deprbas_fed * itc_disallow_sta_percent_sl_20 +
        itc_sta_amount_deprbas_fed * itc_disallow_sta_fixed_sl_20);

    depr_fedbas_sl_39 -= (itc_fed_percent_deprbas_fed * itc_disallow_fed_percent_sl_39 +
        itc_fed_amount_deprbas_fed * itc_disallow_fed_fixed_sl_39 +
        itc_sta_percent_deprbas_fed * itc_disallow_sta_percent_sl_39 +
        itc_sta_amount_deprbas_fed * itc_disallow_sta_fixed_sl_39);

    depr_fedbas_custom -= (itc_fed_percent_deprbas_fed * itc_disallow_fed_percent_custom +
        itc_fed_amount_deprbas_fed * itc_disallow_fed_fixed_custom +
        itc_sta_percent_deprbas_fed * itc_disallow_sta_percent_custom +
        itc_sta_amount_deprbas_fed * itc_disallow_sta_fixed_custom);

    // Bonus depreciation
    depr_fedbas_macrs_5_bonus = depr_fedbas_macrs_5_bonus_frac * depr_fedbas_macrs_5;
    depr_fedbas_macrs_15_bonus = depr_fedbas_macrs_15_bonus_frac * depr_fedbas_macrs_15;
    depr_fedbas_sl_5_bonus = depr_fedbas_sl_5_bonus_frac * depr_fedbas_sl_5;
    depr_fedbas_sl_15_bonus = depr_fedbas_sl_15_bonus_frac * depr_fedbas_sl_15;
    depr_fedbas_sl_20_bonus = depr_fedbas_sl_20_bonus_frac * depr_fedbas_sl_20;
    depr_fedbas_sl_39_bonus = depr_fedbas_sl_39_bonus_frac * depr_fedbas_sl_39;
    depr_fedbas_custom_bonus = depr_fedbas_custom_bonus_frac * depr_fedbas_custom;

    depr_fedbas_macrs_5 -= depr_fedbas_macrs_5_bonus;
    depr_fedbas_macrs_15 -= depr_fedbas_macrs_15_bonus;
    depr_fedbas_sl_5 -= depr_fedbas_sl_5_bonus;
    depr_fedbas_sl_15 -= depr_fedbas_sl_15_bonus;
    depr_fedbas_sl_20 -= depr_fedbas_sl_20_bonus;
    depr_fedbas_sl_39 -= depr_fedbas_sl_39_bonus;
    depr_fedbas_custom -= depr_fedbas_custom_bonus;

    depr_fedbas_total = depr_fedbas_macrs_5 + depr_fedbas_macrs_15 + depr_fedbas_sl_5 + depr_fedbas_sl_15 + depr_fedbas_sl_20 + depr_fedbas_sl_39 + depr_fedbas_custom;
}

void itc_and_depreciation_calculations::set_depr_schedules(compute_module* cm) {
    depreciation_sched_5_year_macrs_half_year(CF_macrs_5_frac);
    depreciation_sched_15_year_macrs_half_year(CF_macrs_15_frac);
    depreciation_sched_5_year_straight_line_half_year(CF_sl_5_frac);
    depreciation_sched_15_year_straight_line_half_year(CF_sl_15_frac);
    depreciation_sched_20_year_straight_line_half_year(CF_sl_20_frac);
    depreciation_sched_39_year_straight_line_half_year(CF_sl_39_frac);
    depreciation_sched_custom(cm, CF_custom_frac, "depr_custom_schedule");

    
}

void itc_and_depreciation_calculations::calc_annual_depreciation(int i) {
    m_cf.at(CF_stadepr_macrs_5, i) = m_cf.at(CF_macrs_5_frac, i) * depr_stabas_macrs_5;
    m_cf.at(CF_stadepr_macrs_15, i) = m_cf.at(CF_macrs_15_frac, i) * depr_stabas_macrs_15;
    m_cf.at(CF_stadepr_sl_5, i) = m_cf.at(CF_sl_5_frac, i) * depr_stabas_sl_5;
    m_cf.at(CF_stadepr_sl_15, i) = m_cf.at(CF_sl_15_frac, i) * depr_stabas_sl_15;
    m_cf.at(CF_stadepr_sl_20, i) = m_cf.at(CF_sl_20_frac, i) * depr_stabas_sl_20;
    m_cf.at(CF_stadepr_sl_39, i) = m_cf.at(CF_sl_39_frac, i) * depr_stabas_sl_39;
    m_cf.at(CF_stadepr_custom, i) = m_cf.at(CF_custom_frac, i) * depr_stabas_custom;


    m_cf.at(CF_stadepr_total, i) =
        m_cf.at(CF_stadepr_macrs_5, i) +
        m_cf.at(CF_stadepr_macrs_15, i) +
        m_cf.at(CF_stadepr_sl_5, i) +
        m_cf.at(CF_stadepr_sl_15, i) +
        m_cf.at(CF_stadepr_sl_20, i) +
        m_cf.at(CF_stadepr_sl_39, i) +
        m_cf.at(CF_stadepr_custom, i) +
        m_cf.at(CF_stadepr_me1, i) +
        m_cf.at(CF_stadepr_me2, i) +
        m_cf.at(CF_stadepr_me3, i);

    if (i == 1) m_cf.at(CF_stadepr_total, i) += (depr_stabas_macrs_5_bonus + depr_stabas_macrs_15_bonus + depr_stabas_sl_5_bonus + depr_stabas_sl_15_bonus + depr_stabas_sl_20_bonus + depr_stabas_sl_39_bonus + depr_stabas_custom_bonus);

    m_cf.at(CF_feddepr_macrs_5, i) = m_cf.at(CF_macrs_5_frac, i) * depr_fedbas_macrs_5;
    m_cf.at(CF_feddepr_macrs_15, i) = m_cf.at(CF_macrs_15_frac, i) * depr_fedbas_macrs_15;
    m_cf.at(CF_feddepr_sl_5, i) = m_cf.at(CF_sl_5_frac, i) * depr_fedbas_sl_5;
    m_cf.at(CF_feddepr_sl_15, i) = m_cf.at(CF_sl_15_frac, i) * depr_fedbas_sl_15;
    m_cf.at(CF_feddepr_sl_20, i) = m_cf.at(CF_sl_20_frac, i) * depr_fedbas_sl_20;
    m_cf.at(CF_feddepr_sl_39, i) = m_cf.at(CF_sl_39_frac, i) * depr_fedbas_sl_39;
    m_cf.at(CF_feddepr_custom, i) = m_cf.at(CF_custom_frac, i) * depr_fedbas_custom;
    m_cf.at(CF_feddepr_total, i) =
        m_cf.at(CF_feddepr_macrs_5, i) +
        m_cf.at(CF_feddepr_macrs_15, i) +
        m_cf.at(CF_feddepr_sl_5, i) +
        m_cf.at(CF_feddepr_sl_15, i) +
        m_cf.at(CF_feddepr_sl_20, i) +
        m_cf.at(CF_feddepr_sl_39, i) +
        m_cf.at(CF_feddepr_custom, i) +
        m_cf.at(CF_feddepr_me1, i) +
        m_cf.at(CF_feddepr_me2, i) +
        m_cf.at(CF_feddepr_me3, i);
    if (i == 1) m_cf.at(CF_feddepr_total, i) += (depr_fedbas_macrs_5_bonus + depr_fedbas_macrs_15_bonus + depr_fedbas_sl_5_bonus + depr_fedbas_sl_15_bonus + depr_fedbas_sl_20_bonus + depr_fedbas_sl_39_bonus + depr_fedbas_custom_bonus);

}

void itc_and_depreciation_calculations::write_outputs(compute_module* cm) {
    cm->assign("depr_fedbas_macrs_5", var_data((ssc_number_t)depr_fedbas_macrs_5));
    cm->assign("depr_fedbas_macrs_15", var_data((ssc_number_t)depr_fedbas_macrs_15));
    cm->assign("depr_fedbas_sl_5", var_data((ssc_number_t)depr_fedbas_sl_5));
    cm->assign("depr_fedbas_sl_15", var_data((ssc_number_t)depr_fedbas_sl_15));
    cm->assign("depr_fedbas_sl_20", var_data((ssc_number_t)depr_fedbas_sl_20));
    cm->assign("depr_fedbas_sl_39", var_data((ssc_number_t)depr_fedbas_sl_39));
    cm->assign("depr_fedbas_custom", var_data((ssc_number_t)depr_fedbas_custom));
    cm->assign("depr_fedbas_total", var_data((ssc_number_t)depr_fedbas_total));

    cm->assign("itc_fed_qual_macrs_5", var_data((ssc_number_t)itc_fed_qual_macrs_5));
    cm->assign("itc_fed_qual_macrs_15", var_data((ssc_number_t)itc_fed_qual_macrs_15));
    cm->assign("itc_fed_qual_sl_5", var_data((ssc_number_t)itc_fed_qual_sl_5));
    cm->assign("itc_fed_qual_sl_15", var_data((ssc_number_t)itc_fed_qual_sl_15));
    cm->assign("itc_fed_qual_sl_20", var_data((ssc_number_t)itc_fed_qual_sl_20));
    cm->assign("itc_fed_qual_sl_39", var_data((ssc_number_t)itc_fed_qual_sl_39));
    cm->assign("itc_fed_qual_custom", var_data((ssc_number_t)itc_fed_qual_custom));

    cm->assign("itc_disallow_fed_percent_macrs_5", var_data((ssc_number_t)itc_disallow_fed_percent_macrs_5));
    cm->assign("itc_disallow_fed_percent_macrs_15", var_data((ssc_number_t)itc_disallow_fed_percent_macrs_15));
    cm->assign("itc_disallow_fed_percent_sl_5", var_data((ssc_number_t)itc_disallow_fed_percent_sl_5));
    cm->assign("itc_disallow_fed_percent_sl_15", var_data((ssc_number_t)itc_disallow_fed_percent_sl_15));
    cm->assign("itc_disallow_fed_percent_sl_20", var_data((ssc_number_t)itc_disallow_fed_percent_sl_20));
    cm->assign("itc_disallow_fed_percent_sl_39", var_data((ssc_number_t)itc_disallow_fed_percent_sl_39));
    cm->assign("itc_disallow_fed_percent_custom", var_data((ssc_number_t)itc_disallow_fed_percent_custom));

    cm->assign("itc_disallow_fed_fixed_macrs_5", var_data((ssc_number_t)itc_disallow_fed_fixed_macrs_5));
    cm->assign("itc_disallow_fed_fixed_macrs_15", var_data((ssc_number_t)itc_disallow_fed_fixed_macrs_15));
    cm->assign("itc_disallow_fed_fixed_sl_5", var_data((ssc_number_t)itc_disallow_fed_fixed_sl_5));
    cm->assign("itc_disallow_fed_fixed_sl_15", var_data((ssc_number_t)itc_disallow_fed_fixed_sl_15));
    cm->assign("itc_disallow_fed_fixed_sl_20", var_data((ssc_number_t)itc_disallow_fed_fixed_sl_20));
    cm->assign("itc_disallow_fed_fixed_sl_39", var_data((ssc_number_t)itc_disallow_fed_fixed_sl_39));
    cm->assign("itc_disallow_fed_fixed_custom", var_data((ssc_number_t)itc_disallow_fed_fixed_custom));

    cm->assign("itc_fed_qual_total", var_data((ssc_number_t)itc_fed_qual_total));
    cm->assign("itc_fed_percent_total", var_data((ssc_number_t)itc_fed_per));
    cm->assign("itc_fed_fixed_total", var_data((ssc_number_t)itc_fed_amount));

    // SAM 1038
    double itc_fed_total = 0.0;
    double itc_sta_total = 0.0;
    double itc_total = 0.0;

    for (size_t k = 0; k <= nyears; k++) {
        itc_fed_total += m_cf.at(CF_itc_fed, k);
        itc_sta_total += m_cf.at(CF_itc_sta, k);
        itc_total += m_cf.at(CF_itc_total, k);
    }
    cm->assign("itc_total_fed", var_data((ssc_number_t)itc_fed_total));
    cm->assign("itc_total_sta", var_data((ssc_number_t)itc_sta_total));
    cm->assign("itc_total", var_data((ssc_number_t)itc_total));

    // save_cf(compute_module *cm, util::matrix_t<double>& mat, int cf_line, int nyears, const std::string &name)
    save_cf(cm, m_cf, CF_stadepr_macrs_5, nyears, "cf_stadepr_macrs_5");
    save_cf(cm, m_cf, CF_stadepr_macrs_15, nyears, "cf_stadepr_macrs_15");
    save_cf(cm, m_cf, CF_stadepr_sl_5, nyears, "cf_stadepr_sl_5");
    save_cf(cm, m_cf, CF_stadepr_sl_15, nyears, "cf_stadepr_sl_15");
    save_cf(cm, m_cf, CF_stadepr_sl_20, nyears, "cf_stadepr_sl_20");
    save_cf(cm, m_cf, CF_stadepr_sl_39, nyears, "cf_stadepr_sl_39");
    save_cf(cm, m_cf, CF_stadepr_custom, nyears, "cf_stadepr_custom");
    save_cf(cm, m_cf, CF_stadepr_me1, nyears, "cf_stadepr_me1");
    save_cf(cm, m_cf, CF_stadepr_me2, nyears, "cf_stadepr_me2");
    save_cf(cm, m_cf, CF_stadepr_me3, nyears, "cf_stadepr_me3");
    save_cf(cm, m_cf, CF_stadepr_total, nyears, "cf_stadepr_total");

    save_cf(cm, m_cf, CF_feddepr_macrs_5, nyears, "cf_feddepr_macrs_5");
    save_cf(cm, m_cf, CF_feddepr_macrs_15, nyears, "cf_feddepr_macrs_15");
    save_cf(cm, m_cf, CF_feddepr_sl_5, nyears, "cf_feddepr_sl_5");
    save_cf(cm, m_cf, CF_feddepr_sl_15, nyears, "cf_feddepr_sl_15");
    save_cf(cm, m_cf, CF_feddepr_sl_20, nyears, "cf_feddepr_sl_20");
    save_cf(cm, m_cf, CF_feddepr_sl_39, nyears, "cf_feddepr_sl_39");
    save_cf(cm, m_cf, CF_feddepr_custom, nyears, "cf_feddepr_custom");
    save_cf(cm, m_cf, CF_feddepr_me1, nyears, "cf_feddepr_me1");
    save_cf(cm, m_cf, CF_feddepr_me2, nyears, "cf_feddepr_me2");
    save_cf(cm, m_cf, CF_feddepr_me3, nyears, "cf_feddepr_me3");
    save_cf(cm, m_cf, CF_feddepr_total, nyears, "cf_feddepr_total");

    // State ITC/depreciation table
    cm->assign("depr_stabas_percent_macrs_5", var_data((ssc_number_t)(depr_stabas_macrs_5_frac * 100.0)));
    cm->assign("depr_alloc_macrs_5", var_data((ssc_number_t)depr_alloc_macrs_5));
    double depr_stabas_ibi_reduc_macrs_5 = depr_stabas_macrs_5_frac * depr_sta_reduction_ibi;
    double depr_stabas_cbi_reduc_macrs_5 = depr_stabas_macrs_5_frac * depr_sta_reduction_cbi;
    cm->assign("depr_stabas_ibi_reduc_macrs_5", var_data((ssc_number_t)depr_stabas_ibi_reduc_macrs_5));
    cm->assign("depr_stabas_cbi_reduc_macrs_5", var_data((ssc_number_t)depr_stabas_cbi_reduc_macrs_5));
    cm->assign("depr_stabas_prior_itc_macrs_5", var_data((ssc_number_t)(depr_alloc_macrs_5 - depr_stabas_ibi_reduc_macrs_5 - depr_stabas_cbi_reduc_macrs_5)));
    cm->assign("itc_sta_qual_macrs_5", var_data((ssc_number_t)itc_sta_qual_macrs_5));
    double depr_stabas_percent_qual_macrs_5 = (itc_sta_qual_total > 0) ? 100.0 * itc_sta_qual_macrs_5 / itc_sta_qual_total : 0.0;
    cm->assign("depr_stabas_percent_qual_macrs_5", var_data((ssc_number_t)depr_stabas_percent_qual_macrs_5));
    cm->assign("depr_stabas_percent_amount_macrs_5", var_data((ssc_number_t)(depr_stabas_percent_qual_macrs_5 / 100.0 * itc_sta_per)));
    cm->assign("itc_disallow_sta_percent_macrs_5", var_data((ssc_number_t)itc_disallow_sta_percent_macrs_5));
    cm->assign("depr_stabas_fixed_amount_macrs_5", var_data((ssc_number_t)(depr_stabas_percent_qual_macrs_5 / 100.0 * itc_sta_amount)));
    cm->assign("itc_disallow_sta_fixed_macrs_5", var_data((ssc_number_t)itc_disallow_sta_fixed_macrs_5));
    double depr_stabas_itc_sta_reduction_macrs_5 = itc_sta_percent_deprbas_sta * itc_disallow_sta_percent_macrs_5 + itc_sta_amount_deprbas_sta * itc_disallow_sta_fixed_macrs_5;
    double depr_stabas_itc_fed_reduction_macrs_5 = itc_fed_percent_deprbas_sta * itc_disallow_fed_percent_macrs_5 + itc_fed_amount_deprbas_sta * itc_disallow_fed_fixed_macrs_5;
    cm->assign("depr_stabas_itc_sta_reduction_macrs_5", var_data((ssc_number_t)depr_stabas_itc_sta_reduction_macrs_5));
    cm->assign("depr_stabas_itc_fed_reduction_macrs_5", var_data((ssc_number_t)depr_stabas_itc_fed_reduction_macrs_5));
    cm->assign("depr_stabas_after_itc_macrs_5", var_data((ssc_number_t)(depr_stabas_macrs_5 + depr_stabas_macrs_5_bonus)));
    cm->assign("depr_stabas_first_year_bonus_macrs_5", var_data((ssc_number_t)depr_stabas_macrs_5_bonus));
    cm->assign("depr_stabas_macrs_5", var_data((ssc_number_t)depr_stabas_macrs_5));

    cm->assign("depr_stabas_percent_macrs_15", var_data((ssc_number_t)(depr_stabas_macrs_15_frac * 100.0)));
    cm->assign("depr_alloc_macrs_15", var_data((ssc_number_t)depr_alloc_macrs_15));
    double depr_stabas_ibi_reduc_macrs_15 = depr_stabas_macrs_15_frac * depr_sta_reduction_ibi;
    double depr_stabas_cbi_reduc_macrs_15 = depr_stabas_macrs_15_frac * depr_sta_reduction_cbi;
    cm->assign("depr_stabas_ibi_reduc_macrs_15", var_data((ssc_number_t)depr_stabas_ibi_reduc_macrs_15));
    cm->assign("depr_stabas_cbi_reduc_macrs_15", var_data((ssc_number_t)depr_stabas_cbi_reduc_macrs_15));
    cm->assign("depr_stabas_prior_itc_macrs_15", var_data((ssc_number_t)(depr_alloc_macrs_15 - depr_stabas_ibi_reduc_macrs_15 - depr_stabas_cbi_reduc_macrs_15)));
    cm->assign("itc_sta_qual_macrs_15", var_data((ssc_number_t)itc_sta_qual_macrs_15));
    double depr_stabas_percent_qual_macrs_15 = (itc_sta_qual_total > 0) ? 100.0 * itc_sta_qual_macrs_15 / itc_sta_qual_total : 0.0;
    cm->assign("depr_stabas_percent_qual_macrs_15", var_data((ssc_number_t)depr_stabas_percent_qual_macrs_15));
    cm->assign("depr_stabas_percent_amount_macrs_15", var_data((ssc_number_t)(depr_stabas_percent_qual_macrs_15 / 100.0 * itc_sta_per)));
    cm->assign("itc_disallow_sta_percent_macrs_15", var_data((ssc_number_t)itc_disallow_sta_percent_macrs_15));
    cm->assign("depr_stabas_fixed_amount_macrs_15", var_data((ssc_number_t)(depr_stabas_percent_qual_macrs_15 / 100.0 * itc_sta_amount)));
    cm->assign("itc_disallow_sta_fixed_macrs_15", var_data((ssc_number_t)itc_disallow_sta_fixed_macrs_15));
    double depr_stabas_itc_sta_reduction_macrs_15 = itc_sta_percent_deprbas_sta * itc_disallow_sta_percent_macrs_15 + itc_sta_amount_deprbas_sta * itc_disallow_sta_fixed_macrs_15;
    double depr_stabas_itc_fed_reduction_macrs_15 = itc_fed_percent_deprbas_sta * itc_disallow_fed_percent_macrs_15 + itc_fed_amount_deprbas_sta * itc_disallow_fed_fixed_macrs_15;
    cm->assign("depr_stabas_itc_sta_reduction_macrs_15", var_data((ssc_number_t)depr_stabas_itc_sta_reduction_macrs_15));
    cm->assign("depr_stabas_itc_fed_reduction_macrs_15", var_data((ssc_number_t)depr_stabas_itc_fed_reduction_macrs_15));
    cm->assign("depr_stabas_after_itc_macrs_15", var_data((ssc_number_t)(depr_stabas_macrs_15 + depr_stabas_macrs_15_bonus)));
    cm->assign("depr_stabas_first_year_bonus_macrs_15", var_data((ssc_number_t)depr_stabas_macrs_15_bonus));
    cm->assign("depr_stabas_macrs_15", var_data((ssc_number_t)depr_stabas_macrs_15));

    cm->assign("depr_stabas_percent_sl_5", var_data((ssc_number_t)(depr_stabas_sl_5_frac * 100.0)));
    cm->assign("depr_alloc_sl_5", var_data((ssc_number_t)depr_alloc_sl_5));
    double depr_stabas_ibi_reduc_sl_5 = depr_stabas_sl_5_frac * depr_sta_reduction_ibi;
    double depr_stabas_cbi_reduc_sl_5 = depr_stabas_sl_5_frac * depr_sta_reduction_cbi;
    cm->assign("depr_stabas_ibi_reduc_sl_5", var_data((ssc_number_t)depr_stabas_ibi_reduc_sl_5));
    cm->assign("depr_stabas_cbi_reduc_sl_5", var_data((ssc_number_t)depr_stabas_cbi_reduc_sl_5));
    cm->assign("depr_stabas_prior_itc_sl_5", var_data((ssc_number_t)(depr_alloc_sl_5 - depr_stabas_ibi_reduc_sl_5 - depr_stabas_cbi_reduc_sl_5)));
    cm->assign("itc_sta_qual_sl_5", var_data((ssc_number_t)itc_sta_qual_sl_5));
    double depr_stabas_percent_qual_sl_5 = (itc_sta_qual_total > 0) ? 100.0 * itc_sta_qual_sl_5 / itc_sta_qual_total : 0.0;
    cm->assign("depr_stabas_percent_qual_sl_5", var_data((ssc_number_t)depr_stabas_percent_qual_sl_5));
    cm->assign("depr_stabas_percent_amount_sl_5", var_data((ssc_number_t)(depr_stabas_percent_qual_sl_5 / 100.0 * itc_sta_per)));
    cm->assign("itc_disallow_sta_percent_sl_5", var_data((ssc_number_t)itc_disallow_sta_percent_sl_5));
    cm->assign("depr_stabas_fixed_amount_sl_5", var_data((ssc_number_t)(depr_stabas_percent_qual_sl_5 / 100.0 * itc_sta_amount)));
    cm->assign("itc_disallow_sta_fixed_sl_5", var_data((ssc_number_t)itc_disallow_sta_fixed_sl_5));
    double depr_stabas_itc_sta_reduction_sl_5 = itc_sta_percent_deprbas_sta * itc_disallow_sta_percent_sl_5 + itc_sta_amount_deprbas_sta * itc_disallow_sta_fixed_sl_5;
    double depr_stabas_itc_fed_reduction_sl_5 = itc_fed_percent_deprbas_sta * itc_disallow_fed_percent_sl_5 + itc_fed_amount_deprbas_sta * itc_disallow_fed_fixed_sl_5;
    cm->assign("depr_stabas_itc_sta_reduction_sl_5", var_data((ssc_number_t)depr_stabas_itc_sta_reduction_sl_5));
    cm->assign("depr_stabas_itc_fed_reduction_sl_5", var_data((ssc_number_t)depr_stabas_itc_fed_reduction_sl_5));
    cm->assign("depr_stabas_after_itc_sl_5", var_data((ssc_number_t)(depr_stabas_sl_5 + depr_stabas_sl_5_bonus)));
    cm->assign("depr_stabas_first_year_bonus_sl_5", var_data((ssc_number_t)depr_stabas_sl_5_bonus));
    cm->assign("depr_stabas_sl_5", var_data((ssc_number_t)depr_stabas_sl_5));

    cm->assign("depr_stabas_percent_sl_15", var_data((ssc_number_t)(depr_stabas_sl_15_frac * 100.0)));
    cm->assign("depr_alloc_sl_15", var_data((ssc_number_t)depr_alloc_sl_15));
    double depr_stabas_ibi_reduc_sl_15 = depr_stabas_sl_15_frac * depr_sta_reduction_ibi;
    double depr_stabas_cbi_reduc_sl_15 = depr_stabas_sl_15_frac * depr_sta_reduction_cbi;
    cm->assign("depr_stabas_ibi_reduc_sl_15", var_data((ssc_number_t)depr_stabas_ibi_reduc_sl_15));
    cm->assign("depr_stabas_cbi_reduc_sl_15", var_data((ssc_number_t)depr_stabas_cbi_reduc_sl_15));
    cm->assign("depr_stabas_prior_itc_sl_15", var_data((ssc_number_t)(depr_alloc_sl_15 - depr_stabas_ibi_reduc_sl_15 - depr_stabas_cbi_reduc_sl_15)));
    cm->assign("itc_sta_qual_sl_15", var_data((ssc_number_t)itc_sta_qual_sl_15));
    double depr_stabas_percent_qual_sl_15 = (itc_sta_qual_total > 0) ? 100.0 * itc_sta_qual_sl_15 / itc_sta_qual_total : 0.0;
    cm->assign("depr_stabas_percent_qual_sl_15", var_data((ssc_number_t)depr_stabas_percent_qual_sl_15));
    cm->assign("depr_stabas_percent_amount_sl_15", var_data((ssc_number_t)(depr_stabas_percent_qual_sl_15 / 100.0 * itc_sta_per)));
    cm->assign("itc_disallow_sta_percent_sl_15", var_data((ssc_number_t)itc_disallow_sta_percent_sl_15));
    cm->assign("depr_stabas_fixed_amount_sl_15", var_data((ssc_number_t)(depr_stabas_percent_qual_sl_15 / 100.0 * itc_sta_amount)));
    cm->assign("itc_disallow_sta_fixed_sl_15", var_data((ssc_number_t)itc_disallow_sta_fixed_sl_15));
    double depr_stabas_itc_sta_reduction_sl_15 = itc_sta_percent_deprbas_sta * itc_disallow_sta_percent_sl_15 + itc_sta_amount_deprbas_sta * itc_disallow_sta_fixed_sl_15;
    double depr_stabas_itc_fed_reduction_sl_15 = itc_fed_percent_deprbas_sta * itc_disallow_fed_percent_sl_15 + itc_fed_amount_deprbas_sta * itc_disallow_fed_fixed_sl_15;
    cm->assign("depr_stabas_itc_sta_reduction_sl_15", var_data((ssc_number_t)depr_stabas_itc_sta_reduction_sl_15));
    cm->assign("depr_stabas_itc_fed_reduction_sl_15", var_data((ssc_number_t)depr_stabas_itc_fed_reduction_sl_15));
    cm->assign("depr_stabas_after_itc_sl_15", var_data((ssc_number_t)(depr_stabas_sl_15 + depr_stabas_sl_15_bonus)));
    cm->assign("depr_stabas_first_year_bonus_sl_15", var_data((ssc_number_t)depr_stabas_sl_15_bonus));
    cm->assign("depr_stabas_sl_15", var_data((ssc_number_t)depr_stabas_sl_15));

    cm->assign("depr_stabas_percent_sl_20", var_data((ssc_number_t)(depr_stabas_sl_20_frac * 100.0)));
    cm->assign("depr_alloc_sl_20", var_data((ssc_number_t)depr_alloc_sl_20));
    double depr_stabas_ibi_reduc_sl_20 = depr_stabas_sl_20_frac * depr_sta_reduction_ibi;
    double depr_stabas_cbi_reduc_sl_20 = depr_stabas_sl_20_frac * depr_sta_reduction_cbi;
    cm->assign("depr_stabas_ibi_reduc_sl_20", var_data((ssc_number_t)depr_stabas_ibi_reduc_sl_20));
    cm->assign("depr_stabas_cbi_reduc_sl_20", var_data((ssc_number_t)depr_stabas_cbi_reduc_sl_20));
    cm->assign("depr_stabas_prior_itc_sl_20", var_data((ssc_number_t)(depr_alloc_sl_20 - depr_stabas_ibi_reduc_sl_20 - depr_stabas_cbi_reduc_sl_20)));
    cm->assign("itc_sta_qual_sl_20", var_data((ssc_number_t)itc_sta_qual_sl_20));
    double depr_stabas_percent_qual_sl_20 = (itc_sta_qual_total > 0) ? 100.0 * itc_sta_qual_sl_20 / itc_sta_qual_total : 0.0;
    cm->assign("depr_stabas_percent_qual_sl_20", var_data((ssc_number_t)depr_stabas_percent_qual_sl_20));
    cm->assign("depr_stabas_percent_amount_sl_20", var_data((ssc_number_t)(depr_stabas_percent_qual_sl_20 / 100.0 * itc_sta_per)));
    cm->assign("itc_disallow_sta_percent_sl_20", var_data((ssc_number_t)itc_disallow_sta_percent_sl_20));
    cm->assign("depr_stabas_fixed_amount_sl_20", var_data((ssc_number_t)(depr_stabas_percent_qual_sl_20 / 100.0 * itc_sta_amount)));
    cm->assign("itc_disallow_sta_fixed_sl_20", var_data((ssc_number_t)itc_disallow_sta_fixed_sl_20));
    double depr_stabas_itc_sta_reduction_sl_20 = itc_sta_percent_deprbas_sta * itc_disallow_sta_percent_sl_20 + itc_sta_amount_deprbas_sta * itc_disallow_sta_fixed_sl_20;
    double depr_stabas_itc_fed_reduction_sl_20 = itc_fed_percent_deprbas_sta * itc_disallow_fed_percent_sl_20 + itc_fed_amount_deprbas_sta * itc_disallow_fed_fixed_sl_20;
    cm->assign("depr_stabas_itc_sta_reduction_sl_20", var_data((ssc_number_t)depr_stabas_itc_sta_reduction_sl_20));
    cm->assign("depr_stabas_itc_fed_reduction_sl_20", var_data((ssc_number_t)depr_stabas_itc_fed_reduction_sl_20));
    cm->assign("depr_stabas_after_itc_sl_20", var_data((ssc_number_t)(depr_stabas_sl_20 + depr_stabas_sl_20_bonus)));
    cm->assign("depr_stabas_first_year_bonus_sl_20", var_data((ssc_number_t)depr_stabas_sl_20_bonus));
    cm->assign("depr_stabas_sl_20", var_data((ssc_number_t)depr_stabas_sl_20));

    cm->assign("depr_stabas_percent_sl_39", var_data((ssc_number_t)(depr_stabas_sl_39_frac * 100.0)));
    cm->assign("depr_alloc_sl_39", var_data((ssc_number_t)depr_alloc_sl_39));
    double depr_stabas_ibi_reduc_sl_39 = depr_stabas_sl_39_frac * depr_sta_reduction_ibi;
    double depr_stabas_cbi_reduc_sl_39 = depr_stabas_sl_39_frac * depr_sta_reduction_cbi;
    cm->assign("depr_stabas_ibi_reduc_sl_39", var_data((ssc_number_t)depr_stabas_ibi_reduc_sl_39));
    cm->assign("depr_stabas_cbi_reduc_sl_39", var_data((ssc_number_t)depr_stabas_cbi_reduc_sl_39));
    cm->assign("depr_stabas_prior_itc_sl_39", var_data((ssc_number_t)(depr_alloc_sl_39 - depr_stabas_ibi_reduc_sl_39 - depr_stabas_cbi_reduc_sl_39)));
    cm->assign("itc_sta_qual_sl_39", var_data((ssc_number_t)itc_sta_qual_sl_39));
    double depr_stabas_percent_qual_sl_39 = (itc_sta_qual_total > 0) ? 100.0 * itc_sta_qual_sl_39 / itc_sta_qual_total : 0.0;
    cm->assign("depr_stabas_percent_qual_sl_39", var_data((ssc_number_t)depr_stabas_percent_qual_sl_39));
    cm->assign("depr_stabas_percent_amount_sl_39", var_data((ssc_number_t)(depr_stabas_percent_qual_sl_39 / 100.0 * itc_sta_per)));
    cm->assign("itc_disallow_sta_percent_sl_39", var_data((ssc_number_t)itc_disallow_sta_percent_sl_39));
    cm->assign("depr_stabas_fixed_amount_sl_39", var_data((ssc_number_t)(depr_stabas_percent_qual_sl_39 / 100.0 * itc_sta_amount)));
    cm->assign("itc_disallow_sta_fixed_sl_39", var_data((ssc_number_t)itc_disallow_sta_fixed_sl_39));
    double depr_stabas_itc_sta_reduction_sl_39 = itc_sta_percent_deprbas_sta * itc_disallow_sta_percent_sl_39 + itc_sta_amount_deprbas_sta * itc_disallow_sta_fixed_sl_39;
    double depr_stabas_itc_fed_reduction_sl_39 = itc_fed_percent_deprbas_sta * itc_disallow_fed_percent_sl_39 + itc_fed_amount_deprbas_sta * itc_disallow_fed_fixed_sl_39;
    cm->assign("depr_stabas_itc_sta_reduction_sl_39", var_data((ssc_number_t)depr_stabas_itc_sta_reduction_sl_39));
    cm->assign("depr_stabas_itc_fed_reduction_sl_39", var_data((ssc_number_t)depr_stabas_itc_fed_reduction_sl_39));
    cm->assign("depr_stabas_after_itc_sl_39", var_data((ssc_number_t)(depr_stabas_sl_39 + depr_stabas_sl_39_bonus)));
    cm->assign("depr_stabas_first_year_bonus_sl_39", var_data((ssc_number_t)depr_stabas_sl_39_bonus));
    cm->assign("depr_stabas_sl_39", var_data((ssc_number_t)depr_stabas_sl_39));

    cm->assign("depr_stabas_percent_custom", var_data((ssc_number_t)(depr_stabas_custom_frac * 100.0)));
    cm->assign("depr_alloc_custom", var_data((ssc_number_t)depr_alloc_custom));
    double depr_stabas_ibi_reduc_custom = depr_stabas_custom_frac * depr_sta_reduction_ibi;
    double depr_stabas_cbi_reduc_custom = depr_stabas_custom_frac * depr_sta_reduction_cbi;
    cm->assign("depr_stabas_ibi_reduc_custom", var_data((ssc_number_t)depr_stabas_ibi_reduc_custom));
    cm->assign("depr_stabas_cbi_reduc_custom", var_data((ssc_number_t)depr_stabas_cbi_reduc_custom));
    cm->assign("depr_stabas_prior_itc_custom", var_data((ssc_number_t)(depr_alloc_custom - depr_stabas_ibi_reduc_custom - depr_stabas_cbi_reduc_custom)));
    cm->assign("itc_sta_qual_custom", var_data((ssc_number_t)itc_sta_qual_custom));
    double depr_stabas_percent_qual_custom = (itc_sta_qual_total > 0) ? 100.0 * itc_sta_qual_custom / itc_sta_qual_total : 0.0;
    cm->assign("depr_stabas_percent_qual_custom", var_data((ssc_number_t)depr_stabas_percent_qual_custom));
    cm->assign("depr_stabas_percent_amount_custom", var_data((ssc_number_t)(depr_stabas_percent_qual_custom / 100.0 * itc_sta_per)));
    cm->assign("itc_disallow_sta_percent_custom", var_data((ssc_number_t)itc_disallow_sta_percent_custom));
    cm->assign("depr_stabas_fixed_amount_custom", var_data((ssc_number_t)(depr_stabas_percent_qual_custom / 100.0 * itc_sta_amount)));
    cm->assign("itc_disallow_sta_fixed_custom", var_data((ssc_number_t)itc_disallow_sta_fixed_custom));
    double depr_stabas_itc_sta_reduction_custom = itc_sta_percent_deprbas_sta * itc_disallow_sta_percent_custom + itc_sta_amount_deprbas_sta * itc_disallow_sta_fixed_custom;
    double depr_stabas_itc_fed_reduction_custom = itc_fed_percent_deprbas_sta * itc_disallow_fed_percent_custom + itc_fed_amount_deprbas_sta * itc_disallow_fed_fixed_custom;
    cm->assign("depr_stabas_itc_sta_reduction_custom", var_data((ssc_number_t)depr_stabas_itc_sta_reduction_custom));
    cm->assign("depr_stabas_itc_fed_reduction_custom", var_data((ssc_number_t)depr_stabas_itc_fed_reduction_custom));
    cm->assign("depr_stabas_after_itc_custom", var_data((ssc_number_t)(depr_stabas_custom + depr_stabas_custom_bonus)));
    cm->assign("depr_stabas_first_year_bonus_custom", var_data((ssc_number_t)depr_stabas_custom_bonus));
    cm->assign("depr_stabas_custom", var_data((ssc_number_t)depr_stabas_custom));

    cm->assign("depr_stabas_percent_total", var_data((ssc_number_t)(100.0 * (depr_stabas_macrs_5_frac + depr_stabas_macrs_15_frac + depr_stabas_sl_5_frac + depr_stabas_sl_15_frac + depr_stabas_sl_20_frac + depr_stabas_sl_39_frac + depr_stabas_custom_frac))));
    cm->assign("depr_alloc_total", var_data((ssc_number_t)depr_alloc_total));
    cm->assign("depr_stabas_ibi_reduc_total", var_data((ssc_number_t)depr_sta_reduction_ibi));
    cm->assign("depr_stabas_cbi_reduc_total", var_data((ssc_number_t)depr_sta_reduction_cbi));
    cm->assign("depr_stabas_prior_itc_total", var_data((ssc_number_t)(depr_alloc_total - depr_sta_reduction_ibi - depr_sta_reduction_cbi)));
    cm->assign("itc_sta_qual_total", var_data((ssc_number_t)itc_sta_qual_total));
    cm->assign("depr_stabas_percent_qual_total", var_data((ssc_number_t)100.0));
    cm->assign("depr_stabas_percent_amount_total", var_data((ssc_number_t)itc_sta_per));
    cm->assign("itc_disallow_sta_percent_total", var_data((ssc_number_t)(itc_disallow_sta_percent_macrs_5 + itc_disallow_sta_percent_macrs_15 + itc_disallow_sta_percent_sl_5 + itc_disallow_sta_percent_sl_15 + itc_disallow_sta_percent_sl_20 + itc_disallow_sta_percent_sl_39 + itc_disallow_sta_percent_custom)));
    cm->assign("depr_stabas_fixed_amount_total", var_data((ssc_number_t)itc_sta_amount));
    cm->assign("itc_disallow_sta_fixed_total", var_data((ssc_number_t)(itc_disallow_sta_fixed_macrs_5 + itc_disallow_sta_fixed_macrs_15 + itc_disallow_sta_fixed_sl_5 + itc_disallow_sta_fixed_sl_15 + itc_disallow_sta_fixed_sl_20 + itc_disallow_sta_fixed_sl_39 + itc_disallow_sta_fixed_custom)));
    double depr_stabas_itc_sta_reduction_total = depr_stabas_itc_sta_reduction_macrs_5 + depr_stabas_itc_sta_reduction_macrs_15 + depr_stabas_itc_sta_reduction_sl_5 + depr_stabas_itc_sta_reduction_sl_15 + depr_stabas_itc_sta_reduction_sl_20 + depr_stabas_itc_sta_reduction_sl_39 + depr_stabas_itc_sta_reduction_custom;
    cm->assign("depr_stabas_itc_sta_reduction_total", var_data((ssc_number_t)depr_stabas_itc_sta_reduction_total));
    double depr_stabas_itc_fed_reduction_total = depr_stabas_itc_fed_reduction_macrs_5 + depr_stabas_itc_fed_reduction_macrs_15 + depr_stabas_itc_fed_reduction_sl_5 + depr_stabas_itc_fed_reduction_sl_15 + depr_stabas_itc_fed_reduction_sl_20 + depr_stabas_itc_fed_reduction_sl_39 + depr_stabas_itc_fed_reduction_custom;
    cm->assign("depr_stabas_itc_fed_reduction_total", var_data((ssc_number_t)depr_stabas_itc_fed_reduction_total));
    double depr_stabas_first_year_bonus_total = depr_stabas_macrs_5_bonus + depr_stabas_macrs_15_bonus + depr_stabas_sl_5_bonus + depr_stabas_sl_15_bonus + depr_stabas_sl_20_bonus + depr_stabas_sl_39_bonus + depr_stabas_custom_bonus;
    cm->assign("depr_stabas_after_itc_total", var_data((ssc_number_t)(depr_stabas_total + depr_stabas_first_year_bonus_total)));
    cm->assign("depr_stabas_first_year_bonus_total", var_data((ssc_number_t)depr_stabas_first_year_bonus_total));
    cm->assign("depr_stabas_total", var_data((ssc_number_t)depr_stabas_total));


    cm->assign("itc_sta_percent_total", var_data((ssc_number_t)itc_sta_per));
    cm->assign("itc_sta_fixed_total", var_data((ssc_number_t)itc_sta_amount));



    // Federal ITC/depreciation table
    cm->assign("depr_fedbas_percent_macrs_5", var_data((ssc_number_t)(depr_fedbas_macrs_5_frac * 100.0)));
    cm->assign("depr_alloc_macrs_5", var_data((ssc_number_t)depr_alloc_macrs_5));
    double depr_fedbas_ibi_reduc_macrs_5 = depr_fedbas_macrs_5_frac * depr_fed_reduction_ibi;
    double depr_fedbas_cbi_reduc_macrs_5 = depr_fedbas_macrs_5_frac * depr_fed_reduction_cbi;
    cm->assign("depr_fedbas_ibi_reduc_macrs_5", var_data((ssc_number_t)depr_fedbas_ibi_reduc_macrs_5));
    cm->assign("depr_fedbas_cbi_reduc_macrs_5", var_data((ssc_number_t)depr_fedbas_cbi_reduc_macrs_5));
    cm->assign("depr_fedbas_prior_itc_macrs_5", var_data((ssc_number_t)(depr_alloc_macrs_5 - depr_fedbas_ibi_reduc_macrs_5 - depr_fedbas_cbi_reduc_macrs_5)));
    cm->assign("itc_fed_qual_macrs_5", var_data((ssc_number_t)itc_fed_qual_macrs_5));
    double depr_fedbas_percent_qual_macrs_5 = (itc_fed_qual_total > 0) ? 100.0 * itc_fed_qual_macrs_5 / itc_fed_qual_total : 0.0;
    cm->assign("depr_fedbas_percent_qual_macrs_5", var_data((ssc_number_t)depr_fedbas_percent_qual_macrs_5));
    cm->assign("depr_fedbas_percent_amount_macrs_5", var_data((ssc_number_t)(depr_fedbas_percent_qual_macrs_5 / 100.0 * itc_fed_per)));
    cm->assign("itc_disallow_fed_percent_macrs_5", var_data((ssc_number_t)itc_disallow_fed_percent_macrs_5));
    cm->assign("depr_fedbas_fixed_amount_macrs_5", var_data((ssc_number_t)(depr_fedbas_percent_qual_macrs_5 / 100.0 * itc_fed_amount)));
    cm->assign("itc_disallow_fed_fixed_macrs_5", var_data((ssc_number_t)itc_disallow_fed_fixed_macrs_5));
    double depr_fedbas_itc_sta_reduction_macrs_5 = itc_sta_percent_deprbas_fed * itc_disallow_sta_percent_macrs_5 + itc_sta_amount_deprbas_fed * itc_disallow_sta_fixed_macrs_5;
    double depr_fedbas_itc_fed_reduction_macrs_5 = itc_fed_percent_deprbas_fed * itc_disallow_fed_percent_macrs_5 + itc_fed_amount_deprbas_fed * itc_disallow_fed_fixed_macrs_5;
    cm->assign("depr_fedbas_itc_sta_reduction_macrs_5", var_data((ssc_number_t)depr_fedbas_itc_sta_reduction_macrs_5));
    cm->assign("depr_fedbas_itc_fed_reduction_macrs_5", var_data((ssc_number_t)depr_fedbas_itc_fed_reduction_macrs_5));
    cm->assign("depr_fedbas_after_itc_macrs_5", var_data((ssc_number_t)(depr_fedbas_macrs_5 + depr_fedbas_macrs_5_bonus)));
    cm->assign("depr_fedbas_first_year_bonus_macrs_5", var_data((ssc_number_t)depr_fedbas_macrs_5_bonus));
    cm->assign("depr_fedbas_macrs_5", var_data((ssc_number_t)depr_fedbas_macrs_5));

    cm->assign("depr_fedbas_percent_macrs_15", var_data((ssc_number_t)(depr_fedbas_macrs_15_frac * 100.0)));
    cm->assign("depr_alloc_macrs_15", var_data((ssc_number_t)depr_alloc_macrs_15));
    double depr_fedbas_ibi_reduc_macrs_15 = depr_fedbas_macrs_15_frac * depr_fed_reduction_ibi;
    double depr_fedbas_cbi_reduc_macrs_15 = depr_fedbas_macrs_15_frac * depr_fed_reduction_cbi;
    cm->assign("depr_fedbas_ibi_reduc_macrs_15", var_data((ssc_number_t)depr_fedbas_ibi_reduc_macrs_15));
    cm->assign("depr_fedbas_cbi_reduc_macrs_15", var_data((ssc_number_t)depr_fedbas_cbi_reduc_macrs_15));
    cm->assign("depr_fedbas_prior_itc_macrs_15", var_data((ssc_number_t)(depr_alloc_macrs_15 - depr_fedbas_ibi_reduc_macrs_15 - depr_fedbas_cbi_reduc_macrs_15)));
    cm->assign("itc_fed_qual_macrs_15", var_data((ssc_number_t)itc_fed_qual_macrs_15));
    double depr_fedbas_percent_qual_macrs_15 = (itc_fed_qual_total > 0) ? 100.0 * itc_fed_qual_macrs_15 / itc_fed_qual_total : 0.0;
    cm->assign("depr_fedbas_percent_qual_macrs_15", var_data((ssc_number_t)depr_fedbas_percent_qual_macrs_15));
    cm->assign("depr_fedbas_percent_amount_macrs_15", var_data((ssc_number_t)(depr_fedbas_percent_qual_macrs_15 / 100.0 * itc_fed_per)));
    cm->assign("itc_disallow_fed_percent_macrs_15", var_data((ssc_number_t)itc_disallow_fed_percent_macrs_15));
    cm->assign("depr_fedbas_fixed_amount_macrs_15", var_data((ssc_number_t)(depr_fedbas_percent_qual_macrs_15 / 100.0 * itc_fed_amount)));
    cm->assign("itc_disallow_fed_fixed_macrs_15", var_data((ssc_number_t)itc_disallow_fed_fixed_macrs_15));
    double depr_fedbas_itc_sta_reduction_macrs_15 = itc_sta_percent_deprbas_fed * itc_disallow_sta_percent_macrs_15 + itc_sta_amount_deprbas_fed * itc_disallow_sta_fixed_macrs_15;
    double depr_fedbas_itc_fed_reduction_macrs_15 = itc_fed_percent_deprbas_fed * itc_disallow_fed_percent_macrs_15 + itc_fed_amount_deprbas_fed * itc_disallow_fed_fixed_macrs_15;
    cm->assign("depr_fedbas_itc_sta_reduction_macrs_15", var_data((ssc_number_t)depr_fedbas_itc_sta_reduction_macrs_15));
    cm->assign("depr_fedbas_itc_fed_reduction_macrs_15", var_data((ssc_number_t)depr_fedbas_itc_fed_reduction_macrs_15));
    cm->assign("depr_fedbas_after_itc_macrs_15", var_data((ssc_number_t)(depr_fedbas_macrs_15 + depr_fedbas_macrs_15_bonus)));
    cm->assign("depr_fedbas_first_year_bonus_macrs_15", var_data((ssc_number_t)depr_fedbas_macrs_15_bonus));
    cm->assign("depr_fedbas_macrs_15", var_data((ssc_number_t)depr_fedbas_macrs_15));

    cm->assign("depr_fedbas_percent_sl_5", var_data((ssc_number_t)(depr_fedbas_sl_5_frac * 100.0)));
    cm->assign("depr_alloc_sl_5", var_data((ssc_number_t)depr_alloc_sl_5));
    double depr_fedbas_ibi_reduc_sl_5 = depr_fedbas_sl_5_frac * depr_fed_reduction_ibi;
    double depr_fedbas_cbi_reduc_sl_5 = depr_fedbas_sl_5_frac * depr_fed_reduction_cbi;
    cm->assign("depr_fedbas_ibi_reduc_sl_5", var_data((ssc_number_t)depr_fedbas_ibi_reduc_sl_5));
    cm->assign("depr_fedbas_cbi_reduc_sl_5", var_data((ssc_number_t)depr_fedbas_cbi_reduc_sl_5));
    cm->assign("depr_fedbas_prior_itc_sl_5", var_data((ssc_number_t)(depr_alloc_sl_5 - depr_fedbas_ibi_reduc_sl_5 - depr_fedbas_cbi_reduc_sl_5)));
    cm->assign("itc_fed_qual_sl_5", var_data((ssc_number_t)itc_fed_qual_sl_5));
    double depr_fedbas_percent_qual_sl_5 = (itc_fed_qual_total > 0) ? 100.0 * itc_fed_qual_sl_5 / itc_fed_qual_total : 0.0;
    cm->assign("depr_fedbas_percent_qual_sl_5", var_data((ssc_number_t)depr_fedbas_percent_qual_sl_5));
    cm->assign("depr_fedbas_percent_amount_sl_5", var_data((ssc_number_t)(depr_fedbas_percent_qual_sl_5 / 100.0 * itc_fed_per)));
    cm->assign("itc_disallow_fed_percent_sl_5", var_data((ssc_number_t)itc_disallow_fed_percent_sl_5));
    cm->assign("depr_fedbas_fixed_amount_sl_5", var_data((ssc_number_t)(depr_fedbas_percent_qual_sl_5 / 100.0 * itc_fed_amount)));
    cm->assign("itc_disallow_fed_fixed_sl_5", var_data((ssc_number_t)itc_disallow_fed_fixed_sl_5));
    double depr_fedbas_itc_sta_reduction_sl_5 = itc_sta_percent_deprbas_fed * itc_disallow_sta_percent_sl_5 + itc_sta_amount_deprbas_fed * itc_disallow_sta_fixed_sl_5;
    double depr_fedbas_itc_fed_reduction_sl_5 = itc_fed_percent_deprbas_fed * itc_disallow_fed_percent_sl_5 + itc_fed_amount_deprbas_fed * itc_disallow_fed_fixed_sl_5;
    cm->assign("depr_fedbas_itc_sta_reduction_sl_5", var_data((ssc_number_t)depr_fedbas_itc_sta_reduction_sl_5));
    cm->assign("depr_fedbas_itc_fed_reduction_sl_5", var_data((ssc_number_t)depr_fedbas_itc_fed_reduction_sl_5));
    cm->assign("depr_fedbas_after_itc_sl_5", var_data((ssc_number_t)(depr_fedbas_sl_5 + depr_fedbas_sl_5_bonus)));
    cm->assign("depr_fedbas_first_year_bonus_sl_5", var_data((ssc_number_t)depr_fedbas_sl_5_bonus));
    cm->assign("depr_fedbas_sl_5", var_data((ssc_number_t)depr_fedbas_sl_5));

    cm->assign("depr_fedbas_percent_sl_15", var_data((ssc_number_t)(depr_fedbas_sl_15_frac * 100.0)));
    cm->assign("depr_alloc_sl_15", var_data((ssc_number_t)depr_alloc_sl_15));
    double depr_fedbas_ibi_reduc_sl_15 = depr_fedbas_sl_15_frac * depr_fed_reduction_ibi;
    double depr_fedbas_cbi_reduc_sl_15 = depr_fedbas_sl_15_frac * depr_fed_reduction_cbi;
    cm->assign("depr_fedbas_ibi_reduc_sl_15", var_data((ssc_number_t)depr_fedbas_ibi_reduc_sl_15));
    cm->assign("depr_fedbas_cbi_reduc_sl_15", var_data((ssc_number_t)depr_fedbas_cbi_reduc_sl_15));
    cm->assign("depr_fedbas_prior_itc_sl_15", var_data((ssc_number_t)(depr_alloc_sl_15 - depr_fedbas_ibi_reduc_sl_15 - depr_fedbas_cbi_reduc_sl_15)));
    cm->assign("itc_fed_qual_sl_15", var_data((ssc_number_t)itc_fed_qual_sl_15));
    double depr_fedbas_percent_qual_sl_15 = (itc_fed_qual_total > 0) ? 100.0 * itc_fed_qual_sl_15 / itc_fed_qual_total : 0.0;
    cm->assign("depr_fedbas_percent_qual_sl_15", var_data((ssc_number_t)depr_fedbas_percent_qual_sl_15));
    cm->assign("depr_fedbas_percent_amount_sl_15", var_data((ssc_number_t)(depr_fedbas_percent_qual_sl_15 / 100.0 * itc_fed_per)));
    cm->assign("itc_disallow_fed_percent_sl_15", var_data((ssc_number_t)itc_disallow_fed_percent_sl_15));
    cm->assign("depr_fedbas_fixed_amount_sl_15", var_data((ssc_number_t)(depr_fedbas_percent_qual_sl_15 / 100.0 * itc_fed_amount)));
    cm->assign("itc_disallow_fed_fixed_sl_15", var_data((ssc_number_t)itc_disallow_fed_fixed_sl_15));
    double depr_fedbas_itc_sta_reduction_sl_15 = itc_sta_percent_deprbas_fed * itc_disallow_sta_percent_sl_15 + itc_sta_amount_deprbas_fed * itc_disallow_sta_fixed_sl_15;
    double depr_fedbas_itc_fed_reduction_sl_15 = itc_fed_percent_deprbas_fed * itc_disallow_fed_percent_sl_15 + itc_fed_amount_deprbas_fed * itc_disallow_fed_fixed_sl_15;
    cm->assign("depr_fedbas_itc_sta_reduction_sl_15", var_data((ssc_number_t)depr_fedbas_itc_sta_reduction_sl_15));
    cm->assign("depr_fedbas_itc_fed_reduction_sl_15", var_data((ssc_number_t)depr_fedbas_itc_fed_reduction_sl_15));
    cm->assign("depr_fedbas_after_itc_sl_15", var_data((ssc_number_t)(depr_fedbas_sl_15 + depr_fedbas_sl_15_bonus)));
    cm->assign("depr_fedbas_first_year_bonus_sl_15", var_data((ssc_number_t)depr_fedbas_sl_15_bonus));
    cm->assign("depr_fedbas_sl_15", var_data((ssc_number_t)depr_fedbas_sl_15));

    cm->assign("depr_fedbas_percent_sl_20", var_data((ssc_number_t)(depr_fedbas_sl_20_frac * 100.0)));
    cm->assign("depr_alloc_sl_20", var_data((ssc_number_t)depr_alloc_sl_20));
    double depr_fedbas_ibi_reduc_sl_20 = depr_fedbas_sl_20_frac * depr_fed_reduction_ibi;
    double depr_fedbas_cbi_reduc_sl_20 = depr_fedbas_sl_20_frac * depr_fed_reduction_cbi;
    cm->assign("depr_fedbas_ibi_reduc_sl_20", var_data((ssc_number_t)depr_fedbas_ibi_reduc_sl_20));
    cm->assign("depr_fedbas_cbi_reduc_sl_20", var_data((ssc_number_t)depr_fedbas_cbi_reduc_sl_20));
    cm->assign("depr_fedbas_prior_itc_sl_20", var_data((ssc_number_t)(depr_alloc_sl_20 - depr_fedbas_ibi_reduc_sl_20 - depr_fedbas_cbi_reduc_sl_20)));
    cm->assign("itc_fed_qual_sl_20", var_data((ssc_number_t)itc_fed_qual_sl_20));
    double depr_fedbas_percent_qual_sl_20 = (itc_fed_qual_total > 0) ? 100.0 * itc_fed_qual_sl_20 / itc_fed_qual_total : 0.0;
    cm->assign("depr_fedbas_percent_qual_sl_20", var_data((ssc_number_t)depr_fedbas_percent_qual_sl_20));
    cm->assign("depr_fedbas_percent_amount_sl_20", var_data((ssc_number_t)(depr_fedbas_percent_qual_sl_20 / 100.0 * itc_fed_per)));
    cm->assign("itc_disallow_fed_percent_sl_20", var_data((ssc_number_t)itc_disallow_fed_percent_sl_20));
    cm->assign("depr_fedbas_fixed_amount_sl_20", var_data((ssc_number_t)(depr_fedbas_percent_qual_sl_20 / 100.0 * itc_fed_amount)));
    cm->assign("itc_disallow_fed_fixed_sl_20", var_data((ssc_number_t)itc_disallow_fed_fixed_sl_20));
    double depr_fedbas_itc_sta_reduction_sl_20 = itc_sta_percent_deprbas_fed * itc_disallow_sta_percent_sl_20 + itc_sta_amount_deprbas_fed * itc_disallow_sta_fixed_sl_20;
    double depr_fedbas_itc_fed_reduction_sl_20 = itc_fed_percent_deprbas_fed * itc_disallow_fed_percent_sl_20 + itc_fed_amount_deprbas_fed * itc_disallow_fed_fixed_sl_20;
    cm->assign("depr_fedbas_itc_sta_reduction_sl_20", var_data((ssc_number_t)depr_fedbas_itc_sta_reduction_sl_20));
    cm->assign("depr_fedbas_itc_fed_reduction_sl_20", var_data((ssc_number_t)depr_fedbas_itc_fed_reduction_sl_20));
    cm->assign("depr_fedbas_after_itc_sl_20", var_data((ssc_number_t)(depr_fedbas_sl_20 + depr_fedbas_sl_20_bonus)));
    cm->assign("depr_fedbas_first_year_bonus_sl_20", var_data((ssc_number_t)depr_fedbas_sl_20_bonus));
    cm->assign("depr_fedbas_sl_20", var_data((ssc_number_t)depr_fedbas_sl_20));

    cm->assign("depr_fedbas_percent_sl_39", var_data((ssc_number_t)(depr_fedbas_sl_39_frac * 100.0)));
    cm->assign("depr_alloc_sl_39", var_data((ssc_number_t)depr_alloc_sl_39));
    double depr_fedbas_ibi_reduc_sl_39 = depr_fedbas_sl_39_frac * depr_fed_reduction_ibi;
    double depr_fedbas_cbi_reduc_sl_39 = depr_fedbas_sl_39_frac * depr_fed_reduction_cbi;
    cm->assign("depr_fedbas_ibi_reduc_sl_39", var_data((ssc_number_t)depr_fedbas_ibi_reduc_sl_39));
    cm->assign("depr_fedbas_cbi_reduc_sl_39", var_data((ssc_number_t)depr_fedbas_cbi_reduc_sl_39));
    cm->assign("depr_fedbas_prior_itc_sl_39", var_data((ssc_number_t)(depr_alloc_sl_39 - depr_fedbas_ibi_reduc_sl_39 - depr_fedbas_cbi_reduc_sl_39)));
    cm->assign("itc_fed_qual_sl_39", var_data((ssc_number_t)itc_fed_qual_sl_39));
    double depr_fedbas_percent_qual_sl_39 = (itc_fed_qual_total > 0) ? 100.0 * itc_fed_qual_sl_39 / itc_fed_qual_total : 0.0;
    cm->assign("depr_fedbas_percent_qual_sl_39", var_data((ssc_number_t)depr_fedbas_percent_qual_sl_39));
    cm->assign("depr_fedbas_percent_amount_sl_39", var_data((ssc_number_t)(depr_fedbas_percent_qual_sl_39 / 100.0 * itc_fed_per)));
    cm->assign("itc_disallow_fed_percent_sl_39", var_data((ssc_number_t)itc_disallow_fed_percent_sl_39));
    cm->assign("depr_fedbas_fixed_amount_sl_39", var_data((ssc_number_t)(depr_fedbas_percent_qual_sl_39 / 100.0 * itc_fed_amount)));
    cm->assign("itc_disallow_fed_fixed_sl_39", var_data((ssc_number_t)itc_disallow_fed_fixed_sl_39));
    double depr_fedbas_itc_sta_reduction_sl_39 = itc_sta_percent_deprbas_fed * itc_disallow_sta_percent_sl_39 + itc_sta_amount_deprbas_fed * itc_disallow_sta_fixed_sl_39;
    double depr_fedbas_itc_fed_reduction_sl_39 = itc_fed_percent_deprbas_fed * itc_disallow_fed_percent_sl_39 + itc_fed_amount_deprbas_fed * itc_disallow_fed_fixed_sl_39;
    cm->assign("depr_fedbas_itc_sta_reduction_sl_39", var_data((ssc_number_t)depr_fedbas_itc_sta_reduction_sl_39));
    cm->assign("depr_fedbas_itc_fed_reduction_sl_39", var_data((ssc_number_t)depr_fedbas_itc_fed_reduction_sl_39));
    cm->assign("depr_fedbas_after_itc_sl_39", var_data((ssc_number_t)(depr_fedbas_sl_39 + depr_fedbas_sl_39_bonus)));
    cm->assign("depr_fedbas_first_year_bonus_sl_39", var_data((ssc_number_t)depr_fedbas_sl_39_bonus));
    cm->assign("depr_fedbas_sl_39", var_data((ssc_number_t)depr_fedbas_sl_39));

    cm->assign("depr_fedbas_percent_custom", var_data((ssc_number_t)(depr_fedbas_custom_frac * 100.0)));
    cm->assign("depr_alloc_custom", var_data((ssc_number_t)depr_alloc_custom));
    double depr_fedbas_ibi_reduc_custom = depr_fedbas_custom_frac * depr_fed_reduction_ibi;
    double depr_fedbas_cbi_reduc_custom = depr_fedbas_custom_frac * depr_fed_reduction_cbi;
    cm->assign("depr_fedbas_ibi_reduc_custom", var_data((ssc_number_t)depr_fedbas_ibi_reduc_custom));
    cm->assign("depr_fedbas_cbi_reduc_custom", var_data((ssc_number_t)depr_fedbas_cbi_reduc_custom));
    cm->assign("depr_fedbas_prior_itc_custom", var_data((ssc_number_t)(depr_alloc_custom - depr_fedbas_ibi_reduc_custom - depr_fedbas_cbi_reduc_custom)));
    cm->assign("itc_fed_qual_custom", var_data((ssc_number_t)itc_fed_qual_custom));
    double depr_fedbas_percent_qual_custom = (itc_fed_qual_total > 0) ? 100.0 * itc_fed_qual_custom / itc_fed_qual_total : 0.0;
    cm->assign("depr_fedbas_percent_qual_custom", var_data((ssc_number_t)depr_fedbas_percent_qual_custom));
    cm->assign("depr_fedbas_percent_amount_custom", var_data((ssc_number_t)(depr_fedbas_percent_qual_custom / 100.0 * itc_fed_per)));
    cm->assign("itc_disallow_fed_percent_custom", var_data((ssc_number_t)itc_disallow_fed_percent_custom));
    cm->assign("depr_fedbas_fixed_amount_custom", var_data((ssc_number_t)(depr_fedbas_percent_qual_custom / 100.0 * itc_fed_amount)));
    cm->assign("itc_disallow_fed_fixed_custom", var_data((ssc_number_t)itc_disallow_fed_fixed_custom));
    double depr_fedbas_itc_sta_reduction_custom = itc_sta_percent_deprbas_fed * itc_disallow_sta_percent_custom + itc_sta_amount_deprbas_fed * itc_disallow_sta_fixed_custom;
    double depr_fedbas_itc_fed_reduction_custom = itc_fed_percent_deprbas_fed * itc_disallow_fed_percent_custom + itc_fed_amount_deprbas_fed * itc_disallow_fed_fixed_custom;
    cm->assign("depr_fedbas_itc_sta_reduction_custom", var_data((ssc_number_t)depr_fedbas_itc_sta_reduction_custom));
    cm->assign("depr_fedbas_itc_fed_reduction_custom", var_data((ssc_number_t)depr_fedbas_itc_fed_reduction_custom));
    cm->assign("depr_fedbas_after_itc_custom", var_data((ssc_number_t)(depr_fedbas_custom + depr_fedbas_custom_bonus)));
    cm->assign("depr_fedbas_first_year_bonus_custom", var_data((ssc_number_t)depr_fedbas_custom_bonus));
    cm->assign("depr_fedbas_custom", var_data((ssc_number_t)depr_fedbas_custom));


    cm->assign("depr_fedbas_percent_total", var_data((ssc_number_t)(100.0 * (depr_fedbas_macrs_5_frac + depr_fedbas_macrs_15_frac + depr_fedbas_sl_5_frac + depr_fedbas_sl_15_frac + depr_fedbas_sl_20_frac + depr_fedbas_sl_39_frac + depr_fedbas_custom_frac))));
    cm->assign("depr_alloc_total", var_data((ssc_number_t)depr_alloc_total));
    cm->assign("depr_fedbas_ibi_reduc_total", var_data((ssc_number_t)depr_fed_reduction_ibi));
    cm->assign("depr_fedbas_cbi_reduc_total", var_data((ssc_number_t)depr_fed_reduction_cbi));
    cm->assign("depr_fedbas_prior_itc_total", var_data((ssc_number_t)(depr_alloc_total - depr_fed_reduction_ibi - depr_fed_reduction_cbi)));
    cm->assign("itc_sta_qual_total", var_data((ssc_number_t)itc_sta_qual_total));
    cm->assign("depr_fedbas_percent_qual_total", var_data((ssc_number_t)100.0));
    cm->assign("depr_fedbas_percent_amount_total", var_data((ssc_number_t)itc_fed_per));
    cm->assign("itc_disallow_fed_percent_total", var_data((ssc_number_t)(itc_disallow_fed_percent_macrs_5 + itc_disallow_fed_percent_macrs_15 + itc_disallow_fed_percent_sl_5 + itc_disallow_fed_percent_sl_15 + itc_disallow_fed_percent_sl_20 + itc_disallow_fed_percent_sl_39 + itc_disallow_fed_percent_custom)));
    cm->assign("depr_fedbas_fixed_amount_total", var_data((ssc_number_t)itc_fed_amount));
    cm->assign("itc_disallow_fed_fixed_total", var_data((ssc_number_t)(itc_disallow_fed_fixed_macrs_5 + itc_disallow_fed_fixed_macrs_15 + itc_disallow_fed_fixed_sl_5 + itc_disallow_fed_fixed_sl_15 + itc_disallow_fed_fixed_sl_20 + itc_disallow_fed_fixed_sl_39 + itc_disallow_fed_fixed_custom)));
    double depr_fedbas_itc_sta_reduction_total = depr_fedbas_itc_sta_reduction_macrs_5 + depr_fedbas_itc_sta_reduction_macrs_15 + depr_fedbas_itc_sta_reduction_sl_5 + depr_fedbas_itc_sta_reduction_sl_15 + depr_fedbas_itc_sta_reduction_sl_20 + depr_fedbas_itc_sta_reduction_sl_39 + depr_fedbas_itc_sta_reduction_custom;
    cm->assign("depr_fedbas_itc_sta_reduction_total", var_data((ssc_number_t)depr_fedbas_itc_sta_reduction_total));
    double depr_fedbas_itc_fed_reduction_total = depr_fedbas_itc_fed_reduction_macrs_5 + depr_fedbas_itc_fed_reduction_macrs_15 + depr_fedbas_itc_fed_reduction_sl_5 + depr_fedbas_itc_fed_reduction_sl_15 + depr_fedbas_itc_fed_reduction_sl_20 + depr_fedbas_itc_fed_reduction_sl_39 + depr_fedbas_itc_fed_reduction_custom;
    cm->assign("depr_fedbas_itc_fed_reduction_total", var_data((ssc_number_t)depr_fedbas_itc_fed_reduction_total));
    double depr_fedbas_first_year_bonus_total = depr_fedbas_macrs_5_bonus + depr_fedbas_macrs_15_bonus + depr_fedbas_sl_5_bonus + depr_fedbas_sl_15_bonus + depr_fedbas_sl_20_bonus + depr_fedbas_sl_39_bonus + depr_fedbas_custom_bonus;
    cm->assign("depr_fedbas_after_itc_total", var_data((ssc_number_t)(depr_fedbas_total + depr_fedbas_first_year_bonus_total)));
    cm->assign("depr_fedbas_first_year_bonus_total", var_data((ssc_number_t)depr_fedbas_first_year_bonus_total));
    cm->assign("depr_fedbas_total", var_data((ssc_number_t)depr_fedbas_total));

    cm->assign("depr_alloc_none_percent", var_data((ssc_number_t)(depr_alloc_none_frac * 100.0)));
    cm->assign("depr_alloc_none", var_data((ssc_number_t)depr_alloc_none));
    cm->assign("depr_alloc_total", var_data((ssc_number_t)depr_alloc_total));

    // SAM 1038
    save_cf(cm, m_cf, CF_itc_fed_amount, nyears, "cf_itc_fed_amount");
    save_cf(cm, m_cf, CF_itc_fed_percent_amount, nyears, "cf_itc_fed_percent_amount");
    save_cf(cm, m_cf, CF_itc_fed, nyears, "cf_itc_fed");
    save_cf(cm, m_cf, CF_itc_sta_amount, nyears, "cf_itc_sta_amount");
    save_cf(cm, m_cf, CF_itc_sta_percent_amount, nyears, "cf_itc_sta_percent_amount");
    save_cf(cm, m_cf, CF_itc_sta, nyears, "cf_itc_sta");
    save_cf(cm, m_cf, CF_itc_total, nyears, "cf_itc_total");
}

ssc_number_t itc_and_depreciation_calculations::get_stadepr(int i) const {
    return m_cf.at(CF_stadepr_total, i);
}
ssc_number_t  itc_and_depreciation_calculations::get_feddepr(int i) const {
    return m_cf.at(CF_feddepr_total, i);
}

ssc_number_t itc_and_depreciation_calculations::get_itc_sta(int i) const {
    return m_cf.at(CF_itc_sta, i);
}
ssc_number_t itc_and_depreciation_calculations::get_itc_fed(int i) const {
    return m_cf.at(CF_itc_fed, i);
}

ssc_number_t itc_and_depreciation_calculations::get_itc_total(int i) const {
    return m_cf.at(CF_itc_total, i);
}

// std lib
void itc_and_depreciation_calculations::depreciation_sched_5_year_macrs_half_year(int cf_line)
{
    for (int i = 1; i <= nyears; i++)
    {
        double factor = 0.0;
        switch (i)
        {
        case 1: factor = 0.2000; break;
        case 2: factor = 0.3200; break;
        case 3: factor = 0.1920; break;
        case 4: factor = 0.1152; break;
        case 5: factor = 0.1152; break;
        case 6: factor = 0.0576; break;
        default: factor = 0.0; break;
        }
        m_cf.at(cf_line, i) = factor;
    }
}
// std lib
void itc_and_depreciation_calculations::depreciation_sched_15_year_macrs_half_year(int cf_line)
{
    for (int i = 1; i <= nyears; i++)
    {
        double factor = 0.0;
        switch (i)
        {
        case 1: factor = 0.0500; break;
        case 2: factor = 0.0950; break;
        case 3: factor = 0.0855; break;
        case 4: factor = 0.0770; break;
        case 5: factor = 0.0693; break;
        case 6: factor = 0.0623; break;
        case 7: factor = 0.0590; break;
        case 8: factor = 0.0590; break;
        case 9: factor = 0.0591; break;
        case 10: factor = 0.0590; break;
        case 11: factor = 0.0591; break;
        case 12: factor = 0.0590; break;
        case 13: factor = 0.0591; break;
        case 14: factor = 0.0590; break;
        case 15: factor = 0.0591; break;
        case 16: factor = 0.0295; break;
        default: factor = 0.0; break;
        }
        m_cf.at(cf_line, i) = factor;
    }
}
// std lib
void itc_and_depreciation_calculations::depreciation_sched_5_year_straight_line_half_year(int cf_line)
{
    for (int i = 1; i <= nyears; i++)
    {
        double factor = 0.0;
        switch (i)
        {
        case 1: factor = 0.1000; break;
        case 2: factor = 0.2000; break;
        case 3: factor = 0.2000; break;
        case 4: factor = 0.2000; break;
        case 5: factor = 0.2000; break;
        case 6: factor = 0.1000; break;
        default: factor = 0.0; break;
        }
        m_cf.at(cf_line, i) = factor;
    }
}
// std lib
void itc_and_depreciation_calculations::depreciation_sched_15_year_straight_line_half_year(int cf_line)
{
    for (int i = 1; i <= nyears; i++)
    {
        double factor = 0.0;
        switch (i)
        {
        case 1: factor = 0.0333; break;
        case 2: factor = 0.0667; break;
        case 3: factor = 0.0667; break;
        case 4: factor = 0.0667; break;
        case 5: factor = 0.0667; break;
        case 6: factor = 0.0667; break;
        case 7: factor = 0.0667; break;
        case 8: factor = 0.0666; break;
        case 9: factor = 0.0667; break;
        case 10: factor = 0.0666; break;
        case 11: factor = 0.0667; break;
        case 12: factor = 0.0666; break;
        case 13: factor = 0.0667; break;
        case 14: factor = 0.0666; break;
        case 15: factor = 0.0667; break;
        case 16: factor = 0.0333; break;

        default: factor = 0.0; break;
        }
        m_cf.at(cf_line, i) = factor;
    }
}
// std lib
void itc_and_depreciation_calculations::depreciation_sched_20_year_straight_line_half_year(int cf_line)
{
    for (int i = 1; i <= nyears; i++)
    {
        double factor = 0.0;
        switch (i)
        {
        case 1: factor = 0.0250; break;
        case 2: factor = 0.0500; break;
        case 3: factor = 0.0500; break;
        case 4: factor = 0.0500; break;
        case 5: factor = 0.0500; break;
        case 6: factor = 0.0500; break;
        case 7: factor = 0.0500; break;
        case 8: factor = 0.0500; break;
        case 9: factor = 0.0500; break;
        case 10: factor = 0.0500; break;
        case 11: factor = 0.0500; break;
        case 12: factor = 0.0500; break;
        case 13: factor = 0.0500; break;
        case 14: factor = 0.0500; break;
        case 15: factor = 0.0500; break;
        case 16: factor = 0.0500; break;
        case 17: factor = 0.0500; break;
        case 18: factor = 0.0500; break;
        case 19: factor = 0.0500; break;
        case 20: factor = 0.0500; break;
        case 21: factor = 0.0250; break;
        default: factor = 0.0; break;
        }
        m_cf.at(cf_line, i) = factor;
    }
}
// std lib
void itc_and_depreciation_calculations::depreciation_sched_39_year_straight_line_half_year(int cf_line)
{
    for (int i = 1; i <= nyears; i++)
    {
        double factor = 0.0;
        double base = 2.56410256410256e-2;
        switch (i)
        {
        case 1: factor = 0.5 * base; break;
        case 2: factor = base; break;
        case 3: factor = base; break;
        case 4: factor = base; break;
        case 5: factor = base; break;
        case 6: factor = base; break;
        case 7: factor = base; break;
        case 8: factor = base; break;
        case 9: factor = base; break;
        case 10: factor = base; break;
        case 11: factor = base; break;
        case 12: factor = base; break;
        case 13: factor = base; break;
        case 14: factor = base; break;
        case 15: factor = base; break;
        case 16: factor = base; break;
        case 17: factor = base; break;
        case 18: factor = base; break;
        case 19: factor = base; break;
        case 20: factor = base; break;
        case 21: factor = base; break;
        case 22: factor = base; break;
        case 23: factor = base; break;
        case 24: factor = base; break;
        case 25: factor = base; break;
        case 26: factor = base; break;
        case 27: factor = base; break;
        case 28: factor = base; break;
        case 29: factor = base; break;
        case 30: factor = base; break;
        case 31: factor = base; break;
        case 32: factor = base; break;
        case 33: factor = base; break;
        case 34: factor = base; break;
        case 35: factor = base; break;
        case 36: factor = base; break;
        case 37: factor = base; break;
        case 38: factor = base; break;
        case 39: factor = base; break;
        case 40: factor = 0.5 * base; break;
        default: factor = 0.0; break;
        }
        m_cf.at(cf_line, i) = factor;
    }
}

void itc_and_depreciation_calculations::depreciation_sched_custom(compute_module* cm, int cf_line, const std::string& custom)
{
    // computes custom percentage schedule 100%
    // if customValue is array then annual schedule of percent
    // if customValue is a single value then that value is used up to 100%
    int i;
    size_t count = 0;
    ssc_number_t* parr = cm->as_array(custom, &count);
    for (i = 1; i <= nyears; i++)
    {
        m_cf.at(cf_line, i) = 0;
    }

    if (count == 1) // single value
    {
        m_cf.at(cf_line, 1) = parr[0] / 100.0;
    }
    else // annual schedule
    {// note schedules begin at year 1 (index 0)
        int scheduleDuration = ((int)count > nyears) ? nyears : (int)count;
        for (i = 1; i <= scheduleDuration; i++)
        {
            m_cf.at(cf_line, i) = parr[i - 1] / 100.0; // percentage to factor
        }
    }
}

// std lib
void itc_and_depreciation_calculations::major_equipment_depreciation(compute_module* cm, util::matrix_t<double>& main_cf, int cf_equipment_expenditure, int expenditure_year, int analysis_period, int cf_equipment_depreciation)
{
    int feddepr_me1 = CF_macrs_5_frac + cm->as_integer("equip_reserve_depr_fed");
    int stadepr_me1 = CF_macrs_5_frac + cm->as_integer("equip_reserve_depr_sta");

    int fed_depr_mcf;
    int sta_depr_mcf;
    switch(cf_equipment_depreciation) {
        case 1:
            fed_depr_mcf = CF_feddepr_me1;
            sta_depr_mcf = CF_stadepr_me1;
            break;
        case 2:
            fed_depr_mcf = CF_feddepr_me2;
            sta_depr_mcf = CF_stadepr_me2;
            break;
        case 3:
            fed_depr_mcf = CF_feddepr_me3;
            sta_depr_mcf = CF_stadepr_me3;
            break;
        default:
            std::stringstream errm;
            errm << "Major equipment depreciation only has 3 slots.Slot " << cf_equipment_depreciation << " is not available.";
            throw exec_error(cm->get_name(), errm.str().c_str());
    }


    // depreciate equipment cost in expenditure_year according to depr_sched schedule subject to cutoff by analysis_period
    if ((expenditure_year > 0) && (expenditure_year <= analysis_period))
    {
        // sign convention from v3 model
        double depreciable_basis = -main_cf.at(cf_equipment_expenditure, expenditure_year);
        for (int i = expenditure_year; i <= analysis_period; i++)
        {
            m_cf.at(fed_depr_mcf, i) += depreciable_basis * m_cf.at(feddepr_me1, i - expenditure_year + 1);
            m_cf.at(sta_depr_mcf, i) += depreciable_basis * m_cf.at(stadepr_me1, i - expenditure_year + 1);
        }

    }
}
