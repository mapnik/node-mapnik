all: mapnik.node

install: all
	node-waf -v build install

mapnik.node:
	node-waf -v build

clean:
	node-waf -v clean distclean

uninstall:
	node-waf -v uninstall

test:
	node test.js