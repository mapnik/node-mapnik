# node-mapnik

Bindings to [Mapnik](http://mapnik.org) for [node](http://nodejs.org).

## Usage

Render a map from a stylesheet:

```js
const fs = require('node:fs');
const mapnik = require('@mapnik/mapnik');

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
var mapnik = require('@mapnik/mapnik');
new mapnik.Image.open('input.jpg').save('output.png');
```

Convert a shapefile to GeoJSON:

```js
const fs = require('node:fs');
const path = require('node:path');
const mapnik = require('@mapnik/mapnik');

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

Starting from `v4.6.0`, `node-mapnik` module is published as "universal" binaries on [GitHub NPM registry](https://docs.github.com/en/packages/working-with-a-github-packages-registry/working-with-the-npm-registry) using[node-addon-api](https://github.com/nodejs/node-addon-api),
[node-gyp-build](https://github.com/prebuild/node-gyp-build) and [prebuildify](https://github.com/prebuild/prebuildify)

Currently supported platforms are

* `linux-x64`
* `darwin-x64`
* `darwin-arm64`


Consult N-API documentation for more details: https://nodejs.org/dist/latest/docs/api/n-api.html#n_api_node_api_version_matrix


## Installing

### With npm

Consult "[Installing a package](https://docs.github.com/en/packages/working-with-a-github-packages-registry/working-with-the-npm-registry#installing-a-package)
". You will need to authenticate to GitHub Packages, see "[Authenticating to GitHub Packages](https://docs.github.com/en/packages/working-with-a-github-packages-registry/working-with-the-npm-registry#authenticating-to-github-packages)."


    npm install @mapnik/mapnik

Note: This will install the latest node-mapnik 4.6.x series, which is recommended.


### Source Build [WIP]

On macOS and Linux:

* Ensure `mapnik-config` program is available and on your `${PATH}`.

* `npm install --build-from-source`

* To "prebuild" binaries `npm run prebuildify`

#### Note on SSE:

SSE support is enabled by default on `x86_64`.

## Using node-mapnik from your node app

To require node-mapnik as a dependency of another package put in your package.json:

    "dependencies"  : { "@mapnik/mapnik":"*" } // replace * with a given semver version string

## Tests

To run the tests do:

    npm test

## License

BSD, see LICENSE.txt
