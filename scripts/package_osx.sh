#!/bin/bash
set -u -e

CURRENT_DIR="$( cd "$( dirname $BASH_SOURCE )" && pwd )"
cd $CURRENT_DIR/../

rm -rf sdk
source ./scripts/build_against_sdk.sh
npm test
./node_modules/.bin/node-pre-gyp package testpackage --overwrite
npm test
./node_modules/.bin/node-pre-gyp publish info
rm -rf {build,lib/binding}
npm install --fallback-to-build=false
npm test
# now do node v0.8.x binaries
npm install --build-from-source --target=0.8.26
~/.nvm/v0.8.26/bin/npm test
~/.nvm/v0.8.26/bin/node ./node_modules/.bin/node-pre-gyp package testpackage --overwrite
~/.nvm/v0.8.26/bin/node ./node_modules/.bin/node-pre-gyp publish info
rm -rf {build,lib/binding}
~/.nvm/v0.8.26/bin/npm install --fallback-to-build=false
~/.nvm/v0.8.26/bin/npm test
