#!/usr/bin/env bash
set -eu
set -o pipefail

MODULE_PATH=$(node -e "console.log(path.dirname(require('node-gyp-build').path()))")
PLATFORM=$(node -e "console.log(process.platform)")
ARCH=$(node -e "console.log(process.arch)")
MAPNIK_VERSION=$(node -e "console.log(require('./package.json').optionalDependencies['@mapnik/core-${PLATFORM}-${ARCH}'])")

if [ -z ${MAPNIK_VERSION+x} ]; then
    echo -e "\033[31m Mapnik core package v${MAPNIK_VERSION} is not available for ${PLATFORM}+${ARCH}\033[0m"
    exit 1
fi

NODE_MODULES_DIR=${MODULE_PATH%%/node_modules/*}/node_modules
MAPNIK_CORE_PATH=${NODE_MODULES_DIR}/@mapnik/core-${PLATFORM}-${ARCH}
#echo -e "\033[36mMODULE_PATH:${MODULE_PATH}\033[0m"
#echo -e "\033[36mMAPNIK_CORE_PATH:${MAPNIK_CORE_PATH}\033[0m"

if [[ ! -d $MAPNIK_CORE_PATH ]]; then
    echo -e "\033[36mMissing Mapnik Core package - @mapnik/core-${PLATFORM}-${ARCH}-${MAPNIK_VERSION}\033[0m"
    echo -e "\033[36mAttempting to build using local mapnik...\033[0m"
    MAPNIK_VERSION=$(mapnik-config -v)
    exit 0
fi

cd "${MODULE_PATH}"
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
