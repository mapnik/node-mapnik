var mapnik = require('../');
var assert = require('assert');
var path = require('path');
var fs = require('fs');

function oc(a) {
    var o = {};
    for (var i = 0; i < a.length; i++) {
        o[a[i]] = '';
    }
    return o;
}

function xmlWithFont(font) {
    return '\
    <Map font-directory="./"><Style name="text"><Rule>\
        <TextSymbolizer size="12" face-name="'+font+'"><![CDATA[[name]]]></TextSymbolizer>\
    </Rule></Style></Map>';
}

describe('font scope', function() {
    var a = 'DejaVu Serif Condensed Bold Italic';
    var b = 'DejaVu Sans Mono Bold Oblique';
    it('fonts are not globally registered', function(done) {
        assert.equal(mapnik.fonts().indexOf(a), -1);
        assert.equal(mapnik.fonts().indexOf(b), -1);
        done();
    });
    it('map a has ' + a, function(done) {
        var map = new mapnik.Map(4, 4);
        assert.doesNotThrow(function() {
            map.fromStringSync(xmlWithFont(a), {
                strict:true,
                base:path.resolve(path.join(__dirname,'data','map-a'))
            });
        });
        assert.equal(mapnik.fonts().indexOf(a), -1);
        done();
    });
    it('map b has ' + b, function(done) {
        var map = new mapnik.Map(4, 4);
        assert.doesNotThrow(function() {
            map.fromStringSync(xmlWithFont(b), {
                strict:true,
                base:path.resolve(path.join(__dirname,'data','map-b'))
            });
        });
        assert.equal(mapnik.fonts().indexOf(b), -1);
        done();
    });
    it('map a should not have ' + b, function(done) {
        var map = new mapnik.Map(4, 4);
        assert.throws(function() {
            map.fromStringSync(xmlWithFont(b), {
                strict:true,
                base:path.resolve(path.join(__dirname,'data','map-a'))
            });
        });
        done();
    });
    it('map b should not have ' + a, function(done) {
        var map = new mapnik.Map(4, 4);
        assert.throws(function() {
            map.fromStringSync(xmlWithFont(a), {
                strict:true,
                base:path.resolve(path.join(__dirname,'data','map-b'))
            });
        });
        done();
    });
});

describe('mapnik fonts ', function() {

    before(function() {
        mapnik.register_system_fonts();
    });

    it('should find new fonts when registering all system fonts', function() {
        // will return true if new fonts are found
        // but should return false now we called in `before`
        assert.ok(!mapnik.register_system_fonts());
    });

    it('should not register hidden fonts file names', function() {
        var fonts = mapnik.fontFiles();
        for (var i = 0; i < fonts.length; i++) {
            assert(fonts[i][1][0] != '.', fonts[i]);
        }
    });

    it('should not register hidden fonts face-names', function() {
        var fonts = mapnik.fonts();
        for (var i = 0; i < fonts.length; i++) {
            assert(fonts[i][0] != '.', fonts[i]);
        }
    });
});

