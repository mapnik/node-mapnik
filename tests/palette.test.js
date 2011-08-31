var mapnik = require('mapnik');
var assert = require('assert');
var fs = require('fs');
var path = require('path');
var helper = require('./support/helper');

exports['test palette creation'] = function(beforeExit) {
    var pal = new mapnik.Palette('\x01\x02\x03\x04');
    assert.equal('[Palette 1 color #01020304]', pal.toString());

    assert['throws'](function() {
        new mapnik.Palette('\x01\x02\x03');
    }, (/invalid palette length/));

    var pal = new mapnik.Palette('\x01\x02\x03', 'rgb');
    assert.equal('[Palette 1 color #010203]', pal.toString());

    var pal = new mapnik.Palette('\xff\x09\x93\xFF\x01\x02\x03\x04');
    assert.equal('[Palette 2 colors #01020304 #ff0993]', pal.toString());
    assert.equal('\x01\x02\x03\x04\xff\x09\x93\xFF', pal.toBuffer().toString('binary'));
};

exports['test ACT palettes'] = function(beforeExit) {
    var pal = new mapnik.Palette(fs.readFileSync('./tests/support/palettes/palette64.act'), 'act');
    assert.equal('[Palette 64 colors #494746 #c37631 #89827c #d1955c #7397b9 #fc9237 #a09f9c #fbc147 #9bb3ce #b7c9a1 #b5d29c #c4b9aa #cdc4a5 #d5c8a3 #c1d7aa #ccc4b6 #dbd19c #b2c4d5 #eae487 #c9c8c6 #e4db99 #c9dcb5 #dfd3ac #cbd2c2 #d6cdbc #dbd2b6 #ece597 #c0ceda #f7ef86 #d7d3c3 #dfcbc3 #d1d0cd #d3dec1 #d1e2bf #dbd3c4 #f4ef91 #e6d8b6 #d3d3cf #cad5de #ded7c9 #fcf993 #ffff8a #dfdbce #dbd9d7 #dbe7cd #d4dce2 #e4ded3 #ebe3c9 #f4edc3 #e0e2e2 #fdfcae #e9e5dc #f4edda #eeebe4 #fefdc5 #edf4e5 #e7edf2 #f2efe9 #f6ede7 #fefedd #f6f4f0 #f1f5f8 #fbfaf8 #ffffff]', pal.toString());

    var pal = new mapnik.Palette(fs.readFileSync('./tests/support/palettes/palette256.act'), 'act');
    assert.equal('[Palette 256 colors #272727 #3c3c3c #484847 #564b41 #605243 #6a523e #555555 #785941 #5d5d5d #746856 #676767 #956740 #ba712e #787777 #cb752a #c27c3d #b68049 #dc8030 #df9e10 #878685 #e1a214 #928b82 #a88a70 #ea8834 #e7a81d #cb8d55 #909090 #94938c #e18f48 #f68d36 #6f94b7 #e1ab2e #8e959b #c79666 #999897 #ff9238 #ef9447 #a99a88 #f1b32c #919ca6 #a1a09f #f0b04b #f8bc39 #8aa4bf #b3ac8f #d1a67a #e3b857 #a8a8a7 #ffc345 #afaeab #a2adb9 #f9ab69 #afbba4 #c4c48a #b4b2af #dec177 #9ab2cf #d7b491 #a3bebb #b6cd9e #b5d29c #b9c8a2 #f1c969 #c5c79e #bbbab9 #cabdaa #a6bcd1 #cec4a7 #e7cc89 #dad98a #fabd8a #d5c9a3 #c1d7aa #d1d1a5 #d9cf9f #cec5b4 #c5c4c3 #ddd59d #d3c7b5 #b4c6d6 #d1cbb4 #d7d1aa #e1c6ab #d1c7ba #cbc7c2 #dbd0a9 #e8e58a #fee178 #d3cbba #dfd7a3 #d2cfb9 #c9ddb5 #d2cbbe #dfd3aa #e5dd9a #c3cbce #dcceb2 #d7cbba #dbd3b1 #dfd3ae #ceccc6 #d7cbbe #dfc3be #d7cfba #cbcbcb #cbd3c3 #d3cfc0 #e0d8aa #d7cfbe #ebe596 #dbd3b8 #dfd8b0 #c0ceda #f1ee89 #decfbc #d7cfc4 #d7d3c3 #d1d0cd #e7d7b3 #f2ed92 #d2dfc0 #e7c7c3 #dbd3c3 #d1e2bf #fef383 #dad7c3 #dbd3c7 #d3d3cf #e0d3c2 #dfd7c0 #ebe4a8 #dbd7c7 #f7f38f #dfd3c7 #c9d4de #dcdcc5 #dfd7c7 #e7d5c2 #faf78e #d6d5d4 #fffb86 #fbfb8a #d7dfca #dfd7cb #e5ddc0 #ecd6c1 #dad7d2 #fffb8a #e8d0cc #fbfb8e #cfd7de #eae3b8 #e3d7cd #dfdbce #fffb8e #ffff8a #f5efa6 #dae6cc #e3dbcf #edddc3 #dddbd6 #d5dbdf #ffff91 #e3dbd3 #fefc99 #e7dbd2 #eaddcd #ebd7d3 #e3dfd3 #dddddd #d4dee6 #fcdcc0 #e2dfd7 #e7dfd3 #e7dbd7 #ebe4cb #f4eeb8 #e7dfd7 #e3dfdb #ebded5 #e7e3d7 #fefea6 #e1ecd6 #ece5d3 #e7e3db #dee3e5 #ebe3db #efdfdb #efe3d8 #f4efc9 #ebe3df #ebe7db #e6ecdb #f0ecd3 #e5e6e5 #efe7da #efe3df #ebe7df #fefeb8 #ebe7e3 #dfe7ef #efe7e0 #edebde #e8efe0 #e7f3df #ebebe3 #e7ebe8 #f5edd9 #efebe3 #e9efe7 #e3ebf1 #ebebea #f0efe2 #efebe7 #fefdc9 #ecf3e5 #efefe7 #f5f3e1 #f3efe7 #f2efe9 #ffeddf #e9eef4 #efefef #f3efeb #f3f3eb #f0f7eb #fbf7e1 #fefed8 #f3f3ef #f7f3eb #f7f7ea #eef3f7 #f3f7ef #f7f3ef #f3f3f3 #f7f3f3 #f3f3f7 #f7f7ef #fffee3 #f3f7f7 #fcf7ee #f7f7f3 #f7f7f7 #f7fbf4 #f5f7fb #fbf7f6 #fffeef #fbfbf7 #f7fbfb #fbfbfb #fbfffb #fffbfb #fbfbff #fffffb #fbffff #ffffff]', pal.toString());
};

exports['test palette rendering'] = function(beforeExit) {
    var map = new mapnik.Map(600, 400);
    map.fromStringSync(fs.readFileSync('./examples/stylesheet.xml', 'utf8'), { strict: true, base: './examples/' });
    map.zoomAll();

    var pal = new mapnik.Palette('\xff\x00\xff\xff\xff\xff', 'rgb');

    // Test rendering a blank image
    var filename = helper.filename();
    map.renderFileSync(filename, {}, pal);
    var stat = fs.statSync(filename);
    assert.ok(stat.size < 7000);
};
