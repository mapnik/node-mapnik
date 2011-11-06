var mapnik = require('mapnik');
var assert = require('assert');
var fs = require('fs');

exports['test constants'] = function(beforeExit) {
    assert.ok(mapnik.settings);
    assert.ok(mapnik.settings.paths);
    assert.ok(mapnik.settings.paths.fonts.length);
    assert.ok(fs.statSync(mapnik.settings.paths.fonts));
    assert.ok(mapnik.settings.paths.input_plugins.length);
    assert.ok(fs.statSync(mapnik.settings.paths.input_plugins));

    // reloading the default plugins path should return false as no more plugins are registered
    assert.ok(!mapnik.register_datasources(mapnik.settings.paths.input_plugins));

    /* has version info */
    assert.ok(mapnik.versions);
    assert.ok(mapnik.versions.node);
    assert.ok(mapnik.versions.v8);
    assert.ok(mapnik.versions.mapnik);
    assert.ok(mapnik.versions.mapnik_number);
    assert.ok(mapnik.versions.boost);
    assert.ok(mapnik.versions.boost_number);

    // make sure we have some
    assert.ok(mapnik.datasources().length > 0);
};
