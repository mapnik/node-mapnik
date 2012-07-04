
# Node-Mapnik
      
Bindings to [Mapnik](http://mapnik.org) for [node](http://nodejs.org).
  
```js
var mapnik = require('mapnik');
var http = require('http');

var port = 8000;
var stylesheet = './examples/stylesheet.xml';

http.createServer(function(req, res) {
  res.writeHead(500, {'Content-Type': 'text/plain'});
  var map = new mapnik.Map(256, 256);
  map.load(stylesheet,
    function(err,map) {
      if (err) {
          res.end(err.message);
      }
      map.zoomAll();
      var im = new mapnik.Image(256, 256);
      map.render(im, function(err,im) {
        if (err) {
            res.end(err.message);
        } else {
            im.encode('png', function(err,buffer) {
                if (err) {
                    res.end(err.message);
                } else {
                    res.writeHead(200, {'Content-Type': 'image/png'});
                    res.end(buffer);
                }
            });
        }
      });
   }
  );
}).listen(port);
```

Note: to run this example locally first do:

    export NODE_PATH=./lib:${NODE_PATH}

For more sample code see 'examples/README.md'


## Depends

* Node v0.6 or v0.8
* Mapnik 2.x

Mapnik 2.1.0 is targeted, but 2.0.x is also supported. This means that if you are running the Mapnik 2.0.x series some minor test failures are expected:

 * 7 test failures are expected using Mapnik 2.0.0
 * 5 test failures are expected using Mapnik 2.0.2
 * 0 test failures are expected using Mapnik 2.1.0-pre

## Installation

First install Mapnik master from github:

    git clone https://github.com/mapnik/mapnik.git

For more details see: https://github.com/mapnik/mapnik/wiki/Mapnik-Installation

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


## Quick rendering test

To see if things are working try rendering a world map with the sample data
  
From the source checkout root do:
  
    export NODE_PATH=./lib
    ./examples/simple/render.js ./examples/stylesheet.xml map.png


## Using node-mapnik from your node app

To require node-mapnik as a depedency of another package put in your package.json:

    "dependencies"  : { "mapnik":"*" } // replace * with a given semver version string

  
## Examples

See the 'examples/' folder for more usage examples.

For some of the tests you will need:

    npm install generic-pool
    npm install get


## Tests

To run the tests first install mocha:
  
    npm install mocha
  
Then run:
  
    make test


## License

  BSD, see LICENSE.txt