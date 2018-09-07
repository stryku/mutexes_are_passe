#include <optional>
#include <functional>

namespace details
{
    class mutexes_are_passé : std::exception
    {
    public:
        const char* what() const noexcept override { return "Mutexes are passé"; }
    };

    template <size_t call_tag>
    void do_sync_call_for_real(std::function<void()> wrapped_callable)
    {
        try
        {
            static auto _ = [wrapped_callable]
            {
                wrapped_callable();
                throw mutexes_are_passé{};
                return 0;
            }();
        }
        catch (const mutexes_are_passé&)
        {}
    }
}

template <size_t call_tag = 0u, // User can pass a tag. Only callables with the same tag will be synced.
          typename Callable, // Obvious.
          typename... Args> // Obvious.
decltype(auto) // Let the compiler deduce return type.
sync_call(Callable&& callable, Args&&... args) // Obvious.
{
    using InvokeResult = std::invoke_result_t<Callable, Args...>; // Gather info about callable result type.

    if constexpr (std::is_same_v<void, InvokeResult>) // If result type is void, we cannot `return callable(args...);` from the function. We can not `return void{};` in C++.
    {
        auto wrapped = // Wrap callable and arguments into lambda that is convertible to std::function<void()>.
        [callable, // Obvious.
         args_tuple = std::forward_as_tuple(args...)] // Capture arguments as a tuple. Tuple will keep exact types, e.g. references, rvalue references etc.
        {
            std::apply(callable, args_tuple); // Basically call callable with arguments stored in tuple.
        };

        details::do_sync_call_for_real<call_tag>(wrapped); // It's a kind of magic. Explained later.
    }
    else
    {
        using decayed_result_t = std::decay_t<InvokeResult>; // Helper for DRY.
        using result_t = // Type that will be used to store callable result.
            std::conditional_t<std::is_lvalue_reference_v<InvokeResult>, // If callable returns lvalue reference...
                               std::reference_wrapper<decayed_result_t>, // ...store it in reference_wrapper
                               decayed_result_t>; // Else, store it as value

        std::optional<result_t> result; // Need to create optional to handle results with deleted default ctor

        auto wrapped = // Same as above.
            [&result, // Capture result by ref. It'll be initialized in lambda body
             callable, // Obvious.
             args_tuple = std::forward_as_tuple(args...)] // Same as above.
        {
            result = std::apply(callable, args_tuple); // Store callable's return in the result variable. After the call, optional is always initialized (if no exception was thrown, but that's the user problem).
        };

        details::do_sync_call_for_real<call_tag>(wrapped); // Abracadabra. Explained later.

        if constexpr(std::is_lvalue_reference_v<InvokeResult>) // If callable returns lvalue reference, store it in reference_wrapper...
        {
            return result->get(); // ...but we need to return lvalue reference. So be it.
        }
        else
        {
            return std::move(*result); // If callable returns rvalue or rvalue reference, just move the result.
        }
    }
}