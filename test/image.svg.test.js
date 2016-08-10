/* global describe it */

'use strict';

var mapnik = require('../');
var assert = require('assert');
var fs = require('fs');

describe('mapnik.Image SVG', function() {
    it('should throw with invalid usage', function() {
        assert.throws(function() { new mapnik.Image.fromSVGSync(); });
        assert.throws(function() { new mapnik.Image.fromSVG(); });
        assert.throws(function() { new mapnik.Image.fromSVGBytes(); });
        assert.throws(function() {
          new mapnik.Image.fromSVGBytesSync(1);
        }, /must provide a buffer argument/);
        assert.throws(function() {
          new mapnik.Image.fromSVGBytesSync({});
        }, /first argument is invalid, must be a Buffer/);
        assert.throws(function() {
          new mapnik.Image.fromSVGBytes(1, function(err, svg) {});
        }, /must provide a buffer argument/);
        assert.throws(function() {
          new mapnik.Image.fromSVGBytes({}, function(err, svg) {});
        }, /first argument is invalid, must be a Buffer/);
        assert.throws(function() {
          new mapnik.Image.fromSVGBytesSync(new Buffer('asdfasdf'));
        }, /SVG parse error:\s+Unable to parse 'asdfasdf'/);
        assert.throws(function() {
          mapnik.Image.fromSVGSync('./test/data/SVG_DOES_NOT_EXIST.svg');
        }, /SVG parse error:\s+Unable to open '.\/test\/data\/SVG_DOES_NOT_EXIST.svg'/);
        assert.throws(function() {
          mapnik.Image.fromSVGSync('./test/data/vector_tile/tile0.expected-svg.svg', 256);
        }, /optional second arg must be an options object/);
        assert.throws(function() {
          mapnik.Image.fromSVG('./test/data/vector_tile/tile0.expected-svg.svg', 256);
        }, /last argument must be a callback function/);
        assert.throws(function() {
          mapnik.Image.fromSVGBytes(new Buffer('asdfasdf'), 256);
        }, /last argument must be a callback function/);
        assert.throws(function() {
          mapnik.Image.fromSVG('./test/data/vector_tile/tile0.expected-svg.svg', 256, function(err, res) {});
        }, /optional second arg must be an options object/);
        assert.throws(function() {
          mapnik.Image.fromSVGBytes(new Buffer('asdfasdf'), 256, function(err, res) {});
        }, /optional second arg must be an options object/);
        assert.throws(function() {
          mapnik.Image.fromSVGSync('./test/data/vector_tile/tile0.expected-svg.svg', { scale: 'foo' });
        }, /'scale' must be a number/);
        assert.throws(function() {
          mapnik.Image.fromSVG('./test/data/vector_tile/tile0.expected-svg.svg', { scale: 'foo' }, function(err, res) {});
        }, /'scale' must be a number/);
        assert.throws(function() {
          mapnik.Image.fromSVGBytes(new Buffer('asdf'), { scale: 'foo' }, function(err, res) {});
        }, /'scale' must be a number/);
        assert.throws(function() {
          mapnik.Image.fromSVGSync('./test/data/vector_tile/tile0.expected-svg.svg', { scale: -1 });
        }, /'scale' must be a positive non zero number/);
        assert.throws(function() {
          mapnik.Image.fromSVG('./test/data/vector_tile/tile0.expected-svg.svg', { scale: -1 }, function(err, res) {});
        }, /'scale' must be a positive non zero number/);
        assert.throws(function() {
          mapnik.Image.fromSVGBytes(new Buffer('asdf'), { scale: -1 }, function(err, res) {});
        }, /'scale' must be a positive non zero number/);
        assert.throws(function() {
          mapnik.Image.fromSVGSync('./test/data/vector_tile/tile0.corrupt-svg.svg');
        }, /image created from svg must have a width and height greater then zero/);
    });

    it('should err with async file w/o width or height', function(done) {
      mapnik.Image.fromSVG('./test/data/vector_tile/tile0.corrupt-svg.svg', function(err, svg) {
        assert.ok(err);
        assert.ok(err.message.match(/image created from svg must have a width and height greater then zero/));
        assert.equal(svg, undefined);
        done();
      });
    });
    
    it('should err with async file w/o width or height as Bytes', function(done) {
        var svgdata = "<svg width='0' height='0'><g id='a'><ellipse fill='#FFFFFF' stroke='#000000' stroke-width='4' cx='50' cy='50' rx='25' ry='25'/></g></svg>";
        var buffer = new Buffer(svgdata);
        mapnik.Image.fromSVGBytes(buffer, function(err, img) {
            assert.ok(err);
            assert.ok(err.message.match(/image created from svg must have a width and height greater then zero/));
            assert.equal(img, undefined);
            done();
        });
    });

    it('should err with async invalid buffer', function(done) {
      mapnik.Image.fromSVGBytes(new Buffer('asdfasdf'), function(err, svg) {
        assert.ok(err);
        assert.ok(err.message.match(/SVG parse error:\s+Unable to parse 'asdfasdf'/));
        assert.equal(svg, undefined);
        done();
      });
    });

    it('should err with async non-existent file', function(done) {
      mapnik.Image.fromSVG('./test/data/SVG_DOES_NOT_EXIST.svg', function(err, svg) {
        assert.ok(err);
        assert.ok(err.message.match(/SVG parse error:\s+Unable to open '.\/test\/data\/SVG_DOES_NOT_EXIST.svg'/));
        assert.equal(svg, undefined);
        done();
      });
    });

    it('should error with async file full of errors', function(done) {
      mapnik.Image.fromSVG('./test/data/vector_tile/errors.svg', function(err, svg) {
        assert.ok(err);
        assert.ok(err.message.match(/SVG parse error:/));
        assert.equal(svg, undefined);
        done();
      });
    });

    it('#fromSVGSync load from SVG file', function() {
        var img = mapnik.Image.fromSVGSync('./test/data/vector_tile/tile0.expected-svg.svg');
        assert.ok(img);
        assert.ok(img instanceof mapnik.Image);
        assert.equal(img.width(), 256);
        assert.equal(img.height(), 256);
        assert.equal(img.encodeSync('png32').length, 17257);
    });
    
    it('#fromSVGSync load from SVG file - 2', function() {
        var img = mapnik.Image.fromSVG('./test/data/vector_tile/tile0.expected-svg.svg');
        assert.ok(img);
        assert.ok(img instanceof mapnik.Image);
        assert.equal(img.width(), 256);
        assert.equal(img.height(), 256);
        assert.equal(img.encodeSync('png32').length, 17257);
    });


    it('#fromSVG load from SVG file', function(done) {
        mapnik.Image.fromSVG('./test/data/vector_tile/tile0.expected-svg.svg', function(err, img) {
            assert.ok(img); assert.ok(img instanceof mapnik.Image);
            assert.equal(img.width(), 256);
            assert.equal(img.height(), 256);
            assert.equal(img.encodeSync('png32').length, 17257);
            assert.equal(img.premultiplied(), false);
            done();
        });
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

    it('#fromSVGBytesSync load from SVG buffer - 2', function() {
        var svgdata = "<svg width='100' height='100'><g id='a'><ellipse fill='#FFFFFF' stroke='#000000' stroke-width='4' cx='50' cy='50' rx='25' ry='25'/></g></svg>";
        var buffer = new Buffer(svgdata);
        var img = mapnik.Image.fromSVGBytes(buffer);
        assert.ok(img);
        assert.ok(img instanceof mapnik.Image);
        assert.equal(img.width(), 100);
        assert.equal(img.height(), 100);
        assert.equal(img.encodeSync("png").length, 1270);
    });

    it('#fromSVGBytes load from SVG buffer', function(done) {
        var svgdata = "<svg width='100' height='100'><g id='a'><ellipse fill='#FFFFFF' stroke='#000000' stroke-width='4' cx='50' cy='50' rx='25' ry='25'/></g></svg>";
        var buffer = new Buffer(svgdata);
        mapnik.Image.fromSVGBytes(buffer, function(err, img) {
            assert.ok(img);
            assert.ok(img instanceof mapnik.Image);
            assert.equal(img.width(), 100);
            assert.equal(img.height(), 100);
            assert.equal(img.encodeSync("png").length, 1270);
            assert.equal(img.premultiplied(), false);
            done();
        });
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
        assert.equal(img.premultiplied(), false);
    });

});
