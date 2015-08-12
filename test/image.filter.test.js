"use strict";

var mapnik = require('../');
var assert = require('assert');
var fs = require('fs');

describe('mapnik.Image Filter', function() {

    it('should throw with invalid usage', function(done) {
        var im = new mapnik.Image(3,3);
        assert.throws(function() { im.filter(); });
        assert.throws(function() { im.filterSync(); });
        assert.throws(function() { im.filter(null); });
        assert.throws(function() { im.filterSync(null); });
        assert.throws(function() { im.filter(null, function(err,im) {}); });
        assert.throws(function() { im.filter('blur', null); });
        assert.throws(function() { im.filterSync('notrealfilter'); });
        assert.throws(function() { im.filter('notrealfilter'); });
        im.filter('notrealfilter', function(err,im) { 
            assert.throws(function() { if (err) throw err; });
            done();
        });
    }); 

    it('should blur image - sync', function() {
        var im = new mapnik.Image(3,3);
        im.fill(new mapnik.Color('blue'));
        im.setPixel(1,1,new mapnik.Color('red'));
        im.filterSync('blur');
        assert.equal(im.getPixel(1,1), 4293001244);
    });
    
    it('should blur image - async', function(done) {
        var im = new mapnik.Image(3,3);
        im.fill(new mapnik.Color('blue'));
        im.setPixel(1,1,new mapnik.Color('red'));
        im.filter('blur', function(err,im) {
            assert.equal(im.getPixel(1,1), 4293001244);
            done();
        });
    });
});
