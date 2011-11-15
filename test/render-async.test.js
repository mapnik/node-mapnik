var mapnik = require('mapnik');

exports['async render'] = function(beforeExit, assert) {

    var rendered = false;
    var map = new mapnik.Map(256, 256);
    map.load('./examples/stylesheet.xml', function(err,map) {
        map.zoomAll();
        var im = new mapnik.Image(map.width, map.height);
        map.render(im, function(err, im) {
            im.encode('png', function(err,buffer) {
                var string = im.toString();
                assert.ok(string);
                rendered = true;
            });
        });
    });

    beforeExit(function() {
        assert.equal(rendered, true);
    });
};


// TODO - pooled and cloned render

exports['async render loop'] = function(beforeExit, assert) {

    var rendered = 0;
    var expected = 10;

    var render = function() {
        var map = new mapnik.Map(256, 256);
        map.load('./examples/stylesheet.xml', function(err,map) {
            map.zoomAll();
            var im = new mapnik.Image(map.width, map.height);
            map.render(im, function(err, im) {
                im.encode('png', function(err,buffer) {
                    var string = im.toString();
                    assert.ok(string);
                    rendered++;
                });
            });
        });
    };

    for (var i = 0; i < expected; ++i) {
        render();
    }

    beforeExit(function() {
        assert.equal(rendered, expected);
    });
};
