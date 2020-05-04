"use strict";

var test = require('tape');
var mapnik = require('../');
var exists = require('fs').existsSync || require('path').existsSync;

for (var name in mapnik.compositeOp) {
  // http://blog.mixu.net/2011/02/03/javascript-node-js-and-for-loops/
  (function(name) {
    test('should blend image correctly with op:' + name, (assert) => {
      var im1 = mapnik.Image.open('test/support/a.png');
      im1.premultiplySync();
      var im2 = mapnik.Image.open('test/support/b.png');
      im2.premultiplySync();
      im2.composite(im1, {comp_op:mapnik.compositeOp[name], opacity:1, dx:0, dy:0}, function(err,im_out) {
        if (err) throw err;
        assert.ok(im_out);
        var out = './test/tmp/' + name + '.png';
        im_out.demultiplySync();
        im_out.save(out);
        assert.ok(exists(out));
        assert.end();
      });
    });
  })(name); // jshint ignore:line
}

test('should fail with bad parameters', (assert) => {
  var im1 = mapnik.Image.open('test/support/a.png');
  im1.premultiply();
  assert.equal(im1.premultiplied(), true);
  var im2 = mapnik.Image.open('test/support/b.png');
  im2.premultiply();
  assert.equal(im2.premultiplied(), true);
  var im3 = new mapnik.Image(5,5,{type:mapnik.imageType.null});
  var im4 = mapnik.Image.open('test/support/a.png');
  assert.equal(im4.premultiplied(), false);
  var im5 = mapnik.Image.open('test/data/images/sat_image.tif');
  assert.throws(function() { im2.composite(); });
  assert.throws(function() { im2.composite(null); });
  assert.throws(function() { im2.composite({}); });
  assert.throws(function() { im2.composite(im1, null); });
  assert.throws(function() { im2.composite(im1, null, function(err, im_out) {}); });
  assert.throws(function() { im2.composite(im1, {comp_op:null}, function(err, im_out) {}); });
  assert.throws(function() { im2.composite(im1, {comp_op:999}, function(err, im_out) {}); });
  assert.throws(function() { im2.composite(im1, {comp_op:-9}, function(err, im_out) {}); });
  assert.throws(function() { im2.composite(im1, {opacity:null}, function(err, im_out) {}); });
  assert.throws(function() { im2.composite(im1, {opacity:1000}, function(err, im_out) {}); });
  assert.throws(function() { im2.composite(im1, {opacity:-1000}, function(err, im_out) {}); });
  assert.throws(function() { im2.composite(im1, {dx:null}, function(err, im_out) {}); });
  assert.throws(function() { im2.composite(im1, {dy:null}, function(err, im_out) {}); });
  assert.throws(function() { im2.composite(im1, {image_filters:null}, function(err, im_out) {}); });
  assert.throws(function() { im2.composite(im1, {image_filters:'foo'}, function(err, im_out) {}); });
  // Fails because im3 is null - will not be premultiplied ever.
  assert.throws(function() { im3.composite(im2, {}, function(err, im_out) {}); });
  // Fails due to not being premultiplied
  assert.throws(function() { im2.composite(im4, {}, function(err, im_out) {}); });
  // Fails due to not being premultiplied
  assert.throws(function() { im4.composite(im2, {}, function(err, im_out) {}); });
  // Fails because gray8 images are not supported
  im5.composite(im5, {}, function(err, im_out) {
    assert.throws(function() { if (err) throw err; });
    assert.end();
  });
});


for (var name in mapnik.compositeOp) {
  // http://blog.mixu.net/2011/02/03/javascript-node-js-and-for-loops/
  (function(name) {
    test('should blend image correctly with op:' + name, (assert) => {
      var im1 = mapnik.Image.open('test/support/a.png');
      im1.premultiply(function(err,im1) {
        var im2 = mapnik.Image.open('test/support/b.png');
        im2.premultiply(function(err,im2) {
          im2.composite(im1, {comp_op:mapnik.compositeOp[name], image_filters:'invert agg-stack-blur(10,10)'}, function(err,im_out) {
            if (err) throw err;
            assert.ok(im_out);
            var out = './test/tmp/' + name + '-async.png';
            im_out.demultiply(function(err,im_out) {
              im_out.save(out);
              assert.ok(exists(out));
              assert.end();
            });
          });
        });
      });
    });
  })(name); // jshint ignore:line
}
