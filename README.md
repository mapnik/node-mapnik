# node-mapnik

Bindings to [Mapnik](http://mapnik.org) for [node](http://nodejs.org).

[![NPM](https://nodei.co/npm/mapnik.png?downloads=true&downloadRank=true)](https://nodei.co/npm/mapnik/)

[![Build Status](https://secure.travis-ci.org/mapnik/node-mapnik.png)](https://travis-ci.org/mapnik/node-mapnik)
[![Coverage Status](https://coveralls.io/repos/mapnik/node-mapnik/badge.svg)](https://coveralls.io/r/mapnik/node-mapnik)

## Usage

Render a map from a stylesheet:

```js
var mapnik = require('mapnik');
var fs = require('fs');

// register fonts and datasource plugins
mapnik.register_default_fonts();
mapnik.register_default_input_plugins();

var map = new mapnik.Map(256, 256);
map.load('./test/stylesheet.xml', function(err,map) {
    if (err) throw err;
    map.zoomAll();
    var im = new mapnik.Image(256, 256);
    map.render(im, function(err,im) {
      if (err) throw err;
      im.encode('png', function(err,buffer) {
          if (err) throw err;
          fs.writeFile('map.png',buffer, function(err) {
              if (err) throw err;
              console.log('saved map image to map.png');
          });
      });
    });
});
```

Convert a jpeg image to a png:

```js
var mapnik = require('mapnik');
new mapnik.Image.open('input.jpg').save('output.png');
```

Convert a shapefile to GeoJSON:

```js
var mapnik = require('mapnik');
mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'shape.input'));
var ds = new mapnik.Datasource({type:'shape',file:'test/data/world_merc.shp'});
var featureset = ds.featureset()
var geojson = {
  "type": "FeatureCollection",
  "features": [
  ]
}
var feat = featureset.next();
while (feat) {
    geojson.features.push(JSON.parse(feat.toJSON()));
    feat = featureset.next();
}
fs.writeFileSync("output.geojson",JSON.stringify(geojson,null,2));
```

For more sample code see [the tests](./test) and [sample code](https://github.com/mapnik/node-mapnik-sample-code).

## Depends

OS|Node.js|C++ minimum requirements|OS versions
---|---|---|---
Mac|v0.10.x, v4, v6, v8|C++11|Mac OS X > 10.10
Linux|v0.10.x, v4, v6, v8|C++11|Ubuntu Linux > 16.04 or other Linux distributions with g++ >= 5 toolchain (>= GLIBCXX_3.4.21 from libstdc++)
Windows|v0.10.x, v4, v6, v8|C++11|See the [Windows requirements](https://github.com/mapnik/node-mapnik#windows-specific) section

An installation error like below indicates your system does not have a modern enough libstdc++/gcc-base toolchain:

```
Error: /usr/lib/x86_64-linux-gnu/libstdc++.so.6: version GLIBCXX_3.4.21 not found (required by /node_modules/osrm/lib/binding/osrm.node)
```

If you are running Ubuntu older than 16.04 you can easily upgrade your libstdc++ version like:

```
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get update -y
sudo apt-get install -y libstdc++-5-dev
```

To upgrade libstdc++ on travis (without sudo) you can do:

```yaml
language: cpp

sudo: false

addons:
  apt:
    sources:
     - ubuntu-toolchain-r-test
    packages:
     - libstdc++-5-dev # upgrade libstdc++ on linux to support C++11
```


## Installing
### With npm
Just do:

    npm install mapnik

Note: This will install the latest node-mapnik 3.x series, which is recommended. There is also an [1.x series](https://github.com/mapnik/node-mapnik/tree/1.x) which maintains API compatibility with Mapnik 2.3.x and 2.2.x and a [v0.7.x series](https://github.com/mapnik/node-mapnik/tree/v0.7.x) which is not recommended unless you need to support Mapnik 2.1 or older.

By default, binaries are provided for:

 - 64 bit OS X >= 10.10, 64 bit Linux (>= Ubuntu Trusty)
 - several node versions:
   - [versions for Linux/Mac](<https://github.com/mapnik/node-mapnik/blob/master/.travis.yml#L19-L47>)

On those platforms no external dependencies are needed.

Other platforms will fall back to a source compile: see [Source Build](#source-build) for details.

Binaries started being provided at node-mapnik >= 1.4.2 for OSX and Linux and at 1.4.8 for Windows. After 3.6.2 no Windows binaries are provided.

### Windows specific

**NOTE:** Windows binaries for the **3.x** series require the Visual C++ Redistributable Packages for **Visual Studio 2015**:

  - <https://mapbox.s3.amazonaws.com/windows-builds/visual-studio-runtimes/vcredist-VS2015/vcredist_x64.exe>
  - <https://mapbox.s3.amazonaws.com/windows-builds/visual-studio-runtimes/vcredist-VS2015/vcredist_x86.exe>

See https://github.com/mapnik/node-mapnik/wiki/WindowsBinaries for more details.

The **1.x** series require the Visual C++ Redistributable Packages for **Visual Studio 2013**:

 - http://www.microsoft.com/en-us/download/details.aspx?id=40784


## Source Build

There are two ways to build from source. These work on both OS X and Linux:

 - A) Against a binary package from Mapnik from [mason](https://github.com/mapbox/mason)
 - B) Against an existing version of Mapnik on your system

Using `A)` is recommended. You do not need to have Mapnik installed already, so this is the easiest and most predictable approach. When you use the route a binary package of Mapnik is downloaded dynamically from [mason](https://github.com/mapbox/mason).

You can invoke this method simply by running:

  `make release`

Or, for debug builds:

  `make debug`

If you want to do a full rebuild do:

  `make distclean`

And then re-run the build:

  `make release`

Using `B)` is also possible, if you would like to build node-mapnik against an external, already installed Mapnik version.

In this case you need to have a Mapnik version installed that is at least as recent as the `mapnik_version` property in the [`package.json`](./package.json) for the branch of node-mapnik you want to build.

And you need to have the `mapnik-config` program is available and on your `${PATH}`.

Then run (within the cloned `node-mapnik` directory:

    make release_base

#### Note on SSE:

By default node mapnik is built with SSE support. If you are building on a platform that is not `x86_64` you will need to disable feature by setting the environment variable `SSE_MATH=false`.

```
SSE_MATH=false make
```

### Building against Mapnik 3.0.x

The `master` branch of node-mapnik is not compatible with `3.0.x` series of Mapnik. To build against Mapnik 3.0.x, use [`v3.0.x`](https://github.com/mapnik/node-mapnik/tree/v3.0.x) branch.


## Using node-mapnik from your node app

To require node-mapnik as a dependency of another package put in your package.json:

    "dependencies"  : { "mapnik":"*" } // replace * with a given semver version string

## Tests

To run the tests do:
  
    npm test

## License

BSD, see LICENSE.txt
