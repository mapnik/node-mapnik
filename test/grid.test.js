"use strict";

var mapnik = require('../');
var assert = require('assert');

describe('mapnik.Grid ', function() {
    it('should throw with invalid usage', function() {
        // no 'new' keyword
        assert.throws(function() { mapnik.Grid(1, 1); });

        // invalid args
        assert.throws(function() { new mapnik.Grid(); });
        assert.throws(function() { new mapnik.Grid(1); });
        assert.throws(function() { new mapnik.Grid('foo'); });
        assert.throws(function() { new mapnik.Grid('a', 'b', 'c'); });
        assert.throws(function() { new mapnik.Grid(256,256,null); });
        assert.throws(function() { new mapnik.Grid(256,256,{key:null}); });
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
        assert.equal(grid.encodeSync({features:true}).length, v.encodeSync().length);
    });

    it('should fail to encode properly', function() {
        var grid = new mapnik.Grid(256, 256);
        assert.throws(function() { grid.encodeSync("foo"); });
        assert.throws(function() { grid.encode("foo"); });
        assert.throws(function() { grid.encode("foo", function(err, result) {}); });
        assert.throws(function() { grid.encodeSync({features:null}); });
        assert.throws(function() { grid.encodeSync({resolution:null}); });
        assert.throws(function() { grid.encodeSync({resolution:0}); });
        assert.throws(function() { grid.encode({features:null}, function(err, result) {}); });
        assert.throws(function() { grid.encode({resolution:null}, function(err, result) {}); });
        assert.throws(function() { grid.encode({resolution:0}, function(err, result) {}); });
        assert.throws(function() { grid.encode({resolution:4}, null); });
    });

    it('should encode properly', function(done) {
        var grid = new mapnik.Grid(256, 256);
        grid.encode({resolution:4, features:true}, function(err, result) {
            if (err) throw err;
            assert.ok(result);
            done();
        });
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
            //assert.equal(grid_blank.background, undefined);
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
            assert.equal(grid_blank2.painted(), false);
            // TODO - expose grid background
            //assert.ok(grid_blank2.background);
            done();
        });
    });

    it('should be painted after rendering 2', function(done) {
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
            grid_blank3.key = "stuff";
            assert.throws(function() { grid_blank3.key = null; });
            assert.throws(function() { grid_blank3.clear(null); });
            grid_blank3.clear(function(err, grid_blank4) {
                assert.equal(grid_blank4.key, "stuff");
                
                assert.equal(grid_blank4.painted(), false);
                done();
            });
        });
    });

    it('should be painted after rendering 3', function(done) {
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
        var mem_datasource = new mapnik.MemoryDatasource({extent: '-180,-90,180,90', somefield:'value'});
        mem_datasource.add({ 'x': 0, 'y': 0, properties: {feat_id:1,null_val:null,name:"name"}});
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
            assert.throws(function() { grid_blank3.addField(null); });
            assert.throws(function() { grid_blank3.addField(); });
            grid_blank3.addField("foo");
            assert.deepEqual(grid_blank3.fields(), ["foo"]);
            // TODO - expose grid background
            //assert.equal(grid_blank3.background,undefined);
            grid_blank3.clear();
            assert.equal(grid_blank3.painted(), false);
            grid_blank3.clearSync();
            assert.equal(grid_blank3.painted(), false);
            done();
        });
    });
});
