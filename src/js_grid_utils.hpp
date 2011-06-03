#ifndef __NODE_MAPNIK_GRID_UTILS_H__
#define __NODE_MAPNIK_GRID_UTILS_H__

// v8
#include <v8.h>

// mapnik
#include <mapnik/grid/grid.hpp>

using namespace v8;
using namespace node;

namespace node_mapnik {

static void grid2utf(mapnik::grid const& grid, 
    Local<Array>& l,
    std::vector<mapnik::grid::lookup_type>& key_order)
{
    mapnik::grid::data_type const& data = grid.data();
    mapnik::grid::feature_key_type const& feature_keys = grid.get_feature_keys();
    mapnik::grid::key_type keys;
    mapnik::grid::key_type::const_iterator key_pos;
    mapnik::grid::feature_key_type::const_iterator feature_pos;
    uint16_t codepoint = 31;
    uint16_t row_idx = 0;

    for (unsigned y = 0; y < data.height(); ++y)
    {
        uint16_t idx = 0;
        boost::scoped_array<uint16_t> line(new uint16_t[data.width()]);
        mapnik::grid::value_type const* row = data.getRow(y);
        for (unsigned x = 0; x < data.width(); ++x)
        {
            feature_pos = feature_keys.find(row[x]);
            if (feature_pos != feature_keys.end())
            {
                mapnik::grid::lookup_type const& val = feature_pos->second;
                key_pos = keys.find(val);
                if (key_pos == keys.end())
                {
                    // Create a new entry for this key. Skip the codepoints that
                    // can't be encoded directly in JSON.
                    ++codepoint;
                    if (codepoint == 34) ++codepoint;      // Skip "
                    else if (codepoint == 92) ++codepoint; // Skip backslash
                
                    keys[val] = codepoint;
                    key_order.push_back(val);
                    line[idx++] = static_cast<uint16_t>(codepoint);
                }
                else
                {
                    line[idx++] = static_cast<uint16_t>(key_pos->second);
                }
            }
            // else, shouldn't get here...
        }
        l->Set(row_idx, String::New(line.get(),data.width()));
        ++row_idx;
    }
}


static void write_features(mapnik::grid::feature_type const& g_features,
    Local<Object>& feature_data,
    std::vector<mapnik::grid::lookup_type> const& key_order,
    std::string const& join_field,
    std::set<std::string> const& attributes)
{
    mapnik::grid::feature_type::const_iterator feat_itr = g_features.begin();
    mapnik::grid::feature_type::const_iterator feat_end = g_features.end();
    bool include_join_field = (attributes.find(join_field) != attributes.end());
    for (; feat_itr != feat_end; ++feat_itr)
    {
        std::map<std::string,mapnik::value> const& props = feat_itr->second;
        std::map<std::string,mapnik::value>::const_iterator const& itr = props.find(join_field);
        if (itr != props.end())
        {
            mapnik::grid::lookup_type const& join_value = itr->second.to_string();
    
            // only serialize features visible in the grid
            if(std::find(key_order.begin(), key_order.end(), join_value) != key_order.end()) {
                Local<Object> feat = Object::New();
                std::map<std::string,mapnik::value>::const_iterator it = props.begin();
                std::map<std::string,mapnik::value>::const_iterator end = props.end();
                bool found = false;
                for (; it != end; ++it)
                {
                    std::string const& key = it->first;
                    if (key == join_field) {
                        // drop join_field unless requested
                        if (include_join_field) {
                            found = true;
                            params_to_object serializer( feat , it->first);
                            boost::apply_visitor( serializer, it->second.base() );
                        }
                    }
                    else if ( (attributes.find(key) != attributes.end()) )
                    {
                        found = true;
                        params_to_object serializer( feat , it->first);
                        boost::apply_visitor( serializer, it->second.base() );
                    }
                }
                if (found)
                {
                    feature_data->Set(String::NewSymbol(feat_itr->first.c_str()), feat);
                }
            }
        }
        else
        {
            std::clog << "should not get here: join_field '" << join_field << "' not found in grid feature properties\n";
        }
    }
}

}
#endif // __NODE_MAPNIK_GRID_UTILS_H__