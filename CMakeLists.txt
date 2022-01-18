cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")

project(node-mapnik)

set(MAPBOX_WAGYU_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/deps/wagyu/include" CACHE INTERNAL "wagyu include dir")

set(BUILD_TESTING OFF)

find_package(Threads REQUIRED)
find_package(PkgConfig)
pkg_check_modules(Cairo REQUIRED IMPORTED_TARGET cairo)
pkg_check_modules(PROJ REQUIRED IMPORTED_TARGET proj)
find_package(Protobuf REQUIRED)

include(FetchContent)
FetchContent_Declare(
  napi_modules
  GIT_REPOSITORY https://github.com/mathisloge/cmake-napi.git
  GIT_TAG        cdc5203c12f7e44ccb260b315256590d54b574af
)
FetchContent_Declare(
  mapnik_vector_tile
  GIT_REPOSITORY https://github.com/mathisloge/mapnik-vector-tile.git
  GIT_TAG        1f5b0d056d3403204f93fdfaf1f1cb6842090897
  GIT_SUBMODULES "src" # https://gitlab.kitware.com/cmake/cmake/-/issues/20579
)
FetchContent_MakeAvailable(mapnik_vector_tile napi_modules)
FetchContent_GetProperties(napi_modules SOURCE_DIR napi_src)
list(APPEND CMAKE_MODULE_PATH "${napi_src}/modules")
include(napi-gyp)

find_package(mapnik CONFIG REQUIRED COMPONENTS shapeindex mapnik-index)

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
set_target_properties(node-mapnik PROPERTIES PREFIX "" SUFFIX ".node")
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
set(NODE_MAPNIK_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(MODULE_PATH "${NODE_MAPNIK_LIB_DIR}/binding/napi-v${NAPI_VERSION}")
set(MODULE_FILE_PATH "${MODULE_PATH}/mapnik.node")
add_custom_command(TARGET node-mapnik POST_BUILD 
    COMMAND ${CMAKE_COMMAND} -E rm -r "${MODULE_PATH}/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:node-mapnik>" "${MODULE_FILE_PATH}"
)

file(GENERATE 
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>_copy.cmake" 
    INPUT "${CMAKE_CURRENT_SOURCE_DIR}/cmake/post_build.cmake"
)
add_custom_command(TARGET node-mapnik POST_BUILD
    COMMAND ${CMAKE_COMMAND} 
        -DMODULE_FILE_PATH=${MODULE_FILE_PATH}
        -DMODULE_PATH=${MODULE_PATH} 
        -DMAPNIK_FONTS_DIR=${MAPNIK_FONTS_DIR} 
        -DMAPNIK_PLUGINS_DIR=${MAPNIK_PLUGINS_DIR}
        -DVCPKG_INSTALLED_DIR=${VCPKG_INSTALLED_DIR}
        -DVCPKG_TARGET_TRIPLET=${VCPKG_TARGET_TRIPLET}
        -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
        -P ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>_copy.cmake
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
