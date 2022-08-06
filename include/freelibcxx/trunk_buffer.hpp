#pragma once
#include "freelibcxx/allocator.hpp"
#include "freelibcxx/extern.hpp"
#include <atomic>

namespace freelibcxx
{
/// read : write = 1 : N, linked ring buffer
class trunk_buffer
{
  public:
    struct trunk
    {
        std::byte *buffer;
        size_t offset;
        size_t size;
        trunk *next;
    };
    enum class strategy
    {
        no_wait,
        discard,
        rewrite,
    };

  private:
    size_t trunk_size_;
    size_t max_trunk_count_;
    std::atomic_ulong count_;

    Allocator *node_allocator_;
    Allocator *allocator_;
    strategy full_strategy_;

    trunk *read_trunk_, *write_trunk_;

  public:
    trunk_buffer(size_t trunk_size, size_t max_trunk_count, strategy full_strategy, Allocator *list_node_allocator,
                 Allocator *trunk_allocator);

    size_t write(const std::byte *buffer, size_t size);

    std::byte *read_buffer(size_t *read_size);

  private:
    trunk *new_trunk();
    void delete_trunk(trunk *t);
};

inline trunk_buffer::trunk_buffer(size_t trunk_size, size_t max_trunk_count, strategy full_strategy,
                                  Allocator *list_node_allocator, Allocator *trunk_allocator)
    : trunk_size_(trunk_size)
    , max_trunk_count_(max_trunk_count)
    , count_(0)
    , node_allocator_(list_node_allocator)
    , allocator_(trunk_allocator)
    , full_strategy_(full_strategy)
{
    trunk *tk = new_trunk();
    read_trunk_ = tk;
    write_trunk_ = tk;
}

inline size_t trunk_buffer::write(const std::byte *buffer, size_t size)
{
    size_t cur_write = 0;
    while (cur_write < size)
    {
        size_t ws = write_trunk_->size;
        if (ws == trunk_size_) [[unlikely]]
        {
            // new trunk
            if (count_ == max_trunk_count_) [[unlikely]]
            {
                if (full_strategy_ == strategy::discard)
                {
                    return size;
                }
                else if (full_strategy_ == strategy::rewrite)
                {
                }
                else if (full_strategy_ == strategy::no_wait)
                {
                    return cur_write;
                }
            }
            trunk *tk = write_trunk_->next;
            if (tk == nullptr)
            {
                tk = new_trunk();
                write_trunk_->next = tk;
            }
            write_trunk_ = tk;
            ws = tk->size;
            tk->size = 0;
            tk->offset = 0;
        }
        size_t rest = trunk_size_ - ws;
        if (size - cur_write < rest)
        {
            rest = size - cur_write;
        }
        memcpy(write_trunk_->buffer + ws, buffer, rest);
        write_trunk_->size += rest;
        cur_write += rest;
    }
    if (read_trunk_ == nullptr)
    {
        read_trunk_ = write_trunk_;
    }
    return cur_write;
}

inline std::byte *trunk_buffer::read_buffer(size_t *size)
{
    if (read_trunk_ == nullptr) [[unlikely]]
    {
        *size = 0;
        return nullptr;
    }
    std::byte *buffer;
    size_t off = read_trunk_->offset;
    *size = read_trunk_->size - off;
    buffer = read_trunk_->buffer + off;

    read_trunk_->offset = *size + off;

    if (read_trunk_->offset == trunk_size_)
    {
        auto cur = read_trunk_;
        read_trunk_ = read_trunk_->next;

        trunk *ft;
        do
        {
            ft = write_trunk_->next;
            cur->next = write_trunk_->next;
        } while (__sync_val_compare_and_swap(&write_trunk_->next, ft, cur) != ft);
    }
    return buffer;
}

inline trunk_buffer::trunk *trunk_buffer::new_trunk()
{
    trunk *tk = (trunk *)node_allocator_->allocate(sizeof(trunk), alignof(trunk));
    tk->buffer = (std::byte *)allocator_->allocate(trunk_size_, 1);
    tk->next = nullptr;
    count_++;
    return tk;
}

inline void trunk_buffer::delete_trunk(trunk_buffer::trunk *t)
{
    count_--;
    allocator_->deallocate(t->buffer);
    node_allocator_->deallocate(t);
}

} // namespace freelibcxx
