#!/usr/bin/env bash
set -eu
set -o pipefail
MODULE_PATH=$(node -e "console.log(path.dirname(require('node-gyp-build').path()))")

echo "MODULE_PATH:${MODULE_PATH} [WIP]"
