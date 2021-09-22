#include <boost/test/unit_test.hpp>

#include <ing/threading.hpp>
#include <chrono>

using namespace std::chrono_literals;
using namespace ing;

BOOST_AUTO_TEST_CASE(thread_name)
{
    set_thread_name("test-parent");

    std::thread t([]{
        std::this_thread::sleep_for(100ms);
        set_thread_name("test-child");
        BOOST_TEST(get_thread_name().c_str() == "test-child");
        std::this_thread::sleep_for(500ms);
        BOOST_TEST(get_thread_name().c_str() == "test-child-2");
    });

    BOOST_TEST(get_thread_name().c_str() == "test-parent");
    BOOST_TEST(get_thread_name(std::this_thread::get_id()).c_str() == "test-parent");
    BOOST_TEST(get_thread_name(t.get_id()).c_str() != "test-child");

    std::this_thread::sleep_for(200ms);

    BOOST_TEST(get_thread_name(t.get_id()).c_str() == "test-child");
    set_thread_name(t.get_id(), "test-child-2");

    t.join();
}
