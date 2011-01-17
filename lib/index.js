var _settings = require('./settings');
var path = require('path');
var assert = require('assert');

var mapnik;
var _mapnik = './_mapnik.node';


// standard build, pull unversioned _mapnik.node
if (path.existsSync(path.join(__dirname, _mapnik))) {
    mapnik = require(_mapnik);
}
//_mapnik.node in versioned subfolder based on node version
// it was compiled against (only currently used for packaging)
else {
    var version = process.versions.node.split('-')[0];
    _mapnik = './' + version + '/_mapnik.node';
    mapnik = require(_mapnik);
}


/* assert ABI compatibility */
// otherwise register_fonts will throw with:
// "TypeError: first argument must be a path to a directory of fonts"
assert.ok(mapnik.versions.node === process.versions.node, 'The running node version "' + process.versions.node + '" does not match the node version that node-mapnik was compiled against: "' + mapnik.versions.node + '"');

function warning(value, what)
{
    var msg = 'Warning: node-mapnik initialization failed\n';
    msg += 'in "' + __dirname + '/settings.js" ';
    msg += what + '\n';
    msg += 'Auto-loading of ';
    if (value == 'fonts')
        msg += 'DejaVu fonts';
    else if (value == 'plugins')
        msg += 'Datasource input plugins';
    msg += ' will not be available.\n';
    msg += 'See the docs/FAQ on ideas for how to fix this issue.';
    console.log(msg);
}

// hack to force libmapnik symbols to be loaded with RTLD_NOW|RTLD_GLOBAL
// so that datasource plugins will have access to symbols since node.cc (process.dlopen)
// uses RTLD_LAZY in its call to dlopen and has no mechanism to set the RTLD_NOW flag.
// Not needed on darwin because mapnik input plugins are directly linked to libmapnik
if (process.platform === 'linux') {
    var mapnik_node_path = path.join(__dirname, _mapnik);
    if (!path.existsSync(mapnik_node_path))
        console.log(mapnik_node + ' does not exist, loading plugins will fail on linux');

    var loaded = mapnik.make_mapnik_symbols_visible(mapnik_node_path);

    if (!loaded)
        console.log('Warning, attempt to pre-load mapnik symbols did not work, see FAQ for potential solutions');
}

if (!_settings.paths.fonts) {
    warning('fonts', 'the fonts path value is missing');
} else if (!path.existsSync(_settings.paths.fonts)) {
    warning('fonts', 'the fonts directory: "' + _settings.paths.fonts + '" does not exist');
} else {
    // TODO - warn if no fonts are successfully registered
    mapnik.register_fonts(_settings.paths.fonts);
}

if (!_settings.paths.input_plugins) {
    warning('plugins', 'the datasource plugins (input) path value is missing');
} else if (!path.existsSync(_settings.paths.input_plugins)) {
    warning('plugins', 'the input_plugins directory: "' + _settings.paths.input_plugins + '" does not exist');
} else {
    mapnik.register_datasources(_settings.paths.input_plugins);
}


exports.register_system_fonts = function() {
    var num_faces = mapnik.fonts().length;
    var dirs = [];

    // node process.version is based on tools/wafadmin/Utils.py
    // which pulls from python's sys.platform (uname)
    // this info taken from Tools/ccroot.py
    /*
    platforms = {
    '__linux__'   : 'linux',
    '__GNU__'     : 'hurd',
    '__FreeBSD__' : 'freebsd',
    '__NetBSD__'  : 'netbsd',
    '__OpenBSD__' : 'openbsd',
    '__sun'       : 'sunos',
    '__hpux'      : 'hpux',
    '__sgi'       : 'irix',
    '_AIX'        : 'aix',
    '__CYGWIN__'  : 'cygwin',
    '__MSYS__'    : 'msys',
    '_UWIN'       : 'uwin',
    '_WIN64'      : 'win32',
    '_WIN32'      : 'win32'
    }
    */

    // linux, bsd systems or solaris/opensolaris
    if (process.platform == 'linux' ||
       (process.platform.indexOf('bsd') != -1) ||
       process.platform.indexOf('bsd') == 'sunos') {
        dirs.push('/usr/share/fonts/truetype/');
        dirs.push('/usr/local/share/fonts/truetype/');
        // https://github.com/mapnik/node-mapnik/issues/issue/3
        //dirs.push(path.resolve('~/.fonts'));
    }

    // macs
    else if (process.platform == 'darwin') {
        dirs.push('/Library/Fonts');
        dirs.push('/System/Library/Fonts');
    }

    else if (process.platform == 'win32') {
       dirs.push('C:\Windows\Fonts');
    }

    // TODO mysys and cygwin

    var found = [];
    for (idx in dirs) {
        var p = dirs[idx];
        if (path.existsSync(p)) {
                //console.log('looking for: ' + p);
                if (mapnik.register_fonts(p, {recurse: true}))
                    found.push(p);
        } else {
            //console.log('does not exist: ' + p);
            dirs.splice(dirs.indexOf(p), 1);
        }
    }

    if (mapnik.fonts().length == num_faces) {
       //console.log('failed to register any new fonts, searched in: [' + dirs + ']');
       return false;
    }
    else {
       //console.log('registered system fonts in: [' + found + ']');
       return true;
    }
};

exports.register_system_fonts();

// push all C++ symbols into js module
for (var k in mapnik) { exports[k] = mapnik[k]; }

// make settings available
exports['settings'] = _settings;

