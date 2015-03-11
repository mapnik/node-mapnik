"use strict";

var mapnik = require('../');
var assert = require('assert');
var path = require('path');

var grid;
var view;

describe('mapnik.GridView ', function() {
    before(function(done) {
        grid = new mapnik.Grid(256, 256);
        view = grid.view(0, 0, 256, 256);
        done();
    });
    
    it('should fail to intialize view', function() {
        assert.throws(function() { grid.view(); });
    });

    it('should fail to encode properly', function() {
        assert.throws(function() { view.encodeSync("foo"); });
        assert.throws(function() { view.encode("foo"); });
        assert.throws(function() { view.encode("foo", function(err, result) {}); });
        assert.throws(function() { view.encodeSync({features:null}); });
        assert.throws(function() { view.encodeSync({resolution:null}); });
        assert.throws(function() { view.encodeSync({resolution:0}); });
        assert.throws(function() { view.encode({features:null}, function(err, result) {}); });
        assert.throws(function() { view.encode({resolution:null}, function(err, result) {}); });
        assert.throws(function() { view.encode({resolution:0}, function(err, result) {}); });
        assert.throws(function() { view.encode({resolution:4}, null); });
    });

    it('should encode properly', function(done) {
        view.encode({resolution:4, features:true}, function(err, result) {
            if (err) throw err;
            assert.ok(result);
            done();
        });
    });

    it('should be solid', function() {
        assert.equal(view.isSolidSync(), true);
    });

    it('should be solid (async)', function(done) {
        view.isSolid(function(err,solid,pixel) {
            assert.equal(solid, true);
            if (mapnik.versions.mapnik_number < 200100) {
                assert.equal(pixel, 0);
            } else {
                assert.ok(pixel == -1 * 0x7FFFFFFFFFFFFFFF || pixel == -2147483648);
                assert.ok(pixel.toFixed() == mapnik.Grid.base_mask.toFixed() || pixel == -2147483648);
            }
            done();
        });
    });

    it('should report grid base_mask value for pixel', function() {
        var pixel = view.getPixel(0, 0);
        if (mapnik.versions.mapnik_number < 200100) {
            assert.equal(pixel, 0);
        } else {
            assert.ok(pixel == -1 * 0x7FFFFFFFFFFFFFFF || pixel == -2147483648);
            assert.ok(pixel.toFixed() == mapnik.Grid.base_mask.toFixed() || pixel == -2147483648);
        }
    });

    it('should be painted after rendering', function(done) {
        mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'shape.input'));
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/stylesheet.xml');
        map.zoomAll();
        var options = {'layer': 0,
                       'fields': ['NAME']
                      };
        var grid = new mapnik.Grid(map.width, map.height, {key: '__id__'});
        map.render(grid, options, function(err, grid) {
            var view = grid.view(0, 0, 256, 256);
            assert.equal(view.isSolidSync(), false);
            // hit alaska (USA is id 207)
            assert.equal(view.getPixel(25, 100), 207);
            view.isSolid(function(err,solid,pixel){
                assert.equal(solid, false);
                assert.equal(pixel, undefined);
                done();
            });
        });
    });
});
