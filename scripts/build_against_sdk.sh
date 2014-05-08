#!/bin/bash
#set -u -e

CURRENT_DIR="$( cd "$( dirname $BASH_SOURCE )" && pwd )"
mkdir -p $CURRENT_DIR/../sdk
cd $CURRENT_DIR/../sdk
BUILD_DIR="$(pwd)"
UNAME=$(uname -s);

function upgrade_gcc {
    echo "adding gcc-4.8 ppa"
    sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
    echo "updating apt"
    sudo apt-get update -qq -y
    echo "installing C++11 compiler"
    sudo apt-get install -qq -y gcc-4.8 g++-4.8
}

if [[ "${CXX11:-false}" != false ]]; then
    # mapnik 3.x / c++11 enabled
    HASH="1048-g439fc9f-cpp11"
    if [[ $UNAME == 'Linux' ]]; then
        CXX_NAME="gcc-4.8"
        CC="gcc-4.8";
        CXX="g++-4.8";
        upgrade_gcc
    else
        CXX_NAME="clang-3.3"
    fi
else
    # mapnik 2.3.x / c++11 not enabled
    HASH="468-g94e78f2-cpp03"
    if [[ $UNAME == 'Linux' ]]; then
        export CXX_NAME="gcc-4.6"
    else
        CXX_NAME="clang-3.3"
    fi
fi

platform=$(echo $UNAME | sed "y/ABCDEFGHIJKLMNOPQRSTUVWXYZ/abcdefghijklmnopqrstuvwxyz/")
if [[ $platform == 'darwin' ]]; then
    platform="macosx"
fi
SDK_URI="http://mapnik.s3.amazonaws.com/dist/dev"
COMPRESSION="tar.bz2"
TARBALL_NAME="mapnik-${platform}-sdk-v2.2.0-${HASH}-libstdcpp-${CXX_NAME}"
REMOTE_URI="${SDK_URI}/${TARBALL_NAME}.${COMPRESSION}"
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

if [[ $UNAME == 'Linux' ]]; then
    export CXXFLAGS="-Wno-unused-local-typedefs"
    readelf -d $MAPNIK_SDK/lib/libmapnik.so
    #sudo apt-get install chrpath -y
    #chrpath -r '$ORIGIN/' ${MAPNIK_SDK}/lib/libmapnik.so
    export LDFLAGS='-Wl,-z,origin -Wl,-rpath=\$$ORIGIN'
fi

cd ../
rm -rf ./lib/binding/
mkdir -p ./lib/binding/
npm install --build-from-source $1
# copy lib
cp $MAPNIK_SDK/lib/libmapnik.* lib/binding/
# copy plugins
cp -r $MAPNIK_SDK/lib/mapnik lib/binding/
# copy share data
mkdir -p lib/binding/share/
cp -r $MAPNIK_SDK/share/mapnik lib/binding/share/
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
" > lib/binding/mapnik_settings.js

# cleanup
rm -rf $BUILD_DIR