#####################################################################################################################
#
# Project Settings
#
#####################################################################################################################

include_directories(. include src $ENV{ORTOOLSDIR}/include/eigen3 ../ssc)

set(SPLINTER_SRC
	src/bspline.cpp
	src/bspline_basis.cpp
	src/bspline_basis_1d.cpp
	src/bspline_builders.cpp
	src/bspline_utils.cpp
	src/data_point.cpp
	src/data_table.cpp
	src/function.cpp
	src/json_parser.cpp
	src/knot_vector.cpp
	src/knot_builders.cpp
	src/kronecker_product.cpp
	src/utilities.cpp)


#####################################################################################################################
#
# CMake Targets
#
#####################################################################################################################

add_library(splinter STATIC ${SPLINTER_SRC})
set_target_properties(splinter
    PROPERTIES
    DEBUG_POSTFIX "d"
	PREFIX ""
	)

set_default_compile_options(splinter)
if(MSVC)
    set_additional_compile_options(splinter "/W2 /wd4267 /wd4244")
endif(MSVC)
set_no_warnings(splinter)


#####################################################################################################################
#
# Link Libraries and Options
#
#####################################################################################################################

if (MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
endif()


