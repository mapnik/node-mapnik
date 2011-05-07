#!/usr/bin/env node

// This example shows how to use node-mapnik to render 
// a map to a file
//
// expected output: http://goo.gl/cyGwo

var mapnik = require('mapnik');
var sys = require('fs');
var child_process = require('child_process');

var usage = 'usage: render.js <stylesheet> <image>';

var stylesheet = process.ARGV[2];
if (!stylesheet) {
   console.log(usage);
   process.exit(1);
}

var image = process.ARGV[3];
if (!image) {
   console.log(usage);
   process.exit(1);
}

var map = new mapnik.Map(600, 400);

map.loadSync(stylesheet);
map.zoom_all();
map.renderFileSync(image);

console.log('rendered map to ' + image);
child_process.exec('open ' + image);
