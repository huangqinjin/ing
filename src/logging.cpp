#include <ing/logging.hpp>

#include <boost/log/detail/default_attribute_names.hpp>
#include <boost/log/attributes/clock.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/attributes/current_thread_id.hpp>
#include <boost/log/attributes/current_process_id.hpp>
#include <boost/log/attributes/current_process_name.hpp>

#include <boost/phoenix/operator.hpp>
#include <boost/log/expressions/keyword.hpp>
#include <boost/log/expressions/message.hpp>
#include <boost/log/expressions/formatters/if.hpp>
#include <boost/log/expressions/formatters/stream.hpp>
#include <boost/log/expressions/formatters/date_time.hpp>
#include <boost/log/expressions/formatters/named_scope.hpp>
#include <boost/log/expressions/formatters/auto_newline.hpp>
#include <boost/log/expressions/formatters/wrap_formatter.hpp>

#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/from_settings.hpp>
#include <boost/log/utility/setup/settings_parser.hpp>
#include <boost/log/utility/setup/formatter_parser.hpp>

#include <fstream>
#include <regex>


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


    static std::vector<std::pair<severity_level, std::regex>> thresholds;

    severity_level minimum_severity_level(std::string_view channel)
    {
        for (const auto& entry : thresholds)
        {
            if (std::regex_match(channel.begin(), channel.end(), entry.second))
                return entry.first;
        }
        return severity_level::trace;
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


    template<typename Keyword>
    auto format_source_location(const Keyword& keyword, std::string_view format)
    {
        return boost::log::expressions::wrap_formatter(
        [keyword, formatter = boost::log::expressions::aux::parse_named_scope_format(format.data(), format.data() + format.size())]
        (boost::log::record_view const& rec, boost::log::formatting_ostream& strm)
        {
            if (const auto& loc = rec[keyword])
            {
                struct unbox
                {
                    boost::log::string_literal::const_iterator str;
                    boost::log::string_literal::size_type len;

                    void operator=(const char* s)
                    {
                        static_assert(sizeof(unbox) == sizeof(boost::log::string_literal));

                        str = s;
                        len = boost::log::string_literal::traits_type::length(s);
                    }
                };

                using scope_entry = attributes::named_scope::scope_entry;
                scope_entry entry({}, {}, loc->line(), scope_entry::function);
                reinterpret_cast<unbox&>(entry.scope_name) = loc->function_name();
                reinterpret_cast<unbox&>(entry.file_name) = loc->file_name();
                formatter(strm, entry);
            }
        });
    }

    const auto reset_sgr = boost::phoenix::val("\033[39;49m");

    template<typename Keyword, std::size_t N>
    auto sgr(const Keyword& keyword, const std::array<std::string, N>& table)
    {
        return boost::log::expressions::wrap_formatter(
            [keyword, table](boost::log::record_view const& rec, boost::log::formatting_ostream& strm)
            {
                if (const auto& level = rec[keyword])
                {
                    auto i = static_cast<int>(*level);
                    if (i >= 0 && i < table.size() && !table[i].empty())
                        strm << '\033' << '[' << table[i] << 'm';
                }
            });
    }

    const auto strip_sgr = boost::log::expressions::wrap_formatter(
        [](boost::log::record_view const&, boost::log::formatting_ostream& strm)
        {
            auto find = [](char* s)
            {
                char* b = nullptr;
                char* e = nullptr;
                while (*s)
                {
                    if (*s++ == '\033' && *s++ == '[')
                    {
                        b = s - 2;
                        while (('0' <= *s && *s <= '9') || *s == ';') ++s;
                        if (*s == 'm')
                        {
                            e = s + 1;
                            break;
                        }
                    }
                }
                return std::make_pair(b, e);
            };

            strm.flush();

            if (std::string* s = strm.rdbuf()->storage())
            {
                auto a = find(s->data());
                if (a.second == nullptr) return;

                while (true)
                {
                    auto b = find(a.second);

                    if (b.second)
                    {
                        auto n = b.first - a.second;
                        std::memmove(a.first, a.second, n);
                        a.first += n;
                        a.second = b.second;
                    }
                    else
                    {
                        s->erase(a.first - s->data(), a.second - a.first);
                        break;
                    }
                }
            }
        });
}

