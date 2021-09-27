#include <ing/logging.hpp>

#include <boost/log/detail/default_attribute_names.hpp>
#include <boost/log/attributes/clock.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/attributes/current_thread_id.hpp>
#include <boost/log/attributes/current_process_id.hpp>
#include <boost/log/attributes/current_process_name.hpp>

#include <boost/log/expressions/keyword.hpp>


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

namespace ing::logging::attributes
{
    using namespace boost::log::attributes;

    using current_thread_name = thread_specific<std::string>;
}

namespace ing::logging::expressions
{
    namespace names
    {
        using namespace boost::log::aux::default_attribute_names;

        static boost::log::attribute_name get(int i)
        {
            static const boost::log::attribute_name names[] = {
                    "ThreadName",
                    "ProcessName",
                    "Scope",
            };
            return names[i];
        }

        boost::log::attribute_name thread_name() { return get(0); }
        boost::log::attribute_name process_name() { return get(1); }
        boost::log::attribute_name scope() { return get(2); }
    }

    BOOST_LOG_ATTRIBUTE_KEYWORD(severity, ::ing::logging::expressions::names::severity(), ::ing::logger_mt::severity_attribute::value_type)
    BOOST_LOG_ATTRIBUTE_KEYWORD(channel, ::ing::logging::expressions::names::channel(), ::ing::logger_mt::channel_attribute::value_type)
    BOOST_LOG_ATTRIBUTE_KEYWORD(location, ::ing::logging::expressions::names::line_id(), ::ing::logger_mt::location_attribute::value_type)
    BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, ::ing::logging::expressions::names::timestamp(), ::ing::logging::attributes::local_clock::value_type)
    BOOST_LOG_ATTRIBUTE_KEYWORD(thread_id, ::ing::logging::expressions::names::thread_id(), ::ing::logging::attributes::current_thread_id::value_type)
    BOOST_LOG_ATTRIBUTE_KEYWORD(process_id, ::ing::logging::expressions::names::process_id(), ::ing::logging::attributes::current_process_id::value_type)
    BOOST_LOG_ATTRIBUTE_KEYWORD(thread_name, ::ing::logging::expressions::names::thread_name(), ::ing::logging::attributes::current_thread_name::value_type)
    BOOST_LOG_ATTRIBUTE_KEYWORD(process_name, ::ing::logging::expressions::names::process_name(), ::ing::logging::attributes::current_process_name::value_type)
    BOOST_LOG_ATTRIBUTE_KEYWORD(scope, ::ing::logging::expressions::names::scope(), ::ing::logging::attributes::named_scope::value_type)
}

namespace ing
{
    BOOST_LOG_GLOBAL_LOGGER_CTOR_ARGS(global_logger, logger_mt, ("global"))
}

void ing::init_logging()
{
    //boost::log::add_common_attributes();
    auto core = boost::log::core::get();
    // Reset core so that can be initialized multiple times.
    core->set_logging_enabled(false);
    core->remove_all_sinks();
    core->reset_filter();
    core->set_logging_enabled(true);

    core->add_global_attribute(logging::expressions::timestamp_type::get_name(), logging::attributes::local_clock());
    core->add_global_attribute(logging::expressions::thread_id_type::get_name(), logging::attributes::current_thread_id());
    core->add_global_attribute(logging::expressions::process_id_type::get_name(), logging::attributes::current_process_id());
    core->add_global_attribute(logging::expressions::thread_name_type::get_name(), logging::attributes::current_thread_name());
    core->add_global_attribute(logging::expressions::process_name_type::get_name(), logging::attributes::current_process_name());
    core->add_global_attribute(logging::expressions::scope_type::get_name(), logging::attributes::named_scope());
}

