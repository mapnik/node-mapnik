"use strict";

var test = require('tape');
var mapnik = require('../');


test('get_severity should return default', (assert) => {
  assert.equal(mapnik.Logger.getSeverity(), mapnik.Logger.ERROR);
  assert.end();
});

test('test that you cant initialize a logger', (assert) => {
  assert.throws(function() { var l = new mapnik.Logger(); });
  assert.end();
});

test('set_severity should fail with bad input', (assert) => {
  assert.throws(function() { mapnik.Logger.setSeverity(); });
  assert.throws(function() { mapnik.Logger.setSeverity(null); });
  assert.throws(function() { mapnik.Logger.setSeverity(2,3); });
  assert.end();
});

test('set_severity should set mapnik.logger', (assert) => {
  var orig_severity = mapnik.Logger.getSeverity();
  mapnik.Logger.setSeverity(mapnik.Logger.NONE);
  assert.equal(mapnik.Logger.getSeverity(), mapnik.Logger.NONE);
  mapnik.Logger.setSeverity(orig_severity);
  assert.end();
});
