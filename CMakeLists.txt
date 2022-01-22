cmake_minimum_required(VERSION 3.15)
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
project(node-mapnik)

set(MAPBOX_WAGYU_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/deps/wagyu/include" CACHE INTERNAL "wagyu include dir")
set(BUILD_TESTING OFF)
set(MAPNIK_VECTOR_TILE_ENABLE_INSTALL OFF)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# RPATH handling for *nix systems. As we copy mapnik.so into the lib/binding/napi-v* path we need to adjust the RPATH accordingly for node-mapnik
# use, i.e. don't skip the full RPATH for the build tree
set(CMAKE_SKIP_BUILD_RPATH FALSE)
# when building, don't use the install RPATH already
# (but later on when installing)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "./lib/binding/napi-v3")
# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

find_package(Threads REQUIRED)
find_package(PkgConfig)
pkg_check_modules(Cairo REQUIRED IMPORTED_TARGET cairo)
pkg_check_modules(PROJ REQUIRED IMPORTED_TARGET proj)
find_package(Protobuf REQUIRED)
find_package(mapnik CONFIG REQUIRED COMPONENTS shapeindex mapnik-index)

include(FetchContent)
FetchContent_Declare(
  napi_modules
  GIT_REPOSITORY https://github.com/mathisloge/cmake-napi.git
  GIT_TAG        cdc5203c12f7e44ccb260b315256590d54b574af
)
FetchContent_Declare(
  mapnik_vector_tile
  GIT_REPOSITORY https://github.com/mathisloge/mapnik-vector-tile.git
  GIT_TAG        1d2198354925a23b05bc96ed11f14629691b9721
  GIT_SUBMODULES "src" # https://gitlab.kitware.com/cmake/cmake/-/issues/20579
)
FetchContent_MakeAvailable(mapnik_vector_tile napi_modules)
FetchContent_GetProperties(napi_modules SOURCE_DIR napi_src)
list(APPEND CMAKE_MODULE_PATH "${napi_src}/modules")
include(napi-gyp)


execute_process(COMMAND node -p "require('node-addon-api').include"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE NODE_ADDON_API_DIR
)
string(REPLACE "\n" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})

add_subdirectory(deps)
mapnik_find_plugin_dir(MAPNIK_PLUGINS_DIR)
message(STATUS "Using plugins from ${MAPNIK_PLUGINS_DIR}")

add_library(node-mapnik MODULE
    src/mapnik_logger.cpp
    src/node_mapnik.cpp
    src/blend.cpp
    src/mapnik_map.cpp
    src/mapnik_map_load.cpp
    src/mapnik_map_from_string.cpp
    src/mapnik_map_render.cpp
    src/mapnik_map_query_point.cpp
    src/mapnik_color.cpp
    src/mapnik_geometry.cpp
    src/mapnik_feature.cpp
    src/mapnik_image.cpp
    src/mapnik_image_encode.cpp
    src/mapnik_image_open.cpp
    src/mapnik_image_fill.cpp
    src/mapnik_image_save.cpp
    src/mapnik_image_from_bytes.cpp
    src/mapnik_image_from_svg.cpp
    src/mapnik_image_solid.cpp
    src/mapnik_image_multiply.cpp
    src/mapnik_image_clear.cpp
    src/mapnik_image_copy.cpp
    src/mapnik_image_resize.cpp
    src/mapnik_image_compositing.cpp
    src/mapnik_image_filter.cpp
    src/mapnik_image_view.cpp
    src/mapnik_grid.cpp
    src/mapnik_grid_view.cpp
    src/mapnik_palette.cpp
    src/mapnik_projection.cpp
    src/mapnik_layer.cpp
    src/mapnik_datasource.cpp
    src/mapnik_featureset.cpp
    src/mapnik_expression.cpp
    src/mapnik_cairo_surface.cpp
    src/mapnik_vector_tile.cpp
    src/mapnik_vector_tile_data.cpp
    src/mapnik_vector_tile_query.cpp
    src/mapnik_vector_tile_json.cpp
    src/mapnik_vector_tile_info.cpp
    src/mapnik_vector_tile_simple_valid.cpp
    src/mapnik_vector_tile_render.cpp
    src/mapnik_vector_tile_clear.cpp
    src/mapnik_vector_tile_image.cpp
    src/mapnik_vector_tile_composite.cpp
)
set_target_properties(node-mapnik PROPERTIES PREFIX "" OUTPUT_NAME "mapnik" SUFFIX ".node")
target_include_directories(node-mapnik PRIVATE 
    ${NODE_ADDON_API_DIR}
)

target_compile_definitions(node-mapnik PRIVATE 
    _USE_MATH_DEFINES 
    MAPNIK_GIT_REVISION=1 
)

if(MSVC)
    target_compile_definitions(node-mapnik PRIVATE /wd4068)
endif()

target_link_libraries(node-mapnik 
    mapnik::core
    mapnik::json
    mapnik::wkt
    mapnik::mapnik
    mapbox-geometry
    mapbox-wagyu
    mapbox-protozero
    node::napi
    mapnik::mapnik-vector-tile
)

# post build
file(GENERATE 
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>_copy.cmake" 
    INPUT "${CMAKE_CURRENT_SOURCE_DIR}/cmake/post_build.cmake"
)

# installation
macro(install_variable var)
    install(CODE "set(${var} \"${${var}}\")")
endmacro()

install(TARGETS node-mapnik 
    RUNTIME_DEPENDENCY_SET node-mapnik-deps
    RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_PREFIX}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}"
)
install_variable(MAPNIK_FONTS_DIR)
install_variable(MAPNIK_PLUGINS_DIR)
install_variable(VCPKG_INSTALLED_DIR)
install_variable(VCPKG_TARGET_TRIPLET)
install(CODE "set(SOURCE_DIR \"${CMAKE_CURRENT_SOURCE_DIR}\")")
install(SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>_copy.cmake)
