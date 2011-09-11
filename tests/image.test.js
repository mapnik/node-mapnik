var mapnik = require('mapnik');
var assert = require('assert');
var fs = require('fs');
var path = require('path');

var Image = mapnik.Image;

exports['test images'] = function(beforeExit) {
    // no 'new' keyword
    assert.throws(function() { Image(1, 1); });

    // invalid args
    assert.throws(function() { new Image(); });
    assert.throws(function() { new Image(1); });
    assert.throws(function() { new Image('foo'); });
    assert.throws(function() { new Image('a', 'b', 'c'); });

    var im = new Image(256, 256);
    assert.ok(im instanceof Image);

    assert.equal(im.width(), 256);
    assert.equal(im.height(), 256);
    var v = im.view(0, 0, 256, 256);
    assert.ok(v instanceof mapnik.ImageView);
    assert.equal(v.width(), 256);
    assert.equal(v.height(), 256);
    assert.equal(im.encodeSync().length, v.encodeSync().length);

    im.save('tests/tmp/image.png');

    var im2 = new Image.open('tests/tmp/image.png');
    assert.ok(im2 instanceof Image);

    assert.equal(im2.width(), 256);
    assert.equal(im2.height(), 256);

    assert.equal(im.encodeSync().length, im2.encodeSync().length);
    
    var im_blank = new Image(4,4);
    assert.equal(im_blank.painted(),false);
    assert.equal(im_blank.background,undefined);
    
    var m = new mapnik.Map(4,4);

    m.render(im_blank,{},function(err,im_blank) {
        assert.equal(im_blank.painted(),false);
        assert.equal(im_blank.background,undefined);
    });

    var im_blank2 = new Image(4,4);
    assert.equal(im_blank2.painted(),false);
    assert.equal(im_blank2.background,undefined);

    var m2 = new mapnik.Map(4,4);
    
    m2.background = new mapnik.Color('green');
    m2.render(im_blank2,{},function(err,im_blank2) {
        assert.equal(im_blank2.painted(),false);
        assert.ok(im_blank2.background);
    });


    var im_blank3 = new Image(4,4);
    assert.equal(im_blank3.painted(),false);
    assert.equal(im_blank3.background,undefined);

    var m3 = new mapnik.Map(4,4);
    var s = '<Map>';
    s += '<Style name="points">';
    s += ' <Rule>';
    s += '  <PointSymbolizer />';
    s += ' </Rule>';
    s += '</Style>';
    s += '</Map>';
    m3.fromStringSync(s);

    var mem_datasource = new mapnik.MemoryDatasource({'extent':'-180,-90,180,90'});
    mem_datasource.add({ 'x':0, 'y':0 });
    mem_datasource.add({ 'x':1, 'y':1 });
    mem_datasource.add({ 'x':2, 'y':2 });
    mem_datasource.add({ 'x':3, 'y':3 });
    var l = new mapnik.Layer('test');
    l.srs = m3.srs;
    l.styles = ["points"];
    l.datasource = mem_datasource;
    m3.add_layer(l);
    m3.zoomAll();    
    m3.render(im_blank3,{},function(err,im_blank3) {
        assert.equal(im_blank3.painted(),true);
        assert.equal(im_blank3.background,undefined);
    });

};
