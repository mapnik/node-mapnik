
var http = require('http');
var mapnik = require('mapnik');
var mappool = require('../utils/pool.js');
var url = require('url');

var port = 8000;
var pool_size = 10;

var usage = 'usage: wms.js <stylesheet> <port>';

var stylesheet = process.argv[2];

if (!stylesheet) {
   console.log(usage);
   process.exit(1);
}

var port = process.argv[3];

if (!port) {
   console.log(usage);
   process.exit(1);
}

var maps = mappool.create_pool(10);

var aquire = function(id,options,callback) {
    methods = {
        create: function(cb) {
                var obj = new mapnik.Map(options.width || 256, options.height || 256);
                obj.load(id, {strict: true},function(err,obj) {
                    if (err) callback(err, null);
                    if (options.bufferSize) {
                        obj.bufferSize = options.bufferSize;
                    }
                    cb(obj);
                });
            },
            destroy: function(obj) {
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
  if (query && query.bbox !== undefined) {

      var bbox = query.bbox.split(',');
      aquire(stylesheet, {}, function(err,map) {
          if (err) {
              res.end(err.message);
          } else {
              var im = new mapnik.Image(map.width, map.height);
              map.extent = bbox;
              map.render(im, function(err, im) {
                  maps.release(stylesheet, map);
                  if (err) {
                      res.end(err.message);
                  } else {
                      res.writeHead(200, {
                        'Content-Type': 'image/png'
                      });
                      res.end(im.encodeSync('png'));
                  }
              });
          }

      });
  } else {
      res.writeHead(200, {
        'Content-Type': 'text/html'
      });
      res.end('No BBOX provided! Try a request like <a href="http://127.0.0.1:' + port + '/?bbox=-20037508.34,-5009377.085697313,-5009377.08569731,15028131.25709193">this</a>');
  }
}).listen(port);


console.log('Server running at http://127.0.0.1:' + port + '/');
