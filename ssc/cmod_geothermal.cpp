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

#include "core.h"
#include "lib_weatherfile.h"
#include "lib_physics.h"
#include "lib_geothermal.h"
// for adjustment factors
#include "common.h"

#include "cmod_geothermal_costs.h"

#include "cmod_geothermal_costs_eqns.h"

#include "cmod_cb_construction_financing.h"

#include "cmod_geothermal.h"


static bool my_update_function( float percent, void *data )
{
	if ( data != 0 ) 
		return ((compute_module*)data)->update("working...", percent);
	else 
		return true;
}

class cm_geothermal_no_fin : public compute_module
{
public:

    cm_geothermal_no_fin() {

        add_var_info(_cm_vtab_geothermal);

        // performance adjustment factors
        add_var_info(vtab_adjustment_factors);


    }

    void exec() {

        std::string geo_cmod_name = "geothermal";
        ssc_module_t geo_cmod = ssc_module_create(geo_cmod_name.c_str());
        var_table* geo_vtab = this->get_var_table();
        ssc_data_t geo_cmod_inputs = static_cast<ssc_data_t>(geo_vtab);

        assign("calc_drill_costs", 0);

        if( !ssc_module_exec(geo_cmod, geo_cmod_inputs) ) {
            std::string str = geo_cmod_name + " execution error. ";
            int idx = 0;
            int type = -1;
            while( const char* msg = ssc_module_log(geo_cmod, idx++, &type, nullptr) )
            {
                if(/*/(type == SSC_NOTICE) || */(type == SSC_WARNING) || (type == SSC_ERROR) ) {
                    str += std::string(msg);
                    str += "\t";
                }
            }
            ssc_module_free(geo_cmod);
            throw std::runtime_error(str);
        }
        // *************************************************************
        // *************************************************************


    }

};
DEFINE_MODULE_ENTRY(geothermal_no_fin, "Geothermal monthly and hourly models using general power block code from TRNSYS Type 224 code by M.Wagner, and some GETEM model code.", 3);


class cm_geothermal : public compute_module
{
private:
public:

	cm_geothermal()
	{
		add_var_info( _cm_vtab_geothermal );

		// performance adjustment factors
		add_var_info(vtab_adjustment_factors);

        add_var_info(_cm_vtab_geothermal_fin_in);

        add_var_info(_cm_vtab_geothermal_costs_unique);
        add_var_info(_cm_vtab_geothermal_om_costs);
        add_var_info(_cm_vtab_cb_construction_financing_independent);



//		add_var_info(vtab_technology_outputs);

	}

