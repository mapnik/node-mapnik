#!/usr/bin/env bash
set -eu
set -o pipefail

MODULE_PATH=$(node -e "console.log(path.dirname(require('node-gyp-build').path()))")
MAPNIK_DIR=../

echo "MODULE_PATH:"$MODULE_PATH

if [[ ! "$(which mapnik-config)" -ef "$MAPNIK_DIR/_stage/bin/mapnik-config" ]]; then
    echo "
var path = require('path');
module.exports.paths = {
    'fonts':         '$(mapnik-config --fonts)',
    'input_plugins': '$(mapnik-config --input-plugins)',
    'mapnik_index':  '$(which mapnik-index)',
    'shape_index':   '$(which shapeindex)'
};
module.exports.env = {
    'ICU_DATA':      '$(mapnik-config --icu-data)',
    'GDAL_DATA':     '$(mapnik-config --gdal-data)',
    'PROJ_LIB':      '$(mapnik-config --proj-lib)'
};
" > ${MODULE_PATH}/mapnik_settings.js

else
    # Here we assume we are using the mason mapnik package, and therefore
    # we copy all the data over to make a portable package.

    mkdir -p ${MODULE_PATH}/bin/

    # the below switch is used since on osx the default cp
    # resolves symlinks automatically with `cp -r`
    # whereas on linux we need to pass `cp -rL`. But the latter
    # command is not supported on OS X. We could upgrade coreutils
    # but ideally we don't depend on more dependencies
    if [[ $(uname -s) == 'Darwin' ]]; then
        cp ${MAPNIK_DIR}/_stage/bin/mapnik-index ${MODULE_PATH}/bin/
        # copy shapeindex
        cp ${MAPNIK_DIR}/_stage/bin/shapeindex ${MODULE_PATH}/bin/
        # copy lib
        mkdir -p ${MODULE_PATH}/lib/
        cp ${MAPNIK_DIR}/_stage/lib/libmapnik.* ${MODULE_PATH}/lib/
        # copy plugins
        cp -r ${MAPNIK_DIR}/_stage/lib/mapnik ${MODULE_PATH}/lib/
        # copy share data
        mkdir -p ${MODULE_PATH}/share/gdal
        cp -L ${MAPNIK_DIR}/_deps/share/gdal/*.* ${MODULE_PATH}/share/gdal/
        cp -r ${MAPNIK_DIR}/_deps/share/proj ${MODULE_PATH}/share/
        mkdir -p ${MODULE_PATH}/share/icu
        cp -L ${MAPNIK_DIR}/_deps/share/icu/*/*dat ${MODULE_PATH}/share/icu/
    else
        cp -L ${MAPNIK_DIR}/_stage/bin/mapnik-index ${MODULE_PATH}/bin/
        # copy shapeindex
        cp -L ${MAPNIK_DIR}/_stage/bin/shapeindex ${MODULE_PATH}/bin/
        # copy lib
        mkdir -p ${MODULE_PATH}/lib/
        cp -L ${MAPNIK_DIR}/_stage/lib/libmapnik.* ${MODULE_PATH}/lib/
        # copy plugins
        cp -rL ${MAPNIK_DIR}/_stage/lib/mapnik ${MODULE_PATH}/lib/
        # copy share data
        mkdir -p ${MODULE_PATH}/share/gdal
        cp -rL ${MAPNIK_DIR}/_deps/share/gdal/*.* ${MODULE_PATH}/share/gdal/
        cp -rL ${MAPNIK_DIR}/_deps/share/proj ${MODULE_PATH}/share/
        mkdir -p ${MODULE_PATH}/share/icu
        cp -rL ${MAPNIK_DIR}/_deps/share/icu/*/*dat ${MODULE_PATH}/share/icu/
    fi

    # generate new settings
    echo "
var path = require('path');
module.exports.paths = {
    'fonts': path.join(__dirname, '../../lib/mapnik/fonts'),
    'input_plugins': path.join(__dirname, 'lib/mapnik/input'),
    'mapnik_index': path.join(__dirname, 'bin/mapnik-index'),
    'shape_index': path.join(__dirname, 'bin/shapeindex')
};
module.exports.env = {
    'ICU_DATA': path.join(__dirname, '../../share/icu'),
    'GDAL_DATA': path.join(__dirname, '../../share/gdal'),
    'PROJ_LIB': path.join(__dirname, '../../share/proj')
};
" > ${MODULE_PATH}/mapnik_settings.js


fi
