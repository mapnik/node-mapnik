
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
          "src/mapnik_layer.cpp",
          "src/mapnik_datasource.cpp",
          "src/mapnik_featureset.cpp"
      ]
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
      ],
      'conditions': [
        [ 'OS=="mac"', {
          'libraries': [
            '-lmapnik2',
            '-undefined dynamic_lookup'
          ],
          'include_dirs': [
             'src/',
             '/usr/local/include/node',
             '/opt/boost-48/include',
             '/usr/local/Cellar/icu4c/4.8.1.1/include',
             '/usr/X11/include/freetype2',
             '/usr/X11/include',
             '/usr/include',
          ],
          'defines': [
            '_LARGEFILE_SOURCE',
            '_FILE_OFFSET_BITS=64',
          ],
        }],
        [ 'OS=="win"', {
          'defines': [
            'PLATFORM="win32"',
            '_LARGEFILE_SOURCE',
            '_FILE_OFFSET_BITS=64',
            '_WINDOWS',
            '__WINDOWS__', # ltdl
            #'WIN32',
            #'_USRDLL',
            'BUILDING_NODE_EXTENSION'
          ],
          'libraries': [ 
              'mapnik2.lib',
              'node.lib',
              #'v8_base.lib',
              'icuuc.lib',
              'libboost_regex-vc100-mt-1_48.lib',
          ],
          'include_dirs': [
             'c:\\mapnik-2.0\\include',
             'c:\\dev2\\freetype',
             'c:\\dev2\\freetype\\include',
             'c:\\dev2\\boost-vc100\\include\\boost-1_48',
             'c:\\dev2\\node-v0.6.2\\deps\\v8\\include',
             'c:\\dev2\\node-v0.6.2\\src',
             'c:\\dev2\\node-v0.6.2\\deps\\uv\\include',
             'c:\\dev2\\proj\\src',
             'c:\\dev2\\icu\\include',
             'C:\dev2\mapnik-packaging\windows\ltdl',
          ],
          'msvs_settings': {
            'VCLinkerTool': {
              'AdditionalLibraryDirectories': [
                'c:\\dev2\\node-v0.6.2\\Release\\lib',
                'c:\\dev2\\node-v0.6.2\\Release',
                #'C:\\mapnik-2.0\\lib',
                'C:\\dev2\\mapnik-packaging\\windows\\build\\src\\msvc-9.0\\release\\threading-multi',
                'C:\\dev2\\boost-vc100\\lib',
                'C:\\dev2\\icu\\lib',
              ],
            },
          },
        },
      ],
      ]
    },
  ],
}