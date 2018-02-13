/* global describe it */

'use strict';

var mapnik = require('../');
var assert = require('assert');
var fs = require('fs');

describe('mapnik.Image SVG Meta', function() {
    it('should throw with invalid usage', function() {
        assert.throws(function() { mapnik.Image.parseSVGMetaBytesSync(); });
        assert.throws(function() { mapnik.Image.parseSVGMetaBytesSync('fail'); });
        assert.throws(function() { mapnik.Image.parseSVGMetaBytesSync(null); });
        assert.throws(function() { mapnik.Image.parseSVGMetaBytesSync({}); });
        assert.throws(function() {
            new mapnik.Image.parseSVGMetaBytesSync(new Buffer('asdfasdf'));
        }, /SVG parse error:\s+Unable to parse 'asdfasdf'/);
        assert.throws(function() {
            mapnik.Image.fromSVGSync('./test/data/vector_tile/tile0.corrupt-svg.svg');
        }, /image created from svg must have a width and height greater then zero/);
    });

    it('should return an object with width and height', function(done) { 
        fs.readFile('./test/data/vector_tile/tile0.expected-svg.svg', function(err, buffer) {
            assert.ifError(err); 
            var meta = mapnik.Image.parseSVGMetaBytesSync(buffer);
            assert.equal(256, meta.width);
            assert.equal(256, meta.height);
            done();
        });
    });

    it('should handle svgs with a really really large size', function(done) {
        var svgdata = "<svg width='2147483647' height='2147483647'><g id='a'><ellipse fill='#FFFFFF' stroke='#000000' stroke-width='4' cx='50' cy='50' rx='25' ry='25'/></g></svg>";
        var buffer = new Buffer(svgdata);
        var meta = mapnik.Image.parseSVGMetaBytesSync(buffer);
        assert.equal(2147483647, meta.width);
        assert.equal(2147483647, meta.height);
        done();
    });
});
