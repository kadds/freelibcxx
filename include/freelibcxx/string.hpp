#pragma once
#include "freelibcxx/algorithm.hpp"
#include "freelibcxx/allocator.hpp"
#include "freelibcxx/assert.hpp"
#include "freelibcxx/extern.hpp"
#include "freelibcxx/formatter.hpp"
#include "freelibcxx/hash.hpp"
#include "freelibcxx/iterator.hpp"
#include "freelibcxx/span.hpp"
#include "freelibcxx/vector.hpp"
#include <cstddef>
#include <utility>

namespace freelibcxx
{

class string;

template <typename CE> class base_string_view
{
  private:
    struct value_fn
    {
        CE *operator()(CE *val) { return val; }
    };
    struct random_fn
    {
        CE *operator[](ptrdiff_t index) { return val_ + index; }

        ptrdiff_t offset_of(CE *val) { return val_ - val; }

        CE *val_;
        random_fn(CE *val)
            : val_(val)
        {
        }
    };

  public:
    using iterator = base_random_access_iterator<CE *, value_fn, random_fn>;

    base_string_view()
        : ptr_(nullptr)
        , len_(0)
    {
    }
    base_string_view(CE *ptr, size_t len)
        : ptr_(ptr)
        , len_(len)
    {
    }
    // from cstr
    base_string_view(CE *ptr)
        : ptr_(ptr)
        , len_(strlen(ptr))
    {
    }

    CE *data() { return ptr_; }

    string to_string(Allocator *allocator);

    iterator begin() { return iterator(ptr_); }

    iterator end() { return iterator(ptr_ + len_); }

    CE &first() { return ptr_[0]; }

    CE &last() { return ptr_[len_ - 1]; }

    base_string_view substr(size_t pos, size_t len)
    {
        CXXASSERT(pos < len_ && pos + len < len_);
        return base_string_view(ptr_ + pos, len);
    }

    template <size_t N> size_t split_n(char c, base_string_view views[N])
    {
        CE *p = ptr_;
        CE *prev = p;
        size_t cnt = 0;
        for (size_t i = 0; i < len_ && cnt < N; i++)
        {
            if (*p == c)
            {
                if (prev < p)
                {
                    views[cnt++] = base_string_view(prev, p - prev);
                }
                prev = p + 1;
            }
            p++;
        }
        if (cnt < N)
        {
            views[cnt++] = base_string_view(prev, p - prev);
        }
        return cnt;
    }

    vector<base_string_view<CE>> split(char c, Allocator *vec_allocator)
    {
        vector<base_string_view<CE>> vec(vec_allocator);
        CE *p = ptr_;
        CE *prev = p;
        for (size_t i = 0; i < len_; i++)
        {
            if (*p == c)
            {
                if (prev < p)
                {
                    vec.push_back(base_string_view<CE>(prev, p - prev));
                }
                prev = p + 1;
            }
            p++;
        }
        vec.push_back(base_string_view(prev, p - prev));
        return vec;
    }

    optional<int> to_int(int base = 10)
    {
        auto s = span();
        return str2int(s, base);
    }
    optional<unsigned int> to_uint(int base = 10)
    {
        auto s = span();
        return str2uint(s, base);
    }
    optional<int64_t> to_int64(int base = 10)
    {
        auto s = span();
        return str2int64(s, base);
    }
    optional<uint64_t> to_uint64(int base = 10)
    {
        auto s = span();
        return str2uint64(s, base);
    }

    bool operator==(const string &rhs) const;
    bool operator==(const base_string_view &rhs) const
    {
        if (len_ != rhs.len_)
        {
            return false;
        }
        return memcmp(ptr_, rhs.ptr_, len_) == 0;
    }

    bool operator==(const char *rhs) const
    {
        size_t len = strlen(rhs);
        if (len != len_)
        {
            return false;
        }
        return memcmp(ptr_, rhs, len) == 0;
    }

    bool operator!=(const string &rhs) const { return !operator==(rhs); }
    bool operator!=(const base_string_view &rhs) const { return !operator==(rhs); }
    bool operator!=(const char *rhs) const { return !operator==(rhs); }

    char operator[](size_t index) const
    {
        CXXASSERT(index < len_);
        return ptr_[len_];
    }

    char &operator[](size_t index)
    {
        CXXASSERT(index < len_);
        return ptr_[len_];
    }

    ::freelibcxx::span<CE> span() const { return ::freelibcxx::span<CE>(ptr_, len_); }

  private:
    CE *ptr_;
    size_t len_;
};

using string_view = base_string_view<char>;
using const_string_view = base_string_view<const char>;

class string
{
  private:
    template <typename N> struct value_fn
    {
        N operator()(N val) { return val; }
    };


