var mapnik = require('mapnik');
var assert = require('assert');
var path = require('path');

describe('mapnik async rendering', function() {
    it('should render to a file', function(done) {
        var map = new mapnik.Map(600, 400);
        var filename = './test/tmp/renderFile.png';
        map.renderFile(filename, function(error) {
            assert.ok(!error);
            assert.ok(path.existsSync(filename));
            done();
        });
    });

    it('should render to an image', function(done) {
        var map = new mapnik.Map(256, 256);
        map.load('./examples/stylesheet.xml', function(err,map) {
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
