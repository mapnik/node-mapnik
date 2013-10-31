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

describe('mapnik fonts ', function() {
    it('should auto-register DejaVu fonts', function() {
        // make sure we have default fonts
        assert.ok('DejaVu Sans Bold' in oc(mapnik.fonts()));
    });

    it('should auto-register a system font like Times Regular on OS X', function() {
        if (process.platform == 'darwin') {
            assert.ok('Times Regular' in oc(mapnik.fonts()));
            // it should already be loaded so trying to register more should return false
            assert.ok(!mapnik.register_fonts('/System/Library/Fonts/', { recurse: true }));
        }
    });

    it('should find new fonts when registering all system fonts', function() {
        // will return true if new fonts are found
        // but should return false as we now call at startup
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
