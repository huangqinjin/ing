#ifndef ING_UPTIME_HPP
#define ING_UPTIME_HPP

#include <boost/timer/timer.hpp>

#include <string_view>

#include "source_location.hpp"

namespace ing
{
    class uptime : public boost::timer::auto_cpu_timer
    {
    public:
        ~uptime() noexcept(false);
        explicit uptime(std::string_view name, const source_location& loc = source_location::current());
        void phase(std::string_view id, const source_location& loc = source_location::current());

    protected:
        void format(std::ostream& os);
        void finalize();

        std::string name;
        const source_location loc;
        boost::timer::cpu_timer total;

    private:
        virtual void report(const std::string& name, const source_location& loc);
    };
}

#endif
