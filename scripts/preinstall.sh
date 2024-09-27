#!/usr/bin/env bash
set -eu
set -o pipefail

MODULE_PATH=$(node -e "console.log(path.dirname(require('node-gyp-build').path()))")
PLATFORM=$(node -e "console.log(process.platform)")
ARCH=$(node -e "console.log(process.arch)")
MAPNIK_VERSION=$(node -e "console.log(require('./package.json').optionalDependencies['@mapnik/core-${PLATFORM}-${ARCH}'])")

if [ -z ${MAPNIK_VERSION+x} ]; then
    echo -e "\033[31m Mapnik core package v${MAPNIK_VERSION} is not available for ${PLATFORM}+${ARCH}"
    exit 1
fi

MAPNIK_CORE_PATH=`npm ls --parseable @mapnik/core-${PLATFORM}-${ARCH}`

if [[ -z "$MAPNIK_CORE_PATH" ]]; then
    echo -e "\033[31m Missing Mapnik Core package - @mapnik/core-${PLATFORM}-${ARCH}-${MAPNIK_VERSION}"
    exit 1
fi

cd ${MODULE_PATH}
ln -sf ${MAPNIK_CORE_PATH}/bin .
ln -sf ${MAPNIK_CORE_PATH}/lib .

# # generate mapnik_settings.js
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
