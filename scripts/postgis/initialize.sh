#!/bin/bash

set -eu
set -o pipefail

if [[ ! -f mason-postgis-config.env ]]; then
    echo "Please run setup.sh first"
    exit 1
fi

# do each time you use the local postgis:
source mason-postgis-config.env

# do once: create directories to hold postgres data
echo "Creating pghost: ${PGHOST} and temp dir: ${PGTEMP_DIR}"
mkdir ${PGHOST}
mkdir ${PGTEMP_DIR}

# do once: initialize local db cluster
echo "Initializing database cluser at ${PGDATA}"
./mason_packages/.link/bin/initdb
sleep 2

# start server and background (NOTE: if running interactively hit return to fully background and get your prompt back)
./mason_packages/.link/bin/postgres -k $PGHOST > postgres.log &
sleep 2

# set up postgres to know about local temp directory
./mason_packages/.link/bin/psql postgres -c "CREATE TABLESPACE temp_disk LOCATION '${PGTEMP_DIR}';"
./mason_packages/.link/bin/psql postgres -c "SET temp_tablespaces TO 'temp_disk';"

# add plpython support if you need
./mason_packages/.link/bin/psql postgres -c "CREATE PROCEDURAL LANGUAGE 'plpythonu' HANDLER plpython_call_handler;"

# create postgis enabled db
./mason_packages/.link/bin/createdb template_postgis -T postgres
./mason_packages/.link/bin/psql template_postgis -c "CREATE EXTENSION postgis;"
./mason_packages/.link/bin/psql template_postgis -c "SELECT PostGIS_Full_Version();"
# load hstore, fuzzystrmatch, and unaccent extensions
./mason_packages/.link/bin/psql template_postgis -c "CREATE EXTENSION hstore;"
./mason_packages/.link/bin/psql template_postgis -c "CREATE EXTENSION fuzzystrmatch;"
./mason_packages/.link/bin/psql template_postgis -c "CREATE EXTENSION unaccent;"

echo "Fully bootstrapped template_postgis database is now ready"

# stop the database
./mason_packages/.link/bin/pg_ctl -w stop
