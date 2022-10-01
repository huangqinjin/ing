#include <boost/test/unit_test.hpp>

#ifndef ING_TEST_SOURCE_LOCATION
#define ING_TEST_SOURCE_LOCATION no_std
#define ING_NO_STD_SOURCE_LOCATION
#endif
#include <ing/source_location.hpp>


static boost::test_tools::predicate_result contains(const char* str, const char* sub)
{
    if(std::strstr(str, sub)) return true;
    boost::test_tools::predicate_result res(false);
    res.message() << "[\"" << str << "\" !contains \"" << sub << "\"]";
    return res;
}


BOOST_AUTO_TEST_SUITE(ING_TEST_SOURCE_LOCATION)

BOOST_AUTO_TEST_CASE(current_source_location)
{
    ing::source_location macro = ING_CURRENT_LOCATION(); std::uint_least32_t line = __LINE__;
    const char* file = __FILE__;
    const char* func = __func__;

    BOOST_TEST(macro.file_name() == file);
    BOOST_TEST(macro.line() == line);
    BOOST_TEST(contains(macro.function_name(), func));
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
