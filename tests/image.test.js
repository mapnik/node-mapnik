var mapnik = require('mapnik');
var assert = require('assert');
var fs = require('fs');
var path = require('path');

var Image = mapnik.Image;

exports['test images'] = function(beforeExit) {
    // no 'new' keyword
    assert.throws(function() { Image(1, 1); });

    // invalid args
    assert.throws(function() { new Image(); });
    assert.throws(function() { new Image(1); });
    assert.throws(function() { new Image('foo'); });
    assert.throws(function() { new Image('a', 'b', 'c'); });

    var im = new Image(256, 256);
    assert.ok(im instanceof Image);

    assert.equal(im.width(), 256);
    assert.equal(im.height(), 256);
    var v = im.view(0, 0, 256, 256);
    assert.ok(v instanceof mapnik.ImageView);
    assert.equal(v.width(), 256);
    assert.equal(v.height(), 256);
    assert.equal(im.encode().length, v.encode().length);

    im.save('tests/tmp/image.png');

    var im2 = new Image.open('tests/tmp/image.png');
    assert.ok(im2 instanceof Image);

    assert.equal(im2.width(), 256);
    assert.equal(im2.height(), 256);

    assert.equal(im.encode().length, im2.encode().length);

};
