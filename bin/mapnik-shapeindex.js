#!/usr/bin/env node

var mapnik = require('..');

mapnik.shapeindex(process.argv.slice(2), process.stdout, process.stderr, function(err, code) {
    if (err) console.error(err);
    if (code > -1) process.exit(code);
});
