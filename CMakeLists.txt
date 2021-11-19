cmake_minimum_required(VERSION 3.15)
if(WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
endif()
project(node-mapnik)
include(FetchContent)

FetchContent_Declare(
  napi_modules
  GIT_REPOSITORY https://github.com/mathisloge/cmake-napi.git
  GIT_TAG        068e325e98490cb5aad6058747e5cbcb1ddab52d
)
FetchContent_MakeAvailable(napi_modules)
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
find_package(Threads REQUIRED)
find_package(PkgConfig)
pkg_check_modules(Cairo REQUIRED IMPORTED_TARGET cairo)
pkg_check_modules(PROJ REQUIRED IMPORTED_TARGET proj)
find_package(mapnik CONFIG REQUIRED COMPONENTS shapeindex mapnik-index)
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
    node_modules/mapnik-vector-tile/src/vector_tile_compression.cpp
    node_modules/mapnik-vector-tile/src/vector_tile_datasource_pbf.cpp
    node_modules/mapnik-vector-tile/src/vector_tile_featureset_pbf.cpp
    node_modules/mapnik-vector-tile/src/vector_tile_geometry_decoder.cpp
    node_modules/mapnik-vector-tile/src/vector_tile_geometry_encoder_pbf.cpp
    node_modules/mapnik-vector-tile/src/vector_tile_layer.cpp
    node_modules/mapnik-vector-tile/src/vector_tile_processor.cpp
    node_modules/mapnik-vector-tile/src/vector_tile_raster_clipper.cpp
    node_modules/mapnik-vector-tile/src/vector_tile_tile.cpp
)
set_target_properties(node-mapnik PROPERTIES PREFIX "" SUFFIX ".node")
target_include_directories(node-mapnik PRIVATE 
    ${NODE_ADDON_API_DIR}
    node_modules/mapnik-vector-tile/src
)

target_compile_definitions(node-mapnik PRIVATE 
    _USE_MATH_DEFINES 
    MAPNIK_GIT_REVISION=1 
    MAPNIK_VECTOR_TILE_LIBRARY=1
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
)

# post build
set(NODE_MAPNIK_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(MODULE_PATH "${NODE_MAPNIK_LIB_DIR}/binding/napi-v${NAPI_VERSION}")
set(MODULE_FILE_PATH "${MODULE_PATH}/mapnik.node")
add_custom_command(TARGET node-mapnik POST_BUILD 
    COMMAND ${CMAKE_COMMAND} -E rm -r "${MODULE_PATH}/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:node-mapnik>" "${MODULE_FILE_PATH}"
)

file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>_copy.cmake CONTENT [[
message(STATUS "creating mapnik_settings.js")
set(MODULE_BIN_PATH "${MODULE_PATH}/bin")
set(MODULE_SHARE_PATH "${MODULE_PATH}/share")
set(MODULE_FONTS_DIR "${MODULE_PATH}/mapnik/fonts")
set(MODULE_PLUGINS_DIR "${MODULE_PATH}/mapnik/input")
set(MODULE_SETTINGS_PATH "${MODULE_PATH}")
set(MODULE_PROJ_PATH "${MODULE_SHARE_PATH}/proj")

file(COPY "${MAPNIK_FONTS_DIR}/" DESTINATION "${MODULE_FONTS_DIR}/")
file(COPY "${MAPNIK_PLUGINS_DIR}/" DESTINATION "${MODULE_PLUGINS_DIR}/" PATTERN "*.input")

find_program(PATH_MAPNIK_INDEX mapnik-index HINTS $<TARGET_FILE_DIR:mapnik::mapnik-index> REQUIRED)
find_program(PATH_SHAPEINDEX shapeindex HINTS $<TARGET_FILE_DIR:mapnik::shapeindex> REQUIRED)

file(COPY "${PATH_MAPNIK_INDEX}" DESTINATION "${MODULE_BIN_PATH}/")
file(COPY "${PATH_SHAPEINDEX}" DESTINATION "${MODULE_BIN_PATH}/")

# write settings file
# determine relative paths to the settings file
file(RELATIVE_PATH MODULE_FONTS_DIR_REL "${MODULE_SETTINGS_PATH}" "${MODULE_FONTS_DIR}")
file(RELATIVE_PATH MODULE_PLUGINS_DIR_REL "${MODULE_SETTINGS_PATH}" "${MODULE_PLUGINS_DIR}")
file(RELATIVE_PATH MODULE_PROJ_PATH_REL "${MODULE_SETTINGS_PATH}" "${MODULE_PROJ_PATH}")

# get mapnik-index and shapeindex name without extension
get_filename_component(MAPNIK_INDEX_NAME "${PATH_MAPNIK_INDEX}" NAME)
get_filename_component(SHAPEINDEX_NAME "${PATH_SHAPEINDEX}" NAME)
# determine relative paths to the settings file for mapnik-index and shapeindex
file(RELATIVE_PATH MODULE_MAPNIK_INDEX_REL "${MODULE_SETTINGS_PATH}" "${MODULE_BIN_PATH}/${MAPNIK_INDEX_NAME}")
file(RELATIVE_PATH MODULE_SHAPEINDEX_REL "${MODULE_SETTINGS_PATH}" "${MODULE_BIN_PATH}/${SHAPEINDEX_NAME}")

# write settings file
configure_file("${SOURCE_DIR}/mapnik_settings.js.in" "${MODULE_SETTINGS_PATH}/mapnik_settings.js")
# copy dlls on windows
if(WIN32)
    message(STATUS "Copy windows dependencies")
    file(GET_RUNTIME_DEPENDENCIES
        RESOLVED_DEPENDENCIES_VAR RESOLVED_DEPS
        UNRESOLVED_DEPENDENCIES_VAR UNRESOLVED_DEPS
        EXECUTABLES
            ${MODULE_BIN_PATH}/${MAPNIK_INDEX_NAME}
            ${MODULE_BIN_PATH}/${SHAPEINDEX_NAME}
        MODULES 
            ${MODULE_FILE_PATH}
            ${MODULE_PLUGINS_DIR}/csv.input 
            ${MODULE_PLUGINS_DIR}/gdal.input
            ${MODULE_PLUGINS_DIR}/geobuf.input
            ${MODULE_PLUGINS_DIR}/geojson.input
            ${MODULE_PLUGINS_DIR}/ogr.input
            ${MODULE_PLUGINS_DIR}/pgraster.input
            ${MODULE_PLUGINS_DIR}/postgis.input
            ${MODULE_PLUGINS_DIR}/raster.input
            ${MODULE_PLUGINS_DIR}/shape.input
            ${MODULE_PLUGINS_DIR}/sqlite.input
            ${MODULE_PLUGINS_DIR}/topojson.input
        DIRECTORIES "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/$<$<CONFIG:Debug>:debug/>bin/"
        PRE_EXCLUDE_REGEXES "api-ms-*" "ext-ms-*"
        POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
    )
    file(COPY ${RESOLVED_DEPS} DESTINATION ${MODULE_PATH})
    foreach(dep ${UNRESOLVED_DEPS})
        message(WARNING "Runtime dependency ${dep} could not be resolved.")
    endforeach()
endif()
]])
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
