#include <boost/test/unit_test.hpp>

#include <boost/log/attributes/named_scope.hpp>

#include <ing/logging.hpp>


BOOST_AUTO_TEST_CASE(logging_with_fmt)
{
    BOOST_LOG_FUNCTION();

    ing::init_logging();

    BOOST_LOG_NAMED_SCOPE("local logger");

    ing::logger logger("local");

    logger.trace() << "A trace severity message at line " << __LINE__ << ' ' << __func__;
    logger.debug() << "A debug severity message at line " << __LINE__ << ' ' << __func__;
    logger.info () << "An info severity message at line " << __LINE__ << ' ' << __func__;
    logger.warn () << "A warn severity message at line " << __LINE__ << ' ' << __func__;
    logger.error() << "An error severity message at line " << __LINE__ << ' ' << __func__;
    logger.fatal() << "A fatal severity message at line " << __LINE__ << ' ' << __func__;

    logger.trace("A trace severity message at line {} ", __LINE__) << __func__;
    logger.debug("A debug severity message at line {} ", __LINE__) << __func__;
    logger.info ("An info severity message at line {} ", __LINE__) << __func__;
    logger.warn ("A warn severity message at line {} ", __LINE__) << __func__;
    logger.error("An error severity message at line {} ", __LINE__) << __func__;
    logger.fatal("A fatal severity message at line {} ", __LINE__) << __func__;

    BOOST_LOG_NAMED_SCOPE("global logger");

    ing::trace() << "A trace severity message at line " << ing::fmt::format("{} {}", __LINE__, __func__);
    ing::debug() << "A debug severity message at line " << ing::fmt::format("{} {}", __LINE__, __func__);
    ing::info () << "An info severity message at line " << ing::fmt::format("{} {}", __LINE__, __func__);
    ing::warn () << "A warn severity message at line " << ing::fmt::format("{} {}", __LINE__, __func__);
    ing::error() << "An error severity message at line " << ing::fmt::format("{} {}", __LINE__, __func__);
    ing::fatal() << "A fatal severity message at line " << ing::fmt::format("{} {}", __LINE__, __func__);

    if (auto h = ing::trace("A trace severity message at line {} ", __LINE__)) h << ing::fmt::format("{}", __func__);
    if (auto h = ing::debug("A debug severity message at line {} ", __LINE__)) h << ing::fmt::format("{}", __func__);
    if (auto h = ing::info ("An info severity message at line {} ", __LINE__)) h << ing::fmt::format("{}", __func__);
    if (auto h = ing::warn ("A warn severity message at line {} ", __LINE__)) h << ing::fmt::format("{}", __func__);
    if (auto h = ing::error("An error severity message at line {} ", __LINE__)) h << ing::fmt::format("{}", __func__);
    if (auto h = ing::fatal("A fatal severity message at line {} ", __LINE__)) h << ing::fmt::format("{}", __func__);
}
