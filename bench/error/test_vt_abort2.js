var mapnik = require('../../lib');
var path = require('path');
var fs = require('fs');
var assert = require('assert');

var vtile = new mapnik.VectorTile(16,33275,22518);
vtile.addData(fs.readFileSync(path.join(__dirname,'../boundary.pbf')));

var error;
var count = 0;
for (var i=0;i< 20;++i) {
    vtile.composite([vtile]);
    // call parse to trigger a "too large" warning inside `google::protobuf::io::CodedInputStream::PrintTotalBytesLimitError`
    try {
        vtile.parse();
        ++count;
        gc();
    } catch (err) {
        error = err;
        break;
    }
}

/*

Above we should at some point hit:

[libprotobuf ERROR google/protobuf/io/coded_stream.cc:180] A protocol message was rejected because it was too big (more than 67108864 bytes).  To increase the limit (or to disable these warnings), see CodedInputStream::SetTotalBytesLimit() in google/protobuf/io/coded_stream.h.

/Users/dane/projects/node-mapnik/bench/error/test_vt_abort2.js:14
    vtile.parse();
          ^
Error: could not merge buffer as protobuf

*/


if (error) {
    assert.equal(error.message,'could not merge buffer as protobuf');
    console.log('Test success! (ran ' + count + ' iterations)');
    process.exit(0)
} else {
    console.log('Test failed, should have thrown and caught error but did not');
    assert.ok(false) // should not get here
}
