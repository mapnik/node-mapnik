{
  'includes': [ 'common.gypi' ],
  'variables': {
      'std%':'ansi',
      'runtime_link%':'shared',
      "module_name":"<!(node -e \"console.log(require('./package.json').binary.module_name)\")",
      "module_path":"<!(node -e \"console.log(require('./package.json').binary.module_path)\")",
  },
  'conditions': [
      ['OS=="win"', {
        'variables': {
          'PROTOBUF_INCLUDES%':'C:/dev2/protobuf/vsprojects/include',
          'PROTOBUF_LIBS%':'C:/dev2/protobuf/vsprojects/Release',
          'PROTOBUF_LIBRARY%':'libprotobuf-lite.lib',
        }
      }]
  ],
  'targets': [
    {
      'target_name': '<(module_name)',
      'sources': [
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
          "src/mapnik_expression.cpp",
          "src/mapnik_query.cpp",
          "src/mapnik_cairo_surface.cpp",
          "src/mapnik_vector_tile.cpp",
          "node_modules/mapnik-vector-tile/src/vector_tile.pb.cc"
      ],
      'include_dirs': [
          './node_modules/mapnik-vector-tile/src/',
          './src'
      ],
      'conditions': [
        ['OS=="win"', {
            'include_dirs':[
                '<!@(mapnik-config --includes)',
                '<!@(mapnik-config --dep-includes)',
                '<@(PROTOBUF_INCLUDES)'
              ],
            'defines': ['NOMINMAX','<!@(mapnik-config --defines)'],
            'libraries': [
                '<!@(mapnik-config --libs)',
                '<!@(mapnik-config --dep-libs)',
                '<@(PROTOBUF_LIBRARY)'
            ],
            'msvs_disabled_warnings': [ 4244,4005,4506,4345,4804 ],
            'msvs_settings': {
              'VCCLCompilerTool': {
                # uneeded now that they are in common.gypi VCCLCompilerTool
                #'AdditionalOptions': [
                #  '/GR',
                #  '/MD',
                #  '/EHsc'
                #]
              },
              'VCLinkerTool': {
                'AdditionalOptions': [
                    # https://github.com/mapnik/node-mapnik/issues/74
                    '/FORCE:MULTIPLE'
                ],
                'AdditionalLibraryDirectories': [
                    '<!@(mapnik-config --ldflags)',
                    '<@(PROTOBUF_LIBS)'
                ],
              },
            }
        },{
            'cflags_cc!': ['-fno-rtti', '-fno-exceptions'],
            'cflags_cc' : [
                '<!@(mapnik-config --cflags)',
                '<!@(pkg-config protobuf --cflags)'
            ],
            'libraries':[
                '<!@(mapnik-config --libs)',
                '<!@(mapnik-config --ldflags)',
                '<!@(pkg-config protobuf --libs-only-L)',
                '-lprotobuf-lite'
            ],
            'conditions': [
              ['runtime_link == "static"', {
                  'libraries': [
                    '<!@(mapnik-config --dep-libs)'
                   ]
              }]
            ],
            'xcode_settings': {
              'OTHER_CPLUSPLUSFLAGS':[
                  '<!@(mapnik-config --cflags)'
              ],
              'OTHER_CFLAGS':[
                  '<!@(mapnik-config --cflags)',
                  '<!@(pkg-config protobuf --cflags)'
              ],
              'GCC_ENABLE_CPP_RTTI': 'YES',
              'GCC_ENABLE_CPP_EXCEPTIONS': 'YES'
            }
        },
        ],
        ['std == "c++11"', {
            'cflags_cc' : [
                '-std=c++11',
            ],
            'xcode_settings': {
              'OTHER_CPLUSPLUSFLAGS':['-std=c++11','-stdlib=libc++'],
              'OTHER_LDFLAGS':['-stdlib=libc++'],
              'CLANG_CXX_LANGUAGE_STANDARD':'c++11',
              'MACOSX_DEPLOYMENT_TARGET':'10.7'
            }
        }]
      ]
    },
    {
      'target_name': 'action_after_build',
      'type': 'none',
      'dependencies': [ '<(module_name)' ],
      'actions': [
          {
            'action_name': 'generate_setting',
            'inputs': [
              'gen_settings.py'
            ],
            'outputs': [
              'lib/binding/mapnik_settings.js'
            ],
            'action': ['python', 'gen_settings.py']
          },
      ],
      'copies': [
          {
            'files': [ '<(PRODUCT_DIR)/<(module_name).node' ],
            'destination': '<(module_path)'
          }
      ]
    }
  ]
}