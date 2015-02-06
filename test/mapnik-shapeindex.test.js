"use strict";

var assert = require('assert');
var path = require('path');
var fs = require('fs');
var os = require('os');
var crypto = require('crypto');

var shp = path.join(__dirname, 'data', 'world_merc.shp');
var shx = path.join(__dirname, 'data', 'world_merc.shx');
var prj = path.join(__dirname, 'data', 'world_merc.prj');
var dbf = path.join(__dirname, 'data', 'world_merc.dbf');

var tmpdir = path.join(os.tmpdir(), crypto.randomBytes(8).toString('hex'));
var tmpshp = path.join(tmpdir, 'world_merc.shp');
var tmpshx = path.join(tmpdir, 'world_merc.shx');
var tmpprj = path.join(tmpdir, 'world_merc.prj');
var tmpdbf = path.join(tmpdir, 'world_merc.dbf');

var mapnik = require('..');

function copy(done) {
    fs.mkdir(tmpdir, function() {
        fs.createReadStream(shp).pipe(fs.createWriteStream(tmpshp)).on('close', function() {
            fs.createReadStream(shx).pipe(fs.createWriteStream(tmpshx)).on('close', function() {
                fs.createReadStream(prj).pipe(fs.createWriteStream(tmpprj)).on('close', function() {
                    fs.createReadStream(dbf).pipe(fs.createWriteStream(tmpdbf)).on('close', done);
                });
            });
        });
    });
}

describe('bin/mapnik-shapefile.js', function() {
    before(copy);

    it('should create a spatial index', function(done) {
        mapnik.shapeindex(['--shape_files', tmpshp], process.stdout, process.stderr, function(err, code) {
            assert.ifError(err, 'no error');
            assert.equal(code, 0, 'exit 0');

            fs.readdir(tmpdir, function(err, files) {

                files = files.filter(function(filename) {
                    console.log(filename);
                    return filename === 'world_merc.index';
                });

                assert.equal(files.length, 1, 'made spatial index');
                done();
            });
        });
    });
});
