#include <ing/logging.hpp>

namespace ing::logging
{
    namespace
    {
        const char level_str[6][6] =
        {
            {'T','R','A','C','E',0},
            {'D','E','B','U','G',0},
            {'I','N','F','O',0},
            {'W','A','R','N',0},
            {'E','R','R','O','R',0},
            {'F','A','T','A','L',0},
        };
    }

    const char* to_string(severity_level level) noexcept
    {
        auto ordinal = static_cast<unsigned>(level);
        if (ordinal < 6) return level_str[ordinal];
        return nullptr;
    }

    bool from_string(std::string_view str, severity_level& level) noexcept
    {
        int ordinal = -1;
        using char_traits = std::char_traits<char>;
        if (str.size() == 5)
        {
            if (char_traits::compare(str.data(), level_str[0], 5) == 0)
                ordinal = 0;
            else if (char_traits::compare(str.data(), level_str[1], 5) == 0)
                ordinal = 1;
            else if (char_traits::compare(str.data(), level_str[4], 5) == 0)
                ordinal = 4;
            else if (char_traits::compare(str.data(), level_str[5], 5) == 0)
                ordinal = 5;
        }
        else if (str.size() == 4)
        {
            if (char_traits::compare(str.data(), level_str[2], 4) == 0)
                ordinal = 2;
            else if (char_traits::compare(str.data(), level_str[3], 4) == 0)
                ordinal = 3;
        }
        if (ordinal >= 0)
        {
            level = static_cast<severity_level>(ordinal);
            return true;
        }
        return false;
    }

    std::ostream& operator<<(std::ostream& os, severity_level level)
    {
        const char* str = to_string(level);
        if (str) os << str;
        else os << static_cast<int>(level);
        return os;
    }

    std::istream& operator>>(std::istream& is, severity_level& level)
    {
        std::string val;
        if (is >> val && !from_string(val, level))
            throw std::invalid_argument(val);
        return is;
    }
}

namespace ing
{
    BOOST_LOG_GLOBAL_LOGGER_CTOR_ARGS(global_logger, logger_mt, ("global"))
}

