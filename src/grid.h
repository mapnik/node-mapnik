#ifndef GRID_H
#define GRID_H

#include <string.h>
//#include <iostream>
#include "renderer.h"


namespace agg
{

    typedef unsigned int grid_value;

    class grid_rendering_buffer
    {
    public:
        
        ~grid_rendering_buffer()
        {
            delete [] m_rows;
        }
    
    
        //------------------------------------------------------------------------
        grid_rendering_buffer(grid_value* buf,
                                           unsigned width, 
                                           unsigned height,
                                           int      stride) :
            m_buf(0),
            m_rows(0),
            m_width(0),
            m_height(0),
            m_stride(0),
            m_max_height(0)
        {
            attach(buf, width, height, stride);
        }
    
    
        //------------------------------------------------------------------------
        void attach(grid_value* buf,
                                      unsigned width, 
                                      unsigned height,
                                      int      stride)
        {
            m_buf = buf;
            m_width = width;
            m_height = height;
            m_stride = stride;
            if(height > m_max_height)
            {
                delete [] m_rows;
                m_rows = new grid_value* [m_max_height = height];
            }
    
            grid_value* row_ptr = m_buf;
    
            if(stride < 0)
            {
                row_ptr = m_buf - int(height - 1) * stride;
            }
    
            grid_value** rows = m_rows;
    
            while(height--)
            {
                *rows++ = row_ptr;
                row_ptr += stride;
            }
        }
    

        //-----------------------------------------Acessors
        const grid_value* buf()    const { return m_buf;    }
        unsigned             width()  const { return m_width;  }
        unsigned             height() const { return m_height; }
        int                  stride() const { return m_stride; }

        bool inbox(int x, int y) const
        {
            return x >= 0 && y >= 0 && x < int(m_width) && y < int(m_height);
        }
        
        grid_value* row(unsigned y) { return m_rows[y];  }
        const grid_value* row(unsigned y) const { return m_rows[y]; }
        
    private:
        grid_rendering_buffer(const grid_rendering_buffer&);
        const grid_rendering_buffer& operator = (const grid_rendering_buffer&);

    private:
        grid_value*  m_buf;        // Pointer to renrdering buffer
        grid_value** m_rows;       // Pointers to each row of the buffer
        unsigned        m_width;      // Width in pixels
        unsigned        m_height;     // Height in pixels
        int             m_stride;     // Number of bytes per row. Can be < 0
        unsigned        m_max_height; // Maximal current height
    };


    //========================================================================
    template<class Span> class grid_renderer
    {
    public:
        //--------------------------------------------------------------------
        grid_renderer(grid_rendering_buffer& rbuf) : m_rbuf(&rbuf)
        {
        }
        
        //--------------------------------------------------------------------
        void clear(grid_value c)
        {
            unsigned y;
            for(y = 0; y < m_rbuf->height(); y++)
            {
                m_span.hline(m_rbuf->row(y), 0, m_rbuf->width(), c);
            }
        }

        //--------------------------------------------------------------------
        void render(const scanline& sl, grid_value c)
        {
            if(sl.y() < 0 || sl.y() >= int(m_rbuf->height()))
            {
                return;
            }

            unsigned num_spans = sl.num_spans();
            int base_x = sl.base_x();
            grid_value* row = m_rbuf->row(sl.y());
            scanline::iterator span(sl);

            do
            {
                int x = span.next() + base_x;
                const int8u* covers = span.covers();
                int num_pix = span.num_pix();
                if(x < 0)
                {
                    num_pix += x;
                    if(num_pix <= 0) continue;
                    covers -= x;
                    x = 0;
                }
                if(x + num_pix >= int(m_rbuf->width()))
                {
                    num_pix = m_rbuf->width() - x;
                    if(num_pix <= 0) continue;
                }
                m_span.render(row, x, num_pix, covers, c);
            }
            while(--num_spans);
        }

        //--------------------------------------------------------------------
        grid_rendering_buffer& rbuf() { return *m_rbuf; }

    private:
        grid_rendering_buffer* m_rbuf;
        Span              m_span;
    };


    class grid_rasterizer
    {
    public:
        enum
        {
            aa_shift = 1,//scanline::aa_shift,
            aa_num   = 1 << aa_shift,
            aa_mask  = aa_num - 1,
            aa_2num  = aa_num * 2,
            aa_2mask = aa_2num - 1
        };

        //---------------------------------------------------------path_commands_e
        enum path_commands_e
        {
            path_cmd_stop     = 0,        //----path_cmd_stop    
            path_cmd_move_to  = 1,        //----path_cmd_move_to 
            path_cmd_line_to  = 2,        //----path_cmd_line_to 
            path_cmd_curve3   = 3,        //----path_cmd_curve3  
            path_cmd_curve4   = 4,        //----path_cmd_curve4  
            path_cmd_curveN   = 5,        //----path_cmd_curveN
            path_cmd_catrom   = 6,        //----path_cmd_catrom
            path_cmd_ubspline = 7,        //----path_cmd_ubspline
            path_cmd_end_poly = 0x0F,     //----path_cmd_end_poly
            path_cmd_mask     = 0x0F      //----path_cmd_mask    
        };

