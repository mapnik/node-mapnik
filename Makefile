#http://www.gnu.org/prep/standards/html_node/Standard-Targets.html#Standard-Targets

all: build-all

./node_modules/node-pre-gyp:
	npm install node-pre-gyp

./node_modules: ./node_modules/node-pre-gyp
	npm install `node -e "console.log(Object.keys(require('./package.json').dependencies).join(' '))"` \
	`node -e "console.log(Object.keys(require('./package.json').devDependencies).join(' '))"` --clang=1

./build:
	./node_modules/.bin/node-pre-gyp configure --loglevel=error --clang=1

build-all: ./node_modules ./build
	./node_modules/.bin/node-pre-gyp build --loglevel=error --clang=1

debug: ./node_modules ./build
	./node_modules/.bin/node-pre-gyp build --debug --clang=1

coverage: ./node_modules ./build
	./node_modules/.bin/node-pre-gyp build --debug --clang=1 --coverage=true

verbose: ./node_modules
	./node_modules/.bin/node-pre-gyp build --loglevel=verbose --clang=1

clean:
	@rm -rf ./build
	rm -rf lib/binding/
	rm ./test/tmp/*
	echo > ./test/tmp/placeholder.txt
	rm -rf ./node_modules/
	rm -f ./*tgz

grind:
	valgrind --leak-check=full node node_modules/.bin/_mocha

testpack:
	rm -f ./*tgz
	npm pack
	tar -ztvf *tgz
	rm -f ./*tgz

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
