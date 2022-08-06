#pragma once
#include "freelibcxx/allocator.hpp"
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
} // namespace freelibcxx
