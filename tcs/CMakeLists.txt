#####################################################################################################################
#
# Project Settings
#
#####################################################################################################################

include_directories(. ../nlopt ../shared ../lpsolve ../solarpilot $ENV{ORTOOLSDIR}/include/eigen3)

set(TCS_SRC
		base_dispatch.cpp
        base_dispatch_lpsolve.cpp
		co2_compressor_library.cpp
		CO2_properties.cpp
		csp_dispatch.cpp
		csp_radiator.cpp
		csp_solver_cavity_receiver.cpp
		csp_solver_core.cpp
		csp_solver_cr_electric_resistance.cpp
		csp_solver_cr_heat_pump.cpp
        csp_solver_fresnel_collector_receiver.cpp
		csp_solver_gen_collector_receiver.cpp
		csp_solver_lf_dsg_collector_receiver.cpp
		csp_solver_mono_eq_methods.cpp
		csp_solver_mspt_collector_receiver.cpp
		csp_solver_mspt_receiver.cpp
		csp_solver_mspt_receiver_222.cpp
        csp_solver_piston_cylinder_tes.cpp
        csp_solver_packedbed_tes.cpp
		csp_solver_pc_gen.cpp
		csp_solver_pc_heat_sink.cpp
        csp_solver_pc_heat_sink_physical.cpp
		csp_solver_pc_ptes.cpp
		csp_solver_pc_Rankine_indirect_224.cpp
		csp_solver_pc_steam_heat_sink.cpp
		csp_solver_pt_receiver.cpp
		csp_solver_pt_sf_perf_interp.cpp
		csp_solver_stratified_tes.cpp
        csp_solver_tes_core.cpp
		csp_solver_tou_block_schedules.cpp
		csp_solver_trough_collector_receiver.cpp
		csp_solver_two_tank_tes.cpp
		csp_solver_util.cpp
		csp_solver_weatherreader.cpp
		csp_system_costs.cpp
		cst_iph_dispatch.cpp
		dispatch_builder.cpp
		etes_dispatch.cpp
		fmin.cpp
		fmin_callbacks.cpp
		heat_exchangers.cpp
		interconnect.cpp
		nlopt_callbacks.cpp
		numeric_solvers.cpp
        ptes_solver_design_point.cpp
		sco2_cycle_components.cpp
		sco2_partialcooling_cycle.cpp
		sco2_pc_csp_int.cpp
		sco2_power_cycle.cpp
		sco2_recompression_cycle.cpp
		tcskernel.cpp
		trnsys_weatherreader.cpp
		typelib.cpp
		ud_power_cycle.cpp
		water_properties.cpp
		waterprop.cpp
		weatherreader_csp_solver.cpp
		cavity_calcs.cpp
		datatest.cpp
		htf_props.cpp
		interpolation_routines.cpp
		pump.cpp
		sam_csp_util.cpp
		sam_lf_st_pt_type232.cpp
		sam_mw_gen_Type260_csp_solver.cpp
		sam_mw_lf_type261_steam.cpp
		sam_mw_lf_type261_Wnet.cpp
		sam_mw_pt_type228.cpp
		sam_mw_type234.cpp
		sam_trough_model_type805.cpp
		sam_trough_plant_type807.cpp
		sam_trough_storage_type806.cpp
		storage_hx.cpp
		sumprod.cpp
		tou_translator.cpp
		flat_plate_solar_collector.cpp
		base_dispatch.h
        base_disptach_lpsolve.h
		cavity_calcs.h
		co2_compressor_library.h
		CO2_properties.h
		co2_testing.h
		csp_dispatch.h
		csp_radiator.h
		csp_solver_cavity_receiver.h
		csp_solver_core.h
		csp_solver_cr_electric_resistance.h
		csp_solver_cr_heat_pump.h
        csp_solver_fresnel_collector_receiver.h
		csp_solver_gen_collector_receiver.h
		csp_solver_lf_dsg_collector_receiver.h
		csp_solver_mspt_collector_receiver.h
		csp_solver_mspt_receiver.h
		csp_solver_mspt_receiver_222.h
        csp_solver_piston_cylinder_tes.h
        csp_solver_packedbed_tes.h
		csp_solver_pc_gen.h
		csp_solver_pc_heat_sink.h
        csp_solver_pc_heat_sink_physical.h
		csp_solver_pc_ptes.h
		csp_solver_pc_Rankine_indirect_224.h
		csp_solver_pc_steam_heat_sink.h
		csp_solver_pt_receiver.h
		csp_solver_pt_sf_perf_interp.h
		csp_solver_stratified_tes.h
        csp_solver_tes_core.h
		csp_solver_trough_collector_receiver.h
		csp_solver_two_tank_tes.h
		csp_solver_util.h
		csp_system_costs.h
		cst_iph_dispatch.h
		dispatch_builder.h
		etes_dispatch.h
		flat_plate_solar_collector.h
		fmin.h
		fmin_callbacks.h
		heat_exchangers.h
		htf_props.h
		interconnect.h
		interpolation_routines.h
		nlopt_callbacks.h
		numeric_solvers.h
        ptes_solver_design_point.h
		sam_csp_util.h
		sco2_cycle_components.h
		sco2_cycle_templates.h
		sco2_partialcooling_cycle.h
		sco2_pc_csp_int.h
		sco2_power_cycle.h
		sco2_recompression_cycle.h
		storage_hx.h
		tcskernel.h
		tcstype.h
		tcs_debug.h
		ud_power_cycle.h
		user_defined_power_cycle.h
		waterprop.h
		water_properties.h
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
