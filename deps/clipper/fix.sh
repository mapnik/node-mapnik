perl -i -p -e "s/\.X/\.x/g;" ./deps/clipper/clipper.*
perl -i -p -e "s/\->X/\->x/g;" ./deps/clipper/clipper.*
perl -i -p -e "s/cInt X;/cInt x;/g;" ./deps/clipper/clipper.*
perl -i -p -e "s/double X;/double x;/g;" ./deps/clipper/clipper.*
perl -i -p -e "s/X\(x\)/x\(x\)/g;" ./deps/clipper/clipper.*
perl -i -p -e "s/X\(\(/x\(\(/g;" ./deps/clipper/clipper.*

perl -i -p -e "s/\.Y/\.y/g;" ./deps/clipper/clipper.*
perl -i -p -e "s/\->Y/\->y/g;" ./deps/clipper/clipper.*
perl -i -p -e "s/cInt Y;/cInt y;/g;" ./deps/clipper/clipper.*
perl -i -p -e "s/double Y;/double y;/g;" ./deps/clipper/clipper.*
perl -i -p -e "s/Y\(y\)/y\(y\)/g;" ./deps/clipper/clipper.*
perl -i -p -e "s/Y\(\(/y\(\(/g;" ./deps/clipper/clipper.*
perl -i -p -e "s/cInt          Y;/cInt          y;/g;" ./deps/clipper/clipper.*


