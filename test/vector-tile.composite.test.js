"use strict";

var mapnik = require('../');
var assert = require('assert');
var util = require('util');
var fs = require('fs');
var path = require('path');
var mercator = new(require('sphericalmercator'))();
var existsSync = require('fs').existsSync || require('path').existsSync;
var overwrite_expected_data = false;

var data_base = './test/data/vector_tile/compositing';

mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'csv.input'));
mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'geojson.input'));

var rendering_defaults = {
    area_threshold: 0.1,
    strictly_simple: false,
    multi_polygon_union: true,
    fill_type: mapnik.polygonFillType.nonZero,
    process_all_rings:false,
    reencode:false,
    simplify_distance: 0.0,
    scale: 1,
    scale_denominator: 0.0,
    offset_x: 0,
    offset_y: 0,
    image_format: "jpeg",
    image_scaling: "near",
    threading_mode: mapnik.threadingMode.deferred
};

function render_data(name,coords,callback) {
    // buffer of >=5 is needed to ensure point ends up in tiles touching null island
    var vtile = new mapnik.VectorTile(coords[0],coords[1],coords[2], {buffer_size: 80});
    var map_i = new mapnik.Map(256, 256);
    map_i.loadSync(data_base +'/layers/'+name+'.xml');
    var layer = map_i.get_layer(0);
    name = name + '-' + coords.join('-');
    layer.name = name;
    var map = new mapnik.Map(256, 256);
    map.extent = vtile.extent();
    map.srs = map_i.srs;
    map.add_layer(layer);
    var opts = JSON.parse(JSON.stringify(rendering_defaults));
    //map.renderFileSync('./test/data/vector_tile/compositing/'+name+'.png')
    map.render(vtile,opts,function(err,vtile) {
        if (err) return callback(err);
        var tiledata = vtile.getData();
        var tilename = data_base +'/tiles/'+name+'.mvt';
        fs.writeFileSync(tilename,tiledata);
        return callback();
    });
}

function render_fresh_tile(name,coords,callback) {
    // buffer of >=5 is needed to ensure point ends up in tiles touching null island
    var vtile = new mapnik.VectorTile(coords[0],coords[1],coords[2], {buffer_size: 80});
    var map_i = new mapnik.Map(256, 256);
    map_i.loadSync(data_base +'/layers/'+name+'.xml');
    var layer = map_i.get_layer(0);
    name = name + '-' + coords.join('-');
    layer.name = name;
    var map = new mapnik.Map(256, 256);
    map.extent = vtile.extent();
    map.srs = map_i.srs;
    map.add_layer(layer);
    var opts = JSON.parse(JSON.stringify(rendering_defaults));
    map.render(vtile,opts,function(err,vtile) {
        if (err) return callback(err);
        return callback(null,vtile);
    });
}

var tiles = [[0,0,0],
             [1,0,0],
             [1,0,1],
             [1,1,0],
             [1,1,1],
             [2,0,0],
             [2,0,1],
             [2,1,1]];

function get_data_at(name,coords) {
    return fs.readFileSync(data_base +'/tiles/'+name+'-'+coords.join('-')+'.mvt');
}

function get_tile_at(name,coords) {
    var vt = new mapnik.VectorTile(coords[0],coords[1],coords[2], {buffer_size: 5});
    try {
        vt.setData(get_data_at(name,coords));
    } finally {
        return vt;
    }
}

function get_image_vtile(coords,file,name,format) {
    var vt = new mapnik.VectorTile(coords[0],coords[1],coords[2], {tile_size:256});
    var image_buffer = fs.readFileSync(__dirname + '/data/vector_tile/'+file);
    var im = new mapnik.Image.fromBytesSync(image_buffer);
    vt.addImage(im, name, {image_format:format});
    return vt;
}

function compare_to_image(actual,expected_file) {
    if (!existsSync(expected_file) || process.env.UPDATE) {
        console.log('generating expected image',expected_file);
        actual.save(expected_file,"png32");
    }
    return actual.compare(new mapnik.Image.open(expected_file));
}

