#include <boost/test/unit_test.hpp>

#include <ing/threading.hpp>
#include <atomic>
#include <chrono>

using namespace std::chrono_literals;
using namespace ing;

BOOST_AUTO_TEST_CASE(thread_name)
{
    set_thread_name("test-parent");

    std::atomic<int> barrier(0);
    auto arrive = [&barrier](int b) {
        barrier.store(b);
    };
    auto wait = [&barrier](int b) {
        while (barrier.load() != b)
            std::this_thread::yield();
    };

    std::thread t([&] {
        wait(1);
        set_thread_name("test-child");
        arrive(2);
        BOOST_TEST(get_thread_name().c_str() == "test-child");
        wait(3);
        sync_thread_name();
        BOOST_TEST(get_thread_name().c_str() == "test-child-2");
    });

    BOOST_TEST(get_thread_name().c_str() == "test-parent");
    BOOST_TEST(get_thread_name(std::this_thread::get_id()).c_str() == "test-parent");
    BOOST_TEST(get_thread_name(t.get_id()).c_str() != "test-child");

    arrive(1);
    wait(2);
    BOOST_TEST(get_thread_name(t.get_id()).c_str() == "test-child");

    set_thread_name(t.get_id(), "test-child-2");
    BOOST_TEST(get_thread_name(t.get_id()).c_str() == "test-child-2");
    arrive(3);

    t.join();
}
