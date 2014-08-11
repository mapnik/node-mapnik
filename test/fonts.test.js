var mapnik = require('../');
var assert = require('assert');
var fs = require('fs');

function oc(a) {
    var o = {};
    for (var i = 0; i < a.length; i++) {
        o[a[i]] = '';
    }
    return o;
}

before(function() {
    this.timeout(6000);
    mapnik.register_system_fonts();
});

describe('mapnik fonts ', function() {
    it('should auto-register a system font like Times Regular on OS X', function() {
        if (process.platform == 'darwin') {
            assert.ok('Times Regular' in oc(mapnik.fonts()));
            // it should already be loaded so trying to register more should return false
            assert.ok(!mapnik.register_fonts('/System/Library/Fonts/', { recurse: true }));
        }
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
