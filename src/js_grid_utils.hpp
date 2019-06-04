#ifndef __NODE_MAPNIK_GRID_UTILS_H__
#define __NODE_MAPNIK_GRID_UTILS_H__

// mapnik
#include <mapnik/version.hpp>
#include <mapnik/feature.hpp>           // for feature_impl, etc
#include <mapnik/grid/grid.hpp>         // for grid

#include "utils.hpp"

// stl
#include <cmath> // ceil
#include <stdint.h>  // for uint16_t

#include <memory>




namespace node_mapnik {

typedef std::unique_ptr<uint16_t[]> grid_line_type;

template <typename T>
static void grid2utf(T const& grid_type,
                     std::vector<grid_line_type> & lines,
                     std::vector<typename T::lookup_type>& key_order,
                     unsigned int resolution)
{
    typedef std::map< typename T::lookup_type, typename T::value_type> keys_type;
    typedef typename keys_type::const_iterator keys_iterator;

    typename T::feature_key_type const& feature_keys = grid_type.get_feature_keys();
    typename T::feature_key_type::const_iterator feature_pos;

    keys_type keys;
    // start counting at utf8 codepoint 32, aka space character
    uint16_t codepoint = 32;

    unsigned array_size = std::ceil(grid_type.width()/static_cast<float>(resolution));
    for (unsigned y = 0; y < grid_type.height(); y=y+resolution)
    {
        uint16_t idx = 0;
        grid_line_type line(new uint16_t[array_size]);
        typename T::value_type const* row = grid_type.get_row(y);
        for (unsigned x = 0; x < grid_type.width(); x=x+resolution)
        {
            // todo - this lookup is expensive
            typename T::value_type feature_id = row[x];
            feature_pos = feature_keys.find(feature_id);
            if (feature_pos != feature_keys.end())
            {
                typename T::lookup_type const& val = feature_pos->second;
                keys_iterator key_pos = keys.find(val);
                if (key_pos == keys.end())
                {
                    // Create a new entry for this key. Skip the codepoints that
                    // can't be encoded directly in JSON.
                    if (codepoint == 34) ++codepoint;      // Skip "
                    else if (codepoint == 92) ++codepoint; // Skip backslash
                    if (feature_id == mapnik::grid::base_mask)
                    {
                        keys[""] = codepoint;
                        key_order.emplace_back("");
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
        lines.push_back(std::move(line));
    }
}


template <typename T>
static void write_features(T const& grid_type,
                           v8::Local<v8::Object>& feature_data,
                           std::vector<typename T::lookup_type> const& key_order)
{
    Nan::HandleScope scope;
    typename T::feature_type const& g_features = grid_type.get_grid_features();
    if (g_features.size() <= 0)
    {
        return;
    }
    std::set<std::string> const& attributes = grid_type.get_fields();
    typename T::feature_type::const_iterator feat_end = g_features.end();
    for (std::string const& key_item : key_order)
    {
        if (key_item.empty())
        {
            continue;
        }

        typename T::feature_type::const_iterator feat_itr = g_features.find(key_item);
        if (feat_itr == feat_end)
        {
            continue;
        }

        bool found = false;
        v8::Local<v8::Object> feat = Nan::New<v8::Object>();
        mapnik::feature_ptr feature = feat_itr->second;
        for (std::string const& attr : attributes)
        {
            if (attr == "__id__")
            {
                Nan::Set(feat, Nan::New<v8::String>(attr).ToLocalChecked(), Nan::New<v8::Number>(feature->id()));
            }
            else if (feature->has_key(attr))
            {
                found = true;
                mapnik::feature_impl::value_type const& attr_val = feature->get(attr);
                Nan::Set(feat, Nan::New<v8::String>(attr).ToLocalChecked(),
                    mapnik::util::apply_visitor(node_mapnik::value_converter(),
                    attr_val));
            }
        }

        if (found)
        {
            Nan::Set(feature_data, Nan::New<v8::String>(feat_itr->first).ToLocalChecked(), feat);
        }
    }
}

}
#endif // __NODE_MAPNIK_GRID_UTILS_H__
