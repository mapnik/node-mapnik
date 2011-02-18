//----------------------------------------------------------------------------
// Anti-Grain Geometry - Version 2.1 Lite 
// Copyright (C) 2002-2003 Maxim Shemanarev (McSeem)
//
// Permission to copy, use, modify, sell and distribute this software 
// is granted provided this copyright notice appears in all copies. 
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//
// The author gratefully acknowleges the support of David Turner, 
// Robert Wilhelm, and Werner Lemberg - the authors of the FreeType 
// libray - in producing this work. See http://www.freetype.org for details.
//
//----------------------------------------------------------------------------
// Contact: mcseem@antigrain.com
//          mcseemagg@yahoo.com
//          http://www.antigrain.com
//----------------------------------------------------------------------------
#ifndef AGG_INCLUDED
#define AGG_INCLUDED

#include <string.h>
//#include <iostream>

namespace agg
{
    //------------------------------------------------------------------------
    typedef signed char    int8;
    typedef unsigned char  int8u;
    typedef signed short   int16;
    typedef unsigned short int16u;
    typedef signed int     int32;
    typedef unsigned int   int32u;


    //========================================================================
    //
    // This class is used to transfer data from class outline (or a similar one)
    // to the rendering buffer. It's organized very simple. The class stores 
    // information of horizontal spans to render it into a pixel-map buffer. 
    // Each span has initial X, length, and an array of bytes that determine the 
    // alpha-values for each pixel. So, the restriction of using this class is 256 
    // levels of Anti-Aliasing, which is quite enough for any practical purpose.
    // Before using this class you should know the minimal and maximal pixel 
    // coordinates of your scanline. The protocol of using is:
    // 1. reset(min_x, max_x)
    // 2. add_cell() / add_span() - accumulate scanline. You pass Y-coordinate 
    //    into these functions in order to make scanline know the last Y. Before 
    //    calling add_cell() / add_span() you should check with method is_ready(y)
    //    if the last Y has changed. It also checks if the scanline is not empty. 
    //    When forming one scanline the next X coordinate must be always greater
    //    than the last stored one, i.e. it works only with ordered coordinates.
    // 3. If the current scanline is_ready() you should render it and then call 
    //    reset_spans() before adding new cells/spans.
    //    
    // 4. Rendering:
    // 
    // Scanline provides an iterator class that allows you to extract
    // the spans and the cover values for each pixel. Be aware that clipping
    // has not been done yet, so you should perform it yourself.
    // Use scanline::iterator to render spans:
    //-------------------------------------------------------------------------
    //
    // int base_x = sl.base_x();          // base X. Should be added to the span's X
    //                                    // "sl" is a const reference to the 
    //                                    // scanline passed in.
    //
    // int y = sl.y();                    // Y-coordinate of the scanline
    //
    // ************************************
    // ...Perform vertical clipping here...
    // ************************************
    //
    // scanline::iterator span(sl);
    // 
    // unsigned char* row = m_rbuf->row(y); // The the address of the beginning 
    //                                      // of the current row
    // 
    // unsigned num_spans = sl.num_spans(); // Number of spans. It's guaranteed that
    //                                      // num_spans is always greater than 0.
    //
    // do
    // {
    //     int x = span.next() + base_x;        // The beginning X of the span
    //
    //     const int8u covers* = span.covers(); // The array of the cover values
    //
    //     int num_pix = span.num_pix();        // Number of pixels of the span.
    //                                          // Always greater than 0, still we
    //                                          // shoud use "int" instead of 
    //                                          // "unsigned" because it's more
    //                                          // convenient for clipping
    //
    //     **************************************
    //     ...Perform horizontal clipping here...
    //     ...you have x, covers, and pix_count..
    //     **************************************
    //
    //     unsigned char* dst = row + x;  // Calculate the start address of the row.
    //                                    // In this case we assume a simple 
    //                                    // grayscale image 1-byte per pixel.
    //     do
    //     {
    //         *dst++ = *covers++;        // Hypotetical rendering. 
    //     }
    //     while(--num_pix);
    // } 
    // while(--num_spans);  // num_spans cannot be 0, so this loop is quite safe
    //------------------------------------------------------------------------
    //
    // The question is: why should we accumulate the whole scanline when we
    // could render just separate spans when they're ready?
    // That's because using the scaline is in general faster. When is consists 
    // of more than one span the conditions for the processor cash system
    // are better, because switching between two different areas of memory 
    // (that can be large ones) occures less frequently.
    //------------------------------------------------------------------------
    class scanline
    {
    public:
        /* as low as 1 if gamma == 0.0 */
        //enum { aa_shift = 8 };
        enum { aa_shift = 1 };

        class iterator
        {
        public:
            iterator(const scanline& sl) :
                m_covers(sl.m_covers),
                m_cur_count(sl.m_counts),
                m_cur_start_ptr(sl.m_start_ptrs)
            {
            }

            int next()
            {
                ++m_cur_count;
                ++m_cur_start_ptr;
                return int(*m_cur_start_ptr - m_covers);
            }

            int num_pix() const { return int(*m_cur_count); }
            const int8u* covers() const { return *m_cur_start_ptr; }

