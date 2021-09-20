#include <boost/test/unit_test.hpp>

#include <ing/uptime.hpp>

#include <sstream>
#include <thread>
#include <chrono>
#include <regex>
#include <vector>

namespace tt = boost::test_tools;
using namespace std::chrono_literals;

namespace
{
    class timer : public ing::uptime
    {
        std::stringstream ss;

        void report(const std::string& name, const ing::source_location& loc) override
        {
            format(ss << loc << ' ' << name);
        }

    public:
        using uptime::uptime;

        ~timer() noexcept(false)
        {
            finalize();

            struct time
            {
                double wall;
                double user;
                double system;
            };
            std::vector<time> times;

            std::regex re(R"(^.*(\d+\.\d+)s wall.*(\d+\.\d+)s user.*(\d+\.\d+)s system.*$)");

            std::string line;
            while (getline(ss, line))
            {
                BOOST_TEST_MESSAGE(line);
                std::smatch matches;
                BOOST_TEST_REQUIRE(std::regex_match(line, matches, re));
                BOOST_TEST_REQUIRE(matches.size() == 4);
                auto& t = times.emplace_back();
                t.wall = std::stod(matches[1]);
                t.user = std::stod(matches[2]);
                t.system = std::stod(matches[3]);
                //BOOST_TEST(t.wall > t.user + t.system); //TODO
            }

            BOOST_TEST_REQUIRE(times.size() == 4);

            time sum{}, total = times.back();
            for (std::size_t i = 0; i < times.size() - 1; ++i)
            {
                sum.wall += times[i].wall;
                sum.user += times[i].user;
                sum.system += times[i].system;
            }
            BOOST_TEST(sum.wall <= total.wall);
            BOOST_TEST(sum.user <= total.user);
            BOOST_TEST(sum.system <= total.system);

            BOOST_TEST(sum.wall == total.wall, tt::tolerance(0.1));
            BOOST_TEST(sum.user == total.user, tt::tolerance(0.001));
            BOOST_TEST(sum.system == total.system, tt::tolerance(0.001));
        }
    };
}


BOOST_AUTO_TEST_CASE(multiple_phases)
{
    timer timer("Uptime");
    {
        std::this_thread::sleep_for(100ms);
        timer.phase("0");
    }
    {
        using clock = std::chrono::system_clock;
        auto now = clock::now();
        while (clock::now() - now < 200ms);
        timer.phase("1");
    }
    {
        std::this_thread::sleep_for(300ms);
        timer.phase("2");
    }
}

