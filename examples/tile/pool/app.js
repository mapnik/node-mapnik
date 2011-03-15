#!/usr/bin/env node

var mapnik = require('mapnik')
  , mercator = require('mapnik/sphericalmercator')
  , mappool = require('mapnik/pool')
  , http = require('http')
  , url = require('url');
  
// create a pool of 10 maps
// this allows us to manage concurrency under high load
var maps = mappool.create(5);

var usage = 'usage: app.js <stylesheet> <port>';

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
    methods = {
        create: function(cb) {
                var obj = new mapnik.Map(options.width || 256, options.height || 256);
                // catch problem loading map
                try { obj.load(id) }
                catch (err) { callback(err, null) }
                if (options.buffer_size) obj.buffer_size(options.buffer_size);
                cb(obj);
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

var parseXYZ = function(req,callback) {
    matches = req.url.match(/(\d+)/g);
    if (matches && matches.length == 3) {
        try {
            callback(null,
               { z: parseInt(matches[0]),
                 x: parseInt(matches[1]),
                 y: parseInt(matches[2])
               });
        } catch (err) {
            callback(err,null);
        }
    } else {
          var query = url.parse(req.url, true).query;
          if (query &&
                query.x !== undefined &&
                query.y !== undefined &&
                query.z !== undefined) {
             try {
             callback(null,
               { z: parseInt(query.z),
                 x: parseInt(query.x),
                 y: parseInt(query.y)
               }
             );
             } catch (err) {
                 callback(err,null);
             }
          } else {
              callback("no x,y,z provided",null);
          }
    }
}

http.createServer(function(req, res) {
    parseXYZ(req,function(err,params) {
        if (err) {
            res.writeHead(500, {
              'Content-Type': 'text/plain'
            });
            res.end(err.message);
        } else {
            aquire(stylesheet, {},function(err,map) {
      
                if (err) {
                    res.writeHead(500, {
                      'Content-Type': 'text/plain'
                    });
                    res.end(err.message);
                } else {
                    // bbox for x,y,z
                    var bbox = mercator.xyz_to_envelope(params.x, params.y, params.z, false);
      
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
