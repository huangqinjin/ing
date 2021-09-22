#include <ing/string.hpp>

#include <new>
#include <atomic>
#include <cstdlib>
#include <cstring>

using namespace ing;

namespace
{
    struct header
    {
        std::atomic<int> refcount = 1;
        std::size_t length = 0;
    };

    struct nullstr
    {
        header h;
        char z[8]{};
    } nullstr;
}

string_base::~string_base()
{
    auto* h = static_cast<header*>(str) - 1;
    if (h->refcount.fetch_sub(1, std::memory_order_relaxed) == 1)
    {
        h->~header();
        std::free(h);
    }
}

string_base::string_base() noexcept : str(nullstr.z)
{
    auto* h = static_cast<header*>(str) - 1;
    h->refcount.fetch_add(1, std::memory_order_relaxed);
}

string_base::string_base(const void* buf, std::size_t count, std::size_t size)
{
    if (buf == nullptr || count == 0 || size == 0)
    {
        if (size > sizeof(nullstr.z)) throw std::bad_array_new_length();
        str = nullstr.z;
        auto* h = static_cast<header*>(str) - 1;
        h->refcount.fetch_add(1, std::memory_order_relaxed);
    }
    else if ((str = std::malloc(sizeof(header) + (count + 1) * size)))
    {
        auto* h = new (str) header;
        h->length = count;
        str = h + 1;
        std::memcpy(str, buf, count * size);
        std::memset((char*)str + count * size, 0, size);
    }
    else
    {
        throw std::bad_alloc();
    }
}

string_base::string_base(const string_base& other) noexcept : str(other.str)
{
    auto* h = static_cast<header*>(str) - 1;
    h->refcount.fetch_add(1, std::memory_order_relaxed);
}

string_base::string_base(string_base&& other) noexcept : str(other.str)
{
    other.str = nullstr.z;
    auto* h = static_cast<header*>(other.str) - 1;
    h->refcount.fetch_add(1, std::memory_order_relaxed);
}

string_base& string_base::operator=(const string_base& other) noexcept
{
    if (this != &other) string_base(other).swap(*this);
    return *this;
}

string_base& string_base::operator=(string_base&& other) noexcept
{
    if (this != &other) string_base(std::move(other)).swap(*this);
    return *this;
}

void string_base::swap(string_base& other) noexcept
{
    std::swap(str, other.str);
}

const void* string_base::data() const noexcept
{
    return str;
}

std::size_t string_base::size() const noexcept
{
    auto* h = static_cast<header*>(str) - 1;
    return h->length;
}
