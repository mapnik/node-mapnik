all: mapnik.node

NPROCS:=1
OS:=$(shell uname -s)

ifeq ($(OS),Linux)
	NPROCS:=$(shell grep -c ^processor /proc/cpuinfo)
endif
ifeq ($(OS),Darwin)
	NPROCS:=$(shell sysctl -n hw.ncpu)
endif

install: all
	@node-waf build install

mapnik.node:
	@node-waf build -j $(NPROCS)

clean:
	@node-waf clean distclean

uninstall:
	@node-waf uninstall

test-tmp:
	@rm -rf tests/tmp
	@mkdir -p tests/tmp

ifndef only
test: test-tmp
	@NODE_PATH=./lib:$NODE_PATH ./node_modules/.bin/expresso
else
test: test-tmp
	@NODE_PATH=./lib:$NODE_PATH ./node_modules/.bin/expresso test/${only}.test.js
endif

fix:
	@fixjsstyle lib/*js bin/*js test/*js examples/*/*.js examples/*/*/*.js

lint:
	@./node_modules/.bin/jshint lib/*js bin/*js test/*js examples/*/*.js examples/*/*/*.js


.PHONY: test lint fix
