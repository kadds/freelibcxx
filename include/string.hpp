#pragma once
#include "allocator.hpp"
#include "assert.hpp"
#include "extern.hpp"
#include "iterator.hpp"
#include "vector.hpp"
#include <utility>

namespace freelibcxx
{

class string;

template <typename CE> class base_string_view
{
  private:
    struct value_fn
    {
        CE operator()(CE val) { return val; }
    };
    struct prev_fn
    {
        CE operator()(CE val) { return val - 1; }
    };
    struct next_fn
    {
        CE operator()(CE val) { return val + 1; }
    };

  public:
    using iterator = base_bidirectional_iterator<CE, value_fn, prev_fn, next_fn>;

    base_string_view()
        : ptr_(nullptr)
        , len_(0)
    {
    }
    base_string_view(CE ptr, size_t len)
        : ptr_(ptr)
        , len_(len)
    {
    }

    CE data() { return ptr_; }

    string to_string(Allocator *allocator);

    iterator begin() { return iterator(ptr_); }

    iterator end() { return iterator(ptr_ + len_); }

    void split2(base_string_view &v0, base_string_view &v1, iterator iter)
    {
        v0 = base_string_view(ptr_, iter.get() - ptr_);
        v1 = base_string_view(iter.get() + 1, len_ - (iter.get() - ptr_));
    }

    vector<base_string_view<CE>> split(char c, Allocator *vec_allocator)
    {
        vector<base_string_view<CE>> vec(vec_allocator);
        CE p = ptr_;
        CE prev = p;
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
        if (prev < p)
        {
            vec.push_back(base_string_view<CE>(prev, p - prev));
        }
        return vec;
    }

  private:
    CE ptr_;
    size_t len_;
};

using string_view = base_string_view<char *>;
using const_string_view = base_string_view<const char *>;

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
    string(const char *str) { init_lit(str); }

    ~string() { free(); }

    string &operator=(const string &rhs);

    string &operator=(string &&rhs);

    size_t size() const { return get_count(); }

    size_t capacity() const
    {
        if (is_sso()) [[unlikely]]
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

    string_view view() { return string_view(data(), size()); }

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
        if (is_sso()) [[likely]]
        {
            return stack_.get_buffer();
        }
        else
        {
            return heap_.get_buffer();
        }
    }

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

    void remove_at(size_t index, size_t end_index);

    void remove(iterator beg, iterator end)
    {
        size_t index = beg.get() - data();
        size_t index_end = end.get() - data();
        remove_at(index, index_end);
    }

    char at(size_t index) const
    {
        // assert(index < get_count());
        return data()[index];
    }

    string &operator+=(const string &rhs)
    {
        append(rhs);
        return *this;
    }

    bool operator==(const string &rhs) const;

    bool operator!=(const string &rhs) const { return !operator==(rhs); }

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
            // assert((reinterpret_cast<size_t>(alc) & 0x1) == 0); // bit 0
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

    size_t get_count() const
    {
        if (is_sso())
            return stack_.get_count();
        else
            return heap_.get_count();
    }

  private:
    bool is_sso() const { return stack_.is_stack(); }
};

template <typename CE> string base_string_view<CE>::to_string(Allocator *allocator)
{
    return string(allocator, ptr_, len_);
}

} // namespace freelibcxx
