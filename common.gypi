{
    'target_defaults': {
        'default_configuration': 'Release',
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
                    'MACOSX_DEPLOYMENT_TARGET':'10.7',
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
                    'MACOSX_DEPLOYMENT_TARGET':'10.7',
                    'OTHER_CPLUSPLUSFLAGS!': [
                        '-Os',
                        '-O2'
                    ],
                    'OTHER_CPLUSPLUSFLAGS': [
                        
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