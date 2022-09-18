#define BOOST_THREAD_PROVIDES_SHARED_MUTEX_UPWARDS_CONVERSIONS

#include <ing/timing.hpp>
#include <ing/spinlock.hpp>

#include <boost/intrusive/avl_set.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/accumulators/framework/accumulator_set.hpp>
#include <boost/accumulators/statistics.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/io/ios_state.hpp>
#include <boost/type_index.hpp>

#include <regex>

using namespace ing;

namespace
{
    void dummy_signal_handler(const char*, int) {}
    auto* signal_handler = dummy_signal_handler;

    struct element : boost::intrusive::avl_set_base_hook<
            boost::intrusive::link_mode<boost::intrusive::normal_link>>
    {
        spinlock guard;

        struct stdev : boost::accumulators::depends_on<boost::accumulators::tag::variance>
        {
            template<typename Sample>
            struct stdev_impl : boost::accumulators::accumulator_base
            {
                typedef typename boost::numeric::functional::fdiv<Sample, std::size_t>::result_type result_type;

                stdev_impl(boost::accumulators::dont_care) {}

                template<typename Args>
                result_type result(Args const &args) const
                {
                    using namespace std;
                    return sqrt(boost::accumulators::variance(args));
                }
            };

            typedef stdev_impl<boost::mpl::_1> impl;
        };

        using features = boost::accumulators::stats<
                boost::accumulators::tag::count,
                boost::accumulators::tag::min,
                boost::accumulators::tag::max,
                boost::accumulators::tag::median,
                boost::accumulators::tag::mean,
                stdev,
                boost::accumulators::tag::variance>;

        boost::accumulators::accumulator_set<double, features>
                wall, user, system;

        const std::size_t len;
        char key[1];

        explicit element(std::size_t len) noexcept : len(len) {}

        std::size_t bytes() const noexcept
        {
            return key - reinterpret_cast<const char*>(this) + len + 1;
        }
    };

    struct name
    {
        using type = std::string_view;
        type operator()(const element& e) const noexcept { return { e.key, e.len }; }
    };

    struct set : boost::intrusive::avl_set<element,
        boost::intrusive::key_of_value<name>,
        boost::intrusive::constant_time_size<false>>
    {
        boost::upgrade_mutex guard;
        boost::condition_variable_any cv;
    };

    set timers;
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

    element* p = nullptr;
    // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2012/n3427.html
    for (auto search = boost::shared_lock<boost::upgrade_mutex>(timers.guard);;)
    {
        set::insert_commit_data ctx;
        auto r = timers.insert_check(name, ctx);
        if (!r.second)
        {
            p = &*r.first;
            break;
        }

        if (auto create = boost::upgrade_lock<boost::upgrade_mutex>(std::move(search), boost::try_to_lock))
        {
            struct notifier : std::unique_ptr<element>
            {
                ~notifier()
                {
                    timers.cv.notify_all();
                }
            } n;

            p = new (operator new(element(name.size()).bytes())) element(name.size());
            n.reset(p);

            std::memcpy(p->key, name.data(), name.size());
            p->key[p->len] = '\0';

            auto insert = boost::unique_lock<boost::upgrade_mutex>(std::move(create));
            timers.insert_commit(*p, ctx);
            (void) n.release();
        }
        else
        {
            timers.cv.wait(search);
        }
        break;
    }

    if (p)
    {
        auto times = elapsed();
        auto _ = boost::lock_guard<spinlock>(p->guard);
        p->wall(times.wall * 1e-9);
        p->user(times.user * 1e-9);
        p->system(times.system * 1e-9);
    }
}

void timer::signal(void (*sig)(const char*, int)) noexcept
{
    if (sig == nullptr) sig = dummy_signal_handler;
    signal_handler = sig;
}

void timer::report(std::ostream& os, std::string_view regex)
{
    std::regex re;
    if (!regex.empty())
    {
        re.assign(regex.data(), regex.size());
    }
    else
    {
        os << "name" << '\t' << "channel";
        boost::mpl::for_each<element::features>([&](auto f) {
            std::string name = boost::typeindex::type_id<decltype(f)>().pretty_name();
            auto i = name.rfind(':');
            if (i != std::string::npos)
                name.erase(0, i + 1);
            os << '\t' << name;
        });
        os << '\n';
    }

    boost::io::ios_flags_saver ifs(os);
    boost::io::ios_precision_saver ips(os);
    os.setf(std::ios_base::fixed, std::ios_base::floatfield);
    os.precision(6);

    auto search = boost::shared_lock<boost::upgrade_mutex>(timers.guard);
    for (auto iter = timers.begin(); iter != timers.end(); search.lock(), ++iter)
    {
        search.unlock();

        if (!regex.empty() && !std::regex_match(iter->key, iter->key + iter->len, re))
            continue;

        const char* const names[] = { "wall", "user", "system" };
        decltype(&iter->wall) const stats[] = { &iter->wall, &iter->user, &iter->system };
        for (std::size_t i = 0; i < std::size(names); ++i)
        {
            os << iter->key << '\t' << names[i];

            boost::mpl::for_each<element::features>([&](auto f) {
                boost::unique_lock<spinlock> guard(iter->guard);
                auto r = boost::accumulators::extract_result<decltype(f)>(*stats[i]);
                guard.unlock();
                os << '\t' << r;
            });

            os << '\n';
        }
    }
}