        private:
            const int8u*        m_covers;
            const int16u*       m_cur_count;
            const int8u* const* m_cur_start_ptr;
        };

        friend class iterator;

        ~scanline();
        scanline();

        void     reset(int min_x, int max_x, int dx=0, int dy=0);

        void     reset_spans();
        void     add_cell(int x, int y, unsigned cover);
        void     add_span(int x, int y, unsigned len, unsigned cover);
        int      is_ready(int y) const;
        int      base_x()    const { return m_min_x + m_dx;  }
        int      y()         const { return m_last_y + m_dy; }
        unsigned num_spans() const { return m_num_spans;     }

    private:
        scanline(const scanline&);
        const scanline& operator = (const scanline&);

    private:
        int      m_min_x;
        unsigned m_max_len;
        int      m_dx;
        int      m_dy;
        int      m_last_x;
        int      m_last_y;
        int8u*   m_covers;
        int8u**  m_start_ptrs;
        int16u*  m_counts;
        unsigned m_num_spans;
        int8u**  m_cur_start_ptr;
        int16u*  m_cur_count;
    };


    //------------------------------------------------------------------------
    inline void scanline::reset_spans()
    {
        m_last_x        = 0x7FFF;
        m_last_y        = 0x7FFF;
        m_cur_count     = m_counts;
        m_cur_start_ptr = m_start_ptrs;
        m_num_spans     = 0;
    }


    //------------------------------------------------------------------------
    inline void scanline::add_cell(int x, int y, unsigned cover)
    {
        x -= m_min_x;
        m_covers[x] = (unsigned char)cover;
        if(x == m_last_x+1)
        {
            (*m_cur_count)++;
        }
        else
        {
            *++m_cur_count = 1;
            *++m_cur_start_ptr = m_covers + x;
            m_num_spans++;
        }
        m_last_x = x;
        m_last_y = y;
    }


    //------------------------------------------------------------------------
    inline int scanline::is_ready(int y) const
    {
        return m_num_spans && (y ^ m_last_y);
    }



    //------------------------------------------------------------------------
    // These constants determine the subpixel accuracy, to be more precise, 
    // the number of bits of the fractional part of the coordinates. 
    // The possible coordinate capacity in bits can be calculated by formula:
    // sizeof(int) * 8 - poly_base_shift * 2, i.e, for 32-bit integers and
    // 8-bits fractional part the capacity is 16 bits or [-32768...32767].
    enum
    {
        //poly_base_shift = 8,
        poly_base_shift = 4,
        poly_base_size  = 1 << poly_base_shift,
        poly_base_mask  = poly_base_size - 1
    };
    
    //------------------------------------------------------------------------
    inline int poly_coord(double c)
    {
        return int(c * poly_base_size);
    }

    //------------------------------------------------------------------------
    // A pixel cell. There're no constructors defined and it was done 
    // intentionally in order to avoid extra overhead when allocating an 
    // array of cells.
    struct cell
    {
        int16 x;
        int16 y;
        int   packed_coord;
        int   cover;
        int   area;

        void set(int x, int y, int c, int a);
        void set_coord(int x, int y);
        void set_cover(int c, int a);
        void add_cover(int c, int a);
    };


    //------------------------------------------------------------------------
    // An internal class that implements the main rasterization algorithm.
    // Used in the rasterizer. Should not be used direcly.
    class outline
    {
        enum
        {
            cell_block_shift = 12,
            cell_block_size  = 1 << cell_block_shift,
            cell_block_mask  = cell_block_size - 1,
            cell_block_pool  = 256,
            cell_block_limit = 1024
        };

    public:

        ~outline();
        outline();

        void reset();

        void move_to(int x, int y);
        void line_to(int x, int y);

        int min_x() const { return m_min_x; }
        int min_y() const { return m_min_y; }
        int max_x() const { return m_max_x; }
        int max_y() const { return m_max_y; }

        unsigned num_cells() const {return m_num_cells; }
        const cell* const* cells();

    private:
        outline(const outline&);
        const outline& operator = (const outline&);

        void set_cur_cell(int x, int y);
        void add_cur_cell();
        void sort_cells();
        void render_scanline(int ey, int x1, int y1, int x2, int y2);
        void render_line(int x1, int y1, int x2, int y2);
        void allocate_block();
        
        static void qsort_cells(cell** start, unsigned num);

    private:
        unsigned  m_num_blocks;
        unsigned  m_max_blocks;
        unsigned  m_cur_block;
        unsigned  m_num_cells;
        cell**    m_cells;
        cell*     m_cur_cell_ptr;
        cell**    m_sorted_cells;
        unsigned  m_sorted_size;
        cell      m_cur_cell;
        int       m_cur_x;
        int       m_cur_y;
        int       m_close_x;
        int       m_close_y;
        int       m_min_x;
        int       m_min_y;
        int       m_max_x;
        int       m_max_y;
        unsigned  m_flags;
    };


    //------------------------------------------------------------------------
    enum filling_rule_e
    {
        fill_non_zero,
        fill_even_odd
    };
    


    
}



#endif

