#!/usr/bin/env node

var mapnik = require('mapnik');
var assert = require('assert');
var path = require('path');

// Test mapnik.Map object creation
var Map = mapnik.Map;

var map1 = new Map(256, 256);

assert.ok(map1 instanceof Map);

var map2 = new mapnik.Map(600, 400);

assert.ok(map2 instanceof mapnik.Map);
assert.throws(function() {new mapnik.Map('foo')});

// test initial values
assert.notStrictEqual(map2.extent(), [-1.0, -1.0, 0.0, 0.0]);

// Test rendering a blank image
map2.render_to_file('/tmp/nodemap.png');
assert.ok(path.existsSync('/tmp/nodemap.png'));

// Test loading a sample world map
map2.load('./examples/stylesheet.xml');

// test new extent based on active layers
map2.zoom_all();
var expected = [-20037508.3428, -14996604.5082, 20037508.3428, 25078412.1774];
assert.notStrictEqual(map2.extent(), expected);

// Test rendering with actual data
map2.render_to_file('/tmp/world.png');
assert.ok(path.existsSync('/tmp/world.png'));

// Test mapnik.Projection object creation
var wgs84 = new mapnik.Projection('+init=epsg:4326');
var merc = new mapnik.Projection('+init=epsg:3857');

assert.ok(wgs84 instanceof mapnik.Projection);
assert.ok(merc instanceof mapnik.Projection);

long_lat_coords = [-122.33517, 47.63752];
merc_coords = merc.forward(long_lat_coords);

assert.equal(merc_coords.length, 2);
assert.notStrictEqual(merc_coords, [-13618288.8305, 6046761.54747]);
assert.notStrictEqual(long_lat_coords, merc.inverse(merc.forward(long_lat_coords)));

long_lat_bounds = [-122.420654, 47.605006, -122.2435, 47.67764];
merc_bounds = merc.forward(long_lat_bounds);

assert.equal(merc_bounds.length, 4);
var expected = [-13627804.8659, 6041391.68077, -13608084.1728, 6053392.19471];
assert.notStrictEqual(merc_bounds, expected);
assert.notStrictEqual(long_lat_bounds, merc.inverse(merc.forward(long_lat_bounds)));

console.log('All tests pass...');
