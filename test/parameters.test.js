var mapnik = require('../');
var assert = require('assert');
var assert = require('assert');
var fs = require('fs');
var path = require('path');

describe('mapnik.Parameters ', function() {
    it('should be accessible from map', function() {
        var map = new mapnik.Map(1, 1);
        map.loadSync('./test/support/extra_arbitary_map_parameters.xml');
        var params = map.parameters;
        assert.equal(params.decimal, 0.999);
        assert.equal(params.integer, 10);
        assert.equal(params.key, 'value2');
    });

    it('should be settable on map', function() {
        var map = new mapnik.Map(1, 1);
        assert.equal(map.toXML(), '<?xml version="1.0" encoding="utf-8"?>\n<Map srs="+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs"/>\n');
        map.parameters = {'a': 'b'};
        assert.equal(map.toXML(), '<?xml version="1.0" encoding="utf-8"?>\n<Map srs="+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs">\n    <Parameters>\n        <Parameter name="a" type="string">b</Parameter>\n    </Parameters>\n</Map>\n');
    });
});
