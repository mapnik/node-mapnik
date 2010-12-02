#!/usr/bin/env node

var http = require('http');
var mapnik = require('mapnik');
var url = require('url');

var port = 8000;
var pool_size = 8;
var render_pool = [];

var usage = 'usage: wms.js <stylesheet>';

var stylesheet = process.ARGV[2];

if (!stylesheet) {
   console.log(usage);
   process.exit(1);
}

for(i=0;i<pool_size;i++) {
    var map = new mapnik.Map(256,256);
    map.load(stylesheet);
    console.log('adding new map to pool: '+ i)
    map.buffer_size(128);
    render_pool[i] = map;
}

var next_map_ = 0;

function get_map()
{
  //console.log('pulling map #'+next_map_+' from map pool');
  var map = render_pool[next_map_];
  ++next_map_;
  if (next_map_ == pool_size)
    next_map_ = 0;
  return map;
}

http.createServer(function (request, response) {
  var query = url.parse(request.url,true).query;
  if (query && query.BBOX !== undefined){
      var bbox = query.BBOX.split(',');
      response.writeHead(200, {'Content-Type': 'image/png'});
      response.end(get_map().render_to_string("png"));
      /*get_map().render(bbox,function(image){
                 //console.log(image);
                 response.end(image);
      });
      */ 
  } else {
      response.writeHead(200, {
        'Content-Type':'text/plain'
      });
      response.end("No BBOX provided!");
  }
}).listen(port);

console.log('Server running at http://127.0.0.1:' + port + '/');