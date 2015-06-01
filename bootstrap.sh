#!/bin/bash

function dep() {
    ./.mason/mason install $1 $2
    ./.mason/mason link $1 $2
}

# default to clang
CXX=${CXX:-clang++}
COVERAGE=${COVERAGE:-false}

function all_deps() {
    dep cairo 1.14.0 &
    dep zlib system &
    dep mapnik latest &
    dep protobuf 2.6.1 &
    dep icu 54.1 &
    dep boost 1.57.0 &
    dep boost_libsystem 1.57.0 &
    dep boost_libthread 1.57.0 &
    dep boost_libfilesystem 1.57.0 &
    dep boost_libprogram_options 1.57.0 &
    dep boost_libregex 1.57.0 &
    dep boost_libpython 1.57.0 &
    dep libpng 1.6.16 &
    dep webp 0.4.2 &
    dep harfbuzz 0.9.40 &
    wait
}

function main() {
    if [[ ! -d ./.mason ]]; then
        git clone --depth 1 https://github.com/mapbox/mason.git ./.mason
    fi
    export MASON_DIR=$(pwd)/.mason
    export MASON_HOME=$(pwd)/mason_packages/.link
    all_deps
    export PATH=${MASON_HOME}/bin:$PATH
    export PKG_CONFIG_PATH=${MASON_HOME}/lib/pkgconfig

    # environment variables to tell the compiler and linker
    # to prefer mason paths over other paths when finding
    # headers and libraries. This should allow the build to
    # work even when conflicting versions of dependencies
    # exist on global paths
    # stopgap until c++17 :) (http://www.open-std.org/JTC1/SC22/WG21/docs/papers/2014/n4214.pdf)
    export C_INCLUDE_PATH="${MASON_HOME}/include"
    export CPLUS_INCLUDE_PATH="${MASON_HOME}/include"
    export LIBRARY_PATH="${MASON_HOME}/lib"

    if [[ $(uname -s) == 'Linux' ]]; then
        export LD_LIBRARY_PATH="${LIBRARY_PATH}"
    elif [[ $(uname -s) == 'Darwin' ]]; then
        export DYLD_LIBRARY_PATH="${LIBRARY_PATH}"
    fi
}

main
