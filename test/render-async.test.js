var mapnik = require('../');
var assert = require('assert');
var exists = require('fs').existsSync || require('path').existsSync;

describe('mapnik async rendering', function() {
    it('should render to a file', function(done) {
        var map = new mapnik.Map(600, 400);
        var filename = './test/tmp/renderFile.png';
        map.renderFile(filename, function(error) {
            assert.ok(!error);
            assert.ok(exists(filename));
            done();
        });
    });

    it('should render to an image', function(done) {
        var map = new mapnik.Map(256, 256);
        map.load('./test/stylesheet.xml', function(err,map) {
            map.zoomAll();
            var im = new mapnik.Image(map.width, map.height);
            map.render(im, function(err, im) {
                im.encode('png', function(err,buffer) {
                    var string = im.toString();
                    assert.ok(string);
                    done();
                });
            });
        });
    });
});
