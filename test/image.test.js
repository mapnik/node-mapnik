"use strict";

var mapnik = require('../');
var assert = require('assert');
var fs = require('fs');
var path = require('path');

mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'gdal.input'));

describe('mapnik.Image ', function() {

    it('should throw with invalid usage', function() {
        // no 'new' keyword
        assert.throws(function() { mapnik.Image(1, 1); }); // jshint ignore:line

        // invalid args
        assert.throws(function() { new mapnik.Image(); });
        assert.throws(function() { new mapnik.Image(1); });
        assert.throws(function() { new mapnik.Image(256,256, 999); });
        assert.throws(function() { new mapnik.Image(256,256, null); });
        assert.throws(function() { new mapnik.Image(256,256,{premultiplied:'asdf'}); });
        assert.throws(function() { new mapnik.Image(256,256,{initialize:null}); });
        assert.throws(function() { new mapnik.Image(256,256,{painted:null}); });
        assert.throws(function() {
          new mapnik.Image(256,256, {type:'foo'});
        }, /'type' option must be a valid 'mapnik.imageType'/);
        assert.throws(function() { new mapnik.Image(256,256,{type:null}); });
        assert.throws(function() { new mapnik.Image(256,256,{type:999}); });
        assert.throws(function() { new mapnik.Image('foo'); });
        assert.throws(function() { new mapnik.Image('a', 'b', 'c'); });
    });

    it('should initialize image successfully with options', function() {
        var options = {
            premultiplied: false,
            initialize: true,
            painted: false,
            type: mapnik.imageType.gray8
        };
        var im1 = new mapnik.Image(256,256,options);
        assert.ok(im1);
        // Check that it is the same image type, also tests getType
        assert.equal(im1.getType(), mapnik.imageType.gray8);
    });
    
    it('should throw with invalid encoding', function(done) {
        var im = new mapnik.Image(256, 256);
        assert.throws(function() { im.encodeSync('foo'); });
        assert.throws(function() { im.encodeSync(1); });
        assert.throws(function() { im.encodeSync('png', null); });
        assert.throws(function() { im.encodeSync('png', {palette:null}); });
        assert.throws(function() { im.encodeSync('png', {palette:{}}); });
        assert.throws(function() { im.encode('png', {palette:{}}, function(err, result) {}); });
        assert.throws(function() { im.encode('png', {palette:null}, function(err, result) {}); });
        assert.throws(function() { im.encode('png', null, function(err, result) {}); });
        assert.throws(function() { im.encode(1, {}, function(err, result) {}); });
        assert.throws(function() { im.encode('png', {}, null); });
        im.encode('foo', {}, function(err, result) {
            assert.throws(function() { if (err) throw err; });
            done();
        });
    });
    
    it('should encode with a pallete', function(done) {
        var im = new mapnik.Image(256, 256);
        var pal = new mapnik.Palette(new Buffer('\xff\x09\x93\xFF\x01\x02\x03\x04','ascii'));
        assert.ok(im.encodeSync('png', {palette:pal}));
        im.encode('png', {palette:pal}, function(err, result) {
            if (err) throw err;
            assert.ok(result);
            done();
        });
    });

    it('should throw with invalid formats and bad input', function(done) {
        var im = new mapnik.Image(256, 256);
        assert.throws(function() { im.save('foo','foo'); });
        assert.throws(function() { im.save(); });
        assert.throws(function() { im.save('file.png', null); });
        assert.throws(function() { im.save('foo'); });
        assert.throws(function() { im.saveSync(); });
        assert.throws(function() { im.saveSync('foo','foo'); });
        assert.throws(function() { im.saveSync('file.png', null); });
        assert.throws(function() { im.saveSync('foo'); });
        assert.throws(function() { im.save(function(err) {}); });
        assert.throws(function() { im.save('file.png', null, function(err) {}); });
        assert.throws(function() { im.save('foo', function(err) {}); });
        im.save('foo','foo', function(err) {
            assert.throws(function() { if (err) throw err; });
            done();
        });
    });

    it('should throw with invalid binary read from buffer', function(done) {
        assert.throws(function() { new mapnik.Image.fromBytesSync(); });
        assert.throws(function() { new mapnik.Image.fromBytes(); });
        assert.throws(function() { new mapnik.Image.fromBytes(null); });
        assert.throws(function() { new mapnik.Image.fromBytes(null, function(err, result) {}); });
        assert.throws(function() { new mapnik.Image.fromBytes({}); });
        assert.throws(function() { new mapnik.Image.fromBytes({}, function(err, result) {}); });
        assert.throws(function() { new mapnik.Image.fromBytesSync({}); });
        assert.throws(function() { new mapnik.Image.fromBytes(new Buffer(0)); });
        assert.throws(function() { new mapnik.Image.fromBytes(new Buffer(0), null); });
        assert.throws(function() { new mapnik.Image.fromBytesSync(new Buffer(1024)); });
        var buffer = new Buffer('\x89\x50\x4E\x47\x0D\x0A\x1A\x0A' + new Array(48).join('\0'), 'binary');
        assert.throws(function() { new mapnik.Image.fromBytesSync(buffer); });
        buffer = new Buffer('\x89\x50\x4E\x47\x0D\x0A\x1A\x0A', 'binary');
        assert.throws(function() { new mapnik.Image.fromBytesSync(buffer); });
        var stuff = new mapnik.Image.fromBytes(buffer, function(err, result) {
            assert.throws(function() { if (err) throw err; });
            done();
        });
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
        assert.throws(function() { im.view(); });
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
        im.save(tmp_filename2, 'jpeg', function(err) {
            var buffer2 = fs.readFileSync(tmp_filename2);
            var im3 = new mapnik.Image.fromBytesSync(buffer2);
            assert.ok(im3 instanceof mapnik.Image);
            assert.equal(im3.width(), 256);
            assert.equal(im3.height(), 256);
            assert.equal(im.encodeSync("jpeg").length, im3.encodeSync("jpeg").length);
            done();
        });
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

    it('should support premultiply and demultiply', function(done) {
        var im = new mapnik.Image(5,5);
        assert.equal(im.premultiplied(), false);
        im.premultiplySync();
        assert.equal(im.premultiplied(), true);
        im.demultiplySync();
        assert.equal(im.premultiplied(), false);
        assert.throws(function() { im.premultiply(null) });
        im.premultiply();
        assert.equal(im.premultiplied(), true);
        assert.throws(function() { im.demultiply(null) });
        im.demultiply();
        assert.equal(im.premultiplied(), false);
        im.premultiply(function(err, im_res) {
            assert.equal(im_res.premultiplied(), true);
            im.demultiply(function (err, im_res2) {
                assert.equal(im_res.premultiplied(), false);
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
            assert.equal(im_blank2.getPixel(0,0,{get_color:true}).g, 128);
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
        
        assert.throws(function() { mapnik.MemoryDatasource({extent: '-180,-90,180,90'}); }); 
        assert.throws(function() { new mapnik.MemoryDatasource(); }); 
        assert.throws(function() { new mapnik.MemoryDatasource(null); });
        var params = {
            extent:'-180,-90,180,90',
            foo: 1,
            waffle: 22.2,
            fee: true,
            boom: {},
            lalala: null,
            heheheh: undefined
        }
        var mem_datasource = new mapnik.MemoryDatasource(params);
        mem_datasource.add({ 'x': 0, 'y': 0 });
        mem_datasource.add({ 'x': 1, 'y': 1 });
        mem_datasource.add({ 'x': 2, 'y': 2 });
        mem_datasource.add({ 'x': 3, 'y': 3 });
        var l = new mapnik.Layer('test');
        l.srs = m3.srs;
        l.styles = ['points'];
        l.datasource = mem_datasource;
        var mem_datasource_same = l.datasource;
        assert.deepEqual(mem_datasource.parameters(), mem_datasource_same.parameters());
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
        assert.throws(function() { gray.setGrayScaleToAlpha(null) });
        assert.throws(function() { gray.setGrayScaleToAlpha({}) });
        gray.setGrayScaleToAlpha();
        var gray_view = gray.view(0, 0, gray.width(), gray.height());
        assert.equal(gray_view.isSolidSync(), true);
        var pixel = gray.getPixel(0, 0, {get_color:true});
        assert.equal(pixel.r, 255);
        assert.equal(pixel.g, 255);
        assert.equal(pixel.b, 255);
        assert.equal(pixel.a, 255);

        gray.fill(new mapnik.Color('black'));
        gray.setGrayScaleToAlpha();
        var pixel2 = gray.getPixel(0, 0, {get_color:true});
        assert.equal(pixel2.r, 255);
        assert.equal(pixel2.g, 255);
        assert.equal(pixel2.b, 255);
        assert.equal(pixel2.a, 0);

        gray.setGrayScaleToAlpha(new mapnik.Color('green'));
        var pixel3 = gray.getPixel(0, 0, {get_color:true});
        assert.equal(pixel3.r, 0);
        assert.equal(pixel3.g, 128);
        assert.equal(pixel3.b, 0);
        assert.equal(pixel3.a, 255);
    });

    it('should fail to compare', function() {
        var im1 = new mapnik.Image(256,256);
        var im2 = new mapnik.Image(256,256);
        im1.fillSync(new mapnik.Color('blue'));
        im2.fillSync(new mapnik.Color('blue'));
        assert.throws(function() { im1.compare(null); });
        assert.throws(function() { im1.compare({}); });
        assert.throws(function() { im1.compare(im2, null); });
        assert.throws(function() { im1.compare(im2, {threshold:null}); });
        assert.throws(function() { im1.compare(im2, {alpha:null}); });
    });

    it('should support setting an individual pixel', function() {
        var gray = new mapnik.Image(256, 256);
        assert.throws(function() { gray.setPixel(); });
        assert.throws(function() { gray.setPixel(1); });
        assert.throws(function() { gray.setPixel(1,2); });
        assert.throws(function() { gray.setPixel(1,2,null); });
        assert.throws(function() { gray.setPixel(1,2, {}); });
        assert.throws(function() { gray.setPixel(-1,-1,3); });
        gray.setPixel(0,0,new mapnik.Color('white'));
        assert.throws(function() { gray.getPixel(); });
        assert.throws(function() { gray.getPixel(0,0, null); });
        assert.throws(function() { gray.getPixel(null,0); });
        assert.throws(function() { gray.getPixel(0,null); });
        assert.throws(function() { gray.getPixel(0,0, {get_color:null}); });
        assert.equal(gray.getPixel(-1,-1), undefined);
        var pixel = gray.getPixel(0,0,{get_color:true});
        assert.equal(pixel.r, 255);
        assert.equal(pixel.g, 255);
        assert.equal(pixel.b, 255);
        assert.equal(pixel.a, 255);
    });
    
    it('should handle setting and getting of a null image ', function() {
        var gray = new mapnik.Image(256, 256, {type: mapnik.imageType.null});
        assert.throws(function() { gray.setPixel(0,0,-1); });
        assert.throws(function() { gray.setPixel(1,0,0); });
        assert.throws(function() { gray.setPixel(2,0,1); });
        assert.equal(gray.getPixel(0,0), undefined);
        assert.equal(gray.getPixel(1,0), undefined);
        assert.equal(gray.getPixel(2,0), undefined);
    });
    
    
    it('should support setting and getting gray8 pixel', function() {
        var gray = new mapnik.Image(256, 256, {type: mapnik.imageType.gray8});
        gray.setPixel(0,0,-1);
        gray.setPixel(1,0,0);
        gray.setPixel(2,0,1);
        assert.equal(gray.getPixel(0,0), 0);
        assert.equal(gray.getPixel(1,0), 0);
        assert.equal(gray.getPixel(2,0), 1);
    });
    
    it('should support setting and getting gray8s pixel', function() {
        var gray = new mapnik.Image(256, 256, {type: mapnik.imageType.gray8s});
        gray.setPixel(0,0,-1);
        gray.setPixel(1,0,0);
        gray.setPixel(2,0,1);
        assert.equal(gray.getPixel(0,0), -1);
        assert.equal(gray.getPixel(1,0), 0);
        assert.equal(gray.getPixel(2,0), 1);
    });
    
    it('should support setting and getting gray16 pixel', function() {
        var gray = new mapnik.Image(256, 256, {type: mapnik.imageType.gray16});
        gray.setPixel(0,0,-1);
        gray.setPixel(1,0,0);
        gray.setPixel(2,0,1);
        assert.equal(gray.getPixel(0,0), 0);
        assert.equal(gray.getPixel(1,0), 0);
        assert.equal(gray.getPixel(2,0), 1);
    });
    
    it('should support setting and getting gray16s pixel', function() {
        var gray = new mapnik.Image(256, 256, {type: mapnik.imageType.gray16s});
        gray.setPixel(0,0,-1);
        gray.setPixel(1,0,0);
        gray.setPixel(2,0,1);
        assert.equal(gray.getPixel(0,0), -1);
        assert.equal(gray.getPixel(1,0), 0);
        assert.equal(gray.getPixel(2,0), 1);
    });
    
    it('should support setting and getting gray32 pixel', function() {
        var gray = new mapnik.Image(256, 256, {type: mapnik.imageType.gray32});
        gray.setPixel(0,0,-1);
        gray.setPixel(1,0,0);
        gray.setPixel(2,0,1);
        assert.equal(gray.getPixel(0,0), 0);
        assert.equal(gray.getPixel(1,0), 0);
        assert.equal(gray.getPixel(2,0), 1);
    });
    
    it('should support setting and getting gray32s pixel', function() {
        var gray = new mapnik.Image(256, 256, {type: mapnik.imageType.gray32s});
        gray.setPixel(0,0,-1);
        gray.setPixel(1,0,0);
        gray.setPixel(2,0,1);
        assert.equal(gray.getPixel(0,0), -1);
        assert.equal(gray.getPixel(1,0), 0);
        assert.equal(gray.getPixel(2,0), 1);
    });
    
    it('should support setting and getting gray32f pixel', function() {
        var gray = new mapnik.Image(256, 256, {type: mapnik.imageType.gray32f});
        gray.setPixel(0,0,-1.9);
        gray.setPixel(1,0,0.8);
        gray.setPixel(2,0,1.2);
        assert(Math.abs(gray.getPixel(0,0) + 1.9) < 1e-7);
        assert(Math.abs(gray.getPixel(1,0) - 0.8) < 1e-7);
        assert(Math.abs(gray.getPixel(2,0) - 1.2) < 1e-7);
    });
    
    it('should support setting and getting gray64 pixel', function() {
        var gray = new mapnik.Image(256, 256, {type: mapnik.imageType.gray64});
        gray.setPixel(0,0,-1);
        gray.setPixel(1,0,0);
        gray.setPixel(2,0,1);
        assert.equal(gray.getPixel(0,0), 0);
        assert.equal(gray.getPixel(1,0), 0);
        assert.equal(gray.getPixel(2,0), 1);
    });
    
    it('should support setting and getting gray64s pixel', function() {
        var gray = new mapnik.Image(256, 256, {type: mapnik.imageType.gray64s});
        gray.setPixel(0,0,-1);
        gray.setPixel(1,0,0);
        gray.setPixel(2,0,1);
        assert.equal(gray.getPixel(0,0), -1);
        assert.equal(gray.getPixel(1,0), 0);
        assert.equal(gray.getPixel(2,0), 1);
    });
    
    it('should support setting and getting gray64f pixel', function() {
        var gray = new mapnik.Image(256, 256, {type: mapnik.imageType.gray64f});
        gray.setPixel(0,0,-1);
        gray.setPixel(1,0,0);
        gray.setPixel(2,0,1);
        assert.equal(gray.getPixel(0,0), -1);
        assert.equal(gray.getPixel(1,0), 0);
        assert.equal(gray.getPixel(2,0), 1);
    });

    it('should support have set_pixel protecting overflow and underflows', function() {
        var imx = new mapnik.Image(4, 4, {type: mapnik.imageType.gray16});
        imx.setPixel(0,0,12);
        imx.setPixel(0,1,-1);
        imx.setPixel(1,0,99999);
        imx.setPixel(1,1,256);
        assert.equal(imx.getPixel(0,0), 12);
        assert.equal(imx.getPixel(0,1), 0);
        assert.equal(imx.getPixel(1,0), 65535);
        assert.equal(imx.getPixel(1,1), 256);
    });

    it('should support scaling and offset', function() {
        var im = new mapnik.Image(4, 4, {type: mapnik.imageType.gray16});
        assert.equal(im.scaling, 1);
        assert.equal(im.offset, 0);
        assert.throws(function() { im.scaling = null });
        assert.throws(function() { im.scaling = 0 });
        assert.throws(function() { im.offset = null });
        im.scaling = 2.2;
        im.offset = 1.1;
        assert.equal(im.scaling, 2.2);
        assert.equal(im.offset, 1.1);

    });

    it('should fail to copy a null image', function(done) {
        var im = new mapnik.Image(4,4,{type: mapnik.imageType.null});
        var im3 = new mapnik.Image(4,4,{type: mapnik.imageType.gray8});
        assert.throws(function() { var im2 = im.copy(); });
        assert.throws(function() { var im2 = im3.copy(mapnik.imageType.null); });
        assert.throws(function() { var im2 = im3.copy(mapnik.imageType.gray8, null); });
        assert.throws(function() { var im2 = im3.copy(99, function(err, result) {}); });
        assert.throws(function() { var im2 = im3.copy(mapnik.imageType.gray8, null, function(err, result) {}); });
        assert.throws(function() { var im2 = im3.copy(null, function(err, result) {}); });
        assert.throws(function() { var im2 = im3.copySync(99); });
        assert.throws(function() { var im2 = im3.copy({}, null); });
        assert.throws(function() { var im2 = im3.copySync({scaling:null}); });
        assert.throws(function() { var im2 = im3.copySync({offset:null}); });
        assert.throws(function() { var im2 = im3.copy({}, null); });
        assert.throws(function() { var im2 = im3.copy({scaling:null}, function(err, result) {}); });
        assert.throws(function() { var im2 = im3.copy({offset:null}, function(err, result) {}); });
        im.copy(function(err, im2) { 
            assert.throws(function() { if (err) throw err; });
            done();
        });
    });

    it('should support copying from gray16 to gray8', function(done) {
        var im = new mapnik.Image(4, 4, {type: mapnik.imageType.gray16});
        im.setPixel(0,0,12);
        im.setPixel(0,1,-1);
        im.setPixel(1,0,99999);
        im.setPixel(1,1,256);
        assert.throws(function() { var foo = im.copySync(null); });
        assert.throws(function() { var foo = im.copySync({scaling:null}); });
        assert.throws(function() { var foo = im.copySync({offset:null}); });
        assert.throws(function() { var foo = im.copySync(mapnik.imageType.gray8, null); });
        assert.throws(function() { var foo = im.copySync(mapnik.imageType.gray8, {offset:null}); });
        var im2 = im.copySync(mapnik.imageType.gray8);
        assert.equal(im2.getPixel(0,0), 12);
        assert.equal(im2.getPixel(0,1), 0);
        assert.equal(im2.getPixel(1,0), 255);
        assert.equal(im2.getPixel(1,1), 255);
        var im3 = im2.copySync(mapnik.imageType.gray16, {offset:1, scaling:2});
        assert.equal(im3.scaling, 2);
        assert.equal(im3.offset, 1);
        assert.equal(im3.getPixel(0,0), 5);
        assert.equal(im3.getPixel(0,1), 0);
        assert.equal(im3.getPixel(1,0), 127);
        assert.equal(im3.getPixel(1,1), 127);
        assert.throws(function() { im.copy(null); });
        assert.throws(function() { im.copy({scaling:null}); });
        assert.throws(function() { im.copy({offset:null}); });
        assert.throws(function() { im.copy(mapnik.imageType.gray8, null); });
        assert.throws(function() { im.copy(mapnik.imageType.gray8, {offset:null}); });
        im.copy(mapnik.imageType.gray8, function(err, im2) {
            if (err) throw err;
            assert.equal(im2.getPixel(0,0), 12);
            assert.equal(im2.getPixel(0,1), 0);
            assert.equal(im2.getPixel(1,0), 255);
            assert.equal(im2.getPixel(1,1), 255);
            im2.copy(mapnik.imageType.gray16, {offset:1, scaling:2}, function (err, im3) {
                if (err) throw err;
                assert.equal(im3.scaling, 2);
                assert.equal(im3.offset, 1);
                assert.equal(im3.getPixel(0,0), 5);
                assert.equal(im3.getPixel(0,1), 0);
                assert.equal(im3.getPixel(1,0), 127);
                assert.equal(im3.getPixel(1,1), 127);
                done();
            });
        });
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
        assert.equal(two.getPixel(0,0,{get_color:true}).r, 255);
        assert.equal(two.getPixel(0,0,{get_color:true}).g, 255);
        assert.equal(two.getPixel(0,0,{get_color:true}).b, 255);
        assert.equal(two.getPixel(0,0,{get_color:true}).a, 0);
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

    it('should fail to open', function(done) {
        assert.throws(function() { var im = new mapnik.Image.openSync(); });
        assert.throws(function() { var im = new mapnik.Image.open(); });
        assert.throws(function() { var im = new mapnik.Image.openSync(null); });
        assert.throws(function() { var im = new mapnik.Image.openSync('./PATH/FILE_DOES_NOT_EXIST.tiff'); });
        assert.throws(function() { var im = new mapnik.Image.openSync('./test/data/markers.xml'); });
        assert.throws(function() { var im = new mapnik.Image.openSync('./test/images/corrupt-10x10.png'); });
        assert.throws(function() { var im = new mapnik.Image.open(null, function(err, result) {}); });
        assert.throws(function() { var im = new mapnik.Image.open('./test/images/10x10.png', null); });
        mapnik.Image.open('./PATH/FILE_DOES_NOT_EXIST.tiff', function(err, result) {
            assert.throws(function() { if (err) throw err; });
            mapnik.Image.open('./test/data/markers.xml', function(err, result) {
                assert.throws(function() { if (err) throw err; });
                mapnik.Image.open('./test/images/corrupt-10x10.png', function(err, result) {
                    assert.throws(function() { if (err) throw err; });
                    done();
                });
            });
        });
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

    it('isSolid async works if true', function(done) {
        var im = new mapnik.Image(256, 256);
        assert.throws(function() { im.isSolid(null); });
        assert.equal(im.isSolid(), true);
        assert.equal(im.isSolidSync(), true);
        im.isSolid(function(err,solid,pixel) {
            assert.equal(solid, true);
            assert.equal(pixel, 0);
            done();
        });
    });

    it('isSolid should fail as not solid', function(done) {
        var im = new mapnik.Image(256, 256);
        im.setPixel(0,0,new mapnik.Color('green'));
        assert.equal(im.isSolid(), false);
        assert.equal(im.isSolidSync(), false);
        im.isSolid(function(err,solid,pixel) {
            assert.equal(solid, false);
            done();
        });
    });

    it('isSolid fails', function(done) {
        var im = new mapnik.Image(0, 0);
        assert.throws(function() { im.isSolidSync(); });
        assert.throws(function() { im.isSolid(); });
        im.isSolid(function(err,solid,pixel) {
            assert.throws(function() { if (err) throw err });
            done();
        });
    });

    it('isSolid async works if true and white', function(done) {
        var im = new mapnik.Image(256, 256);
        var color = new mapnik.Color('white');
        im.fill(color);
        assert.equal(im.isSolidSync(), true);
        im.isSolid(function(err,solid,pixel) {
            assert.equal(solid, true);
            assert.equal(pixel, 0xffffffff);
            done();
        });
    });

    it('isSolid async works if false', function(done) {
        var im = new mapnik.Image.open('./test/support/a.png');
        assert.equal(im.isSolidSync(), false);
        im.isSolid(function(err,solid,pixel) {
            assert.equal(solid, false);
            assert.equal(pixel, undefined);
            done();
        });
    });

    it('fill fails', function(done) {
        var im = new mapnik.Image(5,5, {type: mapnik.imageType.null});
        assert.throws(function() { im.fillSync(new mapnik.Color('blue')); });
        assert.throws(function() { im.fillSync({}); });
        assert.throws(function() { im.fillSync(); });
        assert.throws(function() { im.fillSync(0); });
        assert.throws(function() { im.fillSync(null); });
        assert.throws(function() { im.fill(null, function(err, result) {}); });
        assert.throws(function() { im.fill({}, function(err, result) {}); });
        assert.throws(function() { im.fill(1, null); });
        im.fill(new mapnik.Color('blue'), function(err, result) { 
            assert.throws(function() { if (err) throw err; });
            done(); 
        });
    });

    it('fill sync works', function() {
        var im = new mapnik.Image(5,5);
        im.fillSync(new mapnik.Color('blue'));
        assert.equal(im.getPixel(0,0), 4294901760);
        im.fillSync(-1);
        assert.equal(im.getPixel(0,0), 0);
        im.fillSync(1);
        assert.equal(im.getPixel(0,0), 1);
        im.fillSync(1.99);
        assert.equal(im.getPixel(0,0), 1);
    });

    it('fill async works - color', function(done) {
        var im = new mapnik.Image(5,5);
        im.fill(new mapnik.Color('blue'), function(err, im_res) {
            if (err) throw err;
            assert.equal(im_res.getPixel(0,0), 4294901760);
            done();
        });
    });
    
    it('fill async works - int', function(done) {
        var im = new mapnik.Image(5,5);
        im.fill(-1, function(err, im_res) {
            if (err) throw err;
            assert.equal(im_res.getPixel(0,0), 0);
            done();
        });
    });
    
    it('fill async works - uint', function(done) {
        var im = new mapnik.Image(5,5);
        im.fill(1, function(err, im_res) {
            if (err) throw err;
            assert.equal(im_res.getPixel(0,0), 1);
            done();
        });
    });

    it('fill async works - double', function(done) {
        var im = new mapnik.Image(5,5);
        im.fill(1.99, function(err, im_res) {
            if (err) throw err;
            assert.equal(im_res.getPixel(0,0), 1);
            done();
        });
    });

    it('clear fails', function(done) {
        var im = new mapnik.Image(5,5,{type: mapnik.imageType.null});
        assert.throws(function() { im.clear(); });
        assert.throws(function() { im.clearSync(); });
        assert.throws(function() { im.clear(null); });
        im.clear(function(err,im_res) {
            assert.throws(function() { if (err) throw err; });
            done();
        });
    });
    
    it('clear sync', function() {
        var im = new mapnik.Image(5,5);
        assert.equal(0,im.compare(new mapnik.Image(5,5),{threshold:0}))
        im.fillSync(1);
        assert.equal(im.getPixel(0,0), 1);
        assert.equal(25,im.compare(new mapnik.Image(5,5),{threshold:0}))
        im.clearSync();
        assert.equal(0,im.compare(new mapnik.Image(5,5),{threshold:0}))
        assert.equal(im.getPixel(0,0), 0);
    });

    it('clear async', function(done) {
        var im = new mapnik.Image(5,5);
        im.fillSync(1);
        assert.equal(im.getPixel(0,0), 1);
        im.clear(function(err,im_res) {
            assert.equal(im_res.getPixel(0,0), 0);
            done();
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
    
    it('should fail to resize image with bad input', function(done) {
        var im = new mapnik.Image(4,4,{type: mapnik.imageType.null});
        var im3 = new mapnik.Image(4,4,{type: mapnik.imageType.gray8});
        assert.throws(function() { var im2 = im.resize(4,4); });
        assert.throws(function() { var im2 = im3.resizeSync(); });
        assert.throws(function() { var im2 = im3.resize(3); });
        assert.throws(function() { var im2 = im3.resize(3,null); });
        assert.throws(function() { var im2 = im3.resize(99, function(err, result) {}); });
        assert.throws(function() { var im2 = im3.resize(99, null, function(err, result) {}); });
        assert.throws(function() { var im2 = im3.resize(null,99,function(err, result) {}); });
        assert.throws(function() { var im2 = im3.resize(-1,99,function(err, result) {}); });
        assert.throws(function() { var im2 = im3.resize(99,-1,function(err, result) {}); });
        assert.throws(function() { var im2 = im3.resizeSync(99); });
        assert.throws(function() { var im2 = im3.resizeSync(99,null); });
        assert.throws(function() { var im2 = im3.resizeSync(null,99); });
        assert.throws(function() { var im2 = im3.resizeSync(-1,99); });
        assert.throws(function() { var im2 = im3.resizeSync(99,-1); });
        assert.throws(function() { var im2 = im3.resize(99,99, null); });
        assert.throws(function() { var im2 = im3.resizeSync(99,99,{offset_x:null}); });
        assert.throws(function() { var im2 = im3.resizeSync(99,99,{offset_y:null}); });
        assert.throws(function() { var im2 = im3.resizeSync(99,99,{scaling_method:null}); });
        assert.throws(function() { var im2 = im3.resizeSync(99,99,{filter_factor:null}); });
        assert.throws(function() { var im2 = im3.resizeSync(99,99,{scaling_method:999}); });
        assert.throws(function() { var im2 = im3.resize(99,99, null,function(err, result) {}); });
        assert.throws(function() { var im2 = im3.resize(99,99,{offset_x:null}, function(err, result) {}); });
        assert.throws(function() { var im2 = im3.resize(99,99,{offset_y:null}, function(err, result) {}); });
        assert.throws(function() { var im2 = im3.resize(99,99,{scaling_method:null}, function(err, result) {}); });
        assert.throws(function() { var im2 = im3.resize(99,99,{scaling_method:999}, function(err, result) {}); });
        assert.throws(function() { var im2 = im3.resize(99,99,{filter_factor:null}, function(err, result) {}); });
        im.resize(99,99,function(err, im2) { 
            assert.throws(function() { if (err) throw err; });
            done();
        });
    });

    it('should fail because image is size of zero when trying to resize', function(done) {
        var im = new mapnik.Image(0,0,{type: mapnik.imageType.gray8});
        assert.throws(function() { var im2 = im.resizeSync(4,4); });
        im.resize(4,4,function(err, im2) { 
            assert.throws(function() { if (err) throw err; });
            done();
        });
    });

    it('should fail on types not currently supported by resize', function() {
        var im = new mapnik.Image(4,4,{type: mapnik.imageType.gray8s});
        assert.throws(function() { var im2 = im.resizeSync(4,4); });
        var im = new mapnik.Image(4,4,{type: mapnik.imageType.gray16s});
        assert.throws(function() { var im2 = im.resizeSync(4,4); });
        var im = new mapnik.Image(4,4,{type: mapnik.imageType.gray32});
        assert.throws(function() { var im2 = im.resizeSync(4,4); });
        var im = new mapnik.Image(4,4,{type: mapnik.imageType.gray32s});
        assert.throws(function() { var im2 = im.resizeSync(4,4); });
        var im = new mapnik.Image(4,4,{type: mapnik.imageType.gray64});
        assert.throws(function() { var im2 = im.resizeSync(4,4); });
        var im = new mapnik.Image(4,4,{type: mapnik.imageType.gray64s});
        assert.throws(function() { var im2 = im.resizeSync(4,4); });
        var im = new mapnik.Image(4,4,{type: mapnik.imageType.gray64f});
        assert.throws(function() { var im2 = im.resizeSync(4,4); });
    });
    
    it('should fail to resize resize - not premultiplied rgba8', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.png');
        im.resize(100,100, {scaling_method:mapnik.imageScaling.near, filter_factor:1.0}, function(err, result) {
            assert.throws(function() { if (err) throw err; });
            done();
        });
    });
    
    it('should use resize to offset', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.tif');
        im.premultiply();

        // prepare sync call for testing
        var syncresult = im.resize(50, 50, {
            scaling_method:mapnik.imageScaling.near, 
            filter_factor:1.0, 
            offset_x:-25, 
            offset_y:-25
        });

        im.resize(50, 50, { 
                scaling_method:mapnik.imageScaling.near, 
                filter_factor:1.0, 
                offset_x:-25, 
                offset_y:-25
                }, function(err, result) {
            if (err) throw err;
            var expected = 'test/data/images/sat_image-expected-50x50-offset.tif';
            if (!fs.existsSync(expected) || process.env.UPDATE ) {
                result.save(expected, 'tif');
            }
            var im2 = new mapnik.Image.open(expected);
            assert.equal(0, result.compare(im2));

            // test sync
            assert.equal(0, syncresult.compare(im2));


            done();
        });
    });
    
    it('should resize with offset - 100x100', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.tif');
        im.premultiply();
        im.resize(100, 100, { 
                scaling_method:mapnik.imageScaling.near, 
                filter_factor:1.0, 
                offset_x:10, 
                offset_y:10
                }, function(err, result) {
            if (err) throw err;
            var expected = 'test/data/images/sat_image-expected-100x100-offset.tif';
            if (!fs.existsSync(expected) || process.env.UPDATE ) {
                result.save(expected, 'tif');
            }
            var im2 = new mapnik.Image.open(expected);
            assert.equal(0, result.compare(im2));
            done();
        });
    });

    it('should resize image up grayscale - nearest neighbor', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.tif');
        im.premultiply();
        im.resize(100,100, {scaling_method:mapnik.imageScaling.near, filter_factor:1.0}, function(err, result) {
            if (err) throw err;
            var expected = 'test/data/images/sat_image-expected-100x100-near.tif';
            if (!fs.existsSync(expected) || process.env.UPDATE ) {
                result.save(expected, 'tif');
            }
            var im2 = new mapnik.Image.open(expected);
            assert.equal(0, result.compare(im2, {threshold:8}));
            done();
        });
    });
    
    it('should resize image down grayscale - nearest neighbor', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.tif');
        im.premultiply();
        im.resize(50,50, {scaling_method:mapnik.imageScaling.near}, function(err, result) {
            if (err) throw err;
            var expected = 'test/data/images/sat_image-expected-50x50-near.tif';
            if (!fs.existsSync(expected) || process.env.UPDATE ) {
                result.save(expected, 'tif');
            }
            var im2 = new mapnik.Image.open(expected);
            assert.equal(0, result.compare(im2, {threshold:8}));
            done();
        });
    });

    it('should resize image up - nearest neighbor', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.png');
        im.premultiply();
        im.resize(100,100, {scaling_method:mapnik.imageScaling.near, filter_factor:1.0}, function(err, result) {
            if (err) throw err;
            var expected = 'test/data/images/sat_image-expected-100x100-near.png';
            if (!fs.existsSync(expected) || process.env.UPDATE ) {
                result.save(expected, 'png');
            }
            var im2 = new mapnik.Image.open(expected);
            assert.equal(0, result.compare(im2, {threshold:8}));
            done();
        });
    });
    
    it('should resize image down - nearest neighbor', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.png');
        im.premultiply();
        im.resize(50,50, {scaling_method:mapnik.imageScaling.near}, function(err, result) {
            if (err) throw err;
            var expected = 'test/data/images/sat_image-expected-50x50-near.png';
            if (!fs.existsSync(expected) || process.env.UPDATE ) {
                result.save(expected, 'png');
            }
            var im2 = new mapnik.Image.open(expected);
            assert.equal(0, result.compare(im2, {threshold:8}));
            done();
        });
    });
    
    it('should resize image up - bilinear', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.png');
        im.premultiply();
        im.resize(100,100, {scaling_method:mapnik.imageScaling.bilinear}, function(err, result) {
            if (err) throw err;
            var expected = 'test/data/images/sat_image-expected-100x100-bilinear.png';
            if (!fs.existsSync(expected) || process.env.UPDATE ) {
                result.save(expected, 'png');
            }
            var im2 = new mapnik.Image.open(expected);
            assert.equal(0, result.compare(im2, {threshold:8}));
            done();
        });
    });
    
    it('should resize image down - bilinear', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.png');
        im.premultiply();
        im.resize(50,50, {scaling_method:mapnik.imageScaling.bilinear}, function(err, result) {
            if (err) throw err;
            var expected = 'test/data/images/sat_image-expected-50x50-bilinear.png';
            if (!fs.existsSync(expected) || process.env.UPDATE ) {
                result.save(expected, 'png');
            }
            var im2 = new mapnik.Image.open(expected);
            assert.equal(0, result.compare(im2, {threshold:8}));
            done();
        });
    });
    
    it('should resize image up - bicubic', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.png');
        im.premultiply();
        im.resize(100,100, {scaling_method:mapnik.imageScaling.bicubic}, function(err, result) {
            if (err) throw err;
            var expected = 'test/data/images/sat_image-expected-100x100-bicubic.png';
            if (!fs.existsSync(expected) || process.env.UPDATE ) {
                result.save(expected, 'png');
            }
            var im2 = new mapnik.Image.open(expected);
            assert.equal(0, result.compare(im2, {threshold:8}));
            done();
        });
    });
    
    it('should resize image down - bicubic', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.png');
        im.premultiply();
        im.resize(50,50, {scaling_method:mapnik.imageScaling.bicubic}, function(err, result) {
            if (err) throw err;
            var expected = 'test/data/images/sat_image-expected-50x50-bicubic.png';
            if (!fs.existsSync(expected) || process.env.UPDATE ) {
                result.save(expected, 'png');
            }
            var im2 = new mapnik.Image.open(expected);
            assert.equal(0, result.compare(im2, {threshold:8}));
            done();
        });
    });
    
    it('should resize image up - spline16', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.png');
        im.premultiply();
        im.resize(100,100, {scaling_method:mapnik.imageScaling.spline16}, function(err, result) {
            if (err) throw err;
            var expected = 'test/data/images/sat_image-expected-100x100-spline16.png';
            if (!fs.existsSync(expected) || process.env.UPDATE ) {
                result.save(expected, 'png');
            }
            var im2 = new mapnik.Image.open(expected);
            assert.equal(0, result.compare(im2, {threshold:8}));
            done();
        });
    });
    
    it('should resize image down - spline16', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.png');
        im.premultiply();
        im.resize(50,50, {scaling_method:mapnik.imageScaling.spline16}, function(err, result) {
            if (err) throw err;
            var expected = 'test/data/images/sat_image-expected-50x50-spline16.png';
            if (!fs.existsSync(expected) || process.env.UPDATE ) {
                result.save(expected, 'png');
            }
            var im2 = new mapnik.Image.open(expected);
            assert.equal(0, result.compare(im2, {threshold:8}));
            done();
        });
    });

    it('should resize image up - spline36', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.png');
        im.premultiply();
        im.resize(100,100, {scaling_method:mapnik.imageScaling.spline36}, function(err, result) {
            if (err) throw err;
            var expected = 'test/data/images/sat_image-expected-100x100-spline36.png';
            if (!fs.existsSync(expected) || process.env.UPDATE ) {
                result.save(expected, 'png');
            }
            var im2 = new mapnik.Image.open(expected);
            assert.equal(0, result.compare(im2, {threshold:8}));
            done();
        });
    });
    
    it('should resize image down - spline36', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.png');
        im.premultiply();
        im.resize(50,50, {scaling_method:mapnik.imageScaling.spline36}, function(err, result) {
            if (err) throw err;
            var expected = 'test/data/images/sat_image-expected-50x50-spline36.png';
            if (!fs.existsSync(expected) || process.env.UPDATE ) {
                result.save(expected, 'png');
            }
            var im2 = new mapnik.Image.open(expected);
            assert.equal(0, result.compare(im2, {threshold:8}));
            done();
        });
    });

    it('should resize image up - hanning', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.png');
        im.premultiply();
        im.resize(100,100, {scaling_method:mapnik.imageScaling.hanning}, function(err, result) {
            if (err) throw err;
            var expected = 'test/data/images/sat_image-expected-100x100-hanning.png';
            if (!fs.existsSync(expected) || process.env.UPDATE ) {
                result.save(expected, 'png');
            }
            var im2 = new mapnik.Image.open(expected);
            assert.equal(0, result.compare(im2, {threshold:8}));
            done();
        });
    });
    
    it('should resize image down - hanning', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.png');
        im.premultiply();
        im.resize(50,50, {scaling_method:mapnik.imageScaling.hanning}, function(err, result) {
            if (err) throw err;
            var expected = 'test/data/images/sat_image-expected-50x50-hanning.png';
            if (!fs.existsSync(expected) || process.env.UPDATE ) {
                result.save(expected, 'png');
            }
            var im2 = new mapnik.Image.open(expected);
            assert.equal(0, result.compare(im2, {threshold:8}));
            done();
        });
    });

    it('should resize image up - hamming', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.png');
        im.premultiply();
        im.resize(100,100, {scaling_method:mapnik.imageScaling.hamming}, function(err, result) {
            if (err) throw err;
            var expected = 'test/data/images/sat_image-expected-100x100-hamming.png';
            if (!fs.existsSync(expected) || process.env.UPDATE ) {
                result.save(expected, 'png');
            }
            var im2 = new mapnik.Image.open(expected);
            assert.equal(0, result.compare(im2, {threshold:8}));
            done();
        });
    });
    
    it('should resize image down - hamming', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.png');
        im.premultiply();
        im.resize(50,50, {scaling_method:mapnik.imageScaling.hamming}, function(err, result) {
            if (err) throw err;
            var expected = 'test/data/images/sat_image-expected-50x50-hamming.png';
            if (!fs.existsSync(expected) || process.env.UPDATE ) {
                result.save(expected, 'png');
            }
            var im2 = new mapnik.Image.open(expected);
            assert.equal(0, result.compare(im2, {threshold:8}));
            done();
        });
    });

    it('should resize image up - hermite', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.png');
        im.premultiply();
        im.resize(100,100, {scaling_method:mapnik.imageScaling.hermite}, function(err, result) {
            if (err) throw err;
            var expected = 'test/data/images/sat_image-expected-100x100-hermite.png';
            if (!fs.existsSync(expected) || process.env.UPDATE ) {
                result.save(expected, 'png');
            }
            var im2 = new mapnik.Image.open(expected);
            assert.equal(0, result.compare(im2, {threshold:8}));
            done();
        });
    });
    
    it('should resize image down - hermite', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.png');
        im.premultiply();
        im.resize(50,50, {scaling_method:mapnik.imageScaling.hermite}, function(err, result) {
            if (err) throw err;
            var expected = 'test/data/images/sat_image-expected-50x50-hermite.png';
            if (!fs.existsSync(expected) || process.env.UPDATE ) {
                result.save(expected, 'png');
            }
            var im2 = new mapnik.Image.open(expected);
            assert.equal(0, result.compare(im2, {threshold:8}));
            done();
        });
    });

    it('should resize image up - kaiser', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.png');
        im.premultiply();
        im.resize(100,100, {scaling_method:mapnik.imageScaling.kaiser}, function(err, result) {
            if (err) throw err;
            var expected = 'test/data/images/sat_image-expected-100x100-kaiser.png';
            if (!fs.existsSync(expected) || process.env.UPDATE ) {
                result.save(expected, 'png');
            }
            var im2 = new mapnik.Image.open(expected);
            assert.equal(0, result.compare(im2, {threshold:8}));
            done();
        });
    });
    
    it('should resize image down - kaiser', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.png');
        im.premultiply();
        im.resize(50,50, {scaling_method:mapnik.imageScaling.kaiser}, function(err, result) {
            if (err) throw err;
            var expected = 'test/data/images/sat_image-expected-50x50-kaiser.png';
            if (!fs.existsSync(expected) || process.env.UPDATE ) {
                result.save(expected, 'png');
            }
            var im2 = new mapnik.Image.open(expected);
            assert.equal(0, result.compare(im2, {threshold:8}));
            done();
        });
    });

    it('should resize image up - quadric', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.png');
        im.premultiply();
        im.resize(100,100, {scaling_method:mapnik.imageScaling.quadric}, function(err, result) {
            if (err) throw err;
            var expected = 'test/data/images/sat_image-expected-100x100-quadric.png';
            if (!fs.existsSync(expected) || process.env.UPDATE ) {
                result.save(expected, 'png');
            }
            var im2 = new mapnik.Image.open(expected);
            assert.equal(0, result.compare(im2, {threshold:8}));
            done();
        });
    });
    
    it('should resize image down - quadric', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.png');
        im.premultiply();
        im.resize(50,50, {scaling_method:mapnik.imageScaling.quadric}, function(err, result) {
            if (err) throw err;
            var expected = 'test/data/images/sat_image-expected-50x50-quadric.png';
            if (!fs.existsSync(expected) || process.env.UPDATE ) {
                result.save(expected, 'png');
            }
            var im2 = new mapnik.Image.open(expected);
            assert.equal(0, result.compare(im2, {threshold:8}));
            done();
        });
    });

    it('should resize image up - catrom', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.png');
        im.premultiply();
        im.resize(100,100, {scaling_method:mapnik.imageScaling.catrom}, function(err, result) {
            if (err) throw err;
            var expected = 'test/data/images/sat_image-expected-100x100-catrom.png';
            if (!fs.existsSync(expected) || process.env.UPDATE ) {
                result.save(expected, 'png');
            }
            var im2 = new mapnik.Image.open(expected);
            assert.equal(0, result.compare(im2, {threshold:8}));
            done();
        });
    });
    
    it('should resize image down - catrom', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.png');
        im.premultiply();
        im.resize(50,50, {scaling_method:mapnik.imageScaling.catrom}, function(err, result) {
            if (err) throw err;
            var expected = 'test/data/images/sat_image-expected-50x50-catrom.png';
            if (!fs.existsSync(expected) || process.env.UPDATE ) {
                result.save(expected, 'png');
            }
            var im2 = new mapnik.Image.open(expected);
            assert.equal(0, result.compare(im2, {threshold:8}));
            done();
        });
    });

    it('should resize image up - gaussian', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.png');
        im.premultiply();
        im.resize(100,100, {scaling_method:mapnik.imageScaling.gaussian}, function(err, result) {
            if (err) throw err;
            var expected = 'test/data/images/sat_image-expected-100x100-gaussian.png';
            if (!fs.existsSync(expected) || process.env.UPDATE ) {
                result.save(expected, 'png');
            }
            var im2 = new mapnik.Image.open(expected);
            assert.equal(0, result.compare(im2, {threshold:8}));
            done();
        });
    });
    
    it('should resize image down - gaussian', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.png');
        im.premultiply();
        im.resize(50,50, {scaling_method:mapnik.imageScaling.gaussian}, function(err, result) {
            if (err) throw err;
            var expected = 'test/data/images/sat_image-expected-50x50-gaussian.png';
            if (!fs.existsSync(expected) || process.env.UPDATE ) {
                result.save(expected, 'png');
            }
            var im2 = new mapnik.Image.open(expected);
            assert.equal(0, result.compare(im2, {threshold:8}));
            done();
        });
    });

    it('should resize image up - bessel', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.png');
        im.premultiply();
        im.resize(100,100, {scaling_method:mapnik.imageScaling.bessel}, function(err, result) {
            if (err) throw err;
            var expected = 'test/data/images/sat_image-expected-100x100-bessel.png';
            if (!fs.existsSync(expected) || process.env.UPDATE ) {
                result.save(expected, 'png');
            }
            var im2 = new mapnik.Image.open(expected);
            assert.equal(0, result.compare(im2, {threshold:8}));
            done();
        });
    });
    
    it('should resize image down - bessel', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.png');
        im.premultiply();
        im.resize(50,50, {scaling_method:mapnik.imageScaling.bessel}, function(err, result) {
            if (err) throw err;
            var expected = 'test/data/images/sat_image-expected-50x50-bessel.png';
            if (!fs.existsSync(expected) || process.env.UPDATE ) {
                result.save(expected, 'png');
            }
            var im2 = new mapnik.Image.open(expected);
            assert.equal(0, result.compare(im2, {threshold:8}));
            done();
        });
    });

    it('should resize image up - mitchell', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.png');
        im.premultiply();
        im.resize(100,100, {scaling_method:mapnik.imageScaling.mitchell}, function(err, result) {
            if (err) throw err;
            var expected = 'test/data/images/sat_image-expected-100x100-mitchell.png';
            if (!fs.existsSync(expected) || process.env.UPDATE ) {
                result.save(expected, 'png');
            }
            var im2 = new mapnik.Image.open(expected);
            assert.equal(0, result.compare(im2, {threshold:8}));
            done();
        });
    });
    
    it('should resize image down - mitchell', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.png');
        im.premultiply();
        im.resize(50,50, {scaling_method:mapnik.imageScaling.mitchell}, function(err, result) {
            if (err) throw err;
            var expected = 'test/data/images/sat_image-expected-50x50-mitchell.png';
            if (!fs.existsSync(expected) || process.env.UPDATE ) {
                result.save(expected, 'png');
            }
            var im2 = new mapnik.Image.open(expected);
            assert.equal(0, result.compare(im2, {threshold:8}));
            done();
        });
    });

    it('should resize image up - sinc', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.png');
        im.premultiply();
        im.resize(100,100, {scaling_method:mapnik.imageScaling.sinc}, function(err, result) {
            if (err) throw err;
            var expected = 'test/data/images/sat_image-expected-100x100-sinc.png';
            if (!fs.existsSync(expected) || process.env.UPDATE ) {
                result.save(expected, 'png');
            }
            var im2 = new mapnik.Image.open(expected);
            assert.equal(0, result.compare(im2, {threshold:16}));
            done();
        });
    });
    
    it('should resize image down - sinc', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.png');
        im.premultiply();
        im.resize(50,50, {scaling_method:mapnik.imageScaling.sinc}, function(err, result) {
            if (err) throw err;
            var expected = 'test/data/images/sat_image-expected-50x50-sinc.png';
            if (!fs.existsSync(expected) || process.env.UPDATE ) {
                result.save(expected, 'png');
            }
            var im2 = new mapnik.Image.open(expected);
            assert.equal(0, result.compare(im2, {threshold:16}));
            done();
        });
    });

    it('should resize image up - lanczos', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.png');
        im.premultiply();
        im.resize(100,100, {scaling_method:mapnik.imageScaling.lanczos}, function(err, result) {
            if (err) throw err;
            var expected = 'test/data/images/sat_image-expected-100x100-lanczos.png';
            if (!fs.existsSync(expected) || process.env.UPDATE ) {
                result.save(expected, 'png');
            }
            var im2 = new mapnik.Image.open(expected);
            assert.equal(0, result.compare(im2, {threshold:16}));
            done();
        });
    });
    
    it('should resize image down - lanczos', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.png');
        im.premultiply();
        im.resize(50,50, {scaling_method:mapnik.imageScaling.lanczos}, function(err, result) {
            if (err) throw err;
            var expected = 'test/data/images/sat_image-expected-50x50-lanczos.png';
            if (!fs.existsSync(expected) || process.env.UPDATE ) {
                result.save(expected, 'png');
            }
            var im2 = new mapnik.Image.open(expected);
            assert.equal(0, result.compare(im2, {threshold:16}));
            done();
        });
    });

    it('should resize image up - blackman', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.png');
        im.premultiply();
        im.resize(100,100, {scaling_method:mapnik.imageScaling.blackman}, function(err, result) {
            if (err) throw err;
            var expected = 'test/data/images/sat_image-expected-100x100-blackman.png';
            if (!fs.existsSync(expected) || process.env.UPDATE ) {
                result.save(expected, 'png');
            }
            var im2 = new mapnik.Image.open(expected);
            assert.equal(0, result.compare(im2, {threshold:16}));
            done();
        });
    });
    
    it('should resize image down - blackman', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.png');
        im.premultiply();
        im.resize(50,50, {scaling_method:mapnik.imageScaling.blackman}, function(err, result) {
            if (err) throw err;
            var expected = 'test/data/images/sat_image-expected-50x50-blackman.png';
            if (!fs.existsSync(expected) || process.env.UPDATE ) {
                result.save(expected, 'png');
            }
            var im2 = new mapnik.Image.open(expected);
            assert.equal(0, result.compare(im2, {threshold:16}));
            done();
        });
    });

    it('resize async should yield the same results as rendered image', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.png');
        im.premultiply();
        im.resize(50,50, {scaling_method:mapnik.imageScaling.sinc, filter_factor:2.5}, function(err, result) {
            if (err) throw err;
            var map = new mapnik.Map(50,50);
            map.load('test/data/sat_map.xml', function(err, map) {
                if (err) throw err;
                map.zoomAll();
                var im2 = new mapnik.Image(50,50);
                map.render(im2, function(err, im2) {
                    if (err) throw err;
                    assert.equal(0, result.compare(im2, {threshold:0}));
                    done();
                });
            });
        });

    });
    
    it('resize sync should yield the same results as rendered image', function(done) {
        var im = new mapnik.Image.open('test/data/images/sat_image.png');
        im.premultiply();
        var result = im.resizeSync(50, 50, {scaling_method:mapnik.imageScaling.sinc, filter_factor:2.5});
        var map = new mapnik.Map(50,50);
        map.load('test/data/sat_map.xml', function(err, map) {
            if (err) throw err;
            map.zoomAll();
            var im2 = new mapnik.Image(50,50);
            map.render(im2, function(err, im2) {
                if (err) throw err;
                assert.equal(0, result.compare(im2, {threshold:0}));
                done();
            });
        });
    });

    it('be able to create image with zero allocation / from raw buffer', function() {
        var im = new mapnik.Image.open('test/data/images/sat_image.png');
        assert.equal(im.premultiplied(), false);
        // note: makes copy when data() is called
        var data = im.data();
        var im2 = new mapnik.Image.fromBufferSync(im.width(), im.height(), data);
        // We attach `data` onto the image so that v8 will not
        // clean it up before im2 is destroyed
        assert.equal(im2._buffer,data);
        assert.equal(im2.premultiplied(), false);
        assert.equal(0, im.compare(im2, {threshold:0}));
        im.premultiplySync();
        im2.premultiplySync();
        assert.equal(im.premultiplied(), true);
        assert.equal(im2.premultiplied(), true);
        assert.equal(im.painted(), false);
        assert.equal(im2.painted(), false);
        assert.equal(0, im.compare(im2, {threshold:0}));
        var im3 = new mapnik.Image.fromBufferSync(im.width(), im.height(), im.data(), {
            premultiplied: true,
            painted: true
        });
        assert.equal(im3.premultiplied(), true);
        assert.equal(im3.painted(), true);

        // Just for testing (not good practice) we modify data
        // and ensure that im2._buffer also reflects the modification
        // as another way to prove they are the same instance
        assert.equal(im2._buffer[0],23);
        data[0] = 99;
        assert.equal(im2._buffer[0],99);

    });

    it('should fail to use fromBufferSync due to bad input', function() {
        var b = new Buffer(16);
        assert.throws(function() { var im = new mapnik.Image.fromBufferSync(); });
        assert.throws(function() { var im = new mapnik.Image.fromBufferSync(null, null, null); });
        assert.throws(function() { var im = new mapnik.Image.fromBufferSync(null, 2, b); });
        assert.throws(function() { var im = new mapnik.Image.fromBufferSync(2, null, b); });
        assert.throws(function() { var im = new mapnik.Image.fromBufferSync(2, 2, null); });
        assert.throws(function() { var im = new mapnik.Image.fromBufferSync(1, 2, b); });
        assert.throws(function() { var im = new mapnik.Image.fromBufferSync(0, 2, b); });
        assert.throws(function() { var im = new mapnik.Image.fromBufferSync(2, 2, {}); });
        assert.throws(function() { var im = new mapnik.Image.fromBufferSync(2, 2, b, null); });
        assert.throws(function() { var im = new mapnik.Image.fromBufferSync(2, 2, b, {'type':'stuff'}); });
        assert.throws(function() { var im = new mapnik.Image.fromBufferSync(2, 2, b, {'type':null}); });
        assert.throws(function() { var im = new mapnik.Image.fromBufferSync(2, 2, b, {'premultiplied':null}); });
        assert.throws(function() { var im = new mapnik.Image.fromBufferSync(2, 2, b, {'painted':null}); });
    });

    it('resizes consistently', function(done) {
        var data = require('fs').readFileSync(__dirname + '/support/a.png');
        var image = mapnik.Image.fromBytesSync(data);
        image.premultiplySync();
        image.resize(64, 64, function(err, control) {
            if (err) throw err;
            var remaining = 100;
            for (var i = 0; i < 100; i++) (function() {
                mapnik.Image.fromBytes(data, function(err, im) {
                    if (err) throw err;
                    im.premultiply(function(err, im) {
                        if (err) throw err;
                        im.resize(64,64,{}, function(err,resized) {
                            if (err) throw err;
                            assert.equal(control.compare(resized, {}), 0);
                            if (!--remaining) done();
                        });
                    });
                });
            })();
        });
    });

    it('resizes consistently (sync)', function(done) {
        var data = require('fs').readFileSync(__dirname + '/support/a.png');
        var image = mapnik.Image.fromBytesSync(data);
        image.premultiplySync();
        var control = image.resizeSync(64, 64);
        for (var i = 0; i < 100; i++) {
            assert.equal(control.compare(image.resizeSync(64, 64), {}), 0);
        }
        done();
    });
});
