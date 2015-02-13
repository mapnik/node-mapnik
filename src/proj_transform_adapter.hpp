#ifndef __NODE_MAPNIK_PROJ_TRANSFORM_ADAPTER__
#define __NODE_MAPNIK_PROJ_TRANSFORM_ADAPTER__

#include <mapnik/proj_transform.hpp>
#include <mapnik/geometry.hpp>
#include <mapnik/geometry_container.hpp>
#include <mapnik/util/path_iterator.hpp>
#include <boost/spirit/include/support_container.hpp>

namespace mapnik {

template <typename Geometry>
struct proj_transform_adapter
{
    using size_type = std::size_t;
    using value_type = typename Geometry::value_type;

    proj_transform_adapter(Geometry const& geom,
                           proj_transform const& prj_trans)
        : geom_(geom),
          va_(geom),
          prj_trans_(prj_trans)  {}

    unsigned vertex(double *x, double *y) const
    {
        unsigned command;
        bool ok = false;
        bool skipped_points = false;
        while (!ok)
        {
            command = va_.vertex(x,y);
            if (command == SEG_END)
            {
                return command;
            }
            double z=0;
            ok = prj_trans_.forward(*x, *y, z);
            if (!ok) {
                skipped_points = true;
            }
        }
        if (skipped_points && (command == SEG_LINETO))
        {
            command = SEG_MOVETO;
        }
        return command;
    }

    void rewind(unsigned pos) const
    {
        va_.rewind(pos);
    }

    unsigned type() const
    {
        return static_cast<unsigned>(geom_.type());
    }

    size_type size() const
    {
        return geom_.size();
    }

private:
    Geometry const& geom_;
    mapnik::vertex_adapter va_;
    proj_transform const& prj_trans_;
};

}

namespace node_mapnik {
    using proj_transform_path_type = mapnik::proj_transform_adapter<mapnik::geometry_type>;
    using proj_transform_container = boost::ptr_vector<proj_transform_path_type>;
}

namespace boost { namespace spirit { namespace traits {

template <>
struct is_container<node_mapnik::proj_transform_path_type const> : mpl::true_ {} ;

template <>
struct is_container<node_mapnik::proj_transform_container const> : mpl::false_ {} ;

template <>
struct container_iterator<node_mapnik::proj_transform_path_type const>
{
    using type = mapnik::util::path_iterator<node_mapnik::proj_transform_path_type>;
};

template <>
struct begin_container<node_mapnik::proj_transform_path_type const>
{
    static mapnik::util::path_iterator<node_mapnik::proj_transform_path_type>
    call (node_mapnik::proj_transform_path_type const& g)
    {
        return mapnik::util::path_iterator<node_mapnik::proj_transform_path_type>(g);
    }
};

template <>
struct end_container<node_mapnik::proj_transform_path_type const>
{
    static mapnik::util::path_iterator<node_mapnik::proj_transform_path_type>
    call (node_mapnik::proj_transform_path_type const&)
    {
        return mapnik::util::path_iterator<node_mapnik::proj_transform_path_type>();
    }
};

}}}


#endif
