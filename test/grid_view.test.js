var mapnik = require('mapnik');
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
        assert.equal(view.isSolid(), true);
    });

    it('should have zero value for pixel', function() {
        var pixel = view.getPixel(0, 0);
        assert.equal(pixel, -2147483648);
    });

    it('should be painted after rendering', function(done) {
        var map = new mapnik.Map(256, 256);
        map.loadSync('./examples/stylesheet.xml');
        map.zoomAll();
        var options = {'layer': 0,
                       'fields': ['NAME']
                      };
        var grid = new mapnik.Grid(map.width, map.height, {key: '__id__'});
        map.render(grid, options, function(err, grid) {
            var view = grid.view(0, 0, 256, 256);
            assert.equal(view.isSolid(), false);
            // hit alaska (USA is id 207)
            assert.equal(view.getPixel(25, 100), 207);
            done();
        });
    });
});
