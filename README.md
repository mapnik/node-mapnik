
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

* Node >= v0.6.0
* Mapnik 2.1-dev (current master): (at least [9e397ae55](https://github.com/mapnik/mapnik/commit/9e397ae55e37e0e23477e523d03084790e23e264) / March 1, 2012)


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

Then, if you have expresso installed you can run the tests:

    make test

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

    "dependencies"  : { "mapnik":"0.6.x" }

  
## Examples

See the 'examples/' folder for more usage examples.

For some of the tests you will need:

    npm install express
    npm install generic-pool
    npm install get


## Tests

To run the expresso tests first install expresso.
  
    npm install expresso
  
Then run:
  
    make test


## License

  BSD, see LICENSE.txt