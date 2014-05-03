#http://www.gnu.org/prep/standards/html_node/Standard-Targets.html#Standard-Targets

all: mapnik.node

./node_modules/mapnik-vector-tile:
	npm install mapnik-vector-tile sphericalmercator mocha

./node_modules/.bin/node-pre-gyp:
	npm install node-pre-gyp

mapnik.node: ./node_modules/.bin/node-pre-gyp ./node_modules/mapnik-vector-tile
	./node_modules/.bin/node-pre-gyp build

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
