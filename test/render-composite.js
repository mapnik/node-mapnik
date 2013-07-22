var mapnik = require('mapnik');
var assert = require('assert');
var fs = require('fs');


describe('mapnik.render ', function() {

    it('should throw with invalid params', function() {
        assert.throws(function() { mapnik.render(); });
        assert.throws(function() { mapnik.render('a'); });
        // almost correct but image is invalid
        assert.throws(function() { mapnik.render(0,0,0,new mapnik.Map(256,256),{},[],{},function() {}); });
        // almost correct but sources array is empty
        assert.throws(function() { mapnik.render(0,0,0,new mapnik.Map(256,256),new mapnik.Image(256,256),[],{},function() {}); });
        // almost correct but sources array is invalid
        assert.throws(function() { mapnik.render(0,0,0,new mapnik.Map(256,256),new mapnik.Image(256,256),['foo'],{},function() {}); });
    });

    it('should render blank image', function(done) {
        var sources = [ {'name':'hello','image':new mapnik.Image(256,256) } ]
        mapnik.render(0,0,0,new mapnik.Map(256,256),new mapnik.Image(256,256),sources,{},function(err,im) {
            if (err) throw err;
            assert.ok(im instanceof mapnik.Image);
            assert.equal(im.width(),256);
            assert.equal(im.height(),256);
            assert.equal(im.painted(),false);
            done();
        });
    });

    it('should render blank transparent image', function(done) {
        var sources = [ {'name':'hello','image':new mapnik.Image(256,256) } ]
        mapnik.render(0,0,0,new mapnik.Map(256,256),new mapnik.Image(256,256),sources,{},function(err,im) {
            if (err) throw err;
            assert.ok(im instanceof mapnik.Image);
            assert.equal(im.width(),256);
            assert.equal(im.height(),256);
            assert.equal(im.painted(),false); // true?
            var view = im.view(0,0,256,256);
            assert.equal(view.isSolid(),true);
            assert.equal(view.getPixel(0,0).toString(),new mapnik.Color(0,0,0,0).toString());
            done();
        });
    });

    it('should render semi transparent green image', function(done) {
        var mask = new mapnik.Image(256,256);
        var semi_green = new mapnik.Color('rgba(0,255,0,.5)');
        mask.background = semi_green;
        mask.premultiply();
        var sources = [ {'name':'hello','image':mask } ]
        var map = new mapnik.Map(256,256);
        map.add_layer(new mapnik.Layer('hello'));
        mapnik.render(0,0,0,map,new mapnik.Image(256,256),sources,{},function(err,im) {
            if (err) throw err;
            assert.ok(im instanceof mapnik.Image);
            assert.equal(im.width(),256);
            assert.equal(im.height(),256);
            assert.equal(im.painted(),false); // true?
            var view = im.view(0,0,256,256);
            assert.equal(view.isSolid(),true);
            assert.equal(view.getPixel(0,0).toString(),semi_green);
            done();
        });
    });

    it('should allow a render-time sandwich of vector tiles and raster images', function(done) {
        // first we create a vector tile on the fly
        var vtile_global = new mapnik.VectorTile(0,0,0);
        var map = new mapnik.Map(256,256,'+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +wktext +no_defs +over');
        map.extent = [-20037508.34, -20037508.34, 20037508.34, 20037508.34];
        var global = new mapnik.Layer('global',map.srs)
        var options = {
            type: 'shape',
            file: './test/data/world_merc.shp',
            encoding: 'iso-8859-1'
        };
        global.datasource = new mapnik.Datasource(options);
        map.add_layer(global);
        map.render(vtile_global,{},function(err, vtile_global) {
            if (err) throw err;
            // ensure the vtile contains one layer named 'global'
            assert.deepEqual(vtile_global.names(),['global']);

            // now load a vector tile for some deeper zoom level
            // in this case we grab a tile for japan from the tests
            var japan_vtile = new mapnik.VectorTile(5,28,12);
            japan_vtile.setData(fs.readFileSync("./test/data/vector_tile/tile3.vector.pbf"));
            // ensure the vtile contains one layer named 'world'
            assert.deepEqual(japan_vtile.names(),['world']);

            // now load up a raster image to composite into the final rendered image
            // 128 is used here just for testing purposed - you will want to stick to 256 px images
            var raster = new mapnik.Image(128,128);
            // semi transparent blue
            raster.background = new mapnik.Color('rgba(0,0,255,.5)');
            // image has alpha so it needs to be premultiplied before passing into renderer
            raster.premultiply();
            
            // okay, sweet, now render these various sources into and image
            // NOTE: order of sources does not matter, what matters is the order
            // of the layers in the mapnik.Map (which must match sources by name)
            // EXCEPT if multiple sources provide data for the same layer name - in this
            // case they are currently both rendered in the order of the sources array
            var sources = [vtile_global,{'name':'raster','image':raster},japan_vtile];
            var opts = {scale:1.0,buffer_size:256};
            var map_composite = new mapnik.Map(256,256);
            map_composite.loadSync('./test/data/render-composite/composite.xml');
            var z = 0;
            var x = 0;
            var y = 0;
            mapnik.render(z,x,y,map_composite,new mapnik.Image(256,256),sources,opts,function(err,im) {
                if (err) throw err;
                im.save('./test/data/render-composite/composite-actual.png');
                assert.equal(fs.readFileSync('./test/data/render-composite/composite-actual.png').length,
                             fs.readFileSync('./test/data/render-composite/composite-expected.png').length);
                done();
            });
        });
    });

});