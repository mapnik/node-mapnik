#!/bin/bash
set -u -e

sudo apt-add-repository --yes ppa:mapnik/v2.2.0
sudo apt-add-repository --yes ppa:mapnik/nightly-2.3
sudo apt-add-repository --yes ppa:mapnik/nightly-trunk
sudo apt-get update -qq
sudo apt-get -qq install g++ gcc
sudo apt-get -qq install libmapnik=2.2.0* mapnik-utils=2.2.0* libmapnik-dev=2.2.0*

npm install --build-from-source
npm test
sudo apt-get purge libmapnik=2.2.0* mapnik-utils=2.2.0* libmapnik-dev=2.2.0*

sudo apt-get -qq install libmapnik=2.3.0* mapnik-utils=2.3.0* libmapnik-dev=2.3.0* mapnik-input-plugin*=2.3.0*
npm install --build-from-source
npm test

sudo apt-get purge libmapnik=2.3.0* mapnik-utils=2.3.0* libmapnik-dev=2.3.0* mapnik-input-plugin*=2.3.0*
sudo apt-get -qq install libmapnik=3.0.0* mapnik-utils=3.0.0* libmapnik-dev=3.0.0* mapnik-input-plugin*=3.0.0*
npm install --build-from-source
npm test
