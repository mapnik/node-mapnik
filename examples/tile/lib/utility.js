var url = require('url')

module.exports.parseXYZ = function(req, TMS_SCHEME, callback) {
  
    matches = req.url.match(/(\d+)\/(\d+)\/(\d+)/g);
    if (matches && matches.length == 3) {
        try {
            var x = parseInt(matches[1]);
            var y = parseInt(matches[2]);
            var z = parseInt(matches[0]);
            if (TMS_SCHEME)
                y = (Math.pow(2,z)-1) - y;
            callback(null,
               { z: z,
                 x: x,
                 y: y
               });
        } catch (err) {
            callback(err,null);
        }
    } else {
          var query = url.parse(req.url, true).query;
          if (query &&
                query.x !== undefined &&
                query.y !== undefined &&
                query.z !== undefined) {
             try {
             callback(null,
               { z: parseInt(query.z),
                 x: parseInt(query.x),
                 y: parseInt(query.y)
               }
             );
             } catch (err) {
                 callback(err,null);
             }
          } else {
              callback("no x,y,z provided",null);
          }
    }
}