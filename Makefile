MODULE_NAME := $(shell node -e "console.log(require('./package.json').binary.module_name)")

default: release

node_modules:
	# install deps but for now ignore our own install script
	# so that we can run it directly in either debug or release
	npm install --ignore-scripts

release: node_modules
	V=1 ./node_modules/.bin/node-pre-gyp configure build --loglevel=error
	@echo "run 'make clean' for full rebuild"

debug: node_modules
	V=1 ./node_modules/.bin/node-pre-gyp configure build --loglevel=error --debug
	@echo "run 'make clean' for full rebuild"

coverage:
	./scripts/coverage.sh

clean:
	rm -rf lib/binding
	rm -rf build

distclean: clean
	rm -rf node_modules

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

.PHONY: test docs