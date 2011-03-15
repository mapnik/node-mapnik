all: mapnik.node

install: all
	node-waf build install

mapnik.node:
	node-waf build

clean:
	node-waf clean distclean

uninstall:
	node-waf uninstall

test: all
	node test.js
