#ifdef __linux__

#ifdef MAPNIK_ENABLE_GLIBC_WORKAROUND

#include <stdexcept>

// https://github.com/bitcoin/bitcoin/pull/4042
// allows building against libstdc++-dev-4.9 while avoiding
// GLIBCXX_3.4.20 dep
// This is needed because libstdc++ itself uses this API - its not
// just an issue of your code using it, ughhh

namespace std
{

void __throw_out_of_range_fmt(const char *, ...) __attribute__((__noreturn__));
void __throw_out_of_range_fmt(const char *err, ...)
{
    // Safe and over-simplified version. Ignore the format and print it as-is.
    __throw_out_of_range(err);
}
}

#endif // MAPNIK_ENABLE_GLIBC_WORKAROUND

#endif // __linux__