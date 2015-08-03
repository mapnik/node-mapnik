# node-mapnik

Bindings to [Mapnik](http://mapnik.org) for [node](http://nodejs.org).

[![NPM](https://nodei.co/npm/mapnik.png?downloads=true&downloadRank=true)](https://nodei.co/npm/mapnik/)

[![Build Status](https://secure.travis-ci.org/mapnik/node-mapnik.png)](https://travis-ci.org/mapnik/node-mapnik)
[![Build status](https://ci.appveyor.com/api/projects/status/ju29v1vcpif2iww8?svg=true)](https://ci.appveyor.com/project/Mapbox/node-mapnik)
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

* Node v0.10.x or v0.12.x (v0.12.x support requires node-mapnik >= v3.1.6)
* C++11 compatible C++ runtime library


## Troubleshooting

If you hit an error like:

    Error: /usr/lib/x86_64-linux-gnu/libstdc++.so.6: version `GLIBCXX_3.4.18' not found

This means your Linux distributions libstdc++ library is too old (for example you are running Ubuntu Precise rather than Trusty). To work around this upgrade libstdc++:

    sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
    sudo apt-get update -q
    sudo apt-get install -y libstdc++6

To upgrade libstdc++ on travis (without sudo) you can do:

```yaml
language: cpp

sudo: false

addons:
  apt:
    sources:
     - ubuntu-toolchain-r-test
    packages:
     - libstdc++6 # upgrade libstdc++ on linux to support C++11
```


## Installing

Just do:

    npm install mapnik@3.x

Note: This will install the latest node-mapnik 3.x series, which is recommended. There is also an [1.x series](https://github.com/mapnik/node-mapnik/tree/1.x) which maintains API compatibility with Mapnik 2.3.x and 2.2.x and a [v0.7.x series](https://github.com/mapnik/node-mapnik/tree/v0.7.x) which is not recommended unless you need to support Mapnik 2.1 or older.

By default, binaries are provided for:

 - 64 bit OS X 10.9, 64 bit Linux (>= Ubuntu Trusty), and 64/32 bit Windows
 - several node versions:
   - [versions forLinux/Mac](<https://github.com/mapnik/node-mapnik/blob/master/.travis.yml#L19-L47>)
   - [versions for Windows](<https://github.com/mapnik/node-mapnik/blob/master/appveyor.yml#L9-L32>)

On those platforms no external dependencies are needed.

Other platforms will fall back to a source compile: see [Source Build](#source-build) for details.

Binaries started being provided at node-mapnik >= 1.4.2 for OSX and Linux and at 1.4.8 for Windows.

### Windows specific

**NOTE:** Windows binaries for the **3.x** series require the Visual C++ Redistributable Packages for **Visual Studio 2015**:

  - <https://mapbox.s3.amazonaws.com/windows-builds/visual-studio-runtimes/vcredist-VS2015/vcredist_x64.exe>
  - <https://mapbox.s3.amazonaws.com/windows-builds/visual-studio-runtimes/vcredist-VS2015/vcredist_x86.exe>

See https://github.com/mapnik/node-mapnik/wiki/WindowsBinaries for more details.

The **1.x** series require the Visual C++ Redistributable Packages for **Visual Studio 2013**:

 - http://www.microsoft.com/en-us/download/details.aspx?id=40784


## Source Build

To build from source you need:

 - Mapnik >= v3.x
 - Protobuf >= 2.3.0 (protoc and libprotobuf-lite)

Install Mapnik using the instructions at: https://github.com/mapnik/mapnik/wiki/Mapnik-Installation

Confirm that the `mapnik-config` program is available and on your $PATH.

Then run:

    npm install mapnik --build-from-source

### Windows specific

Windows builds are maintained in https://github.com/mapbox/windows-builds


## Using node-mapnik from your node app

To require node-mapnik as a dependency of another package put in your package.json:

    "dependencies"  : { "mapnik":"*" } // replace * with a given semver version string

## Tests

To run the tests do:
  
    npm test

## License

  BSD, see LICENSE.txt
