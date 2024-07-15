#include <boost/ut.hpp>

#include <vector>
#include <poolparty/pool.hpp>

using namespace boost::ut;

struct priority_task
{
    std::function<void()> callback;
    std::size_t priority;

  public:
    auto operator()()
    {
        return std::invoke(callback);
    }

    bool operator<(const priority_task &other) const
    {
        return priority < other.priority;
    }
};

template <typename... Ts>
struct poolparty::interface<std::priority_queue<Ts...>>
{
    using queue_t = std::priority_queue<Ts...>;

    template <typename... As>
    static void emplace(queue_t &queue, As &&...arguments)
    {
        queue.emplace(std::forward<As>(arguments)...);
    };

    static auto pop(queue_t &queue)
    {
        auto rtn = std::move(queue.top());
        queue.pop();

        return rtn;
    };
};

// NOLINTNEXTLINE
suite<"priority"> priority_test = []()
{
    poolparty::pool<std::priority_queue, priority_task> pool{1};

    expect(eq(pool.size(), 1));
    expect(eq(pool.tasks(), 0));

    pool.pause();

    std::vector<int> order;

    auto fut0 = pool.submit<false, 1>([&]() { order.emplace_back(1); });
    auto fut1 = pool.submit<false, 2>([&]() { order.emplace_back(2); });
    auto fut2 = pool.submit<false, 3>([&]() { order.emplace_back(3); });
    auto fut5 = pool.submit<false, 6>([&]() { order.emplace_back(6); });
    auto fut3 = pool.submit<false, 4>([&]() { order.emplace_back(4); });
    auto fut4 = pool.submit<false, 5>([&]() { order.emplace_back(5); });

    pool.resume();

    fut0.wait();
    fut1.wait();
    fut2.wait();
    fut3.wait();
    fut4.wait();
    fut5.wait();

    expect(eq(order[0], 6));
    expect(eq(order[1], 5));
    expect(eq(order[2], 4));
    expect(eq(order[3], 3));
    expect(eq(order[4], 2));
    expect(eq(order[5], 1));

    expect(eq(pool.tasks(), 0));
};
