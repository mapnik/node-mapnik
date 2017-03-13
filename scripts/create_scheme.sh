#!/usr/bin/env bash

set -eu
set -o pipefail

CONTAINER=build/binding.xcodeproj
OUTPUT="${CONTAINER}/xcshareddata/xcschemes/${SCHEME_NAME}.xcscheme"

# Required ENV vars:
# - SCHEME_TYPE: type of the scheme
# - SCHEME_NAME: name of the scheme

# Optional ENV vars:
# - NODE_ARGUMENT (defaults to "")
# - BUILDABLE_NAME (defaults ot SCHEME_NAME)
# - BLUEPRINT_NAME (defaults ot SCHEME_NAME)


# Try to reuse the existing Blueprint ID if the scheme already exists.
if [ -f "${OUTPUT}" ]; then
    BLUEPRINT_ID=$(sed -n "s/[ \t]*BlueprintIdentifier *= *\"\([A-Z0-9]\{24\}\)\"/\\1/p" "${OUTPUT}" | head -1)
fi

NODE_ARGUMENT=${NODE_ARGUMENT:-}
BLUEPRINT_ID=${BLUEPRINT_ID:-$(hexdump -n 12 -v -e '/1 "%02X"' /dev/urandom)}
BUILDABLE_NAME=${BUILDABLE_NAME:-${SCHEME_NAME}}
BLUEPRINT_NAME=${BLUEPRINT_NAME:-${SCHEME_NAME}}

mkdir -p "${CONTAINER}/xcshareddata/xcschemes"

sed "\
s#{{BLUEPRINT_ID}}#${BLUEPRINT_ID}#;\
s#{{BLUEPRINT_NAME}}#${BLUEPRINT_NAME}#;\
s#{{BUILDABLE_NAME}}#${BUILDABLE_NAME}#;\
s#{{CONTAINER}}#${CONTAINER}#;\
s#{{WORKING_DIRECTORY}}#$(pwd)#;\
s#{{NODE_PATH}}#$(dirname `which node`)#;\
s#{{NODE_ARGUMENT}}#${NODE_ARGUMENT}#" \
    scripts/${SCHEME_TYPE}.xcscheme > "${OUTPUT}"
