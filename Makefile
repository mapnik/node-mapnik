#http://www.gnu.org/prep/standards/html_node/Standard-Targets.html#Standard-Targets

all: mapnik.node

OS:=$(shell uname -s)

ifeq ($(NPROCS),)
	NPROCS:=1
	ifeq ($(OS),Linux)
		NPROCS:=$(shell grep -c ^processor /proc/cpuinfo)
	endif
	ifeq ($(OS),Darwin)
		NPROCS:=$(shell sysctl -n hw.ncpu)
	endif
endif

mapnik.node:
	`npm explore npm -g -- pwd`/bin/node-gyp-bin/node-gyp build

clean:
	@rm -rf ./build
	rm -f lib/_mapnik.node


rebuild:
	@make clean
	@./configure
	@make

test-tmp:
	@rm -rf test/tmp
	@mkdir -p test/tmp

ifndef only
test: test-tmp
	@PATH="./node_modules/mocha/bin:${PATH}" && NODE_PATH="./lib:$(NODE_PATH)" mocha -R spec
else
test: test-tmp
	@PATH="./node_modules/mocha/bin:${PATH}" && NODE_PATH="./lib:$(NODE_PATH)" mocha -R spec test/${only}.test.js
endif

check: test

fix:
	@fixjsstyle lib/*js bin/*js test/*js examples/*/*.js examples/*/*/*.js

fixc:
	@tools/fix_cpp_style.sh
	@rm src/*.*~

lint:
	@./node_modules/.bin/jshint lib/*js bin/*js test/*js examples/*/*.js examples/*/*/*.js


.PHONY: test lint fix
