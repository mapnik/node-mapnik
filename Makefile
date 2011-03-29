all: mapnik.node

install: all
	node-waf build install

mapnik.node:
	node-waf build

clean:
	node-waf clean distclean

uninstall:
	node-waf uninstall

test-tmp:
	@rm -rf test/tmp
	@mkdir -p test/tmp

ifndef only
test: all test-tmp
	expresso -I lib test/*.test.js
else
test: all test-tmp
	expresso -I lib test/${only}.test.js
endif

.PHONY: test
