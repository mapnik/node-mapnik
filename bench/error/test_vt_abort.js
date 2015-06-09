var mapnik = require('../../lib');
var path = require('path');
var fs = require('fs');
var assert = require('assert');
var bytes = require('bytes');

var stats = {
    count:0,
    max_rss:0,
    max_heap:0
}

var collect_memstats = true;

var vtile = new mapnik.VectorTile(16,33275,22518);
vtile.addData(fs.readFileSync(path.join(__dirname,'../boundary.pbf')));

// This is a cheap way to create an excessively large vtile lazy buffer in memory
// but please do not do this in real code.
// NOTE: we do not call vtile.parse below since that would trigger a different memory
// error (see test_vt_abort2.js)

for (var i=0;i< 25;++i) {
    vtile.composite([vtile]);
    gc();
    if (collect_memstats) {
        var mem = process.memoryUsage()
        if (mem.rss > stats.max_rss) stats.max_rss = mem.rss
        if (mem.heapUsed > stats.max_heap) stats.max_heap = mem.heapUsed
    }
    stats.count++;
}

try {

    /*

    Goal here is to protect against the below abort and instead have node-mapnik throw.

    Note: this abort appeared when https://github.com/mapnik/node-mapnik/issues/371 was fixed.

    FATAL ERROR: v8::Object::SetIndexedPropertiesToExternalArrayData() length exceeds max acceptable value
    Abort trap: 6

    Also on linux the error may be:

    node: ../src/node_buffer.cc:181: v8::Local<v8::Object> node::Buffer::New(node::Environment*, const char*, size_t): Assertion `length <= kMaxLength' failed.

    more details https://github.com/mapnik/node-mapnik/issues/370

    */

    var buf_len = vtile.getData().length;
    console.log("getData did not throw and instead returned buffer length of " + buf_len);
} catch (err) {
    assert.ok(err.message.indexOf('Data is too large') > -1);
    console.log('Test success!');
    if (collect_memstats) {
        console.log("max mem: " + bytes(stats.max_rss) + '/' + stats.max_rss);
        console.log("max heap: " + bytes(stats.max_heap));
    }
    process.exit(0)
}

console.log('Test failed, should have thrown and caught error but did not');
assert.ok(false) // should not get here
