var mapnik = require('../');
var assert = require('assert');
var exists = require('fs').existsSync || require('path').existsSync;

describe('mapnik.compositeOp', function() {
    for (var name in mapnik.compositeOp) {
        // http://blog.mixu.net/2011/02/03/javascript-node-js-and-for-loops/
        (function(name) {
            var im1 = mapnik.Image.open('test/support/a.png');
            im1.premultiply();
            it('should blend image correctly with op:' + name, function(done) {
                var im2 = mapnik.Image.open('test/support/b.png');
                im2.premultiply();
                im2.composite(im1, mapnik.compositeOp[name], function(err,im_out) {
                    if (err) throw err;
                    assert.ok(im_out);
                    var out = 'test/tmp/' + name + '.png';
                    im_out.demultiply();
                    im_out.save(out);
                    assert.ok(exists(out));
                    done();
                });
            });
        })(name);
    }
});