    template <typename N> struct random_fn
    {
        N operator[](ptrdiff_t index) { return val_ + index; }

        ptrdiff_t offset_of(N val) { return val_ - val; }

        N val_;
        random_fn(N val)
            : val_(val)
        {
        }
    };

    using CE = const char *;
    using NE = char *;

  public:
    using const_iterator = base_random_access_iterator<CE, value_fn<CE>, random_fn<CE>>;
    using iterator = base_random_access_iterator<NE, value_fn<NE>,  random_fn<NE>>;

    string(const string &rhs) { copy(rhs); }

    string(string &&rhs) { move(std::move(rhs)); }

    ///\brief init empty string ""
    string(Allocator *allocator);

    ///\brief init from char array
    /// no_shared
    string(Allocator *allocator, const char *str, int len = -1) { init(allocator, str, len); }

    ///\brief init from char array lit
    /// readonly & shared
    static string from_cstr(const char *str)
    {
        string s(nullptr);
        s.init_lit(str);
        return s;
    }

    string(const_string_view view) { init_lit(view.data()); }

    string(string_view view) { init_lit(view.data()); }

    ~string() { free(); }

    string &operator=(const string &rhs);

    string &operator=(string &&rhs);

    size_t size() const
    {
        if (is_sso()) [[likely]]
        {
            return stack_.size();
        }
        else
        {
            return heap_.size();
        }
    }

    size_t capacity() const
    {
        if (is_sso()) [[likely]]
        {
            return stack_.cap();
        }
        else
        {
            return heap_.cap();
        }
    }

    iterator begin() { return iterator(data()); }

    iterator end() { return iterator(data() + size()); }

    const_iterator begin() const { return const_iterator(data()); }

    const_iterator end() const { return const_iterator(data() + size()); }

    string_view view()
    {
        CXXASSERT(!is_shared());
        return string_view(data(), size());
    }

    ::freelibcxx::span<const char> span() const
    {
        CXXASSERT(!is_shared());
        return ::freelibcxx::span(data(), size());
    }

    ::freelibcxx::span<char> span()
    {
        CXXASSERT(!is_shared());
        return ::freelibcxx::span(data(), size());
    }

    const_string_view view() const { return const_string_view(data(), size()); }

    bool is_shared() const
    {
        if (is_sso()) [[likely]]
        {
            return false;
        }
        else
        {
            return heap_.allocator() == nullptr;
        }
    }

    char *data()
    {
        CXXASSERT(!is_shared());
        if (is_sso()) [[likely]]
        {
            return stack_.buffer();
        }
        else
        {
            return heap_.buffer();
        }
    }
    const char *cdata() const { return data(); }

    const char *data() const
    {
        if (is_sso()) [[likely]]
        {
            return stack_.buffer();
        }
        else
        {
            return heap_.buffer();
        }
    }

    void append(const string &rhs) { append_buffer(rhs.data(), rhs.size()); }

    void append_buffer(const char *buf, size_t length);

    void push_back(char ch);

    char pop_back();
    char pop_front();

    void remove_at(size_t index, size_t end_index)
    {
        CXXASSERT(index < end_index);
        remove_n(index, end_index - index);
    }
    void remove_n(size_t index, size_t chars);

    void remove(iterator beg, iterator end)
    {
        size_t index = beg.get() - data();
        size_t index_end = end.get() - data();
        remove_at(index, index_end);
    }

    char at(size_t index) const
    {
        CXXASSERT(index < size());
        return data()[index];
    }

