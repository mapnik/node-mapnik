var mapnik = require('mapnik');
var assert = require('assert');
var fs = require('fs');
var path = require('path');


exports['test images view getPixel'] = function(beforeExit) {
    var im = new mapnik.Image(256, 256);
    var view = im.view(0,0,256,256);
    assert.equal(view.isSolid(),true);
    var pixel = view.getPixel(0,0);
    assert.equal(pixel.r,0);
    assert.equal(pixel.g,0);
    assert.equal(pixel.b,0);
    assert.equal(pixel.a,0);

    var im = new mapnik.Image(256, 256);
    im.background = new mapnik.Color(2,2,2,2);
    var view = im.view(0,0,256,256);
    assert.equal(view.isSolid(),true);
    var pixel = view.getPixel(0,0);
    assert.equal(pixel.r,2);
    assert.equal(pixel.g,2);
    assert.equal(pixel.b,2);
    assert.equal(pixel.a,2);
};
