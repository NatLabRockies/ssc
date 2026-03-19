#####################################################################################################################
#
# Project Settings
#
#####################################################################################################################

include_directories(. $ENV{WEXDIR}/include $ENV{LKDIR}/include ../ssc ../shared ../splinter $ENV{SSCDIR})

set(SDKTOOL_SRC
	dataview.cpp
	dllinvoke.cpp
	editvariable.cpp
	scripting.cpp
	sscdev.cpp)


#####################################################################################################################
#
# Packages
#
#####################################################################################################################

if (UNIX)
	set(wxWidgets_CONFIG_EXECUTABLE /usr/local/bin/wx-config-3)
	find_package(wxWidgets QUIET  COMPONENTS qa aui richtext html propgrid adv net stc core xml base REQUIRED)
else()
#	set(wxWidgets_ROOT_DIR $ENV{WXMSW3})
#	find_package(wxWidgets  QUIET COMPONENTS qa webview aui richtext html propgrid adv net stc core xml base scintilla REQUIRED)

    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Debug and Release Builds Configured" FORCE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++20")
    # for linking to Release build of ortools 
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
    # Get the current debug flags and remove the /D_DEBUG option.
    string(REPLACE "/D_DEBUG" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    # following does not force release libraries for wxWidgets using find_package
    #set(CMAKE_MAP_IMPORTED_CONFIG_DEBUG RELEASE)

    set(wxWidgets_ROOT_DIR $ENV{WXMSW3})
    find_package(wxWidgets QUIET  REQUIRED xrc webview stc richtext ribbon propgrid aui gl html qa adv core xml net base scintilla)

 #   message("All wxWidgets: " ${wxWidgets_LIBRARIES})
    set(FOUND "false")
    foreach(lib ${wxWidgets_LIBRARIES})
        string(FIND ${lib} "optimized" INDEX)
        string(FIND ${lib} "debug" INDEX2)
        string(FIND ${lib} "wxWidgets" INDEX3)
        if (INDEX GREATER_EQUAL 0) # skip once
            set(FOUND "true")
            continue()
        elseif((INDEX2 LESS 0) AND (INDEX3 LESS 0))
            set(FOUND "true")
        endif()
        if (${FOUND} STREQUAL "true")
            set(FOUND "false")
            message("lib: ${lib}")
            list(APPEND WX_LIBS_LIST ${lib})
        endif()
    endforeach()

    list(JOIN WX_LIBS_LIST ";" wxWidgets_LIBRARIES)
#    message("Link Libraries for wxWidgets: ${wxWidgets_LIBRARIES}")


endif()

if (wxWidgets_FOUND)
    include(${wxWidgets_USE_FILE})
endif ()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${wxWidgets_CXX_FLAGS}")




#####################################################################################################################
#
# CMake Targets
#
#####################################################################################################################

# Setup Application
if (APPLE)
	set(MACOSX_BUNDLE_ICON_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../build_resources/SDKTool.icns)
	set(SDKTOOL_ICON ${CMAKE_CURRENT_SOURCE_DIR}/../build_resources/SDKTool.icns)
	set_source_files_properties(${SDKTOOL_ICON} PROPERTIES
		MACOSX_PACKAGE_LOCATION "Resources")
elseif(WIN32)
	set(SDKTOOL_ICON ${CMAKE_CURRENT_SOURCE_DIR}/../build_resources/app.rc)
endif()

# Setup Target
add_executable(SDKtool
		WIN32
		MACOSX_BUNDLE
		${SDKTOOL_SRC}
		${SDKTOOL_ICON})
set_target_properties(SDKtool
    PROPERTIES
    DEBUG_POSTFIX "d"
	PREFIX ""
	)

set_default_compile_options(SDKtool)
set_tools_compile_options(SDKtool)

if (APPLE)
	set_target_properties(SDKtool PROPERTIES
		MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../build_resources/Info-SDKTool.plist
	)
endif()

if (MSVC)
	set_target_properties(SDKtool PROPERTIES
        LINK_FLAGS /SUBSYSTEM:WINDOWS)
endif()

#####################################################################################################################
#
# Link Libraries and Options
#
#####################################################################################################################

target_link_libraries(SDKtool ssc)

if (${CMAKE_PROJECT_NAME} STREQUAL system_advisor_model)
	target_link_libraries(SDKtool wex lk)
else()
    if (CMAKE_BUILD_TYPE STREQUAL "Release" OR "Release" IN_LIST CMAKE_CONFIGURATION_TYPES)
        unset(WEX_LIB CACHE)
        unset(LK_LIB CACHE)
        find_library( WEX_LIB
                NAMES wex.a wex.lib
                PATHS $ENV{WEX_LIB} $ENV{WEXDIR}/build $ENV{WEXDIR}/build/Release $ENV{CMAKEBUILDDIR}/wex/Release)
        find_library( LK_LIB
                NAMES lk.a lk.lib
                PATHS $ENV{LK_LIB} $ENV{LKDIR}/build $ENV{LKDIR}/build/Release $ENV{CMAKEBUILDDIR}/lk/Release)
	    target_link_libraries(SDKtool optimized ${WEX_LIB} optimized ${LK_LIB})
    endif()

	if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR "Debug" IN_LIST CMAKE_CONFIGURATION_TYPES)
		unset(LKD_LIB CACHE)
		unset(WEXD_LIB CACHE)
		find_library( WEXD_LIB
				NAMES wexd.a wexd.lib
				PATHS $ENV{WEXD_LIB} $ENV{WEXDIR}/build $ENV{WEXDIR}/build/Debug $ENV{CMAKEBUILDDIR}/wex/Debug)
		find_library( LKD_LIB
				NAMES lkd.a lkd.lib
				PATHS $ENV{LKD_LIB} $ENV{LKDIR}/build $ENV{LKDIR}/build/Debug $ENV{CMAKEBUILDDIR}/lk/Debug)
		target_link_libraries(SDKtool debug ${WEXD_LIB} debug ${LKD_LIB})
	endif()
endif()

target_link_libraries(SDKtool ${wxWidgets_LIBRARIES})

if (UNIX AND NOT APPLE)
	target_link_libraries(SDKtool ${CMAKE_DL_LIBS} -lfontconfig)
endif()

if (UNIX)
	find_library(MATH_LIBRARY m)
	if(MATH_LIBRARY)
		target_link_libraries(SDKtool ${MATH_LIBRARY})
	endif()
	target_link_libraries(SDKtool -lcurl)
elseif (MSVC)
	find_library( CURL_LIB
			NAMES libcurl.lib
			PATHS $ENV{WEXDIR}/build_resources/libcurl_ssl_x64/lib)
	target_link_libraries(SDKtool ${CURL_LIB})
	target_link_libraries(SDKtool Winhttp)
endif()


#####################################################################################################################
#
# Target Installation
#
#####################################################################################################################

if (MSVC)
	set( LIBCURL_FILES
	   dbghelp.dll
	   libcurl.dll
	   libeay32.dll
	   libssh2.dll
	   msvcp140.dll
	   openssl.exe
	   ssleay32.dll
	   vcruntime140.dll
	)

	set(SDKTOOL_DIR ${CMAKE_CURRENT_BINARY_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>)

	foreach( file_i ${LIBCURL_FILES})
		add_custom_command(
		TARGET SDKtool
		POST_BUILD
		COMMAND ${CMAKE_COMMAND}
		ARGS -E copy $ENV{WEXDIR}/build_resources/libcurl_ssl_x64/bin/${file_i} ${SDKTOOL_DIR}
	)
	endforeach( file_i )
endif()
