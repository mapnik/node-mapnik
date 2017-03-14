#!/usr/bin/env bash
set -eu
set -o pipefail

MODULE_PATH=./lib/binding
MAPNIK_SDK=./mason_packages/.link

mkdir -p ${MODULE_PATH}/bin/
cp ${MAPNIK_SDK}/bin/mapnik-index ${MODULE_PATH}/bin/
# copy shapeindex
cp ${MAPNIK_SDK}/bin/shapeindex ${MODULE_PATH}/bin/
# copy lib
mkdir -p ${MODULE_PATH}/lib/
cp ${MAPNIK_SDK}/lib/libmapnik.* ${MODULE_PATH}/lib/
# copy plugins
cp -r ${MAPNIK_SDK}/lib/mapnik ${MODULE_PATH}
# copy share data
mkdir -p ${MODULE_PATH}/share/gdal
cp -r ${MAPNIK_SDK}/share/gdal/*.* ${MODULE_PATH}/share/gdal/
cp -r ${MAPNIK_SDK}/share/proj ${MODULE_PATH}/share/
mkdir -p ${MODULE_PATH}/share/icu
cp -r ${MAPNIK_SDK}/share/icu/*dat ${MODULE_PATH}/share/icu/

# generate new settings
echo "
var path = require('path');
module.exports.paths = {
    'fonts': path.join(__dirname, 'mapnik/fonts'),
    'input_plugins': path.join(__dirname, 'mapnik/input')
};
module.exports.env = {
    'ICU_DATA': path.join(__dirname, 'share/icu'),
    'GDAL_DATA': path.join(__dirname, 'share/gdal'),
    'PROJ_LIB': path.join(__dirname, 'share/proj')
};
" > ${MODULE_PATH}/mapnik_settings.js
