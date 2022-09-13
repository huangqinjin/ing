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

#if defined(ING_WITH_FMT)
#define ING_HAS_FMT
#include <fmt/format.h>

namespace ing::fmt
{
    using ::fmt::format;
    using ::fmt::vformat_to;
    using ::fmt::string_view;
    using ::fmt::format_args;
    using ::fmt::make_format_args;

    template<typename ...Args>
#if defined(__cpp_consteval) && defined(FMT_HAS_CONSTEVAL)
    using format_string = ::fmt::format_string<Args...>;
#else
    using format_string = ::fmt::string_view;
#endif
}

#elif defined(__cpp_lib_format)
#define ING_HAS_FMT
#include <format>

namespace ing::fmt
{
    using std::format;
    using std::vformat_to;
    using std::string_view;
    using std::format_args;
    using std::make_format_args;

    template<typename ...Args>
#if defined(__cpp_consteval) && __cpp_lib_format >= 202207L
    using format_string = std::format_string<Args...>;
#else
    using format_string = std::string_view;
#endif
}
#endif

#ifdef ING_HAS_FMT
namespace ing::fmt
{
    template<typename ...Args>
    struct format_location
    {
        format_string<Args...> format;
        source_location location;

        template<typename S, typename = std::enable_if_t<std::is_convertible_v<const S&, string_view>>>
#if defined(__cpp_consteval)
        consteval
#else
        constexpr
#endif
        format_location(const S& s, source_location loc = source_location::current())
            : format(s), location(loc) {}

        format_location(const format_location&) = default;

        string_view get() const noexcept
        {
            return [](auto f) {
                if constexpr (std::is_convertible_v<decltype(f), string_view>)
                    return f;
                else
                    return f.get();
            }(format);
        }
    };

    template<typename ...Args>
    using fmtloc_t = std::enable_if_t<
                        !std::disjunction_v<std::is_convertible<Args, format_args>...>,
                        format_location<Args...>>;
}
#endif


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
    severity_level minimum_severity_level(std::string_view channel);
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
        class helper
        {
            friend basic_logger;
            using base = boost::log::aux::record_pump<basic_logger>;
            boost::log::record record;
            union { base pump; };

            helper(basic_logger& lg, boost::log::record rec)
                : record(std::move(rec))
            {
                if (record) new ((void*)std::addressof(pump)) base(lg, record);
            }

#ifdef ING_HAS_FMT
            helper(basic_logger& lg, boost::log::record rec,
                   fmt::string_view fmt, fmt::format_args args)
                    : helper(lg, std::move(rec))
            {
                if (record) fmt::vformat_to(std::ostreambuf_iterator<char>(stream()), fmt, args);
            }
#endif

        public:
            helper(const helper&) = delete;
            helper& operator=(const helper&) = delete;
            ~helper() noexcept(false) { if (record) pump.~base(); }
            explicit operator bool() const noexcept { return !!record; }
            std::ostream& stream() noexcept { return pump.stream().stream(); }

            template<typename T>
            helper& operator<<(const T& t)
            {
                if (record) pump.stream() << t;
                return *this;
            }

            helper& operator<<(std::ios_base& (*manip)(std::ios_base&))
            {
                if (record) pump.stream() << manip;
                return *this;
            }

            helper& operator<<(std::ios& (*manip)(std::ios&))
            {
                if (record) pump.stream() << manip;
                return *this;
            }

            helper& operator<<(std::ostream& (*manip)(std::ostream&))
            {
                if (record) pump.stream() << manip;
                return *this;
            }
        };

    public:
        using logger_base = typename basic_logger::final_type;
        using typename logger_base::severity_level;

        template<typename ...Args>
        basic_logger(typename logger_base::channel_type channel,
                     severity_level min_level, Args&&... args)
            : logger_base(boost::log::keywords::channel = std::move(channel),
                          boost::log::keywords::severity = min_level,
                          std::forward<Args>(args)...) {}

        template<typename ...Args>
        explicit basic_logger(typename logger_base::channel_type channel, Args&&... args)
            : basic_logger(std::move(channel), logging::minimum_severity_level(channel),
                           std::forward<Args>(args)...) {}

        bool enabled(severity_level level) const noexcept
        {
            return level >= this->default_severity();
        }

        bool is_trace_enabled() const noexcept
        {
            return enabled(severity_level::trace);
        }

        bool is_debug_enabled() const noexcept
        {
            return enabled(severity_level::debug);
        }

        bool is_info_enabled() const noexcept
        {
            return enabled(severity_level::info);
        }

        bool is_warn_enabled() const noexcept
        {
            return enabled(severity_level::warn);
        }

        bool is_error_enabled() const noexcept
        {
            return enabled(severity_level::error);
        }

        bool is_fatal_enabled() const noexcept
        {
            return enabled(severity_level::fatal);
        }

        auto log(severity_level level, source_location loc = source_location::current())
        {
            return helper(*this, this->open_record(boost::log::keywords::severity = level, loc));
        }

        auto trace(source_location loc = source_location::current())
        {
            return log(severity_level::trace, loc);
        }

        auto debug(source_location loc = source_location::current())
        {
            return log(severity_level::debug, loc);
        }

        auto info(source_location loc = source_location::current())
        {
            return log(severity_level::info, loc);
        }

        auto warn(source_location loc = source_location::current())
        {
            return log(severity_level::warn, loc);
        }

        auto error(source_location loc = source_location::current())
        {
            return log(severity_level::error, loc);
        }

        auto fatal(source_location loc = source_location::current())
        {
            return log(severity_level::fatal, loc);
        }

