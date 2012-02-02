
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

For more sample code see 'examples/'


## Depends

* Node >= v0.4.x
* Mapnik 2.1-dev (current master): (at least [57cf7d54](https://github.com/mapnik/mapnik/commit/57cf7d54cf73a7734a62836759d9d7721340ffc6) / February 1, 2012)


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
  
    ./examples/simple/render.js ./examples/stylesheet.xml map.png


## Using node-mapnik from your node app

To require node-mapnik as a depedency of another package put in your package.json:

    "dependencies"  : { "mapnik":"0.5.x" }

  
## Examples

See the 'examples/' folder for more usage examples.


## Tests

To run the expresso tests first install expresso and step.
  
    npm install -g expresso
    npm install -g step
  
Then run:
  
    make test


## License

  BSD, see LICENSE.txt