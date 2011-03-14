#ifndef NODE_MAPNIK_GRID_BUFFER
#define NODE_MAPNIK_GRID_BUFFER

#include <cassert>
#include <cstring>

#include "grid.h" // for grid_value;


template <class T> class grid_data
{
public:
    typedef T pixel_type;
        
    grid_data(unsigned width,unsigned height)
        : width_(width),
          height_(height),
          pData_((width!=0 && height!=0)? static_cast<T*>(::operator new(sizeof(T)*width*height)):0)
    {
        if (pData_) std::memset(pData_,0,sizeof(T)*width_*height_);
    }
          
    inline T& operator() (unsigned i,unsigned j)
    {
        assert(i<width_ && j<height_);
        return pData_[j*width_+i];
    }
    inline const T& operator() (unsigned i,unsigned j) const
    {
        assert(i<width_ && j<height_);
        return pData_[j*width_+i];
    }
    inline unsigned width() const
    {
        return width_;
    }
    inline unsigned height() const
    {
        return height_;
    }
        
    inline const T* getData() const
    {
        return pData_;
    }

    inline T* getData()
    {
        return pData_;
    }

    inline ~grid_data()
    {
        ::operator delete(pData_),pData_=0;
    }
        
private:
    const unsigned width_;
    const unsigned height_;
    T *pData_;
    grid_data& operator=(const grid_data&);
};

typedef grid_data<agg_grid::grid_value> grid_buffer;


#endif //NODE_MAPNIK_GRID_BUFFER
