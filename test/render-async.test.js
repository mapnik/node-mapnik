"use strict";

var mapnik = require('../');
var assert = require('assert');
var exists = require('fs').existsSync || require('path').existsSync;
var path = require('path');

mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'shape.input'));

describe('mapnik async rendering', function() {
    it('should render to a file', function(done) {
        var map = new mapnik.Map(600, 400);
        var filename = './test/tmp/renderFile.png';
        map.renderFile(filename, function(error) {
            assert.ok(!error);
            assert.ok(exists(filename));
            done();
        });
    });

    it('should fail to render two things at once', function() {
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/stylesheet.xml');
        map.zoomAll();
        var im = new mapnik.Image(map.width, map.height);
        assert.throws(function() {
            map.render(im, function(err, result) {});
            map.render(im, function(err, result) {});
        });
    });
    
    it('should fail to render two things at once with sync', function() {
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/stylesheet.xml');
        map.zoomAll();
        var im = new mapnik.Image(map.width, map.height);
        assert.throws(function() {
            map.render(im, function(err, result) {});
            map.renderSync(im);
        });
    });
    
    it('should fail to renderFile two things at once', function() {
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/stylesheet.xml');
        map.zoomAll();
        assert.throws(function() {
            map.renderFile('./test/tmp/foo1.png', function(err, result) {});
            map.renderFile('./test/tmp/foo2.png', function(err, result) {});
        });
    });
    
    it('should fail to renderFile two things at once with Sync', function() {
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/stylesheet.xml');
        map.zoomAll();
        assert.throws(function() {
            map.renderFile('./test/tmp/foo1.png', function(err, result) {});
            map.renderFileSync('./test/tmp/foo2.png');
        });
    });

    it('should fail to render to an image gray8', function(done) {
        var map = new mapnik.Map(256, 256);
        map.load('./test/stylesheet.xml', function(err,map) {
            if (err) throw err;
            map.zoomAll();
            var im = new mapnik.Image(map.width, map.height, mapnik.imageType.gray8);
            assert.throws(function() { map.render(); } );
            assert.throws(function() { map.render(im, null); } );
            assert.throws(function() { map.render(null, function(err, im) {}); } );
            map.render(im, function(err, im) {
                assert.throws(function() { if (err) throw err; });
                done();
            });
        });
    });

    it('should render to an image', function(done) {
        var map = new mapnik.Map(256, 256);
        map.load('./test/stylesheet.xml', function(err,map) {
            if (err) throw err;
            map.zoomAll();
            var im = new mapnik.Image(map.width, map.height);
            map.render(im, function(err, im) {
                im.encode('png', function(err,buffer) {
                    assert.ok(buffer);
                    var string = im.toString();
                    assert.ok(string);
                    done();
                });
            });
        });
    });
});
