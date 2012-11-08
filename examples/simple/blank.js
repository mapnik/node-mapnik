
var mapnik = require('mapnik');
var http = require('http');
var path = require('path');


// Serve a blank tile. This can indicate the absolute fastest mapnik can return a tile
// and highlights zlib/inflate bottleneck (compare "png" format to "png8")
var port = 8000;

var im = new mapnik.Image(256, 256);
im.background = new mapnik.Color('steelblue');

http.createServer(function(req, res) {
    res.writeHead(200, {'Content-Type': 'image/png'});
    im.encode('png8:z=1',function(err,buffer) {
        res.end(buffer);
    });
}).listen(port);

console.log('server running on port ' + port);
