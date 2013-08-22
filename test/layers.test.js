var mapnik = require('../');
var assert = require('assert');
var fs = require('fs');
var path = require('path');

describe('mapnik.Layer ', function() {
    it('should throw with invalid usage', function() {
        // no 'new' keyword
        assert.throws(function() { mapnik.Layer('foo'); });
        // invalid args
        assert.throws(function() { new mapnik.Layer(); });
        assert.throws(function() { new mapnik.Layer(1); });
        assert.throws(function() { new mapnik.Layer('a', 'b', 'c'); });
        assert.throws(function() { new mapnik.Layer(new mapnik.Layer('foo')); });
    });

    it('should initialize properly', function() {
        var layer = new mapnik.Layer('foo', '+init=epsg:4326');
        assert.equal(layer.name, 'foo');
        assert.equal(layer.srs, '+init=epsg:4326');
        assert.deepEqual(layer.styles, []);
        // will be empty/undefined
        assert.ok(!layer.datasource);
        var options = {
            type: 'shape',
            file: './test/data/world_merc.shp'
        };

        var ds = new mapnik.Datasource(options);
        layer.datasource = ds;
        assert.ok(layer.datasource instanceof mapnik.Datasource);

        // json representation
        var meta = layer.describe();
        assert.equal(meta.name, 'foo');
        assert.equal(meta.srs, '+init=epsg:4326');
        assert.deepEqual(meta.styles, []);
        assert.deepEqual(meta.datasource, options);
    });
});
