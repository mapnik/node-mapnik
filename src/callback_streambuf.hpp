#ifndef CALLBACK_STREAMBUF_H
#define CALLBACK_STREAMBUF_H

#include <array>
#include <iostream>

template<typename Callback, class Char = char>
class callback_streambuf : public std::basic_streambuf<Char>
{
public:
    using base_type = std::streambuf;
    using char_type = typename base_type::char_type;
    using int_type = typename base_type::int_type;

    callback_streambuf(Callback callback, std::size_t buffer_size)
        : callback_(callback),
          buffer_(buffer_size)
    {
        base_type::setp(buffer_.data(), buffer_.data() + buffer_.size());
    }

protected:
    int sync()
    {
        bool ok = callback_(base_type::pbase(),
                            base_type::pptr() - base_type::pbase());
        base_type::setp(buffer_.data(), buffer_.data() + buffer_.size());
        return ok ? 0 : -1;
    }

    int_type overflow(int_type ch)
    {
        int ret = sync();
        if (ch == base_type::traits_type::eof())
        {
            return ch;
        }
        base_type::sputc(ch);
        return ret == 0 ? 0 : base_type::traits_type::eof();
    }

private:
    Callback callback_;
    std::vector<char_type> buffer_;
};

#endif
