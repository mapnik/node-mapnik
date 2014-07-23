var mapnik = require('../');
var assert = require('assert');
var assert = require('assert');
var fs = require('fs');
var path = require('path');

function cleanXml(xml) {
    return xml.replace('\n','').split(' ').join('');
}

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
        var actual = cleanXml(map.toXML());
        var expected = cleanXml('<?xml version="1.0" encoding="utf-8"?>\n<Map srs="+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs"/>\n');
        assert.equal(actual, expected);
        map.parameters = {'a': 'b'};
        actual = cleanXml(map.toXML())
        expected = cleanXml('<?xml version="1.0" encoding="utf-8"?>\n<Map srs="+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs">\n    <Parameters>\n        <Parameter name="a" type="string">b</Parameter>\n    </Parameters>\n</Map>\n');
        assert.equal(actual, expected);
    });
});
