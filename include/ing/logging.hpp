#ifndef ING_LOGGING_HPP
#define ING_LOGGING_HPP

#include <ostream>
#include <istream>
#include <string_view>
#include <type_traits>

#include <boost/log/keywords/log_source.hpp>
#include <boost/log/sources/severity_feature.hpp>
#include <boost/log/sources/channel_feature.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/mp11/map.hpp>

#include "source_location.hpp"


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

namespace ing::logging::attributes
{
    // Refer to:
    // boost::log::sources::aux::severity_level
    // boost::log::attributes::current_thread_id
    // boost::log::attributes::function
    // boost::log::attributes::constant
    // boost::log::attributes::mutable_constant
    template<typename Val, typename Gen = Val[]>
    class thread_specific : public boost::log::attribute
    {
    public:
        using value_type = Val;

    protected:
        struct val
        {
            using held = Val;
            static inline thread_local Val v;
            Val& value() noexcept { return v; }
            Val& get() noexcept { return v; }
            template<typename ...Args>
            explicit val(Args&&... args) { v = Val(std::forward<Args>(args)...); }
        };

        struct gen
        {
            using held = Gen;
            Gen value;
            Gen& get() noexcept { return value; }
            template<typename ...Args>
            explicit gen(Args&&... args) : value(std::forward<Args>(args)...) {}
        };

        using holder = std::conditional_t<std::is_invocable_r_v<Val, Gen>, gen, val>;

        class impl : public boost::log::attribute_value::impl,
                     public holder
        {
        public:
            template<typename ...Args>
            explicit impl(Args&&... args) : holder(std::forward<Args>(args)...) {}

            bool dispatch(boost::log::type_dispatcher& dispatcher) override
            {
                if (auto callback = dispatcher.get_callback<value_type>())
                {
                    callback(this->value());
                    return true;
                }
                return false;
            }

            boost::intrusive_ptr<boost::log::attribute_value::impl> detach_from_thread() override
            {
                using detached_value = boost::log::attributes::attribute_value_impl<value_type>;
                return new detached_value(this->value());
            }
        };

    public:
        using boost::log::attribute::attribute;

        thread_specific() : boost::log::attribute(new impl) {}

        template<typename ...Args>
        explicit thread_specific(Args&&... args)
            : boost::log::attribute(new impl(std::forward<Args>(args)...)) {}

        explicit thread_specific(boost::log::attributes::cast_source const& source)
            : boost::log::attribute(source.as<impl>()) {}

        typename holder::held& get() noexcept
        {
            return static_cast<impl*>(this->get_impl())->get();
        }

        typename holder::held const& get() const noexcept
        {
            return static_cast<impl*>(this->get_impl())->get();
        }

        template<typename ...Args>
        void set(Args&&... args)
        {
            get() = typename holder::held(std::forward<Args>(args)...);
        }
    };
}

namespace ing::logging::sources
{
    /**
     * @brief boost::severity_feature will set the severity of the record to default severity if users
     * do not specify. ing::severity_feature will not open the record if users do not specify severity
     * or the specified severity is lower than default severity. Consider it as a source filter.
     */
    template<typename BaseT, typename LevelT>
    class basic_severity_logger : public boost::log::sources::basic_severity_logger<BaseT, LevelT>
    {
        using base_type = boost::log::sources::basic_severity_logger<BaseT, LevelT>;

    public:
        using base_type::base_type;

    protected:
        template<typename ArgsT>
        boost::log::record open_record_unlocked(ArgsT const& args)
        {
            return open_record_with_severity_unlocked(args,
                   args[boost::log::keywords::severity | boost::parameter::void_()]);
        }

    private:
        template<typename ArgsT>
        boost::log::record open_record_with_severity_unlocked(ArgsT const& args, LevelT const& level)
        {
            if (level < this->default_severity()) return {};
            return base_type::open_record_unlocked(args);
        }

        template<typename ArgsT>
        boost::log::record open_record_with_severity_unlocked(ArgsT const& args, boost::parameter::void_)
        {
            return {};
        }
    };

    template<typename LevelT>
    struct severity
    {
        template<typename BaseT>
        struct apply
        {
            typedef basic_severity_logger<BaseT, LevelT> type;
        };
    };

    // Refer to:
    // boost::log::sources::basic_severity_logger
    // boost::log::sources::basic_channel_logger
    template<typename BaseT, typename LocationT>
    class basic_location_logger : public BaseT
    {
    public:
        using location_attribute = attributes::thread_specific<LocationT>;

    private:
        using base_type = BaseT;
        location_attribute location;

    public:
        basic_location_logger()
        {
            base_type::add_attribute_unlocked(boost::log::aux::default_attribute_names::line_id(), location);
        }

        basic_location_logger(basic_location_logger const& that)
            : base_type(static_cast<const base_type&>(that))
        {
            // Our attributes must refer to our severity attribute
            base_type::attributes()[boost::log::aux::default_attribute_names::line_id()] = location;
        }

