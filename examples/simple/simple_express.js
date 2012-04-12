
// This example shows how to use node-mapnik with the
// express web development framework (http://expressjs.com/)
//
// expected output: http://goo.gl/cyGwo

var mapnik = require('mapnik'),
  express = require('express'),
  path = require('path'),
  app = express.createServer(),
  port = 8000;

app.get('/', function(req, res) {
  var map = new mapnik.Map(256, 256);
  map.loadSync(path.join(__dirname, '../stylesheet.xml'));
  map.zoomAll();
  var im = new mapnik.Image(map.width, map.height);
  map.render(im, function(err,im) {
      if (err) {
        res.contentType('.txt');
        res.send(err.message);
      } else {
        res.send(im.encodeSync('png'), {'Content-Type': 'image/png'});
      }
  });
});

app.listen(port);

console.log('server running on port ' + port);