    void exec()
    {
        // Replacing 'ui_calculations_only' with 'sim_typ'
        //"sim_type", "1 (default): timeseries, 2: design only",
        int sim_type = as_integer("sim_type");  // "1 (default): timeseries, 2: design only",

        // so if ui_calculations_only = 0 -> sim_type = 1; if ui_calculations_only = 1 -> sim_type = 2
        //int iControl = as_integer("ui_calculations_only");		 // 0=run full model, 1=just do UI calculations

        // --------------------------------------------------------------
        // Get main system paramters
        double resource_temperature = as_double("resource_temp");    //[C]

        int analysis_type = as_integer("analysis_type");
        int conversion_type = as_integer("conversion_type");
        int resource_type = as_integer("resource_type");

        int geo_fin_model = as_integer("geo_financial_model");

        int decline_type_in = as_integer("decline_type");
        int decline_type = decline_type_in;
        // Apply decline type logic and reset if necessary
        // This logic was on the Plant and Equipment page on 5/24/26
        if( resource_type == HYDROTHERMAL ){
            decline_type = 0;
        }
        else if( conversion_type == BINARY ) {
            decline_type = decline_type_in;
        }
        else{
            decline_type = 0;
        }

        int reservoir_pressure_change_type = as_integer("reservoir_pressure_change_type");
        // Logic from the Plant and Equipment page on 5/24/26
        if( resource_type != 1 && (reservoir_pressure_change_type == 1 || reservoir_pressure_change_type == 3) ) {
            reservoir_pressure_change_type = 0;
        }
        // --------------------------------------------------------------
        // --------------------------------------------------------------


		// set the geothermal model inputs -------------------------------------
		SGeothermal_Inputs geo_inputs;
        geo_inputs.md_RatioInjectionToProduction = as_double("geotherm.cost.inj_prod_well_ratio"); // THIS SHOULD BE AN INPUT. ALTHOUGH IT'S FROM THE COST PAGE, IT'S USED IN NON-COST EQUATION
        geo_inputs.md_DrillSuccessRate = as_double("drilling_success_rate") / 100.0;
        geo_inputs.md_StimSuccessRate = as_double("stim_success_rate") / 100.0;
        geo_inputs.md_FailedProdFlowRatio = as_double("failed_prod_flow_ratio");
        geo_inputs.md_DesiredSalesCapacityKW = as_double("nameplate");
		geo_inputs.md_NumberOfWells = as_double("num_wells");
        geo_inputs.md_WellsStimulated = as_integer("stimulation_type");
		if (analysis_type  == 0)
			geo_inputs.me_cb = POWER_SALES;
		else
			geo_inputs.me_cb = NUMBER_OF_WELLS;

		if ( conversion_type == 0)
			geo_inputs.me_ct = BINARY;
		else if ( conversion_type == 1)
			geo_inputs.me_ct = FLASH;

		switch ( as_integer("conversion_subtype") )
		{
			case 0:	geo_inputs.me_ft = SINGLE_FLASH_NO_TEMP_CONSTRAINT; break;
			case 1:	geo_inputs.me_ft = SINGLE_FLASH_WITH_TEMP_CONSTRAINT; break;
			case 2:	geo_inputs.me_ft = DUAL_FLASH_NO_TEMP_CONSTRAINT; break;
			case 3:	geo_inputs.me_ft = DUAL_FLASH_WITH_TEMP_CONSTRAINT; break;			
		}
		geo_inputs.md_PlantEfficiency = as_double("plant_efficiency_input")/100;

		// temperature decline
		if ( decline_type == 0 )
			geo_inputs.me_tdm = ENTER_RATE;
		else if ( decline_type == 1 )
			geo_inputs.me_tdm = CALCULATE_RATE;

		geo_inputs.md_TemperatureDeclineRate = as_double("temp_decline_rate")/100;
		geo_inputs.md_MaxTempDeclineC = as_double("temp_decline_max");
        geo_inputs.md_dtProdWell = as_double("dt_prod_well");
        geo_inputs.md_dtProdWellChoice = as_double("prod_well_choice");

		// flash inputs
		geo_inputs.md_TemperatureWetBulbC = as_double("wet_bulb_temp");
		geo_inputs.md_PressureAmbientPSI = as_double("ambient_pressure" );
        geo_inputs.md_UseWeatherFileConditions = 0; //initially set to zero for UI calculations

		//pumping parameters
		geo_inputs.md_ProductionFlowRateKgPerS = as_double("well_flow_rate");
		geo_inputs.md_GFPumpEfficiency = as_double("pump_efficiency")/100;
		geo_inputs.md_PressureChangeAcrossSurfaceEquipmentPSI = as_double("delta_pressure_equip");
		geo_inputs.md_ExcessPressureBar = physics::PsiToBar( as_double("excess_pressure_pump") );
		geo_inputs.md_DiameterProductionWellInches = 0.0;
        geo_inputs.md_ProductionWellType = as_double("geotherm.cost.prod_cost_curve_welltype");
        geo_inputs.md_ProductionWellDiam = as_double("geotherm.cost.prod_cost_curve_welldiam");

		geo_inputs.md_DiameterPumpCasingInches = 0.0;
        geo_inputs.md_DiameterInjPumpCasingInches = 0.0;
		geo_inputs.md_DiameterInjectionWellInches = 0.0;
        geo_inputs.md_InjectionWellType = as_double("geotherm.cost.inj_cost_curve_welltype");
        geo_inputs.md_InjectionWellDiam = as_double("geotherm.cost.inj_cost_curve_welldiam");
		geo_inputs.mb_CalculatePumpWork = ( 1 != as_integer("specify_pump_work") );
		geo_inputs.md_UserSpecifiedPumpWorkKW = as_double("specified_pump_work_amount") * 1000; // entered in MW

		//resource characterization
        // Set design temp equal to resource temperature
        double design_temp = resource_temperature;    //[C]
        assign("design_temp", ssc_number_t(design_temp));

		if ( resource_type == 0 )
			geo_inputs.me_rt =  HYDROTHERMAL;
		else if ( resource_type == 1 )
			geo_inputs.me_rt = EGS;

		geo_inputs.md_ResourceDepthM = as_double("resource_depth");
		geo_inputs.md_TemperatureResourceC = resource_temperature;
		geo_inputs.me_dc = TEMPERATURE;
		geo_inputs.md_TemperaturePlantDesignC = design_temp;
		
		

		//reservoir properties
		geo_inputs.md_TemperatureEGSAmbientC = 10.0;
		geo_inputs.md_EGSThermalConductivity = as_double("rock_thermal_conductivity");
		geo_inputs.md_EGSSpecificHeatConstant = as_double("rock_specific_heat");
		geo_inputs.md_EGSRockDensity = as_double("rock_density");
		switch( reservoir_pressure_change_type )
		{
			case 0: geo_inputs.me_pc = ENTER_PC; break;				// pressure change entered by user
			case 1: geo_inputs.me_pc = SIMPLE_FRACTURE; break;		// use fracture flow (EGS only)
			case 2: geo_inputs.me_pc = K_AREA; break;				// permeability * area
            case 3: geo_inputs.me_pc = USER_TEMP; break;
		}
		geo_inputs.md_ReservoirDeltaPressure = as_double("reservoir_pressure_change");
        geo_inputs.md_InjectivityIndex = as_double("injectivity_index");

        double num_conf_wells = as_double("geotherm.cost.conf_num_wells");
        double perc_conf_wells_production = as_double("geotherm.cost.confirm_wells_percent");
        double num_conf_wells_prod = num_conf_wells * perc_conf_wells_production / 100.0;
        geo_inputs.md_ExplorationWellsProd = num_conf_wells_prod;

		geo_inputs.md_ReservoirWidthM = as_double("reservoir_width");
		geo_inputs.md_ReservoirHeightM = as_double("reservoir_height");
		geo_inputs.md_ReservoirPermeability = as_double("reservoir_permeability");
		geo_inputs.md_DistanceBetweenProductionInjectionWellsM = as_double("inj_prod_well_distance");
		geo_inputs.md_WaterLossPercent = as_double("subsurface_water_loss")/100;
		geo_inputs.md_EGSFractureAperature = as_double("fracture_aperature");
        geo_inputs.md_EGSFractureLength = as_double("fracture_length");
        geo_inputs.md_EGSFractureSpacing = as_double("fracture_spacing");
		geo_inputs.md_EGSNumberOfFractures = as_double("num_fractures");
		geo_inputs.md_EGSFractureWidthM = as_double("fracture_width");
		geo_inputs.md_EGSFractureAngle = as_double("fracture_angle");

        geo_inputs.md_AllowReservoirReplacements = as_boolean("allow_reservoir_replacements");

		// calculate output array sizes
		//geo_inputs.mi_ModelChoice = as_integer("model_choice");		                    // 0=GETEM, 1=Power Block monthly, 2=Power Block hourly
        geo_inputs.mi_cycle_model_type = as_integer("geo_cycle_model_type");		            // 0=GETEM, 1=User Defined, 2=Reduced Order
        geo_inputs.mi_simulation_timestep_type = as_integer("simulation_timestep_type");    // 0=monthly, 1=hourly

        if (is_assigned("reservoir_model_inputs")) 
            geo_inputs.md_ReservoirInputs = as_matrix("reservoir_model_inputs");

        bool system_use_lifetime_output = false;
        int analysis_period_years = 1;              // geo_inputs.mi_ProjectLifeYears = 1;
        if( geo_fin_model <= 6 ) {
            system_use_lifetime_output = true;
            analysis_period_years = as_integer("analysis_period");
        }
        else {
            assign("analysis_period", (ssc_number_t)analysis_period_years);
        }
        assign("system_use_lifetime_output", (ssc_number_t)system_use_lifetime_output);

        geo_inputs.mi_ProjectLifeYears = analysis_period_years; // as_integer("geothermal_analysis_period");
        assign("geothermal_analysis_period", (ssc_number_t)analysis_period_years);

		if ( geo_inputs.mi_ProjectLifeYears == 0)
			throw general_error("invalid analysis period specified in the geothermal hourly model");

        // running the model, we need to specify other inputs
        geo_inputs.md_PotentialResourceMW = as_double("resource_potential");

        geo_inputs.md_UseWeatherFileConditions = as_integer("use_weather_file_conditions");

        // we need to create the SPowerBlockInputs & SPowerBlockParameters and set the inputs

        

        // hybrid dispatch schedule, which will set the value for pbInputs.TOU
        const char* sched = as_string("hybrid_dispatch_schedule");
        int tou[8760];
        int start_day = as_number("start_day_of_year");
        if( !util::translate_schedule(tou, sched, sched, 0, 8, start_day) )
            throw general_error("could not translate schedule for time-of-use rate");

        // weather file and time-of-use data
        geo_inputs.mc_WeatherFileName = as_string("file_name");
        geo_inputs.mia_tou = tou;


        // Set power block parameters (parameters don't change hourly)
        SPowerBlockParameters pbp;

        // power block parameters NOT on the SAM power block input page
        // tech_type is a flag for which coefficient set to use in the power block (1=tower,2=trough,3=Sliding pressure power cycle formulation, 4=geothermal)
        pbp.tech_type = 4; //as_integer("tech_type");
        // the geothermal model is only valid with tech type = 4.  if it's set to any other value, use it as a flag here
        // bool bFlag = (as_integer("tech_type") == 4);
        pbp.T_htf_cold_ref = as_double("T_htf_cold_ref");	// design outlet fluid temp
        pbp.T_htf_hot_ref = resource_temperature;           // [C] design inlet fluid temp
        pbp.HTF = 3;    // as_integer("HTF");						// heat transfer fluid type - set in interface, but no user input

        // power block parameters that ARE on the SAM power block input page
        pbp.P_ref = as_double("nameplate") / 1000; // P_ref wants MW, 'nameplate' in kW
        pbp.P_boil = as_double("P_boil");
        pbp.eta_ref = as_double("eta_ref");
        pbp.q_sby_frac = as_double("q_sby_frac");
        pbp.startup_frac = as_double("startup_frac");
        pbp.startup_time = as_double("startup_time");
        pbp.pb_bd_frac = as_double("pb_bd_frac");
        pbp.T_amb_des = as_double("T_amb_des");
        pbp.CT = as_integer("CT");
        pbp.dT_cw_ref = as_double("dT_cw_ref");
        pbp.T_approach = as_double("T_approach");
        pbp.T_ITD_des = as_double("T_ITD_des");
        pbp.P_cond_ratio = as_double("P_cond_ratio");
        pbp.P_cond_min = as_double("P_cond_min");
        pbp.n_pl_inc = as_integer("hr_pl_nlev");
        pbp.F_wc[0] = as_double("hc_ctl1");
        pbp.F_wc[1] = as_double("hc_ctl2");
        pbp.F_wc[2] = as_double("hc_ctl3");
        pbp.F_wc[3] = as_double("hc_ctl4");
        pbp.F_wc[4] = as_double("hc_ctl5");
        pbp.F_wc[5] = as_double("hc_ctl6");
        pbp.F_wc[6] = as_double("hc_ctl7");
        pbp.F_wc[7] = as_double("hc_ctl8");
        pbp.F_wc[8] = as_double("hc_ctl9");

    
		// Create output object
		SGeothermal_Outputs geo_outputs;

		// --------------------------------------------------------------------------------------------------------------------------
		std::string err_msg;

        // --------------------------------------------------------------------------
        // Design point calcs


		// in all cases, save the UI values in the outputs
		// needed to have the gross power plant size for capacity factor
		// calculation
		if (FillOutputsForUI(err_msg, geo_inputs, geo_outputs) != 0)
			throw general_error("input error: " + err_msg + ".");


        // Assign total GF Flow Rate
        double GF_flowrate = geo_outputs.GF_flowrate;       //[lb/hr]
        assign("GF_flowrate", (ssc_number_t)GF_flowrate);

        // Update SAM powerblock inputs with brine flow rate
        // Set the power block input values that won't change hourly in geothermal model
        SPowerBlockInputs pbInputs;
        pbInputs.mode = 2;
        pbInputs.m_dot_htf = GF_flowrate * 3600.0 / 2.20462;        //[kg/s] converted from [lb/hr]
        pbInputs.demand_var = pbInputs.m_dot_htf;
        pbInputs.standby_control = 1;
        pbInputs.rel_humidity = 0.7;

        assign("num_wells_getem", var_data((ssc_number_t)geo_outputs.md_NumberOfWells));
        assign("num_wells_getem_output", var_data((ssc_number_t)geo_outputs.md_NumberOfWells));
        assign("num_wells_getem_prod_drilled", var_data((ssc_number_t)geo_outputs.md_NumberOfWellsProdDrilled));
        assign("num_wells_getem_prod_failed", var_data((ssc_number_t)geo_outputs.md_NumberOfWellsProdFailed));
        assign("num_wells_getem_inj", var_data((ssc_number_t)geo_outputs.md_NumberOfWellsInj));
        assign("num_wells_getem_inj_drilled", var_data((ssc_number_t)geo_outputs.md_NumberOfWellsInjDrilled));
        assign("num_confirm_wells_to_production", var_data((ssc_number_t)num_conf_wells_prod));

        assign("gross_output", var_data((ssc_number_t)geo_outputs.md_GrossPlantOutputMW));
        assign("gross_cost_output", var_data((ssc_number_t)geo_outputs.md_GrossPowerkW));

        assign("system_capacity", var_data((ssc_number_t)geo_outputs.md_GrossPlantOutputMW*1.E3));
        assign("cp_system_nameplate", var_data((ssc_number_t)geo_outputs.md_GrossPlantOutputMW));

        // Hardcode battery nameplate to 0
        assign("cp_battery_nameplate", 0.0);


        // this assignment happens in UI calculations and model run
        assign("pump_work", var_data((ssc_number_t)geo_outputs.md_PumpWorkKW / 1000)); // kW must be converted to MW
        assign("net_plant_output", ssc_number_t(geo_outputs.md_GrossPlantOutputMW - (geo_outputs.md_PumpWorkKW / 1000.0))); //[MWe]

        // W to hp = divide by 745.7; kg to lb = multiply by 2.20462;
        // hr to s = divide by 3600, so kg/s to lb/hr is multiply by 2.20462 and divide by (1/3600) which is multiply by 3600, so total conversion factor is 2.20462 * 3600 = 7936.64
        double pump_size_hp = geo_outputs.md_pumpwork_prod / 745.7 * geo_inputs.md_ProductionFlowRateKgPerS * 7936.64; // convert W to hp and kg/s to lb/hr
        assign("pump_size_hp", (ssc_number_t)pump_size_hp);

        

        //Assigning 2nd Law efficiency:
            //double eff_secondlaw = var_data(static_cast<ssc_number_t>(geo_outputs.eff_secondlaw));
        double eff_secondlaw = geo_outputs.eff_secondlaw;
        assign("eff_secondlaw", (ssc_number_t)eff_secondlaw);

        assign("pump_depth_ft", var_data((ssc_number_t)geo_outputs.md_PumpDepthFt));
        assign("inj_pump_hp", var_data((ssc_number_t)geo_outputs.md_InjPump_hp));

        assign("plant_brine_eff", var_data((ssc_number_t)geo_outputs.md_PlantBrineEffectiveness));
        assign("pump_watthr_per_lb", var_data((ssc_number_t)geo_outputs.md_PumpWorkWattHrPerLb));
        assign("pumpwork_prod", var_data((ssc_number_t)geo_outputs.md_pumpwork_prod));      //[W-hr/lb]
        assign("pumpwork_inj", var_data((ssc_number_t)geo_outputs.md_pumpwork_inj));        //[W-hr/lb]
        assign("gross_output", var_data((ssc_number_t)geo_outputs.md_GrossPlantOutputMW));

        assign("pump_hp", var_data((ssc_number_t)geo_outputs.md_PumpHorsePower));
        assign("reservoir_pressure", var_data((ssc_number_t)geo_outputs.md_PressureChangeAcrossReservoir));
        assign("reservoir_avg_temp", var_data((ssc_number_t)physics::FarenheitToCelcius(geo_outputs.md_AverageReservoirTemperatureF)));
        assign("bottom_hole_pressure", var_data((ssc_number_t)geo_outputs.md_BottomHolePressure));

        assign("capacity_factor", var_data((ssc_number_t)(0)));
        assign("kwh_per_kw", var_data((ssc_number_t)0));
        //assign("reservoir_avg_temp", var_data(ssc_number_t)physics::FarenheitToCelsius(geo_outputs.md_AverageReservoirTemperatureF));
        //double nameplate = geo_outputs.md_GrossPlantOutputMW * 1000; // in kW

        //Assign HP & LP Flash Pressures: 
        double hp_flash_pressure = geo_outputs.md_PressureHPFlashPSI;
        assign("hp_flash_pressure", (ssc_number_t)hp_flash_pressure);
        double lp_flash_pressure = geo_outputs.md_PressureLPFlashPSI;
        assign("lp_flash_pressure", (ssc_number_t)lp_flash_pressure);

        //Assign all 3 stages of vacuum pump powers:
        double v_stage_1 = geo_outputs.v_stage_1;
        assign("v_stage_1", (ssc_number_t)v_stage_1);
        double v_stage_2 = geo_outputs.v_stage_2;
        assign("v_stage_2", (ssc_number_t)v_stage_2);
        double v_stage_3 = geo_outputs.v_stage_3;
        assign("v_stage_3", (ssc_number_t)v_stage_3);

        

        //Assigning Rejected Total Heat from Flash Plant:
        double qRejectTotal = geo_outputs.qRejectedTotal;	//total heat rejected 
        assign("qRejectTotal", (ssc_number_t)qRejectTotal);

        //Assign qCondenser (Flash Plant Type):
        double qCondenser = geo_outputs.condenser_q;
        assign("qCondenser", (ssc_number_t)qCondenser);

        //Assign NCG Condenser Heat Rejecting Stages:
        double qRejectByStage_1 = geo_outputs.qRejectByStage_1;
        assign("qRejectByStage_1", (ssc_number_t)qRejectByStage_1);
        double qRejectByStage_2 = geo_outputs.qRejectByStage_2;
        assign("qRejectByStage_2", (ssc_number_t)qRejectByStage_2);
        double qRejectByStage_3 = geo_outputs.qRejectByStage_3;
        assign("qRejectByStage_3", (ssc_number_t)qRejectByStage_3);

        //Assign NCG Condensate Pump Work & CW Pump Work Value for Calculating NCG Pump Cost: 
        double ncg_condensate_pump = geo_outputs.ncg_condensate_pump;
        assign("ncg_condensate_pump", (ssc_number_t)ncg_condensate_pump);
        double cw_pump_work = geo_outputs.cw_pump_work;
        assign("cw_pump_work", (ssc_number_t)cw_pump_work);

        //Assign steam suction ratio value for NCG Ejector Cost Calculation
        double pressure_ratio_1 = geo_outputs.pressure_ratio_1;
        assign("pressure_ratio_1", (ssc_number_t)pressure_ratio_1);
        double pressure_ratio_2 = geo_outputs.pressure_ratio_2;
        assign("pressure_ratio_2", (ssc_number_t)pressure_ratio_2);
        double pressure_ratio_3 = geo_outputs.pressure_ratio_3;
        assign("pressure_ratio_3", (ssc_number_t)pressure_ratio_3);

        //Assigning Value of Condensate Pump for Pump Cost Calculation:
        double condensate_pump_power = geo_outputs.condensate_pump_power;
        assign("condensate_pump_power", (ssc_number_t)condensate_pump_power);

        //Assign CW Flow and Head for Pump Cost Caclulation:
        double cwflow = geo_outputs.cwflow;
        assign("cwflow", (ssc_number_t)cwflow);
        double cw_pump_head = geo_outputs.cw_pump_head;
        assign("cw_pump_head", (ssc_number_t)cw_pump_head);

        //Assign Specific Volume and Mass Fraction (x) for Flash Vessel Calculations:
        double spec_vol = geo_outputs.spec_vol;	//HP Specific Volume
        assign("spec_vol", (ssc_number_t)spec_vol);
        double x_hp = geo_outputs.getX_hp;
        assign("x_hp", (ssc_number_t)x_hp);
        double spec_vol_lp = geo_outputs.spec_vol_lp;	//LP Specific Volume
        assign("spec_vol_lp", (ssc_number_t)spec_vol_lp);
        double x_lp = geo_outputs.getX_lp;
        assign("x_lp", (ssc_number_t)x_lp);

        //Assign Flash Count: 
        double flash_count = geo_outputs.flash_count;
        assign("flash_count", (ssc_number_t)flash_count);



        //***********************************************************
        // Call cost models
        if( geo_fin_model < 8 ) {
            // Update geothermal costs by running the geothermal costs cmod
            std::string geo_cost_module_name = "geothermal_costs";
            ssc_module_t geo_cost_module = ssc_module_create(geo_cost_module_name.c_str());
            var_table* geo_cost_vtab = this->get_var_table();

            ssc_data_t geo_cmod_inputs = static_cast<ssc_data_t>(geo_cost_vtab);

            if( !ssc_module_exec(geo_cost_module, geo_cmod_inputs) ) {
                std::string str = geo_cost_module_name + " execution error. ";
                int idx = 0;
                int type = -1;
                while( const char* msg = ssc_module_log(geo_cost_module, idx++, &type, nullptr) )
                {
                    if(/*/(type == SSC_NOTICE) || */(type == SSC_WARNING) || (type == SSC_ERROR) ) {
                        str += std::string(msg);
                        str += "\t";
                    }
                }
                ssc_module_free(geo_cost_module);
                throw std::runtime_error(str);
            }

            // *************************************************************
            // *************************************************************

            if( geo_fin_model < 7 ) {
            std:string construction_financing_module_name = "cb_construction_financing";
                ssc_module_t construction_financing_module = ssc_module_create(construction_financing_module_name.c_str());

                if( !ssc_module_exec(construction_financing_module, geo_cmod_inputs) ) {
                    std::string str = construction_financing_module_name + " execution error. ";
                    int idx = 0;
                    int type = -1;
                    while( const char* msg = ssc_module_log(construction_financing_module, idx++, &type, nullptr) )
                    {
                        if(/*/(type == SSC_NOTICE) || */(type == SSC_WARNING) || (type == SSC_ERROR) ) {
                            str += std::string(msg);
                            str += "\t";
                        }
                    }
                    ssc_module_free(construction_financing_module);
                    throw std::runtime_error(str);
                }

                // *****************************************************
                assign("drilling_cost", as_double("total_drilling_cost_used"));
                assign("field_gathering_system_cost", as_double("total_surface_equipment_cost"));
                assign("water_loss", as_double("subsurface_water_loss"));
                assign("pump_depth", as_double("pump_depth_ft"));
                assign("pump_type", 0);

                geo_cmod_inputs = static_cast<ssc_data_t>(geo_cost_vtab);
                getem_om_cost_calc(geo_cmod_inputs);
            }
        }        
        
        // ***************************************************************

        // Hardcoded outputs
        // Always use recapitalization for geothermal
        assign("system_use_recapitalization", 1);

        // Hardcode degradation to 0
        ssc_number_t* p_deg = allocate("degradation", 1);
        p_deg[0] = 0.0;
        //assign("degradation", 0);

		if (sim_type == 2) {
			
            return;			
			
		}
		else {

            // since we're going to run the model, we have to allocate the arrays

        // allocate lifetime annual arrays (one element per year, over lifetime of project)
//			geo_outputs.maf_ReplacementsByYear = allocate("annual_replacements", geo_inputs.mi_ProjectLifeYears);
            geo_outputs.maf_ReplacementsByYear = allocate("system_lifetime_recapitalize", geo_inputs.mi_ProjectLifeYears);
            //ssc_number_t *annual_replacements = allocate( "annual_replacements", geo_inputs.mi_ProjectLifeYears);

            // allocate lifetime monthly arrays (one element per month, over lifetime of project)
            geo_outputs.maf_monthly_resource_temp = allocate("monthly_resource_temperature", 12 * geo_inputs.mi_ProjectLifeYears);
            geo_outputs.maf_monthly_power = allocate("monthly_power", 12 * geo_inputs.mi_ProjectLifeYears);
            geo_outputs.maf_monthly_energy = allocate("monthly_energy_lifetime", 12 * geo_inputs.mi_ProjectLifeYears);

            // allocate lifetime timestep arrays (one element per timestep, over lifetime of project)
            // if this is a monthly analysis, these are redundant with monthly arrays that track same outputs
            //geo_inputs.mi_MakeupCalculationsPerYear = (geo_inputs.mi_ModelChoice == 2) ? 8760 : 12;
            if(geo_inputs.mi_simulation_timestep_type == 1) {
                geo_inputs.mi_performance_simulations_per_year = 8760;
            }
            else {
                geo_inputs.mi_performance_simulations_per_year = 12;
            }

            geo_inputs.mi_TotalMakeupCalculations = geo_inputs.mi_ProjectLifeYears * geo_inputs.mi_performance_simulations_per_year;

            geo_outputs.maf_timestep_resource_temp = allocate("timestep_resource_temperature", geo_inputs.mi_TotalMakeupCalculations);
            geo_outputs.maf_timestep_power = allocate("timestep_power", geo_inputs.mi_TotalMakeupCalculations);
            geo_outputs.maf_timestep_test_values = allocate("timestep_test_values", geo_inputs.mi_TotalMakeupCalculations);

            geo_outputs.maf_timestep_pressure = allocate("timestep_pressure", geo_inputs.mi_TotalMakeupCalculations);
            geo_outputs.maf_timestep_dry_bulb = allocate("timestep_dry_bulb", geo_inputs.mi_TotalMakeupCalculations);
            geo_outputs.maf_timestep_wet_bulb = allocate("timestep_wet_bulb", geo_inputs.mi_TotalMakeupCalculations);

            size_t n_rec = 8760;
            if( as_boolean("system_use_lifetime_output") ) {
                n_rec *= geo_inputs.mi_ProjectLifeYears;
            }


			geo_outputs.maf_hourly_power = allocate("tmp", n_rec);
			ssc_number_t * p_gen = allocate("gen", n_rec);

			// TODO - implement performance factors 
			adjustment_factors haf(this->get_var_table(), "adjust");
			if (!haf.setup(8760, geo_inputs.mi_ProjectLifeYears))
				throw exec_error("geothermal", "failed to setup adjustment factors: " + haf.error());
            std::vector<double> haf_input;
            haf_input.resize(8760 * geo_inputs.mi_ProjectLifeYears);
            for (int a = 0; a < geo_inputs.mi_ProjectLifeYears; a++) {
                for (int i = 0; i < 8760; i++)
                    haf_input[int(a * 8760 + i)] = haf(a * 8760 + i);
            }
            geo_inputs.haf = haf_input;

			// running
			if (RunGeothermalAnalysis(my_update_function, this, err_msg, pbp, pbInputs, geo_inputs, geo_outputs) != 0)
				throw exec_error("geothermal", "error from geothermal hourly model: " + err_msg + ".");


			// Summary calculations
			ssc_number_t total_energy = 0;
			for (size_t i = 0; i < 12 * geo_inputs.mi_ProjectLifeYears; ++i) {
				total_energy += geo_outputs.maf_monthly_energy[i];
			}
			assign("lifetime_output", var_data(total_energy));

			total_energy = 0;
			for (size_t i = 0; i < 12; ++i) {
				total_energy += geo_outputs.maf_monthly_energy[i];
			}
			assign("first_year_output", var_data(total_energy));	

			// metric outputs moved to technology
			double capacity_fac;	//geothermal plant capacity factor
			double kWhperkW = 0.0;
			double nameplate = geo_inputs.md_DesiredSalesCapacityKW; // Was md_GrossPlantOutputMW*1000 -> now it is md_DesiredSalesCapacityKW
			double annual_energy = 0.0;


			//Loop calculates total energy generation over entire project lifetime (in kWh) 
			// Why?  Is the annual energy changing from year to year?
			for (size_t i = 0; i <n_rec ; i++)	{
				annual_energy += geo_outputs.maf_hourly_power[i];
                p_gen[i] = geo_outputs.maf_hourly_power[i];
			}


            gen_heatmap(this, 1);

			if (nameplate > 0) kWhperkW = annual_energy / nameplate;
			capacity_fac = total_energy / nameplate;
			if (geo_inputs.mi_ProjectLifeYears > 0) kWhperkW = kWhperkW / geo_inputs.mi_ProjectLifeYears;

			assign("gross_output", var_data((ssc_number_t)geo_outputs.md_GrossPlantOutputMW));
            assign("gross_cost_output", var_data((ssc_number_t)geo_outputs.md_GrossPowerkW));
			assign("capacity_factor", var_data((ssc_number_t)(capacity_fac / 87.6)));		//Divided by 8760 and then multiplied by 100 (or divide by 87.6) to return CF as a %
			assign("kwh_per_kw", var_data((ssc_number_t)kWhperkW));
			// 5/28/15 average provided for FCR market
			assign("annual_energy", var_data((ssc_number_t)(annual_energy / geo_inputs.mi_ProjectLifeYears)));
            accumulate_monthly_for_year("gen", "monthly_energy", 8760.0 / n_rec, n_rec / 8760);
		}

		
	}

};

DEFINE_MODULE_ENTRY( geothermal, "Geothermal monthly and hourly models using general power block code from TRNSYS Type 224 code by M.Wagner, and some GETEM model code.", 3 );

// above is equivalent to:
//static compute_module *_create_geothermal () { return new cm_geothermal; }
//module_entry_info cm_entry_geothermal = { "geothermal", "Geothermal monthly...", 3, _create_geothermal }; 
