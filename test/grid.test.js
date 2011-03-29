var util = require('util');
var assert = require('assert');
var Step = require('step');
var helper = require('./support/helper');
var TileBatch = require('tilelive.js').TileBatch;


exports['test batch rendering'] = function(beforeExit) {
    var finished = false;
    var stylesheet = './examples/stylesheet.xml';
    var layer_idx = 0;
    var join_field = 'FIPS';
    var mbtile_file = helper.filename('mbtiles');

    var batch = new TileBatch({
        filepath: mbtile_file,
        batchsize: 100,
        bbox: [
            -20037500,
            -20037500,
            20037500,
            20037500
        ],
        format: 'png',
        minzoom: 12,
        maxzoom: 14,
        datasource: stylesheet,
        language: 'xml',
        interactivity: {
            key_name: join_field,
            layer: layer_idx
        },
        metadata: {
            name: mbtile_file,
            type: 'baselayer',
            description: mbtile_file,
            version: '1.1',
            formatter: 'function(options, data) { '
                + 'return "<strong>" + data.'+ join_field + ' + "</strong><br/>"'
                + '}'
        }
    });


    Step(
        function() {
            console.log('setup');
            batch.setup(function(err) {
                if (err) throw err;
                this();
            }.bind(this));
        },
        function(err) {
            console.log('fillGridData');
            if (err) throw err;
            if (!batch.options.interactivity) return this();
            batch.fillGridData(function(err, tiles) {
                if (err) throw err;            
                this();
            }.bind(this));
        },
        function(err) {
            console.log('renderChunk');
            if (err) throw err;
            var next = this;
            var end = function(err, tiles) {
                if (err) throw err;
                next(err);
            };
            var render = function() {
                process.nextTick(function() {
                    batch.renderChunk(function(err, tiles) {
                        if (err) {
                            console.log('err: ' + err + ' ' + tiles);
                            throw err;
                        }
                        
                        if (!tiles) {
                            console.log('finished ' + err + ' ' + tiles);
                            return end(err, tiles);
                        }
                        render();
                    });
                });
            };
            render();
        },
        function(err) {
            console.log('finish');
            if (err) throw err;
            batch.finish(this);
        },
        function(err) {
            console.log('done');
            if (err) throw err;
            finished = true;
        }
    );
    
    beforeExit(function() {
        assert.ok(finished);
    })
};

