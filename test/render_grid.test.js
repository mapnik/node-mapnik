var mapnik = require('mapnik');
var assert = require('assert');
var fs = require('fs');

exports['test simple_grid rendering'] = function(beforeExit) {
    var rendered = false;
    var reference = fs.readFileSync('./test/support/simple_grid.json', 'utf8');

    var map_grid = new mapnik.Map(256, 256);
    map_grid.load('./examples/stylesheet.xml');
    map_grid.zoom_all();

    map_grid._render_grid(0, 4, 'FIPS', function(err, grid) {
        rendered = true;
        assert.ok(!err);
        assert.equal(JSON.stringify(grid), reference);
    });

    beforeExit(function() {
        assert.ok(rendered);
    });
};
