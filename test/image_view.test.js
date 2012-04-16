var mapnik = require('mapnik');
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
        assert.equal(view.isSolid(), true);
        var pixel = view.getPixel(0, 0);
        assert.equal(pixel.r, 0);
        assert.equal(pixel.g, 0);
        assert.equal(pixel.b, 0);
        assert.equal(pixel.a, 0);

        im = new mapnik.Image(256, 256);
        im.background = new mapnik.Color(2, 2, 2, 2);
        view = im.view(0, 0, 256, 256);
        assert.equal(view.isSolid(), true);
        pixel = view.getPixel(0, 0);
        assert.equal(pixel.r, 2);
        assert.equal(pixel.g, 2);
        assert.equal(pixel.b, 2);
        assert.equal(pixel.a, 2);
        assert.equal(view.getPixel(99999999, 9999999), undefined);
    });
});
