MODULE_NAME := $(shell node -e "console.log(require('./package.json').binary.module_name)")

default: release

deps/geometry/include/mapbox/geometry.hpp:
	git submodule update --init

mason_packages/.link/bin/mapnik-config: deps/geometry/include/mapbox/geometry.hpp
	./install_mason.sh

node_modules: mason_packages/.link/bin/mapnik-config
	# install deps but for now ignore our own install script
	# so that we can run it directly in either debug or release
	npm install --ignore-scripts --clang

release: node_modules
	PATH="./mason_packages/.link/bin/:${PATH}" && V=1 ./node_modules/.bin/node-pre-gyp configure build --loglevel=error --clang
	@echo "run 'make clean' for full rebuild"

debug: node_modules
	PATH="./mason_packages/.link/bin/:${PATH}" && V=1 ./node_modules/.bin/node-pre-gyp configure build --loglevel=error --debug --clang
	@echo "run 'make clean' for full rebuild"

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

testpack:
	rm -f ./*tgz
	npm pack
	tar -ztvf *tgz
	rm -f ./*tgz

.PHONY: test docs
