# Created by the script cgal_create_cmake_script
# This is the CMake script for compiling a CGAL application.


cmake_minimum_required(VERSION 3.1...3.15)
project( Periodic_2_triangulation_2_Tests )


find_package(CGAL QUIET)

if ( CGAL_FOUND )

  include_directories (BEFORE "include")

  # create a target per cppfile
  file(GLOB cppfiles RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
  foreach(cppfile ${cppfiles})
    create_single_source_cgal_program( "${cppfile}" )
  endforeach()

else()

    message(STATUS "This program requires the CGAL library, and will not be compiled.")

endif()

