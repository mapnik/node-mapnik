var mapnik = require('mapnik');
var express = require('express');
var path = require('path');

var port = 8000;
var stylesheet = path.join(__dirname,"stylesheet.xml");

var app = express.createServer();

app.get('/', function(req, res) {
  var map = new mapnik.Map(256,256);
  map.load(stylesheet);
  map.zoom_all();
  res.contentType("tile.png");
  res.send(map.render_to_string());
});

app.listen(port);
console.log('Server running at http://127.0.0.1:' + port);
console.log('For a real server see: https://github.com/tmcw/tilelive.js');