        //------------------------------------------------------------path_flags_e
        enum path_flags_e
        {
            path_flags_none  = 0,         //----path_flags_none 
            path_flags_ccw   = 0x10,      //----path_flags_ccw  
            path_flags_cw    = 0x20,      //----path_flags_cw   
            path_flags_close = 0x40,      //----path_flags_close
            path_flags_mask  = 0xF0       //----path_flags_mask 
        };

        //---------------------------------------------------------------is_vertex
        inline bool is_vertex(unsigned c)
        {
            return c >= path_cmd_move_to && c < path_cmd_end_poly;
        }
    
        //--------------------------------------------------------------is_drawing
        inline bool is_drawing(unsigned c)
        {
            return c >= path_cmd_line_to && c < path_cmd_end_poly;
        }
    
        //-----------------------------------------------------------------is_stop
        inline bool is_stop(unsigned c)
        { 
            return c == path_cmd_stop;
        }
    
        //--------------------------------------------------------------is_move_to
        inline bool is_move_to(unsigned c)
        {
            return c == path_cmd_move_to;
        }
    
        //--------------------------------------------------------------is_line_to
        inline bool is_line_to(unsigned c)
        {
            return c == path_cmd_line_to;
        }
    
        //----------------------------------------------------------------is_curve
        inline bool is_curve(unsigned c)
        {
            return c == path_cmd_curve3 || c == path_cmd_curve4;
        }
    
        //---------------------------------------------------------------is_curve3
        inline bool is_curve3(unsigned c)
        {
            return c == path_cmd_curve3;
        }
    
        //---------------------------------------------------------------is_curve4
        inline bool is_curve4(unsigned c)
        {
            return c == path_cmd_curve4;
        }
    
        //-------------------------------------------------------------is_end_poly
        inline bool is_end_poly(unsigned c)
        {
            return (c & path_cmd_mask) == path_cmd_end_poly;
        }
    
        //----------------------------------------------------------------is_close
        inline bool is_close(unsigned c)
        {
            return (c & ~(path_flags_cw | path_flags_ccw)) ==
                   (path_cmd_end_poly | path_flags_close); 
        }
    
        //------------------------------------------------------------is_next_poly
        inline bool is_next_poly(unsigned c)
        {
            return is_stop(c) || is_move_to(c) || is_end_poly(c);
        }
    
        //-------------------------------------------------------------------is_cw
        inline bool is_cw(unsigned c)
        {
            return (c & path_flags_cw) != 0;
        }
    
        //------------------------------------------------------------------is_ccw
        inline bool is_ccw(unsigned c)
        {
            return (c & path_flags_ccw) != 0;
        }
    
        //-------------------------------------------------------------is_oriented
        inline bool is_oriented(unsigned c)
        {
            return (c & (path_flags_cw | path_flags_ccw)) != 0; 
        }
    
        //---------------------------------------------------------------is_closed
        inline bool is_closed(unsigned c)
        {
            return (c & path_flags_close) != 0; 
        }
    
        grid_rasterizer() :
            m_filling_rule(fill_non_zero) { }

        //--------------------------------------------------------------------
        void reset() { m_outline.reset(); }

        //--------------------------------------------------------------------
        void filling_rule(filling_rule_e filling_rule) 
        { 
            m_filling_rule = filling_rule; 
        }


        //--------------------------------------------------------------------
        void move_to(int x, int y) { m_outline.move_to(x, y); }
        void line_to(int x, int y) { m_outline.line_to(x, y); }

        //--------------------------------------------------------------------
        void move_to_d(double x, double y) { m_outline.move_to(poly_coord(x), 
                                                               poly_coord(y)); }
        void line_to_d(double x, double y) { m_outline.line_to(poly_coord(x), 
                                                               poly_coord(y)); }

        /*void close_polygon()
        {
            if(m_status == status_line_to)
            {
                m_clipper.line_to(m_outline, m_start_x, m_start_y);
                m_status = status_closed;
            }
        }*/

        void add_vertex(double x, double y, unsigned cmd)
        {
            if(is_move_to(cmd)) 
            {
                //std::clog << "move_to x: " << x << " y: " << y << "\n";
                move_to_d(x, y);
            }
            else 
            if(is_vertex(cmd))
            {
                //std::clog << "line_to x: " << x << " y: " << y << "\n";
                line_to_d(x, y);
            }
            /*else
            if(is_close(cmd))
            {
                m_clipper.line_to(m_outline, m_start_x, m_start_y);
            }*/
        }

        //-------------------------------------------------------------------
        template<class VertexSource>
        void add_path(VertexSource& vs, unsigned path_id=0)
        {
            double x;
            double y;

            unsigned cmd;
            vs.rewind(path_id);
            //if(m_outline.sorted()) reset();
            while(!is_stop(cmd = vs.vertex(&x, &y)))
            {
                add_vertex(x, y, cmd);
            }
        }

