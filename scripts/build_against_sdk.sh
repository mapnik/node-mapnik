#!/bin/bash
set -u -e

: '
On linux depends on node and:

    sudo apt-get update
    sudo apt-get install pkg-config build-essential zlib1g-dev

'
ARGS=""
CURRENT_DIR="$( cd "$( dirname $BASH_SOURCE )" && pwd )"
mkdir -p $CURRENT_DIR/../sdk
cd $CURRENT_DIR/../
export PATH=$(pwd)/node_modules/.bin:${PATH}
cd sdk
BUILD_DIR="$(pwd)"
UNAME=$(uname -s);

if [[ ${1:-false} != false ]]; then
    ARGS=$1
fi

function upgrade_gcc {
    echo "adding gcc-4.8 ppa"
    sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
    echo "updating apt"
    sudo apt-get update -y
    echo "installing C++11 compiler"
    sudo apt-get install -y gcc-4.8 g++-4.8
}

COMPRESSION="tar.bz2"
platform=$(echo $UNAME | sed "y/ABCDEFGHIJKLMNOPQRSTUVWXYZ/abcdefghijklmnopqrstuvwxyz/")
SDK_URI="http://mapnik.s3.amazonaws.com/dist/dev"

if [[ "${CXX11:-false}" != false ]]; then
    # mapnik 3.x / c++11 enabled
    HASH="1545-gb172129-cpp11"
    if [[ $UNAME == 'Linux' ]]; then
        export STDLIB="libstdcpp"
        export CXX_NAME="clang-4.2"
        export CC="gcc-4.8";
        export CXX="g++-4.8";
        upgrade_gcc
    else
        export STDLIB="libcpp"
        export CXX_NAME="clang-3.3"
    fi
    TARBALL_NAME="mapnik-${platform}-sdk-v2.2.0-${HASH}-${STDLIB}-${CXX_NAME}-lto-trusty"
    REMOTE_URI="${SDK_URI}/${TARBALL_NAME}.${COMPRESSION}"
else
    # mapnik 2.3.x / c++11 not enabled
    HASH="548-gca2c0d0-cpp03"
    export STDLIB="libstdcpp"
    if [[ $UNAME == 'Linux' ]]; then
        export CXX_NAME="gcc-4.6"
    else
        export CXX_NAME="clang-3.3"
    fi
    TARBALL_NAME="mapnik-${platform}-sdk-v2.2.0-${HASH}-${STDLIB}-${CXX_NAME}"
    REMOTE_URI="${SDK_URI}/${TARBALL_NAME}.${COMPRESSION}"
fi

if [[ $platform == 'darwin' ]]; then
    platform="macosx"
fi
export MAPNIK_SDK=${BUILD_DIR}/${TARBALL_NAME}
export PATH=${MAPNIK_SDK}/bin:${PATH}
export PKG_CONFIG_PATH=${MAPNIK_SDK}/lib/pkgconfig

echo "looking for ~/projects/mapnik-packaging/osx/out/dist/${TARBALL_NAME}.${COMPRESSION}"
if [ -f "$HOME/projects/mapnik-packaging/osx/out/dist/${TARBALL_NAME}.${COMPRESSION}" ]; then
    echo "copying over ${TARBALL_NAME}.${COMPRESSION}"
    cp "$HOME/projects/mapnik-packaging/osx/out/dist/${TARBALL_NAME}.${COMPRESSION}" .
else
    if [ ! -f "${TARBALL_NAME}.${COMPRESSION}" ]; then
        echo "downloading ${REMOTE_URI}"
        curl -f -o "${TARBALL_NAME}.${COMPRESSION}" "${REMOTE_URI}"
    fi
fi

if [ ! -d ${TARBALL_NAME} ]; then
    echo "unpacking ${TARBALL_NAME}"
    tar xf ${TARBALL_NAME}.${COMPRESSION}
fi

if [[ ! `which pkg-config` ]]; then
    echo 'pkg-config not installed'
    exit 1
fi

if [[ ! `which node` ]]; then
    echo 'node not installed'
    exit 1
fi

if [[ $UNAME == 'Linux' ]]; then
    export CXXFLAGS="-Wno-unused-local-typedefs"
    readelf -d $MAPNIK_SDK/lib/libmapnik.so
    #sudo apt-get install chrpath -y
    #chrpath -r '$ORIGIN/' ${MAPNIK_SDK}/lib/libmapnik.so
    export LDFLAGS='-Wl,-z,origin -Wl,-rpath=\$$ORIGIN'
fi

cd ../
npm install node-pre-gyp
MODULE_PATH=$(node-pre-gyp reveal module_path ${ARGS})
# note: dangerous!
rm -rf ${MODULE_PATH}
npm install --build-from-source ${ARGS}
npm ls
# copy lib
cp ${MAPNIK_SDK}/lib/libmapnik.* ${MODULE_PATH}
# copy plugins
cp -r ${MAPNIK_SDK}/lib/mapnik ${MODULE_PATH}
# copy share data
mkdir -p ${MODULE_PATH}/share/
cp -r ${MAPNIK_SDK}/share/mapnik ${MODULE_PATH}/share/
# generate new settings
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

# cleanup
rm -rf $BUILD_DIR
