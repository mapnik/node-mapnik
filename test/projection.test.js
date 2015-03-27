"use strict";

var mapnik = require('../');
var assert = require('assert');

describe('mapnik.Projection ', function() {
    it('should throw with invalid usage', function() {
        assert.throws(function() { mapnik.Projection('+init=epsg:foo'); } );
        assert.throws(function() { new mapnik.Projection('+init=epsg:foo'); } );
        assert.throws(function() { new mapnik.Projection('+proj +foo'); } );
        assert.throws(function() { new mapnik.Projection(1); });
        assert.throws(function() { new mapnik.Projection({}); });
        assert.throws(function() { new mapnik.Projection('+init=epsg:3857', null); } );
        assert.throws(function() { new mapnik.Projection('+init=epsg:3857', {lazy:null}); } );
    });

    it('should initialize properly', function() {
        var wgs84 = new mapnik.Projection('+init=epsg:4326');
        assert.ok(wgs84 instanceof mapnik.Projection);

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

        var long_lat_coords = [-122.33517, 47.63752];
        assert.throws(function() { merc.forward() });
        assert.throws(function() { merc.forward(null) });
        assert.throws(function() { merc.forward([1,2,3]) });
        var merc_coords = merc.forward(long_lat_coords);

        assert.equal(merc_coords.length, 2);
        assert.notStrictEqual(merc_coords, [-13618288.8305, 6046761.54747]);
        assert.notStrictEqual(long_lat_coords, merc.inverse(merc.forward(long_lat_coords)));

        assert.throws(function() { merc.inverse(); });
        assert.throws(function() { merc.inverse(null); });
        assert.throws(function() { merc.inverse([1,2,3]); });

        var long_lat_bounds = [-122.420654, 47.605006, -122.2435, 47.67764];
        var merc_bounds = merc.forward(long_lat_bounds);

        assert.equal(merc_bounds.length, 4);
        var expected = [-13627804.8659, 6041391.68077, -13608084.1728, 6053392.19471];
        assert.notStrictEqual(merc_bounds, expected);
        assert.notStrictEqual(long_lat_bounds, merc.inverse(merc.forward(long_lat_bounds)));
    });

    it('should fail some methods with an uninitialized projection', function() {
        var wgs84 = new mapnik.Projection('+init=epsg:4326', {lazy:true});
        assert.ok(wgs84 instanceof mapnik.Projection);
        var long_lat_coords = [-122.33517, 47.63752];
        assert.throws(function() { wgs84.forward(long_lat_coords); });
        assert.throws(function() { wgs84.inverse(long_lat_coords); });
    });
});

