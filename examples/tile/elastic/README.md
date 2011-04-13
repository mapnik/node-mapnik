# Elastic Search + MemoryDatasource

The idea here is to try to get points rendered on a map from a unique 
data source that Mapnik proper will likely never support formally.

Elastic Search supports point storage and bbox queries, so that is all we need to make a map of points.


# Setup

First install elastic search (ya, its java) and get it running on the default of localhost:9200.


    wget --no-check-certificate http://github.com/downloads/elasticsearch/elasticsearch/elasticsearch-0.15.2.tar.gz
    tar xvf elasticsearch-0.15.2.tar.gz
    ./elasticsearch # done, should be running - do ya kinda like java now?


# Create an index and mapping

Run the 'create.sh' script to do this.

    ./examples/tile/elastic/create.sh

# Import points in spherical mercator projection

Run the 'seed_es_index.js' script:

    ./examples/tile/elastic/seed_es_index.js
    
The above just pulls the polygons from a sample dataset and turns their bounding boxes into points.

# Test


Run the web server sample:

    node ./examples/tile/elastic/app.js 8000


Then open the polymaps example and you should see red dots:

    open ./examples/tile/index.html


# Fun additional curl commands to use with Elastic Search


# refresh to make sure data is live
curl -XPOST 'localhost:9200/_refresh'

# make sure the metadata on the mapping looks good
# you should see "type":"geo_point" in results
curl -XGET http://localhost:9200/geo/_mapping

# http://www.elasticsearch.org/guide/reference/api/search/filter.html

# create some data for id #1
curl -XPUT http://localhost:9200/geo/project/1 -d '{
    "project" : {
        "location" : {
            "lat" : 40.12,
            "lon" : -71.34
        }
    }
}'

# get that record right back to confirm it went in
curl -XGET http://localhost:9200/geo/project/1

# query it by getting all data
curl -XGET http://localhost:9200/geo/_search -d '{
    "query": { "match_all":{} }
}'


# actually query the data spatially using a bbox
# should return same single result
curl -XGET http://localhost:9200/geo/project/_search -d '{
        "from" : 0, "size" : 100000,
        "query" : {
            "match_all" : {}
        },
        "filter" : {
            "geo_bounding_box" : {
                "project.location" : {
                    "top_left" : {
                        "lat" : 50,
                        "lon" : -80
                    },
                    "bottom_right" : {
                        "lat" : 40,
                        "lon" : -70
                    }
                }
            }
        }
}'