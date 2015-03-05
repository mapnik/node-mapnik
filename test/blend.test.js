"use strict";

var mapnik = require('../');
var assert = require('assert');
var fs = require('fs');

var images = [
    fs.readFileSync('test/blend-fixtures/1.png'),
    fs.readFileSync('test/blend-fixtures/2.png')
];

var images_bad = [
    fs.readFileSync('test/blend-fixtures/not_a_real_image.txt'),
    fs.readFileSync('test/blend-fixtures/not_a_real_image.txt')
];

var images_alpha = [
    fs.readFileSync('test/blend-fixtures/1a.png'),
    fs.readFileSync('test/blend-fixtures/2a.png')
];

describe('mapnik.blend', function() {

    it('blend fails', function() {
        assert.throws(function() { mapnik.blend(); });
        assert.throws(function() { mapnik.blend(images); });
        assert.throws(function() { mapnik.blend(images, null); });
        assert.throws(function() { mapnik.blend(images, null, function(err, result) {}); });
        assert.throws(function() { mapnik.blend(images, {}, null); });
        assert.throws(function() { mapnik.blend(images, {format:'foo'}, function(err, result) {}); });
        assert.throws(function() { mapnik.blend([1,2], function(err, result) {}); });
        assert.throws(function() { mapnik.blend(images_bad, function(err, result) {}); if (err) throw err;});
        assert.throws(function() { mapnik.blend(images, {matte:'foo'}, function(err, result) {}); });
        assert.throws(function() { mapnik.blend(images, {matte:''}, function(err, result) {}); });
        assert.throws(function() { mapnik.blend(images, {matte:'#0000000'}, function(err, result) {}); });
        assert.throws(function() { mapnik.blend(images, {compression:20}, function(err, result) {}); });
        assert.throws(function() { mapnik.blend(images, {compression:'foo'}, function(err, result) {}); });
    });

    it('blended png', function(done) {
        var expected = new mapnik.Image.open('test/blend-fixtures/expected.png');
        mapnik.blend(images, function(err, result) {
            if (err) throw err;
            var actual = new mapnik.Image.fromBytesSync(result);
            //actual.save('test/blend-fixtures/actual.png')
            assert.equal(0,expected.compare(actual));
            done();
        });
    });
    
    it('blended png with palette', function(done) {
        var pal = new mapnik.Palette(fs.readFileSync('./test/support/palettes/palette256.act'), 'act');
        var expected = new mapnik.Image.open('test/blend-fixtures/expected-palette-256.png');
        mapnik.blend(images, {palette:pal}, function(err, result) {
            if (err) throw err;
            var actual = new mapnik.Image.fromBytesSync(result);
            //fs.writeFileSync('test/blend-fixtures/expected-palette-256.png',result);
            assert.equal(0,expected.compare(actual));
            done();
        });
    });

    it('blended png with quality - paletted - octree', function(done) {
        var expected = new mapnik.Image.open('test/blend-fixtures/expected-oct-palette-256.png');
        mapnik.blend(images, {quality:256, mode:"octree"}, function(err, result) {
            if (err) throw err;
            var actual = new mapnik.Image.fromBytesSync(result);
            //fs.writeFileSync('test/blend-fixtures/actual-oct-palette-256.png',result);
            assert.equal(0,expected.compare(actual));
            done();
        });
    });
    
    it('blended png with quality - paletted - hextree', function(done) {
        var expected = new mapnik.Image.open('test/blend-fixtures/expected-hex-palette-256.png');
        mapnik.blend(images_alpha, {quality:256, mode:"hextree"}, function(err, result) {
            if (err) throw err;
            var actual = new mapnik.Image.fromBytesSync(result);
            //fs.writeFileSync('test/blend-fixtures/actual-hex-palette-256.png',result);
            assert.equal(0,expected.compare(actual));
            done();
        });
    });

    it('blended png with compression', function(done) {
        var expected = new mapnik.Image.open('test/blend-fixtures/expected.png');
        mapnik.blend(images, {compression:8}, function(err, result) {
            if (err) throw err;
            var actual = new mapnik.Image.fromBytesSync(result);
            //actual.save('test/blend-fixtures/actual.png')
            assert.equal(0,expected.compare(actual));
            done();
        });
    });
    
    it('blended with matte 8', function(done) {
        var expected = new mapnik.Image.open('test/blend-fixtures/expected-matte-8.png');
        mapnik.blend(images_alpha, {matte: '#12345678'}, function(err, result) {
            if (err) throw err;
            var actual = new mapnik.Image.fromBytesSync(result);
            //actual.save('test/blend-fixtures/actual-matte-8.png')
            assert.equal(0,expected.compare(actual));
            done();
        });
    });

    it('blended with matte 6', function(done) {
        var expected = new mapnik.Image.open('test/blend-fixtures/expected-matte-6.png');
        mapnik.blend(images, {matte: '123456'}, function(err, result) {
            if (err) throw err;
            var actual = new mapnik.Image.fromBytesSync(result);
            //actual.save('test/blend-fixtures/actual-matte-6.png')
            assert.equal(0,expected.compare(actual));
            done();
        });
    });

    it('blended pass format png', function(done) {
        assert.throws(function() { 
            mapnik.blend(images, {format:"png", quality:999}, function(err, result) {});
        });
        var expected = new mapnik.Image.open('test/blend-fixtures/expected.png');
        mapnik.blend(images, {format:"png"}, function(err, result) {
            if (err) throw err;
            var actual = new mapnik.Image.fromBytesSync(result);
            //actual.save('test/blend-fixtures/actual.png')
            assert.equal(0,expected.compare(actual));
            done();
        });
    });
    
    it('blended pass format jpeg', function(done) {

        assert.throws(function() { 
            mapnik.blend(images, {format:"jpeg", quality:101}, function(err, result) {});
        });
        var expected = new mapnik.Image.open('test/blend-fixtures/expected.jpg');
        mapnik.blend(images, {format:"jpeg", quality:85}, function(err, result) {
            if (err) throw err;
            var actual = new mapnik.Image.fromBytesSync(result);
            //fs.writeFileSync('test/blend-fixtures/actual.jpg',result);
            assert.equal(0,expected.compare(actual));
            done();
        });
    });
    
    it('blended pass format webp', function(done) {
        assert.throws(function() { 
            mapnik.blend(images, {format:"webp", quality:999}, function(err, result) {});
        });
        var expected = new mapnik.Image.open('test/blend-fixtures/expected.webp');
        mapnik.blend(images, {format:"webp"}, function(err, result) {
            if (err) throw err;
            var actual = new mapnik.Image.fromBytesSync(result);
            //fs.writeFileSync('test/blend-fixtures/actual.webp',result);
            assert.equal(0,expected.compare(actual));
            done();
        });
    });
    
    it('blended pass format webp with compression', function(done) {
        assert.throws(function() { 
            mapnik.blend(images, {format:"webp", compression:999}, function(err, result) {});
        });
        var expected = new mapnik.Image.open('test/blend-fixtures/expected-compression-5.webp');
        mapnik.blend(images, {format:"webp", compression:5}, function(err, result) {
            if (err) throw err;
            var actual = new mapnik.Image.fromBytesSync(result);
            //fs.writeFileSync('test/blend-fixtures/actual-compression-5.webp',result);
            assert.equal(0,expected.compare(actual));
            done();
        });
    });

    it('hsl to rgb works properly', function() {
        // Assert throws on bad parameters
        assert.throws(function() { mapnik.hsl2rgb(); });
        assert.throws(function() { mapnik.hsl2rgb(1); });
        assert.throws(function() { mapnik.hsl2rgb(1,2); });
        assert.throws(function() { mapnik.hsl2rgb(1,2,'3'); });
        
        var c = new mapnik.Color('green');
        var result = mapnik.hsl2rgb(1/3,1,0.25098039215686274);
        assert(Math.abs(result[0] - c.r) < 1e-7);
        assert(Math.abs(result[1] - c.g) < 1e-7);
        assert(Math.abs(result[2] - c.b) < 1e-7);
    });
    
    it('rgb to hsl works properly', function() {
        // Assert throws on bad parameters
        assert.throws(function() { mapnik.rgb2hsl(); });
        assert.throws(function() { mapnik.rgb2hsl(1); });
        assert.throws(function() { mapnik.rgb2hsl(1,2); });
        assert.throws(function() { mapnik.rgb2hsl(1,2,'3'); });
        
        var c = new mapnik.Color('green');
        var result = mapnik.rgb2hsl(c.r,c.g,c.b);
        assert(Math.abs(result[0] - 1/3) < 1e-7);
        assert(Math.abs(result[1] - 1) < 1e-7);
        assert(Math.abs(result[2] - 0.25098039215686274) < 1e-7);
    });


});
