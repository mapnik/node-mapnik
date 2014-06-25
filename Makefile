#http://www.gnu.org/prep/standards/html_node/Standard-Targets.html#Standard-Targets

all: mapnik.node

./node_modules:
	npm install --build-from-source

mapnik.node: ./node_modules
	./node_modules/.bin/node-pre-gyp build --loglevel=silent

debug:
	./node_modules/.bin/node-pre-gyp rebuild --debug

verbose:
	./node_modules/.bin/node-pre-gyp rebuild --loglevel=verbose

clean:
	@rm -rf ./build
	rm -rf lib/binding
	rm ./test/tmp/*
	rm -rf ./node_modules
	echo > ./test/tmp/placeholder.txt

grind:
	valgrind --leak-check=full node node_modules/.bin/_mocha

rebuild:
	@make clean
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
