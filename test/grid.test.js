"use strict";

var test = require('tape');
var mapnik = require('../');
var path = require('path');

if (mapnik.supports.grid) {

  mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'csv.input'));
  test('should throw with invalid usage', (assert) => {
    // no 'new' keyword
    assert.throws(function() { mapnik.Grid(1, 1); });

    // invalid args
    assert.throws(function() { new mapnik.Grid(); });
    assert.throws(function() { new mapnik.Grid(1); });
    assert.throws(function() { new mapnik.Grid('foo'); });
    assert.throws(function() { new mapnik.Grid('a', 'b', 'c'); });
    assert.throws(function() { new mapnik.Grid(256,256,null); });
    assert.throws(function() { new mapnik.Grid(256,256,{key:null}); });
    assert.end();
  });

  test('should be initialized properly', (assert) => {
    var grid = new mapnik.Grid(256, 256);
    assert.ok(grid instanceof mapnik.Grid);

    assert.equal(grid.width(), 256);
    assert.equal(grid.height(), 256);
    var v = grid.view(0, 0, 256, 256);
    assert.ok(v instanceof mapnik.GridView);
    assert.equal(v.width(), 256);
    assert.equal(v.height(), 256);
    assert.equal(grid.encodeSync({features:true}).length, v.encodeSync().length);
    assert.end();
  });

  test('should fail to encode properly', (assert) => {
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
    assert.end();
  });

  test('should encode properly', (assert) => {
    var grid = new mapnik.Grid(256, 256);
    grid.encode({resolution:4, features:true}, function(err, result) {
      if (err) throw err;
      assert.ok(result);
      assert.end();
    });
  });

  test('should not be painted after rendering', (assert) => {
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
      assert.end();
    });
  });

  test('should be have background applied after rendering', (assert) => {

    var grid_blank = new mapnik.Grid(4, 4);
    var m = new mapnik.Map(4, 4);
    var l = new mapnik.Layer('test');
    m.add_layer(l);
    m.background = new mapnik.Color('green');
    m.render(grid_blank, {layer: 0},function(err,grid_blank2) {
      assert.equal(grid_blank2.painted(), false);
      // TODO - expose grid background
      //assert.ok(grid_blank2.background);
      assert.end();
    });
  });

  test('should be painted after rendering 2', (assert) => {
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
    var ds = new mapnik.Datasource({type:'csv', extent: '-180,-90,180,90', 'inline': "x,y\n0,0\n1,1\n2,2\n3,3"});
    var l = new mapnik.Layer('test');
    l.srs = m3.srs;
    l.styles = ['points'];
    l.datasource = ds;
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
        assert.end();
      });
    });
  });

  test('should be painted after rendering 3', (assert) => {
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
    var ds = new mapnik.Datasource({type:'csv', extent: '-180,-90,180,90', 'inline': "x,y\n0,0\n1,1\n2,2\n3,3"});
    var l = new mapnik.Layer('test');
    l.srs = m3.srs;
    l.styles = ['points'];
    l.datasource = ds;
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
      assert.end();
    });
  });
}
