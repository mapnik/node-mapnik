all: mapnik.node

install: all
	@node-waf build install

mapnik.node:
	@node-waf build

clean:
	@node-waf clean distclean

uninstall:
	@node-waf uninstall

test-tmp:
	@rm -rf tests/tmp
	@mkdir -p tests/tmp

ifndef only
test: test-tmp
	@if test -e "node_modules/expresso/"; then ./node_modules/expresso/bin/expresso -I lib tests/*.test.js; else expresso -I lib tests/*.test.js; fi;
else
test: test-tmp
	@if test -e "node_modules/expresso/"; then ./node_modules/expresso/bin/expresso -I lib tests/${only}.test.js; else expresso -I lib tests/${only}.test.js; fi;
endif

.PHONY: test
