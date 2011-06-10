var mapnik = require('mapnik');
var assert = require('assert');
var fs = require('fs');

var stylesheet = './examples/stylesheet.xml';

exports['test simple_grid rendering'] = function(beforeExit) {
    var rendered = false;
    if (mapnik.supports.grid) {
        var reference = fs.readFileSync('./test/support/grid2.json', 'utf8');

        var map_grid = new mapnik.Map(256, 256);
        map_grid.load(stylesheet, {strict: true}, function(err,map) {
            if (err) throw err;
            map.zoom_all();
            var options = {'resolution': 4,
                           'key': '__id__',
                           'fields': ['NAME']
                          };
            map.render_grid('world', options, function(err, grid) {
                rendered = true;
                assert.ok(!err);
                assert.equal(JSON.stringify(grid), reference);
          });
        });
    } else {
        var reference = fs.readFileSync('./test/support/simple_grid.json', 'utf8');

        var map_grid = new mapnik.Map(256, 256);
        map_grid.load(stylesheet, {strict: true}, function(err,map) {
            map.zoom_all();
            map._render_grid(0, 4, 'FIPS', function(err, grid) {
                rendered = true;
                assert.ok(!err);
                assert.equal(JSON.stringify(grid), reference);
            });
        });
    }

    beforeExit(function() {
        assert.ok(rendered);
    });
};
