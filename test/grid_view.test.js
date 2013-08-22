var mapnik = require('../');
var assert = require('assert');
var fs = require('fs');
var path = require('path');

var grid;
var view;

describe('mapnik.GridView ', function() {
    before(function(done) {
        grid = new mapnik.Grid(256, 256);
        view = grid.view(0, 0, 256, 256);
        done();
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
