#!/bin/bash

set -eu
set -o pipefail

GDAL_VERSION="2.1.3"
POSTGIS_VERSION="2.3.2-1"

# do once: install stuff
mason install libgdal ${GDAL_VERSION}
GDAL_DATA_VALUE=$(mason prefix libgdal ${GDAL_VERSION})/share/gdal/
mason install postgis ${POSTGIS_VERSION}
mason link postgis ${POSTGIS_VERSION}


if [[ ! -d ${GDAL_DATA_VALUE} ]]; then
    echo "${GDAL_DATA_VALUE} not found (needed for postgis to access GDAL_DATA)"
    exit 1
fi


# setup config
echo 'export CURRENT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"' > mason-postgis-config.env
echo 'export PGDATA=${CURRENT_DIR}/local-postgres' >> mason-postgis-config.env
echo 'export PGHOST=${CURRENT_DIR}/local-unix-socket' >> mason-postgis-config.env
echo 'export PGTEMP_DIR=${CURRENT_DIR}/local-tmp' >> mason-postgis-config.env
echo 'export PGPORT=1111' >> mason-postgis-config.env
echo 'export PATH=${CURRENT_DIR}/mason_packages/.link/bin:${PATH}' >> mason-postgis-config.env
echo "export GDAL_DATA=${GDAL_DATA_VALUE}" >> mason-postgis-config.env
echo "generated mason-postgis-config.env"
