#include <ing/timing.hpp>

using namespace ing;

namespace
{
    void dummy_signal_handler(const char*, int) {}
    auto* signal_handler = dummy_signal_handler;
}

timer::~timer() noexcept(false)
{
    stage = 0;
    finalize();
}

timer::timer(std::string_view name,
             logger& logger,
             logging::severity_level level,
             const source_location& loc)
    : uptime(name, loc), log(&logger), mt(false),
      level((int)level), basename(name.size()), stage(1)
{
    signal_handler(this->name.c_str(), 1);
}

timer::timer(std::string_view name,
             logger_mt& logger,
             logging::severity_level level,
             const source_location& loc)
    : uptime(name, loc), log(&logger), mt(true),
      level((int)level), basename(name.size()), stage(1)
{
    signal_handler(this->name.c_str(), 1);
}

timer::timer(std::string_view name,
             logger& logger,
             const source_location& loc)
    : timer(name, logger, logger.default_severity(), loc)
{
}

timer::timer(std::string_view name,
             logger_mt& logger,
             const source_location& loc)
    : timer(name, logger, logger.default_severity(), loc)
{
}

timer::timer(std::string_view name,
             logging::severity_level level,
             const source_location& loc)
    : timer(name, global_logger::get(), level, loc)
{
}

timer::timer(std::string_view name,
             const source_location& loc)
    : timer(name, global_logger::get(), loc)
{
}

void timer::report(const std::string& name, const source_location& loc)
{
    struct string_guard
    {
        char& c;
        const char b;
        string_guard(char& c, char b) : c(c), b(c) { c = b; }
        ~string_guard() { c = b; }
    };

    if (stage == 0) // reset signal before exceptional logging
    {
        string_guard _(this->name[basename], '\0');
        signal_handler(this->name.c_str(), 0);
    }

    if (mt)
    {
        if (auto h = static_cast<logger_mt*>(log)->log(static_cast<logging::severity_level>(level), loc))
            format(h.stream() << name << ' ');
    }
    else
    {
        if (auto h = static_cast<logger*>(log)->log(static_cast<logging::severity_level>(level), loc))
            format(h.stream() << name << ' ');
    }

    if (stage > 0)
    {
        string_guard _(this->name[basename], '\0');
        signal_handler(this->name.c_str(), ++stage);
    }
}

void timer::signal(void (*sig)(const char*, int)) noexcept
{
    if (sig == nullptr) sig = dummy_signal_handler;
    signal_handler = sig;
}
