var mapnik = require('../');
var assert = require('assert');
var fs = require('fs');

var images = [
    fs.readFileSync('test/blend-fixtures/1.png'),
    fs.readFileSync('test/blend-fixtures/2.png')
];

describe('mapnik.blend', function() {
    it('should blend images', function() {
        var expected = new mapnik.Image.open('test/blend-fixtures/expected.png');
        mapnik.blend(images, {}, function(err, result, warnings) {
            if (err) throw err;
            assert.equal(0,expected.compare(new mapnik.Image.fromBytesSync(result)));
        });
    });

});
