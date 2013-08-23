var mapnik = require('../');
var assert = require('assert');
var fs = require('fs');
var path = require('path');

describe('mapnik.Grid ', function() {
    it('should throw with invalid usage', function() {
        // no 'new' keyword
        assert.throws(function() { mapnik.Grid(1, 1); });

        // invalid args
        assert.throws(function() { new mapnik.Grid(); });
        assert.throws(function() { new mapnik.Grid(1); });
        assert.throws(function() { new mapnik.Grid('foo'); });
        assert.throws(function() { new mapnik.Grid('a', 'b', 'c'); });
    });

    it('should be initialized properly', function() {
        var grid = new mapnik.Grid(256, 256);
        assert.ok(grid instanceof mapnik.Grid);

        assert.equal(grid.width(), 256);
        assert.equal(grid.height(), 256);
        var v = grid.view(0, 0, 256, 256);
        assert.ok(v instanceof mapnik.GridView);
        assert.equal(v.width(), 256);
        assert.equal(v.height(), 256);
        assert.equal(grid.encodeSync().length, v.encodeSync().length);
    });

    it('should not be painted after rendering', function(done) {
        var grid_blank = new mapnik.Grid(4, 4);
        assert.equal(grid_blank.painted(), false);
        assert.equal(grid_blank.background, undefined);
        var m = new mapnik.Map(4, 4);
        var l = new mapnik.Layer('test');
        m.add_layer(l);
        m.render(grid_blank, {layer: 0},function(err,grid_blank) {
            assert.equal(grid_blank.painted(), false);
            // TODO - expose grid background
            assert.equal(grid_blank.background, undefined);
            done();
        });
    });

    it('should be have background applied after rendering', function(done) {

        var grid_blank = new mapnik.Grid(4, 4);
        var m = new mapnik.Map(4, 4);
        var l = new mapnik.Layer('test');
        m.add_layer(l);
        m.background = new mapnik.Color('green');
        m.render(grid_blank, {layer: 0},function(err,grid_blank2) {
            assert.equal(grid_blank.painted(), false);
            // TODO - expose grid background
            //assert.ok(grid_blank2.background);
            done();
        });
    });

    it('should be painted after rendering2', function(done) {
        var grid_blank3 = new mapnik.Grid(4, 4);
        assert.equal(grid_blank3.painted(), false);
        assert.equal(grid_blank3.background, undefined);
        var m3 = new mapnik.Map(4, 4);
        var s = '<Map>';
        s += '<Style name="points">';
        s += ' <Rule>';
        s += '  <PointSymbolizer />';
        s += ' </Rule>';
        s += '</Style>';
        s += '</Map>';
        m3.fromStringSync(s);
        var mem_datasource = new mapnik.MemoryDatasource({'extent': '-180,-90,180,90'});
        mem_datasource.add({ 'x': 0, 'y': 0 });
        mem_datasource.add({ 'x': 1, 'y': 1 });
        mem_datasource.add({ 'x': 2, 'y': 2 });
        mem_datasource.add({ 'x': 3, 'y': 3 });
        var l = new mapnik.Layer('test');
        l.srs = m3.srs;
        l.styles = ['points'];
        l.datasource = mem_datasource;
        m3.add_layer(l);
        m3.zoomAll();
        m3.render(grid_blank3, {layer: 0},function(err,grid_blank3) {
            assert.equal(grid_blank3.painted(), true);
            // TODO - expose grid background
            //assert.equal(grid_blank3.background,undefined);
            done();
        });
    });
});
