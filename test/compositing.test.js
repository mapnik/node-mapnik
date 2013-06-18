var mapnik = require('mapnik');
var assert = require('assert');
var exists = require('fs').existsSync || require('path').existsSync;

describe('mapnik.compositeOp', function() {
    for (var name in mapnik.compositeOp) {
        // http://blog.mixu.net/2011/02/03/javascript-node-js-and-for-loops/
        (function(name) {
            it('should blend image correctly with op:' + name, function(done) {
                var im1 = mapnik.Image.open('test/support/a.png');
                im1.premultiplySync();
                var im2 = mapnik.Image.open('test/support/b.png');
                im2.premultiplySync();
                im2.composite(im1, {comp_op:mapnik.compositeOp[name]}, function(err,im_out) {
                    if (err) throw err;
                    assert.ok(im_out);
                    var out = 'test/tmp/' + name + '.png';
                    im_out.demultiplySync();
                    im_out.save(out);
                    assert.ok(exists(out));
                    done();
                });
            });
        })(name);
    }
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
                        im2.composite(im1, {comp_op:mapnik.compositeOp[name], filters:'invert agg-stack-blur(10,10)'}, function(err,im_out) {
                            if (err) throw err;
                            assert.ok(im_out);
                            var out = 'test/tmp/' + name + '-async.png';
                            im_out.demultiply(function(err,im_out) {
                                im_out.save(out);
                                assert.ok(exists(out));
                                done();
                            });
                        });
                    });
                });
            });
        })(name);
    }
});
