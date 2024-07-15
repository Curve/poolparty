#include <boost/ut.hpp>
#include <poolparty/pool.hpp>

using namespace boost::ut;

// NOLINTNEXTLINE
suite<"basic"> basic_test = []()
{
    poolparty::pool pool{};

    expect(eq(pool.tasks(), 0));
    expect(eq(pool.size(), std::thread::hardware_concurrency()));

    pool.pause();

    expect(pool.paused());

    auto fut1 = pool.submit([](int x) { return x + 1; }, 1);
    auto fut2 = pool.submit([](int x) { return x - 1; }, 1);

    expect(eq(pool.tasks(), 2));

    pool.resume();

    expect(eq(fut1.get(), 2));
    expect(eq(fut2.get(), 0));

    {
        std::promise<bool> done;

        pool.submit<true>([&]() { done.set_value(true); });
        expect(done.get_future().get());
    }

    {
        std::promise<bool> done;

        pool.emplace([&]() { done.set_value(true); });
        expect(done.get_future().get());
    }

    auto source = pool.add_thread();
    expect(eq(pool.size(), std::thread::hardware_concurrency() + 1));

    source.request_stop();
    expect(eq(pool.size(), std::thread::hardware_concurrency() + 1));

    pool.cleanup();
    expect(eq(pool.size(), std::thread::hardware_concurrency()));
};
