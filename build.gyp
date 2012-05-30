
{
  'variables': {
      'node_mapnik_sources': [
          "src/node_mapnik.cpp",
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
          "src/mapnik_expression.cpp"
          #"src/mapnik_query.cpp"
      ],
      'node_root': '/opt/node-v6.1',
      'node_root_win': 'c:\\node',
      'deps_root_win': 'c:\\dev2'
  },
  'targets': [
    {
      'target_name': '_mapnik',
      'product_name': '_mapnik',
      'type': 'loadable_module',
      'product_prefix': '',
      'product_extension':'node',
      'sources': [
        '<@(node_mapnik_sources)',
      ],
      'defines': [
        'PLATFORM="<(OS)"',
        'HAVE_JPEG',
        'MAPNIK_THREADSAFE',
        'HAVE_LIBXML2',
        'LIBTOOL_SUPPORTS_ADVISE',
      ],
      'conditions': [
        [ 'OS=="mac"', {
          'libraries': [
            '-lmapnik',
            '-undefined dynamic_lookup'
          ],
          'include_dirs': [
             'src/',
             '<@(node_root)/include/node',
             '<@(node_root)/include',
             '/opt/boost-48/include',
             '/usr/local/Cellar/icu4c/4.8.1.1/include',
             '/usr/X11/include/freetype2',
             '/usr/X11/include',
          ],
          'defines': [
            #'HAVE_CAIRO',
            '_LARGEFILE_SOURCE',
            '_FILE_OFFSET_BITS=64',
          ],
        }],
        [ 'OS=="win"', {
          'defines': [
            'HAVE_CAIRO',
            'PLATFORM="win32"',
            '_LARGEFILE_SOURCE',
            '_FILE_OFFSET_BITS=64',
            '_WINDOWS',
            '__WINDOWS__', # ltdl
            'BUILDING_NODE_EXTENSION'
          ],
          'libraries': [ 
              'mapnik.lib',
              'node.lib',
              'icuuc.lib',
              'libboost_regex-vc100-mt-1_49.lib',
          ],
          'include_dirs': [
             'c:\\mapnik-2.0\\include',
             '<@(deps_root_win)\\freetype',
             '<@(deps_root_win)\\freetype\\include',
             '<@(deps_root_win)\\cairo', # for actual cairo_version.h
             '<@(deps_root_win)\\cairo\\src',
             '<@(deps_root_win)\\cairomm',
             '<@(deps_root_win)\\libsigc++',
             '<@(deps_root_win)\\boost-49-vc100\\include\\boost-1_49',
             '<@(node_root_win)\\deps\\v8\\include',
             '<@(node_root_win)\\src',
             '<@(node_root_win)\\deps\\uv\\include',
             '<@(deps_root_win)\\proj\\src',
             '<@(deps_root_win)\\icu\\include',
             '<@(deps_root_win)\\mapnik-packaging\\windows\\ltdl',
          ],
          'msvs_settings': {
            'VCLinkerTool': {
              'AdditionalOptions': [
                # https://github.com/mapnik/node-mapnik/issues/74
                '/FORCE:MULTIPLE'
              ],
              'AdditionalLibraryDirectories': [
                '<@(node_root_win)\\Release\\lib',
                '<@(node_root_win)\\Release',
                '<@(deps_root_win)\\mapnik-packaging\\windows\\build\\src\\msvc-10.0\\release\\threading-multi',
                '<@(deps_root_win)\\boost-49-vc100\\lib',
                '<@(deps_root_win)\\icu\\lib',
              ],
            },
          },
        },
      ],
      ]
    },
  ],
}