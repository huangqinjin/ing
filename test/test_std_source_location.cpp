#include <boost/test/unit_test.hpp>

#include <ing/source_location.hpp>
//#ifndef ING_NO_STD_SOURCE_LOCATION
#if 0
#define ING_TEST_SOURCE_LOCATION with_std

namespace
{
    void compare(ing::source_location current, ing::source_location caller = ing::source_location::current())
    {
        BOOST_TEST(current.file_name() == caller.file_name());
        BOOST_TEST(current.line() == caller.line());
        BOOST_TEST(current.function_name() == caller.function_name());
        BOOST_TEST(current.column() == caller.column());
    }
}

BOOST_AUTO_TEST_SUITE(ING_TEST_SOURCE_LOCATION)

BOOST_AUTO_TEST_CASE(caller_source_location)
{
    compare(ING_CURRENT_LOCATION());
}

BOOST_AUTO_TEST_SUITE_END()


#include "test_source_location.cpp"

#endif
