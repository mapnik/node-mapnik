#!/usr/bin/env node

// This example shows how to use node-mapnik to 
// render maps tiles based on spatial data stored in postgis
//
// To run you must configure the postgis_settings variable


var mapnik = require('mapnik')
  , mercator = require('mapnik/sphericalmercator')
  , url  = require('url')
  , fs   = require('fs')
  , http = require('http')
  , util = require('../lib/utility.js')
  , path = require('path')
  , port = 8000
  , TMS_SCHEME = false;

var postgis_settings = {
  'dbname'          : 'YOUR DATABASE',
  'table'           : 'YOUR TABLE',
  'user'            : 'postgres',
  'host'            : '127.0.0.1',
  'type'            : 'postgis',
  'geometry_field'  : 'YOUR GEOMETRY FIELD (normally the_geom)',
  'srid'            : 'YOUR GEOMETRY SRID (web mercator is 3785)',
  'extent'          : '-20005048.4188,-9039211.13765,19907487.2779,17096598.5401',  //change this if not merc  
  'max_size'        : 1    
};

http.createServer(function(req, res) {
  util.parseXYZ(req, TMS_SCHEME, function(err,params) {
    if (err) {
      res.writeHead(500, {'Content-Type': 'text/plain'});
      res.end(err.message);
    } else {
      try {        
        var map     = new mapnik.Map(256, 256, mercator.srs);                
        var layer   = new mapnik.Layer('tile', mercator.srs);
        var postgis = new mapnik.Datasource(postgis_settings);
        var bbox    = mercator.xyz_to_envelope(parseInt(params.x),
                                               parseInt(params.y),
                                               parseInt(params.z), false);    

        layer.datasource = postgis;
        layer.styles     = ['point'];
        
        map.buffer_size(50);
        map.load('point_vector.xml'); // load style
        map.add_layer(layer);

        // console.log(map.toXML()); // Debug settings
        
        map.render(bbox, 'png', function(err, buffer) {
          if (err) {
            throw err;
          } else {
            res.writeHead(200, {'Content-Type': 'image/png'});
            res.end(buffer);            
          }
        });
      }
      catch (err) {        
        res.writeHead(500, {'Content-Type': 'text/plain'});
        res.end(err.message);
      }
    }
  });  
}).listen(port);

console.log('Server running on port %d', port);
