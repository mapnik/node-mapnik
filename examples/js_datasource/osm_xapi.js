#!/usr/bin/env node
 
// Example of streaming feature showing DC pub locations 
// using OSM XAPI live datasource via Yahoo Pipes
// 
// TODO: add roads and labels from XAPI 
//
// dependencies:
// 
// * node-mapnik
// * node-get

/*
NOTE - maps using mapnik.JSDatasource can only be rendered with
mapnik.render_to_string() or mapnik.render_to_file() as the javascript
callback only works if the rendering happens in the main thread.

If you want async rendering using mapnik.render() then use the
mapnik.MemoryDatasource instead of mapnik.JSDatasource.
*/

var mapnik = require('mapnik');
var sys = require('fs');
var path = require('path');
var get = require('node-get');
var merc = '+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +no_defs +over';

// map with just a style
// eventually the api will support adding styles in javascript
var s = '<Map srs="' + merc + '">';
s += '<Style name="points">';
s += ' <Rule>';
s += '  <MarkersSymbolizer marker-type="ellipse" fill="yellow" width="3" opacity="0.5" allow-overlap="true" placement="point"/>';
s += ' </Rule>';
s += '</Style>';
s += '</Map>';

// create map object with base map
var map  = new mapnik.Map(800,600);
var merc = new mapnik.Projection('+init=epsg:3857');
map.load(path.join(__dirname, '../stylesheet.xml'));
map.from_string(s,'.');

// Pubs around Washington DC - using the OSM XAPI from Mapquest (http://open.mapquestapi.com/xapi/)) 
// XML munged into json using Yahoo pipes
var dl = new get("http://pipes.yahoo.com/pipes/pipe.run?_id=313bef20b9a083d22241c59211b04a91&_render=json")
dl.asString(function(err,str){
  // Loop through pub list
  // WARNING - this API will change!
  var pubs = JSON.parse(str).value.items[0].node;
  var pub;
  var next = function() {
      while (pub = pubs.pop()) {
        var merc_coords = merc.forward([+pub.lon, +pub.lat]); //reproject wgs84 to mercator
        return { 'x'          : merc_coords[0],
                 'y'          : merc_coords[1],
                 'properties' : { 'USER':pub.user}
               };
      }
  }

  // create the Merc special datasource
  var options = {
    extent: '-20037508.342789,-8283343.693883,20037508.342789,18365151.363070',
  };
  var ds = new mapnik.JSDatasource(options,next);

  // contruct a mapnik layer dynamically
  var l = new mapnik.Layer('test');
  l.srs = map.srs;
  l.styles = ["points"];

  // add our custom datasource
  l.datasource = ds;

  // add this layer to the map
  map.add_layer(l);

  // zoom to the extent of the new layer (pulled from options since otherwise we cannot know)
  bl = merc.forward([-77.161742101557, 38.83321750946542])
  tr = merc.forward([-76.88708389844298, 38.9534635955426])
  
  //minx,miny,maxx,maxy
  map.zoom_to_box(bl[0],bl[1],tr[0],tr[1]);

  // render it! You should see a bunch of red and blue points reprenting
  map.render_to_file('dc_pubs.png');

  console.log('rendered to dc_pubs.png' );
});