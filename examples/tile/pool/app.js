#!/usr/bin/env node

// This example shows how to use node-mapnik with the 
// connect http server to serve map tiles to polymaps
// client. Also logs tile render speed
//
// expected output at zoom 0: http://goo.gl/cyGwo

var mapnik = require('mapnik')
  , mercator = require('mapnik/sphericalmercator')
  , mappool = require('mapnik/pool')
  , http = require('http')
  , util = require('../lib/utility.js');

var TMS_SCHEME = false;

// create a pool of 5 maps to manage concurrency under load
var maps = mappool.create(5);

var usage = 'usage: app.js <stylesheet> <port>\ndemo:  app.js ../../stylesheet.xml 8000';

var stylesheet = process.ARGV[2];

if (!stylesheet) {
   console.log(usage);
   process.exit(1);
}

var port = process.ARGV[3];

if (!port) {
   console.log(usage);
   process.exit(1);
}

var aquire = function(id,options,callback) {
    var methods = {
        create: function(cb) {
                var obj = new mapnik.Map(options.width || 256, options.height || 256);
                obj.load(id,{strict:true},function(err,obj) {
                    if (err) callback(err,null);
                    if (options.buffer_size) obj.buffer_size(options.buffer_size);
                    cb(obj)
                })
            },
            destroy: function(obj) {
                obj.clear();
                delete obj;
            }
    };
    maps.acquire(id,
                  methods,
                  function(obj) {
                    callback(null, obj);
                  }
   );
};


http.createServer(function(req, res) {
    util.parseXYZ(req, TMS_SCHEME, function(err,params) {
        if (err) {
            res.writeHead(500, {
              'Content-Type': 'text/plain'
            });
            res.end(err.message);
        } else {
            aquire(stylesheet, {}, function(err, map) {
                if (err) {
                    res.writeHead(500, {
                      'Content-Type': 'text/plain'
                    });
                    res.end(err.message);
                } else {
                    // bbox for x,y,z
                    var bbox = mercator.xyz_to_envelope(params.x, params.y, params.z, TMS_SCHEME);
      
                    map.render(bbox, 'png', function(err, buffer) {
                        maps.release(stylesheet, map);
                        if (err) {
                            res.writeHead(500, {
                              'Content-Type': 'text/plain'
                            });
                            res.end(err.message);
                        } else {
                            res.writeHead(200, {
                              'Content-Type': 'image/png'
                            });
                            res.end(buffer);
                        }
                    });
                }
            });
        }
    })

}).listen(port);


console.log('Test server listening on port %d', port);
