#!/usr/bin/env node

// This example shows how to use node-mapnik with tilelive.js
// to render serve map tiles to polymaps client
//
// expected output at zoom 0: http://goo.gl/cyGwo


var http = require('http'),
    Tile = require('tilelive').Tile,
    path = require('path'),
    sys = require('sys'),
    fs = require('fs'),   
    util = require('../lib/utility.js'),
    usage = 'usage: app.js <stylesheet> <port>\ndemo:  app.js ../../stylesheet.xml 8000',
    MAPFILE_DIR = '.',
    stylesheet = process.ARGV[2],
    port = process.ARGV[3];
    
if (!stylesheet) {
   console.log(usage);
   process.exit(1);
}

if (!port) {
   console.log(usage);
   process.exit(1);
}

var language = (path.extname(stylesheet).toLowerCase() == '.mml') ? 'carto' : 'xml';

http.createServer(function(req, res) {
    util.parseXYZ(req,function(err,params) {
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