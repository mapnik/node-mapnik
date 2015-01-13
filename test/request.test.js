"use strict";

var mapnik = require('../');
var assert = require('assert');

describe('mapnik.Request', function() {
    it('should throw with invalid usage', function() {
        // no 'new' keyword
        assert.throws(function() { mapnik.Request(); });
        // invalid args
        assert.throws(function() { new mapnik.Request(); });
        assert.throws(function() { new mapnik.Request(1); });
        assert.throws(function() { new mapnik.Request('foo'); });
    });

    it('should be able to create', function() {
        var req = new mapnik.Request(256,256,[-180,-90,180,90]);
    });

});
