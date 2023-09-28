# Plugins to build

# Dependencies
set ( CMAKE_CXX_STANDARD 17 ) 
set ( CMAKE_CXX_STANDARD_REQUIRED ON )
set ( CMAKE_CXX17_STANDARD_COMPILE_OPTION "-std=c++17" )
set ( CMAKE_CXX17_EXTENSION_COMPILE_OPTION "-std=c++17" )
set ( CMAKE_CXX_EXTENSIONS ON )

set( CONFIGS
    temp_geom_reader.cfg
)

set( SOURCES
    temp_geom_reader.cpp
)

set( HEADERS
    temp_geom_reader.h
)

include( plugins )
uda_plugin(
    NAME TEMP_GEOM
    ENTRY_FUNC TempGeomReader
    DESCRIPTION "Temporary plugin to read MAST-U GEOM data"
    EXAMPLE "TEMP_GEOM::get()"
    LIBNAME temp_geom_reader
    SOURCES temp_geom_reader.cpp
    CONFIG_FILE ${CONFIGS}
    EXTRA_INCLUDE_DIRS
      ${UDA_CLIENT_INCLUDE_DIRS}
      ${Boost_INCLUDE_DIRS}
    EXTRA_LINK_DIRS
      ${UDA_CLIENT_LIBRARY_DIRS}
      ${Boost_LIBRARY_DIRS}
    EXTRA_LINK_LIBS
      ${UDA_CLIENT_LIBRARIES}
      ${Boost_LIBRARIES}
      uda_cpp
)

list( TRANSFORM SOURCES PREPEND ${CMAKE_CURRENT_LIST_DIR}/ )
list( TRANSFORM HEADERS PREPEND ${CMAKE_CURRENT_LIST_DIR}/ )

set_property( TARGET clang-format APPEND PROPERTY SOURCES ${SOURCES} ${HEADERS} )