namespace ing::logging::setup
{
    template<typename KeywordType>
    void register_simple_formatter_factory()
    {
        boost::log::register_simple_formatter_factory<typename KeywordType::value_type, char>(KeywordType::get_name());
    }

    boost::log::expressions::scope_iteration_direction scope_iteration_direction_from_string(const std::string& iteration)
    {
        if (iteration == "forward") return boost::log::expressions::forward;
        else if (iteration == "reverse") return boost::log::expressions::reverse;
        else throw std::invalid_argument(iteration);
    }

    // https://www.boost.org/doc/libs/develop/libs/log/doc/html/log/detailed/utilities.html#log.detailed.utilities.setup.filter_formatter

    // https://www.boost.org/doc/libs/develop/libs/log/doc/html/log/tutorial/formatters.html#log.tutorial.formatters.string_templates_as_formatters
    // Custom formatter factories to support string templates as formatters.

#define ARG(name) const std::string& name = (iter = args.find(#name)) != args.end() ? iter->second : this->name
#define ARGF(name, F) const auto name = (iter = args.find(#name)) != args.end() ? F(iter->second) : this->name

    // %Default%
    class default_formatter_factory final : public boost::log::formatter_factory<char>
    {
        formatter_type create_formatter(const boost::log::attribute_name& name, const args_map& args) override
        {
            (void) name;
            (void) args;
            return formatter;
        }

        const formatter_type formatter;

    public:
        explicit default_formatter_factory(formatter_type formatter) : formatter(formatter) {}
    };

    // https://www.boost.org/doc/libs/develop/libs/log/doc/html/log/detailed/expressions.html#log.detailed.expressions.formatters.date_time
    // %TimeStamp(format="%Y-%m-%d %H:%M:%S.%f")%
    class timestamp_formatter_factory final : public boost::log::formatter_factory<char>
    {
        formatter_type create_formatter(const boost::log::attribute_name& name, const args_map& args) override
        {
            (void) name;
            args_map::const_iterator iter;
            ARG(format);
            return boost::log::expressions::stream
                << boost::log::expressions::format_date_time(expressions::timestamp, format);
        }

    public:
        std::string format;
    };

    // https://www.boost.org/doc/libs/develop/libs/log/doc/html/log/detailed/expressions.html#log.detailed.expressions.formatters.named_scope
    // %LineID(format="%n %c %C %f %F %l")%
    class location_formatter_factory final : public boost::log::formatter_factory<char>
    {
        formatter_type create_formatter(const boost::log::attribute_name& name, const args_map& args) override
        {
            (void) name;
            args_map::const_iterator iter;
            ARG(format);
            return boost::log::expressions::stream
                << expressions::format_source_location(expressions::location, format);
        }

    public:
        std::string format;
    };

    // https://www.boost.org/doc/libs/develop/libs/log/doc/html/log/detailed/expressions.html#log.detailed.expressions.formatters.named_scope
    // %Scope(format="%n %c %C %f %F %l", iteration=reverse, delimiter="->", depth=0, incomplete_marker="...", empty_marker="")%
    class scope_formatter_factory final : public boost::log::formatter_factory<char>
    {
        formatter_type create_formatter(const boost::log::attribute_name& name, const args_map& args) override
        {
            (void) name;
            args_map::const_iterator iter;
            ARG(format);
            ARG(iteration);
            ARG(delimiter);
            ARGF(depth, std::stoul);
            ARG(incomplete_marker);
            ARG(empty_marker);
            ARGF(auto_newline, boost::lexical_cast<bool>);
            ARGF(threshold, boost::lexical_cast<logging::expressions::severity_type::value_type>);

            return boost::log::expressions::stream
                << boost::log::expressions::if_(logging::expressions::severity > threshold)
                [
                    boost::log::expressions::stream
                    << boost::log::expressions::if_(boost::phoenix::val(auto_newline))[
                       boost::log::expressions::stream << boost::log::expressions::auto_newline
                    ]
                    << boost::log::expressions::format_named_scope(
                       expressions::scope,
                       boost::log::keywords::format = format,
                       boost::log::keywords::delimiter = delimiter,
                       boost::log::keywords::depth = depth,
                       boost::log::keywords::incomplete_marker = incomplete_marker,
                       boost::log::keywords::empty_marker = empty_marker,
                       boost::log::keywords::iteration = scope_iteration_direction_from_string(iteration)
                   )
                ];
        }

