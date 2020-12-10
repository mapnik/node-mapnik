#!/usr/bin/env python

import sys
import json
import os
import re

# Script to generate compile_commands.json based on Makefile output
# Works by accepting Makefile output from stdin, parsing it, and
# turning into json records. These are then printed to stdout.
# More details on the compile_commands format at:
# https://clang.llvm.org/docs/JSONCompilationDatabase.html
#
# Note: make must be run in verbose mode, e.g. V=1 make or VERBOSE=1 make
#
# Usage with node-cpp-skel:
#
# make | ./scripts/generate_compile_commands.py > build/compile_commands.json

# These work for node-cpp-skel to detect the files being compiled
# They may need to be modified if you adapt this to another tool
matcher = re.compile('^(.*) (.+cpp)\n')
build_dir = os.path.join(os.getcwd(),"build")
TOKEN_DENOTING_COMPILED_FILE='NODE_GYP_MODULE_NAME'

def generate():
    compile_commands = []
    for line in sys.stdin.readlines():
        if TOKEN_DENOTING_COMPILED_FILE in line:
            match = matcher.match(line)
            compile_commands.append({
                "directory": build_dir,
                "command": line.strip(),
                "file": os.path.normpath(os.path.join(build_dir,match.group(2)))
            })
    print(json.dumps(compile_commands,indent=4))

if __name__ == '__main__':
    generate()
