var mapnik = require('../');
var assert = require('assert');
var path = require('path');
var fs = require('fs');

function xmlWithFont(font) {
    return '\
    <Map font-directory="./"><Style name="text"><Rule>\
        <TextSymbolizer size="12" face-name="'+font+'"><![CDATA[[name]]]></TextSymbolizer>\
    </Rule></Style></Map>';
}

describe('font scope', function() {
    var a = 'DejaVu Serif Condensed Bold Italic';
    var b = 'DejaVu Serif Condensed Bold';
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
        // global fonts registry should not know about locally registered font
        assert.equal(mapnik.fonts().indexOf(a), -1);
        // map local registry should
        assert.equal(map.fonts().indexOf(a), 0);
        // font-directory should match that passed in via map XML
        assert.equal(map.fontDirectory(),"./");
        // known registered font paths should match local paths
        assert.equal(Object.keys(map.fontFiles())[0],a);
        var font_path = map.fontFiles()[a];
        assert.ok(font_path.indexOf('map-a') > -1);
        // calling loadFonts should cache local font in-memory
        assert.equal(map.loadFonts(),true);
        assert.equal(map.memoryFonts().length,1);
        assert.equal(map.memoryFonts()[0],font_path);
        // loading a second time should not do anything (fonts are already cached)
        assert.deepEqual(map.loadFonts(),false);
        // global cache should be empty
        assert.equal(mapnik.memoryFonts().length,0);
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
        assert.equal(map.fonts().indexOf(b), 0);
        assert.equal(map.fontDirectory(),"./");
        assert.equal(Object.keys(map.fontFiles())[0],b);
        assert.ok(map.fontFiles()[b].indexOf('map-b') > -1);
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
        assert.equal(mapnik.fonts().indexOf(b), -1);
        assert.equal(map.fonts().indexOf(b), -1);
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
        assert.equal(mapnik.fonts().indexOf(a), -1);
        assert.equal(map.fonts().indexOf(a), -1);
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

