#!/usr/bin/env node

"use strict";

var exists = require('fs').existsSync || require('path').existsSync;
var fs = require('fs');
var usage = 'usage:';
usage += '\n  mapnik-info.js <single vector tile> (.vector.pbf|.mvt)';

var tile = process.argv[2];

if (!tile) {
   console.log(usage);
   process.exit(1);
}

if (!exists(tile)) {
    console.log(tile + ' does not exist');
    process.exit(1);
}

var mapnik = require('../');
mapnik.register_default_input_plugins();

var buffer = fs.readFileSync(tile);
console.log(mapnik.VectorTile.info(buffer));