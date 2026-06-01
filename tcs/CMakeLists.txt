#####################################################################################################################
#
# Project Settings
#
#####################################################################################################################

include_directories(. ../nlopt ../shared ../lpsolve ../solarpilot $ENV{ORTOOLSDIR}/include/eigen3 $ENV{ORTOOLSDIR}/include )

set(TCS_SRC
		base_dispatch.cpp							base_dispatch.h
        base_dispatch_lpsolve.cpp					base_dispatch_lpsolve.h
        base_dispatch_ortools.cpp					base_dispatch_ortools.h
		cavity_calcs.cpp							cavity_calcs.h
		co2_compressor_library.cpp					co2_compressor_library.h
		co2_testing.h
		CO2_properties.cpp							CO2_properties.h
		csp_dispatch.cpp							csp_dispatch.h
        csp_dispatch_ortools.cpp					csp_dispatch_ortools.h
		csp_radiator.cpp							csp_radiator.h
		csp_solver_battery.cpp						csp_solver_battery.h
		csp_solver_cavity_receiver.cpp				csp_solver_cavity_receiver.h
		csp_solver_core.cpp							csp_solver_core.h
		csp_solver_cr_electric_resistance.cpp		csp_solver_cr_electric_resistance.h
		csp_solver_cr_heat_pump.cpp					csp_solver_cr_heat_pump.h
		csp_solver_fresnel_collector_receiver.cpp	csp_solver_fresnel_collector_receiver.h
		csp_solver_gen_collector_receiver.cpp		csp_solver_gen_collector_receiver.h
		csp_solver_lf_dsg_collector_receiver.cpp	csp_solver_lf_dsg_collector_receiver.h
		csp_solver_mono_eq_methods.cpp
		csp_solver_mspt_collector_receiver.cpp		csp_solver_mspt_collector_receiver.h
		csp_solver_mspt_receiver.cpp				csp_solver_mspt_receiver.h
		csp_solver_mspt_receiver_222.cpp			csp_solver_mspt_receiver_222.h
		csp_solver_piston_cylinder_tes.cpp			csp_solver_piston_cylinder_tes.h
		csp_solver_packedbed_tes.cpp				csp_solver_packedbed_tes.h
		csp_solver_pc_gen.cpp						csp_solver_pc_gen.h
		csp_solver_pc_heat_sink.cpp					csp_solver_pc_heat_sink.h
		csp_solver_pc_heat_sink_physical.cpp		csp_solver_pc_heat_sink_physical.h
		csp_solver_pc_ptes.cpp						csp_solver_pc_ptes.h
		csp_solver_pc_Rankine_indirect_224.cpp		csp_solver_pc_Rankine_indirect_224.h
		csp_solver_pc_steam_heat_sink.cpp			csp_solver_pc_steam_heat_sink.h
		csp_solver_pt_receiver.cpp					csp_solver_pt_receiver.h
		csp_solver_pt_sf_perf_interp.cpp			csp_solver_pt_sf_perf_interp.h
		csp_solver_stratified_tes.cpp				csp_solver_stratified_tes.h
        csp_solver_tes_core.cpp						csp_solver_tes_core.h
		csp_solver_tou_block_schedules.cpp
		csp_solver_trough_collector_receiver.cpp	csp_solver_trough_collector_receiver.h
		csp_solver_two_tank_tes.cpp					csp_solver_two_tank_tes.h
		csp_solver_util.cpp							csp_solver_util.h
		csp_solver_weatherreader.cpp
		csp_system_costs.cpp						csp_system_costs.h
		cst_iph_dispatch.cpp						cst_iph_dispatch.h
		datatest.cpp
		dispatch_builder.cpp						dispatch_builder.h
		etes_dispatch.cpp							etes_dispatch.h
		flat_plate_solar_collector.cpp				flat_plate_solar_collector.h
		fmin.cpp									fmin.h
		fmin_callbacks.cpp							fmin_callbacks.h
		heat_exchangers.cpp							heat_exchangers.h
		htf_props.cpp								htf_props.h
		interconnect.cpp							interconnect.h
		interpolation_routines.cpp					interpolation_routines.h
		nlopt_callbacks.cpp							nlopt_callbacks.h
		numeric_solvers.cpp							numeric_solvers.h
        ptes_solver_design_point.cpp				ptes_solver_design_point.h
        pump.cpp
		sam_csp_util.cpp							sam_csp_util.h
		sam_lf_st_pt_type232.cpp
		sam_mw_gen_Type260_csp_solver.cpp
		sam_mw_lf_type261_steam.cpp
		sam_mw_lf_type261_Wnet.cpp
		sam_mw_pt_type228.cpp
		sam_mw_type234.cpp
		sam_trough_model_type805.cpp
		sam_trough_plant_type807.cpp
		sam_trough_storage_type806.cpp
		sco2_cycle_components.cpp					sco2_cycle_components.h
		sco2_cycle_templates.h
        sco2_htrbypass_cycle.cpp					sco2_htrbypass_cycle.h
		sco2_partialcooling_cycle.cpp				sco2_partialcooling_cycle.h
		sco2_pc_csp_int.cpp							sco2_pc_csp_int.h
		sco2_power_cycle.cpp						sco2_power_cycle.h
		sco2_recompression_cycle.cpp				sco2_recompression_cycle.h
        sco2_turbinesplitflow_cycle.cpp				sco2_turbinesplitflow_cycle.h
		storage_hx.cpp								storage_hx.h
		sumprod.cpp
		tcskernel.cpp								tcskernel.h
		tcstype.h
		tcs_debug.h
		tou_translator.cpp
		trnsys_weatherreader.cpp
		typelib.cpp
		ud_power_cycle.cpp							ud_power_cycle.h
		user_defined_power_cycle.h
		water_properties.cpp						water_properties.h
		waterprop.cpp								waterprop.h
		weatherreader_csp_solver.cpp
)


#####################################################################################################################
#
# CMake Targets
#
#####################################################################################################################

add_library(tcs STATIC ${TCS_SRC})
set_target_properties(tcs
    PROPERTIES
    DEBUG_POSTFIX "d"
	PREFIX ""
	)

set_default_compile_options(tcs)
set_no_warnings(tcs)
if (MSVC)
    set_additional_compile_options(tcs "/bigobj")
endif ()


#####################################################################################################################
#
# Link Libraries and Options
#
#####################################################################################################################

set( DEPENDENCIES
	splinter
	)

if (MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
endif()
