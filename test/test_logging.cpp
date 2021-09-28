#include <boost/test/unit_test.hpp>

#include <boost/log/attributes/named_scope.hpp>

#include <ing/logging.hpp>


BOOST_AUTO_TEST_CASE(default_settings)
{
    BOOST_LOG_FUNCTION();

    ing::init_logging();

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

