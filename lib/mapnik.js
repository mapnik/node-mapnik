var _settings = require('./mapnik_settings');
var path = require('path');
var assert = require('assert');

var _mapnik = './_mapnik.node';
var mapnik = require(_mapnik);

exports = module.exports = mapnik;


/*
  Warning about API compatibility
  If not the same versions register_fonts may throw with:
     "TypeError: first argument must be a path to a directory of fonts"
*/
if (mapnik.versions.node != process.versions.node) {
    console.warn('node-mapnik warning: The running node version "' + process.versions.node + '" does not match the node version that node-mapnik was compiled against: "' + mapnik.versions.node + '", undefined behavior may result');
}

function warning(value, what)
{
    var msg = 'Warning: node-mapnik initialization failed\n';
    msg += 'in "' + __dirname + '/mapnik_setting.js" ';
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

    /*
      node process.version is based on tools/wafadmin/Utils.py
      which pulls from python's sys.platform (uname)
      this info taken from Tools/ccroot.py

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
        dirs.push(path.resolve('~/.fonts'));
    }

    else if (process.platform == 'darwin') {
        dirs.push('/Library/Fonts');
        dirs.push('/System/Library/Fonts');
        if (path.existsSync('/usr/X11R6/lib/X11/fonts/')) {
            // if xcode's X11 is installed
            dirs.push('/usr/X11R6/lib/X11/fonts/');
        }
        dirs.push(path.resolve('~/Library/Fonts/'));
    }

    else if (process.platform == 'win32') {
       dirs.push('C:\Windows\Fonts');
    }

    // TODO mysys and cygwin

    var found = [];
    for (var idx in dirs) {
        var p = dirs[idx];
        if (path.existsSync(p)) {
                if (mapnik.register_fonts(p, {recurse: true}))
                    found.push(p);
        } else {
            dirs.splice(dirs.indexOf(p), 1);
        }
    }

    if (mapnik.fonts().length == num_faces) {
       return false;
    }
    else {
       return true;
    }
};

exports.register_system_fonts();

// make settings available
exports.settings = _settings;

