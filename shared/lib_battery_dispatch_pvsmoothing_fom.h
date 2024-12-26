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

#ifndef __LIB_BATTERY_DISPATCH_PVSMOOTHING_FOM_H__
#define __LIB_BATTERY_DISPATCH_PVSMOOTHING_FOM_H__

#include "lib_battery_dispatch.h"

/*! PV Smoothing Front of Meter battery dispatch */
class dispatch_pvsmoothing_front_of_meter_t : public dispatch_automatic_t
{
public:
	/**
	 Class takes forecast information about the PV production and user inputs and constraints
	 and programs battery to strategically dispatch to maximize ramp rates to avoid penalties.
     Developed in conjunction with work from EPRI and Southern Company
	 */
	dispatch_pvsmoothing_front_of_meter_t(
		battery_t * Battery,
		double dt,
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
		int dispatch_mode,
        int weather_forecast_mode,
		int pv_dispatch,
		size_t nyears,
		size_t look_ahead_hours,
		double dispatch_update_frequency_hours,
		bool can_charge,
		bool can_clipcharge,
		bool can_grid_charge,
		bool can_fuelcell_charge,
        bool can_curtail_charge,
		double inverter_paco,
        std::vector<double> battReplacementCostPerkWh, // required for base class
		int battCycleCostChoice,
        std::vector<double> battCycleCost, // required for base class
        std::vector<double> battOMCost, // required for base class
        double etaPVCharge,
		double etaGridCharge,
		double etaDischarge,
        // PVSmoothing inputs
        double batt_dispatch_pvs_nameplate_ac,
        double batt_dispatch_pvs_ac_lb,
        bool batt_dispatch_pvs_ac_lb_enable,
        double batt_dispatch_pvs_ac_ub,
        bool batt_dispatch_pvs_ac_ub_enable,
        bool batt_dispatch_pvs_curtail_as_control,
        bool batt_dispatch_pvs_curtail_if_violation,
        size_t batt_dispatch_pvs_forecast_shift_periods, 
        double batt_dispatch_pvs_kf,
        double batt_dispatch_pvs_ki,
        double batt_dispatch_pvs_kp,
        double batt_dispatch_pvs_max_ramp,
        bool batt_dispatch_pvs_short_forecast_enable,
        double batt_dispatch_pvs_soc_rest,
        size_t batt_dispatch_pvs_timestep_multiplier,
        double interconnection_limit
		);

	~dispatch_pvsmoothing_front_of_meter_t();

	/*! deep copy constructor (new memory), from dispatch to this */
	dispatch_pvsmoothing_front_of_meter_t(const dispatch_t& dispatch);

	/*! shallow copy from dispatch to this */
	void copy(const dispatch_t* dispatch);

	/// Public API to run the battery dispatch model for the current timestep, given the system power, and optionally the electric load, amount of system clipping, or specified battery power
	void dispatch(size_t year,
		size_t hour_of_year,
		size_t step);

	/// Compute the updated power to send to the battery over the next N hours
	void update_dispatch(size_t year, size_t hour_of_year, size_t step, size_t lifetimeIndex);

 
	/// Pass in the PV power forecast [kW]
    void update_pv_data(double_vec P_pv_ac);

	/// Return intermediate calculations for validation - unscale by nameplate = min(system ac rating, grid interconnection limit)
    double batt_dispatch_pvs_outpower() { return  m_batt_dispatch_pvs_nameplate_ac * m_batt_dispatch_pvs_outpower; };
    double batt_dispatch_pvs_battpower() { return   m_batt_dispatch_pvs_nameplate_ac * m_batt_dispatch_pvs_battpower; };
    double batt_dispatch_pvs_battsoc() { return    100.0*  m_batt_dispatch_pvs_battsoc ; };
    double batt_dispatch_pvs_curtail() { return    m_batt_dispatch_pvs_nameplate_ac * m_batt_dispatch_pvs_curtail ; };
    double batt_dispatch_pvs_violation_list() { return m_batt_dispatch_pvs_violation_list; };
    double batt_dispatch_pvs_P_pv_ac() { return m_batt_dispatch_pvs_P_pv_ac; };
    double batt_dispatch_pvs_PV_ramp_interval() { return m_batt_dispatch_pvs_nameplate_ac * m_batt_dispatch_pvs_PV_ramp_interval ; };
    double batt_dispatch_pvs_forecast_pv_energy() { return  m_batt_dispatch_pvs_nameplate_ac * m_batt_dispatch_pvs_forecast_pv_energy ;  };


 protected:

	void init_with_pointer(const dispatch_pvsmoothing_front_of_meter_t* tmp);

    /*! Calculate the cost to cycle per kWh */
    void costToCycle();

    /*! Calculate the O and M cost per kWh for current timestep */
    void omCost();

 	/*! Inverter AC power limit */
	double _inverter_paco;

	/*! Efficiencies of the charge and discharge of the battery*/
	double m_etaPVCharge;
	double m_etaGridCharge;
	double m_etaDischarge;

	/* Computed smoothing outputs */
	double m_batt_dispatch_pvs_outpower;
    double m_batt_dispatch_pvs_battpower;
    double m_batt_dispatch_pvs_battsoc;
    double m_batt_dispatch_pvs_curtail;
	double m_batt_dispatch_pvs_violation_list;
    double m_batt_dispatch_pvs_P_pv_ac;
    double m_batt_dispatch_pvs_PV_ramp_interval;
    double m_batt_dispatch_pvs_forecast_pv_energy;



    // PVSmoothing inputs
    double m_batt_dispatch_pvs_nameplate_ac;
    double m_batt_dispatch_pvs_ac_lb;
    bool m_batt_dispatch_pvs_ac_lb_enable;
    double m_batt_dispatch_pvs_ac_ub;
    bool m_batt_dispatch_pvs_ac_ub_enable;
    bool m_batt_dispatch_pvs_curtail_as_control;
    bool m_batt_dispatch_pvs_curtail_if_violation;
    size_t m_batt_dispatch_pvs_forecast_shift_periods;
    double m_batt_dispatch_pvs_kf;
    double m_batt_dispatch_pvs_ki;
    double m_batt_dispatch_pvs_kp;
    double m_batt_dispatch_pvs_max_ramp;
    bool m_batt_dispatch_pvs_short_forecast_enable;
    double m_batt_dispatch_pvs_soc_rest;
    size_t m_batt_dispatch_pvs_timestep_multiplier;

};

#endif // __LIB_BATTERY_DISPATCH_PVSMOOTHING_FOM_H__
