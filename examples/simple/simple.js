#!/usr/bin/env node

// This example shows how to use node-mapnik with the 
// basic node http server
//
// expected output: http://goo.gl/cyGwo


var mapnik = require('mapnik');
var http = require('http');
var path = require('path');

var port = 8000;

http.createServer(function(req, res) {
  var map = new mapnik.Map(256, 256);
  map.loadSync(path.join(__dirname, '../stylesheet.xml'));
  map.zoom_all();
  map.render(map.extent(), 'png', function(err,buffer) {
      if (err) {
        res.writeHead(500, {'Content-Type': 'text/plain'});
        res.end(err.message);
      } else {
        res.writeHead(200, {'Content-Type': 'image/png'});
        res.end(buffer);
      }
  });
}).listen(port);

console.log("server running on port " + port);
