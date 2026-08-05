#pragma once

#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

namespace freelibcxx
{

template <typename Signature, size_t Capacity> class inplace_function;

namespace callback_detail
{

template <typename R, bool IsNoexcept, size_t Capacity, typename... Args> class inplace_function_impl
{
    static_assert(Capacity != 0, "inplace_function requires non-zero storage");

    using invoke_t = R (*)(void *, Args...) noexcept(IsNoexcept);
    using destroy_t = void (*)(void *) noexcept;
    using move_t = void (*)(void *, void *) noexcept;

    template <typename F>
    static R invoke(void *storage, Args... args) noexcept(IsNoexcept)
    {
        auto *callable = static_cast<F *>(storage);
        if constexpr (std::is_void_v<R>)
        {
            (*callable)(std::forward<Args>(args)...);
        }
        else
        {
            return (*callable)(std::forward<Args>(args)...);
        }
    }

    template <typename F> static void destroy(void *storage) noexcept { static_cast<F *>(storage)->~F(); }

    template <typename F> static void move(void *destination, void *source) noexcept
    {
        auto *from = static_cast<F *>(source);
        new (destination) F(std::move(*from));
    }

  public:
    inplace_function_impl() noexcept = default;
    inplace_function_impl(std::nullptr_t) noexcept {}

    template <typename F>
    requires(!std::is_same_v<std::remove_cvref_t<F>, inplace_function_impl>)
    explicit inplace_function_impl(F &&callable) noexcept
    {
        using callable_t = std::remove_cvref_t<F>;
        static_assert(sizeof(callable_t) <= Capacity, "callable does not fit in inplace_function");
        static_assert(alignof(callable_t) <= alignof(storage_t), "callable alignment is too strict");
        static_assert(std::is_nothrow_constructible_v<callable_t, F>, "callable construction must not throw");
        static_assert(std::is_nothrow_destructible_v<callable_t>, "callable destruction must not throw");
        static_assert(std::is_nothrow_move_constructible_v<callable_t>, "callable move must not throw");
        static_assert(std::is_invocable_r_v<R, callable_t &, Args...>);
        static_assert(!IsNoexcept || std::is_nothrow_invocable_r_v<R, callable_t &, Args...>);

        new (&storage_) callable_t(std::forward<F>(callable));
        invoke_ = static_cast<invoke_t>(&invoke<callable_t>);
        destroy_ = &destroy<callable_t>;
        move_ = &move<callable_t>;
    }

    inplace_function_impl(const inplace_function_impl &) = delete;
    inplace_function_impl &operator=(const inplace_function_impl &) = delete;

    inplace_function_impl(inplace_function_impl &&other) noexcept { move_from(other); }

    inplace_function_impl &operator=(inplace_function_impl &&other) noexcept
    {
        if (this != &other)
        {
            reset();
            move_from(other);
        }
        return *this;
    }

    ~inplace_function_impl() { reset(); }

    explicit operator bool() const noexcept { return invoke_ != nullptr; }
    bool operator==(std::nullptr_t) const noexcept { return invoke_ == nullptr; }
    bool operator!=(std::nullptr_t) const noexcept { return invoke_ != nullptr; }

    R operator()(Args... args) noexcept(IsNoexcept)
    {
        if constexpr (std::is_void_v<R>)
        {
            invoke_(&storage_, std::forward<Args>(args)...);
        }
        else
        {
            return invoke_(&storage_, std::forward<Args>(args)...);
        }
    }

    void reset() noexcept
    {
        if (destroy_ != nullptr)
        {
            destroy_(&storage_);
            invoke_ = nullptr;
            destroy_ = nullptr;
            move_ = nullptr;
        }
    }

  private:
    using storage_t = std::aligned_storage_t<Capacity, alignof(std::max_align_t)>;

    void move_from(inplace_function_impl &other) noexcept
    {
        if (other.invoke_ == nullptr)
            return;
        other.move_(&storage_, &other.storage_);
        invoke_ = other.invoke_;
        destroy_ = other.destroy_;
        move_ = other.move_;
        other.reset();
    }

    storage_t storage_{};
    invoke_t invoke_{};
    destroy_t destroy_{};
    move_t move_{};
};

} // namespace callback_detail

template <typename R, typename... Args, size_t Capacity>
class inplace_function<R(Args...), Capacity> : public callback_detail::inplace_function_impl<R, false, Capacity, Args...>
{
    using base = callback_detail::inplace_function_impl<R, false, Capacity, Args...>;

  public:
    using base::base;
    using base::operator();
    using base::operator bool;
    using base::operator!=;
    using base::operator==;
    using base::reset;
};

template <typename R, typename... Args, size_t Capacity>
class inplace_function<R(Args...) noexcept, Capacity>
    : public callback_detail::inplace_function_impl<R, true, Capacity, Args...>
{
    using base = callback_detail::inplace_function_impl<R, true, Capacity, Args...>;

  public:
    using base::base;
    using base::operator();
    using base::operator bool;
    using base::operator!=;
    using base::operator==;
    using base::reset;
};

} // namespace freelibcxx
