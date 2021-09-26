#ifndef ING_LOGGING_HPP
#define ING_LOGGING_HPP

#include <ostream>
#include <istream>
#include <string_view>

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