    string &operator+=(const string &rhs)
    {
        append(rhs);
        return *this;
    }
    string &operator+=(const char *rhs)
    {
        append_buffer(rhs, (size_t)strlen(rhs));
        return *this;
    }
    string &operator+=(char ch)
    {
        append_buffer(&ch, 1);
        return *this;
    }

    bool operator==(const string &rhs) const;
    bool operator==(const char *rhs) const;

    bool operator!=(const string &rhs) const { return !operator==(rhs); }
    bool operator!=(const char *rhs) const { return !operator==(rhs); }

    void clear() { resize(0); }
    void resize(size_t size);

    void ensure(size_t cap);

    void insert(iterator iter, const char *buf, size_t len) { insert_at(iter - begin(), buf, len); }

    void insert_at(size_t index, const char *buf, size_t len);

    void from_int(int val, int base = 10)
    {
        char buf[32];
        ::freelibcxx::span<char> span(buf, sizeof(buf));
        int len = int2str(span, val, base).value_or(0);
        append_buffer(buf, len);
    }

    void from_uint(unsigned int val, int base = 10)
    {
        char buf[32];
        ::freelibcxx::span<char> span(buf, sizeof(buf));
        int len = uint2str(span, val, base).value_or(0);
        append_buffer(buf, len);
    }

    void from_int64(int64_t val, int base = 10)
    {
        char buf[48];
        ::freelibcxx::span<char> span(buf, sizeof(buf));
        int len = int642str(span, val, base).value_or(0);
        append_buffer(buf, len);
    }

    void from_uint64(uint64_t val, int base = 10)
    {
        char buf[48];
        ::freelibcxx::span<char> span(buf, sizeof(buf));
        int len = uint642str(span, val, base).value_or(0);
        append_buffer(buf, len);
    }

  private:
    // little endian machine
    //      0x0
    // |-----------------|---------------|
    // |  count(63)      |  char(64) 8   |
    // | flag_shared(1)  |               |
    // |-----------------|---------------|
    // | buffer(64)     |  char(64) 8    |
    // |----------------|----------------|
    // | cap(63)        |  char(56) 7    |
    // | none(1)        |   count(5)     |
    // |----------------|----------------|
    // |  flag_type(1)0 |  flag_type(1)1 |
    // |  allocator(63) |  allocator(63) |
    // |----------------|----------------|
    // 0x1F

    struct heap_t
    {
        size_t size_;
        char *buffer_;
        size_t cap_;
        Allocator *allocator_;
        size_t size() const { return size_; }
        void set_size(size_t c) { size_ = c; };

        size_t cap() const { return cap_; }
        void set_cap(size_t c) { cap_ = c; }

        char *buffer() { return buffer_; }
        const char *buffer() const { return buffer_; }
        void set_buffer(char *b) { buffer_ = b; }

        Allocator *allocator() const { return allocator_; }
        void set_allocator(Allocator *a)
        {
            CXXASSERT((reinterpret_cast<uintptr_t>(a) & 0x1) == 0); // bit 0
            allocator_ = a;
        }
        void init()
        {
            size_ = 0;
            buffer_ = nullptr;
            cap_ = 0;
            allocator_ = 0;
        }
    };

    struct stack_t
    {
        char data_[24];
        Allocator *allocator_;

      public:
        size_t size() const { return cap() - static_cast<uint8_t>(data_[23]); }
        void set_size(size_t c) { data_[23] = cap() - c; }
        size_t cap() const { return 23; }
        char *buffer() { return data_; }
        const char *buffer() const { return data_; }

        Allocator *allocator() const
        {
            return reinterpret_cast<Allocator *>(reinterpret_cast<uintptr_t>(allocator_) & ~0x1);
        }
        void set_allocator(Allocator *alc)
        {
            allocator_ = reinterpret_cast<Allocator *>(reinterpret_cast<uintptr_t>(alc) | 0x1);
        }

        bool is_stack() const { return reinterpret_cast<uintptr_t>(allocator_) & 0x1; }
        void init()
        {
            allocator_ = 0;
            set_size(0);
        }
    };
    union
    {
        stack_t stack_;
        heap_t heap_;
    };

    static_assert(sizeof(stack_t) == sizeof(heap_t));

  private:
    void free();

    void copy(const string &rhs);

    void move(string &&rhs);

