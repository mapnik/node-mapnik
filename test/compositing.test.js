var mapnik = require('mapnik');
var assert = require('assert');
var path = require('path');

describe('mapnik.compositeOp', function() {
    for (var name in mapnik.compositeOp) {
        // http://blog.mixu.net/2011/02/03/javascript-node-js-and-for-loops/
        (function(name) {
            var im1 = mapnik.Image.open('test/support/a.png');
            var im2 = mapnik.Image.open('test/support/b.png');
            it('should blend image correctly with op:' + name, function(done) {
                im1.composite(im2,mapnik.compositeOp[name], function(err,im_out) {
                    if (err) throw err;
                    assert.ok(im_out);
                    var out = 'test/tmp/' + name + '.png';
                    im_out.save(out);
                    assert.ok(path.existsSync(out));
                    done();
                });
            });
        })(name)
    }
});
