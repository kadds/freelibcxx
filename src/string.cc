#include "string.hpp"

namespace freelibcxx
{

string::string(Allocator *allocator)
{
    heap_.init();
    stack_.set_count(0);
    stack_.set_allocator(allocator);
    stack_.get_buffer()[0] = 0;
}

string &string::operator=(const string &rhs)
{
    if (this == &rhs)
        return *this;
    free();
    copy(rhs);
    return *this;
}

string &string::operator=(string &&rhs)
{
    if (this == &rhs)
        return *this;
    free();
    move(std::move(rhs));
    return *this;
}
void string::append_buffer(const char *buf, size_t length)
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

void string::push_back(char ch) { append_buffer(&ch, 1); }

char string::pop_back()
{
    const char *b = data();
    size_t length = size();
    CXXASSERT(length > 0);
    char c = b[length - 1];
    remove_n(length - 1, 1);
    return c;
}
char string::pop_front()
{
    const char *b = data();
    CXXASSERT(size() > 0);
    char c = b[0];
    remove_n(0, 1);
    return c;
}

void string::remove_n(size_t index, size_t chars)
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
    memcpy(buf + index, buf + end_index, new_count - index);
    buf[new_count] = 0;
}

bool string::operator==(const string &rhs) const
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
bool string::operator==(const char *rhs) const
{
    size_t len = strlen(rhs);
    if (len != size())
    {
        return false;
    }
    return memcmp(data(), rhs, size()) == 0;
}

size_t string::select_capacity(size_t capacity)
{
    size_t new_cap = capacity;
    return new_cap;
}

void string::free()
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
void string::copy(const string &rhs)
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

void string::move(string &&rhs)
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
void string::init(Allocator *allocator, const char *str, int len)
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

void string::init_lit(const char *str)
{
    heap_.init();
    size_t l = strlen(str);
    heap_.set_allocator(nullptr);
    heap_.set_count(l);
    heap_.set_cap(l + 1);
    // readonly string
    heap_.set_buffer(const_cast<char *>(str));
}

} // namespace freelibcxx
