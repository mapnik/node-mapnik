{
  'conditions': [
      ['OS=="win"', {
        'variables': {
          'copy_command%': 'copy',
          'bin_name':'call'
        },
      },{
        'variables': {
          'copy_command%': 'cp',
          'bin_name':'node'
        },
      }]
  ],
  'target_defaults': {
      'default_configuration': 'Release',
      'configurations': {
          'Debug': {
              'cflags_cc!': ['-O3', '-DNDEBUG'],
              'xcode_settings': {
                'OTHER_CPLUSPLUSFLAGS!':['-O3', '-DNDEBUG']
              },
              'msvs_settings': {
                 'VCCLCompilerTool': {
                     'ExceptionHandling': 1,
                     'RuntimeTypeInfo':'true',
                     'RuntimeLibrary': '3'  # /MDd
                 }
              }
          },
          'Release': {

          }
      },
      'include_dirs': [
          './node_modules/mapnik-vector-tile/src/',
          './src'
      ],
      'conditions': [
        ['OS=="win"', {
            'include_dirs':[
              '<!@(mapnik-config --includes)',
              '<!@(mapnik-config --dep-includes)'
              ],
            'defines': ['<!@(mapnik-config --defines)'],
            'libraries': [
              '<!@(mapnik-config --libs)',
              '<!@(mapnik-config --dep-libs)'],
            'msvs_disabled_warnings': [ 4244,4005,4506,4345,4804 ],
            'msvs_settings': {
            'VCCLCompilerTool': {
              # note: not respected, need to change in C:\Users\mapnik\.node-gyp\0.10.3\common.gypi
              'ExceptionHandling': 1,
              'RuntimeTypeInfo':'true',
              'RuntimeLibrary': '2'  # /MD
            },
            'VCLinkerTool': {
              'AdditionalOptions': [
                # https://github.com/mapnik/node-mapnik/issues/74
                '/FORCE:MULTIPLE'
              ],
              'AdditionalLibraryDirectories': [
                 #http://stackoverflow.com/questions/757418/should-i-compile-with-md-or-mt
                 '<!@(mapnik-config --ldflags)'
              ],
            },
          }
        }],
        ['OS=="linux" or OS=="freebsd" or OS=="openbsd" or OS=="netbsd" or OS=="mac"', {
          'cflags_cc!': ['-fno-rtti', '-fno-exceptions'],
          'cflags_cc' : [
              '<!@(mapnik-config --cflags)',
              '<!@(pkg-config protobuf --cflags)'
          ],
          'libraries':[
            '<!@(mapnik-config --libs)', # will bring in -lmapnik and the -L to point to it
            '<!@(pkg-config protobuf --libs-only-L)',
            '-lprotobuf-lite'
          ]
        }]
      ]
  },
  'targets': [
    {
      'target_name': '_mapnik',
      'sources': ["src/node_mapnik.cpp",
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
                   "src/mapnik_cairo_surface.cpp",
                   "src/mapnik_vector_tile.cpp",
                   "node_modules/mapnik-vector-tile/src/vector_tile.pb.cc"
      ],
      # this has to be per target to correctly
      # override node-gyp defaults
      'xcode_settings': {
        'OTHER_CPLUSPLUSFLAGS':[
           '<!@(mapnik-config --cflags)'
        ],
        'GCC_ENABLE_CPP_RTTI': 'YES',
        'GCC_ENABLE_CPP_EXCEPTIONS': 'YES'
      }
    },
    {
      'target_name': 'action_after_build',
      'type': 'none',
      'dependencies': [ '_mapnik' ],
'actions': [
        {
          'action_name': 'generate_setting',
          'inputs': [
            'gen_settings.py'
          ],
          'outputs': [
            'lib/mapnik_settings.js'
          ],
          'action': ['python', 'gen_settings.py']
        },
        {
          'action_name': 'move_node_module',
          'inputs': [
            '<@(PRODUCT_DIR)/_mapnik.node'
          ],
          'outputs': [
            'lib/_mapnik.node'
          ],
          'action': ['<@(copy_command)', '<@(PRODUCT_DIR)/_mapnik.node', 'lib/_mapnik.node']
        }
      ]
    }
  ]
}