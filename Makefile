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
	@rm -rf test/tmp
	@mkdir -p test/tmp

ifndef only
test: test-tmp
	@NODE_PATH=./lib:$NODE_PATH ./node_modules/expresso/bin/expresso -I lib test/*.test.js;
else
test: test-tmp
	@NODE_PATH=./lib:$NODE_PATH ./node_modules/expresso/bin/expresso -I lib test/${only}.test.js;
endif

lint:
	./node_modules/.bin/jshint lib/*js --config=jshint.json

.PHONY: test
