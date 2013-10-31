var mapnik = require('../');
var assert = require('assert');
var fs = require('fs');
var path = require('path');

describe('mapnik.ImageView ', function() {
    it('should throw with invalid usage', function() {
        // no 'new' keyword
        assert.throws(function() { mapnik.Image(1, 1); });

        // invalid args
        assert.throws(function() { new mapnik.Image(); });
        assert.throws(function() { new mapnik.Image(1); });
        assert.throws(function() { new mapnik.Image('foo'); });
        assert.throws(function() { new mapnik.Image('a', 'b', 'c'); });
    });

    it('should be initialized properly', function() {
        var im = new mapnik.Image(256, 256);
        var view = im.view(0, 0, 256, 256);
        assert.equal(view.isSolidSync(), true);
        var pixel = view.getPixel(0, 0);
        assert.equal(pixel.r, 0);
        assert.equal(pixel.g, 0);
        assert.equal(pixel.b, 0);
        assert.equal(pixel.a, 0);

        im = new mapnik.Image(256, 256);
        im.background = new mapnik.Color(2, 2, 2, 2);
        view = im.view(0, 0, 256, 256);
        assert.equal(view.isSolidSync(), true);
        pixel = view.getPixel(0, 0);
        assert.equal(pixel.r, 2);
        assert.equal(pixel.g, 2);
        assert.equal(pixel.b, 2);
        assert.equal(pixel.a, 2);
        assert.equal(view.getPixel(99999999, 9999999), undefined);
    });

    it('isSolid async works if true', function(done) {
        var im = new mapnik.Image(256, 256);
        var view = im.view(0, 0, 256, 256);
        assert.equal(view.isSolidSync(), true);
        view.isSolid(function(err,solid,pixel) {
            assert.equal(solid, true);
            assert.equal(pixel, 0);
            done();
        });
    });

    it('isSolid async works if true and white', function(done) {
        var im = new mapnik.Image(256, 256);
        var color = new mapnik.Color('white');
        im.background = color
        var view = im.view(0, 0, 256, 256);
        assert.equal(view.isSolidSync(), true);
        view.isSolid(function(err,solid,pixel) {
            assert.equal(solid, true);
            assert.equal(pixel, 4294967295);
            // NOTE: shifts are 32 bit signed ints in js, so creating the unsigned
            // rgba for white is not possible using normal bit ops
            // var rgba = (color.a << 24) | (color.b << 16) | (color.g << 8) | (color.r);
            // how about this? (from tilelive source)
            var rgba = color.a*(1<<24) + ((color.b<<16) | (color.g<<8) | color.r);
            assert.equal(pixel, rgba);
            done();
        });
    });

    it('isSolid async works if false', function(done) {
        var im = new mapnik.Image.open('./test/support/a.png');
        var view = im.view(0, 0, im.width(), im.height());
        assert.equal(view.isSolidSync(), false);
        view.isSolid(function(err,solid,pixel) {
            assert.equal(solid, false);
            assert.equal(pixel, undefined);
            done();
        });
    });

});
