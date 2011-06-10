#!/usr/bin/env node

var http = require('http');
var mapnik = require('mapnik');
var mappool = require('mapnik/pool');
var url = require('url');

var port = 8000;
var pool_size = 10;

var usage = 'usage: wms.js <stylesheet> <port>';

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

var maps = mappool.create(10);

var aquire = function(id,options,callback) {
    methods = {
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
  var query = url.parse(req.url.toLowerCase(), true).query;
  res.writeHead(500, {
    'Content-Type': 'text/plain'
  });
  if (query &&
      (query.bbox !== undefined) &&
      (query.width !== undefined) &&
      (query.height !== undefined)) {

      var bbox = query.bbox.split(',');
      var options = {width: parseInt(query.width), height: parseInt(query.height)};
      options.size = 50;
      aquire(stylesheet, {}, function(err,map) {
          if (err) {
              res.end(err.message);
          } else {
              map.render(bbox, 'png', function(err, buffer) {
                  maps.release(stylesheet, map);
                  if (err) {
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
  else {
    res.end('something was not provided!');
  }
}).listen(port);


console.log('Server running at http://127.0.0.1:' + port + '/');
