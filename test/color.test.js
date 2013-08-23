var mapnik = require('../');
var assert = require('assert');

describe('mapnik.Color', function() {
    it('should throw with invalid usage', function() {
        // no 'new' keyword
        assert.throws(function() { mapnik.Color(); });
        // invalid args
        assert.throws(function() { new mapnik.Color(); });
        assert.throws(function() { new mapnik.Color(1); });
        assert.throws(function() { new mapnik.Color('foo'); });
    });

    it('should be green via keyword', function() {
        var c = new mapnik.Color('green');
        assert.equal(c.r, 0);
        assert.equal(c.g, 128);
        assert.equal(c.b, 0);
        assert.equal(c.a, 255);
        assert.equal(c.hex(), '#008000');
        assert.equal(c.toString(), 'rgb(0,128,0)');
    });


    it('should be gray via rgb', function() {
        var c = new mapnik.Color(0, 128, 0);
        assert.equal(c.r, 0);
        assert.equal(c.g, 128);
        assert.equal(c.b, 0);
        assert.equal(c.a, 255);
        assert.equal(c.hex(), '#008000');
        assert.equal(c.toString(), 'rgb(0,128,0)');
    });

    it('should be gray via rgba', function() {
        var c = new mapnik.Color(0, 128, 0, 255);
        assert.equal(c.r, 0);
        assert.equal(c.g, 128);
        assert.equal(c.b, 0);
        assert.equal(c.a, 255);
        assert.equal(c.hex(), '#008000');
        assert.equal(c.toString(), 'rgb(0,128,0)');
    });

    it('should be gray via rgba %', function() {
        var c = new mapnik.Color('rgba(0%,50%,0%,1)');
        assert.equal(c.r, 0);
        assert.equal(c.g, 128);
        assert.equal(c.b, 0);
        assert.equal(c.a, 255);
        assert.equal(c.hex(), '#008000');
        assert.equal(c.toString(), 'rgb(0,128,0)');
    });
});
