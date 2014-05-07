#!/bin/bash
set -u -e

CURRENT_DIR="$( cd "$( dirname $BASH_SOURCE )" && pwd )"
cd $CURRENT_DIR/../
npm install aws-sdk
rm -rf sdk
source ./scripts/build_against_sdk.sh
npm test
./node_modules/.bin/node-pre-gyp package testpackage
npm test
./node_modules/.bin/node-pre-gyp publish info
rm -rf {build,lib/binding}
npm install --fallback-to-build=false
npm test
# now do node v0.8.x binaries
source ./scripts/build_against_sdk.sh --target=0.8.26
~/.nvm/v0.8.26/bin/npm test
~/.nvm/v0.8.26/bin/node ./node_modules/.bin/node-pre-gyp package testpackage
~/.nvm/v0.8.26/bin/node ./node_modules/.bin/node-pre-gyp publish info
rm -rf {build,lib/binding}
~/.nvm/v0.8.26/bin/npm install --fallback-to-build=false
~/.nvm/v0.8.26/bin/npm test
