
var mapnik = require('mapnik')
  , mercator = require('../../utils/sphericalmercator')
  , http = require('http')
  , url = require('url')
  , tile = 256
  , path = require('path');

var usage = 'usage: app.js <port>';

var port = process.argv[2];

if (!port) {
   console.log(usage);
   process.exit(1);
}

var elastic_port = 9200;


// map with just a style
// eventually the api will support adding styles in javascript
var s = '<Map srs="' + mercator.proj4 + '" buffer-size="128">';
s += '<Style name="style">';
s += ' <Rule>';
s += '  <MarkersSymbolizer marker-type="ellipse" fill="red" width="5" allow-overlap="true" placement="point"/>';
s += ' </Rule>';
s += '</Style>';
s += '</Map>';

var client = require('http').createClient(elastic_port, 'localhost');

function search(el_query, callback) {
    el_query = JSON.stringify(el_query);
    var headers = {
        'Content-Length': el_query.length,
        'charset': 'UTF-8',
        'Content-Type': 'application/json'
    };
    var request = client.request('GET', '/geo/_search', headers);
    request.write(el_query, 'utf8');
    request.on('response', function(response) {
        var body = '';
        response.on('data', function(data) {
            body += data;
        });
        response.on('end', function() {
            callback(JSON.parse(body));
        });
    });
    request.end();
}

http.createServer(function(req, res) {

  var query = url.parse(req.url, true).query;

  if (query &&
      query.x !== undefined &&
      query.y !== undefined &&
      query.z !== undefined
      ) {

      var map = new mapnik.Map(tile, tile);

      res.writeHead(200, {'Content-Type': 'image/png'});

      var bbox = mercator.xyz_to_envelope(parseInt(query.x), parseInt(query.y), parseInt(query.z), false);
      map.fromString(s,
          {strict: true, base: './'},
          function(err, map) {
              if (err) {
                  res.writeHead(500, {
                    'Content-Type': 'text/plain'
                  });
                  res.end(err.message);
              }

              var options = {
                  extent: '-20037508.342789,-8283343.693883,20037508.342789,18365151.363070'
              };

              var mem_ds = new mapnik.MemoryDatasource(options);

              var el_query = {
                 'size': 100,
                 'query' : {
                     'match_all' : {}
                 },
                  'filter' : {
                      'geo_bounding_box' : {
                          'project.location' : {
                              'top_left' : {
                                  'lat' : bbox[3],
                                  'lon' : bbox[0]
                              },
                              'bottom_right' : {
                                  'lat' : bbox[1],
                                  'lon' : bbox[2]
                              }
                          }
                      }
                  }
              };

              search(el_query, function(result) {
                  if (!result.hits.hits) {
                        res.writeHead(500, {
                          'Content-Type': 'text/plain'
                        });
                        res.end('no elastic search results');
                  } else {
                      //console.log(JSON.stringify(result.facets, null, 2));
                      //console.log(result.took + 'ms / ' + result.hits.total + ' results');
                      result.hits.hits.forEach(function(hit) {
                             var x = hit._source.project.location.lon;
                             var y = hit._source.project.location.lat;
                             var name = hit._source.project.name;
                             var pop2005 = hit._source.project.pop2005;
                             //console.log('x: ' + x + ' y: ' + y);
                             mem_ds.add({ 'x' : x,
                                          'y' : y,
                                          'properties' : { 'NAME': name, 'pop2005': pop2005 }
                                         });
                      });

                      var l = new mapnik.Layer('test');
                      l.srs = map.srs;
                      l.styles = ['style'];
                      l.datasource = mem_ds;
                      map.add_layer(l);
                      map.extent = bbox;
                      var im = new mapnik.Image(map.width, map.height);
                      map.render(im, function(err, im) {
                          if (err) {
                              res.writeHead(500, {
                                'Content-Type': 'text/plain'
                              });
                              res.end(err.message);
                          } else {
                              res.writeHead(200, {
                                'Content-Type': 'image/png'
                              });
                              res.end(im.encodeSync('png'));
                          }
                      });
                  }
              });
          }
      );
  } else {
      res.writeHead(200, {
        'Content-Type': 'text/plain'
      });
      res.end('no x,y,z provided');
  }
}).listen(port);


console.log('Test server listening on port ' + port);
