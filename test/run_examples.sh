# run all examples just to ensure they don't throw any errors
node ./test/simple/render.js ./test/stylesheet.xml /tmp/map.png
node ./test/simple/blank.js & PID=$!; sleep 2; kill $PID
node ./test/simple/simple.js & PID=$!; sleep 2; kill $PID
node ./test/simple/simple_express.js & PID=$!; sleep 2; kill $PID
node ./test/memory_datasource/simple.js; open memory_points.png
node ./test/wms/wms_pool.js ./test/stylesheet.xml 8000 & PID=$!; sleep 2; kill $PID
node ./test/tile/pool/app.js ./test/stylesheet.xml 8000 & PID=$!; sleep 2; kill $PID
node ./test/tile/database/app.js ./test/stylesheet.xml 8000 & PID=$!; sleep 2; kill $PID
node ./test/tile/elastic/app.js 8000 & PID=$!; sleep 2; kill $PID

# cleanup
sleep 2;
rm memory_points*
