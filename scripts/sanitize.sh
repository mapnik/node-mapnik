#!/usr/bin/env bash

set -eu
set -o pipefail

: '

Rebuilds the code with the sanitizers and runs the tests

'
 # Set up the environment by installing mason and clang++
 # See https://github.com/mapbox/node-cpp-skel/blob/master/docs/extended-tour.md#configuration-files
./scripts/setup.sh --config local.env
source local.env
make clean
export CXXFLAGS="${MASON_SANITIZE_CXXFLAGS} ${CXXFLAGS:-}"
export LDFLAGS="${MASON_SANITIZE_LDFLAGS} ${LDFLAGS:-}"
make debug
export ASAN_OPTIONS=fast_unwind_on_malloc=0:${ASAN_OPTIONS}
if [[ $(uname -s) == 'Darwin' ]]; then
    # NOTE: we must call node directly here rather than `npm test`
    # because OS X blocks `DYLD_INSERT_LIBRARIES` being inherited by sub shells
    # If this is not done right we'll see
    #   ==18464==ERROR: Interceptors are not working. This may be because AddressSanitizer is loaded too late (e.g. via dlopen).
    #
    DYLD_INSERT_LIBRARIES=${MASON_LLVM_RT_PRELOAD} \
      node node_modules/.bin/tape test/*test.js
else
    LD_PRELOAD=${MASON_LLVM_RT_PRELOAD} \
      npm test
fi

