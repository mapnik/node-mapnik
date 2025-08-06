#!/usr/bin/env bash
set -eu
set -o pipefail
MODULE_PATH=$(node -e "console.log(path.dirname(require('node-gyp-build').path()))")
echo "MODULE_PATH:${MODULE_PATH}"

if [[ ! ${MODULE_PATH} == *"node_modules"* ]]; then
    echo -e "\033[36mAssuming local install\033[0m"
    echo -e "\033[35mMODULE_PATH:${MODULE_PATH}\033[0m"
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
    cd "${MODULE_PATH}"
    ln -sf $(mapnik-config --prefix)/bin .
    ln -sf $(mapnik-config --prefix)/lib .
    exit 0
fi
