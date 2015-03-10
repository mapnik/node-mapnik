"use strict";

var mapnik = require('../');
var assert = require('assert');

describe('mapnik.ImageView ', function() {
    it('should throw with invalid usage', function() {
        // no 'new' keyword
        assert.throws(function() { mapnik.ImageView(1, 1); });
        assert.throws(function() { new mapnik.ImageView(); });
    });

    it('should be initialized properly', function() {
        var im = new mapnik.Image(256, 256);
        var view = im.view(0, 0, 256, 256);
        assert.equal(view.isSolidSync(), true);
        var pixel = view.getPixel(0, 0, {get_color:true});
        assert.equal(pixel.r, 0);
        assert.equal(pixel.g, 0);
        assert.equal(pixel.b, 0);
        assert.equal(pixel.a, 0);

        im = new mapnik.Image(256, 256);
        im.fill(new mapnik.Color(2, 2, 2, 2));
        view = im.view(0, 0, 256, 256);
        assert.equal(view.isSolidSync(), true);
        pixel = view.getPixel(0, 0, {get_color:true});
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
        im.fill(color);
        var view = im.view(0, 0, 256, 256);
        assert.equal(view.isSolidSync(), true);
        view.isSolid(function(err,solid,pixel) {
            assert.equal(solid, true);
            assert.equal(pixel, 4294967295);
            done();
        });
    });

    it('isSolid async works if false', function(done) {
        var im = new mapnik.Image.open('./test/support/a.png');
        var view = im.view(0, 0, im.width(), im.height());
        assert.equal(view.isSolid(), false);
        assert.throws(function() { view.isSolid(null); });
        assert.equal(view.isSolidSync(), false);
        view.isSolid(function(err,solid,pixel) {
            assert.equal(solid, false);
            assert.equal(pixel, undefined);
            done();
        });
    });

    it('isSolid should fail with bad parameters', function(done) {
        var im = new mapnik.Image(0,0);
        var view = im.view(0,0,0,0);
        assert.throws(function() { view.isSolidSync(); });
        view.isSolid(function(err,solid,pixel) {
            assert.throws(function() { if (err) throw err; });
            done();
        });
    });

    it('getPixel should fail with bad parameters', function() {
        var im = new mapnik.Image(4,4,mapnik.imageType.rgba8);
        im.fill(1);
        var view = im.view(0,0,4,4);
        assert.throws(function() { view.getPixel(); });
        assert.throws(function() { view.getPixel(1); });
        assert.throws(function() { view.getPixel(1,'2'); });
        assert.throws(function() { view.getPixel('1',2); });
        assert.throws(function() { view.getPixel(1,2, null); });
        assert.throws(function() { view.getPixel(1,2, {get_color:1}); });
    });
    
    it('getPixel supports rgba8', function() {
        var im = new mapnik.Image(4,4,mapnik.imageType.rgba8);
        im.fill(1);
        var view = im.view(0,0,4,4);
        assert.equal(view.getPixel(0,0), 1);
    });

    it('getPixel supports gray8', function() {
        var im = new mapnik.Image(4,4,mapnik.imageType.gray8);
        im.fill(1);
        var view = im.view(0,0,4,4);
        assert.equal(view.getPixel(0,0), 1);
    });

    it('getPixel supports gray8s', function() {
        var im = new mapnik.Image(4,4,mapnik.imageType.gray8s);
        im.fill(1);
        var view = im.view(0,0,4,4);
        assert.equal(view.getPixel(0,0), 1);
    });

    it('getPixel supports gray16', function() {
        var im = new mapnik.Image(4,4,mapnik.imageType.gray16);
        im.fill(1);
        var view = im.view(0,0,4,4);
        assert.equal(view.getPixel(0,0), 1);
    });

    it('getPixel supports gray16s', function() {
        var im = new mapnik.Image(4,4,mapnik.imageType.gray16s);
        im.fill(1);
        var view = im.view(0,0,4,4);
        assert.equal(view.getPixel(0,0), 1);
    });

    it('getPixel supports gray32', function() {
        var im = new mapnik.Image(4,4,mapnik.imageType.gray32);
        im.fill(1);
        var view = im.view(0,0,4,4);
        assert.equal(view.getPixel(0,0), 1);
    });

    it('getPixel supports gray32s', function() {
        var im = new mapnik.Image(4,4,mapnik.imageType.gray32s);
        im.fill(1);
        var view = im.view(0,0,4,4);
        assert.equal(view.getPixel(0,0), 1);
    });

    it('getPixel supports gray32f', function() {
        var im = new mapnik.Image(4,4,mapnik.imageType.gray32f);
        im.fill(1);
        var view = im.view(0,0,4,4);
        assert.equal(view.getPixel(0,0), 1);
    });

    it('getPixel supports gray64', function() {
        var im = new mapnik.Image(4,4,mapnik.imageType.gray64);
        im.fill(1);
        var view = im.view(0,0,4,4);
        assert.equal(view.getPixel(0,0), 1);
    });

    it('getPixel supports gray64s', function() {
        var im = new mapnik.Image(4,4,mapnik.imageType.gray64s);
        im.fill(1);
        var view = im.view(0,0,4,4);
        assert.equal(view.getPixel(0,0), 1);
    });

    it('getPixel supports gray64f', function() {
        var im = new mapnik.Image(4,4,mapnik.imageType.gray64f);
        im.fill(1);
        var view = im.view(0,0,4,4);
        assert.equal(view.getPixel(0,0), 1);
    });

    it('should throw with invalid encoding', function(done) {
        var im = new mapnik.Image(256, 256);
        var view = im.view(0,0,256,256);
        assert.throws(function() { view.encodeSync('foo'); });
        assert.throws(function() { view.encodeSync(1); });
        assert.throws(function() { view.encodeSync('png', null); });
        assert.throws(function() { view.encodeSync('png', {palette:null}); });
        assert.throws(function() { view.encodeSync('png', {palette:{}}); });
        assert.throws(function() { view.encode('png', {palette:{}}, function(err, result) {}); });
        assert.throws(function() { view.encode('png', {palette:null}, function(err, result) {}); });
        assert.throws(function() { view.encode('png', null, function(err, result) {}); });
        assert.throws(function() { view.encode(1, {}, function(err, result) {}); });
        assert.throws(function() { view.encode('png', {}, null); });
        view.encode('foo', {}, function(err, result) {
            assert.throws(function() { if (err) throw err; });
            done();
        });
    });
    
    it('should encode with a pallete', function(done) {
        var im = new mapnik.Image(256, 256);
        var view = im.view(0,0,256,256);
        var pal = new mapnik.Palette('\xff\x09\x93\xFF\x01\x02\x03\x04');
        assert.ok(view.encodeSync('png', {palette:pal}));
        view.encode('png', {palette:pal}, function(err, result) {
            if (err) throw err;
            assert.ok(result);
            done();
        });
    });
    
    it('should throw with invalid formats', function() {
        var im = new mapnik.Image(256, 256);
        var view = im.view(0,0,256,256);
        assert.throws(function() { view.save('foo','foo'); });
        assert.throws(function() { view.save(); });
        assert.throws(function() { view.save('file.png', null); });
        assert.throws(function() { view.save('foo'); });
    });

    if (mapnik.supports.webp) {
        it('should support webp encoding', function(done) {
            var im = new mapnik.Image(256,256);
            im.fill(new mapnik.Color('green'));
            im.encode('webp',function(err,buf1) { // jshint ignore:line
                if (err) throw err;
                var v = im.view(0,0,256,256);
                v.encode('webp', function(err,buf2) { // jshint ignore:line
                    if (err) throw err;
                    // disabled because this is not stable across mapnik versions or webp versions
                    //assert.equal(buf1.length,buf2.length);
                    done();
                });
            });
        });
    }

});
