var mapnik = require('mapnik');
var fs = require('fs');

var stylesheet = './examples/stylesheet.xml';

exports['test simple_grid rendering'] = function(beforeExit, assert) {
    var rendered = false;
    var reference = fs.readFileSync('./test/support/grid2.json', 'utf8');
    var reference_view = fs.readFileSync('./test/support/grid_view.json', 'utf8');

    var map_grid = new mapnik.Map(256, 256);
    map_grid.load(stylesheet, {strict: true}, function(err,map) {
        if (err) throw err;
        map.zoomAll();
        var grid = new mapnik.Grid(map.width, map.height, {key: '__id__'});

        // START TEMP
        // testing sync to isolate crash
        map.renderLayerSync(grid,0, {fields:['NAME']});
        // FIXME: segfault!
        grid_utf = grid.encodeSync('utf', {resolution: 4});
        rendered = true;
        // END TEMP
        /*
        var options = {'layer': 0,
                       'fields': ['NAME']
                      };
        map.render(grid, options, function(err, grid) {
            rendered = true;
            assert.ok(!err);
            grid_utf = grid.encodeSync('utf', {resolution: 4});
            //fs.writeFileSync('./ref.json',JSON.stringify(grid_utf))

            assert.equal(JSON.stringify(grid_utf), reference);

            // pull an identical view and compare it to original grid
            var gv = grid.view(0, 0, 256, 256);
            gv_utf = gv.encodeSync('utf', {resolution: 4});
            assert.equal(JSON.stringify(gv_utf), reference);

            // pull a subsetted view (greenland basically)
            var gv2 = grid.view(64, 64, 64, 64);
            assert.equal(gv2.width(), 64);
            assert.equal(gv2.height(), 64);
            gv_utf2 = gv2.encodeSync('utf', {resolution: 4});
            //fs.writeFileSync('./test/support/grid_view.json',JSON.stringify(gv_utf2),'utf8')
            assert.equal(JSON.stringify(gv_utf2), reference_view);
        });
        */
    });

    beforeExit(function() {
        assert.ok(rendered);
    });
};
