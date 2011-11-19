var crypto = require('crypto');
var fs = require('fs');

exports.md5File = function(file) {
    var data = fs.readFileSync(file, 'binary');
    return crypto.createHash('md5').update(data).digest('hex');
};

exports.md5 = function(data) {
    return crypto.createHash('md5').update(data).digest('hex');
};

exports.filename = function(suffix) {
    return 'test/tmp/file_' + (Math.random() * 1e16).toFixed() + '.' + (suffix || 'png');
};
