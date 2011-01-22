#!/usr/bin/env node

var sys = require('fs');
var child_process = require('child_process');

var usage = 'usage: render.js <stylesheet> <image>'

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

var mapnik = require("mapnik");

var map = new mapnik.Map(600,400);
map.load(stylesheet);
map.zoom_all();
map.render_to_file(image);
child_process.exec('open ' + image)