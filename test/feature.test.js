"use strict";

var mapnik = require('../');
var assert = require('assert');
var path = require('path');

mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'shape.input'));
mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'csv.input'));

var trunc_6 = function(key, val) {
    return val.toFixed ? Number(val.toFixed(6)) : val;
};

function deepEqualTrunc(json1,json2) {
    var first = JSON.stringify(json1,trunc_6,1);
    var second = JSON.stringify(json2,trunc_6,1);
    if (first !== second) {
        throw new Error("JSON are not equal:\n" + first +  "\n------\n" + second);
    }
}

describe('mapnik.Feature ', function() {
    it('should throw with invalid usage', function() {
        // no 'new' keyword
        assert.throws(function() { mapnik.Feature(); });
        // invalid args
        assert.throws(function() { new mapnik.Feature(); });
        assert.throws(function() { new mapnik.Feature(1, 4, 5); });
        assert.throws(function() { new mapnik.Feature('foo'); });
    });

    it('should construct a feature properly', function() {
        var feature = new mapnik.Feature(1);
        assert.ok(feature);
        assert.deepEqual(feature.extent(),[0,0,-1,-1]);
        assert.throws(function() { mapnik.Feature.fromJSON(); });
        assert.throws(function() { mapnik.Feature.fromJSON(null); });
        assert.throws(function() { mapnik.Feature.fromJSON('foo'); });
        assert.throws(function() {
            var feat = mapnik.Feature.fromJSON('{"type":"Feature","id":1,"geometry":{"type":"Point","coordinates":[0,"b"]},"properties":{"feat_id":1,"name":"name"}}');
        });
    });

    it('should match known features', function() {
        var options = {
            type: 'shape',
            file: './test/data/world_merc.shp'
        };

        var ds = new mapnik.Datasource(options);
        // get one feature
        var featureset = ds.featureset();
        var feature = featureset.next();
        assert.deepEqual(feature.attributes(), {
            AREA: 44,
            FIPS: 'AC',
            ISO2: 'AG',
            ISO3: 'ATG',
            LAT: 17.078,
            LON: -61.783,
            NAME: 'Antigua and Barbuda',
            POP2005: 83039,
            REGION: 19,
            SUBREGION: 29,
            UN: 28
        });

        // loop over all of them to ensure the proper feature count
        var count = 1;
        while ((feature = featureset.next())) {
            count++;
        }
        assert.equal(count, 245);
    });

    it('should report null values as js null',function() {
        var extent = '-180,-60,180,60';
        var ds = new mapnik.MemoryDatasource({'extent': extent});
        var feat = {x:0,y:0,properties: {feat_id:1,null_val:null,name:"name",stuff: {hi:"wee"}}};
        var bad_feat = {x:'0',y:0,properties: {feat_id:1,null_val:null,name:"name",stuff: {hi:"wee"}}};
        assert.equal(ds.add(feat), true);
        assert.equal(ds.add(bad_feat), false);
        var featureset = ds.featureset();
        var feature = featureset.next();
        assert.equal(feature.id(),1);
        var attr = feature.attributes();
        assert.ok(attr.null_val === null);
        assert.equal(attr.feat_id,1);
        assert.equal(attr.name,'name');
    });

    it('should output the same geojson that it read', function () {
        var expected = {
                type: "Feature",
                properties: {},
                geometry: {
                    type: 'Polygon',
                    coordinates: [[[1,1],[1,2],[2,2],[2,1],[1,1]]]
                }
            };
        var ds = new mapnik.Datasource({type:'csv', 'inline': "geojson\n'" + JSON.stringify(expected.geometry) + "'"});
        var f = ds.featureset().next();
        var feature = JSON.parse(f.toJSON());

        assert.equal(expected.type, feature.type);
        assert.deepEqual(expected.properties, feature.properties);
        assert.equal(expected.geometry.type, feature.geometry.type);
        if (mapnik.versions.mapnik_number >= 200300) {
            assert.deepEqual(expected.geometry.coordinates, feature.geometry.coordinates);
        }
    });

    it('should be able to create feature from geojson and turn back into geojson', function (done) {
        var expected = {
            type: "Feature",
            properties: {},
            geometry: {
                type: 'Polygon',
                coordinates: [[[1,1],[1,2],[2,2],[2,1],[1,1]]]
            }
        };
        var f = new mapnik.Feature.fromJSON(JSON.stringify(expected));
        var feature = JSON.parse(f.toJSON());
        assert.equal(expected.type, feature.type);
        assert.deepEqual(expected.properties, feature.properties);
        assert.equal(expected.geometry.type, feature.geometry.type);
        if (mapnik.versions.mapnik_number >= 200300) {
            assert.deepEqual(expected.geometry.coordinates, feature.geometry.coordinates);
        }
        var geom = f.geometry();
        assert.deepEqual(geom.extent(),[ 1, 1, 2, 2 ]);
        var expected_geom = JSON.stringify(expected.geometry);
        assert.equal(expected_geom,geom.toJSON());
        var source_proj = new mapnik.Projection('+init=epsg:4326');
        var dest_proj = new mapnik.Projection('+init=epsg:3857');
        var trans = new mapnik.ProjTransform(source_proj,dest_proj);
        var transformed = geom.toJSON({transform:trans});
        assert.notEqual(expected_geom,transformed);
        // async toJSON
        geom.toJSON(function(err,json) {
            assert.equal(expected_geom,json);
            geom.toJSON({transform:trans},function(err,json2) {
                assert.equal(transformed,json2);
                done();
            });
        });
    });

    it('should be able to reproject geojson feature', function (done) {
        var merc_poly = {
         "type": "MultiPolygon",
         "coordinates": [
          [
           [
            [
             -6866928.47049373,
             1923670.30196653
            ],
            [
             -6889254.03965028,
             1933083.00247353
            ],
            [
             -6878926.59653091,
             1939845.92736324
            ],
            [
             -6866928.47049373,
             1923670.30196653
            ]
           ]
          ],
          [
           [
            [
             -6871659.99413039,
             1991787.21060401
            ],
            [
             -6885450.92056682,
             1988802.9255689
            ],
            [
             -6887677.75566065,
             2002918.07047496
            ],
            [
             -6871659.99413039,
             1991787.21060401
            ]
           ]
          ]
         ]
        };
        var input = {
            type: "Feature",
            properties: {},
            geometry: merc_poly
        };
        var f = new mapnik.Feature.fromJSON(JSON.stringify(input));
        var feature = JSON.parse(f.toJSON());
        assert.equal(input.type, feature.type);
        assert.deepEqual(input.properties, feature.properties);
        assert.equal(input.geometry.type, feature.geometry.type);
        if (mapnik.versions.mapnik_number >= 200300) {
            assert.deepEqual(input.geometry.coordinates, feature.geometry.coordinates);
        }
        var geom = f.geometry();
        var ext = geom.extent();
        var expected_ext = [-6889254.03965028,1923670.30196653,-6866928.47049373,2002918.07047496];
        assert.ok(Math.abs(ext[0] - expected_ext[0]) < 0.001);
        var input_geom = JSON.stringify(input.geometry);
        assert.equal(input_geom,geom.toJSON());
        var dest_proj = new mapnik.Projection('+init=epsg:4326');
        var source_proj = new mapnik.Projection('+init=epsg:3857');
        var trans = new mapnik.ProjTransform(source_proj,dest_proj);
        var expected_transformed = {
            "type": "MultiPolygon",
            "coordinates": [
              [
                [
                  [
                    -61.686668,
                    17.0244410000002
                  ],
                  [
                    -61.887222,
                    17.105274
                  ],
                  [
                    -61.7944489999999,
                    17.1633300000001
                  ],
                  [
                    -61.686668,
                    17.0244410000002
                  ]
                ]
              ],
              [
                [
                  [
                    -61.7291719999999,
                    17.608608
                  ],
                  [
                    -61.853058,
                    17.5830540000001
                  ],
                  [
                    -61.873062,
                    17.7038880000001
                  ],
                  [
                    -61.7291719999999,
                    17.608608
                  ]
                ]
              ]
            ]
        };
        var transformed = geom.toJSON({transform:trans});
        deepEqualTrunc(expected_transformed,JSON.parse(transformed));
        //assert.equal(expected_transformed,JSON.parse(transformed));
        assert.notEqual(input_geom,transformed);
        // async toJSON
        geom.toJSON(function(err,json) {
            assert.equal(input_geom,json);
            geom.toJSON({transform:trans},function(err,json2) {
                assert.equal(transformed,json2);
                done();
            });
        });
    });

    it('should output WKT', function () {
        var feature = {
                type: "Feature",
                properties: {},
                geometry: {
                    type: 'Polygon',
                    coordinates: [[[1,1],[1,2],[2,2],[2,1],[1,1]]]
                }
            };
        var expected = 'Polygon((1 1,1 2,2 2,2 1,1 1))';
        var ds = new mapnik.Datasource({type:'csv', 'inline': "geojson\n'" + JSON.stringify(feature.geometry) + "'"});
        var f = ds.featureset().next();
        assert.equal(expected, f.geometry().toWKT());
        assert.equal(expected, f.toWKT());
    });

    it('should output WKB', function () {
        var feature = {
                type: "Feature",
                properties: {},
                geometry: {
                    type: 'Polygon',
                    coordinates: [[[1,1],[1,2],[2,2],[2,1],[1,1]]]
                }
            };
        var expected = new Buffer('01030000000100000005000000000000000000f03f000000000000f03f000000000000f03f0000000000000040000000000000004000000000000000400000000000000040000000000000f03f000000000000f03f000000000000f03f', 'hex');
        var ds = new mapnik.Datasource({type:'csv', 'inline': "geojson\n'" + JSON.stringify(feature.geometry) + "'"});
        var f = ds.featureset().next();
        assert.deepEqual(expected, f.geometry().toWKB());
        assert.deepEqual(expected, f.toWKB());
    });
});