describe('mapnik.VectorTile.composite', function() {
    // generate test data
    before(function(done) {
        if (overwrite_expected_data) {
            var remaining = tiles.length;
            tiles.forEach(function(e) {
                render_data('lines',e,function(err) {
                    if (err) throw err;
                    render_data('points',e,function(err) {
                        if (err) throw err;
                        if (--remaining < 1) {
                            done();
                        }
                    });
                });
            });
        } else {
            done();
        }
    });

    it('should fail to composite due to bad parameters', function(done) {
        var vtile1 = new mapnik.VectorTile(1,0,0);
        var vtile2 = new mapnik.VectorTile(1,0,0);
        var vtile3 = get_image_vtile([0,0,0],'cloudless_1_0_0.jpg','raster','jpeg');
        assert.throws(function() { vtile1.composite(); });
        assert.throws(function() { vtile1.compositeSync(); });
        assert.throws(function() { vtile1.composite(function(err, result) {}); });
        assert.throws(function() { vtile1.compositeSync(null); });
        assert.throws(function() { vtile1.composite(null, function(err, result) {}); });
        assert.throws(function() { vtile1.compositeSync([]); });
        assert.throws(function() { vtile1.composite([], function(err, result) {}); });
        assert.throws(function() { vtile1.compositeSync([null]); });
        assert.throws(function() { vtile1.composite([null], function(err, result) {}); });
        assert.throws(function() { vtile1.compositeSync([{}]); });
        assert.throws(function() { vtile1.composite([{}], function(err, result) {}); });
        assert.throws(function() { vtile1.compositeSync([vtile2], null); });
        assert.throws(function() { vtile1.composite([vtile2], null, function(err, result) {}); });
        assert.throws(function() { vtile1.compositeSync([vtile2], {scale:null}); });
        assert.throws(function() { vtile1.composite([vtile2], {scale:null}, function(err, result) {}); });
        assert.throws(function() { vtile1.compositeSync([vtile2], {scale:-0.1}); });
        assert.throws(function() { vtile1.composite([vtile2], {scale:-0.1}, function(err, result) {}); });
        assert.throws(function() { vtile1.compositeSync([vtile2], {offset_x:null}); });
        assert.throws(function() { vtile1.composite([vtile2], {offset_x:null}, function(err, result) {}); });
        assert.throws(function() { vtile1.compositeSync([vtile2], {offset_y:null}); });
        assert.throws(function() { vtile1.composite([vtile2], {offset_y:null}, function(err, result) {}); });
        assert.throws(function() { vtile1.compositeSync([vtile2], {scale_denominator:null}); });
        assert.throws(function() { vtile1.composite([vtile2], {scale_denominator:-0.1}, function(err, result) {}); });
        assert.throws(function() { vtile1.compositeSync([vtile2], {scale_denominator:-0.1}); });
        assert.throws(function() { vtile1.composite([vtile2], {scale_denominator:null}, function(err, result) {}); });
        assert.throws(function() { vtile1.compositeSync([vtile2], {area_threshold:null}); });
        assert.throws(function() { vtile1.composite([vtile2], {area_threshold:null}, function(err, result) {}); });
        assert.throws(function() { vtile1.compositeSync([vtile2], {area_threshold:-0.1}); });
        assert.throws(function() { vtile1.composite([vtile2], {area_threshold:-0.1}, function(err, result) {}); });
        assert.throws(function() { vtile1.compositeSync([vtile2], {strictly_simple:null}); });
        assert.throws(function() { vtile1.composite([vtile2], {strictly_simple:null}, function(err, result) {}); });
        assert.throws(function() { vtile1.compositeSync([vtile2], {multi_polygon_union:null}); });
        assert.throws(function() { vtile1.composite([vtile2], {multi_polygon_union:null}, function(err, result) {}); });
        assert.throws(function() { vtile1.compositeSync([vtile2], {fill_type:null}); });
        assert.throws(function() { vtile1.composite([vtile2], {fill_type:null}, function(err, result) {}); });
        assert.throws(function() { vtile1.compositeSync([vtile2], {fill_type:99}); });
        assert.throws(function() { vtile1.composite([vtile2], {fill_type:99}, function(err, result) {}); });
        assert.throws(function() { vtile1.compositeSync([vtile2], {process_all_rings:null}); });
        assert.throws(function() { vtile1.composite([vtile2], {process_all_rings:null}, function(err, result) {}); });
        assert.throws(function() { vtile1.compositeSync([vtile2], {threading_mode:null}); });
        assert.throws(function() { vtile1.composite([vtile2], {threading_mode:null}, function(err, result) {}); });
        assert.throws(function() { vtile1.compositeSync([vtile2], {threading_mode:999}); });
        assert.throws(function() { vtile1.composite([vtile2], {threading_mode:999}, function(err, result) {}); });
        assert.throws(function() { vtile1.compositeSync([vtile2], {reencode:null}); });
        assert.throws(function() { vtile1.composite([vtile2], {reencode:null}, function(err, result) {}); });
        assert.throws(function() { vtile1.compositeSync([vtile2], {simplify_distance:null}); });
        assert.throws(function() { vtile1.composite([vtile2], {simplify_distance:null}, function(err, result) {}); });
        assert.throws(function() { vtile1.compositeSync([vtile2], {simplify_distance:-0.5}); });
        assert.throws(function() { vtile1.composite([vtile2], {simplify_distance:-0.5}, function(err, result) {}); });
        assert.throws(function() { vtile1.compositeSync([vtile2], {max_extent:null}); });
        assert.throws(function() { vtile1.composite([vtile2], {max_extent:null}, function(err, result) {}); });
        assert.throws(function() { vtile1.compositeSync([vtile2], {max_extent:[]}); });
        assert.throws(function() { vtile1.composite([vtile2], {max_extent:[]}, function(err, result) {}); });
        assert.throws(function() { vtile1.compositeSync([vtile2], {max_extent:[1,2,3,4,5]}); });
        assert.throws(function() { vtile1.composite([vtile2], {max_extent:[1,2,3,4,5]}, function(err, result) {}); });
        assert.throws(function() { vtile1.compositeSync([vtile2], {max_extent:['1',2,3,4]}); });
        assert.throws(function() { vtile1.composite([vtile2], {max_extent:['1',2,3,4]}, function(err, result) {}); });
        assert.throws(function() { vtile1.compositeSync([vtile2], {image_scaling:'foo'}); });
        assert.throws(function() { vtile1.compositeSync([vtile2], {image_scaling:[]}); });
        assert.throws(function() { vtile1.composite([vtile2], {image_scaling:'foo'}, function(err, result) {}); });
        assert.throws(function() { vtile1.composite([vtile2], {image_scaling:[]}, function(err, result) {}); });
        assert.throws(function() { vtile1.compositeSync([vtile3], {image_format:'foo',reencode:true}); });
        assert.throws(function() { vtile1.compositeSync([vtile3], {image_format:[],reencode:true}); });
        assert.throws(function() { vtile1.composite([vtile2], {image_format:[],reencode:true}, function(err, result) {}); });
        vtile1.composite([vtile3], {image_format:'foo',reencode:true}, function(err, result) {
            assert.throws(function() { if (err) throw err; });
            done();
        });
    });

    it('should support compositing tiles that were just rendered to sync', function(done) {
        render_fresh_tile('lines',[1,0,0], function(err,vtile1) {
            if (err) throw err;
            assert.equal(vtile1.getData().length,55);
            var vtile2 = new mapnik.VectorTile(1,0,0, {buffer_size: 1});
            // Since the tiles are same location, no rendering is required
            // so these options have no effect
            var options = {
                scale: 1.0,
                offset_x: 0,
                offset_y: 0,
                area_threshold: 0.1,
                strictly_simple: false,
                multi_polygon_union: true,
                fill_type: mapnik.polygonFillType.nonZero,
                process_all_rings:false,
                scale_denominator: 0.0,
                simplify_distance: 0.0,
                reencode: false
            }
            vtile2.compositeSync([vtile1,vtile1],options);
            assert.equal(vtile2.getData().length,55);
            assert.deepEqual(vtile2.names(),["lines-1-0-0"]);
            done();
        });
    });

    it('should support compositing tiles that were just rendered to sync (reencode)', function(done) {
        render_fresh_tile('lines',[1,0,0], function(err,vtile1) {
            if (err) throw err;
            assert.equal(vtile1.getData().length,55);
            var vtile2 = new mapnik.VectorTile(1,0,0, {buffer_size: 1});
            var vtile3 = new mapnik.VectorTile(1,0,0, {buffer_size: 1});
            // Since the tiles are same location, no rendering is required
            // so these options have no effect
            var options = {
                scale: 1.0,
                offset_x: 0,
                offset_y: 0,
                area_threshold: 0.1,
                strictly_simple: false,
                multi_polygon_union: true,
                fill_type: mapnik.polygonFillType.nonZero,
                process_all_rings:false,
                scale_denominator: 0.0,
                reencode: true
            }
            vtile2.compositeSync([vtile1,vtile1],options);
            assert.equal(vtile2.getData().length,53);
            assert.deepEqual(vtile2.names(),["lines-1-0-0"]);
            vtile3.composite([vtile1,vtile1],options,function(err) {
                if (err) throw err;
                assert.equal(vtile3.getData().length,53);
                assert.deepEqual(vtile3.names(),["lines-1-0-0"]);
                done();
            });
        });
    });
    
    it('should support compositing tiles and clipping to max_extent (reencode)', function(done) {
        var map = new mapnik.Map(256,256);
        var map_template = fs.readFileSync('./test/data/vector_tile/generic_map.xml', 'utf8');
        var color = function(str) {
          var rgb = [0, 0, 0];
          for (var i = 0; i < str.length; i++) {
              var v = str.charCodeAt(i);
              rgb[v % 3] = (rgb[i % 3] + (13*(v%13))) % 12;
          }
          var r = 4 + rgb[0];
          var g = 4 + rgb[1];
          var b = 4 + rgb[2];
          r = (r * 16) + r;
          g = (g * 16) + g;
          b = (b * 16) + b;
          return [r,g,b];
        };
        var vtile2 = new mapnik.VectorTile(0,0,0);
        vtile2.setData(fs.readFileSync('./test/data/v6-0_0_0.mvt'));
        var xml = '<Map srs="+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0.0 +k=1.0 +units=m +nadgrids=@null +wktext +no_defs +over" background-color="#000000" maximum-extent="-20037508.34,-20037508.34,20037508.34,20037508.34">';
        xml += vtile2.names().map(function(name) {
                    var rgb = color(name).join(',');
                    return util.format(map_template, name, rgb, rgb, rgb, rgb, rgb, name, name);
                  }).join('\n');
        xml += '</Map>';
        map.fromStringSync(xml);

        var vtile = new mapnik.VectorTile(0,0,0);
        var vtile1 = new mapnik.VectorTile(0,0,0);
        var vtile3 = new mapnik.VectorTile(0,0,0);
        var vtile4 = new mapnik.VectorTile(0,0,0);
        var world_clipping_extent = [-20037508.34,-20037508.34,20037508.34,20037508.34];
        vtile.composite([vtile2],{reencode:true});
        vtile1.composite([vtile2],{reencode:true,max_extent:world_clipping_extent});
        assert.equal(vtile.getData().length,54781);
        assert.deepEqual(vtile.names(),["water","admin"]);
        assert.equal(vtile1.getData().length,54522);
        assert.deepEqual(vtile1.names(),["water","admin"]);
        var expected_file = data_base +'/expected/world-reencode.png';
        var expected_file2 = data_base +'/expected/world-reencode-max-extent.png';
        vtile.render(map,new mapnik.Image(256,256),function(err,im) {
            if (err) throw err;
            assert.equal(0,compare_to_image(im,expected_file));
            vtile1.render(map,new mapnik.Image(256,256),function(err,im2) {
                if (err) throw err;
                assert.equal(0,compare_to_image(im2,expected_file2));
                vtile3.composite([vtile2],{reencode:true}, function(err) {
                    if (err) throw err;
                    assert.equal(vtile3.getData().length,54781);
                    assert.deepEqual(vtile3.names(),["water","admin"]);
                    vtile3.render(map,new mapnik.Image(256,256),function(err,im) {
                        if (err) throw err;
                        assert.equal(0,compare_to_image(im,expected_file));
                        vtile4.composite([vtile2],{reencode:true,max_extent:world_clipping_extent}, function(err) {
                            if (err) throw err;
                            assert.equal(vtile4.getData().length,54522);
                            assert.deepEqual(vtile4.names(),["water","admin"]);
                            assert.equal(0,compare_to_image(im2,expected_file2));
                            vtile4.render(map,new mapnik.Image(256,256),function(err,im) {
                                if (err) throw err;
                                assert.equal(0,compare_to_image(im,expected_file2));
                                done();
                            });
                        });
                    });
                });
            });
        });
    });

    it('should support compositing tiles that were just rendered to async', function(done) {
        render_fresh_tile('lines',[1,0,0], function(err,vtile1) {
            if (err) throw err;
            assert.equal(vtile1.getData().length,55);
            var vtile2 = new mapnik.VectorTile(1,0,0);
            // Since the tiles are same location, no rendering is required
            // so these options have no effect
            var options = {
                scale: 1.0,
                offset_x: 0,
                offset_y: 0,
                area_threshold: 0.1,
                strictly_simple: false,
                multi_polygon_union: true,
                fill_type: mapnik.polygonFillType.nonZero,
                process_all_rings:false,
                scale_denominator: 0.0,
                reencode: false
            }
            vtile2.composite([vtile1,vtile1],options, function(err, vtile2) {
                if (err) throw err;
                assert.equal(vtile2.getData().length,55);
                assert.deepEqual(vtile2.names(),["lines-1-0-0"]);
                done();
            });
        });
    });

    it('should support compositing tiles that were just rendered to async (reencode)', function(done) {
        render_fresh_tile('lines',[1,0,0], function(err,vtile1) {
            if (err) throw err;
            assert.equal(vtile1.getData().length,55);
            var vtile2 = new mapnik.VectorTile(1,0,0);
            // Since the tiles are same location, no rendering is required
            // so these options have no effect
            var options = {
                scale: 1.0,
                offset_x: 0,
                offset_y: 0,
                area_threshold: 0.1,
                strictly_simple: false,
                multi_polygon_union: true,
                fill_type: mapnik.polygonFillType.nonZero,
                process_all_rings:false,
                scale_denominator: 0.0,
                reencode: true
            }
            vtile2.composite([vtile1,vtile1],options, function(err, vtile2) {
                if (err) throw err;
                assert.equal(vtile2.getData().length,55);
                assert.deepEqual(vtile2.names(),["lines-1-0-0"]);
                done();
            });
        });
    });

    it('should render with simple concatenation', function(done) {
        var coords = [0,0,0];
        var vtile = new mapnik.VectorTile(coords[0],coords[1],coords[2]);
        var vtiles = [get_tile_at('lines',coords),get_tile_at('points',coords)];
        var expected_length = get_data_at('lines',coords).length + get_data_at('points',coords).length;
        // alternative method of getting combined length
        var expected_length2 = Buffer.concat([vtiles[0].getData(),vtiles[1].getData()]).length;
        // Let's confirm they match
        assert.equal(expected_length,expected_length2);
        // Now composite the tiles together
        var opts = {}; // NOTE: options here will have no impact in the case of concatenation
        vtile.composite(vtiles,opts);
        var composited_data = vtile.getData();
        assert.equal(composited_data.length,expected_length);
        assert.deepEqual(vtile.names(),['lines-0-0-0','points-0-0-0']);
        // ensure the lengths still match
        assert.equal(vtile.getData().length,expected_length);
        var map = new mapnik.Map(256,256);
        map.loadSync(data_base +'/styles/all.xml');
        vtile.render(map,new mapnik.Image(256,256),function(err,im) {
            if (err) throw err;
            var expected_file = data_base +'/expected/concat.png';
            assert.equal(0,compare_to_image(im,expected_file));
            done();
        });
    });

    it('should render with image concatenation', function(done) {
        var coords = [0,0,0];
        var vtile = new mapnik.VectorTile(coords[0],coords[1],coords[2]);
        var vtiles = [
            get_image_vtile(coords,'cloudless_1_0_0.jpg','raster','jpeg'),
            get_tile_at('lines',coords),
            get_tile_at('points',coords)
        ];
        vtile.composite(vtiles,{});
        assert.deepEqual(vtile.names(),['raster','lines-0-0-0','points-0-0-0']);
        assert.deepEqual(vtile.toJSON().map(function(l) { return l.name; }), ['raster','lines-0-0-0','points-0-0-0']);
        var map = new mapnik.Map(256,256);
        map.loadSync(data_base +'/styles/all.xml');
        vtile.render(map,new mapnik.Image(256,256),function(err,im) {
            if (err) throw err;
            var expected_file = data_base +'/expected/image_concat.png';
            assert.ok(compare_to_image(im,expected_file) < 525);
            done();
        });
    });

    it('should render by overzooming+jpeg+near', function(done) {
        var vtile = new mapnik.VectorTile(2,1,1);
        var vtile2 = new mapnik.VectorTile(2,1,1);
        var vtiles = [get_image_vtile([0,0,0],'cloudless_1_0_0.jpg','raster','jpeg'),get_tile_at('lines',[0,0,0]),get_tile_at('points',[1,1,1])];
        // raw length of input buffers
        var opts = {image_format:"jpeg",image_scaling:"near"};
        vtile2.composite(vtiles,opts, function(err) {
            if (err) throw err;
            vtile.composite(vtiles,opts);
            assert.equal(vtile.getData().length,vtile2.getData().length);
            var new_length = vtile.getData().length;
            // re-rendered data should be different length
            assert.deepEqual(vtile.names(),['raster','lines-0-0-0','points-1-1-1']);
            assert.equal(new_length,vtile.getData().length);
            var json_result = vtile.toJSON();
            assert.equal(json_result.length,3);
            assert.equal(json_result[0].features.length,1);
            assert.equal(json_result[1].features.length,2);
            assert.equal(json_result[2].features.length,1);
            // tile is actually bigger because of how geometries are encoded
            var map = new mapnik.Map(256,256);
            map.loadSync(data_base +'/styles/all.xml');
            vtile.render(map,new mapnik.Image(256,256),{buffer_size:256,image_format:"jpeg",image_scaling:"near"},function(err,im) {
                if (err) throw err;
                var expected_file = data_base +'/expected/2-1-1.png';
                //im.save(data_base +'/expected/2-1-1.png');
                assert.equal(0,compare_to_image(im,expected_file));
                done();
            });
        });
    });

    it('should render by overzooming+webp+biliear', function(done) {
        var vtile = new mapnik.VectorTile(2,1,1);
        var vtile2 = new mapnik.VectorTile(2,1,1);
        var vtiles = [get_image_vtile([0,0,0],'cloudless_1_0_0.jpg','raster', 'jpeg'),get_image_vtile([2,1,1],'13-2411-3080.png','raster2', 'png'),get_tile_at('lines',[0,0,0]),get_tile_at('points',[1,1,1])];
        // raw length of input buffers
        var opts = {image_format:"webp",image_scaling:"bilinear"};
        vtile2.composite(vtiles,opts, function(err) {
            if (err) throw err;
            vtile.composite(vtiles,opts);
            assert.equal(vtile.getData().length,vtile2.getData().length);
            var new_length = vtile.getData().length;
            // raster2 is first in list after composite because it is in the same tile and can be
            // simply copied. The others require reprojection and therefore must go through
            // the processor.
            assert.deepEqual(vtile.names(),['raster2','raster','lines-0-0-0','points-1-1-1']);
            assert.equal(new_length,vtile.getData().length);
            var json_result = vtile.toJSON();
            assert.equal(json_result.length,4);
            assert.equal(json_result[0].features.length,1);
            assert.equal(json_result[1].features.length,1);
            assert.equal(json_result[2].features.length,2);
            assert.equal(json_result[3].features.length,1);
            var map = new mapnik.Map(256,256);
            map.loadSync(data_base +'/styles/all.xml');
            vtile.render(map,new mapnik.Image(256,256),{buffer_size:256,image_format:"webp",image_scaling:"bilinear"},function(err,im) {
                if (err) throw err;
                var expected_file = data_base +'/expected/2-1-1b.png';
                //im.save(data_base +'/expected/2-1-1b.png');
                assert.equal(0,compare_to_image(im,expected_file));
                done();
            });
        });
    });
    
    it('should render by overzooming+webp+biliear with threading mode auto', function(done) {
        var vtile = new mapnik.VectorTile(2,1,1);
        var vtile2 = new mapnik.VectorTile(2,1,1);
        var vtiles = [get_image_vtile([0,0,0],'cloudless_1_0_0.jpg','raster', 'jpeg'),get_image_vtile([2,1,1],'13-2411-3080.png','raster2', 'png'),get_tile_at('lines',[0,0,0]),get_tile_at('points',[1,1,1])];
        // raw length of input buffers
        var opts = {image_format:"webp",image_scaling:"bilinear"};
        // until bug is fixed in std::future do not run this test on osx
        if (process.platform == 'darwin') {
            var opts = { threading_mode: mapnik.threadingMode.deferred };
        } else {
            var opts = { threading_mode: mapnik.threadingMode.auto };
        }
        vtile2.composite(vtiles, opts, function(err) {
            if (err) throw err;
            vtile.composite(vtiles,opts);
            assert.equal(vtile.getData().length,vtile2.getData().length);
            var new_length = vtile.getData().length;
            // raster2 is first in list after composite because it is in the same tile and can be
            // simply copied. The others require reprojection and therefore must go through
            // the processor.
            assert.deepEqual(vtile.names(),['raster2','raster','lines-0-0-0','points-1-1-1']);
            assert.equal(new_length,vtile.getData().length);
            var json_result = vtile.toJSON();
            assert.equal(json_result.length,4);
            assert.equal(json_result[0].features.length,1);
            assert.equal(json_result[1].features.length,1);
            assert.equal(json_result[2].features.length,2);
            assert.equal(json_result[3].features.length,1);
            var map = new mapnik.Map(256,256);
            map.loadSync(data_base +'/styles/all.xml');
            vtile.render(map,new mapnik.Image(256,256),{buffer_size:256,image_format:"webp",image_scaling:"bilinear"},function(err,im) {
                if (err) throw err;
                var expected_file = data_base +'/expected/2-1-1b.png';
                //im.save(data_base +'/expected/2-1-1b.png');
                assert.equal(0,compare_to_image(im,expected_file));
                done();
            });
        });
    });
    
    it('should render by overzooming+webp+biliear with threading mode async', function(done) {
        var vtile = new mapnik.VectorTile(2,1,1);
        var vtile2 = new mapnik.VectorTile(2,1,1);
        var vtiles = [get_image_vtile([0,0,0],'cloudless_1_0_0.jpg','raster', 'jpeg'),get_image_vtile([2,1,1],'13-2411-3080.png','raster2', 'png'),get_tile_at('lines',[0,0,0]),get_tile_at('points',[1,1,1])];
        // raw length of input buffers
        var opts = {image_format:"webp",image_scaling:"bilinear"};
        // until bug is fixed in std::future do not run this test on osx
        if (process.platform == 'darwin') {
            var opts = { threading_mode: mapnik.threadingMode.deferred };
        } else {
            var opts = { threading_mode: mapnik.threadingMode.async };
        }
        vtile2.composite(vtiles, opts, function(err) {
            if (err) throw err;
            vtile.composite(vtiles,opts);
            assert.equal(vtile.getData().length,vtile2.getData().length);
            var new_length = vtile.getData().length;
            // raster2 is first in list after composite because it is in the same tile and can be
            // simply copied. The others require reprojection and therefore must go through
            // the processor.
            assert.deepEqual(vtile.names(),['raster2','raster','lines-0-0-0','points-1-1-1']);
            assert.equal(new_length,vtile.getData().length);
            var json_result = vtile.toJSON();
            assert.equal(json_result.length,4);
            assert.equal(json_result[0].features.length,1);
            assert.equal(json_result[1].features.length,1);
            assert.equal(json_result[2].features.length,2);
            assert.equal(json_result[3].features.length,1);
            var map = new mapnik.Map(256,256);
            map.loadSync(data_base +'/styles/all.xml');
            vtile.render(map,new mapnik.Image(256,256),{buffer_size:256,image_format:"webp",image_scaling:"bilinear"},function(err,im) {
                if (err) throw err;
                var expected_file = data_base +'/expected/2-1-1b.png';
                //im.save(data_base +'/expected/2-1-1b.png');
                assert.equal(0,compare_to_image(im,expected_file));
                done();
            });
        });
    });

    it('should render with custom buffer_size', function(done) {
        var vtile = new mapnik.VectorTile(2,1,1, {buffer_size: -2000});
        var vtiles = [get_tile_at('lines',[0,0,0]),get_tile_at('points',[1,1,1])];
        vtile.composite(vtiles);
        assert.deepEqual(vtile.names(),[]);
        var json_result = vtile.toJSON();
        assert.equal(json_result.length,0);
        var map = new mapnik.Map(256,256);
        map.loadSync(data_base +'/styles/all.xml');
        vtile.render(map,new mapnik.Image(256,256),{buffer_size:256},function(err,im) {
            if (err) throw err;
            var expected_file = data_base +'/expected/2-1-1-empty.png';
            assert.equal(0,compare_to_image(im,expected_file));
            done();
        });
    });

    it('should render by overzooming (drops point)', function(done) {
        var vtile = new mapnik.VectorTile(2,1,1);
        var vtiles = [get_tile_at('lines',[2,1,1]),get_tile_at('points',[2,0,1])];
        vtile.composite(vtiles);
        assert.deepEqual(vtile.names(),["lines-2-1-1"]);
        var json_result = vtile.toJSON();
        assert.equal(json_result.length,1);
        assert.equal(json_result[0].features.length,2);
        var map = new mapnik.Map(256,256);
        map.loadSync(data_base +'/styles/all.xml');
        vtile.render(map,new mapnik.Image(256,256),{buffer_size:256},function(err,im) {
            if (err) throw err;
            var expected_file = data_base +'/expected/2-1-1-no-point.png';
            //im.save(data_base +'/expected/2-1-1-no-point.png');
            assert.equal(0,compare_to_image(im,expected_file));
            done();
        });
    });

    // NOTE: this is a unintended usecase, but it can be done, so let's test it
    it('should render by underzooming or mosaicing', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0, {buffer:80});
        var vtiles = [];
        tiles.forEach(function(coords) {
            if (coords[0] == 1) {
                vtiles.push(get_tile_at('lines',[coords[0],coords[1],coords[2]]));
                vtiles.push(get_tile_at('points',[coords[0],coords[1],coords[2]]));
            }
        });
        vtile.composite(vtiles);
        var expected_names = ["lines-1-0-0",
                              "points-1-0-0",
                              "lines-1-0-1",
                              "points-1-0-1",
                              "lines-1-1-0",
                              "points-1-1-0",
                              "lines-1-1-1",
                              "points-1-1-1"];
        assert.deepEqual(vtile.names(),expected_names);
        var json_result = vtile.toJSON();
        assert.equal(json_result.length,8);
        assert.equal(json_result[0].features.length,2);
        assert.equal(json_result[1].features.length,1);
        // tile is actually bigger because of how geometries are encoded
        assert.ok(vtile.getData().length > Buffer.concat([vtiles[0].getData(),vtiles[1].getData()]).length);
        var map = new mapnik.Map(256,256);
        map.loadSync(data_base +'/styles/all.xml');
        vtile.render(map,new mapnik.Image(256,256),{buffer_size:256},function(err,im) {
            if (err) throw err;
            var expected_file = data_base +'/expected/0-0-0-mosaic.png';
            //im.save(data_base + '/expected/0-0-0-mosaic.png');
            assert.equal(0,compare_to_image(im,expected_file));
            done();
        });
    });

    it('should contain two raster layers', function(done) {
        // two tiles that do not overlap
        var vt = new mapnik.VectorTile(0,0,0);
        vt.tileSize = 256;
        var im = new mapnik.Image(vt.tileSize,vt.tileSize);
        im.background = new mapnik.Color('green');
        vt.addImage(im, 'green');
        var vt2 = new mapnik.VectorTile(0,0,0);
        vt2.tileSize = 256;
        var im2 = new mapnik.Image(vt.tileSize,vt.tileSize);
        im2.background = new mapnik.Color('blue');
        vt2.addImage(im2, 'blue');
        vt.composite([vt2]);
        assert.deepEqual(vt.names(),['green','blue']);
        done();
    });

    it('should not contain non-overlapping data', function(done) {
        // two tiles that do not overlap
        var vt = new mapnik.VectorTile(1,0,0);
        vt.tileSize = 256;
        var im = new mapnik.Image(vt.tileSize,vt.tileSize);
        im.background = new mapnik.Color('green');
        vt.addImage(im, 'green');
        var vt2 = new mapnik.VectorTile(1,1,0);
        vt2.tileSize = 256;
        var im2 = new mapnik.Image(vt.tileSize,vt.tileSize);
        im2.background = new mapnik.Color('blue');
        vt2.addImage(im2, 'blue');
        vt.composite([vt2]);
        assert.deepEqual(vt.names(),['green']);
        done();
    });

    it('non intersecting layers should be discarded when compositing', function(done) {
        // two tiles that do not overlap
        var vt1 = new mapnik.VectorTile(1,0,0); // north america
        var vt2 = new mapnik.VectorTile(1,0,1); // south america
        // point in 1,0,0
        var geojson1 = {
          "type": "FeatureCollection",
          "features": [
            {
              "type": "Feature",
              "geometry": {
                "type": "Point",
                "coordinates": [-100,60]
              },
              "properties": {
                "name": "geojson data"
              }
            }
          ]
        };
        vt1.addGeoJSON(JSON.stringify(geojson1),"na");
        assert.deepEqual(vt1.names(),["na"]);
        // clone from raw buffer
        var vt1b = new mapnik.VectorTile(1,0,0);
        vt1b.setData(vt1.getData());
        assert.deepEqual(vt1b.names(),["na"]);

        // point in 1,0,1
        var geojson2 = {
          "type": "FeatureCollection",
          "features": [
            {
              "type": "Feature",
              "geometry": {
                "type": "Point",
                "coordinates": [-100,-60]
              },
              "properties": {
                "name": "geojson data"
              }
            }
          ]
        };
        vt2.addGeoJSON(JSON.stringify(geojson2),"sa");
        assert.deepEqual(vt2.names(),["sa"]);
        // clone from raw buffer
        var vt2b = new mapnik.VectorTile(1,0,1);
        vt2b.setData(vt2.getData());
        assert.deepEqual(vt2b.names(),["sa"]);

        vt1.composite([vt2]);
        assert.deepEqual(vt1.names(),["na"]);

        // verify they would intersect with large buffer.
        vt1b.bufferSize = 4096;
        vt1b.composite([vt2b]);
        assert.deepEqual(vt1b.names(),["na", "sa"]);
        done();
    });

    it('compositing a non-intersecting layer into an empty layer should not throw when parsed', function(done) {
        // two tiles that do not overlap
        var vt1 = new mapnik.VectorTile(1,0,0); // north america
        var vt2 = new mapnik.VectorTile(1,0,1); // south america
        // point in 1,0,1
        var geojson2 = {
          "type": "FeatureCollection",
          "features": [
            {
              "type": "Feature",
              "geometry": {
                "type": "Point",
                "coordinates": [-100,-60]
              },
              "properties": {
                "name": "geojson data"
              }
            }
          ]
        };
        vt2.addGeoJSON(JSON.stringify(geojson2),"sa");
        assert.deepEqual(vt2.names(),["sa"]);
        // clone from raw buffer
        var vt2b = new mapnik.VectorTile(1,0,1);
        vt2b.setData(vt2.getData());
        assert.deepEqual(vt2b.names(),["sa"]);
        vt1.bufferSize = 0;
        vt1.composite([vt2]);
        assert.deepEqual(vt1.names(),[]);
        done();
    });

    it('should correctly composite -- numerical precision issue in mapnik vector tile area calculation', function() {
        // Original data.
        var vt = new mapnik.VectorTile(16,10691,25084);
        var vt_data = fs.readFileSync('./test/data/vector_tile/compositing/25084.vector.pbf');
        var vt2 = new mapnik.VectorTile(16,10691,25084);
        var vt_data2 = fs.readFileSync('./test/data/vector_tile/compositing/25084_2.vector.pbf');
        vt.setData(vt_data);
        // Tile data
        var vtile = new mapnik.VectorTile(18,42764,100336, {buffer_size: 255*16});
        vtile.composite([vt, vt2]);
        assert(!vtile.empty());
        assert.doesNotThrow(function() {
            vtile.toGeoJSON('wildoak');
        });
    });

    it('should correctly composite again -- numerical precision issue in mapnik vector tile area calculation - 2', function(done) {
        // Tile data
        var vtile = new mapnik.VectorTile(15,8816,12063);
        var vt_data = fs.readFileSync('./test/data/vector_tile/compositing/badtile.mvt');
        vtile.setData(vt_data);
        assert(!vtile.empty());
        assert.doesNotThrow(function() {
            vtile.toGeoJSON('__all__');
        });
        var map = new mapnik.Map(256,256);
        map.loadSync('./test/data/vector_tile/compositing/badtile.xml');
        var opts = { z: 16, x: 17632, y: 24126, scale: 1, buffer_size: 256 };
        vtile.render(map, new mapnik.Image(256,256), opts, function(err, image) {
            if (err) throw err;
            done();
        });
    });
});
