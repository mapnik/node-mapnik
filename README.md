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

## Requirements

Starting from `v4.5.0`, `node-mapnik` module is published as "universal" binaries using [node-addon-api](https://github.com/nodejs/node-addon-api)
All Node.js versions >= 10 are known to work, consult N-API documentation for more details: https://nodejs.org/dist/latest/docs/api/n-api.html#n_api_node_api_version_matrix

An installation error like below indicates your system does not have a modern enough libstdc++/gcc-base toolchain:

```
Error: /usr/lib/x86_64-linux-gnu/libstdc++.so.6: version GLIBCXX_3.4.21 not found (required by /node_modules/mapnik/lib/binding/mapnik.node)
```

If you are running Ubuntu older than 16.04 you can easily upgrade your libstdc++ version like:

```
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get update -y
sudo apt-get install -y libstdc++-6-dev
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
     - libstdc++-6-dev # upgrade libstdc++
```


## Installing
### With npm
Just do:

    npm install mapnik

Note: This will install the latest node-mapnik 4.5.x series, which is recommended.

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
    
or
    
    make debug_base
    
for release and debug builds, respectively.

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
