#ifndef ING_TIMING_HPP
#define ING_TIMING_HPP

#include "uptime.hpp"
#include "logging.hpp"

namespace ing
{
    class timer : public uptime
    {
        void* const log;
        const bool mt;
        const signed char level;
        const unsigned short basename;
        int stage;

        void report(const std::string& name, const source_location& loc) override;

    public:
        ~timer() noexcept(false);

        timer(std::string_view name,
              logger& logger,
              logging::severity_level level,
              const source_location& loc = source_location::current());

        timer(std::string_view name,
              logger_mt& logger,
              logging::severity_level level,
              const source_location& loc = source_location::current());

        timer(std::string_view name,
              logger& logger,
              const source_location& loc = source_location::current());

        timer(std::string_view name,
              logger_mt& logger,
              const source_location& loc = source_location::current());

        timer(std::string_view name,
              logging::severity_level level,
              const source_location& loc = source_location::current());

        explicit
        timer(std::string_view name,
              const source_location& loc = source_location::current());

    public:
        static void signal(void (*sig)(const char*, int)) noexcept;
        static void report(std::ostream& os, std::string_view regex = {});
    };
}

#endif
