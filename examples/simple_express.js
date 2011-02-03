#!/usr/bin/env node

var mapnik = require('mapnik');
var express = require('express');

var app = express.createServer();

var port = 8000;

app.get('/', function(req, res){
  var map = new mapnik.Map(256,256);
  //map.load("./examples/stylesheet.xml");
  map.zoom_all();
  map.render(map.extent(),"png",function(err,buffer){
      if (err) {
        //res.send('Tile not found', 500);
        res.contentType('.txt');
        res.send(err.message);
      } else {
        //res.header('Content-Type','image/png');
        res.contentType("tile.png");
        res.send(buffer.toString())
      }
  });
})

app.listen(port);
