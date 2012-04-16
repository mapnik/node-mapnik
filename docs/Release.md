# Releasing

Steps to cut a new release of node-mapnik.


## Tests

All tests must pass if you run:

    make test

## Test examples

We just want to make sure none throw. Test this with:

   ./test/run_examples.sh


## Update Readme.md

Fix the minimum mapnik version required in the `Depends` section of the Readme.md.

Update any examples that may have broken.


## Increment versions

Increment version in `package.json`.


## Commit, tag, and push tag

    VERSION="0.7.0"
    git ci -a -m "bump to ${VERSION}"
    git push
    git tag ${VERSION} -m "tagging ${VERSION}"
    git push --tags


## Publish

First, remove all files/folder in your node-mapnik checkout that are not either checked into git or listed in .gitignore so that the files do not end up in the npm bundle.

Doing `npm publish` on the clean checkout works well for ensuring this is not a problem.

When you are ready to publish do:

    npm publish
    
Note: if you are not an owner ask one of the existing owners `npm owner ls mapnik` for publishing access.