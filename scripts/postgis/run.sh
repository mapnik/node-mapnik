#!/bin/bash

set -eu
set -o pipefail

if [[ ! -f mason-postgis-config.env ]]; then
    echo "Please run setup.sh first"
    exit 1
fi

# do each time you use the local postgis:
source mason-postgis-config.env

# do each time you use this local postgis:
# start server and background (NOTE: if running interactively hit return to fully background and get your prompt back)
./mason_packages/.link/bin/postgres -k $PGHOST > postgres.log &
sleep 2

echo "Server is now running"
echo "To stop server do:"
echo "    source mason-postgis-config.env && ./mason_packages/.link/bin/pg_ctl -w stop"
