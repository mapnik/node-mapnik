#!/usr/bin/env node

// This example shows how to use node-mapnik with the 
// connect http server to serve map tiles to protovis
// client
//
// expected output at zoom 0: http://goo.gl/cyGwo


var mapnik = require('mapnik')
  , mercator = require('mapnik/sphericalmercator')
  , connect = require('connect')
  , url = require('url')
  , fs = require('fs')
  , path = require('path')
  , port = 3000;


var server = connect.createServer(  
  
  connect.logger('\033[90m:method\033[0m \033[36m:url\033[0m \033[90m:status :response-timems -> :res[Content-Type]\033[0m')  
, connect.static(__dirname + '/public/')
, connect.router(function(app){
    
    // Tile request url
    app.get('/:x/:y/:z', function(req, res, next){      

      try {
        // calculate the bounding box for each tile
        var bbox = mercator.xyz_to_envelope(parseInt(req.params.x),
                                            parseInt(req.params.y),
                                            parseInt(req.params.z), false);
      
        // create map
        var map = new mapnik.Map(256, 256, mercator.srs);
        map.load(path.join(__dirname, '../../stylesheet.xml'));
        map.zoom_all();
                  
        // render map
        map.render(bbox, 'png', function(err, buffer) {
          if (err) {
            throw err;
          } else {
            res.statusCode = 200;
            res.setHeader('Content-Type', 'image/png');        
            res.end(buffer);            
          }
        });
      }
      catch (err) {        
        res.statusCode = 500;
        res.setHeader('Content-Type', 'text/plain');        
        res.end(err.message);
      }
    });
  })  
);

server.listen(port);
console.log("Server running on port " + port);