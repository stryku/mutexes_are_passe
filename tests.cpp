#include "mutexes_are_passé.hpp"

#include <iostream>
#include <thread>

bool test_sum()
{
    int sum{};
    int sum2{};

    auto concurrent_fun = [&sum]
    {
        sum += 3;
    };

    auto concurrent_fun2 = [&sum2]
    {
        sum2 += 3;
        return 0;
    };

    auto fun = [concurrent_fun]
    {
        for(auto i = 0u; i < 10000; ++i)
        {
            sync_call(concurrent_fun);
        }
    };

    auto t1 = std::thread{ fun };
    auto t2 = std::thread{ fun };

    t1.join();
    t2.join();

    return sum + sum2 == 2*10000*3;
}

bool test_reference_return()
{
    std::vector<std::unique_ptr<int>> v;

    auto push_back_and_get_ref = [&v](const auto val_to_push) -> int&
    {
        v.emplace_back(std::make_unique<int>(val_to_push));
        return *v.back();
    };

    auto fun = [push_back_and_get_ref, &v]
    {
        for(auto i = 0u; i < 30000; ++i)
        {
            // Gather reference wrapper to pushed val
            auto& new_val = sync_call(push_back_and_get_ref, 0);
            new_val = 1;
        }
    };

    auto t1 = std::thread{ fun };
    auto t2 = std::thread{ fun };

    t1.join();
    t2.join();

    const auto all_ones = std::all_of(std::cbegin(v), std::cend(v), [](const auto& val) { return *val == 1; });
    return all_ones;
}

bool test_rvalue_ref_return()
{
    struct Foo
    {
        Foo() = default;
        Foo(Foo&&) = default;
        Foo& operator=(Foo&&) = default;

        Foo(const Foo&) = delete;
        Foo& operator=(const Foo&) = delete;
    };

    auto make_foo = []() -> Foo&&
    {
        Foo f;
        return std::move(f);
    };
    
    // Here we just want to check whether it will compile -> Foo with deleted copy ctor and operator=
    Foo foo{ sync_call(make_foo) };

    return true;
}

bool test_same_callable_type_different_syncs()
{
    auto concurrent_add = [](int& val)
    {
        val += 3;
    };

    int val{};

    constexpr auto val1_sum_tag = 1u;
    constexpr auto val2_sum_tag = 2u;

    auto fun = [concurrent_add, &val]
    {
        for(auto i = 0u; i < 30000; ++i)
        {
            sync_call<val1_sum_tag>(concurrent_add, val);
        }
    };

    auto fun2 = [concurrent_add, &val]
    {
        for(auto i = 0u; i < 30000; ++i)
        {
            sync_call<val2_sum_tag>(concurrent_add, val);
        }
    };

    auto t1 = std::thread{ fun };
    auto t2 = std::thread{ fun2 };

    t1.join();
    t2.join();

    return val != 30000*2;
}

bool test_different_types_same_sync()
{
    uint64_t val{};

    auto concurrent_add_float = [&val](float)
    {
        val += 3;
    };

    auto concurrent_add_bool = [&val](bool)
    {
        val += 3;
    };

    constexpr auto call_tag = 0u;

    auto fun = [concurrent_add_float, &val]
    {
        for(auto i = 0u; i < 30000; ++i)
        {
            sync_call<call_tag>(concurrent_add_float, float{});
        }
    };

    auto fun2 = [concurrent_add_bool, &val]
    {
        for(auto i = 0u; i < 30000; ++i)
        {
            sync_call<call_tag>(concurrent_add_bool, bool{});
        }
    };

    auto t1 = std::thread{ fun };
    auto t2 = std::thread{ fun2 };

    t1.join();
    t2.join();

    return val == 2*30000*3;
}

bool test_not_default_constructible()
{
    struct Foo
    {
        Foo() = delete;
        Foo(int) {}
        Foo(const Foo&) = default;
        Foo& operator=(const Foo&) = default;
        Foo(Foo&&) = default;
        Foo& operator=(Foo&&) = default;
    };

    auto make_foo = []()
    {
        return Foo{ int{} };
    };
    
    // Here we just want to check whether it will compile -> Foo with deleted default ctor
    Foo foo{ sync_call(make_foo) };

    return true;
}

template <typename Test>
void test(Test&& test_fun, std::string_view name)
{
    const auto passed = test_fun();
    std::cout << "[" << name << "] " << (passed ? " Git majonez (:" : " Not git majonez :(") << "\n";
}

int main()
{
    test(test_sum, "test_sum");
    test(test_reference_return, "test_reference_return");
    test(test_rvalue_ref_return, "test_rvalue_ref_return");
    test(test_same_callable_type_different_syncs, "test_same_callable_type_different_syncs");
    test(test_different_types_same_sync, "test_different_types_same_sync");
    test(test_not_default_constructible, "test_not_default_constructible");
}
