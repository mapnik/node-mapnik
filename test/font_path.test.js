"use strict";

var assert = require('assert');
var path = require('path');

var separator = (process.platform === 'win32') ? ';' : ':';

describe('mapnik fonts ', function() {
    it('should auto-register paths in MAPNIK_FONT_PATH', function() {
        process.env.MAPNIK_FONT_PATH = [path.join(__dirname, 'data', 'dir-区县级行政区划'),
                                        '/System/Library/Fonts/']
                                         .join(separator);

        // ensure the module is reloaded so that the newly set
        // process.env.MAPNIK_FONT_PATH takes effect
        delete require.cache[require.resolve('../')];
        var mapnik = require('../');

        assert.ok(mapnik.fonts().indexOf('DejaVu Sans Mono Bold Oblique') >= 0);
    });
});
