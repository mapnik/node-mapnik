var mapnik = require('mapnik');
var assert = require('assert');
var fs = require('fs');

function oc(a) {
    var o = {};
    for (var i = 0; i < a.length; i++) {
        o[a[i]] = '';
    }
    return o;
}

exports['test fonts'] = function(beforeExit) {
    // make sure we have default fonts
    assert.ok('DejaVu Sans Bold' in oc(mapnik.fonts()));

    // make sure system font was loaded
    if (process.platform == 'darwin') {
        assert.ok('Times Regular' in oc(mapnik.fonts()));
        // it should already be loaded so trying to register more should return false
        assert.ok(!mapnik.register_fonts('/System/Library/Fonts/', { recurse: true }));
    }

    // will return true if new fonts are found
    // but should return false as we now call at startup
    assert.ok(!mapnik.register_system_fonts());
};
