MODULE_NAME := $(shell node -e "console.log(require('./package.json').binary.module_name)")

default: release

deps/geometry/include/mapbox/geometry.hpp:
	git submodule update --init

node_modules:
	npm install --ignore-scripts --clang

mason_packages/.link/bin/mapnik-config:
	./install_mason.sh

pre_build_check:
	mapnik-config -v |>/dev/null

release_base: pre_build_check deps/geometry/include/mapbox/geometry.hpp node_modules
	V=1 ./node_modules/.bin/node-pre-gyp configure build --loglevel=error --clang
	@echo "run 'make clean' for full rebuild"

debug_base: pre_build_check deps/geometry/include/mapbox/geometry.hpp node_modules
	V=1 ./node_modules/.bin/node-pre-gyp configure build --loglevel=error --debug --clang
	@echo "run 'make clean' for full rebuild"

release: mason_packages/.link/bin/mapnik-config
	PATH="./mason_packages/.link/bin/:${PATH}" $(MAKE) release_base

debug: mason_packages/.link/bin/mapnik-config
	PATH="./mason_packages/.link/bin/:${PATH}" $(MAKE) debug_base

coverage:
	./scripts/coverage.sh

clean:
	rm -rf lib/binding
	rm -rf build
	rm -rf mason
	find test/ -name *actual* -exec rm {} \;
	echo "run make distclean to also remove mason_packages and node_modules"

distclean: clean
	rm -rf node_modules
	rm -rf mason_packages

xcode: node_modules
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
