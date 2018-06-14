CREATE TABLE test(gid serial PRIMARY KEY, geom geometry, colbigint bigint, col_text text, "col-char" char, "col+bool" boolean, "colnumeric" numeric, "colsmallint" smallint, "colfloat4" real, "colfloat8" double precision, "colcharacter" character);
INSERT INTO test VALUES (DEFAULT, GeomFromEWKT('SRID=4326;POINT(0 0)'), -9223372036854775808, 'I am a point', 'A', TRUE, 1234567809990001, 0, 0.0, 0.0, 'A');
INSERT INTO test VALUES (DEFAULT, GeomFromEWKT('SRID=4326;POINT(-2 2)'), 9223372036854775807, 'I, too, am a point!', 'B', FALSE, -123456780999001, 0, 0.0, 0.0, 'A');
INSERT INTO test VALUES (DEFAULT, GeomFromEWKT('SRID=4326;MULTIPOINT(2 1,1 2)'), -1, 'I`m even a MULTI Point', 'Z', FALSE, 12345678099901, 0, 0.0, 0.0, 'A');
INSERT INTO test VALUES (DEFAULT, GeomFromEWKT('SRID=4326;LINESTRING(0 0,1 1,1 2)'), 0, 'This is a line string', 'ß', FALSE, -9, 0, 0.0, 0.0, 'A');
INSERT INTO test VALUES (DEFAULT, GeomFromEWKT('SRID=4326;MULTILINESTRING((1 0,0 1,3 2),(3 2,5 4))'), 1, 'multi line string', 'Ü', TRUE, 0.00001, 0, 0.0, 0.0, 'A');
INSERT INTO test VALUES (DEFAULT, GeomFromEWKT('SRID=4326;POLYGON((0 0,4 0,4 4,0 4,0 0),(1 1, 2 1, 2 2, 1 2,1 1))'), 1, 'polygon', 'Ü', TRUE, 0.00001, 0, 0.0, 0.0, 'A');
INSERT INTO test VALUES (DEFAULT, GeomFromEWKT('SRID=4326;MULTIPOLYGON(((1 1,3 1,3 3,1 3,1 1),(1 1,2 1,2 2,1 2,1 1)), ((-1 -1,-1 -2,-2 -2,-2 -1,-1 -1)))'), 5432, 'multi ploygon', 'X', TRUE, 999, 0, 0.0, 0.0, 'A');
INSERT INTO test VALUES (DEFAULT, GeomFromEWKT('SRID=4326;GEOMETRYCOLLECTION(POLYGON((1 1, 2 1, 2 2, 1 2,1 1)),POINT(2 3),LINESTRING(2 3,3 4))'), 8080, 'GEOMETRYCOLLECTION', 'm', TRUE, 9999, 0, 0.0, 0.0, 'A');
