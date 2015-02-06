#!/usr/bin/env node

'use strict';

var binary = require('node-pre-gyp'),
    path = require('path'),
    bindingPath = binary.find(path.resolve(__dirname, '..', 'package.json')),
    shapeindex = path.join(path.dirname(bindingPath), 'shapeindex'),
    spawn = require('child_process').spawn,

    proc = spawn(shapeindex, process.argv.slice(2))
      .on('error', function(err) {
        console.error(err);
      })
      .on('exit', function(code) {
        process.exit(code);
      });

proc.stdout.pipe(process.stdout);
proc.stderr.pipe(process.stderr);
