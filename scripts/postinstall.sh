#!/usr/bin/env bash
set -eu
set -o pipefail

MODULE_PATH=$(node -e "console.log(path.dirname(require('node-gyp-build').path()))")
MAPNIK_DIR=../

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
    PLATFORM=$(node -e "console.log(process.platform)")
    ARCH=$(node -e "console.log(process.arch)")
    MAPNIK_VERSION=$(node -e "console.log(require('./package.json').optionalDependencies['@mapnik/core-${PLATFORM}-${ARCH}'])")
    if [ -z ${MAPNIK_VERSION+x} ]; then
        echo "Mapnik core package v${MAPNIK_VERSION} is not available for ${PLATFORM}+${ARCH}"

        exit 0
    fi
    if [ ! -f node_modules/@mapnik/core-${PLATFORM}-${ARCH}/package.json ]; then
        echo "Missing Mapnik Core package - @mapnik/core-${PLATFORM}-${ARCH}-${MAPNIK_VERSION}"
        exit 0
    fi
    echo "Mapnik Core package: @mapnik/core-${PLATFORM}-${ARCH}-${MAPNIK_VERSION}"
    cd ${MODULE_PATH}
    ln -sf ../../node_modules/@mapnik/core-${PLATFORM}-${ARCH}/bin .
    ln -sf ../../node_modules/@mapnik/core-${PLATFORM}-${ARCH}/lib .
    # generate mapnik_settings.js
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
