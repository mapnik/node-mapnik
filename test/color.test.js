var mapnik = require('mapnik');

exports['test color creation'] = function(beforeExit, assert) {
    // no 'new' keyword
    assert.throws(function() { mapnik.Color(); });

    // invalid args
    assert.throws(function() { new mapnik.Color(); });
    assert.throws(function() { new mapnik.Color(1); });
    assert.throws(function() { new mapnik.Color('foo'); });

    var c = new mapnik.Color('green');
    assert.equal(c.r, 0);
    assert.equal(c.g, 128);
    assert.equal(c.b, 0);
    assert.equal(c.a, 255);
    assert.equal(c.hex(), '#008000');
    assert.equal(c.toString(), 'rgb(0,128,0)');

    c = new mapnik.Color(0, 128, 0);
    assert.equal(c.r, 0);
    assert.equal(c.g, 128);
    assert.equal(c.b, 0);
    assert.equal(c.a, 255);
    assert.equal(c.hex(), '#008000');
    assert.equal(c.toString(), 'rgb(0,128,0)');

    c = new mapnik.Color(0, 128, 0, 255);
    assert.equal(c.r, 0);
    assert.equal(c.g, 128);
    assert.equal(c.b, 0);
    assert.equal(c.a, 255);
    assert.equal(c.hex(), '#008000');
    assert.equal(c.toString(), 'rgb(0,128,0)');

};
