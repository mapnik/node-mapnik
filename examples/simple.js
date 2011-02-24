#!/usr/bin/env node

var mapnik = require('mapnik');
var http = require('http');
var path = require('path');

var port = 8000;

http.createServer(function(req, res) {
  var map = new mapnik.Map(256, 256);
  map.load(path.join(__dirname, './stylesheet.xml'));
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
