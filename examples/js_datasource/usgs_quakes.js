
// Example of streaming feature showing last 30 days of global earthquakes
// from the USGS earthquake data feed
//
// dependencies:
//
// * node-mapnik
// * node-get

/*
NOTE - maps using mapnik.JSDatasource can only be rendered with
mapnik.renderSync() or mapnik.renderFileSync() as the javascript
callback only works if the rendering happens in the main thread.

If you want async rendering using mapnik.render() then use the
mapnik.MemoryDatasource instead of mapnik.JSDatasource.
*/

var mapnik = require('mapnik');
var sys = require('fs');
var path = require('path');
var get = require('get');
var merc = require('../utils/sphericalmercator').proj4;

// map with just a style
// eventually the api will support adding styles in javascript
var s = '<Map srs="' + merc + '">';
s += '<Style name="points">';
s += ' <Rule>';
s += '  <Filter>[MAGNITUDE]&gt;7</Filter>';
s += '  <MarkersSymbolizer marker-type="ellipse" fill="red" width="15" allow-overlap="true" placement="point"/>';
s += ' </Rule>';
s += ' <Rule>';
s += '  <Filter>[MAGNITUDE]&gt;4</Filter>';
s += '  <MarkersSymbolizer marker-type="ellipse" fill="orange" width="7" opacity="0.5" allow-overlap="true" placement="point"/>';
s += ' </Rule>';
s += ' <Rule>';
s += '  <ElseFilter />';
s += '  <MarkersSymbolizer marker-type="ellipse" fill="yellow" width="3" opacity="0.5" allow-overlap="true" placement="point"/>';
s += ' </Rule>';
s += '</Style>';
s += '</Map>';

// create map object with base map
var map = new mapnik.Map(800, 600);
var merc = new mapnik.Projection('+init=epsg:3857');
map.loadSync(path.join(__dirname, '../stylesheet.xml'));
map.fromStringSync(s);

// Latest 30 days of earthquakes > 2.5 from USGS (http://earthquake.usgs.gov/earthquakes/catalogs/)
// CSV munged into json using Yahoo pipes
var dl = new get('http://pipes.yahoo.com/pipes/pipe.run?_id=f36216d2581df7ed23615f42ff2af187&_render=json');
dl.asString(function(err,str) {
  // Loop through quake list
  // WARNING - this API will change!
  var quakes = JSON.parse(str).value.items;
  var quake;
  var next = function() {
      while ((quake = quakes.pop())) {
        var merc_coords = merc.forward([+quake.Lon, +quake.Lat]); //reproject wgs84 to mercator
        return { 'x' : merc_coords[0],
                 'y' : merc_coords[1],
                 'properties' : { 'NAME': quake.Region, 'MAGNITUDE': +quake.Magnitude}
               };
      }
  };

  // create the Merc special datasource
  var options = {
    extent: '-20037508.342789,-8283343.693883,20037508.342789,18365151.363070'
  };
  var ds = new mapnik.JSDatasource(options, next);

  // contruct a mapnik layer dynamically
  var l = new mapnik.Layer('test');
  l.srs = map.srs;
  l.styles = ['points'];

  // add our custom datasource
  l.datasource = ds;

  // add this layer to the map
  map.add_layer(l);

  // zoom to the extent of the new layer (pulled from options since otherwise we cannot know)
  map.zoomAll();

  // render it! You should see a bunch of red and blue points reprenting
  map.renderFileSync('quakes.png');

  console.log('rendered to quakes.png');
});
