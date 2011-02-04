#!/usr/bin/env node

var http = require('http');
var mapnik = require('mapnik');
var maps = require('./mappool');
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

//maps.size = 60;
var aquire = function(xml,options,callback) {
      try {
          maps.acquire(
              xml,
              options,
              function(map) {
                  if (map.width() !== options.width || 
                      map.height() !== options.height) {
                        map.resize(options.width, options.height);
                  }
                  callback(null, map);
              }
          );
      }
      catch (err) {
          callback(err,null);
      }
}

var release = function(stylesheet,map) {
    maps.release(
        stylesheet,
        map
    );
}


http.createServer(function (request, response) {
  var query = url.parse(request.url.toLowerCase(),true).query;
  response.writeHead(500, {
    'Content-Type':'text/plain'
  });
  if (query && 
      (query.bbox !== undefined) &&
      (query.width !== undefined) &&
      (query.height !== undefined) ) {

      var bbox = query.bbox.split(',');
      var options = {width:parseInt(query.width),height:parseInt(query.height)};
      options.size = 50;
      aquire(stylesheet,options,function(err,map) {
          if (err) {
              response.end(err.message);
          } else{
              map.render(bbox,'png',function(err, buffer){
                  release(stylesheet,map);
                  if (err) {
                      response.end(err.message);
                  } else {
                      response.writeHead(200, {
                        'Content-Type':'image/png'
                      });
                      response.end(buffer);  
                  }
              });
          }
      
      })
  }
  else {
    response.end("something was not provided!");
  }
}).listen(port);


console.log('Server running at http://127.0.0.1:' + port + '/');