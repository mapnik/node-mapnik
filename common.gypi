{
    'variables': {
      "prefers_libcpp":"<!(python -c \"import os;import platform;u=platform.uname();print((u[0] == 'Darwin' and int(u[2][0:2]) >= 13) and '-stdlib=libstdc++' not in os.environ.get('CXXFLAGS','') and '-mmacosx-version-min' not in os.environ.get('CXXFLAGS',''))\")"
    },
    'target_defaults': {
        'default_configuration': 'Release',
        'conditions': [
            [ '"<(prefers_libcpp)"=="True"', {
                'xcode_settings': {
                  'MACOSX_DEPLOYMENT_TARGET':'10.9'
                }
              }
            ]
        ],
        'configurations': {
            'Debug': {
                'defines!': [
                    'NDEBUG'
                ],
                'cflags_cc!': [
                    '-O3',
                    '-Os',
                    '-DNDEBUG'
                ],
                'xcode_settings': {
                    'OTHER_CPLUSPLUSFLAGS!': [
                        '-O3',
                        '-Os',
                        '-DDEBUG'
                    ],
                    'GCC_OPTIMIZATION_LEVEL': '0',
                    'GCC_GENERATE_DEBUGGING_SYMBOLS': 'YES'
                },
                'msvs_settings': {
                    'VCCLCompilerTool': {
                        'ExceptionHandling': 1, # /EHsc
                        'RuntimeTypeInfo': 'true', # /GR
                        'RuntimeLibrary': '2' # /MD
                    }
                }
		    },
            'Release': {
                'defines': [
                    'NDEBUG'
                ],
                'xcode_settings': {
                    'OTHER_CPLUSPLUSFLAGS!': [
                        '-Os',
                        '-O2'
                    ],
                    'GCC_OPTIMIZATION_LEVEL': '3',
                    'GCC_GENERATE_DEBUGGING_SYMBOLS': 'NO',
                    'DEAD_CODE_STRIPPING': 'YES',
                    'GCC_INLINES_ARE_PRIVATE_EXTERN': 'YES'
                },
                'ldflags': [
                    '-Wl,-s'
                ],
                'msvs_settings': {
                    'VCCLCompilerTool': {
                        'ExceptionHandling': 1, # /EHsc
                        'RuntimeTypeInfo': 'true', # /GR
                        'RuntimeLibrary': '2' # /MD
                    }
                }
            }
        }
    }
}