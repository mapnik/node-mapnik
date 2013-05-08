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

    it('should be able to put new marker in cache', function() {
        var mc = new mapnik.MarkerCache();
        mc.clear();
        var size = mc.size();
        mc.put('foo',new mapnik.Image(4,4));
        assert.equal(mc.size(),size+1);
    });

});