# "index" and "type" are ES terms, see for more detail:
# http://www.elasticsearch.org/guide/reference/api/index_.html
# "geo" below is the "index"
# "project" is the "type"

# clear out the index if it already exists
curl -XDELETE http://localhost:9200/geo/

# create the index
curl -XPUT http://localhost:9200/geo/

# create the "mapping" which is needed to support geo queries
# a "mapping" is basically giving a field a specific type, in this case "geo_point"
curl -XPUT http://localhost:9200/geo/_mapping -d '{
    "project" : {
        "properties" : {
            "location" : { "type" : "geo_point", "store": "yes", "lat_lon":true }
        }
    }
}'