        basic_location_logger(basic_location_logger&& that) noexcept
            : base_type(static_cast<base_type&&>(that)), location(std::move(that.location))
        {

        }

        template< typename ArgsT >
        explicit basic_location_logger(ArgsT const& args) : base_type(args)
        {
            base_type::add_attribute_unlocked(boost::log::aux::default_attribute_names::line_id(), location);
        }

    protected:
        template<typename ArgsT>
        boost::log::record open_record_unlocked(ArgsT const& args)
        {
            return open_record_with_location_unlocked(args,
                   args[boost::log::keywords::log_source | boost::parameter::void_()]);
        }

        void swap_unlocked(basic_location_logger& that)
        {
            base_type::swap_unlocked(static_cast<base_type&>(that));
            location.swap(that.location);
        }

    private:
        template<typename ArgsT, typename T>
        boost::log::record open_record_with_location_unlocked(ArgsT const& args, T const& loc)
        {
            location.set(loc);
            return base_type::open_record_unlocked(args);
        }

        template<typename ArgsT>
        boost::log::record open_record_with_location_unlocked(ArgsT const& args, boost::parameter::void_)
        {
            return base_type::open_record_unlocked(args);
        }
    };

    template<typename LocationT>
    struct location
    {
        template<typename BaseT>
        struct apply
        {
            typedef basic_location_logger<BaseT, LocationT> type;
        };
    };
}

namespace ing::logging::sources
{
    template<typename CharT, typename ThreadingModelT, typename LevelT, typename ChannelT, typename LocationT>
    class basic_severity_channel_location_logger :
        public boost::log::sources::basic_composite_logger<
            CharT,
            basic_severity_channel_location_logger<CharT, ThreadingModelT, LevelT, ChannelT, LocationT>,
            ThreadingModelT,
            boost::log::sources::features<
                severity<LevelT>,
                boost::log::sources::channel<ChannelT>,
                location<LocationT>
            >
        >
    {
        BOOST_LOG_FORWARD_LOGGER_MEMBERS_TEMPLATE(basic_severity_channel_location_logger)

    public:
        boost::log::record open_record(LocationT location = LocationT::current())
        {
            return open_record(boost::parameter::aux::empty_arg_list(), location);
        }

        template<typename ArgsT>
        boost::log::record open_record(ArgsT const& args, LocationT location = LocationT::current())
        {
            using base_type = typename basic_severity_channel_location_logger::logger_base;
            if constexpr(boost::mp11::mp_map_contains<ArgsT, boost::log::keywords::tag::log_source>::value)
                return base_type::open_record(args);
            else
                return base_type::open_record((args, boost::log::keywords::log_source = location));
        }
    };
}

namespace ing
{
    template<typename ThreadingModelT>
    class basic_logger : public logging::sources::basic_severity_channel_location_logger<
            char, ThreadingModelT, logging::severity_level, std::string, source_location>
    {
    public:
        using logger_base = typename basic_logger::final_type;
        using typename logger_base::severity_level;

        template<typename ...Args>
        explicit basic_logger(typename logger_base::channel_type channel,
                              severity_level min_level = severity_level::trace,
                              Args&&... args)
            : logger_base(boost::log::keywords::channel = std::move(channel),
                          boost::log::keywords::severity = min_level,
                          std::forward<Args>(args)...) {}
    };

    using logger = basic_logger<boost::log::sources::logger::threading_model>;
    using logger_mt = basic_logger<boost::log::sources::logger_mt::threading_model>;

    BOOST_LOG_GLOBAL_LOGGER(global_logger, logger_mt)

    void init_logging();
}

// Generic logging macro with specified logger instance and dynamic severity
#define ING_LOG_SEV(logger, sev) BOOST_LOG_STREAM_WITH_PARAMS((logger), \
                                 (::boost::log::keywords::severity = (sev)) \
                                 (::boost::log::keywords::log_source = ING_CURRENT_LOCATION()))

// Generic logging macro with specified logger instance and static severity
#define ING_LOG(logger, sev) ING_LOG_SEV((logger), ::ing::logging::severity_level::sev)


// Global logging macro with dynamic severity
#define ING_GLOG_SEV(sev) ING_LOG_SEV(::ing::global_logger::get(), (sev))

// Global logging macro with static severity
#define ING_GLOG(sev) ING_GLOG_SEV(::ing::logging::severity_level::sev)


#ifndef ING_LOCAL_LOGGER
#define ING_LOCAL_LOGGER logger
#endif

// Local logging macro with dynamic severity
#define ING_LLOG_SEV(sev) ING_LOG_SEV((ING_LOCAL_LOGGER), (sev))

// Local logging macro with static severity
#define ING_LLOG(sev) ING_LLOG_SEV(::ing::logging::severity_level::sev)


#endif
