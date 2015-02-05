# Contributing

General guidelines for contributing to node-mapnik

## Releasing

To release a new node-mapnik version:

### Create a branch and publish binaries

**1)** Create a branch for your work

**2)** Updating the Mapnik SDK used for binaries

If your node-mapnik release requires a new Mapnik version, then a new Mapnik SDK would need to be published first.

To get a new Mapnik SDK published for Unix:

  - The Mapnik master branch must be production ready.
  - Or you can build an SDK from a testing branch. In this case the `MAPNIK_BRANCH` value [here](https://github.com/mapnik/mapnik-packaging/blob/master/.travis.yml#L8) can be edited.
  - Next you would commit to the `mapnik-packaging` repo with the message of `-m "[publish]"`.
  - Watch the end of the travis logs to get the url of the new SDK (https://travis-ci.org/mapnik/mapnik-packaging/builds/47075304#L4362)

Then, back at node-mapnik, take the new SDK version and edit [these lines](https://github.com/mapnik/node-mapnik/blob/fee02eb4c76d171cbaacd6b7b54afb48460868bc/scripts/build_against_sdk.sh#L111-L119) with the new value. Test that a build works before pushing to travis branch.

To get a new Mapnik SDK published for Windows:

  - The scripts at https://github.com/BergWerkGIS/mapnik-dependencies are used
  - The do not run fast enough to work on appveyor so we plan aws automation to get this working.
  - In the meantime ping @BergWerkGIS to run them and provide a new SDK.

Then, back at node-mapnik, take the new SDK version and edit [this lines](https://github.com/mapnik/node-mapnik/blob/fee02eb4c76d171cbaacd6b7b54afb48460868bc/appveyor.yml#L42) with the new value. Test that a build works before pushing to appveyor branch.

**3)** Make sure all tests are passing on travis and appveyor for your branch. Check the links at https://github.com/mapnik/node-mapnik/blob/master/README.md#node-mapnik.

**4)** Within your branch edit the `version` value in `package.json` to something unique that will not clash with existing released versions. For example if the current release (check this via https://github.com/mapnik/node-mapnik/releases) is `3.1.4` then you could rename your `version` to `3.1.5-alpha` or `3.1.5-branchname`.

**5)** Commit the new version and publish binaries

Do this like:

```sh
git commit package.json -m "bump to v3.1.5-alpha [publish binary]"
```

What if you already committed the `package.json` bump and you have no changes to commit but want to publish binaries. In this case you can do:

```sh
git commit --allow-empty -m "[publish binary]"
```

Note: it is not possible to overwrite previously published binaries. So don't run `"[publish binary]"` unless you really mean it!

**6)** Test your binaries

Lots of ways to do this of course. Just updating a single dependency of node-mapnik is a good place to start: https://www.npmjs.com/browse/depended/mapnik

But the ideal way is to test a lot at once: enter mapnik-swoop.

### mapnik-swoop

Head over to https://github.com/mapbox/mapnik-swoop and create a branch that points [the package.json](https://github.com/mapbox/mapnik-swoop/blob/master/package.json#L14) at your working node-mapnik version.

Ensure that all tests are passing. Only ignore failing tests for dependencies if you can confirm with the downstream maintainers of the modules that those tests are okay to fail and unrelated to your node-mapnik changes. You can check [recent builds](https://travis-ci.org/mapbox/mapnik-swoop/builds) to see if all builds were green and passing before your change. If they were red and failing before then try to resolve those issues before testing your new node-mapnik version.

**7)** Official release

An official release requires:

 - Updating the CHANGELOG.md
 - Publishing new binaries for a non-alpha version like `3.1.5`. So you'd want to merge your branch and then edit the `version` value in package json back to a decent value for release.
 - Create a github tag like `git tag 3.1.5 -m "v3.1.5"`
 - Test mapnik-swoop again for your new tagged version
 - Ensure you have a clean checkout (no extra files in your check that are not known by git). You need to be careful, for instance, to avoid a large accidental file being packaged by npm. You can get a view of what npm will publish by running `make testpack`
 - Then publish the module to npm repositories by running `npm publish`