        //--------------------------------------------------------------------
        int min_x() const { return m_outline.min_x(); }
        int min_y() const { return m_outline.min_y(); }
        int max_x() const { return m_outline.max_x(); }
        int max_y() const { return m_outline.max_y(); }

        //--------------------------------------------------------------------
        unsigned calculate_alpha(int area) const
        {
            
            int cover = area >> (poly_base_shift*2 + 1 - aa_shift);

            if(cover < 0) cover = -cover;
            if(m_filling_rule == fill_even_odd)
            {
                cover &= aa_2mask;
                if(cover > aa_num)
                {
                    cover = aa_2num - cover;
                }
            }
            if(cover > aa_mask) cover = aa_mask;
            return cover;
        }

        //--------------------------------------------------------------------
        // here
        template<class Renderer> void render(Renderer& r, 
                                             grid_value c, 
                                             int dx=0, 
                                             int dy=0)
        {
            const cell* const* cells = m_outline.cells();
            if(m_outline.num_cells() == 0) {
                return;
            }

            int x, y;
            int cover;
            int alpha;
            int area;

            m_scanline.reset(m_outline.min_x(), m_outline.max_x(), dx, dy);

            cover = 0;
            const cell* cur_cell = *cells++;
            for(;;)
            {
                const cell* start_cell = cur_cell;

                int coord  = cur_cell->packed_coord;
                x = cur_cell->x;
                y = cur_cell->y;

                area   = start_cell->area;
                cover += start_cell->cover;

                //accumulate all start cells
                while((cur_cell = *cells++) != 0)
                {
                    if(cur_cell->packed_coord != coord) break;
                    area  += cur_cell->area;
                    cover += cur_cell->cover;
                }

                if(area)
                {
                    alpha = calculate_alpha((cover << (poly_base_shift + 1)) - area);
                    if(alpha)
                    {
                        if(m_scanline.is_ready(y))
                        {
                            r.render(m_scanline, c);
                            m_scanline.reset_spans();
                        }
                        m_scanline.add_cell(x, y, 0);
                    }
                    x++;
                }

                if(!cur_cell) break;

                if(cur_cell->x > x)
                {
                    alpha = calculate_alpha(cover << (poly_base_shift + 1));
                    if(alpha)
                    {
                        if(m_scanline.is_ready(y))
                        {
                            r.render(m_scanline, c);
                            m_scanline.reset_spans();
                        }
                        m_scanline.add_span(x, y, 
                                            cur_cell->x - x, 
                                            0);
                    }
                }
            } 
        
            if(m_scanline.num_spans())
            {
                r.render(m_scanline, c);
            }
        }

        //------------------------------------------------------------------------
        bool hit_test(int tx, int ty)
        {
            const cell* const* cells = m_outline.cells();
            if(m_outline.num_cells() == 0) return false;
    
            int x, y;
            int cover;
            int alpha;
            int area;
    
            cover = 0;
            const cell* cur_cell = *cells++;
            for(;;)
            {
                const cell* start_cell = cur_cell;
    
                int coord  = cur_cell->packed_coord;
                x = cur_cell->x;
                y = cur_cell->y;
    
                if(y > ty) return false;
    
                area   = start_cell->area;
                cover += start_cell->cover;
    
                while((cur_cell = *cells++) != 0)
                {
                    if(cur_cell->packed_coord != coord) break;
                    area  += cur_cell->area;
                    cover += cur_cell->cover;
                }
    
                if(area)
                {
                    alpha = calculate_alpha((cover << (poly_base_shift + 1)) - area);
                    if(alpha)
                    {
                        if(tx == x && ty == y) return true;
                    }
                    x++;
                }
    
                if(!cur_cell) break;
    
                if(cur_cell->x > x)
                {
                    alpha = calculate_alpha(cover << (poly_base_shift + 1));
                    if(alpha)
                    {
                        if(ty == y && tx >= x && tx <= cur_cell->x) return true;
                    }
                }
            }
            return false;
        }
    
    private:
        grid_rasterizer(const grid_rasterizer&);
        const grid_rasterizer& operator = (const grid_rasterizer&);

    private:
        outline        m_outline;
        scanline       m_scanline;
        filling_rule_e m_filling_rule;
    };

    //========================================================================
    // here
    struct span_grid
    {

        //--------------------------------------------------------------------
        static void render(grid_value* ptr, 
                           int x,
                           unsigned count, 
                           const unsigned char* covers, 
                           grid_value c)
        {
            grid_value* p = ptr + x;
            do { *p++ = c; }
            while(--count);
        }

        //--------------------------------------------------------------------
        static void hline(grid_value* ptr, 
                          int x,
                          unsigned count, 
                          grid_value c)
        {
            grid_value* p = ptr + x;
            do { *p++ = c; }
            while(--count);
        }

        //--------------------------------------------------------------------
        static grid_value get(grid_value* ptr, int x)
        {
            return ptr[x];
            
        }
    };    
}



#endif