#####################################################################################################################
#
# Project Settings
#
#####################################################################################################################
set(CMAKE_VERBOSE_MAKEFILE ON)

include_directories(. ../splinter/include $ENV{ORTOOLSDIR}/include/eigen3 $ENV{ORTOOLSDIR}/include ../)

set(SHARED_SRC
        6par_gamma.h
        6par_jacobian.h
        6par_lu.h
        6par_newton.h
        6par_search.h
        6par_solve.h
        CMakeLists.txt
        DB8_vmpp_impp_uint8_bin.h
        lib_aoi.h
        lib_battery.cpp
        lib_battery.h
        lib_battery_capacity.cpp
        lib_battery_capacity.h
		lib_battery_dispatch.cpp
		lib_battery_dispatch.h
		lib_battery_dispatch_automatic_btm.cpp
		lib_battery_dispatch_automatic_btm.h
        lib_battery_dispatch_automatic_fom.cpp
        lib_battery_dispatch_automatic_fom.h
        lib_battery_dispatch_pvsmoothing_fom.cpp
        lib_battery_dispatch_pvsmoothing_fom.h
		lib_battery_dispatch_manual.cpp
		lib_battery_dispatch_manual.h
        lib_battery_lifetime.cpp
        lib_battery_lifetime.h
        lib_battery_lifetime_calendar_cycle.cpp
        lib_battery_lifetime_calendar_cycle.h
        lib_battery_lifetime_lmolto.cpp
        lib_battery_lifetime_lmolto.h
        lib_battery_lifetime_nmc.cpp
        lib_battery_lifetime_nmc.h
        lib_battery_powerflow.cpp
        lib_battery_powerflow.h
        lib_battery_voltage.cpp
        lib_battery_voltage.h
        lib_cec6par.cpp
        lib_cec6par.h
        lib_financial.cpp
        lib_financial.h
        lib_fuel_cell.cpp
        lib_fuel_cell.h
        lib_fuel_cell_dispatch.cpp
        lib_fuel_cell_dispatch.h
        lib_geothermal.cpp
        lib_geothermal.h
        lib_iec61853.cpp
        lib_iec61853.h
        lib_irradproc.cpp
        lib_irradproc.h
        lib_miniz.cpp
        lib_miniz.h
        lib_mlmodel.cpp
        lib_mlmodel.h
        lib_ondinv.cpp
        lib_ondinv.h
        lib_physics.cpp
        lib_physics.h
        lib_power_electronics.cpp
        lib_power_electronics.h
        lib_powerblock.cpp
        lib_powerblock.h
        lib_ptes_chp_dispatch.cpp
        lib_ptes_chp_dispatch.h
        lib_pv_incidence_modifier.cpp
        lib_pv_incidence_modifier.h
        lib_pv_io_manager.cpp
        lib_pv_io_manager.h
        lib_pv_shade_loss_mpp.cpp
        lib_pv_shade_loss_mpp.h
        lib_pvinv.cpp
        lib_pvinv.h
        lib_pvmodel.cpp
        lib_pvmodel.h
        lib_pvshade.cpp
        lib_pvshade.h
        lib_pvwatts.cpp
        lib_pvwatts.h
        lib_resilience.cpp
        lib_resilience.h
        lib_sandia.cpp
        lib_sandia.h
        lib_shared_inverter.cpp
        lib_shared_inverter.h
        lib_snowmodel.cpp
        lib_snowmodel.h
        lib_time.cpp
        lib_time.h
        lib_util.cpp
        lib_util.h
        lib_utility_rate.cpp
        lib_utility_rate.h
        lib_utility_rate_equations.cpp
        lib_utility_rate_equations.h
        lib_weatherfile.cpp
        lib_weatherfile.h
        lib_wind_obos.cpp
        lib_wind_obos.h
        lib_wind_obos_cable_vessel.cpp
        lib_wind_obos_cable_vessel.h
        lib_windfile.cpp
        lib_windfile.h
        lib_windwakemodel.cpp
        lib_windwakemodel.h
        lib_windwatts.cpp
        lib_windwatts.h
        logger.cpp logger.h
        lsqfit.cpp
        lsqfit.h
        )


#####################################################################################################################
#
# CMake Targets
#
#####################################################################################################################

add_library(shared STATIC ${SHARED_SRC})
set_target_properties(shared
        PROPERTIES
        DEBUG_POSTFIX "d"
        PREFIX ""
        )

set_default_compile_options(shared)
set_disabled_warnings(shared)

#####################################################################################################################
#
# Link Libraries and Options
#
#####################################################################################################################

