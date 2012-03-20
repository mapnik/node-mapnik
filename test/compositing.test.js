var mapnik = require('mapnik');
var path = require('path');

exports['test compositing ops'] = function(beforeExit, assert) {
    rendered = 0;
    for (var name in mapnik.compositeOp) {
        // http://blog.mixu.net/2011/02/03/javascript-node-js-and-for-loops/
        (function(name) {
            var im1 = mapnik.Image.open('test/support/a.png');
            var im2 = mapnik.Image.open('test/support/b.png');
            im1.composite(im2,mapnik.compositeOp[name], function(err,im_out) {
                if (err) throw err;
                assert.ok(im_out);
                rendered++;
                var out = 'test/tmp/' + name + '.png';
                im_out.save(out);
                assert.ok(path.existsSync(out));
            })
        })(name)
    }

    beforeExit(function() {
        assert.equal(rendered, 28);
    });

};
