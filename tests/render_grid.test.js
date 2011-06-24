var mapnik = require('mapnik');
var assert = require('assert');
var fs = require('fs');

var stylesheet = './examples/stylesheet.xml';

exports['test simple_grid rendering'] = function(beforeExit) {
    var rendered = false;
    var reference = fs.readFileSync('./tests/support/grid2.json', 'utf8');
    var reference_view = fs.readFileSync('./tests/support/grid_view.json', 'utf8');

    var map_grid = new mapnik.Map(256, 256);
    map_grid.load(stylesheet, {strict: true}, function(err,map) {
        if (err) throw err;
        map.zoomAll();
        var options = {'layer': 0,
                       'fields': ['NAME']
                      };
        var grid = new mapnik.Grid(map.width, map.height, {key: '__id__'});
        map.render(grid, options, function(err, grid) {
            rendered = true;
            assert.ok(!err);
            grid_utf = grid.encode('utf', {resolution: 4});
            //fs.writeFileSync('./tests/support/grid2_.json',JSON.stringify(grid_utf))
            assert.equal(JSON.stringify(grid_utf), reference);

            // pull an identical view and compare it to original grid
            var gv = grid.view(0, 0, 256, 256);
            gv_utf = gv.encode('utf', {resolution: 4});
            assert.equal(JSON.stringify(gv_utf), reference);

            // pull a subsetted view (greenland basically)
            var gv2 = grid.view(64, 64, 64, 64);
            assert.equal(gv2.width(), 64);
            assert.equal(gv2.height(), 64);
            gv_utf2 = gv2.encode('utf', {resolution: 4});
            //fs.writeFileSync('./tests/support/grid_view.json',JSON.stringify(gv_utf2),'utf8')
            assert.equal(JSON.stringify(gv_utf2), reference_view);

        });
    });

    beforeExit(function() {
        assert.ok(rendered);
    });
};
