"use strict";

var mapnik = require('../');
var assert = require('assert');
var exists = require('fs').existsSync || require('path').existsSync;

describe('mapnik.compositeOp', function() {
    for (var name in mapnik.compositeOp) {
        // http://blog.mixu.net/2011/02/03/javascript-node-js-and-for-loops/
        (function(name) {
            it('should blend image correctly with op:' + name, function(done) {
                var im1 = mapnik.Image.open('test/support/a.png');
                //im1.premultiplySync();
                var im2 = mapnik.Image.open('test/support/b.png');
                //im2.premultiplySync();
                im2.composite(im1, {comp_op:mapnik.compositeOp[name], opacity:100, dx:0, dy:0}, function(err,im_out) {
                    if (err) throw err;
                    assert.ok(im_out);
                    var out = './test/tmp/' + name + '.png';
                    im_out.demultiplySync();
                    im_out.save(out);
                    assert.ok(exists(out));
                    done();
                });
            });
        })(name); // jshint ignore:line
    }

    it('should fail with bad parameters', function(done) {
        var im1 = mapnik.Image.open('test/support/a.png');
        var im2 = mapnik.Image.open('test/support/b.png');
        var im3 = new mapnik.Image(5,5,mapnik.imageType.null);
        assert.throws(function() { im2.composite(); });
        assert.throws(function() { im2.composite(null); });
        assert.throws(function() { im2.composite({}); });
        assert.throws(function() { im2.composite(im1, null); });
        assert.throws(function() { im2.composite(im1, null, function(err, im_out) {}); });
        assert.throws(function() { im2.composite(im1, {comp_op:null}, function(err, im_out) {}); });
        assert.throws(function() { im2.composite(im1, {opacity:null}, function(err, im_out) {}); });
        assert.throws(function() { im2.composite(im1, {dx:null}, function(err, im_out) {}); });
        assert.throws(function() { im2.composite(im1, {dy:null}, function(err, im_out) {}); });
        assert.throws(function() { im2.composite(im1, {image_filters:null}, function(err, im_out) {}); });
        assert.throws(function() { im2.composite(im1, {image_filters:'foo'}, function(err, im_out) {}); });
        im3.composite(im1, {}, function(err, im_out) {
            assert.throws(function() { if (err) throw err; });
            done();
        });
    });
});

describe('mapnik.compositeOp async multiply', function() {
    for (var name in mapnik.compositeOp) {
        // http://blog.mixu.net/2011/02/03/javascript-node-js-and-for-loops/
        (function(name) {
            it('should blend image correctly with op:' + name, function(done) {
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
                                done();
                            });
                        });
                    });
                });
            });
        })(name); // jshint ignore:line
    }
});
