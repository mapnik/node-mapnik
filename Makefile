#http://www.gnu.org/prep/standards/html_node/Standard-Targets.html#Standard-Targets

all: mapnik.node

mapnik.node:
	PATH=$$PATH:`npm explore npm -g -- pwd`/bin/node-gyp-bin/ node-gyp build

clean:
	@rm -rf ./build
	rm -rf lib/binding
	rm ./test/tmp/*
	echo > ./test/tmp/placeholder.txt


rebuild:
	@make clean
	@./configure
	@make

ifndef only
test:
	@PATH="./node_modules/mocha/bin:${PATH}" && NODE_PATH="./lib:$(NODE_PATH)" mocha -R spec
else
test:
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
