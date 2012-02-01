var mapnik = require('mapnik');
var assert = require('assert');
var fs = require('fs');
var path = require('path');


exports['test grid view getPixel'] = function(beforeExit) {
    var grid = new mapnik.Grid(256, 256);
    var view = grid.view(0,0,256,256);
    assert.equal(view.isSolid(),true);
    
    var pixel = view.getPixel(0,0);
    assert.equal(pixel,0);

    var map = new mapnik.Map(256, 256);
    map.loadSync('./examples/stylesheet.xml');
    map.zoomAll();
    var options = {'layer': 0,
                   'fields': ['NAME']
                  };
    var grid = new mapnik.Grid(map.width, map.height, {key: '__id__'});
    map.render(grid, options, function(err, grid) {
    
        var view = grid.view(0,0,256,256);
        assert.equal(view.isSolid(),false);
        // hit alaska (USA is id 207)
        assert.equal(view.getPixel(25,100),207);
    })

};
