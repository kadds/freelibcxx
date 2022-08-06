#pragma once
#include "freelibcxx/allocator.hpp"
#include "freelibcxx/assert.hpp"
#include "freelibcxx/extern.hpp"
#include "freelibcxx/formatter.hpp"
#include "freelibcxx/hash.hpp"
#include "freelibcxx/iterator.hpp"
#include "freelibcxx/span.hpp"
#include "freelibcxx/vector.hpp"
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
    struct prev_fn
    {
        CE *operator()(CE *val) { return val - 1; }
    };
    struct next_fn
    {
        CE *operator()(CE *val) { return val + 1; }
    };

  public:
    using iterator = base_bidirectional_iterator<CE *, value_fn, prev_fn, next_fn>;

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
        auto s = get_span();
        return str2int(s, base);
    }
    optional<unsigned int> to_uint(int base = 10)
    {
        auto s = get_span();
        return str2uint(s, base);
    }
    optional<int64_t> to_int64(int base = 10)
    {
        auto s = get_span();
        return str2int64(s, base);
    }
    optional<uint64_t> to_uint64(int base = 10)
    {
        auto s = get_span();
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

    span<CE> get_span() const { return ::freelibcxx::span<CE>(ptr_, len_); }

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
    template <typename N> struct prev_fn
    {
        N operator()(N val) { return val - 1; }
    };
    template <typename N> struct next_fn
    {
        N operator()(N val) { return val + 1; }
    };

    using CE = const char *;
    using NE = char *;

  public:
    using const_iterator = base_bidirectional_iterator<CE, value_fn<CE>, prev_fn<CE>, next_fn<CE>>;
    using iterator = base_bidirectional_iterator<NE, value_fn<NE>, prev_fn<NE>, next_fn<NE>>;

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

    ~string() { free(); }

    string &operator=(const string &rhs);

    string &operator=(string &&rhs);

    size_t size() const
    {
        if (is_sso()) [[likely]]
        {
            return stack_.get_count();
        }
        else
        {
            return heap_.get_count();
        }
    }

    size_t capacity() const
    {
        if (is_sso()) [[likely]]
        {
            return stack_.get_cap();
        }
        else
        {
            return heap_.get_cap();
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

    const_string_view view() const { return const_string_view(data(), size()); }

    bool is_shared() const
    {
        if (is_sso()) [[likely]]
        {
            return false;
        }
        else
        {
            return heap_.get_allocator() == nullptr;
        }
    }

    char *data()
    {
        CXXASSERT(!is_shared());
        if (is_sso()) [[likely]]
        {
            return stack_.get_buffer();
        }
        else
        {
            return heap_.get_buffer();
        }
    }
    const char *cdata() const { return data(); }

    const char *data() const
    {
        if (is_sso()) [[likely]]
        {
            return stack_.get_buffer();
        }
        else
        {
            return heap_.get_buffer();
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

    bool operator==(const string &rhs) const;
    bool operator==(const char *rhs) const;

    bool operator!=(const string &rhs) const { return !operator==(rhs); }
    bool operator!=(const char *rhs) const { return !operator==(rhs); }

    void clear() { resize(0); }
    void resize(size_t size);

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
        size_t count;
        char *buffer;
        size_t cap;
        Allocator *allocator;
        size_t get_count() const { return count & ((1UL << 63) - 1); }
        void set_count(size_t c) { count = (c & ((1UL << 63) - 1)) | (count & (1UL << 63)); };
        size_t get_cap() const { return cap & ((1UL << 63) - 1); }
        void set_cap(size_t c) { cap = (c & ((1UL << 63) - 1)) | (cap & (1UL << 63)); }
        char *get_buffer() { return buffer; }
        const char *get_buffer() const { return buffer; }
        void set_buffer(char *b) { buffer = b; }

        Allocator *get_allocator() const { return allocator; }
        void set_allocator(Allocator *alc)
        {
            CXXASSERT((reinterpret_cast<size_t>(alc) & 0x1) == 0); // bit 0
            allocator = alc;
        }
        void init()
        {
            count = 0;
            buffer = nullptr;
            cap = 0;
            allocator = 0;
        }
    };

    struct stack_t
    {
        std::byte data[24];
        Allocator *allocator;

      public:
        size_t get_count() const { return get_cap() - static_cast<size_t>(data[23]); }
        void set_count(size_t c) { data[23] = static_cast<std::byte>(get_cap() - c); }
        size_t get_cap() const { return 23; }
        char *get_buffer() { return reinterpret_cast<char *>(data); }
        const char *get_buffer() const { return reinterpret_cast<const char *>(data); }

        Allocator *get_allocator() const
        {
            return reinterpret_cast<Allocator *>(reinterpret_cast<size_t>(allocator) & ~0x1);
        }
        void set_allocator(Allocator *alc)
        {
            allocator = reinterpret_cast<Allocator *>(reinterpret_cast<size_t>(alc) | 0x1);
        }

        bool is_stack() const { return reinterpret_cast<size_t>(allocator) & 0x1; }
    };
    union
    {
        stack_t stack_;
        heap_t heap_;
    };

    static_assert(sizeof(stack_t) == sizeof(heap_t));

  private:
    size_t select_capacity(size_t capacity);

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
    stack_.set_count(0);
    stack_.set_allocator(allocator);
    stack_.get_buffer()[0] = 0;
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

inline void string::append_buffer(const char *buf, size_t length)
{
    CXXASSERT(!is_shared());
    size_t count = size();
    size_t new_count = count + length;
    char *old_buf = nullptr;
    Allocator *allocator = nullptr;
    bool is_stack_ = false;
    if (is_sso()) [[likely]]
    {
        old_buf = stack_.get_buffer();
        if (new_count >= stack_.get_cap())
        {
            // try allocate memory
            // switch to heap_
            is_stack_ = true;
            allocator = stack_.get_allocator();
        }
        else
        {
            memcpy(old_buf + count, buf, length);
            old_buf[new_count] = 0;
            stack_.set_count(new_count);
            return;
        }
    }
    else if (heap_.get_cap() > new_count)
    {
        memcpy(heap_.get_buffer() + count, buf, length);
        heap_.get_buffer()[new_count] = 0;
        heap_.set_count(new_count);
        return;
    }
    else
    {
        old_buf = heap_.get_buffer();
        allocator = heap_.get_allocator();
    }
    // append to heap_
    size_t cap = select_capacity(new_count + 1);
    char *new_buf = reinterpret_cast<char *>(allocator->allocate(cap, 8));
    memcpy(new_buf, old_buf, count);
    memcpy(new_buf + count, buf, length);
    new_buf[new_count] = 0;
    if (!is_stack_)
    {
        allocator->deallocate(old_buf);
    }
    heap_.set_allocator(allocator);
    heap_.set_cap(cap);
    heap_.set_count(new_count);
    heap_.set_buffer(new_buf);
}

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
    size_t count = size();
    CXXASSERT(count >= chars);
    size_t new_count = count - chars;
    size_t end_index = index + chars;

    char *buf = nullptr;
    if (!is_sso())
    {
        // heap_
        buf = heap_.get_buffer();
        heap_.set_count(new_count);
    }
    else
    {
        buf = stack_.get_buffer();
        stack_.set_count(new_count);
    }
    memmove(buf + index, buf + end_index, new_count - index);
    buf[new_count] = 0;
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

inline size_t string::select_capacity(size_t capacity)
{
    size_t new_cap = capacity;
    return new_cap;
}

inline void string::resize(size_t size)
{
    if (is_sso())
    {
        if (size > stack_.get_cap())
        {
            auto allocator = stack_.get_allocator();
            auto cap = select_capacity(size);
            auto buf = allocator->allocate(cap, 1);

            memcpy(buf, stack_.get_buffer(), stack_.get_count());

            heap_.set_allocator(allocator);
            heap_.set_buffer(reinterpret_cast<char *>(buf));
            heap_.set_count(size);
            heap_.set_cap(cap);
        }
        else
        {
            stack_.set_count(size);
        }
    }
    else
    {
        heap_.set_count(size);
    }
}

inline void string::free()
{
    if (is_sso()) [[likely]]
    {
        stack_.set_count(0);
        stack_.get_buffer()[0] = 0;
    }
    else if (heap_.get_buffer() != nullptr)
    {
        auto allocator = heap_.get_allocator();
        if (allocator != nullptr)
        {
            allocator->deallocate(heap_.get_buffer());
        }
        heap_.set_buffer(nullptr);
        heap_.set_count(0);
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
        auto a = rhs.heap_.get_allocator();
        size_t size = rhs.heap_.get_count();
        size_t cap = rhs.heap_.get_cap();
        heap_.set_allocator(a);
        heap_.set_count(size);
        if (a == nullptr) [[unlikely]] // shared string
        {
            heap_.set_buffer(const_cast<char *>(rhs.heap_.get_buffer()));
        }
        else
        {
            cap = select_capacity(cap);
            const char *s = rhs.heap_.get_buffer();
            char *p = reinterpret_cast<char *>(a->allocate(cap, 8));
            memcpy(p, s, size);
            p[size] = 0;
        }
        heap_.set_cap(cap);
    }
}

inline void string::move(string &&rhs)
{
    if (rhs.is_sso()) [[likely]]
    {
        this->stack_ = rhs.stack_;
        rhs.stack_.set_count(0);
        rhs.stack_.get_buffer()[0] = 0; // end \0
    }
    else
    {
        this->heap_ = rhs.heap_;
        rhs.heap_.set_count(0);
        rhs.heap_.set_buffer(nullptr);
        rhs.heap_.set_cap(0);
    }
}
inline void string::init(Allocator *allocator, const char *str, int len)
{
    heap_.init();
    if (len < 0)
        len = strlen(str);
    if ((size_t)len >= stack_.get_cap())
    {
        heap_.set_allocator(allocator);
        // go to heap_
        size_t cap = select_capacity(len + 1);
        char *buf = reinterpret_cast<char *>(allocator->allocate(cap, 8));
        memcpy(buf, str, len);
        buf[len] = 0; // end \0
        heap_.set_count(len);
        heap_.set_cap(cap);
        heap_.set_buffer(buf);
    }
    else
    {
        stack_.set_allocator(allocator);
        stack_.set_count(len);
        memcpy(stack_.get_buffer(), str, len);
        stack_.get_buffer()[len] = 0;
    }
}

inline void string::init_lit(const char *str)
{
    heap_.init();
    size_t l = strlen(str);
    heap_.set_allocator(nullptr);
    heap_.set_count(l);
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
