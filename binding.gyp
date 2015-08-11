{
  'includes': [ 'common.gypi' ],
  'variables': {
    'coverage': 'false'
  },
  'targets': [
    {
      'target_name': 'make_vector_tile',
      'hard_dependency': 1,
      'type': 'none',
      'actions': [
        {
          'action_name': 'generate_protoc_files',
          'inputs': [
            './node_modules/mapnik-vector-tile/proto/'
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/vector_tile.pb.cc',
            '<(SHARED_INTERMEDIATE_DIR)/vector_tile.pb.h'
          ],
          'action': [ 'protoc',
            '-I<(RULE_INPUT_PATH)',
            '--cpp_out=<(SHARED_INTERMEDIATE_DIR)/',
            '<(RULE_INPUT_PATH)/vector_tile.proto']
        },
        {
          'action_name': 'generate_setting',
          'inputs': [
            'gen_settings.py'
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/mapnik_settings.js'
          ],
          'action': ['python', 'gen_settings.py', '<(SHARED_INTERMEDIATE_DIR)/mapnik_settings.js']
        }
      ],
      'copies': [
        {
          'files': [ '<(SHARED_INTERMEDIATE_DIR)/mapnik_settings.js' ],
          'destination': '<(module_path)'
        }
      ]
    },
    {
      "target_name": "vector_tile",
      'dependencies': [ 'make_vector_tile' ],
      'hard_dependency': 1,
      "type": "static_library",
      "sources": [
        "<(SHARED_INTERMEDIATE_DIR)/vector_tile.pb.cc"
      ],
      "msvs_disabled_warnings": [
        4267,
        4018
      ],
      'include_dirs': [
        '<(SHARED_INTERMEDIATE_DIR)/'
      ],
      'cflags_cc' : [
          '-D_THREAD_SAFE',
          '<!@(mapnik-config --cflags)', # assume protobuf headers are here
          '-Wno-sign-compare' # to avoid warning from wire_format_lite_inl.h
      ],
      'xcode_settings': {
        'OTHER_CPLUSPLUSFLAGS':[
            '-D_THREAD_SAFE',
            '<!@(mapnik-config --cflags)', # assume protobuf headers are here
            '-Wno-sign-compare' # to avoid warning from wire_format_lite_inl.h
        ],
        'GCC_ENABLE_CPP_RTTI': 'YES',
        'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
        'MACOSX_DEPLOYMENT_TARGET':'10.8',
        'CLANG_CXX_LIBRARY': 'libc++',
        'CLANG_CXX_LANGUAGE_STANDARD':'c++11',
        'GCC_VERSION': 'com.apple.compilers.llvm.clang.1_0'
      },
      'direct_dependent_settings': {
        'include_dirs': [
          '<(SHARED_INTERMEDIATE_DIR)/'
        ],
        'cflags_cc' : [
            '-D_THREAD_SAFE'
        ],
        'xcode_settings': {
          'OTHER_CPLUSPLUSFLAGS':[
             '-D_THREAD_SAFE',
          ],
        },
      },
      'conditions': [
        ['OS=="win"',
          {
            'include_dirs':[
              '<!@(mapnik-config --includes)'
            ],
            'libraries': [
              'libprotobuf-lite.lib'
            ]
          },
          {
            'libraries':[
              '-lprotobuf-lite'
            ],
          }
        ]
      ]
    },
    {
      'target_name': '<(module_name)',
      'dependencies': [ 'vector_tile', 'make_vector_tile' ],
      'sources': [
        "src/mapnik_logger.cpp",
        "src/node_mapnik.cpp",
        "src/blend.cpp",
        "src/mapnik_map.cpp",
        "src/mapnik_color.cpp",
        "src/mapnik_geometry.cpp",
        "src/mapnik_feature.cpp",
        "src/mapnik_image.cpp",
        "src/mapnik_image_view.cpp",
        "src/mapnik_grid.cpp",
        "src/mapnik_grid_view.cpp",
        "src/mapnik_memory_datasource.cpp",
        "src/mapnik_palette.cpp",
        "src/mapnik_projection.cpp",
        "src/mapnik_layer.cpp",
        "src/mapnik_datasource.cpp",
        "src/mapnik_featureset.cpp",
        "src/mapnik_expression.cpp",
        "src/mapnik_cairo_surface.cpp",
        "src/mapnik_vector_tile.cpp",
        "deps/clipper/clipper.cpp"
      ],
      "msvs_disabled_warnings": [
        4267
      ],
      'include_dirs': [
        './deps/clipper/',
        './deps/',
        './node_modules/mapnik-vector-tile/src/',
        './src',
        "<!(node -e \"require('nan')\")"
      ],
      'defines': [
          'MAPNIK_GIT_REVISION="<!@(mapnik-config --git-describe)"',
          'CLIPPER_INTPOINT_IMPL=mapnik::geometry::point<cInt>',
          'CLIPPER_PATH_IMPL=mapnik::geometry::line_string<cInt>',
          'CLIPPER_PATHS_IMPL=mapnik::geometry::multi_line_string<cInt>',
          'CLIPPER_IMPL_INCLUDE=<mapnik/geometry.hpp>'
      ],
      'conditions': [
        ["coverage == 'true'", {
            "cflags_cc": ["--coverage"],
            "xcode_settings": {
                "OTHER_CPLUSPLUSFLAGS":[
                    "--coverage"
                ],
                'OTHER_LDFLAGS':[
                    '--coverage'
                ]
            }
        }],
        ['OS=="win"',
          {
            'include_dirs':[
              '<!@(mapnik-config --includes)',
              '<!@(mapnik-config --dep-includes)'
            ],
            'defines': ['NOMINMAX','<!@(mapnik-config --defines)'],
            'libraries': [
              '<!@(mapnik-config --libs)',
              'mapnik-wkt.lib',
              'mapnik-json.lib',
              '<!@(mapnik-config --dep-libs)',
              'libprotobuf-lite.lib',
            ],
            'msvs_disabled_warnings': [ 4244,4005,4506,4345,4804,4805 ],
            'msvs_settings': {
              'VCLinkerTool': {
                'AdditionalLibraryDirectories': [
                  '<!@(mapnik-config --ldflags)'
                ],
              },
            }
          },
          {
            'cflags_cc!': ['-fno-rtti', '-fno-exceptions'],
            'cflags_cc' : [
              '<!@(mapnik-config --cflags)'
            ],
            'libraries':[
              '<!@(mapnik-config --libs)',
              '-lmapnik-wkt',
              '-lmapnik-json',
              '<!@(mapnik-config --ldflags)',
              '-lprotobuf-lite',
            ],
            'xcode_settings': {
              'OTHER_CPLUSPLUSFLAGS':[
                '<!@(mapnik-config --cflags)',
              ],
              'OTHER_CFLAGS':[
                '<!@(mapnik-config --cflags)'
              ],
              'OTHER_LDFLAGS':[
                '-Wl,-bind_at_load'
              ],
              'GCC_ENABLE_CPP_RTTI': 'YES',
              'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
              'MACOSX_DEPLOYMENT_TARGET':'10.8',
              'CLANG_CXX_LIBRARY': 'libc++',
              'CLANG_CXX_LANGUAGE_STANDARD':'c++11',
              'GCC_VERSION': 'com.apple.compilers.llvm.clang.1_0'
            }
          },
        ]
      ]
    },
    {
      'target_name': 'action_after_build',
      'type': 'none',
      'dependencies': [ '<(module_name)' ],
      'copies': [
        {
          'files': [ '<(PRODUCT_DIR)/<(module_name).node' ],
          'destination': '<(module_path)'
        }
      ]
    }
  ]
}
