#!/bin/bash

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
fi
