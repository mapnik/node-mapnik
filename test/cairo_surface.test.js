"use strict";

var test = require('tape');
var mapnik = require('../');

test('should throw with invalid usage', (assert) => {
  // no 'new' keyword
  assert.throws(function() { mapnik.CairoSurface(1, 1); });

  // invalid args
  assert.throws(function() { new mapnik.CairoSurface(); });
  assert.throws(function() { new mapnik.CairoSurface(1); });
  assert.throws(function() { new mapnik.CairoSurface('foo'); });
  assert.throws(function() { new mapnik.CairoSurface('a', 'b', 'c'); });
  assert.throws(function() { new mapnik.CairoSurface(1, 'b', 'c'); });
  assert.end();
});

test('should be initialized properly', (assert) => {
  var im = new mapnik.CairoSurface('SVG',256, 256);
  assert.ok(im instanceof mapnik.CairoSurface);
  assert.equal(im.width(), 256);
  assert.equal(im.height(), 256);
  assert.equal(im.getData(), '');
  assert.end();
});
