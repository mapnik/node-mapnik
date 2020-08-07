"use strict";

var test = require('tape');
var mapnik = require('../');
var path = require('path');

test('should throw with invalid usage', (assert) => {
  // no 'new' keyword
  assert.throws(function() { mapnik.Layer('foo'); });
  // invalid args
  assert.throws(function() { new mapnik.Layer(); });
  assert.throws(function() { new mapnik.Layer('foo', null); });
  assert.throws(function() { new mapnik.Layer(1); });
  assert.throws(function() { new mapnik.Layer('a', 'b', 'c'); });
  assert.throws(function() { new mapnik.Layer(new mapnik.Layer('foo')); });
  assert.end();
});

test('should initialize properly', (assert) => {
  mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'shape.input'));
  var layer = new mapnik.Layer('foo', '+init=epsg:4326');
  assert.equal(layer.name, 'foo');
  layer.active = true;
  assert.equal(layer.active, true);
  layer.queryable = false;
  assert.equal(layer.queryable, false);
  layer.clear_label_cache = false;
  assert.equal(layer.clear_label_cache, false);
  layer.minimum_scale_denominator = 1;
  assert.equal(layer.minimum_scale_denominator, 1);
  layer.maximum_scale_denominator = 50;
  assert.equal(layer.maximum_scale_denominator, 50);
  assert.throws(function() { layer.name = null; });
  assert.throws(function() { layer.srs = null; });
  assert.throws(function() { layer.styles = null; });
  assert.throws(function() { layer.datasource = null; });
  assert.throws(function() { layer.datasource = {}; });
  assert.throws(function() { layer.active = null; });
  assert.throws(function() { layer.minimum_scale_denominator = null; });
  assert.throws(function() { layer.maximum_scale_denominator = null; });
  assert.throws(function() { layer.queryable = null; });
  assert.throws(function() { layer.clear_label_cache = null; });
  assert.equal(layer.srs, '+init=epsg:4326');
  assert.deepEqual(layer.styles, []);
  // will be empty/undefined
  assert.ok(!layer.datasource);
  var options = {
    type: 'shape',
    file: './test/data/world_merc.shp'
  };

  var ds = new mapnik.Datasource(options);
  layer.datasource = ds;
  //assert.ok(layer.datasource instanceof mapnik.Datasource);

  // json representation
  var meta = layer.describe();
  assert.equal(meta.minimum_scale_denominator, 1);
  assert.equal(meta.maximum_scale_denominator, 50);
  assert.equal(meta.active, true);
  assert.equal(meta.queryable, false);
  assert.equal(meta.clear_label_cache, false);
  assert.equal(meta.name, 'foo');
  assert.equal(meta.srs, '+init=epsg:4326');
  assert.deepEqual(meta.styles, []);
  assert.deepEqual(meta.datasource, options);
  assert.end();
});
