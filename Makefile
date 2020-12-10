MODULE_NAME := $(shell node -e "console.log(require('./package.json').binary.module_name)")

SSE_MATH ?= true

default: release

ifneq (,$(findstring clang,$(CXX)))
    PROFILING_FLAG += -gline-tables-only
else
    PROFILING_FLAG += -g
endif

deps/geometry/include/mapbox/geometry.hpp:
	git submodule update --init

./node_modules/.bin/node-pre-gyp:
	npm install --ignore-scripts --clang

mason_packages/.link/bin/mapnik-config:
	./scripts/install_deps.sh

pre_build_check:
	@node -e "console.log('\033[94mNOTICE: to build from source you need mapnik >=',require('./package.json').mapnik_version,'\033[0m');"
	@echo "Looking for mapnik-config on your PATH..."
	mapnik-config -v

release_base: pre_build_check deps/geometry/include/mapbox/geometry.hpp ./node_modules/.bin/node-pre-gyp
	V=1 CXXFLAGS="-fno-omit-frame-pointer $(PROFILING_FLAG)" ./node_modules/.bin/node-pre-gyp configure build --ENABLE_GLIBC_WORKAROUND=true --enable_sse=$(SSE_MATH) --loglevel=error --clang
	@echo "run 'make clean' for full rebuild"

debug_base: pre_build_check deps/geometry/include/mapbox/geometry.hpp ./node_modules/.bin/node-pre-gyp
	V=1 ./node_modules/.bin/node-pre-gyp configure build --ENABLE_GLIBC_WORKAROUND=true --enable_sse=$(SSE_MATH) --loglevel=error --debug --clang
	@echo "run 'make clean' for full rebuild"

release: mason_packages/.link/bin/mapnik-config
	CXXFLAGS="-D_GLIBCXX_USE_CXX11_ABI=0" PATH="./mason_packages/.link/bin/:${PATH}" $(MAKE) release_base

debug: mason_packages/.link/bin/mapnik-config
	CXXFLAGS="-D_GLIBCXX_USE_CXX11_ABI=0" PATH="./mason_packages/.link/bin/:${PATH}" $(MAKE) debug_base

coverage:
	./scripts/coverage.sh

tidy:
	./scripts/clang-tidy.sh

format:
	./scripts/clang-format.sh

sanitize:
	./scripts/sanitize.sh

clean:
	rm -rf lib/binding
	rm -rf build
	rm -rf ./.mason
	# remove remains from running 'make coverage'
	rm -f *.profraw
	rm -f *.profdata
	find test/ -name *actual* -exec rm {} \;
	echo "run make distclean to also remove mason_packages and node_modules"

distclean: clean
	rm -rf ./.toolchain
	rm -rf node_modules
	rm -rf mason_packages
	# remove remains from running './scripts/setup.sh'
	rm -rf .mason
	rm -rf .toolchain
	rm -f local.env

xcode: ./node_modules/.bin/node-pre-gyp
	./node_modules/.bin/node-pre-gyp configure -- -f xcode

	@# If you need more targets, e.g. to run other npm scripts, duplicate the last line and change NPM_ARGUMENT
	SCHEME_NAME="$(MODULE_NAME)" SCHEME_TYPE=library BLUEPRINT_NAME=$(MODULE_NAME) BUILDABLE_NAME=$(MODULE_NAME).node scripts/create_scheme.sh
	SCHEME_NAME="npm test" SCHEME_TYPE=node BLUEPRINT_NAME=$(MODULE_NAME) BUILDABLE_NAME=$(MODULE_NAME).node NODE_ARGUMENT="`npm bin tape`/tape test/*.test.js" scripts/create_scheme.sh

	open build/binding.xcodeproj

docs:
	npm run docs

test:
	npm test

check: test

testpack:
	rm -f ./*tgz
	npm pack
	tar -ztvf *tgz
	rm -f ./*tgz

.PHONY: test docs
