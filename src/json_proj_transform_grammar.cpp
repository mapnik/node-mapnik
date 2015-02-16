#include <mapnik/json/geometry_generator_grammar_impl.hpp>
#include "proj_transform_adapter.hpp"
#include <string>

namespace mapnik { namespace json { namespace detail {

// adapt mapnik::vertex_adapter get_first
template <>
struct get_first<node_mapnik::proj_transform_path_type>
{
    using geometry_type = node_mapnik::proj_transform_path_type;
    using result_type = geometry_type::value_type;
    result_type operator() (geometry_type const& geom) const
    {
        result_type coord;
        geom.rewind(0);
        std::get<0>(coord) = geom.vertex(&std::get<1>(coord),&std::get<2>(coord));
        return coord;
    }
};


} // namespace detail
} // namespace json
} // namespace mapnik

using sink_type = std::back_insert_iterator<std::string>;
template struct mapnik::json::multi_geometry_generator_grammar<sink_type, node_mapnik::proj_transform_container>;