#ifdef ING_HAS_FMT
        auto log(severity_level level,
                 fmt::string_view fmt,
                 fmt::format_args args,
                 source_location loc = source_location::current())
        {
            return helper(*this, this->open_record(boost::log::keywords::severity = level, loc), fmt, args);
        }

        auto trace(fmt::string_view fmt,
                   fmt::format_args args,
                   source_location loc = source_location::current())
        {
            return log(severity_level::trace, fmt, args, loc);
        }

        auto debug(fmt::string_view fmt,
                   fmt::format_args args,
                   source_location loc = source_location::current())
        {
            return log(severity_level::debug, fmt, args, loc);
        }

        auto info(fmt::string_view fmt,
                  fmt::format_args args,
                  source_location loc = source_location::current())
        {
            return log(severity_level::info, fmt, args, loc);
        }

        auto warn(fmt::string_view fmt,
                  fmt::format_args args,
                  source_location loc = source_location::current())
        {
            return log(severity_level::warn, fmt, args, loc);
        }

        auto error(fmt::string_view fmt,
                   fmt::format_args args,
                   source_location loc = source_location::current())
        {
            return log(severity_level::error, fmt, args, loc);
        }

        auto fatal(fmt::string_view fmt,
                   fmt::format_args args,
                   source_location loc = source_location::current())
        {
            return log(severity_level::fatal, fmt, args, loc);
        }

        template<typename ...Args>
        auto log(severity_level level, fmt::fmtloc_t<Args...> fmtloc, Args&&... args)
        {
            return log(level, fmtloc.get(), fmt::make_format_args(args...), fmtloc.location);
        }

        template<typename ...Args>
        auto trace(fmt::fmtloc_t<Args...> fmtloc, Args&&... args)
        {
            return trace(fmtloc.get(), fmt::make_format_args(args...), fmtloc.location);
        }

        template<typename ...Args>
        auto debug(fmt::fmtloc_t<Args...> fmtloc, Args&&... args)
        {
            return debug(fmtloc.get(), fmt::make_format_args(args...), fmtloc.location);
        }

        template<typename ...Args>
        auto info(fmt::fmtloc_t<Args...> fmtloc, Args&&... args)
        {
            return info(fmtloc.get(), fmt::make_format_args(args...), fmtloc.location);
        }

        template<typename ...Args>
        auto warn(fmt::fmtloc_t<Args...> fmtloc, Args&&... args)
        {
            return warn(fmtloc.get(), fmt::make_format_args(args...), fmtloc.location);
        }

        template<typename ...Args>
        auto error(fmt::fmtloc_t<Args...> fmtloc, Args&&... args)
        {
            return error(fmtloc.get(), fmt::make_format_args(args...), fmtloc.location);
        }

        template<typename ...Args>
        auto fatal(fmt::fmtloc_t<Args...> fmtloc, Args&&... args)
        {
            return fatal(fmtloc.get(), fmt::make_format_args(args...), fmtloc.location);
        }
