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


#include <cmath>
#include <iterator>
#include <vector>
#include "lib_utility_rate.h"
#include "lib_utility_rate_equations.h"

UtilityRate::UtilityRate(
	bool useRealTimePrices,
    size_t start_day_of_year,
	util::matrix_t<size_t> ecWeekday, 
	util::matrix_t<size_t> ecWeekend, 
	util::matrix_t<double> ecRatesMatrix,
	std::vector<double> ecRealTimeBuy)
{
    m_useRealTimePrices = useRealTimePrices;
    m_start_day_of_year = start_day_of_year;
	m_ecWeekday = ecWeekday;
	m_ecWeekend = ecWeekend;
	m_ecRatesMatrix = ecRatesMatrix;
	m_ecRealTimeBuy = std::move(ecRealTimeBuy);
}

UtilityRate::UtilityRate(const UtilityRate& tmp){
    m_useRealTimePrices = tmp.m_useRealTimePrices;
    m_start_day_of_year = tmp.m_start_day_of_year;
    m_ecWeekday = tmp.m_ecWeekday;
    m_ecWeekend = tmp.m_ecWeekend;
    m_ecRatesMatrix = tmp.m_ecRatesMatrix;
    for (auto& kv : tmp.m_energyTiersPerPeriod) {
        m_energyTiersPerPeriod[kv.first] = kv.second;
    }
    m_ecRealTimeBuy = tmp.m_ecRealTimeBuy;
}

UtilityRateCalculator::UtilityRateCalculator(UtilityRate * rate, size_t stepsPerHour) :
	UtilityRate(*rate)
{
	m_stepsPerHour = stepsPerHour;
	initializeRate();
}

UtilityRateCalculator::UtilityRateCalculator(UtilityRate * rate, size_t stepsPerHour, std::vector<double> loadProfile) :
	UtilityRate(*rate)
{
	m_stepsPerHour = stepsPerHour;
	m_loadProfile = std::move(loadProfile);
	initializeRate();
}

UtilityRateCalculator::UtilityRateCalculator(const UtilityRateCalculator& tmp):
UtilityRate(tmp){
    m_electricBill = tmp.m_electricBill;
    m_stepsPerHour = tmp.m_stepsPerHour;
    for (auto& i : tmp.m_loadProfile)
        m_loadProfile.push_back(i);
    for (auto& i : tmp.m_energyUsagePerPeriod)
        m_energyUsagePerPeriod.push_back(i);
}

void UtilityRateCalculator::initializeRate()
{
	
	if (!m_useRealTimePrices) {
		for (size_t r = 0; r != m_ecRatesMatrix.nrows(); r++)
		{
			size_t period = static_cast<size_t>(m_ecRatesMatrix(r, 0));
			size_t tier = static_cast<size_t>(m_ecRatesMatrix(r, 1));

			// assumers table is in monotonically increasing order
			m_energyTiersPerPeriod[period] = tier;

			if (tier == 1)
				m_energyUsagePerPeriod.push_back(0);
		}
	}
}

void UtilityRateCalculator::updateLoad(double loadPower)
{
	m_loadProfile.push_back(loadPower);
}
void UtilityRateCalculator::calculateEnergyUsagePerPeriod()
{
	for (size_t idx = 0; idx != m_loadProfile.size(); idx++)
	{
		size_t hourOfYear = static_cast<size_t>(std::floor(idx / m_stepsPerHour));
		size_t period = static_cast<size_t>(getEnergyPeriod(hourOfYear));
		m_energyUsagePerPeriod[period] += m_loadProfile[idx];
	}
}
double UtilityRateCalculator::getEnergyRate(size_t hourOfYear)
{

	double rate = 0;
	if (m_useRealTimePrices) {
		rate = m_ecRealTimeBuy[hourOfYear];
	}
	else {
		// period is the human readable value from the table (1-based)
		size_t period = getEnergyPeriod(hourOfYear);

		//size_t idx = m_loadProfile.size() - 1;
		//double energy = m_energyTiersPerPeriod[period];
		// add ability to check for tiered usage, for now assume one tier

		// Reduce period to 0-based index
		rate = m_ecRatesMatrix(period - 1, 4);
	}
	return rate;

}
size_t UtilityRateCalculator::getEnergyPeriod(size_t hourOfYear)
{
	size_t period, month, hour;
	util::month_hour(hourOfYear, month, hour);

	if (util::weekday(hourOfYear, m_start_day_of_year)) {
		if (m_ecWeekday.nrows() == 1 && m_ecWeekday.ncols() == 1) {
			period = m_ecWeekday.at(0, 0);
		}
		else {
			period = m_ecWeekday.at(month - 1, hour - 1);
		}
	}
	else {
		if (m_ecWeekend.nrows() == 1 && m_ecWeekend.ncols() == 1) {
			period = m_ecWeekend.at(0, 0);
		}
		else {
			period = m_ecWeekend.at(month - 1, hour - 1);
		}
	}
	return period;
}

