#####################################################################################################################
#
# CMake and System Settings
#
#####################################################################################################################

cmake_minimum_required(VERSION 3.24)

if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()

option(SAM_SKIP_TOOLS "Skips the sdktool and tcsconsole builds" OFF)

option(SAM_SKIP_TESTS "Skips building tests" OFF)

option(SAMAPI_EXPORT "Export of ssc binaries to the SAM_api directory; for Unix, compile ssc libraries for SAM_api" ON)

#
# If project isn't system_advisor_model and SAM_SKIP_TOOLS=1,
#   environment vars LK_LIB and LKD_LIB can be used to specify where to find those libraries
#

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12" CACHE STRING "Minimum OS X deployment version")
endif()

if ( NOT APPLE)
    set(CURL_DIR build_resources/libcurl_ssl_x64)
endif()

Project(sam_simulation_core VERSION 1.0.0 LANGUAGES CXX C)


#####################################################################################################################
#
# Compile Options per Platform
#
#####################################################################################################################
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 11)

function(set_no_warnings target)
    get_target_property(MAIN_CFLAGS ${target} COMPILE_FLAGS)
    if (MAIN_CFLAGS MATCHES "NOTFOUND")
        set(MAIN_CFLAGS "")
    endif()
    if(MSVC)
        set(MAIN_CFLAGS "${MAIN_CFLAGS} /W0")
    else()
        set(MAIN_CFLAGS "${MAIN_CFLAGS} -w")
    endif()
    set_target_properties(${target} PROPERTIES COMPILE_FLAGS ${MAIN_CFLAGS})
endfunction()

function(set_disabled_warnings target)
    get_target_property(MAIN_CFLAGS ${target} COMPILE_FLAGS)
    if (MAIN_CFLAGS MATCHES "NOTFOUND")
        set(MAIN_CFLAGS "")
    endif()
    if(MSVC)
        set(MAIN_CFLAGS "${MAIN_CFLAGS} /W4 /w44191 /w44242  /w44266 /w44302 /w44388 /w44826 /w44905 /w44906 /w44928")
    else()
        set(MAIN_CFLAGS "${MAIN_CFLAGS} -Wall -Wno-unknown-pragmas -Wno-deprecated")
        if(APPLE)
            set(MAIN_CFLAGS "${MAIN_CFLAGS} -Wno-unused-private-field -Wno-unused-private-field -Wno-ignored-attributes")
        endif()
    endif()
    set_target_properties(${target} PROPERTIES COMPILE_FLAGS ${MAIN_CFLAGS})
endfunction()

function(set_default_compile_options target)
    get_target_property(MAIN_CFLAGS ${target} COMPILE_FLAGS)
    if (MAIN_CFLAGS MATCHES "NOTFOUND")
        set(MAIN_CFLAGS "")
    endif()
    set(MAIN_CFLAGS "${MAIN_CFLAGS} -D__64BIT__")
    set_property(TARGET ${target} PROPERTY CXX_STANDARD 11)
    set_property(TARGET ${target} PROPERTY CXX_STANDARD_REQUIRED ON)
    if(MSVC)
        set(MAIN_CFLAGS "${MAIN_CFLAGS} /bigobj /MP")
        set(MAIN_CFLAGS "${MAIN_CFLAGS} /D__WINDOWS__ /D_WINDOWS /D_CRT_SECURE_NO_WARNINGS /DLPWINAPP")
        foreach (flag_var CMAKE_C_FLAGS_DEBUG CMAKE_CXX_FLAGS_DEBUG)
            set(${flag_var} "${${flag_var}} /D_DEBUG" CACHE STRING "compile flags" FORCE)
        endforeach()
    else(MSVC)
        if (APPLE)
            set(MAIN_CFLAGS "${MAIN_CFLAGS} -fno-common -DWX_PRECOMP -D__MACOSX__")
        else()
            set(MAIN_CFLAGS "${MAIN_CFLAGS} -D__UNIX__")
        endif()
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(MAIN_CFLAGS "${MAIN_CFLAGS} -D_DEBUG")
# code coverage generation using gcov and lcov - linux only
            if (NOT APPLE AND ENABLE_COVERAGE)
                # set compiler flags
                set(MAIN_CFLAGS " -D__UNIX__ -D__64BIT__ -g -O0 --coverage")
           endif()            
        else()
            set(MAIN_CFLAGS "${MAIN_CFLAGS} -O3 -DNDEBUG")
        endif()
    endif()
    if (SAMAPI_EXPORT AND APPLE)
        SET(MAIN_CFLAGS "${MAIN_CFLAGS} -mmacosx-version-min=12" )
    endif()
	
    set_target_properties(${target} PROPERTIES COMPILE_FLAGS ${MAIN_CFLAGS})
