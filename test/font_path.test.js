var assert = require('assert');
var path = require('path');

var separator = (process.platform === 'win32') ? ';' : ':';

describe('mapnik fonts ', function() {
    it('should auto-register paths in MAPNIK_FONT_PATH', function() {
        process.env.MAPNIK_FONT_PATH = [path.join(__dirname, 'data', 'dir-区县级行政区划'),
                                        '/System/Library/Fonts/']
                                         .join(separator);

        var mapnik = require('../');

        assert.ok(mapnik.fonts().indexOf('DejaVu Sans Mono Bold Oblique') >= 0);
    });
});
