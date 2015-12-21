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
    path_multiplier: 16,
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
    buffer_size: 1,
    image_format: "jpeg",
    image_scaling: "near"
};

function render_data(name,coords,callback) {
    var map = new mapnik.Map(256, 256);
    map.loadSync(data_base +'/layers/'+name+'.xml');
    var vtile = new mapnik.VectorTile(coords[0],coords[1],coords[2]);
    var extent = mercator.bbox(coords[1],coords[2],coords[0], false, '900913');
    name = name + '-' + coords.join('-');
    map.extent = extent;
    var opts = JSON.parse(JSON.stringify(rendering_defaults));
    // buffer of >=5 is needed to ensure point ends up in tiles touching null island
    opts.buffer_size = 5;
    //map.renderFileSync('./test/data/vector_tile/compositing/'+name+'.png')
    map.render(vtile,opts,function(err,vtile) {
        if (err) return callback(err);
        var tiledata = vtile.getData();
        var tilename = data_base +'/tiles/'+name+'.vector.pbf';
        fs.writeFileSync(tilename,tiledata);
        return callback();
    });
}

function render_fresh_tile(name,coords,callback) {
    var map = new mapnik.Map(256, 256);
    map.loadSync(data_base +'/layers/'+name+'.xml');
    var vtile = new mapnik.VectorTile(coords[0],coords[1],coords[2]);
    var extent = mercator.bbox(coords[1],coords[2],coords[0], false, '900913');
    name = name + '-' + coords.join('-');
    map.extent = extent;
    var opts = JSON.parse(JSON.stringify(rendering_defaults));
    // buffer of >=5 is needed to ensure point ends up in tiles touching null island
    opts.buffer_size = 5;
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
    return fs.readFileSync(data_base +'/tiles/'+name+'-'+coords.join('-')+'.vector.pbf');
}

function get_tile_at(name,coords) {
    var vt = new mapnik.VectorTile(coords[0],coords[1],coords[2]);
    vt.setData(get_data_at(name,coords));
    return vt;
}

