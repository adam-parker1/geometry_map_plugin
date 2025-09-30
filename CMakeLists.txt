cmake_minimum_required( VERSION 3.15 )

project(
    MastuGeomPlugin
    VERSION 0.1.0.0
    LANGUAGES CXX
)

list(
    APPEND CMAKE_MODULE_PATH
    ${CMAKE_SOURCE_DIR}/cmake
    ${CMAKE_SOURCE_DIR}/cmake/Modules
)

set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_EXTENSIONS OFF )

# Dependencies
find_package( UDA 2.7.0 REQUIRED )
find_package( Boost REQUIRED )

include( plugins )
uda_plugin(
    NAME GEOMETRY
    ENTRY_FUNC GeometryMapReader
    DESCRIPTION "Temporary plugin to read MAST-U GEOM data"
    EXAMPLE "GEOMETRY::get()"
    LIBNAME geometry_map_reader
    SOURCES geometry_map_reader.cpp
    CONFIG_FILE geometry_map_reader.cfg
    EXTRA_INCLUDE_DIRS
      ${UDA_CLIENT_INCLUDE_DIRS}
      ext_include
    EXTRA_LINK_DIRS
      ${UDA_CLIENT_LIBRARY_DIRS}
    EXTRA_LINK_LIBS
      ${UDA_CLIENT_LIBRARIES}
      Boost::boost
      uda_cpp
)

# Generate scripts
list( GET UDA_DIR 0 UDA_HOME )
string( REGEX REPLACE ";" " " PLUGINS "${BUILT_PLUGINS}" )
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/scripts/activate-plugins.sh.in
    ${CMAKE_BINARY_DIR}/scripts/activate-plugins.sh
    @ONLY
)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/scripts/remove-plugins.sh.in
    ${CMAKE_BINARY_DIR}/scripts/remove-plugins.sh
    @ONLY
)
