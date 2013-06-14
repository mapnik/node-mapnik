import os
import sys
from glob import glob
from os import unlink, symlink, popen, uname, environ
from os.path import exists
from shutil import copy2 as copy
from subprocess import call

# node-wafadmin
import Options
import Utils

TARGET = '_mapnik'
TARGET_FILE = '%s.node' % TARGET
built = 'build/Release/%s' % TARGET_FILE
dest = 'lib/%s' % TARGET_FILE
settings = 'lib/mapnik_settings.js'

# this goes into a mapnik_settings.js file beside the C++ _mapnik.node
settings_template = """
module.exports.paths = {
    'fonts': %s,
    'input_plugins': %s
};
"""

def version2num(version_string):
    f_parts = map(int,version_string.split('.'))
    return (f_parts[0]*100000)+(f_parts[1]*100)+f_parts[2]

def write_mapnik_settings(fonts='undefined',input_plugins='undefined'):
    global settings_template
    if '__dirname' in fonts or '__dirname' in input_plugins:
        settings_template = "var path = require('path');\n" + settings_template
    open(settings,'w').write(settings_template % (fonts,input_plugins))

def ensure_min_mapnik_version(conf,min_version='2.1.0'):
    found_version = popen("%s --version" % conf.env['MAPNIK_CONFIG']).readline().strip().replace('-pre','')
    if not found_version:
        Utils.pprint('RED',"Warning: Incompatible libmapnik version found (using mapnik-config --version), this 'node-mapnik' requires 'mapnik %s'" % min_version)
    else:
        found_version_num = version2num(found_version)
        min_version_num = version2num(min_version)
        if found_version_num == min_version_num:
            Utils.pprint('GREEN', 'Sweet, found compatible mapnik version %s (via mapnik-config)' % (found_version))
        else:
            Utils.pprint('RED',"Warning: Incompatible libmapnik version found (using mapnik-config --version), this 'node-mapnik' requires 'mapnik %s'" % min_version)

def warn_about_mapnik_version(conf):
    found_version = popen("%s --version" % conf.env['MAPNIK_CONFIG']).readline().strip().replace('-pre','')
    # NOTE: bug in Mapnik 2.0.0 results in blank version returned
    if found_version:
        found_version_num = version2num(found_version)
        min_version = '2.0.0'
        max_version = '2.2.0'
        min_version_num = version2num(min_version)
        max_version_num = version2num(max_version)
        if found_version_num <= max_version_num and found_version_num >= min_version_num:
            Utils.pprint('GREEN', 'Sweet, found compatible mapnik version %s (via mapnik-config)' % (found_version))
        else:
            Utils.pprint('RED',"Warning: Incompatible libmapnik version found (using mapnik-config --version).\nThis 'node-mapnik' requires mapnik >=%s and <=%s" % (min_version,max_version))


def set_options(opt):
    opt.tool_options("compiler_cxx")
    opt.add_option('--debug',
        action='store_true',
        default=None,
        help='Build in debug mode',
        dest='debug'
    )

