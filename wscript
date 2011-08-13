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
built = 'build/default/%s' % TARGET_FILE
dest = 'lib/%s' % TARGET_FILE
settings = 'lib/mapnik_settings.js'

# detect this install: http://dbsgeo.com/downloads/#mapnik200
HAS_OSX_FRAMEWORK = False

# this goes into a mapnik_settings.js file beside the C++ _mapnik.node
settings_template = """
module.exports.paths = {
    'fonts': %s,
    'input_plugins': %s
};
"""

# number of parallel compile jobs
jobs=1
if os.environ.has_key('JOBS'):
  jobs = int(os.environ['JOBS'])

def write_mapnik_settings(fonts='undefined',input_plugins='undefined'):
    global settings_template
    if '__dirname' in fonts or '__dirname' in input_plugins:
        settings_template = "var path = require('path');\n" + settings_template
    open(settings,'w').write(settings_template % (fonts,input_plugins))

def ensure_min_mapnik_revision(conf,revision=3055):
    # mapnik-config was basically written for node-mapnik
    # so a variety of kinks mean that we need a very
    # recent version for things to work properly
    # http://trac.mapnik.org/log/trunk/utils/mapnik-config

    #TODO - if we require >=2503 then we can check return type not "Usage" string...
    if popen("%s --libs" % conf.env['MAPNIK_CONFIG']).read().startswith('Usage') \
      or popen("%s --input-plugins" % conf.env['MAPNIK_CONFIG']).read().startswith('Usage') \
      or popen("%s --svn-revision" % conf.env['MAPNIK_CONFIG']).read().startswith('Usage'):
        Utils.pprint('YELLOW', 'mapnik-config version is too old, mapnik > %s is required for auto-configuring build' % revision)
        conf.fatal('please upgrade to mapnik trunk')

    failed = False
    found_ver = None

    try:
        found_ver = int(popen("%s --svn-revision" % conf.env['MAPNIK_CONFIG']).readline().strip())
        if not found_ver >= revision:
            failed = True
            print found_ver,revision
        else:
            Utils.pprint('GREEN', 'Sweet, found viable mapnik svn-revision r%s (via mapnik-config)' % (found_ver))
    except Exception,e:
        print e
        failed = True

    if failed:
        if found_ver:
            msg = 'mapnik-config version is too old, mapnik > r%s is required for auto-configuring build, found only r%s' % (revision,found_ver)
        else:
            msg = 'mapnik-config version is too old, mapnik > r%s is required for auto-configuring build' % revision

        Utils.pprint('YELLOW', msg)
        conf.fatal('please upgrade to mapnik trunk')


def set_options(opt):
    opt.tool_options("compiler_cxx")
    #opt.add_option('-D', '--debug', action='store_true', default=False, dest='debug')

def configure(conf):
    global HAS_OSX_FRAMEWORK

    conf.check_tool("compiler_cxx")
    conf.check_tool("node_addon")
    if sys.platform == 'darwin':
        conf.check_tool('osx')
    settings_dict = {}
    cairo_cxxflags = []
    grid_cxxflags = []

    # future auto-support for mapnik frameworks..
    path_list = environ.get('PATH', '').split(os.pathsep)
    
    # if user is not setting LINKFLAGS (custom compile) detect and auto-configure
    # against the Mapnik.Framework installer
    framework_path = '/Library/Frameworks/Mapnik.framework/Programs'
    if not os.environ.has_key('LINKFLAGS') and os.path.exists('/Library/Frameworks/Mapnik.framework'):
        path_list.append(framework_path)
        HAS_OSX_FRAMEWORK = True
    else:
        if framework_path in path_list:
            path_list.remove(framework_path)

    mapnik_config = conf.find_program('mapnik-config', var='MAPNIK_CONFIG', path_list=path_list)
    if not mapnik_config:
        conf.fatal('\n\nSorry, the "mapnik-config" program was not found.\nOnly Mapnik Trunk (future Mapnik 2.0 release) provides this tool, and therefore node-mapnik requires Mapnik trunk.\n\nSee http://trac.mapnik.org/wiki/Mapnik2 for more info.\n')
        
    # this breaks with git cloned mapnik repos, so skip it
    #ensure_min_mapnik_revision(conf)

    # todo - check return value of popen otherwise we can end up with
    # return of 'Usage: mapnik-config [OPTION]'
    all_ldflags = popen("%s --libs" % mapnik_config).readline().strip().split(' ')

    # only link to libmapnik, which should be in first two flags
    linkflags = []
    if os.environ.has_key('LINKFLAGS'):
        linkflags.extend(os.environ['LINKFLAGS'].split(' '))
    
    # put on the path the first -L to where libmapnik2 should be
    linkflags.extend(all_ldflags[:1])
    
    # add prefix to linkflags if it is unique
    prefix_lib = os.path.join(conf.env['PREFIX'],'lib')
    if not '/usr/local' in prefix_lib:
        linkflags.insert(0,'-L%s' % prefix_lib)

    conf.env.append_value("LINKFLAGS", linkflags)

    # unneeded currently as second item from mapnik-config is -lmapnik2
    conf.env.append_value("LIB_MAPNIK", "mapnik2")

    # TODO - too much potential pollution here, need to limit this upstream
    cxxflags = popen("%s --cflags" % mapnik_config).readline().strip().split(' ')

    if '-lcairo' in all_ldflags or '-DHAVE_CAIRO' in cxxflags:

        if HAS_OSX_FRAMEWORK and os.path.exists('/Library/Frameworks/Mapnik.framework/Headers/cairo'):
            # prep for this specific install of mapnik 1.0: http://dbsgeo.com/downloads/#mapnik200
            cairo_cxxflags.append('-I/Library/Frameworks/Mapnik.framework/Headers/cairomm-1.0')
            cairo_cxxflags.append('-I/Library/Frameworks/Mapnik.framework/Headers/cairo')
            cairo_cxxflags.append('-I/Library/Frameworks/Mapnik.framework/Headers/sigc++-2.0')
            cairo_cxxflags.append('-I/Library/Frameworks/Mapnik.framework/unix/lib/sigc++-2.0/include')
            cairo_cxxflags.append('-I/Library/Frameworks/Mapnik.framework/Headers') #fontconfig
            Utils.pprint('GREEN','Sweet, found cairo library, will attempt to compile with cairo support for pdf/svg output')
    else:
        Utils.pprint('YELLOW','Notice: "mapnik-config --libs" or "mapnik-config --cflags"" is not reporting Cairo support in your mapnik version, so node-mapnik will not be built with Cairo support (pdf/svg output)')


    if HAS_OSX_FRAMEWORK:
        cxxflags.insert(0,'-I/Library/Frameworks/Mapnik.framework/Versions/2.0/unix/include/freetype2')
    
    # if cairo is available
    if cairo_cxxflags:
        cxxflags.append('-DHAVE_CAIRO')
        cxxflags.extend(cairo_cxxflags)
    
    # add prefix to includes if it is unique
    prefix_inc = os.path.join(conf.env['PREFIX'],'include/node')
    if not '/usr/local' in prefix_inc:
        cxxflags.insert(0,'-I%s' % prefix_inc)

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

def build(bld):
    Options.options.jobs = jobs;
    obj = bld.new_task_gen("cxx", "shlib", "node_addon", install_path=None)
    obj.cxxflags = ["-O3", "-g", "-D_FILE_OFFSET_BITS=64", "-D_LARGEFILE_SOURCE"]
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
                   "src/mapnik_projection.cpp",
                   "src/mapnik_layer.cpp",
                   "src/mapnik_datasource.cpp",
                   "src/mapnik_featureset.cpp"
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
