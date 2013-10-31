
# Node-Mapnik
      
Bindings to [Mapnik](http://mapnik.org) for [node](http://nodejs.org).


## Example

Render a map synchronously:

```js
var mapnik = require('mapnik');

var map = new mapnik.Map(256, 256);
map.loadSync('./examples/stylesheet.xml');
map.zoomAll();
map.renderFileSync('map.png');
```

Render a map asynchronously:

```js
var mapnik = require('mapnik');
var fs = require('fs');

var map = new mapnik.Map(256, 256);
map.load('./examples/stylesheet.xml', function(err,map) {
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

For more sample code see https://github.com/mapnik/node-mapnik-sample-code


## Depends

* Node >v0.6.13 and < v0.11
* Mapnik 2.x

Mapnik 2.2.x is targeted, but 2.1.x and 2.0.x are also supported.

This means that if you are running the Mapnik 2.2 series (current unreleased master) you must be running at least [0eddc2b5a0](https://github.com/mapnik/mapnik/commit/eebc8cc73eb18903b07e3b3e0757c11925962124).

This means that if you are running the Mapnik 2.0.x series some minor test failures are expected:

 * 6-7 test failures are expected using Mapnik 2.0.0
 * 4-5 test failures are expected using Mapnik 2.0.2
 * 0 test failures are expected using Mapnik 2.1.0-pre

## Installation

First install Mapnik - for more details see: https://github.com/mapnik/mapnik/wiki/Mapnik-Installation

Confirm that the `mapnik-config` program is available and on your $PATH.

To install node-mapnik locally for development or testing do:

    git clone git://github.com/mapnik/node-mapnik.git
    cd node-mapnik
    npm install

Set NODE_PATH to test importing:

    export NODE_PATH=./lib
    node -e "require.resolve('mapnik')"

Or you can also install via npm
  
    npm install mapnik

The above will install node-mapnik locally in a node_modules folder. To install globally do:

    npm install -g mapnik

## Using node-mapnik from your node app

To require node-mapnik as a dependency of another package put in your package.json:

    "dependencies"  : { "mapnik":"*" } // replace * with a given semver version string

## Tests

To run the tests do:
  
    npm test

## License

  BSD, see LICENSE.txt