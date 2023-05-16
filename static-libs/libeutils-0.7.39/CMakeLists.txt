cmake_minimum_required(VERSION 2.6)
project( libeutils )

include( CMakeGenericSystem )
include( FindPkgConfig )
include( FindThreads )
include( CheckIncludeFile )
include( FeatureSummary )
include( CMakeDetermineSystem )
include( CMakeDetermineRCCompiler )
include( CMakeDetermineCXXCompiler )
include( CMakeDetermineCCompiler )
include( CMakeBackwardCompatibilityCXX )
include( FindTCL )

if( NOT VERSION )
	set( VERSION 0.0.1 )
endif()

# The libeutils version
SET(libeutils_VERSION_MAJOR "1")
SET(libeutils_VERSION_MINOR "0")
SET(libeutils_VERSION_PATCH "0")
SET(libeutils_VERSION "${libeutils_VERSION_MAJOR}.${libeutils_VERSION_MINOR}")
SET(libeutils_VERSION_FULL "${libeutils_VERSION}.${libeutils_VERSION_PATCH}")
SET(libeutils_INCLUDE_DIR ${CMAKE_INSTALL_PREFIX}/include/libeutils)

if( NOT TCL_FOUND )
	message( FATAL_ERROR "tcl was not found" )
endif()

check_include_file( expect.h HAVE_EXPECT_H )

if( NOT HAVE_EXPECT_H )
	message( FATAL_ERROR "expect was not found" )
endif( NOT HAVE_EXPECT_H )

check_include_file( popt.h HAVE_POPT_H )

if( NOT HAVE_POPT_H )
	message( FATAL_ERROR "popt was not found" )
endif( NOT HAVE_POPT_H )

pkg_check_modules ( SIGC++ sigc++-2.0>=2.0 )
pkg_check_modules ( GLIB glib-2.0>=2.0 )
pkg_check_modules( TUT libtut )

option( BUILD_SHARED_LIBRARIES "Build shared library" on )
option( BUILD_STATIC_LIBRARIES "Build static library" off )

configure_file (${CMAKE_CURRENT_SOURCE_DIR}/libeutils.pc.cmake ${CMAKE_CURRENT_BINARY_DIR}/libeutils.pc @ONLY)
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h @ONLY)
add_definitions( -DHAVE_CONFIG_H )

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libeutils.pc DESTINATION lib/pkgconfig)

enable_testing()


include_directories( . )

add_subdirectory(libeutils)
add_subdirectory(test)
