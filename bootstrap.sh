#!/usr/bin/env bash
set -u -e


export ARGS=""
if [[ ${1:-false} != false ]]; then
    export ARGS=$1
fi

echo $ARGS

function setup_mason() {
    if [[ ! -d ./build ]]; then
        mkdir ./build
    fi
    if [[ ! -d ./build/.mason ]]; then
        git clone --depth 1 https://github.com/mapbox/mason.git ./build/.mason
    else
        echo "Updating to latest mason"
        (cd ./build/.mason && git pull)
    fi
    export MASON_DIR=$(pwd)/build/.mason
    if [[ $(uname -s) == 'Linux' ]]; then source ./build/.mason/scripts/setup_cpp11_toolchain.sh; fi
    export PATH=$(pwd)/build/.mason:$PATH
    export CXX=${CXX:-clang++}
    export CC=${CXX:-clang++}
}

function install() {
    MASON_PLATFORM_ID=$(mason env MASON_PLATFORM_ID)
    if [[ ! -d ./build/mason_packages/${MASON_PLATFORM_ID}/${1}/ ]]; then
        (cd ./build && mason install $1 $2)
        (cd ./build && mason link $1 $2)
    fi
}

function install_mason_deps() {
    install mapnik latest
    install protobuf 2.6.1
    install freetype 2.5.4
    install harfbuzz 2cd5323
    install jpeg_turbo 1.4.0
    install libxml2 2.9.2
    install libpng 1.6.16
    install webp 0.4.2
    install icu 54.1
    install proj 4.8.0
    install libtiff 4.0.4beta
    install boost 1.57.0
    install boost_libsystem 1.57.0
    install boost_libthread 1.57.0
    install boost_libfilesystem 1.57.0
    install boost_libprogram_options 1.57.0
    install boost_libpython 1.57.0
    install boost_libregex 1.57.0
    install boost_libpython 1.57.0
    install pixman 0.32.6
    install cairo 1.12.18
}

function setup_runtime_settings() {
    local MASON_LINKED_ABS=$(pwd)/build/mason_packages/.link
    if [[ $(uname -s) == 'Linux' ]]; then
        readelf -d ${MASON_LINKED_ABS}/lib/libmapnik.so
        export LDFLAGS='-Wl,-z,origin -Wl,-rpath=\$$ORIGIN'
    else
        otool -L ${MASON_LINKED_ABS}/lib/libmapnik.dylib
    fi
    export PROJ_LIB=${MASON_LINKED_ABS}/share/proj
    export ICU_DATA=${MASON_LINKED_ABS}/share/icu/54.1
    export GDAL_DATA=${MASON_LINKED_ABS}/share/gdal
    if [[ $(uname -s) == 'Darwin' ]]; then
        export DYLD_LIBRARY_PATH=$(pwd)/build/mason_packages/.link/lib:${DYLD_LIBRARY_PATH:-''}
    else
        export LD_LIBRARY_PATH=$(pwd)/build/mason_packages/.link/lib:${LD_LIBRARY_PATH:-''}
    fi
    export PATH=$(pwd)/build/mason_packages/.link/bin:${PATH}
}

function build_node_mapnik() {
    local MASON_LINKED_ABS=$(pwd)/build/mason_packages/.link
    npm install node-pre-gyp
    local MODULE_PATH=$(node-pre-gyp reveal module_path ${ARGS})
    # note: dangerous!
    rm -rf ${MODULE_PATH}
    npm install --build-from-source ${ARGS} --clang=1
    npm ls
    # copy shapeindex
    cp ${MASON_LINKED_ABS}/bin/shapeindex ${MODULE_PATH}
    # copy lib
    cp ${MASON_LINKED_ABS}/lib/libmapnik.* ${MODULE_PATH}
    # copy plugins
    cp -r ${MASON_LINKED_ABS}/lib/mapnik ${MODULE_PATH}
    # copy share data
    mkdir -p ${MODULE_PATH}/share/
    cp -r ${MASON_LINKED_ABS}/share/mapnik ${MODULE_PATH}/share/
    echo "
    var path = require('path');
    module.exports.paths = {
        'fonts': path.join(__dirname, 'mapnik/fonts'),
        'input_plugins': path.join(__dirname, 'mapnik/input')
    };
    module.exports.env = {
        'ICU_DATA': path.join(__dirname, 'share/mapnik/icu'),
        'GDAL_DATA': path.join(__dirname, 'share/mapnik/gdal'),
        'PROJ_LIB': path.join(__dirname, 'share/mapnik/proj')
    };
    " > ${MODULE_PATH}/mapnik_settings.js
}

function main() {
    setup_mason
    install_mason_deps
    setup_runtime_settings
    build_node_mapnik
    echo "Ready, now run:"
    echo ""
    echo "    make test"
}

main
set +u +e