set( DEPENDENCIES
	splinter
	)

if (USE_XPRESS)
    if (NOT DEFINED XPRESS_ROOT AND DEFINED ENV{XPRESS_ROOT})
        set(XPRESS_ROOT $ENV{XPRESS_ROOT})
    endif()
    add_library(XPRESS::XPRESS UNKNOWN IMPORTED)

    if(UNIX)
        target_include_directories(XPRESS::XPRESS SYSTEM INTERFACE "${XPRESS_ROOT}/include")
    endif()

    if(APPLE)
        set_target_properties(XPRESS::XPRESS PROPERTIES
            IMPORTED_LOCATION "${XPRESS_ROOT}/lib/libxprs.dylib")
    elseif(UNIX)
        set_target_properties(XPRESS::XPRESS PROPERTIES
            INTERFACE_LINK_DIRECTORIES "${XPRESS_ROOT}/lib"
            IMPORTED_LOCATION ${XPRESS_ROOT}/lib/libxprs.so)
    elseif(MSVC)
        set_target_properties(XPRESS::XPRESS PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${XPRESS_ROOT}\\include"
            IMPORTED_LOCATION "${XPRESS_ROOT}\\lib\\xprs.lib")
    else()
        message(FATAL_ERROR "XPRESS not supported for ${CMAKE_SYSTEM}")
    endif()
    target_link_libraries(shared XPRESS::XPRESS)
endif()


if (UNIX)
    find_package(ortools
                CONFIG REQUIRED)
    target_link_libraries(shared ortools::ortools)
