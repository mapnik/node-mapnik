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


});
