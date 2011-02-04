#!/usr/bin/env node

var mapnik   = require('mapnik')
  , mercator = require('mapnik/sphericalmercator')
  , pool = require('mapnik/pool')
  , http     = require('http')
  , url      = require('url')
  , path     = require('path')
  , settings = require('./settings')

module.exports.server = http.createServer(function (req, res) {

  var query = url.parse(req.url.toLowerCase(),true).query;
  
  if (query &&
      query.x !== undefined &&
      query.y !== undefined &&
      query.z !== undefined &&
      query.sql !== undefined
      ) {
      
      var bbox = mercator.xyz_to_envelope(parseInt(query.x),
                                          parseInt(query.y),
                                          parseInt(query.z), false)
      var map = new mapnik.Map(256,256);
      settings.table = 'points9';//query.sql;
      var postgis = new mapnik.Datasource(settings);
      var map = new mapnik.Map(256,256);
      var layer = new mapnik.Layer("test")
      layer.datasource = postgis;
      layer.styles = ['style'];
      map.add_layer(layer);
      map.load(path.join(__dirname,'style.xml'))
      
      map.render(bbox,"png",function(err, buffer){
          if (err) {
              res.writeHead(500, {
                'Content-Type':'text/plain'
              });
              res.end(err.message);
          } else {
              res.writeHead(200, {
                'Content-Type':'image/png'
              });
              res.end(buffer);
          }
      });
  } else {
      res.writeHead(200, {
        'Content-Type':'text/plain'
      });
      res.end("missing x, y, z, or sql");
  }
});