#!/usr/bin/env node

'use strict';

var binary = require('node-gyp-build'),
    path = require('path'),
    bindingPath = binary.path(__dirname + "/.."),
    shapeindex = path.join(path.dirname(bindingPath), 'bin', 'shapeindex'),
    spawn = require('child_process').spawn,
    fs = require('fs');

if (!fs.existsSync(shapeindex)) {
  // assume it is not packaged but still on PATH
  shapeindex = 'shapeindex';
}

var proc = spawn(shapeindex, process.argv.slice(2))
      .on('error', function(err) {
        if (err && err.code == 'ENOENT') {
          throw new Error("shapeindex not found at " + shapeindex);
        }
        throw err;
      })
      .on('exit', function(code) {
        process.exit(code);
      });

proc.stdout.pipe(process.stdout);
proc.stderr.pipe(process.stderr);
