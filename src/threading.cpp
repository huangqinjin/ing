#include <ing/threading.hpp>
#include <ing/spinlock.hpp>

#include <mutex>
#include <sstream>
#include <unordered_map>

using namespace ing;

namespace
{
    template<class F>
    struct finally
    {
        F f;
        finally(F f) : f(f) {}
        ~finally() { f(); }
    };

    using map = std::unordered_map<std::thread::id, string>;
    map name_map;
    spinlock name_map_guard;

    struct thread_name
    {
        map::iterator entry;

        thread_name(const thread_name&) = delete;
        thread_name& operator=(const thread_name&) = delete;

        thread_name() noexcept
        {
            auto id = std::this_thread::get_id();
            auto name = def(id);
            map tmp = { {id, name} };
            if (auto node = tmp.extract(tmp.begin()))
            {
                std::lock_guard _(name_map_guard);
                entry = name_map.insert(std::move(node)).position;
            }
        }

        ~thread_name()
        {
            [this] {
                std::lock_guard _(name_map_guard);
                return name_map.extract(entry);
            }();
        }

        static void set(std::thread::id id, string name)
        {
            std::lock_guard _(name_map_guard);
            name_map[id].swap(name);
        }

        static string get(std::thread::id id)
        {
            {
                std::lock_guard _(name_map_guard);
                auto iter = name_map.find(id);
                if(iter != name_map.end())
                    return iter->second;
            }
            return def(id);
        }

        static string def(std::thread::id id)
        {
            return string((std::ostringstream{} << std::hex << id).str());
        }
    };

    thread_local thread_name current;
}

void ing::set_thread_name(string name) noexcept
{
    current.entry->second.swap(name);
}

void ing::set_thread_name(std::thread::id id, string name) noexcept
{
    thread_name::set(id, std::move(name));
}

string ing::get_thread_name() noexcept
{
    return current.entry->second;
}

string ing::get_thread_name(std::thread::id id) noexcept
{
    return thread_name::get(id);
}
