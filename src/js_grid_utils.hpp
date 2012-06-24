#ifndef __NODE_MAPNIK_GRID_UTILS_H__
#define __NODE_MAPNIK_GRID_UTILS_H__

// v8
#include <v8.h>

// mapnik
#include <mapnik/grid/grid.hpp>

#include "utils.hpp"

using namespace v8;
using namespace node;

namespace node_mapnik {

template <typename T>
static void grid2utf(T const& grid_type,
                     boost::ptr_vector<uint16_t> & lines,
                     std::vector<typename T::lookup_type>& key_order)
{
    typename T::data_type const& data = grid_type.data();
    typename T::feature_key_type const& feature_keys = grid_type.get_feature_keys();
    typename T::key_type keys;
    typename T::key_type::const_iterator key_pos;
    typename T::feature_key_type::const_iterator feature_pos;
    // start counting at utf8 codepoint 32, aka space character
    uint16_t codepoint = 32;

    unsigned array_size = data.width();
    for (unsigned y = 0; y < data.height(); ++y)
    {
        uint16_t idx = 0;
        uint16_t* line = new uint16_t[array_size];
        typename T::value_type const* row = data.getRow(y);
        for (unsigned x = 0; x < data.width(); ++x)
        {
            typename T::value_type feature_id = row[x];
            feature_pos = feature_keys.find(feature_id);
            if (feature_pos != feature_keys.end())
            {
                typename T::lookup_type const& val = feature_pos->second;
                key_pos = keys.find(val);
                if (key_pos == keys.end())
                {
                    // Create a new entry for this key. Skip the codepoints that
                    // can't be encoded directly in JSON.
                    if (codepoint == 34) ++codepoint;      // Skip "
                    else if (codepoint == 92) ++codepoint; // Skip backslash
                    if (feature_id == mapnik::grid::base_mask)
                    {
                        keys[""] = codepoint;
                        key_order.push_back("");
                    }
                    else
                    {
                        keys[val] = codepoint;
                        key_order.push_back(val);
                    }
                    line[idx++] = static_cast<uint16_t>(codepoint);
                    ++codepoint;
                }
                else
                {
                    line[idx++] = static_cast<uint16_t>(key_pos->second);
                }
            }
            // else, shouldn't get here...
        }
        lines.push_back(line);
    }
}

// requires mapnik >= r2957

template <typename T>
static void grid2utf(T const& grid_type,
                     boost::ptr_vector<uint16_t> & lines,
                     std::vector<typename T::lookup_type>& key_order,
                     unsigned int resolution)
{
    typename T::feature_key_type const& feature_keys = grid_type.get_feature_keys();
    typename T::key_type keys;
    typename T::key_type::const_iterator key_pos;
    typename T::feature_key_type::const_iterator feature_pos;
    // start counting at utf8 codepoint 32, aka space character
    uint16_t codepoint = 32;

    unsigned array_size = static_cast<unsigned int>(grid_type.width()/resolution);
    for (unsigned y = 0; y < grid_type.height(); y=y+resolution)
    {
        uint16_t idx = 0;
        uint16_t* line = new uint16_t[array_size];
        typename T::value_type const* row = grid_type.getRow(y);
        for (unsigned x = 0; x < grid_type.width(); x=x+resolution)
        {
            // todo - this lookup is expensive
            typename T::value_type feature_id = row[x];
            feature_pos = feature_keys.find(feature_id);
            if (feature_pos != feature_keys.end())
            {
                typename T::lookup_type const& val = feature_pos->second;
                key_pos = keys.find(val);
                if (key_pos == keys.end())
                {
                    // Create a new entry for this key. Skip the codepoints that
                    // can't be encoded directly in JSON.
                    if (codepoint == 34) ++codepoint;      // Skip "
                    else if (codepoint == 92) ++codepoint; // Skip backslash
                    if (feature_id == mapnik::grid::base_mask)
                    {
                        keys[""] = codepoint;
                        key_order.push_back("");
                    }
                    else
                    {
                        keys[val] = codepoint;
                        key_order.push_back(val);
                    }
                    line[idx++] = static_cast<uint16_t>(codepoint);
                    ++codepoint;
                }
                else
                {
                    line[idx++] = static_cast<uint16_t>(key_pos->second);
                }
            }
            // else, shouldn't get here...
        }
        lines.push_back(line);
    }
}


template <typename T>
static void write_features(T const& grid_type,
                           Local<Object>& feature_data,
                           std::vector<typename T::lookup_type> const& key_order)
{
    std::string const& key = grid_type.get_key();
    std::set<std::string> const& attributes = grid_type.property_names();
    typename T::feature_type const& g_features = grid_type.get_grid_features();
    typename T::feature_type::const_iterator feat_itr = g_features.begin();
    typename T::feature_type::const_iterator feat_end = g_features.end();
    bool include_key = (attributes.find(key) != attributes.end());
    for (; feat_itr != feat_end; ++feat_itr)
    {
        mapnik::feature_ptr feature = feat_itr->second;
        std::string join_value;
        if (key == grid_type.key_name())
        {
            join_value = feat_itr->first;

        }
        else if (feature->has_key(key))
        {
            join_value = feature->get(key).to_string();
        }

        if (!join_value.empty())
        {
            // only serialize features visible in the grid
            // TODO 6 of 13 seconds taken in std::find
            // switch to storing numbers?
            if(std::find(key_order.begin(), key_order.end(), join_value) != key_order.end()) {
                Local<Object> feat = Object::New();
                bool found = false;
                if (key == grid_type.key_name())
                {
                    // drop key unless requested
                    if (include_key) {
                        found = true;
                        // TODO do we need to duplicate __id__ ?
                        //feat->Set(String::NewSymbol(key.c_str()), String::New(join_value->c_str()) );
                    }
                }
                mapnik::feature_impl::iterator itr = feature->begin();
                mapnik::feature_impl::iterator end = feature->end();
                for ( ;itr!=end; ++itr)
                {
                    std::string key_name = boost::get<0>(*itr);
                    if (key_name == key) {
                        // drop key unless requested
                        if (include_key) {
                            found = true;
                            feat->Set(String::NewSymbol(key_name.c_str()),
                                boost::apply_visitor(node_mapnik::value_converter(),
                                                     boost::get<1>(*itr).base()));
                        }
                    }
                    // TODO - optimize
                    else if ( (attributes.find(key_name) != attributes.end()) )
                    {
                        found = true;
                        feat->Set(String::NewSymbol(key_name.c_str()),
                            boost::apply_visitor(node_mapnik::value_converter(),
                                                 boost::get<1>(*itr).base()));
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
            std::clog << "should not get here: key '" << key << "' not found in grid feature properties\n";
        }
    }
}

}
#endif // __NODE_MAPNIK_GRID_UTILS_H__
