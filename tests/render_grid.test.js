var mapnik = require('mapnik');
var assert = require('assert');
var fs = require('fs');

var stylesheet = './examples/stylesheet.xml';

exports['test simple_grid rendering'] = function(beforeExit) {
    var rendered = false;
    var reference = fs.readFileSync('./tests/support/grid2.json', 'utf8');

    var map_grid = new mapnik.Map(256, 256);
    map_grid.load(stylesheet, {strict: true}, function(err,map) {
        if (err) throw err;
        map.zoom_all();
        var options = {'layer': 0,
                       'fields': ['NAME']
                      };
        var grid = new mapnik.Grid(map.width,map.height, {key:'__id__'})
        map.render(grid, options, function(err, grid) {
            rendered = true;
            assert.ok(!err);
            grid_utf = grid.encode('utf', {resolution: 4});
            //fs.writeFileSync('./tests/support/grid2_.json',JSON.stringify(grid_utf))
            assert.equal(JSON.stringify(grid_utf), reference);
      });
    });

    beforeExit(function() {
        assert.ok(rendered);
    });
};
