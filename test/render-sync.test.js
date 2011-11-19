var mapnik = require('mapnik');
var path = require('path');
var helper = require('./support/helper');

exports['sync render to file'] = function(beforeExit, assert) {

    var rendered = false;
    var map = new mapnik.Map(256, 256);
    map.loadSync('./examples/stylesheet.xml');
    map.zoomAll();
    var im = new mapnik.Image(map.width, map.height);
    var filename = helper.filename();
    map.renderFileSync(filename);
    assert.ok(path.existsSync(filename));
    rendered = true;

    beforeExit(function() {
        assert.equal(rendered, true);
    });
};