def configure(conf):
    o = Options.options
    conf.check_tool("compiler_cxx")
    conf.check_tool("node_addon")
    if sys.platform == 'darwin':
        conf.check_tool('osx')
    settings_dict = {}
    cairo_cxxflags = []
    grid_cxxflags = []

    path_list = environ.get('PATH', '').split(os.pathsep)
    
    mapnik_config = conf.find_program('mapnik-config', var='MAPNIK_CONFIG', path_list=path_list)
    if not mapnik_config:
        conf.fatal('\n\nSorry, the "mapnik-config" program was not found.\nOnly Mapnik >=2.0.0 provides this tool.\n')
        
    #ensure_min_mapnik_version(conf)
    warn_about_mapnik_version(conf)

    # todo - check return value of popen otherwise we can end up with
    # return of 'Usage: mapnik-config [OPTION]'
    all_ldflags = popen("%s --libs" % mapnik_config).readline().strip().split(' ')

    linkflags = ['-lprotobuf-lite']
    linkflags.extend(popen("pkg-config protobuf --libs-only-L" ).read().replace('\n',' ').strip().split(' '))
    if os.environ.has_key('LINKFLAGS'):
        linkflags.extend(os.environ['LINKFLAGS'].split(' '))
    
    # put on the path the first -L to where libmapnik should be and libmapnik itself
    linkflags.extend(all_ldflags)
    
    # add prefix to linkflags if it is unique
    prefix_lib = os.path.join(conf.env['PREFIX'],'lib')
    if not '/usr/local' in prefix_lib:
        linkflags.insert(0,'-L%s' % prefix_lib)

    conf.env.append_value("LINKFLAGS", linkflags)

    # unneeded currently as second item from mapnik-config is -lmapnik
    #conf.env.append_value("LIB_MAPNIK", "mapnik")

    cxxflags = popen("%s --cflags" % mapnik_config).read().replace('\n',' ').strip().split(' ')

    if '-lcairo' in all_ldflags or '-DHAVE_CAIRO' in cxxflags:
        Utils.pprint('GREEN','Sweet, found cairo library, will attempt to compile with cairo support for pdf/svg output')
    else:
        Utils.pprint('YELLOW','Notice: "mapnik-config --libs" or "mapnik-config --cflags" is not reporting Cairo support in your mapnik version, so node-mapnik will not be built with Cairo support (pdf/svg output)')

    
    # if cairo is available
    if cairo_cxxflags:
        cxxflags.append('-DHAVE_CAIRO')
        cxxflags.extend(cairo_cxxflags)
    cxxflags.append('-I../node_modules/mapnik-vector-tile/src/')
    
    # add prefix to includes if it is unique
    prefix_inc = os.path.join(conf.env['PREFIX'],'include/node')
    if not '/usr/local' in prefix_inc:
        cxxflags.insert(0,'-I%s' % prefix_inc)

    cxxflags.extend(popen("pkg-config protobuf --cflags" ).read().replace('\n',' ').strip().split(' '))

    conf.env.append_value("CXXFLAGS_MAPNIK", cxxflags)

    #ldflags = []
    #conf.env.append_value("LDFLAGS", ldflags)

    # settings for fonts and input plugins
    if os.environ.has_key('MAPNIK_INPUT_PLUGINS'):
        settings_dict['input_plugins'] =  os.environ['MAPNIK_INPUT_PLUGINS']
    else:
        settings_dict['input_plugins'] = '\'%s\'' % popen("%s --input-plugins" % mapnik_config).readline().strip()

    if os.environ.has_key('MAPNIK_FONTS'):
        settings_dict['fonts'] =  os.environ['MAPNIK_FONTS']
    else:
        settings_dict['fonts'] = '\'%s\'' % popen("%s --fonts" % mapnik_config).readline().strip()

    write_mapnik_settings(**settings_dict)
    conf.env.DEBUG = o.debug
    if (o.debug):
        if '-O3' in conf.env.CXXFLAGS_MAPNIK:
            conf.env.CXXFLAGS_MAPNIK.remove('-O3')
        conf.env.CXXFLAGS_MAPNIK.insert(0,'-O0')
        conf.env.CXXFLAGS_MAPNIK.append('-g')

def clean(bld):
    pass # to avoid red warning from waf of "nothing to clean"

def build(bld):
    #Options.options.jobs = jobs;
    obj = bld.new_task_gen("cxx", "shlib", "node_addon", install_path=None)
    obj.cxxflags = ["-D_FILE_OFFSET_BITS=64", "-D_LARGEFILE_SOURCE"]
    # uncomment the next line to remove '-undefined dynamic_lookup' 
    # in order to review linker errors (v8, libev/eio references can be ignored)
    #obj.env['LINKFLAGS_MACBUNDLE'] = ['-bundle']
    obj.target = TARGET
    obj.source =  ["src/node_mapnik.cpp",
                   "src/mapnik_map.cpp",
                   "src/mapnik_color.cpp",
                   "src/mapnik_geometry.cpp",
                   "src/mapnik_feature.cpp",
                   "src/mapnik_image.cpp",
                   "src/mapnik_image_view.cpp",
                   "src/mapnik_grid.cpp",
                   "src/mapnik_grid_view.cpp",
                   "src/mapnik_js_datasource.cpp",
                   "src/mapnik_memory_datasource.cpp",
                   "src/mapnik_palette.cpp",
                   "src/mapnik_projection.cpp",
                   "src/mapnik_proj_transform.cpp",
                   "src/mapnik_layer.cpp",
                   "src/mapnik_datasource.cpp",
                   "src/mapnik_featureset.cpp",
                   "src/mapnik_expression.cpp",
                   "src/mapnik_query.cpp",
                   "src/mapnik_vector_tile.cpp",
                   "node_modules/mapnik-vector-tile/src/vector_tile.pb.cc"
                  ]
    obj.uselib = "MAPNIK"
    # install 'mapnik' module
    lib_dir = bld.path.find_dir('./lib')
    bld.install_files('${LIBPATH_NODE}/node/mapnik', lib_dir.ant_glob('**/*'), cwd=lib_dir, relative_trick=True)
    # install command line programs
    bin_dir = bld.path.find_dir('./bin')
    bld.install_files('${PREFIX_NODE}/bin', bin_dir.ant_glob('*'), cwd=bin_dir, relative_trick=True, chmod=0755)


def shutdown():
    if Options.commands['clean']:
        if exists(TARGET): unlink(TARGET)
    if Options.commands['clean']:
        if exists(dest):
            unlink(dest)
    else:
        if exists(built):
            copy(built,dest)
