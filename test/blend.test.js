"use strict";

var mapnik = require('../');
var assert = require('assert');
var fs = require('fs');

var images = [
    fs.readFileSync('test/blend-fixtures/1.png'),
    fs.readFileSync('test/blend-fixtures/2.png')
];

describe('mapnik.blend', function() {

    it('blend fails', function() {
        assert.throws(function() { mapnik.blend(); });
        assert.throws(function() { mapnik.blend(images); });
        assert.throws(function() { mapnik.blend(images, null); });
        assert.throws(function() { mapnik.blend(images, null, function(err, result) {}); });
        assert.throws(function() { mapnik.blend(images, {}, null); });
        assert.throws(function() { mapnik.blend(images, {format:'foo'}, function(err, result) {}); });
    });

    it('blended', function(done) {
        var expected = new mapnik.Image.open('test/blend-fixtures/expected.png');
        mapnik.blend(images, function(err, result) {
            if (err) throw err;
            var actual = new mapnik.Image.fromBytesSync(result);
            //actual.save('test/blend-fixtures/actual.png')
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
