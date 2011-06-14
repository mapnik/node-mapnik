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
	@rm -rf tests/tmp
	@mkdir -p tests/tmp

ifndef only
test: test-tmp
	@expresso -I lib tests/*.test.js
else
test: test-tmp
	@expresso -I lib tests/${only}.test.js
endif

.PHONY: test
