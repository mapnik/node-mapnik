#!/usr/bin/env node

var mapnik = require('mapnik');
var assert = require('assert');
var path = require('path');
var fs = require('fs');

/* settings */

assert.ok(mapnik.settings)
assert.ok(mapnik.settings.paths)
assert.ok(mapnik.settings.paths.fonts.length)
assert.ok(mapnik.settings.paths.input_plugins.length)


/* ABI compatibility */

// be strict/exact
// note process.versions == process.versions.node except the latter has the 'v' stripped
assert.ok(mapnik.versions.node === process.versions.node,'The node version "' + process.versions.node + '" does not match the node version that node-mapnik was compiled against: "' + mapnik.versions.node + '"');


/* MAP */

// Test mapnik.Map object creation
var Map = mapnik.Map;

var map1 = new Map(256, 256);

assert.ok(map1 instanceof Map);

var map = new mapnik.Map(600, 400);

assert.ok(map instanceof mapnik.Map);
assert.throws(function() {new mapnik.Map('foo')});

// test initial values
assert.notStrictEqual(map.extent(), [-1.0, -1.0, 0.0, 0.0]);

// Test rendering a blank image
map.render_to_file('/tmp/nodemap.png');
assert.ok(path.existsSync('/tmp/nodemap.png'));

// Test loading a sample world map
map.load('./examples/stylesheet.xml');

// Test loading a map from a string
var style_string = fs.readFileSync('./examples/stylesheet.xml').toLocaleString();
var base_url = './examples/'; // must end with trailing slash
map.from_string(style_string,base_url);

// test new extent based on active layers
map.zoom_all();
var expected = [-20037508.3428, -14996604.5082, 20037508.3428, 25078412.1774];
assert.notStrictEqual(map.extent(), expected);

// Test rendering with actual data
map.render_to_file('/tmp/world.png');
assert.ok(path.existsSync('/tmp/world.png'));

assert.equal(map.width(), 600);
assert.equal(map.height(), 400);
map.resize(256, 256);
assert.equal(map.width(), 256);
assert.equal(map.height(), 256);

/* PROJECTION */

// Test mapnik.Projection object creation

assert.throws(function() {new mapnik.Projection('+init=epsg:foo')});
assert.throws(function() {new mapnik.Projection('+proj +foo')});

try {
    var proj = new mapnik.Projection('+init=epsg:foo');
}
catch (err)
{
    assert.equal(err.message, 'failed to initialize projection with:+init=epsg:foo');
}

var wgs84 = new mapnik.Projection('+init=epsg:4326');

assert.ok(wgs84 instanceof mapnik.Projection);

var merc = null;

try {
    // perhaps we've got a savvy user?
    merc = new mapnik.Projection('+init=epsg:900913');
}
catch (err) {
    // newer versions of proj4 have this code which is == 900913
    merc = new mapnik.Projection('+init=epsg:3857');
}

if (!merc) {
    /* be friendly if this test fails */
    var msg = 'warning, could not create a spherical mercator projection';
    msg += '\nwith either epsg:900913 or epsg:3857, so they must be missing';
    msg += '\nfrom your proj4 epsg table (/usr/local/share/proj/epsg)';
    console.log(msg);
}
else
{
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
}

console.log('All tests pass...');
