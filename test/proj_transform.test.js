var mapnik = require('../');
var assert = require('assert');
var fs = require('fs');
var path = require('path');

describe('mapnik.ProjTransform ', function() {
    it('should throw with invalid usage', function() {
        assert.throws(function() { new mapnik.ProjTransform('+init=epsg:foo'); });
        assert.throws(function() { new mapnik.ProjTransform('+proj +foo'); });
        assert.throws(function() { new mapnik.ProjTransform(1,1); });
        assert.throws(function() { new mapnik.ProjTransform({},{}); });
    });

    it('should initialize properly', function() {
        var wgs84 = new mapnik.Projection('+init=epsg:4326');
        var wgs84_2 = new mapnik.Projection('+init=epsg:4326');
        var trans = new mapnik.ProjTransform(wgs84,wgs84_2);
        assert.ok(trans);
        assert.ok(trans instanceof mapnik.ProjTransform);
    });

    it('should forward coords properly (no-op)', function() {
        var wgs84 = new mapnik.Projection('+init=epsg:4326');
        var wgs84_2 = new mapnik.Projection('+init=epsg:4326');
        var trans = new mapnik.ProjTransform(wgs84,wgs84_2);
        var long_lat_coords = [-122.33517, 47.63752];
        assert.deepEqual(long_lat_coords,trans.forward(long_lat_coords));
    });

    it('should forward coords properly (no-op)', function() {
        var wgs84 = new mapnik.Projection('+init=epsg:4326');
        var wgs84_2 = new mapnik.Projection('+init=epsg:4326');
        var trans = new mapnik.ProjTransform(wgs84,wgs84_2);
        var long_lat_box = [-122.33517, 47.63752,-122.33517, 47.63752];
        assert.deepEqual(long_lat_box,trans.forward(long_lat_box));
    });

    it('should forward coords properly (4326 -> 3857)', function() {
        var from = new mapnik.Projection('+init=epsg:4326');
        var to = new mapnik.Projection('+init=epsg:3857');
        var trans = new mapnik.ProjTransform(from,to);
        var long_lat_coords = [-122.33517, 47.63752];
        var merc = [-13618288.8305, 6046761.54747];
        assert.notStrictEqual(merc,trans.forward(long_lat_coords));
    });

    it('should backward coords properly (3857 -> 4326)', function() {
        var from = new mapnik.Projection('+init=epsg:4326');
        var to = new mapnik.Projection('+init=epsg:3857');
        var trans = new mapnik.ProjTransform(from,to);
        var long_lat_coords = [-122.33517, 47.63752];
        var merc = [-13618288.8305, 6046761.54747];
        assert.notStrictEqual(long_lat_coords,trans.backward(merc));
    });

/*
    it('should throw with invalid coords (4326 -> 3857)', function() {
        var from = new mapnik.Projection('+init=epsg:4326');
        var to = new mapnik.Projection('+init=epsg:3857');
        var trans = new mapnik.ProjTransform(from,to);
        var long_lat_coords = [-190, 95];
        assert.throws(function() { trans.forward(long_lat_coords); });
    });
*/

    it('should forward bbox properly (4326 -> 3857)', function() {
        var from = new mapnik.Projection('+init=epsg:4326');
        var to = new mapnik.Projection('+init=epsg:3857');
        var trans = new mapnik.ProjTransform(from,to);
        var long_lat_box = [-131.3086, 16.8045, -61.6992, 54.6738];
        var merc = [-14617205.7910, 1898084.2861, -6868325.6126, 7298818.9559];
        assert.notStrictEqual(merc,trans.forward(long_lat_box));
    });

    it('should backward bbox properly (3857 -> 4326)', function() {
        var from = new mapnik.Projection('+init=epsg:4326');
        var to = new mapnik.Projection('+init=epsg:3857');
        var trans = new mapnik.ProjTransform(from,to);
        var long_lat_box = [-131.3086, 16.8045, -61.6992, 54.6738];
        var merc = [-14617205.7910, 1898084.2861, -6868325.6126, 7298818.9559];
        assert.notStrictEqual(long_lat_box,trans.backward(merc));
    });

/*
    it('should throw with invalid bbox (4326 -> 3857)', function() {
        var from = new mapnik.Projection('+init=epsg:4326');
        var to = new mapnik.Projection('+init=epsg:3857');
        var trans = new mapnik.ProjTransform(from,to);
        var long_lat_box = [-180,90,180,90];
        assert.throws(function() { trans.forward(long_lat_box); });
    });
*/

});