UtilityRateForecast::UtilityRateForecast(rate_data* util_rate, size_t stepsPerHour, const std::vector<double>& monthly_load_forecast, const std::vector<double>& monthly_gen_forecast, const std::vector<double>& monthly_avg_load_forecast, size_t analysis_period, const util::matrix_t<double>& monthly_peak_forecast) :
    current_composite_buy_rates(),
    current_composite_sell_rates(),
    next_composite_buy_rates(),
    next_composite_sell_rates()
{
	steps_per_hour = stepsPerHour;
	dt_hour = 1.0f / stepsPerHour;
	last_step = 0;
    last_month_init = -1;
	rate = std::unique_ptr<rate_data>(new rate_data(*util_rate));
	m_monthly_load_forecast = monthly_load_forecast;
	m_monthly_gen_forecast = monthly_gen_forecast;
	m_monthly_avg_load_forecast = monthly_avg_load_forecast;
    m_peaks_forecast = monthly_peak_forecast;
    nyears = analysis_period;
}

UtilityRateForecast::UtilityRateForecast(UtilityRateForecast& tmp) :
current_composite_sell_rates(tmp.current_composite_sell_rates),
current_composite_buy_rates(tmp.current_composite_buy_rates),
next_composite_sell_rates(tmp.next_composite_sell_rates),
next_composite_buy_rates(tmp.next_composite_buy_rates),
steps_per_hour(tmp.steps_per_hour),
dt_hour(tmp.dt_hour),
last_step(tmp.last_step),
last_month_init(tmp.last_month_init),
nyears(tmp.nyears),
m_monthly_load_forecast(tmp.m_monthly_load_forecast),
m_monthly_gen_forecast(tmp.m_monthly_gen_forecast),
m_monthly_avg_load_forecast(tmp.m_monthly_avg_load_forecast),
m_peaks_forecast(tmp.m_peaks_forecast)
{
//    rate = std::shared_ptr<rate_data>(new rate_data(*tmp.rate));
    rate = std::make_shared<rate_data>(*tmp.rate);
}

UtilityRateForecast::~UtilityRateForecast() {}

