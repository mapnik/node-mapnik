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
    install jpeg_turbo 1.5.1 libjpeg
    install libpng 1.6.28 libpng
    install libtiff 4.0.7 libtiff
    install icu ${ICU_VERSION}
    install proj 4.9.3 libproj
    install pixman 0.34.0 libpixman-1
    install cairo 1.14.8 libcairo
    install webp 0.6.0 libwebp
    install libgdal 2.1.3 libgdal
    install boost 1.63.0
    install boost_libsystem 1.63.0
    install boost_libfilesystem 1.63.0
    install boost_libregex_icu 1.63.0
    install freetype 2.7.1 libfreetype
    install harfbuzz 1.4.2-ft libharfbuzz

    # mapnik
    install mapnik 3.0.13

    # node-mapnik deps
    install protozero 1.5.1
fi