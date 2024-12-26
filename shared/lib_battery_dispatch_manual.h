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

#ifndef __LIB_BATTERY_DISPATCH_MANUAL_H__
#define __LIB_BATTERY_DISPATCH_MANUAL_H__

#include "lib_battery_dispatch.h"

/*
Manual dispatch class
*/
class dispatch_manual_t : public dispatch_t
{
public:
	dispatch_manual_t(battery_t * Battery,
		double dt_hour,
		double SOC_min,
		double SOC_max,
		int current_choice,
		double Ic_max,
		double Id_max,
		double Pc_max_kwdc,
		double Pd_max_kwdc,
		double Pc_max_kwac,
		double Pd_max_kwac,
		double t_min,
		int mode,
		int meterPosition,
		util::matrix_t<size_t> dm_dynamic_sched,
		util::matrix_t<size_t> dm_dynamic_sched_weekend,
		std::vector<bool> can_charge,
		std::vector<bool> can_discharge,
		std::vector<bool> can_gridcharge,
		std::vector<bool> can_fuelcellcharge,
        std::vector<bool> can_discharge_btm_to_grid,
		std::map<size_t, double> dm_percent_discharge,
		std::map<size_t, double> dm_percent_gridcharge,
        bool can_clip_charge,
        bool can_curtail_charge,
        double interconnection_limit,
        bool chargeOnlySystemExceedLoad = true,
        bool dischargeOnlyLoadExceedSystem = true,
        double SOC_min_outage = 0.0,
        bool priorityChargeBattery = false);

	// deep copy constructor (new memory), from dispatch to this
	dispatch_manual_t(const dispatch_t& dispatch);

	// copy members from dispatch to this
	void copy(const dispatch_t * dispatch) override;

	virtual ~dispatch_manual_t(){};

	/// Public API to run the battery dispatch model for the current timestep, given the system power, and optionally the electric load, amount of system clipping, or specified battery power
	virtual void dispatch(size_t year,
		size_t hour_of_year,
		size_t step) override;

protected:

	/// Helper function to internally set up the dispatch model
	virtual void prepareDispatch(size_t hour_of_year, size_t step);

	void init_with_vects(
		util::matrix_t<size_t> dm_dynamic_sched,
		util::matrix_t<size_t> dm_dynamic_sched_weekend,
		std::vector<bool>,
		std::vector<bool>,
		std::vector<bool>,
		std::vector<bool>,
        std::vector<bool>,
		std::map<size_t, double> dm_percent_discharge,
		std::map<size_t, double> dm_percent_gridcharge,
        bool can_clip_charge,
        bool can_curtail_charge,
        bool priorityChargeBattery);

	void SOC_controller() override;
	bool check_constraints(double &I, size_t count) override;

	util::matrix_t < size_t > _sched;
	util::matrix_t < size_t > _sched_weekend;

	std::vector<bool> _charge_array;
	std::vector<bool> _discharge_array;
	std::vector<bool> _gridcharge_array;
	std::vector<bool> _fuelcellcharge_array;
    std::vector<bool> _discharge_grid_array;
    bool _can_clip_charge;
    bool _can_curtail_charge;
    bool _priority_charge_battery;

	double _percent_discharge;
	double _percent_charge;

	std::map<size_t, double> _percent_discharge_array;
	std::map<size_t, double> _percent_charge_array;

};

#endif // __LIB_BATTERY_DISPATCH_MANUAL_H__
