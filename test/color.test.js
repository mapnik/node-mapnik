'use strict';

var test = require('tape');
var mapnik = require('../');

test('should throw with invalid usage', (assert) => {
  // no 'new' keyword
  assert.throws(function() { mapnik.Color(); });
  // invalid args
  assert.throws(function() { new mapnik.Color(); });
  assert.throws(function() { new mapnik.Color(1); });
  assert.throws(function() { new mapnik.Color('foo'); });
  assert.throws(function() { new mapnik.Color(-1,0,0); });
  assert.throws(function() { new mapnik.Color(256,0,0); });
  assert.throws(function() { new mapnik.Color(256,0,0,true); });
  assert.throws(function() { new mapnik.Color(0,0,0,-1); });
  assert.throws(function() { new mapnik.Color(0,0,0,-1,true); });
  assert.end();
});


test('should throw for invalid property setting', (assert) => {
  var c = new mapnik.Color('green');
  assert.throws(function() { c.g = -1; });
  assert.throws(function() { c.r = 256; });
  assert.throws(function() { c.b = 'waffle'; });
  assert.throws(function() { c.a = null; });
  assert.throws(function() { c.premultiplied = null; });
  assert.throws(function() { c.premultiplied = 1.0; });
  assert.throws(function() { c.premultiplied = 'true'; });
  assert.end();
});


test('should be green via keyword', (assert) => {
  var c = new mapnik.Color('green');
  assert.equal(c.r, 0);
  assert.equal(c.g, 128);
  assert.equal(c.b, 0);
  assert.equal(c.a, 255);
  assert.equal(c.hex(), '#008000');
  assert.equal(c.premultiplied, false);
  assert.equal(c.toString(), 'rgb(0,128,0)');
  var c = new mapnik.Color('green', true);
  assert.equal(c.r, 0);
  assert.equal(c.g, 128);
  assert.equal(c.b, 0);
  assert.equal(c.a, 255);
  assert.equal(c.hex(), '#008000');
  assert.equal(c.premultiplied, true);
  assert.equal(c.toString(), 'rgb(0,128,0)');
  assert.end();
});


test('should be gray via rgb', (assert) => {
  var c = new mapnik.Color(0, 128, 0);
  assert.equal(c.r, 0);
  assert.equal(c.g, 128);
  assert.equal(c.b, 0);
  assert.equal(c.a, 255);
  assert.equal(c.premultiplied, false);
  assert.equal(c.hex(), '#008000');
  assert.equal(c.toString(), 'rgb(0,128,0)');
  var c = new mapnik.Color(0, 128, 0, true);
  assert.equal(c.r, 0);
  assert.equal(c.g, 128);
  assert.equal(c.b, 0);
  assert.equal(c.a, 255);
  assert.equal(c.premultiplied, true);
  assert.equal(c.hex(), '#008000');
  assert.equal(c.toString(), 'rgb(0,128,0)');
  assert.end();
});

test('should be gray via rgba', (assert) => {
  var c = new mapnik.Color(0, 128, 0, 255);
  assert.equal(c.r, 0);
  assert.equal(c.g, 128);
  assert.equal(c.b, 0);
  assert.equal(c.a, 255);
  assert.equal(c.premultiplied, false);
  assert.equal(c.hex(), '#008000');
  assert.equal(c.toString(), 'rgb(0,128,0)');
  var c = new mapnik.Color(0, 128, 0, 255, true);
  assert.equal(c.r, 0);
  assert.equal(c.g, 128);
  assert.equal(c.b, 0);
  assert.equal(c.a, 255);
  assert.equal(c.premultiplied, true);
  assert.equal(c.hex(), '#008000');
  assert.equal(c.toString(), 'rgb(0,128,0)');
  assert.end();
});


test('should be gray via rgba %', (assert) => {
  var c = new mapnik.Color('rgba(0%,50%,0%,1)');
  assert.equal(c.r, 0);
  assert.equal(c.g, 128);
  assert.equal(c.b, 0);
  assert.equal(c.a, 255);
  assert.equal(c.premultiplied, false);
  assert.equal(c.hex(), '#008000');
  assert.equal(c.toString(), 'rgb(0,128,0)');
  var c = new mapnik.Color('rgba(0%,50%,0%,1)', true);
  assert.equal(c.r, 0);
  assert.equal(c.g, 128);
  assert.equal(c.b, 0);
  assert.equal(c.a, 255);
  assert.equal(c.premultiplied, true);
  assert.end();
});

test('should have all property setting working', (assert) => {
  var c = new mapnik.Color(0,0,0,0);
  assert.equal(c.r, 0);
  assert.equal(c.g, 0);
  assert.equal(c.b, 0);
  assert.equal(c.a, 0);
  assert.equal(c.premultiplied, false);
  c.r = 1;
  c.g = 2;
  c.b = 3;
  c.a = 4;
  c.premultiplied = true;
  assert.equal(c.r, 1);
  assert.equal(c.g, 2);
  assert.equal(c.b, 3);
  assert.equal(c.a, 4);
  assert.equal(c.premultiplied, true);
  assert.end();
});
