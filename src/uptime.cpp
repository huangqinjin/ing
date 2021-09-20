#include <ing/uptime.hpp>

#include <cassert>

using namespace ing;


uptime::~uptime() noexcept(false)
{
    finalize();
}

uptime::uptime(std::string_view name, const source_location& loc)
    : loc(loc), total(*this)
{
    this->name.reserve(name.size() + 1 + 8);
    this->name.append(name);
}

void uptime::phase(std::string_view id, const source_location& loc)
{
    auto_cpu_timer::stop();

    std::size_t len = 0;
    if (!id.empty())
    {
        len = name.size();
        name.push_back('/');
        name.append(id);
    }

    report(name, loc);

    if (!id.empty())
    {
        name.resize(len);
        auto_cpu_timer::start();
    }
}

void uptime::format(std::ostream& os)
{
    struct guard
    {
        struct unbox : cpu_timer
        {
            short           m_places;
            std::ostream*   m_os;
            std::string     m_format;
        } & b;

        auto_cpu_timer& t;
        std::ostream* const old;

        guard(auto_cpu_timer& t, std::ostream& os)
            : b(reinterpret_cast<unbox&>(t)),
              t(t), old(&t.ostream())
        {
            static_assert(sizeof(unbox) == sizeof(t));
            assert(b.m_os == old);
            b.m_os = &os;
        }

        ~guard()
        {
            b.m_os = old;
        }
    } _(*this, os);

    auto_cpu_timer::report();
}

void uptime::finalize()
{
    if (!is_stopped())
    {
        static_cast<cpu_timer&>(*this) = total;
        phase({}, loc);
    }
}

void uptime::report(const std::string& name, const source_location& loc)
{
    format(ostream() << loc << ' ' << name);
}
