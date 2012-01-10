# run all examples just to ensure they don't throw any errors
./examples/simple/render.js ./examples/stylesheet.xml /tmp/map.png
./examples/simple/blank.js & PID=$!; sleep 2; kill $PID
./examples/simple/simple.js & PID=$!; sleep 2; kill $PID
./examples/simple/simple_pool.js & PID=$!; sleep 2; kill $PID
./examples/simple/simple_express.js & PID=$!; sleep 2; kill $PID
./examples/js_datasource/simple.js; open js_points.png
./examples/js_datasource/usgs_quakes.js; open quakes.png
./examples/memory_datasource/simple.js; open memory_points.png
./examples/wms/wms.js ./examples/stylesheet.xml 8000 & PID=$!; sleep 2; kill $PID
./examples/wms/wms_pool.js ./examples/stylesheet.xml 8000 & PID=$!; sleep 2; kill $PID
./examples/tile/pool/app.js ./examples/stylesheet.xml 8000 & PID=$!; sleep 2; kill $PID
./examples/tile/database/app.js ./examples/stylesheet.xml 8000 & PID=$!; sleep 2; kill $PID
./examples/tile/elastic/app.js 8000 & PID=$!; sleep 2; kill $PID

# cleanup
sleep 2;
rm memory_points*
rm js_points.png
rm quakes.png