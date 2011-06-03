var mapnik = require('mapnik');
var assert = require('assert');
var fs = require('fs');
var path = require('path');

var Image = mapnik.Image;

exports['test map generation'] = function(beforeExit) {
    // no 'new' keyword
    assert.throws(function() { Image(1,1); });

    // invalid args
    assert.throws(function() { new Image(); });
    assert.throws(function() { new Image(1); });
    assert.throws(function() { new Image('foo'); });
    assert.throws(function() { new Image('a', 'b', 'c'); });

    var im = new Image(256, 256);
    assert.ok(im instanceof Image);

};
