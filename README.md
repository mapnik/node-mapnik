# node-mapnik

Bindings to [Mapnik](http://mapnik.org) for [node](http://nodejs.org).

[![NPM](https://nodei.co/npm/mapnik.png)](https://nodei.co/npm/mapnik/)

[![Build Status](https://secure.travis-ci.org/mapnik/node-mapnik.png)](https://travis-ci.org/mapnik/node-mapnik)

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
````

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

* Node v0.10.x or v0.8.x

## Installing

By default, binaries are provided for:

 - 64 bit OS X and 64 bit Linux
 - Node v0.8.x and v0.10.x

On those platforms no external dependencies are needed.

Just do:

    npm install mapnik

However other platforms will fall back to a source compile: see [Source Build](#source-build) for details.

## Source Build

To build from source you need:

 - Mapnik >= v2.2.x
 - Protobuf >= 2.3.0 (protoc and libprotobuf-lite)

To build with OS X Mavericks you need to ensure the bindings link to libc++. An easy way to do this is to set:

    export CXXFLAGS=-mmacosx-version-min=10.9

Install Mapnik using the instructions at: https://github.com/mapnik/mapnik/wiki/Mapnik-Installation

Confirm that the `mapnik-config` program is available and on your $PATH.

Then run:

    npm install mapnik --build-from-source

## Using node-mapnik from your node app

To require node-mapnik as a dependency of another package put in your package.json:

    "dependencies"  : { "mapnik":"*" } // replace * with a given semver version string

## Tests

To run the tests do:
  
    npm test

## License

  BSD, see LICENSE.txt