double UtilityRateForecast::forecastCost(std::vector<double>& predicted_loads, size_t year, size_t hour_of_year, size_t step)
{
	double cost = 0;
	int month = util::month_of((double) hour_of_year) - 1;
	size_t lifeTimeIndex = util::lifetimeIndex(year, hour_of_year, step, steps_per_hour);

	size_t n = predicted_loads.size();

	// Determine if this forecast crosses a month
	size_t index_at_end = util::yearOneIndex(dt_hour, lifeTimeIndex + n);
	int month_at_end = util::month_of((double) index_at_end / (double) steps_per_hour) - 1;

	bool crossing_month = month != month_at_end;
	size_t year_at_end = year;
	if (month_at_end < month) {
		year_at_end++;
	}
    if (year_at_end >= nyears)
    {
        crossing_month = false;
    }

	// Get previous peak cost - may need to run two months
    rate->set_billing_demands();
	double previousDemandCharge = rate->get_demand_charge(month, year);
    double previousEnergyCharge = 0;
    if (rate->enable_nm)
    {
        previousEnergyCharge = rate->getEnergyChargeNetMetering(month, current_composite_buy_rates, current_composite_sell_rates);
    }
	if (crossing_month)
	{
        initializeMonth(month_at_end, year_at_end);
        previousDemandCharge += rate->get_demand_charge(month_at_end, year_at_end);
	}

    double newEnergyCharge = 0;
    bool use_next_month = false;
    size_t current_year = year;
	for (size_t i = 0; i < n; i++) {
		// Determine if this new step crosses a month
		size_t year_one_index = util::yearOneIndex(dt_hour, lifeTimeIndex);
		int current_month = util::month_of((double) hour_of_year) - 1;
		
		if (current_month != month && !use_next_month)
		{
			use_next_month = true;
            if (rate->enable_nm)
            {
                // restartMonth will change the sign of energyUse from what getEnergyCharge expects, so compute that first
                newEnergyCharge += rate->getEnergyChargeNetMetering(month, current_composite_buy_rates, current_composite_sell_rates);
            }
			// This handles net metering carryover
			restartMonth(month, current_month, year_at_end);
            current_year = year_at_end;
		}
        ur_month& curr_month = rate->m_month[current_month];

		double power = predicted_loads.at(i);
		double energy = predicted_loads.at(i) * dt_hour;

        curr_month.update_net_and_peak(energy, power, year_one_index);
		rate->sort_energy_to_periods(current_month, energy, year_one_index);
		rate->find_dc_tou_peak(current_month, power, year_one_index);

        // Net billing (rates 2 and 3)
        cost += getEnergyChargeNetBillingOrTimeSeries(energy, year_one_index, current_month, current_year, use_next_month);

		step++;
		lifeTimeIndex++;
		if (step == steps_per_hour)
		{
			hour_of_year++;
			if (hour_of_year >= 8760) {
				hour_of_year = 0;
			}
			step = 0;
		}
	}

    // Compute new peak cost - may need to run two months
    rate->set_billing_demands();
    double newDemandCharge = rate->get_demand_charge(month, year);
    // If forecast length is 1, restartMonth won't be triggered on the next forecast. Trigger it now
    if (crossing_month && n == 1)
    {
        if (rate->enable_nm)
        {
            newEnergyCharge += rate->getEnergyChargeNetMetering(month, current_composite_buy_rates, current_composite_sell_rates);
        }
        restartMonth(month, month_at_end, year_at_end);
        copyTOUForecast();
    }

	if (crossing_month)
	{
		newDemandCharge += rate->get_demand_charge(month_at_end, year_at_end);
        if (rate->enable_nm)
        {
            newEnergyCharge += rate->getEnergyChargeNetMetering(month_at_end, next_composite_buy_rates, next_composite_sell_rates);
        }
	}
    else if(rate->enable_nm)
    {
        newEnergyCharge += rate->getEnergyChargeNetMetering(month, current_composite_buy_rates, current_composite_sell_rates);
    }

	cost += newDemandCharge + newEnergyCharge - previousDemandCharge - previousEnergyCharge;

	return cost;
}

// Year indexes from 0
void UtilityRateForecast::compute_next_composite_tou(int month, size_t year)
{
	double expected_load = m_monthly_load_forecast[year * 12 + (size_t) month];
	
	next_composite_buy_rates.clear();
    next_composite_buy_rates = rate->get_composite_tou_buy_rate(month, year, expected_load);

	// repeat for surplus
	double expected_gen = m_monthly_gen_forecast[year * 12 + month];
	next_composite_sell_rates.clear();
    next_composite_sell_rates = rate->get_composite_tou_sell_rate(month, year, expected_gen);
}

