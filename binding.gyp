{
    'targets': [
        {
            'target_name': 'build-node-mapnik',
            'type': 'none',
            'actions': [
                {
                    'action_name': 'configure',
                    'message': 'configuring node-mapnik...',
                    'inputs': ['CMakeLists.txt'],
                    'outputs': ["build/CMakeCache.txt"],
                    'action': [
                        'cmake', '.', '-B', 'build', '-DCMAKE_BUILD_TYPE=Release', '-DNAPI_VERSION=<(napi_build_version)',
                        '-Dnode_root_dir=<(node_root_dir)', '-Dnode_lib_file=<(node_lib_file)', '-DCMAKE_INSTALL_PREFIX=<(module_path)'
                    ],
                },
                {
                    'action_name': 'build',
                    'message': 'Building node-mapnik...',
                    'inputs': ["build/CMakeCache.txt"],
                    'outputs': ["build/Release/mapnik.node"],
                    'action': ['cmake', '--build', 'build', '--config Release', '--parallel 24', '--target install'],
                }
            ]
        }
    ]
}