    public:
        std::string format;
        std::string iteration;
        std::string delimiter;
        unsigned long depth;
        std::string empty_marker;
        std::string incomplete_marker;
        bool auto_newline;
        expressions::severity_type::value_type threshold;
    };

    // https://en.wikipedia.org/wiki/ANSI_escape_code#SGR_(Select_Graphic_Rendition)_parameters
    // %SGR(mode=reset|strip)%
    class sgr_formatter_factory : public boost::log::formatter_factory<char>
    {
        formatter_type create_formatter(const boost::log::attribute_name& name, const args_map& args) override
        {
            (void) name;
            args_map::const_iterator iter;
            auto mode = (iter = args.find("mode")) != args.end() ? iter->second : "";
            if (mode == "reset") return boost::log::expressions::stream
                        << expressions::reset_sgr;
            if (mode == "strip") return boost::log::expressions::stream
                        << expressions::strip_sgr;
            if (!mode.empty()) throw std::invalid_argument("invalid mode " + mode);
            return boost::log::expressions::stream
                    << expressions::sgr(expressions::severity, table);
        }

    public:
        std::array<std::string, 6> table;
    };

#undef ARG
}


namespace ing
{
    BOOST_LOG_GLOBAL_LOGGER_CTOR_ARGS(global_logger, logger_mt, ("global"))


    void init_logging(const boost::log::settings& settings);

    void init_logging_from_settings(void const * settings)
    {
        auto setts = static_cast<const boost::log::settings*>(settings);
        init_logging(setts ? *setts : boost::log::settings{});
    }

    void init_logging_from_stream(std::istream& in)
    {
        init_logging(boost::log::parse_settings(in));
    }

    void init_logging(const std::string& file)
    {
        if (file.empty())
        {
            init_logging_from_settings(nullptr);
        }
        else
        {
            std::ifstream in(file);
            init_logging_from_stream(in);
        }
    }
}

