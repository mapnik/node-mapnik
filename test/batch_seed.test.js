var util = require('util');
var assert = require('assert');
var Step = require('step');
var helper = require('./support/helper');
var mapnik = require('mapnik');


exports['test batch rendering'] = function(beforeExit) {
    var finished = false;
    try {
        var TileBatch = require('tilelive').TileBatch;
    } catch (err) {
        console.log('tilelive.js not installed, skipping batch rendering test');
        finished=true;
        return;
    }
    var stylesheet = './examples/stylesheet.xml';
    if (mapnik.supports.grid) {
        var layer_name = "world";
        var key_name = 'FIPS';
    } else {
        var layer_name = 0;
        var key_name = 'FIPS';    
    }
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
        minzoom: 0,
        maxzoom: 5,
        datasource: stylesheet,
        language: 'xml',
        interactivity: {
            layer: layer_name,
            key: key_name,
            resolution: 4,
            fields: [key_name,"NAME"]
        },
        metadata: {
            name: mbtile_file,
            type: 'baselayer',
            description: mbtile_file,
            version: '1.1',
            formatter: 'function(options, data) { '
                + 'return "<strong>" + data.NAME + "</strong><br/>"'
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
                            console.log('finished rendering tiles');
                            return end(err, tiles);
                        }
                        render();
                    });
                });
            };
            render();
        },
        function(err) {
            if (err) throw err;
            batch.finish(this);
        },
        function(err) {
            if (err) throw err;
            finished = true;
        }
    );
    
    beforeExit(function() {
        console.log('done rendering to: ' + mbtile_file);
        assert.ok(finished);
    })
};

