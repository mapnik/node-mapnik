"use strict";

var mapnik = require('../');
var assert = require('assert');

describe('logger', function() {
    
    it('get_severity should return default', function(done) {
        assert.equal(mapnik.Logger.getSeverity(), mapnik.Logger.ERROR);
        done();
    });

    it('test that you cant initialize a logger', function() {
        assert.throws(function() { var l = new mapnik.Logger(); });
    });

    it('set_severity should fail with bad input', function() {
        assert.throws(function() { mapnik.Logger.setSeverity(); });
        assert.throws(function() { mapnik.Logger.setSeverity(null); });
        assert.throws(function() { mapnik.Logger.setSeverity(2,3); });
    });

    it('set_severity should set mapnik.logger', function(done) {
        mapnik.Logger.setSeverity(mapnik.Logger.NONE);
        assert.equal(mapnik.Logger.getSeverity(), mapnik.Logger.NONE);
        done();
    });
});
