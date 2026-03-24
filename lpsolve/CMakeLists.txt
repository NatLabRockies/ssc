#####################################################################################################################
#
# Project Settings
#
#####################################################################################################################

include_directories(.)

set(LPSOLVE_SRC
	colamd.cpp
	commonlib.cpp
	fortify.cpp
	ini.cpp
	lp_crash.cpp
	lp_Hash.cpp
	lp_lib.cpp
	lp_LUSOL.cpp
	lp_matrix.cpp
	lp_MDO.cpp
	lp_mipbb.cpp
	lp_MPS.cpp
	lp_params.cpp
	lp_presolve.cpp
	lp_price.cpp
	lp_pricePSE.cpp
	lp_report.cpp
	lp_rlp.cpp
	lp_scale.cpp
	lp_simplex.cpp
	lp_solve.cpp
	lp_SOS.cpp
	lp_utils.cpp
	lp_wlp.cpp
	lusol.cpp
	mmio.cpp
	myblas.cpp
	yacc_read.cpp)


####################################################################################################################
#
# CMake Targets
#
#####################################################################################################################

add_library(lpsolve STATIC ${LPSOLVE_SRC})
set_target_properties(lpsolve
    PROPERTIES
    DEBUG_POSTFIX "d"
	PREFIX ""
	)
if (MSVC)
	set_target_properties(lpsolve
		PROPERTIES
		LINK_FLAGS /SUBSYSTEM:CONSOLE)
endif()

set_default_compile_options(lpsolve)
set_no_warnings(lpsolve)

set(ADD_CFLAGS "")
if(MSVC)
    set(ADD_CFLAGS "${ADD_CFLAGS} /D_CONSOLE /DMSDOS /DCHECK_SOLUTION /D_SCL_SECURE_NO_DEPRECATE")
    set(ADD_CFLAGS "${ADD_CFLAGS} /DYY_NEVER_INTERACTIVE /DPARSER_LP /DINVERSE_ACTIVE=INVERSE_LUSOL")
    set(ADD_CFLAGS "${ADD_CFLAGS} /DRoleIsExternalInvEngine /D_CRT_SECURE_NO_DEPRECATE /D_CRT_NONSTDC_NO_DEPRECATE")
    set(ADD_CFLAGS "${ADD_CFLAGS} /DFPUexception /DNOFORTIFY /D LP_MAXLINELEN=0")
else()
    set(ADD_CFLAGS "${ADD_CFLAGS} -Wall -DWX_PRECOMP -DCHECK_SOLUTION -DYY_NEVER_INTERACTIVE -DPARSER_LP -DINVERSE_ACTIVE=INVERSE_LUSOL -DRoleIsExternalInvEngine")
endif()
set_additional_compile_options(lpsolve ${ADD_CFLAGS})


#####################################################################################################################
#
# Link Libraries and Options
#
#####################################################################################################################

if (MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
endif()

