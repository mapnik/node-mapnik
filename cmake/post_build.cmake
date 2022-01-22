message(STATUS "creating mapnik_settings.js")
set(MODULE_PATH "${CMAKE_INSTALL_PREFIX}")
set(MODULE_FILE_PATH "${MODULE_PATH}/mapnik.node")
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
if(WIN32)
    set(PROJ_DB_PATH "'PROJ_LIB': path.join(__dirname, '${MODULE_PROJ_PATH_REL}')")
endif()
configure_file("${SOURCE_DIR}/mapnik_settings.js.in" "${MODULE_SETTINGS_PATH}/mapnik_settings.js")
# copy dlls on windows

if(WIN32)
    message(STATUS "Copy windows dependencies")
    file(GET_RUNTIME_DEPENDENCIES
        RESOLVED_DEPENDENCIES_VAR RESOLVED_DEPS
        UNRESOLVED_DEPENDENCIES_VAR UNRESOLVED_DEPS
        CONFLICTING_DEPENDENCIES_PREFIX CONFLICTING_DEPS
        EXECUTABLES
            "${MODULE_BIN_PATH}/${MAPNIK_INDEX_NAME}"
            "${MODULE_BIN_PATH}/${SHAPEINDEX_NAME}"
        MODULES
            "${MODULE_FILE_PATH}"
            "${MODULE_PLUGINS_DIR}/csv.input"
            "${MODULE_PLUGINS_DIR}/gdal.input"
            "${MODULE_PLUGINS_DIR}/geobuf.input"
            "${MODULE_PLUGINS_DIR}/geojson.input"
            "${MODULE_PLUGINS_DIR}/ogr.input"
            "${MODULE_PLUGINS_DIR}/pgraster.input"
            "${MODULE_PLUGINS_DIR}/postgis.input"
            "${MODULE_PLUGINS_DIR}/raster.input"
            "${MODULE_PLUGINS_DIR}/shape.input"
            "${MODULE_PLUGINS_DIR}/sqlite.input"
            "${MODULE_PLUGINS_DIR}/topojson.input"
        DIRECTORIES 
            "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/$<$<CONFIG:Debug>:debug/>bin/"
        PRE_EXCLUDE_REGEXES "api-ms-*" "ext-ms-*"
        POST_EXCLUDE_REGEXES ".*system32[/\\].*\\.dll"
    )
    file(COPY ${RESOLVED_DEPS} DESTINATION "${MODULE_PATH}")
    message(STATUS "resolved: ${RESOLVED_DEPS}")
    message(STATUS "conflicting: ${CONFLICTING_DEPS}")
    foreach(dep ${UNRESOLVED_DEPS})
        message(WARNING "Runtime dependency ${dep} could not be resolved.")
    endforeach()
else()
    message(STATUS "Copy mapnik shared lib $<TARGET_FILE:mapnik::mapnik>")
    file(COPY "$<TARGET_FILE:mapnik::mapnik>" DESTINATION "${MODULE_PATH}")
endif()
