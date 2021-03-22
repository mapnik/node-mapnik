#!/usr/bin/env node

'use strict';

var binary = require('@mapbox/node-pre-gyp'),
    path = require('path'),
    bindingPath = binary.find(path.resolve(__dirname, '..', 'package.json')),
    program = path.join(path.dirname(bindingPath), 'bin', 'mapnik-index'),
    spawn = require('child_process').spawn,
    fs = require('fs');

if (!fs.existsSync(program)) {
  // assume it is not packaged but still on PATH
  program = 'mapnik-index';
}

var proc = spawn(program, process.argv.slice(2))
      .on('error', function(err) {
        if (err && err.code == 'ENOENT') {
          throw new Error("mapnik-index not found at " + program);
        }
        throw err;
      })
      .on('exit', function(code) {
        process.exit(code);
      });

proc.stdout.pipe(process.stdout);
proc.stderr.pipe(process.stderr);