// Month is zero indexed
void UtilityRateForecast::initializeMonth(int month, size_t year)
{
	if (last_month_init != month)
	{
		rate->init_dc_peak_vectors(month);

        ur_month& curr_month = rate->m_month[month];

        if (rate->has_kwh_per_kw_rate() || rate->en_billing_demand_lookback) {
            double tou_peak = 0.0;
            for (int period = 0; period < (int)curr_month.dc_periods.size(); period++)
            {
                tou_peak = m_peaks_forecast[year * 12 + month, period];
                curr_month.dc_tou_peak[period] = tou_peak;
                if (tou_peak > curr_month.dc_flat_peak) {
                    curr_month.dc_flat_peak = tou_peak;
                }
            }
            double avg_load = m_monthly_avg_load_forecast[year * 12 + month];
            if (avg_load > curr_month.dc_flat_peak) {
                curr_month.dc_flat_peak = avg_load; // Choose greater of average load or peak minus battery discharge capacity
            }

        }
        else { // Standard demand charges
            // Ignore any peak charges lower than the average gross load - this prevents the retail rate forcast from showing demand charges on the first hour of each month when the load is not really a peak
            double avg_load = m_monthly_avg_load_forecast[year * 12 + month];
            curr_month.dc_flat_peak = avg_load;
            for (int period = 0; period < (int)curr_month.dc_periods.size(); period++)
            {
                curr_month.dc_tou_peak[period] = avg_load;
            }
        }

        rate->init_energy_rates(false, month);
        compute_next_composite_tou(month, year);
        last_month_init = month;
	}
}

void UtilityRateForecast::copyTOUForecast()
{
    current_composite_buy_rates.clear();
    current_composite_sell_rates.clear();
	std::copy(next_composite_buy_rates.begin(), next_composite_buy_rates.end(), std::back_inserter(current_composite_buy_rates));
	std::copy(next_composite_sell_rates.begin(), next_composite_sell_rates.end(), std::back_inserter(current_composite_sell_rates));
}

void UtilityRateForecast::restartMonth(int prevMonth, int currentMonth, size_t year)
{
    ur_month& prev_month = rate->m_month[prevMonth];
    ur_month& curr_month = rate->m_month[currentMonth];
    rate->compute_surplus(prev_month);

    bool skip_rollover = (currentMonth == 0 && year == 0) || (currentMonth == rate->net_metering_credit_month + 1) || (currentMonth == 0 && rate->net_metering_credit_month == 11);
    if (!skip_rollover && rate->nm_credits_w_rollover)
    {
        rate->transfer_surplus(curr_month, prev_month);
    }
    prev_month.reset();
}

double UtilityRateForecast::getEnergyChargeNetBillingOrTimeSeries(double energy, size_t year_one_index, int current_month, size_t year, bool use_next_month)
{
    double cost = 0;
    // If the below is true, this function does nothing, so return zero early
    if (rate->enable_nm && !rate->en_ts_buy_rate && !rate->en_ts_sell_rate) {
        return cost;
    }
    int tou_period = rate->get_tou_row(year_one_index, current_month);
    size_t rate_index = year < rate->rate_scale.size() ? year : rate->rate_scale.size() - 1;
    ssc_number_t rate_esc = rate->rate_scale[rate_index];
    if (energy < 0)
    {
        if (rate->en_ts_buy_rate)
        {
            cost += rate->m_ec_ts_buy_rate[year_one_index] * -energy * rate_esc;
        }
        else if (!rate->enable_nm)
        {
            if (use_next_month)
            {
                cost += next_composite_buy_rates[tou_period] * -energy; // rate esclation is handled in compute_next_composite_tou
            }
            else
            {
                cost += current_composite_buy_rates[tou_period] * -energy;
            }
        }
    }
    else
    {
        if (rate->en_ts_sell_rate)
        {
            cost += rate->m_ec_ts_sell_rate[year_one_index] * -energy * rate_esc;
        }
        else if (!rate->enable_nm)
        {
            if (use_next_month)
            {
                cost += next_composite_sell_rates[tou_period] * -energy;
            }
            else
            {
                cost += current_composite_sell_rates[tou_period] * -energy;
            }
        }
    }
    return cost;
}
