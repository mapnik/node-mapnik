#!/bin/bash

set -eu
set -o pipefail

function install() {
    ./mason/mason install $1 $2
    ./mason/mason link $1 $2
}

ICU_VERSION="55.1"

if [ ! -d ./mason ]; then
    git clone --branch mapnik-3.0.13 --single-branch https://github.com/mapbox/mason.git
fi

if [ ! -f ./mason_packages/.link/bin/mapnik-config ]; then

    # mapnik deps
    install jpeg_turbo 1.5.1
    install libpng 1.6.28
    install libtiff 4.0.7
    install icu ${ICU_VERSION}
    install proj 4.9.3
    install pixman 0.34.0
    install cairo 1.14.8
    install webp 0.6.0
    install libgdal 2.1.3
    install boost 1.63.0
    install boost_libsystem 1.63.0
    install boost_libfilesystem 1.63.0
    install boost_libregex_icu 1.63.0
    install freetype 2.7.1
    install harfbuzz 1.4.2-ft

    # mapnik
    install mapnik 3.0.13

    # node-mapnik deps
    install protozero 1.5.1
fi