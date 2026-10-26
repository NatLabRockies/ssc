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
        int sim_type = as_integer("sim_type");  // "1 (default): timeseries, 2: design only",

        // --------------------------------------------------------------
        // Get main system paramters
        double resource_temperature = as_double("resource_temp");    //[C]
        //resource characterization

        // Set design temp equal to resource temperature
        double design_temp = resource_temperature;    //[C]
        assign("design_temp", ssc_number_t(design_temp));

        int analysis_type = as_integer("analysis_type");
        calculationBasis calc_basis_type;
        if( analysis_type == 0 )
            calc_basis_type = POWER_SALES;
        else
            calc_basis_type = NUMBER_OF_WELLS;

        // Convert conversion type to enum here, then use enum comparisons in downstream logic
        int conversion_type_in = as_integer("conversion_type");
        conversionTypes e_conversion_type = conversionTypes::NO_CONVERSION_TYPE;
        if( conversion_type_in == 0 )
            e_conversion_type = conversionTypes::BINARY;
        else if( conversion_type_in == 1 )
            e_conversion_type = conversionTypes::FLASH;
        // ----------------------------------------------------------------------------------

        // Convert resource type to enum here, then use enum comparisons in downstream logic
        int resource_type_in = as_integer("resource_type");
        resourceTypes e_resource_type = resourceTypes::NO_RESOURCE_TYPE;
        if( resource_type_in == 0 )
            e_resource_type = HYDROTHERMAL;
        else if( resource_type_in == 1 )
            e_resource_type = EGS;
        // --------------------------------------------------------------

        int geo_fin_model = as_integer("geo_financial_model");

        int decline_type_in = as_integer("decline_type");
        tempDeclineMethod e_decline_type = tempDeclineMethod::NO_TEMPERATURE_DECLINE_METHOD;
        //int decline_type = decline_type_in;
        // Apply decline type logic and reset if necessary
        // This logic was on the Plant and Equipment page on 5/24/26
        if( e_resource_type == resourceTypes::HYDROTHERMAL ){
            e_decline_type = ENTER_RATE;
        }
        else if( e_conversion_type == BINARY ) {
            e_decline_type = static_cast<tempDeclineMethod>(decline_type_in + 1);
        }
        else{
            e_decline_type = ENTER_RATE;
        }
        // --------------------------------------------------------------

        reservoirPressureChangeCalculation e_reservoir_pressure_change_type = static_cast<reservoirPressureChangeCalculation>(as_integer("reservoir_pressure_change_type")+1);
        // Logic from the Plant and Equipment page on 5/24/26
        if( e_resource_type != resourceTypes::EGS &&
            (e_reservoir_pressure_change_type == SIMPLE_FRACTURE || e_reservoir_pressure_change_type == USER_TEMP) ) {
            e_reservoir_pressure_change_type = ENTER_PC;
        }

        geo_wells_stimulated geo_wells_stim_type = static_cast<geo_wells_stimulated>(as_integer("stimulation_type"));

        depthCalculationForEGS depth_calc_EGS_type = TEMPERATURE;

        // --------------------------------------------------------------
        // --------------------------------------------------------------

        // Calculate design point production well
        double dt_prod_well = as_double("dt_prod_well");            //[C]
        double dtProdWellChoice = as_double("prod_well_choice");    //[-]
        bool is_prod_well_dt_calc = bool(dtProdWellChoice);

        double TemperatureDeclineRate = as_double("temp_decline_rate") / 100;   //[-/yr]
        double MaxTempDeclineC = as_double("temp_decline_max");                 //[C]
        bool AllowReservoirReplacements = as_boolean("allow_reservoir_replacements");   //[-]

        C_geo_prod_well* p_geo_prod_well = new C_geo_prod_well (resource_temperature, dt_prod_well, is_prod_well_dt_calc,
                                        TemperatureDeclineRate, MaxTempDeclineC, AllowReservoirReplacements);

        double conc_SiO2_design_calc_ppm = std::numeric_limits<double>::quiet_NaN();
        double T_plant_design_calc_C = std::numeric_limits<double>::quiet_NaN();
        double dt_prod_well_des_calc_C = std::numeric_limits<double>::quiet_NaN();

        p_geo_prod_well->get_design_calcs(conc_SiO2_design_calc_ppm, T_plant_design_calc_C, dt_prod_well_des_calc_C);

        // Calculate design point surface plant
        double TemperatureWetBulbC = as_double("wet_bulb_temp");            //[C]
        double PlantEfficiency = as_double("plant_efficiency_input") / 100; //[-]      

        double num_conf_wells = as_double("geotherm.cost.conf_num_wells");
        double perc_conf_wells_production = as_double("geotherm.cost.confirm_wells_percent");
        double num_conf_wells_prod = num_conf_wells * perc_conf_wells_production / 100.0;
        double ExplorationWellsProd = num_conf_wells_prod;

        double num_wells_in = as_double("num_wells");
        double ratio_injection_to_production = as_double("geotherm.cost.inj_prod_well_ratio");
        double ProductionFlowRateKgPerS = as_double("well_flow_rate");

        int InjectionWellDiam = as_integer("geotherm.cost.inj_cost_curve_welldiam");
        int InjectionWellType = as_integer("geotherm.cost.inj_cost_curve_welltype");
        int ProductionWellDiam = as_integer("geotherm.cost.prod_cost_curve_welldiam");
        int ProductionWellType = as_integer("geotherm.cost.prod_cost_curve_welltype");

        double resource_depth_in = as_double("resource_depth");     //[m]
        double InjectivityIndex = as_double("injectivity_index");
        double ExcessPressureBar = physics::PsiToBar(as_double("excess_pressure_pump"));    //[bar]
        double PressureChangeAcrossSurfaceEquipment = as_double("delta_pressure_equip");
        double AdditionalPressure = 1.0;        // ???

        double water_loss_frac = as_double("subsurface_water_loss") / 100;
        double reservoir_pressure_chage = as_double("reservoir_pressure_change");

        double gf_pump_efficiency = as_double("pump_efficiency") / 100;     //[-]

        double desired_sales_capacity_kw = as_double("nameplate");    //[kWe]

        double stim_success_rate = as_double("stim_success_rate") / 100.0;  //[-]
        double drill_success_rate = as_double("drilling_success_rate") / 100.0; //[-]

        double PressureAmbientPSI = as_double("ambient_pressure");          //[psi]
        double FailedProdFlowRatio = as_double("failed_prod_flow_ratio");   //[-]

        double PotentialResourceMW = as_double("resource_potential");       //[MWe]

        int use_weather_file_conditions = as_integer("use_weather_file_conditions");    //[-]

        flashTypes e_flash_type = static_cast<flashTypes>(as_integer("conversion_subtype") + 1);

        // Other settings that I suppose can be design-point
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

        C_geo_surface* p_geo_surface = nullptr;
        C_geo_subsurface* p_geo_subsurface = nullptr;
        C_geo_weather_file* p_geo_weather_file = nullptr;
        if( e_conversion_type == conversionTypes::BINARY && e_resource_type == HYDROTHERMAL && false ) {
        //if( e_conversion_type == conversionTypes::BINARY  ) {
            p_geo_surface = new C_geo_surface(T_plant_design_calc_C, TemperatureWetBulbC, PlantEfficiency, conc_SiO2_design_calc_ppm,
                e_resource_type, e_conversion_type, e_decline_type, e_flash_type);

            double AE_full_brine_dt_design_calc = std::numeric_limits<double>::quiet_NaN();
            double AE_brine_T_sol_to_to_amb_design_calc = std::numeric_limits<double>::quiet_NaN();
            double AE_brine_T_plant_to_T_sol_to_amb_design_calc = std::numeric_limits<double>::quiet_NaN();
            double eta_2nd_law_max_GETEM_and_T_sol_derate = std::numeric_limits<double>::quiet_NaN();
            double AE_2nd_law_max_GETEM_and_T_sol_derate = std::numeric_limits<double>::quiet_NaN();
            double total_brine_effectiveness = std::numeric_limits<double>::quiet_NaN();
            double eta_2nd_law_total_cycle_design = std::numeric_limits<double>::quiet_NaN();
            double T_amb_des_K = std::numeric_limits<double>::quiet_NaN();
            double T_plant_in_des_K = std::numeric_limits<double>::quiet_NaN();
            double carnot_eff_des = std::numeric_limits<double>::quiet_NaN();
            double T_inj_des_C = std::numeric_limits<double>::quiet_NaN();

            p_geo_surface->get_specific_design_calcs(
                AE_full_brine_dt_design_calc,
                AE_brine_T_sol_to_to_amb_design_calc,
                AE_brine_T_plant_to_T_sol_to_amb_design_calc,
                eta_2nd_law_max_GETEM_and_T_sol_derate,
                AE_2nd_law_max_GETEM_and_T_sol_derate,
                total_brine_effectiveness,
                eta_2nd_law_total_cycle_design,
                T_amb_des_K,
                T_plant_in_des_K,
                carnot_eff_des,
                T_inj_des_C);

            p_geo_subsurface = new C_geo_subsurface(geo_wells_stim_type, ExplorationWellsProd, e_resource_type,
                calc_basis_type, num_wells_in, ratio_injection_to_production,
                ProductionFlowRateKgPerS, total_brine_effectiveness,
                InjectionWellDiam, InjectionWellType, resource_depth_in,
                resource_temperature, T_plant_design_calc_C, dt_prod_well_des_calc_C, T_inj_des_C,
                InjectivityIndex, e_conversion_type,
                ExcessPressureBar, PressureChangeAcrossSurfaceEquipment, AdditionalPressure,
                TemperatureWetBulbC, depth_calc_EGS_type,
                water_loss_frac, e_reservoir_pressure_change_type,
                reservoir_pressure_chage, e_flash_type,
                gf_pump_efficiency, PotentialResourceMW,
                ProductionWellDiam, ProductionWellType,
                desired_sales_capacity_kw,
                stim_success_rate, drill_success_rate, PressureAmbientPSI, FailedProdFlowRatio);

            double resource_depth_calc = std::numeric_limits<double>::quiet_NaN();                   //[m]
            double temp_gradient_fixed_T_amb_des_calc = std::numeric_limits<double>::quiet_NaN();                
            double injection_density_des_calc = std::numeric_limits<double>::quiet_NaN();                   
            double pumpwork_watthr_per_lb_design = std::numeric_limits<double>::quiet_NaN(); //[W-hr/lb_m]
            double total_prod_flow_rate = std::numeric_limits<double>::quiet_NaN();          //[lb_m/hr]
            double prod_wells_exploration = std::numeric_limits<double>::quiet_NaN();
            double inj_wells_exploration = std::numeric_limits<double>::quiet_NaN();
            double num_wells_calc = std::numeric_limits<double>::quiet_NaN();
            double NumberOfWellsProdExp = std::numeric_limits<double>::quiet_NaN();
            double NumberOfWellsProdDrilled = std::numeric_limits<double>::quiet_NaN();
            double NumberOfWellsProdFailed = std::numeric_limits<double>::quiet_NaN();
            double NumberOfWellsInj = std::numeric_limits<double>::quiet_NaN();
            double NumberOfWellsInjDrilled = std::numeric_limits<double>::quiet_NaN();
            int NumberOfReservoirs = -1;
            double inj_pump_hp = std::numeric_limits<double>::quiet_NaN();       // ?
            double prod_pump_power_kWe = std::numeric_limits<double>::quiet_NaN();
            double inj_pump_power_kWe = std::numeric_limits<double>::quiet_NaN();
            double total_pump_power_kWe = std::numeric_limits<double>::quiet_NaN();
            double prod_pump_work_ft_design = std::numeric_limits<double>::quiet_NaN();
            double pumpwork_prod = std::numeric_limits<double>::quiet_NaN();
            double pumpwork_inj  = std::numeric_limits<double>::quiet_NaN();
            double PressureChangeAcrossReservoir = std::numeric_limits<double>::quiet_NaN();
            double AverageReservoirTemperatureF = std::numeric_limits<double>::quiet_NaN();
            std::string ErrorString;

            p_geo_subsurface->get_design_calcs(
                resource_depth_calc,
                temp_gradient_fixed_T_amb_des_calc,
                injection_density_des_calc,
                pumpwork_watthr_per_lb_design,
                total_prod_flow_rate,                   //[lb_m/hr]
                prod_wells_exploration,
                inj_wells_exploration,
                num_wells_calc,
                NumberOfWellsProdExp,
                NumberOfWellsProdDrilled,
                NumberOfWellsProdFailed,
                NumberOfWellsInj,
                NumberOfWellsInjDrilled,
                NumberOfReservoirs,
                inj_pump_hp,
                prod_pump_work_ft_design,
                prod_pump_power_kWe,
                inj_pump_power_kWe,
                total_pump_power_kWe,
                pumpwork_prod,
                pumpwork_inj,
                PressureChangeAcrossReservoir,
                AverageReservoirTemperatureF,
                ErrorString);

            double cycle_net_power_design_kWe = std::numeric_limits<double>::quiet_NaN();
            p_geo_surface->design_for_absolute_power(total_prod_flow_rate, cycle_net_power_design_kWe);

            double net_plant_power_kWe = cycle_net_power_design_kWe - total_pump_power_kWe;

            // Set design-point outputs
            assign("GF_flowrate", (ssc_number_t) total_prod_flow_rate);     //[lb_m/hr]

            assign("num_wells_getem", var_data((ssc_number_t)num_wells_calc));
            assign("num_wells_getem_output", var_data((ssc_number_t)num_wells_calc));
            assign("num_wells_getem_prod_drilled", var_data((ssc_number_t)NumberOfWellsProdDrilled));   // geo_outputs.md_NumberOfWellsProdDrilled));
            assign("num_wells_getem_prod_failed", var_data((ssc_number_t)NumberOfWellsProdFailed));     // geo_outputs.md_NumberOfWellsProdFailed));
            assign("num_wells_getem_inj", var_data((ssc_number_t)NumberOfWellsInj));                    // geo_outputs.md_NumberOfWellsInj));
            assign("num_wells_getem_inj_drilled", var_data((ssc_number_t)NumberOfWellsInjDrilled));     // geo_outputs.md_NumberOfWellsInjDrilled));
            assign("num_confirm_wells_to_production", var_data((ssc_number_t)num_conf_wells_prod));

            // The GETEM flash model as of 2026.06.26 uses a different value for gross turbine output
            double gross_cost_output_for_flash = std::numeric_limits<double>::quiet_NaN();
            if( e_conversion_type == conversionTypes::FLASH ) {
                gross_cost_output_for_flash = cycle_net_power_design_kWe;       //[kWe]
            }
            assign("gross_cost_output", var_data((ssc_number_t)gross_cost_output_for_flash));

            assign("system_capacity", var_data((ssc_number_t)cycle_net_power_design_kWe));             // geo_outputs.md_GrossPlantOutputMW * 1.E3));
            assign("cp_system_nameplate", var_data((ssc_number_t)cycle_net_power_design_kWe *1.E-3));   // geo_outputs.md_GrossPlantOutputMW));

            // Hardcode battery nameplate to 0
            assign("cp_battery_nameplate", 0.0);

            // this assignment happens in UI calculations and model run
            assign("pump_work", var_data((ssc_number_t)total_pump_power_kWe*1.E-3));              // geo_outputs.md_PumpWorkKW / 1000)); // kW must be converted to MW
            assign("net_plant_output", ssc_number_t(net_plant_power_kWe*1.E-3));            // geo_outputs.md_GrossPlantOutputMW - (geo_outputs.md_PumpWorkKW / 1000.0))); //[MWe]

            double total_prod_pump_power_kWe = prod_pump_power_kWe*1.341022;             //[hp] total production pumping pump
            double single_pump_prod_pump_power_hp = prod_pump_power_kWe * 1.341022 / num_wells_calc;    //[hp] single production pumping pump
            assign("pump_size_hp", (ssc_number_t)single_pump_prod_pump_power_hp);       //[hp] single production pumping pump

            //Assigning 2nd Law efficiency:
            assign("eff_secondlaw", (ssc_number_t)eta_2nd_law_total_cycle_design);       //eff_secondlaw);       // plant-level (i.e. not considering pump work) 2nd law efficiency

            assign("pump_depth_ft", var_data((ssc_number_t)prod_pump_work_ft_design));      //[ft]    // GetProductionPumpWorkft(void)
            assign("inj_pump_hp", var_data((ssc_number_t)inj_pump_power_kWe * 1.341022));   //[hp]                                       //geo_outputs.md_InjPump_hp));
            
            assign("plant_brine_eff", var_data((ssc_number_t)total_brine_effectiveness));     //[W-hr/lb_m] geo_outputs.md_PlantBrineEffectiveness));
            assign("pump_watthr_per_lb", var_data((ssc_number_t)pumpwork_watthr_per_lb_design));//[W-hr/lb_m] //geo_outputs.md_PumpWorkWattHrPerLb));
            assign("pumpwork_prod", var_data((ssc_number_t)pumpwork_prod));                     //[W-hr/lb_m]  //geo_outputs.md_pumpwork_prod));      //[W-hr/lb]
            assign("pumpwork_inj", var_data((ssc_number_t)pumpwork_inj));                       //[W-hr/lb_m] geo_outputs.md_pumpwork_inj));        //[W-hr/lb]
            assign("gross_output", var_data((ssc_number_t)cycle_net_power_design_kWe *1.E-3));  //[MWe] geo_outputs.md_GrossPlantOutputMW));
            
            assign("reservoir_pressure", var_data((ssc_number_t)PressureChangeAcrossReservoir));    //[psi] geo_outputs.md_PressureChangeAcrossReservoir));
            assign("reservoir_avg_temp", var_data((ssc_number_t)physics::FarenheitToCelcius(AverageReservoirTemperatureF))); //[F] physics::FarenheitToCelcius(geo_outputs.md_AverageReservoirTemperatureF)));

            // This calculation doesn't seem consistent in the code, so let's try not to use it until we update the code
            assign("bottom_hole_pressure", var_data((ssc_number_t) -6.54321));                   // geo_outputs.md_BottomHolePressure));
            
            assign("capacity_factor", var_data((ssc_number_t)(0)));
            assign("kwh_per_kw", var_data((ssc_number_t)0));

            // Flash outputs required
            assign("cwflow", (ssc_number_t) std::numeric_limits<double>::quiet_NaN());

            // Check cost and OM model calcs
            double total_installed_cost_calc = std::numeric_limits<double>::quiet_NaN();
            double total_getem_om_cost = std::numeric_limits<double>::quiet_NaN();

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

                total_installed_cost_calc = as_double("total_installed_cost");

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
                    assign("num_wells_calc", as_double("num_wells_getem"));
                    assign("drilling_cost", as_double("total_drilling_cost_used"));
                    assign("field_gathering_system_cost", as_double("total_surface_equipment_cost"));
                    assign("water_loss", as_double("subsurface_water_loss"));
                    assign("pump_depth", as_double("pump_depth_ft"));
                    assign("pump_type", 0);

                    geo_cmod_inputs = static_cast<ssc_data_t>(geo_cost_vtab);
                    getem_om_cost_calc(geo_cmod_inputs);

                    total_getem_om_cost = as_double("total_getem_om_cost");
                }
            }

            
            assign("geothermal_analysis_period", (ssc_number_t)analysis_period_years);

            if( analysis_period_years == 0 )
                throw general_error("invalid analysis period specified in the geothermal hourly model");

            // weather file and time-of-use data
            const char* WeatherFileName = as_string("file_name");
            p_geo_weather_file = new C_geo_weather_file(WeatherFileName);

            // Hardcoded outputs
            // Always use recapitalization for geothermal
            assign("system_use_recapitalization", 1);

            // Hardcode degradation to 0
            ssc_number_t* p_deg = allocate("degradation", 1);
            p_deg[0] = 0.0;
            //assign("degradation", 0);

            if( sim_type == 2 ) {

                return;

            }
            else {

                geo_simulation_timestep_type simulation_timestep_type = static_cast<geo_simulation_timestep_type>(as_integer("simulation_timestep_type"));    // 0=monthly, 1=hourly


                geo_weather_timestep_type weather_timestep_type = geo_weather_timestep_type::WEATHER_UNDEFINED;
                if(simulation_timestep_type == geo_simulation_timestep_type::MONTHLY_TIMESTEPS && use_weather_file_conditions == 0){
                    weather_timestep_type = geo_weather_timestep_type::WEATHER_DESIGN;
                }
                else if( simulation_timestep_type == geo_simulation_timestep_type::MONTHLY_TIMESTEPS ) {
                    weather_timestep_type = geo_weather_timestep_type::WEATHER_MONTHLY;
                }
                else{
                    weather_timestep_type = geo_weather_timestep_type::WEATHER_HOURLY;
                }

                C_geo_system geo_system(p_geo_prod_well, p_geo_surface,
                    p_geo_subsurface, p_geo_weather_file,
                    simulation_timestep_type,
                    weather_timestep_type,
                    my_update_function, this);
                
                // allocate lifetime annual arrays (one element per year, over lifetime of project)
                geo_system.maf_ReplacementsByYear = allocate("system_lifetime_recapitalize", analysis_period_years);
                
                //// allocate lifetime monthly arrays (one element per month, over lifetime of project)
                //geo_outputs.maf_monthly_resource_temp = allocate("monthly_resource_temperature", 12 * geo_inputs.mi_ProjectLifeYears);
                geo_system.maf_monthly_power = allocate("monthly_power", 12 * analysis_period_years);
                //geo_outputs.maf_monthly_energy = allocate("monthly_energy_lifetime", 12 * geo_inputs.mi_ProjectLifeYears);
                //
                //// allocate lifetime timestep arrays (one element per timestep, over lifetime of project)
                //// if this is a monthly analysis, these are redundant with monthly arrays that track same outputs
                int performance_simulations_per_year = 0;
                if( simulation_timestep_type == HOURLY_TIMESTEPS ) {
                    performance_simulations_per_year = 8760;
                }
                else {
                    performance_simulations_per_year = 12;
                }
                
                int TotalMakeupCalculations = analysis_period_years * performance_simulations_per_year;
                
                //geo_outputs.maf_timestep_test_values = allocate("timestep_test_values", geo_inputs.mi_TotalMakeupCalculations);
                
                //geo_outputs.maf_timestep_pressure = allocate("timestep_pressure", geo_inputs.mi_TotalMakeupCalculations);
                
                size_t n_rec = 8760;
                if( as_boolean("system_use_lifetime_output") ) {
                    n_rec *= analysis_period_years;
                }
                
                //// Still want hourly reporting for some outputs even if simulation timestep is monthly
                geo_system.maf_timestep_resource_temp = allocate("timestep_resource_temperature", n_rec);
                geo_system.maf_timestep_dry_bulb = allocate("timestep_dry_bulb", n_rec);
                geo_system.maf_timestep_wet_bulb = allocate("timestep_wet_bulb", n_rec);
                
                geo_system.maf_AE_total_od = allocate("AE_od", n_rec);
                geo_system.maf_second_law_eta_total_od = allocate("getem_2nd_law_total_od", n_rec);
                geo_system.maf_carnot_od_scaling = allocate("carnot_scaling_od", n_rec);
                geo_system.maf_cycle_net_power_od = allocate("cycle_net_power_od", n_rec);
                geo_system.maf_brine_pumping_power_od = allocate("brine_pumping_power_od", n_rec);
                
                // 26-10-21: Refactor approach is *NOT* applying the availability derate in the hourly calcs, while the old approach did
                //   instead applying availability after timeseries simulation
                //   renaming hourly array to clarify this change approach
                geo_system.maf_hourly_power_no_derate = allocate("plant_net_power_od", n_rec);
                
                // TODO - implement performance factors 
                adjustment_factors haf1(this->get_var_table(), "adjust");
                if( !haf1.setup(8760, analysis_period_years) )
                    throw exec_error("geothermal", "failed to setup adjustment factors: " + haf1.error());
                std::vector<double> haf_input1;
                haf_input1.resize(8760 * analysis_period_years);
                for( int a = 0; a < analysis_period_years; a++ ) {
                    for( int i = 0; i < 8760; i++ )
                        haf_input1[int(a * 8760 + i)] = haf1(a * 8760 + i);
                }

                
                bool is_simulate_success = geo_system.simulate(analysis_period_years, TotalMakeupCalculations, performance_simulations_per_year);

                // Apply availability derate
                ssc_number_t* p_gen = allocate("gen", n_rec);
                for( size_t i = 0; i < n_rec; i++ ) {
                    p_gen[i] = geo_system.maf_hourly_power_no_derate[i]*haf_input1[i];
                }

                ssc_number_t first_year_total_energy = 0;
                for( size_t i = 0; i < 8760; ++i ) {
                    first_year_total_energy += p_gen[i];
                }
                assign("first_year_output", var_data(first_year_total_energy));
                
                double lifetime_energy = 0.0;
                for( size_t i = 0; i < n_rec; i++ ) {
                    lifetime_energy += p_gen[i];
                }
                assign("lifetime_output", var_data(lifetime_energy));

                gen_heatmap(this, 1);

                // The following terms use *first year* total energy
                double kWhperkW = 0.0;
                if( cycle_net_power_design_kWe > 0 ) kWhperkW = first_year_total_energy / cycle_net_power_design_kWe;
                assign("kwh_per_kw", var_data((ssc_number_t)kWhperkW));

                double capacity_fac = first_year_total_energy / cycle_net_power_design_kWe / 87.6;  //[%]
                assign("capacity_factor", var_data((ssc_number_t)(capacity_fac)));		//[%] 

                assign("annual_energy", var_data((ssc_number_t)(first_year_total_energy)));
                accumulate_monthly_for_year("gen", "monthly_energy", 8760.0 / n_rec, n_rec / 8760);

                //return;
            }

        }

        SGeothermal_Inputs geo_inputs;

        geo_inputs.me_rt = e_resource_type;
        geo_inputs.me_tdm = e_decline_type;
        geo_inputs.me_ct = e_conversion_type;
        geo_inputs.me_pc = e_reservoir_pressure_change_type;
        geo_inputs.me_ft = e_flash_type;

        // inputs used in above in refactor
        geo_inputs.md_dtProdWell = dt_prod_well;            //[C]
        geo_inputs.md_dtProdWellChoice = dtProdWellChoice;  //[-]
        geo_inputs.md_TemperatureWetBulbC = TemperatureWetBulbC;    //[C]
        geo_inputs.md_PlantEfficiency = PlantEfficiency;    //[-]

		// set the geothermal model inputs -------------------------------------
        geo_inputs.md_RatioInjectionToProduction = ratio_injection_to_production; // THIS SHOULD BE AN INPUT. ALTHOUGH IT'S FROM THE COST PAGE, IT'S USED IN NON-COST EQUATION
        geo_inputs.md_DrillSuccessRate = drill_success_rate;
        geo_inputs.md_StimSuccessRate = stim_success_rate;
        geo_inputs.md_FailedProdFlowRatio = FailedProdFlowRatio;
        geo_inputs.md_DesiredSalesCapacityKW = desired_sales_capacity_kw;
        geo_inputs.md_NumberOfWells = num_wells_in;
        geo_inputs.md_WellsStimulated = static_cast<int> (geo_wells_stim_type);

        geo_inputs.me_cb = calc_basis_type;

		geo_inputs.md_TemperatureDeclineRate = TemperatureDeclineRate;
		geo_inputs.md_MaxTempDeclineC = MaxTempDeclineC;
        
		// flash inputs
		geo_inputs.md_PressureAmbientPSI = PressureAmbientPSI;
        geo_inputs.md_UseWeatherFileConditions = 0; //initially set to zero for UI calculations

        geo_inputs.mi_simulation_timestep_type = static_cast<geo_simulation_timestep_type>(as_integer("simulation_timestep_type"));    // 0=monthly, 1=hourly

        // Need to use this input during annual simulation but not design
        // So save as a separate member variable, the use this new variable to set md_UseWeatherFileConditions during annual simulation
        geo_inputs.md_UseWeatherFileConditions_annual_sim = use_weather_file_conditions;

        // If hourly timestep, always want to use weather file conditions
        if( geo_inputs.mi_simulation_timestep_type == HOURLY_TIMESTEPS ){
            geo_inputs.md_UseWeatherFileConditions_annual_sim = 1;
        }

		//pumping parameters
		geo_inputs.md_ProductionFlowRateKgPerS = ProductionFlowRateKgPerS;
		geo_inputs.md_GFPumpEfficiency = gf_pump_efficiency;                    //[-]
        geo_inputs.md_PressureChangeAcrossSurfaceEquipmentPSI = PressureChangeAcrossSurfaceEquipment;
        geo_inputs.md_ExcessPressureBar = ExcessPressureBar;
		geo_inputs.md_DiameterProductionWellInches = 0.0;
        geo_inputs.md_ProductionWellType = ProductionWellType;
        geo_inputs.md_ProductionWellDiam = ProductionWellDiam;

		geo_inputs.md_DiameterPumpCasingInches = 0.0;
        geo_inputs.md_DiameterInjPumpCasingInches = 0.0;
		geo_inputs.md_DiameterInjectionWellInches = 0.0;
        geo_inputs.md_InjectionWellType = InjectionWellType;
        geo_inputs.md_InjectionWellDiam = InjectionWellDiam;
		geo_inputs.mb_CalculatePumpWork = ( 1 != as_integer("specify_pump_work") );
		geo_inputs.md_UserSpecifiedPumpWorkKW = as_double("specified_pump_work_amount") * 1000; // entered in MW

		geo_inputs.md_ResourceDepthM = resource_depth_in;
		geo_inputs.md_TemperatureResourceC = resource_temperature;
		geo_inputs.me_dc = depth_calc_EGS_type;
		geo_inputs.md_TemperaturePlantDesignC = design_temp;

		//reservoir properties
		geo_inputs.md_TemperatureEGSAmbientC = 10.0;
		geo_inputs.md_EGSThermalConductivity = as_double("rock_thermal_conductivity");
		geo_inputs.md_EGSSpecificHeatConstant = as_double("rock_specific_heat");
		geo_inputs.md_EGSRockDensity = as_double("rock_density");
		
		geo_inputs.md_ReservoirDeltaPressure = reservoir_pressure_chage;
        geo_inputs.md_InjectivityIndex = InjectivityIndex;
        
        geo_inputs.md_ExplorationWellsProd = num_conf_wells_prod;

		geo_inputs.md_ReservoirWidthM = as_double("reservoir_width");
		geo_inputs.md_ReservoirHeightM = as_double("reservoir_height");
		geo_inputs.md_ReservoirPermeability = as_double("reservoir_permeability");
		geo_inputs.md_DistanceBetweenProductionInjectionWellsM = as_double("inj_prod_well_distance");
		geo_inputs.md_WaterLossPercent = water_loss_frac;      // this variable is mis-named
		geo_inputs.md_EGSFractureAperature = as_double("fracture_aperature");
        geo_inputs.md_EGSFractureLength = as_double("fracture_length");
        geo_inputs.md_EGSFractureSpacing = as_double("fracture_spacing");
		geo_inputs.md_EGSNumberOfFractures = as_double("num_fractures");
		geo_inputs.md_EGSFractureWidthM = as_double("fracture_width");
		geo_inputs.md_EGSFractureAngle = as_double("fracture_angle");

        geo_inputs.md_AllowReservoirReplacements = AllowReservoirReplacements;

		// calculate output array sizes
		//geo_inputs.mi_ModelChoice = as_integer("model_choice");		                    // 0=GETEM, 1=Power Block monthly, 2=Power Block hourly
        //geo_inputs.mi_cycle_model_type = as_integer("geo_cycle_model_type");		            // 0=GETEM, 1=User Defined, 2=Reduced Order
        geo_inputs.mi_cycle_model_type = static_cast<geo_cycle_model_type>(as_integer("geo_cycle_model_type"));	// GETEM_CYCLE = 0, REDUCED_ORDER_CYCLE = 1, USER_DEFINED_CYCLE = 2, UNDEFINED_CYCLE_MODEL = -1


        if (is_assigned("reservoir_model_inputs")) 
            geo_inputs.md_ReservoirInputs = as_matrix("reservoir_model_inputs");

        geo_inputs.mi_ProjectLifeYears = analysis_period_years; // as_integer("geothermal_analysis_period");
        assign("geothermal_analysis_period", (ssc_number_t)analysis_period_years);

		if ( geo_inputs.mi_ProjectLifeYears == 0)
			throw general_error("invalid analysis period specified in the geothermal hourly model");

        // running the model, we need to specify other inputs
        geo_inputs.md_PotentialResourceMW = PotentialResourceMW;

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
        pbInputs.m_dot_htf = GF_flowrate / 2.20462;         //[lb/hr] * 1/2.20462 [kg/lb] = [kg/hr]
        pbInputs.demand_var = pbInputs.m_dot_htf;           //[kg/hr]
        pbInputs.standby_control = 1;
        pbInputs.rel_humidity = 0.7;

        assign("num_wells_getem", var_data((ssc_number_t)geo_outputs.md_NumberOfWells));
        assign("num_wells_getem_output", var_data((ssc_number_t)geo_outputs.md_NumberOfWells));
        assign("num_wells_getem_prod_drilled", var_data((ssc_number_t)geo_outputs.md_NumberOfWellsProdDrilled));
        assign("num_wells_getem_prod_failed", var_data((ssc_number_t)geo_outputs.md_NumberOfWellsProdFailed));
        assign("num_wells_getem_inj", var_data((ssc_number_t)geo_outputs.md_NumberOfWellsInj));
        assign("num_wells_getem_inj_drilled", var_data((ssc_number_t)geo_outputs.md_NumberOfWellsInjDrilled));
        assign("num_confirm_wells_to_production", var_data((ssc_number_t)num_conf_wells_prod));

        // The GETEM flash model as of 2026.06.26 uses a different value for gross turbine output
        double gross_cost_output_for_flash = std::numeric_limits<double>::quiet_NaN();
        if( geo_inputs.me_ct == FLASH ) {
            gross_cost_output_for_flash = geo_outputs.md_GrossPowerkW;
        }
        assign("gross_cost_output", var_data((ssc_number_t)gross_cost_output_for_flash));

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
        assign("eff_secondlaw", (ssc_number_t)eff_secondlaw);       // plant-level (i.e. not considering pump work) 2nd law efficiency

        assign("pump_depth_ft", var_data((ssc_number_t)geo_outputs.md_PumpDepthFt));
        double pump_depth_ft_calc2 = geo_outputs.md_CalculatedPumpDepthInFeet;
        assign("inj_pump_hp", var_data((ssc_number_t)geo_outputs.md_InjPump_hp));

        assign("plant_brine_eff", var_data((ssc_number_t)geo_outputs.md_PlantBrineEffectiveness));
        assign("pump_watthr_per_lb", var_data((ssc_number_t)geo_outputs.md_PumpWorkWattHrPerLb));
        assign("pumpwork_prod", var_data((ssc_number_t)geo_outputs.md_pumpwork_prod));      //[W-hr/lb]
        assign("pumpwork_inj", var_data((ssc_number_t)geo_outputs.md_pumpwork_inj));        //[W-hr/lb]
        assign("gross_output", var_data((ssc_number_t)geo_outputs.md_GrossPlantOutputMW));

        assign("reservoir_pressure", var_data((ssc_number_t)geo_outputs.md_PressureChangeAcrossReservoir));
        assign("reservoir_avg_temp", var_data((ssc_number_t)physics::FarenheitToCelcius(geo_outputs.md_AverageReservoirTemperatureF)));

        // This calculation doesn't seem consistent in the code, so let's try not to use it until we update the code
        assign("bottom_hole_pressure", var_data((ssc_number_t) -6.54321));           // geo_outputs.md_BottomHolePressure));

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

        // Check cost and OM model calcs
        double total_installed_cost_calc_old = std::numeric_limits<double>::quiet_NaN();
        double total_getem_om_cost_old = std::numeric_limits<double>::quiet_NaN();

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

            total_installed_cost_calc_old = as_double("total_installed_cost");

            // *************************************************************
            // *************************************************************

            if( geo_fin_model < 7 ) {
                std::string construction_financing_module_name = "cb_construction_financing";
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
                assign("num_wells_calc", as_double("num_wells_getem"));
                assign("drilling_cost", as_double("total_drilling_cost_used"));
                assign("field_gathering_system_cost", as_double("total_surface_equipment_cost"));
                assign("water_loss", as_double("subsurface_water_loss"));
                assign("pump_depth", as_double("pump_depth_ft"));
                assign("pump_type", 0);

                geo_cmod_inputs = static_cast<ssc_data_t>(geo_cost_vtab);
                getem_om_cost_calc(geo_cmod_inputs);

                total_getem_om_cost_old = as_double("total_getem_om_cost");
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
            if(geo_inputs.mi_simulation_timestep_type == HOURLY_TIMESTEPS) {
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
            
            size_t n_rec = 8760;
            if( as_boolean("system_use_lifetime_output") ) {
                n_rec *= geo_inputs.mi_ProjectLifeYears;
            }

            // Still want hourly reporting for some outputs even if simulation timestep is monthly
            geo_outputs.maf_timestep_dry_bulb = allocate("timestep_dry_bulb", n_rec);
            geo_outputs.maf_timestep_wet_bulb = allocate("timestep_wet_bulb", n_rec);

            geo_outputs.maf_AE = allocate("AE_od", n_rec);
            geo_outputs.maf_getem_2nd_law_total_od = allocate("getem_2nd_law_total_od", n_rec);
            geo_outputs.maf_carnot_od_scaling = allocate("carnot_scaling_od", n_rec);
            geo_outputs.maf_cycle_net_power_od = allocate("cycle_net_power_od", n_rec);
            geo_outputs.maf_plant_net_power_od = allocate("plant_net_power_od", n_rec);
            geo_outputs.maf_brine_pumping_power_od = allocate("brine_pumping_power_od", n_rec);

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

			double first_year_energy = 0;
			for (size_t i = 0; i < 12; ++i) {
				first_year_energy += geo_outputs.maf_monthly_energy[i];
			}
			assign("first_year_output", var_data(first_year_energy));

			// metric outputs moved to technology
			double capacity_fac;	//geothermal plant capacity factor
			double kWhperkW = 0.0;
			double nameplate = geo_inputs.md_DesiredSalesCapacityKW; // Was md_GrossPlantOutputMW*1000 -> now it is md_DesiredSalesCapacityKW
			//double annual_energy = 0.0;

			//Loop calculates total energy generation over entire project lifetime (in kWh) 
			// Why?  Is the annual energy changing from year to year?
			for (size_t i = 0; i <n_rec ; i++)	{
				//annual_energy += geo_outputs.maf_hourly_power[i];
                p_gen[i] = geo_outputs.maf_hourly_power[i];
			}

            gen_heatmap(this, 1);

			if (nameplate > 0) kWhperkW = first_year_energy / nameplate;
			capacity_fac = first_year_energy / nameplate;

			//assign("gross_output", var_data((ssc_number_t)geo_outputs.md_GrossPlantOutputMW));
            //assign("gross_cost_output", var_data((ssc_number_t)geo_outputs.md_GrossPowerkW));
			assign("capacity_factor", var_data((ssc_number_t)(capacity_fac / 87.6)));		//Divided by 8760 and then multiplied by 100 (or divide by 87.6) to return CF as a %
			assign("kwh_per_kw", var_data((ssc_number_t)kWhperkW));
			// 5/28/15 average provided for FCR market
			assign("annual_energy", var_data((ssc_number_t)(first_year_energy)));
            accumulate_monthly_for_year("gen", "monthly_energy", 8760.0 / n_rec, n_rec / 8760);
		}

        delete p_geo_prod_well;
        delete p_geo_surface;
        delete p_geo_subsurface;
        delete p_geo_weather_file;
	}

};

DEFINE_MODULE_ENTRY( geothermal, "Geothermal hourly or monthly annual and multi-year simulation", 4 );

// above is equivalent to:
//static compute_module *_create_geothermal () { return new cm_geothermal; }
//module_entry_info cm_entry_geothermal = { "geothermal", "Geothermal monthly...", 3, _create_geothermal }; 
