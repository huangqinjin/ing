#include <boost/test/unit_test.hpp>

#ifndef ING_TEST_SOURCE_LOCATION
#define ING_TEST_SOURCE_LOCATION no_std
#define ING_NO_STD_SOURCE_LOCATION
#endif
#include <ing/source_location.hpp>


BOOST_AUTO_TEST_SUITE(ING_TEST_SOURCE_LOCATION)

BOOST_AUTO_TEST_CASE(current_source_location)
{
    ing::source_location macro = ING_CURRENT_LOCATION(); std::uint_least32_t line = __LINE__;
    const char* file = __FILE__;
    const char* function =
#ifdef ING_NO_STD_SOURCE_LOCATION
        BOOST_CURRENT_FUNCTION;
#else
        __func__;
#endif

    BOOST_TEST(macro.file_name() == file);
    BOOST_TEST(macro.line() == line);
    BOOST_TEST(macro.function_name() == function);
}

BOOST_AUTO_TEST_CASE(modified_source_location)
{
    ing::source_location modified = ING_CURRENT_LOCATION().function_name("custom").column(12u); std::uint_least32_t line = __LINE__;
    const char* file = __FILE__;

    BOOST_TEST(modified.file_name() == file);
    BOOST_TEST(modified.line() == line);
    BOOST_TEST(modified.function_name() == "custom");
    BOOST_TEST(modified.column() == 12u);
}

BOOST_AUTO_TEST_SUITE_END()
