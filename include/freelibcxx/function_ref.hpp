#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>

namespace freelibcxx
{

template <typename Signature> class function_ref;

namespace callback_detail
{

template <typename R, bool IsNoexcept, typename... Args> class function_ref_impl
{
  private:
    using stub_t = R (*)(const void *, Args...) noexcept(IsNoexcept);
    using function_t = R (*)(Args...) noexcept(IsNoexcept);

    template <typename F>
    static R invoke_object(const void *object, Args... args) noexcept(IsNoexcept)
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

    static R invoke_function(const void *object, Args... args) noexcept(IsNoexcept)
    {
        auto function = reinterpret_cast<function_t>(object);
        if constexpr (std::is_void_v<R>)
        {
            function(std::forward<Args>(args)...);
        }
        else
        {
            return function(std::forward<Args>(args)...);
        }
    }

  public:
    constexpr function_ref_impl() noexcept = default;
    constexpr function_ref_impl(std::nullptr_t) noexcept {}

    template <typename F>
    requires(!std::is_function_v<F> && !std::is_same_v<std::remove_cvref_t<F>, function_ref_impl> &&
             std::is_invocable_r_v<R, F &, Args...> &&
             (!IsNoexcept || std::is_nothrow_invocable_r_v<R, F &, Args...>))
    constexpr function_ref_impl(F &callable) noexcept
        : object_(&callable)
        , stub_(&invoke_object<F>)
    {
    }

    template <typename F>
    requires(std::is_function_v<F> && std::is_invocable_r_v<R, F &, Args...> &&
             (!IsNoexcept || std::is_nothrow_invocable_r_v<R, F &, Args...>))
    constexpr function_ref_impl(F &function) noexcept
        : object_(reinterpret_cast<const void *>(+function))
        , stub_(function == nullptr ? nullptr : &invoke_function)
    {
    }

    constexpr function_ref_impl(function_t function) noexcept
        : object_(reinterpret_cast<const void *>(function))
        , stub_(function == nullptr ? nullptr : &invoke_function)
    {
    }

    constexpr explicit operator bool() const noexcept { return stub_ != nullptr; }

    constexpr bool operator==(std::nullptr_t) const noexcept { return stub_ == nullptr; }
    constexpr bool operator!=(std::nullptr_t) const noexcept { return stub_ != nullptr; }

    constexpr bool operator==(const function_ref_impl &other) const noexcept
    {
        return object_ == other.object_ && stub_ == other.stub_;
    }

    constexpr bool operator!=(const function_ref_impl &other) const noexcept { return !(*this == other); }

    R operator()(Args... args) const noexcept(IsNoexcept)
    {
        // Calling an empty function_ref is a contract violation.  It is kept
        // branch-free here because this type is also used in kernel paths.
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
class function_ref<R(Args...)> : public callback_detail::function_ref_impl<R, false, Args...>
{
    using base = callback_detail::function_ref_impl<R, false, Args...>;

  public:
    using base::base;
    using base::operator();
};

template <typename R, typename... Args>
class function_ref<R(Args...) noexcept> : public callback_detail::function_ref_impl<R, true, Args...>
{
    using base = callback_detail::function_ref_impl<R, true, Args...>;

  public:
    using base::base;
    using base::operator();
};

} // namespace freelibcxx