elseif(MSVC)
    function(find_ortools_package library)
        find_package(${library}
            PATHS "$ENV{ORTOOLSDIR}/lib/cmake/${library}"
                  "$ENV{ORTOOLSDIR}/cmake"
                  "$ENV{ORTOOLSDIR}/share/${library}/cmake"
                  "$ENV{ORTOOLSDIR}/include"
                  "$ENV{ORTOOLSDIR}/lib"
                  $ENV{ORTOOLSDIR}
            REQUIRED
        )
    endfunction()

    function(set_library_by_config target library has_d_suffix)
        string(REPLACE "::" "_" library_name ${library})
        set(lib_release "LIB_${library_name}_RELEASE")
        set(lib_debug "LIB_${library_name}_DEBUG")
        if (has_d_suffix)
            string(REPLACE ".lib" "d.lib" lib_debug ${lib_debug})
        endif()
        set(libs "LIB_${tlibrary_name}")
        get_target_property(lib_release ${library} LOCATION)
        message("${library} path is ${lib_release}")
        if (NOT "${lib_release}" STREQUAL "lib_release-NOTFOUND")
            string(REPLACE release debug lib_debug ${lib_release})
            set(libs
                    "$<$<NOT:$<CONFIG:DEBUG>>:${lib_release}>"
                    "$<$<CONFIG:DEBUG>:${lib_debug}>")
        target_link_libraries(${target} ${libs})
        endif()
    endfunction()

    # OR-Tools must be version 9.8
    find_ortools_package(absl)
    foreach(_absl_target IN ITEMS absl::atomic_hook absl::errno_saver absl::log_severity absl::nullability absl::raw_logging_internal absl::spinlock_wait absl::config absl::dynamic_annotations absl::core_headers absl::malloc_internal absl::base_internal absl::base absl::throw_delegate absl::pretty_function absl::endian absl::scoped_set_env absl::strerror absl::fast_type_id absl::prefetch absl::algorithm absl::algorithm_container absl::cleanup_internal absl::cleanup absl::btree absl::compressed_tuple absl::fixed_array absl::inlined_vector_internal absl::inlined_vector absl::flat_hash_map absl::flat_hash_set absl::node_hash_map absl::node_hash_set absl::container_memory absl::hash_function_defaults absl::hash_policy_traits absl::common_policy_traits absl::hashtablez_sampler absl::hashtable_debug absl::hashtable_debug_hooks absl::node_slot_policy absl::raw_hash_map absl::container_common absl::raw_hash_set absl::layout absl::crc_cpu_detect absl::crc_internal absl::crc32c absl::non_temporal_arm_intrinsics absl::non_temporal_memcpy absl::crc_cord_state absl::stacktrace absl::symbolize absl::examine_stack absl::failure_signal_handler absl::debugging_internal absl::demangle_internal absl::leak_check absl::debugging absl::flags_path_util absl::flags_program_name absl::flags_config absl::flags_marshalling absl::flags_commandlineflag_internal absl::flags_commandlineflag absl::flags_private_handle_accessor absl::flags_reflection absl::flags_internal absl::flags absl::flags_usage_internal absl::flags_usage absl::flags_parse absl::any_invocable absl::bind_front absl::function_ref absl::hash absl::city absl::low_level_hash absl::log_internal_check_impl absl::log_internal_check_op absl::log_internal_conditions absl::log_internal_config absl::log_internal_flags absl::log_internal_format absl::log_internal_globals absl::log_internal_log_impl absl::log_internal_proto absl::log_internal_message absl::log_internal_log_sink_set absl::log_internal_nullguard absl::log_internal_nullstream absl::log_internal_strip absl::log_internal_voidify absl::log_internal_append_truncated absl::absl_check absl::absl_log absl::check absl::die_if_null absl::log_flags absl::log_globals absl::log_initialize absl::log absl::log_entry absl::log_sink absl::log_sink_registry absl::log_streamer absl::log_internal_structured absl::log_structured absl::memory absl::type_traits absl::meta absl::bits absl::int128 absl::numeric absl::numeric_representation absl::sample_recorder absl::exponential_biased absl::periodic_sampler absl::random_random absl::random_bit_gen_ref absl::random_internal_mock_helpers absl::random_distributions absl::random_seed_gen_exception absl::random_seed_sequences absl::random_internal_traits absl::random_internal_distribution_caller absl::random_internal_fast_uniform_bits absl::random_internal_seed_material absl::random_internal_pool_urbg absl::random_internal_salted_seed_seq absl::random_internal_iostream_state_saver absl::random_internal_generate_real absl::random_internal_wide_multiply absl::random_internal_fastmath absl::random_internal_nonsecure_base absl::random_internal_pcg_engine absl::random_internal_randen_engine absl::random_internal_platform absl::random_internal_randen absl::random_internal_randen_slow absl::random_internal_randen_hwaes absl::random_internal_randen_hwaes_impl absl::random_internal_distribution_test_util absl::random_internal_uniform_helper absl::status absl::statusor absl::string_view absl::strings absl::strings_internal absl::str_format absl::str_format_internal absl::cord_internal absl::cordz_update_tracker absl::cordz_functions absl::cordz_statistics absl::cordz_handle absl::cordz_info absl::cordz_sample_token absl::cordz_update_scope absl::cord absl::graphcycles_internal absl::kernel_timeout_internal absl::synchronization absl::time absl::civil_time absl::time_zone absl::any absl::bad_any_cast absl::bad_any_cast_impl absl::span absl::optional absl::bad_optional_access absl::bad_variant_access absl::variant absl::compare absl::utility absl::if_constexpr)
        set_library_by_config(shared ${_absl_target} FALSE)
    endforeach()

    find_ortools_package(utf8_range)
    set_library_by_config(shared utf8_range::utf8_validity FALSE)

    if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR "Debug" IN_LIST CMAKE_CONFIGURATION_TYPES)
        find_library( ZLIBD
                NAMES zlib.lib
                PATHS $ENV{ORTOOLSDBDIR}/lib)
        target_link_libraries(shared debug ${ZLIBD})
    endif()
    find_ortools_package(zlib)
    get_target_property(LIB_ZLIB_RELEASE ZLIB::ZLIB LOCATION)
    target_link_libraries(shared optimized ${LIB_ZLIB_RELEASE})

    find_ortools_package(protobuf)
    set_library_by_config(shared protobuf::libprotobuf TRUE)

     if(USE_GLPK)
        find_ortools_package(glpk)
        set_library_by_config(shared GLPK::GLPK FALSE)
    endif()

    if(USE_COINOR)
        find_ortools_package(CoinUtils)
        find_ortools_package(Osi)
        find_ortools_package(Clp)
        find_ortools_package(Cgl)
        find_ortools_package(Cbc)
        foreach(_target IN ITEMS Coin::CoinUtils Coin::Osi Coin::Clp Coin::OsiClp Coin::ClpSolver Coin::Cbc Coin::OsiCbc Coin::CbcSolver Coin::Cgl)
            set_library_by_config(shared ${_target} FALSE)
        endforeach()
    endif()

    find_ortools_package(re2)
    find_ortools_package(Eigen3)
    find_ortools_package(utf8_range)
    find_ortools_package(scip)
    find_ortools_package(ortools)

    foreach(_target IN ITEMS re2::re2 Eigen3::Eigen utf8_range::utf8_validity ortools::ortools)
        set_library_by_config(shared ${_target} FALSE)
    endforeach()
endif()

if (UNIX)
   set(CMAKE_SHARED_LINKER_FLAGS "-lm")
 endif ()
if (MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
endif ()
