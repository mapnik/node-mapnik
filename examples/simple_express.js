#!/usr/bin/env node

var mapnik = require('mapnik');
var express = require('express');

var ver = process.versions.node.split('.');

if (parseInt(ver[1]) > 2) {
    console.log('sorry express is currently only compatible with node <= v0.2.6 (you are running: ' + process.version + ')');
    process.exit();
}

var app = express.createServer();

var port = 8000;

app.get('/', function(req, res){
  var map = new mapnik.Map(256,256);
  map.load("./examples/stylesheet.xml");
  map.zoom_all();
  map.render(map.extent(),"png",function(err,buffer){
      if (err) {
        //res.send('Tile not found', 500);
        res.contentType('.txt');
        res.send(err.message);
      } else {
        res.send(buffer, {'Content-Type': 'image/png'});
      }
  });
})

app.listen(port);