endfunction()

function(set_tools_compile_options target)
    get_target_property(MAIN_CFLAGS ${target} COMPILE_FLAGS)
    if (MAIN_CFLAGS MATCHES "NOTFOUND")
        set(MAIN_CFLAGS "")
    endif()
    if(MSVC)
        set(MAIN_CFLAGS "${MAIN_CFLAGS} /W3 /DLK_USE_WXWIDGETS /D _CRT_SECURE_NO_DEPRECATE=1 /D _CRT_NON_CONFORMING_SWPRINTFS=1")
        set(MAIN_CFLAGS "${MAIN_CFLAGS} /D _SCL_SECURE_NO_WARNINGS=1 /D__WXMSW__ /D_UNICODE /D_NOPCH")
    else(MSVC)
        if (APPLE)
            set(MAIN_CFLAGS "${MAIN_CFLAGS} -D__WXOSX__")
        endif()
        set(MAIN_CFLAGS "${MAIN_CFLAGS} -Wall -Wno-deprecated -Wno-unknown-pragmas -Wno-overloaded-virtual -fno-common -std=c++11")
        set(MAIN_CFLAGS "${MAIN_CFLAGS} -DLK_USE_WXWIDGETS")
    endif(MSVC)
    set_target_properties(${target} PROPERTIES COMPILE_FLAGS ${MAIN_CFLAGS})
endfunction()

function(set_additional_compile_options target options)
    get_target_property(MAIN_CFLAGS ${target} COMPILE_FLAGS)
    if (MAIN_CFLAGS MATCHES "NOTFOUND")
        set(MAIN_CFLAGS "")
    endif()
    set(MAIN_CFLAGS "${MAIN_CFLAGS} ${options}")
    set_target_properties(${target} PROPERTIES COMPILE_FLAGS ${MAIN_CFLAGS})
endfunction()

#####################################################################################################################
#
# Project Settings
#
#####################################################################################################################


add_subdirectory(splinter)
add_subdirectory(shared)
add_subdirectory(nlopt)
add_subdirectory(lpsolve)
add_subdirectory(solarpilot)
add_subdirectory(tcs)
add_subdirectory(ssc)

if (NOT SAM_SKIP_TOOLS)
	add_subdirectory(sdktool)
endif()

if (NOT SAM_SKIP_TESTS)
	add_subdirectory(test)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug"
       AND ENABLE_COVERAGE
       AND NOT APPLE
       AND NOT WIN32)
#    enable_testing()

    # find required tools
    find_program(LCOV lcov REQUIRED)
    find_program(GENHTML genhtml REQUIRED)

    # add coverage target
    add_custom_target(
        coverage
        # gather data
        COMMAND
        ${LCOV} --directory . --capture --exclude '*/rapidjson/*' --exclude '*/splinter/*' --exclude '*/nlopt/*' --exclude '*/lpsolve/*'  --exclude '*/googletest/*' --exclude '*/tcs/water_properties.cpp' --exclude '*/tcs/CO2_properties.cpp'  --exclude '*/test/*' --exclude '/usr/*'  --output-file coverage.info
        # generate report
        COMMAND ${GENHTML} --demangle-cpp -o coverage coverage.info
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
endif()            
