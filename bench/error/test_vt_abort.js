var mapnik = require('../../lib');
var path = require('path');
var fs = require('fs');
var assert = require('assert');

var vtile = new mapnik.VectorTile(16,33275,22518);
vtile.addData(fs.readFileSync(path.join(__dirname,'../boundary.pbf')));

// This is a cheap way to create an excessively large vtile lazy buffer in memory
// but please do not do this in real code.
// NOTE: we do not call vtile.parse below since that would trigger a different memory
// error (see test_vt_abort2.js)
for (var i=0;i< 100;++i) {
    vtile.composite([vtile])
}

try {

    /*

    Goal here is to protect against the below abort and instead have node-mapnik throw

    https://github.com/mapnik/node-mapnik/issues/370

    FATAL ERROR: v8::Object::SetIndexedPropertiesToExternalArrayData() length exceeds max acceptable value
    Abort trap: 6

    */

    var buf_len = vtile.getData().length;
} catch (err) {
    assert.ok(err.message.indexOf('Data is too large') > -1);
    console.log('Test success!');
    process.exit(0)
}

console.log('Test failed, should have thrown and caught error but did not');
assert.ok(false) // should not get here
