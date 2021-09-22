#ifndef ING_STRING_HPP
#define ING_STRING_HPP

#include <string_view>

namespace ing
{
    class string_base
    {
        void* str;

    public:
        ~string_base();
        string_base() noexcept;
        string_base(const void* buf, std::size_t count, std::size_t size);
        string_base(const string_base& other) noexcept;
        string_base(string_base&& other) noexcept;
        string_base& operator=(const string_base& other) noexcept;
        string_base& operator=(string_base&& other) noexcept;
        void swap(string_base& other) noexcept;
        const void* data() const noexcept;
        std::size_t size() const noexcept;
    };
    
    template<typename CharT, typename Traits = std::char_traits<CharT>>
    class basic_string : string_base
    {
        static_assert(sizeof(string_base) == sizeof(CharT*));

    public:
        basic_string() noexcept = default;
        basic_string(const CharT* s) : basic_string(std::basic_string_view<CharT, Traits>(s)) {}
        basic_string(std::basic_string_view<CharT, Traits> s) : string_base(s.data(), s.size(), sizeof(CharT)) {}
        const CharT* data() const noexcept { return static_cast<const CharT*>(string_base::data()); }
        std::size_t size() const noexcept { return string_base::size(); }
        bool empty() const noexcept { return size() == 0; }
        const CharT* c_str() const noexcept { return data(); }
        std::basic_string_view<CharT, Traits> view() const noexcept { return { data(), size() }; }
        operator std::basic_string_view<CharT, Traits>() const noexcept { return view(); }
        void swap(basic_string& other) noexcept { return string_base::swap(other); }
    };

    using string = basic_string<char>;
    using wstring = basic_string<wchar_t>;
    using u16string = basic_string<char16_t>;
    using u32string = basic_string<char32_t>;
#if defined(__cpp_char8_t)
    using u8string = basic_string<char8_t>;
#endif
}

#endif
