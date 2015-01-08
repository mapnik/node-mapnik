#!/usr/bin/env bash
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

function upgrade_clang {
    CLANG_VERSION="3.5"
    if [[ $(lsb_release --id) =~ "Ubuntu" ]]; then
        echo "adding clang + gcc-4.8 ppa"
        sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
        if [[ $(lsb_release --release) =~ "12.04" ]]; then
           sudo add-apt-repository "deb http://llvm.org/apt/precise/ llvm-toolchain-precise-${CLANG_VERSION} main"
        fi
        if [[ $(lsb_release --release) =~ "14.04" ]]; then
           sudo add-apt-repository "deb http://llvm.org/apt/trusty/ llvm-toolchain-trusty-${CLANG_VERSION} main"
        fi
        echo "updating apt"
        sudo apt-get update -y
        echo 'upgrading libstdc++'
        sudo apt-get install -y libstdc++6 libstdc++-4.8-dev
    fi
    if [[ $(lsb_release --id) =~ "Debian" ]]; then
        if [[ $(lsb_release --codename) =~ "wheezy" ]]; then
           sudo apt-get install -y python-software-properties
           sudo add-apt-repository "deb http://llvm.org/apt/wheezy/ llvm-toolchain-wheezy-${CLANG_VERSION} main"
        fi
        if [[ $(lsb_release --codename) =~ "jessie" ]]; then
           sudo apt-get install -y software-properties-common
           sudo add-apt-repository "deb http://llvm.org/apt/unstable/ llvm-toolchain-${CLANG_VERSION} main"
        fi
        echo "updating apt"
        sudo apt-get update -y
        echo 'upgrading libstdc++'
        sudo apt-get install -y libstdc++6 libstdc++-4.9-dev
    fi
    wget -O - http://llvm.org/apt/llvm-snapshot.gpg.key|sudo apt-key add -
    echo "updating apt"
    sudo apt-get update -y
    echo "installing clang-${CLANG_VERSION}"
    apt-cache policy clang-${CLANG_VERSION}
    sudo apt-get install -y clang-${CLANG_VERSION}
    echo "installing C++11 compiler"
    if [[ ${LTO:-false} != false ]]; then
        echo "upgrading binutils-gold"
        sudo apt-get install -y -qq binutils-gold
        if [[ ! -h "/usr/lib/LLVMgold.so" ]] && [[ ! -f "/usr/lib/LLVMgold.so" ]]; then
            echo "symlinking /usr/lib/llvm-${CLANG_VERSION}/lib/LLVMgold.so"
            sudo ln -s /usr/lib/llvm-${CLANG_VERSION}/lib/LLVMgold.so /usr/lib/LLVMgold.so
        fi
        if [[ ! -h "/usr/lib/libLTO.so" ]] && [[ ! -f "/usr/lib/libLTO.so" ]]; then
            echo "symlinking /usr/lib/llvm-${CLANG_VERSION}/lib/libLTO.so"
            sudo ln -s /usr/lib/llvm-${CLANG_VERSION}/lib/libLTO.so /usr/lib/libLTO.so
        fi
        # TODO - needed on trusty for pkg-config
        # since 'binutils-gold' on trusty does not switch
        # /usr/bin/ld to point to /usr/bin/ld.gold like it does
        # in the precise package
        #sudo rm /usr/bin/ld
        #sudo ln -s /usr/bin/ld.gold /usr/bin/ld
    fi
    # for bjam since it can't find a custom named clang-3.4
    if [[ ! -h "/usr/bin/clang" ]] && [[ ! -f "/usr/bin/clang" ]]; then
        echo "symlinking /usr/bin/clang-${CLANG_VERSION}"
        sudo ln -s /usr/bin/clang-${CLANG_VERSION} /usr/bin/clang
    fi
    if [[ ! -h "/usr/bin/clang++" ]] && [[ ! -f "/usr/bin/clang++" ]]; then
        echo "symlinking /usr/bin/clang++-${CLANG_VERSION}"
        sudo ln -s /usr/bin/clang++-${CLANG_VERSION} /usr/bin/clang++
    fi
    # prefer upgraded clang
    if [[ -f "/usr/bin/clang++-${CLANG_VERSION}" ]]; then
        export CC="/usr/bin/clang-${CLANG_VERSION}"
        export CXX="/usr/bin/clang++-${CLANG_VERSION}"
    else
        export CC="/usr/bin/clang"
        export CXX="/usr/bin/clang++"
    fi
}

COMPRESSION="tar.bz2"
SDK_URI="http://mapnik.s3.amazonaws.com/dist/dev"
platform=$(echo $UNAME | sed "y/ABCDEFGHIJKLMNOPQRSTUVWXYZ/abcdefghijklmnopqrstuvwxyz/")
# mapnik 3.x / c++11 enabled
if [[ ${platform} == 'linux' ]]; then
    upgrade_clang
    TARBALL_NAME="mapnik-${platform}-sdk-v3.0.0-rc1-249-g481b6b7"
fi

if [[ $platform == 'darwin' ]]; then
    platform="macosx"
    TARBALL_NAME="mapnik-${platform}-sdk-v3.0.0-rc1-249-g481b6b7-lto"
fi


REMOTE_URI="${SDK_URI}/${TARBALL_NAME}.${COMPRESSION}"
export MAPNIK_SDK=${BUILD_DIR}/${TARBALL_NAME}
export PATH=${MAPNIK_SDK}/bin:${PATH}

LOCAL_PACKAGE="$HOME/projects/mapnik-package-lto/osx/out/dist"
echo "looking for ${LOCAL_PACKAGE}/${TARBALL_NAME}.${COMPRESSION}"
if [ -f "${LOCAL_PACKAGE}/${TARBALL_NAME}.${COMPRESSION}" ]; then
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

if [[ $UNAME == 'Linux' ]]; then
    readelf -d $MAPNIK_SDK/lib/libmapnik.so
    #sudo apt-get install chrpath -y
    #chrpath -r '$ORIGIN/' ${MAPNIK_SDK}/lib/libmapnik.so
    export LDFLAGS='-Wl,-z,origin -Wl,-rpath=\$$ORIGIN'
else
    # until 10.10 lands: http://blog.travis-ci.com/2014-11-03-xcode-61-beta/
    #python -c "data=open('${MAPNIK_SDK}/bin/mapnik-config','r').read();open('${MAPNIK_SDK}/bin/mapnik-config','w').write(data.replace('10.10','10.9'))"
    otool -L $MAPNIK_SDK/lib/libmapnik.dylib
fi

cd ../
npm install node-pre-gyp
MODULE_PATH=$(node-pre-gyp reveal module_path ${ARGS})
# note: dangerous!
rm -rf ${MODULE_PATH}
npm install --build-from-source ${ARGS} --clang=1
npm ls
# copy shapeindex and nik2img
cp ${MAPNIK_SDK}/bin/shapeindex ${MODULE_PATH}
cp ${MAPNIK_SDK}/bin/nik2img ${MODULE_PATH}
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
set +u +e
