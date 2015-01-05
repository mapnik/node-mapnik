"use strict";

var mapnik = require('../');
var assert = require('assert');
var path = require('path');
var fs = require('fs');

var stylesheet = './test/stylesheet.xml';
var reference = './test/support/grid2.json';
var reference_view = './test/support/grid_view.json';
var reference__id__ = './test/support/grid__id__.json';
var reference__id__2 = './test/support/grid__id__2.json';
var reference__id__3 = './test/support/grid__id__3.json';

mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'shape.input'));

function _c(grid1,grid2) {
    return grid2.replace(/\r/g, '') == grid2.replace(/\r/g, '');
}

describe('mapnik grid rendering ', function() {

    it('should match expected output (sync rendering)', function(done) {
        var map = new mapnik.Map(256, 256);
        map.loadSync(stylesheet, {strict: true});
        map.zoomAll();
        var grid = new mapnik.Grid(map.width, map.height, {key: '__id__'});
        var options = {'layer': 0,
                       'fields': ['NAME']
                      };
        map.render(grid, options, function(err, grid) {
            if (err) throw err;
            var grid_utf = grid.encodeSync('utf', {resolution: 4});
            // pull an identical view and compare it to original grid
            var gv = grid.view(0, 0, 256, 256);
            var gv_utf = gv.encodeSync('utf', {resolution: 4});
            if (process.env.UPDATE) {
                fs.writeFileSync(reference,JSON.stringify(grid_utf,null,1));
            } else {
                var expected = fs.readFileSync(reference,'utf8');
                var equal = _c(JSON.stringify(grid_utf,null,1),expected);
                assert.ok(equal);
                equal = _c(JSON.stringify(gv_utf,null,1),expected);
                assert.ok(equal);
            }
            // pull a subsetted view (greenland basically)
            var gv2 = grid.view(64, 64, 64, 64);
            assert.equal(gv2.width(), 64);
            assert.equal(gv2.height(), 64);
            var gv_utf2 = gv2.encodeSync('utf', {resolution: 4});
            var expected_view = fs.readFileSync(reference_view,'utf8');
            if (process.env.UPDATE) {
                fs.writeFileSync(reference_view,JSON.stringify(gv_utf2,null,1));
            } else {
                assert.ok(_c(JSON.stringify(gv_utf2,null,1),expected_view));
            }
            done();
        });
    });

    it('should match expected output (async rendering)', function(done) {
        var map = new mapnik.Map(256, 256);
        map.load(stylesheet, {strict: true}, function(err,map) {
            if (err) throw err;
            map.zoomAll();
            var grid = new mapnik.Grid(map.width, map.height, {key: '__id__'});

            var options = {'layer': 0,
                           'fields': ['NAME']
                          };
            map.render(grid, options, function(err, grid) {
                if (err) throw err;
                grid.encode('utf', {resolution: 4}, function(err,utf) {
                    var equal = _c(JSON.stringify(utf,null,1),fs.readFileSync(reference,'utf8'));
                    assert.ok(equal);
                    done();
                });
            });
        });
    });

    it('should match expected output (async rendering view)', function(done) {
        var map = new mapnik.Map(256, 256);
        map.load(stylesheet, {strict: true}, function(err,map) {
            if (err) throw err;
            map.zoomAll();
            var grid = new mapnik.Grid(map.width, map.height, {key: '__id__'});

            var options = {'layer': 0,
                           'fields': ['NAME']
                          };
            map.render(grid, options, function(err, grid) {
                assert.ok(!err);
                var gv = grid.view(0, 0, 256, 256);
                gv.encode('utf', {resolution: 4}, function(err,gv_utf1) {
                    var equal = _c(JSON.stringify(gv_utf1,null,1),fs.readFileSync(reference,'utf8'));
                    assert.ok(equal);
                    done();
                });
            });
        });
    });

    it('should match expected output (async rendering view subsetted)', function(done) {
        var map = new mapnik.Map(256, 256);
        map.load(stylesheet, {strict: true}, function(err,map) {
            if (err) throw err;
            map.zoomAll();
            var grid = new mapnik.Grid(map.width, map.height, {key: '__id__'});

            var options = {'layer': 0,
                           'fields': ['NAME']
                          };
            map.render(grid, options, function(err, grid) {
                assert.ok(!err);
                // pull a subsetted view (greenland basically)
                var gv2 = grid.view(64, 64, 64, 64);
                assert.equal(gv2.width(), 64);
                assert.equal(gv2.height(), 64);
                gv2.encode('utf', {resolution: 4}, function(err,gv_utf2) {
                    if (process.env.UPDATE) {
                        fs.writeFileSync(reference_view,JSON.stringify(gv_utf2,null,1));
                    } else {
                        var equal = _c(JSON.stringify(gv_utf2,null,1),fs.readFileSync(reference_view,'utf8'));
                        assert.ok(equal);
                    }
                    done();
                });
            });
        });
    });

    it('should match expected output if __id__ is not the grid key', function(done) {
        var map = new mapnik.Map(256, 256);
        map.loadSync(stylesheet, {strict: true});
        map.zoomAll();
        var grid = new mapnik.Grid(map.width, map.height, {key: 'NAME'});
        var options = {'layer': 0,
                       'fields': ['FIPS','__id__']
                      };
        map.render(grid, options, function(err, grid) {
            if (err) throw err;
            var grid_utf = grid.encodeSync('utf', {resolution: 4});
            if (process.env.UPDATE) {
                fs.writeFileSync(reference__id__,JSON.stringify(grid_utf,null,1));
            } else {
                var equal = _c(JSON.stringify(grid_utf,null,1),fs.readFileSync(reference__id__,'utf8'));
                assert.ok(equal);
            }
            done();
        });
    });

    it('should match expected output if __id__ both the grid key and in the attributes with others', function(done) {
        var map = new mapnik.Map(256, 256);
        map.loadSync(stylesheet, {strict: true});
        map.zoomAll();
        var grid = new mapnik.Grid(map.width, map.height, {key: '__id__'});
        var options = {'layer': 0,
                       'fields': ['__id__','NAME']
                      };
        map.render(grid, options, function(err, grid) {
            if (err) throw err;
            var grid_utf = grid.encodeSync('utf', {resolution: 4});
            if (process.env.UPDATE) {
                fs.writeFileSync(reference__id__2,JSON.stringify(grid_utf,null,1));
            } else {
                var equal = _c(JSON.stringify(grid_utf,null,1),fs.readFileSync(reference__id__2,'utf8'));
                assert.ok(equal);
            }
            done();
        });
    });

    it('should match expected output if __id__ the grid key and the only attributes', function(done) {
        var map = new mapnik.Map(256, 256);
        map.loadSync(stylesheet, {strict: true});
        map.zoomAll();
        var grid = new mapnik.Grid(map.width, map.height, {key: '__id__'});
        var options = {'layer': 0,
                       'fields': ['__id__']
                      };
        map.render(grid, options, function(err, grid) {
            if (err) throw err;
            var grid_utf = grid.encodeSync('utf', {resolution: 4});
            if (process.env.UPDATE) {
                fs.writeFileSync(reference__id__3,JSON.stringify(grid_utf,null,1));
            } else {
                var equal = _c(JSON.stringify(grid_utf,null,1),fs.readFileSync(reference__id__3,'utf8'));
                assert.ok(equal);
            }
            done();
        });
    });

});
