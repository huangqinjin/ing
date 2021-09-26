#ifndef ING_LOGGING_HPP
#define ING_LOGGING_HPP

#include <ostream>
#include <istream>
#include <string_view>

namespace ing::logging
{
    enum class severity_level
    {
        trace,
        debug,
        info,
        warn,
        error,
        fatal,
    };

    const char* to_string(severity_level level) noexcept;
    bool from_string(std::string_view str, severity_level& level) noexcept;
    std::ostream& operator<<(std::ostream& os, severity_level level);
    std::istream& operator>>(std::istream& is, severity_level& level);
}


#endif
