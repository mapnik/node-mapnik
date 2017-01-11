#!/usr/bin/env bash
set -u -e

: '
On linux depends on node and:

    sudo apt-get update
    sudo apt-get install pkg-config build-essential zlib1g-dev
'

git submodule update --init

CURRENT_DIR="$( cd "$( dirname $BASH_SOURCE )" && pwd )"

ARGS="$@"
mkdir -p $CURRENT_DIR/../sdk
cd $CURRENT_DIR/../

export MAPNIK_GIT=${MAPNIK_GIT:-$(node -e "console.log(require('./package.json').mapnik_version)")}
export PATH=$(pwd)/node_modules/.bin:${PATH}
cd sdk
BUILD_DIR="$(pwd)"
UNAME=$(uname -s);


COMPRESSION="tar.bz2"
SDK_URI="http://mapnik.s3.amazonaws.com/dist/dev"
platform=$(echo $UNAME | sed "y/ABCDEFGHIJKLMNOPQRSTUVWXYZ/abcdefghijklmnopqrstuvwxyz/")

# mapnik 3.x / c++11 enabled
if [[ ${platform} == 'linux' ]]; then
    TARBALL_NAME="mapnik-${platform}-sdk-${MAPNIK_GIT}"
fi

if [[ $platform == 'darwin' ]]; then
    platform="macosx"
    TARBALL_NAME="mapnik-${platform}-sdk-${MAPNIK_GIT}"
fi

REMOTE_URI="${SDK_URI}/${TARBALL_NAME}.${COMPRESSION}"
export MAPNIK_SDK=${BUILD_DIR}/${TARBALL_NAME}
export PATH=${MAPNIK_SDK}/bin:${PATH}

LOCAL_PACKAGE="$CURRENT_DIR/../../mapnik-packaging/osx/out/dist"
echo "looking for ${LOCAL_PACKAGE}/${TARBALL_NAME}.${COMPRESSION}"
if [ -f "${LOCAL_PACKAGE}/${TARBALL_NAME}.${COMPRESSION}" ]; then
    LOCAL_PACKAGE=$(realpath $LOCAL_PACKAGE)
    echo "copying over ${TARBALL_NAME}.${COMPRESSION}"
    cp "${LOCAL_PACKAGE}/${TARBALL_NAME}.${COMPRESSION}" .
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

if [[ ! `which node` ]]; then
    echo 'node not installed'
    exit 1
fi

export LDFLAGS=${LDFLAGS:-""}
export CXXFLAGS="${CXXFLAGS:-""} -D_GLIBCXX_USE_CXX11_ABI=0"

if [[ $UNAME == 'Linux' ]]; then
    export LDFLAGS='-Wl,-z,origin -Wl,-rpath=\$$ORIGIN '${LDFLAGS}
fi

if [[ ${COVERAGE:-false} == true ]]; then
    export LDFLAGS="--coverage ${LDFLAGS}"
    export CXXFLAGS="--coverage ${CXXFLAGS}"
fi

cd ../
npm install node-pre-gyp
MODULE_PATH=$(node-pre-gyp reveal module_path ${ARGS})
# note: dangerous!
rm -rf ${MODULE_PATH}
npm install --build-from-source ${ARGS} --clang=1

# copy mapnik-index
cp ${MAPNIK_SDK}/bin/mapnik-index ${MODULE_PATH}
# copy shapeindex
cp ${MAPNIK_SDK}/bin/shapeindex ${MODULE_PATH}
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
#rm -rf $BUILD_DIR
set +u +e
