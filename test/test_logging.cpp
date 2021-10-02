#include <boost/test/unit_test.hpp>

#include <boost/mpl/list.hpp>

#include <boost/log/attributes/named_scope.hpp>

#include <boost/log/utility/setup/console.hpp>

#include <ing/logging.hpp>

#include <sstream>

namespace utf = boost::unit_test;

struct DefaultSetting
{
    DefaultSetting()
    {
        BOOST_TEST_MESSAGE("default setting");
        ing::init_logging();
    }
};

struct DefaultFormatterTemplate
{
    DefaultFormatterTemplate()
    {
        BOOST_TEST_MESSAGE("default formatter template");
        ing::init_logging();
        boost::log::core::get()->remove_all_sinks();
        boost::log::add_console_log(std::clog,
            boost::log::keywords::auto_newline_mode = boost::log::sinks::insert_if_missing,
            boost::log::keywords::auto_flush = true,
            boost::log::keywords::format = "%Default%"
        );
    }
};

struct CustomFormatterTemplate
{
    CustomFormatterTemplate()
    {
        BOOST_TEST_MESSAGE("custom formatter template");
        ing::init_logging();
        boost::log::core::get()->remove_all_sinks();
        boost::log::add_console_log(std::clog,
            boost::log::keywords::auto_newline_mode = boost::log::sinks::insert_if_missing,
            boost::log::keywords::auto_flush = true,
            boost::log::keywords::format =
                "%TimeStamp(format=\"%d/%b/%y %I:%M:%S.%f %p\")% "
                "[%Severity%] <%Channel%> "
                "%LineID(format=\"%F:%l\")% "
                "'%Scope(format=\"%n\",depth=1,incomplete_marker=\"\")%' "
                "- %Message%"
        );
    }
};

struct ThresholdPerLogger
{
    ThresholdPerLogger()
    {
        BOOST_TEST_MESSAGE("threshold per logger");

        std::istringstream in(
R"INI(
[Thresholds]
WARN = global  # global will be TRACE as it has already been initialized.
WARN = "lo.*"  # local will be WARN.
ERROR = local
)INI");

        ing::init_logging_from_stream(in);
    }
};

using Settings = boost::mpl::list<
    DefaultSetting,
    DefaultFormatterTemplate,
    CustomFormatterTemplate,
    ThresholdPerLogger
>;

BOOST_FIXTURE_TEST_CASE_TEMPLATE(logging, Setting, Settings, Setting)
{
    BOOST_LOG_FUNCTION();

    ing::logger logger("local");

    BOOST_LOG_STREAM_SEV(logger, ing::logging::severity_level::trace) << "A trace severity message";
    BOOST_LOG_STREAM_SEV(logger, ing::logging::severity_level::debug) << "A debug severity message";
    BOOST_LOG_STREAM_SEV(logger, ing::logging::severity_level::info)  << "An info severity message";
    BOOST_LOG_STREAM_SEV(logger, ing::logging::severity_level::warn)  << "A warn severity message";
    BOOST_LOG_STREAM_SEV(logger, ing::logging::severity_level::error) << "An error severity message";
    BOOST_LOG_STREAM_SEV(logger, ing::logging::severity_level::fatal) << "A fatal severity message";

    BOOST_LOG_NAMED_SCOPE("local logger");

    ING_LLOG(trace) << "A trace severity message";
    ING_LLOG(debug) << "A debug severity message";
    ING_LLOG(info)  << "An info severity message";
    ING_LLOG(warn)  << "A warn severity message";
    ING_LLOG(error) << "An error severity message";
    ING_LLOG(fatal) << "A fatal severity message";

    BOOST_LOG_NAMED_SCOPE("global logger");

    ING_GLOG(trace) << "A trace severity message";
    ING_GLOG(debug) << "A debug severity message";
    ING_GLOG(info)  << "An info severity message";
    ING_GLOG(warn)  << "A warn severity message";
    ING_GLOG(error) << "An error severity message";
    ING_GLOG(fatal) << "A fatal severity message";
}

