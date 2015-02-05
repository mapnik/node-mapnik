"use strict";

var mapnik = require('../');
var assert = require('assert');
var fs = require('fs');

describe('mapnik.Image ', function() {
    it('should throw with invalid usage', function() {
        // no 'new' keyword
        assert.throws(function() { mapnik.Image(1, 1); }); // jshint ignore:line

        // invalid args
        assert.throws(function() { new mapnik.Image(); });
        assert.throws(function() { new mapnik.Image(1); });
        assert.throws(function() { new mapnik.Image('foo'); });
        assert.throws(function() { new mapnik.Image('a', 'b', 'c'); });
    });

    it('should throw with filename lacking an extension', function() {
        var im = new mapnik.Image(256, 256);
        assert.throws(function() { im.save('foo'); });
    });

    it('should throw with invalid encoding format 1', function() {
        var im = new mapnik.Image(256, 256);
        assert.throws(function() { im.encodeSync('foo'); });
    });

    it('should throw with invalid encoding format 2', function() {
        var im = new mapnik.Image(256, 256);
        assert.throws(function() { im.save('foo','foo'); });
    });

    it('should throw with invalid binary read from buffer', function() {
        assert.throws(function() { new mapnik.Image.fromBytesSync(new Buffer(0)); });
        assert.throws(function() { new mapnik.Image.fromBytesSync(new Buffer(1024)); });
        var buffer = new Buffer('\x89\x50\x4E\x47\x0D\x0A\x1A\x0A' + new Array(48).join('\0'), 'binary');
        assert.throws(function() { new mapnik.Image.fromBytesSync(buffer); });
        buffer = new Buffer('\x89\x50\x4E\x47\x0D\x0A\x1A\x0A', 'binary');
        assert.throws(function() { new mapnik.Image.fromBytesSync(buffer); });
    });

    it('should throw with invalid encoding format 3', function(done) {
        var im = new mapnik.Image(256, 256);
        im.encode('foo',function(err) {
            assert.ok(err);
            done();
        });
    });

    it('should be initialized properly', function() {
        var im = new mapnik.Image(256, 256);
        assert.ok(im instanceof mapnik.Image);

        assert.equal(im.width(), 256);
        assert.equal(im.height(), 256);
        var v = im.view(0, 0, 256, 256);
        assert.ok(v instanceof mapnik.ImageView);
        assert.equal(v.width(), 256);
        assert.equal(v.height(), 256);
        assert.equal(im.encodeSync().length, v.encodeSync().length);
        var tmp_filename = './test/tmp/image'+Math.random()+'.png';
        im.save(tmp_filename);

        var im2 = new mapnik.Image.open(tmp_filename);
        assert.ok(im2 instanceof mapnik.Image);

        assert.equal(im2.width(), 256);
        assert.equal(im2.height(), 256);

        assert.equal(im.encodeSync().length, im2.encodeSync().length);
    });

    it('should be able to open via byte stream', function(done) {
        var im = new mapnik.Image(256, 256);
        // png
        var tmp_filename1 = './test/tmp/image2'+Math.random()+'.png';
        im.save(tmp_filename1);
        var buffer = fs.readFileSync(tmp_filename1);
        var im2 = new mapnik.Image.fromBytesSync(buffer);
        assert.ok(im2 instanceof mapnik.Image);
        assert.equal(im2.width(), 256);
        assert.equal(im2.height(), 256);
        assert.equal(im.encodeSync("png32").length, im2.encodeSync("png32").length);
        // jpeg
        var tmp_filename2 = './test/tmp/image2'+Math.random()+'.jpeg';
        im.save(tmp_filename2);
        var buffer2 = fs.readFileSync(tmp_filename2);
        var im3 = new mapnik.Image.fromBytesSync(buffer2);
        assert.ok(im3 instanceof mapnik.Image);
        assert.equal(im3.width(), 256);
        assert.equal(im3.height(), 256);
        assert.equal(im.encodeSync("jpeg").length, im3.encodeSync("jpeg").length);
        done();
    });

    it('should be initialized properly via async constructors', function(done) {
        var im = new mapnik.Image(256, 256);
        var tmp_filename = './test/tmp/image3'+Math.random()+'.png';
        im.save(tmp_filename);
        mapnik.Image.open(tmp_filename,function(err,im2) {
            if (err) throw err;
            assert.ok(im2 instanceof mapnik.Image);
            assert.equal(im2.width(), 256);
            assert.equal(im2.height(), 256);
            assert.equal(im.encodeSync().length, im2.encodeSync().length);
            mapnik.Image.fromBytes(im.encodeSync(),function(err,im3) {
                if (err) throw err;
                assert.ok(im3 instanceof mapnik.Image);
                assert.equal(im3.width(), 256);
                assert.equal(im3.height(), 256);
                assert.equal(im.encodeSync("png32").length, im3.encodeSync("png32").length);
                done();
            });
        });
    });


    it('should not be painted after rendering', function(done) {
        var im_blank = new mapnik.Image(4, 4);
        assert.equal(im_blank.painted(), false);

        var m = new mapnik.Map(4, 4);

        m.render(im_blank, {},function(err,im_blank) {
            assert.equal(im_blank.painted(), false);
            done();
        });
    });

    it('should have background set after rendering', function(done) {
        var im_blank2 = new mapnik.Image(4, 4);
        assert.equal(im_blank2.painted(), false);

        var m2 = new mapnik.Map(4, 4);

        m2.background = new mapnik.Color('green');
        m2.render(im_blank2, {},function(err,im_blank2) {
            assert.equal(im_blank2.painted(), false);
            assert.equal(im_blank2.getPixel(0,0).g, 128);
            done();
        });
    });

    it('should be painted after rendering', function(done) {

        var im_blank3 = new mapnik.Image(4, 4);
        assert.equal(im_blank3.painted(), false);

        var m3 = new mapnik.Map(4, 4);
        var s = '<Map>';
        s += '<Style name="points">';
        s += ' <Rule>';
        s += '  <PointSymbolizer />';
        s += ' </Rule>';
        s += '</Style>';
        s += '</Map>';
        m3.fromStringSync(s);

        var mem_datasource = new mapnik.MemoryDatasource({'extent': '-180,-90,180,90'});
        mem_datasource.add({ 'x': 0, 'y': 0 });
        mem_datasource.add({ 'x': 1, 'y': 1 });
        mem_datasource.add({ 'x': 2, 'y': 2 });
        mem_datasource.add({ 'x': 3, 'y': 3 });
        var l = new mapnik.Layer('test');
        l.srs = m3.srs;
        l.styles = ['points'];
        l.datasource = mem_datasource;
        m3.add_layer(l);
        m3.zoomAll();
        m3.render(im_blank3, {},function(err,im_blank3) {
            assert.equal(im_blank3.painted(), true);
            done();
        });
    });

    it('should support setting the alpha channel based on the amount of gray', function() {
        var gray = new mapnik.Image(256, 256);
        gray.fill(new mapnik.Color('white'));
        gray.setGrayScaleToAlpha();
        var gray_view = gray.view(0, 0, gray.width(), gray.height());
        assert.equal(gray_view.isSolidSync(), true);
        var pixel = gray.getPixel(0, 0);
        assert.equal(pixel.r, 255);
        assert.equal(pixel.g, 255);
        assert.equal(pixel.b, 255);
        assert.equal(pixel.a, 255);

        gray.fill(new mapnik.Color('black'));
        gray.setGrayScaleToAlpha();
        var pixel2 = gray.getPixel(0, 0);
        assert.equal(pixel2.r, 255);
        assert.equal(pixel2.g, 255);
        assert.equal(pixel2.b, 255);
        assert.equal(pixel2.a, 0);

        gray.setGrayScaleToAlpha(new mapnik.Color('green'));
        var pixel3 = gray.getPixel(0, 0);
        assert.equal(pixel3.r, 0);
        assert.equal(pixel3.g, 128);
        assert.equal(pixel3.b, 0);
        assert.equal(pixel3.a, 255);
    });

    it('should support setting an individual pixel', function() {
        var gray = new mapnik.Image(256, 256);
        gray.setPixel(0,0,new mapnik.Color('white'));
        var pixel = gray.getPixel(0,0);
        assert.equal(pixel.r, 255);
        assert.equal(pixel.g, 255);
        assert.equal(pixel.b, 255);
        assert.equal(pixel.a, 255);
    });

    it('should support have set_pixel protecting overflow and underflows', function() {
        var imx = new mapnik.Image(4, 4, mapnik.imageType.gray16);
        imx.setPixel(0,0,12);
        imx.setPixel(0,1,-1);
        imx.setPixel(1,0,99999);
        imx.setPixel(1,1,256);
        assert.equal(imx.getPixel(0,0), 12);
        assert.equal(imx.getPixel(0,1), 0);
        assert.equal(imx.getPixel(1,0), 65535);
        assert.equal(imx.getPixel(1,1), 256);
    });

    it('should support casting from gray16 to gray8', function() {
        var im = new mapnik.Image(4, 4, mapnik.imageType.gray16);
        im.setPixel(0,0,12);
        im.setPixel(0,1,-1);
        im.setPixel(1,0,99999);
        im.setPixel(1,1,256);
        var im2= im.castSync(mapnik.imageType.gray8);
        assert.equal(im2.getPixel(0,0), 12);
        assert.equal(im2.getPixel(0,1), 0);
        assert.equal(im2.getPixel(1,0), 255);
        assert.equal(im2.getPixel(1,1), 255);
    });

    it('should support comparing images', function() {
        // if width/height don't match should throw
        assert.throws(function() { new mapnik.Image(256, 256).compare(new mapnik.Image(256, 255)); });
        var one = new mapnik.Image(256, 256);
        // two blank images should exactly match
        assert.equal(one.compare(new mapnik.Image(256, 256)),0);
        // here we set one pixel different
        one.setPixel(0,0,new mapnik.Color('white'));
        assert.equal(one.compare(new mapnik.Image(256, 256)),1);
        // here we set all pixels to be different
        one.fill(new mapnik.Color('white'));
        assert.equal(one.compare(new mapnik.Image(256, 256)),one.width()*one.height());
        // now lets test comparing just rgb and not alpha
        var two = new mapnik.Image(256, 256);
        // white image but fully alpha
        two.fill(new mapnik.Color('rgba(255,255,255,0)'));
        assert.equal(two.getPixel(0,0).r, 255);
        assert.equal(two.getPixel(0,0).g, 255);
        assert.equal(two.getPixel(0,0).b, 255);
        assert.equal(two.getPixel(0,0).a, 0);
        // if we consider alpha all pixels should be different
        assert.equal(one.compare(two),one.width()*one.height());
        // but ignoring alpha all pixels should pass as the same
        assert.equal(one.compare(two,{alpha:false}),0);
        // now lets test the threshold option
        // a minorly different color should trigger differences
        var blank = new mapnik.Image(256, 256);
        var blank2 = new mapnik.Image(256, 256);
        blank2.setPixel(0,0,new mapnik.Color('rgba(16,16,16,0)'));
        // should pass because threshold is 16 by default
        assert.equal(blank.compare(blank2),0);
        // with 15 or below threshold should fail
        assert.equal(blank.compare(blank2,{threshold:15}),1);
    });

    it('should be able to open and save jpeg', function(done) {
        var im = new mapnik.Image(10,10);
        im.fill(new mapnik.Color('green'));
        var filename = './test/data/images/10x10.jpeg';
        // sync open
        assert.equal(0,im.compare(new mapnik.Image.open(filename)));
        // sync fromBytes
        assert.equal(0,im.compare(new mapnik.Image.fromBytesSync(im.encodeSync("jpeg"))));
        // async open
        mapnik.Image.open(filename,function(err,im2) {
            if (err) throw err;
            assert.equal(0,im.compare(im2));
            // async fromBytes
            mapnik.Image.fromBytes(im.encodeSync("jpeg"),function(err,im3) {
                if (err) throw err;
                assert.equal(0,im.compare(im3));
                done();
            });
        });
    });

    it('should be able to open and save tiff', function(done) {
        var im = new mapnik.Image(10,10);
        im.fill(new mapnik.Color('green'));
        var filename = './test/data/images/10x10.tiff';
        // sync open
        assert.equal(0,im.compare(new mapnik.Image.open(filename)));
        // sync fromBytes
        assert.equal(0,im.compare(new mapnik.Image.fromBytesSync(im.encodeSync("tiff"))));
        // async open
        mapnik.Image.open(filename,function(err,im2) {
            if (err) throw err;
            assert.equal(0,im.compare(im2));
            // async fromBytes
            mapnik.Image.fromBytes(im.encodeSync("tiff"),function(err,im3) {
                if (err) throw err;
                assert.equal(0,im.compare(im3));
                done();
            });
        });
    });

    if (mapnik.versions.mapnik_number >= 200300) {
        it('should be able to open and save webp', function(done) {
            var im = new mapnik.Image(10,10);
            im.fill(new mapnik.Color('green'));
            var filename = './test/data/images/10x10.webp';
            // sync open
            assert.equal(0,im.compare(new mapnik.Image.open(filename)));
            // sync fromBytes
            assert.equal(0,im.compare(new mapnik.Image.fromBytesSync(im.encodeSync("webp"))));
            // async open
            mapnik.Image.open(filename,function(err,im2) {
                if (err) throw err;
                assert.equal(0,im.compare(im2));
                // async fromBytes
                mapnik.Image.fromBytes(im.encodeSync("webp"),function(err,im3) {
                    if (err) throw err;
                    assert.equal(0,im.compare(im3));
                    done();
                });
            });
        });
    }

});
