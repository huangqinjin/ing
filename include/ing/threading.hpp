#ifndef ING_THREADING_HPP
#define ING_THREADING_HPP

#include <thread>
#include "string.hpp"

namespace ing
{
    void set_thread_name(string name) noexcept;
    void set_thread_name(std::thread::id id, string name) noexcept;
    string get_thread_name() noexcept;
    string get_thread_name(std::thread::id id) noexcept;
    void sync_thread_name() noexcept;
};

#endif
