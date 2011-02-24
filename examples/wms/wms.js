#!/usr/bin/env node

var http = require('http');
var mapnik = require('mapnik');
var url = require('url');

var port = 8000;
var pool_size = 10;
var render_pool = [];
var async_render = false;
var use_map_pool = true;


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


for (i = 0; i < pool_size; i++) {
    var map = new mapnik.Map(256, 256);
    map.load(stylesheet);
    console.log('adding new map to pool: ' + i);
    map.buffer_size(128);
    render_pool[i] = map;
}

var next_map_ = 0;

// http://journal.paul.querna.org/articles/2010/09/04/limiting-concurrency-node-js/
function get_map()
{
  //console.log('pulling map #'+next_map_+' from map pool');
  var map = render_pool[next_map_];
  ++next_map_;
  if (next_map_ == pool_size)
    next_map_ = 0;
  return map;
}

http.createServer(function(request, response) {
  var query = url.parse(request.url, true).query;
  if (query && query.BBOX !== undefined) {
      var bbox = query.BBOX.split(',');
      response.writeHead(200, {'Content-Type': 'image/png'});
      var map;

      if (use_map_pool) {
          map = get_map();
      }
      else {
          map = new mapnik.Map(256, 256);
          map.load(stylesheet);
          map.buffer_size(128);
      }

      if (query.width !== undefined && query.height !== undefined) {
          map.resize(parseInt(query.width), parseInt(query.height));
      }

      if (async_render) {
          map.render(bbox, 'png', function(err, image) {
              if (err) {
                  response.writeHead(500, {
                    'Content-Type': 'text/plain'
                  });
                  response.end(err.message);
              } else {
                  response.end(image);
              }
          });
      }
      else {
          map.zoom_to_box(bbox);
          response.end(map.render_to_string('png'));
      }

  } else {
      response.writeHead(200, {
        'Content-Type': 'text/plain'
      });
      response.end('No BBOX provided!');
  }
}).listen(port);


console.log('Server running at http://127.0.0.1:' + port + '/');
