#ifndef ING_SOURCE_LOCATION_HPP
#define ING_SOURCE_LOCATION_HPP

#if __has_include(<version>)
# include <version>
#endif

#include <cstdint>

#if !defined(ING_NO_STD_SOURCE_LOCATION) && defined(__cpp_lib_source_location)
# include <source_location>
#endif

#if !defined(ING_NO_STD_SOURCE_LOCATION) && !defined(__cpp_lib_source_location) && \
  __has_include(<experimental/source_location>)
# include <experimental/source_location>
#endif

#include <boost/assert/source_location.hpp>


namespace ing
{
    /**
     * @brief std::source_location introduced in C++20 is immutable.
     * ing::source_location is initialized from (std|boost)::source_location and is modifiable.
     * ing::source_location is more flexible as function parameters compared to std::source_location.
     */
    struct source_location : boost::source_location
    {
        static constexpr source_location current(
#if defined(ING_NO_STD_SOURCE_LOCATION)
            boost::source_location loc = BOOST_CURRENT_LOCATION
#elif defined(__cpp_lib_source_location)
            std::source_location loc = std::source_location::current()
#elif defined(__cpp_lib_experimental_source_location)
            std::experimental::source_location loc = std::experimental::source_location::current()
#else
#define ING_NO_STD_SOURCE_LOCATION
            boost::source_location loc = BOOST_CURRENT_LOCATION
#endif
        ) noexcept
        {
            return source_location{{loc.file_name(), loc.line(), loc.function_name(), loc.column()}};
        }

        using boost::source_location::file_name;
        constexpr source_location& file_name(const char* file) noexcept
        {
            boost::source_location& current = *this;
            current = boost::source_location(file, current.line(), current.function_name(), current.column());
            return *this;
        }

        using boost::source_location::line;
        constexpr source_location& line(std::uint_least32_t line) noexcept
        {
            boost::source_location& current = *this;
            current = boost::source_location(current.file_name(), line, current.function_name(), current.column());
            return *this;
        }

        using boost::source_location::function_name;
        constexpr source_location& function_name(const char* function) noexcept
        {
            boost::source_location& current = *this;
            current = boost::source_location(current.file_name(), current.line(), function, current.column());
            return *this;
        }

        using boost::source_location::column;
        constexpr source_location& column(std::uint_least32_t column) noexcept
        {
            boost::source_location& current = *this;
            current = boost::source_location(current.file_name(), current.line(), current.function_name(), column);
            return *this;
        }
    };
}

#ifdef ING_NO_STD_SOURCE_LOCATION
#define ING_CURRENT_LOCATION() ::ing::source_location::current(BOOST_CURRENT_LOCATION)
#else
#define ING_CURRENT_LOCATION() ::ing::source_location::current()
#endif

#endif
