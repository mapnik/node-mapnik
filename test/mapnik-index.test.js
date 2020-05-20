"use strict";

var test = require('tape');
var path = require('path');
var fs = require('fs');
var os = require('os');
var crypto = require('crypto');
var mapnikindex = path.resolve(__dirname, '..', 'bin', 'mapnik-index.js');

var file = path.join(__dirname, 'data', 'world_merc.json');

var tmpdir = path.join(os.tmpdir(), crypto.randomBytes(8).toString('hex'));
var tmpfile = path.join(tmpdir, 'world_merc.json');

var spawn = require('child_process').spawn;

test('setup', (assert) => {
  fs.mkdir(tmpdir, function() {
    fs.createReadStream(file).pipe(fs.createWriteStream(tmpfile)).on('close', function() {
      assert.end()});
  });
});

test('should create a spatial index', (assert) => {
  var args = [mapnikindex, '--files', tmpfile];
  spawn(process.execPath, args)
    .on('error', function(err) { assert.ifError(err, 'no error'); })
    .on('close', function(code) {
      assert.equal(code, 0, 'exit 0');

      fs.readdir(tmpdir, function(err, files) {

        files = files.filter(function(filename) {
          return filename === 'world_merc.json.index';
        });

        assert.equal(files.length, 1, 'made spatial index');
        assert.end();
      });
    });
});
