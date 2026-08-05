#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>

namespace freelibcxx
{

template <typename Signature> class delegate;

namespace callback_detail
{

template <typename R, bool IsNoexcept, typename... Args> class delegate_impl
{
  private:
    using stub_t = R (*)(const void *, Args...) noexcept(IsNoexcept);

    template <auto Function> static R invoke_function(const void *, Args... args) noexcept(IsNoexcept)
    {
        if constexpr (std::is_void_v<R>)
        {
            Function(std::forward<Args>(args)...);
        }
        else
        {
            return Function(std::forward<Args>(args)...);
        }
    }

    template <auto Method, typename T> static R invoke_member(const void *object, Args... args) noexcept(IsNoexcept)
    {
        auto *instance = const_cast<T *>(static_cast<const T *>(object));
        if constexpr (std::is_void_v<R>)
        {
            ((*instance).*Method)(std::forward<Args>(args)...);
        }
        else
        {
            return ((*instance).*Method)(std::forward<Args>(args)...);
        }
    }

    template <typename F> static R invoke_object(const void *object, Args... args) noexcept(IsNoexcept)
    {
        auto *callable = const_cast<F *>(static_cast<const F *>(object));
        if constexpr (std::is_void_v<R>)
        {
            (*callable)(std::forward<Args>(args)...);
        }
        else
        {
            return (*callable)(std::forward<Args>(args)...);
        }
    }

    constexpr delegate_impl(const void *object, stub_t stub) noexcept
        : object_(object)
        , stub_(stub)
    {
    }

  public:
    constexpr delegate_impl() noexcept = default;
    constexpr delegate_impl(std::nullptr_t) noexcept {}

    template <auto Function> static constexpr delegate_impl bind() noexcept
    {
        static_assert(std::is_invocable_r_v<R, decltype(Function), Args...>);
        static_assert(!IsNoexcept || std::is_nothrow_invocable_r_v<R, decltype(Function), Args...>);
        return delegate_impl(nullptr, &invoke_function<Function>);
    }

    template <auto Method, typename T> static constexpr delegate_impl bind(T &object) noexcept
    {
        static_assert(std::is_member_function_pointer_v<decltype(Method)>);
        static_assert(std::is_invocable_r_v<R, decltype(Method), T &, Args...>);
        static_assert(!IsNoexcept || std::is_nothrow_invocable_r_v<R, decltype(Method), T &, Args...>);
        return delegate_impl(&object, &invoke_member<Method, T>);
    }

    template <typename F> static constexpr delegate_impl borrow(F &callable) noexcept
    {
        static_assert(std::is_invocable_r_v<R, F &, Args...>);
        static_assert(!IsNoexcept || std::is_nothrow_invocable_r_v<R, F &, Args...>);
        return delegate_impl(&callable, &invoke_object<F>);
    }

    constexpr explicit operator bool() const noexcept { return stub_ != nullptr; }

    constexpr bool operator==(std::nullptr_t) const noexcept { return stub_ == nullptr; }
    constexpr bool operator!=(std::nullptr_t) const noexcept { return stub_ != nullptr; }

    R operator()(Args... args) const noexcept(IsNoexcept)
    {
        if constexpr (std::is_void_v<R>)
        {
            stub_(object_, std::forward<Args>(args)...);
        }
        else
        {
            return stub_(object_, std::forward<Args>(args)...);
        }
    }

  private:
    const void *object_{};
    stub_t stub_{};
};

} // namespace callback_detail

template <typename R, typename... Args>
class delegate<R(Args...)> : public callback_detail::delegate_impl<R, false, Args...>
{
    using base = callback_detail::delegate_impl<R, false, Args...>;

  public:
    using base::base;
    using base::operator();

    constexpr delegate(const base &other) noexcept
        : base(other)
    {
    }

    constexpr delegate(base &&other) noexcept
        : base(std::move(other))
    {
    }

    template <auto Function> static constexpr delegate bind() noexcept
    {
        return delegate(base::template bind<Function>());
    }

    template <auto Method, typename T> static constexpr delegate bind(T &object) noexcept
    {
        return delegate(base::template bind<Method>(object));
    }

    template <typename F> static constexpr delegate borrow(F &callable) noexcept
    {
        return delegate(base::borrow(callable));
    }
};

template <typename R, typename... Args>
class delegate<R(Args...) noexcept> : public callback_detail::delegate_impl<R, true, Args...>
{
    using base = callback_detail::delegate_impl<R, true, Args...>;

  public:
    using base::base;
    using base::operator();

    constexpr delegate(const base &other) noexcept
        : base(other)
    {
    }

    constexpr delegate(base &&other) noexcept
        : base(std::move(other))
    {
    }

    template <auto Function> static constexpr delegate bind() noexcept
    {
        return delegate(base::template bind<Function>());
    }

    template <auto Method, typename T> static constexpr delegate bind(T &object) noexcept
    {
        return delegate(base::template bind<Method>(object));
    }

    template <typename F> static constexpr delegate borrow(F &callable) noexcept
    {
        return delegate(base::borrow(callable));
    }
};

} // namespace freelibcxx
