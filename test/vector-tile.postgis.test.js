"use strict";

var mapnik = require('../');
var assert = require('assert');
var fs = require('fs');
var path = require('path');
var spawn = require('child_process').spawn;

var dbname = 'node-mapnik-tmp-postgis-test-db';

var postgis_registered = false;
var hasPostgisAvailable = false;

describe('mapnik.VectorTile postgis.input', function() {

    before(function(done) {
        // Check that the postgis plugin is available
        mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'postgis.input'));
        postgis_registered = mapnik.datasources().indexOf('postgis') > -1;

        if (postgis_registered) {
            // Check if postgres is available
            spawn('psql', ['--version'])
                .on('error', function(code, signal) {
                    console.warn('psql --version could not be executed.');
                    return done();
                })
                .on('exit', function(code, signal) {
                    if (code !== 0) {
                        console.warn('psql --version returned ' + code);
                        return done();
                    }
                    // Drop the test database if it exists
                    spawn('dropdb', ['--if-exists', dbname])
                        .on('exit', function(code, signal) {
                            if (code !== 0) {
                                console.warn('dropdb --if-exists ' + dbname + ' returned ' + code);
                                return done();
                            }
                            // Create the test database
                            spawn('createdb', ['-T', 'template_postgis', dbname])
                                .on('exit', function(code, signal) {
                                    if (code !== 0) {
                                        console.warn('createdb -T template_postgis ' + dbname + 'returned ' + code);
                                        return done();
                                    }
                                    hasPostgisAvailable = true;
                                    return done();
                                });
                        })
                });
        } else {
            console.warn('postgis input datasource not registered.');
            return done();
        }
    });

    it('passes variables to replace tokens in query', function(done) {
        if (!hasPostgisAvailable) {
            this.skip('postgis not available');
        }

        spawn('psql', ['-q', '-f', './test/data/postgis-create-db-and-tables.sql', dbname])
            .on('exit', function(code, signal) {
                assert.equal(code, 0, 'could not load data in postgis');
                        var map = new mapnik.Map(256, 256);
                map.loadSync('./test/data/postgis_datasource_tokens_query.xml');

                var opts = {};
                opts.variables = { "fieldid": 2 };
                map.render(new mapnik.VectorTile(0, 0, 0), opts, function(err, vtile) {
                    if (err) throw err;
                    assert(!vtile.empty());
                    var out = JSON.parse(vtile.toGeoJSON(0));
                    assert.equal(out.type,'FeatureCollection');
                    assert.equal(out.features.length,1);
                    assert.equal(out.features[0].properties.gid, 2);
                    var coords = out.features[0].geometry.coordinates;
                    var expected_coords = [-2.0, 2.0]; // From DB, GeomFromEWKT('SRID=4326;POINT(-2 2)')
                    assert.ok(Math.abs(coords[0] - expected_coords[0]) < 0.3);
                    assert.ok(Math.abs(coords[1] - expected_coords[1]) < 0.3);
                    done();
                });
            });

    });
});
