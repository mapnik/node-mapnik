"use strict";

var mapnik = require('../');
var assert = require('assert');
var fs = require('fs');

var images = [
    fs.readFileSync('test/blend-fixtures/1.png'),
    fs.readFileSync('test/blend-fixtures/2.png')
];

describe('mapnik.blend', function() {
    it('blended', function(done) {
        var expected = new mapnik.Image.open('test/blend-fixtures/expected.png');
        mapnik.blend(images, {format:"png"}, function(err, result) {
            if (err) throw err;
            var actual = new mapnik.Image.fromBytesSync(result);
            //actual.save('test/blend-fixtures/actual.png')
            assert.equal(0,expected.compare(actual));
            done();
        });
    });

});
