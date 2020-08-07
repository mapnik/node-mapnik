"use strict";

var test = require('tape');
var mapnik = require('../');
var fs = require('fs');

test('should have valid settings', (assert) => {
  assert.ok(mapnik.settings);
  assert.ok(mapnik.settings.paths);
  assert.ok(mapnik.settings.paths.fonts.length);
  //assert.ok(fs.statSync(mapnik.settings.paths.fonts));
  assert.ok(mapnik.settings.paths.input_plugins.length);
  assert.ok(mapnik.settings.paths.mapnik_index.length);
  assert.ok(mapnik.settings.paths.shape_index.length);
  assert.ok(fs.statSync(mapnik.settings.paths.input_plugins));

  /* has version info */
  assert.ok(mapnik.versions);
  //assert.ok(mapnik.versions.node);
  //assert.ok(mapnik.versions.v8);
  assert.ok(mapnik.versions.mapnik);
  assert.ok(mapnik.versions.mapnik_number);
  assert.ok(mapnik.versions.boost);
  assert.ok(mapnik.versions.boost_number);

  assert.ok(mapnik.Geometry.Point, 1);
  assert.ok(mapnik.Geometry.LineString, 2);
  assert.ok(mapnik.Geometry.Polygon, 3);
  assert.end();
});

test('should have valid version info', (assert) => {
  /* has version info */
  assert.ok(mapnik.versions);
  //assert.ok(mapnik.versions.node);
  //assert.ok(mapnik.versions.v8);
  assert.ok(mapnik.versions.mapnik);
  assert.ok(mapnik.versions.mapnik_number);
  assert.ok(mapnik.versions.boost);
  assert.ok(mapnik.versions.boost_number);

  assert.ok(mapnik.Geometry.Point, 1);
  assert.ok(mapnik.Geometry.LineString, 2);
  assert.ok(mapnik.Geometry.Polygon, 3);
  assert.end();
});

test('should expose Geometry enums', (assert) => {
  assert.ok(mapnik.Geometry.Point, 1);
  assert.ok(mapnik.Geometry.LineString, 2);
  assert.ok(mapnik.Geometry.Polygon, 3);
  assert.end();
});
