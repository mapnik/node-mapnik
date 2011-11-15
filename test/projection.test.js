var mapnik = require('mapnik');
var assert = require('assert');
var fs = require('fs');
var path = require('path');

exports['test projection creation'] = function(beforeExit, assert) {
    assert.throws(function() { new mapnik.Projection('+init=epsg:foo'); },
        /failed to initialize projection with: '\+init=epsg:foo'/);
    assert.throws(function() { new mapnik.Projection('+proj +foo'); },
        /failed to initialize projection with: '\+proj \+foo'/);


    var wgs84 = new mapnik.Projection('+init=epsg:4326');
    assert.ok(wgs84 instanceof mapnik.Projection);
};

exports['test projection'] = function(beforeExit, assert) {
    var merc;
    try {
        // perhaps we've got a savvy user?
        merc = new mapnik.Projection('+init=epsg:900913');
    }
    catch (err) {
        // newer versions of proj4 have this code which is == 900913
        merc = new mapnik.Projection('+init=epsg:3857');
    }

    assert.ok(merc instanceof mapnik.Projection,
        'warning, could not create a spherical mercator projection ' +
        'with either epsg:900913 or epsg:3857, so they must be missing ' +
        'from your proj4 epsg table (/usr/local/share/proj/epsg)');

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
};

