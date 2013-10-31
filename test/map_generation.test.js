var mapnik = require('../');
var assert = require('assert');
var fs = require('fs');
var path = require('path');
var helper = require('./support/helper');
var exists = require('fs').existsSync || require('path').existsSync;

describe('mapnik rendering ', function() {
    it('should render async (blank)', function(done) {
        var map = new mapnik.Map(600, 400);
        assert.ok(map instanceof mapnik.Map);
        map.extent = map.extent;
        var im = new mapnik.Image(map.width, map.height);
        map.render(im, {scale: 1}, function(err, image) {
            assert.ok(image);
            assert.ok(!err);
            var buffer = im.encodeSync('png');
            done();
            //assert.equal(helper.md5(buffer), 'ef33223235b26c782736c88933b35331');
        });
    });

    it('should render async (real data)', function(done) {
        var filename = './test/tmp/renderFile2.png';
        var map = new mapnik.Map(600, 400);
        map.loadSync('./test/stylesheet.xml');
        map.zoomAll();
        map.renderFile(filename, function(error) {
            assert.ok(!error);
            assert.ok(exists(filename));
            done();
        });
    });

    it('should render async to file (png)', function(done) {
        var filename = './test/tmp/renderFile2.png';
        var map = new mapnik.Map(600, 400);
        map.loadSync('./test/stylesheet.xml');
        map.zoomAll();
        map.renderFile(filename, function(error) {
            assert.ok(!error);
            assert.ok(exists(filename));
            done();
        });
    });

    it('should render async to file (cairo format)', function(done) {
        if (mapnik.supports.cairo) {
                var filename = './test/tmp/renderFile2.pdf';
                var map = new mapnik.Map(600, 400);
                map.loadSync('./test/stylesheet.xml');
                map.zoomAll();
                map.renderFile(filename, { format: 'pdf' }, function(error) {
                    assert.ok(!error);
                    assert.ok(exists(filename));
                    done();
                });
        } else { done(); }
    });

    it('should render async to file (guessing format)', function(done) {
        var filename = './test/tmp/renderFile.jpg';
        var map = new mapnik.Map(600, 400);
        map.loadSync('./test/stylesheet.xml');
        map.zoomAll();
        map.renderFile(filename, function(error) {
            assert.ok(!error);
            assert.ok(exists(filename));
            done();
        });
    });

    it('should render async and throw with invalid format', function(done) {
        var filename = './test/tmp/renderFile2.pdf';
        var map = new mapnik.Map(600, 400);
        map.loadSync('./test/stylesheet.xml');
        map.zoomAll();
        try {
            map.renderFile(filename, null, function(error) { });
        } catch (ex) {
            assert.ok(ex);
            done();
        }
    });
});
