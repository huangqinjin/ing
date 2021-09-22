#include <ing/threading.hpp>
#include <ing/spinlock.hpp>

#include <mutex>
#include <sstream>
#include <unordered_map>

#include <cstring>

#if defined(__linux__) || defined(__APPLE__)
#include <pthread.h>
#elif defined(_WIN32)
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

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
            auto name = get_platform(id);
            if (name.empty()) name = def(id);
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

        // https://stackoverflow.com/questions/2369738
        static bool set_platform(std::thread::id id, std::string_view name)
        {
#if defined(__linux__) || defined(__APPLE__)
            static_assert(sizeof(pthread_t) == sizeof(std::thread::id));
            char buf[16]{};
            std::memcpy(buf, name.data(), std::min(sizeof(buf) - 1, name.size()));

#  if defined(__APPLE__)
            return id == std::this_thread::get_id() && 0 == pthread_setname_np(buf);
#  else
            return 0 == pthread_setname_np(reinterpret_cast<pthread_t&>(id), buf);
#  endif
#elif defined(_WIN32)
            static_assert(sizeof(DWORD) == sizeof(std::thread::id));
            if (HANDLE hThread = (id == std::this_thread::get_id()) ? GetCurrentThread() :
                OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, reinterpret_cast<DWORD&>(id)))
            {
                finally _ = [&] {
                    if (hThread != GetCurrentThread()) CloseHandle(hThread);
                };

                std::wstring desc;
                desc.resize(MultiByteToWideChar(CP_UTF8, 0, name.data(), (int)name.size(), NULL, 0));
                MultiByteToWideChar(CP_UTF8, 0, name.data(), (int)name.size(), desc.data(), (int)desc.size());
                return S_OK == SetThreadDescription(hThread, desc.c_str());
            }
            return false;
#else
            return false;
#endif
        }

        static string get_platform(std::thread::id id)
        {
#if defined(__linux__) || defined(__APPLE__)
            char buf[16]{};
            pthread_getname_np(reinterpret_cast<pthread_t&>(id), buf, sizeof(buf));
            return buf;
#elif defined(_WIN32)
            if (HANDLE hThread = (id == std::this_thread::get_id()) ? GetCurrentThread() :
                OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, reinterpret_cast<DWORD&>(id)))
            {
                PWSTR desc = NULL;
                
                finally _ = [&] {
                    if (hThread != GetCurrentThread()) CloseHandle(hThread);
                    if (desc) LocalFree(desc);
                };

                if (S_OK == GetThreadDescription(hThread, &desc))
                {
                    std::string s;
                    std::size_t len = std::wcslen(desc);
                    s.resize(WideCharToMultiByte(CP_UTF8, 0, desc, (int)len, NULL, 0, NULL, NULL));
                    WideCharToMultiByte(CP_UTF8, 0, desc, (int)len, s.data(), (int)s.size(), NULL, NULL);
                    return string(s);
                }
            }
            return {};
#else
            return {};
#endif
        }
    };

    thread_local thread_name current;
}

void ing::set_thread_name(string name) noexcept
{
    thread_name::set_platform(std::this_thread::get_id(), name);
    current.entry->second.swap(name);
}

void ing::set_thread_name(std::thread::id id, string name) noexcept
{
    thread_name::set_platform(id, name);
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

void ing::sync_thread_name() noexcept
{
    thread_name::set_platform(std::this_thread::get_id(), get_thread_name());
}
