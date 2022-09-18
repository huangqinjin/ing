#include <boost/test/unit_test.hpp>

#include <ing/timing.hpp>

#include <thread>
#include <chrono>

BOOST_AUTO_TEST_CASE(timer)
{
    ing::init_logging();

    ing::timer::signal([](const char* timer, int signal) {
        ING_GLOG(warn) << "TIMER: " << timer << ", SIGNAL: " << signal;
    });

    for (int i = 0; i < 3; ++i)
    {
        ing::timer t("timer");

        std::this_thread::sleep_for(std::chrono::milliseconds(20 * (i + 1)));

        t.phase("1");

        std::this_thread::sleep_for(std::chrono::milliseconds(40 * (i + 1)));

        t.phase("2");
    }

    ing::timer::report(std::clog << '\n', ".*");
}
