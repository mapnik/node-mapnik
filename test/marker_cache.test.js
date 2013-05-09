var mapnik = require('mapnik');
var assert = require('assert');

describe('mapnik.MarkerCache', function() {

    it('should throw with invalid usage', function() {
        // no 'new' keyword
        assert.throws(function() { mapnik.MarkerCache('foo'); });
        assert.throws(function() { new mapnik.MarkerCache('foo'); });
    });

    it('should be able to clear marker cache', function() {
        var mc = new mapnik.MarkerCache();
        mc.clear()
        assert.equal(mc.size(),3);
        mc.clear()
        assert.equal(mc.size(),3);
    });

    it('should be able to get marker cache keys', function() {
        var mc = new mapnik.MarkerCache();
        assert.deepEqual(mc.keys(),["image://dot","shape://arrow","shape://ellipse"]);
    });

    it('should be able to remove marker by key', function() {
        var mc = new mapnik.MarkerCache();
        mc.put('hello',new mapnik.Image(3,3));
        assert.deepEqual(mc.keys().indexOf('hello'),0);
        mc.remove('hello');
        assert.deepEqual(mc.keys().indexOf('hello'),-1);
    });

    it('should be able to put new image in cache', function() {
        var mc = new mapnik.MarkerCache();
        mc.clear();
        var size = mc.size();
        mc.put('image',new mapnik.Image(4,4));
        assert.equal(mc.keys().indexOf('image'),0)
        assert.equal(mc.size(),size+1);
    });

    it('should be able to get image copy from cache', function() {
        var mc = new mapnik.MarkerCache();
        mc.clear();
        mc.put('image',new mapnik.Image(4,6));
        var copy_from_cache = mc.get('image');
        assert.equal(copy_from_cache.width(),4);
        assert.equal(copy_from_cache.height(),6);
    });

    it('should be able to put new svg in cache', function() {
        var mc = new mapnik.MarkerCache();
        mc.clear();
        var size = mc.size();
        mc.put('svg',new mapnik.SVG.open('./test/data/octocat.svg'));
        assert.equal(mc.keys().indexOf('svg'),0)
        assert.equal(mc.size(),size+1);
    });

});