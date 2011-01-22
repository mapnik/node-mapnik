
# Node-Mapnik
      
  Bindings to the [Mapnik](http://mapnik.org) tile rendering library for [node](http://nodejs.org).
  
    var mapnik = require('mapnik');
    var express = require('express');
    
    var app = express.createServer();
        
    app.get('/', function(req, res) {
      var map = new mapnik.Map(256,256);
      map.load("./examples/stylesheet.xml");
      map.zoom_all();
      res.contentType("tile.png");
      res.send(map.render_to_string());      
    });
    
    app.listen(8000);
  
  For more see 'examples/'


## Development Status
  
  Prototype at this point, API will be frequently changing.
  
  Developed on OS X (10.6)
  
  Also tested on Debian Squeeze and Centos 5.4.
  

## Depends

  node (development headers)
  
  mapnik (latest trunk)


## Installation
  
  Install node-mapnik:
  
    $ git clone git://github.com/mapnik/node-mapnik.git
    $ cd node-mapnik
    $ node-waf configure build install
    $ node test.js

  Make sure the node modules is on your path:
  
    export NODE_PATH=/usr/local/lib/node/
    
  For more details see 'docs/install.txt'

  Or you can install via npm:
  
    $ npm install mapnik


## Quick rendering test

  To see if things are working try rendering a world map with the sample data
  
  From the source checkout root do:
  
    $ examples/render.js examples/stylesheet.xml map.png

  
## Examples

  See the 'examples/' folder for more usage examples.


## In Action

  See https://github.com/tmcw/tilelive.js


## License

  BSD, see LICENSE.txt