function get_image_vtile(coords,file,name) {
    var vt = new mapnik.VectorTile(coords[0],coords[1],coords[2]);
    vt.addImage(fs.readFileSync(__dirname + '/data/vector_tile/'+file), name);
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
        var vtile3 = new mapnik.VectorTile(3,0,0, {width:0, height:0});
        var vtile4 = new mapnik.VectorTile(4,0,0, {width:-1, height:-1});
        var vtile5 = get_image_vtile([0,0,0],'cloudless_1_0_0.jpg','raster');
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
        assert.throws(function() { vtile1.compositeSync([vtile2], {path_multiplier:null}); });
        assert.throws(function() { vtile1.composite([vtile2], {path_multiplier:null}, function(err, result) {}); });
        assert.throws(function() { vtile1.compositeSync([vtile2], {buffer_size:null}); });
        assert.throws(function() { vtile1.composite([vtile2], {buffer_size:null}, function(err, result) {}); });
        assert.throws(function() { vtile1.compositeSync([vtile2], {scale:null}); });
        assert.throws(function() { vtile1.composite([vtile2], {scale:null}, function(err, result) {}); });
        assert.throws(function() { vtile1.compositeSync([vtile2], {offset_x:null}); });
        assert.throws(function() { vtile1.composite([vtile2], {offset_x:null}, function(err, result) {}); });
        assert.throws(function() { vtile1.compositeSync([vtile2], {offset_y:null}); });
        assert.throws(function() { vtile1.composite([vtile2], {offset_y:null}, function(err, result) {}); });
        assert.throws(function() { vtile1.compositeSync([vtile2], {scale_denominator:null}); });
        assert.throws(function() { vtile1.composite([vtile2], {scale_denominator:null}, function(err, result) {}); });
        assert.throws(function() { vtile1.compositeSync([vtile2], {area_threshold:null}); });
        assert.throws(function() { vtile1.composite([vtile2], {area_threshold:null}, function(err, result) {}); });
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
        assert.throws(function() { vtile3.compositeSync([vtile1]); });
        assert.throws(function() { vtile1.compositeSync([vtile3]); });
        assert.throws(function() { vtile1.compositeSync([vtile2], {image_scaling:'foo'}); });
        assert.throws(function() { vtile1.composite([vtile2], {image_scaling:'foo'}, function(err, result) {}); });
        assert.throws(function() { vtile1.compositeSync([vtile5], {image_format:'foo',reencode:true}); });
        vtile1.composite([vtile5], {image_format:'foo',reencode:true}, function(err, result) {
            assert.throws(function() { if (err) throw err; });
            vtile3.composite([vtile1], function(err, result) {
                assert.throws(function() { if (err) throw err; });
                vtile1.composite([vtile3], function(err, result) {
                    assert.throws(function() { if (err) throw err; });
                    done();
                });
            });
        });
    });

    it('should support compositing tiles that were just rendered to sync', function(done) {
        render_fresh_tile('lines',[1,0,0], function(err,vtile1) {
            if (err) throw err;
            assert.equal(vtile1.getData().length,49);
            var vtile2 = new mapnik.VectorTile(1,0,0);
            // Since the tiles are same location, no rendering is required
            // so these options have no effect
            var options = {
                path_multiplier: 16,
                buffer_size: 1,
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
            assert.equal(vtile2.getData().length,98);
            assert.deepEqual(vtile2.names(),["lines","lines"]);
            done();
        });
    });

    it('should support compositing tiles that were just rendered to sync (reencode)', function(done) {
        render_fresh_tile('lines',[1,0,0], function(err,vtile1) {
            if (err) throw err;
            assert.equal(vtile1.getData().length,49);
            var vtile2 = new mapnik.VectorTile(1,0,0);
            var vtile3 = new mapnik.VectorTile(1,0,0);
            // Since the tiles are same location, no rendering is required
            // so these options have no effect
            var options = {
                path_multiplier: 16,
                buffer_size: 1,
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
            assert.equal(vtile2.getData().length,94);
            assert.deepEqual(vtile2.names(),["lines","lines"]);
            vtile3.composite([vtile1,vtile1],options,function(err) {
                if (err) throw err;
                assert.equal(vtile3.getData().length,94);
                assert.deepEqual(vtile3.names(),["lines","lines"]);
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
        vtile2.setData(fs.readFileSync('./test/data/v6-0_0_0.vector.pbf'));
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
        assert.equal(vtile.getData().length,54626);
        assert.deepEqual(vtile.names(),["water","admin"]);
        assert.equal(vtile1.getData().length,54461);
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
                    assert.equal(vtile3.getData().length,54626);
                    assert.deepEqual(vtile3.names(),["water","admin"]);
                    vtile3.render(map,new mapnik.Image(256,256),function(err,im) {
                        if (err) throw err;
                        assert.equal(0,compare_to_image(im,expected_file));
                        vtile4.composite([vtile2],{reencode:true,max_extent:world_clipping_extent}, function(err) {
                            if (err) throw err;
                            assert.equal(vtile4.getData().length,54461);
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
            assert.equal(vtile1.getData().length,49);
            var vtile2 = new mapnik.VectorTile(1,0,0);
            // Since the tiles are same location, no rendering is required
            // so these options have no effect
            var options = {
                path_multiplier: 16,
                buffer_size: 1,
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
                assert.equal(vtile2.getData().length,98);
                assert.deepEqual(vtile2.names(),["lines","lines"]);
                done();
            });
        });
    });

    it('should support compositing tiles that were just rendered to async (reencode)', function(done) {
        render_fresh_tile('lines',[1,0,0], function(err,vtile1) {
            if (err) throw err;
            assert.equal(vtile1.getData().length,49);
            var vtile2 = new mapnik.VectorTile(1,0,0);
            // Since the tiles are same location, no rendering is required
            // so these options have no effect
            var options = {
                path_multiplier: 16,
                buffer_size: 1,
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
                assert.equal(vtile2.getData().length,94);
                assert.deepEqual(vtile2.names(),["lines","lines"]);
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
        assert.deepEqual(vtile.names(),['lines','points']);
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
            get_image_vtile(coords,'cloudless_1_0_0.jpg','raster'),
            get_tile_at('lines',coords),
            get_tile_at('points',coords)
        ];
        vtile.composite(vtiles,{});
        assert.deepEqual(vtile.names(),['raster','lines','points']);
        assert.deepEqual(vtile.toJSON().map(function(l) { return l.name; }), ['raster','lines','points']);
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
        var vtiles = [get_image_vtile([0,0,0],'cloudless_1_0_0.jpg','raster'),get_tile_at('lines',[0,0,0]),get_tile_at('points',[1,1,1])];
        // raw length of input buffers
        var original_length = Buffer.concat([vtiles[0].getData(),vtiles[1].getData()]).length;
        vtile.composite(vtiles,{buffer_size:1,image_format:"jpeg",image_scaling:"near"});
        var new_length = vtile.getData().length;
        // re-rendered data should be different length
        assert.notEqual(new_length,original_length);
        assert.deepEqual(vtile.names(),['raster','lines','points']);
        assert.equal(new_length,vtile.getData().length);
        var json_result = vtile.toJSON();
        assert.equal(json_result.length,3);
        assert.equal(json_result[0].features.length,1);
        assert.equal(json_result[1].features.length,2);
        assert.equal(json_result[2].features.length,1);
        // tile is actually bigger because of how geometries are encoded
        assert.ok(vtile.getData().length > Buffer.concat([vtiles[0].getData(),vtiles[1].getData()]).length);
        var map = new mapnik.Map(256,256);
        map.loadSync(data_base +'/styles/all.xml');
        vtile.render(map,new mapnik.Image(256,256),{buffer_size:256,image_format:"jpeg",image_scaling:"near"},function(err,im) {
            if (err) throw err;
            var expected_file = data_base +'/expected/2-1-1.png';
            assert.equal(0,compare_to_image(im,expected_file));
            done();
        });
    });

    it('should render by overzooming+webp+biliear', function(done) {
        var vtile = new mapnik.VectorTile(2,1,1);
        var vtiles = [get_image_vtile([0,0,0],'cloudless_1_0_0.jpg','raster'),get_image_vtile([2,1,1],'13-2411-3080.png','raster2'),get_tile_at('lines',[0,0,0]),get_tile_at('points',[1,1,1])];
        // raw length of input buffers
        var original_length = Buffer.concat([vtiles[0].getData(),vtiles[1].getData()]).length;
        vtile.composite(vtiles,{buffer_size:1,image_format:"webp",image_scaling:"bilinear"});
        var new_length = vtile.getData().length;
        // re-rendered data should be different length
        assert.notEqual(new_length,original_length);
        assert.deepEqual(vtile.names(),['raster','raster2','lines','points']);
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
            assert.equal(0,compare_to_image(im,expected_file));
            done();
        });
    });

    it('should render with custom buffer_size', function(done) {
        var vtile = new mapnik.VectorTile(2,1,1);
        var vtiles = [get_tile_at('lines',[0,0,0]),get_tile_at('points',[1,1,1])];
        var opts = {buffer_size:-256}; // will lead to dropped data
        vtile.composite(vtiles,opts);
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
        assert.deepEqual(vtile.names(),["lines"]);
        var json_result = vtile.toJSON();
        assert.equal(json_result.length,1);
        assert.equal(json_result[0].features.length,2);
        var map = new mapnik.Map(256,256);
        map.loadSync(data_base +'/styles/all.xml');
        vtile.render(map,new mapnik.Image(256,256),{buffer_size:256},function(err,im) {
            if (err) throw err;
            var expected_file = data_base +'/expected/2-1-1-no-point.png';
            assert.equal(0,compare_to_image(im,expected_file));
            done();
        });
    });

    // NOTE: this is a unintended usecase, but it can be done, so let's test it
    it('should render by underzooming or mosaicing', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0);
        var vtiles = [];
        tiles.forEach(function(coords) {
            if (coords[0] == 1) {
                vtiles.push(get_tile_at('lines',[coords[0],coords[1],coords[2]]));
                vtiles.push(get_tile_at('points',[coords[0],coords[1],coords[2]]));
            }
        });
        vtile.composite(vtiles);
        assert.deepEqual(vtile.names(),["lines","points","lines","points","lines","points","lines","points"]);
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
            assert.equal(0,compare_to_image(im,expected_file));
            done();
        });
    });

    it('should contain two raster layers', function(done) {
        // two tiles that do not overlap
        var vt = new mapnik.VectorTile(0,0,0);
        var im = new mapnik.Image(vt.width(),vt.height());
        im.background = new mapnik.Color('green');
        vt.addImage(im.encodeSync('webp'), 'green');
        var vt2 = new mapnik.VectorTile(0,0,0);
        var im2 = new mapnik.Image(vt.width(),vt.height());
        im2.background = new mapnik.Color('blue');
        vt2.addImage(im2.encodeSync('webp'), 'blue');
        vt.composite([vt2]);
        assert.deepEqual(vt.names(),['green','blue']);
        done();
    });

    it('should not contain non-overlapping data', function(done) {
        // two tiles that do not overlap
        var vt = new mapnik.VectorTile(1,0,0);
        var im = new mapnik.Image(vt.width(),vt.height());
        im.background = new mapnik.Color('green');
        vt.addImage(im.encodeSync('webp'), 'green');
        var vt2 = new mapnik.VectorTile(1,1,0);
        var im2 = new mapnik.Image(vt.width(),vt.height());
        im2.background = new mapnik.Color('blue');
        vt2.addImage(im2.encodeSync('webp'), 'blue');
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

        vt1.composite([vt2],{buffer_size:0});
        assert.deepEqual(vt1.names(),["na"]);

        vt1b.composite([vt2b],{buffer_size:0});
        assert.deepEqual(vt1b.names(),["na"]);
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
        vt1.composite([vt2],{buffer_size:0});
        assert.deepEqual(vt1.names(),[]);
        done();
    });

});
