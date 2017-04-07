#!/usr/bin/env bash
set -eu
set -o pipefail

MODULE_PATH=./lib/binding
MAPNIK_SDK=./mason_packages/.link

mkdir -p ${MODULE_PATH}/bin/
cp -L ${MAPNIK_SDK}/bin/mapnik-index ${MODULE_PATH}/bin/
# copy shapeindex
cp -L ${MAPNIK_SDK}/bin/shapeindex ${MODULE_PATH}/bin/
# copy lib
mkdir -p ${MODULE_PATH}/lib/
cp -L ${MAPNIK_SDK}/lib/libmapnik.* ${MODULE_PATH}/lib/
# copy plugins
cp -rL ${MAPNIK_SDK}/lib/mapnik ${MODULE_PATH}/lib/
# copy share data
mkdir -p ${MODULE_PATH}/share/gdal
cp -rL ${MAPNIK_SDK}/share/gdal/*.* ${MODULE_PATH}/share/gdal/
cp -rL ${MAPNIK_SDK}/share/proj ${MODULE_PATH}/share/
mkdir -p ${MODULE_PATH}/share/icu
cp -rL ${MAPNIK_SDK}/share/icu/*/*dat ${MODULE_PATH}/share/icu/

# generate new settings
echo "
var path = require('path');
module.exports.paths = {
    'fonts': path.join(__dirname, 'lib/mapnik/fonts'),
    'input_plugins': path.join(__dirname, 'lib/mapnik/input'),
    'mapnik_index': path.join(__dirname, 'bin/mapnik-index'),
    'shape_index': path.join(__dirname, 'bin/shapeindex')
};
module.exports.env = {
    'ICU_DATA': path.join(__dirname, 'share/icu'),
    'GDAL_DATA': path.join(__dirname, 'share/gdal'),
    'PROJ_LIB': path.join(__dirname, 'share/proj')
};
" > ${MODULE_PATH}/mapnik_settings.js
