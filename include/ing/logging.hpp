#ifndef ING_LOGGING_HPP
#define ING_LOGGING_HPP

#include <ostream>
#include <istream>
#include <string_view>
#include <type_traits>

#include <boost/log/sources/severity_feature.hpp>

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
}

#endif
