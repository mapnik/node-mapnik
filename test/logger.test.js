var mapnik = require('../');
var assert = require('assert');
var path = require('path');
var fs = require('fs');

describe('logger', function() {
	it('get_severity should return default', function(done) {
        assert.equal(mapnik.Logger.getSeverity(), mapnik.Logger.ERROR);
        done();
    });
	it('set_severity should set mapnik.logger', function(done) {
		mapnik.Logger.setSeverity(mapnik.Logger.NONE);
        assert.equal(mapnik.Logger.getSeverity(), mapnik.Logger.NONE);
        done();
    });
});