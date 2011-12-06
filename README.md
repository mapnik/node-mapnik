
# Node-Mapnik
      
  Bindings to the [Mapnik](http://mapnik.org) tile rendering library for [node](http://nodejs.org).
  
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

  For more see 'examples/'


## Development Status
  
  Prototype at this point, API will be frequently changing.
  
  Developed on OS X (10.6)
  
  Tested on Debian Squeeze, Ubuntu Maverick/Natty, and Centos 5.4.
  

## Depends

  node (development headers) >= v0.2.3
  
  mapnik 2.1 (current master): (at least [9e924bfa11](https://github.com/mapnik/mapnik/commit/9e924bfa1169b213ed31689b3c1e700251bb7d44))
  
  node-pool for some examples (npm install -g generic-pool)
 
  node-sphericalmercator for some examples (npm install -g sphericalmercator)
  
  expresso and >= node v0.4.x for tests (npm install expresso)
  
  npm >= 1.0 (if you use npm to install deps)


## Installation
  
  Install node-mapnik:
  
    $ git clone git://github.com/mapnik/node-mapnik.git
    $ cd node-mapnik
    $ ./configure
    $ make
    $ sudo make install

  For more details see 'docs/install.txt'

  Or you can also install via npm:
  
    $ npm install -g mapnik


## Quick rendering test

  To see if things are working try rendering a world map with the sample data
  
  From the source checkout root do:
  
    $ ./examples/simple/render.js ./examples/stylesheet.xml map.png

  
## Examples

  See the 'examples/' folder for more usage examples.


## Tests

  To run the expresso tests first install expresso and step.
  
    $ npm install -g expresso
    $ npm install -g step
  
  Then run:
  
    $ make test


## License

  BSD, see LICENSE.txt