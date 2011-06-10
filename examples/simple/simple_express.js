#!/usr/bin/env node

// This example shows how to use node-mapnik with the 
// express web development framework (http://expressjs.com/)  
//
// expected output: http://goo.gl/cyGwo

var mapnik = require('mapnik')
  , express = require('express')
  , path = require('path')
  , app = express.createServer()
  , port = 8000;

app.get('/', function(req, res) {
  var map = new mapnik.Map(256, 256);
  map.loadSync(path.join(__dirname, '../stylesheet.xml'));
  map.zoom_all();
  map.render(map.extent(), 'png', function(err,buffer) {
      if (err) {       
        res.contentType('.txt');
        res.send(err.message);
      } else {
        res.send(buffer, {'Content-Type': 'image/png'});
      }
  });
}).listen(port);

console.log("server running on port " + port);
