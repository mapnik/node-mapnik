#http://www.gnu.org/prep/standards/html_node/Standard-Targets.html#Standard-Targets

all: build

./node_modules/node-pre-gyp:
	npm install node-pre-gyp

./node_modules: ./node_modules/node-pre-gyp
	npm install `node -e "console.log(Object.keys(require('./package.json').dependencies).join(' '))"` \
	`node -e "console.log(Object.keys(require('./package.json').devDependencies).join(' '))"` --clang=1

build: ./node_modules
	./node_modules/.bin/node-pre-gyp build --loglevel=silent --clang=1

debug: ./node_modules
	./node_modules/.bin/node-pre-gyp build --debug --clang=1

verbose: ./node_modules
	./node_modules/.bin/node-pre-gyp build --loglevel=verbose --clang=1

clean:
	@rm -rf ./build
	rm -rf lib/binding/
	rm ./test/tmp/*
	echo > ./test/tmp/placeholder.txt
	rm -rf ./node_modules/

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

.PHONY: test clean build
