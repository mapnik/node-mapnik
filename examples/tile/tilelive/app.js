#!/usr/bin/env node

var http = require('http'),
    Tile = require('tilelive').Tile,
    path = require('path'),
    sys = require('sys'),
    fs = require('fs'),
    url = require('url');

var usage = 'usage: app.js <stylesheet> <port>';

MAPFILE_DIR = '.';

var stylesheet = process.ARGV[2];

if (!stylesheet) {
   console.log(usage);
   process.exit(1);
}

var language = (path.extname(stylesheet).toLowerCase() == '.mml') ?
    'carto' :
    'xml';

var port = process.ARGV[3];

if (!port) {
   console.log(usage);
   process.exit(1);
}

var parseXYZ = function(req,callback) {
    matches = req.url.match(/(\d+)/g);
    if (matches && matches.length == 3) {
        try {
            callback(null,
               { z: parseInt(matches[0]),
                 x: parseInt(matches[1]),
                 y: parseInt(matches[2])
               });
        } catch (err) {
            callback(err,null);
        }
    } else {
          var query = url.parse(req.url, true).query;
          if (query &&
                query.x !== undefined &&
                query.y !== undefined &&
                query.z !== undefined) {
             try {
             callback(null,
               { z: parseInt(query.z),
                 x: parseInt(query.x),
                 y: parseInt(query.y)
               }
             );
             } catch (err) {
                 callback(err,null);
             }
          } else {
              callback("no x,y,z provided",null);
          }
    }
}


http.createServer(function(req, res) {
    parseXYZ(req,function(err,params) {
        res.writeHead(500, {
          'Content-Type': 'text/plain'
        });
        if (err) {
            res.end(err);
        } else {
            try {
                var tile = new Tile({
                    type: 'map',
                    scheme: 'xyz',
                    datasource: stylesheet,
                    language: language,
                    xyz: [params.x, params.y, params.z],
                    format: 'png',
                    mapfile_dir: MAPFILE_DIR
                });
            } catch (err) {
                res.end('Tile invalid: ' + err.message + '\n');
            }
        
            tile.render(function(err, data) {
                if (!err) {
                    res.writeHead(200, data[1]);
                    res.end(data[0]);
                } else {
                    res.end('Tile rendering error: ' + err + '\n');
                }
            });
        }
    });
}).listen(port);

console.log('Test server listening on port %d', port);