    void init(Allocator *allocator, const char *str, int len = -1);

    void init_lit(const char *str);

  private:
    bool is_sso() const { return stack_.is_stack(); }
};

inline string::string(Allocator *allocator)
{
    heap_.init();
    stack_.set_size(0);
    stack_.set_allocator(allocator);
    stack_.buffer()[0] = 0;
}
inline string &string::operator=(const string &rhs)
{
    if (this == &rhs)
        return *this;
    free();
    copy(rhs);
    return *this;
}

inline string &string::operator=(string &&rhs)
{
    if (this == &rhs)
        return *this;
    free();
    move(std::move(rhs));
    return *this;
}

inline void string::append_buffer(const char *buf, size_t length) { insert_at(size(), buf, length); }

inline void string::push_back(char ch) { append_buffer(&ch, 1); }

inline char string::pop_back()
{
    const char *b = data();
    size_t length = size();
    CXXASSERT(length > 0);
    char c = b[length - 1];
    remove_n(length - 1, 1);
    return c;
}
inline char string::pop_front()
{
    const char *b = data();
    CXXASSERT(size() > 0);
    char c = b[0];
    remove_n(0, 1);
    return c;
}

inline void string::remove_n(size_t index, size_t chars)
{
    CXXASSERT(!is_shared());
    size_t s = size();
    CXXASSERT(s >= chars);
    size_t new_size = s - chars;
    size_t end_index = index + chars;

    char *buf = nullptr;
    if (!is_sso())
    {
        // heap_
        buf = heap_.buffer();
        heap_.set_size(new_size);
    }
    else
    {
        buf = stack_.buffer();
        stack_.set_size(new_size);
    }
    memmove(buf + index, buf + end_index, new_size - index);
    buf[new_size] = 0;
}

inline bool string::operator==(const string &rhs) const
{
    if (&rhs == this) [[likely]]
        return true;
    size_t lc = size();
    size_t rc = rhs.size();
    if (lc != rc)
    {
        return false;
    }
    const char *l = data();
    const char *r = rhs.data();
    return memcmp(l, r, lc) == 0;
}
inline bool string::operator==(const char *rhs) const
{
    size_t len = strlen(rhs);
    if (len != size())
    {
        return false;
    }
    return memcmp(data(), rhs, size()) == 0;
}

inline void string::resize(size_t size)
{
    ensure(size);
    size_t s;
    char *buf;

    if (is_sso())
    {
        s = stack_.size();
        buf = stack_.buffer();
    }
    else
    {
        s = heap_.size();
        buf = heap_.buffer();
    }
    if (size > s)
    {
        memset(buf + s, 0, size - s);
    }

    if (is_sso())
    {
        stack_.set_size(size);
    }
    else
    {
        heap_.set_size(size);
    }
}

inline void string::insert_at(size_t index, const char *buf, size_t len)
{
    size_t s = size();
    size_t total_size = s + len;
    CXXASSERT(total_size >= s);
    CXXASSERT(total_size >= len);
    CXXASSERT(index <= s);

    ensure(total_size);
    char *b;
    if (is_sso()) [[likely]]
    {
        b = stack_.buffer();
    }
    else
    {
        b = heap_.buffer();
    }
    memmove(b + index + len, b + index, s - index);
    memcpy(b + index, buf, len);
    b[total_size] = 0;
    if (is_sso()) [[likely]]
    {
        stack_.set_size(total_size);
    }
    else
    {
        heap_.set_size(total_size);
    }
}

inline void string::ensure(size_t cap)
{
    cap = select_capacity(cap);
    if (is_sso()) [[likely]]
    {
        if (cap < stack_.cap())
        {
            return;
        }
        auto size = stack_.size();
        auto allocator = stack_.allocator();
        auto buf = allocator->allocate(cap, 1);

        memcpy(buf, stack_.buffer(), size);

        heap_.set_allocator(allocator);
        heap_.set_buffer(reinterpret_cast<char *>(buf));
        heap_.set_size(size);
        heap_.set_cap(cap);
    }
    else
    {
        if (cap < heap_.cap())
        {
            return;
        }
        auto allocator = heap_.allocator();
        auto buf = allocator->allocate(cap, 1);
        auto old_buf = heap_.buffer();
        memcpy(buf, old_buf, heap_.size());

        heap_.set_buffer(reinterpret_cast<char *>(buf));
        heap_.set_cap(cap);

        allocator->deallocate(old_buf);
    }
}

inline void string::free()
{
    if (is_sso()) [[likely]]
    {
        stack_.set_size(0);
        stack_.buffer()[0] = 0;
    }
    else if (heap_.buffer() != nullptr)
    {
        auto allocator = heap_.allocator();
        if (allocator != nullptr)
        {
            allocator->deallocate(heap_.buffer());
        }
        heap_.set_buffer(nullptr);
        heap_.set_size(0);
        heap_.set_cap(0);
    }
}

inline void string::copy(const string &rhs)
{
    if (rhs.is_sso()) [[likely]]
    {
        this->stack_ = rhs.stack_;
    }
    else
    {
        auto a = rhs.heap_.allocator();
        size_t size = rhs.heap_.size();
        size_t cap = rhs.heap_.cap();
        heap_.set_allocator(a);
        heap_.set_size(size);
        if (a == nullptr) [[unlikely]] // shared string
        {
            heap_.set_buffer(const_cast<char *>(rhs.heap_.buffer()));
        }
        else
        {
            stack_.init();
            stack_.set_allocator(a);
            append_buffer(rhs.heap_.buffer(), size);
        }
        heap_.set_cap(cap);
    }
}

inline void string::move(string &&rhs)
{
    if (rhs.is_sso()) [[likely]]
    {
        this->stack_ = rhs.stack_;
        rhs.stack_.set_size(0);
        rhs.stack_.buffer()[0] = 0; // end \0
    }
    else
    {
        this->heap_ = rhs.heap_;
        rhs.heap_.set_size(0);
        rhs.heap_.set_buffer(nullptr);
        rhs.heap_.set_cap(0);
    }
}
inline void string::init(Allocator *allocator, const char *str, int len)
{
    if (len < 0)
        len = strlen(str);

    stack_.init();
    stack_.set_allocator(allocator);

    append_buffer(str, len);
}

inline void string::init_lit(const char *str)
{
    heap_.init();
    size_t l = strlen(str);
    heap_.set_allocator(nullptr);
    heap_.set_size(l);
    heap_.set_cap(l + 1);
    // readonly string
    heap_.set_buffer(const_cast<char *>(str));
}

template <typename CE> string base_string_view<CE>::to_string(Allocator *allocator)
{
    return string(allocator, ptr_, len_);
}

template <typename CE> bool base_string_view<CE>::operator==(const string &rhs) const
{
    return operator==()(rhs.view());
}

inline string &operator<<(string &s, int val)
{
    char buf[16];
    auto len = int2str(span(buf, sizeof(buf) - 1), val);
    if (len.has_value())
    {
        s.append_buffer(buf, len.value());
    }
    return s;
}

inline string &operator<<(string &s, unsigned int val)
{
    char buf[16];
    auto len = uint2str(span(buf, sizeof(buf) - 1), val);
    if (len.has_value())
    {
        s.append_buffer(buf, len.value());
    }
    return s;
}

inline string &operator<<(string &s, uint64_t val)
{
    char buf[32];
    auto len = uint642str(span(buf, sizeof(buf) - 1), val);
    if (len.has_value())
    {
        s.append_buffer(buf, len.value());
    }
    return s;
}

inline string &operator<<(string &s, int64_t val)
{
    char buf[32];
    auto len = int642str(span(buf, sizeof(buf) - 1), val);
    if (len.has_value())
    {
        s.append_buffer(buf, len.value());
    }
    return s;
}

template <> struct hasher<string>
{
    size_t operator()(const string &t) { return detail::murmur_hash2_64(t.data(), t.size(), 0); }
};

template <> struct hasher<string_view>
{
    size_t operator()(const string &t) { return detail::murmur_hash2_64(t.data(), t.size(), 0); }
};

template <> struct hasher<const_string_view>
{
    size_t operator()(const string &t) { return detail::murmur_hash2_64(t.data(), t.size(), 0); }
};

} // namespace freelibcxx
