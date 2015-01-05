"use strict";

var mapnik = require('../');
var assert = require('assert');
var exists = require('fs').existsSync || require('path').existsSync;
var helper = require('./support/helper');
var path = require('path');

mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'shape.input'));

describe('mapnik sync rendering ', function() {
    it('should render to a file', function() {
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/stylesheet.xml');
        map.zoomAll();
        var filename = helper.filename();
        map.renderFileSync(filename);
        assert.ok(exists(filename));
    });
});
