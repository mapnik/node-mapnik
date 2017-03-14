#!/bin/bash

set -eu
set -o pipefail

# front end to find and use mapnik-config via mapnik mason package
# or fall back to global install of mapnik

# The reason for this script is to ensure that mason and the mason deps are installed before 'mapnik-config' is run
# This hack is needed because that gyp evaluates variables before targets
# such that we cannot use a dependent target to install mason and mason deps
# first because the variable evaluation will fail since it runs first and mapnik
# will not yet be installed.

if hash mapnik-config 2>/dev/null; then
  mapnik-config $@
else
  ./install_mason.sh
  ./mason_packages/.link/bin/mapnik-config $@
fi


