#!/bin/bash

set -eu
set -o pipefail

source mason-postgis-config.env && ./mason_packages/.link/bin/pg_ctl -w stop
