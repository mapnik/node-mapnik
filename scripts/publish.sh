#!/bin/bash

function trigger_docs () {
  body="{
    \"request\": {
      \"message\": \"Triggered build: Mapnik core commit ${TRAVIS_COMMIT}\",
      \"branch\":\"master\"
    }
  }
  "

  curl -s -X POST \
    -H "Content-Type: application/json" \
    -H "Accept: application/json" \
    -H "Travis-API-Version: 3" \
    -H "Authorization: token ${TRAVIS_TRIGGER_TOKEN}" \
    -d "$body" \
    https://api.travis-ci.org/repo/mapnik%2Fdocumentation/requests
}

echo "dumping binary meta..."
./node_modules/.bin/node-pre-gyp reveal ${NPM_FLAGS}

echo "determining publishing status..."

if [[ $(./scripts/is_pr_merge.sh) ]]; then
    echo "Skipping publishing because this is a PR merge commit"
else
    echo "This is a push commit, continuing to package..."
    ./node_modules/.bin/node-pre-gyp package ${NPM_FLAGS}

    COMMIT_MESSAGE=$(git log --format=%B --no-merges | head -n 1 | tr -d '\n')
    echo "Commit message: ${COMMIT_MESSAGE}"

    if [[ ${COMMIT_MESSAGE} =~ "[publish binary]" ]]; then
        echo "Publishing"
        ./node_modules/.bin/node-pre-gyp publish ${NPM_FLAGS}
    elif [[ ${COMMIT_MESSAGE} =~ "[republish binary]" ]]; then
        echo "Re-Publishing"
        ./node_modules/.bin/node-pre-gyp unpublish publish ${NPM_FLAGS}
    else
        echo "Skipping publishing"
    fi;

    # only publish docs from a single build environment which has DOC_JOB set
    if [[ ${COMMIT_MESSAGE} =~ "[publish docs]" ]] && [[ ${DOC_JOB:-} == "true" ]]; then
        echo "Publishing docs"
        trigger_docs
    else
        echo "Skipping publishing docs."
    fi;
fi