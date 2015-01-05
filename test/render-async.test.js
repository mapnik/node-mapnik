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
