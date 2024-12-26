#####################################################################################################################
#
# CMake and System Settings
#
#####################################################################################################################

set(CMAKE_VERBOSE_MAKEFILE ON)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12" CACHE STRING "Minimum OS X deployment version")
endif()


#####################################################################################################################
#
# Project Settings 
#
#####################################################################################################################

Project(solarpilot_core)

include_directories(. ../nlopt ../lpsolve .. $ENV{CORETRACEDIR})

set(SOLARPILOT_SRC
	Ambient.cpp                Ambient.h
	API_structures.cpp         API_structures.h
	AutoPilot_API.cpp          AutoPilot_API.h
	definitions.cpp            definitions.h
	exceptions.hpp
	Financial.cpp              Financial.h
	Flux.cpp                   Flux.h
	fluxsim.cpp                fluxsim.h
	heliodata.cpp              heliodata.h
	Heliostat.cpp              Heliostat.h
	interop.cpp                interop.h
	IOUtil.cpp                 IOUtil.h
	Land.cpp                   Land.h
	LayoutSimulateThread.cpp   LayoutSimulateThread.h
	mod_base.cpp               mod_base.h
	MultiRecOptimize.cpp       MultiRecOptimize.h
	OpticalMesh.cpp            OpticalMesh.h
	optimize.cpp               optimize.h
	Receiver.cpp               Receiver.h
	SolarField.cpp             SolarField.h
	solpos.cpp                 solpos00.h
	sort_method.h
	STObject.cpp               STObject.h
	string_util.cpp            string_util.h
	STSimulateThread.cpp       STSimulateThread.h
	Toolbox.cpp                Toolbox.h)



#####################################################################################################################
#
# Compile Options per Platform
#
#####################################################################################################################

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
	add_compile_options(/MP /W0)
    add_compile_definitions( __64BIT__ __WINDOWS__ _WINDOWS _CRT_SECURE_NO_WARNINGS LPWINAPP )
    foreach (flag_var CMAKE_C_FLAGS_DEBUG CMAKE_CXX_FLAGS_DEBUG)
		set(${flag_var} "${${flag_var}} /D_DEBUG" CACHE STRING "compile flags" FORCE)
	endforeach()
else()
	if (APPLE)
		add_compile_options( -fno-common)
  		add_definitions(-DWX_PRECOMP)
	else()
		add_definitions(-D__64BIT__)
	endif()
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11" )
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-unknown-pragmas -Wno-sign-compare -Wno-unused-variable -Wno-uninitialized -Wno-int-to-pointer-cast ")
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")
		add_compile_definitions(_DEBUG)
	else()
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG" )
	endif()
endif()


#####################################################################################################################
# 
# CMake Targets
#
#####################################################################################################################



if(NOT BUILD_COPILOT_API)
	add_library(solarpilot_core STATIC ${SOLARPILOT_SRC})
	set_target_properties(solarpilot_core
		PROPERTIES
		DEBUG_POSTFIX "d"
		PREFIX ""
		)
else()
	add_library(solarpilot SHARED ${SOLARPILOT_SRC})
	set_target_properties(solarpilot
		PROPERTIES
		DEBUG_POSTFIX "d"
		PREFIX ""
		)
endif()

#####################################################################################################################
#
# Link Libraries and Options
#
#####################################################################################################################

if (MSVC)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
else()
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:LINUX")
endif()
	
if(BUILD_COPILOT_API)
	target_link_libraries(solarpilot lpsolve nlopt shared coretrace)
	if (MSVC)		
		set(COPILOT_DLL $<TARGET_FILE_DIR:solarpilot>/$<TARGET_FILE_BASE_NAME:solarpilot>.dll)
	else()
		set(COPILOT_DLL $<TARGET_FILE_DIR:solarpilot>/$<TARGET_FILE_BASE_NAME:solarpilot>.so)
	endif()
	

	if(EXISTS ${CMAKE_SOURCE_DIR}/SolarPILOT/deploy/api)
		set(DLL_COPY_PATH ${CMAKE_SOURCE_DIR}/SolarPILOT/deploy/api)	
	else()
		set(DLL_COPY_PATH ${CMAKE_SOURCE_DIR})
	endif()

	add_custom_command(
		TARGET solarpilot
		POST_BUILD
		COMMAND ${CMAKE_COMMAND}
		ARGS -E copy $<TARGET_FILE:solarpilot> ${COPILOT_DLL} ${DLL_COPY_PATH}
	)
		
endif()

