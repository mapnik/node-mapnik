var mapnik = require('mapnik');
var assert = require('assert');
var fs = require('fs');
var path = require('path');

describe('mapnik.CairoSurface ', function() {
    it('should throw with invalid usage', function() {
        // no 'new' keyword
        assert.throws(function() { mapnik.CairoSurface(1, 1); });

        // invalid args
        assert.throws(function() { new mapnik.CairoSurface(); });
        assert.throws(function() { new mapnik.CairoSurface(1); });
        assert.throws(function() { new mapnik.CairoSurface('foo'); });
        assert.throws(function() { new mapnik.CairoSurface('a', 'b', 'c'); });
    });

    it('should be initialized properly', function() {
        var im = new mapnik.CairoSurface('SVG',256, 256);
        assert.ok(im instanceof mapnik.CairoSurface);
        assert.equal(im.width(), 256);
        assert.equal(im.height(), 256);
        assert.equal(im.getData(), '');
    });

});