#endif
    };

    using logger = basic_logger<boost::log::sources::logger::threading_model>;
    using logger_mt = basic_logger<boost::log::sources::logger_mt::threading_model>;

    BOOST_LOG_GLOBAL_LOGGER(global_logger, logger_mt)

    inline bool enabled(logging::severity_level level) noexcept
    {
        return global_logger::get().enabled(level);
    }

    inline bool is_trace_enabled() noexcept
    {
        return global_logger::get().is_trace_enabled();
    }

    inline bool is_debug_enabled() noexcept
    {
        return global_logger::get().is_debug_enabled();
    }

    inline bool is_info_enabled() noexcept
    {
        return global_logger::get().is_info_enabled();
    }

    inline bool is_warn_enabled() noexcept
    {
        return global_logger::get().is_warn_enabled();
    }

    inline bool is_error_enabled() noexcept
    {
        return global_logger::get().is_error_enabled();
    }

    inline bool is_fatal_enabled() noexcept
    {
        return global_logger::get().is_fatal_enabled();
    }

    inline auto log(logging::severity_level level, source_location loc = source_location::current())
    {
        return global_logger::get().log(level, loc);
    }

    inline auto trace(source_location loc = source_location::current())
    {
        return global_logger::get().trace(loc);
    }

    inline auto debug(source_location loc = source_location::current())
    {
        return global_logger::get().debug(loc);
    }

    inline auto info(source_location loc = source_location::current())
    {
        return global_logger::get().info(loc);
    }

    inline auto warn(source_location loc = source_location::current())
    {
        return global_logger::get().warn(loc);
    }

    inline auto error(source_location loc = source_location::current())
    {
        return global_logger::get().error(loc);
    }

    inline auto fatal(source_location loc = source_location::current())
    {
        return global_logger::get().fatal(loc);
    }

#ifdef ING_HAS_FMT
    inline auto log(logging::severity_level level,
                    fmt::string_view fmt,
                    fmt::format_args args,
                    source_location loc = source_location::current())
    {
        return global_logger::get().log(level, fmt, args, loc);
    }

    inline auto trace(fmt::string_view fmt,
                      fmt::format_args args,
                      source_location loc = source_location::current())
    {
        return global_logger::get().trace(fmt, args, loc);
    }

    inline auto debug(fmt::string_view fmt,
                      fmt::format_args args,
                      source_location loc = source_location::current())
    {
        return global_logger::get().debug(fmt, args, loc);
    }

    inline auto info(fmt::string_view fmt,
                     fmt::format_args args,
                     source_location loc = source_location::current())
    {
        return global_logger::get().info(fmt, args, loc);
    }

    inline auto warn(fmt::string_view fmt,
                     fmt::format_args args,
                     source_location loc = source_location::current())
    {
        return global_logger::get().warn(fmt, args, loc);
    }

    inline auto error(fmt::string_view fmt,
                      fmt::format_args args,
                      source_location loc = source_location::current())
    {
        return global_logger::get().error(fmt, args, loc);
    }

    inline auto fatal(fmt::string_view fmt,
                      fmt::format_args args,
                      source_location loc = source_location::current())
    {
        return global_logger::get().fatal(fmt, args, loc);
    }

    template<typename ...Args>
    auto log(logging::severity_level level, fmt::fmtloc_t<Args...> fmtloc, Args&&... args)
    {
        return global_logger::get().log(level, fmtloc, std::forward<Args>(args)...);
    }

    template<typename ...Args>
    auto trace(fmt::fmtloc_t<Args...> fmtloc, Args&&... args)
    {
        return global_logger::get().trace(fmtloc, std::forward<Args>(args)...);
    }

    template<typename ...Args>
    auto debug(fmt::fmtloc_t<Args...> fmtloc, Args&&... args)
    {
        return global_logger::get().debug(fmtloc, std::forward<Args>(args)...);
    }

    template<typename ...Args>
    auto info(fmt::fmtloc_t<Args...> fmtloc, Args&&... args)
    {
        return global_logger::get().info(fmtloc, std::forward<Args>(args)...);
    }

    template<typename ...Args>
    auto warn(fmt::fmtloc_t<Args...> fmtloc, Args&&... args)
    {
        return global_logger::get().warn(fmtloc, std::forward<Args>(args)...);
    }

    template<typename ...Args>
    auto error(fmt::fmtloc_t<Args...> fmtloc, Args&&... args)
    {
        return global_logger::get().error(fmtloc, std::forward<Args>(args)...);
    }

    template<typename ...Args>
    auto fatal(fmt::fmtloc_t<Args...> fmtloc, Args&&... args)
    {
        return global_logger::get().fatal(fmtloc, std::forward<Args>(args)...);
    }
#endif

    void init_logging(const std::string& file = {});
    void init_logging_from_stream(std::istream& in);
    void init_logging_from_settings(/*boost::log::settings*/void const * settings);
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
