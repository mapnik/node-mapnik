"use strict";


var test = require('tape');
var mapnik = require('../');

var path = require('path');

if (mapnik.supports.grid) {

  var grid;
  var view;

  test('setup', (assert) => {
    grid = new mapnik.Grid(256, 256);
    view = grid.view(0, 0, 256, 256);
    assert.end();
  });

  test('should fail to initialize view', (assert) => {
    assert.throws(function() { grid.view(); });
    assert.throws(function() { mapnik.GridView(); });
    assert.throws(function() { new mapnik.GridView(); });
    assert.end();
  });

  test('should fail to encode properly', (assert) => {
    assert.throws(function() { view.encodeSync("foo"); });
    assert.throws(function() { view.encode("foo"); });
    assert.throws(function() { view.encode("foo", function(err, result) {}); });
    assert.throws(function() { view.encodeSync({features:null}); });
    assert.throws(function() { view.encodeSync({resolution:null}); });
    assert.throws(function() { view.encodeSync({resolution:0}); });
    assert.throws(function() { view.encode({features:null}, function(err, result) {}); });
    assert.throws(function() { view.encode({resolution:null}, function(err, result) {}); });
    assert.throws(function() { view.encode({resolution:0}, function(err, result) {}); });
    assert.throws(function() { view.encode({resolution:4}, null); });
    assert.end();
  });

  test('should encode properly', (assert) => {
    view.encode({resolution:4, features:true}, function(err, result) {
      if (err) throw err;
      assert.ok(result);
      assert.end();
    });
  });

  test('should support fields method', (assert) => {
    assert.deepEqual([], view.fields());
    assert.end();
  });

  test('isSolid should fail with bad input', (assert) => {
    assert.throws(function(){ view.isSolid(null) });
    var view2 = grid.view(0, 0, 0, 0);
    view2.isSolid(function(err, solid, pixel) {
      assert.throws(function() { if (err) throw err; });
      assert.end();
    });
  });

  test('should be solid', (assert) => {
    assert.equal(view.isSolidSync(), true);
    assert.equal(view.isSolid(), true);
    assert.end();
  });

  test('should fail with bad input on getPixel', (assert) => {
    assert.throws(function() { view.getPixel(); });
    assert.throws(function() { view.getPixel(0); });
    assert.throws(function() { view.getPixel('x',0); });
    assert.throws(function() { view.getPixel(0,'x'); });
    // undefined for outside range of image
    assert.equal(undefined, view.getPixel(999,999));
    assert.end();
  });

  test('should be solid (async)', (assert) => {
    view.isSolid(function(err,solid,pixel) {
      assert.equal(solid, true);
      if (mapnik.versions.mapnik_number < 200100) {
        assert.equal(pixel, 0);
      } else {
        assert.ok(pixel == -1 * 0x7FFFFFFFFFFFFFFF || pixel == -2147483648);
        assert.ok(pixel.toFixed() == mapnik.Grid.base_mask.toFixed() || pixel == -2147483648);
      }
      assert.end();
    });
  });

  test('should report grid base_mask value for pixel', (assert) => {
    var pixel = view.getPixel(0, 0);
    if (mapnik.versions.mapnik_number < 200100) {
      assert.equal(pixel, 0);
    } else {
      assert.ok(pixel == -1 * 0x7FFFFFFFFFFFFFFF || pixel == -2147483648);
      assert.ok(pixel.toFixed() == mapnik.Grid.base_mask.toFixed() || pixel == -2147483648);
    }
    assert.end();
  });

  test('should be painted after rendering', (assert) => {
    mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'shape.input'));
    var map = new mapnik.Map(256, 256);
    map.loadSync('./test/stylesheet.xml');
    map.zoomAll();
    var options = {'layer': 0,
                   'fields': ['NAME']
                  };
    var grid = new mapnik.Grid(map.width, map.height, {key: '__id__'});
    map.render(grid, options, function(err, grid) {
      var view = grid.view(0, 0, 256, 256);
      assert.deepEqual(options['fields'], view.fields());
      assert.equal(view.isSolidSync(), false);
      // hit alaska (USA is id 207)
      assert.equal(view.getPixel(25, 100), 207);
      view.isSolid(function(err,solid,pixel){
        assert.equal(solid, false);
        assert.equal(pixel, undefined);
        assert.end();
      });
    });
  });
}
