#!/bin/bash

set -eu
set -o pipefail
shopt -s nullglob

: '

Ensure no GLIBCXX_3.4.2x symbols are present in the binary

If symbols >= 3.4.20 then it returns error code 1. This means
the binaries would not run on ubuntu trusty without upgrading libstdc++

'

FINAL_RETURN_CODE=0

function check() {
    local RESULT=0
    nm ${1} | grep "GLIBCXX_3.4.2[0-9]" > /tmp/out.txt || RESULT=$?
    if [[ ${RESULT} != 0 ]]; then
        echo "Success: GLIBCXX_3.4.2[0-9] symbols not present in binary (as expected)"
    else
        echo "$(cat /tmp/out.txt | c++filt)"
        FINAL_RETURN_CODE=1
    fi
}


echo "checking ./lib/binding/mapnik.node"
check "lib/binding/mapnik.node"

exit ${FINAL_RETURN_CODE}
