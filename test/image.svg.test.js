/* global describe it */

'use strict';

var mapnik = require('../');
var assert = require('assert');
var fs = require('fs');

describe('mapnik.Image SVG', function() {
    it('should throw with invalid usage', function() {
        assert.throws(function() { new mapnik.Image.fromSVGSync(); });
        assert.throws(function() { new mapnik.Image.fromSVG(); });
        assert.throws(function() {
          new mapnik.Image.fromSVGBytesSync(1);
        }, /must provide a buffer argument/);
        assert.throws(function() {
          new mapnik.Image.fromSVGBytesSync({});
        }, /first argument is invalid, must be a Buffer/);
        assert.throws(function() {
          new mapnik.Image.fromSVGBytesSync(new Buffer('asdfasdf'));
        }, /can not parse SVG buffer/);
        assert.throws(function() {
          mapnik.Image.fromSVGSync('./test/data/SVG_DOES_NOT_EXIST.svg');
        }, /can not parse SVG file/);
        assert.throws(function() {
          mapnik.Image.fromSVGSync('./test/data/vector_tile/tile0.expected-svg.svg', 256);
        }, /optional second arg must be an options object/);
        assert.throws(function() {
          mapnik.Image.fromSVGSync('./test/data/vector_tile/tile0.expected-svg.svg', { scale: 'foo' });
        }, /'scale' must be a number/);
        assert.throws(function() {
          mapnik.Image.fromSVGSync('./test/data/vector_tile/tile0.expected-svg.svg', { scale: -1 });
        }, /'scale' must be a positive non zero number/);
        assert.throws(function() {
          mapnik.Image.fromSVGSync('./test/data/vector_tile/tile0.corrupt-svg.svg');
        }, /image created from svg must have a width and height greater then zero/);
    });

    it('#fromSVGSync load from SVG file', function() {
        var img = mapnik.Image.fromSVGSync('./test/data/vector_tile/tile0.expected-svg.svg');
        assert.ok(img);
        assert.ok(img instanceof mapnik.Image);
        assert.equal(img.width(), 256);
        assert.equal(img.height(), 256);
        assert.equal(img.encodeSync('png32').length, 17272);
    });

    it('#fromSVGBytesSync load from SVG buffer', function() {
        var svgdata = "<svg width='100' height='100'><g id='a'><ellipse fill='#FFFFFF' stroke='#000000' stroke-width='4' cx='50' cy='50' rx='25' ry='25'/></g></svg>";
        var buffer = new Buffer(svgdata);
        var img = mapnik.Image.fromSVGBytesSync(buffer);
        assert.ok(img);
        assert.ok(img instanceof mapnik.Image);
        assert.equal(img.width(), 100);
        assert.equal(img.height(), 100);
        assert.equal(img.encodeSync("png").length, 1270);
    });

    it('svg scaling', function() {
        var svgdata = "<svg width='100' height='100'><g id='a'><ellipse fill='#FFFFFF' stroke='#000000' stroke-width='4' cx='50' cy='50' rx='25' ry='25'/></g></svg>";
        var buffer = new Buffer(svgdata);
        var img = mapnik.Image.fromSVGBytesSync(buffer, { scale: 0.5});
        assert.ok(img);
        assert.ok(img instanceof mapnik.Image);
        assert.equal(img.width(), 50);
        assert.equal(img.height(), 50);
        assert.equal(img.encodeSync("png").length, 616);
    });
});