void ing::init_logging(const boost::log::settings& settings)
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

    // https://www.boost.org/doc/libs/develop/libs/log/doc/html/log/detailed/expressions.html#log.detailed.expressions.predicates.channel_severity_filter
    logging::thresholds.clear();
    if (auto severities = settings["Thresholds"].get_section())
    {
        for (const auto& entry : severities.property_tree())
        {
            auto& pair = logging::thresholds.emplace_back();
            pair.first = boost::lexical_cast<logging::expressions::severity_type::value_type>(entry.first);
            pair.second.assign(entry.second.get_value<std::string>());
        }
    }

    auto timestamp_formatter_factory = boost::make_shared<logging::setup::timestamp_formatter_factory>();
    auto location_formatter_factory = boost::make_shared<logging::setup::location_formatter_factory>();
    auto scope_formatter_factory = boost::make_shared<logging::setup::scope_formatter_factory>();
    auto sgr_formatter_factory = boost::make_shared<logging::setup::sgr_formatter_factory>();

    if (auto sgr = settings["Attributes"]["SGR"].get_section())
    {
        for (const auto& entry : sgr.property_tree())
        {
            auto severity = boost::lexical_cast<logging::expressions::severity_type::value_type>(entry.first);
            sgr_formatter_factory->table.at(static_cast<std::size_t>(severity)) = entry.second.get_value<std::string>();
        }
    }
    else
    {
        sgr_formatter_factory->table[0] = "36"; // Cyan
        sgr_formatter_factory->table[1] = "35"; // Magenta
        sgr_formatter_factory->table[2] = "39"; // Default
        sgr_formatter_factory->table[3] = "33"; // Yellow
        sgr_formatter_factory->table[4] = "31"; // Red
        sgr_formatter_factory->table[5] = "32"; // Green
    }

    timestamp_formatter_factory->format = settings["Attributes"]["TimeStamp"]["format"].or_default("%H:%M:%S.%f");
    location_formatter_factory->format = settings["Attributes"]["LineID"]["format"].or_default("%F(%l) '%n'");
    scope_formatter_factory->format = settings["Attributes"]["Scope"]["format"].or_default("\t <- %f(%l) '%n'");
    scope_formatter_factory->iteration = settings["Attributes"]["Scope"]["iteration"].or_default("reverse");
    scope_formatter_factory->depth = settings["Attributes"]["Scope"]["depth"].or_default(8ul);
    scope_formatter_factory->delimiter = settings["Attributes"]["Scope"]["delimiter"].or_default("\n");
    scope_formatter_factory->empty_marker = settings["Attributes"]["Scope"]["empty_marker"].or_default("");
    scope_formatter_factory->incomplete_marker = settings["Attributes"]["Scope"]["incomplete_marker"].or_default("\n\t <- ...");
    scope_formatter_factory->auto_newline = settings["Attributes"]["Scope"]["auto_newline"].or_default(true);
    scope_formatter_factory->threshold = settings["Attributes"]["Scope"]["threshold"].or_default(logging::expressions::severity_type::value_type::error);

    // https://www.boost.org/doc/libs/develop/libs/log/doc/html/log/detailed/expressions.html#log.detailed.expressions.formatters
    // stream-style syntax usually results in a faster formatter than the one constructed with the Boost.Format-style.
    auto fmt = boost::log::expressions::stream
            << logging::expressions::sgr(logging::expressions::severity, sgr_formatter_factory->table)
            << boost::log::expressions::format_date_time(logging::expressions::timestamp, timestamp_formatter_factory->format) << ' '
            << '[' << logging::expressions::severity << ']' << ' '
            << boost::log::expressions::if_(
                   logging::expressions::severity == logging::expressions::severity_type::value_type::info ||
                   logging::expressions::severity == logging::expressions::severity_type::value_type::warn
               )[boost::log::expressions::stream << ' ']
            << '<' << logging::expressions::channel << '>' << ' '
            << logging::expressions::format_source_location(logging::expressions::location, location_formatter_factory->format) << ' '
            << '-' << ' '
            << boost::log::expressions::smessage
            << boost::log::expressions::if_(logging::expressions::severity > scope_formatter_factory->threshold)
               [
                    boost::log::expressions::stream
                    << boost::log::expressions::auto_newline
                    << boost::log::expressions::format_named_scope(
                           logging::expressions::scope,
                           boost::log::keywords::format = scope_formatter_factory->format,
                           boost::log::keywords::delimiter = scope_formatter_factory->delimiter,
                           boost::log::keywords::depth = scope_formatter_factory->depth,
                           boost::log::keywords::empty_marker = scope_formatter_factory->empty_marker,
                           boost::log::keywords::incomplete_marker = scope_formatter_factory->incomplete_marker,
                           boost::log::keywords::iteration = logging::setup::scope_iteration_direction_from_string(
                                   scope_formatter_factory->iteration))
               ]
            << logging::expressions::reset_sgr
            ;

    boost::log::register_formatter_factory("Default", boost::make_shared<logging::setup::default_formatter_factory>(fmt));
    boost::log::register_formatter_factory(logging::expressions::timestamp_type::get_name(), timestamp_formatter_factory);
    boost::log::register_formatter_factory(logging::expressions::location_type::get_name(), location_formatter_factory);
    boost::log::register_formatter_factory(logging::expressions::scope_type::get_name(), scope_formatter_factory);
    boost::log::register_formatter_factory("SGR", sgr_formatter_factory);
    logging::setup::register_simple_formatter_factory<logging::expressions::severity_type>();

    boost::log::init_from_settings(settings);
    if (settings.has_section("Sinks")) return;

    boost::log::add_console_log(std::clog,
        boost::log::keywords::auto_newline_mode = boost::log::sinks::insert_if_missing,
        boost::log::keywords::auto_flush = true,
        // boost::log::keywords::filter = ,
        boost::log::keywords::format = fmt
    );
}

