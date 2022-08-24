cmake_minimum_required(VERSION 3.22)

project (mapnik)

set(CMAKE_CXX_STANDARD 14 CACHE STRING "Sets the c++ standard. c++14 is minimum.")
message(STATUS "Using c++${CMAKE_CXX_STANDARD}")
# https://cmake.org/cmake/help/latest/prop_tgt/CXX_STANDARD_REQUIRED.html#prop_tgt:CXX_STANDARD_REQUIRED
set(CXX_STANDARD_REQUIRED ON) # require the specified CMAKE_CXX_STANDARD

find_package(PkgConfig)

pkg_check_modules(Cairo REQUIRED IMPORTED_TARGET cairo)

pkg_check_modules(PROJ REQUIRED IMPORTED_TARGET proj)

find_package(mapnik CONFIG REQUIRED)

include_directories(${CMAKE_JS_INC})

add_library(${PROJECT_NAME} SHARED)

set(SOURCE_FILES src/mapnik_logger.cpp
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
    deps/mapnik-vector-tile/src/vector_tile_compression.cpp
    deps/mapnik-vector-tile/src/vector_tile_datasource_pbf.cpp
    deps/mapnik-vector-tile/src/vector_tile_featureset_pbf.cpp
    deps/mapnik-vector-tile/src/vector_tile_geometry_decoder.cpp
    deps/mapnik-vector-tile/src/vector_tile_geometry_encoder_pbf.cpp
    deps/mapnik-vector-tile/src/vector_tile_layer.cpp
    deps/mapnik-vector-tile/src/vector_tile_processor.cpp
    deps/mapnik-vector-tile/src/vector_tile_raster_clipper.cpp
    deps/mapnik-vector-tile/src/vector_tile_tile.cpp
    )

target_sources(${PROJECT_NAME} PRIVATE ${SOURCE_FILES} ${CMAKE_JS_SRC})


set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")


target_compile_definitions(${PROJECT_NAME} PUBLIC MAPNIK_GIT_REVISION=123456789 MAPNIK_VECTOR_TILE_LIBRARY=1)


target_include_directories(${PROJECT_NAME} PRIVATE
  "${MAPNIK_INCLUDE_DIR}"
  "${CMAKE_SOURCE_DIR}/deps/geometry/include"
  "${CMAKE_SOURCE_DIR}/deps/protozero/include"
  "${CMAKE_SOURCE_DIR}/deps/wagyu/include"
  "${CMAKE_SOURCE_DIR}/deps/mapnik-vector-tile/src/"
  "${CMAKE_SOURCE_DIR}/node_modules/node-addon-api")

target_link_libraries(${PROJECT_NAME}
  ${CMAKE_JS_LIB}
  mapnik::core
  mapnik::mapnik
  mapnik::json
  mapnik::wkt
  )
