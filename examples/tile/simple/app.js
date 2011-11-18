#!/usr/bin/env node

// This example shows how to use node-mapnik with the
// connect http server to serve map tiles to polymaps
// client. Also logs tile render speed
//
// expected output at zoom 0: http://goo.gl/cyGwo


var mapnik = require('mapnik'),
  mercator = require('mapnik/sphericalmercator'),
  connect = require('connect'),
  url = require('url'),
  fs = require('fs'),
  path = require('path'),
  port = 3000;


var stylesheet = path.join(__dirname, '../../stylesheet.xml');

var server = connect.createServer(

  connect.logger('\\033[90m:method\\033[0m \\033[36m:url\\033[0m \\033[90m:status :response-timems -> :res[Content-Type]\\033[0m'),
, connect.static(__dirname + '/public/')
, connect.router(function(app) {

    // Tile request url
    app.get('/:x/:y/:z', function(req, res, next) {

      try {
        // calculate the bounding box for each tile
        var bbox = mercator.xyz_to_envelope(parseInt(req.params.x, 10),
                                            parseInt(req.params.y, 10),
                                            parseInt(req.params.z, 10), false);

        // create map
        var map = new mapnik.Map(256, 256, mercator.proj4);
        map.load(stylesheet, {strict: true}, function(err, map) {

            // render map
            var im = new mapnik.Image(map.width, map.height);
            map.extent = bbox;
            map.render(im, function(err, im) {
              if (err) {
                throw err;
              } else {
                res.statusCode = 200;
                res.setHeader('Content-Type', 'image/png');
                res.end(im.encodeSync('png'));
              }
            });
        });
      } catch (err) {
        res.statusCode = 500;
        res.setHeader('Content-Type', 'text/plain');
        res.end(err.message);
      }
    });
  });
);

server.listen(port);
console.log("Server running on port " + port);
