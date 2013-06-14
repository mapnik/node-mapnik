
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

* Node >= v0.6
* Mapnik v2.2.x
* Protobuf (protoc and libprotobuf-lite)

## Installation

Install Mapnik using the instructions at: https://github.com/mapnik/mapnik/wiki/Mapnik-Installation

Confirm that the `mapnik-config` program is available and on your $PATH.

To install node-mapnik locally for development or testing do:

    git clone git://github.com/mapnik/node-mapnik.git
    cd node-mapnik
    ./configure
    make

Or set NODE_PATH to test importing:

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

Or you can manually install mocha and then just do:

    make test

## License

  BSD, see LICENSE.txt