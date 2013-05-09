var mapnik = require('mapnik');
var assert = require('assert');
var fs = require('fs');

describe('mapnik.SVG', function() {

    it('should throw with invalid usage', function() {
        // no 'new' keyword
        assert.throws(function() { mapnik.SVG('foo'); });
        assert.throws(function() { new mapnik.SVG('foo'); });
        assert.throws(function() { new mapnik.SVG(); });
    });

    it('should be able to open from file', function() {
        var svg = new mapnik.SVG.open('./test/data/octocat.svg');
        assert.equal(svg.width(),0);
        assert.equal(svg.height(),0);
        var e = svg.extent();
        // TODO
        //assert.almostEqual(e[0],0.00700000000001);
        //assert.almostEqual(e[1],-4.339);
        //assert.almostEqual(e[2],378.46);
        //assert.almostEqual(e[3],332.606);
    });

    it('should be able to open from string', function(done) {
        fs.readFile('./test/data/octocat.svg',function(err,buf) {
            var svg = new mapnik.SVG.fromString(buf.toString('utf8'));
            assert.equal(svg.width(),0);
            assert.equal(svg.height(),0);
            done();
        });
    });


});