all: mapnik.node

install: all
	node-waf -v install

mapnik.node:
	node-waf -v configure build

clean:
	node-waf -v clean distclean

uninstall:
	node-waf -v uninstall

test:
	node test.js