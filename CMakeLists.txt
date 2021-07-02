cmake_minimum_required(VERSION 3.15)
project(node-mapnik)
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
endif()

execute_process(COMMAND node -p "require('node-addon-api').include"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE NODE_ADDON_API_DIR
)
string(REPLACE "\n" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})

find_path(MAPBOX_GEOMETRY_INCLUDE_DIRS "mapbox/geometry.hpp" REQUIRED)
find_path(MAPBOX_PROTOZERO_INCLUDE_DIRS "protozero/pbf_message.hpp")
find_package(mapnik CONFIG REQUIRED)

add_library(node-mapnik SHARED
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
    node_modules/mapnik-vector-tile/src/vector_tile_compression.cpp
    node_modules/mapnik-vector-tile/src/vector_tile_datasource_pbf.cpp
    node_modules/mapnik-vector-tile/src/vector_tile_featureset_pbf.cpp
    node_modules/mapnik-vector-tile/src/vector_tile_geometry_decoder.cpp
    node_modules/mapnik-vector-tile/src/vector_tile_geometry_encoder_pbf.cpp
    node_modules/mapnik-vector-tile/src/vector_tile_layer.cpp
    node_modules/mapnik-vector-tile/src/vector_tile_processor.cpp
    node_modules/mapnik-vector-tile/src/vector_tile_raster_clipper.cpp
    node_modules/mapnik-vector-tile/src/vector_tile_tile.cpp
    ${CMAKE_JS_SRC}
)
set_target_properties(node-mapnik PROPERTIES PREFIX "" SUFFIX ".node")
target_include_directories(node-mapnik PRIVATE 
    ${CMAKE_JS_INC} 
    ${NODE_ADDON_API_DIR}
    ${MAPBOX_GEOMETRY_INCLUDE_DIRS}
    ${MAPBOX_PROTOZERO_INCLUDE_DIRS}
    node_modules/mapnik-vector-tile/src
)

target_compile_definitions(node-mapnik PRIVATE 
    _USE_MATH_DEFINES 
    MAPNIK_GIT_REVISION=1 
    MAPNIK_VECTOR_TILE_LIBRARY=1
    NAPI_VERSION=3
)

target_link_libraries(node-mapnik 
    mapnik::core
    mapnik::json
    mapnik::wkt
    mapnik::mapnik
    ${CMAKE_JS_LIB}
)
