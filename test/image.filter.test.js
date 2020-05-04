"use strict";

var test = require('tape');
var mapnik = require('../');

test('should throw with invalid usage', (assert) => {
  var im = new mapnik.Image(3,3);
  assert.throws(function() { im.filter(); });
  assert.throws(function() { im.filterSync(); });
  assert.throws(function() { im.filter(null); });
  assert.throws(function() { im.filterSync(null); });
  assert.throws(function() { im.filter(null, function(err,im) {}); });
  assert.throws(function() { im.filter('blur', null); });
  assert.throws(function() { im.filterSync('notrealfilter'); });
  assert.throws(function() { im.filter('notrealfilter'); });
  im.filter('notrealfilter', function(err,im) {
    assert.throws(function() { if (err) throw err; });
    assert.end();
  });
});

test('should blur image - sync', (assert) => {
  var im = new mapnik.Image(3,3);
  im.fill(new mapnik.Color('blue'));
  im.setPixel(1,1,new mapnik.Color('red'));
  im.filterSync('blur');
  assert.equal(im.getPixel(0,0), 4291166264);
  assert.equal(im.getPixel(0,1), 4293001244);
  assert.equal(im.getPixel(0,2), 4291166264);
  assert.equal(im.getPixel(1,0), 4291166264);
  assert.equal(im.getPixel(1,1), 4293001244);
  assert.equal(im.getPixel(1,2), 4291166264);
  assert.equal(im.getPixel(2,0), 4291166264);
  assert.equal(im.getPixel(2,1), 4293001244);
  assert.equal(im.getPixel(2,2), 4291166264);
  assert.end();
});

test('should blur image - async', (assert) => {
  var im = new mapnik.Image(3,3);
  im.fill(new mapnik.Color('blue'));
  im.setPixel(1,1,new mapnik.Color('red'));
  im.filter('blur', function(err,im) {
    assert.equal(im.getPixel(0,0), 4291166264);
    assert.equal(im.getPixel(0,1), 4293001244);
    assert.equal(im.getPixel(0,2), 4291166264);
    assert.equal(im.getPixel(1,0), 4291166264);
    assert.equal(im.getPixel(1,1), 4293001244);
    assert.equal(im.getPixel(1,2), 4291166264);
    assert.equal(im.getPixel(2,0), 4291166264);
    assert.equal(im.getPixel(2,1), 4293001244);
    assert.equal(im.getPixel(2,2), 4291166264);
    assert.end();
  });
});
