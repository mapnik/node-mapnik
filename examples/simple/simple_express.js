
// This example shows how to use node-mapnik with the
// express web development framework (http://expressjs.com/)
//
// expected output: http://goo.gl/cyGwo

var mapnik = require('../..');
var express = require('express');
var path = require('path');

var app = express();
var port = 8000;

app.get('/', function(req, res) {
  var map = new mapnik.Map(256, 256);
  map.loadSync(path.join(__dirname, '../stylesheet.xml'));
  map.zoomAll();
  var im = new mapnik.Image(map.width, map.height);

  map.render(im, function (err, im) {
    if (err) {
      res.set('Content-Type', 'text/plain');
      return res.send(err.message);
    }

    res.set('Content-Type', 'image/png');
    res.send(im.encodeSync('png'));
  });
});

app.listen(port);

console.log('server running on port ' + port);
