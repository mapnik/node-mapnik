#!/bin/bash
set -e

CURRENT_DIR="$( cd "$( dirname $BASH_SOURCE )" && pwd )"
cd $CURRENT_DIR/../
source ~/.nvm/nvm.sh
nvm install 0.10
nvm use 0.10
npm install node-pre-gyp
npm install aws-sdk
./node_modules/.bin/node-pre-gyp info
npm cache clean
rm -rf sdk

function doit () {
    NVER=$1
    nvm install $1
    nvm use $1
    source ./scripts/build_against_sdk.sh --target=$1
    npm test
    node ./node_modules/.bin/node-pre-gyp package testpackage
    node ./node_modules/.bin/node-pre-gyp testpackage
    npm ls
    node ./node_modules/.bin/node-pre-gyp publish
    node ./node_modules/.bin/node-pre-gyp info
    rm -rf {build,lib/binding}
    npm install --fallback-to-build=false
    npm test
}

#doit 0.8.26
doit 0.10.33
doit 0.11.14

# to avoid then publishing with node v0.11.x
# https://github.com/npm/npm/issues/5515#issuecomment-46688278
nvm use 0.10
